#!/usr/bin/env python3
"""grid_grafismo — o MAPA para colocar grafismo por coordenada, nao por arrasto.

Referencia do metodo: `.claude/skills/pcb/references/grafismos-grid.md` (Estagio 6
da pipeline `pcb`). Este script automatiza o PASSO 2 do metodo: gerar um render
da face superior com grid de 0,5 mm, majors de 5 mm rotulados, eixos no frame de
coordenadas do KiCad, zonas "no-go" sombreadas (courtyards + pads + furos +
margem da borda >= 1,5 mm) e os maiores retangulos de espaco livre anotados com
as dimensoes em mm.

⚠️ ESTADO: v0.1.0, FUNCIONAL (nao e stub). Extrai a geometria com o `pcbnew` do
KiCad 10 e emite um SVG sem dependencias (sem matplotlib). Testado a carregar e a
desenhar o mapa das placas cerebro e painel (version 20260206).

⚠️ EXIGE O pcbnew DO KiCAD 10. O ficheiro e version 20260206; um `pcbnew` mais
antigo (ex.: 7.x do sistema) NAO carrega — o proprio KiCad recusa "created with a
more recent version". Corre com o python do AppImage KiCad 10, p.ex.:

    <squashfs-root>/bin/python3 kicad/grid_grafismo.py kicad/cotuca_painel/cotuca_painel.kicad_pcb

USO:  grid_grafismo.py <placa.kicad_pcb> [-o saida.svg] [--margem-mm 1.5]
                        [--passo-mm 0.5] [--n-livres 4]

O SVG e o MAPA do passo 2. A posicao aprova-se LENDO a coordenada do grid (passo
4) e transfere-se em UMA edicao so-silk (passo 5) — nunca se arrasta no viewer,
que nao recorta silk sobre pad e por isso ENGANA (ver a causa-raiz na referencia).
"""

VERSAO = "0.1.0"

import argparse
import os
import sys

try:
    import pcbnew
except Exception as e:  # pragma: no cover
    sys.stderr.write(
        "*** sem modulo pcbnew: %s\n"
        "    corre com o python do AppImage KiCad 10 (o ficheiro e version "
        "20260206; o pcbnew 7.x do sistema NAO o carrega).\n" % e)
    raise SystemExit(2)


def mm(v):
    """nanometros do KiCad -> mm."""
    return pcbnew.ToMM(v)


def bbox_mm(bb):
    """BOX2I do pcbnew -> (x0, y0, x1, y1) em mm (frame do KiCad, Y para baixo)."""
    return (mm(bb.GetLeft()), mm(bb.GetTop()),
            mm(bb.GetRight()), mm(bb.GetBottom()))


def obstaculos(board):
    """Retangulos no-go da face superior, em mm: pads(topo/TH) + furos + vias +
    courtyards(F.CrtYd). Cada um como (x0, y0, x1, y1). Aproximacao por bbox — e
    CONSERVADORA (bloqueia um pouco a mais, nunca a menos), que e o lado certo
    para uma zona de exclusao."""
    rets = []
    F_Cu = pcbnew.F_Cu
    F_CrtYd = pcbnew.F_CrtYd
    for fp in board.GetFootprints():
        # courtyard do topo
        try:
            cy = fp.GetCourtyard(F_CrtYd)
            if cy and cy.OutlineCount() > 0:
                rets.append(bbox_mm(cy.BBox()))
        except Exception:
            pass
        for pad in fp.Pads():
            ls = pad.GetLayerSet()
            th = pad.GetAttribute() in (pcbnew.PAD_ATTRIB_PTH,
                                        pcbnew.PAD_ATTRIB_NPTH)
            if th or ls.Contains(F_Cu):
                rets.append(bbox_mm(pad.GetBoundingBox()))
    return rets


def vias_do_ficheiro(caminho):
    """Vias lidas do TEXTO do .kicad_pcb — (x0,y0,x1,y1) em mm pelo diametro.

    ⚠️ Nao via pcbnew: o `board.GetTracks()` do binding KiCad 10 rebenta no
    python 3.14 (SwigPyIterator sem `.next`). O furo atravessa todas as faces e e
    no-go na face de topo; ler do texto e robusto e independente do binding."""
    import re
    t = open(caminho, encoding="utf-8").read()
    out = []
    for m in re.finditer(r"\(via\b", t):
        bloco = t[m.start():m.start() + 400]
        at = re.search(r"\(at\s+(-?[\d.]+)\s+(-?[\d.]+)", bloco)
        sz = re.search(r"\(size\s+([\d.]+)", bloco)
        if not at or not sz:
            continue
        x, y, s = float(at.group(1)), float(at.group(2)), float(sz.group(1))
        out.append((x - s / 2, y - s / 2, x + s / 2, y + s / 2))
    return out


