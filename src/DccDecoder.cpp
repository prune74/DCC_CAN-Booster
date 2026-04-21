#include "DccDecoder.h"
#include "pins.h"
#include "config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static volatile uint32_t lastEdge = 0;
static volatile uint8_t currentPhase = 0;
static volatile bool inCutout = false;

static QueueHandle_t dccQueue = nullptr;

// Stats
#if DCCB_MEASURE_STATS
static volatile uint32_t s_bit0Count   = 0;
static volatile uint32_t s_bit1Count   = 0;
static volatile uint32_t s_cutoutCount = 0;
static volatile uint32_t s_badTiming   = 0;
#endif

static void IRAM_ATTR sendEventFromISR(const DccEvent &ev) {
    xQueueSendFromISR(dccQueue, (void*)&ev, nullptr);
}

static void IRAM_ATTR dccISR() {
    uint32_t now = micros();
    uint32_t dt  = now - lastEdge;
    lastEdge = now;

    DccEvent ev;
    ev.dt_us = dt;
    ev.phase = currentPhase;
    ev.bit   = 0;

    // Début de cutout : gros trou
    if (dt > DCC_CUTOUT_THRESHOLD) {
        if (!inCutout) {
            inCutout = true;
#if DCCB_MEASURE_STATS
            s_cutoutCount++;
#endif
            ev.type = DCC_EVT_CUTOUT_START;
            sendEventFromISR(ev);
        }
        return;
    }

    // Si on était en cutout et qu'on voit un front "normal" → fin de cutout
    if (inCutout) {
        inCutout = false;
        ev.type  = DCC_EVT_CUTOUT_END;
        ev.bit   = 0;
        sendEventFromISR(ev);
        // on continue pour classifier ce front comme bit
    }

    // Bit 1
    if (dt >= DCC_MIN_1 && dt <= DCC_MAX_1) {
        currentPhase ^= 1;
        ev.type  = DCC_EVT_BIT;
        ev.bit   = 1;
        ev.phase = currentPhase;
#if DCCB_MEASURE_STATS
        s_bit1Count++;
#endif
        sendEventFromISR(ev);
        return;
    }

    // Bit 0
    if (dt >= DCC_MIN_0 && dt <= DCC_MAX_0) {
        currentPhase ^= 1;
        ev.type  = DCC_EVT_BIT;
        ev.bit   = 0;
        ev.phase = currentPhase;
#if DCCB_MEASURE_STATS
        s_bit0Count++;
#endif
        sendEventFromISR(ev);
        return;
    }

    // Timing invalide
#if DCCB_MEASURE_STATS
    s_badTiming++;
#endif
}

void DccDecoder_begin() {
    dccQueue = xQueueCreate(DCC_EVENT_QUEUE_SIZE, sizeof(DccEvent));

    pinMode(PIN_DCC_IN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_DCC_IN), dccISR, CHANGE);
}

bool DccDecoder_getEvent(DccEvent &ev) {
    if (dccQueue == nullptr) return false;
    return xQueueReceive(dccQueue, &ev, 0) == pdTRUE;
}

void DccDecoder_getStats(uint32_t &bit0Count, uint32_t &bit1Count,
                         uint32_t &cutoutCount, uint32_t &badTiming) {
#if DCCB_MEASURE_STATS
    bit0Count   = s_bit0Count;
    bit1Count   = s_bit1Count;
    cutoutCount = s_cutoutCount;
    badTiming   = s_badTiming;
#else
    bit0Count = bit1Count = cutoutCount = badTiming = 0;
#endif
}
