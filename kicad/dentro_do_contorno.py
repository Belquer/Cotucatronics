#!/usr/bin/env python3
"""dentro_do_contorno — o courtyard cabe na placa? (contornos, nao pads)

POR QUE EXISTE.

A 08/09 uma imagem do painel mostrou, em dez segundos, o `J1` com o courtyard
por cima do bordo e o `J2` encostado ao topo. O DRC dizia **geometria 0**, e
nao estava a mentir: **o KiCad nao confere courtyard contra `Edge.Cuts`**. Nao
ha regra dessas por omissao, portanto a pergunta nunca foi feita.

⚠️ E O PAD NAO SERVE DE MEDIDA. E a irmã do `confere_furos`: um conector com o
corpo a sair da placa tem os PADS todos la dentro — o que sai e o plastico. Um
verificador que meça pads da a placa por boa e o conector nao entra.

O QUE MEDE: para todo o footprint, cada vertice do courtyard (`F.CrtYd` ou
`B.CrtYd`) esta DENTRO do contorno e a >= FOLGA do bordo. E ao contrario
tambem: cada vertice do contorno a >= FOLGA do courtyard, senao um bordo que
entra por um recorte passa entre dois vertices sem tocar em nenhum.

⚠️ FOOTPRINT SEM COURTYARD E ACHADO, nao e passe. Um verificador que salte o
que nao sabe medir aprova tudo o que nao percebe (§2c).

USO:  dentro_do_contorno.py <cerebro|painel> [--folga MM] [--sabota REF]
"""
import os
import sys

VERSAO = "0.3.0"
FOLGA = 0.5             # mm entre o courtyard e o bordo da placa

# ⚠️ ISENCOES: com razao escrita, uma a uma, e NUNCA em bloco (skill kicad §7).
#
# Uma isencao aqui nao diz "ignora"; diz **o que foi medido em vez disso**. Se
# a razao nao contiver um numero, nao e razao — e vontade.
ISENCOES = {
    ("cerebro", "H1"): "furo M3 a 3,50 do bordo; a anilha real e ø6,5 (raio "
                       "3,25 < 3,50, sobra 0,25). O courtyard do footprint tem "
                       "raio 3,44 e e que passa 0,06 — a peca cabe, o desenho "
                       "e que e generoso. Posicao vem do acrilico e nao se move.",
    ("cerebro", "NT1"): "net-tie: nao tem corpo, logo nao tem courtyard. Nada "
                        "de fisico para caber ou nao caber.",
}
ISENCOES[("cerebro", "H2")] = ISENCOES[("cerebro", "H1")]
ISENCOES[("cerebro", "H3")] = ISENCOES[("cerebro", "H1")]
ISENCOES[("cerebro", "H4")] = ISENCOES[("cerebro", "H1")]
AQUI = os.path.dirname(os.path.abspath(__file__))


def _pcbnew():
    try:
        import pcbnew
        return pcbnew
    except ImportError:
        sys.exit("RECUSO: sem `pcbnew`. Corre com o Python do KiCad:\n"
                 "  /Applications/KiCad/KiCad.app/Contents/Frameworks/"
                 "Python.framework/Versions/3.9/bin/python3.9")


def _dist(cadeia, p):
    """Distancia em mm de `p` a ARESTA de `cadeia`, esteja `p` dentro ou fora.

    ⚠️ `SHAPE_LINE_CHAIN.Distance()` NAO serve: numa cadeia fechada ela mede a
    REGIAO, e devolve **zero para todo ponto interior**. A primeira versao
    disto usou-a e deu 74 achados em 74 footprints no cerebro e 28 em 28 no
    painel, todos a `0.000` — uma placa inteira reprovada por um leitor
    partido. Zeros identicos em cima de tudo sao leitor, nao sao placa.
    """
    q = cadeia.NearestPoint(p)
    return ((q.x - p.x) ** 2 + (q.y - p.y) ** 2) ** 0.5 / 1e6


