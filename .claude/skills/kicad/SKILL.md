---
name: kicad
description: Gerar, verificar e entregar esquemáticos KiCad por script (kiutils/Python) — provar que o ficheiro ABRE antes de tudo, correr o ERC a cada edição, encaixar na grade de ligação, e declarar isenções com prazo. Use ao escrever ou alterar geradores de esquemático, ao entregar folhas, e antes de dar qualquer folha por verificada.
---

# KiCad por script — as regras que custaram folhas

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

## 4. Pino no MEIO de um fio NÃO liga

Só liga em **ponta de fio** ou onde há **junção explícita**. Um `PWR_FLAG`
colocado a meio de um segmento fica solto, e o ERC di-lo como
`pin_not_connected`.

## 5. `power_pin_not_driven`: declarar, não isentar

Símbolos caseiros não declaram `power_out`, e todo pino `power_in` fica órfão.
A resposta é um **PWR_FLAG por rede de energia** — símbolo com pino `power_out`,
referência começada por `#` (convenção: não entra na BOM).

⚠️ **UM por rede, e só um.** Dois `power_out` na mesma rede são conflito: troca-se
um erro por outro.

⚠️ **A referência `#...` NÃO se mostra.** Além da convenção, é colisão certa: a
etiqueta cai por cima do rótulo global que está ao lado.

⚠️ **E a bandeira não vai na coordenada do rótulo** — tapa-o. Vai num ponto
livre do mesmo fio, **com junção** (ver §4).

## 6. Isenções: com prazo, com razão, e nunca em bloco

Isenção generosa transforma verificador em carimbo. Cada uma leva **porquê** e
**até quando**.

⚠️ **Isenção que se INVERTE é a mais valiosa de marcar.** `isolated_pin_label`
numa folha isolada é falso positivo legítimo; **dentro do projecto único passa a
ser rede sem destino** — defeito real. A mesma linha do relatório muda de
significado, e quem não marcou a data não repara.

⚠️ **E declarar onde a isenção NÃO está aplicada.** O KiCad guarda-as no
`.kicad_pro`; sem projecto, o registo é só um documento. **Registo sem aplicação
é promessa, e diz-se que é.**

## 7. O gerador é a FONTE

Nunca editar o `.kicad_sch` à mão: as duas versões divergem e ninguém sabe qual
manda. Todo conserto entra no `.py` e regera-se.

**E a fonte tem de correr fora da máquina do autor.** Em 01/09 os geradores
tinham `/mnt/user-data/outputs/` e `/home/claude/*.svg` gravados — rebentavam
com `FileNotFoundError` na primeira tentativa noutro computador. Caminhos
relativos ao próprio ficheiro (`pathlib.Path(__file__).parent`), sempre.

## 8. O laço de saída, e ele inclui o ERC

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
