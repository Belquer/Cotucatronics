# Lista de roteamento — painel

Gerada por `kicad/lista_roteamento.py` v0.3.1 em **2026-09-08**, commit `4a17a0e`.  
⚠️ **Não comitado**: `kicad/cotuca_painel/cotuca_painel.kicad_pcb`, `kicad/pcb_contorno.py`, `kicad/roteia_fr.py`, `kicad/sincroniza_pcb.py`, `kicad/compara_netlist.py` — a placa pode não corresponder a este commit.

**8 ligações**, mais **2 pad(s) que perderam o plano** — 10 no total, que é o que o DRC diz. Tudo o resto já está feito e o DRC dá geometria zero com os planos inteiros, o que quer dizer que **o que desenhares tem de continuar a dar isso**.

Depois de as fazeres, o portão diz se ficou bom:

```bash
python3 kicad/para_fabrica.py painel
```

## trilhos de alimentacao

*largas, e primeiro que as de sinal: ocupam mais espaco*

**`+3V3A`** — 2 ligação(ões) · **0.50 mm**
- [ ] `pista` (31.9, 28.7) → `RV1` (139.0, 35.5) · 107.3 mm
- [ ] `RV1` (139.0, 35.5) → `RV2` (174.0, 35.5) · 35.0 mm

## terra

*o plano fez o resto; estas ficaram por fora dele*

**`GND`** — 6 ligação(ões) · **0.25 mm**
- [ ] `J1` (36.2, 21.5) → `J1` (33.6, 21.5) · 2.5 mm
- [ ] `J1` (41.2, 21.5) → `J1` (41.2, 24.0) · 2.5 mm
- [ ] `pista` (56.5, 21.5) → `J1` (53.9, 24.0) · 3.6 mm
- [ ] `pista` (28.5, 24.0) → `J1` (26.0, 24.0) · 2.5 mm
- [ ] `J4` (40.5, 34.0) → `J1` (41.2, 24.0) · 10.0 mm
- [ ] `pista` (65.0, 51.9) → `pista` (57.2, 46.0) · 9.8 mm

## pads que perderam o plano

*Não são pistas: são pads da rede de terra que o cobre do plano deixou de alcançar. O conserto é um **coto curto até ao cobre mais próximo da mesma rede**, ou uma via, e não uma pista até outro pad.*

- [ ] `GND` pad de `J1` em (33.6, 21.5)
- [ ] `GND` pad de `J1` em (41.2, 21.5)

