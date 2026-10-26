<!-- ⚠️ THIS FILE IS NOT A LIVE SKILL. -->
<!-- On moving from .claude/skills/ to docs/, this file came into the reach of
     check_docs, which until now had never read it. The names below are KiCad
     codes and tools outside src/ — not params. Declared, not deleted. -->
<!-- doc-check: ignore exclude_from_bom (field of the .kicad_sch format) -->
<!-- doc-check: ignore global_label (element of the .kicad_sch format) -->
<!-- doc-check: ignore lib_symbols (section of the .kicad_sch format) -->
<!-- doc-check: ignore isolated_pin_label (KiCad ERC violation code) -->
<!-- doc-check: ignore pin_not_connected (same) -->
<!-- doc-check: ignore power_pin_not_driven (same) -->
<!-- doc-check: ignore power_in (KiCad electrical pin type) -->
<!-- doc-check: ignore power_out (same) -->
<!-- doc-check: ignore svg_colisoes (tool in tools/, outside the reach of check_docs) -->
<!-- doc-check: ignore colisoes_sch (same) -->
<!-- doc-check: ignore kicad_layout (name of another skill, not a param) -->

# ⚠️ RETIRED on 09/09/2026 — replaced by `/aipcb`

This is the full content of the `/kicad` skill as it was on the day it left
`.claude/skills/kicad/`, moved here without a single line changed. **It keeps the
original frontmatter on purpose** — it is the record of what the skill was, not a
file to load.

**What replaced it:** `/aipcb`. The fundamental difference is written there:
the `/kicad` described generators that wrote KiCad files by hand; the `/aipcb`
describes a flow in which nobody writes KiCad files by hand.

⚠️ **Old references of the kind «skill `kicad` §7» point here**, not to the
tombstone in `.claude/skills/kicad/SKILL.md`. They are scattered through
`ERC-ISENCOES.md`, through the revB generators and through the `battle_scars` —
they were not rewritten, and it is deliberate: rewriting them would erase the
trace of when they were written and against which version.

---

---
name: kicad
description: Draw, generate, check and deliver KiCad schematics — prove that the file OPENS before anything, ERC on every edit, the junction dot as semantics, the three crossing patterns, the connection grid, what every schematic declares, and the contract with the BOM. Use when drawing or changing schematics and generators, when assigning footprints or assembling a BOM, and before giving any sheet as checked. ⚠️ Layout and routing live in the `kicad_layout` skill, and it is MANDATORY at the close of any board — this one alone never closes a board.
---

# KiCad by script — the rules that cost sheets

## BOUNDARY WITH `design-review`

This skill is the **ELECTRICAL** gate of a schematic; the `design-review` is the
**VISUAL** one. They are not two levels of rigor of the same question — they are
different questions, and **neither replaces the other**:

| question | who answers | what it does NOT see |
|---|---|---|
| does the file **open**? | `kicad-cli sch erc` (this skill) | none of this appears in the drawing |
| does the **copper** match the netlist? | **`kicad_layout`** skill | a clean schematic says nothing about the board |
| do the connections **exist**? | ERC + netlist (this skill) | a pin and a wire at the same spot, unconnected, are **identical** in the render |
| does the drawing **read**? | `design-review` | text over text does not appear in any netlist |

⚠️ **A skill that orders "render and look" and does not order confirming that the
file opens is wrong by OMISSION** — and its silence has the same face as a good
sheet. If another schematics skill exists, it must POINT to this one instead of
repeating the content: two saying almost the same thing drift, and then nobody
knows which one rules.

## ⚠️⚠️ CLOSING A BOARD REQUIRES `kicad_layout`, ALWAYS

This skill proves the **schematic**. A proven schematic says nothing about the
copper, about the silkscreen, or about the package that goes to the factory —
and the two things have already been confused here.

> **Never give a board as closed with this skill alone.** Before generating
> gerbers, before ordering, before saying it is ready: **run the `kicad_layout`.**
> It is not «consult if in doubt» — it is a step of the close.

```
proven schematic (this skill)  →  kicad_layout  →  para_fabrica  →  order
```

⚠️ **Pointing is not running**, and that is why this is at the top and not in a
footnote: the layout skill has 1058 lines and is where the defects that cost a
remade board live — planes on both layers, text over a pad, a designator
pointing at the wrong part. None of them appears in the ERC.

## 0. THE RULE THIS SKILL EXISTS TO ENFORCE

> **"The checker passed" and "the sheet opens" are DIFFERENT questions.**

On 01/09 two sheets were delivered with the netlist checker clean and the visual
review done three times. **Neither of the two opened in KiCad.**

```
gerado:    (symbol "D_V_0_1_0_1")   ← four numbers
correcto:  (symbol "D_V_0_1")       ← two
```

The `kiutils` adds `_unitId_styleId` on its own, and the generator was already
putting the suffix by hand. KiCad splits that suffix to deduce the parent
symbol's name, found a parent that does not exist, and **refused the whole
sheet** — with a single error line, `Failed to load schematic`, that points
nowhere.

The net checker could not catch it: it checks **connectivity**, not the
**grammar of the file**. Two questions, and only one was being asked.

**First command of any delivery:**

```
kicad-cli sch erc <file>     # if it fails to load, nothing else matters
```

## 1. The `kicad-cli` probably EXISTS — look by the PLACE, not the name

```
/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli      # macOS
```

It is not in the `PATH`. A whole session gave it up for missing because it ran
`which kicad-cli`, and so delivered sheets with no ERC. **Look inside the bundle
before declaring there is no tool.**

Subcommands: `erc`, `export` (svg, pdf, netlist, bom), `upgrade`, `drc`.

⚠️ **AND THE PROJECT IS FOUND BY THE FILE NAME.** A copy called
`_teste_x.kicad_pcb` has no `.kicad_pro` to match it, and the `drc` falls back to
the **default rules** — inventing violations the board does not have. Here that
produced `track_width 0 → 8` on a change that only touched silkscreen text, and I
almost reported it as a regression.

> **A before/after comparison is only valid if the two sides are measured in the
> same context.** Measuring the "before" in the real place and the "after" in a
> copy compares two environments, not two versions.

