# CC MAP — Cotucatronics
<!-- CARIMBO gerar_ccmap -->
> **GERADO do `src/manifest.h`** por `gerar_ccmap.py v0.10.0` em **2026-09-04 17:23 EDT**.
> Nao editar a mao: rode `python3 docs/operacao/gerar_ccmap.py`.
>
> **119 vinculos em 81 numeros de CC** · `src/manifest.h` @ `f4345e2` · manifest v1.12.1
>
> Seis canais: manda `ch1`/`ch2`/`ch3`, escuta `ch4`/`ch5`/`ch6` (escuta = manda + 3).
>
> ⚠️ **ARVORE SUJA** — `src/manifest.h` tem alteracoes por commitar.
> Este mapa descreve codigo que mais ninguem tem.
<!-- FIM CARIMBO -->


## Como ler

- **`_a` / `_b`** = cadeia A (piezo 1) e cadeia B (piezo 2). Cada cadeia tem
  entrada, caráter e **espaço próprios**.
- **Sem sufixo** = compartilhado entre as duas cadeias.
- **Faixa** é em unidade real (o que o `set` aceita). O CC 0–127 é mapeado
  nela pela curva do param.

## Os SEIS canais — escuta = manda + 3

    MANDA    ch1  compartilhados
             ch2  cadeia A
             ch3  cadeia B

    ESCUTA   ch4  compartilhados
             ch5  cadeia A
             ch6  cadeia B

**O número do CC é o mesmo nos seis.** Só o canal diz de quem é e em que
direção — e a regra é aritmética, não uma tabela para decorar.

**Cada página tem UM assunto**, e é isso que faz a duplicação funcionar: monte
a página da cadeia A no ch2 e duplique trocando o canal para 3. Os 37 pareados
saem numa operação, sem nenhum compartilhado junto para apagar.

⚠️ **O canal é parte do endereço, não filtro.** O CC 34 no ch2 é
`drv_drive_a`; no ch3 é `drv_drive_b`. Um CC que chega num canal sem vínculo
**não move nada** e é contado (`canal sem vinculo` e `(canal,cc) fora do mapa`,
no `midi`).

**Todo widget deve escutar**, e não é conveniência: a placa muda params
**sozinha** em três situações — os búzios sorteiam a cada batida, o CTOS
derruba efeitos quando o teto de soma estoura, e `preset load` troca 33 de uma
vez. Widget que não escuta passa a mentir depois de qualquer uma delas.

**O `sync` confere o retorno sozinho** e sai de `AVISO` para `OK` conforme os
widgets voltam a escutar.

**Janela de autoria: 250 ms, indexada por PARÂMETRO.** Era por CC, e com a
multiplexação o CC deixou de identificar o assunto. Descarte é contado
(`cc_janela`).

## Tabela

<!-- gerado por gerar_ccmap v0.10.0 de manifest.h — nao editar a mao -->
| CC | manda | escuta | param | família | faixa real |
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
| 112 | ch1 | ch4 | `shift_hz` | proteção | 0 – 10 |
| 113 | ch1 | ch4 | `out_l` | saída | 0 – 1 |
| 114 | ch1 | ch4 | `out_r` | saída | 0 – 1 |
| 116 | ch1 | ch4 | `post_level_l` | — | 0 – 1 |
| 117 | ch1 | ch4 | `post_level_r` | — | 0 – 1 |
| 118 | ch1 | ch4 | `exc_filter_l` | saída | 0 – 1 |
| 119 | ch1 | ch4 | `exc_filter_r` | saída | 0 – 1 |

<!-- 119 vinculos em 81 numeros de CC -->
>
> Seis canais: manda `ch1`/`ch2`/`ch3`, escuta `ch4`/`ch5`/`ch6` (escuta = manda + 3).
>
> ⚠️ **ARVORE SUJA** — `src/manifest.h` tem alteracoes por commitar.
> Este mapa descreve codigo que mais ninguem tem.
<!-- FIM CARIMBO -->


## Como ler

- **`_a` / `_b`** = cadeia A (piezo 1) e cadeia B (piezo 2). Cada cadeia tem
  entrada, caráter e **espaço próprios**.
- **Sem sufixo** = compartilhado entre as duas cadeias.
- **Faixa** é em unidade real (o que o `set` aceita). O CC 0–127 é mapeado
  nela pela curva do param.

## Os SEIS canais — escuta = manda + 3

    MANDA    ch1  compartilhados
             ch2  cadeia A
             ch3  cadeia B

    ESCUTA   ch4  compartilhados
             ch5  cadeia A
             ch6  cadeia B

**O número do CC é o mesmo nos seis.** Só o canal diz de quem é e em que
direção — e a regra é aritmética, não uma tabela para decorar.

**Cada página tem UM assunto**, e é isso que faz a duplicação funcionar: monte
a página da cadeia A no ch2 e duplique trocando o canal para 3. Os 37 pareados
saem numa operação, sem nenhum compartilhado junto para apagar.

⚠️ **O canal é parte do endereço, não filtro.** O CC 34 no ch2 é
`drv_drive_a`; no ch3 é `drv_drive_b`. Um CC que chega num canal sem vínculo
**não move nada** e é contado (`canal sem vinculo` e `(canal,cc) fora do mapa`,
no `midi`).

**Todo widget deve escutar**, e não é conveniência: a placa muda params
**sozinha** em três situações — os búzios sorteiam a cada batida, o CTOS
derruba efeitos quando o teto de soma estoura, e `preset load` troca 33 de uma
vez. Widget que não escuta passa a mentir depois de qualquer uma delas.

**O `sync` confere o retorno sozinho** e sai de `AVISO` para `OK` conforme os
widgets voltam a escutar.

**Janela de autoria: 250 ms, indexada por PARÂMETRO.** Era por CC, e com a
multiplexação o CC deixou de identificar o assunto. Descarte é contado
(`cc_janela`).

## Tabela

<!-- gerado por gerar_ccmap v0.10.0 de manifest.h — nao editar a mao -->
| CC | manda | escuta | param | família | faixa real |
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
| 112 | ch1 | ch4 | `shift_hz` | proteção | 0 – 10 |
| 113 | ch1 | ch4 | `out_l` | saída | 0 – 1 |
| 114 | ch1 | ch4 | `out_r` | saída | 0 – 1 |
| 116 | ch1 | ch4 | `post_level_l` | — | 0 – 1 |
| 117 | ch1 | ch4 | `post_level_r` | — | 0 – 1 |
| 118 | ch1 | ch4 | `exc_filter_l` | saída | 0 – 1 |
| 119 | ch1 | ch4 | `exc_filter_r` | saída | 0 – 1 |

<!-- 119 vinculos em 81 numeros de CC -->
