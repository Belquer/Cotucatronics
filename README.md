# Cotucatronics OS

Firmware modular para **instrumentos-escultura sonora**. A ideia central é que
o mesmo cérebro rode qualquer escultura: o código de instrumento é agnóstico de
placa, e trocar de hardware significa escrever um arquivo novo — não reescrever
o sistema.

Projeto colaborativo. Estado atual: **v0.81.0** — delay + reverb + freeze granular e a **suíte de cinco efeitos residentes** (drive, phaser, warble, detune, shimmer), bloco 48, MIDI certificado sob carga (T16c, 2026-08-18); vocoder aposentado do build,
**stack anti-feedback completo em software** (gate, governor, limiter e
shifter SSB, os quatro em C++ do shell), **entrada MIDI** por CC,
**painel v2** (5 pots + 2 botões, com randomizador e hold) e **caixa-preta**
em BACKUP_SRAM.
**T1, T2, T3, T5, T6, T7 e T8 do [TESTES.md](src/TESTES.md) passaram em bancada**;
T4 espera hardware.

> ### O que ainda NÃO foi validado em placa
>
> - **MIDI (T10b).** Parser e mapa passam no banco nativo — 16 verificações —
>   mas a UART (baud rate, pino, DMA) só o loopback com jumper prova.
> - **Painel v2 (T11).** Firmware pronto e simulável por serial (`b1`, `b2`);
>   os pots e botões ainda não foram soldados.
> - **Vocoder no build.** Ligado em 2026-08-16 com export novo; ainda não
>   ouvido em placa nesta configuração.

> ### Estado versionado — VOCODER LIGADO
>
> **480 MHz · bloco 256 (5,3 ms de latência) · delay no slot Instrument ·
> vocoder no Fx2** · app na QSPI
>
> **O bloco 256 é exigência do vocoder, não preferência:** ele pede
> ~3 520 µs de pico e o prazo do bloco 48 é 1 000 µs. `Makefile` e
> `manifest.h` mudam **juntos** — módulo no build e `block_size` — e o
> comentário está nos dois. Pra voltar ao estado instrumento (bloco 48,
> só o delay), a receita está em [BOARDS.md](src/BOARDS.md).

---

## Arquitetura

Cinco regras que não se negociam:

1. **Fronteira HAL.** Nenhum `#include` de fabricante fora de
   `hal_<board>.cpp`. `main.cpp`, params, macros e manifests não sabem em que
   placa estão rodando. A placa se escolhe com `make BOARD=daisy_seed`.
2. **Parâmetros normalizados.** Tudo é `0.0–1.0` numa tabela central; o manifest
   é quem traduz pra Hz, segundos, dB.
3. **Macros.** Um knob físico pode mover vários parâmetros, com peso e inversão.
4. **Manifest por instrumento.** É o único arquivo que muda de escultura pra
   escultura (junto com o futuro módulo RNBO). Declara `Requirements` — RAM
   grande, audio in, nº de knobs — conferidos no boot: placa insuficiente dá
   erro claro em vez de bug misterioso no palco.
5. **`AllocBig()`.** Abstrai a SDRAM de 64 MB. Alocador bump, só no boot, nunca
   no callback de áudio. Placa sem RAM grande devolve `nullptr` — quem chama
   **precisa** checar.

## Mapa de arquivos

