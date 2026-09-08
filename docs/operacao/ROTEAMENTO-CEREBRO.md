# Lista de roteamento — cerebro

Gerada por `kicad/lista_roteamento.py` v0.3.1 em **2026-09-07**, commit `cc359f6`.  
⚠️ **Não comitado**: `kicad/cotuca_cerebro/cotuca_cerebro.kicad_pcb`, `kicad/faz_placa.py`, `kicad/roteia.py` — a placa pode não corresponder a este commit.

**11 ligações**, mais **1 pad(s) que perderam o plano** — 12 no total, que é o que o DRC diz. Tudo o resto já está feito e o DRC dá geometria zero com os planos inteiros, o que quer dizer que **o que desenhares tem de continuar a dar isso**.

Depois de as fazeres, o portão diz se ficou bom:

```bash
python3 kicad/para_fabrica.py cerebro
```

## alta impedancia

*curtas, sobre a ilha de AGND, sem digital em paralelo (P5)*

**`Net-(C21-Pad2)`** — 1 ligação(ões) · **0.25 mm**
- [ ] `pista` (42.9, 73.6) → `U1` (28.5, 91.4) · 22.9 mm

**`Net-(U1A-+)`** — 1 ligação(ões) · **0.25 mm**
- [ ] `pista` (38.6, 47.1) → `U1` (23.5, 92.6) · 48.0 mm

**`Net-(U1A--)`** — 1 ligação(ões) · **0.25 mm**
- [ ] `U1` (23.5, 91.4) → `pista` (39.1, 58.1) · 36.7 mm

**`Net-(U1B--)`** — 1 ligação(ões) · **0.25 mm**
- [ ] `pista` (24.6, 81.8) → `U1` (28.5, 92.6) · 11.5 mm

## trilhos de alimentacao

*largas, e primeiro que as de sinal: ocupam mais espaco*

**`V+PRE`** — 2 ligação(ões) · **0.50 mm**
- [ ] `U1` (28.5, 90.1) → `R5` (25.1, 103.0) · 13.3 mm
- [ ] `pista` (38.9, 91.3) → `U1` (28.5, 90.1) · 10.5 mm

## digital

*o que sobra: OLED, encoder, botoes, MIDI*

**`BOT_BUZIOS`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J14` (62.9, 29.0) → `U2` (98.0, 91.3) · 71.5 mm

**`BOT_FRZ_B`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J14` (60.3, 26.5) → `U2` (98.0, 81.1) · 66.4 mm

**`ENC_A_A`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J14` (62.9, 26.5) → `U2` (113.2, 109.1) · 96.8 mm

**`ENC_A_B`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J14` (65.4, 29.0) → `U2` (113.2, 106.5) · 91.1 mm

**`ENC_A_PUSH`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J14` (65.4, 26.5) → `U2` (113.2, 104.0) · 91.1 mm

## pads que perderam o plano

*Não são pistas: são pads da rede de terra que o cobre do plano deixou de alcançar. O conserto é um **coto curto até ao cobre mais próximo da mesma rede**, ou uma via, e não uma pista até outro pad.*

- [ ] `GND` pad de `J14` em (55.2, 29.0)

