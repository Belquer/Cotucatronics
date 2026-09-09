#!/usr/bin/env python3
import sys, math; sys.path.insert(0, '/home/claude/audit')
from kaudit import *
from collections import defaultdict
from shapely.geometry import Point, Polygon

def rot(px, py, ang):
    r = math.radians(ang); return (px*math.cos(r) - py*math.sin(r), px*math.sin(r) + py*math.cos(r))

def analyze(path, label):
    s = pcb_summary(path)
    copper = [l[1] for l in s['layers'] if l[2] in ('signal', 'power', 'mixed')]
    # ---- nós ----
    par = {}
    def find(a):
        while par.setdefault(a, a) != a: a = par[a]
        return a
    def union(a, b): par[find(a)] = find(b)
    pads = []   # (key, net, x, y, layers, ref, num, size, drill, shape)
    for f in s['footprints']:
        fx, fy, fa = f['at']
        for p in f['pads']:
            dx, dy, pa = p['at']
            # KiCad: pad at é relativo ao footprint, já rodado por 'fa' (pad rot é absoluto); y do footprint cresce para baixo
            rx, ry = rot(dx, dy, -fa)  # rotação do footprint no sistema com y para baixo
            x, y = fx + rx, fy + ry
            lays = copper if any(l in ('*.Cu',) for l in p['layers']) else [l for l in p['layers'] if l.endswith('.Cu')]
            key = ('pad', f['ref'], p['num'], round(x, 3), round(y, 3))
            pads.append(dict(key=key, net=p['net'], x=x, y=y, layers=lays, ref=f['ref'], num=p['num'], size=p['size'], drill=p['drill'], type=p['type']))
    vias = [dict(key=('via', round(v['at'][0], 3), round(v['at'][1], 3)), net=v['net'], x=v['at'][0], y=v['at'][1], layers=copper, size=(v['size'], v['size']), drill=v['drill'], vsize=v['size']) for v in s['vias']]
    segs = []
    for sg in s['segments']:
        segs.append(dict(net=sg['net'], a=sg['start'], b=sg['end'], layer=sg['layer'], width=sg['width']))
    TOL = 0.02
    def near(x1, y1, x2, y2): return abs(x1-x2) < TOL and abs(y1-y2) < TOL
    # nós de segmentos: cada extremo é um nó (layer-specific)
    for i, sg in enumerate(segs):
        sg['ka'] = ('seg', i, 'a'); sg['kb'] = ('seg', i, 'b'); union(sg['ka'], sg['kb'])
    # ligar extremos coincidentes na mesma camada
    byl = defaultdict(list)
    for sg in segs:
        byl[sg['layer']].append((sg['a'], sg['ka'])); byl[sg['layer']].append((sg['b'], sg['kb']))
    for lay, pts in byl.items():
        pts.sort()
        for i in range(len(pts)):
            for j in range(i+1, min(i+30, len(pts))):
                if abs(pts[j][0][0]-pts[i][0][0]) > TOL: break
                if near(*pts[i][0], *pts[j][0]): union(pts[i][1], pts[j][1])
    # ponto de segmento a meio de outro segmento (T sem via): ligar
    def on_seg(px, py, a, b, w):
        (x1, y1), (x2, y2) = a, b
        L2 = (x2-x1)**2 + (y2-y1)**2
        if L2 == 0: return near(px, py, x1, y1)
        t = max(0, min(1, ((px-x1)*(x2-x1) + (py-y1)*(y2-y1)) / L2))
        cx, cy = x1 + t*(x2-x1), y1 + t*(y2-y1)
        return math.hypot(px-cx, py-cy) <= w/2 + TOL
    for lay, pts in byl.items():
        layer_segs = [sg for sg in segs if sg['layer'] == lay]
        for (p, k) in pts:
            for sg in layer_segs:
                if sg['ka'] != k and sg['kb'] != k and on_seg(p[0], p[1], sg['a'], sg['b'], sg['width']): union(k, sg['ka'])
    # pads e vias ↔ segmentos (extremo dentro do pad/via) e ↔ vias
    nodes = pads + vias
    for nd in nodes:
        r = (max(nd['size'])/2 if nd.get('size') else 0.75)
        for sg in segs:
            if sg['layer'] in nd['layers']:
                for pt, k in ((sg['a'], sg['ka']), (sg['b'], sg['kb'])):
                    if math.hypot(pt[0]-nd['x'], pt[1]-nd['y']) <= r + TOL: union(nd['key'], k)
    for i in range(len(nodes)):
        for j in range(i+1, len(nodes)):
            a, b = nodes[i], nodes[j]
            if set(a['layers']) & set(b['layers']) and math.hypot(a['x']-b['x'], a['y']-b['y']) <= (max(a.get('size') or (1,1))/2 + max(b.get('size') or (1,1))/2): union(a['key'], b['key'])
    # zonas: tudo do mesmo net dentro do polígono cheio na camada liga-se à zona
    zone_nodes = []
    for zi, z in enumerate(s['zones']):
        if z['keepout']: continue
        zk = ('zone', zi); zone_nodes.append((zk, z['net_name']))
        polys = [(lay, Polygon(pts)) for lay, pts in z['fills'] if len(pts) >= 3]
        for nd in nodes:
            if nd['net'] != z['net_name']: continue
            for lay, poly in polys:
                if lay in nd['layers'] and poly.intersects(Point(nd['x'], nd['y']).buffer((max(nd['size'])/2 if nd.get('size') else 0.5)*0.9)): union(zk, nd['key']); break
        for sg in segs:
            if sg['net'] != z['net_name']: continue
            for lay, poly in polys:
                if lay == sg['layer'] and (poly.buffer(0.05).contains(Point(*sg['a'])) or poly.buffer(0.05).contains(Point(*sg['b']))): union(zk, sg['ka']); break
    # ---- ilhas por rede ----
    print(f"\n===== {label}: {len(pads)} pads, {len(segs)} segmentos, {len(vias)} vias, {len(s['zones'])} zonas, cobre {copper}")
    bynet = defaultdict(set)
    for nd in pads:
        if nd['net'] and not nd['net'].startswith('unconnected'): bynet[nd['net']].add(find(nd['key']))
    unrouted = {n: c for n, c in bynet.items() if len(c) > 1}
    print(f"redes com pads: {len(bynet)} | redes com mais de uma ilha (por rotear): {len(unrouted)}")
    for n, c in sorted(unrouted.items()):
        groups = defaultdict(list)
        for nd in pads:
            if nd['net'] == n: groups[find(nd['key'])].append(f"{nd['ref']}.{nd['num']}")
        print(f"  !! {n}: {len(c)} ilhas → " + " || ".join(",".join(g) for g in groups.values()))
    # ---- ponte única GND↔AGND: excluir NT1 e ver se ficam separados; e verificar que nada mais os une ----
    nets_of_root = defaultdict(set)
    for nd in pads:
        if nd['net'] in ('GND', 'AGND') and nd['ref'] != 'NT1': nets_of_root[find(nd['key'])].add(nd['net'])
    mixed = [r for r, ns in nets_of_root.items() if len(ns) > 1]
    print("GND e AGND partilham cobre fora do NT1?", "SIM — DEFEITO" if mixed else "não (separados; NT1 é a única ponte)")
    # ---- DRC-lite ----
    minw = min(sg['width'] for sg in segs) if segs else None
    print(f"pista mínima: {minw} mm | vias: " + ", ".join(sorted({f"ø{v['vsize']}/furo{v['drill']}" for v in vias})))
    thin = [(sg['net'], sg['width']) for sg in segs if sg['width'] < 0.2]
    print("pistas < 0,20 mm:", len(thin), sorted(set(thin))[:8])
    # anel das vias
    for v in vias:
        if (v['vsize'] - v['drill'])/2 < 0.13: print("  !! via anel fino", v['vsize'], v['drill']); break
    # furos THT: mínimo
    drills = sorted({(nd['ref'], float(nd['drill'])) for nd in pads if isinstance(nd['drill'], (int, float))}, key=lambda t: t[1])
    print("furos THT mínimo/máximo:", drills[0], drills[-1])
    # cobre ao bordo
    bx0, by0, bx1, by1 = edge_bbox(s['edges'])
    def edge_dist(x, y): return min(x-bx0, bx1-x, y-by0, by1-y)
    close = []
    for sg in segs:
        for pt in (sg['a'], sg['b']):
            d = edge_dist(*pt) - sg['width']/2
            if d < 0.3: close.append((sg['net'], round(d, 2)))
    for nd in nodes:
        r = max(nd['size'])/2 if nd.get('size') else 0.75
        d = edge_dist(nd['x'], nd['y']) - r
        if d < 0.3: close.append((nd.get('ref', 'via'), round(d, 2)))
    print("cobre a < 0,3 mm do bordo:", len(close), sorted(set(close))[:6])
    for z in s['zones']:
        for lay, pts in z['fills']:
            xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
            d = min(min(xs)-bx0, bx1-max(xs), min(ys)-by0, by1-max(ys))
            if d < 0.3: print(f"  zona {z['net_name']} {lay} a {d:.2f} mm do bordo")
    # ---- alturas de pista por rede de potência ----
    pw = defaultdict(set)
    for sg in segs:
        if sg['net'] in ('+15V', '+5V', 'GND', 'AGND', 'V+PRE', '+5V_PONTE', '+3V3A', 'EXC_L', 'EXC_R'): pw[sg['net']].add(sg['width'])
    print("larguras por rede de potência:", {k: sorted(v) for k, v in pw.items()})
    # ---- pads sem rede que não são NC ----
    nonet = [(nd['ref'], nd['num']) for nd in pads if nd['net'] is None and nd['type'] != 'np_thru_hole' and nd['num'] not in ('MP', '')]
    print("pads SEM rede (nem 'unconnected'):", nonet)
    # ---- textos fora da placa e no cobre ----
    for t, lay, (x, y) in s['texts']:
        if not (bx0 <= x <= bx1 and by0 <= y <= by1): print("  texto fora da placa:", t, lay, (x, y))
        if lay.endswith('.Cu'): print("  texto em cobre:", t, lay)
    return s

if __name__ == '__main__':
    analyze('/home/claude/audit/cotuca_cerebro/cotuca_cerebro.kicad_pcb', 'CÉREBRO')
    analyze('/home/claude/audit/cotuca_painel/cotuca_painel.kicad_pcb', 'PAINEL')
