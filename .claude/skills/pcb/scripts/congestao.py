#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""congestao.py — previsão de congestão pré-layout (Estágio 3.5 do /pcb)

    congestao.py <placa.kicad_pcb>   mede a COLOCAÇÃO; sai !=0 em VERMELHO
    congestao.py --sabota            prova que sabe dizer NÃO (não precisa de placa)

⚠️⚠️ O QUE ESTA FERRAMENTA EXISTE PARA APANHAR

Congestão descoberta a rotear é falha de ARQUITETURA tarde demais — o router não
a resolve, só a revela quando já custa refazer a colocação. Esta ferramenta olha
para a placa ANTES de tocar no cobre (footprints + netlist, zero pistas) e diz se
cada conector tem canais de escape para os sinais que a colocação lhe impõe, e se
algum courtyard já colide antes de existir uma única pista.

READ-ONLY: carrega a placa e mede; NUNCA grava. Não roteia, não move nada.

As DUAS RECUSAS DURAS (provadas na sessão ppc/002 rev C — ver
`references/congestao.md`), qualquer uma = VERMELHO = exit != 0:

  (a) conector de ≥2 filas onde **procura > oferta** de canais de escape
      (procura = redes não-plano distintas a sair; oferta = broadsides vivos ×
      colunas). Foi isto que previu J14/C16 no cérebro e J1/C5 no painel a partir
      só da colocação, antes de uma pista existir.
  (b) courtyard que sobrepõe **a borda da placa** (folga negativa) ou **o corpo
      (F.Fab/B.Fab) de outro componente do mesmo lado** — a âncora C16↔J14: um
      0805 fisicamente dentro do shroud de um box-header de assento rente. Mede-se
      por CORPO, não por courtyard nominal.

