---
name: caixa
description: Integrate the enclosure and the electronics of Cotucatronics end to end — which artifact rules when there are several, generate the cut by parameter, the five gates that refuse before spending sheet, and how the board becomes immune to thickness. Use when changing the enclosure's material or dimensions, when sizing or placing a PCB against it, and before sending anything to Ponoko or JLCPCB.
---

# Enclosure ↔ electronics — the whole path

## BOUNDARY WITH `kicad`

| question | who answers |
|---|---|
| does the enclosure close, fit together, fit on the sheet? | **this one** |
| does the board fit in the enclosure and do the holes line up? | **this one** |
| does the schematic connect? does the file open? | `kicad` |
| does the drawing read? | `design-review` |

This skill never decides anything electrical. It rules the **geometry between the
two artifacts** — and that is where this project has already lost more time than
in ERC.

## ⚠️ WHO GIVES WAY: the enclosure, to the board — in priority order (IFÁ10000-M)

Decided by dB on 09/18/2026, because **it is much easier to adjust a package than
a PCB**: the enclosure regenerates in minutes, the board costs a factory run and
weeks. It is a **priority order, not a law**:

1. **The board brings its own means of fastening.** Without holes there is no way
   to hold it, and no enclosure solves that — here the board gives way.
2. **With that guaranteed, the enclosure gives way to the board** in holes,
   cutouts, posts, angles, interior clearance, and assembly order: it is
   generated **from the board**.
3. **The board only gives way on something else with an excellent justification**,
   written, measured, and signed by dB.

