---
name: design-review
description: Portão de saída obrigatório para qualquer artefato visual — SVG, esquemático, build sheet, render de PCB, slide, mockup, plot. Use SEMPRE que produzir ou editar um visual, antes de o apresentar. O passo 1 é a ferramenta `tools/svg_colisoes.py`, que pode dizer não; só depois entra o olho. Apanha texto sobre texto, texto sobre forma, elementos a sair da página, rótulos sobre fios, e a armadilha da vista velha (editar depois do último olhar). Dispara em qualquer entrega de .svg/.pdf/.png com layout, em "design review", "confere o visual", ou no dB a reportar sobreposição — mas o objetivo é correr SEM ser pedida.
---

# design-review

> **Esta é a versão do Cotucatronics, e vive no repo de propósito.** Absorveu a
> skill homónima do plugin (que vive num cache de sessão e não sobrevive) e a
> **emenda de 05/09**, que acrescentou o passo 1. Uma skill por assunto: se
> reaparecer a do plugin, esta manda, porque é a que tem o portão.

Um artefato visual entregue sem inspeção é um artefato **não verificado** — a
versão gráfica do instrumento que não testemunha a si mesmo.

## 0 · A regra de ferro

> **Nenhum visual sai sem que o RENDER FINAL tenha sido VISTO — e qualquer
> edição invalida toda inspeção anterior.**

O segundo termo é o que mais morde: "corrigi e exportei" não é "corrigi e
conferi". Editou → renderizou → **olhou de novo**. Sem exceção, inclusive para
"mudanças triviais de uma linha" — foi uma mudança de uma linha (mover uma
nota) que atravessou um componente.

---

## 1 · O PORTÃO: `tools/svg_colisoes.py`, e vem ANTES do olho

**Emenda de 05/09.** Quatro entregas com "revisão visual limpa" tinham
sobreposições. Duas eram visíveis e passaram porque o olho olhou o todo e não
a fatia; **duas eram de meio pixel** — texto a tocar o risco de uma cruz,
rótulo a cruzar uma linha tracejada cinzenta — invisíveis em qualquer render, e
mesmo assim defeito.

> **O olho é o instrumento errado para sobreposição fina.** Serve para
> composição, legibilidade, hierarquia. Não serve para "este texto toca esta
> linha?", que é pergunta de geometria e tem resposta exata.

```bash
python3 tools/svg_colisoes.py ficheiro.svg
```

**Zero colisões é o portão** — e ele sabe dizer não, que é o que o torna
portão: um script que o chama pára.

⚠️ **São TRÊS desfechos, não dois, e o terceiro não é reprovação:**

| saída | quer dizer |
|---|---|
| **0** | verificado, zero colisões — **passa** |
| **1** | verificado, há colisões — **defeito no desenho** |
| **3** | **NÃO VERIFICÁVEL** — não consegui olhar. Nem aprovação nem reprovação |

## 1b · EMENDA DE 22/09 — o portão mediu 78 figuras com a caixa errada

Durante semanas o manual teve **15 das 78 figuras com sobreposição** sem
ninguém saber, e quando a medição foi feita ela estava **errada nas duas
direcções**. Três causas, e todas valem para qualquer SVG teu:

1. **O SVG tem de declarar `xmlns`.** Sem ele o `ElementTree` parseia sem
   espaço de nomes, o verificador não acha o `<style>`, e cai no omisso de
   12 px e `text-anchor: start` **em silêncio**.
2. **Tamanho e alinhamento vão como ATRIBUTO, não só em classe.** O
   verificador não resolve CSS a sério — não há contexto nem especificidade.
   Ele resolvia `.rot` como 12 px porque um desenho diferente tinha uma regra
   com o mesmo nome de classe. Desde a v0.9.0 ele **só aceita selectores de
   uma classe só e descarta classes com regras em desacordo**, e **diz quantos
   textos teve de adivinhar** — mas a resposta certa é não o obrigar a
   adivinhar. Um rótulo centrado modelado como se começasse no ponto está
   deslocado de **meia largura**, e isso inventa colisões E cala colisões.
3. **Figura gerada em lote precisa do seu próprio portão.** A regra "toda
   imagem passa pelo verificador" era cumprida só nos desenhos NOVOS; ninguém
   ia buscar 78 SVGs embutidos num HTML um a um. Para o manual existe agora o
   `tools/colisoes_manual.py`, que os extrai, **embute o `<style>` do
   documento** e corre o portão em cada um.

> **E o portão não vê tudo.** Um rótulo que fugiu para longe da peça não
> colide com nada e não serve para nada. O gerador do manual passou a **medir
> o desvio** e a relatá-lo. Portão de geometria pede gémeo de legibilidade.

