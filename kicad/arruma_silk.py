#!/usr/bin/env python3
"""arruma_silk v0.12.0 — tira os campos de referencia de cima dos pads.

⚠️ SERIGRAFIA SO. Nao toca em cobre, em pistas, em zonas nem em posicoes de
peca: mexe apenas no `(at ...)` do campo `Reference` de cada footprint, que e
texto. Uma placa routeada pode passar por aqui sem perder nada.

O defeito que arruma: numa grelha apertada o campo de referencia nasce a uma
distancia fixa da peca e vai parar em cima do pad da vizinha. A JLC recorta a
serigrafia sobre a mascara, entao o que se perde nao e funcao — e a legibilidade
na bancada, que e o que se paga quando se solda a mao e se depura.

Como escolhe: prova oito posicoes a volta da peca, por ordem de preferencia
(fora da placa primeiro, para o texto fugir do centro cheio), e fica na PRIMEIRA
que nao caia em cima de pad nenhum nem de outra referencia ja colocada.

⚠️ E SE NENHUMA SERVIR, DEIXA COMO ESTA e diz. Empurrar para um sitio mau nao e
arrumar — e trocar de sitio o mesmo problema, com a diferenca de eu ja nao estar
a olhar.

USO:  arruma_silk.py <ficheiro.kicad_pcb> [--escreve]
      sem --escreve, so diz o que faria.
"""
import io, math, os, re, sys

VERSAO = "0.12.0"
NUM = r"-?\d+(?:\.\d+)?"
# ⚠️⚠️ UM DESIGNADOR E UM RECTANGULO LARGO E BAIXO, NAO UM CIRCULO.
#
# `RAIO_TXT = 1.6` modelava o texto como um disco: 1,6 mm em todas as
# direccoes. Um `C24` a 1 mm de altura ocupa ~2,8 x 1,4 mm — o dobro na
# horizontal e metade na vertical. O disco e simultaneamente GRANDE de mais em
# cima (recusa posicoes boas) e PEQUENO de mais aos lados (aceita posicoes que
# caem sobre um pad).
#
# Media-se pelo resultado: com o disco, arrumar as referencias comprava 3
# `silk_over_copper` por placa que nao eram precisos. O DRC ve a caixa real do
# texto; o modelo tinha de a ver tambem.
#
# ⚠️ 0,7 em por caractere e a largura de traço do KiCad, e e CONSERVADORA de
# proposito — acusa antes do toque. Falso positivo custa mover um rotulo; falso
# negativo custa um designador recortado pela mascara.
def caixa_txt(texto, tam=1.0):
    """(meia-largura, meia-altura) de um campo de referencia."""
    return (max(len(texto), 1) * tam * 0.35 + 0.15, tam * 0.7)


RAIO_TXT = 1.6          # so para as pistas, que sao finas e isotropicas
PASSOS = [1.6, 2.6, 3.6, 4.8]
# ⚠️ TECTO da distancia a propria peca. Sem ele, "livre" recompensa fugir para o
# vazio: quanto mais longe, menos obstaculos — e o optimo de uma restricao sem
# objectivo e sempre a borda da placa.
TECTO_REF = 6.0
# ⚠️ o recuo pode ir MAIS LONGE antes de aceitar cobre por cima. O tecto normal
# e uma preferencia estetica; o criterio do dono e que garante a correccao, e
# ele continua a valer a 9 mm. Entre "limpo a 8 mm da sua peca, e ainda a mais
# proxima dela" e "recortado pela mascara a 3 mm", o legivel ganha.
TECTO_RECUO = 9.0
PASSOS_LONGE = [6.0, 7.2, 8.4]   # so o recuo os alcanca (ver TECTO_REF)


