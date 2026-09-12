---
name: edicao-segura
description: Disciplina de edição de fonte no Cotucatronics — âncora conferida antes, grep depois, versão semântica bumpada e reportada. Use em TODA alteração de arquivo do projeto. Edições silenciosamente falhas já custaram uma célula inteira de instrumentação e duas entradas de documento que eu relatei como escritas e nunca existiram.
---

# Edição segura de fonte

**Uma edição que falha em silêncio é pior que uma que quebra o build** — ela
te deixa relatar como feito algo que não existe.

Já custou: duas entradas do `battle_scars.md` reportadas como escritas e nunca
inseridas (âncora errada, sem verificação); e uma célula inteira de
instrumentação cuja ausência apareceu como `VOLTAS=0, nan` — o código nunca
tinha entrado no arquivo.

## Protocolo

### 1. Antes — a âncora é única?

```python
assert s.count(ancora) == 1, "ancora <nome>"
```

Conferir **antes de escrever**. Se o assert estourar, nada foi gravado e o
arquivo continua íntegro — é o comportamento desejado, não uma falha do
processo.

Âncora com 0 ocorrências = o texto que você imaginou não existe.
Âncora com 2+ = você não sabe qual vai mudar.

### 2. Depois — confirmar no arquivo

```bash
grep -n "<trecho novo>" <arquivo>
```

**Nunca reportar uma edição sem ter feito o grep.** O relato vem do arquivo,
não da intenção.

### 3. Compilar antes de relatar

Alteração em `.cpp`/`.h` só é "feita" depois de `make` limpo. Erros comuns
que só o compilador pega: função de escopo de arquivo usando `hal::` sem o
`using namespace cotuca;`, e helper definido depois de quem o chama.

### 4. Versão semântica — sempre

Todo arquivo tocado bumpa e **expõe a versão em runtime**:

- MAJOR: mudança incompatível (protocolo, assinatura, remoção)
- MINOR: adição compatível (comando novo, tunable novo)
- PATCH: correção que não muda comportamento

`main.cpp` carrega `COTUCA_OS_VERSION` **e** o comentário do cabeçalho —
**os dois** precisam mudar juntos.

### 4b. Conferir a VERSÃO — no arquivo e na placa

**Rotina obrigatória**, e ela existe porque falhou três vezes seguidas no
mesmo dia: eu anunciei consertos como gravados enquanto o `main.cpp` ficava
travado numa versão antiga, porque as edições morriam em `assert` de âncora e
eu só olhava o resultado da *gravação*.

```bash
head -3 main.cpp | grep esqueleto     # o que o fonte diz
# e depois, na placa:  `board` imprime  Cotucatronics OS vX.Y.Z
```

**Os dois têm que bater.** Se o fonte não subiu de versão, a edição não
entrou — por mais que a gravação tenha dito `GRAVOU`.

> **Saída de ferramenta não é confirmação de intenção.** A laçada confirma que
> gravou. O `assert` confirma que a âncora casou. O `grep` confirma que o texto
> entrou. São três confirmações diferentes — não trate a primeira como se
> cobrisse as três.

**Corolário:** quanto melhor a proteção contra edição silenciosa, mais fácil é
não perceber que ela agiu — o efeito de uma edição barrada é o arquivo
continuar igual, e arquivo igual não faz barulho.

### 5. Relatar

Dizer **quais arquivos mudaram e em que versão ficaram**. Sempre.

## Regra da deleção por faixa

**Nunca apagar uma faixa cujos limites você calculou sem listar o que há
dentro.** Duas vezes em 20/08 isso custou código e prosa (cicatriz 12e).

O compilador testemunha **dependências de símbolo, e nada mais**. Então:

> **O perigo de uma deleção por faixa é proporcional a quanto do conteúdo
> apagado não é referenciado por nome em outro lugar.**

As duas classes que somem em silêncio:
- **código só-escrita** — instrumentação, contadores, log, efeito colateral.
  Produz *zero com cara de dado*.
- **prosa** — comentários que explicam um porquê. Some ainda mais calado,
  porque ninguém nunca nota a falta de um parágrafo.

**Antídoto, duas etapas:**

```python
m = re.search(padrao, s, re.S)
print(repr(s[m.start():m.end()]))          # imprime o que casou
print(s[m.start():m.end()].count("\n"))    # confere a contagem
# só então apagar
```

`.*?` com `re.S` pode atravessar o arquivo inteiro dependendo do lookahead.
**A falha é rodar operação destrutiva com matcher de extensão imprevisível** —
não o casamento inesperado.

**Recuperação:** `git show HEAD:arquivo` e recolagem cirúrgica.
**Nunca `git checkout`** quando há trabalho não commitado no mesmo arquivo.

## Regra do primeiro caso de uma forma nova

Padrão copiado carrega uma **pré-condição que ninguém escreveu**.

Caso (20/08): `Add(nome, def_norm, min_real, max_real)` — o primeiro argumento
é NORMALIZADO e está no nome. Todo param do projeto tinha `min_real = 0`, e com
mínimo zero normalizado e real coincidem: a distinção nunca precisou existir.
O primeiro param de faixa **simétrica** `[-1,+1]` nasceu na ponta em vez do
centro, com um transdutor mudo e sem sintoma.

