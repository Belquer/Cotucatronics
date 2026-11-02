#!/usr/bin/env python3
"""colisoes_sch v0.5.0 — colisoes de texto dentro de um .kicad_sch.

POR QUE EXISTE, e nao e para substituir o olho.

⚠️ CORRIGIDO A 2026-09-06. Este cabecalho dizia que o `svg_colisoes.py` devolve
sempre codigo 3 nos exports do `kicad-cli` "porque nao ha um unico <text>
para medir". E FALSO, e foi medido: ha 222 no preamp — invisiveis
(`opacity="0"`) mas com `textLength`, que e a largura que o proprio KiCad
calculou. O que ele nao lia eram os GLIFOS como segmentos e os textos em
duplicado; corrigido no svg_colisoes v0.4.0, que ganhou `--esquematico`.

A razao para ESTE existir e outra, e continua de pe: ele le o FONTE, com as
coordenadas em milimetros antes de virarem curvas. E o par do outro, nao o
substituto — ver a skill `design-review`, que mede o que cada um ve e o que
cada um NAO ve.

Este le o FONTE em vez do render. As coordenadas e os tamanhos de letra estao
todos la, em milimetros, antes de virarem curvas.

⚠️ O QUE ELE NAO E. A largura de uma cadeia na fonte de tracos do KiCad e
estimada (`largura ≈ n × tamanho × 0,6`), nao medida. Portanto:
  - falso positivo e barato e esperado;
  - **codigo 0 nao autoriza saltar o olho** — autoriza olhar sabendo onde.
Foi calibrado a 2026-09-05 contra tres colisoes ja confirmadas a olho na folha do
painel; se uma alteracao futura o fizer perder alguma delas, o teste de
calibracao no fim do ficheiro grita.

USO:  colisoes_sch.py <ficheiro.kicad_sch> [...]
SAI:  0 limpo · 1 colisoes · 2 uso · 3 NAO VERIFICAVEL (nada legivel)
"""
import io
import re
import sys

VERSAO = "0.5.0"

# Avanco medio por caractere, em fraccao do corpo nominal escrito na folha.
#
# NAO E UM PALPITE. O export SVG do kicad-cli traz `textLength` em cada <text>
# — a largura que o proprio KiCad calculou. Medido a 2026-09-05 em tres notas da
# folha do painel: 0,799 · 0,802 · 0,849. O valor abaixo e o topo dessa faixa,
# porque falso positivo custa um olhar e falso negativo custa uma placa.
#
# O 0,60 da v0.1.0 vinha de fontes proporcionais de SVG e errava por 18 mm
# numa nota de 73 caracteres: dizia "limpo" sobre um texto que atravessava um
# potenciometro inteiro, e o olho via.
AVANCO = 0.85
ALTURA = 1.20


def _n(s):
    return float(s)


