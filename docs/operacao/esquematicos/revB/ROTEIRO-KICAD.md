# Cotucatronics — roteiro KiCad → fábrica
### 04/09/2026 · pré-amp aprovado em bancada, caixa formato pedal fechada

---

## 1 · Fornecedor: JLCPCB para os protótipos

**Veredito:** JLCPCB. É o mais barato e mais rápido para placas de 2–4
camadas com componentes do stock LCSC — que é o caso, porque o BOM já foi
desenhado a pensar nisso. PCBWay cobra mais nas placas simples e ganha em
flexibilidade (BOM próprio, THT montado, revisão humana) e em consistência de
qualidade; para este projeto isso só vale a pena se o OPA1642 faltar no stock
da LCSC ou se se quiser o soquete da Daisy montado por eles.

| | JLCPCB | PCBWay |
|---|---|---|
| preço, 2–4 camadas, 5–10 placas | **o mais baixo** | ~20–40 % acima |
| montagem SMT | económica se **100 % do BOM em stock LCSC** | aceita peças próprias |
| THT montado | limitado | sim |
| prazo (fab) | 24 h a 3–5 dias | 3–7 dias SMT |
| entrega EUA (DHL) | +5–8 dias | igual |
| quando escolher | **este projeto** | BOM fora da LCSC, ou série com QC reforçado |

**Prática:** 4 camadas (L2 terra sólido), montagem SMT só na face de cima,
THT à mão. **5 protótipos**, nunca quantidade. Pedir a data de **entrega**, não
a de fabrico. Confirmar o stock do **OPA1642** antes de submeter — peça sem
stock trava o pedido inteiro.

---

## 2 · Duas placas, um contrato

| placa | o que leva | onde fica |
|---|---|---|
| **cérebro** | Daisy no soquete · pré-amp · ponte ESP32 · filtro/banco de energia · DPDT (fiação) · jacks (fiação) | fundo da caixa |
| **painel** | OLED · encoder · 2 pots · **3** botões arcade (só 2 com LED) + 2 MOSFET | tampa |
| **fora das placas** | módulo decoy PD · módulo step-down · módulo TPA3116 | decoy e amp atrás/direita; step-down junto ao cérebro |

**Contrato = IDC 2×13.** Pinagem no §5. O mesmo nome de rede nos dois
projetos, via a via.

---

## 3 · A caixa: quatro faces

Desenho em [`docs/desenhos/caixa/`](../../../desenhos/caixa/) — seis páginas,
geradas por `gen_caixa_paginas.py` — ⚠️ **apagadas a 06/09**, descreviam uma
caixa que nunca foi cortada; ver `caixa/README.md`. **Cunha**, 200 × 120:
**frente 40 mm, trás 70 mm**. Sem interruptor de energia — ligar o USB é ligar.

**Perfil de altura, e é restrição de layout antes da primeira peça:** os arcade
têm 30 mm dentro da caixa e ficam à frente, na parte baixa. Por baixo deles
nada acima de ~8 mm — pré-amp, step-down, ESP32 deitada. Tudo o que é alto vai
atrás: Daisy no soquete (~20), módulo do amp com dissipador (~25), os
eletrolíticos grandes. **Frente baixa = coisas baixas e o pé; trás alta = coisas
altas e as mãos.** A face da frente é a da marca — búzio e nome, sem furos.

| face | o que sai | porquê |
|---|---|---|
| **esquerda** | 2× TRS piezo IN | escultura entra por aqui |
| **direita** | 2× bornes exciter OUT | escultura sai por aqui — **os dois cabos nunca correm juntos** |
| **trás** | USB-C PD · 2× TRS linha OUT · 2× DPDT | o mundo: energia, mesa, configuração |
| **frente** | nada — búzio e nome | a face do público |
| **cima** | OLED · encoder · 2 pots · 2 freeze | performance |

Por baixo: **pré-amp na metade esquerda** (encostado aos jacks de entrada,
ilha de AGND), **amp na metade direita**, conector do painel na borda oposta
ao pré-amp. Pots e encoder do lado do amp.

---

## 4 · Folhas do cérebro

**Energia** — atualizada 04/09: **sem interruptor**. Decoy → PTC → TVS →
trilho 15 V → {J amp com 2200 µF + 100 nF · step-down}. 5 V → Daisy VIN ·
1N5819 → ponte. **Divisor 100 k/22 k do trilho ao A2 (D17)** — 15 V dão 2,7 V
no ADC: o firmware lê a tensão do trilho, mostra no OLED e **recusa ligar o amp
abaixo de 12 V** com mensagem clara. É a linha nova do `Requirements`.
Powerbank que não negoceia PD deixa de ser silêncio e passa a ser aviso.

**Pré-amp** — feita, ERC 0; **atualizar para o que a bancada aprovou:**
- seguidor (saída à entrada −) com a rede de ganho (RV, R3, C3, C1) em
  footprint **DNP** e 0 Ω populado no lugar do trimpot
- **C2 = 20 nF C0G** (os −6 dB da crista; ajustar ao piezo é facto de exemplar)
- D1 = BAV99 (os 1N4148 eram bancada) · jacks com ring + sleeve ao AGND

