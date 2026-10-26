<!-- carimbo: MOUSER_ALTERNATES.md v2 · 2026-09-12 · placas: painel rev C · cerebro rev C -->
# Mouser — substitutions and alternatives (rev C)

Mouser is blocked by egress from this environment. For each problem part I list
the PRIMARY pick (the one that ended up in the files `COMPRA_MOUSER.csv`,
`BOM_CONSOLIDADO.csv`, and the `.xlsx`/`.xls` template) and 2–3 widely stocked
alternatives, so that at least one is purchasable. Footprint is the critical
criterion — all the alternatives listed keep the same footprint as the original
part.

## ✅ 7 parts verified in stock at Mouser (via browser, 2026-09-12)

The user confirmed these 7 LIVE at Mouser with the "In Stock" filter. They are the
definitive PRIMARY picks, already written into the 4 BOM files (MPN in both
columns of the template + Mouser no. filled in). The earlier picks became backups
and are listed in the sections below.

| Ref | Mfr Part # (final) | Manufacturer | Mouser No. | Qty | Spec |
|---|---|---|---|---|---|
| C8 (10µF 1206) | `CL31B106KAHNNWE` | Samsung | 187-CL31B106KAHNNWE | 3 | 10µF 25V X7R 1206 |
| F1 (PTC 2920) | `MF-LSMF300/24X-2` | Bourns | 652-MF-LSMF300/24X-2 | 3 | hold 3A / trip 5.2A / 24V, 2920 |
| C14,C24 (film 2µ2) | `MKS2C042201M00KN00` | WIMA | 505-MKS2C042201M00KN | 4 | 2.2µF 63V PET, PCM5, 7.2×8.5×14mm |
| J14 + J1 painel (IDC 2x13) | `61202621621` | Würth Elektronik | 710-61202621621 | 4 | 26p 2.54mm boxed vert male |
| 1x40 header strip | `61304011121` | Würth Elektronik | 710-61304011121 | 4 | 1x40 2.54mm male |
| D2 (SS14) | `SS14` | onsemi | 512-SS14 | 3 | 40V 1A SMA/DO-214AC |
| Q1,Q2 (MOSFET) | `SI2302CDS-T1-GE3` | Vishay | 781-SI2302CDS-T1-GE3 | 4 | N-ch 20V logic SOT-23 |

**Engineering notes:**
- **Si2302 is 20V** (the AO3400A/DMN3404L were 30V): OK — Q1/Q2 are low-side LED
  drivers, Vds ≤ 5V, so 20V is plenty.
- **WIMA film cap is 14mm tall**: fits in the enclosure (minimum internal height
  ~42mm).
- **PTC listed as 2920/7450 metric = same 2920 land** as our footprint
  `Fuse_2920_7451Metric` (7451 vs 7450 metric is the same 2920 imperial
  footprint).

## Reach to Mouser from this environment (2026-09-12)

- `mouser.com` and `octopart.com` directly (curl/WebFetch): **BLOCKED** (403,
  egress policy). Can't read live stock from here.
- **WebSearch works**: when the search returns a `mouser.com/ProductDetail/...`
  page that confirms **Mouser CATALOGS the part** (the quantity in stock remains
  invisible — the user confirms the stock green on their own screen).
- Rule adopted: prefer manufacturers Mouser surely lists (Yageo, KEMET, Murata,
  TDK, Vishay, onsemi, Nexperia, Diodes, TE, Amphenol, Würth, Bourns, Littelfuse,
  Panasonic, Samsung). It was over cataloging that AOS `AO3400A`, WIMA
  `MKS2C042201...`, 3M `N2513`, Sullins `PRPC`, Diodes `SS14-13-F` failed.
- **Mouser product page confirmed via search**: `DMN3404L-7` (Diodes), `SS14`
  (onsemi), `2920L330/24MR` (Littelfuse), `61304011121` (Würth). Remaining items:
  the manufacturer is cataloged by Mouser, exact code/stock to be confirmed.
- **I do NOT claim verified stock** for any item — no live stock source was
  reached.

### Mouser search URLs (the user clicks and chooses an item with stock green)

- MOSFET N-ch SOT-23 30V: `https://www.mouser.com/c/?q=DMN3404L-7`
- PTC 2920 ~3A 24V: `https://www.mouser.com/c/?q=2920L%2024V%20resettable%20fuse`
- Film 2.2µF 5mm: `https://www.mouser.com/c/passive-components/capacitors/film-capacitors/?capacitance=2.2+uF` (filter pitch 5mm, 50–63V)
- Box header IDC 2x13 2.54mm: `https://www.mouser.com/c/?q=box%20header%202x13%202.54mm%20vertical`
- 1x40 2.54mm pin header: `https://www.mouser.com/c/?q=61304011121`
- Schottky SMA 40V 1A: `https://www.mouser.com/c/?q=SS14%20onsemi`

