# `caixa/` — o artefacto físico, ao lado do `kicad/`

Os dois artefactos deste projecto são a placa e a caixa. Vivem no mesmo nível.

**Caixa cunha, 200 × 120, frente 55 e trás 80, em acrílico transparente de
3,0 mm.** Casca soldada (tampa + 4 paredes, juntas a dedo); **fundo removível**,
que é por onde se abre — e é nele que o carrier e o amp montam.

| ficheiro | o que é |
|---|---|
| `cotuca_caixa_corte.py` | **A FONTE.** Gera o corte a partir de `--t` e `--kerf`. Tem `if __name__ == "__main__"`, por isso o `kicad/pcb_contorno.py` **importa-o** para dimensionar as placas |
| `cotuca_caixa_20260906_r3_acrilico_3p0_037a9c30.svg` · `.dxf` | o que vai para a Ponoko |

**20 peças.** A vigésima é o **REFORÇO da tampa** — faixa de 193 × 50 colada por
baixo, só onde os arcade estão. Um pé são ~300 N a meio de um vão, e 3 mm de
acrílico raja **a partir dos furos de ø30**. Fica 6 mm efectivos onde o pé pisa
e 3 mm onde os pots atravessam — que é a razão de a tampa ser de 3 (o PTV09 tem
~7 mm de rosca). Custa **+$4,52**.

✅ Os furos do reforço são **ø30 e concêntricos** com os da tampa, confirmado
por medição no ficheiro gerado. O arcade monta num painel de 6 mm — provado
pelo **protótipo em madeira, que tinha 6 mm** e onde os botões apertaram.

Os três arcade estão à **distância máxima que a face permite**: x = 30 · 100 ·
170, **70 mm entre eixos e 40 entre bordos**. Se ainda assim um pé largo
apertar dois, o que resolve é um ressalto — ver o documento de montagem, §4c.
| `CAIXA-ACRILICO-MONTAGEM.md` | materiais, solvente, calços, ordem de montagem, percursos de cabo |

```bash
python3 caixa/cotuca_caixa_corte.py --reforco-furo 30
python3 tools/nesting.py caixa/cotuca_caixa_20260906_r3_acrilico_3p0_037a9c30.svg --pecas=20
```

## O nome é a identidade

```
cotuca_caixa_20260906_r3_acrilico_3p0_037a9c30.svg
             │        │       │   │   └── QUAL É — digest do desenho
             │        │       │   └────── espessura
             │        │       └────────── material
             │        └────────────────── intenção — revisão
             └─────────────────────────── QUAL VEM DEPOIS — data da revisão
```

⚠️ **Um digest identifica e não ordena.** A data vem primeiro para o `ls` sair
cronológico, e é da **revisão**, não da corrida — gerar duas vezes dá o mesmo
ficheiro. O `revisoes.json` guarda `revisão → digest` e o gerador **recusa-se a
escrever** se a mesma revisão der geometria diferente.

⚠️ **A revisão bumba-se à mão e a mão esquece; o digest não.** Geometria
diferente é ficheiro diferente **por construção** — a caixa com reforço e a
caixa sem reforço chegaram a partilhar o nome `..._acrilico_3p0.svg`, uma já
encomendada e a outra não, indistinguíveis na pasta. Não volta a acontecer.

| revisão | o que é | encomenda | onde está |
|---|---|---|---|
| r1 | 19 peças · arcade a 40/100/160 · sem reforço | **#1263497**, $35,66 | `git show cbc6fbd:caixa/cotuca_caixa_corte_acrilico_3p0.svg` |
| r2 | 20 peças · arcade a 30/100/170 · reforço da tampa | 06/09, $38,23 — entrega 15/09 | `git show 6a86006:` |
| **r3** | + **aberturas de USB** na parede de trás, pedidas pela placa | — | na pasta |

⚠️ **A encomenda da r2 foi feita com o ficheiro sob o nome ANTIGO**
(`cotuca_caixa_corte_acrilico_3p0.svg`), porque foi enviado antes de a regra de
nomes existir. **A geometria é a da r2** — a Ponoko mede **374,10 × 323,00**, que
é exactamente o bbox da r2 (a r1 media 372,00 × 296,55). Fica escrito aqui
porque o nome na factura já não existe no repositório, e sem isto ninguém liga
a caixa que chegar ao desenho que a fez.

