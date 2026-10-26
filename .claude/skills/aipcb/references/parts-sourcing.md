# Component sources — the 3-level tree and the STOP gate

Reference for **stage 2** of the `/aipcb` pipeline. Every BOM line has to end up
**either** with an in-stock LCSC C-number that JLC actually assembles, **or** with
an own footprint, justified and verified against the datasheet. This document fixes
the 3-level decision tree and the STOP gate that `scripts/manifesto_fontes.py`
enforces.

> ⚠️ **Prices and fees change.** The numbers here are "order of magnitude / how it
> works", never a contract — **re-confirm the live quote on the JLCPCB order page**
> before trusting any cached value.

---

## 1. JLCPCB's part model

### The tiers

The tier is a property of the LCSC part **as it is in stock for assembly**, not of
the entry in the LCSC catalog — a part may sell on reel and **not** be in the
assembly library.

| Tier | What it is | Feeder fee |
|---|---|---|
| **Basic** | ~600–700 jellybeans always loaded in the machine (R/C 0402/0603/0805, transistors, LEDs, common regulators) | **None** |
| **Preferred Extended** | High-rotation Extended, marked "Preferred" | **Waived** in Economic PCBA |
| **Extended** (non-preferred) | Everything else in the library; needs a feeder mounted for your job | **per unique part type** |

Cost-optimal ranking: **Basic > Preferred Extended > Extended > (custom / outside the
library)**. The Extended feeder fee is **once per distinct C-number**, not per
placement (100× the same capacitor = one fee).

### What "in JLC's assembly library" means

= the C-number appears in the **JLCPCB SMT assembly** catalog with assembly stock > 0
and an assigned tier. It is a **different stock pool** from LCSC retail — sister
companies, separate logistics, same C-number reference.

How to check a part (`C#####`):
- **Official UI:** `https://jlcpcb.com/parts` — shows tier, assembly stock, price
  ladder. (Weak for parametric search — see §2.)
- **In the order:** the assembly flow matches each C-number and flags "out of stock"
  / "not in the library" per line. Authoritative, but not scriptable beforehand.
- **Programmatic:** §2.

There is **no official public REST API** for stock/assembly. All programmatic access
is via scraped datasets or community wrappers.

---

## 2. Programmatic search (find a part in the library / equivalent)

### (a) `jlcparts` (yaqwsx) — recommended offline source

- Repo: `https://github.com/yaqwsx/jlcparts` · UI: `https://yaqwsx.github.io/jlcparts/`
- A GitHub Action scrapes the assembly library into `cache.sqlite3`; then
  `jlcparts buildtables ... cache.sqlite3 web/public/data` emits JSON per category.
  Pull the built DB and **query locally with SQL** — no rate limits, deterministic,
  offline.
- Each record carries: C-number, manufacturer+MPN, category/subcategory,
  package/footprint, Basic/Extended(+Preferred) flag, stock, price ladder,
  datasheet URL, and a parametric-attributes blob. It is this that enables the
  parametric "find an equivalent" search.
- ⚠️ The canonical repo is **yaqwsx/jlcparts** (names like "yaganidev/jlcparts" are
  mirrors/forks).

### (b) `jlcsearch` (tscircuit)

Search engine for in-stock JLC parts; **any page returns JSON by appending `.json`**
to the URL → trivial parametric queries by LCSC#, manufacturer, package,
description, stock, price.

### (c) Apify "JLCPCB Parts Finder"

`https://apify.com/amicable_grid/jlcpcb-parts-finder/api` — C-number → JSON with
live stock, price ladder, specs, package, MOQ, datasheet. Paid API; good for
confirming a part on demand.

### (d) Direct LCSC endpoints (fragile — need CSRF + cookies)

`GET https://lcsc.com/api/global/additional/search?q=<C-number>` and
`POST https://lcsc.com/api/products/search`. Throttle ~100 req/min, no official API.
Use sparingly.

### SAFE equivalent checklist (level-1 fallback)

When the exact MPN is not an assemblable part, search the dataset for a part in the
library (Basic → Preferred Extended → Extended) that matches **all** of these before
substituting:

1. **Footprint/package** identical (0402, SOT-23-3, SOIC-8). Different package =
   different footprint = not a drop-in.
2. **Primary value** — R/C/L, or the functional family for ICs.
3. **Voltage** ≥ original. **4. Current/power** ≥ original.
5. **Tolerance** ≤ original (tighter or equal, never looser).
6. **Dielectric/temp** — on caps, **X7R/X5R/C0G have to match class** (a Y5V
   "equivalent" of an X7R is not safe).
