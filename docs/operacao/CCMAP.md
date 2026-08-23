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

## Endereçamento por canal (voo A, v0.134.0)

    ENTRADA   ch1 -> cadeia A + COMPARTILHADOS
              ch2 -> cadeia B

**O canal é parte do endereço, não filtro.** O CC 34 no ch1 é `drv_drive_a`;
o CC 34 no ch2 é `drv_drive_b`. Um CC que chega num canal sem vínculo **não
move nada** e é contado (`canal sem vinculo` e `(canal,cc) fora do mapa`, no
`midi`).

**As páginas do Loopy ficam idênticas:** copiar a página da cadeia A e trocar
o campo de canal. Os números da cadeia A **não mudaram nenhum**, então a
página que já existe segue valendo.

⚠️ **Números liberados pela cadeia B** — 22, 24, 26, 32, 33, 42, 43, 52, 53,
62, 63, 68, 69, 70, 80–88, 106, 116, 117, 118. Os widgets antigos ainda
apontam para eles e **agora não fazem nada**. O `vistos` os nomeia como
`ORFAO ch1 ccNN`. Não reaproveitar esses números antes de refazer as páginas:
número reaproveitado cedo vira widget velho mexendo em param novo.

## O espelho (canal 2 — interino do voo A)

A placa **transmite** seu estado no canal 2 e **recusa** na entrada tudo que
chega no canal 2 — senão obedeceria a própria voz. No Loopy, o widget envia
no canal 1 e escuta no canal 2.

⚠️ **Colisão declarada, e ela some no voo B:** o ch2 agora é *também* a
entrada da cadeia B. Enquanto o espelho falar nele, a entrada da cadeia B
fica **recusada** — `espelho off` libera, e o `board` diz isso em toda
consulta. Ao desligar, a entrada só abre **300 ms depois**, porque desligar
um transmissor não esvazia o fio: mensagens ainda em voo chegariam como
escrita na cadeia B (o espelho manda o CC 34 com o valor da A, e no ch2 o
CC 34 é a B). No voo B o espelho vai para ch3/ch4 e dá para espelhar e
receber ao mesmo tempo.

**Janela de autoria: 250 ms, indexada por PARÂMETRO.** Era por CC, e com a
multiplexação o CC deixou de identificar o assunto — a janela do drive da
cadeia A silenciaria o da B. Depois de o espelho transmitir um param, a
entrada ignora aquele param por 250 ms. Isso existe porque widget de toggle devolve o
*estado dele*, não a mensagem — e fora de fase devolve o valor oposto,
desfazendo a mudança. Descarte é **contado** (`cc_janela` no `midi`).

## Tabela

