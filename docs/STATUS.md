# STATUS — Cotucatronics

**Onde estamos.** O par deste documento é
[battle_scars.md](battle_scars.md), que diz *o que aprendemos chegando
aqui*. Leia os dois antes de tocar em código.

> Atualizado em 2026-08-17 · último commit desta leva: `7ba5744`

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

## ⚠️ Estado do repo é BANCADA, não palco

A dinâmica está **neutralizada** — foi o A/B honesto com o Max, que não
tem nada disso no caminho:

| Parâmetro | Bancada | Instrumento (a decidir) |
|---|---|---|
| `gate_thresh` | 0 | ? |
| `shift_hz` | 0 | 4 Hz? |
| `gov_ceiling` | 1.0 (fora) | 0.5–0.8? |
| `level` | 0.4 | 0.7? |

**Decisão pendente do dB:** quanto de proteção volta, e a que custo
tímbrico. Cada bloco tem preço conhecido — governor comprime o vocoder
(que sai 11× mais quente que a entrada), shifter borra harmônicos, gate
corta as caudas do piezo.

Isso **viola a regra da casa** ("repo versiona o instrumento; bancada é
estado transitório") de propósito e temporariamente: duas sessões de
diagnóstico moram neste estado, e versioná-lo custou menos que perdê-lo.
Assim que a decisão vier, o default volta a ser o instrumento.

---

## Versões

| Arquivo | Versão |
|---|---|
| `hal.h` / `hal_daisy_seed.cpp` | v0.10.0 / v0.15.0 |
| `main.cpp` | v0.29.0 |
| `manifest.h` | v0.28.0 |
| `params.h` | v0.6.1 |
| `module_rnbo_vocoder.cpp` | v0.6.0 |
| `module.h` / `dynamics.h` / `midi.h` | v0.4.0 / v0.4.0 / v0.1.0 |
| `TESTES.md` / `BOARDS.md` | v0.16.0 / v0.7.0 |

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

**🟡 Pool SRAM a 92,4%.** Terceiro módulo não entra sem mudar a
estratégia de alocação (rotular alocações quente/fria antes de crescer).

---

## Fila congelada (espera hardware)

T4 (buffer + piezo) · T7e (gate com transiente real) · teste percussivo do
limiter · T10b (loopback MIDI, precisa de jumper D13→D14) · **solda do
painel v2** (5 pots + 2 botões; B2 é candidato natural pro toggle dos
Búzios).

---

## Próximos passos, em ordem

1. **Decisão do dB:** quanto de proteção volta → grava o estado
   instrumento como default do repo.
2. **Ring de log** — mata o defeito prioridade 1 por arquitetura.
3. **Defaults de palco:** `voc_mix`, `voc_tone`, `buzios_thresh`,
   `buzios_cooldown`.
4. **Painel v2** quando o hardware chegar (T11 já testa por serial).