Ler o 3 como "há colisões" é o erro fácil, porque ambos são ≠ 0 e querem dizer
o contrário um do outro. O 3 sai quando não há `<text>` nenhum — as letras
podem estar como paths — e aí *"zero colisões" seria uma frase verdadeira
sobre nada*. **Verificação sem objeto é a classe que a skill `audit` persegue,
e esta ferramenta recusa-se a cometê-la.**

O que ele testa:

| | |
|---|---|
| **texto × texto** | sempre defeito |
| **texto × segmento** | geometria real (Liang–Barsky), não bbox de path — um path em L não acusa o texto que está no canto vazio dele |
| **texto × borda de forma** | texto **dentro** de um rect/círculo é etiqueta e é legítimo; texto a **cruzar** a borda é defeito |
| formas com `opacity < 0,5` | sombreado decorativo, não contam |

⚠️ **A largura do texto é estimada** (0,55 em por caractere, Helvetica), e é
**conservadora de propósito** — acusa uns pixels antes do toque real. É o
comportamento certo para um portão: *falso positivo custa mover um rótulo;
falso negativo custa uma entrega errada.*

### Cada acusação diz QUANTO, e isso muda o que se faz com ela

A partir da v0.5.0 cada linha traz a profundidade da sobreposição no eixo mais
raso, em unidades da folha:

```
!! texto×texto  V- 4              42 um     <- ruído do modelo
!! texto×borda  V- rect 54,252    1.65      <- a sério, vai-se olhar
```

⚠️ **Nasceu de eu ter reportado mal.** O portão acusou um par `V-`/`4` e eu
disse ao dB que era colisão a sério. Fui medir: **42 micrómetros** — dentro do
erro do próprio modelo, que supõe ascendente de 0,75 do corpo, e a fonte de
traços do KiCad não tem essa métrica. O símbolo estava correcto e simétrico;
não havia nada para endireitar.

> **Veredito binário obriga quem lê a ir calcular à mão.** Medir não baixa a
> régua — o portão continua a acusar — dá contexto a quem decide. Abaixo de
> ~0,1 da unidade da folha, suspeitar do modelo antes de suspeitar do desenho.

E o contrário também vale: na mesma folha havia **cinco** acusações de 0,9 a
1,65 que eu não tinha distinguido do ruído. Quantificar separou-as.

**A linha de resultado vai no relatório**, sempre: `N textos, M segmentos, 0
colisões`. **Sem essa linha, a revisão não aconteceu.**

### E numa PLACA: `--layers <F|B>.Silkscreen`, e o DRC ao lado

A serigrafia de um `.kicad_pcb` exporta-se por camada e passa pelo mesmo portão:

```bash
kicad-cli pcb export svg --output /tmp/s.svg --layers F.Silkscreen \
    --page-size-mode 2 --exclude-drawing-sheet placa.kicad_pcb
python3 tools/svg_colisoes.py /tmp/s.svg
```

⚠️ **Numa placa o mesmo achado tem GRAU diferente**, e a tabela dos graus vive
na skill `kicad_layout` **§11g-bis** — não aqui, para as duas não derivarem. O resumo
para usar ao ler a saída: `texto × texto` é sempre defeito; `texto × contorno de
OUTRA peça` é defeito; `texto × contorno da PRÓPRIA peça` é **aviso**, evita-se
quando há sítio e embarca-se quando não há.

⚠️ **Eu já escrevi aqui que era "o caso normal" enquanto a `kicad` dizia "na
placa é defeito"** — duas skills, duas frases afirmativas, nenhuma marcada. Foi
preciso ir medir para ver que nenhuma das duas estava certa sozinha: faltava o
grau. **Quando duas skills da casa afirmam coisas opostas, a resposta não é
escolher — é a distinção que falta às duas.**

Classificar por «são a mesma referência?» separa os casos: das 9 `silk_overlap`
do cérebro, 9 eram a peça sobre si própria.

⚠️ **E corre-se o DRC do KiCad ao lado, porque numa placa é ele quem manda.**
`silk_overlap`, `silk_over_copper`, `silk_edge_clearance`. Os dois responderam
diferente — `0 texto×texto` contra `9 silk_overlap` — e foi ir medir a
discordância que destapou quatro defeitos na ferramenta que arruma a
serigrafia. **Dois verificadores que discordam são informação; escolher o que
agrada é deitá-la fora.**

⚠️ **A comparação antes/depois mede-se no MESMO contexto.** O KiCad acha o
`.kicad_pro` pelo nome do ficheiro: uma cópia chamada `_teste_x.kicad_pcb` cai
nas regras por omissão e inventa violações que a placa não tem. Ou a cópia vive
na pasta do projecto com o nome que casa, ou se mede o antes e o depois no
mesmo sítio — e aí lê-se a **diferença**, nunca o valor absoluto.

