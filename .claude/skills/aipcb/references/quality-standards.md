# Quality standards — catalog of failures + hard numbers

Reference for **stages 3, 5, 6, and 7** of the `/aipcb` pipeline. The numbers here
are all subordinate to the gate philosophy of SKILL.md — **a gate that warns and
lets it pass is not a gate**, **measure in the right place**, **a negative result
is a finding**, **the last line of defense is not the tool that did the work**,
**prove that the guard can say no**.

---

## 1. Placement vs physical structure (stage 3)

The `caixa` is the authority over the enclosure↔board geometry and its 5 gates;
this stage **verifies** that the board honors what `caixa` fixed. Constraints to
lock **before** placement, all **derived from the model** (never rewritten — a
dimension by hand ages against the calculated one):

| constraint | source | rule |
|---|---|---|
| outline | enclosure interior `W−2T × D−2T` | from the model, not drawn by eye |
| mounting holes | centering invariant | fall at (15.5·128.5)×(3.5·96.5) from the corner for **any T** |
| hole keepout | head+washer+screwdriver | ≥ ⌀ of the head, not just the hole |
| connector/button/display | cutout center line, same datum | XY + Z locked; knob shafts at 105/140/175 × 92 from the face |
| edge keepout | wall + fab | components ≥ **2.5 mm** from the edge |
| height zones | usable height, front vs back | height(footprint) ≤ ceiling of the zone under the lid/arcade |

⚠️ **The footprint's origin is not the shaft.** In the `EC11E-Switch` the origin
is on the pin row and the shaft is at (7,50·2,50). Aligning the origin to the hole
puts the pins there and leaves the shaft 7 mm to the side. You read the circle in
`F.Fab`.

**The three the 2D DRC doesn't see** (each with its own gate): **volume (Z)** — a
tall part underneath the arcade passes every 2D test and keeps the enclosure from
closing; **access** — the USB-C has to remain reachable with the board assembled;
**empty layer ≠ empty face** — a layer with no objects doesn't prove the face is
free, it proves nobody drew there. **Acceptance:** each feature facing the panel
has a *number* within the tolerance budget, plus the ⟨dB⟩ dry fit of ONE enclosure
before the series.

---

## 2. Grounding (stage 5) — numbers

Invokes `revisao-terra` (P0–P7); these are the hard values on the layout side.

### Layout rules (`kicad_layout` §11a/§11h)
- **2 layers + plane:** GND pour only on **B.Cu**, signal on F.Cu **with no
  zone**, and one via in each SMD-GND pad dropping to the plane. A pour on both
  layers leaves no signal layer — every track cuts the plane. (Cost paid: **19
  vias on the brain board, 8 on the panel board.**)
- **Via adjacency ≠ connection:** 0,9 mm of clearance isolates as well as 9 mm.
  Via to the **side** of the pad, never in the pad (via-in-pad wicks solder).
- **AGND is an island** inside GND (higher priority), with a bridge to GND at a
  **single** point (net-tie). Two bridges = mesh. The bridge has to be
  **plane-to-plane, low impedance** — a net-tie 57 mm away fed by a thin track is
  half a bridge (right topology, wrong impedance).

### Decoupling by load type
| load | set | classic error |
|---|---|---|
| logic (MCU) | **100 nF per pin** + **10 µF** bulk per rail | nothing → diffuse noise |
| audible pulsed (class-D amp) | large bank **+ 100 nF** at the pins | missing bank → cross hum |
| **switched-converter input** | **nothing large** at the input | low-ESR bank → **squeal** 🔴 |
| radio (BLE/WiFi) | **100 µF + 100 nF** local | nothing → radio comes out the speaker |
| step-down output | **100 µF — not 2200** | too much C disturbs the loop stability |

Iron rule: every consumer has **the pair** (only the large one passes the edges,
only the small one has fast energy — one alone is a finding); voltage ≥ **1,5–2×**
the rail; correct polarity; a few mm from the pins.

### Stitching (via stitching)
Pitch between **λ/20 (conservative) and λ/10**, λ = c/(f·√ε_eff) at the highest
frequency. Stitch at: layer transitions, board edges, pour bottlenecks, connector
returns, shield perimeters. There is no universal pitch — compute it from the
frequency ceiling. (For a slow audio/MCU board, edge+bottleneck+return matter far
more than a fine RF grid.)

---

## 3. Silkscreen / labels (stage 6) — numbers

| parameter | recommended | absolute minimum |
|---|---|---|
| line stroke | **0,15 mm (6 mil)** | 0,10 mm |
| character height | **1,0 mm** min; 1,5 preferred | 0,6–0,8 mm |
| silk↔pad / silk↔hole | **≥ 0,15 mm**; silk outside the mask openings | 0,10 mm |

- **Silk outside pads/holes.** Silk over exposed copper = defect: the factory cuts
  the silk over the opening and the text disappears. *Half a designator is worse
  than none — it looks like one.*
- **Legibility:** designators read from ≤ 2 directions (0° and 90°), never upside
  down or mirrored.
- **Polarity/pin-1:** cathode band, `+` on polarized caps, pin-1 dot/notch on
  every IC — on the **printed** layer.
- **TP:** the name comes from the pad's `(net …)`, **never from `Value`** (the
  `Value` lives in F.Fab, isn't printed, and lies if the point moves). Legend
  block **near** the connector, not 90 mm away.

