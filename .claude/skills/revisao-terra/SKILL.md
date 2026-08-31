---
name: revisao-terra
description: Revisão de aterramento, massa e desacoplamento no desenho de placa — classifica cada condutor de terra, confere estrela contra margarida, AGND/DGND, condensadores por tipo de carga, acoplamento pelo ar e entradas de alta impedância. Use ao desenhar ou rever hardware, ao montar um circuito novo, e sempre que houver zumbido, guincho, clique ou ruído que não se explica no firmware.
---

# Revisão de terra — a varredura

Formulada por **quantificação fechada**: cada condutor de terra tem um conjunto
finito de estados legais, e **tudo que não está num estado legal é achado, por
definição**. Um defeito de massa não escapa por não ser reconhecido — escapa
por haver um lugar onde a regra não olhou.

Ruído de massa é caro porque **imita tudo o resto**: parece ruído de ADC, parece
defeito de firmware, parece componente queimado. Uma varredura sistemática
custa uma hora; o diagnóstico por hipótese custou uma tarde e três inocentes.

---

## Regras de ferro DO PRÓPRIO REVISOR

Estas vêm antes das passadas, porque o revisor erra primeiro.

**R1 — Medição sem carga não vale.** Em repouso não há corrente, e sem corrente
não há queda de tensão. **Dá zero em cima de um defeito.** Toda medição de
terra declara em que condição de carga foi feita, ou não é medição.

**R2 — Uma variável de cada vez.** Mudar duas coisas e ouvir melhoria não diz
qual delas agiu — e uma delas pode estar a piorar. Isto inclui não fazer
reset/ciclo de energia entre testes sem o declarar: a recuperação pode vir do
reset e ser atribuída à mudança.

**R3 — Anotar todos os números ANTES de mexer.** Sem o "antes" não há como
provar que o "depois" melhorou. Metade das discussões de massa morrem aqui.

**R4 — Hipótese sem teste que a derrube é narrativa, não diagnóstico.** Para
cada suspeita, escrever **qual observação a mataria**. Se não houver nenhuma, a
suspeita não entra no relatório.

**R5 — Ler o próprio código antes de acusar o cobre.** Guarda mal posta, filtro
com constante errada, histerese medida do centro em vez da fronteira — tudo
isto **imita ruído com perfeição**. Antes de afirmar "o ADC é ruidoso",
**medir o ruído** e comparar com o limiar do software.

**R6 — Resultado negativo é achado.** O que passou fica escrito, ou a próxima
revisão refaz o trabalho.

**R7 — Cada achado sai com severidade e com dono.** 🔴 destrói peça ou perde
sinal · 🟠 audível/visível · 🟡 latente · ⚪ higiene. E: conserta-se com fio,
com componente, ou com desenho?

**R8 — Declarar o que não foi verificado** e porquê. Nunca omitir.

---

## As passadas

Cada passada responde UMA pergunta. Nenhuma é opcional; passada saltada é
**declarada**, não silenciada.

### P0 — Inventário e classificação

**Listar TODOS os condutores de terra do desenho.** Cada um é exactamente um
destes três:

| tipo | o que é | corrente | destino legal |
|---|---|---|---|
| **força** | volta da corrente consumida | alta, pulsada | fio próprio à estrela |
| **referência lógica** | zero do ADC e dos pinos digitais | ~nenhuma | fica no aparelho que lê |
| **referência de áudio** | zero que viaja com o sinal | ~nenhuma | ilha de `AGND` |

**Condutor não classificável = achado.** Condutor num destino que não é o legal
do seu tipo = achado.

⚠️ **O erro mais comum desta passada** é levar referência à estrela. "Estrela é
bom, então levo tudo à estrela" — mas referência levada à estrela **injecta**
a diferença de tensão estrela↔leitor na medição. Estrela é para **corrente**.

Pergunta que decide: *este condutor transporta corrente, ou define um zero?*

### P1 — Estrela contra margarida

- **Onde está o ponto de estrela?** Se não se souber apontar um ponto físico,
  não há estrela — achado 🔴.
- Para **cada** consumidor de força: tem fio próprio até lá, ou chega através
  de outro? Chegar através de outro = achado.
- **O retorno do maior consumidor é o fio mais curto e grosso?** Se não, achado.
- **Há chave, relé ou conector no caminho de retorno?** Achado — resistência de
  contacto variável no caminho de todos.

### P2 — Ilhas de referência são folhas

Toda referência (lógica ou de áudio) deve **entrar num aparelho e não sair para
mais lado nenhum**.

- Traçar cada ilha: onde termina? Se toca dois aparelhos, é achado.
- **Conferência física:** com a placa fora do soquete, continuidade entre a ilha
  e o trilho de força. **Se apitar, há ligação a mais.**
- O **positivo de referência** (3V3 dos pots, etc.) sai da mesma placa que faz a
  medição? Alimentar de uma fonte e referenciar noutra = achado.

### P3 — `AGND` e `DGND`

Placas com codec juntam os dois **internamente, num ponto só**, de propósito.

- **Estão juntos em algum sítio fora da placa?** Achado 🔴 — dois pontos de
  encontro formam anel, e a corrente divide-se pelos dois caminhos; a fracção
  que escolhe o de dentro passa pela referência do codec.
