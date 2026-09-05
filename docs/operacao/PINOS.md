# MAPA DE PINOS — Cotucatronics

> Verificado em `src/hal_daisy_seed.cpp` e `src/manifest.h` @ `hal v0.74.1` (04/09).
> **Pino se confirma no fonte, não na memória.** Se este documento e o
> código divergirem, o código está certo e este documento está velho.

## Ocupados

| pino | função | como |
|---|---|---|
| **D13** | UART TX → **RX** da ponte ESP32 | PB6 · USART1 · 31250 8N1 · ★ CRUZADO |
| **D14** | UART RX ← **TX** da ponte ESP32 | PB7 · ★ CRUZADO · é o que recebe MIDI · **PULL-UP INTERNO LIGADO** (~40 kΩ) desde 02/09 — ver abaixo |
| **D15, D16** | pots ×2 (A0, A1) | ⚠️ **A0–A4 são ALIASES de D15–D19** — mesmo pino físico. **Só D15 e D16 são pots**; `board` diz `knobs:2`. Ambos com leitura **invertida** (`kKnobInverte`) — medido nos dois extremos, cru 0,0008 no máximo e 0,9753 no mínimo |
| **D0, D1, D2** | **o encoder** — fase A, fase B, push | `kEncPin[0]`, e é o único desde 05/09 |
| **D17** | **A2 — divisor 100 k/22 k do trilho de 15 V** | leitura da tensão; o firmware recusa ligar o amp abaixo de 12 V |
| **D5, D6, D7, D8, D10** | OLED SSD1309 SPI | ver a tabela do painel, mais abaixo |
| **D22** | botão B4 — freeze B | INPUT + PULLUP, ativo-baixo |
| **D23** | LED — freeze B | OUTPUT, segue o param `frz_on_b` |
| **D24** | botão B2 — freeze A | INPUT + PULLUP, ativo-baixo |
| ~~**D25**~~ | ~~botão B3 — rearme do CTOS~~ · **REMOVIDO 05/09** | a função fica pelo `b3` da serial; o painel não leva botão |
| **D26** | botão B1 — búzios (randomizador) | INPUT + PULLUP |
| **D27** | LED — freeze A | OUTPUT, segue o param `frz_on_a` |
| **D28** | STATUS da ponte ESP32 (link BLE) | INPUT + PULLUP · ativo em BAIXO, aberto-dreno |

> ✅ **O CONFLITO DO `D17` MORREU EM 05/09, e não por conserto.**
>
> O `D17` era o push do encoder G *e* o A2 do divisor do trilho — dois usos
> escritos, ambos intencionais, e nenhum verificador os via porque moravam em
> documentos diferentes. Propus duas saídas, e **nenhuma foi precisa**: o dB
> fechou a interface num **encoder só**, o encoder G saiu, e o `D17` ficou
> livre para o divisor sem se tocar em firmware que funcionava.
>
> **Uma decisão de interface dissolveu um problema de hardware.** Fica
> escrito porque a próxima colisão pode ter a mesma saída, e ninguém a
> procura desse lado.

## Livres

**D3** · **D4** · **D9** · **D11** · **D12** · **D18** · **D19** · **D20** · **D21** · **D25** — dez.

Passaram de quatro a dez a 05/09: saíram os encoders B e G (`D3 D4 D9 D18 D19`)
e o botão de rearme (`D25`). O `D17` **não** está aqui — é o A2 do divisor.

⚠️ **`D29` e `D30` saíram desta lista em 04/09: são as linhas de dados do
USB** (pinos 36 e 37, `USB_HS_D−`/`D+`). Ver o aviso abaixo.

> ⚠️ Esta linha dizia **`D0–D12`** até 04/09, e estava errada de duas
> maneiras ao mesmo tempo: D0–D4 e D9 são os encoders A e B, e D5–D8 e D10
> são o OLED — que este mesmo documento lista como ocupado três secções
> abaixo. **O documento contradizia-se a si próprio**, e ninguém reparou
> porque as duas afirmações nunca são lidas na mesma passagem.
>
> Apanhado pelo exame de pinos do `bts`, que é para isto que existe: ler o
> fonte e o documento lado a lado em vez de confiar em qualquer um deles.

⚠️ **Não confundir com `kLivresPin` no fonte** (`hal_daisy_seed.cpp`). Essa
lista chama-se "livres" mas quer dizer **"que a varredura pode reconfigurar
sem matar o OLED nem o MIDI"** — e inclui D17/D18/D19, que são o encoder G.
O nome mente; o comportamento é intencional (a varredura existe justamente
para descobrir cablagem de encoder).

