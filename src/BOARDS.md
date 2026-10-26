# BOARDS.md — supported boards and resource rules

**Document v0.14.0**

How to port the Cotucatronics OS to a new board, and the resource rules that
hold across all of them.

---

## Boards

| Board | `BOARD=` | File | State |
|---|---|---|---|
| Daisy Seed 2 (DFM) | `daisy_seed` | `hal_daisy_seed.cpp` v0.3.1 | ✅ validated on the bench |
| Teensy 4.1 | `teensy41` | `hal_teensy41.cpp` | 🔜 planned |

### Daisy Seed 2 — `BoardInfo`

| Field | Value |
|---|---|
| `name` | `"Daisy Seed"` |
| `num_knobs` | 5 (4 character + SRC MIX) |
| `has_audio_in` | `true` |
| `sample_rate` | 48000 Hz |
| `block_size` | 48 (~1 ms) |
| `big_ram_bytes` | 67108864 (64 MB SDRAM) |

Detecting Seed 1.1 vs Seed 2 DFM is automatic inside libDaisy's `hw.Init()`
(`CheckBoardVersion()`), and the Seed 2's PCM3060 codec is handled in there.
**None of this leaks into our code.**

> ### Daisy: the app runs from the 8 MB QSPI, via bootloader
>
> Since v0.4.0 the firmware is compiled with **`APP_TYPE=BOOT_QSPI`**: the app
> lives in the **8 MB QSPI flash** and **the 128 KB of internal flash belong to
> Electrosmith's bootloader**. Not one byte of our code lives in internal flash —
> the memory map shows `FLASH: 0 B`, and that is correct.
>
> **Why:** the first external DSP module alone pushed the firmware to 94.14% of
> the 128 KB internal. The second would not fit. On QSPI the same firmware takes
> 1.52% of 7,936 KB.
>
> **What it costs:**
> - the Cortex-M7 executes from QSPI slower than from internal flash — mitigated
>   by the I-cache, but it is real, and worth measuring if a heavy module starts
>   to blow the audio block's deadline;
> - boot gains ~2 s of grace window from the bootloader;
> - flashing has one extra step, **once per board** (`make program-boot`).
>
> **This is not a project rule, it is Daisy configuration.** A new board inherits
> none of it: `APP_TYPE` belongs to the `Makefile` and to the Electrosmith
> ecosystem. The Teensy 4.1 has 8 MB of flash directly, with no intermediate
> bootloader.
>
> Full flashing flow in [TESTES.md](TESTES.md).

---

## Panel board v2 — soldering guide

**5 pots + 2 momentary buttons.** Hardware not yet soldered; the firmware is
already done and testable over serial (T11 in [TESTES.md](TESTES.md)).

### The 5 pots

| Pot | Function | libDaisy | Physical pin |
|---|---|---|---|
| k1 | **level** (master volume) | `seed::A0` | **16** |
| k2 | **src_mix** (piezo ↔ jack) | `seed::A1` | 17 |
| k3 | character macro | `seed::A2` | 18 |
| k4 | character macro | `seed::A3` | 19 |
| k5 | character macro | `seed::A4` | 20 |

**10 kΩ linear.** Wiring of each one:

```
  3V3A  ──── terminal 1
  wiper ──── ADC (A0–A4)  ──┬── 100 nF ── AGND
  AGND  ──── terminal 3     └── (the capacitor goes RIGHT AT the wiper)
```

The **100 nF at the wiper** is not optional: without it the ADC reads the noise
the pot picks up like an antenna, and the parameter jitters on its own. Place it
near the board pin, not near the pot.

**Use 3V3A and AGND, not 3V3D and DGND.** The pots are an analog reference;
powering them from the digital side injects the switching straight into the read
value.

> **k1 and k2 are FIXED on every instrument** — volume and source. Only k3–k5
> change from sculpture to sculpture. That gives the panel a constant grammar:
> the hand learns it once and it holds for any piece.

