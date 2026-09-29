---
name: aipcb
description: Levar um circuito de uma especificação em linguagem normal até um pacote de fabrico encomendado (Gerbers, BOM, CPL) — "prompt in, PCB out" — com Claude a propor e verificar, o KiCad a dar o veredicto, e três vetos humanos que não se automatizam. Use SEMPRE que aparecer /aipcb, "nova placa", "rev C", "manda para a JLC", "redesenha o cérebro", "novo PCB", "prompt to board", "leva isto até aos gerbers", uma espec de circuito, uma mudança de esquema, ou qualquer pedido que vá de uma ideia/especificação a uma placa fabricável. Substitui o /kicad (aposentado em 09/09/2026): o /kicad descrevia geradores que escreviam ficheiros KiCad à mão; o /aipcb descreve um fluxo em que ninguém escreve ficheiros KiCad à mão. Funde também a orquestradora /pcb (aposentada em 2026-09-19): é a ORQUESTRADORA de ponta a ponta — chama `kicad_layout`, `caixa`, `revisao-terra`, `design-review`, `pm`, `para_fabrica` — e certifica o encaixe mecânico placa↔caixa (IFÁ10000-M).
---

# /aipcb — do prompt à placa

> **O veredicto vem do KiCad. O significado vem da ficha. O agente propõe e
> verifica; não serializa.**

Uma frase, três cláusulas, e cada uma é uma semana que já se pagou:
- *o veredicto vem do KiCad* — ERC, DRC, netlist, ratsnest, Gerbers: `kicad-cli`
  e a API IPC. Nunca um leitor nosso a decidir "está ligado".
- *o significado vem da ficha* — pino 2 é `−IN` porque a folha de dados do
  OPA1642 o diz, não porque o símbolo desenhou um sinal em baixo.
- *o agente propõe e verifica; não serializa* — nenhum `.py` escreve
  `.kicad_sch` ou `.kicad_pcb`. Escreve-se pela API, pela GUI, ou não se escreve.

---

## 0 · O que este fluxo é, e o que não é

**É:** uma espec entra, um pacote de fabrico sai, em horas. Entre os dois há
ferramentas que produzem e verificam, e **três portas** onde tudo para e espera
por um humano — porque foram os únicos olhares que apanharam um encoder no ar,
um conector fora da placa e o silk a sair do bordo.

**Não é:** sem vetos. Automatiza a produção e a verificação. Não automatiza o
julgamento.

**É também a ORQUESTRADORA de ponta a ponta** (o que era a `/pcb`): não desenha
esquemas, não roteia, não corta caixas — **delega** a quem já sabe e cobra o
portão de cada etapa. O corpo executável de metade do fluxo já existe em
`kicad/faz_placa.py` e nas skills irmãs — **a regra número um é chamar, nunca
reescrever.**

**Quem faz o quê:**

| | faz | não faz |
|---|---|---|
| **chat** | espec → ppc; decisões de circuito; roteiros do Cowork; **varredura independente** nas portas 2 e 3 | não afirma facto de hardware sem fonte; não escreve ficheiros KiCad |
| **Code** | API (`kipy`/Konnect), `kicad-cli`, os três verificadores nossos, o pacote, o banco de prova | não patch de texto em `.kicad_*`; não "confirmar" o que não mediu |
| **Cowork** | o ecrã, **só por roteiro** determinístico: KiCad onde não há API, JLC, Ponoko | não improvisa, não desenha com o rato livre |
| **humano** | os três vetos, o multímetro, pagar | não solda com o circuito ligado; não refaz terra sem a Daisy fora |

---

## 1 · A entrada: uma espec tem de trazer

Sem estes, o fluxo **não começa** — devolve-se a espec com o que falta:

1. **Função** em uma frase, e o que é facto de exemplar (varia por unidade da
   série: ganho de exciter, C2 do piezo) vs facto de série.
2. **Peças fixas** com MPN **da ficha**, não da memória — e a ficha vai para
   `docs/datasheets/` no mesmo commit. MPN sem ficha aberta é hipótese e diz-se.
3. **Redes** com nome humano (`AUDIO_IN_L`, não `Net-(U1A-+)`), e quais são
   contratos entre placas (o IDC) ou com a caixa (furos, conectores de bordo).
4. **Restrições**: caixa (contorno, furos, alturas por zona), camadas, potência
   por rede, o que é ilha analógica e onde está a ponte única de terra.
5. **O que se mede na bancada** antes de fixar (valor do resistor do LED, C2 do
   piezo, tipo do DPDT) — com a medição escrita como passo, não como nota.

---

## 2b · MECÂNICA-PRIMEIRO — corre ANTES da §2 e antes da porta 1

> Está numerada 2b e colocada antes da §2 de propósito: é a secção que decide se
> a §2 sequer faz sentido. Escolher símbolos e footprints antes de fixar o datum
> é escolher peças para uma caixa que ainda não existe.

**O painel é o datum. O fundo é tampa.** O fundo não carrega nada.

Nasceu de nove sintomas de montagem da rev C com **uma** causa: o fundo tratado
como datum, a mecânica como consumidora do desenho eléctrico. bts 15ga.

### As sete asserções, e todas são verificáveis

1. **O painel é o datum**; a caixa é derivada dele, nunca o contrário.
2. **Ordem de montagem: peças enfiadas sem soldar → painel aparafusado → soldar
   depois.** Nunca acertar altura por cálculo — a mecânica decide, a solda segue.
3. **Sanduíche unida por header, zero cabos internos de sinal:**
   controlo (topo) · E/S (borda, a 90°) · cérebro (fundo).
4. **Conectores PCB-mount com rosca de painel.** Sem fios de áudio internos; a
   blindagem fica no cabo externo.
5. **Conversão de energia na placa** (recetáculo USB-C + chip de PD sink), não em
   módulo aparafusado.
6. **Comutação de potência em compartimento próprio**, com parede e terra
   próprios — regra eléctrica, não de espaço.
7. **Display atrás do painel**; o painel é a protecção.

E a que atravessa todas: **madeira é aparência, painel é tolerância.** O datum
nunca vive em material que se move com a humidade.

### O footprint é onde isto passa a ser possível ou impossível

> **Pino longo não é detalhe de encomenda: é a folga física que permite à
> mecânica mandar.** Um pot de pino curto obriga a acertar a altura por cálculo,
> porque não há curso para a ajustar — e nesse momento a asserção 2 deixou de
> ser executável, qualquer que seja a intenção do desenho.

Ao escolher footprint de qualquer peça que atravesse o painel, a pergunta é:
**quanto curso vertical é que isto me deixa?** Zero curso é defeito de
arquitectura, não de montagem.

### ⚠️ E a fronteira que nenhum portão desta skill vigia

ERC, DRC, `funcao_dos_pads`, folgas, courtyards: **todos verdes, e a caixa não
se monta.** Ver §5. A verificação de montabilidade é o dB com as peças na mão; o
modelo 3D do `ppc/003` é a única coisa que a antecipa. **Um portão verde aqui é
silêncio com cara de aprovação.**

### Prova externa

`docs/REFERENCIA-GUITARPEDAL-STM32.md` — um pedal aberto e vendido cujo README
manda *"use enclosure to ensure proper fit while soldering"*, cuja biblioteca de
footprints se chama `GuitarPedal125B` (a caixa no nome), e em que **17 de 101
footprints** são through-hole: exactamente os que atravessam o painel.

---

## 2 · A fonte

**KiCad, biblioteca oficial.** O símbolo oficial traz a função por pino
(`+`, `−`, `G`, `S`, `D`, `K`, `A`), o footprint oficial traz os nomes de pad
(`A B C S1 S2` no EC11). C1, C2 e C4 da rev B não podiam ter existido.

**Símbolo próprio só para o que não existe** (módulos: Daisy, SuperMini,
decoy) — e obrigatoriamente com **pinos com nome de função**, ficha ao lado,
e entrada no `funcao_dos_pads`. Um símbolo com pinos `1 2 3` sem nome é
defeito, não atalho.

**Se a Fase 1 do banco de prova provar que o atopile recusa C1/C2/C4 sozinho:**
a fonte do esquema é o `.ato`, o KiCad é só layout, e **ninguém edita o esquema
no editor** — qualquer clique é perdido na compilação. Decisão escrita aqui,
uma vez; não se revisita a meio de uma rev.

**O que nunca mais:** geradores que escrevem `.kicad_sch`/`.kicad_pcb`. Os da
rev B vivem em `docs/rev_b/geradores/` como registo de como a rev B nasceu.

