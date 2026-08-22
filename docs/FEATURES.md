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
| **Ganho de entrada** por cadeia | `in_gain_a`, `in_gain_b` | 21, **50** | ativo · nasce em **1,0×**, faixa 1–8× com curva quadrática |
| **Mix entre as CADEIAS** (shell) | `piezo_mix` | **58** | ⏳ aceite pendente · **mudou de posição no pivô**: era balanço na entrada entre sinais crus, agora mistura as duas cadeias já processadas. Lei igual: centro = as duas cheias |
| **Tom de teste: destino** | `tone_dest` | — | ✅ 20/08 · 0=A · 1=B · **2=AMBAS (default)** · injetado **depois do gate**, senão o gate corta o próprio tom e você mede o gate |
| **Histograma de entrada** por canal | — | — | ✅ 20/08 · `hist` / `hist reset` · baldes de 6 dB pelo expoente do float, cru **e** pós-`in_gain`, clip com latch, e o dB que falta tirar pro atenuador · +27 µs |
| **Teto de soma do CTOS** | ordem em `Manifest::sacrif[]` | aviso no **CC 119** | ✅ **20/08, três testes passados** · irmão do watchdog por-efeito: pega N efeitos saudáveis que juntos não cabem · prioridade declarada a frio, não LIFO · `ctos teto <us>` simula · +14 µs |
| **Gate** por cadeia | `gate_thresh_{a,b}`, `gate_release_{a,b}` | 30, **51** | ativo · duplicado por **material**, não por desequilíbrio: o piezo do tambor lê transiente curto, o da mola lê raspagem contínua |
| **Delay** (RNBO, slot Instrument) | `dly_on`, `dly_time`, `dly_mix` | **60**, 22, 23 | ✅ certificado T5 · `dly_on` mistura pro seco e o módulo **segue rodando**: RNBO é opaco, e pular o `Process()` deixaria a linha rançosa |
| **Freeze granular** (shell) | `frz_on`, `frz_mix`, `frz_size` | 27, 28, 29 | ✅ certificado 18/08 |
| **Drive** — chave e dry/wet por canal | `drv_on_{a,b}`, `drv_mix_{a,b}` · `drv_drive`, `drv_tone` compartilhados | 38,39 / 44,45 · primário **43** | ⏳ **reaceite pendente na nova posição** (IFÁ10000 não se herda) · +21 µs por canal |
| **Phaser** — chave e dry/wet por canal | `phs_on_{a,b}`, `phs_mix_{a,b}` · `phs_rate`, `phs_depth`, `phs_fb`, `phs_stages` compartilhados | 40,41 / 46,47 · primário **49** | ⏳ **reaceite pendente** · +26 µs por canal · ⚠️ `phs_mix` é MONTANHA: pico em ~50% |
| **Warble** — chave por canal, params compartilhados | `wrb_on_{a,b}` · `wrb_rate`, `wrb_depth`, `wrb_irreg` compartilhados | 42 / 48 · primário **52** | ⏳ **reaceite pendente** · +14 µs por canal · 16 cents medidos |

> ### 📖 `wrb_mix` é NOME, não param — renomeação de exibição (20/08)
>
> **Não existe um param `wrb_mix`, e não é esquecimento.** O dry/wet do
> warble é o **crossfade do próprio `EffectSlot`** (`gs`/`gm` no callback),
> e foi assim que ele passou no IFÁ10000. Não aparece no `list`, não tem
> CC, não é ajustável.
>
> O nome existe só para alinhar o vocabulário com os vizinhos — drive e
> phaser **têm** `mix` ajustável — e para que ninguém procure um estágio
> que nunca existiu. **Se um dia virar param de verdade, é mudança no
> efeito aprovado** e passa por decisão do dB, não por consistência de
> tabela.
>
> O `EffectSlot` não foi tocado: é recurso compartilhado, e renomear
> dentro dele encostaria nos comentários que explicam a curva de
> igual-potência e nos nomes internos da rampa.
| **Shimmer** (shell) | `shm_on`, `shm_mix`, `shm_shift`, `shm_feedback`, `shm_tone` | 33, 37, 36, 35, **57** | ✅ **aprovado** 20/08 · cauda de 2 s · `shm_tone` **sobe pra clarear**: 0 = sino −6 dB em 3 kHz + corte 6 kHz, 1 = aberto até 18 kHz |
| **Reverb** Freeverb (shell, moldura) | `rev_on`, `rev_mix`, `rev_size`, `rev_damp` | **61**, 24, 25, 26 | ✅ certificado T14a · ⚠️ `rev_on` em 0 **emudece o shimmer junto** (ele realimenta este reverb) — o `board` declara |

