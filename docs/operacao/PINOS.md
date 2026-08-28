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
| OLED SSD1309 SPI | 5 | **SOLDADO 23/08** — ver a tabela abaixo. SPI1, modo 0, MISO não atribuído |
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

## OLED SSD1309 — soldado e ocupado (23/08, v0.154.0)

| sinal | pino | porta | origem |
|---|---|---|---|
| CLK | **D8** | `PORTG11` | default da libDaisy |
| DIN | **D10** | `PORTB5` | default da libDaisy |
| CS | **D7** | `PORTG10` | default da libDaisy |
| DC | **D6** | `PORTC12` | **sobrescrito** (default seria D9) |
| RST | **D5** | `PORTD2` | **sobrescrito** (default seria D30) |

**D5, D6, D7, D8 e D10 saíram do pool de livres.** O **D9** continua livre —
é o que o default usaria para DC e a solda não usou.

**O RST fora do default é escolha certa e vale registrar por quê:** o default
é `PORTB15` = **D30**, e D29/D30 estão reservados para a leitura dos DPDT do
exciter. Usar o default aqui criaria uma disputa que só apareceria no dia de
soldar o painel.

**MISO não é atribuído, e não custou decisão.** O default da libDaisy já é
`direction = TWO_LINES_TX_ONLY` com `miso = Pin(PORTX, 0)` — o "nenhum pino"
da casa. O display é escrita pura.

> ⚠️ **Consequência que atravessa todo diagnóstico deste painel: ele NÃO É
> DETECTÁVEL.** Não há caminho de leitura, então nenhum comando pode dizer
> "OLED presente". O `board` diz **configurado**, e a diferença não é
> preciosismo — é a mesma linha do `placa viva: sim` que já mentiu aqui.

## MAPA CONFERIDO POR MÁQUINA

<!-- pinos: inicio -->
| pino | dono |
|---|---|
| D0 | encoder A fase A |
| D1 | encoder A fase B |
| D2 | encoder A push |
| D3 | encoder B fase A |
| D4 | encoder B fase B |
| D5 | OLED RST |
| D6 | OLED DC |
| D7 | OLED CS |
| D8 | OLED CLK |
| D9 | encoder B push |
| D10 | OLED DIN |
| D13 | MIDI TX |
| D14 | MIDI RX |
| D17 | encoder G push |
| D18 | encoder G fase **B** |
| D19 | encoder G fase **A** |
| D22 | botão B4 freeze B |
| D23 | LED freeze B |
| D24 | botão B2 freeze A |
| D25 | botão B3 rearme CTOS |
| D26 | botão B1 randomize |
| D27 | LED freeze A |
| D28 | WIDI LED |
| D29 | RESERVADO — DPDT exciter |
| D30 | RESERVADO — DPDT exciter |
<!-- pinos: fim -->

**Livres:** D11, D12 (I2C_1 para o INA219), D15, D16, D20, D21.

> **Esta tabela é conferida pelo `make check-docs`** contra os
> `seed::Dnn` do `hal_daisy_seed.cpp`. Pino reclamado no código e ausente
> daqui, ou daqui e não usado lá, vira acusação.
>
> A rotina existe porque **a placa está entupida e os pontos de solda
> espremidos**: conferir de cabeça é o que falha exatamente quando o
> custo de errar é maior. E porque um mapa suposto já custou horas — o
> encoder global estava em D17/D18/D19 e o firmware media D20/D21,
> então todo instrumento apontava para o lugar errado e nenhum podia
> dizer isso.
