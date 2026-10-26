# GROUNDING — the chapter

**v0.2.0** · written on 31/08/2026, after an afternoon chasing three noises
that had three different causes and one similar symptom.

> This is the document that **owns** the subject. `HARDWARE.md` describes *this*
> instrument; this one describes **the rule**. When the two disagree, this
> one wins, and the other is the one to be corrected.

Ground review during board layout has its own skill:
`.claude/skills/revisao-terra/`.

---

## 1 · The single principle

Almost everything in this chapter comes out of one equation:

```
V = L · di/dt
```

**A wire has inductance.** Current changing fast, as it runs through a wire, creates
a voltage along it. That voltage does not stay in the wire: it is seen by
**everything hanging off it**.

From here is born the concept that decides everything else:

> ⚠️ **SHARED IMPEDANCE.** When the return current of one device
> runs through a stretch of copper that **another device uses as a reference**,
> the voltage drop across that stretch becomes **signal** for the second.

And from here is born the inversion that is hard to internalize:

> **Ground is not a place. It is a path.**
>
> Connecting two points to "ground" says nothing. The useful question is always
> **where does the current return**, and **who is sitting on that path**.

---

## 2 · Taxonomy — three kinds of ground

**Every ground conductor in the project is exactly one of these three.** One that cannot
be classified is a defect, not an exception.

| kind | what it is | current | where it goes |
|---|---|---|---|
| **power** | return of the current the device draws | high, pulsed | **its own wire to the star** |
| **logic reference** | the zero the ADC and digital pins measure against | ~none | **stays at the device that reads** |
| **audio reference** | the zero that travels along with the signal | ~none | **`AGND` island** |

⚠️ **The star rule applies only to the first kind.** The other two are not
return paths — they are **references**, and a reference wants to be as
close as possible to whoever reads it, not as close to the star point.

Confusing the two is the most common error, and it is seductive: "star is good, so
I take everything to the star". Taking a pot's reference to the star **injects into it**
the voltage difference between the star and the MCU that measures it.

**How to decide, in one question:** *does this conductor carry current, or does it define
a zero?* Carries → star. Defines → stays where it is read.

---

## 3 · The star

**One physical point** where the power returns arrive, each one by its own
wire. Not a rail that everyone passes through.

```
   DAISY-CHAIN (wrong)            STAR (right)
   ┌───┐  ┌───┐  ┌───┐              ┌───┐  ┌───┐  ┌───┐
   │ A │──│ B │──│ C │              │ A │  │ B │  │ C │
   └───┘  └───┘  └───┘              └─┬─┘  └─┬─┘  └─┬─┘
     └──────┴──────┴──> source          └──────┼──────┘
                                              ▼
   C's current crosses                 each one returns on its own;
   the stretch where B references      no one steps on anyone else's path
```

**Where the star goes:** at the point where the supply negative enters the
largest consumer. In this instrument, it is where the `GND` of the USB-C PD decoy reaches the
amp — and it is from there that it branches off to the step-down.

**The amp's return is the shortest and thickest wire of all.** It is the one that
carries the pulsed current.

🚫 **Ground never passes through a switch.** Switches only on the positive. A switch on the
return adds variable contact resistance to everyone's path.

---

## 4 · The decoupling capacitors

### What they do

They do not filter. They are a **local reservoir of energy**.

When a consumer asks for a burst of current, it comes from the capacitor
glued to the pins — a two-centimeter loop — instead of coming from the supply, over
thirty centimeters of wire with inductance. **The fast current never gets to
run through the long wire**, and so it does not create a voltage in it.

### Why there are always TWO

They are not redundant. They cover **different bands**:

| | band | why |
|---|---|---|
| **large electrolytic** | slow to medium (µs–ms) | much energy, but slow: it has its own ESR and ESL |
| **100 nF ceramic** | fast (ns–µs) | little energy, but responds to the edges the large one does not even see |

Only the large one lets the edges through. Only the small one has no energy for the
transient. **Always the pair, in parallel, together.**

### The value depends on the KIND OF LOAD, not the rail

The question *"does it need a capacitor?"* does not have one answer for the whole
system:

