# BOM por MPN — Cotucatronics rev B
### 05/09/2026 · dois projetos KiCad · JLCPCB monta o SMD · dB compra e solda o resto

> ⚠️ **Nomes actualizados a 05/09 para os do esquema.** O KiCad trata uma
> referência sem parte numérica (`R0A`, `C1A`, `RVA`, `DA`) como não anotada e
> renumera-a sozinha ao gravar. A dezena passou a dizer o canal:
> `R10`–`R14` = canal A, `R20`–`R24` = canal B, `C11`–`C14` / `C21`–`C24`,
> `RV1`/`RV2`, `D11`/`D21`. Mapa completo em `ERC-ISENCOES.md`.
> **Os 10 k das entradas do amp são `R50`/`R51`** — a dezena 2 foi ocupada
> pelo canal B do pré-amp, e a referência é única em toda a hierarquia.
> **Regra:** o footprint vem da folha de dados do MPN escolhido, não de medição.
> Só se mede o que não tem ficha (§5). Códigos LCSC: o Code puxa na hora do
> pedido com stock confirmado — a ferramenta de BOM da JLC casa por MPN.
> Critério de escolha: **peça de fabricante conhecido, em stock na LCSC, no
> tier Basic quando existe** (Basic = sem taxa de setup; Extended = ~US$3/peça
> uma vez por pedido).

---

## 1 · CÉREBRO — SMD, JLCPCB monta (face de cima)

### Semicondutores
| ref | qtd | peça | MPN | encaps. | tier | nota |
|---|---|---|---|---|---|---|
| U1 | 1 | amp-op dual JFET | **OPA1642AIDR** (TI) | SOIC-8 | Extended | **confirmar stock antes do pedido — trava tudo.** Alternativa se faltar: OPA1652AIDR (CMOS, 4,5 nV) |
| D11, D21 | 2 | díodo duplo série | **BAV99,215** (Nexperia) | SOT-23 | Basic | clamp da entrada; pino 3 = sinal |
| D2 | 1 | Schottky 1 A 40 V | **SS14** (MDD ou Vishay) | SMA | Basic | o 1N5819 da bancada, em SMD; faixa para a ponte |
| D1 | 1 | TVS 24 V standoff | **SMBJ24A** (Littelfuse) | SMB | Basic/Ext | cátodo no +15 V |
| F1 | 1 | PTC 3 A hold, ≥ 24 V | **2920L300/24MR** (Littelfuse) | 2920 | Extended | os 1812 de 3 A só chegam a 16 V — 15 V PD é margem zero |

### Resistores 0805, 1 %, filme espesso (Uniroyal 0805W8F… ou Yageo RC0805FR — todos Basic)
| ref | qtd | valor | nota |
|---|---|---|---|
| R11, R21 | 2 | 4,7 MΩ | polarização da entrada — **1 %**; 5 % desloca o VBIAS do nó |
| R12, R22 | 2 | 1 kΩ | proteção da entrada |
| R13, R23 | 2 | 4,7 kΩ | **DNP** — perna de ganho, só se voltar o trimpot |
| R10, R20 | 2 | 0 Ω | seguidor: OUT → −IN |
| R14, R24 | 2 | 100 Ω | isolamento de saída |
| R5, R6 | 2 | 100 kΩ | divisor VBIAS |
| R8 | 1 | 100 kΩ | divisor do trilho → A2 |
| R9 | 1 | 22 kΩ | idem |
| R50, R51 | 2 | 10 kΩ | entradas do amp ao GND (fora da chave) — **eram `R10`/`R11`; a dezena 1 passou a ser do canal A do pré-amp** |
| R7 | 1 | 100 Ω **¼ W, 1206** | filtro V+PRE; 0805 é ⅛ W |

