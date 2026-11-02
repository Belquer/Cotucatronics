#!/usr/bin/env python3
"""cotuca_caixa_corte.py — ficheiros de corte da caixa em acrílico.

Estrutura (decidida 2026-09-05):
  · CASCA soldada: TAMPA inclinada + 4 paredes, juntas a dedo nos 4 cantos
    verticais e nas 4 bordas superiores.
  · FUNDO removível: chapa que entra por baixo, apoia em 6 calços colados e
    leva 6 parafusos M3 por baixo — como um pedal tradicional.
  · Carrier e amp montam no FUNDO; sai tudo com ele para manutenção.
Tudo em acrílico CAST 6 mm. Kerf 0,2 compensado nos dedos.

Saída: SVG 1:1 (mm) com cores Ponoko (azul = corte, vermelho = gravação) e DXF.
O nome do ficheiro leva a ESPESSURA — `..._acrilico_3p0.svg`. Não é arrumação:
esta caixa já teve quatro espessuras plausíveis (6 nos desenhos, 6,4 na
nogueira cortada, 6,0 aqui, 3,0 no material encomendado) e um ficheiro de corte
sem espessura no nome mede-se tão bem como o certo. Ver cicatriz 15cf.

USO:  cotuca_caixa_corte.py [--t 3.0] [--kerf 0.2]
"""
import math, os, sys, subprocess

VERSAO = "0.7.0"

# --- identidade do ARTEFACTO, que nao e a versao do gerador ------------------
# ⚠️ DUAS COISAS DIFERENTES PRECISAM DE DOIS NUMEROS. `VERSAO` diz o que mudou
# no CODIGO; `REV` diz o que mudou na CAIXA. Um gerador novo pode emitir a mesma
# chapa (refiz o encaixe tres vezes sem mexer numa cota), e o mesmo gerador emite
# chapas diferentes conforme os parametros.
#
# E o nome leva os dois, a DATA da revisao, e o DIGEST do proprio desenho:
#     cotuca_caixa_20260906_r3_acrilico_3p0_a1b2c3d4.svg
#
# ⚠️ A DATA NAO E ORNAMENTO: e a unica parte do nome que diz O QUE VEM DEPOIS.
# Um digest identifica e NAO ORDENA — `55cf3955` e `037a9c30` nao dizem qual e
# mais recente, e eu tinha-o posto a fazer um trabalho que ele nao sabe fazer.
# O `REV` ordena mas bumpa-se a mao, que e precisamente o que o digest existe
# para nao depender. A data vem primeiro no nome para o `ls` sair cronologico.
#
# ⚠️ E a data e da REVISAO, nao da corrida: gerar duas vezes tem de dar o mesmo
# ficheiro. Um nome que muda a cada corrida diz "mudou" quando nada mudou, e ai
# ninguem confia nele.
#
# ⚠️ O DIGEST E O QUE NAO SE ESQUECE. O `REV` bumpa-se a mao e a mao esquece —
# foi assim que a caixa com reforco e a caixa sem reforco ficaram com o MESMO
# nome, uma ja encomendada e a outra nao, indistinguiveis na pasta e no upload.
# Com o digest no nome, geometria diferente E ficheiro diferente, por construcao:
# nao ha como sobrepor sem dar por isso.
REV, REV_DATA = 5, "2026-09-06"
# r1 (2026-09-06) 19 pecas · arcade a 40/100/160 · sem reforco. ENCOMENDA #1263497.
# r2 (2026-09-06) 20 pecas · arcade a 30/100/170 (70 entre eixos) · REFORCO da tampa
#            193x50 com furos o30, confirmado pelo prototipo de madeira de 6 mm.
# r3 (2026-09-06) + ABERTURAS na parede de tras, pedidas pela placa em
#            caixa/aberturas.json, para reprogramar os modulos montados. A r2 nao
#            tinha nenhuma livre: o unico recorte de USB e o do decoy PD.
#            NAO ENCOMENDADA — so tinha a abertura da Daisy.
# r4 (2026-09-06) DUAS aberturas: Daisy e ESP32-C3, que passou a soquete. A do ESP32
#            mudou de x=164 para x=75 quando o modulo achou lugar livre a
#            esquerda em vez da direita. NAO ENCOMENDADA.
# r5 (2026-09-06) as duas aberturas JUNTAM-SE numa: ficavam a 2,6 mm uma da outra, e
#            uma tira de 2,6 em acrilico de 3 parte ao montar.

# --------------- parâmetros ---------------
def _arg(nome, omissao):
    """Lê `--nome V` ou `--nome=V` do argv. Ignora tudo o resto — este módulo é
    IMPORTADO pelo `kicad/pcb_contorno.py`, que tem argv seu."""
    for i, a in enumerate(sys.argv[1:]):
        if a == "--" + nome and i + 2 < len(sys.argv) + 1:
            try:
                return float(sys.argv[i + 2])
            except (IndexError, ValueError):
                raise SystemExit("*** --%s precisa de um número" % nome)
        if a.startswith("--" + nome + "="):
            return float(a.split("=", 1)[1])
    return omissao


# ⚠️ A OMISSAO E A CAIXA QUE ESTA A SER FEITA, e nao a ultima que se desenhou.
# Este modulo e IMPORTADO pelo kicad/pcb_contorno.py, que nao passa argumentos —
# com a omissao a 6.0 a placa foi conferida contra uma caixa de 6 mm enquanto a
# encomenda era de 3. O portao dizia "espacador minimo 15" quando o certo era 9.
# Omissao errada nao rebenta: responde com confianca sobre outro objecto.
T = _arg("t", 3.0)          # espessura — acrilico transparente, encomendado
K = _arg("kerf", 0.2)       # kerf

