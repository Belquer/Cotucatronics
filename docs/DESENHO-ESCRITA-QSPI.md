# Design — our own QSPI write routine

**State:** design, nothing implemented. Written after the inventory, before
the first line — which is the order this session learned to respect.

**Why our own routine and not libDaisy's:** see bts 12v. `PersistentStorage`
has an unwritten precondition — *"the app does not run from QSPI"* — revoked by
`APP_TYPE=BOOT_QSPI` with no compile warning.

**Why not fork `qspi.cpp`:** the transitive closure decides, and it decides against
the fork.

    EraseSector calls
      ├─ CheckProgramMemory  → EnterQuadMode, GetProgramMemoryRegion, SetMode
      ├─ SetMode             → Init, and recursively SetMode
      ├─ WriteEnable         → HAL_QSPI_Command, HAL_QSPI_AutoPolling
      ├─ HAL_QSPI_Command    → ST driver
      ├─ AutopollingMemReady → HAL_QSPI_AutoPolling
      └─ ERR_RECOVERY        → SetMode(MEMORY_MAPPED)

    qspi.cpp (libDaisy)        1,308 lines
    stm32h7xx_hal_qspi.c (ST)  2,666 lines
    ITCM available                64 KB

The chain **crosses the libDaisy boundary and enters the ST HAL**. Moving
`EraseSector` without `HAL_QSPI_Command` is not an incomplete solution: it is a
guaranteed failure at the worst moment — the instruction fetch dies exactly when the
mode changes. And taking everything means loading ~4,000 lines of third-party code into
64 KB, with a boundary that moves at every update.

**The criterion is dependency surface, not lines written.** Our own routine
is the one that writes the most and inherits the least: **zero** transitive closure.

---

## The ISR inventory, and it decides the shape of the window

Measured in the `.map` and the `.elf` of `9f47efb`, BOOT_QSPI:

| what | where |
|---|---|
| `.isr_vector` — **the vector table** | **`0x90040000` — QSPI** |
| 51 handlers, including `SysTick_Handler`, `DMA1_Stream0/1`, `OTG_FS`, `QUADSPI` | `0x9005xxxx` — QSPI |
| `HardFault_Handler` | `0x90053ef0` — QSPI |

**This resolves the question by necessity, not by choice.** During the window
the QSPI is not memory-mapped. Any interrupt is fatal **before reaching the
handler**: the processor fetches the vector at `0x90040000+offset`, which is
exactly what went off the air. There is no possible exception list — the
entire dispatch mechanism is in the erased memory.

> **`__disable_irq()` for the whole window is mandatory.** The simplest answer
> won over the finer one because the finer one does not exist.

**Note on `HardFault_Handler`:** it also lives in QSPI. If something is missing
during the window, there is nowhere to go — the processor enters lockup. That
is not an argument against the design; it is the reason the routine has to be
**entirely** ITCM, without a single call to the outside.

## Stopping the audio: the two sides, and libDaisy kills both

`DaisySeed::StopAudio()` → `AudioHandle::Stop()` → `SaiHandle::StopDmaTransfer()`
→ `HAL_SAI_DMAStop()`, which does **`SAI_Disable`** and **`HAL_DMA_Abort`** on the
Tx and Rx streams.

So the API covers the two distinct acts: it stops the peripheral **and** aborts the
DMA. Without that, the DMA would keep transferring on its own during the window — it is not
fatal (DMA does not fetch instructions) but it would leave the last half-turn of the buffer
looping, and pending interrupts would fire the instant
`__enable_irq()` returned.

---

## The sequence

```
save <n>                            // bench gesture, over serial
  1. hal::StopAudio()               // SAI stopped + DMA aborted
  2. __disable_irq()                // the vector table is in QSPI
  3. ITCM routine:                  // ZERO transitive closure
       - QSPI -> indirect mode
       - read/erase/write
       - QSPI -> memory-mapped
  4. __enable_irq()
  5. hal::StartAudio()
  6. sync                           // the world checks: re-push of the mirror
```

