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
| `medidas/` | as 6 páginas cotadas, geradas. Vigiadas pelo `tools/desenhos.py` |
| `cotuca_caixa_corte_nogueira_6p4.svg` · `.dxf` · `_previa.png` | o corte v1 em **nogueira 6,4 mm**, kerf 0,15. Foi para a Ponoko — é real, mas é **outra caixa** |
| `cotuca_caixa_corte.py` | **A FONTE.** Gerador do corte em **acrílico cast 6,0**, kerf 0,2 — a caixa que está a ser feita. Tem `if __name__ == "__main__"`, por isso **importa-se** sem correr nada |
| `CAIXA-ACRILICO-MONTAGEM.md` | materiais e montagem em acrílico (Weld-On 4 e 16, calços com porca cativa) |
| `CAIXA-V1-NOGUEIRA.md` | como encomendar, colar e acabar |
| `ponoko_nogueira/` | as 19 peças em **8 ficheiros separados**, reserva para o caso de o encaixe único ser recusado no upload ou de se querer cortar só uma peça de substituição |

## O gerador do acrílico existe — o da nogueira não

O `cotuca_caixa_corte.py` (06/09) gera o corte em acrílico a partir de
parâmetros — `T`, `K`, `W`, `D`, `HF`, `HB` — e traz portão de nesting próprio.
**É a fonte, e o `kicad/pcb_contorno.py` importa-o.**

Os ficheiros da **nogueira** continuam sem gerador e são mantidos à mão. Ficam
como estão: são o registo de uma caixa que foi encomendada, não a caixa a fazer.

⚠️ **Defeito no gerador:** escreve `/home/claude/corte_preview.svg`, um caminho
absoluto de outra máquina. Rebenta em macOS ao chegar lá. As duas linhas úteis
(`cotuca_caixa_corte.svg` e `.dxf`) saem antes e depois dela.

## Gerar o corte

```bash
python3 caixa/cotuca_caixa_corte.py --t 3.0 --kerf 0.2
python3 tools/nesting.py caixa/cotuca_caixa_corte_acrilico_3p0.svg --pecas=19
```

**O ficheiro para a Ponoko é o `cotuca_caixa_corte_acrilico_3p0.svg`** — chapa
P2 (384 × 384), 19 peças, ocupa **375,0 × 299,5** com 9,0 mm de margem à borda,
≈ 6,07 m de corte. O `_previa.svg` é o mesmo com traço grosso, só para olhar.

⚠️ **A chapa não pode encolher para P1 (181 × 181): a tampa tem 200 mm de
largura.** Nenhuma disposição a faz caber — encolher a chapa exige encolher a
caixa. Mas o encaixe apertado (1123 cm² contra os 1364 das coordenadas à mão)
**paga-se à mesma**: a Ponoko cobra o fabrico por **tempo de máquina**, e o
exemplo que eles publicam mostra quatro peças espalhadas a custar $14,77 de
fabrico contra $4,53 encaixadas, com o material igual nos dois casos. Viagem do
laser é tempo.

E a gordura seguinte são os **12 calços — 0,70 m, ou 12% dos 6 metros de corte,
para doze quadrados de 12 mm**. Peça pequena tem perímetro enorme por unidade
de área.

⚠️ **O que NÃO está confirmado:** o afastamento mínimo à borda da chapa (os
3 mm que o gerador exige são nossos, não deles), o preço do acrílico
transparente, e se o site novo — que dá cotação por peça — ainda cobra a chapa
inteira. A tabela de estados está na skill `/caixa`, §6. **Uma peça pequena
carregada no site deles responde às três de uma vez.**

⚠️ **A profundidade do dente é a espessura.** Trocar de material é sempre
**regerar**, nunca só mudar a linha do pedido: um ficheiro cortado a 6 e
executado em 3 dá 3 mm de folga em cada junta e 3 mm de lingueta saliente — a
caixa não fecha em esquadro e o solvente não tem o que colar. O nome do
ficheiro leva a espessura por causa disso.

⚠️ **Uma coisa que nenhum portão mede:** a tampa vence 188 mm em **3 mm** de
acrílico e leva o **pé** do músico nos arcade. A nervura ajuda; 3 não é 6.
Cortar UMA, montar, e pisar antes da série.

## Quatro caixas plausíveis, e só uma vai ser feita

| | `medidas/*.svg` | nogueira 6,4 | gerador a 6,0 | **acrílico 3,0 (o encomendado)** |
|---|---|---|---|---|
| foi cortado? | **não, nunca** | sim | não | **é o que foi encomendado** |
| espessura | 6 (tampa 8, ou 9) | 6,4 | 6,0 | **3,0** |
| frente · trás | 40 · 70 | 55,08 · 80,07 | 55 · 80 | **55 · 80** |
| interior | 188 × 108 | 187,2 × 107,2 | 188 × 108 | **194 × 114** |
| altura útil frente · trás | 28 · 58 | — | 43 · 68 | **49 · 74** |
| chapa de fundo | não existe | 186,2 × 106,2 | 187 × 107 | **193 × 113** |
| arcade ø30 | 2 | 3 | 3 | **3** (freeze A · búzios · freeze B) |
| calços | 6 blocos de 12×12×**25** | → 12,8 | → 12,0 | **→ 6,0** |
| berço do powerbank | uma página inteira (p6) | **não existe** | não existe | **não existe** |

**A placa não depende de nenhuma destas colunas.** Está centrada na chapa de
fundo, e daí sai um invariante confirmado a correr o gerador nas duas
espessuras: os furos de fixação caem sempre em **(15,5 · 128,5) × (3,5 · 96,5)**
do canto da placa, e os eixos dos knobs sempre em **105/140/175 × 92** da face.

Duas consequências que não são cosméticas:

1. **Quem abre esta caixa é o fundo, não a tampa.** A casca é soldada (tampa +
   4 paredes) e os seis parafusos M3 sobem do fundo para os calços. O carrier e
   o amp montam no fundo e saem com ele. A electrónica serve-se por baixo.
2. **Os calços têm 12,0 mm** e a placa passa por cima de seis — o espaçador
   M3 × 8 do `CAIXA-ACRILICO-MONTAGEM.md` **não serve para o cérebro**; mínimo
   15 mm. Cabe: 43 mm úteis à frente.

**As `medidas/` descrevem a primeira coluna — uma caixa que não existe.** Pela
regra do projecto, ou o `gen_caixa*.py` passa a gerar-se do
`cotuca_caixa_corte.py`, ou as páginas vão abaixo. Enquanto não for uma coisa
nem outra, quem as ler está a ler uma caixa que nunca foi feita.
