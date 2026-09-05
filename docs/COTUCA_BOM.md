# COTUCA_BOM — o instrumento inteiro

### gerado por `tools/bom_completa.py` v0.1.0 · **2026-09-05** · commit `0d6bd37`  ⚠️ **ARVORE SUJA** — ha alteracoes por commitar


> **Esta lista e GERADA das folhas KiCad.** Referencias, valores, footprints e
> marcas de DNP saem do esquema; nao se editam aqui. O que o esquema nao sabe —
> MPN, preco, fornecedor, e o que vive FORA das placas — esta na tabela do
> gerador, e **cada preco declara a proveniencia**:
>
> - **medido** — fui a fonte na data do carimbo e li o numero.
> - **estimado** — ordem de grandeza. **Nao serve para fechar pedido.**
>
> ⚠️ **Stock da LCSC nao e stock de montagem da JLC.** Armazens diferentes. O
> tier Basic/Extended confirma-se na biblioteca de pecas da JLC.


## 1 · CEREBRO — SMD (JLC monta)

| ref | valor | peca / MPN | fornecedor | footprint | US$ | fonte |
|---|---|---|---|---|---|---|
| `C2` | 100n | condensador 0805 — ver o valor p/ dielectrico | — | `C_0805_2012Metric` | 0.0050 | estimado |
| `C4` | 100n | condensador 0805 — ver o valor p/ dielectrico | — | `C_0805_2012Metric` | 0.0050 | estimado |
| `C7` | 100n | condensador 0805 — ver o valor p/ dielectrico | — | `C_0805_2012Metric` | 0.0050 | estimado |
| `C8` | 10u | condensador 1206 — ver o valor p/ dielectrico | — | `C_1206_3216Metric` | 0.0200 | estimado |
| `C9` | 100n | condensador 0805 — ver o valor p/ dielectrico | — | `C_0805_2012Metric` | 0.0050 | estimado |
| `C11` **(DNP)** | 22p C0G | condensador 0805 — ver o valor p/ dielectrico | — | `C_0805_2012Metric` | — | estimado |
| `C12` | 22n C0G | C1206C223J5GACTU (Kemet C0G) | LCSC | `C_1206_3216Metric` | 0.0165 | medido |
| `C21` **(DNP)** | 22p C0G | condensador 0805 — ver o valor p/ dielectrico | — | `C_0805_2012Metric` | — | estimado |
| `C22` | 22n C0G | C1206C223J5GACTU (Kemet C0G) | LCSC | `C_1206_3216Metric` | 0.0165 | medido |
| `D1` | SMBJ24A | SMBJ24A (Littelfuse) | LCSC | `D_SMB` | 0.0790 | medido |
| `D2` | 1N5819 | SS14 (SMA) | LCSC | `D_SMA` | 0.0200 | estimado |
| `D11` | BAV99 | BAV99,215 (Nexperia) | LCSC | `SOT-23` | 0.0200 | estimado |
| `D21` | BAV99 | BAV99,215 (Nexperia) | LCSC | `SOT-23` | 0.0200 | estimado |
| `F1` | PTC 3A | 2920L330/24MR (Littelfuse) | LCSC C720183 | `Fuse_2920_7451Metric` | 0.1804 | medido |
| `R5` | 100k | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R6` | 100k | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R7` | 100R 1/4W | resistor 1206 1/4 W | — | `R_1206_3216Metric` | 0.0100 | estimado |
| `R8` | 100k | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R9` | 22k | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R10` | 0R | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R11` | 4M7 | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R12` | 1k | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R13` **(DNP)** | 4k7 | resistor 0805 1% | — | `R_0805_2012Metric` | — | estimado |
| `R14` | 100R | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R20` | 0R | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R21` | 4M7 | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R22` | 1k | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R23` **(DNP)** | 4k7 | resistor 0805 1% | — | `R_0805_2012Metric` | — | estimado |
| `R24` | 100R | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R50` | 10k — L | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R51` | 10k — R | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `U1` | OPA1642 | OPA1642AIDR (TI) | LCSC C67640 | `SOIC-8_3.9x4.9mm_P1.27mm` | 1.0145 | medido |

## 2 · CEREBRO — THT (o dB solda)

| ref | valor | peca / MPN | fornecedor | footprint | US$ | fonte |
|---|---|---|---|---|---|---|
| `C1` | 2200u/35V | EEU-FR1V222 (Panasonic FR) | LCSC C407956 | `CP_Radial_D16.0mm_P7.50mm` | 0.7162 | medido |
| `C3` | 100u | EEU-FR1C101 (Panasonic FR) | LCSC | `CP_Radial_D6.3mm_P2.50mm` | 0.1500 | estimado |
| `C5` | 47u | EEU-FR1E470 (Panasonic FR) | LCSC | `CP_Radial_D6.3mm_P2.50mm` | 0.1500 | estimado |
| `C6` | 470u 25V | EEU-FR1E471 (Panasonic FR) | LCSC | `CP_Radial_D10.0mm_P5.00mm` | 0.2000 | estimado |
| `C13` **(DNP)** | 22u NP | UES1H220MEM (Nichicon UES) | LCSC | `CP_Radial_D6.3mm_P2.50mm` | — | estimado |
| `C14` | 2u2 filme | MKS2C042201K00KSSD (WIMA) | TME/Mouser | `C_Rect_L7.2mm_W11.0mm_P5.00mm_FKS2_FKP2_MKS2_MKP2` | 0.7000 | estimado |
| `C23` **(DNP)** | 22u NP | UES1H220MEM (Nichicon UES) | LCSC | `CP_Radial_D6.3mm_P2.50mm` | — | estimado |
| `C24` | 2u2 filme | MKS2C042201K00KSSD (WIMA) | TME/Mouser | `C_Rect_L7.2mm_W11.0mm_P5.00mm_FKS2_FKP2_MKS2_MKP2` | 0.7000 | estimado |
| `J1` | entrada decoy PD | terminal parafuso 2p 5,08 (MKDS ou KF301) | — | `TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal` | 0.6000 | estimado |
| `J2` | ao amp TPA3116 | terminal parafuso 2p 5,08 (MKDS ou KF301) | — | `TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal` | 0.6000 | estimado |
| `J3` | step-down 5V fixo | header 1x3 2,54 | — | `PinHeader_1x03_P2.54mm_Vertical` | 0.0500 | estimado |
| `J4` | a ponte ESP32 | header 1x2 2,54 | — | `PinHeader_1x02_P2.54mm_Vertical` | 0.0400 | estimado |
| `J5` | fio -> jack piezo J5 | header 1x3 2,54 | — | `PinHeader_1x03_P2.54mm_Vertical` | 0.0500 | estimado |
| `J6` | fio -> jack piezo J6 | header 1x3 2,54 | — | `PinHeader_1x03_P2.54mm_Vertical` | 0.0500 | estimado |
| `J10` | ponte ESP32-C3 | header 1x5 2,54 | — | `PinHeader_1x05_P2.54mm_Vertical` | 0.0700 | estimado |
| `J11` | modulo TPA3116 | header 1x4 2,54 | — | `PinHeader_1x04_P2.54mm_Vertical` | 0.0600 | estimado |
| `J12` | fio -> TRS linha L | header 1x3 2,54 | — | `PinHeader_1x03_P2.54mm_Vertical` | 0.0500 | estimado |
| `J13` | fio -> TRS linha R | header 1x3 2,54 | — | `PinHeader_1x03_P2.54mm_Vertical` | 0.0500 | estimado |
| `J14` | IDC 2x13 -> painel | IDC 2x13 macho blindado (3M N2526-6002RB ou gen.) | LCSC | `IDC-Header_2x13_P2.54mm_Vertical` | 0.4500 | estimado |
| `J15` | fio -> DPDT tipo 2, canal L | header 1x6 2,54 | — | `PinHeader_1x06_P2.54mm_Vertical` | 0.0800 | estimado |
| `J16` | fio -> DPDT tipo 2, canal R | header 1x6 2,54 | — | `PinHeader_1x06_P2.54mm_Vertical` | 0.0800 | estimado |
| `RV1` **(DNP)** | trim 100k | 3296W-1-104LF (Bourns) | LCSC C48084 | `Potentiometer_Bourns_3296W_Vertical` | — | medido |
| `RV2` **(DNP)** | trim 100k | 3296W-1-104LF (Bourns) | LCSC C48084 | `Potentiometer_Bourns_3296W_Vertical` | — | medido |
| `U2` | Daisy Seed3 | Daisy **Seed3** (Electrosmith) | Electrosmith | `Electrosmith_Daisy_Seed` | 29.9900 | medido |

## 3 · PAINEL — SMD (JLC monta)

| ref | valor | peca / MPN | fornecedor | footprint | US$ | fonte |
|---|---|---|---|---|---|---|
| `C1` | 100n COLADO ao cursor | condensador 0805 — ver o valor p/ dielectrico | — | `C_0805_2012Metric` | 0.0050 | estimado |
| `C2` | 100n COLADO ao cursor | condensador 0805 — ver o valor p/ dielectrico | — | `C_0805_2012Metric` | 0.0050 | estimado |
| `Q1` | AO3400A | AO3400A (Alpha & Omega) | LCSC C20917 | `SOT-23` | 0.0487 | medido |
| `Q2` | AO3400A | AO3400A (Alpha & Omega) | LCSC C20917 | `SOT-23` | 0.0487 | medido |
| `R1` | 100R gate | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R2` | 100R gate | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R3` | 10k pull-down | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R4` | 10k pull-down | resistor 0805 1% | — | `R_0805_2012Metric` | 0.0050 | estimado |
| `R5` | 100R serie (~30 mA) | resistor 1210 1/2 W | — | `R_1210_3225Metric` | 0.0200 | estimado |
| `R6` | 100R serie (~30 mA) | resistor 1210 1/2 W | — | `R_1210_3225Metric` | 0.0200 | estimado |

