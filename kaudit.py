#!/usr/bin/env python3
"""kaudit.py — leitor estrutural de .kicad_pcb / .kicad_sch (S-expressions), sem regex à aparência."""
import sys, math, re
from collections import defaultdict

def parse(text):
    """S-expr → listas aninhadas; strings entre aspas → str com prefixo, símbolos → str, números → float/int."""
    i, n = 0, len(text); stack = [[]]
    while i < n:
        c = text[i]
        if c.isspace(): i += 1; continue
        if c == '(':
            stack.append([]); i += 1; continue
        if c == ')':
            node = stack.pop(); stack[-1].append(node); i += 1; continue
        if c == '"':
            j = i + 1; buf = []
            while j < n:
                if text[j] == '\\' and j + 1 < n: buf.append(text[j+1]); j += 2; continue
                if text[j] == '"': break
                buf.append(text[j]); j += 1
            stack[-1].append(('S', ''.join(buf))); i = j + 1; continue
        j = i
        while j < n and not text[j].isspace() and text[j] not in '()': j += 1
        tok = text[i:j]; i = j
        try:
            stack[-1].append(float(tok) if ('.' in tok or 'e' in tok.lower()) and not tok.startswith('0x') else int(tok))
        except ValueError:
            stack[-1].append(tok)
    return stack[0][0]

def isstr(x): return isinstance(x, tuple) and x[0] == 'S'
def sval(x): return x[1] if isstr(x) else x
def kids(node, name):
    return [k for k in node[1:] if isinstance(k, list) and k and k[0] == name]
def kid(node, name, default=None):
    ks = kids(node, name); return ks[0] if ks else default
def atoms(node):
    return [sval(x) for x in node[1:] if not isinstance(x, list)]
def prop(node, name):
    for p in kids(node, 'property'):
        if len(p) > 2 and sval(p[1]) == name: return sval(p[2])
    return None

def netname(node):
    if not node: return None
    vals = [sval(x) for x in node[1:] if not isinstance(x, list)]
    strs = [v for v in vals if isinstance(v, str)]
    return strs[0] if strs else (str(vals[0]) if vals else None)

def load(path):
    return parse(open(path, encoding='utf-8').read())

# ---------------- PCB ----------------
def pcb_summary(path):
    b = load(path)
    out = {}
    out['layers'] = [(k[0], sval(k[1]), sval(k[2]) if len(k) > 2 else None) for k in kid(b, 'layers')[1:]]
    out['nets'] = sorted(set(netname(k) for k in kids(b, 'net')))
    fps = []
    for f in kids(b, 'footprint'):
        at = kid(f, 'at'); layer = sval(kid(f, 'layer')[1])
        attr = atoms(kid(f, 'attr', ['attr']))
        pads = []
        for p in kids(f, 'pad'):
            pat = kid(p, 'at'); net = kid(p, 'net')
            pads.append(dict(num=str(sval(p[1])), type=sval(p[2]), shape=sval(p[3]),
                             at=(pat[1], pat[2], pat[3] if len(pat) > 3 else 0),
                             size=tuple(kid(p, 'size')[1:3]) if kid(p, 'size') else None,
                             drill=(kid(p, 'drill')[1] if kid(p, 'drill') and len(kid(p, 'drill')) > 1 and not isinstance(kid(p, 'drill')[1], list) else None),
                             layers=[sval(x) for x in kid(p, 'layers', ['layers'])[1:]],
                             net=netname(net)))
        fps.append(dict(lib=sval(f[1]), ref=prop(f, 'Reference'), value=prop(f, 'Value'),
                        footprint=prop(f, 'Footprint'), lcsc=prop(f, 'LCSC') or prop(f, 'LCSC Part') or prop(f, 'LCSC Part #'),
                        mpn=prop(f, 'MPN'), layer=layer, at=(at[1], at[2], at[3] if len(at) > 3 else 0),
                        attr=attr, pads=pads, node=f))
    out['footprints'] = fps
    segs = []
    for s in kids(b, 'segment'):
        segs.append(dict(start=tuple(kid(s, 'start')[1:3]), end=tuple(kid(s, 'end')[1:3]), width=kid(s, 'width')[1],
                         layer=sval(kid(s, 'layer')[1]), net=netname(kid(s, 'net'))))
    out['segments'] = segs
    vias = []
    for v in kids(b, 'via'):
        vias.append(dict(at=tuple(kid(v, 'at')[1:3]), size=kid(v, 'size')[1], drill=kid(v, 'drill')[1],
                         layers=[sval(x) for x in kid(v, 'layers')[1:]], net=netname(kid(v, 'net'))))
    out['vias'] = vias
    zones = []
    for z in kids(b, 'zone'):
        pts = kid(kid(z, 'polygon'), 'pts')
        poly = [(p[1], p[2]) for p in pts[1:] if p[0] == 'xy']
        fills = []
        for fp in kids(z, 'filled_polygon'):
            fpts = kid(fp, 'pts'); fills.append((sval(kid(fp, 'layer')[1]), [(p[1], p[2]) for p in fpts[1:] if p[0] == 'xy']))
        lay = kid(z, 'layers') or kid(z, 'layer')
        zones.append(dict(net=netname(kid(z, 'net')), net_name=(sval(kid(z, 'net_name')[1]) if kid(z, 'net_name') else netname(kid(z, 'net'))), layers=[sval(x) for x in lay[1:]],
                          priority=(kid(z, 'priority')[1] if kid(z, 'priority') else 0), poly=poly, fills=fills,
                          keepout=kid(z, 'keepout') is not None, name=sval(kid(z, 'name')[1]) if kid(z, 'name') else ''))
    out['zones'] = zones
    edges = []
    for g in b[1:]:
        if isinstance(g, list) and g and g[0] in ('gr_line', 'gr_arc', 'gr_rect', 'gr_circle', 'gr_poly'):
            lay = kid(g, 'layer')
            if lay and sval(lay[1]) == 'Edge.Cuts': edges.append(g)
    out['edges'] = edges
    texts = []
    for g in b[1:]:
        if isinstance(g, list) and g and g[0] == 'gr_text':
            texts.append((sval(g[1]), sval(kid(g, 'layer')[1]), tuple(kid(g, 'at')[1:3])))
    out['texts'] = texts
    setup = kid(b, 'setup'); out['setup'] = setup
    return out

def edge_bbox(edges):
    xs, ys = [], []
    for g in edges:
        for tag in ('start', 'end', 'mid', 'center'):
            k = kid(g, tag)
            if k: xs.append(k[1]); ys.append(k[2])
        pts = kid(g, 'pts')
        if pts:
            for p in pts[1:]: xs.append(p[1]); ys.append(p[2])
    return (min(xs), min(ys), max(xs), max(ys)) if xs else None

if __name__ == '__main__':
    for path in sys.argv[1:]:
        s = pcb_summary(path)
        print(f"\n=== {path}")
        print("camadas de cobre:", [l for l in s['layers'] if l[2] in ('signal', 'power', 'mixed')])
        print("redes:", len(s['nets']), "| footprints:", len(s['footprints']), "| segmentos:", len(s['segments']), "| vias:", len(s['vias']), "| zonas:", len(s['zones']))
        print("contorno bbox:", edge_bbox(s['edges']))
        for z in s['zones']: print("  zona", z['net_name'], z['layers'], "prio", z['priority'], "keepout" if z['keepout'] else "", f"{len(z['poly'])} pts, {len(z['fills'])} fills")
