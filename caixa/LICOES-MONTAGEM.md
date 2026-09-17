# Lições de montagem — a caixa de acrílico, montada à mão

### Cotucatronics · 17/09/2026 · rev. c · acrílico transparente **3,0 mm** · gravado da bancada

> **rev. c (17/09/2026):** lição #1 corrigida — o pot físico em uso é um
> **pot de corpo 16 mm** (Ø16,90 · bucha **M7×0,75** · **3 terminais Ø1,20 a
> passo 5,0 mm** · pino anti-rotação a **7,80 mm @ 30°** por datasheet ≈ 7,5 mm
> medido pelo dB), **NÃO** um PTV09A de 9 mm. Isto **supersede** a premissa
> anterior do PTV09A. Adicionada a **nota #6** (footprint RV1/RV2 do painel vs
> pot físico — resolução por **fiação**, caso B; sem problema no painel já
> fabricado). Cota da tampa mantida (ø3,2 · 7,5–7,8 mm c-c · esquerda/−X na vista
> de cima).

> **rev. b (16/09/2026):** lição #1 — furo do pino de indexação do pot
> **totalmente locado na peça** (**ø3,2 mm · 7,5 mm centro-a-centro · à esquerda
> do eixo na vista de cima**), substituindo a incerteza anterior do datasheet.

> Isto não é o manual de montagem (esse é o `CAIXA-ACRILICO-MONTAGEM.md`). Isto
> é o que a **peça em mão** ensinou depois de cortada e montada — cinco coisas
> que só apareceram com o acrílico real na mesa, cada uma com a linha do gerador
> onde nasce, a correção escolhida (**parafusada**, ver diretriz no fim), se
> obriga a re-corte, e a ferragem que passa a fazer falta. É a matéria-prima da
> próxima **caixa rev**.

---

> ## ⚠️ Para a caixa ATUAL (a que já está montada)
>
> Estas correções são para a **próxima** revisão. A caixa que já existe fica
> como está: **o fundo só segura se os 6 calços estiverem colados às paredes
> primeiro** — cola os 6 calços, deixa curar, e só depois aperta os 6 parafusos
> por baixo. Não há furo parede↔fundo nesta caixa; a carga do fundo passa toda
> pelas colas dos calços (é precisamente o defeito nº 5). Não tentes apertar o
> fundo antes de os calços estarem firmes na parede.

---

## 1 · Furo do pino de indexação do pot — não existe no gerador

**Sintoma observado:** os pots (e o encoder) não assentam rentes à face; ficam
com folga para rodar em vez de travarem contra o painel.

**Causa no design:** o gerador faz **só o ø7 central** por pot —
`cotuca_caixa_corte.py:264` (encoder em x=105, RV1/pot L em x=140, RV2/pot R em
x=175, todos a y=92). **Não faz o furo do pino anti-rotação** (*locating pin* /
*anti-rotation pin*; o furo que o recebe é o *locating hole*). Sem esse furo o
corpo do pot não tem onde encaixar o pino e não assenta.

**Distância MEDIDA (locada): 7,5mm centro-a-centro.** Medido direto na peça
pelo dB, do eixo-centro ao pino-centro. **A peça em mão manda — supersede o
datasheet.**

**Esclarecimento importante a registrar** (evita confusão futura): o pot físico
em uso é um **pot de corpo 16 mm** — Ø16,90 · bucha **M7×0,75** · **3 terminais
Ø1,20 a passo 5,0 mm** · pino anti-rotação/locate a **7,80 mm de raio @ 30°**
por datasheet (≈ 7,5 mm medido pelo dB, à esquerda). **Isto supersede a premissa
anterior do PTV09A**: o PTV09A é um pot de **9 mm** (passo de pino 2,5 mm, postes
de locate ~4,4 mm) e **não é** a peça física. O **7,5mm é a trava anti-rotação
do lado do PAINEL** — o *tab*/pino que entra na **TAMPA** da caixa; é a feature
que a cota da tampa recebe. O furo da **TAMPA** usa **ø3,2mm · 7,5mm c-c**
(mantido). Sobre o footprint RV1/RV2 do painel (PTV09A) vs o pot 16 mm físico,
ver a **nota #6** — resolvido por **fiação** (caso B), sem problema no painel já
fabricado.

