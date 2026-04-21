#include <Arduino.h>
#include "pins.h"
#include "config.h"
#include "DccDecoder.h"
#include "CanBooster.h"

TaskHandle_t taskDccHandle = nullptr;
TaskHandle_t taskCanHandle = nullptr;

// Tâche DCC (core 0) : l’ISR fait le boulot, ici on peut ajouter du diag si besoin
void taskDcc(void *pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Tâche CAN (core 1) : lit la queue et envoie les trames
void taskCan(void *pvParameters) {
    DccEvent ev;
    uint32_t lastDccMs   = millis();
    uint32_t lastStatsMs = millis();

    for (;;) {
        if (DccDecoder_getEvent(ev)) {
            lastDccMs = millis();

            switch (ev.type) {
                case DCC_EVT_BIT:
                    CanBooster_sendDccBit(ev.bit, ev.phase);
#if DCCB_DEBUG_SERIAL >= 2
                    Serial.printf("BIT %u phase=%u dt=%u\n", ev.bit, ev.phase, ev.dt_us);
#endif
                    break;

                case DCC_EVT_CUTOUT_START:
                    CanBooster_sendCutout(true, true);
#if DCCB_DEBUG_SERIAL >= 1
                    Serial.println("CUTOUT START");
#endif
                    break;

                case DCC_EVT_CUTOUT_END:
                    CanBooster_sendCutout(false, false);
#if DCCB_DEBUG_SERIAL >= 1
                    Serial.println("CUTOUT END");
#endif
                    break;
            }

            digitalWrite(PIN_LED, !digitalRead(PIN_LED));
        } else {
            // Watchdog DCC
            if (millis() - lastDccMs > 500) {
#if DCCB_DEBUG_SERIAL >= 1
                Serial.println("WARN: no DCC for 500ms");
#endif
                lastDccMs = millis();
            }

#if DCCB_MEASURE_STATS
            if (millis() - lastStatsMs > 1000) {
                uint32_t b0, b1, co, bad;
                DccDecoder_getStats(b0, b1, co, bad);
#if DCCB_DEBUG_SERIAL >= 1
                Serial.printf("STATS: b0=%lu b1=%lu cutout=%lu bad=%lu\n",
                              b0, b1, co, bad);
#endif
                lastStatsMs = millis();
            }
#endif
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    Serial.println("\nDCC_CANBooster FreeRTOS - 2 cores");

    DccDecoder_begin();
    CanBooster_begin();

    // Tâche DCC sur core 0
    xTaskCreatePinnedToCore(
        taskDcc,
        "DCC",
        4096,
        nullptr,
        2,
        &taskDccHandle,
        0
    );

    // Tâche CAN sur core 1
    xTaskCreatePinnedToCore(
        taskCan,
        "CAN",
        4096,
        nullptr,
        3,
        &taskCanHandle,
        1
    );
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
