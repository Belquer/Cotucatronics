#!/usr/bin/env python3
"""roteia v0.1.0 — as pistas FACEIS, e so essas.

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

VERSAO = "0.2.1"
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

    feitas, novas = [], []
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
                feitas.append((rede, A, B, len(caminho) - 1))
                break
    print("  %d faceis · %d ficam para a pessoa"
          % (len(feitas), len([f for f in faltam if not so or f[0] == so]) - len(feitas)),
          file=sys.stderr)
    por_rede = {}
    for rede, _, _, n in feitas:
        por_rede[rede] = por_rede.get(rede, 0) + 1
    for k in sorted(por_rede):
        print("     %-16s %d" % (k, por_rede[k]), file=sys.stderr)
    if escreve and novas:
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
