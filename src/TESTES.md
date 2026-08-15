# TESTES.md — Cotucatronics OS · roteiro de bancada

**Documento v0.4.0** · última execução em placa: 2026-08-15 (T1–T3, firmware
v0.3.0) · firmware atual **v0.4.0 / HAL v0.3.1**, compilado e **ainda não
gravado** — T5 pendente.

Roteiro mínimo pra dizer "a placa está viva e o painel falso responde"
antes de encostar em DSP. T1–T3 rodam só com USB; T4 espera hardware;
T5 é o critério de aceite do primeiro módulo RNBO.

---

## Ambiente validado

| Item | Versão |
|---|---|
| Placa | Daisy Seed 2 (DFM) — detectada por `CheckBoardVersion()`, codec PCM3060 |
| Toolchain | ARM GNU Toolchain 15.3.rel1 (`arm-none-eabi-gcc 15.3.1`) |
| libDaisy | `~/dev/libDaisy` (`libdaisy.a`, 7.6 MB) |
| dfu-util | 0.11 |
| Bootloader | Electrosmith `dsy_bootloader_v6_4-intdfu-2000ms.bin` (vem com a libDaisy) |
| Firmware | Cotucatronics OS v0.4.0 / `hal_daisy_seed v0.3.1` · `APP_TYPE=BOOT_QSPI` |

Ocupação do firmware v0.4.0 (primeiro com módulo RNBO embarcado):

| Região | Usado | Total | % |
|---|---|---|---|
| FLASH (interna) | 0 B | 128 KB | 0,00% — é do bootloader agora |
| QSPIFLASH | 123 240 B | 7 936 KB | **1,52%** |
| SRAM | 71 100 B | 512 KB | 13,56% |
| SDRAM | 64 MB | 64 MB | 100,00% (o `sdram_pool` inteiro — ver nota no fim) |

> **Antes da migração o mesmo firmware ocupava 123 392 B / 128 KB = 94,14%
> da flash interna.** Um único módulo RNBO comeu quase tudo; o segundo não
> caberia. Daí o `APP_TYPE=BOOT_QSPI`.

> O toolchain do Homebrew (`brew install arm-none-eabi-gcc`) **não serve**:
> vem sem newlib, e a libDaisy morre em `stdint.h: No such file or directory`.
> Use o cask `gcc-arm-embedded` (pede senha de admin).

## Como rodar

### Passo único: gravar o bootloader (só na primeira vez por placa)

O bootloader mora na flash interna, como qualquer app normal — então
grava-se pelo **DFU da STM**: segure **BOOT**, toque **RESET**, solte BOOT.

```bash
make program-boot
```

Isso é **uma vez por placa**. Depois disso a flash interna é do bootloader e
não se mexe mais nela, a não ser pra atualizar o próprio bootloader.

### Ciclo normal: compilar e gravar o app na QSPI

```bash
make -C ~/dev/libDaisy          # uma vez
make BOARD=daisy_seed
# toque RESET e grave DURANTE a janela de graça (LED pulsando, ~2 s).
# Pra estender a janela indefinidamente: segure BOOT (o LED responde com
# piscadas rápidas).
make program-dfu               # o APP_TYPE já aponta pro endereço da QSPI
./cotuca_serial.py --send board list mods --gap 1.3
```

**O que mudou em relação ao fluxo antigo:**

| | Antes (`BOOT_NONE`) | Agora (`BOOT_QSPI`) |
|---|---|---|
| Entrar em modo de gravação | BOOT + RESET (DFU da STM) | só RESET, e gravar na janela de ~2 s |
| Onde o app vai parar | flash interna, 128 KB | QSPI, 8 MB |
| Teto de tamanho | 128 KB | 7 936 KB |
| USB PID no DFU | `0483:df11` (STM) | `0483:df11` (bootloader da Daisy) |
| Tempo de boot | imediato | +2 s de janela de graça |

`make program-dfu` sempre termina com `Error 74` no `leave request`: a placa
pula pro app e para de responder ao último `get_status`. Se apareceu
`File downloaded successfully`, gravou.

### Voltar pra flash interna

```bash
make clean && make APP_TYPE=BOOT_NONE
# BOOT + RESET + solta BOOT
make APP_TYPE=BOOT_NONE program-dfu
```

Isso **apaga o bootloader** (o app volta a ocupar a flash interna) — pra
voltar pra QSPI é preciso refazer o `make program-boot`.

---

## T1 — LED pisca 1×/seg · ✅ PASSOU

**Procedimento:** gravar, olhar o LED USER.
**Esperado:** ~1 Hz. Pisca **rápido** (10 Hz) = `CheckRequirements()` reprovou a placa.

