# Redoing the ground — bench sheet

Written on 02/09 for the next day's assembly. The diagnosis is in
battle_scars **15bm** and in the P3 sweep.

## BEFORE disassembling — note this (rule R3)

Multimeter on **AC / mV**, **with the part playing** (a measurement with no load reads
zero on top of a defect — rule R1):

| measure between | before | after |
|---|---|---|
| Daisy `AGND` ↔ permaproto rail | | |
| Daisy `AGND` ↔ negative of the amp input | | |
| Daisy `AGND` ↔ `GND` of the 15 V | | |

And note **how it sounds** — the hiss, with a word you can repeat tomorrow.

## The drawing

```
AGND ISLAND  (reference — does not carry current)
├── gnd of the piezo jacks
├── Daisy AGND
└── negative of the amp INPUT
     …and nothing else.

STAR  (current — at the supply negative)
├── GND of the amp's 15 V      ← its own wire, short and thick
├── Daisy power
└── ESP32 power
```

**The bridge between the two worlds already exists and is unique:** the Daisy joins
`AGND` and `DGND` internally, at one point. Do not duplicate it.

## The question that decides each wire

> **Does this conductor carry current, or define a zero?**

Current → star, its own wire. Zero → island, and it does not leave there.

## Acceptance criterion, written BEFORE (rule R7)

**The AC voltage between `AGND` and the rail RISES** — they become two different
zeros, and that is expected, not a defect. **And the hiss drops
audibly.**

If the hiss does **not** drop: undo and say so. The hypothesis falls, and the next
one is P1 (star vs. daisy-chain) — there may be a consumer reaching the star
*through* another.

## One variable at a time (rule R2)

Do **only** the `AGND` separation. Do not take the chance to touch cables,
capacitors or routes at the same time — if you change two things and it
improves, you do not know which acted, and one of them may be making things worse.

## Routing cautions

- **Signal and return of the piezo travel together** — side by side or twisted. The
  area between them is a loop, and a loop picks up field.
- **Shielded cable: shield at one end only**, the Daisy end. At both ends it
  becomes a ring and conducts.
- **Input cables do not run parallel to the amp output** — class D
  is a square wave at hundreds of kHz with high current. Cross at right
  angles (P6).

## ⚠️ Do not confuse this with the 31/08 mistake

There I said to remove the **SIGNAL** ground between the ESP and the Daisy, and that
killed the board. **Here no ground is removed** — a **reference** is separated
from a **current rail**. They are opposite things, and the P3b pass of the
sweep exists because of that mistake.

## To verify later

- **P1** (star vs. daisy-chain) and **P2** (islands are leaves) — require seeing
  the assembly done.
- **Does the amp have `100 µF + 100 nF` on the pins?** A class-D amp with no local bank
  is a candidate for the residual that remained after unplugging the USB from the Mac.