⚠️ **And the trap has two faces.** Fixed — both sides in the copy — the
comparison became valid and the absolute numbers stayed wrong: the
`track_width 8 → 8` was true *in that context* and false about the board.
**A number right to compare and wrong to quote.** Either the copy takes the name
that matches the `.kicad_pro`, or you only read the difference.

## 2. Bisecting the "Failed to load schematic"

The error does not say where. The method that works is to reduce until it
disappears:

1. Minimal file (header + `paper` + empty `lib_symbols`) — **does it load?**
2. Add ONE block at a time: `wire`, `junction`, `global_label`,
   `lib_symbols`.
3. Within the guilty block, **one symbol at a time**.
4. Within the symbol, remove parts until it loads.

On 01/09 this went from the whole file to the exact token in six steps.

⚠️ **And prove first that the TOOL works**: run the ERC on a known third-party
`.kicad_sch`. Without that, "fails to load" could just as well be the file or the
`kicad-cli`.

## 3. Connection grid: 1.27 mm, and it is not aesthetics

KiCad connects by **coordinate coincidence**, and the editor's grid is 1.27 mm.
Coordinates in whole millimeters connect **today** — because a pin and a wire
share the same value — and break **on the day someone drags a wire**: the cursor
snaps to the grid, the end goes to 97.79 and the pin stays at 98.00. The two
remain DRAWN touching, and the connection no longer exists. With no warning.

**Snap, and where:**

| snap | do not touch |
|---|---|
| placement of the symbols | drawing inside the symbol |
| wire ends, junctions, labels | pin offsets (usually already in multiples of 2.54) |

**Measure before transforming:** count how many points in each category are off.
On 01/09 the internal pins were 100% on the grid and only the placement was off —
rounding the drawing would have distorted diode triangles to fix nothing.

**The guarantee:** rounding is a **pure function**, so two points that coincided
still coincide — no connection is lost. The risk is the reverse: two DISTINCT
points less than half a grid apart falling onto the same spot and **inventing** a
connection. Always check, and **raise** instead of warn: an extra connection does
not appear anywhere until fabrication.

**Acceptance:** identical netlist before and after.

## 4. THE JUNCTION DOT IS SEMANTICS, not decoration

This is the most expensive defect family in schematic drawing, because **neither
of the two sides raises an error anywhere**:

| | means | when it is wrong |
|---|---|---|
| crossing **without** a dot | the wires **do not** connect | a missing dot = **open circuit** |
| crossing **with** a dot | the wires **connect** | an extra dot = **short on paper** |

🚫 **A FOUR-WAY crossing with a dot is forbidden.** It is ambiguous: nobody can
tell "two nets that cross and one has a junction" from "four wires all
connected". It is split into two offset T-crossings.

**And the case discovered late:** a pin in the middle of a wire segment does
**NOT** connect. Only at a wire end, or with an explicit junction. A `PWR_FLAG`
placed in the middle is left loose — and in the drawing it is touching the wire,
indistinguishable from connected.

⚠️ **It is the same family:** in all these cases, the drawing is pretty and the
net is wrong. **The eye cannot arbitrate** — only the netlist.

## 4b. The CROSSINGS family, with the patterns named

A crossing is not a lapse of care; it is the drawing saying that the
**arrangement** is wrong. Three patterns, and each has its own fix:

**C1 — the node with two loads in opposite directions.** In the non-inverter, the
`Rf` rises from the `−` input and the `Rg` descends from it, and the path crosses
the `+` input's wire. 🚫 **Not solved by moving parts** — the crossing is
topological. It is solved by putting **both loads leaving in the same
direction**, with the signal entering from below.

**C2 — the rail drawn as a long line.** A supply line that crosses the sheet
crosses everything it meets. It is solved with the supply in **its own
sub-section**, connected by **labeled stubs** — the label is the wire.

**C3 — a label collision** with a wire or with another label. It is the only one
of the three that the `design-review` catches, and the only one that does not
change the topology.

## 5. Grid BEFORE emitting, and generate by script

**Snapping to the grid is an EMISSION step, not a review one.** See §3 for the
mechanism and the guarantees. What this section adds is the timing: if the snap
runs after the sheet is given as good, it **moves everything** and invalidates
the inspection that approved it.

**And draw by script, not by hand.** A generated schematic is reproducible,
diffable and fixable at the source; a hand-drawn one is an artifact nobody knows
how to remake. On 01/09 there were **176 off-the-grid warnings** on a sheet drawn
with coordinates chosen by eye — and the fix was a three-line function in the
generator, applied to everything at once.

## 6. `power_pin_not_driven`: declare, do not exempt

Home-made symbols do not declare `power_out`, and every `power_in` pin is left
orphaned. The answer is a **PWR_FLAG per power net** — a symbol with a
`power_out` pin, a reference starting with `#` (convention: it does not enter the
BOM).

⚠️ **ONE per net, and only one.** Two `power_out` on the same net are a conflict:
you trade one error for another.

⚠️ **The `#...` reference is NOT shown.** Besides the convention, it is a certain
collision: the label falls over the global label next to it.

⚠️ **And the flag does not go at the label's coordinate** — it covers it. It goes
at a free point of the same wire, **with a junction** (see §4).

## 7. Exemptions: with a deadline, with a reason, and never in bulk

A generous exemption turns a checker into a rubber stamp. Each one carries
**why** and **until when**.

⚠️ **An exemption that FLIPS is the most valuable to mark.** `isolated_pin_label`
on an isolated sheet is a legitimate false positive; **within the single project
it becomes a net with no destination** — a real defect. The same report line
changes meaning, and whoever did not mark the date does not notice.

⚠️ **And declare where the exemption is NOT applied.** KiCad stores them in the
`.kicad_pro`; without a project, the record is just a document. **A record
without application is a promise, and it is said to be one.**

## 7b. What EVERY schematic declares

A schematic that does not say these things forces whoever reads it to guess — and
whoever guesses, guesses wrong one time in ten:

- **The voltage, and WHERE IT COMES FROM.** «5 V» is not enough: 5 V from the
  step-down, from the USB, or from the local regulator are three different nets
  with three different behaviors.
- **What is PER CHANNEL and what is SHARED.** Two chains that share a source are
  not two independent chains, and the drawing has to show it.