**O KiCad é a fonte-da-verdade, o JLCONE é o carimbo final.** Formato aberto,
versionado em git, automatizável por agentes, independente de fornecedor
(coerente com o ethos do projeto: fronteira HAL, Konnect isolado). O **EasyEDA
Pro / JLCONE é o front-end final de DFM + encomenda, nunca a fonte.** Por isso o
nosso portão dá **paridade com o DFM da JLC** (regras de silk→pad/→furo/largura
no `.kicad_dru` + `para_fabrica`) para o passo no JLCONE ser um **carimbo**, não
caça-surpresas. Não migrar o pipeline para o EasyEDA Pro (perderia a automação
por agentes e o versionamento, e criaria lock-in na JLC); para placas
existentes, no máximo importar os gerbers no EZEDA só para DFM/order finais.

---

## Filosofia dos portões

O fluxo inteiro é subordinado a frases que as skills irmãs compraram com tempo
perdido:

- **Um portão que avisa e deixa passar não é portão** (`caixa`). Cada etapa tem
  de conseguir dizer *não* e parar a fila.
- **Prova que a guarda sabe dizer não** (`funcao_dos_pads.py --sabota`). Uma
  guarda que nunca reprovou é uma promessa, não uma verificação. Verde na
  primeira corrida é suspeito — só se aceita o que se viu recusar sob sabotagem.
- **O gerador é a fonte** (§2). Cota escrita à mão ao lado da calculada
  envelhece sempre; importa-se, não se copia.
- **Medir no sítio certo, ou o visto não vale nada** (`kicad_layout`). Um
  verificador que mede o ponto errado aprova tudo com ar de ter conferido.
- **Resultado negativo é achado** (`revisao-terra` R6). O que passou fica
  escrito, ou a próxima revisão refaz o trabalho.
- **Certo não é preciso** (15ez). Uma revisão de correcção só vê o que está lá
  para corrigir; o que sobra está correcto. A cada peça, **primeiro «o que liga
  aqui?»**, só depois «está certo?». Uma migração troca o consumidor e deixa o
  fornecedor — procurar do outro lado da troca.
- **Uma placa que não se monta não está pronta** (dB, 18/09: *«de que vale uma
  placa correta eletricamente se não podemos montá-la?»*). A montagem tem o mesmo
  peso que a ligação, e o `para_fabrica` recusa as duas.
- **A caixa cede à placa — por ordem de prioridade, não como lei** (IFÁ10000-M).
  É muito mais fácil ajustar uma embalagem do que uma PCB. Mas a placa traz os
  seus próprios meios de fixação: sem eles não há como a prender, e aí é ela que
  cede.

---

## Fronteira — quem responde o quê

| pergunta | quem |
|---|---|
| o esquema liga? o ficheiro abre? ERC? | **/aipcb** (este fluxo) |
| placa, cobre, planos, serigrafia, pacote de fabrica | `kicad_layout` |
| a caixa fecha, a placa cabe, os furos batem | `caixa` |
| a massa está num estado legal | `revisao-terra` |
| o desenho lê-se | `design-review` |
| ordem de encomendas / caminho crítico | `pm` |
| toda edição de ficheiro que a pipeline faça | `edicao-segura` |

Este fluxo não decide nada de electrónica que uma skill irmã já decida:
**decide a ORDEM, cobra o PORTÃO de cada etapa, e recusa avançar com uma etapa
reprovada.** O trabalho de esquema/layout que era a antiga skill `/kicad`
(aposentada) entra por aqui; o layout e o roteamento correm com `kicad_layout`.

---

## 3 · As três portas

Cada porta tem **o que exige para abrir**. Nada passa por "está quase". A
máquina operacional é uma pipeline de **9 estágios (1–9) mais a etapa 0 de
pré-voo e o portão 3.5** — cada estágio passa com um número que pode dizer
**não** — e ela mapeia-se nas três portas assim:

- **Estágio 0 (pré-voo)** corre **antes de tudo**, é o arranque da cadeia.
- **Estágios 1–2 → Porta 1** (espec, diagrama, esquema congelado, fontes).
- **Estágios 3, 3.5, 4, 5, 6, 7 + Certificação de Risco → Porta 2** (colocação
  vs caixa, congestão, cobre, terra, rótulos, revisão tripla — tudo antes do
  humano vetar o layout).
- **Estágio 8 + Certificação Mecânica IFÁ10000-M final → Porta 3** (pacote,
  encomenda, pagamento).
- **Estágio 9 (retrospetiva)** corre **depois da entrega** — ver Anti-decadência.

E **dois portões de certificação**: o de **Risco** (saída da revisão tripla,
entrada do pacote) e o **Mecânico IFÁ10000-M** (corre no Estágio 3, antes do
cobre, e outra vez antes do 8, com a caixa real).

### Estágio 0 — Pré-voo: a ferramenta bate com os ficheiros?

- **Objetivo:** provar que a cadeia de ferramentas consegue mesmo abrir e medir
  estes ficheiros, antes de acreditar em qualquer número.
- **O que faz:** confirma que o KiCad instalado é o certo, que os ficheiros
  **ABREM**, e que os scripts estão presentes.
- **Se a ferramenta faltar ou for velha demais, a pipeline NÃO está bloqueada:**
  arranca uma via `scripts/bootstrap_kicad.sh` (AppImage do GitHub — ver
  `references/fab-automation.md` §0) e **volta a correr o portão de versão** abaixo.
- **Delega a:** `medicao-limpa` (é a etapa 0 do método — canal provado, ambiente
  zerado, regime declarado).
- **PORTÃO:** ⚠️ **A versão MAIOR do KiCad tem de ser igual ao token
  `(version …)` dos ficheiros.** Ficheiro KiCad-10 exige KiCad 10 — um
  descasamento **PÁRA a pipeline**. E `kicad-cli sch erc` / `pcb drc` têm de
  correr sem erro de carregamento: *"o checker passou" ≠ "a folha abre"*.
  Netlist limpo não é ficheiro que abre.
- **Artefacto:** linha de versão (`kicad-cli version`) + token dos ficheiros, e
  o exit-code do primeiro `erc`/`drc`, no relatório da sessão.

### Porta 1 — esquema congelado

**O que a porta exige para abrir:**
- `kicad-cli sch erc` = 0 (isenções só com prazo, e o prazo escrito)
- `funcao_dos_pads` = 0 nas duas placas
- contratos: `contrato_idc` 26/26 (ou o que for), redes de caixa nomeadas
- revisão de terra (skill `revisao-terra`) feita sobre **este** esquema
- render do esquema visto pelo humano
- **O humano diz "congela".** A partir daqui, mudança de esquema é rev seguinte.

O que alimenta esta porta são os Estágios 1 e 2:

#### Estágio 1 — Avaliação + diagrama legível

- **Objetivo:** transformar o prompt numa spec e num diagrama que um humano
  aprova antes de se gastar em peças.
- **O que faz:** prompt → spec (função, trilhos de alimentação, conectores,
  quantidades, restrições mecânicas), e um **diagrama de blocos legível**.
- **Delega a:** o que todo esquema declara (tensão+fonte, por-canal vs
  partilhado, afinação por unidade, massas distintas com símbolos distintos,
  premissas — a doutrina de esquema deste fluxo); `design-review` (o diagrama
  passa por `tools/svg_colisoes.py` antes do olho).
- **PORTÃO:** a spec declara os seis itens de esquema; o diagrama passa
  `design-review` (exit 0/1/3, 3=não-verificável não é aprovado); **⟨dB⟩** um
  sign-off marcado **antes** de gastar em peças — corrido como o questionário do
  portão humano (`references/portao-humano.md`), nas categorias de nível-placa e
  de contratos de pinagem.
- **Artefacto:** spec + diagrama commitados, com o resultado do `svg_colisoes` e
  o ⟨dB⟩ registados.

#### Estágio 2 — Fontes de componentes (o maior buraco)

- **Objetivo:** toda linha do BOM resolvida numa peça que a JLC monta mesmo.
- **O que faz:** aplica a árvore de **3 níveis** — (1) equivalente na biblioteca
  de montagem da JLC → (2) importar/converter a peça exacta
  (`easyeda2kicad`/`JLC2KiCadLib`) → (3) construir do datasheet (IPC-7351
  Nominal). Ver **`references/parts-sourcing.md`** para a árvore, o checklist de
  equivalência, e a tabela de ferramentas.