# ⚠️ REFORCO DA TAMPA. Um pe sao ~300 N a meio do vao entre a parede da frente e
# a nervura. Em 3 mm de acrilico isso flete a olho e, com o tempo, raja A PARTIR
# DOS FUROS DE o30, que sao o ponto de concentracao de tensao. Cortar uma e
# pisar diz se parte no primeiro dia; nao diz se parte a centesima.
#
# A resposta e uma segunda chapa colada por BAIXO da tampa, so na faixa dos
# arcade: 6 mm efectivos onde o pe pisa, 3 mm onde os pots e o encoder
# atravessam — que e a razao pela qual a tampa e de 3 (o PTV09 tem ~7 mm de
# rosca e nao ha porca com 6 de tampa mais a placa por baixo).
#
# ⚠️ E O FURO DO REFORCO NAO E o30 POR OMISSAO. Se ele repetisse o o30, o botao
# passaria a montar num painel de 6 mm — e os arcade JA ESTAO COMPRADOS, com um
# comprimento de rosca que ninguem mediu. Repetir o o30 e a armadilha do bucim
# dos pots ao contrario. Por omissao o furo e de passagem: o botao monta nos
# 3 mm da tampa e a porca gira dentro do rasgo do reforco.
#   --reforco-furo 30  -> painel de 6 mm no botao. SO depois de medir a rosca.
#   --reforco-furo 42  -> passagem (omissao). Precisa de caber a PORCA.
#   --reforco 0        -> sem reforco.
REFORCO = _arg("reforco", 1.0) != 0.0
# ⚠️ SEM OMISSAO, DE PROPOSITO. A cicatriz 15cg acabou de custar horas por um
# valor por omissao que respondia com confianca sobre outro objecto. Aqui as
# duas hipoteses dao caixas diferentes e uma delas nao monta:
#   o30 -> o botao passa a montar num painel de 6 mm. So se a ROSCA do arcade
#          o permitir — e os arcade ja estao comprados, com uma rosca que
#          ninguem mediu (a mesma familia do bucim dos pots, ao contrario).
#   o42 -> passagem: o botao monta nos 3 mm da tampa e a PORCA gira dentro do
#          rasgo. So se a porca couber em 42.
# Duas medicoes no botao que esta em cima da mesa decidem. Ate la, recusa.
REFORCO_FURO = _arg("reforco-furo", 0.0)
# ⚠️ A recusa vive no __main__, NAO aqui. Este modulo e importado pelo
# `kicad/pcb_contorno.py`, que so quer as cotas da caixa e nao tem opiniao sobre
# o furo do reforco. Recusar no topo fazia a importacao rebentar — o portao
# certo no sitio errado para de ser portao e passa a ser avaria.
FALTA_REFORCO_FURO = (
        "*** --reforco-furo e obrigatorio. Mede no arcade que ja compraste:\n"
        "      · comprimento da ROSCA (o painel que ela aperta)\n"
        "      · diametro exterior da PORCA\n"
        "    rosca >= 6 mm            -> --reforco-furo 30   (painel de 6, o forte)\n"
        "    rosca < 6, porca <= 42   -> --reforco-furo 42   (passagem)\n"
        "    porca > 42               -> --reforco-furo <o da porca + 2>\n"
        "    sem reforco              -> --reforco 0\n"
        "  Adivinhar aqui e uma chapa cortada que nao monta.")
REFORCO_LIGA = 8.0          # material minimo entre o furo e a borda da faixa
_r_y = 35.0                 # os arcade estao todos a y=35 da face
# ⚠️ A FAIXA SEGUE O FURO, e nao um numero escrito. Com o o42 de passagem numa
# faixa de 50 (10..60, que foi a proposta) sobram 4 mm entre o furo e a borda —
# e o ligamento fino e exactamente onde a fissura comeca, que e o defeito que o
# reforco existe para evitar. A faixa cresce ate ter `REFORCO_LIGA` de cada lado.
_meia = max(25.0, REFORCO_FURO / 2 + REFORCO_LIGA)
REFORCO_Y = (_r_y - _meia, _r_y + _meia)
W, D = 200.0, 120.0            # exterior: largura, profundidade
HF, HB = 55.0, 80.0            # altura exterior frente / trás
RUN = D                        # projeção da tampa
SLOPE = math.hypot(RUN, HB - HF)   # comprimento da tampa no plano = 122.6
ANG = math.degrees(math.atan2(HB - HF, RUN))
BODY_W = W - 2*T               # 188
BODY_D = D - 2*T               # 108
FRONT_H = HF - T               # parede frente, sem dedos de cima = 49
BACK_H = HB - T                # 74
SLOPE_BODY = SLOPE * BODY_D / RUN   # tampa entre paredes = 110.3

# --------------- geometria ---------------
def cells(L, n_target, first_owner, owners):
    """divide L em n células (ímpar) e devolve [(s0,s1,owner)], alternando owners a partir de first_owner"""
    n = max(3, round(L / n_target)); n += (n % 2 == 0)
    w = L / n
    seq = []
    o = owners.index(first_owner)
    for i in range(n):
        seq.append((i*w, (i+1)*w, owners[o])); o = 1 - o
    return seq

