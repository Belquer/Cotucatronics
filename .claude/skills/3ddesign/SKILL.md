---
name: 3ddesign
description: 3D mechanical integration of the board ↔ enclosure of Cotucatronics — the executable home of the IFÁ10000-M certification and of the ppc/003 pipeline. Use for "cabe na caixa?" ("does it fit in the enclosure?"), "folga da tampa" ("lid clearance"), "os furos alinham?" ("do the holes line up?"), "IFÁ10000-M", "verificação 3D" ("3D verification"), "STEP", or any board-vs-enclosure fitment question (volume, height by zone, cable, assembly). Called by /aipcb at the Mechanical Certification Gate (stage 3, before copper; and again before 8, with the real enclosure). It measures REAL geometry from the STEP — not opinion. It does not draw the schematic or route copper (that is /aipcb + kicad_layout); it does not cut the enclosure (that is /caixa).
---

# /3ddesign — the volumetric fit of the board ↔ enclosure

> **The clearance comes from the STEP, not from the drawing. The assembly comes
> from the eye, not from the number. What is asserted without measuring is called
> a declared-envelope — and it is signed, not disguised as physics.**

This skill is the **executable home** of the mechanical track of IFÁ10000 (the
**IFÁ10000-M**) and of **ppc/003**. `/aipcb` **orchestrates it** — it calls it at
the Mechanical Certification Gate the same way it calls `kicad_layout`, `caixa`,
and `revisao-terra`. It does not reimplement what they do: it measures the
**real STEP** of the board, cross-checks it with the enclosure dimensions, and
says **yes/no** with a number.

It was born from panel rev C: five boards fabricated **without a single fastening
point** because no gate asked "how does this get held, and does it fit?". The M
track answered the fastening; this skill answers the **volume**.

---

## 1 · Answers / DOES NOT answer

| tool | answers | DOES NOT answer |
|---|---|---|
| `kicad-cli` STEP export | the **real geometry** of the board in 3D (bodies, heights, outline) | clearance against the enclosure · cable · assembly |
| `build123d` / OCP | bbox, height per solid, **interference** of solids (`&`/`intersect`), minimum clearance (`distances`) | whether the 3D model **exists** and is **right** (a part with no STEP vanishes silently) |
| `kiutils` | named zones of the `.kicad_pcb` (outline, footprints, courtyards for keep-out) | any **3D** geometry — it is a 2D reader of the file |
| human eye ⟨dB⟩ | the 4-6 envelope, the dry-fit — **the piece in hand rules** | anything that requires measuring from the STEP |

**Rule (from /aipcb §5):** a gate that "passes" is only accepted after you have
seen it **refuse** under sabotage. Green on the first run is suspect.

---

## 0 · Pre-flight — the gate that fails closed

Before believing any number, the chain is proven. Any step that does not prove
what it claims **STOPS the pipeline** (exit ≠ 0).

1. **Resolve `kicad-cli` — and REJECT the impostors.**
   - ❌ **stubs**: `kbcache/.../bin/kicad-cli` and `fakebin/kicad-cli` — shell < 100 B
     that **only echoes the version**. Echoing `10.0.6` is not being KiCad 10.
   - ❌ **`/usr/bin/kicad-cli` 7.0.11** — fails to open the new board with
     *Unknown token 'generator_version'*.
   - ✅ **the only valid path:** `APPDIR=<appdir> <appdir>/AppRun.sh kicad-cli`
     reporting **10.0.6**. **Hard check:** the resolved target (`<appdir>/bin/
     kicad-cli`) is a **multi-MB ELF**, not a script. `file` says `ELF`, not
     `POSIX shell script`.
2. **The real STEP comes out** with:
   `AppRun.sh kicad-cli pcb export step --subst-models` (STEP **AP214** by
   default — label + color per solid), **with `KICAD10_3DMODEL_DIR` set**
   (unresolved paths **vanish silently**). The board outline has to be a
   **closed polygon**; **exit 0 and file > 1 MB** (brain board ref: **3.0 MB**;
   measured 3.19 MB). ⚠️ **This KiCad 10 does NOT have `--format`** — passing it
   makes `kicad-cli` spit out the help and exit (looks like exit-0 without
   writing a file). Warnings about `Fuse_2920` and `Electrosmith_Daisy_Seed` are
   **expected** unless the lib is pointed at — they are declared as a **known
   envelope**, not an error.
3. **`import build123d`** (0.12.0) has to run. **FreeCAD and the 4 CAD MCPs are
   ABSENT here** → future/optional, **never baseline**. No CAD MCP is connectable
   in this environment: the skill uses **libraries + CLI, no MCP**.

---

## 2 · The adopted stack (with license)

- **`kicad-cli` STEP export** (via AppRun) — the real geometry of the board.
- **`build123d` 0.12.0** (Apache-2.0) — the **measurement engine**:
  `import_step` → `Compound`; `.solids()` / `get_type(Solid)` preserves `.label` /
  `.color` of the AP214; `bounding_box()` → `BoundBox`; **`distances(*others)`** =
  minimum clearance; **`intersect()` / `&`** non-empty = **interference**.
- **`kiutils`** (MIT, offline) — reads outline + footprints + courtyards of the
  `.kicad_pcb` to **name** the keep-out zones (does no 3D).
- **`bd_warehouse`** (Apache-2.0) — standoffs / inserts / screws; matches the
  "everything screwed" directive of `caixa`.
