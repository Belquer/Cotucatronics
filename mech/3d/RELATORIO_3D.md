# 3D REPORT — verifica_3d.py v0.1.0

> carimbo: RELATORIO_3D.md · 2026-09-19 · placa: kicad/cotuca_cerebro/cotuca_cerebro.kicad_pcb · gerado por verifica_3d.py v0.1.0

## §0 Pre-flight (gate, fail-closed)

- ✅ real kicad-cli: **10.0.6** via `AppRun.sh` in `/tmp/claude-0/-home-user/5c95766f-27b8-5355-a8ae-af20a107b4e6/scratchpad/AppDir`

## §2 Real STEP (AP214, --subst-models, KICAD10_3DMODEL_DIR)

- ✅ exit=0, file=3.19 MB (ref brain board ~3.0 MB)
  - known envelope: 2 warning(s) for an unpointed model (Fuse_2920/Electrosmith_Daisy_Seed) — expected unless the lib is pointed

## §3 Measurement (build123d 0.12.0)

- ✅ 56 solid(s); bbox = 180.0 × 100.0 × 22.02 mm
  - min=(20.0, -120.0, -4.425)  max=(200.0, -20.0, 17.595)

## Assertions 1-3 — RIGID (computed from the STEP)

| # | assertion | result | detail |
|---|---|---|---|
| | A1 board fits in the cavity | ❌ FAIL | bbox 180.0x100.0 mm; clearance x=-60.00 y=-20.00 mm (min 2x0.5) |
| | A2 holes align board↔posts | ⏳ N/A | declared-stub: tol 0.4 mm, min 3 points — needs enclosure STEP (Phase A/B) |
| | A3 height per zone (wedge 11.8) | ✅ PASS | daisy_seed: clearance Z=3.00 mm; idc_j14: clearance Z=3.15 mm; electrolytics: clearance Z=1.40 mm (env-declared until the enclosure STEP) |

## Assertions 4-6 — DECLARED-ENVELOPE (we state the number, ⟨dB⟩ signs off)

| # | assertion | type | declared value |
|---|---|---|---|
| | A4 lid cutouts (OLED/16mm pots/encoder) | envelope | janela_oled=True, furos_pots=16.0 mm (Case B), encoder=True |
| | A5 cables/service (J1↔J14, pots-to-lid) | envelope | j1_j14=265.0 mm, pots_casoB=True |
| | A6 assemblability (screws, removal) | envelope | parafusos_acessiveis=True, removivel_sem_dessoldar=True |

## MANIFEST

- This report and each `mech/3d/*.step` go into `export/MANIFEST.md` with a stamp + a «declared bbox vs real STEP» flag (diverged = red).
