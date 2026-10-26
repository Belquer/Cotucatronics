# OPERATION — Cotucatronics

> **State frozen on 22/08/2026** · firmware v0.130.0 · commit `6002a70`
>
> This folder is the **operation manual**: what you need to power up, play,
> address and fix the instrument. It is distinct from the *process* documents
> (`STATUS.md`, `battle_scars.md`, `FEATURES.md`, `CTOS.md`), which record how
> we got here.

## Index

| document | for what |
|---|---|
| **[PINOS.md](PINOS.md)** | pin map — **final authority** on what is connected where |
| **[CCMAP.md](CCMAP.md)** | the 76 CCs, with the real range of each param · **generated from the manifest** |
| **[HARDWARE.md](HARDWARE.md)** | power, input, output, WIDI, panel, power-up checklist |
| **[ARQUITETURA.md](ARQUITETURA.md)** | signal topology, CTOS, and all serial commands |
| **[PRESETS.md](PRESETS.md)** | the approved design + the workflow (Loopy builds, OLED operates) |
| **[ROTINAS.md](ROTINAS.md)** | `ud`, `bts`, `ppc`, `cp`, `cv` — what each one does |
| `esquematicos/` | SVGs, brief and BOM of the carrier board |
| `gerar_ccmap.py` | regenerates the CCMAP from `manifest.h` |

## How to use it in five minutes

**Power up:** PD powerbank → 15V decoy → switch. Check `board` on the serial: version,
memory and pins print at boot.

**Play from the iPad:** Loopy Pro, channel 1. The board responds and **mirrors back
on channel 2** — the Loopy screen tells the truth even after a flash or a CTOS
drop.

**If MIDI does not arrive:** `midi` shows bytes/msgs/applied. Zero bytes →
suspect the TX/RX crossover **before anything else** (see HARDWARE §4).

**If a widget is on the wrong CC:** `vistos` — the board listens, you sweep the
Loopy, and the inventory hands you the culprit on its own.

**If something drops on its own:** `ctos` — drops by load and by overflow are
reported **separately**, because they call for different fixes.

## The house rules

These cut across everything and hold for any future session:

- **One flight, one variable** — and list the variable's jobs before touching it.
- **Every measurement declares REGIME and UNIT.** RMS or peak? norm or real? which
  block? debut or permanent?
- **A delta between builds is not attributable.** Cost is measured by A/B in the same
  binary, toggling one parameter at a time.
- **No value in the domain means "no data".** Absence lives outside the
  domain — a separate flag or an impossible sentinel, never a zero that the data
  can itself produce.
- **The report prints the raw value and labels the interpretation beside it.**
- **The plausible hypothesis is the dangerous one.** The deciding test is the one that
  reads the source, not the one that confirms the hypothesis.
- **Always the surgical one** — measured in delta of *behavior*, not of effort.
  A shortcut charges 10× later.
- **New feature: acceptance belongs to the musician.**

## On the validity of this document

The numbers here were **extracted from the source** at `6002a70`, not from memory.
`CCMAP.md` is generated; `PINOS.md` was read from `hal_daisy_seed.cpp`.

⚠️ **The SVGs in `esquematicos/` predate panel v2.** The analog circuits
(piezo, WIDI, output, OLED) are still valid; **the pin annotations on them
are superseded** — `PINOS.md` is the authority. Regenerate once the panel is
soldered.

**If this document and the code diverge, the code is right.** `make
check-docs` covers the repo's docs; this folder is part of the `ud` routine.
