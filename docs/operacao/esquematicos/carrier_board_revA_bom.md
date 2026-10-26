# Cotucatronics Carrier rev A — candidate BOM
### Verified against LCSC on 20/08 (stock fluctuates: re-check on the day of the order)

## ✅ Critical — verified today, with a real number

| block | part | LCSC | package | stock seen | note |
|---|---|---|---|---|---|
| PD sink | **CH224K** (WCH) | **C970725** | ESSOP-10 | ~5–6k | Rset **24 kΩ** on CFG1 = 12V. VDD max 3,6V — power via the WCH reference schematic (series resistor), NEVER directly from VBUS. LED on PG = "12V confirmed" |
| Amp ×2 | **TPA3116D2DADR** (TI) | **C50144** | HTSSOP-32 (PowerPAD) | in stock | PBTL mono; 12V/4Ω ≈ 15–18W per channel. Thermal pad → thermal vias in the layout |
| Meter | **INA219** (TI) | ⚠️ **LCSC to be verified** | SOT-23-8 or SOIC-8 | — | I2C, bus 0–26 V, VCC 3–5 V. Kelvin on the shunt. **Same part as the bench** — see GEAR-AUDIT |

## Commodity — values defined, final number at order time
(almost everything is a JLC *basic part* = no setup fee; check the flag in the order)

| block | part | value/spec | qty | note |
|---|---|---|---|---|
| Piezo protection | 1N4148WS | SOD-323 | 4 | classic candidate C2128 (basic) |
| Piezo protection | R series | 1 kΩ 0603 | 2 | |
| Piezo protection | R2 divider pads | 0603, **DNP** | 2 | do not populate — a regime decision |
| Shunt | 10 mΩ | 2512, ±1%, ≥1W | 1 | 3A → 30mV, 90mW |
| Buck 12→5V | TPS563201 / MP2315 class | SOT-583/SOIC | 1 | ≥2A; choose a basic part in the schematic |
| Amp LC filter | inductor 10 µH | ≥5A sat, shielded | 4 | 2 per amp (PBTL) |
| Amp LC filter | cap 680 nF | X7R 50V 0805 | 4 | |
| Amp bulk | 2200 µF/25V | radial THT low-ESR **or** 2×1000µF polymer SMD | 2 (or 4) | decide by cost; THT = mixed assembly |
| Decoupling | 100 nF X7R 0603 | — | ~20 | one per VCC pin of each IC |
| TVS on VBUS | SMBJ (≈24V standoff for a 12–20V profile) | SMB | 1 | cheap; the CH224 already died of an inductive spike in a third-party test — do not pay to find out |
| PTC/fuse | ~3–4A hold | 1812/THT | 1 | after the connector, before everything |
| I2C pull-ups | 4,7 kΩ 0603 | — | 2 | |
| LED PG + R | green 0603 + 1 kΩ | — | 1+1 | "12V on air" |

## Connectors

| what | spec | qty | note |
|---|---|---|---|
| Daisy socket | female 2×20 pins 2,54mm THT | 2 strips | |
| Exciter out | lever/Wago-style 2 poles, SMD if available in the catalog (DEGSON/HDGC clone), otherwise THT | 2 | dB's decision: lever |
| OLED | MX1.25 7p SMD receptacle | 1 | matches the Waveshare cable |
| Panel (encoder, buttons, DPDT sense, rotary, fork, WIDI) | JST-PH 2.0 SMD: 2p×6, 3p×2, 5p×1, 6p×1 | ~10 | final count in the schematic |
| Power in | header 2p (chassis USB-C already bought) **and** USB-C 16p footprint on the edge | 1+1 | both on the board; populate one |
| 1/4" jacks | panel + header 2p each (default) — PCB-mount is left for rev B if the enclosure calls for it | 4 | 2 in + 2 out |

## Rules the BOM already embeds
- A part from memory is a part that does not exist: the 3 critical ones were verified TODAY with
  a number; commodity carries a value + candidate and settles the number in the order.
- Prefer JLC *basic parts* where an equivalent exists (setup fee zero per reel).
- DNP declared on the silk (the divider's R2) — an empty pad WITH a name is not an oversight.
- TVS and PTC are not optional: powerbank + cable + removable connector = the most
  abused input in the system.

## Next step
KiCad schematic by block, in this order: power → input → amps → measurement →
headers. Each block closes with a clean ERC before the next. Then: 4-layer
layout, headless DRC, render for dB's veto, Gerber+CPL+BOM → JLCPCB, 5 boards.
