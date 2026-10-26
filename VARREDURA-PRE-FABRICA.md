# Pre-fabrication sweep — brain board rev B + panel board rev B
### 08/09/2026 · on the `.kicad_sch` and `.kicad_pcb` received (KiCad 10.0) · structural reading, not regex
> **Verdict: DO NOT GENERATE GERBERS.** Six defects that make the board nonfunctional
> or unusable in the enclosure, all in the design — none is a fabrication one. All are
> fixed in the schematic/board before the package, and none is large. But each one,
> alone, was a board that Dendê would take to Europe without it working.

---

## 🔴 CRITICALS — the board doesn't work

### C1 · Preamp: signal on the INVERTING input, feedback on the NON-inverting one (both channels)
**Evidence (schematic, trace with junctions):** `U1` unit A —
- pin **2 (−IN)**: `R12` (1k, input), `R11` (4M7, VBIAS), `C12` (22n), `D11.3` (clamp) → **the input net**
- pin **3 (+IN)**: `R10` (0Ω → output), `RV1`, `R13`, `C11` (DNP) → **the feedback net**
- on the board: `U1` pad 2 = `Net-(U1A--)`, pad 3 = `Net-(U1A-+)` ← `R10` ← pad 1 (OUT). Channel B the same (pads 5/6).

**Consequence:** output connected to +IN and signal on −IN is **positive feedback** — the OPA1642 latches at one of the supplies. It doesn't amplify, doesn't follow, doesn't pass sound. Both channels.

**Probable cause:** the `cotuca:OPA1642` symbol draws `−` at the bottom and `+` at the top (the opposite of the official library). When the generic 3-pin symbol was swapped for the SOIC-8 one without touching the wires, the two nets swapped pins. The "identical netlist" check counted connections, not functions.

**Fix:** swap the two nets' pins on U1A and U1B — the input net (R12/R11/C12/D11) goes to **pin 3 (+)**, the feedback one (R10/RV1/R13/C11) to **pin 2 (−)**. On the board, pads 2 and 3 are neighbors: two traces swap. Do **not** renumber the symbol — pad 2 of the real chip is −IN.

### C2 · Panel board MOSFETs: drain and source swapped
**Evidence:** the `cotuca:MOSFET_N` symbol = **1 G, 2 D, 3 S**. AO3400A datasheet (SOT-23, top view, read): **1 G, 2 S, 3 D**. On the board: `Q1` pad 2 = `LED_K_A` (LED cathode), pad 3 = `GND`.
**Consequence:** the body diode (source→drain) ends up between the LED cathode and GND: **both LEDs light permanently**, independent of the gate. Freeze A/B loses its state light.
**Fix:** swap pins 2 and 3 in the `MOSFET_N` symbol (2 = S, 3 = D) **or** swap the two nets on the pads. Confirm with the datasheet, which is already in the repo.

### C3 · D2 (ESP32 bridge Schottky) backwards
**Evidence:** footprint `Diode_SMD:D_SMA` — pad 1 = **cathode** (KiCad convention, the silk band is on pad 1). On the board `D2` pad 1 = `+5V`, pad 2 = `+5V_PONTE`. Intent documented on the power sheet: *"band = cathode, facing the bridge"*.
**Consequence:** the +5V rail doesn't power the bridge (blocked), and the ESP32's USB **feeds back the rail** — the exact defect the diode existed to prevent, and that the bench already paid for. `D1` (TVS) is correct: pad 1 (K) = +15V.
**Fix:** swap the nets of D2's pads 1↔2 (or rotate 180° and re-route). The `cotuca:D_H` symbol is generic (pins with no A/K) — give it A/K names so this doesn't happen again.

### C4 · Encoder SW1: NO pad connected
**Evidence:** the `cotuca:EC11` symbol has pins **1,2,3,4,5**; the `RotaryEncoder_Alps_EC11E-Switch` footprint has pads **A, B, C, S1, S2, MP**. On the board, the 5 signal pads have `net = None`. `ENC_A_A/B/PUSH` arrive via the IDC (vias 20–22) and stop.
**Consequence:** the encoder does nothing. The instrument's only menus are left with no input.
**Fix:** renumber the symbol's pins to the pads' names: `A`, `C` (common), `B`, `S1`, `S2` — the Alps footprint uses letters. Re-import and route (5 short traces). The "0 to route" didn't catch it because with no pad there's no connection to make.

### C5 · Signal to the amp never reaches the amp connector
**Evidence:** net `EXC_L` = only `J15.3` and `J15.6` (the "exciter" terminals of the DPDT switch). Net `AMP_IN_L` = `J11.3` + `R50` (10k to GND). **Two nets, no wire between them.** Same for R.
**Consequence:** the switch's "amp" position goes nowhere; the module's input (J11.3/4) is left with only the 10k to GND. No sound at the exciters via the board.
**Fix:** `EXC_L` and `AMP_IN_L` are the same net — rename one (suggestion: everything `AMP_IN_L/R`) on the socket sheet, re-import, route.

