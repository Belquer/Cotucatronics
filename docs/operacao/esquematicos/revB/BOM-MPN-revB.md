> [!warning] RETIRED on 08/09/2026 — do not order from here
>
> The live purchase list is **`COTUCA_BOM.md`**, generated from the KiCad sheets by
> `tools/bom_completa.py`; the rev C purchase is `export/BOM_CONSOLIDADO.csv`
> (see `export/MANIFEST.md`). This one was written by hand and already diverged from the schematic.
>
> **Everything useful in it was migrated**, and these MPNs were CORRECTED
> — if you read them here, they are wrong:
>
> | it says here | the live one (17/09), and why |
> |---|---|
> | `2920L300/24MR` | **`MF-LSMF300/24X-2`** (Bourns) — the /300 did not exist (the 24 V one was the `2920L330/24MR`), and that one was in turn swapped in the rev C purchase for this one, verified in stock at Mouser on 12/09 |
> | `PEC11R-4220F-S0024` | **`EC11E15244G1`** (Alps) — the PEC11R has pins at 5,0 mm and the footprint is the Alps one, at 2,5 |
> | `PTV09A-4020**F**-B103` | **`PTV09A-1020S-B103`** — `S` is a knurled shaft, `F` is smooth, and the note next to this very table called for knurled. The swap from `4020` to `1020` is in the generator with no written reason |
> | `1935161` | **`1715721`** — the MKDS **1** is 1,5 mm narrower than the MKDS **1,5**, which is the one in the footprint |
> | `53047-0710` (J_OLED, PicoBlade) | **`61304011121`** cut down to 1x07 — the OLED comes with a cable with female jumpers, and the board takes a 2,54 male header (17/09) |
>
> And two things that only surfaced during migration: **the sockets for the Daisy and the ESP32
> were not budgeted anywhere**, and the **cable to the DPDTs was not in the
> BOM at all** — it became 8-way on 08/09, with pins 7 and 8 on GND.
>
> Kept for the record: the selection reasons written here still hold, and
> it was from them that the notes now living in the generator came.

# BOM by MPN — Cotucatronics rev B
<!-- doc-check: ignore power_pin_not_driven (codigo de violacao do ERC do KiCad) -->
### 05/09/2026 · two KiCad projects · JLCPCB assembles the SMD · dB buys and solders the rest

> ⚠️ **Names updated on 05/09 to match the schematic.** KiCad treats a
> reference without a numeric part (`R0A`, `C1A`, `RVA`, `DA`) as unannotated and
> renumbers it on its own when flashing. The tens digit now states the channel:
> `R10`–`R14` = channel A, `R20`–`R24` = channel B, `C11`–`C14` / `C21`–`C24`,
> `RV1`/`RV2`, `D11`/`D21`. Full map in `ERC-ISENCOES.md`.
> **The 10 k at the amp inputs are `R50`/`R51`** — the tens digit 2 was taken
> by the preamp channel B, and the reference is unique across the whole hierarchy.
> **Rule:** the footprint comes from the datasheet of the chosen MPN, not from measurement.
> Only what has no datasheet is measured (§5). LCSC codes: the Code is pulled at
> order time with confirmed stock — JLC's BOM tool matches by MPN.
> Selection criterion: **a part from a known manufacturer, in stock at LCSC, in
> the Basic tier when one exists** (Basic = no setup fee; Extended = ~US$3/part
> once per order).

---

## 1 · BRAIN BOARD — SMD, JLCPCB assembles (top face)

### Semiconductors
| ref | qty | part | MPN | encaps. | tier | note |
|---|---|---|---|---|---|---|
| U1 | 1 | dual JFET op-amp | **OPA1642AIDR** (TI) | SOIC-8 | Extended | **confirm stock before the order — it holds everything up.** Alternative if out of stock: OPA1652AIDR (CMOS, 4,5 nV) |
| D11, D21 | 2 | dual series diode | **BAV99,215** (Nexperia) | SOT-23 | Basic | input clamp; pin 3 = signal |
| D2 | 1 | Schottky 1 A 40 V | **SS14** (MDD or Vishay) | SMA | Basic | the bench 1N5819, in SMD; range for the bridge |
| D1 | 1 | TVS 24 V standoff | **SMBJ24A** (Littelfuse) | SMB | Basic/Ext | cathode on +15 V |
| F1 | 1 | PTC 3 A hold, ≥ 24 V | **2920L300/24MR** (Littelfuse) | 2920 | Extended | the 1812 3 A ones only reach 16 V — 15 V PD is zero margin |

