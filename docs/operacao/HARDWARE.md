# HARDWARE — Cotucatronics

> Settled decisions, with the **why** of each one. Schematics in
> `esquematicos/`. When a decision is revised, the old reason stays on
> record — a premise silently revoked is the most expensive family of bug in
> this project.

---

## 1 · POWER — single input, no boost

```
any PD powerbank ──USB-C──> [PD decoy 15V, 5A] ──> RAIL 15V
                                   ┌──> TPA3116D2 stereo  (+2200µF +100nF)
                                   └──> step-down 5V      (NO bank)
                                              ├──> Daisy VIN
                                              └──▶|── 5V from the ESP32 bridge
                                                SCHOTTKY
```

✅ **SOLDERED AND VERIFIED ON 01/09 — 1N5819.** The bridge's `VIN` at **4,72 V**
(the right drop, with no path bypassing it). **The gate test passed:** powerbank
off with the bridge's USB on, and the amp LED **dark**.

And the **100 µF + 100 nF** ended up soldered on the bridge's pins, **after** the
diode. **A ground of its own to the star was not made, and isn't needed:** 0 mV
AC between its ground and the star, with a tone playing — see
[`ATERRAMENTO.md`](ATERRAMENTO.md) §8.

⚠️ **THE SCHOTTKY ON THE BRIDGE IS NOT OPTIONAL — SS14 or 1N5819, anode on the rail.**

On the ESP32-C3 SuperMini the `5V` pin connects to the USB's **VBUS directly in
the copper**, with no on-board diode. Without the Schottky, connecting the
bridge's USB **feeds back the entire 5 V rail** — measured on 31/08: the amp LED
lit up with the powerbank off, just from the USB cable on the bridge.

It costs ~0,3 V of drop, and the bridge accepts 4,3–6,0 V: there's clearance to spare.

**Why a part and not a rule:** the rule *"power it by USB **or** by the 5 V pin,
never both"* was already written in `esp32/README.md` from the start, and it was
violated anyway — by the one who wrote it. The diode makes the mistake
**impossible**, and a rule that depends on someone reading it does not protect a
gallery piece.

🚫 **And you don't fix this in firmware.** There is no transistor in that path:
`5V` and VBUS are the same node, and no GPIO opens copper. What the firmware
could do was *warn* — and a warning that only detects a USB host stays silent in
front of a dumb charger, which is exactly the gallery case.

**ONE stereo amp, not two.** The design said `amp A` and `amp B` from when they
were two modules; today it's a two-channel TPA3116D2, and the two outputs share
the supply within the board — **one bank at the input serves both**. The
`WIDI VI` also went away: it's the ESP32 bridge.

**Why 15V and not 12V:** 5V/9V/**15V**/20V are the **mandatory** PD profiles;
12V is optional and many powerbanks don't offer it. 15V = any powerbank works,
which was the goal (the powerbank becomes a consumable, not a part of the
project).

**Why without boost:** the 2A MT3608 couldn't handle it — it delivered 6V under
load. A capacitor didn't solve it, and the diagnosis is the entry: *a capacitor
lends energy for a PEAK; if the voltage keeps dropping, what's missing is AVERAGE
current, and no capacitor manufactures power.* The PD decoy doesn't create
voltage, it **negotiates** — the heavy lifting stays in the powerbank's
electronics, sized at the factory.

**Mandatory:** **2200 µF / 35 V + 100 nF glued to the amp's pins.**
Measured on 31/08: without the bank, powering the Daisy from the rail made the
exciter hum, and powering it separately cleaned it up. **In the gallery there is
no separate supply**, so this is a requirement of the piece, not a recommendation.

🚫 **NO BANK AT THE INPUT OF THE STEP-DOWN.** Tried on 31/08 and undone the same
day: immediate **high-pitched squeal**. The amp is a pulsed-current load and
wants the bank; the step-down is a **constant-power** load and does not tolerate it.

📖 The mechanism (negative input impedance, Middlebrook criterion) and the rest
of the decoupling rules are in
[`ATERRAMENTO.md`](ATERRAMENTO.md) §4.

### The two pots have their ends swapped — fixed in firmware