7. **Temperature range** ≥ original.
8. For ICs/regulators: **identical pinout** + electrical spec (dropout, Iq,
   logic family/threshold, speed grade).

Only after all the applicable lines match is the substitute "safe". Record the
original MPN, the chosen LCSC#, the tier and the parameters that matched — it is the
audit trail.

---

## 3. Import / convert a part (level-2 fallback)

When there is no acceptable equivalent in the library but the part **exists** on
LCSC/EasyEDA, pull symbol+footprint+3D instead of drawing it by hand.

### `easyeda2kicad.py` (uPesy) — primary converter

```bash
pip install easyeda2kicad
easyeda2kicad --full --lcsc_id=C2040
easyeda2kicad --full --lcsc_id=C2040 --output <proj>/lib/jlc --overwrite
```
- KiCad v6–v9; emits `.kicad_sym` + `.pretty/` + `.3dshapes/`.
- ⚠️ Point `--output` at a **project** lib and add it to the project's
  `sym-lib-table`/`fp-lib-table` — the part travels with the repo.
- ⚠️ README: *"the correctness of symbols/footprints is not guaranteed — always
  check."* → run the §4 verification before trusting a converted footprint.

### `JLC2KiCadLib` (TousstNicolas) — alternative

```bash
pip install JLC2KiCadLib
JLC2KiCadLib C1337258 -dir My_lib -symbol_lib My_symbol_lib \
  -footprint_lib My_footprint_lib -models STEP WRL
```
Writes a self-contained project lib tree, easy to commit.

### Other sources (when EasyEDA has no good model)

SnapEDA · Component Search Engine (SamacSys) · Ultra Librarian — native KiCad
export, often **more IPC-accurate** than the converted EasyEDA; but **no C-number**,
so you still owe §2 to tie an assemblable C-number to the line.

---

## 4. Build from the datasheet (level-3 fallback)

Only when the part is real and has no usable symbol/footprint anywhere.

