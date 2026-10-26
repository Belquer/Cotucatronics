---
name: bancada-firmware
description: Bench routine for any embedded firmware project — entering the bootloader by software instead of by button, checking that the version on the board matches the one in the source, and not confusing tool output with confirmation of intent. Use when flashing, when building a build/flash cycle, or when a flashing cycle is repeatedly costing time.
---

# Firmware bench — the three rules

## 1. LOOK FOR THE SOFTWARE RESET BEFORE ACCEPTING THE BUTTON

**Before building anything around a physical RESET, look in the platform's API for
how to enter the bootloader by command.** Almost every modern MCU has one — and
almost every project discovers it late.

```
STM32 / libDaisy   System::ResetToBootloader(DAISY_INFINITE_TIMEOUT)
Generic ARM        NVIC_SystemReset() with a flag in uninitialized RAM
RP2040             reset_usb_boot()
ESP32              esp_restart() + boot pin, or OTA
AVR/Arduino        watchdog reset, or a DTR toggle by the serial itself
```

**The gain is an order of magnitude, not incremental.** On a real project: the
flashes took **119, 66 and 47 attempts** fighting for a ~2 s window after the
button. With the software command, the very next flash took **1**.

**And the infinite timeout matters more than the reset.** Entering the bootloader
with a short window only moves the problem; entering and *waiting* eliminates the
coordination between two parties.

> **Repeated friction becomes scenery.** The 2 s window was accepted as a fact of
> the bench from the first session, and the whole flow — retry loops, "press RESET"
> requests, timing synchronization — was built around it. The solution was in the
> library's header the whole time.
>
> **When a friction repeats for the third time, ask whether it needs to exist** —
> before automating the workaround.

## 2. CHECK THE VERSION: IN THE SOURCE AND ON THE BOARD

**Every flash ends by comparing two strings.** The firmware must print the version
**at any time**, not only in the boot banner — whoever connects the serial after
boot misses the banner.

```
board:   <status command>   ->  "Projeto vX.Y.Z"
source:  head -3 main.cpp    ->  "vX.Y.Z"
```

If they do not match, the edit did not go in — **no matter how much the flash said
it flashed.**

*It cost three "fixed" announcements with the source stuck on an old version: the
edits were dying in an anchor `assert`, and only the flash result was being checked.*

**If the board cannot state its own version outside boot, fix that first** — a rule
that cannot be executed is not a rule.

## 2b. BOARD STATE DOES NOT SURVIVE THE FLASH

Params, calibration, log mode — everything volatile dies on re-flash. On this
project the params **all start at zero by dB's rule**, so after flashing the board
is always in the neutral state, never in what was configured.

Case (08/20): I configured the detune, flashed to fix something else, and announced
*"it's on and ready"* repeating a state from before the flash. The effect had its
switch at zero and never played.

> **Either you re-read the state after flashing, or you assert nothing about it.**
> The re-flash is a boundary, and an assertion does not cross it.

Re-reading costs one command:

```
list
```

**Corollary — a boot alarm needs an on-demand twin.** The table-overflow report
had existed for nine versions and did not prevent the second occurrence, because it
only came out in the banner: whoever connects the serial afterward never sees it.
Every boot alarm gains a path consultable at any time (here, `board` came to print
`tabelas: N/teto params | N/teto CCs`).

## 2c. FLASHING DESTROYS STATE — RESTORING AND DECLARING IS PART OF FLASHING

§2b covers the **reporting** error (asserting state the flash erased). This one
covers the **action** error, which is worse because the other side has no way to
know it happened.

Case (08/20): four flashes in the same conversation, each one zeroing `level`,
none restored nor announced. dB re-dialed through Loopy without knowing why, and one
of his questions ("no sound on p2") was born from a silence that was mine.

> **Flashing does not end at `File downloaded successfully`.** It ends at restoring
> the operating state and **saying which state was restored**.

Minimum to restore and declare: `level`, `in_gain`, and the switches of whatever was
playing. If the decision is to leave it zeroed (a dry test, for example), **say that
too** — deliberate silence and accidental silence are indistinguishable from the
other side.

### And on 09/04 this rule stopped depending on my remembering it