Measured on 31/08: **clockwise LOWERED the volume** on both. It's not software —
it's the order of the two wires at the **ends** of the potentiometer, with the
wiper rising toward the end connected to `GND`.

The fix went into `hal_daisy_seed.cpp`, in the `kKnobInverte[2]` table, and not
in the macro. That's the boundary: **the direction of a potentiometer is a fact
of the copper**, and inverting it right at the read makes everything downstream
— takeover, hysteresis, steps, `Inverso()` — work in corrected coordinates, with
no second place where someone could forget the sign.

If the wires are ever re-soldered in the right order, **it's that table that
goes to `false`** — there's nowhere else to touch.

### Ground, chassis, and decoupling

📖 **The subject has its own chapter: [`ATERRAMENTO.md`](ATERRAMENTO.md).**
Taxonomy of the three kinds of ground, the star, `AGND`/`DGND`, capacitors by
load type, high-impedance inputs, coupling through the air, and the method of
measuring under load. **When this file and that one disagree, that one wins.**

Ground review in the board design: skill `revisao-terra`.

The target design for **this** instrument:

```
   AUDIO ISLAND — signal only
     AGND ── piezos ── DPDT commons ── jack sleeves ── amp input
          ↑ meets DGND ONLY inside the Daisy
   POWER AND LOGIC RAIL
     DGND ── own wire to the star ── pots (Daisy 3V3) ── encoders ── buttons
          ── ESP32 bridge (own wire to the star)
```

### The pot doesn't reach the ends — stops stretched in firmware

Measured on 31/08, sweeping both end to end. **The copper value**, with no
inversion or correction:

| pot | bottom | top |
|---|---|---|
| k1 (L) | 0,0009 | **0,9756** |
| k2 (R) | 0,0005 | **0,9753** |

The bottom reaches nearly perfect; **the top stops at 0,975** — the wiper never
delivers the full 3V3. After inversion that becomes a **floor of 0,025**, which
isn't silence: it's about **−32 dB, audible**. That was exactly what was heard
with the pots at minimum, and the test that proved it was setting
`post_level_l`/`post_level_r` to zero by command — then there was **total
silence**, which also absolved the amp.

The fix is `kKnobFundo` (0,035) and `kKnobTopo` (0,990) in
`hal_daisy_seed.cpp`, applied after the inversion.

⚠️ **The constants sit INSIDE the measured travel, not on it.** That way each end
gets ~1% of a dead zone where the value settles at 0 or 1 and **stays there**. A
stop set at the measured value would give an extreme reachable only by luck and
that ADC noise would make quiver.

And there's a second defect the floor caused, less obvious: with the parameter at
0 and the pot at its minimum of 0,025, the **takeover window was 0,02** — smaller
than the floor. The pot was **permanently unable to retake the parameter**.
Stretching the stops cures both at once.

`knob` now shows `cobre X INV -> vale Y`: the raw value next to the treated one,
because **a correction that erases its own evidence cannot be re-checked**.

### The capacitors of this piece

| consumer | needs | state |
|---|---|---|
| **amp TPA3116** | 2200 µF / 35 V + 100 nF on the pins | ✅ soldered |
| **step-down 5 V** | **nothing at the input** | ✅ removed (it was the squeal) |
| **ESP32 bridge** | 100 µF / 16 V + 100 nF + a ground of its own | ⬜ to be soldered |
| **Daisy** | 100 µF / 16 V + 100 nF | ⬜ recommended |

📖 The why of each value, and the prohibitions, in [`ATERRAMENTO.md`](ATERRAMENTO.md) §4.

**Ground in a STAR, not a daisy chain.** The point is where the PD decoy's `GND`
reaches the amp, and it's from there that it forks to the step-down. The amp's
return is the shortest and thickest wire.

🚫 **Switch only on the positive. Ground never passes through a switch.**

---

## 1b · THE OLED PANEL COSTS 6 dB — that's why it is born dark

Measured on 31/08, with the instrument at rest:

| | panel dark | panel lit |
|---|---|---|
| IN_L | −75,3 dBFS | **−69,3 dBFS** |
| IN_R | −77,7 dBFS | **−71,3 dBFS** |
| spectral peak | 119,7 Hz · dom 0,28 | **7040,8 Hz · dom 0,95** |

