---
name: kicad
description: Desenhar, gerar, verificar e entregar esquemáticos KiCad — provar que o ficheiro ABRE antes de tudo, ERC a cada edição, ponto de junção como semântica, os três padrões de cruzamento, grade de ligação, o que todo esquema declara, e o contrato com o BOM. Use ao desenhar ou alterar esquemáticos e geradores, ao atribuir footprints ou montar BOM, e antes de dar qualquer folha por verificada.
---

# KiCad por script — as regras que custaram folhas

## FRONTEIRA COM A `design-review`

Esta skill e o portao **ELECTRICO** de um esquematico; a `design-review` e o
**VISUAL**. Nao sao dois niveis de rigor da mesma pergunta — sao perguntas
diferentes, e **nenhuma substitui a outra**:

| pergunta | quem responde | o que NAO ve |
|---|---|---|
| o ficheiro **abre**? | `kicad-cli sch erc` (esta skill) | nada disto aparece no desenho |
| as ligacoes **existem**? | ERC + netlist (esta skill) | pino e fio no mesmo sitio, sem ligacao, sao **identicos** no render |
| o desenho **le-se**? | `design-review` | texto por cima de texto nao aparece em netlist nenhum |

⚠️ **Uma skill que mande "renderizar e olhar" e nao mande confirmar que o
ficheiro abre esta errada por OMISSAO** — e o silencio dela tem a mesma cara de
uma folha boa. Se existir outra skill de esquematicos, ela deve APONTAR para
esta em vez de repetir o conteudo: duas a dizerem quase o mesmo derivam, e ai
ninguem sabe qual manda.

## 0. A REGRA QUE ESTA SKILL EXISTE PARA IMPOR

> **"O verificador passou" e "a folha abre" são perguntas DIFERENTES.**

Em 01/09 foram entregues duas folhas com o verificador de netlist limpo e a
revisão visual feita três vezes. **Nenhuma das duas abria no KiCad.**

```
gerado:    (symbol "D_V_0_1_0_1")   ← quatro números
correcto:  (symbol "D_V_0_1")       ← dois
```

O `kiutils` acrescenta `_unitId_styleId` sozinho, e o gerador já punha o sufixo
à mão. O KiCad separa esse sufixo para deduzir o nome do símbolo pai, encontrava
um pai que não existe, e **recusava a folha inteira** — com uma única linha de
erro, `Failed to load schematic`, que não aponta para lado nenhum.

O verificador de rede não podia apanhá-lo: ele confere **conectividade**, não a
**gramática do ficheiro**. Duas perguntas, e só uma estava a ser feita.

**Primeiro comando de qualquer entrega:**

```
kicad-cli sch erc <ficheiro>     # se falhar a carregar, nada mais importa
```

## 1. O `kicad-cli` provavelmente EXISTE — procura pelo SÍTIO, não pelo nome

```
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli      # macOS
```

Não está no `PATH`. Uma sessão inteira deu-o por ausente porque correu `which
kicad-cli`, e por isso entregou folhas sem ERC. **Procurar dentro do bundle
antes de declarar que não há ferramenta.**

Subcomandos: `erc`, `export` (svg, pdf, netlist, bom), `upgrade`.

## 2. Bisectar o "Failed to load schematic"

O erro não diz onde. O método que funciona é reduzir até ele desaparecer:

1. Ficheiro mínimo (cabeçalho + `paper` + `lib_symbols` vazio) — **carrega?**
2. Acrescentar UM bloco de cada vez: `wire`, `junction`, `global_label`,
   `lib_symbols`.
3. Dentro do bloco culpado, **um símbolo de cada vez**.
4. Dentro do símbolo, remover partes até carregar.

Em 01/09 isto levou do ficheiro inteiro ao token exacto em seis passos.

⚠️ **E prova primeiro que a FERRAMENTA funciona**: corre o ERC num `.kicad_sch`
alheio conhecido. Sem isso, "falha a carregar" tanto pode ser o ficheiro como o
`kicad-cli`.

## 3. Grade de ligação: 1,27 mm, e não é estética

O KiCad liga por **coincidência de coordenada**, e a grade do editor é 1,27 mm.
Coordenadas em milímetros inteiros ligam **hoje** — porque pino e fio partilham
o mesmo valor — e partem **no dia em que alguém arrastar um fio**: o cursor
salta para a grade, a ponta vai para 97,79 e o pino fica em 98,00. Os dois
continuam DESENHADOS encostados, e a ligação já não existe. Sem aviso.

**Encaixar, e onde:**

