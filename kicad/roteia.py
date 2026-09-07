#!/usr/bin/env python3
"""roteia — as pistas FACEIS, e so essas.

⚠️⚠️ NAO USAR EM PLACAS COM PLANO DE COBRE NA MESMA CAMADA DAS PISTAS. ⚠️⚠️

Este roteador nao ve zonas. Numa placa com plano de terra em F.Cu, cada pista
que o atravessa CORTA-O — o cobre recua dos dois lados e o plano fica em
pedacos. Medido no cerebro rev B: `AGND` passou de 1 ligacao por fazer para 8,
com o DRC a dizer GEOMETRIA 0 o tempo todo, porque do ponto de vista dele nada
estava errado. Cicatriz 15co.

Serve para placas de dois lados sem plano, ou para uma camada que nao tenha
zona. Ate saber ler zonas, mais nada.

⚠️ NAO E UM AUTOROTEADOR. Nao procura caminho: prova tres percursos triviais —
a direito, em L pelo horizontal, em L pelo vertical — e se nenhum estiver livre
DEIXA A LIGACAO PARA A PESSOA. Um roteador que insiste faz caminhos que ninguem
quereria e que depois ninguem percebe.

⚠️ VERIFICA-SE COM AS ZONAS CHEIAS. Sem isso o DRC compara as pistas novas com
um preenchimento VELHO e devolve dezenas de `clearance` contra zonas que nao
sao defeito nenhum:

    kicad-cli pcb drc --refill-zones ...

Escrevi num commit que o `kicad-cli` nao sabia encher zonas, e o `--help` que
corri a seguir desmentiu-me. A opcao esta la desde sempre.

⚠️ E TUDO O QUE ESCREVE E VERIFICAVEL PELO DRC. A regra e: se o numero de
ligacoes por ligar nao descer exactamente pelo numero de pistas que eu disse ter
feito, alguma coisa esta errada e nao se acredita no resto.

O que evita: pad de outra rede, pista de outra rede, e o furo de qualquer peca.
O que NAO evita: zonas (o cobre de GND esta por baixo e as pistas passam-lhe por
cima noutra camada ou com folga — o DRC diz se nao).

USO:  roteia.py <ficheiro.kicad_pcb> [--escreve] [--rede NOME]
"""
import io, json, math, os, re, subprocess, sys

VERSAO = "0.4.0"
NUM = r"-?\d+(?:\.\d+)?"
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
FOLGA = 0.35            # entre a pista e o que nao e dela
LARGURA = {"+15V": 1.0, "Net-(F1-Pad1)": 1.0, "V_TRILHO_A2": 1.0,
           "V+PRE": 0.5, "VBIAS": 0.5, "+5V": 0.5, "+5V_PONTE": 0.5, "+3V3A": 0.5}


def pads_da_placa(t):
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
        for pm in re.finditer(r'\(pad "[^"]*" \w+[\s\S]*?\(at (' + NUM + r") (" + NUM
                              + r")(?: " + NUM + r")?\)[\s\S]*?\(size (" + NUM + r") ("
                              + NUM + r")\)([\s\S]{0,300}?)\(uuid", bl):
            px, py, sw, sh, resto = pm.groups()
            px, py, sw, sh = float(px), float(py), float(sw), float(sh)
            n = re.search(r'\(net "([^"]+)"\)', resto)
            out.append((fx + px * math.cos(ang) + py * math.sin(ang),
                        fy - px * math.sin(ang) + py * math.cos(ang),
                        max(sw, sh) / 2, n.group(1) if n else None))
    return out


def cruzam(a, b, c, d):
    """Dois segmentos cruzam-se? ⚠️ Comparar EXTREMOS nao chega: duas pistas
    podem cruzar-se a meio sem que nenhuma ponta esteja perto da outra. Foi o
    que fiz na primeira versao e deu dez cruzamentos que o DRC apanhou."""
    def o(p, q, r):
        v = (q[1] - p[1]) * (r[0] - q[0]) - (q[0] - p[0]) * (r[1] - q[1])
        return 0 if abs(v) < 1e-9 else (1 if v > 0 else 2)
    o1, o2, o3, o4 = o(a, b, c), o(a, b, d), o(c, d, a), o(c, d, b)
    return o1 != o2 and o3 != o4


def dist_ponto_seg(px, py, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1
    if dx == dy == 0:
        return math.hypot(px - x1, py - y1)
    tt = max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - (x1 + tt * dx), py - (y1 + tt * dy))


