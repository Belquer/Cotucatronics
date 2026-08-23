# MAPA DE PINOS — Cotucatronics

> Verificado em `src/hal_daisy_seed.cpp` e `src/manifest.h` @ `6002a70`.
> **Pino se confirma no fonte, não na memória.** Se este documento e o
> código divergirem, o código está certo e este documento está velho.

## Ocupados

| pino | função | como |
|---|---|---|
| **D13** | UART TX → **RX** do WIDI | PB6 · USART1 · 31250 8N1 · ★ CRUZADO |
| **D14** | UART RX ← **TX** do WIDI | PB7 · ★ CRUZADO · é o que recebe MIDI |
| **D15–D19** | pots (A0–A4) | ⚠️ **A0–A4 são ALIASES de D15–D19** — mesmo pino físico |
| **D22** | botão B4 — freeze B | INPUT + PULLUP, ativo-baixo |
| **D23** | LED — freeze B | OUTPUT, segue o param `frz_on_b` |
| **D24** | botão B2 — freeze A | INPUT + PULLUP, ativo-baixo |
| **D25** | botão B3 — rearme do CTOS | INPUT + PULLUP, sem alvo de param |
| **D26** | botão B1 — búzios (randomizador) | INPUT + PULLUP |
| **D27** | LED — freeze A | OUTPUT, segue o param `frz_on_a` |
| **D28** | LB do WIDI (status do link) | INPUT + PULLUP |

## Livres

**D0–D12** · **D20** · **D21** · **D29** · **D30**

D29/D30 estão **reservados** para o sense dos DPDT dos exciters (`exc_on`
lendo estado real do hardware, não intenção de software).

## Reservado para o painel v2

| bloco | pinos | nota |
|---|---|---|
| OLED SSD1309 SPI | 5 (SCK, MOSI, CS, DC, RES) | faixa D7–D10 + 2 GPIO — **confirmar o periférico SPI na libDaisy** |
| Encoder EC11 | 3 (A, B, push) | push = confirmar |
| Pots de volume ×2 | 2 (ADC) | um por exciter |

## A regra de hardware que custou uma gravação

> **GPIO se remapeia no fonte (dois números, uma gravação).
> UART, SAI, ADC e QSPI NÃO — a função está amarrada ao pino pelo silício.**

A diferença **não aparece no esquema**, só na tabela de funções alternadas do
chip. LED/switch invertidos custaram uma gravação; o MIDI de volta exigiu fio
novo.

## Tabela de botões (fonte única)

`AddBotao()` em `manifest.h` amarra índice do HAL → param → LED:

| índice | nome | switch | LED | alvo |
|---|---|---|---|---|
| B1 | búzios | D26 | — | randomizador |
| B2 | freeze A | D24 | D27 (LED 0) | `frz_on_a` |
| B3 | rearme CTOS | D25 | — | age no CTOS direto |
| B4 | freeze B | D22 | D23 (LED 1) | `frz_on_b` |

**Debounce: 30 ms, MEDIDO** — com 10 ms, cinco batidas viravam seis.

**O LED segue o PARAM, nunca o gesto.** Se o freeze mudar por CC, serial ou
CTOS, o LED conta a verdade. Verificado por serial, sem tocar no botão.
