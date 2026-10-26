# CTOS — Cotucatronics Operational System

**Design document v0.4.0 · 2026-08-20 · design before code**

> **Status: PROPOSAL.** Nothing here is implemented. The document exists to
> be discussed and approved before any refactor — and so that the discussion
> happens over the boundary, not over code that has already been written.

---

## 0. Why it exists

An effect brought down the main loop. The instrument kept playing — the
audio runs on an interrupt — but the board went deaf to **everything**: serial,
MIDI, commands, parameters. Only an LED blinking straight from the loop managed
to report the death, because every other diagnostic instrument passed
through the loop that had died.

**The question that raises is not "which effect has the bug".** It is: *why can an
effect bring down the base?*

In a compartmentalized system that would be structurally impossible. The effect
could fail in every way — lock up, blow the deadline, read out of
bounds — and the instrument would lose **the effect**, never **the base**.

### A caveat of honesty, and it matters

The freeze is the **prime suspect** in the loop's death, and it is **not
confirmed**. The picture, after the cable was identified and stabilized:

| Build | Freeze | Loop |
|---|---|---|
| v0.46.2 | **off** | ✅ alive |
| v0.47.0 | on | ❌ dead |
| v0.48.0 | on | ❌ dead |
| v0.48.1 | on + mute log | ❌ dead |

Three deaths with freeze, one life without it — consistent, and the log has already been
cleared along the way. **What is missing is a single flashing:** a build with the
freeze off, with the cable confirmed. The v0.47.1 was exactly that and the
flashing timed out. (There was a freeze-off that died, v0.46.1, but with the cable
still moving — it does not count.)

**That does not weaken the CTOS — it reinforces it.** The argument is not *"the freeze is
guilty, so compartmentalize"*. It is:

> We spent hours unable to say **which half of the system** was
> broken, because there is no boundary that separates the halves. A system in
> which the question *"was it the base or the effect?"* has no observable answer
> is a system with no compartments — and the difficulty of the diagnosis IS the
> symptom.

The CTOS is justified by the **ability to answer that question**, not by the
guilt of a specific effect.

---

## 1. Context division as a CONTRACT, not a convention

Two halves, two execution contexts, and the boundary is law:

| | **CTOS** | **Effect** |
|---|---|---|
| Where it runs | main loop | **only** the audio callback |
| Interface | commands, params, state | `buffer → buffer` |
| Can it block? | yes (it owns the idle time) | **never** |
| Can it allocate? | yes, at boot | **only in Init** |
| Can it log? | yes | **never** |

**The two hard rules:**

1. **No effect code runs in the main loop.**
2. **No CTOS code runs in the audio callback.**

> **A convention is what we remember; a contract is what the build charges for.** Every
> rule in this project that depended on remembering has been violated at least
> once — including by whoever wrote it, on the same day.

### 1.1 How to audit it in `make check`

Each effect compiles to its own object. The boundary becomes verifiable by
**imported symbols**:

```bash
# an effect can only import from libc/libm. No hal, params, log.
arm-none-eabi-nm -u build/effect_*.o | grep -vE '<allowlist>' && FALHA
```

**Effect allowlist:** `memcpy`, `memset`, functions from `<cmath>`, and nothing
else. Any reference to `hal::`, `params`, `Log`, `Blackbox` fails the
build.

**In the reverse direction:** the CTOS cannot call an effect's `Process()` outside the
callback. That is harder to prove by symbol; the proposal is structural
— the effect pointer is only visible from inside the module that contains the
callback, so calling it from outside **does not compile**.

**Honest limit:** the symbol check catches dependency, not
behavior. An effect that does `while(1)` without importing anything passes the
audit. It is the watchdog (§3) that covers that — the two defenses are
complementary and neither alone is enough.

---

## 2. The effect's resource contract

An effect, to be embeddable:

- [ ] **Allocates only in `Init`**, and only via `hal::AllocBig()`. Zero allocation at
      runtime.
- [ ] **Never blocks, never waits, never sleeps.** No `Delay`, no
      retry, no wait loop.
- [ ] **No loop of unbounded duration.** Every `while` on the audio
      path needs a ceiling provable by construction.
- [ ] **Every buffer index is bounded BEFORE the access**, not trusting the
      caller's invariant.