**Posição angular do furo (agora conhecida):** furo do pino a **7,5mm c-c, à
esquerda do eixo na vista de cima do painel** (**−X, mesmo Y** do centro do
eixo). Olhando o painel de cima, o furo do pino anti-rotação fica à **ESQUERDA**
do furo do eixo.

> ⚠️ **Cuidado para a implementação no gerador:** «esquerda na vista de cima»
> tem de ser mapeado pelo transform de coordenadas da tampa (`to_top()` /
> qualquer *mirror* da TAMPA em `cotuca_caixa_corte.py`) — **confirmar o SINAL
> contra o transform antes de gerar**, para não sair espelhado. É a família de
> bug de eixo/*mirror* que a própria skill avisa. **A peça em mão + a vista de
> cima mandam.**

Regra (§0 da skill `caixa`, e vale aqui inteira): **a peça em mão manda.** O
**pot 16 mm tem pino anti-rotação** (7,80 mm @ 30° por datasheet) — confirmar na
peça se a tampa precisa de **1 ou 2** furos de indexação.

**Correção escolhida:** adicionar furo(s) no *locating hole* à cota medida
(**7,5mm c-c**), diâmetro **ø3,2mm** (medido pelo dB, confirmado na peça).

> **Spec completa da lição #1 (locate hole do pot):** **ø3,2mm · 7,5mm
> centro-a-centro · à esquerda do eixo na vista de cima do painel (−X, mesmo Y)**.

**Precisa re-corte?** **Sim.**

**Ferragem:** nenhuma.

---

## 2 · OLED monta por fora, exposto

**Sintoma observado:** o módulo OLED acaba empurrado para a face **externa** da
tampa — vidro, PCB e cabo ficam do lado de fora, à vista e desprotegidos.

**Causa no design:** hoje a tampa tem 4× **ø2,5** (`cotuca_caixa_corte.py:265`)
mais a **janela 55×27,5** (`:266`), e **nada mais** — sem *standoff*/espaçador
que defina de que lado o módulo se monta. Sem batente interno, a montagem
natural empurra o módulo para fora.

**Correção escolhida:** montar o OLED **por dentro**, em **4 standoffs M2.5** —
o vidro olha pela janela, o parafuso entra pela frente, e o módulo e o cabo
ficam **dentro** da caixa.

**Precisa re-corte?** **Não** — os 4 furos ø2,5 já existem. Só se quiseres um
**rebaixo** para o vidro afinar com a face é que há re-corte.

**Ferragem:** 4 standoffs M2.5 + parafusos.

---

## 3 · Os cabos não entraram na caixa

**Sintoma observado:** os fios do OLED não tinham por onde entrar; na montagem
passaram-se **pelos retângulos de ventilação** como gambiarra — os `BACK_RECTS`
(`cotuca_caixa_corte.py:270`) e os `BOT_RECTS` (`:343`).

**Causa no design:** é **consequência direta do nº 2** — com o OLED por fora, o
módulo e o seu chicote ficam do lado errado da face e o cabo não tem furo de
serviço para voltar para dentro.

**Correção escolhida:** **resolve-se ao corrigir o nº 2.** Com o OLED montado
por dentro, o cabo já nasce dentro da caixa e não há gambiarra pelas grelhas.

**Precisa re-corte?** Não por si — segue o nº 2.

**Ferragem:** nenhuma (a do nº 2).

---

## 4 · A cola do reforço/nervura mancha o acrílico transparente

**Sintoma observado:** o solvente das juntas do reforço e da nervura **nubla e
mancha** a tampa transparente — o embranquecimento onde o vapor/solvente toca,
que o próprio manual já avisa (`CAIXA-ACRILICO-MONTAGEM.md:36` e `:98-101`).

**Causa no design:** a peça `REFORCO` (`cotuca_caixa_corte.py:345-350`) e a
`NERVURA` (`:363`) são **soldadas com Weld-On 4 capilar** por baixo da tampa
transparente. Solvente sobre acrílico transparente = mancha visível, e numa
casca transparente o interior é a cara (§4e da skill `caixa`).

