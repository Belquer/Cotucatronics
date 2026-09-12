# versionamento — três camadas, uma regra de ouro

> **O que muda o artefacto fabricado tem de aparecer na placa.** Um pacote que sai
> com o silk a dizer uma revisão e o gerber a dizer outra é um pacote que mente
> sobre si mesmo — e é a fábrica, ou o dB com a placa na mão, quem paga a mentira.

Este projecto versiona em **três camadas independentes**, cada uma a responder
uma pergunta diferente. Confundi-las é o erro: subir o semver de um script não é
subir a revisão da placa, e carimbar um commit não é dizer que o cobre mudou.

## As três camadas

### 1. Rev da placa, no silk — `rev <LETRA><N>`, POR PLACA

Vive na serigrafia da própria placa (`gr_text "cerebro rev C1"`,
`gr_text "painel rev C"`), escrita pelo gerador (`kicad/pcb_contorno.py`,
função `marca`). É a identidade do **artefacto físico**.

- **Letra** = **geração fabricada**. Sobe quando um spin físico foi pedido e
  começa o design que o vai substituir — a letra nova nasce com a decisão de
  mandar cortar a anterior, não quando a nova chega.
- **Número** = **iteração de design dentro da geração**. Incrementa a **CADA**
  mudança que altera o **gerber/drill**. Sem número = a **1ª** da letra (`rev C`
  ≡ `rev C1` implícito; o `C1` explícito marca a segunda mexida em cobre/silk da
  geração C).
- É **POR PLACA**. O cérebro e o painel evoluem independentes: o painel pode ir a
  `rev C1` enquanto o cérebro fica em `rev C` — cada um carrega a sua verdade.

### 2. Stamp do pacote — short SHA do commit, em `export/`

Cada pacote de fábrica vive em `export/<placa>_<data>_<commit>/`, e o `<commit>`
é o **short SHA** do git no estado de que o pacote saiu (`para_fabrica` carimba
`git rev-parse --short HEAD` + a data + o aviso de árvore-suja). É a
**procedência**: diz de que ponto exacto do repo os gerbers foram gerados, e é
recuperável do PR. Duas placas com a mesma rev de silk podem ter stamps
diferentes se algo à volta mudou — o stamp é do **pacote**, a rev é da **placa**.

### 3. Semver dos scripts — o pipeline

Cada ferramenta do pipeline (`para_fabrica`, `pcb_contorno`, `faz_placa`,
`kit_montagem`, …) carrega `VERSAO = "X.Y.Z"` e bumpa segundo a `edicao-segura`
(MAJOR incompatível · MINOR adição compatível · PATCH correcção sem mudança de
comportamento). É a versão do **método**, não da placa. Melhorar o gerador não
sobe a rev da placa; só sobe se a saída do gerador **mudar o gerber**.

## A regra de ouro

> **Se o gerber/drill muda, o rev da placa sobe.**
> **Mudança só de tooling/doc/BOM-agrupamento — que não toca o artefacto
> fabricado — NÃO sobe o rev da placa.**

Casos, para não ficar por fé:

| mudança | gerber/drill muda? | rev da placa | stamp | semver script |
|---|---|---|---|---|
| reordenar pinos de um conector, mover cobre | **sim** | **sobe** | novo | — |
| mover a marca de B.SilkS → F.SilkS, novo símbolo | **sim** (silk é gerber) | **sobe** | novo | do gerador, se mudou |
| agrupar a BOM por LCSC, escrever CPL rico | não | **fica** | novo (é outro commit) | sobe |
| corrigir um comentário/doc, um teste, uma skill | não | **fica** | (não gera pacote) | sobe |
| re-preencher zonas sem alterar geometria | não | **fica** | novo se re-empacotar | — |

⚠️ **Silk É gerber.** Mover, acrescentar ou apagar serigrafia muda o ficheiro
`.GTO`/`.GBO` que a fábrica imprime — logo sobe a rev, mesmo que o cobre não se
mexa. A rev C1 do cérebro (marca para o topo + búzios) subiu **por causa do
silk**, com o cobre provado idêntico (fingerprint de cobre/rede igual, 0
alterações) — é exactamente o caso "gerber muda, rev sobe" sem tocar em cobre.

## Onde vive o histórico

O **histórico de revisões** — cada rev, o que mudou, e o commit — vive em
**`docs/relatorio/002.md`**, na secção «Histórico de revisões». Uma linha por
bump, na forma **rev → o que mudou → commit**. É lá que se lê a saga de cada
placa; o silk diz *qual* rev, o 002 diz *porquê* e *de onde*.

## Parte do Estágio 8 / do portão

Emitir um pacote **afirma** que o rev do silk bate com a **classe da mudança**:

- Se este pacote mudou o gerber/drill face ao pacote anterior da mesma placa, o
  silk **tem** de trazer uma rev **superior** — senão o pacote está a dizer que
  nada mudou quando mudou. `para_fabrica` não corre sem o registo de risco em GO,
  e o portão humano (`portao-humano.md`) abre com a proveniência pelo LEIA-ME: é
  aí que se confere, item a item, que a rev carimbada no pacote e a rev impressa
  no silk são a mesma, e que ela subiu se o gerber subiu.
- Se nada no gerber mudou (só tooling/doc), **não** se sobe a rev nem se finge um
  pacote novo por vaidade — reempacota-se com stamp novo se for preciso, mas a
  rev da placa fica.

A pergunta que o Estágio 8 faz antes de deixar pagar: **«o silk desta placa diz a
verdade sobre o que este gerber tem de diferente do último?»** Se não disser, o
estágio não fecha.
