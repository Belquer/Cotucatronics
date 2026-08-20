# STATUS — Cotucatronics

**Onde estamos.** O par deste documento é
[battle_scars.md](battle_scars.md), que diz *o que aprendemos chegando
aqui*. Leia os dois antes de tocar em código.

> Atualizado em 2026-08-20 · leva: suíte de cinco efeitos fechada, shimmer
> suavizado por medição, tetos das tabelas corrigidos

---

## Rotina de operação: os dois destinos, sempre juntos

**Toda nova versão de `STATUS.md` ou `battle_scars.md` vai para os DOIS
destinos na mesma ação:**

| Destino | Caminho | Forma |
|---|---|---|
| Repo | `docs/` | cru, sem frontmatter |
| Vault | `~/Documents/Obsidian Vault/Cotucatronics/` | com frontmatter YAML + tags |

Os dois servem leitores diferentes e nenhum substitui o outro. O **repo**
é o que sobrevive ao clone limpo — documento citado que não existe no
clone é a mesma classe do `module_rnbo_alloc.h` que faltou por semanas.
O **vault** é onde se lê, linka e pensa entre sessões, com tags e
backlinks que o Markdown cru não tem.

Nunca deixar um destino pra depois: o par desalinha em silêncio, e o
desalinho só aparece quando alguém precisa da versão certa.

---

## Como retomar

```bash
cd ~/CODE/GitHub/Cotucatronics/src
make BOARD=daisy_seed && make check
# RESET simples (SEM segurar BOOT — isso entra no DFU da STM, onde mora
# o bootloader). Grave na janela de graça de ~2 s:
make program-dfu
./cotuca_serial.py --send board mods list --gap 2
```

Placa roda da QSPI via bootloader da Electrosmith (`APP_TYPE=BOOT_QSPI`),
480 MHz permanente, `-O3` (ver a cicatriz do `-O3` no cap. 3 do battle
scars — ele PIOROU; reverter exige remedir tudo).

---

## O que toca hoje

**Pedaleira de cinco efeitos residentes, em ordem fixa.** Vocoder fora do
build desde 18/08 (decisão de ouvido, revivível). Bloco **48**.

```
piezo/linha → src_mix → in_gain → gate → DRIVE → PHASER → WARBLE
            → DELAY (Fx1) → DETUNE → FREEZE → [SHIMMER ↺ REVERB] → level
            → OUT_WIDTH (mid/side) → split → governor+SSB (exciter) | jack
```

`piezo_mix` é BALANÇO, não crossfade: no centro os dois piezos entram cheios.
`out_width` fica entre o LEVEL e o split porque depois do split OUT_L e OUT_R
deixam de ser uma imagem estéreo — são exciter e jack, com processamentos
diferentes.

```
```

O shimmer não tem reverb próprio: ele realimenta o Freeverb que já está na
cadeia, e por isso aparece dentro do laço.

| Recurso | Medido |
|---|---|
| CPU (suíte + shimmer + detune) | **59,5% média / 65,8% pico** do prazo de 1000 µs |
| Tabelas | **55/64 params · 37/48 CCs** (o `board` declara) |
| Bloco | **48** (1 ms de prazo) |
| Custo por efeito | drive +21 µs · warble +14 · phaser +26 · detune **+125** |

---

## ✅ Estado do repo é INSTRUMENTO

Resolvido em 2026-08-17. A dinâmica voltou e a placa **sobe tocando**:

| | |
|---|---|
| `level` 1.00 · `in_gain` **8×** | `gate_thresh` 0.02 · `gov_ceiling` 0.85 |
| `rev_mix` 0.35 · `fx2_hp` 120 Hz | `fx2_on` 1 · `buzios_on` 1 · `buzios_thresh` 0.04 |

`shift_hz` segue em 0: é anti-feedback do laço piezo→corpo→piezo, e liga
quando o **exciter** entrar no caminho — antes disso só metaliza o timbre.

