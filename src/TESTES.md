# TESTES.md — Cotucatronics OS · bench script

## LED USR — what it is saying

The board speaks through the LED at two moments, with **the same grammar**: it
counts N blinks, pauses, and repeats.

### Boot refusal — the stop loop

If `CheckRequirements()` fails, the board **refuses to boot on purpose** and the
LED counts the code of each missing requirement, cycling through them:

| blinks | missing requirement |
|---|---|
| **1** | audio input — the instrument requires it, the board does not have it |
| **2** | insufficient large RAM |
| **3** | fewer knobs than the manifest requires |

⚠️ **Nothing else works in this state** — the main loop never starts, so there
is no serial to ask. But **the error text repeats** every three cycles:
connecting the serial at any moment shows the reason.

> It cost half an hour on 08/31. `min_knobs` said 4, the panel has 2, and the
> board refused to boot — correctly. But it blinked a constant 5 Hz
> (indistinguishable from a hang) and printed the error **once**, at boot, for a
> host that was not yet listening. **A boot alarm that gets lost, in a form with
> no "after".**

### Loop phase — when it is running

With the instrument alive, the same LED counts the **phase** the loop is in. It
runs in the audio callback, and so it **survives a dead loop** — which is what
addressed the `PollMidi` *livelock* on 08/28.

| blinks | phase |
|---|---|
| 1 | knobs |
| 2 | `PollSerial` · encoders · OLED mirror |
| 3 | panel buttons |
| 4 | draw trigger |
| 5 | draw ramps |
| 6 | `PollMidi` · LbTick |
| 7 | MIDI mirror, end of loop |

**How to tell the two apart:** the boot refusal never goes past **3** and the
board is dead to commands; the phase reaches **7** and the board responds.


**Document v0.25.0** · last run on board: 2026-08-18 · firmware
**v0.47.0 / HAL v0.11.0** on the QSPI. **T1, T2, T3, T5, T6, T7 and T8 passed**; T4 awaits
hardware, and T7e (gate with piezo) waits alongside. **T10a passed natively;
T10b awaits a jumper.**

Minimal script to say "the board is alive and the fake panel responds" before
touching DSP. T1–T3 run with USB only; T4 awaits hardware; T5 is the acceptance
criterion of the first external module; T10 is the one for MIDI.

> **The native suites run without a board and without flash:** `./test.command`
> validates `dynamics.h` and `midi.h` on the Mac in a second. They do not
> replace testing on the board — none of them touch hardware — but they avoid
> spending a flashing cycle to discover an arithmetic or state-machine error.

---

## Validated environment

| Item | Version |
|---|---|
| Board | Daisy Seed 2 (DFM) — detected by `CheckBoardVersion()`, PCM3060 codec |
| Toolchain | ARM GNU Toolchain 15.3.rel1 (`arm-none-eabi-gcc 15.3.1`) |
| libDaisy | `~/dev/libDaisy` (`libdaisy.a`, 7.6 MB) |
| dfu-util | 0.11 |
| Bootloader | Electrosmith `dsy_bootloader_v6_4-intdfu-2000ms.bin` (comes with libDaisy) |
| Firmware | Cotucatronics OS v0.5.0 / `hal_daisy_seed v0.4.0` · `APP_TYPE=BOOT_QSPI` |

Firmware v0.4.1 footprint (with external module and CPU meter):

| Region | Used | Total | % |
|---|---|---|---|
| FLASH (internal) | 0 B | 128 KB | 0.00% — it belongs to the bootloader now |
| QSPIFLASH | 123 784 B | 7 936 KB | **1.52%** |
| SRAM | 71 100 B | 512 KB | 13.56% |
| SDRAM | 64 MB | 64 MB | 100.00% (the whole `sdram_pool` — see note at the end) |

> **Before the migration the same firmware occupied 123 392 B / 128 KB = 94.14%
> of the internal flash.** A single external module ate almost everything; the
> second would not fit. Hence `APP_TYPE=BOOT_QSPI`.

> The Homebrew toolchain (`brew install arm-none-eabi-gcc`) **does not work**:
> it comes without newlib, and libDaisy dies at `stdint.h: No such file or directory`.
> Use the `gcc-arm-embedded` cask (asks for an admin password).

## How to run

### Single step: flash the bootloader (only the first time per board)

The bootloader lives in the internal flash, like any normal app — so it is
flashed over the **STM DFU**: hold **BOOT**, tap **RESET**, release BOOT.

```bash
make program-boot
```

This is **once per board**. After that the internal flash belongs to the
bootloader and is not touched again, except to update the bootloader itself.

### Normal cycle: build and flash the app to the QSPI

```bash
make -C ~/dev/libDaisy          # once
make BOARD=daisy_seed
# tap RESET and flash DURING the grace window (LED pulsing, ~2 s).
# To extend the window indefinitely: hold BOOT (the LED responds with
# fast blinks).
make program-dfu               # the APP_TYPE already points to the QSPI address
./cotuca_serial.py --send board list mods --gap 1.3
```

**What changed relative to the old flow:**

| | Before (`BOOT_NONE`) | Now (`BOOT_QSPI`) |
|---|---|---|
| Enter flashing mode | BOOT + RESET (STM DFU) | RESET only, and flash in the ~2 s window |
| Where the app ends up | internal flash, 128 KB | QSPI, 8 MB |
| Size ceiling | 128 KB | 7 936 KB |
| USB PID in DFU | `0483:df11` (STM) | `0483:df11` (Daisy bootloader) |
| Boot time | immediate | +2 s of grace window |

`make program-dfu` always ends with `Error 74` on the `leave request`: the board
jumps to the app and stops responding to the last `get_status`. If
`File downloaded successfully` appeared, it flashed.

### Back to internal flash

```bash
make clean && make APP_TYPE=BOOT_NONE
# BOOT + RESET + solta BOOT
make APP_TYPE=BOOT_NONE program-dfu
```

This **erases the bootloader** (the app goes back to occupying the internal
flash) — to go back to QSPI you have to redo `make program-boot`.

---

## T1 — LED blinks 1×/sec · ✅ PASSED

**Procedure:** flash, look at the USER LED.
**Expected:** ~1 Hz. Blinks **fast** (10 Hz) = `CheckRequirements()` failed the board.

**Result:** blinking slowly. Also confirmed by the serial: the boot prints
`pronto.` instead of entering the fast-blink loop, i.e. the Seed 2 satisfies
`needs_audio_in`, `min_knobs 4` and the RAM the manifest requires.
In v0.3.0 the LED became a counted heartbeat (toggle every 500 ticks of ~1 ms),
and the 1 Hz rhythm held.

## T2 — fake panel over serial 115200 · ✅ PASSED

**Procedure:** `board`, `list`, `k1 0.5`, `list`.

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

Sample rate and block size match the `BoardInfo`. The normalized→real
translation of the manifest checks out: `0.40` in `[80, 2000]` = 848 Hz.

## T3 — audio on AUDIO_OUT_L · ✅ PASSED

**Procedure:** cable on **pin 18 (AUDIO OUT L)** and **pin 20 (AGND)**;
`tone on`, `k2 0.1875`, `k2 0.8`, `k5 0.5`, `tone off`.

**Firmware side — passed:**

```
> tone on     < tone ON            (tone_mix 0.00 → 0.50)
> k2 0.1875   < knob 2 -> 0.19     → tone_freq real = 440.00 Hz
> k2 0.8      < knob 2 -> 0.80     → tone_freq real = 1616.00 Hz
> tone off    < tone OFF
```

Conversion to the target pitch, with `tone_freq` in `[80, 2000]`:
`norm = (Hz − 80) / 1920`. So **440 Hz = 0.1875** and 880 Hz = 0.4167.

**Ear side — passed.** Continuous, clean tone in the retest with 3 s per pitch
(848 → 440 → 1616 → 440 Hz), no click, dropout or distortion. The first pass had
sounded like a short beep; that was partial observation, not a defect.