### Capacitores SMD
| ref | qtd | valor | MPN / spec | encaps. | tier | nota |
|---|---|---|---|---|---|---|
| C11, C21 | 2 | 22 pF **C0G** 50 V | qualquer C0G Basic | 0805 | Basic | **DNP** com o seguidor |
| C12, C22 | 2 | 22 nF **C0G** 50 V | **C1206C223J5GACTU** (Kemet) | **1206** | Extended | divisor −6 dB; **alternativa PPS: ECHU1C223GX5** (Panasonic). Nunca X7R aqui |
| C7, C9, C9, C4, C2 | 5 | 100 nF X7R 50 V | Basic | 0805 | Basic | desacople (energia + pré-amp) |
| C8 | 1 | 10 µF X7R **50 V** | Basic | **1206** | Basic | colado ao pino 8 de U1 (14,6 V — 0805/25 V perde metade por polarização) |
| C4 (energia) | 1 | 100 nF X7R | Basic | 0805 | Basic | no terminal do amp, ao lado do 2200 µ |

### Sem peça (não entram na BOM nem no CPL)
NetTie-2 (AGND↔GND, cobre) · PWR_FLAG ×4.

---

## 2 · CÉREBRO — THT, dB compra e solda

### Eletrolíticos radiais (Panasonic FR = low-ESR, longa vida; footprint da ficha)
| ref | qtd | valor | MPN | ⌀ × alt. · passo | nota |
|---|---|---|---|---|---|
| C1 (energia) | 1 | 2200 µF 35 V | **EEU-FR1V222** (Panasonic FR) | 16 × 25 · **P7,5** | no terminal do amp — medido: sem ele o exciter zumbe |
| C3 (energia) | 1 | 100 µF 16 V | **EEU-FR1C101** | 6,3 × 11 · P2,5 | na ponte |
| C6, C6 (pré-amp) | 2 | 470 µF 25 V | **EEU-FR1E471** | 10 × 12,5 · P5 | reservatório V+PRE |
| C5 (pré-amp) | 1 | 47 µF 25 V | **EEU-FR1E470** | 6,3 × 11 · P2,5 | VBIAS |
| C13, C23 (pré-amp) | 2 | 22 µF **bipolar** 50 V | **UES1H220MEM** (Nichicon UES, áudio) | 6,3 × 11 · P2,5 | **DNP** com o seguidor; footprint fica |

### Filme
| ref | qtd | valor | MPN | corpo · passo | nota |
|---|---|---|---|---|---|
| C14, C24 | 2 | 2,2 µF 63 V | **MKS2C042201K00KSSD** (WIMA MKS2) | 7,2 × 11 × 7,2 · **P5** | acoplamento de saída — filme, nunca X7R |

### Trimpot (DNP, footprint fica)
| ref | qtd | peça | MPN |
|---|---|---|---|
| RV1, RV2 | 2 | 100 kΩ multivoltas | **3296W-1-104LF** (Bourns) |

### Conectores e soquete
| ref | qtd | peça | MPN / spec | nota |
|---|---|---|---|---|
| soquete Daisy | 2 | fêmea 1×20, 2,54, contactos dourados | **M20-7822046** (Harwin) ou equiv. | substituibilidade — vale os US$2 |
| J2 (amp 15 V), J_L, J_R (linha ao amp) | 3 | terminal parafuso 2 vias, 5,08 | **1935161** (Phoenix MKDS 1/2-5.08) ou KF301-5.0 genérico | Phoenix se for série; KF301 na bancada |
| J step-down | 1 | fêmea 1×3, 2,54 | genérico | passo confirmado quando o módulo chegar |
| J decoy | 1 | terminal parafuso 2 vias 5,08 | idem J2 | |
| J ponte | 1 | fêmea 1×5, 2,54 | genérico | TX RX STATUS 5V GND |
| J painel | 1 | IDC 2×13 macho, blindado, 2,54 | **N2526-6002RB** (3M) ou genérico | o contrato entre placas |
| J jacks piezo / linha | 4 | header 1×3, 2,54 | genérico | fio aos jacks de painel |
| J DPDT | 2 | header 1×6, 2,54 | genérico | fio às chaves |

---

## 3 · PAINEL — SMD, JLCPCB monta
| ref | qtd | peça | MPN | encaps. | tier | nota |
|---|---|---|---|---|---|---|
| Q1, Q2 | 2 | MOSFET N logic-level | **AO3400A** (Alpha & Omega) | SOT-23 | Basic | 5 A; margem contra os 100 mA por LED |
| R_g1, R_g2 | 2 | 100 Ω | 0805 Basic | | | gate |
| R_pd1, R_pd2 | 2 | 10 kΩ | 0805 Basic | | | **pull-down obrigatório** — GPIO flutua no boot |
| R_led1, R_led2 | 2 | 47 Ω **½ W** | 1210 ou 2512 | Basic | | série dos LEDs, alvo ~30 mA; **valor confirmado na bancada** com o azul |
| C_pot1, C_pot2 | 2 | 100 nF X7R | 0805 Basic | | | do cursor ao GND, colado ao pot |

