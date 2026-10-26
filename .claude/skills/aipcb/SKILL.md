---
name: aipcb
description: Take a circuit from a plain-language specification to an ordered fabrication package (Gerbers, BOM, CPL) — "prompt in, PCB out" — with Claude proposing and verifying, KiCad delivering the verdict, and three human vetoes that do not automate. Use WHENEVER /aipcb appears, "new board"/"nova placa", "rev C", "send to JLC"/"manda para a JLC", "redraw the brain board"/"redesenha o cérebro", "new PCB"/"novo PCB", "prompt to board", "take this to the gerbers"/"leva isto até aos gerbers", a circuit spec, a schematic change, or any request that goes from an idea/specification to a fabricable board. Replaces /kicad (retired on 09/09/2026): /kicad described generators that wrote KiCad files by hand; /aipcb describes a flow in which no one writes KiCad files by hand. It also merges the /pcb orchestrator (retired on 2026-09-19): it is the end-to-end ORCHESTRATOR — it calls `kicad_layout`, `caixa`, `revisao-terra`, `design-review`, `pm`, `para_fabrica` — and certifies the mechanical board↔enclosure fit (IFÁ10000-M).
---

# /aipcb — from prompt to board

> **The verdict comes from KiCad. The meaning comes from the datasheet. The agent
> proposes and verifies; it does not serialize.**

One sentence, three clauses, and each one is a week already paid for:
- *the verdict comes from KiCad* — ERC, DRC, netlist, ratsnest, Gerbers: `kicad-cli`
  and the IPC API. Never a reader of ours deciding "it's connected".
- *the meaning comes from the datasheet* — pin 2 is `−IN` because the OPA1642 data
  sheet says so, not because the symbol drew a sign at the bottom.
- *the agent proposes and verifies; it does not serialize* — no `.py` writes
  `.kicad_sch` or `.kicad_pcb`. You write through the API, through the GUI, or you don't.

---

## 0 · What this flow is, and what it is not

**It is:** a spec goes in, a fabrication package comes out, in hours. Between the
two there are tools that produce and verify, and **three gates** where everything
stops and waits for a human — because they were the only eyes that caught an
encoder in mid-air, a connector off the board, and the silk running off the edge.

**It is not:** without vetoes. It automates production and verification. It does
not automate judgment.

**It is also the end-to-end ORCHESTRATOR** (what `/pcb` used to be): it does not
draw schematics, does not route, does not cut enclosures — it **delegates** to
whoever already knows how and enforces the gate of each stage. The executable body
of half the flow already exists in `kicad/faz_placa.py` and in the sister skills —
**rule number one is to call, never to rewrite.**

**Who does what:**

| | does | does not do |
|---|---|---|
| **chat** | spec → ppc; circuit decisions; Cowork scripts; **independent sweep** at gates 2 and 3 | does not assert a hardware fact without a source; does not write KiCad files |
| **Code** | API (`kipy`/Konnect), `kicad-cli`, our three checkers, the package, the test bench | no text patching of `.kicad_*`; does not "confirm" what it did not measure |
| **Cowork** | the screen, **only by** deterministic **script**: KiCad where there is no API, JLC, Ponoko | does not improvise, does not draw with a free mouse |
| **human** | the three vetoes, the multimeter, paying | does not solder with the circuit powered on; does not redo ground without the Daisy removed |

---

## 1 · The input: a spec must bring

Without these, the flow **does not start** — the spec is returned with what is missing:

1. **Function** in one sentence, and what is a per-unit fact (varies per unit of
   the batch: exciter gain, piezo C2) vs a series-wide fact.
2. **Fixed parts** with an MPN **from the datasheet**, not from memory — and the
   datasheet goes into `docs/datasheets/` in the same commit. An MPN without an
   open datasheet is a hypothesis, and you say so.
3. **Nets** with a human name (`AUDIO_IN_L`, not `Net-(U1A-+)`), and which ones are
   contracts between boards (the IDC) or with the enclosure (holes, edge connectors).
4. **Constraints**: enclosure (outline, holes, heights per zone), layers, power per
   net, what is an analog island and where the single ground bridge is.
5. **What is measured on the bench** before fixing it (the LED resistor value, the
   piezo C2, the DPDT type) — with the measurement written as a step, not as a note.

---

## 2 · The source

**KiCad, official library.** The official symbol brings the function per pin
(`+`, `−`, `G`, `S`, `D`, `K`, `A`), the official footprint brings the pad names
(`A B C S1 S2` on the EC11). C1, C2 and C4 of rev B could never have existed.

**A custom symbol only for what does not exist** (modules: Daisy, SuperMini,
decoy) — and mandatorily with **pins named by function**, the datasheet alongside,
and an entry in `funcao_dos_pads`. A symbol with pins `1 2 3` without names is a
defect, not a shortcut.

**If Phase 1 of the test bench proves that atopile rejects C1/C2/C4 on its own:**
the source of the schematic is the `.ato`, KiCad is layout only, and **no one edits
the schematic in the editor** — any click is lost in compilation. Decision written
here, once; it is not revisited in the middle of a rev.

**What never again:** generators that write `.kicad_sch`/`.kicad_pcb`. The rev B
ones live in `docs/rev_b/geradores/` as a record of how rev B was born.

**KiCad is the source of truth, JLCONE is the final stamp.** An open format,
versioned in git, automatable by agents, vendor-independent (consistent with the
project's ethos: HAL boundary, Konnect isolated). **EasyEDA Pro / JLCONE is the
final DFM + ordering front-end, never the source.** That is why our gate gives
**parity with JLC's DFM** (silk→pad/→hole/width rules in `.kicad_dru` +
`para_fabrica`) so that the step in JLCONE is a **stamp**, not a surprise hunt. Do
not migrate the pipeline to EasyEDA Pro (it would lose agent automation and
versioning, and create lock-in on JLC); for existing boards, at most import the
gerbers into EZEDA only for final DFM/order.

---

## Gate philosophy

The entire flow is subordinate to sentences that the sister skills bought with
lost time:

- **A gate that warns and lets you pass is not a gate** (`caixa`). Each stage must
  be able to say *no* and stop the queue.
- **Prove that the guard knows how to say no** (`funcao_dos_pads.py --sabota`). A
  guard that has never failed anything is a promise, not a check. Green on the
  first run is suspect — you only accept what you have seen reject under sabotage.
- **The generator is the source** (§2). A dimension written by hand next to a
  calculated one always ages; you import it, you do not copy it.
- **Measure at the right spot, or the check is worth nothing** (`kicad_layout`). A
  checker that measures the wrong point approves everything while looking as if it
  had checked.
- **A negative result is a finding** (`revisao-terra` R6). What passed is written
  down, or the next review redoes the work.
- **Correct is not complete** (15ez). A correction review only sees what is there
  to correct; what is left over is assumed correct. For each part, **first "what
  connects here?"**, only then "is it correct?". A migration swaps the consumer and
  leaves the supplier — look on the other side of the swap.