Dominance 0,95 is the signature of a **clock**, not of thermal noise. And 7040 Hz
has an exact explanation: **64 lines × ~110 Hz of frame = 7040 line scans per
second**. It's the drive current of the lines, proportional to the **lit**
pixels — that's why the measurement with the pixels dark came out clean.

**The rule (v0.221.0):** the panel is born dark, wakes when someone turns or
presses an encoder, and sleeps after **5 s** of inactivity. During the
performance nobody touches it, so it's dark when the ear is working.

🚫 **You don't fix it with a notch.** A notch from 201 Hz to 7 kHz would be
musically cheap, but it **doesn't scale**: the piece is replicated in a series and
each panel will have its own clock. A dark panel is right on every unit, costs
zero DSP, and leaves the notch bands free for microphonics.

⚠️ **Bench item:** the OLED's five wires (`D5 D6 D7 D8 D10`) run close to the
piezo cables. Worth moving them apart and crossing them at a right angle — same
rule of coupling through the air that applies to the class-D amp output.

## 2 · PIEZO INPUT (×2) — protection only, gain intact

⚠️ **THE ACOUSTIC LOOP IS REAL, AND IT REAPS THE ADC — measured on 31/08.**

Exciter and piezo live in the same body, so they **are** a feedback loop.
Measured with the same touch force, changing only whether the output plays:

| | output playing | output muted |
|---|---|---|
| **IN_L** | **0,0 dBFS · 313 samples reaped** | **−7,4 dBFS · 0 reaped** |

**The loop adds ~7,4 dB**, and it's exactly what's needed to hit the ceiling with
`pre_level_ab` at 0,6. (For comparison: the string gave −17 dBFS and the membrane
−21 in measurements without the loop.)

The loop **isn't a defect** — it's half of what's good about the instrument.
What's no good is the end of it: **reaped at the ADC**, hard digital distortion
with no control.

⚠️ **Today there is no input gain control.** `in_gain` was retired on 31/08 for
being inert — the right decision, but this is the first case that calls for a
serious input trim. While there isn't one, the control is to **lower
`pre_level_ab`** until `pk` stops reporting `clip`.

**Diagnosis:** `pk reset`, play, `pk`. If `clip > 0`, it's reaping. Repeat with
`set pre_level_ab 0` to separate **loop** from **hot source**: if the peak drops,
it's the loop.


```
piezo (+) ──1kΩ──●──> AUDIO IN
                 ├── D1 1N4148 (cathode → 3V3)
                 └── D2 1N4148 (cathode → the node, anode → GND)
piezo (−) ────────────> ground rail (direct, no resistor)
```

- **1/4" TRS** jacks on the panel · tip = signal, sleeve = ground, ring grounded
- **Shielded** wire: center = (+), shield = (−)
- Piezo A → AUDIO IN L → chain A · Piezo B → AUDIO IN R → chain B

**No attenuator, and the reason is one of method:** the measured clip came from a
*finger directly on a loose disc* — a regime that isn't the one of use. On the
sculpture the piezo reads board flexing, a much lower level. **A measurement's
validity has a REGIME.** Divider pads planned on the carrier board, not populated.

**The diodes are invisible below 3,3 V** — they don't attenuate, don't color,
they only wake at the exceptional peak. The two point in the same direction
(cathode toward the more positive side of their pair).

**Entry:** *a physical defect in a transducer perfectly mimics a circuit
asymmetry.* The p2 arriving low was the piezo, not the channel — the test that
separated it was to **swap the piezos between channels**, not to measure deeper.

---

## 3 · OUTPUT — two branches, and the destination belongs to the SWITCH

```
OUT_L ──> [DPDT 3 pos: jack | both | amp] ─┬──> jack TS
                                           └──> TPA3110 PBTL ──> exciter DAEX32QMB-4 (4Ω)
OUT_R ──> same, own switch
```

**The firmware doesn't name a destination — it names an OUTPUT.** `OUT_L` is not
"the exciter" and `OUT_R` is not "the jack": each goes wherever its switch sends
it, and the three positions exist precisely because the destination changes in
operation.