| load | example | needs | what happens if you get it wrong |
|---|---|---|---|
| **audible pulsed current** | class-D amp | **2200 µF + 100 nF** at the pins | hum when another consumer joins the rail |
| **constant power** | switching converter | **nothing large at the input** | **squeal** — it oscillates against the inductance |
| **radio bursts** | ESP32, any BLE/WiFi | **100 µF + 100 nF** local | hum and click — the radio comes out of the loudspeaker |
| **logic** | MCU | **100 µF + 100 nF** local | diffuse noise, hard to attribute |

### 🚫 The prohibition — a large bank at the input of a switching converter

A switching converter is a **constant-power load**: if the input voltage
drops, it draws **more** current. That is a **negative input impedance**.

Add a large low-ESR bank and the wiring inductance to it, and you have a
resonant circuit with negative damping. **It oscillates** — it is Middlebrook's
criterion.

Measured on 31/08: 2200 µF at the step-down input produced a **sharp squeal**
immediately, and removing it cured it in the same gesture. The modules already carry what
they need at the input; do not add load to them.

If one day it is needed there: a small value **with** ESR, or a damping RC.
Never 2200 µF of low ESR.

⚠️ **The same caution holds for the OUTPUT.** Whatever hangs off the 5 V of a
step-down takes **100 µF**, not 2200 — too much capacitance on the output also affects
the converter's loop stability. For radio bursts what matters is not
being large: it is being **close**.

### The physical rules

**📍 Glued to the pins.** A capacitor at 10 cm connects through 10 cm of
inductance — which is exactly the thing it exists to get around. Far away, it is
as if it were not there.

**Polarity.** An electrolytic has a stripe on the body = negative = `GND`. Backwards,
it heats up and bursts. A ceramic has no polarity.

**Voltage.** At least 1,5–2× the rail. 15 V → use 25 V or 35 V. 5 V → 16 V.

⚠️ **Inrush.** 2200 µF at startup is a considerable peak, and some PD
adapters read that as a short and re-negotiate. If the amp does not start reliably
when powered on, it is this — not a new defect.

---

## 5 · `AGND` and `DGND` — the narrow bridge

> ⚠️ **CORRECTED ON 04/09, AND THE PREVIOUS VERSION OF THIS SECTION WAS WRONG.**
> It claimed that the Daisy joins `AGND` and `DGND` internally. **It does not.**
> The libDaisy documentation says you should **always connect the `DGND` and
> `AGND` pins outside the Daisy**, and the Seed exposes them on two distinct header
> pins — 20 and 40, checked in the manufacturer's `Daisy_Seed_Rev4_Pinout.csv`.
>
> The error was asserted with confidence and repeated, and sent the white-noise
> analysis in the wrong direction. See `battle_scars.md`.

**The bridge must exist, and it is ours to make.** The general rule for
converters (Analog Devices) says where: the optimal `DGND` connection is to `AGND`
**at the package itself**, at the same potential, **and nowhere else** —
joining them there minimizes the potential difference between the two grounds of the
converter, which is what injects digital noise into the analog.

🚫 **What stays forbidden is it being MORE THAN ONE, and being a rail.** Two
meeting points form a ring, and the current divides across the two paths
— the fraction that chooses the inner one passes **exactly** through the audio
reference.

```
   WRONG — two meetings, and one of them shares current
        ┌──── short wire at pins 20/40 (the bridge we want) ────┐
   AGND ┤                                                       ├ DGND
        └──────── permaproto rail (the EXTRA connection) ───────┘

   RIGHT — one meeting only, and on a short wire
   AGND ──── pin 20 ──[ NetTie-2 ]── pin 40 ──── DGND
```

> **The P3 ring was not the bridge existing: it was the bridge being a rail shared with
> current, instead of a short wire at the pins.** And that is why the fix is
> to **MOVE** the bridge, never remove it — removing it leaves the codec without a
> reference, which is what on 31/08 "traded a hum for a dead board".

🚫 **Never pull the supply through `AGND`.** The return of the whole board
would cross the narrow bridge and ground at the codec's reference — the exact
opposite of what the pin exists to do. **Power enters through `DGND`.**

