# `caixa/` — the physical artifact, alongside `kicad/`

The two artifacts of this project are the board and the enclosure. They live at the same level.

**Wedge enclosure, 200 × 120, front 55 and back 80, in 3.0 mm clear acrylic.**
Welded shell (lid + 4 walls, joined by hand); **removable bottom**,
which is how it opens — and it is on the bottom that the carrier and the amp mount.

| file | what it is |
|---|---|
| `cotuca_caixa_corte.py` | **THE SOURCE.** Generates the cut from `--t` and `--kerf`. It has `if __name__ == "__main__"`, so `kicad/pcb_contorno.py` **imports it** to size the boards |
| `cotuca_caixa_20260906_r3_acrilico_3p0_037a9c30.svg` · `.dxf` | what goes to Ponoko |

**20 parts.** The twentieth is the lid **REINFORCEMENT** — a 193 × 50 strip glued
underneath, only where the arcade buttons are. A foot is ~300 N at the middle of a span, and 3 mm of
acrylic cracks **from the ø30 holes**. It becomes an effective 6 mm where the foot presses
and 3 mm where the pots pass through — which is the reason the lid is 3 (the PTV09 has
~7 mm of thread). It costs **+$4.52**.

✅ The reinforcement holes are **ø30 and concentric** with the lid's, confirmed
by measurement in the generated file. The arcade button mounts in a 6 mm panel — proven
by the **wooden prototype, which was 6 mm** and where the buttons tightened down.

The three arcade buttons are at the **maximum spacing the face allows**: x = 30 · 100 ·
170, **70 mm between axes and 40 between edges**. If a wide foot still presses
two at once, the fix is a ridge — see the assembly document, §4c.
| `CAIXA-ACRILICO-MONTAGEM.md` | materials, solvent, standoffs, assembly order, cable routes |

```bash
python3 caixa/cotuca_caixa_corte.py --reforco-furo 30
python3 tools/nesting.py caixa/cotuca_caixa_20260906_r3_acrilico_3p0_037a9c30.svg --pecas=20
```

## The name is the identity

```
cotuca_caixa_20260906_r3_acrilico_3p0_037a9c30.svg
             │        │       │   │   └── WHICH ONE — digest of the drawing
             │        │       │   └────── thickness
             │        │       └────────── material
             │        └────────────────── intent — revision
             └─────────────────────────── WHICH COMES NEXT — revision date
```

⚠️ **A digest identifies and does not order.** The date comes first so that `ls` comes out
chronological, and it is the **revision's** date, not the run's — generating twice gives the same
file. `revisoes.json` stores `revision → digest` and the generator **refuses to
write** if the same revision yields different geometry.

⚠️ **The revision is bumped by hand and the hand forgets; the digest does not.** Different
geometry is a different file **by construction** — the enclosure with reinforcement and the
enclosure without once shared the name `..._acrilico_3p0.svg`, one already ordered and the other
not, indistinguishable in the folder. It will not happen again.

| revision | what it is | order | where it is |
|---|---|---|---|
| r1 | 19 parts · arcade at 40/100/160 · no reinforcement | **#1263497**, $35.66 | `git show cbc6fbd:caixa/cotuca_caixa_corte_acrilico_3p0.svg` |
| r2 | 20 parts · arcade at 30/100/170 · lid reinforcement | 06/09, $38.23 — delivery 15/09 | `git show 6a86006:` |
| **r3** | + **USB openings** in the back wall, requested by the board | — | in the folder |

⚠️ **The r2 order was placed with the file under its OLD name**
(`cotuca_caixa_corte_acrilico_3p0.svg`), because it was sent before the naming rule
existed. **The geometry is r2's** — Ponoko measures **374.10 × 323.00**, which
is exactly r2's bbox (r1 measured 372.00 × 296.55). It is written here
because the name on the invoice no longer exists in the repository, and without this no one connects
the enclosure that arrives to the drawing that made it.

r1 was born before this rule and was named `..._corte_acrilico_3p0.svg` — the
same name r2 had until now. It is the case that created the rule.

⚠️ **`--reforco-furo` is mandatory and has no default.** The lid reinforcement
thickens the panel in the arcade strip, and the hole diameter decides whether the button
mounts in 6 mm or in 3. **Measure on the arcade button already bought**: thread length
and the nut's outer diameter. The generator refuses to run without the choice and
says what to ask for.

`_previa.svg` comes out of the same command, with thick strokes, just to look at — it does not
go into the repository, it is regenerated.

## What the generator guarantees, and what it does not

Three gates of its own, all **refusing before writing**:

1. **nesting** — everything inside the sheet, nothing overlapping. `tools/nesting.py`
   confirms from the outside, and counts the parts: a part that was not emitted collides
   with nothing, and a file with 18 passes as one with 19.
2. **mirror** — a rotated part that was actually mirrored cuts perfectly and
   assembles backwards. It is checked by the signed area.
3. **edge margin** — 3 mm. ⚠️ **Our rule**: Ponoko says "do not use the
   raw edge of the sheet" and publishes no number.

Current state: **20 parts, occupies 377.1 × 326.0 on a P2, margin 6.9 mm,
≈ 6.84 m of cut.**

## Cost — measured, not assumed

**Ponoko's price follows the cut length: ≈ $5.87/m.** Two real invoices,
same material, predict each other to within **$0.02** of error on the cut and
**$3.52** on area. `Setup $0.00`, no material line — **there is no sheet
price**; P1/P2/P3 only matters for fitting.

| | meters | cost |
|---|---|---|
| plain rectangles (the floor) | 4.07 | $23.89 |
| teeth, holes and slots | 2.00 | $11.74 |
| — of these, the **12 standoffs** | 0.70 | **$4.11** |
| **the tight nesting** | **0.00** | **$0.00** |

⚠️ **Nesting saves nothing.** The cut is a property of the parts, not of the
arrangement — the same 6.07 m before and after. It is done anyway, because of the
upload. The $1.90 between the two orders came from **regenerating at 3.0 instead of
6.4**, which shortened the cut from 6.39 to 6.07 m.

Design to save: **fewer parts and fewer small parts** (cost follows the
perimeter), wider teeth via the `n_target` of `cells()`, a smaller
enclosure — and above all **shipping, $22.43 out of $35.66, which is 39% of the subtotal**:
several enclosures in one package is the biggest lever of all, and it does not touch the design.

Detail and what remains to confirm: skill `/caixa`, §6.

## ⚠️ What was deleted on 06/09, and why

Dead: the 6 fabrication pages (`medidas/`), their two generators
(`gen_caixa*.py`), and the 6.4 walnut cut with its `.dxf`, `.png`,
`CAIXA-V1-NOGUEIRA.md` and `ponoko_nogueira/`.

The pages drew an enclosure that **was never cut**: 6 mm walls and an 8 mm
lid, a 40 mm front, two arcade buttons instead of three, and a powerbank cradle that does not
exist. The walnut cut was **real** — it went to Ponoko — but it is **another
enclosure**.

> **A superseded source beside the right one measures just as well.** The only thing
> that tells them apart is a check that no one runs before trusting. That was
> exactly how the two PCBs were sized against the wrong enclosure,
> twice in a row — scar **15cf**.

It is all in the history:

```bash
git show 94066fd:caixa/medidas/caixa-fabricacao.svg > /tmp/velho.svg
```

What had value and was not a dimension — the cable-routing rules — was
**rescued** into `CAIXA-ACRILICO-MONTAGEM.md`, §6b, before the page
died.
