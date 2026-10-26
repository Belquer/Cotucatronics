# Graphics by GRID — placement by coordinate, not by drag

Reference for **Stage 6** of the `/aipcb` pipeline, for the placement of **graphics**
(logo, name, symbol, decorative banner) on the silkscreen. It **replaces** the method
of "dragging in the gerber viewer", which is **explicitly retired** at the end of this
document.

> A graphic here is **decorative** silkscreen — what is neither a reference nor a
> functional value. The refs/values follow the rest of Stage 6 (`arruma_silk.py`,
> `refs_apontam.py`); this document is only about what you put there for taste.

---

## Root cause — why this method exists

Positioning a graphic **by eye / by pixel** — dragging until it "looks right" — cost
three things this session:

1. **Re-render in a loop.** Each nudge asked for a new render to check, and the cycle
   *drag → render → not yet → drag* does not converge: the eye corrects by fractions
   the gerber cannot even resolve, and the board becomes a churn of commits that only
   move silkscreen.
2. **Collision with pads.** The banner landed **on the MIDI/audio pads of U2** —
   invisible in the drag, because in the viewer the text **drew itself on top** of the
   pads as if nothing.
3. **The viewer trap.** **Dragging in the gerber viewer DECEIVES**: the viewer **does
   NOT clip silk over pad** — it shows the whole text, pretty, on top of the copper.
   **The factory clips** (silk over pad is removed in production), and what arrives is
   **broken text**, with letters eaten where there was a pad or hole. What you approved
   on screen is not what gets fabricated.

> **The gerber viewer shows the silk the factory will erase.** Approving a position in
> the viewer is approving an image that does not exist on the board.

---

## Method — in coordinates, a single pass

1. **Close the design with ONLY functional silk.** References and values tidied
   (normal Stage 6); **no decorative graphic yet**. The graphic enters last, against an
   already-stable board.

2. **Generate a render of the board with a 0.5 mm grid.** Larger lines labeled every
   **5 mm**; the **XY axes in the SAME coordinate frame as KiCad** (the same origin, the
   same direction — so the number you read on the grid is the number you write on the
   part); the **"no-go" zones shaded** (courtyards + pads + holes + **edge margin ≥ 1.5
   mm**); and the **free-space rectangles annotated with the dimensions in mm**. The
   render is the map; the grid is the ruler.

3. **Declare each graphic's bbox BEFORE.** E.g.: *"COTUCATRONICS, text height 2.6 mm =
   **45 × 8.5 mm**"*. Knowing the box the graphic occupies **before** placing it is what
   trades trial-and-error for "fits / does not fit" read on the map. A graphic with no
   declared bbox does not enter.

4. **The human approves the position by READING the grid coordinates.** A single pass:
   look at the map, choose the free rectangle, read the corner/center coordinate. No
   dragging, no "a little to the left".

5. **Transfer the graphic to the exact coordinates.** **1 silk-only edit → 1 gerber
   regen.** The transfer **rejects silk over pad/hole** (it is the *gerber-truth*: what
   the factory would clip is refused at the source, not discovered at the end). No
   iteration — the position was already approved in step 4 against the true map.

6. **Retire "dragging in the gerber viewer".** It is not an optional step alongside
   this one — it is the **replaced** method. The viewer serves to check what has already
   been placed by coordinate; **never** to choose the position.

---

## Versioning note

A graphic **changes the gerber** (silk is gerber — see `references/versionamento.md`).
But under the **freeze-on-release** rule, the **rev only goes up when the board is
actually fabricated/ordered**. While the board has not been sent, moving or placing a
graphic generates a **new package (new commit stamp), same rev**. The rev letter does
not move because of a logo on a board that has not yet gone to the factory.

---

## BEST-EFFORT — the `kicad/grid_grafismo.py` helper

The method above is the contract. The tool below automates step 2 (the map). **It is
only committed if it really works** — see the state in the header of
`kicad/grid_grafismo.py` itself and in the session report. If it is not possible to get
it **solid** in a reasonable time, it stays a **documented stub with this spec**, and
the report says it is a **stub** — you do not fake a tool.

Spec (v0.1.0):

- Renders the **top face** (F.Cu + F.SilkS + F.Courtyard) via `kicad-cli`/`pcbnew`.
- Overlays the **0.5 mm grid** + **labeled 5 mm majors** + **axes in mm in the KiCad
  frame** (same origin/direction as the file).
- **Shades the no-go zones** (courtyards / pads / holes / edge margin).
- Prints the **largest free rectangles with dimensions** (to match the graphic's
  declared bbox in step 3).
- Uses the real `kicad-cli` and Python's `pcbnew`.
