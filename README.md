# 🚂 DCC_CAN-Booster — Chemins de Fer du Chablais (CFC)

Module **Convertisseur DCC → CAN** de l'architecture **Chemins de Fer du Chablais (CFC)**.
L'ESP32 décode le signal DCC (via un XOR SN74LVC1G86 sur GPIO 27) et le
retransmet sur le bus CAN (TWAI natif, TX GPIO 5 / RX GPIO 4, 500 kbit/s).

Basé sur la bibliothèque [prune74/CanUniversal](https://github.com/prune74/CanUniversal).

## Architecture

| Élément | Rôle |
| --- | --- |
| `DccDecoder` (ISR) | Décode les **bits complets** (paires de demi-bits), phases et cutouts depuis le signal DCC |
| `taskCan` (core 1) | Empaquette les bits (16 max/trame) et les envoie sur le bus CAN |
| `taskCanRx` (core 1) | Sniffer CAN (commande `can on` via le moniteur série) |
| Failsafe | Perte DCC > 500 ms → `BOOSTER_OFF` (télémétrie) + bloc vidé |
| Télémétrie | Toutes les 1 s sur l'ID 0x102 |

```
LaBox (CommandStation-EX)                ESP32                       Bus CAN
┌─────────────────────┐   EXT1/EXT2   ┌──────────────────────────┐
│  Sortie booster    │  0–3.3 V  ──►  │ XOR SN74LVC1G86 → GPIO27 │
│  signal DCC + cutout│                │ ISR : demi-bits → bits   │
└─────────────────────┘                │ taskCan : blocs 0x100    │ ──► 500 kbit/s
                                       │ TWAI : TX=5, RX=4        │      vers boosters aval
                                       └──────────────────────────┘
```

## Câblage

| Signal | Origine | ESP32 | Remarques |
| --- | --- | --- | --- |
| DCC logique | LaBox EXT1/EXT2 (0–3.3 V) via XOR SN74LVC1G86 | GPIO 27 | **Masse commune** LaBox ↔ ESP32 obligatoire |
| CAN TX | TWAI natif | GPIO 5 | Vers transceiver CAN (TJA1050/SN65HVD230…) |
| CAN RX | TWAI natif | GPIO 4 | |
| LED debug | — | GPIO 2 | Bascule à chaque lot d'événements |

Conseils pour un signal propre :
- Le signal EXT de la LaBox est déjà au niveau logique 3.3 V : ne jamais y
  brancher le DCC de puissance directement.
- Un petit filtre RC (ex. 1 kΩ + 10 nF) et/ou un trigger de Schmitt
  (74HCT14) entre l'XOR et le GPIO27 éliminent les rebonds de ringing des
  boosters — les glitches < 25 µs sont de toute façon ignorés par le
  décodeur (`DCCB_TIMING_GLITCH_US`).

## Principe du décodage (NMRA S-9.1)

L'ISR mesure la durée `dt` entre deux fronts (mode CHANGE) — c'est-à-dire
**un demi-bit** DCC :

| `dt` mesuré | Interprétation |
| --- | --- |
| `< 25 µs` | Glitch : ignoré, compté `bad` |
| `≤ 85 µs` | Demi-bit « 1 » (nominal 58 µs) |
| `85–300 µs` | Demi-bit « 0 » (nominal 100 µs, étiré accepté) |
| `≥ 300 µs`, demi-bit précédent = « 1 » | **Cutout** : START+END émis, re-synchronisation |
| `≥ 300 µs`, demi-bit précédent = « 0 » | Zéro-stretching (traction DC) : demi-bit « 0 » |

Un bit DCC = **deux demi-bits identiques**. Le décodeur n'émet qu'**un seul
évément par bit complet** (à la fermeture de la paire) : le bitstream CAN
restaure exactement le débit binaire d'origine. La classification par point
médian (85 µs) évite tout « trou » de non-classification entre les fenêtres.

**Cutout** : le cutout NMRA prolonge le second demi-bit du dernier « 1 » de la
trame. Le décodeur clôture la paire en cours, émet `CUTOUT_START` puis
`CUTOUT_END` consécutifs, et re-synchronise sur le front de reprise (aucun
demi-bit perdu). La durée exacte n'est pas transportée : le booster aval
**régénère un cutout nominal (454 µs)**.

**RailCom** : ce module ne distribue pas le RailCom — il relaie uniquement la
présence du cutout (ID 0x101) pour que le booster aval le régénère ou
l'ignore selon sa configuration. L'ID 0x103 est réservé à un usage futur.

## Protocole CAN (IDs 11 bits)

| ID | Contenu |
| --- | --- |
| `0x100` | Bloc de bits DCC : `data[0]` = N bits (1..**16**), puis 4 bits/octet, chaque nibble = `(bit << 1) \| phase` — DLC = `1 + ceil(N/2)` |
| `0x101` | Cutout **réel** : `{ local, global }` — jamais émis en failsafe |
| `0x102` | Télémétrie : `{ mA/10, mV/100, état }` (état : 0=OK, 1=FAULT, 2=OVERHEAT, 3=OFF) |
| `0x103` | RailCom : adresse little-endian (réservé, non distribué) |

Le bloc part : à 16 bits (trame pleine), sur cutout, ou après 500 µs de
silence DCC (`DCCB_BLOCK_FLUSH_TIMEOUT_US`) pour ne pas ajouter de latence en
fin de trame DCC.

## Failsafe

- Perte du signal DCC > 500 ms → le bloc en cours est vidé, puis télémétrie
  `BOOSTER_OFF` sur 0x102 (l'ID 0x101 reste réservé au cutout réel).
- Retour du DCC → télémétrie `BOOSTER_OK`.
- Statistiques en erreur (`bad > 0` en régime établi) → état `BOOSTER_FAULT`.

## Compilation (VSCode + PlatformIO)

```
pio run
pio run -t upload
pio device monitor
```

Configuration : `src/Pins.h` (broches), `src/Config.h` (seuils, IDs, failsafe,
bitrate). Moniteur série à **115200 bauds**.

## Commandes série (115200 bauds)

```
stats        statistiques DCC + événements perdus + état CAN
reset        redémarrage ESP32
can on|off   sniffer CAN
can filter 0x100   filtre le sniffer sur un ID (can filter off pour retirer)
debug on|off, scope on|off
```

**Diagnostic sain** : `stats` doit montrer `dropped=0` et `bad` stable
(seuls les premiers instants après le démarrage peuvent en compter, le temps
de la première synchronisation sur un préambule).

## Historique des corrections (cette version)

1. **Un événement par bit complet** — avant, un événement par demi-bit :
   le bitstream CAN partait au double du débit DCC réel.
2. **Cutout** — le front de reprise est désormais consommé comme front
   d'ouverture du bit suivant (avant : demi-bit perdu + `badTiming` à
   chaque cutout) ; les trames `0x101` START/END sont émises consécutives.
3. **Header du bloc 0x100** — `data[0]` = nombre réel de bits (avant :
   valeur ×4, illisible côté récepteur) ; capacité réelle plafonnée à
   16 bits (débordement tampon évité) ; DLC cohérent.
4. **Classification par point médian** — plus de trou 80–90 µs ni de rejet
   des « 0 » longs ; les glitches < 25 µs sont ignorés.
5. **Garde anti faux-cutout** — un zéro-stretching (traction DC) n'est plus
   interprété comme un début de cutout.
6. **Failsafe propre** — plus de trame cutout parasite : `BOOSTER_OFF` via
   la télémétrie uniquement.

## Auteur

Bruno Rivera (prune74) — Projet Chemins de Fer du Chablais (CFC)