# Previsão de congestão — o método do Estágio 3.5

Referência do **Estágio 3.5** da pipeline `pcb` (pré-layout, entre o 3
colocação e o 4 vias/cobre). Subordinada à filosofia dos portões da SKILL.md —
**um portão que avisa e deixa passar não é portão**, **medir no sítio certo**,
**resultado negativo é achado**, **prova que a guarda sabe dizer não**.

> **A frase que governa este estágio:** congestão descoberta a rotear é falha de
> **arquitetura** tarde demais. O router não a resolve — só a revela quando já
> custa refazer a colocação. Por isso mede-se a densidade **antes de encostar
> cobre**, sobre a colocação e o netlist, e um número pode dizer *não*.

## Índice

1. [As cinco métricas](#1-as-cinco-métricas) — o que se mede e a fórmula de cada uma
2. [As duas recusas duras](#2-as-duas-recusas-duras) — os `if` que param a fila
3. [O portão GO / REVER-COLOCAÇÃO](#3-o-portão-go--rever-colocação)
4. [As âncoras J14/C16 e J1/C5](#4-as-âncoras-j14c16-e-j1c5)
5. [A ferramenta `scripts/congestao.py`](#5-a-ferramenta-scriptscongestaopy)

---

## 1. As cinco métricas

Todas medidas da **colocação + netlist** (footprints e redes), **sem uma única
pista**. Números do projecto carregados, não re-derivados (ver
`quality-standards.md` §2): pista default 0,25 mm, clearance 0,20 mm →
**passo de canal em ar livre 0,45 mm**; IDC passo 2,54 mm, pad ⌀1,70 → **folga
inter-pad 0,84 mm** (cabe exactamente **1** pista às regras da placa). B.Cu é
plano de terra **sólido** nas duas placas, logo **todo sinal não-terra escapa
pela única camada de sinal F.Cu** — um sinal empurrado para B.Cu **corta o
plano** (é a métrica 5).

### 1.1 Orçamento de canais de escape por conector — a métrica que prevê J14/C16

Para um conector de **≥2 filas** numa placa de **1 camada de sinal**, cada
broadside (lado longo) passa ~`ncol` canais de escape (uma pista por coluna de
pinos, à folga inter-pad de 0,84 mm). Um broadside está **morto** se tem menos
de **3 mm** de placa aberta além do courtyard para onde abrir o leque.

```
PROCURA  S      = nº de redes não-plano distintas que TÊM de sair do footprint
                  (GND/AGND vão ao plano → não contam)
OFERTA          = (broadsides vivos) × ncol        ncol = colunas de pinos
veredicto       = VERMELHO  se  S > OFERTA
                  ÂMBAR     se  S > 0,8 × OFERTA
                  VERDE     caso contrário
```

Encostar um conector de duas filas a um bordo mata um dos dois broadsides e
**parte a oferta ao meio** — é a raiz de J14 e de J1.

### 1.2 Densidade de pinos por célula

Grelha de **5 mm**; conta pads por célula. O pico localiza o campo de pinos que
alimenta a sobre-alocação da métrica 1 (num 2×13, cinco células adjacentes no
máximo da grelha = as 26 almofadas do IDC).

### 1.3 Densidade de cruzamento do ratsnest

Árvore geradora mínima (MST/Prim) por rede de sinal, amostrada em células de
5 mm: o comprimento de airwire que cada célula acumula. Picos = onde muitos
sinais têm de se cruzar antes de existir cobre — o gargalo que o router vai herdar.

### 1.4 Procura vs oferta de cobre por região

Por célula de 5 mm:

```
PROCURA (mm)  = Σ (comprimento de airwire na célula × largura de classe da rede)
                larguras: potência 1,0+clr · rail "+…" 0,5+clr · sinal 0,25+clr
OFERTA  (mm)  = área livre de F.Cu na célula / passo de canal (0,45 mm)
                área livre = 5×5 − área de pads na célula
rácio         = PROCURA / OFERTA     RED >1 · ÂMBAR >0,6 · ok caso contrário
```

### 1.5 Fragmentação de plano na colocação

Comprimento de pista de **sinal em B.Cu** por célula de 5 mm — cada milímetro de
sinal em B.Cu é um corte no plano de terra. Alto = retorno obrigado a contornar
ranhuras (liga-se à revisão de terra do Estágio 5 — `revisao-terra` P5/P6, o
retorno que contorna o fosso). Numa placa sem cobre ainda é zero; a métrica serve
o 3.5 quando já há um esboço de roteamento, e serve o Estágio 5 depois.

---

## 2. As duas recusas duras

Provadas na sessão ppc/002 rev C. Escrevem-se como **recusa**, não como aviso:

- **(a) recusar um conector de ≥2 filas onde PROCURA > OFERTA** de canais de
  escape (métrica 1.1). Foi o que apanhou J14 e J1 a partir só da colocação.
- **(b) recusar um courtyard que sobrepõe outro courtyard, a borda da placa
  (folga negativa), ou o corpo (F.Fab/B.Fab) de outro componente.** Mede-se por
  **corpo**, não por courtyard nominal — a lição C16↔J14 (`review-protocol.md`):
  um courtyard-overlap parecia dispensável, mas corpo-contra-corpo era colisão
  real. *Um waiver é uma medição, não uma opinião.*

`scripts/congestao.py` faz o exit ≠0 disparar em: **(a)** qualquer conector
vermelho de escape; **(b)** courtyard além da borda, ou courtyard sobre o corpo
de outro componente do mesmo lado. (Courtyard∩courtyard puro é o
`courtyards_overlap` que o DRC do Estágio 7 já possui; aqui a recusa dura pré-
layout foca-se no corpo, que é o que o DRC de courtyard nominal **não** vê.)

---

## 3. O portão GO / REVER-COLOCAÇÃO

Duas saídas, nunca "avança com aviso":

- **GO** — nenhuma região vermelha. Segue para o Estágio 4 (vias+pistas).
- **REVER-COLOCAÇÃO** — há vermelho. A região volta ao **Estágio 3** (recolocar),
  **ou ao 1/2** se o que estrangula é o **pinout do próprio conector** (a fila ou
  a ordem de pinos que nenhuma recolocação salva — é decisão de esquema). **Nunca
  se avança para o 4 com vermelho em aberto** — rotear por cima de uma colocação
  vermelha é gastar tempo a confirmar o inevitável.

**Artefacto:** o **mapa de congestão** vai para o **registo de risco** (uma
região por linha, com procura/oferta medidas), ligado ao **Portão de
Certificação de Risco** (saída do Estágio 7, ver `review-protocol.md`). O 3.5 é a
forma **pré-layout** do **IFÁ10000** — o protocolo de certificação canónico do
projeto: certifica a colocação contra **medida**, não contra opinião.

---

## 4. As âncoras J14/C16 e J1/C5

**J14/C16 (cérebro) — congestão apanhada tarde demais.** J14 é um IDC 2×13
board-to-board encostado ao bordo de cima: broadside de cima **morto**
(gap 2,80 mm < 3 mm), 17 sinais para escapar por 13 colunas de um só lado →
**procura 17 > oferta 13, VERMELHO**. O corredor interior (o único vivo) já
estava tomado pelo front-end do pré-amp (D11/R11/J5), por isso o cap de
desacoplamento **C16 (100n +3V3A) não teve casa junto ao seu rail e foi parar
DENTRO do corpo de J14** — o MUST-FIX C16↔J14 do registo de risco. O nudge de
J14 para dentro estava **fisicamente bloqueado pelo pré-amp**; o cérebro
resolveu levando **C16 para B.Cu + FreeRouting**, não relaxando o orçamento de
escape. É por isso que `congestao.py` ainda marca J14 vermelho na rev C fechada:
o over-commit é real; foi contornado por roteamento, não desfeito na colocação.

**J1/C5 (painel) — latente, resolvido pelo nudge.** J1 é o **mesmo** IDC 2×13,
mesma classe de falha, e o seu cap +3V3A **C5** estava a ~0,5 mm do corpo — um
nudge de ser o próximo C16. Aqui a recolocação era possível: o **nudge real de
5,5 mm** de J1 para dentro abriu o broadside de cima (gap passou a 3,30 mm ≥ 3),
oferta 13→26, **17 < 26 VERDE**, e C5 ganhou casa. `congestao.py` confirma:
painel passa o portão 3.5 na rev C. O mesmo defeito, os dois desfechos — um
contornado por cobre porque a colocação estava presa, o outro desfeito na
colocação porque havia espaço.

---

## 5. A ferramenta `scripts/congestao.py`

Read-only (carrega a placa, mede, **nunca grava**). Um *starter*: implementa as
métricas 1.1 e 1.2 e as duas recusas duras; as métricas 1.3–1.5 estão descritas
aqui e nos scripts de origem para quem quiser endurecê-las.

```
congestao.py <placa.kicad_pcb>   mede a colocação; sai !=0 em VERMELHO
congestao.py --sabota            prova que sabe dizer NÃO (sem placa, sem KiCad)
```

O motor numérico opera sobre um modelo puro (dicts), por isso o `--sabota` corre
sem placa: planta um conector sobre-alocado, um courtyard fora da borda e um
courtyard dentro do corpo de outro, e exige que **os três** bloqueiem — a guarda
tem dentes (convenção do projeto: `funcao_dos_pads.py --sabota`,
`manifesto_fontes.py --sabota`). Limitação declarada: mede courtyard/corpo por
**bounding box**, não por polígono — apanha o corpo-em-corpo grosso (C16↔J14);
refinar para polígono é trabalho de endurecimento.
