#pragma once

// Debug
#define DCCB_DEBUG_SERIAL        1   // 0=off, 1=events clés, 2=verbeux
#define DCCB_MEASURE_STATS       1
#define DCCB_SCOPE_MODE          1   // 1 = oscilloscope DCC sur Serial

// DCC timings (µs)
static const uint32_t DCC_MIN_1 = 40;
static const uint32_t DCC_MAX_1 = 80;
static const uint32_t DCC_MIN_0 = 90;
static const uint32_t DCC_MAX_0 = 150;

static const uint32_t DCC_CUTOUT_THRESHOLD = 300;

// CAN
static const uint32_t CAN_BOOSTER_BITRATE = 500000;

// Queue
static const uint8_t DCC_EVENT_QUEUE_SIZE = 64;

// Télémétrie
#define DCCB_TELEMETRY_PERIOD_MS  1000   // envoi stats toutes les 1s

// FAILSAFE
#define DCCB_FAILSAFE_TIMEOUT_MS   500     // plus de DCC → failsafe
#define DCCB_FAILSAFE_COOLDOWN_MS  1000    // délai avant réarmement