**Correção escolhida:** **aparafusar** o reforço e a nervura — furos **ø3,4 na
TAMPA** + bolso de porca na peça (a mesma maquinaria de porca cativa dos calços,
ver nº 5) — **zero solvente**. A mancha desaparece na origem porque a origem
(o solvente) deixa de existir.

**Precisa re-corte?** **Sim.**

**Ferragem:** ~9 M3 + 9 porcas.

---

## 5 · Fundo — fixação escondida e colada (*design smell*)

**Sintoma observado:** a fixação do fundo é confusa e frágil. O fundo pendura em
**6 parafusos M3** que sobem para **porcas cativas em 6 calços** colados; a carga
inteira do fundo passa por **6 colas de 12 mm**, sem nenhum furo que ligue
parede a fundo.

**Causa no design:** os calços **CB/CT** (`cotuca_caixa_corte.py:366-370`) são
**colados nas PAREDES**, 3 mm acima da borda inferior
(`CAIXA-ACRILICO-MONTAGEM.md:66-84`). Não há furo parede↔fundo — o fundo só se
segura pelas 6 colas dos calços. É escondido (não se vê como está preso) e
frágil (toda a carga em 6 juntas coladas de 12 mm).

**Correção escolhida:** **postes parafusados à parede E ao fundo** — porca cativa
dos **dois** lados, furo **ø3,4 pela parede** (cabeça de parafuso visível, e não
faz mal ser visível) mais o furo **ø3,4 do fundo que já existe**. O fundo passa
a estar preso por parafuso à estrutura, não pendurado em cola.

**Precisa re-corte?** **Sim.**

**Ferragem:** ~12 M3 + 12 porcas.

---

## 6 · Footprint RV1/RV2 do painel (PTV09A) vs o pot físico de 16 mm — resolvido por fiação

**O que os arquivos dizem (conferido no PCB e no esquema):** RV1 e RV2 no painel
usam o footprint `Potentiometer_THT:Potentiometer_Bourns_PTV09A-1_Single_Vertical`
(pot **9 mm** montado NA PCB). Medido no `cotuca_painel.kicad_pcb`:

- **3 pads passantes** — pad 1 (GND) em (0, 0), pad 2 (POT_L no RV1 / POT_R no
  RV2) em (0, −2,5), pad 3 (+3V3A) em (0, −5). **Passo centro-a-centro = 2,5 mm**,
  furo **ø1,0 mm**, pad ø1,8 mm.
- **2 postes de fixação "MP"** (locate do PTV09A) furo ø2,2 mm em (7, −6,9) e
  (7, 1,9).
- **Corpo (F.Fab) ~12 × 9,7 mm; courtyard ~14,4 × 13,7 mm** — dimensão de pot 9 mm.
- Esquema/BOM do painel declaram MPN **PTV09A-1020S-B103** (Bourns, 9 mm, 10k lin).

**O pot físico** é um **16 mm** com **3 terminais a passo 5,0 mm** — ou seja,
**2× o passo** do footprint (5,0 vs 2,5 mm) e corpo bem maior. Como peça
**board-mounted** ele **NÃO caberia** no footprint PTV09A do painel.

**Resolução (caso B, decidido pelo usuário — NÃO é problema de fabricação):** os
pots são montados na **TAMPA** e ligados por **FIO** às pads RV1/RV2 do painel —
**não** soldados como pots board-mounted. Com fiação, soldam-se **3 fios nas 3
pads** de cada RV; o passo do footprint (2,5 mm) **é irrelevante** para fiação, e
os postes MP ficam sem uso. A função é preservada (POT_L/POT_R/+3V3A/GND chegam
ao pot). **O painel já fabricado está OK — não há re-fabricação a fazer por causa
disto.**

**Para a PRÓXIMA revisão do painel (clareza, não urgência):** trocar o footprint
PTV09A de RV1/RV2 por um **header/conector de 3 pinos de fiação** (ou marcar
explicitamente como pads de solda-fio), para o desenho ficar honesto — **fiado,
não board-mounted**.

