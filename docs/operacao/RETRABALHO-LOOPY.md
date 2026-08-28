# Lista de CCs para configurar o Loopy

> **Gerada de `src/manifest.h` — firmware v0.156.0.** Se o mapa mudar,
> regerar antes de confiar.

## A regra: seis canais, e ESCUTA = MANDA + 3

```
MANDA    ch1  compartilhados
         ch2  cadeia A
         ch3  cadeia B

ESCUTA   ch4  compartilhados
         ch5  cadeia A
         ch6  cadeia B
```

**O número do CC é o mesmo nos seis canais.** Só o canal diz de quem é e
em que direção.

**Cada página tem UM assunto**, e é isto que faz a duplicação funcionar:
monte a página da cadeia A no ch2 e duplique trocando o canal para 3 — os
37 saem numa operação, sem nenhum compartilhado junto para apagar.

### Por que todos os widgets devem escutar

Não é conveniência. A placa muda params **sozinha** em três situações, e
widget que não escuta passa a mentir:

- **búzios** sorteiam a cada batida;
- o **CTOS** derruba efeitos quando o teto de soma estoura;
- **`preset load`** troca 33 params de uma vez.

O `sync` empurra o estado e **confere o retorno sozinho** — sai de `AVISO`
para `OK` conforme os widgets voltam a escutar.


## COMPARTILHADOS — 12 · manda **ch1** · escuta **ch4**

Existem uma vez só. **Não duplicar.**

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
| 113 | `out_l` |
| 114 | `out_r` |

## PAREADOS — 37 · manda **ch2**(A)/**ch3**(B) · escuta **ch5**/**ch6**

O nome é a família: no firmware são `<nome>_a` e `<nome>_b`, e o canal os
separa.


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

## Reservado, ainda não implementado

```
preset da cadeia A   ch2 cc101   valor 0..127 = slot
preset da cadeia B   ch3 cc101   valor 0..127 = slot
```

## Ao terminar

```
vistos reset
# ... move TODOS os widgets, um por vez
vistos
```

`FALTA` = não chegou · `ORFAO` = chegou fora do mapa → **conserto no
firmware**, não no widget.

