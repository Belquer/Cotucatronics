<!-- carimbo: MANIFEST.md v7 · 2026-09-18 · placas: painel rev C · cerebro rev C -->
# MANIFEST — authoritative index of `export/`

> **Protocol:** before uploading/consuming an artifact, check its **stamp**
> against this MANIFEST; if it doesn't match, `git pull`. A cart, an upload, or an
> assembly that starts from a stamp older than the one here is consuming a
> superseded copy — the root of the old-MPN mistake that has already cost us.

This is the **only** point for "am I on the current version?". The stamp lives in
each artifact (first line of the CSV/MD, `carimbo` cell in the xlsx/xls,
`LEIA-ME.txt` in the packages); this MANIFEST says which stamp is current. The two
have to match.

## Purchase (Mouser) — loose files

| Artifact | Version/stamp | Date | Board rev | For what |
|---|---|---|---|---|
| `COMPRA_MOUSER.csv` | v2 | 2026-09-12 | painel rev C · cerebro rev C | BOM of real MPNs to **import into the Mouser BOM Tool** |
| `BOM_CONSOLIDADO.csv` | v2 | 2026-09-12 | painel rev C · cerebro rev C | unified cerebro+painel BOM (LCSC + Mouser MPN + designators + qty) |
| `MOUSER_ALTERNATES.md` | v2 | 2026-09-12 | painel rev C · cerebro rev C | PRIMARY pick + 2–3 alternatives per problem part; the **7 verified in stock** |
| `mouser_40440548.csv` | v1 | 2026-09-18 | painel rev C · cerebro rev C | **what ARRIVED** from Mouser (21 rows, from the 14/09 shipping notification), each bag matched to the refs of the two boards. The `manual_montagem.py` reads it and **refuses** if a ref doesn't exist, if the size doesn't match, if a ref is in two bags, or if an SMD part ends up with no bag |
| `COMPRA_MOUSER_template.xlsx` | v2 | 2026-09-12 | painel rev C · cerebro rev C | Mouser import template (MPN col + Mouser no. filled in) |
| `COMPRA_MOUSER_template.xls` | v2 | 2026-09-12 | painel rev C · cerebro rev C | same, legacy `.xls` format |

**v2 = cut with the 7 MPNs verified in stock at Mouser (2026-09-12).** The previous
cut (v1, the initial export) had MPNs that were replaced due to backorder/no-match
and **should no longer be consumed** — it was that old copy that generated the cart
with the superseded MPN.

## Factory packages (JLCPCB) — folders

These carry their own stamp in the `LEIA-ME.txt` (generator + date + commit),
which **is** the board's versioning mechanism. Stamp =
`para_fabrica v0.19.0 · commit 06db1c7`.

| Artifact | Version/stamp | Date | Board rev | For what |
|---|---|---|---|---|
| `cerebro_2026-09-12_06db1c7/` | para_fabrica v0.19.0 · 06db1c7 | 2026-09-12 | cerebro rev C | factory package for the cerebro |
| ├ `cerebro_2026-09-12_06db1c7_gerbers.zip` | (from package 06db1c7) | 2026-09-12 | cerebro rev C | gerbers+drill for JLCPCB |
| ├ `bom_cerebro.csv` / `cpl_cerebro.csv` | (from package 06db1c7) | 2026-09-12 | cerebro rev C | BOM + placement (JLC CPL) |
| ├ `cpl_rico_cerebro.csv` | (from package 06db1c7) | 2026-09-12 | cerebro rev C | CPL with Val/Package preserved |
| ├ `VISTORIA.md` | (from package 06db1c7) | 2026-09-12 | cerebro rev C | package inspection/gate |
| └ `kit/` (COMPRA_LCSC.csv · MAPA_COLOCACAO.md · LEIA-ME-KIT.txt) | (from package 06db1c7) | 2026-09-12 | cerebro rev C | home assembly kit (LCSC cart + map) |
| `painel_2026-09-12_06db1c7/` | para_fabrica v0.19.0 · 06db1c7 | 2026-09-12 | painel rev C | factory package for the painel |
| ├ `painel_2026-09-12_06db1c7_gerbers.zip` | (from package 06db1c7) | 2026-09-12 | painel rev C | gerbers+drill for JLCPCB |
| ├ `bom_painel.csv` / `cpl_painel.csv` | (from package 06db1c7) | 2026-09-12 | painel rev C | BOM + placement (JLC CPL) |
| ├ `cpl_rico_painel.csv` | (from package 06db1c7) | 2026-09-12 | painel rev C | CPL with Val/Package preserved |
| ├ `VISTORIA.md` | (from package 06db1c7) | 2026-09-12 | painel rev C | package inspection/gate |
| └ `kit/` (COMPRA_LCSC.csv · MAPA_COLOCACAO.md · LEIA-ME-KIT.txt) | (from package 06db1c7) | 2026-09-12 | painel rev C | home assembly kit (LCSC cart + map) |

