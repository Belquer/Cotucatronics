#!/usr/bin/env python3
"""cotuca_caixa_corte.py — ficheiros de corte da caixa em acrílico.

Estrutura (decidida 05/09):
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

VERSAO = "0.3.0"

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


T = _arg("t", 6.0)          # espessura
K = _arg("kerf", 0.2)       # kerf
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
TOP_HOLES = [("c", *to_top(40, 35), 30), ("c", *to_top(100, 35), 30), ("c", *to_top(160, 35), 30),   # freeze A · BÚZIOS (branco) · freeze B
             ("c", *to_top(105, 92), 7), ("c", *to_top(140, 92), 7), ("c", *to_top(175, 92), 7)]
TOP_HOLES += [("c", *to_top(x, y), 2.5) for x in (17.25, 72.75) for y in (75.25, 108.75)]
TOP_RECTS = [("r", *to_top(45, 92), 55, 27.5)]      # janela do OLED (omitir se a tampa for fumada)

def to_back(x_out, z_out): return (x_out - T, z_out)           # painel de trás: x exterior 0..200 → corpo; z desde o chão
BACK_HOLES = [("c", *to_back(75, 38), 9.5), ("c", *to_back(105, 38), 9.5), ("c", *to_back(155, 38), 6), ("c", *to_back(180, 38), 6)]
BACK_RECTS = [("r", *to_back(34, 38), 12, 6)] + [("r", *to_back(112 + i*14, 66), 3, 8) for i in range(6)]
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
    """Encaixe em TRÊS prateleiras, com o FUNDO e as laterais rodados.

    ⚠️ AS ALTURAS SAIEM DOS BBOX, NÃO DE CONSTANTES. A T=6 a disposição antiga
    (uma coluna alta com TAMPA+FRENTE+TRÁS+FUNDO) media 376,5 numa chapa de
    384 e passava; a T=3 as peças CRESCEM — o fundo vai de 187×107 para
    193×113 e as paredes ficam mais altas por HF−T — e a mesma coluna dava
    382,6, ou seja 1,4 mm de margem. Desenho colado à borda é recusa no upload.

    A cura foi tirar as duas peças grandes da mesma coluna: o FUNDO passa a
    abrir a prateleira do meio DEITADO, e a altura que ele desperdiçava passa a
    levar o TRÁS e a nervura ao lado.

    Não é óptimo — é repetível e VERIFICADO: o `portao_nesting` corre a seguir
    e recusa-se a escrever se alguma peça sair da chapa ou tocar noutra."""
    # ---- prateleira A: a tampa, e as duas laterais DEITADAS ao lado dela
    yA = GAP
    place("TAMPA", GAP, yA)
    x = GAP + _dim("TAMPA")[0] + GAP
    for n in ("ESQ", "DIR"):
        place(n, x, yA, rot=90)
        x += _dim(n, 90)[0] + GAP
    hA = max(_dim("TAMPA")[1], _dim("ESQ", 90)[1], _dim("DIR", 90)[1])

    # ---- prateleira B: o FUNDO de pé abre a altura; trás e nervura ao lado
    yB = yA + hA + GAP
    place("FUNDO", GAP, yB, rot=90)
    x = GAP + _dim("FUNDO", 90)[0] + GAP
    place("TRAS", x, yB)
    x += _dim("TRAS")[0] + GAP
    place("NERVURA", x, yB)
    hB = _dim("FUNDO", 90)[1]

    # ---- prateleira C: a frente, e os 12 calços em duas filas
    yC = yB + hB + GAP
    place("FRENTE", GAP, yC)
    x = GAP + _dim("FRENTE")[0] + GAP
    for i in range(6):
        place(f"CB{i+1}", x + i * (12 + GAP), yC)
        place(f"CT{i+1}", x + i * (12 + GAP), yC + 12 + GAP)


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
    # ⚠️ ao lado do próprio script. As versões anteriores escreviam
    # `/mnt/user-data/outputs/` e `/home/claude/`, caminhos de outra máquina —
    # a segunda rebentava a meio, DEPOIS de o SVG já ter saído, o que é o pior
    # sítio para rebentar: parece falha total e não é.
    OUT = os.path.dirname(os.path.abspath(__file__)) + os.sep
    tag = f"acrilico_{T:.1f}".replace(".", "p")   # 3.0 -> "3p0", nunca "3"
    erros = portao_nesting()
    if erros:
        print("PORTÃO RECUSOU:"); [print("  !!", e) for e in erros]; sys.exit(1)
    print(f"portão nesting: {len(placed)} peças, disjuntas, dentro da chapa, furos dentro das peças — 0 erros")
    open(OUT + f"cotuca_caixa_corte_{tag}.svg", "w").write(svg())
    open(OUT + f"cotuca_caixa_corte_{tag}_previa.svg", "w").write(svg_preview())
    open(OUT + f"cotuca_caixa_corte_{tag}.dxf", "w").write(dxf())
    print(f"escrito: cotuca_caixa_corte_{tag}.svg · _previa.svg · .dxf")
    print(f"chapa {SHEET[0]:.0f}×{SHEET[1]:.0f} · usado {xmax:.1f}×{ymax:.1f} · {len(placed)} peças")
    margem = min(SHEET[0] - xmax, SHEET[1] - ymax)
    aviso = "  ⚠️ APERTADO — a Ponoko recusa desenho colado à borda" if margem < 3 else ""
    print(f"margem à borda da chapa: {margem:.1f} mm{aviso}")
    print(f"corte total ≈ {cut/1000:.2f} m · área das peças {area/1e4:.1f} dm² · tampa {SLOPE:.1f} mm · ângulo {ANG:.1f}°")
    print(f"frente {HF} / trás {HB} · interior sob a tampa: frente {HF-2*T:.0f}, trás {HB-2*T:.0f}")
