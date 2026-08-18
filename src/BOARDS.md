# BOARDS.md — placas suportadas e regras de recurso

**Documento v0.10.0**

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

## Painel v2 — guia de solda

**5 pots + 2 botões momentâneos.** Hardware ainda não soldado; o
firmware já está pronto e testável por serial (T11 no
[TESTES.md](TESTES.md)).

### Os 5 pots

| Pot | Função | libDaisy | Pino físico |
|---|---|---|---|
| k1 | **level** (volume master) | `seed::A0` | **16** |
| k2 | **src_mix** (piezo ↔ jack) | `seed::A1` | 17 |
| k3 | macro de caráter | `seed::A2` | 18 |
| k4 | macro de caráter | `seed::A3` | 19 |
| k5 | macro de caráter | `seed::A4` | 20 |

**10 kΩ lineares.** Ligação de cada um:

```
  3V3A  ──── terminal 1
  wiper ──── ADC (A0–A4)  ──┬── 100 nF ── AGND
  AGND  ──── terminal 3     └── (o capacitor vai JUNTO do wiper)
```

O **100 nF no wiper** não é opcional: sem ele o ADC lê o ruído que o
pot capta como antena, e o parâmetro treme sozinho. Ponha perto do
pino da placa, não perto do pot.

**Use 3V3A e AGND, não 3V3D e DGND.** Os pots são referência analógica;
alimentá-los pelo lado digital injeta o chaveamento direto no valor
lido.

> **k1 e k2 são FIXOS em todo instrumento** — volume e fonte. Só k3–k5
> mudam de escultura pra escultura. Isso dá ao painel uma gramática
> constante: a mão aprende uma vez e vale pra qualquer peça.

### Os 2 botões

| Botão | Função | libDaisy | Pino físico |
|---|---|---|---|
| B1 | randomizador | `seed::D26` | 33 |
| B2 | assinalável (hold) | `seed::D27` | 34 |

```
  botão ── um terminal no pino (D26/D27)
        └─ outro terminal em DGND
```

**Sem resistor externo.** O firmware liga o pull-up interno, então o
botão só precisa fechar pra terra. Repouso lê alto, pressionado lê
baixo. Debounce de 10 ms é feito em software, por tempo.

> ### ⚠️ A0–A4 SÃO D15–D19. O mesmo pino físico.
>
> Na Daisy, `A0` é apenas outro nome pra `D15`, `A1` pra `D16`, e
> assim por diante até `A4` = `D19`. **Não são pinos diferentes.**
>
> É por isso que os botões foram pra D26/D27: qualquer pino de D15 a
> D19 colidiria com um pot, e D13/D14 já são a UART do MIDI. D26 e
> D27 estão entre os poucos sem alias de ADC (A10=D25, A11=D28) e sem
> função tomada.
>
> Se um dia mover um botão, **confira o alias antes de soldar** —
> a colisão não dá erro de compilação nem de boot: dá pot que não lê
> e botão que não responde, os dois em silêncio.

**Sobre os números de pino físico:** seguem a mesma contagem do
[TESTES.md](TESTES.md), em que o pino 18 é AUDIO OUT L e o 20 é AGND.
**Confira contra o diagrama oficial da Electrosmith antes de encostar
o ferro** — a referência que não depende dessa contagem é o nome
`D`/`A`.

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

### ✅ CONFERIDO 2026-08-17, com o WIDI Core na mão

Antes de encostar o ferro, o estado que importa:

- **Pinos:** D13 (TX, PB6) e D14 (RX, PB7) — vêm direto do `daisy_seed.h`
  da libDaisy, são os default do `MidiUartTransport`, e não colidem com
  nada: os cinco pots ocupam A0–A4 (= D15–D19) e os botões D26/D27.
- **Parser:** board-agnostic em `midi.h`, commitado em `b3d994c`. A ISR
  só enfileira; quem despacha é `hal::PollMidi()` no loop principal —
  mesma disciplina que o serial aprendeu na marra.
- **Mapa de CC:** 13 entradas (limite 16). Ver a tabela abaixo.

