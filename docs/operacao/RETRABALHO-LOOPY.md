# Retrabalho do Loopy — o mapa inteiro

> **Gerado de `src/manifest.h` em 23/08 (v0.153.0).** Se o manifest
> mudar, este arquivo envelhece — regerar antes de confiar.

## A regra, em uma linha

**Cadeia A e compartilhados no canal 1. Cadeia B no canal 2.**
O número do CC é o **mesmo** nos dois — a página da cadeia B é uma
cópia da página A com o campo de canal trocado, e mais nada.

**A placa responde de volta no canal 3 (cadeia A + compartilhados)
e no canal 4 (cadeia B).** Widget que quiser acompanhar o estado da
placa escuta ali, nunca no 1 nem no 2.

São **86 vínculos** em **49 números de CC**.


## CENTRAL — entrada, ganho e porta

| CC | canal 1 (cadeia A / compart.) | canal 2 (cadeia B) |
|---|---|---|
| 20 | `piezo_mix` | — (compartilhado) |
| 21 | `in_gain_a` | `in_gain_b` |
| 23 | `gate_thresh_a` | `gate_thresh_b` |
| 25 | `gate_release_a` | `gate_release_b` |

## EFEITOS — drive, phaser, warble

| CC | canal 1 (cadeia A / compart.) | canal 2 (cadeia B) |
|---|---|---|
| 30 | `drv_on_a` | `drv_on_b` |
| 31 | `drv_mix_a` | `drv_mix_b` |
| 34 | `drv_drive_a` | `drv_drive_b` |
| 35 | `drv_tone_a` | `drv_tone_b` |
| 36 | `drv_sujeira_a` | `drv_sujeira_b` |
| 40 | `phs_on_a` | `phs_on_b` |
| 41 | `phs_mix_a` | `phs_mix_b` |
| 44 | `phs_rate_a` | `phs_rate_b` |
| 45 | `phs_depth_a` | `phs_depth_b` |
| 46 | `phs_fb_a` | `phs_fb_b` |
| 47 | `phs_stages_a` | `phs_stages_b` |
| 50 | `wrb_on_a` | `wrb_on_b` |
| 51 | `wrb_mix_a` | `wrb_mix_b` |
| 54 | `wrb_rate_a` | `wrb_rate_b` |
| 55 | `wrb_depth_a` | `wrb_depth_b` |
| 56 | `wrb_irreg_a` | `wrb_irreg_b` |

## TEMPO — delay, freeze

| CC | canal 1 (cadeia A / compart.) | canal 2 (cadeia B) |
|---|---|---|
| 60 | `dly_on_a` | `dly_on_b` |
| 61 | `dly_mix_a` | `dly_mix_b` |
| 64 | `dly_time` | — (compartilhado) |
| 65 | `frz_on_a` | `frz_on_b` |
| 66 | `frz_vol_a` | `frz_vol_b` |
| 67 | `frz_size_a` | `frz_size_b` |

## ESPACO — reverb, shimmer

| CC | canal 1 (cadeia A / compart.) | canal 2 (cadeia B) |
|---|---|---|
| 71 | `rev_on_a` | `rev_on_b` |
| 72 | `rev_mix_a` | `rev_mix_b` |
| 73 | `rev_size_a` | `rev_size_b` |
| 74 | `rev_damp_a` | `rev_damp_b` |
| 75 | `shm_on_a` | `shm_on_b` |
| 76 | `shm_mix_a` | `shm_mix_b` |
| 77 | `shm_feedback_a` | `shm_feedback_b` |
| 78 | `shm_shift_a` | `shm_shift_b` |
| 79 | `shm_tone_a` | `shm_tone_b` |

## TECH — buzios, tom, nivel

| CC | canal 1 (cadeia A / compart.) | canal 2 (cadeia B) |
|---|---|---|
| 90 | `buzios_on` | — (compartilhado) |
| 91 | `buzios_thresh` | — (compartilhado) |
| 92 | `buzios_cooldown` | — (compartilhado) |
| 93 | `tone_freq` | — (compartilhado) |
| 94 | `tone_mix` | — (compartilhado) |
| 95 | `tone_dest` | — (compartilhado) |
| 100 | `level` | — (compartilhado) |
| 105 | `exc_on_a` | `exc_on_b` |

## PROTECAO — governor, rota, boost

| CC | canal 1 (cadeia A / compart.) | canal 2 (cadeia B) |
|---|---|---|
| 110 | `gov_ceiling_a` | `gov_ceiling_b` |
| 111 | `gov_release_a` | `gov_release_b` |
| 112 | `shift_hz` | — (compartilhado) |
| 113 | `route_exciter` | — (compartilhado) |
| 114 | `route_jack` | — (compartilhado) |
| 115 | `bench_boost_a` | `bench_boost_b` |
