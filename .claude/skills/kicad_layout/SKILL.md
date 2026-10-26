---
name: kicad_layout
description: PCB layout and routing in KiCad — planes and copper, what no DRC sees, measuring per net, silkscreen that reads and designators that point to the right part, and routing by script without trusting the router. Use when placing parts, routing copper, tidying board text, or preparing the factory package. It is MANDATORY at the close of any board, called by the `kicad` skill.
---

# /kicad_layout — the schematic is done, and another machine begins

> **This skill split off from `kicad` on 08/09/2026**, when the layout chapter
> grew to **more than half** of it (1033 of 2022 lines). A skill you don't read
> before using is not a skill — it's a file. `kicad` still rules the
> **schematic**; this one rules the **board**, and the two meet at the close.

## ⚠️ THE NUMBERING IS THE ORIGINAL, ON PURPOSE

The sections keep the numbers they had in `kicad` — **§11**, **§11g-bis**,
**§12r**, and so on. They were not renumbered.

> **Renumbering would have broken every pointer at once**, and there are dozens:
> in code comments (`arruma_silk` cites §11c, `refs_apontam` cites §11g-ter),
> in the scars, in `design-review`. A number that changes value is worse than an
> ugly number — it's class 13 of the `audit` skill, the sentence broken by a
> rename, applied to cross-references.

The jump from §10 to §11 in this skill is not a mistake: it's the mark of where
it was cut.

## THE BOUNDARY, AND IT IS REAL

| question | who answers |
|---|---|
| does the file **open**? do the connections **exist**? | **`kicad`** skill — ERC and netlist |
| does the drawing **read**? | **`design-review`** skill |
| does the copper match the netlist? does the silkscreen read? does the package come out? | **this one** |

⚠️ **None replaces the others, and the order is non-negotiable:** the schematic
has to be proven before the copper is worth anything. Routing over a wrong
netlist produces a perfect board of the wrong circuit.

⚠️ **And what this skill CANNOT see:** whether the board does what the schematic
intended. It proves that the copper matches the netlist, not that the netlist is
right.

## 11. LAYOUT: the schematic ends, and a machine with other rules begins

Everything above is schematic. From here on it's copper, and copper punishes
other things. These rules are not specific to this instrument — they belong to
**any** two-sided board with a ground plane, and that's why they live in a skill
and not in a README.

### 11.0 F8 matches by UUID, and a generator swaps every UUID

`Update PCB from Schematic` **does not match by reference** — it matches by UUID.
A script-generated schematic changes every UUID on each generation, so F8
recognizes nothing and **adds a second copy of each part next to the first**. The
symptom is not an error: it's the board turning confusing, with the Daisy twice
over and the rest yet to be discovered.

> **If the schematic is generated, F8 is useless, and the fix is not deleting
> duplicates by hand.** It's a synchronizer that matches by **reference**,
> removes the extras, adds the missing ones, and writes the nets onto the pads
> (`sincroniza_pcb.py`). F8 stops being a step of the process.

Two traps inside the synchronizer, both paid for:

- **The system library counts too.** The first synchronizer only knew the
  project's `.pretty`, and I emptied both boards with `--zero` **before proving
  it could rebuild**. Prove the way back before using the way there.
- **The `.kicad_mod` brings its own `Reference` `REF**`.** If you add yours,
  KiCad keeps the last one and the board shows `REF**`. Remove the module's.

### 11a. Copper on both layers leaves no layer for signal

⚠️ **The structural rule that replaces half a session of clever work.**

A two-sided board has two layers. If you put a ground plane on F.Cu **and** on
B.Cu, no layer is left for signal — and then every trace crosses a plane.
**Crossing a plane is not passing over it. It's cutting it:** the copper pulls
back from both sides of the trace and the plane ends up in pieces.

The right arrangement, and it's the industry norm:

```
B.Cu   solid ground plane, whole
F.Cu   signal, and no zone
       + one via in each SMD ground pad, dropping to the plane
```

> **When a router seems to need to get smarter, distrust the drawing before you
> distrust the router.** The question was "how do I teach the router to see
> zones"; the answer was "take the plane out of its way." Twenty lines of
> geometry against half a session of heuristics, and the second would never have
> become reliable.

Cost: **19 vias on the brain board, 8 on the panel board.** It's the normal
price, and you pay it once.

### 11b. Adjacent copper is not connected copper

I put the vias **next to** the pads and connected them to nothing. The DRC
returned `via_dangling` on each of the 19. **0,9 mm of void isolates as well as
9 mm.**

Each via carries a **stub** to the pad, on the pad's layer. And the via goes to
the side, never on top of the pad: **a via in the pad wicks solder by
capillarity** and leaves the joint starved — and on the SMD side it's the factory
that solders.

### 11c. Every spacing test looks at pads **AND** traces

The first via collision test looked only at pads, and shoved two vias 0,10 and
0,15 mm from traces that the person had already routed by hand. **What's already
there is an obstacle, no matter where it came from.**

### 11d. `--refill-zones`, or the DRC compares against an old fill

```bash
kicad-cli pcb drc --refill-zones --save-board -o r.rpt placa.kicad_pcb
```

Without `--refill-zones` the DRC compares the new traces against the zones'
**previous fill** and returns dozens of phantom `clearance` errors. You'll go
hunting defects that don't exist.

> **And before you say a tool doesn't do X, run `--help`.** I wrote in a commit
> that `kicad-cli` couldn't fill zones. It could, and the flag was named exactly
> that. It was the third claim of its kind that same day.

### 11e. MEASURE PER NET — the total lies by default

⚠️ **The rule that catches the regression disguised as progress.**

A routing run brought the total of unrouted connections down from **71 to 52**
and at the same time broke the AGND island into eight pieces. The total improved.
The board got worse. And the DRC reported zero geometry the whole time, because
from its point of view nothing was wrong.

What you measure, always, and never just the first:

| measure | what it catches |
|---|---|
| unrouted, **per net** | the regression hidden behind a better total |
| `Zone [X] <-> Zone [X]` | the plane cut into pieces |
| `starved_thermal` | pad clinging to a narrow island (copper width, not a cut plane) |
| `via_dangling` | via that connects to nothing on either side |
| geometry (`clearance`, `short`) | what any factory rejects |

