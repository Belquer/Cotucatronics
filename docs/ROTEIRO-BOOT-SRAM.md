# Revalidation roadmap — switch to BOOT_SRAM

**Why it exists:** without a roadmap written **before** flashing, revalidation
turns into "I turned it on and it worked". This page is the contract of step 1.

**Why the switch:** `PersistentStorage::Save()` takes the QSPI out of
memory-mapped mode to erase, and that code lives in the QSPI. Its precondition is
*"the app doesn't execute from QSPI"*, revoked by `APP_TYPE=BOOT_QSPI` with no
compile warning at all (bts 12v). Presets depend on this.

---

## Before flashing — the memory map

Measured in `48832a3`, BOOT_QSPI:

| section | where | size |
|---|---|---|
| `.isr_vector` + `.text` | QSPI 0x90040000 | **182 KB** |
| `.data` | SRAM 0x24000000 | 4,7 KB |
| `.bss` | SRAM 0x24001280 | **276 KB** |
| `.sdram_bss` | SDRAM | 64 MB (pool) |

The 276 KB of `.bss` are dominated by **`reverb_mem` = 220 KB**; then
`warble_mem` 15 KB and the libDaisy audio buffers, 8 KB each.

**The BOOT_SRAM arithmetic:** the region shrinks from 512 to **480 KB** and
`.text` comes to live in it.

    276 (.bss) + 5 (.data) + 182 (.text) = 463 of 480 KB

**~17 KB are left over, not 300.** The earlier estimate ignored that the SRAM
already carried `.bss`. It fits, but the margin is a declared wall — not space.

**Two checks done before flashing:**

**The stack is in the DTCM** — `_estack = 0x20020000`, the top of the 128 KB of
DTCM, in both linkers. The 17 KB are a real 17 KB, not optimistic margin.

**The heap LEAVES the AXI in BOOT_SRAM.** The BOOT_SRAM linker sends the heap to
`RAM_D2` (line 248: `} > RAM_D2`); in BOOT_QSPI it stays in the AXI, at
0x24044900. One less consumer in the tight region, for free.

**If it doesn't fit — and the right destination is NOT the SDRAM.** The H750 has
**RAM_D2: 256 KB at 0x30008000**, declared in the linker and today unused by the
app (all of `.bss` lives in the AXI). It's real SRAM, on the AHB bus — slower than
the AXI for the M7, but **orders of magnitude closer to SRAM than to SDRAM**.
The 220 KB of `reverb_mem` fit in it with margin.

The question "can the reverb withstand slow memory?" almost disappears with D2 as
the destination. **It remains a separate flight, after the base is declared** —
but the track stops being expensive.

---

## After flashing — the list, in this order

Nothing here is "I turned it on and it worked". Each item has a criterion.

### 1. The binary linked and fits
`arm-none-eabi-size` and the `.map`. **Diff the two `.map`** (before/after),
not just read the new one: a symbol that changed address appears in the diff and
vanishes in an isolated read.
**Criterion:** no symbol left the region it should be in; total ≤ 480 KB.
Confirm in the diff that the **heap** migrated to RAM_D2 and that the **stack**
stays in the DTCM — if the stack went to the AXI, the real margin is smaller than
the arithmetic.

### 2. The four regimes, average AND peak
Redo with `cpu reset` between readings. **IT'S A NEW BASE** — code in SRAM has
another I-cache profile, and this platform already proved the weight of that in
the `-O3` case. Expected to improve; expected is not measured.
**Criterion:** declare "reference v0.124+, BOOT_SRAM". **No delta against
the BOOT_QSPI numbers is attributable** (12d).

**A nuance that reinforces "expected is not measured":** with `.text` in the AXI,
**the reverb's code and data now contend for the same bus** — instruction
fetches and the buffers' traffic now live in the same SRAM. The I-cache
mitigates and the expectation is still to improve over the QSPI, but the
improvement may not be uniform. If some regime surprises, the candidate is R2/R3 —
the heavy-reverb ones.

### 3. MIDI mirror
TX of 76 at boot (`midi` → `eco: TX 76`). Authorship window by the test that
validated it: 4 cycles of 500 ms on a CC with a widget → **8/8**; a CC with no widget →
**8/8**. A collision within the window → `cc_janela` increments.

### 4. `vistos`
`vistos reset`, sweep a family, check that the CCs appear.

### 5. Overflow criterion
`ctos prazo 300` → drops in the declared order, the report says **ARTIFICIAL
DEADLINE** and **DROPPED by OVERFLOW**. `ctos prazo 0` restores.

### 6. Serial and black box under load
With the heavy regime in the air: `list`, `ctos`, `midi`, `bb`. **Criterion:** the loop
doesn't starve — the serial responds and the LED keeps beating.

### 7. MIDI back to Loopy
Move a widget and see the parameter change; change a parameter over serial and
see the widget follow.

### 8. Audio
Play. No click, no dropout, no change of character without an
explanation. **This is the only item Code can't judge on its own.**

---

## At the end — the ceiling calls for RE-EXAMINATION, not automatic change

The 850 µs were chosen as a budget over the execution profile of the
**QSPI**. If the SRAM really changes the peaks, 850 may become too loose or too
tight relative to the new distance to the wall.

**The question:** *does the administrative budget still administer the distance
to the deadline well?*

The answer goes labeled as **chosen over measurement**, in the form that the
step 0 standardized — and the old value is recorded with the reason for the
change, if there is one.

## The boot report gains the wall

480 KB is a new limit, and nobody should discover it by hitting it. The boot now
declares **"binary X of 480 KB"**.
