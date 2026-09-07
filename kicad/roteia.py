#!/usr/bin/env python3
"""roteia — as duas camadas, com via, e a prova de que nao partiu o plano.

⚠️ EXIGE QUE F.Cu NAO TENHA ZONA DE COBRE, e recusa-se a correr se tiver.

O ARRANJO QUE ESTE ROTEADOR PRESSUPOE (e que o `pcb_contorno.costura()` cria):

    B.Cu   plano solido de terra, inteiro, e o sinal que precisar de passar
    F.Cu   sinal, e nenhuma zona
           + uma via em cada pad SMD de terra a descer ao plano

POR QUE DUAS CAMADAS E NAO UMA.

A versao anterior so escrevia em F.Cu, e parou em 47 ligacoes por fazer num
total de 71. Nao foi falta de esperteza da busca: numa camada so, um sinal que
precise de atravessar outro nao tem por onde. **Uma camada de sinal e um grafo
planar, e as ligacoes de uma placa nao sao um grafo planar.** A saida e a
mesma da industria ha quarenta anos — salta-se para o outro lado com duas
vias, anda-se um bocado, e volta-se.

⚠️⚠️ E SALTAR PARA B.Cu ABRE UMA FENDA NO PLANO. O cobre recua dos dois lados
da pista, e o plano fica com um rasgo. Um rasgo curto e inofensivo; um rasgo
que separe o plano em duas ilhas destroi a terra da placa, e o DRC nao chama a
isso um erro de geometria — chama-lhe `Zone [GND] <-> Zone [GND]`, escondido
no meio das ligacoes por fazer. Foi assim que uma tentativa anterior baixou o
total de 71 para 52 **enquanto partia a ilha de AGND em oito pedacos**, com o
DRC a dizer geometria zero o tempo todo (15co).

Por isso a via e CARA no custo da busca — ela prefere dar a volta em F.Cu a
saltar — e por isso existe a retraccao por bisseccao no fim: escreve-se tudo,
mede-se, e se algum plano se partiu procura-se **qual** das travessias o
partiu, devolvendo essas a lista da pessoa. O plano inteiro vale mais que
qualquer ligacao automatica.

⚠️ A FERRAMENTA CONFERE O PROPRIO TRABALHO, e nao entrega o que nao consegue
provar. Entregar cobre que o DRC acusa e pior que nao entregar nada.

⚠️ E PREFERE A LINHA RECTA. O custo penaliza cada mudanca de direccao, senao a
busca devolve escadas de mil degraus, electricamente validas e humanamente
ilegiveis — e uma placa que ninguem le nao se depura.

⚠️ VERIFICA-SE COM AS ZONAS CHEIAS (`--refill-zones`). Sem isso o DRC compara
as pistas novas com o enchimento ANTERIOR e devolve dezenas de `clearance`
fantasma. E antes de dizeres que o `kicad-cli` nao faz X, corre o `--help`:
escrevi num commit que ele nao sabia encher zonas, e a flag chamava-se
exactamente aquilo.

USO:  roteia.py <ficheiro.kicad_pcb> [--escreve] [--rede NOME] [--so-uma-face]
"""
import io, json, math, os, re, subprocess, sys, uuid