**Resultado:** piscando devagar. Confirmado também pelo serial: o boot imprime
`pronto.` em vez de entrar no laço de pisca rápido, ou seja a Seed 2 atende
`needs_audio_in`, `min_knobs 4` e a RAM exigidos pelo manifest.
Na v0.3.0 o LED virou heartbeat contado (toggle a cada 500 ticks de ~1 ms),
e o ritmo de 1 Hz se manteve.

## T2 — painel falso via serial 115200 · ✅ PASSOU

**Procedimento:** `board`, `list`, `k1 0.5`, `list`.

```
> board      < Daisy Seed | knobs:5 | in:sim | 48000Hz/blk48
> list       <  0 level      norm=0.70 real=0.70
             <  1 tone_freq  norm=0.40 real=848.00
             <  2 tone_mix   norm=0.00 real=0.00
             <  3 spare      norm=0.00 real=0.00
             <  4 src_mix    norm=0.00 real=0.00
> k5 0.5     < knob 5 -> 0.50   (src_mix 0.00 → 0.50)
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
> tone off    < tone OFF
```

Conversão pra afinação alvo, com `tone_freq` em `[80, 2000]`:
`norm = (Hz − 80) / 1920`. Logo **440 Hz = 0.1875** e 880 Hz = 0.4167.

**Lado do ouvido — passou.** Tom contínuo e limpo no reteste com 3 s por altura
(848 → 440 → 1616 → 440 Hz), sem estalo, dropout ou distorção. A primeira
passada tinha soado como um bip curto; foi observação parcial, não defeito.

> **`k5` / SRC MIX passou a existir na v0.3.0.** `list` mostra `src_mix` no
> índice 4 e `k5 0.5` move de 0.00 → 0.50. O crossfade `IN_L`↔`IN_R` está no
> callback, mas só será audível com as duas entradas ligadas (T4).

## T4 — piezo / exciter / toggle de roteamento · ⏸️ BLOQUEADO

Espera o buffer de alta impedância, o TPA3116 e o painel. Nada a rodar.

## T5 — módulo RNBO "delay" · ⏳ PENDENTE DE GRAVAÇÃO

**O critério de aceite do primeiro módulo.** Compilar não é evidência: o
patcher pode estar montado e mudo. Só o ouvido fecha a conta.

**Pré-requisito:** bootloader gravado (`make program-boot`, uma vez) e
firmware v0.4.0 na QSPI (`make program-dfu`).

### T5a — descoberta dos parâmetros (serial, sem áudio)

```bash
./cotuca_serial.py --send board list mods --gap 1.5
```

**Esperado:**

- No **boot**: `modulo "delay": 2 params, 2 ligados`, e **nenhum**
  `AVISO: param … nao existe no modulo`.
- `list` mostra **7** parâmetros — os 5 de sistema mais `dly_time` e
  `dly_mix`, ambos `norm=0.50`.
- `mods` mostra o módulo, os dois params **descobertos por nome**
  (`[0] dly_time`, `[1] dly_mix`), as duas ligações do manifest com índice
  `≥ 0`, e o extrato de memória com **`0 falhas`**.
- O boot imprime `SDRAM via AllocBig: N de 67108864 bytes` com **N > 0** —
  é a linha de delay. Antes do módulo esse número era zero.

**Falhou se:** índice `-1` em alguma ligação (nome divergente do
`description.json`), `falhas > 0` no extrato (SDRAM esgotada), ou
`ERRO: modulo "delay" falhou no Init` (o firmware cai pra passthrough e o
áudio sai seco, o que **não** é o teste passando).

### T5b — confirmação sonora

Cabo no **pino 18 (AUDIO OUT L)** e **pino 20 (AGND)**.

| # | Comando | Esperado no ouvido |
|---|---|---|
| 1 | `tone on` · `k2 0.7` | senóide **ecoando em trem que decai** — o feedback interno do patch é 0.4, então cada repetição sai a ~40% da anterior e a cauda morre sozinha em alguns ecos |
| 2 | `k1 0.0` → `k1 0.5` → `k1 1.0` | o **tempo entre os ecos varre audivelmente**, de curto a longo. Varrer com o eco tocando dá o glissando típico de delay time modulado |
| 3 | `k2 0.0` | som **seca**: só a senóide direta, sem cauda |
| 4 | `tone off` | silêncio depois da cauda escoar |

`k1` → `dly_time`, `k2` → `dly_mix`, `k3` → `level` (ver `manifest.h`).

