---
name: pcb
description: Pipeline de ponta a ponta de um prompt até gerbers prontos para a JLCPCB — leva uma ideia/spec de placa por 9 estágios com portão em cada um até um pacote de fabricação que a fábrica aceita. Use em "novo PCB", "prompt to board", "design a board", "leva isto até aos gerbers", "/pcb", ou qualquer pedido que vá de uma ideia/especificação a uma placa fabricável. NÃO use para editar uma única folha de esquema já existente — isso é a skill `kicad`. Esta é a ORQUESTRADORA: chama `kicad`, `kicad_layout`, `caixa`, `revisao-terra`, `design-review`, `pm`, `para_fabrica` — nunca reimplementa o que elas já fazem.
---

# pcb — do prompt aos gerbers, com portão em cada estágio

> **9 estágios (1–9) mais a etapa 0 de pré-voo** (modelo do `ifa10000`):
> 0 pré-voo · 1 avaliação+diagrama · 2 fontes · 3 colocação vs caixa · 4
> vias+pistas · 5 terra · 6 rótulos · 7 revisão tripla · 8 gerbers · 9
> retrospetiva. Cada um passa com um número que pode dizer **não**.

Esta skill é a **orquestradora**. Não desenha esquemas, não roteia, não corta
caixas: **delega** a quem já sabe e cobra o portão de cada etapa. O corpo
executável de metade dos estágios já existe em `kicad/faz_placa.py` e nas skills
irmãs — a regra número um é **chamar, nunca reescrever**.

## Filosofia dos portões

Toda esta pipeline é subordinada a cinco frases que as outras skills compraram
com tempo perdido:

- **Um portão que avisa e deixa passar não é portão** (`caixa`). Cada estágio
  tem de conseguir dizer *não* e parar a fila.
- **Prova que a guarda sabe dizer não** (`funcao_dos_pads.py --sabota`). Uma
  guarda que nunca reprovou é uma promessa, não uma verificação.
- **O gerador é a fonte** (`kicad`). Cota escrita à mão ao lado da calculada
  envelhece sempre; importa-se, não se copia.
- **Medir no sítio certo, ou o visto não vale nada** (`kicad_layout`). Um
  verificador que mede o ponto errado aprova tudo com ar de ter conferido.
- **Resultado negativo é achado** (`revisao-terra` R6). O que passou fica
  escrito, ou a próxima revisão refaz o trabalho.

## Decisões que precisam do dB

Alguns portões **não são scriptáveis** — são humanos, e marcam-se com **⟨dB⟩**.
A pipeline pára e espera aprovação antes de gastar dinheiro:

- **⟨dB⟩ Estágio 1** — o diagrama legível e a spec, aprovados **antes** de gastar
  em peças.
- **⟨dB⟩ Estágio 8** — a rotação peça-a-peça no pré-visualizador da JLC, antes
  de pagar. O CPL não é verificável por script (a rotação é a do footprint, a
  JLC espera a da biblioteca dela).
- **⟨dB⟩ Estágio 3/7** — o VETO dos renders de placa, e a montagem a seco de UMA
  caixa antes da série.

Sem o ⟨dB⟩, o estágio fica em `INCOMPLETA` — nunca se confunde com feito.

## Fronteira — quem responde o quê

| pergunta | quem |
|---|---|
| o esquema liga? o ficheiro abre? ERC? | `kicad` |
| placa, cobre, planos, serigrafia, pacote de fabrica | `kicad_layout` |
| a caixa fecha, a placa cabe, os furos batem | `caixa` |
| a massa está num estado legal | `revisao-terra` |
| o desenho lê-se | `design-review` |
| ordem de encomendas / caminho crítico | `pm` |
| toda edição de ficheiro que a pipeline faça | `edicao-segura` |

Esta skill não decide nada de electrónica: **decide a ORDEM, cobra o PORTÃO de
cada estágio, e recusa avançar com um estágio reprovado.**

---

## Os 9 estágios (+ a etapa 0)