### The target drawing

```
   AUDIO ISLAND — signal only, no current
     AGND ── piezo input
          ── DPDT commons
          ── jack sleeves
          ── amp input ground
          ↑ meets DGND at ONE point: the net-tie at pins 20/40
            of the socket. NOT inside the Daisy — it does not join them.

   POWER AND LOGIC RAIL
     DGND ── its own wire to the star
          ── pots (with the 3V3 coming from the same board that reads them)
          ── encoders, buttons
          ── ESP32 bridge (or its own wire to the star)
```

⚠️ **The pots' 3V3 comes from the same board that measures them.** The pair travels
together: the top reference and the bottom reference both come from whoever does the
measurement. Powering the pots from one source and referencing them to another injects
the difference between the two directly into the reading.

**The number that shows the cost:** **42 mV** was measured between the Daisy's ground and the
star before the dedicated wire. Over a 3,3 V reference that is **1,3% of the
travel — about 1,6 MIDI steps**, moving on their own.

### Checking — THREE measurements, and in this order

> ⚠️ **CORRECTED ON 05/09.** The previous version of this section had you measure the
> island against the rail with the board out of the socket and said *"if it beeps, there is one
> connection too many"*. **With the net-tie drawn, beeping is CORRECT** — and the
> old test would read a correct board as defective. The live test is the one in
> [`MAPA-DE-TERRA.md`](esquematicos/revB/MAPA-DE-TERRA.md) §6, reproduced
> here because this is the document that owns the subject.

**1 · The premise, on the bare Daisy.** Continuity between pin 20 (`AGND`) and
pin 40 (`GND`), with the board out of everything.

| result | means |
|---|---|
| **open** | ✅ **measured on 04/09.** The bridge is ours to make |
| **beeps** | 🔴 it would be a different revision. Stop and rethink |

