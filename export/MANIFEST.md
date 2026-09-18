<!-- carimbo: MANIFEST.md v6 · 2026-09-18 · placas: painel rev C · cerebro rev C -->
# MANIFEST — índice autoritativo de `export/`

> **Protocolo:** antes de subir/consumir um artefato, confira o **carimbo** dele
> contra este MANIFEST; se não bater, `git pull`. Um carrinho, um upload ou uma
> montagem que parta de um carimbo mais antigo que o daqui está a consumir uma
> cópia superada — a raiz do engano do MPN velho que já custou.

Este é o **único** ponto de "estou com a versão de agora?". O carimbo vive em cada
artefato (primeira linha do CSV/MD, célula `carimbo` no xlsx/xls, `LEIA-ME.txt` nos
pacotes); este MANIFEST diz qual é o carimbo corrente. Os dois têm de bater.

## Compra (Mouser) — arquivos soltos

| Artefato | Versão/stamp | Data | Placa rev | Para quê |
|---|---|---|---|---|
| `COMPRA_MOUSER.csv` | v2 | 2026-09-12 | painel rev C · cerebro rev C | BOM de MPNs reais para **importar no Mouser BOM Tool** |
| `BOM_CONSOLIDADO.csv` | v2 | 2026-09-12 | painel rev C · cerebro rev C | BOM unificada cerebro+painel (LCSC + MPN Mouser + designadores + qty) |
| `MOUSER_ALTERNATES.md` | v2 | 2026-09-12 | painel rev C · cerebro rev C | pick PRIMÁRIO + 2–3 alternativas por peça-problema; as **7 verificadas em estoque** |
| `COMPRA_MOUSER_template.xlsx` | v2 | 2026-09-12 | painel rev C · cerebro rev C | template de importação Mouser (col MPN + nº Mouser preenchido) |
| `COMPRA_MOUSER_template.xls` | v2 | 2026-09-12 | painel rev C · cerebro rev C | idem, formato legado `.xls` |

**v2 = corte dos 7 MPNs verificados em estoque no Mouser (2026-09-12).** O corte
anterior (v1, export inicial) tinha MPNs que foram substituídos por
backorder/no-match e **não deve mais ser consumido** — foi essa cópia velha que
gerou o carrinho com MPN superado.

## Pacotes de fábrica (JLCPCB) — pastas

Estes carregam o seu próprio carimbo no `LEIA-ME.txt` (gerador + data + commit),
que **é** o mecanismo de versão da placa. Stamp = `para_fabrica v0.19.0 · commit 06db1c7`.

| Artefato | Versão/stamp | Data | Placa rev | Para quê |
|---|---|---|---|---|
| `cerebro_2026-09-12_06db1c7/` | para_fabrica v0.19.0 · 06db1c7 | 2026-09-12 | cerebro rev C | pacote de fábrica do cérebro |
| ├ `cerebro_2026-09-12_06db1c7_gerbers.zip` | (do pacote 06db1c7) | 2026-09-12 | cerebro rev C | gerbers+drill para a JLCPCB |
| ├ `bom_cerebro.csv` / `cpl_cerebro.csv` | (do pacote 06db1c7) | 2026-09-12 | cerebro rev C | BOM + colocação (CPL da JLC) |
| ├ `cpl_rico_cerebro.csv` | (do pacote 06db1c7) | 2026-09-12 | cerebro rev C | CPL com Val/Package preservados |
| ├ `VISTORIA.md` | (do pacote 06db1c7) | 2026-09-12 | cerebro rev C | vistoria/portão do pacote |
| └ `kit/` (COMPRA_LCSC.csv · MAPA_COLOCACAO.md · LEIA-ME-KIT.txt) | (do pacote 06db1c7) | 2026-09-12 | cerebro rev C | kit de montagem em casa (carrinho LCSC + mapa) |
| `painel_2026-09-12_06db1c7/` | para_fabrica v0.19.0 · 06db1c7 | 2026-09-12 | painel rev C | pacote de fábrica do painel |
| ├ `painel_2026-09-12_06db1c7_gerbers.zip` | (do pacote 06db1c7) | 2026-09-12 | painel rev C | gerbers+drill para a JLCPCB |
| ├ `bom_painel.csv` / `cpl_painel.csv` | (do pacote 06db1c7) | 2026-09-12 | painel rev C | BOM + colocação (CPL da JLC) |
| ├ `cpl_rico_painel.csv` | (do pacote 06db1c7) | 2026-09-12 | painel rev C | CPL com Val/Package preservados |
| ├ `VISTORIA.md` | (do pacote 06db1c7) | 2026-09-12 | painel rev C | vistoria/portão do pacote |
| └ `kit/` (COMPRA_LCSC.csv · MAPA_COLOCACAO.md · LEIA-ME-KIT.txt) | (do pacote 06db1c7) | 2026-09-12 | painel rev C | kit de montagem em casa (carrinho LCSC + mapa) |