| encaixar | não tocar |
|---|---|
| colocação dos símbolos | desenho dentro do símbolo |
| pontas de fio, junções, rótulos | offsets dos pinos (já costumam estar em múltiplos de 2,54) |

**Medir antes de transformar:** contar quantos pontos de cada categoria estão
fora. Em 01/09 os pinos internos estavam 100% na grade e só a colocação estava
fora — arredondar o desenho teria distorcido triângulos de díodo para não
consertar nada.

**A garantia:** arredondar é **função pura**, logo dois pontos que coincidiam
continuam a coincidir — nenhuma ligação se perde. O risco é o inverso: dois
pontos DISTINTOS a menos de meia grade caírem no mesmo sítio e **inventarem**
uma ligação. Verificar sempre, e **levantar** em vez de avisar: ligação a mais
não aparece em lado nenhum até à fabricação.

**Aceitação:** netlist idêntico antes e depois.

## 4. O PONTO DE JUNÇÃO É SEMÂNTICA, não decoração

Esta é a família de defeito mais cara do desenho esquemático, porque **nenhum
dos dois lados dá erro em lado nenhum**:

| | significa | quando está errado |
|---|---|---|
| cruzamento **sem** ponto | os fios **não** se ligam | ponto a menos = **circuito aberto** |
| cruzamento **com** ponto | os fios **ligam-se** | ponto a mais = **curto no papel** |

🚫 **Cruzamento de QUATRO VIAS com ponto é proibido.** É ambíguo: ninguém
distingue "duas redes que se cruzam e uma tem junção" de "quatro fios todos
ligados". Parte-se em dois cruzamentos em T, desencontrados.

**E o caso que se descobre tarde:** um **pino a meio de um segmento de fio NÃO
liga**. Só em ponta de fio, ou com junção explícita. Um `PWR_FLAG` posto a meio
fica solto — e no desenho está encostado ao fio, indistinguível de ligado.

⚠️ **É a mesma família:** em todos estes casos, o desenho está bonito e a rede
está errada. **O olho não pode arbitrar** — só o netlist.

## 4b. A família dos CRUZAMENTOS, com os padrões nomeados

Cruzamento não é falha de cuidado; é o desenho a dizer que o **arranjo** está
errado. Três padrões, e cada um tem conserto próprio:

**C1 — o nó com duas cargas em direcções opostas.** No não-inversor, o `Rf` sobe
da entrada `−` e o `Rg` desce dela, e o percurso atravessa o fio da entrada `+`.
🚫 **Não se resolve movendo peças** — o cruzamento é topológico. Resolve-se
pondo **as duas cargas a sair na mesma direcção**, com o sinal a entrar por
baixo.

**C2 — o trilho desenhado como linha longa.** Uma linha de alimentação que
atravessa a folha cruza tudo o que encontra. Resolve-se com a alimentação em
**sub-secção própria**, ligada por **tocos rotulados** — o rótulo é o fio.

**C3 — colisão de rótulo** com fio ou com outro rótulo. É o único dos três que a
`design-review` apanha, e o único que não muda a topologia.

## 5. Grelha ANTES de emitir, e gerar por script

**Encaixar na grade é passo de EMISSÃO, não de revisão.** Ver §3 para o
mecanismo e para as garantias. O que esta secção acrescenta é o momento: se o
encaixe corre depois de a folha ser dada por boa, ele **desloca tudo** e
invalida a inspecção que a aprovou.

**E desenhar por script, não à mão.** Um esquemático gerado é reproduzível,
diffável e corrigível na fonte; um desenhado à mão é um artefacto que ninguém
sabe refazer. Em 01/09 foram **176 avisos de fora-da-grade** numa folha desenhada
com coordenadas escolhidas a olho — e o conserto foi uma função de três linhas
no gerador, aplicada a tudo de uma vez.

## 6. `power_pin_not_driven`: declarar, não isentar

Símbolos caseiros não declaram `power_out`, e todo pino `power_in` fica órfão.
A resposta é um **PWR_FLAG por rede de energia** — símbolo com pino `power_out`,
referência começada por `#` (convenção: não entra na BOM).

⚠️ **UM por rede, e só um.** Dois `power_out` na mesma rede são conflito: troca-se
um erro por outro.

⚠️ **A referência `#...` NÃO se mostra.** Além da convenção, é colisão certa: a
etiqueta cai por cima do rótulo global que está ao lado.

⚠️ **E a bandeira não vai na coordenada do rótulo** — tapa-o. Vai num ponto
livre do mesmo fio, **com junção** (ver §4).

## 7. Isenções: com prazo, com razão, e nunca em bloco

