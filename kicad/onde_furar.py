#!/usr/bin/env python3
"""onde_furar v0.1.0 — onde se pode abrir um furo numa placa JA FABRICADA.

    (Python do KiCad)  kicad/onde_furar.py <cerebro|painel> [--diametro 3.4]

POR QUE EXISTE. A 20/09 o dB perguntou: *"nao vejo tracos em nenhum lado da
placa, posso furar entao?"*. A pergunta e boa e a premissa e falsa:

  ⚠️ **"nao vejo tracos" NAO e "nao ha cobre".** A face de baixo do cerebro e
  quase toda PLANO — GND com 8305 mm2 e AGND com 3266. Um plano nao parece um
  traco: e uma chapa continua debaixo da mascara, e leva toda a corrente de
  retorno do instrumento. Furar no meio dele pode nao cortar nada ou pode
  estrangular o caminho de retorno, e o rebarbado do furo poe cobre nu onde a
  cabeca do parafuso assenta.

  ⚠️ E a cabeca metalica do parafuso encosta nesse cobre nu. No cerebro isso e
  pior do que parece: ha DOIS terras (GND e AGND), separados de proposito — a
  separacao deles vale 35 a 39 dB de chao de ruido, medidos a 05/09. Um
  parafuso que toque nos dois solda-os um ao outro, e o instrumento volta ao
  chao de ruido de antes sem que nada pareca partido.

O QUE ESTA FERRAMENTA FAZ. Varre a placa num passo fino e diz onde um furo do
diametro pedido cabe com folga, **sem tocar** em: cobre de qualquer camada
(tracos, vias, pads, planos preenchidos), contorno da placa, ou o corpo
(courtyard) de qualquer peca — para o parafuso nao bater num componente.

SO LE. Nunca grava a placa. O `main()` do `pcb_contorno` grava; este nao.

⚠️ E O VEREDICTO FINAL NAO E DESTE FICHEIRO. Ele diz onde ha espaco no
DESENHO. A placa na mao pode ter sido furada, riscada ou ter peca fora de
sitio; e a broca pode andar. Quem fura confirma na peca, com a placa contra a
luz, e usa anilha isolante se o furo ficar a menos de 2 mm de qualquer cobre.
"""
import math
import os
import sys

VERSAO = "0.1.0"
AQUI = os.path.dirname(os.path.abspath(__file__))

# folgas, em mm, medidas do BORDO do furo (nao do centro)
FOLGA_COBRE = 1.0     # ate qualquer cobre: traco, via, pad ou plano
FOLGA_CORPO = 1.0     # ate o courtyard de qualquer peca
FOLGA_BORDO = 2.0     # ate o bordo da placa (o acrilico do fundo tambem pisa aqui)
CABECA = 6.0          # diametro da cabeca do parafuso M3 + anilha, para o assento


def _pcbnew():
    try:
        import pcbnew
        return pcbnew
    except ImportError:
        sys.exit("RECUSO: sem `pcbnew`. Corre com o Python do KiCad:\n"
                 "  /Applications/KiCad/KiCad.app/Contents/Frameworks/"
                 "Python.framework/Versions/3.9/bin/python3.9")


