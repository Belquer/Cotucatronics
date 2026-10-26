# Congestion forecast — the Stage 3.5 method

Reference for **Stage 3.5** of the `/aipcb` pipeline (pre-layout, between 3
placement and 4 vias/copper). Subordinate to the gate philosophy of SKILL.md —
**a gate that warns and lets you pass is not a gate**, **measure at the right spot**,
**a negative result is a finding**, **prove that the guard knows how to say no**.

> **The sentence that governs this stage:** congestion discovered while routing is a
> failure of **architecture** caught too late. The router does not solve it — it only
> reveals it when redoing the placement already costs. That is why density is measured
> **before laying down copper**, over the placement and the netlist, and a number can
> say *no*.

## Index

1. [The five metrics](#1-the-five-metrics) — what is measured and each one's formula
2. [The two hard refusals](#2-the-two-hard-refusals) — the `if`s that stop the queue
3. [The GO / REVISE-PLACEMENT gate](#3-the-go--revise-placement-gate)
4. [The J14/C16 and J1/C5 anchors](#4-the-j14c16-and-j1c5-anchors)
5. [The tool `scripts/congestao.py`](#5-the-tool-scriptscongestaopy)

---

## 1. The five metrics

All measured from the **placement + netlist** (footprints and nets), **without a
single trace**. Project numbers loaded, not re-derived (see `quality-standards.md`
§2): default trace 0.25 mm, clearance 0.20 mm → **free-air channel pitch 0.45 mm**;
IDC pitch 2.54 mm, pad ⌀1.70 → **inter-pad clearance 0.84 mm** (exactly **1** trace
fits at the board rules). B.Cu is a **solid** ground plane on both boards, so **every
non-ground signal escapes through the single signal layer F.Cu** — a signal pushed to
B.Cu **cuts the plane** (that is metric 5).

### 1.1 Escape-channel budget per connector — the metric that predicts J14/C16

For a **≥2-row** connector on a board with **1 signal layer**, each broadside (long
side) passes ~`ncol` escape channels (one trace per pin column, at the inter-pad
clearance of 0.84 mm). A broadside is **dead** if it has less than **3 mm** of open
board beyond the courtyard into which to fan out.

```
DEMAND  S       = nr. of distinct non-plane nets that MUST leave the footprint
                  (GND/AGND go to the plane → do not count)
SUPPLY          = (live broadsides) × ncol         ncol = pin columns
verdict         = RED    if  S > SUPPLY
                  AMBER  if  S > 0.8 × SUPPLY
                  GREEN  otherwise
```

Pushing a two-row connector against an edge kills one of the two broadsides and
**halves the supply** — it is the root of J14 and J1.

### 1.2 Pin density per cell

A **5 mm** grid; counts pads per cell. The peak locates the pin field that feeds
metric 1's over-allocation (on a 2×13, five adjacent cells at the grid maximum = the
IDC's 26 pads).

### 1.3 Ratsnest crossing density

Minimum spanning tree (MST/Prim) per signal net, sampled in 5 mm cells: the airwire
length each cell accumulates. Peaks = where many signals have to cross before copper
exists — the bottleneck the router will inherit.

### 1.4 Copper demand vs supply per region

Per 5 mm cell:

```
DEMAND (mm)   = Σ (airwire length in the cell × net class width)
                widths: power 1.0+clr · rail "+…" 0.5+clr · signal 0.25+clr
SUPPLY (mm)   = free F.Cu area in the cell / channel pitch (0.45 mm)
                free area = 5×5 − pad area in the cell
ratio         = DEMAND / SUPPLY      RED >1 · AMBER >0.6 · ok otherwise
```

### 1.5 Plane fragmentation in the placement

Length of **signal trace on B.Cu** per 5 mm cell — each millimeter of signal on B.Cu
is a cut in the ground plane. High = return forced to go around slots (it links to the
Stage 5 ground review — `revisao-terra` P5/P6, the return that goes around the moat).
On a board with no copper it is still zero; the metric serves 3.5 when there is
already a routing sketch, and serves Stage 5 afterwards.

---

## 2. The two hard refusals

Proven in session ppc/002 rev C. They are written as a **refusal**, not as a warning:

- **(a) refuse a ≥2-row connector where DEMAND > SUPPLY** of escape channels (metric
  1.1). It is what caught J14 and J1 from the placement alone.
- **(b) refuse a courtyard that overlaps another courtyard, the board edge (negative
  clearance), or another component's body (F.Fab/B.Fab).** It is measured by
  **body**, not by nominal courtyard — the C16↔J14 lesson (`review-protocol.md`): a
  courtyard overlap seemed dispensable, but body-against-body was a real collision.
  *A waiver is a measurement, not an opinion.*

`scripts/congestao.py` makes the exit ≠0 fire on: **(a)** any red escape connector;
**(b)** a courtyard beyond the edge, or a courtyard over another same-side component's
body. (Pure courtyard∩courtyard is the `courtyards_overlap` that the Stage 7 DRC
already has; here the pre-layout hard refusal focuses on the body, which is what the
nominal-courtyard DRC does **not** see.)

---

## 3. The GO / REVISE-PLACEMENT gate

Two exits, never "advance with a warning":

- **GO** — no red region. Proceed to Stage 4 (vias+traces).
- **REVISE-PLACEMENT** — there is red. The region goes back to **Stage 3** (re-place),
  **or to 1/2** if what bottlenecks is the **connector's own pinout** (the row or the
  pin order that no re-placement saves — it is a schematic decision). **You never
  advance to Stage 4 with an open red** — routing over a red placement is spending
  time confirming the inevitable.

**Artifact:** the **congestion map** goes to the **risk register** (one region per
line, with demand/supply measured), linked to the **Risk Certification Gate** (exit of
Stage 7, see `review-protocol.md`). 3.5 is the **pre-layout** form of **IFÁ10000** —
the project's canonical certification protocol: it certifies the placement against
**measurement**, not against opinion.

---

## 4. The J14/C16 and J1/C5 anchors

**J14/C16 (brain board) — congestion caught too late.** J14 is a board-to-board IDC
2×13 pushed against the top edge: top broadside **dead** (gap 2.80 mm < 3 mm), 17
signals to escape through 13 columns on a single side → **demand 17 > supply 13,
RED**. The inner corridor (the only live one) was already taken by the pre-amp
front-end (D11/R11/J5), so the decoupling cap **C16 (100n +3V3A) had no home next to
its rail and ended up INSIDE J14's body** — the risk register's MUST-FIX C16↔J14. The
nudge of J14 inward was **physically blocked by the pre-amp**; the brain board
resolved it by taking **C16 to B.Cu + FreeRouting**, not by relaxing the escape
budget. That is why `congestao.py` still marks J14 red in the closed rev C: the
over-commit is real; it was worked around by routing, not undone in the placement.

**J1/C5 (panel board) — latent, resolved by the nudge.** J1 is the **same** IDC 2×13,
the same class of failure, and its +3V3A cap **C5** was ~0.5 mm from the body — one
nudge away from being the next C16. Here re-placement was possible: the **real 5.5 mm
nudge** of J1 inward opened the top broadside (gap became 3.30 mm ≥ 3), supply 13→26,
**17 < 26 GREEN**, and C5 got a home. `congestao.py` confirms: the panel board passes
the 3.5 gate in rev C. The same defect, the two outcomes — one worked around by copper
because the placement was stuck, the other undone in the placement because there was room.

---

## 5. The tool `scripts/congestao.py`

Read-only (loads the board, measures, **never writes**). A *starter*: it implements
metrics 1.1 and 1.2 and the two hard refusals; metrics 1.3–1.5 are described here and
in the source scripts for whoever wants to harden them.

```
congestao.py <placa.kicad_pcb>   measures the placement; exits !=0 on RED
congestao.py --sabota            proves it knows how to say NO (no board, no KiCad)
```

The numeric engine operates over a pure model (dicts), so `--sabota` runs without a
board: it plants an over-allocated connector, a courtyard off the edge, and a
courtyard inside another's body, and requires that **all three** block — the guard has
teeth (project convention: `funcao_dos_pads.py --sabota`, `manifesto_fontes.py
--sabota`). Declared limitation: it measures courtyard/body by **bounding box**, not
by polygon — it catches the coarse body-in-body (C16↔J14); refining to a polygon is
hardening work.
