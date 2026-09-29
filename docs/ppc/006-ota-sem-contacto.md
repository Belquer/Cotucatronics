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

> **🔴 DESENHO REVISTO a 29/09, e a medição é que o obrigou.** A versão
> anterior desta secção supunha dois slots **executáveis**. Não dá sem custo:
> o app executa da QSPI (`BOOT_QSPI`) e um binário só corre no endereço para
> que foi ligado — dois slots executáveis exigiriam **compilar a imagem duas
> vezes**, uma por endereço, e escolher a certa no envio. E `BOOT_SRAM`, que
> resolveria isso, está fora: **285 KB de código + 325 KB de dados = 610 KB**
> contra 512 KB de SRAM (medido a 29/09).

O bootloader da Electrosmith salta para um endereço **fixo** (`0x90040000`).
Logo é lá que a decisão tem de viver.

**O mapa real da QSPI, lido do descritor do DFU a 29/09** — e ele corrigiu um
número deste plano:

```
@Flash /0x90000000/64*4Kg  /0x90040000/60*64Kg  /0x90400000/60*64Kg
        └ sectores de 4 KB   └ sectores de 64 KB
```

⚠️ **O META não pode viver em `0x90048000`**, como esta secção dizia primeiro:
ali o sector mínimo apagável são **64 KB**, e um META de 4 KB não se reescreve
sem apagar 64. Mas há **256 KB de sectores de 4 KB** em `0x90000000`, abaixo do
app, e é lá que ele pertence.

```
0x90000000  META          4 KB   (sector pequeno: reescreve-se barato)
0x90040000  ESCOLHEDOR   64 KB   um sector, gravado UMA vez, NUNCA por OTA
0x90050000  EXECUÇÃO    512 KB   onde o app SEMPRE corre (um só endereço)
0x90200000  ENCENAÇÃO   512 KB   onde a imagem nova aterra, vinda do rádio
            (sobram ~6,5 MB)
```

✅ **E o mecanismo de gravação está provado (29/09):** o `FLASH_ADDRESS` do
Makefile da libDaisy é `?=`, logo aceita ser trocado, e o bootloader anuncia a
QSPI inteira ao DFU. Escrevi 289 KB em `0x90200000` e a placa continuou a
arrancar de `0x90040000` sem estranhar — gravar num endereço não perturba o
outro, que é o que todo o resto assume.

**A ideia que troca "dois executáveis" por "uma cópia repetível":** o app tem
**um** endereço de execução, e a imagem nova chega a uma área de **encenação**.
O escolhedor, no arranque:

1. lê o META;
2. se houver **intenção pendente**, copia encenação → execução e só então
   limpa a intenção;
3. confere o CRC da área de execução;
4. põe o `VTOR` e salta.

⚠️ **E é aqui que está a propriedade toda:** se a energia cair **a meio da
cópia**, a área de execução fica meia escrita — mas a **encenação continua
intacta** e o META **continua a dizer "pendente"**. No arranque seguinte o
escolhedor repete a cópia. A janela perigosa deixa de ser fatal e passa a ser
**repetível**, que é coisa diferente.

| o que pode falhar | porquê não mata |
|---|---|
| energia a meio da recepção | escreve-se na **encenação**; a execução nem foi tocada |
| energia a meio da **cópia** | a encenação sobrevive e o META diz "pendente": repete-se |
| imagem nova com defeito | CRC na recepção **e** antes do salto; e o app novo tem de confirmar-se vivo, senão o META volta ao anterior |
| META corrompido a meio | escrita em **dois passos** — grava o novo, só depois invalida o velho |
| escolhedor com defeito | **não se actualiza nunca**; gravado uma vez e provado com sabotagem antes de fechar |

⚠️ **O risco que FICA, e é o único:** um defeito **no escolhedor**. Pequeno,
sem dependências, e provado com sabotagem antes de a caixa fechar — CRC
errado, META em branco, encenação vazia, e corte de energia a meio da cópia.

⚠️ **E o app tem de ser religado** de `0x90040000` para `0x90060000`. Enquanto
o escolhedor não estiver gravado, uma imagem religada **não arranca** — o
bootloader saltaria para um `0x90040000` vazio. **Por isso os passos 3 e 4 são
uma sessão física, e não se começam sem a caixa aberta.**

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

3. 🟡 **Escolhedor + META + slots — EM CURSO.**

   ✅ **Provado a 29/09:** o `FLASH_ADDRESS` aceita ser trocado e o bootloader
   anuncia a QSPI inteira ao DFU; o app religa para `0x90050000` e o binário
   sai correcto (`.isr_vector 0x90050000`); o mapa real de sectores está lido.

   🔴 **E a ORDEM é obrigatória, por cicatriz (15fv):** o **escolhedor
   primeiro**, num sector de 64 KB em `0x90040000`, e só depois o app no slot
   de execução. Gravar o app religado enquanto o app velho ainda ocupa
   `0x90040000 + 289 KB` **apaga o que está a correr** — os dois sobrepõem-se
   em 225 KB, e foi o que aconteceu.

   ✅ **E a recuperação ficou mais barata:** o bootloader passou de
   `extdfu-10ms` para `intdfu-2000ms`. A janela de DFU era de **dez
   milissegundos** — não havia instante a apanhar — e agora são **dois
   segundos**.
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