def edge(A, B, fingers, t=T, k=K, ext_start=False, ext_end=False):
    """pontos de A a B com dedos para FORA (direita do sentido A→B, para polígono CCW).
    fingers: intervalos (s0,s1) medidos ao longo de A→B a partir de A.
    ext_start/ext_end: o primeiro/último dedo estende-se t para fora do canto (preenche o cubo do canto)."""
    ax, ay = A; bx, by = B
    L = math.hypot(bx-ax, by-ay); ux, uy = (bx-ax)/L, (by-ay)/L
    nx, ny = uy, -ux           # normal para a direita = fora
    pts = [A]
    for i, (s0, s1) in enumerate(fingers):
        s0k, s1k = s0 - k/2, s1 + k/2          # dedo mais largo: compensação de kerf
        if ext_start and i == 0: s0k = -t
        if ext_end and i == len(fingers)-1: s1k = L + t
        p0 = (ax + ux*s0k, ay + uy*s0k); p1 = (ax + ux*s1k, ay + uy*s1k)
        pts += [p0, (p0[0]+nx*t, p0[1]+ny*t), (p1[0]+nx*t, p1[1]+ny*t), p1]
    pts.append(B)
    return pts

def fingers_of(seq, owner):
    return [(s0, s1) for s0, s1, o in seq if o == owner]

# padrões de junta (partilhados pelas duas peças de cada junta)
J_FL = cells(FRONT_H, 12, "F", ["F", "L"])          # vertical frente-esq, de baixo para cima
J_FR = cells(FRONT_H, 12, "F", ["F", "R"])
J_BL = cells(BACK_H,  12, "B", ["B", "L"])
J_BR = cells(BACK_H,  12, "B", ["B", "R"])
J_TF = cells(W, 10.5, "F", ["F", "T"])              # borda de cima da FRENTE, sobre a largura EXTERIOR (0..200)
J_TB = cells(W, 10.5, "B", ["B", "T"])
J_TL = cells(SLOPE, 11, "X", ["X", "L"])            # lateral esq ↔ tampa, sobre o comprimento da tampa; X = canto (frente/trás)
J_TL = [(s0, s1, ("L" if o == "L" else ("T" if 0 < i < len(J_TL)-1 else "X"))) for i, (s0, s1, o) in enumerate(J_TL)]
# nas células interiores alternam L e T; recomputa: L nas ímpares, T nas pares interiores
_n = len(J_TL); J_TL = [(s0, s1, "X" if i in (0, _n-1) else ("L" if i % 2 == 1 else "T")) for i, (s0, s1, o) in enumerate(J_TL)]
J_TR = [(s0, s1, o.replace("L", "R")) for s0, s1, o in J_TL]

def shift(fs, d): return [(a+d, b+d) for a, b in fs]

# ---- FRENTE: corpo 188 × 49, coords locais x 0..188, z 0..49, CCW desde (0,0) ----
def part_front():
    P = []
    P += edge((0, 0), (BODY_W, 0), [])                                            # base: lisa (fundo removível)
    P += edge((BODY_W, 0), (BODY_W, FRONT_H), fingers_of(J_FR, "F"))              # canto direito
    top = shift(fingers_of(J_TF, "F"), -T)                                        # células medidas em 0..200 → corpo começa em x=6
    top = [(BODY_W - b, BODY_W - a) for a, b in top][::-1]                        # a borda de cima percorre-se da direita p/ esquerda
    P += edge((BODY_W, FRONT_H), (0, FRONT_H), top, ext_start=True, ext_end=True)
    P += edge((0, FRONT_H), (0, 0), [(FRONT_H - b, FRONT_H - a) for a, b in fingers_of(J_FL, "F")][::-1])
    return dedupe(P)

def part_back():
    P = []
    P += edge((0, 0), (BODY_W, 0), [])
    P += edge((BODY_W, 0), (BODY_W, BACK_H), fingers_of(J_BL, "B"))               # visto de fora: a esquerda da caixa fica à direita do painel de trás
    top = shift(fingers_of(J_TB, "B"), -T); top = [(BODY_W - b, BODY_W - a) for a, b in top][::-1]
    P += edge((BODY_W, BACK_H), (0, BACK_H), top, ext_start=True, ext_end=True)
    P += edge((0, BACK_H), (0, 0), [(BACK_H - b, BACK_H - a) for a, b in fingers_of(J_BR, "B")][::-1])
    return dedupe(P)

# ---- LATERAL ESQUERDA: trapézio, corpo x 0..108 (frente→trás), z 0..49 na frente, 0..74 atrás ----
def part_side(which):
    JF, JB, JT = (J_FL, J_BL, J_TL) if which == "L" else (J_FR, J_BR, J_TR)
    P = []
    P += edge((0, 0), (BODY_D, 0), [])                                            # base lisa
    P += edge((BODY_D, 0), (BODY_D, BACK_H), fingers_of(JB, which))               # canto de trás
    # topo inclinado, de trás para a frente; células medidas ao longo da tampa exterior (0..SLOPE), corpo vai de T..SLOPE-T
    topf = shift(fingers_of(JT, which), -T * SLOPE / RUN)                         # desloca pela projecção da parede na inclinação
    topf = [(SLOPE_BODY - b, SLOPE_BODY - a) for a, b in topf][::-1]
    P += edge((BODY_D, BACK_H), (0, FRONT_H), topf)
    P += edge((0, FRONT_H), (0, 0), [(FRONT_H - b, FRONT_H - a) for a, b in fingers_of(JF, which)][::-1])
    pts = dedupe(P)
    return pts if which == "L" else [(BODY_D - x, y) for x, y in pts][::-1]       # direita = espelho