⚠️ **And this table is the weak version of the rule.** A list of symptoms
protects against what has already happened; against what hasn't happened yet it
protects nothing — and the next symptom showed up two hours later (§12a). The
strong form is to compare the **profile** of unrouted connections, net by net,
before and after, with a single acceptance rule: **no net may get worse**. Use
the table to read a report; use the invariant to let a tool write copper.

⚠️ **And the net name is NOT on the finding's line.** The `unconnected_items`
header says only `Missing connection between items`; the net comes in the **next
two item lines**. An extractor that matches the header returns **zero findings
with the look of a clean board** against a DRC that says 47. **A checker that
lies is worse than no checker** — this one handed me back the clean board and I
almost went with it.

### 11f. What NO DRC sees

The DRC is 2D and measures copper. Three things are left out, and each has
already cost something:

1. **Volume.** The arcade button descends through the third dimension. A tall
   part underneath it passes every test and won't let the enclosure close. It
   needs a table of heights per footprint pattern and a gate of its own.
2. **Access.** The USB-C has to remain reachable with the board assembled, or the
   first reflash opens the whole enclosure. **Plan, volume, and access are three
   gates, not one.**
3. **Empty layer ≠ empty face.** A layer with no objects doesn't prove that face
   is free; it proves nobody drew there.

### 11g. Two things that ruin the silkscreen

- **References on top of the pads.** A synchronizer that places each reference at
  a fixed relative position lands squarely on the pads: `silk_over_copper` 39.
  It's tidied by script, and **proven by the number that does NOT change** —
  connections left to route 104 → 104 says only silkscreen was touched.
- **The logo at the center of the board.** It ended up there by default and sat
  on top of 64 of 130 pads. You look for the **largest free strip** and put it
  there.

### 11g-bis. SILKSCREEN: each obstacle you add pushes the text to the next one

⚠️ **The rule the user asked for, and that took four measurements to get right.**

A tool that moves the references away from the **pads** and nothing else does not
tidy the silkscreen: it just moves the defect elsewhere. Measured on the brain
board, step by step:

| obstacles the test knew about | `silk_overlap` | `silk_over_copper` | `silk_edge_clearance` |
|---|---|---|---|
| pads only | 18 | 6 | 1 |
| + silkscreen | **5** | **12** ⬆ | 1 |
| + traces, without exempting the parts' own pads | 5 | **1** | 1 |
| + board outline | 6 | 1 | **0** |

Each row fixes one column and spoils the next, because the text **flees to the
spot that isn't being checked yet**. It only stops when all four are on the list:
**pads, traces, silkscreen, and outline**.

> **An incomplete checker doesn't leave the defect where it was — it pushes it to
> where it isn't seen.** It's worse than having no checker, because the count it
> shows improves.

⚠️ **And the exemption of a part's own pads has to go.** It exists because a
reference over its own pad is the normal case for a small SMD — but as soon as
the silkscreen enters the obstacle list, the references flee the outline and end
up exactly there. Without the exemption, whoever has no room **stays where it is
and says so**, which is better than being pushed to somewhere worse.

⚠️⚠️ **AND HERE THE RULE CHANGES IN DEGREE RELATIVE TO THE SCHEMATIC.** In the
schematic, the reference inside the body of its own symbol is where it lives
(§9b) and comparing one against the other gives a false alarm. On the board it
**counts**: the two things come out in the same white ink and legibility gets
worse.

⚠️ **But degree matters, and this section already said the wrong thing by not
distinguishing it.** Written like that — *"on the board it's a defect"* — it fell
into direct contradiction with `/design-review`, which called the same case *"the
normal case"*. Two skills, two affirmative statements, neither marked as
historical: class 8 of the `audit` skill, across files. **The resolution is not
to pick the one you like.**

| case | degree | who rules |
|---|---|---|
| **text × text** | defect, always | blocks the gate |
| reference × outline of **ANOTHER** part | defect | gets fixed |
| reference × outline of **ITS OWN** part | **warning** | avoided when there's room; shipped when there isn't |
| reference × exposed copper (`silk_over_copper`) | defect | the factory clips it and the text vanishes |

That's why `arruma_silk` treats the part's own outline as an obstacle — to
**avoid** it — and `para_fabrica` **does not reject** because of it. The two
things are consistent, and neither of them is "it's a defect" or "it's normal" on
its own. Six were shipped this way on the brain board and one on the panel board,
with this line justifying them.

**The proof that the tool spoiled nothing is the number that does NOT change:**
traces, vias, pads, and parts identical before and after. Silkscreen only touches
silkscreen.

### 11g-quater. Four ways to measure the text in the wrong place

The `arruma_silk` had **four** defects at once, and the family is a single one:
*the right calculation already existed in the file and wasn't applied where it
should be.* None of them raised an error; all four produced **approval**.

**1 — the reference field ROTATES WITH THE PART.** A property's `(at ...)` is
relative to the footprint. Doing `fx + dx, fy + dy` adds an offset in the
**part's** space to a coordinate in the **board's** space: on parts at 0° it's
right by accident, on rotated ones it isn't. `U2` (180°) was evaluated **7,2 mm
to the side** of where it is — on the other side of the module.

```python
X = fx + dx*cos(a) + dy*sin(a)     # the SAME one the pads already used
Y = fy - dx*sin(a) + dy*cos(a)
```

> ⚠️ **And the failure mode is the worst there is: in the wrong place there was
> nothing.** The tool said *free* and considered the reference tidied. **Whoever
> measures in the wrong place fails nothing — approves everything, and with the
> air of having checked.**

**2 — and the WAY BACK takes the inverse.** The candidates are deltas in the
board's space and the field is written in the part's space. Writing the raw delta
makes KiCad rotate it again when drawing, and the part ends up with the text in a
**third** place. Cheap proof: `campo_do_abs(abs_do_campo(x)) == x`.

**3 — `cos(π)` doesn't give just −1.** It gives `sin(π) = 1,2e-16`, and the
offset comes out `-4,40873e-16`. It's **§2b** in a new place: zero in scientific
notation, which KiCad accepts and which no reader using `(-?[\d.]+)` catches.
Every number that enters a KiCad file passes through the formatter, **including
those that come from a rotation calculation**.

