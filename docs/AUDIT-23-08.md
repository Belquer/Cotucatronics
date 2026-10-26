# Audit — commit `696fa0c`, board v0.171.0

> **Done after a session of ~40 versions**, with the MIDI multiplexing redone
> twice, presets in RAM implemented, three encoders with the pin map fixed
> halfway through, and the OLED that never worked.
>
> **The audit does not fix** (R4). The fixes are separate commits.

> **PARTIAL CLOSE, 2026-09-04.** This document is a dated snapshot and
> **is not rewritten** — what has landed since then is recorded here, on top.
>
> - **⚪ 4 (README with old versions) — CLOSED.** Not just the seven: the
>   `versoes_readme.py` v1.0.0 found **15 of 58** derivatives, one of them with
>   the README AHEAD of the source. Today **61 of 61 match**, and the class now
>   has a checker — which was exactly what the item asked for (*"`check_docs`
>   doesn't catch it… Fix: code + doc"*). It runs inside `/audit`.
> - **⚪ 5 ("76 CCs") — OPEN, and bigger than it was written.** It is no longer
>   the two comments at `main.cpp:1114` and `1290`: today there are **three**
>   (`1521`, `1697`, `1708`), and the real number is **119** bindings, not 86.
>   It has not been touched — it is `src/`, and is with another session.
> - **🟡 2 (CC duplicate detector) — WAS ALREADY CLOSED, 05/09.** The item
>   asked for a sweep in `Init` and `board` shouting. It exists: `Colisoes()` and
>   `Colisao()` in `manifest.h`, called at `main.cpp:5134`, and `board`
>   prints `colisao de CC`. It was implemented after the audit and nobody closed
>   the entry. The function's comment narrates the case that motivated it — the
>   notch registered on CC 115 that already belonged to `bench_boost` — and
>   leaves the line that stands on its own: *"counting who has no CC does not
>   find who has a stolen CC — they are different questions, and only one of
>   them was being asked."*
> - **🟠 1 (four documents on the FOUR-channel scheme) — CLOSED,
>   verified on 05/09.** The truth is in `midi.h:88` — *"the six channels, and
>   the rule is just one: LISTEN = SEND + 3"*. The four documents match: the
>   `CCMAP.md` is generated and carries **send/listen** columns (ch1→ch4); the
>   `ACEITE-MULTIPLEXACAO.md` proves ch4/ch5/ch6 and orders ch4–ch6 to be
>   **refused** at the input; the `PRESETS.md` carries the whole scheme in lines
>   64–69; the `STATUS` likewise. The mentions of "four channels" that remain are
>   **narrations of the fix** — `ACEITE-MULTIPLEXACAO.md:61` itself says it
>   described the old scheme *until 23/08*. The presence of new text does not
>   prove the absence of the old, so it was checked against the lines this audit
>   cited, one by one.
> - **🟡 3 (revoked comment about `kCanalEco`) — CLOSED.** `kCanalEco`
>   has **0 occurrences** in `main.cpp`. The three lines left with the symbol.
>
> **With this the five items from 23/08 are resolved** — three had been done for
> a while and nobody closed the entry, and that is the lesson that stays: *an
> audit that does not fix needs someone to come back and close it, otherwise it
> becomes a phantom backlog that scares without informing.*

## What passed

| pass | result |
|---|---|
| **P0** inventory | 30 files, all classifiable. `detune.h` out of the build **with a full record** in FEATURES.md |
| **P1** context | **Clean callback** — `main.cpp:1652–2276`, zero `Log`, `MidiSendBytes`, `malloc`, `DelayMs` or OLED drawing |
| **P2** order | `hal::Init` → `params.Init` → `manifest.Init` → `pmapa`/hash → `EncoderInit` → `midi.Init` |
| **P4** ceilings | params 86/96 · bindings 86/128 · sacrificeable 10/12 · buttons 2/6 · randomizable 2/8 |
| **P5** orphans | **none** — every counter has a writer and a reader |
| **P7** arithmetic | `Set()` clamps via `Clamp01`, so the presets' `×127+0.5` casts do not overflow |
| **P9** symmetry | **37 `_a`/`_b` families, zero orphans on either side** |

## Findings

### 🟠 1 — Four documents describe the FOUR-channel scheme

`operacao/CCMAP.md:37` · `operacao/ACEITE-MULTIPLEXACAO.md:9,13,29,68` ·
`operacao/PRESETS.md` · `STATUS.md`

The change to six channels (v0.156.0) updated only `RETRABALHO-LOOPY.md` —
**which is generated.** The hand-written ones stayed.

**Consequence:** the acceptance script orders checking `ch3`/`ch4` in the MIDI
Wrench, and the board speaks **ch4/ch5/ch6**. It sends the operator to the wrong
channel.

**Fix: doc.**

### 🟡 2 — `ApplyMidiCc` declares "a duplicate is an error" and nothing detects it

`manifest.h:648`. `midi_overflow` only catches a capacity overflow.

**And the multiplexing made it worse:** before, two `AddMidi` with the same
number jumped out; now the same number appears **legitimately twice**, one per
channel, and a real duplicate hides among the legitimate repetitions.

**Fix: code.** A sweep in `Init`, a `midi_dup` counter, `board` shouts.

### 🟡 3 — Revoked comment surviving above what replaced it

`main.cpp:245-247` — three lines about `kCanalEco` immediately above the block
that begins at 248 with *"THE MIRROR LEFT CH2"*.

**Fix: doc-in-code.** Delete the three lines.

### ⚪ 4 — README with old versions in 7 files

`hal.h` 0.33/0.22 · `hal_daisy_seed.cpp` 0.38/0.24 · `main.cpp` 0.171/0.155 ·
`manifest.h` 0.81/0.79 · `midi.h` 0.8/0.7 · `qspi_raw.h` 0.2/0.1 ·
`test_midi.cpp` 0.4/0.2

**And `check_docs` doesn't catch it:** the drift rule checks whether the last
commit touched the version — it does not check the README's table. **Fix: code +
doc.**

### ⚪ 5 — "76 CCs" in two comments that today would be 86

`main.cpp:1114` and `1290`, used as a reference magnitude for a burst.

## Meta-finding: the audit script fell into 12c

The version detector accused `main.cpp` of a `0.125.0` header. It was **the
narration of the removal**, inside the comment that explains why the number left
there. R5 caught it — but only because the rule orders reading the neighboring
lines before asserting.

## Order of attack

1. **🟠 1** — before going back to the bench; it is the only one that changes what you do with your hands
2. **🟡 3** — same region of `main.cpp` that ⚪5 touches, a single flight
3. **🟡 2** — duplicate detector
4. **⚪ 4** — README rule in `check_docs`

## What was NOT verified (R7)

- **No sound.** All of P8 was left out — it requires A/B in the same binary and
  dB's ear.
- **The OLED track** — panel killed by the connector, no claim about its driver
  is confirmable until the new one arrives.
- **The four encoder acceptance criteria** — reverse direction, isolated push,
  fast spin, independence.
- **Full P3** — I covered only the central variables.