- **A board that cannot be assembled is not ready** (dB, 18/09: *"what good is an
  electrically correct board if we can't assemble it?"*). Assembly carries the same
  weight as connectivity, and `para_fabrica` refuses both.
- **The enclosure yields to the board — by order of priority, not as law**
  (IFÁ10000-M). It is much easier to adjust packaging than a PCB. But the board
  brings its own means of fastening: without them there is no way to secure it, and
  there it is the board that yields.

---

## Boundary — who answers what

| question | who |
|---|---|
| does the schematic connect? does the file open? ERC? | **/aipcb** (this flow) |
| board, copper, planes, silkscreen, fabrication package | `kicad_layout` |
| the enclosure closes, the board fits, the holes line up | `caixa` |
| the ground is in a legal state | `revisao-terra` |
| the drawing is readable | `design-review` |
| ordering sequence / critical path | `pm` |
| every file edit the pipeline makes | `edicao-segura` |

This flow does not decide anything in electronics that a sister skill already
decides: **it decides the ORDER, enforces the GATE of each stage, and refuses to
advance with a failed stage.** The schematic/layout work that was the old `/kicad`
skill (retired) enters through here; layout and routing run with `kicad_layout`.

---

## 3 · The three gates

Each gate has **what it requires to open**. Nothing passes on "it's almost there".
The operational machine is a pipeline of **9 stages (1–9) plus the pre-flight stage
0 and gate 3.5** — each stage passes with a number that can say **no** — and it
maps onto the three gates like this:

- **Stage 0 (pre-flight)** runs **before everything**, it is the start of the chain.
- **Stages 1–2 → Gate 1** (spec, diagram, frozen schematic, sources).
- **Stages 3, 3.5, 4, 5, 6, 7 + Risk Certification → Gate 2** (placement vs
  enclosure, congestion, copper, ground, labels, triple review — all before the
  human vetoes the layout).
- **Stage 8 + final IFÁ10000-M Mechanical Certification → Gate 3** (package, order,
  payment).
- **Stage 9 (retrospective)** runs **after delivery** — see Anti-decay.

And **two certification gates**: the **Risk** one (exit of the triple review, entry
of the package) and the **IFÁ10000-M Mechanical** one (runs at Stage 3, before the
copper, and again before Stage 8, with the real enclosure).

### Stage 0 — Pre-flight: does the tool match the files?

- **Goal:** prove that the toolchain can actually open and measure these files,
  before believing any number.
- **What it does:** confirms that the installed KiCad is the right one, that the
  files **OPEN**, and that the scripts are present.
- **If the tool is missing or too old, the pipeline is NOT blocked:** it starts a
  `scripts/bootstrap_kicad.sh` path (AppImage from GitHub — see
  `references/fab-automation.md` §0) and **re-runs the version gate** below.
- **Delegates to:** `medicao-limpa` (it is step 0 of the method — channel proven,
  environment zeroed, regime declared).
- **GATE:** ⚠️ **The MAJOR version of KiCad must equal the `(version …)` token of
  the files.** A KiCad-10 file requires KiCad 10 — a mismatch **STOPS the
  pipeline**. And `kicad-cli sch erc` / `pcb drc` must run without a loading error:
  *"the checker passed" ≠ "the sheet opens"*. A clean netlist is not a file that opens.
- **Artifact:** the version line (`kicad-cli version`) + the files' token, and the
  exit code of the first `erc`/`drc`, in the session report.

### Gate 1 — frozen schematic

**What the gate requires to open:**
- `kicad-cli sch erc` = 0 (exemptions only with a deadline, and the deadline written down)
- `funcao_dos_pads` = 0 on both boards
- contracts: `contrato_idc` 26/26 (or whatever), enclosure nets named
- ground review (skill `revisao-terra`) done on **this** schematic
- schematic render seen by the human
- **The human says "freeze".** From here on, a schematic change is the next rev.

What feeds this gate are Stages 1 and 2:

#### Stage 1 — Assessment + readable diagram

- **Goal:** turn the prompt into a spec and a diagram that a human approves before
  spending on parts.
- **What it does:** prompt → spec (function, power rails, connectors, quantities,
  mechanical constraints), and a **readable block diagram**.
- **Delegates to:** what every schematic declares (voltage+source, per-channel vs
  shared, per-unit tuning, distinct grounds with distinct symbols, assumptions —
  the schematic doctrine of this flow); `design-review` (the diagram passes through
  `tools/svg_colisoes.py` before the eye).
- **GATE:** the spec declares the six schematic items; the diagram passes
  `design-review` (exit 0/1/3, 3=non-verifiable is not approved); **⟨dB⟩** a
  sign-off marked **before** spending on parts — run as the human-gate questionnaire
  (`references/portao-humano.md`), in the board-level and pinout-contract categories.
- **Artifact:** spec + diagram committed, with the `svg_colisoes` result and the
  ⟨dB⟩ recorded.

#### Stage 2 — Component sourcing (the biggest hole)

- **Goal:** every BOM line resolved to a part that JLC actually assembles.
- **What it does:** applies the **3-level** tree — (1) an equivalent in JLC's
  assembly library → (2) import/convert the exact part
  (`easyeda2kicad`/`JLC2KiCadLib`) → (3) build from the datasheet (IPC-7351
  Nominal). See **`references/parts-sourcing.md`** for the tree, the equivalence
  checklist, and the tools table.
- **When the path is Mouser (home assembly / part outside JLC):** the sandbox
  **cannot reach Mouser** (403 egress) — `WebSearch` proves catalog, never stock.
  **The human's browser (Claude-on-Chrome) is the source of truth for
  availability:** "available" = **Ships Now ≥ qty**, and **backordered/long lead
  time count as UNAVAILABLE** (they MATCH but do not ship — the backorder slips past
  the "no match" filter). NEVER declare "in stock" from the sandbox — catalog ≠
  stock. The 3-step flow, the copy-pasteable prompt template, and the full filter
  are in **`references/parts-sourcing.md §7`**.
- **Delegates to:** `codigos_lcsc.py` (finds the C-number by value/package),
  `fichas.py` (datasheet verified from the inside), `bom_completa.py` (the generated
  BOM), and **`scripts/manifesto_fontes.py`** (the STOP gate).
- **GATE — the STOP gate:** it only advances when **every** line is in a legal
  state: (A) a valid C-number, in the assembly library (stock>0), with a tier; (B)
  footprint imported/by-hand **verified**; (C) customer-supplied verified.
  `manifesto_fontes.py` exits ≠ 0 and lists the blocked lines — proven with
  `--sabota`. ⚠️ *Stock at LCSC ≠ assembly stock at JLC.*
- **Artifact:** **sources manifest committed** (one line per BOM line), and the exit
  0 of `manifesto_fontes.py` over it.

### Gate 2 — render before routing

**What the gate requires to open:**
- fixed parts (lid holes, edge connectors, M3 holes) placed by the API at
  coordinates **derived** from the enclosure, not written by hand
- `paridade_caixa` = 0; courtyards **inside** the outline with ≥ 0.5 mm; holes with
  an exclusion radius ≥ 3.5 mm to any courtyard
- 2D render of both boards + the 3D (`product-view`) with the enclosure
- **independent sweep** (chat) over the files, not over the report
- **The human vetoes or not.** Only then: ground and audio by hand, the rest Freerouting.

The heavy lifting of this gate is Stages 3 to 7 and the Risk Certification Gate.

#### Stage 3 — Placement against the physical structure

- **Goal:** the board **has somewhere to be fastened**, and the enclosure fits onto
  it — everything that crosses the panel lands on the right cutout, before the copper.
- **What it does:** fastening holes **of the board first** (IFÁ10000-M priority 1:
  ≥3 points, near where you push); the enclosure's interior span is the board's size
  limit, and **the rest of the enclosure is derived from it** (holes, cutouts,
  posts); connectors/buttons/display land on the panel cutouts (`aberturas.json`);
  no screw under a component; height zones respected; wiring/connectors with
  clearance for the cable and reachable with the board assembled.
- **Delegates to:** `caixa` (the 5 gates, and generates the enclosure geometry
  **from the board** — IFÁ10000-M priority 2), `kicad/pcb_contorno.py` (imports the
  enclosure generator, writes `aberturas.json`), `poe_furos.py` (M3 holes derived
  from the outline).
- **GATE:** the 5 gates of `caixa` (nesting, mirror, margin, **the board fits**
  `confere_caixa`, **hole clear** `confere_furos` — both in `pcb_contorno.py`; plus
  `dentro_do_contorno.py` for courtyard×edge). ⚠️ **The footprint's origin is not
  the axis** — read the shaft in `F.Fab`. And the three the 2D DRC does not see:
  **volume (Z), access (USB-C), empty layer ≠ empty face** — each one is its own
  gate. **And steps M1–M6 of IFÁ10000-M** (interfaces, fastening, holes, sizes,
  angles, volume, via `kicad/encaixe.py`) — M2 with zero fastening points fails
  here, before there is copper to redo. ⟨dB⟩ dry fit of ONE enclosure before the batch.
- **Artifact:** `.kicad_pcb` with outline+holes, `aberturas.json`, and the
  outline↔openings handshake confirmed.

#### Stage 3.5 — Congestion forecast (pre-layout, between 3 and 4)

- **Goal:** forecast the bottleneck **BEFORE touching the copper**. Congestion
  discovered while routing is an **architecture** failure caught too late — the
  router does not solve it, it only reveals it when redoing the placement already costs.
- **What it does:** measures the placement (footprints+netlist, zero traces) by
  density and says whether each region has supply for the demand the placement
  imposes on it. The metrics: **escape-channel budget per connector** (demand =
  distinct non-plane nets · supply = broadsides with ≥3 mm open × columns); pin
  density per cell; ratsnest crossing density; copper demand vs supply per region;
  plane fragmentation in the placement. Formulas, the complete method, and the two
  rules in **`references/congestao.md`**.
- **Delegates to:** `kicad_layout` (measures per net, authority over copper and
  planes), `caixa` (the geometry that fixed the placement). Tool:
  **`scripts/congestao.py`** (escape budget + density grid; exits ≠0 on any red;
  `--sabota` proves it knows how to say no).
- **GATE — GO / REVISE-PLACEMENT:** a red region **does not advance to Stage 4**; it
  goes back to **Stage 3** (re-place), or to **1/2** if what bottlenecks is the
  **connector pinout** (row/pin order that no re-placement saves) — **never** to
  Stage 4 with an open red. **Two hard refusals:** **(a)** refuse a ≥2-row connector
  where demand > escape-channel supply; **(b)** refuse a courtyard that overlaps
  another courtyard, the board edge, or another component's body — measured by body,
  not by nominal courtyard.
- **Artifact:** **congestion map in the risk register** (one region per line,
  demand/supply measured), linked to the Risk Certification Gate. Anchors:
  **J14/C16** (brain board) — a J14 nudge physically blocked by the pre-amp, the
  congestion caught too late that became the MUST-FIX C16↔J14; **J1/C5** (panel
  board) — the **same** class of failure, latent, undone by the real 5.5 mm nudge
  that opened the second broadside and gave C5 a home. It is the **pre-layout** form
  of **IFÁ10000**: it certifies the placement against measurement, not against opinion.

#### Stage 4 — Vias + trace width (only AFTER placement)

- **Goal:** each net with copper that carries its current, vias the factory can
  drill, zero unrouted per (net, species).
- **What it does:** widths per net class sized by IPC-2221 for each net's current;
  vias ≥ JLC minimum; stitching; routing. See **`references/fab-automation.md`**
  (IPC-2221 table, JLC capabilities).
- **Delegates to:** `kicad_layout` (MANDATORY at close), `classes_de_rede.py` (the
  widths live in the classes, the DRC and the router read them), `roteia.py`/
  `roteia_fr.py`, `pcb_contorno.costura()`.
- **GATE:** **copper is only laid down after the gates of stages 3 and 3.5 close** —
  the approved placement (and with no congestion red) is the entry door, "only then"
  the vias, never before; widths in the net classes (the DRC enforces it); vias ≥
  JLC (hole ≥0.2 mm, annular ring ≥0.13 mm, ⌀ ≥0.45 mm); **per (net, species)
  unrouted = 0**, and the invariant is *no net may get worse* (the total hides a net
  that regresses; `Zone<->Zone` is only an indication). DRC with `--refill-zones`.
- **Artifact:** DRC report per net (via `le_drc.py`), net classes written in the file.

#### Stage 5 — Grounding (a whole stage just for this)

- **Goal:** each ground conductor in a legal state — what no DRC sees.
- **What it does:** a complete pass of `revisao-terra` — return paths, single
  AGND/DGND bridge, decoupling pair per consumer according to the load type, λ/20
  stitching pitch, plane on B.Cu + a via per SMD-GND pad.
- **Delegates to:** `revisao-terra` (passes P0–P7, iron rules R1–R8). See
  **`references/quality-standards.md`** §2 for the numbers (decoupling per load,
  stitching pitch, plane).
- **GATE:** passes P0–P7, each answering ONE question; a skipped pass is
  **declared**, not silenced. A finding comes out with severity+owner (R7). 🔴 a star
  ground that cannot be pointed to · AGND/DGND joined twice · a low-ESR bank at the
  input of a switching converter. ⚠️ *Do not remove the signal ground to kill noise*
  (P3b) — it is solved with local decoupling.
- **Artifact:** ground report in the fixed format of `revisao-terra` (what passed,
  findings by severity, what was NOT verified).

#### Stage 6 — Labels and text

- **Goal:** silkscreen that reads and that points to the right part.
- **What it does:** `silk_over_copper = 0`, silk outside pads/mask, refs pointing to
  **their own** part, pin-1/polarity marks; and the placement of **graphics**
  (logo/name/symbol) by **grid coordinate**, not by dragging.
- **Graphics by GRID (replaces "drag in the viewer"):** a decorative graphic is
  placed by a **coordinate read on a render with a 0.5 mm grid** (5 mm majors
  labeled, axes in the KiCad frame, no-go zones shaded, free rectangles dimensioned
  in mm), not by eye/pixel. ⚠️ **Dragging in the gerber viewer DECEIVES** — the
  viewer **does not clip** silk over a pad, the factory clips it, and the text
  arrives **broken**. The method (the graphic's bbox declared BEFORE · 1 approval
  reading the grid · 1 silk-only edit → 1 regen · a transfer that **rejects silk
  over a pad/hole**, gerber-truth) and the helper `kicad/grid_grafismo.py` are in
  **`references/grafismos-grid.md`**.
- **Delegates to:** `arruma_silk.py` (moves refs, self-polices — does not write if
  collisions go up), `refs_apontam.py` (distance to its own part vs to the
  neighbor), `rotula_tp.py` (the TP name comes from the pad's `(net)`, never from the
  `Value`), `grid_grafismo.py` (the grid map for placing graphics by coordinate),
  `design-review` (legibility with the eye).
- **GATE:** silk over copper = 0; text×text = 0; stroke ≥0.15 mm, height ≥1 mm;
  `refs_apontam.py` exits ≠0 if a designator is closer to the wrong part;
  polarity/pin-1 marks present on the **printed output**. ⚠️ *A zero count authorizes
  looking while knowing where, not skipping the look* — the field rotates with the
  part (`X = fx + dx·cos a + dy·sin a`), the obstacle is pads+tracks+silk+outline.
- **Artifact:** exit 0 of `refs_apontam.py` and of the silk gate; the `design-review`
  result in the report.

#### Stage 7 — Triple review (independent agents, 3 lenses, 3 passes)

- **Goal:** three different, independent, adversarial looks at the same board, so
  that no class of defect escapes by not being looked for.
- **What it does:** runs **three lenses**, ideally by different reviewers, and
  **runs the set 3×**:
  - **Lens A — electrical/connectivity:** ERC; `funcao_dos_pads.py`
    (pad→net→function, the *meaning*); schematic↔board parity; unrouted per net.
  - **Lens B — DFM/manufacturability:** DRC vs JLC capabilities with `.kicad_dru`;
    courtyards; BOM↔CPL↔board; rotation preview.
  - **Lens C — physical/human factors:** fit in the enclosure; silk legibility with
    the eye; connector reach; thermal.
- **Delegates to:** `kicad_layout`, `caixa`, `revisao-terra`, `design-review`,
  `revisao_externa.py` (kicad-happy, external second opinion). See
  **`references/review-protocol.md`** (the 3 lenses in detail, the findings ledger,
  adversarial reviewers). Modeled on `ifa10000`.
- **GATE:** each lens passes with **numbers, not sign-offs**; a failed lens
  **interrupts** (you do not skip it "to see if it passes"); **the last line of
  defense is not the tool that did the work**; a **findings ledger** so that nothing
  falls silently; a finding that reappears becomes a **new gate in the same commit**
  (not a note); **⟨dB⟩** the veto of the board renders in lens C (`design-review`
  goes through the eye — exit 3 non-verifiable is not approved), run as the
  mechanical category of the human-gate questionnaire (`references/portao-humano.md`).
- **Artifact:** the 3 lenses × 3 passes sheet filled in with numbers, and the
  findings ledger (severity, where, rule by name, fix).

#### Risk Certification Gate — exit of the triple review, entry of the package

Between the triple review and the gerbers there is a gate that is not one more
lens: it is the **certification**. `para_fabrica` (Stage 8) **only runs when the
risk register is GO.** An embarked warning or a waiver **is a MEASUREMENT, not an
opinion** — the same discipline that `ifa10000` demands for effects, here
instantiated for boards (the **IFÁ10000 is the project's canonical certification
protocol**; this is its form for hardware).

- **Certify against the signal, not against the noise.** Clean the environment
  artifacts before judging — a DRC screaming *"library not configured"* comes from a
  missing `fp-lib-table`, not from the board; resolve that first and only then read
  the **real residue**. **A warning you have not yet explained is not automatically
  noise.**
- **Every embarked item carries a MEASURED verdict,** one line each in the **RISK
  REGISTER**: `id · item · lens/stage · severity · MEASURED evidence · verdict
  {SAFE-EMBARK (with proof) | MUST-FIX | decisão-dB} · action`.
- **GO/NO-GO:** GO only with **zero open MUST-FIX**; SAFE-EMBARK requires **measured
  evidence** (a waiver without a number is MUST-FIX by omission); a `decisão-dB` item
  rises to the human gate ⟨dB⟩.
- **Anchor (why this exists):** the **C16↔J14** case — a "reasonable"
  courtyard-overlap waiver was, when measured body-against-body, a real collision
  (the 0805 sat inside the shroud of a flush-seated DIN-41651 box-header). The
  opinion said *waive*; the measurement said **MUST-FIX**. That gap is the whole point.

The complete register format, the *"certify against the signal"* method, and the
C16↔J14 case live in **`references/review-protocol.md`** — the risk register is the
**certification output** of the triple review. `para_fabrica` refuses NO-GO as it
refuses a dirty DRC.

### Gate 3 — preview before paying

**What the gate requires to open:**
- `kicad-cli pcb drc` = 0 with silk-over-copper and silk-off-the-board active
- KiCad's ratsnest = 0 unrouted (not a proximity counter of ours)
- zones re-filled **in this** file (fill saved; zero `filled_polygon` is a board
  without ground)
- `funcao_dos_pads` = 0 again (routing does not change nets; the sync may have
  changed them)
- BOM = CPL = board in designators; LCSC code on **every** line JLC assembles; the
  MPN may be empty, the code may not
- complete independent sweep
- the package goes up to JLC (Cowork), 3D preview opened on the list of polarized
  parts in `LEIA-ME.txt`
- **The human checks the polarities and pays.**

Before this gate, the entire mechanical track runs (IFÁ10000-M, with the real
enclosure) and Stage 8.

#### Mechanical Certification Gate — IFÁ10000-M (at Stage 3, and again before Stage 8)

The mechanical track of IFÁ10000: **the pairing of each board with any enclosure it
goes into** — holes, angles, sizes, volume, fastening technique, access and
assembly, all explored for a perfect fit.

**This gate DELEGATES to the `/3ddesign` sub-skill** — the executable home of
IFÁ10000-M and of the 3D pipeline (ppc/003). `/aipcb` **orchestrates it** here just
as it calls `kicad_layout`, `caixa` and `revisao-terra`: it does not reimplement the
volumetric fit. The complete text, the M0–M10 anchors, and the sheet are in
**`.claude/skills/3ddesign/references/ifa10000-m.md`**; the measurement engine (real
STEP via `kicad-cli` + `build123d`, the 6 assertions, the pre-flight that fails
closed) lives in `/3ddesign` (`mech/verifica_3d.py`, `mech/params.py`); the
certificate register lives in `docs/IFA10000.md`.

**The order of priorities** (it is an order, not an absolute list):

1. **The board brings its own means of fastening.** Without holes there is no way to
   secure it, and no enclosure resolves that — **here the board yields**.
2. **With that guaranteed, the enclosure yields to the board in everything else.**
   It is much easier to adjust packaging than a PCB: the enclosure regenerates in
   minutes, the board costs a factory run and weeks, and once fabricated it cannot
   be undone.
3. **The board only yields on something else with an excellent, written
   justification:** what was tried on the enclosure and why it does not work
   (measured), where the constraint the enclosure cannot absorb comes from, and ⟨dB⟩
   signs. Without the three, it is MUST-FIX on the enclosure.

**The steps:** M0 measurement (each dimension with a source; part in hand >
datasheet) · M1 interfaces · **M2 fastening** · M3 derived holes and cutouts · M4
sizes and outlines · M5 angles and orientation · M6 Z volume · M7 access and
assembly · M8 tolerances · **M9 dry fit ⟨dB⟩** · M10 register.

- **The executable:** `kicad/encaixe.py` — read-only — measures M2 (fastening), M4
  (outline), M6 (volume under the arcade buttons, **by the body**) and the
  **cables** between boards (3D path + service slack against the BOM), with
  `--sabota` proving each one. And **M1**: every connector declares what it
  connects; two paths to the same part, or the socket of a module that is already in
  a socket, are a leftover. `para_fabrica` calls it and refuses "it doesn't
  assemble". The rest is investigation, with the method written in the reference
  (the purchased model's datasheet read in the drawing, thread only if the datasheet
  shows it, purchase↔BOM↔part-in-hand, mated height, cable along the real path).
- **When it runs:** M1–M6 at **Stage 3**, before the copper (that is when changing
  the board is still cheap); the entire track **before Stage 8**, with the real enclosure.
- **GO:** all steps pass, all board concessions justified and signed. `para_fabrica`
  should not run with M2 failed.
- **Anchor:** the rev C panel board was fabricated (5 units, 12/09) **without a
  single fastening point** — designed to hang from the shafts, the pots ended up
  going on the lid by wires and the encoder has no thread. All the gates were green,
  because none of them asked "how does this get fastened?".

#### Stage 8 — Gerbers for JLCPCB

- **Goal:** a fabrication package that JLC accepts, and that says what repo state it
  came from.
- **What it does:** `para_fabrica.py` measures, refuses if bad, exports
  gerbers/drill/pos/bom (BOM **grouped by LCSC** — one physical part per line,
  v0.17.0; and a companion **`cpl_rico_*`** to JLC's CPL, so the VISTORIA and the kit
  do not lose the rich geometry, v0.18.0), stamps date/commit/dirty, writes
  `INCOMPLETA` if forced, and — on the way through — copies the lean package (zip +
  CPL + `cpl_rico` + BOM + LEIA-ME + `VISTORIA.md` + `kit/`) to
  `export/<placa>_<data>_<commit>/`, the TRACKED top-level folder, printing that
  durable path as the last line. `fabrica/` is gitignored scratch; `export/` is what
  leaves the gate and reaches the repo/PR.
- **The home path (hot plate) is the second exit, with a cost gate.** When JLC's
  assembly quote at low quantity is dominated by the **fixed cost** (setup + stencil
  + parts markup + engineering) — rule: if **assembly >> (bare PCB + parts)** for the
  quantity — route to assembling at home: order a **bare PCB + stencil (Assembly
  OFF)**, buy parts via the `kit/`'s LCSC cart, reflow the top SMD on the hot plate,
  and hand-solder the bottom + THT. **`kicad/kit_montagem.py`** (v1.0.0) emits that
  `kit/` (MAPA_COLOCACAO, COMPRA_LCSC, LEIA-ME-KIT) as this stage's standard output.
  The complete flow and cost gate are in **`references/montagem-casa.md`**.
- **Delegates to:** `para_fabrica.py` (the fab gate) and `kit_montagem.py` (the
  home-assembly kit). See **`references/fab-automation.md`** for the `kicad-cli`
  cheat-sheet, the zip's contents, the BOM-by-LCSC, the rich CPL, and the kit; and
  **`references/montagem-casa.md`** for the hot-plate path.
- **GATE:** **risk register at GO** (certification gate above — without GO,
  `para_fabrica` does not even run); DRC = 0 with JLC rules active; zones re-filled
  **in the file** (old fill ships silently); BOM = CPL = board; LCSC on every SMT
  line; **⟨dB⟩ run the human-gate questionnaire (see `references/portao-humano.md`),
  item by item — the questionnaire opens with the loading of the package (Item 0:
  which stamped `.zip` to upload to JLCONE, how to fetch it from the PR, and the
  provenance via the LEIA-ME) — and only a clean pass authorizes the payment** — the
  part-by-part rotation in JLC's previewer is not scriptable (the rotation is the
  footprint's, JLC expects its library's; the render of OUR board is the source of
  truth, and the safety net is **Confirm Parts Placement = YES** — see
  `references/portao-humano.md §3`). The same gate runs the **JLCONE order settings
  checklist** (2 layers · **Via Tented**, critical · ENIG · Confirm Parts
  Placement=YES · remark of bottom-side/hand-solder parts · bottom parts on the
  **Bottom** list in matching — `portao-humano.md §4a`); provenance stamp
  (date/commit/dirty-tree) + `INCOMPLETA` guard; **and the stamped package COMMITTED
  in `export/<placa>_<data>_<commit>/`.** The gerber delivery is PART of the gate,
  not an afterthought: a package that exists only in `fabrica/` (gitignored)
  vanishes with the container and never reaches the PR — Stage 8 **does not close**
  until `export/` is in git.
