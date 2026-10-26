# FLIGHT — BLE↔UART MIDI bridge on the ESP32-C3

> **v0.1.0** · 25/08 · replaces the WIDI Core. The decision and the price are in
> [GEAR-AUDIT.md](GEAR-AUDIT.md); here is what gets built and how it's accepted.

**The property that defines flight 1: the Daisy's firmware doesn't change a
single line.** Both modules speak serial MIDI 31250 8N1 over the same D13/D14.

---

## 0 · There are TWO flights, and the separation is what preserves the rule

The architecture discussion attached to flight 1 things that **change the
Daisy's firmware** — in particular the automatic `sync` on D28's rising edge.
That's flight 2. Calling them one would break the property above, and the
discipline of one variable per flight is what has already paid for itself three
times in this project.

| | flight 1 | flight 2 |
|---|---|---|
| what | the bridge | automatic `sync` on reconnection |
| Daisy | **untouched** | changes |
| depends on | nothing | flight 1 being accepted |

---

## 1 · The three checks, done before writing

They cost minutes and changed the shape of the flight. They're recorded because
**an undeclared check will be redone, or worse, presumed.**

### Is D28 read as a level or as an edge?

`LbTick` ([main.cpp:2460](../../src/main.cpp:2460)) samples at 200 Hz and
computes **transitions per 2 s window** and **duty**. There is no instantaneous
edge event.

```cpp
static constexpr bool kLbAcesoEmBaixo = true; // confirmado: V3=3V3, LB=0V
const bool aceso = kLbAcesoEmBaixo ? !nivel : nivel;
```

```cpp
widi_led.Init(seed::D28, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
```

**Two corrections to the original design, and both invert what was written
— see section 3.** And it confirms the split into two flights: detecting a
rising edge is new code on the Daisy.

### Does the authorship window assume a human sender?

**No.** `MidiPushEstado` stamps the window and **re-stamps all params
after the last send** ([main.cpp:616](../../src/main.cpp:616)), with the
arithmetic written in the comment itself: *"the dump takes ~258 ms (86 × 3 ms)
and the window lasts 250, so the FIRST param's expires before the LAST goes out."*

Declared authorship already exists. The second burst sender in the queue — the
preset `load` — is also already handled: **it doesn't use this route**, and
drains through the normal mirror.

**One remains to check: the widened cowrie-shell randomizer.** It enters as a
requirement of its own design, not as a debt of this flight.

### Does `eco_ult` store what's sent or what's received?

It stores a **value**, updated by any origin
([main.cpp:239](../../src/main.cpp:239)):

```cpp
// eco_ult[cc] guarda o ultimo valor visto naquele CC, VENHA DE ONDE VIER
if(eco_ult[id] == v) continue;
```

So: a message lost on the wire leaves `eco_ult` saying "I sent it", the param
doesn't change, and **nothing diverges from the board's point of view**.
Resend-on-divergence would never act — it would be a safeguard in name only.
That's why flight 2 uses **spread-out repetition**, not self-correction.

---

## 2 · Hardware — five wires

| ESP32-C3 SuperMini | Daisy | note |
|---|---|---|
| **5V** | rail 5V | accepts 4,3–6,0 V |
| **GND** | GND | |
| GPIO (TX) | **D14** (RX) | ★ CROSSED |
| GPIO (RX) | **D13** (TX) | ★ CROSSED |
| GPIO (status) | **D28** | **active LOW** — see section 3 |

**DATA CROSSED.** Connecting it straight doesn't burn it out: it gives silence,
identical to a module turned off. If `midi` gives 0 bytes, suspect the crossover
**before anything else**. The scar was born with the WIDI and applies in full here.

**3,3 V logic on both sides, no converter.** The C3 **is not 5 V tolerant**;
the Daisy speaks 3,3 V.

**Choice of GPIOs — the opposite of the Daisy.** The GPIO Matrix puts the UART on
almost any pin, so you choose the ones that fall closest to the existing wiring.
**Avoid:** GPIO20/21 (USB console — needed to flash and debug),
GPIO2/8/9 (strapping), GPIO18/19 (USB).

**There is no ON jumper.** The ESP32 turns on when powered — one less assembly
gesture and one less failure mode, times hundreds of units.

**Bench:** power it by USB **or** by the 5V pin, never both.

---

## 3 · D28 is ACTIVE LOW, and takes no external resistor

> **The ESP32 pulls the pin LOW when connected, and leaves it open (or high)
> when not.** If you invert it, the firmware reads the opposite of reality —
> plausibly, with no error at all. It's the misaddressed message on a single wire.

The original spec said *"high = connected"* and called for a **10 kΩ pull-down**.
Both are wrong. The pull-down was **Code's proposal**, accepted into the spec
without either of the two reading the code — and the record stays because the
reasoning that produced it was sound:

