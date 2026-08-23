# Lista para refazer as páginas do Loopy

> **Gerada de `src/manifest.h` (v0.155.0).** Regerar antes de confiar se o
> manifest mudar.

## As duas regras, e não há uma terceira

**1. MANDAR** — cadeia A e compartilhados no **ch1**, cadeia B no **ch2**.

**2. ESCUTAR** (widget que mostra estado: LED, posição de fader) —
cadeia A e compartilhados no **ch3**, cadeia B no **ch4**.

**O número do CC é o mesmo nos quatro canais.** Só o canal diz de quem é
e em que direção.

### O atalho que faz a cadeia B quase de graça

Monte a página da cadeia A no **ch1** e **duplique trocando o campo de
canal para 2**. Os 37 pareados ficam prontos numa operação — é a mesma
página, noutro canal.

### Widget que manda E mostra

Um toggle que envia e reflete (o LED do freeze, por exemplo) manda no
**ch1** e escuta no **ch3**, ao mesmo tempo. O eco dele não desfaz o
próprio gesto: a placa mantém uma **janela de autoria de 250 ms** por
parâmetro, e descarta o retorno dentro dela — contado em `midi`.


## 12 COMPARTILHADOS — só ch1 (escuta ch3). **Não duplicar.**

| CC | param |
|---|---|
| 20 | `piezo_mix` |
| 64 | `dly_time` |
| 90 | `buzios_on` |
| 91 | `buzios_thresh` |
| 92 | `buzios_cooldown` |
| 93 | `tone_freq` |
| 94 | `tone_mix` |
| 95 | `tone_dest` |
| 100 | `level` |
| 112 | `shift_hz` |
| 113 | `route_exciter` |
| 114 | `route_jack` |

## 37 PAREADOS — ch1 = cadeia A · ch2 = cadeia B

O nome abaixo é a família: no firmware são `<nome>_a` e `<nome>_b`, e o
canal é que os separa.


### CENTRAL

| CC | família |
|---|---|
| 21 | `in_gain` |
| 23 | `gate_thresh` |
| 25 | `gate_release` |

### MOD

| CC | família |
|---|---|
| 30 | `drv_on` |
| 31 | `drv_mix` |
| 34 | `drv_drive` |
| 35 | `drv_tone` |
| 36 | `drv_sujeira` |
| 40 | `phs_on` |
| 41 | `phs_mix` |
| 44 | `phs_rate` |
| 45 | `phs_depth` |
| 46 | `phs_fb` |
| 47 | `phs_stages` |
| 50 | `wrb_on` |
| 51 | `wrb_mix` |
| 54 | `wrb_rate` |
| 55 | `wrb_depth` |
| 56 | `wrb_irreg` |

### TIME

| CC | família |
|---|---|
| 60 | `dly_on` |
| 61 | `dly_mix` |
| 65 | `frz_on` |
| 66 | `frz_vol` |
| 67 | `frz_size` |

### TECH

| CC | família |
|---|---|
| 71 | `rev_on` |
| 72 | `rev_mix` |
| 73 | `rev_size` |
| 74 | `rev_damp` |
| 75 | `shm_on` |
| 76 | `shm_mix` |
| 77 | `shm_feedback` |
| 78 | `shm_shift` |
| 79 | `shm_tone` |
| 105 | `exc_on` |

### PROTEÇÃO

| CC | família |
|---|---|
| 110 | `gov_ceiling` |
| 111 | `gov_release` |
| 115 | `bench_boost` |

## Números LIVRES, para o que vier

Reservados dois para os presets (128 slots por cadeia, um CC cada):

```
  preset da cadeia A   ch1 cc101   valor 0..127 = slot
  preset da cadeia B   ch2 cc101   valor 0..127 = slot
```

Mesmo número, canal separando as cadeias — a mesma regra de tudo o mais.


## Ao terminar

```
vistos reset
# ... move TODOS os widgets, um por vez
vistos
```

`FALTA` = não chegou · `ORFAO` = chegou fora do mapa → **conserto no
firmware**. E `sync` confere o outro sentido, saindo de `AVISO` para `OK`
conforme os widgets voltam a escutar.

