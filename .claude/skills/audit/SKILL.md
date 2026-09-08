---
name: audit
description: Auditoria profunda de consistência no Cotucatronics — caça nomes fantasma, lápides invertidas, declarações falsas, números mortos, tabelas à mão a envelhecer por cima de geradas, links mortos, isenções órfãs, verificações sem objeto, frases partidas por renomeação e destinos dessincronizados. Use ao apagar um nome de toda a árvore, ao remover uma feature, ao fechar uma reestruturação, e antes de dar documentação por limpa.
---

# /audit — auditar sem partir nada

**Uma auditoria que quebra o build perdeu o direito de ter razão.** E uma
que "limpa" um nome deixando a frase em volta a mentir é pior do que não
ter corrido: tirar o qualificador de uma afirmação morta faz o morto passar
por vivo.

Esta skill é o processo. A parte mecânica está em `docs/operacao/auditar.sh`
(v1.2.0), que chama `coerencia.py`, `versoes_readme.py` e `vizinhos.py` —
corre-o primeiro, sempre.

## A ORDEM É INEGOCIÁVEL

Ela existe porque a inversão de dois passos quebra o build de verdade.

```
1. BASELINE   correr todo verificador ANTES de tocar. Guardar exit code
              E os contadores (fantasmas, órfãos, aposentados, ERC…).
              Sem baseline não se sabe o que é herdado e o que é teu.
2. INVENTÁRIO por `git ls-files`, NUNCA por glob à mão (ver abaixo).
3. CLASSIFICAR cada ocorrência, antes de editar uma única.
4. EDITAR     com âncora única (skill `edicao-segura`), nunca replace cego.
5. GREP       o relato vem do ficheiro, não da intenção.
6. RE-VERIFICAR e comparar contra o baseline, número a número.
7. SINCRONIZAR os três destinos.
8. PENDENTES  varrer o que a árvore declara por fazer, e para cada um
              perguntar se ainda é verdade. Pendência resolvida que ficou
              escrita é uma declaração falsa com data.
```

### O passo 2 enumera-se pelo git, nunca à mão

Varri o fonte com `*.h *.cpp Makefile` — 33 ficheiros — e dei o trabalho por
fechado. A árvore tem **137 rastreados em 14 extensões**. Ficaram de fora
`.py` (22), `.command` (8), `.sh` (3), `.lds`, `.ino`, `.html`, `.txt` e um
**segundo `Makefile`**. O `check_repo.sh` escapou e só apareceu na varredura
final, por acaso.

> **Enumerar é do git; escolher à mão é como se esquece um tipo de ficheiro
> inteiro.** `git ls-files -z | xargs -0 grep -lI` — e ainda
> `--others --exclude-standard` para o que não está rastreado, mais o vault,
> que vive fora do git.

O `auditar.sh` imprime as extensões que existem na árvore ao lado do
resultado: extensão nova aparece à vista, em vez de ficar de fora em silêncio.

### O passo 2 e o passo 4 têm dependência de ordem

Há ficheiros que são **licença** para outros: `docs/params-aposentados.txt`
e as directivas `<!-- doc-check: ignore … -->` autorizam a doc a citar um
nome que já não existe no fonte.

> **Apagar a licença antes da menção quebra o `check_docs`.** Menções
> primeiro, licença por último, verificador a fechar.

## OS TRÊS DESTINOS

Repo (`docs/`) · vault (`~/Documents/Obsidian Vault/Cotucatronics/`) ·
skills (`.claude/skills/*/SKILL.md`). A skill é a que **carrega sozinha na
hora do uso** — deixá-la suja é deixar suja a cópia que fala primeiro. E a
`description:` do frontmatter conta como corpo: ela aparece no listador.

⚠️ O vault tem uma cópia do `README.md` da **raiz**, não a de
`docs/operacao/`. Comparar pelo basename produz falso positivo.

## AS CLASSES — e qual delas o grep apanha

O `auditar.sh` cobre as cinco primeiras. As outras exigem leitura, e é por
isso que ele **não finge** cobri-las: verificador que passa sempre é a
classe 10 dele próprio.

