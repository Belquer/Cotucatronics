# MAPA DE COLOCACAO — cotuca painel

> Gerado por `kit_montagem.py` v1.0.0 a partir de `cpl_rico_painel.csv` + `bom_painel.csv`.
>
> **Como montar (ordem que este mapa assume):**
> 1. Refluxo do **SMD do TOPO** na chapa quente PRIMEIRO — coloca-se por lote, todos os de um valor antes do valor seguinte.
> 2. **Pecas do lado BOTTOM soldam-se A MAO** (nao vao na chapa): nenhuma.
> 3. **THT por ultimo**, todos a mao: C3, J1, J2, J3, J4, J5, J6, J7, RV1, RV2, SW1.
> 4. **Orientacao/polaridade:** a referencia e o RENDER 3D da placa e a `VISTORIA.md` do pacote, peca a peca — este mapa da posicao e rotacao, nao decide o sentido do pino 1 / catodo / faixa.

## TOP — refluxo na chapa quente

_Coloca-se por lote (todos os de um valor de uma vez), depois refluxo._

### 100n · SMD
- **Footprint:** Capacitor_SMD:C_0805_2012Metric
- **LCSC:** C49678
- **Qty:** 4
- **Colocar:**
    - `C1` @ (133, -38) Top  rot 0°
    - `C2` @ (168, -38) Top  rot 0°
    - `C4` @ (64, -52) Top  rot 0°
    - `C5` @ (34.5, -34.5) Top  rot 0°

### 100R gate · SMD
- **Footprint:** Resistor_SMD:R_0805_2012Metric
- **LCSC:** C17408
- **Qty:** 2
- **Colocar:**
    - `R1` @ (52, -42.3) Top  rot 0°
    - `R2` @ (64, -42.3) Top  rot 0°

### 100R serie (~30 mA) · SMD
- **Footprint:** Resistor_SMD:R_1210_3225Metric
- **LCSC:** C2933799
- **Qty:** 2
- **Colocar:**
    - `R5` @ (76, -46) Top  rot 0°
    - `R6` @ (88, -46) Top  rot 0°

### 100u 16V · THT — soldar a mao, por ultimo
- **Footprint:** Capacitor_THT:CP_Radial_D6.3mm_P2.50mm
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `C3` @ (52, -52) Top  rot 0°

### 10k lin · THT — soldar a mao, por ultimo
- **Footprint:** Potentiometer_THT:Potentiometer_Bourns_PTV09A-1_Single_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 2
- **Colocar:**
    - `RV1` @ (139, -40.5) Top  rot 0°
    - `RV2` @ (174, -40.5) Top  rot 0°

### 10k pull-down · SMD
- **Footprint:** Resistor_SMD:R_0805_2012Metric
- **LCSC:** C17414
- **Qty:** 2
- **Colocar:**
    - `R3` @ (76, -40) Top  rot 0°
    - `R4` @ (88, -40) Top  rot 0°

### AO3400A · SMD
- **Footprint:** Package_TO_SOT_SMD:SOT-23
- **LCSC:** C20917
- **Qty:** 2
- **Colocar:**
    - `Q1` @ (52, -46) Top  rot 0°
    - `Q2` @ (64, -46) Top  rot 0°

### EC11 — o unico encoder · THT — soldar a mao, por ultimo
- **Footprint:** Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `SW1` @ (103.5, -35.5) Top  rot 0°

### fio -> arcade AZUL freeze A · THT — soldar a mao, por ultimo
- **Footprint:** Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J3` @ (26, -36.5) Top  rot 0°

### fio -> arcade AZUL freeze B · THT — soldar a mao, por ultimo
- **Footprint:** Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J4` @ (40, -36.5) Top  rot 0°

### fio -> arcade BRANCO buzios · THT — soldar a mao, por ultimo
- **Footprint:** Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J5` @ (53, -36.5) Top  rot 0°

### fio -> LED do arcade A (1=A, 2=K) · THT — soldar a mao, por ultimo
- **Footprint:** Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J6` @ (26, -46) Top  rot 0°

### fio -> LED do arcade B (1=A, 2=K) · THT — soldar a mao, por ultimo
- **Footprint:** Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J7` @ (38, -46) Top  rot 0°

### IDC 2x13 femea <- cerebro · THT — soldar a mao, por ultimo
- **Footprint:** Connector_IDC:IDC-Header_2x13_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J1` @ (26, -29.5) Top  rot 90°

### OLED SSD1309 2,54mm 7p · THT — soldar a mao, por ultimo
- **Footprint:** Connector_PinHeader_2.54mm:PinHeader_1x07_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J2` @ (68, -24) Top  rot 90°

## BOTTOM — soldar a mao (NAO vai na chapa)

_Depois do topo estar feito. Cada peca aqui e soldada a mao._

_(nenhuma peca deste lado)_
