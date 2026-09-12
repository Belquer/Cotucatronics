# Questionário do Portão Humano ⟨dB⟩ — cotuca painel

> Gerado por `portao_humano.py` v0.2.0 a partir de `cpl_rico_painel.csv` + `bom_painel.csv`. Método: `references/portao-humano.md`.
>
> **REGRA DO PORTÃO:** um item de cada vez. O portão só avança com **✅ conforme**. Qualquer **❌** ou ambiguidade **PÁRA** e encaminha para um conserto — **nunca** para o pagamento. Um passe limpo de ponta a ponta é a precondição para o humano autorizar o pagamento (o pagamento é sempre acção do humano).

## 0. Carregar o pacote (antes de tudo)

### Item 0 — Carregar o pacote — buscar ao PR e carregar na JLCONE
- **ONDE OLHAR:** O pacote está **committado no repo** em `export/painel_2026-09-12_deddd3d/` na branch do PR (`claude/slack-session-sm2ldz`, repo `Belquer/Cotucatronics` no GitHub) — está no **GitHub, não necessariamente na tua máquina**. ⚠️ O terminal que vês na pipeline corre no **sandbox na nuvem** e **não consegue pôr ficheiros na tua máquina** — a recuperação é um comando que **TU corres localmente**. Depois carregas no site da **JLCPCB/JLCONE** (*Add Gerber*). Antes de QUALQUER item de inspeção, a placa tem de estar carregada.
- **O QUE DEVES VER:** **Gerber a carregar:** `export/painel_2026-09-12_deddd3d/painel_2026-09-12_deddd3d_gerbers.zip` — é ESTE `.zip` que entra na JLCONE, não o `.kicad_pcb` nem gerbers soltos.
  · **Como buscar (terminal no teu Mac — o preferido):** `cd Cotucatronics && git fetch origin && git checkout claude/slack-session-sm2ldz && git pull origin claude/slack-session-sm2ldz && open export/painel_2026-09-12_deddd3d/` — se ainda não o clonaste: `git clone -b claude/slack-session-sm2ldz git@github.com:Belquer/Cotucatronics.git`.
  · **Fallback (GitHub no browser):** navega `claude/slack-session-sm2ldz` até `export/painel_2026-09-12_deddd3d/`, abre a página do `painel_2026-09-12_deddd3d_gerbers.zip` e clica **Download raw file** (repo privado → tens de estar autenticado como o dono); leva também o `cpl_painel.csv` + `bom_painel.csv`. Ou **Code → Download ZIP** da branch e tira a pasta `export/`.
  · **Montagem SMT:** quando a JLC pedir, junta `cpl_painel.csv` (posições) + `bom_painel.csv` (BOM), da MESMA pasta.
  · **Camadas:** 2.
  · Depois, arrasta o `painel_2026-09-12_deddd3d_gerbers.zip` para a JLCONE (*Add Gerber*).
- **COMO CONFIRMAR:** **Proveniência** — PASS: carregaste o `painel_2026-09-12_deddd3d_gerbers.zip` (buscado a `claude/slack-session-sm2ldz` no GitHub) e o `LEIA-ME.txt` da MESMA pasta carimba o commit `deddd3d` e árvore limpa — é ESSA a revisão que estás a encomendar. FAIL: não encontras o pacote na tua máquina (está só no PR — vai buscá-lo ao GitHub), ou carregaste outro zip, ou o `LEIA-ME.txt` carimba outro commit / árvore suja → PÁRA e vai buscar o pacote certo do PR, NÃO encomendas (guarda contra encomendar um pacote velho ou trocado).
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 1. Nível-placa

### Item 1 — Nível-placa — número de camadas
- **ONDE OLHAR:** O gerber (ou o LEIA-ME do pacote) e o formulário da JLC.
- **O QUE DEVES VER:** A placa é de **2 camadas**. O formulário da JLC tem de dizer o MESMO número de camadas.
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