def mede(caminho, folga=FOLGA, sabota=None, placa=None):
    """Devolve (achados, isentos, n_footprints). Achado = (ref, mm, razao)."""
    pcbnew = _pcbnew()
    b = pcbnew.LoadBoard(caminho)
    cont = pcbnew.SHAPE_POLY_SET()
    if not b.GetBoardPolygonOutlines(cont, False) or cont.OutlineCount() == 0:
        sys.exit("RECUSO: nao consegui ler o contorno de %s. Sem contorno nao"
                 " ha pergunta a fazer." % os.path.basename(caminho))
    borda = cont.COutline(0)

    fps = list(b.GetFootprints())
    achados = []
    for f in fps:
        ref = f.GetReference()
        cy = f.GetCourtyard(pcbnew.F_CrtYd)
        if cy.OutlineCount() == 0:
            cy = f.GetCourtyard(pcbnew.B_CrtYd)
        if cy.OutlineCount() == 0:
            achados.append((ref, None, "sem courtyard — nada a medir"))
            continue

        desloca = 0
        if sabota and ref == sabota:
            desloca = int(-25.0 * 1e6)     # 25 mm para cima: tem de sair de QUALQUER placa

        pior = None
        fora = False
        for i in range(cy.OutlineCount()):
            o = cy.Outline(i)
            for k in range(o.PointCount()):
                p = pcbnew.VECTOR2I(o.CPoint(k).x, o.CPoint(k).y + desloca)
                d = _dist(borda, p)
                if not cont.Contains(p):
                    fora = True
                    d = -d
                pior = d if pior is None else min(pior, d)
        # ⚠️ e ao contrario: um recorte do contorno pode entrar pelo meio de
        # duas arestas do courtyard sem chegar perto de nenhum vertice dele.
        cy0 = cy.COutline(0)
        for k in range(borda.PointCount()):
            p = borda.CPoint(k)
            if desloca:
                p = pcbnew.VECTOR2I(p.x, p.y - desloca)
            d = _dist(cy0, p)
            pior = d if pior is None else min(pior, d)

        if fora:
            achados.append((ref, pior, "FORA do contorno"))
        elif pior < folga:
            achados.append((ref, pior, "a %.3f do bordo (minimo %.2f)"
                            % (pior, folga)))
    # ⚠️ o isento SAI da lista de achados mas ENTRA numa lista propria. Uma
    # isencao que nao se ve e uma isencao que ninguem revoga.
    isentos = [(r, d, ISENCOES[(placa, r)]) for r, d, _ in achados
               if (placa, r) in ISENCOES]
    achados = [a for a in achados if (placa, a[0]) not in ISENCOES]
    return achados, isentos, len(fps)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    nome = sys.argv[1]
    folga = (float(sys.argv[sys.argv.index("--folga") + 1])
             if "--folga" in sys.argv else FOLGA)
    sabota = (sys.argv[sys.argv.index("--sabota") + 1]
              if "--sabota" in sys.argv else None)
    p = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_pcb" % nome)
    if not os.path.exists(p):
        sys.exit("RECUSO: nao existe %s" % p)

    print("dentro_do_contorno v%s — %s (folga %.2f mm)%s"
          % (VERSAO, nome, folga, "  SABOTADO %s" % sabota if sabota else ""))
    achados, isentos, n = mede(p, folga, sabota, nome)
    print("  %d footprint(s) medidos" % n)
    for ref, d, razao in isentos:
        print("  %-6s ISENTO (%s) — %s"
              % (ref, "sem courtyard" if d is None else "%.3f mm" % d, razao))
    if not achados:
        print("  0 achados ✓  todos os courtyards dentro do contorno")
        return 0
    for ref, d, razao in sorted(achados, key=lambda a: (a[1] is not None, a[1])):
        print("  %-6s %s" % (ref, razao if d is None
                             else "%-18s (%.3f mm)" % (razao, d)))
    print("  %d achado(s)" % len(achados))
    return 1


if __name__ == "__main__":
    sys.exit(main())
