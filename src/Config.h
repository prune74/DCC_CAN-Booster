#pragma once

// Debug
#define DCCB_DEBUG_SERIAL        1   // 0=off, 1=events clés, 2=verbeux
#define DCCB_MEASURE_STATS       1

// DCC timings (µs)
static const uint32_t DCC_MIN_1 = 40;
static const uint32_t DCC_MAX_1 = 80;
static const uint32_t DCC_MIN_0 = 90;
static const uint32_t DCC_MAX_0 = 150;

// Seuils cutout
static const uint32_t DCC_CUTOUT_THRESHOLD = 300;   // > → début cutout
static const uint32_t DCC_CUTOUT_END_GAP   = 80;    // non utilisé ici mais dispo

// CAN
static const uint32_t CAN_BOOSTER_BITRATE = 500000;

// Queue
static const uint8_t DCC_EVENT_QUEUE_SIZE = 64;