**Everything between 2 and 4 runs from ITCM, without a single call to the outside.** No
library function, no `Log`, no variable in QSPI. It is <100 ms, it is
bench, and it is deaf by construction.

## THE REAL PROTOCOL — read in libDaisy's `Init`, 22/08

**This is the most valuable result of the round, and it is knowledge, not code.**
It is here because the next attempt may lock up the board, and the finding has to
survive the session.

**libDaisy NEVER enters quad:**

```cpp
// These functions are defined, but we haven't added the ability to
// switch to quad mode. So they're currently unused.
QSPIHandle::Result EnterQuadMode() __attribute__((unused));
```

The chip is in **plain SPI**. This **refutes** the hypothesis that explained the
`FF FF FF` (chip in QPI ignoring a 1-line command) — a plausible hypothesis,
one that matched the symptom, and wrong (bts 13j).

**The format of the memory-mapped read, which is what the chip understands today:**

| field | value |
|---|---|
| `InstructionMode` | **1 LINE** |
| `Instruction` | `0xEB` — quad I/O fast read |
| `AddressMode` | **4 lines** |
| `DataMode` | **4 lines** |
| `DummyCycles` | **6** |

It is neither SPI nor QPI: it is **hybrid**. Only the instruction goes serial; the address
and data go on four lines. Any new command is born knowing this.

**The chip libDaisy assumes** appears in a commented-out comment beside the
dummies: `IS25LP080D_DUMMY_CYCLES_READ_QUAD` — an ISSI IS25LP. **The library's
assumption, not a reading** — hence the JEDEC ID still holding.

**And `0x88` stopped being a mystery:** it is two `1000` nibbles — one line
high and three low. It is the signature of a four-line command arriving at a
chip that only listens to IO0.

**Next attempt, with the protocol known instead of assumed:** `0x9F`
with instruction and data on **1 line**, **zero** dummy, and the `CR`'s `SSHIFT`
checked — with a high prescaler, the half-phase sampling is what separates
`03 07 0F 1F` from a legible ID. One variable.

## The chip is born anonymous

The first thing the routine reads is the **JEDEC ID (0x9F)** — three bytes,
manufacturer and density.

This is not a prerequisite of the route: **it is its first increment.** libDaisy
does not expose ID reading and the peripheral handle is private, so reading the ID requires
exactly the same register access as writing, minus the erase. It is the "hello
world" of the infrastructure: it validates ITCM, the mode change and the
return to memory-mapped **with the only command that cannot break anything**.

The ID goes into the boot-report, and an unknown chip is **refused loudly** instead
of being written with the wrong command table — family 3 closed by design,
now at the silicon level.

## Where the presets live

To be decided with the map in hand. What is already known:

- The app's `QSPIFLASH`: `0x90040000`, 7936 KB. The binary uses ~189 KB.
- The first 256 KB of the chip (`0x90000000`–`0x9003FFFF`) are **outside** the
  app's region in BOOT_QSPI — bootloader area. **Do not write there:**
  it risks bricking even the BOOT+RST.
- Erase is by **4 KB sector**. The 8 slots live in a single sector, so every
  `save` is **read the 8, change 1, rewrite the block**.
- +4 bytes of a write counter in the sector header: the flash endures
  ~100k cycles and save-by-command never exhausts it, but the counter turns
  "did it wear out?" from a mystery into a reading — ten years from now, in a gallery.

**The guarantee is from the linker**, not from convention — the same play as the SDRAM
gate: a clandestine one does not even link. The forked `.lds` with a certificate of origin
already exists on the `investigacao-boot-sram` branch and is reusable.

## Acceptance

1. **JEDEC ID read and the board survives** — validates the whole mechanism without
   writing anything.
2. **ID in the boot-report**, and an unknown chip refused with a log.
3. `save 1` / `load 1` surviving **unplugging from the wall**, not just a reset.
4. `list` identical before and after a `save` — the write does not touch state.
5. A preset from an old map **refused loudly** at boot, not loaded crooked.
6. Write counter going up, and visible.

## What I still do not know

- **Which chip.** By design: the ID is the first read.
- **Whether the 4 KB erase fits comfortably in the window** — it depends on the chip and
  becomes known after the ID.