**4 — the silkscreen is made of SEGMENTS, not points.** Keeping three samples per
segment (start, middle, end) leaves gaps proportional to the length: on the
outline of `J1`, at 41 mm, the gaps are 10 mm, and the reference of `C5` ended up
**0,11 mm** from the line without anyone noticing. The correct test —
point-to-segment — was already in the file, in the traces. **§11c says to look at
pads and traces; the silkscreen was left out of the rule because it isn't
copper.**

⚠️ **And an `fp_rect` is NOT a line.** `(start ...)` and `(end ...)` are
**opposite** corners: treating it as a segment tests the rectangle's **diagonal**
and ignores the four edges, which are the real obstacle.

### 11g-quinquies. The "stays where it was" fallback can create the defect

With the four fixed, the tool moved five references and announced *"5 moved, 6
with no room"*. The gate said **text×text: 1**, where before it was 0. The DRC
agreed.

The list of occupied positions filled up **as the loop ran**: whoever hadn't been
visited yet was invisible. And the loop's order is the order in the **file**,
which has no relation at all to the board's geometry.

```
RV1 visited first → doesn't see R21 → moves on top of it
R21 visited later  → no room left → "stays where it was" → inside RV1
```

> **"Stays where it was" sounds like doing nothing, and it isn't.** It's
> reaffirming a position that in the meantime **stopped being free, because of a
> change by the tool itself.** The fallback that existed to be safe became the
> mechanism of the defect.

**Two things, and both are needed:**

1. **All** positions go into the list **before** the loop begins — and then the
   order stops mattering.
2. **Self-test at the end:** if two references end up overlapping, it **does not
   write** and says so. *Moving text cannot create the defect it came to tidy.*

⚠️ And this holds for every tool that tidies by iteration — routers, placers,
text tidiers. **A checker that only looks at what it has already processed
approves what it hasn't seen yet.**

### 11g-sexies. NOT COLLIDING IS A CONSTRAINT; POINTING TO THE PART IS THE OBJECTIVE

⚠️ **§11g-ter foresaw this defect in writing and it was there anyway**, on 8
references, because nothing measured it. *Diagnosed and yet-to-be-instrumented is
the state in which a defect lives longest: everybody knows, nobody counts.*

**The cause was upstream and was a constant.** The synchronizer created every new
part with the reference at `-11,5 mm` — the same for an 11 mm capacitor and for a
1,5 mm test point, 23 of the 70. And the tidier didn't touch them, because it
only moved what **collided**, and at 11,5 mm nothing collides with anything.

> **Two tools, each correct by its own criterion, and the defect in between.**
> Neither had as its mission *"the designator points to its own part"*.

> **The optimum of a constraint with no objective is always emptiness:** the
> farther away, the fewer obstacles. As long as "don't collide" was the only
> rule, the candidate *"off the board"* always won — and won rightly.

**The four pieces of the fix:**

| | |
|---|---|
| **objective** | among the free spots the **closest to its own part** wins, not the first in a fixed order |
| **owner criterion** | the part closest to the text has to be the **owner** — this is what makes it verifiable and not aesthetic |
| **ceiling** | without it the objective still rewards fleeing |
| **the text is a RECTANGLE** | an isotropic radius is too large vertically (rejects good positions) and too small sideways (accepts positions over a pad) |

⚠️ **And the fallback has THREE steps, by the scale of the damage:** with no
clean spot, you search **farther out while keeping the copper clear**, and only at
the end do you accept copper. Letting go of everything at once buys
`silk_over_copper` you don't need — and copper is what **erases** the text,
because the factory clips the silkscreen over the mask opening. **Half a
designator is worse than none: it looks like one.**

**The trade-off is told in numbers:** 8 → 0 identifying the wrong part, 75 moved;
it cost `silk_overlap` +2 and +2, and `silk_over_copper` +2 on one board. *Text
touching a trace is readable; text pointing to the wrong part lies.*

⚠️ **And it goes into the factory gate, which is the only way not to come back.**
A `refs_apontam` that compares the distance to its own part against the distance
to the nearest one. Proven both ways — a known defect injected, and the token it
reads renamed, so that the guard screams instead of approving zero parts (§2c).

### 11g-septies. A TOOL'S GUARD MEASURES WITH THE RULER OF WHOEVER JUDGES

The `arruma_silk` has its own self-test and it **approved** an arrangement that
drove the gate up from `text×text 0` to `1`. It wasn't a bug: the self-test uses
the tool's box model and `svg_colisoes` uses its own (0,55 em per character). Two
models of the same text, and the tool's is the more permissive.

> **A guard that measures with its own ruler does not guard against the criterion
> of whoever judges.** It honestly approves, by its own ruler, what the gate
> rejects.

**The fix is the external proof turned inward:** before writing, the tool runs the
**gate** over the before and the after, and refuses if the number goes up. It's
the same discipline applied to a drawing, applied to a tool of one's own.

⚠️ **And you confirm that the result OPENS, before replacing the original.** A
write with stale offsets ate the space in an `(at 0 1.65 0)` and left
`(at0 1.65 0)`: KiCad rejected the whole board and only the *next* command
revealed it. You write to a copy, load it, and only then write.

⚠️ **The F.Fab counts, but it doesn't count the same.** It isn't printed — 210 of
the 336 texts live there — and it's the layer read in the editor. Tidying it is
legitimate, with its own rules: there pads and traces cost nothing, **but the
Fab's own part outlines do cost**. *"Not printed" doesn't mean "no obstacles" —
it means the obstacles are different, and different isn't none.* Without that,
tidying pushed the values on top of the outlines and the panel board rose from 51
to 68 collisions.

⚠️⚠️ **AND EACH PASS IS JUDGED AND ACCEPTED ON ITS OWN.** Judged as a block, on
the panel board the Fab pass was optimal (51→37, text×text 10→3) and was rejected
whole because the silkscreen pass, in the same package, made the silkscreen
worse. **A good one was held hostage by a bad one because they traveled
together.** The gate was right to reject the set; the error was that the set
existed.

