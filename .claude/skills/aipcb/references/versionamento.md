# versionamento — three layers, one golden rule (freeze-at-release)

> **What changes the fabricated artifact has to show up on the board.** A package
> that ships with the silk saying one revision and the gerber saying another is a
> package that lies about itself — and it is the factory, or dB with the board in
> hand, who pays for the lie.
>
> **⚠️ FREEZE-AT-RELEASE (2026-09-12).** The rev does NOT go up on every commit that
> touches the gerber. It goes up **per RELEASE**: while a board is not **fabricated /
> ordered**, it stays at the **same rev**, and the pre-fabrication iterations **share
> that rev** — they are distinguished by the package's **commit stamp**, not by a new
> number. The number/letter only goes up when a board is **actually FABRICATED/ORDERED**;
> at that point that rev is frozen and the **next** gerber change opens the following rev.

This project versions in **three independent layers**, each answering a different
question. Confusing them is the error: bumping the semver of a script is not
bumping the revision of the board, and stamping a commit is not saying the copper
changed.

## The three layers

### 1. Board rev, on the silk — `rev <LETTER><N>`, PER BOARD

Lives on the silkscreen of the board itself (`gr_text "cerebro rev C"`,
`gr_text "painel rev C"`), written by the generator (`kicad/pcb_contorno.py`,
function `marca`). It is the identity of the **physical artifact**.

- **Letter** = **generation**. It is the "open" rev while the board has not been
  fabricated. All the pre-fabrication iterations (fixes, repositionings, silk or
  copper changes) **stay at this same letter** — the silk says `rev C`, with no
  number, and what separates them is the package's **commit stamp**.
- **When it goes up** = **only at release**. As soon as a `rev C` board is
  **fabricated/ordered**, that rev is **frozen** (the physical artifact that
  exists in the world is `rev C`); the **next** change that alters the
  gerber/drill opens the **following** rev (`rev D`). The bump is the decision to
  fabricate, not just any commit.
- **Number** = optional, and only makes sense to distinguish **fabricated spins**
  within a generation; while there is no fabrication, no number is used (`rev C`,
  not `rev C1`). The important thing is: **there is no new number per commit.**
- It is **PER BOARD**. The brain board and the panel board evolve independently:
  one may already have been fabricated and be frozen while the other is still
  open — each carries its own truth.

### 2. Package stamp — the commit's short SHA, in `export/`

Each factory package lives in `export/<placa>_<data>_<commit>/`, and the
`<commit>` is the git **short SHA** in the state the package came out of
(`para_fabrica` stamps `git rev-parse --short HEAD` + the date + the dirty-tree
warning). It is the **provenance**: it says from exactly which point of the repo
the gerbers were generated, and it is recoverable from the PR. Two boards with
the same silk rev can have different stamps if something around them changed —
the stamp is of the **package**, the rev is of the **board**.

### 3. Script semver — the pipeline

Each tool of the pipeline (`para_fabrica`, `pcb_contorno`, `faz_placa`,
`kit_montagem`, …) carries `VERSAO = "X.Y.Z"` and bumps per `edicao-segura`
(MAJOR incompatible · MINOR compatible addition · PATCH fix with no behavior
change). It is the version of the **method**, not of the board. Improving the
generator does not bump the board rev; it only bumps if the generator's output
**changes the gerber**.

## The golden rule