# --- A*: contornar, que e o que falta as triviais ---------------------------
#
# ⚠️ ISTO NAO E DESVIO AUTOMATICO. O `shove` do KiCad EMPURRA o que ja la esta;
# isto so CONTORNA. Confirmado a correr o `--help` e a listar o modulo `pcbnew`:
# nao ha PNS nem Shove expostos em lado nenhum fora da janela.
#
# ⚠️ E PREFERE A LINHA RECTA. O custo penaliza cada mudanca de direccao, senao o
# A* devolve escadas de mil degraus que sao electricamente validas e humanamente
# ilegiveis — e uma placa que ninguem consegue ler nao se depura.
PASSO = 0.25            # mm por celula
VIRAGEM = 3.0           # o que custa dobrar, em celulas


def astar(A, B, rede, larg, obst, limites):
    import heapq
    x0, y0, x1, y1 = limites
    nx = int((x1 - x0) / PASSO) + 1
    ny = int((y1 - y0) / PASSO) + 1

    def cel(p):
        return (int(round((p[0] - x0) / PASSO)), int(round((p[1] - y0) / PASSO)))

    def pos(c):
        return (x0 + c[0] * PASSO, y0 + c[1] * PASSO)

    ini, fim = cel(A), cel(B)
    # ⚠️ MEIA DIAGONAL DE MARGEM A MAIS. O A* so testa CENTROS de celula, e o
    # segmento entre duas passa mais perto de um obstaculo do que qualquer das
    # pontas — ate meia diagonal. Sem isto saem folgas de 0,02 mm que o DRC
    # apanha e que ninguem consegue explicar olhando para o caminho.
    m = larg / 2 + FOLGA + PASSO * 0.71

    livre_cache = {}

    def ok(c):
        if c in livre_cache:
            return livre_cache[c]
        if not (0 <= c[0] < nx and 0 <= c[1] < ny):
            livre_cache[c] = False
            return False
        px, py = pos(c)
        v = True
        for ox, oy, orr, on in obst.perto(px, py):
            if on == rede:
                continue
            if math.hypot(px - ox, py - oy) < orr + m:
                v = False
                break
        livre_cache[c] = v
        return v

    DIRS = [(1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)]
    h = lambda c: (abs(c[0] - fim[0]) + abs(c[1] - fim[1])) * PASSO
    fila = [(h(ini), 0.0, ini, None)]
    veio, custo = {}, {ini: 0.0}
    limite = 0
    while fila:
        limite += 1
        if limite > 90000:          # ⚠️ teto: sem ele uma ligacao impossivel come tudo
            return None
        _, g, c, d = heapq.heappop(fila)
        if c == fim:
            cam, cur = [c], c
            while cur in veio:
                cur = veio[cur][0]
                cam.append(cur)
            cam.reverse()
            pts, ant = [pos(cam[0])], None
            for i in range(1, len(cam)):
                dd = (cam[i][0] - cam[i - 1][0], cam[i][1] - cam[i - 1][1])
                if ant and dd != ant:
                    pts.append(pos(cam[i - 1]))
                ant = dd
            pts.append(pos(cam[-1]))
            return pts
        for dd in DIRS:
            n = (c[0] + dd[0], c[1] + dd[1])
            if n != fim and not ok(n):
                continue
            passo = PASSO * (1.414 if dd[0] and dd[1] else 1.0)
            ng = g + passo + (VIRAGEM * PASSO if d and dd != d else 0.0)
            if ng < custo.get(n, 1e9):
                custo[n] = ng
                veio[n] = (c, dd)
                heapq.heappush(fila, (ng + h(n), ng, n, dd))
    return None


def amostra(lista, A, B, rede, passo=0.4):
    """Poe um segmento na lista de obstaculos, como uma fila de circulos."""
    n = max(2, int(math.hypot(B[0] - A[0], B[1] - A[1]) / passo))
    for i in range(n + 1):
        lista.append((A[0] + (B[0] - A[0]) * i / n,
                      A[1] + (B[1] - A[1]) * i / n, 0.18, rede))