- **Quando a via é o Mouser (montagem em casa / peça fora da JLC):** o sandbox
  **não alcança o Mouser** (403 de egress) — o `WebSearch` prova catálogo, nunca
  estoque. **O navegador do humano (Claude-on-Chrome) é a fonte-da-verdade de
  disponibilidade:** "disponível" = **Ships Now ≥ qty**, e **backordered/long
  lead time contam como INDISPONÍVEL** (dão MATCH e não embarcam — o backorder
  escapa ao filtro "no match"). NUNCA declarar "em estoque" a partir do sandbox —
  catálogo ≠ estoque. O fluxo em 3 passos, o prompt-template copiável e o filtro
  completo estão em **`references/parts-sourcing.md §7`**.
- **Delega a:** `codigos_lcsc.py` (acha o C-number por valor/pacote),
  `fichas.py` (datasheet verificado por dentro), `bom_completa.py` (o BOM
  gerado), e **`scripts/manifesto_fontes.py`** (o portão STOP).
- **PORTÃO — o STOP gate:** só avança quando **toda** linha está num estado
  legal: (A) C-number válido, na biblioteca de montagem (stock>0), com tier;
  (B) footprint importado/à-mão **verificado**; (C) cliente-fornece
  verificado. `manifesto_fontes.py` sai ≠ 0 e lista as linhas bloqueadas —
  provado com `--sabota`. ⚠️ *Stock na LCSC ≠ stock de montagem na JLC.*
- **Artefacto:** **manifesto de fontes commitado** (uma linha por linha do BOM),
  e o exit 0 do `manifesto_fontes.py` sobre ele.

### Porta 2 — render antes de rotear

**O que a porta exige para abrir:**
- peças fixas (furos da tampa, conectores de bordo, furos M3) colocadas pela
  API nas coordenadas **derivadas** da caixa, não escritas à mão
- `paridade_caixa` = 0; courtyards **dentro** do contorno com ≥ 0,5 mm;
  furos com raio de exclusão ≥ 3,5 mm a qualquer courtyard
- render 2D das duas placas + o 3D (`product-view`) com a caixa
- **varredura independente** (chat) sobre os ficheiros, não sobre o relatório
- **O humano veta ou não.** Só depois: terra e áudio à mão, resto Freerouting.

O trabalho pesado desta porta são os Estágios 3 a 7 e o Portão de Certificação
de Risco.

#### Estágio 3 — Colocação contra a estrutura física

- **Objetivo:** a placa **tem por onde ser presa**, e a caixa encaixa nela —
  tudo o que atravessa o painel bate no recorte certo, antes do cobre.
- **O que faz:** furos de fixação **da placa primeiro** (prioridade 1 do
  IFÁ10000-M: ≥3 pontos, perto de onde se empurra); o vão interior da caixa é o
  limite de tamanho da placa, e **o resto da caixa deriva-se dela** (furos,
  recortes, postes); conectores/botões/display batem nos recortes do painel
  (`aberturas.json`); nenhum parafuso debaixo de um componente; zonas de altura
  respeitadas; cablagem/conectores com folga para o cabo e alcançáveis com a
  placa montada.
- **Delega a:** `caixa` (os 5 portões, e gera a geometria da caixa **a partir da
  placa** — prioridade 2 do IFÁ10000-M), `kicad/pcb_contorno.py` (importa o
  gerador da caixa, escreve `aberturas.json`), `poe_furos.py` (furos M3
  derivados do contorno).
- **PORTÃO:** os 5 portões de `caixa` (nesting, espelho, margem, **a placa
  cabe** `confere_caixa`, **furo livre** `confere_furos` — ambos em
  `pcb_contorno.py`; mais `dentro_do_contorno.py` para courtyard×bordo). ⚠️ **A
  origem do footprint não é o eixo** — ler o veio em `F.Fab`. E os três que o
  DRC 2D não vê: **volume (Z), acesso (USB-C), camada vazia ≠ face vazia** —
  cada um é o seu portão. **E as etapas M1–M6 do IFÁ10000-M** (interfaces,
  fixação, furos, tamanhos, ângulos, volume, via `kicad/encaixe.py`) — M2 com
  zero pontos de fixação reprova aqui, antes de haver cobre para refazer.
  ⟨dB⟩ montagem a seco de UMA caixa antes da série.
- **Artefacto:** `.kicad_pcb` com contorno+furos, `aberturas.json`, e o
  handshake contorno↔aberturas confirmado.

#### Estágio 3.5 — Previsão de congestão (pré-layout, entre 3 e 4)

- **Objetivo:** prever o estrangulamento **ANTES de tocar no cobre**. Congestão
  descoberta a rotear é falha de **arquitetura** tarde demais — o router não a
  resolve, só a revela quando já custa refazer a colocação.
- **O que faz:** mede a colocação (footprints+netlist, zero pistas) por
  densidade e diz se cada região tem oferta para a procura que a colocação lhe
  impõe. As métricas: **orçamento de canais de escape por conector** (procura =
  redes não-plano distintas · oferta = broadsides com ≥3 mm abertos × colunas);
  densidade de pinos por célula; densidade de cruzamento do ratsnest; procura vs
  oferta de cobre por região; fragmentação de plano na colocação. Fórmulas,
  método completo e as duas regras em **`references/congestao.md`**.
- **Delega a:** `kicad_layout` (mede por rede, autoridade sobre cobre e planos),
  `caixa` (a geometria que fixou a colocação). Ferramenta:
  **`scripts/congestao.py`** (orçamento de escape + grelha de densidade; sai ≠0
  em qualquer vermelho; `--sabota` prova que sabe dizer não).
- **PORTÃO — GO / REVER-COLOCAÇÃO:** uma região vermelha **não avança para o 4**;
  volta ao **Estágio 3** (recolocar), ou ao **1/2** se o que estrangula é o
  **pinout do conector** (fila/ordem de pinos que nenhuma recolocação salva) —
  **nunca** para o 4 com vermelho em aberto. **Duas recusas duras:** **(a)**
  recusar conector de ≥2 filas onde procura > oferta de canais de escape;
  **(b)** recusar courtyard que sobrepõe outro courtyard, a borda da placa, ou o
  corpo de outro componente — medido por corpo, não por courtyard nominal.
- **Artefacto:** **mapa de congestão no registo de risco** (uma região por
  linha, procura/oferta medidas), ligado ao Portão de Certificação de Risco.
  Âncoras: **J14/C16** (cérebro) — nudge de J14 bloqueado fisicamente pelo
  pré-amp, a congestão apanhada tarde demais que virou o MUST-FIX C16↔J14;
  **J1/C5** (painel) — a **mesma** classe de falha, latente, desfeita pelo nudge
  real de 5,5 mm que abriu o segundo broadside e deu casa a C5. É a forma
  **pré-layout** do **IFÁ10000**: certifica a colocação contra medida, não
  contra opinião.

#### Estágio 4 — Vias + largura de pista (só DEPOIS da colocação)

- **Objetivo:** cada rede com cobre que aguenta a sua corrente, vias que a
  fábrica consegue furar, zero por rotear por (rede, espécie).
- **O que faz:** larguras por classe de rede dimensionadas por IPC-2221 para a
  corrente de cada rede; vias ≥ mínimo JLC; costura (stitching); roteamento.
  Ver **`references/fab-automation.md`** (tabela IPC-2221, capacidades JLC).
- **Delega a:** `kicad_layout` (OBRIGATÓRIA no fecho), `classes_de_rede.py` (as
  larguras vivem nas classes, o DRC e o router lêem-nas), `roteia.py`/
  `roteia_fr.py`, `pcb_contorno.costura()`.
- **PORTÃO:** **só se encosta cobre depois de os portões dos estágios 3 e 3.5
  fecharem** — a colocação aprovada (e sem vermelho de congestão) é a porta de
  entrada, «só então» as vias, nunca antes; larguras nas classes de rede (o DRC
  obriga); vias ≥ JLC (furo ≥0,2 mm, anel ≥0,13 mm, ⌀ ≥0,45 mm); **por (rede,
  espécie) o não-roteado = 0**, e o invariante é *nenhuma rede pode piorar* (o
  total esconde uma rede que regride; `Zone<->Zone` é só indicação). DRC com
  `--refill-zones`.
- **Artefacto:** relatório DRC por rede (via `le_drc.py`), classes de rede
  escritas no ficheiro.

#### Estágio 5 — Aterramento (um estágio inteiro só para isto)

- **Objetivo:** cada condutor de terra num estado legal — o que nenhum DRC vê.
- **O que faz:** passagem completa de `revisao-terra` — caminhos de retorno,
  ponte única AGND/DGND, par de desacoplamento por consumidor conforme o tipo de
  carga, passo de costura λ/20, plano em B.Cu + via por pad SMD-GND.
