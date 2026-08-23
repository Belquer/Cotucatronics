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

## O espelho (canal 2)

A placa **transmite** seu estado no canal 2 e **recusa** na entrada tudo que
chega no canal 2 — senão obedeceria a própria voz. No Loopy, o widget envia
no canal 1 e escuta no canal 2.

**Janela de autoria: 250 ms.** Depois de o espelho transmitir um CC, a entrada
ignora aquele CC por 250 ms. Isso existe porque widget de toggle devolve o
*estado dele*, não a mensagem — e fora de fase devolve o valor oposto,
desfazendo a mudança. Descarte é **contado** (`cc_janela` no `midi`).

## Tabela

<!-- gerado por gerar_ccmap.py de manifest.h — nao editar a mao -->
| CC | param | família | faixa real |
|---|---|---|---|
| 20 | `piezo_mix` | mistura | -1 – 1 |
| 21 | `in_gain_a` | entrada | 1 – 8 |
| 22 | `in_gain_b` | entrada | 1 – 8 |
| 23 | `gate_thresh_a` | entrada | 0 – 1 |
| 24 | `gate_thresh_b` | entrada | 0 – 1 |
| 25 | `gate_release_a` | entrada | 50 – 2000 |
| 26 | `gate_release_b` | entrada | 50 – 2000 |
| 30 | `drv_on_a` | drive | 0 – 1 |
| 31 | `drv_mix_a` | drive | 0 – 1 |
| 32 | `drv_on_b` | drive | 0 – 1 |
| 33 | `drv_mix_b` | drive | 0 – 1 |
| 34 | `drv_drive` | drive | 1.5 – 48 |
| 35 | `drv_tone` | drive | 800 – 16000 |
| 36 | `drv_sujeira` | drive | 0 – 1 |
| 40 | `phs_on_a` | phaser | 0 – 1 |
| 41 | `phs_mix_a` | phaser | 0 – 1 |
| 42 | `phs_on_b` | phaser | 0 – 1 |
| 43 | `phs_mix_b` | phaser | 0 – 1 |
| 44 | `phs_rate` | phaser | 0.02 – 4 |
| 45 | `phs_depth` | phaser | 0 – 1 |
| 46 | `phs_fb` | phaser | 0 – 1 |
| 47 | `phs_stages` | phaser | 4 – 8 |
| 50 | `wrb_on_a` | warble | 0 – 1 |
| 51 | `wrb_mix_a` | warble | 0 – 1 |
| 52 | `wrb_on_b` | warble | 0 – 1 |
| 53 | `wrb_mix_b` | warble | 0 – 1 |
| 54 | `wrb_rate` | warble | 0.05 – 3 |
| 55 | `wrb_depth` | warble | 0 – 3 |
| 56 | `wrb_irreg` | warble | 0 – 1 |
| 60 | `dly_on_a` | delay | 0 – 1 |
| 61 | `dly_mix_a` | delay | 0 – 1 |
| 62 | `dly_on_b` | delay | 0 – 1 |
| 63 | `dly_mix_b` | delay | 0 – 1 |
| 64 | `dly_time` | delay | 0 – 1 |
| 65 | `frz_on_a` | freeze | 0 – 1 |
| 66 | `frz_vol_a` | freeze | 0 – 1 |
| 67 | `frz_size_a` | freeze | 20 – 165 |
| 68 | `frz_on_b` | freeze | 0 – 1 |
| 69 | `frz_vol_b` | freeze | 0 – 1 |
| 70 | `frz_size_b` | freeze | 20 – 165 |
| 71 | `rev_on_a` | reverb | 0 – 1 |
| 72 | `rev_mix_a` | reverb | 0 – 1 |
| 73 | `rev_size_a` | reverb | 0 – 1 |
| 74 | `rev_damp_a` | reverb | 0 – 1 |
| 75 | `shm_on_a` | shimmer | 0 – 1 |
| 76 | `shm_mix_a` | shimmer | 0 – 1 |
| 77 | `shm_feedback_a` | shimmer | 0 – 1 |
| 78 | `shm_shift_a` | shimmer | 0 – 2 |
| 79 | `shm_tone_a` | shimmer | 0 – 1 |
| 80 | `rev_on_b` | reverb | 0 – 1 |
| 81 | `rev_mix_b` | reverb | 0 – 1 |
| 82 | `rev_size_b` | reverb | 0 – 1 |
| 83 | `rev_damp_b` | reverb | 0 – 1 |
| 84 | `shm_on_b` | shimmer | 0 – 1 |
| 85 | `shm_mix_b` | shimmer | 0 – 1 |
| 86 | `shm_feedback_b` | shimmer | 0 – 1 |
| 87 | `shm_shift_b` | shimmer | 0 – 2 |
| 88 | `shm_tone_b` | shimmer | 0 – 1 |
| 90 | `buzios_on` | búzios | 0 – 1 |
| 91 | `buzios_thresh` | búzios | 0 – 1 |
| 92 | `buzios_cooldown` | búzios | 10 – 1000 |
| 93 | `tone_freq` | tom | 80 – 2000 |
| 94 | `tone_mix` | tom | 0 – 1 |
| 95 | `tone_dest` | tom | 0 – 2 |
| 100 | `level` | saída | 0 – 1 |
| 105 | `exc_on_a` | saída | 0 – 1 |
| 106 | `exc_on_b` | saída | 0 – 1 |
| 110 | `gov_ceiling_a` | proteção | 0 – 1 |
| 111 | `gov_release_a` | proteção | 100 – 3000 |
| 112 | `shift_hz` | proteção | 0 – 10 |
| 113 | `route_exciter` | saída | 0 – 1 |
| 114 | `route_jack` | saída | 0 – 1 |
| 115 | `bench_boost_a` | bancada | 1 – 8 |
| 116 | `gov_ceiling_b` | proteção | 0 – 1 |
| 117 | `gov_release_b` | proteção | 100 – 3000 |
| 118 | `bench_boost_b` | bancada | 1 – 8 |

<!-- 76 CCs -->

## Faixas vazias no mapa

`27–29`, `37–39`, `48–49`, `57–59`, `89`, `96–99`, `101–104`, `107–109`,
`119+` — reservadas para crescimento por família. **Página de proteção
(11 CCs) ainda não construída no Loopy.**