### Resistors 0805, 1 %, thick film (Uniroyal 0805W8F… or Yageo RC0805FR — all Basic)
| ref | qty | value | note |
|---|---|---|---|
| R11, R21 | 2 | 4,7 MΩ | input bias — **1 %**; 5 % shifts the node's VBIAS |
| R12, R22 | 2 | 1 kΩ | input protection |
| R13, R23 | 2 | 4,7 kΩ | **DNP** — gain leg, only if the trimpot comes back |
| R10, R20 | 2 | 0 Ω | follower: OUT → −IN |
| R14, R24 | 2 | 100 Ω | output isolation |
| R5, R6 | 2 | 100 kΩ | VBIAS divider |
| R8 | 1 | 100 kΩ | rail → A2 divider |
| R9 | 1 | 22 kΩ | likewise |
| R50, R51 | 2 | 10 kΩ | amp inputs to GND (outside the switch) — **they were `R10`/`R11`; the tens digit 1 became the preamp channel A** |
| R7 | 1 | 100 Ω **¼ W, 1206** | V+PRE filter; 0805 is ⅛ W |

### SMD capacitors
| ref | qty | value | MPN / spec | encaps. | tier | note |
|---|---|---|---|---|---|---|
| C11, C21 | 2 | 22 pF **C0G** 50 V | any Basic C0G | 0805 | Basic | **DNP** with the follower |
| C12, C22 | 2 | 22 nF **C0G** 50 V | **C1206C223J5GACTU** (Kemet) | **1206** | Extended | −6 dB divider; **PPS alternative: ECHU1C223GX5** (Panasonic). Never X7R here |
| C7, C9, C9, C4, C2 | 5 | 100 nF X7R 50 V | Basic | 0805 | Basic | decoupling (power + preamp) |
| C8 | 1 | 10 µF X7R **50 V** | Basic | **1206** | Basic | glued to pin 8 of U1 (14,6 V — 0805/25 V loses half to bias) |
| C4 (power) | 1 | 100 nF X7R | Basic | 0805 | Basic | at the amp terminal, next to the 2200 µ |

### No part (do not go in the BOM or the CPL)
NetTie-2 (AGND↔GND, copper) · PWR_FLAG ×4.

---

## 2 · BRAIN BOARD — THT, dB buys and solders

### Radial electrolytics (Panasonic FR = low-ESR, long life; footprint from the datasheet)
| ref | qty | value | MPN | ⌀ × ht. · pitch | note |
|---|---|---|---|---|---|
| C1 (power) | 1 | 2200 µF 35 V | **EEU-FR1V222** (Panasonic FR) | 16 × 25 · **P7,5** | at the amp terminal — measured: without it the exciter hums |
| C3 (power) | 1 | 100 µF 16 V | **EEU-FR1C101** | 6,3 × 11 · P2,5 | on the bridge |
| C6, C6 (preamp) | 2 | 470 µF 25 V | **EEU-FR1E471** | 10 × 12,5 · P5 | V+PRE reservoir |
| C5 (preamp) | 1 | 47 µF 25 V | **EEU-FR1E470** | 6,3 × 11 · P2,5 | VBIAS |
| C13, C23 (preamp) | 2 | 22 µF **bipolar** 50 V | **UES1H220MEM** (Nichicon UES, audio) | 6,3 × 11 · P2,5 | **DNP** with the follower; footprint stays |

### Film
| ref | qty | value | MPN | body · pitch | note |
|---|---|---|---|---|---|
| C14, C24 | 2 | 2,2 µF 63 V | **MKS2C042201K00KSSD** (WIMA MKS2) | 7,2 × 11 × 7,2 · **P5** | output coupling — film, never X7R |