| # | Classe | Mecânico? |
|---|---|---|
| 1 | link morto (ficheiro referido que não existe) | ✅ |
| 2 | isenção órfã (`doc-check: ignore` de nome ausente do corpo) | ✅ |
| 3 | aposentado órfão / **entrada duplicada** na lista | ✅ |
| 4 | destino dessincronizado (repo ≠ vault ≠ skill) | ✅ |
| 5 | fantasma / CC errado / faixa que não bate | ✅ `check_docs` |
| 6 | **lápide invertida** | ❌ leitura |
| 7 | **tabela à mão a envelhecer por cima de uma gerada** | ❌ leitura |
| 8 | **contradição intra-ficheiro** | ❌ leitura |
| 9 | **número morto que passa por actual** | ❌ leitura |
| 10 | **verificação sem objeto** | ❌ leitura |
| 11 | **defeito órfão do seu sujeito** | ❌ leitura |
| 12 | **item pendente** (🔴🟡⬜⏳ TODO, "por confirmar", "em aberto") | ✅ `coerencia.py` |
| 13 | **frase partida pela renomeação** | ✅ `vizinhos.py` |

### 6. Lápide invertida

Comentário que afirma que algo **continua** — e não continua.

Caso real: o `manifest.h` dizia, de um grupo de params de um efeito
aposentado, *"CONTINUAM na tabela de propósito"*. Não continuavam: todas as
ocorrências restantes estavam dentro de comentários, e não havia um único
`t.Add()` para nenhum deles. Eu acreditei no comentário e reportei ao dB que
era preciso mudar código. Era falso.

> **Comentário que afirma o estado do código é a afirmação menos fiável do
> ficheiro** — ele não compila, ninguém o testa, e sobrevive à mudança que
> o desmente. Prova pelo símbolo (`grep` da declaração), nunca pela prosa.

### 7. Tabela à mão por cima de uma gerada

`src/BOARDS.md` tem uma tabela intitulada *"Mapa de CC do instrumento
**atual**"*, escrita à mão, que lista três params inexistentes e um
removido em 28/08. O mapa gerado de verdade é `docs/operacao/CCMAP.md`,
produzido por `gerar_ccmap.py` a partir do `manifest.h`, e traz aviso de
*não editar à mão*.

> **Antes de corrigir uma tabela, descobre se ela é gerada.** Corrigir a
> gerada à mão é trabalho que a próxima geração desfaz; corrigir a de mão
> quando existe uma gerada é manter viva a que mente.

⚠️ **E ao apontar a de mão para a fonte, confirma que a fonte não aponta de
volta.** O `ROTEIRO-KICAD` §5 listava as 26 vias do IDC e passou a dizer «vê o
gerador» — mas o cabeçalho do gerador dizia «a pinagem sai do §5». Ficavam os
dois a apontar um para o outro, e nenhum era a fonte.

> **Documento explica; gerador afirma.** No §5 ficou só o que **não** se deduz
> da tabela — porque é que as terras estão onde estão, e os 6 dB medidos com o
> painel aceso. Isso não vive no gerador e morreria com a tabela.

Como descobrir: `grep -rl '<nome do doc>' --include='*.py' --include='*.sh'`.

### 8. Contradição intra-ficheiro

O `README.md` anunciava o `flash.sh` a entrar no bootloader por `dfu go`
e, trezentas linhas abaixo, dizia *"gravar exige o botão BOOT"*. As duas no
mesmo ficheiro, ambas afirmativas, nenhuma marcada como histórica.

> Procurar pelo **par**, não pela frase: estado declarado duas vezes é
> candidato a divergir. Versões, contagens, "nenhum defeito conhecido",
> "ainda não implementado", números de CPU.

### 9. Número morto que passa por actual — **a armadilha desta skill**

O `README` dizia *"Com o delay do motor externo carregado: 5,7% média"*. A
carga real hoje é 59,5%. Enquanto o qualificador lá estava, a frase ao menos
avisava que era de outra configuração.

> **Tirar o qualificador de um número morto é falsificá-lo.** Ao apagar um
> nome de uma frase que carrega medição: ou actualizas o número contra a
> fonte que mede, ou apagas a frase inteira. Nunca só o nome.

### 10. Verificação sem objeto

O `check_repo.sh` teve um bloco inteiro a conferir exports que já não
existiam: passava sempre, e ia continuar a passar para sempre.

