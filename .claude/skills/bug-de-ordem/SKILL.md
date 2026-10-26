---
name: bug-de-ordem
description: Diagnosis of the three families of ordering bug in Cotucatronics — context race, configuration order, and a variable with two jobs. Use when something works sometimes, works in the simulator but not on the board, or when a test gives an incoherent result. Naming the family comes BEFORE fixing.
---

# The three families of ordering bug

**New bug: name the family before fixing it.** A fix without a name becomes a
patch, and the patch hides the family for next time.
See `docs/battle_scars.md` ch. 1.

## A — Context race

*Arming and checking have to happen in the same context.*

If the arming runs in an ISR/callback and the check runs in the loop (or vice
versa), the two observe different worlds. Classic symptom: paired counters that
"prove" the opposite of what is happening — M1==M2 saying that no line latched,
when the pair came from different lines.

**Diagnostic question:** *who arms and who reads, and do they run in the same
context?*

## B — Configuration order

*Configure before you consume.*

A value read before it is configured does not throw an error — it gives the
default, silently. That is how a module went dead: `r_window` filled in after
`prepareToProcess`, when the patch had already read the size.

**Diagnostic question:** *who reads that value, and have they already read it
before I write it?* When in doubt, **write at both moments** (before and after)
and comment why.

## C — One variable with two jobs

*The most treacherous one, because the test looks clean.*

One thing serves two purposes, and a test changes both at once. The result
measures the sum and separates nothing. Named on 08/17 after it cost a phantom
gain escalation (`fx2_gain` 8→48→64 calibrated against the wrong case).

**Diagnostic question, mandatory before touching anything:** *what are ALL the
jobs of this variable?* List them. If there are two, the flight has to isolate
one.

## Protocol when the symptom repeats

**On the third repetition of the same symptom, stop generating hypotheses and
look for the test that cuts the system in half.**

Switching suspects looks like progress and is not — it is the same measurement
dressed up as new. An hour was spent on five hypotheses (BLE link, fader, app,
DMA, dump), none confirmed and none ruled out, while the **loopback** — which
proves the whole board in ten seconds — had been waiting in the queue from the
start.

For an input channel, the test that cuts it in half **is the loopback**.

## Fourth family: the external-observer gap

A process with a human in the loop, observed from the outside by periodic
scanning, has between the event and the reading a window in which the wrong
state is recorded. This **is not latency** — it is corruption.

Case (08/22): the `learn` mode pulsed a CC so dB could arm a widget in Loopy;
a script read the history every 3 s and told it to advance. Minutes passed
between one call and the next, and the firmware kept pulsing the same CC. Every
widget armed in that interval learned the wrong CC — four of them.

The right signal already existed and was being wasted: a CC arriving with the
number of the current target **can only** come from a widget that just learned
it. The board saw this and merely incremented a counter.

> **Whoever decides the advance has to be whoever sees the confirmation.**
> Detection from the outside only works when nothing changes while no one is
> looking.

**Symptom that gives it away:** "everything is landing on the same value / the
same step". State that should vary and does not, in a process whose clock is
outside the observed system.

**Sister of the third family:** a mode that runs on its own and a mode that
waits for confirmation are not configuration of the same procedure — they are
two procedures. Treating them as one parameter is giving one variable two jobs,
and choosing wrong silently misaligns everything.

## When closing windows never ends, invert the question

Five firmware versions on a MIDI-learn addressing mechanism: the board pulsed a
CC, the operator armed the widget, it advanced. Each version closed a window
where the wrong number could be recorded, and with each window closed another
appeared. It never closed.

dB inverted it: *"I set them all to send on channel 1 — talk to them and see if
they match; if they don't, adjust on your side."* Two scans and 128 bytes of
counter closed 65 of 65, and flagged four widgets with another parameter's
number without anyone searching.

> **A mechanism that needs a synchronization window at every step, with one of
> its ends outside your view, is wrong — not mis-tuned.** Look for the
> formulation in which each side speaks when it wants, and someone merely
> listens and adapts.

**Sign that you are on the wrong mechanism:** fixes that spawn fixes, all in
the same family (timing, order, synchronization), without the original problem
shrinking.

**And declare what you cannot see.** I ran three hours of choreography with no
access at all to the operator's side — I could not see the widget, the name,
the page. That changes which mechanism is viable and had to be said in the
first message, not after he asked.

## Inherited API precondition, revoked by a build decision

A Makefile decision made months earlier can invalidate the precondition of a
library, **with no compilation warning at all**. It is an ordering bug on the
scale of the project, not of the function.

Case (08/22): libDaisy's `PersistentStorage::Save()` takes the QSPI out of
memory-mapped mode to erase. The code that does this lives in the QSPI. The
precondition — *"the app does not execute from the QSPI"* — is true under
`BOOT_NONE` and **revoked by `APP_TYPE=BOOT_QSPI`**. Nothing in the build
complains; the call compiles and the board hangs by luck of the cache.

> **The description says what the function does; the code says what it
> REQUIRES.** Before building on an inherited API, read the implementation
> looking for what it assumes about the environment — and check whether some
> build decision has already revoked it.

**Where the hidden precondition almost always lives:** operations that change
the mode of a peripheral the code is executing from; that write to memory from
which instructions are fetched; that disable interrupts for an indefinite time.

**And distrust "the library already handles that"** — even coming from someone
who knows the platform. In that case the description said "writes to a reserved
area well away from the code"; the code aligned to 256 bytes, had a default
offset of ZERO inside the bootloader's region, and erased by 4 KB sector.

## Record

New hypothesis: **record the bet with a percentage** before testing. A bet
without a number is an opinion you rewrite after the result.
