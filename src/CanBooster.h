#pragma once
#include <ACAN_ESP32.h>
#include "DccDecoder.h"

enum BoosterState : uint8_t {
    BOOSTER_OK       = 0,
    BOOSTER_FAULT    = 1,
    BOOSTER_OVERHEAT = 2,
    BOOSTER_OFF      = 3
};

void CanBooster_begin();

bool CanBooster_sendDccBit(uint8_t bit, uint8_t phase);
bool CanBooster_sendCutout(bool local, bool global);
bool CanBooster_sendTelemetry(uint16_t mA, uint16_t mV, BoosterState st);
bool CanBooster_sendRailcomAddress(uint16_t addr);
