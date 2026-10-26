# List of CCs to configure Loopy

> ⚠️ **NOT GENERATED, despite this sheet having said so until 06/09.** No
> generator writes it — it was made by hand from `manifest.h` **firmware v0.156.0**, and the
> manifest is today at **v1.13.0**. Treat it as a record of that state, not
> as a mirror of the current one.
>
> A label without a generator rots with authority: nobody maintains it by hand *because
> it is generated*, and nothing regenerates it *because there is no generator*. Either you write one for it,
> or you take the label off it — this is the second.

## The rule: six channels, and LISTEN = SEND + 3

```
SEND     ch1  shared
         ch2  chain A
         ch3  chain B

LISTEN   ch4  shared
         ch5  chain A
         ch6  chain B
```

**The CC number is the same across the six channels.** Only the channel says whose it is and
in which direction.

**Each page has ONE subject**, and this is what makes the duplication work:
build the chain-A page on ch2 and duplicate it swapping the channel to 3 — the
37 come out in one operation, with no shared one mixed in to delete.

### Why every widget must listen

It is not convenience. The board changes params **on its own** in three situations, and
a widget that does not listen starts lying:

- **the cowrie shells** draw lots on every beat;
- the **CTOS** drops effects when the sum ceiling overflows;
- **`preset load`** swaps 33 params at once.

`sync` pushes the state and **checks the return on its own** — it goes from `AVISO`
to `OK` as the widgets return to listening.


## SHARED — 12 · sends on **ch1** · listens on **ch4**

They exist only once. **Do not duplicate.**

| CC | parameter |
|---|---|
| 20 | `piezo_mix` |
| 64 | `dly_time` |
| 90 | `buzios_on` |
| 91 | `buzios_thresh` |
| 92 | `buzios_cooldown` |
| 93 | `tone_freq` |
| 94 | `tone_mix` |
| 95 | `tone_dest` |
| 100 | `level` |
| 112 | `shift_hz` |
| 113 | `out_l` |
| 114 | `out_r` |

## PAIRED — 37 · sends on **ch2**(A)/**ch3**(B) · listens on **ch5**/**ch6**

The name is the family: in the firmware they are `<name>_a` and `<name>_b`, and the channel
separates them.


### CENTRAL

| CC | family |
|---|---|
| 23 | `gate_thresh` |
| 25 | `gate_release` |

### MOD

| CC | family |
|---|---|
| 30 | `drv_on` |
| 31 | `drv_mix` |
| 34 | `drv_drive` |
| 35 | `drv_tone` |
| 36 | `drv_sujeira` |
| 40 | `phs_on` |
| 41 | `phs_mix` |
| 44 | `phs_rate` |
| 45 | `phs_depth` |
| 46 | `phs_fb` |
| 47 | `phs_stages` |
| 50 | `wrb_on` |
| 51 | `wrb_mix` |
| 54 | `wrb_rate` |
| 55 | `wrb_depth` |
| 56 | `wrb_irreg` |

### TIME

| CC | family |
|---|---|
| 60 | `dly_on` |
| 61 | `dly_mix` |
| 65 | `frz_on` |
| 66 | `frz_vol` |
| 67 | `frz_size` |

### TECH

| CC | family |
|---|---|
| 71 | `rev_on` |
| 72 | `rev_mix` |
| 73 | `rev_size` |
| 74 | `rev_damp` |
| 75 | `shm_on` |
| 76 | `shm_mix` |
| 77 | `shm_feedback` |
| 78 | `shm_shift` |
| 79 | `shm_tone` |
| 105 | `exc_on` |

### PROTECTION

| CC | family |
|---|---|
| 110 | `gov_ceiling` |
| 111 | `gov_release` |
| 115 | `bench_boost` |

## Reserved, not yet implemented

```
chain A preset   ch2 cc101   value 0..127 = slot
chain B preset   ch3 cc101   value 0..127 = slot
```

## When finishing

```
vistos reset
# ... move ALL the widgets, one at a time
vistos
```

`FALTA` = did not arrive · `ORFAO` = arrived off the map → **fix in the
firmware**, not in the widget.
