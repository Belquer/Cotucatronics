---
name: caixa
description: Integrar a caixa e a electrónica do Cotucatronics de ponta a ponta — qual artefacto manda quando há vários, gerar o corte por parâmetro, os cinco portões que recusam antes de gastar chapa, e como a placa se torna imune à espessura. Use ao mudar material ou cotas da caixa, ao dimensionar ou colocar uma PCB contra ela, e antes de mandar seja o que for para a Ponoko ou a JLCPCB.
---

# Caixa ↔ electrónica — o percurso inteiro

## FRONTEIRA COM A `kicad`

| pergunta | quem responde |
|---|---|
| a caixa fecha, encaixa, cabe na chapa? | **esta** |
| a placa cabe na caixa e os furos batem? | **esta** |
| o esquema liga? o ficheiro abre? | `kicad` |
| o desenho lê-se? | `design-review` |

Esta skill nunca decide electricamente nada. Manda na **geometria entre os dois
artefactos** — e é aí que este projecto já perdeu mais tempo do que em ERC.

---

## 0. ANTES DE MEDIR: qual das caixas é a caixa?

Este projecto teve **quatro** fontes para a mesma caixa ao mesmo tempo, com
quatro espessuras, e três delas davam números plausíveis. A ordem de
autoridade não é discutível:

| artefacto | vale? |
|---|---|
| a peça **em mão** | manda em tudo — kerf e tolerância reais |
| **gerador da peça que está a ser feita agora** | sim — é a fonte |
| ficheiro de corte de uma geração anterior | **não** — é um objecto real, mas *outro* |
| desenho cotado, página de fabricação | **não** |

> **«O ficheiro que foi para a fábrica» não chega.** Tem de ser o da peça que
> está a ser feita **agora**. Um artefacto real da geração anterior mede-se tão
> bem como o certo.

⚠️ E **confirmar o material com o dB**, não com o nome do ficheiro. O corte em
nogueira de 6,4 era real, foi para a Ponoko, e não era esta caixa — a encomenda
foi acrílico de 3,00.

⚠️ **E depois APAGAR as fontes vencidas**, não arquivá-las ao lado da actual.
Foi o que se fez a 06/09: morreram as 6 páginas, os dois geradores delas e os
ficheiros da nogueira. O histórico do git guarda-os; a pasta não deve. Uma
fonte vencida ao lado da certa mede-se igualmente bem, e a única coisa que as
distingue é uma verificação que ninguém faz antes de confiar.

⚠️ **Ausência verificada tem prazo.** «Procurei o gerador e não existe» era
verdade às 9h e mentira às 9h20 — o dB estava a escrevê-lo. Num repositório com
duas mãos, voltar a procurar antes de agir sobre uma ausência.

---

## 1. A caixa sai de parâmetros, nunca de edição

```bash
python3 caixa/cotuca_caixa_corte.py --t 3.0 --kerf 0.2
```

Escreve `cotuca_caixa_r<REV>_acrilico_3p0_<digest>.svg`, `_previa.svg` e `.dxf` ao lado
do script. **O nome leva a espessura** — um ficheiro de corte sem espessura no
nome mede-se tão bem como o certo, e foi exactamente assim que se perdeu meio
dia.

Tudo o que depende de `T` tem de ser **derivado**, nunca escrito:

| grandeza | fórmula |
|---|---|
| interior | `W − 2T` × `D − 2T` |
| chapa de fundo | `(W − 2T − 1)` × `(D − 2T − 1)` — 0,5 de folga por lado |
| altura útil | frente `HF − 2T` · trás `HB − 2T` |
| altura do calço | nº de chapas empilhadas × `T` |
| furos dos calços | 6 mm da borda **da chapa**, não cotas fixas |

⚠️ **A profundidade do dente É a espessura.** Um ficheiro cortado a 6 e
executado em 3 mm dá 3 mm de folga em cada junta e 3 mm de lingueta saliente:
a caixa não fecha em esquadro e o solvente não tem o que colar. **Trocar de
material é sempre regerar**, nunca só mudar a linha do pedido.