The reasoning was *"not driven has to read as no-link — the safe direction of
failure"*, and it's right. The implementation was the opposite, because
**active-high logic was assumed without reading the source**. With LB active low
and an internal pull-up already present, the safe direction **already exists by
construction**: not driven → high → `aceso = false` → no link.

**The proposed pull-down would have inverted exactly that**, making the floating
pin read "connected" during the ESP32's boot — the failure mode it existed to
avoid, produced by the safeguard against it.

> **Before proposing a safeguard, read the default in the source.** It's the
> same rule that avoided the MISO problem on the OLED, and it wasn't applied
> here. The decision may not exist — and when it already does and is right,
> adding a second one inverts it.

**Practical consequence: no new component.** The Daisy's internal pull-up
already does the job.

### The pin has two consumers, and they want different coins

The machine wants a **level**, to decide. The human wants a **cadence**, to
understand. Serving both on the same wire is family C — a variable with two jobs
— and the fix isn't to choose one, it's to **separate**:

- **D28 = pure level**, identical to LB. No blinking.
- **The cadence goes on a LED of the ESP32's own**, on one of its pins.

Two coins, two wires, zero conflict. And the expressive LED **doesn't cost a
flight** — it's born whole on the ESP32 side, without touching the Daisy.

---

## 4 · The bridge, and what it refuses to do

Library: **Arduino-BLE-MIDI (lathoub)** with **NimBLE** (uses far less
RAM/flash than the standard stack).

The loop is literally: BLE MIDI in → Serial2 (31250) out, and the reverse.

> **No CC map, no params, no interpretation. The bridge passes bytes.**

It's what stops the map from gaining a **second home** — which would be the most
expensive defect this project could invent right now. And it's what makes
the boundary thin enough that drift between the two firmwares is impossible by
construction, not by discipline.

**The price of that thinness, acknowledged:** the thin boundary **cannot verify
itself**. A version handshake would require the ESP32 to speak on its own in the
stream — and then it stops passing bytes and becomes an author, with an
authorship window, a message with no human gesture, and the map gaining the home
the design exists to prevent.

> **As long as the bridge only passes bytes, there's no handshake; if it fattens
> for another reason, the argument falls and the handshake returns with the cost
> already paid.**

The reopening condition lives in the same statement of purpose: whoever arrives
three months from now reads the prohibition paragraph and **stops there**. An
exception in a neighboring paragraph is an exception that doesn't exist for
someone who only finds half.

The safeguard of the convention is **procedural and visible**: the baud declared
in both READMEs and printed by each side's `board`. MAJOR on both if it changes.

---

## 5 · TX buffer — an explicit decision, not a default

**BLE delivers faster than 31250 baud drains.** The bottleneck is the buffer,
not the radio.

The system's largest burst is known: the `sync`, **86 CCs = 258 bytes ≈ 82 ms
of UART**. The buffer is sized by that number.

**Overflow is COUNTED, never silently dropped.** A default inherited from a
library is the same class of unexamined premise that the whole audit attacks.

---

## 6 · Drop counter — with recency

A cumulative count answers *"how many times since boot"*. The real question,
in a gallery, is ***"is it dropping now?"*** — and **data without the when is not data**.

It's implemented with a sliding window or the timestamp of the last drop.

And it brings a bonus: **it distinguishes assembly from operation without anyone
marking anything.** During assembly the drops are dense and then cease; in a
gallery with interference, they reappear spaced out. The same reading separates
*"it was being assembled"* from *"it has a problem"* — no state, no mode, just
the when.

---

## 7 · Before the gate: the Loopy profile must be regenerated

**The 86 bindings are tied to the device's identity.** The exported file
is called `227f559e.MIDI.Cotucatronics1 Bluetooth.controllerprofile`
— and that `227f559e` is the identifier Loopy assigned to the WIDI.

A new BLE device is, to Loopy, **a different device**: the 86
bindings don't apply and the widgets stop responding.

⚠️ **The symptom of this is indistinguishable from the bridge not working.** It
pairs, connects, you move a slider, nothing happens — and the natural verdict
is *"the ESP32 doesn't work"*. **The gate would reject the right thing for the
wrong reason**, and without a plan B that costs dearly.

It's a defect that lives inside no single part: the ESP32 is fine, Loopy
is fine, the profile is fine. **It's the relationship between the profile and the
device's identity that broke**, and nobody has it in a file.

**It's already solved, and the procedure is short** — the format was deciphered
on 24/08 and `Device`/`Device Name` are two text fields in the plist:

1. turn on the ESP32 and let Loopy see it;
2. export any profile to discover the new identifier;
3. regenerate with [gerar_perfil_loopy.py](gerar_perfil_loopy.py) for that
   identifier;
4. import.

**It has to happen BEFORE the gate, otherwise the gate measures something else.**

---

## 8 · THE GATE — and the two kinds of criterion don't mix

Without a plan B, the acceptance **blocks assembly and authorizes the order**. It
runs on a protoboard, before soldering and before buying in quantity.