### Trimpot (DNP, footprint stays)
| ref | qty | part | MPN |
|---|---|---|---|
| RV1, RV2 | 2 | 100 kΩ multiturn | **3296W-1-104LF** (Bourns) |

### Connectors and socket
| ref | qty | part | MPN / spec | note |
|---|---|---|---|---|
| Daisy socket | 2 | female 1×20, 2,54, gold contacts | **M20-7822046** (Harwin) or equiv. | substitutability — worth the US$2 |
| J2 (amp 15 V), J_L, J_R (line to amp) | 3 | 2-way screw terminal, 5,08 | **1935161** (Phoenix MKDS 1/2-5.08) or generic KF301-5.0 | Phoenix if it goes into production; KF301 on the bench |
| J step-down | 1 | female 1×3, 2,54 | generic | pitch confirmed when the module arrives |
| J decoy | 1 | 2-way screw terminal 5,08 | same as J2 | |
| J bridge | 1 | female 1×5, 2,54 | generic | TX RX STATUS 5V GND |
| J panel | 1 | IDC 2×13 male, shrouded, 2,54 | **N2526-6002RB** (3M) or generic | the contract between boards |
| J piezo / line jacks | 4 | header 1×3, 2,54 | generic | wire to the panel jacks |
| J DPDT | 2 | header 1×6, 2,54 | generic | wire to the switches |

---

## 3 · PANEL BOARD — SMD, JLCPCB assembles
| ref | qty | part | MPN | encaps. | tier | note |
|---|---|---|---|---|---|---|
| Q1, Q2 | 2 | N logic-level MOSFET | **AO3400A** (Alpha & Omega) | SOT-23 | Basic | 5 A; margin against the 100 mA per LED |
| R_g1, R_g2 | 2 | 100 Ω | 0805 Basic | | | gate |
| R_pd1, R_pd2 | 2 | 10 kΩ | 0805 Basic | | | **mandatory pull-down** — the GPIO floats at boot |
| R_led1, R_led2 | 2 | 47 Ω **½ W** | 1210 or 2512 | Basic | | LED series, target ~30 mA; **value confirmed on the bench** with the blue one |
| C_pot1, C_pot2 | 2 | 100 nF X7R | 0805 Basic | | | from wiper to GND, glued to the pot |

## 4 · PANEL BOARD — THT, dB buys and solders
| ref | qty | part | MPN | note |
|---|---|---|---|---|
| SW1 | 1 | EC11 with push, 20 mm shaft | **PEC11R-4220F-S0024** (Bourns) | 24 detents, knurled shaft; if you keep the one bought, footprint by measurement |
| RV1, RV2 | 2 | pot 10 kΩ **linear**, 9 mm, 20 mm shaft | **PTV09A-4020F-B103** (Bourns) | knurled shaft ⌀6 — check the knob hole ⌀13 |
| J_OLED | 1 | header 1,25 mm 7-way vertical | **53047-0710** (Molex PicoBlade) | it is the module's Waveshare "MX1.25" |
| J panel | 1 | IDC 2×13 female | **N3793-6002RB** (3M) or generic | |
| J_frzA, J_frzB (switch) | 2 | JST XH 2-way | **B2B-XH-A** | wire to the arcade microswitch |
| J_ledA, J_ledB | 2 | JST XH 2-way | **B2B-XH-A** | wire to the arcade LED |
| arcade buttons | 2 | 30 mm illuminated **blue** 5 V | EG Starts (already bought) | **color is spec** — at 3,3 V the yellow one will not light |
| OLED | 1 | Waveshare 2,42" SSD1309 SPI | already bought | |

---

## 5 · OFF-BOARD — bought, connect by wire