<!-- gerado por gerar_ccmap.py de manifest.h — nao editar a mao -->
| CC | canal | param | família | faixa real |
|---|---|---|---|---|
| 20 | ch1 | `piezo_mix` | mistura | -1 – 1 |
| 21 | ch1 | `in_gain_a` | entrada | 1 – 8 |
| 21 | ch2 | `in_gain_b` | entrada | 1 – 8 |
| 23 | ch1 | `gate_thresh_a` | entrada | 0 – 1 |
| 23 | ch2 | `gate_thresh_b` | entrada | 0 – 1 |
| 25 | ch1 | `gate_release_a` | entrada | 50 – 2000 |
| 25 | ch2 | `gate_release_b` | entrada | 50 – 2000 |
| 30 | ch1 | `drv_on_a` | drive | 0 – 1 |
| 30 | ch2 | `drv_on_b` | drive | 0 – 1 |
| 31 | ch1 | `drv_mix_a` | drive | 0 – 1 |
| 31 | ch2 | `drv_mix_b` | drive | 0 – 1 |
| 34 | ch1 | `drv_drive_a` | drive | 1.5 – 48 |
| 34 | ch2 | `drv_drive_b` | drive | 1.5 – 48 |
| 35 | ch1 | `drv_tone_a` | drive | 800 – 16000 |
| 35 | ch2 | `drv_tone_b` | drive | 800 – 16000 |
| 36 | ch1 | `drv_sujeira_a` | drive | 0 – 1 |
| 36 | ch2 | `drv_sujeira_b` | drive | 0 – 1 |
| 40 | ch1 | `phs_on_a` | phaser | 0 – 1 |
| 40 | ch2 | `phs_on_b` | phaser | 0 – 1 |
| 41 | ch1 | `phs_mix_a` | phaser | 0 – 1 |
| 41 | ch2 | `phs_mix_b` | phaser | 0 – 1 |
| 44 | ch1 | `phs_rate_a` | phaser | 0.02 – 4 |
| 44 | ch2 | `phs_rate_b` | phaser | 0.02 – 4 |
| 45 | ch1 | `phs_depth_a` | phaser | 0 – 1 |
| 45 | ch2 | `phs_depth_b` | phaser | 0 – 1 |
| 46 | ch1 | `phs_fb_a` | phaser | 0 – 1 |
| 46 | ch2 | `phs_fb_b` | phaser | 0 – 1 |
| 47 | ch1 | `phs_stages_a` | phaser | 4 – 8 |
| 47 | ch2 | `phs_stages_b` | phaser | 4 – 8 |
| 50 | ch1 | `wrb_on_a` | warble | 0 – 1 |
| 50 | ch2 | `wrb_on_b` | warble | 0 – 1 |
| 51 | ch1 | `wrb_mix_a` | warble | 0 – 1 |
| 51 | ch2 | `wrb_mix_b` | warble | 0 – 1 |
| 54 | ch1 | `wrb_rate_a` | warble | 0.05 – 3 |
| 54 | ch2 | `wrb_rate_b` | warble | 0.05 – 3 |
| 55 | ch1 | `wrb_depth_a` | warble | 0 – 3 |
| 55 | ch2 | `wrb_depth_b` | warble | 0 – 3 |
| 56 | ch1 | `wrb_irreg_a` | warble | 0 – 1 |
| 56 | ch2 | `wrb_irreg_b` | warble | 0 – 1 |
| 60 | ch1 | `dly_on_a` | delay | 0 – 1 |
| 60 | ch2 | `dly_on_b` | delay | 0 – 1 |
| 61 | ch1 | `dly_mix_a` | delay | 0 – 1 |
| 61 | ch2 | `dly_mix_b` | delay | 0 – 1 |
| 64 | ch1 | `dly_time` | delay | 0 – 1 |
| 65 | ch1 | `frz_on_a` | freeze | 0 – 1 |
| 65 | ch2 | `frz_on_b` | freeze | 0 – 1 |
| 66 | ch1 | `frz_vol_a` | freeze | 0 – 1 |
| 66 | ch2 | `frz_vol_b` | freeze | 0 – 1 |
| 67 | ch1 | `frz_size_a` | freeze | 20 – 165 |
| 67 | ch2 | `frz_size_b` | freeze | 20 – 165 |
| 71 | ch1 | `rev_on_a` | reverb | 0 – 1 |
| 71 | ch2 | `rev_on_b` | reverb | 0 – 1 |
| 72 | ch1 | `rev_mix_a` | reverb | 0 – 1 |
| 72 | ch2 | `rev_mix_b` | reverb | 0 – 1 |
| 73 | ch1 | `rev_size_a` | reverb | 0 – 1 |
| 73 | ch2 | `rev_size_b` | reverb | 0 – 1 |
| 74 | ch1 | `rev_damp_a` | reverb | 0 – 1 |
| 74 | ch2 | `rev_damp_b` | reverb | 0 – 1 |
| 75 | ch1 | `shm_on_a` | shimmer | 0 – 1 |
| 75 | ch2 | `shm_on_b` | shimmer | 0 – 1 |
| 76 | ch1 | `shm_mix_a` | shimmer | 0 – 1 |
| 76 | ch2 | `shm_mix_b` | shimmer | 0 – 1 |
| 77 | ch1 | `shm_feedback_a` | shimmer | 0 – 1 |
| 77 | ch2 | `shm_feedback_b` | shimmer | 0 – 1 |
| 78 | ch1 | `shm_shift_a` | shimmer | 0 – 2 |
| 78 | ch2 | `shm_shift_b` | shimmer | 0 – 2 |
| 79 | ch1 | `shm_tone_a` | shimmer | 0 – 1 |
| 79 | ch2 | `shm_tone_b` | shimmer | 0 – 1 |
| 90 | ch1 | `buzios_on` | búzios | 0 – 1 |
| 91 | ch1 | `buzios_thresh` | búzios | 0 – 1 |
| 92 | ch1 | `buzios_cooldown` | búzios | 10 – 1000 |
| 93 | ch1 | `tone_freq` | tom | 80 – 2000 |
| 94 | ch1 | `tone_mix` | tom | 0 – 1 |
| 95 | ch1 | `tone_dest` | tom | 0 – 2 |
| 100 | ch1 | `level` | saída | 0 – 1 |
| 105 | ch1 | `exc_on_a` | saída | 0 – 1 |
| 105 | ch2 | `exc_on_b` | saída | 0 – 1 |
| 110 | ch1 | `gov_ceiling_a` | proteção | 0 – 1 |
| 110 | ch2 | `gov_ceiling_b` | proteção | 0 – 1 |
| 111 | ch1 | `gov_release_a` | proteção | 100 – 3000 |
| 111 | ch2 | `gov_release_b` | proteção | 100 – 3000 |
| 112 | ch1 | `shift_hz` | proteção | 0 – 10 |
| 113 | ch1 | `route_exciter` | saída | 0 – 1 |
| 114 | ch1 | `route_jack` | saída | 0 – 1 |
| 115 | ch1 | `bench_boost_a` | bancada | 1 – 8 |
| 115 | ch2 | `bench_boost_b` | bancada | 1 – 8 |

<!-- 86 vinculos em 49 numeros de CC -->

## Faixas vazias no mapa

`27–29`, `37–39`, `48–49`, `57–59`, `89`, `96–99`, `101–104`, `107–109`,
`119+` — reservadas para crescimento por família. **Página de proteção
(11 CCs) ainda não construída no Loopy.**
