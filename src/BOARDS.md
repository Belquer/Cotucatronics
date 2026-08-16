# BOARDS.md — placas suportadas e regras de recurso

**Documento v0.5.0**

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

## MIDI — guia de solda do WIDI

**A porta MIDI da Daisy é a USART1 a 31250 baud, 8N1.** Ligada no boot
por `hal::SetMidiByteCallback()`; placa que nunca chama não paga
periférico nenhum.

### Pinos

| Sinal | libDaisy | STM32 | Pino físico | Vai em quê no módulo |
|---|---|---|---|---|
| **TX** (a placa fala) | `seed::D13` | `PB6` | **14** | **RX** do WIDI |
| **RX** (a placa ouve) | `seed::D14` | `PB7` | **15** | **TX** do WIDI |
| 3V3 | — | — | ver serigrafia | VCC do WIDI |
| GND | — | — | qualquer DGND | GND do WIDI |

> ### ⚠️ É CRUZADO. Este é o erro que custa uma tarde.
>
> **TX do módulo → RX da placa (D14).** **RX do módulo → TX da placa
> (D13).** Ligar reto — TX no TX — não queima nada e não dá erro
> nenhum: simplesmente não chega byte. O sintoma é `bytes 0` no
> comando `midi`, idêntico ao de um módulo sem alimentação.
>
> **Se o `midi` mostrar `bytes 0`, desconfie do cruzamento antes de
> qualquer outra coisa.**

**Sobre os números de pino físico.** D13/D14 vêm direto do
`daisy_seed.h` da libDaisy (`PORTB,6` e `PORTB,7`) — esses são certos.
Os números 14 e 15 seguem da numeração do SOM, em que os pinos 1–15 são
D0–D14 (é o que faz o pino 18 ser AUDIO OUT L e o 20 ser AGND, como o
[TESTES.md](TESTES.md) já usava). **Confira contra o diagrama oficial
da Electrosmith antes de encostar o ferro** — errar o pino de
alimentação custa a placa, e D-número é a referência que não depende
dessa contagem.

Os pinos de **3V3 e GND não estão listados por número de propósito**:
o SOM tem mais de um GND, e chutar o pino de alimentação é a única
coisa aqui que estraga hardware. Localize na serigrafia.

**São os pinos default do `MidiUartTransport` da libDaisy.** Um cabo
montado pra qualquer exemplo de MIDI da Electrosmith serve aqui sem
alteração — foi por isso que foram escolhidos, e não por preferência.

### Por que DMA em modo listen, e não polling nem uma IRQ por byte

O fluxo MIDI é esporádico e em rajadas curtas. Polling ou desperdiça o
loop principal ou perde byte; uma interrupção por byte a 31250 baud é
troca de contexto à toa. `DmaListenStart` — DMA circular com interrupção
de IDLE — só incomoda a CPU quando uma rajada termina. É o mesmo
mecanismo que a própria libDaisy usa no `MidiUartTransport`.

> **O buffer do DMA tem que morar em seção acessível por DMA**
> (`DMA_BUFFER_MEM_SECTION`, que é `.sram1_bss`). Num buffer comum o
> DMA escreve na RAM e o núcleo lê o cache: os bytes chegam e o parser
> vê zeros. No mapa de memória ele aparece em `RAM_D2_DMA` — 256 bytes.

### O que é da placa e o que não é

O HAL entrega **bytes crus**, um a um. Não parseia. A máquina de estados
MIDI é board-agnostic e mora em [midi.h](midi.h), do lado do shell.

Isso não é purismo: "como os bytes chegam" muda de placa pra placa
(DMA, interrupção, polling), mas "o que eles significam" não muda nunca.
Um parser dentro do HAL teria que ser reescrito no Teensy — e reescrito
igual.

**O RX chega por interrupção e é despachado no loop principal**, via
`hal::PollMidi()`, exatamente como as linhas do serial. Mesma regra,
mesmo motivo: rodar o parser na ISR significa mexer em `params` e
possivelmente logar de dentro dela, que é o deadlock que matou o `list`
na v0.2.0.

---

## Estado versionado — INSTRUMENTO

> **Regra da casa: o repo versiona o INSTRUMENTO. Bancada é estado
> transitório.** O que está no `main` é o que deve estar na placa pra
> tocar; qualquer configuração de medição é receita documentada, não
> commit.

