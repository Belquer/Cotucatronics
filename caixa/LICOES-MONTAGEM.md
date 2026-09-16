# Lições de montagem — a caixa de acrílico, montada à mão

### Cotucatronics · 16/09/2026 · acrílico transparente **3,0 mm** · gravado da bancada

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

**A distância está POR CONFIRMAR na peça.** Há duas fontes que não batem:
- o datasheet do **PTV09A** dá **~4,4 mm** de centro-a-centro entre o eixo e o
  pino (≈ 0,9 mm da borda do ø7);
- mas a medição na bancada deu **4,7 mm a partir da BORDA** do furo central ⇒
  **~8,2 mm de centro-a-centro**, que **diverge do datasheet**.

Regra (§0 da skill `caixa`, e vale aqui inteira): **a peça em mão manda.** Antes
de gerar, mede-se na peça real, do eixo-centro ao pino-centro, e é essa a cota.
O PTV09A tem **DOIS pinos opostos** — confirmar na peça se o painel precisa de
**1 ou 2** furos de indexação.

**Correção escolhida:** adicionar furo(s) **ø~3 mm** no *locating hole*, à cota
medida na peça.

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
| 1 | furo(s) ø~3 do pino de indexação do pot (cota **medida na peça**) | **Sim** | nenhuma | pequena |
| 2 | OLED por dentro, em 4 standoffs M2.5 | Não (furos existem)* | 4 standoffs M2.5 + parafusos | pequena |
| 3 | cabos entram (resolve-se com o nº 2) | Não | — | nula (segue nº 2) |
| 4 | reforço + nervura **aparafusados** (ø3,4 na tampa + bolso de porca) | **Sim** | ~9 M3 + 9 porcas | média |
| 5 | fundo: **postes parafusados à parede E ao fundo** — **o casco, a MAIOR obra** | **Sim** | ~12 M3 + 12 porcas | **maior** |

\* nº 2 só pede re-corte se quiseres um rebaixo para o vidro afinar com a face.

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
