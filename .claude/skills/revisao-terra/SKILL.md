---
name: revisao-terra
description: Grounding, ground and decoupling review in board design — classifies each ground conductor, checks star against daisy chain, AGND/DGND, capacitors by load type, air coupling and high-impedance inputs. Use when designing or reviewing hardware, when building a new circuit, and whenever there is hum, squeal, click or noise that is not explained in the firmware.
---

# Ground review — the sweep

Formulated by **closed quantification**: each ground conductor has a finite set of
legal states, and **everything that is not in a legal state is a finding, by
definition**. A ground defect does not escape by not being recognized — it escapes
because there is a place where the rule did not look.

Ground noise is expensive because it **imitates everything else**: it looks like ADC
noise, looks like a firmware defect, looks like a burned component. A systematic sweep
costs an hour; diagnosis by hypothesis cost an afternoon and three innocents.

---

## Iron rules OF THE REVIEWER THEMSELVES

These come before the passes, because the reviewer errs first.

**R1 — A measurement without load is worthless.** At rest there is no current, and
without current there is no voltage drop. **It gives zero on top of a defect.** Every
ground measurement declares under what load condition it was made, or it is not a
measurement.

**R2 — One variable at a time.** Changing two things and hearing an improvement does
not say which one acted — and one of them may be making things worse. This includes
not doing a reset/power cycle between tests without declaring it: the recovery may
come from the reset and be attributed to the change.

**R3 — Record all the numbers BEFORE touching anything.** Without the "before" there
is no way to prove that the "after" improved. Half the ground arguments die here.

**R4 — A hypothesis without a test that would knock it down is narrative, not
diagnosis.** For each suspicion, write **which observation would kill it**. If there
is none, the suspicion does not enter the report.

**R5 — Read your own code before accusing the copper.** A badly placed guard, a
filter with the wrong constant, hysteresis measured from the center instead of the
boundary — all of this **imitates noise perfectly**. Before stating "the ADC is
noisy", **measure the noise** and compare it with the software threshold.

**R6 — A negative result is a finding.** What passed is written down, or the next
review redoes the work.

**R7 — Each finding comes out with severity and with an owner.** 🔴 destroys a part or
loses signal · 🟠 audible/visible · 🟡 latent · ⚪ hygiene. And: is it fixed with a
wire, with a component, or with the design?

**R8 — Declare what was not verified** and why. Never omit.

---

## The passes

Each pass answers ONE question. None is optional; a skipped pass is **declared**, not
silenced.

### P0 — Inventory and classification

**List ALL the ground conductors in the design.** Each one is exactly one of these
three:

| type | what it is | current | legal destination |
|---|---|---|---|
| **power** | the return of the consumed current | high, pulsed | its own wire to the star |
| **logic reference** | the zero of the ADC and the digital pins | ~none | stays in the device that reads |
| **audio reference** | the zero that travels with the signal | ~none | `AGND` island |

**An unclassifiable conductor = a finding.** A conductor at a destination that is not
the legal one for its type = a finding.

⚠️ **The most common error of this pass** is taking a reference to the star. "The star
is good, so I take everything to the star" — but a reference taken to the star
**injects** the star↔reader voltage difference into the measurement. The star is for
**current**.

The deciding question: *does this conductor carry current, or define a zero?*

### P1 — Star against daisy chain

- **Where is the star point?** If you cannot point to a physical point, there is no
  star — finding 🔴.
- For **each** power consumer: does it have its own wire all the way there, or does it
  arrive through another? Arriving through another = a finding.
- **Is the largest consumer's return the shortest and thickest wire?** If not, a finding.
- **Is there a switch, relay or connector in the return path?** A finding — variable
  contact resistance in everyone's path.

### P2 — Reference islands are leaves

Every reference (logic or audio) must **enter a device and not leave for anywhere
else**.

- Trace each island: where does it end? If it touches two devices, it is a finding.
- **Physical check:** with the board out of the socket, continuity between the island
  and the power rail. **If it beeps, there is a connection too many.**
