# FEATURES — a caixa única
<!-- doc-check: ignore in_gain_a (REMOVIDO em 28/08. A lapide fica de proposito: e ela que impede alguem de o reintroduzir sem ler por que saiu, e o que se perdeu com ele.) -->
<!-- doc-check: ignore in_gain_b (idem) -->

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
| **Volume por ramo, nos pots físicos** | `vol_l`, `vol_r` | **116**, **117** | ✅ **28/08** · pots em A0 (D15) e A1 (D16), depois do limiter · **captura suave** contra o salto, e adopção da posição física no arranque · `knob` mede |
| ~~**Ganho de entrada** por cadeia~~ | ~~`in_gain_a`, `in_gain_b`~~ | ~~21~~ | 🗑️ **REMOVIDO 28/08 — SE NÃO SOA, NÃO FICA.** O `Governor` é um AGC: 18 dB postos aqui eram 18 dB que ele tirava adiante, e a saída não mudava. O único efeito audível era o degrau. O balanço entre piezos já era do `piezo_mix`. **Perdeu-se** o casamento com transdutores de sensibilidade diferente — o governor só REDUZ; se aparecer na replicação, conserta-se no divisor da carrier board |
| **Mix entre as CADEIAS** (shell) | `piezo_mix` | **58** | ⏳ aceite pendente · **mudou de posição no pivô**: era balanço na entrada entre sinais crus, agora mistura as duas cadeias já processadas. Lei igual: centro = as duas cheias |
| **Tom de teste: destino** | `tone_dest` | — | ✅ 20/08 · 0=A · 1=B · **2=AMBAS (default)** · injetado **depois do gate**, senão o gate corta o próprio tom e você mede o gate |
| **Histograma de entrada** por canal | — | — | ✅ 20/08 · `hist` / `hist reset` · baldes de 6 dB pelo expoente do float, cru **e** pós-`in_gain`, clip com latch, e o dB que falta tirar pro atenuador · +27 µs |
| **Teto de soma do CTOS** | ordem em `Manifest::sacrif[]` | aviso no **CC 119** | ✅ **20/08, três testes passados** · irmão do watchdog por-efeito: pega N efeitos saudáveis que juntos não cabem · prioridade declarada a frio, não LIFO · `ctos teto <us>` simula · +14 µs |
| **A trava do CTOS fala** | `set` avisa · `list` marca · `board` grita | ✅ **22/08, v0.111.0** · a queda é travada até `ctos rearm`, e antes disso era invisível: o `set` era desfeito todo bloco e o `list` mostrava 0 igual a um efeito desligado à mão · custou uma sessão procurando defeito em drive/phaser/shimmer intactos (bts 12k) |
| **B3 = rearme do CTOS** | botão físico em **D25** (pull-up, ativo em baixo) | ✅ **22/08, v0.111.0** · o dB toca sem computador · **devolve os efeitos ligados**, ao contrário do `ctos rearm` por serial, que só solta a trava · `b3` no serial pulsa o botão enquanto o painel não existe (bts 12m) |
| **LED avisa a trava pela cadência** | batimento 4× mais rápido | ✅ **22/08, v0.111.0** · o LED continua sendo a prova de "loop vivo"; o alarme entra na cadência, não no estado · sem ele o B3 seria inútil — ninguém aperta um rearme sem saber que precisa (bts 12m) |
| **Drive: o joelho é relativo ao nível** | `in_gain` antes de `drv_drive` | ✅ **22/08, medido** · shaper normalizado é ganho limpo abaixo de `x ≈ 1/d` · com `in_gain` em 1.0 o p2 (−19,5 dBFS) fica em ~1% de THD até no pico: inaudível por nível, não por defeito · corrigido em `in_gain` 2,2×/6,0× (bts 12n) |
| **Espelho MIDI — a placa fala de volta** | canal 2, `eco_ult[cc]` | ✅ **22/08, v0.114.x** · varre os 76 params e transmite o que mudou · no boot empurra o estado inteiro (a placa fala primeiro) · resolve a tela do Loopy mentindo depois de gravação, queda do CTOS ou mudança por serial |
| **Janela de autoria, 250 ms por CC** | `quiet_until[128]` | ✅ **22/08, v0.115.0** · o widget do Loopy é toggle COM ESTADO: fora de fase devolve o valor oposto e desfaz a mudança autônoma · casamento de mensagem não serve, o critério é "o assunto sobre o qual acabei de falar" · medido: CC65 passou de 6/8 para **8/8**, CC68 permaneceu 8/8 (bts 12p) |
| **Modo `learn`** | ~~`learn [stop\|next\|<cc>]`~~ | ⬛ **REMOVIDO em 22/08, v0.121.0** · cinco versões e nunca fechou o endereçamento · exigia a placa surda, o que cegava o `vistos` que provaria o resultado — dois regimes incompatíveis, não dois métodos · preservado no commit `514e09c` (bts 12t) |
| **`drv_sujeira` (CC 36)** | morph tanh → wavefolder | ✅ **22/08, v0.4.0** · THD 17,9% → 58,9% no pico, 8× mais energia acima do 7º harmônico · clipe duro e assimetria reprovados (< 1% de diferença) · passa-baixa fixo de 2 kHz antes da dobra: sem ele o aliasing ia a −36 dB a 3 kHz · +9 µs, zero em 0 |
| **`vistos` — inventário de endereçamento** | um contador por CC, 128 bytes | ✅ **22/08, v0.120.0** · responde "o que já chegou alguma vez", que histórico de mensagens não responde · fechou **65 de 65** em duas varreduras depois de o modo `learn` falhar em cinco versões · flagrou 4 widgets com número de outro parâmetro · ponto cego declarado: só vê widget que ENVIA (bts 12r, 12s) |
| **Contador de estouros de prazo** | por regime, com carimbo e retrato | ✅ **22/08, v0.122.0** · o CTOS guardava só o MÁXIMO, e máximo sem denominador não decide política · mede **zero** em uso desenhado, no extremo com tudo ligado, e sob provocação · cada estouro carimba quais efeitos estavam no ar |
| **Critério de estouro (mira o PRAZO)** | 3 estouros em 2 s derrubam um efeito | ✅ **22/08, v0.123.2** · teto (850) é orçamento, prazo (1000) é parede — um critério mirando o teto derrubaria efeito numa config que toca limpa · nasce quieto, que é o certo · `ctos prazo <us>` demonstra (bts 12w) |
<!-- doc-check: ignore route_exciter (nome APOSENTADO em 28/08, hoje `out_l`. A mencao e deliberada: e o registo da renomeacao, e apagar o nome velho apagaria justamente o que explica por que ele estava errado.) -->
<!-- doc-check: ignore route_jack (idem, hoje `out_r`) -->
| **Áudio não passa por contato mecânico** | chave → GPIO → firmware corta | ⬜ **REGRA DE HARDWARE, 22/08** · chave de potência não conduz nível de linha (circuito seco): fecha mecanicamente e segue aberta pro sinal · custou uma tarde · `out_l`/`out_r` cortam o ramo (nomes corrigidos em 28/08: eram `route_exciter`/`route_jack`, e o DPDT de 3 posições — jack \| ambos \| amp — mostrou que o firmware nomeia SAÍDA, não destino) · o polo 2 do DPDT em D29/D30 segue o mesmo desenho (bts 13b) |
| **Freeze físico com LED** | switch **D24**, LED **D27** | ✅ **22/08, aprovado de ouvido** · arcade 30mm, microswitch NO · **toggle** (bateu congela, bateu solta), não hold · debounce **30 ms MEDIDO**: com 10 ms cinco batidas viravam seis · o LED segue o **param**, nunca o gesto — se o freeze mudar por CC, preset ou queda do CTOS ele conta a verdade · LED de 5 V a 3,3 V: brilho aprovado, plano B do transistor arquivado |
| **Botões com alvo, generalizado** | tabela no manifest | ✅ **22/08, v0.127.1** · cada botão declara param + índice de LED; o manifest não conhece pino · um só `AlternaBotao()` serve todos, pelo mesmo motivo que o sacrifício do CTOS virou função única · sentinela `-1` com init explícito (zero-init apontava o randomizador pro `level`, bts 13f) |
| **Freeze B — fiação pendente** | switch **D22**, LED **D23** | ⬜ firmware pronto e declarado (`B4`), falta soldar |
| **`frz_size` — curso morto** | faixa 20–165 vs clamp `NucleusLen()/3` ≈ 166,7 | ✅ **corrigido em `d80e45d`** · era 20–165 contra o mesmo clamp: 61% do fader não fazia nada (bts 12i) · o `165` ganhou comentário amarrando ao `kNucleusMs/3` para não dessincronizar em silêncio · **o item ficou meses na FILA descrevendo o estado pré-conserto** — deriva num lugar que o `check-docs` não olha (bts 13i) |
| **Preset em flash** | salvar e recarregar no boot | ⬜ **aberto 22/08** · toda gravação zera todo param e `level` nasce em 0 · perda silenciosa dos dois lados, e o sintoma imita defeito de efeito (bts 12l) |
| **Gate** por cadeia | `gate_thresh_{a,b}`, `gate_release_{a,b}` | 30, **51** | ativo · duplicado por **material**, não por desequilíbrio: o piezo do tambor lê transiente curto, o da mola lê raspagem contínua |
| **Delay** (shell, C++) — envio por cadeia | `dly_on_{a,b}`, `dly_mix_{a,b}`, `dly_time` | 60–64 | ⏳ **reaceite pendente** · `delay.h` v0.1.0 · +11 µs (era 45, **sempre**) · piso 50 ms · feedback fixo **0,4**, lido do patch |

