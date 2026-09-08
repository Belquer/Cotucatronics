#!/usr/bin/env python3
"""arruma_silk v0.19.0 — arruma os textos das DUAS camadas.

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
import io, math, os, re, subprocess, sys

KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"

VERSAO = "0.19.0"
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


def mede_camada(caminho, cam, tmp):
    """(texto×texto, total) da camada `cam`, pelo PORTAO — nao pela minha regua.

    ⚠️ O auto-teste interno usa o modelo de caixa desta ferramenta; o
    `svg_colisoes` usa o dele (0,55 em por caractere). O meu e o mais
    permissivo, e por isso aprovou um arranjo que o portao reprova. Quem julga
    e quem mede."""
    s = os.path.join(tmp, "%s_%s.svg" % (os.path.basename(caminho), cam))
    subprocess.run([KICAD, "pcb", "export", "svg", "-o", s, "--layers", cam,
                    "--page-size-mode", "2", "--exclude-drawing-sheet", caminho],
                   capture_output=True, text=True)
    v = subprocess.run(
        [sys.executable,
         os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                      "tools", "svg_colisoes.py"), s],
        capture_output=True, text=True)
    n = re.search(r"(\d+) colisoes", v.stdout)
    return (v.stdout.count("texto×texto"), int(n.group(1)) if n else 0)


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
    # ⚠️ OS CONTORNOS SEPARAM-SE POR CAMADA. A F.Fab tem os seus proprios
    # desenhos de peca, e um `Value` posto por cima de um deles conta como
    # colisao no portao tal como na serigrafia. A v0.15.0 so evitava OUTROS
    # TEXTOS na Fab, e por isso "arrumar" empurrava os valores para cima dos
    # contornos: o painel foi de 51 para 68 colisoes.
    contornos = {"F.SilkS": [], "F.Fab": []}
    silk = contornos["F.SilkS"]
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
                              + r')\)[\s\S]{0,200}?\(layer "([FB]\.(?:SilkS|Fab))', bl):
            _lay = "F.Fab" if sm.group(6).endswith("Fab") else "F.SilkS"
            x1, y1 = float(sm.group(2)), float(sm.group(3))
            x2, y2 = float(sm.group(4)), float(sm.group(5))
            if sm.group(1) == "rect":
                cantos = [(x1, y1), (x2, y1), (x2, y2), (x1, y2)]
                pares = [(cantos[k], cantos[(k + 1) % 4]) for k in range(4)]
            else:
                pares = [((x1, y1), (x2, y2))]
            for (ax, ay), (bx, by) in pares:
                A, B = _ab(ax, ay), _ab(bx, by)
                contornos[_lay].append((A[0], A[1], B[0], B[1]))
        for sm in re.finditer(r'\(fp_circle\s*\(center (' + NUM + r') (' + NUM
                              + r')\)\s*\(end (' + NUM + r') (' + NUM
                              + r')\)[\s\S]{0,200}?\(layer "([FB]\.(?:SilkS|Fab))', bl):
            _lay = "F.Fab" if sm.group(5).endswith("Fab") else "F.SilkS"
            cxx, cyy = float(sm.group(1)), float(sm.group(2))
            r_ = math.hypot(float(sm.group(3)) - cxx, float(sm.group(4)) - cyy)
            # ⚠️ o circulo entra como poligono de 12 lados: a conta de
            # ponto-a-segmento ja existe e nao vale a pena um caso especial que
            # depois ninguem se lembra de manter.
            pts = [_ab(cxx + r_ * math.cos(k * math.pi / 6),
                       cyy + r_ * math.sin(k * math.pi / 6)) for k in range(12)]
            for k in range(12):
                A, B = pts[k], pts[(k + 1) % 12]
                contornos[_lay].append((A[0], A[1], B[0], B[1]))
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

    def livre(x, y, ignora, quem=None, so_cobre=False, caixa=(1.6, 1.6),
              so_texto=False):
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
        if so_texto:
            # ⚠️ A F.Fab NAO E IMPRESSA: pads e pistas ali nao custam nada. Mas
            # os CONTORNOS da propria camada custam, porque e o que se le.
            # ⚠️ A F.Fab NAO E IMPRESSA. Passar por cima de um pad ou de uma
            # pista ali nao custa nada — o que custa e texto por cima de texto,
            # porque essa camada e a que se LE no editor enquanto se trabalha.
            # Aplicar-lhe as regras da serigrafia seria recusar posicoes boas
            # por um defeito que nao existe naquela camada.
            for _r, (tx, ty, thw, thh) in postos.items():
                if _r == quem:
                    continue
                if abs(tx - x) < thw + caixa[0] and abs(ty - y) < thh + caixa[1]:
                    return False
            for a1, b1, a2, b2 in contornos["F.Fab"]:
                dx, dy = a2 - a1, b2 - b1
                L2 = dx * dx + dy * dy
                u = 0.0 if L2 == 0 else max(0.0, min(1.0,
                    ((x - a1) * dx + (y - b1) * dy) / L2))
                qx, qy = a1 + u * dx, b1 + u * dy
                if abs(qx - x) < caixa[0] and abs(qy - y) < caixa[1]:
                    return False
            return True
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
        for _r, (tx, ty, thw, thh) in postos.items():
            if _r == quem:
                continue      # ⚠️ uma referencia nao colide consigo propria
            if abs(tx - x) < thw + caixa[0] and abs(ty - y) < thh + caixa[1]:
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

    # ⚠️⚠️ DUAS PASSAGENS, UMA POR CAMADA — e as regras nao sao as mesmas.
    #
    # A `Reference` vive na F.SilkS e E IMPRESSA: obstaculos sao pads, pistas,
    # serigrafia e vizinhos. A `Value` vive na F.Fab, que NAO e impressa: ali o
    # unico defeito e texto por cima de texto, porque aquela camada existe para
    # se LER no editor enquanto se trabalha.
    #
    # ⚠️ Foi por so olhar a serigrafia que a placa parecia arrumada e o dB
    # continuava a ver desordem: a vista do editor mostra as duas, e a F.Fab
    # tinha 114 colisoes contra 6 da serigrafia. **Eu media a camada que sai da
    # fabrica; ele olhava a camada em que trabalha.** As duas contam, por
    # razoes diferentes, e nenhuma das duas e a outra.
    novo = t
    relatorio = []
    # ⚠️⚠️ A PASSAGEM DA F.Fab ESTA DESLIGADA POR OMISSAO, E ISSO E UM RESULTADO.
    #
    # A ideia era boa: a F.Fab nao e impressa, mas e a camada que se LE no
    # editor, e tinha 114 colisoes contra 6 da serigrafia. Implementada e
    # medida, deu isto:
    #
    #     cerebro  Fab  114 -> 105   texto×texto 29 -> 16   melhorou
    #     painel   Fab   51 ->  68   texto×texto 10 -> 13   PIOROU
    #     painel   SILK   6 ->   7   texto×texto  0 ->  1   PIOROU, e e impressa
    #
    # ⚠️ Duas de quatro pioraram, e uma delas na camada que sai da fabrica. Nao
    # se troca uma regressao no que se fabrica por uma melhoria no que se le.
    #
    # ⚠️ E o auto-teste NAO a apanhou, o que e o achado mais util daqui: ele usa
    # o meu modelo de caixa e o portao usa o do `svg_colisoes` (0,55 em por
    # caractere). Dois modelos do mesmo texto, e o meu e o mais permissivo — por
    # isso a minha guarda aprovou o que o portao reprovou. **Uma guarda que mede
    # com regua propria nao guarda contra o criterio de quem julga.**
    #
    # Fica com `--fab` para se continuar a trabalhar nela, e fora do caminho
    # ate as quatro linhas descerem.
    passagens = [("Reference", "F.SilkS", False)]
    if "--fab" in sys.argv:
        passagens.append(("Value", "F.Fab", True))
    # ⚠️⚠️ CADA PASSAGEM E JULGADA E ACEITE POR SI.
    #
    # A v0.18.0 acumulava as duas e submetia o conjunto ao portao. No painel a
    # passagem da Fab era optima — 51 para 37 colisoes, texto×texto 10 para 3 —
    # e foi recusada em bloco porque a passagem da SERIGRAFIA piorava a
    # serigrafia. Uma boa ficou refem de uma ma por estarem no mesmo pacote.
    #
    # Agora cada uma parte do estado ja aceite, e e medida na SUA camada: se
    # nao piorar, entra; se piorar, cai sozinha e a outra segue.
    import tempfile as _tf
    _tmp = _tf.mkdtemp()
    aceites, recusadas = [], []
    for PROP, CAMADA, SO_TXT in passagens:
        cand = novo
        # ⚠️⚠️ OS DESLOCAMENTOS SAO RECALCULADOS SOBRE O TEXTO JA MODIFICADO.
        #
        # O laco escreve por posicao absoluta — `cand[:i+a] + ... + cand[i+b:]`
        # — e a primeira passagem MUDA O COMPRIMENTO do texto. A segunda, a
        # correr com os `i` da leitura original, escrevia deslocada: no painel
        # comeu o espaco de um `(at 0 1.65 0)` e deixou `(at0 1.65 0)`, e o
        # KiCad recusou a placa inteira.
        #
        # ⚠️ O ficheiro ja avisava disto tres linhas abaixo — "cada substituicao
        # muda os indices dos blocos seguintes" — e a regra estava aplicada
        # DENTRO de uma passagem (por isso se anda de tras para a frente) e nao
        # ENTRE passagens. Acrescentar a segunda passagem quebrou uma invariante
        # que estava escrita e que eu li como sendo sobre outra coisa.
        pecas = []
        for _i, _j, _bl in blocos(cand):
            _a = re.search(r"\(at (" + NUM + r") (" + NUM + r")(?: (" + NUM + r"))?\)", _bl)
            _r = re.search(r'\(property "Reference" "([^"]+)"', _bl)
            if not _a or not _r:
                continue
            pecas.append((_i, _j, _bl, _r.group(1), float(_a.group(1)),
                          float(_a.group(2)),
                          math.radians(float(_a.group(3) or 0)), []))
        postos = {}
        for _i, _j, _bl, _ref, _fx, _fy, _ang, _meus in pecas:
            _m = re.search(r'\(property "%s" "([^"]*)"\s*\n\s*\(at (' % PROP + NUM
                           + r") (" + NUM + r")( " + NUM + r")?\)", _bl)
            if _m:
                _x, _y = abs_do_campo(_fx, _fy, _ang,
                                      float(_m.group(2)), float(_m.group(3)))
                # ⚠️ a CAIXA vai junto: um `V_TRILHO_A2` ocupa ~8 mm e um `R9`
                # ocupa ~1 mm. Guardar so o ponto obriga o teste a inventar um
                # tamanho — e ele inventou uma caixa fixa de 3x2, que aprovava
                # dois valores longos a 4 mm um do outro.
                postos[_ref] = (_x, _y) + caixa_txt(_m.group(1))
        movidos, presos, apertados = [], [], []
    # ⚠️ de tras para a frente: cada substituicao muda os indices dos blocos
    # seguintes, e mexer do fim para o inicio nao os invalida.
        for i, j, bl, ref, fx, fy, ang, meus in sorted(pecas, key=lambda z: -z[0]):
            m = re.search(r'\(property "%s" "([^"]*)"\s*\n\s*\(at (' % PROP + NUM
                          + r") (" + NUM + r")( " + NUM + r")?\)", bl)
            if not m:
                continue
            dx, dy = float(m.group(2)), float(m.group(3))
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
            # ⚠️ a caixa e do TEXTO que se move: `R9` na serigrafia, `2u2 filme`
            # na Fab. Usar sempre a referencia subestima os valores longos, que sao
            # exactamente os que colidem.
            cx_txt = caixa_txt(m.group(1) or ref, 1.0)
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
                if not livre(X, Y, set(meus), ref, caixa=cx_txt, so_texto=SO_TXT):
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
                                               caixa=cx_txt, so_texto=SO_TXT):
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
                postos[ref] = (X, Y) + cx_txt
                continue
            # ⚠️ E A VOLTA TAMBEM: os candidatos sao deltas no espaco da PLACA, e o
            # campo grava-se no espaco da PECA. Escrever o delta cru fazia o KiCad
            # roda-lo outra vez ao desenhar — a peca acabava com o texto num
            # terceiro sitio, nem onde estava nem onde a ferramenta o pos.
            pdx, pdy = campo_do_abs(ang, X - fx, Y - fy)
            a_, b_ = m.span(2)[0], m.span(3)[1]
            cand = cand[:i + a_] + ("%s %s" % (num(pdx), num(pdy))) + cand[i + b_:]
            postos[ref] = (X, Y) + cx_txt
            movidos.append((ref, (dx, dy), (float(num(pdx)), float(num(pdy)))))
        # ⚠️ AUTO-TESTE, POR CAMADA: a ferramenta nao termina tendo CRIADO uma
        # sobreposicao. Foi assim que o par R21/RV1 apareceu sem ninguem dar por
        # ele — o relatorio dizia "5 movidas, 6 sem sitio", que soa a trabalho
        # feito.
        maus = []
        _it = sorted(postos.items())
        for _a in range(len(_it)):
            for _b in range(_a + 1, len(_it)):
                (r1, (x1, y1, w1, h1)), (r2, (x2, y2, w2, h2)) = _it[_a], _it[_b]
                if abs(x1 - x2) < w1 + w2 and abs(y1 - y2) < h1 + h2:
                    maus.append("%s~%s" % (r1, r2))
        if maus:
            print("*** %s/%s: ficaram textos sobrepostos: %s\n"
                  "    Nada gravado — mover texto nao pode criar o defeito que "
                  "vem arrumar." % (PROP, CAMADA, " ".join(maus)), file=sys.stderr)
            return 1
        # ⚠️ o veredicto e do portao, na camada desta passagem
        _cam = "F.Silkscreen" if CAMADA == "F.SilkS" else "F.Fab"
        _f = os.path.join(_tmp, "%s_%s.kicad_pcb" % (PROP, os.path.basename(p)))
        io.open(_f, "w", encoding="utf-8").write(cand)
        _r = subprocess.run([KICAD, "pcb", "drc", "-o",
                             os.path.join(_tmp, "x.rpt"), _f],
                            capture_output=True, text=True)
        if "Failed to load" in (_r.stdout + _r.stderr):
            # ⚠️ e ANTES de medir, o resultado tem de ABRIR. Uma escrita com
            # deslocamentos desactualizados deixou `(at0 1.65 0)` e o KiCad
            # recusou a placa inteira — e so o comando SEGUINTE o revelou.
            recusadas.append((PROP, _cam, "nao abre no KiCad"))
            continue
        _a = mede_camada(p, _cam, _tmp)
        _b = mede_camada(_f, _cam, _tmp)
        _txt = "texto×texto %d->%d · total %d->%d" % (_a[0], _b[0], _a[1], _b[1])
        if _b[0] > _a[0] or _b[1] > _a[1]:
            recusadas.append((PROP, _cam, _txt))
            continue
        novo = cand
        aceites.append((PROP, _cam, _txt))
        relatorio.append((PROP, CAMADA, list(movidos), list(presos), list(apertados)))

    for PROP, CAMADA, movidos, presos, apertados in relatorio:
        print("  %-9s (%s): %d movida(s) · %d sem sitio"
              % (PROP, CAMADA, len(movidos), len(presos)), file=sys.stderr)
        for ref, de, para in sorted(movidos):
            print("     %-5s (%+g,%+g) -> (%+g,%+g)"
                  % (ref, de[0], de[1], para[0], para[1]), file=sys.stderr)
        if apertados:
            print("     %d aproximada(s) da propria peca ACEITANDO sobreposicao: "
                  "%s" % (len(apertados), " ".join(sorted(apertados))),
                  file=sys.stderr)
        if presos:
            print("     ⚠️ sem sitio: %s — ficam onde estavam."
                  % " ".join(sorted(presos)), file=sys.stderr)
    for PROP, cam, txt in aceites:
        print("  ✓ %-9s (%s) aceite · %s" % (PROP, cam, txt), file=sys.stderr)
    for PROP, cam, txt in recusadas:
        print("  ✗ %-9s (%s) RECUSADA · %s" % (PROP, cam, txt), file=sys.stderr)
    if not aceites:
        print("  nada a gravar: nenhuma passagem melhorou.", file=sys.stderr)
        return 0
    if escreve:
        # ⚠️ `novo` so contem passagens que o PORTAO aprovou, uma a uma, e cada
        # uma ja foi carregada pelo KiCad. Nao ha nada por conferir aqui.
        io.open(p, "w", encoding="utf-8").write(novo)
        print("  gravado.", file=sys.stderr)
    else:
        print("  ensaio: nada gravado. Junta --escreve para aplicar.", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
