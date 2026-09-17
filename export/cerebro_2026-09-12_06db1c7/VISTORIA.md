# Questionário do Portão Humano ⟨dB⟩ — cerebro

> Gerado por `portao_humano.py` v0.2.0 a partir de `cpl_rico_cerebro.csv` + `bom_cerebro.csv`. Método: `references/portao-humano.md`.
>
> **REGRA DO PORTÃO:** um item de cada vez. O portão só avança com **✅ conforme**. Qualquer **❌** ou ambiguidade **PÁRA** e encaminha para um conserto — **nunca** para o pagamento. Um passe limpo de ponta a ponta é a precondição para o humano autorizar o pagamento (o pagamento é sempre acção do humano).

## 0. Carregar o pacote (antes de tudo)

### Item 0 — Carregar o pacote — buscar ao PR e carregar na JLCONE
- **ONDE OLHAR:** O pacote está **committado no repo** em `export/cerebro_2026-09-12_06db1c7/` na branch do PR (`claude/slack-session-sm2ldz`, repo `Belquer/Cotucatronics` no GitHub) — está no **GitHub, não necessariamente na tua máquina**. ⚠️ O terminal que vês na pipeline corre no **sandbox na nuvem** e **não consegue pôr ficheiros na tua máquina** — a recuperação é um comando que **TU corres localmente**. Depois carregas no site da **JLCPCB/JLCONE** (*Add Gerber*). Antes de QUALQUER item de inspeção, a placa tem de estar carregada.
- **O QUE DEVES VER:** **Gerber a carregar:** `export/cerebro_2026-09-12_06db1c7/cerebro_2026-09-12_06db1c7_gerbers.zip` — é ESTE `.zip` que entra na JLCONE, não o `.kicad_pcb` nem gerbers soltos.
  · **Como buscar (terminal no teu Mac — o preferido):** `cd Cotucatronics && git fetch origin && git checkout claude/slack-session-sm2ldz && git pull origin claude/slack-session-sm2ldz && open export/cerebro_2026-09-12_06db1c7/` — se ainda não o clonaste: `git clone -b claude/slack-session-sm2ldz git@github.com:Belquer/Cotucatronics.git`.
  · **Fallback (GitHub no browser):** navega `claude/slack-session-sm2ldz` até `export/cerebro_2026-09-12_06db1c7/`, abre a página do `cerebro_2026-09-12_06db1c7_gerbers.zip` e clica **Download raw file** (repo privado → tens de estar autenticado como o dono); leva também o `cpl_cerebro.csv` + `bom_cerebro.csv`. Ou **Code → Download ZIP** da branch e tira a pasta `export/`.
  · **Montagem SMT:** quando a JLC pedir, junta `cpl_cerebro.csv` (posições) + `bom_cerebro.csv` (BOM), da MESMA pasta.
  · **Camadas:** 2.
  · Depois, arrasta o `cerebro_2026-09-12_06db1c7_gerbers.zip` para a JLCONE (*Add Gerber*).
- **COMO CONFIRMAR:** **Proveniência** — PASS: carregaste o `cerebro_2026-09-12_06db1c7_gerbers.zip` (buscado a `claude/slack-session-sm2ldz` no GitHub) e o `LEIA-ME.txt` da MESMA pasta carimba o commit `06db1c7` e árvore limpa — é ESSA a revisão que estás a encomendar. FAIL: não encontras o pacote na tua máquina (está só no PR — vai buscá-lo ao GitHub), ou carregaste outro zip, ou o `LEIA-ME.txt` carimba outro commit / árvore suja → PÁRA e vai buscar o pacote certo do PR, NÃO encomendas (guarda contra encomendar um pacote velho ou trocado).
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 1. Nível-placa

### Item 1 — Nível-placa — número de camadas
- **ONDE OLHAR:** O gerber (ou o LEIA-ME do pacote) e o formulário da JLC.
- **O QUE DEVES VER:** A placa é de **N camadas (confirmar)**. O formulário da JLC tem de dizer o MESMO número de camadas.
- **COMO CONFIRMAR:** PASS: nº de camadas no formulário = nº real do desenho. FAIL: pedir 4 quando o desenho só usa F.Cu+B.Cu paga um plano interior a mais → corrigir o formulário, não pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 2 — Nível-placa — dimensões e contorno vs caixa
- **ONDE OLHAR:** A pré-visualização 2D do contorno (Edge.Cuts) e o modelo da caixa.
- **O QUE DEVES VER:** O contorno bate na abertura da caixa; dimensões conforme o desenho. Os furos de fixação coincidem.
- **COMO CONFIRMAR:** PASS: contorno e furos batem na caixa (dry-fit / render). FAIL: contorno fora do encaixe ou furo sob componente → voltar ao Estágio 3, não pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 3 — Nível-placa — quantidade a encomendar
- **ONDE OLHAR:** O carrinho da JLC.
- **O QUE DEVES VER:** A quantidade pedida = a que a ordem precisa (confirmar contra a ordem).
- **COMO CONFIRMAR:** PASS: quantidade certa no carrinho. FAIL: quantidade errada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 2. Peças polarizadas (banda/cátodo/pino-1 vs serigrafia)