<!-- doc-check: ignore route_exciter (name RETIRED on 28/08, now `out_l`. The mention is deliberate: it is the record of the rename, and deleting the old name would delete exactly what explains why it was wrong.) -->
<!-- doc-check: ignore route_jack (idem, now `out_r`) -->
It was for not seeing this that the params were called `route_exciter` /
`route_jack` until 28/08 — and the name misled *within the session that fixed it*,
when `route_exciter 0` was used thinking it silenced only the transducer. Today
they're called `out_l` / `out_r` (CC 113/114), and they cut the whole branch,
whatever the destination the switch happens to be at.

**Which CHAIN feeds which OUTPUT is a firmware decision**, and it's the 2×2
matrix — `route_a_l`, `route_b_l`, `route_a_r`, `route_b_r`, on CC 97/98 (the CC
says the output, the channel says the chain). It is born full, which reproduces
exactly the hardcoded code that preceded it. It serves to swap the chains when
the transducers are connected the wrong way, in two gestures and without
duplicating any preset — and that's why it **stays out of the presets**: it's a
wiring fact, not a timbre one.

⚠️ **This switch contradicts the rule in the next paragraph, and it's the rule
that's badly worded.** *"Audio does not pass through a mechanical contact"* was
born from a POWER switch (dry contact, does not conduct line level) and
generalized too far. The signal DPDT conducts audio on purpose. The correct rule
is the one the original case taught: **a power switch is no good for switching
signal.**

**The switch lives BEFORE the amp, at line level.** Never after: the TPA3110 in
PBTL is a bridge — **both output terminals are hot**, and touching one of them to
the jack ground kills the amp.

**Permanent 10 kΩ resistor** from the amp's IN+ to GND: a floating input is an
antenna amplified straight into the exciter.

**Architecture rule:** *audio does not pass through a mechanical contact anywhere
in this instrument — the switch governs a GPIO, the firmware cuts the branch.*
Discovered by measurement: a 3-position power switch (marked 12V) has a **dry
contact** and does not conduct line level.

**The exciter is deaf in the low end: 100 Hz is good for measuring, not for
hearing.**

**THE EXCITER'S CURRENT IS NOT MEASURED WITH A SHUNT.** For the same reason the
switch can't live after the amp: in PBTL the output is a bridge switched at
hundreds of kHz, **there's no DC to measure and both terminals are hot** — an INA
there has no valid common mode nor a signal that makes sense. For the exciter: a
microphone, or an AC current clamp.

What answers "how much the amp is drawing" is the measurement **at its input**,
on the amp branch after the fork — see panel 5 of the schematics. And it answers
the ELECTRICAL question: current is a good anomaly detector and a bad judge of
quality. An exciter poorly coupled to the sculpture can draw double and sound
less. For *"is it better?"* the instrument is the ear.

---

## 4 · THE ESP32 BRIDGE — and its cable is NOT optional

🔴 **THE BRIDGE AND THE DAISY NEED A DIRECT GROUND LOOP.**

Measured on 31/08, and it cost an afternoon hunting innocent firmware. The
sequence that produced it: **the ground wire linking the bridge to the Daisy was
removed** (they were left to meet only at the star), and then **the rail supply
was turned off**.

The Daisy was coming from the laptop's USB and stayed alive; the bridge was left
**without power and without a reference**. Its `TX`, still connected to `D14`,
started injecting current through the protection diodes of a dead chip. USART1 is
**the same** for TX and RX, and the undefined line held whoever tried to transmit
— `MidiEspelha()` entered `BlockingTransmit` and did not return.

⚠️ **The defect showed up on the POWERED side.** It was the live board that hung,
because of the dead one. We looked where it manifested, and it lived on the other
side of the cable.

**Symptom:** a stuck LED (phase 2 · sub-phase 6), no serial, and the USB port
taking ~1 minute to open instead of instantly. Reproducible from a clean install.

**Rule, and it's in the chapter:** two boards that exchange signals and can be
powered separately need a **direct ground loop next to the signal wires** — see
[`ATERRAMENTO.md`](ATERRAMENTO.md) §5b. Removing the signal ground to fix noise
is trading a hum for a dead board; noise is fixed with **local decoupling**.

