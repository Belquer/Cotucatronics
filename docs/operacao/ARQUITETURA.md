# SIGNAL ARCHITECTURE — Cotucatronics

> Firmware v0.130.0 · `6002a70` · Daisy Seed 2, BOOT_QSPI, pure C++

## Topology

```
p1 → in_gain_a → gate_a → [drive · phaser · warble] ─┐
p2 → in_gain_b → gate_b → [drive · phaser · warble] ─┤ (× piezo_mix)
     ├→ × dly_a/b → DELAY (1 instance) ──────────────┤
     ├→ × frz_a/b → FREEZE_A · FREEZE_B ─────────────┤→ sum → level → split
     ├→ × esp_a   → [SHIMMER_A ↺ SALA_A] ────────────┤
     ├→ × esp_b   → [SHIMMER_B ↺ SALA_B] ────────────┤
     └→ dry (both, whole) ───────────────────────────┘
```

**PARALLEL tail, not serial.** Each destination receives its own send from the
two chains. The serial tail was a choice, not a law — and the imagined law
blocked the per-effect send.

## What is per chain, what is shared

**Per chain (23 params):** `in_gain`, `gate_thresh/release`, `drv_on/mix`,
`phs_on/mix`, `wrb_on/mix`, `dly_on/mix`, `frz_on/vol/size`, **and the whole
space** — `rev_on/mix/size/damp`, `shm_on/mix/feedback/shift/tone`. Plus
`gov_*`, `bench_boost`, `exc_on`.

**Shared (22 params):** the tuning of the character effects (`drv_drive/
tone/sujeira`, `phs_rate/depth/fb/stages`, `wrb_rate/depth/irreg`), `dly_time`,
`piezo_mix`, `pre_level_ab`, `buzios_*`, `test_tone_*`, `route_*`, `shift_hz`.

**Musical reason for the two spaces:** a string gives a sustained tone, and pitch
shift over a definite pitch sounds harmonic; a drum gives a wide-spectrum
transient, and raising that an octave turns metallic and smears the attack.
**Each material goes to the space that reveals it.**

## Protection — CTOS

| concept | value | nature |
|---|---|---|
| **CEILING** | 850 µs | administrative budget, **chosen** |
| **DEADLINE** | 1000 µs | physical wall, imposed by the hardware |

> Confusing the two makes the protection create the defect it should avoid.
> **Every safeguard declares which of the two it aims at.**

- **Ceiling**: N consecutive blocks above 850 → drops an effect by priority.
- **Deadline**: 3 overruns of 1000 µs in 2 s → drops. Constants **chosen,
  not measured** (zero overruns observed on the bench).
- Testable both ways: `ctos prazo <us>` and `ctos teto <us>` force artificial
  values. *Unexercised protection is supposed protection.*

**Regimes measured (v0.123.2, reader fixed):**

| regime | mean | peak |
|---|---|---|
| everything off | 183 | 262 |
| drive 48× A+B, reverb A+B, shimmer A | 536 | 687 |
| + dirt, shimmer B, delay, phaser | 697 | 992 |
| everything ON | 818 | ~975 |

⚠️ **2.5% of slack at the peak with everything on.** It is not designed use (two
shimmers contradict the premise of the spaces), but it is reachable.

## Anti-feedback

`governor` + `shifter SSB`, **per branch** (duplicated when the second exciter
came in). `shift_hz` is **shared for acoustic reasons**: two exciters on the same
body with different shifts would beat.

---

# SERIAL COMMANDS

| command | what it does |
|---|---|
| `list` | all params with current value |
| `set <name> <value>` | writes in **real units** (not normalized) |
| `board` | board, version, memory, pin map |
| `cpu` | mean and peak per regime · `cpu reset` |
| `ctos` | protection state, drops, overruns · `ctos rearm` / `prazo` / `teto` |
| `midi` | traffic, applied, discarded, `cc_janela` |
| `vistos` | **addressing inventory** — what arrived and from which CC |
| `sync` | pushes the 76 params to Loopy through the mirror |
| `hist` | level histogram (input and post-gain) |
| `bb` / `bb arm` | black box |
| `b1` `b2` `b3` | simulates the buttons |
| `tone on` / `tone off` | test tone |
| `pitch` `pk` `dyn` `mscope` `ble` `log` | diagnostics |
| `qspi vida` / `qspi id` | probe of the ITCM window |
| `dfu` | enters flashing mode |

**`vistos` is the addressing method** — the board listens and reports; dB sweeps
the Loopy. It beat `learn` (the board announcing) because it **does not require
the board to be deaf**, and it was what flagged four widgets carrying another
parameter's number.
