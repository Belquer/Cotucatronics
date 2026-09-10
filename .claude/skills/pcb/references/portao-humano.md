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

## 4. As categorias

O questionário é **auto-derivado da placa** (do CPL + BOM) e cobre **sempre**
estas sete categorias, por esta ordem:

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

## 5. O gerador

`scripts/portao_humano.py` (read-only, determinístico) constrói o questionário a
partir do **CPL + BOM** da placa (opcionalmente o `.kicad_pcb` para coordenadas),
no formato de quatro campos, cobrindo as sete categorias:

```bash
python3 scripts/portao_humano.py --cpl <cpl.csv> --bom <bom.csv> \
        --placa "<nome>" --camadas "2 camadas" --out VISTORIA.md
python3 scripts/portao_humano.py --sabota      # prova que assinala o risco conhecido
```

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
  item, no pré-visualizador 3D. **Só o passe limpo autoriza o pagamento.**

Sem o ⟨dB⟩ em ✅, o estágio fica em `INCOMPLETA` — nunca se confunde com feito.