---

## 1b. O NOME DO FICHEIRO É A IDENTIDADE DO ARTEFACTO

Um ficheiro de fabrico não é código: **depois de encomendado, existe como
objecto**, e o nome é a única coisa que liga a chapa que chegou ao desenho que
a gerou. Se dois desenhos diferentes partilham um nome, essa ligação parte-se —
e parte-se em silêncio.

Aconteceu aqui: a caixa **com** reforço e a caixa **sem** reforço saíram as
duas como `cotuca_caixa_corte_acrilico_3p0.svg`. Uma estava encomendada, a
outra não, e na pasta eram indistinguíveis.

> **Duas coisas diferentes precisam de dois números.** A **versão do gerador**
> diz o que mudou no código; a **revisão do artefacto** diz o que mudou na
> peça. Não são o mesmo: um gerador novo pode emitir a mesma chapa (refiz o
> encaixe três vezes sem mexer numa cota), e o mesmo gerador emite chapas
> diferentes conforme os parâmetros.

O nome carrega **quatro** coisas, e cada uma responde a uma pergunta diferente:

```
cotuca_caixa_20260906_r3_acrilico_3p0_037a9c30.svg
             │        │       │   │   └── QUAL É — digest do desenho
             │        │       │   └────── espessura
             │        │       └────────── material
             │        └────────────────── QUAL FOI A INTENÇÃO — revisão
             └─────────────────────────── QUAL VEM DEPOIS — data da revisão
```

⚠️ **Um digest identifica e NÃO ORDENA.** `55cf3955` e `037a9c30` não dizem qual
é mais recente — pu-lo a fazer um trabalho que ele não sabe fazer, e foi o dB a
perguntar «o que vem depois, 955 ou c30?». A data vem **primeiro** no nome para
o `ls` sair cronológico.

⚠️ **E a data é da REVISÃO, não da corrida.** Gerar duas vezes tem de dar o
mesmo ficheiro; um nome que muda a cada corrida diz «mudou» quando nada mudou, e
aí ninguém confia nele.

⚠️ **O digest é o que não se esquece.** A revisão e a data bumpam-se à mão, e a
mão esquece — foi assim que dois ficheiros ficaram com o mesmo nome. Com o
digest no nome, **geometria diferente é ficheiro diferente por construção**.

E há uma guarda que fecha o laço: o gerador guarda `revisão → digest` num
registo e **recusa-se a escrever** se a mesma revisão der um digest diferente do
que já saiu:

```
*** a geometria mudou e o REV nao.
    r3/acrilico_3p0 ja saiu com digest 037a9c30 e agora da 2e2533b3.
```

> Sem isso, o esquecimento produzia duas geometrias com a mesma revisão — que é
> exactamente o defeito que o digest existe para apanhar.

E é reprodutível: a mesma entrada dá o mesmo digest, sempre. Um digest que
mudasse a cada corrida seria pior que nenhum — dizia «mudou» quando nada mudou.

**O que fica no repositório:** o ficheiro encomendado e o registo de qual foi.
Um artefacto de fabrico não é um build descartável: é o que vai ser preciso
para perceber a peça que chegou dentro de um mês.

| revisão | o que é | encomenda |
|---|---|---|
| r1 | 19 peças · arcade a 40/100/160 · sem reforço | **#1263497** |
| r2 | 20 peças · arcade a 30/100/170 · reforço da tampa | — |

> Generaliza para lá da caixa: **qualquer artefacto que saia do repositório para
> o mundo físico** — gerbers, CPL, ficheiro de corte, STL — leva revisão e
> digest no nome. O que vive só no disco pode chamar-se o que quiser; o que
> alguém vai fabricar, não.

## 2. Os cinco portões — nenhum é opcional

