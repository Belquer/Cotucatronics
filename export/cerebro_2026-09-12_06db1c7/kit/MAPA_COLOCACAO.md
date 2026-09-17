# MAPA DE COLOCACAO — cotuca cerebro

> Gerado por `kit_montagem.py` v1.0.0 a partir de `cpl_rico_cerebro.csv` + `bom_cerebro.csv`.
>
> **Como montar (ordem que este mapa assume):**
> 1. Refluxo do **SMD do TOPO** na chapa quente PRIMEIRO — coloca-se por lote, todos os de um valor antes do valor seguinte.
> 2. **Pecas do lado BOTTOM soldam-se A MAO** (nao vao na chapa): C16.
> 3. **THT por ultimo**, todos a mao: C1, C3, C5, C6, C14, C24, J1, J2, J3, J4, J5, J6, J11, J12, J13, J14, J15, J16, U2, U3.
> 4. **Orientacao/polaridade:** a referencia e o RENDER 3D da placa e a `VISTORIA.md` do pacote, peca a peca — este mapa da posicao e rotacao, nao decide o sentido do pino 1 / catodo / faixa.

## TOP — refluxo na chapa quente

_Coloca-se por lote (todos os de um valor de uma vez), depois refluxo._

### 0R · SMD
- **Footprint:** Resistor_SMD:R_0805_2012Metric
- **LCSC:** C17477
- **Qty:** 2
- **Colocar:**
    - `R10` @ (55, -47) Top  rot 0°
    - `R20` @ (55, -75) Top  rot 0°

### 100k · SMD
- **Footprint:** Resistor_SMD:R_0805_2012Metric
- **LCSC:** C149504
- **Qty:** 3
- **Colocar:**
    - `R5` @ (26, -103) Top  rot 0°
    - `R6` @ (40.5, -103) Top  rot 0°
    - `R8` @ (180, -47) Top  rot 0°

### 100n · SMD
- **Footprint:** Capacitor_SMD:C_0805_2012Metric
- **LCSC:** C49678
- **Qty:** 5
- **Colocar:**
    - `C2` @ (152, -47) Top  rot 0°
    - `C4` @ (166, -47) Top  rot 0°
    - `C7` @ (40.5, -92) Top  rot 0°
    - `C9` @ (69.5, -92) Top  rot 0°
    - `C15` @ (105, -115) Top  rot 0°

### 100R · SMD
- **Footprint:** Resistor_SMD:R_0805_2012Metric
- **LCSC:** C17408
- **Qty:** 2
- **Colocar:**
    - `R14` @ (69.5, -47) Top  rot 0°
    - `R24` @ (69.5, -75) Top  rot 0°

### 100R 1/4W · SMD
- **Footprint:** Resistor_SMD:R_1206_3216Metric
- **LCSC:** C17901
- **Qty:** 1
- **Colocar:**
    - `R7` @ (55, -103) Top  rot 0°

### 100u · THT — soldar a mao, por ultimo
- **Footprint:** Capacitor_THT:CP_Radial_D6.3mm_P2.50mm
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `C3` @ (166, -73) Top  rot 0°

### 10k — L · SMD
- **Footprint:** Resistor_SMD:R_0805_2012Metric
- **LCSC:** C17414
- **Qty:** 2
- **Colocar:**
    - `R50` @ (118, -34) Top  rot 0°
    - `R51` @ (118, -56) Top  rot 0°

### 10u · SMD
- **Footprint:** Capacitor_SMD:C_1206_3216Metric
- **LCSC:** C13585
- **Qty:** 1
- **Colocar:**
    - `C8` @ (55, -92) Top  rot 0°

### 1k · SMD
- **Footprint:** Resistor_SMD:R_0805_2012Metric
- **LCSC:** C17513
- **Qty:** 2
- **Colocar:**
    - `R12` @ (26, -36) Top  rot 0°
    - `R22` @ (69.5, -64) Top  rot 0°

### 2200u/35V · THT — soldar a mao, por ultimo
- **Footprint:** Capacitor_THT:CP_Radial_D16.0mm_P7.50mm
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `C1` @ (178, -108) Top  rot 0°

### 22k · SMD
- **Footprint:** Resistor_SMD:R_0805_2012Metric
- **LCSC:** C17560
- **Qty:** 1
- **Colocar:**
    - `R9` @ (152, -60) Top  rot 0°

### 22n C0G · SMD
- **Footprint:** Capacitor_SMD:C_1206_3216Metric
- **LCSC:** C3862155
- **Qty:** 2
- **Colocar:**
    - `C12` @ (40.5, -58) Top  rot 0°
    - `C22` @ (26, -86) Top  rot 0°

### 2u2 filme · THT — soldar a mao, por ultimo
- **Footprint:** Capacitor_THT:C_Rect_L7.2mm_W11.0mm_P5.00mm_FKS2_FKP2_MKS2_MKP2
- **LCSC:** — (sem LCSC)
- **Qty:** 2
- **Colocar:**
    - `C14` @ (86, -38) Top  rot 0°
    - `C24` @ (86, -66) Top  rot 0°