- **⚠️ What is a PER-UNIT ADJUSTMENT.** Gain trimpot, piezo matching, exciter
  level: values that belong to **this** unit and not to the production run.
  Without that mark, someone copies them to the next part — and carries the
  calibration of one body to another body.
- **Distinct grounds with distinct SYMBOLS**, when they are distinct nodes. A
  `GND` and an `AGND` drawn with the same symbol are a connection nobody sees and
  nobody can discuss.
- **Values that depend on a PREMISE, with the premise written.** «R1 4M7» says
  nothing; «R1 4M7 with a ~20 nF piezo cuts at 1.7 Hz» says what is lost if the
  piezo changes.

## 7c. The contract with the BOM

The schematic and the shopping list are the same document seen from two sides.
What keeps them married:

- **The designator is a KEY.** `C7` in the drawing and `C7` in the BOM are the
  same part, and any divergence is a defect — not a matter of style.
- **The package is part of the PURCHASE**, not a layout detail. 0805 and 1206 are
  different parts with the same value.
- **MPN yes; distributor code with a CAVEAT** — it changes, runs out and differs
  by region. And **never invented**: a plausible, wrong code costs a whole order,
  and looks right until the box arrives.

⚠️ **An assumed footprint is a respin.** What depends on measuring a part in hand
— modules, connectors, buttons — stays **empty until the part arrives**. Empty is
honest; guessed is not.

## 8. The generator is the SOURCE

Never edit the `.kicad_sch` by hand: the two versions diverge and nobody knows
which one rules. Every fix goes into the `.py` and is regenerated.

**And the source has to run off the author's machine.** On 01/09 the generators
had `/mnt/user-data/outputs/` and `/home/claude/*.svg` hard-coded — they blew up
with `FileNotFoundError` on the first attempt on another computer. Paths relative
to the file itself (`pathlib.Path(__file__).parent`), always.

## 8b. GENERATED IS NOT ENOUGH: IT HAS TO BE REPRODUCIBLE

§8 says the generator is the source because the generated is «reproducible,
diffable and fixable at the source». **A generator that mints a `uuid4()` per
element on each run is none of the three.**

Adding a comment to the generator and regenerating produced **229 insertions and
229 deletions, with ZERO non-`uuid` lines**. The sheet came out electrically
identical and the diff was pure noise.

> **What is lost is not elegance, it is the REVIEW.** A real two-line change
> disappears inside the churn, and a diff nobody can read is not reviewed. A
> review that does not happen and a review that approves everything are the same
> thing.

**The key is semantic, not a counter:** `uuid5(NS, "sym:R13")`,
`"fio:x1,y1-x2,y2"`, `"rot:GND:x,y,ang"`. A counter would also be deterministic
— and inserting an element in the middle would renumber everything after it,
returning the illegible diff exactly when it matters most.

⚠️ **Its own namespace per sheet.** With a shared one, the `C1` of one sheet and
the `C1` of another give the **same** `uuid` — and a repeated `uuid` between
sheets of the same project is a serious defect, not noise.

⚠️ **The switch opens a door the random one kept closed: the COLLISION.** Two
exactly overlapping wires, two texts at the same point — they ask for the same
key, and KiCad does not complain: it joins them, or loses one. Without a guard
that shouts, this trades a visible noise for a silent defect, which is the worst
deal there is.

