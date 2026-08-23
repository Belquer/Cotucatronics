# ARQUITETURA DE SINAL — Cotucatronics

> Firmware v0.130.0 · `6002a70` · Daisy Seed 2, BOOT_QSPI, C++ puro (sem RNBO)

## Topologia

```
p1 → in_gain_a → gate_a → [drive · phaser · warble] ─┐
p2 → in_gain_b → gate_b → [drive · phaser · warble] ─┤ (× piezo_mix)
     ├→ × dly_a/b → DELAY (1 instância) ─────────────┤
     ├→ × frz_a/b → FREEZE_A · FREEZE_B ─────────────┤→ soma → level → split
     ├→ × esp_a   → [SHIMMER_A ↺ SALA_A] ────────────┤
     ├→ × esp_b   → [SHIMMER_B ↺ SALA_B] ────────────┤
     └→ seco (as duas, inteiro) ─────────────────────┘
```

**Cauda PARALELA, não serial.** Cada destino recebe um envio próprio das duas
cadeias. A cauda serial era escolha, não lei — e a lei imaginada bloqueava o
envio por efeito.

## O que é por cadeia, o que é compartilhado

**Por cadeia (23 params):** `in_gain`, `gate_thresh/release`, `drv_on/mix`,
`phs_on/mix`, `wrb_on/mix`, `dly_on/mix`, `frz_on/vol/size`, **e o espaço
inteiro** — `rev_on/mix/size/damp`, `shm_on/mix/feedback/shift/tone`. Mais
`gov_*`, `bench_boost`, `exc_on`.

**Compartilhado (22 params):** afinação dos efeitos de caráter (`drv_drive/
tone/sujeira`, `phs_rate/depth/fb/stages`, `wrb_rate/depth/irreg`), `dly_time`,
`piezo_mix`, `level`, `buzios_*`, `tone_*`, `route_*`, `shift_hz`.

**Razão musical dos dois espaços:** corda dá tom sustentado, e pitch shift
sobre altura definida soa harmônico; tambor dá transiente de espectro largo, e
subir uma oitava disso vira metálico e borra o ataque. **Cada material vai
para o espaço que o revela.**

## Proteção — CTOS

| conceito | valor | natureza |
|---|---|---|
| **TETO** | 850 µs | orçamento administrativo, **escolhido** |
| **PRAZO** | 1000 µs | parede física, imposta pelo hardware |

> Confundir os dois faz a proteção criar o defeito que deveria evitar.
> **Toda defesa declara em qual dos dois mira.**

- **Teto**: N blocos consecutivos acima de 850 → derruba efeito por prioridade.
- **Prazo**: 3 estouros de 1000 µs em 2 s → derruba. Constantes **escolhidas,
  não medidas** (zero estouros observados em bancada).
- Testável nos dois sentidos: `ctos prazo <us>` e `ctos teto <us>` forçam
  valores artificiais. *Proteção não-exercitada é proteção suposta.*

**Regimes medidos (v0.123.2, leitor corrigido):**

| regime | média | pico |
|---|---|---|
| tudo desligado | 183 | 262 |
| drive 48× A+B, reverb A+B, shimmer A | 536 | 687 |
| + sujeira, shimmer B, delay, phaser | 697 | 992 |
| TUDO ligado | 818 | ~975 |

⚠️ **2,5 % de folga no pico com tudo ligado.** Não é uso desenhado (dois
shimmers contradizem a premissa dos espaços), mas é alcançável.

## Anti-feedback

`governor` + `shifter SSB`, **por ramo** (duplicados quando o segundo exciter
entrou). `shift_hz` é **compartilhado por acústica**: dois exciters no mesmo
corpo com shifts diferentes bateriam.

---

# COMANDOS SERIAL

| comando | o que faz |
|---|---|
| `list` | todos os params com valor atual |
| `set <nome> <valor>` | escreve em **unidade real** (não normalizada) |
| `board` | placa, versão, memória, mapa de pinos |
| `cpu` | média e pico por regime · `cpu reset` |
| `ctos` | estado da proteção, quedas, estouros · `ctos rearm` / `prazo` / `teto` |
| `midi` | tráfego, aplicadas, descartadas, `cc_janela` |
| `vistos` | **inventário de endereçamento** — o que chegou e de qual CC |
| `sync` | empurra os 76 params pro Loopy pelo espelho |
| `hist` | histograma de nível (entrada e pós-ganho) |
| `bb` / `bb arm` | caixa-preta |
| `b1` `b2` `b3` | simula os botões |
| `tone on` / `tone off` | tom de teste |
| `pitch` `pk` `dyn` `mscope` `ble` `log` | diagnóstico |
| `qspi vida` / `qspi id` | sonda da janela ITCM |
| `dfu` | entra em modo de gravação |

**`vistos` é o método de endereçamento** — a placa escuta e reporta; o dB varre
o Loopy. Venceu o `learn` (a placa anunciando) porque **não exige a placa
surda**, e foi ele que flagrou quatro widgets com número de outro parâmetro.