# ---- TAMPA: corpo 188 × 110.3, coords x 0..188, y 0..110.3 (y ao longo da inclinação, frente em baixo) ----
def part_top():
    P = []
    P += edge((0, 0), (BODY_W, 0), shift(fingers_of(J_TF, "T"), -T))              # borda da frente (dedos para baixo, entram na frente)
    P += edge((BODY_W, 0), (BODY_W, SLOPE_BODY), shift(fingers_of(J_TR, "T"), -T * SLOPE / RUN))
    P += edge((BODY_W, SLOPE_BODY), (0, SLOPE_BODY), [(BODY_W - b, BODY_W - a) for a, b in shift(fingers_of(J_TB, "T"), -T)][::-1])
    P += edge((0, SLOPE_BODY), (0, 0), [(SLOPE_BODY - b, SLOPE_BODY - a) for a, b in shift(fingers_of(J_TL, "T"), -T * SLOPE / RUN)][::-1])
    return dedupe(P)

def part_bottom():
    c = 0.5   # folga para entrar
    w, d = BODY_W - 2*c, BODY_D - 2*c
    return [(0, 0), (w, 0), (w, d), (0, d)]

def dedupe(P):
    out = []
    for p in P:
        if not out or math.hypot(p[0]-out[-1][0], p[1]-out[-1][1]) > 1e-6: out.append(p)
    if math.hypot(out[0][0]-out[-1][0], out[0][1]-out[-1][1]) < 1e-6: out.pop()
    return out

# --------------- furos (coords locais de cada peça) ---------------
def to_top(x_out, y_out):
    """cotas do desenho (bordas exteriores da tampa: x 0..200, y 0..122.6) → corpo da tampa"""
    return (x_out - T, y_out - T * SLOPE / RUN)
# ⚠️ ESPAÇAMENTO DOS ARCADE: derivado, não escrito. Estavam a 40/100/160 — 60 mm
# entre eixos, 30 de vão entre bordos — e um pé largo aperta dois de uma vez.
# Afastam-se o máximo que a face permite, deixando `ARCADE_MARGEM` de material
# entre o bordo do furo e a parede. Com 3 botões numa face de 200 isto é o
# limite geométrico; se não chegar, o que resolve não é espaço, é um ressalto
# entre eles (ver CAIXA-ACRILICO-MONTAGEM.md).
ARCADE_D = 30.0
ARCADE_MARGEM = 12.0        # material entre o bordo do furo e a parede
_ax0 = T + ARCADE_MARGEM + ARCADE_D / 2
ARCADE_X = [_ax0, W / 2, W - _ax0]
ARCADE_VAO = ARCADE_X[1] - ARCADE_X[0] - ARCADE_D          # entre bordos

TOP_HOLES = [("c", *to_top(x, 35), ARCADE_D) for x in ARCADE_X]      # freeze A · BÚZIOS · freeze B
TOP_HOLES += [("c", *to_top(x, 92), 7) for x in (105, 140, 175)]     # encoder · pot L · pot R
TOP_HOLES += [("c", *to_top(x, y), 2.5) for x in (17.25, 72.75) for y in (75.25, 108.75)]
TOP_RECTS = [("r", *to_top(45, 92), 55, 27.5)]      # janela do OLED (omitir se a tampa for fumada)

def to_back(x_out, z_out): return (x_out - T, z_out)           # painel de trás: x exterior 0..200 → corpo; z desde o chão
BACK_HOLES = [("c", *to_back(75, 38), 9.5), ("c", *to_back(105, 38), 9.5), ("c", *to_back(155, 38), 6), ("c", *to_back(180, 38), 6)]
BACK_RECTS = [("r", *to_back(34, 38), 12, 6)] + [("r", *to_back(112 + i*14, 66), 3, 8) for i in range(6)]

# --- aberturas PEDIDAS PELA PLACA -------------------------------------------
# ⚠️ A DEPENDENCIA INVERTE-SE AQUI. A caixa impoe a placa o contorno, os furos e
# o volume; a placa impoe a caixa as ABERTURAS, porque so ela sabe onde os
# conectores calharam. Escrever aqui "USB da Daisy em x=95,6" era transcrever de
# um ficheiro para o outro, que e a familia de erro que este projecto ja pagou
# tres vezes (15cf). O `kicad/pcb_contorno.py` escreve o pedido; isto le-o.
#
# ⚠️ SAO DUAS PASSAGENS, e nao ha volta a dar: a placa so sabe onde estao os
# conectores depois de colocada contra a caixa, e a caixa so sabe onde furar
# depois disso. Correr `pcb_contorno` -> correr este -> correr `pcb_contorno`
# outra vez, e conferir que o portao de acesso passa a verde. Se as cotas da
# caixa nao mudarem entre passagens, converge a segunda.
_ped = os.path.join(os.path.dirname(os.path.abspath(__file__)), "aberturas.json")
ABERTURAS_PEDIDAS = []
if os.path.exists(_ped):
    import json
    _j = json.load(open(_ped, encoding="utf-8"))
    for a in _j.get("aberturas", []):
        # colisao com o que a parede ja tem?
        for _t, _x, _z, _w, _h in BACK_RECTS:
            if abs((_x + T) - a["x"]) < (_w + a["w"]) / 2 and abs(_z - a["z"]) < (_h + a["h"]) / 2:
                raise SystemExit("*** a abertura pedida para %s (x=%.1f z=%.1f) "
                                 "sobrepoe um recorte que a parede ja tem. Duas "
                                 "coisas nao passam pelo mesmo buraco."
                                 % (a["ref"], a["x"], a["z"]))
        for _t, _x, _z, _d in BACK_HOLES:
            if abs((_x + T) - a["x"]) < _d / 2 + a["w"] / 2 and abs(_z - a["z"]) < _d / 2 + a["h"] / 2:
                raise SystemExit("*** a abertura pedida para %s bate no furo de "
                                 "o%g em x=%.1f." % (a["ref"], _d, _x + T))
        ABERTURAS_PEDIDAS.append(a)
    # ⚠️ JUNTAR AS QUE FICAM PERTO. Os dois USB calharam a 20,6 mm de eixo a eixo,
    # com 18 de largura cada — sobrava uma TIRA DE 2,6 mm de acrilico de 3 entre
    # elas, que parte ao montar e nao se ve num portao de geometria: para o
    # nesting duas aberturas separadas estao perfeitas. O que falha nao e a
    # forma, e o material que sobra entre formas.
    PONTE_MIN = 6.0        # acrilico de 3 nao aguenta menos que isto entre furos
    ABERTURAS_PEDIDAS.sort(key=lambda a: a["x"])
    juntas, i = [], 0
    while i < len(ABERTURAS_PEDIDAS):
        a = dict(ABERTURAS_PEDIDAS[i])
        while i + 1 < len(ABERTURAS_PEDIDAS):
            b = ABERTURAS_PEDIDAS[i + 1]
            vao = (b["x"] - b["w"] / 2) - (a["x"] + a["w"] / 2)
            if vao >= PONTE_MIN or abs(b["z"] - a["z"]) > max(a["h"], b["h"]):
                break
            e, d = a["x"] - a["w"] / 2, b["x"] + b["w"] / 2
            z0 = min(a["z"] - a["h"] / 2, b["z"] - b["h"] / 2)
            z1 = max(a["z"] + a["h"] / 2, b["z"] + b["h"] / 2)
            a = dict(ref=a["ref"] + "+" + b["ref"], x=(e + d) / 2, z=(z0 + z1) / 2,
                     w=round(d - e, 2), h=round(z1 - z0, 2),
                     recuo=max(a["recuo"], b["recuo"]),
                     porque="%s e %s: juntas, sobrava tira de %.1f mm"
                            % (a["ref"], b["ref"], vao))
            i += 1
        juntas.append(a)
        i += 1
    ABERTURAS_PEDIDAS = juntas
    for a in ABERTURAS_PEDIDAS:
        BACK_RECTS.append(("r", *to_back(a["x"], a["z"]), a["w"], a["h"]))