> 🔴 **D29 E D30 NAO PODEM SER USADOS. SAO O USB.**
>
> Descoberto em 04/09, ao gerar o símbolo da Seed do
> `Daisy_Seed_Rev4_Pinout.csv` do fabricante:
>
> ```
> pino 36  →  D29  =  USB_HS_D_-   (PB14)
> pino 37  →  D30  =  USB_HS_D_+   (PB15)
> pino  1  →  D0   =  USB_HS_ID    (PB12)
> ```
>
> São **as linhas de dados do próprio USB** — as que o conector usa e por onde
> passa a serial. Reconfigurá-las como GPIO mata a porta, e o defeito só
> apareceria com a placa feita: firmware a correr, painel bom, e nenhuma
> forma de falar com ele. Passámos um dia inteiro nesse quadro por outra
> causa, e sabemos como é.
>
> **Este documento reservava-as para o sense dos DPDT.** Não vão lá.

**O sense dos DPDT foi DESCARTADO a 05/09.** A DPDT tipo 2 usa os dois polos
para o mesmo canal (comuns amarrados), portanto não sobra contacto para ler o
estado. `exc_on` continua a ser **intenção de software** — o firmware acredita
no que lhe dizem. As opções e o que cada uma custa estão em
[`FUTURO.md`](../FUTURO.md) §1, incluindo uma que não gasta pino nenhum.

⚠️ **`D0` é o `USB_HS_ID`** e está a ser usado como fase A do encoder A. Em
modo dispositivo o pino ID não é usado, portanto funciona — mas fica escrito,
porque é a mesma família e ninguém o sabia.

## Reservado para o painel v2

| bloco | pinos | nota |
|---|---|---|
| OLED SSD1309 SPI | 5 | **SOLDADO 23/08** — ver a tabela abaixo. SPI1, modo 0, MISO não atribuído |
| Encoder EC11 | 3 (A, B, push) | ✅ **JÁ SÃO TRÊS E ESTÃO LIGADOS** — ver os `kEncPin` na tabela de ocupados |
| Pots de volume ×2 | 2 (ADC) | ✅ **LIGADOS** em D15/D16, um por exciter |

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

⚠️ **O PAINEL ACESO CUSTA 6 dB DE CHÃO DE RUÍDO NA ENTRADA** — medido em
31/08: IN_L de −75,3 para −69,3 dBFS, com um espeto a **7040 Hz** e dominância
0,95. É a corrente de accionamento das linhas (64 linhas × ~110 Hz de quadro),
proporcional aos píxeis **acesos**.

Por isso, desde a v0.221.0, **o painel nasce apagado**: acorda quando alguém
mexe num encoder, acende 200 ms depois de a mão parar, e dorme aos 5 s. Ver
[`HARDWARE.md`](HARDWARE.md) §1b.

**Item de bancada:** estes cinco fios correm perto dos cabos dos piezos.
Afastá-los, e cruzá-los em ângulo recto — acoplamento pelo ar não se corrige
com esquema de terra.

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
| D15 | **pot L** (A0) — volume do ramo L |
| D16 | **pot R** (A1) — volume do ramo R |
| D17 | encoder G push |
| D18 | encoder G fase **B** |
| D19 | encoder G fase **A** |
| D22 | botão B4 freeze B |
| D23 | LED freeze B |
| D24 | botão B2 freeze A |
| D25 | botão B3 rearme CTOS |
| D26 | botão B1 randomize |
| D27 | LED freeze A |
| D28 | STATUS da ponte ESP32 (ativo em BAIXO, aberto-dreno) |
| D29 | RESERVADO — DPDT exciter |
| D30 | RESERVADO — DPDT exciter |
<!-- pinos: fim -->

**Livres:** D11, D12 (I2C_1 para o INA219), D20, D21.

⚠️ **O bloco do ADC acabou.** `A0–A4` são aliases de `D15–D19`: os pots
ocupam A0/A1 e o encoder global ocupa D17/D18/D19. **Não há um terceiro pot
possível** sem mudar o painel — e é por isso que `num_knobs` é 2 e não 5.

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

---

## D14 tem pull-up interno, e não é detalhe

A libDaisy inicia os pinos da UART com **`GPIO_NOPULL`**
(`src/per/uart.cpp`). Com a ponte ESP32 sem alimentação o pino dela fica em
alta impedância e o nosso RX **fica a flutuar** — e uma entrada CMOS solta
apanha ruído a MHz. A USART entra em tempestade **por taxa** (FE/NE/ORE ao
ritmo do serviço da interrupção), o CPU fica na ISR e o laço morre à fome.

Medido em 01/09: a placa deixava de responder à serial em **menos de 4 s** e
**não voltava** com a linha já em repouso.

Repouso de uma linha de UART é **alto, por definição**. Se o transmissor
desaparece, alguém tem de segurar o alto. O `hal_daisy_seed.cpp` reconfigura
PB7 com `GPIO_PULLUP` logo a seguir ao `Init` da UART — com ~100 pF de cabo a
subida é ~4 µs contra 32 µs por bit.

**Efeito lateral conhecido e aceite:** com a ESP32 desligada, o pull-up injecta
~80 µA pelo díodo de protecção do pino dela. Inofensivo a esse nível; com
10 kΩ externo seriam ~330 µA.

**Se alguém reescrever o `Init` do MIDI, esta linha tem de sobreviver.** Sem
ela o defeito volta, e volta de uma forma que parece firmware.

Ver battle_scars **15az**.
