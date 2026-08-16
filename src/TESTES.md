# TESTES.md — Cotucatronics OS · roteiro de bancada

**Documento v0.9.0** · última execução em placa: 2026-08-15 · firmware
**v0.9.0 / HAL v0.4.0** na QSPI. **T1, T2, T3, T5, T6, T7 e T8 passaram**; T4 espera
hardware, e o T7e (gate com piezo) espera junto.

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
| Firmware | Cotucatronics OS v0.5.0 / `hal_daisy_seed v0.4.0` · `APP_TYPE=BOOT_QSPI` |

Ocupação do firmware v0.4.1 (com módulo RNBO e medidor de CPU):

| Região | Usado | Total | % |
|---|---|---|---|
| FLASH (interna) | 0 B | 128 KB | 0,00% — é do bootloader agora |
| QSPIFLASH | 123 784 B | 7 936 KB | **1,52%** |
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

## T5 — módulo RNBO "delay" · ✅ PASSOU

**O critério de aceite do primeiro módulo.** Compilar não é evidência: o
patcher pode estar montado e mudo. Aqui a conta foi fechada por medição, não
só por ouvido — ver T5b.

**Pré-requisito:** bootloader gravado (`make program-boot`, uma vez) e
firmware na QSPI (`make program-dfu`). Ambos feitos em 2026-08-15;
executado com o firmware v0.4.1 / HAL v0.4.0.

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

**Resultado — passou:**

```
> mods
< modulo "delay": 2 params
<   [0] dly_time
<   [1] dly_mix
<   dly_time   -> dly_time [0]
<   dly_mix    -> dly_mix [1]
<   ram: 2000 SRAM (de 32768) + 1048584 SDRAM, 15 blocos, 0 falhas
> list
<  5 dly_time   norm=0.50 real=0.50
<  6 dly_mix    norm=0.50 real=0.50
```

7 parâmetros, ligações em `[0]` e `[1]`, nenhum `AVISO`, 15 blocos e
**0 falhas**.

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

**Resultado — passou, e medido em vez de só ouvido.**

Eco não muda afinação, então FFT de frequência não prova delay. O que prova é
a **cauda que sobra depois de cortar a fonte** — se o sinal persiste com a
fonte muda, ele está saindo da linha de delay e de nenhum outro lugar:

| Condição | Nível 1 s após `tone off` |
|---|---|
| `dly_mix = 0.0` | silêncio (−94 dBFS) |
| `dly_mix = 1.0` | **848 Hz a −33,7 dBFS** |
| `dly_time = 0.05` | cauda já extinta na janela de medição |
| `dly_time = 0.9` | −27,2 dBFS |

Com o tom ligado e 100% molhado: 848,23 Hz a −23,4 dBFS; logo após o corte,
848,01 Hz a −33,8 dBFS (−10,4 dB de decaimento); 3 s depois, silêncio.

O par `dly_mix = 0` → seco / `dly_mix = 1` → cauda é o controle que fecha a
cadeia inteira: **ParamTable → ligação por nome → `setParameterValue` do
RNBO → mudança audível**.

### T5c — CPU

```
> cpu
< cpu: 5.7% media, 9.9% pico (bloco de 48 @ 48000 Hz)
```

**Constante** em 5,7% / 9,9% com qualquer combinação de `dly_time` e
`dly_mix`, e igual em repouso e com o tom passando. Era esperado: a linha de
delay lê e escreve toda amostra independente do ajuste, então a carga não
depende do parâmetro. Carga determinística é o que se quer no palco — mas
significa que **o custo do módulo não cai quando ele está "desligado"**.

## T6 — curva de parâmetro e `set` serial · ✅ PASSOU

**Procedimento:** girar `k1` (→ `dly_time`, `curve 2.5`) por todo o curso e
conferir o `real` no `list`; depois exercitar `set <nome> <valor>`.

### T6a — a curva bate com a conta

`real = pow(norm, 2.5)`, medido nos cinco pontos:

| `k1` | Previsto | Medido |
|---|---|---|
| 0.00 | 0.000 | 0.00 |
| 0.25 | 0.031 | 0.03 |
| **0.50** | **0.177** | **0.18** |
| 0.75 | 0.487 | 0.49 |
| 1.00 | 1.000 | 1.00 |

Metade do curso do knob cobre os primeiros ~18% da faixa — os delays curtos
ganharam metade útil do percurso, que era o objetivo.

### T6b — o valor CURVADO chega no DSP

O teste que importa: não basta o `list` mostrar o número certo, o RNBO tem que
receber esse número. Medindo a cauda 1 s após cortar a fonte (440 Hz,
`dly_mix = 1.0`):