| part | qty | MPN / spec | note |
|---|---|---|---|
| decoy PD 15 V / 5 A | 1 | AITRIP (already bought) | USB-C receptacle on the rear face |
| step-down 5 V fixed 1,5 A | 1 | Euogeudel (already bought, 5 pcs.) | **fixed output**, by decision |
| class-D stereo amp | 1 | Clyxgs TPA3116D2 (already bought) | screw terminals; off-board |
| panel ¼" TRS jacks | 4 | **NMJ6HCD2** (Neutrik) | isolated from the chassis; quality/price |
| DPDT ON-ON-ON type 2 | 2 | already bought | **measure the type with a multimeter before wiring** |
| BLE bridge | 1 | ESP32-C3 SuperMini | 5 wires |
| IDC 2×13 cable | 1 | 26-way, ~150 mm, crimped | test each crimp **by pulling** |
| short USB-C cable | 1 | 15 cm, C-C, with data (PD) | from the powerbank to the decoy |

## 6 · What is still MEASURED (no usable datasheet)
Jacks (thread, nut, depth, T/R/S) · DPDT (thread, depth, **type**) ·
step-down and decoy (pin pitch, holes) · amp terminal · arcade (⌀ of the
hole, bezel) · OLED (c-c holes). It is the sheet `MEDICAO-21-PECAS.md`, sections 3, 4,
8 and 9 — the other sections are made obsolete by this BOM.

## 7 · Before submitting to JLC
1. Code: LCSC of each SMD line, **with stock**, and the real tier. Where Basic
   does not exist, state how many Extended there are (each one costs setup).
2. **OPA1642AIDR in stock?** If not: OPA1652AIDR is the substitute (same
   footprint, CMOS, 4,5 nV — slightly worse in current noise, still
   well above what is needed).
3. **C0G 22 nF 1206** in stock? If not: PPS ECHU1C223GX5. If neither: 2× 10 nF
   C0G 0805 in parallel (double footprint).
4. BOM and CPL without NetTie, without PWR_FLAG, with the DNPs marked as DNP.
5. Assembly only on the top face; no THT assembled by JLC.

---

## 8 · Findings of 05/09 while assigning the footprints

Verified file by file and against the manufacturers' datasheets. **Three
choices in this BOM do not survive the confrontation with the mechanical drawing.**

### 8.1 The pot `PTV09A-4020F-B103` does not fit

The style **`-4` is *Horizontal/Rear Mount, bushingless*** — no thread and with the
shaft lying down. The decision of 05/09 is the pot **on the board, shaft rising through the
ø7 hole in the lid**, which calls for a **vertical style with a threaded bushing**.

The vertical style is the **`PTV09A-1`**, and it is the only one KiCad carries a footprint for
(`Potentiometer_Bourns_PTV09A-1_Single_Vertical`: 3 pins at 2,5 mm, two
posts ø2,2 at x=7). **To be reviewed before buying.**

### 8.2 The encoder `PEC11R` has no footprint, and is not the Alps one

The Bourns datasheet puts the encoder's three pins at **5,0 mm**; the Alps `EC11E`,
which is the one KiCad carries, puts them at **2,5 mm**. They are not interchangeable
no matter how much they both call themselves "12 mm EC11".

Two ways out: draw a footprint of our own for the PEC11R (dimensions in the datasheet:
pins ø1,0 at 5,0 mm, tabs 2,6 × 1,8, gaps 7,0 and 7,5), **or** choose an
Alps `EC11E` and use the footprint that already exists. The second is cheaper in
work and in risk.

### 8.3 Panel `R5`/`R6`: value and package in disagreement

The BOM says **47 Ω ½ W**; the schematic says **`100R serie (~30 mA)`**. And the
package was at 0805, which is **⅛ W** — wrong in either case.
It ended up **1210**; if the chosen part is ¼ W, it goes up to 2512. **The value
is still to be decided.**

### 8.4 Other corrections applied