### C6 · Brain board with no mounting holes
**Evidence:** `Edge.Cuts` is only the rounded rectangle; zero `MountingHole` footprints, zero `np_thru_hole`. The QUEUE from the 06/09 briefing called for 4× M3 at (15,5 · 128,5) × (3,5 · 96,5) — it didn't get in.
**Consequence:** the board doesn't fasten to the bottom of the enclosure; the acrylic's ø3,2 holes have no mate.
**Fix:** 4× `MountingHole:MountingHole_3.2mm_M3` at the `pcb_contorno.py` coordinates, with a copper exclusion ring. Re-run the parity against the enclosure.

---

## 🟠 HIGH — confirm before ordering

### A1 · OLED pin order (J2) against the Waveshare cable
Board: **1 GND · 2 +5V · 3 CLK · 4 DIN · 5 CS · 6 DC · 7 RST**. The Waveshare 2,42" SPI module has, in connector order: **VCC · GND · DIN · CLK · CS · DC · RST**. With a 1:1 cable, **VCC and GND swap** (OLED dies on power-up) and DIN/CLK swap. Either reorder J2 to the module's order (recommended — the factory cable then works), or crimp a crossed cable and write that in the LEIA-ME. Confirm the order with the module in hand before deciding.

### A2 · Pots: the MPN is the horizontal style
`RV1`/`RV2` have the footprint `PTV09A-1` (vertical, bushing) and **MPN `PTV09A-4020S-B103`** (`-4` = rear mount, no bushing, shaft lying down). Bought that way, the shaft doesn't rise through the lid. Correct MPN: **`PTV09A-1020S-B103`** (Mouser/DigiKey — it's THT, doesn't go through JLC).

### A3 · The board is 2-layer; the documentation says 4
Copper: only `F.Cu` and `B.Cu`. Zones `GND` (prio 0) and `AGND` (prio 1) both on `B.Cu`, separated, `NT1` as the single bridge — **this is correct and verified**. But the briefing, the roadmap, and the ground review speak of "4 layers, L2 solid ground". Decide and align: the JLC order has to say 2 layers (cheaper), and the documents have to stop saying 4. Without an inner plane, the amp's return is the B.Cu zone — acceptable for 3 A with the width it has.

---

## 🟡 MEDIUM — tidy up, don't block

- `D2` Value `1N5819` / MPN `SS14`; `F1` Value `PTC 3A` / MPN `2920L330` (3,3 A). Value and MPN tell different stories.
- `RV1.3`/`RV2.3` (DNP trimpots) with no net — add a "no connect" flag.
- **AGND traces at 0,187 mm** between preamp pads: passes at JLC (min. 0,127), but it's thin for a return; 0,3 costs nothing.
- LCSC codes **are not in the source files** I received (neither in symbols nor on the board) — they live in the generated CSV. Check the CSV against the board before uploading.
- Panel board with no mounting holes: it hangs from the pots' and the encoder's bushings. A conscious decision? With the IDC + 5 JST pulling, it holds, but it's said.

---

## ✅ Verified and correct

| what | how |
|---|---|
| IDC contract 26/26 | pad→net of `J14` (brain board) vs `J1` (panel board), via by via: 0 divergences |
| Daisy pinout | 40 pads vs `Seed3_pinout.csv`: AGND 20, +3V3A 21, VIN 39, GND 40, audio 16–19, D29/D30 free, D26 = `BOT_BUZIOS` |
| Single ground bridge | with `NT1` excluded, `GND` and `AGND` don't share copper anywhere |
| Routing | 0 nets with more than one island on both boards (the `J12.3` connects via a trace that crosses it) |
| Polarities correct | `D1` TVS (K→+15V) · `D11/D21` BAV99 (1→AGND, 2→V+PRE, 3→signal: correct clamp) · electrolytics pad 1 = + on all |
| Power | +15V at 1,0 mm · +5V/3V3 0,5 · vias ø0,6/0,3 · nothing < 0,3 mm from the edge · minimum hole 0,5 (PicoBlade) |
| Schematic ↔ board | 70/70 and 28/28 references, values, and DNP identical |
| ESP32 socket | order of the 2×8 pads matches the SuperMini (5V·GND·3V3·4·3·2·1·0 / 5·6·7·8·9·10·20·21) |
| Decoupling | +5V (C15), +3V3A (C16), +5V_PONTE (C3+C4), V+PRE (C6+C7+C8+C9), panel board +5V (C3+C4), +3V3A (C5) |

---

## The pattern beneath the six

Five of the six criticals (C1–C5) are **the same defect**: a generic symbol from the `cotuca` library — pins `1,2,3` with no function name — matched with a real footprint whose pads have a fixed function (SOIC-8, SOT-23, D_SMA, EC11). KiCad's netlist comes out "correct" — each pin connects to a pad — and the function comes out wrong. No ERC, DRC, or "0 to route" sees it, because they all check **connection**, not **meaning**.

The check that catches this is a table per polarized part: **pad → net → function expected by the datasheet**. It's the one I did here by hand for U1, Q, D, and SW1. Worth writing it as a gate pass (`kicad` §7): for each footprint with named or polarized pads, the generator declares the function of each pad and the checker compares with the net.

**Fix order:** C1 → C2 → C3 → C5 → C4 → C6 (all in the schematic/symbols, then an "Update PCB", re-route the little that changes, DRC, and **repeat this sweep**), then A1/A2 in the BOM and LEIA-ME, A3 in the order. Only then Gerbers.
