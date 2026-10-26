# Hardware acceptance — per-channel multiplexing (flights A and B)

> **State (23/08, v0.171.0): the FIRMWARE side is GREEN.**
> What remains is to remap the Loopy — see
> [RETRABALHO-LOOPY.md](RETRABALHO-LOOPY.md).
>
> | path | proof |
> |---|---|
> | board → iPad on **ch4** (shared) | *(to confirm)* |
> | board → iPad on **ch5** (chain A) | MIDI Wrench |
> | board → iPad on **ch6** (chain B) | MIDI Wrench |
> | iPad → board (return wire) | 93 CCs counted by the latch |
> | per-channel addressing, 37 pairs | native bench |
> | refusal of ch4–ch6 at the input | sweep of the map |
>
> **The Loopy learns were erased on 23/08**, so the remapping is from scratch
> and not a migration of 37 widgets. The whole map is in the document above.
>
> ⚠️ **If the Loopy does not react even with the learns redone, it is not the
> WIDI:** the Wrench proved delivery. It is the MIDI source inside the Loopy, in
> its preferences.

## Why before step 3

**A preset is a photograph: it photographs whatever is in front of it, the
defect included.** Capturing over an unvalidated addressing freezes the error
*inside the library* — and then it stops being in one place and comes to be in
every preset. If a chain-B widget still sends on ch1, or if the mirror does not
arrive on ch4–ch6, the "string-shimmer" preset is born with values that came from
the wrong place.

And the Loopy rework blocks step 3.5 (the listening that builds the library)
anyway — doing the acceptance now joins the two human tasks instead of
interleaving them.

## What is already proven, and by which instrument

| claim | proven by | where |
|---|---|---|
| every `_a`/`_b` pair uses the SAME CC number | native bench, **37/37 pairs** | `test_midi` |
| `_a` on ch1, `_b` on ch2, no exception | native bench, **37/37** | `test_midi` |
| the same CC on ch1 and ch2 moves DIFFERENT params | native bench | `test_midi` |
| no input binding on the mirror channels | sweep of the map | `test_midi` |
| each chain mirrors on its own channel | `CanalEspelhoDe` | `test_midi` |
| the mirror never coincides with an input | **`static_assert`**, exercised | `midi.h` |
| the board transmits on ch5 and ch4 | `eco: TX` counter | `midi` |

**What is NOT proven and only the bench proves:** that the TX *arrives* at the
iPad on the right channels, and that the input works **through the real WIDI
path** — not just by the table. `voltaram 0` today is honest: the return path is
not on the wire.

## The scheme in force — SIX channels

    SEND     ch1 shared · ch2 chain A · ch3 chain B
    LISTEN   ch4 shared · ch5 chain A · ch6 chain B

**Listen = send + 3**, same CC number on all six.

> This document described the FOUR-channel scheme until 23/08 — and the script
> ordered checking `ch3`/`ch4` when the board already spoke
> `ch4`/`ch5`/`ch6`. **A wrong document does not sit still: it sends someone to
> the wrong place with the multimeter in hand** (bts 14h).

## Script

Serial open (`./monitor.command`), and `log on` before anything.

**1. TX on the right channels.** ✅ **DONE 23/08** — the MIDI Wrench saw the three
values of `level` (CC 100 = 64, 127, 0) on **ch5**. The path
`board → WIDI → iPad` is proven end to end.

*(original procedure below, to repeat when needed)*
 In the MIDI Wrench, after a `sync`:

```
sync
```

Expected: a burst on **ch4** (shared), **ch5** (chain A) and
**ch6** (chain B).
Confirm in `midi` that the TX went up:

```
midi
```

**2. Injection on the mirror channels.** Send any CC on ch5 and on ch4 and
confirm that it was **discarded and counted**, not obeyed:

```
midi
```

Expected: `voltaram` goes up (the board recognizes it as its own voice) and **no
param moves**. If any moves, the `static_assert` was bypassed by some new route
— it is a serious finding, stop and report.

**3. Input through both channels, through the real path.** The same CC number on
ch1 and on ch2 must move different params. The drive is the easiest to read:

```
set drv_drive_a 1.5
set drv_drive_b 1.5
list
```

Send **CC 34 = 30** on **ch1** and **CC 34 = 100** on **ch2**, and reread:

```
list
```

Expected: `drv_drive_a` and `drv_drive_b` with **different** values. This is the
test the Code ran over serial; here it runs over the WIDI.

**4. Loopy rework.**

- chain-B widgets now **send on ch2** (they were ch1, on other numbers);
- listening widgets migrate from ch2 to **ch5** (chain A) and **ch4** (chain B);
- the orphans `cc24`, `cc26`, `cc33` are re-addressed — they are
  `gate_thresh_b`, `gate_release_b` and `drv_mix_b`, which swapped their number
  for the pair's and changed channel.

⚠️ **Numbers freed by chain B, do not reuse yet:** 22, 24, 26, 32, 33, 42, 43,
52, 53, 62, 63, 68–70, 80–88, 106, 116–118. As long as there is an old widget
pointing at them, a reused number becomes an old widget touching a new param.

**4b. The RETURN path, and it is independent of the outbound one.**

The `sync` now checks itself and reports. If it gives `AVISO`, the next line
says what the board knows:

- *"o fio de volta JÁ FUNCIONOU neste voo (N CCs pelo FIO)"* → it is not the
  cable or the pairing; it is the widgets, which do not yet listen on ch4–ch6;
- *"a placa NUNCA ouviu o controlador"* → two remain: widgets on the wrong
  channel, **or** the `WIDI OUT → D14` wire does not close.

**The cheapest test to separate the two:** move **any fader** on the Loopy and
run `midi`. The latch accumulates since boot and has no clock — if `volta:` goes
up, the wire is good and the rest is configuration.

**5. Closing by the inventory.**

```
vistos reset
```

Sweep all the widgets of the pages, and then:

```
vistos
```

Expected: **86 of 86 bindings addressed**, zero `ORFAO`.

## The open item: `cc119`

It appears as `ORFAO ch1 cc119` and **nobody knows who sends it**. The test that
separates the two hypotheses:

- with the **iPad disconnected**, `vistos reset`, wait, `vistos` — if cc119
  appears, it is internal (something from the MIDI chain itself);
- if it only appears with the iPad connected, it is a **forgotten widget** on
  some page.

One command, two hypotheses, a binary answer.

## Useful commands at the bench

| command | for what |
|---|---|
| `pk` | RAW input per channel, pre-gain — answers "is signal arriving?" with no guess |
| `vistos [reset]` | inventory by (channel, CC), with orphans named |
| `midi` | counters: TX, voltaram, window, channel-without-binding, off-map, drained |
| `board` | version, tables, params with no CC, and every mode that alters the instrument |
| `espelho on/off` | mutes the output without affecting the input — measures MIDI without generating traffic |
| `dreno <ms>` | re-arms the input drain, to exercise the safeguard |
| `dup on/off` | A/B of the duplication in the same binary |

## After the green

Step 3 (presets in RAM) starts on **proven** addressing, and the listening that
builds the library happens with the certainty that each knob moves what it says
it moves.
