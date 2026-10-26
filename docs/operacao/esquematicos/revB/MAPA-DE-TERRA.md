# GROUND MAP — carrier rev B

**04/09/2026 · v1.0.0 · the board's ground contract, before the layout**

> **This document does not teach grounding** — that is the
> [`../../ATERRAMENTO.md`](../../ATERRAMENTO.md), and the sweep is the
> `revisao-terra` skill. Here is **this design**: which conductors exist, what
> type each one is, where it goes, and **how it is proven** that it ended up as
> written.
>
> If this document and the schematic diverge, **the schematic is right and this
> one is old** — and §6 is the test that catches the divergence.

## Why it exists

The broadband hiss that cost three sessions was diagnosed down to a named rule:
`AGND` and `DGND` joined **outside** the board, forming a loop
([`ATERRAMENTO.md`](../../ATERRAMENTO.md) §5). On the permaproto that happened
because there was no design — there was a rail, and everything that needed
ground ended up there.

On a printed circuit board the same error is cheaper to commit and more
expensive to fix: a copper leak between the island and the plane is not seen,
does not beep anywhere, and only shows up as noise after five prototypes are
made.

**That is why the map comes before the layout, and brings the test with it.**

---

## 1 · Classification — every conductor is exactly one of these three

The question that decides is not "is this ground?", it is **"does this carry
current, or define a zero?"**

| type | current | legal destination |
|---|---|---|
| **force** | high, pulsed | its own wire/plane up to the star |
| **logic reference** | ~none | stays at the device that reads it |
| **audio reference** | ~none | `AGND` island |

### FORCE — goes to the star, and only to the star

| conductor | consumer | note |
|---|---|---|
| return from the **decoy PD** module | — | it is the origin; the star is born here |
| return from the **step-down** | 15 V → 5 V | ⚠️ **no large bank at the input** — Middlebrook, squeal guaranteed |
| return from the **TPA3116** module | class D | the largest consumer: **the shortest and thickest wire** |
| return from the **arcade LEDs** | ~30 mA × 2 | its own pair in the IDC (vias 11–14), never to the island |
| return from the **PTC/TVS** | protection | at the input node |

🚫 **None of these touches `AGND` at any point on the board.**

### LOGIC REFERENCE — `GND`, stays where it is read

Daisy `DGND` · ESP32-C3 · OLED · EC11 · the two pots · the two arcades · the
AO3400A gates · the `V_TRILHO_A2` divider.

⚠️ **The pots' 3V3 comes from the SAME Daisy that reads them.** Powering from
one supply and referencing to another puts the difference between the two inside
the reading.

⚠️ **The 100 k/22 k divider is a reference, not force** — even though it hangs
off the 15 V rail. What flows in it is 122 µA. It goes to `GND`, next to the A2
pin that reads it, and **not** to the star: taking it to the star would inject
the star↔Daisy difference into the rail's voltage measurement.

### AUDIO REFERENCE — the `AGND` island

Sleeve **and ring** of the two piezo jacks · shield of the piezo cables (**at one
end only**) · the ground of the whole pre-amp · commons of the two DPDTs · sleeve
of the two line TRS · the 10 kΩ of each amp input.

---

## 2 · The island is a LEAF — it enters and does not leave

The `AGND` island touches **one** device: the Daisy, through its audio pins. From
there it does not leave to anywhere else.

```
   AGND ISLAND (signal only, no current)
     piezo jacks (sleeve + ring) ────┐
     pre-amp, full ground ───────────┤
     DPDT commons ───────────────────┼── AGND ──┐
     sleeve of the line TRS ─────────┤          │
     10 k of the amp inputs ─────────┘          │
                                        AGND ── pin 20 of the socket
                                                │
                                          [ NetTie-2 ]  ⚠️ THE BRIDGE, AND IT IS THE ONLY ONE
                                                │
   GND PLANE (force and logic)            GND ── pin 40 of the socket
     star ── amp · step-down · decoy            │
     Daisy DGND · ESP32 · panel ────────────────┘
        20 and 40 are on the DIAGONAL of the module (20 next to USB-C, 40 opposite):
        the bridge conductor crosses the module, and its path is a choice
```

### ⚠️ THE BRIDGE IS DRAWN. THE SEED DOES NOT JOIN THE GROUNDS INTERNALLY.

> **Corrected on 04/09, against what this same document said hours earlier.**
> The earlier version claimed that the Daisy joins `AGND` and `DGND` internally
> and that drawing the bridge would create a second one. **It is the opposite.**

The libDaisy documentation says that, when working with the Daisy and external
hardware, you should **always connect the `DGND` and `AGND` pins outside the
Daisy**. The Seed exposes them on two distinct header pins — **20 (`AGND`)** and
**40 (`GND`)**, checked in the manufacturer's `Daisy_Seed_Rev4_Pinout.csv` — and
**does not join them internally**.

