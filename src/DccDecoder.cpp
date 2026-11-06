#include "DccDecoder.h"
#include "Pins.h"
#include "Config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// ---------------------------------------------------------------------------
// Décodeur DCC — mesure des demi-bits (durées entre fronts sur PIN_DCC_IN)
// ---------------------------------------------------------------------------
// Principe (NMRA S-9.1) :
//  - Un bit DCC est fait de DEUX demi-bits identiques :
//      bit "1" = 2 × 58 µs, bit "0" = 2 × 100 µs (ou étiré : zéro-stretching).
//  - On mesure la durée dt entre deux fronts (CHANGE) :
//      dt <= 85 µs        → demi-bit "1"
//      85 < dt < 300 µs   → demi-bit "0"
//      dt >= 300 µs       → cutout (si le demi-bit précédent était un "1")
//                           ou demi-bit "0" étiré (sinon)
//  - UN SEUL événement DCC_EVT_BIT est émis par bit COMPLET (à la fermeture
//    de la paire de demi-bits). C'est essentiel : un événement par demi-bit
//    donnerait un bitstream à débit double côté CAN.
//  - Cutout : le gros trou prolonge le second demi-bit du dernier "1".
//    On clôture la paire en cours, on émet CUTOUT_START puis CUTOUT_END,
//    puis on RE-SYNCHRONISE (le front de reprise ouvre le premier demi-bit
//    du bit suivant : il ne doit pas être classifié avec le dt du trou).
// ---------------------------------------------------------------------------

static volatile uint32_t lastEdge = 0;

// Paire de demi-bits en cours
static volatile bool   pairOpen = false; // 1er demi-bit reçu, 2e attendu
static volatile uint8_t pairBit = 0;     // classe du 1er demi-bit (0/1)

// Garde anti faux-cutout : le cutout NMRA suit toujours un demi-bit "1"
static volatile bool lastHalfWasOne = false;

// Parité de debug (alterne par bit émis — information indicative uniquement)
static volatile uint8_t bitPhase = 0;

static QueueHandle_t dccQueue = nullptr;

// Stats
#if DCCB_MEASURE_STATS
static volatile uint32_t s_bit0Count   = 0;
static volatile uint32_t s_bit1Count   = 0;
static volatile uint32_t s_cutoutCount = 0;
static volatile uint32_t s_badTiming   = 0;
#endif

// Événements perdus : queue pleine → événement jeté. Il faut le savoir !
static volatile uint32_t s_droppedEvents = 0;

static void IRAM_ATTR sendEventFromISR(const DccEvent &ev) {
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    if (xQueueSendFromISR(dccQueue, (void*)&ev, &higherPriorityTaskWoken) != pdTRUE) {
        s_droppedEvents++;   // queue pleine : événement perdu, comptabilisé
    }
    if (higherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

// Émission d'un bit complet (paire fermée)
static void IRAM_ATTR emitBit(uint8_t bit) {
    DccEvent ev;
    ev.type  = DCC_EVT_BIT;
    ev.bit   = bit;
    ev.phase = bitPhase;
    ev.dt_us = 0; // rempli par l'appelant si besoin (debug)
    bitPhase ^= 1;

#if DCCB_MEASURE_STATS
    if (bit) s_bit1Count++; else s_bit0Count++;
#endif
    sendEventFromISR(ev);
}

// Classification d'un demi-bit et fermeture de paire le cas échéant
static void IRAM_ATTR processHalf(uint8_t half) {
    lastHalfWasOne = (half == 1);

    if (!pairOpen) {
        // Premier demi-bit d'une nouvelle paire : mémoriser sa classe
        pairOpen = true;
        pairBit  = half;
        return;
    }

    // Second demi-bit : identique au premier par construction DCC.
    if (half != pairBit) {
#if DCCB_MEASURE_STATS
        s_badTiming++;   // désynchronisation (bruit) : on garde le 1er demi
#endif
    }
    pairOpen = false;
    emitBit(pairBit);
}

static void IRAM_ATTR dccISR() {
    uint32_t now = micros();
    uint32_t dt  = now - lastEdge;
    lastEdge = now;

    // -------------------------------------------------------------------
    // 1) Glitch : trop court → ignoré (stats seulement)
    // -------------------------------------------------------------------
    if (dt < DCCB_TIMING_GLITCH_US) {
#if DCCB_MEASURE_STATS
        s_badTiming++;
#endif
        return;
    }

    // -------------------------------------------------------------------
    // 2) Gros trou : cutout (après un demi-bit "1") ou zéro-stretching
    // -------------------------------------------------------------------
    if (dt >= DCCB_TIMING_CUTOUT_START_US) {

        if (lastHalfWasOne) {
            // Cutout NMRA : il prolonge le second demi-bit du dernier "1".
            // Clôturer la paire en cours si elle est ouverte.
            if (pairOpen) {
                pairOpen = false;
                emitBit(pairBit);
            }

            DccEvent ev;
            ev.type  = DCC_EVT_CUTOUT_START;
            ev.bit   = 0;
            ev.phase = bitPhase;
            ev.dt_us = dt;
            sendEventFromISR(ev);

            // Le cutout est borné par le front de reprise (celui-ci) :
            // START + END immédiats, le booster aval régénère un cutout
            // nominal (454 µs) à cet endroit.
            ev.type = DCC_EVT_CUTOUT_END;
            sendEventFromISR(ev);

#if DCCB_MEASURE_STATS
            s_cutoutCount++;
#endif
            lastHalfWasOne = false;
            return;   // le front courant ouvre le 1er demi-bit du bit suivant
        }

        // Zéro-stretching (traction DC) : demi-bit "0" étiré, PAS un cutout
        processHalf(0);
        return;
    }

    // -------------------------------------------------------------------
    // 3) Demi-bit normal : classification par point médian
    // -------------------------------------------------------------------
    processHalf(dt <= DCCB_TIMING_MIDPOINT_US ? 1 : 0);
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

uint32_t DccDecoder_getDroppedEvents() {
    return s_droppedEvents;
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