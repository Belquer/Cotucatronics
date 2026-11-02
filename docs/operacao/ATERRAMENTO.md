# ATERRAMENTO — o capítulo

**v0.2.0** · escrito em 2026-08-31, depois de uma tarde a caçar três ruídos
que tinham três causas diferentes e um sintoma parecido.

> Este é o documento **dono** do assunto. O `HARDWARE.md` descreve *este*
> instrumento; este descreve **a regra**. Quando os dois discordarem, é este
> que manda, e o outro que se corrige.

A revisão de terra durante o desenho de placa tem skill própria:
`.claude/skills/revisao-terra/`.

---

## 1 · O princípio único

Quase tudo neste capítulo sai de uma equação:

```
V = L · di/dt
```

**Fio tem indutância.** Corrente a mudar depressa, ao percorrer um fio, cria
tensão ao longo dele. Essa tensão não fica no fio: é vista por **tudo o que
estiver pendurado nele**.

Daqui nasce o conceito que decide tudo o resto:

> ⚠️ **IMPEDÂNCIA PARTILHADA.** Quando a corrente de retorno de um aparelho
> percorre um troço de cobre que **outro aparelho usa como referência**, a
> queda de tensão nesse troço vira **sinal** para o segundo.

E daqui nasce a inversão que custa a interiorizar:

> **Terra não é um sítio. É um caminho.**
>
> Ligar dois pontos ao "terra" não diz nada. A pergunta útil é sempre
> **por onde volta a corrente**, e **quem está sentado nesse caminho**.

---

## 2 · Taxonomia — três tipos de terra

**Todo condutor de terra do projecto é exactamente um destes três.** Um que não
se consiga classificar é um defeito, não uma excepção.

| tipo | o que é | corrente | onde vai |
|---|---|---|---|
| **força** | volta da corrente que o aparelho consome | alta, pulsada | **fio próprio à estrela** |
| **referência lógica** | zero contra o qual o ADC e os pinos digitais medem | ~nenhuma | **fica no aparelho que lê** |
| **referência de áudio** | zero que viaja junto com o sinal | ~nenhuma | **ilha de `AGND`** |

⚠️ **A regra da estrela aplica-se só ao primeiro tipo.** Os outros dois não são
caminhos de retorno — são **referências**, e uma referência quer estar o mais
perto possível de quem a lê, não o mais perto do ponto de estrela.

Confundir os dois é o erro mais comum, e é sedutor: "estrela é bom, então
levo tudo à estrela". Levar a referência de um pot à estrela é **injectar-lhe**
a diferença de tensão entre a estrela e o MCU que o mede.

**Como decidir, numa pergunta:** *este condutor transporta corrente, ou define
um zero?* Transporta → estrela. Define → fica onde é lido.

---

## 3 · A estrela

**Um ponto físico** onde chegam os retornos de força, cada um pelo seu próprio
fio. Não um trilho por onde todos passam.

```
   MARGARIDA (errado)              ESTRELA (certo)
   ┌───┐  ┌───┐  ┌───┐              ┌───┐  ┌───┐  ┌───┐
   │ A │──│ B │──│ C │              │ A │  │ B │  │ C │
   └───┘  └───┘  └───┘              └─┬─┘  └─┬─┘  └─┬─┘
     └──────┴──────┴──> fonte          └──────┼──────┘
                                              ▼
   a corrente de C atravessa           cada um volta por si;
   o troço onde B se referencia        ninguém pisa o caminho do outro
```

**Onde fica a estrela:** no ponto onde o negativo da alimentação entra no
maior consumidor. Neste instrumento, é onde o `GND` do decoy USB-C PD chega ao
amp — e é daí que bifurca para o step-down.

**O retorno do amp é o fio mais curto e mais grosso de todos.** É ele que
transporta a corrente pulsada.

🚫 **Terra nunca passa por chave.** Interruptores só no positivo. Uma chave no
retorno acrescenta resistência de contacto variável ao caminho de todos.

---

## 4 · Os condensadores de desacoplamento

### O que fazem

Não filtram. São um **reservatório local de energia**.