- **⚠️ Version assertion — FREEZE-AT-RELEASE (see `references/versionamento.md`).**
  Issuing a package **asserts that the silk rev matches the board's FABRICATION
  STATE**, and the rev rises **per release, not per commit**: as long as a board
  **has not been fabricated/ordered**, the gerber may iterate N times and the rev
  **STAYS** (`rev C`, no number) — the iterations are distinguished by the package's
  **commit stamp**. The rev only rises when a board is **actually
  fabricated/ordered**: there it freezes, and the **next** gerber change opens the
  next rev. **Silk is gerber**, but moving/scaling silkscreen on a not-yet-fabricated
  board generates a **new package (new stamp), same rev** — it does not raise the
  rev. The human gate verifies, via the LEIA-ME provenance, that the stamped rev and
  the printed one are the same; and the line **rev → what changed → commit** goes
  into `docs/relatorio/002.md`.
- **Artifact:** the stamped package under `export/<placa>_<data>_<commit>/` (gerber
  zip + CPL + `cpl_rico` + BOM-by-LCSC + LEIA-ME + `VISTORIA.md`, the human-gate
  questionnaire generated for the board, + the `kit/` home-assembly folder),
  **committed and recoverable from the PR**, and the record of the ⟨dB⟩ rotation (the
  `VISTORIA.md` filled in, all ✅).