def grelha_livre(x0, y0, x1, y1, rets, margem, passo):
    """Matriz de ocupacao no passo dado. Celula livre = fora da margem da borda E
    fora de todo o obstaculo. Devolve (cols, rows, livre[r][c])."""
    largura = x1 - x0
    altura = y1 - y0
    cols = max(1, int(round(largura / passo)))
    rows = max(1, int(round(altura / passo)))
    livre = [[True] * cols for _ in range(rows)]
    for r in range(rows):
        cy = y0 + (r + 0.5) * passo
        dentro_y = (cy - y0) >= margem and (y1 - cy) >= margem
        for c in range(cols):
            cx = x0 + (c + 0.5) * passo
            if not (dentro_y and (cx - x0) >= margem and (x1 - cx) >= margem):
                livre[r][c] = False
    for (ax0, ay0, ax1, ay1) in rets:
        c_ini = max(0, int((ax0 - x0) / passo))
        c_fim = min(cols - 1, int((ax1 - x0) / passo))
        r_ini = max(0, int((ay0 - y0) / passo))
        r_fim = min(rows - 1, int((ay1 - y0) / passo))
        for r in range(r_ini, r_fim + 1):
            row = livre[r]
            for c in range(c_ini, c_fim + 1):
                row[c] = False
    return cols, rows, livre


def maior_retangulo(livre, cols, rows):
    """Maior retangulo de celulas livres (histograma / DP). Devolve
    (r0, c0, r1, c1) em indices de celula, ou None."""
    alturas = [0] * cols
    melhor = None  # (area, r0, c0, r1, c1)
    for r in range(rows):
        linha = livre[r]
        for c in range(cols):
            alturas[c] = alturas[c] + 1 if linha[c] else 0
        pilha = []  # (col_inicio, altura)
        for c in range(cols + 1):
            h = alturas[c] if c < cols else 0
            inicio = c
            while pilha and pilha[-1][1] >= h:
                ini, ph = pilha.pop()
                area = ph * (c - ini)
                if melhor is None or area > melhor[0]:
                    melhor = (area, r - ph + 1, ini, r, c - 1)
                inicio = ini
            pilha.append((inicio, h))
    return None if melhor is None else melhor[1:]


def retangulos_livres(livre, cols, rows, n):
    """Os n maiores retangulos livres, nao sobrepostos (mascara o achado e
    repete). Devolve lista de (c0, r0, c1, r1) em indices."""
    import copy
    m = copy.deepcopy(livre)
    saida = []
    for _ in range(n):
        achado = maior_retangulo(m, cols, rows)
        if not achado:
            break
        r0, c0, r1, c1 = achado
        if (r1 - r0 + 1) * (c1 - c0 + 1) < 4:  # menor que 1 mm2 no passo 0,5: irrelevante
            break
        saida.append((c0, r0, c1, r1))
        for r in range(r0, r1 + 1):
            for c in range(c0, c1 + 1):
                m[r][c] = False
    return saida