**On the firmware side (v0.216.0):** the transmission became a queue + 3-byte
chunks with a 3 ms ceiling and a **500 ms back-off after three failures**. There
is no longer an unbounded wait in the loop.

⚠️ **NOT TESTED AGAINST THE REAL FAILURE.** It was verified that there's no
regression and that the board survives with the bridge unpowered — but in that
measurement the transmission succeeded, meaning **the defect's condition did not
reproduce**. The test that closes this is to recreate the exact condition: no
direct ground loop, and rail off.

### Flashing: two different DFUs, and swapping them costs an attempt

| gesture | DFU that appears | writes |
|---|---|---|
| **`BOOT` + `RST`** | ST ROM (`@Internal Flash`, `@Option Bytes`) | the **bootloader** |
| **`RST` alone** | Daisy bootloader (`0x90000000`) | the **application**, in QSPI |

🚫 `BOOT+RST` **does not flash the application** — it gives `Last page at
0x9007f663 is not writeable`, because the ROM doesn't know the QSPI. Distinguish
the two by the address `0x90000000` in `dfu-util -l`, **never** by `df11`, which
both share.

## 4b · WIDI CORE — BLE MIDI, 6 wires

> ⚠️ **REPLACEMENT DECIDED, NOT YET EXECUTED.** The WIDI goes out in favor of an
> ESP32-C3 (US$ 45 → US$ 4, on a piece replicated in a series on the order of
> hundreds) — decision and price in [GEAR-AUDIT.md](GEAR-AUDIT.md), the flight in
> [VOO-ESP32-PONTE.md](VOO-ESP32-PONTE.md). **No plan B: the acceptance is the
> gate**, and what it blocks is the order.
>
> This section stays because it describes what's **on the bench today**. It goes
> out whole when the gate passes — and it never leaves `battle_scars.md`, where
> the TX/RX crossover stays alive, because it applies the same to the
> replacement.

| WIDI | Daisy | note |
|---|---|---|
| VI | rail **5V** | not 3V3 |
| ON | jumper from VI | without it, it sleeps |
| GND | ground rail | |
| RX | ← **D13** | |
| TX | → **D14** | ★ essential — it's what receives MIDI |
| LB | → **D28** | link status |
| V3 | (measure only) | ≈3,3 V = module alive |

**DATA CROSSED.** Connecting it straight doesn't burn it out — it gives silence,
identical to a module turned off. If `midi` gives 0 bytes, suspect the crossover
**before anything else**. LG/LR removed (they marked no useful state) → D29/D30
free.

---

## 5 · PANEL v2 (designed, not assembled)

- **OLED Waveshare 2.42" SSD1309**, 128×64, 4-wire SPI, with an on-board voltage
  translator (3,3 V ok). The driver **already exists in libDaisy** (SSD130x).
  Visible area 55,01 × 27,49 mm · module 61,50 × 39,50 mm · MX1.25 7p connector.

  **The seven colors of the harness that comes with the panel**, and the Daisy
  pin for each. It's here because wire color is the only label that survives the
  bench — and on 28/08 a swapped cable with no marking cost a whole diagnosis,
  with the defect attributed to a fix that was right:

  | color | signal | Daisy |
  |---|---|---|
  | red | VCC | 3V3 |
  | black | GND | GND |
  | white | RST | **D5** |
  | green | DC | **D6** |
  | orange | CS | **D7** |
  | yellow | CLK | **D8** |
  | blue | DIN (MOSI) | **D10** |

  Diagnosis on the board: `oled` initializes and lights · `oled off` turns it off ·
  `oled texto` writes (**proves more than the fill**: filling can happen with the
  `DC` stuck, writing requires it to toggle) · `oledvel <2..256>` sweeps the SPI
  speed · `fio curto` hunts for bridges across the ten pairs · `fio <rst|dc|
  cs|clk|din> <0|1>` holds a pin to measure **at the panel's connector**.

  ⚠️ **The panel NEVER responds** — it's pure writing, no MISO. The firmware can
  only say *CONFIGURED*, never *present*. The only detector is the eye.

  ⚠️ **Check the `BS` jumpers on the back of the module.** They choose the
  interface — 4-wire SPI, 3-wire SPI, I²C, or parallel. In I²C or 3-wire the `DC`
  pin stops being `DC`, and the firmware ends up speaking the wrong language to a
  panel that responds just enough to clear the screen and nothing more.
