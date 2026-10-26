# Home assembly — hotplate + the cost gate

Reference for **Stage 8** of the `/aipcb` pipeline, and for the human gate ⟨dB⟩
that decides **who assembles the board**: JLC (SMT assembly), or you, at home, on
the hotplate. Subordinate to the same gate philosophy — a gate that knows only one
answer is not a gate. Home assembly is the **second way out**, and there's a
number that decides which of the two you take.

> JLC-assembly is convenient but its price at **low quantity** is dominated by
> **fixed cost** — line setup, the stencil, the parts markup, and the engineering
> (extended-parts, feeders). For 5 boards, that fixed cost can be many times the
> value of the bare copper + the parts in bulk. There, home assembly is not
> penny-pinching: it's the right order of magnitude.

## 1. The cost gate — when to go home

**Rule of thumb:** if, for the quantity you need, the **assembly** (the JLC line:
setup + stencil + parts markup + engineering) **>> (bare PCB + parts in bulk)**,
route it to the **home path**. It's not the absolute price that decides, it's the
**proportion**: when the fixed cost dominates, the quantity is too small to pay
for the line.

- **In favor of JLC-assembly:** a quantity that dilutes the fixed cost; fine pitch
  (QFN/BGA) that the hotplate+hand can't place reliably; many identical boards.
- **In favor of home:** prototype / short run; most parts in 0805/1206 (easy on
  the hotplate); few polarized; you already have a hotplate + stencil + paste.

The gate runs at the ⟨dB⟩ of Stage 8, alongside the rotation inspection: **before
paying for the assembly, compare the assembly quote against (bare + parts).** If
the fixed cost dominates → **bare PCB + stencil** and you assemble at home.

## 2. The hidden bonus — the home path makes the rotation risk MOOT

The danger of the **180°** (the JLC library rotates its model vs. our footprint —
see `portao-humano.md`) exists **because a machine places the part without looking
at our render**. At home, **you are the operator**, and you place each part
looking at the **3D render of our own board** as the map (the
`kit/MAPA_COLOCACAO.md` + the `VISTORIA.md`). The source of truth and whoever
places the part are the **same** — the JLC library's rotation can no longer betray
you, because the JLC library doesn't enter the assembly. This does **not** waive
the polarity inspection (band/cathode/pin-1 can still go the wrong way if you
place them wrong), but it removes the entire class of the "library 180°".

## 3. The home flow — the order that doesn't burn a part

1. **Order: bare PCB + stencil, with _Assembly OFF_.** In JLCONE, order the bare
   board and the **stencil** (framework/frameless depending on your hotplate) and
   **turn off assembly**. With no assembly, the factory package is just
   gerbers+stencil — JLC's `bom`/`cpl` aren't needed for the factory, but the
   **`kit/`** is what YOU use.
2. **Parts via the LCSC cart.** The `kit/COMPRA_LCSC.csv` is the ready-made cart —
   one row per physical part, `Qty` + `Qty+spares` (bring spares: hotplate parts
   get lost and burned). The THT ones and those that don't go on the hotplate are
   bought from the same list or from the distributor (pots, radials, modules).
3. **Stencil + Sn63/Pb37 paste.** Align the stencil over the board, spread the
   **Sn63/Pb37 solder paste** (eutectic, 183 °C — more tolerant than lead-free for
   the manual hotplate), lift the stencil straight up.
4. **Place by MAP.** Place the **TOP** SMD following the `kit/MAPA_COLOCACAO.md`
   — grouped by value (all of one value at once), TOP and BOTTOM separated, with
   bottom and THT flagged. The render of our board is the orientation map (§2).
5. **Reflow of the TOP SMD on the hotplate.** Board on the hotplate, ramp up until
   the paste collapses and shines (the paste's profile curve), hold the peak,
   cool. **Only the TOP** goes on the hotplate.
6. **BOTTOM-side parts by hand.** The bottom parts **would fall onto the hotplate**
   if you tried to reflow the board upside down — they're **hand-soldered**, one by
   one, after the top is done. The `MAPA` lists them separately (e.g., `C16` on
   the brain board).
7. **THT last, all by hand.** Connectors, pots, radials, modules (Daisy, ESP32) —
   after all the SMD, so they don't get in the way of the hotplate or the access.
8. **Inspection with a loupe.** **SOIC/SOT** joints under the magnifier: bridges
   between pins, dry joint, tombstone of a passive, lifted pin. The polarized ones
   are checked against the `VISTORIA.md` (band/cathode/pin-1).

## 4. What the pipeline already hands you for this

The `kicad/kit_montagem.py` (v1.0.0) is the **home deliverable**: give it the
package folder and it emits the `kit/` — `MAPA_COLOCACAO.md`, `COMPRA_LCSC.csv`,
`LEIA-ME-KIT.txt` (see `fab-automation.md §5`). It is standard output of Stage 8
**alongside** the factory package, precisely so that the home path doesn't depend
on manual work on every run.
