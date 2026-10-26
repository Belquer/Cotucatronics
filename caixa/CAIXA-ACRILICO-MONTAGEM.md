# Acrylic enclosure — materials and assembly
### Cotucatronics · 09/06/2026 · transparent acrylic **3.0 mm**, Ponoko
> Valid for the enclosure already being cut (blind standoffs, thread in the acrylic) and
> for the following ones (standoffs with captive nut — file `cotuca_caixa_corte.svg`
> regenerated today). The difference is in §4.

---

## 1 · Materials and tools

| item | qty | note |
|---|---|---|
| **Weld-On 4** (SCIGRIP 4) | 1 bottle | thin solvent — all the 90° joints |
| **Weld-On 16** | 1 tube | gel — only the lid ↔ front and lid ↔ back joint (11.8°) |
| applicator bottle with a **needle** | 1 | comes with the Weld-On 4 or bought separately; without it there is no capillary action |
| **nitrile** gloves | | methylene chloride; latex does not do |
| strong painter's tape, or 4 spring clamps | | hold dry and during the cure |
| small square | 1 | check the corners before the solvent takes hold |
| **M3 × 10 screw, cheese head** | 6 | bottom → standoffs |
| **M3 nut** | 6 | only on the enclosures with a hex standoff (series) |
| **ø2.5** drill bit + **M3** tap | | only on the first enclosure (blind standoffs) |
| **M3 × 10 F-F spacer** + 8× M3 × 6 screw | 4 + 8 | carrier on the bottom. ⚠️ **10 and not 8**: the board passes over six standoffs of 2 × T = **6 mm**, and `pcb_contorno.py` asks for 9 minimum |
| spacers for the amp module | 4 | height per the module |
| **ø15 adhesive** rubber foot | 4 | bottom, 12 mm from the corners |
| 400 and 600 sandpaper (wet) | | only if you want the edges polished; the laser already leaves them smooth |
| microfiber cloth, isopropyl alcohol | | clean without scratching; **never acetone** |

**Do not use:** superglue (cracks the acrylic into a white web), epoxy, hot glue,
acetone to clean (clouds the surface).

---

## 2 · Before any solvent: dry-fit

1. **The protective film stays** on all pieces until the end of the gluing. The
   solvent vapor whitens the surface where it touches.
2. Dry-fit: **front + left + right + back**. The fingers go in by hand with light
   friction. If it binds: 400 sandpaper on the sides of the fingers of that joint,
   one pass. If it is loose: note it — it is the kerf for the series (`K` in the
   generator).
3. Rest the **lid** on top, dry. The front and back fingers go in at an angle
   (11.8°) — in acrylic they bind more than in wood. If they do not go in with
   hand pressure, sand the top corners of the front and back fingers; **never the
   lid.**
4. Measure the exterior: 200 × 120, 55 at the front, 80 at the back. Equal
   diagonals of the open bottom = square.
5. **Fit the bottom underneath** with the pieces assembled. It has to go in with
   0.5 clearance. Remove it.
6. **Fit the real pieces into the holes** — arcade, pots, encoder, OLED, jacks,
   DPDT, USB-C — **before gluing the lid.** A hole 0.5 mm off is a new shell; now
   it costs five minutes of sanding.

---

## 3 · Solvent-weld the shell (without the lid)

1. Disassemble. Reassemble **front + left + right + back**, without the lid. Tape
   around it. Square on the diagonals.
2. Weld-On 4 in the needle bottle. **The needle tip along the line of each joint,
   from inside** — the liquid enters by capillary action. Do not brush, do not
   spread beforehand. One slow pass per joint.
3. Thirty seconds without moving per joint. Four corners, two minutes.
4. **Rib:** it is glued to the lid, not to the shell — that is left for §4.
5. One hour before touching it. Twenty-four before loading it.

## 4 · Standoffs — the first enclosure and the following ones

