# Design — preset library in RAM

> **Status: steps 1–3 IMPLEMENTED and proven on the board (23/08, v0.148.0);
> steps 4–5 pending.** The cycle `capturar → dump → reset → paste → load`
> closes with 33 params and zero refusals. Written while the hardware acceptance
> runs, on purpose: it is head work, it does not touch the firmware and it captures
> no state, so it does not run the risk of freezing an unvalidated
> addressing. See `ACEITE-MULTIPLEXACAO.md`.
>
> Complements `PRESETS.md` (approved 22/08) with the structure decisions.

## 0. Three count corrections, before any decision

**a) There are 37 families per chain, not 33.** The estimate was 23 originals + 10
character copies. The real figure is **27 + 10 = 37** — counted from the manifest, not from
memory.

**b) The DTCM is not tight: it is empty.** The premise was that `.bss` in the DTCM
would compete with what is already there. The linker report says otherwise:

    DTCMRAM:        0 B / 128 KB      0.00%
    SRAM (AXI):   281 KB / 512 KB    53.73%

The `.bss` falls in the **SRAM**, not in the DTCM — the script has a separate
`.dtcmram_bss` section, and **nothing uses it**. The 281 KB of SRAM are almost all
`reverb_mem` (225 KB).

**c) `sync` does not open the authorship window.** Found while investigating point 3, and
it holds for today's firmware, not only for the design — see §3.

## 1. What goes into a chain preset

The criterion, which is dB's and resolves the hard cases by itself:

> **A preset stores musical intent — not hardware state, nor a measurement
> tool.**

Applied to the 37 families, it excludes **four**, not two:

| out | why |
|---|---|
| `exc_on` | **physical state.** It will become a read of the DPDTs on D29/D30 — a preset that wrote it would try to change a metal switch |
| `bench_boost` | **measurement tool.** It is bench |
| `gov_ceiling` | **hardware calibration.** The manifest already says why: *"each amp and each exciter has its own gain and coupling, so the useful ceiling differs"* — it is a property of the physical branch, not of the music |
| `gov_release` | likewise |

The last two are the consistent extension of the same argument that excludes the
`bench_boost`. A "string-shimmer" preset loaded into a sculpture with a different
exciter would carry along a protection ceiling **calibrated for another body** — and
protection is the subsystem where being wrong is most expensive.

**Chain preset = 33 params.** One byte each (normalized 0–127, the same
resolution MIDI delivers and that `set` refines when someone needs it).

## 2. What goes into the global preset

Of the 12 shared ones, the same criterion excludes **three**:

| out | why |
|---|---|
| `tone_freq`, `tone_mix`, `tone_dest` | **the test tone is bench.** A preset that turned it on would put a sine wave in the middle of the piece |

<!-- doc-check: ignore route_exciter (nome APOSENTADO em 28/08, hoje `out_l`. A mencao e deliberada: e o registo da renomeacao, e apagar o nome velho apagaria justamente o que explica por que ele estava errado.) -->
<!-- doc-check: ignore route_jack (idem, hoje `out_r`) -->
**Global preset = 9 params:** `level`, `piezo_mix`, `dly_time`,
`out_l`, `out_r`, `shift_hz`, `buzios_on`, `buzios_thresh`,
`buzios_cooldown`.

⚠️ **`out_*` stays, with the tension noted.** It is output topology, which sounds
like hardware — but *which branch is live* is a performance decision, and the
approved design includes it. (They were called `route_exciter`/`route_jack` until
28/08; the three-position DPDT made those names false.)

🚫 **The matrix `route_*` does NOT go in, and the reason is another.** It compensates for how the
transducers are WIRED — a fact of wiring. Inside the preset, each preset
would carry the wiring, and undoing a wire would force re-recording the whole
library: the same family as the *"string would exist twice"* that made the
library be ONE.

And it **falls under the suffix rule** if no one excludes it: `route_l_a` ends in
`_a`, so without the exception in `presets.h` it would enter the **chain**
presets on its own — the worst of the two possible destinations.