And the general rule for converters (Analog Devices) says where: the optimal
connection of `DGND` is to `AGND` **at the package itself**, at the same
potential, **and nowhere else**. Joining there minimizes the potential difference
between the converter's two grounds, which is what injects digital noise into the
analog.

### What this rewrites, and it is big

🔴 **P3 was not "removing a second bridge".** The permaproto's rail was the
**only** bridge. Separating `AGND` from the rail — as the plan said — leaves the
codec **with no reference at all**. It is exactly what happened on 31/08:
*"removing the signal ground traded a hum for a dead board."*

> **The right P3 is to MOVE the bridge, not remove it.** It leaves the shared
> rail and becomes a short wire between pin 20 and pin 40, at the socket. **In a
> single gesture, never in two.**

And the P3 loop is still a defect — but for another reason. It was not that the
bridge existed: it was that it was a **shared rail carrying current** instead of
a short wire at the pins.

### How it is drawn

`AGND` and `GND` remain **two nets** in the schematic. They connect through
**one `NetTie-2`**, at pins 20 and 40.

> ⚠️ **AND THEY ARE NOT NEIGHBORS.** This section said they were, and it was
> wrong — another position claim made without looking at the board. **Measured
> by dB on 04/09: pin 20 is in the LOWER LEFT corner, up against the USB-C, and
> 40 in the UPPER RIGHT corner, at the end opposite the USB.** They are on the
> diagonal of the module.

**A layout consequence, and it is not small:** the bridge cannot be a short wire
between pins next to each other. The conductor that joins the two grounds crosses
the length of the module, and **where it passes is a choice, not an accident**.

The Analog Devices rule — join next to the converter — becomes ambiguous when the
two pins are in opposite corners. The proposed resolution, to be decided with dB
in the layout:

> **The `NetTie-2` lives up against pin 20**, inside the `AGND` island, and from
> there **a single dedicated conductor** runs to pin 40. This way the island ends
> at the net-tie, and only one thing leaves it — which is the leaf-island
> definition of §2.
>
> 🚫 What is **not** done: letting the `GND` plane approach the island and touch
> it next to pin 20. That would be the bridge in the right place with the wrong
> copper — a plane instead of a conductor, and loop area to go with it.

| | |
|---|---|
| the net-tie gives | **exactly one** copper connection, at a chosen spot |
| the ERC still sees | **two** nets, each with its own `PWR_FLAG` |
| without a net-tie | `AGND` **floats** on the board — the codec with no reference |

🚫 **Still forbidden:** any *other* connection between the two. Planes that touch
at a corner, a second net-tie, a convenience wire. **One, and only one, and it is
the socket's.**

## 3 · Decoupling, by load type

See [`ATERRAMENTO.md`](../../ATERRAMENTO.md) §4 for the why. On this board:

| consumer | carries | ⚠️ |
|---|---|---|
| **TPA3116** | 2200 µF + 100 nF at the pins | without the bank: cross hum |
| **step-down** (input) | **only 100 nF** | 🔴 **a large bank here oscillates** (Middlebrook) |
| **step-down** (output) | 100 µF, **not 2200** | too much capacitance disturbs the loop |
| **ESP32-C3** | 100 µF + 100 nF local | without this, the radio comes out of the speaker |
| **Daisy** | 100 µF + 100 nF local | — |
| **OPA1642** | 100 nF per rail, **glued** | at centimeters it no longer serves |

**Every consumer carries the PAIR.** Only the large one lets the edges through;
only the small one has no energy. One alone is a finding.

⚠️ **Inrush:** the 2200 µF bank can make the PD supply read a short and
renegotiate. If the startup becomes intermittent, **this is it** — it is declared
here so a new defect is not hunted.

---

## 4 · The ESP32 ↔ Daisy bridge — the ground loop that is NOT negotiable

The two can be in different power states (one on USB, the other on the rail).
**There is a state in which one is alive and the other is not** — therefore:

🔴 **A direct ground alongside the signal wires, mandatory.** Without it, the
powered board injects current through the protection diodes of the unpowered one,
the line becomes undefined and whoever tries to transmit gets stuck.

⚠️ **The defect appears on the POWERED side.** It is exactly the family of the
02/09 lockup, which manifested on the Daisy and lived on the other side of the
cable.

🚫 **Do not remove this ground to fix noise.** Supply noise is fixed with local
decoupling (§3), not by cutting signal ground.

---

## 5 · High impedance and air coupling

