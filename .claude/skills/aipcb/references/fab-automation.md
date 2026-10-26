# KiCad → JLCPCB fabrication automation

Reference for **stages 4 and 8** of the `/aipcb` pipeline. The headless, scriptable
path from the schematic to a fabrication package that JLCPCB accepts. Target:
**KiCad 8/9/10**. ⚠️ Always check what is version-sensitive against
`kicad-cli <sub> --help` on the installed binary — the help is the authority for your
build. And remember the **stage 0** gate: the MAJOR version of KiCad must equal the
`(version …)` token of the files.

Sources: KiCad CLI docs (`docs.kicad.org/9.0/en/cli/cli.html`,
`.../10.0/...`) · JLCPCB capabilities (`jlcpcb.com/capabilities/pcb-capabilities`)
· JLC DRC gist (`gist.github.com/darkxst/f713268e5469645425eed40115fb8b49`) ·
IPC-2221 (`resources.altium.com/p/ipc-2221-calculator-pcb-trace-current-and-heating`)
· Freerouting CLI (`github.com/freerouting/freerouting`).

> In the repo, **`kicad/para_fabrica.py`** already wraps everything that follows and
> refuses to emit the package if something is bad. This document is the background
> cheat-sheet; the pipeline **delegates** to `para_fabrica.py`.

---

## 0. KiCad missing in the environment? AppImage bootstrap

⚠️ **Before all of this you must have a `kicad-cli` whose MAJOR version matches the
`(version …)` token of the files** (stage 0 gate). If the environment has no KiCad —
or only one that is too old — the pipeline is **not blocked**: there is a proven plan B.

**Fallback chain (what failed vs. what passed, session 2026-09-10):**

| Source | Result |
|---|---|
| `apt` | KiCad **7** only — too old for v10 files |
| conda-forge | **has no** `kicad` package |
| KiCad's official PPA | **blocked** by the egress proxy (403) |
| `downloads.kicad.org` / `kicad.org` / sourceforge | **blocked** (403) |
| **AppImage on a GitHub release** | ✅ **passes** the proxy (`/releases/download/…`) |

The proxy lets through the URLs `github.com/<owner>/<repo>/releases/download/<tag>/<asset>`
(they redirect to `objects.githubusercontent.com`, reachable). GitHub's web pages
and `api.github.com` remain gated — use the direct download URL.

**Command (wrapped in `scripts/bootstrap_kicad.sh`):**
```bash
KCLI=$(.claude/skills/aipcb/scripts/bootstrap_kicad.sh 10)   # 10 = MAIOR requerido
"$KCLI" version                                            # -> 10.0.6
```
The script is **idempotent** (reuses the download/extracted tree), **verifies** the
download (HTTP-ok + minimum size, and sha256 if you pass it `KICAD_APPIMAGE_SHA256`)
and **never proceeds silently** over a partial download. The URL and version are
parameterizable (`KICAD_APPIMAGE_URL`, arg/`KICAD_MAJOR`).

**Proven asset:** `pkgforge-dev/KiCad-AppImage`, `KiCad-10.0.6-1-anylinux-x86_64.AppImage`
(~487 MB), extracted with `--appimage-extract` (**no FUSE**). Proven: `kicad-cli
version` → 10.0.6, and it **parses a board `(version 20260206)`** (the DRC ran).

**Execution notes:**
- `squashfs-root/bin/kicad-cli` is a **`sharun`** launcher: run it by the **real
  path** the script prints — **WITHOUT `LD_LIBRARY_PATH`** (it resolves the bundled
  libs by its own argv0).
- A **benign** warning on every run, ignore it:
  `Error: schema file '.../kicad/schemas/api.v1.schema.json' not found` — it is the
  IPC-API schema, nothing to do with the board parse; DRC/ERC work.
- Exit code 5 on a `pcb drc --exit-code-violations` = **it found violations** (gate
  success), not a parse failure.