| | Valor | Onde muda |
|---|---|---|
| Clock | **480 MHz** | permanente, `hw.Init(true)` |
| Bloco | **48** — 1 000 µs, latência 1 ms | `Requirements::block_size` |
| Módulos | `delay` no slot instrument, **fx2 vazio** | `MODULE_SRC` no `Makefile` |
| `dly_time` / `dly_mix` | 0,40 / 0,50 | manifest |
| App | QSPI via bootloader | `APP_TYPE=BOOT_QSPI` |
| CPU medida | **188 µs de 1 000 — 18,8%** | |

### Como remontar a bancada do vocoder

Duas mudanças, nesta ordem:

```make
# Makefile
MODULE_SRC ?= module_rnbo_delay.cpp module_rnbo_vocoder.cpp
```

```cpp
// manifest.h, em Requirements
size_t block_size = 256;
```

O bloco 256 é **exigência do vocoder**, não preferência: ele pede 3 520 µs
de pico e o prazo do bloco 48 é 1 000 µs. Custa 5,3 ms de latência, que
num instrumento realimentado não é neutro.

Depois: gravar, `set fx2_on 1`, e o watchdog cuida do resto. A janela de
graça absorve a estreia; `probe` dá o perfil bloco a bloco; `cpu` dá o
regime em µs contra o prazo.

O **freeze** segue fora de qualquer regime: 7 349 µs de pico, não cabe
em prazo nenhum que valha a latência. `module_rnbo_freeze.cpp` e o export
em `modules/xxxfreeze/` continuam no repo pra quando o patch emagrecer.

### Nota sobre `-O3`

Fica até o orçamento apertar. Medido contra `-O2`: média **piorou** de
11,3% para 13,0%, pico praticamente igual. O app executa da QSPI e o
binário inflou 13% (260 → 295 KB) — mais código, mais falha de I-cache,
e em memória externa isso custa mais que o inlining rende.

> **Ao reverter, REMEDIR TUDO.** Todos os números deste documento e dos
> commits — 188 µs do instrumento, 3 520 do vocoder, 7 349 do freeze —
> têm `-O3` como base. Comparar número novo com número velho depois de
> trocar a otimização é comparar coisas diferentes.

---

## Pool de SRAM dos módulos: 96,6% ocupado

O alocador dos módulos RNBO (`module_rnbo_alloc.h`) serve de um pool
estático de **48 KB** em SRAM e só cai pra `AllocBig()` (SDRAM) no que
não couber. Estado com dois módulos:

| Módulo | SRAM | Blocos |
|---|---|---|
| delay | 9 368 B | 12 |
| vocoder | 38 128 B | 22 |
| **total** | **47 496 B de 49 152** | **96,6%** |

**Um terceiro módulo não cabe.** O número sempre foi esse; o warm-up no
`Init` só o tornou visível, porque antes parte das alocações acontecia
no primeiro `Process` e ficava fora da contabilidade.

### O método, antes de crescer o pool ou migrar às cegas

A decisão fica adiada até o módulo 3 existir, mas o **método** já está
definido, e ele importa mais que a decisão:

1. **Rotular as alocações por origem** no `MemReport`, separando
   **quentes** (lidas amostra a amostra — sinais de bloco, linhas de
   delay, buffers de FFT) de **mornas** (estado, filas de evento,
   tabelas consultadas por bloco).
2. **Quentes ficam na SRAM.** Migrá-las pra SDRAM custa latência de
   acesso dentro do callback, que é exatamente onde não há folga.
3. **Mornas migram barato.** Estado que se lê uma vez por bloco não
   sente a diferença.
4. Só então decidir: crescer o pool, migrar as mornas, ou as duas.

Crescer o pool às cegas rouba SRAM de quem talvez precise mais; migrar
às cegas pode empurrar um buffer quente pra SDRAM e criar um gargalo
onde não havia. **Sem o mapa, as duas escolhas são apostas.**

> Ressalva honesta sobre o rótulo: o alocador **não sabe** a semântica
> de quem chama — o RNBO pede memória sem dizer pra quê. Rotular por
> origem exige ou anotação manual por módulo, ou heurística por
> tamanho e fase de alocação. A heurística é barata e imperfeita; a
> anotação é exata e não escala pra patch que muda. Escolher entre as
> duas é parte da tarefa, não detalhe dela.

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
