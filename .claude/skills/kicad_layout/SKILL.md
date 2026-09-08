---
name: kicad_layout
description: Layout e roteamento de PCB no KiCad — planos e cobre, o que nenhum DRC vê, medir por rede, serigrafia que se lê e designadores que apontam para a peça certa, e rotear por script sem acreditar no roteador. Use ao colocar peças, encaminhar cobre, arrumar textos da placa ou preparar o pacote de fábrica. É OBRIGATÓRIA no fecho de qualquer placa, chamada pela skill `kicad`.
---

# /kicad_layout — o esquema acabou, e começa outra máquina

> **Esta skill saiu da `kicad` a 08/09/2026**, quando o capítulo de layout passou
> a ser **mais de metade** dela (1033 de 2022 linhas). Uma skill que não se lê
> antes de usar não é uma skill — é um arquivo. A `kicad` continua a mandar no
> **esquemático**; esta manda na **placa**, e as duas encontram-se no fecho.

## ⚠️ A NUMERAÇÃO É A ORIGINAL, DE PROPÓSITO

As secções mantêm os números que tinham na `kicad` — **§11**, **§11g-bis**,
**§12r**, e por aí. Não foram renumeradas.

> **Renumerar teria partido todos os apontadores de uma vez**, e há dezenas: em
> comentários de código (`arruma_silk` cita a §11c, o `refs_apontam` a §11g-ter),
> nas cicatrizes, na `design-review`. Um número que muda de valor é pior que um
> número feio — é a classe 13 da skill `audit`, a frase partida pela renomeação,
> aplicada a referências cruzadas.

O salto de §10 para §11 nesta skill não é um erro: é o sinal de onde ela foi
cortada.

## A FRONTEIRA, E ELA É REAL

| pergunta | quem responde |
|---|---|
| o ficheiro **abre**? as ligações **existem**? | skill **`kicad`** — ERC e netlist |
| o desenho **lê-se**? | skill **`design-review`** |
| o cobre corresponde à netlist? a serigrafia lê-se? o pacote sai? | **esta** |

⚠️ **Nenhuma substitui as outras, e a ordem não é negociável:** o esquemático
tem de estar provado antes de o cobre valer alguma coisa. Rotear sobre uma
netlist errada produz uma placa perfeita do circuito errado.

⚠️ **E o que esta skill NÃO pode ver:** se a placa faz o que o esquema queria.
Ela prova que o cobre corresponde à netlist, não que a netlist está certa.

## 11. LAYOUT: o esquema acaba, e começa uma máquina com outras regras

Tudo acima é esquema. A partir daqui é cobre, e o cobre castiga outras coisas.
Estas regras não são deste instrumento — são de **qualquer** placa de dois
lados com plano de terra, e é por isso que estão numa skill e não num README.

### 11.0 O F8 casa por UUID, e um gerador troca todos os UUID

`Update PCB from Schematic` **não casa por referência** — casa por UUID. Um
esquema gerado por script muda todos os UUID a cada geração, portanto o F8 não
reconhece nada e **acrescenta um segundo exemplar de cada peça ao lado do
primeiro**. O sintoma não é um erro: é a placa a ficar confusa, com a Daisy
duas vezes e o resto por descobrir.

> **Se o esquema é gerado, o F8 não serve, e o conserto não é apagar
> duplicados à mão.** É um sincronizador que casa por **referência**, tira os
> que sobram, põe os que faltam e escreve as redes nos pads
> (`sincroniza_pcb.py`). O F8 deixa de ser um passo do processo.

Duas armadilhas dentro do sincronizador, ambas pagas:

- **A biblioteca do sistema também conta.** O primeiro sincronizador só
  conhecia a `.pretty` do projecto, e esvaziei as duas placas com `--zero`
  **antes de provar que sabia reconstruir**. Prova o caminho de volta antes de
  usar o caminho de ida.
- **O `.kicad_mod` traz o seu próprio `Reference` `REF**`.** Se acrescentares o
  teu, o KiCad fica com o último e a placa mostra `REF**`. Tira o do módulo.

### 11a. Cobre nas duas camadas não deixa camada nenhuma para sinal

⚠️ **A regra estrutural que substitui meia sessão de trabalho esperto.**

Um dois-lados tem duas camadas. Se puseres plano de terra em F.Cu **e** em
B.Cu, não sobra camada para sinal — e então toda a pista atravessa um plano.
**Atravessar um plano não é passar por cima dele. É cortá-lo:** o cobre recua
dos dois lados da pista e o plano fica em pedaços.

O arranjo certo, e é o normal da indústria:

```
B.Cu   plano sólido de terra, inteiro
F.Cu   sinal, e nenhuma zona
       + uma via em cada pad SMD de terra, a descer ao plano
```

> **Quando um roteador te parece precisar de ficar mais esperto, desconfia do
> desenho antes de desconfiar do roteador.** A pergunta era "como ensino o
> roteador a ver zonas"; a resposta foi "tira o plano do caminho dele". Vinte
> linhas de geometria contra meia sessão de heurística, e a segunda nunca teria
> ficado fiável.

Custo: **19 vias no cérebro, 8 no painel.** É o preço normal, e paga-se uma vez.

### 11b. Cobre adjacente não é cobre ligado

Pus as vias **ao lado** dos pads e não as liguei a nada. O DRC devolveu
`via_dangling` em cada uma das 19. **0,9 mm de vazio isola tão bem como 9 mm.**

Cada via leva um **coto** até ao pad, na camada do pad. E a via vai para o
lado, nunca em cima do pad: **via no pad suga solda por capilaridade** e deixa
a junta pobre — e no lado SMD quem solda é a fábrica.

### 11c. Todo teste de espaço olha pads **E** pistas

O primeiro teste de colisão das vias só olhou pads, e enfiou duas vias a 0,10 e
0,15 mm de pistas que a pessoa já tinha roteado à mão. **O que já lá está é
obstáculo, tenha vindo de onde vier.**

### 11d. `--refill-zones`, ou o DRC compara com um enchimento velho

```bash
kicad-cli pcb drc --refill-zones --save-board -o r.rpt placa.kicad_pcb
```

Sem `--refill-zones` o DRC compara as pistas novas contra o **enchimento
anterior** das zonas e devolve dezenas de `clearance` fantasma. Vais caçar
defeitos que não existem.

> **E antes de dizeres que uma ferramenta não faz X, corre `--help`.** Escrevi
> num commit que o `kicad-cli` não sabia encher zonas. Sabia, e a flag chamava-
> se exactamente aquilo. Foi a terceira afirmação do género no mesmo dia.

### 11e. MEDIR POR REDE — o total mente por omissão

⚠️ **A regra que apanha a regressão que se disfarça de progresso.**

Um roteamento baixou o total de ligações por fazer de **71 para 52** e ao mesmo
tempo partiu a ilha de AGND em oito pedaços. O total melhorou. A placa piorou.
E o DRC dizia geometria zero o tempo todo, porque do ponto de vista dele nada
estava errado.

O que se mede, sempre, e nunca só o primeiro:

