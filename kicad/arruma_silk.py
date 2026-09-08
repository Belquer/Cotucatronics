#!/usr/bin/env python3
"""arruma_silk v0.4.0 — tira os campos de referencia de cima dos pads.

⚠️ SERIGRAFIA SO. Nao toca em cobre, em pistas, em zonas nem em posicoes de
peca: mexe apenas no `(at ...)` do campo `Reference` de cada footprint, que e
texto. Uma placa routeada pode passar por aqui sem perder nada.

O defeito que arruma: numa grelha apertada o campo de referencia nasce a uma
distancia fixa da peca e vai parar em cima do pad da vizinha. A JLC recorta a
serigrafia sobre a mascara, entao o que se perde nao e funcao — e a legibilidade
na bancada, que e o que se paga quando se solda a mao e se depura.

Como escolhe: prova oito posicoes a volta da peca, por ordem de preferencia
(fora da placa primeiro, para o texto fugir do centro cheio), e fica na PRIMEIRA
que nao caia em cima de pad nenhum nem de outra referencia ja colocada.

⚠️ E SE NENHUMA SERVIR, DEIXA COMO ESTA e diz. Empurrar para um sitio mau nao e
arrumar — e trocar de sitio o mesmo problema, com a diferenca de eu ja nao estar
a olhar.

USO:  arruma_silk.py <ficheiro.kicad_pcb> [--escreve]
      sem --escreve, so diz o que faria.
"""
import io, math, os, re, sys

VERSAO = "0.4.0"
NUM = r"-?\d+(?:\.\d+)?"
RAIO_TXT = 1.6          # meia-altura tipica de um campo de referencia
PASSOS = [1.6, 2.6, 3.6, 4.8]


