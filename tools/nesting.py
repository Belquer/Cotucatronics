#!/usr/bin/env python3
"""nesting v0.1.0 — o portao de um ficheiro de CORTE, que nao e o de um desenho.

O `svg_colisoes.py` pergunta "o desenho le-se?". Num ficheiro de corte a
pergunta e outra e as respostas nao se substituem:

  desenho  : dois textos sobrepostos sao ilegiveis  -> defeito
  corte    : duas PECAS sobrepostas sao uma peca perdida e uma chapa gasta,
             e uma peca fora da chapa e um pedido recusado pela fabrica

Este verifica tres coisas, e so tres:

  1. toda a geometria cabe DENTRO da chapa (a Ponoko P2 e 384 x 384 mm);
  2. nenhuma peca se sobrepoe a outra (caixas envolventes, com folga de kerf);
  3. o numero de pecas e o que se espera — porque uma peca que nao foi emitida
     nao colide com nada, e um ficheiro com 18 pecas passa exactamente como um
     com 19. **Ausencia nao colide.**

⚠️ Caixa envolvente, nao contorno real. Duas pecas em L podem encaixar uma na
outra e este acusa. Falso positivo custa um olhar; peca perdida custa a chapa.

USO:  nesting.py <ficheiro.svg> [--chapa 384x384] [--pecas 19]
SAI:  0 limpo · 1 defeito · 2 uso · 3 NAO VERIFICAVEL (nada legivel)
"""
import io
import re
import sys

VERSAO = "0.1.0"
KERF = 0.2          # mm de folga entre pecas antes de se chamar sobreposicao


def num(v):
    m = re.match(r'^(-?[\d.]+(?:[eE][-+]?\d+)?)\s*(px|pt|mm|cm|in)?$', (v or "").strip())
    if not m:
        return None
    x = float(m.group(1))
    return {"cm": x * 10, "in": x * 25.4, None: x, "px": x, "pt": x, "mm": x}[m.group(2)]


def caixas(t):
    """Uma caixa envolvente por elemento de geometria de topo."""
    out = []
    for m in re.finditer(r'<(path|rect|circle|polygon|polyline)\b([^>]*)>', t, re.S):
        tag, at = m.group(1), m.group(2)
        xs, ys = [], []
        if tag == "rect":
            x = num(re.search(r'\bx="([^"]*)"', at).group(1)) if re.search(r'\bx="', at) else 0.0
            y = num(re.search(r'\by="([^"]*)"', at).group(1)) if re.search(r'\by="', at) else 0.0
            w = num(re.search(r'\bwidth="([^"]*)"', at).group(1)) if re.search(r'\bwidth="', at) else 0.0
            h = num(re.search(r'\bheight="([^"]*)"', at).group(1)) if re.search(r'\bheight="', at) else 0.0
            xs, ys = [x, x + w], [y, y + h]
        elif tag == "circle":
            cx = num(re.search(r'\bcx="([^"]*)"', at).group(1)) if re.search(r'\bcx="', at) else 0.0
            cy = num(re.search(r'\bcy="([^"]*)"', at).group(1)) if re.search(r'\bcy="', at) else 0.0
            r = num(re.search(r'\br="([^"]*)"', at).group(1)) if re.search(r'\br="', at) else 0.0
            xs, ys = [cx - r, cx + r], [cy - r, cy + r]
        else:
            d = re.search(r'\b(?:d|points)="([^"]*)"', at)
            if not d:
                continue
            n = [float(v) for v in re.findall(r'-?\d+\.?\d*(?:[eE][-+]?\d+)?', d.group(1))]
            if len(n) < 4:
                continue
            xs, ys = n[0::2], n[1::2]
        if xs and ys:
            out.append((min(xs), min(ys), max(xs), max(ys)))
    return out


def confere(p, chapa, esperadas):
    t = io.open(p, encoding="utf-8").read()
    cs = caixas(t)
    if not cs:
        print("%s: NAO VERIFICAVEL — nenhuma geometria legivel." % p)
        return 3
    W, H = chapa
    mal = 0
    fora = [c for c in cs if c[0] < -0.01 or c[1] < -0.01 or c[2] > W + 0.01 or c[3] > H + 0.01]
    # agrupar por peca: caixas que se tocam sao a mesma peca (contorno + furos)
    pecas = []
    for c in sorted(cs, key=lambda c: (c[0], c[1])):
        for i, p2 in enumerate(pecas):
            if not (c[2] < p2[0] or p2[2] < c[0] or c[3] < p2[1] or p2[3] < c[1]):
                pecas[i] = (min(c[0], p2[0]), min(c[1], p2[1]),
                            max(c[2], p2[2]), max(c[3], p2[3]))
                break
        else:
            pecas.append(c)
    sobre = []
    for i in range(len(pecas)):
        for j in range(i + 1, len(pecas)):
            a, b = pecas[i], pecas[j]
            if not (a[2] + KERF <= b[0] or b[2] + KERF <= a[0]
                    or a[3] + KERF <= b[1] or b[3] + KERF <= a[1]):
                sobre.append((a, b))
    print("%s: %d formas, %d pecas, chapa %gx%g mm" % (p, len(cs), len(pecas), W, H))
    if fora:
        mal = 1
        print("   *** %d formas FORA DA CHAPA:" % len(fora))
        for c in fora[:5]:
            print("       x %.1f..%.1f  y %.1f..%.1f" % (c[0], c[2], c[1], c[3]))
    if sobre:
        mal = 1
        print("   *** %d pares de pecas SOBREPOSTAS:" % len(sobre))
        for a, b in sobre[:5]:
            print("       (%.1f,%.1f)-(%.1f,%.1f)  ><  (%.1f,%.1f)-(%.1f,%.1f)"
                  % (a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]))
    if esperadas is not None and len(pecas) != esperadas:
        mal = 1
        print("   *** esperava %d pecas e contei %d — **ausencia nao colide**, "
              "e um ficheiro a menos uma peca passa como um completo" % (esperadas, len(pecas)))
    if not mal:
        print("   dentro da chapa, sem sobreposicao%s ✓"
              % (", e sao %d pecas" % esperadas if esperadas else ""))
    return mal


if __name__ == "__main__":
    print("nesting v%s" % VERSAO, file=sys.stderr)
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not args:
        print(__doc__)
        raise SystemExit(2)
    ch = (384.0, 384.0)
    pecas = None
    for a in sys.argv[1:]:
        if a.startswith("--chapa"):
            w, _, h = a.split("=")[-1].partition("x")
            ch = (float(w), float(h))
        if a.startswith("--pecas"):
            pecas = int(a.split("=")[-1])
    rc = 0
    for p in args:
        rc = max(rc, confere(p, ch, pecas))
    raise SystemExit(rc)
