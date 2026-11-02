# Varredura pré-fábrica — cérebro rev B + painel rev B
### 2026-09-08 · sobre os `.kicad_sch` e `.kicad_pcb` recebidos (KiCad 10.0) · leitura estrutural, não regex
> **Veredito: NÃO GERAR GERBERS.** Seis defeitos que tornam a placa não funcional
> ou inutilizável na caixa, todos no desenho — nenhum é de fabrico. Todos se
> corrigem no esquema/placa antes do pacote, e nenhum é grande. Mas cada um,
> sozinho, era uma placa que a Dendê levava para a Europa sem funcionar.

---

## 🔴 CRÍTICOS — a placa não funciona

### C1 · Pré-amp: sinal na entrada INVERSORA, realimentação na NÃO-inversora (ambos os canais)
**Evidência (esquema, trace com junções):** `U1` unidade A —
- pino **2 (−IN)**: `R12` (1k, entrada), `R11` (4M7, VBIAS), `C12` (22n), `D11.3` (clamp) → **a rede de entrada**
- pino **3 (+IN)**: `R10` (0Ω → saída), `RV1`, `R13`, `C11` (DNP) → **a rede de realimentação**
- na placa: `U1` pad 2 = `Net-(U1A--)`, pad 3 = `Net-(U1A-+)` ← `R10` ← pad 1 (OUT). Canal B igual (pads 5/6).

**Consequência:** saída ligada ao +IN e sinal no −IN é **realimentação positiva** — o OPA1642 trava numa das alimentações. Não amplifica, não segue, não passa som. Os dois canais.

**Causa provável:** o símbolo `cotuca:OPA1642` desenha `−` em baixo e `+` em cima (o oposto da biblioteca oficial). Quando o símbolo genérico de 3 pinos foi trocado pelo de SOIC-8 sem mexer nos fios, as duas redes trocaram de pino. A verificação "netlist idêntica" contou ligações, não funções.

**Conserto:** trocar as duas redes de pino em U1A e U1B — a rede de entrada (R12/R11/C12/D11) vai ao **pino 3 (+)**, a de realimentação (R10/RV1/R13/C11) ao **pino 2 (−)**. Na placa, os pads 2 e 3 são vizinhos: duas pistas trocam. **Não** renumerar o símbolo — pad 2 do chip real é −IN.

### C2 · MOSFETs do painel: dreno e fonte trocados
**Evidência:** símbolo `cotuca:MOSFET_N` = **1 G, 2 D, 3 S**. Folha de dados AO3400A (SOT-23, top view, lida): **1 G, 2 S, 3 D**. Na placa: `Q1` pad 2 = `LED_K_A` (cátodo do LED), pad 3 = `GND`.
**Consequência:** o díodo de corpo (fonte→dreno) fica entre o cátodo do LED e o GND: **os dois LEDs acendem permanentemente**, independentes do gate. O freeze A/B perde a luz de estado.
**Conserto:** trocar os pinos 2 e 3 no símbolo `MOSFET_N` (2 = S, 3 = D) **ou** trocar as duas redes nos pads. Confirmar com a ficha, que já está no repo.

### C3 · D2 (Schottky da ponte ESP32) ao contrário
**Evidência:** footprint `Diode_SMD:D_SMA` — pad 1 = **cátodo** (convenção KiCad, a faixa do silk está no pad 1). Na placa `D2` pad 1 = `+5V`, pad 2 = `+5V_PONTE`. Intenção documentada na folha de energia: *"faixa = cátodo, virada para a ponte"*.
**Consequência:** o trilho +5V não alimenta a ponte (bloqueado), e o USB do ESP32 **realimenta o trilho** — o defeito exato que o díodo existia para impedir, e que a bancada já pagou. `D1` (TVS) está certo: pad 1 (K) = +15V.
**Conserto:** trocar as redes dos pads 1↔2 de D2 (ou rodar 180° e re-rotear). Símbolo `cotuca:D_H` é genérico (pinos sem A/K) — dar-lhe nomes A/K para isto não voltar a acontecer.