**Falhou se:** eco único em vez de trem (feedback não chegou), cauda que
**cresce** em vez de decair (feedback > 1, perigo pro exciter — corte o
volume), estalos a cada mudança de `k1` (interpolação do delay), ou `k2`
sem efeito nenhum (parâmetro não ligado — volte pro T5a).

> **A confirmação sonora ainda não foi feita.** O firmware v0.4.0 compilou
> limpo e a migração QSPI está pronta, mas a placa segue com o v0.3.0 (sem
> módulo) porque gravar exige o botão BOOT. Enquanto o T5 não rodar, o
> módulo RNBO é **código que compila**, não módulo que funciona.

---

## Defeitos conhecidos

| # | Sintoma | Estado |
|---|---|---|
| 1 | `list` travava a placa (CDC calada até o reset) | ✅ corrigido no HAL v0.2.0 |
| 2 | Todo `%f` saía vazio — `board` sem o `48000`, `list` com `norm= real=` | ✅ corrigido no Makefile |
| 3 | Linha de log saía picotada e grudada na seguinte | ✅ corrigido no HAL v0.3.1 |
| 4 | Comando serial demorava até 0,5 s pra responder | ✅ corrigido no main.cpp v0.3.0 |

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

**#3 — buffer único do Logger.** O `Logger` da libDaisy formata tudo num
`static char tx_buff_[128]` **compartilhado** e transmite sem esperar: a segunda
linha sobrescrevia o buffer enquanto o USB ainda lia a primeira, e a saída vinha
truncada e grudada (`SDRAM via AllocBig:$$ 0 level...`), às vezes sumindo.

Não era ritmo — por isso um `System::Delay(2)` por linha **não** curou. O
`hal::Log` agora ignora o Logger e transmite direto por
`hw.usb_handle.TransmitInternal()`, com **dois buffers alternados** e retry até
o CDC aceitar. Como `Log` só retorna após o aceite, ao escrever em B o buffer A
já foi entregue ao hardware, e o transmit de B só é aceito quando A terminou:
nenhum buffer é tocado enquanto o DMA o lê. Retry tem teto de ~50 ms, pra placa
sem host lendo não travar.

**Verificado:** 21 linhas em rajada com 50 ms entre comandos, todas íntegras.

**#4 — latência.** Com o despacho por polling, o `DelayMs(500)` do loop virava o
teto de resposta a comando. O loop agora gira a ~1 ms e o LED virou heartbeat
contado (toggle a cada 500 ticks), mantendo 1 Hz. Verificado com comandos a
0,35 s de intervalo: resposta imediata.

---

## Nota de arquitetura

**O `AllocBig()` deixou de ser descartado pelo linker.** Até a v0.3.0 a SDRAM
aparecia como `0 B / 64 MB` porque nada chamava a função e o `--gc-sections`
comia o pool inteiro. Com o módulo RNBO alocando a linha de delay, os 64 MB
voltaram de uma vez: o mapa agora mostra `SDRAM: 64 MB / 64 MB = 100%`.

Isso é **esperado, não estouro**. O `sdram_pool` em `hal_daisy_seed.cpp` reserva
a região inteira de propósito, e reparte por dentro — os 100% são o pool, não o
consumo. O consumo real é o que o boot imprime:

```
SDRAM via AllocBig: N de 67108864 bytes
```

A consequência prática é a de sempre: **não sobra byte pra mais nada em
`.sdram_bss`**. Todo acesso à SDRAM tem que passar pelo portão. Regra completa
em [BOARDS.md](BOARDS.md).

### Quanto o RNBO deve pedir (previsão, ainda não medida)

Calculado lendo o export, não medido em placa — o T5 é que confirma:

| Origem | Conta | Bytes |
|---|---|---|
| Linha de delay (`delaytilde_01_del`) | `evaluateSizeExpr` devolve **96 000** samples (2 s @ 48 kHz); o RNBO arredonda pra potência de dois → **131 072** samples × 4 B (`RNBO_USE_FLOAT32`) | 524 288 |
| Header do nosso alocador | 8 B por bloco | 8 |
| **Total esperado em `AllocBigUsed()`** | | **≈ 524 296** |

Os demais buffers do patcher são do tamanho do bloco (48 samples = 192 B cada) e
**cabem no pool de 32 KB em SRAM** — não devem aparecer na conta da SDRAM.

Se o número medido no boot destoar muito disso, alguma coisa está alocando por
fora, e é exatamente pra isso que o extrato existe.

> **Nota sobre `RNBO_USE_FLOAT32`:** sem essa macro, `SampleValue` seria
> `double` e a mesma linha de delay pediria **1 MB** em vez de 512 KB.