Isenção generosa transforma verificador em carimbo. Cada uma leva **porquê** e
**até quando**.

⚠️ **Isenção que se INVERTE é a mais valiosa de marcar.** `isolated_pin_label`
numa folha isolada é falso positivo legítimo; **dentro do projecto único passa a
ser rede sem destino** — defeito real. A mesma linha do relatório muda de
significado, e quem não marcou a data não repara.

⚠️ **E declarar onde a isenção NÃO está aplicada.** O KiCad guarda-as no
`.kicad_pro`; sem projecto, o registo é só um documento. **Registo sem aplicação
é promessa, e diz-se que é.**

## 7b. O que TODO esquema declara

Um esquemático que não diz estas coisas obriga quem o lê a adivinhar — e quem
adivinha, adivinha errado uma vez em cada dez:

- **A tensão, e DE ONDE VEM.** «5 V» não chega: 5 V do step-down, do USB, ou do
  regulador local são três redes diferentes com três comportamentos diferentes.
- **O que é POR CANAL e o que é PARTILHADO.** Duas cadeias que partilham uma
  fonte não são duas cadeias independentes, e o desenho tem de o mostrar.
- **⚠️ O que é AJUSTE DE EXEMPLAR.** Trimpot de ganho, casamento de piezo,
  nível de exciter: valores que pertencem a **esta** unidade e não à série.
  Sem essa marca, alguém copia-os para a próxima peça — e leva a calibração de
  um corpo para outro corpo.
- **Terras distintos com SÍMBOLOS distintos**, quando são nós distintos. Um
  `GND` e um `AGND` desenhados com o mesmo símbolo são uma ligação que ninguém
  vê e ninguém consegue discutir.
- **Valores que dependem de uma PREMISSA, com a premissa escrita.** «R1 4M7»
  não diz nada; «R1 4M7 com piezo de ~20 nF corta em 1,7 Hz» diz o que se perde
  se o piezo mudar.

## 7c. O contrato com o BOM

O esquemático e a lista de compras são o mesmo documento visto de dois lados. O
que os mantém casados:

- **O designador é CHAVE.** `C7` no desenho e `C7` no BOM são a mesma peça, e
  qualquer divergência é defeito — não é questão de estilo.
- **O encapsulamento é parte da COMPRA**, não detalhe de layout. 0805 e 1206 são
  peças diferentes com o mesmo valor.
- **MPN sim; código de distribuidor com RESSALVA** — ele muda, esgota e difere
  por região. E **nunca inventado**: um código plausível e errado custa uma
  encomenda inteira, e parece certo até chegar a caixa.

⚠️ **Footprint suposto é respin.** O que depende de medir peça em mão — módulos,
conectores, botões — fica **vazio até a peça chegar**. Vazio é honesto; adivinhado
não é.

## 8. O gerador é a FONTE

Nunca editar o `.kicad_sch` à mão: as duas versões divergem e ninguém sabe qual
manda. Todo conserto entra no `.py` e regera-se.

**E a fonte tem de correr fora da máquina do autor.** Em 01/09 os geradores
tinham `/mnt/user-data/outputs/` e `/home/claude/*.svg` gravados — rebentavam
com `FileNotFoundError` na primeira tentativa noutro computador. Caminhos
relativos ao próprio ficheiro (`pathlib.Path(__file__).parent`), sempre.

## 9. O laço de saída, e ele inclui o ERC

A `design-review` diz: *editou → renderizou → olhou de novo*. Aqui vale o mesmo
para a verificação eléctrica, e foi por a esquecer que se entregou uma folha com
pinos soltos:

> **Editar invalida TODAS as inspecções anteriores, não só a visual.**

Depois de qualquer alteração ao gerador:

1. regerar
2. `kicad-cli sch erc` — **e ler a contagem de erros**
3. netlist, comparado com o de antes
4. render, e **olhar**
5. só então empacotar

Saltar o 2 depois de mexer na geometria foi o que produziu
`pin_not_connected` numa folha que já tinha dado zero erros.


## 2b. RUÍDO DE VÍRGULA FLUTUANTE: o número que nenhuma ferramenta lê

`h/2 - 1.27 - 6*2.54` não dá zero. Dá **`1.7763568394002505e-15`**, e o KiCad
aceita-o sem se queixar. Nenhum leitor escrito com `(-?[\d.]+)` — que é como
toda a gente escreve — apanha essa linha.

O efeito não é um número feio. É **desaparecimento silencioso**: os pinos 13 e
14 de um IDC26 sumiram de QUATRO geradores ao mesmo tempo, e a folha saiu com
24 rótulos em vez de 26. Sem erro, sem aviso, sem ERC. As duas vias que
faltavam eram terra.