> **Quando o exciter entrar, revisar `level` e `gov_ceiling` juntos.** Lá
> não há atenuador: o TPA3116 recebe o que a placa mandar, e os dois
> formam o par que decide onde o laço para de crescer.

---

## Versões

| Arquivo | Versão |
|---|---|
| `hal.h` / `hal_daisy_seed.cpp` | v0.17.0 / v0.16.0 |
| `main.cpp` | **v0.84.0** |
| `manifest.h` | v0.56.0 |
| `params.h` / `midi.h` | v0.8.0 / v0.4.0 |
| `pitch.h` / `shimmer.h` | **v0.5.0** / **v0.3.0** |
| `suave.h` | **v0.1.0** (novo) |
| `drive.h` / `phaser.h` / `warble.h` / `detune.h` | v0.2.0 / v0.3.0 / v0.1.0 / v0.1.0 |
| `freeze.h` / `reverb.h` / `effect_slot.h` | v0.3.1 / v0.2.0 / v0.3.0 |
| `module_rnbo_vocoder.cpp` | v0.6.0 (fora do build) |

---

## Fechado em 18/08 — MIDI certificado sob carga

**T16c passou.** 2 779 CCs em 90 s com o vocoder a **65,1%** do prazo do
bloco: zero órfão, zero ressync, zero perdido, zero erro de UART. A marca
d'água da fila ficou em **32 de 128 bytes — idêntica com e sem carga**, o
que fecha com número o furo do `PollMidi` (bytes entram por DMA, mas quem
despacha é o loop). Os 32 bytes são o tamanho da rajada do BLE, não atraso
do loop: se fosse latência, teria subido com a carga.

**Regra revogada:** o pareamento BLE **sobrevive** ao reset. O
`gravar → reparear → testar` era superstição nascida de um log que eu
mesmo silenciei — caso 8 do battle_scars, com adendo no capítulo 11 e a
seção do BOARDS.md marcada como REVOGADA.

**Cicatrizes viraram skills.** `.claude/skills/`: `ifa10000`,
`medicao-limpa`, `instrumentacao`, `bug-de-ordem`, `edicao-segura`. O
IFÁ10000 ganhou a **etapa 0 — MEDIÇÃO LIMPA**.

---

## Ganho sonoro não planejado (18/08): o grave voltou

Aceite do dB, sem eu ter mexido em nada de timbre: **"presença de grave que
nunca teve antes, muito melhor sonoramente."**

**Causa, verificada no código:** o passa-alta de 120 Hz mora dentro do bloco
do Fx2. Vocoder fora ⇒ slot vazio ⇒ `fx2_want` falso ⇒ bloco pulado ⇒ **sem
filtro**. Ele existia só pra conter o estouro de grave que o vocoder
causava, e saiu junto com a causa porque foi instalado no escopo certo.

**Contribuição secundária, não medida:** `block_size` de 256 → 48 derruba a
latência de 5,3 ms pra 1 ms. Num instrumento realimentado isso desloca onde
o laço acústico se estabelece e aperta o transiente. Plausível e não
isolado — se quisermos o número, é um voo com uma variável.

---

## ✅ Freeze granular — CERTIFICADO (2026-08-18)

IFÁ10000 completo. **Etapa 9 passou:** *"ficou ótimo assim"* — sustentado de
10 s sem pulsação, cauda de 2 s, instrumento vivo audível por baixo.

| Etapa | Resultado |
|---|---|
| 4 MEMÓRIA | 768 KB SDRAM, 4 leituras **sequenciais**/amostra |
| 5 CARGA | **98 µs** do efeito em 500 de orçamento; 370 de 1000 no sistema |
| 6 MATERIAL | transiente real (tap na escultura) |
| 8 SEGURANÇA | fora dos búzios — categoria **GESTO** |
| 9 ACEITE | ✅ dB aprovou |