- **Delega a:** `revisao-terra` (passadas P0–P7, regras de ferro R1–R8). Ver
  **`references/quality-standards.md`** §2 para os números (desacoplamento por
  carga, passo de costura, plano).
- **PORTÃO:** as passadas P0–P7, cada uma respondendo UMA pergunta; passada
  saltada é **declarada**, não silenciada. Achado sai com severidade+dono (R7).
  🔴 estrela não apontável · AGND/DGND juntos duas vezes · banco de baixa ESR na
  entrada de conversor comutado. ⚠️ *Não tirar a terra de sinal para matar
  ruído* (P3b) — resolve-se com desacoplamento local.
- **Artefacto:** relatório de terra no formato fixo de `revisao-terra` (o que
  passou, achados por severidade, o que NÃO foi verificado).

#### Estágio 6 — Rótulos e texto

- **Objetivo:** serigrafia que se lê e que aponta para a peça certa.
- **O que faz:** `silk_over_copper = 0`, silk fora de pads/máscara, refs a
  apontar para a **própria** peça, marcas de pino-1/polaridade; e a colocação de
  **grafismos** (logo/nome/símbolo) por **coordenada de grid**, não por arrasto.
- **Grafismos por GRID (substitui "arrastar no viewer"):** grafismo decorativo
  coloca-se por **coordenada lida num render com grid de 0,5 mm** (majors de 5 mm
  rotulados, eixos no frame do KiCad, zonas no-go sombreadas, retângulos livres
  cotados em mm), não por olho/pixel. ⚠️ **Arrastar no visualizador de gerber
  ENGANA** — o viewer **não recorta** silk sobre pad, a fábrica recorta, e o
  texto chega **quebrado**. Método (bbox do grafismo declarada ANTES · 1
  aprovação lendo o grid · 1 edição só-silk → 1 regen · transfer que **rejeita
  silk sobre pad/furo**, gerber-truth) e o helper `kicad/grid_grafismo.py` em
  **`references/grafismos-grid.md`**.
- **Delega a:** `arruma_silk.py` (move refs, auto-porta-se — não escreve se as
  colisões sobem), `refs_apontam.py` (distância à própria peça vs à vizinha),
  `rotula_tp.py` (nome do TP vem do `(net)` do pad, nunca do `Value`),
  `grid_grafismo.py` (o mapa de grid para colocar grafismos por coordenada),
  `design-review` (legibilidade com o olho).
- **PORTÃO:** silk sobre cobre = 0; texto×texto = 0; stroke ≥0,15 mm, altura
  ≥1 mm; `refs_apontam.py` sai ≠0 se um designador está mais perto da peça
  errada; marcas de polaridade/pino-1 presentes no **impresso**. ⚠️ *Contagem-
  zero autoriza olhar sabendo onde, não saltar o olho* — o campo roda com a peça
  (`X = fx + dx·cos a + dy·sin a`), o obstáculo é pads+trilhos+silk+contorno.
- **Artefacto:** exit 0 de `refs_apontam.py` e do gate de silk; resultado de
  `design-review` no relatório.

#### Estágio 7 — Revisão tripla (agentes independentes, 3 lentes, 3 passagens)

- **Objetivo:** três olhares diferentes, independentes, adversariais, sobre a
  mesma placa, para que nenhuma classe de defeito escape por não ser procurada.
- **O que faz:** corre **três lentes**, idealmente por revisores diferentes, e
  **corre o conjunto 3×**:
  - **Lente A — eléctrica/conectividade:** ERC; `funcao_dos_pads.py`
    (pad→rede→função, o *significado*); paridade esquema↔placa; não-roteado por
    rede.
  - **Lente B — DFM/fabricabilidade:** DRC vs capacidades JLC com `.kicad_dru`;
    courtyards; BOM↔CPL↔placa; pré-visualização de rotação.
  - **Lente C — físico/fatores humanos:** encaixe na caixa; legibilidade do silk
    com o olho; alcance dos conectores; térmica.
- **Delega a:** `kicad_layout`, `caixa`, `revisao-terra`, `design-review`,
  `revisao_externa.py` (kicad-happy, segunda opinião externa). Ver
  **`references/review-protocol.md`** (as 3 lentes em detalhe, o livro-razão de
  achados, revisores adversariais). Modelado no `ifa10000`.
- **PORTÃO:** cada lente passa com **números, não vistos**; lente reprovada
  **interrompe** (não se salta "pra ver se passa"); **a última linha de defesa
  não é a ferramenta que fez o trabalho**; **livro-razão de achados** para que
  nada caia em silêncio; um achado que reaparece vira **portão novo no mesmo
  commit** (não uma nota); **⟨dB⟩** o veto dos renders de placa na lente C
  (`design-review` passa pelo olho — exit 3 não-verificável não é aprovado),
  corrido como a categoria de mecânica do questionário do portão humano
  (`references/portao-humano.md`).
- **Artefacto:** a ficha das 3 lentes × 3 passagens preenchida com números, e o
  livro-razão de achados (severidade, onde, regra pelo nome, conserto).

#### Portão de Certificação de Risco — saída da revisão tripla, entrada do pacote

Entre a revisão tripla e os gerbers há um portão que não é mais uma lente: é a
**certificação**. `para_fabrica` (Estágio 8) **só corre quando o registo de
risco está GO.** Um aviso embarcado ou um waiver **é uma MEDIÇÃO, não uma
opinião** — a mesma disciplina que o `ifa10000` exige para efeitos, aqui
instanciada para placas (o **IFÁ10000 é o protocolo de certificação canónico do
projeto**; isto é a sua forma para hardware).

- **Certifica contra o sinal, não contra o ruído.** Limpa os artefactos de
  ambiente antes de julgar — um DRC a gritar *"library not configured"* vem de um
  `fp-lib-table` em falta, não da placa; resolve isso primeiro e só então lê o
  **resíduo real**. **Um aviso que ainda não explicaste não é automaticamente
  ruído.**
- **Todo item embarcado leva um veredito MEDIDO,** uma linha cada no **REGISTO DE
  RISCO**: `id · item · lente/etapa · severidade · evidência MEDIDA · veredito
  {SAFE-EMBARK (com prova) | MUST-FIX | decisão-dB} · ação`.
- **GO/NO-GO:** GO só com **zero MUST-FIX em aberto**; SAFE-EMBARK exige
  **evidência medida** (waiver sem número é MUST-FIX por omissão); item
  `decisão-dB` sobe ao portão humano ⟨dB⟩.
- **Âncora (porque isto existe):** o caso **C16↔J14** — um waiver "razoável" de
  sobreposição de courtyard era, ao medir corpo-contra-corpo, uma colisão real
  (o 0805 sentava-se dentro do shroud de um box-header DIN-41651 de assento
  rente). A opinião dizia *waive*; a medição dizia **MUST-FIX**. Esse fosso é o
  ponto todo.

O formato completo do registo, o método *"certifica contra o sinal"* e o caso
C16↔J14 vivem em **`references/review-protocol.md`** — o registo de risco é a
**saída de certificação** da revisão tripla. `para_fabrica` recusa NO-GO como
recusa um DRC sujo.

### Porta 3 — pré-visualização antes de pagar

**O que a porta exige para abrir:**
- `kicad-cli pcb drc` = 0 com silk-sobre-cobre e silk-fora-da-placa ativos
- ratsnest do KiCad = 0 por rotear (não um contador nosso de proximidade)
- zonas re-preenchidas **neste** ficheiro (fill guardado; zero `filled_polygon`
  é placa sem terra)
- `funcao_dos_pads` = 0 outra vez (o roteamento não muda redes; a sincronização
  pode ter mudado)
- BOM = CPL = placa em designadores; código LCSC em **toda** linha que a JLC
  monta; MPN pode ser vazio, código não
- varredura independente completa
- pacote sobe à JLC (Cowork), pré-visualização 3D aberta na lista de peças
  polarizadas do `LEIA-ME.txt`
- **O humano confere as polaridades e paga.**

Antes desta porta corre a trilha mecânica inteira (IFÁ10000-M, com a caixa
real) e o Estágio 8.

#### Portão de Certificação Mecânica — IFÁ10000-M (no 3, e outra vez antes do 8)

A trilha mecânica do IFÁ10000: **o pareamento de cada placa com qualquer caixa
em que ela entre** — furos, ângulos, tamanhos, volume, técnica de fixação,
acesso e montagem, tudo explorado para um encaixe perfeito.