> ⚠️ **CAVEAT — it is a COMMUNITY build, not KiCad's official one.** It is plan B.
> When the official one (apt/PPA/`downloads.kicad.org`) is reachable and gives the
> right MAJOR version, **always prefer it**. After the bootstrap, **re-run the stage
> 0 version gate** (`kicad-cli` MAJOR == the files' token).

Detail and parameters: **`scripts/bootstrap_kicad.sh`** (commented header).

---

## 1. `kicad-cli` — cheat-sheet (all headless)

### ERC (schematic) — build gate
```bash
kicad-cli sch erc --output erc.rpt --format report \
  --severity-error --exit-code-violations board.kicad_sch
```

### DRC (board) — build gate
```bash
kicad-cli pcb drc --output drc.json --format json \
  --schematic-parity --all-track-errors \
  --severity-error --exit-code-violations board.kicad_pcb
```
⚠️ Both exit **0 even with violations** unless `--exit-code-violations`. The DRC uses
the board rules **plus** any `.kicad_dru` (§2).

### Gerbers (2-layer JLCPCB set)
```bash
kicad-cli pcb export gerbers --output gerbers/ \
  --layers F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts \
  --use-drill-file-origin --subtract-soldermask board.kicad_pcb
```
JLCPCB reads Protel extensions **and** X2 `.gbr` names — leave Protel on, send a
consistent set. Edge.Cuts is **mandatory**. 4 layers: add `In1.Cu,In2.Cu`.

### Drill (Excellon — JLC default)
```bash
kicad-cli pcb export drill --output gerbers/ --format excellon \
  --drill-origin plot --excellon-units mm --generate-map board.kicad_pcb
```
⚠️ **Same origin** for gerbers and drill (`--use-drill-file-origin` +
`--drill-origin plot`) or the holes come out offset.

### CPL / pick-and-place
```bash
kicad-cli pcb export pos --output cpl.csv --format csv --units mm \
  --side both --smd-only --exclude-dnp board.kicad_pcb
```
⚠️⚠️ **KiCad's NATIVE POS IS NOT UPLOADABLE TO JLCONE** (measured on the real upload,
2026-09-11): JLCONE rejected the panel board's CPL with **"Failed processing the CPL
file"**. Cause: `kicad-cli` emits the native header `Ref,Val,Package,PosX,PosY,
Rot,Side` with the side in lowercase (`top`/`bottom`), and JLC's SMT parser does not
recognize it. JLC requires **exactly** `Designator,Mid X,Mid Y,Layer,Rotation`, with
`Layer` **capitalized** (`Top`/`Bottom`). `para_fabrica` **v0.16.0** does the
remapping on export (`cpl_para_jlc`: Ref→Designator, PosX→Mid X, PosY→Mid Y,
Side→Layer with Top/Bottom, Rot→Rotation; Val/Package discarded; mm with no suffix,
trailing zeros trimmed; UTF-8 without BOM) and the committed CPL is **directly
uploadable**. It is the same lesson as DFM parity: the **gate's OUTPUT must be what
the factory ACCEPTS**, not what the tool emits by default.
Rotations frequently need a per-footprint correction (a known pain — see
`parts-sourcing.md` §5).

⚠️ **JLC's CPL discards the rich geometry — so a companion is kept** (lesson of
2026-09-11, `para_fabrica` **v0.18.0**). The `cpl_para_jlc` remapping throws away
`Val` and `Package` and capitalizes the side — it is what JLC ACCEPTS, but it is also
the **only copy** of the board's `Ref,Val,Package,PosX,PosY,Rot,Side`, and the
downstream tools (the **VISTORIA** of `portao_humano.py`, the **kit** of
`kit_montagem.py`) need it. So, besides `cpl_<placa>.csv` (JLC format), the pipeline
writes **`cpl_rico_<placa>.csv`** (the complete native header) in the same folder.
Rule: **JLC's CPL is never the only copy of the rich geometry** — `cpl_rico_*` is
what the VISTORIA and the kit read, `cpl_*` is what you upload.

### BOM (native, 7+/9)
```bash
kicad-cli sch export bom --output bom.csv \
  --fields 'Value,Reference,Footprint,LCSC' \
  --labels 'Comment,Designator,Footprint,LCSC Part #' \
  --group-by 'Value,Footprint,LCSC' --exclude-dnp board.kicad_sch
```
⚠️ The LCSC code column must be named **`LCSC Part #`** — that is the key by which JLC
reads the code on the BOM upload; `LCSC` alone is not recognized. `para_fabrica`
v0.16.0 writes the column with that header (`mete_lcsc`).