#### The ⟨dB⟩ vetoes — the method is the human-gate questionnaire

Some gates **are not scriptable** — they are human, and are marked with **⟨dB⟩**.
The flow stops and waits for approval before spending money. They correspond to the
three gates:

- **⟨dB⟩ Gate 1** (Stage 1) — the readable diagram and the spec, approved **before**
  spending on parts.
- **⟨dB⟩ Gate 2** (Stage 3/7) — the VETO of the board renders, and the dry fit of
  ONE enclosure before the batch (it is the M9 of IFÁ10000-M).
- **⟨dB⟩ Gate 3** (Stage 8) — the part-by-part rotation in JLC's previewer, before
  paying. The CPL is not script-verifiable (the rotation is the footprint's, JLC
  expects its library's).
- **⟨dB⟩ IFÁ10000-M, priority 3** — every concession of the board to the enclosure,
  beyond the means of fastening, carries dB's signature with the written justification.

**All the ⟨dB⟩ gates run the same method — the human-gate questionnaire**
(`references/portao-humano.md`): one item at a time, four fields per item (WHERE TO
LOOK · WHAT YOU SHOULD SEE · HOW TO CONFIRM · ANSWER), and **only a clean pass
authorizes the next step** — at Gate 3, the payment. Any ❌ or ambiguity stops and
routes to a fix, never to paying. "Confirm rotations" is not a gate; the
questionnaire is. Without the ⟨dB⟩, the stage stays `INCOMPLETA` — it is never
confused with done.

---

## 4 · Tools by station

| station | tool | rule |
|---|---|---|
| read files without KiCad open | `rjwalters/kicad-tools` or our `kaudit.py` | only for **reading**; delimit by parentheses, never by character count |
| schematic | KiCad editor (human/Cowork) · `kicad-mcp-kipy` to read · atopile if Phase 1 says so | official library |
| board | `kicad-python` (`kipy`) · Konnect | SWIG `pcbnew` is in maintenance: no new code is written in it. ⚠️ **The socket is per process**: `/tmp/kicad/api.sock` is the project manager and refuses everything with `no handler … GetOpenDocuments`; the board editor is `api-<PID>.sock`. Use `kicad/_v2/liga.py`. (15ev) |
| verification | `kicad-cli sch erc / pcb drc / export netlist` | it is the verdict |
| routing | ground and audio by hand; Freerouting via Konnect for the rest | never "faz_placa from scratch" on an already-placed board |
| parts | LCSC/JLC via API; datasheet read with `pdftotext` and the **ordering table** checked by suffix | fuzzy search answers with confidence what it does not know: filter by strict description, not by order |
| package | `kicad-cli pcb export gerbers/drill/pos`, KiCad BOM | nothing written by us in the package |
| screen | Cowork via `docs/roteiro/NNN.md` | hotkeys, values in dialogs, typed coordinates |
| external review | kicad-happy, Konnect audits | second opinion; **not** a gate |

**Our three, and only these:**
- `funcao_dos_pads.py` — by reference, from the datasheet: pad → net → expected
  function. The contract "these pads on the same net", never an auto-generated name.
- `contrato_idc.py` — via by via between boards, from KiCad's netlist.
- `paridade_caixa.py` — outline, holes, and edge connectors against the enclosure
  generator (`caixa/cotuca_caixa_corte.py`), by courtyard. **It is the unified
  checker this flow intends for the edge mechanics; as long as it does not exist as a
  single script, the gate is realized through the gates of the `caixa` skill —
  `confere_caixa` and `confere_furos` (in `kicad/pcb_contorno.py`),
  `dentro_do_contorno.py`, and `kicad/encaixe.py` (M2/M4/M6/cables).**

Plus `svg_colisoes.py` over the exported silk, and the **sweep** (`kaudit` + `conn`
or equivalent) as an independent read. The pipeline's gates also use
**`scripts/manifesto_fontes.py`** (Stage 2), **`scripts/congestao.py`** (Stage 3.5),
and **`scripts/portao_humano.py`** (the generator of the ⟨dB⟩ questionnaire).

---

## 5 · Gates: the question they answer, and the one they do NOT answer

Every checker enters this table with both columns. It was the right-hand column that
was missing in rev B.

| checker | answers | does NOT answer |
|---|---|---|
| ERC | compatible pin types, loose labels | diode direction, pin function, D/S swapped |
| DRC | clearances, widths, silk, courtyard×courtyard | courtyard×edge (enable it), zone to be filled, pad without a net |
| ratsnest 0 | everything that has a net is connected | a pad **without** a net (symbol↔footprint mismatched) |
| "synced" | the parts match | the **nets** match — only if the synchronizer refreshes nets |
| `funcao_dos_pads` | function per pad from the datasheet | position, routing, values |
| `contrato_idc` | equal vias between boards | the OLED cable's order (it is the module's) |
| `confere_furos` | courtyard outside the 3.5 circle | nothing beyond holes |
| kicad-happy | count and shape | reading of meaning ("50/50 have an MPN" were descriptions) |
| render / eye | what the numbers do not tell | nothing that requires measuring |

