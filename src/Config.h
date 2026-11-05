#pragma once

// ============================================================================
//  Profil standardisé pour modules "Convertisseur DCC → CAN" 
//  Ce fichier définit :
//   - IDs CAN officiels
//   - timings DCC validés
//   - comportement failsafe
//   - options debug recommandées
//   - télémétrie standard
// ============================================================================

// ---------------------------------------------------------------------------
// CAN IDs
// ---------------------------------------------------------------------------
#define DCCB_CAN_ID_DCC_BLOCK 0x100 // BLOC de bits DCC (jusqu'à 32 bits)
                                    // data[0] = nombre de bits N (1..32)
                                    // bits suivants : 4 par octet,
                                    // chaque nibble = (bit << 1) | phase
#define DCCB_CAN_ID_CUTOUT 0x101    // cutout local/global
#define DCCB_CAN_ID_TELEMETRY 0x102 // télémétrie booster
#define DCCB_CAN_ID_RAILCOM 0x103   // RailCom (EXCC)

// ---------------------------------------------------------------------------
// Mode d'envoi des bits DCC
// ---------------------------------------------------------------------------
// 0 = empaquetage (défaut, obligatoire : un bit par trame sature le bus —
//     le DCC produit jusqu'à ~15 000 bits/s, un bus CAN 500 kbit/s n'accepte
//     que ~4 400 trames/s)
// 1 = brut (debug uniquement : une trame par bit, comme avant)
#define DCCB_SEND_RAW_BITS 0

// Nombre de bits DCC par trame CAN en mode empaqueté (max 32, multiple de 4)
#define DCCB_BITS_PER_BLOCK 32

// Anti-latence : si aucun bit n'arrive pendant ce délai (µs), le bloc
// partiel est envoyé tel quel (fin de trame DCC, etc.)
#define DCCB_BLOCK_FLUSH_TIMEOUT_US 500

// ---------------------------------------------------------------------------
// Timings DCC validés
// ---------------------------------------------------------------------------
#define DCCB_TIMING_BIT1_MIN_US 40
#define DCCB_TIMING_BIT1_MAX_US 80
#define DCCB_TIMING_BIT0_MIN_US 90
#define DCCB_TIMING_BIT0_MAX_US 150

// Détection cutout
#define DCCB_TIMING_CUTOUT_START_US 300 // > → début cutout
#define DCCB_TIMING_CUTOUT_END_GAP 80   // marge pour fin cutout

// ---------------------------------------------------------------------------
// CAN Booster
// ---------------------------------------------------------------------------
#define DCCB_CAN_BITRATE 500000 // 500 kbps standard

// ---------------------------------------------------------------------------
// Failsafe
// ---------------------------------------------------------------------------
#define DCCB_FAILSAFE_TIMEOUT_MS 500   // perte DCC → OFF
#define DCCB_FAILSAFE_COOLDOWN_MS 1000 // réarmement

// ---------------------------------------------------------------------------
// Télémétrie
// ---------------------------------------------------------------------------
#define DCCB_TELEMETRY_PERIOD_MS 1000 // envoi toutes les 1s

// ---------------------------------------------------------------------------
// Options debug recommandées
// ---------------------------------------------------------------------------
#define DCCB_DEBUG_SERIAL 1 // 0=off, 1=normal, 2=verbeux
#define DCCB_MEASURE_STATS 1
#define DCCB_SCOPE_MODE 0 // 1 = oscilloscope DCC

// ---------------------------------------------------------------------------
// Taille de la queue d'événements DCC
// ---------------------------------------------------------------------------
#define DCC_EVENT_QUEUE_SIZE 64