**Global slot 0 = boot state**, which is what kills `level` being born at 0.

## 3. `load`, the authorship window, and a defect that already exists

### Today's defect: `sync` does not open the window

`MidiEspelha()` writes `quiet_until[id]` on every message. **`MidiPushEstado()`
— the `sync` — writes none.** It sends the 86 and leaves.

Since a toggle widget returns *its own state* (which is exactly why the
window exists), after a `sync` the board **obeys the echo**. The `eco_ult`
keeps the mirror from resending; it does not keep the input from accepting.

**It is not a design hypothesis — it is the current firmware.** Fixing it is one line, but
not with the bench in progress.

### What `load` adds

Loading writes 33 params at once. If each one announces, that is **33 windows
opening together** and 33 messages on the wire.

**Cost of the dump, measured by what already exists:** `sync` spends
`hal::DelayMs(3)` per message × 86 = **258 ms of blocked loop**. And the
comment next to it still says *"dump 12 CCs"* — it was written when the
map had 12, and the block grew 7× in silence. It is finding 5 of the audit
recurring through growth, not through editing.

### The decision

**`load` does NOT use the `sync` path.** It marks the 33 params dirty and lets the
normal mirror drain them at its own cadence — two messages every 8 ms, without
blocking the loop. 33 params take ~132 ms of draining **without stopping anything**,
against 99 ms of a locked loop if it were a dump.

    load  ->  writes the params
          ->  opens quiet_until[p] for the 33, ALL AT ONCE
          ->  marks dirty; the mirror drains 2 per 8 ms
          ->  input drain of 300 ms (the mechanism already exists)

**Why the window opens on `load` and not as the mirror sends:** the
autonomous gesture is the `load`, not each message. If the window opened only when the
message went out, the widget that echoed on millisecond 5 — before its
message had gone out — would be obeyed, and the `load` would be undone halfway.

**The drain exists and serves here with no change:** it is the same problem as the 13n with
a different trigger — the instrument just changed on its own, and what arrives in the
next 300 ms is a reaction, not intent.

## 4. Where the library lives

**`.bss` → SRAM (AXI), explicitly, and not in the DTCM.** The reason is not
capacity — it fits in either:

    16 chain presets  × 33 B  =    528 B
     8 global presets  ×  9 B  =     72 B
    headers + indexes          ≈    128 B
    ------------------------------------------
    total                      ≈    728 B

It is **by usage**: the library is cold — read on `load`, written on `capturar`,
never touched in the audio callback. The DTCM is the core's RAM with no wait state and no
cache, and spending it on cold storage is wasting the only memory that
could still save a hot path.

> **Noted for another occasion, not for now:** 128 KB of DTCM entirely
> idle while 225 KB of `reverb_mem` live in the SRAM is an asymmetry that
> deserves a measurement of its own — with A/B in the same binary, because on this board the delta
> between builds can be 25% (scar 12d). It is not in the presets' scope.

## 5. The header, and the lesson of the crossing

> **Proving that the new configuration is safe does not prove that the crossing is.**

A preset captured today and loaded after a remap writes values
to the wrong params: it is **exactly the misaddressed ghost of the 13n, with a
delay of weeks** — and worse, because there is not even a wire to suspect.

```
struct PresetHeader
{
    uint16_t magic;        // 'CT'
    uint8_t  formato;      // versao do RECIPIENTE (não do mapa)
    uint8_t  n_params;     // contagem, redundante de propósito
    uint16_t versao_mapa;  // HASH dos nomes, ver abaixo
    uint8_t  checksum;     // cobre CABEÇALHO E payload
    uint8_t  _pad;
};
```

### The `formato` is the crossing in another dimension

The verdicts cover a swapped file, a remap and corruption. **None
covered "the header gained a field between the writing and the reading."** A preset
written with the v1 layout and read by a v2 firmware is interpreted with
**an offset**: it passes the magic, fails the checksum, and reports *corruption*
for a format-migration case — sending you to hunt for the wrong thing.

