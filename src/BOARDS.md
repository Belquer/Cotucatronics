# BOARDS.md — placas suportadas e regras de recurso

**Documento v0.2.0**

Como portar o Cotucatronics OS pra uma placa nova, e as regras de recurso que
valem em todas elas.

---

## Placas

| Board | `BOARD=` | Arquivo | Estado |
|---|---|---|---|
| Daisy Seed 2 (DFM) | `daisy_seed` | `hal_daisy_seed.cpp` v0.3.1 | ✅ validada em bancada |
| Teensy 4.1 | `teensy41` | `hal_teensy41.cpp` | 🔜 planejada |

### Daisy Seed 2 — `BoardInfo`

| Campo | Valor |
|---|---|
| `name` | `"Daisy Seed"` |
| `num_knobs` | 5 (4 de caráter + SRC MIX) |
| `has_audio_in` | `true` |
| `sample_rate` | 48000 Hz |
| `block_size` | 48 (~1 ms) |
| `big_ram_bytes` | 67108864 (64 MB SDRAM) |

A detecção de Seed 1.1 vs Seed 2 DFM é automática dentro do `hw.Init()` da
libDaisy (`CheckBoardVersion()`), e o codec PCM3060 do Seed 2 é tratado lá
dentro. **Nada disso vaza pro nosso código.**

> ### Daisy: o app roda da QSPI de 8 MB, via bootloader
>
> Desde a v0.4.0 o firmware é compilado com **`APP_TYPE=BOOT_QSPI`**: o app vive
> na **QSPI flash de 8 MB** e **os 128 KB de flash interna são do bootloader**
> da Electrosmith. Nenhum byte do nosso código mora na flash interna — o mapa de
> memória mostra `FLASH: 0 B`, e isso está certo.
>
> **Por quê:** o primeiro módulo RNBO sozinho levou o firmware a 94,14% dos
> 128 KB internos. O segundo não caberia. Na QSPI o mesmo firmware ocupa 1,52%
> de 7 936 KB.
>
> **O que isso custa:**
> - o Cortex-M7 executa da QSPI mais devagar que da flash interna — mitigado
>   pelo I-cache, mas é real, e vale medir se um módulo pesado começar a estourar
>   o prazo do bloco de áudio;
> - o boot ganha ~2 s de janela de graça do bootloader;
> - gravar tem um passo a mais, **uma vez por placa** (`make program-boot`).
>
> **Não é regra do projeto, é configuração da Daisy.** Uma placa nova não herda
> nada disso: o `APP_TYPE` é do `Makefile` e do ecossistema Electrosmith. O
> Teensy 4.1 tem 8 MB de flash direto, sem bootloader intermediário.
>
> Fluxo de gravação completo em [TESTES.md](TESTES.md).

---

## Regra da RAM grande

> **`AllocBig()` é o único acesso à SDRAM em todo o projeto.**

É a regra que faz a fronteira HAL valer também pra memória, não só pra API.

### Proibido

Declarar `DSY_SDRAM_BSS` — ou o macro de seção equivalente de qualquer outra
placa — **fora de `hal_<board>.cpp`**. Isso inclui módulos, exports do RNBO,
buffers de freeze, delays longos e notch banks.

```cpp
// ERRADO: prende o módulo à Daisy e fura a fronteira
static float DSY_SDRAM_BSS buffer[1024 * 1024];
```

```cpp
// CERTO: portão único, e a checagem de nullptr é obrigatória
float* buffer = (float*)cotuca::hal::AllocBig(1024 * 1024 * sizeof(float));
if(!buffer)
    return false; // placa sem RAM grande, ou pool esgotado
```

### Por quê

1. **Portabilidade.** Um módulo que declara `DSY_SDRAM_BSS` não compila no
   Teensy. Um que chama `AllocBig()` compila em qualquer lugar e degrada com
   graça: a placa sem RAM grande devolve `nullptr` e o módulo se recusa a
   carregar, com erro claro no boot.
2. **Contabilidade.** Seções declaradas espalhadas pelo código são invisíveis
   até o linker estourar. Pelo portão, o total é sempre conhecido.
3. **Determinismo.** O alocador é bump — sem free, sem fragmentação, sem
   surpresa em tempo de execução.

### Contrato de uso

- **Só no boot.** `AllocBig()` **nunca** no callback de áudio.
- **Checar `nullptr` sempre.** Quem chama decide o que fazer sem a RAM.
- **Sem `free`.** É bump allocator: o que foi entregue não volta. Declare o
  que precisa uma vez.
- **Declare em `Requirements`.** O manifest de cada instrumento anuncia
  `big_ram_bytes`, conferido no boot contra `hal::Info().big_ram_bytes`.
  Placa insuficiente = LED rápido + log, em vez de bug no palco.

### Extrato no boot

Todo boot imprime quanto saiu pelo portão:

```
SDRAM via AllocBig: 524296 de 67108864 bytes
```

Se um módulo alocar por fora, **esse número não fecha** — é o detector de
violação da regra.

O módulo RNBO tem seu próprio extrato, mais detalhado, no comando `mods`: ele
serve de um pool de 32 KB em SRAM primeiro e só cai pra `AllocBig()` no que não
couber. Receita completa em [modules/README.md](modules/README.md).

### Armadilha conhecida — agora ativa

`sdram_pool` em `hal_daisy_seed.cpp` reserva a região SDRAM **inteira**
(64 MB). Até a v0.3.0 o `--gc-sections` descartava o pool porque nada chamava
`AllocBig()`, e o mapa mostrava `SDRAM: 0 B`. **A partir da v0.4.0 o módulo
RNBO chama, e os 64 MB voltaram de uma vez:** o mapa mostra `SDRAM: 100%`.

Isso é o pool, não o consumo — mas a consequência é real: **não sobra byte pra
mais nada em `.sdram_bss`**. Toda alocação tem que passar pelo portão. Como
passa, é seguro — mas só enquanto a regra for respeitada.

---

## Portar pra uma placa nova

1. Escreva `hal_<board>.cpp` implementando **todo** o contrato de `hal.h`:
   `Init`, `Info`, `StartAudio`, `ReadKnob`, `ReadButton`, `SetLed`, `Log`,
   `SetSerialLineCallback`, `PollSerial`, `DelayMs`, `AllocBig`, `AllocBigUsed`.
2. Preencha `BoardInfo` com a verdade da placa. Sem RAM grande?
   `big_ram_bytes = 0` e `AllocBig()` devolvendo `nullptr` — não minta, o
   boot depende disso pra recusar instrumentos que não cabem.
3. `make BOARD=<board>`. O Makefile acha `hal_<board>.cpp` sozinho e lista as
   placas disponíveis se errar o nome.
4. Rode o [TESTES.md](TESTES.md). T1–T3 não dependem de placa.

### Armadilhas por implementar

- **`Log()` não pode ser chamado de ISR** se transmitir pelo mesmo periférico
  que gera a interrupção. Na Daisy isso travava a placa inteira. Se o RX da
  sua placa for por interrupção, **enfileire e despache no `PollSerial()`**.
- **`ReadKnob()` devolve `-1`** quando não há pot físico ligado. O loop
  principal trata isso como "sem hardware, obedeça o serial". Não devolva 0.
