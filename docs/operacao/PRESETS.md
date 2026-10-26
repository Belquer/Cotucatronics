# PRESETS — approved design

> **Status: steps 1 and 2 IMPLEMENTED (23/08, v0.135.0); steps 3–6
> designed.** Approved by dB on 22/08.
>
> Step 2 came out in **two separate flights**: A, the per-channel input
> addressing; B, the mirror to its own channels (which at the time
> were ch3/ch4 and became ch4–ch6 in the six-channel symmetry). Separated because they fail
> in different ways and `vistos` — the acceptance instrument — was
> in the middle of the change that affects it.
> Flash writing is blocked (see `DESENHO-ESCRITA-QSPI.md`); the
> design below **does not depend on it** to begin.

## The decision that reorganizes everything

> **The preset becomes the center of the user experience.**
> The params do not disappear — they stop being the interface.

Whoever plays chooses between *"corda-shimmer"* and *"tambor-seco"*. Whoever builds
touches the 86. The instrument's front panel shows **presets**, not parameters.

## One library, two pointers

**They are not two lists.** If they were, "corda" would exist twice and the copies
would diverge on the first adjustment.

```
CHAIN LIBRARY   n presets × ~34 bytes   ← pointer A, pointer B
GLOBAL PRESET   m presets × ~22 bytes   ← single pointer
```

Load "corda" into A and "tambor" into B. If one day there are two strings, load
"corda" into both. **Global slot 0 = boot state** — it is what kills `pre_level_ab`
being born at 0 and the false diagnostics it caused.

A **scene** = global + pointer A + pointer B.

## Duplication of the character params

The **10** shared character params (`drv_drive/tone/sujeira`,
`phs_rate/depth/fb/stages`, `wrb_rate/depth/irreg`) **become per-chain**.

> **Count correction (23/08):** this document said 11, and the list always
> had 10. The 11th character one is `dly_time`, which is left out on purpose —
> see just below. So 76 → **86** params, not ~87. Slack of 10 under the ceiling of 96.

**Cost: zero CPU — CONFIRMED on 23/08.** The internal states are already
separated per chain (two sets of allpass in the phaser, two warble buffers) —
it was measured when they were collected: *"it shrank the table, not the
work"*.

Measured with A/B **in the same binary** (`dup on|off`, which stayed in the firmware):
**+6,6 µs against 14 µs of spread — indistinguishable.** The comparison between
binaries would have lied: in the same regime, three builds gave 460 → 437 → 347 µs
just from instruction layout, because the code runs from QSPI. See scar 12d.

**`dly_time` stays shared** — it is the only one whose duplication would truly
cost (the delay is a single instance; different times would require two delays,
with real memory and CPU). Recorded as a separate question.

## Multiplexing by MIDI channel

```
INPUT    ch1 → shared
         ch2 → chain A
         ch3 → chain B
MIRROR   ch4 → shared
         ch5 → chain A
         ch6 → chain B
REFUSE   ch4–ch6 at the input (the board never obeys its own voice)

RULE: listen = send + 3. The CC number is the same across all six.
```

**The same CC numbers on both channels.** The Loopy pages come out
identical — copy the page and change one field. The total of CC numbers drops from
76 to ~45, **even duplicating the params**.

⚠️ *(historical)* The mirror moved off channel 2 (which becomes chain B input) and goes
to ch4–ch6. The safeguard of refusing its own voice stays, it just changes number.

**Status: DONE (v0.135.0).** What the implementation added to the design:

- the **`espelho on|off` switch** had to be born in flight A, not in B: without it
  the unconditional refusal of ch2 made chain B unreachable and the acceptance of
  flight A had no way to run. It stayed afterward as a test gain, decoupled
  from the input;
- **`espelho off` does not open the input right away** while the channels
  overlap — turning off a transmitter does not empty the wire, and a mirror message
  in flight becomes a **misaddressed** write on the sibling chain. In flight B the
  overlap became impossible by construction (`static_assert`);
- the **numbers freed up by chain B** (22, 24, 26, 32, 33, 42, 43, 52, 53,
  62, 63, 68–70, 80–88, 106, 116–118) still have old widgets pointing
  at them. `vistos` names them as `ORFAO ch1 ccNN`.

**Implementation consequences:**
- The **authorship window becomes indexed by PARAMETER, not by CC** — with
  multiplexing, CC 30 means different things depending on the channel.
- **`vistos` needs to report the channel** — a CC alone stopped identifying the param.
- **`ccmap` and the docs gain a channel column** — and it is the moment to generate the tables
  of the manifest (`<!-- gerado: ccmap -->`) instead of checking by hand.

## Storage: RAM first, flash later

The library lives in **RAM**, and `preset dump` / `preset load` over serial
persist it **on the computer**, as text. That delivers the whole system today —
library, pointers, capture, recall, OLED navigation — without waiting for the
flash.

On power-off, the board forgets; reloading is a command. When the flash
unlocks, **the library just changes house** — not a line of the system changes.
*One variable at a time, and the one that is blocked does not block the others.*

## Format: readable text, versioned in git

```
# presets.txt — biblioteca de cadeia
1  corda-shimmer   gate=12 drv_on=0 ... shm_on=1 shm_shift=88
2  tambor-seco     gate=40 drv_on=1 ... rev_on=1 rev_size=30
```

Reorder = move lines. Rename = edit text. Delete = delete a line.
**Version = git. Backup = free. Take to the next instrument = copy.**

Same philosophy as the manifest: *the instrument's identity is data, not code*.

## Save ≠ capture

**Writing to the flash stops the audio** (the ITCM window demands it). So:

- **`capturar`** = instantaneous, in RAM, no glitch. It is the bench gesture and
  can have a button in Loopy.
- **`gravar`** = writes to the flash, stops the audio, an explicit command.

A "save" button during the performance would produce a cut in the sound.

## OLED navigation

```
PRESET
> A  corda-shimmer
  B  tambor-seco
  G  palco-quente
```

The encoder moves the cursor between the three lines · **push enters** (the encoder now
scrolls the library, the name changes live) · **push again confirms and loads**.
One encoder, no extra button, the three states visible at once.

**Pre-listen while scrolling** (it loads while you navigate) with **restoration on
cancellation** — it is instrument behavior. It costs a copy of the previous state
on entering the edit mode, and a rate-limit on the mirror.

---

## THE WORKFLOW

```
dB    →  designs everything on the Loopy Pro (iPad), tests by ear, captures the preset
      →  if he needs to redo or reorder the list, does it by text (dump/load)
user  →  uses ONLY the instrument's OLED: chooses preset A, preset B, plays
```

The iPad is the **construction bench**. The OLED is the **performance interface**.
The complexity exists, but only whoever builds encounters it.

## Order of implementation

1. **Duplicate the character params** (before the library, otherwise it is born
   unable to express a different drive between a string and a drum)
2. **Multiplexing by channel** + mirror to ch4–ch6
3. **Preset system in RAM** with `dump`/`load`
4. **LISTENING SESSION = library construction session** — the judgment
   of the ~20 versions *becomes* the library, instead of being a bureaucratic prerequisite
5. **Flash** when the QSPI unlocks
6. **App** only if practice calls for it (a browser with Web Serial, not Python — zero
   installation; the downside is it does not run on the iPad)