### Item 4 — C1 — 2200u/35V (CP_Radial_D16.0mm_P7.50mm)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre C1 em X=178, Y=-108 (rotação CPL = 0°, lado top) — comparar 3D vs serigrafia (silk)
- **O QUE DEVES VER:** A faixa − (electrolítico) do modelo 3D fica do lado que a serigrafia marca. Confirmar a perna longa/curta vs o pad quadrado.
- **COMO CONFIRMAR:** PASS: polaridade do 3D = marca do silk. FAIL: + e − trocados → o condensador rebenta ao ligar → NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 5 — C3 — 100u (CP_Radial_D6.3mm_P2.50mm)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre C3 em X=166, Y=-73 (rotação CPL = 0°, lado top) — comparar 3D vs serigrafia (silk)
- **O QUE DEVES VER:** A faixa − (electrolítico) do modelo 3D fica do lado que a serigrafia marca. Confirmar a perna longa/curta vs o pad quadrado.
- **COMO CONFIRMAR:** PASS: polaridade do 3D = marca do silk. FAIL: + e − trocados → o condensador rebenta ao ligar → NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 6 — C5 — 47u (CP_Radial_D6.3mm_P2.50mm)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre C5 em X=69.5, Y=-103 (rotação CPL = 0°, lado top) — comparar 3D vs serigrafia (silk)
- **O QUE DEVES VER:** A faixa − (electrolítico) do modelo 3D fica do lado que a serigrafia marca. Confirmar a perna longa/curta vs o pad quadrado.
- **COMO CONFIRMAR:** PASS: polaridade do 3D = marca do silk. FAIL: + e − trocados → o condensador rebenta ao ligar → NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 7 — C6 — 470u 25V (CP_Radial_D10.0mm_P5.00mm)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre C6 em X=55, Y=-114 (rotação CPL = 0°, lado top) — comparar 3D vs serigrafia (silk)
- **O QUE DEVES VER:** A faixa − (electrolítico) do modelo 3D fica do lado que a serigrafia marca. Confirmar a perna longa/curta vs o pad quadrado.
- **COMO CONFIRMAR:** PASS: polaridade do 3D = marca do silk. FAIL: + e − trocados → o condensador rebenta ao ligar → NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 8 — D1 — SMBJ24A (D_SMB)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre D1 em X=166, Y=-34 (rotação CPL = 0°, lado top) — comparar 3D vs serigrafia (silk)
- **O QUE DEVES VER:** A barra/anel do CÁTODO no modelo 3D coincide com a faixa impressa na serigrafia (o lado do K). ⚠️ Pacote D_SMB tem risco de 180°.
- **COMO CONFIRMAR:** PASS: cátodo do 3D = faixa do silk. FAIL: invertido → o díodo conduz ao contrário → corrigir a rotação, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 9 — D2 — SS14 (D_SMA)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre D2 em X=180, Y=-34 (rotação CPL = 0°, lado top) — comparar 3D vs serigrafia (silk)
- **O QUE DEVES VER:** A barra/anel do CÁTODO no modelo 3D coincide com a faixa impressa na serigrafia (o lado do K). ⚠️ Pacote D_SMA tem risco de 180°. ⚠️ NOME: `SS14` e `1N5819` são a MESMA peça (Schottky SMA) com designações diferentes — confirmar que a serigrafia (Value) e a compra (MPN/LCSC C2480) falam da mesma peça.
- **COMO CONFIRMAR:** PASS: cátodo do 3D = faixa do silk. FAIL: invertido → o díodo conduz ao contrário → corrigir a rotação, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 10 — D11 — BAV99 (SOT-23)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre D11 em X=40.5, Y=-36 (rotação CPL = 0°, lado top) — comparar 3D vs serigrafia (silk)
- **O QUE DEVES VER:** A disposição de pinos e a marca de pino-1 do modelo 3D coincidem com a serigrafia (o díodo duplo BAV99-e-afins tem 1 pino de um lado, 2 do outro). ⚠️ A biblioteca da JLC roda SOT-23 180° com frequência — este é o perigo.
- **COMO CONFIRMAR:** PASS: pino-1 do 3D = pino-1 do silk. FAIL: rodado 180° (1 pino trocou de lado) → corrigir a rotação no CPL/footprint, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 11 — D21 — BAV99 (SOT-23)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre D21 em X=40.5, Y=-64 (rotação CPL = 0°, lado top) — comparar 3D vs serigrafia (silk)
- **O QUE DEVES VER:** A disposição de pinos e a marca de pino-1 do modelo 3D coincidem com a serigrafia (o díodo duplo BAV99-e-afins tem 1 pino de um lado, 2 do outro). ⚠️ A biblioteca da JLC roda SOT-23 180° com frequência — este é o perigo.
- **COMO CONFIRMAR:** PASS: pino-1 do 3D = pino-1 do silk. FAIL: rodado 180° (1 pino trocou de lado) → corrigir a rotação no CPL/footprint, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 12 — U1 — OPA1642 (SOIC-8_3.9x4.9mm_P1.27mm)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre U1 em X=26, Y=-92 (rotação CPL = 0°, lado top) — comparar 3D vs serigrafia (silk)
- **O QUE DEVES VER:** O ponto/chanfro do PINO 1 no modelo 3D fica no canto que a serigrafia marca (o ponto no canto).
- **COMO CONFIRMAR:** PASS: pino-1 do 3D = pino-1 do silk. FAIL: rodado 90°/180° → pinout todo trocado → corrigir a rotação, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 3. Pacotes com risco de rotação 180° (orientação vs pino-1)

