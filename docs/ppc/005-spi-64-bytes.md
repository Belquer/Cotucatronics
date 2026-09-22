# ppc/005 — por que um envio de 64 bytes esgota o prazo no SPI do painel
<!-- doc-check: ignore BlockingTransmit (metodo da libDaisy, biblioteca externa) -->
<!-- doc-check: ignore oledspi (comando da consola serie, em src/main.cpp) -->
<!-- doc-check: ignore SSD130x (driver da libDaisy, biblioteca externa) -->

> **Estado: PLANO.** O sintoma está **contornado e a funcionar** desde
> hal 0.82.0 — o quadro vai em pedaços de 32 bytes. Este documento é a caça à
> **causa**, que continua desconhecida e escrita como tal.

- **Data:** 2026-09-22
- **Ramo:** `claude/slack-session-sm2ldz`
- **Pareia com:** `battle_scars.md` **15fi**
- **Instrumento que já existe:** comando `oledspi [prescaler]` na consola série

## §1 — O que está MEDIDO, e é daqui que se parte

Envios feitos pela sonda, com a configuração exacta do painel:

| tamanho | PS_2 | PS_8 | PS_256 |
|---|---|---|---|
| 1 … 32 bytes | OK, tempo cresce | OK, tempo cresce | OK, tempo cresce |
| 33 | OK | — | OK |
| 40 | **ERR, 100 ms** | — | OK (2 343 µs) |
| 64 | ERR | **ERR, 99 988 µs** | **ERR, 99 492 µs** |
| 128 | ERR | ERR | ERR |

Três factos que qualquer hipótese tem de explicar:

1. **Até 32 bytes passa sempre**, em qualquer velocidade.
2. **A fronteira move-se com o relógio** — mais lento tolera mais bytes (40
   passa no `PS_256` e parte no `PS_2`). Logo **não é só o FIFO**, que tem
   32 bytes fixos no `SPI1` do H7.
3. Quando falha, gasta **exactamente o prazo** (100 ms) e devolve `ERR`.

⚠️ E o mais importante: **isto funcionou a 23/08** (bts 13z), com a mesma
libDaisy, a mesma configuração e o mesmo driver — em jumpers curtos, direto à
Daisy. O que mudou foi o caminho físico e ~40 versões de firmware.

## §2 — Hipóteses, e o teste que separa cada uma

| # | hipótese | o teste que a mata ou confirma |
|---|---|---|
| H1 | o `BlockingTransmit` espera um sinal que só chega com o FIFO a drenar, e a drenagem depende de interrupções que o áudio esfomeia | correr o `oledspi` com o áudio **parado** e comparar a fronteira |
| H2 | o periférico entra em erro (overrun/underrun) e o `Result` esconde qual | ler os registos `SR`/`IER` do `SPI1` logo a seguir ao `ERR` e imprimi-los |
| H3 | é do `NSSPMode = SPI_NSS_PULSE_ENABLE` da libDaisy com `MasterInterDataIdleness = 0` | repetir com `nss` em software, guiando o `D7` à mão |
| H4 | regressão nossa entre 23/08 e hoje | `git bisect` sobre o firmware, com o `oledspi` como critério — **é o teste mais barato e o único que dá uma data** |

**Ordem: H4, H1, H2, H3.** O H4 não precisa de entender nada do H7 e devolve o
commit; os outros só valem se o H4 disser "sempre esteve assim".

## §3 — O que NÃO se faz

⚠️ **Não se tira o envio em pedaços antes de a causa estar fechada.** Ele está
a funcionar e o instrumento depende dele. A tentação de "consertar a sério
primeiro" custaria o painel outra vez.

⚠️ **Não se mexe na libDaisy do disco.** Se a causa for dela, o conserto vive
na nossa subclasse `OledFb`, que já existe — ou num *fork* declarado, nunca
numa edição local que a próxima actualização apaga em silêncio.

## §4 — Aceitação

- Uma frase que diga **porquê**, com o registo ou o commit que a prova.
- O `oledspi` a mostrar a fronteira a mudar quando a causa é removida.
- Se a causa for nossa: o conserto, e o envio em pedaços **mantido na mesma**
  como cinto de segurança, com o comentário a dizer que já não é necessário.
- Se a causa for da biblioteca ou do silício: escrita no `battle_scars.md`
  **15fi** como desfecho, e o contorno passa a ser permanente e justificado.

## §5 — O que anda a reboque disto

- **Módulo OLED novo.** O caminho está medido inteiro (`STATUS.md`); o módulo
  actual recebe dados correctos a 1,5 V e não responde.
- **O custo do painel no laço:** hoje −8% das voltas com o painel a desenhar.
  Antes do conserto eram −99%. Se a causa se fechar, vale remedir.
