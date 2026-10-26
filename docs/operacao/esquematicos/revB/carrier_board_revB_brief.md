# Cotucatronics — Carrier rev B · project brief

> ⚠️⚠️ **THE BOARD IS 2-LAYER, and this document said 4.** Measured on 08/09 in the files: only `F.Cu` and `B.Cu`, with the `GND` (prio 0) and `AGND` (prio 1) zones both on `B.Cu`, separated, and `NT1` as the single bridge — and that **is verified and correct**. What was wrong was the text.
>
> With no inner plane, the amp return is the `B.Cu` zone: acceptable for 3 A at the width it has, and stated so it is not discovered on the bench. **The JLC order must say 2 layers** — it is cheaper, and ordering 4 for a board designed as 2 pays for a plane no one uses.

### Target: KiCad → JLCPCB (fab + SMT) · **two boards** · updated 01/09
> Replaces `carrier_board_revA_brief.md`, which was left at 20/08 and carries five
> superseded decisions. Rev A **was never fabricated**; this is not a respin, it is
> the update of the design before the first copper cut.

---

## 0 · What changed since rev A, and why

| item | rev A (20/08) | rev B | reason |
|---|---|---|---|
| rail voltage | 12 V (CH224K Rset 24 k) | **15 V** | 15 V is a mandatory PD profile; 12 V is optional and many powerbanks do not provide it |
| PD sink | CH224K on the copper | **PD 15 V/5 A decoy module** | module already bought; it brings the receptacle with the CC lines (a 2-wire pigtail leaves the chip deaf) |
| step-down | TPS563201 on the copper | **5–30 V → 5 V/5 A module** | same ⚠️ see §7, selectable output |
| amp | TPA3116D2DADR on the copper, PBTL ×2 | **stereo TPA3116D2 module, OFF the board** | class-D is the biggest layout risk on a first board; the module has screw terminals and connects by wire |
| current measurement | 10 mΩ shunt + INA219 + I2C | **removed** | the powerbank became a consumable with its own display; the purpose vanished. D11/D12 return to the pool |
| MIDI bridge | 6-wire WIDI header | **ESP32-C3 + Schottky + 100 µF + 100 nF** | US$45 → US$4; the Schottky is not optional (`HARDWARE.md` §1) |
| piezo input | 1 kΩ + 2 diodes, DNP attenuator pads | **OPA1642 preamplifier** | the signal-to-noise ratio was decided by measurement on 01/09 |
| encoders | 1 EC11 | **1 EC11, and on D0/D1/D2** | see §1: frees the A2/A3/A4 aliases |
| buttons | 4 (cowrie shells, frz A, CTOS re-arm, frz B) | **2 (frz A, frz B)** | see §6, it has a firmware consequence |
| LEDs | direct on the pin | **AO3400A + gate, from the 5 V rail** | measured: 30–80 mA per LED, against the pin's 20 mA limit |
| boards | one, panel via headers | **two: brain board + panel** | the lit panel costs 6 dB of noise floor at the input (`PINOS.md`) |

---

## 1 · Pin map of rev B

```
D0  encoder phase A       D13 MIDI TX → bridge RX     ★ crossed
D1  encoder phase B       D14 MIDI RX ← bridge TX     ★ crossed
D2  encoder push          D15 pot L  (A0)
D5  OLED RST              D16 pot R  (A1)
D6  OLED DC               D22 freeze B button
D7  OLED CS               D23 freeze B LED gate
D8  OLED CLK              D24 freeze A button
D10 OLED DIN              D27 freeze A LED gate
                          D28 bridge STATUS (active LOW, open-drain)
AUDIO IN  L ← preamp A    AUDIO OUT L → DPDT L
AUDIO IN  R ← preamp B    AUDIO OUT R → DPDT R
```

**Free: D3, D4, D9, D11, D12, D17, D18, D19, D20, D21, D25, D26, D29, D30.**

Fourteen pins, and **three of them analog** — D17/D18/D19 are the aliases of
A2/A3/A4, and only returned to the pool because the encoder went to D0/D1/D2. `PINOS.md`
declared "there is no possible third pot"; with a single encoder,
**there are three**. This is the main gain from not locking functionality into copper.

**D29/D30 are no longer reserved.** The DPDT sense required a third pole,
and the switch has two — both are spent on `amp | ambos | jack`. With
that, the OLED RST on D5 no longer has the conflict that motivated the choice; the choice
stays anyway, because it is already soldered and proven.

---

## 2 · Power block

```
[PD 15 V decoy module] ── 15 V ── PTC 3 A ── TVS SMBJ ── SW(+V) ── RAIL15
                                                                    │
   ┌────────────────────────────────────────────────────────────────┤
   │                                                                │
 J_AMP (2p screw, edge)                                    [step-down module]
   15 V + GND → TPA3116 module                                     │ 5 V
   ⚠️ 2200 µF/35 V + 100 nF AT THE TERMINAL                     RAIL5
      (measured: with no bank, the exciter hums)                    │
                                          ┌───────────────┬─────────┴────────┐
                                     Daisy VIN      1N5819 → bridge     J_PAINEL
                                                  (+100 µF +100 nF)     5 V LEDs
```

