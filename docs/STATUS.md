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

**Moram no mapa de arquivos do [README.md](../README.md), em um lugar só.**

Esta seção já teve uma tabela própria. Ela chegou a `main.cpp v0.85.1`
enquanto a placa rodava a v0.130.0 — **45 versões de deriva**, porque era uma
segunda cópia que ninguém imprimia e ninguém lia. Mesma forma que o
cabeçalho do `main.cpp` carregava (cicatriz 13k e a regra do checklist):
*número que ninguém imprime não tem quem o corrija.*

A versão que vale é a que a **placa diz no boot**, e o `flash.sh` a confere
contra o binário depois de gravar.

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
<!-- doc-check: ignore frz_release (proposta nao implementada) -->
> **Nota:** `frz_release` é **proposta**, não parâmetro. A marca acima o
> declara ao verificador de deriva — fantasma declarado deixa de ser
> fantasma; fantasma silencioso quebra o build.

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

## Pivô de 20/08 — duas cadeias de caráter, medido

```
p1 → in_gain_A → gate_A → [DRIVE·PHASER·WARBLE] ┐
                                                 ├→ MIX → [DELAY·FREEZE·SHIMMER↺REVERB]
p2 → in_gain_B → gate_B → [DRIVE·PHASER·WARBLE] ┘   → level → OUT_WIDTH → split
```

Firmware **v0.91.0** · **71/96 params · 36/48 CCs** · SDRAM liberada: 153,6 KB.

| | µs |
|---|---|
| tudo ligado | **620 média / 663 pico (66%)** |
| base (nada ligado) | 160 |
| os seis de caráter | ~130 (90 o primeiro a ligar, 38 o segundo — I-cache) |
| cauda (reverb 99 + freeze 69 + shimmer 202) | 370 |

**A assimetria entre cadeias era I-cache, não código.** Quem liga primeiro
paga ~90, quem liga depois ~38, seja qual for a cadeia — três corridas
concordantes com a ordem invertida. As duas cadeias são simétricas.

**A base decomposta matou o caso de otimização:** dos 182 µs originais, o
desperdício real eram os dois gates com o portão aberto. Módulo do delay (45),
osso da dinâmica (94), medição (15) e escritas do warble (7) estão todos
fazendo algo. Guarda `thresh > 0` implementado — **26–30 µs medidos A/B no
mesmo binário**.

**Aberto:** aceite de ouvido no instrumento híbrido, e IFÁ10000 dos seis
efeitos nas novas posições (não herdam o aceite da cadeia antiga).

---

## Fechado em 20/08 — RNBO fora, inteiro

O delay era o último consumidor do runtime. Reescrito em C++ (`delay.h`, grafo
lido do export: feedback fixo **0,4**, crossfade, mono nos dois canais), o
RNBO saiu por completo.

| | antes | depois |
|---|---|---|
| tudo ligado | 678 µs | **545 µs / 656 pico** |
| base (nada ligado) | 182 µs | **115 µs** |
| o delay | 45 µs, **sempre** | +11 µs, só ligado |
| binário | 212 424 B | 175 980 B |
| params | 61 | **54** |
| repo | — | **−8 MB** de export |

**Os 67 µs da base são o dado que interessa:** um `Module::Process` roda ligado
ou desligado, porque o módulo não sabe que está em bypass. Custo-zero-desligado,
que todo efeito da casa segue, era **estruturalmente impossível** dentro do
sistema de slots.

Saíram junto: `MODULE_SRC`, `RNBO_DIR`, `module.h`, `mod_slot[]`, `ModBind` e o
push-por-nome, seis arquivos, quatro exports, os seis params órfãos do vocoder,
o watchdog do fx2 que julgava pelo CPU global (**achado 8**), o comando `probe`
e o histograma nota-a-nota (**achado 7**).

**Método na cicatriz 12f:** remover o símbolo-raiz e deixar o compilador
enumerar os dependentes. Quatro erros na primeira passada, depois um, depois
zero — sem faixa calculada e sem prosa perdida.

**Mapa de CC refeito:** todo param recebe CC (54/64), uma década por efeito.
Nada mais só-serial.

---

## Fechado em 20/08 — cauda PARALELA e dois espaços

O dB pediu p1 com delay e p2 com reverb, e depois corda com shimmer e couro
com sala limpa. A cauda serial não conseguia nenhum dos dois.

```
A ─┬→ ×dly_a ──┐
B ─┼→ ×dly_b ──┴→ DELAY  (uma instância) ──┐
   ├→ ×frz_a ──┐                           │
   ├→ ×frz_b ──┴→ FREEZE (uma instância) ──┤→ soma → level → split
   ├→ ×esp_a ───→ [SHIMMER_A ↺ SALA_A] ────┤
   └→ ×esp_b ───→ [SHIMMER_B ↺ SALA_B] ────┤
   └→ seco (as duas, inteiro) ─────────────┘
```

