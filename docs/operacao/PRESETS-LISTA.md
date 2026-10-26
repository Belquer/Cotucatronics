# ⚠️ MODEL CORRECTED ON 01/09 — READ THIS BEFORE THE REST

What is below this section describes the OLD model, of **two
libraries** (16 chain + 8 global, with different contents). **It is
wrong**, and dB corrected it with three sentences:

> *"presets are a single list. What changes is whether it is applied to piezo A, B or
> global (both)."*
> *"a preset always admits two values because I can select one for A and another
> for B or vice versa."*
> *"select preset A B or G = I spin the list until I find it and I confirm. G contains the
> values of A and B."*

## The right model

**ONE list.** Each preset stores a complete portrait:

| | |
|---|---|
| **chain A** values | the 39 `_a` params |
| **chain B** values | the 39 `_b` params |
| **shared** | `piezo_mix` · `dly_time` · `shift_hz` · `buzios_on/thresh/cooldown` |

**The destination decides how much of it goes in:**

| apply as | what goes in |
|---|---|
| **A** | only the A part, into chain A |
| **B** | only the B part, into chain B |
| **G** | **everything** — A part into A, B part into B, and the shared ones |

**The state is TWO pointers**, one per chain: the A part of preset 3 can be on
chain A while the B part of preset 7 is on B. Hence `ponteiro[2]`.

## The gesture

**Spin the list until you find it, and confirm.** It is not sixteen buttons — it is
a selector that scrolls through and a confirmation gesture, per destination (A, B, G).

⚠️ **And that leaves an open protocol question**, which is the first to
resolve: with "spin and confirm", CC 121 with the slot in the value no longer describes the
gesture. Either the selector sends the index and a second CC confirms, or the confirmation
sends the index at once. **dB's decision before any code is written** — getting it wrong
here is rewriting the dispatch and the Loopy page.

## What is ALREADY done

- **Exclusions** (`presets.h` v0.9.0): out went `out_l`, `out_r` and `pre_level_ab`,
  by dB's decision. The first two cut off the branch — topology and safety,
  like `route_*`; the third is test and emergency.
- **6 shared ones** remain, all musical.
- **CC 121 loads** and is proven on the board, but still with the old model:
  ch1 reads from a separate global library instead of applying to both chains.

## What is MISSING, and it is the first task

Unify the libraries in `presets.h`, and propagate. **It touches eight places**: the
structures, the helpers, the `dump` parser, the `capturar`, the `load`, the
CC 121 dispatch, the listing and the tests.

⚠️ **It is not end-of-session work.** It is the subsystem that will store the
result of the listening session, and it was attempted on 01/09 at the end of a long session
— the refactor was reverted halfway, on purpose, leaving only the exclusions.
Better not started than half done.

---

# PRESETS — the list, and how the buttons address it

> **v0.2.0** · 28/08 · a **living** document: the list grows during the listening
> session and this table keeps up, so the Loopy buttons can be created
> all at once instead of one by one.
>
> The preset system is in [PRESETS.md](PRESETS.md); the design, in
> [DESENHO-PRESETS-RAM.md](DESENHO-PRESETS-RAM.md).

---

## ONE library, TWO pointers

This is not an implementation choice — it is the decision that avoids a defect, and
it is written in `presets.h`:

> *"ONE library, TWO pointers. They are not two lists: if they were, 'corda'
> would exist twice and the copies would diverge on the first adjustment."*

So **there is no list A and list B.** There are sixteen chain presets, and each
one can be loaded into A, into B, or into both. If one day there are two different
strings, they are two presets — not two copies of the same one.

---

## Addressing: one CC, three channels

The same six-channel scheme of the whole instrument, applied to commands.
**The CC says what to do; the channel says where.**

| CC | action | ch1 | ch2 | ch3 |
|---|---|---|---|---|
| **121** | load preset | global | **chain A** | **chain B** |
| **122** | flash to the slot | global | **chain A** | **chain B** |

**The value is the slot number:**

