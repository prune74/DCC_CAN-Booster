#include <Arduino.h>
#include "Pins.h"
#include "Config.h"
#include "BoosterCanConfig.h"
#include "DccDecoder.h"
#include "CanBooster.h"
#include "Cli.h"
#include <CanUniversal.h>

TaskHandle_t taskCanHandle    = nullptr;
TaskHandle_t taskCanRxHandle  = nullptr;

volatile bool canMonitorEnabled = false;
volatile int32_t canMonitorFilter = -1;

BoosterCanConfig boosterCanConfig;

// ---------------------------------------------------------------------------
// Tâche CAN (core 1) : événements DCC → bus CAN + failsafe + télémétrie
// ---------------------------------------------------------------------------
// Depuis la refonte du décodeur, un événement = UN BIT COMPLET (paire de
// demi-bits) ou un cutout (START+END consécutifs). Le débit d'événements
// est donc ≤ ~17 000 bits/s, très confortable pour la queue (64 entrées).
// ---------------------------------------------------------------------------
void taskCan(void *pvParameters) {
    DccEvent ev;
    uint32_t lastDccMs   = millis();
    uint32_t lastStatsMs = millis();
    uint32_t lastBitUs   = micros();

    bool failsafeActive = false;

    for (;;) {

        // -------------------------------------------------------------------
        // 1) Événements DCC disponibles
        // -------------------------------------------------------------------
        bool hadEvent = false;

        while (DccDecoder_getEvent(ev)) {
            hadEvent = true;
            lastDccMs = millis();
            lastBitUs = micros();

            if (failsafeActive) {
                failsafeActive = false;
                CanBooster_sendTelemetry(0, 0, BOOSTER_OK);
            }

            switch (ev.type) {
                case DCC_EVT_BIT:
#if DCCB_SEND_RAW_BITS
                    CanBooster_sendDccBit(ev.bit, ev.phase);
#else
                    CanBooster_packDccBit(ev.bit, ev.phase);
#endif
                    break;

                case DCC_EVT_CUTOUT_START:
                    CanBooster_sendCutout(true, true);
                    break;

                case DCC_EVT_CUTOUT_END:
                    CanBooster_sendCutout(false, false);
                    break;
            }
        }

        if (hadEvent) {
            digitalWrite(PIN_LED, !digitalRead(PIN_LED));
            taskYIELD();   // ne pas affamer taskCanRx (prio inférieure)
        }

        // -------------------------------------------------------------------
        // 2) Anti-latence : bloc partiel envoyé si le DCC se tait un instant
        // -------------------------------------------------------------------
#if !DCCB_SEND_RAW_BITS
        if (CanBooster_hasPendingBits()) {
            uint32_t nowUs = micros();
            if (nowUs - lastBitUs > DCCB_BLOCK_FLUSH_TIMEOUT_US) {
                CanBooster_flushDccBits();
            }
        }
#endif

        // -------------------------------------------------------------------
        // 3) Failsafe : perte du signal DCC
        // -------------------------------------------------------------------
        // CORRECTION : plus de trame cutout ici — l'ID 0x101 est réservé au
        // cutout réel du signal. L'état "rails coupés" est porté par la
        // télémétrie (BOOSTER_OFF), sans ambiguïté pour le booster aval.
        // -------------------------------------------------------------------
        uint32_t now = millis();

        if (!failsafeActive && (now - lastDccMs > DCCB_FAILSAFE_TIMEOUT_MS)) {
            failsafeActive = true;
            CanBooster_flushDccBits();                    // ne rien perdre
            CanBooster_sendTelemetry(0, 0, BOOSTER_OFF);
        }

        // -------------------------------------------------------------------
        // 4) Télémétrie périodique (valeurs factices pour l'instant)
        // -------------------------------------------------------------------
#if DCCB_MEASURE_STATS
        if (now - lastStatsMs > DCCB_TELEMETRY_PERIOD_MS) {
            uint32_t b0, b1, co, bad;
            DccDecoder_getStats(b0, b1, co, bad);

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

// ---------------------------------------------------------------------------
// Tâche CAN RX (sniffer)
// ---------------------------------------------------------------------------
void taskCanRx(void *pvParameters) {
    CanMsg msg;

    for (;;) {
        if (CanBus::bus(0).receive(msg)) {

            if (canMonitorEnabled) {
                if (canMonitorFilter == -1 || msg.id == (uint32_t)canMonitorFilter) {

                    Serial.printf("[CAN RX] ID=0x%03X LEN=%u DATA=",
                                  msg.id, msg.dlc);

                    for (uint8_t i = 0; i < msg.dlc; i++)
                        Serial.printf("%02X ", msg.data[i]);

                    Serial.println();
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ---------------------------------------------------------------------------
// SETUP
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(200);

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    Serial.println("\nDCC_CANBooster — prune74/CanUniversal");

    DccDecoder_begin();

    // Bibliothèque CanUniversal (registre) via le provider du booster
    CanInit::begin(boosterCanConfig);
    if (!CanBus::bus(0).isValid()) {
        Serial.println("[ERREUR] CAN0 invalide — vérifiez le transceiver.");
    }

    Cli_begin();

    // Le décodage se fait en interruption, la consolidation dans
    // taskCan (core 1). Le WiFi/BT restent désactivés (jamais initialisés)
    // pour ne pas ajouter de gigue sur les interruptions GPIO.
    xTaskCreatePinnedToCore(taskCan,   "CAN",    4096, nullptr, 3, &taskCanHandle,   1);
    xTaskCreatePinnedToCore(taskCanRx, "CAN_RX", 4096, nullptr, 1, &taskCanRxHandle, 1);
}

// ---------------------------------------------------------------------------
// LOOP (CLI)
// ---------------------------------------------------------------------------
void loop() {
    Cli_task();
    vTaskDelay(pdMS_TO_TICKS(10));
}