**Delay e freeze continuam sendo UM**, com envio por cadeia — o gesto pedido
era sobre espaço, e duplicá-los custaria CPU sem entregar diferença. **O
espaço duplica de verdade**, porque shimmer não é um efeito ao lado do reverb:
ele *é* o reverb realimentado com pitch, e duas molhaduras exigem duas salas.

| | previsto | medido |
|---|---|---|
| A: delay+shimmer · B: sala limpa | 625 µs | **490 / 530 pico — 53%** |
| pior caso: as duas com shimmer e sala | 807 µs | **598 / 639 pico — 64%** |

O pior caso ficou **209 µs abaixo** da previsão: somei custos medidos na cauda
serial, onde cada efeito processava o sinal inteiro. Em paralelo cada um
recebe só o seu envio — **duas salas custam menos que o dobro de uma**
(cicatriz 12g).

**Consequências declaradas:** o delay deixou de ser reverberado (os ecos saem
secos, a sala trabalha sobre a fonte); os mixes internos viraram envios por
cadeia; e o freeze virou **dois independentes**, um por cadeia, com buffer
próprio: `frz_on_{a,b}` **são** o gesto de congelar, com `frz_vol` e
`frz_size` por cadeia.

> **Correção de deriva (22/08).** Este parágrafo descrevia por meses um
> desenho de "fonte e gesto separados", com um `frz_hold` que **nunca
> existiu como parâmetro** — o `manifest.h:264` já registrava a decisão
> contrária e a doc ficou no modelo antigo. Achado pelo audit de deriva
> doc↔código; ver bts 13h.

**Proteção anti-feedback duplicada** para o ramo B (bloqueante do hardware de
dois exciters): governor e boost por ramo, `shift_hz` compartilhado — dois
exciters no mesmo corpo com deslocamentos diferentes bateriam um contra o
outro. **Painel — pinos (22/08).** D26 = B1 randomizador, D27 = B2 assinalável,
**D25 = B3 rearme do CTOS** (novo em v0.111.0). D28 = LB do WIDI.
D29/D30 reservados pras chaves DPDT dos excitadores. Os três botões usam
pull-up interno e são ativos em baixo, então pino sem botão lê "solto".

**O LED do Seed carrega dois fatos numa cadência só:** devagar = loop
vivo e sem trava; **4× mais rápido = tem efeito derrubado pelo teto,
aperte B3**; congelado = loop morto. O alarme entrou na cadência
justamente pra não gastar o estado, que é a prova de aliveness.

**Aberto, e é o mais caro agora: não há preset em flash.** Toda gravação
zera todo parâmetro e `level` nasce em 0. A perda é silenciosa dos dois
lados e o sintoma imita defeito de efeito — custou dois diagnósticos
falsos em 22/08 (bts 12l).

`exc_on_a/b` gateiam a proteção; quando os DPDT forem soldados, D29/D30
substituem o param pelo estado real.

**Simplificação depois (v0.106.0):** `esp_on/esp_mix` e o `rev_mix` antigo
eram **o mesmo controle duas vezes** — ganho na entrada e ganho na saída da
mesma sala linear. O dB viu antes de mim. Ficou um envio só por destino
(`rev_on`/`rev_mix`, mesma forma do delay e do freeze), retorno inteiro, e
`shm_mix` de volta ao emprego único. Cicatriz 12h.

**Mapa de CC: 75 params, 75 CCs, nada só-serial.** Uma faixa por família, e
todo destino com a mesma forma: `on_a, mix_a, on_b, mix_b`, depois os
compartilhados.

---

## Auditoria de params (20/08) — uma feature estava morta

O dB pediu auditoria de todos os params, um por um. Três camadas, quinze
minutos:

| camada | resultado |
|---|---|
| quem lê cada param | 🔴 **detector dos búzios apagado havia 5 commits** |
| faixa × trava interna | 🟠 `frz_size` com 61% de curso morto (corrigido) |
| varredura viva min/max | ✅ 75/75 atingem os extremos declarados |

**Os búzios estavam mortos desde `6a8175d`** — o detector vivia pós-gate,
dentro da faixa que substituí na reestruturação em duas cadeias. Nada
denunciou: as variáveis seguiram declaradas e lidas no loop, os params na
tabela, os CCs no mapa apontando pro nada, e o `board` reportando tudo certo.

**Feature morta parece feature desligada** — e `buzios_on` nasce em 0, então o
estado normal era o estado quebrado. Restaurado do `3303028` e testado: oito
pulsos, oito sorteios. A posição mudou de significado — agora lê a soma das
duas cadeias, e uma batida em qualquer piezo dispara.