| Arquivo | Versão | O que é |
|---|---|---|
| [src/hal.h](src/hal.h) | **v0.17.0** | O contrato. A fronteira de independência de fornecedor |
| [src/hal_daisy_seed.cpp](src/hal_daisy_seed.cpp) | **v0.16.0** | Único arquivo que pode incluir `daisy_seed.h` |
| [src/module.h](src/module.h) | **v0.4.0** | Interface, **slots fixos** e registro de módulos |
| [src/reverb.h](src/reverb.h) | **v0.2.0** | Freeverb + passa-alta. C++ puro, buffers vêm de fora |
| [src/freeze.h](src/freeze.h) | **v0.3.1** | Freeze granular: núcleo de 500 ms, 2 grãos sorteados, Hann. C++ puro |
| [src/pitch.h](src/pitch.h) | **v0.5.0** | Motor de pitch compartilhado: 1 buffer, até 4 vozes. **Anti-alias automático na escrita** (Butterworth 6ª ordem, custo zero se nenhuma voz sobe). C++ puro |
| [src/shimmer.h](src/shimmer.h) | **v0.3.0** | Shimmer: reverb + pitch num laço. Realimenta o Freeverb existente. **Tom = corte + sino de presença em 3 kHz** |
| [src/drive.h](src/drive.h) | **v0.2.0** | Drive: waveshaper tanh normalizado como cola. Zero memória |
| [src/phaser.h](src/phaser.h) | **v0.3.0** | Phaser: N allpass varridos por LFO triangular. Zero buffer |
| [src/warble.h](src/warble.h) | **v0.1.0** | Warble: deriva de fita por delay curto modulado |
| [src/detune.h](src/detune.h) | **v0.1.0** | Detune: 2 vozes micro-desafinadas + largura estéreo |
| [src/effect_slot.h](src/effect_slot.h) | **v0.3.0** | Moldura: fade assimétrico, custo-zero desligado, cronometragem |
| [src/pitch_spectrum.py](src/pitch_spectrum.py) | **v0.1.0** | Os N maiores picos do espectro — pico dominante mente com laterais fortes |
| [src/dynamics.h](src/dynamics.h) | **v0.4.0** | Gate, governor, limiter e **shifter SSB**. C++ puro |
| [src/midi.h](src/midi.h) | **v0.4.0** | Máquina de estados MIDI + mapa CC (teto 48). C++ puro, board-agnostic |
| [src/test_dynamics.cpp](src/test_dynamics.cpp) | **v0.1.0** | Banco de teste nativo — roda no Mac, sem placa |
| [src/test_midi.cpp](src/test_midi.cpp) | **v0.1.1** | Banco nativo do parser MIDI e da via CC→ParamTable |
| [src/test.command](src/test.command) | **v0.2.0** | Wrapper duplo-clicável dos dois bancos de teste |
| [src/module_rnbo.h](src/module_rnbo.h) | **v0.2.0** | Declaração pública do módulo RNBO. Não inclui nada do RNBO |
| [src/module_rnbo_alloc.h](src/module_rnbo_alloc.h) | **v0.1.0** | Alocador híbrido dos módulos RNBO |
| [src/module_rnbo_delay.cpp](src/module_rnbo_delay.cpp) | — | Delay no Fx1. Inclui `RNBO.h` |
| [src/module_rnbo_freeze.cpp](src/module_rnbo_freeze.cpp) | — | Freeze. Reprovado na etapa 5 do IFÁ10000 (7 349 µs) |
| [src/module_rnbo_vocoder.cpp](src/module_rnbo_vocoder.cpp) | **v0.6.0** | Vocoder. **FORA do build desde 18/08** — decisão de ouvido do dB, revivível |
| [src/main.cpp](src/main.cpp) | **v0.81.0** | Loop, callback de áudio, painel falso via serial. `board` declara as tabelas, `pk` declara a entrada crua por canal |
| [src/suave.h](src/suave.h) | **v0.1.0** | Suavizador de parâmetro de um polo. C++ puro — a quarta ocorrência da forma virando única |
| [src/params.h](src/params.h) | **v0.8.0** | `ParamTable` + `Macro` (teto 64). Sem alocação dinâmica |
| [src/manifest.h](src/manifest.h) | **v0.55.1** | Instrumento aprovado: delay + reverb + suíte de cinco efeitos + I/O, **56 params / 39 CCs** |
| [src/Makefile](src/Makefile) | **v0.2.0** | Build por placa. `APP_TYPE=BOOT_QSPI`. Sem DaisySP |
| [src/modules/README.md](src/modules/README.md) | **v0.4.0** | **Manual de embarque de módulos RNBO — o template** |
| [.claude/skills/](.claude/skills/) | **vivo** | **O braço executável do IFÁ10000** — `ifa10000`, `medicao-limpa`, `instrumentacao`, `bug-de-ordem`, `edicao-segura` |
| [docs/FEATURES.md](docs/FEATURES.md) | **vivo** | **A CAIXA ÚNICA** — toda feature vigente, e as aposentadas no arquivo |
| [docs/CTOS.md](docs/CTOS.md) | **v0.3.0** | A base à prova de falha, a fronteira CTOS/Efeito e a suíte de efeitos |
| [docs/IFA10000.md](docs/IFA10000.md) | **vivo** | **Certificação de efeitos** — as 10 etapas que todo efeito passa |
| [docs/STATUS.md](docs/STATUS.md) | **vivo** | **Onde estamos** — estado da placa, defeitos abertos, próximos passos |
| [docs/battle_scars.md](docs/battle_scars.md) | **vivo** | **Aprendizados e metodologia** — as cicatrizes, pra não pagar duas vezes |
| [src/TESTES.md](src/TESTES.md) | **v0.24.0** | Roteiro T1–T12, fluxo de gravação e defeitos conhecidos |
| [src/BOARDS.md](src/BOARDS.md) | **v0.12.0** | Placas suportadas, a regra da SDRAM e a nota do bootloader |
| [src/cotuca_serial.py](src/cotuca_serial.py) | **v0.1.0** | Monitor serial, sem dependências |
| [src/monitor.command](src/monitor.command) | **v0.1.0** | Wrapper duplo-clicável do monitor |
| [src/cotuca_measure.py](src/cotuca_measure.py) | **v0.1.0** | Mede a frequência de saída por FFT |
| [src/measure.command](src/measure.command) | **v0.1.0** | Wrapper duplo-clicável do medidor |

