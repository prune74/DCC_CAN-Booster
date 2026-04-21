#pragma once
#include <Arduino.h>

enum DccEventType : uint8_t {
    DCC_EVT_BIT,
    DCC_EVT_CUTOUT_START,
    DCC_EVT_CUTOUT_END
};

struct DccEvent {
    DccEventType type;
    uint8_t bit;      // 0/1 si BIT
    uint8_t phase;    // 0/1
    uint16_t dt_us;   // durée mesurée (debug/stats)
};

void DccDecoder_begin();
bool DccDecoder_getEvent(DccEvent &ev);

void DccDecoder_getStats(uint32_t &bit0Count, uint32_t &bit1Count,
                         uint32_t &cutoutCount, uint32_t &badTiming);
