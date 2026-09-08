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
exposto **não traz o PNS** (nem `Shove`, nem `Walkaround`, nem autoroute) —
confirmado a listar o módulo, não deduzido. **E não há export de DSN**, portanto
nenhum autoroteador externo é alcançável por aqui. Quem rotear, roteia na
interface ou escreve o seu.

⚠️ **O `pcbnew` importado do Python do sistema falha no `dlopen`** — pede as
bibliotecas wx do próprio KiCad. Ou se usa o interpretador do bundle, ou se
mede pelo relatório do DRC, que é a ferramenta externa que pode dizer não.

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
