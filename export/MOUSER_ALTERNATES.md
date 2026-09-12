<!-- carimbo: MOUSER_ALTERNATES.md v2 · 2026-09-12 · placas: painel rev C · cerebro rev C -->
# Mouser — substituições e alternativas (rev C)

Mouser está bloqueado por egress deste ambiente. Para cada peça-problema listo o
pick PRIMÁRIO (o que ficou nos arquivos `COMPRA_MOUSER.csv`,
`BOM_CONSOLIDADO.csv` e o template `.xlsx`/`.xls`) e 2–3 alternativas amplamente
estocadas, para que pelo menos uma seja comprável. Footprint é o critério
crítico — todas as alternativas listadas mantêm o mesmo footprint da peça
original.

## ✅ 7 peças verificadas em estoque no Mouser (via navegador, 2026-09-12)

O usuário confirmou estas 7 AO VIVO no Mouser com o filtro "In Stock". São os
picks PRIMÁRIOS definitivos, já gravados nos 4 arquivos de BOM (MPN em ambas as
colunas do template + nº Mouser preenchido). Os picks anteriores viraram
backup e ficam listados nas seções abaixo.

| Ref | Mfr Part # (final) | Fabricante | Nº Mouser | Qty | Spec |
|---|---|---|---|---|---|
| C8 (10µF 1206) | `CL31B106KAHNNWE` | Samsung | 187-CL31B106KAHNNWE | 3 | 10µF 25V X7R 1206 |
| F1 (PTC 2920) | `MF-LSMF300/24X-2` | Bourns | 652-MF-LSMF300/24X-2 | 3 | hold 3A / trip 5.2A / 24V, 2920 |
| C14,C24 (filme 2µ2) | `MKS2C042201M00KN00` | WIMA | 505-MKS2C042201M00KN | 4 | 2.2µF 63V PET, PCM5, 7.2×8.5×14mm |
| J14 + J1 painel (IDC 2x13) | `61202621621` | Würth Elektronik | 710-61202621621 | 4 | 26p 2.54mm boxed vert macho |
| Barra 1x40 header | `61304011121` | Würth Elektronik | 710-61304011121 | 4 | 1x40 2.54mm macho |
| D2 (SS14) | `SS14` | onsemi | 512-SS14 | 3 | 40V 1A SMA/DO-214AC |
| Q1,Q2 (MOSFET) | `SI2302CDS-T1-GE3` | Vishay | 781-SI2302CDS-T1-GE3 | 4 | N-ch 20V logic SOT-23 |

**Notas de engenharia:**
- **Si2302 é 20V** (o AO3400A/DMN3404L eram 30V): OK — Q1/Q2 são drivers
  low-side de LED, Vds ≤ 5V, então 20V sobra.
- **Cap de filme WIMA tem 14mm de altura**: cabe na caixa (altura interna mínima
  ~42mm).
- **PTC listado como 2920/7450 métrico = mesma land 2920** do nosso footprint
  `Fuse_2920_7451Metric` (7451 vs 7450 métrico é a mesma pegada 2920 imperial).

## Alcance ao Mouser deste ambiente (2026-09-12)

- `mouser.com` e `octopart.com` diretos (curl/WebFetch): **BLOQUEADOS** (403,
  política de egress). Não dá para ler estoque ao vivo daqui.
- **WebSearch funciona**: quando a busca devolve uma página `mouser.com/ProductDetail/...`
  isso confirma que **o Mouser CATALOGA a peça** (a quantidade em estoque continua
  invisível — o usuário confirma o verde de estoque na própria tela).
- Regra adotada: preferir fabricantes que o Mouser certamente lista (Yageo,
  KEMET, Murata, TDK, Vishay, onsemi, Nexperia, Diodes, TE, Amphenol, Würth,
  Bourns, Littelfuse, Panasonic, Samsung). Foi por catalogação que falharam
  AOS `AO3400A`, WIMA `MKS2C042201...`, 3M `N2513`, Sullins `PRPC`, Diodes `SS14-13-F`.
- **Página de produto Mouser confirmada via busca**: `DMN3404L-7` (Diodes),
  `SS14` (onsemi), `2920L330/24MR` (Littelfuse), `61304011121` (Würth). Demais
  itens: fabricante é catalogado pelo Mouser, código exato/estoque a confirmar.
- **NÃO afirmo estoque verificado** para nenhum item — nenhuma fonte de estoque
  ao vivo foi alcançada.

### URLs de busca no Mouser (o usuário clica e escolhe item com estoque verde)