**Este portão DELEGA à sub-skill `/3ddesign`** — o home executável da
IFÁ10000-M e do pipeline 3D (ppc/003). A `/aipcb` **orquestra-a** aqui como
chama `kicad_layout`, `caixa` e `revisao-terra`: não reimplementa o encaixe
volumétrico. O texto completo, as âncoras M0–M10 e a ficha estão em
**`.claude/skills/3ddesign/references/ifa10000-m.md`**; o motor de medição (STEP
real via `kicad-cli` + `build123d`, as 6 asserções, o pré-voo que falha fechado)
vive na `/3ddesign` (`mech/verifica_3d.py`, `mech/params.py`); o registo de
certificados vive em `docs/IFA10000.md`.

**A ordem de prioridades** (é ordem, não lista absoluta):

1. **A placa traz os seus meios de fixação.** Sem furos não há como a prender, e
   nenhuma caixa o resolve — **aqui a placa cede**.
2. **Com isso garantido, a caixa cede à placa em tudo o resto.** É muito mais
   fácil ajustar uma embalagem do que uma PCB: a caixa regenera-se em minutos,
   a placa custa fábrica e semanas, e fabricada não se desfaz.
3. **A placa só cede noutra coisa com justificação excelente, escrita:** o que
   se tentou na caixa e porque não serve (medido), de onde vem a restrição que a
   caixa não absorve, e ⟨dB⟩ assina. Sem as três, é MUST-FIX na caixa.

**As etapas:** M0 medição (cada cota com fonte; peça em mão > ficha) · M1
interfaces · **M2 fixação** · M3 furos e recortes derivados · M4 tamanhos e
contornos · M5 ângulos e orientação · M6 volume Z · M7 acesso e montagem · M8
tolerâncias · **M9 encaixe a seco ⟨dB⟩** · M10 registo.

- **O executável:** `kicad/encaixe.py` — só lê — mede M2 (fixação), M4
  (contorno), M6 (volume sob os arcade, **pelo corpo**) e os **cabos** entre
  placas (percurso 3D + folga de serviço contra o BOM), com `--sabota` a provar
  cada um. E o **M1**: todo conector declara o que liga; dois caminhos para a
  mesma peça, ou a tomada de um módulo que já está num soquete, são vestígio.
  O `para_fabrica` chama-o e recusa «não se monta». O resto é investigação, com
  o método escrito na referência (ficha do modelo comprado lida no desenho,
  rosca só se a ficha a mostrar, compra↔BOM↔peça em mão, altura acoplada, cabo
  pelo percurso real).
- **Quando corre:** M1–M6 no **Estágio 3**, antes do cobre (é aí que mudar a
  placa ainda é barato); a trilha inteira **antes do 8**, com a caixa real.
- **GO:** todas as etapas passam, cedências da placa todas justificadas e
  assinadas. `para_fabrica` não devia correr com M2 reprovado.
- **Âncora:** o painel rev C foi fabricado (5 unidades, 12/09) **sem um único
  ponto de fixação** — desenhado para ficar pendurado nos eixos, os pots
  passaram a ir na tampa por fios e o encoder não tem rosca. Todos os portões
  estavam verdes, porque nenhum perguntava «como é que isto se prende?».

#### Estágio 8 — Gerbers para a JLCPCB

- **Objetivo:** um pacote de fabricação que a JLC aceita, e que diz de que
  estado do repo veio.
- **O que faz:** `para_fabrica.py` mede, recusa se mau, exporta gerbers/drill/
  pos/bom (BOM **agrupada por LCSC** — uma peça física por linha, v0.17.0; e um
  **`cpl_rico_*`** companheiro do CPL da JLC, para a VISTORIA e o kit não
  perderem a geometria rica, v0.18.0), carimba data/commit/dirty, escreve
  `INCOMPLETA` se forçado, e — na passagem — copia o pacote enxuto (zip + CPL +
  `cpl_rico` + BOM + LEIA-ME + `VISTORIA.md` + `kit/`) para
  `export/<placa>_<data>_<commit>/`, a pasta de topo RASTREADA, imprimindo esse
  caminho durável como última linha. O `fabrica/` é scratch gitignored; o
  `export/` é o que sai do portão e chega ao repo/PR.
- **A via de casa (chapa quente) é a segunda saída, com portão de custo.** Quando
  a cotação de montagem da JLC a baixa quantidade é dominada pelo **custo fixo**
  (setup + stencil + markup de peças + engenharia) — regra: se **montagem >>
  (PCB nua + peças)** para a quantidade — encaminha-se para montar em casa:
  encomendar **PCB nua + stencil (Assembly OFF)**, comprar peças pelo carrinho
  LCSC do `kit/`, refluir o SMD do topo na chapa e soldar à mão o bottom + THT.
  O **`kicad/kit_montagem.py`** (v1.0.0) emite esse `kit/` (MAPA_COLOCACAO,
  COMPRA_LCSC, LEIA-ME-KIT) como saída-padrão deste estágio. Fluxo e portão de
  custo completos em **`references/montagem-casa.md`**.
- **Delega a:** `para_fabrica.py` (o portão de fabrica) e `kit_montagem.py` (o
  kit de montagem em casa). Ver **`references/fab-automation.md`** para o
  cheat-sheet do `kicad-cli`, o conteúdo do zip, a BOM-por-LCSC, o CPL rico e o
  kit; e **`references/montagem-casa.md`** para a via da chapa quente.
- **PORTÃO:** **registo de risco em GO** (portão de certificação acima — sem GO,
  `para_fabrica` nem corre); DRC = 0 com regras JLC activas; zonas
  re-preenchidas **no ficheiro** (fill velho embarca calado); BOM = CPL = placa;
  LCSC em toda linha SMT; **⟨dB⟩ correr o questionário do portão humano (ver
  `references/portao-humano.md`), item a item — o questionário abre com o
  carregamento do pacote (Item 0: qual `.zip` carimbado carregar na JLCONE, como
  o buscar ao PR, e a proveniência pelo LEIA-ME) — e só o passe limpo autoriza o
  pagamento** — a rotação peça-a-peça no pré-visualizador da JLC não é
  scriptável (a rotação é a do footprint, a JLC espera a da biblioteca dela; a
  render do NOSSO board é a fonte-de-verdade, e a rede de segurança é **Confirm
  Parts Placement = YES** — ver `references/portao-humano.md §3`). No mesmo
  portão corre o **checklist de settings do pedido JLCONE** (2 camadas · **Via
  Tented**, crítico · ENIG · Confirm Parts Placement=YES · remark das peças
  bottom-side/hand-solder · peças do fundo na lista **Bottom** do matching —
  `portao-humano.md §4a`); carimbo de procedência (data/commit/árvore-suja) +
  guarda `INCOMPLETA`; **e o pacote carimbado COMMITADO em
  `export/<placa>_<data>_<commit>/`.** A entrega dos gerbers é PARTE do portão,
  não um depois: um pacote que só existe em `fabrica/` (gitignored) some com o
  container e nunca chega ao PR — o Estágio 8 **não fecha** até o `export/` estar
  no git.
- **⚠️ Afirmação de versão — CONGELAMENTO-NO-LANÇAMENTO (ver
  `references/versionamento.md`).** Emitir um pacote **afirma que o rev do silk
  bate com o ESTADO DE FABRICO da placa**, e a rev sobe **por lançamento, não por
  commit**: enquanto uma placa **não foi fabricada/pedida**, o gerber pode iterar
  N vezes e a rev **FICA** (`rev C`, sem número) — as iterações distinguem-se
  pelo **stamp de commit** do pacote. A rev só sobe quando um board é
  **efectivamente fabricado/pedido**: aí congela, e a **próxima** mudança de
  gerber abre o rev seguinte. **Silk é gerber**, mas mover/escalar serigrafia
  numa placa ainda não fabricada gera **pacote novo (stamp novo), mesma rev** —
  não sobe a rev. O portão humano confere, pela proveniência do LEIA-ME, que a
  rev carimbada e a impressa são a mesma; e a linha **rev → o que mudou →
  commit** entra em `docs/relatorio/002.md`.
- **Artefacto:** o pacote carimbado sob `export/<placa>_<data>_<commit>/`
  (zip de gerbers + CPL + `cpl_rico` + BOM-por-LCSC + LEIA-ME + `VISTORIA.md`, o
  questionário do portão humano gerado para a placa, + a pasta `kit/` de
  montagem em casa), **commitado e recuperável do PR**, e o registo do ⟨dB⟩ da
  rotação (o `VISTORIA.md` preenchido, todo em ✅).

