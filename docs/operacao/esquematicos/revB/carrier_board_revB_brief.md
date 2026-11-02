# Cotucatronics — Carrier rev B · brief de projeto

> ⚠️⚠️ **A PLACA É DE 2 CAMADAS, e este documento dizia 4.** Medido a 2026-09-08 nos ficheiros: só `F.Cu` e `B.Cu`, com as zonas `GND` (prio 0) e `AGND` (prio 1) ambas em `B.Cu`, separadas, e o `NT1` como ponte única — e isso **está verificado e certo**. O que estava errado era o texto.
>
> Sem plano interior, o retorno do amp é a zona de `B.Cu`: aceitável para 3 A com a largura que tem, e dito para não ser descoberto na bancada. **A encomenda na JLC tem de dizer 2 camadas** — é mais barato, e pedir 4 para uma placa desenhada a 2 paga um plano que ninguém usa.

### Alvo: KiCad → JLCPCB (fab + SMT) · **duas placas** · atualizado 2026-09-01
> Substitui `carrier_board_revA_brief.md`, que ficou de 2026-08-20 e traz cinco
> decisões vencidas. A rev A **nunca foi fabricada**; isto não é respin, é a
> atualização do desenho antes do primeiro corte de cobre.

---

## 0 · O que mudou desde a rev A, e porquê

| item | rev A (2026-08-20) | rev B | razão |
|---|---|---|---|
| tensão do trilho | 12 V (CH224K Rset 24 k) | **15 V** | 15 V é perfil PD obrigatório; 12 V é opcional e muitos powerbanks não o dão |
| PD sink | CH224K no cobre | **módulo decoy PD 15 V/5 A** | módulo já comprado; traz o receptáculo com as linhas CC (pigtail de 2 fios deixa o chip surdo) |
| step-down | TPS563201 no cobre | **módulo 5–30 V → 5 V/5 A** | idem ⚠️ ver §7, saída selecionável |
| amp | TPA3116D2DADR no cobre, PBTL ×2 | **módulo TPA3116D2 estéreo, FORA da placa** | classe-D é o maior risco de layout numa primeira placa; o módulo tem terminais de parafuso e liga por fio |
| medição de corrente | shunt 10 mΩ + INA219 + I2C | **removido** | o powerbank virou consumível com display próprio; o propósito sumiu. D11/D12 voltam ao pool |
| ponte MIDI | header WIDI 6 fios | **ESP32-C3 + Schottky + 100 µF + 100 nF** | US$45 → US$4; o Schottky não é opcional (`HARDWARE.md` §1) |
| entrada de piezo | 1 kΩ + 2 diodos, pads de atenuador DNP | **pré-amplificador OPA1642** | a relação sinal/ruído decidiu-se por medição em 2026-09-01 |
| encoders | 1 EC11 | **1 EC11, e em D0/D1/D2** | ver §1: liberta os aliases A2/A3/A4 |
| botões | 4 (búzios, frz A, rearme CTOS, frz B) | **2 (frz A, frz B)** | ver §6, tem consequência de firmware |
| LEDs | direto no pino | **AO3400A + gate, do trilho de 5 V** | medido: 30–80 mA por LED, contra os 20 mA de limite do pino |
| placas | uma, painel por headers | **duas: cérebro + painel** | o painel aceso custa 6 dB de chão de ruído na entrada (`PINOS.md`) |

---

## 1 · Mapa de pinos da rev B

```
D0  encoder fase A        D13 MIDI TX → RX da ponte   ★ cruzado
D1  encoder fase B        D14 MIDI RX ← TX da ponte   ★ cruzado
D2  encoder push          D15 pot L  (A0)
D5  OLED RST              D16 pot R  (A1)
D6  OLED DC               D22 botão freeze B
D7  OLED CS               D23 gate do LED freeze B
D8  OLED CLK              D24 botão freeze A
D10 OLED DIN              D27 gate do LED freeze A
                          D28 STATUS da ponte (ativo BAIXO, aberto-dreno)
AUDIO IN  L ← pré-amp A   AUDIO OUT L → DPDT L
AUDIO IN  R ← pré-amp B   AUDIO OUT R → DPDT R
```