**As duas metades do conserto, e são precisas as duas:**

1. **Na geração** — todo número que entra num ficheiro KiCad passa por um
   formatador. `round(float(v), 4)` mata a notação científica na origem; 0,1 µm
   está três ordens de grandeza abaixo de qualquer tolerância de fabrico.
   ```python
   def num(v):
       return "%g" % (round(float(v), 4) + 0.0)   # o +0.0 troca -0.0 por 0
   ```
2. **Na leitura** — toda regex de coordenada aceita expoente:
   `(-?[\d.]+(?:[eE][-+]?\d+)?)`. Consertar só a geração deixa a ferramenta
   cega ao próximo ficheiro que venha de outra mão.

> **Ao contar coordenadas suspeitas, não uses `[0-9]e-?[0-9]+` cru:** ele apanha
> UUIDs em hexadecimal e infla a contagem. Ancorar em `\((at|xy|start|end) `.

## 2c. TODO LEITOR CONTA O QUE LEU

A lição a seguir apareceu **três vezes no mesmo dia**, em três ferramentas
diferentes, e uma delas era o verificador escrito para apanhar as outras duas:

| leitor | tinha | leu | o que anunciou |
|---|---|---|---|
| `pinos_de()` (×4 geradores) | 26 pinos | 24 | desenhou 24 rótulos |
| `contrato_idc.py` v0.1.0 | 26 vias | 24 | *"as 26 vias batem"* |
| `colisoes_sch.py` v0.3.0 | 40 propriedades | 0 | *"0 colisões"* |

> **Um leitor que perde linhas em silêncio não reprova nada — aprova tudo.**

**A guarda, e é barata:** contar quantos blocos existem e exigir que a leitura
os apanhe todos.

```python
blocos = len(re.findall(r'\(pin\s', b))
if blocos != len(lidos):
    raise SystemExit("*** o símbolo tem %d pinos e a leitura apanhou %d "
                     "— LEITURA INCOMPLETA, nada a jusante é confiável"
                     % (blocos, len(lidos)))
```

E a guarda **conta o mesmo universo que o leitor**: contar propriedades
escondidas (`(hide yes)`), que o leitor salta de propósito, dá alarme falso — e
guarda que dá alarme falso é guarda que se desliga.

**Provar a guarda pede DOIS testes**, porque um só engana:

- injectar o defeito **conhecido** → a regex nova lê-o, a guarda cala-se;
- injectar um defeito **desconhecido** (por exemplo renomear `(number` para
  `(numero`) → a guarda **reprova**.

Só o primeiro teste prova a regex, não a guarda.

## 2d. O BLOCO COLADO À LINHA ANTERIOR

Um `.kicad_sch` pode ter `)\t(symbol` — o bloco seguinte a começar na MESMA
linha em que o anterior fecha. Uma varredura ancorada em `\n\t(symbol` salta-o
sem um aviso. Foi assim que, numa renomeação de 27 referências, **uma** escapou;
o relatório disse "26 renomeadas" e eu aceitei o número sem o comparar com 27.

> **Varre por contagem de parêntesis, não por indentação.** E quando um
> relatório diz "N feitos", compara N com o que era esperado.

## 5b. O SINAL DO `y` VIVE NUM SÍTIO SÓ

Na **folha** o `y` cresce para BAIXO. No **símbolo** cresce para CIMA. Escrever
as linhas de continuação de uma nota com `y - 3.8` — que é o que a intuição do
símbolo dita — imprime a nota **de trás para a frente**: o render mostra o
último período primeiro.

Aconteceu em **9 notas de dois geradores ao mesmo tempo**, porque a coordenada
estava repetida em cada sítio de chamada. O conserto não é acertar as 9:

```python
def nota(x, y, *linhas, tam=1.6, passo=3.81):
    """O SINAL DO Y VIVE SÓ AQUI."""
    for i, s in enumerate(linhas):
        texto(s, x, y + i * passo, tam if i == 0 else 1.27)
```

⚠️ **Cuidado com o nome:** `rot, nota = VIAS[n]` num laço tapa a função `nota()`
e o erro que sai é `TypeError: 'str' object is not callable`. Renomeia a
VARIÁVEL, nunca a função.

⚠️ **O KiCad não desenha emoji.** Um `⚠️` numa nota sai como quadrado vazio.
Usa `(!)`.

## 6b. ANOTAÇÃO: a referência TEM de terminar em dígito

`SWL`, `SWR`, `RVA`, `R0A`, `C1B`, `DA`, `JA` lêem-se muito bem e **o KiCad
trata-as como NÃO ANOTADAS**. O aviso é discreto:

```
Warning: schematic has annotation errors, please use the schematic editor to fix them
```

sai do `kicad-cli sch export netlist`, **não do ERC**, e é fácil passar-lhe por
cima. O risco não é o aviso: é o editor **renumerar essas referências sozinho
ao gravar**, e a correspondência com desenhos, BOM e documentos quebrar em
silêncio.

> **A referência é `PREFIXO` + `NÚMERO`. O significado (canal, lado, função)
> mora no VALOR e na nota, que é onde se lê.**

Esquema que funciona: **a dezena diz o canal** — canal A `R10..R14`,
canal B `R20..R24`, `C11..C14` / `C21..C24`, `RV1`/`RV2`. E a referência é única
em TODA a hierarquia, não por folha: escolher a dezena sem verificar as outras
folhas cria um choque (aqui, o soquete já usava `R20`/`R21` e teve de ir para
`R50`/`R51`).

**A outra causa do mesmo aviso, e é traiçoeira:** unidades do mesmo componente
têm de concordar em **valor** e **footprint**. `U1` unidade 3 dizia
`"OPA1642 8/4"` e as unidades 1 e 2 diziam `"OPA1642"` — e isso sozinho mantém
o aviso de anotação depois de todas as referências estarem correctas. Custou
três hipóteses erradas antes de ser medido.

**O que NÃO era a causa DESTE aviso**, testado e descartado: blocos
`(instances)` em falta ou obsoletos. ⚠️ **Isso não os torna dispensáveis** — são
a causa de OUTRA coisa, a caixa de reparação ao carregar; ver §6c. Foi
precisamente por medir só o aviso de anotação que se concluiu cedo de mais que
`(instances)` não importava.

## 6c. A CAIXA DE REPARAÇÃO AO CARREGAR — e nenhum comando a vê

> *"An error was found when loading the schematic that has been automatically
> fixed. Please save the schematic to repair the broken file or it may not be
> usable with other versions of KiCad."*

**A causa é o `(instances)` em falta nos símbolos.** Todo símbolo colocado
precisa de:

```
(instances
  (project "<nome do projecto>"
    (path "/<raiz>/<folha>"      ; numa folha-filha
      (reference "R50")
      (unit 1))))
```
e, para um símbolo na própria raiz, o caminho é só `/<raiz>`.

⚠️ **O primeiro segmento sai do `.kicad_pro`, não do `(uuid)` do `.kicad_sch`
da raiz.** Os dois podem divergir — aqui divergiam — e o KiCad usa o do
`.kicad_pro`. Escrever o outro produz dados que nascem obsoletos.

⚠️ **Um gerador que mine um uuid novo a cada corrida garante que nunca batem.**
A identidade da raiz é FIXA: constante no gerador e igual à do `.kicad_pro`.
(E um `.kicad_pro` copiado de outro projecto traz a identidade do outro: aqui
dois projectos reclamavam a mesma raiz.)

**O que torna isto perigoso é que NENHUMA ferramenta de linha de comando o
denuncia:**

| ferramenta | o que diz sobre um ficheiro sem `(instances)` |
|---|---|
| `sch erc` | as violações de sempre, nem mais nem menos |
| `sch export netlist` | exporta bem; o aviso que dá é de **anotação**, outra coisa |
| `sch upgrade` | *"Schematic file was not updated"* — o formato já é o corrente |

> **Só o editor repara, e só o editor conta.** Um portão de linha de comando
> completamente verde não prova que o ficheiro abre limpo.

**O método que respondeu, depois de três hipóteses erradas:** pedir ao dB para
abrir, clicar `OK` e **gravar** — e depois `diff` entre o que o KiCad gravou e
o que estava lá. O diff é o KiCad a dizer literalmente o que reparou. Guarda a
cópia ANTES de mandar abrir, senão o diff mistura as tuas edições com as dele.

**E o gerador tem de reproduzir o que o KiCad escreve**, senão a regeneração
seguinte apaga a reparação e a caixa volta. Critério de aceitação: gerar para
uma cópia e comparar os blocos `(instances)` com os do ficheiro que o KiCad
gravou — **iguais, um a um**.

## 7f. O MPN É UMA AFIRMAÇÃO, NÃO UM FACTO

Nenhum verificador apanha um MPN errado. O ERC fica limpo, a netlist fica
limpa, o footprint até pode existir — e a peça que chega não é a peça que o
desenho supõe. **O esquema fica igualmente limpo com o número de peça errado.**

E o que falha é quase sempre **o sufixo**, não a família:

| escolhido | o que a ficha dizia |
|---|---|
| `PTV09A-4020**F**-B103` | `F` = eixo liso; a nota ao lado pedia estriado (`S`) |
| `PEC11R` | pinos a **5,0 mm**; o footprint do KiCad é do Alps, a **2,5** |
| `2920L300/24MR` | não existe; o que há a 24 V é o 3,3 A |

> **Todo MPN entra na BOM com a linha da ficha que justifica cada carácter do
> sufixo.** Sem isso é família com número.

⚠️ **Resumo de busca não é ficha.** Um resumo diz o que alguém entendeu. A
05/09 um resumo disse que um estilo de pot "não servia", e a conclusão foi
reportada e decidida — até um distribuidor descrever a mesma peça de outra
maneira e a contradição obrigar a ir à tabela de estilos. A tabela deu razão
a metade de cada lado.

**Truque que funcionou:** quando o PDF da ficha sai baralhado em texto (são
desenhos em colunas), **renderiza a página e OLHA** — `pdftoppm -png` e ler a
imagem. As cotas estão no desenho, não no fluxo de texto.

**E o stock também é uma afirmação:** stock na LCSC **não é** stock de
montagem da JLC. São armazéns diferentes, e o tier Basic/Extended confirma-se
na biblioteca de peças da JLC.

## 7d. MULTI-UNIDADE: um chip é UMA referência

Um dual op-amp desenhado como **dois símbolos de uma unidade** (`U1A`, `U1B`)
mais um símbolo de alimentação (`U1C`) parece certo no desenho e está errado de
três maneiras ao mesmo tempo:

1. **Três linhas de BOM e três pegadas** para um único chip.
2. Os dois amplificadores usam **os mesmos números de pino** (1/2/3): o
   amplificador B fica no encapsulamento do A.
3. As referências não terminam em dígito (§6b).

O certo é **um símbolo multi-unidade**: referência `U1`, unidades 1, 2, 3.

**A pinagem tira-se da biblioteca oficial do KiCad, não da cabeça.** O símbolo
base dos duais SOIC-8 é o `LM2904` de `Amplifier_Operational.kicad_sym` — de
que `OPA1602`, `OPA1612AxD`, `OPA2134`, `LM4562` e `NE5532` todos herdam por
`extends`:

| | amp A (u1) | amp B (u2) | alimentação (u3) |
|---|---|---|---|
| saída | **1** | **7** | — |
| −IN | 2 | 6 | — |
| +IN | 3 | 5 | — |
| V− / V+ | — | — | 4 / 8 |

⚠️ Cinco duais independentes concordarem é **evidência forte, não é a folha da
peça que se vai comprar**. Fica por confirmar no datasheet antes de submeter.

**E o erro que isto destapou:** no símbolo caseiro o pino **1** estava na
posição do `+IN` e o pino **3** na da saída — **trocados**. Numa placa
fabricada, a rede de entrada iria soldada ao pino de saída do chip. Não aparecia
no protótipo, porque esse é ligado à mão e nunca passou pelo mapeamento de
pinos do encapsulamento.

> **O protótipo que soa bem não valida a pinagem do símbolo.** Ele valida a
> topologia. A numeração dos pinos só é exercida quando a fábrica solda.

**Ao trocar um símbolo por outro, mantém a GEOMETRIA dos pinos** (aqui ±10,16 e
não os ±7,62 da biblioteca oficial): os fios já encostam nesses pontos, e mover
o pino parte ligações que estavam certas. Só os **números** mudam.

**Critério de aceitação da troca:** a netlist tem de ficar idêntica *depois de
aplicar o mapa de renomeação e o mapa de pinos*. Compara-se a **pertença** de
cada rede — o conjunto de `(referência, pino)` — nunca os nomes das redes, que
mudam de propósito.

```
so ANTES : [('C11','1'), ('R10','1'), ('RV1','1'), ('U1','3')]
so DEPOIS: [('C11','1'), ('R0A','1'), ('RV1','1'), ('U1','3')]
```
Foi esta comparação que apanhou o `R0A` da §2d. **Sem ela, teria ido para a
fábrica um resistor com o nome que o KiCad ia renumerar.**

⚠️ `Net-(U1A-+)` numa netlist com um símbolo multi-unidade **não é resto do
nome antigo**: é o KiCad a escrever "U1, unidade A". É sinal de que funciona.

## 9b. O PORTÃO DE COLISÕES DOS ESQUEMÁTICOS

