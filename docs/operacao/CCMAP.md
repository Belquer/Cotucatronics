# CC MAP — Cotucatronics
<!-- CARIMBO gerar_ccmap -->
> **GENERATED from `src/manifest.h`** by `gerar_ccmap.py v0.11.0` on **2026-09-05 08:32 EDT**.
> Do not edit by hand: run `python3 docs/operacao/gerar_ccmap.py`.
>
> **122 bindings across 84 CC numbers** · `src/manifest.h` @ `ddbcc49` · manifest v1.13.0
>
> Six channels: send `ch1`/`ch2`/`ch3`, listen `ch4`/`ch5`/`ch6` (listen = send + 3).
<!-- FIM CARIMBO -->


## How to read it

- **`_a` / `_b`** = chain A (piezo 1) and chain B (piezo 2). Each chain has its own
  input, character and **space**.
- **No suffix** = shared between the two chains.
- **Range** is in real units (what `set` accepts). The CC 0–127 is mapped
  onto it by the param's curve.

## The SIX channels — listen = send + 3

    SEND     ch1  shared
             ch2  chain A
             ch3  chain B

    LISTEN   ch4  shared
             ch5  chain A
             ch6  chain B

**The CC number is the same across all six.** Only the channel says whose it is and in which
direction — and the rule is arithmetic, not a table to memorize.

**Each page has ONE subject**, and that is what makes the duplication work: build
the chain A page on ch2 and duplicate it switching the channel to 3. The 37 paired ones
come out in one operation, with no shared one alongside to delete.

⚠️ **The channel is part of the address, not a filter.** CC 34 on ch2 is
`drv_drive_a`; on ch3 it is `drv_drive_b`. A CC that arrives on a channel with no binding
**moves nothing** and is counted (`canal sem vinculo` and `(canal,cc) fora do mapa`,
in `midi`).

**Every widget must listen**, and it is not a convenience: the board changes params
**on its own** in three situations — the cowrie shells draw lots on every beat, the CTOS
brings down effects when the sum ceiling overruns, and `preset load` swaps 33 at
once. A widget that does not listen starts lying after any one of them.

**`sync` checks the return by itself** and goes from `AVISO` to `OK` as the
widgets go back to listening.

**Authorship window: 250 ms, indexed by PARAMETER.** It used to be by CC, and with
multiplexing the CC stopped identifying the subject. A discard is counted
(`cc_janela`).

## Table