LEFT_HOLES = [("c", 35 - T, 22, 9.5), ("c", 85 - T, 22, 9.5)]          # x da frente (corpo), z do chão
RIGHT_RECTS = [("r", BODY_D - (35 - T), 24, 24, 12), ("r", BODY_D - (85 - T), 24, 24, 12)]   # espelhado
# ⚠️ DERIVADO DA CHAPA, e não escrito. Estas seis cotas estavam fixas em
# (6·93,5·181) × (6·101), que é a chapa de T=6 (187 × 107) com os furos a 6 mm
# de cada borda — centrados sob um calço de 12×12 encostado à parede. A T=3 a
# chapa passa a 193 × 113 e as mesmas cotas punham os calços da direita e de
# trás a 12 mm da borda: deixavam de tocar na parede a que são colados, e os
# parafusos do fundo subiam para o ar. Cicatriz 15cf, outra vez: cota escrita
# à mão ao lado de coisa calculada.
_BW, _BD = BODY_W - 1, BODY_D - 1                       # a chapa de fundo
BOT_HOLES = [("c", x, y, 3.4) for x in (6, _BW / 2, _BW - 6) for y in (6, _BD - 6)]
BOT_HOLES += [("c", x - T - 0.5, y - T - 0.5, 3.2) for x, y in [(25.5, 13.5), (138.5, 13.5), (25.5, 106.5), (138.5, 106.5)]]
BOT_RECTS = [("r", 169 - T - 0.5, 50 + i*8 - T - 0.5, 36, 3) for i in range(5)]

def part_reforco():
    """Faixa de reforco, colada por baixo da tampa. Largura = corpo da tampa
    menos folga, para encostar nas duas paredes e transferir carga para elas."""
    y0, y1 = [v - T * SLOPE / RUN for v in REFORCO_Y]   # face -> corpo
    w = BODY_W - 1.0                                     # 0,5 de folga por lado
    return [(0, 0), (w, 0), (w, y1 - y0), (0, y1 - y0)]


REFORCO_HOLES = [("c", x - T - 0.5, 35 - T * SLOPE / RUN - (REFORCO_Y[0] - T * SLOPE / RUN),
                  REFORCO_FURO) for x in ARCADE_X]

PARTS = [
    ("TAMPA",   part_top(),        TOP_HOLES,  TOP_RECTS),
    ("FRENTE",  part_front(),      [],         []),
    ("TRAS",    part_back(),       BACK_HOLES, BACK_RECTS),
    ("ESQ",     part_side("L"),    LEFT_HOLES, []),
    ("DIR",     part_side("R"),    [],         RIGHT_RECTS),
    ("FUNDO",   part_bottom(),     BOT_HOLES,  BOT_RECTS),
    ("NERVURA", [(0,0),(12,0),(12,176),(0,176)], [], []),
]
# calços: 6 de BAIXO com sexágono para porca M3 cativa (5,7 entre faces), 6 de CIMA lisos (tapam a porca)
def hexagono(cx, cy, af):
    r = af / math.sqrt(3)          # raio circunscrito de um hexágono com "af" entre faces
    return [(cx + r*math.cos(math.radians(60*k+30)), cy + r*math.sin(math.radians(60*k+30))) for k in range(6)]
for i in range(6): PARTS.append((f"CB{i+1}", [(0,0),(12,0),(12,12),(0,12)], [], [("hex", 6, 6, 5.7)]))
for i in range(6): PARTS.append((f"CT{i+1}", [(0,0),(12,0),(12,12),(0,12)], [], []))
# so entra na chapa quando o furo estiver decidido
if REFORCO and REFORCO_FURO: PARTS.append(("REFORCO", part_reforco(), REFORCO_HOLES, []))

