# CCs per page — Loopy Pro

> ⚠️ **NOT GENERATED, despite this sheet having said so until 06/09.** No
> generator writes it — it was made by hand from the `manifest.h`
> **v0.85.0 · firmware v0.178.0**, and the manifest is today at **v1.13.0**.
> Treat it as a record of that state, not as a mirror of the current one.
>
> A label with no generator rots with authority: nobody maintains it by hand
> *because it is generated*, and nothing regenerates it *because there is no
> generator*. Either you write it one, or you take the label away — this is the
> second.

## The rule

```
SEND     ch1 shared · ch2 chain A · ch3 chain B
LISTEN   ch4 shared · ch5 chain A · ch6 chain B
```
**Listen = send + 3.** The CC number is the same on all six.

⚠️ **A shared one never enters a page that gets duplicated.** It was the
session's only error pattern: `dly_time` and `shift_hz` traveled along with
chain A and appeared on the wrong channel.


## CENTRAL

Piezo mix, gain, gate and level. **Contains shared ones** —
do not duplicate this page.

| CC | send | listen | param |
|---|---|---|---|
| 20 | ch1 | ch4 | `piezo_mix` |
| 64 | ch1 | ch4 | `dly_time` |
| 23 | ch2 | ch5 | `gate_thresh_a` |
| 23 | ch3 | ch6 | `gate_thresh_b` |
| 25 | ch2 | ch5 | `gate_release_a` |
| 25 | ch3 | ch6 | `gate_release_b` |
| 100 | ch1 | ch4 | `level` |

*9 widgets*

## MOD A

Drive, phaser and warble of **chain A**.

| CC | send | listen | param |
|---|---|---|---|
| 30 | ch2 | ch5 | `drv_on_a` |
| 31 | ch2 | ch5 | `drv_mix_a` |
| 34 | ch2 | ch5 | `drv_drive_a` |
| 35 | ch2 | ch5 | `drv_tone_a` |
| 36 | ch2 | ch5 | `drv_sujeira_a` |
| 40 | ch2 | ch5 | `phs_on_a` |
| 41 | ch2 | ch5 | `phs_mix_a` |
| 44 | ch2 | ch5 | `phs_rate_a` |
| 45 | ch2 | ch5 | `phs_depth_a` |
| 46 | ch2 | ch5 | `phs_fb_a` |
| 47 | ch2 | ch5 | `phs_stages_a` |
| 50 | ch2 | ch5 | `wrb_on_a` |
| 51 | ch2 | ch5 | `wrb_mix_a` |
| 54 | ch2 | ch5 | `wrb_rate_a` |
| 55 | ch2 | ch5 | `wrb_depth_a` |
| 56 | ch2 | ch5 | `wrb_irreg_a` |

*16 widgets*

## MOD B

**Copy of MOD A with the channel at 3.** Same numbers.

| CC | send | listen | param |
|---|---|---|---|
| 30 | ch3 | ch6 | `drv_on_b` |
| 31 | ch3 | ch6 | `drv_mix_b` |
| 34 | ch3 | ch6 | `drv_drive_b` |
| 35 | ch3 | ch6 | `drv_tone_b` |
| 36 | ch3 | ch6 | `drv_sujeira_b` |
| 40 | ch3 | ch6 | `phs_on_b` |
| 41 | ch3 | ch6 | `phs_mix_b` |
| 44 | ch3 | ch6 | `phs_rate_b` |
| 45 | ch3 | ch6 | `phs_depth_b` |
| 46 | ch3 | ch6 | `phs_fb_b` |
| 47 | ch3 | ch6 | `phs_stages_b` |
| 50 | ch3 | ch6 | `wrb_on_b` |
| 51 | ch3 | ch6 | `wrb_mix_b` |
| 54 | ch3 | ch6 | `wrb_rate_b` |
| 55 | ch3 | ch6 | `wrb_depth_b` |
| 56 | ch3 | ch6 | `wrb_irreg_b` |

*16 widgets*

## TIME A

Delay, freeze, reverb and shimmer of **chain A**.

