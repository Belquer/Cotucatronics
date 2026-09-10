#!/usr/bin/env python3
"""serigrafia_dfm v0.1.0 — poe a serigrafia da placa em conformidade com o DFM da JLC.

POR QUE EXISTE (licao de 2026-09-10, o segundo round).

Duas passagens deram a serigrafia por "limpa" (o DRC do KiCad SUB-CONTA-a) e a
JLCONE continuou a marcar dezenas de vermelhos no GERBER exportado. A verdade
esta no gerber (ver `mede_serigrafia.py`), e a correccao tem de ser NA FONTE
para o pacote nascer bom — nao um recorte no gerber que a proxima exportacao
desfaz. Este script faz, medindo contra o gerber a cada passo:

  1. LARGURA   sobe todo traco de serigrafia (texto E grafico) a 0,18 mm. A JLC
               marca a laranja o que fica RENTE a 0,15; 0,18 fica acima do minimo.
  2. CONTORNO  o texto de marca (ex.: COTUCATRONICS) que caiu por cima da fila de
               pads/vias/test-points muda-se para o vazio mais proximo (nao e um
               marcador de polaridade/pino — logo, move-se; nao ha embarque
               "cosmetico" para serigrafia).
  3. silk->PAD/FURO  recorta cada grafico de serigrafia (incl. discos CHEIOS,
               como o corpo do C3 electrolitico) a 0,15 mm de folga de todo pad
               (abertura de mascara) e furo. O `--subtract-soldermask` aparava-os
               RENTE (0 folga = vermelho na JLC); aqui fica a folga de verdade.
  4. TEXTO junto de pad/furo  move o designador o minimo para limpar, medindo o
               glifo no gerber (a forma do texto no pcbnew NAO bate com o plotado).
               `refs_apontam` continua a apontar cada referencia a sua peca.

⚠️ Precisa do pcbnew (Python do bundle KiCad) E de um python de sistema com
`gerbonara`+`shapely` (para a geometria de gerber). O caminho do segundo vem de
`SILK_SYS_PY` (default `python3`); o `kicad-cli` de `KICAD_CLI`.

⚠️ NAO recorta TEXTO (partiria os glifos) nem marcadores de pino/polaridade que
NAO cruzam pad — so o que entra na folga de um pad/furo e aparado/afastado.

USO:  serigrafia_dfm.py <placa.kicad_pcb>
"""
import sys, os, json, math, subprocess
import pcbnew as P
P.SwigPyIterator.next = P.SwigPyIterator.__next__

AQUI = os.path.dirname(os.path.abspath(__file__))
MEDE = os.path.join(AQUI, "mede_serigrafia.py")
SYS_PY = os.environ.get("SILK_SYS_PY", "python3")
KICAD = os.environ.get("KICAD_CLI", "kicad-cli")
LARG = 180000     # 0,18 mm alvo de largura de traco
SILK = {P.F_SilkS, P.B_SilkS}
F_S, B_S, F_C, B_C = P.F_SilkS, P.B_SilkS, P.F_Cu, P.B_Cu
LN = {F_S: 'F', B_S: 'B'}
mm = lambda v: v / 1e6
nm = lambda v: int(round(v * 1e6))
ENV = dict(os.environ)
for k in ("PYTHONHOME", "PYTHONPATH"):
    ENV.pop(k, None)


def _filled(it):
    try:
        return it.GetFillMode() == P.FILLED
    except Exception:
        return False


# ---------- fase 1: largura ----------
def widen(b):
    n = 0
    def bt(t):
        nonlocal n
        if t.GetTextThickness() < LARG:
            t.SetTextThickness(LARG); n += 1
    for fp in b.GetFootprints():
        for fld in (fp.Reference(), fp.Value()):
            if fld.GetLayer() in SILK:
                bt(fld)
        for it in fp.GraphicalItems():
            if it.GetLayer() not in SILK:
                continue
            if it.GetClass() == 'FP_TEXT':
                bt(it)
            elif not (it.GetShape() == P.SHAPE_T_POLY and _filled(it)) and 0 < it.GetWidth() < LARG:
                it.SetWidth(LARG); n += 1
    for it in b.GetDrawings():
        if it.GetLayer() not in SILK:
            continue
        if it.GetClass() == 'PCB_TEXT':
            bt(it)
        elif not (it.GetShape() == P.SHAPE_T_POLY and _filled(it)) and 0 < it.GetWidth() < LARG:
            it.SetWidth(LARG); n += 1
    return n


