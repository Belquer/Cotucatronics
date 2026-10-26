<!-- KiCad and revB generator vocabulary, not firmware symbols. check_docs
     sweeps `src/`; these live in KiCad or in
     docs/operacao/esquematicos/revB/, which it doesn't read. -->
<!-- doc-check: ignore endpoint_off_grid (KiCad ERC violation code) -->
<!-- doc-check: ignore lib_symbol_issues (KiCad ERC violation code) -->
<!-- doc-check: ignore isolated_pin_label (KiCad ERC violation code) -->
<!-- doc-check: ignore power_pin_not_driven (KiCad ERC violation code) -->
<!-- doc-check: ignore pin_not_connected (KiCad ERC violation code) -->
<!-- doc-check: ignore power_out (KiCad pin electrical type) -->
<!-- doc-check: ignore power_in (KiCad pin electrical type) -->
<!-- doc-check: ignore confere_colisoes (a function in revB/grade.py, outside the sweep) -->

# ERC rev B — exemption record
<!-- doc-check: ignore no_connect (KiCad ERC violation code) -->

**Run on 01/09 with `kicad-cli` 10.0.5.** Reports in `erc_energia.rpt` and
`erc_preamp.rpt`, next to this file.

⚠️ **The exemptions are still NOT applied anywhere.** KiCad stores them in the
`.kicad_pro`, and there is no project — the two sheets are loose files. This
document is the record; the application happens when the project is born.
**A record without application is a promise, and it's marked as such.**

## Before and after snapping to the grid

| sheet | before | after |
|---|---|---|
| energia | 68 violations · 0 errors | **13 · 0 errors** |
| preamp | 159 violations · 2 errors | **38 · 2 errors** |

The **176 `endpoint_off_grid` vanished** by fix, not by exemption — the
coordinates became multiples of 1,27 mm. See `grade.py`.

**Proof that nothing broke:** the netlist is identical before and after. Rounding
is a pure function, so points that coincided still coincide; and
`confere_colisoes` guarantees the reverse — that two distinct points didn't land
in the same spot inventing a connection.

---

## Exemption 1 — `lib_symbol_issues` · 46 warnings

> *The configuration does not include the symbol library 'cotuca'*

**Reason:** the symbols are **self-contained** in the `.kicad_sch` itself, by
design — the sheet opens on any machine with no missing symbol. KiCad warns
because it doesn't find a registered library with that name.

**Deadline:** until the sheets enter a project with the library declared.
**It disappears on its own at that point** — if it doesn't, the exemption lapses
and becomes a defect.

## Exemption 2 — `isolated_pin_label` · 3 warnings

> *`+15V` · `AUDIO_IN_L` · `AUDIO_IN_R` — label connected to a single pin*

**Reason:** these are nets that **cross sheets** via a global label. On an
isolated sheet they really do have a single pin.

**Deadline:** until the sheets are all in the same project. ⚠️ **And then the
exemption inverts:** if they still have a single pin *within the project*, it
stops being a false positive and becomes a **net with no destination** — a real
defect.

## ~~Exemption 3~~ — FIXED, not exempted

> *U1C pin 8 (`V+`) and pin 4 (`V-`): power pin with no `power_out`*

> ⚠️ **05/09 — `U1C` is now called `U1` unit 3.** The OPA1642 was three
> single-unit symbols (`U1A`/`U1B`/`U1C`) pretending to be multi-unit, which
> gave three BOM lines and three footprints for one chip. It's now a single
> three-unit symbol. Pins 4 and 8 are the same; only the name changed.

**dB's decision:** *«Declaring who supplies power is an answer; exempting is
telling the checker to look away.»* In came **four `PWR_FLAG`s**, one per power
net — and four, not two, because the ERC doesn't cross R7: `V+PRE` is a net
distinct from `+15V` and needs its own.

| flag | net | sheet |
|---|---|---|
| `#FLG1` | `+15V` | energia — source: PD decoy |
| `#FLG2` | `+5V` | energia — source: step-down |
| `#FLG1` | `V+PRE` | preamp |
| `#FLG2` | `GND` | preamp |

⚠️ **One per net, and only one.** Two `power_out` on the same net are a conflict
for the ERC — it would trade one error for another.

