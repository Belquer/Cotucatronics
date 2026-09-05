# `caixa/` — o artefacto físico, ao lado do `kicad/`

Os dois artefactos deste projecto são a placa e a caixa. Vivem no mesmo nível.

**O portão é o `tools/nesting.py`, não o `svg_colisoes`.** Num desenho, dois
textos sobrepostos são ilegíveis; num ficheiro de corte, duas **peças**
sobrepostas são uma peça perdida e uma chapa gasta, e uma peça fora da chapa é
um pedido recusado. São perguntas diferentes e nenhuma substitui a outra.

```bash
python3 tools/nesting.py caixa/cotuca_caixa_corte_nogueira_6p4.svg --pecas=19
```

Verifica três coisas: tudo dentro da chapa (Ponoko P2, 384 × 384), nenhuma peça
sobre outra, e **o número de peças** — porque uma peça que não foi emitida não
colide com nada, e um ficheiro com 18 passa exactamente como um com 19.

| ficheiro | o que é |
|---|---|
| `gen_caixa.py` · `gen_caixa_paginas.py` | **a fonte** das páginas de fabricação. Correm o `svg_colisoes` antes de escrever |
| `paginas/` | as 6 páginas cotadas, geradas. Vigiadas pelo `tools/desenhos.py` |
| `cotuca_caixa_corte_nogueira_6p4.svg` · `.dxf` · `_previa.png` | o corte v1 em nogueira 6,4 mm, kerf 0,15 |
| `CAIXA-V1-NOGUEIRA.md` | como encomendar, colar e acabar |
| `ponoko_nogueira/` | as 19 peças em **8 ficheiros separados**, reserva para o caso de o encaixe único ser recusado no upload ou de se querer cortar só uma peça de substituição |

## ⚠️ Os ficheiros de corte não têm gerador

O `CAIXA-V1-NOGUEIRA.md` descreve o SVG como *"regenerado para 6,4 mm e kerf
0,15"* — o que diz que houve um gerador com parâmetros. **Ele não está aqui,
nem no disco, nem no histórico do git.** Procurado por nome e por conteúdo a
05/09.

Enquanto assim for, os `.svg`/`.dxf` de corte são **mantidos à mão**: mudar a
espessura da madeira ou o kerf deixa de ser um parâmetro e passa a ser redesenho.
É a regra do projecto ao contrário — *"quando um artefacto ganha uma fonte, os
desenhos anteriores ou passam a ser gerados dessa fonte, ou morrem"* — e aqui o
artefacto nunca teve fonte versionada.

**Ou o gerador aparece e passa a ser a fonte, ou isto fica escrito como está.**
O que não pode é a próxima pessoa supor que existe.
