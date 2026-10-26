# PIN MAP — Cotucatronics

> ⚠️ **05/09 — the board is the Daisy Seed3, not the rev4.** The pinout is the
> SAME: the STM32 name matches on all 40 pins between the two manufacturer CSVs,
> and the doc says *"maintaining pin-to-pin compatibility with the original Seed
> pinout and footprint"*. Everything this file says still holds. The source is
> now `kicad/cotuca_cerebro/Seed3_pinout.csv`, versioned in the repo.
>
> The practical difference is just one, and it is in our favor: the codec
> becomes the **TI TAC5242** (32 bit / 192 kHz, -120 dB). The connector is USB-C,
> but the bench has flashed over USB-C ever since the first board burned — it
> changes nothing in the flow.


> Verified in `src/hal_daisy_seed.cpp` and `src/manifest.h` @ `hal v0.74.1` (04/09).
> **A pin is confirmed in the source, not in memory.** If this document and the
> code diverge, the code is right and this document is old.

## Occupied

| pin | function | how |
|---|---|---|
| **D13** | UART TX → **RX** of the ESP32 bridge | PB6 · USART1 · 31250 8N1 · ★ CROSSED |
| **D14** | UART RX ← **TX** of the ESP32 bridge | PB7 · ★ CROSSED · it is the one that receives MIDI · **INTERNAL PULL-UP ON** (~40 kΩ) since 02/09 — see below |
| **D15, D16** | pots ×2 (A0, A1) | ⚠️ **A0–A4 are ALIASES of D15–D19** — the same physical pin. **Only D15 and D16 are pots**; `board` says `knobs:2`. Both with **inverted** reading (`kKnobInverte`) — measured at both extremes, raw 0.0008 at the maximum and 0.9753 at the minimum |
| **D0, D1, D2** | **the** encoder — phase A, phase B, push | `kEncPin[0]`, and it is the only one since 05/09 |
| **D17** | **A2 — 100 k/22 k divider of the 15 V rail** | voltage reading; the firmware refuses to turn on the amp below 12 V |
| **D5, D6, D7, D8, D10** | OLED SSD1309 SPI | see the panel table, further down |
| **D22** | button B4 — freeze B | INPUT + PULLUP, active-low |
| **D23** | LED — freeze B | OUTPUT, follows the param `frz_on_b` |
| **D24** | button B2 — freeze A | INPUT + PULLUP, active-low |
| ~~**D25**~~ | ~~button B3 — CTOS reset~~ · **REMOVED 05/09** | the function stays with the serial's `b3`; the panel carries no button |
| **D26** | button B1 — cowrie shells (randomizer) | INPUT + PULLUP |
| **D27** | LED — freeze A | OUTPUT, follows the param `frz_on_a` |
| **D28** | STATUS of the ESP32 bridge (BLE link) | INPUT + PULLUP · active LOW, open-drain |

> ✅ **THE `D17` CONFLICT DIED ON 05/09, and not by a fix.**
>
> The `D17` was the push of encoder G *and* the A2 of the rail divider — two
> uses written, both intentional, and no checker saw them because they lived in
> different documents. I proposed two ways out, and **neither was needed**: dB
> closed the interface on a **single encoder**, encoder G left, and `D17` became
> free for the divider without touching firmware that worked.
>
> **An interface decision dissolved a hardware problem.** It is written down
> because the next collision may have the same way out, and nobody looks for it
> from that side.

## Free

**D3** · **D4** · **D9** · **D11** · **D12** · **D18** · **D19** · **D20** · **D21** · **D25** — ten.

They went from four to ten on 05/09: encoders B and G left (`D3 D4 D9 D18 D19`)
and the reset button (`D25`). The `D17` is **not** here — it is the A2 of the
divider.

⚠️ **`D29` and `D30` left this list on 04/09: they are the USB data lines**
(pins 36 and 37, `USB_HS_D−`/`D+`). See the warning below.

> ⚠️ This line said **`D0–D12`** until 04/09, and was wrong in two ways at the
> same time: D0–D4 and D9 are encoders A and B, and D5–D8 and D10 are the OLED —
> which this same document lists as occupied three sections below. **The document
> contradicted itself**, and nobody noticed because the two statements are never
> read in the same pass.
>
> Caught by the pin exam of the `bts`, which is what it exists for: reading the
> source and the document side by side instead of trusting either one of them.