def svg(caminho, x0, y0, x1, y1, rets, livres, cols, rows, passo, escala=6.0):
    largura = x1 - x0
    altura = y1 - y0
    pad = 14.0  # mm de moldura para rotulos
    W = (largura + 2 * pad) * escala
    H = (altura + 2 * pad) * escala

    def px(mx):
        return (mx - x0 + pad) * escala

    def py(my):
        return (my - y0 + pad) * escala

    L = ['<?xml version="1.0" encoding="UTF-8"?>',
         '<svg xmlns="http://www.w3.org/2000/svg" width="%.0f" height="%.0f" '
         'viewBox="0 0 %.0f %.0f" font-family="monospace">' % (W, H, W, H),
         '<rect x="0" y="0" width="%.0f" height="%.0f" fill="white"/>' % (W, H)]

    # grid 0,5 mm (minor) + majors 5 mm rotulados, no frame do KiCad
    import math
    vx0 = math.floor(x0 / passo) * passo
    vy0 = math.floor(y0 / passo) * passo
    x = vx0
    while x <= x1 + 1e-6:
        maior = abs((round(x / 5.0) * 5.0) - x) < 1e-6
        L.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" stroke="%s" '
                 'stroke-width="%.2f"/>'
                 % (px(x), py(y0), px(x), py(y1),
                    "#8899bb" if maior else "#e2e6ee", 0.8 if maior else 0.4))
        if maior:
            L.append('<text x="%.2f" y="%.2f" font-size="7" fill="#33415a" '
                     'text-anchor="middle">%g</text>' % (px(x), py(y0) - 3, x))
        x += passo
    y = vy0
    while y <= y1 + 1e-6:
        maior = abs((round(y / 5.0) * 5.0) - y) < 1e-6
        L.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" stroke="%s" '
                 'stroke-width="%.2f"/>'
                 % (px(x0), py(y), px(x1), py(y),
                    "#8899bb" if maior else "#e2e6ee", 0.8 if maior else 0.4))
        if maior:
            L.append('<text x="%.2f" y="%.2f" font-size="7" fill="#33415a" '
                     'text-anchor="end">%g</text>' % (px(x0) - 4, py(y) + 2, y))
        y += passo

    # eixos XY (frame KiCad: X ->, Y v para baixo)
    L.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" stroke="#c0392b" '
             'stroke-width="1.4"/>' % (px(x0), py(y0), px(x1), py(y0)))
    L.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" stroke="#c0392b" '
             'stroke-width="1.4"/>' % (px(x0), py(y0), px(x0), py(y1)))
    L.append('<text x="%.2f" y="%.2f" font-size="8" fill="#c0392b">+X (mm, KiCad)'
             '</text>' % (px(x1) - 60, py(y0) - 5))
    L.append('<text x="%.2f" y="%.2f" font-size="8" fill="#c0392b" transform='
             '"rotate(90 %.2f %.2f)">+Y (mm, KiCad)</text>'
             % (px(x0) - 6, py(y0) + 6, px(x0) - 6, py(y0) + 6))

    # borda da placa
    L.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="none" '
             'stroke="#111" stroke-width="1.2"/>'
             % (px(x0), py(y0), largura * escala, altura * escala))

    # zonas no-go sombreadas
    for (ax0, ay0, ax1, ay1) in rets:
        L.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" '
                 'fill="#e74c3c" fill-opacity="0.22" stroke="#e74c3c" '
                 'stroke-opacity="0.5" stroke-width="0.3"/>'
                 % (px(ax0), py(ay0), (ax1 - ax0) * escala, (ay1 - ay0) * escala))

    # retangulos de espaco livre, anotados em mm
    for i, (c0, r0, c1, r1) in enumerate(livres, 1):
        gx0 = x0 + c0 * passo
        gy0 = y0 + r0 * passo
        gx1 = x0 + (c1 + 1) * passo
        gy1 = y0 + (r1 + 1) * passo
        w = gx1 - gx0
        h = gy1 - gy0
        L.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" '
                 'fill="#2ecc71" fill-opacity="0.16" stroke="#1e8449" '
                 'stroke-width="1.0" stroke-dasharray="4 2"/>'
                 % (px(gx0), py(gy0), w * escala, h * escala))
        L.append('<text x="%.2f" y="%.2f" font-size="8" fill="#145a32" '
                 'text-anchor="middle">#%d  %.1f x %.1f mm</text>'
                 % (px((gx0 + gx1) / 2), py((gy0 + gy1) / 2), i, w, h))
        L.append('<text x="%.2f" y="%.2f" font-size="6" fill="#145a32" '
                 'text-anchor="middle">@ (%.1f, %.1f)</text>'
                 % (px((gx0 + gx1) / 2), py((gy0 + gy1) / 2) + 9, gx0, gy0))

    L.append('<text x="8" y="%.0f" font-size="8" fill="#555">grid_grafismo v%s · '
             'grid 0,5 mm · majors 5 mm · no-go = pads+furos+courtyards+margem · '
             'coordenadas no frame do KiCad</text>' % (H - 6, VERSAO))
    L.append('</svg>')
    open(caminho, "w", encoding="utf-8").write("\n".join(L) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("placa")
    ap.add_argument("-o", "--saida", default=None)
    ap.add_argument("--margem-mm", type=float, default=1.5)
    ap.add_argument("--passo-mm", type=float, default=0.5)
    ap.add_argument("--n-livres", type=int, default=4)
    a = ap.parse_args()

    if not os.path.exists(a.placa):
        raise SystemExit("*** nao existe: %s" % a.placa)
    board = pcbnew.LoadBoard(a.placa)
    x0, y0, x1, y1 = bbox_mm(board.GetBoardEdgesBoundingBox())
    rets = obstaculos(board) + vias_do_ficheiro(a.placa)
    cols, rows, livre = grelha_livre(x0, y0, x1, y1, rets, a.margem_mm, a.passo_mm)
    livres = retangulos_livres(livre, cols, rows, a.n_livres)

    saida = a.saida or (os.path.splitext(a.placa)[0] + "_grid_grafismo.svg")
    svg(saida, x0, y0, x1, y1, rets, livres, cols, rows, a.passo_mm)

    print("grid_grafismo v%s -> %s" % (VERSAO, saida))
    print("  placa %.1f x %.1f mm · %d obstaculo(s) no-go · passo %.2f mm · "
          "margem %.2f mm" % (x1 - x0, y1 - y0, len(rets), a.passo_mm, a.margem_mm))
    print("  maiores retangulos livres (anota a bbox do grafismo ANTES, passo 3):")
    for i, (c0, r0, c1, r1) in enumerate(livres, 1):
        gx0 = x0 + c0 * a.passo_mm
        gy0 = y0 + r0 * a.passo_mm
        w = (c1 - c0 + 1) * a.passo_mm
        h = (r1 - r0 + 1) * a.passo_mm
        print("    #%d  %.1f x %.1f mm  @ (%.1f, %.1f) no frame do KiCad"
              % (i, w, h, gx0, gy0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
