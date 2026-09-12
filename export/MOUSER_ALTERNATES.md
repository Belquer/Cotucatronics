# Mouser — substituições e alternativas (rev C)

Mouser está bloqueado por egress deste ambiente, então **estoque ao vivo não foi
verificado**. Para cada peça-problema listo o pick PRIMÁRIO (o que ficou nos
arquivos `COMPRA_MOUSER.csv`, `BOM_CONSOLIDADO.csv` e o template `.xlsx`/`.xls`)
e 2–3 alternativas amplamente estocadas, para que pelo menos uma seja
comprável. Footprint é o critério crítico — todas as alternativas listadas
mantêm o mesmo footprint da peça original.

## Peças-problema (No Match / backorder no Mouser)

### 1. 10µF 1206 (cerebro C8) — footprint `C_1206_3216Metric`
- Original: `CL31A106KBHNNNE` (Samsung, 50V X5R) — long lead / backorder.
- **PRIMÁRIO: `GRM31CR61E106KA12L`** (Murata, 10µF **25V** X5R, 1206).
- Alt: `CL31B106KBHNNNE` (Samsung, 10µF 50V X7R, 1206 — upgrade de dielétrico, mesmo fabricante/família do original).
- Alt: `C3216X5R1E106K160AB` (TDK, 10µF 25V X5R, 1206).
- Alt: `TMK316ABJ106KL-T` (Taiyo Yuden, 10µF 25V X7R, 1206).
- Nota: tensão caiu para 25V (permitido); todos 1206 X5R/X7R.

### 2. PTC resetável 2920 (cerebro F1) — footprint `Fuse_2920_7451Metric`
- Original: `2920L330/24MR` (Littelfuse, 3.3A hold, 24V) — backorder.
- **PRIMÁRIO: `2920L300/24MR`** (Littelfuse, 3.0A hold, 24V) — mesma família 2920L, footprint idêntico.
- Alt: `MF-LSMF300/24X-2` (Bourns Multifuse, 3.0A hold, 24V, 2920).
- Alt: `0ZCJ0300FF2E` (Bel Fuse, ~3.0A, 24V, 2920).
- Alt: `SMD300F/24-2` (Littelfuse, 3.0A, 24V, 2920).
- Nota: hold 3.0A dentro da faixa aceita (3.0–3.5A). NÃO trocar de pacote.

### 3. Box header IDC 2x13 26p (cerebro J14 + painel J1) — footprint `IDC-Header_2x13_P2.54mm_Vertical`
- Original: `N2513-6002RB` (3M) — No Match no Mouser.
- **PRIMÁRIO: `61202621621`** (Würth Elektronik WR-BHD, 2x13 26p, 2.54mm, THT vertical, blindado/box, macho) — spec confirmada em Newark/DigiKey/we-online.
- Alt: `BHR-26-VUA` (Adam Tech, box header 2x13 vertical 2.54mm).
- Alt: `3020-26-0100-00` (CNC Tech, box header 2x13 vertical 2.54mm).
- Alt: `71922-126LF` (Amphenol ICC/FCI, box header 2x13 vertical 2.54mm).
- Nota: qualquer box header 2x13 2.54mm THT vertical casa o footprint. Alternativas Adam Tech/CNC/Amphenol não conferidas nº-a-nº.

### 4. Barra 1x40 macho 2.54mm THT (todos os headers 1 fila) — footprint `PinHeader_1xNN_P2.54mm_Vertical`
- Original: `PRPC040SAAN-RC` (Sullins) — No Match no Mouser.
- **PRIMÁRIO: `61304011121`** (Würth Elektronik WR-PHD, 1x40, 2.54mm, THT vertical, macho) — spec confirmada em Newark/DigiKey/Farnell.
- Alt: `68000-140HLF` (Amphenol ICC/FCI, 1x40 2.54mm breakaway macho).
- Alt: `PREC040SAAN-RC` (Sullins, mesma barra com flash de ouro).
- Alt: `PH1-40-UA` (Adam Tech, 1x40 2.54mm macho).
- Nota: barra cortada serve J3/J4/J5/J6/J11/J12/J13/J15/J16 do cerebro + J2 (OLED 1x7) do painel.

### 5. Schottky SMA SS14 (cerebro D2) — footprint `D_SMA` (DO-214AC)
- Original: `SS14-13-F` (Diodes Inc) — No Match no Mouser.
- **PRIMÁRIO: `SS14`** (onsemi, Schottky 40V 1A, DO-214AC/SMA).
- Alt: `SK14-E3/61T` (Vishay, 40V 1A, DO-214AC/SMA).
- Alt: `SS14-TP` (Micro Commercial Co, 40V 1A, SMA).
- Alt: `SS14L-13-F` (Diodes Inc, 40V 1A, DO-214AC).
- ⚠️ NÃO usar `SS14FL` (onsemi) — é pacote SMAF (flat-lead) e **não casa** o footprint D_SMA.

## Rechecagem das 3 peças "confirmar estoque"

### 22nF C0G 1206 (cerebro C12,C22) — footprint `C_1206_3216Metric`
- Mantido: **`C1206C223J5GACTU`** (Kemet, 22nF 50V C0G/NP0, 1206) — peça correta, família Kemet C0G 1206 amplamente estocada no Mouser. C0G obrigatório.
- Alt (se faltar): `CGA5L2C0G1H223J160AA` (TDK, 22nF 50V C0G, 1206), `GCM31CR71H223KA55` (Murata, 22nF 50V C0G, 1206).

### WIMA 2.2µF filme (cerebro C14,C24) — footprint `C_Rect_L7.2mm_W11.0mm_P5.00mm_MKS2`
- Mantido: **`MKS2C042201F00KSSD`** (WIMA MKS2, 2.2µF, passo 5mm) — footprint é específico do corpo MKS2, então mantido para não quebrar o footprint.
- Atenção: há relatos de restrição de fornecimento na linha MKS2. Alt de mesmo corpo: `MKS2B042201F00KSSD` (WIMA MKS2, mesmo corpo, código de tensão/tol alternativo). Alt Kemet R82 2.2µF 5mm — **conferir corpo contra o footprint MKS2 antes de substituir** (o corpo Kemet difere levemente).

### AO3400A MOSFET N-ch SOT-23 (painel Q1,Q2) — footprint `SOT-23`
- Mantido: **`AO3400A`** (Alpha & Omega, 30V logic-level, SOT-23) — um dos MOSFETs SOT-23 mais estocados; sem indício de ruptura.
- Fallback (mesmo footprint SOT-23): `DMN3404L-7` (Diodes Inc, 30V logic-level).