⚠️ **And the `total` counts as much as the `text×text`.** A gate that looked only
at the second let the panel board's Fab rise from 51 to 68 without screaming,
because the text×text had dropped. **A criterion is only a gate for what it
measures.**

Measured, with the verdict per pass in the output:

```
cerebro  ✓ Reference (F.Silkscreen)  0->0   · total   6->6
         ✓ Value     (F.Fab)        29->18  · total 114->87
painel   ✓ Value     (F.Fab)        10->3   · total  51->37
         ✗ Reference (F.Silkscreen)  0->1   RECUSADA
```

### 11g-octies. WHAT GETS PRINTED ALONGSIDE AND WHAT GETS PRINTED IN A BLOCK

A test point without the net name on the silkscreen is an anonymous point: the
name lives in the `Value` field, which is on **F.Fab** and **is not printed**.
Whoever gets to the bench sees `TP7` and doesn't know whether it's `+3V3A` or
`AGND`.

⚠️ **The name comes from the PAD's `(net ...)`, never from `Value`.** The `Value`
is a claim about the net; the pad **is** the net. If the point moves on the
copper, the `Value` keeps saying what it said — and the silkscreen starts lying,
printed, in ink, on the part. Reading from the pad, the label is either right or
doesn't exist.

**Two forms, and density decides which:**