Forma subtil, encontrada nesta árvore: uma isenção `doc-check: ignore
route_exciter` cujo comentário diz *"a menção é o registo da renomeação"* —
e a menção que ela protegia já não está no corpo. **A isenção passou a ser
o único registo do que ela existia para proteger.** Apagá-la por "órfã"
apagaria a última cópia. Ler antes de podar.

### 11. Defeito órfão do seu sujeito

🔴 *"A serial morre com o efeito X ligado"* — o efeito saiu há semanas. O
defeito é real (starvation da ISR do USB por callback longo), o **repro** é
que morreu com o sujeito.

> Ao aposentar uma feature, varrer os defeitos abertos: os que a nomeiam ou
> fecham com ela, ou perdem o modo de reproduzir. Renomear pelo mecanismo.

### Como podar uma isenção sem perder o registo

Uma `doc-check: ignore` só trabalha se o nome aparecer **entre crases** no
corpo — e o escopo é o **ficheiro inteiro**, apesar de a docstring do
`check_docs` dizer "na linha ou na anterior". Antes de podar, três perguntas:

1. O nome aparece em crase no corpo deste ficheiro? Se sim, **não é órfã**.
2. O comentário da isenção carrega informação que não existe em mais lado
   nenhum? Se sim, **migrar para `params-aposentados.txt`** — é a casa
   declarada de nome aposentado — e só então apagar a directiva.
3. Outro ficheiro carrega o mesmo registo E usa o nome? Então esta é sobra.

Foi assim que seis órfãs viraram zero sem perder uma linha de história: duas
migraram, quatro eram cópias de um registo que vivia noutro documento.

### 12. A passada de pendentes

`coerencia.py` varre as marcas que a árvore usa para dizer "por fazer":
os emojis de estado, `TODO`/`FIXME`/`XXX`/`HACK`, e as frases portuguesas
("por confirmar", "por medir", "em aberto", "ficou por", "não foi validado").

**Duas armadilhas de precisão, ambas pagas aqui:**

- `\bTODO\b` com *ignorecase* casa o português **"todo"**. A primeira versão
  desta passada trouxe ~200 falsos positivos. TODO/FIXME são **maiúsculas-só**;
  só as frases é que são insensíveis.
- `pend[êe]ncia` sem `\b` casa **de**pendência e **inde**pendência.

> Um verificador que berra é um verificador que se desliga. Precisão antes de
> cobertura: a passada só vale se cada linha que ela imprime merecer leitura.

O que fazer com o resultado: para cada pendência, **ainda é verdade?** As que
já se resolveram e ficaram escritas são a classe 9 disfarçada — uma declaração
falsa com data em cima.

### 13. A frase partida pela renomeação

Renomeei uma frase no `hal_daisy_seed.cpp` com âncora única e verificada. A
âncora casou uma vez. O `grep` confirmou o texto novo. O `make` passou. E a
frase ficou assim:

```
    // ... "a serial morre sob callback
    // ligado": quem morria era o LOOP
```

porque a **continuação estava na linha seguinte**, que ninguém tocou.

> **A âncora única prova que o casamento foi ÚNICO. Não prova que a frase
> continua a LER-SE.** São garantias diferentes, e só a primeira é mecânica.
> Compilador, `grep` e `assert` calam-se todos os três.

`vizinhos.py` lê o diff e assinala as vizinhanças em risco. O discriminador
que o torna utilizável: **só há risco se a edição tocou o FIM da linha.**
Renomeação no meio deixa a cauda igual e a frase reata como reatava. Sem esse
filtro a ferramenta assinalava 211 vizinhanças — ruído a fingir de achado, que
é a classe 10 aplicada a ela própria. Com ele, 89; e com `--termo=<nome>`, que
aperta para os casos em que a **cauda removida continha o nome**, 16.

```bash
python3 docs/operacao/vizinhos.py --termo=<nome-que-sai>
```

Ela não decide — manda ler. Numa remoção de nome, ler essas dez ou vinte
linhas é o preço de não deixar uma frase sem sentido no meio da árvore.

### 14. Corpo de emissor copiado em vez de chamado

O `gera_soquete.py` tinha uma função `nao_liga(x, y)` que emitia um bloco
`(no_connect ...)`. E tinha, quinhentas linhas abaixo, **o corpo dela outra
vez**, escrito à mão:

```python
L.append('\t(no_connect\n\t\t(at %s %s)\n\t\t(uuid "%s")\n\t)\n'
         % (g(_x), g(_y), u()))
```

Enquanto o emissor não mudou, as duas versões produziram output idêntico e
nada as distinguia. Quando o `uuid` passou a exigir chave, **só o original a
recebeu** — a cópia ficou com a assinatura antiga e o gerador rebentou. Se a
mudança tivesse sido silenciosa em vez de partir a assinatura, a cópia teria
continuado a emitir a versão velha, sem erro nenhum.

> **É a classe 7 em forma de código.** Duas fontes do mesmo comportamento, e a
> que não é chamada é a que envelhece. A tabela à mão pelo menos parece uma
> tabela; um corpo de emissor duplicado parece código normal, e é por isso que
> só se encontra procurando por ele.

**Como procurar** — não por nomes de função, que a cópia justamente não usa,
mas pelo **output**: toda a família de emissores escreve strings com forma
reconhecível.

⚠️ **E ancora na ACUMULAÇÃO, não na forma.** Buscar só a forma devolveu oito
acertos por ficheiro, e metade eram **leitores** — `Lb.find('(symbol "%s"')`,
`_confere_lib`, `lib_embutida` — que procuram a forma em vez de a produzir.
Isso é a **classe 10** a acontecer dentro desta passada: ruído a fingir de
achado. O que emite é o que faz `append`:

```bash
grep -n "L\.append(" gera_*.py | grep -o "L\.append(f\?'\{1,3\}.\{0,26\}" | sort | uniq -c
```

Cada forma deve aparecer **uma vez**. Duas ou mais, e só uma dentro de um
`def`: as outras são cópias. Corrido sobre os três geradores deste projecto dá
uma linha por forma — a única cópia que existia foi a que originou esta classe.

A mesma busca serve para qualquer gerador, trocando o acumulador: SQL montado à
mão ao lado de um construtor, cabeçalhos HTTP, mensagens de log com formato
fixo.

⚠️ **E o discriminador que evita o ruído:** duplicar a *chamada* é normal e
esperado — um `nao_liga()` invocado em vinte sítios é o desenho a funcionar.
O achado é duplicar o **corpo**. A pergunta é «este sítio reconstrói o que um
`def` já constrói?», não «esta forma aparece muitas vezes?».

⚠️ **Achado mesmo quando o output é idêntico hoje.** A cópia não está errada:
está **por envelhecer**. Reportar só quando já divergiu é reportar o incêndio,
não a instalação eléctrica — e a divergência, quando chega, não dá erro.

## O TESTE QUE LÊ A RECUSA COMO APROVAÇÃO

Num só dia, quatro verificações minhas deram o veredicto **ao contrário** — e
nenhuma por o código auditado estar errado:

| o teste | porque aprovou em falso |
|---|---|
| `grep -q "gravado"` | a recusa diz **`"Nada gravado"`**, que contém `"gravado"` |
| `case "$out" in *gravado*)` | o mesmo, outra sintaxe |
| `set -- $cfg` num `for` | o **zsh não separa palavras** sem aspas — dois valores entraram como um |
| busca por prosa num documento | `"preenchimento"` onde o ficheiro diz `"enchimento"` |

> **A mensagem de falha quase sempre contém a palavra de sucesso**, porque é
> escrita para gente ler: *«nada gravado»*, *«no changes committed»*, *«0 tests
> failed»*, *«failed to build»*. Um teste que casa a **subcadeia** lê a recusa
> como aprovação.

⚠️ **É o pior modo de falhar que há**, e vale a pena dizer porquê: a ferramenta
estava certa, a mensagem estava certa, e só quem a consultou é que percebeu o
contrário. Não há nada partido para encontrar depois — só uma conclusão errada
já espalhada por quem a leu.

**O que se ancora, por ordem de preferência:**

1. **o código de saída** — e o do comando certo: `cmd | tail` devolve o do
   `tail`, não o do `cmd`;
2. **o início da linha** (`^ *gravado ·`), não a palavra solta;
3. **a estrutura** (a secção existe? o bloco existe?), nunca a prosa — que muda
   quando alguém reescreve uma frase e não mexe em nada.