Quando um consumidor pede uma rajada de corrente, ela vem do condensador
colado nos pinos — um laço de dois centímetros — em vez de vir da fonte, por
trinta centímetros de fio com indutância. **A corrente rápida nunca chega a
percorrer o fio comprido**, e por isso não cria tensão nele.

### Por que são sempre DOIS

Não são redundantes. Cobrem **bandas diferentes**:

| | banda | porquê |
|---|---|---|
| **electrolítico grande** | lenta a média (µs–ms) | muita energia, mas lento: tem ESR e ESL próprias |
| **cerâmico 100 nF** | rápida (ns–µs) | pouca energia, mas responde às arestas que o grande nem vê |

Só o grande deixa passar as arestas. Só o pequeno não tem energia para o
transiente. **Sempre o par, em paralelo, juntos.**

### O valor depende do TIPO DE CARGA, não do trilho

A pergunta *"precisa de condensador?"* não tem uma resposta para o sistema
inteiro:

| carga | exemplo | precisa | o que acontece se errar |
|---|---|---|---|
| **corrente pulsada audível** | amp classe D | **2200 µF + 100 nF** nos pinos | zumbido quando outro consumidor entra no trilho |
| **potência constante** | conversor comutado | **nada de grande na entrada** | **guincho** — oscila contra a indutância |
| **rajadas de rádio** | ESP32, qualquer BLE/WiFi | **100 µF + 100 nF** locais | zumbido e clique — o rádio sai pelo altifalante |
| **lógica** | MCU | **100 µF + 100 nF** locais | ruído difuso, difícil de atribuir |

### 🚫 A proibição — banco grande na entrada de conversor comutado

Um conversor comutado é **carga de potência constante**: se a tensão de entrada
cai, ele puxa **mais** corrente. Isso é uma **impedância de entrada negativa**.

Junta-lhe um banco grande de baixa ESR e a indutância da fiação, e tens um
circuito ressonante com amortecimento negativo. **Oscila** — é o critério de
Middlebrook.

Medido em 2026-08-31: 2200 µF na entrada do step-down produziu um **guincho agudo**
imediato, e removê-lo curou-o no mesmo gesto. Os módulos já trazem o que
precisam à entrada; não lhes acrescentar carga.

Se um dia fizer falta lá: valor pequeno **com** ESR, ou um RC de amortecimento.
Nunca 2200 µF de baixa ESR.

⚠️ **A mesma cautela vale para a SAÍDA.** Quem se pendura no 5 V de um
step-down leva **100 µF**, não 2200 — capacitância a mais na saída também mexe
na estabilidade da malha do conversor. Para rajadas de rádio o que importa não
é ser grande: é estar **perto**.

### As regras físicas

**📍 Colado nos pinos.** Um condensador a 10 cm liga-se através de 10 cm de
indutância — que é exactamente a coisa que ele existe para contornar. Longe, é
como se não estivesse lá.

**Polaridade.** Electrolítico tem risca no corpo = negativo = `GND`. Ao
contrário, aquece e rebenta. Cerâmico não tem polaridade.

**Tensão.** Pelo menos 1,5–2× o trilho. 15 V → usar 25 V ou 35 V. 5 V → 16 V.

⚠️ **Inrush.** 2200 µF no arranque é um pico considerável, e alguns adaptadores
PD lêem isso como curto e re-negoceiam. Se o amp não arrancar de forma fiável
ao ligar, é isto — não um defeito novo.

---

## 5 · `AGND` e `DGND` — a ponte estreita

> ⚠️ **CORRIGIDO EM 2026-09-04, E A VERSÃO ANTERIOR DESTA SECÇÃO ESTAVA ERRADA.**
> Ela afirmava que a Daisy junta `AGND` e `DGND` internamente. **Não junta.**
> A documentação da libDaisy diz que se devem **sempre ligar os pinos `DGND` e
> `AGND` fora da Daisy**, e a Seed expõe-os em dois pinos distintos do header
> — 20 e 40, conferidos no `Daisy_Seed_Rev4_Pinout.csv` do fabricante.
>
> O erro foi afirmado com confiança e repetido, e mandou a análise do ruído
> branco na direcção errada. Ver `battle_scars.md`.

