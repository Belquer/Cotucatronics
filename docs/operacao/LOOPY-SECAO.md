# Loopy Pro — secção nova, com MIDI learn

**Gerado por `gerar_secao_loopy.py v0.3.0` a partir do `manifest.h`, manifest v3.0.0.** Não editar à mão — correr o gerador.

⚠️ **Isto é a ORDEM DE APRENDIZAGEM, não o layout.** O Loopy endereça widgets por índice de array dentro da cena, e esse índice só existe depois de alguém os criar. Cria os widgets na ordem abaixo e faz `learn` um a um — assim o mapa fica igual ao documento.

## Regra dos canais, e ela é o endereço

| canal | destino |
|---|---|
| **ch1** | parâmetros compartilhados — e só eles |
| **ch2** | cadeia A |
| **ch3** | cadeia B |

**Escuta = manda + 3.** O mesmo número de CC no ch4/ch5/ch6 é o retorno do instrumento. Widget que só manda usa ch1–ch3; widget que também ouve aprende o par.

⚠️ **O mesmo CC em canais diferentes são parâmetros DIFERENTES.** Uma tabela sem a coluna de canal descreve metade do instrumento.

## NIVEIS — antes e depois do limitador

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **100** | ch1 (compartilhado) | `pre_level_ab` | Fader | 0 – 1 | 1 |
| **116** | ch1 (compartilhado) | `post_level_l` | Fader | 0 – 1 | 1 |
| **117** | ch1 (compartilhado) | `post_level_r` | Fader | 0 – 1 | 1 |

## TOM DE TESTE

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **93** | ch1 (compartilhado) | `test_tone_freq` | Fader | 80 – 2000 | 0.4 |
| **94** | ch1 (compartilhado) | `test_tone_level` | Fader | 0 – 1 | 0 |
| **95** | ch1 (compartilhado) | `test_tone_out` | Botao x3 (passos) | 0 – 2 | 0 |
| **96** | ch1 (compartilhado) | `test_tone_via` | Botao (toggle) | 0 – 1 | 0 |

## GATE

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **23** | ch2 (cadeia A) | `gate_thresh_a` | Fader | 0 – 1 | 0 |
| **23** | ch3 (cadeia B) | `gate_thresh_b` | Fader | 0 – 1 | 0 |
| **25** | ch2 (cadeia A) | `gate_release_a` | Fader | 50 – 2000 | 0.1282 |
| **25** | ch3 (cadeia B) | `gate_release_b` | Fader | 50 – 2000 | 0.1282 |

## DRIVE

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **30** | ch2 (cadeia A) | `drv_on_a` | Botao (toggle) | 0 – 1 | 0 |
| **30** | ch3 (cadeia B) | `drv_on_b` | Botao (toggle) | 0 – 1 | 0 |
| **31** | ch2 (cadeia A) | `drv_mix_a` | Fader | 0 – 1 | 0 |
| **31** | ch3 (cadeia B) | `drv_mix_b` | Fader | 0 – 1 | 0 |
| **34** | ch2 (cadeia A) | `drv_drive_a` | Fader | 1.5 – 48 | 0 |
| **34** | ch3 (cadeia B) | `drv_drive_b` | Fader | 1.5 – 48 | 0 |
| **35** | ch2 (cadeia A) | `drv_tone_a` | Fader | 800 – 16000 | 0 |
| **35** | ch3 (cadeia B) | `drv_tone_b` | Fader | 800 – 16000 | 0 |
| **36** | ch2 (cadeia A) | `drv_sujeira_a` | Fader | 0 – 1 | 0 |
| **36** | ch3 (cadeia B) | `drv_sujeira_b` | Fader | 0 – 1 | 0 |

