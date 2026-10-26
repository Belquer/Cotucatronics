# Recurring procedure — addressing the Loopy Pro

**The method that worked (22/08): `vistos`, not `learn`.**

65 of 65 were closed in two sweeps. The `learn` mode took five firmware
versions and never closed — it is described further on as a record, not as a
recommendation.

---

## The method: the board listens, you sweep

The right question is not *"how do I make the widgets learn my numbers?"*
but *"what are the widgets sending, and does it match my map?"*. What
adapts is the firmware, which is the side that gets edited.

```
vistos reset      # zeroes the inventory
                  # ... you move ALL the widgets, one at a time, no rush
vistos            # report
```

The report brings three things:

- **how many of the 76 arrived** — addressed and correct
- **FALTA cc<n> <param>** — never arrived: no address, or with another
  parameter's number
- **ORFAO cc<n>** — arrived and is not in the map: there the fix is in the firmware,
  not in the widget

`vistos` counts **distinct** CCs, not messages — sweep freely, repeat,
it does not overflow. It accumulates between sweeps until the `reset`.

## The two blind spots, and they matter

**`vistos` only sees a widget that SENDS.** A widget that only receives is
invisible, however well it is addressed. On 22/08 four showed up as
"missing" and one was perfect — dB saw it blinking. If a FALTA
persists, the test is to **press the widget and see whether the parameter changes on the board**.

**The learn has to be OFF.** In learn the board talks and does not listen:
every CC that arrives is discarded, and the inventory goes blind to everything.

## What the errors look like

Four widgets were **on another parameter's number** — `wrb_on_b` on the
51 (`wrb_mix_a`), and three more. Symptom: the neighboring parameter responds to two
controls and the widget "does nothing". `vistos` delivers them on its own: they
show up as FALTA while the neighbors arrive.

## On finishing

The sweep touches **every** parameter — the instrument ends up playing
what the widget positions say. `sync` does the inverse: the board pushes its
state and the widgets jump to the true positions.

And the warning that does not change: **with no preset in flash, the next firmware
flashing zeroes all the values.** The addressing survives (it lives on the iPad);
the adjustments do not.

---

# Record: the `learn` mode, REMOVED in v0.121.0

**It no longer exists in the firmware.** It went out whole — 136 lines, ~10 KB of
source, 83 references. It is preserved in commit `514e09c`, from where it can
be resurrected if one day it is needed.

**Why it did not coexist with `vistos`:** in `learn` the board talks and does
NOT LISTEN — every CC that arrives is discarded, by design. But the inventory reads
exactly what arrives. Turning on the learn blinds the only instrument capable of
proving that the learn worked. They were not two methods: they were two incompatible
regimes (bts 12t).

**If you need to address from scratch on a new iPad**, the way is to type the
numbers straight into the widgets, from the CC list in the README and the `ccmap`. It is
a single pass, deterministic, and does not depend on choreography.

What follows is the record of the removed mechanism.

## What the mechanism does

The board walks the CC map **in family order** (which is the page order:
CENTRAL, MOD, TIME, TECH) and pulses each one until the widget confirms. The
operator only arms; the board detects and advances.

**It transmits on channel 2**, which is the mirror's voice — so what the widget
learns is exactly what will receive it in operation.

**Short pulse, 127 followed by 0 in 60 ms, repeated every second.** A 127
sustained for a second is read by Loopy as *hold*, and the widget toggles
between hold and normal CC. The gesture it recognizes is press-and-release.

**The board confirms on its own.** A CC that arrives on the input channel with the
number of the current target can only come from a widget that just learned it — it is
proof of arming, at the instant of arming. The board logs `cc26 ARMADO` and advances.

---

## The procedure

```
learn <first cc>     # e.g.: learn 20 — enters PARKED
```

Parked is the right mode for the whole walk: it repeats the current CC
indefinitely and only leaves it when the widget confirms. The automatic
walk (`learn` with no argument) advances after 4 pulses **whether or not
someone is armed**, and is not suitable for work with a human in the loop.

After that, for each widget:

1. Read on the serial which CC is on air: `learn: 7/76 · cc26 · gate_release_b`
2. Arm the corresponding widget in Loopy
3. The board logs `cc26 ARMADO — avancando` and moves to the next one, on its own

Commands during the walk:

| command | effect |
|---|---|
| `learn next` | skips the current CC without arming (preserves the parking) |
| `learn <cc>` | repositions at a specific CC, to re-learn a widget |
| `learn <name>` | likewise, by parameter name |
| `learn stop` | **pauses**, without re-push |
| `sync` | pushes the 76 CCs on purpose, when you want to align |

---

## What NOT to do, and why (each one cost a round)

**Do not leave the board sitting there pulsing while no one operates.** The firmware
repeats the parked CC indefinitely. Every widget armed in that interval
learns the same CC. On 22/08 four widgets learned cc26 because it was
what stayed on the wire during a pause. **If you are going to interrupt, `learn stop`.**

**Do not use external polling to detect the arming.** It was the first attempt:
a script reading the history every 3 s and sending `learn next`. The delay
between the sweep and the arming IS the defect — whoever decides the advance has to be
whoever sees the confirmation, and that is the board. Detection from outside always has a
gap, and the gap is where the widgets learn the wrong CC.

**Do not trust the CC history to check an old arming.** It keeps the
**last 8** messages, with no timestamp. And the return path has
BLE latency: old messages arrive late and mix with the
recent ones. On 22/08 I read `[26, 30, 32]` while on 32 and nearly concluded that the
board was transmitting the wrong CC — the 26 and 30 were late echoes of
earlier parkings. **Verified afterward: the board transmits
exactly what it reports.**

**Do not count on automatic detection of the arming.** Tested and rejected for two
independent reasons: (a) not every widget returns a CC on learning — the
faders return, others do not; (b) the return latency makes it impossible to distinguish
"just armed" from "late echo from two steps back". What is left is the
operator saying they armed.

---

## Pausing is not finishing

`learn stop` **pauses and pushes nothing** (9 fine-tuning messages, not 76).
On 22/08 each pause fired a full re-push and dumped 76 CCs onto the Loopy screen
— dB read that as the board sending the wrong CC, and we spent a
round investigating a defect that was my pause command.

After a `learn stop`, the widgets may be showing **values from the
pulses**, not the real state. `sync` aligns whenever you want.

## On finishing

The **end of the list** fires automatically:

1. `eco_ult` goes back to `0xFF` — the table **has** to forget the walk,
   otherwise the mirror compares the real state against the `0`s of the pulses and swallows
   the edges where the two coincide.
2. Re-push of the 76 CCs by the same mechanism as boot. The pulses left the
   widgets in arbitrary positions; the re-push restores the board's truth
   across all pages.
3. Each push opens a 250 ms authorship window, which refuses the
   widget's return. The board wins, as it should.

Check in `midi`: `eco: TX <n>` should have gone up ~76, and `autoria:` should
show refusals — if it shows zero, either the widgets are not returning, or
the window is not working.

---

## Acceptance (what to prove before considering it addressed)

1. Each widget learned the **right CC** — check by moving the widget and seeing
   the right parameter change in `list`.
2. After the re-push, they all show the **real state of the board**.
3. `list` identical to the one before the learn — the pulses are **TX-only** and do not
   write to any parameter.

**State on 22/08: not concluded.** CC 20 to 25 (CENTRAL) were armed
and confirmed. The walk stopped at 7/76, cc26. The automatic confirmation
mechanism (v0.116.0) was implemented but **not yet exercised end to
end** — CC 20–25 were armed with external detection, the method
this page advises against.