**A ponte tem de existir, e é nossa para a fazer.** A regra geral de
conversores (Analog Devices) diz onde: a ligação óptima do `DGND` é ao `AGND`
**no próprio encapsulamento**, ao mesmo potencial, **e em mais lado nenhum** —
juntar ali minimiza a diferença de potencial entre os dois terras do
conversor, que é o que injecta ruído digital no analógico.

🚫 **O que continua proibido é ela ser MAIS DO QUE UMA, e ser um trilho.** Dois
pontos de encontro formam um anel, e a corrente divide-se pelos dois caminhos
— a fracção que escolhe o de dentro passa **exactamente** pela referência do
áudio.

```
   ERRADO — dois encontros, e um deles partilha corrente
        ┌──── fio curto nos pinos 20/40 (a ponte que queremos) ────┐
   AGND ┤                                                          ├ DGND
        └──────── trilho da permaproto (a ligação A MAIS) ─────────┘

   CERTO — um encontro só, e num fio curto
   AGND ──── pino 20 ──[ NetTie-2 ]── pino 40 ──── DGND
```

> **O anel do P3 não era a ponte existir: era ela ser um trilho partilhado com
> corrente, em vez de um fio curto nos pinos.** E por isso o conserto é
> **MOVER** a ponte, nunca removê-la — remover deixa o codec sem referência,
> que foi o que em 2026-08-31 "trocou um zumbido por uma placa morta".

🚫 **Nunca puxar a alimentação pelo `AGND`.** O retorno de toda a placa
atravessaria a ponte estreita e aterraria na referência do codec — o oposto
exacto do que o pino existe para fazer. **A força entra pelo `DGND`.**

### O desenho alvo

```
   ILHA DE ÁUDIO — só sinal, corrente nenhuma
     AGND ── entrada dos piezos
          ── comuns dos DPDT
          ── sleeve dos jacks
          ── terra de entrada do amp
          ↑ encontra o DGND num ponto SÓ: o net-tie nos pinos 20/40
            do soquete. NAO por dentro da Daisy — ela nao os junta.

   TRILHO DE FORÇA E LÓGICA
     DGND ── fio próprio à estrela
          ── pots (com o 3V3 vindo da mesma placa que os lê)
          ── encoders, botões
          ── ponte ESP32 (ou fio próprio dela à estrela)
```

⚠️ **O 3V3 dos pots sai da mesma placa que os mede.** O par anda junto: a
referência de cima e a de baixo vêm ambas de quem faz a medição. Alimentar os
pots de uma fonte e referenciá-los noutra injecta a diferença entre as duas
directamente na leitura.

**O número que mostra o custo:** mediu-se **42 mV** entre a terra da Daisy e a
estrela antes do fio dedicado. Sobre uma referência de 3,3 V isso é **1,3% do
curso — cerca de 1,6 passos de MIDI**, a mexer sozinho.

### Conferência — TRÊS medições, e por esta ordem

> ⚠️ **CORRIGIDO EM 2026-09-05.** A versão anterior desta secção mandava medir a
> ilha contra o trilho com a placa fora do soquete e dizia *"se apitar, há uma
> ligação a mais"*. **Com o net-tie desenhado, apitar é o CORRECTO** — e o
> teste velho faria ler uma placa certa como defeituosa. O teste vivo é o do
> [`MAPA-DE-TERRA.md`](esquematicos/revB/MAPA-DE-TERRA.md) §6, reproduzido
> aqui porque é este o documento que manda no assunto.

**1 · A premissa, na Daisy nua.** Continuidade entre o pino 20 (`AGND`) e o
pino 40 (`GND`), com a placa fora de tudo.

| resultado | significa |
|---|---|
| **aberto** | ✅ **medido em 2026-09-04.** A ponte é nossa para fazer |
| **apita** | 🔴 seria outra revisão. Parar e repensar |

**2 · A ponte é única?** Sem alimentação, **levantar uma perna do net-tie**
(ou ainda não o povoar). Ponta A na ilha de `AGND` (sleeve de um jack de
piezo), ponta B no plano de `GND` (terra do módulo do amp).