- MOSFET N-ch SOT-23 30V: `https://www.mouser.com/c/?q=DMN3404L-7`
- PTC 2920 ~3A 24V: `https://www.mouser.com/c/?q=2920L%2024V%20resettable%20fuse`
- Filme 2.2µF 5mm: `https://www.mouser.com/c/passive-components/capacitors/film-capacitors/?capacitance=2.2+uF` (filtrar passo 5mm, 50–63V)
- Box header IDC 2x13 2.54mm: `https://www.mouser.com/c/?q=box%20header%202x13%202.54mm%20vertical`
- Barra de pinos 1x40 2.54mm: `https://www.mouser.com/c/?q=61304011121`
- Schottky SMA 40V 1A: `https://www.mouser.com/c/?q=SS14%20onsemi`

## Peças-problema (No Match / backorder no Mouser)

### 1. 10µF 1206 (cerebro C8) — footprint `C_1206_3216Metric`
- Original: `CL31A106KBHNNNE` (Samsung, 50V X5R) — long lead / backorder.
- **PRIMÁRIO ✅ verificado em estoque no Mouser (via navegador, 2026-09-12): `CL31B106KAHNNWE`** (Samsung, 10µF 25V X7R, 1206) — nº Mouser 187-CL31B106KAHNNWE.
- Backup: `GRM31CR61E106KA12L` (Murata, 10µF 25V X5R, 1206).
- Alt: `CL31B106KBHNNNE` (Samsung, 10µF 50V X7R, 1206 — upgrade de dielétrico, mesmo fabricante/família do original).
- Alt: `C3216X5R1E106K160AB` (TDK, 10µF 25V X5R, 1206).
- Alt: `TMK316ABJ106KL-T` (Taiyo Yuden, 10µF 25V X7R, 1206).
- Nota: tensão caiu para 25V (permitido); todos 1206 X5R/X7R.

### 2. PTC resetável 2920 (cerebro F1) — footprint `Fuse_2920_7451Metric`
- Anterior: `2920L300/24MR` (Littelfuse, 3.0A/24V) — **não aparece no Mouser** (o código /300/24 não é catalogado).
- **PRIMÁRIO ✅ verificado em estoque no Mouser (via navegador, 2026-09-12): `MF-LSMF300/24X-2`** (Bourns MF-LSMF, hold 3A / trip 5.2A / 24V, 2920/7450 métrico) — nº Mouser 652-MF-LSMF300/24X-2. Land 2920/7450 métrico = mesma pegada 2920 do footprint `Fuse_2920_7451Metric`.
- Backup: `2920L330/24MR` (Littelfuse 2920L, 3.3A hold, 24V) — página de produto Mouser confirmada via busca; 3.3A dentro da faixa aceita (2.5–3.5A).
- Alt: `2920L200/24DR` (Littelfuse, 2.0A/24V, 2920) — só se o /330 estiver esgotado e a carga permitir 2.0A de hold.
- Alt: Bourns Multifuse série 2920 24V ~3A (conferir na tela o código 2920 que o Mouser lista).
- ⚠️ NÃO usar Bel Fuse `0ZCF0300FF2C` — é **6V** (série 0ZCF de baixa tensão); não cobre 24V. As séries Bel 2920 de ~3A catalogadas são de baixa tensão.
- Nota: NÃO trocar de pacote (fica 2920). Busca: `https://www.mouser.com/c/?q=2920L%2024V%20resettable%20fuse`

### 3. Box header IDC 2x13 26p (cerebro J14 + painel J1) — footprint `IDC-Header_2x13_P2.54mm_Vertical`
- Original: `N2513-6002RB` (3M) — No Match no Mouser.
- **PRIMÁRIO ✅ verificado em estoque no Mouser (via navegador, 2026-09-12): `61202621621`** (Würth Elektronik WR-BHD, 2x13 26p, 2.54mm, THT vertical, blindado/box, macho) — nº Mouser 710-61202621621.
- Alt: `BHR-26-VUA` (Adam Tech, box header 2x13 vertical 2.54mm).
- Alt: `3020-26-0100-00` (CNC Tech, box header 2x13 vertical 2.54mm).
- Alt: `71922-126LF` (Amphenol ICC/FCI, box header 2x13 vertical 2.54mm).
- Nota: qualquer box header 2x13 2.54mm THT vertical casa o footprint. Alternativas Adam Tech/CNC/Amphenol não conferidas nº-a-nº. Würth é catalogado pelo Mouser; código exato/estoque a confirmar. Busca: `https://www.mouser.com/c/?q=box%20header%202x13%202.54mm%20vertical`

### 4. Barra 1x40 macho 2.54mm THT (todos os headers 1 fila) — footprint `PinHeader_1xNN_P2.54mm_Vertical`
- Original: `PRPC040SAAN-RC` (Sullins) — No Match no Mouser.
- **PRIMÁRIO ✅ verificado em estoque no Mouser (via navegador, 2026-09-12): `61304011121`** (Würth Elektronik WR-PHD, 1x40, 2.54mm, THT vertical, macho) — nº Mouser 710-61304011121.
- Alt: `68000-140HLF` (Amphenol ICC/FCI, 1x40 2.54mm breakaway macho).
- Alt: `PREC040SAAN-RC` (Sullins, mesma barra com flash de ouro).
- Alt: `PH1-40-UA` (Adam Tech, 1x40 2.54mm macho).
- Nota: barra cortada serve J3/J4/J5/J6/J11/J12/J13/J15/J16 do cerebro + J2 (OLED 1x7) do painel. `61304011121` (Würth) tem **página de produto Mouser confirmada via busca**; estoque a confirmar. Busca: `https://www.mouser.com/c/?q=61304011121`

