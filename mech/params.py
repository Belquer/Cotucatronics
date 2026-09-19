# mech/params.py — fonte única de parâmetros mecânicos placa ↔ caixa
# VERSAO = '0.1.0'  (2026-09-19)
#
# POR QUE ESTE FICHEIRO EXISTE
#   A cicatriz-mãe do projeto: cota escrita à mão ao lado da calculada envelhece
#   em silêncio (aipcb §Filosofia dos portões, edicao-segura §Versionar TUDO).
#   A verificação 3D (/3ddesign, ppc/003) mede o STEP real, mas precisa de UM
#   sítio de onde vêm os números declarados — a caixa, os ângulos, as folgas e as
#   alturas de envelope. É AQUI. Nenhum outro ficheiro da trilha 3D reescreve
#   estes valores: importa-os, não os copia.
#
# ⚠️ PLACEHOLDERS: as alturas por zona (ALTURAS_ZONA) e algumas cotas ainda são
#   de envelope-declarado (afirmamos o número; não saiu de medição do STEP). Cada
#   um traz a sua fonte no comentário. Quando o STEP real medir a zona, o número
#   passa a "STEP-real" e a divergência bbox-declarado vs STEP-real vira vermelho
#   no MANIFEST (ver /3ddesign §MANIFEST).

VERSAO = '0.1.0'

# --- Caixa (cotas externas do vão interior, mm) ------------------------------
# Fonte: caixa/cotuca_caixa_corte.py (gerador) + caixa rev parafusada.
# Estes valores são STUB até o gerador da caixa ser importado em Fase A.
CAIXA = {
    'vao_interior_x_mm': 120.0,   # STUB — vem do gerador da caixa (parametrizado)
    'vao_interior_y_mm': 80.0,    # STUB — idem
    'espessura_chapa_mm': 3.0,    # acrílico fundido 3,0 mm (caixa rev acrílico 3p0)
    'angulo_tampa_graus': 11.8,   # cunha da junta/tampa (ifa10000-m M5; caixa)
    'folga_borda_mm': 0.5,        # folga mínima placa↔parede declarada (M4)
    'kerf_laser_mm': 0.1,         # kerf do corte (K no gerador; M8)
}

# --- Alturas máximas por zona (mm) -------------------------------------------
# ENVELOPE-DECLARADO (placeholder): o componente mais alto de cada zona, medido
# depois contra o interior da tampa na cunha de 11,8° (assertiva 3, wedge POR
# ZONA). Trocar por STEP-real quando verifica_3d.py medir a zona.
ALTURAS_ZONA = {
    'daisy_seed': 11.0,   # STUB — módulo Daisy Seed empilhado (ficha ao lado)
    'idc_j14':     8.85,  # caixa de pinos IDC (ficha Würth) — acoplar ficha do cabo
    'eletroliticos': 11.6,  # STUB — corpo dos eletrolíticos altos (M6 âncora J14 corpo=11,6)
}

# --- Folga de tampa por zona (mm), na cunha 11,8° ----------------------------
# ENVELOPE-DECLARADO: altura interior da tampa acima de cada zona (varia com x
# por causa da cunha). STUB até o STEP da caixa entrar em Fase A.
TAMPA_INTERIOR_ZONA = {
    'daisy_seed': 14.0,   # STUB
    'idc_j14':    12.0,   # STUB
    'eletroliticos': 13.0,  # STUB
}

# --- Furos de fixação (assertiva 2: furos alinham placa↔postes/brackets) ------
# ENVELOPE-DECLARADO: pares (placa, poste) que têm de coincidir. STUB.
FUROS_FIXACAO = {
    'raio_anilha_m3_mm': 3.5,   # raio de exclusão da anilha M3 (ifa10000-m M2)
    'tolerancia_alinhamento_mm': 0.4,  # desvio máx placa↔poste (M8)
    'min_pontos': 3,            # ≥3 pontos não colineares (M2)
}

# --- Assertivas 4-6: declared-envelope (afirmamos o número, não é física) -----
# NÃO são medidas do STEP. São afirmações que o humano assina (M9, ⟨dB⟩).
ENVELOPE_DECLARADO = {
    'recortes_tampa': {          # assertiva 4
        'janela_oled': True,
        'furos_pots_mm': 16.0,   # Caso B — pots de 16 mm na tampa
        'encoder': True,
    },
    'cabos_servico': {           # assertiva 5
        'j1_j14_mm': 265.0,      # fita IDC pelo percurso real (ifa10000-m: 150→265)
        'pots_para_tampa_casoB': True,
    },
    'montabilidade': {           # assertiva 6
        'parafusos_acessiveis': True,
        'placa_removivel_sem_dessoldar': True,
    },
}

# --- Alvo de STEP / placa de referência --------------------------------------
CEREBRO_PCB = 'kicad/cotuca_cerebro/cotuca_cerebro.kicad_pcb'
STEP_REF_MB = 3.0   # tamanho de referência do STEP do cérebro (cérebro ~3,0 MB)