- [ ] **Every cross-context state is `volatile`** and written by one side only.
- [ ] **Does not log, does not time, does not talk to the world.** Whoever observes is the
      CTOS, from outside.

### 2.1 Case study: the freeze v0.1.0/v0.2.0

What you can **verify by reading the code**, without attributing the loop's death to it:

**Allocation:** ✅ compliant. Allocates once, in `Init`, via `AllocBig`.

**Loops with no ceiling:** ❌ **violates.**
```cpp
float WrapF(float x) const {
    while(x < 0.f) x += n;      // small n + large x = many turns
    while(x >= n)  x -= n;
}
```
Two turns in normal use. But the ceiling depends on `x` and `n` being sane — and
**inside the audio callback, a loop whose ceiling depends on data is a structural
risk**, not a detail. `fmod` or direct arithmetic do not have that
problem.

**Unbounded index:** ❌ **violates.**
```cpp
const size_t i0 = (size_t)p;   // if p leaves the range, it reads out of the buffer
return b[i0] + f * (b[i1] - b[i0]);
```
`i1` is protected against going past the end; `i0` is **not**. It trusts that `p` is in
the range. In a 768 KB buffer in SDRAM, an invalid `p` reads someone else's memory with
no error at all.

**Cross-context state:** ✅ does not apply. All of the freeze's state is written
and read inside the callback — there is no context crossing. *(The initial
claim that it violated all three does not hold up on reading the code; it
violates two, and the two are real.)*

**The lesson of the case:** the two violations are the kind that **does not show up in a
test** — they always work, until the day the data leaves the range. Neither
would be caught by CPU measurement, by listening, or by the IFÁ10000 as it stands today.
Only an explicit contract catches them, and only before embarking.

---

## 3. Watchdog with an automatic veto

The CTOS measures the effect's time **per block**. An overrun of the budget:

1. the effect enters **automatic bypass**, in the same context that detected it;
2. the reason is logged and queryable;
3. **the instrument keeps playing** — loses the effect, keeps the base.

```
> ctos
CTOS v0.1.0 | loop vivo | fase 6 | uptime 412 s
  efeito fx2 "freeze": BYPASS por overrun
    estourou em t=3.2 s | pico 1180 us de 1000 us | 1 ocorrencia
  bypass e PERMANENTE ate `ctos rearm` — efeito que estourou uma vez
  estoura de novo, e religar sozinho esconderia o defeito
```

**This resurrects by design the fx2 bypass**, which existed and was revoked
when the Fx2 module came to be born on. The difference is one of nature: before it was
**point protection**, decided case by case and revocable for convenience;
now it is **a structural law of the CTOS**, and the condition for its return was already written
in the IFÁ10000 — *an uncertified module enters bypass until it passes the
protocol*.

**DECIDED by dB (18/08): PERMANENT bypass until `ctos rearm`.** Re-engaging
on its own would turn a reproducible defect into an intermittent one — and
intermittent is what cost this entire session. An effect that overran
once overruns again; the value of the bypass lies precisely in its **not** undoing
itself without someone looking.

---

## 3b. Sum ceiling — the watchdog's brother, not its substitute

§3 protects against **one effect with a defect**. It does not protect against **N healthy
effects that together do not fit**, and that is a situation of a different nature
that requires a different mechanism.

### The distinction that decides everything

| | Per-effect watchdog (§3) | Sum ceiling (§3b) |
|---|---|---|
| **What it catches** | one effect that alone overruns its budget | N healthy effects whose sum does not fit the deadline |
| **Is there a culprit?** | **yes** — the effect has an individual defect | **no** — it is excess ambition, no one is wrong |
| **The right question** | "who overran?" | "which instrument do we lose with the least harm?" |
| **Basis for the decision** | the effect's own measurement | **a priority declared cold by dB** |
| **Fairness** | the veto is fair because there is guilt | there is no notion of guilt here |

**It is here that the error of punishing the innocent would be born.** Any rule that tries
to find "who did it" when there is no one, ends up inventing a culprit — and the
invented one will be some perfectly correct effect. The way out is not a better
detection: it is **abandoning the question**. The sum ceiling does not look for guilt because
there is no guilt to look for.

### Why not LIFO