def blocos(t):
    out = []
    for m in re.finditer(r"\n\t\(footprint ", t):
        j = m.start() + 1
        d = 0
        for k in range(j, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    out.append((j, k + 1, t[j:k + 1]))
                    break
    return out


def num(v):
    r"""⚠️ §2b: TODO numero que entra num ficheiro KiCad passa por aqui.

    A rotacao de 180° produz `cos(pi) = -1` e `sin(pi) = 1.2e-16`, e o offset
    do `U2` saiu como `-4.40873e-16`. Zero, escrito em notacao cientifica.

    O KiCad aceita-o sem se queixar, e nenhum leitor escrito com `(-?[\d.]+)`
    — que e como toda a gente escreve — apanha essa linha. Foi assim que os
    pinos 13 e 14 de um IDC26 sumiram de QUATRO geradores ao mesmo tempo, sem
    erro, sem aviso, sem ERC; as duas vias que faltavam eram terra.

    0,1 um esta tres ordens de grandeza abaixo de qualquer tolerancia de
    fabrico, e o `+ 0.0` troca `-0.0` por `0`."""
    return "%g" % (round(float(v), 4) + 0.0)


def abs_do_campo(fx, fy, ang, dx, dy):
    """Offset do campo (espaco da PECA) -> coordenada absoluta (espaco da PLACA).

    ⚠️ E EXACTAMENTE a transformacao que os pads ja usavam. Ter duas contas
    para a mesma mudanca de espaco, e so uma delas escrita, foi o defeito:
    ver o comentario no sitio da chamada."""
    return (fx + dx * math.cos(ang) + dy * math.sin(ang),
            fy - dx * math.sin(ang) + dy * math.cos(ang))


def campo_do_abs(ang, ex, ey):
    """A inversa: delta absoluto (PLACA) -> offset a gravar no campo (PECA)."""
    return (ex * math.cos(ang) - ey * math.sin(ang),
            ex * math.sin(ang) + ey * math.cos(ang))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    p = sys.argv[1]
    escreve = "--escreve" in sys.argv
    print("arruma_silk v%s — %s%s" % (VERSAO, os.path.basename(p),
                                      "" if escreve else "  (ensaio)"), file=sys.stderr)
    t = io.open(p, encoding="utf-8").read()

    # ⚠️ A SERIGRAFIA TAMBEM E OBSTACULO, e nao so o cobre.
    #
    # A primeira versao desviava a referencia dos PADS e pousava-a em cima do
    # contorno da propria peca. Das 18 `silk_overlap` do cerebro, quase todas
    # eram exactamente isso: `Reference field of D11` contra `Segment of D11`.
    #
    # ⚠️ E INCLUI O CONTORNO DA PROPRIA PECA, que e onde esta regra INVERTE em
    # relacao ao esquema. Num esquematico a referencia dentro do corpo do
    # proprio simbolo e onde ela mora (skill kicad §9b); numa placa e defeito,
    # porque as duas coisas saem na MESMA tinta branca e o texto deixa de se
    # ler. A mesma frase — "a referencia esta por cima do seu proprio corpo" —
    # e normal num sitio e defeito no outro.
    silk = []
    pistas = [tuple(map(float, m.groups())) for m in
              re.finditer(r"\(segment\s*\(start (" + NUM + r") (" + NUM
                          + r")\)\s*\(end (" + NUM + r") (" + NUM + r")\)", t)]
    pads, pecas = [], []
    for i, j, bl in blocos(t):
        a = re.search(r"\(at (" + NUM + r") (" + NUM + r")(?: (" + NUM + r"))?\)", bl)
        fx, fy = float(a.group(1)), float(a.group(2))
        ang = math.radians(float(a.group(3) or 0))
        r = re.search(r'\(property "Reference" "([^"]+)"', bl)
        ref = r.group(1) if r else "?"
        meus = []
        for pm in re.finditer(r'\(pad "[^"]*" \w+[\s\S]*?\(at (' + NUM + r") (" + NUM
                              + r")(?: " + NUM + r")?\)[\s\S]*?\(size (" + NUM + r") ("
                              + NUM + r")\)", bl):
            px, py, sw, sh = map(float, pm.groups())
            pads.append((fx + px * math.cos(ang) + py * math.sin(ang),
                         fy - px * math.sin(ang) + py * math.cos(ang),
                         max(sw, sh) / 2))
            meus.append(pads[-1])
        # ⚠️⚠️ A SERIGRAFIA E FEITA DE SEGMENTOS, NAO DE PONTOS.
        #
        # Ate a v0.5.1 isto guardava TRES PONTOS por segmento — inicio, meio e
        # fim — e o teste comparava distancias a esses pontos. Num contorno de
        # 41 mm, como o do `J1` do painel, tres amostras deixam vaos de 10 mm
        # onde o obstaculo e invisivel: a referencia do `C5` ficou a **0,11 mm**
        # da linha e a ferramenta deu-a por livre. O DRC do KiCad viu-a.
        #
        # ⚠️ E o teste certo estava TRES LINHAS ACIMA o tempo todo: as pistas ja
        # usavam distancia ponto-a-segmento. Mesma ferramenta, dois tipos de
        # obstaculo, e so um com a conta boa — a §11c desta casa dita para pads
        # e pistas, e a serigrafia ficou de fora da regra por nao ser cobre.
        #
        # ⚠️ E UM `fp_rect` NAO E UMA LINHA. O `(start ...)` e o `(end ...)` sao
        # cantos OPOSTOS: tratar isso como um segmento testa a DIAGONAL do
        # rectangulo e ignora as quatro arestas, que sao o obstaculo real.
        def _ab(px, py):
            return (fx + px * math.cos(ang) + py * math.sin(ang),
                    fy - px * math.sin(ang) + py * math.cos(ang))

        for sm in re.finditer(r'\(fp_(line|rect)\s*\(start (' + NUM + r') ('
                              + NUM + r')\)\s*\(end (' + NUM + r') (' + NUM
                              + r')\)[\s\S]{0,200}?\(layer "[FB]\.SilkS', bl):
            x1, y1 = float(sm.group(2)), float(sm.group(3))
            x2, y2 = float(sm.group(4)), float(sm.group(5))
            if sm.group(1) == "rect":
                cantos = [(x1, y1), (x2, y1), (x2, y2), (x1, y2)]
                pares = [(cantos[k], cantos[(k + 1) % 4]) for k in range(4)]
            else:
                pares = [((x1, y1), (x2, y2))]
            for (ax, ay), (bx, by) in pares:
                A, B = _ab(ax, ay), _ab(bx, by)
                silk.append((A[0], A[1], B[0], B[1]))
        for sm in re.finditer(r'\(fp_circle\s*\(center (' + NUM + r') (' + NUM
                              + r')\)\s*\(end (' + NUM + r') (' + NUM
                              + r')\)[\s\S]{0,200}?\(layer "[FB]\.SilkS', bl):
            cxx, cyy = float(sm.group(1)), float(sm.group(2))
            r_ = math.hypot(float(sm.group(3)) - cxx, float(sm.group(4)) - cyy)
            # ⚠️ o circulo entra como poligono de 12 lados: a conta de
            # ponto-a-segmento ja existe e nao vale a pena um caso especial que
            # depois ninguem se lembra de manter.
            pts = [_ab(cxx + r_ * math.cos(k * math.pi / 6),
                       cyy + r_ * math.sin(k * math.pi / 6)) for k in range(12)]
            for k in range(12):
                A, B = pts[k], pts[(k + 1) % 12]
                silk.append((A[0], A[1], B[0], B[1]))
        pecas.append((i, j, bl, ref, fx, fy, ang, meus))

    # ⚠️ o centro da placa serve para decidir "para fora": o texto foge do
    # miolo, que e onde estao os vizinhos.
    cx = sum(p[0] for p in pads) / len(pads)
    cy = sum(p[1] for p in pads) / len(pads)

    # ⚠️ E O CONTORNO DA PLACA E O ULTIMO OBSTACULO, e faltava.
    #
    # Com pads, pistas e serigrafia nos obstaculos, o texto foge para fora — e
    # no painel o `silk_edge_clearance` subiu de 4 para 6. E a terceira
    # variante da mesma troca: cada obstaculo que se acrescenta empurra o texto
    # para o sitio que ainda nao esta a ser verificado. So para quando todos os
    # sitios maus estao na lista.
    bordo = [tuple(map(float, m.groups())) for m in
             re.finditer(r'\(gr_line\s*\(start (' + NUM + r') (' + NUM + r')\)\s*'
                         r'\(end (' + NUM + r') (' + NUM + r')\)[\s\S]{0,140}?Edge\.Cuts',
                         t)]
    if bordo:
        _bx = [v for b in bordo for v in (b[0], b[2])]
        _by = [v for b in bordo for v in (b[1], b[3])]
        LIM = (min(_bx) + RAIO_TXT, min(_by) + RAIO_TXT,
               max(_bx) - RAIO_TXT, max(_by) - RAIO_TXT)
    else:
        LIM = None

    def livre(x, y, ignora, quem=None, so_cobre=False, caixa=(1.6, 1.6)):
        """`so_cobre` larga a serigrafia e os vizinhos, e guarda o COBRE.

        ⚠️ O recuo nao pode largar tudo de uma vez, porque os obstaculos nao
        custam o mesmo. Tocar num traco de serigrafia da duas linhas brancas
        cruzadas — le-se. Tocar num pad faz a fabrica RECORTAR o texto sobre a
        abertura de mascara: o que fica e meio designador, e meio designador e
        pior que nenhum, porque parece um.

        A v0.9.0 largava as duas juntas e comprou 6 `silk_over_copper` que nao
        precisava de comprar."""
        if LIM and not (LIM[0] <= x <= LIM[2] and LIM[1] <= y <= LIM[3]):
            return False
        # ⚠️ A ISENCAO DOS PROPRIOS PADS CAIU, e a medicao e que decidiu.
        #
        # Ela existia porque a referencia por cima do proprio pad e o caso
        # normal de um SMD pequeno. Mas assim que a serigrafia entrou nos
        # obstaculos, as referencias fugiram do contorno e foram parar aos
        # proprios pads: `silk_overlap` caiu de 18 para 5 e `silk_over_copper`
        # SUBIU de 6 para 12. Trocar um defeito por outro nao e arrumar.
        #
        # Sem a isencao, quem nao tiver sitio fica onde esta e diz — que ja era
        # o comportamento previsto, e e melhor que ser empurrado para pior.
        hw, hh = caixa
        for px, py, pr in pads:
            if abs(px - x) < pr + hw and abs(py - y) < pr + hh:
                return False
        # ⚠️ E AS PISTAS TAMBEM, que e a mesma regra do §11c: todo teste de
        # espaco olha pads E pistas. Depois de rotear, o cobre esta em todo o
        # lado, e um teste que so ve pads acha livre o que nao esta.
        for a1, b1, a2, b2 in pistas:
            dx, dy = a2 - a1, b2 - b1
            L2 = dx * dx + dy * dy
            u = 0.0 if L2 == 0 else max(0.0, min(1.0, ((x - a1) * dx + (y - b1) * dy) / L2))
            if math.hypot(x - (a1 + u * dx), y - (b1 + u * dy)) < RAIO_TXT * 0.8:
                return False
        if so_cobre:
            return True
        for _r, (tx, ty) in postos.items():
            if _r == quem:
                continue      # ⚠️ uma referencia nao colide consigo propria
            if abs(tx - x) < 3.0 and abs(ty - y) < 2.0:
                return False
        # ⚠️ SEM `ignora` AQUI. Nos pads ignoram-se os da propria peca, porque a
        # referencia por cima do seu proprio pad e o caso normal de um SMD
        # pequeno. Na serigrafia nao ha excepcao: o contorno da propria peca
        # conta como qualquer outro, e era ele o culpado da maioria.
        # ⚠️ a MESMA conta das pistas, que e a unica correcta para um segmento.
        for a1, b1, a2, b2 in silk:
            dx, dy = a2 - a1, b2 - b1
            L2 = dx * dx + dy * dy
            u = 0.0 if L2 == 0 else max(0.0, min(1.0, ((x - a1) * dx + (y - b1) * dy) / L2))
            qx, qy = a1 + u * dx, b1 + u * dy
            if abs(qx - x) < caixa[0] and abs(qy - y) < caixa[1]:
                return False
        return True

    # ⚠️⚠️ TODA REFERENCIA E OBSTACULO DESDE O INICIO, tenha-se movido ou nao.
    #
    # Ate a v0.6.0 o `postos` enchia-se A MEDIDA que o laco andava: quem ainda
    # nao tinha sido visitado era INVISIVEL para quem estava a ser colocado. E
    # a ordem do laco e a ordem no FICHEIRO, que nao tem relacao nenhuma com a
    # geometria da placa.
    #
    # O resultado medido: o `RV1` foi visitado primeiro, nao viu o `R21`, e
    # mudou-se para cima dele. Depois o `R21` foi visitado, ja nao tinha sitio
    # nenhum, e o recuo — "fica onde estava" — devolveu-o para dentro do `RV1`.
    # A ferramenta anunciou "5 movidas, 6 sem sitio" e tinha CRIADO uma
    # sobreposicao texto×texto onde nao havia nenhuma.
    #
    # ⚠️ E o recuo e que a torna traicoeira: "fica onde estava" soa a nao ter
    # feito nada, e nao e — e reafirmar uma posicao que entretanto deixou de
    # estar livre, por causa de uma mudanca da propria ferramenta.
    #
    # Com todas as posicoes conhecidas a partida, a ordem deixa de importar.
    # ⚠️ centro de CADA peca, para saber de quem e o texto mais proximo
    centros = [(_r, _fx, _fy) for _i, _j, _bl, _r, _fx, _fy, _a, _m in pecas]

    postos = {}
    for _i, _j, _bl, _ref, _fx, _fy, _ang, _meus in pecas:
        _m = re.search(r'\(property "Reference" "[^"]+"\s*\n\s*\(at (' + NUM
                       + r") (" + NUM + r")( " + NUM + r")?\)", _bl)
        if _m:
            postos[_ref] = abs_do_campo(_fx, _fy, _ang,
                                        float(_m.group(1)), float(_m.group(2)))
    movidos, presos, apertados = [], [], []
    novo = t
    # ⚠️ de tras para a frente: cada substituicao muda os indices dos blocos
    # seguintes, e mexer do fim para o inicio nao os invalida.
    for i, j, bl, ref, fx, fy, ang, meus in sorted(pecas, key=lambda z: -z[0]):
        m = re.search(r'\(property "Reference" "[^"]+"\s*\n\s*\(at (' + NUM + r") ("
                      + NUM + r")( " + NUM + r")?\)", bl)
        if not m:
            continue
        dx, dy = float(m.group(1)), float(m.group(2))
        # ⚠️⚠️ O `(at ...)` DO CAMPO E RELATIVO AO FOOTPRINT, E RODA COM ELE.
        #
        # Ate a v0.4.0 isto fazia `fx + dx, fy + dy` — somava um offset do
        # espaco da PECA a uma coordenada do espaco da PLACA. Nas pecas a 0°
        # dava certo por acidente, e nas rodadas punha o texto num sitio que
        # nao existe: o `U2` (Daisy, 180°) foi avaliado em y=112,66 quando esta
        # em y=105,46 — **7,2 mm de distancia, do outro lado da peca**.
        #
        # ⚠️ E o modo de falhar e o pior que ha: la nao havia nada, logo a
        # ferramenta disse LIVRE e deu a referencia por arrumada. O DRC do
        # KiCad, esse, viu `silk_over_copper` entre o campo do U2 e o pad 2.
        # Uma ferramenta que mede no sitio errado nao reprova nada — aprova
        # tudo, e com ar de quem verificou.
        #
        # Os pads ja usavam esta transformacao (ver acima); so o campo e que
        # nao. Duas contas para a mesma mudanca de espaco, e so uma foi feita.
        cx_txt = caixa_txt(ref, 1.0)
        rx, ry = abs_do_campo(fx, fy, ang, dx, dy)

        # ⚠️⚠️ NAO CHEGA NAO COLIDIR: A REFERENCIA TEM DE APONTAR PARA A PECA.
        #
        # Ate a v0.7.0 isto so mexia no que COLIDIA. Uma referencia a 11,5 mm da
        # propria peca nao colide com nada — e fica la, livre e inutil, a
        # apontar para a vizinha. Na placa medida, 14 das 70 estavam mais perto
        # de OUTRA peca do que da sua, e a ferramenta dava tudo por arrumado.
        #
        # A §11g-ter ja tinha previsto exactamente isto: "uma referencia
        # tecnicamente livre pode estar encostada a peca errada, e nenhum DRC
        # tem opiniao sobre isso". Estava escrito e nao estava implementado.
        #
        # ⚠️ O que muda e a NATUREZA do problema: nao colidir e uma RESTRICAO,
        # ficar perto da propria peca e um OBJECTIVO. Uma restricao satisfaz-se
        # com o primeiro candidato que passa; um objectivo obriga a ver todos e
        # a escolher. Foi por tratar isto como restricao que o primeiro
        # candidato "para fora da placa" ganhava sempre.
        cands = [(dx, dy, rx, ry)]
        for d in PASSOS + PASSOS_LONGE:
            for ex, ey in ((0, d), (0, -d), (d, 0), (-d, 0),
                           (d, d), (d, -d), (-d, d), (-d, -d)):
                cands.append((None, None, fx + ex, fy + ey))

        melhor = None
        for cdx, cdy, X, Y in cands:
            if not livre(X, Y, set(meus), ref, caixa=cx_txt):
                continue
            dist = math.hypot(X - fx, Y - fy)
            if dist > TECTO_REF:
                continue
            # ⚠️ E O CRITERIO DECISIVO: a peca mais proxima do texto tem de ser
            # a DONA dele. Uma referencia a 3 mm da sua peca mas a 2 mm da
            # vizinha identifica a peca errada — e isso nao e um aviso
            # estetico, e uma instrucao errada para quem solda a mao.
            dono = min(((math.hypot(X - ox, Y - oy), oref)
                        for oref, ox, oy in centros), key=lambda z: z[0])[1]
            if dono != ref:
                continue
            if melhor is None or dist < melhor[0]:
                melhor = (dist, cdx, cdy, X, Y)

        if melhor is None:
            # ⚠️⚠️ RECUO GRADUADO, e a ordem das preferencias e uma DECISAO.
            #
            # Nao havendo sitio limpo, o que fazer? A v0.8.0 deixava ficar onde
            # estava — e onde estava eram os `-11,5 mm` que o `sincroniza_pcb`
            # poe em toda peca nova: limpo, longe, e a apontar para a vizinha.
            #
            # Uma referencia encostada a peca CERTA mas a tocar num traco
            # LE-SE. Uma referencia limpa a 11,5 mm identifica outra peca, e
            # isso nao e feiura — e uma instrucao errada para quem solda a mao.
            # Entre as duas, escolhe-se a legivel.
            #
            # Por isso a segunda volta larga a exigencia de estar livre e
            # mantem as duas que importam: o tecto e o dono. E diz-se quantas
            # sairam assim, que e o preco a ser pago com os olhos abertos.
            perto = None
            # ⚠️ TRES degraus, e a ordem e a escala do estrago:
            #    1) limpo de cobre, perto        2) limpo de cobre, mais longe
            #    3) qualquer coisa — ultimo recurso, e conta-se
            for etapa, tecto in ((True, TECTO_REF), (True, TECTO_RECUO),
                                 (False, TECTO_RECUO)):
                for cdx, cdy, X, Y in cands:
                    dist = math.hypot(X - fx, Y - fy)
                    if dist > tecto:
                        continue
                    if etapa and not livre(X, Y, set(meus), ref, so_cobre=True,
                                       caixa=cx_txt):
                        continue
                    dono = min(((math.hypot(X - ox, Y - oy), oref)
                                for oref, ox, oy in centros), key=lambda z: z[0])[1]
                    if dono != ref:
                        continue
                    if perto is None or dist < perto[0]:
                        perto = (dist, cdx, cdy, X, Y)
                if perto is not None:
                    break
            if perto is None:
                presos.append(ref)      # fica onde estava; ja esta em `postos`
                continue
            melhor = perto
            apertados.append(ref)
        _, cdx, cdy, X, Y = melhor
        if cdx is not None:             # o melhor e onde ja estava
            postos[ref] = (X, Y)
            continue
        # ⚠️ E A VOLTA TAMBEM: os candidatos sao deltas no espaco da PLACA, e o
        # campo grava-se no espaco da PECA. Escrever o delta cru fazia o KiCad
        # roda-lo outra vez ao desenhar — a peca acabava com o texto num
        # terceiro sitio, nem onde estava nem onde a ferramenta o pos.
        pdx, pdy = campo_do_abs(ang, X - fx, Y - fy)
        a_, b_ = m.span(1)[0], m.span(2)[1]
        novo = novo[:i + a_] + ("%s %s" % (num(pdx), num(pdy))) + novo[i + b_:]
        postos[ref] = (X, Y)
        movidos.append((ref, (dx, dy), (float(num(pdx)), float(num(pdy)))))

    # ⚠️ AUTO-TESTE: a ferramenta nao termina tendo CRIADO uma sobreposicao.
    # Foi assim que o par R21/RV1 apareceu sem ninguem dar por ele — o relatorio
    # dizia "5 movidas, 6 sem sitio", que soa a trabalho feito.
    maus = []
    _it = sorted(postos.items())
    for _a in range(len(_it)):
        for _b in range(_a + 1, len(_it)):
            (r1, (x1, y1)), (r2, (x2, y2)) = _it[_a], _it[_b]
            if abs(x1 - x2) < 3.0 and abs(y1 - y2) < 2.0:
                maus.append("%s~%s" % (r1, r2))
    if maus:
        print("*** ficaram referencias sobrepostas: %s\n"
              "    Nada gravado — mover texto nao pode criar o defeito que vem "
              "arrumar." % " ".join(maus), file=sys.stderr)
        return 1

    print("  %d pecas · %d referencias movidas · %d sem sitio livre"
          % (len(pecas), len(movidos), len(presos)), file=sys.stderr)
    for ref, de, para in sorted(movidos):
        print("     %-5s (%+g,%+g) -> (%+g,%+g)" % (ref, de[0], de[1], para[0], para[1]),
              file=sys.stderr)
    if apertados:
        print("  %d aproximada(s) da propria peca ACEITANDO sobreposicao: %s\n"
              "     (legivel encostado a peca certa > limpo a apontar para a "
              "errada)" % (len(apertados), " ".join(sorted(apertados))),
              file=sys.stderr)
    if presos:
        print("  ⚠️ sem sitio: %s — ficam onde estavam." % " ".join(sorted(presos)),
              file=sys.stderr)
    if escreve:
        io.open(p, "w", encoding="utf-8").write(novo)
        print("  gravado.", file=sys.stderr)
    else:
        print("  ensaio: nada gravado. Junta --escreve para aplicar.", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
