# TESTES.md — Cotucatronics OS v0.2 · roteiro de bancada

**Documento v0.1.0** · última execução: 2026-08-15

Roteiro mínimo pra dizer "a placa está viva e o painel falso responde"
antes de encostar em DSP. T1–T3 rodam só com USB; T4 espera hardware.

---

## Ambiente validado

| Item | Versão |
|---|---|
| Placa | Daisy Seed 2 (DFM) — detectada por `CheckBoardVersion()`, codec PCM3060 |
| Toolchain | ARM GNU Toolchain 15.3.rel1 (`arm-none-eabi-gcc 15.3.1`) |
| libDaisy | `~/dev/libDaisy` (`libdaisy.a`, 7.6 MB) |
| dfu-util | 0.11 |
| Firmware | `hal_daisy_seed v0.2.1`, FLASH 103732 B / 128 KB (79.14%) |

> O toolchain do Homebrew (`brew install arm-none-eabi-gcc`) **não serve**:
> vem sem newlib, e a libDaisy morre em `stdint.h: No such file or directory`.
> Use o cask `gcc-arm-embedded` (pede senha de admin).

## Como rodar

```bash
make -C ~/dev/libDaisy          # uma vez
make BOARD=daisy_seed
# BOOT + RESET + solta BOOT
make program-dfu               # "Error during download get_status" no fim é normal
./cotuca_serial.py --send board list "k1 0.5" --gap 1.3
```

`make program-dfu` sempre termina com `Error 74` no `leave request`: a placa
pula pro app e para de responder ao último `get_status`. Se apareceu
`File downloaded successfully`, gravou.

---

## T1 — LED pisca 1×/seg · ✅ PASSOU

**Procedimento:** gravar, olhar o LED USER.
**Esperado:** ~1 Hz. Pisca **rápido** (10 Hz) = `CheckRequirements()` reprovou a placa.

**Resultado:** piscando devagar. Confirmado também pelo serial: o boot imprime
`pronto.` em vez de entrar no laço de pisca rápido, ou seja a Seed 2 atende
`needs_audio_in`, `min_knobs 4` e a RAM exigidos pelo manifest.

## T2 — painel falso via serial 115200 · ✅ PASSOU

**Procedimento:** `board`, `list`, `k1 0.5`, `list`.

```
> board      < Daisy Seed | knobs:4 | in:sim | 48000Hz/blk48
> list       <  0 level      norm=0.70 real=0.70
             <  1 tone_freq  norm=0.40 real=848.00
             <  2 tone_mix   norm=0.00 real=0.00
             <  3 spare      norm=0.00 real=0.00
> k1 0.5     < knob 1 -> 0.50
> list       <  0 level      norm=0.50 real=0.50
```

Sample rate e block size batem com o `BoardInfo`. A tradução
normalizado→real do manifest confere: `0.40` em `[80, 2000]` = 848 Hz.

## T3 — áudio no AUDIO_OUT_L · ✅ PASSOU

**Procedimento:** cabo em **pino 18 (AUDIO OUT L)** e **pino 20 (AGND)**;
`tone on`, `k2 0.1875`, `k2 0.8`, `k5 0.5`, `tone off`.

**Lado do firmware — passou:**

```
> tone on     < tone ON            (tone_mix 0.00 → 0.50)
> k2 0.1875   < knob 2 -> 0.19     → tone_freq real = 440.00 Hz
> k2 0.8      < knob 2 -> 0.80     → tone_freq real = 1616.00 Hz
> k5 0.5      < ? k1..k4 <0-1> | tone on/off | board | list
> tone off    < tone OFF
```

Conversão pra afinação alvo, com `tone_freq` em `[80, 2000]`:
`norm = (Hz − 80) / 1920`. Logo **440 Hz = 0.1875** e 880 Hz = 0.4167.

**Lado do ouvido — passou.** Tom contínuo e limpo no reteste com 3 s por altura
(848 → 440 → 1616 → 440 Hz), sem estalo, dropout ou distorção. A primeira
passada tinha soado como um bip curto; foi observação parcial, não defeito.

> **`k5` não existe neste firmware.** O parser do `main.cpp` cobre k1..k4 e o
> manifest tem `kNumKnobs = 4`. O `k5` cai no help. O teste constata a ausência
> do SRC MIX — não valida o roteamento dele.

## T4 — piezo / exciter / toggle de roteamento · ⏸️ BLOQUEADO

Espera o buffer de alta impedância, o TPA3116 e o painel. Nada a rodar.

---

## Defeitos conhecidos

| # | Sintoma | Estado |
|---|---|---|
| 1 | `list` travava a placa (CDC calada até o reset) | ✅ corrigido no HAL v0.2.0 |
| 2 | Todo `%f` saía vazio — `board` sem o `48000`, `list` com `norm= real=` | ✅ corrigido no Makefile |
| 3 | Primeira linha de log sai picotada e grudada na resposta seguinte | ❌ **em aberto** |
| 4 | Comando serial demora até 0,5 s pra responder | ⚠️ conhecido, fora do HAL |

**#1 — despacho serial rodava na ISR.** `SetReceiveCallback` dispara em contexto
de interrupção, e `hal::Log()` transmite pelo mesmo USB: o segundo `Log`
esperava um TX que só completa quando a ISR retorna. Deadlock. `board` (1 log)
passava, `list` (4 logs) matava. A ISR agora só enfileira (SPSC de 8, fila cheia
descarta em vez de bloquear) e o `PollSerial()` despacha do loop principal —
que é exatamente pra isso que ele existe no contrato do `hal.h`.

**#2 — printf sem float.** A libDaisy linka com `--specs=nano.specs`, e o
newlib-nano vem sem suporte a `%f`: falha **calada**, sem warning. Como o
manifest e o painel são todos float, metade da saída sumia. `LDFLAGS = -u
_printf_float` no Makefile, custa ~4,6 KB.

**#3 — buffer do Logger.** A resposta do primeiro comando não sai na hora: fica
presa e só é empurrada pela escrita seguinte, chegando truncada e grudada
(`Daisy Seed | knobs:4 $$ 0 level...`). Comando isolado sai íntegro. Um
`System::Delay(2)` por linha no `hal::Log` **não resolveu** — a causa parece
estar no buffer interno do Logger da libDaisy, não no código do projeto. O
`TransmitSync()` (bloqueante) que resolveria é `protected`.

**#4 — latência.** Com o despacho agora por polling, o `DelayMs(500)` do loop do
`main.cpp` virou o teto de resposta. Conserto natural: loop a ~10 ms contando
iterações pro LED. É `main.cpp`, fora da fronteira HAL — não alterado.

---

## Nota de arquitetura

`AllocBig()` está sendo descartado pelo linker: SDRAM aparece como 0 B / 64 MB
porque nada chama a função e o `--gc-sections` come o pool inteiro. Quando o
primeiro instrumento chamar, os 64 MB voltam de uma vez — e o `sdram_pool` pede
a região SDRAM **inteira**, sem folga pra mais nada em `.sdram_bss`.