⚠️ **Do not confuse it with `kLivresPin` in the source** (`hal_daisy_seed.cpp`).
That list is called "free" but means **"that the sweep can reconfigure without
killing the OLED or the MIDI"** — and it includes D17/D18/D19, which are encoder
G. The name lies; the behavior is intentional (the sweep exists precisely to
discover encoder wiring).

> 🔴 **D29 AND D30 CANNOT BE USED. THEY ARE THE USB.**
>
> Discovered on 04/09, while generating the Seed symbol from the manufacturer's
> `Daisy_Seed_Rev4_Pinout.csv`:
>
> ```
> pino 36  →  D29  =  USB_HS_D_-   (PB14)
> pino 37  →  D30  =  USB_HS_D_+   (PB15)
> pino  1  →  D0   =  USB_HS_ID    (PB12)
> ```
>
> They are **the data lines of the USB itself** — the ones the connector uses
> and through which the serial passes. Reconfiguring them as GPIO kills the port,
> and the defect would only appear with the board built: firmware running, panel
> good, and no way to talk to it. We spent a whole day in that picture for
> another cause, and we know what it is like.
>
> **This document used to reserve them for the DPDT sense.** They do not go there.

**The DPDT sense was DISCARDED on 05/09.** The type-2 DPDT uses both poles for
the same channel (commons tied), so there is no contact left to read the state.
`exc_on` remains **a software intention** — the firmware believes what it is
told. The options and what each one costs are in
[`FUTURO.md`](../FUTURO.md) §1, including one that spends no pin at all.

⚠️ **`D0` is the `USB_HS_ID`** and is being used as phase A of encoder A. In
device mode the ID pin is not used, so it works — but it is written down,
because it is the same family and nobody knew.

## Reserved for panel v2

| block | pins | note |
|---|---|---|
| OLED SSD1309 SPI | 5 | **SOLDERED 23/08** — see the table below. SPI1, mode 0, MISO unassigned |
| Encoder EC11 | 3 (A, B, push) | ✅ **THEY ARE NOW THREE AND CONNECTED** — see the `kEncPin` in the occupied table |
| Volume pots ×2 | 2 (ADC) | ✅ **CONNECTED** on D15/D16, one per exciter |

## The hardware rule that cost a flashing

> **GPIO is remapped in the source (two numbers, one flashing).
> UART, SAI, ADC and QSPI are NOT — the function is tied to the pin by the silicon.**

The difference **does not appear in the schematic**, only in the chip's
alternate-function table. Inverted LED/switch cost a flashing; getting the MIDI
back required a new wire.

## Button table (single source)

`AddBotao()` in `manifest.h` ties HAL index → param → LED:

| index | name | switch | LED | target |
|---|---|---|---|---|
| B1 | cowrie shells | D26 | — | randomizer |
| B2 | freeze A | D24 | D27 (LED 0) | `frz_on_a` |
| ~~B3~~ | ~~CTOS reset~~ · **REMOVED 05/09** | ~~D25~~ | — | the function stays with the serial's `b3`; the panel carries no button |
| B4 | freeze B | D22 | D23 (LED 1) | `frz_on_b` |

**Debounce: 30 ms, MEASURED** — at 10 ms, five hits turned into six.

**The LED follows the PARAM, never the gesture.** If the freeze changes via CC,
serial or CTOS, the LED tells the truth. Verified over serial, without touching
the button.

## OLED SSD1309 — soldered and occupied (23/08, v0.154.0)

| signal | pin | port | origin |
|---|---|---|---|
| CLK | **D8** | `PORTG11` | libDaisy default |
| DIN | **D10** | `PORTB5` | libDaisy default |
| CS | **D7** | `PORTG10` | libDaisy default |
| DC | **D6** | `PORTC12` | **overridden** (default would be D9) |
| RST | **D5** | `PORTD2` | **overridden** (default would be D30) |

⚠️ **THE LIT PANEL COSTS 6 dB OF NOISE FLOOR AT THE INPUT** — measured on
31/08: IN_L from −75.3 to −69.3 dBFS, with a spike at **7040 Hz** and dominance
0.95. It is the drive current of the lines (64 lines × ~110 Hz of frame),
proportional to the **lit** pixels.

So, since v0.221.0, **the panel is born dark**: it wakes when someone moves an
encoder, lights up 200 ms after the hand stops, and sleeps at 5 s. See
[`HARDWARE.md`](HARDWARE.md) §1b.

