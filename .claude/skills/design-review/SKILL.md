---
name: design-review
description: Mandatory exit gate for any visual artifact — SVG, schematic, build sheet, PCB render, slide, mockup, plot. Use ALWAYS when you produce or edit a visual, before presenting it. Step 1 is the tool `tools/svg_colisoes.py`, which can say no; only then does the eye come in. It catches text over text, text over shape, elements running off the page, labels over wires, and the stale-view trap (editing after the last look). Triggers on any delivery of .svg/.pdf/.png with layout, on "design review", "confere o visual" ("check the visual"), or on dB reporting overlap — but the goal is to run WITHOUT being asked.
---

# design-review

> **This is the Cotucatronics version, and it lives in the repo on purpose.** It
> absorbed the plugin's skill of the same name (which lives in a session cache
> and does not survive) and the **amendment of 09/05**, which added step 1. One
> skill per subject: if the plugin's reappears, this one rules, because it is the
> one with the gate.

A visual artifact delivered without inspection is an **unverified** artifact —
the graphic version of the instrument that does not witness itself.

## 0 · The iron rule

> **No visual goes out without the FINAL RENDER having been SEEN — and any edit
> invalidates every prior inspection.**

The second term is the one that bites most: "I fixed it and exported" is not "I
fixed it and checked". Edited → rendered → **looked again**. No exception,
including for "trivial one-line changes" — it was a one-line change (moving a
note) that crossed a component.

---

## 1 · THE GATE: `tools/svg_colisoes.py`, and it comes BEFORE the eye

**Amendment of 09/05.** Four deliveries with "clean visual review" had overlaps.
Two were visible and passed because the eye looked at the whole and not the
slice; **two were half-a-pixel** — text touching the stroke of a cross, a label
crossing a gray dashed line — invisible in any render, and a defect all the same.

> **The eye is the wrong instrument for fine overlap.** It serves composition,
> legibility, hierarchy. It does not serve "does this text touch this line?",
> which is a question of geometry and has an exact answer.

```bash
python3 tools/svg_colisoes.py ficheiro.svg
```

**Zero collisions is the gate** — and it knows how to say no, which is what makes
it a gate: a script that calls it stops.

⚠️ **There are THREE outcomes, not two, and the third is not a failure:**

| exit | means |
|---|---|
| **0** | verified, zero collisions — **passes** |
| **1** | verified, there are collisions — **defect in the drawing** |
| **3** | **NOT VERIFIABLE** — I couldn't look. Neither approval nor rejection |

Reading the 3 as "there are collisions" is the easy mistake, because both are
≠ 0 and they mean the opposite of each other. The 3 comes out when there is no
`<text>` at all — the letters may be paths — and then *"zero collisions" would be
a true sentence about nothing*. **Verification without an object is the class the
`audit` skill chases, and this tool refuses to commit it.**

What it tests:

| | |
|---|---|
| **text × text** | always a defect |
| **text × segment** | real geometry (Liang–Barsky), not the bbox of a path — an L-shaped path does not flag the text sitting in its empty corner |
| **text × shape edge** | text **inside** a rect/circle is a label and is legitimate; text **crossing** the edge is a defect |
| shapes with `opacity < 0.5` | decorative shading, don't count |

⚠️ **The text width is estimated** (0.55 em per character, Helvetica), and is
**deliberately conservative** — it flags a few pixels before the real touch. It
is the right behavior for a gate: *a false positive costs moving a label; a false
negative costs a wrong delivery.*

### Each flag says HOW MUCH, and that changes what you do with it

As of v0.5.0, each line carries the overlap depth on the shallowest axis, in
sheet units:

```
!! texto×texto  V- 4              42 um     <- model noise
!! texto×borda  rect 54,252    1.65      <- real, go look
```

⚠️ **It was born from my having reported it wrong.** The gate flagged a `V-`/`4`
pair and I told dB it was a real collision. I went to measure: **42 micrometers**
— within the error of the model itself, which assumes an ascender of 0.75 of the
body, and KiCad's stroke font does not have that metric. The symbol was correct
and symmetric; there was nothing to straighten.

> **A binary verdict forces whoever reads it to go calculate by hand.** Measuring
> does not lower the bar — the gate still flags — it gives context to whoever
> decides. Below ~0.1 of the sheet unit, suspect the model before suspecting the
> drawing.

And the opposite also holds: on the same sheet there were **five** flags from 0.9
to 1.65 that I had not distinguished from the noise. Quantifying separated them.

**The result line goes in the report**, always: `N texts, M segments, 0
collisions`. **Without that line, the review did not happen.**

### And on a BOARD: `--layers <F|B>.Silkscreen`, and the DRC alongside

The silkscreen of a `.kicad_pcb` is exported per layer and passes through the
same gate:

```bash
kicad-cli pcb export svg --output /tmp/s.svg --layers F.Silkscreen \
    --page-size-mode 2 --exclude-drawing-sheet placa.kicad_pcb
python3 tools/svg_colisoes.py /tmp/s.svg
```

⚠️ **On a board the same finding has a different DEGREE**, and the table of
degrees lives in the `kicad_layout` skill **§11g-bis** — not here, so the two do
not diverge. The summary to use when reading the output: `text × text` is always
a defect; `text × outline of ANOTHER part` is a defect; `text × outline of the
PART ITSELF` is a **warning**, avoided when there is room and shipped when there
isn't.

⚠️ **I already wrote here that it was "the normal case" while `kicad` said "on the
board it's a defect"** — two skills, two assertive sentences, neither flagged. It
took going to measure to see that neither of the two was right on its own: the
degree was missing. **When two house skills assert opposite things, the answer is
not to choose — it is the distinction both of them lack.**

Classifying by "are they the same reference?" separates the cases: of the brain
board's 9 `silk_overlap`, 9 were the part over itself.

⚠️ **And you run KiCad's DRC alongside, because on a board it is the one that
rules.** `silk_overlap`, `silk_over_copper`, `silk_edge_clearance`. The two
answered differently — `0 text×text` against `9 silk_overlap` — and it was going to
measure the disagreement that uncovered four defects in the tool that arranges the
silkscreen. **Two checkers that disagree are information; choosing the one you
like is throwing it away.**

⚠️ **The before/after comparison is measured in the SAME context.** KiCad finds
the `.kicad_pro` by the file name: a copy called `_teste_x.kicad_pcb` falls into
the default rules and invents violations the board does not have. Either the copy
lives in the project folder with the matching name, or you measure the before and
the after in the same place — and then you read the **difference**, never the
absolute value.

### What to do when the tool doesn't reach

A plot PNG, a PDF, a KiCad render: it does not read them. There the old rule holds
— slice into regions and look at each one — **plus** the lesson of 09/05: *looking
at the whole does not replace looking at the slice, because the whole hides
exactly what is small.*

### KiCad has a path, but with `--esquematico`

```bash
kicad-cli sch export svg --output /tmp/x  ficheiro.kicad_sch
python3 tools/svg_colisoes.py /tmp/x/ficheiro.svg --esquematico
```

⚠️ **Without the flag, a clean schematic flags dozens — and all of them are
convention.** Measured on 09/05 on the three sheets of revB:

| sheet | without the mode | with `--esquematico` |
|---|---|---|
| `cotuca_energia` | 39 | **6** |
| `cotuca_preamp` | 74 | **7** |
| `cotuca_soquete` | 116 | **26** |

The reason is one of design, not tool: **in a schematic, text touching a line is
the convention.** A net label *attaches* to the wire — it is what a label is; a
pin number *sits* on the leg. Of the pre-amp's 74, 67 were `AGND` (13×), `VBIAS`,
`V+PRE`, `PWR_FLAG`, and pins 1–8.

> **Flagging the convention is worse than flagging nothing:** it teaches you to
> ignore the gate, and it is the anti-pattern of §6 of this same skill.

The mode keeps `text × text` and `text × edge`, which are still a defect in a
schematic — that is how a `V-` on top of pin `4` showed up in the pre-amp, which
three eye reviews had not seen.

### ⚠️ There are TWO gates for a schematic, and they are not the same

| | reads | sees |
|---|---|---|
| `tools/colisoes_sch.py` | the **source** `.kicad_sch` | exact coordinates in mm, before they become curves |
| `tools/svg_colisoes.py --esquematico` | the exported **render** | what is actually drawn on the sheet |

**Run both, and the disagreement is information.** In the pre-amp the source one
says **117 texts, 0 collisions**; the render one says **194, 7** — among them a
`V-` on top of pin `4`.

**The mechanism is measured (09/06), and the blind one is the source's.** It
skips the `lib_symbols` section on purpose — it is in its docstring:
*"everything that prints letters, **outside lib_symbols**"*. It reads `label`,
`global_label`, `hierarchical_label` and the `Reference`/`Value` properties. So
**it does not see pin names or numbers**, which is where the symbols keep the
letters. And it does not see the **title block and the frame** — `Date:`, `Rev:`,
`File:`, `Id: 1/1`, the edge coordinates — because KiCad draws those from the
*worksheet* and not from the `.kicad_sch`: 27 of the render's texts do not exist in
the file.

That is exactly why only the render saw the `V- 4`: `V-` is a **pin name** and `4`
is a **pin number**, and `(number "4")` appears **1 time inside `lib_symbols` and 0
outside**. The two sides of the collision live in the section the other one skips.

> **Neither of the two replaces the other, and now we know why:** the source one
> has the exact coordinates but does not see what the symbol draws; the render one
> sees everything on the sheet but measures over the drawing. A `0` from either
> one **does not authorize dispensing with the other**.

