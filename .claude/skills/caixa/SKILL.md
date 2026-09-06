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

Escreve `cotuca_caixa_corte_acrilico_3p0.svg`, `_previa.svg` e `.dxf` ao lado
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
python3 tools/nesting.py caixa/cotuca_caixa_corte_acrilico_3p0.svg --pecas=19
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

## 6. CUSTO: o que o encaixe muda, e o que não muda

O dB olhou para o visualizador da Ponoko e viu espaço a sobrar. Tinha razão — e
a resposta tem duas metades que é preciso não confundir.

### Metade que se arruma: a área ocupada

O encaixe estava escrito à mão. Substituído por **bottom-left-fill com rotação
de 90° e semente fixa**, dentro do próprio gerador:

| | caixa envolvente | área |
|---|---|---|
| coordenadas à mão (prateleiras) | 362,4 × 376,5 | 1364 cm² |
| **bottom-left-fill** | **375,0 × 299,5** | **1123 cm² — 82%** |
| chão teórico (soma das peças) | — | 960 cm² |

E a margem à borda passou de 4,5 para 9,0 mm, que é risco de recusa a menos.

> **Encaixe escrito à mão é cota escrita à mão.** Estava certo a T=6, deixava
> 1,4 mm a T=3, e desperdiçava 18% em qualquer das duas. Um encaixador com
> semente fixa é repetível *e* reencaixa sozinho quando a espessura muda.

### O que o aperto rendeu, medido na cotação real

**$1,90.** Reduzir de 1364 para 1123 cm² — 18% de área — poupou um dólar e
noventa na cotação da Ponoko, lida pelo dB no site.

⚠️ **Isto corrige uma expectativa que eu tinha tirado do blogue deles.** O
exemplo publicado mostra o fabrico de quatro peças a cair de $14,77 para $4,53
só por encaixar, e eu esperava algo dessa ordem. **Não generaliza**: aquelas
eram quatro peças *pequenas e iguais*, onde o tempo é dominado pela **viagem**
do laser entre elas. Uma caixa são 19 peças **grandes**, e aí o tempo é
dominado pelo **corte**, que tem exactamente o mesmo comprimento antes e depois
de encaixar — 6,07 m nos dois casos.

> **O encaixe move a viagem, não o corte.** Vale a pena quando as peças são
> pequenas e muitas; numa caixa de peças grandes vale $1,90 e sossego no
> upload. Faz-se à mesma — é automático e tira risco de recusa — mas **não é
> onde se projecta para poupar.**

### Onde se projecta para poupar, então

Por ordem de efeito, com os números desta caixa:

**1. O tamanho da chapa.** É o único degrau grande, e decide-se **antes de
desenhar seja o que for**: a peça maior contra a chapa mais pequena.

| | P1 181 × 181 | P2 384 × 384 | P3 790 × 384 |
|---|---|---|---|

A tampa tem **200 mm**. Se a caixa fosse ≤ 181 na maior dimensão, caía numa P1
— e isso é uma decisão de **instrumento**, não de encaixe. Uma caixa de 200 e
uma de 180 custam degraus diferentes, e nenhum encaixador atravessa esse
degrau.

**2. Comprimento de corte e número de peças.** Medido nesta caixa, dos 6,07 m:

| | metros | % |
|---|---|---|
| se as peças fossem rectângulos lisos | 4,07 | 67% |
| dentes, furos e rasgos | 2,00 | 33% |
| **os 12 calços** (doze quadrados de 12 mm) | **0,70** | **12%** |

⚠️ **12% do corte são doze quadrados de 12 mm.** Peça pequena tem perímetro
enorme por unidade de área — o custo segue o perímetro, não a área. Reduzir a
12 calços para 6 (material do dobro da espessura), ou substituí-los por
esquadros comprados, tira mais que qualquer encaixe.

**3. Dentes.** Menos dentes e mais largos cortam menos que muitos e estreitos,
para a mesma junta. O `cells(L, n_target, ...)` do gerador tem esse `n_target`
como parâmetro — é onde se mexe.

