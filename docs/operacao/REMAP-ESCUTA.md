# Targeted remap for the listening session

> ⚠️ **IT IS NOT GENERATED, even though this sheet said so until 06/09.** No
> generator writes it — it was made by hand from `manifest.h` **v0.171.0, plus human judgment from `PAUTA-ESCUTA.md`**, and the
> manifest is today at **v1.13.0**. Treat it as a record of that state, not
> as a mirror of the current one.
>
> A label with no generator rots with authority: no one maintains it by hand *because
> it is generated*, and nothing regenerates it *because there is no generator*. Either
> one is written for it, or the label is removed from it — this is the latter.

The Loopy learns were erased. **Remapping the 86 before listening is
work the session does not need** — the agenda uses a fraction.

**And this does not limit what the library stores:** `capturar` records the 33
chain params **regardless of whether they are mapped**. What the iPad
does not yet control keeps being saved with whatever value it has.

**Rule:** send on ch1 (shared) · ch2 (chain A) · ch3 (chain B).
Listen = send + 3. Same CC number across all six.


## ESSENTIAL — without this you hear nothing

| CC | send | listen | param |
|---|---|---|---|
| 100 | ch1 | ch4 | `level` |
| 20 | ch1 | ch4 | `piezo_mix` |

*4 widgets · accumulated: 4*

## BLOCK 1+2 — drive and shimmer (the two that have a specific question)

| CC | send | listen | param |
|---|---|---|---|
| 30 | ch2 | ch5 | `drv_on_a` |
| 31 | ch2 | ch5 | `drv_mix_a` |
| 34 | ch2 | ch5 | `drv_drive_a` |
| 36 | ch2 | ch5 | `drv_sujeira_a` |
| 75 | ch2 | ch5 | `shm_on_a` |
| 76 | ch2 | ch5 | `shm_mix_a` |
| 77 | ch2 | ch5 | `shm_feedback_a` |
| 78 | ch2 | ch5 | `shm_shift_a` |
| 79 | ch2 | ch5 | `shm_tone_a` |

*9 widgets · accumulated: 13*

## BLOCK 3+4 — delay alone and the two spaces

| CC | send | listen | param |
|---|---|---|---|
| 60 | ch2 | ch5 | `dly_on_a` |
| 61 | ch2 | ch5 | `dly_mix_a` |
| 64 | ch1 | ch4 | `dly_time` |
| 71 | ch2 | ch5 | `rev_on_a` |
| 72 | ch2 | ch5 | `rev_mix_a` |
| 73 | ch2 | ch5 | `rev_size_a` |
| 74 | ch2 | ch5 | `rev_damp_a` |
| 71 | ch3 | ch6 | `rev_on_b` |
| 72 | ch3 | ch6 | `rev_mix_b` |
| 75 | ch3 | ch6 | `shm_on_b` |
| 76 | ch3 | ch6 | `shm_mix_b` |

*11 widgets · accumulated: 24*

## BLOCK 5 — gate, warble, double freeze, cowrie shells

| CC | send | listen | param |
|---|---|---|---|
| 23 | ch2 | ch5 | `gate_thresh_a` |
| 25 | ch2 | ch5 | `gate_release_a` |
| 50 | ch2 | ch5 | `wrb_on_a` |
| 51 | ch2 | ch5 | `wrb_mix_a` |
| 55 | ch2 | ch5 | `wrb_depth_a` |
| 65 | ch2 | ch5 | `frz_on_a` |
| 66 | ch2 | ch5 | `frz_vol_a` |
| 65 | ch3 | ch6 | `frz_on_b` |
| 66 | ch3 | ch6 | `frz_vol_b` |
| 90 | ch1 | ch4 | `buzios_on` |
| 91 | ch1 | ch4 | `buzios_thresh` |
| 92 | ch1 | ch4 | `buzios_cooldown` |

*12 widgets · accumulated: 36*

## Order of attack

Do it layer by layer and stop when you cover what you are going to judge. **The first
layer alone already lets you play and listen**; the others unlock specific blocks
of the agenda.


The rest of the 86 is remapped later, when there is a reason — the complete
list is in `RETRABALHO-LOOPY.md`.