### Item 4 — C3 — 100u 16V (CP_Radial_D6.3mm_P2.50mm)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre C3 em X=52, Y=-52 (rotação CPL = 0°, lado top) — comparar 3D vs serigrafia (silk)
- **O QUE DEVES VER:** A faixa − (electrolítico) do modelo 3D fica do lado que a serigrafia marca. Confirmar a perna longa/curta vs o pad quadrado.
- **COMO CONFIRMAR:** PASS: polaridade do 3D = marca do silk. FAIL: + e − trocados → o condensador rebenta ao ligar → NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 3. Pacotes com risco de rotação 180° (orientação vs pino-1)

### Item 5 — Q1 — AO3400A (SOT-23)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre Q1 em X=52, Y=-46 (rotação CPL = 0°, lado top) — confirmar orientação vs pino-1 pretendido
- **O QUE DEVES VER:** O pino-1 (chanfro/ponto) do 3D bate na serigrafia. ⚠️ SOT-23 é dos pacotes que a JLC roda 180° — este é o perigo dos 180° da biblioteca JLC vs KiCad.
- **COMO CONFIRMAR:** PASS: pino-1 do 3D = pino-1 do silk. FAIL: rodado 180° → pinout trocado (dreno/fonte, entrada/saída) → corrigir a rotação no CPL/footprint, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 6 — Q2 — AO3400A (SOT-23)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre Q2 em X=64, Y=-46 (rotação CPL = 0°, lado top) — confirmar orientação vs pino-1 pretendido
- **O QUE DEVES VER:** O pino-1 (chanfro/ponto) do 3D bate na serigrafia. ⚠️ SOT-23 é dos pacotes que a JLC roda 180° — este é o perigo dos 180° da biblioteca JLC vs KiCad.
- **COMO CONFIRMAR:** PASS: pino-1 do 3D = pino-1 do silk. FAIL: rodado 180° → pinout trocado (dreno/fonte, entrada/saída) → corrigir a rotação no CPL/footprint, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 4. Conectores / contratos de pinagem