> **O MIDI deixou de ser conveniência e virou CONTROLE DE VOO.** A serial
> USB morre segundos depois do vocoder subir (defeito conhecido, ver
> [TESTES.md](TESTES.md)); o MIDI é imune — UART própria, DMA, nada a ver
> com o CDC. Todo param que só a serial alcançava está no mapa agora.

### Mapa de CC do instrumento atual

| CC | Parâmetro | | CC | Parâmetro |
|---|---|---|---|---|
| 20 | `dly_time` | | 27 | `voc_tone` |
| 21 | `dly_mix` | | 28 | `voc_mix` |
| 22 | `level` | | 29 | `rev_mix` |
| 23 | `src_mix` | | 30 | `in_gain` |
| 24 | `gov_ceiling` | | 31 | `gate_thresh` |
| 25 | `fx2_gain` | | 32 | `buzios_on` (0–63 off, 64–127 on) |
| 26 | `voc_pitch` (grade cromática) | | | |

CCs 20–31 são a faixa "undefined" da spec MIDI — não colidem com nada
padrão (1 é mod wheel, 7 é volume, 64 é sustain), então controlador
genérico sai de fábrica sem usá-las.

**`voc_pitch` tem `steps=49`:** o `Set()` quantiza em semitons, e os 128
passos do CC caem na grade cromática sozinhos. A quantização mora no
ponto ÚNICO de escrita, então MIDI, serial e knob herdam a mesma grade —
nada a tratar aqui.

**`buzios_on` vira chave sem tratamento especial:** o param é [0,1], o
`main` testa `>= 0.5`, e `MidiNorm(63)=0.496` cai abaixo enquanto
`MidiNorm(64)=0.504` cai acima. O contrato já fazia isso funcionar.

### ⚠️ PROCEDIMENTO OBRIGATÓRIO: reparear depois de todo RESET

**O pareamento BLE não sobrevive ao reset da placa.** Descoberto pelo dB
em 2026-08-17, depois de uma tarde inteira de diagnóstico errado.

```
1. gravar / resetar a placa
2. no app: ESQUECER o dispositivo WIDI
3. parear de novo
4. só então testar
```

**Gravar e testar MIDI não cabem no mesmo ciclo.** O RESET necessário pra
gravar invalida o pareamento, então a ordem é sempre
*gravar → reparear → testar*, nunca gravar no meio de um teste.

> **Por que isso enganou por tanto tempo:** o sintoma é `bytes 0` limpo —
> UART reportando ESCUTANDO, zero erro, zero descarte. Idêntico a fio
> cruzado errado, a módulo sem alimentação, a app sem autorização e a
> central competindo. Todo zero de 2026-08-17 veio depois de um reset sem
> reparear; todo sucesso veio depois de parear. **A regra de suspeita
> passa a ser: `bytes 0` logo após gravar = pareamento, antes de qualquer
> outra hipótese.**

### 🔬 MIDI SCOPE — o analisador é nosso, e roda na própria placa

Ferramenta **oficial e permanente**. Sem apps externos, sem hardware
extra: um cabo USB e a serial.

```
mscope on      stream: hex cru + o que o parser CONCLUIU de cada byte
mscope stats   uma linha por segundo (pro caso fader-deslizando)
mscope off
midi           contadores permanentes, sempre ativos
```

**Por que duas colunas.** O objetivo nº1 é ver o parser **dessincronizar**,
e pra isso não basta mostrar o que chegou — tem que mostrar o que ele
concluiu. O mesmo byte significa coisas opostas conforme o estado: `0x45`
é *data1 de um CC* ou *órfão*, dependendo do que veio antes.

```
  B0  status canal
  1D  data parcial (data1)
  45  CC ch1 cc29 val69 -> rev_mix 0.543
  45  ?? DADO SEM STATUS
  B0  !! TRUNCADA (ressync)
```

**Contadores permanentes** (no `midi`, sempre ativos, espelhados na
caixa-preta a cada segundo):

| | |
|---|---|
| `bytes` | chegaram pela UART |
| `msgs completas` | viraram mensagem |
| **`APLICADAS`** | acharam dono no mapa |
| `ressync` | status chegou com data pendente = mensagem cortada |
| `órfãos` | data sem status = fluxo pego no meio |
| `fila perdeu` | descarte na ISR |
| estado | aguardando STATUS / data1 / data2 |