- Does the **reference positive** (the pots' 3V3, etc.) come from the same board that
  does the measurement? Powering from one source and referencing on another = a finding.

### P3 — `AGND` and `DGND`

Boards with a codec join the two **internally, at a single point**, on purpose.

- **Are they joined anywhere off the board?** Finding 🔴 — two meeting points form a
  loop, and the current splits between the two paths; the fraction that chooses the
  inner one passes through the codec's reference.
- **Does the power enter through `DGND`?** Entering through `AGND` is a finding 🔴 —
  the whole board's return crosses the narrow bridge.
- **Does the audio return (jacks, the amp input, the analog inputs) come from
  `AGND`?** Coming from the general rail is a finding 🟠.

### P3b — Signal between separately powered boards

For **each pair of boards connected by signal** (UART, SPI, I2C, GPIO):

> *Is there a power state in which one is alive and the other is not?*

On the bench there almost always is — one on USB, the other on the rail. If so:

- 🔴 **Is there a DIRECT ground loop next to the signal wires?** Not having one = a
  finding that **kills the live board**, not the dead one.
- Without it, the powered board injects current through the unpowered one's protection
  diodes; the line becomes undefined, the peripheral goes into error, and whoever
  tries to **transmit** gets stuck.

⚠️ **The defect appears on the POWERED side** — you look where it manifests, and it
lives on the other side of the cable.

⚠️ **Do NOT remove the signal ground to solve noise.** It is trading a hum for a dead
board. Power noise is solved with **local decoupling** (§P4), which kills the current
where it is born; the signal ground is not negotiable.

*(This pass was born from a wrong piece of advice, given on 31/08 by applying the §P0
rule — "a reference does not go to the star" — to a case it does not cover. It cost a
board that would not boot and an afternoon hunting innocent firmware.)*

### P4 — Decoupling by LOAD TYPE

The question *"does it need a capacitor?"* **has no single answer for the system**.
Classify each consumer:

| load | needs | common error |
|---|---|---|
| audible pulsed current (class-D amp) | a large bank **+ 100 nF** at the pins | missing bank → cross-hum |
| **constant power** (switching converter) | **nothing large at the input** | a bank → **squeal** |
| radio bursts (BLE/WiFi) | local **100 µF + 100 nF** | nothing → the radio comes out of the speaker |
| logic (MCU) | local 100 µF + 100 nF | nothing → diffuse noise |

- **Does every consumer have the PAIR?** The large one alone lets the edges through;
  the small one alone has no energy. One alone = a finding.
- **Is it glued to the pins?** More than a few centimeters away, it connects through
  the very inductance it exists to bypass — a finding 🟠.
- **Voltage ≥1.5–2× the rail?** Right polarity?
- 🔴 **A large low-ESR bank at the input of a switching converter** = negative input
  impedance against the wiring inductance → it oscillates (Middlebrook). A finding
  that **destroys the sound immediately**.
- ⚠️ **At a step-down's output, 100 µF — not 2200.** Too much capacitance also affects
  the loop's stability.
- **Inrush:** a large bank can make a PD source read a short and re-negotiate. If the
  startup is intermittent, this is it — declare it, do not hunt a new defect.

### P5 — High-impedance inputs

A high-impedance source (piezo, pickup, crystal) **is an antenna**, and what it picks
up passes through **the entire** downstream gain chain.

- Return on the `AGND` island?
- Do the signal and return **travel together** (side by side or twisted)? Separate
  paths = loop area = a finding.
- Shielded cable: **shield connected at one end only?** At both ends it becomes a loop
  and conducts — a finding.
- ⚠️ **Is there a gate/expander covering input noise?** A disguised finding: the
  threshold that silences the noise also eats the instrument's tails. If the threshold
  had to be raised, the defect is at the input.

### P6 — Coupling through the air

Not everything is copper, and what couples through the air **is immune to a ground
scheme**.

- **A class-D amp output** = a square wave at hundreds of kHz with high current. Input
  cables parallel to it = a finding 🟠. Crossings **at a right angle**.
- **Radio** (BLE/WiFi): it has **two faces**, and they look like two defects —
  **connected**, bursts at the connection interval = **continuous hum**;
  **disconnected**, advertising bursts (~100–300 ms) = **a click at ~3 Hz**. Same
  mechanism, two rhythms. Do not treat them as separate defects.
- Is the cable routing declared in the design, or "we'll see at assembly"? The second
  is a ⚪ finding that becomes 🟠 at the next assembly.

### P7 — Measurement plan

The review ends with **how it is proven**, not with opinion.

- The star point as reference, multimeter on **AC/mV**, **under load**.
- One number per consumer, **recorded before any change** (R3).
- Acceptance criterion **written before the fix**: what number, at what point, means
  resolved.

| reading | means |
|---|---|
| 0 mV | it is in fact at the star |
| tens of mV | it arrives there through something else — its own wire is missing |
| rose after the change | the change made it worse; undo it |

⚠️ **Look for a free ground meter.** An audible periodic click that follows an LED, a
relay, a screen refresh — that is coupling denouncing itself. An LED draws milliamps:
**if it is audible, the coupling is large**, and everything else the board does passes
through the same path without a pattern that gives it away.

**Silencing it in firmware hides the defect.** Once the ground is tidied it disappears
on its own — and that is the test that it was done right.

---

## Report format

1. **What was reviewed** — design, revision, date. Passes covered.
2. **WHAT PASSED**, explicit, per pass (R6).
3. **Findings by severity**, each with: where · rule violated **by name** ·
   audible/measurable consequence · minimum fix · wire, component or design.
4. **Order of attack** — one variable at a time (R2), with each step's acceptance
   criterion written **beforehand**.
5. **What was NOT verified** and why (R8).

---

## Anti-decay

A new defect family discovered in a review **enters as a pass or item in this file** —
proposed, never in silence. The review that does not update itself reviews yesterday's
project.