<!-- gerado por gerar_ccmap v0.11.0 de manifest.h — nao editar a mao -->
| CC | send | listen | param | family | real range |
|---|---|---|---|---|---|
| 1 | ch1 | ch4 | `microfonia_on_l` | — | 0 – 1 |
| 2 | ch1 | ch4 | `microfonia_on_r` | — | 0 – 1 |
| 3 | ch1 | ch4 | `microfonia1_hz_l` | — | 60 – 8000 |
| 4 | ch1 | ch4 | `microfonia1_hz_r` | — | 60 – 8000 |
| 5 | ch1 | ch4 | `microfonia1_db_l` | — | 0 – 24 |
| 6 | ch1 | ch4 | `microfonia1_db_r` | — | 0 – 24 |
| 7 | ch1 | ch4 | `microfonia2_hz_l` | — | 60 – 8000 |
| 8 | ch1 | ch4 | `microfonia2_hz_r` | — | 60 – 8000 |
| 9 | ch1 | ch4 | `microfonia2_db_l` | — | 0 – 24 |
| 10 | ch1 | ch4 | `microfonia2_db_r` | — | 0 – 24 |
| 11 | ch1 | ch4 | `microfonia3_hz_l` | — | 60 – 8000 |
| 12 | ch1 | ch4 | `microfonia3_hz_r` | — | 60 – 8000 |
| 13 | ch1 | ch4 | `microfonia3_db_l` | — | 0 – 24 |
| 14 | ch1 | ch4 | `microfonia3_db_r` | — | 0 – 24 |
| 15 | ch1 | ch4 | `microfonia4_hz_l` | — | 60 – 8000 |
| 16 | ch1 | ch4 | `microfonia4_hz_r` | — | 60 – 8000 |
| 17 | ch1 | ch4 | `microfonia4_db_l` | — | 0 – 24 |
| 18 | ch1 | ch4 | `microfonia4_db_r` | — | 0 – 24 |
| 19 | ch1 | ch4 | `micro_auto` | — | 0 – 1 |
| 20 | ch1 | ch4 | `piezo_mix` | mistura | -1 – 1 |
| 21 | ch1 | ch4 | `micro_dom` | — | 0 – 1 |
| 22 | ch1 | ch4 | `comp_on` | — | 0 – 1 |
| 23 | ch1 | ch4 | `comp_thresh` | — | 0.001 – 1 |
| 23 | ch2 | ch5 | `gate_thresh_a` | entrada | 0 – 1 |
| 23 | ch3 | ch6 | `gate_thresh_b` | entrada | 0 – 1 |
| 24 | ch1 | ch4 | `comp_razao` | — | 1 – 20 |
| 25 | ch1 | ch4 | `comp_makeup` | — | 1 – 8 |
| 25 | ch2 | ch5 | `gate_release_a` | entrada | 50 – 2000 |
| 25 | ch3 | ch6 | `gate_release_b` | entrada | 50 – 2000 |
| 30 | ch2 | ch5 | `drv_on_a` | drive | 0 – 1 |
| 30 | ch3 | ch6 | `drv_on_b` | drive | 0 – 1 |
| 31 | ch2 | ch5 | `drv_mix_a` | drive | 0 – 1 |
| 31 | ch3 | ch6 | `drv_mix_b` | drive | 0 – 1 |
| 34 | ch2 | ch5 | `drv_drive_a` | drive | 1.5 – 48 |
| 34 | ch3 | ch6 | `drv_drive_b` | drive | 1.5 – 48 |
| 35 | ch2 | ch5 | `drv_tone_a` | drive | 800 – 16000 |
| 35 | ch3 | ch6 | `drv_tone_b` | drive | 800 – 16000 |
| 36 | ch2 | ch5 | `drv_sujeira_a` | drive | 0 – 1 |
| 36 | ch3 | ch6 | `drv_sujeira_b` | drive | 0 – 1 |
| 40 | ch2 | ch5 | `phs_on_a` | phaser | 0 – 1 |
| 40 | ch3 | ch6 | `phs_on_b` | phaser | 0 – 1 |
| 41 | ch2 | ch5 | `phs_mix_a` | phaser | 0 – 1 |
| 41 | ch3 | ch6 | `phs_mix_b` | phaser | 0 – 1 |
| 44 | ch2 | ch5 | `phs_rate_a` | phaser | 0.02 – 4 |
| 44 | ch3 | ch6 | `phs_rate_b` | phaser | 0.02 – 4 |
| 45 | ch2 | ch5 | `phs_depth_a` | phaser | 0 – 1 |
| 45 | ch3 | ch6 | `phs_depth_b` | phaser | 0 – 1 |
| 46 | ch2 | ch5 | `phs_fb_a` | phaser | 0 – 1 |
| 46 | ch3 | ch6 | `phs_fb_b` | phaser | 0 – 1 |
| 47 | ch2 | ch5 | `phs_stages_a` | phaser | 4 – 8 |
| 47 | ch3 | ch6 | `phs_stages_b` | phaser | 4 – 8 |
| 50 | ch2 | ch5 | `wrb_on_a` | warble | 0 – 1 |
| 50 | ch3 | ch6 | `wrb_on_b` | warble | 0 – 1 |
| 51 | ch2 | ch5 | `wrb_mix_a` | warble | 0 – 1 |
| 51 | ch3 | ch6 | `wrb_mix_b` | warble | 0 – 1 |
| 54 | ch2 | ch5 | `wrb_rate_a` | warble | 0.5 – 12 |
| 54 | ch3 | ch6 | `wrb_rate_b` | warble | 0.5 – 12 |
| 55 | ch2 | ch5 | `wrb_depth_a` | warble | 0 – 3 |
| 55 | ch3 | ch6 | `wrb_depth_b` | warble | 0 – 3 |
| 56 | ch2 | ch5 | `wrb_irreg_a` | warble | 0 – 1 |
| 56 | ch3 | ch6 | `wrb_irreg_b` | warble | 0 – 1 |
| 60 | ch2 | ch5 | `dly_on_a` | delay | 0 – 1 |
| 60 | ch3 | ch6 | `dly_on_b` | delay | 0 – 1 |
| 61 | ch2 | ch5 | `dly_mix_a` | delay | 0 – 1 |
| 61 | ch3 | ch6 | `dly_mix_b` | delay | 0 – 1 |
| 64 | ch1 | ch4 | `dly_time` | delay | 0 – 1 |
| 65 | ch2 | ch5 | `frz_on_a` | freeze | 0 – 1 |
| 65 | ch3 | ch6 | `frz_on_b` | freeze | 0 – 1 |
| 66 | ch2 | ch5 | `frz_vol_a` | freeze | 0 – 1 |
| 66 | ch3 | ch6 | `frz_vol_b` | freeze | 0 – 1 |
| 67 | ch2 | ch5 | `frz_size_a` | freeze | 20 – 165 |
| 67 | ch3 | ch6 | `frz_size_b` | freeze | 20 – 165 |
| 71 | ch2 | ch5 | `rev_on_a` | reverb | 0 – 1 |
| 71 | ch3 | ch6 | `rev_on_b` | reverb | 0 – 1 |
| 72 | ch2 | ch5 | `rev_mix_a` | reverb | 0 – 1 |
| 72 | ch3 | ch6 | `rev_mix_b` | reverb | 0 – 1 |
| 73 | ch2 | ch5 | `rev_size_a` | reverb | 0 – 1 |
| 73 | ch3 | ch6 | `rev_size_b` | reverb | 0 – 1 |
| 74 | ch2 | ch5 | `rev_damp_a` | reverb | 0 – 1 |
| 74 | ch3 | ch6 | `rev_damp_b` | reverb | 0 – 1 |
| 75 | ch2 | ch5 | `shm_on_a` | shimmer | 0 – 1 |
| 75 | ch3 | ch6 | `shm_on_b` | shimmer | 0 – 1 |
| 76 | ch2 | ch5 | `shm_mix_a` | shimmer | 0 – 1 |
| 76 | ch3 | ch6 | `shm_mix_b` | shimmer | 0 – 1 |
| 77 | ch2 | ch5 | `shm_feedback_a` | shimmer | 0 – 1 |
| 77 | ch3 | ch6 | `shm_feedback_b` | shimmer | 0 – 1 |
| 78 | ch2 | ch5 | `shm_shift_a` | shimmer | 0 – 2 |
| 78 | ch3 | ch6 | `shm_shift_b` | shimmer | 0 – 2 |
| 79 | ch2 | ch5 | `shm_tone_a` | shimmer | 0 – 1 |
| 79 | ch3 | ch6 | `shm_tone_b` | shimmer | 0 – 1 |
| 80 | ch2 | ch5 | `tarol_on_a` | tarolator | 0 – 1 |
| 80 | ch3 | ch6 | `tarol_on_b` | tarolator | 0 – 1 |
| 81 | ch2 | ch5 | `tarol_mix_a` | tarolator | 0 – 4 |
| 81 | ch3 | ch6 | `tarol_mix_b` | tarolator | 0 – 4 |
| 90 | ch1 | ch4 | `buzios_on` | búzios | 0 – 1 |
| 91 | ch1 | ch4 | `buzios_thresh` | búzios | 0 – 1 |
| 92 | ch1 | ch4 | `buzios_cooldown` | búzios | 10 – 1000 |
| 93 | ch1 | ch4 | `test_tone_freq` | tom | 80 – 2000 |
| 94 | ch1 | ch4 | `test_tone_level` | tom | 0 – 1 |
| 95 | ch1 | ch4 | `test_tone_out` | tom | 0 – 2 |
| 96 | ch1 | ch4 | `test_tone_via` | tom | 0 – 1 |
| 97 | ch2 | ch5 | `route_a_l` | saída | 0 – 1 |
| 97 | ch3 | ch6 | `route_b_l` | saída | 0 – 1 |
| 98 | ch2 | ch5 | `route_a_r` | saída | 0 – 1 |
| 98 | ch3 | ch6 | `route_b_r` | saída | 0 – 1 |
| 100 | ch1 | ch4 | `pre_level_ab` | saída | 0 – 1 |
| 101 | ch1 | ch4 | `gov_ceiling_l` | proteção | 0 – 1 |
| 102 | ch1 | ch4 | `gov_ceiling_r` | proteção | 0 – 1 |
| 103 | ch1 | ch4 | `gov_release_l` | proteção | 100 – 3000 |
| 104 | ch1 | ch4 | `gov_release_r` | proteção | 100 – 3000 |
| 105 | ch1 | ch4 | `bench_boost_l` | bancada | 1 – 8 |
| 106 | ch1 | ch4 | `bench_boost_r` | bancada | 1 – 8 |
| 107 | ch1 | ch4 | `sim_laco` | — | 0 – 1 |
| 108 | ch1 | ch4 | `sim_hz` | — | 60 – 8000 |
| 109 | ch1 | ch4 | `sim_fb` | — | 0 – 2 |
| 112 | ch1 | ch4 | `shift_hz` | proteção | 0 – 10 |
| 113 | ch1 | ch4 | `out_l` | saída | 0 – 1 |
| 114 | ch1 | ch4 | `out_r` | saída | 0 – 1 |
| 116 | ch1 | ch4 | `post_level_l` | — | 0 – 1 |
| 117 | ch1 | ch4 | `post_level_r` | — | 0 – 1 |
| 118 | ch1 | ch4 | `exc_filter_l` | saída | 0 – 1 |
| 119 | ch1 | ch4 | `exc_filter_r` | saída | 0 – 1 |

