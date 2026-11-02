# Cotucatronics Carrier rev A — BOM candidato
### Verificado contra LCSC em 2026-08-20 (estoque flutua: re-conferir no dia do pedido)

## ✅ Críticos — verificados hoje, com número real

| bloco | peça | LCSC | pacote | estoque visto | nota |
|---|---|---|---|---|---|
| PD sink | **CH224K** (WCH) | **C970725** | ESSOP-10 | ~5–6k | Rset **24 kΩ** no CFG1 = 12V. VDD máx 3,6V — alimentar pelo esquema de referência WCH (resistor série), NUNCA direto do VBUS. LED no PG = "12V confirmado" |
| Amp ×2 | **TPA3116D2DADR** (TI) | **C50144** | HTSSOP-32 (PowerPAD) | em estoque | PBTL mono; 12V/4Ω ≈ 15–18W por canal. Pad térmico → vias de térmica no layout |
| Medidor | **INA219** (TI) | ⚠️ **LCSC a verificar** | SOT-23-8 ou SOIC-8 | — | I2C, bus 0–26 V, VCC 3–5 V. Kelvin no shunt. **Mesma peça da bancada** — ver GEAR-AUDIT |

## Commodity — valores definidos, número final na hora do pedido
(quase tudo é JLC *basic part* = sem taxa de setup; conferir a flag no pedido)

| bloco | peça | valor/spec | qtd | nota |
|---|---|---|---|---|
| Proteção piezo | 1N4148WS | SOD-323 | 4 | candidato clássico C2128 (basic) |
| Proteção piezo | R série | 1 kΩ 0603 | 2 | |
| Proteção piezo | pads R2 divisor | 0603, **DNP** | 2 | não popula — decisão de regime |
| Shunt | 10 mΩ | 2512, ±1%, ≥1W | 1 | 3A → 30mV, 90mW |
| Buck 12→5V | TPS563201 / MP2315 classe | SOT-583/SOIC | 1 | ≥2A; escolher basic part no esquemático |
| Filtro LC amp | indutor 10 µH | ≥5A sat, blindado | 4 | 2 por amp (PBTL) |
| Filtro LC amp | cap 680 nF | X7R 50V 0805 | 4 | |
| Bulk amp | 2200 µF/25V | radial THT low-ESR **ou** 2×1000µF polímero SMD | 2 (ou 4) | decidir por custo; THT = montagem mista |
| Desacople | 100 nF X7R 0603 | — | ~20 | um por pino de VCC de cada CI |
| TVS no VBUS | SMBJ (≈24V standoff p/ perfil 12–20V) | SMB | 1 | barato; o CH224 já morreu de spike indutivo em teste de terceiros — não pagar pra descobrir |
| PTC/fusível | ~3–4A hold | 1812/THT | 1 | depois do conector, antes de tudo |
| Pull-ups I2C | 4,7 kΩ 0603 | — | 2 | |
| LED PG + R | verde 0603 + 1 kΩ | — | 1+1 | "12V no ar" |

## Conectores

| o quê | spec | qtd | nota |
|---|---|---|---|
| Soquete Daisy | fêmea 2×20 pinos 2,54mm THT | 2 tiras | |
| Exciter out | lever/Wago-style 2 polos, SMD se houver no catálogo (DEGSON/HDGC clone), senão THT | 2 | decisão do dB: alavanca |
| OLED | receptáculo MX1.25 7p SMD | 1 | casa com o cabo da Waveshare |
| Painel (encoder, botões, DPDT sense, rotary, fork, WIDI) | JST-PH 2.0 SMD: 2p×6, 3p×2, 5p×1, 6p×1 | ~10 | contagem final no esquemático |
| Energia in | header 2p (chassi USB-C já comprado) **e** footprint USB-C 16p na borda | 1+1 | os dois na placa; popula um |
| Jacks 1/4" | painel + header 2p cada (default) — PCB-mount fica pra rev B se a caixa pedir | 4 | 2 in + 2 out |

## Regras que o BOM já embute
- Peça de memória é peça que não existe: os 3 críticos foram verificados HOJE com
  número; commodity leva valor + candidato e fecha número no pedido.
- Preferir *basic parts* JLC onde existir equivalente (setup fee zero por reel).
- DNP declarado no silk (R2 do divisor) — pad vazio COM nome não é esquecimento.
- TVS e PTC não são opcionais: powerbank + cabo + conector removível = a entrada
  mais espetada do sistema.

## Próximo passo
Esquemático KiCad por bloco, nesta ordem: energia → entrada → amps → medição →
headers. Cada bloco fecha com ERC limpo antes do seguinte. Depois: layout 4
camadas, DRC headless, render pro veto do dB, Gerber+CPL+BOM → JLCPCB, 5 placas.
