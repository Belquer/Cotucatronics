# Audit — commit `696fa0c`, placa v0.171.0

> **Feito depois de uma sessão de ~40 versões**, com a multiplexação MIDI
> refeita duas vezes, presets em RAM implementados, três encoders com o mapa
> de pinos corrigido a meio caminho, e o OLED que nunca funcionou.
>
> **O audit não conserta** (R4). Os fixes são commits separados.

## O que passou

| passada | resultado |
|---|---|
| **P0** inventário | 30 arquivos, todos classificáveis. `detune.h` fora do build **com registro completo** em FEATURES.md |
| **P1** contexto | **Callback limpo** — `main.cpp:1652–2276`, zero `Log`, `MidiSendBytes`, `malloc`, `DelayMs` ou desenho de OLED |
| **P2** ordem | `hal::Init` → `params.Init` → `manifest.Init` → `pmapa`/hash → `EncoderInit` → `midi.Init` |
| **P4** tetos | params 86/96 · vínculos 86/128 · sacrificáveis 10/12 · botões 2/6 · randomizáveis 2/8 |
| **P5** órfãs | **nenhuma** — todo contador tem escritor e leitor |
| **P7** aritmética | `Set()` clampa via `Clamp01`, então os casts `×127+0.5` dos presets não estouram |
| **P9** simetria | **37 famílias `_a`/`_b`, zero órfãs dos dois lados** |

## Achados

### 🟠 1 — Quatro documentos descrevem o esquema de QUATRO canais

`operacao/CCMAP.md:37` · `operacao/ACEITE-MULTIPLEXACAO.md:9,13,29,68` ·
`operacao/PRESETS.md` · `STATUS.md`

A mudança para seis canais (v0.156.0) atualizou só o `RETRABALHO-LOOPY.md` —
**que é gerado.** Os escritos à mão ficaram.

**Consequência:** o roteiro de aceite manda conferir `ch3`/`ch4` no MIDI
Wrench, e a placa fala **ch4/ch5/ch6**. Manda o operador ao canal errado.

**Fix: doc.**

### 🟡 2 — `ApplyMidiCc` declara "duplicata é erro" e nada detecta

`manifest.h:648`. O `midi_overflow` só pega estouro de capacidade.

**E a multiplexação piorou:** antes, dois `AddMidi` com o mesmo número
saltavam à vista; agora o mesmo número aparece **legitimamente duas vezes**,
uma por canal, e uma duplicata real se esconde entre as repetições legítimas.

**Fix: código.** Varredura no `Init`, contador `midi_dup`, `board` grita.

### 🟡 3 — Comentário revogado sobrevivendo acima do que o substituiu

`main.cpp:245-247` — três linhas sobre o `kCanalEco` imediatamente acima do
bloco que começa em 248 com *"O ESPELHO SAIU DO CH2"*.

**Fix: doc-no-código.** Apagar as três linhas.

### ⚪ 4 — README com versões velhas em 7 arquivos

`hal.h` 0.33/0.22 · `hal_daisy_seed.cpp` 0.38/0.24 · `main.cpp` 0.171/0.155 ·
`manifest.h` 0.81/0.79 · `midi.h` 0.8/0.7 · `qspi_raw.h` 0.2/0.1 ·
`test_midi.cpp` 0.4/0.2

**E o `check_docs` não pega:** a regra de deriva confere se o último commit
mexeu na versão — não confere a tabela do README. **Fix: código + doc.**

### ⚪ 5 — "76 CCs" em dois comentários que hoje seriam 86

`main.cpp:1114` e `1290`, usados como magnitude de referência para rajada.

## Meta-achado: o script do audit caiu na 12c

O detector de versões acusou `main.cpp` com cabeçalho `0.125.0`. Era **a
narração da remoção**, dentro do comentário que explica por que o número saiu
de lá. R5 pegou — mas só porque a regra manda ler as vizinhas antes de afirmar.

## Ordem de ataque

1. **🟠 1** — antes de voltar à bancada; é o único que muda o que se faz com as mãos
2. **🟡 3** — mesma região do `main.cpp` que o ⚪5 toca, um voo só
3. **🟡 2** — detector de duplicata
4. **⚪ 4** — regra do README no `check_docs`

## O que NÃO foi verificado (R7)

- **Nada de som.** A P8 inteira ficou de fora — exige A/B no mesmo binário e o
  ouvido do dB.
- **A trilha do OLED** — painel morto pelo conector, nenhuma afirmação sobre o
  driver dele é confirmável até o novo chegar.
- **Os quatro critérios de aceite dos encoders** — sentido inverso, push
  isolado, giro rápido, independência.
- **P3 completa** — cobri só as variáveis centrais.