| `k1` | `dly_time` real | Cauda |
|---|---|---|
| 0.5 | 0.177 (curto) | −28,7 dBFS |
| 0.9 | 0.768 (longo) | **−7,2 dBFS** |

21,5 dB de diferença: delay curto repete mais vezes por segundo e a energia
decai mais rápido. Se o módulo estivesse recebendo o normalizado cru (0.5 e
0.9), a separação seria bem menor.

> **Armadilha que isso apanha.** O push pro módulo usava `params.Get()` — o
> normalizado **cru**. Com a curva só dentro do `Real()`, ela ficaria presa do
> nosso lado e nunca chegaria no RNBO: o sintoma seria "a curva não faz nada",
> sem erro, sem log, sem pista. O push agora usa `Real()`. **Todo parâmetro
> futuro herda isso:** o que chega no módulo é `Real()`, e é por ali que
> qualquer transformação de manifest passa a valer.

### T6c — `set <nome> <valor>`

```
> set tone_freq 440      < tone_freq -> real=440.000 (norm=0.188, curve 1.0)
> set dly_time 0.5       < dly_time -> real=0.500 (norm=0.758, curve 2.5)
> set naoexiste 0.5      < ? param "naoexiste" nao existe (use list)
```

Alcança qualquer parâmetro pelo nome **em unidade real**, mostra a curva
aplicada e recusa nome inexistente ou valor fora da faixa. `tone_freq` e `tone_mix` são **serial-only por design** — os
knobs 1–3 foram pro delay e pro level, e o tom de teste é bancada, não
performance.

## T7 — cadeia de dinâmica do shell · ✅ PASSOU (com ressalva no gate)

Gate → [módulo] → level → split → `OUT_L`: governor → [bench_boost] → limiter ·
`OUT_R`: limiter. Firmware v0.6.3 / dynamics v0.3.1.

### T7a — governor comprime, e comprime o previsto

Tom 440 Hz, `dly_mix 0`, nível fora da região de clipe do ADC:

| `gov_ceiling` | ganho | saída medida | previsto |
|---|---|---|---|
| 1.0 | 1.00 | −9,0 dBFS | — |
| 0.02 | 0.39 | −17,2 dBFS | **−8,2 dB** |

Redução medida **8,2 dB**, batendo ao décimo com `20·log₁₀(0,39)`, repetível
duas vezes.

> **Armadilha de medição.** Numa primeira tentativa o medido (6,7 dB) não batia
> com o previsto (11,1 dB). Não era o governor: `cotuca_measure.py` reporta
> **RMS**, e pra senóide o pico fica 3 dB acima — a leitura de −1,1 dBFS RMS
> significava pico em **+1,9 dBFS**, ou seja o ADC da interface estava clipando
> e a leitura alta saía subestimada. Baixado o nível, os números fecharam.
> Toda medida de nível aqui precisa de pico abaixo de 0 dBFS, não RMS.

### T7b — limiter segura, e a conta fecha

Com `bench_boost` (ferramenta de bancada) empurrando o sinal contra o teto:

| `bench_boost` | interno | `gainL` medido | previsto `0,95/env` |
|---|---|---|---|
| 1× | 0,25 | 1.00 | 1.00 |
| 2× | 0,50 | 1.00 | 1.00 |
| 4× | 1,00 | **0.96** | 0,95 |
| 8× | 2,00 | **0.48** | 0,475 |

A saída trava de 4× em diante enquanto a entrada dobra.

**Distorção — medida nativamente**, porque na placa o ADC clipa antes:

| Sobrecarga | Pico de saída | THD |
|---|---|---|
| 1,1× | 0,95000 | 0,13% |
| 2× | 0,95000 | 0,13% |
| 4× | 0,95000 | 0,13% |
| 8× | 0,95000 | **0,13%** |

**A distorção não cresce com a sobrecarga.** Em regime permanente o clamp duro
nunca atua: o ganho suavizado já trouxe o sinal abaixo do teto, e o que sai é
senóide limpa apenas escalada. O clamp só pega o transiente inicial de ~1 ms,
que é o papel de backstop dele.

> **Ressalva:** isso é regime permanente. Material **percussivo** — que é
> exatamente o piezo com taps — bate no clamp a cada ataque. Quando o piezo
> chegar, medir de novo com transiente real, não com senóide.

### T7c — roteamento

`route_exciter=0` silencia o canal medido; `route_jack=0` não o afeta.
Confirma que `OUT_L` é o ramo do exciter e que o mute entra depois do limiter.

### T7d — CPU

