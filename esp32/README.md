# esp32/ — the BLE MIDI bridge

> **`ponte` v0.5.0** · ESP32-C3 SuperMini · replaces the WIDI Core 1.3.
> The decision is in [GEAR-AUDIT.md](../docs/operacao/GEAR-AUDIT.md) and the flight in
> [VOO-ESP32-PONTE.md](../docs/operacao/VOO-ESP32-PONTE.md).

**This board is a wire with a radio.** It knows no CC, no param, no
channel. If one day it needs to know what a message *means*, the boundary
has grown and the entire decision comes back to the table.

---

## Wiring — five wires

| ESP32-C3 | Daisy | |
|---|---|---|
| `5V` | 5 V rail | accepts 4.3–6.0 V |
| `GND` | GND | |
| `IO21` (TX) | **D14** (RX) | ★ CROSSED |
| `IO20` (RX) | **D13** (TX) | ★ CROSSED |
| `IO3` (status) | **D28** | **active LOW** |

> ⚠️ **Changed on 21/09 (ponte v1.0.0), and it is MAJOR.** Up to v0.5.0 they were
> `GPIO4/5/6`, chosen here because the bridge lived on five loose wires.
> Since rev C the module plugs into **socket `U3`** and the pins are in copper:
> the board became the source. A bridge with old firmware on a rev C board
> **connects to the iPad and delivers nothing** — the radio does not use these pins.
> `tools/pinos_ponte.py` compares the two and refuses if they diverge.
| `GPIO8` | — | onboard LED, this board only |

**CROSSED DATA.** Wiring it straight does not burn anything: it gives silence, identical to a
disconnected module. If the Daisy's `midi` gives 0 bytes, suspect the crossing **before
everything**. The scar was born with the WIDI and applies the same here.

🔴 **THE DAISY DOES NOT BOOT WITHOUT THIS BRIDGE CONNECTED.** Its `D14` is the RX of
USART1 and, with the bridge out, it floats with the DMA listening — which jams the
transmission and locks the main loop. Measured on 31/08. See
[`HARDWARE.md`](../docs/operacao/HARDWARE.md) §4. **Open defect on the
Daisy side; until then, the bridge is a boot requirement.**

🔌 **SCHOTTKY IN SERIES ON THE RAIL SUPPLY — SS14 or 1N5819.**
Anode on the 5 V rail, cathode on this board's `5V` pin.

On this board the `5V` connects to **USB VBUS directly**, with no onboard diode.
Without the Schottky, the USB cable **feeds back into the whole rail** — measured on 31/08:
the amp's LED lit with the powerbank off. With the diode, you can power
from both with no consequence, and the rule below is no longer necessary.

**Bench, while the diode is not soldered: power via USB *or* via the 5 V
pin, never both.**

⚠️ **MANDATORY DECOUPLING — 100 µF + 100 nF on the `5V`/`GND` pins.**

📖 The why is in [`docs/operacao/ATERRAMENTO.md`](../docs/operacao/ATERRAMENTO.md) §4 and §7.

Measured on 31/08: without them, **the radio comes out through the exciter**. The
transmission bursts are microsecond current spikes, and without a local reservoir
they turn into ripple on the shared rail, which the amp amplifies.

And the symptom has two faces, which makes it look like two defects:

| BLE state | what you hear | why |
|---|---|---|
| **connected** | continuous hum | bursts at the connection interval, fast |
| **disconnected** | click at ~3 Hz | bursts at the advertising interval, ~100–300 ms |

**And its own ground all the way to the star point**, not hung off the ground of the
step-down — a switching converter shakes its own ground, and whoever hangs
off it inherits the shake.

**Do not put 2200 µF here.** It lives at the step-down's output, and a large bank next
to a switching converter is the path to oscillation — see `HARDWARE.md`.

### Why these GPIO

The C3's GPIO Matrix puts the UART on almost any pin — the opposite of the Daisy.
Avoided: `GPIO20/21` (console UART0, needed for debugging), `GPIO2/8/9`
(strapping), `GPIO18/19` (native USB). `GPIO8` is strapping but only at the instant
of boot; after that it is the onboard LED and is used without drama.