## 4 · PAINEL — THT (o dB solda)

| ref | valor | peca / MPN | fornecedor | footprint | US$ | fonte |
|---|---|---|---|---|---|---|
| `J1` | IDC 2x13 femea <- cerebro | IDC 2x13 femea (3M N3793-6002RB ou gen.) | LCSC | `IDC-Header_2x13_P2.54mm_Vertical` | 0.4500 | estimado |
| `J2` | OLED SSD1309 MX1.25 7p | 53047-0710 (Molex PicoBlade 1x7) | LCSC | `Molex_PicoBlade_53047-0710_1x07_P1.25mm_Vertical` | 0.3000 | estimado |
| `J3` | fio -> arcade AZUL freeze A | JST XH 1x2 (B2B-XH-A) | — | `JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical` | 0.0500 | estimado |
| `J4` | fio -> arcade AZUL freeze B | JST XH 1x2 (B2B-XH-A) | — | `JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical` | 0.0500 | estimado |
| `J5` | fio -> arcade BRANCO buzios | JST XH 1x2 (B2B-XH-A) | — | `JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical` | 0.0500 | estimado |
| `J6` | fio -> LED do arcade A (1=A, 2=K) | JST XH 1x2 (B2B-XH-A) | — | `JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical` | 0.0500 | estimado |
| `J7` | fio -> LED do arcade B (1=A, 2=K) | JST XH 1x2 (B2B-XH-A) | — | `JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical` | 0.0500 | estimado |
| `RV1` | 10k lin | PTV09A-4020S-B103 (Bourns) | distribuidor | `—` | 0.6000 | estimado |
| `RV2` | 10k lin | PTV09A-4020S-B103 (Bourns) | distribuidor | `—` | 0.6000 | estimado |
| `SW1` | EC11 — o unico encoder | EC11E15244G1 (Alps Alpine) | LCSC C370970 | `RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` | 1.7000 | medido |

