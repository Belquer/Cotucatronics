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