## Problem parts (No Match / backorder at Mouser)

### 1. 10µF 1206 (cerebro C8) — footprint `C_1206_3216Metric`
- Original: `CL31A106KBHNNNE` (Samsung, 50V X5R) — long lead / backorder.
- **PRIMARY ✅ verified in stock at Mouser (via browser, 2026-09-12): `CL31B106KAHNNWE`** (Samsung, 10µF 25V X7R, 1206) — Mouser no. 187-CL31B106KAHNNWE.
- Backup: `GRM31CR61E106KA12L` (Murata, 10µF 25V X5R, 1206).
- Alt: `CL31B106KBHNNNE` (Samsung, 10µF 50V X7R, 1206 — dielectric upgrade, same manufacturer/family as the original).
- Alt: `C3216X5R1E106K160AB` (TDK, 10µF 25V X5R, 1206).
- Alt: `TMK316ABJ106KL-T` (Taiyo Yuden, 10µF 25V X7R, 1206).
- Note: voltage dropped to 25V (allowed); all 1206 X5R/X7R.

### 2. Resettable PTC 2920 (cerebro F1) — footprint `Fuse_2920_7451Metric`
- Previous: `2920L300/24MR` (Littelfuse, 3.0A/24V) — **doesn't appear at Mouser** (the /300/24 code isn't cataloged).
- **PRIMARY ✅ verified in stock at Mouser (via browser, 2026-09-12): `MF-LSMF300/24X-2`** (Bourns MF-LSMF, hold 3A / trip 5.2A / 24V, 2920/7450 metric) — Mouser no. 652-MF-LSMF300/24X-2. 2920/7450 metric land = same 2920 footprint as `Fuse_2920_7451Metric`.
- Backup: `2920L330/24MR` (Littelfuse 2920L, 3.3A hold, 24V) — Mouser product page confirmed via search; 3.3A within the accepted range (2.5–3.5A).
- Alt: `2920L200/24DR` (Littelfuse, 2.0A/24V, 2920) — only if the /330 is sold out and the load allows 2.0A of hold.
- Alt: Bourns Multifuse 2920 series 24V ~3A (check on screen the 2920 code Mouser lists).
- ⚠️ Do NOT use Bel Fuse `0ZCF0300FF2C` — it's **6V** (low-voltage 0ZCF series); doesn't cover 24V. The cataloged Bel 2920 ~3A series are low-voltage.
- Note: do NOT change package (stays 2920). Search: `https://www.mouser.com/c/?q=2920L%2024V%20resettable%20fuse`

### 3. Box header IDC 2x13 26p (cerebro J14 + painel J1) — footprint `IDC-Header_2x13_P2.54mm_Vertical`
- Original: `N2513-6002RB` (3M) — No Match at Mouser.
- **PRIMARY ✅ verified in stock at Mouser (via browser, 2026-09-12): `61202621621`** (Würth Elektronik WR-BHD, 2x13 26p, 2.54mm, THT vertical, shielded/box, male) — Mouser no. 710-61202621621.
- Alt: `BHR-26-VUA` (Adam Tech, box header 2x13 vertical 2.54mm).
- Alt: `3020-26-0100-00` (CNC Tech, box header 2x13 vertical 2.54mm).
- Alt: `71922-126LF` (Amphenol ICC/FCI, box header 2x13 vertical 2.54mm).
- Note: any 2x13 2.54mm THT vertical box header matches the footprint. Adam Tech/CNC/Amphenol alternatives not checked number-by-number. Würth is cataloged by Mouser; exact code/stock to be confirmed. Search: `https://www.mouser.com/c/?q=box%20header%202x13%202.54mm%20vertical`

### 4. 1x40 male strip 2.54mm THT (all 1-row headers) — footprint `PinHeader_1xNN_P2.54mm_Vertical`
- Original: `PRPC040SAAN-RC` (Sullins) — No Match at Mouser.
- **PRIMARY ✅ verified in stock at Mouser (via browser, 2026-09-12): `61304011121`** (Würth Elektronik WR-PHD, 1x40, 2.54mm, THT vertical, male) — Mouser no. 710-61304011121.
- Alt: `68000-140HLF` (Amphenol ICC/FCI, 1x40 2.54mm breakaway male).
- Alt: `PREC040SAAN-RC` (Sullins, same strip with gold flash).
- Alt: `PH1-40-UA` (Adam Tech, 1x40 2.54mm male).
- Note: a cut strip serves J3/J4/J5/J6/J11/J12/J13/J15/J16 of the cerebro + J2 (OLED 1x7) of the painel. `61304011121` (Würth) has a **Mouser product page confirmed via search**; stock to be confirmed. Search: `https://www.mouser.com/c/?q=61304011121`