## 5 · Fora das placas — modulos, painel, cablagem, caixa

| tipo | peca | fornecedor | qtd | US$/un | fonte | nota |
|---|---|---|---|---|---|---|
| modulo | ESP32-C3 SuperMini (ponte BLE) | AliExpress | 1 | 4.00 | estimado | 5 fios ao soquete; o modulo cru ESP32-C3-MINI-1-H4 na LCSC e US$2,17 |
| modulo | Amp classe D TPA3116D2 estereo | AliExpress/Amazon | 1 | 7.00 | estimado | ja comprado (Clyxgs); terminais de parafuso, fora da placa |
| modulo | Step-down 5 V fixo 1,5 A | AliExpress | 1 | 1.50 | estimado | ja comprado (Euogeudel, 5 un.); SAIDA FIXA, por decisao |
| modulo | Decoy PD 15 V / 5 A | AliExpress | 1 | 3.00 | estimado | ja comprado (AITRIP); receptaculo USB-C na face de tras |
| painel | OLED Waveshare 2,42" SSD1309 SPI | Waveshare | 1 | 18.00 | estimado | ja comprado; liga por PicoBlade 7 vias |
| painel | Botao arcade 30 mm iluminado AZUL 5 V | EG Starts | 2 | 2.50 | estimado | ja comprados. ⚠️ A COR E SPEC: a 3,3 V o amarelo nao acende |
| painel | Botao arcade 30 mm BRANCO (buzios) | EG Starts | 1 | 2.50 | estimado | sem LED |
| painel | Jack TRS 1/4" NMJ6HCD2 (Neutrik) | Stompboxparts | 4 | 1.75 | medido | isolado do chassis; 2 entradas de piezo + 2 saidas de linha |
| painel | DPDT ON-ON-ON tipo 2 | ja comprados | 2 | 2.00 | estimado | ⚠️ MEDIR O TIPO COM MULTIMETRO ANTES DE FIAR |
| cablagem | Cabo IDC 2x13, 26 vias, ~150 mm, crimpado | — | 1 | 2.00 | estimado | testar cada crimpe POR TRACAO |
| cablagem | Cabo USB-C curto 15 cm C-C com dados (PD) | — | 1 | 3.00 | estimado | do powerbank ao decoy |
| caixa | Caixa (ver docs/desenhos/caixa/) | — | 1 | — | por orcar | paredes 6 mm, tampa 9 mm; furos na tampa para encoder e 2 pots |