It had been written here since 08/20 **and failed all the same**, because a rule
that only exists as text is executed by memory — and memory is not an instrument. dB
made it permanent: *"always do this, when flashing restore the values"*.

> **A bench rule that depends on remembering is not a rule, it is luck.** The fix is
> to put it in the script that is run anyway.

In Cotucatronics it lives in `flash.sh` (v0.4.0), which calls `estado.py` on **both
sides** of the DFU:

```bash
python3 estado.py guardar build/estado_antes.txt   # BEFORE the `dfu go`
# ... DFU, flash, version check ...
python3 estado.py repor   build/estado_antes.txt   # AFTER
```

The `guardar` has to come **before** the `dfu go`: two steps ahead the board is in
the bootloader and does not respond to `list`.

**Three traps, all found while building the thing:**

1. **Find out which unit the write command speaks, by READING it.** Here the `set`
   accepts the **real value** (`set gate_release_a 300`), not the normalized one.
   v0.1.0 sent `norm`, and for the params whose real range happens to fall in `0..1`
   it coincided and worked — only the others were rejected. A class of defect that
   works in half the cases is worse than one that never works.
2. **Read the response of each write.** It is the only thing that knows whether it
   took: `nome -> real=...` or `fora da faixa: aceita [...]`. Discarding it made the
   tool print "restored" for six restorations that never happened.
3. **Check the precision of what you read.** The `list` printed `%.2f` — great for a
   human eye, and wrong as a restoration source (±7.5 Hz in a notch of 0–1500 Hz).
   **An instrument changes contract when it changes reader**: it was a read for a
   human, it became a machine input, and no one re-evaluated.

And what the tool does **not** do: restore what the flash changed on purpose. If a
new version is born with a different factory value, restoring would be undoing the
change on the sly — so everything it touches is printed with the before and the
after, and what it does not recognize (a param that vanished, a new param) is said
out loud instead of ignored.

## 2d. IDENTIFY THE BOARD BY ITS RESPONSE, NEVER BY THE PORT NAME

With two boards on the USB, the by-name selector (`if '11201' not in x`) is a bet on
how the operating system is going to enumerate today. On 09/01 the ESP32 showed up as
`usbmodem101`, passed the filter, and **came first in alphabetical order**: five
configuration commands went to the BLE bridge.

```python
# the right port is the one that RESPONDS
for p in sorted(glob.glob('/dev/cu.usbmodem*')):
    s = serial.Serial(p, 115200, timeout=1); time.sleep(0.8)
    s.write(b'board\n'); time.sleep(1.0)
    if 'Cotucatronics OS' in s.read(300).decode('utf8','replace'):
        return s
```

And the aggravating part was not the selector, it was the report: the script printed
`notches repostos: ...` **unconditionally**, so it said the same thing whether it had
worked or not. **A bench helper script obeys the same rule as the firmware: you do
not print "done", you print the read-back.**

## 3. TOOL OUTPUT ≠ CONFIRMATION OF INTENT

They are **different** confirmations, and each covers only its own piece:

| Confirms | Does not confirm |
|---|---|
| the loop's `GRAVOU` | *which* binary went up |
| the anchor's `assert` | that the file was written |
| a clean build | that the new code is in the build |
| `grep` in the source | that the board has the source |

**Uncomfortable corollary:** the better the protection against silent editing
(asserts, anchors), the easier it is to **not notice that it acted** — the effect of
a barred edit is the file staying the same, and a file staying the same makes no
noise.

## 4. If the manual RESET is unavoidable: loop first, button later

The bootloader window lasts ~2 s. **Start the flashing loop BEFORE asking for the
tap** — asking for the event before being ready to receive it is the same family of
ordering bug as "configure before you consume", applied to two people instead of to
two functions.

```bash
# loop in the background, trying 1×/s
for i in $(seq 1 120); do <flash> && break; sleep 1; done &
# only THEN: "press RESET"
```

## Before the first exploratory flash: declare the known good

On 08/22 an `APP_TYPE` swap produced a reproducible freeze. I investigated through
**six flashes in a row**, each with a new fix, without ever fixing the point of
return. When the app stopped enumerating the USB, there was nowhere to go back to
without rebuilding — and recovery cost BOOT+RST, reinstalling the bootloader and
several physical RESETs by the operator.