### The 2 buttons

| Button | Function | libDaisy | Physical pin |
|---|---|---|---|
| B1 | randomizer | `seed::D26` | 33 |
| B2 | assignable (hold) | `seed::D27` | 34 |

```
  button ── one terminal on the pin (D26/D27)
         └─ other terminal on DGND
```

**No external resistor.** The firmware enables the internal pull-up, so the
button only needs to close to ground. At rest it reads high, pressed it reads
low. A 10 ms debounce is done in software, by timing.

> ### ⚠️ A0–A4 ARE D15–D19. The same physical pin.
>
> On the Daisy, `A0` is just another name for `D15`, `A1` for `D16`, and so on up
> to `A4` = `D19`. **They are not different pins.**
>
> That is why the buttons went to D26/D27: any pin from D15 to D19 would collide
> with a pot, and D13/D14 are already the MIDI UART. D26 and D27 are among the
> few with no ADC alias (A10=D25, A11=D28) and no function taken.
>
> If you ever move a button, **check the alias before soldering** — the collision
> throws no compile error and no boot error: it gives you a pot that does not read
> and a button that does not respond, both in silence.

**About the physical pin numbers:** they follow the same count as
[TESTES.md](TESTES.md), in which pin 18 is AUDIO OUT L and pin 20 is AGND.
**Check against Electrosmith's official diagram before touching the iron** — the
reference that does not depend on that count is the `D`/`A` name.

---

## MIDI — WIDI soldering guide

**The Daisy's MIDI port is USART1 at 31250 baud, 8N1.** Turned on at boot by
`hal::SetMidiByteCallback()`; a board that never calls it pays for no
peripheral.

### Pins

| Signal | libDaisy | STM32 | Physical pin | Goes to what on the module |
|---|---|---|---|---|
| **TX** (the board speaks) | `seed::D13` | `PB6` | **14** | **RX** of the WIDI |
| **RX** (the board listens) | `seed::D14` | `PB7` | **15** | **TX** of the WIDI |
| 3V3 | — | — | see silkscreen | VCC of the WIDI |
| GND | — | — | any DGND | GND of the WIDI |
| **LB** (link state) | `seed::D28` | — | — | **LB** pad of the WIDI |

> ### D29 AND D30 ARE FREE as of 08/20
>
> They were **LG** (D29) and **LR** (D30), read together with LB to find out
> which pad carried the connection state. They were measured for two days and
> **marked nothing useful** — removed from the hardware.
>
> **This is a result, not a surrender:** LG and LR are discarded as candidates
> *because they mark nothing*, which closes two of the three hypotheses. What the
> **LB** indicates remains an open question — the CME's legend was disproved by
> measurement (dB disconnected and LB stayed fixed), so the LB is **not**
> connected/disconnected.
>
> **D29 and D30 went back into the pool of available pins.** They are natural
> candidates for the next subsystem that needs GPIO — more buttons, a status LED
> of our own, an encoder. **Check the ADC alias before soldering** (the rule in
> the box above applies the same): the collision throws no compile error and no
> boot error, it gives a pot that does not read and a button that does not
> respond, both in silence.

> ### ⚠️ IT IS CROSSED. This is the mistake that costs an afternoon.
>
> **TX of the module → RX of the board (D14).** **RX of the module → TX of the
> board (D13).** Wiring it straight — TX to TX — burns nothing and throws no
> error at all: no byte simply arrives. The symptom is `bytes 0` on the `midi`
> command, identical to a module with no power.
>
> **If `midi` shows `bytes 0`, suspect the crossover before anything else.**

**About the physical pin numbers.** D13/D14 come straight from libDaisy's
`daisy_seed.h` (`PORTB,6` and `PORTB,7`) — those are certain. The numbers 14 and
15 follow from the SOM's numbering, in which pins 1–15 are D0–D14 (which is what
makes pin 18 be AUDIO OUT L and pin 20 be AGND, as [TESTES.md](TESTES.md)
already used). **Check against Electrosmith's official diagram before touching
the iron** — getting the power pin wrong costs the board, and the D-number is the
reference that does not depend on that count.