def varre(nome, diametro=3.4, passo=1.0):
    pcbnew = _pcbnew()
    p = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_pcb" % nome)
    b = pcbnew.LoadBoard(p)
    mm = lambda v: v / 1e6
    ui = lambda v: int(round(v * 1e6))

    # ⚠️ o raio que interessa e o do ASSENTO da cabeca, nao o do furo: o
    # parafuso aperta sobre uma coroa de 6 mm, e e ela que raspa o cobre.
    r_furo = max(diametro, CABECA) / 2.0

    contorno = pcbnew.SHAPE_POLY_SET()
    b.GetBoardPolygonOutlines(contorno, False)

    solidos = []                      # (poligono, folga) — tudo o que nao se pode tocar
    for z in [b.GetArea(i) for i in range(b.GetAreaCount())]:
        if not z.IsFilled():
            continue
        for lay in z.GetLayerSet().Seq():
            poly = z.GetFilledPolysList(lay)
            if poly.OutlineCount():
                solidos.append((poly, FOLGA_COBRE))
    for f in b.GetFootprints():
        for lado in (pcbnew.F_CrtYd, pcbnew.B_CrtYd):
            cy = f.GetCourtyard(lado)
            if cy.OutlineCount():
                solidos.append((cy, FOLGA_CORPO))

    pads, vias, tracos = [], [], []
    for f in b.GetFootprints():
        for pd in f.Pads():
            s = pd.GetSize()
            pads.append((mm(pd.GetPosition().x), mm(pd.GetPosition().y),
                         math.hypot(mm(s.x), mm(s.y)) / 2.0))
    for t in b.GetTracks():
        if t.GetClass() == "PCB_VIA":
            # ⚠️ em KiCad 10 a via tem largura POR CAMADA: GetWidth() sem argumento
            # dispara um assert e devolve um valor em que nao se pode confiar.
            try:
                w = t.GetWidth(pcbnew.F_Cu)
            except TypeError:
                w = t.GetWidth()
            vias.append((mm(t.GetPosition().x), mm(t.GetPosition().y), mm(w) / 2.0))
        else:
            tracos.append((mm(t.GetStart().x), mm(t.GetStart().y),
                           mm(t.GetEnd().x), mm(t.GetEnd().y), mm(t.GetWidth()) / 2.0))

    def dist_seg(px, py, x1, y1, x2, y2):
        dx, dy = x2 - x1, y2 - y1
        L = dx * dx + dy * dy
        u = 0.0 if L == 0 else max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / L))
        return math.hypot(px - (x1 + u * dx), py - (y1 + u * dy))

    o = contorno.COutline(0)
    xs = [mm(o.CPoint(k).x) for k in range(o.PointCount())]
    ys = [mm(o.CPoint(k).y) for k in range(o.PointCount())]
    x0, x1_, y0, y1_ = min(xs), max(xs), min(ys), max(ys)

    bons = []
    x = x0 + r_furo + FOLGA_BORDO
    while x <= x1_ - r_furo - FOLGA_BORDO:
        y = y0 + r_furo + FOLGA_BORDO
        while y <= y1_ - r_furo - FOLGA_BORDO:
            pt = pcbnew.VECTOR2I(ui(x), ui(y))
            ok = contorno.Contains(pt)
            if ok:
                for cx, cy_, cr in pads + vias:
                    if math.hypot(x - cx, y - cy_) < r_furo + cr + FOLGA_COBRE:
                        ok = False
                        break
            if ok:
                for ax, ay, bx, by, w in tracos:
                    if dist_seg(x, y, ax, ay, bx, by) < r_furo + w + FOLGA_COBRE:
                        ok = False
                        break
            if ok:
                for poly, folga in solidos:
                    if poly.Collide(pt, ui(r_furo + folga)):
                        ok = False
                        break
            if ok:
                bons.append((round(x, 1), round(y, 1)))
            y += passo
        x += passo
    return b, (x0, y0, x1_, y1_), bons, r_furo


def agrupa(bons, passo=1.0):
    """junta os pontos vizinhos numa mancha so, e devolve o centro de cada uma."""
    resto = set(bons)
    manchas = []
    while resto:
        semente = resto.pop()
        fila, mancha = [semente], [semente]
        while fila:
            px, py = fila.pop()
            for vx in (-passo, 0, passo):
                for vy in (-passo, 0, passo):
                    v = (round(px + vx, 1), round(py + vy, 1))
                    if v in resto:
                        resto.discard(v)
                        fila.append(v)
                        mancha.append(v)
        cx = sum(p[0] for p in mancha) / len(mancha)
        cy = sum(p[1] for p in mancha) / len(mancha)
        manchas.append((len(mancha), round(cx, 1), round(cy, 1), mancha))
    return sorted(manchas, reverse=True)


def main():
    print("onde_furar v%s — so le, nunca grava" % VERSAO, file=sys.stderr)
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) != 1 or args[0] not in ("cerebro", "painel"):
        print(__doc__)
        raise SystemExit(2)
    d = 3.4
    if "--diametro" in sys.argv:
        d = float(sys.argv[sys.argv.index("--diametro") + 1])
    nome = args[0]
    b, (x0, y0, x1, y1), bons, r = varre(nome, d)
    print("  placa %s · furo o%.1f mm · assento o%.1f mm" % (nome, d, 2 * r))
    print("  folgas: %.1f mm ao cobre · %.1f ao corpo das pecas · %.1f ao bordo"
          % (FOLGA_COBRE, FOLGA_CORPO, FOLGA_BORDO))
    if not bons:
        print("  ✗ NENHUM ponto livre — nao ha onde furar esta placa sem tocar cobre ou peca")
        return 1
    manchas = agrupa(bons)
    print("  %d ponto(s) livres em %d mancha(s):" % (len(bons), len(manchas)))
    for n, cx, cy, _ in manchas[:12]:
        print("     mancha de %3d mm² · centro a %.1f mm da esquerda e %.1f mm de cima"
              % (n, cx - x0, cy - y0))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