**Rule:** a gate that "passes" is accepted only after you have seen it **reject**
under sabotage. Green on the first run is suspect.

---

## 6 · Message format

Shared numbering `NNN`, one per design change, all in the repo:

- `docs/ppc/NNN-titulo.md` — chat → Code. Context (2 lines), steps with **proof per
  step**, measured acceptance, what not to touch.
- `docs/relatorio/NNN.md` — Code → chat. Closed (with proof) · did not close (why) ·
  **decisions that need the human** (marked) · commits.
- `docs/varredura/NNN.md` — chat, over the files, at gates 2 and 3.
- `docs/roteiro/NNN.md` — chat → Cowork. Deterministic steps, one per line, with the
  expected state at the end of each one.
- `STATUS.md` — the queue, one page, the first file of any session.

With Slack (Claude Tag on Team/Enterprise; Claude Code in Slack on Pro/Max): the ppc
is the message, the report is the thread, the commit stays linked. The files remain
either way — Slack is transport, the repo is memory.

**Nothing that is decided lives only in the conversation.** If it is not in one of
these files, it did not happen.

---

## 7 · Scars that survive (one line each)

- **A generic symbol married to a real footprint: the netlist comes out right and
  the function comes out wrong; no connectivity gate sees it.** (15C1–C5)