## PHASER

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **40** | ch2 (cadeia A) | `phs_on_a` | Botao (toggle) | 0 – 1 | 0 |
| **40** | ch3 (cadeia B) | `phs_on_b` | Botao (toggle) | 0 – 1 | 0 |
| **41** | ch2 (cadeia A) | `phs_mix_a` | Fader | 0 – 1 | 0 |
| **41** | ch3 (cadeia B) | `phs_mix_b` | Fader | 0 – 1 | 0 |
| **44** | ch2 (cadeia A) | `phs_rate_a` | Fader | 0.02 – 4 | 0 |
| **44** | ch3 (cadeia B) | `phs_rate_b` | Fader | 0.02 – 4 | 0 |
| **45** | ch2 (cadeia A) | `phs_depth_a` | Fader | 0 – 1 | 0 |
| **45** | ch3 (cadeia B) | `phs_depth_b` | Fader | 0 – 1 | 0 |
| **46** | ch2 (cadeia A) | `phs_fb_a` | Fader | 0 – 1 | 0 |
| **46** | ch3 (cadeia B) | `phs_fb_b` | Fader | 0 – 1 | 0 |
| **47** | ch2 (cadeia A) | `phs_stages_a` | Botao x3 (passos) | 4 – 8 | 0 |
| **47** | ch3 (cadeia B) | `phs_stages_b` | Botao x3 (passos) | 4 – 8 | 0 |

## WARBLE

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **50** | ch2 (cadeia A) | `wrb_on_a` | Botao (toggle) | 0 – 1 | 0 |
| **50** | ch3 (cadeia B) | `wrb_on_b` | Botao (toggle) | 0 – 1 | 0 |
| **51** | ch2 (cadeia A) | `wrb_mix_a` | Fader | 0 – 1 | 0 |
| **51** | ch3 (cadeia B) | `wrb_mix_b` | Fader | 0 – 1 | 0 |
| **54** | ch2 (cadeia A) | `wrb_rate_a` | Fader | 0.5 – 12 | 0 |
| **54** | ch3 (cadeia B) | `wrb_rate_b` | Fader | 0.5 – 12 | 0 |
| **55** | ch2 (cadeia A) | `wrb_depth_a` | Fader | 0 – 3 | 0 |
| **55** | ch3 (cadeia B) | `wrb_depth_b` | Fader | 0 – 3 | 0 |
| **56** | ch2 (cadeia A) | `wrb_irreg_a` | Fader | 0 – 1 | 0 |
| **56** | ch3 (cadeia B) | `wrb_irreg_b` | Fader | 0 – 1 | 0 |

## TAROLATOR

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **80** | ch2 (cadeia A) | `tarol_on_a` | Botao (toggle) | 0 – 1 | 0 |
| **80** | ch3 (cadeia B) | `tarol_on_b` | Botao (toggle) | 0 – 1 | 0 |
| **81** | ch2 (cadeia A) | `tarol_mix_a` | Fader | 0 – 4 | 0.5 |
| **81** | ch3 (cadeia B) | `tarol_mix_b` | Fader | 0 – 4 | 0.5 |

## FREEZE

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **65** | ch2 (cadeia A) | `frz_on_a` | Botao (toggle) | 0 – 1 | 0 |
| **65** | ch3 (cadeia B) | `frz_on_b` | Botao (toggle) | 0 – 1 | 0 |
| **66** | ch2 (cadeia A) | `frz_vol_a` | Fader | 0 – 1 | 0 |
| **66** | ch3 (cadeia B) | `frz_vol_b` | Fader | 0 – 1 | 0 |
| **67** | ch2 (cadeia A) | `frz_size_a` | Fader | 20 – 165 | 0 |
| **67** | ch3 (cadeia B) | `frz_size_b` | Fader | 20 – 165 | 0 |

## REVERB

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **71** | ch2 (cadeia A) | `rev_on_a` | Botao (toggle) | 0 – 1 | 0 |
| **71** | ch3 (cadeia B) | `rev_on_b` | Botao (toggle) | 0 – 1 | 0 |
| **72** | ch2 (cadeia A) | `rev_mix_a` | Fader | 0 – 1 | 0 |
| **72** | ch3 (cadeia B) | `rev_mix_b` | Fader | 0 – 1 | 0 |
| **73** | ch2 (cadeia A) | `rev_size_a` | Fader | 0 – 1 | 0.5 |
| **73** | ch3 (cadeia B) | `rev_size_b` | Fader | 0 – 1 | 0.5 |
| **74** | ch2 (cadeia A) | `rev_damp_a` | Fader | 0 – 1 | 0.5 |
| **74** | ch3 (cadeia B) | `rev_damp_b` | Fader | 0 – 1 | 0.5 |