**Falso positivo registrado:** a camada 2 acusou `drv_drive`, `drv_tone` e
`phs_rate` de alcance desperdiçado. São deliberados. A trava do efeito é rede
de segurança, não alvo — só faixa MAIOR que a trava é defeito.

A auditoria virou seção do IFÁ10000, para rodar depois de qualquer refactor
que toque no callback.

---

## Onde estamos (23/08, v0.155.0)

**A cadeia inteira do controlador está provada, e o painel acendeu.**

| frente | estado |
|---|---|
| params de caráter por cadeia | ✅ custo indistinguível, medido com A/B no mesmo binário |
| endereçamento por canal (ch1/ch2) | ✅ 37 pares, banco nativo |
| espelho em ch3/ch4 | ✅ MIDI Wrench viu os dois |
| fio de volta (iPad → placa) | ✅ 93 CCs contados |
| presets em RAM | ✅ ciclo `capturar → dump → reset → colar → load` |
| botão de freeze B (D22/D23) | ✅ 4 batidas para 4 apertos |
| rearme do CTOS pelo B3 | ✅ ressuscitado — tinha ramo inalcançável |
| OLED SSD1309 | ✅ acende e escreve. **Configurado, nunca "presente"** |

**O que falta é humano:** remapear o Loopy (mapa inteiro em
[RETRABALHO-LOOPY.md](operacao/RETRABALHO-LOOPY.md)) e a **escuta que
constrói a biblioteca** — cada julgamento vira um preset em vez de uma
anotação.

## Painel físico (23/08) — encoders vivos, OLED morto

**Os três encoders EC11 funcionam**, com o mapa conferido por medição:

| encoder | fase A | fase B | push |
|---|---|---|---|
| cadeia A | D0 | D1 | D2 |
| cadeia B | D3 | D4 | D9 |
| global | **D18** | **D19** | **D17** |

**Um pulso por detente, medido** — dez cliques deram dez. A divisão fica em 1.

**O OLED não sobreviveu.** GND e 3V3 estavam invertidos na placa de
distribuição, e o módulo recebeu polaridade reversa. Sete causas foram
eliminadas por prova — firmware (o binário que funcionou também dá preto),
alimentação, encoders, fio rompido, ponte, velocidade e ordem de energização —
e tudo que divide a mesma placa de alimentação funciona.

**Ao trocar o painel, o teste é de um minuto:** `oled` acende tudo,
`oled texto ola` prova endereçamento de pixel e o DC.

### Sondas que ficaram, e servem a cada solda nova

| comando | o que responde |
|---|---|
| `varre [reset]` | **quais dos 15 pinos livres têm algo que se move** — não supõe mapa |
| `encpino [reset]` | nível cru dos nove pinos, trava nas duas direções |
| `fio <pino> <0\|1>` | segura um pino do OLED para medir no conector |
| `fio curto` | ponte entre os cinco, teste diferencial |
| `oledvel <n>` | prescaler do SPI em tempo de execução |

Todas acumulam desde o reset — **nenhuma abre janela cronometrada.**

### Revisão de pinos entra na rotina de fechamento

`make check-docs` confere `PINOS.md` contra os `seed::Dnn` do fonte, em três
colunas: **no código e fora do doc · no doc e não usado · RESERVADO usado.**
Pedido do dB, e a razão é dele: *a placa está entupida e os pontos de solda
espremidos — conferir de cabeça falha exatamente quando errar custa mais.*

---

## Frente na fila — ampliar os búzios (pedido do dB, 23/08, "mas não ainda")

**Os búzios estão com DUAS cartas** — `dly_time` e `dly_mix_a`, num array que
cabe 8. O manifest já registrava isso como empobrecimento conhecido, herdado
da saída do vocoder do build:

> *"Os búzios ficaram com DUAS cartas em vez de quatro, e o sorteio soa
> proporcionalmente mais pobre. Não há conserto silencioso pra isso — ou volta
> um módulo de caráter, ou a decisão é aceitar o sorteio menor."*

**Há um terceiro caminho que aquele texto não previu:** a duplicação de
caráter do passo 1 criou **dez params novos da categoria certa**. Não é
"randomizar quase tudo" — é **preencher a categoria que já estava definida e
estava vazia.**

### Onde parar já está decidido, e as razões são de música

| categoria | fora | por quê |
|---|---|---|
| **MOLDURA** | reverb | o fundo parado que faz o contraste existir — *se tudo salta, nada é surpresa* |
| **GESTO** | freeze | decisão de execução com hora certa; sorteada vira acidente |
| **SEGURANÇA** | level, gate, governor, limiter, shift | — |