| ref | was | became | why |
|---|---|---|---|
| `F1` | 1812 | **2920** | the 1812 3 A ones only go to 16 V — the BOM's own reason |
| `C8` | 0805 | **1206** | 10 µF X7R 50 V; 0805/25 V loses half to bias |
| `C12`/`C22` | — | **1206** | C0G 22 nF (Kemet `C1206C223J5GACTU`) |
| `J3`–`J7` (panel) | header 2,54 | **JST XH** `B2B-XH-A` | connector with a latch |
| `J2` (panel) | — | **PicoBlade `53047-0710`** | it is the Waveshare "MX1.25" |
| `J1`,`J2` (power) | header 2,54 | **Phoenix MKDS-1,5-2-5.08** | screw terminal |
| radials and film | — | from the ⌀×pitch of this BOM | `CP_Radial_D16.0mm_P7.50mm`, `D6.3mm_P2.50mm`, `D10.0mm_P5.00mm`, `C_Rect_L7.2mm_W11.0mm_P5.00mm_…MKS2` |

⚠️ The WIMA footprint was chosen by name (`…MKS2`, pitch 5) and by the `11`
in this BOM's line. **Check it with calipers** when the part arrives: the three dimensions
`7,2 × 11 × 7,2` do not say which is the thickness.

### 8.5 Count discrepancies, unresolved

- The BOM listed `C6A`/`C6B` (2× 470 µF) and `C7A`/`C7B` (2× 100 n); **the schematic
  has one of each** — shared supply, not duplicated per channel.
- The BOM and `ERC-ISENCOES.md` say **four `PWR_FLAG`**; the schematic has
  **two**, and exactly **2 `power_pin_not_driven`** are left over in the brain board.

### 8.6 Six parts still drawn as parts that ought to be connectors

This BOM already decides that they connect by wire (`header 1×3` for the jacks, `1×6` for
the DPDTs), but the schematic draws them as parts: `J5`/`J6` (piezo), `J12`/`J13`
(line TRS), `SW1`/`SW2` (DPDT). It is the same correction already made to the panel's
arcade buttons.

### 8.7 Stock, measured on 05/09

`OPA1642AIDR` at LCSC: **`C67640`, 980 units, US$ 1,0145** at 5–10 pcs.
⚠️ LCSC stock **is not** JLC assembly stock — they are different warehouses,
and the Basic/Extended tier is confirmed in JLC's parts library, not here.

---

## 9 · Decisions of 05/09 and cost per part

### 9.1 What changed part

| ref | was | becomes | why |
|---|---|---|---|
| `RV1`,`RV2` (panel) | `PTV09A-4020F-B103` | **`PTV09A-4020S-B103`** | only the `F` changes: `F` is a **smooth** shaft, `S` is **knurled, 18 teeth**, which is what the note said. The `-4` style stays — it is *bushingless*, but with the pot **on the board** the lid hole is a passage and what holds it is the PCB. ⚠️ The `-1` is not in stock in 10 kΩ; only the `-4` family. |
| `SW1` (panel) | Bourns `PEC11R-4220F-S0024` | **Alps `EC11E15244G1`** | the Bourns datasheet puts the pins at **5,0 mm**; KiCad only carries the Alps footprint, at **2,5 mm**. A footprint of our own is work and risk to save nothing. |
| `F1` | `2920L300/24MR` | **`2920L330/24MR`** | ⚠️ **the 300/24 does not show up in stock anywhere.** What exists at 24 V is the **3,3 A**; the `2920L300/15DR` is 3 A but only at **15 V**, which is the zero margin this BOM already rejected. |
| `C14`,`C24` | 7,2 × 11 × 7,2 | real body **7,2 × 7,2 × 13**, pitch 5,0 | measured from the `MKS2C042201K00KSSD` datasheet. ⚠️ KiCad **does not have** `L7,2 × W7,2`; it ended up as `W11.0`, which is **larger** than the part — clearance to spare, never short. Check with calipers. |
| `R5`,`R6` (panel) | 47 Ω ½ W | **`100R`, 1210, value to be measured** | 30 mA in 100 Ω is 90 mW; 0805 is ⅛ W and will not take it. The value is settled on the bench, with the blue one at 5 V, measuring the current. |

### 9.2 Cost per part — **measured on 05/09**

