> ⚠️ **ABSORVIDA. A skill manda.** Este texto foi integrado em
> `.claude/skills/design-review/SKILL.md` a 2026-09-05, como o **passo 1** dela —
> não como apêndice, para a ordem não ficar ambígua. Fica aqui como registo
> do que se descobriu e da prova que o motivou; **se os dois divergirem, a
> skill está certa e isto está velho.**
>
> A ferramenta vive em `tools/svg_colisoes.py` — a versão corrente está no
> mapa do README, e não aqui, para não haver dois sítios a envelhecer.

# design-review — emenda 2026-09-05: o portão que pode dizer não

## O que falhou

Quatro entregas com "revisão visual limpa" tinham sobreposições. Duas eram
visíveis a olho e passaram porque o olho olhou o todo e não a fatia; duas eram
de meio pixel — texto a tocar o risco de uma cruz, rótulo a cruzar uma linha
tracejada cinzenta — invisíveis em qualquer render, e mesmo assim defeito.

**O olho é o instrumento errado para sobreposição fina.** Serve para
composição, legibilidade, hierarquia. Não serve para "este texto toca esta
linha?", que é pergunta de geometria e tem resposta exata.

## A regra nova, e vem ANTES do olho

> **Todo SVG entregue passa por `svg_colisoes.py` e sai com ZERO colisões.
> Só depois entra a inspeção visual — e ela passa a olhar composição, não
> toque.**

`svg_colisoes.py ficheiro.svg` calcula a caixa de cada texto e testa contra:
- **texto × texto** — sempre defeito
- **texto × segmento** — geometria real (Liang–Barsky), não bbox de path; um
  path em L não acusa o texto que está no canto vazio dele
- **texto × borda de forma** — texto DENTRO de um rect/círculo é etiqueta e é
  legítimo; texto a **cruzar** a borda é defeito
- **formas com `opacity < 0,5`** são sombreado decorativo e não contam

Sai com código ≠ 0 se houver colisão. É isso que o torna portão: pode dizer
não, e um script que o chama pára.

**Largura de texto é estimada** (0,55 em por caractere, Helvetica). É
conservadora — acusa uns pixels antes do toque real. Isso é o comportamento
certo para um portão: falso positivo custa mover um rótulo; falso negativo
custa uma entrega errada.

## Como se torna automático

1. **Nos geradores:** o `.py` que escreve o SVG chama o verificador antes de
   copiar para `outputs/`; com colisões, escreve para `/tmp` e recusa entregar.
   Um `assert` no fim do gerador.
2. **Na skill:** o passo 1 de qualquer revisão de SVG é correr a ferramenta e
   colar a linha de resultado no relatório — `N textos, M segmentos, 0
   colisões`. Sem essa linha, a revisão não aconteceu.
3. **Para o que não é SVG** (PNG de plot, PDF, render do KiCad): a ferramenta
   não alcança. Aí a regra é a que já existia — fatiar em regiões e olhar cada
   uma — **mais** a lição de hoje: olhar o todo não substitui olhar a fatia,
   porque o todo esconde exatamente o que é pequeno. Para o KiCad, o portão é
   o `kicad-cli sch export svg` seguido desta mesma ferramenta.

## A ordem, fixa

```
gerar → svg_colisoes (0) → fatiar e olhar (composição) → entregar
```

Qualquer edição depois de qualquer um dos três reinicia os três.

## Resultado de hoje, como prova

| ficheiro | antes | depois |
|---|---|---|
| caixa-fabricacao.svg | 30 acusações (2 reais visíveis, 4 quase-toques, 24 falsos por trapézio) | **0**, ferramenta corrigida para contentores |
| preamp-piezo.svg | 3 quase-toques que o olho nunca viu em três revisões | **0** |
| `caixa-pedal-4-faces.svg` *(aposentado 2026-09-05)* | 2 falsos por sombreado | **0**, com o filtro de opacidade |
| `caixa-pedal-layout.svg` *(aposentado 2026-09-05)* | 1 falso por sombreado | **0** |

A ferramenta apanhou o defeito que o dB viu **e** três que ninguém tinha visto.