- **TVS and PTC are not optional**: a removable input + powerbank + cable is the
  most spike-prone point in the system.
- **Switch on the positive only.** Continuous ground, in a star.
- **The Schottky makes the error impossible**, and that is why it is a part and not a rule:
  the rule "power by USB **or** by the 5 V pin" was already written and was violated
  by the person who wrote it.

## 3 · Piezo input with preamplifier (×2)

```
JACK TRS ──R2 1k──●──[D1 BAV99 → V+ / AGND]──●──> U1 +in
 tip=signal       │                          │
 sleeve+ring=AGND R1 4M7 → VBIAS          C2 1n → AGND

 U1 −in ──● RV1 100k trim ‖ C1 22p ── U1 out ──R4 100R──C4 2µ2──> AUDIO IN L/R
          └ R3 4k7 + C3 22µ NP → VBIAS

 V+  : RAIL15 ──R7 100R──●── C6 470µ ‖ C7 100n ──> ≈14.6 V
 VBIAS: R5/R6 100 k + C5 47 µ  ──> ≈7.3 V
```

- Gain of 1 to 22 (0 to 27 dB) on the trimpot. **It is a per-unit adjustment** — same
  category as `gov_ceiling`, it does not go into a preset nor is it copied between units.
- **The staging is the point, not the gain**: preamp +20 dB with `pre_level_ab`
  dropping 20 dB leaves the loop gain where it is and raises the signal 20 dB above the
  captured noise. Gain early, attenuation late.
- **X7R ceramic is piezoelectric** — in a vibrating body it becomes a microphone. C0G
  in the small ones, film in the output coupling. Never X7R where signal passes.
- **Piezo cable shielded, shield to AGND only on the board side.** At both
  ends it closes a loop, which is the defect measured this week.
- **Preamp hard against the input jacks, inside the AGND island**, short input
  trace, panel connector on the opposite edge.

## 4 · Line output and DPDT (×2)

```
AUDIO OUT L ──> DPDT L (ON-ON-ON type 2) ──┬─> J_AMP_IN L (screw, to the module)
                                           └─> JACK TRS L
                          permanent 10 kΩ from each amp input to AGND,
                          OUTSIDE the switch
```

- **The switch switches LINE LEVEL, never the amp output.** The
  TPA3116's BTL output has both terminals hot.
- Center position = both; the two commons tied together.
- ⚠️ **Measure the ON-ON-ON before soldering.** The listing does not say type 1 or 2, and the
  design depends on type 2 (commons 2 and 5, center = 2-3/5-4).

## 5 · ESP32-C3 bridge

```
D13 ──────────> GPIO5 (bridge RX)        ★ CROSSED
D14 <────────── GPIO4 (bridge TX)        ★ CROSSED
D28 <────────── GPIO6 (status, open-drain, ACTIVE LOW)
                Daisy internal pull-up — NO external resistor
RAIL5 ──▶|───── 5V     1N5819, stripe (cathode) toward the bridge
        +100 µF +100 nF glued to the pins
GND ─────────── GND, its own wire to the star
```

31250 8N1. **The bridge passes bytes and interprets none** — this is what keeps the
CC map from gaining a second home. Baud printed by `board` and in both
READMEs; no version handshake, by choice (automatic verification and a thin
boundary are incompatible; the thin one is worth more).

## 6 · Panel connector — IDC 2×13 (26 ways)

| way | signal | way | signal |
|---|---|---|---|
| 1 | GND | 14 | GND (LED return) |
| 2 | **OLED CLK** (D8) | 15 | frz A LED gate (D27) |
| 3 | GND | 16 | frz B LED gate (D23) |
| 4 | OLED DIN (D10) | 17 | frz A button (D24) |
| 5 | OLED CS (D7) | 18 | frz B button (D22) |
| 6 | OLED DC (D6) | 19 | GND |
| 7 | OLED RST (D5) | 20 | encoder phase A (D0) |
| 8 | GND | 21 | encoder phase B (D1) |
| 9 | 3V3 | 22 | encoder push (D2) |
| 10 | GND | 23 | GND |
| 11 | 5 V (LED) | 24 | **pot L** (D15/A0) |
| 12 | 5 V (LED) | 25 | **pot R** (D16/A1) |
| 13 | GND (LED return) | 26 | GND |

**The OLED clock is guarded by ground on both sides** (ways 1 and 3), and the
analog pot pair is sandwiched by ground (23 and 26). It is not
fussiness: the lit panel measured **−75.3 → −69.3 dBFS** at the input, with a
spike at 7040 Hz, and the five lines run alongside the piezo cables.
The LEDs' 5 V on its own pair with its own returns — 200 mA if both light up.