> **`k5` / SRC MIX came into existence in v0.3.0.** `list` shows `src_mix` at
> index 4 and `k5 0.5` moves from 0.00 → 0.50. The `IN_L`↔`IN_R` crossfade is in
> the callback, but will only be audible with both inputs connected (T4).

## T4 — piezo / exciter / routing toggle · ⏸️ BLOCKED

Awaits the high-impedance buffer, the TPA3116 and the panel. Nothing to run.

## T5 — external module "delay" · ✅ PASSED

**The acceptance criterion of the first module.** Compiling is not evidence: the
patcher may be mounted and dead. Here the case was closed by measurement, not
just by ear — see T5b.

**Prerequisite:** bootloader flashed (`make program-boot`, once) and firmware on
the QSPI (`make program-dfu`). Both done on 2026-08-15; run with firmware
v0.4.1 / HAL v0.4.0.

### T5a — parameter discovery (serial, no audio)

```bash
./cotuca_serial.py --send board list mods --gap 1.5
```

**Expected:**

- At **boot**: `modulo "delay": 2 params, 2 ligados`, and **no**
  `AVISO: param … nao existe no modulo`.
- `list` shows **7** parameters — the 5 system ones plus `dly_time` and
  `dly_mix`, both `norm=0.50`.
- `mods` shows the module, the two params **discovered by name**
  (`[0] dly_time`, `[1] dly_mix`), the two manifest connections with index
  `≥ 0`, and the memory statement with **`0 falhas`**.
- The boot prints `SDRAM via AllocBig: N de 67108864 bytes` with **N > 0** —
  it is the delay line. Before the module that number was zero.

**Failed if:** index `-1` on some connection (name diverging from
`description.json`), `falhas > 0` in the statement (SDRAM exhausted), or
`ERRO: modulo "delay" falhou no Init` (the firmware falls to passthrough and the
audio comes out dry, which is **not** the test passing).

**Result — passed:**

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

7 parameters, connections on `[0]` and `[1]`, no `AVISO`, 15 blocks and
**0 failures**.

### T5b — sonic confirmation

Cable on **pin 18 (AUDIO OUT L)** and **pin 20 (AGND)**.

| # | Command | Expected at the ear |
|---|---|---|
| 1 | `tone on` · `k2 0.7` | sine **echoing in a decaying train** — the patch's internal feedback is 0.4, so each repetition comes out at ~40% of the previous one and the tail dies on its own in a few echoes |
| 2 | `k1 0.0` → `k1 0.5` → `k1 1.0` | the **time between echoes sweeps audibly**, from short to long. Sweeping with the echo playing gives the glissando typical of modulated delay time |
| 3 | `k2 0.0` | sound **goes dry**: only the direct sine, no tail |
| 4 | `tone off` | silence after the tail drains |

`k1` → `dly_time`, `k2` → `dly_mix`, `k3` → `level` (see `manifest.h`).

**Failed if:** single echo instead of a train (feedback did not arrive), a tail
that **grows** instead of decaying (feedback > 1, danger to the exciter — cut the
volume), clicks at each change of `k1` (delay interpolation), or `k2` with no
effect at all (parameter not connected — go back to T5a).

**Result — passed, and measured instead of only heard.**

An echo does not change pitch, so a frequency FFT does not prove delay. What
proves it is the **tail that remains after cutting the source** — if the signal
persists with the source silent, it is coming out of the delay line and nowhere
else:

| Condition | Level 1 s after `tone off` |
|---|---|
| `dly_mix = 0.0` | silence (−94 dBFS) |
| `dly_mix = 1.0` | **848 Hz at −33.7 dBFS** |
| `dly_time = 0.05` | tail already extinguished within the measurement window |
| `dly_time = 0.9` | −27.2 dBFS |

With the tone on and 100% wet: 848.23 Hz at −23.4 dBFS; right after the cut,
848.01 Hz at −33.8 dBFS (−10.4 dB of decay); 3 s later, silence.

The pair `dly_mix = 0` → dry / `dly_mix = 1` → tail is the control that closes
the entire chain: **ParamTable → connection by name → the engine's
`setParameterValue` → audible change**.

### T5c — CPU

```
> cpu
< cpu: 5.7% media, 9.9% pico (bloco de 48 @ 48000 Hz)
```

**Constant** at 5.7% / 9.9% with any combination of `dly_time` and `dly_mix`,
and the same at rest and with the tone passing. It was expected: the delay line
reads and writes every sample regardless of the setting, so the load does not
depend on the parameter. Deterministic load is what you want on stage — but it
means that **the module's cost does not drop when it is "off"**.

## T6 — parameter curve and serial `set` · ✅ PASSED

**Procedure:** turn `k1` (→ `dly_time`, `curve 2.5`) across the whole travel and
check the `real` in `list`; then exercise `set <nome> <valor>`.

### T6a — the curve matches the math

`real = pow(norm, 2.5)`, measured at the five points:

| `k1` | Predicted | Measured |
|---|---|---|
| 0.00 | 0.000 | 0.00 |
| 0.25 | 0.031 | 0.03 |
| **0.50** | **0.177** | **0.18** |
| 0.75 | 0.487 | 0.49 |
| 1.00 | 1.000 | 1.00 |

Half of the knob's travel covers the first ~18% of the range — the short delays
gained half of the useful travel, which was the objective.

### T6b — the CURVED value reaches the DSP

The test that matters: it is not enough for `list` to show the right number, the
engine has to receive that number. Measuring the tail 1 s after cutting the
source (440 Hz, `dly_mix = 1.0`):

| `k1` | `dly_time` real | Tail |
|---|---|---|
| 0.5 | 0.177 (short) | −28.7 dBFS |
| 0.9 | 0.768 (long) | **−7.2 dBFS** |

21.5 dB of difference: a short delay repeats more times per second and the energy
decays faster. If the module were receiving the raw normalized value (0.5 and
0.9), the separation would be much smaller.

> **The trap this catches.** The push to the module used `params.Get()` — the
> **raw** normalized value. With the curve only inside `Real()`, it would stay
> stuck on our side and never reach the engine: the symptom would be "the curve
> does nothing", with no error, no log, no clue. The push now uses `Real()`.
> **Every future parameter inherits this:** what reaches the module is `Real()`,
> and that is where any manifest transformation takes effect.

### T6c — `set <nome> <valor>`

```
> set tone_freq 440      < tone_freq -> real=440.000 (norm=0.188, curve 1.0)
> set dly_time 0.5       < dly_time -> real=0.500 (norm=0.758, curve 2.5)
> set naoexiste 0.5      < ? param "naoexiste" nao existe (use list)
```

Reaches any parameter by name **in real units**, shows the applied curve and
refuses a nonexistent name or a value out of range. `tone_freq` and `tone_mix`
are **serial-only by design** — knobs 1–3 went to the delay and the level, and
the test tone is bench, not performance.

## T7 — shell dynamics chain · ✅ PASSED (with a caveat on the gate)

Gate → [module] → level → split → `OUT_L`: governor → [bench_boost] → limiter ·
`OUT_R`: limiter. Firmware v0.6.3 / dynamics v0.3.1.

### T7a — the governor compresses, and compresses as predicted

440 Hz tone, `dly_mix 0`, level outside the ADC clip region:

| `gov_ceiling` | gain | measured output | predicted |
|---|---|---|---|
| 1.0 | 1.00 | −9.0 dBFS | — |
| 0.02 | 0.39 | −17.2 dBFS | **−8.2 dB** |

Measured reduction **8.2 dB**, matching `20·log₁₀(0.39)` to the tenth, repeatable
twice.

> **Measurement trap.** On a first attempt the measured value (6.7 dB) did not
> match the predicted one (11.1 dB). It was not the governor: `cotuca_measure.py`
> reports **RMS**, and for a sine the peak sits 3 dB above — the reading of
> −1.1 dBFS RMS meant a peak at **+1.9 dBFS**, i.e. the interface's ADC was
> clipping and the high reading came out underestimated. Lowering the level, the
> numbers closed. Every level measurement here needs a peak below 0 dBFS, not
> RMS.

### T7b — the limiter holds, and the math closes