| medida | o que apanha |
|---|---|
| por ligar, **por rede** | a regressão escondida atrás de um total melhor |
| `Zone [X] <-> Zone [X]` | o plano cortado em pedaços |
| `starved_thermal` | pad agarrado a uma ilha estreita (largura de cobre, não plano cortado) |
| `via_dangling` | via que não liga a nada dos dois lados |
| geometria (`clearance`, `short`) | o que qualquer fábrica recusa |

⚠️ **E esta tabela é a versão fraca da regra.** Uma lista de sintomas protege
contra o que já aconteceu; contra o que ainda não aconteceu não protege nada —
e o sintoma seguinte apareceu duas horas depois (§12a). A forma forte é comparar
o **perfil** de ligações por fazer, rede a rede, antes e depois, com uma única
regra de aceitação: **nenhuma rede pode piorar**. Usa a tabela para ler um
relatório; usa o invariante para deixar uma ferramenta escrever cobre.

⚠️ **E o nome da rede NÃO está na linha do achado.** O cabeçalho do
`unconnected_items` diz só `Missing connection between items`; a rede vem nas
**duas linhas de item seguintes**. Um extractor que casa o cabeçalho devolve
**zero achados com ar de placa limpa** contra um DRC que diz 47.
**Verificador que mente é pior que verificador nenhum** — este devolveu-me a
placa limpa e eu quase segui.

### 11f. O que NENHUM DRC vê

O DRC é 2D e mede cobre. Três coisas ficam de fora, e cada uma já custou:

1. **Volume.** O botão de arcade desce pela terceira dimensão. Uma peça alta
   debaixo dele passa em todos os testes e não deixa fechar a caixa. Precisa de
   uma tabela de alturas por padrão de footprint e de um portão próprio.
2. **Acesso.** O USB-C tem de continuar a ser alcançável com a placa montada,
   ou a primeira regravação abre a caixa toda. **Planta, volume e acesso são
   três portões, não um.**
3. **Camada vazia ≠ face vazia.** Uma camada sem objectos não prova que aquela
   face está livre; prova que ninguém desenhou lá.

### 11g. Duas coisas que arruínam a serigrafia

- **Referências em cima dos pads.** Um sincronizador que põe cada referência
  numa posição fixa relativa acerta em cheio nos pads: `silk_over_copper` 39.
  Arruma-se por script, e **prova-se pelo número que NÃO muda** — ligações por
  rotear 104 → 104 diz que só se mexeu em serigrafia.
- **O logótipo no centro da placa.** Foi lá parar por defeito e ficou por cima
  de 64 de 130 pads. Procura-se a **maior faixa livre** e põe-se lá.

### 11g-bis. SERIGRAFIA: cada obstáculo que se acrescenta empurra o texto para o próximo

⚠️ **A regra que o utilizador pediu, e que custou quatro medições para ficar
certa.**

Uma ferramenta que afasta as referências dos **pads** e mais nada não arruma a
serigrafia: muda o defeito de sítio. Medido no cérebro, passo a passo:

| obstáculos que o teste conhecia | `silk_overlap` | `silk_over_copper` | `silk_edge_clearance` |
|---|---|---|---|
| só pads | 18 | 6 | 1 |
| + serigrafia | **5** | **12** ⬆ | 1 |
| + pistas, sem isentar os próprios pads | 5 | **1** | 1 |
| + contorno da placa | 6 | 1 | **0** |

Cada linha conserta uma coluna e estraga a seguinte, porque o texto **foge para
o sítio que ainda não está a ser verificado**. Só para quando os quatro estão na
lista: **pads, pistas, serigrafia e contorno**.

> **Um verificador incompleto não deixa o defeito onde estava — empurra-o para
> onde ele não é visto.** É pior que não ter verificador, porque a contagem que
> ele mostra melhora.

⚠️ **E a isenção dos próprios pads tem de cair.** Ela existe porque a referência
por cima do próprio pad é o caso normal de um SMD pequeno — mas assim que a
serigrafia entra nos obstáculos, as referências fogem do contorno e vão parar
exactamente aí. Sem a isenção, quem não tiver sítio **fica onde está e diz**, que
é melhor que ser empurrado para pior.

⚠️⚠️ **E AQUI A REGRA MUDA DE GRAU EM RELAÇÃO AO ESQUEMA.** No esquemático, a
referência dentro do corpo do próprio símbolo é onde ela mora (§9b) e comparar
uma contra a outra dá alarme falso. Na placa **conta**: as duas coisas saem na
mesma tinta branca e a leitura piora.

⚠️ **Mas o grau importa, e esta secção já disse a coisa errada por não o
distinguir.** Escrito assim — *"na placa é defeito"* — entrou em contradição
directa com a `/design-review`, que chamava ao mesmo caso *"o caso normal"*.
Duas skills, duas frases afirmativas, nenhuma marcada como histórica: a classe 8
da skill `audit`, entre ficheiros. **A resolução não é escolher a que agrada.**

| caso | grau | quem manda |
|---|---|---|
| **texto × texto** | defeito, sempre | trava o portão |
| referência × contorno de **OUTRA** peça | defeito | conserta-se |
| referência × contorno da **PRÓPRIA** peça | **aviso** | evita-se quando há sítio; embarca-se quando não há |
| referência × cobre exposto (`silk_over_copper`) | defeito | a fábrica recorta e o texto some |

É por isso que o `arruma_silk` trata o contorno próprio como obstáculo — para o
**evitar** — e o `para_fabrica` **não recusa** por ele. As duas coisas são
coerentes, e nenhuma delas é "é defeito" ou "é normal" sozinha. Foram embarcadas
seis assim no cérebro e uma no painel, com esta linha a justificá-las.

**A prova de que a ferramenta não estragou nada é o número que NÃO muda:**
pistas, vias, pads e peças idênticos antes e depois. Serigrafia só mexe em
serigrafia.

### 11g-quater. Quatro maneiras de medir o texto no sítio errado

O `arruma_silk` tinha **quatro** defeitos ao mesmo tempo, e a família é uma só:
*a conta certa já existia no ficheiro e não estava aplicada onde devia.* Nenhum
deles dava erro; os quatro produziam **aprovação**.

**1 — o campo de referência RODA COM A PEÇA.** O `(at ...)` de uma propriedade é
relativo ao footprint. Fazer `fx + dx, fy + dy` soma um offset do espaço da
**peça** a uma coordenada do espaço da **placa**: nas peças a 0° acerta por
acidente, nas rodadas não. O `U2` (180°) foi avaliado **7,2 mm ao lado** de onde
está — do outro lado do módulo.

```python
X = fx + dx*cos(a) + dy*sin(a)     # a MESMA que os pads já usavam
Y = fy - dx*sin(a) + dy*cos(a)
```

> ⚠️ **E o modo de falhar é o pior que há: no sítio errado não havia nada.** A
> ferramenta disse *livre* e deu a referência por arrumada. **Quem mede no sítio
> errado não reprova nada — aprova tudo, e com ar de quem verificou.**

**2 — e a VOLTA leva a inversa.** Os candidatos são deltas no espaço da placa e
o campo grava-se no espaço da peça. Escrever o delta cru faz o KiCad rodá-lo
outra vez ao desenhar, e a peça acaba com o texto num **terceiro** sítio. Prova
barata: `campo_do_abs(abs_do_campo(x)) == x`.