A r1 nasceu antes desta regra e chamava-se `..._corte_acrilico_3p0.svg` — o
mesmo nome que a r2 tinha até agora. É o caso que criou a regra.

⚠️ **`--reforco-furo` é obrigatório e não tem omissão.** O reforço da tampa
engrossa o painel na faixa dos arcade, e o diâmetro do furo decide se o botão
monta em 6 mm ou em 3. **Mede no arcade que já está comprado**: comprimento da
rosca e diâmetro exterior da porca. O gerador recusa-se a correr sem a escolha
e diz o que pedir.

O `_previa.svg` sai no mesmo comando, com traço grosso, só para olhar — não
entra no repositório, regenera-se.

## O que o gerador garante, e o que não

Três portões próprios, todos a **recusar antes de escrever**:

1. **nesting** — tudo dentro da chapa, nada sobreposto. O `tools/nesting.py`
   confirma por fora, e conta as peças: uma peça que não foi emitida não colide
   com nada, e um ficheiro com 18 passa como um com 19.
2. **espelho** — peça rodada que na verdade foi espelhada corta na perfeição e
   monta ao contrário. Confere-se a área com sinal.
3. **margem à borda** — 3 mm. ⚠️ **Regra nossa**: a Ponoko diz «não uses a
   aresta crua da chapa» e não publica número.

Estado actual: **20 peças, ocupa 377,1 × 326,0 numa P2, margem 6,9 mm,
≈ 6,84 m de corte.**

## Custo — medido, não suposto

**O preço da Ponoko segue o comprimento de corte: ≈ $5,87/m.** Duas facturas
reais, mesmo material, prevêem-se uma à outra com **$0,02** de erro por corte e
**$3,52** por área. `Setup $0.00`, sem linha de material — **não há preço de
chapa**; a P1/P2/P3 só importa por caber.

| | metros | custo |
|---|---|---|
| rectângulos lisos (o chão) | 4,07 | $23,89 |
| dentes, furos e rasgos | 2,00 | $11,74 |
| — destes, os **12 calços** | 0,70 | **$4,11** |
| **o encaixe apertado** | **0,00** | **$0,00** |

⚠️ **O encaixe não poupa nada.** O corte é propriedade das peças, não da
arrumação — os mesmos 6,07 m antes e depois. Faz-se à mesma, por causa do
upload. O $1,90 entre as duas encomendas veio de **regerar a 3,0 em vez de
6,4**, que encurtou o corte de 6,39 para 6,07 m.

Projectar para poupar: **menos peças e menos peças pequenas** (o custo segue o
perímetro), dentes mais largos pelo `n_target` do `cells()`, caixa mais pequena
— e sobretudo **o envio, $22,43 sobre $35,66, que é 39% do subtotal**: várias
caixas num pacote é a maior alavanca de todas, e não se mexe no desenho.

Detalhe e o que falta confirmar: skill `/caixa`, §6.

## ⚠️ O que foi apagado a 06/09, e porquê

Morreram: as 6 páginas de fabricação (`medidas/`), os dois geradores delas
(`gen_caixa*.py`), e o corte em nogueira de 6,4 com o seu `.dxf`, `.png`,
`CAIXA-V1-NOGUEIRA.md` e `ponoko_nogueira/`.

As páginas desenhavam uma caixa que **nunca foi cortada**: paredes de 6 e tampa
de 8, frente de 40, dois arcade em vez de três, e um berço de powerbank que não
existe. O corte em nogueira era **real** — foi para a Ponoko — mas é **outra
caixa**.

> **Uma fonte vencida ao lado da certa mede-se igualmente bem.** A única coisa
> que as distingue é uma verificação que ninguém faz antes de confiar. Foi
> exactamente assim que as duas PCB foram dimensionadas contra a caixa errada,
> duas vezes seguidas — cicatriz **15cf**.

Está tudo no histórico:

```bash
git show 94066fd:caixa/medidas/caixa-fabricacao.svg > /tmp/velho.svg
```

O que tinha valor e não era cota — as regras de percurso de cabo — foi
**resgatado** para o `CAIXA-ACRILICO-MONTAGEM.md`, §6b, antes de a página
morrer.
