#pragma once

#include <Arduino.h>

/* ============================================================
   Types de base pour le pipeline DCC → CAN Booster Discovery
   ============================================================ */

/* -----------------------------
   1. Fronts DCC (ISR → Bitstream)
   ----------------------------- */
struct DCCEdge {
    uint32_t timestamp;   // micros() du front
    bool level;           // HIGH ou LOW
};


/* -----------------------------------
   2. Bits DCC (Bitstream → PacketParser)
   ----------------------------------- */
enum class DCCBit : uint8_t {
    BIT_0 = 0,
    BIT_1 = 1,
    INVALID = 2
};

struct DCCBitItem {
    DCCBit bit;
    uint32_t duration;    // durée du bit en µs (debug / supervision)
};


/* -----------------------------------
   3. Paquets DCC (PacketParser → CAN)
   ----------------------------------- */
struct DCCPacket {
    uint8_t size;         // nombre d’octets valides
    uint8_t data[6];      // paquet DCC complet (max 6 octets)
    bool validCRC;        // CRC OK ?
};


/* -----------------------------------
   4. État du booster (Supervision → CAN)
   ----------------------------------- */
enum class BoosterState : uint8_t {
    OK = 0,
    NO_DCC_SIGNAL,
    CAN_LOST,
    CUTOUT_ACTIVE,
    OVERCURRENT,
    OVERTEMP
};

struct BoosterStatus {
    BoosterState state;
    uint32_t lastDCCMicros;     // dernier bit reçu
    uint32_t lastCANMicros;     // dernière trame CAN Booster reçue
    bool cutoutLocal;
    bool cutoutGlobal;
};


/* -----------------------------------
   5. Trames CAN Booster Discovery
   ----------------------------------- */
enum class CANBoosterFrameType : uint8_t {
    DCC_BIT = 0,      // 0x100
    CUTOUT = 1,       // 0x101
    TELEMETRY = 2,    // 0x102
    RAILCOM = 3       // 0x103
};