**4. Gravação.** É tempo como o corte. Confirmar que ficou **desligada** no
upload: as peças distinguem-se pela forma, os rótulos não fazem falta.

### Metade que NÃO se arruma arrumando

⚠️ **A chapa continua a ser a P2.** A TAMPA tem **200 mm** de largura e a P1
tem 181 — nenhuma disposição a faz caber, nem rodada, nem com margem zero.

> **Antes de optimizar o encaixe para poupar dinheiro, perguntar se a chapa
> pode sequer encolher.** A resposta é uma comparação de dois números: a peça
> mais larga contra a chapa mais pequena. Se não passar, o encaixe é higiene e
> risco, não é preço.

**Encolher a chapa exige encolher a CAIXA** — 200 → ≤ 181 de largura, ou a
tampa parte-se em duas. Isso é decisão do dB sobre o instrumento, nunca do
encaixador.

### Onde o dinheiro está mesmo, medido

Corte total **6,07 m**. Repartido:

| | metros | % |
|---|---|---|
| se as peças fossem rectângulos lisos | 4,07 | 67% |
| **dentes, furos e rasgos** | **2,00** | **33%** |
| os 12 calços (peças minúsculas) | 0,70 | 12% |

⚠️ **12% do corte são doze quadrados de 12 mm.** Peça pequena tem perímetro
enorme por unidade de área — se o custo for por tempo de laser, é aí que está a
gordura, não no espaço vazio da chapa.

E confirmar sempre se a **gravação** ficou ligada no upload: são minutos de
laser por rótulos que não fazem falta, porque as peças distinguem-se pela forma.

### O que está confirmado, o que não está

| | estado |
|---|---|
| P1 **181 × 181** · P2 **384 × 384** · P3 **790 × 384** | confirmado na fonte |
| material cobrado por **chapa inteira**, ocupada ou não | confirmado — no modelo antigo |
| fabrico cobrado por **tempo de máquina** | confirmado |
| encaixe apertado baixa o fabrico | confirmado — mas **$1,90 nesta caixa**, medido na cotação, não a ordem de grandeza do exemplo deles |
| gravação: é tempo, e *linha* vs *área* inverte-se conforme a geometria | confirmado |
| **afastamento mínimo à borda da chapa** | **não publicado** |
| **preço do acrílico transparente por chapa** | **não publicado** — só «from $0.09» |
| se o site novo ainda cobra chapa inteira, ou rateia por peça | **não confirmado** |

⚠️ **A Ponoko mudou de modelo.** O site actual dá cotação instantânea **por
peça** («from $50 for just 1 … 93% off for 10,000») e diz que os templates
antigos já não são precisos. A aritmética publicada no blogue é anterior a essa
mudança.

> **A coisa de maior valor é carregar UMA peça pequena e ler a cotação.** Isso
> responde de uma vez ao modelo de preço, ao afastamento à borda (a ferramenta
> recusa geometria fora de limites) e ao preço do acrílico — no motor que
> existe hoje, e não em documentação que é anterior à mudança dele.

⚠️ **Dois números que aparecem em pesquisa e NÃO são o que parecem:** os «$12»
do exemplo do blogue são *brushed gold on black acrylic*, e coincidem por acaso
com a sobretaxa de impressão de uma P2 — nenhum dos dois é acrílico
transparente. E «2 cêntimos por polegada de corte» não se confirma em página
nenhuma deles.

⚠️ **Os 3 mm de margem à borda que o gerador exige são NOSSOS.** A Ponoko diz
«não uses a aresta crua da chapa para a tua peça» e não publica número. Os
números que publicam são entre **peças** (1 mm no encaixe, 2 mm nos impressos).
Facto de fabrico sem fonte citada é hipótese, e diz-se como hipótese.

## Procedência

Nasceu a 06/09/2026 da cicatriz **15cf** (`docs/battle_scars.md`): as duas PCB
tinham sido dimensionadas contra desenhos de uma caixa que nunca foi cortada, e
as duas correcções seguintes foram para caixas reais mas erradas. Ver também
`caixa/README.md`, que põe as quatro caixas lado a lado.