- **Whether `__disable_irq()` for ~100 ms has a side effect** on the USB CDC (the
  enumeration may drop and come back). Bench, acceptable, but to be observed.

**And the record that closes the BOOT_SRAM route:** dozens of `SetMode` calls
survive on every boot — almost certainly the I-cache holding the short routine.
But *a short routine survives* does not scale to *a 4 KB erase
survives*: the erase is long, the polling is a loop, and a long loop is where the I-cache
eventually loses a line and fetches from the dead QSPI. **ITCM is the difference
between works-by-luck and works-by-construction.**

---

# Where the front stopped (22/08) — and how to resume

**Stopped by scoping, not by giving up.** The architectural risk is over;
what remains is a register sequence, which is reading work.

## What was PROVEN

1. **The ITCM window works.** The board leaves memory-mapped executing from an
   independent RAM, talks to the peripheral, returns and survives. Reproduced
   across six flashes. `ITCMRAM: 212 B` is the proof that the section was applied.
2. **The real protocol is read from the source**, not assumed — see the `Init`
   section above. libDaisy **never** enters quad.
3. **The probe sweeps without writing:** `qspi id [dummy] [modo] [opcode]`.
4. **`00 00 00` is distinguished from `FF FF FF`** in the report, with the
   interpretation labeled beside it. Diagnostic information that did not
   exist before — see scar 13k.

## PROTOCOL FINDING: leaving memory-mapped requires ABORT

Turning off `QUADSPI_CR_EN` **is not enough**. The peripheral keeps state from
the mapped configuration — timeout counter, `TCEN`, transaction in progress — and
with that state standing the data line stays stuck. The symptom was reading
`00 00 00` even on `0x06` (WREN), which **has no data phase**.

    QUADSPI->CR |= QUADSPI_CR_ABORT;
    while(QUADSPI->SR & QUADSPI_SR_BUSY) { }   // with a cap on loops
    __DSB();
    LimpaFlags();
    QUADSPI->CR |= QUADSPI_CR_EN;

libDaisy does this inside `SetMode`. **Our own routine inherits nothing
— it has to do it explicitly**, and this is the first item on that bill.

*(It came from a guess, and is recorded as a finding all the same. See 13l on
why a guess that is partly right is the most dangerous.)*

## THE THREE LIVE HYPOTHESES — named, not tested

The reference is `QSPIHandle::Impl::GetStatusRegister()` at
`libDaisy/src/per/qspi.cpp:1045`, which does an indirect 1-byte read on this chip,
with this peripheral, and **works**. It differs from our own routine in three points,
and any one of them explains the current result:

1. **Two steps vs one.** The HAL calls `HAL_QSPI_Command` and **then**
   `HAL_QSPI_Receive`; our own routine configures `CCR` with `FMODE=01` in a single
   write. The order of writing `CCR` may be what triggers the read.
2. **`NbData` / `DLR`.** The reference requests **1 byte** (`NbData = 1` → `DLR = 0`);
   the probe requests 3 (`DLR = 2`). A wrong length can leave the transaction
   hanging.
3. **Order of writing `CCR`** relative to `DLR` and `AR`. In QUADSPI the
   write of `CCR` is the trigger when there is no address — if `DLR` is written
   afterward, it arrived late.

**They are not independent enough for blind bisection.** Testing one costs
a flash and, if it fails, does not eliminate the other two: without the manual one cannot
distinguish "the hypothesis is wrong" from "the execution of the hypothesis is wrong".

## RESUMPTION CONDITION

> **RM0433 (STM32H7 reference manual), QUADSPI chapter, indirect-read
> section.** With it open, the sequence is **reading**, and the three hypotheses
> turn into one answer.

**The criterion, written so that whoever resumes does not confuse a pause with abandonment:**
*half an hour with the manual is worth more than ten flashes guessing.* Without the
manual, the front stays stopped — and staying stopped here is the right decision, because
the hard part is already solved.

## After the ID reads

The order has not changed: ID → loud refusal of an unknown chip → erase → program
→ `save`/`load` surviving unplugging from the wall.