**3 — `cos(π)` não dá −1 e mais nada.** Dá `sin(π) = 1,2e-16`, e o offset sai
`-4,40873e-16`. É a **§2b** num sítio novo: zero em notação científica, que o
KiCad aceita e que nenhum leitor com `(-?[\d.]+)` apanha. Todo número que entra
num ficheiro KiCad passa pelo formatador, **incluindo os que vêm de uma conta de
rotação**.

**4 — a serigrafia é feita de SEGMENTOS, não de pontos.** Guardar três amostras
por segmento (início, meio, fim) deixa vãos proporcionais ao comprimento: no
contorno do `J1`, com 41 mm, os vãos são de 10 mm, e a referência do `C5` ficou
a **0,11 mm** da linha sem ninguém dar por ela. O teste correcto —
ponto-a-segmento — já estava no ficheiro, nas pistas. **A §11c manda olhar pads
e pistas; a serigrafia ficou de fora da regra por não ser cobre.**

⚠️ **E um `fp_rect` NÃO é uma linha.** `(start ...)` e `(end ...)` são cantos
**opostos**: tratá-lo como um segmento testa a **diagonal** do rectângulo e
ignora as quatro arestas, que são o obstáculo real.

### 11g-quinquies. O recuo «fica onde estava» pode criar o defeito

Consertados os quatro, a ferramenta moveu cinco referências e anunciou *"5
movidas, 6 sem sítio"*. O portão disse **texto×texto: 1**, onde antes eram 0. O
DRC concordou.

A lista de posições ocupadas enchia-se **à medida que o laço andava**: quem
ainda não tinha sido visitado era invisível. E a ordem do laço é a ordem no
**ficheiro**, que não tem relação nenhuma com a geometria da placa.

```
RV1 visitado primeiro → não vê R21 → muda-se para cima dele
R21 visitado depois   → já não há sítio → "fica onde estava" → dentro do RV1
```

> **«Fica onde estava» soa a não ter feito nada, e não é.** É reafirmar uma
> posição que entretanto **deixou de estar livre, por causa de uma mudança da
> própria ferramenta.** O recuo que existia para ser seguro tornou-se o
> mecanismo do defeito.

**Duas coisas, e são precisas as duas:**

1. **Todas** as posições entram na lista **antes** do laço começar — e aí a
   ordem deixa de importar.
2. **Auto-teste no fim:** se ficarem duas referências sobrepostas, **não grava**
   e diz. *Mover texto não pode criar o defeito que vem arrumar.*

⚠️ E isto vale para toda ferramenta que arruma por iteração — encaminhadores,
colocadores, arrumadores de texto. **Um verificador que só olha para o que já
processou aprova o que ainda não viu.**

### 11g-sexies. NÃO COLIDIR É RESTRIÇÃO; APONTAR PARA A PEÇA É OBJECTIVO

⚠️ **A §11g-ter previu este defeito por escrito e ele esteve lá na mesma**, em 8
referências, porque nada o media. *Diagnosticado e por instrumentar é o estado
em que um defeito vive mais tempo: toda a gente sabe, ninguém conta.*

**A causa estava a montante e era uma constante.** O sincronizador criava toda
peça nova com a referência a `-11,5 mm` — igual para um condensador de 11 mm e
para um ponto de teste de 1,5 mm, 23 das 70. E o arrumador não lhes tocava,
porque só mexia no que **colidia**, e a 11,5 mm não se colide com nada.

> **Duas ferramentas, cada uma correcta no seu critério, e o defeito no meio.**
> Nenhuma tinha por missão *«o designador aponta para a peça dele»*.

> **O óptimo de uma restrição sem objectivo é sempre o vazio:** quanto mais
> longe, menos obstáculos. Enquanto "não colidir" foi a única regra, o candidato
> *"para fora da placa"* ganhou sempre — e ganhou com razão.

**As quatro peças do conserto:**

| | |
|---|---|
| **objectivo** | entre os livres ganha o **mais perto da própria peça**, não o primeiro de uma ordem fixa |
| **critério do dono** | a peça mais próxima do texto tem de ser a **dona** — é o que torna isto verificável e não estético |
| **tecto** | sem ele o objectivo ainda premeia fugir |
| **o texto é um RECTÂNGULO** | um raio isotrópico é grande de mais em cima (recusa posições boas) e pequeno de mais aos lados (aceita posições sobre um pad) |

⚠️ **E o recuo tem TRÊS degraus, pela escala do estrago:** sem sítio limpo,
procura-se **mais longe mantendo o cobre livre**, e só no fim se aceita cobre.
Largar tudo de uma vez compra `silk_over_copper` que não é preciso — e o cobre
é o que **apaga** o texto, porque a fábrica recorta a serigrafia sobre a
abertura de máscara. **Meio designador é pior que nenhum: parece um.**

**A troca diz-se com números:** 8 → 0 a identificar a peça errada, 75 movidas;
pagou-se `silk_overlap` +2 e +2, e `silk_over_copper` +2 numa placa. *Texto a
tocar num traço lê-se; texto a apontar para a peça errada mente.*

⚠️ **E entra no portão de fábrica, que é a única forma de não voltar.** Um
`refs_apontam` que compara a distância à própria peça com a distância à mais
próxima. Provado nos dois sentidos — defeito conhecido injectado, e o token que
ele lê renomeado, para que a guarda grite em vez de aprovar zero peças (§2c).

### 11g-septies. A GUARDA DE UMA FERRAMENTA MEDE COM A RÉGUA DE QUEM JULGA

O `arruma_silk` tem auto-teste próprio e ele **aprovou** um arranjo que fez o
portão subir de `texto×texto 0` para `1`. Não foi um bug: o auto-teste usa o
modelo de caixa da ferramenta e o `svg_colisoes` usa o dele (0,55 em por
caractere). Dois modelos do mesmo texto, e o da ferramenta é o mais permissivo.

> **Uma guarda que mede com régua própria não guarda contra o critério de quem
> julga.** Ela aprova honestamente, segundo a régua dela, o que o portão
> reprova.

**O conserto é a prova externa virada para dentro:** antes de gravar, a
ferramenta corre o **portão** sobre o antes e o depois, e recusa se o número
subir. É a mesma disciplina que se aplica a um desenho, aplicada a uma
ferramenta própria.

⚠️ **E confirma-se que o resultado ABRE, antes de substituir o original.** Uma
escrita com deslocamentos desactualizados comeu o espaço de um `(at 0 1.65 0)`
e deixou `(at0 1.65 0)`: o KiCad recusou a placa inteira e só o comando
*seguinte* o revelou. Grava-se numa cópia, carrega-se, e só então se escreve.

⚠️ **A F.Fab conta, mas não conta o mesmo.** Ela não é impressa — 210 dos 336
textos vivem lá — e é a camada que se lê no editor. Arrumá-la é legítimo, com
regras próprias: ali pads e pistas não custam nada, **mas os contornos de peça
da própria Fab custam**. *«Não é impresso» não quer dizer «não tem obstáculos» —
quer dizer que os obstáculos são outros, e outros não é nenhum.* Sem isso,
arrumar empurrava os valores para cima dos contornos e o painel subia de 51 para
68 colisões.

