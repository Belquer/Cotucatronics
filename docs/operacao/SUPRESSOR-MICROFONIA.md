# SuMiA™ — automated microphonics suppressor

> **BUILT AND RUNNING** since 02/09 (`main.cpp`, `PassoDoSupressor`).
> This document was born as the *contract*, written before the code, and kept
> saying "not yet built" for two days after the code existed — caught in the
> 04/09 deep clean.
>
> **What it has already done in service:** on 04/09, on the real sculpture and
> not the test bench, it filled the four mouths of both branches on its own —
> 3025,7 · 4523,4 · 790,9 · 5434,5 Hz on L and similar on R. It's the first time
> it holds findings of its own off the bench.
>
> ⚠️ **On by default** (`micro_auto` is born at 1). During a listening session
> it should be **turned off** — a hunter that changes a notch in the middle of
> the judgment makes a change of its own get attributed to the effect.

The original contract, measured before the code, follows below — it serves as a
design and as a record of what was decided and why.

## What it is, and what it isn't

**It's not a ritual triggered by a gesture.** That was the first idea (three taps
on the encoder) and dB replaced it with a better one: the suppressor **runs on
its own at startup and whenever the potentiometers move**.

That forces it to be **silent**. A ritual can raise the gain until the loop is
born; a thing that runs on every pot move can't. It **listens to what is already
happening** — when a peak dominates the band, it cuts it; when the mode
disappears, it backs off slowly.

**It's for the AMP path, not the jack.** Without an exciter there's no loop.

## The three triggers

| trigger | why |
|---|---|
| **startup** | the params are born zeroed and the body is what it is |
| **any pot move** | the loop gain scales with the volume: an adjustment made at one volume **isn't valid at another** |
| (continuous, at a slow cadence) | the modes drift with temperature |

## ⚠️ ANY pot rearms BOTH hunters

**Never "pot L rearms suppressor L".** The exciters are on the same body —
in the measured case, the drum is **welded** to the gas cylinder, it's a single
piece of metal. Raising the volume of one exciter changes the loop gain the
**other** piezo sees.

Measured on 02/09, and it's the proof: with the matrix separated (`route_b_l=0`,
`route_a_r=0`), the two branches reported **3275,7 and 3275,8 Hz** — the same
frequency to the tenths. The matrix separates the **electrical** paths; it
doesn't separate the **metal**.

Corollary for calibration: **isolating the matrix doesn't isolate the bodies.**
You can't assume L and R find different modes — they find the same ones, with
different strengths.

## What the listening session measured, and that the suppressor must reproduce

**Each mode that dies hands its place to the next.** In the same session:
1536 Hz → 3276 Hz → 535 Hz, and the second mouth was done in by the third. It was
that cascade that motivated **four mouths per branch** (measured cost: ~0,5% CPU
per mouth; eight in total cost ~4%, with a peak at 42,7% and margin up to 80% — it's
not the CPU that limits).

**The matrix's cross loop is worth 49 dB.** With the four cells open:
1536,4 Hz at **+24,4 dB**, dominance **0,99**. With `A→L` and `B→R` only:
1543,1 Hz at **−24,3 dB**, dominance **0,27**. The `route` stopped being a
routing detail and became a first-order control over the microphonics.

**Dominance is what separates a howl from a note**, and the threshold comes from
measurements and not from theory: real microphonics measured **0,97 to 1,00** when
it was screaming, and **0,14 to 0,29** after being cut. An earlier calibration on
synthetic sines gave 0,85, which would have rejected real cases of 0,61–0,68.

## The three decisions, made by dB on 02/09

### 1. The mouths don't live in any preset

**Ephemeral.** Each time the Daisy turns on, the suppressor goes into action,
suppresses, and stays quiet. Nothing persists.

That solves by construction the problem that was open: a notch in a global preset
**would travel between units**, and the pieces are replicated in a series with
modes particular to each weld — a frequency right on one piece is noise on
another. By not persisting, it doesn't travel.

And it limits the accumulation to **one session** and to **four mouths per branch**.

### 2. The rearm is the pot, and comes from the human gesture

> *"the user's action whenever there's microphonics, the instinctive action is to
> move the volume, so that should solve it"*

The trigger isn't an arbitrary cadence: it's the **gesture the person already
makes**. Whoever hears microphonics moves the volume — and that's exactly where
the loop gain changes and the old decision becomes invalid.

⚠️ And **any pot rearms BOTH hunters** — see the section above.

### 3. THERE IS NO BACK-OFF, and the reason is that the notch hides its own proof

Once placed, the mode is quiet **because of the mouth**. Looking at the spectrum
to decide whether it's still needed is looking at the fix itself. To know the
truth you'd have to **lower the mouth and listen for whether the howl returns** —
let the person take it, on purpose, in a cycle.

And the arithmetic is asymmetric:

| | cost |
|---|---|
| one mouth too many that's no longer needed | ~20 Hz at Q 35, 15 dB — **inaudible** |
| a wrong back-off | **the howl returns, with the gain that put it there** |

With the mouths ephemeral and limited to four, the accumulation doesn't
degenerate. **The back-off is the off button; the rearm is the pot.**

### 4. Arbitration: the HIGHEST-pitched always wins

> *"in the case of more than 4 peaks, always suppress the highest-pitched ones
> because they're the most painful"*

With the four mouths full and a new mode:
- if the new one is **higher-pitched** than the lowest mouth in use → **evict the
  lowest** and keep it;
- if it's **lower** than all four → **it doesn't enter**.

The criterion is **pain**, not strength. The edge case this leaves open is
recorded: a **low and very strong** mode can be left out while four weak high ones
occupy the mouths. If that shows up in listening, the rule gains a tiebreaker by
level — but it isn't invented before it hurts.

## What's left to decide

- **The dominance threshold** at which it decides "this is a howl". This session's
  measurements give the range (0,97–1,00 while screaming; 0,14–0,29 after being cut),
  but the final value comes from dB's ear and not from the middle of the range.
- **The cadence.** How often it analyzes, and how long a peak has to persist
  before it earns a mouth.
- **The initial depth.** 15 dB solved everything that came up by hand today;
  whether the automatic one should start shallower and deepen, or enter at 15 right away.