# --------------- nesting numa chapa 384 × 384 (Ponoko P2) ---------------
SHEET = (384.0, 384.0); GAP = 3.0
def bbox(pts):
    xs = [p[0] for p in pts]; ys = [p[1] for p in pts]; return min(xs), min(ys), max(xs), max(ys)
placed = []
by = {n: (n, p, h, r) for n, p, h, r in PARTS}


def _area(pts):
    """Área com SINAL. O sinal diz a orientação do contorno."""
    return sum(pts[i][0] * pts[i-1][1] - pts[i-1][0] * pts[i][1]
               for i in range(len(pts))) / 2


def _rot90(pts):
    """(x, y) -> (-y, x): roda 90° CCW. O `place` reencosta pelo bbox.

    ⚠️ TEM DE SER ROTAÇÃO, NUNCA ESPELHO. Uma peça espelhada corta na
    perfeição e monta ao contrário — os dedos ficam do lado errado da junta, e
    isso só se descobre com as peças na mão e a chapa paga. A matriz [(0,-1),
    (1,0)] tem determinante +1, logo preserva a orientação; o assert abaixo
    confirma-o na peça concreta em vez de confiar na álgebra."""
    out = [(-y, x) for x, y in pts]
    a0, a1 = _area(pts), _area(out)
    if a0 * a1 <= 0 or abs(abs(a1) - abs(a0)) > 1e-6:
        raise SystemExit("*** _rot90 espelhou ou deformou a peça: área %+.3f "
                         "-> %+.3f. Peça espelhada corta bem e monta ao "
                         "contrário." % (a0, a1))
    return out


def place(name, x, y, rot=0):
    """Encosta a peça com o canto inferior-esquerdo do bbox em (x, y)."""
    n, p, h, r = by[name]
    if rot == 90:
        p = _rot90(p)
        # furos: mesma rotação. Rectângulos trocam largura por altura.
        h = [(tp, -b, a, *rest) for tp, a, b, *rest in h]
        r = [(tp, -b, a, rest[1], rest[0]) if tp == "r" else (tp, -b, a, *rest)
             for tp, a, b, *rest in r]
    x0, y0, x1, y1 = bbox(p)
    dx, dy = x - x0, y - y0
    placed.append((n, [(a + dx, b + dy) for a, b in p],
                   [(tp, a + dx, b + dy, *rest) for tp, a, b, *rest in h],
                   [(tp, a + dx, b + dy, *rest) for tp, a, b, *rest in r]))


def _dim(n, rot=0):
    x0, y0, x1, y1 = bbox(by[n][1])
    w, h = x1 - x0, y1 - y0
    return (h, w) if rot == 90 else (w, h)


def encaixa():
    """Encaixe por bottom-left-fill, com rotação de 90°, minimizando a área da
    caixa envolvente. Determinístico: ordens fixas + baralhadas com semente.

    ⚠️ POR QUE NÃO É À MÃO, E POR QUE NÃO SÃO PRATELEIRAS. As coordenadas já
    estiveram escritas uma a uma: funcionavam a T=6 e a T=3 deixavam 1,4 mm de
    margem, porque as peças CRESCEM (o fundo passa de 187×107 para 193×113).
    Substituí por três prateleiras — melhor, mas ainda uma disposição escolhida
    por mim, e o dB viu no visualizador da Ponoko que sobrava espaço.

    Medido: prateleiras davam 362,4 × 376,5 = 1364 cm². Este dá ~1100 cm², 81%.
    O chão teórico (soma das áreas das peças) é 960 cm².

    ⚠️ O QUE ISTO **NÃO** RESOLVE: a chapa continua a ser P2. A TAMPA tem 200 mm
    de largura e a P1 tem 181 — nenhuma disposição a faz caber, nem rodada.
    Encolher a chapa exige encolher a CAIXA, não arrumá-la melhor.

    Não é óptimo, é repetível — e o `portao_nesting` corre a seguir e recusa-se
    a escrever se alguma peça sair da chapa ou tocar noutra."""
    import random
    itens = [(n, *_dim(n)) for n, _, _, _ in PARTS]
    lim = (SHEET[0] - 2 * GAP, SHEET[1] - 2 * GAP)

    def tenta(ordem):
        postas = []
        for n, w, h in ordem:
            melhor = None
            ys = sorted({0.0} | {y + ph + GAP for _, y, _, ph in postas})
            xs = sorted({0.0} | {x + pw + GAP for x, _, pw, _ in postas})
            for ww, hh in ((w, h), (h, w)):
                for cy in ys:
                    for cx in xs:
                        if cx + ww > lim[0] or cy + hh > lim[1]:
                            continue
                        if any(not (cx + ww + GAP <= px or px + pw + GAP <= cx or
                                    cy + hh + GAP <= py or py + ph + GAP <= cy)
                               for px, py, pw, ph in postas):
                            continue
                        chave = (cy + hh, cx + ww)
                        if melhor is None or chave < melhor[0]:
                            melhor = (chave, n, cx, cy, ww, hh)
                        break
            if melhor is None:
                return None
            _, n, cx, cy, ww, hh = melhor
            postas.append((cx, cy, ww, hh))
            ordem_posta.append((n, cx + GAP, cy + GAP, 0 if (ww, hh) == _dim(n) else 90))
        W = max(x + w for x, _, w, _ in postas)
        H = max(y + h for _, y, _, h in postas)
        return W * H

    ordens = [sorted(itens, key=lambda i: -max(i[1], i[2])),
              sorted(itens, key=lambda i: -i[1] * i[2]),
              sorted(itens, key=lambda i: -i[2]),
              sorted(itens, key=lambda i: -i[1])]
    r = random.Random(7)                      # semente fixa: o resultado repete-se
    for _ in range(1200):
        o = list(itens); r.shuffle(o); ordens.append(o)

    melhor_area, melhor_plano = None, None
    for o in ordens:
        ordem_posta = []
        a = tenta(o)
        if a is not None and (melhor_area is None or a < melhor_area):
            melhor_area, melhor_plano = a, list(ordem_posta)
    if melhor_plano is None:
        raise SystemExit("*** nada encaixa na chapa a T=%g." % T)
    for n, x, y, rot in melhor_plano:
        place(n, x, y, rot)