⚠️⚠️ **E CADA PASSAGEM É JULGADA E ACEITE POR SI.** Julgadas em bloco, no painel
a passagem da Fab era óptima (51→37, texto×texto 10→3) e foi recusada inteira
porque a da serigrafia, no mesmo pacote, piorava a serigrafia. **Uma boa ficou
refém de uma má por viajarem juntas.** O portão estava certo a recusar o
conjunto; o erro era o conjunto existir.

⚠️ **E o `total` conta tanto como o `texto×texto`.** Um portão que só olhasse o
segundo deixou a Fab do painel subir de 51 para 68 sem gritar, porque o
texto×texto tinha descido. **Um critério só é um portão para o que ele mede.**

Medido, com o veredicto por passagem na saída:

```
cerebro  ✓ Reference (F.Silkscreen)  0->0   · total   6->6
         ✓ Value     (F.Fab)        29->18  · total 114->87
painel   ✓ Value     (F.Fab)        10->3   · total  51->37
         ✗ Reference (F.Silkscreen)  0->1   RECUSADA
```

### 11g-octies. O QUE SE IMPRIME AO LADO E O QUE SE IMPRIME NUM BLOCO

Um ponto de teste sem o nome da rede na serigrafia é um ponto anónimo: o nome
vive no campo `Value`, que está na **F.Fab** e **não se imprime**. Quem chega à
bancada vê `TP7` e não sabe se é `+3V3A` ou `AGND`.

⚠️ **O nome sai do `(net ...)` do PAD, nunca do `Value`.** O `Value` é uma
afirmação sobre a rede; o pad **é** a rede. Se o ponto mudar de sítio no cobre,
o `Value` continua a dizer o que dizia — e a serigrafia passa a mentir,
impressa, em tinta, na peça. A ler o pad, o rótulo ou está certo ou não existe.

**Duas formas, e a densidade decide qual:**

| | quando | a referência |
|---|---|---|
| **rótulo ao lado** de cada ponto | passo largo (6 mm chegou) | **sai** da serigrafia — `TP7` ao lado de `+3V3A` é ruído |
| **bloco de legenda** | passo apertado (4 mm não chegou) | **fica** — é o que liga o ponto à linha |

> **A mesma decisão, invertida, e as duas certas.** O que muda não é o critério:
> é se a informação está *ao lado* ou *noutro sítio*. Ao lado, o designador é
> ruído; noutro sítio, é o índice.

⚠️ **Onze rótulos e um bloco não ocupam a mesma área.** Onze textos têm de achar
onze buracos, cada um ao lado do seu ponto; o bloco precisa de **um**, e escolhe
entre centenas. Foi por isso que o que não cabia espalhado coube inteiro —
**com zero colisões acrescentadas**.

⚠️ **E o bloco vai PERTO do que descreve.** Uma legenda a 90 mm obriga a
procurá-la, e quem está com a ponta de prova numa mão não procura: desiste, e
mede o pino errado.

### 11g-nonies. O QUE UMA FERRAMENTA NÃO VÊ, ELA ATROPELA

O `arruma_silk` conhecia os campos dos **footprints** e mais nada. Os `gr_text`
da placa — `COTUCATRONICS`, `cerebro rev B`, e depois a legenda dos pontos de
teste — eram invisíveis para ele: era livre de mudar uma referência para cima do
título da placa, e **nenhuma guarda dele daria por isso**. Um `grep -c gr_text`
no ficheiro devolvia **zero**.

> **Antes de acreditar num arrumador, pergunta o que ele lê.** Ele obedece à
> lista de obstáculos que tem, e essa lista é uma afirmação sobre o mundo que
> alguém escreveu uma vez — não é o mundo.

Descobriu-se ao inverter a ordem do fluxo: rotular primeiro e arrumar depois só
faz sentido se o arrumador **vir** os rótulos. Sem isso, a segunda ferramenta
desfaz o trabalho da primeira em silêncio.

### 11g-decies. UM PORTÃO POR PASSO NÃO APROVA UM PASSO QUE PRECISA DO SEGUINTE

A ordem certa parecia clara: os rótulos escolhem o lugar primeiro (o que se lê
na bancada é a **rede**, não o `TP7`), e o arrumador afasta as referências
depois. Está certo — e **não funciona** com um portão por ferramenta:

```
estado intermédio: os rótulos em cima das referências
portão do rotulador: texto×texto 0 -> 12 → RECUSADO
```

O portão não sabia que o passo seguinte arrumava aquilo. Estava a julgar um
estado que nunca ia existir.

> **Ou o portão é do FLUXO, ou cada passo se basta.** As duas saídas são
> legítimas e a escolha é de custo: um portão de fluxo obriga a refazer a
> orquestração; «cada passo basta-se» custa um resultado pior — aqui, os
> rótulos a contornar referências em vez de as mandar embora.

⚠️ **E há uma terceira, que é a que resolveu:** perguntar se o estado intermédio
tem de existir. Não tinha — a referência do ponto de teste podia **mudar de
camada** em vez de ser deslocada, e aí não havia competição nenhuma pelo espaço.
*O conflito entre dois passos costuma ser a pergunta errada a montante.*

### 11g-ter. A fronteira com a `/design-review`, no lado da placa

A `/design-review` é o portão **visual**; o DRC é o **numérico**. Na serigrafia
de uma placa nenhum dos dois chega sozinho:

| pergunta | quem responde |
|---|---|
| há texto por cima de texto, de cobre, ou fora da placa? | `kicad-cli pcb drc` — conta |
| o texto **lê-se**, e aponta para a peça certa? | `/design-review` — olha o render |
| a referência está do lado por onde se solda? | só o olho |

⚠️ **Contagem zero não autoriza saltar o olho** — autoriza olhar sabendo onde.
Uma referência tecnicamente livre pode estar encostada à peça errada, e nenhum
DRC tem opinião sobre isso.

### 11h. A ilha de terra analógica e a ponte única

AGND é uma **ilha** dentro do plano de GND, com prioridade maior para ganhar o
cobre, e liga-se a GND **num sítio só** — uma ponte, num ponto escolhido. Duas
pontes fazem uma malha, e a malha é a corrente de retorno digital a passar pelo
chão do analógico. A ilha tem de existir nas **duas** metades do arranjo: zona
de AGND em B.Cu, e as vias dos pads de AGND a cair dentro dela.

**O plano fecha ligações de graça:** 23 ligações de terra fecharam-se sem uma
única pista, só por o cobre estar lá.

### 11i. Um roteador que insiste é pior que um que desiste

O roteador prova percursos triviais — a direito, dois Ls — e contorna por A\*
se for preciso. **O que não sair fica para a pessoa.** Um roteador que insiste
faz caminhos que ninguém quereria e que depois ninguém percebe ao rever.

E **verifica-se a si próprio**: corre o DRC com as zonas cheias e **retira as
redes que forem acusadas**, devolvendo-as à lista. Escrever e depois medir não
chega — tem de desfazer.

Quatro rondas de defeitos, e as quatro valem para qualquer roteador que
escrevas:

1. os obstáculos não incluíam as pistas que o próprio roteador tinha acabado de
   pôr — 81 cruzamentos;
2. a colisão testava só o centro da célula, e passaram folgas de 0,02 mm;
3. os limites vinham da extensão das peças, não do contorno da placa — pistas
   fora da placa;
