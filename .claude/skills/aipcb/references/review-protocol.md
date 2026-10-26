# Triple review protocol (stage 7)

Reference for **stage 7** of the `/aipcb` pipeline. Modeled on `ifa10000`: **an
explicit pass criterion per step, numbers not checkmarks, a failed step interrupts,
a negative result recorded.** Three different **lenses** so they do not repeat the
same look, each ideally by an **independent reviewer** — whoever designed it
auto-completes what they expect, not what is on the sheet — and the set runs **3×**.

> The rule that governs everything: **the last line of defense cannot be the same
> tool that did the work** (`kicad_layout` §12s). A self-test has exactly the blind
> spots of whoever wrote it — the fab gate caught a shattered plane that the router's
> self-test did not see.

---

## Why three lenses, and not a list

A single list of checks is read in one breath and tires out halfway — and defects do
not come from a single family. Three lenses force three different **mental postures**
on the same board: *does it implement the circuit?* · *can the factory build it?* ·
*does it fit in the world and in the hand?* A defect that escapes one lens falls into
another because the question is different.

---

## Lens A — Electrical / connectivity

**Question:** does the copper implement the intended circuit?

- **Clean ERC** (owner: `/aipcb`); netlist ↔ layout match; PCB synced by
  **reference, not UUID** (`sincroniza_pcb.py` — the F8 matches by UUID and duplicates
  each part in a script-generated schematic).
- **Unrouted per net** (not the total — a total improves while a net regresses).
  Invariant: **no net may get worse**, key (net, kind) — a plane shatters while the
  net's unrouted stays the same.
- **`funcao_dos_pads.py`** — pad → net → **function** (meaning, not connection). A
  generic symbol (pins 1,2,3) on a fixed-function footprint gives a **correct**
  netlist and a **wrong** function: signal on the inverting input, MOSFET source on
  the LED cathode, Schottky cathode backwards. **No ERC/DRC/"0 to route" sees this.**
  The contract is written as *"these two pads have to be on the SAME net"* (survives
  renames), literal names only for human decisions (`GND`, `+5V`). Proven with
  `--sabota`.

**Passes if:** ERC=0, schematic↔board parity OK, unrouted per net = 0 with the
invariant held, `funcao_dos_pads.py` exits 0.

---

## Lens B — Manufacturability / DFM

**Question:** do the factory and the assembler build it well — and does the factory
**accept** it?

- **DRC vs the real capabilities** with `.kicad_dru` (see `fab-automation.md` §2) and
  **`--refill-zones`** — otherwise the DRC compares with an old fill and returns
  phantom clearances. Watch out: annular ring, min track/space, min hole, **thermal
  spoke** (below 0.09/0.09 mm at 1 oz it does not etch reliably and even so passes the
  DRC).
- **`starved_thermal`** (a pad in a copper bottleneck — solders badly, conducts
  worse); **courtyards** IPC-7351B (nominal excess 0.25 mm; measure new placements by
  **courtyard, not by pad**).
- **BOM ↔ CPL ↔ board** — a three-way agreement (designator, orientation, value).
- ⚠️ **The assembly rotation is not script-verifiable** (`para_fabrica`): the CPL's
  angle is the footprint's, the factory expects its library's; for many packages they
  differ — **confirm part by part in the ⟨dB⟩ preview before paying**.
- The package comes out **stamped** with date/commit/dirty-tree; an incomplete package
  is impossible to confuse with a good one (`INCOMPLETA` in the name).

**Passes if:** DRC=0 with JLC rules and zones refilled, courtyards without overlap,
BOM=CPL=board, ⟨dB⟩ rotation confirmed, stamp present.

---

## Lens C — Physical / mechanical + human factors

**Question:** does it fit in the world and in the human who uses it?

- **Fit in the enclosure** (`caixa` gates 4/5; volume + access + face — see
  `quality-standards.md` §1).
- **Silk legibility with the EYE** (`design-review`) — a zero count authorizes looking
  *knowing where*, not skipping the eye; `refs_apontam.py` for wrong-part.
- **Connector reach**, **thermal** (part-to-part space for heat, copper for current),
  and "reads from the side you solder" — which **only the eye** confirms.

**Passes if:** the `caixa` gates pass, the `design-review` (svg_colisoes + eye)
passes, `refs_apontam.py` exits 0, and reach/thermal are annotated with a number.

---

## Independent / adversarial reviewers

