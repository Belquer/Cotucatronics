#!/usr/bin/env python3
"""mede_serigrafia v0.1.0 — a serigrafia medida NO GERBER, como a JLC a ve.

POR QUE EXISTE.

O DRC do KiCad SUB-CONTA a serigrafia contra a JLC (licao de 2026-09-10):
mede silk->pad com folga 0, nao ve a folga de 0,15 mm exigida, e as regras
`.kicad_dru` de `silk_clearance` nem sequer dispararam pelo `kicad-cli pcb drc`.
Duas passagens deram "limpo" e a JLCONE continuou a mostrar dezenas de vermelhos.

⚠️ A UNICA MEDICAO FIEL E O GERBER EXPORTADO — e o artefacto que a JLC ingere,
JA com o `--subtract-soldermask` aplicado (que apara a serigrafia RENTE ao pad e
cria a borda que a JLC conta como vermelho). Este modulo mede o gerber:

  silk->pad     folga entre a serigrafia (F/B.Silkscreen) e a abertura de
                mascara (F/B.Mask, = o pad exposto). < 0,15 mm ou sobreposto.
  silk->furo    folga entre a serigrafia e o FURO (o .drl, incl. slots G85 e
                vias). A JLC mede contra o furo, nao so contra o cobre.
  largura       largura de CADA traco de serigrafia (a abertura usada nos D01).
                Texto E grafico. A JLC marca a laranja tudo <= ao minimo; o alvo
                da fonte e 0,18 mm (acima do minimo 0,15, para nao ficar rente).

Sem pcbnew: le so os ficheiros do pacote (gerbonara + shapely). Precisa das
duas bibliotecas (`pip install gerbonara shapely`); um portao que nao consegue
medir RECUSA — nao passa em silencio.

USO (portao):   mede_serigrafia.py <dir> [--min-largura 0.15]
  <dir> e a pasta do pacote (contem cotuca_<placa>-F_Silkscreen.gto etc).
  Imprime silk->pad / silk->furo / largura e sai !=0 se houver QUALQUER violacao.

USO (clip, para o serigrafia_dfm.py):  mede_serigrafia.py clip <geom.json> <out.json>
"""
import sys, os, re, math, json, glob

MIN = 0.15          # folga JLC silk->pad / silk->furo (mm)
MIN_LARGURA = 0.15  # largura minima de traco JLC (mm); < isto = vermelho


def _need_libs():
    try:
        import gerbonara, shapely  # noqa
    except Exception as e:
        sys.stderr.write("mede_serigrafia: falta biblioteca (%s). "
                         "Corre: pip install gerbonara shapely\n" % e)
        raise SystemExit(3)


# ---------- geometria de gerber -> shapely ----------
def _flatten(ap):
    pts = []
    for p1, p2, arc in ap.segments:
        p1 = (float(p1[0]), float(p1[1])); p2 = (float(p2[0]), float(p2[1]))
        if arc is None or arc[0] is None:
            pts.append(p1)
        else:
            cw, c = arc; cx, cy = float(c[0]), float(c[1])
            a1 = math.atan2(p1[1] - cy, p1[0] - cx)
            a2 = math.atan2(p2[1] - cy, p2[0] - cx)
            r = math.hypot(p1[0] - cx, p1[1] - cy)
            if cw:
                if a2 >= a1: a2 -= 2 * math.pi
            else:
                if a2 <= a1: a2 += 2 * math.pi
            n = max(2, int(abs(a2 - a1) / math.radians(5)))
            for k in range(n):
                a = a1 + (a2 - a1) * k / n
                pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return pts


def _poly(pr):
    from shapely.geometry import Polygon
    pts = _flatten(pr.to_arc_poly())
    if len(pts) < 3:
        return None
    p = Polygon(pts)
    if not p.is_valid:
        p = p.buffer(0)
    return p if (not p.is_empty and p.area > 1e-9) else None


def _gerber_geom(path):
    from gerbonara import GerberFile
    from shapely.geometry import Polygon
    from shapely.ops import unary_union
    gf = GerberFile.open(path)
    dark, clear = [], []
    for o in gf.objects:
        for pr in o.to_primitives('mm'):
            pp = _poly(pr)
            if pp is None:
                continue
            (dark if pr.polarity_dark else clear).append(pp)
    g = unary_union(dark) if dark else Polygon()
    if clear:
        g = g.difference(unary_union(clear))
    return g


