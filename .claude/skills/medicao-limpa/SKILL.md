---
name: medicao-limpa
description: Mandatory pre-flight before ANY measurement in Cotucatronics — proves the channel speaks, zeroes the environment, declares unit and regime. Use before running any bench test, reading the black box, measuring CPU, or interpreting counters. It is step 0 of IFÁ10000.
---

# Clean measurement — step 0 of IFÁ10000

**A zero on an unconfirmed channel is not data. It is the absence of data.**
Four MIDI test windows were read as "dead link" when the log was mute because
of a `bb arm` of mine (case 8 of `docs/battle_scars.md`).

No measurement begins before these five steps.

## 1. Prove the channel speaks — BEFORE any reading

Send a known echo command and **check the response**:

```
log on
board
```

- No echo → **abort**. Don't interpret, don't form a hypothesis, don't report.
- With echo → the channel is alive and the silence that follows now means
  something.

If a status command prints `[LOG MUDO]`, the mute is active: `log on`
unlocks it. Firmware < v0.39.0 doesn't declare it — in that case, reset.

## 2. Zero the environment — it is part of the measurement

A previous test state contaminates the next test. It has happened twice:
`tone on` forgotten, masking the piezo in the reverb measurement; cowrie shells
on, modifying the instrument on their own during the MIDI tests.

```
tone off
set buzios_on 0
list
```

Check in `list` whether any parameter is outside the approved default — a CC
sweep leaves `fx2_gain` and `src_mix` in a mess. If so, **the RESET restores the
approved manifest** (and in a MIDI test, it requires re-pairing).

## 2b. Check that nobody else is touching things

A previous test state is not the only contamination. **Ghost commands exist:**
BLE link transitions make iOS resend the state of every widget, and the
instrument obeys. It has already pushed seven parameters to the maximum in the
middle of a session — `src_mix` and `gate_thresh` at the top muted everything.

```
ble      → how many (re)connections and how many CCs the quarantine refused
list     → any param at the extreme (0.00 or 1.00) that you didn't put there?
```

**Seven parameters at the top simultaneously is not a human gesture.** If `list`
shows extremes you didn't set, the previous measurement is contaminated and the
instrument needs to return to defaults before any reading.

## 3. Declare what is being measured

Every measurement records, in writing, before running:

- **unit**: µs? % of what deadline? RMS, peak, or norm? real or normalized?
- **TEMPORAL regime**: onset or steady-state?
- **PHYSICAL regime**: how is the signal being produced? A loose transducer,
  struck by direct contact, is a different instrument from the same transducer
  mounted and excited by vibration. I measured the input with a **finger on the
  loose disc** and recorded "the input saturates" — a statement about the
  mounted instrument, made with a measurement that never touched it (scar 12a).

> **The regime is part of the data, not context around the data.** A number with
> no declared regime is not an incomplete measurement: it is a measurement whose
> validity nobody can judge, including whoever made it.
- **context**: which block (48/256), which clock, `-O2` or `-O3`?
- **material**: what signal is coming in (see step 6 of IFÁ10000)?

## 4. One flight, one variable

Before you touch anything, **list that variable's jobs**. The third family of
ordering bug is "one thing with two jobs, and a test that changes both at once."
If the variable serves two purposes, the flight measures two effects summed and
separates neither.

## 4b. ASSIGNING a delta requires that only one thing changed

The `full-audit` states this as R2 — *"a delta between builds is NOT
attributable"* — and the word **builds** makes the rule seem narrow. It isn't.

> **The A/B rule is not about builds: it's about deltas.** Two measurements that
> differ in more than one thing attribute nothing to any of them — whatever is
> being measured: CPU time, background noise, laser minutes, **dollars**.

Case (06/09): between two Ponoko invoices the price dropped $1,90. I attributed
it to the fit — the only one of the changes I had in mind. Two had changed: the
fit *and* the material thickness, which changes the geometry of the teeth.
Measured afterward: the price follows the **cut length** ($5,87/m, prediction to
$0,02) and the fit doesn't change the length. **The fit saved $0,00.** The $1,90
was all from the thickness. And I had already written a project rule on top of
the wrong attribution.

**What to do when you can't isolate** — and often you can't, because the other
side of the comparison has already happened:

