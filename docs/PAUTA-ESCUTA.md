# Listening session agenda

**Why it exists:** the items were scattered across the alignment notes of the
last few sessions. Without a list, the session judges what it remembers instead
of what it should — and ~20 firmware versions have gone by with no verdict from
the ear.

**When:** it is step 2 of the presets plan, and it comes **before** freezing
slot 0. Saving a boot state over unjudged parameters is photographing a debt and
calling it a reference.

---

## 1. Drive — the top of the dirt dial

`drv_sujeira` (CC 36) was approved **only at 0**, which is the tanh tube. The
wavefolder at the top has never passed the ear — only measurement (THD 17.9% →
58.9% at the peak, 8× more energy above the 7th harmonic, aliasing held to
−65 dB by the fixed 2 kHz low-pass).

**To judge:** does the top serve? Where in the travel does it stop serving? Does
the range between 0 and 1 have useful resolution, or is the interesting part
squeezed?

## 2. Shimmer back on, with `shm_shift` checked

On 22/08 `shm_shift_a` was at **0.00** — shimmer with no transposition sounds
like a fed-back reverb, not like shimmer. It is possible that much of the earlier
judgment of the shimmer was made in that state.

**To judge:** with real transposition, does the tail serve? Did `shm_tone`
(a cut + a −6 dB bell at 3 kHz, chosen against the measured ratio of the dry
tail) resolve the harshness that produced the "still scorched"?

## 3. Dry delay

Rewritten in C++ on 20/08, with a 50 ms floor and feedback 0.4 read from the
original patch. **It has never been judged on its own** — it has always sounded
inside a chain.

**To judge:** is the 50 ms floor short enough? Does the feedback fixed at 0.4 ask
for control?

## 4. The two spaces with real material

The parallel tail with independent space A and space B was built for
"string with shimmer, leather with clean reverb". Measured (490 µs in the target
config), never played with both materials at the same time.

**To judge:** does the separation do what it promised? Do the per-chain
`rev_mix` meter out as expected?

## 5. Gate, warble, double freeze, cowrie shells

The ones that went through a rewrite or restoration and have no verdict:
`Gate::Abre()` (resumed open), warble with mix, the two independent freezes,
and the cowrie-shell detector brought back from the dead.

---

## What the session must NOT do

**Do not adjust while judging.** Judging is saying whether it serves; adjusting
is another session. Mixing the two produces "it got better" with no reference.

**Do not leave the Code instrumenting during the listening.** It cost two false
diagnoses on 22/08: scripts loaded a whole "base" on every measurement and
overwrote dB's adjustments in the middle of the playing (bts 12l).

**Declare the CPU regime beforehand.** With everything on, the peak is at ~940 µs
of a 1000 deadline (bts 12u). If the session uses a heavy configuration, a block
overrun is a possible explanation for an artifact heard — and must not be
confused with an effect defect.