## 4 · PAINEL — THT, dB compra e solda
| ref | qtd | peça | MPN | nota |
|---|---|---|---|---|
| SW1 | 1 | EC11 com push, eixo 20 mm | **PEC11R-4220F-S0024** (Bourns) | 24 detentes, eixo estriado; se ficares com o comprado, footprint por medição |
| RV1, RV2 | 2 | pot 10 kΩ **linear**, 9 mm, eixo 20 mm | **PTV09A-4020F-B103** (Bourns) | eixo estriado ⌀6 — confere o furo do knob ⌀13 |
| J_OLED | 1 | header 1,25 mm 7 vias vertical | **53047-0710** (Molex PicoBlade) | é o "MX1.25" do módulo Waveshare |
| J painel | 1 | IDC 2×13 fêmea | **N3793-6002RB** (3M) ou genérico | |
| J_frzA, J_frzB (switch) | 2 | JST XH 2 vias | **B2B-XH-A** | fio ao microswitch do arcade |
| J_ledA, J_ledB | 2 | JST XH 2 vias | **B2B-XH-A** | fio ao LED do arcade |
| botões arcade | 2 | 30 mm iluminado **azul** 5 V | EG Starts (já comprados) | **cor é spec** — a 3,3 V o amarelo não acende |
| OLED | 1 | Waveshare 2,42" SSD1309 SPI | já comprado | |

---

## 5 · FORA DAS PLACAS — comprados, ligam por fio

| peça | qtd | MPN / spec | nota |
|---|---|---|---|
| decoy PD 15 V / 5 A | 1 | AITRIP (já comprado) | receptáculo USB-C na face de trás |
| step-down 5 V fixo 1,5 A | 1 | Euogeudel (já comprado, 5 un.) | **saída fixa**, por decisão |
| amp classe D estéreo | 1 | Clyxgs TPA3116D2 (já comprado) | terminais de parafuso; fora da placa |
| jacks TRS ¼" painel | 4 | **NMJ6HCD2** (Neutrik) | isolados do chassis; qualidade/preço |
| DPDT ON-ON-ON tipo 2 | 2 | já comprados | **medir tipo com multímetro antes de fiar** |
| ponte BLE | 1 | ESP32-C3 SuperMini | 5 fios |
| cabo IDC 2×13 | 1 | 26 vias, ~150 mm, crimpado | testar cada crimpe **por tração** |
| cabo USB-C curto | 1 | 15 cm, C-C, com dados (PD) | do powerbank ao decoy |

## 6 · O que ainda se MEDE (sem ficha útil)
Jacks (rosca, porca, profundidade, T/R/S) · DPDT (rosca, profundidade, **tipo**) ·
step-down e decoy (passo dos pinos, furos) · terminal do amp · arcade (⌀ do
furo, aro) · OLED (furos c-c). É a folha `MEDICAO-21-PECAS.md`, secções 3, 4,
8 e 9 — as outras secções ficam obsoletas por esta BOM.

## 7 · Antes de submeter à JLC
1. Code: LCSC de cada linha SMD, **com stock**, e o tier real. Onde o Basic
   não existir, dizer quantos Extended há (cada um custa setup).
2. **OPA1642AIDR em stock?** Se não: OPA1652AIDR é a substituta (mesmo
   footprint, CMOS, 4,5 nV — ligeiramente pior em ruído de corrente, ainda
   muito acima do necessário).
3. **C0G 22 nF 1206** em stock? Se não: PPS ECHU1C223GX5. Se nenhum: 2× 10 nF
   C0G 0805 em paralelo (footprint duplo).
4. BOM e CPL sem NetTie, sem PWR_FLAG, com os DNP marcados como DNP.
5. Montagem só na face de cima; THT nada montado pela JLC.

---

## 8 · Achados de 05/09 ao atribuir os footprints

