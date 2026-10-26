# GEAR-AUDIT — WIDI Core → ESP32-C3

> **v0.1.0** · 25/08 · the decision to replace the BLE MIDI module, with the
> price of each side. When this decision is revisited, the old reason stays
> — a premise revoked in silence is the most expensive bug family in this project.

The flight that executes this decision is in
[VOO-ESP32-PONTE.md](VOO-ESP32-PONTE.md).

---

## 1 · The number, and why it decides

| | WIDI Core 1.3 | ESP32-C3 SuperMini |
|---|---|---|
| unit cost | ~US$ 45 | ~US$ 4 |
| firmware | closed | ours |
| status pin | the manufacturer's semantics | our semantics |
| ON jumper | required | does not exist |
| debugging | none | USB console |

**It is not 30% cheaper — it is an order of magnitude.** And the instrument is
replicated in a series, on the order of hundreds: forty-one dollars per unit,
times hundreds, is the difference between a viable project and an expensive prototype.

In a single unit the swap would not be justified. It is the **scale** that
makes it mandatory, and that is why the requirement is written as scale and not as a
technical preference.

---

## 2 · What the WIDI delivered, and what it charged

It delivered one real thing: **it worked without anyone writing a line.**
It pairs, connects, passes MIDI. To get this far, that was worth the price.

It charged four:

- **A closed box.** When it failed, it failed. There was nothing to read, to
  instrument, to correct. The diagnosis ended at "the module is not
  responding" and there was no next step.
- **The LB pin said what the manufacturer decided it said.** Active low,
  with no documentation of what distinguishes *advertising* from *connected* from *error*.
  We measured transitions and duty to infer — inference over someone else's
  decision.
- **The ON jumper.** An assembly gesture, a failure mode ("it is on
  but asleep"), and a checklist line, times hundreds of units.
- **31250 baud as law.** The serial had to be MIDI because the other end
  was MIDI. There was no decision to make — and there was no way to know that it was a
  decision, because it was on the other side.

---

## 3 · What becomes ours — and is free

The line that separates what the migration gives for free from what it merely
**makes available** is a single one:

> **Everything that lives only on the ESP32 side is free. Everything that crosses is expensive.**

On the free side, and already incorporated into flight 1:

- **The cadence of the status LED.** Solid, blinking, an error rhythm — our
  choice, on an LED of the ESP32 itself, without touching the Daisy.
- **The sizing of the TX buffer**, by the system's largest known
  burst, instead of a default inherited from a library.
- **The count of link drops, with recency** — which answers *"is it
  dropping now?"* and not *"how many times since boot"*.
- **The behavior on reconnection**, including the report of what was done.
- **The name advertised over Bluetooth.**

None of that did the WIDI allow, and none of that costs a line of the Daisy. **The
migration already charges the freedom it was meant to charge.**

---

## 4 · What is recorded and NOT built

**The 31250 baud stop being law and become our decision.** Today they are
the WIDI's imposition; after the migration both ends are ours, and the number
could go up with a direct gain on the burst.

It is not changed now, and the reason has a named price: **changing the baud drags
the Daisy along**, and the moment it drags, a compatibility matrix is created
between the versions of the two firmwares — an old ESP32 with a new Daisy goes dead,
with a symptom **identical to a swapped wire**. On a piece replicated in the hundreds that
is a permanent cost, not a bench inconvenience.

> This is **bts 12g in hardware**: separating what is LAW from what is OUR
> DECISION. The number was law while one of the ends belonged to a third party. It stopped
> being — and passing as law is exactly what the scar says to separate.

Also not entering this flight: OSC, web editor, presets in the ESP32's flash,
ESP-NOW. All available, none blocks anything, all enter when they
become a real need with a clear requirement.

---

## 5 · No plan B — and what that means in fact

**The WIDI goes for good. It does not stay in a header, it does not stay as a reserve.** With that the
acceptance of flight 1 stops being a confirmation and becomes a **gate**: failing a
criterion blocks the assembly instead of postponing it.

Three consequences the decision drags along:

**Plan B did not disappear — it changed nature.** Without BLE, the instrument
is not left without MIDI: it is left **without wireless MIDI**. D13/D14 are UART, and a
wired MIDI TRS or DIN jack works with no radio at all. On stage, the difference
between *"there is no instrument"* and *"there is an instrument with a cable"* is everything.

**Failing stops meaning a deadlock.** The WIDI was a closed box: when it
failed, there was nothing to debug. The bridge is our code, with a public library
and a USB console. Failing comes to mean *"there is a bug and it has an
address"* — and **eliminating the black box is what makes it tolerable to have nowhere
to fall back to**.

**The gate authorizes the purchase.** In a series of hundreds, what the acceptance
blocks is not the bench: it is the order. The gate on protoboard, **before** soldering
anything and before buying in quantity.

---

## 6 · What leaves the documents, and what never leaves

When the gate passes, the WIDI leaves `HARDWARE.md`, `PINOS.md`, the
README and the schematic — everything that describes the **current state**.

**It does not leave `battle_scars.md`.** The scars do not document the present: they
record what it cost to get here, and erasing them would erase the lesson along with the
part.

And one of them is not even historical: **the TX/RX crossing is still alive.** It was born
with the WIDI and applies entirely to the substitute — the ESP32 also connects TX to D14
and RX to D13. Erasing it by association would be losing the lesson at the exact moment
it becomes necessary again.