- **Footprint:** land patterns per **IPC-7351(B)**. Density suffix in the name:
  **M** (Most, hand-solder), **N** (Nominal — the default is), **L** (Least).
  Use **Nominal (N)** barring a density reason. Courtyard on `F.CrtYd`/`B.CrtYd`
  (KiCad's DRC flags overlap; nominal excess ≈ 0.25 mm). **Never delete the
  courtyard.**
- Tools: KiCad's Footprint Wizard (parametric families) ·
  `pointhi/kicad-footprint-generator` · IPC-7351 calculators.
- **Symbol:** pin numbers/names/types straight from the pinout table; power pins
  with the correct type; symbol pin maps 1:1 to the footprint pad.

### Verify a hand-built footprint BEFORE trusting it

1. **Dimension audit vs datasheet:** pitch, pad size, courtyard, span — a swapped
   pitch or an X/Y inversion is the #1 killer.
2. **Pad count/numbering vs pinout;** pin-1 mark on the silk/fab matches the corner
   in the datasheet.
3. **Footprint Editor checker + DRC** after placing.
4. **3D + paper test:** print 1:1 and put the real part on top — catches
   metric/imperial and pitch errors on the spot.
5. **Polarity/orientation:** for polarized parts the pin-1/cathode/plus mark on the
   **fab** has to be unambiguous — it drives the CPL rotation (§5).
6. **Paste/mask:** paste apertures present; segmented thermal pad (QFN/DFN).

Record: datasheet revision+page of the land pattern, the dimensions used, and
who/what verified it. It is the artifact that justifies an own footprint.

---

## 5. BOM / CPL and the rotation trap

**BOM** (minimum columns): `Comment` (value), `Designator`, `Footprint`,
`LCSC Part #`. Identical parts on a single line, designators joined by commas.

**CPL/centroid:** `Designator`, `Mid X`, `Mid Y`, `Layer` (Top/Bottom),
`Rotation` (degrees, JLC convention, in **mm**).

⚠️ **KiCad's 0° rotation almost never matches JLC's library** — for many packages
(SOT-23-x, SOIC/SOP, QFN, tantalum/electro caps, LEDs, SOD-123 diodes) the offset is
fixed (90/180/270°); some need an XY offset. And JLC's CPL uses a **Y-flipped**
convention vs KiCad. Sending the raw rotation → part mounted crooked → **dead
board**. Tools that fix it: `Bouni/kicad-jlcpcb-tools` (GUI),
`matthewlai/JLCKicadTools` (CLI), KiBot. **After generating, confirm part-by-part in
JLC's previewer** — it is the ⟨dB⟩ gate of stage 8, not scriptable.

---

## 6. The 3-level tree — deterministic decision (per BOM line)

Run it **per line**. The board does not advance to layout/fabrication until **every**
line passes the STOP gate.

```
Q0. Is there already a valid, in-stock C-number on this line (stock>0 AND in the library)?
    YES → record tier → PASS.
    NO → LEVEL 1.

LEVEL 1: equivalent in the library (cheapest, most reliable).
    search by category+package+value → filter stock>0 AND in the library →
    rank Basic>Preferred Extended>Extended → confirm the §2 equivalence checklist.
    safe match → assign C-number + tier + params + original MPN → PASS.
    no safe equivalent → LEVEL 2.

LEVEL 2: import the exact part (exists on LCSC/EasyEDA).
    easyeda2kicad --full --lcsc_id=C##### --output <proj>/lib/jlc   (or JLC2KiCadLib)
    → PROJECT lib → §4 verification.
    C-number in the assembly library → PASS.
    buyable but NOT in the library → mark "customer-supplied/consigned" → PASS*
        (*only if the order allows a self-supplied part).
    no symbol/footprint anywhere → LEVEL 3.

LEVEL 3: build from the datasheet.
    symbol from the pinout · IPC-7351 Nominal(N) footprint · courtyard on F.CrtYd
    → §4 VERIFICATION (mandatory) → tie the source (C-number or customer-supplied)
    → PASS.

═══ STOP GATE ═══  advance only when, for EACH line, one is true:
  (A) valid C-number, in stock, in the assembly library, tier recorded; OR
  (B) imported/hand-built footprint §4-VERIFIED with a C-number confirmed in the
      library; OR
  (C) line explicitly customer-supplied with a verified footprint (only if the
      order allows).
  Any line without (A/B/C) = BLOCKED. The fabrication package is not generated.
```

### The source manifest (the gate's artifact)

CSV/JSON committed alongside the project, one line per BOM line:

```
designator, intended_MPN, resolved_LCSC, tier(Basic/Preferred Extended/Extended/NA),
assembly_stock, source(existing|equivalent|imported|built|customer_supplied),
footprint_source(kicad_std|easyeda2kicad|JLC2KiCadLib|snapeda|hand),
verified(yes/no + método), datasheet_ref, notes
```

Sourcing is "done" when **every** line has a non-empty `resolved_LCSC` **and**
(`assembly_stock` > 0) **or** a justified customer-supplied flag, **and** every
imported/built footprint has `verified=yes`. It is this file that
`scripts/manifesto_fontes.py` reads and refuses (exits ≠0 and lists the blocked
ones). Proven with `--sabota`.

```bash
python3 .claude/skills/aipcb/scripts/manifesto_fontes.py manifesto_fontes.csv
python3 .claude/skills/aipcb/scripts/manifesto_fontes.py --sabota   # proves it can say no
```

---

## 7. Sourcing with Claude-on-Chrome — the human's browser is the source of truth

All of §2 above assumes programmatic access to stock. In this session that broke,
and the lesson is hard enough to become its own method.

### Root cause

The **agent sandbox cannot reach Mouser or Octopart** — `mouser.com` and
`octopart.com` reply **403** by egress policy (curl and WebFetch hit the wall).
**`WebSearch` works**, but it only proves one thing: when it returns a
`mouser.com/ProductDetail/...` page, Mouser **CATALOGS** that part. **It never says
the stock.** Catalog is presence; stock is availability — and they are different
things.

### Golden rule

> **The human's browser (Claude-on-Chrome) is the source of truth of
> availability.** The sandbox proposes; the browser confirms. **NEVER declare "in
> stock" from the sandbox** — the furthest the sandbox gets is "Mouser catalogs this
> part". **Catalog ≠ stock.** A BOM line is only *sourced* when the human has read
> the green stock on their own screen.

### "Available" has a definition — and it excludes backorder

A MATCH in the catalog is **not** availability. The filter has to be explicit, or it
lets through a part that exists and does not ship:

- **Definition of "available":** there is **"Ships Now" ≥ the needed quantity** in
  stock.
- **Treat as UNAVAILABLE (reject and pick an alternative):** **No Match**;
  **0 "Ships Now"**; **any "Backordered"**; and **long lead time** (Mouser marks
  *"Long lead time reported on this product"* / several-week lead).
- ⚠️ **An MPN can MATCH and still be backordered — that does NOT count as sourced.**
  The naive "No Match?" filter lets the backorder slip through, because the backorder
  *does match*. So the backorder has to be rejected **by name**, not by absence.

**Anchor (what this session cost):** the Samsung `CL31A106KBHNNNE` (10 µF **50 V**)
**matched** on Mouser and was **3 Backordered / long lead** — it sailed through the
"no match" filter and would have closed the order on an item that does not ship. It
was swapped for `CL31B106KAHNNWE` (10 µF **25 V**, ~10 k in stock, *Ships Now*). The
method's stated goal is to **not repeat the unavailable-MPN ping-pong**.

### Prefer manufacturers that Mouser carries

The choice of candidate MPN already solves half the problem. **Prefer** brands Mouser
certainly lists:

> Yageo · KEMET · Murata · TDK · Samsung · Vishay · onsemi · Nexperia · Diodes ·
> TE · Amphenol · Würth · Bourns · Littelfuse · Panasonic.

**Avoid** codes/brands Mouser usually does **not** list — that is what produced the
*"No Match"* of this session: **AOS** (`AO3400A`), **exact-code WIMA**
(`MKS2C042201...`), **specific 3M/Sullins** (`N2513`, `PRPC...`). The AO3400A is great
on paper and simply is not in the Mouser catalog; it stays as a *schematic value and
equivalence anchor*, and what you buy is the cataloged equivalent (e.g.: Vishay's
`SI2302CDS-T1-GE3`).

### 3-step flow

1. **The pipeline generates the consolidated BOM** (`export/BOM_CONSOLIDADO.csv`)
   with **candidate MPNs** from the brands above — one line per physical part, spec +
   footprint + qty. It is sandbox work, and it stops at "candidate", never at
   "confirmed".
2. **Hand-off to Claude-on-Chrome** — the human opens the browser with Claude and
   runs the prompt-template below, part by part, filtering by real stock.
3. **Lock the confirmed ones** in `export/COMPRA_MOUSER.csv` + in the template
   (`.xlsx`/`.xls`), filling in **Mfr Part# AND Mouser#** (exact match, both fields)
   only for the parts that came back with *Ships Now* ≥ qty. What came back
   backordered/long-lead goes back to step 1 for an alternative.

### What Chrome returns

**Per part, the `Ships Now` stock AND the lead time** — not "it exists in the
catalog". The return column becomes:

```
Peça | Mfr Part# | Mouser# | Ships Now (qty) | lead
```

### Prompt-template ready for Chrome (copyable block)

```
Estás no mouser.com com sessão do humano. Para CADA peça da lista abaixo:
  1. Busca pela spec + footprint + quantidade indicados.
  2. Aplica o filtro "In Stock".
  3. Devolve uma linha na tabela:  Peça | Mfr Part# | Mouser# | Ships Now (qty) | lead

Regras (duras):
  - NÃO escolhas nada com 0 em estoque, backordered, ou long lead time —
    só peças com "Ships Now" >= qty.
  - NÃO mudes o footprint da peça (o encapsulamento é o critério crítico).
  - Se a peça pedida não tiver "Ships Now" >= qty, escolhe a alternativa
    listada que tenha, e diz qual escolheste.
  - Reporta SEMPRE Ships Now (qty) E o lead time, por peça.

Lista:
  <Peça> — <valor/spec> — <footprint> — qty <N> — candidato <MPN>; alt <MPN2, MPN3>
  ...
```

The goal, said again: **do not repeat the unavailable-MPN ping-pong.** The sandbox
proposes cataloged candidates; the human's browser closes with real stock; and no
BOM line is taken as *sourced* without the "Ships Now ≥ qty" read on the screen.

---

## Tools — quick reference

| Step | Tool | Command / URL |
|---|---|---|
| Parametric search (offline) | jlcparts (yaqwsx) | `github.com/yaqwsx/jlcparts` |
| Search (JSON) | jlcsearch (tscircuit) | append `.json` to the listing URL |
| Search (hosted) | Apify JLCPCB Parts Finder | `apify.com/amicable_grid/jlcpcb-parts-finder` |
| Official UI | JLCPCB Parts Library | `jlcpcb.com/parts` |
| Convert LCSC→KiCad | easyeda2kicad.py | `pip install easyeda2kicad` |
| Convert LCSC→KiCad | JLC2KiCadLib | `pip install JLC2KiCadLib` |
| Vendor libs | SnapEDA · CSE · Ultra Librarian | web + KiCad plugin (no C-number) |
| Build footprint | Footprint Wizard · pointhi/kicad-footprint-generator | IPC-7351 Nominal(N) |
| BOM+CPL + rotation (GUI) | Bouni/kicad-jlcpcb-tools | PCM; fixes rotation+offset |
| BOM+CPL + rotation (CLI) | matthewlai/JLCKicadTools | rotation-correction DB |

Tools from the repo itself for this stage: `tools/codigos_lcsc.py`,
`tools/fichas.py`, `tools/bom_completa.py`, `kicad/mpn_nas_folhas.py`,
`kicad/dnp_para_pcb.py`, and this skill's `scripts/manifesto_fontes.py`.