With `bench_boost` (a bench tool) pushing the signal against the ceiling:

| `bench_boost` | internal | measured `gainL` | predicted `0.95/env` |
|---|---|---|---|
| 1× | 0.25 | 1.00 | 1.00 |
| 2× | 0.50 | 1.00 | 1.00 |
| 4× | 1.00 | **0.96** | 0.95 |
| 8× | 2.00 | **0.48** | 0.475 |

The output latches from 4× onward while the input doubles.

**Distortion — measured natively**, because on the board the ADC clips first:

| Overload | Output peak | THD |
|---|---|---|
| 1.1× | 0.95000 | 0.13% |
| 2× | 0.95000 | 0.13% |
| 4× | 0.95000 | 0.13% |
| 8× | 0.95000 | **0.13%** |

**Distortion does not grow with overload.** In steady state the hard clamp never
acts: the smoothed gain has already brought the signal below the ceiling, and
what comes out is a clean sine merely scaled. The clamp only catches the initial
~1 ms transient, which is its backstop role.

> **Caveat:** that is steady state. **Percussive** material — which is exactly
> the piezo with taps — hits the clamp on every attack. When the piezo arrives,
> measure again with a real transient, not with a sine.

### T7c — routing, the two stages

```bash
python3 teste_roteamento.py
```

⚠️ **A WIRING PRECONDITION, and it is not optional: each output on its OWN
channel of the interface.** The script measures the map and **aborts** if the
two fall on the same channel — because that is exactly what invalidated the
first run (see below).

Six criteria, measured in v0.178.0 · `OUT_L → ch1`, `OUT_R → ch2` ·
useful separation 59.6 dB · floor −78.2 / −81.5 dBFS · base −32.6 / −36.7:

| criterion | OUT_L | OUT_R | |
|---|---|---|---|
| `route_l_a=0` cuts A in L, and **only** in L | −44.8 | +0.0 | ✅ |
| `route_r_a=0` cuts A in R, and **only** in R | −0.0 | −43.9 | ✅ |
| `out_l=0` cuts the L branch, and **only** L | −44.1 | +0.0 | ✅ |
| `out_r=0` cuts the R branch, and **only** R | +0.0 | −41.5 | ✅ |
| **inversion**: A only in L → A only in R | −32.6 → −77.4 | −80.9 → −36.7 | ✅ |
| the delay **survives** `route_r_a=0` | — | −40.5 against a −81.5 floor | ✅ |

The last one is what separates the stages: the matrix removes the dry, the freeze
and the space of chain A, but the delay's return stays **41 dB above the floor** —
a single instance, sends already summed, and nothing in it knows which chain it
came from.

> **THE FIRST RUN HAD TWO EMPTY CRITERIA.** The two outputs arrived summed at a
> single input (coherent sum confirmed to the hundredth: −22.1 and −19.1 dBFS
> give −14.45, measured −14.5). With `OUT_L` not reaching the interface,
> *"L does not affect R"* gave +0.0 dB **whatever the firmware was**.
> **A criterion that cannot fail is not a criterion** — and it passed, with the
> look of proof.
>
> **And the map came out CROSSED on the first setup** (`OUT_L → ch2`). That is
> why the script MEASURES it instead of assuming it: a swapped index does not
> produce silence, it produces a verdict on the wrong side.

### T7d — CPU