4. e mesmo assim duas redes violavam, o que é a razão de existir a retracção.

### 11j. Guardas que se escrevem uma vez e valem para todos os produtos

A automação só compensa se as ferramentas **souberem recusar**. Cada uma
destas é uma linha de defesa que não depende de eu me lembrar:

- o roteador **recusa correr** se encontrar zona em F.Cu (11a);
- o gerador de contorno **recusa escrever** se a placa não couber na caixa, ou
  se um furo de fixação ficar debaixo de uma peça;
- o gerador da caixa **recusa** um valor de espessura por omissão em campos onde
  a omissão é perigosa;
- a lista gerada sai **carimbada com data, commit e aviso de árvore suja**.

> **Quando uma instrução pode matar hardware, a garantia não é a instrução
> estar certa — é a instrução não poder matar.** O mesmo vale para uma
> ferramenta: a garantia não é eu lembrar-me da regra, é a ferramenta parar.

### 11k. O caminho até à fábrica

Tudo em `kicad-cli`, sem abrir a interface, portanto tudo repetível por
produto:

```bash
K=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli
$K pcb drc --refill-zones --save-board -o drc.rpt placa.kicad_pcb   # o portão
$K pcb export gerbers -o fab/ placa.kicad_pcb
$K pcb export drill   -o fab/ placa.kicad_pcb
$K pcb export pos     -o fab/cpl.csv --format csv --units mm placa.kicad_pcb
$K sch export bom     -o fab/bom.csv esquema.kicad_sch
```

⚠️ **O `kicad-cli` não tem subcomando de roteamento**, e o módulo `pcbnew`
**não traz o PNS** (nem `Shove`, nem `Walkaround`, nem autoroute) — confirmado
a listar o módulo, não deduzido.

⚠️⚠️ **MAS HÁ EXPORT DE DSN, E EU DISSE O CONTRÁRIO.** O `pcbnew` expõe
`ExportSpecctraDSN` e `ImportSpecctraSES`, portanto **o FreeRouting — que é um
autoroteador a sério, com rip-up e shove — é alcançável**. Escrevi aqui, e no
README, e num commit, que não era. Ver 15de.

```python
import pcbnew
b = pcbnew.LoadBoard("placa.kicad_pcb")
pcbnew.ExportSpecctraDSN(b, "placa.dsn")     # -> freerouting
pcbnew.ImportSpecctraSES(b, "placa.ses")     # <- o resultado
pcbnew.SaveBoard("placa.kicad_pcb", b)
```

⚠️ **E CORRE-SE COM O PYTHON DO PRÓPRIO KICAD, que existe.** O `pcbnew`
importado do Python do sistema falha no `dlopen` — pede as bibliotecas wx do
KiCad — e eu tirei daí a conclusão errada de que o módulo era inutilizável. O
interpretador está em

```
/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/3.9/bin/python3.9
```

e importa `pcbnew` 10.0.5 sem um queixume. Não o achei porque listei o `bin/`
com `head` e ele está mais abaixo na ordem alfabética, depois dos `img2py` e
companhia. **Um `ls | head` não é uma busca.**

⚠️ **E o `--save-board` reformata o ficheiro.** Um medidor por expressão
regular com janela fixa passa a alcançar campos que antes não alcançava, e a
mesma placa dá dois números diferentes em duas medições. Foi o que me
aconteceu, e a placa não tinha mudado. **Medição por regex em formato que a
ferramenta reescreve não é medição.**


## 12. ROTEAR POR SCRIPT: o que se aprende quando se deixa a maquina tentar

### 12.0 Uma camada de sinal e um grafo planar, e as ligacoes nao sao

Um roteador limitado a uma camada para sempre — e nao por falta de esperteza.
Dois sinais que precisem de se cruzar nao tem por onde. Medido: a versao de uma
camada so fechou 24 de 71 e ficou ali. Com a segunda camada e vias, a mesma
placa foi de **122 para 23**.

A saida e a da industria ha quarenta anos: salta-se para o outro lado com duas
vias, anda-se um bocado, volta-se. **E a via tem de ser CARA no custo da
busca** (aqui, 30 celulas), senao a busca salta por tudo e por nada, e cada
salto abre uma fenda no plano.

### 12a. O estrago no plano tem DUAS formas, e eu so procurava uma

A obvia: `Zone [X] <-> Zone [X]` — o plano partido em ilhas.

A outra, que me escapou e custou uma ronda inteira: **um pad cortado do plano**.
A pista em B.Cu passa entre o pad e o cobre, o pad fica orfao, e isso aparece
como uma ligacao por fazer normalissima na rede GND. O plano continua inteiro.
A terra e que nao. Medido: um roteamento baixou o total de 47 para 20 **e levou
o `GND` de 0 para 5**.

> **Por isso nao se conta uma forma de estrago. Conta-se o PERFIL.** Quantas
> ligacoes faltam em cada rede, antes e depois, e a regra de aceitacao e uma
> so: **nenhuma rede pode piorar**. Nao ha lista de sintomas a manter
> actualizada, e nao ha sintoma novo que passe por nao estar na lista.

Uma lista de sintomas conhecidos protege contra o que ja aconteceu. Um
invariante protege contra o que ainda nao aconteceu.

### 12b. A isencao vale para o defeito que se viu, nunca para o nome dele

A placa chega com defeitos que nao sao da ferramenta, e ela tem de os subtrair
ou passa a vida a culpar-se por estrago alheio e a retirar as suas pistas
todas. Mas **a assinatura da isencao e o par (tipo, rede), nao a rede**.

Isentei `AGND` inteira por causa de uma via solta na linha de base — e com isso
deixei passar uma folga de 0,193 mm numa pista de AGND, que e outro defeito na
mesma rede.

### 12c. A retraccao por bisseccao, e porque nao se retira em bloco

Quando alguma rede piora, a resposta preguicosa e tirar todas as pistas que
passaram pela outra camada. A maioria e inofensiva — o cobre flui a volta — e
tira-las e deitar fora trabalho bom por causa de trabalho mau.

Procura-se **qual**: comeca-se por tirar todas e confirmar que zera (se nao
zerar, o estrago ja la estava e nao ha nada a retirar), e depois devolvem-se uma
a uma, ficando as que couberem. Na pratica isolou 2 travessias culpadas em 33 —
e as outras 31 ficaram.

### 12d. APERTAR A FOLGA PIOROU O RESULTADO

Contra-intuitivo o bastante para ficar escrito. Medido, sempre de raiz e ate
convergir:

| folga | malha | ligacoes por fazer |
|---|---|---|
| **0,35 mm** | **0,25 mm** | **23** ← fica esta |
| 0,26 mm | 0,25 mm | 34 |
| 0,26 mm | 0,20 mm | 44 |

A razao e a ganancia: com mais espaco disponivel o roteador enfia mais pistas
cedo, e essas pistas fecham corredores de que as ligacoes seguintes
precisavam — e mais delas acabam retiradas por estragarem o plano.
**Um roteador guloso melhora quando se lhe aperta o apetite, nao a folga.**

E a malha mais fina nao acha mais caminhos: acha os mesmos mais devagar, e bate
no tecto de nos antes de chegar ao fim.