| resultado | veredito |
|---|---|
| **abre** | ✅ a ponte é única — só existe através do net-tie |
| **continua a apitar** | 🔴 há uma segunda ponte. Não montar. Achar e cortar |

**3 · A ponte existe?** Net-tie reposto, ainda sem alimentação, mesmas pontas.

| resultado | veredito |
|---|---|
| **apita** | ✅ a ponte existe e passa pelo net-tie, como desenhado |
| **abre** | 🔴 a ilha está flutuante — o codec sem referência |

⚠️ **Os três, e por ordem.** O passo 2 sozinho não distingue *"ponte única"*
de *"nenhuma ponte"*, e uma delas mata o áudio. **Um teste que não separa os
dois desfechos não é teste.**

### ⚠️ Separar as fontes de alimentação PIORA — medido em 2026-09-01

Tentação óbvia: se o trilho partilhado acopla, dá-se a cada um a sua fonte.
**Três voos, e a tendência é monótona na direcção errada.** Ruído em 8–16 kHz:

| topologia | ruído |
|---|---|
| **trilho único** | **+9,6 dB** ← melhor |
| Daisy na parede | +17,3 dB |
| dois powerbanks | +24,3 dB |

O mecanismo: separadas as alimentações, **o cabo de áudio passa a ser o único
elo de terra entre o amp e a Daisy**. Toda a corrente de retorno que antes tinha
o trilho passa a viajar pelo condutor que carrega o sinal — que é a definição de
impedância partilhada, agora concentrada no pior sítio possível.

**Veredito: trilho único.** E a resposta ao ruído que sobra **não é separação
nem filtro em software — é encadeamento de ganho**: entrar quente na dinâmica de
saída (`pre_level_ab` a 1.0) e dosar depois do limitador (`post_level_l/r`, nos
pots). A arquitectura já o faz.

### O que isto não resolve, e é honesto dizer

Amps de entrada não-balanceada ligam o terra de entrada ao de força **dentro
do módulo**. A ilha acaba ligada à estrela por lá, e o anel fecha-se por fora.
**Não há como evitar** sem entrada diferencial.

O ganho real é outro: **o retorno do áudio deixa de partilhar cobre com o
retorno da alimentação.** É a impedância partilhada que faz ruído virar sinal,
não o anel em si.

---

## 5b · Sinal entre duas placas: a terra vai JUNTO com o fio

🔴 **Esta secção nasceu de um conselho errado meu, em 2026-08-31.** Eu disse:

> *"Não acrescentes um fio de terra ao lado dos fios da UART. Isso criaria um
> segundo caminho entre a ponte e a Daisy, em paralelo com o da estrela."*

O raciocínio era o da §2 — terra de sinal é **referência**, e referência não vai
à estrela. Mas eu esqueci o que torna este caso diferente, e o custo foi
**a Daisy deixar de arrancar** e uma tarde de caça a um defeito de firmware que
não existia.

### A regra

> **Duas placas que trocam sinais e podem ser alimentadas SEPARADAMENTE
> precisam de um laço de terra DIRECTO entre elas, ao lado dos fios de sinal.**

O que falha sem ele: a Daisy vem do USB do portátil, a ponte vem do trilho. Com
o trilho **desligado**, a ponte fica sem alimentação **e sem referência** — e o
`TX` dela, ainda ligado ao `D14` da Daisy, passa a injectar corrente pelos
díodos de protecção de um chip morto. A linha fica indefinida, a USART entra em
erro de enquadramento, e quem tenta **transmitir** por ela fica preso.

⚠️ **Repara que o defeito aparece do lado ALIMENTADO.** A placa viva é que
trava, por causa da morta. Isso torna o diagnóstico traiçoeiro: procura-se o
defeito onde ele se manifesta, e ele mora do outro lado do cabo.

### ⚠️ O que o teste mostrou, e corrige esta secção

Escrevi acima que a falta do laço directo era **a causa** da Daisy não arrancar.
**O teste não sustenta isso.** Medido em 2026-08-31, ao fim do dia:

| condição | resultado |
|---|---|
| ponte alimentada, com laço | ✅ arranca, série abre em 0,0 s |
| ponte **sem alimentação**, com laço directo re-soldado | ❌ trava — série leva 59 s e fica muda |
| ponte sem alimentação, terra pela Daisy **ou** directo | ❌ trava nos dois |