### O que fazer quando a ferramenta não alcança

PNG de plot, PDF, render do KiCad: ela não lê. Aí vale a regra antiga —
fatiar em regiões e olhar cada uma — **mais** a lição de 05/09: *olhar o todo
não substitui olhar a fatia, porque o todo esconde exatamente o que é
pequeno.*

### O KiCad tem caminho, mas com `--esquematico`

```bash
kicad-cli sch export svg --output /tmp/x  ficheiro.kicad_sch
python3 tools/svg_colisoes.py /tmp/x/ficheiro.svg --esquematico
```

⚠️ **Sem a bandeira, um esquema limpo acusa dezenas — e todas são convenção.**
Medido a 05/09 nas três folhas do revB:

| folha | sem o modo | com `--esquematico` |
|---|---|---|
| `cotuca_energia` | 39 | **6** |
| `cotuca_preamp` | 74 | **7** |
| `cotuca_soquete` | 116 | **26** |

A razão é de desenho, não de ferramenta: **num esquemático, texto a tocar
linha é a convenção.** Um rótulo de rede *prende-se* ao fio — é o que um
rótulo é; um número de pino *assenta* na perna. Dos 74 do pré-amp, 67 eram
`AGND` (13×), `VBIAS`, `V+PRE`, `PWR_FLAG` e os pinos 1–8.

> **Acusar a convenção é pior do que não acusar nada:** ensina a ignorar o
> portão, e é o anti-padrão do §6 desta mesma skill.

O modo mantém `texto × texto` e `texto × borda`, que continuam a ser defeito
num esquemático — foi assim que apareceu um `V-` por cima do pino `4` no
pré-amp, que três revisões de olho não tinham visto.

### ⚠️ Há DOIS portões para esquemático, e não são o mesmo

| | lê | vê |
|---|---|---|
| `tools/colisoes_sch.py` | o **fonte** `.kicad_sch` | coordenadas exactas em mm, antes de virarem curvas |
| `tools/svg_colisoes.py --esquematico` | o **render** exportado | o que está realmente desenhado na folha |

**Correr os dois, e a discordância é informação.** No pré-amp o do fonte diz
**117 textos, 0 colisões**; o do render diz **194, 7** — entre elas um `V-`
por cima do pino `4`.

**O mecanismo está medido (06/09), e o cego é o do fonte.** Ele salta a secção
`lib_symbols` de propósito — está na docstring dele: *"tudo que imprime
letras, **fora do lib_symbols**"*. Lê `label`, `global_label`,
`hierarchical_label` e as propriedades `Reference`/`Value`. Logo **não vê
nomes nem números de pino**, que é onde os símbolos guardam as letras. E não
vê o **bloco de título e a moldura** — `Date:`, `Rev:`, `File:`, `Id: 1/1`,
as coordenadas de borda — porque o KiCad desenha isso do *worksheet* e não do
`.kicad_sch`: 27 dos textos do render não existem no ficheiro.

Foi exactamente por isso que só o render viu o `V- 4`: `V-` é **nome de
pino** e `4` é **número de pino**, e `(number "4")` aparece **1 vez dentro do
`lib_symbols` e 0 fora**. Os dois lados da colisão vivem na secção que o
outro salta.

> **Nenhum dos dois substitui o outro, e agora sabe-se porquê:** o do fonte
> tem as coordenadas exactas mas não vê o que o símbolo desenha; o do render
> vê tudo o que está na folha mas mede sobre o desenho. Um `0` de qualquer um
> **não autoriza dispensar o outro**.

> Dois verificadores que respondem à mesma pergunta e discordam: o correcto
> não é escolher o que agrada, é registar a discordância e ir medir porquê.

⚠️ E o cabeçalho do `colisoes_sch.py` justifica-se com uma premissa que hoje
se mediu **falsa** — diz que o `svg_colisoes` devolve sempre código 3 nos
exports do KiCad *"porque não há um único `<text>` para medir"*. Há **222**,
invisíveis (`opacity="0"`) mas com `textLength`. A ferramenta continua útil; a
razão escrita para ela existir é que está velha.

O portão elétrico é a skill `kicad`; este é o visual, e nenhum substitui o
outro.

### ⚠️ E na PLACA a serigrafia tem portão próprio, que vive na `kicad`

Um render de PCB não se revê a olho para caçar texto sobreposto: o `kicad-cli
pcb drc` **conta** — `silk_overlap`, `silk_over_copper`, `silk_edge_clearance` —
e o `kicad/arruma_silk.py` arruma. A regra completa está na skill **`kicad`
§11g-bis** da `kicad_layout`, e não se repete aqui: **duas skills a dizerem quase o mesmo
derivam, e aí ninguém sabe qual manda.**