#### Os vetos ⟨dB⟩ — o método é o questionário do portão humano

Alguns portões **não são scriptáveis** — são humanos, e marcam-se com **⟨dB⟩**.
O fluxo pára e espera aprovação antes de gastar dinheiro. Correspondem às três
portas:

- **⟨dB⟩ Porta 1** (Estágio 1) — o diagrama legível e a spec, aprovados **antes**
  de gastar em peças.
- **⟨dB⟩ Porta 2** (Estágio 3/7) — o VETO dos renders de placa, e a montagem a
  seco de UMA caixa antes da série (é a M9 do IFÁ10000-M).
- **⟨dB⟩ Porta 3** (Estágio 8) — a rotação peça-a-peça no pré-visualizador da
  JLC, antes de pagar. O CPL não é verificável por script (a rotação é a do
  footprint, a JLC espera a da biblioteca dela).
- **⟨dB⟩ IFÁ10000-M, prioridade 3** — toda cedência da placa à caixa, além dos
  meios de fixação, leva a assinatura do dB com a justificação escrita.

**Todos os ⟨dB⟩ correm o mesmo método — o questionário do portão humano**
(`references/portao-humano.md`): um item de cada vez, quatro campos por item
(ONDE OLHAR · O QUE DEVES VER · COMO CONFIRMAR · RESPOSTA), e **só o passe limpo
autoriza o passo seguinte** — na Porta 3, o pagamento. Qualquer ❌ ou
ambiguidade pára e encaminha para um conserto, nunca para pagar. "Confirmar
rotações" não é portão; o questionário é. Sem o ⟨dB⟩, o estágio fica em
`INCOMPLETA` — nunca se confunde com feito.

---

## 4 · Ferramentas por estação

| estação | ferramenta | regra |
|---|---|---|
| ler ficheiros sem KiCad aberto | `rjwalters/kicad-tools` ou o nosso `kaudit.py` | só para **ler**; delimitar por parêntesis, nunca por contagem de caracteres |
| esquema | editor KiCad (humano/Cowork) · `kicad-mcp-kipy` para ler · atopile se a Fase 1 disser | biblioteca oficial |
| placa | `kicad-python` (`kipy`) · Konnect | SWIG `pcbnew` está em manutenção: não se escreve código novo nele. ⚠️ **O socket é por processo**: `/tmp/kicad/api.sock` é o gestor de projecto e recusa tudo com `no handler … GetOpenDocuments`; o editor de placa é `api-<PID>.sock`. Usar `kicad/_v2/liga.py`. (15ev) |
| verificação | `kicad-cli sch erc / pcb drc / export netlist` | é o veredicto |
| roteamento | terra e áudio à mão; Freerouting via Konnect para o resto | nunca "faz_placa do zero" numa placa já colocada |
| peças | LCSC/JLC via API; ficha lida com `pdftotext` e a **tabela de encomenda** conferida por sufixo | busca difusa responde com confiança ao que não sabe: filtrar por descrição estrita, não por ordem |
| pacote | `kicad-cli pcb export gerbers/drill/pos`, BOM do KiCad | nada escrito por nós no pacote |
| ecrã | Cowork por `docs/roteiro/NNN.md` | hotkeys, valores em diálogos, coordenadas digitadas |
| revisão externa | kicad-happy, Konnect audits | segunda opinião; **não** portão |

**Os três nossos, e só estes:**
- `funcao_dos_pads.py` — por referência, da ficha: pad → rede → função
  esperada. Contrato "estes pads na mesma rede", nunca nome auto-gerado.
- `contrato_idc.py` — via a via entre placas, pela netlist do KiCad.
- `paridade_caixa.py` — contorno, furos e conectores de bordo contra o
  gerador da caixa (`caixa/cotuca_caixa_corte.py`), por courtyard. **É o
  verificador unificado que este fluxo pretende para a mecânica de bordo;
  enquanto não existe como um único script, o portão realiza-se pelos gates da
  skill `caixa` — `confere_caixa` e `confere_furos` (em `kicad/pcb_contorno.py`),
  `dentro_do_contorno.py`, e `kicad/encaixe.py` (M2/M4/M6/cabos).**

Mais `svg_colisoes.py` sobre o silk exportado, e a **varredura** (`kaudit` +
`conn` ou equivalente) como leitura independente. Os portões da pipeline usam
ainda **`scripts/manifesto_fontes.py`** (Estágio 2), **`scripts/congestao.py`**
(Estágio 3.5) e **`scripts/portao_humano.py`** (o gerador do questionário ⟨dB⟩).

---

## 5 · Portões: a pergunta que respondem, e a que NÃO respondem

> 🔴 **NENHUM portão desta lista mede MONTABILIDADE.** ERC, DRC,
> `funcao_dos_pads`, folgas e courtyards podem estar todos verdes com a caixa
> por montar — aconteceu na rev C, com nove sintomas (bts 15ga). A resposta a
> "isto monta-se?" vem do dB com as peças na mão, e o modelo do `ppc/003` é a
> única coisa que a antecipa. Ao reportar portões verdes, **dizer esta frase**;
> caso contrário o verde lê-se como aprovação de uma pergunta que ninguém fez.


Todo verificador entra nesta tabela com as duas colunas. Foi a coluna da
direita que faltou na rev B.

| verificador | responde | NÃO responde |
|---|---|---|
| ERC | tipos de pino compatíveis, rótulos soltos | direção de díodo, função de pino, D/S trocado |
| DRC | folgas, larguras, silk, courtyard×courtyard | courtyard×bordo (ativar), zona por preencher, pad sem rede |
| ratsnest 0 | tudo o que tem rede está ligado | pad **sem** rede (símbolo↔footprint desencontrados) |
| "sincronizado" | as peças batem | as **redes** batem — só se o sincronizador refrescar redes |
| `funcao_dos_pads` | função por pad da ficha | posição, roteamento, valores |
| `contrato_idc` | vias iguais entre placas | ordem do cabo do OLED (é do módulo) |
| `confere_furos` | courtyard fora do círculo de 3,5 | nada além de furos |
| kicad-happy | contagem e forma | leitura de significado ("50/50 têm MPN" eram descrições) |
| render / olho | o que os números não contam | nada que exija medir |

**Regra:** um portão que "passa" é aceite só depois de o ter visto **recusar**
sob sabotagem. Verde na primeira corrida é suspeito.

---

## 6 · Formato das mensagens

Numeração partilhada `NNN`, um por mudança de desenho, tudo no repo:

- `docs/ppc/NNN-titulo.md` — chat → Code. Contexto (2 linhas), passos com
  **prova por passo**, aceitação medida, o que não mexer.
- `docs/relatorio/NNN.md` — Code → chat. Fechou (com prova) · não fechou
  (porquê) · **decisões que precisam do humano** (marcadas) · commits.
- `docs/varredura/NNN.md` — chat, sobre os ficheiros, nas portas 2 e 3.
- `docs/roteiro/NNN.md` — chat → Cowork. Passos determinísticos, um por
  linha, com o estado esperado no fim de cada um.
- `STATUS.md` — a fila, uma página, o primeiro ficheiro de qualquer sessão.

Com Slack (Claude Tag em Team/Enterprise; Claude Code in Slack em Pro/Max): o
ppc é a mensagem, o relatório é a thread, o commit fica ligado. Os ficheiros
ficam na mesma — o Slack é transporte, o repo é memória.

**Nada que se decida vive só na conversa.** Se não está num destes ficheiros,
não aconteceu.

---

## 7 · Cicatrizes que sobrevivem (uma linha cada)

- **Símbolo genérico casado com footprint real: a netlist fica certa e a
  função fica errada; nenhum portão de ligação o vê.** (15C1–C5)
- **Um verificador responde à pergunta que lhe fizeram.** "Sincronizado" eram
  peças, não redes; "livre" eram pads, não corpos; "zona em F.Cu" era keepout.
- **"Perto" não é "ligado".** A via do D21 a 0,21 mm da pista: só o ratsnest
  do KiCad decide.
- **MPN sem abrir a ficha é palpite com número de peça.** `-4` vs `-1`,
  `1935161` vs `1715721`, `LF` que não é lead-free, `215` que são mA.
- **Facto de hardware sem fonte não entra em documento nenhum.** AGND/DGND
  "por dentro da Daisy" custou uma Seed3.
- **Lista à mão ao lado de coisa gerada envelhece em silêncio.** A tabela do
  IDC no roteiro, o `usados`, o cabeçalho de versão.
