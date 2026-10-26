# Assembly lessons — the acrylic enclosure, assembled by hand

### Cotucatronics · 19/09/2026 · rev. f · transparent acrylic **3.0 mm** · recorded from the bench

> **rev. f (19/09/2026):** lesson **#11** — the panel board only comes out with
> the TRS jacks removed; the encoder's tip catches on the lid. And #10 closed the
> brain board's orientation (the Daisy is under the center button) and opened the
> measure that is still missing: the depth of the blue button.
>
> **rev. e (19/09/2026):** lesson **#10** — the order of the three arcade buttons
> in the lid's holes stopped being free: **blue A, blue B, white**, left to right,
> because a tall capacitor on the brain board sits under the right hole and only
> the white button (no LED, shorter) clears over it.
>
> **rev. d (18/09/2026):** lessons **#7**, **#8** and **#9** (the threaded encoder
> fixes the clearance at 6.5 mm) (the IDC underneath does NOT get flipped; soldered
> ribbon). Lesson **#7** — OLED header soldered from below, jumpers in an L facing
> down; pin anchor = the square pad (VCC).
>
> **rev. c (17/09/2026):** lesson #1 corrected — the physical pot in use is a
> **16 mm body pot** (Ø16.90 · bushing **M7×0.75** · **3 terminals Ø1.20 at a 5.0
> mm pitch** · anti-rotation pin at **7.80 mm @ 30°** per datasheet ≈ 7.5 mm
> measured by dB), **NOT** a 9 mm PTV09A. This **supersedes** the earlier PTV09A
> premise. Added **note #6** (panel RV1/RV2 footprint vs physical pot — resolved by
> **wiring**, case B; no problem on the already-fabricated panel). Lid dimension
> kept (ø3.2 · 7.5–7.8 mm c-c · left/−X in the top view).

> **rev. b (16/09/2026):** lesson #1 — the pot's indexing-pin hole **fully located
> on the part** (**ø3.2 mm · 7.5 mm center-to-center · to the left of the shaft in
> the top view**), replacing the earlier datasheet uncertainty.

> This is not the assembly manual (that is `CAIXA-ACRILICO-MONTAGEM.md`). This is
> what the **part in hand** taught after it was cut and assembled — five things
> that only showed up with the real acrylic on the table, each with the generator
> line where it is born, the chosen fix (**screwed**, see the directive at the
> end), whether it forces a re-cut, and the hardware that becomes necessary. It is
> the raw material of the next **enclosure rev**.

---