Para cada um: **objetivo · o que faz · delega a · PORTÃO · artefacto**. O detalhe
mora nas referências (`references/`) — aqui fica só o suficiente para conduzir.

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
  descasamento **PÁRA a pipeline** (lição desta sessão). E `kicad-cli sch erc` /
  `pcb drc` têm de correr sem erro de carregamento: *"o checker passou" ≠ "a
  folha abre"*. Netlist limpo não é ficheiro que abre.
- **Artefacto:** linha de versão (`kicad-cli version`) + token dos ficheiros, e
  o exit-code do primeiro `erc`/`drc`, no relatório da sessão.

### Estágio 1 — Avaliação + diagrama legível

- **Objetivo:** transformar o prompt numa spec e num diagrama que um humano
  aprova antes de se gastar em peças.
- **O que faz:** prompt → spec (função, trilhos de alimentação, conectores,
  quantidades, restrições mecânicas), e um **diagrama de blocos legível**.
- **Delega a:** `kicad` (o que todo esquema declara: tensão+fonte, por-canal vs
  partilhado, afinação por unidade, massas distintas com símbolos distintos,
  premissas); `design-review` (o diagrama passa por `tools/svg_colisoes.py`
  antes do olho).
- **PORTÃO:** a spec declara os seis itens do `kicad`; o diagrama passa
  `design-review` (exit 0/1/3, 3=não-verificável não é aprovado); **⟨dB⟩** um
  sign-off marcado **antes** de gastar em peças.
- **Artefacto:** spec + diagrama commitados, com o resultado do `svg_colisoes` e
  o ⟨dB⟩ registados.

### Estágio 2 — Fontes de componentes (o maior buraco)

- **Objetivo:** toda linha do BOM resolvida numa peça que a JLC monta mesmo.
- **O que faz:** aplica a árvore de **3 níveis** — (1) equivalente na biblioteca
  de montagem da JLC → (2) importar/converter a peça exacta
  (`easyeda2kicad`/`JLC2KiCadLib`) → (3) construir do datasheet (IPC-7351
  Nominal). Ver **`references/parts-sourcing.md`** para a árvore, o checklist de
  equivalência, e a tabela de ferramentas.
- **Delega a:** `codigos_lcsc.py` (acha o C-number por valor/pacote),
  `fichas.py` (datasheet verificado por dentro), `bom_completa.py` (o BOM
  gerado), e o novo **`scripts/manifesto_fontes.py`** (o portão STOP).
- **PORTÃO — o STOP gate:** só avança quando **toda** linha está num estado
  legal: (A) C-number válido, na biblioteca de montagem (stock>0), com tier;
  (B) footprint importado/à-mão **verificado** (§4); (C) cliente-fornece
  verificado. `manifesto_fontes.py` sai ≠ 0 e lista as linhas bloqueadas —
  provado com `--sabota`. ⚠️ *Stock na LCSC ≠ stock de montagem na JLC.*
- **Artefacto:** **manifesto de fontes commitado** (uma linha por linha do BOM),
  e o exit 0 do `manifesto_fontes.py` sobre ele.

### Estágio 3 — Colocação contra a estrutura física

- **Objetivo:** a placa cabe na caixa e tudo o que atravessa o painel bate no
  recorte certo — antes do cobre.
- **O que faz:** contorno tirado do modelo da caixa; furos de fixação;
  conectores/botões/display batem nos recortes do painel (`aberturas.json`);
  nenhum parafuso debaixo de um componente; zonas de altura respeitadas;
  cablagem/conectores com folga para o cabo e alcançáveis com a placa montada.
- **Delega a:** `caixa` (os 5 portões, e é a autoridade sobre a geometria caixa↔
  placa), `kicad/pcb_contorno.py` (importa o gerador da caixa, escreve
  `aberturas.json`), `poe_furos.py` (furos M3 derivados do contorno).
