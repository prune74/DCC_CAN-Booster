#include "DCC_CANEncoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Queue fournie par Task.cpp
extern QueueHandle_t dccPacketsQueue;

// IDs à mettre plus tard dans Config.h
static const uint32_t CAN_ID_DCC_BIT   = 0x100;
static const uint32_t CAN_ID_CUTOUT    = 0x101;
static const uint32_t CAN_ID_TELEMETRY = 0x102;
static const uint32_t CAN_ID_RAILCOM   = 0x103;

void DCC_CANEncoder::begin() {
    // Rien ici : la tâche sera créée dans Task.cpp
}

void DCC_CANEncoder::task(void *pvParameters) {
    (void)pvParameters;

    for (;;) {
        DCCPacket pkt;

        if (xQueueReceive(dccPacketsQueue, &pkt, portMAX_DELAY) == pdTRUE) {
            if (!pkt.validCRC) {
                continue;
            }

            CANMessage frame;
            frame.ext = true;
            frame.len = pkt.size;

            // Exemple simple : on envoie le paquet brut sur 0x100
            frame.id = CAN_ID_DCC_BIT;
            for (uint8_t i = 0; i < pkt.size; i++) {
                frame.data[i] = pkt.data[i];
            }

            ACAN_ESP32::can.tryToSend(frame);
        }
    }
}