VERSAO = "1.5.0"
NUM = r"-?\d+(?:\.\d+)?"
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
# ⚠️ A FOLGA DO ROTEADOR NAO PODE SER MAIS APERTADA QUE A REGRA DA PLACA, mas
# tambem nao ha razao para ser MUITO mais larga. Estive em 0,35 mm com a regra
# da placa em 0,20 — quase o dobro — e paguei isso em caminhos que nao existiam.
# 0,26 deixa 30% de margem sobre a regra, que chega para o erro de discretizacao
# da grelha, e abriu caminhos que a 0,35 estavam fechados.
# ⚠️ APERTAR A FOLGA PIOROU O RESULTADO, e isso e contra-intuitivo o bastante
# para ficar escrito. Medido, sempre de raiz e com duas passadas:
#
#     FOLGA 0,35 · PASSO 0,25  ->  23 ligacoes por fazer   <- fica esta
#     FOLGA 0,26 · PASSO 0,20  ->  44
#     FOLGA 0,26 · PASSO 0,25  ->  34
#
# A razao e a ganancia: com mais espaco disponivel o roteador enfia mais pistas
# cedo, e essas pistas fecham corredores de que as ligacoes seguintes
# precisavam — e mais delas acabam retiradas por estragarem o plano. Um
# roteador guloso melhora quando lhe apertamos o apetite, nao a folga.
#
# ⚠️ E a malha mais fina nao acha mais caminhos: acha os mesmos mais devagar, e
# bate no tecto de nos antes de chegar ao fim.
#
# ⚠️ Da primeira vez mudei as duas de uma so vez e a medicao nao soube dizer
# qual delas tinha feito o estrago. Uma variavel de cada vez, mesmo quando
# parece obvio que andam juntas.
FOLGA = 0.35            # entre a pista e o que nao e dela (regra da placa: 0,20)
PASSO = 0.25            # mm por celula
VIRAGEM = 3.0           # o que custa dobrar, em celulas
CUSTO_VIA = 30.0        # ⚠️ em celulas. Alto de proposito: a via abre fenda no
                        # plano, portanto so se salta quando nao ha volta a dar.
VIA_R = 0.3             # raio do anel da via
CAMADAS = ("F.Cu", "B.Cu")
LARGURA = {"+15V": 1.0, "Net-(F1-Pad1)": 1.0, "V_TRILHO_A2": 1.0,
           "V+PRE": 0.5, "VBIAS": 0.5, "+5V": 0.5, "+5V_PONTE": 0.5, "+3V3A": 0.5}


# --- leitura da placa -------------------------------------------------------