> **Ao ser o primeiro caso de uma forma nova** — faixa negativa, unidade
> diferente, sinal invertido, tipo novo — **reler a assinatura em vez de
> imitar o vizinho.** Os vizinhos estavam certos por um motivo que pode não
> valer para o caso novo.

## Regra de premissa

Se a tarefa começa com uma premissa sobre o estado do sistema ("com a v0.39.0
gravada..."), **verificar a premissa antes de agir sobre ela**. Uma sessão
inteira já foi planejada em cima de um firmware que não estava na placa.

## Guarda de faixa em float se escreve pela NEGAÇÃO

**NaN falha toda comparação.** Isso torna a forma direta insegura:

```cpp
if(r >= 0.f && r < (float)cap_)  { usa(r); }        // NaN PASSA como "fora"
if(!(r >= 0.f && r < (float)cap_)) { r = 0.f; }     // NaN cai na rede
```

Em 22/08, `delay.h:117` tinha `while(r < 0.f) r += cap_;` e escondia **dois**
defeitos: com `atraso_` em +Inf o laço nunca terminava; com NaN ele saía na
hora e `(size_t)NaN` — comportamento **indefinido** — indexava fora do
buffer em silêncio. O segundo era o pior e estava invisível.

> **Toda guarda de faixa em float que protege um índice se escreve pela
> negação.** Qualquer `if(x >= lo && x < hi)` guardando acesso derivado de
> float é candidato a ter a forma invertida.

Quando for auditar, o padrão a procurar é: **float → índice**. Conversão de
float para inteiro com valor fora de faixa (ou NaN/Inf) é UB, não wrap.

**E leia o contexto antes de tocar.** O segundo defeito só apareceu porque a
linha foi lida inteira antes de ser trocada. Conserto parcial que parece
completo é pior que nenhum: fecha o sintoma que denunciava o resto.

## O tamanho da seção prova que o atributo foi aplicado

Quando `__attribute__((section(...)))` **é o mecanismo** — código que precisa
rodar de outra memória, dado que precisa morar noutra região — o tamanho da
seção no output do build é a verificação, e ela é grátis.

Em 22/08 marquei o atributo em dois ajudantes e esqueci na função principal.
Ela rodou de onde não devia e travou a placa: dois RESETs físicos e uma
reinstalação de bootloader. O número estava impresso antes de tudo isso:

    ITCMRAM:  68 B    <- pouco demais pro que eu tinha escrito
    ITCMRAM: 212 B    <- depois do conserto

> **Seção do tamanho errado é atributo não-aplicado até prova em contrário.**
> Ler "a seção existe" e parar ali é meio caminho; o que importa é "a seção
> tem o tamanho certo".

Mesma família do `.map` que pegou o `.bss` indo para a DTCM numa troca de
`APP_TYPE`: **o linker fala; o custo é ler.**

Escreva a expectativa no cabeçalho do arquivo, junto do `#define` do
atributo — é lá que alguém vai acrescentar a próxima função e esquecer.

## Sentinela negativa exige init explícito E teste explícito

`-1` como "nenhum" convive no mesmo tipo com `0`, que significa "o primeiro".
As duas metades falham em silêncio, por motivos opostos:

```cpp
BotaoAlvo t[N] = {};        // zero-init: param = 0, e ZERO E INDICE VALIDO
if(SacrifTravado(i)) ...    // -1 e VERDADEIRO; 0 (indice valido) e falso
```

Em 22/08 as duas apareceram no mesmo dia, no mesmo projeto. Na primeira, a
marca de "travado" aparecia em todos os efeitos sãos e sumia no índice 0. Na
segunda, o botão randomizador passou a apontar para o `level` e para o LED do
freeze.

> **`= {}` não respeita a sentinela; `if(x)` não a testa.** Inicializador no
> próprio `struct` (`int param = -1;`) e comparação explícita (`>= 0`).

## GPIO se remapeia no fonte; periférico, não

Antes de propor "troco no firmware em vez de refazer a solda", perguntar se o
pino é GPIO puro ou está amarrado a periférico.

Em 22/08 o LED e o switch do freeze vieram invertidos e custaram dois números
no fonte — GPIO não tem preferência de pino. Minutos depois, o MIDI de volta
exigiu fio novo: `D13`/`D14` são `PB6`/`PB7`, as pernas da USART1, e a função
está amarrada pelo silício.

> A diferença não aparece no esquema — aparece na tabela de funções
> alternadas do chip. UART, SAI, ADC e QSPI não se remapeiam livremente.

## Reestruturação tem três frentes; o compilador cobra uma

Código, documentação e símbolos. O compilador cobra os símbolos que
**quebram** — e cala sobre os que sobrevivem sem uso e sobre a doc que
continua descrevendo o desenho morto.

Em 22/08 um audit achou doze casos de deriva de uma única reestruturação:
params fantasmas citados na doc por meses, contadores lidos-sem-escritor e
escritos-sem-leitor, e três órfãos que o compilador já acusava em warning
sem que ninguém lesse.

> **Ao terminar uma reestruturação, varrer as três frentes.** E se a lição
> se repetir, automatizar: `make check-docs` neste projeto verifica a doc
> contra o manifest a cada build. Hábito automatizado vence hábito lembrado.

**Ao escrever o verificador, a ordem é inegociável: limpar primeiro.** Contra
base suja o primeiro build falha em massa e a tentação vira baixar a régua.

**E cuidado com o verificador que acusa a frase que documenta o conserto** —
casar padrão em texto pega a cicatriz que narra a remoção. Três regras
resolveram: nome que existe no fonte é legítimo; família conta como nome;
aposentados vão para uma lista declarada, que vira documentação útil por si.

## Âncora de edição prova EXISTÊNCIA, não unicidade

O padrão que parece cuidadoso e não é:

```python
assert velho in s          # <-- só prova que existe
s = s.replace(velho, novo, 1)   # <-- e troca o PRIMEIRO
```

Caso real (28/08). Ancorei a inserção de um subcomando em
`if(strncmp(a, "reset", 5) == 0)`. Esse texto existe **três vezes** no
`main.cpp` — nos comandos `varre`, `encpino` e `enc`. O bloco foi parar ao
handler do `vistos`, quinhentas linhas acima do destino.

**E então compilou, gravou e não deu erro em lado nenhum.** O comando novo
respondia com o relatório genérico, porque o `enc` não reconhecia o
subcomando e caía no caso final. Duas gravações com uma funcionalidade que
simplesmente não existia — e um `vistos inverte g` acessível que ninguém ia
digitar.

O guarda custa uma linha:

```python
assert s.count(velho) == 1, "AMBIGUO: " + tag
```

**E não basta.** Depois de inserir, verifique **programaticamente** onde o
bloco caiu, em vez de o reler e achar que está bem:

```python
alvo = [i for i, l in enumerate(linhas) if 'strncmp(a, "inverte"' in l][0]
h    = [i for i, l in enumerate(linhas) if 'strncmp(line,' in l and i < alvo][-1]
print("bloco dentro de:", linhas[h].strip())
```

> **Escolha âncoras com o CONTEXTO que as torna únicas** — a linha anterior, o
> nome do handler, o comentário acima. Texto de estrutura repetida (`{`,
> `return;`, um `if` genérico) nunca é âncora.

O agravante deste caso: **eu tinha o guarda escrito noutro script da mesma
sessão**, com a mensagem `"AMBIGUO"`. Não o apliquei aqui.

> **Defesa que existe e não se aplica por hábito não é defesa: é sorte.** Se
> um guarda vale a pena escrever uma vez, ele vira o helper que TODA edição
> usa — não uma boa prática que se lembra quando dá jeito.

## Isenção de verificador supõe um PAPEL, e refactor muda papel

O `check_docs` recorta de propósito as tabelas de sonda (`kLivresPin`,
`kEncPin`, `kPino`) antes de procurar `seed::Dnn`: sonda aponta para pinos que
já têm dono, e sem a isenção todo pino instrumentado viraria conflito.

Em 28/08 promovi o `kEncPin` a **fonte única** — o `EncoderInit` passou a ler
dela num laço em vez de ter os pinos escritos à mão ao lado. A tabela deixou
de ser sonda e passou a ser a declaração.

O verificador continuou a recortá-la, e acusou: *"D0, D1, D2, D3, D4, D9,
D17, D18, D19 no PINOS.md e não aparecem no fonte"* — **nove pinos, todos os
dos encoders**. Verdade para ele, falso para a placa.

> **Ao promover uma estrutura a fonte única, procure quem a estava a ignorar
> de propósito.** Isenções são escritas contra um papel; quando o papel muda,
> a isenção passa a mentir — e mente para o lado silencioso, porque um
> verificador que ignora não acusa nada.

Neste caso o verificador acusou, e isso foi sorte da forma do erro: ele
comparava duas direcções. Uma isenção que só suprimisse o falso positivo teria
ficado calada para sempre.

## Versionar TUDO — todo artefato gerado carrega carimbo

Não é só o fonte que se versiona (§4). **Todo artefato gerado que sai do repo para
ser consumido lá fora** — BOM, export, template, pacote — carrega um **carimbo** no
início e num lugar válido para o seu formato (`carimbo: <arquivo> v<N> · <data> ·
placas: …`; CSV como comentário `#`, Markdown em comentário HTML, xlsx/xls numa
célula dedicada, pacote no `LEIA-ME.txt`), e o **`export/MANIFEST.md` é a fonte
única de "o mais novo"**: antes de subir/consumir um artefato, confere-se o carimbo
dele contra o MANIFEST e, se não bater, `git pull`. Nasce de um engano real
(2026-09-12): um carrinho Mouser com um MPN de um **export superado** enquanto o
repo já tinha o novo — um artefato que não se versiona não deixa detetar que
envelheceu. O detalhe do método vive na skill `pcb`
(`references/versionamento.md`, secção «Versionar TUDO»).