### C4 · Encoder SW1: NENHUM pad ligado
**Evidência:** símbolo `cotuca:EC11` pinos **1,2,3,4,5**; footprint `RotaryEncoder_Alps_EC11E-Switch` pads **A, B, C, S1, S2, MP**. Na placa, os 5 pads de sinal têm `net = None`. `ENC_A_A/B/PUSH` chegam pelo IDC (vias 20–22) e param.
**Consequência:** o encoder não faz nada. Os únicos menus do instrumento ficam sem entrada.
**Conserto:** renumerar os pinos do símbolo para os nomes dos pads: `A`, `C` (comum), `B`, `S1`, `S2` — o footprint Alps usa letras. Re-importar e rotear (5 pistas curtas). O "0 por rotear" não apanhou porque sem pad não há ligação a fazer.

### C5 · Sinal para o amp nunca chega ao conector do amp
**Evidência:** rede `EXC_L` = só `J15.3` e `J15.6` (terminais "exciter" da chave DPDT). Rede `AMP_IN_L` = `J11.3` + `R50` (10k ao GND). **Duas redes, nenhum fio entre elas.** Idem R.
**Consequência:** a posição "amp" da chave não vai a lado nenhum; a entrada do módulo (J11.3/4) fica só com o 10k ao GND. Nenhum som nos exciters via placa.
**Conserto:** `EXC_L` e `AMP_IN_L` são a mesma rede — renomear uma (sugestão: tudo `AMP_IN_L/R`) na folha do soquete, re-importar, rotear.

### C6 · Cérebro sem furos de fixação
**Evidência:** `Edge.Cuts` é só o retângulo arredondado; zero footprints `MountingHole`, zero `np_thru_hole`. A FILA do briefing de 2026-09-06 pedia 4× M3 em (15,5 · 128,5) × (3,5 · 96,5) — não entrou.
**Consequência:** a placa não se fixa ao fundo da caixa; os furos ø3,2 do acrílico não têm par.
**Conserto:** 4× `MountingHole:MountingHole_3.2mm_M3` nas coordenadas do `pcb_contorno.py`, com anel de exclusão de cobre. Re-correr a paridade contra a caixa.

---

## 🟠 ALTOS — confirmar antes de encomendar

### A1 · Ordem dos pinos do OLED (J2) contra o cabo da Waveshare
Placa: **1 GND · 2 +5V · 3 CLK · 4 DIN · 5 CS · 6 DC · 7 RST**. O módulo Waveshare 2,42" SPI tem, na ordem do conector: **VCC · GND · DIN · CLK · CS · DC · RST**. Com um cabo 1:1, **VCC e GND trocam** (OLED morre ao ligar) e DIN/CLK trocam. Ou se reordena J2 para a ordem do módulo (recomendado — o cabo de fábrica passa a servir), ou se crimpa um cabo cruzado e se escreve isso no LEIA-ME. Confirmar a ordem no módulo em mão antes de decidir.

### A2 · Pots: MPN é o estilo horizontal
`RV1`/`RV2` têm footprint `PTV09A-1` (vertical, bucim) e **MPN `PTV09A-4020S-B103`** (`-4` = rear mount, sem bucim, eixo deitado). Comprado assim, o eixo não sobe pela tampa. MPN correto: **`PTV09A-1020S-B103`** (Mouser/DigiKey — é THT, não passa pela JLC).

### A3 · A placa é de 2 camadas; a documentação diz 4
Cobre: só `F.Cu` e `B.Cu`. Zonas `GND` (prio 0) e `AGND` (prio 1) ambas em `B.Cu`, separadas, `NT1` como única ponte — **isto está correto e verificado**. Mas o briefing, o roteiro e a revisão de terra falam em "4 camadas, L2 terra sólido". Decidir e alinhar: a encomenda na JLC tem de dizer 2 camadas (mais barato), e os documentos têm de deixar de dizer 4. Sem plano interior, o retorno do amp é a zona de B.Cu — aceitável para 3 A com a largura que tem.