## Ambiente que funciona

| Item | Versão |
|---|---|
| Placa | Daisy Seed 2 (DFM) |
| Toolchain | ARM GNU Toolchain 15.3.rel1 (`arm-none-eabi-gcc 15.3.1`) |
| libDaisy | `~/dev/libDaisy` |
| dfu-util | 0.11 |
| Bootloader | Electrosmith `dsy_bootloader_v6_4-intdfu-2000ms.bin` (vem com a libDaisy) |
| RNBO / Max | 1.4.3 / 9.1.4 |

> **Não use o `arm-none-eabi-gcc` do Homebrew.** Ele vem sem newlib e a libDaisy
> morre em `stdint.h: No such file or directory`. Use o cask (pede senha de admin):
> ```bash
> brew install --cask gcc-arm-embedded
> brew install dfu-util
> ```

## Build e flash

```bash
git clone --recurse-submodules https://github.com/electro-smith/libDaisy ~/dev/libDaisy
make -C ~/dev/libDaisy
```

```bash
cd src && make BOARD=daisy_seed
```

> ⚠️ **A manobra de gravar o dia a dia é SÓ O RESET.** `BOOT + RESET` entra
> no DFU da **ST**, que só enxerga a flash interna — e o app mora na QSPI.
> O sintoma é `dfu-util: Last page at 0x9007760b is not writeable`
> (`Error 64`). `BOOT + RESET` serve pro `make program-boot`, uma vez por
> placa. Gravação bem-sucedida diz `File downloaded successfully` e termina
> com `Error 74` no `leave request` — isso é normal.

### O app roda da QSPI, via bootloader

Desde a v0.4.0 o firmware é compilado com `APP_TYPE=BOOT_QSPI`: **o app vive nos
8 MB da QSPI e os 128 KB de flash interna são do bootloader** da Electrosmith. O
motivo é direto — com um único módulo RNBO o firmware já ocupava **94,14%** dos
128 KB internos; o segundo módulo não caberia. Na QSPI o mesmo firmware ocupa
**1,52%** de 7 936 KB.

