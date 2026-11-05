# 🚂 DCC_CAN-Booster — Chemins de Fer du Chablais (CFC)

Module **Convertisseur DCC → CAN** de l'architecture **Chemins de Fer du Chablais (CFC)**.
L'ESP32 décode le signal DCC (via un XOR SN74LVC1G86 sur GPIO 27) et le
retransmet sur le bus CAN (TWAI natif, TX GPIO 5 / RX GPIO 4, 500 kbit/s).

Basé sur la bibliothèque [prune74/CanUniversal](https://github.com/prune74/CanUniversal).

## Architecture

| Élément | Rôle |
| --- | --- |
| `DccDecoder` (ISR) | Décode bits 0/1, phases et cutouts depuis le signal DCC |
| `taskCan` (core 1) | Empaquette les bits et les envoie sur le bus CAN |
| `taskCanRx` (core 1) | Sniffer CAN (commande `can on` via le moniteur série) |
| Failsafe | Perte DCC > 500 ms → `BOOSTER_OFF` + trame cutout |
| Télémétrie | Toutes les 1 s sur l'ID 0x102 |

## Protocole CAN (IDs 11 bits)

| ID | Contenu |
| --- | --- |
| `0x100` | Bloc de bits DCC : `data[0]` = N bits (1..32), puis 4 bits/octet, chaque nibble = `(bit << 1) \| phase` |
| `0x101` | Cutout : `{ local, global }` |
| `0x102` | Télémétrie : `{ mA/10, mV/100, état }` |
| `0x103` | RailCom : adresse little-endian |

## Compilation

```
pio run
pio run -t upload
```

Configuration : `src/Pins.h` (broches), `src/Config.h`
(timings, IDs, failsafe, bitrate).

## Commandes série (115200 bauds)

```
stats        statistiques DCC + événements perdus + état CAN
reset        redémarrage ESP32
can on|off   sniffer CAN
can filter 0x100   filtre le sniffer sur un ID (can filter off pour retirer)
```

## Auteur

Bruno Rivera (prune74) — Projet Chemins de Fer du Chablais (CFC)# CAN_DCC-Booster
