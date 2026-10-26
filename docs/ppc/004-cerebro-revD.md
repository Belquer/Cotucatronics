# ppc/004 — brain board **rev D**: remove what does not assemble
<!-- doc-check: ignore confere_arcade (function of pcb_contorno.py, in kicad/) -->
<!-- doc-check: ignore y_min (value of the FRENTE_CEREBRO constant, in pcb_contorno.py) -->

> **State: PLAN, with execution BLOCKED** on three measurements (§5). Written on
> 2026-09-19, with rev C on the bench being assembled.

- **Date:** 2026-09-19
- **Branch:** `claude/slack-session-sm2ldz`
- **dB's request (19/09):** *"clean all unused components from the
  board"* → option **(a)**: a new brain-board revision. The **test pads stay**
  (dB's decision, 19/09): they cost neither height nor money, and they are how the
  board is measured when it does not power on.
- **Pairs with:** `docs/IFA10000.md` (track M), `ppc/003` (3D volume),
  `caixa/LICOES-MONTAGEM.md` #10.

## §0 — The rule this plan exists to enforce

> **Rev C is FROZEN.** It is being assembled right now, and the manual, the BOM,
> the maps and the factory package are **generated from it**. A `git diff` in
> `kicad/cotuca_cerebro/` silently invalidates everything already printed.

Rev D is born as a **copy** (`kicad/cotuca_cerebro_revD/`), and the gate is
mechanical: if the tree shows rev C altered, the work is wrong, whatever
the result.

⚠️ **And the `main()` of `pcb_contorno.py` WRITES the board.** Running
`pcb_contorno.py cerebro` on rev C rewrites the fabricated file — it has
happened once (scar). In rev D, every check runs in **read mode**,
calling `confere_*` on the text read, as was done on 19/09 for the arcade buttons.

## §1 — What goes out, and why

| ref | what it is | why it goes out |
|---|---|---|
| `J4` | 1×2 header "the ESP32 bridge" | **vestige**: the bridge goes into the `U3` socket, which already brings 5 V and GND. `J4` only repeats those two nets. Three revisions saw it and none asked what it connects on the other side |
| `C11` `C21` | 22 pF C0G | **gain** leg of the preamp, which is today a follower (`R10`/`R20` = 0 Ω) |
| `R13` `R23` | 4k7 | same |
| `C13` `C23` | 22 µF NP | same |
| `RV1` `RV2` | trimpot 100 k | same |
| `J15` and `J16` go from **1×8 to 1×4** | 1×8 → 1×4 header | **decided on 21/09** (bts 15fc): the two switch poles are paralleled on the copper (`1`=`4`, `2`=`5`, `3`=`6`), so half the pins are convenience, not necessity. What stays is **common · jack · amp · drain**, and the paralleling is done with jumpers on the switch — which is how the working assembly is already built. Half the cable (4 ways instead of 8) |
| `J11` pin 1 (the connector becomes **1×3**) | 1×4 → 1×3 header | **decided on 20/09:** the module is powered through the **screw terminal**, so `J2` is left for the power and `J11` only for the audio (reference + L + R). Pin 1 is duplicated `+15V` (bts 15fb) |

**What STAYS, and is an explicit decision:**

- **`TP1`..`TP9`** — test pads (dB, 19/09).
- **The follower as it is.** Removing the gain leg is removing **empty pads**, not
  changing the circuit: no live net changes function. If someone wants the
  adjustable gain back, it comes back in rev E, with measurement — it is not what this plan does.

## §2 — Where each thing lives (measured in the repo, 19/09)

| artifact | who's in charge | note |
|---|---|---|
| `cotuca_preamp.kicad_sch` | **the file itself** | ⚠️ the generator `docs/operacao/esquematicos/revB/gen_preamp.py` is **two generations behind** and **is not run**: it reintroduces `U1A/U1B/U1C` and unannotated references. Here the rule "the generator is the source" is **inverted**, and its own header already says so |
| `cotuca_energia.kicad_sch` (has `J4`) | the file itself | same |
| `cotuca_conector.kicad_sch` | `gera_conector.py` v0.10.0 | generated; not touched by this plan |
| `cotuca_soquete.kicad_sch` | `gera_soquete.py` | same |
| `cotuca_cerebro.kicad_pcb` | placement/routing on the board | the parts come out of here **and** of the schematic, or the netlist stops matching |

Consequence: **§3 is editing `.kicad_sch` by hand**, and `edicao-segura` applies
(sweep by parenthesis count, never by indentation; count what you read).

## §3 — Order of execution

1. **Copy** rev C to `kicad/cotuca_cerebro_revD/`, with its own `.kicad_pro`.
   Record the rev C commit it came from.
2. **Measure the starting point**: `kicad-cli sch erc`, netlist, DRC. These three
   numbers are the "known good" everything is compared to.
3. **Remove the 9 symbols** from the schematic (8 from the gain leg + `J4`), one at a
   time, with a before/after count.
4. **Remove the 9 footprints** from the board, and the copper that only served them.
5. **Refill the zones** and run the DRC.
6. **Re-place the tall parts** — ⚠️ **§5 blocks this step**.
7. **Gates**: ERC 0 · DRC 0 · `encaixe.py` · `confere_arcade` in read mode ·
   `compara_netlist.py` against rev C.
8. **Documents**: BOM, manual and MANIFEST **keep pointing to rev C**
   while rev D is not fabricated. Rev D enters as a new line, not as a
   replacement.

## §3b — What the first assembly (22/09) added to the list

Three requirements that do not come from cleanup, they come from the bench — see `battle_scars.md`
**15fe** and **15ff**:

1. **Test points on the piezo path.** The board's eleven `TP`s cover
   power, grounds and the line outputs; **none** touches the input
   chain, which is the most sensitive and the one that failed. Rev D carries, per channel, a
   `TP` at the **preamp output** — which rests on VBIAS and says in a second whether
   `U1` is alive.
2. **The input divider with pads for two values.** The 22 nF `C12`/`C22`
   gives −6.9 dB with an 18 nF piezo, and the bench needed **−17.8 dB**. The
   rev D carries the measured value as a starting point **and** a second pad in
   parallel, because each source asks for its own — and the silkscreen says the two
   channels can (and should) be different.
3. **The `TP`s on the accessible side, with the name printed** (bts 15fg). On the brain board
   it is right by accident — it lies on the bottom. **On the panel it is wrong**: the 5 `TP`s
   are on `TOP`, facing the acrylic, and with the reference on `F.Fab`,
   which does not print. The next panel rev moves them to `B.Cu` +
   `B.Silkscreen`, and gains `TP`s on the buttons and on the encoder, which today have
   none. `encaixe.py` **M8** already fails this.
4. **The margin calculation written beside the value.** The original −6 dB was an
   estimate that no revision confronted with the real transducer. What closes
   this is not another value: it is the **sentence that says which piezo, which coupling and
   which measured peak** justify the value.

## §4 — Acceptance (what proves it is done)

- `compara_netlist.py` rev C → rev D: the **only** differences are the nets that
  died with the removed parts. Any live net that changes is a **defect**.
- ERC: **0 violations**, and no new exemption.
- DRC: **0**, with the zones refilled.
- `git status` of rev C: **clean**. If it is not, the work goes back.
- Rev D's BOM has no line marked DNP.

## §5 — ⚠️ WHAT BLOCKS, and it is not bureaucracy

**Resolved on 19/09, and contrary to what I wrote at first:** the board is
**rotated 180°** relative to the model. The proof is the brain board's silkscreen upside
down with the user's side facing front; the Daisy under the center button did not
prove anything, because it falls there in both orientations. In the real orientation, `C1`
(25 mm) sits under hole 1 and `U2` + `J13` + `J16` under hole 2.

**New requirement for rev D, and it is the most important in this document:** the
brain board's mounting is a **symmetric rectangle** (`H1`–`H4`), so the board goes in
both ways and the wrong one meets no resistance at all. Rev D carries
**asymmetric mounting or a keying cutout**: the wrong orientation has to stop
fitting. Documenting is not enough — it was already documented in a constant, and the
`encaixe.py` refuses if it changes, which made the tool believe the
declaration instead of asking the board.

**What is still missing is ONE measurement, and it decides the whole of rev D:** the
**depth of the blue button** below the panel. The checker uses the worst case
(34 mm, head = 0) and gives **7.5 mm** of clearance — but the Daisy is 17 mm and **fit,
snugly**, which is only possible if the blue one drops **~24 mm**. With the real number:

- if the blue one drops ~24 mm, the clearance under it is ~17.5 mm, and rev D **does not need**
  to re-place anything because of the arcade buttons — the cleanup (§1) is the whole work;
- if it drops more, `U2`, `J14` and `RV1` have to come out from under the discs, and the
  rev D becomes a **re-placement**, with the routing following along.

Measuring the blue button costs a caliper and decides between a small job and a
big one. **That is why §3 step 6 stays blocked.**

Another thing measured the same day, which rev D does not solve but the next enclosure
must solve: **the panel only comes out with the TRS jacks removed** (LICOES #11).