Bringing down the last one engaged is an approximation, and a bad approximation: it punishes by
**order of arrival**, which has no relation at all to musical value. The last
effect engaged may be exactly the one that dB was looking for when the
sum overran — the most recent gesture is, if anything, the most intentional.

**A declared priority is the correct version.** The choice is musical, and a musical
decision is taken **cold**, in the manifest, outside the heat of the overrun. The system,
at the instant of the failure, chooses nothing: it reads the order and obeys.

### The policy

1. **Sacrifice order in the manifest** (`Manifest::sacrif[]`). The order of the
   array **is** the priority — index 0 falls first. There is no hidden priority
   number anywhere; reordering the array is the only way to
   change the policy, and that is deliberate: the policy has to be readable at
   a glance.

2. **Measure the WHOLE BLOCK**, not the sum of estimates. An estimate goes missing
   precisely in the case that matters — interaction between effects, cache, debut. The
   block's stopwatch is the only number that corresponds to what actually
   happens. Subtract in **ticks** and convert afterward: dividing first makes the
   counter wrap stop correcting itself (the trap that already brought down the
   watchdog at 4 277 071 629 µs).

3. **Ceiling at 85% of the deadline, and only after 4 CONSECUTIVE blocks above.** The
   first block of any effect is expensive — it is the debut, and the project already has
   the debut/regime distinction documented. Bringing it down because of that would punish the
   act of **engaging**. Only the regime counts. The counter comes back slowly (it decrements,
   does not zero) so that an alternation does not mask a sustained overrun.

4. **The lowest-priority one that is ENGAGED falls.** Bypass by the same
   path dB would use: the on/off param goes to zero. The latch is
   reapplied **every block**, not only at the instant of the fall — without that a CC from
   the Loopy would re-engage the effect and the ceiling would have become a suggestion.

5. **The warning is mandatory.** Loud log + CC 119 on the MIDI output (value =
   index+1 of the effect brought down). Bringing it down in silence would be the "silent
   failure" family again, and this time with the instrument **changing its sound
   on its own** — the worst possible case of that family.

   **The warning comes from the LOOP, never from the callback.** The first version logged from
   inside the `AudioCb` and killed the serial on the first test: writing to the CDC from
   inside the callback starves the USB ISR, which is the priority-1 defect
   of this project and was catalogued before I committed it. The callback only
   marks (`volatile`); the loop reads the mark and speaks. Family A of the ordering
   bugs: whoever arms and whoever executes in the same context — and the context that can speak
   with the world is the loop.

6. **Permanent until `ctos rearm` or boot**, by the policy already settled in §3.
   `ctos rearm` releases the latches but **does not re-engage the effects**: releasing the latch
   gives back the *right* to engage; engaging remains dB's gesture. Re-engaging
   automatically would decide for him which configuration he wanted back.

7. **Overrunning with nothing to bring down is a state of its own**, and it declares itself. If
   all the sacrificeable ones have already fallen and the sum still does not fit, the base alone
   does not fit — and that is not solved by bypass. The mechanism's silence in that
   case would be read as its success.

### How to test without waiting for the limit to arrive on stage

```
ctos teto <us>     # artificial ceiling, forces the overrun without loading the CPU
ctos teto 0        # back to automatic
ctos               # shows block, peak, ceiling, drops and the whole order
```

Finding out whether the mechanism works at the instant the instrument meets the
limit is finding out too late.

### Cost

The mechanism cost **+14 µs** per block (within the noise between runs; the
peak did not change). It is two tick reads and one sweep of six
comparisons. Timing per effect was already happening; summing is free.

---

## 4. Orthogonal first-class diagnostics

**The principle, bought dearly:** *a diagnostic instrument cannot
depend on what it diagnoses.* Serial, MIDI and the black box all pass
through the main loop — when it died, **none of the three managed to
report**. The first version of the LED pulse had the same defect: it was the
loop itself that blinked, so it could only say "it died", never "where".

**It becomes permanent in the CTOS:**

- **The loop always blinks.** "Loop alive?" never again depends on serial.
- **The pattern encodes the last completed phase** — N blinks = phase N, a long
  pause between bursts. It is the lesson of the `DelayMs` hunt, now structural instead
  of temporary instrumentation.
- **Whoever blinks is the audio callback**, reading a `volatile` the loop
  publishes. The observer lives in the context that survives.

