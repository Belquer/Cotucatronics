# RELATÓRIO 3D — verifica_3d.py v0.1.0

> carimbo: RELATORIO_3D.md · 2026-09-19 · placa: kicad/cotuca_cerebro/cotuca_cerebro.kicad_pcb · gerado por verifica_3d.py v0.1.0

## §0 Pré-voo (portão, falha fechado)

- ✅ kicad-cli REAL: **10.0.6** via `AppRun.sh` em `/tmp/claude-0/-home-user/5c95766f-27b8-5355-a8ae-af20a107b4e6/scratchpad/AppDir`

## §2 STEP real (AP214, --subst-models, KICAD10_3DMODEL_DIR)

- ✅ exit=0, ficheiro=3.19 MB (ref cérebro ~3.0 MB)
  - envelope conhecido: 2 warning(s) de modelo não-apontado (Fuse_2920/Electrosmith_Daisy_Seed) — esperado a menos que a lib esteja apontada

## §3 Medição (build123d 0.12.0)

- ✅ 56 sólido(s); bbox = 180.0 × 100.0 × 22.02 mm
  - min=(20.0, -120.0, -4.425)  max=(200.0, -20.0, 17.595)

## Asserções 1-3 — RÍGIDAS (computadas do STEP)

| # | asserção | resultado | detalhe |
|---|---|---|---|
| | A1 placa cabe na cavidade | ❌ FALHA | bbox 180.0x100.0 mm; folga x=-60.00 y=-20.00 mm (min 2x0.5) |
| | A2 furos alinham placa↔postes | ⏳ N/D | declared-stub: tol 0.4 mm, min 3 pontos — precisa STEP da caixa (Fase A/B) |
| | A3 altura por zona (cunha 11,8) | ✅ PASSA | daisy_seed: folga Z=3.00 mm; idc_j14: folga Z=3.15 mm; eletroliticos: folga Z=1.40 mm (env-declarado ate STEP da caixa) |

## Asserções 4-6 — DECLARED-ENVELOPE (afirmamos o número, ⟨dB⟩ assina)

| # | asserção | tipo | valor declarado |
|---|---|---|---|
| | A4 recortes tampa (OLED/pots 16mm/encoder) | envelope | janela_oled=True, furos_pots=16.0 mm (Caso B), encoder=True |
| | A5 cabos/serviço (J1↔J14, pots-tampa) | envelope | j1_j14=265.0 mm, pots_casoB=True |
| | A6 montabilidade (parafusos, remoção) | envelope | parafusos_acessiveis=True, removivel_sem_dessoldar=True |

## MANIFEST

- Este relatório e cada `mech/3d/*.step` entram em `export/MANIFEST.md` com carimbo + flag «bbox declarado vs STEP real» (divergiu = vermelho).