> **Attribution isn't guessed, it's decomposed.** You look for the quantity the
> result is proportional to, and you test it with the measurements you have. Two
> invoices were enough to separate "by area" from "by meter of cut": one misses
> by $3,52, the other by $0,02. From there each segment has a price, and the
> attribution stops being opinion.

### And its sibling: two points always fit a line

After decomposing the Ponoko price into `$/m of cut`, I wrote that *"it's
predicted to within $0,02 of error"*. That wasn't a result — it was
**arithmetic**: with two points and a line, the error is zero by construction,
whatever quantity you choose. The third point (a new invoice) missed by **5%**,
and several two-variable models then fit all three — which, with three points and
two parameters, is also guaranteed and proves nothing.

> **A model is only tested on points that didn't fit it.** Counting how many
> points and how many parameters, and stating both, is part of declaring the
> number (§3). "Fits perfectly" with n points and n parameters is an empty
> phrase.

⚠️ **Special caution with numbers that arrive from outside** — an invoice, a
report, a sentence from dB. They come with the air of fact and no history, and it
is precisely because they don't have the shape of a benchmark that the A/B rule
isn't recognized in time.

## 5. Black box: arm by command, at the start of the flight

- `bb arm` **explicitly**, at the start — never in the middle of a flow.
- **The reading session writes nothing.**
- `bb arm` silences the log until `log on` (or reset). It is declared in the
  header of the status commands from v0.39.0 on.

## 5b. Before sizing, confirm that the problem exists in the real regime

Sizing early is solving a problem that may not exist, with a number that
certainly doesn't hold.

When there's a pending measurement, the right order is to do first **what serves
both possible outcomes**. Case (20/08): before sizing an attenuator for a
saturation measured outside the regime of use, the **protection** goes in — which
doesn't touch the gain, protects the ADC whether it clips or not, and doesn't
depend on the measurement's result.

## 6. Before spending code on a hypothesis, the measurement that would KILL it

A good hypothesis endorsed by the code itself is still a hypothesis. Before
implementing, run the test that would **knock it down** if it were wrong.

Case (20/08): the harshness of the shimmer looked like grain-splice noise — the
header of `detune.h` even prescribed "longer grain." The **spectral flatness**
measurement of the suspect band answered in thirty seconds:

```
shimmer tail 0,009   |   dry reverb tail 0,004   →  TONAL, not noise
```

Longer grain would have cost memory and **made the band worse**. Hypothesis dead
before it became code.

- **flatness ≈ 1** → noise (splices, aliasing, granulation)
- **flatness ≈ 0** → tonal (legitimate sound in the wrong place)

The two call for opposite fixes: a noise filter against the first, **EQ** against
the second.

## 7. A measured ruler, not the ear, to choose depth

When the fix is one of degree (how much to cut, how much to feed back), the
reference is a **measurement of the signal that already sounds right**, not a
matter of taste.

Case: to choose the presence drop of the shimmer, the ruler was the ratio
(2–6 kHz)/(300–2000 Hz) of the **dry** tail — the instrument's natural timbre in
the room. Dry 0,28; shimmer without bell 1,01; with −6 dB at 3 kHz, 0,19.

## 8. The complaint has as many parts as dB named

*"Highs **and mid-highs**"* are two parts. Fixing one, measuring an improvement,
and declaring victory leaves the other standing — and sometimes **worse**: taking
energy off the top made the mid-high band grow from 26,4% to 35,7% as a slice of
the total.

Before publishing, re-read your own measurement, asking **what it says about the
part you didn't attack**.

## 9. In a complaint with "but", the request is what comes after

*"When I turn on the drive I have more gain **but** the nice distortion
disappeared."* I read the gain as the defect and redesigned the module's
normalization to eliminate it. The gain was right. The absent distortion was the
request, and I fixed the complaint he didn't make.

Worse: **the two numbers that decided were on my screen when I chose.** The two
normalizations gave identical THD (12,0%); the difference was the layer's weight,
+30,4 dB against −3,2 dB. I chose the one that solved the gain.

> **When two quantities change together in a complaint, the one the operator
> NAMES as good is not the one he wants fixed.** Before touching anything, repeat
> the complaint out loud and underline the request.