### 5. Schottky SMA SS14 (cerebro D2) — footprint `D_SMA` (DO-214AC)
- Original: `SS14-13-F` (Diodes Inc) — No Match at Mouser.
- **PRIMARY ✅ verified in stock at Mouser (via browser, 2026-09-12): `SS14`** (onsemi, Schottky 40V 1A, DO-214AC/SMA) — Mouser no. 512-SS14.
- Alt: `SK14-E3/61T` (Vishay, 40V 1A, DO-214AC/SMA).
- Alt: `SS14-TP` (Micro Commercial Co, 40V 1A, SMA).
- Alt: `SS14L-13-F` (Diodes Inc, 40V 1A, DO-214AC).
- ⚠️ Do NOT use `SS14FL` (onsemi) — it's an SMAF (flat-lead) package and **doesn't match** the D_SMA footprint.
- Mouser: product page for `SS14` (onsemi) **confirmed via search**. Search: `https://www.mouser.com/c/?q=SS14%20onsemi`

## Recheck of the 3 "confirm stock" parts

### 22nF C0G 1206 (cerebro C12,C22) — footprint `C_1206_3216Metric`
- Kept: **`C1206C223J5GACTU`** (Kemet, 22nF 50V C0G/NP0, 1206) — correct part, the Kemet C0G 1206 family widely stocked at Mouser. C0G mandatory.
- Alt (if unavailable): `CGA5L2C0G1H223J160AA` (TDK, 22nF 50V C0G, 1206), `GCM31CR71H223KA55` (Murata, 22nF 50V C0G, 1206).

### Film 2.2µF (cerebro C14,C24) — footprint `C_Rect_L7.2mm_W11.0mm_P5.00mm_MKS2`
- Previous: `MKS2C042201F00KSSD` (WIMA MKS2, "C" variant) — **No Match at Mouser**.
- **PRIMARY ✅ verified in stock at Mouser (via browser, 2026-09-12): `MKS2C042201M00KN00`** (WIMA MKS2, 2.2µF 63V PET, PCM5, body 7.2×8.5×14mm) — Mouser no. 505-MKS2C042201M00KN. 14mm height fits in the enclosure (minimum internal height ~42mm). 5mm pitch native to the footprint.
- Backup: `MKS2B042201F00KSSD` (WIMA MKS2, 2.2µF 50V, 5mm pitch).
- ⚠️ **Kemet R82 2.2µF does NOT fit the P5**: at 2.2µF the R82 goes up to 7.5mm+ pitch (e.g., `R82EC2200...` uses the "EC" pitch), so it does **not** serve as a drop-in here even though Kemet is well stocked. Only use R82 if the footprint changes to the larger pitch.
- Alt same P5 body (manufacturers cataloged by Mouser — **check body against 7.2×11 P5**, a film body grows with voltage): Vishay MKT BFC2378 series 2.2µF, EPCOS/TDK B32 2.2µF.
- Search: `https://www.mouser.com/c/passive-components/capacitors/film-capacitors/?capacitance=2.2+uF` (filter pitch 5mm, 50–63V, film).

### MOSFET N-ch SOT-23 (painel Q1,Q2) — footprint `SOT-23`
- Previous: `AO3400A` (Alpha & Omega) — **No Match at Mouser** (Mouser doesn't catalog AOS); kept only as an equivalence reference.
- **PRIMARY ✅ verified in stock at Mouser (via browser, 2026-09-12): `SI2302CDS-T1-GE3`** (Vishay, N-ch logic-level 20V, SOT-23) — Mouser no. 781-SI2302CDS-T1-GE3. 20V OK: Q1/Q2 are low-side LED drivers, Vds ≤ 5V.
- Backup: `DMN3404L-7` (Diodes Incorporated, N-ch logic-level 30V, SOT-23) — Mouser product page confirmed via search; drop-in for the AO3400A, same SOT-23 footprint.
- Alt: `DMG3404L-7` (Diodes Incorporated, N-ch logic-level 30V, SOT-23).
- Alt: `NTR4003NT1G` (onsemi, N-ch logic-level 30V, SOT-23).
- Search: `https://www.mouser.com/c/?q=DMN3404L-7`
