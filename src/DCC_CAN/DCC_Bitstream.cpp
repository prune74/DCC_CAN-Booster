#include "DCC_Bitstream.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Queues fournies par Task.cpp
extern QueueHandle_t dccEdgesQueue;
extern QueueHandle_t dccBitsQueue;

// Durées typiques DCC
static const uint32_t DCC_ONE_MIN = 52;   // µs
static const uint32_t DCC_ONE_MAX = 70;   // µs
static const uint32_t DCC_ZERO_MIN = 90;  // µs
static const uint32_t DCC_ZERO_MAX = 120; // µs

void DCC_Bitstream::begin() {
    // Rien à initialiser ici : la tâche sera lancée dans Task.cpp
}

void DCC_Bitstream::task(void *pvParameters) {
    (void)pvParameters;

    DCCEdge prevEdge = {0, false};
    bool firstEdge = true;

    for (;;) {
        DCCEdge edge;

        // Attente d’un front venant de l’ISR
        if (xQueueReceive(dccEdgesQueue, &edge, portMAX_DELAY) == pdTRUE) {

            if (!firstEdge) {
                uint32_t duration = edge.timestamp - prevEdge.timestamp;

                DCCBitItem bitItem;

                // Détection du bit 1
                if (duration >= DCC_ONE_MIN && duration <= DCC_ONE_MAX) {
                    bitItem.bit = DCCBit::BIT_1;
                }
                // Détection du bit 0
                else if (duration >= DCC_ZERO_MIN && duration <= DCC_ZERO_MAX) {
                    bitItem.bit = DCCBit::BIT_0;
                }
                else {
                    bitItem.bit = DCCBit::INVALID;
                }

                bitItem.duration = duration;

                // Envoi dans la queue des bits
                xQueueSend(dccBitsQueue, &bitItem, 0);
            }

            prevEdge = edge;
            firstEdge = false;
        }
    }
}