- **A checker answers the question it was asked.** "Synced" was parts, not nets;
  "clear" was pads, not bodies; "zone in F.Cu" was a keepout.
- **"Near" is not "connected".** D21's via at 0.21 mm from the trace: only KiCad's
  ratsnest decides.
- **An MPN without opening the datasheet is a guess with a part number.** `-4` vs
  `-1`, `1935161` vs `1715721`, `LF` that is not lead-free, `215` that is mA.
- **A hardware fact without a source does not go into any document.** AGND/DGND
  "inside the Daisy" cost a Seed3.
- **A hand list next to a generated thing ages in silence.** The IDC table in the
  script, the `usados`, the version header.
- **A copy of a source survives changes to the original.** Call, do not copy.
- **A reader that delimits by character count is not a reader.** Five quick probes
  truncated on the same day; the definitive one delimits by parentheses.
- **A date is generated data, not written.** A hook in Code; `date` in chat.
- **A change in the middle of routing is the one that costs.** A third arcade, 4→2
  layers, 6→3 mm: freeze before, next rev after.
- **The render is the last check that sees what the numbers do not tell.** An encoder
  in mid-air, a connector off, silk on the edge — ten seconds in an image.
- **Do not rebuild the tool that already exists to avoid learning to use it.**
  Seventeen generators to avoid clicking in KiCad.
- **"Leave it to the person" looks the same whether the board is full or the mesh
  has no node in the gap.** The pitch has to divide 1.27. (15eq)