O `tools/svg_colisoes.py` devolvia **código 3 — NÃO VERIFICÁVEL** em todo
esquemático exportado, e a explicação aceite era "o KiCad escreve as letras como
paths". **Era meia verdade e o portão estava cego:** o KiCad escreve TAMBÉM um
`<text>` por cada cadeia, com `opacity="0"` e com **`textLength`** — a largura
exacta que ele próprio calculou. A ferramenta descartava-os por opacidade (regra
pensada para sombreado decorativo) e anunciava "não verificável" a ficheiros com
336 textos medidos lá dentro.

- **Se o ficheiro traz `textLength`, não se estima: mede-se.** A estimativa
  `n × corpo × 0,6` erra por **18 mm** numa nota de 73 caracteres — o suficiente
  para dizer "limpo" sobre um texto que atravessa um potenciómetro inteiro.
- O avanço real da fonte de traços do KiCad, medido no `textLength`, é
  **≈0,80–0,85 do corpo nominal**, não 0,6.
- Em esquemáticos o `svg_colisoes` é **ruidoso** (cada texto sai duplicado, e
  todo rótulo toca no seu próprio fio). Para esses o portão é o
  **`tools/colisoes_sch.py`**, que lê o `.kicad_sch`.

**A regra de comparação do `colisoes_sch`, e a excepção que faltou:**

| texto | contra | vale? |
|---|---|---|
| nota | tudo | sim |
| referência / valor | corpo do **próprio** símbolo | não — é onde moram |
| referência / valor | corpo de **outro** símbolo | **sim, é defeito** |
| referência / valor | fio | não |

A v0.4.1 comparava **só as notas** contra formas, com o argumento de que
"rótulo colado ao seu fio é normal" — verdade para o fio, falso para o corpo de
outro componente. O valor `22p C0G` estava desenhado DENTRO do corpo do `R10`,
o olho viu e a ferramenta disse "0 colisões".

> **Uma excepção larga de mais é uma cegueira com desculpa.** Calibra sempre o
> verificador contra um defeito que já confirmaste com os olhos: se ele não o
> apanha, é ele que está errado, não o olho.

E o corolário, que vale para os dois portões: **código 0 não autoriza saltar o
olho.** Autoriza olhar sabendo onde.


## 7e. O RECTÂNGULO VAZIO NÃO É DEFEITO — é a escolha a aparecer

Numa raiz hierárquica, o rectângulo de cada folha desenha o nome, o ficheiro e
**os pinos de folha**. Se o projecto liga as folhas por **rótulos globais** —
que é a escolha deste — não há pinos de folha nenhuns, e os rectângulos saem
vazios. Aqui: **0 pinos de folha, 125 rótulos globais**.

O custo dessa escolha é este: **a raiz deixa de dizer o que liga a quê.** Quem
abre o projecto vê quatro caixas vazias. É por isso que a legenda da raiz
declara a escolha em texto — sem essa declaração, o desenho parece incompleto
em vez de deliberado.

> Rectângulo vazio **com** a escolha declarada na legenda: correcto.
> Rectângulo vazio **sem** ela: indistinguível de trabalho por acabar.

## 9c. A LEGENDA IMPRIME-SE DE BAIXO PARA CIMA

O KiCad desenha o `(comment N ...)` de **maior número mais ACIMA**. Numerar
1, 2, 3 pela ordem de leitura imprime a legenda **de trás para a frente**:

```
;; ficheiro          ;; render (de cima para baixo)
(comment 1 "A")       C
(comment 2 "B")       B
(comment 3 "C")       A
```

> **A ordem no ficheiro é a INVERSA da ordem de leitura.** O primeiro período
> a ler leva o número mais alto.

Estava assim nas quatro folhas com legenda, e só se vê no render — nenhum
verificador o apanha, porque cada linha está correcta e no sítio certo. É a
mesma família do §5b (o sinal do `y`), com causa diferente: ali era aritmética
minha, aqui é convenção do KiCad. **O sintoma comum — texto que se lê ao
contrário — só o olho no render final o denuncia.**

⚠️ E a legenda é DOCUMENTAÇÃO COM AUTORIDADE: a do soquete afirmava *"Sense dos
DPDT em D11/D12"* meses depois de o sense ter sido abandonado. Quando uma
decisão morre, a legenda que a repete morre com ela — senão reensina-a a quem
abrir o ficheiro, que é exactamente quem não tem como saber que está errada.

---

## 10. A CAIXA MANDA — mas só a caixa que foi CORTADA

O contorno de uma placa e a posição dos furos mecânicos vêm da caixa. A
armadilha é **de qual caixa**.

