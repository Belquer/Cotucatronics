# Portão Humano — Questionário de Vistoria (o método do ⟨dB⟩)

Referência dos portões **⟨dB⟩** da pipeline `pcb` — os que não são scriptáveis e
por isso passam por um humano: o **sign-off da spec** (Estágio 1), o **veto de
render / dry-fit** (Estágio 3/7) e, sobretudo, a **inspeção pré-pagamento no
pré-visualizador 3D da JLC** (Estágio 8). Subordinada à filosofia dos portões da
SKILL.md — **um portão que avisa e deixa passar não é portão**, **medir no sítio
certo, ou o visto não vale nada**, **prova que a guarda sabe dizer não**.

> **A frase que governa este portão:** "confirmar rotações" não é um portão — é
> um convite ao mal-entendido. O humano e o script imaginam coisas diferentes e o
> erro passa. Um portão humano só é portão quando é um **questionário**: um item
> de cada vez, cada item ancorado num sítio concreto e num visual concreto, e
> **nenhum avanço sem um ✅ explícito**. A ambiguidade não é um "talvez" — é um
> **NÃO** que pára a fila.

## Índice

1. [O formato de quatro campos](#1-o-formato-de-quatro-campos)
2. [A regra do portão: um item de cada vez, sem avanço em ambiguidade](#2-a-regra-do-portão)
3. [Princípios anti-miscomunicação](#3-princípios-anti-miscomunicação)
4. [As categorias que o questionário cobre sempre](#4-as-categorias)
5. [O gerador `scripts/portao_humano.py`](#5-o-gerador)
6. [Onde o portão corre](#6-onde-o-portão-corre)

## 1. O formato de quatro campos

Cada item apresenta-se **sozinho** e traz um cabeçalho (**ITEM**) mais **quatro
campos**, nesta ordem exacta:

- **ITEM** — designador + peça + pacote (ex.: `D2 — SS14 (D_SMA)`). Nunca "os
  díodos"; sempre a peça nomeada.
- **ONDE OLHAR** — o sítio exacto: qual vista do pré-visualizador 3D da JLC
  (rodar para topo/baixo), qual camada de gerber, e a **coordenada** X/Y da peça
  mais o **valor de rotação do CPL**. O olho tem de chegar ao pixel certo sem
  adivinhar.
- **O QUE DEVES VER** — o estado correcto, em concreto: a **banda/cátodo**, o
  **ponto de pino-1**, a perna **+/−**, o **pinout**. O que a peça certa,
  orientada certa, mostra ali.
- **COMO CONFIRMAR** — o que é um **PASS** e o que é um **FAIL**, ambos escritos.
  O FAIL nomeia a consequência ("invertido 180° → o díodo conduz ao contrário") e
  a rota ("corrigir a rotação no CPL/footprint, NÃO pagar").
- **RESPOSTA** — `☐ ✅ conforme` / `☐ ❌ + o que vês`. O ❌ obriga a escrever **o
  que se viu** — porque um ❌ mudo não encaminha para conserto nenhum.

## 2. A regra do portão

- **Um item de cada vez.** Apresenta-se o item, espera-se a RESPOSTA, e só então
  se passa ao seguinte. Uma lista despejada de uma vez é lida na diagonal — e é
  na diagonal que o 180° passa.
- **Só ✅ avança.** Qualquer **❌**, ou qualquer **ambiguidade** ("acho que sim",
  "não dá para ver bem"), **PÁRA** o questionário e encaminha para um **conserto**
  (corrigir rotação/pinout/fonte, ou reordenar o conector). Nunca para o
  pagamento.
- **O passe limpo é a precondição do pagamento.** Só um questionário inteiro em
  ✅ autoriza o humano a pagar — e **o pagamento continua a ser acção do humano**,
  não do script. O portão autoriza; não paga.
- **O ❌ é um achado, não um incómodo.** Fica escrito (o que se viu), e o conserto
  fecha-o. Resultado negativo é achado — o que passou fica registado.

## 3. Princípios anti-miscomunicação

Toda a ambiguidade que este portão remove vem de uma âncora em falta. As regras:

- **Ancora sempre no concreto:** designador + coordenada + valor de rotação do
  CPL + visual esperado. Nunca um "confere as peças" ou "vê se as rotações estão
  bem".
- **O CPL não é verificável por script:** a rotação no CPL é a do **footprint**
  (KiCad); a JLC monta pela rotação da **biblioteca dela**. O perigo dos **180°**
  (biblioteca JLC vs KiCad) só se vê no pré-visualizador 3D — por isso o portão é
  humano.
- **Diz o PASS e o FAIL, os dois.** Um campo que só descreve o estado certo deixa
  o revisor sem saber o que é uma reprovação. O FAIL nomeia a consequência física.
- **Nome do desenho ≠ nome da compra.** Quando o `Value` e o MPN/LCSC são a mesma
  peça com designações diferentes (ex.: `SS14` ↔ `1N5819`), o item **assinala-o** —
  senão a serigrafia e a encomenda falam de coisas que parecem diferentes.
- **Polaridade e orientação são perguntas distintas.** A banda/cátodo (a peça
  conduz ao contrário) e o pino-1/rotação (o pinout todo troca) verificam-se cada
  uma no seu item, com o seu FAIL.
- **A render do NOSSO board é a fonte-de-verdade; o preview 3D da JLC não prova a
  rotação da biblioteca dela** (lição de 2026-09-11). O pré-visualizador 3D da
  JLCONE desenha o **modelo 3D da peça na biblioteca DELES** por cima do nosso
  footprint — e uma "inversão" que ali aparece é, muitas vezes, **artefacto do
  modelo 3D** (o modelo roda vs. o nosso footprint), **não** um defeito da placa.
  Não se conclui do preview sozinho: **confirma-se** comparando a **banda de cátodo**
  ou o **canto do pino-1** do preview da JLC contra a **render do nosso próprio
  board** (gerar crops por peça se preciso). E há uma **rede de segurança** que não
  depende de acertar o olho: pedir **Confirm Parts Placement = YES** — a JLC manda
  a folha de colocação para tu aprovares antes de montar. Só as **polarizadas /
  pino-1** são críticas aqui (díodos SMA/SMB com a banda de cátodo à esquerda,
  SOT-23 com a perna única, SOIC com o triângulo do pino-1 no canto top-left); os
  **passivos são simétricos** e não gastam olho.

## 4. As categorias

O questionário é **auto-derivado da placa** (do CPL + BOM) e cobre **sempre**
estas categorias, por esta ordem — e **abre SEMPRE pelo Item 0** (carregar o
pacote) antes de qualquer item de inspeção:

0. **Carregar o pacote certo (antes de tudo)** — qual ficheiro carregar na
   JLCPCB/JLCONE e **como**, antes da inspeção começar. O questionário não pode
   assumir que a placa já está carregada — este é o primeiro item de todo o
   questionário:
   - **Gerber a carregar:** o `.zip` carimbado exacto,
     `export/<placa>_<data>_<commit>/<placa>_<data>_<commit>_gerbers.zip` — é este
     que entra na JLCONE (*Add Gerber*), não o `.kicad_pcb` nem gerbers soltos.
   - **Como o buscar:** o pacote está **committado no repo, na branch do PR** — vive
     no **GitHub, não necessariamente na máquina do humano**. ⚠️ O terminal da
     pipeline corre no **sandbox na nuvem** e **não põe ficheiros na máquina do
     humano**: a recuperação é um comando que **o humano corre localmente**. O
     caminho preferido é o terminal (`git fetch` / `git checkout <branch>` /
     `git pull` e abrir a pasta `export/…`, ou `git clone -b <branch> …`); o
     **fallback** é o GitHub no browser (página do `.zip` → **Download raw file**,
     ou **Code → Download ZIP** da branch).
   - **Proveniência:** abrir o `LEIA-ME.txt` do pacote e confirmar que carimba o
     **commit** certo e **árvore limpa** — é ESSA a revisão que se encomenda (guarda
     contra encomendar um pacote velho ou trocado). A proveniência lê-se no
     `LEIA-ME.txt`, não no CPL.
   - **Montagem SMT:** quando a JLC pedir, juntar `cpl_<placa>.csv` (posições) +
     `bom_<placa>.csv` (BOM), da MESMA pasta.
   - **Camadas:** 2.
1. **Nível-placa** — nº de camadas, dimensões, contorno vs caixa, quantidade a
   encomendar. (⚠️ pedir 4 camadas num desenho de 2 paga um plano interior a mais.)
2. **Peças polarizadas** — díodos, electrolíticos, tântalo, LEDs, ICs de pino-1:
   cada uma, a **banda/cátodo/pino-1** vs a serigrafia.
3. **Pacotes com risco de rotação** — SOT-23, SOIC/SOP, SON/QFN, SMA/SMB/SOD,
   electrolítico, módulos, conectores: confirmar a **orientação no 3D vs o pino-1
   pretendido** (o perigo dos 180° da biblioteca JLC vs KiCad).
4. **Conectores / contratos de pinagem** — OLED vs módulo (perigo VCC/GND e
   DIN/CLK trocados com cabo 1:1), IDC placa-a-placa (26/26, pino-1 alinhado),
   polaridade de alimentação.
5. **BOM / montagem** — toda linha SMT com LCSC em stock **de montagem** (≠ stock
   na LCSC); THT (pots, radiais) enviados à parte; DNP correcto.
6. **Mecânica** — conectores/botões/display/pots/encoder alcançam os recortes do
   painel (dry-fit vs render da caixa).
7. **Confirmação final antes de pagar** — o passe limpo de tudo acima, a
   precondição do pagamento.

### 4a. Checklist de settings do pedido JLCONE (parte do portão, antes de pagar)

A vistoria de rotação não é a única coisa que se confere na JLCONE — os **settings
do pedido** são portão a par das peças (lição de 2026-09-11). Antes de pagar,
confirmar, um a um:

- **Camadas: 2.** (⚠️ pedir 4 num desenho de 2 paga um plano interior a mais.)
- **Via Tented — CRÍTICO.** As nossas laranjas "mask-exposing-trace" (§DFM) só são
  benignas **com a via coberta**. Se a via ficar destapada, deixam de ser benignas.
  Confirmar **Via Tented** ligado.
- **Acabamento: ENIG.**
- **Assembly → Confirm Parts Placement = YES.** A rede de segurança da rotação (§3):
  a JLC manda a folha de colocação para aprovares antes de montar.
- **Remark das peças bottom-side / hand-solder** — sinalizar as peças do fundo ou a
  soldar à mão (ex.: `C16` no cérebro) na nota do pedido.
- **Top vs Bottom Designator no matching de peças.** No casamento de peças, a JLCONE
  separa **listas Top e Bottom**. Conferir que as peças do fundo (ex.: `C16`)
  aparecem na lista **Bottom** — e não sumiram por a lista Top ser a única que
  olhaste.

## 5. O gerador

`scripts/portao_humano.py` (read-only, determinístico) constrói o questionário a
partir do **CPL + BOM** da placa (opcionalmente o `.kicad_pcb` para coordenadas),
no formato de quatro campos, cobrindo as categorias **0 a 7** — abrindo sempre
pelo **Item 0** (carregar o pacote):

```bash
python3 scripts/portao_humano.py --cpl <cpl.csv> --bom <bom.csv> \
        --placa "<nome>" --camadas "2 camadas" \
        --pacote "<placa>_<data>_<commit>" --branch "<branch-do-PR>" \
        --out export/<placa>_<data>_<commit>/VISTORIA.md
python3 scripts/portao_humano.py --sabota      # prova que assinala o risco conhecido
```

O `--pacote` (ou, se omitido, o nome da pasta do `--out`) dá ao **Item 0** o
caminho exacto do `.zip` a carregar; o `--branch` diz de que branch do PR o
buscar. O `--sabota` também exige que o **Item 0** apareça e nomeie um `.zip`.

Auto-lista as peças polarizadas (por heurística de valor/footprint + BOM), os
pacotes com risco de rotação (por nome de footprint: SOT-23/SOIC/SOP/QFN/SON/
SMA/SMB/SOD/electrolítico/módulo), os conectores/contratos-de-pinagem, e os
passos de nível-placa + confirmação final. O `--sabota` corre sobre uma placa
sintética com defeitos plantados e **exige** que cada classe conhecida (SOT-23,
SMB/SMA cátodo, SOIC pino-1, OLED, IDC, alias de nome) apareça — uma guarda que
nunca assinalou nada é promessa, não verificação.

O artefacto por placa é o `VISTORIA.md`, escrito **dentro do pacote de fábrica**
(`export/<placa>_<data>_<commit>/`), ao lado do CPL/BOM/gerbers a que se refere.

## 6. Onde o portão corre

O mesmo método serve os três portões ⟨dB⟩ da pipeline:

- **Estágio 1 — sign-off da spec:** o diagrama e a spec correm o questionário na
  sua forma de nível-placa e de contratos (categorias 1 e 4) antes de se gastar
  em peças.
- **Estágio 3/7 — veto de render / dry-fit:** a categoria 6 (mecânica) é o veto
  do dry-fit de UMA caixa antes da série.
- **Estágio 8 — inspeção pré-pagamento na JLC:** o questionário completo, item a
  item, no pré-visualizador 3D — **a abrir pelo Item 0 (carregar o pacote
  carimbado + proveniência)**, porque a inspeção não pode assumir a placa já
  carregada. **Só o passe limpo autoriza o pagamento.**

Sem o ⟨dB⟩ em ✅, o estágio fica em `INCOMPLETA` — nunca se confunde com feito.