### 5. Schottky SMA SS14 (cerebro D2) — footprint `D_SMA` (DO-214AC)
- Original: `SS14-13-F` (Diodes Inc) — No Match no Mouser.
- **PRIMÁRIO ✅ verificado em estoque no Mouser (via navegador, 2026-09-12): `SS14`** (onsemi, Schottky 40V 1A, DO-214AC/SMA) — nº Mouser 512-SS14.
- Alt: `SK14-E3/61T` (Vishay, 40V 1A, DO-214AC/SMA).
- Alt: `SS14-TP` (Micro Commercial Co, 40V 1A, SMA).
- Alt: `SS14L-13-F` (Diodes Inc, 40V 1A, DO-214AC).
- ⚠️ NÃO usar `SS14FL` (onsemi) — é pacote SMAF (flat-lead) e **não casa** o footprint D_SMA.
- Mouser: página de produto de `SS14` (onsemi) **confirmada via busca**. Busca: `https://www.mouser.com/c/?q=SS14%20onsemi`

## Rechecagem das 3 peças "confirmar estoque"

### 22nF C0G 1206 (cerebro C12,C22) — footprint `C_1206_3216Metric`
- Mantido: **`C1206C223J5GACTU`** (Kemet, 22nF 50V C0G/NP0, 1206) — peça correta, família Kemet C0G 1206 amplamente estocada no Mouser. C0G obrigatório.
- Alt (se faltar): `CGA5L2C0G1H223J160AA` (TDK, 22nF 50V C0G, 1206), `GCM31CR71H223KA55` (Murata, 22nF 50V C0G, 1206).

### Filme 2.2µF (cerebro C14,C24) — footprint `C_Rect_L7.2mm_W11.0mm_P5.00mm_MKS2`
- Anterior: `MKS2C042201F00KSSD` (WIMA MKS2, variante "C") — **No Match no Mouser**.
- **PRIMÁRIO ✅ verificado em estoque no Mouser (via navegador, 2026-09-12): `MKS2C042201M00KN00`** (WIMA MKS2, 2.2µF 63V PET, PCM5, corpo 7.2×8.5×14mm) — nº Mouser 505-MKS2C042201M00KN. Altura 14mm cabe na caixa (altura interna mínima ~42mm). Passo 5mm nativo do footprint.
- Backup: `MKS2B042201F00KSSD` (WIMA MKS2, 2.2µF 50V, passo 5mm).
- ⚠️ **Kemet R82 2.2µF NÃO cabe no P5**: a 2.2µF o R82 sobe para passo 7.5mm+ (ex.: `R82EC2200...` usa o passo "EC"), então **não** serve de drop-in aqui apesar de o Kemet ser bem estocado. Só usar R82 se o footprint mudar para o passo maior.
- Alt mesmo corpo P5 (fabricantes catalogados pelo Mouser — **conferir corpo contra 7.2×11 P5**, corpo de filme cresce com a tensão): Vishay MKT série BFC2378 2.2µF, EPCOS/TDK B32 2.2µF.
- Busca: `https://www.mouser.com/c/passive-components/capacitors/film-capacitors/?capacitance=2.2+uF` (filtrar passo 5mm, 50–63V, filme).

### MOSFET N-ch SOT-23 (painel Q1,Q2) — footprint `SOT-23`
- Anterior: `AO3400A` (Alpha & Omega) — **No Match no Mouser** (o Mouser não cataloga a AOS); mantido só como referência de equivalência.
- **PRIMÁRIO ✅ verificado em estoque no Mouser (via navegador, 2026-09-12): `SI2302CDS-T1-GE3`** (Vishay, N-ch logic-level 20V, SOT-23) — nº Mouser 781-SI2302CDS-T1-GE3. 20V OK: Q1/Q2 são drivers low-side de LED, Vds ≤ 5V.
- Backup: `DMN3404L-7` (Diodes Incorporated, N-ch logic-level 30V, SOT-23) — página de produto Mouser confirmada via busca; drop-in do AO3400A, mesmo footprint SOT-23.
- Alt: `DMG3404L-7` (Diodes Incorporated, N-ch logic-level 30V, SOT-23).
- Alt: `NTR4003NT1G` (onsemi, N-ch logic-level 30V, SOT-23).
- Busca: `https://www.mouser.com/c/?q=DMN3404L-7`