def _mask_flashes(path):
    from gerbonara import GerberFile
    from gerbonara.graphic_objects import Flash
    from shapely.ops import unary_union
    gf = GerberFile.open(path)
    out = []
    for o in gf.objects:
        if isinstance(o, Flash) and o.polarity_dark:
            ps = [_poly(pr) for pr in o.to_primitives('mm')]
            ps = [p for p in ps if p]
            if ps:
                out.append(unary_union(ps))
    return out


def _silk_apertures(path):
    """larguras (mm) das aberturas C usadas em desenhos D01 (tracos)."""
    s = open(path).read()
    aps = {int(m.group(1)): float(m.group(2))
           for m in re.finditer(r'%ADD(\d+)C,([0-9.]+)\*%', s)}
    used, cur = set(), None
    for tok in s.split('*'):
        tok = tok.strip()
        dm = re.fullmatch(r'D(\d+)', tok)
        if dm and int(dm.group(1)) >= 10:
            cur = int(dm.group(1))
        elif tok.endswith('D01') and cur in aps:
            used.add(cur)
    # aberturas de largura 0 sao o CONTORNO de regioes cheias (fill), nao tracos:
    # a JLC nao as conta como linha fina. So contam os tracos reais (largura > 0).
    return sorted(aps[a] for a in used if aps[a] > 1e-4)


def _drill_holes(drl):
    """poligonos de furo do .drl (circulos + slots G85), coords Y do gerber (-y)."""
    from shapely.geometry import Point, LineString
    if not os.path.exists(drl):
        return []
    tools = {}
    cur = None
    holes = []
    for ln in open(drl):
        ln = ln.strip()
        m = re.match(r'T(\d+)C([0-9.]+)', ln)
        if m:
            tools[int(m.group(1))] = float(m.group(2)); continue
        m = re.match(r'T(\d+)\s*$', ln)
        if m:
            cur = int(m.group(1)); continue
        # slot: X..Y..G85X..Y..
        m = re.match(r'X([\-0-9.]+)Y([\-0-9.]+)G85X([\-0-9.]+)Y([\-0-9.]+)', ln)
        if m and cur in tools:
            x1, y1, x2, y2 = map(float, m.groups()); r = tools[cur] / 2
            holes.append(LineString([(x1, -y1), (x2, -y2)]).buffer(r, 24)); continue
        m = re.match(r'X([\-0-9.]+)Y([\-0-9.]+)\s*$', ln)
        if m and cur in tools:
            x, y = float(m.group(1)), float(m.group(2)); r = tools[cur] / 2
            holes.append(Point(x, -y).buffer(r, 48))
    return holes


def medir(prefix):
    """(violacoes, resumo) para o pacote em <prefix>-*.  prefix sem sufixo."""
    _need_libs()
    res = {}
    viol = []
    for side, silkext, mext in (("F", "gto", "gts"), ("B", "gbo", "gbs")):
        silkf = "%s-%s_Silkscreen.%s" % (prefix, side, silkext)
        maskf = "%s-%s_Mask.%s" % (prefix, side, mext)
        if not os.path.exists(silkf):
            continue
        silk = _gerber_geom(silkf)
        pads = _mask_flashes(maskf) if os.path.exists(maskf) else []
        drl = glob.glob(os.path.dirname(prefix) + os.sep + "*.drl")
        holes = _drill_holes(drl[0]) if drl else []
        pr = po = hr = ho = 0
        for t in pads:
            if silk.intersects(t):
                it = silk.intersection(t)
                if it.area > 1e-7 or it.length > 0.02:
                    pr += 1; viol.append((side, "pad", t.centroid.x, t.centroid.y, 0.0)); continue
            d = silk.distance(t)
            if d < MIN:
                po += 1; viol.append((side, "pad", t.centroid.x, t.centroid.y, d))
        for t in holes:
            if silk.intersects(t):
                it = silk.intersection(t)
                if it.area > 1e-7 or it.length > 0.02:
                    hr += 1; viol.append((side, "furo", t.centroid.x, t.centroid.y, 0.0)); continue
            d = silk.distance(t)
            if d < MIN:
                ho += 1; viol.append((side, "furo", t.centroid.x, t.centroid.y, d))
        larg = _silk_apertures(silkf)
        finos = [w for w in larg if w < MIN_LARGURA - 1e-9]
        res[side] = dict(pad_red=pr, pad_org=po, hole_red=hr, hole_org=ho,
                         min_larg=(min(larg) if larg else None), finos=finos)
        for w in finos:
            viol.append((side, "largura", None, None, w))
    return viol, res