**Soquete + ponte + DPDT + saídas** — por fazer. **A armadilha:** `Dnn` → posição
no header 2×20 sai da **folha de dados da Seed**, não do PINOS.

| pino lógico | função | pino lógico | função |
|---|---|---|---|
| D0 D1 D2 | **o** encoder — A, B, push | D13 → RX · D14 ← TX | ponte, cruzado |
| D5 D6 D7 D8 D10 | OLED RST DC CS CLK DIN | D28 | status da ponte, ativo baixo |
| D15 D16 | pot L, pot R (A0/A1) | D22 D24 | freeze B, freeze A |
| D23 D27 | gate LED B, gate LED A | AUDIO IN/OUT L/R | pré-amp / DPDT |

**Livres: D3 D4 D9 D11 D12 D18 D19 D20 D21 D25** — dez. **D17 = A2 = V_TRILHO**, e
desde 05/09 sem conflito: o encoder G saiu.

> ⚠️ **Corrigido em 05/09. A lista anterior estava errada de duas maneiras.**
> Dizia `D3 D4 D9 D11 D12 D18 D19 D20 D21 D25 D26 D29 D30`, e:
>
> - **`D29`/`D30` são as linhas de dados do USB** (pinos 36/37,
>   `USB_HS_D−`/`D+`, no CSV do fabricante). Usá-las como GPIO mata a serial.
>   O sense dos DPDT ia para lá — vai agora para `D11`/`D12` ou `D20`/`D21`.
> - **`D3 D4 D9` são o encoder B, `D18 D19` o encoder G, `D25` o botão de
>   rearme e `D26` o dos búzios.** Estão todos ocupados e soldados.
>
> A fonte é o `Daisy_Seed_Rev4_Pinout.csv` cruzado com o
> `hal_daisy_seed.cpp` — ver `docs/operacao/PINOS.md`, que é onde esta lista
> vive. **O CSV manda; um resumo de memória não.**
DPDT tipo 2, comuns amarrados, 10 kΩ de cada entrada do amp ao GND do módulo.
**AGND e GND ligam-se num ponto só, nesta folha, nos pinos da Daisy.**

**Conector do painel** — IDC 2×13 macho, §5.

## Folha do painel

IDC fêmea · OLED (MX1.25 7p, 3V3 da Daisy) · EC11 5 pinos (comum e um pino
do push ao GND) · 2 pots 10 k lin (1→GND, 3→3V3 da Daisy, 100 nF do cursor
ao GND colado) · 2 botões arcade **azuis** (um lado ao GND) · 2× AO3400A
(gate 100 Ω, **pull-down 10 k obrigatório**, source ao GND) · LEDs do par de
5 V com resistor de série para ~30 mA. Um terra só: o DGND do IDC.

---

## 5 · IDC 2×13 — o contrato

**A pinagem vive no `kicad/cotuca_cerebro/gera_conector.py`, no dicionário
`VIAS`** — e é conferida via a via, dos dois lados, por
`tools/contrato_idc.py`, que lê os **ficheiros** e não os geradores.

```bash
python3 tools/contrato_idc.py
```

⚠️ **Esta secção listava as 26 vias, e a lista envelheceu.** O pino 19 deixou de
ser terra a 05/09 e passou a `BOT_BUZIOS`; a tabela daqui continuou a dizer
nove terras e o 19 livre, e a partir dela concluiu-se **duas vezes** que havia
uma via disponível que não havia. A edição de 05/09 até deixou a marca à vista
— a linha do pino 6 tinha *seis* colunas numa tabela de quatro, com o 20
repetido — e ninguém reparou, porque uma tabela à mão não tem quem a confira.

> É a **classe 7** da skill `audit`: tabela à mão por cima de uma gerada. Duas
> fontes do mesmo facto, e a que não corre é a que envelhece. Quem quiser ler a
> pinagem lê o `VIAS`; quem quiser saber se as duas metades batem corre o
> `contrato_idc.py`.

### O que NÃO está no gerador, e por isso fica escrito aqui

O arranjo das terras não é estética, e é a única coisa desta secção que não se
deduz da tabela:

- **relógio do OLED guardado por terra dos dois lados** (vias 1 e 3);
- **pots ensanduichados** por terra (23 e 26);
- **LED em par próprio** (11/12 alimentação, 13/14 retorno).

Com o painel aceso mediram-se **6 dB de chão na entrada**, e o caminho era este
cabo. As oito terras que restam guardam todas uma destas três coisas — não há
terra sobrante, e ceder uma tem custo conhecido.

---

## 6 · O que ainda não se desenha

Footprints de **step-down, decoy, jacks, DPDT, EC11**: só com a peça na mão. Footprint suposto é respin.

## 7 · Ordem

1. Atualizar o pré-amp (seguidor + DNP) → ERC 0
2. Folha do soquete → ERC 0
3. Folha do conector → ERC 0
4. Projeto do painel → ERC 0
5. Tabela via-a-via gerada dos dois projetos e conferida uma contra a outra
6. Footprints, depois de medir as peças
7. Layout 4 camadas → DRC → render para veto → revisão de terra → Gerbers + BOM + CPL → 5 protos