## Fora de `export/` — carimbados, mas com pendência

| Artefato | Versão/stamp | Data | Placa rev | Para quê |
|---|---|---|---|---|
| `docs/datasheets/README.md` | ⚠️ **regenerar online** | 2026-09-08 (fichas.py v0.9.0, `18729f1`) | pré-rev-C-sourcing | índice de fichas; **lista MPNs superados** (PTC `2920L330/24MR`→`MF-LSMF300/24X-2`, MOSFET `AO3400A`→`SI2302CDS-T1-GE3`, filme `MKS2C042201K00KSSD`→`MKS2C042201M00KN00`, IDC 3M `N2526/N3793`→Würth `61202621621`). Casar cada MPN novo à sua **ficha** exige download online — não fazer por substituição cega (ver nota). |
| `tools/codigos_lcsc.json` | ⚠️ **regenerar online** | 2026-09-08 (codigos_lcsc.py v0.2.0, `1ce1d20`) | pré-rev-C-sourcing | mapa LCSC→JLCPCB; MPNs Mouser aqui divergem por caminho (LCSC vs Mouser). O trio `mpn`/`codigo`/`prova` tem de ficar coerente — trocar só a string do `mpn` mente sobre o código LCSC e a prova. Requer lookup LCSC online. |

> **Nota sobre "regenerar online".** Estes dois não são substituição de string
> segura: cada MPN carrega uma **ficha PDF** (README) ou um **par código-LCSC +
> prova** (json) que descrevem a peça antiga. Trocar só o MPN deixaria a ficha/prova
> a falar de outra peça — uma declaração falsa, pior que o valor velho. O conserto
> correcto pede rede (download da ficha nova / lookup LCSC do part novo), que este
> ambiente não alcança. **Até lá, a fonte-da-verdade dos MPNs finais é
> `MOUSER_ALTERNATES.md` (as 7 verificadas) + `BOM_CONSOLIDADO.csv`.**

## ⚠️ Nota para quem monta o painel — `Q1`/`Q2`

O `kit/MAPA_COLOCACAO.md` do pacote `painel_2026-09-12_06db1c7` (e o Value na
placa, em F.Fab) diz **`AO3400A`**. A peça que veio da Mouser (#40440548) é o
**`SI2302CDS-T1-GE3`** (Vishay, marcação **N2**). **É essa que se solda nos
`Q1`/`Q2`.** Mesmo SOT-23 e mesma pinagem — 1 G · 2 S · 3 D, conferida na
ficha Vishay doc. 68645 (`docs/datasheets/SI2302CDS-T1-GE3.pdf`) a 2026-09-17.
Só o nome difere; o mapa não foi editado porque é artefato carimbado do
pacote que foi à fábrica.

## ⚠️ Pots `RV1`/`RV2` do painel — NÃO vieram da Mouser

O `COMPRA_MOUSER.csv` v2 lista 4 × `PTV09A-4020F-B103`, mas **os pots não foram
comprados na Mouser** (dB, 2026-09-18): havia pots em casa, de 16 mm, que não
cabem na placa (pinos a 5,0 mm; a placa tem o layout Bourns PTV09A de montagem
traseira, pinos a 2,5 mm). E pela ficha Bourns PTV09 nenhum dos dois códigos do
repo tem rosca: `-4` é **sem bucim**, `-1` é **bucim liso**; só o estilo `-6` é
roscado. Para prender a placa pelos pots é preciso um pot de 9 mm com bucim
roscado — ver `docs/STATUS.md` (18/09). **10 kΩ linear (B10K).**

## Status de encomenda (2026-09-12)

- **Mouser (peças):** **PLACED** — Web Order # **40440548**, data **2026-09-12**
  (peças/BOM, `COMPRA_MOUSER*`).
- **JLCPCB (placas nuas):** **PLACED** — Order # **W2026091304545438**, data
  **2026-09-12** (placas nuas). Cobre **as duas placas, cérebro e painel**
  — confirmado pelo dB a 2026-09-17.
  **ENVIADO** (email JLC, 17/09): 5 × `cerebro_2026-09-12_06db1c7` e
  5 × `painel_2026-09-12_06db1c7`, DHL Express Priority (DDP), tracking
  **7099774990**.