**Bench item:** these five wires run near the piezo cables. Keep them apart, and
cross them at a right angle — air coupling is not fixed with a ground scheme.

**D5, D6, D7, D8 and D10 left the free pool.** The **D9** remains free — it is
what the default would use for DC and the soldering did not use.

**Putting RST off the default is the right choice and worth recording why:** the
default is `PORTB15` = **D30**, and D29/D30 are reserved for reading the
exciter's DPDTs. Using the default here would create a dispute that would only
appear on the day of soldering the panel.

**MISO is not assigned, and it cost no decision.** The libDaisy default is
already `direction = TWO_LINES_TX_ONLY` with `miso = Pin(PORTX, 0)` — the house
"no pin". The display is pure writing.

> ⚠️ **Consequence that runs through every diagnosis of this panel: it is NOT
> DETECTABLE.** There is no read path, so no command can say
> "OLED present". The `board` says **configured**, and the difference is not
> pedantry — it is the same line as `placa viva: sim` that has already lied here.

## MAP CHECKED BY MACHINE

<!-- pinos: inicio -->
| pin | owner |
|---|---|
| D0 | encoder A phase A |
| D1 | encoder A phase B |
| D2 | encoder A push |
| D3 | encoder B phase A |
| D4 | encoder B phase B |
| D5 | OLED RST |
| D6 | OLED DC |
| D7 | OLED CS |
| D8 | OLED CLK |
| D9 | encoder B push |
| D10 | OLED DIN |
| D13 | MIDI TX |
| D14 | MIDI RX |
| D15 | **pot L** (A0) — volume of branch L |
| D16 | **pot R** (A1) — volume of branch R |
| D17 | encoder G push |
| D18 | encoder G phase **B** |
| D19 | encoder G phase **A** |
| D22 | button B4 freeze B |
| D23 | LED freeze B |
| D24 | button B2 freeze A |
| D25 | button B3 CTOS reset |
| D26 | button B1 randomize |
| D27 | LED freeze A |
| D28 | STATUS of the ESP32 bridge (active LOW, open-drain) |
| D29 | RESERVED — DPDT exciter |
| D30 | RESERVED — DPDT exciter |
<!-- pinos: fim -->

**Free:** D11, D12 (I2C_1 for the INA219), D20, D21.

⚠️ **The ADC block is used up.** `A0–A4` are aliases of `D15–D19`: the pots
occupy A0/A1 and the global encoder occupies D17/D18/D19. **There is no possible
third pot** without changing the panel — and that is why `num_knobs` is 2 and
not 5.

> **This table is checked by `make check-docs`** against the
> `seed::Dnn` of the `hal_daisy_seed.cpp`. A pin claimed in the code and absent
> here, or here and not used there, becomes an accusation.
>
> The routine exists because **the board is clogged and the solder points
> squeezed**: checking from memory is what fails exactly when the cost of being
> wrong is highest. And because an assumed map has already cost hours — the
> global encoder was on D17/D18/D19 and the firmware measured D20/D21, so every
> instrument pointed at the wrong place and none could say so.

---

## D14 has an internal pull-up, and it is not a detail

The libDaisy initializes the UART pins with **`GPIO_NOPULL`**
(`src/per/uart.cpp`). With the ESP32 bridge unpowered its pin is high-impedance
and our RX **floats** — and a loose CMOS input picks up noise at MHz. The USART
storms **by rate** (FE/NE/ORE at the pace of the interrupt service), the CPU
stays in the ISR and the loop starves to death.

Measured on 01/09: the board stopped responding to the serial in **less than
4 s** and **did not come back** with the line already at rest.

The rest state of a UART line is **high, by definition**. If the transmitter
disappears, someone has to hold the high. The `hal_daisy_seed.cpp` reconfigures
PB7 with `GPIO_PULLUP` right after the UART's `Init` — with ~100 pF of cable the
rise is ~4 µs against 32 µs per bit.

**Known and accepted side effect:** with the ESP32 off, the pull-up injects
~80 µA through its pin's protection diode. Harmless at that level; with an
external 10 kΩ it would be ~330 µA.

**If anyone rewrites the MIDI `Init`, this line has to survive.** Without it the
defect comes back, and it comes back in a form that looks like firmware.

See battle_scars **15az**.