- **PORTÃO:** os 5 portões de `caixa` (nesting, espelho, margem, **a placa
  cabe** `confere_caixa`, **furo livre** `confere_furos`). ⚠️ **A origem do
  footprint não é o eixo** — ler o veio em `F.Fab`. E os três que o DRC 2D não
  vê: **volume (Z), acesso (USB-C), camada vazia ≠ face vazia** — cada um é o seu
  portão. ⟨dB⟩ montagem a seco de UMA caixa antes da série.
- **Artefacto:** `.kicad_pcb` com contorno+furos, `aberturas.json`, e o handshake
  contorno↔aberturas confirmado.

### Estágio 4 — Vias + largura de pista (só DEPOIS da colocação)

- **Objetivo:** cada rede com cobre que aguenta a sua corrente, vias que a
  fábrica consegue furar, zero por rotear por (rede, espécie).
- **O que faz:** larguras por classe de rede dimensionadas por IPC-2221 para a
  corrente de cada rede; vias ≥ mínimo JLC; costura (stitching); roteamento.
  Ver **`references/fab-automation.md`** (tabela IPC-2221, capacidades JLC).
- **Delega a:** `kicad_layout` (OBRIGATÓRIA no fecho), `classes_de_rede.py` (as
  larguras vivem nas classes, o DRC e o router lêem-nas), `roteia.py`/
  `roteia_fr.py`, `pcb_contorno.costura()`.
- **PORTÃO:** **só se encosta cobre depois de o portão do estágio 3 fechar** —
  a colocação aprovada é a porta de entrada, «só então» as vias, nunca antes;
  larguras nas classes de rede (o DRC obriga); vias ≥ JLC (furo
  ≥0,2 mm, anel ≥0,13 mm, ⌀ ≥0,45 mm); **por (rede, espécie) o não-roteado = 0**,
  e o invariante é *nenhuma rede pode piorar* (o total esconde uma rede que
  regride; `Zone<->Zone` é só indicação). DRC com `--refill-zones`.
- **Artefacto:** relatório DRC por rede (via `le_drc.py`), classes de rede
  escritas no ficheiro.

### Estágio 5 — Aterramento (um estágio inteiro só para isto)

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

### Estágio 6 — Rótulos e texto

- **Objetivo:** serigrafia que se lê e que aponta para a peça certa.
- **O que faz:** `silk_over_copper = 0`, silk fora de pads/máscara, refs a
  apontar para a **própria** peça, marcas de pino-1/polaridade.
- **Delega a:** `arruma_silk.py` (move refs, auto-porta-se — não escreve se as
  colisões sobem), `refs_apontam.py` (distância à própria peça vs à vizinha),
  `rotula_tp.py` (nome do TP vem do `(net)` do pad, nunca do `Value`),
  `design-review` (legibilidade com o olho).
- **PORTÃO:** silk sobre cobre = 0; texto×texto = 0; stroke ≥0,15 mm, altura
  ≥1 mm; `refs_apontam.py` sai ≠0 se um designador está mais perto da peça
  errada; marcas de polaridade/pino-1 presentes no **impresso**. ⚠️ *Contagem-
  zero autoriza olhar sabendo onde, não saltar o olho* — o campo roda com a peça
  (`X = fx + dx·cos a + dy·sin a`), o obstáculo é pads+trilhos+silk+contorno.
- **Artefacto:** exit 0 de `refs_apontam.py` e do gate de silk; resultado de
  `design-review` no relatório.

### Estágio 7 — Revisão tripla (agentes independentes, 3 lentes, 3 passagens)

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
- **Delega a:** `kicad`, `kicad_layout`, `caixa`, `revisao-terra`,
  `design-review`, `revisao_externa.py` (kicad-happy, segunda opinião externa).
  Ver **`references/review-protocol.md`** (as 3 lentes em detalhe, o livro-razão
  de achados, revisores adversariais). Modelado no `ifa10000`.
- **PORTÃO:** cada lente passa com **números, não vistos**; lente reprovada
  **interrompe** (não se salta "pra ver se passa"); **a última linha de defesa
  não é a ferramenta que fez o trabalho**; **livro-razão de achados** para que
  nada caia em silêncio; um achado que reaparece vira **portão novo no mesmo
  commit** (não uma nota); **⟨dB⟩** o veto dos renders de placa na lente C
  (`design-review` passa pelo olho — exit 3 não-verificável não é aprovado).