⚠️ **The assembly BOM groups by LCSC number, not by comment** (lesson of 2026-09-11,
`para_fabrica` **v0.17.0**). JLCONE matches each BOM line to **one physical part**;
two lines with the same C-number (because the annotated `Value` differed — `10k — L`
vs `10k pull-down`) make it warn **"Multiple lines matched to the same part"** and
force a manual merge. The gate's output avoids that: the assembly BOM has **ONE line
per LCSC number** — designators joined in a single cell, `Qty` summed. Rule: **group
by `LCSC Part #`, never by the annotated `Comment`/`Value`** (the same C-number with
two comments is the same part; equal comments with different C-numbers are different
parts). `kicad-cli`'s native `--group-by` on `Value,Footprint,LCSC` is not enough —
the re-aggregation by LCSC is `para_fabrica`'s.

---

## 2. `.kicad_dru` — JLC rules as a DRC gate

Put `<projecto>.kicad_dru` next to the board (KiCad loads it; `kicad-cli pcb drc`
honors it). Rules oriented to JLCPCB (standard 2 layers):

⚠️⚠️ **A `#` COMMENT ONLY ON ITS OWN LINE — NEVER at the end of a `(rule` line.**
An inline `#` (e.g.: `... )))   # 5 mil`) makes KiCad's DRU parser **silently discard**
that rule and the following ones: measured on 2026-09-10, a `# 5 mil` glued to a rule
took silk→pad from 4 to 2 findings **without a single error**. A whole-line comment
(even with accents) is safe; glued to the code, not.

```
(version 1)
# minimos absolutos JLC 2 camadas (5 mil pista/folga)
(rule "min_trace_width"   (constraint track_width    (min 0.127mm)))
(rule "min_clearance"     (constraint clearance      (min 0.127mm)))
(rule "min_annular"       (constraint annular_width  (min 0.13mm)))
(rule "min_via_hole"      (constraint hole_size      (min 0.2mm)))
(rule "min_via_diameter"  (constraint via_diameter   (min 0.45mm)))
(rule "hole_to_hole"      (constraint hole_to_hole   (min 0.5mm)))
(rule "edge_clearance"    (constraint edge_clearance (min 0.3mm)))
# silk TEXTO -> pad e -> furo (folga 0,15 mm). A condicao e BIDIRECCIONAL: o KiCad
# nao garante qual item e A e qual e B, e a versao so-`A.Type=='Text'` DEIXA PASSAR
# metade dos casos (media-se — o Q1/Q2 do painel escaparam assim).
(rule "silk_text_to_pad"
  (condition "(A.Type == 'Text' && A.Layer == '?.Silkscreen' && B.Type == 'Pad') || (B.Type == 'Text' && B.Layer == '?.Silkscreen' && A.Type == 'Pad')")
  (constraint silk_clearance (min 0.15mm)))
# silk -> furo: sai como `hole_clearance` no relatorio
(rule "silk_text_to_hole"
  (condition "(A.Type == 'Text' && A.Layer == '?.Silkscreen' && B.Type == 'Pad') || (B.Type == 'Text' && B.Layer == '?.Silkscreen' && A.Type == 'Pad')")
  (constraint physical_hole_clearance (min 0.15mm)))
(rule "silk_text_thickness" (layer "?.Silkscreen") (condition "A.Type == 'Text'")
  (constraint text_thickness (min 0.15mm)))
(rule "silk_text_height"    (layer "?.Silkscreen") (condition "A.Type == 'Text'")
  (constraint text_height (min 1.0mm)))
```
4 layers: relax the inner copper to 3.5 mil (`(layer inner)` + `0.0889mm`). The
community's tested ruleset (darkxst gist) covers 2/4 layers, silk-over-copper,
silk-off-board, edge, hole-to-hole, castellated — copy and trim. This house's
complete ruleset lives in `kicad/cotuca_painel/cotuca_painel.kicad_dru`.

### 2a. Parity with JLC's DFM — the silkscreen (lesson of 2026-09-10)

⚠️ **Our gate must MEASURE WHAT THE FACTORY MEASURES.** JLCONE's DFM failed the panel
board's gerbers with silkscreen in RED that our pipeline under-counted: **silk→pad 25
red + 35 orange, silk→hole 31 red + 9 orange, trace width 50 orange**. Our DRC saw
only ~2-3 `silk_over_copper`.

The cause, measured, was threefold — and each piece is a rule that was missing:

1. **KiCad's default DRC measures silkscreen over copper with `min_silk_clearance
   = 0`**: it only flags the OVERLAPPING, never the 0.15 mm **clearance** that JLC
   requires. A `Q1`/`Q2` at 0.04 mm from a pad is red at JLC and invisible to us. →
   rule `silk_text_to_pad` (`silk_clearance` 0.15 mm), **bidirectional** condition.
2. **There was no silk→HOLE rule.** JLC measures the silkscreen against the hole, not
   just against the copper. → rule `silk_text_to_hole` (`physical_hole_clearance`
   0.15 mm, comes out as `hole_clearance`).
3. **Silkscreen trace width < 0.15 mm.** KiCad's library outlines are born at 0.12 mm,
   and **NO KiCad DRC rule measures the width of a silkscreen GRAPHIC** (`text_thickness`
   only sees text). → the source: `kicad/normaliza_silk.py` raises them to 0.15 mm
   (Stage 6, `--check` is a gate).

And the gate now **refuses** for this: `para_fabrica.py` (v0.13.0) gates on
`silk_over_copper` **filtered to text** (the footprint outline over a pad is case (b)
of `kicad_layout` §11g-ter — JLC clips it, it is not refused) + on
`text_thickness`/`text_height`; silk→hole already falls under GEOMETRY (`hole_clearance`).

> **The lesson: the default DRC UNDER-COUNTS the silkscreen against JLC.** A low
> `silk_over_copper` count is not a clean board — it is a checker measuring with 0
> clearance. The step in JLCONE must be a **stamp**, not a surprise hunt.

**The oranges (warnings), by class:** *annular ring 76* — below the preferred,
**above** the min. 0.13 mm (our rules do not refuse; acceptable). *Trace width 50* —
resolved at the source (`normaliza_silk`). *Negative mask expansion 2* and *short
slot 2* — inherent to the THT/slot footprints used; printable, acceptable as long as
the design does not change. You only act on an orange if it is cheap **and** real.

### 2a-ter. The oranges MEASURED, one by one (lesson of 2026-09-11)

⚠️ **dB's rule: no "cosmetic" dismissal — each orange is measured on the real
gerber/hole and the verdict carries the number.** With the silkscreen already at 0 red
on JLCONE, only oranges were left. Measured on the artifact (not assumed), with
`pcbnew` (features) + gerbonara/shapely (gerber), reproducing the mechanism of each
class. Each one's verdict: **manufacturable as is, ACCEPTED with the number** — none
had a fix that was real **and** cheap that did not make something else worse:

| JLCONE class | measured on the panel board | mechanism | verdict |
|---|---|---|---|
| **annular ring ~75** | min **0.15 mm** on the 96 drilled features (0 < 0.13; 0 < 0.15); 33 vias 0.6/0.3 = **JLC's standard via** | JLC counts top+base per feature at the preferred limit 0.15 | **ACCEPTED**. ≥ 0.13 min. Raising the tested pad: 17 vias drop from 0.20→~0.15 mm of clearance to foreign copper — a **worse** trade. |
| **mask opening exposing trace 14** | **34/34 vias tented** (0 opening over a via); pad mask expansion = **0.0 mm**; all mask webs ≥ 0.15 mm; trace copper exposed in our gerber = **0** (only 0.05 mm² of faceting noise) | JLC applies **its own house expansion** to our 0-expansion pads and exposes the pad/trace neck | **ACCEPTED**. Our gerber delivers 0 real exposure; the vias are already tented — there is nothing to cover. The neck stays in the solder fillet. |
| **negative mask expansion 2** + **short slot 2** | both from the **single** EC11 encoder footprint (`SW1`), the 2 mechanical fastening slots (2.8×1.5 mm PTH hole) | inherent to the EC11 mechanical footprint | **ACCEPTED**. Mounting slots — JLC fabricates EC11 mounts routinely. |
| **silk to pad 1** | min silk→pad clearance = **0.1924 mm** (next to C3), the label that `fix_text` lands on its target ~0.19 | ≥ 0.15 min (prints fine), below JLC's preferred ~0.2 | **ACCEPTED**. Pushing it above 0.2 means changing the silk REGIME of the whole board (regression risk, §11g-quinquies) — it is not a trivial "nudge". |

⚠️ **What was gained as a GATE, and what is not worth triggering:**
- **Do not** trigger annular ring at 0.15 (preferred): it would fail JLC's standard
  via — noise. The `.kicad_dru` triggers at the absolute minimum 0.13, which is the real floor.
- **Yes**, trigger on **via tenting** (`kicad/vias_tented.py`, in `para_fabrica`
  v0.15.0): the "mask opening exposing trace" class is only benign *as long as* the
  vias are tented. Un-tenting them is a choice that **no DRC refuses** and that turns
  the orange into a real red. The guard reads the tenting in the text (`(tenting ...)`
  of the setup + per-via override), without pcbnew, and proves itself in both
  directions (`--auto-teste`: default un-tented and override `none` are caught).

### 2a-bis. The 2nd round — the DRC gate kept lying (lesson of 2026-09-10)

⚠️ **The `.kicad_dru` rules above were NOT enough.** Two passes called the panel board
"clean" and JLCONE kept showing **silk→pad 18 red + 32 orange, silk→hole 11 red + 4
orange, width 50 orange**. Reproducing the numbers **on the exported gerber** (the
only truth — it is what JLC ingests), the cause was another and deeper:

1. **`kicad-cli pcb drc` does not fire the `.kicad_dru` `silk_clearance` rules.** On a
   board full of silkscreen flush to a pad, the DRC reported **0** `silk_over_copper`
   and **0** of the custom rule. The gate was measuring with a ruler that does not
   measure. *A 0 count from a checker that does not fire is the worst "clean".*
2. **JLC's red is born from our own `--subtract-soldermask`.** That flag trims the
   silkscreen **flush** to the mask opening (0 clearance) — and the coincident edge
   counts as silk-over-pad at JLC. It is only seen **in the gerber**, never on the
   board or in the DRC.
3. **The "width 50 orange" was TEXT AND GRAPHIC at EXACTLY 0.15 mm.** The bump of
   outlines to 0.15 (round 1) did not move it: 0.15 is the *minimum*, and JLC marks
   orange whatever sits **at** the minimum. The target must sit **above** — we used
   0.18 mm. And the DRC's `text_thickness` also does not catch text that is already at 0.15.

**The fix, measured at each step against the gerber:**
- **Source** (`kicad/serigrafia_dfm.py`): raises every stroke (text **and** graphic)
  to 0.18 mm; clips each silkscreen graphic — **including FILLED discs** (the
  electrolytic C3's body was a *filled* `fp_circle` over the two pads, and clipping
  only the outline does not remove the fill) — to 0.15 mm of clearance from pad and
  hole; repositions the brand text (`COTUCATRONICS` had fallen over the row of
  test-points/vias) and moves away the designator that sits next to a pad, measuring
  the **glyph in the gerber** (pcbnew's text shape **does not match** the plotted one).
- **Gate** (`kicad/mede_serigrafia.py`, called by `para_fabrica` v0.14.0 after
  exporting): measures the **gerber** — silk→pad (against the mask), silk→hole
  (against the `.drl`, incl. slots and vias) and **each trace's width** (the D01
  aperture, text and graphic). Refuses if gerbonara/shapely are missing — a gate that
  cannot measure **does not pass**.

> **The mother lesson: there is no "cosmetic" embark for silkscreen.** The "auto-clip
> / order-safe" label for silk→pad has been WRONG every single time. A "cosmetic" is a
> **measurement** (the risk-certification principle): either the gerber shows 0 reds,
> or it is not clean. Silkscreen reds go to **zero, measured on the gerber** — the
> only exception would be deleting a required pin/polarity/cathode marker, and there
> you STOP and show that item, you do not pass it in bulk.

> **And the trace lesson:** hardening the `.kicad_pro` *defaults* does not touch text
> that already exists. The gate had to **measure the trace width on the artifact**,
> not trust a default. The width lives in the gerber; the cost is reading it.

*(2026-09-11 — the brain board was taken through the same v2 step, measured on the
gerber. Before: silk→pad 23 red (F) + 2 (B), min trace 0.12 mm. After `serigrafia_dfm`:
silk→pad/→hole 0 red on both sides, every trace ≥ 0.18 mm — and each
polarity/pin-1/cathode marker (D1 SMB, D2 SMA, D11/D21 SOT-23, U1 SOIC-8, «+» of
C1/C3/C5/C6) rendered and verified to survive and to read. One fix in the tool:
`serigrafia_dfm` v0.2.0 now treats footprint text of the `PCB_TEXT` class as FP_TEXT
(the panel board only had Reference/Value; the brain board brings 17 pin labels of the
ESP32-C3 module on U3 at 0.8 mm) — before, `widen` blew up on a text's `GetShape()`.
The brain board's oranges, measured on the artifact: min annular ring 0.15 mm on 217
plated drilled features (0 < 0.13; 0 < 0.15; H1–H4 are NPTH, no ring — excluded), 72
vias 0.6/0.3 = JLC's standard via; 0 slots (the brain board does not have the panel
board's EC11 — the slot/negative-mask class does not apply: it was measured, not
assumed); mask expansion 0.0 mm and 72 vias ALL tented (0 open); silk→pad at the
limit = 0.163 mm (F) / 0.195 mm (B), ≥ 0.15 min. All ACCEPTED with the number. ⚠️
`cotuca_cerebro.kicad_dru` was NOT applied: the panel board's DRU `text_height ≥ 1.0 mm`
rule would fire on U3's 17 labels of 0.8 mm (which `para_fabrica` treats as a hard
refusal) — they are a legitimate orange (JLC prints 0.8 mm; raising them to 1.0 at the
socket's 2.54 mm pitch risked new silk→pad reds). The silk gate that RULES is the
gerber measurement, and that one passes. See report 002 §6a.)*

---

## 3. Trace width & via sizing (IPC-2221)

```
I = k · ΔT^0.44 · A^0.725   →   A(mil²) = ( I / (k·ΔT^0.44) )^(1/0.725)
largura(mil) = A / espessura ;  k=0.048 externo, k=0.024 interno
1 oz Cu = 1.378 mil ≈ 0.035 mm
```

### Table — 1 oz Cu, ΔT = 10 °C
| Current | External | Internal |
|---|---|---|
| 0.5 A | 0.13 mm (5 mil) | 0.30 mm (12 mil) |
| 1 A | 0.30 mm (12 mil) | 0.75 mm (30 mil) |
| 2 A | 0.80 mm (31 mil) | 2.0 mm (80 mil) |
| 3 A | 1.4 mm (55 mil) | 3.6 mm (140 mil) |
| 5 A | 2.8 mm (108 mil) | 7.2 mm (285 mil) |

Internal needs ~2× the width for the same current; 2 oz Cu = half the width; ΔT=20 °C
allows ~30% narrower.

**Practical defaults:** signal 0.2–0.25 mm (well above the JLC min 5 mil); power by
the table + inrush margin; **GND is a plane, not a rail**. ⚠️ The widths live in the
**net classes** (`kicad/classes_de_rede.py`) — that way the DRC and any router enforce them.

### Current per via
The barrel ≈ a track of area `π·d_furo·t_plating` (t≈0.020–0.025 mm) in the same
formula: **~1–2 A per 0.3 mm via** at ΔT 10 °C. Rule of thumb: **~1 A per 0.3 mm
via**; **stitch N vias in parallel** for power. Cheap minimum hole ≥ 0.3 mm (JLC goes
to 0.2). A via should not be a bottleneck: hole ≈ 0.75× the track width (1 oz).

---

## 4. Freerouting (KiCad 9/10)

**Reality: route power/ground and everything sensitive by hand or by script.** The
autorouter is only for dense signal fill, and always reviewed. In the repo,
`kicad/roteia_fr.py` does the round-trip via `pcbnew` (ExportSpecctraDSN/
ImportSpecctraSES) + `freerouting.jar`.

```bash
java -jar freerouting-2.x.jar -de board.dsn -do board.ses \
  -mp 100 -mt 1 -oit 0.1 -da