> Two checkers that answer the same question and disagree: the correct thing is
> not to choose the one you like, it is to record the disagreement and go measure
> why.

⚠️ And the header of `colisoes_sch.py` justifies itself with a premise that was
measured **false** today — it says `svg_colisoes` always returns code 3 on KiCad
exports *"because there isn't a single `<text>` to measure"*. There are **222**,
invisible (`opacity="0"`) but with `textLength`. The tool is still useful; the
written reason for it to exist is stale.

The electrical gate is the `kicad` skill; this is the visual one, and neither
replaces the other.

### ⚠️ And on the BOARD the silkscreen has its own gate, which lives in `kicad`

A PCB render is not reviewed by eye to hunt overlapping text: `kicad-cli pcb drc`
**counts** — `silk_overlap`, `silk_over_copper`, `silk_edge_clearance` — and
`kicad/arruma_silk.py` arranges it. The full rule is in the **`kicad` §11g-bis**
of `kicad_layout`, and is not repeated here: **two skills saying almost the same
thing diverge, and then nobody knows which one rules.**

What stays on this side is the half that counting does not answer:

| question | who |
|---|---|
| is there text over text, over copper, or off the board? | `kicad` — counts |
| does the text **read** at the size it will be printed? | this skill — looks at the render |
| does the reference point to the **right** part? | only the eye |

⚠️ **A count of zero does not authorize dispensing with the eye** — it is the same
corollary as §9b of `kicad`, and here it holds the same: it authorizes looking
knowing where.

⚠️ And there is a trap specific to the PCB: **arranging the silkscreen pushes the
text toward the obstacle that is not yet being checked.** Measured — moving away
from the pads took the overlaps from 18 to 5 and the text-over-copper from 6 to
12. If you see one count improve and another worsen in the same pass, it is not
progress: it is the defect changing place.

---

## 2 · The order, fixed

```
generate → svg_colisoes (0) → slice and look (composition) → deliver
```

**Any edit after any of the three restarts the three.**

---

## 3 · The eye loop

1. **Render in the delivery format** (SVG→PNG; `.kicad_sch`→SVG via `kicad-cli`;
   HTML→screenshot). Inspecting the source does not count: the overlap only exists
   in the render.
2. **View in slices.** A tall image (>1200 px) is cut into 2–4 strips with **~8%
   overlap** between them — a defect at the seam of the cuts escapes cuts without
   overlap — and each strip passes by the eye.
3. **Sweep with the closed list** (below). It is not "look if it's good": it is to
   search for each named class, one by one.
4. **Defect found → fix → BACK TO STEP 1**, the gate's, not the eye's.
5. Only then package. Packaging before checking generates the "zip with a known
   defect inside".

## 4 · The closed list

After the gate, the eye no longer searches for touch. It searches for this:

- **Composition and hierarchy** — does what jumps out first is what matters first?
- **Element × edge**: nothing cut by the page/viewBox margin; long text on the
  right is suspect #1.
- **Wire × wire / wire × shape** where the crossing suggests a false connection.
  ⚠️ See the `kicad` skill §4: the **eye cannot arbitrate** a connection — only the
  netlist.
- **Seams**: a wire that LOOKS continuous but has a gap (the clamp's D2 had 17 px
  of invisible gap in the source, visible in the render).
- **Legibility at the final size**: a font < ~9 px in the delivery render is
  illegible on mobile; check at the width dB will see.
- **Layer overlap** (z-order): an element drawn later covering information from
  before.
- **Label-drawing consistency**: the text says "12 buckets", the drawing has 13.

## 5 · How this becomes automatic

1. **In the generators:** the `.py` that writes the SVG calls the checker before
   copying to the delivery; with collisions, it writes to `/tmp` and **refuses to
   deliver**. An `assert` at the end of the generator.
2. **In the skill:** step 1 is this, and the result line goes in the report.
3. **In what is not SVG:** slice and look, declaring that the tool did not reach.

## 6 · Named anti-patterns, the ones that have already happened

- **Exporting after editing without re-viewing** — the case of the note over C3.
- **Checking only the edited region** — the fix pushes the neighbor.
- **Trusting the source** ("the coordinates look right") — overlap is a property
  of the render, not of the source.
- **Cut without overlap** — the defect lives at the seam.
- **"It's just text, it doesn't need it"** — text is the element that collides
  most.
- **⚠️ "The visual review passed"** — without the gate line, it did not pass. That
  was the pattern the 09/05 amendment came to close, and it had three recurrences.

## 7 · Relationship with the others

| question | who answers |
|---|---|
| is there overlap? | **this skill, step 1** — geometry, not the eye |
| does the drawing read? | **this skill, step 3** |
| does the file open? do the connections exist? | skill **`kicad`** — ERC and netlist |
| is the code consistent? | skill **`audit`** |

The three share the rule: **what passed is declared.** An undeclared inspection
will be redone or, worse, presumed.
