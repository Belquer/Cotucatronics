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

---

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
