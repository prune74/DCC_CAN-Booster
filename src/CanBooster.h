#pragma once
#include <CanUniversal.h>
#include "DccDecoder.h"

enum BoosterState : uint8_t {
    BOOSTER_OK       = 0,
    BOOSTER_FAULT    = 1,
    BOOSTER_OVERHEAT = 2,
    BOOSTER_OFF      = 3
};

// --- Bloc de bits DCC (empaquetage) ----------------------------------------
// Empaquette un bit DCC dans le bloc en cours.
// Renvoie true si le bloc a été envoyé (plein) ou si l'empaquetage s'est bien passé.
bool CanBooster_packDccBit(uint8_t bit, uint8_t phase);

// Envoie immédiatement le bloc partiel en cours (à appeler sur cutout ou idle).
bool CanBooster_flushDccBits();

// --- Mode brut (debug uniquement) -------------------------------------------
bool CanBooster_sendDccBit(uint8_t bit, uint8_t phase);

// --- Événements --------------------------------------------------------------
bool CanBooster_sendCutout(bool local, bool global);
bool CanBooster_sendTelemetry(uint16_t mA, uint16_t mV, BoosterState st);
bool CanBooster_sendRailcomAddress(uint16_t addr);

// Y a-t-il des bits en attente dans le bloc en cours ?
bool CanBooster_hasPendingBits();