**A convention, and it lives next to the field because in another place no one would
remember it:** every change to the layout of `PresetHeader`, or to the order/width of
the payload, increments `kFormato`.

### The ORDER of the checks decides the diagnosis

The checksum covers the **header**, and it has to — a flipped bit in the hash
field produces *"written under another map"*, which is a **plausible** sentence, and
the operator will go looking for a remap that never happened.

Hence the rule: **no field is trusted before the checksum passes.**

    1. magic     -> swapped file          (cheapest and most specific)
    2. format    -> container migration   (the layout changed under your feet)
    3. size      -> inconsistent          (without this there is nothing to sum)
    4. checksum  -> corruption            (before trusting any field)
    5. map       -> remap                 (now the field is trustworthy)
    6. count     -> which side to look on

The first three come before the checksum for the same reason: they are
**preconditions for the sum to exist**.

The bank proves the **pair**, not just the half: a corrupted hash field reads as
corruption, **and** a real remap with an intact header still reads
as a remap. Without the twin, the new order could mask everything as
corruption and the first test would pass just the same.

### The map version is DERIVED, not incremented

**A number someone has to remember to bump is a number that does not get bumped.** The version
is a hash (16-bit FNV-1a) of the concatenation of the **param names in
order**, computed in `Init`:

- renaming a param changes the hash;
- reordering changes the hash;
- adding or removing changes the hash;
- **changing only a param's range does NOT change it** — and that is correct, because the
  preset stores the normalized value, which survives a range change.

The last point is the reason the hash is over names and not over the whole
table: a hash that was too broad would refuse good presets and teach the operator to
ignore the refusal.

### `n_params` is redundant on purpose

The hash would already cover it. The count exists because **a hash that does not match does not say
what changed**, and the count says which side to look on. It costs one byte.

### The refusal is shouted, and here is what it says

```
PRESET RECUSADO: gravado sob o mapa 0x4F2A, este firmware e 0x91C7.
  33 params gravados, 33 esperados — a CONTAGEM bate, entao mudou
  NOME ou ORDEM, nao a quantidade.
  Nada foi carregado. `preset dump` do arquivo velho ainda funciona.
```

**Refusing without saying what to do is refusing halfway.** The last line
matters: the text is still legible, so nothing was lost — the way is to edit
the names in the file and reload.

## 6. The text format, and why it is the whole curation

```
# presets.txt — biblioteca de cadeia · mapa 0x91C7
1  corda-shimmer   in_gain=64 gate_thresh=12 drv_on=0 ... shm_on=1 shm_shift=88
2  tambor-seco     in_gain=71 gate_thresh=40 drv_on=1 ... rev_on=1 rev_size=30
```

**By NAME, not by position.** A positional format would be shorter and
would turn any reordering of the manifest into silent corruption — the
same defect as the header, moved inside the line.

Reorder = move a line · rename = edit text · delete = delete a line ·
**version = git · backup = free · take to the next instrument = copy.**

Same philosophy as the manifest: *the instrument's identity is data, not code.*

## 7. Order of implementation, and what each step can prove on its own

**DONE (v0.148.0):** 1, 2 and 3. **Remaining:** pointers with pre-listen (4) and OLED (5).

1. **Structures + map hash.** Proof: the hash changes on renaming a param, and
   does not change on touching a range. Native bench, no board.
2. **`capturar` + `preset list`.** Proof: capturing twice with a `set` in
   between gives different presets.
3. **`preset dump` / `preset load` over serial.** Proof: `dump` → edit → `load`
   → `list` reproduces what was edited. And a `load` with a tampered map is refused
   shouting — **exercise the refusal, otherwise it is a hypothesis**.
4. **A/B pointers + `load` with window and drain.** Proof: loading is not
   undone by the widgets' echo (the test is `vistos` and `cc_janela` after
   a `load` with the iPad connected).
5. **OLED**, when the panel is soldered.

**Before step 4, fix the `sync`** — it has the same defect that
`load` must not have, and it is cheaper to prove the fix on `sync`, which already
exists, than on `load`, which does not yet.