- **A piezo is an antenna.** Signal and return travel **together**; shield
  connected **at one end only** (the board's), otherwise it becomes a loop.
- **Pre-amp up against the input jacks**, inside the island, short input trace.
  It is a layout constraint, not a preference.
- 🚫 **The input cables never run parallel to the amp's output** — class D is a
  square wave at hundreds of kHz with high current, and that is immune to ground
  schemes. Crossings at a **right angle**. In the enclosure: piezos on the left,
  exciters on the right, and that is why.
- **The lit panel measured 6 dB of floor at the input.** That is why the OLED
  clock goes guarded by ground on both sides in the IDC, and the LEDs have their
  own pair.

---

## 6 · THE TEST THAT PROVES THE SINGLE BRIDGE

> **It is this section that `ROTEIRO-KICAD.md` §4 cites.** One bridge too many
> does not appear in the schematic, does not appear in the render, raises no
> error in the ERC or the DRC, and is not heard until there is current running.
> Only a test catches it.

### 6a · First, the premise — and this time it is measured

Before anything, prove that the Seed does **not** join the grounds internally.
With a Daisy in hand, out of everything, a multimeter in continuity between the
**pin 20 (`AGND`)** and the **pin 40 (`GND`)** of the header:

✅ **MEASURED ON 04/09: THERE IS NO CONTINUITY BETWEEN 20 AND 40.** Open, as the
documentation says. The premise is now confirmed on three independent fronts —
the libDaisy doc, the manufacturer's CSV, and dB's multimeter on a board in hand.
**The bridge is ours to draw.**

| result | means |
|---|---|
| **open** | ✅ it is what was measured. The bridge is ours |
| **beeps** | 🔴 it would be another revision. Stop and rethink |

⚠️ **Do this test even when it is "resolved on paper".** The earlier premise of
this document — "the Daisy joins them internally" — was asserted with confidence,
repeated, and was wrong. It costs thirty seconds.

### 6b · Then, is the bridge the only one?

With the net-tie mounted, the island and the star beep even with the Daisy out —
and that is **correct**, not a defect. So the uniqueness test stops being
"measure with the Daisy out" and becomes:

```
1. Board with no power.
2. LIFT one leg of the net-tie (or do not populate it yet).
3. Multimeter in continuity:
     probe A on the AGND island  (sleeve of a piezo jack)
     probe B on the GND plane    (ground of the amp module)
```

| result | verdict |
|---|---|
| **opens** | ✅ **the bridge is the only one** — it exists only through the net-tie |
| **keeps beeping** | 🔴 **there is a second bridge.** Do not mount. Find and cut |

### 6c · And the positive confirmation

```
4. Net-tie populated/replaced, still with no power.
5. Same two probes.
```

| result | verdict |
|---|---|
| **beeps** | ✅ the bridge exists and passes through the net-tie, as drawn |
| **opens** | 🔴 **the island is floating** — the codec with no reference |

⚠️ **The three steps, and in order.** 6b alone does not distinguish "single
bridge" from "no bridge", and one of them kills the audio. **A test that does not
separate the two outcomes is not a test.**

### Acceptance criterion, written before the fix

> **Passes if, and only if: 20↔40 open on the bare Daisy · island↔star open with
> the net-tie lifted · island↔star closed with it in place.** Write the three
> results in the report, even when they pass.

### Then, under load

The test above is of copper, with the board dead. The one for **ground mass**
requires current — a ground measurement at rest gives zero on top of a defect.
The method is in [`ATERRAMENTO.md`](../../ATERRAMENTO.md) §8: multimeter in
**AC/mV**, black probe on the star, sound actually playing, **all the numbers
written down before touching anything**.

Reference measured on the permaproto, with a tone playing:

| path | before | after |
|---|---|---|
| star → amp | 0 mV | 0 mV |
| star → Daisy | **42 mV** | 0 mV (dedicated wire) |
| star → ESP32 bridge | — | 0 mV |

**On the new board, acceptance is 0 mV on all.** Tens of mV mean that that
consumer reaches the star through something else.

---

## 7 · The free ground meter

An audible periodic click that follows an LED, a relay or the screen refresh **is
coupling denouncing itself**. An LED draws milliamps: if that is audible, the
coupling is large, and everything else the board does passes through the same
path **with no pattern to give itself away**.

🚫 **Silencing it in firmware hides the defect.** With the ground done well it
disappears on its own — and that is the test that it was done well.

---

## 8 · What this map does NOT guarantee

Declared, not omitted:

- **It does not replace the DRC or the layout's ground review.** It is the
  schematic's contract; the copper can disobey it and nothing here catches it.
- **The purchased modules** (decoy, step-down, TPA3116) have internal grounds we
  did not draw and cannot audit. What we control is **where they go in and out**.
- **The AGND↔DGND connection inside the Daisy** is a datasheet claim, not our
  measurement.
- **The footprints of step-down, decoy, jacks, DPDT and EC11 do not exist yet** —
  dB measures when the parts arrive. Without a footprint there is no island
  drawn, and part of this map only becomes copper after that.
