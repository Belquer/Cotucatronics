# Lista de roteamento — cerebro

Gerada por `kicad/lista_roteamento.py` v0.3.1 em **2026-09-07**, commit `6acc1e8`.

**8 ligações**, mais **0 pad(s) que perderam o plano** — 8 no total, que é o que o DRC diz. Tudo o resto já está feito e o DRC dá geometria zero com os planos inteiros, o que quer dizer que **o que desenhares tem de continuar a dar isso**.

Depois de as fazeres, o portão diz se ficou bom:

```bash
python3 kicad/para_fabrica.py cerebro
```

## alta impedancia

*curtas, sobre a ilha de AGND, sem digital em paralelo (P5)*

**`Net-(J6-Pad1)`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J6` (26.0, 64.0) → `R22` (68.6, 64.0) · 42.6 mm

## terra

*o plano fez o resto; estas ficaram por fora dele*

**`AGND`** — 4 ligação(ões) · **0.25 mm**
- [ ] `pista` (69.5, 41.1) → `J5` (69.5, 38.5) · 2.5 mm
- [ ] `pista` (74.6, 55.9) → `NT1` (73.0, 81.0) · 25.2 mm
- [ ] `pista` (29.0, 66.5) → `pista` (39.6, 63.0) · 11.1 mm
- [ ] `NT1` (73.0, 81.0) → `pista` (71.5, 92.1) · 11.2 mm

**`GND`** — 3 ligação(ões) · **0.25 mm**
- [ ] `pista` (75.4, 26.5) → `J14` (67.9, 29.0) · 7.9 mm
- [ ] `J14` (40.0, 29.0) → `pista` (42.5, 29.0) · 2.5 mm
- [ ] `J14` (55.2, 29.0) → `J14` (67.9, 29.0) · 12.7 mm

