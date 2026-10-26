# Technical drawings
<!-- doc-check: ignore build_sheet (name of an SVG in docs/operacao/esquematicos/, not a symbol from src/.) -->
<!-- doc-check: ignore svg_colisoes (the tool lives in `tools/`, and check_docs only scans `src/`. Not a ghost: it is a limit of its reach.) -->
<!-- doc-check: ignore cotuca_energia_previa (drawing RETIRED on 05/09. The tombstone stays: it is what keeps someone from regenerating it without reading why it left.) -->
<!-- doc-check: ignore energia_preview (same) -->
<!-- doc-check: ignore preamp_preview (same) -->

**One folder, and one rule that governs it entirely:**

> **When an artifact gains a source, its earlier drawings either become generated
> from that source, or they die.**
>
> A drawing that describes what the source no longer says is **worse than
> none**: it looks official and lies.

## How it is kept up to date — and not by memory

```bash
python3 tools/desenhos.py            # checks; exits != 0 if any is old
python3 tools/desenhos.py --gerar    # regenerates and checks
```

The criterion is mechanical and asks for no opinion: **if the source is newer
than the drawing, the drawing is old.** You do not ask whether what matters
changed — that question is where the discipline is lost.

## What is here

### `esquematicos/` — **generated**, not edited

| drawing | source |
|---|---|
| `cotuca_energia.svg` | `kicad/cotuca_cerebro/cotuca_energia.kicad_sch` |
| `cotuca_preamp.svg` | `kicad/cotuca_cerebro/cotuca_preamp.kicad_sch` |
| `cotuca_soquete.svg` | `kicad/cotuca_cerebro/cotuca_soquete.kicad_sch` |

They come out of the same file the factory will read, so **they cannot diverge —
they have no life of their own.**

### `caixa/` — **generated**, except page 1

Six pages of the enclosure's fabrication drawing, by two generators that run
from here:



⚠️ **The dimensions marked "confirm" change after the parts are measured — and
they change in the `.py`, never in the SVG.** An SVG edited by hand diverges from
the generator on the first regeneration, and then nobody knows which one rules.

⚠️ **The 6 enclosure fabrication pages were deleted on 06/09.** They drew an
enclosure that was never cut. The enclosure's artifact is now
`caixa/cotuca_caixa_corte.py`, which generates the cut file from
`--t`/`--kerf` and carries its own gates — it does not pass through this
watchman.

Unlike the schematics, these **pass `svg_colisoes`** — they are real `<text>` —
and the generator itself runs the gate on every page, with an `assert` at the
end: with collisions, it does not deliver.

### ⚠️ What is lost in that trade, and it is honest to say so

These SVGs **do not pass the `svg_colisoes.py` gate**: `kicad-cli` draws the
letters as *paths*, not as `<text>`, and the tool answers
`NAO VERIFICAVEL — nenhum <text> encontrado`, code **3** — which is *"I couldn't
look"*, not *"it's clean"*.

We traded **"it can diverge but can be checked"** for **"it cannot diverge but
cannot be checked"**. For a documentation figure it is the right trade: the
truth of the drawing is worth more than the guarantee of no overlap, and the
overlap has another gate upstream — the ERC. But it is left **declared**, not
presumed.

The inspection of these is by eye, in slices, as the `design-review` §3 requires.

## What died, and why

**`preamp-piezo.svg`**, retired on 05/09. It was the figure for conversation from
before a schematic existed. When the pre-amp gained a `.kicad_sch`, it came to
describe a circuit that is not the board's — it said `C2 1 nF` and showed `RV1`
live, when the schematic says `C2A 20n` and `RVA` has been **DNP** ever since
the bench approved the follower. It was not fixed: it was retired. It is still
in git.

**The three generator previews** (`cotuca_energia_previa`, `energia_preview`,
`preamp_preview`), removed the same day: output of a path that stopped being the
source when the KiCad project came in.

## Still outside here, and undecided

Drawings that **have no source** and therefore are not generated — they remain to
be judged one by one, with the question that is worth more than the fix: **what
is this still good for?**

- `docs/operacao/esquematicos/piezo_v2_protecao.svg` — the piezo's protection
  **already lives** in `cotuca_preamp.kicad_sch` (D1 BAV99, R1, R2). By this
  folder's rule, it is a candidate to die.
- `cotucatronics_esquematicos_v3.svg` — superseded by `v4`, cited by nobody.
- `cotucatronics_esquematicos_v4.svg` · `cotucatronics_build_sheet.svg` —
  cited by nobody. The `build_sheet` has no KiCad equivalent, so it survives the
  rule — but not necessarily the question.

## Already dead

| drawing | when | why |
|---|---|---|
| `preamp-piezo.svg` | 05/09 | described the pre-amp from before the `.kicad_sch` — `C2 1 nF` and `RV1` live, against `20n` and `DNP` |
| `caixa-pedal-4-faces.svg` · `caixa-pedal-layout.svg` | 05/09 | the two earlier versions of the enclosure, replaced by the six pages |
| `cotuca_energia_previa.svg` · `energia_preview.svg` · `preamp_preview.svg` | 05/09 | previews of the `gen_*.py`, which stopped being the source |

All are still in git.