| Configuração | Média | Pico |
|---|---|---|
| Sem cadeia (v0.5.0, só delay RNBO) | 5,7% | 9,9% |
| Repouso, cadeia carregada | 10,2% | 20,5% |
| **Tudo engajado** (gate aberto + governor comprimindo + boost 8× + limiter atuando + delay) | **11,0%** | **20,5%** |

A cadeia custou ~5 pontos de média. Note que engajar tudo custa quase nada a
mais que o repouso: os três blocos rodam o detector e o ganho **sempre**, ativos
ou não. Carga determinística — bom pro palco, mas o custo não desaparece quando
o bloco está "desligado".

### T7e — gate · ⏸️ PARCIAL

Fecha corretamente com o chão em silêncio digital (`gain=0.00`, `env=0.0001`) e
abre com sinal (`gain=1.00`, `env=0.47`). Mas os dois critérios que importam —
*silenciar chão de ruído real* e *abrir no tap sem comer transiente* — precisam
do piezo. Com a entrada desligada não há chão que gatear.

O teste nativo cobre a lógica: fecha a −60 dBFS, abre em 10,1 ms, e deixa passar
**9,5% do sinal no primeiro milissegundo**. Esse último número é o que vale
discutir quando o piezo chegar: com attack de 10 ms o gate **come parte do
transiente**. Pra um tap seco em metal pode soar como perda de ataque. O
conserto seria attack de ~1 ms, e o detector já está rápido o bastante (0,5 ms)
pra sustentar isso sem falso disparo.

## T8 — shifter SSB (anti-feedback) · ✅ PASSOU

Quarto bloco permanente do stack, em C++ do shell. Ramo do exciter, entre o
split e o governor. Firmware v0.9.0 / dynamics v0.4.0.

**Este é o teste perfeito pro medidor por FFT**: o deslocamento é literalmente
uma frequência, então o critério de aceite é diretamente mensurável em vez de
inferido.

### T8a — o deslocamento acontece, e é o valor pedido

Tom 440 Hz, `set shift_hz 5`, medido no ramo do exciter:

```
ch2:  445.12 Hz   -7.4 dBFS   erro +0.12 Hz (+0.5 cents)
```

**445,12 Hz** = 440 + 5, com o mesmo desvio de clock de +0,12 Hz que aparece em
toda medição desde o T3. Deslocamento linear em Hz, não intervalo.

### T8b — banda lateral inferior suprimida

FFT alvo em três frequências, 3 s de captura, janela de Hann:

| Componente | Nível | vs. banda superior |
|---|---|---|
| **445 Hz** (banda superior) | −4,3 dBFS | — |
| 440 Hz (portadora) | −101,0 dBFS | **−96,7 dB** |
| 435 Hz (banda inferior) | −54,9 dBFS | **−50,5 dB** |

**50,5 dB de supressão**, contra os 30 dB pedidos — e batendo *exatamente* com
os 50,5 dB previstos nativamente a 440 Hz. A portadora original praticamente
desapareceu (96,7 dB abaixo): o SSB está limpo.

Perfil nativo em toda a banda:

| Tom | Supressão |
|---|---|
| 100 Hz | 55,7 dB |
| 440 Hz | 50,5 dB |
| 5 kHz | 46,8 dB |

### T8c — CPU

| Configuração | Média | Pico |
|---|---|---|
| Sem shifter (v0.8.0) | 9,6% | 20,4% |
| **Com shifter** | **10,7%** | **23,6%** |

**~1 ponto de média**, abaixo do orçamento de 3–6% previsto. Constante para
qualquer `shift_hz` — o filtro roda sempre, como os outros blocos do stack.

O barateamento vem do oscilador de quadratura em **forma acoplada** (4
multiplicações por amostra) em vez de dois `sinf`/`cosf` por amostra.

### T8d — jack sem shift · ⏸️ PENDENTE

`OUT_R` deve sair em 440 Hz intacto, por design. Não medido: o cabo está no
pino 18 (`OUT_L`) e medir o outro ramo exige movê-lo pro pino 19.

> ### A armadilha que quase passou: `a` vs `a²`
>
> A primeira versão do shifter dava **18,9 dB** de supressão, degradando em
> direção ao meio da banda (60 dB a 100 Hz, 19 dB a 5 kHz). Testei as quatro
> combinações de atraso e sinal — a melhor parava em 19 dB. Testei duas formas
> de seção allpass — sem melhora.
>
> A causa: **os coeficientes clássicos publicados são `a`, e a seção usa `a²`.**
> Elevar ao quadrado levou de 18,9 pra 46,8 dB a 5 kHz. Não era erro de
> estrutura nem de sinal — era convenção não documentada no conjunto de
> coeficientes.
>
> **Se tivesse gravado sem medir, o anti-feedback ficaria com metade da
> supressão e ninguém descobriria até uma escultura apitar no palco.** É o
> argumento mais forte que este projeto tem a favor de medir antes de gravar.

