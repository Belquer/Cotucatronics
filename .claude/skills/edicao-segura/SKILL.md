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

### 5. Relatar

Dizer **quais arquivos mudaram e em que versão ficaram**. Sempre.

## Regra de premissa

Se a tarefa começa com uma premissa sobre o estado do sistema ("com a v0.39.0
gravada..."), **verificar a premissa antes de agir sobre ela**. Uma sessão
inteira já foi planejada em cima de um firmware que não estava na placa.
