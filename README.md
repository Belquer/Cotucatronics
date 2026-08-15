# Cotucatronics OS

Firmware modular para **instrumentos-escultura sonora**. A ideia central é que
o mesmo cérebro rode qualquer escultura: o código de instrumento é agnóstico de
placa, e trocar de hardware significa escrever um arquivo novo — não reescrever
o sistema.

Projeto colaborativo. Estado atual: **v0.3.0**, validado em bancada na
Daisy Seed 2 (T1–T3 do [TESTES.md](src/TESTES.md) passando).

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
| [src/hal.h](src/hal.h) | **v0.2.0** | O contrato. A fronteira de independência de fornecedor |
| [src/hal_daisy_seed.cpp](src/hal_daisy_seed.cpp) | **v0.3.0** | Único arquivo que pode incluir `daisy_seed.h` |
| [src/module.h](src/module.h) | **v0.1.0** | Interface de módulo DSP. Isola o motor (RNBO) |
| [src/main.cpp](src/main.cpp) | **v0.3.0** | Loop, callback de áudio, painel falso via serial |
| [src/params.h](src/params.h) | **v0.2.0** | `ParamTable` + `Macro`. Sem alocação dinâmica |
| [src/manifest.h](src/manifest.h) | **v0.2.0** | Instrumento "bancada": passthrough + tom de teste |
| [src/Makefile](src/Makefile) | **v0.1.0** | Build por placa. Sem DaisySP |
| [src/TESTES.md](src/TESTES.md) | **v0.1.0** | Roteiro T1–T4 e defeitos conhecidos |
| [src/BOARDS.md](src/BOARDS.md) | **v0.1.0** | Placas suportadas e a regra da SDRAM |
| [src/cotuca_serial.py](src/cotuca_serial.py) | **v0.1.0** | Monitor serial, sem dependências |
| [src/monitor.command](src/monitor.command) | **v0.1.0** | Wrapper duplo-clicável do monitor |

## Ambiente que funciona

| Item | Versão |
|---|---|
| Placa | Daisy Seed 2 (DFM) |
| Toolchain | ARM GNU Toolchain 15.3.rel1 (`arm-none-eabi-gcc 15.3.1`) |
| libDaisy | `~/dev/libDaisy` |
| dfu-util | 0.11 |

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

Ponha a Seed em bootloader — **segura BOOT, toca RESET, solta BOOT** — e grave:

```bash
cd src && make program-dfu
```

`make program-dfu` **sempre** termina com `Error 74` no `leave request`: a placa
pula pro app e para de responder ao último `get_status`. Se apareceu
`File downloaded successfully`, gravou.

## Testar

```bash
cd src && ./cotuca_serial.py --send board list "k1 0.5" --gap 1.3
```

Ou duplo-clique em `src/monitor.command`.

Comandos do firmware: `board` · `list` · `k1..k5 <0-1>` · `tone on` · `tone off`

`k5` é o **SRC MIX**: crossfade linear entre `IN_L` (piezo, `0.0`) e `IN_R`
(jack de linha, `1.0`), aplicado antes de qualquer processamento. Enquanto o
painel de 5 pots não existe, o `k4` também aponta pra ele — por isso
`req.min_knobs` segue em 4: uma placa de 4 knobs ainda alcança o SRC MIX.

Como os knobs físicos ainda não estão ligados, `ReadKnob()` devolve `-1` e o
painel vive no serial. Conversão pra uma afinação alvo, com `tone_freq` em
`[80, 2000]`: `norm = (Hz − 80) / 1920` — logo **440 Hz = `k2 0.1875`**.

## Gotchas

- **Nada de `hal::Log()` dentro do callback de áudio.** Ele bloqueia por
  milissegundos.
- **`--specs=nano.specs` engole `%f` calado.** O Makefile já força
  `-u _printf_float`; se remover, metade da saída do painel some sem aviso.
- **O callback de RX do USB roda em ISR.** Nunca chame `Log()` dali — foi
  exatamente isso que travava a placa antes da v0.2.1. Enfileire e despache no
  `PollSerial()`.
- **`sdram_pool` pede a região SDRAM inteira.** Hoje o `--gc-sections` descarta
  tudo porque nada chama `AllocBig`. No primeiro instrumento que chamar, os
  64 MB voltam de uma vez e não sobra espaço pra mais nada em `.sdram_bss`.
- **`AllocBig()` é o único acesso à SDRAM.** Declarar `DSY_SDRAM_BSS` fora de
  `hal_daisy_seed.cpp` é proibido — regra completa em [BOARDS.md](src/BOARDS.md).
  O boot imprime o extrato (`SDRAM via AllocBig: N de M bytes`); se um módulo
  alocar por fora, o número não fecha.

## Defeitos em aberto

| Sintoma | Onde |
|---|---|
| Linha de log sai picotada e grudada na seguinte; às vezes some | `tx_buff_[128]` estático e compartilhado do Logger da libDaisy |

Diagnóstico completo de cada um no [TESTES.md](src/TESTES.md).

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

- Módulos DSP vindos de exports C++ do **RNBO**, encaixados onde hoje vive o
  oscilador de teste. A interface já existe ([module.h](src/module.h)); falta o
  `module_rnbo.cpp`, que espera o export em `src/modules/<nome>/rnbo_export/`.
- Stack anti-feedback: frequency shifter Hilbert, gain governor, notch bank
  adaptativo, limiter de segurança.
- Segunda placa: **Teensy 4.1**, via um `hal_teensy41.cpp` novo — sem tocar em
  mais nada.