**Livres: D3, D4, D9, D11, D12, D17, D18, D19, D20, D21, D25, D26, D29, D30.**

Catorze pinos, e **três deles analógicos** — D17/D18/D19 são os aliases de
A2/A3/A4, e só voltaram ao pool porque o encoder foi para D0/D1/D2. O
`PINOS.md` declarava "não há um terceiro pot possível"; com um encoder só,
**há três**. Isto é o principal ganho de não fechar funcionalidade em cobre.

**D29/D30 deixam de estar reservados.** O sense dos DPDT exigia um terceiro
polo, e a chave é de dois — os dois estão gastos no `amp | ambos | jack`. Com
isso o RST do OLED em D5 deixa de ter o conflito que motivou a escolha; a
escolha mantém-se na mesma, porque já está soldada e provada.

---

## 2 · Bloco de energia

```
[módulo decoy PD 15 V] ── 15 V ── PTC 3 A ── TVS SMBJ ── SW(+V) ── RAIL15
                                                                    │
   ┌────────────────────────────────────────────────────────────────┤
   │                                                                │
 J_AMP (parafuso 2p, borda)                                [módulo step-down]
   15 V + GND → módulo TPA3116                                     │ 5 V
   ⚠️ 2200 µF/35 V + 100 nF NO TERMINAL                         RAIL5
      (medido: sem banco, o exciter zumbe)                          │
                                          ┌───────────────┬─────────┴────────┐
                                     Daisy VIN      1N5819 → ponte      J_PAINEL
                                                  (+100 µF +100 nF)     5 V LEDs
```

- **TVS e PTC não são opcionais**: entrada removível + powerbank + cabo é o
  ponto mais espetado do sistema.
- **Switch só no positivo.** Terra contínuo, em estrela.
- **O Schottky torna o erro impossível**, e é por isso que é peça e não regra:
  a regra "alimenta por USB **ou** pelo pino de 5 V" já estava escrita e foi
  violada por quem a escreveu.

## 3 · Entrada de piezo com pré-amplificador (×2)

```
JACK TRS ──R2 1k──●──[D1 BAV99 → V+ / AGND]──●──> U1 +in
 tip=sinal        │                          │
 sleeve+ring=AGND R1 4M7 → VBIAS          C2 1n → AGND

 U1 −in ──● RV1 100k trim ‖ C1 22p ── U1 out ──R4 100R──C4 2µ2──> AUDIO IN L/R
          └ R3 4k7 + C3 22µ NP → VBIAS

 V+  : RAIL15 ──R7 100R──●── C6 470µ ‖ C7 100n ──> ≈14,6 V
 VBIAS: R5/R6 100 k + C5 47 µ  ──> ≈7,3 V
```

- Ganho de 1 a 22 (0 a 27 dB) no trimpot. **É ajuste de exemplar** — mesma
  categoria do `gov_ceiling`, não entra em preset nem se copia entre unidades.
- **A encenação é o ponto, não o ganho**: preamp +20 dB com `pre_level_ab`
  a descer 20 dB deixa o ganho de laço onde está e sobe o sinal 20 dB acima do
  ruído captado. Ganho cedo, atenuação tarde.
- **Cerâmico X7R é piezoelétrico** — num corpo que vibra vira microfone. C0G
  nos pequenos, filme no acoplamento de saída. Nunca X7R onde passa sinal.
- **Cabo do piezo blindado, malha ao AGND só do lado da placa.** Nas duas
  pontas fecha laço, que é o defeito medido esta semana.
- **Preamp encostado aos jacks de entrada, dentro da ilha de AGND**, pista de
  entrada curta, conector do painel na borda oposta.

## 4 · Saída de linha e DPDT (×2)

```
AUDIO OUT L ──> DPDT L (ON-ON-ON tipo 2) ──┬─> J_AMP_IN L (parafuso, ao módulo)
                                           └─> JACK TRS L
                          10 kΩ permanente de cada entrada do amp ao AGND,
                          FORA da chave
```

