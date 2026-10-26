# ppc/003 — 3D verification pipeline board ↔ enclosure — **rev C**
<!-- doc-check: ignore import_step (a build123d function, external library) -->
<!-- doc-check: ignore bd_warehouse (external library Apache-2.0, standoffs/screws) -->
<!-- doc-check: ignore pcb_contorno (a tool in kicad/, out of check_docs's reach) -->

> **State: PLAN.** The executable home is the **`/3ddesign`** sub-skill; the
> answers/does-NOT-answer discipline is **`/aipcb §5`**. This document is the plan
> of the 3D pipeline — what runs today (Phase A) and what matches the screwed
> enclosure rev (Phase B). It pairs with the **screwed enclosure rev** (hence **rev C**).

- **Date:** 2026-09-19
- **Branch:** `claude/slack-session-sm2ldz`
- **Reference board:** `kicad/cotuca_cerebro/cotuca_cerebro.kicad_pcb`
- **Executable home:** `.claude/skills/3ddesign/SKILL.md` (+ `references/ifa10000-m.md`)
- **Scripts:** `mech/params.py` (v0.1.0), `mech/verifica_3d.py` (v0.1.0)
- **Output:** `mech/3d/RELATORIO_3D.md` (+ `mech/3d/*.step`, gitignored)
- **Context (2 lines):** rev B proved that clearance/height/assembly have no
  gate; track M (IFÁ10000-M) closed the **mounting**, and this pipeline closes the
  **volume**. Extracted from /aipcb into /3ddesign in the /pcb→/aipcb merge.

## The 5 refinements — taken as settled

1. **Pre-flight before building.** The §0 gate runs **before** any
   measurement and **fails closed**.
2. **The gate is reproducible from OUR scripts without freecadcmd** —
   `kicad-cli` (via AppRun) + `build123d`. The CAD MCPs are **accelerators**,
   and **none is present here**.
3. **Assertions 4-6 are declared-envelope**, labeled as such — **not
   physical**. The part in hand (M9, ⟨dB⟩) confirms them.
4. **Phase A first, then Phase B.**
5. **Rev label = C** (pairs with the screwed enclosure rev).

---

## §0 — Pre-flight (proven-here)

Proven in this session (2026-09-19), baked into `/3ddesign §0`:

- Resolve the `kicad-cli` and **reject** the impostors:
  - stubs `kbcache/.../bin/kicad-cli` and `fakebin/kicad-cli` — a **48 B** shell that
    only echoes the version (measured);
  - `/usr/bin/kicad-cli` **7.0.11** — fails on *Unknown token 'generator_version'*;
  - ✅ the only valid one: `APPDIR=<appdir> <appdir>/AppRun.sh kicad-cli` → **10.0.6**,
    with the target resolved to be a **237 KB ELF** (not a script).
- **Proof per step:** `file <appdir>/bin/kicad-cli` says `ELF`; `AppRun.sh
  kicad-cli version` says `10.0.6`.
- `import build123d` → **0.12.0** ok; `import kiutils` ok. **FreeCAD and the 4 CAD
  MCPs absent** → future/optional, never baseline.

## §1 — Params single-source

`mech/params.py` is the **single source**: enclosure dimensions, **thickness 3,0 mm**
(cast acrylic), **lid angle 11,8°**, max heights per zone, edge clearance, and the
`ENVELOPE_DECLARADO` block of 4-6. **Proof:** no dimension written by hand in
another file of the 3D track; `verifica_3d.py` imports, doesn't copy.

## §2 — Reproducible pipeline (adopt-stack, freecadcmd-free)

1. Real STEP: `AppRun.sh kicad-cli pcb export step --subst-models` (AP214),
   **`KICAD10_3DMODEL_DIR` set** — unresolved paths vanish silently.
   ⚠️ this KiCad 10 **has no `--format`**. **Proof:** exit 0, a **3,19 MB** file
   (ref brain board ~3,0 MB); warnings `Fuse_2920`/`Electrosmith_Daisy_Seed` =
   known envelope.
2. Measure: `build123d` `import_step` → `Compound`; `.solids()` preserves
   `.label`/`.color`; `bounding_box()`; **`distances`** = minimum clearance;
   **`&`/`intersect`** = interference. **Proof:** **56 solids**, bbox
   **180 × 100 × 22,02 mm** of the brain board (measured).
3. Name keep-out zones: `kiutils` (offline, reads the `.kicad_pcb`).
4. Mounting/screws: `bd_warehouse` (standoffs/inserts). Laser panel:
   `boxes.py` **as an external tool, NOT vendored** (GPLv3).

Licenses: build123d/bd_warehouse Apache-2.0, kiutils MIT, boxes.py GPLv3.
Reference-only: KiCad StepUp (FreeCAD), enclozure (CadQuery).

## §3 — The 6 assertions (rigid vs declared)

- **Rigid (from the STEP):** (1) the board fits in the cavity (bbox+standoffs,
  edge clearance ≥ declared); (2) holes align board↔posts/brackets (screwed);
  (3) height per zone — the tallest component of each zone (Daisy Seed, IDC J14,
  electrolytics) < the lid interior at the **11,8° wedge PER ZONE**.
- **Declared-envelope (we assert the number, it's not a simulation):** (4) lid
  cutouts (OLED window, 16 mm pot holes Case B, encoder); (5) cables/service
  (J1↔J14, pots-to-lid Case B); (6) mountability (accessible screws, board
  removable without desoldering). **⟨dB⟩ signs.**

## §4 — Phases

- **Phase A** (runs today): params single-source + free STEP + assertions 1-3 +
  wedge per zone.
- **Phase B**: assertions 4-6 (cable/service), paired with the screwed enclosure rev.

## §5 — Reviewer's table (answers / does NOT answer) — see /aipcb §5

| tool | answers | does NOT answer |
|---|---|---|
| `kicad-cli` STEP | real geometry | clearance · cable · assembly |
| `build123d`/OCP | bbox, height, solid interference | whether the 3D model exists/is correct |
| `kiutils` | named zones from the `.kicad_pcb` | 3D geometry |
| human eye ⟨dB⟩ | envelope 4-6 — the part in hand rules | what requires measuring |

## §6 — MANIFEST integration

Each `mech/3d/*.step` and the `RELATORIO_3D.md` enter the `export/MANIFEST.md` with
a stamp + flag **«declared bbox vs real STEP»** (**diverged = red**). In this
run the STUB `params.py` (enclosure 120×80) diverges from the real brain board
(180×100) — it's the red working, not a bug.

---

## What NOT to touch

- The brain board/panel board `.kicad_pcb`: `verifica_3d.py` **only reads**. Never run a
  generator to read the report (the `pcb_contorno` scar).
- The rev labels: **rev C** stays until a board is actually fabricated
  (freeze-at-release, /aipcb Stage 8).

## Acceptance (measured)

- [x] Pre-flight rejects stubs + 7.0.11, accepts AppRun 10.0.6 (ELF 237 KB).
- [x] STEP exports > 1 MB (3,19 MB) with the 2 known-envelope warnings.
- [x] `build123d` measures bbox (56 solids, 180×100×22 mm).
- [x] Assertions 1-3 run from the STEP; 4-6 read from `params.py` as envelope.
- [x] `RELATORIO_3D.md` emitted with a stamp + MANIFEST line.
- [ ] **Phase B** (cable/service) — pending, pairs with the screwed enclosure rev.
