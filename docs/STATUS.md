# STATUS — Cotucatronics
<!-- doc-check: ignore svg_colisoes (tool in tools/, out of reach of check_docs) -->
<!-- doc-check: ignore dentro_do_contorno (tool in kicad/, out of reach of check_docs) -->
<!-- doc-check: ignore sincroniza_pcb (tool in kicad/, out of reach of check_docs) -->
<!-- doc-check: ignore para_fabrica (tool in kicad/, out of reach of check_docs) -->
<!-- doc-check: ignore funcao_dos_pads (tool in kicad/, out of reach of check_docs) -->
<!-- doc-check: ignore confere_arcade (function of pcb_contorno.py, in kicad/) -->
<!-- doc-check: ignore y_max (value of the FRENTE_CEREBRO constant, in pcb_contorno.py) -->
<!-- doc-check: ignore y_min (value of the FRENTE_CEREBRO constant, in pcb_contorno.py) -->
<!-- doc-check: ignore refresca_redes (function of sincroniza_pcb, in kicad/) -->
<!-- doc-check: ignore silk_over_copper (KiCad DRC violation code) -->
<!-- doc-check: ignore silk_overlap (KiCad DRC violation code) -->
<!-- doc-check: ignore starved_thermal (KiCad DRC violation code) -->
<!-- doc-check: ignore lib_footprint_mismatch (KiCad DRC violation code) -->
<!-- doc-check: ignore courtyards_overlap (KiCad DRC violation code) -->

**Where we are.** The companion of this document is
[battle_scars.md](battle_scars.md), which says *what we learned getting
here*. Read both before touching code.

> Updated on **2026-09-06 (the enclosure is ready for Ponoko)** · brings:
> **`caixa/cotuca_caixa_r2_acrilico_3p0_55cf3955.svg`, 20 parts, ready to
> order.** The name carries the revision and the **digest of the drawing** — r1 (19 parts,
> no reinforcement) was order #1263497 and was named the same as r2 before this.
> Transparent acrylic 3.0 mm, kerf 0.2, sheet P2, occupies 377.1 × 326.0 with
> a 6.9 mm margin, ≈ 6.84 m of cut (≈ $40 at $5.87/m, plus shipping).
>
> The twentieth part is the **lid REINFORCEMENT** — a 193 × 50 strip glued
> underneath, only on the arcade strip: 6 mm where the foot presses, 3 mm where the shafts
> pass through. Concentric ø30 holes, verified. **The wooden prototype
> had 6 mm and the buttons jammed there** — it was that measurement, on an object that
> existed, that settled the question.
>
> Arcade at maximum spacing: **x = 30 · 100 · 170**, 70 between centers, 40 between
> edges. If a wide foot still jams two, the answer is a glued ridge,
> not more space — there is no more space on a 200 face.
>
> ⚠️ **THE BOARD IS NOT READY, and it is independent of the enclosure.** The three arcade buttons
> go down 40 mm and pass over the brain board, with **7.5 mm** of free height in the
> worst case. **Six parts do not fit** — `RV1` (10), `C13` (11) and the headers
> `J4 J5 J10 J15` (20 with the plug and the wire). It is placement, not a redesign.
> Orientation decided by measurement: `FRENTE_CEREBRO = "y_min"`, because the other
> put the Daisy (17) and `C1` (25) under buttons. `pcb_contorno.py` **v0.7.0**
> has the gate that measures this — no DRC sees it. Scar **15ci**.
>
> Queue: move the 6 parts · mounting holes (symbol → schematic → PCB) · route
> 139 and 47 · `revisao-terra` · gerbers.

> Updated on **2026-09-05 (KiCad for JLCPCB)** · brings: **the panel board was born**
> (`kicad/cotuca_painel/`, ERC 0) and the contract between the two boards became
> verifiable — `tools/contrato_idc.py` reads the 26 links of the two projects by
> geometry and checks one against the other; KiCad's own netlist confirms
> 26/26. Two of them (13 and 14, the LED ground) **had never been drawn**:
> they came out as `1.7763568394002505e-15` and no reader with `[\d.]+` saw them.
>
> **The OPA1642 was wrong in three ways at the same time** — pins 1 and 3
> swapped relative to the real SOIC-8, the two amplifiers in the same package,
> and three BOM lines for one chip. It is now a three-unit symbol, with the
> pinout taken from the `LM2904` of KiCad's official library. None of this was visible
> on the prototype, which is hand-wired.
>
> **29 references renamed** (`R0A`→`R10`…): KiCad treats a reference
> without a numeric part as unannotated and renumbers it on its own when saving. The tens digit
> came to say the channel. Proven by netlist comparison under the map.
>
> **Board: Daisy Seed3**, checked pin by pin against the rev4 — the STM32 name
> matches on all 40 pins. TI TAC5242 codec, 32 bit / 192 kHz. Symbol, footprint and
> netlist identical.
>
> **Footprints: 77 of 79**, all names verified against the installed
> libraries. The two pots are missing. **`docs/COTUCA_BOM.md`** is the BOM of the
> whole instrument, **generated from the sheets** — ≈ US$ 22 in board components,
> ≈ US$ 97 with the modules.
>
> ⚠️ **What blocks the factory:** the `OPA1642` in JLC's parts library (LCSC
> stock is not the same warehouse), the pots' footprint, and the value of `R5`/`R6`
> of the panel. **There is no `.kicad_pcb` at all** — the layout is the next step.
>
> New tools: `colisoes_sch.py` (the schematics gate, which
> `svg_colisoes` could not be), `contrato_idc.py`, `bom_completa.py`.
> Scars 15cb–15ce, and the `kicad` skill from 270 to ~600 lines.

> Updated on 2026-09-04 (cleanup) · brings: **the retired effect from the
> Fx2 slot left the entire documentation** — 43 mentions across eight files (`STATUS`,
> `battle_scars`, `FEATURES`, `IFA10000`, `CTOS`, `params-aposentados` and
> three skills). Where it was the REASON for a live decision, the reason stayed and
> only the name left: the block 48, the 🔴 serial defect (which is starvation of
> the USB ISR by a long callback, and not of an effect), the Fx2 high-pass
> and the four steps of the IFÁ10000 it taught.
>
> In the same batch, **the external DSP engine left the entire documentation** —
> another 53 mentions, including 15 in the README. Where the name was an API
> identifier (the macros of the embedding recipe) the recipe left instead of being
> renamed: a macro with an invented name is worse than no macro. The three
> `doc-check: ignore` directives of `FEATURES.md` left with the names they
> exempted, and the path back to dB's `.maxpat` files was **rejected
> before being rewritten** (`git show 8271e97:Max/` lists the four; the delay's
> returns 104 335 bytes). `check_docs` at 0 ghosts in both rounds.
>
> From the process the **`/audit`** skill was born + `auditar.sh` v1.0.0 and
> `versoes_readme.py` v1.0.0. In its debut against this tree it caught **5
> dead links** in `MAPA-DE-TERRA.md` (it pointed to `../` while being one level
> deeper), one **duplicate entry** in the retired list, and **15 of
> 58 versions in the README map drifted** — one of them with the README AHEAD
> of the source (`ponte.ino` v0.5.0 announced, v0.4.0 in the file).
> All fixed; the map is at 60/60.
>
> Second pass of `/audit` (`auditar.sh` v1.1.0 + `coerencia.py` v1.0.0):
> **two of my own checks were flagging by the wrong criterion**
> — they counted plain occurrences when `check_docs` only sees names **between
> backticks**, and assumed line scope when `isentos()` scans the whole
> file. Rewritten in Python. The **6 orphan exemptions** went to zero without
> losing history: `oct_on` and `test_tone_on` **migrated** to the retired
> list (52 → 54) because they carry a unique record; the other four were
> copies of a record that lives in `battle_scars`. The **vault** was aligned
> across the three files that were behind — the `HARDWARE.md` there still had the old
> matrix names (`route_l_a`), renamed on 02/09.
>
> New pass of **pending items**: 186 lines flagged in the tree. The
> **⚪4 of `AUDIT-23-08`** was closed (old versions in the README), with proof; and the **⚪5**
> was recorded as **open and larger**: the `"76 CCs"` is today in three
> comments of `main.cpp`, not two, and the real number is 119.
>
> **04/09, close: the cleanup is done, end to end.** With dB's decision:
> `src/TESTES.md` (v0.25.0) and `src/BOARDS.md` (v0.14.0) cleaned without waiting
> for the other session, and **the 86 source comments** in 13 files. The build
> proves they were only comments: **QSPI 283 196 B, SRAM 322 488 B, ITCM 308 B
> — identical to the baseline**, before and after.
>
> Three **false recipes** left — they told you to use files that do not exist:
> the "bench" of `BOARDS.md`, the revival block of the `Makefile` (v0.4.1), and
> the commented-out binds of `manifest.h` (v1.12.2), where the claim lived that
> the module's params **were still in the table**. They were not, and it was that claim
> that sent me looking for code that does not exist. It is there now saying so.
>
> The **T12** of `TESTES.md` (98 lines, the spectral-coincidence test)
> became an 8-line tombstone: the two lessons it bought live in step 6 of
> IFÁ10000 and in ch. 6 of battle_scars, which is where they serve any effect.
> The hand-made CC map of `BOARDS.md`, which said "current instrument" and listed
> four dead params, was **removed instead of corrected** — pointing to
> `CCMAP.md`, which is generated, instead of keeping alive the table that lies.
>
> The README's tail was fixed: the open defects now mirror
> this document, the signal path stops being "waiting for hardware", Pot 5
> stops doing a crossfade retired on 20/08, and the "next steps"
> point here instead of duplicating a list that drifts again.
>
> **⚪5 of `AUDIT-23-08` closed:** the `"76 CCs"` was in three comments of
> `main.cpp` (v0.301.1), not two, and the real number is 119. Corrected in all three.
> **🟡2 was closed and nobody had noticed.** I went to write it and
> found it done: `Colisoes()`/`Colisao()` in `manifest.h`, called at
> `main.cpp:5134`, and the `board` printing `colisao de CC`. Implemented after
> the 23/08 audit, with the entry left to close. Building on top would have been
> work duplicating code that already existed.
>
> **05/09 — new board flashed, and the three invisible params stopped being so.**
> The new Daisy came in (just one unit, for now), with the bootloader already there — the
> DFU that appeared exposed `0x90000000`, the QSPI. Flashed and checked on both
> sides: source and board at **v0.302.0**.
>
> The `board` flagged `sem CC: 3 de 122 params — alcançáveis por `set`,
> invisíveis ao iPad`: `sim_laco`, `sim_hz`, `sim_fb`, the suppressor's test
> bench. They got **CC 107/108/109** on `ch1`, stuck next to `bench_boost`
> (105/106) because they are the same family — bench things, not stage things, and the
> proximity is a safeguard: a misconfigured widget hits its neighbor. Chosen
> free in **both** sources (source and generated `CCMAP`) and outside 120–127
> (Channel Mode), 0/32 (Bank Select) and 32–63 (the LSBs of 0–31).
>
> Measured on the board after flashing: **`sem CC` 3 → 0 · CCs 119 → 122 ·
> `colisao de CC: nenhuma`** — reported by the detector that was already there, which is
> exactly its job. ⚠️ **There is no widget in Loopy for these**, by
> dB's decision: the CC exists so the param stops being invisible, not because
> there is a page waiting.
>
> **And `check_docs` flagged 10 files for me as "touched without bumping the version"** —
> those of commit `1cc896c`. Looked at, not assumed, which is what its docstring
> asks: they are **pure comment**, and the proof is the byte-for-byte identical binary
> (QSPI 283 196 B · SRAM 322 488 B · ITCM 308 B, before and after). **Not
> bumped on purpose:** `hal`, `params` and `dynamics` report a version in the
> `board`, and bumping it would announce a change that does not exist — besides forcing
> a reflash for nothing. It is written down so it is not discussed again.
>
> **A contradiction of mine, caught in the sweep of pending items.** When choosing
> the CCs I wrote that 120–127 are Channel Mode and are not used. But the project
> uses **120/121/122 on purpose** as COMMAND CCs (`sync`, preset load,
> preset save) — they are not params, so they do not enter the map. The comment
> read as a general prohibition and now says what it is: the high range is not
> free, it is reserved for something else.
>
> **`/audit` was tightened against the two failures of this session** (`auditar.sh`
> v1.2.0). The first: I swept the source with `*.h *.cpp Makefile` — 33 files —
> and the tree has **137 tracked across 14 extensions**; `check_repo.sh` escaped
> and only appeared by accident in the final sweep. The inventory is now done by
> `git ls-files`, and prints the tree's extensions alongside, so that a new type
> appears in plain view. The second: the **sentence broken by the renaming** — a unique
> anchor, clean `grep`, passing `make`, and the sentence left as *"dies under a connected
> callback"* because the continuation was on the next line. `vizinhos.py` v1.0.0 was
> born, which reads the diff and flags only where the edit touched the END
> of the line. Run over this session: **16 neighborhoods, all read, no
> other broken.** And the exhaustive sweep confirms **zero in 137 files**.
>
> ⚠️ **The README's tail is rotten on its own** and the cleanup only
> uncovered it: it said "No known defect" with a 🔴 priority 1 open; it
> said flashing "requires the BOOT button" fifteen lines after announcing
> `flash.sh` entering via `dfu go`; it gave the signal path as "not yet
> implemented, hardware on the way"; and it pointed three times to
> `src/modules/README.md`, which does not exist. **This was not fixed** —
> it is outside the request, and is dB's decision.
>
> Before, 2026-09-04 (deep clean) · brings: the suppressor is now
> called **SuMiA™** in every instance · **deep clean of the documentation**
> — the README declared two incompatible states fifteen lines apart from each other
> ("block 48, no external engine" and an FFT module on with block 256),
> announced
> v0.172.0 with
> the source at v0.301.0, and said the pots and buttons were not soldered · the
> `Max/` folder (1.8 MB of retired patches) left the tree with the path
> back proven · `CCMAP.md` had the hand-made header aging on top
> of a generated table · and ten live documents entered the README map
>
> ⚠️ **NEW HARDWARE: the pre-amp is assembled and tested** (04/09), with a
> fuller sound. That invalidates all the previous gain measurement — §14 of
> `medicao-limpa`. The listening session was just beginning when the deep clean
> interrupted it; **the pre-flight has to be redone from scratch.**
>
> Before, 2026-09-04 · brings: **flashing now restores the state** —
> `flash.sh` v0.4.0 + `estado.py`, saving before the DFU and restoring after,
> because the rule existed in the skill and lived only in my head · the **splash
> mark redone in three parts** (hollow shell, lip, mouth) against
> dB's drawings and renders, with the **top↔bottom mirror at 0 px of 1215** and the
> shell proven closed by **flooding** · and the **pin exam** catching
> `PINOS.md` contradicting itself
>
> **05/09 — the ground test in the owner document was stale.** The
> `ATERRAMENTO.md` (v0.2.0) is, per the README, the owner of the subject, but its
> *Conferência* section still told you to measure the island against the rail and said *"if
> it beeps, there is one connection too many"*. **With the revB net-tie, beeping is
> correct** — the old test would read a correct board as defective. It now
> mirrors `MAPA-DE-TERRA.md` §6: three measurements and in order, because the
> middle test alone does not separate *"single bridge"* from *"no bridge"*, and one
> of them kills the audio. The premise (20↔40 open on the bare Daisy) is **measured on
> 04/09**; what remains to be measured is the assembled board.
>
> ⚠️ **Still to be confirmed with the multimeter**: the `AGND`/`DGND` joined outside
> the board (P3 of the ground sweep) — dB redid the grounding on 03/09 and
> reported cleaner sound, but the measurement was not done.
>
> Before, 2026-09-02 (close) · brings: **compressor** (49 dB of crest
> factor, measured) · **output meter** `pkout` · and the **broadband
> hiss** diagnosed down to a named rule — `AGND`/`DGND` joined outside
> the board, P3 of the ground sweep, **to be confirmed with the multimeter**
>
> Before, 2026-09-02 (night) · brings: **SuMiA™** — automated
> microphonics suppressor, per exciter, four mouths, proven on a **test
> bench** without the sculpture · the notch changed axis (chain → **branch**) ·
> **supervised startup ramp**, which backs off when it catches a mode ·
> and three microphonics incidents that hurt, all mine
>
> Before, 2026-09-02 (end of day) · brings: **the splash mark
> shrunk and raised** at dB's request — clearance from 3 to 6 lines up to the
> name · the **PS_2 measured** and the prescaler map closed · the **spectrum at
> 7 kHz** measured (the framebuffer does not touch it, but the broadband floor
> rises 5.4/6.9 dB) · **20 black-box orphans at 0** · and all the
> checkers at zero
>
> Before, 2026-09-02 · brings: **the lockup with the bridge, closed at the
> root** — a `GPIO_PULLUP` was missing on the MIDI RX · **loop watchdog** that
> restarts the board on its own · **black box fixed**: 20 orphans and an
> address collision · and the rule that organizes the rest, **15az**
>
> Before, 2026-09-01 · brings: **the OLED splash** alive (cowrie-shell
> throw → mark → name → version, 50 frames at 20 Hz) · **the whole framebuffer
> over SPI at PS_8**, 3 811 µs/frame, verified on the glass · a 5×7 font generated
> from legible glyphs · the ornament that charged **13% to the idle loop**
>
> Before, 2026-08-28: **2×2 routing matrix** measured ·
> **OLED alive** and three encoders counting · **MIDI livelock fixed** ·
> `in_gain` removed · **tarolator**, a new effect born from a measurement