Cada um recusa **antes** de se gastar chapa ou placa. Um portão que avisa e
deixa passar não é portão.

| # | portão | onde | o que impede |
|---|---|---|---|
| 1 | **nesting** | no gerador + `tools/nesting.py` | peça fora da chapa, peças sobrepostas, **peça em falta** |
| 2 | **espelho** | `_rot90` no gerador | peça rodada que na verdade foi espelhada — corta bem, monta ao contrário |
| 3 | **margem à chapa** | no gerador | desenho colado à borda: recusa no upload |
| 4 | **a placa cabe** | `confere_caixa` no `pcb_contorno.py` | placa maior que a chapa que a segura |
| 5 | **furo livre** | `confere_furos` no `pcb_contorno.py` | parafuso de fixação debaixo de um componente |

```bash
python3 tools/nesting.py caixa/cotuca_caixa_r2_acrilico_3p0_55cf3955.svg --pecas=29
```

⚠️ **O `--pecas` não é decoração.** Uma peça que não foi emitida não colide com
nada: um ficheiro com 18 passa exactamente como um com 19.

⚠️ **Portão caseiro pede prova externa.** O `nesting.py` confere o que o
gerador escreveu, mas não foi escrito pelo gerador — é a segunda opinião. Onde
existir ferramenta de terceiros que possa **dizer não** (o visualizador da
Ponoko, o DRC do KiCad, o pré-visualizador da JLCPCB), passa-se por ela.

---

## 3. A placa lê a caixa — e depois deixa de depender dela

```bash
python3 kicad/pcb_contorno.py cerebro
python3 kicad/pcb_contorno.py painel
```

**Importar > medir.** O `cotuca_caixa_corte.py` tem `if __name__ == "__main__"`,
por isso importa-se sem correr nada. Importar não tem parsing entre a cota e o
uso, logo **não tem onde divergir**. Só se não houver gerador é que se mede o
SVG — e aí com guarda de contagem de elementos (leitura incompleta não é
leitura).

### 3a. O passo que fecha o assunto: o invariante

Seguir a cota à mão perde sempre — perdeu quatro vezes em duas horas. O que
fecha é **a cota deixar de entrar na conta**: a placa **centra-se na chapa de
fundo**, e daí sai

```
furo_x  = 25,5 − T − 0,5             (o gerador desconta a parede)
x0      = ((W − 2T − 1) − larg) / 2  (placa centrada)
furo_x − x0  →  constante, para QUALQUER T
```

Confirmado a correr o gerador a `T=3` e `T=6`: os furos caem sempre em
**(15,5 · 128,5) × (3,5 · 96,5)** do canto da placa, e os eixos dos knobs
sempre em **105/140/175 × 92** da face. O `pcb_contorno.py` **recusa-se a
escrever** se isso deixar de valer.

> **Quando um número externo não pára de mudar, o conserto não é segui-lo — é
> construir a peça de modo a que ele deixe de entrar na conta.**

---

## 4. As armadilhas de geometria já pagas

**A origem de um footprint não é o eixo.** No `EC11E-Switch` a origem está na
fila de pinos e o veio a (7,50 · 2,50); no `PTV09A-1` a (7,00 · −2,50). Pôr a
origem no furo da tampa põe lá os **pinos** e deixa o veio 7 mm ao lado — a
tampa fecha e o botão não entra. Lê-se o círculo em `F.Fab`, não se escreve.

**O sinal do `y` vive num sítio só.** No desenho da caixa cresce para cima; no
KiCad para baixo. A conversão mora inteira no `da_tampa()`. Repetida em cada
chamada, imprime tudo ao contrário — já custou nove notas de uma vez.

**Datum da face ≠ aresta cortada.** Medir um furo até à borda da peça dá um
número verdadeiro e inútil: os dedos da junta fazem a aresta não ser a face.
Um «desvio de 0,137 mm» inteiro nasceu daí.