**Nota de BOM (não alterar o BOM já pedido — só registrar):** o pot real
comprado é um **16 mm** (M7×0,75, 3T @ 5,0 mm, anti-rot 7,80 @ 30°), à parte; o
**"PTV09A" no `export/painel_2026-09-12_06db1c7/bom_painel.csv` (linha RV1,RV2)**
e o MPN `PTV09A-1020S-B103` no esquema são **vestigiais** para a peça física —
funcionam como a *landing* de fiação. Discrepância registrada; BOM encomendado
mantido como está.

**Precisa re-corte / re-fabricação?** **Não** (nem caixa nem painel). Só a
próxima rev do painel ganha o conector de fiação.

**Ferragem:** 3 fios por pot (6 no total) + terminais.

---

## Diretriz da próxima versão

> **TUDO PARAFUSADO — cola só se for absolutamente necessário.**
>
> A próxima caixa é **100% desmontável e reworkável**. A cola (Weld-On) deixa de
> ser o meio de montagem por omissão e passa a exceção justificada. Todas as
> cinco correções acima seguem daqui: onde antes havia solvente ou dependência
> de cola, passa a haver parafuso + porca cativa.

## A decisão do casco — a maior obra

O casco (as paredes e o modo como fecham) é a maior reescrita, e a decisão do
usuário está tomada:

- **Escolhido: postes / cantoneiras internos parafusados.** São **clones dos
  postes do nº 5** — reusam a maquinaria de **bolso de porca** que já existe no
  gerador: `hexagono()` e a *feature* `"hex"`
  (`cotuca_caixa_corte.py:366-368` e a verificação em `:521`). Escolhido sobre
  T-slot porque tem **MUITO menos reescrita de gerador**: a máquina de porca
  cativa já está feita e testada; só é preciso replicá-la nos cantos.

- **T-slot fica como upgrade estético futuro.** É o acabamento mais bonito, mas
  não agora. Nota técnica: as **2 juntas da tampa a 11,8°** complicam um T-slot
  *in-plane* — mais uma razão para deixar o T-slot para depois e ir de postes
  parafusados já.

---

## Tabela-resumo — as 5 correções

| # | correção | re-corte? | ferragem | obra |
|---|---|---|---|---|
| 1 | furo(s) **ø3,2** do pino de indexação do pot (**7,5mm c-c**, à esquerda do eixo em vista de cima, medido na peça) | **Sim** | nenhuma | pequena |
| 2 | OLED por dentro, em 4 standoffs M2.5 | Não (furos existem)* | 4 standoffs M2.5 + parafusos | pequena |
| 3 | cabos entram (resolve-se com o nº 2) | Não | — | nula (segue nº 2) |
| 4 | reforço + nervura **aparafusados** (ø3,4 na tampa + bolso de porca) | **Sim** | ~9 M3 + 9 porcas | média |
| 5 | fundo: **postes parafusados à parede E ao fundo** — **o casco, a MAIOR obra** | **Sim** | ~12 M3 + 12 porcas | **maior** |

\* nº 2 só pede re-corte se quiseres um rebaixo para o vidro afinar com a face.

> **Nota #6 (não é correção da caixa):** o pot é montado na tampa e **fiado** às
> pads RV1/RV2 do painel (caso B). Sem problema no painel já fabricado; a próxima
> rev do painel troca o footprint PTV09A por um conector de 3 pinos de fiação.
> Ver secção **6**.

> A obra maior é o **casco (nº 5)**: reescrever a fixação do fundo como postes
> parafusados nos cantos, clonando a máquina de porca cativa (`hexagono()` /
> `"hex"`) para lá dos 6 calços de hoje.

---

## Procedência

Gravado a 16/09/2026 da montagem à mão da caixa r2 (acrílico 3,0 mm). As cinco
lições vêm da peça real na bancada, não de ficha — que é a regra da própria
skill `caixa` (§0: a peça em mão manda). Alimenta a próxima **caixa rev**; a
reescrita da geometria do gerador (postes/T-slot, furo do pino) é tarefa
posterior — este documento é o **escopo**, não a implementação.