- **A chave comuta NÍVEL DE LINHA, nunca a saída do amp.** A saída BTL do
  TPA3116 tem os dois terminais quentes.
- Posição central = ambos; os dois comuns amarrados.
- ⚠️ **Medir o ON-ON-ON antes de soldar.** O anúncio não diz tipo 1 ou 2, e o
  desenho depende do tipo 2 (comuns 2 e 5, centro = 2-3/5-4).

## 5 · Ponte ESP32-C3

```
D13 ──────────> GPIO5 (RX da ponte)      ★ CRUZADO
D14 <────────── GPIO4 (TX da ponte)      ★ CRUZADO
D28 <────────── GPIO6 (status, aberto-dreno, ATIVO EM BAIXO)
                pull-up interno da Daisy — SEM resistor externo
RAIL5 ──▶|───── 5V     1N5819, faixa (cátodo) para a ponte
        +100 µF +100 nF colados nos pinos
GND ─────────── GND, fio próprio à estrela
```

31250 8N1. **A ponte passa bytes e não interpreta nenhum** — é o que impede o
mapa de CC de ganhar segunda casa. Baud impresso pelo `board` e nos dois
READMEs; sem handshake de versão, por escolha (verificação automática e
fronteira fina são incompatíveis; a fina vale mais).

## 6 · Conector do painel — IDC 2×13 (26 vias)

| via | sinal | via | sinal |
|---|---|---|---|
| 1 | GND | 14 | GND (retorno LED) |
| 2 | **OLED CLK** (D8) | 15 | gate LED frz A (D27) |
| 3 | GND | 16 | gate LED frz B (D23) |
| 4 | OLED DIN (D10) | 17 | botão frz A (D24) |
| 5 | OLED CS (D7) | 18 | botão frz B (D22) |
| 6 | OLED DC (D6) | 19 | GND |
| 7 | OLED RST (D5) | 20 | encoder fase A (D0) |
| 8 | GND | 21 | encoder fase B (D1) |
| 9 | 3V3 | 22 | encoder push (D2) |
| 10 | GND | 23 | GND |
| 11 | 5 V (LED) | 24 | **pot L** (D15/A0) |
| 12 | 5 V (LED) | 25 | **pot R** (D16/A1) |
| 13 | GND (retorno LED) | 26 | GND |

**O relógio do OLED é guardado por terra dos dois lados** (vias 1 e 3), e o
par analógico dos pots está ensanduichado por terra (23 e 26). Não é
preciosismo: o painel aceso mediu **−75,3 → −69,3 dBFS** na entrada, com
espeto a 7040 Hz, e as cinco linhas correm ao lado dos cabos de piezo.
5 V dos LEDs em par próprio com retornos próprios — 200 mA se ambos acenderem.

**Os MOSFET ficam na placa do PAINEL, junto aos botões.** Assim os 200 mA de
comutação não atravessam o chicote; só o atravessam os dois gates, que são
microamperes.

**Consequência de firmware, a resolver antes do cobre:** com dois botões, os
**búzios** perdem gatilho manual (sobrevivem por transiente e por CC) e o
**rearme do CTOS** perde o caminho de recuperação sem computador — numa
galeria passa a ser corte de energia. D25/D26 ficam livres caso se queira
devolver, e o push do encoder é candidato.

## 7 · Item aberto que é risco, não detalhe

⚠️ **O módulo step-down tem saída selecionável: 3,3 / 5 / 9 / 12 / 15 / 24 V.**
Isso põe **24 V a um jumper de distância do VIN da Daisy**. Numa série de
dezenas, uma seleção que pode estar errada vai estar errada nalguma. Ou se
tranca a seleção (solda + verniz sobre os pads, registado no procedimento de
montagem), ou se passa a um módulo de 5 V fixo. **Módulo com regime
selecionável é módulo que não recusa fora do regime.**