## SHIMMER

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **75** | ch2 (cadeia A) | `shm_on_a` | Botao (toggle) | 0 – 1 | 0 |
| **75** | ch3 (cadeia B) | `shm_on_b` | Botao (toggle) | 0 – 1 | 0 |
| **76** | ch2 (cadeia A) | `shm_mix_a` | Fader | 0 – 1 | 0 |
| **76** | ch3 (cadeia B) | `shm_mix_b` | Fader | 0 – 1 | 0 |
| **77** | ch2 (cadeia A) | `shm_feedback_a` | Fader | 0 – 1 | 0 |
| **77** | ch3 (cadeia B) | `shm_feedback_b` | Fader | 0 – 1 | 0 |
| **78** | ch2 (cadeia A) | `shm_shift_a` | Botao x3 (passos) | 0 – 2 | 0 |
| **78** | ch3 (cadeia B) | `shm_shift_b` | Botao x3 (passos) | 0 – 2 | 0 |
| **79** | ch2 (cadeia A) | `shm_tone_a` | Fader | 0 – 1 | 0 |
| **79** | ch3 (cadeia B) | `shm_tone_b` | Fader | 0 – 1 | 0 |

## DELAY

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **60** | ch2 (cadeia A) | `dly_on_a` | Botao (toggle) | 0 – 1 | 0 |
| **60** | ch3 (cadeia B) | `dly_on_b` | Botao (toggle) | 0 – 1 | 0 |
| **61** | ch2 (cadeia A) | `dly_mix_a` | Fader | 0 – 1 | 0 |
| **61** | ch3 (cadeia B) | `dly_mix_b` | Fader | 0 – 1 | 0 |
| **64** | ch1 (compartilhado) | `dly_time` | Fader | 0 – 1 | 0 |

## FONTE

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **20** | ch1 (compartilhado) | `piezo_mix` | Fader | -1 – 1 | 0.5 |

## GOVERNOR

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **101** | ch1 (compartilhado) | `gov_ceiling_l` | Fader | 0 – 1 | 0.85 |
| **102** | ch1 (compartilhado) | `gov_ceiling_r` | Fader | 0 – 1 | 0.85 |
| **103** | ch1 (compartilhado) | `gov_release_l` | Fader | 100 – 3000 | 0.2414 |
| **104** | ch1 (compartilhado) | `gov_release_r` | Fader | 100 – 3000 | 0.2414 |

## BUZIOS — sorteio

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **90** | ch1 (compartilhado) | `buzios_on` | Botao (toggle) | 0 – 1 | 0 |
| **91** | ch1 (compartilhado) | `buzios_thresh` | Fader | 0 – 1 | 0.04 |
| **92** | ch1 (compartilhado) | `buzios_cooldown` | Fader | 10 – 1000 | 0.091 |

## SHIFT

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **112** | ch1 (compartilhado) | `shift_hz` | Fader | 0 – 10 | 0 |

## BANCADA — nao vai para o painel

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **105** | ch1 (compartilhado) | `bench_boost_l` | Fader | 1 – 8 | 0 |
| **106** | ch1 (compartilhado) | `bench_boost_r` | Fader | 1 – 8 | 0 |

## OUTROS

