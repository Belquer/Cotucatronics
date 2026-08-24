# Remapeamento MIRADO na sessão de escuta

> **Gerado de `src/manifest.h` + `docs/PAUTA-ESCUTA.md` (v0.171.0).**

Os learns do Loopy foram apagados. **Remapear os 86 antes de escutar é
trabalho que a sessão não precisa** — a pauta usa uma fração.

**E isto não limita o que a biblioteca guarda:** `capturar` grava os 33
params da cadeia **independentemente de estarem mapeados**. O que o iPad
não controlar ainda continua sendo salvo com o valor que estiver.

**Regra:** manda no ch1 (compartilhados) · ch2 (cadeia A) · ch3 (cadeia B).
Escuta = manda + 3. Mesmo número de CC nos seis.


## ESSENCIAL — sem isto não se escuta nada

| CC | manda | escuta | param |
|---|---|---|---|
| 100 | ch1 | ch4 | `level` |
| 20 | ch1 | ch4 | `piezo_mix` |
| 21 | ch2 | ch5 | `in_gain_a` |
| 21 | ch3 | ch6 | `in_gain_b` |

*4 widgets · acumulado: 4*

## BLOCO 1+2 — drive e shimmer (os dois que têm pergunta específica)

| CC | manda | escuta | param |
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

*9 widgets · acumulado: 13*

## BLOCO 3+4 — delay sozinho e os dois espaços

| CC | manda | escuta | param |
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

*11 widgets · acumulado: 24*

## BLOCO 5 — gate, warble, freeze duplo, búzios

| CC | manda | escuta | param |
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

*12 widgets · acumulado: 36*

## Ordem de ataque

Faz por camada e para quando cobrir o que vais julgar. **A primeira
camada sozinha já permite tocar e ouvir**; as outras destravam blocos
específicos da pauta.


O resto dos 86 remapeia depois, quando houver motivo — a lista
completa está em `RETRABALHO-LOOPY.md`.