### Judgment — from dB's ear, and binding

- [ ] **Latency in the real gesture.** dB turns a knob and judges. BLE MIDI sits
      at 3–15 ms depending on the interval iOS negotiates; for remote control it's
      tolerable, but **the one who decides is the ear**, not the number.

> The judgment is **made once and binding**. Approved, it isn't reopened later
> with numbers; rejected, it blocks. What kills a gate isn't having a subjective
> criterion — it's having a subjective criterion that can be revisited until it
> gives the desired result.

### Measurement — doesn't depend on anyone watching

- [ ] **Twenty minutes of operation with zero drops in the window.** The counter
      was born as serial protection, became gallery diagnostics, and pays for the
      acceptance criterion it itself made possible.
- [ ] **Pairing** with Loopy as simple as the WIDI's: it appears in the
      Bluetooth MIDI settings, connects, stays.
- [ ] **Burst:** `sync` pushing 86 CCs at once, with no loss. `midi`
      counts bytes/msgs/applied and `vistos` names what arrived per channel.
- [ ] **The six channels pass through intact** — ch1/2/3 input, ch4/5/6
      mirror. The bridge is transparent, but confirm it.
- [ ] **D28 tracks the real state**, with the polarity from section 3:
      disconnect the iPad and the level changes in the right direction.
- [ ] **Recovery:** cut power to the ESP32 during a gesture. Does it come back
      on its own? How long? Does the Daisy notice?

On recovery: **the Daisy's state survives** — the bridge keeps nothing. The harm
is Loopy's view going stale, and `sync` is the fix. What's missing is who
triggers it, and that's flight 2.

---

## 9 · What NOT to do in this flight

No OSC, no web editor, no presets in the ESP32's flash, no ESP-NOW, and no
touching the baud. They're all available and none blocks anything — they come in
when they become a real need, with a clear requirement.

**This flight proves the bridge and stops.**

---

## 10 · Where it lives

`esp32/` in the same repo, versioned together. **A second firmware = a second
version**, and each side's `board` declares its own.

While it's only a bridge, the doc is thin: a README with wiring, chosen pins, and
how to flash. If it ever grows into OSC or a web editor, the boundary fattens and
demands the same rigor as the main one.

---

## 11 · FLIGHT 2 — a sketch, so it isn't lost

Depends on flight 1 being accepted. **It changes the Daisy.**

**Automatic `sync` on D28's rising edge**, with the sweeps **spread out**
(`t=0`, `~300 ms`, `~1 s`) and not bunched together: the failure mode is
**systematic** (the MIDI port still closed), not random — three sweeps bunched
within 250 ms fall entirely inside a dead 500 ms window.

> Spreading is **coverage** (it fails gracefully); a fixed placement is a
> **threshold** (it fails binary and loses everything).

**Do not call `MidiPushEstado`.** It does `hal::DelayMs(3)` per message and
**blocks the loop for 258 ms**. With a human triggering it, they know they
triggered it; automatic and spread out would be **three 258 ms stalls with no
explanation** after each reconnection — encoders stalled, buttons stalled, and
`LbTick` itself stalled, meaning **the link detection goes blind precisely
while it responds to the link**.

The fix already exists in the code: **the `load` route** invalidates
`eco_ult[id] = 0xFF` ([main.cpp:346](../../src/main.cpp:346)) and lets the
mirror drain on its own — 2 messages per 8 ms, without blocking, with the window
stamped at the instant of each send, which is the correct stamp for staggered
draining. The spreading becomes **three instantaneous invalidations**.

**A spread in progress absorbs a new edge by RESTARTING**, not by ignoring: if
there was a drop, the earlier sweeps went to a dead link and are worth nothing.
Same logic as the safe level — assume it didn't arrive.

**To check when flight 2 arrives:** with staggered draining (~344 ms for
the 86) and invalidations spaced ~1 s apart, there's overlap. Invalidating is
idempotent — the param that hasn't drained yet drains once — but **confirm
that the mirror handles invalidation during draining** without double-counting
or losing one.

**`sync` reports what it did** — how many sweeps, when, triggered by an
edge or by a command, and whether it was restarted. Without that the count
doesn't reconcile with any possible spread, and whoever reads a month from now
finds an orphan number.

> The criterion is **repairable AND announced**. A stale screen is recoverable
> but **doesn't announce itself**: a wrong widget is indistinguishable from a
> right widget, and only someone who knows the expected value at that instant
> notices. A repair that depends on someone noticing a defect that doesn't show
> itself is a theoretical repair.

**To declare in `HARDWARE.md`:** if the link drops and someone moves a widget,
`sync` pushes the board's state back and the widget jumps. **It's not the
instrument undoing the gesture — it's revealing that the gesture never arrived.**
The widget changed position; the param didn't. Undeclared, it becomes an
"intermittent Bluetooth bug" in a future session.