<!-- 122 vinculos em 84 numeros de CC -->
>
> Six channels: send `ch1`/`ch2`/`ch3`, listen `ch4`/`ch5`/`ch6` (listen = send + 3).
>
> ⚠️ **DIRTY TREE** — `src/manifest.h` has uncommitted changes.
> This map describes code that no one else has.
<!-- FIM CARIMBO -->


## How to read it

- **`_a` / `_b`** = chain A (piezo 1) and chain B (piezo 2). Each chain has its own
  input, character and **space**.
- **No suffix** = shared between the two chains.
- **Range** is in real units (what `set` accepts). The CC 0–127 is mapped
  onto it by the param's curve.

## The SIX channels — listen = send + 3

    SEND     ch1  shared
             ch2  chain A
             ch3  chain B

    LISTEN   ch4  shared
             ch5  chain A
             ch6  chain B

**The CC number is the same across all six.** Only the channel says whose it is and in which
direction — and the rule is arithmetic, not a table to memorize.

**Each page has ONE subject**, and that is what makes the duplication work: build
the chain A page on ch2 and duplicate it switching the channel to 3. The 37 paired ones
come out in one operation, with no shared one alongside to delete.

⚠️ **The channel is part of the address, not a filter.** CC 34 on ch2 is
`drv_drive_a`; on ch3 it is `drv_drive_b`. A CC that arrives on a channel with no binding
**moves nothing** and is counted (`canal sem vinculo` and `(canal,cc) fora do mapa`,
in `midi`).

**Every widget must listen**, and it is not a convenience: the board changes params
**on its own** in three situations — the cowrie shells draw lots on every beat, the CTOS
brings down effects when the sum ceiling overruns, and `preset load` swaps 33 at
once. A widget that does not listen starts lying after any one of them.

**`sync` checks the return by itself** and goes from `AVISO` to `OK` as the
widgets go back to listening.

**Authorship window: 250 ms, indexed by PARAMETER.** It used to be by CC, and with
multiplexing the CC stopped identifying the subject. A discard is counted
(`cc_janela`).

## Table