- **Cópia de emissor sobrevive às mudanças do original.** Chamar, não copiar.
- **Leitor que delimita por contagem de caracteres não é leitor.** Cinco
  sondas rápidas truncaram no mesmo dia; o definitivo delimita por parêntesis.
- **Data é dado gerado, não escrito.** Hook no Code; `date` no chat.
- **Mudança a meio do roteamento é a que custa.** Terceiro arcade, 4→2
  camadas, 6→3 mm: congelar antes, rev seguinte depois.
- **O render é a última verificação que vê o que os números não contam.**
  Encoder no ar, conector fora, silk no bordo — dez segundos numa imagem.
- **Não reconstruir a ferramenta que já existe para evitar aprender a usá-la.**
  Dezassete geradores para não clicar no KiCad.
- **«Fica para a pessoa» tem a mesma cara quando a placa está cheia e quando a
  malha não tem nó no vão.** O passo tem de dividir 1,27. (15eq)
- **Zeros idênticos em cima de tudo são leitor partido, não placa.**
  `SHAPE_LINE_CHAIN.Distance()` dá 0 dentro de uma cadeia fechada. (15er)
- **Canto arredondado: a caixa envolvente mente.** Courtyard contra o
  polígono do bordo, vértice a vértice. (15er)
- **«Existe» e «corre» são afirmações diferentes.** O `sincroniza_pcb` não
  tinha chamador: a cadeia nunca lia o esquema. (15es)
- **Guarda de nomes vê chaves; só a ficha vê significado.** Encoder
  apanhado por uma, MOSFET só pela outra. (15et)
- **A função de um conector está na peça do outro lado do cabo.** OLED com
  as sete posições trocadas e todos os portões verdes. (15eu)
- **Uma placa tem de ter por onde ser presa; o resto, a caixa cede.** O
  painel rev C saiu da fábrica sem um ponto de fixação. Trilha M do
  IFÁ10000, neste fluxo (IFÁ10000-M). (15ex)
- **Certo não é preciso.** Três revisões viram o `J4` e perguntaram se estava
  certo; era a tomada de uma ponte que já estava num soquete. Primeiro «o que
  liga aqui?». (15ez)
- **…e do cabo que existe.** Cabo directo: o contrato é a posição. Jumpers
  soltos: é o nome de cada fio. Antes de chamar divergência a outro ramo,
  ler o commit que a criou. (15ew)

---

## 8 · Primeiro uso

`ppc/001 — banco de prova`: copiar as duas placas da rev B **de antes das
correções** para `kicad/_lab/`, instalar a pilha (KiCad 10 + IPC, `kicad-python`,
Konnect, `kicad-mcp-kipy`, atopile, kicad-happy), correr cada revisor contra
os seis defeitos conhecidos, e devolver a tabela *seis defeitos × revisores*.
O que nenhum apanhar é o que continua a ser nosso — e é a lista final da §4.

---

## Referências (divulgação progressiva)

- `references/parts-sourcing.md` — a árvore de 3 níveis, o esquema do manifesto,
  as ferramentas (jlcparts, easyeda2kicad, JLC2KiCadLib), URLs e comandos; e o
  **§7 sourcing com Claude-on-Chrome** (o navegador do humano é a
  fonte-da-verdade de estoque: *Ships Now* ≥ qty, backorder/long-lead =
  indisponível).
- `references/fab-automation.md` — cheat-sheet do `kicad-cli`, tabela de
  capacidades JLC, tabela IPC-2221 de largura/via, regras `.kicad_dru`,
  freerouting; e a saída do portão: BOM **agrupada por LCSC**, o **CPL rico**
  companheiro, e o **kit de montagem** (`kit_montagem.py`).
- `references/montagem-casa.md` — a via de casa (chapa quente): o **portão de
  custo** (montagem >> nua+peças → casa), o fluxo passo-a-passo (nua+stencil,
  pasta Sn63/Pb37, refluxo do topo, bottom+THT à mão, lupa nas SOIC/SOT), e o
  bónus de tornar o risco de rotação da biblioteca da JLC moot.
- `references/quality-standards.md` — o catálogo de modos de falha (20 linhas) e
  os números duros (silk, costura, desacoplamento, courtyard/borda).
- **`/3ddesign`** (sub-skill) — a **certificação mecânica** placa↔caixa e o
  encaixe 3D volumétrico: a ordem de prioridades (a placa traz a fixação; a
  caixa cede no resto; a placa só cede com justificação assinada), as etapas
  M0–M10 com critério e âncora, e a ficha vivem em
  `.claude/skills/3ddesign/references/ifa10000-m.md`; o motor de medição (STEP
  real + `build123d`, as 6 asserções, o pré-voo) em `mech/verifica_3d.py` +
  `mech/params.py`. O Portão de Certificação Mecânica delega a ela; o plano é o
  ppc/003.
- `references/review-protocol.md` — a revisão tripla: 3 lentes A/B/C, 3
  passagens, revisores independentes/adversariais, o livro-razão, e o **registo
  de risco** (a saída de certificação: SAFE-EMBARK/MUST-FIX/decisão-dB, GO/NO-GO).
- `references/congestao.md` — o método do Estágio 3.5: as cinco métricas de
  densidade e as fórmulas, as duas recusas duras, o portão GO/REVER-COLOCAÇÃO, e
  as âncoras J14/C16 e J1/C5.
- `references/portao-humano.md` — o método dos portões ⟨dB⟩: os quatro campos por
  item, a regra "um item de cada vez, sem avanço em ambiguidade", os princípios
  anti-miscomunicação, e as 7 categorias que o questionário cobre sempre.
- `references/versionamento.md` — as **três camadas** de versão (rev da placa no
  silk `rev <LETRA><N>` POR PLACA · stamp do pacote = short SHA em `export/` ·
  semver dos scripts) e a **regra de ouro** (gerber/drill muda → rev sobe;
  tooling/doc/BOM-agrupamento → não sobe). O histórico de revisões vive em
  `docs/relatorio/002.md`.
- `references/grafismos-grid.md` — o método do Estágio 6 para **grafismos por
  GRID**: a causa-raiz (arrasto por olho gera loop e colisão com pad; o viewer de
  gerber engana porque não recorta silk sobre pad), os 6 passos (bbox declarada
  antes · aprovar lendo o grid · 1 edição só-silk → 1 regen · transfer
  gerber-truth), e a spec do helper `kicad/grid_grafismo.py`.
- `scripts/manifesto_fontes.py` — o portão STOP do Estágio 2 (`--sabota` prova
  que sabe dizer não).
- `scripts/congestao.py` — o portão do Estágio 3.5: orçamento de canais de
  escape + grelha de densidade de pinos, read-only, sai ≠0 em vermelho
  (`--sabota` prova que sabe dizer não).
- `scripts/portao_humano.py` — o gerador do questionário dos portões ⟨dB⟩ (CPL +
  BOM → checklist de 4 campos, read-only, determinístico; escreve o `VISTORIA.md`
  do pacote; `--sabota` prova que assinala as peças de risco conhecidas). Lê o
  `cpl_rico_*.csv` (a geometria rica que o CPL da JLC descarta).
- `scripts/bootstrap_kicad.sh` — a via AppImage do Estágio 0 quando o KiCad
  instalado falta ou é velho demais.
- `kicad/kit_montagem.py` — o kit de montagem em casa (v1.0.0): pasta do pacote →
  `kit/` (MAPA_COLOCACAO.md agrupado por valor com TOP/BOTTOM e bottom+THT
  sinalizados · COMPRA_LCSC.csv carrinho LCSC com Qty+spares · LEIA-ME-KIT.txt).
- `kicad/grid_grafismo.py` — o mapa do Estágio 6 para colocar grafismos por
  coordenada (v0.1.0, funcional): render da face de topo com grid de 0,5 mm +
  majors de 5 mm rotulados + eixos no frame do KiCad + zonas no-go sombreadas
  (pads+furos+courtyards+margem) + os maiores retângulos livres cotados em mm.
  Emite SVG sem dependências; exige o `pcbnew` do KiCad 10.

## Skills que esta orquestra

`kicad_layout` · `caixa` · `revisao-terra` · `design-review` · `pm` ·
`medicao-limpa` · `edicao-segura` · `3ddesign` · `audit` · e o modelo
`ifa10000`. O trabalho de esquema e verificação que era a antiga skill `/kicad`
(aposentada em 09/09/2026) entra por **este** fluxo; o layout e o roteamento
correm com `kicad_layout` (OBRIGATÓRIA no fecho de qualquer placa). **A
integração 3D caixa↔PCB** (IFÁ10000-M volumétrico + o pipeline ppc/003) corre na
sub-skill **`/3ddesign`**, orquestrada por este fluxo no Portão de Certificação
Mecânica. O braço executável de ponta a ponta é `kicad/faz_placa.py`
(esquema→pacote de fabrica numa ordem escolhida por medição) — o fluxo conduz por
cima dele, não o substitui.