| Pattern | Means |
|---|---|
| N blinks, repeating | loop alive, completing up to phase N |
| steady on / steady off | callback alive, **loop dead** |
| nothing, and no sound | board dead or no power |

**A natural extension, if a second indicator exists:** one LED per context —
one for the CTOS, one for the effect. Then the question *"which half broke?"* comes to
have a **visual and immediate** answer, with no tool at all.

---

## 5. Map: what lives where

Explicit, so that the next addition does not have to guess.

### CTOS — main loop

| Responsibility | Why |
|---|---|
| WIDI / MIDI (`PollMidi`) | dispatch needs a context that can block |
| Serial (`PollSerial`, commands) | likewise |
| Parameter table | sole owner of the truth; the effect only reads |
| Watchdog and bypass | whoever judges cannot be judged in the same context |
| Black box | logging is the system's, not the effect's |
| Boot state, manifest, defaults | the sculpture's identity |
| Source routing and crossfade | decides **what** enters the effect |
| **Output dynamics**: gate, governor, limiter | the instrument's safety |
| **Reverb** | a permanent frame, not a swappable character |
| Cowrie shells (drawing lots) | touches params — it is a CTOS act, not a DSP one |
| Life pulse and phase | diagnostics is infrastructure |

### Effect — only the audio callback

| Responsibility | Why |
|---|---|
| Swappable DSP core | it is what changes from sculpture to sculpture |
| Own buffer, allocated in `Init` | a declared resource, never asked for in flight |
| Reading params (read only) | the effect obeys, it does not decide |

**The rule of thumb to classify something new:** *if it responds to a command, touches
params, or serves as a fixed reference point of the instrument, it is CTOS. If it transforms
sample into sample and can be swapped without changing the sculpture's identity, it is
Effect.*

**Note on the freeze:** by this yardstick it is an **Effect** (it transforms samples,
it is swappable) — but the `frz_on` is **CTOS**, because it is a performance gesture tied to a
button and a CC. The boundary passes between the gesture and the processing, and that is why
it needs to be drawn beforehand: without the map, "freeze" looks like one thing
only.

---

## 5b. Resident effects suite v1 — design (PROPOSAL)