| CC | canal | parâmetro | widget | faixa | nasce |
|---|---|---|---|---|---|
| **1** | ch1 (compartilhado) | `microfonia_on_l` | Botao (toggle) | 0 – 1 | 0 |
| **2** | ch1 (compartilhado) | `microfonia_on_r` | Botao (toggle) | 0 – 1 | 0 |
| **3** | ch1 (compartilhado) | `microfonia1_hz_l` | Fader | 60 – 8000 | 0 |
| **4** | ch1 (compartilhado) | `microfonia1_hz_r` | Fader | 60 – 8000 | 0 |
| **5** | ch1 (compartilhado) | `microfonia1_db_l` | Fader | 0 – 24 | 0 |
| **6** | ch1 (compartilhado) | `microfonia1_db_r` | Fader | 0 – 24 | 0 |
| **7** | ch1 (compartilhado) | `microfonia2_hz_l` | Fader | 60 – 8000 | 0 |
| **8** | ch1 (compartilhado) | `microfonia2_hz_r` | Fader | 60 – 8000 | 0 |
| **9** | ch1 (compartilhado) | `microfonia2_db_l` | Fader | 0 – 24 | 0 |
| **10** | ch1 (compartilhado) | `microfonia2_db_r` | Fader | 0 – 24 | 0 |
| **11** | ch1 (compartilhado) | `microfonia3_hz_l` | Fader | 60 – 8000 | 0 |
| **12** | ch1 (compartilhado) | `microfonia3_hz_r` | Fader | 60 – 8000 | 0 |
| **13** | ch1 (compartilhado) | `microfonia3_db_l` | Fader | 0 – 24 | 0 |
| **14** | ch1 (compartilhado) | `microfonia3_db_r` | Fader | 0 – 24 | 0 |
| **15** | ch1 (compartilhado) | `microfonia4_hz_l` | Fader | 60 – 8000 | 0 |
| **16** | ch1 (compartilhado) | `microfonia4_hz_r` | Fader | 60 – 8000 | 0 |
| **17** | ch1 (compartilhado) | `microfonia4_db_l` | Fader | 0 – 24 | 0 |
| **18** | ch1 (compartilhado) | `microfonia4_db_r` | Fader | 0 – 24 | 0 |
| **19** | ch1 (compartilhado) | `micro_auto` | Fader | 0 – 1 | 1 |
| **21** | ch1 (compartilhado) | `micro_dom` | Fader | 0 – 1 | 0.5 |
| **22** | ch1 (compartilhado) | `comp_on` | Botao (toggle) | 0 – 1 | 0 |
| **23** | ch1 (compartilhado) | `comp_thresh` | Fader | 0.001 – 1 | 0.05 |
| **24** | ch1 (compartilhado) | `comp_razao` | Fader | 1 – 20 | 0.3 |
| **25** | ch1 (compartilhado) | `comp_makeup` | Fader | 1 – 8 | 0 |
| **107** | ch1 (compartilhado) | `sim_laco` | Fader | 0 – 1 | 0 |
| **108** | ch1 (compartilhado) | `sim_hz` | Fader | 60 – 8000 | 0 |
| **109** | ch1 (compartilhado) | `sim_fb` | Fader | 0 – 2 | 0 |

## Página de PRESETS

**CC 121 = carregar.** O canal diz o destino e o valor diz o slot — a mesma gramática do resto.

| widget | CC | canal | valor | efeito |
|---|---|---|---|---|
| Preset global 1..8 | **121** | ch1 | 0..7 | carrega o slot global |
| Preset cadeia A 1..16 | **121** | ch2 | 0..15 | carrega na cadeia A |
| Preset cadeia B 1..16 | **121** | ch3 | 0..15 | carrega na cadeia B |

São **botões de valor fixo**, não faders: cada botão manda o CC 121 com o seu número de slot. No Loopy, o binding fica `B0 79 <slot>` (ch1), `B1 79 <slot>` (ch2), `B2 79 <slot>` (ch3) — `79` é 121 em hexadecimal.

**Slot vazio responde.** A placa diz `preset CC: slot N ... VAZIO` no log. Um botão de palco que não faz nada é indistinguível de um botão que não chegou, e as duas coisas pedem consertos opostos.

🚫 **CC 122 (gravar) NÃO existe, e a omissão é deliberada.** Um CC que escreve preset destrói uma biblioteca com uma mensagem perdida, e não traz nome nenhum para lhe dar. Capturar fica no serial — `capturar <a|b|g> <nome>` — onde o gesto é deliberado e o preset nasce com nome.

⚠️ **Os presets vivem em RAM.** Desliga da tomada e somem. `preset dump` guarda no computador enquanto a escrita na flash não destrava.

⚠️ **Quantização em 7 bits.** O preset guarda 0..127, como o MIDI. Medido em 01/09: um notch capturado a 834 Hz volta a 847,6 Hz — a 834, com curva 2.0 sobre 60–8000, um passo vale ~14 Hz. O notch tem 24 Hz de largura e ainda cobre o modo, mas ajuste mais fino que 1/127 não sobrevive à captura.

---

**114 vínculos** nesta folha.
