#!/usr/bin/env python3
"""onde_furar v0.3.0 — onde se pode abrir um furo numa placa JA FABRICADA.

    (Python do KiCad)  kicad/onde_furar.py <cerebro|painel> [--diametro 3.4]
                       [--ponto X,Y]  avalia ESTE ponto (mm da esquerda e do
                                      topo da placa, vista por cima); repetivel
                       [--plastico]   parafuso isolante: o furo pode cair
                                      DENTRO de um plano, mas nunca em traco,
                                      via ou pad — e o relatorio diz que plano
                                      e com que margem ate ao bordo dele.

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

VERSAO = "0.3.0"
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


# ⚠️ O PARAFUSO DE PLASTICO RESOLVE UM DOS DOIS PROBLEMAS, NAO OS DOIS.
#   resolve: a cabeca metalica a tocar cobre nu, e a ponte GND<->AGND.
#   NAO resolve: o furo CORTA cobre. Num plano largo um furo de 3,4 mm e um
#   buraco num campo; num ESTRANGULAMENTO do plano, ele parte o caminho de
#   retorno — e corrente de retorno que da a volta e a familia de defeito mais
#   cara deste projecto (31/08: o terra custou 35 a 39 dB de chao de ruido).
# Por isso o modo --plastico nao diz so "pode": diz em que plano cai e a que
# distancia do bordo desse plano, que e a medida do estrangulamento.
def varre(nome, diametro=3.4, passo=1.0, plastico=False):
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
    planos = []                       # (rede, camada, poligono) — cobre de plano
    for z in [b.GetArea(i) for i in range(b.GetAreaCount())]:
        if not z.IsFilled():
            continue
        for lay in z.GetLayerSet().Seq():
            poly = z.GetFilledPolysList(lay)
            if not poly.OutlineCount():
                continue
            planos.append((z.GetNetname() or "(sem rede)", b.GetLayerName(lay), poly))
            if not plastico:
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
    return b, (x0, y0, x1_, y1_), bons, r_furo, planos


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


def margem_no_plano(pcbnew, planos, x, y, r_furo, limite=12.0):
    """Em que plano(s) o furo cai, e a que distancia do bordo do plano.

    A distancia mede-se por tentativa: aumenta-se o raio ate o furo deixar de
    caber dentro do plano. E a medida do estrangulamento — pouca margem quer
    dizer que o furo come um pescoco por onde passa a corrente toda."""
    ui = lambda v: int(round(v * 1e6))
    pt = pcbnew.VECTOR2I(ui(x), ui(y))
    achados = []
    for rede, camada, poly in planos:
        if not poly.Collide(pt, ui(r_furo)):
            continue
        folga = r_furo
        while folga < limite and poly.Collide(pt, ui(folga + 0.5)):
            folga += 0.5
        achados.append((rede, camada, folga - r_furo))
    return achados


def avalia(nome, pontos, diametro=3.4):
    """"Posso furar AQUI?" — a pergunta como ela e feita na bancada.

    Recebe pontos em mm da esquerda e do topo da placa (que e como se mede com
    a regua) e devolve, por ponto, a distancia ao cobre mais proximo de cada
    especie. Nao decide sozinho entre parafuso metalico e isolante: da os dois
    veredictos, porque a escolha e de quem monta."""
    pcbnew = _pcbnew()
    b, (x0, y0, x1, y1), _, r_furo, planos = varre(nome, diametro, passo=1e9)
    mm = lambda v: v / 1e6
    ui = lambda v: int(round(v * 1e6))
    pads, vias, tracos, corpos = [], [], [], []
    for f in b.GetFootprints():
        for pd in f.Pads():
            s = pd.GetSize()
            pads.append((f.GetReference(), mm(pd.GetPosition().x), mm(pd.GetPosition().y),
                         math.hypot(mm(s.x), mm(s.y)) / 2.0))
        for lado in (pcbnew.F_CrtYd, pcbnew.B_CrtYd):
            cy = f.GetCourtyard(lado)
            if cy.OutlineCount():
                corpos.append((f.GetReference(), cy))
    for tr in b.GetTracks():
        if tr.GetClass() == "PCB_VIA":
            try:
                w = tr.GetWidth(pcbnew.F_Cu)
            except TypeError:
                w = tr.GetWidth()
            vias.append(("via", mm(tr.GetPosition().x), mm(tr.GetPosition().y), mm(w) / 2.0))
        else:
            tracos.append((mm(tr.GetStart().x), mm(tr.GetStart().y),
                           mm(tr.GetEnd().x), mm(tr.GetEnd().y), mm(tr.GetWidth()) / 2.0))

    def dist_seg(px, py, ax, ay, bx, by):
        dx, dy = bx - ax, by - ay
        L = dx * dx + dy * dy
        u = 0.0 if L == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / L))
        return math.hypot(px - (ax + u * dx), py - (ay + u * dy))

    saida = []
    for (px, py) in pontos:
        x, y = x0 + px, y0 + py
        perto_p = min(((math.hypot(x - cx, y - cy) - cr, ref) for ref, cx, cy, cr in pads + vias),
                      default=(99, "-"))
        perto_t = min((dist_seg(x, y, *tr[:4]) - tr[4] for tr in tracos), default=99)
        bate = [ref for ref, cy in corpos if cy.Collide(pcbnew.VECTOR2I(ui(x), ui(y)), ui(r_furo))]
        bordo = min(px, py, (x1 - x0) - px, (y1 - y0) - py)
        emplano = margem_no_plano(pcbnew, planos, x, y, r_furo)
        saida.append(dict(ponto=(px, py), pad=perto_p, traco=perto_t, corpos=bate,
                          bordo=bordo, planos=emplano))
    return saida, r_furo


def main():
    print("onde_furar v%s — so le, nunca grava" % VERSAO, file=sys.stderr)
    # ⚠️ filtrar so pelo "--" comia o NOME DA PLACA quando vinha depois de uma
    # opcao com valor (--ponto 114,5): o valor nao comeca por "--" e passava por
    # nome de placa. Saltar o valor de cada opcao que o leva.
    COM_VALOR = ("--ponto", "--diametro")
    args, saltar = [], False
    for a_ in sys.argv[1:]:
        if saltar:
            saltar = False
            continue
        if a_ in COM_VALOR:
            saltar = True
        elif not a_.startswith("--"):
            args.append(a_)
    if len(args) != 1 or args[0] not in ("cerebro", "painel"):
        print(__doc__)
        raise SystemExit(2)
    d = 3.4
    if "--diametro" in sys.argv:
        d = float(sys.argv[sys.argv.index("--diametro") + 1])
    nome = args[0]
    if "--ponto" in sys.argv:
        pontos = [tuple(float(v) for v in sys.argv[i + 1].split(","))
                  for i, a in enumerate(sys.argv) if a == "--ponto"]
        res, r = avalia(nome, pontos, d)
        print("  placa %s · furo o%.1f mm · assento o%.1f mm (mm da esquerda e do topo)"
              % (nome, d, 2 * r))
        mau = 0
        for e in res:
            px, py = e["ponto"]
            folga_pad = e["pad"][0] - r
            folga_tr = e["traco"] - r
            print("  ── %.1f , %.1f" % (px, py))
            print("     pad/via mais proximo: %.1f mm de folga (%s) · traco: %.1f mm · bordo: %.1f mm"
                  % (folga_pad, e["pad"][1], folga_tr, e["bordo"] - r))
            if e["corpos"]:
                print("     ✗ o assento bate no corpo de: %s" % " ".join(e["corpos"]))
            for rede, camada, folga in e["planos"]:
                print("     • cai no plano %s (%s) · margem ao bordo do plano %s"
                      % (rede, camada, ("> %.1f mm — campo largo" % folga) if folga >= 6
                         else ("%.1f mm — ESTRANGULAMENTO" % folga)))
            ok_iso = (folga_pad >= FOLGA_COBRE and folga_tr >= FOLGA_COBRE
                      and not e["corpos"] and e["bordo"] - r >= FOLGA_BORDO
                      and all(f >= 6 for _, _, f in e["planos"]))
            ok_met = ok_iso and not e["planos"]
            print("     %s parafuso ISOLANTE · %s parafuso METALICO"
                  % ("✓" if ok_iso else "✗", "✓" if ok_met else "✗"))
            mau += not ok_iso
        return 1 if mau else 0
    plastico = "--plastico" in sys.argv
    b, (x0, y0, x1, y1), bons, r, planos = varre(nome, d, plastico=plastico)
    print("  placa %s · furo o%.1f mm · assento o%.1f mm%s"
          % (nome, d, 2 * r, " · PARAFUSO ISOLANTE (o furo pode cair em plano)" if plastico else ""))
    print("  folgas: %.1f mm a traco/via/pad · %.1f ao corpo das pecas · %.1f ao bordo%s"
          % (FOLGA_COBRE, FOLGA_CORPO, FOLGA_BORDO,
             "" if plastico else " · e ao plano"))
    if not bons:
        print("  ✗ NENHUM ponto livre — nao ha onde furar esta placa sem tocar cobre ou peca")
        return 1
    manchas = agrupa(bons)
    print("  %d ponto(s) livres em %d mancha(s):" % (len(bons), len(manchas)))
    pcbnew = _pcbnew()
    for n, cx, cy, _ in manchas[:12]:
        linha = ("     mancha de %3d mm² · centro a %.1f mm da esquerda e %.1f mm de cima"
                 % (n, cx - x0, cy - y0))
        for rede, camada, folga in margem_no_plano(pcbnew, planos, cx, cy, r):
            linha += "\n        ⚠️ corta o plano %s em %s · margem ate ao bordo do plano: %s" % (
                rede, camada,
                ("> %.1f mm (campo largo)" % folga) if folga >= 6.0 else
                ("%.1f mm — ESTRANGULAMENTO, nao furar aqui" % folga))
        print(linha)
    if plastico:
        print("  ⚠️ parafuso isolante resolve a cabeca a tocar cobre; NAO resolve o furo"
              "\n     cortar plano. Le a margem de cada mancha antes de escolher.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
