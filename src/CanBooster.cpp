#include "CanBooster.h"
#include "pins.h"
#include "config.h"

void CanBooster_begin() {
    ACAN_ESP32_Settings settings(CAN_BOOSTER_BITRATE);
    settings.mTxPin = PIN_CAN_TX;
    settings.mRxPin = PIN_CAN_RX;

    uint32_t err = ACAN_ESP32::can.begin(settings);
    if (err != 0) {
        Serial.printf("CAN error 0x%X\n", err);
    } else {
        Serial.println("CAN Booster OK");
    }
}

bool CanBooster_sendDccBit(uint8_t bit, uint8_t phase) {
    CANMessage msg;
    msg.id  = 0x100;
    msg.len = 2;
    msg.data[0] = bit;
    msg.data[1] = phase;
    return ACAN_ESP32::can.tryToSend(msg);
}

bool CanBooster_sendCutout(bool local, bool global) {
    CANMessage msg;
    msg.id  = 0x101;
    msg.len = 2;
    msg.data[0] = local  ? 1 : 0;
    msg.data[1] = global ? 1 : 0;
    return ACAN_ESP32::can.tryToSend(msg);
}

bool CanBooster_sendTelemetry(uint16_t mA, uint16_t mV, BoosterState st) {
    CANMessage msg;
    msg.id  = 0x102;
    msg.len = 3;
    msg.data[0] = (uint8_t)(mA / 10);    // x10 mA
    msg.data[1] = (uint8_t)(mV / 100);   // x100 mV
    msg.data[2] = (uint8_t)st;
    return ACAN_ESP32::can.tryToSend(msg);
}

bool CanBooster_sendRailcomAddress(uint16_t addr) {
    CANMessage msg;
    msg.id  = 0x103;
    msg.len = 2;
    msg.data[0] = (uint8_t)(addr & 0xFF);
    msg.data[1] = (uint8_t)(addr >> 8);
    return ACAN_ESP32::can.tryToSend(msg);
}
