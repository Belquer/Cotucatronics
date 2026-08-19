# STATUS — Cotucatronics

**Onde estamos.** O par deste documento é
[battle_scars.md](battle_scars.md), que diz *o que aprendemos chegando
aqui*. Leia os dois antes de tocar em código.

> Atualizado em 2026-08-17 · último commit desta leva: `7ba5744`

---

## Rotina de operação: os dois destinos, sempre juntos

**Toda nova versão de `STATUS.md` ou `battle_scars.md` vai para os DOIS
destinos na mesma ação:**

| Destino | Caminho | Forma |
|---|---|---|
| Repo | `docs/` | cru, sem frontmatter |
| Vault | `~/Documents/Obsidian Vault/Cotucatronics/` | com frontmatter YAML + tags |

Os dois servem leitores diferentes e nenhum substitui o outro. O **repo**
é o que sobrevive ao clone limpo — documento citado que não existe no
clone é a mesma classe do `module_rnbo_alloc.h` que faltou por semanas.
O **vault** é onde se lê, linka e pensa entre sessões, com tags e
backlinks que o Markdown cru não tem.

Nunca deixar um destino pra depois: o par desalinha em silêncio, e o
desalinho só aparece quando alguém precisa da versão certa.

---

## Como retomar

```bash
cd ~/CODE/GitHub/Cotucatronics/src
make BOARD=daisy_seed && make check
# RESET simples (SEM segurar BOOT — isso entra no DFU da STM, onde mora
# o bootloader). Grave na janela de graça de ~2 s:
make program-dfu
./cotuca_serial.py --send board mods list --gap 2
```

Placa roda da QSPI via bootloader da Electrosmith (`APP_TYPE=BOOT_QSPI`),
480 MHz permanente, `-O3` (ver a cicatriz do `-O3` no cap. 3 do battle
scars — ele PIOROU; reverter exige remedir tudo).

---

## O que toca hoje

**Delay + vocoder em série, com Búzios.** Aprovado de ouvido em
2026-08-16: um minuto contínuo de taps, 74 transientes, 74 sorteios.

```
piezo/linha → src_mix → gate → DELAY → VOCODER → fx2_gain → level
            → governor → shifter SSB → limiter → OUT
```

| Recurso | Medido |
|---|---|
| CPU (delay + dinâmica + vocoder) | ~1650 µs de pico / 5333 (31%) |
| Pool SRAM dos módulos | 45 432 / 49 152 B = **92,4%**, 0 falhas |
| SDRAM do vocoder | 65 544 B (tabela de seno `RNBODefaultSinus`) |
| QSPI | ~215 KB / 7 936 KB = 2,7% |
| Bloco | **256** (5,3 ms de latência) — exigido pelo vocoder |
| Loop principal | ~9,7 M de voltas em 63 s (sem `DelayMs`) |

---

## ✅ Estado do repo é INSTRUMENTO

Resolvido em 2026-08-17. A dinâmica voltou e a placa **sobe tocando**:

| | |
|---|---|
| `level` 1.00 · `in_gain` **8×** | `gate_thresh` 0.02 · `gov_ceiling` 0.85 |
| `rev_mix` 0.35 · `fx2_hp` 120 Hz | `fx2_on` 1 · `buzios_on` 1 · `buzios_thresh` 0.04 |

`shift_hz` segue em 0: é anti-feedback do laço piezo→corpo→piezo, e liga
quando o **exciter** entrar no caminho — antes disso só metaliza o timbre.

> **Quando o exciter entrar, revisar `level` e `gov_ceiling` juntos.** Lá
> não há atenuador: o TPA3116 recebe o que a placa mandar, e os dois
> formam o par que decide onde o laço para de crescer.

---

## Versões

| Arquivo | Versão |
|---|---|
| `hal.h` / `hal_daisy_seed.cpp` | v0.12.0 / v0.10.0 |
| `main.cpp` | **v0.40.0** |
| `manifest.h` | v0.41.0 |
| `params.h` | v0.6.1 |
| `module_rnbo_vocoder.cpp` | v0.6.0 |
| `reverb.h` | v0.2.0 |
| `module.h` / `dynamics.h` / `midi.h` | v0.4.0 / v0.4.0 / v0.2.1 |
| `TESTES.md` / `BOARDS.md` | v0.21.0 / v0.10.0 |