**The MOSFETs stay on the PANEL board, next to the buttons.** That way the 200 mA of
switching does not cross the harness; only the two gates cross it, and those are
microamps.

**Firmware consequence, to resolve before the copper:** with two buttons, the
**cowrie shells** lose their manual trigger (they survive by transient and by CC) and the
**CTOS re-arm** loses its recovery path without a computer — in a
gallery it becomes a power cut. D25/D26 stay free in case one wants to
give it back, and the encoder push is a candidate.

## 7 · Open item that is a risk, not a detail

⚠️ **The step-down module has a selectable output: 3.3 / 5 / 9 / 12 / 15 / 24 V.**
That puts **24 V a jumper away from the Daisy's VIN**. In a run of
dozens, a selection that can be wrong will be wrong on some of them. Either the selection is
locked (solder + varnish over the pads, recorded in the assembly procedure),
or it moves to a fixed 5 V module. **A module with a selectable regime
is a module that does not refuse outside the regime.**

Recorded and not measured: it is a switching supply powering the Daisy, and the hunt
of 01/09 left unexplained a 400 Hz series and the 500 Hz–4 kHz grass.
Do not accuse without measuring — but it is on the list.

---

## 8 · BOM — carrier (brain board)

| block | part | package | qty | note |
|---|---|---|---|---|
| preamp | **OPA1642AIDR** | SOIC-8 | 1 | JLC *extended* likely — confirm stock, it blocks the order |
| preamp | BAV99 | SOT-23 | 2 | |
| preamp | 100 kΩ multiturn trim 3296W | THT | 2 | per-unit adjustment |
| preamp | 4M7 · 1k · 4k7 · 100R · 100k×2 | 0805 1% | 8 | |
| preamp | 22 pF C0G · 1 nF C0G | 0805 | 4 | **C0G mandatory** |
| preamp | 22 µF non-polarized | radial | 2 | |
| preamp | 2.2 µF **film** | radial | 2 | do not substitute with X7R |
| preamp | 47 µF · 470 µF 25 V | radial | 2 | |
| preamp | 100 nF X7R · 10 µF | 0805 | 4 | decoupling, glued to U1 |
| preamp | 100 R ¼ W | 1206 | 1 | R7 of the supply filter |
| power | PTC ~3 A hold | 1812 | 1 | |
| power | TVS SMBJ (24 V standoff) | SMB | 1 | |
| power | 2200 µF / 35 V low-ESR | radial THT | 1 | **at the amp terminal** |
| power | 100 nF X7R | 0805 | 1 | same |
| bridge | **1N5819** | THT/SMA | 1 | already soldered on the bench |
| bridge | 100 µF · 100 nF | radial/0805 | 2 | glued to the pins |
| DPDT | 10 kΩ | 0805 | 2 | amp input to AGND, outside the switch |
| connectors | female Daisy socket 2×20 2.54 | THT | 2 strips | replaceability — the dead Daisy proved it is worth it |
| connectors | header decoy PD · step-down · bridge | 2.54 | 3 | |
| connectors | 2p screw terminal (15 V, line L, line R) | THT | 3 | |
| connectors | 1/4" TRS jack | panel + 2p header | 2 in + 2 out | |
| connectors | IDC 2×13 male | THT | 1 | to the panel |
| switches | DPDT ON-ON-ON **type 2** | panel | 2 | **measure before soldering** |

## 9 · BOM — panel

| part | package | qty | note |
|---|---|---|---|
| OLED Waveshare 2.42" SSD1309 | MX1.25 7p | 1 | receptacle on the board |
| EC11 5 pins, 20 mm shaft | THT | 1 | |
| 30 mm illuminated arcade button **BLUE** | panel | 2 | **the color is spec, it goes in the silk** — at 3.3 V the yellow does not light |
| **AO3400A** | SOT-23 | 2 | 5 A; margin against the 100 mA per LED |
| gate resistor 100 Ω · pull-down 10 kΩ | 0805 | 4 | pull-down mandatory: GPIO floats at boot |
| linear 10 kΩ pot | panel | 2 | + 100 nF from the wiper to ground, glued to the pot |
| IDC 2×13 female | THT | 1 | |

---

## 10 · Flow (inherited from rev A, and it holds)

1. BOM against the LCSC/JLC catalog, stock checked → dB's approval
2. KiCad schematic per block, **clean ERC before the next**
3. PCB **2 layers**, ground on `B.Cu` → headless DRC → **render for dB's veto**
4. ⚠️ **Ground review (skill `revisao-terra`) before energizing**
5. Gerbers + CPL + BOM → **5 prototypes**, never a quantity run
6. Rev C only after rev B plays

**A part from memory is a part that does not exist.** The LCSC numbers are pulled on the day of the
order; here go MPNs, which is what survives a catalog change.