**A ponte tem de estar ALIMENTADA.** Terra sozinha não chega: um chip sem
alimentação pendurado na linha de RX continua a estragar a USART, mesmo com
referência comum.

A regra do laço directo **mantém-se como boa prática** — sem ela não há sequer
referência, e o comportamento fica indefinido em vez de meramente mau. Mas ela
**não era a cura**, e eu apresentei-a como se fosse antes de a testar.

### E o laço que eu queria evitar?

Existe, e é real — mas é o problema **menor**, e tem conserto próprio:

| problema | conserto certo |
|---|---|
| corrente do rádio pela terra partilhada | **condensador local** (100 µF + 100 nF nos pinos) — mata a corrente onde ela nasce |
| falta de referência entre placas | **laço de terra directo**, ao lado do sinal |

Tirar a terra de sinal para resolver ruído é trocar um zumbido por uma placa
morta. **O desacoplamento é que resolve o ruído; a terra de sinal não se
negoceia.**

### Como reconhecer o caso

Perguntar de cada par de placas ligadas por sinal:

> *Existe um estado de alimentação em que uma está viva e a outra não?*

Se sim — e há quase sempre, na bancada, com uma delas no USB — **elas precisam
do laço directo**. Alimentação comum garantida é a única excepção, e "comum" tem
de incluir a bancada, não só a peça montada.

## 6 · Entradas de alta impedância

Um piezo é uma fonte de **impedância altíssima** — e impedância alta é uma
antena: basta pouquíssima corrente induzida para gerar tensão apreciável.

E o que se apanha na entrada é pior do que o que se apanha na saída: **passa
por toda a cadeia de ganho a jusante**. Ruído de entrada sai amplificado pelo
drive, pelo gate, pela reverb, por tudo.

**Três cuidados, por ordem de eficácia:**

**1 · Afastar fisicamente dos cabos do amp classe D.** A saída de um classe D
não é áudio — é uma **onda quadrada a ~400 kHz** com corrente alta, e esses
fios irradiam. Correr em paralelo com cabos de entrada é injecção directa
**pelo ar**, e esquema de terra nenhum a corrige. Se tiverem de se cruzar,
**em ângulo recto**, nunca lado a lado.

**2 · Retorno na ilha de `AGND`**, com o sinal e o seu retorno a **viajarem
juntos** — lado a lado ou torcidos. Área de laço pequena apanha menos campo.

**3 · Cabo blindado com a malha ligada numa ponta só**, no `AGND`, do lado de
quem lê. **Ligada nas duas pontas a malha vira anel** e passa a conduzir
corrente — que é o que se queria evitar.

⚠️ **Um gate a tapar ruído de entrada é um defeito disfarçado.** Um limiar alto
o suficiente para o silenciar também come as caudas longas do instrumento. Se
o `gate_thresh` teve de subir para o som ficar limpo, o problema está na
entrada, não no gate.

---

## 7 · O que se acopla pelo ar

Nem tudo é cobre. Duas fontes valem revisão explícita:

| fonte | o que irradia | contra-medida |
|---|---|---|
| **saída de amp classe D** | quadrada a centenas de kHz, corrente alta | afastar; cruzar em ângulo recto |
| **rádio (BLE/WiFi)** | rajadas de corrente de µs | desacoplamento local — mata a corrente onde nasce |

⚠️ **O rádio tem duas caras, e parecem dois defeitos.** Com o BLE **conectado**,
as rajadas seguem o intervalo de ligação e soam a **zumbido contínuo**;
**desconectado**, seguem o intervalo de anúncio (~100–300 ms) e soam a
**clique a ~3 Hz**. Mesmo mecanismo, dois ritmos.

**O teste que o isolou:** cortar a alimentação da ponte por completo. O ruído
parou.

---

## 8 · Como medir

⚠️ **Medição de terra só significa alguma coisa SOB CARGA.** Em repouso não há
corrente, e sem corrente não há queda de tensão para medir — dá zero em cima
de um defeito.

**O método:**

