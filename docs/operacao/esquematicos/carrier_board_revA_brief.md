# Cotucatronics — Carrier Board rev A · project brief
### Target: KiCad → JLCPCB (fab + SMT assembly) · updated 20/08 night

## What this board is
An SMD carrier board where the **Daisy Seed plugs into a socket** and all the rest of the
instrument — power, input protection, amplification, measurement — lives in copper
instead of a jumper thicket. The panel (encoder, buttons, OLED, switches, jacks)
connects via headers: the board does not tie down the enclosure design.

## Inputs and outputs (dB's decisions, 20/08)
| what | how |
|---|---|
| Piezos (×2) | **1/4" TRS jack** — tip = signal, sleeve = ground, ring grounded |
| Exciters (×2) | **SMD lever connector (lever/Wago-style)**, 2 poles each |
| Power | **USB-C on the panel** → external powerbank/PSU, removable (checked baggage) |
| Line (×2) | 1/4" TS/TRS jack, always connected (L=A, R=B in stems; mix depending on mode) |

## Circuit blocks

### 1 · Power
```
USB-C (panel) → [CH224K: PD sink, asks 15V] → PTC fuse → switch (+V)
   → RAIL 15V ─┬→ shunt 10 mΩ + INA219 (I2C) ─┬→ AMP A (bulk 2200µF/35V + 100nF)
               │                              └→ AMP B (same)
               └→ buck 15→5V (≥2A) → RAIL 5V → Daisy VIN + BLE + OLED
```
- **CH224K** replaces the PD trigger module: SOP, ~US$0.30, config fixed at 12V.
  Declared fallback: a supply without PD 12V → 5V on the rail → logic alive (buck in
  dropout ≈ 4.7V, within the Daisy's VIN range), amps dead. An LED on the CH224K's PG
  flags "I'm at 12V" — it does not fail silently.
- **Buck 12→5V**: SMD IC class 2–3A (candidates in the JLC catalog at schematic
  time: TPS54331 / MP2315 / TPS562201). Logic draws <1A; 3× margin.
- **INA219 + shunt 10 mΩ 2512** on the positive, **on the AMPS branch, AFTER the
  fork** — the logic pulls <1 A and is constant, and in the total it would only dilute
  the resolution. Kelvin at the shunt. I2C with pull-ups on the board.
  On the carrier board there is no module: the shunt is chosen. 10 mΩ with ~4 A gives 40 mV,
  which fits in the INA219's ÷1 PGA — a 10 µV LSB = **1 mA of resolution**.
  ⚠️ The INA219's VCC is 3–5 V: power it from the 5 V rail, NEVER from the 15 V one.
- Bulk per amp: low-ESR radial electrolytic THT (JLC mixed assembly) **or** 2×
  1000µF SMD polymer per amp — decide by cost in the BOM.

### 2 · Input (×2, one per piezo)
TRS jack → 1kΩ series → node → {D1 1N4148WS→3V3 · D2 1N4148WS→GND} → AUDIO IN.
- SMD: R 0603, SOD-323 diodes. No attenuator (regime decision, 20/08);
  **pads provided for the divider's R2** (not populated) — if the `hist` in the
  sculpture calls for it, solder without a respin.
- TRS ring to ground = free cable shielding.

### 3 · Amplification (×2)
- **TPA3116D2 in mono PBTL** integrated on the board (HTSSOP w/ thermal pad — JLC
  assembles it). 12V/4Ω PBTL ≈ 15–18W per channel: plenty for the exciters.
- LC output filter per amp (10µH + 680nF): the exciters exit by WIRE — without a
  filter, the wire becomes an EMI antenna for the class-D.
- Amp enable accessible: the DPDT's physical cut acts HERE (pole 1) besides the
  audio, and pole 2 → D29/D30 (sense, internal pull-up).

### 4 · Panel headers (JST-PH 2.0 except where noted)
| header | pins |
|---|---|
| Encoder EC11 | A, B, GND, SW, GND |
| Buttons: freeze (pinball+LED), cowrie shells, +, − | 2 wires each; freeze LED +2 |
| Rotary screen selector | divider → 1 ADC pin (3 wires) |
| Switch fork mix/stems | 1 GPIO + GND |
| DPDT sense ×2 | → D29 / D30 |
| OLED Waveshare 2.42" | **MX1.25 7 pins** (receptacle directly on the board) |
| WIDI Core | 6 wires: 5V, ON, GND, TX, RX, LB |
| Panel USB-C | 2-pin VBUS/GND header coming from the chassis connector **or** 16p USB-C receptacle on the board edge — decide with the enclosure design |

### 5 · Layout rules (the DRC does not police, I draw and dB vetoes)
- **4 layers** (costs ~nothing at JLC): L2 = continuous solid ground. Class-D with
  a short return is the difference between silence and hiss.
- A single analog/digital union point, next to the Daisy socket.
- Bulk cap loop → amp pins: the smallest copper possible.
- Audio traces (piezo input) away from the switching (amps, buck) — opposite
  sides of the board; a high-impedance input is an antenna.
- Shunt with Kelvin sense; I2C kept away from the amps' LC.
- THT only where it makes mechanical sense: Daisy socket (2×20 female), bulk caps,
  lever connectors if the SMD variant isn't in the catalog.

## Flow
1. **Candidate BOM against the LCSC/JLC catalog** (real part numbers, stock
   checked) — dB's approval
2. KiCad schematic (.kicad_sch, text, versioned in the repo) — review
3. PCB + headless DRC (kicad-cli) → **render for dB's veto**
4. Gerbers + CPL + BOM → JLCPCB zip → **5 prototype boards** before any
   quantity
5. Rev B only after rev A plays

## Open (few, none blocks the schematic)
- 1/4" jacks: mounted on the board (edge) or on the panel with a header? → decide with the enclosure
- USB-C: chassis+header (already bought) vs edge receptacle → same
- Bulk: radial THT vs SMD polymer → decide in the BOM by cost
