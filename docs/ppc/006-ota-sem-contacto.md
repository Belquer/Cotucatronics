# ppc/006 — firmware e diagnóstico **sem contacto físico**, por BLE, sem hipótese de brick
<!-- doc-check: ignore esp_ota_mark_app_valid_cancel_rollback (API do ESP-IDF) -->
<!-- doc-check: ignore espota (ferramenta do Arduino/ESP32) -->
<!-- doc-check: ignore bleak (biblioteca BLE de Python) -->
<!-- doc-check: ignore qspi_raw (ferramenta em src/) -->
<!-- doc-check: ignore VTOR (registo do Cortex-M) -->

> **Estado: PLANO.** Nada implementado. Escrito a 2026-09-29, a pedido do dB:
> *"firmware e troubleshooting completo sem possibilidade de brick e sem
> precisar ter contato físico com qualquer uma das placas"*, por **BLE** — não
> WiFi, porque tem de funcionar em qualquer rede ou sem nenhuma.

- **Data:** 2026-09-29
- **Ramo:** `claude/slack-session-sm2ldz`
- **Pareia com:** `battle_scars.md` 12v (a janela de escrita da QSPI),
  `ppc/005` (a causa do envio de 64 bytes, ainda aberta)

## §0 — O requisito que manda em tudo

> **Zero hipótese de brick.** Não é um desejo: é o critério que escolhe a
> arquitectura. Um OTA que pode deixar a placa muda obriga a abrir a caixa —
> que é precisamente o que este trabalho existe para evitar. Um OTA assim é
> **pior que não ter**, porque troca um incómodo certo por um desastre raro.

E o corolário incómodo, dito já: **há UMA sessão física obrigatória**, a
última. Não se instala um mecanismo de recuperação por um caminho que ainda
não existe.

## §1 — O que já está construído, e não é pouco

| peça | onde | porque importa aqui |
|---|---|---|
| app a viver na **QSPI** (`APP_TYPE=BOOT_QSPI`) | `src/Makefile` | actualizar = escrever na QSPI, não na flash interna |
| **janela de escrita da QSPI a correr da ITCM** | `cotucatronics_qspi.lds`, `qspi_raw.cpp` | é *exactamente* a peça que um OTA precisa, e já está provada nos presets |
| **7 936 KB** de QSPI, app ocupa **284 KB** | `.lds` | sobram ~7,6 MB: cabem **quatro** imagens |
| **SysEx já parseado** (`SysexInicio/Corpo/Fim`) | `midi.h`, `main.cpp` | o console cabe no fio que já existe, sem fio novo |
| bootloader da Electrosmith na **flash interna** | 128 KB, intocado | o OTA nunca lhe toca — é terra firme |

⚠️ **E o que NÃO existe:** só três sinais ligam a ponte à Daisy (`IO20`
MIDI_TX, `IO21` MIDI_RX, `IO3` PONTE_STATUS). **Não há fio sobrando** — o
`IO9 (BOOT)` do ESP32 está desligado, logo a Daisy não o pode pôr em
bootloader. Tudo tem de passar pelo par que já lá está.

## §2 — Arquitectura, em três camadas

```
Mac  --BLE NUS-->  ESP32-C3  --SysEx @ UART-->  Daisy  -->  QSPI
```

**Transporte (Mac ↔ ponte): Nordic UART Service.** Um cano de bytes puro, ao
lado do BLE MIDI que já lá está — dois serviços GATT na mesma ligação. Do lado
do Mac conduz-se com `bleak`, e eu corro-o daqui.

**Multiplexagem (ponte ↔ Daisy): SysEx no fio de 31250.** O par MIDI é o único
que há, e o SysEx é a única moldura que **não se confunde** com tráfego de CC.
A Daisy já o reconhece; falta agir sobre ele.

⚠️ **E a UART é o gargalo, com conserto:** 284 KB a 31250 baud são **92 s** (mais
o custo de 7 bits do SysEx: ~105 s). Aquele fio não é MIDI de verdade, é dois
chips a falar: durante a transferência sobe para **1 Mbaud (~3 s)** e volta.
A subida negoceia-se por SysEx e **reverte por timeout** — se o outro lado não
responder na nova velocidade, os dois caem para 31250 sozinhos.

## §3 — O mecanismo que torna o brick impossível

O bootloader da Electrosmith carrega de um endereço **fixo** (`0x90040000`).
Logo, a escolha entre imagens tem de viver lá.

```
0x90040000  ESCOLHEDOR   ~8 KB   gravado UMA vez, fisicamente, NUNCA por OTA
0x90050000  META          4 KB   qual slot vale, CRC, contador de tentativas
0x90060000  SLOT A       512 KB
0x900E0000  SLOT B       512 KB
            (sobram ~6,5 MB)
```

**O escolhedor** é um programa minúsculo: lê o META, confere o **CRC da imagem
inteira** do slot preferido, põe o `VTOR` e salta. CRC mau → tenta o outro
slot. Os dois maus → fica num **modo de resgate** que só sabe fazer uma coisa:
receber uma imagem pela UART e gravá-la. É o fundo do poço, e tem chão.

**Por que não pode brickar, e a lista é curta:**