| Configuration | Average | Peak |
|---|---|---|
| No chain (v0.5.0, only the external engine's delay) | 5.7% | 9.9% |
| At rest, chain loaded | 10.2% | 20.5% |
| **Everything engaged** (gate open + governor compressing + boost 8× + limiter acting + delay) | **11.0%** | **20.5%** |

The chain cost ~5 points of average. Note that engaging everything costs almost
nothing more than resting: the three blocks run the detector and the gain
**always**, active or not. Deterministic load — good for the stage, but the cost
does not disappear when the block is "off".

### T7e — gate · ⏸️ PARTIAL

Closes correctly with the floor at digital silence (`gain=0.00`, `env=0.0001`) and
opens with signal (`gain=1.00`, `env=0.47`). But the two criteria that matter —
*silencing a real noise floor* and *opening on a tap without eating the
transient* — need the piezo. With the input disconnected there is no floor to
gate.

The native test covers the logic: closes at −60 dBFS, opens in 10.1 ms, and lets
**9.5% of the signal through in the first millisecond**. That last number is what
is worth discussing when the piezo arrives: with a 10 ms attack the gate **eats
part of the transient**. For a dry tap on metal it may sound like a loss of
attack. The fix would be a ~1 ms attack, and the detector is already fast enough
(0.5 ms) to sustain that without a false trigger.

## T8 — SSB shifter (anti-feedback) · ✅ PASSED

Fourth permanent block of the stack, in C++ of the shell. Exciter branch, between
the split and the governor. Firmware v0.9.0 / dynamics v0.4.0.

**This is the perfect test for the FFT meter**: the shift is literally a
frequency, so the acceptance criterion is directly measurable instead of
inferred.

### T8a — the shift happens, and it is the requested value

440 Hz tone, `set shift_hz 5`, measured on the exciter branch:

```
ch2:  445.12 Hz   -7.4 dBFS   erro +0.12 Hz (+0.5 cents)
```

**445.12 Hz** = 440 + 5, with the same +0.12 Hz clock drift that shows up in
every measurement since T3. Linear shift in Hz, not an interval.

### T8b — lower sideband suppressed

Target FFT at three frequencies, 3 s of capture, Hann window:

| Component | Level | vs. upper band |
|---|---|---|
| **445 Hz** (upper band) | −4.3 dBFS | — |
| 440 Hz (carrier) | −101.0 dBFS | **−96.7 dB** |
| 435 Hz (lower band) | −54.9 dBFS | **−50.5 dB** |

**50.5 dB of suppression**, against the 30 dB requested — and matching *exactly*
the 50.5 dB predicted natively at 440 Hz. The original carrier practically
disappeared (96.7 dB down): the SSB is clean.

Native profile across the whole band:

| Tone | Suppression |
|---|---|
| 100 Hz | 55.7 dB |
| 440 Hz | 50.5 dB |
| 5 kHz | 46.8 dB |

### T8c — CPU

| Configuration | Average | Peak |
|---|---|---|
| No shifter (v0.8.0) | 9.6% | 20.4% |
| **With shifter** | **10.7%** | **23.6%** |

**~1 point of average**, below the predicted 3–6% budget. Constant for any
`shift_hz` — the filter runs always, like the other blocks of the stack.

The cheapening comes from the quadrature oscillator in **coupled form** (4
multiplications per sample) instead of two `sinf`/`cosf` per sample.

### T8d — jack without shift · ✅ PASSED

Cable moved to **pin 19** (`OUT_R`), same 440 Hz tone and same `shift_hz 5`:

| Component | Level | vs. 440 |
|---|---|---|
| **440 Hz** (original) | −4.4 dBFS | — |
| 445 Hz (upper band) | −111.1 dBFS | **−106.7 dB** |
| 435 Hz (lower band) | −108.7 dBFS | −104.3 dB |

**106.7 dB of rejection** — that is the noise floor, not partial suppression. The
jack comes out absolutely clean, with no trace of the shift. The split is real.

### T8e — routing, both sides

With the cable on pin 19:

| Routing | Measured on `OUT_R` |
|---|---|
| exciter=1, jack=1 | 440.12 Hz, −7.4 dBFS |
| exciter=1, jack=0 | silence |
| exciter=0, jack=1 | 440.12 Hz, −7.4 dBFS |

Exact mirror of the test on pin 18: each branch responds only to its own `route`,
and muting one does not touch the other.

> **A bench error that nearly became a wrong conclusion.** The first round of
> this test used `set -- $cfg` in a shell loop to separate the pairs. **zsh does
> not do word-splitting** on an unquoted expansion, so `$1` became `"1 1"` and
> `$2` ended up empty — the command sent was `set route_jack` with no value,
> which `strtof` reads as **0**. The jack was muted throughout the whole test.
>
> The earlier conclusion (that `route_exciter` — today `out_l` — controls the
> channel measured on pin 18) was still correct, but **by accident**. Redone with
> explicit arguments.
>
> It is worth the lesson for the script: a test that gives the expected result
> deserves the same distrust as one that fails — here it was the *unexpected*
> silence on pin 19 that exposed the bug, and it had been there since the
> previous round.

> ### The trap that nearly got through: `a` vs `a²`
>
> The first version of the shifter gave **18.9 dB** of suppression, degrading
> toward the middle of the band (60 dB at 100 Hz, 19 dB at 5 kHz). I tested the
> four combinations of delay and sign — the best stopped at 19 dB. I tested two
> forms of the allpass section — no improvement.
>
> The cause: **the classic published coefficients are `a`, and the section uses
> `a²`.** Squaring took it from 18.9 to 46.8 dB at 5 kHz. It was not a structure
> error nor a sign error — it was a convention undocumented in the coefficient
> set.
>
> **If I had flashed without measuring, the anti-feedback would have had half the
> suppression and no one would have found out until a sculpture squealed on
> stage.** It is the strongest argument this project has in favor of measuring
> before flashing.

---

## T10 — MIDI: parser, map and UART loopback · ⚙️ NATIVE PASSED · ⏳ BOARD PENDING

**The validation is split in two on purpose, because the two halves prove
different things and only one of them runs without hardware.**

| | What it proves | What it does **not** prove |
|---|---|---|
| **T10a** — native suite | parser, manifest map, normalization, curve | nothing about the UART |
| **T10b** — loopback with jumper | UART, baud rate, pin, DMA, queue, ISR | — |

A perfect parser fed by a UART configured at 9600 baud receives no byte at all,
and T10a would pass just the same. That is why T10a is **not** the acceptance
test — it is what lets you reach the bench with the parser already known to be
correct.

### T10a — native suite · ✅ PASSED

```bash
cd src && ./test.command      # ou: clang++ -std=c++14 -O2 -I. test_midi.cpp -o /tmp/t && /tmp/t
```

`midi.h` and `manifest.h` are pure C++ and board-agnostic, so the state machine
**and the complete CC→ParamTable path** run on the Mac.

**Result: 16 checks, 0 failures.** The ones that matter:

```
OK   status omitido: 3 CCs de um B0 so                     n=3 ultimo val=127
OK   clock entre os data bytes nao quebra o CC             n=1 val=64 realtime=1
OK   nota/bend/pgm/pressure engolidos, CC seguinte intacto n=1 cc=21 val=32 outras=4
OK   corpo de sysex nao vira CC, e o CC seguinte funciona  n=2 cc=22 val=99
OK   dados sem status sao descartados e contados           n=1 orfaos=3 val=70
OK   CC 20 val 64 -> dly_time norm 0.50                    norm=0.5039 real=0.1803 (curve 2.5)
OK   CC 24 val 127 -> gov_ceiling em 1.00 (param sem knob) norm=1.0000
OK   CC e knob no mesmo param dao o MESMO valor            midi=1.0000 knob=1.0000
```

The second-to-last is the one that closes the acceptance criterion of the spec.
The last is the one that proves that **MIDI is a remote knob and not a new path**:
the same gesture by the two routes gives bit for bit the same value.

> **`real=0.1803` is not an error.** `dly_time` has curve 2.5, so
> `real = 0.5039^2.5 = 0.18`. `list` shows the two columns; the criterion is the
> `norm`. The manifest curve applies to MIDI **exactly as it applies to the
> knob** — that is the point, not a side effect.

### T10b — UART loopback · ⏳ PENDING

**Needs a jumper between pin 14 (D13/TX) and pin 15 (D14/RX).**
One wire, no solder. It is the only way to prove baud rate and pin without the
WIDI in hand.

```bash
cd src && ./cotuca_serial.py --send midi "midiloop 20 64" midi list --gap 1.5
```

**Expected:**

1. At **boot**: `MIDI: USART1 31250 8N1 | TX=D13(pino14) RX=D14(pino15)`
   and `MIDI: canal omni, 5 CCs mapeados`.
2. `midi` before: `uart ESCUTANDO | perdidos 0`, `bytes 0`, `cc 0`.
3. `midiloop 20 64` sends `B0 14 40` over the TX.
4. `midi` after: **`bytes 3 | cc 1`**, and in the history
   `cc 20 val 64 ch 1 -> dly_time`.
5. `list`: **`dly_time norm=0.50`**.

**Failed, and what each failure means:**

| Symptom | Where the problem is |
|---|---|
| `uart PARADA` at boot | the UART did not even initialize — peripheral conflict |
| `bytes 0` after the `midiloop` | the jumper: loose wire, or wrong pin |
| `bytes 3` but `cc 0` | parser — but T10a already covers that, so suspect the jumper first |
| `cc 1` but `dly_time` unchanged | the manifest map, or an id resolved before `Add()` |
| `perdidos > 0` | the queue of 128 overflowed — should not happen with 3 bytes |

### `miditest` — the shortcut, and what it costs

```
miditest 20 64
```

Injects the same three bytes **into the RX queue**, without going through the
UART. It serves to separate "the wire did not arrive" from "it arrived and the
firmware ignored it" without disassembling anything.

> **`miditest` does NOT replace `midiloop`.** It comes in after the wire: a wrong
> baud rate, a swapped pin or a misconfigured DMA would pass through it without
> complaint. The command itself warns about this in the response
> (`NAO passa pela UART`).
>
> And it writes to the same queue index as the RX ISR. With live MIDI coming in,
> the two writes race. **It is a bench hook with the door silent.**

### On the day of the WIDI solder

`midi` is the first thing to consult. It answers **at which step the signal
stopped**:

```
midi: uart ESCUTANDO | perdidos 0 | canal omni
  bytes 42 | cc 3 | outras 12 | orfaos 0 | realtime 24 | canal alheio 0
  parser: running status B0 | 0 byte(s) pendente(s)
  cc 20  -> dly_time
  ...
  ultimos CCs (mais recente primeiro):
    cc 20  val 64  ch 1  -> dly_time
```

- `bytes 0` → **check the TX↔RX crossover before anything else**
  (see [BOARDS.md](BOARDS.md)). Wiring it straight burns nothing and gives no
  error at all: no byte simply arrives, just like a module without power.
- `bytes > 0` but `cc 0` → junk is coming in, or only real-time. If
  `realtime` rises and nothing else, the controller is sending clock and not CC.
- `cc > 0` but `(fora do mapa)` in the history → the controller is sending CC,
  only another number. Adjust the `midi_map[]` in the manifest.
- `canal alheio > 0` → it arrived on the wrong channel. The default is omni, so
  this only shows up if someone fixed `midi_channel`.

## T11 — panel v2 over serial (buttons before the solder) · ⏳

Same strategy that validated the knobs before the pots existed: the commands
`b1` and `b2` simulate the buttons over the REAL path — same edge detection,
same randomizer, same ramps. What is left out is only the GPIO and the debounce.

### T11a — B1 randomizer

```bash
./cotuca_serial.py --send list b1 list --gap 1.5
```

**Expected:** `B1: sorteando 2 param(s) em 50 ms`, and `list` afterward shows
`dly_time` and `dly_mix` with values different from before.

**Whitelist:** only those two. If `level`, `src_mix`, `gate_*`, `gov_*`,
`route_*`, `shift_hz` or `bench_boost` change, **it is a grave defect** — the
whitelist exists so the button can surprise the timbre and never the safety nor
the identity of the instrument.

### T11b — the ramp does not click

With `tone on` playing, trigger `b1` repeatedly. The timbre should change with no
click. A 50 ms ramp: short for the gesture, long for the speaker.

### T11c — B2 hold

With `button2_target = -1` (current instrument), `b2 1` should warn that there is
no target. When the freeze goes back to Fx2 and the target is `p_frz_on`:
`b2 1` freezes, `b2 0` releases, and `list` shows the previous value restored —
not zeroed.

### T11d — knobs in the new drawing

| | Before | Panel v2 |
|---|---|---|
| k1 | dly_time | **level** |
| k2 | dly_mix | **src_mix** |
| k3 | level | dly_time |
| k4 | src_mix | dly_mix |
| k5 | src_mix | **free** |

`k5 0.5` should move nothing — the delay only has two parameters of character.

### What the serial does NOT prove

GPIO, pull-up and debounce. `b1`/`b2` come in after that. A button that bounces
or a swapped pin passes this test and fails on the panel — the hardware
validation is to hit the real button and watch the log.

## T12 — RETIRED

The spectral-coincidence trial left along with the effect that motivated it. The
two lessons it bought live where they serve any future effect: the proof material
per family is in step 6 of [IFÁ10000](../docs/IFA10000.md)
(*broadband is the ruler for cross-synthesis; a sine is the one for delay and
shifter*), and the cost of getting it wrong is in ch. 6 of
[battle_scars](../docs/battle_scars.md).

## T13 — BÚZIOS: draw by transient in A minor · ✅ PASSED

**Approved by ear by dB on 2026-08-16, one continuous minute of taps:
74 transients, 74 draws applied. Ratio 1:1.**

dB's name, and it describes the gesture better than any technical term: each hit
on the sculpture is a throw of cowrie shells, and what falls decides the timbre.
The A minor scale makes the luck always fall within the key — surprise with no
wrong note.

```bash
./cotuca_serial.py --send "bb arm" "set buzios_on 1" "set fx2_on 1" --gap 3
```

| Target | Draw |
|---|---|
| `dly_time` | 100–500 ms in **steps of 50 ms** (clamped in REAL) |
| Fx2's note card | new note in **A minor**, repetition accepted |
| `dly_mix` / Fx2's timbre card | [0.3–0.8] / [0.4–1.0] |

Forbidden by design: `level`, `src_mix`, gate, governor, limiter, routing, shift.
**The cowrie shell may surprise the TIMBRE, never the SAFETY nor the IDENTITY of
the instrument.**

### The hunt that T13 cost — the loop that kept dying

The buzios "worked" for 5 seconds and froze. There were nine diagnostic flights
until the killer had a name, and the method that solved it was to swap theory for
measurement at every step:

| Flight | Variable | Loop iterations | Death |
|---|---|---|---|
| complete | everything on | 1 505 | 12.6 s |
| lean | no `DelayMs`, minimal instr. | 8 748 683 | alive at 58.7 s |
| discriminant | `DelayMs` turned back on | 1 516 | 12.7 s |
| **fix** | `DelayMs` out | **9 711 576** | **alive at 62.9 s** |

**`hal::DelayMs(1)` at the end of the loop was the killer.** With the Fx2
module's callback holding the CPU in bursts of ~3.4 ms, the tick-based wait
swallows a tick and at some point never finishes. The timestamp is written
immediately BEFORE the delay: the loop got there at 12 674 ms and never came
back — it went in and did not come out.

Removing it is safe because the delay was CPU courtesy and the audio runs on
interrupt. The only thing that depended on the loop's cadence was the draw's
ramp — and it became a function of the clock in the previous patch. **Removing
the metronome stopped having a side effect at the exact commit where the
metronome stopped existing.**

(In dB's spec this test came in as "T12"; it became T13 because T12 was already
the spectral coincidence one.)

Each hit on the piezo draws:
- **a new delay time, uniform between 100 and 500 ms** (clamped in REAL,
  because the parameter's curve 2.5 would warp a draw in norm);
- **a new note of the Fx2 module in natural A minor** ({0,2,3,5,7,8,10} over
  110 Hz — which ALREADY is an A), repetition accepted: a memoryless draw;
- `dly_mix` [0.3–0.8] and Fx2's timbre card [0.4–1.0].

```bash
./cotuca_serial.py --send "set arand_on 1" "set fx2_on 1" --gap 2.5
```

**Expected, by ear:**
- each hit moves the echo (pitch slip in the delay's tail = expected and
  DESIRED) and jumps the Fx2 module's note;
- the note **never sounds "wrong"** — always within A minor;
- two hits within < 100 ms = ONE draw only (cooldown);
- `set arand_on 0` freezes everything (requires a live serial; with the module
  on it dies — RESET also freezes, arand_on is born at 0).

**Logging:** while the serial lives, each draw logs
`arand: dly_time=… <nota>=…`. After it, the **last 4 draws** stay in the black
box (slots 4–7: id in the integer part, norm in the fraction) and come out in
`bb` after the reset.

**Cost:** the detector is an envelope + per-sample comparison. Expected ~zero;
check in `cpu` that the peak did not leave the 63–64% range.

**Safety unchanged:** level, src_mix, gate, governor, limiter, routing and shift
remain OUT of any draw — the whitelist with fences replaces the simple list, not
the policy.

## T14 — REVERB + Fx2 high-pass · ✅ PASSED

Certified by [IFÁ10000](../docs/IFA10000.md) on 2026-08-17.

### T14a — reverb (Freeverb in the shell)

`rev_mix` 0.15 · `rev_size` 0.5 · `rev_damp` 0.5, all serial/MIDI.
**Proof material: transient + silence** — reverb on a continuous sine is almost
inaudible, because the tail only appears after the attack.

**Cost, and the scar it left:**

| Buffers (108 KB) | No reverb | With reverb | Cost |
|---|---|---|---|
| SDRAM (`AllocBig`) | 8.0% | 20.9% | **+12.9 pp** |
| **SRAM** (`.bss`) | 7.9% | **13.5%** | **+5.6 pp** |

**The access pattern matters more than the size.** It is 48 accesses per sample
spread across 24 delay lines — 2.3 M/s, the worst possible case for cache. The
same 108 KB in the right region cost less than half.

Peak with the Fx2 module: 71.3% (3800 µs of 5333), 29% of headroom.

**Out of the buzios on purpose:** the reverb is FRAME, not character. With
everything jumping on each hit, the constant room is what makes the throws belong
to the same place. *If everything jumps, nothing is a surprise.*

### T14b — Fx2 high-pass: the measurement that prevented the wrong fix

Symptom: "a bass is clipping". The peak-per-note histogram (pre-level,
pre-limiter) showed:

| Carrier | Peak |
|---|---|
| ~110 Hz | 1.417 🔴 |
| ~139 Hz | 1.478 🔴 |
| ~175 Hz | 0.756 |
| ~220 Hz | 0.704 |
| ~277 Hz | 1.485 🔴 |
| ~349 Hz | 0.549 |

**Two reading corrections worth more than the number.** First: the buckets
covered 4 semitones each, far too coarse — resolution doubled afterward. Second,
and more serious: the histogram groups by **carrier pitch** and measures **total
peak** — it says *when* the output peaked, not *at what frequency the energy is*.
Calling that "resonance" and prescribing a notch would have been surgery based on
a guess.

The high-pass then became **hypothesis and fix at the same time**: if it
flattened the histogram, it was bass energy; if not, the problem would be
per-note level imbalance and the filter would come out.

**It flattened.** `fx2_hp` = **120 Hz**, approved by ear ("no longer clips"). It
stays **on the Fx2 path**, not on the master — without the module there is no
clipping (measured), so filtering the master would punish the delay for free. And
120 Hz sits **below** the 174–220 Hz region that the histogram showed to be
healthy: cutting higher would take the body of the sound along with it.

## T15 — INSTRUMENT STATE: the board boots up playing · ✅ 2026-08-17

The real instrument, approved by ear by dB. **No serial command needed** — RESET
and play.

| Parameter | Value | How it was arrived at |
|---|---|---|
| `level` | **1.00** | full scale; the attenuator is on the sound card |
| `in_gain` | **8×** | top of the range, raised 1→2→4→8 by ear |
| `rev_mix` | **0.35** | played and approved value (0.15 was an unheard guess) |
| `buzios_thresh` | **0.04** | 0.15 was too hard for the real gesture |
| `gate_thresh` | 0.02 | restored |
| `gov_ceiling` | 0.85 | protection, not color |
| `fx2_on` / `buzios_on` | **1 / 1** | born on |

### ~~`in_gain` — post-source, PRE-GATE~~ 🗑️ REMOVED 08/28

> **The param went away, and this record stays.** The `Governor` is an AGC: 18 dB
> put on `in_gain` were 18 dB it took back further on, and the output did not
> change — the only audible effect was the step, because the gain was read per
> block and applied per sample. **If it does not sound, it does not stay.**
>
> The test below remains true about what it measured at the time; what changed was
> the instrument.

The oldest promise of this manifest, fulfilled in the place it itself reserved on
day one: *"the piezo's compensation gain does NOT live in the level: it lives
post-input / pre-gate"*.

**Why it is not the same as raising the `level`:** the level pushes the OUTPUT
against the governor and the limiter — more volume there is more compression,
which is the artifact you want to avoid. The input gain lifts the signal before
the entire chain, and the dynamics keep working in the same proportion.

**Pre-gate** because the gate needs to decide about the signal already at the
right level; after it, the threshold would mean different things depending on the
gain.

> ⚠️ **It is at the CEILING, and that is information.** At 8× the piezo's noise
> floor is 18 dB above where it was. If more is lacking, **raising the ceiling is
> the wrong path** — the right one is the high-impedance buffer
> ([BOARDS.md](BOARDS.md)), which lifts the signal before the ADC and improves
> the signal/noise ratio instead of multiplying both. The 08/16 conclusion ("the
> Daisy handles the piezo") remains valid *for the level it was at* — and that
> level changed meaning.

### The bypass rule, revoked by measurement

`fx2_on` was born at 0 because a heavy module that blows the deadline kills the
loop. The risk was **measured and ruled out**: the module at 37% of the deadline,
watchdog intact, and the loop death it was blamed for had another cause
(`DelayMs`, ch. 7 of the battle scars) — already fixed.

In place of the rule comes something better: **the watchdog**, which forces
bypass if the module really blows the deadline. *Protection by measurement, not
by abstinence.*

**Return condition, written in the manifest:** a new, unmeasured module in the
Fx2 slot → go back to bypass. The rule still holds for anyone who has not yet
been through [IFÁ10000](../docs/IFA10000.md).

## T10b — MIDI LOOPBACK · ✅ PASSED (2026-08-17)

Out of the frozen queue. Jumper **D13 → D14** and:

```bash
./cotuca_serial.py --send "mscope on" "midiloop 22 0" "midiloop 22 64" "midiloop 22 127" midi --gap 3
```

**Passed:** the three messages went out over the TX, came back over the RX and
traversed the real path — UART → DMA → ISR → queue → parser → map →
parameter. Hex matching the decoding, `level` going to 0.000 / 0.504 / 1.000.

| bytes | msgs | applied | orphans | resync | lost | UART errors |
|---|---|---|---|---|---|---|
| 395 | 113 | 112 | 0 | 0 | 0 | 0 |

**It is the test that cuts the system in half.** `mscope` measures what comes in
and therefore never distinguishes "the source did not send" from "it sent and was
lost"; the loopback uses a known source on the real path and gives a binary
answer. Every input channel should be born with its pair like this.

## T16 — real MIDI with the WIDI, by the scope · ✅ a/b/c PASSED

> **PREREQUISITE, and it explains all the false negatives:** the BLE pairing
> **does not survive the board's reset**. Always *flash → forget the device →
> pair → test*. See [BOARDS.md](BOARDS.md).

Reproducible by anyone with a USB cable. **Cold session first** —
`fx2_on 0` so the serial lives — then with the module, through the black box.

### T16a — cold session, fader sliding

```bash
./cotuca_serial.py --send "set fx2_on 0" "mscope on" --gap 2
```

dB pairs the iPhone and slides the Loopy fader. The scope tells the story, and
each outcome points to a different place:

| What shows up | Where the problem is |
|---|---|
| **nothing** | BLE did not pair, or UART/crossover (see the solder guide) |
| bytes arrive, `msgs completas` does not rise | parser stuck — see `ressync` and `órfãos` |
| msgs rise, `APLICADAS` does not | CC out of the map |
| `fila perdeu` > 0 | a burst overflowing the ISR queue |
| everything rises and the sound does not change | the problem is after the map |

### T16b — burst · ✅ PASSED with 914 messages

Deterministic source (MIDI Wrench, knobs 20–23 and buttons 24–33):

| CC | msgs | | CC | msgs |
|---|---|---|---|---|
| 20 `dly_time` | 62 | | 28 (Fx2 mix) | 42 |
| 21 `dly_mix` | 49 | | 29 `rev_mix` | 58 |
| **22 `level`** | **381** | | 30 `in_gain` | 27 |
| 23 `src_mix` | 53 | | 31 `gate_thresh` | 25 |
| 24 `gov_ceiling` | 43 | | 32 `buzios_on` | 19 |
| 25 `fx2_gain` | 72 | | 26 (Fx2 note) | 57 |

**914 messages · orphans 0 · truncated 0 · range 0–127 on all.**

`cc 33` showed up as "(fora do mapa)" on this flight and was therefore mapped
to `fx2_on` — **instrumentation that shows what is NOT mapped made the gap appear
on its own.** Now the Fx2 module turns on over MIDI, and the instrument no longer
depends on serial or boot to play.

### T16b (historical) — burst of 200 CCs

Fader swept fast end to end. With `mscope stats`, one line per second:
**msgs/s**, drops, last 4 CCs. The stream would have gaps here on purpose — it is
the case the `stats` mode exists to cover.

**Passes if:** `ressync` stays at 0 and `fila perdeu` stays at 0.

### T16c — MIDI under Fx2 module load · ✅ PASSED (2026-08-18, v0.40.0)

**The question:** does MIDI survive with the module consuming 65% of the block's
deadline? The known hole was `hal::PollMidi()`, which runs in the main loop — the
bytes arrive by DMA (immune to load), but **it is the loop that dispatches**. If
the loop chokes, the channel goes down with it.

**Procedure** (step 0 of IFÁ10000 first, always):

```
log on            → check echo. No echo, abort.
board             → confirm version
tone off ; set buzios_on 0     → zero the environment
midi              → watermark baseline
set fx2_on 1      → bring up the module (one variable changed)
mscope stats      → 90 s window sweeping CC 22 nonstop
midi ; cpu        → verdict
```

**Result**

| | no load | under load |
|---|---|---|
| Peak CPU | — | **65.1%** (3 470 of 5 333 µs) |
| messages | 65 | **2 779 in 90 s** (~70/s sustained) |
| bytes | 220 | 8 362 |
| orphans / resync / lost | 0 / 0 / 0 | **0 / 0 / 0** |
| UART errors / rearms | 0 / 0 | **0 / 0** |
| **queue peak** | **32 of 128** | **32 of 128** |

**Passes.** The watermark **did not rise by a byte** with the module on: the loop
drains the queue at the same speed at 65% CPU as it does empty. The `PollMidi`
hole is closed **with a number**, not with a guess.

**And the number explains itself.** The 32 bytes are not loop delay — they are
the **burst size of the source**. The WIDI delivers in packets of ~10 messages,
and the queue reflects the BLE, not the dispatch. That is why the value is
identical in both regimes: if it were loop latency, it would have risen along
with the load.

**Hierarchy confirmed in practice:** the black box is a record, the serial is
opportunistic telemetry, **MIDI is control** — and the control did not waver
under load.

> **Minor pending item:** `msgs completas 2779 | APLICADAS 2778`. Exactly **one**
> is missing, the same as the no-load window (65/64). A **fixed** difference of 1,
> not a loss rate — almost certainly the message in transit at the instant of the
> reading. Confirm before calling it cosmetic.

> **Historical note.** This test stayed "not run" for a whole day because four
> earlier attempts reported `0 CCs` — with the log silent due to a `bb arm` of
> mine. They did not measure the link: they measured my silence.
> See case 8 in [../docs/battle_scars.md](../docs/battle_scars.md).

---

## T18 — PAT: Parameter Alignment Test · ⏳ 4 of 14

One parameter at a time, requested **by name** (not by CC number). dB moves the
control, the board records which CC arrived and which param it hit, and dB reports
what he heard. It separates three things that "it does not work" mixes together:
**CC does not arrive**, **CC arrives at the wrong param**, and **CC arrives right
and does not sound**.

**Non-negotiable prerequisite:** step 0 of IFÁ10000 **after each RESET** — the
buzios are born on and draw `dly_time` and `dly_mix`, so judging those two with
them active is measuring two hands on the same control. It has happened, and dB
noticed before I did.

| # | Parameter | CC | Alignment | Behavior |
|---|---|---|---|---|
| 1 | `freeze on` | 26 ✅ | ok | ✅ **certified** (v0.3.1) |
| 2 | `freeze mix` | 27 ✅ | fixed in the app (was sending CC 0) | ✅ works |
| 3 | `delay time` | 20 ✅ | ok | ⚠️ re-test (buzios on during the judgment) |
| 4 | `delay mix` | 21 ✅ | ok | ✅ works |

Missing: `level`, `src mix`, `gov ceiling`, `reverb mix`, `in gain`,
`gate thresh`, `buzios on`, `freeze size`, and the two **expected-dead**
`fx2 gain` (CC 25) and `fx2 on` (CC 33) — the Fx2 slot empty, the expectation is
that the CC arrives and does nothing.

**#1 and #2 UNBLOCKED on 08/18:** the freeze v0.3.1 passed the 10 s acceptance
(*"it came out great like this"*) and the `freeze mix` widget was fixed to CC 27 in
the app. The rule that held up to here still holds for the next: **an unverified
fix becomes a green line nowhere.**

**Method note:** `ctos` was checked AFTER the acceptance to prove that the freeze
was active the whole test. Without that the result is worthless — the first
acceptance was annulled precisely by a false watchdog veto, and the 15 s "of
freeze" were 15 s of silence.

**Finding that justifies the test's existence, and which was confirmed:** the
`freeze mix` widget was sending **CC 0** (out of the map). The instrument was
right and the app on the wrong number — a diagnosis no code reading would find,
and which dB fixed in the app as soon as the PAT pointed it out.

**Map reduced on 08/18:** `src_mix` (CC 23) and `gov_ceiling` (CC 24) left the
MIDI map for safety — see T17. Their widgets on the controller keep sending and
the board ignores them, which shows up as `msgs completas` greater than
`APLICADAS` (95 of 887 in one session). **That is the protection working, not
loss.**

---

## T17 — RECONNECTION QUARANTINE: the sound does not change without a gesture · ⏳ NEW

**The field case (2026-08-18).** BLE link transitions make iOS/Loopy resend the
position of all widgets at once. The instrument obeyed: **sound muted on
disconnect, and freeze with `frz_size` at the ceiling on reconnect** — commands
no one gave, applied as if they were a gesture.

**The defense** (main.cpp v0.44.0): every CC that arrives after silence
> 5 s is **held for up to 100 ms without being applied**. If within the window
≥ 4 **distinct** CCs appear, it is a dump: discard everything that was held and
block for 500 ms. Otherwise, release in order and life goes on.

> **Why hold before deciding.** The trigger is the *fourth* distinct CC. Blocking
> from there on would let the **first three** in — and with 14 CCs in the map,
> three are enough for the observed damage.

### Phase 1 — capture the real burst (the data we still do not have)

```
log on            → check echo
mscope on
```
Connect and disconnect the WIDI **with the scope on**. Record: **which CCs, which
values, and the timing**. The black box stamps them (slots 87 and 89).

**Criterion:** have the burst profile in hand — without it, the threshold of 4
distinct CCs is a reasonable hypothesis, not a measured number.

### Phase 2 — the cycle of 5

Instrument **playing**, with the delay and the reverb in the approved state:

```
1. connect     → did the sound change? 
2. disconnect  → did the sound change?
   ... 5 times
midi     → how many CCs the quarantine refused
ble      → how many (re)connections were detected
```

**Passes if: the sound does NOT change in any of the 10 transitions without
someone having touched a control.** Any audible change without a gesture is a
failure, even if the counters say the quarantine acted.

**False positive to watch:** a legitimate human gesture blocked. The profile is
different — a real fader is **one** CC repeated in a ramp, two fingers are **two**
CCs — but the test has to confirm that playing normally after a pause still
works. Declared cost: the first gesture after 5 s idle arrives with up to 100 ms
of delay, once only.

### Defense in depth (off the board)

Investigate in Loopy whether there is a *"send state on connect"* to turn off at
the source. **It does not replace ours** — a third-party app in the critical path
is exactly what this project avoids — but if it exists, it removes the cause
instead of filtering the symptom.

---

## Forensic catalog — when the instrument destroys its own data

Seven cases in one session, and the pattern is worth more than any one of them:
**instrumentation is production code.** Whoever measures has to be armored like
whoever plays.

1. **D-cache** — a write to the black box died in the cache and vanished on reset.
2. **Bootloader** — its `BootInfo` lives at the base of the backup SRAM and is
   written on every boot, on top of the magic word.
3. **Locked domain** — access without `HAL_PWR_EnableBkUpAccess()` is a bus
   fault, not an ignored write.
4. **The reading erasing the flight** — `static` counters zero on boot, and the
   session that READS was rewriting the slots before looking. It always reported
   "5 pushes" (the reading's number). Fix: only write with the flight active.
5. **The same, one layer down** — I armored the `main.cpp` counters and forgot
   the HAL ones. Fix: a single gate in `BlackboxArm()`.
6. **Paired counters that lie** — `M1==M2` looked like an alibi, but the recorder
   ARMED in the middle of a line, so the counters paired events from different
   lines. Fix: a **state token**, not a count.
7. **Old data with a fresh face** — `BlackboxArm` only zeroed when the magic was
   invalid, so `disparos` from the previous flight showed up mixed in. Fix: zero
   always.

And a **third family** of ordering bug, sister of the context-race and the
configuration-order ones, but in the experimental design: **one variable serving
two purposes, and the test touching both at once.** `DelayMs` was both the clock
AND the metronome of the ramp; removing it to test the clock changed the music
along with it, and invalidated the flight's auditory reading.

### Tools the hunt left behind

`bb arm` (arms before the flight, never in the middle) · `log_mute` (Log only
writes to the black box) · flight token · phase markers · timestamp ·
complete-iteration counter.

---

## Known defects

### The serial dies under a long callback · OPEN

With the Fx2 active, the USB CDC drops a few blocks later (`Device not
configured`) and does not come back until RESET. **The audio keeps running the
whole time** — the sculpture plays normally, there is just no longer any way to
talk to it.

It is not starvation of the main loop, which was the first theory: the measured
load is **61%**, and 61% starves no loop. The standing hypothesis is the callback
holding the processor in blocks of ~3.2 ms and delaying the service of the USB
interrupt enough to break the CDC timing. **Not measured — it is a hypothesis,
not a diagnosis.**

**Practical impact, which is what matters:** no parameter can be adjusted under
the load, so every adjustment becomes a reflash. It was the cause of almost all
the slowness of the 2026-08-16 session.

Workarounds that exist today: the **black box** (`bb`) records during the flight
and is read after the reset; **CC 25** adjusts `fx2_gain` over MIDI, which is the
only control path that survives.

### Undriven external engine: the module runs but events never execute · FIXED

`MinimalEngine` requires the HOST to run `processEventsUntil()` and
`advanceTime()` on every block. Calling only `patcher_.process()` leaves
`_currentTime` at zero forever: pure signal runs (FFT, delay), but everything
that depends on an event — clock, ramp, capture — stays dead. In Max it works
because the engine is Max's. Signature: **runs, costs CPU, does not sound (or
sounds wrong), no log**. Fixed in the Fx2 module's wrapper, v0.5.0; the delay had
the same violation and worked because it was signal-only — fix it when it gets
touched.

### Black box: three silent traps until the first read · FIXED

1. **Locked backup domain**: access without `HAL_PWR_EnableBkUpAccess()` is a
   bus fault, not an ignored write.
2. **D-cache**: a write stays in the cache and dies on reset — every write needs
   `SCB_CleanDCache_by_Addr`.
3. **Bootloader**: ITS `BootInfo` is linked at the BASE of the backup SRAM
   (separate binary!) and written on every boot — on top of anything the app's
   linker put there. The black box lives at `0x38800F00` (end of the region,
   fixed pointer), away from the fight.

### `r_window`: the "fallback" size is the only path · FIXED

`r_window` is a `buffer~` and the export **never** calls `requestSize()` on it,
so `getSizeInBytes()` always returns 0 and the code that "reads the size from the
dataref itself" never executed. The fallback was at 2048 for a 1024 FFT: the
patch read half a window — a ramp — and the overlap-add never summed flat. Fixed
in the Fx2 module's wrapper, v0.3.0.

### Backup SRAM: unlock the domain before any access · FIXED

Just turning on the clock is not enough. On the H7 the backup domain is born with
write protection, and access to `0x38800000` without `HAL_PWR_EnableBkUpAccess()`
gives a **bus fault**, not an ignored write — the board dies on the first `bb`,
looking like a boot defect unrelated to the module. The full sequence is in
`hal_daisy_seed v0.10.1`.

| # | Symptom | State |
|---|---|---|
| 1 | `list` hung the board (CDC silent until reset) | ✅ fixed in HAL v0.2.0 |
| 2 | Every `%f` came out empty — `board` without the `48000`, `list` with `norm= real=` | ✅ fixed in the Makefile |
| 3 | A log line came out chopped and stuck to the next | ✅ fixed in HAL v0.3.1 |
| 4 | A serial command took up to 0.5 s to respond | ✅ fixed in main.cpp v0.3.0 |
| 5 | The governor's soft-knee had a 0.09 step at the top of the knee | ✅ fixed in dynamics v0.3.1 |
| 6 | `set <param>` received NORMALIZED, not real — `set bench_boost 1.0` gave **8×** | ✅ fixed in main.cpp v0.7.0 |

**#1 — serial dispatch ran in the ISR.** `SetReceiveCallback` fires in interrupt
context, and `hal::Log()` transmits over the same USB: the second `Log` waited for
a TX that only completes when the ISR returns. Deadlock. `board` (1 log) passed,
`list` (4 logs) killed it. The ISR now only enqueues (SPSC of 8, full queue
discards instead of blocking) and `PollSerial()` dispatches from the main loop —
which is exactly what it exists for in the `hal.h` contract.

**#2 — printf without float.** libDaisy links with `--specs=nano.specs`, and
newlib-nano comes without `%f` support: a **silent** failure, no warning. Since
the manifest and the panel are all floats, half the output vanished. `LDFLAGS = -u
_printf_float` in the Makefile, costs ~4.6 KB.

**#3 — the Logger's single buffer.** libDaisy's `Logger` formats everything into a
**shared** `static char tx_buff_[128]` and transmits without waiting: the second
line overwrote the buffer while the USB was still reading the first, and the
output came out truncated and stuck (`SDRAM via AllocBig:$$ 0 level...`), sometimes
vanishing.

It was not timing — that is why a `System::Delay(2)` per line did **not** cure it.
`hal::Log` now ignores the Logger and transmits directly over
`hw.usb_handle.TransmitInternal()`, with **two alternating buffers** and retry
until the CDC accepts. Since `Log` only returns after the accept, when writing to
B the buffer A has already been handed to the hardware, and the transmit of B is
only accepted when A finished: no buffer is touched while the DMA reads it. The
retry has a ceiling of ~50 ms, so a board with no host reading does not hang.

**Verified:** 21 lines in a burst with 50 ms between commands, all intact.

**#4 — latency.** With dispatch by polling, the loop's `DelayMs(500)` became the
ceiling of command response. The loop now spins at ~1 ms and the LED became a
counted heartbeat (toggle every 500 ticks), holding 1 Hz. Verified with commands
at 0.35 s intervals: immediate response.

**#6 — `set` received normalized.** It was consistent with `ParamTable::Set()`,
but became a trap on a parameter whose range is not `[0,1]`: `set bench_boost 1.0`
looked like "no boost" and delivered **8×**, the maximum — on a parameter that
exists precisely to overload the exciter's output. "Looks safe and is the
maximum" is the wrong inversion to have in an instrument.

`set` now speaks in **real units** — Hz, ms, ×. It converts to normalized by
inverting the curve:

```
norm = pow((real - min) / (max - min), 1/curve)
```

and **refuses** out of range instead of saturating: whoever types 12 on a
parameter that goes up to 8 made a mistake, and saturating at 8 would hide the
error.

The normalized `Set()` still exists alongside `SetReal()`, and knob and macro
keep using it — a travel position is normalized by nature. The change is only at
the human boundary.

Welcome side effect: it also fixes `dly_time`. With `curve 2.5`,
`set dly_time 0.5` gave real `0.177`; now it gives `0.500`. Before you had to know
the curve by heart to type the right value.

**Validated on board** (firmware v0.7.0):

```
> set tone_freq 440     < tone_freq -> real=440.000 (norm=0.188, curve 1.0)
> set bench_boost 1     < bench_boost -> real=1.000 (norm=0.000, curve 1.0)
> set bench_boost 12    < fora da faixa: bench_boost aceita [1.000 .. 8.000]
> set dly_time 0.5      < dly_time -> real=0.500 (norm=0.758, curve 2.5)
> set tone_freq 5000    < fora da faixa: tone_freq aceita [80.000 .. 2000.000]
```

`pow(0.5, 1/2.5) = 0.758` confirms the curve inversion, and the FFT confirms the
440 Hz at the output (440.12 Hz measured, +0.5 cents — the usual clock drift).

---

## Architecture note

**`AllocBig()` stopped being discarded by the linker.** Until v0.3.0 the SDRAM
showed up as `0 B / 64 MB` because nothing called the function and `--gc-sections`
ate the entire pool. With the external module allocating the delay line, the 64 MB
came back at once: the map now shows `SDRAM: 64 MB / 64 MB = 100%`.

This is **expected, not overflow**. The `sdram_pool` in `hal_daisy_seed.cpp`
reserves the entire region on purpose, and parcels it out internally — the 100% is
the pool, not the consumption. The real consumption is what the boot prints:

```
SDRAM via AllocBig: N de 67108864 bytes
```

The practical consequence is the usual one: **no byte is left for anything else in
`.sdram_bss`**. Every SDRAM access has to go through the gate. The full rule is in
[BOARDS.md](BOARDS.md).

### How much the external engine asked for — prediction vs. measured

The prediction was off by exactly **2×**, and the cause is instructive:

| Source | Math | Bytes |
|---|---|---|
| Predicted | 131 072 samples × **4 B**, assuming the float32 macro applied to the delay line | 524 296 |
| **Measured** | 131 072 samples × **8 B** + 8 B of header | **1 048 584** |

**Why double:** the float32 macro switches `SampleValue` (the patcher's signals)
to `float`, but the delay line is a `buffer~`, and the generated code instantiates
it as **`Float64Buffer`** — 8 B per sample, independent of the macro:

```cpp
this->delaytilde_01_del_buffer = new Float64Buffer(this->delaytilde_01_del_bufferobj);
```

131 072 × 8 = 1 048 576, plus 8 of our header = **1 048 584**, which is the
measured number, to the byte. The macro does not reach the `buffer~` storage.

**Planning consequence:** every `buffer~` of a patch costs 8 B per sample, not 4.
A 10 s freeze at 48 kHz asks for ~4 MB, not 2. With 64 MB of SDRAM there is still
plenty left, but the math for sizing modules is that one.

The patcher's other buffers are the size of the block and **fit in the 32 KB pool
in SRAM**, as predicted: 2 000 B in 15 blocks, 6% of the pool. Only the delay line
went to SDRAM.

> **Note on the float32 macro:** without it, `SampleValue` would be `double` and
> the same delay line would ask for **1 MB** instead of 512 KB.
