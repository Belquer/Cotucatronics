---
name: medicao-limpa
description: Pré-voo obrigatório antes de QUALQUER medição no Cotucatronics — prova que o canal fala, zera o ambiente, declara unidade e regime. Use antes de rodar qualquer teste de bancada, ler a caixa-preta, medir CPU, ou interpretar contadores. É a etapa 0 do IFÁ10000.
---

# Medição limpa — etapa 0 do IFÁ10000

**Um zero num canal não confirmado não é dado. É ausência de dado.**
Quatro janelas de teste de MIDI foram lidas como "link morto" quando o
log estava mudo por um `bb arm` meu (caso 8 de `docs/battle_scars.md`).

Nenhuma medição começa antes destes cinco passos.

## 1. Provar que o canal fala — ANTES de qualquer leitura

Mandar um comando de eco conhecido e **conferir a resposta**:

```
log on
board
```

- Sem eco → **aborta**. Não interpreta, não formula hipótese, não relata.
- Com eco → o canal está vivo e o silêncio seguinte passa a significar algo.

Se um comando de status imprimir `[LOG MUDO]`, o mudo está ativo: `log on`
destrava. Firmware < v0.39.0 não declara — nesse caso, resetar.

## 2. Zerar o ambiente — faz parte da medição

Estado de teste anterior contamina o teste seguinte. Já aconteceu duas vezes:
`tone on` esquecido mascarando o piezo na medição do reverb; búzios ligados
modificando o instrumento sozinho durante os testes de MIDI.

```
tone off
set buzios_on 0
list
```

Conferir no `list` se algum parâmetro está fora do default aprovado —
varredura de CC deixa `fx2_gain` e `src_mix` bagunçados. Se estiver, **o
RESET devolve o manifest aprovado** (e num teste de MIDI, exige reparear).

## 2b. Conferir que ninguém mais está mexendo

Estado de teste anterior não é a única contaminação. **Comandos fantasma
existem:** transições de link BLE fazem o iOS reenviar o estado de todos os
widgets, e o instrumento obedece. Já pôs sete parâmetros no máximo no meio de
uma sessão — `src_mix` e `gate_thresh` no topo emudeceram tudo.

```
ble      → quantas (re)conexões e quantos CCs a quarentena recusou
list     → algum param no extremo (0.00 ou 1.00) que você não pôs lá?
```

**Sete parâmetros no topo simultâneo não é gesto humano.** Se o `list` mostra
extremos que você não colocou, a medição anterior está contaminada e o
instrumento precisa voltar aos defaults antes de qualquer leitura.

## 3. Declarar o que está sendo medido

Toda medição registra, por escrito, antes de rodar:

- **unidade**: µs? % de que prazo? RMS, pico ou norm? real ou normalizado?
- **regime**: estreia ou permanente?
- **contexto**: qual bloco (48/256), qual clock, `-O2` ou `-O3`?
- **material**: qual sinal está entrando (ver etapa 6 do IFÁ10000)?

## 4. Um voo, uma variável

Antes de mexer, **listar os empregos daquela variável**. A terceira família
de bug de ordem é "uma coisa com dois empregos, e um teste que muda os dois
de uma vez". Se a variável serve a dois propósitos, o voo mede dois efeitos
somados e não separa nenhum.

## 5. Caixa-preta: armar por comando, no início do voo

- `bb arm` **explicitamente**, no começo — nunca no meio de um fluxo.
- **A sessão de leitura não escreve nada.**
- `bb arm` silencia o log até `log on` (ou reset). Está declarado no
  cabeçalho dos comandos de status a partir da v0.39.0.

## Ao terminar

Relatar o número **com a unidade e o regime junto**. Número sem unidade já
condenou um módulo bom (freeze, 233 965 µs de estreia contra 3 520 de regime).