---

## 🟡 MÉDIOS — arrumar, não travam

- `D2` Value `1N5819` / MPN `SS14`; `F1` Value `PTC 3A` / MPN `2920L330` (3,3 A). Value e MPN contam histórias diferentes.
- `RV1.3`/`RV2.3` (trimpots DNP) sem rede — pôr bandeira "no connect".
- Pistas de **AGND a 0,187 mm** entre pads do pré-amp: passa na JLC (mín. 0,127), mas é fino para retorno; 0,3 custa nada.
- Códigos LCSC **não estão nos ficheiros-fonte** que recebi (nem em símbolos nem na placa) — vivem no CSV gerado. Conferir o CSV contra a placa antes de subir.
- Painel sem furos de fixação: pendura pelos bucins dos pots e do encoder. Decisão consciente? Com IDC + 5 JST a puxar, aguenta, mas fica dito.

---

## ✅ Verificado e certo

| o quê | como |
|---|---|
| Contrato IDC 26/26 | pad→rede de `J14` (cérebro) vs `J1` (painel), via a via: 0 divergências |
| Pinagem da Daisy | 40 pads vs `Seed3_pinout.csv`: AGND 20, +3V3A 21, VIN 39, GND 40, áudio 16–19, D29/D30 livres, D26 = `BOT_BUZIOS` |
| Ponte única de terra | com `NT1` excluído, `GND` e `AGND` não partilham cobre em lado nenhum |
| Roteamento | 0 redes com mais de uma ilha nas duas placas (o `J12.3` liga por pista que o atravessa) |
| Polaridades certas | `D1` TVS (K→+15V) · `D11/D21` BAV99 (1→AGND, 2→V+PRE, 3→sinal: clamp correto) · eletrolíticos pad 1 = + em todos |
| Potência | +15V a 1,0 mm · +5V/3V3 0,5 · vias ø0,6/0,3 · nada a < 0,3 mm do bordo · furo mínimo 0,5 (PicoBlade) |
| Esquema ↔ placa | 70/70 e 28/28 referências, valores e DNP iguais |
| Soquete ESP32 | ordem dos 2×8 pads bate com a SuperMini (5V·GND·3V3·4·3·2·1·0 / 5·6·7·8·9·10·20·21) |
| Desacople | +5V (C15), +3V3A (C16), +5V_PONTE (C3+C4), V+PRE (C6+C7+C8+C9), painel +5V (C3+C4), +3V3A (C5) |

---

## O padrão por baixo dos seis

Cinco dos seis críticos (C1–C5) são **o mesmo defeito**: símbolo genérico da biblioteca `cotuca` — pinos `1,2,3` sem nome de função — casado com um footprint real cujos pads têm função fixa (SOIC-8, SOT-23, D_SMA, EC11). A netlist do KiCad fica "correta" — cada pino liga a um pad — e a função fica errada. Nenhum ERC, DRC ou "0 por rotear" o vê, porque todos verificam **ligação**, não **significado**.

A verificação que apanha isto é uma tabela por peça polarizada: **pad → rede → função esperada pela ficha**. Foi a que fiz aqui à mão para U1, Q, D e SW1. Vale escrevê-la como passada do portão (`kicad` §7): para cada footprint com pads nomeados ou polarizados, o gerador declara a função de cada pad e o verificador compara com a rede.

**Ordem de conserto:** C1 → C2 → C3 → C5 → C4 → C6 (todos no esquema/símbolos, depois um "Update PCB", re-rotear o pouco que muda, DRC, e **repetir esta varredura**), depois A1/A2 na BOM e LEIA-ME, A3 na encomenda. Só então Gerbers.