> ⚠️ Da primeira vez mudei as duas de uma so vez, e a medicao nao soube dizer
> qual delas tinha feito o estrago. **Uma variavel de cada vez, mesmo quando
> parece obvio que andam juntas.**

### 12e. Mexer numa peca mata o cobre que lhe tocava

Mudei uma peca de sitio numa placa ja roteada. As pistas que iam ter com ela
ficaram a apontar para onde ela ja nao estava, e o DRC seguinte devolveu
**curtos-circuitos** na linha de base — que a ferramenta, correctamente, tratou
como "nao e meu" e nao contou.

> **Quando a colocacao muda, o cobre que lhe tocava e lixo.** Nao se remenda:
> apaga-se tudo e roteia-se de raiz. Numa cadeia automatizada isso custa dois
> minutos, e e por isso que a cadeia se paga.

### 12f. A ordem dos passos, e o defeito que passa em todos os testes

A costura das vias corria **antes** da colocacao das pecas: cosia os pads onde
eles ainda nao estavam. Nunca deu erro, porque a placa ja chegava colocada e a
segunda corrida e idempotente — os numeros batiam sempre.

**So se ve no dia em que alguma coisa muda de sitio.** Mudei a ponte de terra e
a costura continuou a falar da posicao antiga.

> Um defeito de ordem que so aparece quando algo muda e um defeito que passa em
> **todos** os testes menos no dia em que interessa. A defesa nao e mais um
> teste: e a ordem estar certa, e o passo que depende de posicoes vir depois de
> as posicoes serem escritas.

### 12g. Ninguem em dois sitios ao mesmo tempo

Uma referencia colocada por uma grelha **e** por uma tabela de posicoes
explicitas fica com a explicita — ate ao passo seguinte recalcular a grelha e a
devolver ao sitio antigo, em silencio. Mudei a ponte, o ficheiro continuou a
mostra-la onde estava, e **nenhum passo se queixou**.

> **Uma colocacao que se desfaz sem dizer nada e pior que uma que falha.** Isto
> nao se resolve com cuidado: resolve-se com um portao que recusa arrancar se
> alguma referencia aparecer nas duas listas.

### 12h. A ferramenta que recusa encontra o que ninguem foi procurar

A costura recusou-se a descer uma via para um plano que ali nao existia, e
disse de quem era o pad. O pad era do **net-tie que e a unica ponte entre a
terra analogica e a digital** — e estava a 57 mm da ilha que devia unir.

A topologia ate estava certa (continuava a ser uma ponte so), mas toda a
corrente de retorno analogica voltava por uma pista fina em vez de por cobre
largo, e **uma ponte com impedancia e meia ponte**. A ponte passou para a
fronteira da ilha, com um pad de cada lado: plano a plano.

> Ninguem tinha ido procurar isto. Apareceu porque uma ferramenta pequena se
> recusou a fazer uma coisa sem sentido **e explicou porque**. Uma ferramenta
> que falha em silencio, ou que faz o que lhe pedem sem perguntar, nao teria
> encontrado nada.

### 12j. Não há ordem de ataque vencedora — há vencedora nesta placa

Um roteador guloso dá resultados diferentes conforme quem entra primeiro.
Medido, de raiz e até convergir:

| ordem | cérebro | painel |
|---|---|---|
| mais curta primeiro | **19** | 13 |
| a do DRC | 23 | **10** |
| mais longa primeiro | 29 | 11 |

Cada placa tem a sua vencedora, e não há forma de a adivinhar olhando. Por isso
a ferramenta **corre as três em cópias e fica com a melhor, medindo** — em vez
de me deixar escolher uma e defendê-la.

> Automação não é só correr os passos sozinha. É **tirar decisões de cima de
> alguém** — e uma decisão que se resolve medindo três vezes não devia estar à
> espera de opinião.

⚠️ E compara-se pelas **duas** medidas, ligações e geometria. A ordem que fecha
mais ligações pode ser a que estraga mais cobre, e escolher pela primeira sem
olhar à segunda é escolher o pior resultado com confiança.

### 12k. Um de cada vez sobre os mesmos ficheiros

Duas escritas concorrentes sobre um `.kicad_pcb` **não dão excepção nem ficheiro
corrompido**. Dão um número: 34 violações de geometria numa placa onde nenhuma
das duas corridas, sozinha, produzia mais do que zero.

> **Uma medição contaminada tem o mesmo aspecto de uma boa.** Não traz marca e
> entra num relatório com a mesma cara que as outras.

A defesa é um **ferrolho** que recusa arrancar se outra corrida já estiver a
escrever as mesmas placas, e diz o pid da que está. Dez linhas.

⚠️ E lançar um segundo trabalho sobre os mesmos ficheiros **é uma escrita, não
uma leitura** — mesmo quando quem o lança não está a tocar em nada, porque o
processo que ficou a correr está.

### 12l. O algoritmo melhor na notação pode ser pior na máquina

A retracção testa uma rede de cada vez — um DRC por rede. Parece preguiça, e a
versão que parte ao meio parece a correcção óbvia. Medida, é pior nos dois eixos:

- **mais lenta**: com 5 culpadas em 33, descer nas metades que falham custa mais
  testes que percorrer a lista. A bissecção paga-se quando há **uma** culpada;
- **pior resultado**: 25 ligações por fazer contra 19, porque o recuo de que ela
  precisa para as culpadas-em-conjunto retira **todas** as candidatas de uma vez.

> **"Melhorar" uma parte que ninguém mediu é uma aposta, não um conserto.**

### 12m. O código de saída da ferramenta anterior faz parte da medição

O roteador recusa gravar quando não consegue provar o que fez, e repõe a placa —
correctamente. Se a cadeia **ignorar o código de saída**, uma passada falhada
fica indistinguível de uma passada que não tinha nada a fazer, e o número final
muda sem que nada o explique.

⚠️ E o mesmo vale para o *shell*: em zsh, **um glob sem correspondência aborta a
linha inteira**. Um `rm -f a b*` encadeado com `&&` a uma edição faz a edição não
correr, e a mensagem `no matches found` perde-se no meio da saída de outro
comando. Custou três medições idênticas e duas conclusões sobre código que não
estava no ficheiro.

> **Resultados idênticos entre condições que deviam diferir não são um
> resultado: são uma avaria do aparelho.** Foi só isso que apanhou o erro.

### 12n. Zero pistas à volta não é espaço livre

Quando um roteador falha, a explicação fácil é congestionamento. Mede-se antes
de acreditar nela: as falhas desta placa tinham **zero pistas num raio de 8 mm**
e concentravam-se todas em peças de duas filas de pinos.

O que as fechava era aritmética:

```
vão entre dois pads de um header 2,54 com pads 1,70:   0,840 mm
o roteador exigia largura/2 + folga + margem-da-grelha
                        0,125 +  0,35 +          0,178 = 0,950 mm
```

Ele não conseguia **sair de entre os pinos** — de nenhum conector, em nenhuma
placa.

> **Um sítio sem nada à volta que a ferramenta não alcança não está
> congestionado: está fechado por uma regra da própria ferramenta.**

