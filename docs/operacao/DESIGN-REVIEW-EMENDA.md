> ⚠️ **ABSORBED. The skill rules.** This text was integrated into
> `.claude/skills/design-review/SKILL.md` on 05/09, as its **step 1** —
> not as an appendix, so the order would not be ambiguous. It stays here as a
> record of what was discovered and the proof that motivated it; **if the two
> diverge, the skill is right and this is stale.**
>
> The tool lives in `tools/svg_colisoes.py` — the current version is in the
> README map, and not here, so there aren't two places aging.

# design-review — amendment 05/09: the gate that can say no

## What failed

Four deliverables with "clean visual review" had overlaps. Two were
visible to the eye and passed because the eye looked at the whole and not the
slice; two were half-a-pixel ones — text touching the stroke of a cross, a label crossing a
gray dashed line — invisible in any render, and defects even so.

**The eye is the wrong instrument for fine overlap.** It serves for
composition, legibility, hierarchy. It does not serve for "does this text touch this
line?", which is a question of geometry and has an exact answer.

## The new rule, and it comes BEFORE the eye

> **Every SVG delivered passes through `svg_colisoes.py` and comes out with ZERO collisions.
> Only then does the visual inspection begin — and it now looks at composition, not
> touching.**

`svg_colisoes.py file.svg` computes the bounding box of each text and tests it against:
- **text × text** — always a defect
- **text × segment** — real geometry (Liang–Barsky), not the bbox of a path; an
  L-shaped path does not flag the text sitting in its empty corner
- **text × shape border** — text INSIDE a rect/circle is a label and is
  legitimate; text **crossing** the border is a defect
- **shapes with `opacity < 0.5`** are decorative shading and do not count

It exits with code ≠ 0 if there is a collision. That is what makes it a gate: it can say
no, and a script that calls it stops.

**Text width is estimated** (0.55 em per character, Helvetica). It is
conservative — it flags a few pixels before the real touch. That is the
right behavior for a gate: a false positive costs moving a label; a false negative
costs a wrong deliverable.

## How it becomes automatic

1. **In the generators:** the `.py` that writes the SVG calls the checker before
   copying to `outputs/`; with collisions, it writes to `/tmp` and refuses to deliver.
   An `assert` at the end of the generator.
2. **In the skill:** step 1 of any SVG review is to run the tool and
   paste the result line into the report — `N texts, M segments, 0
   collisions`. Without that line, the review did not happen.
3. **For what is not SVG** (plot PNG, PDF, KiCad render): the tool
   does not reach it. There the rule is the one that already existed — slice into regions and look at each
   one — **plus** today's lesson: looking at the whole does not replace looking at the slice,
   because the whole hides exactly what is small. For KiCad, the gate is
   `kicad-cli sch export svg` followed by this same tool.

## The order, fixed

```
generate → svg_colisoes (0) → slice and look (composition) → deliver
```

Any edit after any of the three restarts all three.

## Today's result, as proof

| file | before | after |
|---|---|---|
| caixa-fabricacao.svg | 30 flags (2 real visible, 4 near-touches, 24 false from trapezoid) | **0**, tool fixed for containers |
| preamp-piezo.svg | 3 near-touches the eye never saw in three reviews | **0** |
| `caixa-pedal-4-faces.svg` *(retired 05/09)* | 2 false from shading | **0**, with the opacity filter |
| `caixa-pedal-layout.svg` *(retired 05/09)* | 1 false from shading | **0** |

The tool caught the defect that dB saw **and** three that nobody had seen.