---

## Fechado em 18/08 — MIDI certificado sob carga

**T16c passou.** 2 779 CCs em 90 s com o vocoder a **65,1%** do prazo do
bloco: zero órfão, zero ressync, zero perdido, zero erro de UART. A marca
d'água da fila ficou em **32 de 128 bytes — idêntica com e sem carga**, o
que fecha com número o furo do `PollMidi` (bytes entram por DMA, mas quem
despacha é o loop). Os 32 bytes são o tamanho da rajada do BLE, não atraso
do loop: se fosse latência, teria subido com a carga.

**Regra revogada:** o pareamento BLE **sobrevive** ao reset. O
`gravar → reparear → testar` era superstição nascida de um log que eu
mesmo silenciei — caso 8 do battle_scars, com adendo no capítulo 11 e a
seção do BOARDS.md marcada como REVOGADA.

**Cicatrizes viraram skills.** `.claude/skills/`: `ifa10000`,
`medicao-limpa`, `instrumentacao`, `bug-de-ordem`, `edicao-segura`. O
IFÁ10000 ganhou a **etapa 0 — MEDIÇÃO LIMPA**.

---

## Ganho sonoro não planejado (18/08): o grave voltou

Aceite do dB, sem eu ter mexido em nada de timbre: **"presença de grave que
nunca teve antes, muito melhor sonoramente."**

**Causa, verificada no código:** o passa-alta de 120 Hz mora dentro do bloco
do Fx2. Vocoder fora ⇒ slot vazio ⇒ `fx2_want` falso ⇒ bloco pulado ⇒ **sem
filtro**. Ele existia só pra conter o estouro de grave que o vocoder
causava, e saiu junto com a causa porque foi instalado no escopo certo.

**Contribuição secundária, não medida:** `block_size` de 256 → 48 derruba a
latência de 5,3 ms pra 1 ms. Num instrumento realimentado isso desloca onde
o laço acústico se estabelece e aperta o transiente. Plausível e não
isolado — se quisermos o número, é um voo com uma variável.

---

## Defeitos abertos

**🔴 A serial morre com o vocoder ligado — prioridade 1.**
As rajadas de ~3,4 ms do callback starvam a ISR do USB; o CDC do host
apodrece. Contornado por `log_mute`, caixa-preta e MIDI, mas o conserto
certo está desenhado e não feito: **ring de log kick-and-forget**, em que
o loop nunca toca o USB e a drenagem acontece por ISR.

**🟡 Vocoder exige bloco 256.** 5,3 ms de latência num instrumento
realimentado desloca onde o feedback se estabelece. Preço aceito, não
resolvido.

**🟢 Off-by-one no contador de aplicadas.** `msgs completas N` vem sempre
**exatamente 1** acima de `APLICADAS`, com e sem carga (65/64 e
2779/2778). Diferença fixa, não taxa de perda — provavelmente a mensagem
em trânsito no instante da leitura. Confirmar antes de chamar de cosmético.

**🟡 Pool SRAM a 92,4%.** Terceiro módulo não entra sem mudar a
estratégia de alocação (rotular alocações quente/fria antes de crescer).

---

## Fila congelada (espera hardware)

T4 (buffer + piezo) · T7e (gate com transiente real) · teste percussivo do
limiter · **solda do painel v2** (5 pots + 2 botões; B2 é candidato natural pro toggle dos
Búzios).

---

## Próximos passos, em ordem

1. **Decisão do dB:** quanto de proteção volta → grava o estado
   instrumento como default do repo.
2. **Ring de log** — mata o defeito prioridade 1 por arquitetura.
3. **Defaults de palco:** `voc_mix`, `voc_tone`, `buzios_thresh`,
   `buzios_cooldown`.
4. **Painel v2** quando o hardware chegar (T11 já testa por serial).