- **EC11 encoder** with detents and push (push = confirm). Relative, not absolute
  — it kills the jump problem of the shared knob.
- **2 volume pots**, one per branch — ✅ **soldered and implemented on 28/08**:
  `A0` = **D15** (branch L) · `A1` = **D16** (branch R). Params
  `post_level_l`/`post_level_r` (CC 116/117), applied **after the limiter**, where
  you can only reduce.

  ⚠️ **There is no possible third pot.** `A0–A4` are aliases of `D15–D19`, and the
  global encoder occupies `D17/D18/D19`. The ADC block is used up.

  **The pot is absolute and the param is shared with MIDI**, so there's **soft
  takeover**: the pot only takes over when it passes through the current value,
  and lets go when the iPad writes. Exception at startup — the first read
  **adopts** the physical position, because there nobody has written yet and the
  truth is the knob. Diagnosis: `knob` over serial.
- **Arcade buttons: three, and only TWO have their LED wired.**
  - `freeze A` and `freeze B` (blue) — button **and** LED. The 5 V LED is driven
    with 3,3 V by the panel's `Q1`/`Q2`: **brightness approved in the real
    environment**; the transistor plan B shelved at no cost.
  - `cowrie shells` (white) — **button only**. You know it's on from the
    **display**, not from light on the button. That's why there are two MOSFETs
    for three buttons, and that's correct.

  > ⚠️ This line said *«30 mm arcade with LED (freeze A and B, cowrie shells)»*,
  > and the natural reading was that all three light up. That led **twice** to the
  > same wrong conclusion — that a `Q3` was missing on the panel — and the second
  > time it nearly cost a via of the IDC 2×13, which is full and whose eight
  > grounds each hold something measured. The cowrie-shells path has been complete
  > since 05/09: panel `J5` → IDC via **19** → Daisy pin **33**.

  **All three are 30 mm.** The centers sit at `x = 40 | 100 | 160` on the lid, and
  the cowrie shells one is in the middle. The «100 mm» that appeared in a note was
  the **position on the axis**, not the diameter — and the cut piece confirms
  three 30 holes.

  ⚠️ **What this costs on stage, and it's said:** the white button **doesn't light
  in the dark**. You know where it is by its position between the two blue ones,
  not by light. If that ever bothers, **one** via is missing in the IDC (not two —
  the button already has 19), and the candidate is **pin 10**, with the measured
  cost of the 6 dB.

**Burn-in:** low contrast + shift 1–2 px every few minutes. The piece may stay on
for months in a gallery.

---

## 6 · CARRIER BOARD rev A (designed, not fabricated)

KiCad → JLCPCB, SMD. Power block with a schematic **verified pin by pin** (netlist
checked against the intended connectivity, automatically).

Critical parts with a verified LCSC number: **CH224K** `C970725` (PD sink),
**TPA3116D2DADR** `C50144` (amp). The current meter is the **INA219**, the same
part as on the bench — LCSC ⚠️ **to be verified**, unlike the other two. The
INA226 left the design on 25/08: bench and production become the same part, and a
number measured on one part doesn't carry over to another.

⚠️ **A 2-wire connector is no good for the input** — PD negotiates through the
CC1/CC2 pins. A pigtail without CC leaves the chip deaf and the powerbank never
leaves 5V: it fails silently, in copper.

---

## Checklist before powering up

⚠️ **Before powering up a new assembly, run the ground review** — skill
`revisao-terra`. Chassis-ground noise mimics a firmware defect, and diagnosing it
afterward cost an afternoon and three accusations of the innocent.

- [ ] Continuity +V ↔ GND: **must not beep**
- [ ] PD decoy measured: **15 V confirmed with no load**
- [ ] Step-down measured at **5 V** before touching the Daisy
- [ ] Capacitor polarity (stripe = negative) and diode polarity (band = cathode)
- [ ] Switch on **+V**, continuous ground in a star, zero loops
- [ ] WIDI's V3 ≈ 3,3 V after powering up
- [ ] Exciter **last**, with `pre_level_ab` low and anti-feedback checked
