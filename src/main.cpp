#include <Arduino.h>
#include "pins.h"
#include "config.h"
#include "DccDecoder.h"
#include "CanBooster.h"
#include "Cli.h"

TaskHandle_t taskDccHandle = nullptr;
TaskHandle_t taskCanHandle = nullptr;

// ---------------------------------------------------------------------------
// Tâche DCC (core 0)
// L’ISR fait tout le travail, cette tâche peut servir pour du diag futur
// ---------------------------------------------------------------------------
void taskDcc(void *pvParameters)
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ---------------------------------------------------------------------------
// Tâche CAN (core 1)
// Lit la queue DCC → envoie les trames CAN
// Gère : scope, stats, télémétrie, watchdog, FAILSAFE
// ---------------------------------------------------------------------------
void taskCan(void *pvParameters)
{
    DccEvent ev;
    uint32_t lastDccMs   = millis();
    uint32_t lastStatsMs = millis();

    bool failsafeActive = false;
    uint32_t failsafeSince = 0;

    for (;;)
    {
        // ---------------------------------------------------------
        // 1) Un événement DCC est disponible
        // ---------------------------------------------------------
        if (DccDecoder_getEvent(ev))
        {
            lastDccMs = millis();

            // Si FAILSAFE actif → on réarme
            if (failsafeActive)
            {
                failsafeActive = false;
#if DCCB_DEBUG_SERIAL >= 1
                Serial.println("FAILSAFE: DCC activity detected → booster OK");
#endif
                CanBooster_sendTelemetry(0, 0, BOOSTER_OK);
            }

#if DCCB_SCOPE_MODE
            // Mode oscilloscope : on logge tout
            switch (ev.type)
            {
                case DCC_EVT_BIT:
                    Serial.printf("SCOPE BIT b=%u ph=%u dt=%u\n",
                                  ev.bit, ev.phase, ev.dt_us);
                    break;

                case DCC_EVT_CUTOUT_START:
                    Serial.printf("SCOPE CUTOUT_START dt=%u\n", ev.dt_us);
                    break;

                case DCC_EVT_CUTOUT_END:
                    Serial.printf("SCOPE CUTOUT_END dt=%u\n", ev.dt_us);
                    break;
            }
#endif

            // ---------------------------------------------------------
            // Envoi CAN normal
            // ---------------------------------------------------------
            switch (ev.type)
            {
                case DCC_EVT_BIT:
                    CanBooster_sendDccBit(ev.bit, ev.phase);
#if DCCB_DEBUG_SERIAL >= 2 && !DCCB_SCOPE_MODE
                    Serial.printf("BIT %u phase=%u dt=%u\n",
                                  ev.bit, ev.phase, ev.dt_us);
#endif
                    break;

                case DCC_EVT_CUTOUT_START:
                    CanBooster_sendCutout(true, true);
#if DCCB_DEBUG_SERIAL >= 1 && !DCCB_SCOPE_MODE
                    Serial.println("CUTOUT START");
#endif
                    break;

                case DCC_EVT_CUTOUT_END:
                    CanBooster_sendCutout(false, false);
#if DCCB_DEBUG_SERIAL >= 1 && !DCCB_SCOPE_MODE
                    Serial.println("CUTOUT END");
#endif
                    break;
            }

            digitalWrite(PIN_LED, !digitalRead(PIN_LED));
        }

        // ---------------------------------------------------------
        // 2) Aucun événement DCC → watchdog + stats + FAILSAFE
        // ---------------------------------------------------------
        else
        {
            uint32_t now = millis();

            // -------------------------
            // FAILSAFE : perte DCC
            // -------------------------
            if (!failsafeActive && (now - lastDccMs > DCCB_FAILSAFE_TIMEOUT_MS))
            {
                failsafeActive = true;
                failsafeSince = now;

#if DCCB_DEBUG_SERIAL >= 1
                Serial.println("FAILSAFE: DCC lost → sending global cutout + booster OFF");
#endif

                // Couper globalement
                CanBooster_sendCutout(true, true);

                // Booster OFF
                CanBooster_sendTelemetry(0, 0, BOOSTER_OFF);
            }

            // Réarmement automatique
            if (failsafeActive && (now - failsafeSince > DCCB_FAILSAFE_COOLDOWN_MS))
            {
                if (now - lastDccMs < DCCB_FAILSAFE_TIMEOUT_MS)
                {
                    failsafeActive = false;

#if DCCB_DEBUG_SERIAL >= 1
                    Serial.println("FAILSAFE: DCC restored → booster OK");
#endif

                    CanBooster_sendTelemetry(0, 0, BOOSTER_OK);
                }
            }

            // -------------------------
            // Stats + télémétrie CAN
            // -------------------------
#if DCCB_MEASURE_STATS
            if (now - lastStatsMs > DCCB_TELEMETRY_PERIOD_MS)
            {
                uint32_t b0, b1, co, bad;
                DccDecoder_getStats(b0, b1, co, bad);

#if DCCB_DEBUG_SERIAL >= 1
                Serial.printf("STATS: b0=%lu b1=%lu cutout=%lu bad=%lu\n",
                              b0, b1, co, bad);
#endif

                uint16_t pseudoCurrent = (uint16_t)((b0 + b1) & 0xFFFF);
                uint16_t pseudoVoltage = (uint16_t)(co & 0xFFFF);
                BoosterState st = (bad > 0) ? BOOSTER_FAULT : BOOSTER_OK;

                CanBooster_sendTelemetry(pseudoCurrent, pseudoVoltage, st);

                lastStatsMs = now;
            }
#endif

            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

// ---------------------------------------------------------------------------
// SETUP
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(200);

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    Serial.println("\nDCC_CANBooster FreeRTOS - 2 cores");

    DccDecoder_begin();
    CanBooster_begin();
    Cli_begin();

    // Tâche DCC sur core 0
    xTaskCreatePinnedToCore(
        taskDcc, "DCC", 4096, nullptr, 2, &taskDccHandle, 0);

    // Tâche CAN sur core 1
    xTaskCreatePinnedToCore(
        taskCan, "CAN", 4096, nullptr, 3, &taskCanHandle, 1);
}

// ---------------------------------------------------------------------------
// LOOP : uniquement CLI
// ---------------------------------------------------------------------------
void loop()
{
    Cli_task();
    vTaskDelay(pdMS_TO_TICKS(10));
}
