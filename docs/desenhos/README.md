# Desenhos técnicos

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

Desenhos que **não** têm fonte KiCad e por isso não se geram — ficam por
julgar um a um, com a pergunta que vale mais que o conserto: **isto ainda
serve para quê?**

- `docs/operacao/esquematicos/revB/caixa-pedal-4-faces.svg` — citado pelo
  `ROTEIRO-KICAD.md` §3. As seis páginas que o substituiriam **não existem
  neste repo**.
- `docs/operacao/esquematicos/revB/caixa-pedal-layout.svg` — citado só pela
  emenda da `design-review`, como prova histórica.
- `docs/operacao/esquematicos/piezo_v2_protecao.svg` — a protecção do piezo
  **já vive** no `cotuca_preamp.kicad_sch` (D1 BAV99, R1, R2). Pela regra
  acima, é candidato a morrer.
- `cotucatronics_esquematicos_v3.svg` — superseded pelo `v4`, citado por
  ninguém.
- `cotucatronics_esquematicos_v4.svg` · `cotucatronics_build_sheet.svg` —
  citados por ninguém; o `build_sheet` não tem equivalente em KiCad.
