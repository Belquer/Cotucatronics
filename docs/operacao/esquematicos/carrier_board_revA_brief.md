# Cotucatronics — Carrier Board rev A · brief de projeto
### Alvo: KiCad → JLCPCB (fab + SMT assembly) · atualizado 20/08 noite

## O que esta placa é
Uma carrier board SMD onde a **Daisy Seed encaixa em soquete** e todo o resto do
instrumento — energia, proteção de entrada, amplificação, medição — vive em cobre
em vez de floresta de jumpers. O painel (encoder, botões, OLED, switches, jacks)
conecta por headers: a placa não amarra o desenho da caixa.

## Entradas e saídas (decisões do dB, 20/08)
| o quê | como |
|---|---|
| Piezos (×2) | **jack 1/4" TRS** — tip = sinal, sleeve = terra, ring aterrado |
| Exciters (×2) | **conector de alavanca (lever/Wago-style) SMD**, 2 polos cada |
| Energia | **USB-C no painel** → powerbank/PSU externo, removível (bagagem despachada) |
| Linha (×2) | jack 1/4" TS/TRS, sempre ligados (L=A, R=B em stems; mix conforme modo) |

## Blocos do circuito

### 1 · Energia
```
USB-C (painel) → [CH224K: PD sink, pede 12V] → fusível PTC → switch (+V)
   → RAIL 12V ─┬→ shunt 10 mΩ + INA226 (I2C) → bifurcação
               ├→ AMP A (bulk 2200µF/25V + 100nF colados)
               ├→ AMP B (idem)
               └→ buck 12→5V (≥2A) → RAIL 5V → Daisy VIN + WIDI VI + OLED
```
- **CH224K** substitui o módulo PD trigger: SOP, ~US$0,30, config fixa em 12V.
  Fallback declarado: fonte sem PD 12V → 5V no rail → lógica viva (buck em
  dropout ≈ 4,7V, dentro da faixa do VIN da Daisy), amps mudos. LED no PG
  do CH224K denuncia "estou em 12V" — falha em silêncio, não.
- **Buck 12→5V**: IC SMD classe 2–3A (candidatos no catálogo JLC na hora do
  esquemático: TPS54331 / MP2315 / TPS562201). Lógica consome <1A; folga 3×.
- **INA226 + shunt 10 mΩ 2512** no positivo, ANTES da bifurcação (mede tudo que
  sai do powerbank). Kelvin no shunt. I2C com pull-ups na placa.
- Bulk por amp: eletrolítico radial low-ESR THT (montagem mista JLC) **ou** 2×
  1000µF polímero SMD por amp — decidir por custo no BOM.

### 2 · Entrada (×2, um por piezo)
Jack TRS → 1kΩ série → nó → {D1 1N4148WS→3V3 · D2 1N4148WS→GND} → AUDIO IN.
- SMD: R 0603, diodos SOD-323. Sem atenuador (decisão de regime, 20/08);
  **pads previstos para R2 do divisor** (não populados) — se o `hist` na
  escultura pedir, solda sem respin.
- Ring do TRS no terra = blindagem grátis do cabo.

### 3 · Amplificação (×2)
- **TPA3116D2 em PBTL mono** integrado na placa (HTSSOP c/ pad térmico — JLC
  monta). 12V/4Ω PBTL ≈ 15–18W por canal: sobra pros exciters.
- Filtro de saída LC por amp (10µH + 680nF): os exciters saem por FIO — sem
  filtro, o fio vira antena de EMI do classe-D.
- Enable do amp acessível: o corte físico do DPDT atua AQUI (polo 1) além do
  áudio, e o polo 2 → D29/D30 (sense, pull-up interno).

### 4 · Headers de painel (JST-PH 2.0 exceto onde dito)
| header | pinos |
|---|---|
| Encoder EC11 | A, B, GND, SW, GND |
| Botões: freeze (pinball+LED), búzios, +, − | 2 fios cada; LED do freeze +2 |
| Rotary seletor de tela | divisor → 1 pino ADC (3 fios) |
| Switch fork mix/stems | 1 GPIO + GND |
| DPDT sense ×2 | → D29 / D30 |
| OLED Waveshare 2.42" | **MX1.25 7 pinos** (receptáculo direto na placa) |
| WIDI Core | 6 fios: 5V, ON, GND, TX, RX, LB |
| USB-C do painel | header 2 pinos VBUS/GND vindo do conector de chassi **ou** receptáculo USB-C 16p na borda da placa — decidir com o desenho da caixa |

### 5 · Regras de layout (o DRC não cobra, eu desenho e o dB veta)
- **4 camadas** (custa ~nada na JLC): L2 = terra sólido contínuo. Classe-D com
  retorno curto é a diferença entre silêncio e chiado.
- Um ponto de união analógico/digital, junto ao soquete da Daisy.
- Loop do bulk cap → pinos do amp: o menor cobre possível.
- Trilhas de áudio (entrada piezo) longe do chaveamento (amps, buck) — lados
  opostos da placa; entrada de alta impedância é antena.
- Shunt com sense Kelvin; I2C afastado do LC dos amps.
- THT só onde faz sentido mecânico: soquete da Daisy (2×20 fêmea), bulk caps,
  conectores de alavanca se a variante SMD não houver no catálogo.

## Fluxo
1. **BOM candidato contra o catálogo LCSC/JLC** (part numbers reais, estoque
   conferido) — aprovação do dB
2. Esquemático KiCad (.kicad_sch, texto, versionado no repo) — revisão
3. PCB + DRC headless (kicad-cli) → **render pra veto do dB**
4. Gerbers + CPL + BOM → zip JLCPCB → **5 placas protótipo** antes de qualquer
   quantidade
5. Rev B só depois de a rev A tocar

## Abertos (poucos, nenhum bloqueia o esquemático)
- Jacks 1/4": montados na placa (borda) ou no painel com header? → decide com a caixa
- USB-C: chassi+header (já comprado) vs receptáculo na borda → idem
- Bulk: THT radial vs polímero SMD → decide no BOM por custo