---

## Defeitos conhecidos

| # | Sintoma | Estado |
|---|---|---|
| 1 | `list` travava a placa (CDC calada até o reset) | ✅ corrigido no HAL v0.2.0 |
| 2 | Todo `%f` saía vazio — `board` sem o `48000`, `list` com `norm= real=` | ✅ corrigido no Makefile |
| 3 | Linha de log saía picotada e grudada na seguinte | ✅ corrigido no HAL v0.3.1 |
| 4 | Comando serial demorava até 0,5 s pra responder | ✅ corrigido no main.cpp v0.3.0 |
| 5 | Soft-knee do governor tinha degrau de 0,09 no topo do joelho | ✅ corrigido no dynamics v0.3.1 |
| 6 | `set <param>` recebia NORMALIZADO, não real — `set bench_boost 1.0` dava **8×** | ✅ corrigido no main.cpp v0.7.0 |

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

**#6 — `set` recebia normalizado.** Era coerente com `ParamTable::Set()`, mas
virava armadilha em parâmetro cuja faixa não é `[0,1]`: `set bench_boost 1.0`
parecia "sem boost" e entregava **8×**, o máximo — num parâmetro que existe
justamente pra sobrecarregar a saída do exciter. "Parece seguro e é o máximo" é
a inversão errada pra ter num instrumento.

`set` agora fala em **unidade real** — Hz, ms, ×. Converte pra normalizado
invertendo a curva:

```
norm = pow((real - min) / (max - min), 1/curve)
```

e **recusa** fora da faixa em vez de saturar: quem digita 12 num parâmetro que
vai até 8 errou, e saturar em 8 esconderia o erro.

`Set()` normalizado continua existindo ao lado de `SetReal()`, e knob e macro
seguem usando ele — posição de curso é normalizada por natureza. A mudança é só
na fronteira humana.

Efeito colateral bem-vindo: conserta o `dly_time` também. Com `curve 2.5`,
`set dly_time 0.5` dava real `0.177`; agora dá `0.500`. Antes era preciso saber
a curva de cabeça pra digitar o valor certo.

**Validado em placa** (firmware v0.7.0):

```
> set tone_freq 440     < tone_freq -> real=440.000 (norm=0.188, curve 1.0)
> set bench_boost 1     < bench_boost -> real=1.000 (norm=0.000, curve 1.0)
> set bench_boost 12    < fora da faixa: bench_boost aceita [1.000 .. 8.000]
> set dly_time 0.5      < dly_time -> real=0.500 (norm=0.758, curve 2.5)
> set tone_freq 5000    < fora da faixa: tone_freq aceita [80.000 .. 2000.000]
```

`pow(0.5, 1/2.5) = 0.758` confere a inversão da curva, e a FFT confirma os
440 Hz na saída (440,12 Hz medidos, +0,5 cents — o desvio de clock de sempre).

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

### Quanto o RNBO pede — previsão vs. medido

A previsão errou por exatamente **2×**, e a causa é instrutiva:

| Origem | Conta | Bytes |
|---|---|---|
| Previsto | 131 072 samples × **4 B**, supondo que `RNBO_USE_FLOAT32` valesse pra linha de delay | 524 296 |
| **Medido** | 131 072 samples × **8 B** + 8 B de header | **1 048 584** |

**Por que o dobro:** `RNBO_USE_FLOAT32` troca o `SampleValue` (os sinais do
patcher) para `float`, mas a linha de delay é um `buffer~`, e o código gerado
a instancia como **`Float64Buffer`** — 8 B por amostra, independente da macro:

```cpp
this->delaytilde_01_del_buffer = new Float64Buffer(this->delaytilde_01_del_bufferobj);
```

131 072 × 8 = 1 048 576, mais 8 do nosso header = **1 048 584**, que é o número
medido, ao byte. A macro não alcança o armazenamento de `buffer~`.

**Consequência de planejamento:** todo `buffer~` de um patch custa 8 B por
amostra, não 4. Um freeze de 10 s a 48 kHz pede ~4 MB, não 2. Com 64 MB de
SDRAM ainda sobra muito, mas a conta pra dimensionar módulos é essa.

Os demais buffers do patcher são do tamanho do bloco e **couberam no pool de
32 KB em SRAM**, como previsto: 2 000 B em 15 blocos, 6% do pool. Só a linha de
delay foi pra SDRAM.

> **Nota sobre `RNBO_USE_FLOAT32`:** sem essa macro, `SampleValue` seria
> `double` e a mesma linha de delay pediria **1 MB** em vez de 512 KB.