ordem_posta = []
encaixa()
ymax = max(bbox(p)[3] for _, p, _, _ in placed); xmax = max(bbox(p)[2] for _, p, _, _ in placed)

# --------------- PORTÃO: nesting ---------------
def portao_nesting():
    erros = []
    for n, p, _, _ in placed:
        x0, y0, x1, y1 = bbox(p)
        if x0 < 0 or y0 < 0 or x1 > SHEET[0] or y1 > SHEET[1]:
            erros.append(f"{n} fora da chapa: ({x0:.1f},{y0:.1f})-({x1:.1f},{y1:.1f})")
    for i in range(len(placed)):
        for j in range(i+1, len(placed)):
            a = bbox(placed[i][1]); b = bbox(placed[j][1])
            if not (a[2] + GAP*0.9 <= b[0] or b[2] + GAP*0.9 <= a[0] or a[3] + GAP*0.9 <= b[1] or b[3] + GAP*0.9 <= a[1]):
                erros.append(f"{placed[i][0]} × {placed[j][0]} sobrepostas ou a menos de {GAP} mm")
    # furos dentro da sua peça
    for n, p, h, r in placed:
        x0, y0, x1, y1 = bbox(p)
        for (_, cx, cy, d) in h:
            if not (x0 + d/2 <= cx <= x1 - d/2 and y0 + d/2 <= cy <= y1 - d/2): erros.append(f"{n}: furo ø{d} em ({cx:.1f},{cy:.1f}) sai da peça")
        for rr in r:
            if rr[0] == "hex":
                _, cx, cy, af = rr; rc = af / math.sqrt(3)
                if not (x0 + rc <= cx <= x1 - rc and y0 + rc <= cy <= y1 - rc): erros.append(f"{n}: sexágono sai da peça")
                continue
            _, cx, cy, w, hh = rr
            if not (x0 <= cx - w/2 and cx + w/2 <= x1 and y0 <= cy - hh/2 and cy + hh/2 <= y1): erros.append(f"{n}: recorte {w}×{hh} em ({cx:.1f},{cy:.1f}) sai da peça")
    return erros

# --------------- métricas ---------------
def perim(p): return sum(math.hypot(p[i][0]-p[i-1][0], p[i][1]-p[i-1][1]) for i in range(len(p)))
cut = sum(perim(p) for _, p, _, _ in placed)
cut += sum(math.pi*d for _, _, h, _ in placed for (_, _, _, d) in h)
cut += sum((2*(rr[3]+rr[4]) if rr[0]!='hex' else 6*rr[3]/math.sqrt(3)) for _, _, _, r in placed for rr in r)
area = sum((bbox(p)[2]-bbox(p)[0])*(bbox(p)[3]-bbox(p)[1]) for _, p, _, _ in placed)

# --------------- SVG 1:1 ---------------
def svg():
    o = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{SHEET[0]}mm" height="{SHEET[1]}mm" viewBox="0 0 {SHEET[0]} {SHEET[1]}">',
]
    CUT = 'fill="none" stroke="#0000FF" stroke-width="0.01"'
    ENG = 'fill="none" stroke="#FF0000" stroke-width="0.01"'
    for n, p, h, r in placed:
        d = "M " + " L ".join(f"{x:.3f} {SHEET[1]-y:.3f}" for x, y in p) + " Z"
        o.append(f'<path d="{d}" {CUT}/>')
        for (_, cx, cy, dia) in h: o.append(f'<circle cx="{cx:.3f}" cy="{SHEET[1]-cy:.3f}" r="{dia/2:.3f}" {CUT}/>')
        for rr in r:
            if rr[0] == "hex":
                _, cx, cy, af = rr; hp = hexagono(cx, cy, af)
                o.append('<path d="M ' + " L ".join(f"{x:.3f} {SHEET[1]-y:.3f}" for x, y in hp) + f' Z" {CUT}/>')
            else:
                _, cx, cy, w, hh = rr
                o.append(f'<rect x="{cx-w/2:.3f}" y="{SHEET[1]-cy-hh/2:.3f}" width="{w:.3f}" height="{hh:.3f}" {CUT}/>')
    o.append("</svg>"); return "\n".join(o)

# --------------- DXF R12 ---------------
def dxf():
    e = ["0", "SECTION", "2", "ENTITIES"]
    def line(a, b, layer): e.extend(["0", "LINE", "8", layer, "10", f"{a[0]:.4f}", "20", f"{a[1]:.4f}", "30", "0", "11", f"{b[0]:.4f}", "21", f"{b[1]:.4f}", "31", "0"])
    for n, p, h, r in placed:
        for i in range(len(p)): line(p[i-1], p[i], "CORTE")
        for (_, cx, cy, dia) in h: e.extend(["0", "CIRCLE", "8", "CORTE", "10", f"{cx:.4f}", "20", f"{cy:.4f}", "30", "0", "40", f"{dia/2:.4f}"])
        for rr in r:
            if rr[0] == "hex":
                c = hexagono(rr[1], rr[2], rr[3])
            else:
                _, cx, cy, w, hh = rr; c = [(cx-w/2, cy-hh/2), (cx+w/2, cy-hh/2), (cx+w/2, cy+hh/2), (cx-w/2, cy+hh/2)]
            for i in range(len(c)): line(c[i-1], c[i], "CORTE")
    e.extend(["0", "ENDSEC", "0", "EOF"]); return "\n".join(e) + "\n"