And the corollary of step 6: **a hypothesis about a module with closed-form math
is tested by replicating the math in two minutes, not by flashing the board.** The
two measurements that decided the drive case — layer by tone, THD by level — ran
on the host, without touching the hardware, and killed two of my hypotheses before
they became code.

## 10. A number said in passing is a measurement too

On 22/08 I reported "CPU 180 µs" in a side remark, next to a 509 from the same
session. It looks like a 65% optimization. The two were **different regimes**: 181
is the floor with every effect off (post-flash default), 526 is the same profile
as the 509. Nothing had changed.

The rule was already written in this skill. It escaped because the mention was *in
passing*, not a formal measurement — and that is exactly where it escapes.

> **If a number appears near another of the same quantity, the reader WILL compare
> them.** The regime has to come along, or the comparison becomes a lie nobody
> wrote on purpose.

## 11. A check that requires turning off the sensor is inverted design

Two mechanisms solved the same problem and were mutually exclusive: one of them
needed the board **not to listen** in order to work — and listening was the only
way to prove it had worked.

> **A method that requires turning off the sensor that would verify it is
> inverted. The right one lets the world act and observes; the wrong one acts and
> assumes.**

Recognize it early: if the mechanism asks to suspend the input, silence a channel,
or "not listen right now", ask **what I lose the ability to observe while it
runs**. If the answer includes the very thing it is supposed to produce, stop
there — it's not five versions of tuning, it's the wrong mechanism.

## 12. Budget and wall are not the same limit

    CEILING = administrative budget. CHOSEN. Where you decide that the
              load is too high for comfort.
    DEADLINE = physical wall. IMPOSED by the hardware. Where it actually fails.

On 22/08 a safeguard was proposed as "M peaks above the **ceiling** in a window
T". With the number in hand — peak 975, ceiling 850, zero failures — it became
clear that exceeding the ceiling is benign routine at that load: the criterion
would knock the effect out all the time in a configuration that plays clean.

> **Confusing budget with wall makes the protection create the defect it was
> supposed to avoid.** Every safeguard declares which of the two it aims at — and
> the answer changes the criterion, not just the number.

**And the error only appeared on SIZING.** On paper the formulation sounded
reasonable. Sizing is not an implementation detail: it's where the option is
tested against reality.

## 13. A chosen constant ≠ a measured constant — label it

`kEstLimite = 3` / `kEstJanelaMs = 2000` came from a declared CRITERION ("don't
punish the isolated event"), not from measurement — the phenomenon measured zero.
The comment says so, and says to revise it if real usage disagrees.

> Months from now, "why 3?" has to find the answer in the code, instead of
> becoming archaeology.

## 14. A fix changes the regime — redo what it made obsolete

On 22/08, a power switch barred the audio signal before the amplifier. With the
signal barred the amplifier sat in no-load, and the supply measured **13 V firm
under load** — a correct measurement, and one that ruled out the supply.

With the switch fixed, the signal passed, the load appeared, and the supply
dropped to 6 V. **The earlier measurement was true about the broken system, not
about the fixed one.**

> **When you fix a cause, list which measurements that fix made obsolete — and
> redo them.** "I already ruled out X" has validity limited to the regime in which
> X was ruled out.

It's the same family as scar 12a (measuring the instrument in a regime that isn't
the one of use), with a twist: here the regime changed **because of our own fix**,
not through a mounting oversight.

## 15. The plausible hypothesis is the dangerous one — read the source of the configuration

On 22/08 the JEDEC ID came back `FF FF FF` and the explanation was good: *libDaisy
puts the chip in QPI; a chip in QPI ignores a 1-line command; the line sits at
pull-up; reading a bus at rest gives `FF`.* It explained the symptom **byte by
byte**.

It was wrong. The code said:

```cpp
// we haven't added the ability to switch to quad mode. So they're
// currently unused.
QSPIHandle::Result EnterQuadMode() __attribute__((unused));
```

Six flashes sweeping dummies and modes didn't answer what a `grep` in `Init`
answered in one minute.

> **When the symptom has a plausible explanation, the plausibility is the
> danger.** The test that decides is not the one that CONFIRMS the hypothesis —
> it's the one that **reads the source of the configuration**.

It's the inverse of step 6 ("the measurement that would kill it"): there the
hypothesis was weak and the measurement killed it cheaply. Here it was too good to
be questioned, and what knocked it down was **reading, not measuring**. Before
sweeping a peripheral's parameters, read whoever configured it.