Until 09/18 this house said the opposite ("the enclosure already exists, the
board gives way") — and the panel rev C was fabricated without a single fastening
point. The full certification is the **M track** of IFÁ10000, in the sub-skill
`/3ddesign` (`.claude/skills/3ddesign/references/ifa10000-m.md`), orchestrated by
`/aipcb`.

---

## 0. BEFORE MEASURING: which of the enclosures is the enclosure?

This project had **four** sources for the same enclosure at the same time, with
four thicknesses, and three of them gave plausible numbers. The order of
authority is not negotiable:

| artifact | does it count? |
|---|---|
| the piece **in hand** | rules everything — real kerf and tolerance |
| **generator of the piece being made right now** | yes — it is the source |
| cut file from a previous generation | **no** — it is a real object, but a *different* one |
| dimensioned drawing, fabrication page | **no** |

> **"The file that went to the factory" is not enough.** It has to be the one for
> the piece being made **now**. A real artifact from the previous generation
> measures just as well as the right one.

⚠️ And **confirm the material with dB**, not with the file name. The 6.4 walnut
cut was real, went to Ponoko, and was not this enclosure — the order was 3.00
acrylic.

⚠️ **And then DELETE the superseded sources**, do not archive them next to the
current one. That is what was done on 09/06: the 6 pages died, their two
generators, and the walnut files. Git history keeps them; the folder should not.
A superseded source next to the right one measures just as well, and the only
thing that distinguishes them is a check nobody does before trusting.

⚠️ **A verified absence has an expiry date.** "I looked for the generator and it
doesn't exist" was true at 9:00 and a lie at 9:20 — dB was writing it. In a
repository with two hands, look again before acting on an absence.

---

## 1. The enclosure comes out of parameters, never out of editing

```bash
python3 caixa/cotuca_caixa_corte.py --t 3.0 --kerf 0.2
```

Writes `cotuca_caixa_r<REV>_acrilico_3p0_<digest>.svg`, `_previa.svg`, and `.dxf`
next to the script. **The name carries the thickness** — a cut file without the
thickness in the name measures just as well as the right one, and that is exactly
how half a day was lost.

Everything that depends on `T` has to be **derived**, never written:

| quantity | formula |
|---|---|
| interior | `W − 2T` × `D − 2T` |
| bottom sheet | `(W − 2T − 1)` × `(D − 2T − 1)` — 0.5 clearance per side |
| usable height | front `HF − 2T` · back `HB − 2T` |
| standoff height | number of stacked sheets × `T` |
| standoff holes | 6 mm from the edge **of the sheet**, not fixed dimensions |

⚠️ **The finger depth IS the thickness.** A file cut at 6 and run at 3 mm gives
3 mm of clearance at each joint and 3 mm of protruding tab: the enclosure does
not close square and the solvent has nothing to bond. **Changing material is
always regenerating**, never just changing the request line.

---

## 1b. THE FILE NAME IS THE IDENTITY OF THE ARTIFACT

A fabrication file is not code: **once ordered, it exists as an object**, and the
name is the only thing that links the sheet that arrived to the drawing that
generated it. If two different drawings share a name, that link breaks — and it
breaks in silence.

It happened here: the enclosure **with** reinforcement and the enclosure
**without** reinforcement both came out as
`cotuca_caixa_corte_acrilico_3p0.svg`. One was ordered, the other was not, and in
the folder they were indistinguishable.

> **Two different things need two numbers.** The **generator version** says what
> changed in the code; the **artifact revision** says what changed in the piece.
> They are not the same: a new generator can emit the same sheet (I redid the
> joint three times without touching a single dimension), and the same generator
> emits different sheets depending on the parameters.

The name carries **four** things, and each one answers a different question:

```
cotuca_caixa_20260906_r3_acrilico_3p0_037a9c30.svg
             │        │       │   │   └── WHICH ONE — drawing digest
             │        │       │   └────── thickness
             │        │       └────────── material
             │        └────────────────── WHAT THE INTENT WAS — revision
             └─────────────────────────── WHAT COMES AFTER — revision date
```

⚠️ **A digest identifies and DOES NOT ORDER.** `55cf3955` and `037a9c30` do not
say which is more recent — I had it do a job it doesn't know how to do, and it was
dB who asked "what comes after, 955 or c30?". The date comes **first** in the
name so that `ls` comes out chronological.

⚠️ **And the date is the REVISION's, not the run's.** Generating twice has to give
the same file; a name that changes on every run says "it changed" when nothing
changed, and then nobody trusts it.

⚠️ **The digest is the thing you can't forget.** The revision and the date are
bumped by hand, and the hand forgets — that is how two files ended up with the
same name. With the digest in the name, **different geometry is a different file
by construction**.

And there is a guard that closes the loop: the generator stores `revision →
digest` in a record and **refuses to write** if the same revision gives a digest
different from the one that already came out:

```
*** a geometria mudou e o REV nao.
    r3/acrilico_3p0 ja saiu com digest 037a9c30 e agora da 2e2533b3.
```

> Without it, forgetting would produce two geometries with the same revision —
> which is exactly the defect the digest exists to catch.

And it is reproducible: the same input gives the same digest, always. A digest
that changed on every run would be worse than none — it would say "it changed"
when nothing changed.

**What stays in the repository:** the ordered file and the record of which one it
was. A fabrication artifact is not a disposable build: it is what will be needed
to understand the piece that arrived a month from now.

| revision | what it is | order |
|---|---|---|
| r1 | 19 pieces · arcade at 40/100/160 · no reinforcement | **#1263497** |
| r2 | 20 pieces · arcade at 30/100/170 · lid reinforcement | — |

> Generalizes beyond the enclosure: **any artifact that leaves the repository for
> the physical world** — gerbers, CPL, cut file, STL — carries revision and
> digest in the name. What lives only on disk can be called whatever it wants;
> what someone is going to fabricate cannot.

## 2. The five gates — none is optional

Each one refuses **before** sheet or board is spent. A gate that warns and lets
through is not a gate.

| # | gate | where | what it blocks |
|---|---|---|---|
| 1 | **nesting** | in the generator + `tools/nesting.py` | piece off the sheet, overlapping pieces, **missing piece** |
| 2 | **mirror** | `_rot90` in the generator | a piece that is actually mirrored rather than rotated — cuts fine, assembles backwards |
| 3 | **margin to the sheet** | in the generator | drawing glued to the edge: refused on upload |
| 4 | **the board fits** | `confere_caixa` in `pcb_contorno.py` | board larger than the sheet that holds it |
| 5 | **hole clear** | `confere_furos` in `pcb_contorno.py` | a fastening screw under a component |

```bash
python3 tools/nesting.py caixa/cotuca_caixa_r2_acrilico_3p0_55cf3955.svg --pecas=29
```

⚠️ **`--pecas` is not decoration.** A piece that was not emitted does not collide
with anything: a file with 18 passes exactly like one with 19.

⚠️ **A homemade gate asks for external proof.** `nesting.py` checks what the
generator wrote, but it was not written by the generator — it is the second
opinion. Where there is a third-party tool that can **say no** (Ponoko's viewer,
KiCad's DRC, JLCPCB's previewer), you pass through it.

---

## 3. The board reads the enclosure — and then stops depending on it

```bash
python3 kicad/pcb_contorno.py cerebro
python3 kicad/pcb_contorno.py painel
```

**Import > measure.** `cotuca_caixa_corte.py` has `if __name__ == "__main__"`, so
it imports without running anything. Importing has no parsing between the
dimension and its use, so **there is nowhere to diverge**. Only if there is no
generator do you measure the SVG — and then with an element-count guard (an
incomplete read is not a read).

### 3a. The step that closes the matter: the invariant

Following the dimension by hand always loses — it lost four times in two hours.
What closes it is **the dimension dropping out of the calculation**: the board
**centers itself on the bottom sheet**, and from that comes

```
furo_x  = 25.5 − T − 0.5             (the generator subtracts the wall)
x0      = ((W − 2T − 1) − larg) / 2  (board centered)
furo_x − x0  →  constant, for ANY T
```

Confirmed running the generator at `T=3` and `T=6`: the holes always fall at
**(15.5 · 128.5) × (3.5 · 96.5)** from the board corner, and the knob axes always
at **105/140/175 × 92** of the face. `pcb_contorno.py` **refuses to write** if
that stops holding.

> **When an external number won't stop changing, the fix is not to follow it — it
> is to build the piece so that it stops entering the calculation.**

---

## 4. The geometry traps already paid for

**The origin of a footprint is not the axis.** In `EC11E-Switch` the origin is on
the pin row and the shaft at (7.50 · 2.50); in `PTV09A-1` at (7.00 · −2.50).
Putting the origin on the lid hole puts the **pins** there and leaves the shaft 7
mm to the side — the lid closes and the button does not go in. You read the circle
in `F.Fab`, you do not write it.

**The sign of `y` lives in one place only.** In the enclosure drawing it grows
upward; in KiCad downward. The conversion lives entirely in `da_tampa()`.
Repeated in each call, it prints everything upside down — it already cost nine
notes at once.

**The face datum ≠ the cut edge.** Measuring a hole to the edge of the piece
gives a true and useless number: the joint fingers make the edge not be the face.
A whole "deviation of 0.137 mm" was born from that.

**A dimension written by hand next to a calculated dimension always ages.**
`INTERIOR = (188, 108)` was declared and never read by any line. By chance it was
right — which is worse: a dimension nobody consults cannot be wrong out loud, and
so it also cannot be right *in a verifiable way*.
→ **When reviewing a generator, `grep` each constant by its own name. Zero uses is
a finding, even with the right value.**

**A comment with a revoked premise re-teaches the error.** "The enclosure only has
28 mm free at the front" justified two placement decisions; the enclosure came to
have 49. The decisions were still right for *another* reason — and the comment,
which has authority, taught the dead reason.

---

### 4b. The default value of a fabrication parameter

When a generator is **imported** by another tool, its default value stops being a
command-line convenience and becomes **the importer's production configuration**.

`pcb_contorno.py` imports the enclosure generator without passing arguments. With
the default `T` at 6.0 and the ordered enclosure at 3.0, the gates checked the
boards against the wrong enclosure throughout the whole work — and said ✓.

> **The default is the current object, never the last one you drew.** A gate that
> measures the wrong object is worse than none: both let the defect through, but
> only one does it with a checkmark next to it.

→ When reviewing, for each default value: **who consumes it without passing it?**

### 4c. Delete the superseded source — and only it

A superseded source kept "as a record" next to the current one measures just as
well. Git history is the archive; the folder is the present.

But **`diff` before deleting**: the file that looks like a forgotten copy might be
the new version. And **read before deleting**: what had value and was not a
dimension (the cable-routing rules) moved house before the page died.

## 4d. Thin thickness: the foot, the hole, and the local reinforcement

A foot is **~300 N** at mid-span between the front wall and the rib. In 3 mm of
acrylic that flexes visibly and, over time, **cracks from the ø30 holes** — the
hole is the stress-concentration point, and acrylic is notch-sensitive.

> **Cutting one and stepping on it tells you if it breaks on the first day. It
> does not tell you if it breaks at the hundredth.** For a piece replicated in
> series, the destructive test of one unit is not evidence about the series.

The answer that does not force thickening everything: **local reinforcement**. A
second sheet of the same thickness, glued underneath, **only on the band of the
holes that take load** — 6 mm effective where the foot steps, 3 mm where the axes
pass through.

The thin thickness was not an aesthetic choice: a **PTV09 has ~7 mm of thread**,
and 6 mm of lid plus the board underneath leave no room for a nut. The local
reinforcement respects that reason instead of canceling it.

⚠️ **But thickening the panel changes the assembly of everything that passes
through it in that band.** It is the inverted-gland trap: with a hole equal to the
button's, the arcade now mounts in 6 mm — and the arcades **are already bought**,
with a thread length nobody measured. That is why the reinforcement hole diameter
**has no default**: the generator refuses to run without it, and says which
measurements it asks for.

| reinforcement hole | the button mounts in | needs |
|---|---|---|
| = ø of the button | **6 mm** — the strong one | thread ≥ 6 mm |
| = ø of the nut + clearance | 3 mm, nut in the pocket | nut fitting |

And the band **follows the hole**: it grows until it leaves a minimum ligament
(8 mm) between the hole and the edge. A thin ligament is exactly where the crack
starts — it is the defect the reinforcement exists to avoid, reintroduced by its
own geometry.

⚠️ **To confirm:** a laser-cut acrylic edge is left with residual stress, and
solvent on a stressed edge is known to accelerate *crazing*. If that is so,
annealing before gluing stops being optional. **I did not confirm it in a
source** — it is a hypothesis, and it is stated as a hypothesis.

## 4e. Transparent material: the interior becomes the face

A consequence that is not geometry and is decided **early**, not at the end: with
a transparent shell, harnesses, service slack, and the IDC cable are in view.
**Either they are arranged with intent — routes, fasteners, wire color — or the
art object shows the workbench.**

`CAIXA-ACRILICO-MONTAGEM.md` §6b already fixes the routes for electrical reasons.
With transparent material, those routes now also have to look good, and the two
requirements do not always coincide.

## 4f. Assembly lessons (field) + screwed direction

Assembling the r2 enclosure by hand taught five things no gate catches, because
they only appear with the **real piece on the bench**. They are complete —
symptom, cause with `file:line`, fix, recut, hardware — in
**`caixa/LICOES-MONTAGEM.md`**. Read that doc **before opening the next enclosure
rev**.

> **Directive that governs the next version: EVERYTHING SCREWED — glue only if
> absolutely necessary.** The enclosure becomes **100% disassemblable and
> reworkable**. Where Weld-On used to be the means of assembly, there is now a
> screw + captive nut (the same machinery as the `hexagono()` / `"hex"` feature of
> the standoffs, `cotuca_caixa_corte.py:366-368,521`). The chosen shell is
> **internal screwed posts** at the corners — clones of the bottom posts —
> because they require **much less generator rewriting** than the T-slot (which
> stays as a future aesthetic upgrade; the 2 lid joints at 11.8° complicate it
> *in-plane*).

⚠️ **The pot's indexing-pin hole DOES NOT EXIST in the generator.**
`cotuca_caixa_corte.py:264` makes only the **central ø7** of each pot (encoder
x=105, pots x=140/175, y=92) — the **anti-rotation pin** hole (*locating hole*) is
missing, without which the pot does not seat flush. When mounting the pot in the
enclosure's guide, **this hole enters the list** (**ø3.2 mm**, measured on the
piece), alongside the ø7.