def _portao(argv):
    pos = [a for a in argv if not a.startswith("--")]
    if not pos:
        print(__doc__); return 2
    prefix = pos[0]
    if os.path.isdir(prefix):
        g = glob.glob(os.path.join(prefix, "*-F_Silkscreen.gto")) or \
            glob.glob(os.path.join(prefix, "*-B_Silkscreen.gbo"))
        if not g:
            print("mede_serigrafia: sem gerber de serigrafia em %s" % prefix); return 2
        prefix = re.sub(r'-[FB]_Silkscreen\.(gto|gbo)$', '', g[0])
    viol, res = medir(prefix)
    jout = None
    for a in argv:
        if a.startswith("--json="):
            jout = a.split("=", 1)[1]
    if jout:
        json.dump([dict(side=s, kind=k, x=x, y=y, clr=d) for s, k, x, y, d in viol],
                  open(jout, "w"))
    for side, r in res.items():
        print("   %s  silk->pad %d red/%d org · silk->furo %d red/%d org · traco min %s mm%s"
              % (side, r['pad_red'], r['pad_org'], r['hole_red'], r['hole_org'],
                 ("%.3f" % r['min_larg']) if r['min_larg'] else "-",
                 ("  ⚠️ %d traco(s) <%.2f" % (len(r['finos']), MIN_LARGURA)) if r['finos'] else ""))
    bad = [v for v in viol]
    if bad:
        print("   RECUSO: %d violacao(oes) de serigrafia no GERBER:" % len(bad))
        for side, kind, x, y, d in bad[:40]:
            loc = ("@(%.2f,%.2f)" % (x, y)) if x is not None else ""
            print("     %s %-7s %s clr/larg=%.3f" % (side, kind, loc, d))
        return 1
    print("   serigrafia GERBER limpa ✓ (silk->pad/furo 0, traco todo >= %.2f mm)" % MIN_LARGURA)
    return 0


# ---------- clip (chamado pelo serigrafia_dfm.py, que tem o pcbnew) ----------
def _clip(geomjson, outjson):
    _need_libs()
    from shapely.geometry import LineString, Polygon, Point, box
    from shapely.ops import unary_union
    from shapely.affinity import rotate as shrot
    CLIP = 0.20  # apara a 0,20 mm para a folga MEDIDA ficar >= 0,15 apos facetagem
    d = json.load(open(geomjson))

    def mkpoly(pts):
        p = Polygon(pts)
        return p if p.is_valid else p.buffer(0)
    holes = []
    for cx, cy, w, h, rot in d['holes']:
        if abs(w - h) < 1e-6:
            holes.append(Point(cx, cy).buffer(w / 2, 64))
        else:
            r = min(w, h) / 2
            g = box(cx - w / 2 + r, cy - h / 2, cx + w / 2 - r, cy + h / 2).buffer(r, 24)
            holes.append(shrot(g, rot, origin=(cx, cy)))
    keepU = {}
    for s in ('F', 'B'):
        polys = [mkpoly(p) for p in d['keep'][s] if len(p) >= 3]
        keepU[s] = unary_union(polys + holes).buffer(CLIP)
    out = {}
    for it in d['items']:
        idx, s, w, filled = it['i'], it['layer'], it['w'], it['filled']
        parts = []
        for ring in it['rings']:
            if len(ring) < 2:
                continue
            if filled and len(ring) >= 3:
                parts.append(mkpoly(ring).buffer(w / 2, join_style=1))
            else:
                parts.append(LineString(ring).buffer(w / 2, cap_style=1, join_style=1))
        if not parts:
            continue
        area = unary_union(parts)
        if not area.intersects(keepU[s]):
            continue
        new = area.difference(keepU[s])
        if area.area - new.area < 1e-6:
            continue
        polys = []
        gs = [new] if new.geom_type == 'Polygon' else \
            [g for g in getattr(new, 'geoms', []) if g.geom_type == 'Polygon']
        for g in gs:
            if g.area < 1e-5:
                continue
            ext = [list(c) for c in g.exterior.simplify(0.002).coords]
            hs = [[list(c) for c in ig.simplify(0.002).coords]
                  for ig in g.interiors if Polygon(ig).area > 1e-6]
            polys.append({'ext': ext, 'holes': hs})
        out[idx] = polys
    json.dump(out, open(outjson, 'w'))
    print("mede_serigrafia clip: %d/%d item(s)" % (len(out), len(d['items'])))
    return 0


def main():
    if len(sys.argv) >= 2 and sys.argv[1] == "clip":
        return _clip(sys.argv[2], sys.argv[3])
    return _portao(sys.argv[1:])


if __name__ == "__main__":
    sys.exit(main())