**Where:** inside, in the 4 corners and at the middle of the front and back. Bottom
face at **T mm from the lower edge of the walls** (3, in this enclosure) — use the
bottom itself as a gauge: set it inside, flush with the edge, glue the standoffs on
top of it, remove the bottom before it dries.

**First enclosure (blind standoffs, already cut):** stack 2 squares with Weld-On 4,
glue the pair to the wall and to the neighbor. Once dry: **ø2.5 hole from
underneath, M3 tap**, slowly, without forcing — acrylic cracks under stress. Good
for a few dozen tighten/loosen cycles.

**Series (standoffs with hex):** each standoff is **two different squares**:
- **CB** (bottom): has the 5.7 mm hex — glued with the hex facing **down**
- **CT** (top): smooth — glued on top of the CB and covers the hex

Before gluing the CT, **put the M3 nut in the hex**. It becomes captive: the CT
covers it from above, the bottom covers it from below, and the hex does not let it
turn. The screw threads into steel forever. No hole, no tap, no heat.

## 4b · Lid reinforcement — before mounting anything on it

The `REFORCO` piece, 193 × 50, is glued **underneath the lid**, centered, with the
front edge **10 mm** from the front edge of the body, so that the three ø30 holes
are concentric with those of the lid.

- **Weld-On 4 face to face**, with a needle, along the edge — capillary action.
  Distributed weight (board + book), 30 min. Only after curing do you mount
  anything on the ø30s.
- Check the concentricity **dry**, with a button pushed into both holes, before
  putting solvent. Once it takes hold it cannot be corrected.
- ⚠️ A laser-cut acrylic edge is left with **residual stress**, and there is an
  indication that solvent on a stressed edge accelerates *crazing* (those fine
  cracks). **It is not confirmed in a source** — if it worries you, annealing the
  two pieces before gluing (80 °C, 1 h per mm, cool slowly) is the normal
  procedure and does no harm.

**Why:** a foot is ~300 N at mid-span between the front wall and the rib. In 3 mm
that flexes visibly and, over time, cracks **from the holes**. With the
reinforcement it is 6 mm where the foot steps and 3 mm where the pots and the
encoder pass through — which is the reason the lid is 3 (the PTV09 has ~7 mm of
thread).

✅ **The reinforcement hole is ø30, and the arcade mounts on a 6 mm panel.** It is
not a deduction: the **wood prototype had 6 mm** and the buttons — the LED ones and
the cowrie-shell one — tightened there. An object that existed is worth more than
any datasheet, and it was that measurement that closed the question.

## 4c · If the foot presses two buttons at the same time

The three arcades are at the **maximum distance the face allows**: axes at
x = 30 · 100 · 170, **70 mm between axes and 40 between edges**. There is no way to
spread them further — three ø30 holes on a 200 face with 12 mm of material up to the
wall is the geometric limit.

⚠️ **And 40 mm may not be enough.** The front part of a shod foot spans the 70–90
mm of width: it fits on top of two edges at the same time. If it happens on the
bench, what solves it **is not more space — it is a ridge**:

- two strips of 3 mm acrylic, ~10 × 45, glued **on edge** (10 mm high) on the lid,
  halfway between the buttons — x ≈ 65 and 135;
- or, more discreetly, the same strips lying flat: 3 mm of step already changes
  where the foot slides, and does not get in the way of the view.

It is left to decide **after stepping on it**, which is the only way to know. The
strips are cut at any moment from a scrap of sheet.

## 5 · The lid, last

Only after §2·6 has confirmed that all the pieces go into the holes:

1. **Rib:** glued under the lid with Weld-On 4, along the 200 mm, at **y ≈ 62 mm
   from the front** (between the arcade row and the knob row). Weight on top, 30
   min.
2. Weld-On **16** in the front and back grooves of the lid (the 11.8° ones).
   Weld-On **4** in the side grooves (the 90° ones).
3. Rest it, align the fingers, distributed weight (board + book). Wipe the excess
   gel with a dry cloth while it is fresh.
4. 24 h. Only then remove the films.