O motor numérico opera sobre um modelo puro (dicts), por isso o --sabota corre
sem placa e sem KiCad — planta defeitos e exige que ELES bloqueiem.
"""

VERSAO = "0.1.0"

import math
import os
import sys

# ─────────────────────────────────────────────────────────────────────────────
# NÚMEROS DO PROJECTO (carregados, não re-derivados — ver quality-standards.md §2)
TRACK = 0.25          # largura de pista default (mm)
CLR = 0.20            # clearance default (mm)
LANE = TRACK + CLR    # passo de canal em ar livre = 0.45 mm
MIN_OPEN = 3.0        # mm de placa aberta além do courtyard para um broadside "viver"
CELL = 5.0            # célula da grelha de densidade (mm)
PLANE_NETS = {"GND", "AGND"}   # vão ao plano B.Cu → não contam como escape
EPS = 0.05            # tolerância de sobreposição (mm) — abaixo disto é "encostar"


# ─────────────────────────────────────────────────────────────────────────────
# MOTOR PURO (opera sobre o modelo em dict; sem pcbnew — testável por --sabota)

def _clusters(vals, passo=0.6):
    """quantos grupos distintos há nos valores (fila/coluna de pinos)."""
    if not vals:
        return 0
    vals = sorted(vals)
    n = 1
    for a, b in zip(vals, vals[1:]):
        if abs(b - a) > passo:
            n += 1
    return n


def _grade(pads_pos):
    """(filas, colunas) a partir das posições dos pads."""
    nx = _clusters([p[0] for p in pads_pos])
    ny = _clusters([p[1] for p in pads_pos])
    return min(nx, ny), max(nx, ny)


def _overlap(a, b):
    """área de sobreposição de dois bboxes (x0,y0,x1,y1); 0 se só se encostam."""
    if not a or not b:
        return 0.0
    ox = min(a[2], b[2]) - max(a[0], b[0])
    oy = min(a[3], b[3]) - max(a[1], b[1])
    if ox > EPS and oy > EPS:
        return ox * oy
    return 0.0


def escape_veredicto(S, oferta):
    if oferta <= 0 or S > oferta:
        return "VERMELHO"
    if S > 0.8 * oferta:
        return "ÂMBAR"
    return "VERDE"


def analisa(modelo):
    """Corre as métricas sobre o modelo puro.

    Devolve (reds, linhas): reds = lista de (ref, classe, motivo) que bloqueiam;
    linhas = relatório legível (inclui âmbar e higiene, que NÃO bloqueiam).
    """
    X0, Y0, X1, Y1 = modelo["bbox"]
    reds, linhas = [], []

    # ---- 1. ORÇAMENTO DE CANAIS DE ESCAPE (a métrica que prevê J14/C16) ----
    linhas.append("--- 1. ORÇAMENTO DE CANAIS DE ESCAPE (conectores ≥2 filas) ---")
    for c in modelo["conectores"]:
        ref = c["ref"]
        filas, cols = _grade(c["pads_pos"])
        if filas < 2 or len(c["pads_pos"]) < 6:
            continue
        nets = c["nets"]
        S = len([n for n in nets if n not in PLANE_NETS])
        plane_pads = sum(v for n, v in nets.items() if n in PLANE_NETS)
        cb = c["crt"]
        if not cb:
            continue
        cx0, cy0, cx1, cy1 = cb
        horiz = (cx1 - cx0) > (cy1 - cy0)
        if horiz:
            lados = [("cima", cy0 - Y0), ("baixo", Y1 - cy1)]
        else:
            lados = [("esq", cx0 - X0), ("dir", X1 - cx1)]
        ncol = cols
        vivos = sum(1 for _, d in lados if d >= MIN_OPEN)
        mortos = ["%s=%.2f<%.1f" % (s, d, MIN_OPEN) for s, d in lados if d < MIN_OPEN]
        oferta = vivos * ncol
        v = escape_veredicto(S, oferta)
        linhas.append("  %-5s filas=%d cols=%d  redes=%d PLANO-pads=%d  PROCURA S=%d"
                      % (ref, filas, cols, len(nets), plane_pads, S))
        linhas.append("        %s  vivos=%d %s"
                      % (" / ".join("%s gap=%.2f" % (s, d) for s, d in lados),
                         vivos, ("MORTO: " + ",".join(mortos)) if mortos else ""))
        linhas.append("        OFERTA(canais)=%d  PROCURA=%d  -> %s" % (oferta, S, v))
        if v == "VERMELHO":
            reds.append((ref, "escape",
                         "procura %d > oferta %d (broadside morto: %s)"
                         % (S, oferta, ",".join(mortos) or "—")))

    # ---- 2. COURTYARD vs BORDA e vs CORPO DE OUTRO COMPONENTE ----
    linhas.append("--- 2. COURTYARD vs BORDA / CORPO (medido por corpo, não nominal) ---")
    comps = modelo["componentes"]
    n_edge = n_body = 0
    for a in comps:
        ca = a["crt"]
        if not ca:
            continue
        # (b1) courtyard além da borda da placa (folga negativa)
        fora = []
        if ca[0] < X0 - EPS:
            fora.append("esq %.2f" % (X0 - ca[0]))
        if ca[1] < Y0 - EPS:
            fora.append("cima %.2f" % (Y0 - ca[1]))
        if ca[2] > X1 + EPS:
            fora.append("dir %.2f" % (ca[2] - X1))
        if ca[3] > Y1 + EPS:
            fora.append("baixo %.2f" % (ca[3] - Y1))
        if fora:
            n_edge += 1
            reds.append((a["ref"], "borda",
                         "courtyard além da borda (%s mm)" % ", ".join(fora)))
        # (b2) courtyard de A sobre o corpo (fab) de B, do MESMO lado
        for b in comps:
            if b["ref"] == a["ref"] or b["side"] != a["side"]:
                continue
            ov = _overlap(ca, b["fab"])
            if ov > 0:
                n_body += 1
                reds.append((a["ref"], "corpo",
                             "courtyard invade o corpo de %s (%.2f mm² lado %s)"
                             % (b["ref"], ov, a["side"])))
    linhas.append("  cortyards fora da borda: %d · courtyard-sobre-corpo: %d"
                  % (n_edge, n_body // 1))

    # ---- 3. GRELHA DE DENSIDADE DE PINOS (5 mm) ----
    linhas.append("--- 3. DENSIDADE DE PINOS (grelha %.0f mm) ---" % CELL)
    nx = int(math.ceil((X1 - X0) / CELL)) or 1
    ny = int(math.ceil((Y1 - Y0) / CELL)) or 1
    grid = [[0] * nx for _ in range(ny)]
    for (px, py) in modelo["pads"]:
        ix = int((px - X0) // CELL)
        iy = int((py - Y0) // CELL)
        if 0 <= ix < nx and 0 <= iy < ny:
            grid[iy][ix] += 1
    cells = []
    for iy in range(ny):
        for ix in range(nx):
            if grid[iy][ix]:
                cells.append((grid[iy][ix], X0 + ix * CELL, Y0 + iy * CELL))
    cells.sort(reverse=True)
    if cells:
        dens = [c[0] for c in cells]
        linhas.append("  células com pads=%d  max=%d  média(ocupadas)=%.1f"
                      % (len(cells), dens[0], sum(dens) / len(dens)))
        for cnt, cx, cy in cells[:5]:
            linhas.append("   %3d pads na célula x[%.0f,%.0f] y[%.0f,%.0f]"
                          % (cnt, cx, cx + CELL, cy, cy + CELL))
    return reds, linhas


# ─────────────────────────────────────────────────────────────────────────────
# EXTRAÇÃO DA PLACA (única parte que fala com o pcbnew)

def _extrai(caminho):
    import pcbnew
    try:
        pcbnew.SwigPyIterator.next = pcbnew.SwigPyIterator.__next__
    except AttributeError:
        pass
    MM = 1e6
    b = pcbnew.LoadBoard(caminho)
    bb = b.GetBoardEdgesBoundingBox()
    bbox = (bb.GetLeft() / MM, bb.GetTop() / MM, bb.GetRight() / MM, bb.GetBottom() / MM)

    def _layer_bbox(fp, layers):
        xs, ys = [], []
        for it in fp.GraphicalItems():
            if it.GetLayer() in layers:
                gb = it.GetBoundingBox()
                xs += [gb.GetLeft() / MM, gb.GetRight() / MM]
                ys += [gb.GetTop() / MM, gb.GetBottom() / MM]
        return (min(xs), min(ys), max(xs), max(ys)) if xs else None

    conectores, componentes, pads_all = [], [], []
    for fp in b.GetFootprints():
        ref = fp.GetReference()
        side = "F" if fp.GetLayer() == pcbnew.F_Cu else "B"
        crt_layers = (pcbnew.F_CrtYd,) if side == "F" else (pcbnew.B_CrtYd,)
        fab_layers = (pcbnew.F_Fab,) if side == "F" else (pcbnew.B_Fab,)
        crt = _layer_bbox(fp, crt_layers)
        fab = _layer_bbox(fp, fab_layers)
        pads_pos, nets = [], {}
        for p in fp.Pads():
            pos = p.GetPosition()
            pads_pos.append((pos.x / MM, pos.y / MM))
            pads_all.append((pos.x / MM, pos.y / MM))
            n = p.GetNetname()
            if n:
                nets[n] = nets.get(n, 0) + 1
        componentes.append({"ref": ref, "side": side, "crt": crt, "fab": fab})
        if len(pads_pos) >= 6:
            conectores.append({"ref": ref, "pads_pos": pads_pos, "nets": nets, "crt": crt})
    return {"nome": os.path.basename(caminho), "bbox": bbox,
            "conectores": conectores, "componentes": componentes, "pads": pads_all}


# ─────────────────────────────────────────────────────────────────────────────
# AUTO-TESTE --sabota (modelos sintéticos; sem placa, sem KiCad)

def _modelo_limpo():
    """placa saudável: conector com folga nas duas filas, courtyards separados."""
    return {
        "nome": "LIMPO", "bbox": (0.0, 0.0, 100.0, 60.0),
        # conector 2x5 (10 pads) bem no interior, 6 sinais, dois broadsides vivos
        "conectores": [{
            "ref": "J9",
            "pads_pos": [(40 + i * 2.54, 28) for i in range(5)]
                        + [(40 + i * 2.54, 30.54) for i in range(5)],
            "nets": {"GND": 4, "A": 1, "B": 1, "C": 1, "D": 1, "E": 1, "F": 1},
            "crt": (39.0, 27.0, 51.16, 31.5),
        }],
        "componentes": [
            {"ref": "J9", "side": "F", "crt": (39.0, 27.0, 51.16, 31.5),
             "fab": (39.5, 27.5, 50.66, 31.0)},
            {"ref": "U9", "side": "F", "crt": (10.0, 10.0, 20.0, 20.0),
             "fab": (10.5, 10.5, 19.5, 19.5)},
            {"ref": "C9", "side": "F", "crt": (60.0, 40.0, 62.0, 42.0),
             "fab": (60.3, 40.3, 61.7, 41.7)},
        ],
        "pads": [(40 + i * 2.54, 28) for i in range(5)]
                + [(40 + i * 2.54, 30.54) for i in range(5)],
    }


def _modelo_mau():
    """três defeitos plantados, um por classe de recusa dura."""
    m = _modelo_limpo()
    # (a) conector 2x13 encostado ao bordo de cima: broadside de cima morto,
    #     17 sinais para 13 colunas de um só lado -> procura > oferta.
    pads = [(20 + i * 2.54, 1.0) for i in range(13)] \
        + [(20 + i * 2.54, 3.54) for i in range(13)]
    nets = {"GND": 8}
    nets.update({"S%02d" % i: 1 for i in range(17)})
    m["conectores"].append({
        "ref": "J14", "pads_pos": pads, "nets": nets,
        "crt": (18.5, 0.5, 53.5, 5.0),   # cima gap=0.5 < 3.0 -> morto
    })
    # (b1) courtyard além da borda direita
    m["componentes"].append({"ref": "J1", "side": "F",
                             "crt": (95.0, 20.0, 103.0, 26.0),
                             "fab": (95.5, 20.5, 102.5, 25.5)})
    # (b2) courtyard de C16 dentro do corpo de J14 (mesmo lado F) — a âncora
    m["componentes"].append({"ref": "J14", "side": "F",
                             "crt": (18.5, 0.5, 53.5, 5.0),
                             "fab": (19.0, 1.0, 53.0, 4.5)})
    m["componentes"].append({"ref": "C16", "side": "F",
                             "crt": (40.0, 1.5, 42.0, 3.5),
                             "fab": (40.3, 1.8, 41.7, 3.2)})
    return m


def sabota():
    print("congestao v%s — auto-teste --sabota" % VERSAO)
    print("  [limpo] espera 0 vermelhos:")
    reds_ok, linhas = analisa(_modelo_limpo())
    for _l in linhas:
        print("    " + _l)
    print("     -> %d vermelho(s)" % len(reds_ok))
    print("  [mau]   espera ≥3 vermelhos (escape, borda, corpo):")
    reds_bad, _ = analisa(_modelo_mau())
    for r in reds_bad:
        print("     ⛔ %-5s [%s] %s" % r)
    classes = {c for _, c, _ in reds_bad}
    tem_escape = "escape" in classes
    tem_borda = "borda" in classes
    tem_corpo = "corpo" in classes
    ok = (len(reds_ok) == 0 and len(reds_bad) >= 3
          and tem_escape and tem_borda and tem_corpo)
    print("     limpo=%d  mau=%d  escape=%s borda=%s corpo=%s  %s"
          % (len(reds_ok), len(reds_bad), tem_escape, tem_borda, tem_corpo,
             "✓ sabe dizer não" if ok else "*** NÃO REPROVOU"))
    return 0 if ok else 1


# ─────────────────────────────────────────────────────────────────────────────

def main(argv):
    print("congestao v%s" % VERSAO)
    if "--sabota" in argv:
        return sabota()
    args = [a for a in argv if not a.startswith("--")]
    if not args:
        raise SystemExit(__doc__)
    caminho = args[0]
    if not os.path.exists(caminho):
        raise SystemExit("*** não há %s" % caminho)
    modelo = _extrai(caminho)
    bx0, by0, bx1, by1 = modelo["bbox"]
    print("  placa %s  x[%.1f,%.1f] y[%.1f,%.1f]  %.0fx%.0f mm"
          % (modelo["nome"], bx0, bx1, by0, by1, bx1 - bx0, by1 - by0))
    reds, linhas = analisa(modelo)
    for l in linhas:
        print("  " + l)
    if reds:
        print("  %d REGIÃO(ÕES) VERMELHA(S) — a colocação NÃO passa o portão 3.5:" % len(reds))
        for ref, classe, motivo in reds:
            print("     ⛔ %-5s [%s] %s" % (ref, classe, motivo))
        raise SystemExit(
            "*** congestão: volta ao Estágio 3 (recolocar) — ou ao 1/2 se o que "
            "estrangula é o pinout do conector. NÃO se avança para o 4 com "
            "vermelho em aberto.")
    print("  ✓ colocação sem vermelhos — GO para o Estágio 4 (vias+pistas).")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