O que fica deste lado é a metade que a contagem não responde:

| pergunta | quem |
|---|---|
| há texto por cima de texto, de cobre, ou fora da placa? | `kicad` — conta |
| o texto **lê-se** no tamanho a que vai ser impresso? | esta skill — olha o render |
| a referência aponta para a peça **certa**? | só o olho |

⚠️ **Contagem zero não autoriza dispensar o olho** — é o mesmo corolário do
§9b da `kicad`, e aqui vale igual: autoriza olhar sabendo onde.

⚠️ E há uma armadilha própria do PCB: **arrumar a serigrafia empurra o texto
para o obstáculo que ainda não está a ser verificado.** Medido — afastar dos
pads levou as sobreposições de 18 para 5 e o texto-sobre-cobre de 6 para 12. Se
vires uma contagem melhorar e outra piorar na mesma passagem, não é progresso: é
o defeito a mudar de sítio.

---

## 2 · A ordem, fixa

```
gerar → svg_colisoes (0) → fatiar e olhar (composição) → entregar
```

**Qualquer edição depois de qualquer um dos três reinicia os três.**

---

## 3 · O laço do olho

1. **Renderizar no formato de entrega** (SVG→PNG; `.kicad_sch`→SVG via
   `kicad-cli`; HTML→screenshot). Inspecionar o fonte não vale: a sobreposição
   só existe no render.
2. **Ver em fatias.** Imagem alta (>1200 px) é cortada em 2–4 faixas com
   **~8% de sobreposição** entre elas — defeito na emenda dos cortes escapa de
   cortes sem sobreposição — e cada faixa passa pelo olho.
3. **Varrer com a lista fechada** (abaixo). Não é "olhar se está bom": é
   procurar cada classe nomeada, uma a uma.
4. **Defeito achado → corrige → VOLTA AO PASSO 1**, o do portão, não o do olho.
5. Só então empacotar. Empacotar antes de conferir gera o "zip com defeito
   conhecido dentro".

## 4 · A lista fechada

Depois do portão, o olho já não procura toque. Procura isto:

- **Composição e hierarquia** — o que salta primeiro é o que importa primeiro?
- **Elemento × borda**: nada cortado pela margem da página/viewBox; texto longo
  à direita é o suspeito nº 1.
- **Fio × fio / fio × forma** onde o cruzamento sugere ligação falsa. ⚠️ Ver a
  skill `kicad` §4: o **olho não pode arbitrar** ligação — só o netlist.
- **Emendas**: fio que PARECE contínuo mas tem vão (o D2 do grampo teve 17 px
  de vão invisível no fonte, visível no render).
- **Legibilidade no tamanho final**: fonte < ~9 px no render de entrega é
  ilegível em mobile; conferir na largura que o dB vai ver.
- **Sobreposição de camadas** (z-order): elemento desenhado depois a cobrir
  informação de antes.
- **Coerência rótulo-desenho**: o texto diz "12 baldes", o desenho tem 13.

## 5 · Como isto se torna automático

1. **Nos geradores:** o `.py` que escreve o SVG chama o verificador antes de
   copiar para a entrega; com colisões, escreve para `/tmp` e **recusa
   entregar**. Um `assert` no fim do gerador.
2. **Na skill:** o passo 1 é este, e a linha de resultado vai no relatório.
3. **No que não é SVG:** fatiar e olhar, declarando que a ferramenta não
   alcançou.

## 6 · Anti-padrões nomeados, os que já aconteceram

- **Exportar após editar sem re-ver** — o caso da nota sobre o C3.
- **Conferir só a região editada** — o conserto empurra o vizinho.
- **Confiar no fonte** ("as coordenadas parecem certas") — sobreposição é
  propriedade do render, não do fonte.
- **Corte sem sobreposição** — o defeito mora na emenda.
- **"É só um texto, não precisa"** — texto é o elemento que mais colide.
- **⚠️ "A revisão visual passou"** — sem a linha do portão, não passou. Foi
  esse o padrão que a emenda de 05/09 veio fechar, e tinha três reincidências.

## 7 · Relação com as outras

| pergunta | quem responde |
|---|---|
| há sobreposição? | **esta skill, passo 1** — geometria, não olho |
| o desenho lê-se? | **esta skill, passo 3** |
| o ficheiro abre? as ligações existem? | skill **`kicad`** — ERC e netlist |
| o código está coerente? | skill **`audit`** |

As três partilham a regra: **o que passou fica declarado.** Inspeção
não-declarada será refeita ou, pior, presumida.