⚠️ **E o conserto é uma passada apertada no FIM, nunca desde o início.** Apertar
desde o início mede-se pior (23 → 34): com mais espaço o roteador ganancioso
enfia pistas cedo e fecha corredores de que as seguintes precisavam. No fim o
que já foi roteado já lá está, e a folga apertada só serve para entrar onde a
larga não entrava.

⚠️ **A malha aperta com a folga**, porque a margem da grelha (`PASSO·0,71`)
conta no mesmo corredor. E malha fina na placa toda esgota a busca — daí a
busca da passada apertada ficar presa a uma **caixa** em volta de cada ligação:
o que sobra de uma passada larga é quase sempre curto.

### 12o. Entre dois pinos de conector não cabem a pista e o plano

O mesmo cálculo, na direcção contrária. Para caberem no corredor a pista **e** o
cobre do plano a contornar o pad:

```
pista/2 0,125 + folga 0,20 + cobre mínimo 0,25 + folga 0,20 = 0,775 mm
disponível de cada lado:                                       0,420 mm
```

Uma pista na camada do plano, entre dois pinos de conector, **não divide o
espaço com o cobre de terra: expulsa-o**, e os pinos de terra do conector ficam
órfãos. Marcar os pads da rede do plano com um raio maior nessa camada custa
alguns caminhos e poupa a retracção de os descobrir um a um, a um DRC cada.

### 12p. Uma ferramenta que recusa precisa de poder investigar tudo o que fez

A retracção interrogava só as redes que tinham passado pela camada do plano,
porque a teoria do estrago era essa. Quando o estrago veio de outro lado, a
lista não o continha, e a ferramenta **desistiu de gravar um trabalho quase todo
bom** por causa de uma pista que nem sabia acusar.

> Senão a recusa deixa de ser prudência e passa a ser um ponto cego com boas
> maneiras.

⚠️ E **a referência de um filtro é a propriedade física, não o parâmetro do
momento.** O filtro de suspeitos usava a folga da corrida; numa passada
apertada a folga encolhe, o filtro encolhe com ela, e ele devolveu "0 de 3
suspeitas" numa corrida em que havia estrago.

### 12q. Um conector de duas filas precisa de fuga dos dois lados

Quando os sinais de um conector não roteiam e o diagnóstico dá **zero pistas num
raio de 8 mm**, olha para o bordo da placa. Um header de duas filas encostado a
um bordo deixa a fila de fora com um canal estreito e a de dentro sem saída —
porque sair pelo outro lado obriga a passar entre dois pads, e aí só cabe uma
pista se a folga for a da placa e não a do roteador (§12n).

Medido: canal de 1,05 mm para treze sinais. Descer o conector 4 mm (e o bloco
de peças logo abaixo 2 mm, para lhe dar lugar) levou-o a 5,05 mm e fechou três
ligações — **18 por fazer contra 21**, sem tocar em mais nada.

> **Encostar um conector de duas filas a um bordo é dar-lhe metade dos pinos**,
> e nada no desenho o denuncia.

⚠️ **E a posição nova mede-se com PÁTIOS, não com pads.** A primeira busca deu
2,49 mm de folga entre pads e o DRC devolveu dois `courtyards_overlap`. O que
reserva espaço num footprint é o pátio — e o pátio deste conector chegava a
0,8 mm fora da placa.

### 12r. A fuga do pad: apertar a folga só junto às pontas

Quando as ligações que sobram **terminam todas num pad de passo fino** — um
SOIC de 1,27 mm, os pinos de um conector — o problema não é o caminho: é o
primeiro milímetro. A folga generosa que serve para o resto do percurso não
cabe entre os pads de onde é preciso sair.

Apertar a folga em toda a parte mede-se pior (23 → 34), porque o roteador
guloso enfia pistas cedo e fecha corredores. Apertá-la **só nos primeiros ~3 mm
à volta das duas pontas** não tem esse efeito: ali não há corredor a fechar, há
um pad de onde é preciso sair. Medido: **18 → 12**.

> **Um parâmetro que é mau em toda a parte pode ser exactamente certo num
> sítio.** Três experiências a medir a folga alta e baixa, quando a resposta era
> que ela não devia ser um número global de todo.

⚠️ O apertado nunca desce abaixo da regra da placa, e o DRC valida o percurso
inteiro com a mesma regra — a retracção continua a existir por trás.

### 12s. Quando afinar deixa de render, o que falta é uma capacidade

Malha, folga, tecto de nós, ordem de ataque, corredor achado numa malha grossa:
cinco afinações, nenhuma rendeu mais de duas ligações e três renderam menos.
Todas partiam do mesmo pressuposto — **que existe caminho livre e falta
achá-lo**. Para as últimas não existe.

A capacidade que falta chama-se **arrancar e refazer**: tira-se o cobre que
estorva, passa-se, e volta-se a rotear o que se tirou, que agora dá a volta
porque tinha alternativas e a outra não. Numa passada isolada: 15 → 8.

> **Quando cinco afinações seguidas rendem uma ou duas cada e oscilam entre
> corridas, já não se está a melhorar nada — está-se a ajustar ruído.** É o
> sinal para mudar de ideia, não para afinar melhor.

⚠️ **E uma capacidade certa pode não estar pronta.** Na cadeia inteira o
arrancar-e-refazer mediu 14 contra 12, e uma corrida entregou a placa com um
plano de terra partido que o auto-teste do roteador não apanhou — foi o portão
da fábrica que o viu. Ficou opcional.

> **A última linha de defesa não pode ser a mesma ferramenta que fez o
> trabalho.** Um auto-teste tem exactamente os pontos cegos de quem o escreveu.

### 12t. Um invariante só protege na dimensão em que foi medido

A regra de aceitação — *nenhuma rede pode piorar* — substituiu uma lista de
sintomas precisamente para não haver sintoma novo que passasse por não estar na
lista (§12a). Passou um:

```
antes  GND = 1   (um pad solto)
depois GND = 1   (o pad ligou-se, e o PLANO PARTIU-SE)
piorou -> NADA
```

Contar por rede protege contra a rede errada; **não protege contra a espécie
errada dentro da mesma rede**. Um plano partido não é "mais uma ligação por
fazer" que se compensa com outra qualquer. A chave passou a ser
**(rede, espécie)**.

⚠️ **E é a mesma família da assinatura `(tipo, rede)` das violações** (§12b), que
eu tinha corrigido horas antes no mesmo ficheiro. **Uma lição aprendida num
sítio não se aplica sozinha ao sítio do lado** — procura os gémeos por corrigir
sempre que corrigires um destes.

⚠️ **E o verificador leva um verificador.** `roteia.py --auto-teste` interroga a
regra com os casos que ela tem de apanhar, começando pelo que ela deixou passar.
Prova-se que tem dentes correndo-o contra a versão antiga: falha.

> **Um verificador que não é verificado é uma opinião com ar de medição.**

### 12u. Desfazer trabalho para tentar mais trabalho leva a mesma regra

Arrancar o cobre que estorva para deixar passar outra ligação (*rip-up*) é a
capacidade que falta quando as últimas ligações já não têm caminho livre — e
nenhuma afinação a substitui, porque nenhuma busca acha um caminho que não há.
Numa passada isolada: 15 por fazer → 8.

Mas se a rede arrancada não conseguir refazer-se, é uma troca — e às vezes um
prejuízo, porque a arrancada **estava feita**.