def blocos(t):
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
                    out.append((j, k + 1, t[j:k + 1]))
                    break
    return out


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    p = sys.argv[1]
    escreve = "--escreve" in sys.argv
    print("arruma_silk v%s — %s%s" % (VERSAO, os.path.basename(p),
                                      "" if escreve else "  (ensaio)"), file=sys.stderr)
    t = io.open(p, encoding="utf-8").read()

    # ⚠️ A SERIGRAFIA TAMBEM E OBSTACULO, e nao so o cobre.
    #
    # A primeira versao desviava a referencia dos PADS e pousava-a em cima do
    # contorno da propria peca. Das 18 `silk_overlap` do cerebro, quase todas
    # eram exactamente isso: `Reference field of D11` contra `Segment of D11`.
    #
    # ⚠️ E INCLUI O CONTORNO DA PROPRIA PECA, que e onde esta regra INVERTE em
    # relacao ao esquema. Num esquematico a referencia dentro do corpo do
    # proprio simbolo e onde ela mora (skill kicad §9b); numa placa e defeito,
    # porque as duas coisas saem na MESMA tinta branca e o texto deixa de se
    # ler. A mesma frase — "a referencia esta por cima do seu proprio corpo" —
    # e normal num sitio e defeito no outro.
    silk = []
    pistas = [tuple(map(float, m.groups())) for m in
              re.finditer(r"\(segment\s*\(start (" + NUM + r") (" + NUM
                          + r")\)\s*\(end (" + NUM + r") (" + NUM + r")\)", t)]
    pads, pecas = [], []
    for i, j, bl in blocos(t):
        a = re.search(r"\(at (" + NUM + r") (" + NUM + r")(?: (" + NUM + r"))?\)", bl)
        fx, fy = float(a.group(1)), float(a.group(2))
        ang = math.radians(float(a.group(3) or 0))
        r = re.search(r'\(property "Reference" "([^"]+)"', bl)
        ref = r.group(1) if r else "?"
        meus = []
        for pm in re.finditer(r'\(pad "[^"]*" \w+[\s\S]*?\(at (' + NUM + r") (" + NUM
                              + r")(?: " + NUM + r")?\)[\s\S]*?\(size (" + NUM + r") ("
                              + NUM + r")\)", bl):
            px, py, sw, sh = map(float, pm.groups())
            pads.append((fx + px * math.cos(ang) + py * math.sin(ang),
                         fy - px * math.sin(ang) + py * math.cos(ang),
                         max(sw, sh) / 2))
            meus.append(pads[-1])
        for sm in re.finditer(r'\(fp_(?:line|rect)\s*\(start (' + NUM + r') ('
                              + NUM + r')\)\s*\(end (' + NUM + r') (' + NUM
                              + r')\)[\s\S]{0,200}?\(layer "[FB]\.SilkS', bl):
            for px, py in ((float(sm.group(1)), float(sm.group(2))),
                           (float(sm.group(3)), float(sm.group(4))),
                           ((float(sm.group(1)) + float(sm.group(3))) / 2,
                            (float(sm.group(2)) + float(sm.group(4))) / 2)):
                silk.append((fx + px * math.cos(ang) + py * math.sin(ang),
                             fy - px * math.sin(ang) + py * math.cos(ang)))
        for sm in re.finditer(r'\(fp_circle\s*\(center (' + NUM + r') (' + NUM
                              + r')\)[\s\S]{0,200}?\(layer "[FB]\.SilkS', bl):
            px, py = float(sm.group(1)), float(sm.group(2))
            silk.append((fx + px * math.cos(ang) + py * math.sin(ang),
                         fy - px * math.sin(ang) + py * math.cos(ang)))
        pecas.append((i, j, bl, ref, fx, fy, ang, meus))

    # ⚠️ o centro da placa serve para decidir "para fora": o texto foge do
    # miolo, que e onde estao os vizinhos.
    cx = sum(p[0] for p in pads) / len(pads)
    cy = sum(p[1] for p in pads) / len(pads)

    # ⚠️ E O CONTORNO DA PLACA E O ULTIMO OBSTACULO, e faltava.
    #
    # Com pads, pistas e serigrafia nos obstaculos, o texto foge para fora — e
    # no painel o `silk_edge_clearance` subiu de 4 para 6. E a terceira
    # variante da mesma troca: cada obstaculo que se acrescenta empurra o texto
    # para o sitio que ainda nao esta a ser verificado. So para quando todos os
    # sitios maus estao na lista.
    bordo = [tuple(map(float, m.groups())) for m in
             re.finditer(r'\(gr_line\s*\(start (' + NUM + r') (' + NUM + r')\)\s*'
                         r'\(end (' + NUM + r') (' + NUM + r')\)[\s\S]{0,140}?Edge\.Cuts',
                         t)]
    if bordo:
        _bx = [v for b in bordo for v in (b[0], b[2])]
        _by = [v for b in bordo for v in (b[1], b[3])]
        LIM = (min(_bx) + RAIO_TXT, min(_by) + RAIO_TXT,
               max(_bx) - RAIO_TXT, max(_by) - RAIO_TXT)
    else:
        LIM = None

    def livre(x, y, ignora):
        if LIM and not (LIM[0] <= x <= LIM[2] and LIM[1] <= y <= LIM[3]):
            return False
        # ⚠️ A ISENCAO DOS PROPRIOS PADS CAIU, e a medicao e que decidiu.
        #
        # Ela existia porque a referencia por cima do proprio pad e o caso
        # normal de um SMD pequeno. Mas assim que a serigrafia entrou nos
        # obstaculos, as referencias fugiram do contorno e foram parar aos
        # proprios pads: `silk_overlap` caiu de 18 para 5 e `silk_over_copper`
        # SUBIU de 6 para 12. Trocar um defeito por outro nao e arrumar.
        #
        # Sem a isencao, quem nao tiver sitio fica onde esta e diz — que ja era
        # o comportamento previsto, e e melhor que ser empurrado para pior.
        for px, py, pr in pads:
            if abs(px - x) < pr + RAIO_TXT and abs(py - y) < pr + RAIO_TXT:
                return False
        # ⚠️ E AS PISTAS TAMBEM, que e a mesma regra do §11c: todo teste de
        # espaco olha pads E pistas. Depois de rotear, o cobre esta em todo o
        # lado, e um teste que so ve pads acha livre o que nao esta.
        for a1, b1, a2, b2 in pistas:
            dx, dy = a2 - a1, b2 - b1
            L2 = dx * dx + dy * dy
            u = 0.0 if L2 == 0 else max(0.0, min(1.0, ((x - a1) * dx + (y - b1) * dy) / L2))
            if math.hypot(x - (a1 + u * dx), y - (b1 + u * dy)) < RAIO_TXT * 0.8:
                return False
        for tx, ty in postos:
            if abs(tx - x) < 3.0 and abs(ty - y) < 2.0:
                return False
        # ⚠️ SEM `ignora` AQUI. Nos pads ignoram-se os da propria peca, porque a
        # referencia por cima do seu proprio pad e o caso normal de um SMD
        # pequeno. Na serigrafia nao ha excepcao: o contorno da propria peca
        # conta como qualquer outro, e era ele o culpado da maioria.
        for sx, sy in silk:
            if abs(sx - x) < RAIO_TXT and abs(sy - y) < RAIO_TXT * 0.7:
                return False
        return True

    postos, movidos, presos = [], [], []
    novo = t
    # ⚠️ de tras para a frente: cada substituicao muda os indices dos blocos
    # seguintes, e mexer do fim para o inicio nao os invalida.
    for i, j, bl, ref, fx, fy, ang, meus in sorted(pecas, key=lambda z: -z[0]):
        m = re.search(r'\(property "Reference" "[^"]+"\s*\n\s*\(at (' + NUM + r") ("
                      + NUM + r")( " + NUM + r")?\)", bl)
        if not m:
            continue
        dx, dy = float(m.group(1)), float(m.group(2))
        if livre(fx + dx, fy + dy, set(meus)):
            postos.append((fx + dx, fy + dy))
            continue
        fora_y = 1 if fy >= cy else -1
        fora_x = 1 if fx >= cx else -1
        cand = []
        for d in PASSOS:
            cand += [(0, d * fora_y), (0, -d * fora_y),
                     (d * fora_x, 0), (-d * fora_x, 0),
                     (d * fora_x, d * fora_y), (-d * fora_x, -d * fora_y)]
        for ndx, ndy in cand:
            if livre(fx + ndx, fy + ndy, set(meus)):
                a, b = m.span(1)[0], m.span(2)[1]
                novo = novo[:i + a] + ("%g %g" % (ndx, ndy)) + novo[i + b:]
                postos.append((fx + ndx, fy + ndy))
                movidos.append((ref, (dx, dy), (ndx, ndy)))
                break
        else:
            presos.append(ref)
            postos.append((fx + dx, fy + dy))

    print("  %d pecas · %d referencias movidas · %d sem sitio livre"
          % (len(pecas), len(movidos), len(presos)), file=sys.stderr)
    for ref, de, para in sorted(movidos):
        print("     %-5s (%+g,%+g) -> (%+g,%+g)" % (ref, de[0], de[1], para[0], para[1]),
              file=sys.stderr)
    if presos:
        print("  ⚠️ sem sitio: %s — ficam onde estavam." % " ".join(sorted(presos)),
              file=sys.stderr)
    if escreve:
        io.open(p, "w", encoding="utf-8").write(novo)
        print("  gravado.", file=sys.stderr)
    else:
        print("  ensaio: nada gravado. Junta --escreve para aplicar.", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