def svg_preview():
    return svg().replace('stroke-width="0.01"', 'stroke-width="0.35"').replace('font-size="4"', 'font-size="5"')

if __name__ == "__main__":
    print(f"cotuca_caixa_corte.py v{VERSAO} — T = {T} mm, kerf = {K} mm")
    if REFORCO and not REFORCO_FURO:
        raise SystemExit(FALTA_REFORCO_FURO)
    # ⚠️ ao lado do próprio script. As versões anteriores escreviam
    # `/mnt/user-data/outputs/` e `/home/claude/`, caminhos de outra máquina —
    # a segunda rebentava a meio, DEPOIS de o SVG já ter saído, o que é o pior
    # sítio para rebentar: parece falha total e não é.
    OUT = os.path.dirname(os.path.abspath(__file__)) + os.sep
    tag = f"acrilico_{T:.1f}".replace(".", "p")   # 3.0 -> "3p0", nunca "3"
    data = REV_DATA.replace("-", "")
    erros = portao_nesting()
    if erros:
        print("PORTÃO RECUSOU:"); [print("  !!", e) for e in erros]; sys.exit(1)
    print(f"portão nesting: {len(placed)} peças, disjuntas, dentro da chapa, furos dentro das peças — 0 erros")
    import hashlib, json
    corpo = svg()
    # ⚠️ o digest e do DESENHO, e o nome nao entra no desenho — logo e estavel.
    dig = hashlib.sha256(corpo.encode()).hexdigest()[:8]
    raiz = f"cotuca_caixa_{data}_r{REV}_{tag}_{dig}"
    # ⚠️ A GUARDA QUE FECHA O LAÇO: se a geometria mudou e o REV nao, ninguem
    # dava por isso — era como as duas caixas com o mesmo nome, outra vez.
    reg = os.path.join(OUT, "revisoes.json")
    hist = json.load(open(reg, encoding="utf-8")) if os.path.exists(reg) else {}
    chave = f"r{REV}·{tag}"
    if chave in hist and hist[chave]["digest"] != dig:
        raise SystemExit(
            "*** a geometria mudou e o REV nao.\n"
            "    r%d/%s ja saiu com digest %s e agora da %s.\n"
            "    Sobe o REV e a REV_DATA, ou desfaz a mudanca. Duas geometrias\n"
            "    com a mesma revisao e o defeito que o digest existe para\n"
            "    apanhar — e apanhou." % (REV, tag, hist[chave]["digest"], dig))
    hist[chave] = {"digest": dig, "data": REV_DATA, "ficheiro": raiz + ".svg",
                   "pecas": len(placed), "corte_m": round(cut / 1000, 2)}
    json.dump(hist, open(reg, "w", encoding="utf-8"), indent=2, ensure_ascii=False)
    open(OUT + raiz + ".svg", "w").write(corpo)
    open(OUT + raiz + "_previa.svg", "w").write(svg_preview())
    open(OUT + raiz + ".dxf", "w").write(dxf())
    print(f"escrito: {raiz}.svg · _previa.svg · .dxf")
    print(f"identidade: rev {REV} · digest {dig}")
    print("  ⚠️ o digest E a identidade. Geometria diferente -> ficheiro "
          "diferente, por construcao. Encomenda-se pelo nome inteiro.")
    print(f"chapa {SHEET[0]:.0f}×{SHEET[1]:.0f} · usado {xmax:.1f}×{ymax:.1f} · {len(placed)} peças")
    margem = min(SHEET[0] - xmax, SHEET[1] - ymax)
    # ⚠️ Os 3 mm são MEUS, não da Ponoko. Eles dizem "não uses a aresta crua da
    # chapa para a tua peça" e não publicam número nenhum para o afastamento à
    # borda (help.ponoko.com/en/articles/2688726). Os números que publicam são
    # entre PEÇAS: 1 mm no guia de encaixe, 2 mm nos materiais impressos.
    # Facto de fabrico sem fonte citada é hipótese, e diz-se como hipótese.
    aviso = "  ⚠️ apertado (regra nossa de 3 mm; a Ponoko não publica número)" if margem < 3 else ""
    print(f"margem à borda da chapa: {margem:.1f} mm{aviso}")
    print(f"corte total ≈ {cut/1000:.2f} m · área das peças {area/1e4:.1f} dm² · tampa {SLOPE:.1f} mm · ângulo {ANG:.1f}°")
    print(f"frente {HF} / trás {HB} · interior sob a tampa: frente {HF-2*T:.0f}, trás {HB-2*T:.0f}")
    print(f"arcade em x = {' · '.join('%g' % x for x in ARCADE_X)} · "
          f"{ARCADE_X[1]-ARCADE_X[0]:.0f} mm entre eixos, {ARCADE_VAO:.0f} entre bordos")
    if ABERTURAS_PEDIDAS:
        print(f"aberturas pedidas pela placa: {len(ABERTURAS_PEDIDAS)}")
        for a in ABERTURAS_PEDIDAS:
            print(f"  {a['ref']}: {a['w']:g}x{a['h']:g} em x={a['x']:g} z={a['z']:g}"
                  f"  ({a['porque']}; modulo a {a['recuo']:g} mm da parede)")
    else:
        print("⚠️ sem caixa/aberturas.json — corre o kicad/pcb_contorno.py primeiro")