**2 · Is the bridge unique?** With no power, **lift one leg of the net-tie**
(or do not populate it yet). Probe A on the `AGND` island (the sleeve of a
piezo jack), probe B on the `GND` plane (the amp module's ground).

| result | verdict |
|---|---|
| **opens** | ✅ the bridge is unique — it only exists through the net-tie |
| **keeps beeping** | 🔴 there is a second bridge. Do not assemble. Find and cut |

**3 · Does the bridge exist?** Net-tie restored, still no power, same probes.

| result | verdict |
|---|---|
| **beeps** | ✅ the bridge exists and passes through the net-tie, as drawn |
| **opens** | 🔴 the island is floating — the codec without a reference |

⚠️ **All three, and in order.** Step 2 alone does not distinguish *"unique bridge"*
from *"no bridge"*, and one of them kills the audio. **A test that does not separate the
two outcomes is not a test.**

### ⚠️ Separating the power supplies makes it WORSE — measured on 01/09

Obvious temptation: if the shared rail couples, give each one its own supply.
**Three flights, and the trend is monotonic in the wrong direction.** Noise in 8–16 kHz:

| topology | noise |
|---|---|
| **single rail** | **+9,6 dB** ← best |
| Daisy on the wall | +17,3 dB |
| two powerbanks | +24,3 dB |

The mechanism: with the supplies separated, **the audio cable becomes the only
ground link between the amp and the Daisy**. All the return current that before had
the rail now travels along the conductor that carries the signal — which is the definition of
shared impedance, now concentrated at the worst possible spot.

**Verdict: single rail.** And the answer to the noise that remains **is not separation
nor a software filter — it is gain chaining**: coming in hot into the output
dynamics (`pre_level_ab` at 1.0) and trimming after the limiter (`post_level_l/r`, on the
pots). The architecture already does it.

### What this does not solve, and it is honest to say

Single-ended-input amps connect the input ground to the power ground **inside
the module**. The island ends up tied to the star through there, and the ring closes on the
outside. **There is no way to avoid it** without a differential input.

The real gain is another: **the audio return stops sharing copper with the
supply return.** It is the shared impedance that makes noise become signal,
not the ring itself.

---

## 5b · Signal between two boards: the ground travels ALONGSIDE the wire

🔴 **This section was born from a wrong piece of advice of mine, on 31/08.** I said:

> *"Don't add a ground wire next to the UART wires. That would create a
> second path between the bridge and the Daisy, in parallel with the one to the star."*

The reasoning was §2's — signal ground is a **reference**, and a reference does not go
to the star. But I forgot what makes this case different, and the cost was
**the Daisy no longer starting** and an afternoon chasing a firmware defect that
did not exist.

### The rule

> **Two boards that exchange signals and can be powered SEPARATELY
> need a DIRECT ground loop between them, alongside the signal wires.**

What fails without it: the Daisy comes from the laptop's USB, the bridge comes from the rail. With
the rail **off**, the bridge is left without power **and without a reference** — and
its `TX`, still connected to the Daisy's `D14`, begins injecting current through the
protection diodes of a dead chip. The line becomes undefined, the USART enters a
framing error, and whoever tries to **transmit** on it gets stuck.

⚠️ **Note that the defect appears on the POWERED side.** The live board is the one that
locks up, because of the dead one. That makes the diagnosis treacherous: you look for the
defect where it manifests, and it lives on the other side of the cable.

### ⚠️ What the test showed, and it corrects this section

I wrote above that the lack of the direct loop was **the cause** of the Daisy not starting.
**The test does not support that.** Measured on 31/08, at the end of the day:

| condition | result |
|---|---|
| bridge powered, with loop | ✅ starts, chain opens in 0,0 s |
| bridge **without power**, with direct loop re-soldered | ❌ locks up — chain takes 59 s and goes dead |
| bridge without power, ground via the Daisy **or** direct | ❌ locks up in both |

**The bridge has to be POWERED.** Ground alone is not enough: a chip with no
power hanging off the RX line keeps wrecking the USART, even with a
common reference.

The direct-loop rule **stands as good practice** — without it there is not even a
reference, and the behavior becomes undefined instead of merely bad. But it
**was not the cure**, and I presented it as if it were before testing it.

### And the loop I wanted to avoid?

It exists, and it is real — but it is the **lesser** problem, and it has its own fix:

| problem | right fix |
|---|---|
| radio current through the shared ground | **local capacitor** (100 µF + 100 nF at the pins) — kills the current where it is born |
| lack of reference between boards | **direct ground loop**, alongside the signal |

Removing the signal ground to solve noise is trading a hum for a dead
board. **Decoupling is what solves the noise; the signal ground is not
negotiable.**

### How to recognize the case

Ask of each pair of boards connected by signal:

> *Is there a power state in which one is alive and the other is not?*

If yes — and there almost always is, on the bench, with one of them on USB — **they need
the direct loop**. A guaranteed common supply is the only exception, and "common" has
to include the bench, not just the assembled piece.

## 6 · High-impedance inputs

A piezo is a source of **very high impedance** — and high impedance is an
antenna: it takes very little induced current to generate an appreciable voltage.

And what you pick up at the input is worse than what you pick up at the output: **it passes
through the entire gain chain downstream**. Input noise comes out amplified by the
drive, by the gate, by the reverb, by everything.

**Three precautions, in order of effectiveness:**

**1 · Physically keep away from the class-D amp cables.** The output of a class D
is not audio — it is a **square wave at ~400 kHz** with high current, and those
wires radiate. Running in parallel with input cables is direct injection
**through the air**, and no ground scheme corrects it. If they must cross,
**at a right angle**, never side by side.

**2 · Return on the `AGND` island**, with the signal and its return **traveling
together** — side by side or twisted. A small loop area picks up less field.

**3 · Shielded cable with the shield connected at one end only**, at `AGND`, on the side
of whoever reads. **Connected at both ends the shield becomes a ring** and starts conducting
current — which is what you wanted to avoid.

⚠️ **A gate covering up input noise is a defect in disguise.** A threshold high
enough to silence it also eats the instrument's long tails. If
the `gate_thresh` had to be raised for the sound to come out clean, the problem is in the
input, not in the gate.

---

## 7 · What couples through the air

Not everything is copper. Two sources are worth an explicit review:

| source | what it radiates | countermeasure |
|---|---|---|
| **class-D amp output** | square wave at hundreds of kHz, high current | keep away; cross at a right angle |
| **radio (BLE/WiFi)** | µs current bursts | local decoupling — kills the current where it is born |

⚠️ **The radio has two faces, and they look like two defects.** With BLE **connected**,
the bursts follow the connection interval and sound like a **continuous hum**;
**disconnected**, they follow the advertising interval (~100–300 ms) and sound like a
**click at ~3 Hz**. Same mechanism, two rhythms.

**The test that isolated it:** cutting the bridge's power entirely. The noise
stopped.

---

## 8 · How to measure

⚠️ **A ground measurement only means something UNDER LOAD.** At rest there is no
current, and with no current there is no voltage drop to measure — it gives zero over
a defect.

**The method:**

1. Get the system producing real sound (a test tone works, and frees your
   hands — holding the probes and playing at the same time does not work).
2. Multimeter on **AC**, on the millivolt scale.
3. Black probe on the **star**. Red probe on the ground of each consumer,
   one at a time.
4. **Write down every number before touching anything.**

**What the numbers mean:**

| reading | means |
|---|---|
| **0 mV** | that consumer really is on the star |
| **tens of mV** | it gets there through something else — it is missing its own wire |
| **went up after a change** | the change made it worse; undo |

**Measured in this instrument, on 31/08, with a tone playing:**

| path | before | after |
|---|---|---|
| star → amp | 0 mV | 0 mV |
| star → Daisy | **42 mV** | **0 mV** (dedicated wire) |
| star → **bridge** | — | **0 mV** (measured 01/09) |

⚠️ **And it was the measurement that dispensed with the third wire.** I had prescribed
"its own ground to the star" for the bridge too, out of symmetry with the Daisy. Measured with
a tone playing: **0 mV** — it already gets there fine. The Daisy's 42 mV were a concrete
number and justified its wire; **prescribing the same for the bridge without measuring was
generalizing from one case**. The wire was not made, and it is not needed.

> **Symmetry is not measurement.** Two boards on the same rail can have return
> paths that are very different, and the multimeter costs less than the wire.

⚠️ **An audible click is a free ground meter.** You could hear through the exciter
a click at ~3,3 Hz that followed the Daisy's diagnostic LED. An LED draws a
few milliamperes: **if that is audible, the coupling is large** — and everything
else the board does passes through the same path, with no pattern that gives it away.

Silencing it via firmware hides the defect. **Once the ground is tidied it
disappears on its own, and that is the test that it was done right.** That is what
happened.

---

## 9 · The table of prohibitions

| 🚫 | why |
|---|---|
| Large low-ESR bank at the input of a switching converter | negative input impedance → oscillates (squeal) |
| 2200 µF at the output of a step-down | affects the loop stability; 100 µF is enough |
| Power through `AGND` | the return crosses the narrow bridge and grounds at the codec |
| `AGND` and `DGND` joined off the board | second meeting point = ring through the audio reference |
| Signal reference taken to the star | injects the star↔reader difference into the measurement |
| Shielded-cable shield connected at both ends | becomes a ring and starts conducting |
| Switch on the return | variable contact resistance in everyone's path |
| Input cable in parallel with a class-D output | coupling through the air, immune to ground |
| Concluding from a measurement taken **without load** | zero over a defect |

---

## 10 · Log — the three noises of 31/08

They were all at the same time and looked like one. Separating them was half the
work.

| sound | real cause | fix |
|---|---|---|
| **sharp squeal** | 2200 µF at the step-down input oscillating | remove |
| **click at ~3 Hz** | ESP32 bridge advertising; Daisy's ground via the step-down | dedicated wire to the star · decoupling at the bridge |
| **continuous hum** | ESP32 bridge connected; bursts of the connection interval | local decoupling at the bridge |

> **And the lesson that stuck:** in this case I accused the USB hub, the capacitors, and the
> circuit — three innocents — before reaching the cause. Each accusation had a
> coherent explanation and **none had a test that could knock it down**.
>
> What closed the case was no measurement: it was **taking out the bridge and listening**.
> One variable, one gesture.
>
> **A hypothesis without a test that could knock it down is not a diagnosis — it is a narrative.**
