# CC MAP — Cotucatronics

> **GERADO do `src/manifest.h`.** Não editar à mão: rode `python3 gerar_ccmap.py`.
> Estado atual: **76 CCs**, canal MIDI **1** na entrada, **canal 2** no espelho.
> Firmware v0.130.0 · commit `6002a70`

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

<!-- gerado por gerar_ccmap.py de manifest.h — nao editar a mao -->
| CC | manda | escuta | param | família | faixa real |
|---|---|---|---|---|---|
| 20 | ch1 | ch4 | `piezo_mix` | mistura | -1 – 1 |
| 21 | ch2 | ch5 | `in_gain_a` | entrada | 1 – 8 |
| 21 | ch3 | ch6 | `in_gain_b` | entrada | 1 – 8 |
| 23 | ch2 | ch5 | `gate_thresh_a` | entrada | 0 – 1 |
| 23 | ch3 | ch6 | `gate_thresh_b` | entrada | 0 – 1 |
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
| 54 | ch2 | ch5 | `wrb_rate_a` | warble | 0.05 – 3 |
| 54 | ch3 | ch6 | `wrb_rate_b` | warble | 0.05 – 3 |
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
| 90 | ch1 | ch4 | `buzios_on` | búzios | 0 – 1 |
| 91 | ch1 | ch4 | `buzios_thresh` | búzios | 0 – 1 |
| 92 | ch1 | ch4 | `buzios_cooldown` | búzios | 10 – 1000 |
| 93 | ch1 | ch4 | `tone_freq` | tom | 80 – 2000 |
| 94 | ch1 | ch4 | `tone_mix` | tom | 0 – 1 |
| 95 | ch1 | ch4 | `tone_dest` | tom | 0 – 2 |
| 100 | ch1 | ch4 | `level` | saída | 0 – 1 |
| 105 | ch2 | ch5 | `exc_on_a` | saída | 0 – 1 |
| 105 | ch3 | ch6 | `exc_on_b` | saída | 0 – 1 |
| 110 | ch2 | ch5 | `gov_ceiling_a` | proteção | 0 – 1 |
| 110 | ch3 | ch6 | `gov_ceiling_b` | proteção | 0 – 1 |
| 111 | ch2 | ch5 | `gov_release_a` | proteção | 100 – 3000 |
| 111 | ch3 | ch6 | `gov_release_b` | proteção | 100 – 3000 |
| 112 | ch1 | ch4 | `shift_hz` | proteção | 0 – 10 |
| 113 | ch1 | ch4 | `route_exciter` | saída | 0 – 1 |
| 114 | ch1 | ch4 | `route_jack` | saída | 0 – 1 |
| 115 | ch2 | ch5 | `bench_boost_a` | bancada | 1 – 8 |
| 115 | ch3 | ch6 | `bench_boost_b` | bancada | 1 – 8 |

<!-- 86 vinculos em 49 numeros de CC -->

## Faixas vazias no mapa

`27–29`, `37–39`, `48–49`, `57–59`, `89`, `96–99`, `101–104`, `107–109`,
`119+` — reservadas para crescimento por família. **Página de proteção
(11 CCs) ainda não construída no Loopy.**