| part | source | unit price | stock |
|---|---|---|---|
| `OPA1642AIDR` | LCSC `C67640` | **US$ 1,0145** (5–10 pcs.) | 980 |
| Alps `EC11E15244G1` | LCSC `C370970` | **US$ 1,70** (5 pcs.) | 1 125 |
| Neutrik `NMJ6HCD2` ×4 | Stompboxparts | **US$ 1,75** (25+: 1,70) | 196 |
| Panasonic `EEU-FR1V222` | LCSC `C407956` | **US$ 0,7162** | in stock |
| Bourns `3296W-1-104LF` ×2 | LCSC `C48084` | **US$ 0,4458** | 11 068 |
| Littelfuse `2920L330/24MR` | LCSC `C720183` | **US$ 0,1804** | 4 075 |
| `AO3400A` ×2 | LCSC | **US$ 0,0119–0,0487** | several |
| `SMBJ24A` | LCSC | **US$ 0,0143–0,0793** | several |
| C0G 22 nF 1206 | LCSC (comparable) | **~US$ 0,0165** | — |

### 9.3 Cost per part — **estimated, not measured**

0805/1206 Basic passives ~US$ 0,002–0,01 · `BAV99` and `SS14` ~US$ 0,02 ·
2,54 headers ~US$ 0,05–0,15 · `JST XH` ~US$ 0,05 · PicoBlade ~US$ 0,30 ·
IDC 2×13 ~US$ 0,30–0,60 · Phoenix terminal ~US$ 0,80–1,50 (the generic `KF301`
~US$ 0,15) · Daisy socket ~US$ 2 (the BOM's own note) ·
small FR electrolytics ~US$ 0,10–0,30 · WIMA `MKS2` ~US$ 0,50–0,90 ·
Nichicon `UES` ~US$ 0,30 · pot `PTV09A` ~US$ 0,60.

### 9.4 Order of magnitude per assembly

| block | ~US$ |
|---|---|
| brain board, SMD side | 1,50 |
| brain board, THT | 9,60 |
| panel, SMD | 0,20 |
| panel, THT | 4,00 |
| 4 Neutrik jacks | 7,00 |
| **components, per assembly** | **≈ 22** |

⚠️ **Does not include** PCB, JLC assembly, the Extended *setup* fee, nor what
is already bought (arcade, OLED, TPA3116, step-down, decoy, ESP32-C3).

### 9.5 The check that remains, and it is the one that holds everything up

**LCSC stock is not JLC assembly stock.** They are different warehouses. The
980 units of `OPA1642AIDR` at LCSC say nothing about JLC's parts
library, and the Basic/Extended tier is confirmed **there**, not here.

If the `OPA1642` is not in JLC's library, the answer **is not to wait**:
it is the **`OPA1652AIDR`** (same SOIC-8 footprint, CMOS, 4,5 nV), or asking JLC
to buy it at LCSC as a *pre-order* part — they do that, with a lead time.

### 9.6 What still has no footprint

`RV1`/`RV2` — 2 of 79. KiCad carries a footprint for the `PTV09A-1` and the `-2`; the
buyable part is the **`-4`**, and I **did not confirm** that the three share the same
PCB drawing. A hardware fact with no cited source is a hypothesis: either the
`-4` drawing is read in the datasheet, or the part is measured by hand.

---

## 10 · 05/09 — the board became the **Daisy Seed3**

The rev4 had production suspended; the Seed3 came back in stock and is the one that goes.

**The pinout was checked, not assumed.** I compared the two manufacturer CSVs pin
by pin: **the STM32 name matches across all 40 pins, without a single exception.** The Seed3
documentation says verbatim *"maintaining pin-to-pin compatibility with the
original Seed pinout and footprint"*, and the pinout drawing that dB sent
confirms pin by pin on the ones this project uses.

Result in the schematic: **identical symbol, footprint, and netlist.** The swap did not
touch a single connection — proven by netlist comparison, 63 nets equal.

**⚠️ Two traps found along the way, and they hold for next time:**

1. **The two CSVs have columns with different meanings.** Where the rev4 one puts the
   alternate function (`SDMMC1_D3`), the Seed3 one puts `GPIO`. Comparing column by
   column gives **35 "differences" that do not exist**. You compare the **STM32 name**.
   `gera_daisy_sym.py` now reads by the header NAME, and that is why it
   blew up loud when the source changed, instead of emitting a wrong symbol.
2. **The manufacturer's CSV contradicts the manufacturer's drawing.** On pin 40 the
   name column says `PG3`; the function one says `GND`; the drawing says `DGND`. The
   generator now refuses names that are an MCU port designation
   (`P[A-K]n`) and **warns** which one it discarded — choosing silently between two
   sources that disagree is the same as inventing.

**What changes in our favor:** codec **TI TAC5242**, 32 bit / 192 kHz, noise floor
of −120 dB, versus the rev4's AK4556. This same session spent hours recovering
35–39 dB of noise floor; the new part comes in above all that.

**What changes against: nothing practical.** I had written that "the flashing
cable changes" — **that was wrong**: dB flashes over USB-C ever since the first
board burned out, and the bench board is already USB-C. The enclosure never had a cutout
for the Daisy's USB and still does not: you flash with the enclosure open.

**Versioned source:** `kicad/cotuca_cerebro/Seed3_pinout.csv`, taken from
`daisy.nyc3.cdn.digitaloceanspaces.com/products/seed3/`. One typo fixed
in a path: pin 17 was called `AUDIO_INR` (a missing space in the rev4 CSV)
and became `AUDIO_IN_R`.

---

## 11 · 05/09 — the `OPA1642` is confirmed at JLC, and the `OPA1656` was evaluated and rejected

### 11.1 The blocker fell

**`OPA1642AIDR` = LCSC `C67640` = JLCPCB `C67640`, EXTENDED tier**, available
for SMT assembly (*Economic* and *Standard*). It was marked in this BOM as
*"confirm stock — holds everything up"*. **It is confirmed.** It costs the one-time Extended
*setup*, which was what was foreseen.

LCSC: US$ 1,0145 at 5–10 pcs., 980 in stock (measured 05/09).

### 11.2 The `OPA1656` was considered, and stays out

It came up because **KiCad has a symbol for it and none for the `OPA1642`** — the
`OPA1656ID` inherits from the `LM2904`, and is dual SOIC-8 with the same footprint.

| | OPA1642 | OPA1656 |
|---|---|---|
| voltage noise | 5,1 nV/√Hz | **2,9 nV/√Hz** (10 kHz) |
| current noise | — | 6 fA/√Hz (1 kHz) |
| GBW | ~11 MHz | **53 MHz** |
| output | — | rail-to-rail |
| LCSC | US$ 1,01 · 980 | **US$ 0,86** (`IDR`, reel) · **5 245** |

On paper it is a better amplifier and cheaper. **It stays out for three reasons:**

1. **The preamp is built and MEASURED with the `OPA1642`.** The 05/09 recovery of 35–39
   dB of noise floor was done in that circuit. Swapping puts on the fabricated board
   an amplifier the bench never heard — and the house rule is
   *if it does not sound, it does not stay*.
2. **53 MHz on a 4M7 node is a new variable.** The piezo input is very high
   impedance. Five times faster at that node is more RF pickup and more
   risk of a peak with the input capacitance — and it would probably call for the
   **22 pF C0G of `C11`/`C21` populated**, which today are **DNP**. It stops being a
   part swap and becomes an unmeasured circuit change.
3. **In a piezo the dominant noise is the CURRENT noise**, not the voltage, and both are
   FET-input. The gain of the 2,9 against 5,1 nV is smaller here than at a
   low-impedance source.

**If one day you want to try it:** the KiCad symbol exists, the footprint is
the same SOIC-8, and it is one line of this BOM plus one listening session. It is not a
redesign. It is written down so this is not decided from scratch again.

### 11.3 A free check that came out of this

Since KiCad's `OPA1656ID` inherits from the **`LM2904`** — the same base symbol from which
the pinout for our hand-made `OPA1642` was taken — the two were
compared pin by pin: **unit and type match across all 8**. The home-made symbol is
validated against one maintained by the official library, which was the confirmation
missing after pins 1 and 3 had been caught swapped in the old symbol.