---


## 19/09/2026 — brain board #1 on the bench: what was assembled differently from the BOM

The assembly of the first brain board is underway, by the manual
(`docs/montagem/manual_montagem.html`). What this unit has different
from the BOM stays here, so no one later reads it as an assembly error:

- **`C14`/`C24`: electrolytic, not film.** The BOM calls for a WIMA MKS2 2.2 µF 63 V
  with 5 mm pitch; it was not bought (dB was counting on the lab's stock) and the
  only 2.2 µF in the lab was a 400 V film, too big for the 5 mm pitch
  and its 7.8 × 11.6 mm body. An **electrolytic** was fitted, with the **+ on the
  `R14`/`R24` side** (VBIAS, ~7 V; the Daisy's input stays always below), and
  the polarity was checked by dB. ⚠️ **Value and voltage of the fitted
  electrolytic: to be noted.** What changes: nothing audible if ≥ 2.2 µF; the manual
  (v0.3.0) now accepts this swap, with the + marked on the drawing, and forbids
  multilayer ceramic (microphonic, and it loses capacitance with 7 V on top).
- **Arcade button order on the lid: blue A · blue B · white**, with the white one
  on the side of the tall capacitor — decided at the bench because the white one (cowrie shells) has
  no LED and is shorter. Measured by dB on 19/09: **the white one goes down 20 mm below
  the panel, with connectors**; the capacitor is **C1, 2200 µF**.
  The `confere_arcade` (`pcb_contorno.py` **v0.24.0**) now has **one
  depth per button** and **electrolytic height by diameter** (D16 = 25,
  D10 = 16 mm, from catalog). With that the free height is **7.5 mm** under a blue one and
  **21.5 mm** under the white one.
- ❌ **MY ERROR, corrected the same day.** I wrote that the Daisy under the central button
  proved the declared orientation. **It proves nothing:** `U2` falls under the
  middle hole in **both** orientations — I measured both and only then saw. A fact
  that is the same on both sides does not distinguish the two sides.
- 🔴 **The brain board is assembled ROTATED 180° relative to the model (dB, 19/09).** The proof
  is the silkscreen: with the user's side facing you (so, from the front) and
  looking from above, the **panel**'s silkscreen reads upright and the **brain board**'s reads
  **upside down**. That is the `y_max` orientation — the one that was **rejected**
  on 06/09 exactly for putting tall parts under the buttons. And in that orientation the
  checker flags, measured today:
  · **`C1` (2200 µF, 25 mm)** under hole **1** — matches the tall capacitor that
  forced the button order to change, which was what remained to explain;
  · **`U2` (17 mm)**, **`J13` (1×3)** and **`J16` (1×8)** under hole **2** — the two
  headers count **20 mm** with plug and wire.
- 🔴 **And nothing prevented it:** the brain board's four mounting holes (`H1`–`H4`, ø3.2)
  form a **symmetric rectangle** — measured on 19/09 — so the board screws
  in both directions, and the wrong direction has no mechanical resistance at all. The
  `FRENTE_CEREBRO` is a **declared constant**, and `encaixe.py` even **refuses**
  if it is not `y_min`: the tool believes the declaration and never asks the
  board. **For rev D this is a requirement:** the fastening has to be asymmetric (or
  gain a key cutout), so the wrong orientation **does not go in**.
- ⚠️ **To be decided, and it is dB who decides:** rotate the brain board to the model's
  orientation (and see whether the connectors still reach the walls), or accept the
  rotated orientation and **relocate the tall parts in rev D**.
  And dB says the Daisy **fits, tight** — which measures what was missing: the
  checker gives **7.5 mm** free under a blue one and the Daisy is **17 mm**, so the
  blue button's depth is **not** the worst case's 34 mm. For the Daisy
  to fit, the blue one goes down **~24 mm** below the panel. ⚠️ **To be measured:** the
  real depth of the blue button (the white one's, 20 mm, is already measured). While
  it is not measured, `confere_arcade` fails `U2`, `J14` and `RV1` — and it fails
  **out of excess caution**, which is the right side to err on.
- ⚠️ **STAYS OPEN: which is the tall part under the white button.** With the
  orientation confirmed, `C1` (2200 µF) is **64 mm** from the nearest hole
  — it cannot have been it. Under the left hole is the **pin header
  `J14`** (8.85 mm, plus the IDC cable's plug on top); under the right one there are only
  low-lying parts. **To be measured:** what is under the white button.
- ✅ **The amplifier got a home (dB, 20/09): a shelf over the brain board, on
  4 hot-GLUED M3 towers**, in the back half (points at 114 · 175 mm from the
  left and 5 · 95 mm from the top). `onde_furar.py` v0.3.0 said the four
  points **passed with an isolating screw** and failed with a metallic one (they fall on the
  `GND` plane, in a wide field, with 4.7 to 11.1 mm clearance to the nearest pad).
  **dB chose not to drill:** the board is irreplaceable mid-assembly, and the
  gain in tightness did not pay for the risk. Gluing is reversible; drilling is not.
  ⚠️ **The heat does not arrive by contact** (note corrected on 20/09, with dB's
  reasoning): the amplifier screws to the **top** of the tower and the glue is on the **base**.
  Two weak paths remain — conduction through the tower (the module's holes are its
  cold part; a nylon washer between module and tower cuts this off for good) and
  the **ambient of the closed enclosure**, which is what really measures. EVA glue softens
  at 60–80 °C. **Cheap test:** half an hour playing loud and a finger on the base of the
  tower — five seconds without letting go ≈ below 50 °C, comfortable.
  ⚠️ **And the bigger risk is mechanical, not thermal:** hot glue is strong in
  shear and weak in peel. A stretched cable peels the tower off before
  any temperature — slack in the wires, and a bead around the whole foot.
  `encaixe.py` M7 already accepts this home; the **step-down** is missing.
- 🔴 **The enclosure had no defined place for any module (20/09).**
  `CAIXA-ACRILICO-MONTAGEM.md` §6 says *"the amp module on its [standoffs]"*,
  but those standoffs never existed: the bottom plate has **4** holes, and they are
  the brain board's. And there is no floor: the plate is **193 × 113** and the brain board occupies
  **180 × 100**, which leaves strips of **6.5 mm** to the right and behind — no
  module fits alongside. Measured on 20/09.
  The volume that remains is **above the brain board**: **31.4 mm** at the front and
  **56.4 mm** behind (interior minus standoff 6 + spacer 10 + board 1.6). Behind
  is where it fits, and it is also where the cable rules already send the amp (the
  exciters exit to the right, the table line comes from the DPDT on the back wall).
  ⚠️ **To be measured before deciding:** the module's dimensions (with heatsink and
  terminals), its hole pattern, and the height of the already-fitted `C1` — a second
  floor above the brain board has to clear the 25 mm of `C1`.
  ⚠️ **And the heat:** the enclosure is closed acrylic. A class D at 15 V inside a
  sealed enclosure was not discussed anywhere in the repo.
- ⚠️ **The panel only comes out with the TRS jacks removed (dB, 19/09).** With the TRS
  mounted the panel has no way out: the encoder's tip stays stuck in the
  acrylic of the lid. Assembly and maintenance order: **the panel goes in and comes out
  before the jacks**. See `caixa/LICOES-MONTAGEM.md` #11.
- ⚠️ **(historical) DIVERGENCE — the orientation of the brain board in the enclosure.** The model declares
  `FRENTE_CEREBRO = "y_min"`, and in **that** orientation `C1` is **64 mm** from the
  nearest button: nothing tall under the holes. `C1` only falls under an arcade
  in the **opposite** orientation (`y_max`) — which was rejected on 06/09 exactly
  for putting `C1` and the Daisy under buttons. Either the assembled board is turned
  the opposite way from the model, or the hole dimensions are not those of the drawing. **I did
  not record it as a fact until measuring**, because it changes what rev D has to resolve.
  In the `y_max` orientation the checker also flags **U2 (Daisy, 17 mm)** and the
  headers **J13/J16 (20 mm with plug)** under blue buttons, where only
  7.5 mm fit. **To be measured:** which part is under each of the three holes, and the
  real height of `C1` (the catalog says 25 mm, and only 21.5 fit under the white one).
  See `caixa/LICOES-MONTAGEM.md` #10.
- **Panel, state of the first connection (22/09):** **5 V and 3.3 V** power
  good · the **three buttons** respond (the white one of the cowrie shells is **NC**, an exception
  declared in the firmware, `hal` v0.76.0) · the **two pots** read by the ADC ·
  **arcade LEDs working** after swapping the purchased harness, which came
  with the colors crossed (bts 15fh).
  **OLED cable, colors lifted from the part (dB, 22/09)** — Waveshare does not
  publish them, its wiki only lists pin names: `J2` pin 1 = **red** (+5 V)
  · 2 **black** (GND) · 3 **blue** (DIN) · 4 **yellow** (CLK) · 5 **orange**
  (CS) · 6 **green** (DC) · 7 **white** (RST). The cable order coincides with
  `J2`'s, so it is a direct connection — counting pin 1 by the **square pad**, which
  with the header soldered underneath ends up on the opposite side from what the silkscreen suggests.
  ⚠️ **To be resolved: the OLED does not turn on** and the **encoder counts 1 pulse in 20
  clicks**, with its three pins switching — suspicion on the common terminal.
- ⚠️ **Debt measured on 22/09: 15 of the 47 maps in the manual have text collision.**
  Measured with `tools/svg_colisoes.py` map by map — **15 before and 15 after** the
  change of the label that deflects (so it is not a regression, it is inheritance). They are mostly
  labels crossing the board edge (`texto×borda contentor`) and two cases of
  `texto×texto` (`VCC`/`GND` in the OLED map, and `1 vermelho`/`2`). Until today only
  the new drawings passed through the gate; the old maps never passed.
- ✅ **22/09: the brain board plays.** Piezo → pre-amp → Daisy → delay and reverb →
  line output, on **both** channels, with no clip anywhere. What was
  between the assembly and this paragraph is in `battle_scars.md` **15fd**, **15fe**
  and **15ff**: a bridge with wrong pins, three cold solder joints in a chain, and the input
  divider that had never been measured with the real piezo.
- **Measured state of this unit (22/09), so it is not reinvented later:**
  · piezos: **18 nF** (metal string, enters at `p1`/`J5`) and **17 nF**
  (structure next to the drum skin, enters at `p2`/`J6`)
  · `C12` and `C22`: **22 nF from the factory + 100 nF C0G soldered on top** →
  attenuation of **−17.8 dB** per channel
  · peaks playing hard: **−1.4 dBFS** at `p1` and **−15.6 dBFS** at `p2`, `clip 0`
  · `pre_level_ab` = **0.5** (was 1.0; the internal sum overflowed at +5.5 dBFS)
  · `k1`/`k2` = 0.60 · `post_level_l/r` = 0.88 · output at **−12.7 dBFS** peak
  ⚠️ **The 14 dB asymmetry between the channels is from the INSTRUMENT**, not a defect:
  metal string and drum skin are different sources. Whoever "corrects" that
  undoes the adjustment.
  ⚠️ **To be decided:** `p1` is 1.4 dB from the ceiling. The firmware recommends another
  6 dB — it would be swapping the `C12`'s 100 nF for 220 nF. Not done yet.
  ⚠️ **All of this lives in RAM:** any reflash of the Daisy resets the params to
  zero, and only the capacitors stay.
- **DPDT, the wiring that has already worked (dB, 21/09, described from the assembled part):
  3 wires and ONE jumper, switch used on the DIAGONAL.** `A1` = line (mixer) ·
  `B1` **empty** · `A2`–`B2` jumpered = the common, which receives the signal ·
  `A3` **empty** · `B3` = amp (exciter). In the three positions it gives **line only**,
  **both**, **amp only**. On the board: `J15` pin `1` → common, `2` → `A1`,
  `3` → `B3`; `4`/`5`/`6` are not used, `7`/`8` are drain.
  ⚠️ **To verify with dB:** he noted the common as *GND*. On this board the
  common is `OUT_L_DAISY` (the signal) — if it were GND, the line and the amp would never
  receive signal, because `LINHA_L` and `AMP_IN_L` only meet the Daisy's output
  **through the switch**.
- **(historical) the earlier reading: 3 wires and 3 jumpers**, with
  **no ground on the switch**. Measured on the board: the three signals are
  **duplicated** on the header — `1`=`4` (`OUT_L_DAISY`), `2`=`5` (`LINHA_L`),
  `3`=`6` (`AMP_IN_L`) — because the two poles work in parallel, and that is what
  gives the middle position with **both outputs**. So **6 wires without a jumper** and
  **3 wires with 3 jumpers** are the same connection: in the first the parallel is made
  in the board's copper, in the second it is made on the switch.
  The manual's drawing showed only the 6-wire one and so seemed to diverge from
  what was assembled; it now shows both, with the jumpers drawn on the
  three lines.
- **Amplifier: power via `J2`, audio via `J11` (dB, 20/09).** The module
  in hand is powered via a **screw terminal**, so `J2` carries the power
  (+15 V/GND, short thick wire — it is a power return, star rule) and
  `J11` carries the audio (2 = reference, 3 = L, 4 = R). ⚠️ **`J11` pin 1 is NOT
  connected:** it is the same +15 V, and connecting both puts the power in parallel by two
  paths. Declared in `encaixe.py` **v0.5.0** (`COMPLEMENTARES` +
  `VESTIGIO_PINO`), with the gate requiring the declaration pin by pin — declaring only
  the pair would be silencing the duplication instead of resolving it. Rev D shrinks `J11`
  to 1×3.
- **Step-down in hand: `GND IN` and `GND OUT` are the SAME node** (beeped out by dB,
  20/09). So **a single wire** goes to `J3` pin 2, on the `OUT−` side — which is what
  `ATERRAMENTO.md` asks for (a dedicated ground wire; the 5 V return by another
  path already cost a click at ~3 Hz).
- **`U1` (OPA1642AID from Mouser) marks pin 1 with a band on the end**, joining
  pins 1 and 8, not with a dot. Confirmed by dB with the diode test on the loose
  chip (red on pin 4 = V−, 0.4–0.8 V to the others). The manual (v0.2.1)
  describes the two marks and the test.

## 17/09/2026 — a single branch, and the `J2` is that of the board that went to the factory

The branch `claude/slack-session-sm2ldz` (rev C, packages and orders from 12/09) was
**merged into `main`**, and the two became equal. The board, the positions and the `J2`
come from rev C; from `main` the guards stayed (1.27 mesh, courtyard against the
edge, `sincroniza_pcb` in the chain, the `liga()` guard) — none contradicts the
ordered board.

**The OLED's `J2`:** a **male** 1×07 header at 2.54 on the board, where the **female**
jumpers of the cable that comes with the module enter (MX1.25 on its side). Order
`1 VCC · 2 GND · 3 DIN · 4 CLK · 5 CS · 6 DC · 7 RST`, which is the module's read
from VCC: **with loose jumpers the contract is the name of each wire**, and the seven
match. Sources: Waveshare «Control Interface» page and dB, 17/09. The PicoBlade
`RST…VCC` that `main` had from 08/09 to 17/09 never reached any fabricated
board (scar 15ew).

**Proven over the ordered boards** (commit `06db1c7`, JLC
`W2026091304545438`): the merged generator reproduces the panel's netlist (24 nets, 0
differences) · `funcao_dos_pads` 0 failures on both, with the `J2` contract
failing under sabotage and the old PicoBlade contract failing 6 of 7 · IDC 26/26
· panel ERC 0 · courtyards: brain board 0, panel with `J1` at **0.405** from the left
edge (rule 0.5; the body stays inside) — exemption with the number, and in the next
revision `J1 x ≥ 26.1`.

**JLC order `W2026091304545438`: both boards, brain board and panel**
(confirmed by dB, 17/09). **Shipped on 17/09**: 5 of each, from the `06db1c7`
packages — the same boards measured above. DHL Express, tracking `7099774990`.

⚠️ **In assembly, the panel's `Q1`/`Q2`:** the kit map says `AO3400A`; you solder the
**`SI2302CDS-T1-GE3`** that came from Mouser (marking **N2**). Same pinout, 1 G ·
2 S · 3 D, checked in the Vishay datasheet doc. 68645 (17/09). See the note in MANIFEST.

⚠️ **The panel has no way to be fastened (18/09).** No mounting holes; the
`EC11E15244G1` encoder has no thread and the pots went to the lid. **Failed in
M2 of the M track of the IFÁ10000** (mechanical board↔enclosure certification, new, in `/aipcb`).
Drillable points measured, in mm from the top-left corner, component side:
**(77.5 · 32.0) · (109.0 · 32.0) · (146.0 · 4.0) · (172.0 · 32.0)** — M3 with washer,
outside the copper of other nets, in intact acrylic of the lid. The first 60 mm
are cantilevered. The lid takes the matching holes (the enclosure yields to the board).

**Panel pots (18/09):** the board has the Bourns PTV09A rear-mount
layout (3 pins at 2.5 mm, legs at 8.8 mm between centers, 7.0 mm from the pin
line to the leg line — checked in the Bourns PTV09 datasheet). The pots at home are
16 mm and do not fit; the Mouser ones were never bought. For the pots to **fasten
the board** to the lid they have to have a **threaded bushing** (Bourns `-6` style, or a generic
9 mm with M7): the lid's ø7 hole opens up to pass the thread, and the three
dimensions above are measured on the part before soldering. **10 kΩ linear (B10K)** — the 100 kΩ
of the project are the brain board's trimpots.

**Panel↔brain-board cable (18/09):** both ends are the male Würth pin header
`61202621621` (4 bought). The cable is **IDC 2×13 female-female, keyed,
straight (pin 1 ↔ pin 1), ~300 mm**: route measured ~170 mm along the back
wall (panel's `J1` at 98 mm from the front, brain board's `J14` at 19) plus the 80 mm of
slack the enclosure asks for to remove the bottom. The BOM said ~150, estimated.

⚠️ **Brain board rev C fails M6 (`encaixe.py`, by the body): three parts inside
the arcade discs**, against the HOLE radius (15) and 7.5 mm clearance: `J14`
(8.85 mm) at **11.6** from the center of the left arcade, trimpot `RV1` (10 mm) at
**13.3**, Daisy `U2` (17 mm) at **14.8** from the middle one. The button body under the
lid is narrower and was never measured — **measuring the diameter of the threaded body and
the microswitch** decides: it may absolve `U2` and maybe `RV1`; `J14` is the
one that most likely hits. By priority 2, what yields is the enclosure (moving the
arcade on the lid), not the fabricated board.

**New gate (18/09): `kicad/encaixe.py`**, read-only, called by
`para_fabrica` — today it refuses both boards: the panel on M2, the brain board on M6.

**Panel, M6 measured by dB (18/09): the OLED header does not fit on top** — neither with
straight jumpers, nor in an L. **Resolved: header soldered underneath, jumpers in an L
back and down** (`caixa/LICOES-MONTAGEM.md` #7). Pin anchor, visible
from both sides: **the square pad is the VCC**, and from there `VCC · GND · DIN · CLK · CS · DC ·
RST`; confirm by continuity with `TP1` (+5V) and `TP3` (GND) before connecting.
Do not repeat on `J1` nor on the LEDs' JST.

**`J1` (IDC) also does not fit on top** (it hits the OLED and the encoder's tightening — dB,
18/09). **Turning the pin header downward gives 0/26 or 2/26 correct pins and, in
two combinations, +3V3A shorted to GND** — simulated. Solution: **ribbon soldered
directly to the panel's pads, underneath**, IDC plug only on the brain-board side;
procedure and continuity in `caixa/LICOES-MONTAGEM.md` #8. Turning at **both**
ends gives 26/26 in 8 of 16 combinations, but under the brain board there are only 9 mm.

**The real encoder has a thread (generic EC11, 6.5 mm body) and fixes the board↔acrylic
clearance at 6.5 mm.** Everything taller on the top face hits: `J3`–`J7`
and `C3` go underneath (the `J6`/`J7` LEDs with **soldered wires**, not JST); **the pots
cannot go on the board** — they stay on the lid, wired, and the 16 mm ones from the drawer serve
(no need to buy the RK097). Table and anchors in `LICOES-MONTAGEM.md` #9.

**The brain board's `J4` («the ESP32 bridge») is a vestige (18/09):** the bridge's socket from
when it was by wire; the bridge is in socket `U3` since 06/09. **It stays empty**;
in the next revision, it leaves. Declared in `encaixe.py` (M1) — scar 15ez.

---

## ⚠️ 08/09/2026 — «synchronized» never meant what I read

`sincroniza_pcb` wrote the net onto the pads **only when adding a new
footprint**. For the parts already on the board, nothing refreshed anything: the board kept the
nets from the day the part entered.

Six defects were fixed in the schematic, the command ran, said **«they are already
synchronized»**, and the board kept all twenty-two faults — `EXC_R` was still
there.

> **Its report did not lie.** It compares *footprints* — which exist,
> reference, value — and on that everything was correct. A checker answers the
> question it was asked, and the question was another.

⚠️ **This changes how you read the history:** **no schematic change ever
reached a board already routed by this route.** It did not show because, until
today, every change came with new parts — and then the path that writes nets
ran. Any «synchronized» before this date means *the parts
match*, and not *the board is equal to the schematic*.

Fixed in `sincroniza_pcb` **v0.5.0** (`refresca_redes`), which puts on each
pad the net of the netlist **and erases the one the netlist no longer has**.


## Operation routine: the two destinations, always together

**Every new version of `STATUS.md` or `battle_scars.md` goes to BOTH
destinations in the same action:**

| Destination | Path | Form |
|---|---|---|
| Repo | `docs/` | raw, no frontmatter |
| Vault | `~/Documents/Obsidian Vault/Cotucatronics/` | with YAML frontmatter + tags |

The two serve different readers and neither replaces the other. The **repo**
is what survives the clean clone — a cited document that does not exist in the
clone is the same class as the allocation header that was missing for weeks.
The **vault** is where you read, link and think between sessions, with tags and
backlinks that raw Markdown does not have.

Never leave a destination for later: the pair misaligns silently, and the
misalignment only appears when someone needs the right version.

---


## 02/09, close — the hiss, and the measurement that knocked down two of my plans

**The perennial white noise**, which dB described as *"a hiss like a radio
off the station"*. Four facts, all measured by him:

| test | result |
|---|---|
| move the pot (digital volume) | **does not change** → it enters after the converter |
| **amp input** volume at zero | **does not change** → it does not enter through the input |
| remove the cable, or turn off the Daisy | **stops** → it is the Daisy, through the cable |
| remove the USB from the Mac | **improves, does not resolve** |

And on the code side, proven by reading: with `out_l = out_r = 0` the only
code after the mute is the LED and the CPU accounting. **The converter receives
exact zeros** — the firmware is exonerated by construction.

**Diagnosis, by the ground sweep (P3):** `AGND` and `DGND` **combined
outside the board**, on the permaproto rail. The Daisy already joins them internally at a
point; joining them again makes a **ring**, the current divides, and the fraction
that chooses the inside path crosses the **codec reference**. It explains
the four facts at once, and it is white because it is the sum of everything that runs
on the rail.

**To be confirmed with the multimeter**, in AC/mV and under load. The plan of attack
and the acceptance criterion are written in the sweep report.

### The compressor, and the measurement that asked for it

Built the **`pkout`** — the other side of the `pk` account was missing. The first
reading gave **74.7 dB of margin** and I was going to build a gain stage on
top of it. It was the **ambient**. With dB hitting:

```
ENTRADA  IN_L  0,0 dBFS · 24 CLIPS   (piezo directo, sem pré-amp)
SAÍDA    OUT_L pico −0,4 dBFS · média −49,2 dBFS   →  MARGEM 0,4 dB
```

**49 dB of crest factor.** The amp needed so much gain because it was
being asked to make the **average** audible. So the answer was not gain
— it was **compression**. New `Compressor` block in `dynamics.h`, shared
params, **born off**.

**Hardware requirement, measured:** the piezo needs ~6 dB of attenuation
before the converter, or the transients continue to be destroyed.

---

## 02/09, night — SuMiA™, and the price of iterating on a human being

**What exists now.** The `notch` is called **`microfonia`** and changed axis:
it was per chain (piezo), it is per **branch** (exciter). The loop closes through the
exciter+body pair, and dB has a drum on the L and a gas canister on the R — a per-chain adjustment
forced the same frequency to serve both. **Four mouths per branch**,
CC 1–18 on ch1.

**SuMiA™** (his name; the pun is exact) runs in the loop, 250 ms
alternating branches. It detects by **dominance**, acts at first sight,
**deepens** from 18 to 24 dB when the mouth is not enough, and **evicts the lowest
one** when they are full — his rule: *"the highest ones because they are the
most painful"*. It never touches a mouth placed by hand. It does not persist, does not enter
a preset, and **`micro_auto` is out of the preset for safety**: a scene
cannot turn off the protection.

**The startup ramp is supervised.** It rises over 8 s, quadratic curve, and
**backs off 25%** whenever SuMiA™ catches a mode — the mouth gains time to
bite and what already sounded drops. A body that howls a lot takes longer to open;
a clean body opens in 8 s. **The piece negotiates with its own body.**

### The listening finding: the loop crossed TWO bodies

With the matrix's four cells open: **1536.4 Hz at +24.4 dB,
dominance 0.99**. With `A→L` and `B→R` only: **1543.1 Hz at −24.3 dB,
dominance 0.27**. **Forty-nine decibels.**

And even with the matrix separated, the two branches reported **3275.7 and 3275.8
Hz** — the drum is **welded** to the gas canister. The matrix separates the
electrical paths; **it does not separate the metal**. That is why any pot has to rearm
both hunters.

### The test bench, and why it should have come first

SuMiA™ was tuned **with the sculpture on next to a child**, and
three times the microphonics reached her ears — one of them made dB's
son scream. See **15bi**, **15bj**, **15bk**.

There is now a **test tone** (`sim_laco`, `sim_hz`, `sim_fb`) that
injects a spike of known amplitude before the notch, with the outputs muted.
It proved the five behaviors in **ten minutes**: detection error
< 0.3 Hz, filling in order, deepening 18→21→24, eviction of the lowest
one, and silence when resolved.

**Declared in the code:** the tone does **not** test whether the notch breaks a real
loop. Only the body answers that.

**Open, and it is next:** the **perennial white noise** that plays all the time.

---

## 02/09 — the day a pin without a pull-up cost five hours

**The cause was one line.** libDaisy initializes the UART pins with
`GPIO_NOPULL`; with the ESP32 bridge unpowered the RX (PB7) was left
floating, and a loose CMOS input picks up noise at MHz. The USART went into a
storm **by rate**, the CPU stayed in the interrupt and the loop starved
to death — in less than 4 s, and without returning when the line healed.

`g.Pull = GPIO_PULLUP`. **Verified: 7/7 probings with the line floating
and all counters at ZERO** — the fault is not survived, it does not happen.

### What survived as a safeguard, and why

| safeguard | why it stays |
|---|---|
| ceiling and backoff in the `DmaListen` rearm | the line **held low** still exercises it (gave `rearmes 2`), and without it it ran at the loop's pace — **10 029 rearms with a healthy bridge** |
| loop watchdog in the audio callback | field requirement: pieces in a series, nobody presses a button for them. It restarts if the loop does not complete a turn in 48 000 samples |
| USART1 at priority 1 | they were **all** at `(0,0)`, and equal priority does not allow preemption: the DMA abort never completed. The grouping became an **invariant that the board checks and shouts** |

Two left that pretended to defend: flag clearing in the callback (useless
against a storm by rate, and a context race with the ISR itself) and
error ceiling in `PollMidi` (phase 2, for a fault that kills in phase 1 —
it only ran while the problem did not exist).

### The black box was lying in two ways

Orphan sweep in both directions: **14 slots read and never written**
(zero by construction) and **6 written and never read**. One of them was the
central question of the session — slot 30 reads as *"loop STUCK?"* and
**nobody wrote it**.

And worse than an orphan: **address collision.** The "PICO POR NOTA" block read
`32..44`, but 33–39 are MIDI counters and 40–44 the per-minute log —
it printed `midi.Bytes()` disguised as an audio peak.

**20 orphans → 0.** The flight recorder was also stuck at `if(voando)`
(`buzios_on`, which is born at zero after each flashing). Verified:
`blocos 14 080 · CPU 29% · voltas 218 780 · laço não ficou preso`, and the
three **close among themselves**.

### The rule that organizes the others

**15az** — a deferred fix does not stay small: it becomes the **equipment of the
next emergency**. None of the nine defects found was caused
by the pull-up, and none grew. What changed is that they came to be used,
and all were instrument, tool or report.

Counterweight: fixing now is not fixing everything you see — it is fixing
**what the current task is using**. In a hunt, that is the instrument.

**The listening session gained a new argument** (dB, 02/09): the warning
"flashing zeroes the params" is repeated since August and managed by memory. The
preset design already has the answer — **global slot 0 = boot state**
— and on the day the library exists, flashing stops zeroing: it loads 0.
The listening is not just the oldest bottleneck, it is what **retires that
warning**. See [PRESETS-LISTA.md](operacao/PRESETS-LISTA.md).

~~**Open:** the ~5% base of the loop~~ — **CLOSED on 02/09, and it did not
exist.** Two inert builds showed that the first measurement after
a flashing gives ~15 000 turns/s and the following ones ~22 800. Discarding the
first, the builds differ **0.27%** against **1.3%** of dispersion of the
instrument itself. It was **warm-up** — and the rule was written in the
`medicao-limpa` skill, item 17. See battle_scars **15be**.

---

## 01/09 — the panel started drawing, and the cost was measured

**What exists now.** The OLED stopped being two lines of text and now
receives **the whole framebuffer** via `hal::OledDesenha(fb)` — a single entry,
with the HAL boundary intact. On top of that, three **pure** C++ pieces, with no
hardware at all: `desenho.h` (primitives), `fonte5x7.h` (43 glyphs, generated from
legible art by `fonte5x7.py`) and `splash.h` (the opening). All of this can be seen
whole on the computer before there is any glass, and the Bresenham is **integer**, so
what comes out on the panel is bit for bit what came out in the ground test.

**The opening.** A cowrie-shell throw that enters from the top and fans out (1.05 s),
**settles** 0.45 s, one of the shells that fell rises and grows until it becomes the
mark, the name enters letter by letter, the version signs. 50 frames at 20 Hz, 2.50 s.
It runs **after** `StartAudio` — the instrument already sounds while the throw falls —
and turns off 2 s after finishing, because a lit panel costs 6 dB of noise
floor and a signature lit forever charged that price for an ornament.

It repeats without flashing: `splash` and `splash curto`.

**The numbers, and the ones missing.**

| | measured |
|---|---|
| Frame at PS_64 | 23 401 µs |
| Frame at PS_16 | 6 945 µs |
| **Frame at PS_8** (default) | **3 811 µs** — costs 7% of the loop at 20 Hz |
| Frame at PS_4 | 2 023 µs |
| Fixed cost, deduced from the two fast points | ~300 µs + ~440 µs per unit |

PS_8 and PS_4 draw **clean**, confirmed on the glass with a 1 px pattern. PS_8
is the chosen one, not PS_4: both serve, but PS_8 has an octave of clearance
**proven** below, and measured margin is worth more than the 3% it costs. **PS_2
was not measured** and is declared as unknown.

**Three open debts, written as debts:**

1. ~~The startup throw's seed is weak~~ — **PAID on 01/09.** The throw
   is now seeded from the **ADC's own noise**, gathered over 150 ms
   before the animation starts. Four startups gave `AB25A781`, `771E58D9`,
   `6E671930`, `190173E9` — and **917 distinct readings** of the converter in each
   gathering. That number has physical meaning: 917 in 150 ms is **6 113 Hz**,
   the ADC's conversion rate. It is the best proof that the entropy is real and not
   fabricated — it matches a hardware clock no one chose.

   The quality **is stated, not assumed**: `board` prints the seed and the
   count, and shouts `<- ENTROPIA FRACA` if it falls below 8 distinct
   readings (which is what would happen if the ADC were dead). A consultable
   twin, because the startup line comes out 150 ms after `StartAudio`
   and `reboot` knocks down the USB — whoever reconnects after would never see it.
2. **7% of the loop base is missing.** `noinline` recovered 8 of the 13 lost
   points; the others have no explanation and are not rounding.
3. ~~The waking opening~~ — **RESOLVED by dB on 01/09**, and the decision
   gave the animation a better job than an ornament. See below.

### dB's decision: three states, and the throw becomes an instrument

> *"the opening runs when I turn on the device · the oled works normally as
> we had established for the presets, without the opening · when we activate the
> cowrie shells the animation comes back but without the logo"*

| when | what the glass shows |
|---|---|
| Startup | **full opening** — throw, mark, name, version (50 frames), and the mark stays **3 s** on the glass |
| Normal use | the preset panel, **immediate**, with no animation at all |
| Cowrie shells fire | **only the throw** — 30 frames, no shell growing, no mark |

The third is the one that changes the nature of the thing: **the throw on the glass is the throw that
the instrument just made.** It stops being decoration and becomes a display.

It follows the `buzios_on` that already governs the draw — **no new parameter**, because a
switch of its own would be a second place where the same decision lives, and the two
would diverge. A new fire in the middle of the animation **restarts**: they are two throws, and the
second does not wait for the first. At the end it gives the glass back to whoever had it, and the
12-frame `splash curto` **was removed** — it existed for the waking, which
has just been cancelled.

### The cowrie-shell throw: it works, and what it is NOT

Confirmed by dB on 01/09: **it works**. But it cost a round-trip to get there, for
two reasons that are written down.

**`buzios_on` is a switch, not a trigger.** It arms the detector; what throws the
play is the hit. Sending the CC via Loopy fires nothing — and the opposite
expectation is reasonable, so it is now in the [CCMAP](operacao/CCMAP.md) and not in
anyone's head. **Open gap:** there is no way to throw the cowrie shells by
command. If it is made, it has to go through the same path as the detector — draw
**and** animation — so there are not two places where the throw is born.

**The detector listens post-gate, not the piezo.** With the chain closed it goes deaf.
A new serial command **`buzios`** splits the question into three: `disparos`,
`passaram o buzios_on`, `foram a animação`, and shows the peak the envelope
reached next to the threshold.

### The spectrum at 7 kHz with the framebuffer at 20 Hz — MEASURED on 02/09

Requested on 01/09 and done now. Alternation respected, first reading
discarded, watchdog floor at −80 dB.

| condition | peak | occurrences |
|---|---|---|
| QUIET 1 | 7017.6 Hz · **−24.9 dB** | 88 |
| 20 Hz 1 | 7018.3 Hz · **−25.0 dB** | 86 |
| QUIET 2 | 7018.9 Hz · **−25.1 dB** | 86 |
| 20 Hz 2 | 7019.8 Hz · **−25.3 dB** | 81 |

**The framebuffer at 20 Hz does NOT touch the 7 kHz spike** — ±0.4 dB, with the
occurrences overlapping. Negative result, and it is written down.

**But two things appeared that were not in the question:**

**1. The frequency drifts, always in the same direction** — 7016.9 → 7017.6 →
7018.3 → 7018.9 → 7019.8 Hz, ~0.7 Hz per run. A digital artifact of the MCU
would be locked to the crystal and **would not drift**. This is analog, and
probably thermal — the natural suspect is the step-down from the 31/08 hunt.

**2. The broadband `pk` RISES when drawing**: −69.6 → −64.2 dBFS on the L and
−75.2 → −68.3 on the R. **+5.4 and +6.9 dB.** The framebuffer adds broadband
noise (SPI switching) and **does not feed the 7 kHz tone**. They are two
phenomena in the same signal, and they only separate because both were measured.

**Method note:** the first attempt passed `-95` as the floor and the
instrument used `-25` in silence — it expects **magnitude**. The four
peaks all gave `−24.6/−24.7` against a floor of `−25`: a value pressed against the
boundary is a sign you are measuring the floor, not the peak. It was not
published.

### PS_2 — MEASURED on 02/09, and the prescaler map closes

| prescaler | frame | cost at 20 Hz | glass |
|---|---|---|---|
| PS_64 | 23 401 µs | −45 % | clean |
| PS_16 | 6 945 µs | −12 % | — |
| **PS_8** ← chosen | **3 861 µs** | **−7 %** | **clean** |
| PS_4 | 2 050 µs | −4 % | clean |
| PS_2 | 1 419 µs | −4 % | **clean** (photo) |

**PS_2 draws clean** — confirmed by photograph of the 1 px pattern, not
by description: a fine, uniform checkerboard, with no blocks or seams. The
peripheral does not break down to the floor.

**The decision remains PS_8, and now with a measured reason instead of prudence.**
From PS_4 to PS_2 the clock doubles and the frame only drops 31 % (2 050 → 1 419
µs): at ~300 µs of fixed cost that is already 21 % of the frame, and the loop does not feel the
difference — **−4 % on both**. PS_2 buys nothing that PS_4 does not already give, and
spends the last octave of electrical margin.

**Reading note:** the photo showed a dark band in the lower third, and
it was treated as a defect candidate until discarded — it is **rolling
shutter against the OLED's scan**, a camera artifact. The glass is
homogeneous. Verifying cost two minutes; assuming would have cost a hunt.

---

## 31/08 — the day of the ground and the microphonics

### 01/09 — THE PRESETS GATE IS CLOSED

**The two decisions were made by dB and are in the code.** The map is now
**`0xFB5A`** — and the global dropped from 11 to **9 params**, which are the two
`post_level_*` leaving.

**1 · `post_level_l/r` out of the preset.** His reason: *«they follow the physical pots;
if the preset restores them, firmware and copper disagree without anyone knowing which
rules. And each body in the series has its own exciter gain.»* Same category as
`exc_on` and `gov_ceiling` — **a fact of the unit, not musical intent**.

**2 · Tone family renamed:** `test_tone_freq · mix · out · via`. One exclusion
rule covers the four (there were two, and the third one being born would escape both).


⚠️ **The name went through three, and the last is the one that stays: `test_tone_level`.**
First `test_tone_mix`, because the code does a real crossfade
(`b*(1-m) + tom*m`, and at 0.5 you hear half of each). Then `gain`, then
**`level`** — dB's choice.

And the criterion that decided was not literalness: **the widget↔param matching in
Loopy is by TITLE**, so what rules the name is what is painted on the glass,
not what is exact in the code. Forcing him to repaint the panel to follow a
code name is the wrong trade.

⚠️ **Practical consequence:** the widget has to become **«Test Tone Level»**.
While it says anything else, the profile generator leaves it orphaned and the fader sends
nothing — **a silent failure, with no error at all**.

It is recorded what it DOES, so no one writes `>= 0.5f` against it: it is a
crossfade, not a switch.

⚠️ **And `ntc_*` became `notch_*`.** dB asked *«what is ntc?»* — and if the
owner of the instrument does not recognize the name, the name failed. Worse in this repository:
**NTC is a component** (negative-coefficient thermistor), and there are KiCad sheets
next to the firmware. The CCs (82–86) did not change.

### THE WORK ORDER, given by dB on 01/09

| | |
|---|---|
| **1** | ~~The rename~~ ✅ **done** — it was the hard gate |
| **2** | **THE LISTENING SESSION** — the oldest bottleneck, depends on no part, and it is what builds the preset library |
| **3** | In parallel, the ERC of the rev B sheets |
| **4** | The board **waits for the parts** — step-down, decoy and button footprints depend on measuring what arrives. Drawing before is guessing |

🚫 **What NOT to do now, his decision:** the `DmaListen` rearm. It has an artificial
trigger since the Schottky, but **it does not block the listening**, and it touches the path
that all the telemetry crosses.

### 01/09 — the copper closed, and the presets stopped being inert

**The bridge's hardware is done and measured by dB:** Schottky **1N5819** with
`VIN` at **4.72 V**, trigger test passed (powerbank off + USB on the
bridge = amp's LED off), and **100 µF + 100 nF** on the pins, after the diode.
**The dedicated ground to the star was not done, and is not needed** — 0 mV measured.

**The noise hunt closed with a verdict:** a **single** rail. Separating the sources
**worsens** it, and monotonically — +9.6 → +17.3 → +24.3 dB in 8–16 kHz. Mechanism: the
audio cable becomes the only ground link between amp and Daisy. The answer is
**gain chaining**, which the architecture already does.

**The two decisions that were blocking the presets were made:** `post_level_l/r`
out of the preset, and `test_tone_*` → `test_tone_*`.

**And CC 121 (load preset) came to exist** — it did not exist, and the Loopy buttons
were inert. The channel says the destination, the value says the slot. **122 (save) does not
exist by decision**: a CC that writes a preset destroys a library with one
lost message.

📄 **`LOOPY-SECAO.md`** is the assembly sheet, generated from `manifest.h`.

⚠️ **NEW ITEM, from dB — a comb of exactly 1000 Hz**, from 1 to 23 kHz, +17 to +35 dB,
**immune to the three power topologies**. 48000/48 = block rate. The test
that decides is **block 48 → 96**: if the spacing drops to 500 Hz, it is manipulation
to confirm. `src/medir_pente.py` measures the spacing (the `cotuca_measure`
answers another question — it gives the peak, not the distance between peaks).

### End of 31/08 — the instrument is standing

Firmware **v0.227.0**. Three things changed at the end of the day:

**The levels came to say the topology:** `pre_level_ab` (before the
governor/limiter — feeds the protection) and `post_level_l`/`post_level_r`
(after the limiter — they only reduce). They are not redundant: they are on opposite sides
of the protection. And `pre_level_ab` **is born at 1.0**, the first deliberate exception
to «every param is born at zero» — it is for test and emergency, and the piezo should
enter hot into the output dynamics.

**The `board` shouts when the master is at zero**, and explains why the test tone
sounds and the piezos do not. That defect disguised itself five times in one day.

**The OLED panel is born off**, wakes on the encoder and sleeps at 5 s — and **lights only at the END of the gesture** — you turn whatever is needed with the screen still off, and it lights 500 ms after the hand stops, without having touched any value — it cost
**6 dB of noise floor** and a spike at 7040 Hz. See `HARDWARE.md` §1b.

⚠️ **State that is still lost on each flashing** (it is the argument for presets):
```
set microfonia1_hz_l 770 · set microfonia1_db_l 15
set microfonia2_hz_l 834 · set microfonia2_db_l 15 · set microfonia_on_l 1
```

🔴 **THE BRIDGE HAS TO BE POWERED — open defect.**

The Daisy does not boot with the ESP32 bridge unpowered: `D14` (RX of USART1)
hangs on a dead chip, and the main loop stops completing turns.
**Fastest health symptom: the serial opens in 0.0 s when it is healthy, and takes
~59 s when it is sick.**

Tested at the end of 31/08, and what the test corrected:

| condition | result |
|---|---|
| powered bridge | ✅ boots, instant serial |
| unpowered bridge, **with** a direct ground loop | ❌ locks up all the same |
| direct ground **or** via the Daisy | ❌ locks up in both |

⚠️ **The ground alone does not cure it** — I said it did, before testing, and dB
soldered because of that. Today's firmware fix (3 ms ceiling and backoff in the
transmission, v0.216.0) **is also not enough**: with the fault recreated, the board locked up.

**Path, for tomorrow:** the problem is on **reception**, not transmission —
stop and rearm `DmaListen` when the UART errors pass a threshold.
⚠️ **But with a new instrument first:** the sub-phase LED is only valid with the
loop STOPPED; with it running it gives arbitrary numbers, and I diagnosed
from one of them.

### What was resolved

| | |
|---|---|
| **Pots** | direction inverted, stops stretched (the copper stops at 0.0009–0.9756), and the hysteresis that measured from the center and silenced the iPad — **210 150 windows in 20 s at rest → 0** |
| **Drum microphonics** | **770 and 834 Hz**, found by the watchdog (199 and 252 of 999 captures, dominance 0.96/0.98), cut at 15 dB. **Resolved** |
| **Amp noise** | squeal = bank at the step-down's input · click = ground · hum = bridge radio via the shared rail |
| **CC collision** | the notch had entered 115, of `bench_boost`, on the same channels → moved to 82–86, and `Manifest::Colisoes()` was born |

### What was left to do

**Bench (hardware), with the plan written in [`ATERRAMENTO.md`](operacao/ATERRAMENTO.md):**
1. Separate the **`AGND` island** — piezos, DPDT, jacks, amp input.
   Check with the **three measurements in order** from
   [`ATERRAMENTO.md`](operacao/ATERRAMENTO.md) v0.2.0 — "with the Daisy out of the
   socket" was the old method, and with the revB net-tie it gives the verdict the
   opposite way.
2. **100 µF + 100 nF** on the ESP32 bridge's pins, and a dedicated ground wire to the
   star. ⚠️ 100 µF, **not** 2200 — it hangs off the step-down's output.
3. Keep the piezo cables away from the exciter cables (coupling **through the air**,
   immune to grounding scheme).

**Firmware:**
- **Automatic microphonics detector.** The argument closed today: it is not only
  that each piece in the series will have its own modes — the modes **change during the
  session**. The watchdog already has the pieces (measured dominance, appearance count,
  95% duty cycle); the automatic decision is missing.
- **Two mouths may not be enough** — four modes capable of screaming have already been seen
  (770, 834, 930, 1403). Going up to four is a constant in `notch.h`.
- **The notch did not pass the IFA10000.** A new effect, certification owed.
- **Presets** — blocked by two decisions not made (see below). And today
  the cost became concrete: two values were tuned by measurement that only exist
  while the board does not restart.

**Decisions only dB can make, and that block the presets:**
- Do `post_level_l`/`post_level_r` enter the global preset? *I recommend not* — they follow
  physical buttons, like `exc_on`.
- Rename `test_tone_*` → `test_tone_*` to align with the Loopy widgets?

Both are free **only until the first capture** freezes the map (today `0x3505`).

## How to resume

```bash
cd ~/CODE/GitHub/Cotucatronics/src
make BOARD=daisy_seed && make check
# Simple RESET (WITHOUT holding BOOT — that enters the STM's DFU, where the
# bootloader lives). Flash in the ~2 s grace window:
make program-dfu
./cotuca_serial.py --send board mods list --gap 2
```

The board runs from the QSPI via the Electrosmith bootloader (`APP_TYPE=BOOT_QSPI`),
480 MHz permanent, `-O3` (see the `-O3` scar in ch. 3 of battle
scars — it MADE IT WORSE; reverting requires remeasuring everything).

---

## What plays today

**A pedalboard of five resident effects, in fixed order.** Block **48**.

```
piezo/line → src_mix → in_gain → gate → DRIVE → PHASER → WARBLE   (per chain)
            → 2×2 MATRIX ─┬─ DELAY (one instance, does not follow the matrix)
                          ├─ FREEZE  (per chain, follows)
                          └─ [SHIMMER ↺ REVERB] (per chain, follows)
            → level → governor+SSB per branch → limiter → out_l / out_r
            → [DPDT 3 pos: jack | both | amp]        ← this is COPPER
```

`piezo_mix` is BALANCE, not crossfade: at the center both piezos enter full.

**The `out_width` (mid/side) was retired on 20/08** — with two transducers
in the same body there is no stereo image to widen: the sum is acoustic, in the object.

**The matrix decides which CHAIN enters which OUTPUT** (`route_a_l` … `route_b_r`,
CC 97/98). The destination of each output is not the firmware's decision: it is the DPDT's
position. The firmware names OUTPUT; the copper names the destination.

The shimmer has no reverb of its own: it feeds back into the Freeverb that is already in the
chain, and that is why it appears inside the loop.

| Resource | Measured |
|---|---|
| CPU (suite + shimmer + detune) | **59.5% average / 65.8% peak** of the 1000 µs budget |
| Tables | **91/96 params · 91/128 CCs** (the `board` declares, 28/08) |
| Block | **48** (1 ms budget) |
| Cost per effect | drive +21 µs · warble +14 · phaser +26 · detune **+125** |

---

## ✅ The repo state is INSTRUMENT

Resolved on 2026-08-17. The dynamics returned and the board **comes up playing**:

| | |
|---|---|
| `pre_level_ab` 1.00 · `in_gain` **8×** | `gate_thresh` 0.02 · `gov_ceiling` 0.85 |
| `rev_mix` 0.35 · `fx2_hp` 120 Hz | `fx2_on` 1 · `buzios_on` 1 · `buzios_thresh` 0.04 |

`shift_hz` stays at 0: it is anti-feedback of the piezo→body→piezo loop, and turns on
when the **exciter** enters the path — before that it only metallizes the timbre.

> **When the exciter enters, review `pre_level_ab` and `gov_ceiling` together.** There
> is no attenuator there: the TPA3116 receives whatever the board sends, and the two
> form the pair that decides where the loop stops growing.

---

## 28/08 — the routing matrix, measured on the board

**What changed.** There were two stages confused into one. Now they are separated:

| | where it acts | what it does |
|---|---|---|
| `route_a_l/b`, `route_a_r/b` | before the effects | which **chain** enters which **output** |
| `out_l`, `out_r` | after the limiter | cuts the whole **branch** |

The matrix is born full and reproduces exactly the `out[0] = out[1] = A + B` that
preceded it. It serves to invert the chains when the transducers are connected
the wrong way — and **stays out of the presets**, because it is a fact of wiring and not of
timbre.

**T7c measured in v0.178.0**, path floor at −69.2 dBFS, base at −35.6:

**Redone with one output per channel** (`OUT_L → ch1`, `OUT_R → ch2`, separation
59.6 dB). The first run had the two outputs summed into one input, and so
two of the criteria passed **without being able to fail** — see `TESTES.md`.

| criterion | OUT_L | OUT_R | |
|---|---|---|---|
| `route_a_l=0` cuts A on L, and only on L | −44.8 | +0.0 | ✅ |
| `route_a_r=0` cuts A on R, and only on R | −0.0 | −43.9 | ✅ |
| `out_l=0` cuts the L branch, and only L | −44.1 | +0.0 | ✅ |
| `out_r=0` cuts the R branch, and only R | +0.0 | −41.5 | ✅ |
| **inversion** A→L becomes A→R | −32.6 → −77.4 | −80.9 → −36.7 | ✅ |
| the delay survives the matrix | — | +41.0 dB above the floor | ✅ |

The last one separates the stages: the matrix takes the dry, the freeze and the space from
chain A, and the delay return stays — a single instance, sends already summed.

The test lives in `src/teste_roteamento.py`: it asks the port, finds the interface
by name, **measures** the channel map and aborts if the two outputs fall on the
same.

⚠️ **The params map changed: `0xFD72` → `0xBE1B`.** Since no preset exists
yet, it came for free — it was the last window.

### Open defects this session left

- ✅ **`ccmap.html` / `ccmap.pdf` — RESOLVED, and became GENERATED.** The
  `gerar_ccmap.py` v0.3.0 writes **both** destinations (`CCMAP.md` and the A4
  card) from `manifest.h`. The human notes live in a `NOTAS` table — the
  only part written by hand — and the generator **flags an orphan note** (a param that no
  longer exists) and **a note that lies about topology** (says "compart." on a paired
  param). It found **ten wrong notes** on the first pass, including
  `frz_size` saying 20–400 ms with the manifest at 20–165 (the case that generated the
  range rule) and `test_tone_out` with the inverted semantics. The pair collapses
  into one line (`in_gain_a/b`), and so it fits again on **one sheet**.
- ✅ **`OUT_L` — RESOLVED on 28/08.** It was wiring, and the board was exonerated
  by internal witness before touching a wire: with the tone at `dest 1` the
  `gov A` rises to 0.285 while `gov B` stays at 0.001, and they invert with
  `dest 2` — signal on both branches, symmetric, inside the firmware.
- ✅ **Bridge dropout counter — RESOLVED.** `ponte` v0.5.0 saves in
  NVS, with **two counts**: `quedas` for this session answers *"is it dropping
  now?"*; `desde sempre` + `arranques` answers *"did it drop while I was not
  here?"*, and the second distinguishes unstable power from unstable radio.
  Deferred write (max. 1×/10 s, outside the BLE callback, because the NVS has
  finite endurance and the piece stays on for months). Persistence **proven**: the
  boot counter went 2 → 3 → 4 across resets.
- ✅ **The 20 minutes of the BLE link — PASSED on 28/08.** `0 quedas`, with the iPad
  awake and Loopy in the foreground. And `arranques` rose **exactly
  one** since the previous reading — and that one was the probe opening the port —,
  which also closes the other hypothesis: **the bridge did not restart.** A power
  flicker would have appeared there.

  ⚠️ **The probe knocks down what it measures, and this has to be written down.** Opening the
  ESP32-C3's serial port **restarts the board** (the native USB reads the opening
  as a reset, even with `DTR`/`RTS` false). The first attempt of this
  test died this way: I read the `board` and knocked down dB's connection. To
  observe live, **open ONCE and hold** — `/tmp/teste20.py` does
  that. And the previous run, which "failed at ~6 min", **does not count as
  evidence**: I read the `board` in the middle of it.

  **The NVS paid for itself on the first real use.** Reading the bridge requires
  restarting it, which erases the session counters; only the lifetime one survived,
  and it was the one that gave the result. Three days ago this number would have disappeared
  in the act of going to fetch it.

  Not proven: the message counters went with the reset, so there is no
  numerical proof of traffic — only dB's observation, who was playing.
- ✅ **Ceilings raised, with margin in hand and not with the overflow.** `kMaxParams`
  96 → **128** and `kMaxPresetVals` 40 → **56** — this was the tightest
  (33/40), because a new chain param spends one slot here and **two** in the
  params table. Measured cost: **+1504 bytes**, SRAM 55.59 % → 56.00 %. And the
  preset overflow alarm gained a **consultable twin** in the `board`: it only
  came out at boot, and whoever connects the serial after never saw it.
- ✅ **`varre` blind spot — RESOLVED.** `varre tudo` reads the **31** pins,
  with the declared price: it kills the OLED and the MIDI, because they are the same pins.
  `reboot` undoes it — a new command, because a mode that only exits with someone's
  finger does not serve a sculpture in a gallery.
- ✅ **`check_docs` v0.12.0 watches `.py`.** The old rule crossed the header
  with `#define`, and a script has no `#define`. The new one collects **all** the
  self-identification — docstring, `VERSAO` constant, and what it **prints**
  — and requires that they agree. The one that matters most is the third: it is the only one the
  operator sees. It found a second case on the first run (`pitch_spectrum.py`
  said 0.2.0 and printed 0.1.0).

---

## 28/08 — the panel: OLED alive, three encoders counting

**The OLED works.** It fills, clears and writes formatted text, and the mirror of the
encoders draws the three lines with the counters. It was the first time — what
was seen before was **retained image** from a previous session, because nothing
initializes the panel at startup (see scar 15o).

Bench tool that stayed, and that proved the five wires one by one:

| command | what it does |
|---|---|
| `oled` · `oled off` · `oled texto` | initializes/fills · clears · **writes** (proves more than filling) |
| `oledvel <2..256>` | sweeps the SPI prescaler — eight speeds |
| `fio curto` | bridge-hunter on the ten pairs |
| `fio <rst\|dc\|cs\|clk\|din> <0\|1>` | holds the pin to measure **at the panel connector** |
| `varre` / `varre reset` | reads the **15 free pins** at once, without opinion |
| `varre tudo` | reads the **31** — kills OLED and MIDI until a `reboot` |
| `reboot` | restarts the app by software; the state returns to defaults |
| `encpino` / `encpino reset` | per-pin latch of the encoders, in both directions |
| `enc inverte <a\|b\|g>` | inverts the direction **at runtime**, without flashing |

⚠️ **The normal `varre` reads 15 of the 31 pins** — `D5`–`D8`, `D10`
(OLED), `D13`/`D14` (MIDI) and `D22`–`D30` are left out. That is correct for common use:
a probe that drives a peripheral pin breaks the peripheral. But it created a whole failure
mode that none of our instruments saw, and **`varre tudo` opens it**.

**Encoder G: phases swapped in `kEncPin`** (`D19` phase A · `D18` phase B ·
`D17` push), because as mounted CW counted backward. The swap lives in the pin
map and not in an inverted signal in the loop — the direction of an encoder *is* which
phase comes first, and inverting the result afterward would make `encpino` lie.

### 🔴 Open defect: encoder G is INTERMITTENT

The two phases of G **opened during the session**, without anyone touching the
hardware. `varre` proved it: fifteen free pins, and only the push switching
(`push 14 · crus +0`). **They came back during the handling of the harness** — and
intermittence correlated with handling is the diagnosis, not noise.

The weak point is still there and will reopen. **Resolder before the gallery.** The push
has its own circuit in the EC11, which explains it surviving while the
two phases died together: they share a common that the button does not use.

---

## 28/08, end of day — the listening session and what it cost

**🔴 The board locked up, and the phase LED was what found it.** `PollMidi`
drained with `while(m_tail != m_head)` — a condition that chases a pointer
written by the ISR. Under continuous BLE traffic the loop never ended: audio
playing, frozen screen, silent serial, and no watchdog fired because the
call always returns. Fixed with a snapshot of the pointer, **and the sibling
`PollSerial` got the same**. The `board` came to declare the drain
delay — a sensor that would have flagged this before locking up.

**🗑️ `in_gain` REMOVED — "if it doesn't sound, it doesn't stay"** (dB's rule). The
`Governor` is an AGC: 18 dB put on `in_gain` were 18 dB it took away
downstream. The control worked and was inaudible **by construction**; only the
step remained. Deep clean in code, generators and seven documents. **CC 21 free.**

⚠️ **What was lost:** the matching with transducers of different
sensitivity. The governor only REDUCES. If it appears in the replication, it is fixed on the
carrier board's divider — the tombstone is in `FEATURES.md` and `TESTES.md`.

**🥁 TAROLATOR — new effect** (`tarol_on` cc80, `tarol_mix` cc81, per
chain). It was born as an octaver by frequency division, and it does not make an octave: on a
membrane the modes are inharmonic and the divider trembles, generating percussive
noise with the envelope of the hit. **dB liked it and gave it the right name.**
Tone and tracking fixed (6000 and 1200 Hz); `mix` 0–4× with a 0.5 curve, because
0–1 linear left two thirds of the fader dead. Cost **+2.9 %** in both
chains, measured in A/B on the same binary — it does not enter the sacrifice order.

**Also:** `wrb_rate` became **0.5–12 Hz** (was 0.05–3, and the top was already
slow); **`shm_on` opens the space on its own** — before it only ran with `rev_on`, and
the switch existed deaf.

### The measurement that changed the design

The membrane was measured instead of described: **76.5 % of the energy between 640 and
1280 Hz, ZERO below 160 Hz**, dominant mode at **768 Hz**. "Thin" is
literal. That killed the drive suggestion (it would add where there is already excess) and
resurrected the octaver I had objected to — the fundamental was not low.

### Input

Piezos at **−17 dBFS** (string, normal touch), **−7.5** (string, hard) and
**−21.3** (membrane). The membrane arrives **14 dB below** the string. Separation
between the two piezos through the body: **34 dB**.

---

## 28/08 — the potentiometers

**Two pots, and only two fit:** `A0` = D15 (L branch), `A1` = D16 (R branch). The
other three of the ADC block are the global encoder — `A0–A4` are aliases of
`D15–D19`. `num_knobs` went from **5 to 2**; declaring five made the loop
iterate over nonexistent knobs.

**New params:** `post_level_l` / `post_level_r` (CC 116/117), continuous volume per branch,
**after the limiter** — same point as the `out_*` mute, and for the same reason: there
you can only reduce. They are different functions in the same place — `out_*` kills the
branch (safety, instant), `vol_*` doses (performance, continuous).

### The three problems, and none was reading the ADC

| | |
|---|---|
| **the old guard killed the MIDI** | it was `if(v == knob_last[k])` — float equality over noise **never fires**, and the pot would write on every turn of the loop. The comment that was there predicted it: *"without it the MIDI stops working silently the same day"* |
| **absolute pot, shared param** | **soft capture**: it only takes over when it passes through the current value, releases when the iPad writes. Release window larger than the capture one, otherwise it changes owner at the boundary |
| **but at startup the truth is the button** | the first reading **adopts** the physical position — a volume that is born far from the button startles whoever turns it on |

**Meter:** `knob` shows raw, filtered, param and owner; `knob <s> <c> <l> <z>`
adjusts the four numbers without flashing, because only the bench knows the noise of this
ADC.

**New native benches:** `src/teste_tarol.cpp` (the divider on periodic
material) and `src/teste_macro.cpp` (the round-trip of `Macro::Inverso`, on which
the capture depends — if it errs, the pot "doesn't catch" and the bench blames the ADC).

⚠️ **None of this was flashed or measured on the board** — dB dismantled the bench.
The `knob` says on the spot whether the ADC reads.

---

## 31/08 — the pots work, end to end

**`post_level_l` / `post_level_r` follow the two potentiometers.** Full travel
verified: `0.9752 → 0.0001` with the param following along. Noise at rest
**σ 0.0003**, which is 26× below a MIDI step, and **zero writes**
with the pots at rest.

Capacitors of **0.1 µF from the wiper to ground, on the pot itself** — they stay, and
are worth it: **29× better** on the noisy channel, and the asymmetry between the two
disappeared. With the signal clean, the filtering was loosened (filter 12× faster,
hysteresis 2.4× finer) with no cost at all in stability.

### 🔴 The instability was a short, and I accused three innocents

The board kept falling off the bus during assembly. In two hours I accused **the USB
hub** (the Daisy was in no hub), **the capacitors** (I measured with the
pots disconnected) and **the circuit** (I cut the power between tests, and
credited the recovery to the wrong variable).

It was an **intermittent short between PWR and GND of the bridge**, leaning in when moving the
cables. Brownout — the board loses power and vanishes from the bus; it is not
a lockup. **dB found it, looking at the wires.**

⚠️ **Isolate those two wires.** An intermittent power short disappears
whenever you go looking for it.

## 31/08 — and the ADC defect was firmware

**`post_level_l` / `post_level_r`** (CC 116/117) follow the two physical pots. It cost a
whole session, and the culprit was not the copper:

### 🔴 The free-pin scanner was stealing the pin from the ADC

`D15` and `D16` were in `kLivres`, and `VarreLivres()` — which runs **on every
turn of the loop** — reconfigured them as digital on the first pass,
ripping them from the **analog** mode. The ADC sampled a disconnected input, the
two channels read the same ~0.12, and **nothing done to the copper could change anything**: the
firmware always arrived first.

dB swapped wires, removed a capacitor, measured 3.3 V at the ends and swapped the
pins with each other — all in vain, because of me.

> **A list of "free pins" is a claim about OWNERSHIP, and it ages.** Third
> time this month: `num_knobs = 5` with two pots, the `kEncPin` exemption after
> it became a source, and now this.

The latch now lives inside `VarreLivres` and shouts instead of stealing quietly.

### ⚠️ And the two probes I built to hunt it lied

**`knob pino`** held the pull for 5 ms; with 0.1 µF and ~40 kΩ the time constant is ~4 ms —
it read before the capacitor charged, and gave *"ON"* to the case that existed to
catch. Now it waits 60 ms.

**`knob prova`** drove the pin as a digital output, which on the STM32 **turns off the
analog switch** — the ADC stops seeing the pin. It was guaranteed to say
*"does not follow"*, and I used that to announce a conclusion.

What resolved it was no probe at all: it was reading the code looking for **who
else touched those pins**.

### Measured state

| | |
|---|---|
| writes at rest | **0.07/s** (were 623/s before quantization) |
| `k2` noise (D16) | σ **0.00085** — clean |
| `k1` noise (D15) | σ **0.0055** — six times worse, without a filter capacitor |

⚠️ **Restore the 0.1 µF, now in PARALLEL** (pin → GND), at least on D15. With
the signal clean the filtering can be loosened and the pot becomes more responsive.

### Also in this session

- **`min_knobs` 4 → 2.** Lowering `num_knobs` made `CheckRequirements()`
  refuse to boot — correctly. The **refusal came to explain itself**: the LED
  counts the fault code (1 audio, 2 RAM, 3 knobs) with the grammar of the
  phase instrument, and the text **repeats**, for whoever connects the serial after.
- **`AdcInit` moved out from before `StartLog`.** Initialization that can fail does not
  live before the channel that would report the failure.
- **Quantization at the wire step** (1/127 with 0.85-step hysteresis) instead of
  a magic dead zone — the pot has the protocol's resolution, no more and no less.
- **The pot closes the MIDI reception of its param** when writing, instead of waiting
  for the mirror to announce it up to 8 ms later.

---

## Versions

**They live in the file map of [README.md](../README.md), in one place only.**

This section once had a table of its own. It reached `main.cpp v0.85.1`
while the board ran v0.130.0 — **45 versions of drift**, because it was a
second copy that no one printed and no one read. Same form that the
`main.cpp` header carried (scar 13k and the checklist rule):
*a number no one prints has no one to correct it.*

The version that counts is the one the **board says at boot**, and `flash.sh` checks it
against the binary after flashing.

---

## Closed on 18/08 — MIDI certified under load

**T16c passed.** 2 779 CCs in 90 s with the load at **65.1%** of the block
budget: zero orphans, zero resync, zero lost, zero UART errors. The queue's high-water
mark stayed at **32 of 128 bytes — identical with and without load**, which
closes with a number the `PollMidi` hole (bytes enter by DMA, but what
dispatches is the loop). The 32 bytes are the size of the BLE burst, not loop
delay: if it were latency, it would have risen with the load.

**Rule revoked:** the BLE pairing **survives** the reset. The
`flash → re-pair → test` was superstition born of a log that I
myself silenced — case 8 of battle_scars, with an addendum in chapter 11 and the
BOARDS.md section marked REVOKED.

**Scars became skills.** `.claude/skills/`: `ifa10000`,
`medicao-limpa`, `instrumentacao`, `bug-de-ordem`, `edicao-segura`. The
IFÁ10000 gained **step 0 — CLEAN MEASUREMENT**.

---

## Unplanned sonic gain (18/08): the low end came back

dB's acceptance, without my having touched anything in the timbre: **"a presence of low
end it never had before, much better sonically."**

**Cause, verified in the code:** the 120 Hz high-pass lives inside the Fx2
block. Empty Fx2 slot ⇒ `fx2_want` false ⇒ block skipped ⇒ **no
filter**. It existed only to contain the low-end overflow that the Fx2 module
caused, and it left along with the cause because it was installed in the right scope.

**Secondary contribution, not measured:** `block_size` from 256 → 48 knocks the
latency from 5.3 ms down to 1 ms. In a fed-back instrument that shifts where
the acoustic loop settles and tightens the transient. Plausible and not
isolated — if we want the number, it is a flight with one variable.

---

## ✅ Granular freeze — CERTIFIED (2026-08-18)

IFÁ10000 complete. **Step 9 passed:** *"it turned out great like this"* — a 10 s
sustain with no pulsation, a 2 s tail, the live instrument audible underneath.

| Step | Result |
|---|---|
| 4 MEMORY | 768 KB SDRAM, 4 **sequential** reads/sample |
| 5 LOAD | **98 µs** of the effect in 500 of budget; 370 of 1000 in the system |
| 6 MATERIAL | real transient (tap on the sculpture) |
| 8 SAFETY | outside the cowrie shells — **GESTURE** category |
| 9 ACCEPTANCE | ✅ dB approved |

**Three versions to get here, and each one fixed a design mistake of mine:**
- **v0.1.0** failed by ear: linear sweep = 2 s loop. I had
  written in the header that the jitter would kill the periodicity. It does not.
- **v0.2.0** swapped the sweep for a **draw within a 500 ms core** — it killed the
  cage and, in the same gesture, broke the window's correlation premise,
  creating the low gain. *Window and position strategy are a pair.*
- **v0.3.0/0.3.1** closed it: **root-of-Hann** (constant power for decorrelated
  grains), **mix as a layer** and not crossfade (the crossfade
  erased the live instrument), and a **2 s tail**.

---

## For tomorrow — dB's requests (18/08, end of session)

The instrument is **working and approved** ("all ok! excellent"). Three
freeze adjustments, and all three have a common design root:

**1. `frz_mix` is crossfade and should be a LAYER.**
```cpp
l = l * (1.f - mix_) + wl * mix_;   // mix 1.0 ERASES the instrument
```
Freezing should **add** a sustained layer, not replace the source. It is the
cause of "when the frz is running I don't hear the instrument properly" — it is not
a side effect, it is what the line does.

**2. Freeze gain too low.** Technical cause: two Hann windows sum
1.0 only for **correlated** content. With the positions now **drawn**,
the grains are decorrelated and sum in power — ~0.7× of the expected
amplitude. The right fix is not a makeup gain: it is a **root-of-Hann window**
(`sqrt(hann)`), which gives constant power precisely for decorrelated
grains.

**3. Tail on the cut.** `frz_on` 1→0 cuts dry; it sounds like a plug yanked out, not
like a gesture. ~300 ms release ramp, with the grains running during the descent.
*dB's pending decision:* fixed time or a `frz_release` parameter?
<!-- doc-check: ignore frz_release (unimplemented proposal) -->
> **Note:** `frz_release` is a **proposal**, not a parameter. The mark above
> declares it to the drift checker — a declared ghost stops being a
> ghost; a silent ghost breaks the build.

### Also in the queue, of the same design (CTOS protecting the base)
- **Remove `src_mix`, `gate_thresh` and `gov_ceiling` from the MIDI map.** They are
  routing and safety — they were exactly the ones that muted the instrument
  in the phantom eviction. *What is not mapped cannot be knocked down.*
- **Rate-of-change limit per parameter** coming from MIDI: a real fader moves
  in a ramp; a jump from 0 to 127 on a CC is a machine signature.
- **A reliable clock for the watchdog:** `hal::NowUs()` returns garbage
  sporadically (3 samples discarded in 12 min). The veto rests on it.

---

## Closed on 20/08 — the shimmer that burned

Two distinct causes, measured on the offline bench (the headers are pure C++,
so the host reproduces the exact loop):

**1. Aliasing in the pitch engine.** Reading the buffer at `ratio 2.0` doubles the
frequency; everything above 12 kHz crosses Nyquist and comes back **inharmonic**. With
3500 Hz at the input, the highest peak of the whole tail was 19963 Hz. A 6th-order
Butterworth anti-alias **on the engine's write** (not on the shimmer — the defect is
reading fast, and what reads fast is the engine): a fold from −11.9 to **−64 dB**.

**2. Legitimate content in the ear's most sensitive band.** The spectral flatness
of 2–6 kHz is 0.009 — tonal, not noise. The sculpture's 1–3 kHz goes to 2–6 kHz
by the octave and the reverb sustains there. Fix: **a −6 dB bell at 3 kHz**, with
the depth chosen against the measured ratio of the dry tail (0.28), not by
ear.

| | v0.1.0 | v0.3.0 |
|---|---|---|
| ratio (2–6 k)/(300–2 k) | 1.01 | **0.19** |
| tail centroid | 7134 Hz | **1907 Hz** |
| 6–12 kHz | 28.2% | **4%** |

`shm_tone` (**CC 57**) moves the two things and **rises to brighten**: 0 is the full
bell with a cut at 6 kHz, 1 opens up to 18 kHz. Zero has to be the good state, or
the harshness comes back on every boot.

---

## Closed on 20/08 — the CTOS sum ceiling

The last structural boundary. The §3 watchdog protected against **one defective
effect**; the protection was missing against **N healthy effects that together do not
fit**. They are situations of a different nature and require different mechanisms —
the distinction is entirely in [CTOS.md §3b](CTOS.md).

**There is no culprit in the sum ceiling**, and that is why it does not look for one:
any rule that tries to find "who did it" when there is no one ends up inventing
a culprit, and the invented one would be a correct effect. The policy is **priority
declared cold** by dB in `Manifest::sacrif[]` — the order of the array *is* the
priority. LIFO was discarded: it punishes by arrival order, which has no relation
to musical value.

Three tests, all passed in v0.84.0:

| Test | Expected | Result |
|---|---|---|
| ceiling 620 µs (block 639) | **only** the detune drops | ✅ block 639 → 526 µs, the other five playing |
| ceiling 200 µs | cascade in order + "no output" state | ✅ detune→phaser→warble→drive→freeze→shimmer, then *"the base alone does not fit"* |
| `ctos rearm` | releases latches **without** turning back on | ✅ |

With the six effects on the block stays at **577–639 µs with a ~700 peak**, against
a ceiling of 850 (85% of 1000). The whole suite still fits with margin. The mechanism
cost **+14 µs**.

**Two corrections the tests forced:**

1. **The serial died in the first test** — I called `LogAlways` and
   `MidiSendBytes` from inside `AudioCb`, which is the priority 1 defect
   of this document, catalogued before I committed it. It locked up the whole
   instrument, not only the CDC. Fixed: the callback marks (`volatile`), the loop
   speaks.
2. **It knocked down two when one was enough.** Zeroing the param does not make the effect stop
   costing — the `EffectSlot` descends by ramp and keeps processing. The
   mechanism measured again before its own action took effect. Fixed with a
   **250 ms grace** after each drop.

---

## Pivot of 20/08 — two character chains, measured

```
p1 → in_gain_A → gate_A → [DRIVE·PHASER·WARBLE] ┐
                                                 ├→ MIX → [DELAY·FREEZE·SHIMMER↺REVERB]
p2 → in_gain_B → gate_B → [DRIVE·PHASER·WARBLE] ┘   → level → OUT_WIDTH → split
```

Firmware **v0.91.0** · **71/96 params · 36/48 CCs** · SDRAM freed: 153.6 KB.

| | µs |
|---|---|
| everything on | **620 average / 663 peak (66%)** |
| base (nothing on) | 160 |
| the six character ones | ~130 (90 the first to turn on, 38 the second — I-cache) |
| tail (reverb 99 + freeze 69 + shimmer 202) | 370 |

**The asymmetry between chains was I-cache, not code.** Whoever turns on first
pays ~90, whoever turns on after ~38, whichever chain it is — three runs
in agreement with the order inverted. The two chains are symmetric.

**The decomposed base killed the optimization case:** of the original 182 µs, the
real waste was the two gates with the gate open. The delay module (45),
the bone of the dynamics (94), the measurement (15) and the warble writes (7) are all
doing something. `thresh > 0` guard implemented — **26–30 µs measured A/B on the
same binary**.

**Open:** acceptance by ear on the hybrid instrument, and IFÁ10000 of the six
effects in the new positions (they do not inherit the acceptance of the old chain).

---

## Closed on 20/08 — the external DSP engine left, entirely

The delay was the last consumer of the runtime. Rewritten in C++ (`delay.h`, graph
read from the export: fixed feedback **0.4**, crossfade, mono on both channels), the
runtime left completely.

| | before | after |
|---|---|---|
| everything on | 678 µs | **545 µs / 656 peak** |
| base (nothing on) | 182 µs | **115 µs** |
| the delay | 45 µs, **always** | +11 µs, only when on |
| binary | 212 424 B | 175 980 B |
| params | 61 | **54** |
| repo | — | **−8 MB** of export |

**The 67 µs of the base are the datum that matters:** a `Module::Process` runs on
or off, because the module does not know it is in bypass. Zero-cost-when-off,
which every in-house effect follows, was **structurally impossible** inside the
slot system.

Left along with it: `MODULE_SRC`, the export directory, `module.h`, `mod_slot[]`,
`ModBind` and the
push-by-name, six files, four exports, the module's six orphan params,
the fx2 watchdog that judged by the global CPU (**finding 8**), the `probe` command
and the note-by-note histogram (**finding 7**).

**Method in scar 12f:** remove the root symbol and let the compiler
enumerate the dependents. Four errors on the first pass, then one, then
zero — with no calculated range and no lost prose.

**CC map redone:** every param receives a CC (54/64), one decade per effect.
Nothing else serial-only.

---

## Closed on 20/08 — PARALLEL tail and two spaces

dB asked for p1 with delay and p2 with reverb, and then string with shimmer and leather
with clean room. The serial tail could do neither.

```
A ─┬→ ×dly_a ──┐
B ─┼→ ×dly_b ──┴→ DELAY  (one instance) ───┐
   ├→ ×frz_a ──┐                           │
   ├→ ×frz_b ──┴→ FREEZE (one instance) ───┤→ sum → level → split
   ├→ ×esp_a ───→ [SHIMMER_A ↺ ROOM_A] ─────┤
   └→ ×esp_b ───→ [SHIMMER_B ↺ ROOM_B] ─────┤
   └→ dry (both, whole) ───────────────────┘
```

**Delay and freeze remain ONE**, with a send per chain — the requested gesture
was about space, and duplicating them would cost CPU without delivering a difference. **The
space duplicates for real**, because shimmer is not an effect alongside the reverb:
it *is* the reverb fed back with pitch, and two wettings require two rooms.

| | predicted | measured |
|---|---|---|
| A: delay+shimmer · B: clean room | 625 µs | **490 / 530 peak — 53%** |
| worst case: both with shimmer and room | 807 µs | **598 / 639 peak — 64%** |

The worst case came in **209 µs below** the prediction: I summed costs measured on the serial
tail, where each effect processed the whole signal. In parallel each one
receives only its own send — **two rooms cost less than twice one**
(scar 12g).

**Declared consequences:** the delay stopped being reverberated (the echoes come out
dry, the room works on the source); the internal mixes became per-chain
sends; and the freeze became **two independent ones**, one per chain, with its own
buffer: `frz_on_{a,b}` **are** the freeze gesture, with `frz_vol` and
`frz_size` per chain.

> **Drift correction (22/08).** This paragraph for months described a
> design of "source and gesture separated", with a `frz_hold` that **never
> existed as a parameter** — `manifest.h:264` already recorded the
> opposite decision and the doc stayed in the old model. Found by the doc↔code
> drift audit; see bts 13h.

**Anti-feedback protection duplicated** for branch B (hardware blocker for
two exciters): governor and boost per branch, `shift_hz` shared — two
exciters on the same body with different shifts would beat against each
other. **Panel — pins (22/08).** D26 = B1 randomizer, D27 = B2 assignable,
**D25 = B3 CTOS rearm** (new in v0.111.0). D28 = WIDI LB.
D29/D30 reserved for the exciters' DPDT switches. The three buttons use
an internal pull-up and are active low, so a pin with no button reads "released".

**The Seed's LED carries two facts in a single cadence:** slow = live
loop with no lock; **4× faster = there is an effect knocked down by the ceiling,
press B3**; frozen = dead loop. The alarm entered the cadence
precisely so as not to spend the state, which is the proof of aliveness.

**Open, and it is the most expensive now: there is no preset in flash.** Every flashing
zeroes every parameter and `pre_level_ab` is born at 0. The loss is silent on both
sides and the symptom imitates an effect defect — it cost two false
diagnoses on 22/08 (bts 12l).

`exc_filter_l/b` gate the protection; when the DPDTs are soldered, D29/D30
replace the param with the real state.

**Simplification after (v0.106.0):** `esp_on/esp_mix` and the old `rev_mix`
were **the same control twice** — gain at the input and gain at the output of
the same linear room. dB saw it before me. It became a single send per destination
(`rev_on`/`rev_mix`, same form as the delay and the freeze), whole return, and
`shm_mix` back to a single job. Scar 12h.

**CC map: 75 params, 75 CCs, nothing serial-only.** One range per family, and
every destination with the same form: `on_a, mix_a, on_b, mix_b`, then the
shared ones.

---

## Params audit (20/08) — a feature was dead

dB asked for an audit of all the params, one by one. Three layers, fifteen
minutes:

| layer | result |
|---|---|
| who reads each param | 🔴 **cowrie-shell detector off for 5 commits** |
| range × internal lock | 🟠 `frz_size` with 61% dead travel (corrected) |
| live min/max sweep | ✅ 75/75 reach the declared extremes |

**The cowrie shells were dead since `6a8175d`** — the detector lived post-gate,
inside the stretch I replaced in the restructuring into two chains. Nothing
flagged it: the variables stayed declared and read in the loop, the params in the
table, the CCs in the map pointing at nothing, and the `board` reporting everything correct.

**A dead feature looks like a disabled feature** — and `buzios_on` is born at 0, so the
normal state was the broken state. Restored from `3303028` and tested: eight
pulses, eight draws. The position changed meaning — it now reads the sum of the
two chains, and a hit on any piezo fires.

**False positive recorded:** layer 2 flagged `drv_drive`, `drv_tone` and
`phs_rate` for wasted range. They are deliberate. The effect's lock is a safety
net, not a target — only a range LARGER than the lock is a defect.

The audit became a section of the IFÁ10000, to run after any refactor
that touches the callback.

---

## Where we are (23/08, v0.155.0)

**The whole controller chain is proven, and the panel lit up.**

| front | state |
|---|---|
| per-chain character params | ✅ indistinguishable cost, measured with A/B on the same binary |
| per-channel addressing (ch1/ch2) | ✅ 37 pairs, native bench |
| mirror on ch4/ch5/ch6 | ✅ Wrench saw ch5; ch4 and ch6 to confirm |
| return wire (iPad → board) | ✅ 93 CCs counted |
| presets in RAM | ✅ cycle `capturar → dump → reset → colar → load` |
| freeze B button (D22/D23) | ✅ 4 hits for 4 presses |
| CTOS rearm via B3 | ✅ resurrected — had an unreachable branch |
| OLED SSD1309 | ✅ lights and writes. **Configured, never "present"** |

**What is missing is human:** remap Loopy (whole map in
[RETRABALHO-LOOPY.md](operacao/RETRABALHO-LOOPY.md)) and the **listening that
builds the library** — each judgment becomes a preset instead of a
note.

## Physical panel (23/08) — encoders alive, OLED dead

**The three EC11 encoders work**, with the map checked by measurement:

| encoder | phase A | phase B | push |
|---|---|---|---|
| chain A | D0 | D1 | D2 |
| chain B | D3 | D4 | D9 |
| global | **D18** | **D19** | **D17** |

**One pulse per detent, measured** — ten clicks gave ten. The division stays at 1.

**The OLED did not survive.** GND and 3V3 were inverted on the distribution
board, and the module received reverse polarity. Seven causes were
eliminated by proof — firmware (the binary that worked also gives black),
power, encoders, broken wire, bridge, speed and power-up order —
and everything that shares the same power board works.

**When swapping the panel, the test is one minute:** `oled` lights everything up,
`oled texto ola` proves pixel addressing and the DC.

### Probes that stayed, and serve each new solder

| command | what it answers |
|---|---|
| `varre [reset]` | **which of the 15 free pins have something that moves** — assumes no map |
| `encpino [reset]` | raw level of the nine pins, latch in both directions |
| `fio <pino> <0\|1>` | holds an OLED pin to measure at the connector |
| `fio curto` | bridge among the five, differential test |
| `oledvel <n>` | SPI prescaler at runtime |

All accumulate since the reset — **none opens a timed window.**

### Pin review enters the closing routine

`make check-docs` checks `PINOS.md` against the source's `seed::Dnn`, in three
columns: **in the code and not in the doc · in the doc and unused · RESERVED used.**
dB's request, and the reason is his: *the board is clogged and the solder points
cramped — checking from memory fails exactly when a mistake costs most.*

---

## Front in the queue — expand the cowrie shells (dB's request, 23/08, "but not yet")

**The cowrie shells have TWO cards** — `dly_time` and `dly_mix_a`, in an array that
fits 8. The manifest already recorded this as a known impoverishment, inherited
from the removal of the build's character module:

> *"The cowrie shells ended up with TWO cards instead of four, and the draw sounds
> proportionally poorer. There is no silent fix for that — either a
> character module comes back, or the decision is to accept the smaller draw."*

**There is a third path that text did not foresee:** the character duplication
of step 1 created **ten new params of the right category**. It is not
"randomize almost everything" — it is **filling the category that was already defined and
was empty.**

### Where to stop is already decided, and the reasons are musical

| category | out | why |
|---|---|---|
| **FRAME** | reverb | the still background that makes the contrast exist — *if everything jumps, nothing is a surprise* |
| **GESTURE** | freeze | a performance decision with a precise time; drawn it becomes an accident |
| **SAFETY** | level, gate, governor, limiter, shift | — |

**Legitimate candidates (20, against today's 2):** `drv_drive`, `drv_tone`,
`drv_sujeira`, `phs_rate`, `phs_depth`, `phs_fb`, `phs_stages`, `wrb_rate`,
`wrb_depth`, `wrb_irreg` — in both chains.

### What the work really is

Raising the array's ceiling is trivial. **The work is the draw RANGE of each
one**, and it is by ear: drawing `drv_drive` over the whole travel produces from clean
to destroyed on every hit. The record of the old note draw shows the
pattern — narrow range, chosen center, and in its case even scale
quantization.

And the warning the manifest itself left: *"the previous record drew correctly
but everything sounded low and similar — the record approved by ear was the one
above."* **Draw range is a musical decision, and the acceptance is the musician's.**

---

**Two small fronts with an argument of their own, both about "giving testimony to
what only discipline protects":**

- **A table of RESERVED pins in code**, with a `static_assert` over the pins
  effectively configured. Today `D29/D30` are reserved for the exciter DPDTs
  **only in a document — and the compiler does not read a document.** The OLED RST
  default is precisely `D30`: if the soldering had followed the default, the
  collision would only appear on the day of assembling the panel, with the wire in the enclosure, and nothing
  would have caught it before.
- **`defesas`**: an inventory of what was never exercised (bts 13w), three
  columns — already acted · has a trigger · neither.

**In the queue after that:** an inventory of what was never exercised (bts 13w) ·
unify the two global temporal refusals · preset pointers with
pre-listen · panel on the OLED · QSPI (ITCM window proven, indirect read
missing — do not spend a flashing without the RM0433).

---

## Preset route (22/08) — the hard part solved, the tedious part identified

**The ITCM window works.** The board leaves memory-mapped executing from independent
RAM, talks to the peripheral, returns and survives — six flashings,
`ITCMRAM: 212 B` proving the section was applied. **It was the architectural
risk of the preset route, and it is over.**

**What is missing is protocol**, which is reading work: the exact sequence of
indirect reading of the QUADSPI. Three live and named hypotheses, explicit
resumption condition — all in
[DESENHO-ESCRITA-QSPI.md](DESENHO-ESCRITA-QSPI.md).

> **Resumption condition: RM0433, QUADSPI chapter, indirect read.** Without
> it the front stays stopped — by sizing, not by giving up. Half
> an hour with the manual is worth more than ten flashings guessing.

The BOOT_SRAM remains parked on the `investigacao-boot-sram` branch, with the
hypothesis recorded for whoever resumes.

---

## Open defects

**✅ CLOSED 22/08 — the input does NOT saturate, and the divider left the queue.**

Measured with the piezos **mounted on the sculpture**, which is the usage regime:

```
p1 (IN_L): pico  -9,2 dBFS | 18,0% das amostras ativas | clip 0
p2 (IN_R): pico -19,5 dBFS |  1,3% das amostras ativas | clip 0
pos-ganho (in_gain 2,8x):   -0,3 dBFS                  | clip 0
```

**No clip anywhere.** The eight-step plan shrank to two: the
protection (1 kΩ + 2× 1N4148), which serves anyway because a piezo delivers
tens of volts in a strong transient, and recalibrating `in_gain`.

**Scar 12a got the outcome right.** The 20/08 measurement that said "the input
saturates" was done with a **finger directly on the loose disc** — the most violent case the
pickup encounters. Mounted and excited by vibration, it delivers twenty dB
less. *"Sizing before measuring in the right regime would be solving a problem
that maybe does not exist"* — and it did not.

**Two points of attention the measurement left:**

**The post-gain is three tenths from the ceiling** (−0.3 dBFS with `in_gain` 2.8×). A
stronger hit clips. There is margin at the input, not at the gain output.

**Ten dB between the two piezos, and p1 with 14× more active samples.** It may be
performance or coupling, and the measurement does not distinguish — the separation is **swapping the
piezos' channel** (scar 12b). While it is not done, `in_gain_a/b` exist
precisely to match the two.

**🔴 The serial dies under a long callback — priority 1.**
The callback's ~3.4 ms bursts starve the USB ISR; the host's CDC
rots. Worked around with `log_mute`, black box and MIDI, but the right
fix is designed and not done: a **kick-and-forget log ring**, in which
the loop never touches the USB and the drain happens by ISR.

**🟡 Block 256 costs 5.3 ms of latency.** In a fed-back instrument
that shifts where the feedback settles — it is the price of any FFT
module that requires it, and the reason the block is 48. Known constraint, not
resolved.

**🟢 Off-by-one in the applied counter.** `msgs completas N` always comes
**exactly 1** above `APLICADAS`, with and without load (65/64 and
2779/2778). A fixed difference, not a loss rate — probably the message
in transit at the instant of reading. Confirm before calling it cosmetic.

**🟡 SRAM pool at 92.4%.** A third module does not fit without changing the
allocation strategy (label hot/cold allocations before growing).

---

## Frozen queue (waiting for hardware)

T4 (buffer + piezo) · T7e (gate with a real transient) · percussive test of the
limiter · **panel v2 solder** (5 pots + 2 buttons; B2 is a natural candidate for the cowrie-shell
toggle).

---

## Next steps, in order

1. **dB's decision:** how much protection comes back → flash the instrument
   state as the repo default.
2. **Log ring** — kills the priority 1 defect by architecture.
3. **Stage defaults:** `buzios_thresh`, `buzios_cooldown`.
4. **Panel v2** when the hardware arrives (T11 already tests via serial).