Registado e não medido: é uma fonte comutada a alimentar a Daisy, e a caçada
de 2026-09-01 deixou por explicar uma série de 400 Hz e o relvado de 500 Hz–4 kHz.
Não acusar sem medir — mas está na lista.

---

## 8 · BOM — carrier (cérebro)

| bloco | peça | pacote | qtd | nota |
|---|---|---|---|---|
| pré-amp | **OPA1642AIDR** | SOIC-8 | 1 | JLC *extended* provável — confirmar stock, trava o pedido |
| pré-amp | BAV99 | SOT-23 | 2 | |
| pré-amp | trim 100 kΩ multivoltas 3296W | THT | 2 | ajuste de exemplar |
| pré-amp | 4M7 · 1k · 4k7 · 100R · 100k×2 | 0805 1% | 8 | |
| pré-amp | 22 pF C0G · 1 nF C0G | 0805 | 4 | **C0G obrigatório** |
| pré-amp | 22 µF não-polarizado | radial | 2 | |
| pré-amp | 2,2 µF **filme** | radial | 2 | não substituir por X7R |
| pré-amp | 47 µF · 470 µF 25 V | radial | 2 | |
| pré-amp | 100 nF X7R · 10 µF | 0805 | 4 | desacople, colados a U1 |
| pré-amp | 100 R ¼ W | 1206 | 1 | R7 do filtro de alimentação |
| energia | PTC ~3 A hold | 1812 | 1 | |
| energia | TVS SMBJ (24 V standoff) | SMB | 1 | |
| energia | 2200 µF / 35 V low-ESR | radial THT | 1 | **no terminal do amp** |
| energia | 100 nF X7R | 0805 | 1 | idem |
| ponte | **1N5819** | THT/SMA | 1 | já soldado na bancada |
| ponte | 100 µF · 100 nF | radial/0805 | 2 | colados aos pinos |
| DPDT | 10 kΩ | 0805 | 2 | entrada do amp ao AGND, fora da chave |
| conectores | soquete Daisy fêmea 2×20 2,54 | THT | 2 tiras | substituibilidade — a Daisy morta provou que vale |
| conectores | header decoy PD · step-down · ponte | 2,54 | 3 | |
| conectores | terminal parafuso 2p (15 V, linha L, linha R) | THT | 3 | |
| conectores | jack 1/4" TRS | painel + header 2p | 2 in + 2 out | |
| conectores | IDC 2×13 macho | THT | 1 | ao painel |
| chaves | DPDT ON-ON-ON **tipo 2** | painel | 2 | **medir antes de soldar** |

## 9 · BOM — painel

| peça | pacote | qtd | nota |
|---|---|---|---|
| OLED Waveshare 2,42" SSD1309 | MX1.25 7p | 1 | receptáculo na placa |
| EC11 5 pinos, eixo 20 mm | THT | 1 | |
| botão arcade 30 mm iluminado **AZUL** | painel | 2 | **a cor é spec, vai no silk** — a 3,3 V o amarelo não acende |
| **AO3400A** | SOT-23 | 2 | 5 A; margem contra os 100 mA por LED |
| resistor de gate 100 Ω · pull-down 10 kΩ | 0805 | 4 | pull-down obrigatório: GPIO flutua no boot |
| pot 10 kΩ linear | painel | 2 | + 100 nF do cursor ao terra, colado ao pot |
| IDC 2×13 fêmea | THT | 1 | |

---

## 10 · Fluxo (herdado da rev A, e vale)

1. BOM contra o catálogo LCSC/JLC, estoque conferido → aprovação do dB
2. Esquemático KiCad por bloco, **ERC limpo antes do seguinte**
3. PCB **2 camadas**, terra em `B.Cu` → DRC headless → **render para veto do dB**
4. ⚠️ **Revisão de terra (skill `revisao-terra`) antes de energizar**
5. Gerbers + CPL + BOM → **5 protótipos**, nunca quantidade
6. Rev C só depois de a rev B tocar

**Peça de memória é peça que não existe.** Os números LCSC puxam-se no dia do
pedido; aqui vão MPN, que é o que sobrevive a mudança de catálogo.