**Uma vez por placa**, grave o bootloader. Ponha a Seed no DFU da STM —
**segura BOOT, toca RESET, solta BOOT** — e:

```bash
cd src && make program-boot
```

**No dia a dia, NÃO toque em nada.** A placa entra no bootloader por comando,
com timeout infinito:

```bash
dfu go            # pela serial — a placa reinicia no DFU e ESPERA
cd src && make program-dfu
```

Isso trocou gravações de 119, 66 e 47 tentativas (disputando a janela de ~2 s
do RESET físico) por **uma**. Ver a skill `bancada-firmware`.

**Se precisar do caminho manual** (placa travada, firmware sem o comando):
só toque **RESET** e grave durante a janela de graça do
bootloader (LED pulsando, ~2 s; segurar BOOT estende a janela):

```bash
cd src && make program-dfu
```

`make program-dfu` **sempre** termina com `Error 74` no `leave request`: a placa
pula pro app e para de responder ao último `get_status`. Se apareceu
`File downloaded successfully`, gravou.

Pra voltar pra flash interna: `make clean && make APP_TYPE=BOOT_NONE` e gravar
pelo DFU da STM — mas isso apaga o bootloader.

## Testar

```bash
cd src && ./cotuca_serial.py --send board list "k1 0.5" --gap 1.3
```

Ou duplo-clique em `src/monitor.command`.

Comandos do firmware: `board` · `list` · `mods` · `cpu` · `dyn` · `probe` ·
`bb` · `midi` · `miditest` · `midiloop` · `b1` · `b2` · `k1..k5 <0-1>` ·
`set <nome> <real>` · `tone on` · `tone off`

`b1` e `b2` **simulam os botões do painel pelo serial**, então o painel v2 é
testável antes de existir fisicamente: `b1` dispara o randomizador, `b2`
segura o parâmetro assinalado. `bb` lê a **caixa-preta** — memória em
BACKUP_SRAM que sobrevive ao RESET, pra medir o que só acontece depois que a
serial já morreu.

`set` alcança **qualquer** parâmetro pelo nome, **em unidade real** — Hz, ms, ×
— e recusa valor fora da faixa citando os limites. Com os knobs 1–3 no delay e
no level, `tone_freq` e `tone_mix` são **serial-only por design** — o tom de
teste é bancada, não performance:

```
set tone_freq 440        # Hz, direto
set gate_release 500     # ms
set bench_boost 2        # vezes
```

Knob e macro seguem falando normalizado (posição de curso é normalizada por
natureza); a unidade real é só na fronteira humana.

### Curva de parâmetro

`ParamTable::Add()` aceita um expoente opcional:

```
real = min + pow(norm, curve) * (max - min)
```

`curve = 1.0` é linear (default, com caminho rápido que pula o `pow`). O
`dly_time` usa **2.5**: metade do curso do knob cobre os primeiros ~18% da
faixa, que é onde moram os delays curtos.

**Curva é decisão de manifest, não de módulo.** É ergonomia — como o knob se
comporta na mão. O DSP recebe o valor já curvado e não sabe que houve curva; o
mesmo export do RNBO serve a esculturas com curvas diferentes.

`cpu` reporta a carga do bloco de áudio (média e pico). Com o delay RNBO
carregado: **5,7% média, 9,9% pico** a 48 kHz / bloco 48 — constante, porque a
linha de delay lê e escreve toda amostra independente do ajuste.

`mods` mostra o módulo DSP carregado, os parâmetros que ele expôs (descobertos
**por nome**, não por índice), as ligações do manifest e o extrato de memória do
alocador do RNBO.

### MIDI

Porta MIDI na **USART1 a 31250 baud** — TX em `D13` (pino 14), RX em `D14`
(pino 15). Guia de solda do WIDI, **com o aviso do cruzamento TX↔RX**, em
[BOARDS.md](src/BOARDS.md).

O mapa CC→parâmetro mora no manifest e começa assim:

| CC | Parâmetro | Por quê |
|---|---|---|
| 20 | `dly_time` | também no knob 1 |
| 21 | `dly_mix` | também no knob 2 |
| 22 | `level` | também no knob 3 |
| 23 | `src_mix` | também nos knobs 4/5 |
| 24 | `gov_ceiling` | **sem knob** — a borda de feedback só é alcançável por MIDI ou serial |

Canal configurável no manifest, **omni por default**. CCs 20–24 são a faixa
"undefined" da spec, então não colidem com mod wheel, volume ou sustain.

> **MIDI é um knob remoto, não um caminho novo.** O CC entrega um valor
> normalizado à `ParamTable` e para aí — exatamente onde o knob físico para.
> Curva do manifest, `Shaped()` e `Real()` acontecem depois, iguais pra
> qualquer origem. O banco nativo verifica isso: o mesmo gesto por CC e por
> knob dá bit a bit o mesmo valor.

**Prioridade entre fontes: última escrita vence.** Serial, MIDI e o futuro knob
físico são pares, sem hierarquia — o raciocínio inteiro está no topo do
[main.cpp](src/main.cpp).

`midi` mostra o estado da porta, os contadores do parser, o mapa e os últimos
CCs recebidos. É a primeira coisa a consultar no dia da solda: ele diz **em qual
etapa o sinal parou** — fio, parser, canal ou mapa.

`miditest <cc> <val>` injeta bytes na fila de RX sem passar pela UART (prova o
parser e o mapa); `midiloop <cc> <val>` manda pelo fio e precisa de um jumper
`D13→D14` pra voltar — é o único que prova baud rate e pino. Roteiro em
[T10](src/TESTES.md).

### Bancos de teste nativos

```bash
cd src && ./test.command
```

`dynamics.h` e `midi.h` são C++ puro e board-agnostic, então a matemática dos
blocos e a máquina de estados MIDI rodam no Mac — sem placa, sem flash. **Não
substituem teste em placa**: nenhum deles toca hardware, e nenhum prova a UART.

`k5` é o **SRC MIX**: crossfade linear entre `IN_L` (piezo, `0.0`) e `IN_R`
(jack de linha, `1.0`), aplicado antes de qualquer processamento. Enquanto o
painel de 5 pots não existe, o `k4` também aponta pra ele — por isso
`req.min_knobs` segue em 4: uma placa de 4 knobs ainda alcança o SRC MIX.

Como os knobs físicos ainda não estão ligados, `ReadKnob()` devolve `-1` e o
painel vive no serial. Conversão pra uma afinação alvo, com `tone_freq` em
`[80, 2000]`: `norm = (Hz − 80) / 1920` — logo **440 Hz = `k2 0.1875`**.

### Medir em vez de confiar no ouvido

Com o `OUT_L` numa entrada de áudio do Mac (aqui: MiniFuse 4, canal 2):

```bash
cd src && ./cotuca_measure.py --scan --expect 440
```

Reporta a frequência dominante por FFT com interpolação parabólica, o nível em
dBFS e o erro em cents. Ou duplo-clique em `src/measure.command`.

Medido na faixa inteira (80 / 440 / 880 / 1760 / 2000 Hz): erro constante de
**+0,4 a +0,5 cents**. Sendo constante em termos *relativos*, é diferença de
clock — a taxa real do SAI da Daisy é ~0,026% acima dos 48000 Hz que o
`AudioSampleRate()` reporta e que o oscilador usa como base. Não é erro de
código, e é inaudível (1 cent é ~1/100 de semitom).

## Gotchas

- **Nada de `hal::Log()` dentro do callback de áudio.** Ele bloqueia por
  milissegundos.
- **`--specs=nano.specs` engole `%f` calado.** O Makefile já força
  `-u _printf_float`; se remover, metade da saída do painel some sem aviso.
- **O callback de RX do USB roda em ISR.** Nunca chame `Log()` dali — foi
  exatamente isso que travava a placa antes da v0.2.1. Enfileire e despache no
  `PollSerial()`.