- **Identical zeros on top of everything are a broken reader, not a board.**
  `SHAPE_LINE_CHAIN.Distance()` returns 0 inside a closed chain. (15er)
- **A rounded corner: the bounding box lies.** Courtyard against the edge polygon,
  vertex by vertex. (15er)
- **"Exists" and "runs" are different claims.** `sincroniza_pcb` had no caller: the
  chain never read the schematic. (15es)
- **A name guard sees keys; only the datasheet sees meaning.** The encoder caught by
  one, the MOSFET only by the other. (15et)
- **A connector's function is in the part on the other side of the cable.** The OLED
  with the seven positions swapped and all gates green. (15eu)
- **A board must have somewhere to be fastened; the rest, the enclosure yields.** The
  rev C panel board left the factory without a fastening point. The M track of
  IFÁ10000, in this flow (IFÁ10000-M). (15ex)
- **Correct is not complete.** Three reviews saw `J4` and asked whether it was
  correct; it was the socket of a bridge that was already in a socket. First "what
  connects here?". (15ez)
- **…and of the cable that exists.** A direct cable: the contract is the position.
  Loose jumpers: it is the name of each wire. Before calling a divergence on another
  branch, read the commit that created it. (15ew)

---

## 8 · First use

`ppc/001 — banco de prova`: copy the two rev B boards **from before the
corrections** into `kicad/_lab/`, install the stack (KiCad 10 + IPC, `kicad-python`,
Konnect, `kicad-mcp-kipy`, atopile, kicad-happy), run each reviewer against the six
known defects, and return the *six defects × reviewers* table. What none of them
catches is what remains ours — and it is the final list in §4.

---

## References (progressive disclosure)

- `references/parts-sourcing.md` — the 3-level tree, the manifest's schema, the
  tools (jlcparts, easyeda2kicad, JLC2KiCadLib), URLs and commands; and the **§7
  sourcing with Claude-on-Chrome** (the human's browser is the source of truth for
  stock: *Ships Now* ≥ qty, backorder/long-lead = unavailable).
- `references/fab-automation.md` — `kicad-cli` cheat-sheet, JLC capabilities table,
  IPC-2221 width/via table, `.kicad_dru` rules, freerouting; and the gate's output:
  BOM **grouped by LCSC**, the companion **rich CPL**, and the **assembly kit**
  (`kit_montagem.py`).
- `references/montagem-casa.md` — the home path (hot plate): the **cost gate**
  (assembly >> bare+parts → home), the step-by-step flow (bare+stencil, Sn63/Pb37
  paste, top reflow, bottom+THT by hand, loupe on the SOIC/SOT), and the bonus of
  making JLC's library rotation risk moot.
- `references/quality-standards.md` — the failure-mode catalog (20 lines) and the
  hard numbers (silk, stitching, decoupling, courtyard/edge).
- **`/3ddesign`** (sub-skill) — the **mechanical certification** board↔enclosure and
  the volumetric 3D fit: the order of priorities (the board brings the fastening; the
  enclosure yields on the rest; the board only yields with a signed justification),
  the M0–M10 steps with criterion and anchor, and the sheet live in
  `.claude/skills/3ddesign/references/ifa10000-m.md`; the measurement engine (real
  STEP + `build123d`, the 6 assertions, the pre-flight) in `mech/verifica_3d.py` +
  `mech/params.py`. The Mechanical Certification Gate delegates to it; the plan is ppc/003.
- `references/review-protocol.md` — the triple review: 3 lenses A/B/C, 3 passes,
  independent/adversarial reviewers, the ledger, and the **risk register** (the
  certification output: SAFE-EMBARK/MUST-FIX/decisão-dB, GO/NO-GO).
- `references/congestao.md` — the Stage 3.5 method: the five density metrics and the
  formulas, the two hard refusals, the GO/REVISE-PLACEMENT gate, and the J14/C16 and
  J1/C5 anchors.
- `references/portao-humano.md` — the ⟨dB⟩ gate method: the four fields per item, the
  rule "one item at a time, no advancing on ambiguity", the anti-miscommunication
  principles, and the 7 categories the questionnaire always covers.
- `references/versionamento.md` — the **three layers** of version (the board's silk
  rev `rev <LETTER><N>` PER BOARD · the package stamp = short SHA in `export/` · the
  scripts' semver) and the **golden rule** (gerber/drill changes → rev rises;
  tooling/doc/BOM-grouping → does not rise). The revision history lives in
  `docs/relatorio/002.md`.
- `references/grafismos-grid.md` — the Stage 6 method for **graphics by GRID**: the
  root cause (dragging by eye generates a loop and collision with a pad; the gerber
  viewer deceives because it does not clip silk over a pad), the 6 steps (bbox
  declared before · approve reading the grid · 1 silk-only edit → 1 regen ·
  gerber-truth transfer), and the spec of the helper `kicad/grid_grafismo.py`.
- `scripts/manifesto_fontes.py` — the Stage 2 STOP gate (`--sabota` proves it knows
  how to say no).
- `scripts/congestao.py` — the Stage 3.5 gate: escape-channel budget + pin-density
  grid, read-only, exits ≠0 on red (`--sabota` proves it knows how to say no).
- `scripts/portao_humano.py` — the generator of the ⟨dB⟩ gate questionnaire (CPL +
  BOM → 4-field checklist, read-only, deterministic; writes the package's
  `VISTORIA.md`; `--sabota` proves it flags the known risk parts). Reads the
  `cpl_rico_*.csv` (the rich geometry that JLC's CPL discards).
- `scripts/bootstrap_kicad.sh` — the Stage 0 AppImage path when the installed KiCad
  is missing or too old.
- `kicad/kit_montagem.py` — the home-assembly kit (v1.0.0): package folder → `kit/`
  (MAPA_COLOCACAO.md grouped by value with TOP/BOTTOM and bottom+THT flagged ·
  COMPRA_LCSC.csv LCSC cart with Qty+spares · LEIA-ME-KIT.txt).
- `kicad/grid_grafismo.py` — the Stage 6 map for placing graphics by coordinate
  (v0.1.0, functional): top-face render with a 0.5 mm grid + 5 mm majors labeled +
  axes in the KiCad frame + no-go zones shaded (pads+holes+courtyards+margin) + the
  largest free rectangles dimensioned in mm. Emits SVG with no dependencies; requires
  KiCad 10's `pcbnew`.

## Skills this one orchestrates

`kicad_layout` · `caixa` · `revisao-terra` · `design-review` · `pm` ·
`medicao-limpa` · `edicao-segura` · `3ddesign` · `audit` · and the `ifa10000`
model. The schematic and verification work that was the old `/kicad` skill (retired
on 09/09/2026) enters through **this** flow; layout and routing run with
`kicad_layout` (MANDATORY at the close of any board). **The 3D enclosure↔PCB
integration** (volumetric IFÁ10000-M + the ppc/003 pipeline) runs in the sub-skill
**`/3ddesign`**, orchestrated by this flow at the Mechanical Certification Gate. The
end-to-end executable arm is `kicad/faz_placa.py` (schematic→fabrication package in
an order chosen by measurement) — the flow drives on top of it, it does not replace it.

## Anti-decay

A new-board run that reveals a hole in the pipeline **gains a stage, gate, or lens**
— proposed, never in silence. The pipeline that does not update itself fabricates
yesterday's board.

**Stage 9 — retrospective / method improvement** (runs after delivery): each defect
that the triple review (or the factory) caught makes the method better on the next
run — every defect becomes a **new automatic check** or a **skill edit**. Delegates
to `audit` (docs↔code↔skills consistency), `edicao-segura` (semver bump + report
which files changed), `pm` (when to order). **"What no gate caught becomes a new
gate."** A finding that stays only as a note does not close Stage 9; it has to come
out as a script, a `.kicad_dru`, a `revisao-terra` pass, or a review lens — in the
same commit.

## History

- **v1.9.1 (2026-09-19)** — **extracts the 3D enclosure↔PCB integration into the new
  `/3ddesign`; delegates the Mechanical Gate to it.** `references/ifa10000-m.md`
  migrated (`git mv`) to `.claude/skills/3ddesign/references/`; the IFÁ10000-M
  Mechanical Certification Gate now **delegates** to `/3ddesign` (as it delegates to
  `kicad_layout`/`caixa`), with the 3D measurement engine (real STEP via `kicad-cli`
  + `build123d`, `mech/verifica_3d.py` + `mech/params.py`) and the ppc/003 pipeline
  living there. `/3ddesign` enters the list of orchestrated skills; the "(to be
  created)" placeholder goes — the sub-skill exists.