**Candidatos legítimos (20, contra os 2 de hoje):** `drv_drive`, `drv_tone`,
`drv_sujeira`, `phs_rate`, `phs_depth`, `phs_fb`, `phs_stages`, `wrb_rate`,
`wrb_depth`, `wrb_irreg` — nas duas cadeias.

### O que o trabalho realmente é

Subir o teto do array é trivial. **O trabalho é a FAIXA de sorteio de cada
um**, e ela é de ouvido: sortear `drv_drive` no curso inteiro produz do limpo
ao destruído a cada batida. O registro do `voc_pitch` mostra o padrão — faixa
estreita, centro escolhido, e no caso dele até quantização em escala.

E o aviso que o próprio manifest deixou: *"o registro anterior sorteava certo
mas soava tudo grave e parecido — o registro aprovado de ouvido foi o de
cima."* **Faixa de sorteio é decisão de música, e o aceite é do músico.**

---

**Duas frentes pequenas com argumento próprio, ambas de "dar testemunha ao
que só a disciplina protege":**

- **Tabela de pinos RESERVADOS em código**, com `static_assert` sobre os pinos
  efetivamente configurados. Hoje `D29/D30` estão reservados aos DPDT do
  exciter **só em documento — e o compilador não lê documento.** O default do
  RST do OLED é justamente `D30`: se a solda tivesse seguido o default, a
  colisão só apareceria no dia de montar o painel, com o fio na caixa, e nada
  a teria pego antes.
- **`defesas`**: inventário do que nunca foi exercitado (bts 13w), três
  colunas — já agiu · tem gatilho · nenhum dos dois.

**Na fila depois disso:** inventário do que nunca foi exercitado (bts 13w) ·
unificar as duas recusas temporais globais · ponteiros de preset com
pré-escuta · painel no OLED · QSPI (janela ITCM provada, falta a leitura
indireta — não gastar gravação sem o RM0433).

---

## Rota dos presets (22/08) — o difícil resolvido, o tedioso identificado

**A janela ITCM funciona.** A placa sai do memory-mapped executando de RAM
independente, dialoga com o periférico, volta e sobrevive — seis gravações,
`ITCMRAM: 212 B` provando que a seção foi aplicada. **Era o risco
arquitetural da rota dos presets, e ele acabou.**

**O que falta é protocolo**, que é trabalho de leitura: a sequência exata de
leitura indireta do QUADSPI. Três hipóteses vivas e nomeadas, condição de
retomada explícita — tudo em
[DESENHO-ESCRITA-QSPI.md](DESENHO-ESCRITA-QSPI.md).

> **Condição de retomada: RM0433, capítulo QUADSPI, leitura indireta.** Sem
> ele a frente fica parada — por dimensionamento, não por desistência. Meia
> hora com o manual vale mais que dez gravações adivinhando.

O BOOT_SRAM segue estacionado no ramo `investigacao-boot-sram`, com a
hipótese registrada para quem retomar.

---

## Defeitos abertos

**✅ FECHADO 22/08 — a entrada NÃO satura, e o divisor saiu da fila.**

Medido com os piezos **montados na escultura**, que é o regime de uso:

```
p1 (IN_L): pico  -9,2 dBFS | 18,0% das amostras ativas | clip 0
p2 (IN_R): pico -19,5 dBFS |  1,3% das amostras ativas | clip 0
pos-ganho (in_gain 2,8x):   -0,3 dBFS                  | clip 0
```

**Nenhum clip em lugar nenhum.** O plano de oito passos encolheu para dois: a
proteção (1 kΩ + 2× 1N4148), que serve de qualquer jeito porque piezo entrega
dezenas de volts num transiente forte, e recalibrar o `in_gain`.

**A cicatriz 12a acertou o desfecho.** A medição de 20/08 que dizia "a entrada
satura" foi feita com **dedo direto no disco solto** — o caso mais violento que
a pastilha encontra. Montada e excitada por vibração, ela entrega vinte dB a
menos. *"Dimensionar antes de medir no regime certo seria resolver um problema
que talvez não exista"* — e não existia.

**Dois pontos de atenção que a medição deixou:**

**O pós-ganho está a três décimos do teto** (−0,3 dBFS com `in_gain` 2,8×). Uma
batida mais forte clipa. Há folga na entrada, não na saída do ganho.

**Dez dB entre os dois piezos, e p1 com 14× mais amostras ativas.** Pode ser
execução ou acoplamento, e a medição não distingue — a separação é **trocar os
piezos de canal** (cicatriz 12b). Enquanto não se faz, `in_gain_a/b` existem
justamente para casar os dois.

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