**Três versões até chegar aqui, e cada uma consertou um erro de desenho meu:**
- **v0.1.0** reprovou de ouvido: varredura linear = laço de 2 s. Eu tinha
  escrito no cabeçalho que o jitter mataria a periodicidade. Não mata.
- **v0.2.0** trocou varredura por **sorteio num núcleo de 500 ms** — matou a
  gaiola e, no mesmo gesto, quebrou a premissa de correlação da janela,
  criando o ganho baixo. *Janela e estratégia de posição são um par.*
- **v0.3.0/0.3.1** fechou: **raiz-de-Hann** (potência constante para grãos
  descorrelacionados), **mix como camada** e não crossfade (o crossfade
  apagava o instrumento vivo), e **cauda de 2 s**.

---

## Para amanhã — pedidos do dB (18/08, fim de sessão)

O instrumento está **funcionando e aprovado** ("tudo ok! excelente"). Três
ajustes do freeze, e os três têm raiz comum de desenho:

**1. `frz_mix` é crossfade e deveria ser CAMADA.**
```cpp
l = l * (1.f - mix_) + wl * mix_;   // mix 1.0 APAGA o instrumento
```
Congelar deve **somar** uma camada sustentada, não substituir a fonte. É a
causa de "quando o frz está rolando não escuto o instrumento direito" — não é
efeito colateral, é o que a linha faz.

**2. Ganho do freeze muito baixo.** Causa técnica: duas janelas Hann somam
1.0 só para conteúdo **correlacionado**. Com as posições agora **sorteadas**,
os grãos são descorrelacionados e somam em potência — ~0,7× da amplitude
esperada. O conserto certo não é ganho de maquiagem: é **janela raiz-de-Hann**
(`sqrt(hann)`), que dá potência constante justamente para grãos
descorrelacionados.

**3. Cauda no corte.** `frz_on` 1→0 corta seco; soa como tomada arrancada, não
como gesto. Rampa de saída de ~300 ms, com os grãos rodando durante a descida.
*Decisão pendente do dB:* tempo fixo ou parâmetro `frz_release`?

### Também na fila, do mesmo desenho (CTOS protegendo a base)
- **Tirar `src_mix`, `gate_thresh` e `gov_ceiling` do mapa MIDI.** São
  roteamento e segurança — foram exatamente eles que emudeceram o instrumento
  no despejo fantasma. *O que não está mapeado não pode ser derrubado.*
- **Limite de taxa de mudança por parâmetro** vindo de MIDI: fader real anda
  em rampa; salto de 0 a 127 num CC é assinatura de máquina.
- **Relógio confiável pro watchdog:** `hal::NowUs()` devolve lixo
  esporadicamente (3 amostras descartadas em 12 min). O veto repousa nele.

---

## Fechado em 20/08 — o shimmer que ardia

Duas causas distintas, medidas na bancada offline (os headers são C++ puro,
então o host reproduz o laço exato):

**1. Aliasing no motor de pitch.** Ler o buffer a `ratio 2.0` dobra a
frequência; tudo acima de 12 kHz atravessa Nyquist e volta **inarmônico**. Com
3500 Hz na entrada, o pico mais alto de toda a cauda era 19963 Hz. Anti-alias
Butterworth de 6ª ordem **na escrita do motor** (não no shimmer — o defeito é
de ler rápido, e quem lê rápido é o motor): dobra de −11,9 para **−64 dB**.

**2. Conteúdo legítimo na faixa mais sensível do ouvido.** A planura espectral
de 2–6 kHz é 0,009 — tonal, não ruído. O 1–3 kHz da escultura vai a 2–6 kHz
pela oitava e o reverb sustenta lá. Conserto: **sino de −6 dB em 3 kHz**, com
a profundidade escolhida contra a razão medida da cauda seca (0,28), não de
ouvido.

| | v0.1.0 | v0.3.0 |
|---|---|---|
| razão (2–6 k)/(300–2 k) | 1,01 | **0,19** |
| centroide da cauda | 7134 Hz | **1907 Hz** |
| 6–12 kHz | 28,2% | **4%** |