**Cota escrita à mão ao lado de cota calculada envelhece sempre.** `INTERIOR =
(188, 108)` esteve declarada e nunca foi lida por linha nenhuma. Por acaso
estava certa — o que é pior: uma cota que ninguém consulta não pode estar
errada em voz alta, e por isso também não pode estar certa *de forma
verificável*.
→ **Ao rever um gerador, `grep` cada constante pelo seu próprio nome. Zero usos
é achado, mesmo com o valor certo.**

**Comentário com premissa revogada re-ensina o erro.** «A caixa só tem 28 mm
livres à frente» justificava duas decisões de colocação; a caixa passou a ter
49. As decisões continuavam certas por *outro* motivo — e o comentário, que
tem autoridade, ensinava o motivo morto.

---

### 4b. O valor por omissão de um parâmetro de fabrico

Quando um gerador é **importado** por outra ferramenta, o seu valor por omissão
deixa de ser conveniência de linha de comandos e passa a ser **a configuração
de produção do importador**.

O `pcb_contorno.py` importa o gerador da caixa sem passar argumentos. Com o `T`
por omissão a 6,0 e a caixa encomendada de 3,0, os portões conferiram as placas
contra a caixa errada durante todo o trabalho — e disseram ✓.

> **A omissão é o objecto actual, nunca o último que se desenhou.** Um portão
> que mede o objecto errado é pior que nenhum: os dois deixam passar o defeito,
> mas só um o faz com um visto ao lado.

→ Ao rever, para cada valor por omissão: **quem o consome sem o passar?**

### 4c. Apagar a fonte vencida — e só ela

Fonte vencida guardada «como registo» ao lado da actual mede-se igualmente bem.
O histórico do git é o arquivo; a pasta é o presente.

Mas **`diff` antes de apagar**: o ficheiro com ar de cópia esquecida pode ser a
versão nova. E **ler antes de apagar**: o que tinha valor e não era cota (as
regras de percurso de cabo) mudou de casa antes de a página morrer.

## 4d. Espessura fina: o pé, o furo, e o reforço local

Um pé são **~300 N** a meio do vão entre a parede da frente e a nervura. Em
3 mm de acrílico isso flete a olho e, com o tempo, **raja a partir dos furos de
ø30** — o furo é o ponto de concentração de tensão, e o acrílico é sensível a
entalhe.

> **Cortar uma e pisar diz se parte no primeiro dia. Não diz se parte à
> centésima.** Para uma peça replicada em série, o teste destrutivo de uma
> unidade não é evidência sobre a série.

A resposta que não obriga a engrossar tudo: **reforço local**. Uma segunda
chapa da mesma espessura, colada por baixo, **só na faixa dos furos que levam
carga** — 6 mm efectivos onde o pé pisa, 3 mm onde os eixos atravessam.

A espessura fina não foi escolha estética: um **PTV09 tem ~7 mm de rosca**, e
6 mm de tampa mais a placa por baixo não deixam porca. O reforço local respeita
essa razão em vez de a anular.

⚠️ **Mas engrossar o painel muda a montagem de tudo o que o atravessa nessa
faixa.** É a armadilha do bucim ao contrário: com furo igual ao do botão, o
arcade passa a montar em 6 mm — e os arcade **já estão comprados**, com um
comprimento de rosca que ninguém mediu. Por isso o diâmetro do furo do reforço
**não tem valor por omissão**: o gerador recusa-se a correr sem ele, e diz que
medições pede.

| furo do reforço | o botão monta em | precisa de |
|---|---|---|
| = ø do botão | **6 mm** — o forte | rosca ≥ 6 mm |
| = ø da porca + folga | 3 mm, porca no rasgo | porca a caber |

E a faixa **segue o furo**: cresce até deixar um ligamento mínimo (8 mm) entre
o furo e a borda. Um ligamento fino é exactamente onde a fissura começa — é o
defeito que o reforço existe para evitar, reintroduzido pela geometria dele.