```
0        clear — back to the manifest defaults
1 … 16   chain presets         (ch2 / ch3)
1 …  8   global presets        (ch1)
```

⚠️ **The widget has to be a BUTTON, not a fader.** The value is the payload, not a
position: a fader swept from 0 to 127 would fire sixteen loads along the
way. A button sends one message, once.

### Why 121 and 122, and not 34 or 127

A command lives outside the parameter range, and that is a scar:

> On 22/08 `cc34` was given to `drv_drive` in a reorganization of the map and
> no one cross-checked the two lists. The command intercept runs **before** the
> map and returns — so that fader went **deaf to MIDI in silence**, and
> every gesture on it fired a whole state push. Two owners for one
> number, and the intercept always wins.

`120` (sync), `121`, `122` stay together and far from everything that is a param.
`123`–`127` are left free for the rest: delete, next, previous.

---

## THE LIST

> **Empty until the listening session.** Each line is born of a judgment by dB,
> not of a guess — the sound acceptance is the musician's.

### Chain (16 slots · CC 121/122 on ch2 or ch3)

| slot | name | what it is | captured on |
|---|---|---|---|
| 0 | *(clear)* | back to the manifest defaults | — |
| 1 | | | |
| 2 | | | |
| 3 | | | |
| 4 | | | |
| 5 | | | |
| 6 | | | |
| 7 | | | |
| 8 | | | |
| 9 | | | |
| 10 | | | |
| 11 | | | |
| 12 | | | |
| 13 | | | |
| 14 | | | |
| 15 | | | |
| 16 | | | |

### Global (8 slots · CC 121/122 on ch1)

| slot | name | what it is | captured on |
|---|---|---|---|
| 0 | *(clear)* | back to the defaults | — |
| 1 | | | |
| 2 | | | |
| 3 | | | |
| 4 | | | |
| 5 | | | |
| 6 | | | |
| 7 | | | |
| 8 | | | |

---

## How to create the buttons all at once

One column per destination, the same set of values, different channels:

```
column A   →  CC 121, ch2, values 0,1,2,3…
column B   →  CC 121, ch3, values 0,1,2,3…
global     →  CC 121, ch1, values 0,1,2…
flash      →  CC 122, same channel as the column, same value as the slot
```

The buttons can exist **before** the presets: loading an empty slot is
refused with its own verdict, not with silence.

---

## What does not exist yet

⚠️ **The firmware does not yet listen to CC 121/122.** Today there is only `capturar
<a|b|g> <nome>` and `preset load` over serial. The buttons stay inert until the
dispatch is implemented — and that is deliberate: it is in step 3 of the plan,
after the listening, because pointers are only tested with a real library.

⚠️ **Flashing by button has no name.** A button sends a number, not a
word. Two ways out: the slot is born with a position name (`s01`…`s16`) and
is renamed later, or it is captured over serial with the right name already. During
the listening, writing the name on the spot is probably better — the name is the
judgment, and it is easier to write while you are still listening.

⚠️ **The map is `0xBE1B`.** A preset captured under another map is refused with
`gravado sob outro mapa de params` — correct behavior. If the map changes,
the whole library needs to be recaptured, and that is why the param renames
were done **before** the first capture.

---

## The global's slot 0 retires the warning "flashing zeroes the params"

An observation by dB on 02/09, and it changes the library's priority.

**"Flashing zeroes the params" appears in every briefing since August** and is
managed by memory: restore by hand and declare what was restored. It has already cost at
least twice — a state asserted that the flash had erased, and a whole
session re-dialing `level` without knowing why.

The library design already has the answer and no one had named it:

> **Global, slot 0 = boot state.**

On the day the library exists, flashing stops zeroing. It loads the 0.

That changes the argument of the listening session: it is not only the oldest
bottleneck of the project — it is what **retires a whole operating rule**
and the human error that it exists to contain. A rule that depends on someone
remembering is debt; a rule the startup fulfills on its own is design.

See the flashing routine in the skill `bancada-firmware` §2b/§2c — the two
stop being needed on the day this exists.