`shm_tone` (**CC 57**) move as duas coisas e **sobe pra clarear**: 0 é o sino
cheio com corte em 6 kHz, 1 abre até 18 kHz. Zero tem que ser o estado bom, ou
a aspereza volta a cada boot.

---

## Fechado em 20/08 — teto de soma do CTOS

A última fronteira estrutural. O watchdog do §3 protegia contra **um efeito
com defeito**; faltava a proteção contra **N efeitos saudáveis que juntos não
cabem**. São situações de natureza diferente e exigem mecanismos diferentes —
a distinção está inteira em [CTOS.md §3b](CTOS.md).

**Não há culpado no teto de soma**, e é por isso que ele não procura um:
qualquer regra que tente achar "quem foi" quando não há quem acaba inventando
um culpado, e o inventado seria um efeito correto. A política é **prioridade
declarada a frio** pelo dB em `Manifest::sacrif[]` — a ordem do array *é* a
prioridade. LIFO foi descartado: pune por ordem de chegada, que não tem relação
com valor musical.

Três testes, todos passados na v0.84.0:

| Teste | Esperado | Resultado |
|---|---|---|
| teto 620 µs (bloco 639) | cai **só** o detune | ✅ bloco 639 → 526 µs, os outros cinco tocando |
| teto 200 µs | cascata na ordem + estado "sem saída" | ✅ detune→phaser→warble→drive→freeze→shimmer, depois *"a base sozinha não cabe"* |
| `ctos rearm` | solta latches **sem** religar | ✅ |

Com os seis efeitos ligados o bloco fica em **577–639 µs de pico ~700**, contra
teto de 850 (85% de 1000). A suíte inteira ainda cabe com folga. Mecanismo
custou **+14 µs**.

**Duas correções que os testes forçaram:**

1. **A serial morreu no primeiro teste** — eu chamava `LogAlways` e
   `MidiSendBytes` de dentro do `AudioCb`, que é o defeito de prioridade 1
   deste documento, catalogado antes de eu cometê-lo. Travou o instrumento
   inteiro, não só o CDC. Consertado: o callback marca (`volatile`), o loop
   fala.
2. **Derrubava dois quando um bastava.** Zerar o param não faz o efeito parar
   de custar — o `EffectSlot` desce por rampa e continua processando. O
   mecanismo media de novo antes da própria ação surtir efeito. Consertado com
   **carência de 250 ms** depois de cada queda.

---

## Defeitos abertos

**🔴 O p1 satura o conversor — e era isso o "p2 sem som".** Medido 20/08
com o dB tocando, entrada crua:

```
IN_L (p1): pico 1.00000 =  -0.0 dBFS |  42 365 amostras | clip 65
IN_R (p2): pico 0.46398 =  -6.7 dBFS | 244 237 amostras | clip 0
```

O p2 **não está mudo**: entrega −6.7 dBFS e tem quase 6× mais amostras ativas
que o p1. O p1 é que bate no teto do conversor (65 amostras em clip) ANTES de
qualquer ganho de firmware, e ocupa todo o espaço. É literalmente "um encobre
o outro" — o caso que o `piezo_mix` existe para resolver.

**Medido depois: OS DOIS grampeiam.** Batendo no p2 com `in_gain` em 1,0×,
o p2 crava em 0 dBFS com 252–542 amostras em clip e o p1 cai para −55 dBFS.
Batendo no p1, o inverso. Não é um transdutor desregulado — é o par.

**Pico grampeado não dimensiona atenuador.** A medição satura em 0 dBFS e o
pico real é "pelo menos isso". O `hist` passou a RECUSAR dar o número nesse
estado (v0.82.2) e manda atenuar 20 dB provisório primeiro — a versão anterior
imprimia "precisa tirar +6,0 dB", que é um piso com cara de resposta, e alguém
soldaria o divisor errado por causa dela.