The **3V3 and GND pins are not listed by number on purpose**: the SOM has more
than one GND, and guessing the power pin is the one thing here that destroys
hardware. Locate it on the silkscreen.

**These are the default pins of libDaisy's `MidiUartTransport`.** A cable wired
for any Electrosmith MIDI example works here without change — that is why they
were chosen, and not out of preference.

### ✅ CHECKED 2026-08-17, with the WIDI Core in hand

Before touching the iron, the state that matters:

- **Pins:** D13 (TX, PB6) and D14 (RX, PB7) — come straight from libDaisy's
  `daisy_seed.h`, are the defaults of `MidiUartTransport`, and collide with
  nothing: the five pots occupy A0–A4 (= D15–D19) and the buttons D26/D27.
- **Parser:** board-agnostic in `midi.h`, committed in `b3d994c`. The ISR only
  enqueues; whoever dispatches is `hal::PollMidi()` in the main loop — the same
  discipline the serial learned the hard way.
- **CC map:** 13 entries (limit 16). See the table below.

> **MIDI stopped being a convenience and became FLIGHT CONTROL.** USB serial dies
> seconds after the callback runs long (known defect, see [TESTES.md](TESTES.md));
> MIDI is immune — its own UART, DMA, nothing to do with the CDC. Every param that
> only the serial could reach is on the map now.

### CC map

**The map lives in [docs/operacao/CCMAP.md](../docs/operacao/CCMAP.md), which is
GENERATED** from `manifest.h` by `gerar_ccmap.py` — and that is where you read it,
not here.

Until 09/04 this section carried a hand-made table titled *"of the current
instrument"* that listed four params that no longer exist. A hand table aging on
top of a generated one lies with an air of authority; it was removed instead of
corrected, because correcting it would be keeping alive the one that lies.

The chosen range still stands as a decision: the low CCs sit in the MIDI spec's
"undefined" range — they collide with nothing standard (1 is mod wheel, 7 is
volume, 64 is sustain), so a generic controller ships from the factory without
using them.

**A param with `steps` quantizes at the SINGLE point of writing:** `Set()` rounds
to the grid, and MIDI, serial and knob inherit the same. Nothing to handle in
each path.

**`buzios_on` becomes a switch with no special handling:** the param is [0,1],
`main` tests `>= 0.5`, and `MidiNorm(63)=0.496` falls below while
`MidiNorm(64)=0.504` falls above. The contract already made this work.

### ✅ REVOKED: the pairing SURVIVES the reset (measured on 2026-08-18)

Until v0.40.0 this section ordered forgetting and re-pairing the WIDI after
every RESET. **The rule was revoked by measurement** — there is no special
procedure. Flash, test, go.

| Measured on 08/18, v0.40.0, without re-pairing | |
|---|---|
| after DFU + flashing + reset | 65 CCs, 220 bytes |
| orphans / resyncs / lost / errors | 0 / 0 / 0 / 0 |

**Where the false rule came from:** the `bb arm` command silenced the log and had
no path back. Four test windows measured that silence and read it as "dead link";
the pairing became the plausible explanation that filled the hole. Full story in
case 8 and in the addendum to chapter 11 of
[../docs/battle_scars.md](../docs/battle_scars.md).

**What remains of the old section, and still holds:** `bytes 0` is a **blind**
symptom — identical for a crossed wire, a module with no power, an app with no
authorization, a competing central, and a muted log. That is why the first
question facing a zero is never *"what's the cause?"* but ***"is this channel
proven?"*** — `log on`, check the echo, and only then interpret. If a status
command prints `[LOG MUDO]`, you are looking at your own silence.

### 🔬 MIDI SCOPE — the analyzer is ours, and runs on the board itself

An **official and permanent** tool. No external apps, no extra hardware: a USB
cable and the serial.

