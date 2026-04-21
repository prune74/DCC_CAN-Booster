#include "DCC_PacketParser.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Queues fournies par Task.cpp
extern QueueHandle_t dccBitsQueue;
extern QueueHandle_t dccPacketsQueue;

void DCC_PacketParser::begin() {
    // Rien à initialiser ici : la tâche sera lancée dans Task.cpp
}

void DCC_PacketParser::task(void *pvParameters) {
    (void)pvParameters;

    uint8_t packet[6];
    uint8_t bitCount = 0;
    uint8_t byteCount = 0;
    bool inPacket = false;

    for (;;) {
        DCCBitItem bitItem;

        // Attente d’un bit venant de DCC_Bitstream
        if (xQueueReceive(dccBitsQueue, &bitItem, portMAX_DELAY) == pdTRUE) {

            if (bitItem.bit == DCCBit::INVALID) {
                // Reset du parsing
                inPacket = false;
                bitCount = 0;
                byteCount = 0;
                continue;
            }

            // Détection du préambule : au moins 10 bits "1"
            if (!inPacket) {
                if (bitItem.bit == DCCBit::BIT_1) {
                    bitCount++;
                    if (bitCount >= 10) {
                        // Préambule détecté → attendre le start bit (0)
                        // On ne reset pas bitCount ici
                    }
                } else { // bit 0
                    if (bitCount >= 10) {
                        // Start bit détecté → début du paquet
                        inPacket = true;
                        bitCount = 0;
                        byteCount = 0;
                    } else {
                        // Pas un vrai préambule
                        bitCount = 0;
                    }
                }
                continue;
            }

            // Reconstruction des octets
            if (bitItem.bit == DCCBit::BIT_1) {
                packet[byteCount] = (packet[byteCount] << 1) | 1;
            } else {
                packet[byteCount] = (packet[byteCount] << 1);
            }

            bitCount++;

            // Un octet = 8 bits
            if (bitCount == 8) {
                bitCount = 0;
                byteCount++;

                // Paquet complet ?
                if (byteCount >= 3 && byteCount <= 6) {
                    // Vérification CRC
                    if (checkCRC(packet, byteCount)) {
                        DCCPacket out;
                        out.size = byteCount;
                        out.validCRC = true;
                        memcpy(out.data, packet, byteCount);

                        xQueueSend(dccPacketsQueue, &out, 0);
                    }
                }

                // Trop long → reset
                if (byteCount >= 6) {
                    inPacket = false;
                    byteCount = 0;
                }
            }
        }
    }
}

bool DCC_PacketParser::checkCRC(const uint8_t *data, uint8_t size) {
    uint8_t xorVal = 0;
    for (uint8_t i = 0; i < size - 1; i++) {
        xorVal ^= data[i];
    }
    return xorVal == data[size - 1];
}
