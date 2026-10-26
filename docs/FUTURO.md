# FUTURE — what was considered, what it costs, and why not now

> **This document is not a task list.** What is still to be done lives in
> [`STATUS.md`](STATUS.md); the defects, in [`battle_scars.md`](battle_scars.md).
> Here live **possibilities**: things that were considered, with the cost already
> worked out and the reason they were left out.
>
> It exists because the alternative is to rediscover. Three times this week someone
> reopened a question that already had a measured answer, and the answer was in a
> conversation nobody saved.

## The budget, and it is what decides almost everything

**Ten free pins:** `D3` `D4` `D9` `D11` `D12` `D18` `D19` `D20` `D21` `D25`

Each idea below spends from this pocket. And there are two that are **not** in it and never
will be:

🔴 **`D29` and `D30` are the USB data lines** (`USB_HS_D−`/`D+`, pins
36/37 of the header). Using them as GPIO kills the serial — and a board like that gives
running firmware, a good panel, normal power, and no way to talk
to it. We already spent a whole day in that picture for another cause.

`D20` and `D21` are the only free ones that are also **ADC** (`ADC_INP11`,
`ADC1_INP4`) — whoever needs an analog reading spends these two first.

---

## 1 · DPDT sense — `exc_on` reading hardware instead of believing

**Status: discarded on 05/09. Nice to have.**

Today `exc_on` is **software intent**: someone declares "the exciter is in the
circuit" and the firmware believes it. When the declaration and reality diverge,
the anti-feedback protection (shifter + Governor) stays on with no loop to
break — it costs timbre — or off with a loop in the air, which is the case that has already
cost microphonics incidents.

**Why it was left out:** the **type-2 DPDT** uses both poles for the same
channel (commons tied together, as the drawing requires), so **no contact is left over**
to read the state.

**What it would cost:**

| path | cost |
|---|---|
| **3PDT** or DPDT with an auxiliary terminal | 1 pin per channel · a different part in the BOM · the extra pole is clean and is the right route |
| read a switched terminal with a pull-up | 1 pin per channel · ⚠️ puts an MCU pin in contact with the audio path |
| ⭐ **detect by signal** | 0 pins · `AUDIO_OUT` going out without `AUDIO_IN` reacting *is* the signature of an exciter out of the circuit, and SuMiA™ already measures dominance |

The third was not in the conversation and may be the best: **it spends no pin
at all** and uses measurement that already exists. It stays as an idea, not as a plan.

## 2 · Color screen

**Status: possibility. Never budgeted.**

⚠️ **The warning any study has to begin by reading:** the monochrome panel
lit **measured a 6 dB rise in the input noise floor** —
IN_L from −75.3 to −69.3 dBFS, with a spike at 7040 Hz and dominance 0.95. It is
line-driving current, **proportional to the lit pixels**.

A color screen has more pixels, more bits per pixel and a higher refresh
rate. **The cost in noise is not linear and not small**, and in an
instrument whose input is a high-impedance piezo that goes straight to the
signal.

It was because of those 6 dB that the current panel **is born off** and sleeps at
5 s (v0.221.0). A color screen probably cannot stay lit while
playing — and then the question is whether it is still useful.

**What would be measured before deciding:** the input floor with the new screen
on and off, in the usage regime, with the instrument playing — never at
rest, which was the mistake of 02/09 (74.7 dB of margin at rest against 0.4 dB
while playing).

**Pins:** SPI already exists (`D5..D10`). A bigger screen probably asks for more
bandwidth, not more pins — but confirm against the screen's datasheet, not
from memory.

## 3 · The encoders that were removed

**Status: reversible, and the pins are kept.**

There were three; on 05/09 **one** remained, with push+turn giving the other controls.
`D3 D4 D9` (encoder B) and `D18 D19` (encoder G) went back into the pocket.

⚠️ **If any comes back, watch out for `D17`.** It was the push of encoder G and
became the **A2 of the divider** that reads the rail voltage. Returning
encoder G as it was recreates the collision — and it is invisible to the
checkers, because the two uses live in different documents.

And the lesson of encoder G remains, which holds independently: when its
phases were swapped and the count stopped, the defect **was not the swap** — `Init` and
`Debounce` treat `a` and `b` symmetrically, so swapping can only invert,
never zero out. It was the two phases open in the harness.

## 4 · Smaller things, with the cost already known

| idea | pins | note |
|---|---|---|
| **RGB LED** instead of the two monochrome ones | +2 (or 1 with WS2812) | ⚠️ one LED draws milliamps and **is already audible** when the ground is poorly done — that is how coupling was caught on 31/08. An RGB switches more |
| **Fourth arcade button** | +1 | ⚠️ the third **already exists**: the white one of the cowrie shells, on `J5` → IDC 19 → Daisy 33. It has no LED, and that is why there are two MOSFETs for three buttons — it is known to be on via the **display**. The 2×13 IDC is full: a fourth signal requires 2×15, or sacrificing ground — and the **eight** that remain all guard something measured (OLED clock 1/3, ADC reference 8/10, LED return 13/14 that was worth 6 dB, pot returns 23/26) |
| **Expression / pedal input** | +1 ADC | spends `D20` or `D21`, the only free ones with ADC |
| **Second pair of exciters** | — | not a pin, it is power and it is enclosure. The current amp has two channels |

---

## The rule this document serves

> **A possibility without a worked-out cost is a conversation; with a worked-out cost it is
> a choice.**

Each entry here has its price in pins, in measured noise, or in a BOM part.
Whoever reopens one of them starts where the last conversation stopped, and not from the beginning.

And when one of these is done, **it leaves here** — to `FEATURES.md` if it lives,
or to `battle_scars.md` if it taught something along the way.
