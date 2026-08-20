# FEATURES — a caixa única

**Documento vivo v0.1.0 · 2026-08-18**

> **Comece por aqui.** Toda feature vigente do instrumento está nesta página.
> Se não está aqui, ou não existe, ou foi aposentada — e nesse caso está no
> arquivo, no fim.

## Por que existe

Em 18/08 duas tabelas estouraram no mesmo dia e **em silêncio**: quatro
parâmetros novos empurraram três para fora (`kMaxParams`), e a 17ª entrada do
mapa MIDI foi escrita fora do array (`kMaxMidiMap`). Os búzios **sumiram do
instrumento** e o `shm_shift` chegava sem encontrar dono.

Nenhum dos dois deu erro. O sintoma foi *uma feature ausente* e *um `?` numa
tabela de debug*.

**A causa profunda não é o teto — é não haver lugar único onde a lista de
features vive.** Sem isso, ninguém revisa o que já existe antes de acrescentar,
e o custo de cada adição fica invisível até algo quebrar.

> **A regra: antes de acrescentar, revisar o que já está aqui.** Feature
> aposentada vai para o arquivo; feature viva fica na caixa. A caixa é o
> começo de toda sessão.

---

## Vigentes

### Cadeia de áudio
| Feature | Params | CC | Estado |
|---|---|---|---|
| **Fonte** — crossfade piezo/jack + tom de teste | `src_mix`, `tone_freq`, `tone_mix` | — | ativo (`src_mix` fora do MIDI por segurança) |
| **Ganho de entrada** | `in_gain` | 21 | ativo |
| **Gate** | `gate_thresh`, `gate_release` | 30 | ativo |
| **Delay** (RNBO, slot Instrument) | `dly_time`, `dly_mix` | 22, 23 | ✅ certificado T5 |
| **Freeze granular** (shell) | `frz_on`, `frz_mix`, `frz_size` | 27, 28, 29 | ✅ certificado 18/08 |
| **Drive** (shell) — waveshaper tanh como cola | `drv_on`, `drv_drive`, `drv_tone`, `drv_mix` | 38–41 | ✅ **aprovado** 18/08 · +21 µs |
| **Shimmer** (shell) | `shm_on`, `shm_mix`, `shm_shift`, `shm_feedback` | 33, 37, 36, 35 | 🎵 aprovado de ouvido · cauda de 2 s na saída |
| **Reverb** Freeverb (shell, moldura) | `rev_mix`, `rev_size`, `rev_damp` | 24, 25, 26 | ✅ certificado T14a |
| **Passa-alta do Fx2** | `fx2_hp` | — | ⏸ inativo enquanto o slot Fx2 estiver vazio |
| **Governor / limiter / gate de saída** | `gov_ceiling`, `gov_release` | — | ativo (fora do MIDI por segurança) |
| **Shifter SSB** | `shift_hz` | — | ✅ certificado T8, desligado (espera o exciter) |
| **Level** master | `level` | 20 | ativo |

### Gesto e automação
| Feature | Params | CC | Estado |
|---|---|---|---|
| **Búzios** — sorteio por transiente | `buzios_on`, `buzios_thresh`, `buzios_cooldown` | 31, 32 | ativo · whitelist: `dly_time`, `dly_mix` |
| **B2 hold** — congela enquanto segurado | `button2_target` = `frz_on` | — | espera o painel v2 |

### Infraestrutura (CTOS)
| Feature | Comando | Estado |
|---|---|---|
| **Motor de pitch compartilhado** | `pitch <ratio>` | ✅ validado — 1 buffer, 4 vozes, por ponto de derivação |
| **EffectSlot** — fade, custo-zero, cronometragem | — | ✅ validado |
| **Watchdog com veto** — bypass até `ctos rearm` | `ctos` | ativo |
| **Pulso de vida no LED** por fase | — | ativo |
| **Quarentena de reconexão** | `ble` | ⚠️ rebaixada a **sensor** — detecta, não descarta |
| **Caixa-preta** | `bb` | ativo · ⚠️ `bb arm` silencia o log até `log on` |
| **MIDI scope** | `mscope` | ativo |
| **Diagnóstico BLE** — LEDs do WIDI em D28/29/30 | `ble raw` | ✅ legenda medida |
| **Sync** — empurra estado pro controlador | `sync` / CC 34 | ⏸ pronto, inútil até o Loopy fazer MIDI learn |
| **Denúncia de capacidade** das tabelas | boot | ✅ novo, 18/08 |
| **`dfu go`** — reinicia no bootloader por software | `dfu go` | ✅ novo — trocou 119 tentativas por 1 |
| **Aviso de sessão MIDI ausente** | `ble` | ✅ novo — LB fixo + zero bytes ⇒ "reconecte, não esqueça" |

---

## Arquivo — aposentadas

| Feature | Quando | Por quê | Como reviver |
|---|---|---|---|
| **Vocoder RNBO** | 18/08 | decisão de ouvido do dB | `MODULE_SRC` no Makefile + `block_size` 256 no manifest. Export, wrapper, binds e a cerca de Lá menor dos búzios seguem no repo, comentados |
| **Freeze RNBO** | — | reprovado na etapa 5 do IFÁ10000: **7 349 µs** contra 2 667 de orçamento, e 89% do pool de SRAM | só se o patch emagrecer. Substituído pelo `freeze.h`, que faz o mesmo gesto por **98 µs** |
| **`fx2_gain`, `fx2_on` no mapa MIDI** | 18/08 | apontavam pro slot RNBO vazio — mexer não fazia nada, em silêncio | voltam com o vocoder, no CC 33 reservado |
| **`src_mix`, `gov_ceiling` no mapa MIDI** | 18/08 | roteamento e segurança; despejos do iOS os punham no extremo e emudeciam o instrumento **3× numa sessão** | não devem voltar. Seguem no `set` da serial |
| **Regra "reparear após todo RESET"** | 18/08 | **revogada por medição** — o pareamento sobrevive ao reset. Nasceu de um log que eu mesmo silenciei | — |
| **Quarentena que DESCARTA** | 18/08 | as duas decisões acima mataram a ameaça; ela passou a descartar a inicialização legítima | rebaixada a sensor, não removida |
| **Filtro de plausibilidade por magnitude** no watchdog | 18/08 | descartava a catástrofe junto com o ruído | removido; a causa (relógio) foi consertada |

---

## Checklist antes de acrescentar feature nova

- [ ] **Li esta página inteira?** Alguma feature vigente já faz parte disto?
- [ ] Quantos **params** e **CCs** ela adiciona? Cabem nos tetos? (o boot diz)
- [ ] Ela **aposenta** alguma coisa? Se sim, a aposentada vai pro arquivo **no
      mesmo commit**
- [ ] Ela é **CTOS** ou **Efeito**? (mapa do `docs/CTOS.md` §5)
- [ ] Se é Efeito: passa pelo **IFÁ10000** completo, etapa 0 inclusive
- [ ] Ganha **on/off próprio**, separado do mix — decisão do dB, 18/08

> **A pergunta que este documento existe para forçar:** *o que já está aqui
> que eu deveria revisar antes de acrescentar mais?*