- **Não use o `Logger` da libDaisy.** O `tx_buff_[128]` dele é único e
  compartilhado: duas linhas seguidas se sobrescrevem em pleno voo. O
  `hal::Log` transmite por conta própria, com buffer duplo e retry.
- **`sdram_pool` pede a região SDRAM inteira, e agora aparece nos 100%.** Até a
  v0.3.0 o `--gc-sections` descartava o pool porque nada chamava `AllocBig`. O
  módulo RNBO chama, então os 64 MB voltaram de uma vez: o mapa mostra `SDRAM:
  100%`. **É o pool, não o consumo** — mas não sobra espaço pra mais nada em
  `.sdram_bss`.
- **`AllocBig()` é o único acesso à SDRAM.** Declarar `DSY_SDRAM_BSS` fora de
  `hal_daisy_seed.cpp` é proibido — regra completa em [BOARDS.md](src/BOARDS.md).
  O boot imprime o extrato (`SDRAM via AllocBig: N de M bytes`); se um módulo
  alocar por fora, o número não fecha.
- **`FLASH: 0 B` no mapa de memória está certo.** O app roda da QSPI; a flash
  interna é do bootloader. Não é build vazia.
- **As macros do RNBO têm que vir antes dos includes.** `RNBO_NOTHROW`,
  `RNBO_USE_FLOAT32` e `RNBO_USECUSTOMALLOCATOR` são lidas pelos headers —
  chegar depois é o mesmo que não existir, e a falha é silenciosa. Por isso o
  export do RNBO entra como `#include` de fonte dentro de `module_rnbo.cpp`, e
  **não** como unidade de compilação separada. Receita inteira em
  [src/modules/README.md](src/modules/README.md).
- **Todo export novo do RNBO precisa de um Class Name próprio.** O export do
  `delay` saiu com o padrão `rnbomatic`; um segundo export com o mesmo nome
  colide no link. Detalhes em [src/modules/README.md](src/modules/README.md).

## Defeitos em aberto

Nenhum defeito conhecido. Os quatro encontrados na bancada estão corrigidos —
diagnóstico de cada um no [TESTES.md](src/TESTES.md).

**Em aberto, porém: a v0.4.0 não foi validada em placa.** O módulo RNBO e a
migração QSPI compilam limpo, mas gravar exige o botão BOOT. Até o **T5** rodar,
o módulo é código que compila, não módulo que funciona.

## Signal path do sistema completo

Ainda não implementado — o hardware está a caminho. Registrado aqui pra não
bloquear decisões de arquitetura:

- **Entradas.** `IN_L` = piezo via buffer de alta impedância (circuito externo);
  `IN_R` = jack TS/TRS de linha. **Pot 5 (SRC MIX)** faz crossfade entre os dois.
- **Saídas.** `OUT_L` → amp TPA3116 → exciter no corpo da escultura;
  `OUT_R` → jack out. O roteamento (exciter / jack / ambos) é firmware, por
  toggle de 3 posições.
- **Jacks switched (NC)** servem de sensor de inserção via GPIO — não roteiam
  sinal.
- **Painel fixo.** 4 knobs de caráter + SRC MIX + encoder com push + 2 botões
  momentâneos + toggle + LED RGB.

## Próximos passos

- **Rodar o T5** — gravar o bootloader, gravar o v0.4.0 na QSPI e fazer a
  confirmação sonora do delay. É o que falta pra v0.4.0 deixar de ser teórica.
- **Class Name próprio no export do `delay`**, antes do segundo módulo existir.
- Segundo módulo RNBO, seguindo [src/modules/README.md](src/modules/README.md).
- Stack anti-feedback: frequency shifter Hilbert, gain governor, notch bank
  adaptativo, limiter de segurança.
- Segunda placa: **Teensy 4.1**, via um `hal_teensy41.cpp` novo — sem tocar em
  mais nada.