### 470u 25V · THT — soldar a mao, por ultimo
- **Footprint:** Capacitor_THT:CP_Radial_D10.0mm_P5.00mm
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `C6` @ (55, -114) Top  rot 0°

### 47u · THT — soldar a mao, por ultimo
- **Footprint:** Capacitor_THT:CP_Radial_D6.3mm_P2.50mm
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `C5` @ (69.5, -103) Top  rot 0°

### 4M7 · SMD
- **Footprint:** Resistor_SMD:R_0805_2012Metric
- **LCSC:** C17674
- **Qty:** 2
- **Colocar:**
    - `R11` @ (55, -36) Top  rot 0°
    - `R21` @ (55, -64) Top  rot 0°

### a ponte ESP32 · THT — soldar a mao, por ultimo
- **Footprint:** Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J4` @ (180, -73) Top  rot 0°

### ao amp TPA3116 · THT — soldar a mao, por ultimo
- **Footprint:** TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J2` @ (152, -73) Top  rot 0°

### BAV99 · SMD
- **Footprint:** Package_TO_SOT_SMD:SOT-23
- **LCSC:** C2500
- **Qty:** 2
- **Colocar:**
    - `D11` @ (40.5, -36) Top  rot 0°
    - `D21` @ (40.5, -64) Top  rot 0°

### Daisy Seed3 · THT — soldar a mao, por ultimo
- **Footprint:** Module:Electrosmith_Daisy_Seed
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `U2` @ (113.24, -109.06) Top  rot 180°

### entrada decoy PD · THT — soldar a mao, por ultimo
- **Footprint:** TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J1` @ (156, -108) Top  rot 0°

### ESP32-C3 SuperMini · THT — soldar a mao, por ultimo
- **Footprint:** cotuca:ESP32C3_SUPERMINI_SOQUETE
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `U3` @ (85, -106) Top  rot 180°

### fio -> DPDT tipo 2, canal L · THT — soldar a mao, por ultimo
- **Footprint:** Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J15` @ (134, -34) Top  rot 0°

### fio -> DPDT tipo 2, canal R · THT — soldar a mao, por ultimo
- **Footprint:** Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J16` @ (118, -78) Top  rot 0°

### fio -> jack piezo J5 · THT — soldar a mao, por ultimo
- **Footprint:** Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J5` @ (69.5, -36) Top  rot 0°

### fio -> jack piezo J6 · THT — soldar a mao, por ultimo
- **Footprint:** Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J6` @ (26, -64) Top  rot 0°

### fio -> TRS linha L · THT — soldar a mao, por ultimo
- **Footprint:** Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J12` @ (134, -78) Top  rot 0°

### fio -> TRS linha R · THT — soldar a mao, por ultimo
- **Footprint:** Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J13` @ (118, -100) Top  rot 0°

### IDC 2x13 -> painel · THT — soldar a mao, por ultimo
- **Footprint:** Connector_IDC:IDC-Header_2x13_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J14` @ (45, -29) Top  rot 90°

### modulo TPA3116 · THT — soldar a mao, por ultimo
- **Footprint:** Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J11` @ (134, -56) Top  rot 0°

### OPA1642 · SMD
- **Footprint:** Package_SO:SOIC-8_3.9x4.9mm_P1.27mm
- **LCSC:** C67640
- **Qty:** 1
- **Colocar:**
    - `U1` @ (26, -92) Top  rot 0°

### PTC 3,3A · SMD
- **Footprint:** Fuse:Fuse_2920_7451Metric
- **LCSC:** C720183
- **Qty:** 1
- **Colocar:**
    - `F1` @ (152, -34) Top  rot 0°

### SMBJ24A · SMD
- **Footprint:** Diode_SMD:D_SMB
- **LCSC:** C19077578
- **Qty:** 1
- **Colocar:**
    - `D1` @ (166, -34) Top  rot 0°

### SS14 · SMD
- **Footprint:** Diode_SMD:D_SMA
- **LCSC:** C2480
- **Qty:** 1
- **Colocar:**
    - `D2` @ (180, -34) Top  rot 0°

### step-down 5V fixo · THT — soldar a mao, por ultimo
- **Footprint:** Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical
- **LCSC:** — (sem LCSC)
- **Qty:** 1
- **Colocar:**
    - `J3` @ (166, -60) Top  rot 0°

## BOTTOM — soldar a mao (NAO vai na chapa)

_Depois do topo estar feito. Cada peca aqui e soldada a mao._

### 100n · SMD
- **Footprint:** Capacitor_SMD:C_0805_2012Metric
- **LCSC:** C49678
- **Qty:** 1
- **Colocar:**
    - `C16` @ (113, -47) Bottom  rot 0°  ⚠️ BOTTOM — soldar a mao