## 6 · Ordem de grandeza

**≈ US$ 97.39 por instrumento**, so componentes e modulos.

⚠️ **NAO inclui**: PCB, montagem da JLC, taxa de *setup* das pecas
Extended, caixa, parafusos, knobs, nem transporte. E a soma mistura precos
**medidos** com **estimados** — vale como ordem de grandeza, nao como orcamento.

Os DNP nao contam para o total: o footprint fica na placa, a peca nao se compra.


## 7 · Valores que o esquema e o MPN nao dizem igual

| ref | esquema | MPN escolhido | |
|---|---|---|---|
| ~~`C12`, `C22`~~ | ~~`20n C0G`~~ | | **RESOLVIDO 05/09**: o esquema passou a `22n C0G`. 20 nF C0G nao e valor de serie; o esquema mentia ao BOM. |
| `R5`, `R6` (painel) | `100R serie (~30 mA)` | 47 Ω 1/2 W na `BOM-MPN-revB` | por medir na bancada com o azul a 5 V |

Enquanto nao baterem, a BOM esta a comprar uma peca e o desenho a descrever
outra — e nenhum verificador apanha isso.

## 8 · O que ainda trava o pedido

1. **`OPA1642AIDR` na biblioteca da JLC.** As 980 unidades na LCSC nao dizem
   nada sobre isso. Se nao estiver: **`OPA1652AIDR`** (mesmo SOIC-8, CMOS,
   4,5 nV), ou pedir a JLC que compre na LCSC como *pre-order*.
2. **`RV1`/`RV2` do painel sem footprint.** O KiCad traz o `PTV09A-1` e o
   `-2`; a peca compravel e o **`-4`**, e nao esta confirmado que partilham o
   desenho de PCB. Le-se o desenho do `-4` na ficha, ou mede-se a peca.
3. **`R5`/`R6` do painel: valor por medir.** Fica `100R` no 1210 como partida;
   fecha-se na bancada com o azul a 5 V, medindo a corrente.
4. ~~A Daisy Seed rev4.~~ **RESOLVIDO a 05/09: passou a Seed3.** A pinagem foi
   conferida pino a pino entre os dois CSV do fabricante — **o nome STM32 bate
   nos 40 pinos, sem uma excepcao** — e a doc diz textualmente *"maintaining
   pin-to-pin compatibility with the original Seed pinout and footprint"*.
   Simbolo, footprint e netlist ficaram **identicos**. O que mudou a favor:
   codec **TI TAC5242**, 32 bit / 192 kHz, chao de ruido de -120 dB (contra o
   AK4556 do rev4) — e este projecto passou a sessao de 05/09 a cacar chao de
   ruido. Nada muda contra: o dB ja grava por **USB-C** desde
   que a primeira placa queimou — a placa de bancada ja e de USB-C. A caixa
   nunca teve recorte para o USB da Daisy e continua a nao ter: grava-se com a
   caixa aberta.