```
mscope on      stream: raw hex + what the parser CONCLUDED from each byte
mscope stats   one line per second (for the sliding-fader case)
mscope off
midi           permanent counters, always active
```

**Why two columns.** Goal #1 is to see the parser **desync**, and for that it is
not enough to show what arrived — it has to show what it concluded. The same byte
means opposite things depending on state: `0x45` is *data1 of a CC* or an
*orphan*, depending on what came before.

```
  B0  channel status
  1D  partial data (data1)
  45  CC ch1 cc29 val69 -> rev_mix 0.543
  45  ?? DATA WITH NO STATUS
  B0  !! TRUNCATED (resync)
```

**Permanent counters** (in `midi`, always active, mirrored in the black box every
second):

| | |
|---|---|
| `bytes` | arrived over the UART |
| `complete msgs` | became a message |
| **`APPLIED`** | found an owner in the map |
| `resync` | status arrived with data pending = truncated message |
| `orphans` | data with no status = stream caught mid-flow |
| `queue lost` | dropped in the ISR |
| state | awaiting STATUS / data1 / data2 |

> **`complete msgs` vs `APPLIED` is the separation that matters:** "arrived and
> did not become a message" and "became a message and did not become sound" are
> completely different fixes. One is wire/parser, the other is CC map.

**The scope respects the hard budget.** It writes through `hal::Log`, which drops
rather than waits — in a burst the stream has **holes, and that is by design**. A
diagnostic tool that holds the loop is the next `DelayMs` (see ch. 7 of
[battle_scars](../docs/battle_scars.md)). **The counters never lie; the stream
does** — and that is why the `stats` mode exists, in which the deluge becomes one
line per second.

### ⚡ AFTER FLASHING: reconnect, DO NOT forget

**The BLE link survives flashing. The CoreMIDI session does not.** They are
different layers, and confusing them cost three steps where two are enough.

| Layer | After `dfu go` + flash |
|---|---|
| **BLE** (what the LB pin sees) | ✅ connected — `LB 0/100%` |
| **CoreMIDI session** (what Loopy uses) | ❌ dies |

**The ritual, measured on 08/18:**

1. ~~Forget the Bluetooth on the iPad~~ — **unnecessary**, the pairing never
   broke
2. **Reconnect the WIDI** in the Bluetooth MIDI panel (you can do it inside Loopy
   Pro itself)
3. Go back to playing

> **This refines the six-quadrant matrix below:** `FIXED + zero traffic` **does
> not distinguish** *"the app stopped sending"* from *"the MIDI session dropped
> with the radio intact"*. Same pair of symptoms, different causes — and the
> second is the one that happens **every time** you flash.
>
> The `ble` command detects this specific case (LB fixed + zero bytes since boot)
> and prints the instruction instead of the diagnosis.

---

### 📡 WIDI/BLE DIAGNOSIS — three layers, no third-party app

The `mscope` answers *"what arrived"*. It never knew how to answer *"is the radio
alive?"* — and that is the stage's question. This is the mscope of the radio
layer, same philosophy: **the tool is ours and no third-party app enters the
critical path.**

#### Layer 1 — the LED on the LB pad

LED + 470 Ω soldered on the WIDI Core's **LB** pad, with **V3 as the source**.
**LB only** — LG and LR were removed on 08/20 (see the box in the WIDI soldering
section).

Official legend of the module, per the **CME**:

| LED | Means |
|---|---|
| slow blink | advertising — no one connected |
| fixed | connected |

> ⚠️ **THE BENCH DISAGREED, AND THE BENCH IS BOSS.** This table was a CME
> hypothesis, relayed by dB, and measurement disproved it: **dB disconnected and
> the LB stayed FIXED.** So the LB is not connected/disconnected — it carries
> something else, and what that is is still unknown.
>
> The obvious way out was "then it's the LG or the LR". Both were read for two
> days and **marked nothing**, and that is why they came out of the hardware on
> 08/20. What remains is an open question with **two fewer candidates** — which is
> progress, not the same place as before.
>
> In practice the LB stays **useful as a raw signal**: `LB 0/100%` (fixed) vs
> transitions counting. What you cannot do is translate it to "connected" without
> measuring again.