> **Uma operação que desfaz trabalho para tentar mais trabalho precisa da mesma
> regra que tudo o resto: só vale se o número descer.** Guarda-se o estado
> antes, e volta-se atrás se não desceu.

⚠️ E limita-se quanto arranca. Se uma ligação precisa de arrancar meia placa,
não é ela que está mal roteada — é o desenho, e isso é uma pergunta para quem
desenha, não um problema para o roteador resolver a martelo.

> **Uma capacidade que só pode melhorar ou não fazer nada pertence ao caminho
> por omissão.** Antes das duas correcções, esta não pertencia — fechava
> ligações e partia planos.

### 12v. O FreeRouting é alcançável, e é melhor

O `pcbnew` expõe `ExportSpecctraDSN` e `ImportSpecctraSES`. Na mesma placa, de
raiz:

| | por ligar | tempo |
|---|---|---|
| roteador escrito à mão | 12 | ~9 min (duas placas) |
| **FreeRouting** | **8** | **35 s** |

E as que sobram ao FreeRouting são **todas ligações ao plano**, não sinal: ele
fecha todos os sinais. Faz *rip-up* e *shove* a sério.

```python
import pcbnew                      # com o Python do BUNDLE do KiCad
b = pcbnew.LoadBoard("placa.kicad_pcb")
pcbnew.ExportSpecctraDSN(b, "p.dsn")
# java -jar freerouting.jar -de p.dsn -do p.ses -mp 100 -mt 1
pcbnew.ImportSpecctraSES(b, "p.ses"); pcbnew.SaveBoard("placa.kicad_pcb", b)
```

⚠️ **Quatro coisas que ele não sabe, e que quem o chama tem de tratar:**

1. **Net-ties.** A ponte entre duas terras aparece-lhe como cobre "sem rede" e
   ele roteia por cima: curto-circuito. Põe-se um *keepout* sobre ela no DSN.
2. **`-mt 1`.** A própria ferramenta avisa que a optimização multi-tarefa está
   partida e gera violações de folga. Um aviso de uma ferramenta sobre si mesma
   não se ignora por pressa.
3. **As larguras vêm das classes de rede da placa**, não de uma tabela dentro do
   teu script (§12w).
4. **Ele conta um pad como ligado se cai dentro do polígono do plano**; o KiCad
   só o conta se o cobre *enchido* lhe chegar. À volta de um conector apertado o
   cobre é apagado no enchimento e os dois discordam. Quem manda é o KiCad.

⚠️ E telefona para casa a cada arranque (`api.freerouting.app`), sem opção
documentada para desligar.

⚠️ **E a costura passa a atrapalhar.** As vias que se punham para os pads SMD
chegarem ao plano deixam de ser necessárias — ele fá-lo sozinho — e ocupam
sítio: com elas o plano sai partido. Quando se troca o roteador, reavaliam-se
todas as peças que existiam para compensar as fraquezas do antigo.

### 12w. Uma regra que vive dentro de uma ferramenta só vale para essa ferramenta

As larguras dos trilhos de potência viviam numa tabela dentro do meu roteador.
Funcionou enquanto o roteador era meu — e escondia que **o DRC não sabia delas**.
Quando entrou um roteador de fora, ele desenhou o trilho de entrada com largura
de sinal, porque não tinha como saber.

> O sítio de uma regra de desenho são as **classes de rede** da placa: qualquer
> roteador as lê, e o DRC passa a exigi-las em vez de as ignorar.

⚠️ E atribuem-se por **padrão de nome**, não por lista: uma rede acrescentada
depois entra na classe certa sozinha, em vez de ficar esquecida na largura por
omissão.

### 12x. `Zone [X] <-> Zone [X]` NÃO é uma medição estável

⚠️⚠️ **O achado mais desconfortável desta série, porque invalida verdicts que
eu já tinha dado.**

Cinco corridas do `kicad-cli pcb drc` sobre **cópias idênticas do mesmo
ficheiro**, com as zonas cheias uma vez e gravadas:

```
planos partidos:   3 · 0 · 0 · 3 · 0
ligações por fazer: 15 · 15 · 15 · 15 · 15
perfil por rede:   AGND=4 GND=8   nas cinco
```

Os **factos** são estáveis; o **rótulo** não. O mesmo item por fazer sai umas
vezes descrito como *zona contra zona* e outras como pad contra outra coisa.

Eu tinha construído o veredicto do portão em cima desse rótulo, e reportei
«planos inteiros ✓» dezenas de vezes sobre uma moeda ao ar. Pior: a retracção do
roteador tomava decisões com ele — o que explica parte da variação entre
corridas que eu andei a atribuir à ganância do roteador (§12d, §12j).

> **O critério que manda é o PERFIL POR REDE**, que é estável e que a §12a já
> ensinava. Um plano que se parta a sério faz subir as ligações por fazer da sua
> rede, e o perfil apanha isso. O contador de `Zone <-> Zone` fica como
> **indicação** — serve para saber onde olhar, nunca para decidir.

⚠️ **E um portão não recusa por ruído**, porque um portão que recusa por ruído
ensina quem o lê a ignorá-lo.

**Como se descobre isto:** correndo a mesma medição sobre o mesmo ficheiro mais
do que uma vez. É barato, quase nunca se faz, e foi o que separou o facto do
rótulo aqui. A mesma disciplina apanhou o glob abortado (§12m) — *resultados
idênticos entre condições que deviam diferir são uma avaria do aparelho*; esta é
a face oposta: **resultados diferentes entre condições que deviam ser idênticas
são outra.**

### 12i. O portao antes da fabrica

Uma encomenda e irreversivel e demora semanas: o erro nao se descobre no ecra,
descobre-se quando a caixa chega. Por isso o ultimo passo nao e "exportar
gerbers" — e um portao que mede e **recusa**, e so depois exporta.

O que se mede, e porque cada um esta na lista:

| medida | porque |
|---|---|
| geometria | a fabrica recusa — ou pior, nao recusa, e entrega uma placa em curto |
| ligacoes por fazer, **por rede** | uma placa incompleta nao e incompleta: nao funciona |
| `Zone [X] <-> Zone [X]` | plano de terra em pedacos, invisivel no meio das ligacoes |
| `starved_thermal` | pad agarrado a uma ilha estreita: solda mal, conduz pior |

E o pacote sai **carimbado com data gerada, commit e aviso de arvore suja**: um
pacote que nao diz de que estado do repositorio saiu nao se reproduz, e o que
nao se reproduz nao se corrige.

⚠️ **Um pacote incompleto tem de ser impossivel de confundir com um bom.** A
saida forcada existe — ha razoes legitimas para querer ver os gerbers de uma
placa por acabar — mas escreve `INCOMPLETA` no nome do ficheiro.

⚠️ **E o portao diz o que NAO pode ver.** Prova que o cobre corresponde a
netlist, nao que a netlist esta certa. E a **rotacao da montagem** nao se
verifica por script nenhum: o angulo que o KiCad escreve no CPL e o do
footprint, e a fabrica espera o da biblioteca dela — para muitos
encapsulamentos os dois nao batem, e confirma-se peca a peca na
pre-visualizacao, antes de pagar. O ficheiro sai; a promessa nao.