**Result: zero errors on both sheets.**

Three things that cost one round each, and are left for next time:

1. **The flag doesn't go at the label's coordinate** — the body covers it. `+15V`
   read as «15V» and `+5V` ended up with the triangle inside the hexagon.
2. **The `#...` reference doesn't show** — besides the KiCad convention, the text
   `#FLG1` fell on top of the `V+PRE` label.
3. **A pin in the middle of a wire does NOT connect** — only at an endpoint or
   with an explicit junction. Without one, `pin_not_connected`. And **this is
   invisible in the render**: wire and pin drawn in the same spot, with no
   connection at all.

---

## What this ERC does NOT cover

- **Footprints:** all empty, on purpose. The SMD ones are assigned already; the
  ones that depend on a part in hand (step-down, PD decoy, TRS jack, trimpot,
  button) only after measuring. **A guessed footprint is a respin.**
- **The missing sheets:** the Daisy socket + bridge + DPDT, and the panel board
  connector. None of them exist yet.
- **The file grammar**, which `check_sch.py` doesn't see — it checks the net. That's
  why the suffix duplication (`D_V_0_1_0_1`) got past it and made KiCad reject the
  whole sheet.

---

## 04/09 — `isolated_pin_label` on `AUDIO_IN_L` and `AUDIO_IN_R`

**State: ✅ DIED ON 05/09**, in the commit that created `cotuca_soquete.kicad_sch`.
`AUDIO_IN_L` connects to `C14.2` and `U2.16`; `AUDIO_IN_R` to `C24.2` and `U2.17`.
*(they were `C4A` and `C4B` until 05/09 — see the rename note at the end.)*
Zero mentions in the report. **The deadline was met as written** — and it is
the first time an exemption in this repo expires for having been resolved, and
not for someone deleting it.

```
[isolated_pin_label]: Label connected to only one pin
    @(299.72 mm,  68.58 mm): Global Label 'AUDIO_IN_L'
    @(299.72 mm, 173.99 mm): Global Label 'AUDIO_IN_R'
```

**Why it's a false positive now:** they are the preamp's outputs. Their
destination is the Daisy's audio pins, which live on the **socket sheet — which
hasn't been drawn yet**. Today the label touches a single pin only because only
one endpoint exists.

⚠️ **And this is exactly the exemption that INVERTS** (skill `kicad` §7): in a
single-sheet project it's a legitimate false positive; **as soon as the socket
sheet enters, the same line of the report comes to mean a net with no
destination — a real defect.** The same sentence, two meanings, and whoever
doesn't note the date won't notice.

**Deadline:** it dies in the commit that adds `cotuca_soquete.kicad_sch`. If it
survives that commit, **it's not an exemption, it's a defect**.

## 04/09 — `power_pin_not_driven` on U1 unit 3 (pins 4 and 8)

**State: ✅ RESOLVED on 04/09** — not by exemption, by PWR_FLAG.

Home-made symbols don't declare `power_out`, so every `power_in` pin is orphaned.
The answer is a **PWR_FLAG per power net** (skill §6), not an exemption: here the
`V+PRE` net really is powered through `R7` from `+15V`, and that's what the
design has to say.

⚠️ **ONE per net, and only one** — two `power_out` on the same net trade one error
for another. And the flag doesn't go at the label's coordinate (it covers it): it
goes at a free point on the same wire, with a junction.