| artefacto | o que é | vale? |
|---|---|---|
| desenho cotado | a intenção | **não**, se houver gerador ou corte |
| ficheiro de corte de uma geração anterior | um objecto real — **outro** | **não** |
| **gerador da peça que está a ser feita** | a fonte | **sim** |
| a peça em mão | o objecto, com kerf e tolerância reais | manda em tudo |

> **«O ficheiro que foi para a fábrica» não chega — tem de ser o da peça que
> está a ser feita AGORA.** Um artefacto real da geração anterior mede-se tão
> bem como o certo e dá números plausíveis e errados.

No Cotucatronics havia **quatro** fontes para a mesma caixa, com quatro
espessuras (6 nos desenhos, 6,4 na nogueira cortada, 6,0 no gerador, **3,0 no
material encomendado**), e a placa tinha sido dimensionada contra a primeira.

**O que fazer, e não é actualizar as cotas à mão:**

1. **Importar o gerador**, se ele tiver `if __name__ == "__main__"` — é mais
   forte que medir o SVG que ele emite, porque não há parsing entre a cota e o
   uso, logo não há sítio onde as duas divirjam. Só se não houver gerador é que
   se mede o SVG, e aí com guarda de contagem (§2c).
1b. **Melhor que seguir a cota: construir de modo a que ela não entre na conta.**
   Centrar a placa na chapa que a segura fez os furos de fixação caírem sempre
   no mesmo sítio *relativo à placa*, para qualquer espessura — confirmado a
   correr o gerador a `T=3` e `T=6`. Quando um número externo não pára de
   mudar, o conserto não é segui-lo.
2. **Portão antes de escrever**: a placa cabe? algum furo de fixação ficou
   debaixo de um componente? Um furo tapado não é um furo apertado — é um
   parafuso que não entra com a caixa fechada e a solda feita.
3. **Nenhuma cota escrita à mão sobrevive** ao lado da leitura. Ver a regra
   abaixo, que é a razão pela qual a divergência durou.

### 10a. Constante que ninguém lê não pode estar errada em voz alta

`INTERIOR = (188.0, 108.0)` esteve declarada e **nunca foi consultada por
nenhuma linha**. Para um número dar erro, alguém tem de o ler.

> Uma cota ao lado de coisa medida envelhece como uma lista ao lado de coisa
> gerada (§ da lista embutida). **A cura não é actualizar — é derivar e
> recusar.**

Corolário para revisão: ao ler um gerador, `grep` cada constante declarada
pelo seu próprio nome. **Zero usos é achado**, mesmo que o valor esteja certo
hoje.

### 10b. Comentário com premissa revogada re-ensina o erro

Quando a caixa cresceu, «a caixa só tem 28 mm livres à frente» — o motivo
escrito para duas decisões de colocação (são 43) — passou a ser falso, e as decisões
continuaram certas por *outro* motivo. Comentário com autoridade que afirma
premissa revogada tem a gravidade do código: o próximo leitor decide com ele.

### 10c. Divergência anunciada sem medir acusa a peça errada metade das vezes

A caixa tinha três arcade e o desenho dois; ia anunciar que o **esquema**
estava desactualizado. O esquema já tinha três. Quem estava errado era o
desenho — outra vez. **Medir antes de anunciar de que lado está a falta.**

## Procedência

Esta skill absorveu em 01/09 a antiga `esquematico`, que deixou de existir por
decisão do dB: **uma skill por assunto; duas a dizerem quase o mesmo derivam, e
aí ninguém sabe qual manda.**

Dela vieram §4b (a família dos cruzamentos, com C1/C2/C3 nomeados), a semântica
do ponto de junção que abre a §4, §5 (grelha antes de emitir e gerar por
script), §7b (o que todo esquema declara) e §7c (o contrato com o BOM).

O resto nasceu de uma sessão em que se entregaram **duas folhas que não abriam**
com o verificador de rede limpo, e em que uma edição foi re-inspeccionada
visualmente **sem se repetir o ERC** — precisamente o defeito que a `esquematico`
não podia prever, porque foi descoberto depois dela.

Em 05/09 entraram §2b, §2c, §2d, §5b, §6b, §6c, §7d, §7e, §7f, §9b e §9c, de uma sessão a preparar
os ficheiros para a JLCPCB. O fio que as liga é um só: **ferramentas que perdiam
linhas em silêncio e por isso aprovavam tudo** — quatro geradores a saltar dois
pinos escritos em notação científica, um verificador de contrato a dizer "as 26
batem" tendo lido 24, e o verificador de colisões a dizer "0" cego a 40
propriedades. O que as apanhou não foi ler o código: foi **comparar o que a
ferramenta leu com o que existia**, e comparar a netlist com a de antes.