#### The LED × mscope matrix — six quadrants, one culprit each

The LED alone is not enough, the `mscope` alone is not enough. **Crossed, each
combination has a single culprit** — which is exactly what was missing when
`bytes 0` was compatible with five causes at once.

| LED | traffic in `mscope` | What is happening | Culprit / fix |
|---|---|---|---|
| slow blink | zero | advertising, no one connected | **the central**: pair; Forget Device if it refuses |
| slow blink | arriving | contradiction — should not exist | **the instrumentation**: LED badly soldered, or wrong pad. The radio is fine |
| fixed | zero | connected, and still nothing arrives | **the app**: not sending, CC off the map, or wrong channel |
| fixed | arriving | ✅ all good | — |
| off | zero | module with no power | **the power**: 5 V rail, wire, solder |
| off | arriving | radio working, LED not | **the LED**, not the link. Ignore it and keep playing |

The line that most pays for the work of soldering is the third: **fixed + zero**
was, until now, indistinguishable from everything else. It separates "the radio
failed" from "the radio is fine and the problem is what you configured on the
iPad".

#### Layer 2 — the `ble` command

```
ble
```

It answers four things the `midi` never knew:

- **age of the last byte** — the measure that separates *"never connected"* from
  *"connected and dropped"*. `bytes 0` is identical in both; age is not.
- **session messages** — zeroed on each detected (re)connection.
- **drops** — silence > 10 s **after** there had been traffic. The condition
  *"after traffic"* is what stops a board that never paired from accumulating a
  drop every 10 s.
- **(re)connections detected** — see below.

All stamped in the black box (slots 86, 87, 88), so it survives RESET and can be
read after a flight in which the serial did not exist.

And `ble` **flags `[LOG MUDO]`** like all the other status commands — step 0
built in. A diagnostic tool that can lie by silence already cost a whole day
(case 8 of battle_scars).

#### The sync-dump detector

**A field pattern became a sensor.** On (re)connecting, the controller dumps the
state of several controls at once — we watched this happen several times before
understanding what it was. Now the board recognizes it on its own:

> **≥ 4 DISTINCT CCs in less than 100 ms, right after silence > 5 s**
> ⇒ "probable (re)connection", stamped.

**Distinct, and not quantity** — and that word is the whole detector. Sweeping a
single fader sends dozens of messages of the *same* CC in 100 ms; by a quantity
criterion, playing would be indistinguishable from reconnecting.

#### Layer 3 — decision tree: "the Bluetooth stopped"

**1. The LED, before anything else.**
Off → it is power, not radio: 5 V rail, wire, solder. Slow blink → no one is
connected, go to step 3. Fixed → the link exists; the problem is further on, go
to step 2.

**2. `ble` — is the link alive or stopped?**
With no echo at all, check `[LOG MUDO]` and send `log on` first: a zero on an
unproven channel is not data. With the log alive: *NEVER RECEIVED* → never paired.
*SILENT for more than 10 s* → dropped, go to step 3. *ALIVE* → the radio is fine
and the problem is the map: check CC and channel with `midi`.

**3. One central in the universe.**
More than one device trying to talk to the same WIDI is the most common cause and
the most invisible. Turn off the Bluetooth of everything that is not the device
in use — the iPhone forgotten in your pocket counts.

**4. The known fixes, in order of cost.**
*Forget Device* on the device and pair again → power-cycle the module → pair
again. If it comes back, `ble` stamps the (re)connection on its own and the drop
count stays in the record.

> **What this tree replaces:** an hour spent switching suspects five times — link
> dropping, misconfigured fader, app with no authorization, aborted DMA,
> controller sending a dump — without confirming or ruling out any. The symptom
> was always the same `bytes 0`, and the explanation was new each round. Four
> steps with one culprit each are worth more than five hypotheses with none.