```
⚠️ Cautions: **it phones home** unless `-da`; **`-mt > 1` can violate constraints**
(use `-mt 1`); **the net classes carry the widths** (the router only knows what the
DSN encodes — define the classes BEFORE the export); keepouts/net-ties imperfect
(check pours and re-run the DRC after importing).
⚠️ **Do not trust the TOTAL length** — measure per net (a critical net can detour
while the total improves).

---

## 5. Reproducible fabrication package

Bare board zip: `*_F_Cu.gbr` (top) · `*_B_Cu` (bottom) · `*_F/B_Mask` ·
`*_F/B_Silkscreen` · `*_F/B_Paste` (only with stencil) · `Edge_Cuts` (**mandatory**)
· `*.DRL (+ -NPTH.DRL)`. Assembly: `BOM.csv` + `CPL.csv`.

Alongside the zip, the lean package also carries: `cpl_<placa>.csv` (JLC format) +
**`cpl_rico_<placa>.csv`** (rich geometry for the downstream tools, §1 CPL) +
`bom_<placa>.csv` (grouped by LCSC, §1 BOM) + `LEIA-ME.txt` + `VISTORIA.md` (the
human-gate questionnaire) + the **`kit/`** folder (the home-assembly kit, below).

### DFM checklist before upload
- [ ] ERC clean (`--exit-code-violations`).
- [ ] DRC clean incl. `--schematic-parity`.
- [ ] `.kicad_dru` JLC passes.
- [ ] **Zones re-filled after the last edit** (old fill ships silently).
- [ ] Edge.Cuts closed, single outline, right size.
- [ ] Silk: stroke ≥0.15 mm, height ≥1 mm; none over a pad or off the board.
- [ ] Gerber & drill share the **same origin**.
- [ ] Open the gerbers in a viewer — **eyes on the output**, not just on the DRC.
- [ ] Layers in the zip = ordered layers (2 vs 4).
- [ ] PCBA: LCSC valid and in stock; **CPL rotations checked ⟨dB⟩ part-by-part**;
      DNPs out of BOM/CPL.

`para_fabrica.py` stamps **date/commit/dirty-tree** and writes `INCOMPLETA` in the
name if forced — an incomplete package is never confused with a good one.

### Where the package lives — `export/` is the delivery, `fabrica/` is scratch
`para_fabrica.py` writes on every run to **`fabrica/`**, which is **gitignored** on
purpose (a versioned package ages silently while the board changes). But a package
that exists only in `fabrica/` **vanishes with the container and never reaches the
repo/PR** — a human would have to decide how to get it out of there. So, on the way
through, the gate **automatically** copies the lean package — the **gerber zip +
`cpl_*.csv` + `cpl_rico_*.csv` + `bom_*.csv` + `LEIA-ME.txt` + `VISTORIA.md` +
`kit/`** — to **`export/<placa>_<data>_<commit>/`**, the **tracked** top-level
folder, and prints that durable path as the last line. The `.gitignore` keeps
`fabrica/` out and only makes an exception for the `.zip` under `export/`
(`!export/**/*.zip`, against the global `*.zip`). **The gerber delivery is only done
when that `export/…` is committed** — it is part of the stage 8 gate, not an
afterthought; anyone downloads the stamped package straight from the PR.

### 5a. The home-assembly kit — `kicad/kit_montagem.py` (v1.0.0)

Stage 8 standard output **alongside** the factory package: give it a package's folder
and it emits the **`kit/`** subfolder, what you need to assemble the board **at home,
on the hot plate** (see `montagem-casa.md` for the flow and the cost gate). It reads
the `cpl_rico_*.csv` (geometry) + the `bom_*.csv` (grouped by LCSC) and writes:

- **`MAPA_COLOCACAO.md`** — the placement map, **grouped by value** (all of one value
  at once), with **TOP and BOTTOM separated**, and the **bottom** and **THT** parts
  flagged (the bottom is hand-soldered because it would fall off the plate; the THT
  goes last). It gives position and rotation — it does **not** decide the
  pin-1/cathode direction, which the `VISTORIA.md` + the 3D render decide.
- **`COMPRA_LCSC.csv`** — the LCSC cart, one line per physical part, with `Qty` and
  **`Qty+spares`** (hot-plate parts get lost/burned — take spares).
- **`LEIA-ME-KIT.txt`** — the assembly order in prose.

The kit makes **JLC's library rotation risk moot** on the home path: whoever places
(you) uses the render of our own board as the map, so JLC's library rotation does not
even come into play (see `montagem-casa.md §2`).

---

## JLCPCB fabrication capabilities (standard PCB)

| Parameter | 1–2 layers | 4+ layers |
|---|---|---|
| Min trace width/spacing | 5 mil (0.127 mm) | 3.5 mil (0.0889 mm) inner |
| Min via ⌀ (pad) | 0.45 mm | 0.45 mm (0.25 advanced) |
| Min via hole | 0.2 mm (0.3 cheap) | 0.15 advanced |
| Min annular ring | 0.13 mm | design to ≥0.15 (Class 2), 0.2 (Class 3) |
| Min hole-to-hole | 0.5 mm | 0.5 mm |
| Silk: min stroke / height | 0.15 mm / 1 mm | 0.15 mm / 1 mm |
| Edge↔track | 0.3 mm | 0.3 mm |
| Board thickness | 0.4–2.0 mm (1.6 default) | up to 2.0+ |
| Copper weight | 1 oz default | 1 oz outer / 0.5 inner |

⚠️ **Do not design to the advanced limits** (3 mil, 0.15 mm hole) on a cheap standard order.