> **The rev of a FABRICATED artifact has to be traceable — but per RELEASE,
> not per commit.**
> **While the board has not been fabricated/ordered, the gerber can change N times
> and the rev STAYS** (the iterations are distinguished by the package's commit stamp).
> **The rev only goes up when a board is actually fabricated/ordered: at that point
> it freezes, and the next gerber change opens the rev after it.**

What still holds: a package can never say one thing on the silk and another in the
gerber. What CHANGED: "the gerber changed" no longer **forces** the number up — it
only forces a new **commit stamp**, within the same rev, until there is a
fabrication.

Cases, so it is not left to faith:

| change | gerber/drill changes? | board already fabricated at this rev? | board rev | stamp | script semver |
|---|---|---|---|---|---|
| fix copper/silk on a board **not yet fabricated** | yes | no | **stays** (same letter) | **new** | of the generator, if it changed |
| move/scale the mark on a board not yet fabricated | **yes** (silk is gerber) | no | **stays** | **new** | of the generator, if it changed |
| first gerber change **after** the board has been fabricated | yes | yes (frozen) | **goes up** (next letter) | new | — |
| group the BOM by LCSC, write rich CPL | no | — | **stays** | new (it is another commit) | goes up |
| fix a comment/doc, a test, a skill | no | — | **stays** | (generates no package) | goes up |
| re-fill zones without changing geometry | no | — | **stays** | new if re-packaged | — |

⚠️ **Silk IS gerber** — moving/adding/deleting silkscreen changes the `.GTO`/`.GBO`
that the factory prints. But under freeze-at-release this does **not** bump the rev
while the board has not been fabricated: it generates a **new package (new stamp),
same rev**. Example: the brain board and the panel board both stayed at **`rev C`**
across several pre-fabrication iterations (reordering the panel's J2, repositioning +
scaling the brain board's mark) — each with its own commit stamp, all `rev C`,
because no board had yet been sent to be cut. The copper was proven identical
(identical copper/net fingerprint, 0 changes) in the iterations that only touched silk.

## Where the history lives

The **revision history** — each rev, what changed, and the commit — lives in
**`docs/relatorio/002.md`**, in the «Revision history» section. One line per bump,
in the form **rev → what changed → commit**. That is where you read each board's
saga; the silk says *which* rev, 002 says *why* and *from where*.

## Part of Stage 8 / of the gate

Emitting a package **asserts** that the silk rev matches the board's **fabrication
state**:

- While the board **has not been fabricated**, iterating the gerber is normal and
  does **not** bump the rev: a **new package (new stamp), same rev** is emitted.
  What tracks the iteration is the commit stamp, not a number. `para_fabrica` does
  not run without the risk record at GO, and the human gate (`portao-humano.md`)
  opens with provenance from the LEIA-ME: it is checked, item by item, that the rev
  stamped on the package and the rev printed on the silk are the **same** — and that
  this is the board's **open** rev.
- **At the moment of fabricating/ordering**, the open rev freezes: that is the rev
  of the physical artifact that will exist. The **next** gerber change, after that,
  **has** to bump the rev — otherwise a new package would be reusing the rev of a
  board that is already cut, and two different physical artifacts would share the
  same identity.

The question Stage 8 asks before letting you pay: **«has this rev already been
fabricated? If so, this gerber has to carry the next rev; if not, the open rev stays
and what distinguishes this package is the stamp.»** If the answer does not match the
silk, the stage does not close.

## Version EVERYTHING — every generated artifact carries a stamp (2026-09-12)

The three layers above cover the **board**, the **package** and the **script**. The
generalization was missing: **every artifact that leaves the repo to be consumed out
there** — not just factory packages — has to **declare its own version**, so that no
one consumes an old copy without realizing it.

> **Root of the problem (2026-09-12):** dB's Mouser cart ended up with an MPN **from
> a superseded export**, while the repo already had the new one. Neither side said
> "I am version X" in a comparable way — and an artifact that does not version itself
> gives no way to detect that it has aged. An export that is not stamped is a package
> that lies about itself, exactly like the silk that disagrees with the gerber.

**The rule, in three parts:**

1. **Stamp in the artifact itself.** Every generated file carries, at the start and
   in a place valid for its format, a stamp:

   ```
   carimbo: <arquivo> v<N> · <YYYY-MM-DD> · placas: painel rev C · cerebro rev C
   ```

   - **CSV** → first line as a `#` comment.
   - **Markdown** → HTML comment `<!-- ... -->` at the top (or a visible block).
   - **xlsx/xls** → a **dedicated cell** (`carimbo` on its own tab and/or a cell
     outside the columns the tool imports — never in column A of the data, or it
     becomes a phantom "part" on import).
   - **Factory package** → already does it via the `LEIA-ME.txt` (generator + date + commit).

   The `v<N>` starts at `v1` and **goes up on each new cut** that changes the
   consumable content (a swapped MPN, a recalculated qty). It is not the SHA of this
   commit (impossible self-reference) — the fine **provenance** comes from the
   MANIFEST + git.

2. **The MANIFEST is the single source of "the newest".** `export/MANIFEST.md` lists
   **every** current deliverable with its stamp. It is where you answer "am I on the
   current version?": you compare the stamp of the artifact in hand against the
   MANIFEST; if it does not match, `git pull`.

3. **Delivery/consumption protocol.** When **I deliver** a file, **I declare the
   stamp** ("this is `COMPRA_MOUSER.csv` v2, 2026-09-12"). When **the human is about
   to consume** (upload to Mouser, send to the factory, assemble), **they check the
   stamp** against the MANIFEST **before** acting. The two halves exist because a
   tool's output does not prove the input was current — the stamp does.

It is the same truth as the three layers, extended: **freeze-at-release** says that
the board rev only goes up at fabrication; **Version EVERYTHING** says that *any*
consumable artifact is stamped and checked against the MANIFEST before it becomes a
purchase, upload, or solder.