**Não há caminho de software.** AK4556 não expõe controle de ganho; o PCM3060
só tem atenuação **digital**, pós-conversão, que reduz o número sem desfazer o
grampo; o WM8731 não tem setter no libDaisy. O divisor físico é obrigatório.

**Alvo −6 dBFS por piezo, não −1:** no centro do balanço os dois somam, e dois
piezos em −6 batidos juntos dão 0 dBFS. A folga é o espaço da soma — já visível
no histograma, que marcou 253 clips pós-ganho contra 252 no IN_R.

**Plano do dB, 8 passos — BLOQUEADO NO PASSO 2.**

| # | Quem | O quê | Estado |
|---|---|---|---|
| 1 | Code | histograma de entrada (`hist`) | ✅ **feito** — v0.82.2, commit `8e06635` |
| 2 | dB | medir o pico com atenuação provisória | 🔴 **bloqueado: faltam resistores de megaohm** (lab) |
| 3 | dB | divisor 10:1 em protoboard (90k/10k = −20 dB) | ⏳ |
| 4 | Code + dB | confirmar pico ~−6 dBFS sem clip | ⏳ |
| 5 | dB | soldar definitivo, comutável 0/−6/−12 por piezo | ⏳ |
| 6 | dB | recalibrar `in_gain` sobre sinal limpo | ⏳ |
| 7 | Code | esticar a faixa do `in_gain` se 8× virar teto | ⏳ — decisão de desenho aberta: esticar a faixa muda o significado de todos os CCs já decorados |
| 8 | dB | reconfirmar os cinco efeitos | ⏳ — o aceite do `piezo_mix`/`out_width` cai aqui |

**Nada do passo 3 em diante avança sem o divisor.** O diagnóstico está
completo e a ferramenta está commitada; o que falta é peça física.

**CORRIGIDO 20/08:** o `in_gain` **não** nasce em 8,0. O default é **1,0×**
(faixa 1–8×, curva quadrática), escrito na mesma linha do `Add()`. O 8,0 que
figurava aqui como defeito era **ajuste do dB**, lido de placa viva e tratado
como default do projeto.

**🔴 A serial morre com o vocoder ligado — prioridade 1.**
As rajadas de ~3,4 ms do callback starvam a ISR do USB; o CDC do host
apodrece. Contornado por `log_mute`, caixa-preta e MIDI, mas o conserto
certo está desenhado e não feito: **ring de log kick-and-forget**, em que
o loop nunca toca o USB e a drenagem acontece por ISR.

**🟡 Vocoder exige bloco 256.** 5,3 ms de latência num instrumento
realimentado desloca onde o feedback se estabelece. Preço aceito, não
resolvido.

**🟢 Off-by-one no contador de aplicadas.** `msgs completas N` vem sempre
**exatamente 1** acima de `APLICADAS`, com e sem carga (65/64 e
2779/2778). Diferença fixa, não taxa de perda — provavelmente a mensagem
em trânsito no instante da leitura. Confirmar antes de chamar de cosmético.

**🟡 Pool SRAM a 92,4%.** Terceiro módulo não entra sem mudar a
estratégia de alocação (rotular alocações quente/fria antes de crescer).

---

## Fila congelada (espera hardware)

T4 (buffer + piezo) · T7e (gate com transiente real) · teste percussivo do
limiter · **solda do painel v2** (5 pots + 2 botões; B2 é candidato natural pro toggle dos
Búzios).

---

## Próximos passos, em ordem

1. **Decisão do dB:** quanto de proteção volta → grava o estado
   instrumento como default do repo.
2. **Ring de log** — mata o defeito prioridade 1 por arquitetura.
3. **Defaults de palco:** `voc_mix`, `voc_tone`, `buzios_thresh`,
   `buzios_cooldown`.
4. **Painel v2** quando o hardware chegar (T11 já testa por serial).