**How it ended up:** a new `cotuca:PWR_FLAG` symbol (`power_out` pin, reference
`#FLG`, doesn't enter the BOM), with one flag on `V+PRE` and another on `AGND`.
Each sits at a free point on the wire, with a junction — **not** at the label's
coordinate, which would cover the label.

**One per net, and only one.** Two `power_out` on the same net trade one error for
another.

Effect: the project's ERC from **4 to 2 violations**, and the netlist **didn't
change** — 23 nets, 103 nodes. The flags power the net for the ERC without
entering as a component, which is what the `#` reference convention exists to do.

---

## 05/09 — `isolated_pin_label` × 25, panel board signals

**State:** an exemption **WITH A DEADLINE**. It expires in the commit that creates
the IDC 2×13 connector sheet (`ROTEIRO-KICAD.md` §5).

These are the labels the socket sheet carries from the Daisy to the panel board —
OLED (`RST/DC/CS/CLK/DIN`), the three encoders, the two pots, the four buttons,
and the two LEDs. Today they touch a single pin only because only one endpoint
exists.

⚠️ **It's the same exemption that inverts** (skill `kicad` §7): when the connector
sheet enters, the same line comes to mean a **net with no destination**.

## 05/09 — `pin_not_connected` × 4: D20, D21, D29, D30

**State: it's NOT an exemption — it's the correct design, and it's declared.**

`D20` and `D21` are free on purpose. **`D29` and `D30` are the USB data lines**
(pins 36/37, `USB_HS_D−`/`D+`): they can't be used, and the absence of a
connection is the correct behavior — see `PINOS.md`.

If one day these four stop appearing here, someone connected something to them,
and on the last two that kills the serial.

---

## 05/09 — the references were renamed, and why

KiCad treats a reference that **doesn't end in a digit** as unannotated, and
renumbers it on its own when saving — the correspondence with the drawings and
the BOM broke silently. The 29 that were in that state were renamed before going
to the factory. **The tens digit now tells the channel.**

| was | is | | was | is |
|---|---|---|---|---|
| `R0A`…`R4A` | `R10`…`R14` | | `R0B`…`R4B` | `R20`…`R24` |
| `C1A`…`C4A` | `C11`…`C14` | | `C1B`…`C4B` | `C21`…`C24` |
| `RVA` / `RVB` | `RV1` / `RV2` | | `DA` / `DB` | `D11` / `D21` |
| `JA` / `JB` | `J5` / `J6` | | `SWL` / `SWR` | `SW1` / `SW2` |
| `U1A`/`U1B`/`U1C` | `U1` units 1/2/3 | | `R20`/`R21` (socket) | `R50`/`R51` |

The socket's `R20`/`R21` had to leave the 2-tens because the reference is unique
across **the whole hierarchy**, not per sheet, and the preamp's channel B came to
occupy it.

**Acceptance criterion for the rename:** the netlist came out identical after
applying the map — compared by the *membership* of each net, the set of
`(reference, pin)`, and not by the net names, which change on purpose. It was
that comparison that caught a symbol (`R0A`) that had escaped the rename because
its block started stuck to the previous one's parenthesis.

---

## 05/09 — `isolated_pin_label` on `+3V3A`: **RESOLVED, and it was a defect**

The warning pointed to the Daisy's pin 21 (`+3V3A`) with a global label and no
consumer. It wasn't ERC noise: it was the symptom of the **pots being powered
from the wrong rail**.

The Seed3 datasheet says, verbatim, *"Both the Digital and Analog +3.3V pins
will output the voltage from **their regulators**"* — there are two regulators —
and *"65535 corresponding to **+3V3A**"*: the ADC's full scale is the analog one.
The manufacturer's own application figure is called *"4.1 - Potentiometer"* and
powers it from the `+3V3A` pin.

The pots were on the digital `+3V3` and read against the analog. **The difference
between two independent regulators went entirely into the reading** — which is
exactly what the panel board note already warned about, while the schematic did
the opposite of it.

**What changed (05/09):**
- IDC **via 9** went from `+3V3` to **`+3V3A`**, and powers only the two pots
  (0,66 mA);
- the **OLED went to 5 V** — the Waveshare module accepts 3,3 or 5 V, and a
  screen switching on the ADC's full scale would be worse than the problem solved;
- the `+3V3` label on pin 38 **was removed**: nothing consumes it, and a global
  label with a single pin isn't a net, it's a warning pretending to be a
  connection. The pin remains available as an output for whoever declares the
  consumer.

**It wasn't an exemption. It was a fix**, and the ground guard wasn't touched.

## 05/09 — `D29`/`D30` are no longer free pins

They are `USB_HS_D−`/`D+`. The rule was carried only in a document (scar 15by, in
which `PINOS.md` even reserved them for the DPDTs). They now have **`no_connect`
in the schematic**: the ERC now rejects anyone connecting a wire there.

The other 11 free pins do **not** carry `no_connect` — they are a reserve declared
in `FUTURO.md`, and promising "nothing connects here" would be lying about them.
That's why the brain board's ERC stays at **11 `pin_not_connected`**, and it's
the right number.
