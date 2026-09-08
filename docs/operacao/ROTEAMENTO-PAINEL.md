# Lista de roteamento — painel

Gerada por `kicad/lista_roteamento.py` v0.3.1 em **2026-09-07**, commit `36a500d`.

**9 ligações**, mais **1 pad(s) que perderam o plano** — 10 no total, que é o que o DRC diz. Tudo o resto já está feito e o DRC dá geometria zero com os planos inteiros, o que quer dizer que **o que desenhares tem de continuar a dar isso**.

Depois de as fazeres, o portão diz se ficou bom:

```bash
python3 kicad/para_fabrica.py painel
```

## trilhos de alimentacao

*largas, e primeiro que as de sinal: ocupam mais espaco*

**`+3V3A`** — 2 ligação(ões) · **0.50 mm**
- [ ] `J1` (36.2, 24.0) → `C5` (25.1, 30.0) · 12.6 mm
- [ ] `J1` (36.2, 24.0) → `RV1` (139.0, 35.5) · 103.5 mm

**`+5V`** — 1 ligação(ões) · **0.50 mm**
- [ ] `J2` (69.2, 24.0) → `C4` (63.0, 52.0) · 28.7 mm

## digital

*o que sobra: OLED, encoder, botoes, MIDI*

**`BOT_FRZ_B`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J4` (38.0, 34.0) → `J1` (46.3, 21.5) · 15.0 mm

**`LED_FRZ_B`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J1` (43.8, 21.5) → `R2` (63.1, 40.0) · 26.8 mm

**`OLED_CLK`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J1` (26.0, 21.5) → `J2` (70.5, 24.0) · 44.6 mm

**`OLED_DC`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J1` (31.1, 21.5) → `J2` (74.2, 24.0) · 43.2 mm

**`OLED_DIN`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J1` (28.5, 21.5) → `J2` (71.8, 24.0) · 43.3 mm

**`POT_L`** — 1 ligação(ões) · **0.25 mm**
- [ ] `J1` (53.9, 21.5) → `C1` (132.1, 38.0) · 79.8 mm

## pads que perderam o plano

*Não são pistas: são pads da rede de terra que o cobre do plano deixou de alcançar. O conserto é um **coto curto até ao cobre mais próximo da mesma rede**, ou uma via, e não uma pista até outro pad.*

- [ ] `GND` pad de `J1` em (41.2, 21.5)