## Outside `export/` — stamped, but with a pending issue

| Artifact | Version/stamp | Date | Board rev | For what |
|---|---|---|---|---|
| `docs/datasheets/README.md` | ⚠️ **regenerate online** | 2026-09-08 (fichas.py v0.9.0, `18729f1`) | pre-rev-C-sourcing | datasheet index; **lists superseded MPNs** (PTC `2920L330/24MR`→`MF-LSMF300/24X-2`, MOSFET `AO3400A`→`SI2302CDS-T1-GE3`, film `MKS2C042201K00KSSD`→`MKS2C042201M00KN00`, IDC 3M `N2526/N3793`→Würth `61202621621`). Matching each new MPN to its **datasheet** requires an online download — don't do it by blind substitution (see note). |
| `tools/codigos_lcsc.json` | ⚠️ **regenerate online** | 2026-09-08 (codigos_lcsc.py v0.2.0, `1ce1d20`) | pre-rev-C-sourcing | LCSC→JLCPCB map; Mouser MPNs here diverge by path (LCSC vs Mouser). The `mpn`/`codigo`/`prova` trio has to stay consistent — changing only the `mpn` string lies about the LCSC code and the proof. Requires an online LCSC lookup. |

> **Note on "regenerate online".** These two are not safe string substitutions:
> each MPN carries a **PDF datasheet** (README) or a **LCSC-code + proof pair**
> (json) that describe the old part. Changing only the MPN would leave the
> datasheet/proof talking about a different part — a false statement, worse than
> the old value. The correct fix needs the network (download the new datasheet /
> LCSC lookup of the new part), which this environment cannot reach. **Until then,
> the source of truth for the final MPNs is `MOUSER_ALTERNATES.md` (the 7
> verified) + `BOM_CONSOLIDADO.csv`.**

## ⚠️ Note for whoever assembles the painel — `Q1`/`Q2`

The `kit/MAPA_COLOCACAO.md` of the `painel_2026-09-12_06db1c7` package (and the
Value on the board, in F.Fab) says **`AO3400A`**. The part that came from Mouser
(#40440548) is the **`SI2302CDS-T1-GE3`** (Vishay, marking **N2**). **That's the
one soldered at `Q1`/`Q2`.** Same SOT-23 and same pinout — 1 G · 2 S · 3 D,
verified against Vishay datasheet doc. 68645
(`docs/datasheets/SI2302CDS-T1-GE3.pdf`) on 2026-09-17. Only the name differs; the
map was not edited because it's a stamped artifact of the package that went to the
factory.

## ⚠️ Pots `RV1`/`RV2` of the painel — did NOT come from Mouser

The `COMPRA_MOUSER.csv` v2 lists 4 × `PTV09A-4020F-B103`, but **the pots were not
bought at Mouser** (dB, 2026-09-18): there were pots at home, 16 mm, that don't fit
on the board (pins at 5,0 mm; the board has the rear-mount Bourns PTV09A layout,
pins at 2,5 mm). And per the Bourns PTV09 datasheet neither of the repo's two codes
has a thread: `-4` is **without bushing**, `-1` is **smooth bushing**; only style
`-6` is threaded. To fasten the board by the pots you need a 9 mm pot with a
threaded bushing — see `docs/STATUS.md` (18/09). **10 kΩ linear (B10K).**

## Order status (2026-09-12)

- **Mouser (parts):** **DELIVERED** — Web Order # **40440548**, date
  **2026-09-12**, shipped 14/09 (invoice 92489985). **21 of the rows** of
  `COMPRA_MOUSER.csv` v2 came, all complete; **what did not come**: the
  electrolytics, the WIMA 2,2 µF, the 1×40 strip, the pots, and the encoder. What
  came, bag by bag: `mouser_40440548.csv`.
- **JLCPCB (bare boards):** **PLACED** — Order # **W2026091304545438**, date
  **2026-09-12** (bare boards). Covers **both boards, cerebro and painel**
  — confirmed by dB on 2026-09-17.
  **SHIPPED** (JLC email, 17/09): 5 × `cerebro_2026-09-12_06db1c7` and
  5 × `painel_2026-09-12_06db1c7`, DHL Express Priority (DDP), tracking
  **7099774990**.