- **Artefacto:** a ficha das 3 lentes × 3 passagens preenchida com números, e o
  livro-razão de achados (severidade, onde, regra pelo nome, conserto).

### Estágio 8 — Gerbers para a JLCPCB

- **Objetivo:** um pacote de fabricação que a JLC aceita, e que diz de que estado
  do repo veio.
- **O que faz:** `para_fabrica.py` mede, recusa se mau, exporta gerbers/drill/
  pos/bom, carimba data/commit/dirty, escreve `INCOMPLETA` se forçado.
- **Delega a:** `para_fabrica.py` (o portão de fabrica). Ver
  **`references/fab-automation.md`** para o cheat-sheet do `kicad-cli` e o
  conteúdo do zip.
- **PORTÃO:** DRC = 0 com regras JLC activas; zonas re-preenchidas **no
  ficheiro** (fill velho embarca calado); BOM = CPL = placa; LCSC em toda linha
  SMT; **⟨dB⟩ rotação confirmada peça-a-peça no pré-visualizador da JLC antes de
  pagar** (não scriptável); carimbo de procedência (data/commit/árvore-suja) +
  guarda `INCOMPLETA`.
- **Artefacto:** `fabrica/*.zip` carimbado, e o registo do ⟨dB⟩ da rotação.

### Estágio 9 — Retrospetiva / melhoria do método

- **Objetivo:** cada defeito que a revisão tripla (ou a fábrica) apanhou torna o
  método melhor na próxima corrida.
- **O que faz:** depois da entrega, todo defeito vira uma **verificação
  automática nova** ou uma **edição de skill** — a pipeline melhora-se a cada
  corrida.
- **Delega a:** `audit` (coerência docs↔código↔skills depois da mudança),
  `edicao-segura` (bump semver + reportar quais ficheiros mudaram), `pm`
  (quando encomendar). Estilo `ifa10000`: documento vivo.
- **PORTÃO:** **«O que nenhum portão apanhou vira portão novo.»** Um achado que
  fica só como nota não fecha o estágio 9; tem de sair como script, `.kicad_dru`,
  passada de `revisao-terra`, ou lente de revisão — no mesmo commit.
- **Artefacto:** o diff que acrescenta o portão novo, mais o `audit` a passar.

---

## Referências (divulgação progressiva)

- `references/parts-sourcing.md` — a árvore de 3 níveis, o esquema do manifesto,
  as ferramentas (jlcparts, easyeda2kicad, JLC2KiCadLib), URLs e comandos.
- `references/fab-automation.md` — cheat-sheet do `kicad-cli`, tabela de
  capacidades JLC, tabela IPC-2221 de largura/via, regras `.kicad_dru`,
  freerouting.
- `references/quality-standards.md` — o catálogo de modos de falha (20 linhas) e
  os números duros (silk, costura, desacoplamento, courtyard/borda).
- `references/review-protocol.md` — a revisão tripla: 3 lentes A/B/C, 3
  passagens, revisores independentes/adversariais, o livro-razão.
- `scripts/manifesto_fontes.py` — o portão STOP do estágio 2 (`--sabota` prova
  que sabe dizer não).

## Skills que esta orquestra

`kicad` · `kicad_layout` · `caixa` · `revisao-terra` · `design-review` · `pm` ·
`medicao-limpa` · `edicao-segura` · `audit` · e o modelo `ifa10000`. O braço
executável de ponta a ponta é `kicad/faz_placa.py` (esquema→pacote de fabrica
numa ordem escolhida por medição) — a pipeline conduz por cima dele, não o
substitui.

## Anti-decadência

Corrida de placa nova que revele um buraco na pipeline **ganha estágio, portão
ou lente** — proposta, nunca em silêncio. A pipeline que não se actualiza
fabrica a placa de ontem.