> ### 🔀 CAUDA PARALELA e DOIS ESPAÇOS (20/08)
>
> A cauda deixou de ser fila e virou **destinos irmãos**, cada um com envio
> por cadeia. E o **espaço duplicou**: cada cadeia tem sala e shimmer
> próprios, porque shimmer não é um efeito ao lado do reverb — ele *é* o
> reverb realimentado com pitch, e duas molhaduras exigem duas salas.
>
> Delay e freeze continuam sendo **um só**, com envio por cadeia: o gesto
> pedido era sobre espaço, e duplicá-los custaria CPU sem entregar diferença.
>
> **Medido:** A com delay+shimmer e B com sala limpa = **490 µs / 530 pico
> (53%)**. Pior caso, as duas com tudo = **598 / 639 (64%)** — 209 µs abaixo
> da previsão, porque custo medido numa arquitetura não se transporta para
> outra (cicatriz 12g).
>
> **Perdeu-se:** o delay não é mais reverberado. Os ecos saem secos e a sala
> trabalha sobre a fonte.
>
> **Simplificado em seguida:** `esp_on/esp_mix` e o `rev_mix` antigo eram **o
> mesmo controle duas vezes** — ganho na entrada e ganho na saída da mesma
> sala linear. Ficou um envio só por destino: `rev_on`/`rev_mix` dizem quanto
> da cadeia entra no espaço, o retorno volta inteiro, e `shm_mix` voltou ao
> emprego único (só a camada direta). **75 params, 75 CCs.**
| **Freeze granular** (shell) — um por cadeia | `frz_on_{a,b}` (**o gesto**), `frz_vol_{a,b}`, `frz_size_{a,b}` | 65–70 | ✅ certificado 18/08 · ✅ **botão físico aprovado de ouvido 22/08** · **DUAS instâncias independentes**, com buffer próprio · o `_on` **é** o gesto de congelar, não chave de bypass — não existe `frz_hold` (bts 13h) |
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
> bloqueante para o hardware · (g) segundo espaço — **entregue de outra
> forma**: em vez de um `rev_type` escolhendo o caráter, saíram **dois
> espaços independentes** com params por cadeia (`rev_*_{a,b}`,
> `shm_*_{a,b}`), que é o desenho que "corda com shimmer, couro com
> reverb limpo" pedia · (h) send
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
| **RNBO INTEIRO** — runtime, sistema de slots, módulos e exports | 20/08 | o delay era o **último consumidor**, e foi reescrito em C++ (`delay.h`). O que saiu junto: `MODULE_SRC`, `RNBO_DIR`, `module.h`, `mod_slot[]`, `ModBind`/`bind[]`/push-por-nome, seis arquivos `module*.h/.cpp`, quatro diretórios de export (**8 MB**), os seis params órfãos do vocoder, o watchdog do fx2 que julgava pelo CPU global, o comando `probe` e o histograma nota-a-nota que media um vocoder fora do build. **Rendeu −133 µs de média, −67 µs só na base** (um `Module::Process` roda ligado ou desligado — custo-zero-desligado era estruturalmente impossível no sistema de slots), −36 KB de binário e o pool de 49 KB de SRAM | tudo em git. Reviver exige devolver `RNBO_DIR`, um export com `rnbo/`, `MODULE_SRC`, o `module.h` e os binds — e aceitar o custo fixo de volta |
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