### Why DMA in listen mode, and not polling nor one IRQ per byte

The MIDI flow is sporadic and in short bursts. Polling either wastes the main
loop or loses a byte; an interrupt per byte at 31250 baud is a pointless context
switch. `DmaListenStart` — circular DMA with an IDLE interrupt — only bothers the
CPU when a burst ends. It is the same mechanism libDaisy itself uses in
`MidiUartTransport`.

> **The DMA buffer has to live in a DMA-accessible section**
> (`DMA_BUFFER_MEM_SECTION`, which is `.sram1_bss`). In a common buffer the DMA
> writes to RAM and the core reads the cache: the bytes arrive and the parser sees
> zeros. In the memory map it appears under `RAM_D2_DMA` — 256 bytes.

### What belongs to the board and what does not

The HAL delivers **raw bytes**, one by one. It does not parse. The MIDI state
machine is board-agnostic and lives in [midi.h](midi.h), on the shell side.

This is not purism: "how the bytes arrive" changes from board to board (DMA,
interrupt, polling), but "what they mean" never changes. A parser inside the HAL
would have to be rewritten on the Teensy — and rewritten the same.

**The RX arrives by interrupt and is dispatched in the main loop**, via
`hal::PollMidi()`, exactly like the serial lines. Same rule, same reason: running
the parser in the ISR means touching `params` and possibly logging from inside it,
which is the deadlock that killed `list` in v0.2.0.

---

## Noise: the computer's USB is the dominant source

> **Powering the Daisy from the computer's USB injects audible noise.** Measured,
> not assumed.
>
> **The internal battery was always in the project; the USB is only for flashing
> firmware.** So this is not a new requirement — it is the measure of what the
> bench setup costs, and the confirmation that the reported noise does NOT come
> along with the assembled instrument.
>
> On battery, the only noise left is the residual 1 kHz below — ours, at −90 dBFS.
> It is the only one that goes to the field.

Protocol: input shorted (`AUDIO_IN_L` → `AGND`, piezo out), chain neutralized
(`dly_mix` 0, `tone_mix` 0, `shift_hz` 0, `bench_boost` 1×, `gate_thresh` **0** to
keep the gate open — with it closing, the floor is masked and the measurement is
worthless). Spectrum by `cotuca_noise.py`.

| | Computer USB | Powerbank | Δ |
|---|---|---|---|
| Peak | −44.1 dBFS | **−64.4** | **−20 dB** |
| 16,002 Hz | −80.9 | gone | — |
| 7,998 Hz | −85.2 | gone | — |
| 3,000 Hz | −86.9 | −100.5 | −13.6 |
| 1,998 Hz | −88.9 | −97.4 | −8.5 |
| 1,002 Hz | −88.1 | −90.4 | −2.3 |

**The two symptoms have distinct signatures and the same source:**

- **High-pitched whistle** = the **16 kHz** harmonic of the 1 kHz series. Vanishes
  completely on battery.
- **Crackling** = **impulsive** energy: a crest factor of 30 dB on USB (peak 30 dB
  above RMS), against 12 dB on battery. Continuous noise has a low crest; a high
  crest is an isolated event, which is what you hear as a pop.

### The 1 kHz residual is OURS, and it is the block rate

The 1,002 Hz survive on battery at nearly the same level. **With no USB host there
is no frame** — so that peak cannot be USB. A block of 48 at 48 kHz gives exactly
**1,000 blocks per second**.

It is recorded as **unresolved and probably inaudible**: −90 dBFS and without the
high harmonics, which were what the ear was catching. If it ever bothers, the test
that confirms is cheap — **changing the block moves the frequency** (128 → 375
Hz). And the two plausible causes move together, so the test confirms the origin
but **does not distinguish**:

1. **Software** — a parameter read once per block and applied as a step at the
   boundary: zipper noise. Fix: interpolate the gain over the block.
