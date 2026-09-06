# Desenhos técnicos
<!-- doc-check: ignore build_sheet (nome de um SVG em docs/operacao/esquematicos/, nao simbolo de src/.) -->
<!-- doc-check: ignore svg_colisoes (a ferramenta vive em `tools/`, e o check_docs varre so `src/`. Nao e fantasma: e limite de alcance dele.) -->
<!-- doc-check: ignore cotuca_energia_previa (desenho APOSENTADO em 05/09. A lapide fica: e ela que impede alguem de o regenerar sem ler por que saiu.) -->
<!-- doc-check: ignore energia_preview (idem) -->
<!-- doc-check: ignore preamp_preview (idem) -->

**Uma pasta, e uma regra que a governa inteira:**

> **Quando um artefacto ganha uma fonte, os desenhos anteriores dele ou
> passam a ser gerados dessa fonte, ou morrem.**
>
> Um desenho que descreve o que a fonte deixou de dizer é **pior que
> nenhum**: tem ar de oficial e mente.

## Como se mantém atualizada — e não é por lembrança

```bash
python3 tools/desenhos.py            # confere; sai != 0 se algum está velho
python3 tools/desenhos.py --gerar    # regenera e confere
```

O critério é mecânico e não pede opinião: **se a fonte é mais nova que o
desenho, o desenho está velho.** Não se pergunta se mudou o que importa —
essa pergunta é onde a disciplina se perde.

## O que há aqui

### `esquematicos/` — **gerados**, não se editam

| desenho | fonte |
|---|---|
| `cotuca_energia.svg` | `kicad/cotuca_cerebro/cotuca_energia.kicad_sch` |
| `cotuca_preamp.svg` | `kicad/cotuca_cerebro/cotuca_preamp.kicad_sch` |
| `cotuca_soquete.svg` | `kicad/cotuca_cerebro/cotuca_soquete.kicad_sch` |

Saem do mesmo ficheiro que a fábrica vai ler, portanto **não podem divergir —
não têm vida própria.**

### `caixa/` — **gerados**, menos a página 1

Seis páginas do desenho de fabricação da caixa, por dois geradores que
correm daqui:



⚠️ **As cotas marcadas "confirmar" mudam depois de medir as peças — e mudam
no `.py`, nunca no SVG.** Um SVG editado à mão diverge do gerador na primeira
regeneração, e aí ninguém sabe qual manda.

⚠️ **As 6 páginas de fabricação da caixa foram apagadas a 06/09.** Desenhavam
uma caixa que nunca foi cortada. O artefacto da caixa é agora o
`caixa/cotuca_caixa_corte.py`, que gera o ficheiro de corte a partir de
`--t`/`--kerf` e traz portões próprios — não passa por este vigia.

Ao contrário dos esquemáticos, estas **passam o `svg_colisoes`** — são
`<text>` a sério — e o próprio gerador corre o portão a cada página, com
`assert` no fim: com colisões, não entrega.

### ⚠️ O que se perde nessa troca, e é honesto dizê-lo

Estes SVG **não passam o portão do `svg_colisoes.py`**: o `kicad-cli` desenha
as letras como *paths*, não como `<text>`, e a ferramenta responde
`NAO VERIFICAVEL — nenhum <text> encontrado`, código **3** — que é *"não
consegui olhar"*, e não *"está limpo"*.

Trocámos **"pode divergir mas verifica-se"** por **"não pode divergir mas não
se verifica"**. Para figura de documentação é a troca certa: a verdade do
desenho vale mais que a garantia de não haver toque, e o toque tem outro
portão a montante — o ERC. Mas fica **declarado**, não presumido.

A inspecção destes é a do olho, em fatias, como manda a `design-review` §3.

## O que morreu, e porquê

**`preamp-piezo.svg`**, aposentado a 05/09. Era a figura de conversa de antes
de existir esquema. Quando o pré-amp ganhou um `.kicad_sch`, passou a
descrever um circuito que não é o da placa — dizia `C2 1 nF` e mostrava o
`RV1` vivo, quando o esquema diz `C2A 20n` e o `RVA` está **DNP** desde que a
bancada aprovou o seguidor. Não se consertou: aposentou-se. Continua em git.

**As três prévias dos geradores** (`cotuca_energia_previa`, `energia_preview`,
`preamp_preview`), removidas no mesmo dia: saída de um caminho que deixou de
ser a fonte quando o projecto KiCad entrou.

## Ainda fora daqui, e a decidir

Desenhos que **não** têm fonte e por isso não se geram — ficam por julgar um
a um, com a pergunta que vale mais que o conserto: **isto ainda serve para
quê?**

- `docs/operacao/esquematicos/piezo_v2_protecao.svg` — a protecção do piezo
  **já vive** no `cotuca_preamp.kicad_sch` (D1 BAV99, R1, R2). Pela regra
  desta pasta, é candidato a morrer.
- `cotucatronics_esquematicos_v3.svg` — superseded pelo `v4`, citado por
  ninguém.
- `cotucatronics_esquematicos_v4.svg` · `cotucatronics_build_sheet.svg` —
  citados por ninguém. O `build_sheet` não tem equivalente em KiCad, portanto
  sobrevive à regra — mas não necessariamente à pergunta.

## Já morreram

| desenho | quando | porquê |
|---|---|---|
| `preamp-piezo.svg` | 05/09 | descrevia o pré-amp de antes do `.kicad_sch` — `C2 1 nF` e `RV1` vivo, contra `20n` e `DNP` |
| `caixa-pedal-4-faces.svg` · `caixa-pedal-layout.svg` | 05/09 | as duas versões anteriores da caixa, substituídas pelas seis páginas |
| `cotuca_energia_previa.svg` · `energia_preview.svg` · `preamp_preview.svg` | 05/09 | prévias dos `gen_*.py`, que deixaram de ser a fonte |

Todos continuam em git.