def pads_da_placa(t):
    """(x, y, raio, rede, camadas) — e `camadas` importa: um pad SMD so existe
    de um lado, e uma pista do outro lado passa-lhe por baixo sem lhe tocar.
    Tratar todos os pads como obstaculo nas duas camadas custa metade dos
    caminhos por nada."""
    out = []
    for m in re.finditer(r"\n\t\(footprint ", t):
        j = m.start() + 1
        d = 0
        for k in range(j, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    bl = t[j:k + 1]
                    break
        a = re.search(r"\(at (" + NUM + r") (" + NUM + r")(?: (" + NUM + r"))?\)", bl)
        fx, fy = float(a.group(1)), float(a.group(2))
        ang = math.radians(float(a.group(3) or 0))
        for pm in re.finditer(r'\(pad "[^"]*" (\w+)[\s\S]*?\(at (' + NUM + r") (" + NUM
                              + r")(?: " + NUM + r")?\)[\s\S]*?\(size (" + NUM + r") ("
                              + NUM + r")\)([\s\S]{0,300}?)\(uuid", bl):
            tipo, px, py, sw, sh, resto = pm.groups()
            px, py, sw, sh = float(px), float(py), float(sw), float(sh)
            n = re.search(r'\(net "([^"]+)"\)', resto)
            lay = re.search(r'\(layers ([^)]*)\)', resto)
            txt = lay.group(1) if lay else ""
            if tipo == "thru_hole" or "*.Cu" in txt:
                cams = (0, 1)
            elif "B.Cu" in txt:
                cams = (1,)
            else:
                cams = (0,)
            out.append((fx + px * math.cos(ang) + py * math.sin(ang),
                        fy - px * math.sin(ang) + py * math.cos(ang),
                        max(sw, sh) / 2, n.group(1) if n else None, cams))
    return out


def pistas_da_placa(t):
    """(x1, y1, x2, y2, rede, camada)."""
    out = []
    for m in re.finditer(r"\(segment\s*\(start (" + NUM + r") (" + NUM
                         + r")\)\s*\(end (" + NUM + r") (" + NUM
                         + r")\)[\s\S]{0,200}?\(layer \"([^\"]+)\"\)[\s\S]{0,200}?"
                         r"\(net \"([^\"]+)\"\)", t):
        x1, y1, x2, y2 = (float(v) for v in m.groups()[:4])
        out.append((x1, y1, x2, y2, m.group(6), CAMADAS.index(m.group(5))
                    if m.group(5) in CAMADAS else 0))
    return out


def vias_da_placa(t):
    return [(float(m.group(1)), float(m.group(2)), m.group(3))
            for m in re.finditer(r"\(via\s*\(at (" + NUM + r") (" + NUM
                                 + r")\)[\s\S]{0,220}?\(net \"([^\"]+)\"\)", t)]


def dist_ponto_seg(px, py, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1
    if dx == dy == 0:
        return math.hypot(px - x1, py - y1)
    tt = max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - (x1 + tt * dx), py - (y1 + tt * dy))


class Grelha:
    """Indice espacial dos obstaculos, para a busca nao varrer a lista toda."""

    def __init__(self, itens, celula=4.0):
        self.c = celula
        self.d = {}
        for it in itens:
            k = (int(it[0] / celula), int(it[1] / celula))
            for a in (-1, 0, 1):
                for b in (-1, 0, 1):
                    self.d.setdefault((k[0] + a, k[1] + b), []).append(it)

    def perto(self, x, y):
        return self.d.get((int(x / self.c), int(y / self.c)), ())


# --- a busca, em tres dimensoes ---------------------------------------------

def astar(A, B, camA, camB, rede, larg, gs, limites, so_uma_face=False):
    """A* sobre (x, y, camada). `camA`/`camB` sao as camadas em que cada ponta
    e alcancavel: um pad passante vale nas duas, um SMD so na sua.

    ⚠️ MEIA DIAGONAL DE MARGEM A MAIS. A busca so testa CENTROS de celula, e o
    segmento entre duas passa mais perto de um obstaculo do que qualquer das
    pontas — ate meia diagonal. Sem isto saem folgas de 0,02 mm que o DRC
    apanha e que ninguem consegue explicar olhando para o caminho."""
    import heapq
    x0, y0, x1, y1 = limites
    nx = int((x1 - x0) / PASSO) + 1
    ny = int((y1 - y0) / PASSO) + 1
    m = larg / 2 + FOLGA + PASSO * 0.71
    mv = VIA_R + FOLGA + PASSO * 0.71

    def pos(c):
        return (x0 + c[0] * PASSO, y0 + c[1] * PASSO)

    def cel(p):
        return (int(round((p[0] - x0) / PASSO)), int(round((p[1] - y0) / PASSO)))

    cache = {}

    def ok(c, L, marg=None):
        chave = (c, L, marg is not None)
        if chave in cache:
            return cache[chave]
        if not (0 <= c[0] < nx and 0 <= c[1] < ny):
            cache[chave] = False
            return False
        px, py = pos(c)
        mm = marg if marg is not None else m
        v = True
        for ox, oy, orr, on in gs[L].perto(px, py):
            if on == rede:
                continue
            if math.hypot(px - ox, py - oy) < orr + mm:
                v = False
                break
        cache[chave] = v
        return v

    def cabe_via(c):
        return ok(c, 0, mv) and ok(c, 1, mv)

    ini = [(cel(A), L) for L in camA]
    fins = {(cel(B), L) for L in camB}
    fimxy = cel(B)
    DIRS = [(1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)]

    def h(e):
        return (abs(e[0][0] - fimxy[0]) + abs(e[0][1] - fimxy[1])) * PASSO

    fila, veio, custo = [], {}, {}
    for e in ini:
        custo[e] = 0.0
        heapq.heappush(fila, (h(e), 0.0, e, None))
    limite = 0
    while fila:
        limite += 1
        # ⚠️ TECTO. Sem ele uma ligacao impossivel come tudo. Subi-o de 300k
        # para 900k a pensar que fecharia mais ligacoes, e medi: fechou
        # EXACTAMENTE as mesmas, tres vezes mais devagar. Uma busca que nao
        # acha em 300 mil nos nao acha em 900 mil — nao lhe falta paciencia,
        # falta-lhe caminho. Voltou a 300k.
        if limite > 300000:
            return None
        _, g, e, d = heapq.heappop(fila)
        if e in fins:
            cam, cur = [e], e
            while cur in veio:
                cur = veio[cur][0]
                cam.append(cur)
            cam.reverse()
            # cantos: guarda so onde a direccao muda ou a camada muda
            pts, ant = [(pos(cam[0][0]), cam[0][1])], None
            for i in range(1, len(cam)):
                mudou_cam = cam[i][1] != cam[i - 1][1]
                dd = (cam[i][0][0] - cam[i - 1][0][0], cam[i][0][1] - cam[i - 1][0][1])
                if mudou_cam:
                    pts.append((pos(cam[i - 1][0]), cam[i - 1][1]))
                    pts.append((pos(cam[i][0]), cam[i][1]))
                    ant = None
                    continue
                if ant and dd != ant:
                    pts.append((pos(cam[i - 1][0]), cam[i - 1][1]))
                ant = dd
            pts.append((pos(cam[-1][0]), cam[-1][1]))
            # limpa pontos repetidos seguidos
            lim = [pts[0]]
            for q in pts[1:]:
                if q != lim[-1]:
                    lim.append(q)
            return lim
        (cx, cy), L = e
        for dd in DIRS:
            n = ((cx + dd[0], cy + dd[1]), L)
            if n not in fins and not ok(n[0], L):
                continue
            passo = PASSO * (1.414 if dd[0] and dd[1] else 1.0)
            ng = g + passo + (VIRAGEM * PASSO if d and dd != d else 0.0)
            if ng < custo.get(n, 1e9):
                custo[n] = ng
                veio[n] = (e, dd)
                heapq.heappush(fila, (ng + h(n), ng, n, dd))
        if not so_uma_face:
            n = ((cx, cy), 1 - L)
            if (n in fins or ok(n[0], 1 - L)) and cabe_via((cx, cy)):
                ng = g + CUSTO_VIA * PASSO
                if ng < custo.get(n, 1e9):
                    custo[n] = ng
                    veio[n] = (e, None)
                    heapq.heappush(fila, (ng + h(n), ng, n, None))
    return None


def amostra(lista, A, B, rede, passo=0.4, r=0.18):
    n = max(2, int(math.hypot(B[0] - A[0], B[1] - A[1]) / passo))
    for i in range(n + 1):
        lista.append((A[0] + (B[0] - A[0]) * i / n,
                      A[1] + (B[1] - A[1]) * i / n, r, rede))


# --- o que o DRC diz --------------------------------------------------------

def estado(caminho):
    """(redes cujas pistas violam, ligacoes por fazer POR REDE).

    ⚠️ O ESTRAGO NO PLANO NAO E UM ERRO DE GEOMETRIA, e tem DUAS formas.
    A obvia e `Zone [X] <-> Zone [X]`: o plano partido em ilhas. A outra, que
    me escapou, e um **pad cortado do plano** — a pista em B.Cu passa entre o
    pad e o cobre, o pad fica orfao, e isso aparece como uma ligacao por fazer
    normalissima na rede GND. O plano continua inteiro. A terra e que nao.

    Medido: um roteamento baixou o total de 47 para 20 e ao mesmo tempo levou
    o `GND` de 0 para 5. O total melhorou 27; a placa perdeu cinco ligacoes de
    terra, que sao as que menos se podem perder.

    ⚠️ POR ISSO NAO SE CONTA UMA FORMA DE ESTRAGO. Conta-se o PERFIL: quantas
    ligacoes faltam em cada rede. E a regra de aceitacao e uma so —
    **nenhuma rede pode piorar**. Nao ha lista de sintomas a manter
    actualizada, e nao ha sintoma novo que passe por nao estar na lista."""
    subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--severity-error",
                    "--severity-warning", "--format", "json", "-o", "/tmp/_v.json",
                    caminho], capture_output=True)
    d = json.load(open("/tmp/_v.json"))
    # ⚠️ A ASSINATURA E (TIPO, REDE), E NAO A REDE. Subtrair a rede inteira da
    # linha de base e demasiado grosseiro: a base tinha uma via solta em AGND, e
    # com isso deixei passar uma folga de 0,193 mm numa pista de AGND, que e
    # outro defeito na mesma rede. Uma isencao vale para o defeito que se viu,
    # nunca para tudo o que partilhe o nome com ele.
    import collections
    maus = collections.Counter()
    for x in d.get("violations", []):
        if x["type"] not in ("clearance", "tracks_crossing", "shorting_items",
                             "copper_edge_clearance", "track_dangling",
                             "via_dangling", "copper_sliver",
                             "holes_co_located", "hole_clearance"):
            continue
        for i in x.get("items", []):
            m = re.search(r"(?:Track|Via) \[([^\]]+)\]", i.get("description", ""))
            if m:
                maus[(x["type"], m.group(1))] += 1
    import collections
    perfil = collections.Counter()
    for x in d.get("unconnected_items", []):
        redes = set()
        for i in x.get("items", []):
            m = re.search(r"\[([^\]]+)\]", i.get("description", ""))
            if m:
                redes.add(m.group(1))
        for r in redes:
            perfil[r] += 1
    return maus, perfil