⚠️ **The axis→pin distance is LOCKED: 7.5 mm center-to-center** — measured directly
on the piece by dB (§0: **the piece in hand rules**, supersedes the datasheet).
This 7.5 mm is the **anti-rotation latch on the PANEL side** (the *tab* that
enters the enclosure LID), a feature **distinct** from the ~4.4 mm of the posts on
the PCB side (which enter the panel, footprint
`Potentiometer_Bourns_PTV09A-1`) — two different sets of features, no conflict
between enclosure and board. Confirm on the piece whether there are **1 or 2**
holes (the PTV09A has a latch). **Position:** the pin hole is **to the left of the
axis in the top view of the panel** (**−X, same Y**) — map "left in top view" via
the lid transform (`to_top()`/*mirror*) and **confirm the sign before
generating**, so it does not come out mirrored. See `caixa/LICOES-MONTAGEM.md`,
lesson #1.

## 5. Work order, and where dB comes in

```
1. confirm MATERIAL and thickness with dB             ← not inferred from the name
2. regenerate the cut with --t                        ← gates 1, 2, 3
3. run pcb_contorno for both boards                   ← gates 4, 5
4. check what CHANGED in the assembly:
     · standoff height  -> spacer height
     · usable height    -> what fits at the front
     · what opens the enclosure (lid glued? bottom?)
5. render the boards, and dB's VETO
6. route · revisao-terra · gerbers                    ← skill kicad
7. cut ONE enclosure, dry-fit, check the holes with the pieces in hand
8. only then the series
```

⚠️ **Step 7 is not skipped.** No gate in this skill measures real kerf, acrylic
tolerance, nor whether the musician's foot cracks a 3 mm lid. That is answered
with the piece in hand — and it is the piece in hand that rules everything (§0).

⚠️ **A divergence announced without measuring blames the wrong piece half the
time.** The enclosure had three arcades and the drawing two; the obvious
conclusion was that the schematic was behind. The schematic already had three.
**Measure which side the shortfall is on, before telling anyone to change
anything.**

---

## 6. COST: measured on two real invoices

**The price follows the CUT LENGTH. That's it.** Two Ponoko orders, same material
(Clear Acrylic 3.00 mm):

| order | bounding box | area | cut | price |
|---|---|---|---|---|
| #1263434 | 346.0 × 372.6 | 1289 cm² | 6.39 m | **$37.56** |
| #1263497 | 372.0 × 296.5 | 1103 cm² | 6.07 m | **$35.66** |

| prediction from the first | error |
|---|---|
| by **area** | **−$3.52** |
| by **cut length** | **−$0.02** |

$/m: **5.878** and **5.875** — they differ by **0.1%**. $/cm²: they differ by 11%.

### ⚠️ And then a third point arrived, which disproved the precision

| order | cut | price | $/m |
|---|---|---|---|
| #1263434 | 6.39 m | $37.56 | 5.880 |
| #1263497 | 6.07 m | $35.66 | 5.874 |
| **r2, 09/06** | **6.84 m** | **$38.23** | **5.589** |

The model fitted on the first two predicted **$40.19**; the invoice says
**$38.23**. An error of **$1.96 — 5%**.

> **Two points always fit a straight line, with zero error.** "Predicts with $0.02
> of error" was not a result: it was arithmetic. With the third point, the best
> single-variable model errs **$1.23 (3%)** in the worst case — and several
> two-variable models (cut+area, cut+pieces, area+pieces) fit all three, which
> with three points and two parameters is guaranteed and proves nothing.

**What stands:** `Setup $0.00`, no separate material line, **no sheet price** —
P1/P2/P3 does not enter the calculation, it only matters by fitting. And **cut
length is the best single predictor, to ±3%.** It is enough to decide between
drawings; it is not enough to promise cents.

### What this erases from what I had written here

⚠️ **The tight nesting saved $0.00.** Nesting moves the pieces, and the cut is a
property of the **pieces** — the same 6.07 m before and after. The $1.90 between
the two invoices came entirely from **regenerating at 3.0 mm instead of 6.4**,
which changes the finger geometry and shortens the cut from 6.39 to 6.07 m.

⚠️ **And the "sheet step" that I used to put as lever #1 does not exist.** There is
no sheet charge. A smaller enclosure is cheaper because it has less perimeter, not
because it drops into a P1.

You nest it anyway — it is automatic and removes the risk of upload rejection —
but **it is not a saving and is not sold as one.**

### Where the money is, at $5.87/m

| | meters | cost |
|---|---|---|
| if the pieces were smooth rectangles | 4.07 | **$23.89** |
| fingers, holes, and slots | 2.00 | **$11.74** |
| — of these, **the 12 standoffs** | 0.70 | **$4.11** |
| the tight nesting | 0.00 | **$0.00** |

**Design to save, in order of effect:**

1. **Fewer pieces, and fewer small pieces.** The cost follows the **perimeter**,
   and a small piece is extremely expensive per square millimeter: twelve 12 mm
   squares cost $4.11. Six standoffs of double-thickness material, or bought
   brackets, remove more than everything else combined.
2. **Fewer and wider fingers**, for the same joint — it is the `n_target` of
   `cells()`, a parameter that already exists.
3. **Smaller enclosure**: less perimeter, linearly. No steps.
4. **Engraving off** on upload — it is time, like the cut.
5. **Holes and slots**: each ø30 is 94 mm of cut ($0.55). The five bottom
   ventilation slots, 390 mm ($2.29). They are worth what they are worth — but you
   count them.

### Shipping, which is the biggest of all

**$22.43 of shipping on $35.66 of piece — 39% of the subtotal.** For the series,
shipping one enclosure and shipping several in the same package are not very
different: **doubling the order does not double the cost per enclosure.** It is the
only lever worth more than all the ones above combined, and you do not touch the
drawing to pull it.

### What remains to be confirmed

- **Minimum setback from the sheet edge.** Ponoko does not publish a number; the 3
  mm the generator requires are **ours**. The two orders passed, which only proves
  that 3 mm is enough.
- Whether $/m changes with **thickness** or **material** — the two invoices are of
  the same, so they say nothing about this. Cutting 6 mm is slower than 3.
- They are **two points**. Cut length explains them to 0.1%; a third would
  confirm.

## Provenance

Born on 09/06/2026 from scar **15cf** (`docs/battle_scars.md`): the two PCBs had
been sized against drawings of an enclosure that was never cut, and the two
following corrections went to real but wrong enclosures. See also
`caixa/README.md`, which puts the four enclosures side by side.

## History

- **v1.2.2 (2026-09-19)** — repoints the **M track** reference again: `ifa10000-m.md`
  migrated to the new sub-skill `/3ddesign`
  (`.claude/skills/3ddesign/references/ifa10000-m.md`), orchestrated by `/aipcb`.
  Only the reference changes.
- **v1.2.1 (2026-09-19)** — repoints the **M track** reference of IFÁ10000: the
  orchestrator `/pcb` was merged into `/aipcb` (canonical), and `ifa10000-m.md`
  moved to `.claude/skills/aipcb/references/`. Only the reference changes.
- **v1.2.0 (2026-09-18)** — **who gives way becomes the enclosure**, in priority
  order (IFÁ10000-M, dB's request): the board brings its own means of fastening;
  with that, the enclosure is generated from the board; the board only gives way
  with a signed justification. New section at the top. Born from panel rev C,
  fabricated without fastening.
- **v1.1.1 (2026-09-16)** — §4f: the pot's indexing-pin hole **fully locked —
  ø3.2 mm · 7.5 mm center-to-center · to the left of the axis in the top view
  (−X, same Y)**, all measured on the piece by dB (supersedes the datasheet),
  replacing the uncertainty datasheet ~4.4 mm ≠ 4.7 mm-from-the-edge ~8.2 mm.
  Clarifies that the 7.5 mm is the **anti-rotation latch on the PANEL side** (tab
  on the LID) — a feature distinct from the ~4.4 mm of the posts on the PCB side
  (footprint `Potentiometer_Bourns_PTV09A-1`), no enclosure↔board conflict. The
  angular position is now known: the pin hole is **to the left of the axis in the
  top view** (−X, same Y) — map via the lid transform and confirm the sign before
  generating. "The piece in hand rules".
- **v1.1.0 (2026-09-16)** — new section **4f. Assembly lessons (field) + screwed
  direction**: declares the directive **"EVERYTHING SCREWED — glue only if
  absolutely necessary"** as the governor of the next enclosure rev, points to
  `caixa/LICOES-MONTAGEM.md` (the 5 lessons from the hand assembly of r2), adds to
  the pot-mounting guidance the **missing indexing-pin hole**
  (`cotuca_caixa_corte.py:264` makes only the ø7) and reaffirms **"the piece in
  hand rules"** for the axis→pin distance (PTV09A datasheet ~4.4 mm ≠ 4.7 mm
  measured from the edge). Chosen shell = internal screwed posts (clones of the
  standoffs, reusing `hexagono()`/`"hex"`), T-slot as a future upgrade.
- **v1.0.0 (2026-09-06)** — initial version (see Provenance).
