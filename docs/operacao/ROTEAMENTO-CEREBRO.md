# Lista de roteamento — cerebro

Gerada por `kicad/lista_roteamento.py` v0.3.1 em **2026-09-08**, commit `4a17a0e`.  
⚠️ **Não comitado**: `kicad/cotuca_cerebro/cotuca_cerebro.kicad_pcb`, `kicad/pcb_contorno.py`, `kicad/roteia_fr.py`, `kicad/sincroniza_pcb.py`, `kicad/compara_netlist.py` — a placa pode não corresponder a este commit.

**12 ligações**, mais **3 pad(s) que perderam o plano** — 15 no total, que é o que o DRC diz. Tudo o resto já está feito e o DRC dá geometria zero com os planos inteiros, o que quer dizer que **o que desenhares tem de continuar a dar isso**.

Depois de as fazeres, o portão diz se ficou bom:

```bash
python3 kicad/para_fabrica.py cerebro
```

## trilhos de alimentacao

*largas, e primeiro que as de sinal: ocupam mais espaco*

**`+5V`** — 1 ligação(ões) · **0.50 mm**
- [ ] `pista` (115.7, 72.1) → `J14` (52.7, 29.0) · 76.3 mm

**`+5V_PONTE`** — 1 ligação(ões) · **0.50 mm**
- [ ] `pista` (167.3, 71.7) → `U3` (77.4, 114.9) · 99.8 mm

## audio

*afastadas do digital, e de preferencia sobre a ilha*

**`LINHA_R`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J16` (118.0, 80.5) → `pista` (116.8, 89.3) · 8.9 mm

## terra

*o plano fez o resto; estas ficaram por fora dele*

**`AGND`** — 4 ligação(ões) · **0.25 mm**
- [ ] `pista` (74.6, 59.6) → `J5` (69.5, 41.1) · 19.2 mm
- [ ] `J6` (26.0, 66.5) → `pista` (39.6, 63.0) · 14.0 mm
- [ ] `NT1` (73.0, 81.0) → `pista` (74.6, 59.6) · 21.4 mm
- [ ] `pista` (71.5, 92.1) → `NT1` (73.0, 81.0) · 11.2 mm

**`GND`** — 5 ligação(ões) · **0.25 mm**
- [ ] `J14` (42.5, 29.0) → `J14` (40.0, 29.0) · 2.5 mm
- [ ] `J14` (67.9, 29.0) → `pista` (70.5, 26.5) · 3.6 mm
- [ ] `J14` (67.9, 29.0) → `J14` (55.2, 29.0) · 12.7 mm
- [ ] `J11` (134.0, 58.5) → `R9` (152.9, 60.0) · 19.0 mm
- [ ] `U3` (77.4, 112.3) → `NT1` (77.0, 81.0) · 31.4 mm

## pads que perderam o plano

*Não são pistas: são pads da rede de terra que o cobre do plano deixou de alcançar. O conserto é um **coto curto até ao cobre mais próximo da mesma rede**, ou uma via, e não uma pista até outro pad.*

- [ ] `GND` pad de `J11` em (134.0, 58.5)
- [ ] `GND` pad de `J16` em (118.0, 95.8)
- [ ] `GND` pad de `J16` em (118.0, 95.8)