2. **Hardware** — the callback's current peak coupling into the analog supply.
   Software does not solve it.

### A recorded interpretation trap

On the first reading of this spectrum the 1 kHz series was named "USB frame"
because it matched a suspect from the list. It was right by accident: in test 1
the board **was** on the computer's USB. When the battery test was read out of
order, the same series became "block rate" and the conclusion inverted.

**A frequency coinciding with a known suspect is not identification.** Two
different sources here produce exactly 1 kHz, and only the test that removes one of
them separates the two.

---

## Versioned state — INSTRUMENT

> **House rule: the repo versions the INSTRUMENT. The bench is transient state.**
> What is in `main` is what should be on the board to play; any measurement
> configuration is a documented recipe, not a commit.

| | Value | Where it changes |
|---|---|---|
| Clock | **480 MHz** | permanent, `hw.Init(true)` |
| Block | **48** — 1,000 µs, 1 ms latency | `Requirements::block_size` |
| Modules | `delay` in the instrument slot, **fx2 empty** | `MODULE_SRC` in the `Makefile` |
| `dly_time` / `dly_mix` | 0.40 / 0.50 | manifest |
| App | QSPI via bootloader | `APP_TYPE=BOOT_QSPI` |
| Measured CPU | **188 µs of 1,000 — 18.8%** | |

### Note on `-O3`

It stays until the budget tightens. Measured against `-O2`: the average **got
worse** from 11.3% to 13.0%, peak practically the same. The app executes from QSPI
and the binary inflated 13% (260 → 295 KB) — more code, more I-cache misses, and
in external memory that costs more than the inlining earns.

> **When reverting, RE-MEASURE EVERYTHING.** All the numbers in this document and
> in the commits — 188 µs of the instrument, 3,520 of the FFT module, 7,349 of the
> freeze — have `-O3` as a baseline. Comparing a new number with an old one after
> switching the optimization is comparing different things.

---

## Module SRAM pool — HISTORY (through 08/20)

> ⚠️ **The numbers in this section are from the external module system, which went
> out entirely on 08/20.** They stay because the sizing lesson survives the tool.
> **Today's value is different: 92.4%**, and lives in
> [STATUS.md](../docs/STATUS.md). Do not compare the two.

The allocator served a static pool of **48 KB** in SRAM and only fell to
`AllocBig()` (SDRAM) for what did not fit. State with two modules:

| Module | SRAM | Blocks |
|---|---|---|
| delay | 9,368 B | 12 |
| the Fx2-slot one | 38,128 B | 22 |
| **total** | **47,496 B of 49,152** | **96.6%** |

**A third module does not fit.** The number was always that; the warm-up in `Init`
only made it visible, because before, part of the allocations happened in the
first `Process` and stayed off the books.

### The method, before growing the pool or migrating blind

The decision is deferred until module 3 exists, but the **method** is already
defined, and it matters more than the decision:

1. **Label the allocations by origin** in the `MemReport`, separating **hot** ones
   (read sample by sample — block signals, delay lines, FFT buffers) from **warm**
   ones (state, event queues, tables consulted per block).
2. **Hot ones stay in SRAM.** Migrating them to SDRAM costs access latency inside
   the callback, which is exactly where there is no slack.
3. **Warm ones migrate cheaply.** State read once per block does not feel the
   difference.
4. Only then decide: grow the pool, migrate the warm ones, or both.

Growing the pool blind steals SRAM from whoever might need it more; migrating
blind can push a hot buffer to SDRAM and create a bottleneck where there was none.
**Without the map, both choices are bets.**

> An honest caveat about the label: the allocator **does not know** the semantics
> of the caller — the external engine asked for memory without saying what for.
> Labeling by origin requires either manual annotation per module, or a heuristic
> by allocation size and phase. The heuristic is cheap and imperfect; the
> annotation is exact and does not scale to a patch that changes it. Choosing
> between the two is part of the task, not a detail of it.