- **v1.9.0 (2026-09-19)** — **merges the /pcb orchestrator (v1.8.0) into /aipcb; /pcb
  retired.** /aipcb becomes the single canonical skill: its three-gate frame + all of
  /pcb's operational machine (the 9 stages + stage 0 + gate 3.5, the Risk
  Certification Gate, the IFÁ10000-M Mechanical Certification Gate, the gate
  philosophy, the boundary, the human-gate questionnaire) folded under the three
  gates. /pcb's 10 references and 4 scripts migrated to
  `.claude/skills/aipcb/references/` and `.claude/skills/aipcb/scripts/`. The
  references to /kicad (retired) were repointed to this flow or to `kicad_layout`.
  The 3D enclosure↔PCB integration is marked for the future `/3ddesign`. Earlier
  history entries are from /pcb, kept as context:
- **v1.8.0 (2026-09-18)** — **correct is not complete** (15ez). The brain board's
  `J4` ("the ESP32 bridge") was the socket of the wire bridge; the bridge moved to a
  socket on 06/09 and it stayed, and three reviews asked whether it was correct — the
  inspection even presupposed "the other side". New philosophy at the top;
  `portao_humano` (v0.3.0) opens each connector by "what connects here?", with a
  self-test; `encaixe.py` (v0.2.0) gains **M1** — declared destinations, and the two
  signatures of a leftover (two paths to the same part; the power-only socket of a
  module already in a socket) — with sabotage. And the method gains step 8: writing
  the assembly manual is a review.
- **v1.7.0 (2026-09-18)** — **assembly becomes a criterion of done.** dB's question —
  *"what good is an electrically correct board if we can't assemble it?"* — becomes
  top philosophy and a gate: `kicad/encaixe.py` (v0.1.0, read-only; M2, M4, M6 by the
  body, cables; `--sabota`) called by `para_fabrica` v0.20.0, which now also returns
  the board intact when it refuses. The `ifa10000-m.md` reference gains the day's
  **investigation method** (seven steps). And two source fixes: `confere_arcade` now
  tests the **body** and not just the pads, and the heights table gains the IDC boxed
  header (8.85, Würth datasheet) and the test points.
- **v1.6.0 (2026-09-18)** — **Mechanical Certification Gate, IFÁ10000-M**
  (`references/ifa10000-m.md`, new): the IFÁ10000 track for pairing any board with
  any enclosure — M0–M10 with criterion and anchor, and a sheet. The doctrine
  **inverts itself, by priority and not as law** (dB's request): the board brings its
  own means of fastening; with that, **the enclosure yields to the board** on the
  rest; the board only yields with a written and signed justification. Stage 3 no
  longer takes the outline "from the enclosure model" and `caixa` is no longer the
  authority over the geometry: it generates it from the board. Born from the rev C
  panel board, fabricated without a single fastening point.
- **v1.5.0 (2026-09-12)** — **Version EVERYTHING.** `references/versionamento.md`
  gains the "Version EVERYTHING" section: the three layers (board/package/script)
  generalize to **every generated artifact that leaves the repo** — each one carries
  a **stamp** (`carimbo: <arquivo> v<N> · <data> · placas: …`) in its format (CSV
  `#`, MD HTML comment, xlsx/xls dedicated cell, package in `LEIA-ME.txt`), and the
  new `export/MANIFEST.md` is the **single source of "the newest"** with the
  delivery/consumption protocol. Born from the real root cause: a Mouser cart ended
  up with an MPN from a **superseded export** while the repo already had the new one.
- **v1.4.0 (2026-09-12)** — three lessons from the Mouser sourcing run. **(1)** Stage
  2 gains §7 of `parts-sourcing.md`: sourcing with Claude-on-Chrome — the sandbox
  cannot reach Mouser (403 egress), so the **human's browser is the source of truth**
  for availability ("available" = Ships Now ≥ qty; backordered/long-lead =
  UNAVAILABLE). **(2)** Stage 6 gains `references/grafismos-grid.md` (new) and the
  helper `kicad/grid_grafismo.py` (v0.1.0): a graphic is placed by **grid
  coordinate**, not by dragging. **(3)** MPN reconciliation: `bom_completa.py`
  (v0.9.0) and `fichas.py` (v0.10.0) load the 7 final MPNs verified on Mouser.
- **v1.3.0 (2026-09-12)** — versioning becomes **FREEZE-AT-RELEASE**
  (`references/versionamento.md` rewritten): the rev rises **per release, not per
  commit** — as long as a board is not fabricated/ordered, the gerber iterates and
  the rev **stays** (`rev C`, no number), distinguished by the **commit stamp**; the
  letter only rises when a board is actually fabricated. **Silk is gerber**, but
  touching silkscreen on a non-fabricated board generates a new package, same rev.
- **v1.2.0 (2026-09-12)** — the **versioning method** enters the skill
  (`references/versionamento.md`, new): the three layers and the golden rule. Stage 8
  gains the **version assertion** in the gate, and the revision history now lives in
  `docs/relatorio/002.md`.
- **v1.1.0 (2026-09-11)** — Stage 8 learns from the real brain board/panel board run
  (stamp `6ebbda5`): the **home-assembly path** (hot plate) with a cost gate
  (`references/montagem-casa.md`, new), the **assembly kit** as standard output
  (`kit_montagem.py` v1.0.0), the **BOM grouped by LCSC** (para_fabrica v0.17.0) and
  the companion **rich CPL** (v0.18.0), and in the human gate the **rotation
  inspection in JLC's 3D preview** + the **JLCONE order settings checklist**.
- **v1.0.0** — the 9-stage pipeline + stage 0 + gate 3.5, orchestrator (born as /pcb).