⚠️ **E a mesma armadilha do outro lado:** procurar por prosa para provar que uma
lição foi absorvida deu **dois falsos negativos** numa conferência de dez, por
uma palavra e um travessão. **Um verificador de documentação ancora em títulos,
não em frases.**

## FERRAMENTA NÃO CORRIDA NÃO EXISTE

A classe 14 foi escrita, revista e dada por boa. Depois **corri-a**, e ela
devolvia oito acertos por ficheiro, metade deles **leitores** da forma em vez
de emissores dela — a classe 10 a acontecer dentro da passada que eu tinha
acabado de escrever para apanhar defeitos. O discriminador que a torna
utilizável (ancorar na acumulação, não na forma) só apareceu na primeira
corrida real.

> **Ferramenta consertada e não corrida não está consertada; ferramenta nova e
> não corrida não existe.** Uma passada por estrear não é uma passada fraca —
> é uma promessa, e as promessas não apanham defeitos.

E corre-se **sobre a árvore inteira**, não sobre o caso que a motivou: foi ao
correr sobre todos os `gera_*.py` — e não sobre o ficheiro onde o defeito
nasceu — que a classe 14 encontrou o terceiro gerador.

## A REGRA DO PORQUÊ — quando o nome sai e quando o registo sai

Ao apagar um nome de toda a árvore, cada ocorrência é uma de duas coisas:

- **O nome é a RAZÃO de uma decisão que ainda vale.** O bloco é 48 porque
  um módulo de FFT exigia 256. → **a razão fica, só o nome sai.**
- **O registo É sobre ele.** A linha no livro de certificações, a lápide no
  arquivo de aposentadas. → **sai inteiro, e isso é perda de registo.**

> **Declarar as perdas do segundo tipo, nominalmente, no relatório.** O
> utilizador autorizou apagar um nome; não autorizou apagar em silêncio a
> prova de que um ensaio foi corrido.

O nome vai para a mensagem do commit — git é onde a história pertence.

## IDENTIFICADOR DE API NÃO SE RENOMEIA

Uma macro `XPTO_NOTHROW` de um SDK não vira `MOTOR_NOTHROW`. Receita com
macro inventada não é uma receita genérica: é uma receita **falsa**, e falha
em silêncio no dia em que alguém a seguir.

> Quando o nome a apagar é símbolo e não prosa, a escolha é **apagar a
> receita** ou **manter o símbolo**. Generalizar não está na mesa.

Mesma regra para caminhos reais em git: `git show <hash>:Max/x.maxpat` é
uma instrução de recuperação. Se o nome tem de sair, **provar o caminho
novo antes de escrever** — e o número que ele devolve entra na doc.

## FECHO

```bash
./docs/operacao/auditar.sh <termo>          # ou: auditar.command
python3 docs/operacao/vizinhos.py --termo=<termo>   # ler cada uma
python3 src/check_docs.py; echo $?          # tem de bater com o baseline
git ls-files -z | xargs -0 grep -lI -i '<termo>'    # tem de dar VAZIO
```

O último não é `grep -rni` numa lista de pastas escrita à mão — é a árvore
inteira pelo git. Foi a lista à mão que deixou o `check_repo.sh` de fora.

> **Quando a afirmação é sobre um CONJUNTO, a verificação conta o conjunto —
> nunca a amostra.** Converti dois geradores, relatei «os dois geradores», e
> havia um terceiro a minar `uuid4` na folha ao lado: contei os que tinha
> olhado, não os que existem. `grep -c uuid4 gera_*.py` custa um segundo e não
> depende de memória nenhuma.
>
> A forma da armadilha é sempre a mesma — uma lista escrita à mão a fazer de
> universo. A lista de pastas, os dois geradores, os ficheiros que me lembro de
> ter aberto. **O universo pergunta-se ao `git`, ao `glob`, ao gerador; não à
> memória de quem audita.**

Relatar: ficheiros tocados, contadores **antes → depois**, achados que não
foram consertados (e porquê), e as perdas de registo, uma a uma.

Ver também: [[edicao-segura]] (âncora e verificação),
[[medicao-limpa]] (nunca concluir sem medir),
[[instrumentacao]] (instrumentação falha como código de produção).