Verificado ficheiro a ficheiro e contra as fichas dos fabricantes. **Três
escolhas desta BOM não sobrevivem ao confronto com o desenho mecânico.**

### 8.1 O pot `PTV09A-4020F-B103` não serve

O estilo **`-4` é *Horizontal/Rear Mount, bushingless*** — sem rosca e com o
eixo deitado. A decisão de 05/09 é o pot **na placa, eixo a subir pelo furo
ø7 da tampa**, o que pede um estilo **vertical com bucim roscado**.

O estilo vertical é o **`PTV09A-1`**, e é o único que o KiCad traz footprint
(`Potentiometer_Bourns_PTV09A-1_Single_Vertical`: 3 pinos a 2,5 mm, dois
postes ø2,2 a x=7). **A rever antes de comprar.**

### 8.2 O encoder `PEC11R` não tem footprint, e não é o Alps

A ficha da Bourns põe os três pinos do encoder a **5,0 mm**; o `EC11E` da
Alps, que é o que o KiCad traz, põe-nos a **2,5 mm**. Não são intercambiáveis
por muito que ambos se chamem "EC11 de 12 mm".

Duas saídas: desenhar footprint próprio para o PEC11R (cotas na ficha:
pinos ø1,0 a 5,0 mm, patilhas 2,6 × 1,8, vãos 7,0 e 7,5), **ou** escolher um
Alps `EC11E` e usar o footprint que já existe. A segunda é mais barata em
trabalho e em risco.

### 8.3 `R5`/`R6` do painel: valor e encapsulamento em desacordo

A BOM diz **47 Ω ½ W**; o esquema diz **`100R serie (~30 mA)`**. E o
encapsulamento estava em 0805, que é **⅛ W** — errado em qualquer dos casos.
Ficou **1210**; se a peça escolhida for de ¼ W, sobe para 2512. **O valor
continua por decidir.**

### 8.4 Outras correcções aplicadas

| ref | estava | ficou | porquê |
|---|---|---|---|
| `F1` | 1812 | **2920** | os 1812 de 3 A só vão a 16 V — razão da própria BOM |
| `C8` | 0805 | **1206** | 10 µF X7R 50 V; 0805/25 V perde metade por polarização |
| `C12`/`C22` | — | **1206** | C0G 22 nF (Kemet `C1206C223J5GACTU`) |
| `J3`–`J7` (painel) | header 2,54 | **JST XH** `B2B-XH-A` | conector com trava |
| `J2` (painel) | — | **PicoBlade `53047-0710`** | é o "MX1.25" do Waveshare |
| `J1`,`J2` (energia) | header 2,54 | **Phoenix MKDS-1,5-2-5.08** | terminal de parafuso |
| radiais e filme | — | da ⌀×passo desta BOM | `CP_Radial_D16.0mm_P7.50mm`, `D6.3mm_P2.50mm`, `D10.0mm_P5.00mm`, `C_Rect_L7.2mm_W11.0mm_P5.00mm_…MKS2` |

⚠️ O footprint do WIMA foi escolhido pelo nome (`…MKS2`, passo 5) e pelo `11`
da linha desta BOM. **Confere à craveira** quando a peça chegar: as três cotas
`7,2 × 11 × 7,2` não dizem qual é a espessura.

### 8.5 Divergências de contagem, por resolver

- A BOM listava `C6A`/`C6B` (2× 470 µF) e `C7A`/`C7B` (2× 100 n); **o esquema
  tem um de cada** — alimentação partilhada, não duplicada por canal.
- A BOM e o `ERC-ISENCOES.md` dizem **quatro `PWR_FLAG`**; o esquema tem
  **dois**, e sobram exactamente **2 `power_pin_not_driven`** no cérebro.

### 8.6 Seis peças que ainda são desenhadas como peças e deviam ser conectores

Esta BOM já decide que ligam por fio (`header 1×3` para os jacks, `1×6` para
os DPDT), mas o esquema desenha-as como peças: `J5`/`J6` (piezo), `J12`/`J13`
(TRS de linha), `SW1`/`SW2` (DPDT). É a mesma correcção já feita aos botões
arcade do painel.

### 8.7 Stock, medido a 05/09