def piorou(antes, depois):
    """Chaves que ficaram PIORES. A regra de aceitacao, e a unica."""
    return {k for k in set(antes) | set(depois) if depois[k] > antes[k]}


def redes(chaves):
    return sorted({k[1] if isinstance(k, tuple) else k for k in chaves})


def texto_com(segs, vias, base):
    L = ""
    for (x1, y1), (x2, y2), w, rede, cam in segs:
        L += ('\t(segment\n\t\t(start %g %g)\n\t\t(end %g %g)\n\t\t(width %g)\n'
              '\t\t(layer "%s")\n\t\t(net "%s")\n\t\t(uuid "%s")\n\t)\n'
              % (x1, y1, x2, y2, w, CAMADAS[cam], rede, uuid.uuid4()))
    for x, y, rede in vias:
        L += ('\t(via\n\t\t(at %g %g)\n\t\t(size 0.6)\n\t\t(drill 0.3)\n'
              '\t\t(layers "F.Cu" "B.Cu")\n\t\t(net "%s")\n\t\t(uuid "%s")\n\t)\n'
              % (x, y, rede, uuid.uuid4()))
    i = base.rstrip().rfind(")")
    return base[:i] + L + base[i:]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    p = sys.argv[1]
    escreve = "--escreve" in sys.argv
    so_uma = "--so-uma-face" in sys.argv
    so = sys.argv[sys.argv.index("--rede") + 1] if "--rede" in sys.argv else None
    t = io.open(p, encoding="utf-8").read()

    for z in re.finditer(r'\(zone[\s\S]{0,400}?\(layers? ("[^"]+"[^)]*)\)', t):
        if "F.Cu" in z.group(1):
            sys.exit("RECUSO: ha zona de cobre em F.Cu nesta placa.\n"
                     "        Ponha o plano so em B.Cu e costure os pads SMD com\n"
                     "        vias (pcb_contorno.costura) antes de correr. Ver 15cp.")

    print("roteia v%s — %s%s" % (VERSAO, os.path.basename(p),
                                 "" if escreve else "  (ensaio)"), file=sys.stderr)
    pads = pads_da_placa(t)
    pistas = pistas_da_placa(t)
    vias0 = vias_da_placa(t)

    subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--format", "json",
                    "-o", "/tmp/_r.json", p], capture_output=True)
    d = json.load(open("/tmp/_r.json"))
    faltam = []
    for x in d.get("unconnected_items", []):
        pts, ds = [], []
        for i in x.get("items", []):
            po = i.get("pos") or {}
            ds.append(i.get("description", ""))
            nm = re.search(r"\[([^\]]+)\]", ds[-1])
            if "x" in po:
                pts.append((po["x"], po["y"], nm.group(1) if nm else None))
        if len(pts) == 2 and pts[0][2] and not any("Zone [" in s for s in ds):
            faltam.append((pts[0][2], (pts[0][0], pts[0][1]), (pts[1][0], pts[1][1])))
    print("  %d ligacoes por rotear" % len(faltam), file=sys.stderr)

    # camadas alcancaveis em cada ponta: pelo pad mais proximo
    def camadas_em(q, rede):
        melhor, dm = None, 1e9
        for px, py, pr, pn, cams in pads:
            if pn != rede:
                continue
            dd = math.hypot(px - q[0], py - q[1])
            if dd < dm:
                melhor, dm = cams, dd
        return melhor if (melhor and dm < 0.6) else (0, 1)

    obst = [[], []]
    for px, py, pr, pn, cams in pads:
        for L in cams:
            obst[L].append((px, py, pr, pn))
    for x1, y1, x2, y2, n, L in pistas:
        amostra(obst[L], (x1, y1), (x2, y2), n, r=0.15)
    for x, y, n in vias0:
        obst[0].append((x, y, VIA_R, n))
        obst[1].append((x, y, VIA_R, n))

    bordo = [tuple(map(float, m.groups())) for m in
             re.finditer(r'\(gr_line\s*\(start (' + NUM + r') (' + NUM + r')\)\s*\(end ('
                         + NUM + r') (' + NUM + r')\)[\s\S]{0,120}?Edge\.Cuts', t)]
    bx = [v for b in bordo for v in (b[0], b[2])]
    by = [v for b in bordo for v in (b[1], b[3])]
    limites = (min(bx) + 0.6, min(by) + 0.6, max(bx) - 0.6, max(by) - 0.6)

    # ⚠️ A ORDEM MUDA O RESULTADO, num roteador guloso. Quem entra primeiro
    # escolhe o corredor; quem entra depois fica com o que sobrar. Escolhida por
    # medicao — ver o quadro no cabecalho.
    ordem = "longa"
    if "--ordem" in sys.argv:
        ordem = sys.argv[sys.argv.index("--ordem") + 1]
    if ordem == "longa":
        faltam.sort(key=lambda f: -math.hypot(f[2][0] - f[1][0], f[2][1] - f[1][1]))
    elif ordem == "curta":
        faltam.sort(key=lambda f: math.hypot(f[2][0] - f[1][0], f[2][1] - f[1][1]))

    novas, novas_vias, feitas, saltos = [], [], [], 0
    for rede, A, B in faltam:
        if so and rede != so:
            continue
        larg = LARGURA.get(rede, 0.25)
        gs = [Grelha(obst[0]), Grelha(obst[1])]
        cam = astar(A, B, camadas_em(A, rede), camadas_em(B, rede),
                    rede, larg, gs, limites, so_uma)
        if not cam or len(cam) < 2:
            continue
        usou_b = False
        for i in range(len(cam) - 1):
            (p1, L1), (p2, L2) = cam[i], cam[i + 1]
            if L1 != L2:
                # ⚠️ SEM REPETIR. Duas travessias no mesmo ponto dao dois furos
                # coincidentes, e a fabrica broca duas vezes o mesmo sitio:
                # `holes_co_located`. Uma via por ponto, e chega.
                q = (round(p1[0], 3), round(p1[1], 3))
                if any(abs(v[0] - q[0]) < 0.05 and abs(v[1] - q[1]) < 0.05
                       for v in novas_vias) or \
                   any(abs(v[0] - q[0]) < 0.05 and abs(v[1] - q[1]) < 0.05
                       for v in vias0):
                    saltos += 1
                    continue
                novas_vias.append((q[0], q[1], rede))
                obst[0].append((p1[0], p1[1], VIA_R, rede))
                obst[1].append((p1[0], p1[1], VIA_R, rede))
                saltos += 1
                continue
            if p1 == p2:
                continue
            novas.append((p1, p2, larg, rede, L1))
            amostra(obst[L1], p1, p2, rede, r=0.15)
            if L1 == 1:
                usou_b = True
        feitas.append((rede, usou_b))

    print("  %d roteadas · %d salto(s) de camada · %d ficam para a pessoa"
          % (len(feitas), saltos,
             len([f for f in faltam if not so or f[0] == so]) - len(feitas)),
          file=sys.stderr)

    if not escreve:
        print("  ensaio: nada gravado.", file=sys.stderr)
        return 0
    if not novas:
        print("  nada a gravar.", file=sys.stderr)
        return 0

    # ⚠️ LINHA DE BASE PRIMEIRO. A placa ja pode chegar com defeitos que nao sao
    # meus — havia um `via_dangling` em AGND vindo da costura — e sem os
    # subtrair a ferramenta culpa-se pelo estrago alheio, retira as suas pistas
    # todas e nao entrega nada. O que se mede e o DELTA, nunca o absoluto.
    maus0, perfil0 = estado(p)
    if maus0:
        print("  linha de base ja com defeito: %s (nao e meu, e nao o conto)"
              % " ".join("%s/%s" % k for k in sorted(maus0)), file=sys.stderr)

    io.open(p, "w", encoding="utf-8").write(texto_com(novas, novas_vias, t))
    maus, perfil = estado(p)
    maus = piorou(maus0, maus)
    pior = piorou(perfil0, perfil)
    if maus:
        antes = len(novas)
        rm = set(redes(maus))
        novas = [s for s in novas if s[3] not in rm]
        novas_vias = [v for v in novas_vias if v[2] not in rm]
        feitas = [f for f in feitas if f[0] not in rm]
        io.open(p, "w", encoding="utf-8").write(texto_com(novas, novas_vias, t))
        print("  ⚠️ %d segmento(s) retirados: %s violavam o DRC."
              % (antes - len(novas), " ".join(redes(maus))), file=sys.stderr)
        maus, perfil = estado(p)
        maus = piorou(maus0, maus)
        pior = piorou(perfil0, perfil)

    if pior:
        # ⚠️ BISSECCAO, e nao "tira todas as que passaram por B.Cu". A maioria
        # das travessias e inofensiva: o cobre flui a volta. So se procura a
        # que corta. Comeca-se por tirar TODAS e confirmar que zera — se nem
        # assim zerar, o plano ja estava partido antes de eu chegar, e nao ha
        # nada a retirar.
        print("  ⚠️ %s piorou. A procurar a travessia culpada."
              % " ".join(redes(pior)), file=sys.stderr)
        todas = sorted({r for r, ub in feitas if ub})

        def com(exclui):
            io.open(p, "w", encoding="utf-8").write(texto_com(
                [s for s in novas if s[3] not in exclui],
                [v for v in novas_vias if v[2] not in exclui], t))
            return piorou(perfil0, estado(p)[1])

        # ⚠️ FILTRAR AS CANDIDATAS POR GEOMETRIA ANTES DE GASTAR DRCs NELAS.
        # Cada teste da retraccao e um DRC completo com enchimento de zonas —
        # dezenas de segundos numa placa cheia — e eu testava as 33 redes que
        # tocaram em B.Cu quando so duas ou tres podiam ser culpadas. Uma
        # passada chegou a levar quinze minutos, quase toda a interrogar redes
        # inocentes.
        #
        # Uma travessia so pode orfanar um pad se passar tao perto dele que o
        # plano ja nao cabe entre os dois: e preciso caber folga + uma largura
        # minima de cobre + folga. Isso mede-se aqui, de graca.
        #
        # ⚠️ O DRC continua a ser quem decide. O filtro nao absolve ninguem —
        # so escolhe a quem vale a pena perguntar.
        COBRE_MIN = 0.25
        perto = FOLGA * 2 + COBRE_MIN
        redes_z = {z.group(1) for z in
                   re.finditer(r'\(zone[\s\S]{0,400}?\(net_name "([^"]*)"\)', t)
                   if z.group(1)}
        pads_z = [(px, py, pr) for px, py, pr, pn, cams in pads
                  if pn in redes_z and 1 in cams]
        susp = set()
        for (x1, y1), (x2, y2), _lw, rd, cm in novas:
            if cm != 1 or rd in susp:
                continue
            for px, py, pr in pads_z:
                if dist_ponto_seg(px, py, x1, y1, x2, y2) < pr + perto:
                    susp.add(rd)
                    break
        cands = sorted(set(todas) & susp)
        print("     %d de %d travessias passam perto de um pad do plano: so "
              "essas vale a pena interrogar" % (len(cands), len(todas)),
              file=sys.stderr)

        # ⚠️ E o atalho tem por onde voltar. Se tirar as suspeitas todas nao
        # chegar, volta-se a lista completa — um atalho que erra sem recuo e so
        # um erro mais rapido.
        if cands and com(set(cands)):
            print("     tirar so as suspeitas nao chegou: volto a lista toda.",
                  file=sys.stderr)
            cands = todas
        if not cands or com(set(cands)):
            io.open(p, "w", encoding="utf-8").write(texto_com(novas, novas_vias, t))
            print("     nem retirando todas as travessias isto zera: o estrago "
                  "ja la estava. Nao retiro nada.", file=sys.stderr)
        else:
            # ⚠️ VARRIMENTO, e nao bisseccao — medido, e ao contrario do que se
            # espera. Escrevi uma versao que partia ao meio, a pensar em
            # O(culpadas · log n), e ficou PIOR nos dois eixos: mais lenta (com
            # 5 culpadas em 33, descer nas metades que falham custa mais testes
            # que percorrer a lista) e com pior resultado — 25 ligacoes por
            # fazer contra 19 — porque o recuo que ela precisa para o caso das
            # culpadas-em-conjunto retira TODAS as candidatas de uma vez.
            # A bisseccao paga-se quando ha uma culpada. Aqui ha varias.
            manter = []
            for c in cands:                 # devolve-as uma a uma; fica o que couber
                if not com(set(cands) - set(manter) - {c}):
                    manter.append(c)
            fora = [c for c in cands if c not in manter]
            com(set(fora))
            novas = [s for s in novas if s[3] not in fora]
            novas_vias = [v for v in novas_vias if v[2] not in fora]
            feitas = [f for f in feitas if f[0] not in fora]
            print("     %d rede(s) devolvidas a pessoa: %s"
                  % (len(fora), " ".join(fora) or "—"), file=sys.stderr)

    maus, perfil = estado(p)
    maus = piorou(maus0, maus)
    pior = piorou(perfil0, perfil)
    if maus or pior:
        io.open(p, "w", encoding="utf-8").write(t)
        raise SystemExit("*** ainda viola (%s) ou piora (%s). Nao gravo nada."
                         % (" ".join(redes(maus)) or "—",
                            " ".join(redes(pior)) or "—"))
    print("  gravado: %d segmentos · %d vias · nenhuma rede piorou"
          % (len(novas), len(novas_vias)), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
