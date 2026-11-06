#pragma once

// ============================================================================
//  Profil standardisé pour modules "Convertisseur DCC → CAN" (CFC)
// ============================================================================
// Ce fichier définit :
//  - les IDs CAN officiels
//  - le format des blocs de bits DCC (16 bits max par trame)
//  - les seuils de décodage DCC, alignés NMRA S-9.1
//  - le comportement failsafe
//  - la télémétrie standard
// ============================================================================

// ---------------------------------------------------------------------------
// CAN IDs
// ---------------------------------------------------------------------------
#define DCCB_CAN_ID_DCC_BLOCK 0x100 // BLOC de bits DCC (jusqu'à 16 bits)
                                    // data[0]  = nombre de bits N (1..16)
                                    // data[1..]= bits, 4 par octet,
                                    //            chaque nibble = (bit << 1) | phase
#define DCCB_CAN_ID_CUTOUT 0x101    // cutout { local, global } — cutout RÉEL uniquement
#define DCCB_CAN_ID_TELEMETRY 0x102 // télémétrie booster { mA/10, mV/100, état }
#define DCCB_CAN_ID_RAILCOM 0x103   // réservé (RailCom non distribué par ce module)

// ---------------------------------------------------------------------------
// Mode d'envoi des bits DCC
// ---------------------------------------------------------------------------
// 0 = empaquetage (défaut, obligatoire : le DCC produit jusqu'à ~15 000 bits/s,
//     or un bus CAN 500 kbit/s n'accepte que ~4 400 trames/s)
// 1 = brut (debug uniquement : une trame par bit)
#define DCCB_SEND_RAW_BITS 0

// Nombre maximum de bits DCC par trame CAN.
// Attention : 1 bit = 1 nibble, donc 16 bits = 8 octets de données.
// Ne pas dépasser 16 (data[0] compteur + 8 octets de bits = DLC 9).
#define DCCB_BITS_PER_BLOCK 16

// Anti-latence : si aucun bit n'arrive pendant ce délai (µs), le bloc
// partiel est envoyé tel quel (fin de trame DCC, etc.)
#define DCCB_BLOCK_FLUSH_TIMEOUT_US 500

// ---------------------------------------------------------------------------
// Seuils de décodage DCC — NMRA S-9.1 (demi-bit "1" ≈ 58 µs, "0" ≈ 100 µs)
// ---------------------------------------------------------------------------
// Classification par point médian de la durée ENTRE DEUX FRONT (demi-bit) :
//   dt <  DCCB_TIMING_GLITCH_US       → glitch : ignoré, compté badTiming
//   dt <= DCCB_TIMING_MIDPOINT_US     → demi-bit "1"
//   dt <  DCCB_TIMING_CUTOUT_START_US → demi-bit "0" (100 µs nominal,
//                                        tolérant aux "0" légèrement étirés)
//   dt >= DCCB_TIMING_CUTOUT_START_US :
//     - demi-bit précédent = "1" → CUTOUT (prolonge le dernier bit "1")
//     - sinon               → demi-bit "0" étiré (zéro-stretching, traction DC)
#define DCCB_TIMING_GLITCH_US 25
#define DCCB_TIMING_MIDPOINT_US 85
#define DCCB_TIMING_CUTOUT_START_US 300

// ---------------------------------------------------------------------------
// CAN Booster
// ---------------------------------------------------------------------------
#define DCCB_CAN_BITRATE 500000 // 500 kbps standard

// ---------------------------------------------------------------------------
// Failsafe
// ---------------------------------------------------------------------------
#define DCCB_FAILSAFE_TIMEOUT_MS 500   // perte DCC → BOOSTER_OFF
#define DCCB_FAILSAFE_COOLDOWN_MS 1000 // réarmement

// ---------------------------------------------------------------------------
// Télémétrie
// ---------------------------------------------------------------------------
#define DCCB_TELEMETRY_PERIOD_MS 1000 // envoi toutes les 1 s

// ---------------------------------------------------------------------------
// Options debug recommandées
// ---------------------------------------------------------------------------
#define DCCB_DEBUG_SERIAL 1 // 0=off, 1=normal, 2=verbeux
#define DCCB_MEASURE_STATS 1
#define DCCB_SCOPE_MODE 0 // 1 = oscilloscope DCC

// ---------------------------------------------------------------------------
// Taille de la queue d'événements DCC (1 événement = 1 bit complet ou 1 cutout)
// Le DCC produit au pire ~17 000 bits/s, soit ~30 événements/ms : 64 entrées
// couvrent largement une latence de 1 ms de taskCan.
// ---------------------------------------------------------------------------
#define DCC_EVENT_QUEUE_SIZE 64