⚠️ **Por confirmar:** aresta de acrílico cortada a laser fica com tensão
residual, e solvente sobre aresta tensionada é conhecido por acelerar o
*crazing*. Se for assim, o recozimento antes de colar deixa de ser opcional.
**Não confirmei em fonte** — é hipótese, e diz-se como hipótese.

## 4e. Material transparente: o interior passa a ser a cara

Uma consequência que não é de geometria e que se decide **cedo**, não no fim:
com a casca transparente, chicotes, folgas de serviço e o cabo do IDC ficam à
vista. **Ou se arrumam com intenção — percursos, prendedores, cor de fio — ou
o objecto de arte mostra a bancada.**

O `CAIXA-ACRILICO-MONTAGEM.md` §6b já fixa os percursos por razão eléctrica.
Com transparente, esses percursos passam a ter também de se ver bem, e as duas
exigências nem sempre coincidem.

## 4f. Lições de montagem (campo) + rumo parafusado

Montar a caixa r2 à mão ensinou cinco coisas que nenhum portão apanha, porque
só aparecem com a **peça real na bancada**. Estão inteiras — sintoma, causa com
`file:line`, correção, re-corte, ferragem — em **`caixa/LICOES-MONTAGEM.md`**.
Ler esse doc **antes de abrir a próxima caixa rev**.

> **Diretriz que governa a próxima versão: TUDO PARAFUSADO — cola só se for
> absolutamente necessário.** A caixa passa a **100% desmontável e reworkável**.
> Onde antes o Weld-On era o meio de montagem, passa a haver parafuso + porca
> cativa (a mesma máquina do `hexagono()` / *feature* `"hex"` dos calços,
> `cotuca_caixa_corte.py:366-368,521`). O casco escolhido são **postes internos
> parafusados** nos cantos — clones dos postes do fundo — por pedirem **muito
> menos reescrita de gerador** que o T-slot (que fica como upgrade estético
> futuro; as 2 juntas da tampa a 11,8° complicam-no *in-plane*).

⚠️ **O furo do pino de indexação do pot NÃO EXISTE no gerador.** O
`cotuca_caixa_corte.py:264` faz só o **ø7 central** de cada pot (encoder x=105,
pots x=140/175, y=92) — falta o furo do **pino anti-rotação** (*locating hole*),
sem o qual o pot não assenta rente. Ao montar o pot na guia da caixa, **este
furo entra na lista** (**ø3,2 mm**, medido na peça), a par do ø7.