Five effects in C++, all **Effect** by the yardstick of §5 (they transform sample
into sample, swappable without changing the sculpture's identity). Calibrated for dB's
material — percussion, kalimba, monochord, springs — and for the
Chase Bliss / dreamy aesthetic.

**Fixed pedalboard order**, and it is a musical decision, not an implementation one:

```
drive → phaser → warble → detune → [delay] → shimmer → [reverb]
```

Drive before everything because it glues and gives body to what comes in. Shimmer after the
delay and before the reverb because it **is** a reverb with tuned feedback:
putting it before the delay would make the delay repeat its tail.

### Pitch engine — ONE PER DERIVATION POINT

> ⚠️ **CORRECTION (18/08):** the first version of this document said "a single
> engine, consumed by the three". **It is wrong, and the error only showed up on writing
> the shimmer loop.**

An engine has **one buffer with one write point**. It can only serve
consumers that shift the **same source** — and the five effects do not tap at the
same spot:

```
dry → [detune, warble tap HERE] → drive/phaser → delay → reverb ⇄ [shimmer taps HERE]
```

The shimmer needs to write the **reverb's output**, because it is the tail that rises an
octave and feeds back. Detune and warble need the **dry**, before everything.

**dB's decision: one engine per derivation point.** The economy of
sharing is still real — it is just **per point**, not global:

| Engine | Taps | Consumers |
|---|---|---|
| `pitch_dry` | dry, pre-effects | detune (2 voices), warble (1) |
| `pitch_tail` | reverb output | shimmer (1 or 2 voices: +12 and +7) |

Cost: 150 KB of SDRAM each, sequential access. With 64 MB available, the
bottleneck was never memory — it was the SRAM pool of the external modules, and none
of these uses that pool.

**The lesson of the error:** *"shared" is a statement about the SOURCE, not about
the resource.* Two effects only share a history engine if they listen to the
same point of the chain. I wrote the design assuming they did, without checking
where each one pulls from — and the check cost one line per effect.

> ⚠️ **PREMISE CORRECTION: the freeze does NOT have a pitch-shifter.** It has the
> granular *machinery* — root-Hann window, drawn positions, interpolated
> reading — but it reads at **rate 1.0** (`pos_ += 1.f`). Pitch is rate ≠ 1.
>
> So the shared engine is an **extension** of the freeze's structure, not
> reuse of existing capability. The work is small because the structure is
> proven and certified, but it is not zero — and planning as if it were zero is
> like the `r_window` that no one checked.

**What the engine inherits from the freeze, and it is what makes it cheap:** grains with
root-Hann sum to constant power for uncorrelated positions, and the
position draw **kills the comb filtering** that a fixed-window pitch-shifter
produces. The two have already been measured and approved by ear.

### Cost — what dB's bet runs into

Bet: **< 15% of block 48 with all five engaged.** The numbers we already have:

| Piece | Measured |
|---|---|
| Freeverb (110 KB in SRAM) | **+5,6 pp** of CPU |
| Freeze: 2 grains, stereo, interpolated | **98 µs** of 1000 (≈10%) |

**The bet is reachable, but it depends on a design choice for the shimmer:**

- **Shimmer with its own Freeverb:** +110 KB of SRAM and +5,6 pp, plus the pitch
  (~10%) ⇒ **~15% for it alone**. The bet does not close.
- **Shimmer feeding back through the EXISTING reverb:** cost = only the pitch and the
  sum ⇒ **~10%**, and zero new memory. More faithful to the Eno/Lanois original,
  where the shimmer *is* the fed-back room.

**Recommendation: the second.** And it brings a declared risk: the feedback
loop now crosses the instrument's reverb, so `rev_mix` and
`rev_size` affect the shimmer's tail. That is a feature, not a defect —
but it needs to be in the `frz`-style comment and be told to the musician, because a
control that touches two places surprises.

The other three are cheap by construction: **drive** is a per-sample function
(zero memory), **phaser** is just allpass state (zero buffer), **warble** is
a ~20 ms delay (≈1 KB). Three of the five do not weigh.

### Each effect passes the full IFÁ10000

The freeze is the template — and what it cost serves as a warning: **three versions**
until it passed step 9, and the three failures were of design, not of code
(linear sweep becoming a loop; the window assuming correlation after the
draw removed it; mix as a crossfade erasing the live instrument).

**Measure individually AND with all engaged**, at block 48. The per-slot watchdog already
covers the isolated overrun; what it **does not** cover is the sum — five effects
within budget on their own may overrun together, and none would be vetoed
because none went past its own ceiling.

> **A project item this raises:** the CTOS needs a **sum ceiling**,
> not only per-effect ceilings. Recorded here as a pending item of §3.

### Order of work

1. **Shared pitch engine** (extension of the freeze).
2. **Shimmer** — first effect, validates the whole model: shared
   engine, slot, watchdog, IFÁ10000.
3. The other four, in order of increasing cost: drive, phaser, warble,
   detune.

---

## 6. Order of the refactor, when approved

1. **`ctos` as a command** — exposes loop alive, phase, uptime, effects in
   bypass. It is first because it is what makes the rest observable.
2. **Permanent LED pulse** with phase encoding *(already exists in
   v0.49.0 as a diagnostic; becomes structure)*.
3. **Watchdog with veto** and permanent bypass.
4. **Migrate what is on the wrong side of the line**, starting with what the freeze
   put on the audio path: `WrapF` with no ceiling and `Read` with no bound.
5. **Boundary check in `make check`**, by imported symbols.
6. **Boundary clause in the IFÁ10000** — a new step, or an item of step 1
   (CONTRACT): *"does this effect respect the CTOS's context division?"*

**The current dead loop resolves itself along the way.** It is not an obstacle to the
refactor — it is the living proof that the boundary needs to exist, and the first
test case of it.

---

## 7. What this document does NOT solve

- **We still do not know which half broke** — the v0.49.0 (LED per phase) is
  what will tell, and it has not been read yet.
- **It does not replace the kick-and-forget log ring.** Compartmentalizing keeps the
  effect from killing the loop; it does not keep the loop from drowning itself in USB.
- **The symbol check does not catch behavior.** The watchdog covers it; neither
  of the two alone is enough.

---

*"The instrument loses the effect, never the base."*
