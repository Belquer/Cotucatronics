# `caixa/` — o artefacto físico, ao lado do `kicad/`

Os dois artefactos deste projecto são a placa e a caixa. Vivem no mesmo nível.

**Caixa cunha, 200 × 120, frente 55 e trás 80, em acrílico transparente de
3,0 mm.** Casca soldada (tampa + 4 paredes, juntas a dedo); **fundo removível**,
que é por onde se abre — e é nele que o carrier e o amp montam.

| ficheiro | o que é |
|---|---|
| `cotuca_caixa_corte.py` | **A FONTE.** Gera o corte a partir de `--t` e `--kerf`. Tem `if __name__ == "__main__"`, por isso o `kicad/pcb_contorno.py` **importa-o** para dimensionar as placas |
| `cotuca_caixa_corte_acrilico_3p0.svg` · `.dxf` | o que vai para a Ponoko. O nome leva a espessura, de propósito |
| `CAIXA-ACRILICO-MONTAGEM.md` | materiais, solvente, calços, ordem de montagem, percursos de cabo |

```bash
python3 caixa/cotuca_caixa_corte.py --t 3.0 --kerf 0.2
python3 tools/nesting.py caixa/cotuca_caixa_corte_acrilico_3p0.svg --pecas=19
```

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

Estado actual: **19 peças, 375,0 × 299,5 numa P2, margem 9,0 mm, ≈ 6,07 m de
corte.**

## Custo

**Medido na cotação real: apertar o encaixe de 1364 para 1123 cm² poupou
$1,90.** Faz-se à mesma — é automático e tira risco de recusa — mas **não é
onde se projecta para poupar**: o encaixe move a viagem do laser, não o corte,
e o corte são os mesmos 6,07 m antes e depois.

Onde se projecta, por ordem de efeito:

1. **O tamanho da chapa**, que se decide antes de desenhar. A tampa tem 200 mm
   e a P1 tem 181 — uma caixa ≤ 181 na maior dimensão cai um degrau inteiro.
   É decisão de **instrumento**, e nenhum encaixador a atravessa.
2. **Corte e número de peças.** Dos 6,07 m, 2,00 são dentes/furos/rasgos e
   **0,70 — 12% — são os doze calços de 12 mm**. O custo segue o perímetro, não
   a área: peça pequena é cara por milímetro quadrado.
3. **Menos dentes e mais largos**, pelo `n_target` do `cells()`.
4. **Gravação desligada** no upload.

Não confirmado, e escrito como tal: o afastamento mínimo à borda, o preço do
acrílico transparente, e se o site novo — que dá cotação por peça — ainda cobra
a chapa inteira. **Uma peça pequena carregada no site deles responde às três.**
Tabela de estados na skill `/caixa`, §6.

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