---

## The big-RAM rule

> **`AllocBig()` is the only access to the SDRAM in the whole project.**

It is the rule that makes the HAL boundary hold for memory too, not just for the
API.

### Forbidden

Declaring `DSY_SDRAM_BSS` — or the equivalent section macro of any other board —
**outside `hal_<board>.cpp`**. This includes modules, external-engine exports,
freeze buffers, long delays and notch banks.

```cpp
// WRONG: binds the module to the Daisy and breaches the boundary
static float DSY_SDRAM_BSS buffer[1024 * 1024];
```

```cpp
// RIGHT: single gate, and the nullptr check is mandatory
float* buffer = (float*)cotuca::hal::AllocBig(1024 * 1024 * sizeof(float));
if(!buffer)
    return false; // board with no big RAM, or pool exhausted
```

### Why

1. **Portability.** A module that declares `DSY_SDRAM_BSS` does not compile on the
   Teensy. One that calls `AllocBig()` compiles anywhere and degrades gracefully:
   the board with no big RAM returns `nullptr` and the module refuses to load, with
   a clear error at boot.
2. **Accountability.** Sections declared scattered through the code are invisible
   until the linker blows up. Through the gate, the total is always known.
3. **Determinism.** The allocator is bump — no free, no fragmentation, no runtime
   surprise.

### Usage contract

- **Boot only.** `AllocBig()` **never** in the audio callback.
- **Check `nullptr` always.** The caller decides what to do without the RAM.
- **No `free`.** It is a bump allocator: what was handed out does not come back.
  Declare what you need once.
- **Declare it in `Requirements`.** Each instrument's manifest announces
  `big_ram_bytes`, checked at boot against `hal::Info().big_ram_bytes`. An
  insufficient board = fast LED + log, instead of a bug on stage.

### Boot extract

Every boot prints how much went out through the gate:

```
SDRAM via AllocBig: 524296 de 67108864 bytes
```

If a module allocates outside, **that number does not balance** — it is the
detector of a violation of the rule.

The `mods` command brings its own extract, more detailed.

### Known trap — now active

`sdram_pool` in `hal_daisy_seed.cpp` reserves the **entire** SDRAM region (64 MB).
Through v0.3.0 the `--gc-sections` discarded the pool because nothing called
`AllocBig()`, and the map showed `SDRAM: 0 B`. **As of v0.4.0 there are callers —
delay, freeze, reverb and the pitch engines — and the 64 MB came back all at
once:** the map shows `SDRAM: 100%`.

This is the pool, not the consumption — but the consequence is real: **not a byte
is left for anything else in `.sdram_bss`**. Every allocation has to go through the
gate. As long as it does, it is safe — but only while the rule is respected.

---

## Porting to a new board

1. Write `hal_<board>.cpp` implementing **all** of `hal.h`'s contract: `Init`,
   `Info`, `StartAudio`, `ReadKnob`, `ReadButton`, `SetLed`, `Log`,
   `SetSerialLineCallback`, `PollSerial`, `DelayMs`, `AllocBig`, `AllocBigUsed`.
2. Fill `BoardInfo` with the truth of the board. No big RAM? `big_ram_bytes = 0`
   and `AllocBig()` returning `nullptr` — do not lie, the boot depends on this to
   refuse instruments that do not fit.
3. `make BOARD=<board>`. The Makefile finds `hal_<board>.cpp` on its own and lists
   the available boards if you get the name wrong.
4. Run [TESTES.md](TESTES.md). T1–T3 do not depend on the board.

### Traps to implement

- **`Log()` cannot be called from an ISR** if it transmits over the same
  peripheral that generates the interrupt. On the Daisy this froze the whole board.
  If your board's RX is by interrupt, **enqueue and dispatch in `PollSerial()`**.
- **`ReadKnob()` returns `-1`** when there is no physical pot connected. The main
  loop treats this as "no hardware, obey the serial". Do not return 0.