The repository had the floor the whole time. It took me six flashes to use it.

> **Before the first exploratory flash, say out loud which commit is the known good
> and how you get back to it.** It is not ceremony: it is the difference between "I
> revert in one command" and "I rebuild blind with the board mute".

And one fix per flash, with the board responding between each. Stacking fixes in one
flash only makes every later diagnosis ambiguous.

## The bootloader window is not caught by command, it is caught by waiting

Flashing **after** asking for the RESET fails: the command takes longer than the 2 s
window. The right way is the inverse — **wait for the bootloader to appear and flash
the instant it shows up**:

```
poll `dfu-util -l` every 0.25 s; flash on the first hit
```

It worked on the first try after two attempts lost to the old method.

## Recovery when the app enumerates nothing

Symptom: no serial port, no DFU, LED lit. The app came up and brought down the USB.

1. **BOOT+RST** enters the **ST ROM** DFU (`0483:df11`, shows the internal flash at
   `0x08000000`) — independent of the app and the bootloader.
2. `make program-boot` reinstalls the Daisy bootloader. The error on the `leave
   request` at the end is normal: the device already left.
3. A simple RESET, and flash the app by the waiting described above.

## Recovery when the board ENUMERATES but does not speak

A different symptom from the previous one, and more deceiving: the serial port
exists, opens without error, and returns **zero bytes** — to everything. No
spontaneous output, no response to command.

The temptation is to conclude "it froze". **Do not conclude from muteness.** The
operating system still sees the device, and it is an independent witness to the
firmware:

```bash
ls -l /dev/cu.usbmodem*                       # the node's timestamp
ioreg -p IOUSB -w0 -l | grep -E '"idProduct"|"USB Product Name"'
```

- **The node's timestamp changes on each re-enumeration.** If it changes when you
  send a command, the **RX works and the command executed** — only the TX is dead.
- **`idProduct` says which mode it is in.** `0x5740` = the app's CDC; `0xdf11` =
  DFU. A board that "will not enter DFU" may be restarting and coming back to the
  app, and the two cases are indistinguishable by `/dev`.

Real case (08/28): the board went mute after a `dfu`, and stayed mute through 8 s of
passive listening and three commands. The node changed from 11:08 to 11:12 on each
`dfu go` — it was restarting the whole time. **Flashing resolved the muteness**,
because the log's circuit breaker lives in the app.

> **A mute channel proves that that channel is mute, and nothing beyond that.** A
> witness that does not depend on the accused is worth more than ten attempts to make
> it talk.

And do not attribute the muteness to your last command just because they coincided:
on this bench a perfect coincidence already produced an *"it was the cable"* that was
the dongle.

## A flash that fails midway leaves the flash PARTIALLY ERASED

`dfu-util` erases before writing. If it fails on the erase — seen on 08/28 at
**52%**, with `Error during special command "ERASE_PAGE" get_status` and `make: ***
Error 74` — **there is no app on the board at all**, and the state is worse than
before starting.

What to do, in order:

1. **Check whether it is still in DFU** (`dfu-util -l | grep 0483:df11`). After a
   failed download it usually stays, because the `:leave` never got to run.
2. **Repeat immediately**, without asking the operator for any gesture. The second
   attempt ran clean through to `File downloaded successfully`.
3. Only if it has left DFU do you go back to BOOT+RST.

**Usual suspect: the USB chain.** The board was on `1-1.4`, behind two hubs. Before
blaming the flash, take the hubs out of the path.

> **`Error 74` and `File downloaded successfully` are the only two reads that
> matter.** Rule §3 applied to the case: your script's echo says it ran, not that the
> binary went up.

## A destructive command asks for confirmation — and its echo may not arrive

This project's `dfu` requires `dfu go`; `dfu` alone only prints the warning. That is
correct — there is no software undo.

But note the combined failure mode: if the log is mute, **you send `dfu`, do not
receive the warning, and conclude that the command does not exist or did not take.**
The confirmation exists for the human, and the human is precisely the one left
blind when the output channel dies.

> **When you send a command and receive no echo, the next question is not "did I send
> it right?" — it is "does this channel still speak?".** Prove the channel with a
> known echo command before rewriting the command.
