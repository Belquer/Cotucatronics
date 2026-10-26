# Human Gate — Inspection Questionnaire (the ⟨dB⟩ method)

Reference for the **⟨dB⟩** gates of the `/aipcb` pipeline — the ones that are not
scriptable and therefore pass through a human: the **spec sign-off** (Stage 1), the
**render / dry-fit veto** (Stage 3/7) and, above all, the **pre-payment inspection
in JLC's 3D previewer** (Stage 8). Subordinate to the gate philosophy of the
SKILL.md — **a gate that warns and lets you pass is not a gate**, **measure in the
right place, or the checkmark is worth nothing**, **prove the guard can say no**.

> **The sentence that governs this gate:** "confirm rotations" is not a gate — it is
> an invitation to misunderstanding. The human and the script imagine different
> things and the error passes. A human gate is only a gate when it is a
> **questionnaire**: one item at a time, each item anchored to a concrete place and a
> concrete visual, and **no advance without an explicit ✅**. Ambiguity is not a
> "maybe" — it is a **NO** that stops the queue.

## Index

1. [The four-field format](#1-the-four-field-format)
2. [The gate rule: one item at a time, no advance on ambiguity](#2-the-gate-rule)
3. [Anti-miscommunication principles](#3-anti-miscommunication-principles)
4. [The categories the questionnaire always covers](#4-the-categories)
5. [The generator `scripts/portao_humano.py`](#5-the-generator)
6. [Where the gate runs](#6-where-the-gate-runs)

## 1. The four-field format

Each item presents itself **alone** and carries a header (**ITEM**) plus **four
fields**, in this exact order:

- **ITEM** — designator + part + package (e.g.: `D2 — SS14 (D_SMA)`). Never "the
  diodes"; always the named part.
- **WHERE TO LOOK** — the exact place: which view of JLC's 3D previewer (rotate to
  top/bottom), which gerber layer, and the part's X/Y **coordinate** plus the
  **CPL rotation value**. The eye has to reach the right pixel without guessing.
- **WHAT YOU SHOULD SEE** — the correct state, concretely: the **band/cathode**, the
  **pin-1 dot**, the **+/−** leg, the **pinout**. What the right part, oriented
  right, shows there.
- **HOW TO CONFIRM** — what a **PASS** is and what a **FAIL** is, both written. The
  FAIL names the consequence ("flipped 180° → the diode conducts backwards") and the
  route ("fix the rotation in the CPL/footprint, do NOT pay").
- **ANSWER** — `☐ ✅ conforming` / `☐ ❌ + what you see`. The ❌ forces you to write
  **what you saw** — because a mute ❌ routes to no fix.

## 2. The gate rule

- **One item at a time.** You present the item, wait for the ANSWER, and only then
  move to the next. A list dumped all at once is read at a glance — and it is at a
  glance that the 180° passes.
- **Only ✅ advances.** Any **❌**, or any **ambiguity** ("I think so", "can't quite
  see"), **STOPS** the questionnaire and routes to a **fix** (fix
  rotation/pinout/source, or reorder the connector). Never to payment.
- **The clean pass is the precondition for payment.** Only a whole questionnaire in
  ✅ authorizes the human to pay — and **payment stays a human action**, not the
  script's. The gate authorizes; it does not pay.
- **The ❌ is a finding, not a nuisance.** It stays written (what was seen), and the
  fix closes it. A negative result is a finding — what passed stays recorded.

## 3. Anti-miscommunication principles

All the ambiguity this gate removes comes from a missing anchor. The rules:

- **Always anchor on the concrete:** designator + coordinate + CPL rotation value +
  expected visual. Never a "check the parts" or "see if the rotations are right".
- **The CPL is not script-verifiable:** the rotation in the CPL is the
  **footprint**'s (KiCad); the JLC assembles by the rotation of **its own library**.
  The **180°** danger (JLC library vs KiCad) is only seen in the 3D previewer — which
  is why the gate is human.
- **Say the PASS and the FAIL, both.** A field that only describes the correct state
  leaves the reviewer not knowing what a failure is. The FAIL names the physical
  consequence.
- **The drawing name ≠ the purchase name.** When the `Value` and the MPN/LCSC are the
  same part under different designations (e.g.: `SS14` ↔ `1N5819`), the item **flags
  it** — otherwise the silkscreen and the order speak of things that look different.
- **Polarity and orientation are distinct questions.** The band/cathode (the part
  conducts backwards) and the pin-1/rotation (the whole pinout swaps) are each
  verified in their own item, with their own FAIL.
- **The render of OUR board is the source of truth; JLC's 3D preview does not prove
  the rotation of their library** (lesson of 2026-09-11). JLCONE's 3D previewer draws
  the **3D model of the part in THEIR library** on top of our footprint — and an
  "inversion" that appears there is often an **artifact of the 3D model** (the model
  rotates vs. our footprint), **not** a board defect. You do not conclude from the
  preview alone: you **confirm** by comparing the **cathode band** or the **pin-1
  corner** of JLC's preview against the **render of our own board** (generate per-part
  crops if needed). And there is a **safety net** that does not depend on getting the
  eye right: ask for **Confirm Parts Placement = YES** — the JLC sends the placement
  sheet for you to approve before assembling. Only the **polarized / pin-1** ones are
  critical here (SMA/SMB diodes with the cathode band on the left, SOT-23 with the
  single leg, SOIC with the pin-1 triangle in the top-left corner); the **passives
  are symmetric** and do not waste the eye.

## 4. The categories

The questionnaire is **auto-derived from the board** (from the CPL + BOM) and
**always** covers these categories, in this order — and **ALWAYS opens with Item 0**
(load the package) before any inspection item:

0. **Load the right package (before anything)** — which file to load into
   JLCPCB/JLCONE and **how**, before the inspection begins. The questionnaire cannot
   assume the board is already loaded — this is the first item of every questionnaire:
   - **Gerber to load:** the exact stamped `.zip`,
     `export/<placa>_<data>_<commit>/<placa>_<data>_<commit>_gerbers.zip` — this is the
     one that goes into JLCONE (*Add Gerber*), not the `.kicad_pcb` nor loose gerbers.
   - **How to fetch it:** the package is **committed in the repo, on the PR's branch**
     — it lives on **GitHub, not necessarily on the human's machine**. ⚠️ The
     pipeline's terminal runs in the **cloud sandbox** and **does not put files on the
     human's machine**: the recovery is a command **the human runs locally**. The
     preferred path is the terminal (`git fetch` / `git checkout <branch>` / `git
     pull` and open the `export/…` folder, or `git clone -b <branch> …`); the
     **fallback** is GitHub in the browser (the `.zip`'s page → **Download raw file**,
     or **Code → Download ZIP** of the branch).
   - **Provenance:** open the package's `LEIA-ME.txt` and confirm it stamps the right
     **commit** and a **clean tree** — THAT is the revision you order (a guard against
     ordering an old or swapped package). Provenance is read in the `LEIA-ME.txt`, not
     in the CPL.
   - **SMT assembly:** when the JLC asks, add `cpl_<placa>.csv` (positions) +
     `bom_<placa>.csv` (BOM), from the SAME folder.
   - **Layers:** 2.
1. **Board-level** — number of layers, dimensions, outline vs enclosure, quantity to
   order. (⚠️ asking for 4 layers on a 2-layer drawing pays for an extra inner plane.)
2. **Polarized parts** — diodes, electrolytics, tantalum, LEDs, pin-1 ICs: each one,
   the **band/cathode/pin-1** vs the silkscreen.
3. **Packages with rotation risk** — SOT-23, SOIC/SOP, SON/QFN, SMA/SMB/SOD,
   electrolytic, modules, connectors: confirm the **orientation in 3D vs the intended
   pin-1** (the 180° danger of the JLC library vs KiCad).
4. **Connectors / pinout contracts** — OLED vs module (VCC/GND and DIN/CLK swap
   danger with a 1:1 cable), board-to-board IDC (26/26, pin-1 aligned), power polarity.
5. **BOM / assembly** — every SMT line with an LCSC in **assembly** stock (≠ stock on
   LCSC); THT (pots, radials) shipped separately; DNP correct.
6. **Mechanical** — connectors/buttons/display/pots/encoder reach the panel board's
   cutouts (dry-fit vs enclosure render).
7. **Final confirmation before paying** — the clean pass of everything above, the
   precondition for payment.

### 4a. Checklist of JLCONE order settings (part of the gate, before paying)

The rotation inspection is not the only thing you check in JLCONE — the **order
settings** are a gate on par with the parts (lesson of 2026-09-11). Before paying,
confirm, one by one:

- **Layers: 2.** (⚠️ asking for 4 on a 2-layer drawing pays for an extra inner plane.)
- **Via Tented — CRITICAL.** Our "mask-exposing-trace" oranges (§DFM) are only benign
  **with the via covered**. If the via is left uncovered, they stop being benign.
  Confirm **Via Tented** on.
- **Finish: ENIG.**
- **Assembly → Confirm Parts Placement = YES.** The rotation safety net (§3): the JLC
  sends the placement sheet for you to approve before assembling.
- **Remark the bottom-side / hand-solder parts** — flag the bottom parts or the
  hand-soldered ones (e.g.: `C16` on the brain board) in the order note.
- **Top vs Bottom Designator in the part matching.** In the part matching, JLCONE
  separates **Top and Bottom lists**. Check that the bottom parts (e.g.: `C16`) appear
  in the **Bottom** list — and did not vanish because the Top list was the only one
  you looked at.

## 5. The generator

`scripts/portao_humano.py` (read-only, deterministic) builds the questionnaire from
the board's **CPL + BOM** (optionally the `.kicad_pcb` for coordinates), in the
four-field format, covering categories **0 to 7** — always opening with **Item 0**
(load the package):

```bash
python3 scripts/portao_humano.py --cpl <cpl.csv> --bom <bom.csv> \
        --placa "<nome>" --camadas "2 camadas" \
        --pacote "<placa>_<data>_<commit>" --branch "<branch-do-PR>" \
        --out export/<placa>_<data>_<commit>/VISTORIA.md
python3 scripts/portao_humano.py --sabota      # proves it flags the known risk
```

The `--pacote` (or, if omitted, the `--out` folder's name) gives **Item 0** the exact
path of the `.zip` to load; the `--branch` says which PR branch to fetch it from. The
`--sabota` also requires that **Item 0** appear and name a `.zip`.

It auto-lists the polarized parts (by a value/footprint heuristic + BOM), the packages
with rotation risk (by footprint name:
SOT-23/SOIC/SOP/QFN/SON/SMA/SMB/SOD/electrolytic/module), the connectors/pinout
contracts, and the board-level + final-confirmation steps. The `--sabota` runs over a
synthetic board with planted defects and **requires** each known class (SOT-23,
SMB/SMA cathode, SOIC pin-1, OLED, IDC, name alias) to appear — a guard that never
flagged anything is a promise, not a verification.

The per-board artifact is the `VISTORIA.md`, written **inside the fab package**
(`export/<placa>_<data>_<commit>/`), next to the CPL/BOM/gerbers it refers to.

## 6. Where the gate runs

The same method serves the three ⟨dB⟩ gates of the pipeline:

- **Stage 1 — spec sign-off:** the diagram and the spec run the questionnaire in its
  board-level and contract form (categories 1 and 4) before spending on parts.
- **Stage 3/7 — render / dry-fit veto:** category 6 (mechanical) is the dry-fit veto
  of ONE enclosure before the batch.
- **Stage 8 — pre-payment inspection at JLC:** the full questionnaire, item by item,
  in the 3D previewer — **opening with Item 0 (load the stamped package +
  provenance)**, because the inspection cannot assume the board already loaded. **Only
  the clean pass authorizes payment.**

Without the ⟨dB⟩ in ✅, the stage stays at `INCOMPLETA` — never confused with done.