# ---------- fase 2: reposicionar texto de placa sobre pads/furos ----------
def relocate_board_text(b):
    def obstacles():
        out = []
        for fp in b.GetFootprints():
            for pad in fp.Pads():
                x = pad.GetBoundingBox(); out.append((x.GetLeft(), x.GetTop(), x.GetRight(), x.GetBottom()))
        for tr in b.GetTracks():
            if tr.GetClass() == 'PCB_VIA':
                c = tr.GetPosition(); r = int(tr.GetDrillValue() / 2)
                out.append((c.x - r, c.y - r, c.x + r, c.y + r))
        return out
    EX = b.GetBoardEdgesBoundingBox(); GAP, EDGE = 250000, 350000
    obs = obstacles()
    bt = [d for d in b.GetDrawings() if d.GetLayer() in SILK and d.GetClass() == 'PCB_TEXT']

    def others(cur):
        return [(d.GetBoundingBox().GetLeft(), d.GetBoundingBox().GetTop(),
                 d.GetBoundingBox().GetRight(), d.GetBoundingBox().GetBottom())
                for d in bt if d is not cur]

    def hits(l, t, r, btm, oth):
        if l - EDGE < EX.GetLeft() or r + EDGE > EX.GetRight() or \
           t - EDGE < EX.GetTop() or btm + EDGE > EX.GetBottom():
            return True
        for (ol, ot, orr, ob) in obs + oth:
            if r + GAP > ol and l - GAP < orr and btm + GAP > ot and t - GAP < ob:
                return True
        return False
    n = 0
    for d in bt:
        bb = d.GetBoundingBox(); w, h = bb.GetWidth(), bb.GetHeight()
        if not hits(bb.GetLeft(), bb.GetTop(), bb.GetRight(), bb.GetBottom(), others(d)):
            continue
        p0 = d.GetPosition(); best = None
        for rad in range(1, 400):
            R = rad * 250000
            for ang in range(0, 360, 10):
                cx = p0.x + int(R * math.cos(math.radians(ang)))
                cy = p0.y + int(R * math.sin(math.radians(ang)))
                if not hits(cx - w // 2, cy - h // 2, cx + w // 2, cy + h // 2, others(d)):
                    best = (cx, cy); break
            if best:
                break
        if best:
            d.SetPosition(P.VECTOR2I(*best)); n += 1
            print("   reposicionado '%s' -> (%.1f,%.1f)" % (d.GetText(), best[0] / 1e6, best[1] / 1e6))
    return n


# ---------- fase 3: recortar graficos de serigrafia a 0,15 mm de pad/furo ----------
def clip_graphics(b):
    def pad_polys(pad, cu):
        sp = P.SHAPE_POLY_SET(); pad.TransformShapeToPolygon(sp, cu, 0, 10000, P.ERROR_INSIDE)
        out = []
        for i in range(sp.OutlineCount()):
            ol = sp.Outline(i)
            pts = [(mm(ol.CPoint(j).x), -mm(ol.CPoint(j).y)) for j in range(ol.PointCount())]
            if len(pts) >= 3:
                out.append(pts)
        return out
    keep = {'F': [], 'B': []}; holes = []
    for fp in b.GetFootprints():
        for pad in fp.Pads():
            if pad.IsOnLayer(F_C): keep['F'] += pad_polys(pad, F_C)
            if pad.IsOnLayer(B_C): keep['B'] += pad_polys(pad, B_C)
            if pad.HasHole():
                ds = pad.GetDrillSize(); pos = pad.GetPosition()
                holes.append([mm(pos.x), -mm(pos.y), mm(ds.x), mm(ds.y), pad.GetOrientationDegrees()])
    for tr in b.GetTracks():
        if tr.GetClass() == 'PCB_VIA':
            pos = tr.GetPosition(); dd = mm(tr.GetDrillValue()); holes.append([mm(pos.x), -mm(pos.y), dd, dd, 0])

    def circle_pts(cx, cy, r, n=180):
        return [(cx + r * math.cos(2 * math.pi * k / n), cy + r * math.sin(2 * math.pi * k / n)) for k in range(n + 1)]

    def rings(it):
        sh = it.GetShape(); pt = lambda v: (mm(v.x), -mm(v.y))
        if sh == P.SHAPE_T_SEGMENT: return [[pt(it.GetStart()), pt(it.GetEnd())]], False
        if sh == P.SHAPE_T_CIRCLE:
            c = pt(it.GetCenter()); return [circle_pts(c[0], c[1], mm(it.GetRadius()))], _filled(it)
        if sh == P.SHAPE_T_ARC:
            c = pt(it.GetCenter()); s = pt(it.GetStart()); e = pt(it.GetEnd())
            r = math.hypot(s[0] - c[0], s[1] - c[1]); a1 = math.atan2(s[1] - c[1], s[0] - c[0]); a2 = math.atan2(e[1] - c[1], e[0] - c[0])
            if a2 < a1: a2 += 2 * math.pi
            n = max(8, int(abs(a2 - a1) / math.radians(4)))
            return [[(c[0] + r * math.cos(a1 + (a2 - a1) * k / n), c[1] + r * math.sin(a1 + (a2 - a1) * k / n)) for k in range(n + 1)]], False
        if sh == P.SHAPE_T_RECTANGLE:
            s = pt(it.GetStart()); e = pt(it.GetEnd()); xs = [s[0], e[0]]; ys = [s[1], e[1]]
            return [[(min(xs), min(ys)), (max(xs), min(ys)), (max(xs), max(ys)), (min(xs), max(ys)), (min(xs), min(ys))]], _filled(it)
        if sh == P.SHAPE_T_POLY:
            sp = it.GetPolyShape(); out = []
            for i in range(sp.OutlineCount()):
                ol = sp.Outline(i); pts = [(mm(ol.CPoint(j).x), -mm(ol.CPoint(j).y)) for j in range(ol.PointCount())]
                pts.append(pts[0]); out.append(pts)
            return out, _filled(it)
        return None, False
    items = []; refs = []
    def collect(container, kind):
        itr = container.GraphicalItems() if kind == 'fp' else container.GetDrawings()
        for it in itr:
            if it.GetLayer() not in SILK or it.GetClass() in ('FP_TEXT', 'PCB_TEXT'):
                continue
            rr, filled = rings(it)
            if not rr:
                continue
            items.append(dict(i=len(items), layer=LN[it.GetLayer()], w=mm(it.GetWidth() or LARG), filled=filled, rings=rr))
            refs.append(it)
    for fp in b.GetFootprints():
        collect(fp, 'fp')
    collect(b, 'board')
    gj = os.path.join(AQUI, "_silk_geom.json"); cj = os.path.join(AQUI, "_silk_clips.json")
    json.dump(dict(keep=keep, holes=holes, items=items), open(gj, 'w'))
    r = subprocess.run([SYS_PY, MEDE, "clip", gj, cj], capture_output=True, text=True, env=ENV)
    if not os.path.exists(cj):
        raise SystemExit("clip falhou: %s" % (r.stderr or r.stdout))
    clips = json.load(open(cj)); os.remove(gj); os.remove(cj)
    def add_poly(parent, lay, ext, hs):
        sp = P.SHAPE_POLY_SET(); o = sp.NewOutline()
        for x, y in (ext[:-1] if len(ext) > 1 and ext[0] == ext[-1] else ext):
            sp.Append(nm(x), nm(-y), o, -1)
        for hole in hs:
            hh = sp.NewHole(o)
            for x, y in (hole[:-1] if len(hole) > 1 and hole[0] == hole[-1] else hole):
                sp.Append(nm(x), nm(-y), o, hh)
        s = P.PCB_SHAPE(parent if parent is not b else None)
        s.SetShape(P.SHAPE_T_POLY); s.SetPolyShape(sp); s.SetLayer(lay); s.SetFilled(True); s.SetWidth(0)
        parent.Add(s)
    n = 0
    for k, polys in clips.items():
        it = refs[int(k)]; lay = it.GetLayer(); parent = it.GetParentFootprint() or b
        parent.Remove(it)
        for pl in polys:
            add_poly(parent, lay, pl['ext'], pl['holes'])
        n += 1
    return n


# ---------- fase 4: mover TEXTO de designador que fica junto de pad/furo ----------
def plot_measure(b):
    tg = os.path.join(AQUI, "_silk_tg"); os.makedirs(tg, exist_ok=True)
    for f in os.listdir(tg):
        os.remove(os.path.join(tg, f))
    tb = os.path.join(tg, "cotuca_painel.kicad_pcb"); b.Save(tb)
    subprocess.run([KICAD, "pcb", "export", "gerbers", "-o", tg + os.sep, "--layers",
                    "F.Cu,B.Cu,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts",
                    "--no-x2", "--subtract-soldermask", "--check-zones", tb], capture_output=True)
    subprocess.run([KICAD, "pcb", "export", "drill", "-o", tg + os.sep, "--format", "excellon",
                    "--drill-origin", "absolute", "--excellon-units", "mm",
                    "--excellon-zeros-format", "decimal", tb], capture_output=True)
    vj = os.path.join(AQUI, "_silk_v.json")
    if os.path.exists(vj):
        os.remove(vj)
    subprocess.run([SYS_PY, MEDE, os.path.join(tg, "cotuca_painel"), "--json=" + vj],
                   capture_output=True, env=ENV)
    return json.load(open(vj)) if os.path.exists(vj) else []


def fix_text(b):
    def texts():
        out = []
        for fp in b.GetFootprints():
            for fld in (fp.Reference(), fp.Value()):
                if fld.GetLayer() in SILK and fld.IsVisible():
                    out.append(fld)
            for it in fp.GraphicalItems():
                if it.GetLayer() in SILK and it.GetClass() == 'FP_TEXT' and it.IsVisible():
                    out.append(it)
        for it in b.GetDrawings():
            if it.GetLayer() in SILK and it.GetClass() == 'PCB_TEXT':
                out.append(it)
        return out

    def bbdist(bb, px, py):
        dx = max(bb.GetLeft() - nm(px), 0, nm(px) - bb.GetRight())
        dy = max(bb.GetTop() - nm(py), 0, nm(py) - bb.GetBottom())
        return math.hypot(dx, dy)
    for it in range(10):
        V = plot_measure(b)
        # so as violacoes que um TEXTO pode causar (pad/furo); largura ja tratada
        V = [v for v in V if v['kind'] in ('pad', 'furo')]
        if not V:
            print("   texto: limpo (%d iteracao(oes))" % it); return True
        ts = texts(); moves = {}
        for v in V:
            px, py = v['x'], -v['y']
            best = min(ts, key=lambda t: bbdist(t.GetBoundingBox(), px, py))
            need = (0.16 - v['clr']) + 0.03; mag = max(need, 0.10)
            if best.GetClass() == 'PCB_TEXT':
                ux, uy = 0.0, -1.0
            else:
                c = best.GetBoundingBox().GetCenter(); dx = mm(c.x) - px; dy = mm(c.y) - py
                dd = math.hypot(dx, dy) or 1; ux, uy = dx / dd, dy / dd
            k = id(best)
            if k not in moves or mag > moves[k][3]:
                moves[k] = (best, ux, uy, mag)
        for t, ux, uy, mag in moves.values():
            p = t.GetPosition(); t.SetPosition(P.VECTOR2I(p.x + nm(ux * mag), p.y + nm(uy * mag)))
    print("   texto: AVISO — nao limpou em 10 iteracoes"); return False


def main():
    if len(sys.argv) < 2:
        print(__doc__); return 2
    path = sys.argv[1]
    b = P.LoadBoard(path)
    print("serigrafia_dfm: %s" % path)
    print("  1. largura -> %d traco(s) subido(s) a 0,18 mm" % widen(b))
    print("  2. contorno de placa:")
    print("     %d texto(s) reposicionado(s)" % relocate_board_text(b))
    print("  3. recorte silk->pad/furo:")
    print("     %d grafico(s) recortado(s)" % clip_graphics(b))
    b.Save(path)
    print("  4. texto junto de pad/furo:")
    ok = fix_text(b)
    b.Save(path)
    # ⚠️ limpa os proprios ficheiros temporarios — se ficarem em kicad/, o
    # `carimbo()` do para_fabrica ve a arvore SUJA e o pacote deixa de ser limpo.
    import shutil
    shutil.rmtree(os.path.join(AQUI, "_silk_tg"), ignore_errors=True)
    for f in ("_silk_v.json", "_silk_geom.json", "_silk_clips.json"):
        try:
            os.remove(os.path.join(AQUI, f))
        except OSError:
            pass
    print("  gravado %s" % path)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