`OPA1642AIDR` na LCSC: **`C67640`, 980 unidades, US$ 1,0145** a 5–10 un.
⚠️ Stock da LCSC **não é** stock de montagem da JLC — são armazéns diferentes,
e o tier Basic/Extended confirma-se na biblioteca de peças da JLC, não aqui.

---

## 9 · Decisões de 05/09 e custo por peça

### 9.1 O que mudou de peça

| ref | era | passa a ser | porquê |
|---|---|---|---|
| `RV1`,`RV2` (painel) | `PTV09A-4020F-B103` | **`PTV09A-4020S-B103`** | só o `F` muda: `F` é eixo **liso**, `S` é **estriado 18 dentes**, que é o que estava escrito na nota. O estilo `-4` mantém-se — é *bushingless*, mas com o pot **na placa** o furo da tampa é passagem e quem segura é o PCB. ⚠️ O `-1` não existe em stock em 10 kΩ; só a família `-4`. |
| `SW1` (painel) | Bourns `PEC11R-4220F-S0024` | **Alps `EC11E15244G1`** | a ficha da Bourns põe os pinos a **5,0 mm**; o KiCad só traz footprint do Alps, a **2,5 mm**. Footprint próprio é trabalho e risco para poupar nada. |
| `F1` | `2920L300/24MR` | **`2920L330/24MR`** | ⚠️ **o 300/24 não aparece em stock em lado nenhum.** O que existe a 24 V é o **3,3 A**; o `2920L300/15DR` é 3 A mas só a **15 V**, que é a margem zero que esta BOM já rejeitava. |
| `C14`,`C24` | 7,2 × 11 × 7,2 | corpo real **7,2 × 7,2 × 13**, passo 5,0 | medido na ficha do `MKS2C042201K00KSSD`. ⚠️ O KiCad **não tem** `L7,2 × W7,2`; ficou o `W11.0`, que é **maior** que a peça — folga a mais, nunca a menos. Confere à craveira. |
| `R5`,`R6` (painel) | 47 Ω ½ W | **`100R`, 1210, valor por medir** | 30 mA em 100 Ω são 90 mW; 0805 é ⅛ W e não aguenta. O valor fecha-se na bancada, com o azul a 5 V, medindo a corrente. |

### 9.2 Custo por peça — **medido a 05/09**

| peça | fonte | preço unit. | stock |
|---|---|---|---|
| `OPA1642AIDR` | LCSC `C67640` | **US$ 1,0145** (5–10 un.) | 980 |
| Alps `EC11E15244G1` | LCSC `C370970` | **US$ 1,70** (5 un.) | 1 125 |
| Neutrik `NMJ6HCD2` ×4 | Stompboxparts | **US$ 1,75** (25+: 1,70) | 196 |
| Panasonic `EEU-FR1V222` | LCSC `C407956` | **US$ 0,7162** | em stock |
| Bourns `3296W-1-104LF` ×2 | LCSC `C48084` | **US$ 0,4458** | 11 068 |
| Littelfuse `2920L330/24MR` | LCSC `C720183` | **US$ 0,1804** | 4 075 |
| `AO3400A` ×2 | LCSC | **US$ 0,0119–0,0487** | vários |
| `SMBJ24A` | LCSC | **US$ 0,0143–0,0793** | vários |
| C0G 22 nF 1206 | LCSC (comparável) | **~US$ 0,0165** | — |

### 9.3 Custo por peça — **estimado, não medido**

Passivos 0805/1206 Basic ~US$ 0,002–0,01 · `BAV99` e `SS14` ~US$ 0,02 ·
headers 2,54 ~US$ 0,05–0,15 · `JST XH` ~US$ 0,05 · PicoBlade ~US$ 0,30 ·
IDC 2×13 ~US$ 0,30–0,60 · terminal Phoenix ~US$ 0,80–1,50 (o `KF301`
genérico ~US$ 0,15) · soquete Daisy ~US$ 2 (nota da própria BOM) ·
electrolíticos FR pequenos ~US$ 0,10–0,30 · WIMA `MKS2` ~US$ 0,50–0,90 ·
Nichicon `UES` ~US$ 0,30 · pot `PTV09A` ~US$ 0,60.

### 9.4 Ordem de grandeza por conjunto