### Item 13 — U2 — Daisy Seed3 (Electrosmith_Daisy_Seed)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre U2 em X=113.24, Y=-109.06 (rotação CPL = 180°, lado top) — confirmar orientação vs pino-1 pretendido
- **O QUE DEVES VER:** A orientação do módulo (chanfro / pino-1 / borda do USB) no 3D bate na serigrafia. Rotação CPL = 180°.
- **COMO CONFIRMAR:** PASS: pino-1/borda do 3D = silk. FAIL: rodado 180° → alimentação e sinais trocam de fila → corrigir, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 14 — U3 — ESP32-C3 SuperMini (ESP32C3_SUPERMINI_SOQUETE)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre U3 em X=85, Y=-106 (rotação CPL = 180°, lado top) — confirmar orientação vs pino-1 pretendido
- **O QUE DEVES VER:** A orientação do módulo (chanfro / pino-1 / borda do USB) no 3D bate na serigrafia. Rotação CPL = 180°.
- **COMO CONFIRMAR:** PASS: pino-1/borda do 3D = silk. FAIL: rodado 180° → alimentação e sinais trocam de fila → corrigir, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 4. Conectores / contratos de pinagem

### Item 15 — J1 — entrada decoy PD (TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J1 em X=156, Y=-108 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A polaridade da alimentação: qual pino é + e qual é GND/−, batendo na serigrafia e no que o cabo entrega.
- **COMO CONFIRMAR:** PASS: + e GND na ordem certa. FAIL: polaridade trocada → corrigir a serigrafia/cablagem, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 16 — J2 — ao amp TPA3116 (TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J2 em X=152, Y=-73 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A polaridade da alimentação: qual pino é + e qual é GND/−, batendo na serigrafia e no que o cabo entrega.
- **COMO CONFIRMAR:** PASS: + e GND na ordem certa. FAIL: polaridade trocada → corrigir a serigrafia/cablagem, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 17 — J3 — step-down 5V fixo (PinHeader_1x03_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J3 em X=166, Y=-60 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A polaridade da alimentação: qual pino é + e qual é GND/−, batendo na serigrafia e no que o cabo entrega.
- **COMO CONFIRMAR:** PASS: + e GND na ordem certa. FAIL: polaridade trocada → corrigir a serigrafia/cablagem, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 18 — J4 — a ponte ESP32 (PinHeader_1x02_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J4 em X=180, Y=-73 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J4 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 19 — J5 — fio -> jack piezo J5 (PinHeader_1x03_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J5 em X=69.5, Y=-36 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J5 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 20 — J6 — fio -> jack piezo J6 (PinHeader_1x03_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J6 em X=26, Y=-64 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J6 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 21 — J11 — modulo TPA3116 (PinHeader_1x04_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J11 em X=134, Y=-56 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J11 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 22 — J12 — fio -> TRS linha L (PinHeader_1x03_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J12 em X=134, Y=-78 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J12 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 23 — J13 — fio -> TRS linha R (PinHeader_1x03_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J13 em X=118, Y=-100 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J13 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 24 — J14 — IDC 2x13 -> painel (IDC-Header_2x13_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J14 em X=45, Y=-29 (rotação CPL = 90°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** O contrato placa-a-placa: os dois lados do IDC (ex.: 2x13 / 26 vias) batem pino-a-pino entre as duas placas, com o pino-1 alinhado (chanfro do shroud do mesmo lado nas duas).
- **COMO CONFIRMAR:** PASS: 26/26 pino-a-pino, pino-1 alinhado. FAIL: fila invertida ou pino-1 no lado errado → corrigir o footprint/orientação, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 25 — J15 — fio -> DPDT tipo 2, canal L (PinHeader_1x08_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J15 em X=134, Y=-34 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J15 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 26 — J16 — fio -> DPDT tipo 2, canal R (PinHeader_1x08_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J16 em X=118, Y=-78 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J16 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 5. BOM / montagem

### Item 27 — BOM — toda linha SMT com LCSC em stock de MONTAGEM
- **ONDE OLHAR:** O BOM (coluna LCSC) e a página da peça na JLC (Assembly / stock).
- **O QUE DEVES VER:** Todas as 30 linhas SMT levam C-number e stock de MONTAGEM>0. Sem LCSC: nenhuma.
- **COMO CONFIRMAR:** PASS: cada SMT com LCSC e stock de montagem. FAIL: linha SMT sem LCSC ou sem stock de montagem → resolver a fonte (⚠️ stock na LCSC ≠ stock de montagem), NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 28 — BOM — THT enviados à parte (não vão pela JLC)
- **ONDE OLHAR:** A lista de peças THT do desenho.
- **O QUE DEVES VER:** As peças THT são montadas por ti, à parte: C1 (2200u/35V), C3 (100u), C5 (47u), C6 (470u 25V), C14 (2u2 filme), C24 (2u2 filme), J1 (entrada decoy PD), J2 (ao amp TPA3116), J3 (step-down 5V fixo), J4 (a ponte ESP32), J5 (fio -> jack piezo J5), J6 (fio -> jack piezo J6), J11 (modulo TPA3116), J12 (fio -> TRS linha L), J13 (fio -> TRS linha R), J14 (IDC 2x13 -> painel), J15 (fio -> DPDT tipo 2, canal L), J16 (fio -> DPDT tipo 2, canal R), U2 (Daisy Seed3), U3 (ESP32-C3 SuperMini).
- **COMO CONFIRMAR:** PASS: THT marcadas como 'cliente fornece / à parte'. FAIL: THT metida na montagem SMT → corrigir o BOM/CPL.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 29 — BOM — DNP correcto
- **ONDE OLHAR:** As linhas marcadas DNP (do not place) no BOM/CPL.
- **O QUE DEVES VER:** As peças DNP estão fora da montagem; as de montar estão dentro.
- **COMO CONFIRMAR:** PASS: DNP coincide com a intenção. FAIL: peça a montar marcada DNP (ou vice-versa) → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 6. Mecânica (alcance dos recortes do painel)

### Item 30 — Mecânica — conectores/botões/display alcançam os recortes
- **ONDE OLHAR:** O render da caixa e/ou o dry-fit de UMA caixa antes da série.
- **O QUE DEVES VER:** Cada peça que atravessa o painel alcança o recorte certo: J1 (entrada decoy PD); J2 (ao amp TPA3116); J5 (fio -> jack piezo J5); J6 (fio -> jack piezo J6); J14 (IDC 2x13 -> painel). Folga do cabo e alcance com a placa montada.
- **COMO CONFIRMAR:** PASS: dry-fit/render mostra cada uma no seu recorte. FAIL: encoder/pot/display/botão não alcança ou colide → voltar ao Estágio 3, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 7. Confirmação final antes de pagar

### Item 31 — Passe limpo — precondição do pagamento
- **ONDE OLHAR:** Este questionário, de cima a baixo.
- **O QUE DEVES VER:** TODOS os itens acima com ✅. Zero ❌, zero campo em branco, zero ambiguidade.
- **COMO CONFIRMAR:** PASS (todos ✅): o humano fica AUTORIZADO a pagar — o pagamento é acção do humano. FAIL (qualquer ❌/dúvida): PÁRA, encaminha para o conserto, e NÃO se paga.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

---
_32 itens (o Item 0 é o carregamento do pacote, antes da inspeção). Enquanto houver um ❌ ou um campo em branco, o portão está FECHADO e o pagamento não é autorizado._