> ### 🔀 PIVÔ DE ARQUITETURA em curso (20/08) — duas cadeias de caráter
>
> Os instrumentos híbridos (tambor + corda + molas no mesmo corpo) pedem
> tratamento diferente por material. Decisão do dB:
>
> ```
> p1 → [DRIVE · PHASER · WARBLE] ┐
>                                 ├→ MIX → [DELAY · FREEZE · SHIMMER↺REVERB]
> p2 → [DRIVE · PHASER · WARBLE] ┘        → OUT_WIDTH → split
>      caráter: DUPLICADO                  espaço: COMPARTILHADO
> ```
>
> A cauda fica única de propósito: é o que faz tambor e corda soarem como
> **o mesmo objeto**. Duplicar shimmer e reverb separaria o que
> musicalmente deve unir — e são justamente os mais caros.
>
> **`piezo_mix` muda de posição:** sai da entrada e vira o balanço entre as
> duas cadeias **já processadas**.
>
> Cada efeito duplicado passa pelo **IFÁ10000 na nova posição** — não herda
> o aceite da cadeia antiga (regra herdada tem pré-condição, cicatriz 11z).
>
> Ordem: (a) detune fora ✅ · (b) infraestrutura das duas cadeias + mix ✅
> (620 µs / 66%) · (c) tetos resolvidos no desenho ✅ (96 params / 48 CCs) ·
> (d) caráter recolhido a params compartilhados ✅ · (e) `out_width`
> aposentado ✅ · (f) **proteção anti-feedback do ramo B** ← próximo, e
> bloqueante para o hardware · (g) segundo espaço + `rev_type` · (h) send
> por cadeia · (i) GPIOs dos DPDT · (j) IFÁ10000 e aceite de ouvido.
>
> ### 🔴 O ramo B não tem proteção anti-feedback
>
> Governor e shifter SSB existem **só em `OUT_L`** — correto enquanto
> `OUT_R` era jack de linha limpo *por desenho*. Com **dois exciters**,
> `OUT_R` passa a empurrar um transdutor no corpo da escultura sem
> nenhum dos dois, fechando laço acústico desprotegido. **É trabalho
> novo, não configuração**, e precisa existir antes de o exciter 2 ser
> soldado.
>
> ### Fork de saída é modo GLOBAL (decidido 20/08)
>
> Amps e jacks compartilham `OUT_L`/`OUT_R`, então não existe "exciters
> em stems e jacks em mix". **stems:** `OUT_L` = cadeia A, `OUT_R` =
> cadeia B — cada exciter recebe seu material, os jacks gravam separado.
> **mix:** `OUT_L` = `OUT_R` = mistura, os dois exciters empurram o som
> inteiro.
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
| **`src_mix`, `gov_ceiling` no mapa MIDI** | 18/08 | roteamento e segurança; despejos do iOS os punham no extremo e emudeciam o instrumento **3× numa sessão** | `gov_ceiling` não deve voltar. `src_mix` foi aposentado por inteiro (abaixo) |
| **`src_mix`** (o param inteiro) | 20/08 | era **crossfade** das duas entradas: no centro derrubava as duas em −6 dB, e por isso não resolvia "um encobre o outro". Substituído pelo `piezo_mix`, que é **balanço**. Manter os dois seria dois controles disputando os mesmos ganhos — a família C do catálogo | `t.Add("src_mix", 0.f, 0.f, 1.f)` e o crossfade linear no passo 1 do `AudioCb`. O knob **k2** foi junto para o `piezo_mix` |
| **Regra "reparear após todo RESET"** | 18/08 | **revogada por medição** — o pareamento sobrevive ao reset. Nasceu de um log que eu mesmo silenciei | — |
| **Quarentena que DESCARTA** | 18/08 | as duas decisões acima mataram a ameaça; ela passou a descartar a inicialização legítima | rebaixada a sensor, não removida |
| **Filtro de plausibilidade por magnitude** no watchdog | 18/08 | descartava a catástrofe junto com o ruído | removido; a causa (relógio) foi consertada |
| **`out_width`** — largura mid/side | 20/08 | **a arquitetura de dois exciters esvaziou o controle de sentido.** M/S pressupõe que L e R são um **par estéreo**. Aqui não são: em `stems` os dois canais são **fontes independentes** (aplicar M/S misturaria o piezo 1 dentro do piezo 2 — contaminação silenciosa), e em `mix` L = R, então a largura é zero por construção. **Não sobrou modo em que ele significasse algo.** Decisivo: com dois exciters no **mesmo corpo** não há imagem estéreo — a soma é **acústica, no objeto, antes do ar**, e o modelo mental do M/S não se aplica | reviver só se existir um modo em que a saída seja um par estéreo de verdade. O bloco ficava entre o LEVEL e o split (posição marcada como vaga no fonte) e custava +4 µs, zero no neutro. Liberou 1 param e o **CC 59** |
| **Params de caráter por canal** (14 × 2) | 20/08 | **decisão de uso do dB**: seis efeitos com parâmetros independentes ficou confuso de operar. Voltou a existir **um** drive, **um** phaser, **um** warble, com params compartilhados; duplicam só `_on` e `_mix`. **Não houve economia de CPU e isso era esperado** — cada efeito segue processando os dois sinais com estados internos separados (dois conjuntos de allpass, dois buffers de warble), senão os canais se contaminam. O que encolheu foi a superfície de controle e a tabela (71 → 62 params) | reinstanciar por cadeia: **a estrutura suporta** — o `struct Cadeia` já tem objeto próprio por canal, e o manifest volta a declarar os params como arrays `[kCadeias]` |
| **Rota de teste isolada do motor de pitch** | 20/08 | custava uma **instância inteira do `PitchEngine` — 153,6 KB de SDRAM** — só para bancada. Com o detune fora do build, o único consumidor real virou o shimmer, que vive na cauda e tem instância própria. **Instrumentação é código de produção e paga custo de produção:** uma sonda que custa uma instância precisa justificar-se por uso corrente, não por conveniência histórica | reinstanciar `PitchEngine` + `EffectSlot` e o bloco 1.6 do `AudioCb` (marcado como vago no fonte) se/quando um segundo consumidor de pitch entrar numa cadeia. Os tunáveis `pitch grain` e `pitch jitter` **não** foram junto: seguem agindo na instância da cauda |
| **Detune** (shell) — 2 vozes micro-desafinadas + largura | 20/08 | **decisão de ouvido do dB: _"o warble só já está bom"_.** O warble cobre o mesmo território — modulação de altura — e o detune custava **+125 µs, cinco vezes o phaser**, o efeito mais caro da suíte. Custo desproporcional ao que acrescentava sobre algo que já estava lá. Foi **aprovado de ouvido em 20/08 e aposentado no mesmo dia**, e isso é legítimo: aprovar responde *"isto funciona?"*, a arquitetura responde *"isto vale o lugar que ocupa?"* | `detune.h` v0.1.0 segue no repo, **intacto**. Reviver: (1) `#include "detune.h"` no `main.cpp` — o comentário que ficou no lugar do include explica; (2) declarar `Detune detune`, `EffectSlot detune_slot` e uma **terceira** instância de `PitchEngine`, 153,6 KB de SDRAM; (3) o bloco de processamento ia em **4.3, pós-módulo** — a posição está marcada como vaga no fonte e é obrigatória: antes dela a cadeia é mono, e sem estéreo não há `spread`; (4) manifest: 5 params + `AddMidi` em **52–56, que ficaram livres**; (5) recolocar no índice 0 da ordem de sacrifício |

---

## Checklist antes de acrescentar feature nova

- [ ] **Li esta página inteira?** Alguma feature vigente já faz parte disto?
- [ ] Quantos **params** e **CCs** ela adiciona? Cabem nos tetos? (o boot diz, e o `board` repete sob demanda — alarme só no banner se perde)
- [ ] Ela **aposenta** alguma coisa? Se sim, a aposentada vai pro arquivo **no
      mesmo commit**
- [ ] Ela é **CTOS** ou **Efeito**? (mapa do `docs/CTOS.md` §5)
- [ ] Se é Efeito: passa pelo **IFÁ10000** completo, etapa 0 inclusive
- [ ] Ganha **on/off próprio**, separado do mix — decisão do dB, 18/08
- [ ] **Nasce com o caminho MONO como padrão.** Antes do reverb a cadeia é um
      canal só; `Process(l, r)` sobre fonte mono paga o dobro **sem sintoma
      algum** — a saída fica certa e só o custo denuncia. Três efeitos
      seguidos caíram nisso

> **A pergunta que este documento existe para forçar:** *o que já está aqui
> que eu deveria revisar antes de acrescentar mais?*