⚠️ **And the root is fixed to the value the file ALREADY HAS**, not to a new
constant — this way going deterministic does not touch it a single time. (See
§6c: the root's identity comes from the `.kicad_pro` and can never be minted.)

**Acceptance: generate twice and compare byte by byte.** And against the previous
commit, the change has to be `uuid` only — churn once, and never again.

⚠️ **And convert them ALL at once.** I converted two, reported «the two
generators», and a third was left — because I counted the ones I had looked at,
not the ones that exist. A `grep -c uuid4 gera_*.py` answers in seconds and
depends on no memory at all.

⚠️ **This usually uncovers a second defect:** a line that **copies** the body of
an emitter instead of calling it. When the emitter changed shape, only the
original followed. **A copy of an emitter silently survives the original's
changes**, and only shows up when the original changes.

## 9. The output loop, and it includes the ERC

The `design-review` says: *edited → rendered → looked again*. Here the same holds
for electrical verification, and it was by forgetting it that a sheet with loose
pins was delivered:

> **Editing invalidates ALL previous inspections, not just the visual one.**

After any change to the generator:

1. regenerate
2. `kicad-cli sch erc` — **and read the error count**
3. netlist, compared with the earlier one
4. render, and **look**
5. only then package

Skipping 2 after touching the geometry was what produced `pin_not_connected` on a
sheet that had already given zero errors.


## 2b. FLOATING-POINT NOISE: the number no tool reads

`h/2 - 1.27 - 6*2.54` does not give zero. It gives **`1.7763568394002505e-15`**,
and KiCad accepts it without complaint. No reader written with `(-?[\d.]+)` —
which is how everyone writes it — catches that line.

The effect is not an ugly number. It is **silent disappearance**: pins 13 and
14 of an IDC26 vanished from FOUR generators at the same time, and the sheet came
out with 24 labels instead of 26. No error, no warning, no ERC. The two vias
missing were ground.

**The two halves of the fix, and both are needed:**

1. **In generation** — every number that enters a KiCad file goes through a
   formatter. `round(float(v), 4)` kills scientific notation at the origin;
   0.1 µm is three orders of magnitude below any fabrication tolerance.
   ```python
   def num(v):
       return "%g" % (round(float(v), 4) + 0.0)   # the +0.0 swaps -0.0 for 0
   ```
2. **In reading** — every coordinate regex accepts an exponent:
   `(-?[\d.]+(?:[eE][-+]?\d+)?)`. Fixing only the generation leaves the tool
   blind to the next file that comes from another hand.

> **When counting suspect coordinates, do not use `[0-9]e-?[0-9]+` raw:** it
> catches hexadecimal UUIDs and inflates the count. Anchor on
> `\((at|xy|start|end) `.

## 2c. EVERY READER COUNTS WHAT IT READ

The lesson that follows appeared **three times on the same day**, in three
different tools, and one of them was the checker written to catch the other two:

| reader | had | read | what it announced |
|---|---|---|---|
| `pinos_de()` (×4 generators) | 26 pins | 24 | drew 24 labels |
| `contrato_idc.py` v0.1.0 | 26 vias | 24 | *"the 26 vias match"* |
| `colisoes_sch.py` v0.3.0 | 40 properties | 0 | *"0 collisions"* |

> **A reader that silently loses lines fails nothing — it passes everything.**

**The guard, and it is cheap:** count how many blocks exist and require that the
reading catches them all.

```python
blocos = len(re.findall(r'\(pin\s', b))
if blocos != len(lidos):
    raise SystemExit("*** o símbolo tem %d pinos e a leitura apanhou %d "
                     "— LEITURA INCOMPLETA, nada a jusante é confiável"
                     % (blocos, len(lidos)))
```

And the guard **counts the same universe as the reader**: counting hidden
properties (`(hide yes)`), which the reader skips on purpose, gives a false
alarm — and a guard that gives false alarms is a guard that gets turned off.

**Proving the guard requires TWO tests**, because one alone deceives:

- inject the **known** defect → the new regex reads it, the guard stays silent;
- inject an **unknown** defect (for example renaming `(number` to
  `(numero`) → the guard **fails it**.

Only the first test proves the regex, not the guard.

## 2d. THE BLOCK STUCK TO THE PREVIOUS LINE

A `.kicad_sch` can have `)\t(symbol` — the next block beginning on the SAME line
where the previous one closes. A sweep anchored on `\n\t(symbol` skips it without
a warning. That is how, in a rename of 27 references, **one** escaped; the report
said "26 renamed" and I accepted the number without comparing it to 27.

> **Sweep by parenthesis count, not by indentation.** And when a report says
> "N done", compare N with what was expected.

⚠️ **And the parenthesis counter skips STRINGS.** A schematic with notes like
`100R serie (~30 mA)` or `(!)` desynchronizes the depth at the first parenthesis
that is inside quotes, and from there on **all** blocks come out wrong. Here that
returned **zero** DNP symbols in a file with eight. A counter that does not know
quotes is not counting the structure — it is counting characters that look like
it.

⚠️ **And KiCad's netlist is INDENTED.** `(net`, `(code` and `(name` each come on
their own line. A regex like `\(net\s+\(code[^)]*\)\s*\(name "([^"]*)"\)` never
matches, the `finditer` catches **one** giant block, and all the file's nodes are
assigned to the first net's name — which reads as *«the module's 40 pins are
shorted»*.

> **What caught both was the result being ABSURD, not a guard.** 40 pins on a net
> is a shorted module; 0 DNP in a file where `grep` had already shown 8 is
> arithmetic. If the same regexes had returned *39* and *7*, both numbers would
> have passed. **Luck is not a method** — the §2c guard is: how many blocks there
> are, how many the reading caught, and shout if they differ.

## 2e. THE NEGATION CONTAINS THE AFFIRMATION

In a single day, four of my checks gave the verdict **backwards**, and none
because the code was wrong:

| the test | why it falsely passed |
|---|---|
| `grep -q "gravado"` | the refusal says **`"Nada gravado"`**, which contains `"gravado"` |
| `case "$out" in *gravado*)` | the same, another syntax |
| `set -- $cfg` in a `for` | **zsh does not split words** without quotes: two values came in as one and corrupted the file |
| search for prose in a skill | `"preenchimento"` where the file says `"enchimento"` |

> **A failure message almost always contains the success word**, because it is
> written to be read by people: *«nada gravado»*, *«no changes committed»*,
> *«failed to build»*. A test that matches the substring reads the refusal as
> approval — and it is the worst way to fail there is, because the tool was
> right and whoever consulted it understood the opposite.

**Anchor on the structure, not the word:**

```bash
grep -q "^ *gravado ·"        # the start of the line, not the loose word
[ "$(cmd; echo $?)" = 0 ]     # better still: the exit code
```

⚠️ **And the exit code is what you read when it exists.** `cmd | tail` returns
`tail`'s status; `$?` after a pipe is not that of the command that matters. This
has already made me announce «exit 0» from a tool that had exited with 1.

⚠️ **And what stopped me in all four was the result being absurd, not a guard** —
a `git status` showing the board unmodified after I had read «accepted».
**Luck is not a method.**

## 5b. THE SIGN OF `y` LIVES IN ONE PLACE ONLY

On the **sheet** the `y` grows DOWNWARD. In the **symbol** it grows UPWARD.
Writing the continuation lines of a note with `y - 3.8` — which is what the
symbol's intuition dictates — prints the note **backwards**: the render shows the
last sentence first.

It happened in **9 notes from two generators at the same time**, because the
coordinate was repeated at each call site. The fix is not to correct the 9:

```python
def nota(x, y, *linhas, tam=1.6, passo=3.81):
    """O SINAL DO Y VIVE SÓ AQUI."""
    for i, s in enumerate(linhas):
        texto(s, x, y + i * passo, tam if i == 0 else 1.27)
```

⚠️ **Careful with the name:** `rot, nota = VIAS[n]` in a loop shadows the
`nota()` function and the error that comes out is `TypeError: 'str' object is not
callable`. Rename the VARIABLE, never the function.

⚠️ **KiCad does not draw emoji.** A `⚠️` in a note comes out as an empty square.
Use `(!)`.

## 6b. ANNOTATION: the reference MUST end in a digit

`SWL`, `SWR`, `RVA`, `R0A`, `C1B`, `DA`, `JA` read very well and **KiCad treats
them as NOT ANNOTATED**. The warning is discreet:

```
Warning: schematic has annotation errors, please use the schematic editor to fix them
```

it comes from `kicad-cli sch export netlist`, **not from the ERC**, and it is
easy to skip over. The risk is not the warning: it is the editor **renumbering
those references on its own when saving**, and the correspondence with drawings,
BOM and documents breaking silently.

> **The reference is `PREFIX` + `NUMBER`. The meaning (channel, side, function)
> lives in the VALUE and in the note, which is where it is read.**

A scheme that works: **the tens digit says the channel** — channel A `R10..R14`,
channel B `R20..R24`, `C11..C14` / `C21..C24`, `RV1`/`RV2`. And the reference is
unique across the WHOLE hierarchy, not per sheet: choosing the tens digit without
checking the other sheets creates a clash (here, the socket already used
`R20`/`R21` and had to go to `R50`/`R51`).

**The other cause of the same warning, and it is treacherous:** units of the same
component have to agree on **value** and **footprint**. `U1` unit 3 said
`"OPA1642 8/4"` and units 1 and 2 said `"OPA1642"` — and that alone keeps the
annotation warning after all the references are correct. It cost three wrong
hypotheses before it was measured.

**What was NOT the cause of THIS warning**, tested and discarded: missing or
obsolete `(instances)` blocks. ⚠️ **That does not make them dispensable** — they
are the cause of SOMETHING ELSE, the repair box on loading; see §6c. It was
precisely by measuring only the annotation warning that it was concluded too
early that `(instances)` did not matter.

## 6c. THE REPAIR BOX ON LOADING — and no command sees it

> *"An error was found when loading the schematic that has been automatically
> fixed. Please save the schematic to repair the broken file or it may not be
> usable with other versions of KiCad."*

**The cause is the missing `(instances)` in the symbols.** Every placed symbol
needs:

```
(instances
  (project "<project name>"
    (path "/<root>/<sheet>"      ; on a child sheet
      (reference "R50")
      (unit 1))))
```
and, for a symbol on the root itself, the path is just `/<root>`.

⚠️ **The first segment comes from the `.kicad_pro`, not from the `(uuid)` of the
root's `.kicad_sch`.** The two can diverge — here they did — and KiCad uses the
one from the `.kicad_pro`. Writing the other produces data that is born obsolete.

⚠️ **A generator that mints a new uuid on each run guarantees they never match.**
The root's identity is FIXED: constant in the generator and equal to the
`.kicad_pro`'s. (And a `.kicad_pro` copied from another project brings the
other's identity: here two projects claimed the same root.)

**What makes this dangerous is that NO command-line tool denounces it:**

| tool | what it says about a file with no `(instances)` |
|---|---|
| `sch erc` | the usual violations, no more no less |
| `sch export netlist` | exports fine; the warning it gives is about **annotation**, a different thing |
| `sch upgrade` | *"Schematic file was not updated"* — the format is already the current one |

> **Only the editor repairs, and only the editor counts.** A completely green
> command-line gate does not prove that the file opens clean.

**The method that answered, after three wrong hypotheses:** ask dB to open, click
`OK` and **save** — and then `diff` between what KiCad saved and what was there.
The diff is KiCad saying literally what it repaired. Save the copy BEFORE sending
it to be opened, otherwise the diff mixes your edits with its own.

**And the generator has to reproduce what KiCad writes**, otherwise the next
regeneration erases the repair and the box comes back. Acceptance criterion:
generate to a copy and compare the `(instances)` blocks with those of the file
KiCad saved — **equal, one by one**.

## 7f. THE MPN IS A CLAIM, NOT A FACT

No checker catches a wrong MPN. The ERC stays clean, the netlist stays clean, the
footprint may even exist — and the part that arrives is not the part the drawing
assumes. **The schematic stays equally clean with the wrong part number.**

And what fails is almost always **the suffix**, not the family:

| chosen | what the datasheet said |
|---|---|
| `PTV09A-4020**F**-B103` | `F` = smooth shaft; the note next to it asked for knurled (`S`) |
| `PEC11R` | pins at **5.0 mm**; KiCad's footprint is the Alps's, at **2.5** |
| `2920L300/24MR` | does not exist; what there is at 24 V is the 3.3 A |

> **Every MPN enters the BOM with the datasheet line that justifies each
> character of the suffix.** Without that it is a family with a number.

⚠️ **A search summary is not a datasheet.** A summary says what someone
understood. On 05/09 a summary said that a pot style "would not serve", and the
conclusion was reported and decided — until a distributor described the same part
another way and the contradiction forced a trip to the style table. The table
gave half the right to each side.

**A trick that worked:** when the datasheet PDF comes out scrambled as text (they
are drawings in columns), **render the page and LOOK** — `pdftoppm -png` and read
the image. The dimensions are in the drawing, not in the text flow.

**And stock is also a claim:** stock at LCSC **is not** JLC assembly stock. They
are different warehouses, and the Basic/Extended tier is confirmed in the JLC
parts library.

## 7d. MULTI-UNIT: a chip is ONE reference

A dual op-amp drawn as **two single-unit symbols** (`U1A`, `U1B`) plus a power
symbol (`U1C`) looks right in the drawing and is wrong in three ways at the same
time:

1. **Three BOM lines and three footprints** for a single chip.
2. The two amplifiers use **the same pin numbers** (1/2/3): amplifier B ends up
   in A's package.
3. The references do not end in a digit (§6b).

The right thing is **one multi-unit symbol**: reference `U1`, units 1, 2, 3.

**The pinout is taken from KiCad's official library, not from memory.** The base
symbol of the SOIC-8 duals is the `LM2904` from `Amplifier_Operational.kicad_sym`
— from which `OPA1602`, `OPA1612AxD`, `OPA2134`, `LM4562` and `NE5532` all
inherit via `extends`:

| | amp A (u1) | amp B (u2) | power (u3) |
|---|---|---|---|
| output | **1** | **7** | — |
| −IN | 2 | 6 | — |
| +IN | 3 | 5 | — |
| V− / V+ | — | — | 4 / 8 |

⚠️ Five independent duals agreeing is **strong evidence, it is not the datasheet
of the part you are going to buy**. It remains to be confirmed in the datasheet
before submitting.

**And the error this uncovered:** in the home-made symbol pin **1** was in the
position of the `+IN` and pin **3** in the output's — **swapped**. On a
fabricated board, the input net would be soldered to the chip's output pin. It
did not show up in the prototype, because that one is wired by hand and never
went through the package's pin mapping.

> **The prototype that sounds good does not validate the symbol's pinout.** It
> validates the topology. The pin numbering is only exercised when the factory
> solders.

**When swapping one symbol for another, keep the GEOMETRY of the pins** (here
±10.16 and not the ±7.62 of the official library): the wires already touch those
points, and moving the pin breaks connections that were correct. Only the
**numbers** change.

**Acceptance criterion of the swap:** the netlist has to come out identical
*after applying the rename map and the pin map*. You compare the **membership**
of each net — the set of `(reference, pin)` — never the net names, which change
on purpose.

```
so ANTES : [('C11','1'), ('R10','1'), ('RV1','1'), ('U1','3')]
so DEPOIS: [('C11','1'), ('R0A','1'), ('RV1','1'), ('U1','3')]
```
It was this comparison that caught the `R0A` of §2d. **Without it, a resistor
with the name KiCad was going to renumber would have gone to the factory.**

⚠️ `Net-(U1A-+)` in a netlist with a multi-unit symbol **is not a remnant of the
old name**: it is KiCad writing "U1, unit A". It is a sign that it works.

## 7g. THE FIELD NOBODY CONSUMES IS NOT VERIFIED

I stamped `MPN` on 83 of the 84 symbols. The outside review said **«50/50 parts
have MPNs»**. I wrote 99% in the README. The three things were true, and the
field had `MPN = "header 1x4 2,54"` inside it — I stamped the BOM's **description
column**.

> **A fill checker measures fill.** It never promised anything else. And the
> count goes up **exactly** when the junk comes in.

It only showed up when something tried to USE the value — the datasheet
downloader went to fetch `header 1x4 2,54` from LCSC. ERC, netlist, count and
external review all passed, because none consumed the field.

> **Before believing a new field, write its consumer.** The first tool that reads
> it seriously is the first test it undergoes. Without a consumer, the field is
> not verified — it is unpremiered.

⚠️ **And the empty is an answer.** A 0805 1% resistor has no MPN: it has a value
and a package. Filling it so the column looks full tears down the column's only
useful property. When the MPN became its own field and the generics were left
empty, the coverage fell from **99% to 43%** on the same drawing.

> **A metric that goes up when you fill and down when you clean is measuring the
> fill.** The denominator is easy; it is the numerator that is work.

## 7h. THE DATASHEET IS CHECKED FROM THE INSIDE, AND WITH AN OUTSIDE TOOL

A PDF called `OPA1642AIDR.pdf` can have anything in it. Distributors point to
family datasheets, redirect to error pages, and serve HTML with a `.pdf`
extension. **The file name is proof of nothing.**

Each datasheet goes through `pdftotext` — which I did not write and which may say
no — and the number has to appear in the text. **Five rungs, and all five are
needed:**

| rung | means |
|---|---|
| **OK** | the complete MPN is in the text |
| **FAMILY** | datasheet of the right series; the suffix is assembled from the ordering table — **requires a citation** |
| **IN THE CATALOG** | no datasheet of its own, but the number is in a series catalog that is already here |
| **SUSPECT** | the file does not speak of the part — **locks the gate** |
| **NO DATASHEET** | not published; and the line says **which manufacturer and which document** |

⚠️ **FAMILY is not SUSPECT and is not OK.** Bourns publishes one datasheet for
the whole 3296 series; calling it suspect accuses the good document, promoting it
to OK gives the suffix as proven without reading it. It is its own rung, and it
forces the citation to exist.

⚠️ **Searching the text does not prove the right paper.** The `BAV99,215`
datasheet has a `215` — and that is **215 mA** of current, not the packaging
code. The ordering table only lists `BAV99`, and the document says `Packing
information removed.`. So the citation keeps **two** things: what the datasheet
proves **and what it does not prove**. The second is the one that gets lost.

⚠️ **And the other side of the same error: the suffix can mean the OPPOSITE of
what you assume.** I was going to write *«LF = lead-free»* on the
`3296W-1-104LF`. Bourns's own legend says `Blank: Lead-free product compliant
with RoHS` and **`LF: High-end product`** — the blank is the one that is already
lead-free, and the `LF` is something else.

> The part was right and its **reason** was wrong, which is the hardest state to
> catch: nothing fails, nothing accuses, and the next person to choose a part
> from that family inherits the false premise. **Reading the suffix legend costs
> a minute and is the difference between having the right number and knowing
> why.**

⚠️ **And there are suffixes that the product datasheet NEVER justifies**, by the
manufacturer's decision. The `BAV99`'s says, in so many words, `Packing
information removed.` — the `,215` is not proven there no matter how much you
reread it. That is not a failure to search: it is the wrong document for the
question, and the citation has to say so instead of staying blank.

⚠️ **And the flattening removes only punctuation.** The catalogs write
`EEUFR1C101` without hyphens and `B2B-XH-A(LF)(SN)` with the code stuck together
— thirteen «does not exist» that were thirteen punctuation differences. But never
prefixes, never letters: `EEUFR1E471B` is another part. It is proven with the
whole list — *N MPNs give N distinct forms, zero clashes* — and the proof goes in
the comment.

⚠️ **Read the WHOLE document.** `pdftotext -l 6` marked the `OPA1642AIDR` suspect
in a datasheet where the MPN appears three times: TI's ordering table lives at
the end. **A false rejection is the shortest path to the checker being turned
off.**

⚠️ **A mode that knows less does NOT WRITE.** The `--confere` does not go to the
network, so it does not know whether the distributor has the part — only that the
file is not here. Letting it write the index made each pass through the gate swap
*«LCSC does not have this part · Phoenix Contact, article 1715721»* for *«not on
disk»*. A checker silently degrading the document it checks. The proof of the fix
is the **md5 equal before and after**.

## 7i. A FIX IN A PLUGIN'S CACHE IS NOT A FIX

The `kicad-happy` 2.2.1 downloader failed on all 19 parts, and the cause was not
my list: **the `jlcsearch` API stopped serving the `datasheet` field.** It finds
the part — exact match on the MPN, with stock and price — and returns the field
empty. The plugin treats «found but no URL» as the end of the line and never
reaches the second path it has itself (`wmsc.lcsc.com` by the LCSC code).

Editing `~/.claude/plugins/cache/.../2.2.1/` would solve it in two minutes. You
do not do it:

> **It is someone else's working copy, and the next update erases the edit
> without warning.** A fix that lives in a place another process rewrites is not
> a fix — it is a trap for whoever comes later to see version 2.2.2 broken again,
> with the memory that «this was already resolved».

The outside tool still holds — it was it that showed the defect in my MPNs. What
changes is where the fix lives: **at home, with the comment saying which version
of the plugin failed and why.**

## 7j. THE MPN IS FOR HUMANS; THE DISTRIBUTOR CODE IS WHAT THE MACHINE READS

The factory **does not assemble a line without its code**, MPN or no MPN. So the
references without an MPN divide into two things that are not treated alike:

| | the empty | why |
|---|---|---|
| THT and modules | **right** | they are bought and soldered by hand |
| SMD passives | **defect** | the machine assembles them, and each needs a code |

A generic 0805 resistor has no MPN and **must** have a code — because to have a
code you have to choose a concrete part, and the factory's «Basic» parts are
concrete parts.

⚠️ **SMD is read from the footprint's ATTRIBUTE (`attr smd`), not from the library
name.** Deducing it from `R_0805` is right almost always and fails exactly in the
rare cases that matter.

### The distributor search lies with confidence

`4.7MΩ 0805` returns an **LED** in first place. `0Ω 0805` returns 1 kΩ.
`100Ω 1206` returns 3.3 kΩ. The ordering is by text similarity and the value does
not dominate.

> **The order of a search is not an answer.** The **descriptions** are the data:
> you ask for many, you analyze each one — value, package, class, tolerance, and
> the dielectric when the swap is **heard** — and the order only breaks ties
> among those that have already passed.

⚠️ **Compare values with relative tolerance, never `==`.** `22e-12` and
`22 * 1e-12` differ in the last bit, and the output — «0 of 35 pass» — reads as
«the catalog does not have this». A false negative with the face of a catalog
fact is worse than an error: nobody searches again for what has already been
given as nonexistent.

### The reference is unique WITHIN a board

A `ref → code` dictionary built over two boards **runs over itself**: each one
has its own `C1`, its own `R5`. Here the brain board's `C1` — a 2200 µF THT
electrolytic — took the code of a 100 n 0805.

> **The key is `(board, reference)`.** And you add a **second independent
> guard**: the recorded package has to appear in the footprint the BOM declares.
> If the key breaks again, it is the package that shouts —
> `CP_Radial_D16.0mm` does not contain `C_0805`.

## 7k. THE CPL AND THE BOM ARE TWO SOURCES, AND NOTHING FORCES THEM TO AGREE

The CPL comes from the **board**. The BOM comes from the **schematic**. And each
mark lives on one side only:

| mark | lives in | who obeys it |
|---|---|---|
| `(dnp yes)` | the **schematic**'s symbol | the BOM |
| `exclude_from_bom` | the **board**'s footprint | the CPL |

`pcb export pos --exclude-dnp` reads the **footprint**'s attribute. If the DNP
only exists in the symbols, the option is written and **does nothing** — and the
CPL orders placing parts the BOM does not buy. In the reverse direction, test
points with `exclude_from_bom` in the footprint come out in the BOM as purchase
lines for copper.

> **Neither of the two exporters is wrong**: each obeys its own source. Neither
> raises an error. **Only the comparison sees them.**

**Three pieces, and all three are needed:** carry the DNP to the board (with the
`pcbnew` API, which serializes what THIS version reads — the mark has already
changed shape between versions, and writing the token by hand fails silently on
the next one); take the test points out of the BOM **at the source, by the
`^TP\d+$` criterion and not by a list** (a fixed list protects exactly the names
that are already there); and **the gate comparing the designators of the two
files**, because the other two can fail.

⚠️ **When sweeping blocks, counting parentheses is right — but it skips strings.**
A schematic with notes like `100R serie (~30 mA)` or `(!)` desynchronizes the
depth and returns all blocks wrong, silently: here, zero DNP symbols in a file
with eight.

## 9b. THE SCHEMATIC COLLISION GATE

The `tools/svg_colisoes.py` returned **code 3 — NOT VERIFIABLE** on every
exported schematic, and the accepted explanation was "KiCad writes the letters as
paths". **It was a half-truth and the gate was blind:** KiCad ALSO writes a
`<text>` for each string, with `opacity="0"` and with **`textLength`** — the
exact width it computed itself. The tool discarded them by opacity (a rule meant
for decorative shading) and announced "not verifiable" on files with 336 texts
measured inside them.

- **If the file carries `textLength`, you do not estimate: you measure.** The
  estimate `n × body × 0.6` errs by **18 mm** on a 73-character note — enough to
  say "clean" over a text that crosses a whole potentiometer.
- The real advance of KiCad's stroke font, measured in the `textLength`, is
  **≈0.80–0.85 of the nominal body**, not 0.6.
- On schematics the `svg_colisoes` is **noisy** (each text comes out duplicated,
  and every label touches its own wire). For those the gate is the
  **`tools/colisoes_sch.py`**, which reads the `.kicad_sch`.

**The comparison rule of the `colisoes_sch`, and the exception that was missing:**

| text | against | counts? |
|---|---|---|
| note | everything | yes |
| reference / value | body of **its own** symbol | no — it is where they live |
| reference / value | body of **another** symbol | **yes, it is a defect** |
| reference / value | wire | no |

v0.4.1 compared **only the notes** against shapes, with the argument that
"a label stuck to its own wire is normal" — true for the wire, false for the body
of another component. The value `22p C0G` was drawn INSIDE the body of the `R10`,
the eye saw it and the tool said "0 collisions".

> **An exception too broad is blindness with an excuse.** Always calibrate the
> checker against a defect you have already confirmed with your eyes: if it does
> not catch it, it is the one that is wrong, not the eye.

And the corollary, which holds for both gates: **code 0 does not authorize
skipping the eye.** It authorizes looking while knowing where.


## 7e. THE EMPTY RECTANGLE IS NOT A DEFECT — it is the choice showing

In a hierarchical root, each sheet's rectangle draws the name, the file and
**the sheet pins**. If the project connects the sheets by **global labels** —
which is this one's choice — there are no sheet pins at all, and the rectangles
come out empty. Here: **0 sheet pins, 125 global labels**.

The cost of that choice is this: **the root stops saying what connects to what.**
Whoever opens the project sees four empty boxes. That is why the root's legend
declares the choice in text — without that declaration, the drawing looks
incomplete instead of deliberate.

> An empty rectangle **with** the choice declared in the legend: correct.
> An empty rectangle **without** it: indistinguishable from unfinished work.

## 9c. THE LEGEND PRINTS FROM THE BOTTOM UP

KiCad draws the `(comment N ...)` with the highest number HIGHER up. Numbering
1, 2, 3 in reading order prints the legend **backwards**:

```
;; file             ;; render (top to bottom)
(comment 1 "A")       C
(comment 2 "B")       B
(comment 3 "C")       A
```

> **The order in the file is the REVERSE of the reading order.** The first
> sentence to read carries the highest number.

It was like this on the four sheets with a legend, and it is only seen in the
render — no checker catches it, because each line is correct and in the right
place. It is the same family as §5b (the sign of `y`), with a different cause:
there it was my arithmetic, here it is KiCad's convention. **The common symptom —
text that reads backwards — is denounced only by the eye on the final render.**

⚠️ And the legend is DOCUMENTATION WITH AUTHORITY: the socket's claimed *"DPDT
sense on D11/D12"* months after the sense had been abandoned. When a decision
dies, the legend that repeats it dies with it — otherwise it re-teaches it to
whoever opens the file, which is exactly the one who has no way of knowing it is
wrong.

---

## 10. THE ENCLOSURE RULES — but only the enclosure that was CUT

The outline of a board and the position of the mechanical holes come from the
enclosure. The trap is **which enclosure**.

| artifact | what it is | counts? |
|---|---|---|
| dimensioned drawing | the intention | **no**, if there is a generator or a cut |
| cut file from an earlier generation | a real object — **another one** | **no** |
| **generator of the part being made** | the source | **yes** |
| the part in hand | the object, with real kerf and tolerance | rules over everything |

> **«The file that went to the factory» is not enough — it has to be that of the
> part being made NOW.** A real artifact from the earlier generation measures
> just as well as the right one and gives plausible, wrong numbers.

In the Cotucatronics there were **four** sources for the same enclosure, with
four thicknesses (6 in the drawings, 6.4 in the cut walnut, 6.0 in the generator,
**3.0 in the ordered material**), and the board had been dimensioned against the
first.

**What to do, and it is not to update the dimensions by hand:**

1. **Import the generator**, if it has `if __name__ == "__main__"` — it is
   stronger than measuring the SVG it emits, because there is no parsing between
   the dimension and the use, so there is no place where the two diverge. Only if
   there is no generator do you measure the SVG, and then with a count guard
   (§2c).
1b. **Better than following the dimension: build so that it does not enter the
   calculation.** Centering the board on the plate that holds it made the
   mounting holes always fall in the same place *relative to the board*, for any
   thickness — confirmed by running the generator at `T=3` and `T=6`. When an
   external number will not stop changing, the fix is not to follow it.
2. **A gate before writing**: does the board fit? did any mounting hole end up
   under a component? A covered hole is not a tight hole — it is a screw that does
   not go in with the enclosure closed and the soldering done.
3. **No hand-written dimension survives** alongside the reading. See the rule
   below, which is the reason the divergence lasted.

### 10a. A constant nobody reads cannot be wrong out loud

`INTERIOR = (188.0, 108.0)` was declared and **was never consulted by any line**.
For a number to raise an error, someone has to read it.

> A dimension next to a measured thing ages like a list next to a generated thing
> (§ of the embedded list). **The cure is not to update — it is to derive and
> refuse.**

Corollary for review: when reading a generator, `grep` each declared constant by
its own name. **Zero uses is a finding**, even if the value is right today.

### 10b. A comment with a revoked premise re-teaches the error

When the enclosure grew, «the enclosure only has 28 mm free at the front» — the
reason written for two placement decisions (there are 43) — became false, and the
decisions stayed right for *another* reason. A comment with authority that
asserts a revoked premise has the gravity of code: the next reader decides with
it.

### 10c. A divergence announced without measuring accuses the wrong part half the time

The enclosure had three arcades and the drawing two; I was going to announce that
the **schematic** was out of date. The schematic already had three. The one that
was wrong was the drawing — again. **Measure before announcing which side the
lack is on.**
## 11-12. LAYOUT AND ROUTING → `kicad_layout` skill

The layout chapter **left here on 08/09/2026**. It was 1033 of 2022 lines — more
than half — and a skill that is not read before being used is not a skill, it is
an archive.

**It now lives in `.claude/skills/kicad_layout/`**, and covers: planes and copper
on both layers, what no DRC sees, measuring per net, the silkscreen (collisions,
designators that point to the right part, labels and legends), and routing by
script.

⚠️⚠️ **AND IT IS MANDATORY AT THE CLOSE OF ANY BOARD.** It is not «consult if in
doubt»: it is **running the `kicad_layout` before giving a board as closed** —
before generating gerbers, before ordering, before saying it is ready. A clean
schematic proves that the drawing is right; **it proves nothing about the
copper**, and the two things have already been confused here.

```
proven schematic (this skill)  →  kicad_layout  →  para_fabrica  →  order
```

⚠️ **Pointing is not running.** This section exists so that nobody closes a board
with the `kicad` alone — if you are closing, the `kicad_layout` is the next step
and it is not optional.

## Provenance

This skill absorbed, on 01/09, the old `esquematico`, which stopped existing by
dB's decision: **one skill per subject; two saying almost the same thing drift,
and then nobody knows which one rules.**

From it came §4b (the crossings family, with C1/C2/C3 named), the junction-dot
semantics that opens §4, §5 (grid before emitting and generate by script), §7b
(what every schematic declares) and §7c (the contract with the BOM).

The rest was born from a session in which **two sheets that did not open** were
delivered with the net checker clean, and in which an edit was re-inspected
visually **without repeating the ERC** — precisely the defect the `esquematico`
could not foresee, because it was discovered after it.

On 05/09 came §2b, §2c, §2d, §5b, §6b, §6c, §7d, §7e, §7f, §9b and §9c, from a
session preparing the files for JLCPCB. The thread that links them is a single
one: **tools that silently lost lines and therefore approved everything** — four
generators skipping two pins written in scientific notation, a contract checker
saying "the 26 match" having read 24, and the collision checker saying "0" blind
to 40 properties. What caught them was not reading the code: it was **comparing
what the tool read with what existed**, and comparing the netlist with the
earlier one.