> **`msgs completas` vs `APLICADAS` é a separação que importa:** "chegou e
> não virou mensagem" e "virou mensagem e não virou som" são consertos
> completamente diferentes. Um é fio/parser, o outro é mapa de CC.

**O scope respeita o orçamento duro.** Ele escreve pelo `hal::Log`, que
descarta em vez de esperar — em rajada o stream tem **buracos, e isso é
projeto**. Ferramenta de diagnóstico que segura o loop é o próximo
`DelayMs` (ver cap. 7 do [battle_scars](../docs/battle_scars.md)). **Os
contadores nunca mentem; o stream sim** — e é por isso que existe o modo
`stats`, em que o dilúvio vira uma linha por segundo.

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

## Ruído: o USB do computador é a fonte dominante

> **Alimentar a Daisy pelo USB do computador injeta ruído audível.**
> Medido, não suposto.
>
> **A bateria interna sempre esteve no projeto; o USB é só pra gravar
> firmware.** Então isto não é requisito novo — é a medida do que a
> configuração de bancada custa, e a confirmação de que o ruído
> reportado NÃO acompanha o instrumento montado.
>
> Na bateria, o único ruído que sobra é o residual de 1 kHz abaixo —
> nosso, a −90 dBFS. É o único que vai pro campo.

Protocolo: entrada em curto (`AUDIO_IN_L` → `AGND`, piezo fora), cadeia
neutralizada (`dly_mix` 0, `tone_mix` 0, `shift_hz` 0, `bench_boost` 1×,
`gate_thresh` **0** pra manter o gate aberto — com ele fechando, o piso
fica mascarado e a medição não vale nada). Espectro por `cotuca_noise.py`.

| | USB do computador | Powerbank | Δ |
|---|---|---|---|
| Pico | −44,1 dBFS | **−64,4** | **−20 dB** |
| 16 002 Hz | −80,9 | some | — |
| 7 998 Hz | −85,2 | some | — |
| 3 000 Hz | −86,9 | −100,5 | −13,6 |
| 1 998 Hz | −88,9 | −97,4 | −8,5 |
| 1 002 Hz | −88,1 | −90,4 | −2,3 |

**Os dois sintomas têm assinatura distinta e a mesma fonte:**

- **Apito agudo** = o harmônico de **16 kHz** da série de 1 kHz. Some por
  completo na bateria.
- **Crackling** = energia **impulsiva**: fator de crista de 30 dB no USB
  (pico 30 dB acima do RMS), contra 12 dB na bateria. Ruído contínuo tem
  crista baixa; crista alta é evento isolado, que é o que se ouve como
  estalo.

### O residual de 1 kHz é NOSSO, e é a taxa de bloco

Os 1 002 Hz sobrevivem na bateria quase no mesmo nível. **Sem host USB não
existe frame** — então esse pico não pode ser USB. Bloco 48 a 48 kHz dá
exatamente **1 000 blocos por segundo**.

Fica registrado como **não resolvido e provavelmente inaudível**: −90 dBFS
e sem os harmônicos altos, que eram o que o ouvido pegava. Se um dia
incomodar, o teste que confirma é barato — **mudar o bloco move a
frequência** (128 → 375 Hz). E as duas causas plausíveis movem junto, então
o teste confirma a origem mas **não distingue**:

1. **Software** — parâmetro lido uma vez por bloco e aplicado em degrau na
   fronteira: zipper noise. Conserto: interpolar o ganho ao longo do bloco.
2. **Hardware** — pico de consumo do callback acoplando na alimentação
   analógica. Software não resolve.

### Armadilha de interpretação registrada

Na primeira leitura deste espectro a série de 1 kHz foi nomeada "frame
USB" porque batia com um suspeito da lista. Estava certo por acaso: no
teste 1 a placa **estava** no USB do computador. Quando o teste da bateria
foi lido fora de ordem, a mesma série virou "taxa de bloco" e a conclusão
inverteu.

**Frequência coincidir com suspeito conhecido não é identificação.** Duas
fontes diferentes aqui produzem exatamente 1 kHz, e só o teste que remove
uma delas separa as duas.

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