| bloco | ~US$ |
|---|---|
| cérebro, lado SMD | 1,50 |
| cérebro, THT | 9,60 |
| painel, SMD | 0,20 |
| painel, THT | 4,00 |
| 4 jacks Neutrik | 7,00 |
| **componentes, por conjunto** | **≈ 22** |

⚠️ **Não inclui** PCB, montagem da JLC, taxa de *setup* dos Extended, nem o
que já está comprado (arcade, OLED, TPA3116, step-down, decoy, ESP32-C3).

### 9.5 A verificação que fica, e é a que trava tudo

**Stock da LCSC não é stock de montagem da JLC.** São armazéns diferentes. As
980 unidades do `OPA1642AIDR` na LCSC não dizem nada sobre a biblioteca de
peças da JLC, e o tier Basic/Extended confirma-se **lá**, não aqui.

Se o `OPA1642` não estiver na biblioteca da JLC, a resposta **não é esperar**:
é o **`OPA1652AIDR`** (mesmo footprint SOIC-8, CMOS, 4,5 nV), ou pedir à JLC
que o compre na LCSC como peça de *pre-order* — eles fazem, com prazo.

### 9.6 O que ainda não tem footprint

`RV1`/`RV2` — 2 de 79. O KiCad traz footprint do `PTV09A-1` e do `-2`; a peça
comprável é o **`-4`**, e **não confirmei** que os três partilham o mesmo
desenho de PCB. Facto de hardware sem fonte citada é hipótese: ou se lê o
desenho do `-4` na ficha, ou se mede a peça na mão.

---

## 10 · 05/09 — a placa passou a **Daisy Seed3**

O rev4 esteve com produção suspensa; o Seed3 voltou a stock e é o que vai.

**A pinagem foi conferida, não assumida.** Comparei os dois CSV do fabricante
pino a pino: **o nome STM32 bate nos 40 pinos, sem uma excepção.** A doc do
Seed3 diz textualmente *"maintaining pin-to-pin compatibility with the
original Seed pinout and footprint"*, e o desenho de pinagem que o dB mandou
confirma pino a pino nos que este projecto usa.

Resultado no esquema: **símbolo, footprint e netlist idênticos.** A troca não
mexeu numa ligação — provado por comparação de netlist, 63 redes iguais.

**⚠️ Duas armadilhas encontradas pelo caminho, e valem para a próxima vez:**

1. **Os dois CSV têm colunas com sentidos diferentes.** Onde o do rev4 põe a
   função alternativa (`SDMMC1_D3`), o do Seed3 põe `GPIO`. Comparar coluna a
   coluna dá **35 "diferenças" que não existem**. Compara-se o **nome STM32**.
   O `gera_daisy_sym.py` passou a ler pelo NOME do cabeçalho, e é por isso que
   rebentou alto quando a fonte mudou, em vez de emitir um símbolo errado.
2. **O CSV do fabricante contradiz o desenho do fabricante.** No pino 40 a
   coluna do nome diz `PG3`; a da função diz `GND`; o desenho diz `DGND`. O
   gerador passou a recusar nomes que sejam designação de porta do MCU
   (`P[A-K]n`) e a **avisar** qual descartou — escolher em silêncio entre duas
   fontes que discordam é o mesmo que inventar.

**O que muda a favor:** codec **TI TAC5242**, 32 bit / 192 kHz, chão de ruído
de −120 dB, contra o AK4556 do rev4. Esta mesma sessão passou horas a recuperar
35–39 dB de chão de ruído; a peça nova entra por cima disso.

**O que muda contra: nada de prático.** Eu tinha escrito que "o cabo de
gravação muda" — **estava errado**: o dB grava por USB-C desde que a primeira
placa queimou, e a placa de bancada já é de USB-C. A caixa nunca teve recorte
para o USB da Daisy e continua a não ter: grava-se com a caixa aberta.

**Fonte versionada:** `kicad/cotuca_cerebro/Seed3_pinout.csv`, tirado de
`daisy.nyc3.cdn.digitaloceanspaces.com/products/seed3/`. Uma gralha corrigida
de caminho: o pino 17 chamava-se `AUDIO_INR` (falta de espaço no CSV do rev4)
e passou a `AUDIO_IN_R`.