## 16. A backlog item is a statement about the code, and it ages

*"`frz_size`: 61% dead travel, range 20–400"* — verified in the source: the range
is 20–165 and the clamp is 166,7. They match. Fixed months earlier, and the item
stayed in the queue, cited by me in several alignments.

> **Before citing a pending item, verify it** — especially the old ones, which are
> the ones most likely already fixed by another path.

## 17. The first reading is warm-up — including of an old probe

Rule §1 says to prove the channel speaks. This is its sibling on the instrument
side: **the first acquisition after opening the device is garbage.**

Case (28/08): the first run of a mapping gave `ch3/ch4` at −28,8 dBFS where
−77,2 was later measured — 48 dB of difference, and no code path that would
produce it. It was the transient of the `sounddevice` stream being caught by the
first `rec`.

Two defenses, both cheap:

```python
rms(0.4)                      # warm-up reading, thrown away
x = x[int(0.15*SR):]          # discard the attack within each acquisition
```

And the check that separates data from artifact: **measure the SAME state three
times.** With the instrument warm it repeated to 0,1 dB.

> **A number that doesn't close with any code path is a suspicion against the
> INSTRUMENT before it's against the system.** Checking the probe costs seconds;
> hunting a defect that doesn't exist costs the session.

## 18. A continuous periodic signal measures badly everything that copies itself

Delay, reverb, chorus, freeze — all return a delayed copy of the input. With a
continuous sine, the dry and the copy sum **with fixed phase** and interfere. The
resulting number is not the level of the effect: it's the result of the
interference.

Case (28/08): with the delay on, the output measured −44,4 dBFS against a base of
−35,6. **Turning on an effect lowered the level 8,8 dB** — and nothing in the
firmware did that.

The valid comparison came from another pair, with one variable only and the
**floor** as reference:

```
route_r_a=0, delay off   −69,2 dBFS   (floor)
route_r_a=0, delay ON    −39,5 dBFS   (+29,7 dB above the floor)
```

> **For "is it here or not", compare against the FLOOR with one variable only** —
> never two states that both contain interference. If you need the absolute level
> of the effect, use a burst or noise, not a sine.

## 17b. The warm-up applies to the WHOLE BOARD, not just the probe

Item 17 says the first reading is warm-up. **It also holds for the first
measurement after a FLASH** — and in that case the error is huge, not marginal.

*Measured on 02/09 with two inert builds (only a comment changed): the first
reading after the flash gave ~15 000 turns/s and the next three ~22 800, on both.
With the first discarded, the builds differ **0,27%** — against 1,3% of dispersion
from the instrument itself on the same binary.*

That artifact spawned an open item — "5 base points unexplained" — that survived
two days, crossed two requests for advice, and nearly led to a linker-map diff and
to moving the loop into ITCM. **The rule was written and the number that applied
it cost three minutes.**

> **Before attributing a difference between builds, measure the dispersion WITHIN
> one.** If the difference doesn't fall outside that envelope, there's nothing to
> hunt — and the envelope is measured by discarding the first reading.

## 19. Rare code on the hot path costs by SIZE, not by what runs

An `if` that almost never comes out true is cheap. **Its body, inlined, is not.**
If the compiler drags into the hot function a drawing, a font, or a parser that
runs once per startup, the I-cache pays the bill **on every turn**.

*Measured: turning on the OLED open in the body of the loop dropped the base from
~21 500 to ~18 600 turns/s **with the splash stopped**. A
`__attribute__((noinline))` gave back eight of the thirteen points — and was the
**only delta** between the two builds, which is the tightest A/B this project can
manage.*

The symptom that gives it away: **the base changed and the work didn't.** If the
number of turns per second falls without anything new running, look for code bloat
before looking for new work.

## 20. A saved baseline catches a regression nobody went looking for

The defect above was not caught by suspicion. It was caught because **there was a
base number from the same session** and the comparison offered itself.

> Every measurement is worth twice: once for the question that motivated it, once
> as a baseline for the next question. **Record the base even when it isn't the
> answer.**

## When finishing

Report the number **with the unit and the regime alongside**. A number with no
unit has already condemned a good module (freeze, 233 965 µs of onset against
3 520 of steady-state).