### The rule no DRC has: "each designator points to ITS part"
Owner: **`refs_apontam.py`**. The DRC answers "is there text over something?" — it
doesn't answer "which part does this text refer to?", because that's meaning, not
forbidden geometry. A ref 11,5 mm from its part and 5 mm from the neighbor passes
everything and **identifies the wrong part** — the solderer reads `C9` next to
`R24` and shoves the cap where the resistor goes. Proven both ways (defect
injected; token renamed to scream instead of approving zero parts).

⚠️ **The field rotates with the part** (`X = fx + dx·cos a + dy·sin a`,
`Y = fy − dx·sin a + dy·cos a`). A raw offset put U2 (180°) 7,2 mm to the side — and
*there was nothing in the wrong place, so the tool said "free"*. **The mover's
guard measures with the judge's ruler** — `arruma_silk` runs the fab gate
before/after and refuses if the number goes up. Degrees: text×text = defect
always; ref over another part = defect; ref over its own = warning; ref over
copper = defect.

---

## 4. Catalog of failure modes (stage 7)

Each row: the defect, the check that catches it, the stage that owns it.
🔴 destroys a part/loses signal · 🟠 audible/visible · 🟡 latent · ⚪ hygiene.

| # | defect | sev | check | owning stage |
|---|---|---|---|---|
| 1 | wrong footprint / pitch ≠ datasheet | 🔴 | pad map vs datasheet; DFM review | B — DFM |
| 2 | wrong pin-mapping (generic symbol on a fixed footprint) | 🔴 | `funcao_dos_pads.py` (`--sabota`) | A — Electrical |
| 3 | polarized part reversed | 🔴 | polarity contract + pin-1/cathode silk + rotation preview | A + C |
| 4 | wrong connector rotation in the CPL | 🟠 | BOM↔CPL↔board + **⟨dB⟩ part-by-part preview** | B — DFM |
| 5 | silk under pads / over copper | 🟠 | `silk_over_copper` count; `arruma_silk` with 4 classes | C — Silk |
| 6 | designator points to the wrong part | 🟠 | `refs_apontam.py` | C — Silk |
| 7 | starved thermal / spoke too thin | 🟠 | `starved_thermal`; spoke ≥ 0.09/0.16 mm by weight | B — DFM |
| 8 | island left to route / shattered plane | 🔴 | unrouted by (net, kind); `Zone<->Zone` indication only | A — Electrical |
| 9 | pad orphaned from the plane (a track cut it) | 🔴 | profile per net (the total hides it) | A — Electrical |
| 10 | insufficient width for power | 🔴 | IPC-2221 from I, ΔT; widths in the net classes | A/B |
| 11 | wrong / missing decoupling | 🟠 | `revisao-terra` P4 — the *pair* per consumer | Ground |
| 12 | ground loop / AGND-DGND joined 2× | 🔴 | `revisao-terra` P3 (island↔track continuity open) | Ground |
| 13 | return crossing a moat under a fast signal | 🟠 | `revisao-terra` P5/P6; return with the signal; stitching λ/20 | Ground |
| 14 | holes missing/misplaced; screw under a part | 🔴 | `caixa` Gate 5 `confere_furos` | Enclosure |
| 15 | courtyard overlap | 🟠 | DRC courtyard; IPC-7351B 0.25 mm; measures by courtyard not by pad | B — DFM |
| 16 | USB doesn't reach the panel; tall part under the lid | 🔴 | Z-face + access + height zone (§1) | Enclosure |
| 17 | outline ≠ enclosure interior | 🔴 | outline imported from the model; invariant `pcb_contorno.py` | Enclosure |
| 18 | anonymous TP / lying label | 🟡 | label from `(net)` not from `Value`; block near the connector | C — Silk |
| 19 | edge clearance / part off the board | 🟠 | keepout ≥2.5 mm; router bounds = **outline** not the extent of the parts | B/C |
| 20 | package doesn't say what repo state it came from | ⚪→🔴 | `para_fabrica` stamps date/commit/dirty; `INCOMPLETA` | B — DFM |

---

## Key numbers (quick reference)

**Silkscreen:** stroke **0,15 mm** (0,10 floor) · height **1,0 mm** (0,6–0,8
floor) · silk↔pad **≥ 0,15 mm**, outside the mask openings.

**Stitching:** pitch **λ/20 → λ/10**, λ = c/(f·√ε_eff) at the frequency ceiling.

**Width (IPC-2221):** `A(mil²) = (I/(k·ΔT^0.44))^(1/0.725)`, width = A/thickness;
k = **0.048 external**, **0.024 internal**; 1 oz = 1.378 mil, 2 oz = 2.756 mil.

**Decoupling:** logic **100 nF/pin + 10 µF** · RF **100 µF + 100 nF** · step-down
output **100 µF (not 2200)** · switched input **no low-ESR bank**
🔴 · voltage ≥ **1,5–2×** the rail, a few mm from the pins.

**Courtyard/space/thermal/edge:** IPC-7351B excess **0,10 (L) / 0,25 (N) /
0,50 (M) mm** · edge **≥ 2,5 mm** · thermal spoke JLC floor **0,09/0,09 (1 oz),
0,16/0,16 (2 oz)** · mounting holes **2,5–5 mm** ⌀, keepout ≥ sweep of the
head/washer.

**Project facts (loaded, not re-derived):** 2 layers: pour B.Cu only, signal F.Cu
with no zone, via per SMD-GND pad (cost 19/8 vias); holes at
(15.5·128.5)×(3.5·96.5) for any T; knob shafts 105/140/175 × 92 from the face.