- **A alimentação entra pelo `DGND`?** Entrar pelo `AGND` é achado 🔴 — o
  retorno de toda a placa atravessa a ponte estreita.
- **O retorno do áudio (jacks, entrada do amp, entradas analógicas) vem do
  `AGND`?** Vir do trilho geral é achado 🟠.

### P4 — Desacoplamento por TIPO DE CARGA

A pergunta *"precisa de condensador?"* **não tem resposta única para o
sistema**. Classificar cada consumidor:

| carga | precisa | erro comum |
|---|---|---|
| corrente pulsada audível (amp classe D) | banco grande **+ 100 nF** nos pinos | banco em falta → zumbido cruzado |
| **potência constante** (conversor comutado) | **nada de grande na entrada** | banco → **guincho** |
| rajadas de rádio (BLE/WiFi) | **100 µF + 100 nF** locais | nada → o rádio sai pelo altifalante |
| lógica (MCU) | 100 µF + 100 nF locais | nada → ruído difuso |

- **Todo consumidor tem o PAR?** Só o grande deixa passar as arestas; só o
  pequeno não tem energia. Um sozinho = achado.
- **Está colado nos pinos?** A mais de poucos centímetros, liga-se através da
  indutância que existe para contornar — achado 🟠.
- **Tensão ≥1,5–2× o trilho?** Polaridade certa?
- 🔴 **Banco grande de baixa ESR na entrada de conversor comutado** =
  impedância de entrada negativa contra indutância da fiação → oscila
  (Middlebrook). Achado que **destrói o som na hora**.
- ⚠️ **Na saída de um step-down, 100 µF — não 2200.** Capacitância a mais
  também mexe na estabilidade da malha.
- **Inrush:** banco grande pode fazer uma fonte PD ler curto e re-negociar. Se
  o arranque for intermitente, é isto — declarar, não caçar defeito novo.

### P5 — Entradas de alta impedância

Fonte de impedância alta (piezo, captador, cristal) **é uma antena**, e o que
ela apanha passa por **toda** a cadeia de ganho a jusante.

- Retorno na ilha de `AGND`?
- Sinal e retorno **viajam juntos** (lado a lado ou torcidos)? Caminhos
  separados = área de laço = achado.
- Cabo blindado: **malha ligada numa ponta só?** Nas duas pontas ela vira anel
  e conduz — achado.
- ⚠️ **Há um gate/expansor a tapar ruído de entrada?** Achado disfarçado: o
  limiar que silencia o ruído também come as caudas do instrumento. Se o
  limiar teve de subir, o defeito está na entrada.

### P6 — Acoplamento pelo ar

Nem tudo é cobre, e o que se acopla pelo ar **é imune a esquema de terra**.

- **Saída de amp classe D** = onda quadrada a centenas de kHz com corrente
  alta. Cabos de entrada em paralelo com ela = achado 🟠. Cruzamentos **em
  ângulo recto**.
- **Rádio** (BLE/WiFi): tem **duas caras**, e parecem dois defeitos —
  **conectado**, rajadas no intervalo de ligação = **zumbido contínuo**;
  **desconectado**, rajadas de anúncio (~100–300 ms) = **clique a ~3 Hz**.
  Mesmo mecanismo, dois ritmos. Não os tratar como defeitos separados.
- Percurso de cabos declarado no desenho, ou "logo se vê na montagem"? O
  segundo é achado ⚪ que vira 🟠 na montagem seguinte.

### P7 — Plano de medição

A revisão termina com **como se prova**, não com opinião.

- Ponto de estrela como referência, multímetro em **AC/mV**, **sob carga**.
- Um número por consumidor, **anotado antes de qualquer mudança** (R3).
- Critério de aceitação **escrito antes do conserto**: que número, em que ponto,
  significa resolvido.

| leitura | significa |
|---|---|
| 0 mV | está de facto na estrela |
| dezenas de mV | chega lá através de outra coisa — falta fio próprio |
| subiu após a mudança | a mudança piorou; desfazer |

⚠️ **Procurar um medidor de massa gratuito.** Um clique periódico audível que
segue um LED, um relé, um refrescamento de ecrã — isso é acoplamento a
denunciar-se sozinho. Um LED consome miliamperes: **se é audível, o
acoplamento é grande**, e tudo o resto que a placa faz passa pelo mesmo
caminho sem padrão que dê por si.

**Silenciá-lo por firmware esconde o defeito.** Depois da massa arrumada ele
desaparece sozinho — e é esse o teste de que ficou bem feita.

---

## Formato do relatório

1. **O que foi revisto** — desenho, revisão, data. Passadas cobertas.
2. **O QUE PASSOU**, explícito, por passada (R6).
3. **Achados por severidade**, cada um com: onde · regra violada **pelo nome** ·
   consequência audível/medível · conserto mínimo · fio, componente ou desenho.
4. **Ordem de ataque** — uma variável de cada vez (R2), com o critério de
   aceitação de cada passo escrito **antes**.
5. **O que NÃO foi verificado** e porquê (R8).

---

## Anti-decadência

Família de defeito nova descoberta numa revisão **entra como passada ou item
neste arquivo** — proposta, nunca em silêncio. A revisão que não se actualiza
revê o projecto de ontem.
