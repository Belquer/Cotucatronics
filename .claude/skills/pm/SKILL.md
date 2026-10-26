---
name: pm
description: Align deadlines and equate processes across Cotucatronics' artifacts — enclosure, board and firmware. What the critical path is, what can be done in parallel, and the steps the two processes have in common under different names. Use when deciding the ordering sequence, when estimating when something will be assemblable, and before sending anything to the factory.
---

# `/pm` — deadlines and equivalences

This project has **three artifacts with different clocks**: the enclosure, the board
and the firmware. Each has its own gate, its own factory and its own deadline — and
the instrument only exists when the three arrive at the same place.

## 1. The question is the CRITICAL PATH, not the task list

> **What decides the date is not what is left to do: it is what is left to do that no
> one can speed up afterwards.** A week of factory time does not compress with effort;
> an afternoon of routing does.

| artifact | factory | typical lead time | compressible? |
|---|---|---|---|
| enclosure | Ponoko | ~3 days of production + shipping | **no** |
| board | JLCPCB | ~2–5 days + shipping | **no** |
| assembly | bench | hours | yes |
| firmware | bench | hours | yes, and it is done afterwards |

**Operational corollary:** what goes to the factory is ordered **first**, and
everything that can be done while it is there is done **after** the order, not before.
Postponing an order to "finish something that does not affect the file" is spending
calendar days on work that fit inside the wait.

⚠️ **But the reverse kills:** ordering before the gate passes trades days for a wrong
part, and the wrong part costs the ENTIRE lead time again. The order is **gate →
order → rest**, never **order → gate**.

## 2. What needs to be decided before each order

It is not "being ready" — it is **being decided on what the file carries**.

**Enclosure (Ponoko):** material and thickness · holes and diameters of everything
that crosses it · what is removable · nesting within the sheet. It **does not need**
to know that what goes inside is routed.

**Board (JLCPCB):** frozen schematic · footprints · outline and mechanical holes ·
**routing** · mask and silkscreen color. It **does not need** the firmware.

**Firmware:** it needs the board in hand. It is always the last, and it is the only
one that is corrected afterwards without a factory cost.

## 3. The equivalences — the same steps under different names

Recognizing this saves reinventing the process for each artifact, and makes a scar
from one serve the other.

| step | enclosure | board |
|---|---|---|
| the source | `cotuca_caixa_corte.py` | `gera_*.py` + `pcb_contorno.py` |
| "connects?" | — | **ERC** |
| "doesn't overlap?" | **nesting** | **DRC geometry** |
| "is it all there?" | part count | schematic↔PCB **parity** |
| "fits the other?" | `confere_caixa` | `confere_arcade`, `confere_furos` |
| external proof | `tools/nesting.py` | KiCad's DRC, JLC's viewer |
| identity | rev + digest in the name | rev + digest in the name |
| the veto | render, dB's eye | render, dB's eye |
| "one first" | cut ONE and assemble | **one board, not the batch** |

> **The "one first" rule is the same for both, and for the same reason:** no gate
> measures real tolerance, and the part in hand rules over everything.

## 4. When the two cross

⚠️ **The dependency is NOT one-way** — I wrote it that way and was wrong, on the same
day. It is in opposite directions for different things:

| | who rules |
|---|---|
| outline, fastening holes, shafts that cross through | **enclosure → board** |
| **volume** (what comes down from the lid over the board) | **enclosure → board** |
| **wall openings** (USB, access) | **board → enclosure** |

Only the board knows where the connectors ended up, and the wall has to follow them.
So **there are two passes, and there is no way around it**: the board is placed
against the enclosure's envelope, the board **emits** the openings it needs, the
enclosure consumes them, and the board is run again to confirm that the access gate
turns green. If the enclosure's dimensions do not change between passes, it converges
on the second.

In Cotucatronics this is literal: `pcb_contorno.py` writes `caixa/aberturas.json` and
`cotuca_caixa_corte.py` reads it. Writing "Daisy USB at x=95.6" on the wall by hand
was transcribing from one file to another — the error family this project has already
paid for three times.

What remains true: **changing the enclosure after the board is routed** costs redoing
paths, because the mechanical positions change and the traces follow them. That is why
the two passes are done **before** routing, never after.

**Who yields to whom (IFÁ10000-M, since 18/09):** the board brings its own **fastening
holes**; with that, it is the **enclosure that yields to the board** — interior
outline, holes, shafts that exit through it, posts — because adjusting packaging is
much cheaper than redoing a PCB. The board only yields with a justification signed by
dB. For the critical path this means: **the enclosure can regenerate after the board,
the board cannot after the enclosure**. And **the air column** (what comes down from
the lid over the board) still has no tool that sees it — that is M6.

## 4b. What automation changed in the critical path

Before, "board ready to order" was a milestone of unknown duration: it depended on
someone sitting down to route, and the estimate was an opinion. Now it is **a command
of about half an hour** (`faz_placa.py todas`) that erases the copper, re-places,
stitches, routes in the three orders of attack, chooses the best by measuring, tidies
the silkscreen and passes through the factory gate.

That changes two things in planning:

1. **Redoing stopped being expensive.** A placement change no longer costs the whole
   routing again in person-hours — it costs half an hour of machine time. So decisions
   that used to be postponed out of fear of rework (moving a part, touching the
   enclosure) can now be taken early, which is when they are worth most.
2. **What is left is what the machine does not close.** The board's critical path has
   become exactly that list — and it is a short, named list, not a cloud. Estimating
   has become counting connections, not guessing.

⚠️ **And the gate is not a formality at the end: it is the milestone.** "Ready to
order" has an executable definition — zero geometry, whole planes, zero connections
left to make — and no meeting can declare it by any other means.

## 5. Estimating without inventing

> **A lead time given by a factory is a given; a lead time for my own work is an
> estimate.** Stating both with the same confidence is like stating a measured number
> and a deduced one without distinguishing them (`medicao-limpa` §3).

When giving a date, always separate:

```
Ponoko: delivery estimated 15/09      ← theirs, with a name
routing: 1 session                    ← my estimate, may double
JLCPCB: 2-5 days + shipping           ← theirs, published range
```

And when a deadline slips, say **which of the three** slipped. A factory delay and a
delay of mine call for different responses: the first is absorbed with parallelism,
the second with scope cutting.

## Provenance

It was born on 06/09/2026, when the enclosure was ordered with delivery on 15/09 and
it became clear that the board had to be at JLCPCB **on that day or before** for the
two to arrive together — and that everything left on the board (routing, ground
review) fit inside the wait for the enclosure, as long as the order went out first.