class Grelha:
    """Indice espacial dos obstaculos, para o A* nao varrer a lista toda."""

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


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    p = sys.argv[1]
    escreve = "--escreve" in sys.argv
    so = None
    if "--rede" in sys.argv:
        so = sys.argv[sys.argv.index("--rede") + 1]
    print("roteia v%s — %s%s" % (VERSAO, os.path.basename(p),
                                 "" if escreve else "  (ensaio)"), file=sys.stderr)
    t = io.open(p, encoding="utf-8").read()
    t_placa = t
    pads = pads_da_placa(t)

    r = subprocess.run([KICAD, "pcb", "drc", "--format", "json", "-o", "/tmp/_r.json", p],
                       capture_output=True, text=True)
    d = json.load(open("/tmp/_r.json"))
    faltam = []
    for x in d.get("unconnected_items", []):
        pts = []
        for i in x.get("items", []):
            pos = i.get("pos") or {}
            nm = re.search(r"\[([^\]]+)\]", i.get("description", ""))
            if "x" in pos:
                pts.append((pos["x"], pos["y"], nm.group(1) if nm else None))
        if len(pts) == 2 and pts[0][2]:
            faltam.append((pts[0][2], (pts[0][0], pts[0][1]), (pts[1][0], pts[1][1])))
    print("  %d ligacoes por rotear" % len(faltam), file=sys.stderr)

    pistas = [tuple(map(float, m.groups()[:4])) + (m.group(5),)
              for m in re.finditer(r"\(segment\s*\(start (" + NUM + r") (" + NUM
                                   + r")\)\s*\(end (" + NUM + r") (" + NUM
                                   + r")\)[\s\S]*?\(net \"([^\"]+)\"\)", t)]

    def livre(x1, y1, x2, y2, rede, larg):
        m = larg / 2 + FOLGA
        for px, py, pr, pn in pads:
            if pn == rede:
                continue
            if dist_ponto_seg(px, py, x1, y1, x2, y2) < pr + m:
                return False
        for a, b, c, e, n in pistas:
            if n == rede:
                continue
            if cruzam((x1, y1), (x2, y2), (a, b), (c, e)):
                return False
            for qx, qy in ((a, b), (c, e)):
                if dist_ponto_seg(qx, qy, x1, y1, x2, y2) < m + 0.2:
                    return False
            for qx, qy in ((x1, y1), (x2, y2)):
                if dist_ponto_seg(qx, qy, a, b, c, e) < m + 0.2:
                    return False
        return True

    # obstaculos: pads e pistas, como circulos
    obst = [(px, py, pr, pn) for px, py, pr, pn in pads]
    for a, b, c, e, n in pistas:
        passos = max(2, int(math.hypot(c - a, e - b) / 0.5))
        for i in range(passos + 1):
            obst.append((a + (c - a) * i / passos, b + (e - b) * i / passos, 0.15, n))
    # ⚠️ os limites sao o CONTORNO, e nao a extensao das pecas. Com a segunda,
    # o A* saia da placa e deu 10 `copper_edge_clearance`.
    bordo = [tuple(map(float, m.groups())) for m in
             re.finditer(r'\(gr_line\s*\(start (' + NUM + r') (' + NUM + r')\)\s*\(end ('
                         + NUM + r') (' + NUM + r')\)[\s\S]{0,120}?Edge\.Cuts', t_placa)]
    if bordo:
        bx = [v for b in bordo for v in (b[0], b[2])]
        by = [v for b in bordo for v in (b[1], b[3])]
        limites = (min(bx) + 0.6, min(by) + 0.6, max(bx) - 0.6, max(by) - 0.6)
    else:
        xs = [o[0] for o in obst]; ys = [o[1] for o in obst]
        limites = (min(xs) - 1, min(ys) - 1, max(xs) + 1, max(ys) + 1)

    feitas, novas, curvas = [], [], 0
    for rede, A, B in faltam:
        if so and rede != so:
            continue
        larg = LARGURA.get(rede, 0.25)
        for caminho in ([A, B],
                        [A, (B[0], A[1]), B],
                        [A, (A[0], B[1]), B]):
            ok = all(livre(caminho[i][0], caminho[i][1], caminho[i + 1][0],
                           caminho[i + 1][1], rede, larg)
                     for i in range(len(caminho) - 1))
            if ok:
                for i in range(len(caminho) - 1):
                    novas.append((caminho[i], caminho[i + 1], larg, rede))
                    pistas.append((caminho[i][0], caminho[i][1],
                                   caminho[i + 1][0], caminho[i + 1][1], rede))
                    # ⚠️ TAMBEM nos obstaculos. Sem isto o A* seguinte nao ve as
                    # pistas que os percursos triviais deixaram, e atravessa-as:
                    # 81 cruzamentos, e nenhum era culpa do metodo.
                    amostra(obst, caminho[i], caminho[i + 1], rede)
                feitas.append((rede, A, B, len(caminho) - 1))
                break
        else:
            # ⚠️ so agora o A*, e nao antes: se a recta serve, a recta e melhor
            # que qualquer caminho que uma busca invente.
            g = Grelha(obst)
            cam = astar(A, B, rede, larg, g, limites)
            if cam and len(cam) >= 2:
                for i in range(len(cam) - 1):
                    novas.append((cam[i], cam[i + 1], larg, rede))
                    pistas.append((cam[i][0], cam[i][1], cam[i + 1][0], cam[i + 1][1], rede))
                    amostra(obst, cam[i], cam[i + 1], rede)
                feitas.append((rede, A, B, len(cam) - 1))
                curvas += 1
    print("  %d roteadas (%d por contorno) · %d ficam para a pessoa"
          % (len(feitas), curvas,
             len([f for f in faltam if not so or f[0] == so]) - len(feitas)),
          file=sys.stderr)
    por_rede = {}
    for rede, _, _, n in feitas:
        por_rede[rede] = por_rede.get(rede, 0) + 1
    for k in sorted(por_rede):
        print("     %-16s %d" % (k, por_rede[k]), file=sys.stderr)
    def escrever(segs, texto):
        import uuid
        L = ""
        for (x1, y1), (x2, y2), w, rede in segs:
            L += ('\t(segment\n\t\t(start %g %g)\n\t\t(end %g %g)\n\t\t(width %g)\n'
                  '\t\t(layer "F.Cu")\n\t\t(net "%s")\n\t\t(uuid "%s")\n\t)\n'
                  % (x1, y1, x2, y2, w, rede, uuid.uuid4()))
        i = texto.rstrip().rfind(")")
        return texto[:i] + L + texto[i:]

    def viola(caminho):
        """Redes cujas pistas o DRC acusa. ⚠️ Com as zonas cheias, senao compara
        com um preenchimento velho e acusa tudo."""
        subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--severity-error",
                        "--severity-warning", "--format", "json", "-o", "/tmp/_v.json",
                        caminho], capture_output=True)
        d = json.load(open("/tmp/_v.json"))
        mas = set()
        for x in d.get("violations", []):
            if x["type"] not in ("clearance", "tracks_crossing", "shorting_items",
                                 "copper_edge_clearance", "track_dangling"):
                continue
            for i in x.get("items", []):
                m = re.search(r"Track \[([^\]]+)\]", i.get("description", ""))
                if m:
                    mas.add(m.group(1))
        return mas

    if escreve and novas:
        # ⚠️ A FERRAMENTA CONFERE O PROPRIO TRABALHO. Escreve, corre o DRC, e as
        # redes cujas pistas violarem alguma coisa sao RETIRADAS — voltam para a
        # lista da pessoa. Duas pistas ficaram a 0,13 mm de um pad por uma razao
        # que eu nao consegui isolar em tempo util; a resposta certa nao e
        # persegui-la ate tarde, e a ferramenta nao entregar o que nao consegue
        # provar. Entregar cobre que o DRC acusa e pior que nao entregar nada.
        io.open(p, "w", encoding="utf-8").write(escrever(novas, t))
        maus = viola(p)
        if maus:
            antes = len(novas)
            novas = [s for s in novas if s[3] not in maus]
            feitas = [f for f in feitas if f[0] not in maus]
            io.open(p, "w", encoding="utf-8").write(escrever(novas, t))
            print("  ⚠️ %d segmento(s) retirados: as redes %s violavam o DRC e "
                  "voltam para a lista." % (antes - len(novas), " ".join(sorted(maus))),
                  file=sys.stderr)
            resta = viola(p)
            if resta:
                io.open(p, "w", encoding="utf-8").write(t)
                raise SystemExit("*** ainda viola depois de retirar (%s). Nao gravo "
                                 "nada." % " ".join(sorted(resta)))
        import uuid
        L = ""
        for (x1, y1), (x2, y2), w, rede in novas:
            L += ('\t(segment\n\t\t(start %g %g)\n\t\t(end %g %g)\n\t\t(width %g)\n'
                  '\t\t(layer "F.Cu")\n\t\t(net "%s")\n\t\t(uuid "%s")\n\t)\n'
                  % (x1, y1, x2, y2, w, rede, uuid.uuid4()))
        i = t.rstrip().rfind(")")
        io.open(p, "w", encoding="utf-8").write(t[:i] + L + t[i:])
        print("  gravado: %d segmentos" % len(novas), file=sys.stderr)
    elif not escreve:
        print("  ensaio: nada gravado.", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