- **`boxes.py`** (GPLv3) — **RUN as an external tool, DO NOT vendor** (GPL): laser
  panels + finger joints + built-in kerf, SVG/DXF output.

**Reference-only (do not install):** KiCad StepUp (FreeCAD), enclozure (CadQuery).

---

## 3 · The 6 assertions

**Rigid (physical, computed from the STEP):**

1. **The board fits in the cavity** — STEP bbox + standoffs, edge clearance ≥ the
   one declared in `params.py`.
2. **The holes align board ↔ posts/brackets** (everything screwed) — within the
   declared tolerance, ≥ 3 points.
3. **Height by zone** — the tallest component of **each zone** (Daisy Seed, IDC
   J14, electrolytics) < lid interior on the **11.8° wedge PER ZONE** (the ceiling
   varies with x; the clearance is measured zone by zone, not with a single
   number).

**Declared-envelope (we assert the number, it is NOT a simulation; ⟨dB⟩ signs):**

4. **Lid cutouts** — OLED window, 16 mm pot holes (Case B), encoder.
5. **Cables / service** — J1 ↔ J14, pots-to-lid (Case B), service clearance.
6. **Assemblability** — accessible screws, board removable **without
   desoldering**.

> **The 3-vs-4 boundary is the whole lesson:** 1-3 come out of the STEP and can
> say **no** on their own; 4-6 are assertions that the **piece in hand** (M9,
> ⟨dB⟩) confirms. Labeling 4-6 as physics would be the rev B lie all over again.

---

## 4 · Phasing

- **Phase A** (what runs today): single-source params (`mech/params.py`) + free
  STEP + assertions **1-3** + wedge by zone.
- **Phase B**: assertions **4-6** (cable / service), matched with the **screwed
  enclosure rev** — the geometry of the posts/cutouts comes in from the enclosure
  generator.

---

## 5 · Scars (one line each)

- **`kicad-cli` stub that only echoes the version.** 48 B of shell do not open a
  board.
- **`/usr/bin` 7.0.11 rejects the new board.** *Unknown token 'generator_version'*.
- **`KICAD10_3DMODEL_DIR` unset drops parts silently.** STEP comes out "ok",
  bodies missing, the height lies low.
- **`--format step` in this KiCad 10 spits out the help and exits exit-0 with no
  file.** The family is the sub-order; the output is already AP214.
- **FreeCAD not installable here.** No connectable CAD MCP. Baseline = libraries +
  CLI.
- **declared-bbox ≠ real-STEP is red, not a note.** The STUB `params.py` (120×80)
  against the real brain board (180×100) diverges — the MANIFEST has to flag it.

---

## 6 · The executable

- **`mech/params.py`** (v0.1.0) — the **single source**: enclosure dimensions,
  thickness 3.0 mm (cast acrylic), lid angle 11.8°, max heights per zone, edge
  clearance, and the `ENVELOPE_DECLARADO` block of assertions 4-6. No dimension
  written by hand anywhere else: it is imported, not copied.
- **`mech/verifica_3d.py`** (v0.1.0) — runs §0 (pre-flight, fails closed), exports
  the STEP via AppRun, measures with `build123d`, runs assertions 1-3 (4-6 as an
  envelope read from `params.py`), and writes **`mech/3d/RELATORIO_3D.md`**. It
  only reads: **it never rewrites the `.kicad_pcb`** (the scar of `pcb_contorno`
  that rewrote the fabricated brain board).

---

## 7 · MANIFEST

Each **`mech/3d/*.step`** and the **`RELATORIO_3D.md`** enter the
**`export/MANIFEST.md`** with a stamp (`carimbo: <file> · <date> · board: …`) and
the **"declared bbox vs real STEP"** flag — **diverged = red**. A 3D artifact that
is not versioned leaves no way to detect that it has aged (edicao-segura §Version
EVERYTHING). The `*.step` (multi-MB) regenerate and stay gitignored; the
`RELATORIO_3D.md` is tracked.

---

## Sibling skills

Called by **`/aipcb`** (Mechanical Certification Gate). It delegates to **`caixa`**
(enclosure geometry, the 5 gates), **`kicad_layout`** (measure per net, copper),
**`medicao-limpa`** (step 0 of the method), **`edicao-segura`** (semver bump +
report), and is the volumetric instance of the model **`ifa10000`**. The M0–M10
datasheet and the priority order live in **`references/ifa10000-m.md`**; the
register of certificates in `docs/IFA10000.md`; the pipeline plan in `docs/ppc/003`.

---

## History

- **v0.1.0 (2026-09-19)** — born from the /pcb→/aipcb merge: the volumetric 3D
  integration enclosure↔PCB is extracted into this sub-skill, the executable home
  of IFÁ10000-M (the `references/ifa10000-m.md` migrated from /aipcb to here) and
  of ppc/003. It encodes the sandbox-proven pipeline: pre-flight that rejects
  stubs and the 7.0.11, STEP AP214 via AppRun (3.19 MB, proven), measurement with
  build123d 0.12.0 (56 solids, bbox 180×100×22 mm of the brain board), rigid
  assertions 1-3 + declared-envelope 4-6. It brings `mech/params.py` and
  `mech/verifica_3d.py` (both v0.1.0). Phase A running; Phase B (cable/service)
  matched with the screwed enclosure rev.