## 6 · Mounting the interior

Everything goes in **through the open bottom**, and everything with a connector — no
wire soldered board-to-panel.

1. **On the lid, from inside:** arcade (nut), pots and encoder (nut), OLED (4×
   M2.5). Cables: JST on the arcades, the 7-way one on the OLED.
2. **On the sides and back:** jacks (nut), DPDT (nut), PD decoy (2 screws or strong
   double-sided tape). Wires to the carrier with a header.
3. **On the bottom:** carrier on the 4 M3 × 10 spacers; amp module on its own; the
   two connect to the rest by screw terminal and IDC.
4. Connect the panel IDC, the jacks, the DPDTs, the bridge. **~80 mm of slack** on
   all the cables that go from the bottom to the shell — it is what allows you to
   open without disconnecting.
5. Bottom up, 6 M3 × 10 screws. Adhesive feet.

### 6b · Cable routes

They came from the page-4 drawing, which described an enclosure that was never made
— its dimensions died, these rules did not, because they speak of **relations** and
not of millimeters.

- **Piezos** come in from the left and go straight to the pre-amp: **< 60 mm**. It
  is the highest-impedance run of the whole instrument.
- **Exciters** leave the amp on the right, and **never cross the pre-amp**.
- **Panel IDC** (26 ways) runs along the **back** wall, far from the left.
- **Line to the mixer**: from the DPDT (back wall) to the amp and the back jacks.
- **How a signal cable is terminated** (the method, written on 09/20 at dB's
  request — *"it looks like a kludge, document the professional way"*): sheath
  stripped **25 mm**, braid gathered into a pigtail **≤ 10 mm**, center **4–5 mm**,
  both tinned, termination by **solder + heat-shrink** or by **a latching system
  crimp connector** (the Molex KK-254 or Harwin M20 families fit the board's 2.54
  headers; the code is confirmed in the datasheet), and a **strain-relief clamp ≤
  20 mm from the termination**. The complete step-by-step is in the assembly
  manual, in the wires chapter.
  🚫 A generic Dupont is not a system connector: with no latch, it backs out with
  vibration.
  ⚠️ **The strain relief is the part missing from almost every improvisation**: the
  force of the cable has to die at the clamp, never at the solder.
- **Piezo cable braid**: it depends on how many conductors the cable has, and
  confusing the two cases costs the whole signal (clarified on 09/20, with dB's
  cable in hand):
  - **two conductors + braid** (there is a return wire of its own): the braid is a
    **shield** and connects **only on the board side**; on both ends it closes a
    loop.
  - **coaxial** (one center + braid — the cable in use): the braid **is the
    return** and connects **on both ends**. Loose at the jack it is not less noise:
    it is **no return path**, so no signal.
  - In both cases there is no loop here, because the jacks are **isolated from the
    chassis** and the enclosure is acrylic — there is no second path. The loop is
    born when the sleeve touches metal that reaches ground by another route.
  - Always a **short pigtail**: an unbraided shield is shielding that is lost.

And the slack: **~80 mm** on all the cables that go from the bottom to the shell —
it is what allows you to open the enclosure without disconnecting anything.

## 7 · Swap or repair

| what | how |
|---|---|
| carrier, amp, step-down, bridge | 6 screws, the bottom comes down with everything |
| arcade, pot, encoder | disconnect the JST/wire, nut from inside, comes out the top |
| OLED | 4× M2.5 from inside, 7-way cable |
| jack, DPDT | nut from inside, wire to the header |
| **the shell** | not repaired: a new one is cut and the interior is transplanted |

Rule: **what costs money comes out; what stays glued is acrylic.**

## 8 · What this enclosure decides for the series

- real kerf in acrylic (`K` in the generator: tight fingers → 0.15; loose → 0.25)
- whether the 11.8° joint works with Weld-On 16, or whether the series needs a bevel
- whether 55 mm of front is enough with the arcades inside
- whether the captive-nut standoffs make it into the next order (yes, barring a
  surprise)