> ## ⚠️ For the CURRENT enclosure (the one already assembled)
>
> These corrections are for the **next** revision. The enclosure that already
> exists stays as it is: **the bottom only holds if the 6 standoffs are glued to
> the walls first** — glue the 6 standoffs, let them cure, and only then tighten
> the 6 screws from below. There is no wall↔bottom hole in this enclosure; the
> bottom's load passes entirely through the standoffs' glue joints (this is
> precisely defect #5). Do not try to tighten the bottom before the standoffs are
> firm on the wall.

---

## 1 · Pot indexing-pin hole — does not exist in the generator

**Observed symptom:** the pots (and the encoder) do not seat flush to the face;
they are left with play to rotate instead of locking against the panel board.

**Cause in the design:** the generator makes **only the central ø7** per pot —
`cotuca_caixa_corte.py:264` (encoder at x=105, RV1/pot L at x=140, RV2/pot R at
x=175, all at y=92). **It does not make the anti-rotation pin hole** (*locating
pin* / *anti-rotation pin*; the hole that receives it is the *locating hole*).
Without that hole the pot body has nowhere to seat the pin and does not sit flush.

**MEASURED distance (located): 7.5mm center-to-center.** Measured directly on the
part by dB, from shaft-center to pin-center. **The part in hand is boss — it
supersedes the datasheet.**

**Important clarification to record** (avoids future confusion): the physical pot
in use is a **16 mm body pot** — Ø16.90 · bushing **M7×0.75** · **3 terminals
Ø1.20 at a 5.0 mm pitch** · anti-rotation/locate pin at **7.80 mm radius @ 30°**
per datasheet (≈ 7.5 mm measured by dB, to the left). **This supersedes the
earlier PTV09A premise**: the PTV09A is a **9 mm** pot (2.5 mm pin pitch, locate
posts ~4.4 mm) and **is not** the physical part. The **7.5mm is the anti-rotation
lock on the PANEL side** — the *tab*/pin that enters the enclosure **LID**; it is
the feature the lid dimension receives. The **LID** hole uses **ø3.2mm · 7.5mm
c-c** (kept). On the panel RV1/RV2 footprint (PTV09A) vs the physical 16 mm pot,
see **note #6** — resolved by **wiring** (case B), no problem on the
already-fabricated panel.

**Angular position of the hole (now known):** pin hole at **7.5mm c-c, to the left
of the shaft in the top view of the panel board** (**−X, same Y** as the shaft
center). Looking at the panel board from above, the anti-rotation pin hole is to
the **LEFT** of the shaft hole.

> ⚠️ **Care for the implementation in the generator:** «left in the top view» has
> to be mapped by the lid's coordinate transform (`to_top()` / any *mirror* of the
> LID in `cotuca_caixa_corte.py`) — **confirm the SIGN against the transform before
> generating**, so it does not come out mirrored. It is the axis/*mirror* family of
> bug the skill itself warns about. **The part in hand + the top view are boss.**

Rule (§0 of the `caixa` skill, and it holds here in full): **the part in hand is
boss.** The **16 mm pot has an anti-rotation pin** (7.80 mm @ 30° per datasheet) —
confirm on the part whether the lid needs **1 or 2** indexing holes.

**Chosen fix:** add hole(s) at the *locating hole* to the measured dimension
(**7.5mm c-c**), diameter **ø3.2mm** (measured by dB, confirmed on the part).

> **Full spec of lesson #1 (pot locate hole):** **ø3.2mm · 7.5mm
> center-to-center · to the left of the shaft in the top view of the panel board
> (−X, same Y)**.

**Needs re-cut?** **Yes.**

**Hardware:** none.

---

## 2 · OLED mounts outward, exposed

**Observed symptom:** the OLED module ends up pushed to the **outer** face of the
lid — glass, PCB and cable are left on the outside, exposed and unprotected.

**Cause in the design:** today the lid has 4× **ø2.5** (`cotuca_caixa_corte.py:265`)
plus the **55×27.5 window** (`:266`), and **nothing else** — no standoff/spacer
that defines which side the module mounts on. With no internal stop, the natural
assembly pushes the module outward.

**Chosen fix:** mount the OLED **on the inside**, on **4 M2.5 standoffs** — the
glass looks out through the window, the screw goes in from the front, and the
module and cable stay **inside** the enclosure.

**Needs re-cut?** **No** — the 4 ø2.5 holes already exist. Only if you want a
**recess** for the glass to sit flush with the face is there a re-cut.

**Hardware:** 4 M2.5 standoffs + screws.

---

## 3 · The cables did not enter the enclosure

**Observed symptom:** the OLED wires had nowhere to enter; during assembly they
were run **through the ventilation rectangles** as a kludge — the `BACK_RECTS`
(`cotuca_caixa_corte.py:270`) and the `BOT_RECTS` (`:343`).

**Cause in the design:** it is a **direct consequence of #2** — with the OLED on
the outside, the module and its harness end up on the wrong side of the face and
the cable has no service hole to come back inside.

**Chosen fix:** **solved by fixing #2.** With the OLED mounted on the inside, the
cable is already born inside the enclosure and there is no kludge through the
grilles.

**Needs re-cut?** Not on its own — it follows #2.

**Hardware:** none (that of #2).

---

## 4 · The stiffener/rib glue stains the transparent acrylic

**Observed symptom:** the solvent of the stiffener and rib joints **clouds and
stains** the transparent lid — the whitening where the vapor/solvent touches,
which the manual itself already warns about (`CAIXA-ACRILICO-MONTAGEM.md:36` and
`:98-101`).

**Cause in the design:** the `REFORCO` part (`cotuca_caixa_corte.py:345-350`) and
the `NERVURA` (`:363`) are **bonded with capillary Weld-On 4** underneath the
transparent lid. Solvent on transparent acrylic = visible stain, and on a
transparent shell the interior is the face (§4e of the `caixa` skill).

**Chosen fix:** **screw** the stiffener and the rib — **ø3.4 holes in the LID** +
a nut pocket in the part (the same captive-nut machinery as the standoffs, see #5)
— **zero solvent**. The stain disappears at the source because the source (the
solvent) ceases to exist.

**Needs re-cut?** **Yes.**

**Hardware:** ~9 M3 + 9 nuts.

---

## 5 · Bottom — hidden, glued fastening (*design smell*)

**Observed symptom:** the bottom's fastening is confusing and fragile. The bottom
hangs from **6 M3 screws** that rise to **captive nuts in 6 glued standoffs**; the
bottom's entire load passes through **6 12 mm glue joints**, with no hole tying
wall to bottom.

**Cause in the design:** the **CB/CT** standoffs (`cotuca_caixa_corte.py:366-370`)
are **glued to the WALLS**, 3 mm above the lower edge
(`CAIXA-ACRILICO-MONTAGEM.md:66-84`). There is no wall↔bottom hole — the bottom is
held only by the 6 standoffs' glue. It is hidden (you cannot see how it is held)
and fragile (all the load in 6 glued 12 mm joints).

**Chosen fix:** **posts screwed to the wall AND to the bottom** — captive nut on
**both** sides, **ø3.4 hole through the wall** (screw head visible, and being
visible does no harm) plus the **ø3.4 bottom hole that already exists**. The bottom
goes from hanging on glue to being screwed to the structure.

**Needs re-cut?** **Yes.**

**Hardware:** ~12 M3 + 12 nuts.

---

## 6 · Panel RV1/RV2 footprint (PTV09A) vs the physical 16 mm pot — resolved by wiring

**What the files say (checked on the PCB and in the schematic):** RV1 and RV2 on
the panel board use the footprint
`Potentiometer_THT:Potentiometer_Bourns_PTV09A-1_Single_Vertical` (a **9 mm** pot
mounted ON the PCB). Measured in `cotuca_painel.kicad_pcb`:

- **3 through-hole pads** — pad 1 (GND) at (0, 0), pad 2 (POT_L on RV1 / POT_R on
  RV2) at (0, −2.5), pad 3 (+3V3A) at (0, −5). **Center-to-center pitch = 2.5 mm**,
  hole **ø1.0 mm**, pad ø1.8 mm.
- **2 "MP" mounting posts** (PTV09A locate) hole ø2.2 mm at (7, −6.9) and (7, 1.9).
- **Body (F.Fab) ~12 × 9.7 mm; courtyard ~14.4 × 13.7 mm** — a 9 mm pot's
  dimension.
- The panel's schematic/BOM declare MPN **PTV09A-1020S-B103** (Bourns, 9 mm, 10k
  lin).

**The physical pot** is a **16 mm** one with **3 terminals at a 5.0 mm pitch** —
that is, **2× the pitch** of the footprint (5.0 vs 2.5 mm) and a much larger body.
As a **board-mounted** part it **would NOT fit** the panel's PTV09A footprint.

**Resolution (case B, decided by the user — NOT a fabrication problem):** the pots
are mounted on the **LID** and connected by **WIRE** to the panel's RV1/RV2 pads —
**not** soldered as board-mounted pots. With wiring, you solder **3 wires to the 3
pads** of each RV; the footprint pitch (2.5 mm) **is irrelevant** for wiring, and
the MP posts go unused. The function is preserved (POT_L/POT_R/+3V3A/GND reach the
pot). **The already-fabricated panel board is OK — there is no re-fabrication to do
because of this.**

**For the NEXT panel revision (clarity, not urgency):** swap the RV1/RV2 PTV09A
footprint for a **3-pin wiring header/connector** (or explicitly mark it as
wire-solder pads), so the drawing is honest — **wired, not board-mounted**.

**BOM note (do not change the already-ordered BOM — just record):** the real pot
purchased is a **16 mm** one (M7×0.75, 3T @ 5.0 mm, anti-rot 7.80 @ 30°),
separate; the **"PTV09A" in `export/painel_2026-09-12_06db1c7/bom_painel.csv`
(line RV1,RV2)** and the MPN `PTV09A-1020S-B103` in the schematic are **vestigial**
for the physical part — they function as the wiring *landing*. Discrepancy
recorded; the ordered BOM kept as is.

**Needs re-cut / re-fabrication?** **No** (neither enclosure nor panel board).
Only the next panel rev gains the wiring connector.

**Hardware:** 3 wires per pot (6 total) + terminals.

---

## 7 · The OLED header does not fit over the top — soldered from below, jumpers in an L facing down

**Observed symptom (dB, 18/09):** the male header of `J2` (OLED) with **straight**
female jumpers does not fit between the panel board and the acrylic. Jumpers in an
**L** over the top do not either: the height to the lid reached, but the top of the
enclosure does not let the wires out; turning the L down squeezes the wire between
the components and the lid.

**Cause in the design:** the board↔lid clearance was **never drawn** — it is step
M6 of the IFÁ10000 M-track (`/aipcb`), which did not exist when the panel was made.
The connectors are all on the face toward the lid.

**Chosen fix:** header **soldered from below** the board, jumpers **in an L turned
back and down**. Cleaner and with the wire free. The wires are **color-coded** on
the module side.

⚠️ **From below the silkscreen cannot be read and the order appears reversed.** The
anchor that is seen from both sides: **pad 1 is SQUARE, and pad 1 is VCC (+5V)**.
From the square, the order is always **VCC · GND · DIN · CLK · CS · DC · RST**
(pads 1–7, checked on the fabricated board `06db1c7`). Before wiring, confirm with
the multimeter by continuity: the square connects to **`TP1` (+5V)**, its neighbor
to **`TP3` (GND)**. The colors say which wire is which signal; only the anchor says
which pin is which signal.

⚠️ **Do not repeat on `J1` (IDC 2×13) nor on the LED JSTs (`J6`/`J7`):** on a keyed
connector, mounting from the other side swaps pins — on the IDC pairs of vias with
power, on the LEDs anode with cathode. The question «does it fit with the connector
mated?» is left undone on those.

**For the NEXT panel revision:** cable connectors on the **bottom** side by design,
with the names on **`B.Silkscreen`** (readable from where you connect), and the
board↔lid clearance drawn, not discovered.

**Needs re-cut?** No.

**Hardware:** female L jumpers (7).

---

## 8 · The IDC pin box (`J1`) does not fit over the top — and does NOT get flipped down

**Observed symptom (dB, 18/09):** the Würth pin box of `J1` soldered over the top
hits the OLED and leaves no room to screw down the encoder.

⚠️ **Flipping it down does NOT work, and may burn.** A 2-row keyed pin box, seen
from the other side of the board, is the mirror image — and no rotation of it or of
the connector on the cable undoes a mirror. Simulated on 18/09 against the
contract's 26 vias:

| from below | connector on cable | correct | effect |
|---|---|---|---|
| key on one side | straight | 0/26 | **+3V3A shorted to GND** (via 9 → pad 10) |
| key on one side | flipped | 2/26 | 24 signals swapped |
| rotated 180° | straight | 2/26 | 24 signals swapped |
| rotated 180° | flipped | 0/26 | **+3V3A shorted to GND** |

**Chosen fix:** **ribbon soldered directly to the panel pads, from below, with no
connector on that side.** Each conductor goes to its pad by number — there is no
mirror possible. On the brain board side the IDC connector stays at `J14` (that is
where you disconnect). Over the top of the panel board, the height is zero.

**How, without going wrong:**
1. IDC connector crimped normally on the brain board end: **red wire = pin 1** (the
   triangle on the connector).
2. On the panel board end, separate the conductors ~15 mm. **Red = 1**, and from
   there on in order: **odds (1, 3 … 25) in the row farthest from the nearest long
   edge, evens (2, 4 … 26) in the row closest to it.** Pad 1 is **round** (there is
   no anchor by shape): it is **at the `J1` end on the side of connectors
   `J3`/`J6`**.
3. **Before wiring, continuity** from the loose connector (probe in the connector's
   holes) to the board: pin **9 ↔ `TP2`** (+3V3A), pins **11 and 12 ↔ `TP1`**
   (+5V), pin **1 ↔ `TP3`** (GND), pin **2 ↔ `J2` pad 4** (OLED_CLK). And **no**
   continuity between `TP1`, `TP2` and `TP3`. If one fails, the ribbon went in
   shifted.

**For the NEXT panel revision:** the cable connector on the bottom side **by
design** (footprint on the `B` face, which KiCad mirrors correctly), and the
board↔lid clearance drawn.

**Needs re-cut?** No. **Hardware:** none (the ~300 mm ribbon is already planned).

---

## 9 · The screwed-down encoder fixes the clearance at 6.5 mm — and anything taller hits

**What dB has in hand (18/09):** a generic EC11 **with a threaded bushing** (6 mm
knurled shaft, 20 mm above the body, body 11.7 × 6.5 mm tall) — **not** the
`EC11E15244G1` from the BOM, which has no thread. Screwed to the lid, it **holds
the board** (it counts toward M2). And it sets the distance: **the board sits 6.5
mm below the acrylic.**

**Consequence (measured against the height table):** on the top face only what is
less than 6.5 mm fits. These hit:

| part | what to do | anchor from below |
|---|---|---|
| `J2` (OLED) | header from below, jumpers in an L (#7) | pad 1 **square** = VCC |
| `J1` (IDC) | ribbon soldered to the pads (#8) — do **not** flip the pin box, nor at either end (8 of the 16 combinations give a short or a swap, and under the brain board there are only 9 mm) | pad 1 round — see #8 |
| `J3`, `J4`, `J5` (buttons) | JST from below, or soldered wires — switches, no polarity | pad 1 **square** = signal, pad 2 = GND |
| `J6`, `J7` (LEDs, 1=A 2=K) | **soldered wires** from below, **not** JST (flipping the keyed connector swaps A and K) | pad 1 **square** = **+5V (anode)**; confirm with `TP1` |
| `C3` (100 µF, 11 mm) | from below; polarity goes by the hole | pad 1 **square** = **+** (+5V) |
| `RV1`, `RV2` | **the pots CANNOT go on the board**: the body (≥ 8 mm) does not fit in 6.5. They go on the lid, **wired** (case B, #6) — the 16 mm ones from the drawer work | — |

**On the lid:** the encoder hole is ø7 and the thread is M7 — open it to ~7.3 (the
enclosure gives). **Posts** for fastening (M2): height **6.5 mm**, derived from the
encoder, not chosen.

⚠️ **To verify on the part:** whether the encoder has the **2 button pins** (push)
on the other side. The board uses `ENC_A_PUSH`; an encoder with no switch loses
that function.

**For the NEXT panel revision:** the board↔lid clearance **drawn** from the chosen
encoder, and the top face only with what fits on it.

**Needs re-cut?** Just opening the encoder hole. **Hardware:** the encoder's nut
and washer; 4 posts of 6.5 mm (or 6 + washer).

---

## 10 · The order of the arcade buttons is not free — the tall capacitor is boss

**What happened (dB, 19/09):** on assembly, a capacitor on the brain board was too
tall under one of the ø30 holes. The two **blue (freeze)** ones went to the
**left** holes, together, and the **white (cowrie shells)** one to the **right**,
which is the hole over the capacitor: the white one **has no LED**, and so its body
is much shorter.

**Why this did not show up before:** the `pcb_contorno` volume check
(`confere_arcade`) treats the three buttons as **equal**, with the worst-case
depth — 40 mm end to end, which is that of the button **with an LED**. A button
with no LED is shorter, and that difference never entered the count. The result is
that the tool does not distinguish which hole accepts a tall part underneath.

**What is decided for this enclosure:** order **blue A · blue B · white**, left to
right in the top view. It is in the assembly manual, in the step for the arcade
wires.

**Measured (dB, 19/09):** the white button descends **20 mm** below the panel
board, with connectors. The capacitor is **C1, 2200 µF**. With this, the
`confere_arcade` (v0.24.0) now has **depth per button** — blue 34, white 20 — and
electrolytic height **by diameter** (D16 = 25 mm). Free height: **7.5 mm** under a
blue, **21.5 mm** under the white.

**The orientation was resolved the same day, and contrary to what I had written.**
The Daisy under the center button **distinguishes** nothing: it falls under the
middle hole in both orientations. What distinguishes is the **silkscreen** — with
the user side facing you, the panel's reads right-side-up and the **brain board's
reads upside down**. The board is **rotated 180°** relative to the model, in the
orientation that 09/06 had rejected for putting tall parts under the buttons. And
then `C1` (25 mm) falls under hole 1 — the tall capacitor that forced the buttons
to be swapped, which was what remained to be explained.

⚠️ **And nothing prevented the mistake:** the four mounting holes form a
**symmetric rectangle**, so the board screws down in both directions with the same
effort. **The guarantee has to be mechanical, not documentary:** on rev D, an
asymmetric mounting or a keyed cutout, so the wrong orientation cannot go in.

⚠️ **And this knocks down the clearance number, not the lesson.** The Daisy is 17
mm and the checker only gives 7.5 mm under a blue — but it **fits, tightly**, says
whoever assembled it. So the blue button does **not** descend the 34 mm of the
worst case: for the Daisy to fit, it descends **~24 mm**. The worst case came from
never having measured the button head above the panel board. **To be measured: the
depth of the blue button**, from the panel board downward — the white one's has
already been measured (20 mm).

⚠️ **And it is left open which was the tall part.** With the orientation confirmed,
`C1` is 64 mm from the nearest hole. Under the left hole is the **pin box `J14`**,
with the IDC cable connector on top; under the right one, only low parts. The order
of the buttons is recorded as **what worked on the bench**, not as checked
geometry.

---

## 11 · The panel board only comes out with the TRS jacks removed

**What happened (dB, 19/09):** with the four TRS jacks mounted, the panel board
**has no way to come out** — the encoder's tip catches on the lid's acrylic before
the board frees the jacks.

**The rule this creates, and it holds for assembly and for repair:** the panel
board **goes in and comes out before the TRS jacks**. Whoever is going to swap a
part on the panel board first removes the four jacks, and not the other way around.

**For the next enclosure rev:** either the panel board comes out without passing
through the jack zone, or the encoder hole in the lid is a slot that lets it recede.
Until it is one or the other, this is a **mandatory order**, not a preference.

---

## Directive for the next version

> **EVERYTHING SCREWED — glue only if absolutely necessary.**
>
> The next enclosure is **100% disassemblable and reworkable**. Glue (Weld-On)
> stops being the default assembly means and becomes a justified exception. All
> five corrections above follow from here: where there was once solvent or
> dependence on glue, there comes a screw + captive nut.

## The shell decision — the biggest job

The shell (the walls and the way they close) is the biggest rewrite, and the
user's decision is made:

- **Chosen: internal screwed posts / brackets.** They are **clones of the posts
  from #5** — they reuse the **nut-pocket** machinery that already exists in the
  generator: `hexagono()` and the `"hex"` feature (`cotuca_caixa_corte.py:366-368`
  and the check at `:521`). Chosen over T-slot because it has **MUCH less generator
  rewrite**: the captive-nut machine is already made and tested; it only needs
  replicating at the corners.

- **T-slot stays as a future aesthetic upgrade.** It is the prettier finish, but
  not now. Technical note: the **2 lid joints at 11.8°** complicate an *in-plane*
  T-slot — one more reason to leave the T-slot for later and go with screwed posts
  now.

---

## Summary table — the 5 corrections

| # | correction | re-cut? | hardware | job |
|---|---|---|---|---|
| 1 | **ø3.2** hole(s) for the pot indexing pin (**7.5mm c-c**, left of the shaft in top view, measured on the part) | **Yes** | none | small |
| 2 | OLED on the inside, on 4 M2.5 standoffs | No (holes exist)* | 4 M2.5 standoffs + screws | small |
| 3 | cables enter (solved with #2) | No | — | none (follows #2) |
| 4 | stiffener + rib **screwed** (ø3.4 in the lid + nut pocket) | **Yes** | ~9 M3 + 9 nuts | medium |
| 5 | bottom: **posts screwed to the wall AND to the bottom** — **the shell, the BIGGEST job** | **Yes** | ~12 M3 + 12 nuts | **biggest** |

\* #2 only needs a re-cut if you want a recess for the glass to sit flush with the
face.

> **Note #6 (not an enclosure correction):** the pot is mounted on the lid and
> **wired** to the panel's RV1/RV2 pads (case B). No problem on the
> already-fabricated panel board; the next panel rev swaps the PTV09A footprint for
> a 3-pin wiring connector. See section **6**.

> The biggest job is the **shell (#5)**: rewriting the bottom's fastening as posts
> screwed at the corners, cloning the captive-nut machine (`hexagono()` / `"hex"`)
> beyond today's 6 standoffs.

---

## Provenance

Recorded on 16/09/2026 from the hand assembly of enclosure r2 (acrylic 3.0 mm).
The five lessons come from the real part on the bench, not from a datasheet — which
is the rule of the `caixa` skill itself (§0: the part in hand is boss). It feeds
the next **enclosure rev**; the rewrite of the generator's geometry (posts/T-slot,
the pin hole) is a later task — this document is the **scope**, not the
implementation.
