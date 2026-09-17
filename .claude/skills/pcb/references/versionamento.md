# versionamento — três camadas, uma regra de ouro (congelamento-no-lançamento)

> **O que muda o artefacto fabricado tem de aparecer na placa.** Um pacote que sai
> com o silk a dizer uma revisão e o gerber a dizer outra é um pacote que mente
> sobre si mesmo — e é a fábrica, ou o dB com a placa na mão, quem paga a mentira.
>
> **⚠️ CONGELAMENTO-NO-LANÇAMENTO (2026-09-12).** A rev NÃO sobe a cada commit que
> mexe no gerber. Sobe **por LANÇAMENTO**: enquanto uma placa não é **fabricada /
> pedida**, ela fica no **mesmo rev**, e as iterações pré-fabricação **partilham
> esse rev** — distinguem-se pelo **stamp de commit** do pacote, não por um número
> novo. O número/letra só sobe quando um board é **efectivamente FABRICADO/PEDIDO**;
> aí esse rev fica congelado e a **próxima** mudança de gerber abre o rev seguinte.

Este projecto versiona em **três camadas independentes**, cada uma a responder
uma pergunta diferente. Confundi-las é o erro: subir o semver de um script não é
subir a revisão da placa, e carimbar um commit não é dizer que o cobre mudou.

## As três camadas

### 1. Rev da placa, no silk — `rev <LETRA><N>`, POR PLACA

Vive na serigrafia da própria placa (`gr_text "cerebro rev C"`,
`gr_text "painel rev C"`), escrita pelo gerador (`kicad/pcb_contorno.py`,
função `marca`). É a identidade do **artefacto físico**.

- **Letra** = **geração**. É o rev "em aberto" enquanto a placa não foi
  fabricada. Todas as iterações pré-fabricação (correcções, reposicionamentos,
  mudanças de silk ou de cobre) **ficam nesta mesma letra** — o silk diz
  `rev C`, sem número, e o que as separa é o **stamp de commit** do pacote.
- **Quando sobe** = **só no lançamento**. Assim que uma placa `rev C` é
  **fabricada/pedida**, esse rev fica **congelado** (o artefacto físico que
  existe no mundo é `rev C`); a **próxima** mudança que altere o gerber/drill
  abre o rev **seguinte** (`rev D`). O bump é a decisão de fabricar, não um
  commit qualquer.
- **Número** = opcional, e só faz sentido para distinguir **spins fabricados**
  dentro de uma geração; enquanto não há fabrico, não se usa número (`rev C`, não
  `rev C1`). O importante é: **não há um número novo por commit.**
- É **POR PLACA**. O cérebro e o painel evoluem independentes: um pode já ter
  sido fabricado e estar congelado enquanto o outro continua em aberto — cada um
  carrega a sua verdade.

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

> **A rev de um artefacto FABRICADO tem de ser rastreável — mas por LANÇAMENTO,
> não por commit.**
> **Enquanto a placa não foi fabricada/pedida, o gerber pode mudar N vezes e a rev
> FICA** (as iterações distinguem-se pelo stamp de commit do pacote).
> **A rev só sobe quando um board é efectivamente fabricado/pedido: aí congela, e a
> mudança seguinte de gerber abre o rev a seguir.**

O que continua a valer: um pacote nunca pode dizer no silk uma coisa e no gerber
outra. O que MUDOU: "gerber mudou" já **não** obriga a subir o número — obriga só
a um **stamp de commit** novo, dentro do mesmo rev, até haver um fabrico.

Casos, para não ficar por fé:

| mudança | gerber/drill muda? | placa já foi fabricada nesta rev? | rev da placa | stamp | semver script |
|---|---|---|---|---|---|
| corrigir cobre/silk numa placa **ainda não fabricada** | sim | não | **fica** (mesma letra) | **novo** | do gerador, se mudou |
| mover/escalar a marca numa placa ainda não fabricada | **sim** (silk é gerber) | não | **fica** | **novo** | do gerador, se mudou |
| primeira mudança de gerber **depois** de a placa ter sido fabricada | sim | sim (congelada) | **sobe** (letra a seguir) | novo | — |
| agrupar a BOM por LCSC, escrever CPL rico | não | — | **fica** | novo (é outro commit) | sobe |
| corrigir um comentário/doc, um teste, uma skill | não | — | **fica** | (não gera pacote) | sobe |
| re-preencher zonas sem alterar geometria | não | — | **fica** | novo se re-empacotar | — |

⚠️ **Silk É gerber** — mover/acrescentar/apagar serigrafia muda o `.GTO`/`.GBO`
que a fábrica imprime. Mas sob congelamento-no-lançamento isso **não** sobe a rev
enquanto a placa não foi fabricada: gera um **pacote novo (stamp novo), mesma
rev**. Exemplo: o cérebro e o painel ficaram ambos em **`rev C`** ao longo de
várias iterações pré-fabricação (reordenação do J2 do painel, reposicionamento +
escala da marca do cérebro) — cada uma com o seu stamp de commit, todas `rev C`,
porque nenhuma placa foi ainda mandada cortar. O cobre foi provado idêntico
(fingerprint de cobre/rede igual, 0 alterações) nas iterações que só mexeram silk.