| CC | send | listen | param |
|---|---|---|---|
| 60 | ch2 | ch5 | `dly_on_a` |
| 61 | ch2 | ch5 | `dly_mix_a` |
| 65 | ch2 | ch5 | `frz_on_a` |
| 66 | ch2 | ch5 | `frz_vol_a` |
| 67 | ch2 | ch5 | `frz_size_a` |
| 71 | ch2 | ch5 | `rev_on_a` |
| 72 | ch2 | ch5 | `rev_mix_a` |
| 73 | ch2 | ch5 | `rev_size_a` |
| 74 | ch2 | ch5 | `rev_damp_a` |
| 75 | ch2 | ch5 | `shm_on_a` |
| 76 | ch2 | ch5 | `shm_mix_a` |
| 77 | ch2 | ch5 | `shm_feedback_a` |
| 78 | ch2 | ch5 | `shm_shift_a` |
| 79 | ch2 | ch5 | `shm_tone_a` |

*14 widgets*

## TIME B

**Copy of TIME A with the channel at 3.** Same numbers.

⚠️ The `dly_time` (cc64) is **shared** and lives in **CENTRAL**.
It does not enter this copy — it was the error pattern of the 24/08 session.

| CC | send | listen | param |
|---|---|---|---|
| 60 | ch3 | ch6 | `dly_on_b` |
| 61 | ch3 | ch6 | `dly_mix_b` |
| 65 | ch3 | ch6 | `frz_on_b` |
| 66 | ch3 | ch6 | `frz_vol_b` |
| 67 | ch3 | ch6 | `frz_size_b` |
| 71 | ch3 | ch6 | `rev_on_b` |
| 72 | ch3 | ch6 | `rev_mix_b` |
| 73 | ch3 | ch6 | `rev_size_b` |
| 74 | ch3 | ch6 | `rev_damp_b` |
| 75 | ch3 | ch6 | `shm_on_b` |
| 76 | ch3 | ch6 | `shm_mix_b` |
| 77 | ch3 | ch6 | `shm_feedback_b` |
| 78 | ch3 | ch6 | `shm_shift_b` |
| 79 | ch3 | ch6 | `shm_tone_b` |

*14 widgets*

## RANDOM

The cowrie shells. All shared, ch1.

| CC | send | listen | param |
|---|---|---|---|
| 90 | ch1 | ch4 | `buzios_on` |
| 91 | ch1 | ch4 | `buzios_thresh` |
| 92 | ch1 | ch4 | `buzios_cooldown` |

*3 widgets*

## TECH

Test tone, exciter, governor, routing and bench boost.
**Mixes shared and paired ones** — do not duplicate the whole thing.

| CC | send | listen | param |
|---|---|---|---|
| 93 | ch1 | ch4 | `tone_freq` |
| 94 | ch1 | ch4 | `tone_mix` |
| 95 | ch1 | ch4 | `tone_dest` |
| 96 | ch1 | ch4 | `test_tone_stage` |
| 105 | ch2 | ch5 | `exc_filter_l` |
| 105 | ch3 | ch6 | `exc_filter_r` |
| 110 | ch2 | ch5 | `gov_ceiling_l` |
| 110 | ch3 | ch6 | `gov_ceiling_r` |
| 111 | ch2 | ch5 | `gov_release_l` |
| 111 | ch3 | ch6 | `gov_release_r` |
| 112 | ch1 | ch4 | `shift_hz` |
| 113 | ch1 | ch4 | `out_l` |
| 114 | ch1 | ch4 | `out_r` |
| 115 | ch2 | ch5 | `bench_boost_l` |
| 115 | ch3 | ch6 | `bench_boost_r` |

*15 widgets*  ·  **+2 new paired ones: `route_a_l`/`route_b_l` cc97, `route_a_r`/`route_b_r` cc98**

## Shared ones, the closed list

They exist **only once**, always on ch1, and **never** enter a page that
gets duplicated:

- `piezo_mix` — cc20
- `dly_time` — cc64
- `buzios_on` — cc90
- `buzios_thresh` — cc91
- `buzios_cooldown` — cc92
- `tone_freq` — cc93
- `tone_mix` — cc94
- `tone_dest` — cc95
- `test_tone_stage` — cc96
- `level` — cc100
- `shift_hz` — cc112
- `out_l` — cc113
- `out_r` — cc114

**13 shared · 78 paired · 91 bindings**

### The routing matrix — cc97 / cc98

Paired like any other: **the CC says the OUTPUT, the channel says the CHAIN.**

| CC | ch2 (A) | ch3 (B) |
|---|---|---|
| **97** | `route_a_l` | `route_b_l` |
| **98** | `route_a_r` | `route_b_r` |

Four switches in two numbers. They are all born **on**, which is
what reproduces the sound from before the matrix. Turning off `route_a_r` and
`route_b_l` leaves chain A only on output L and B only on R; swapping the two
columns **inverts the chains** — which is what it exists for, when the
transducers are wired backwards.