⚠️ **A distância eixo→pino está LOCADA: 7,5 mm centro-a-centro** — medido direto
na peça pelo dB (§0: **a peça em mão manda**, supersede o datasheet). Este
7,5 mm é a **trava anti-rotação do lado do PAINEL** (o *tab* que entra na TAMPA
da caixa), uma feature **distinta** dos ~4,4 mm dos postes do lado da PCB (que
entram no painel, footprint `Potentiometer_Bourns_PTV09A-1`) — dois conjuntos de
features diferentes, sem conflito entre caixa e placa. Confirma-se na peça se são
**1 ou 2** furos (o PTV09A tem trava). **Posição:** furo do pino **à esquerda do
eixo na vista de cima do painel** (**−X, mesmo Y**) — mapear «esquerda em vista
de cima» pelo transform da tampa (`to_top()`/*mirror*) e **confirmar o sinal
antes de gerar**, para não sair espelhado. Ver `caixa/LICOES-MONTAGEM.md`, lição #1.

## 5. Ordem de trabalho, e onde o dB entra

```
1. confirmar MATERIAL e espessura com o dB          ← não se infere do nome
2. regerar o corte com --t                          ← portões 1, 2, 3
3. correr o pcb_contorno das duas placas            ← portões 4, 5
4. conferir o que MUDOU de montagem:
     · altura do calço  -> altura do espaçador
     · altura útil      -> o que cabe à frente
     · o que abre a caixa (tampa colada? fundo?)
5. render das placas, e VETO do dB
6. rotear · revisao-terra · gerbers                 ← skill kicad
7. cortar UMA caixa, montar a seco, conferir os furos com as peças na mão
8. só então a série
```

⚠️ **O passo 7 não se salta.** Nenhum portão desta skill mede kerf real,
tolerância de acrílico, nem se o pé do músico racha uma tampa de 3 mm. Isso
responde-se com a peça em mão — e é a peça em mão que manda em tudo (§0).

⚠️ **A divergência anunciada sem medir acusa a peça errada metade das vezes.**
A caixa tinha três arcade e o desenho dois; a conclusão óbvia era que o esquema
estava atrasado. O esquema já tinha três. **Medir de que lado está a falta,
antes de mandar mexer.**

---

## 6. CUSTO: medido em duas facturas reais

**O preço segue o COMPRIMENTO DE CORTE. Só.** Duas encomendas da Ponoko, mesmo
material (Clear Acrylic 3,00 mm):

| encomenda | caixa envolvente | área | corte | preço |
|---|---|---|---|---|
| #1263434 | 346,0 × 372,6 | 1289 cm² | 6,39 m | **$37,56** |
| #1263497 | 372,0 × 296,5 | 1103 cm² | 6,07 m | **$35,66** |

| previsão a partir da primeira | erro |
|---|---|
| por **área** | **−$3,52** |
| por **comprimento de corte** | **−$0,02** |

$/m: **5,878** e **5,875** — diferem **0,1%**. $/cm²: diferem 11%.

### ⚠️ E depois chegou um terceiro ponto, que desmentiu a precisão

| encomenda | corte | preço | $/m |
|---|---|---|---|
| #1263434 | 6,39 m | $37,56 | 5,880 |
| #1263497 | 6,07 m | $35,66 | 5,874 |
| **r2, 06/09** | **6,84 m** | **$38,23** | **5,589** |

O modelo ajustado nos dois primeiros previa **$40,19**; a factura diz **$38,23**.
Erro de **$1,96 — 5%**.

> **Dois pontos ajustam sempre uma recta, com erro zero.** «Prevê-se com $0,02
> de erro» não era um resultado: era aritmética. Com o terceiro ponto, o melhor
> modelo de uma variável erra **$1,23 (3%)** no pior caso — e vários modelos de
> duas variáveis (corte+área, corte+peças, área+peças) ajustam os três, o que
> com três pontos e dois parâmetros é garantido e não prova nada.

**O que fica de pé:** `Setup $0.00`, nenhuma linha de material separada,
**nenhum preço de chapa** — a P1/P2/P3 não entra na conta, só importa por
caber. E **o comprimento de corte é o melhor previsor único, a ±3%.** Chega
para decidir entre desenhos; não chega para prometer cêntimos.

### O que isto apaga do que eu tinha escrito aqui

⚠️ **O encaixe apertado poupou $0,00.** Encaixar move as peças, e o corte é uma
propriedade das **peças** — os mesmos 6,07 m antes e depois. O $1,90 entre as
duas facturas veio inteiro de **regerar a 3,0 mm em vez de 6,4**, que muda a
geometria dos dentes e encurta o corte de 6,39 para 6,07 m.

⚠️ **E o «degrau da chapa» que eu punha como lever nº 1 não existe.** Não há
cobrança de chapa. Uma caixa mais pequena é mais barata porque tem menos
perímetro, não porque cai numa P1.

Faz-se o encaixe à mesma — é automático e tira risco de recusa no upload — mas
**não é poupança e não se vende como tal.**

### Onde o dinheiro está, a $5,87/m

| | metros | custo |
|---|---|---|
| se as peças fossem rectângulos lisos | 4,07 | **$23,89** |
| dentes, furos e rasgos | 2,00 | **$11,74** |
| — destes, **os 12 calços** | 0,70 | **$4,11** |
| o encaixe apertado | 0,00 | **$0,00** |

**Projectar para poupar, por ordem de efeito:**

1. **Menos peças, e menos peças pequenas.** O custo segue o **perímetro**, e
   peça pequena é caríssima por milímetro quadrado: doze quadrados de 12 mm
   custam $4,11. Seis calços de material do dobro, ou esquadros comprados,
   tiram mais que tudo o resto junto.
2. **Menos dentes e mais largos**, para a mesma junta — é o `n_target` do
   `cells()`, um parâmetro que já existe.
3. **Caixa mais pequena**: menos perímetro, linearmente. Sem degraus.
4. **Gravação desligada** no upload — é tempo como o corte.
5. **Furos e rasgos**: cada ø30 são 94 mm de corte ($0,55). Os cinco rasgos de
   ventilação do fundo, 390 mm ($2,29). Valem o que valem — mas conta-se.

### O envio, que é o maior de todos

**$22,43 de envio sobre $35,66 de peça — 39% do subtotal.** Para a série, o
envio de uma caixa e o de várias no mesmo pacote não são muito diferentes:
**duplicar a encomenda não duplica o custo por caixa.** É a única alavanca que
vale mais que todas as de cima somadas, e não se mexe no desenho para a puxar.

### O que continua por confirmar

- **Afastamento mínimo à borda da chapa.** A Ponoko não publica número; os 3 mm
  que o gerador exige são **nossos**. As duas encomendas passaram, o que só
  prova que 3 mm chega.
- Se o $/m muda com a **espessura** ou o **material** — as duas facturas são do
  mesmo, logo não dizem nada sobre isso. Cortar 6 mm é mais lento que 3.
- São **dois pontos**. O comprimento explica-os a 0,1%; um terceiro confirmava.

## Procedência

Nasceu a 06/09/2026 da cicatriz **15cf** (`docs/battle_scars.md`): as duas PCB
tinham sido dimensionadas contra desenhos de uma caixa que nunca foi cortada, e
as duas correcções seguintes foram para caixas reais mas erradas. Ver também
`caixa/README.md`, que põe as quatro caixas lado a lado.

## Histórico

- **v1.1.1 (2026-09-16)** — §4f: furo do pino de indexação do pot **totalmente
  locado — ø3,2 mm · 7,5 mm centro-a-centro · à esquerda do eixo na vista de cima
  (−X, mesmo Y)**, tudo medido na peça pelo dB (supersede o datasheet),
  substituindo a incerteza datasheet ~4,4 mm ≠ 4,7 mm-da-borda ~8,2 mm.
  Esclarece que o 7,5 mm é a **trava anti-rotação do lado do PAINEL** (tab na
  TAMPA) — feature distinta dos ~4,4 mm dos postes do lado da PCB (footprint
  `Potentiometer_Bourns_PTV09A-1`), sem conflito caixa↔placa. Posição angular
  agora conhecida: furo do pino **à esquerda do eixo na vista de cima** (−X,
  mesmo Y) — mapear pelo transform da tampa e confirmar o sinal antes de gerar.
  «A peça em mão manda».
- **v1.1.0 (2026-09-16)** — nova secção **4f. Lições de montagem (campo) + rumo
  parafusado**: declara a diretriz **«TUDO PARAFUSADO — cola só se absolutamente
  necessário»** como governante da próxima caixa rev, aponta para
  `caixa/LICOES-MONTAGEM.md` (as 5 lições da montagem à mão da r2), acrescenta à
  orientação de montagem de pot o **furo do pino de indexação ausente**
  (`cotuca_caixa_corte.py:264` só faz o ø7) e reafirma **«a peça em mão manda»**
  para a distância eixo→pino (datasheet PTV09A ~4,4 mm ≠ 4,7 mm medidos da
  borda). Casco escolhido = postes internos parafusados (clones dos calços,
  reusam `hexagono()`/`"hex"`), T-slot como upgrade futuro.
- **v1.0.0 (2026-09-06)** — versão inicial (ver Procedência).