### D28 is active LOW, and takes no resistor

Measured in the Daisy firmware: `kLbAcesoEmBaixo = true` and the pin comes up with
`Pull::PULLUP`. Therefore **connected = pull LOW; no link = RELEASE** and
let the internal pull-up take the pin high.

Open-drain by construction: the bridge never drives high. So *"no link"* reads
the same whether the board is off, booting, or disconnected — and that is
the safe direction of failure.

---

## Flashing

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc ponte
arduino-cli upload -p /dev/cu.usbmodemXXXX --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc ponte
```

`CDCOnBoot=cdc` **is not optional**: without it the `Serial` goes out through UART0 (GPIO20/21)
instead of USB, and the `board` becomes invisible.

### Environment known to work

| | version |
|---|---|
| `arduino-cli` | homebrew |
| core `esp32:esp32` | **3.1.1** |
| `MIDI Library` | **5.0.2** |
| `BLE-MIDI` | **2.2.0** |
| `NimBLE-Arduino` | **1.4.3** |

⚠️ **NimBLE 2.x does NOT compile** with BLE-MIDI 2.2.0: 2.x removed
`NimBLESecurity` and changed the `onWrite` signature. If `lib install` brings
2.x, drop down to 1.4.3.

---

## `board` — what the bridge knows about itself

```
ponte v0.1.0 · BAUD 31250 · sem link
  pinos: TX=4 RX=5 STATUS=6 (ativo em BAIXO) LED=8
  quedas: 0 · nenhuma desde o boot
  msgs: BLE->ser 0 · ser->BLE 0 · TX sem espaco 0
```

**The dropouts carry recency** — *"how many seconds ago"*, not just how many. The
question you ask in a gallery is *"is it dropping now?"*, and a count without the
when is not data. As a bonus, it separates assembly from operation without anyone marking
anything: during assembly the dropouts are dense and then cease.

**And they survive the power cut, since v0.5.0.** They lived only in RAM, and that
made them useless for the question that matters in a gallery — *"did it drop in the
early morning?"*. A power flicker, which is **one of the causes of a dropout**, zeroed out
the witness of the dropout.

```
quedas: 0 · nenhuma desde o boot        <- this session: "is it dropping NOW?"
desde sempre: 0 quedas em 4 arranques   <- NVS: "did it drop while I was away?"
```

`arranques` separates the two readings: a high lifetime **with** a high boot count is
unstable power, not an unstable radio. `zera` erases the lifetime memory.

⚠️ **It is not written on every dropout**, for two independent reasons: the NVS has
finite endurance and the piece stays on for months; and the disconnect callback runs
in the context of the BLE stack, where writing flash is long work in the wrong
place. The dropout **marks**, and the loop writes — at most every 10 s. The `board`
shows `(por gravar)` while there is something pending.

**`TX sem espaço` counts and does not stay quiet.** The BLE delivers faster than 31250
baud drains, so the bottleneck is the buffer and not the radio — and the system's biggest burst
is known: the Daisy's `sync` sends 86 CCs = 258 bytes ≈ 82 ms of
UART. The buffer is 1024, a 4× margin.

---

## The baud is a convention, and is defended by procedure

**31250 8N1.** The bridge only passes bytes, so **there is no version handshake**
between the two boards — and there is none on purpose: a handshake would force this board
to speak on its own account in the stream, and then it stops being a wire and becomes an author, with
an authorship window and a message with no human gesture.

The assumed price is that the thin boundary **cannot verify
itself**. The defense is procedural and visible: the number lives in this README, in the
Daisy's README, and comes out in the `board` on each side. **Changing it is MAJOR on both.**

> As long as the bridge only passes bytes, there is no handshake; if it grows for
> another reason, the argument falls and the handshake comes back with the cost already paid.

---

## A clarification about "passes bytes"

BLE MIDI is **framed**: each packet carries a header and timestamps
interleaved. De-framing is mandatory — there is no way not to do it. So the
bridge works at the **message** level: it receives, and re-emits the same on the other side.

What it does not do is interpret the **meaning**. There is no table, no number of
CC written anywhere in this code, and that is the property that matters.