- **A different person per lens where possible;** at minimum, the last line of defense
  is not the tool that did the work (§12s).
- The reviewer is **adversarial**: tries to prove the board is wrong, not that it is
  right. "Looks fine" is not a result.
- When the pipeline runs with multiple agents, each lens is a separate agent with its
  own tool and its own sheet — and a fourth agent reconciles the ledger.

---

## The ledger of findings (so nothing falls in silence)

Fixed format (joins `revisao-terra` + `ifa10000`), filled **per pass**:

1. **What was reviewed** — file, revision, date, lenses/passes covered.
2. **WHAT PASSED, explicit, per lens** (R6) — a negative result is a finding.
3. **Findings by severity** 🔴/🟠/🟡/⚪, each with: **where · rule violated by name ·
   measurable/audible consequence · minimal fix · wire/component/redesign**.
4. **Order of attack** — one variable at a time, with the acceptance criterion written
   **before** the fix.
5. **What was NOT verified** and why (R8).

It is filled with **numbers**. `[x] ok` is not a result.

> *Diagnosed-and-not-instrumented is the state in which a defect lives longest —
> everyone knows, no one counts it* (§11g-sexies). **A finding that reappears becomes a
> new lens/pass/gate in the SAME commit, never a note.** It is the bridge to stage 9.

---

## Run 3×

The A+B+C set runs **three times**, because the first pass changes the board (fixes
findings) and the changed board is a new artifact to review — **any edit invalidates
the previous inspection** (`design-review`). The set runs **at least three times** and
**does not stop before one whole pass comes out with no new findings** — if the third
still finds something, it continues. The three are the floor; stability (one clean
pass) is the stopping criterion.

Delegates to: `/aipcb` (schematic/ERC), `kicad_layout`, `caixa`, `revisao-terra`,
`design-review`, and `kicad/revisao_externa.py` (kicad-happy, MIT, tested on ~5800
projects) as a **second external opinion** — the third-party tool that can say no
about what ours approved.

---

## Risk certification — the exit of the triple review

The ledger lists what was **found**; the certification decides what **SHIPS**. It is
the exit gate from stage 7 to 8: `para_fabrica` only runs with the record at **GO**.
Modeled on `ifa10000` — the project's canonical certification protocol — from which
comes the sentence that governs everything here:

> **A waiver is a MEASUREMENT, not an opinion.**

This section is the instance of that protocol for boards.

### Certify against the signal, not against the noise

Before giving a verdict on any item, **clean the environment artifacts**. A DRC
returning dozens of *"library not configured"* comes from a missing `fp-lib-table` —
it is environment, not board; resolve it, run again, and only then read the **real
residue**. It is step 0 (`medicao-limpa`) applied to the review: you certify against a
**clean signal**.

> **Corollary: a warning you have not yet explained is NOT automatically noise.**
> "Noise" is a measured verdict — not the lazy assumption of whoever wants to close.

### THE RISK REGISTER — one line per shipped item

Every shipped warning and every waiver enters the register, **one per line**:

| field | content |
|---|---|
| **id** | stable identifier of the item |
| **item** | the warning/waiver, in one sentence |
| **lens/step** | where it appeared (A/B/C, or the stage) |
| **severity** | 🔴/🟠/🟡/⚪ |
| **MEASURED evidence** | the number/measurement that backs the verdict — never "looks fine" |
| **verdict** | `SAFE-EMBARK` (with proof) · `MUST-FIX` · `dB-decision` |
| **action** | the fix, or the routing to ⟨dB⟩ |

**GO/NO-GO:**

- **GO** requires **zero open MUST-FIX**.
- **SAFE-EMBARK** only holds with **measured evidence** — a waiver with no number is
  **MUST-FIX by omission**.
- **dB-decision** goes up to the ⟨dB⟩ human gate; it does not self-approve.

### Anchor — the C16↔J14 case (why this register exists)

A courtyard-overlap between **C16** (0805) and **J14** (box-header DIN-41651) looked
like a **reasonable waiver** — courtyards are nominal clearance, they overlap without
drama. The opinion said *waive*. On measuring **body against body** instead of
courtyard, the 0805 was **physically inside** the shroud of the flush-seating header:
a real collision, not clearance. The measurement inverted the opinion — **MUST-FIX**,
not SAFE-EMBARK.

> The gulf between what the opinion grants and what the measurement proves **is the
> whole reason for this register.**

And that is why courtyards and new placements are measured by **body**, not by nominal
courtyard (lens B).
