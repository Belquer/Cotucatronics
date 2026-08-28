# CCs por página — Loopy Pro

> **Gerado de `src/manifest.h` (v0.85.0, firmware v0.178.0).** Regerar se o mapa mudar.

## A regra

```
MANDA    ch1 compartilhados · ch2 cadeia A · ch3 cadeia B
ESCUTA   ch4 compartilhados · ch5 cadeia A · ch6 cadeia B
```
**Escuta = manda + 3.** O número do CC é o mesmo nos seis.

⚠️ **Compartilhado nunca entra em página que se duplica.** Foi o único
padrão de erro da sessão: `dly_time` e `shift_hz` viajaram junto com a
cadeia A e apareceram no canal errado.


## CENTRAL

Mistura de piezos, ganho, porta e nível. **Contém compartilhados** —
não duplicar esta página.

| CC | manda | escuta | param |
|---|---|---|---|
| 20 | ch1 | ch4 | `piezo_mix` |
| 64 | ch1 | ch4 | `dly_time` |
| 21 | ch2 | ch5 | `in_gain_a` |
| 21 | ch3 | ch6 | `in_gain_b` |
| 23 | ch2 | ch5 | `gate_thresh_a` |
| 23 | ch3 | ch6 | `gate_thresh_b` |
| 25 | ch2 | ch5 | `gate_release_a` |
| 25 | ch3 | ch6 | `gate_release_b` |
| 100 | ch1 | ch4 | `level` |

*9 widgets*

## MOD A

Drive, phaser e warble da **cadeia A**.

| CC | manda | escuta | param |
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

**Cópia da MOD A com o canal em 3.** Mesmos números.

| CC | manda | escuta | param |
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

Delay, freeze, reverb e shimmer da **cadeia A**.

| CC | manda | escuta | param |
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

**Cópia da TIME A com o canal em 3.** Mesmos números.

⚠️ O `dly_time` (cc64) é **compartilhado** e mora na **CENTRAL**.
Não entra nesta cópia — foi o padrão de erro da sessão de 24/08.

| CC | manda | escuta | param |
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

Os búzios. Todos compartilhados, ch1.

| CC | manda | escuta | param |
|---|---|---|---|
| 90 | ch1 | ch4 | `buzios_on` |
| 91 | ch1 | ch4 | `buzios_thresh` |
| 92 | ch1 | ch4 | `buzios_cooldown` |

*3 widgets*

## TECH

Tom de teste, exciter, governor, roteamento e boost de bancada.
**Mistura compartilhados e pareados** — não duplicar inteira.

| CC | manda | escuta | param |
|---|---|---|---|
| 93 | ch1 | ch4 | `tone_freq` |
| 94 | ch1 | ch4 | `tone_mix` |
| 95 | ch1 | ch4 | `tone_dest` |
| 96 | ch1 | ch4 | `test_tone_stage` |
| 105 | ch2 | ch5 | `exc_on_a` |
| 105 | ch3 | ch6 | `exc_on_b` |
| 110 | ch2 | ch5 | `gov_ceiling_a` |
| 110 | ch3 | ch6 | `gov_ceiling_b` |
| 111 | ch2 | ch5 | `gov_release_a` |
| 111 | ch3 | ch6 | `gov_release_b` |
| 112 | ch1 | ch4 | `shift_hz` |
| 113 | ch1 | ch4 | `out_l` |
| 114 | ch1 | ch4 | `out_r` |
| 115 | ch2 | ch5 | `bench_boost_a` |
| 115 | ch3 | ch6 | `bench_boost_b` |

*15 widgets*  ·  **+2 pareados novos: `route_l` cc97, `route_r` cc98**

## Compartilhados, a lista fechada

Existem **uma vez só**, sempre no ch1, e **nunca** entram em página que
se duplica:

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

**13 compartilhados · 78 pareados · 91 vínculos**

### A matriz de roteamento — cc97 / cc98

Pareados como qualquer outro: **o CC diz a SAÍDA, o canal diz a CADEIA.**

| CC | ch2 (A) | ch3 (B) |
|---|---|---|
| **97** | `route_l_a` | `route_l_b` |
| **98** | `route_r_a` | `route_r_b` |

Quatro interruptores em dois números. Todos nascem **ligados**, que é o
que reproduz o som anterior à matriz. Desligar `route_r_a` e `route_l_b`
deixa a cadeia A só na saída L e a B só na R; trocar as duas colunas
**inverte as cadeias** — que é para o que ela existe, quando os
transdutores estão ligados ao contrário.