| | when | the reference |
|---|---|---|
| **label beside** each point | wide pitch (6 mm was enough) | **leaves** the silkscreen — `TP7` next to `+3V3A` is noise |
| **legend block** | tight pitch (4 mm wasn't enough) | **stays** — it's what ties the point to the line |

> **The same decision, inverted, and both right.** What changes is not the
> criterion: it's whether the information is *beside* or *elsewhere*. Beside, the
> designator is noise; elsewhere, it's the index.

⚠️ **Eleven labels and one block don't occupy the same area.** Eleven texts have
to find eleven holes, each beside its own point; the block needs **one**, and
chooses among hundreds. That's why what didn't fit spread out fit as a whole —
**with zero added collisions**.

⚠️ **And the block goes NEAR what it describes.** A legend 90 mm away forces you
to hunt for it, and whoever has the probe tip in one hand doesn't hunt: they give
up, and measure the wrong pin.

### 11g-nonies. WHAT A TOOL DOESN'T SEE, IT RUNS OVER

The `arruma_silk` knew the **footprints'** fields and nothing else. The board's
`gr_text` — `COTUCATRONICS`, `cerebro rev B`, and later the test-point legend —
were invisible to it: it was free to move a reference on top of the board title,
and **none of its guards would notice**. A `grep -c gr_text` on the file returned
**zero**.

> **Before trusting a tidier, ask what it reads.** It obeys the obstacle list it
> has, and that list is a claim about the world that someone wrote once — it is
> not the world.

It was discovered by inverting the order of the flow: labeling first and tidying
afterward only makes sense if the tidier **sees** the labels. Without that, the
second tool silently undoes the first's work.

### 11g-decies. ONE GATE PER STEP DOESN'T APPROVE A STEP THAT NEEDS THE NEXT ONE

The right order seemed clear: the labels pick the place first (what you read at
the bench is the **net**, not the `TP7`), and the tidier moves the references away
afterward. It's correct — and it **doesn't work** with one gate per tool:

```
intermediate state: the labels on top of the references
labeler gate: text×text 0 -> 12 → REJECTED
```

The gate didn't know that the next step tidied that. It was judging a state that
was never going to exist.

> **Either the gate belongs to the FLOW, or each step stands on its own.** Both
> ways out are legitimate and the choice is one of cost: a flow gate forces you to
> redo the orchestration; "each step stands on its own" costs a worse result —
> here, the labels routing around references instead of sending them away.

⚠️ **And there's a third, which is the one that solved it:** asking whether the
intermediate state has to exist. It didn't — the test point's reference could
**change layer** instead of being moved, and then there was no competition for the
space at all. *The conflict between two steps is usually the wrong question
upstream.*

### 11g-ter. The boundary with `/design-review`, on the board side

The `/design-review` is the **visual** gate; the DRC is the **numerical** one. On
a board's silkscreen neither of the two is enough on its own:

| question | who answers |
|---|---|
| is there text on top of text, of copper, or off the board? | `kicad-cli pcb drc` — it counts |
| does the text **read**, and point to the right part? | `/design-review` — it looks at the render |
| is the reference on the side from which you solder? | only the eye |

⚠️ **A zero count does not authorize skipping the eye** — it authorizes looking
while knowing where. A technically free reference can be nestled against the wrong
part, and no DRC has an opinion on that.

### 11h. The analog ground island and the single bridge

AGND is an **island** inside the GND plane, with higher priority to win the
copper, and connects to GND **in a single place** — a bridge, at a chosen point.
Two bridges make a mesh, and the mesh is the digital return current passing
through the analog's floor. The island has to exist in **both** halves of the
arrangement: an AGND zone on B.Cu, and the vias of the AGND pads dropping into it.

**The plane closes connections for free:** 23 ground connections closed without a
single trace, just because the copper was there.

### 11i. A router that insists is worse than one that gives up

The router tries trivial paths — straight, two Ls — and routes around via A\* if
needed. **What won't come out is left for the person.** A router that insists
makes paths nobody would want and that then nobody understands on review.

And it **verifies itself**: it runs the DRC with the zones filled and **removes
the nets that get flagged**, returning them to the list. Writing and then
measuring isn't enough — it has to undo.

Four rounds of defects, and all four hold for any router you write:

1. the obstacles didn't include the traces the router itself had just laid — 81
   crossings;
2. the collision tested only the cell's center, and clearances of 0,02 mm slipped
   through;
3. the bounds came from the extent of the parts, not from the board outline —
   traces off the board;
4. and even so two nets violated, which is the reason the retraction exists.

### 11j. Guards you write once and that hold for all products

Automation only pays off if the tools **know how to refuse**. Each of these is a
line of defense that doesn't depend on my remembering:

- the router **refuses to run** if it finds a zone on F.Cu (11a);
- the outline generator **refuses to write** if the board doesn't fit in the
  enclosure, or if a mounting hole ends up under a part;
- the enclosure generator **refuses** a default thickness value in fields where
  the default is dangerous;
- the generated list comes out **stamped with date, commit, and a dirty-tree
  warning**.

> **When an instruction can kill hardware, the guarantee is not that the
> instruction is right — it's that the instruction cannot kill.** The same holds
> for a tool: the guarantee is not my remembering the rule, it's the tool
> stopping.

### 11k. The path to the factory

All in `kicad-cli`, without opening the interface, so all repeatable per product:

```bash
K=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli
$K pcb drc --refill-zones --save-board -o drc.rpt placa.kicad_pcb   # the gate
$K pcb export gerbers -o fab/ placa.kicad_pcb
$K pcb export drill   -o fab/ placa.kicad_pcb
$K pcb export pos     -o fab/cpl.csv --format csv --units mm placa.kicad_pcb
$K sch export bom     -o fab/bom.csv esquema.kicad_sch
```

⚠️ **The `kicad-cli` has no routing subcommand**, and the `pcbnew` module
**doesn't bring the PNS** (no `Shove`, no `Walkaround`, no autoroute) — confirmed
by listing the module, not deduced.

⚠️⚠️ **BUT THERE IS DSN EXPORT, AND I SAID THE OPPOSITE.** The `pcbnew` exposes
`ExportSpecctraDSN` and `ImportSpecctraSES`, so **FreeRouting — which is a serious
autorouter, with rip-up and shove — is reachable**. I wrote here, and in the
README, and in a commit, that it wasn't. See 15de.

```python
import pcbnew
b = pcbnew.LoadBoard("placa.kicad_pcb")
pcbnew.ExportSpecctraDSN(b, "placa.dsn")     # -> freerouting
pcbnew.ImportSpecctraSES(b, "placa.ses")     # <- the result
pcbnew.SaveBoard("placa.kicad_pcb", b)
```

⚠️ **AND YOU RUN IT WITH KICAD'S OWN PYTHON, which exists.** The `pcbnew`
imported from the system Python fails at `dlopen` — it asks for KiCad's wx
libraries — and I drew from that the wrong conclusion that the module was
unusable. The interpreter is at

```
/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/3.9/bin/python3.9
```

and imports `pcbnew` 10.0.5 without a grumble. I didn't find it because I listed
`bin/` with `head` and it's further down the alphabetical order, after `img2py`
and company. **An `ls | head` is not a search.**

⚠️ **And `--save-board` reformats the file.** A measurer using a regular
expression with a fixed window starts reaching fields it didn't reach before, and
the same board gives two different numbers across two measurements. That's what
happened to me, and the board hadn't changed. **Measurement by regex on a format
the tool rewrites is not measurement.**


## 12. ROUTING BY SCRIPT: what you learn when you let the machine try

### 12.0 One signal layer and a planar graph, and the connections aren't

A router limited to one layer, forever — and not for lack of cleverness. Two
signals that need to cross have no way to. Measured: the one-layer version only
closed 24 of 71 and stalled there. With the second layer and vias, the same board
went from **122 to 23**.

The way out is the industry's for forty years: you jump to the other side with
two vias, travel a bit, come back. **And the via has to be EXPENSIVE in the
search cost** (here, 30 cells), otherwise the search jumps for anything and
nothing, and each jump opens a crack in the plane.

### 12a. The damage to the plane has TWO forms, and I was only looking for one

The obvious one: `Zone [X] <-> Zone [X]` — the plane broken into islands.

The other one, which escaped me and cost a whole round: **a pad cut off from the
plane**. The trace on B.Cu passes between the pad and the copper, the pad is
orphaned, and that shows up as a perfectly ordinary unrouted connection on the GND
net. The plane stays whole. The ground doesn't. Measured: a routing run brought
the total from 47 to 20 **and took `GND` from 0 to 5**.

> **That's why you don't count one form of damage. You count the PROFILE.** How
> many connections are missing on each net, before and after, and the acceptance
> rule is a single one: **no net may get worse**. There's no symptom list to keep
> updated, and there's no new symptom that slips through by not being on the list.

A list of known symptoms protects against what has already happened. An invariant
protects against what hasn't happened yet.

### 12b. The exemption applies to the defect you saw, never to its name

The board arrives with defects that aren't the tool's, and it has to subtract them
or it spends its life blaming itself for someone else's damage and pulling up all
its traces. But **the exemption's signature is the pair (type, net), not the
net**.

I exempted all of `AGND` because of a stray via in the baseline — and with that I
let a clearance of 0,193 mm on an AGND trace slip through, which is another defect
on the same net.

### 12c. Retraction by bisection, and why you don't remove in bulk

When some net gets worse, the lazy answer is to pull up every trace that went
through the other layer. Most are harmless — the copper flows around — and pulling
them up is throwing away good work because of bad work.

You look for **which one**: you start by removing all and confirming it zeroes (if
it doesn't zero, the damage was already there and there's nothing to remove), and
then you put them back one by one, keeping those that fit. In practice it isolated
2 guilty crossings out of 33 — and the other 31 stayed.

### 12d. TIGHTENING THE CLEARANCE MADE THE RESULT WORSE

Counterintuitive enough to be written down. Measured, always from scratch and
until convergence:

| clearance | mesh | unrouted connections |
|---|---|---|
| **0,35 mm** | **0,25 mm** | **23** ← this one stays |
| 0,26 mm | 0,25 mm | 34 |
| 0,26 mm | 0,20 mm | 44 |

The reason is greed: with more space available the router shoves more traces in
early, and those traces close corridors the following connections needed — and
more of them end up removed for spoiling the plane. **A greedy router improves
when you tighten its appetite, not its clearance.**

And the finer mesh doesn't find more paths: it finds the same ones more slowly,
and hits the node ceiling before reaching the end.

> ⚠️ The first time I changed both at once, and the measurement couldn't tell
> which of them had done the damage. **One variable at a time, even when it seems
> obvious they go together.**

### 12e. Moving a part kills the copper that touched it

I moved a part on an already-routed board. The traces that led to it ended up
pointing to where it no longer was, and the next DRC returned **short circuits**
in the baseline — which the tool, correctly, treated as "not mine" and didn't
count.

> **When the placement changes, the copper that touched it is garbage.** You don't
> patch it: you erase everything and route from scratch. In an automated chain
> that costs two minutes, and that's why the chain pays for itself.

### 12f. The order of the steps, and the defect that passes every test

The via stitching ran **before** the parts were placed: it stitched the pads where
they weren't yet. It never raised an error, because the board already arrived
placed and the second run is idempotent — the numbers always matched.

**You only see it on the day something moves.** I moved the ground bridge and the
stitching kept talking about the old position.

> An order defect that only appears when something changes is a defect that passes
> **all** the tests except on the day it matters. The defense is not one more
> test: it's the order being right, and the step that depends on positions coming
> after the positions are written.

### 12g. Nobody in two places at once

A reference placed by a grid **and** by a table of explicit positions keeps the
explicit one — until the next step recomputes the grid and returns it to the old
place, silently. I moved the bridge, the file kept showing it where it was, and
**no step complained**.

> **A placement that undoes itself without saying anything is worse than one that
> fails.** This isn't solved with care: it's solved with a gate that refuses to
> start if any reference appears in both lists.

### 12h. The tool that refuses finds what nobody went looking for

The stitching refused to drop a via into a plane that didn't exist there, and said
whose pad it was. The pad belonged to the **net-tie that is the only bridge
between the analog and the digital ground** — and it was 57 mm from the island it
was supposed to join.

The topology was actually correct (it was still a single bridge), but all the
analog return current came back through a thin trace instead of through wide
copper, and **a bridge with impedance is half a bridge**. The bridge moved to the
island's boundary, with a pad on each side: plane to plane.

> Nobody had gone looking for this. It showed up because a small tool refused to
> do something senseless **and explained why**. A tool that fails silently, or
> that does what it's asked without question, would have found nothing.

### 12j. There is no winning attack order — there's a winner on this board

A greedy router gives different results depending on who goes first. Measured, from
scratch and until convergence:

| order | brain board | panel board |
|---|---|---|
| shortest first | **19** | 13 |
| the DRC's | 23 | **10** |
| longest first | 29 | 11 |

Each board has its own winner, and there's no way to guess it by looking. That's
why the tool **runs all three on copies and keeps the best, by measuring** —
instead of letting me pick one and defend it.

> Automation isn't just running the steps on its own. It's **taking decisions off
> someone's shoulders** — and a decision that's settled by measuring three times
> shouldn't be waiting on an opinion.

⚠️ And you compare by **both** measures, connections and geometry. The order that
closes the most connections may be the one that spoils the most copper, and
choosing by the first without looking at the second is choosing the worst result
with confidence.

### 12k. One at a time on the same files

Two concurrent writes on a `.kicad_pcb` **give neither an exception nor a
corrupted file**. They give a number: 34 geometry violations on a board where
neither of the two runs, alone, produced more than zero.

> **A contaminated measurement looks the same as a good one.** It carries no mark
> and enters a report with the same face as the others.

The defense is a **lock** that refuses to start if another run is already writing
the same boards, and says the pid of the one that is. Ten lines.

⚠️ And launching a second job on the same files **is a write, not a read** — even
when whoever launches it isn't touching anything, because the process left running
is.

### 12l. The algorithm that's better on paper can be worse on the machine

The retraction tests one net at a time — one DRC per net. It looks like laziness,
and the version that splits in the middle looks like the obvious correction.
Measured, it's worse on both axes:

- **slower**: with 5 guilty of 33, descending into the failing halves costs more
  tests than walking the list. Bisection pays off when there's **one** guilty one;
- **worse result**: 25 unrouted connections against 19, because the fallback it
  needs for the guilty-together ones removes **all** the candidates at once.

> **"Improving" a part nobody measured is a bet, not a fix.**

### 12m. The exit code of the previous tool is part of the measurement

The router refuses to write when it can't prove what it did, and restores the
board — correctly. If the chain **ignores the exit code**, a failed pass becomes
indistinguishable from a pass that had nothing to do, and the final number changes
with nothing to explain it.

⚠️ And the same holds for the *shell*: in zsh, **a glob with no match aborts the
whole line**. An `rm -f a b*` chained with `&&` to an edit makes the edit not run,
and the message `no matches found` gets lost in the middle of another command's
output. It cost three identical measurements and two conclusions about code that
wasn't in the file.

> **Identical results across conditions that should differ are not a result:
> they're an instrument malfunction.** That alone caught the error.

### 12n. Zero traces around is not free space

When a router fails, the easy explanation is congestion. You measure before
believing it: this board's failures had **zero traces within an 8 mm radius** and
were all concentrated on parts with two rows of pins.

What closed them off was arithmetic:

```
gap between two pads of a 2,54 header with 1,70 pads:   0,840 mm
the router required width/2 + clearance + grid-margin
                        0,125 +  0,35 +         0,178 = 0,950 mm
```

It couldn't **get out from between the pins** — of any connector, on any board.

> **A spot with nothing around it that the tool can't reach is not congested:
> it's closed off by a rule of the tool itself.**

⚠️ **And the fix is a tight pass at the END, never from the start.** Tightening
from the start measures worse (23 → 34): with more space the greedy router shoves
traces in early and closes corridors the next ones needed. At the end what was
already routed is already there, and the tight clearance only serves to get in
where the wide one couldn't.

⚠️ **The mesh tightens with the clearance**, because the grid margin
(`PASSO·0,71`) counts in the same corridor. And a fine mesh over the whole board
exhausts the search — hence the tight pass's search is confined to a **box** around
each connection: what's left over from a wide pass is almost always short.

### 12o. Between two connector pins the trace and the plane don't both fit

The same calculation, in the opposite direction. For both the trace **and** the
plane's copper routing around the pad to fit in the corridor:

```
trace/2 0,125 + clearance 0,20 + min copper 0,25 + clearance 0,20 = 0,775 mm
available on each side:                                             0,420 mm
```

A trace on the plane's layer, between two connector pins, **doesn't share the
space with the ground copper: it evicts it**, and the connector's ground pins are
orphaned. Marking the plane-net pads with a larger radius on that layer costs a
few paths and saves the retraction from discovering them one by one, at one DRC
each.

### 12p. A tool that refuses needs to be able to investigate everything it did

The retraction interrogated only the nets that had gone through the plane's layer,
because that was the theory of the damage. When the damage came from elsewhere,
the list didn't contain it, and the tool **gave up writing an almost entirely good
job** because of a trace it couldn't even flag.

> Otherwise the refusal stops being prudence and becomes a blind spot with good
> manners.

⚠️ And **a filter's reference is the physical property, not the parameter of the
moment.** The suspect filter used the run's clearance; in a tight pass the
clearance shrinks, the filter shrinks with it, and it returned "0 of 3 suspects"
in a run where there was damage.

### 12q. A two-row connector needs escape on both sides

When a connector's signals won't route and the diagnosis gives **zero traces
within an 8 mm radius**, look at the board edge. A two-row header pushed up against
an edge leaves the outer row with a narrow channel and the inner one with no way
out — because getting out the other side forces a pass between two pads, and there
only one trace fits if the clearance is the board's and not the router's (§12n).

Measured: a 1,05 mm channel for thirteen signals. Lowering the connector 4 mm (and
the block of parts just below it 2 mm, to make room) took it to 5,05 mm and closed
three connections — **18 unrouted against 21**, without touching anything else.

> **Pushing a two-row connector up against an edge gives it half the pins**, and
> nothing in the drawing gives it away.

⚠️ **And the new position is measured with COURTYARDS, not with pads.** The first
search gave 2,49 mm of clearance between pads and the DRC returned two
`courtyards_overlap`. What reserves space in a footprint is the courtyard — and
this connector's courtyard reached 0,8 mm off the board.

### 12r. Pad escape: tighten the clearance only near the ends

When the connections that remain **all end at a fine-pitch pad** — a 1,27 mm SOIC,
a connector's pins — the problem isn't the path: it's the first millimeter. The
generous clearance that serves for the rest of the route doesn't fit between the
pads you need to get out of.

Tightening the clearance everywhere measures worse (23 → 34), because the greedy
router shoves traces in early and closes corridors. Tightening it **only in the
first ~3 mm around the two ends** doesn't have that effect: there's no corridor to
close there, there's a pad you need to get out of. Measured: **18 → 12**.

> **A parameter that's bad everywhere can be exactly right in one place.** Three
> experiments measuring the clearance high and low, when the answer was that it
> shouldn't be a global number at all.

⚠️ The tight value never drops below the board's rule, and the DRC validates the
whole route with the same rule — the retraction still exists behind it.

### 12s. When tuning stops paying off, what's missing is a capability

Mesh, clearance, node ceiling, attack order, a corridor found on a coarse mesh:
five tunings, none yielded more than two connections and three yielded less. All
started from the same assumption — **that there's a free path and it just needs
finding**. For the last ones there isn't.

The missing capability is called **rip up and redo**: you remove the copper that's
in the way, you pass through, and you re-route what you removed, which now goes
around because it had alternatives and the other didn't. In an isolated pass:
15 → 8.

> **When five tunings in a row yield one or two each and oscillate between runs,
> you're no longer improving anything — you're adjusting noise.** It's the signal
> to change your mind, not to tune better.

⚠️ **And a right capability may not be ready.** In the whole chain
rip-up-and-redo measured 14 against 12, and one run delivered the board with a
broken ground plane that the router's self-test didn't catch — it was the factory
gate that saw it. It stayed optional.

> **The last line of defense cannot be the same tool that did the work.** A
> self-test has exactly the blind spots of whoever wrote it.

### 12t. An invariant only protects in the dimension in which it was measured

The acceptance rule — *no net may get worse* — replaced a symptom list precisely
so that no new symptom could slip through by not being on the list (§12a). One
slipped through:

```
before GND = 1   (one loose pad)
after  GND = 1   (the pad connected, and the PLANE BROKE)
got worse -> NOTHING
```

Counting per net protects against the wrong net; **it doesn't protect against the
wrong kind within the same net**. A broken plane is not "one more unrouted
connection" that's offset by any other. The key became **(net, kind)**.

⚠️ **And it's the same family as the violations' `(type, net)` signature** (§12b),
which I had fixed hours earlier in the same file. **A lesson learned in one place
doesn't apply itself to the place next door** — look for the twins yet to be fixed
whenever you fix one of these.

⚠️ **And the checker gets a checker.** `roteia.py --auto-teste` interrogates the
rule with the cases it has to catch, starting with the one it let through. You
prove it has teeth by running it against the old version: it fails.

> **A checker that isn't checked is an opinion with the air of a measurement.**

### 12u. Undoing work to attempt more work follows the same rule

Ripping up the copper in the way to let another connection through (*rip-up*) is
the capability that's missing when the last connections no longer have a free path
— and no tuning replaces it, because no search finds a path that isn't there. In
an isolated pass: 15 unrouted → 8.

But if the ripped-up net can't be remade, it's a trade — and sometimes a loss,
because the ripped-up one **was done**.

> **An operation that undoes work to attempt more work needs the same rule as
> everything else: it only counts if the number goes down.** You save the state
> before, and you go back if it didn't drop.

⚠️ And you limit how much it rips up. If a connection needs to rip up half the
board, it's not that connection that's badly routed — it's the design, and that's
a question for the designer, not a problem for the router to solve with a hammer.

> **A capability that can only improve or do nothing belongs in the default
> path.** Before the two corrections, this one didn't belong — it closed
> connections and broke planes.

### 12v. FreeRouting is reachable, and it's better

The `pcbnew` exposes `ExportSpecctraDSN` and `ImportSpecctraSES`. On the same
board, from scratch:

| | unrouted | time |
|---|---|---|
| hand-written router | 12 | ~9 min (two boards) |
| **FreeRouting** | **8** | **35 s** |

And the ones left over to FreeRouting are **all connections to the plane**, not
signal: it closes all the signals. It does real *rip-up* and *shove*.

```python
import pcbnew                      # with KiCad's BUNDLE Python
b = pcbnew.LoadBoard("placa.kicad_pcb")
pcbnew.ExportSpecctraDSN(b, "p.dsn")
# java -jar freerouting.jar -de p.dsn -do p.ses -mp 100 -mt 1
pcbnew.ImportSpecctraSES(b, "p.ses"); pcbnew.SaveBoard("placa.kicad_pcb", b)
```

⚠️ **Four things it doesn't know, and that whoever calls it has to handle:**

1. **Net-ties.** The bridge between two grounds appears to it as copper "with no
   net" and it routes over it: short circuit. You put a *keepout* over it in the
   DSN.
2. **`-mt 1`.** The tool itself warns that the multi-threaded optimization is
   broken and generates clearance violations. A tool's warning about itself is not
   ignored out of haste.
3. **The widths come from the board's net classes**, not from a table inside your
   script (§12w).
4. **It counts a pad as connected if it falls inside the plane's polygon**; KiCad
   only counts it if the *filled* copper reaches it. Around a tight connector the
   copper is erased in the fill and the two disagree. KiCad rules.

⚠️ And it phones home on every startup (`api.freerouting.app`), with no documented
option to turn it off.

⚠️ **And the stitching starts getting in the way.** The vias that were placed so
the SMD pads could reach the plane are no longer needed — it does it on its own —
and they take up room: with them the plane comes out broken. When you swap the
router, you reassess every piece that existed to compensate for the old one's
weaknesses.

### 12w. A rule that lives inside a tool only holds for that tool

The widths of the power rails lived in a table inside my router. It worked while
the router was mine — and it hid that **the DRC didn't know about them**. When an
outside router came in, it drew the input rail at signal width, because it had no
way to know.

> The place for a design rule is the board's **net classes**: any router reads
> them, and the DRC starts requiring them instead of ignoring them.

⚠️ And they're assigned by **name pattern**, not by list: a net added later enters
the right class on its own, instead of being forgotten at the default width.

### 12x. `Zone [X] <-> Zone [X]` is NOT a stable measurement

⚠️⚠️ **The most uncomfortable finding of this series, because it invalidates
verdicts I had already given.**

Five runs of `kicad-cli pcb drc` on **identical copies of the same file**, with the
zones filled once and saved:

```
broken planes:       3 · 0 · 0 · 3 · 0
unrouted connections: 15 · 15 · 15 · 15 · 15
profile per net:     AGND=4 GND=8   in all five
```

The **facts** are stable; the **label** isn't. The same unrouted item comes out
sometimes described as *zone against zone* and other times as pad against
something else.

I had built the gate's verdict on top of that label, and reported "whole planes ✓"
dozens of times over a coin toss. Worse: the router's retraction made decisions
with it — which explains part of the variation between runs that I had been
attributing to the router's greed (§12d, §12j).

> **The criterion that rules is the PROFILE PER NET**, which is stable and which
> §12a already taught. A plane that truly breaks drives up the unrouted
> connections of its net, and the profile catches that. The `Zone <-> Zone`
> counter stays as an **indication** — it serves to know where to look, never to
> decide.

⚠️ **And a gate does not reject on noise**, because a gate that rejects on noise
teaches whoever reads it to ignore it.

**How you discover this:** by running the same measurement on the same file more
than once. It's cheap, almost never done, and it's what separated the fact from
the label here. The same discipline caught the aborted glob (§12m) — *identical
results across conditions that should differ are an instrument malfunction*; this
is the opposite face: **different results across conditions that should be
identical are another.**

### 12y. The mesh step MUST divide 1,27

The escape of a pad from the back row of a 2,54 header passes through the exact
middle of the gap between two front pads — with 1,70 pads and 0,22 clearance the
tolerance is **0,075 mm**, at 1,27 from the pad's center. A 0,25 mesh (1,27/5,08)
has no node there, and the router answers "left for the person," **just like** when
the board is full. Measured on the panel board: 0,25 → 0 of 3; 0,125 → 0 (misses
by 0,002); 0,0635 (1,27/20) → 3 of 3 on an empty board. The `roteia.py` refuses a
step that doesn't divide.

⚠️ **And fix the mesh before concluding there's no space** — but don't stop at it:
the three only closed on the real board when the back row got a way out (`J1` came
down). Two causes, the same face. (scar 15eq)

### 12z. Courtyard against the EDGE: KiCad doesn't ask, and the corner is round

The DRC has no courtyard-against-`Edge.Cuts` rule by default: "geometry 0" with
`J1` 2,83 mm outside. The pad doesn't serve as a measure (the pads were inside;
the plastic stuck out). `dentro_do_contorno.py`:

- each courtyard vertex **against the polygon** of the edge, and each edge vertex
  against the courtyard — the corner has radius 3 and a vertex 0,535 from the top
  edge fell outside the arc;
- distance via `NearestPoint`, **never** `SHAPE_LINE_CHAIN.Distance()`, which on a
  closed chain gives 0 for every interior point (74 of 74 failed at `0.000` in the
  first version);
- a footprint without a courtyard is a finding; exemption with a number in the
  reason;
- proven by moving `SW1` up 25 mm (8 mm didn't get out of a 36 mm board). (15er)

### 12i. The gate before the factory

An order is irreversible and takes weeks: the error isn't discovered on the
screen, it's discovered when the box arrives. That's why the last step isn't
"export gerbers" — it's a gate that measures and **refuses**, and only then
exports.

What you measure, and why each one is on the list:

| measure | why |
|---|---|
| geometry | the factory refuses — or worse, doesn't refuse, and delivers a shorted board |
| unrouted connections, **per net** | an incomplete board isn't incomplete: it doesn't work |
| `Zone [X] <-> Zone [X]` | ground plane in pieces, invisible among the connections |
| `starved_thermal` | pad clinging to a narrow island: solders poorly, conducts worse |

And the package comes out **stamped with generated date, commit, and a dirty-tree
warning**: a package that doesn't say what repository state it came from can't be
reproduced, and what can't be reproduced can't be fixed.

⚠️ **An incomplete package has to be impossible to confuse with a good one.** The
forced output exists — there are legitimate reasons to want to see the gerbers of
an unfinished board — but it writes `INCOMPLETA` in the file name.

⚠️ **And the gate says what it CANNOT see.** It proves the copper matches the
netlist, not that the netlist is right. And the **assembly rotation** isn't
verified by any script: the angle KiCad writes in the CPL is the footprint's, and
the factory expects its library's — for many packages the two don't match, and you
confirm it part by part in the preview, before paying. The file comes out; the
promise doesn't.