## Onde vive o histórico

O **histórico de revisões** — cada rev, o que mudou, e o commit — vive em
**`docs/relatorio/002.md`**, na secção «Histórico de revisões». Uma linha por
bump, na forma **rev → o que mudou → commit**. É lá que se lê a saga de cada
placa; o silk diz *qual* rev, o 002 diz *porquê* e *de onde*.

## Parte do Estágio 8 / do portão

Emitir um pacote **afirma** que o rev do silk bate com o **estado de fabrico** da
placa:

- Enquanto a placa **não foi fabricada**, iterar o gerber é normal e **não** sobe
  a rev: emite-se um **pacote novo (stamp novo), mesma rev**. O que rastreia a
  iteração é o stamp de commit, não um número. `para_fabrica` não corre sem o
  registo de risco em GO, e o portão humano (`portao-humano.md`) abre com a
  proveniência pelo LEIA-ME: confere-se, item a item, que a rev carimbada no
  pacote e a rev impressa no silk são a **mesma** — e que essa é a rev **em
  aberto** da placa.
- **No momento de fabricar/pedir**, a rev em aberto congela: é essa a rev do
  artefacto físico que vai existir. A **próxima** mudança de gerber, depois disso,
  **tem** de subir a rev — senão um pacote novo estaria a reutilizar a rev de uma
  placa que já está cortada, e dois artefactos físicos diferentes partilhariam a
  mesma identidade.

A pergunta que o Estágio 8 faz antes de deixar pagar: **«esta rev já foi
fabricada? Se sim, este gerber tem de trazer a rev seguinte; se não, mantém-se a
rev em aberto e o que distingue este pacote é o stamp.»** Se a resposta não bater
com o silk, o estágio não fecha.

## Versionar TUDO — todo artefato gerado carrega carimbo (2026-09-12)

As três camadas acima cobrem a **placa**, o **pacote** e o **script**. Faltava a
generalização: **todo artefato que sai do repo para ser consumido lá fora**
— não só os pacotes de fábrica — tem de **declarar a própria versão**, para que
ninguém consuma uma cópia velha sem perceber.

> **Raiz do problema (2026-09-12):** o carrinho do Mouser do dB ficou com um MPN
> **de um export superado**, enquanto o repo já tinha o novo. Nenhum dos dois lados
> dizia "sou a versão X" de forma comparável — e um artefato que não se versiona
> não deixa detetar que envelheceu. Um export que não se carimba é um pacote que
> mente sobre si mesmo, exactamente como o silk que discorda do gerber.

**A regra, em três partes:**

1. **Carimbo no próprio artefato.** Todo ficheiro gerado carrega, no início e num
   lugar válido para o seu formato, um carimbo:

   ```
   carimbo: <arquivo> v<N> · <YYYY-MM-DD> · placas: painel rev C · cerebro rev C
   ```

   - **CSV** → primeira linha como comentário `#`.
   - **Markdown** → comentário HTML `<!-- ... -->` no topo (ou um bloco visível).
   - **xlsx/xls** → uma **célula dedicada** (`carimbo` numa aba própria e/ou uma
     célula fora das colunas que a ferramenta importa — nunca na coluna A dos dados,
     senão vira uma "peça" fantasma na importação).
   - **Pacote de fábrica** → já o faz pelo `LEIA-ME.txt` (gerador + data + commit).

   O `v<N>` começa em `v1` e **sobe a cada novo corte** que muda o conteúdo
   consumível (um MPN trocado, uma qty recalculada). Não é o SHA deste commit
   (auto-referência impossível) — a **procedência** fina vem do MANIFEST + git.

2. **O MANIFEST é a fonte única de "o mais novo".** `export/MANIFEST.md` lista
   **todo** deliverable corrente com o seu carimbo. É onde se responde "estou com
   a versão de agora?": compara-se o carimbo do artefato em mão contra o MANIFEST;
   se não bater, `git pull`.

3. **Protocolo de entrega/consumo.** Quando **eu entrego** um ficheiro, **declaro o
   carimbo** ("este é o `COMPRA_MOUSER.csv` v2, 2026-09-12"). Quando **o humano vai
   consumir** (subir ao Mouser, mandar à fábrica, montar), **confere o carimbo**
   contra o MANIFEST **antes** de agir. As duas metades existem porque a saída de
   uma ferramenta não prova que o insumo estava actual — o carimbo, sim.

É a mesma verdade das três camadas, alargada: **congelamento-no-lançamento** diz
que a rev da placa só sobe no fabrico; **Versionar TUDO** diz que *qualquer*
artefato consumível se carimba e se confere contra o MANIFEST antes de virar
compra, upload ou solda.