| o que pode falhar | porquê não mata |
|---|---|
| falha de energia a gravar um slot | grava-se sempre o slot **inactivo**; o activo fica intacto |
| imagem nova com defeito | o escolhedor exige CRC **e** o app novo tem de confirmar-se vivo; sem confirmação, o META volta ao slot anterior no arranque seguinte |
| META corrompido a meio | escreve-se em **dois passos** — grava o registo novo, só depois invalida o velho; há sempre um válido |
| escolhedor com defeito | **não se actualiza nunca**; é gravado uma vez e verificado byte a byte antes de fechar a caixa |
| bootloader da Electrosmith | o OTA nunca escreve na flash interna |

⚠️ **O risco que FICA, e é honesto dizê-lo:** um defeito **no escolhedor**. Ele
é a única peça sem rede de segurança, e por isso tem de ser pequeno, sem
dependências, e provado com sabotagem antes de a caixa fechar — imagem com CRC
errado, META em branco, os dois slots vazios. Se ele estiver certo, o resto
recupera-se sozinho.

## §4 — Diagnóstico sem contacto: o console por SysEx

Hoje tudo — `board`, `midi`, `fio`, `oledspi`, `knob`, `pk` — vive na **CDC do
USB**, que é o cabo que se quer deixar de ligar. O `hal::Log` escreve por lá.

O plano **não reescreve o console**: acrescenta uma segunda saída. Cada linha
de log sai também embrulhada em SysEx, e cada comando entra pelo mesmo
caminho. Do lado do Mac, o `cotuca_serial.py` ganha um modo BLE e **todos os
comandos que já existem passam a funcionar sem fio**, incluindo os que esta
semana custaram uma tarde cada.

> **Isto sozinho já paga o trabalho.** Metade do que se fez em Setembro foi
> diagnóstico pela consola; fazê-lo sem abrir a caixa muda o custo de cada
> pergunta.

## §5 — Combinações de botões: recuperação sem console

O painel tem três arcade e o push do encoder. Reservam-se combinações que o
**escolhedor** e o app reconhecem, para o caso de o BLE não subir:

| combinação (a segurar no arranque) | o que faz |
|---|---|
| FRZ A + FRZ B | arranca o **slot anterior** (volta atrás à mão) |
| FRZ A + FRZ B + búzios | **modo de resgate**: só recebe imagem pela UART |
| búzios + push do encoder | anuncia versão e slot activo pelos LEDs |

⚠️ Os botões vivem no painel, e o painel liga por fita. **A recuperação não
pode depender do painel estar são** — por isso o modo de resgate tem de ser
alcançável também por SysEx, e o escolhedor entra nele sozinho quando os dois
slots reprovam o CRC.

## §6 — Fases, e a ordem não é negociável

1. ✅ **Console por BLE — FEITO a 29/09** (`battle_scars` **15ft**). O dB
   escreveu `board` no Mac e a placa respondeu sem cabo. Os dois sentidos
   provados com contadores nos três pontos: `NUS: N escrita(s)` na ponte,
   `consola sem fio: N linha(s) executada(s)` na Daisy, e `N B da Daisy ->
   Mac` de volta. Ferramentas: `tools/cotuca_ble.py`,
   `run/consola-sem-fio.command`.
   ⚠️ **Dívida medida:** o anel do espelho (2 KB) enche numa rajada — `144
   perdidos` no primeiro `board`. Melhor-esforço por desenho, e contado. Se
   incomodar: anel maior (a SRAM tem folga) ou subir o baud do par durante
   rajadas, que é a mesma manobra da fase 5.
2. ✅ **OTA da ponte — FEITO a 29/09** (`battle_scars` **15fu**). Imagem de
   628 KB por BLE, MD5 conferido, arranque em `app1` e **auto-confirmação**
   após 15 s com o rádio a anunciar. Quatro tentativas abortadas não deixaram
   marca: escreve-se sempre na partição inactiva.
   **O que destravou:** MTU 517 (dobrou o débito) e **prazo de supervisão de
   720 ms → 4 s** — uma escrita na flash bloqueia **85 ms**, e a tolerância ao
   silêncio é que faltava, não velocidade. Débito final ~1,6 kB/s (~6,5 min).
   Comandos: `!board` e `!ota <N> <md5>`; cliente `tools/cotuca_ble.py --ota`.

3. **Escolhedor + META + slots**, provado com sabotagem, ainda **com a caixa
   aberta**.
4. **Sessão física final:** gravar o escolhedor, gravar as duas imagens,
   exercitar as três sabotagens e as combinações de botões. Só então fechar.
5. **OTA da Daisy por BLE**, já com rede de segurança debaixo.

## §7 — Aceitação

- Uma gravação da Daisy por BLE, de ponta a ponta, **sem tocar na caixa**.
- Corte de energia forçado a meio da gravação de um slot → arranca no anterior.
- Imagem com CRC deliberadamente errado → recusada pelo escolhedor.
- Os dois slots inválidos → cai no modo de resgate e aceita imagem pela UART.
- Console por BLE a responder `board`, `midi` e `fio` como responde hoje no USB.

## §8 — O que ainda não sei, e não vou fingir que sei

- **Débito real do NUS no C3** com BLE MIDI a correr ao lado. Mede-se na fase 1.
- Se o **1 Mbaud** é fiável no percurso real (fita, conector, 185 mm). Se não
  for, cai-se para 460800 ou 230400 — o desenho não depende do número.
- Quanto do `hal::Log` pode ser espelhado sem custar voltas ao laço. ⚠️ Esta
  semana um periférico bloqueante levou o laço de 13 000 voltas/s a **1**
  (bts 15fi) e destapou um defeito latente no MIDI (15fn). **O console por
  SysEx não pode ser bloqueante**, e isso entra no desenho desde a primeira
  linha, não como afinação posterior.