1. Pôr o sistema a produzir som de verdade (tom de teste serve, e liberta as
   mãos — segurar as pontas e tocar ao mesmo tempo não dá).
2. Multímetro em **AC**, na escala de milivolts.
3. Ponta preta na **estrela**. Ponta vermelha na terra de cada consumidor,
   um de cada vez.
4. **Anotar todos os números antes de mexer em qualquer coisa.**

**O que os números querem dizer:**

| leitura | significa |
|---|---|
| **0 mV** | esse consumidor está de facto na estrela |
| **dezenas de mV** | chega lá através de outra coisa — falta-lhe fio próprio |
| **subiu depois de uma mudança** | a mudança piorou; desfazer |

**Medido neste instrumento, em 2026-08-31, com tom a tocar:**

| percurso | antes | depois |
|---|---|---|
| estrela → amp | 0 mV | 0 mV |
| estrela → Daisy | **42 mV** | **0 mV** (fio dedicado) |
| estrela → **ponte** | — | **0 mV** (medido 2026-09-01) |

⚠️ **E foi a medição que dispensou o terceiro fio.** Eu tinha prescrito «terra
própria à estrela» também para a ponte, por simetria com a Daisy. Medido com
tom a tocar: **0 mV** — ela já lá chega bem. Os 42 mV da Daisy eram um número
concreto e justificavam o fio dela; **prescrever o mesmo à ponte sem medir era
generalizar de um caso**. O fio não se fez, e não faz falta.

> **Simetria não é medição.** Duas placas no mesmo trilho podem ter caminhos de
> retorno muito diferentes, e o multímetro custa menos que o fio.

⚠️ **Um clique audível é um medidor de massa gratuito.** Ouvia-se pelo exciter
um clique a ~3,3 Hz que seguia o LED de diagnóstico da Daisy. Um LED consome
poucos miliamperes: **se isso é audível, o acoplamento é grande** — e tudo o
resto que a placa faz passa pelo mesmo caminho, sem padrão que dê por si.

Silenciá-lo por firmware esconde o defeito. **Depois da massa arrumada ele
desaparece sozinho, e é esse o teste de que ficou bem feita.** Foi o que
aconteceu.

---

## 9 · O quadro das proibições

| 🚫 | porquê |
|---|---|
| Banco grande de baixa ESR na entrada de conversor comutado | impedância de entrada negativa → oscila (guincho) |
| 2200 µF na saída de um step-down | mexe na estabilidade da malha; 100 µF chega |
| Alimentação pelo `AGND` | o retorno atravessa a ponte estreita e aterra no codec |
| `AGND` e `DGND` juntos fora da placa | segundo ponto de encontro = anel pela referência do áudio |
| Referência de sinal levada à estrela | injecta a diferença estrela↔leitor na medição |
| Malha de cabo blindado ligada nas duas pontas | vira anel e passa a conduzir |
| Chave no retorno | resistência de contacto variável no caminho de todos |
| Cabo de entrada em paralelo com saída de classe D | acoplamento pelo ar, imune a terra |
| Concluir de medição feita **sem carga** | zero em cima de um defeito |

---

## 10 · Registo — os três ruídos de 2026-08-31

Estavam todos ao mesmo tempo e pareciam um só. Separá-los foi metade do
trabalho.

| som | causa real | conserto |
|---|---|---|
| **guincho agudo** | 2200 µF na entrada do step-down a oscilar | remover |
| **clique a ~3 Hz** | ponte ESP32 a anunciar; terra da Daisy pelo step-down | fio dedicado à estrela · desacoplamento na ponte |
| **zumbido contínuo** | ponte ESP32 ligada; rajadas do intervalo de ligação | desacoplamento local na ponte |

> **E a lição que ficou:** neste caso acusei o hub USB, os condensadores e o
> circuito — três inocentes — antes de chegar à causa. Cada acusação tinha
> explicação coerente e **nenhuma tinha um teste que a pudesse derrubar**.
>
> O que fechou o caso não foi medição nenhuma: foi **tirar a ponte e ouvir**.
> Uma variável, um gesto.
>
> **Hipótese sem teste que a derrube não é diagnóstico — é narrativa.**