## Anti-decadência

Corrida de placa nova que revele um buraco na pipeline **ganha estágio, portão
ou lente** — proposta, nunca em silêncio. A pipeline que não se actualiza
fabrica a placa de ontem.

**Estágio 9 — retrospetiva / melhoria do método** (corre depois da entrega):
cada defeito que a revisão tripla (ou a fábrica) apanhou torna o método melhor
na próxima corrida — todo defeito vira uma **verificação automática nova** ou
uma **edição de skill**. Delega a `audit` (coerência docs↔código↔skills),
`edicao-segura` (bump semver + reportar quais ficheiros mudaram), `pm` (quando
encomendar). **«O que nenhum portão apanhou vira portão novo.»** Um achado que
fica só como nota não fecha o Estágio 9; tem de sair como script, `.kicad_dru`,
passada de `revisao-terra`, ou lente de revisão — no mesmo commit.

## Histórico

- **v1.9.1 (2026-09-19)** — **extrai a integração 3D caixa↔PCB para a nova
  `/3ddesign`; delega o Portão Mecânico a ela.** A `references/ifa10000-m.md`
  migrou (`git mv`) para `.claude/skills/3ddesign/references/`; o Portão de
  Certificação Mecânica IFÁ10000-M passa a **delegar** à `/3ddesign` (como delega
  a `kicad_layout`/`caixa`), com o motor de medição 3D (STEP real via `kicad-cli`
  + `build123d`, `mech/verifica_3d.py` + `mech/params.py`) e o pipeline ppc/003 a
  viverem lá. `/3ddesign` entra na lista de skills orquestradas; o placeholder
  «(a criar)» sai — a sub-skill existe.
- **v1.9.0 (2026-09-19)** — **funde a orquestradora /pcb (v1.8.0) para dentro da
  /aipcb; /pcb aposentada.** A /aipcb passa a ser a skill canónica única: o seu
  frame de três portas + toda a máquina operacional da /pcb (os 9 estágios + a
  etapa 0 + o portão 3.5, o Portão de Certificação de Risco, o Portão de
  Certificação Mecânica IFÁ10000-M, a filosofia dos portões, a fronteira, o
  questionário do portão humano) folded sob as três portas. As 10 referências e
  os 4 scripts da /pcb migraram para `.claude/skills/aipcb/references/` e
  `.claude/skills/aipcb/scripts/`. As referências ao /kicad (aposentado) foram
  repointadas para este fluxo ou para `kicad_layout`. A integração 3D caixa↔PCB
  fica marcada para a futura `/3ddesign`. Entradas de histórico anteriores são da
  /pcb, mantidas como contexto:
- **v1.8.0 (2026-09-18)** — **certo não é preciso** (15ez). O `J4` do cérebro
  («a ponte ESP32») era a tomada da ponte por fio; a ponte passou a soquete a
  06/09 e ele ficou, e três revisões perguntaram se estava certo — a vistoria
  até pressupunha «o outro lado». Filosofia nova no topo; o `portao_humano`
  (v0.3.0) abre cada conector por «o que liga aqui?», com auto-teste; o
  `encaixe.py` (v0.2.0) ganha o **M1** — destinos declarados, e as duas
  assinaturas de um vestígio (dois caminhos para a mesma peça; tomada só de
  alimentação de um módulo em soquete) — com sabotagem. E o método ganha o
  passo 8: escrever o manual de montagem é uma revisão.
- **v1.7.0 (2026-09-18)** — **a montagem passa a ser critério de pronto.** A
  pergunta do dB — *«de que vale uma placa correta eletricamente se não podemos
  montá-la?»* — vira filosofia do topo e portão: `kicad/encaixe.py` (v0.1.0, só
  leitura; M2, M4, M6 pelo corpo, cabos; `--sabota`) chamado pelo `para_fabrica`
  v0.20.0, que agora também devolve a placa intacta quando recusa. A referência
  `ifa10000-m.md` ganha o **método de investigação** do dia (sete passos). E
  duas correcções de fonte: o `confere_arcade` passa a testar o **corpo** e não
  só os pads, e a tabela de alturas ganha a caixa de pinos IDC (8,85, ficha
  Würth) e os pontos de teste.
- **v1.6.0 (2026-09-18)** — **Portão de Certificação Mecânica, IFÁ10000-M**
  (`references/ifa10000-m.md`, nova): trilha do IFÁ10000 para o pareamento de
  qualquer placa com qualquer caixa — M0–M10 com critério e âncora, e ficha. A
  doutrina **inverte-se, por prioridade e não como lei** (pedido do dB): a placa
  traz os seus meios de fixação; com isso, **a caixa cede à placa** no resto; a
  placa só cede com justificação escrita e assinada. O estágio 3 deixa de tirar
  o contorno "do modelo da caixa" e a `caixa` deixa de ser a autoridade sobre a
  geometria: gera-a a partir da placa. Nasce do painel rev C, fabricado sem um
  único ponto de fixação.
- **v1.5.0 (2026-09-12)** — **Versionar TUDO.** `references/versionamento.md`
  ganha a secção «Versionar TUDO»: as três camadas (placa/pacote/script)
  generalizam-se para **todo artefato gerado que sai do repo** — cada um carrega
  um **carimbo** (`carimbo: <arquivo> v<N> · <data> · placas: …`) no seu formato
  (CSV `#`, MD comentário HTML, xlsx/xls célula dedicada, pacote no
  `LEIA-ME.txt`), e o novo `export/MANIFEST.md` é a **fonte única de "o mais
  novo"** com o protocolo entrega/consumo. Nasce da raiz real: um carrinho
  Mouser ficou com MPN de um **export superado** enquanto o repo já tinha o novo.
- **v1.4.0 (2026-09-12)** — três aprendizagens da corrida de sourcing pelo
  Mouser. **(1)** Estágio 2 ganha o §7 de `parts-sourcing.md`: sourcing com
  Claude-on-Chrome — o sandbox não alcança o Mouser (403 de egress), então o
  **navegador do humano é a fonte-da-verdade** de disponibilidade ("disponível" =
  Ships Now ≥ qty; backordered/long-lead = INDISPONÍVEL). **(2)** Estágio 6 ganha
  `references/grafismos-grid.md` (nova) e o helper `kicad/grid_grafismo.py`
  (v0.1.0): grafismo coloca-se por **coordenada de grid**, não por arrasto.
  **(3)** Reconciliação de MPN: `bom_completa.py` (v0.9.0) e `fichas.py`
  (v0.10.0) carregam os 7 MPNs finais verificados no Mouser.
- **v1.3.0 (2026-09-12)** — o versionamento passa a
  **CONGELAMENTO-NO-LANÇAMENTO** (`references/versionamento.md` reescrita): a rev
  sobe **por lançamento, não por commit** — enquanto uma placa não é
  fabricada/pedida, o gerber itera e a rev **fica** (`rev C`, sem número),
  distinguida pelo **stamp de commit**; a letra só sobe quando um board é
  efectivamente fabricado. **Silk é gerber**, mas mexer serigrafia numa placa não
  fabricada gera pacote novo, mesma rev.
- **v1.2.0 (2026-09-12)** — o **método de versionamento** entra na skill
  (`references/versionamento.md`, nova): as três camadas e a regra de ouro. O
  Estágio 8 ganha a **afirmação de versão** no portão, e o histórico de revisões
  passa a viver em `docs/relatorio/002.md`.
- **v1.1.0 (2026-09-11)** — Estágio 8 aprende a corrida real do cérebro/painel
  (stamp `6ebbda5`): a **via de montagem em casa** (chapa quente) com portão de
  custo (`references/montagem-casa.md`, nova), o **kit de montagem** como
  saída-padrão (`kit_montagem.py` v1.0.0), a **BOM agrupada por LCSC**
  (para_fabrica v0.17.0) e o **CPL rico** companheiro (v0.18.0), e no portão
  humano a **vistoria de rotação no preview 3D da JLC** + o **checklist de
  settings do pedido JLCONE**.
- **v1.0.0** — a pipeline de 9 estágios + etapa 0 + portão 3.5, orquestradora
  (nascida como /pcb).