### Item 7 — J1 — IDC 2x13 femea <- cerebro (IDC-Header_2x13_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J1 em X=26, Y=-29.5 (rotação CPL = 90°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** O contrato placa-a-placa: os dois lados do IDC (ex.: 2x13 / 26 vias) batem pino-a-pino entre as duas placas, com o pino-1 alinhado (chanfro do shroud do mesmo lado nas duas).
- **COMO CONFIRMAR:** PASS: 26/26 pino-a-pino, pino-1 alinhado. FAIL: fila invertida ou pino-1 no lado errado → corrigir o footprint/orientação, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 8 — J2 — OLED SSD1309 2,54mm 7p (PinHeader_1x07_P2.54mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J2 em X=68, Y=-24 (rotação CPL = 90°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** O contrato de pinagem placa↔módulo, pino a pino. ⚠️ Perigo clássico do OLED SPI: com um cabo 1:1 o **VCC e o GND trocam** (o módulo morre ao ligar) e o **DIN/CLK também trocam**. Confirmar a ordem de J2 contra o conector do módulo REAL em mão (ex.: Waveshare 2,42": VCC·GND·DIN·CLK·CS·DC·RST).
- **COMO CONFIRMAR:** PASS: cada pino do J bate no pino homólogo do módulo (ou o cabo cruzado corrige). FAIL: VCC/GND ou DIN/CLK trocados → reordenar o J ou crimpar cabo cruzado ANTES de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 9 — J3 — fio -> arcade AZUL freeze A (JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J3 em X=26, Y=-36.5 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J3 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 10 — J4 — fio -> arcade AZUL freeze B (JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J4 em X=40, Y=-36.5 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J4 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 11 — J5 — fio -> arcade BRANCO buzios (JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J5 em X=53, Y=-36.5 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J5 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 12 — J6 — fio -> LED do arcade A (1=A, 2=K) (JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J6 em X=26, Y=-46 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J6 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 13 — J7 — fio -> LED do arcade B (1=A, 2=K) (JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical)
- **ONDE OLHAR:** JLC 3D preview — rodar para vista de TOPO; ampliar sobre J7 em X=38, Y=-46 (rotação CPL = 0°, lado top) — confirmar contrato de pinagem vs o outro lado
- **O QUE DEVES VER:** A ordem de pinos de J7 bate no que o cabo/módulo do outro lado espera (pino-1 na serigrafia).
- **COMO CONFIRMAR:** PASS: pinagem coincide. FAIL: fila/ordem trocada → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 5. BOM / montagem

### Item 14 — BOM — toda linha SMT com LCSC em stock de MONTAGEM
- **ONDE OLHAR:** O BOM (coluna LCSC) e a página da peça na JLC (Assembly / stock).
- **O QUE DEVES VER:** Todas as 12 linhas SMT levam C-number e stock de MONTAGEM>0. Sem LCSC: nenhuma.
- **COMO CONFIRMAR:** PASS: cada SMT com LCSC e stock de montagem. FAIL: linha SMT sem LCSC ou sem stock de montagem → resolver a fonte (⚠️ stock na LCSC ≠ stock de montagem), NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 15 — BOM — THT enviados à parte (não vão pela JLC)
- **ONDE OLHAR:** A lista de peças THT do desenho.
- **O QUE DEVES VER:** As peças THT são montadas por ti, à parte: C3 (100u 16V), J1 (IDC 2x13 femea <- cerebro), J2 (OLED SSD1309 2,54mm 7p), J3 (fio -> arcade AZUL freeze A), J4 (fio -> arcade AZUL freeze B), J5 (fio -> arcade BRANCO buzios), J6 (fio -> LED do arcade A (1=A, 2=K)), J7 (fio -> LED do arcade B (1=A, 2=K)), RV1 (10k lin), RV2 (10k lin), SW1 (EC11 — o unico encoder).
- **COMO CONFIRMAR:** PASS: THT marcadas como 'cliente fornece / à parte'. FAIL: THT metida na montagem SMT → corrigir o BOM/CPL.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

### Item 16 — BOM — DNP correcto
- **ONDE OLHAR:** As linhas marcadas DNP (do not place) no BOM/CPL.
- **O QUE DEVES VER:** As peças DNP estão fora da montagem; as de montar estão dentro.
- **COMO CONFIRMAR:** PASS: DNP coincide com a intenção. FAIL: peça a montar marcada DNP (ou vice-versa) → corrigir antes de pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 6. Mecânica (alcance dos recortes do painel)

### Item 17 — Mecânica — conectores/botões/display alcançam os recortes
- **ONDE OLHAR:** O render da caixa e/ou o dry-fit de UMA caixa antes da série.
- **O QUE DEVES VER:** Cada peça que atravessa o painel alcança o recorte certo: J1 (IDC 2x13 femea <- cerebro); J2 (OLED SSD1309 2,54mm 7p); J3 (fio -> arcade AZUL freeze A); J4 (fio -> arcade AZUL freeze B); J5 (fio -> arcade BRANCO buzios); J6 (fio -> LED do arcade A (1=A, 2=K)); J7 (fio -> LED do arcade B (1=A, 2=K)); RV1 (10k lin); RV2 (10k lin); SW1 (EC11 — o unico encoder). Folga do cabo e alcance com a placa montada.
- **COMO CONFIRMAR:** PASS: dry-fit/render mostra cada uma no seu recorte. FAIL: encoder/pot/display/botão não alcança ou colide → voltar ao Estágio 3, NÃO pagar.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

## 7. Confirmação final antes de pagar

### Item 18 — Passe limpo — precondição do pagamento
- **ONDE OLHAR:** Este questionário, de cima a baixo.
- **O QUE DEVES VER:** TODOS os itens acima com ✅. Zero ❌, zero campo em branco, zero ambiguidade.
- **COMO CONFIRMAR:** PASS (todos ✅): o humano fica AUTORIZADO a pagar — o pagamento é acção do humano. FAIL (qualquer ❌/dúvida): PÁRA, encaminha para o conserto, e NÃO se paga.
- **RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: ________________________________

---
_19 itens (o Item 0 é o carregamento do pacote, antes da inspeção). Enquanto houver um ❌ ou um campo em branco, o portão está FECHADO e o pagamento não é autorizado._