def caixas(t):
    """[(x0, y0, x1, y1, rotulo)] de tudo que imprime letras, fora do lib_symbols."""
    i = t.find("(lib_symbols")
    if i >= 0:
        d = 0
        for k in range(i, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    t = t[:i] + t[k + 1:]
                    break
    out = []
    num = r"-?[\d.]+(?:[eE][-+]?\d+)?"

    def cx(s, x, y, tam, just, tipo):
        w = len(s) * tam * AVANCO
        h = tam * ALTURA
        if just == "right":
            x0 = x - w
        elif just == "center":
            x0 = x - w / 2
        else:
            x0 = x
        return (x0, y - h / 2, x0 + w, y + h / 2, "%s %r" % (tipo, s[:38]))

    # notas soltas
    for m in re.finditer(r'\(text "((?:[^"\\]|\\.)*)"(?:.|\n)*?\(at (%s) (%s)'
                         r'(?:.|\n)*?\(size (%s)' % (num, num, num), t):
        s = m.group(1)
        out.append(cx(s, _n(m.group(2)), _n(m.group(3)), _n(m.group(4)), "left", "nota"))

    # rotulos: o triangulo do global_label ocupa mais um corpo de cada lado
    for tag in ("global_label", "hierarchical_label", "label"):
        for m in re.finditer(r'\(%s "((?:[^"\\]|\\.)*)"(?:.|\n)*?\(at (%s) (%s) (%s)\)'
                             r'((?:.|\n)*?)\(uuid' % (tag, num, num, num), t):
            s, x, y, ang = m.group(1), _n(m.group(2)), _n(m.group(3)), _n(m.group(4))
            corpo = m.group(5)
            ts = re.search(r'\(size (%s)' % num, corpo)
            tam = _n(ts.group(1)) if ts else 1.27
            ju = re.search(r'\(justify ([a-z]+)', corpo)
            just = ju.group(1) if ju else "left"
            if ang in (180.0, 270.0) and just == "left":
                just = "right"
            b = cx(s + ("  " if tag == "global_label" else ""), x, y, tam, just, tag)
            out.append(b)

    # referencia e valor de cada simbolo, VARRIDOS BLOCO A BLOCO
    #
    # ⚠️ Bloco a bloco, e nao por regex global, por duas razoes que ja morderam:
    #  1. a caixa precisa de saber DE QUEM e, para "valor por cima do corpo de
    #     outro componente" se poder distinguir de "valor ao lado do seu";
    #  2. um bloco pode comecar colado ao parentesis do anterior — `)\t(symbol`
    #     sem mudanca de linha — e uma regex ancorada em `\n\t(symbol` salta-o
    #     em silencio. Foi assim que o R0A escapou a uma renomeacao de 27.
    for m in re.finditer(r'\(symbol\n\s*\(lib_id ', t):
        j = m.start(); d = 0
        for k in range(j, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    bl = t[j:k + 1]
                    break
        else:
            continue
        dono = re.search(r'\(property "Reference" "([^"]+)"', bl)
        dono = dono.group(1) if dono else "?"
        for pm in re.finditer(r'\(property "(Reference|Value)" "((?:[^"\\]|\\.)*)"\s*\n'
                              r'\s*\(at (%s) (%s)(?: %s)?\)'
                              r'((?:.|\n){0,400}?)\(size (%s)' % (num, num, num, num), bl):
            s = pm.group(2)
            if not s or s.startswith("~") or "(hide yes)" in pm.group(5):
                continue
            b = cx(s, _n(pm.group(3)), _n(pm.group(4)), _n(pm.group(6)),
                   "center", pm.group(1).lower())
            out.append(b + (dono,))

    # GUARDA: contar o que existe contra o que se leu. Sem isto, uma regex que
    # deixa de casar transforma-se em "esta limpo" sem um unico aviso.
    esperado = (len(re.findall(r'\(text "', t))
                + len(re.findall(r'\(global_label "', t))
                + len(re.findall(r'\(hierarchical_label "', t))
                + len(re.findall(r'\n\s*\(label "', t))
                + len([m for m in re.finditer(
                    r'\(property "(?:Reference|Value)" "[^"~][^"]*"'
                    r'((?:.|\n){0,400}?)\(size', t)
                    if "(hide yes)" not in m.group(1)]))
    # A propriedade ESCONDIDA nao imprime nada e e saltada de proposito la em
    # cima; conta-la aqui fazia a guarda gritar sobre a folha do pre-amp por
    # causa das duas referencias ocultas dos PWR_FLAG. Guarda que da alarme
    # falso e guarda que se desliga.
    if len(out) < esperado:
        raise SystemExit('*** colisoes_sch: o ficheiro tem %d textos e a leitura '
                         'apanhou %d — LEITURA INCOMPLETA, "sem colisoes" nao '
                         'significa nada' % (esperado, len(out)))
    return out



def formas(t):
    """[(x0,y0,x1,y1,rotulo)] do que NAO e letra: fios e corpos de simbolo.

    ⚠️ SEM ISTO O VERIFICADOR MENTE. A v0.1.0 so comparava texto com texto e
    deu 1 colisao numa folha onde o olho via seis: a nota dos LED acaba a
    194,82 e o potenciometro comeca logo ali — nao toca em nenhuma LETRA, toca
    no CORPO do simbolo e no fio. "Texto × forma" e um item da lista fechada da
    skill design-review, e faltava-lhe metade da lista."""
    num = r"-?[\d.]+(?:[eE][-+]?\d+)?"
    # 1. a biblioteca embutida: caixa de cada simbolo, em coordenadas do simbolo
    corpos = {}
    i = t.find("(lib_symbols")
    if i >= 0:
        d = 0
        for k in range(i, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    lib, resto = t[i:k + 1], t[:i] + t[k + 1:]
                    break
        else:
            lib, resto = "", t
    else:
        lib, resto = "", t
    for m in re.finditer(r'\(symbol "([^":]+:)?([^"]+?)"\n', lib):
        nome = m.group(2)
        if re.search(r"_\d+_\d+$", nome):
            continue
        j = m.start(); d = 0
        for k in range(j, len(lib)):
            if lib[k] == "(":
                d += 1
            elif lib[k] == ")":
                d -= 1
                if d == 0:
                    b = lib[j:k + 1]; break
        xs, ys = [], []
        for r in re.finditer(r'\((?:start|end|mid|center|xy) (%s) (%s)\)' % (num, num), b):
            xs.append(float(r.group(1))); ys.append(float(r.group(2)))
        if xs:
            corpos[nome] = (min(xs), min(ys), max(xs), max(ys))

    out = []
    # 2. fios: rectangulo fino ao longo do segmento
    for m in re.finditer(r'\(wire\s*\(pts\s*\(xy (%s) (%s)\)\s*\(xy (%s) (%s)\)'
                         % (num, num, num, num), resto):
        x1, y1, x2, y2 = (float(m.group(i)) for i in (1, 2, 3, 4))
        out.append((min(x1, x2) - 0.2, min(y1, y2) - 0.2,
                    max(x1, x2) + 0.2, max(y1, y2) + 0.2, "fio"))
    # 3. corpos: caixa da biblioteca posta no sitio da instancia
    for m in re.finditer(r'\(symbol\s*\n\s*\(lib_id "([^"]+)"\)\s*\n\s*'
                         r'\(at (%s) (%s)(?: (%s))?\)' % (num, num, num), resto):
        nome = m.group(1).split(":")[-1]
        if nome not in corpos:
            continue
        ox, oy = float(m.group(2)), float(m.group(3))
        ang = float(m.group(4) or 0)
        a, b_, c, d_ = corpos[nome]
        cantos = [(a, b_), (c, b_), (a, d_), (c, d_)]
        pts = []
        for x, y in cantos:
            # y do simbolo cresce para CIMA, o da folha para BAIXO
            if ang == 90:
                x, y = -y, x
            elif ang == 180:
                x, y = -x, -y
            elif ang == 270:
                x, y = y, -x
            pts.append((ox + x, oy - y))
        xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
        dono = "?"
        dd = 0
        for k in range(m.start(), len(resto)):
            if resto[k] == "(":
                dd += 1
            elif resto[k] == ")":
                dd -= 1
                if dd == 0:
                    r = re.search(r'\(property "Reference" "([^"]+)"', resto[m.start():k + 1])
                    if r:
                        dono = r.group(1)
                    break
        out.append((min(xs), min(ys), max(xs), max(ys), "corpo %s" % nome, dono))
    return out


def cruza(a, b):
    return not (a[2] <= b[0] or b[2] <= a[0] or a[3] <= b[1] or b[3] <= a[1])


def area(a, b):
    return (min(a[2], b[2]) - max(a[0], b[0])) * (min(a[3], b[3]) - max(a[1], b[1]))


def confere(p, mostrar=True):
    t = io.open(p, encoding="utf-8").read()
    cs = caixas(t)
    fs = formas(t)
    if not cs:
        if mostrar:
            print("%s: NAO VERIFICAVEL — nenhum texto legivel no fonte." % p)
        return 3, []
    mal = []
    for i in range(len(cs)):
        for j in range(i + 1, len(cs)):
            if cruza(cs[i], cs[j]):
                mal.append((area(cs[i], cs[j]), cs[i], cs[j]))
    # texto × forma.
    #
    # ⚠️ A v0.4.1 so comparava as NOTAS contra as formas, com o argumento de que
    # "rotulo colado ao seu fio e normal". Verdade para o fio; FALSO para o
    # corpo de outro componente. A 2026-09-05 o valor `22p C0G` do C11 estava
    # desenhado DENTRO do corpo do R10 e a ferramenta disse "0 colisoes" — o
    # olho viu, ela nao. Uma excepcao larga de mais e uma cegueira com desculpa.
    #
    # A regra agora: a nota vale contra tudo; a referencia e o valor valem
    # contra o corpo de QUALQUER OUTRO simbolo (contra o proprio nao, que e
    # onde eles moram por desenho).
    for c in cs:
        eh_nota = c[4].startswith("nota")
        eh_campo = c[4].startswith(("reference", "value"))
        if not (eh_nota or eh_campo):
            continue
        for f in fs:
            if eh_campo and not f[4].startswith("corpo"):
                continue                      # campo contra fio: normal
            if eh_campo and len(c) > 5 and len(f) > 5 and c[5] == f[5]:
                continue                      # campo contra o SEU corpo
            if cruza(c, f):
                mal.append((area(c, f), c, f))
    mal.sort(key=lambda r: -r[0])
    if mostrar:
        print("%s: %d textos, %d formas, %d colisoes"
              % (p, len(cs), len(fs), len(mal)))
        for ar, a, b in mal[:25]:
            print("   %6.1f mm²  x≈%7.2f y≈%7.2f   %s  ><  %s"
                  % (ar, a[0], a[1], a[4], b[4]))
        if len(mal) > 25:
            print("   ... e mais %d" % (len(mal) - 25))
    return (1 if mal else 0), mal


if __name__ == "__main__":
    print("colisoes_sch v%s" % VERSAO, file=sys.stderr)
    if len(sys.argv) < 2:
        print(__doc__)
        raise SystemExit(2)
    rc = 0
    for p in sys.argv[1:]:
        r, _ = confere(p)
        rc = max(rc, r)
    raise SystemExit(rc)
