#!/usr/bin/env python3
"""roteia — as duas camadas, com via, e a prova de que nao partiu o plano.

⚠️ EXIGE QUE F.Cu NAO TENHA ZONA DE COBRE, e recusa-se a correr se tiver.

O ARRANJO QUE ESTE ROTEADOR PRESSUPOE (e que o `pcb_contorno.costura()` cria):

    B.Cu   plano solido de terra, inteiro, e o sinal que precisar de passar
    F.Cu   sinal, e nenhuma zona
           + uma via em cada pad SMD de terra a descer ao plano

POR QUE DUAS CAMADAS E NAO UMA.

A versao anterior so escrevia em F.Cu, e parou em 47 ligacoes por fazer num
total de 71. Nao foi falta de esperteza da busca: numa camada so, um sinal que
precise de atravessar outro nao tem por onde. **Uma camada de sinal e um grafo
planar, e as ligacoes de uma placa nao sao um grafo planar.** A saida e a
mesma da industria ha quarenta anos — salta-se para o outro lado com duas
vias, anda-se um bocado, e volta-se.

⚠️⚠️ E SALTAR PARA B.Cu ABRE UMA FENDA NO PLANO. O cobre recua dos dois lados
da pista, e o plano fica com um rasgo. Um rasgo curto e inofensivo; um rasgo
que separe o plano em duas ilhas destroi a terra da placa, e o DRC nao chama a
isso um erro de geometria — chama-lhe `Zone [GND] <-> Zone [GND]`, escondido
no meio das ligacoes por fazer. Foi assim que uma tentativa anterior baixou o
total de 71 para 52 **enquanto partia a ilha de AGND em oito pedacos**, com o
DRC a dizer geometria zero o tempo todo (15co).

Por isso a via e CARA no custo da busca — ela prefere dar a volta em F.Cu a
saltar — e por isso existe a retraccao por bisseccao no fim: escreve-se tudo,
mede-se, e se algum plano se partiu procura-se **qual** das travessias o
partiu, devolvendo essas a lista da pessoa. O plano inteiro vale mais que
qualquer ligacao automatica.

⚠️ A FERRAMENTA CONFERE O PROPRIO TRABALHO, e nao entrega o que nao consegue
provar. Entregar cobre que o DRC acusa e pior que nao entregar nada.

⚠️ E PREFERE A LINHA RECTA. O custo penaliza cada mudanca de direccao, senao a
busca devolve escadas de mil degraus, electricamente validas e humanamente
ilegiveis — e uma placa que ninguem le nao se depura.

⚠️ VERIFICA-SE COM AS ZONAS CHEIAS (`--refill-zones`). Sem isso o DRC compara
as pistas novas com o enchimento ANTERIOR e devolve dezenas de `clearance`
fantasma. E antes de dizeres que o `kicad-cli` nao faz X, corre o `--help`:
escrevi num commit que ele nao sabia encher zonas, e a flag chamava-se
exactamente aquilo.

USO:  roteia.py --auto-teste            # interroga a regra de aceitacao
      roteia.py <ficheiro.kicad_pcb> [--escreve] [--rede NOME]
      [--so-uma-face] [--folga MM] [--passo MM]
      [--caixa MM] [--tecto N]
      [--guarda-plano] [--guiado] [--arranca]
"""
import io, json, math, os, re, subprocess, sys, uuid

VERSAO = "1.17.0"
NUM = r"-?\d+(?:\.\d+)?"
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
# ⚠️ A FOLGA DO ROTEADOR NAO PODE SER MAIS APERTADA QUE A REGRA DA PLACA, mas
# tambem nao ha razao para ser MUITO mais larga. Estive em 0,35 mm com a regra
# da placa em 0,20 — quase o dobro — e paguei isso em caminhos que nao existiam.
# 0,26 deixa 30% de margem sobre a regra, que chega para o erro de discretizacao
# da grelha, e abriu caminhos que a 0,35 estavam fechados.
# ⚠️ APERTAR A FOLGA PIOROU O RESULTADO, e isso e contra-intuitivo o bastante
# para ficar escrito. Medido, sempre de raiz e com duas passadas:
#
#     FOLGA 0,35 · PASSO 0,25  ->  23 ligacoes por fazer   <- fica esta
#     FOLGA 0,26 · PASSO 0,20  ->  44
#     FOLGA 0,26 · PASSO 0,25  ->  34
#
# A razao e a ganancia: com mais espaco disponivel o roteador enfia mais pistas
# cedo, e essas pistas fecham corredores de que as ligacoes seguintes
# precisavam — e mais delas acabam retiradas por estragarem o plano. Um
# roteador guloso melhora quando lhe apertamos o apetite, nao a folga.
#
# ⚠️ E a malha mais fina nao acha mais caminhos: acha os mesmos mais devagar, e
# bate no tecto de nos antes de chegar ao fim.
#
# ⚠️ Da primeira vez mudei as duas de uma so vez e a medicao nao soube dizer
# qual delas tinha feito o estrago. Uma variavel de cada vez, mesmo quando
# parece obvio que andam juntas.
FOLGA = 0.35            # entre a pista e o que nao e dela (regra da placa: 0,20)
PASSO = 0.25            # mm por celula

# ⚠️⚠️ E POR ISSO EXISTE A PASSADA APERTADA, no fim e so sobre o que sobrou.
#
# Com a folga larga o roteador NAO CONSEGUE SAIR DE ENTRE OS PINOS DE UM
# CONECTOR, e isso nao e falta de espaco na placa — e aritmetica. Num header de
# 2,54 mm com pads de 1,70, o vao entre dois pads vizinhos e 0,840 mm. Medido no
# J14, o cabo plano de 26 pinos.
#
#   corredor preciso = largura/2 + folga + margem-da-grelha, de cada lado
#   com folga 0,35 e passo 0,25:  0,125 + 0,35 + 0,178 = 0,653  -> 1,306 > 1,27  NAO CABE
#   com folga 0,20 e passo 0,127: 0,125 + 0,20 + 0,090 = 0,415  -> 0,830 < 1,27  CABE
#
# (1,27 e a distancia do centro do corredor ao centro de cada pad.)
#
# Foi isto que fechou o cerebro em 22 e o painel em 10: 9 falhas no J14, 8 no
# J1, 11 na Daisy — todas peças de duas filas, e nenhuma delas com pistas a
# volta. Um sitio com zero pistas num raio de 8 mm nao esta congestionado: esta
# fechado por uma regra minha.
#
# ⚠️ E APERTA-SE SO NO FIM. Apertar desde o inicio mede-se PIOR (23 -> 34): com
# mais espaco o roteador ganancioso enfia pistas cedo e fecha corredores de que
# as ligacoes seguintes precisavam. No fim nao ha esse risco — o que ja foi
# roteado ja la esta, e a folga apertada so serve para entrar onde a larga nao
# entrava. O DRC valida na mesma, e a retraccao continua a existir.
#
# ⚠️ A margem da grelha e `PASSO * 0.71` — meia diagonal — porque o A* so testa
# CENTROS de celula. Apertar a malha aperta essa margem: e por isso que a
# passada apertada tambem muda o passo, e nao so a folga.
VIRAGEM = 3.0           # o que custa dobrar, em celulas
# ⚠️ O TECTO CONTROLA O TEMPO E TAMBEM A QUALIDADE, e nao no sentido esperado.
# Medido de raiz: 300k -> 19 ligacoes por fazer · 500k -> 22, e mais lento. Um
# tecto maior nao acha mais caminhos: deixa o roteador guloso teimar em ligacoes
# dificeis, gastar minutos nelas e fechar corredores de que as seguintes
# precisavam. Quem paga o tecto sao as buscas SEM solucao.
FOLGA_FUGA = 0.22       # a folga apertada, so nos primeiros mm a volta do pad
RAIO_FUGA = 3.0         # ate onde vale a fuga apertada
TECTO = 300000          # nos que uma busca pode gastar antes de desistir
CUSTO_VIA = 30.0        # ⚠️ em celulas. Alto de proposito: a via abre fenda no
                        # plano, portanto so se salta quando nao ha volta a dar.
VIA_R = 0.3             # raio do anel da via
CAMADAS = ("F.Cu", "B.Cu")
LARGURA = {"+15V": 1.0, "Net-(F1-Pad1)": 1.0, "V_TRILHO_A2": 1.0,
           "V+PRE": 0.5, "VBIAS": 0.5, "+5V": 0.5, "+5V_PONTE": 0.5, "+3V3A": 0.5}


# --- leitura da placa -------------------------------------------------------

def pads_da_placa(t):
    """(x, y, raio, rede, camadas) — e `camadas` importa: um pad SMD so existe
    de um lado, e uma pista do outro lado passa-lhe por baixo sem lhe tocar.
    Tratar todos os pads como obstaculo nas duas camadas custa metade dos
    caminhos por nada."""
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
                    bl = t[j:k + 1]
                    break
        a = re.search(r"\(at (" + NUM + r") (" + NUM + r")(?: (" + NUM + r"))?\)", bl)
        fx, fy = float(a.group(1)), float(a.group(2))
        ang = math.radians(float(a.group(3) or 0))
        for pm in re.finditer(r'\(pad "[^"]*" (\w+)[\s\S]*?\(at (' + NUM + r") (" + NUM
                              + r")(?: " + NUM + r")?\)[\s\S]*?\(size (" + NUM + r") ("
                              + NUM + r")\)([\s\S]{0,300}?)\(uuid", bl):
            tipo, px, py, sw, sh, resto = pm.groups()
            px, py, sw, sh = float(px), float(py), float(sw), float(sh)
            n = re.search(r'\(net "([^"]+)"\)', resto)
            lay = re.search(r'\(layers ([^)]*)\)', resto)
            txt = lay.group(1) if lay else ""
            if tipo == "thru_hole" or "*.Cu" in txt:
                cams = (0, 1)
            elif "B.Cu" in txt:
                cams = (1,)
            else:
                cams = (0,)
            out.append((fx + px * math.cos(ang) + py * math.sin(ang),
                        fy - px * math.sin(ang) + py * math.cos(ang),
                        max(sw, sh) / 2, n.group(1) if n else None, cams))
    return out


def pistas_da_placa(t):
    """(x1, y1, x2, y2, rede, camada)."""
    out = []
    for m in re.finditer(r"\(segment\s*\(start (" + NUM + r") (" + NUM
                         + r")\)\s*\(end (" + NUM + r") (" + NUM
                         + r")\)[\s\S]{0,200}?\(layer \"([^\"]+)\"\)[\s\S]{0,200}?"
                         r"\(net \"([^\"]+)\"\)", t):
        x1, y1, x2, y2 = (float(v) for v in m.groups()[:4])
        out.append((x1, y1, x2, y2, m.group(6), CAMADAS.index(m.group(5))
                    if m.group(5) in CAMADAS else 0))
    return out


def vias_da_placa(t):
    return [(float(m.group(1)), float(m.group(2)), m.group(3))
            for m in re.finditer(r"\(via\s*\(at (" + NUM + r") (" + NUM
                                 + r")\)[\s\S]{0,220}?\(net \"([^\"]+)\"\)", t)]


def redes_com_plano(t):
    """As redes que tem zona de cobre nesta placa.

    ⚠️⚠️ E O CAMPO CHAMA-SE `net`, NAO `net_name`. Escrevi tres filtros a
    procurar `(net_name "X")` dentro de `(zone ...)`, e o KiCad 10 escreve
    `(net "X")`. Os tres devolviam SEMPRE conjunto vazio — e um filtro vazio nao
    da erro: da a resposta mais tranquilizadora que existe.

    Li dezenas de vezes a linha "0 de 34 travessias passam perto de um pad do
    plano" e li-a como um achado. Nao era: era isto. E a guarda dos pads do
    plano em B.Cu, que medi e declarei inutil, nunca chegou a fazer nada —
    portanto o que eu medi foi duas vezes a mesma coisa.

    ⚠️ POR ISSO ESTA FUNCAO GRITA. Se a placa tem `(zone` e ela nao achar rede
    nenhuma, o silencio deixa de ser uma opcao."""
    redes = set()
    for m in re.finditer(r'\(zone\b', t):
        j = m.start()
        d = 0
        for k in range(j, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    break
        cab = t[j:min(k, j + 400)]
        n = re.search(r'\(net_name "([^"]*)"\)', cab) or \
            re.search(r'\(net "([^"]+)"\)', cab)
        if n and n.group(1) and not n.group(1).isdigit():
            redes.add(n.group(1))
    if "(zone" in t and not redes:
        raise SystemExit("*** a placa tem zonas e eu nao achei a rede de "
                         "nenhuma. O filtro esta partido, e um filtro partido "
                         "devolve vazio sem se queixar.")
    return redes


def cruzam(a, b, c, d):
    """Dois segmentos cruzam-se? ⚠️ Comparar EXTREMOS nao chega: duas pistas
    podem cruzar-se a meio sem que nenhuma ponta esteja perto da outra. Foi o
    que fiz numa primeira versao e deu dez cruzamentos que o DRC apanhou."""
    def o(p, q, r):
        v = (q[1] - p[1]) * (r[0] - q[0]) - (q[0] - p[0]) * (r[1] - q[1])
        return 0 if abs(v) < 1e-9 else (1 if v > 0 else 2)
    o1, o2, o3, o4 = o(a, b, c), o(a, b, d), o(c, d, a), o(c, d, b)
    return o1 != o2 and o3 != o4


def dist_ponto_seg(px, py, x1, y1, x2, y2):
    dx, dy = x2 - x1, y2 - y1
    if dx == dy == 0:
        return math.hypot(px - x1, py - y1)
    tt = max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - (x1 + tt * dx), py - (y1 + tt * dy))


class Grelha:
    """Indice espacial dos obstaculos, para a busca nao varrer a lista toda."""

    def __init__(self, itens, celula=4.0):
        self.c = celula
        self.d = {}
        for it in itens:
            k = (int(it[0] / celula), int(it[1] / celula))
            for a in (-1, 0, 1):
                for b in (-1, 0, 1):
                    self.d.setdefault((k[0] + a, k[1] + b), []).append(it)

    def perto(self, x, y):
        return self.d.get((int(x / self.c), int(y / self.c)), ())


# --- a busca, em tres dimensoes ---------------------------------------------

def astar(A, B, camA, camB, rede, larg, gs, limites, so_uma_face=False,
          passo=None, folga=None, tecto=None):
    """A* sobre (x, y, camada). `camA`/`camB` sao as camadas em que cada ponta
    e alcancavel: um pad passante vale nas duas, um SMD so na sua.

    ⚠️ MEIA DIAGONAL DE MARGEM A MAIS. A busca so testa CENTROS de celula, e o
    segmento entre duas passa mais perto de um obstaculo do que qualquer das
    pontas — ate meia diagonal. Sem isto saem folgas de 0,02 mm que o DRC
    apanha e que ninguem consegue explicar olhando para o caminho."""
    import heapq
    PASSO_L = PASSO if passo is None else passo
    FOLGA_L = FOLGA if folga is None else folga
    TECTO_L = TECTO if tecto is None else tecto
    x0, y0, x1, y1 = limites
    nx = int((x1 - x0) / PASSO_L) + 1
    ny = int((y1 - y0) / PASSO_L) + 1
    m = larg / 2 + FOLGA_L + PASSO_L * 0.71
    mv = VIA_R + FOLGA_L + PASSO_L * 0.71
    # ⚠️ FOLGA APERTADA SO JUNTO AS PONTAS — a fuga do pad.
    #
    # Todas as ligacoes que sobravam terminavam num pad de passo fino: os pinos
    # do U1 (SOIC, 1,27 mm) e os do cabo plano. As pistas chegavam a 5 mm e
    # paravam. Nao e falta de espaco na placa: e que o PRIMEIRO MILIMETRO a
    # sair de um pad de passo fino nao cabe na folga generosa que serve para o
    # resto do percurso.
    #
    # Apertar a folga em toda a parte mede-se pior (23 -> 34): o roteador
    # guloso enfia pistas cedo e fecha corredores. Apertar SO nos primeiros
    # milimetros a volta das duas pontas nao tem esse efeito — ali nao ha
    # corredor a fechar, ha um pad de onde e preciso sair.
    #
    # ⚠️ E o apertado nunca desce abaixo da regra da placa. O DRC valida o
    # percurso todo com a mesma regra, e a retraccao continua a existir.
    mf = larg / 2 + FOLGA_FUGA + PASSO_L * 0.71

    def pos(c):
        return (x0 + c[0] * PASSO_L, y0 + c[1] * PASSO_L)

    def cel(p):
        return (int(round((p[0] - x0) / PASSO_L)), int(round((p[1] - y0) / PASSO_L)))

    cache = {}

    def ok(c, L, marg=None):
        chave = (c, L, marg is not None)
        if chave in cache:
            return cache[chave]
        if not (0 <= c[0] < nx and 0 <= c[1] < ny):
            cache[chave] = False
            return False
        px, py = pos(c)
        if marg is not None:
            mm = marg
        elif (math.hypot(px - A[0], py - A[1]) < RAIO_FUGA
              or math.hypot(px - B[0], py - B[1]) < RAIO_FUGA):
            mm = mf
        else:
            mm = m
        v = True
        for ox, oy, orr, on in gs[L].perto(px, py):
            if on == rede:
                continue
            if math.hypot(px - ox, py - oy) < orr + mm:
                v = False
                break
        cache[chave] = v
        return v

    def cabe_via(c):
        return ok(c, 0, mv) and ok(c, 1, mv)

    ini = [(cel(A), L) for L in camA]
    fins = {(cel(B), L) for L in camB}
    fimxy = cel(B)
    DIRS = [(1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)]

    def h(e):
        return (abs(e[0][0] - fimxy[0]) + abs(e[0][1] - fimxy[1])) * PASSO_L

    fila, veio, custo = [], {}, {}
    for e in ini:
        custo[e] = 0.0
        heapq.heappush(fila, (h(e), 0.0, e, None))
    limite = 0
    while fila:
        limite += 1
        # ⚠️ TECTO. Sem ele uma ligacao impossivel come tudo. Subi-o de 300k
        # para 900k a pensar que fecharia mais ligacoes, e medi: fechou
        # EXACTAMENTE as mesmas, tres vezes mais devagar. Uma busca que nao
        # acha em 300 mil nos nao acha em 900 mil — nao lhe falta paciencia,
        # falta-lhe caminho. Voltou a 300k.
        # ⚠️ O TECTO E O QUE CONTROLA O TEMPO, e nao a qualidade. Medido: uma
        # busca que nao acha em 300 mil nos nao acha em 900 mil — fecha
        # exactamente as mesmas ligacoes, tres vezes mais devagar. E quando se
        # fecham celulas (a guarda dos pads do plano em B.Cu), o numero de
        # buscas SEM SOLUCAO cresce, e cada uma delas gasta o tecto inteiro
        # antes de desistir: a mesma corrida passou de 75 segundos para treze
        # minutos so por isso. Quem paga o tecto sao as impossiveis.
        if limite > TECTO_L:
            return None
        _, g, e, d = heapq.heappop(fila)
        if e in fins:
            cam, cur = [e], e
            while cur in veio:
                cur = veio[cur][0]
                cam.append(cur)
            cam.reverse()
            # cantos: guarda so onde a direccao muda ou a camada muda
            pts, ant = [(pos(cam[0][0]), cam[0][1])], None
            for i in range(1, len(cam)):
                mudou_cam = cam[i][1] != cam[i - 1][1]
                dd = (cam[i][0][0] - cam[i - 1][0][0], cam[i][0][1] - cam[i - 1][0][1])
                if mudou_cam:
                    pts.append((pos(cam[i - 1][0]), cam[i - 1][1]))
                    pts.append((pos(cam[i][0]), cam[i][1]))
                    ant = None
                    continue
                if ant and dd != ant:
                    pts.append((pos(cam[i - 1][0]), cam[i - 1][1]))
                ant = dd
            pts.append((pos(cam[-1][0]), cam[-1][1]))
            # limpa pontos repetidos seguidos
            lim = [pts[0]]
            for q in pts[1:]:
                if q != lim[-1]:
                    lim.append(q)
            return lim
        (cx, cy), L = e
        for dd in DIRS:
            n = ((cx + dd[0], cy + dd[1]), L)
            if n not in fins and not ok(n[0], L):
                continue
            pas = PASSO_L * (1.414 if dd[0] and dd[1] else 1.0)
            ng = g + pas + (VIRAGEM * PASSO_L if d and dd != d else 0.0)
            if ng < custo.get(n, 1e9):
                custo[n] = ng
                veio[n] = (e, dd)
                heapq.heappush(fila, (ng + h(n), ng, n, dd))
        if not so_uma_face:
            n = ((cx, cy), 1 - L)
            if (n in fins or ok(n[0], 1 - L)) and cabe_via((cx, cy)):
                ng = g + CUSTO_VIA * PASSO_L
                if ng < custo.get(n, 1e9):
                    custo[n] = ng
                    veio[n] = (e, None)
                    heapq.heappush(fila, (ng + h(n), ng, n, None))
    return None


# ⚠️ O GUIA TEM DE SER PERMISSIVO, senao nao guia nada. A primeira versao
# corria a busca grossa com a folga a serio (0,35) numa malha de 1 mm, e a
# margem da grelha soma-se: 0,125 + 0,35 + 0,71 = 1,19 mm de raio proibido em
# volta de cada obstaculo. Com isso o corredor nao existe em lado nenhum e o
# guia devolveu ZERO em onze tentativas.
#
# Ele nao desenha cobre: so diz por onde. Quem valida e a busca fina, com a
# folga a serio, e o DRC por tras dela. Um guia optimista faz perder tempo; um
# guia pessimista nao faz nada.
PASSO_GROSSO = 0.5      # malha da busca que so procura o CORREDOR
FOLGA_GROSSA = 0.10     # ⚠️ optimista de proposito: e uma sugestao, nao cobre
RONDAS_ARRANCAR = 3     # quantas voltas de arrancar-e-refazer
MAX_ARRANCAR = 3        # ⚠️ acima disto o problema e de desenho, nao de rota
CAIXA_TROCO = 9.0       # quanto se alarga a caixa de cada troco fino


def guiado(A, B, camA, camB, rede, larg, gs, limites, so_uma_face=False):
    """Busca em dois niveis: acha o CORREDOR numa malha grossa, e so depois
    desenha cada troco na malha fina.

    POR QUE EXISTE.

    As ligacoes que sobravam eram quase todas travessias longas — 97 mm, 91 mm,
    de um lado da placa ao outro. Nao falhavam por nao haver caminho: falhavam
    por o A* esgotar o tecto de nos antes de la chegar. Uma travessia de 97 mm
    numa malha de 0,25 mm sao quase 400 celulas de percurso, e com penalizacao
    de curva a busca abre um leque enorme antes de se decidir.

    ⚠️ E SUBIR O TECTO NAO RESOLVE — medido duas vezes: 300k e 900k fecham
    exactamente as mesmas ligacoes, e 500k fecha MENOS que 300k porque deixa o
    guloso teimar em ligacoes dificeis e fechar corredores das seguintes. O
    problema nao e paciencia, e escala.

    Numa malha de 1 mm a mesma placa tem 18 mil celulas em vez de 288 mil, e a
    travessia sao 100 passos. O corredor acha-se num instante. Depois cada
    troco entre dois cantos desse corredor e curto, e a malha fina resolve-o
    numa caixa pequena.

    ⚠️ O GROSSO NAO DESENHA COBRE. Ele so diz por onde. Quem escreve e sempre a
    busca fina, com a folga a serio, e o DRC valida na mesma — com a retraccao
    por tras. Um guia errado faz perder tempo, nao faz perder a placa."""
    cam = astar(A, B, camA, camB, rede, larg, gs, limites, so_uma_face,
                passo=PASSO_GROSSO, folga=FOLGA_GROSSA, tecto=120000)
    if not cam or len(cam) < 2:
        return None
    # os cantos do corredor sao os pontos de passagem
    marcos = [cam[0]] + [c for c in cam[1:-1]] + [cam[-1]]
    if len(marcos) < 3:
        return None                 # sem cantos, o grosso nao acrescenta nada
    todo = []
    for i in range(len(marcos) - 1):
        (p1, L1), (p2, L2) = marcos[i], marcos[i + 1]
        if p1 == p2 and L1 != L2:
            todo.append((p1, L1))
            continue
        a = A if i == 0 else p1
        b = B if i == len(marcos) - 2 else p2
        ca = camA if i == 0 else (L1,)
        cb = camB if i == len(marcos) - 2 else (L2,)
        cx = (max(limites[0], min(a[0], b[0]) - CAIXA_TROCO),
              max(limites[1], min(a[1], b[1]) - CAIXA_TROCO),
              min(limites[2], max(a[0], b[0]) + CAIXA_TROCO),
              min(limites[3], max(a[1], b[1]) + CAIXA_TROCO))
        troco = astar(a, b, ca, cb, rede, larg, gs, cx, so_uma_face)
        if not troco:
            return None             # um troco que falha invalida o corredor
        todo += troco if not todo else troco[1:]
    return todo


def amostra(lista, A, B, rede, passo=0.4, r=0.18):
    n = max(2, int(math.hypot(B[0] - A[0], B[1] - A[1]) / passo))
    for i in range(n + 1):
        lista.append((A[0] + (B[0] - A[0]) * i / n,
                      A[1] + (B[1] - A[1]) * i / n, r, rede))


# --- o que o DRC diz --------------------------------------------------------

def estado(caminho):
    """(redes cujas pistas violam, ligacoes por fazer POR REDE).

    ⚠️ O ESTRAGO NO PLANO NAO E UM ERRO DE GEOMETRIA, e tem DUAS formas.
    A obvia e `Zone [X] <-> Zone [X]`: o plano partido em ilhas. A outra, que
    me escapou, e um **pad cortado do plano** — a pista em B.Cu passa entre o
    pad e o cobre, o pad fica orfao, e isso aparece como uma ligacao por fazer
    normalissima na rede GND. O plano continua inteiro. A terra e que nao.

    Medido: um roteamento baixou o total de 47 para 20 e ao mesmo tempo levou
    o `GND` de 0 para 5. O total melhorou 27; a placa perdeu cinco ligacoes de
    terra, que sao as que menos se podem perder.

    ⚠️ POR ISSO NAO SE CONTA UMA FORMA DE ESTRAGO. Conta-se o PERFIL: quantas
    ligacoes faltam em cada rede. E a regra de aceitacao e uma so —
    **nenhuma rede pode piorar**. Nao ha lista de sintomas a manter
    actualizada, e nao ha sintoma novo que passe por nao estar na lista."""
    subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--severity-error",
                    "--severity-warning", "--format", "json", "-o", "/tmp/_v.json",
                    caminho], capture_output=True)
    d = json.load(open("/tmp/_v.json"))
    # ⚠️ A ASSINATURA E (TIPO, REDE), E NAO A REDE. Subtrair a rede inteira da
    # linha de base e demasiado grosseiro: a base tinha uma via solta em AGND, e
    # com isso deixei passar uma folga de 0,193 mm numa pista de AGND, que e
    # outro defeito na mesma rede. Uma isencao vale para o defeito que se viu,
    # nunca para tudo o que partilhe o nome com ele.
    import collections
    maus = collections.Counter()
    for x in d.get("violations", []):
        if x["type"] not in ("clearance", "tracks_crossing", "shorting_items",
                             "copper_edge_clearance", "track_dangling",
                             "via_dangling", "copper_sliver",
                             "holes_co_located", "hole_clearance"):
            continue
        for i in x.get("items", []):
            m = re.search(r"(?:Track|Via) \[([^\]]+)\]", i.get("description", ""))
            if m:
                maus[(x["type"], m.group(1))] += 1
    # ⚠️⚠️ A CHAVE E (REDE, ESPECIE), E NAO SO A REDE.
    #
    # Contar quantas ligacoes faltam em cada rede deixa um estrago mascarar-se
    # noutro: se um pad de GND se liga (-1) e ao mesmo tempo o PLANO SE PARTE
    # (+1), o total da rede nao muda, e a regra "nenhuma rede pode piorar" ve
    # zero. Foi assim que uma corrida entregou a placa com a terra em pedacos e
    # o auto-teste disse que estava boa — quem a apanhou foi o portao da
    # fabrica, que conta as especies em separado.
    #
    # Um plano partido nao e "mais uma ligacao por fazer" que se compensa com
    # outra qualquer: e uma especie propria, e tem de ter contador proprio.
    #
    # ⚠️ E E A MESMA FAMILIA DA ASSINATURA (TIPO, REDE) DAS VIOLACOES, que ja
    # tinha corrigido ali e nao tinha corrigido aqui. Uma licao aprendida num
    # sitio nao se aplica sozinha ao sitio do lado.
    import collections
    perfil = collections.Counter()
    for x in d.get("unconnected_items", []):
        ds = [i.get("description", "") for i in x.get("items", [])]
        especie = "zona-partida" if (len(ds) == 2
                                     and all("Zone [" in s for s in ds)) else "pad"
        redes = set()
        for s in ds:
            m = re.search(r"\[([^\]]+)\]", s)
            if m:
                redes.add(m.group(1))
        for r in redes:
            perfil[(r, especie)] += 1
    return maus, perfil


def piorou(antes, depois):
    """Chaves que ficaram PIORES. A regra de aceitacao, e a unica."""
    return {k for k in set(antes) | set(depois) if depois[k] > antes[k]}


def redes(chaves):
    """O nome da rede de cada chave. ⚠️ As chaves das VIOLACOES sao
    (tipo, rede) e as do PERFIL sao (rede, especie) — a rede esta em pontas
    diferentes, e trocar isto imprime o tipo do defeito onde devia estar o nome
    da rede, e depois retira-se a rede errada."""
    fora = set()
    for k in chaves:
        if not isinstance(k, tuple):
            fora.add(k)
        elif k[1] in ("zona-partida", "pad"):
            fora.add(k[0])          # perfil: (rede, especie)
        else:
            fora.add(k[1])          # violacoes: (tipo, rede)
    return sorted(fora)


def auto_teste():
    """A regra de aceitacao interrogada por casos, e nao por confianca.

    ⚠️ EXISTE PORQUE ELA JA FALHOU EM SILENCIO. A versao anterior contava
    quantas ligacoes faltavam por rede, e com isso um pad que se ligava
    mascarava um plano que se partia: o total nao mudava e a regra dizia que
    estava bom. A placa saiu com a terra em pedacos e foi o portao da fabrica
    que a apanhou.

    Um verificador que nao e verificado e uma opiniao com ar de medicao."""
    import collections as C
    casos = [
        ("plano parte-se enquanto um pad se liga (o buraco de 07/09)",
         C.Counter({("GND", "pad"): 1}),
         C.Counter({("GND", "zona-partida"): 1}), True),
        ("plano parte-se do nada",
         C.Counter(), C.Counter({("GND", "zona-partida"): 1}), True),
        ("uma ligacao fecha-se e mais nada muda",
         C.Counter({("OLED_CLK", "pad"): 1}), C.Counter(), False),
        ("tudo igual",
         C.Counter({("GND", "pad"): 2}), C.Counter({("GND", "pad"): 2}), False),
        ("uma rede piora em pads",
         C.Counter({("V+PRE", "pad"): 1}), C.Counter({("V+PRE", "pad"): 2}), True),
        ("troca-se um plano partido por dois pads soltos — pior, e nao no total",
         C.Counter({("GND", "zona-partida"): 1}),
         C.Counter({("GND", "pad"): 2}), True),
    ]
    maus = 0
    for nome, a, b, espera in casos:
        deu = bool(piorou(a, b))
        ok = deu == espera
        maus += not ok
        print("  %s  %s" % ("✓" if ok else "✗ FALHA", nome))
    print("  redes() lê a rede nas DUAS formas de chave:")
    for chave, esperado in ((("clearance", "AGND"), "AGND"),
                            (("GND", "zona-partida"), "GND")):
        r = redes({chave})
        ok = r == [esperado]
        maus += not ok
        print("    %s %s -> %s" % ("✓" if ok else "✗ FALHA", chave, r))
    if maus:
        raise SystemExit("*** %d caso(s) falharam. A regra de aceitacao nao "
                         "esta a proteger o que diz proteger." % maus)
    print("  a regra passa em todos os casos.")


def texto_com(segs, vias, base):
    L = ""
    for (x1, y1), (x2, y2), w, rede, cam in segs:
        L += ('\t(segment\n\t\t(start %g %g)\n\t\t(end %g %g)\n\t\t(width %g)\n'
              '\t\t(layer "%s")\n\t\t(net "%s")\n\t\t(uuid "%s")\n\t)\n'
              % (x1, y1, x2, y2, w, CAMADAS[cam], rede, uuid.uuid4()))
    for x, y, rede in vias:
        L += ('\t(via\n\t\t(at %g %g)\n\t\t(size 0.6)\n\t\t(drill 0.3)\n'
              '\t\t(layers "F.Cu" "B.Cu")\n\t\t(net "%s")\n\t\t(uuid "%s")\n\t)\n'
              % (x, y, rede, uuid.uuid4()))
    i = base.rstrip().rfind(")")
    return base[:i] + L + base[i:]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    if sys.argv[1] == "--auto-teste":
        print("roteia v%s — a interrogar a regra de aceitacao" % VERSAO)
        auto_teste()
        return 0
    p = sys.argv[1]
    escreve = "--escreve" in sys.argv
    so_uma = "--so-uma-face" in sys.argv
    so = sys.argv[sys.argv.index("--rede") + 1] if "--rede" in sys.argv else None
    global FOLGA, PASSO, TECTO
    if "--tecto" in sys.argv:
        TECTO = int(sys.argv[sys.argv.index("--tecto") + 1])
    if "--folga" in sys.argv:
        FOLGA = float(sys.argv[sys.argv.index("--folga") + 1])
    if "--passo" in sys.argv:
        PASSO = float(sys.argv[sys.argv.index("--passo") + 1])
    caixa = float(sys.argv[sys.argv.index("--caixa") + 1]) if "--caixa" in sys.argv else 0.0
    # ⚠️ A BUSCA EM DOIS NIVEIS FICOU OPCIONAL, e a razao e medicao. Parecia
    # obviamente certa — as ligacoes que sobravam eram travessias de 90 mm que
    # falhavam por escala, e uma malha grossa acha o corredor num instante.
    # Acha mesmo. Mas o corredor dela tem resolucao de 1 mm, e as voltas largas
    # que sugere fecham passagens a quem vem depois:
    #
    #     cadeia sem o guia -> 12 ligacoes por fazer
    #     cadeia com o guia -> 20
    #
    # Fica em `--guiado` para quem tiver uma placa onde a escala seja mesmo o
    # problema. Nesta, o problema era outro (§12r: a fuga do pad).
    guia = "--guiado" in sys.argv
    arranca = "--arranca" in sys.argv
    if FOLGA < 0.20:
        sys.exit("RECUSO: folga %.3f abaixo da regra da placa (0,20). O DRC "
                 "recusaria tudo o que eu escrevesse." % FOLGA)
    t = io.open(p, encoding="utf-8").read()

    # ⚠️ UM KEEPOUT NAO E UM PLANO, e a guarda tinha de aprender a diferenca.
    #
    # A regra da 15cp e "sem cobre ENCHIDO em F.Cu, senao nao ha camada de
    # sinal". Um keepout nao enche nada — EXCLUI. Os quatro furos M3 levam um
    # circulo de exclusao em ambas as camadas, e essa guarda recusava a placa
    # inteira por causa deles: a proibicao certa aplicada ao objecto errado.
    #
    # Distingue-se pelo que o bloco DIZ: um keepout traz `(keepout ...)`.
    for z in re.finditer(r'\(zone([\s\S]{0,900}?)\(layers? ("[^"]+"[^)]*)\)', t):
        if "(keepout" in z.group(1) or "(keepout" in t[z.end():z.end() + 700]:
            continue
        if "F.Cu" in z.group(2):
            sys.exit("RECUSO: ha zona de cobre em F.Cu nesta placa.\n"
                     "        Ponha o plano so em B.Cu e costure os pads SMD com\n"
                     "        vias (pcb_contorno.costura) antes de correr. Ver 15cp.")

    print("roteia v%s — %s%s  (folga %.2f · passo %.3f)"
          % (VERSAO, os.path.basename(p), "" if escreve else "  ensaio",
             FOLGA, PASSO), file=sys.stderr)
    pads = pads_da_placa(t)
    pistas = pistas_da_placa(t)
    vias0 = vias_da_placa(t)

    subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--format", "json",
                    "-o", "/tmp/_r.json", p], capture_output=True)
    d = json.load(open("/tmp/_r.json"))
    faltam = []
    for x in d.get("unconnected_items", []):
        pts, ds = [], []
        for i in x.get("items", []):
            po = i.get("pos") or {}
            ds.append(i.get("description", ""))
            nm = re.search(r"\[([^\]]+)\]", ds[-1])
            if "x" in po:
                pts.append((po["x"], po["y"], nm.group(1) if nm else None))
        if len(pts) == 2 and pts[0][2] and not any("Zone [" in s for s in ds):
            faltam.append((pts[0][2], (pts[0][0], pts[0][1]), (pts[1][0], pts[1][1])))
    print("  %d ligacoes por rotear" % len(faltam), file=sys.stderr)

    # camadas alcancaveis em cada ponta: pelo pad mais proximo
    def camadas_em(q, rede):
        melhor, dm = None, 1e9
        for px, py, pr, pn, cams in pads:
            if pn != rede:
                continue
            dd = math.hypot(px - q[0], py - q[1])
            if dd < dm:
                melhor, dm = cams, dd
        return melhor if (melhor and dm < 0.6) else (0, 1)

    # ⚠️ NA CAMADA DO PLANO, OS PADS DO PLANO AFASTAM MAIS QUEM NAO E DELES.
    # Aritmetica, medida entre dois pads de um header de 2,54: sobram 0,42 mm de
    # cada lado do corredor. Para caberem la a pista E o cobre do plano a
    # contornar o pad, sao precisos
    #     pista/2 0,125 + folga 0,20 + cobre minimo 0,25 + folga 0,20 = 0,775
    # Nao cabe. Uma pista em B.Cu entre dois pinos de conector nao divide o
    # espaco com o plano: expulsa-o, e os pinos de terra do conector ficam
    # orfaos. Era o que faziam as tres redes que a retraccao devolvia sempre.
    #
    # Marcar isto como obstaculo custa alguns caminhos em B.Cu e poupa a
    # retraccao de os descobrir um a um, a um DRC cada.
    COBRE_MIN = 0.25
    redes_de_plano = redes_com_plano(t)
    # ⚠️ DESLIGADA POR OMISSAO, por medicao. A guarda parece obviamente certa —
    # impede de graca um estrago que a retraccao so descobre a um DRC cada — mas
    # medida de raiz, ate convergir, com duas passadas largas e a apertada:
    #
    #     tecto 300k · sem guarda -> 19 por fazer · geometria 0 · 227 s   <- fica
    #     tecto 300k · com guarda -> 20 por fazer · geometria 0 · 243 s
    #     tecto 500k · com guarda -> 22 por fazer · geometria 0 · 274 s
    #
    # Fecha celulas, e as buscas que passam a nao ter solucao gastam o tecto
    # inteiro antes de desistir. `--guarda-plano` liga-a para quem quiser.
    guarda_plano = FOLGA + COBRE_MIN if "--guarda-plano" in sys.argv else 0.0

    obst = [[], []]
    for px, py, pr, pn, cams in pads:
        for L in cams:
            extra = guarda_plano if (L == 1 and pn in redes_de_plano) else 0.0
            obst[L].append((px, py, pr + extra, pn))
    for x1, y1, x2, y2, n, L in pistas:
        amostra(obst[L], (x1, y1), (x2, y2), n, r=0.15)
    for x, y, n in vias0:
        obst[0].append((x, y, VIA_R, n))
        obst[1].append((x, y, VIA_R, n))

    bordo = [tuple(map(float, m.groups())) for m in
             re.finditer(r'\(gr_line\s*\(start (' + NUM + r') (' + NUM + r')\)\s*\(end ('
                         + NUM + r') (' + NUM + r')\)[\s\S]{0,120}?Edge\.Cuts', t)]
    bx = [v for b in bordo for v in (b[0], b[2])]
    by = [v for b in bordo for v in (b[1], b[3])]
    limites = (min(bx) + 0.6, min(by) + 0.6, max(bx) - 0.6, max(by) - 0.6)

    # ⚠️ A ORDEM MUDA O RESULTADO, num roteador guloso. Quem entra primeiro
    # escolhe o corredor; quem entra depois fica com o que sobrar. Escolhida por
    # medicao — ver o quadro no cabecalho.
    ordem = "longa"
    if "--ordem" in sys.argv:
        ordem = sys.argv[sys.argv.index("--ordem") + 1]
    if ordem == "longa":
        faltam.sort(key=lambda f: -math.hypot(f[2][0] - f[1][0], f[2][1] - f[1][1]))
    elif ordem == "curta":
        faltam.sort(key=lambda f: math.hypot(f[2][0] - f[1][0], f[2][1] - f[1][1]))

    novas, novas_vias, feitas, saltos, guiadas = [], [], [], 0, 0
    for rede, A, B in faltam:
        if so and rede != so:
            continue
        larg = LARGURA.get(rede, 0.25)
        gs = [Grelha(obst[0]), Grelha(obst[1])]
        # ⚠️ CAIXA A VOLTA DA LIGACAO, quando pedida. Uma malha fina o bastante
        # para entrar entre pinos de conector (0,085 mm) transforma esta placa
        # em 2,5 milhoes de celulas por camada, e a busca esgota o tecto antes
        # de sair do sitio. Mas uma ligacao que sobrou de uma passada larga e
        # quase sempre CURTA — nao precisa da placa toda para dar a volta.
        # Restringir a busca a uma caixa em volta das duas pontas troca
        # generalidade por uma malha que cabe, e e uma troca boa no fim.
        lim = limites
        if caixa:
            lim = (max(limites[0], min(A[0], B[0]) - caixa),
                   max(limites[1], min(A[1], B[1]) - caixa),
                   min(limites[2], max(A[0], B[0]) + caixa),
                   min(limites[3], max(A[1], B[1]) + caixa))
        cam = astar(A, B, camadas_em(A, rede), camadas_em(B, rede),
                    rede, larg, gs, lim, so_uma)
        if (not cam or len(cam) < 2) and not caixa and guia:
            # ⚠️ SO DEPOIS DE A DIRECTA FALHAR. Se a busca fina acha o caminho
            # sozinha, o caminho dela e melhor: o corredor do grosso e uma
            # sugestao com resolucao de 1 mm, e obedecer-lhe quando nao e
            # preciso da voltas que ninguem quereria.
            cam = guiado(A, B, camadas_em(A, rede), camadas_em(B, rede),
                         rede, larg, gs, lim, so_uma)
            if cam:
                guiadas += 1
        if not cam or len(cam) < 2:
            continue
        usou_b = False
        for i in range(len(cam) - 1):
            (p1, L1), (p2, L2) = cam[i], cam[i + 1]
            if L1 != L2:
                # ⚠️ SEM REPETIR. Duas travessias no mesmo ponto dao dois furos
                # coincidentes, e a fabrica broca duas vezes o mesmo sitio:
                # `holes_co_located`. Uma via por ponto, e chega.
                q = (round(p1[0], 3), round(p1[1], 3))
                if any(abs(v[0] - q[0]) < 0.05 and abs(v[1] - q[1]) < 0.05
                       for v in novas_vias) or \
                   any(abs(v[0] - q[0]) < 0.05 and abs(v[1] - q[1]) < 0.05
                       for v in vias0):
                    saltos += 1
                    continue
                novas_vias.append((q[0], q[1], rede))
                obst[0].append((p1[0], p1[1], VIA_R, rede))
                obst[1].append((p1[0], p1[1], VIA_R, rede))
                saltos += 1
                continue
            if p1 == p2:
                continue
            novas.append((p1, p2, larg, rede, L1))
            amostra(obst[L1], p1, p2, rede, r=0.15)
            if L1 == 1:
                usou_b = True
        feitas.append((rede, usou_b))

    # ─────────────────────────────────────────────────────────────────────
    # ARRANCAR E REFAZER
    #
    # ⚠️ E A CAPACIDADE QUE FALTAVA, e nao mais uma afinacao. Tudo o que tentei
    # antes — malha, folga, tecto, ordem de ataque, corredor grosso — parte do
    # mesmo pressuposto: que existe caminho livre e falta acha-lo. Para as que
    # sobram nao existe. O cobre que la esta ocupa o unico sitio por onde elas
    # podiam passar, e nenhuma busca acha um caminho que nao ha.
    #
    # O que um roteador a serio faz e o obvio depois de dito: ARRANCA o que
    # estorva, passa, e volta a rotear o que arrancou — que agora tem de dar a
    # volta, e quase sempre pode, porque tinha alternativas e a outra nao.
    #
    # ⚠️ E SO NO FIM, com tudo o resto ja feito. A meio seria destruir trabalho
    # bom para ganhar espaco que o proximo passo voltaria a fechar.
    #
    # ⚠️ SO ARRANCA POUCAS. Se uma ligacao precisa de arrancar meia placa, nao
    # e ela que esta mal colocada — e o desenho, e isso e uma pergunta para
    # quem desenha, nao um problema para o roteador resolver a martelo.
    if arranca:
        por_fazer = [f for f in faltam
                     if f[0] not in {r for r, _ in feitas}]
        # ⚠️ SO VALE SE O NUMERO DESCER, e guarda-se o estado para poder voltar.
        # Arrancar a rede X para a Y passar troca uma ligacao por outra se a X
        # nao conseguir refazer-se — e pode ficar PIOR que antes, porque a X
        # estava feita e a Y continua por fazer. Sem esta guarda o
        # arrancar-e-refazer melhora uma passada e piora a placa, que e como
        # ele mediu 14 contra 12 na cadeia inteira.
        antes_n = len(por_fazer)
        guarda_novas = list(novas)
        guarda_vias = list(novas_vias)
        guarda_feitas = list(feitas)
        arrancadas = 0
        for ronda in range(RONDAS_ARRANCAR):
            if not por_fazer:
                break
            progrediu = False
            for rede, A, B in list(por_fazer):
                larg = LARGURA.get(rede, 0.25)
                # 1. procura ignorando as PISTAS de outras redes (os pads nao)
                so_pads = [[], []]
                for px, py, pr, pn, cams in pads:
                    for L in cams:
                        so_pads[L].append((px, py, pr, pn))
                for x, y, n in vias0:
                    so_pads[0].append((x, y, VIA_R, n))
                    so_pads[1].append((x, y, VIA_R, n))
                gsp = [Grelha(so_pads[0]), Grelha(so_pads[1])]
                livre = astar(A, B, camadas_em(A, rede), camadas_em(B, rede),
                              rede, larg, gsp, limites, so_uma)
                if not livre:
                    continue        # nem sem cobre nenhum ha caminho: nao e disto
                # 2. quem e que esse caminho atravessa
                estorvo = set()
                for i in range(len(livre) - 1):
                    (p1, L1), (p2, L2) = livre[i], livre[i + 1]
                    if L1 != L2 or p1 == p2:
                        continue
                    for (q1, q2, _w, rd, cm) in novas:
                        if cm != L1 or rd == rede:
                            continue
                        if (cruzam(p1, p2, q1, q2)
                                or dist_ponto_seg(q1[0], q1[1], p1[0], p1[1],
                                                  p2[0], p2[1]) < larg / 2 + FOLGA):
                            estorvo.add(rd)
                if not estorvo or len(estorvo) > MAX_ARRANCAR:
                    continue
                # 3. arranca, poe esta, e devolve as arrancadas a fila
                guardadas = [s for s in novas if s[3] in estorvo]
                guardadas_v = [v for v in novas_vias if v[2] in estorvo]
                novas[:] = [s for s in novas if s[3] not in estorvo]
                novas_vias[:] = [v for v in novas_vias if v[2] not in estorvo]
                obst[0][:] = []
                obst[1][:] = []
                for px, py, pr, pn, cams in pads:
                    for L in cams:
                        obst[L].append((px, py, pr, pn))
                for x1, y1, x2, y2, n, L in pistas:
                    amostra(obst[L], (x1, y1), (x2, y2), n, r=0.15)
                for x, y, n in vias0 + [(a1, b1, r1) for a1, b1, r1 in novas_vias]:
                    obst[0].append((x, y, VIA_R, n))
                    obst[1].append((x, y, VIA_R, n))
                for (q1, q2, _w, rd, cm) in novas:
                    amostra(obst[cm], q1, q2, rd, r=0.15)
                gs2 = [Grelha(obst[0]), Grelha(obst[1])]
                cam2 = astar(A, B, camadas_em(A, rede), camadas_em(B, rede),
                             rede, larg, gs2, limites, so_uma)
                if not cam2:
                    novas.extend(guardadas)
                    novas_vias.extend(guardadas_v)
                    continue
                for i in range(len(cam2) - 1):
                    (p1, L1), (p2, L2) = cam2[i], cam2[i + 1]
                    if L1 != L2:
                        q = (round(p1[0], 3), round(p1[1], 3))
                        if not any(abs(v[0] - q[0]) < 0.05 and abs(v[1] - q[1]) < 0.05
                                   for v in novas_vias + vias0):
                            novas_vias.append((q[0], q[1], rede))
                        continue
                    if p1 != p2:
                        novas.append((p1, p2, larg, rede, L1))
                feitas.append((rede, any(L == 1 for _p, L in cam2)))
                por_fazer.remove((rede, A, B))
                for rd in estorvo:
                    for f in faltam:
                        if f[0] == rd and f not in por_fazer:
                            por_fazer.append(f)
                    feitas[:] = [f for f in feitas if f[0] != rd]
                arrancadas += len(estorvo)
                progrediu = True
            if not progrediu:
                break
        if arrancadas and len(por_fazer) >= antes_n:
            novas[:] = guarda_novas
            novas_vias[:] = guarda_vias
            feitas[:] = guarda_feitas
            print("  arrancar-e-refazer: tirei %d pista(s) e ficou em %d por "
                  "fazer contra %d — nao desceu, e volto atras."
                  % (arrancadas, len(por_fazer), antes_n), file=sys.stderr)
        elif arrancadas:
            print("  arrancar-e-refazer: %d pista(s) tiradas do caminho, "
                  "%d ligacao(oes) por fazer contra %d"
                  % (arrancadas, len(por_fazer), antes_n), file=sys.stderr)

    print("  (%d achadas pelo corredor grosso)" % guiadas, file=sys.stderr)
    print("  %d roteadas · %d salto(s) de camada · %d ficam para a pessoa"
          % (len(feitas), saltos,
             len([f for f in faltam if not so or f[0] == so]) - len(feitas)),
          file=sys.stderr)

    if not escreve:
        print("  ensaio: nada gravado.", file=sys.stderr)
        return 0
    if not novas:
        print("  nada a gravar.", file=sys.stderr)
        return 0

    # ⚠️ LINHA DE BASE PRIMEIRO. A placa ja pode chegar com defeitos que nao sao
    # meus — havia um `via_dangling` em AGND vindo da costura — e sem os
    # subtrair a ferramenta culpa-se pelo estrago alheio, retira as suas pistas
    # todas e nao entrega nada. O que se mede e o DELTA, nunca o absoluto.
    maus0, perfil0 = estado(p)
    if maus0:
        print("  linha de base ja com defeito: %s (nao e meu, e nao o conto)"
              % " ".join("%s/%s" % k for k in sorted(maus0)), file=sys.stderr)

    io.open(p, "w", encoding="utf-8").write(texto_com(novas, novas_vias, t))
    maus, perfil = estado(p)
    maus = piorou(maus0, maus)
    pior = piorou(perfil0, perfil)
    if maus:
        antes = len(novas)
        rm = set(redes(maus))
        novas = [s for s in novas if s[3] not in rm]
        novas_vias = [v for v in novas_vias if v[2] not in rm]
        feitas = [f for f in feitas if f[0] not in rm]
        io.open(p, "w", encoding="utf-8").write(texto_com(novas, novas_vias, t))
        print("  ⚠️ %d segmento(s) retirados: %s violavam o DRC."
              % (antes - len(novas), " ".join(redes(maus))), file=sys.stderr)
        maus, perfil = estado(p)
        maus = piorou(maus0, maus)
        pior = piorou(perfil0, perfil)

    if pior:
        # ⚠️ BISSECCAO, e nao "tira todas as que passaram por B.Cu". A maioria
        # das travessias e inofensiva: o cobre flui a volta. So se procura a
        # que corta. Comeca-se por tirar TODAS e confirmar que zera — se nem
        # assim zerar, o plano ja estava partido antes de eu chegar, e nao ha
        # nada a retirar.
        print("  ⚠️ %s piorou. A procurar a travessia culpada."
              % " ".join(redes(pior)), file=sys.stderr)
        todas = sorted({r for r, ub in feitas if ub})

        def com(exclui):
            io.open(p, "w", encoding="utf-8").write(texto_com(
                [s for s in novas if s[3] not in exclui],
                [v for v in novas_vias if v[2] not in exclui], t))
            return piorou(perfil0, estado(p)[1])

        # ⚠️ FILTRAR AS CANDIDATAS POR GEOMETRIA ANTES DE GASTAR DRCs NELAS.
        # Cada teste da retraccao e um DRC completo com enchimento de zonas —
        # dezenas de segundos numa placa cheia — e eu testava as 33 redes que
        # tocaram em B.Cu quando so duas ou tres podiam ser culpadas. Uma
        # passada chegou a levar quinze minutos, quase toda a interrogar redes
        # inocentes.
        #
        # Uma travessia so pode orfanar um pad se passar tao perto dele que o
        # plano ja nao cabe entre os dois: e preciso caber folga + uma largura
        # minima de cobre + folga. Isso mede-se aqui, de graca.
        #
        # ⚠️ O DRC continua a ser quem decide. O filtro nao absolve ninguem —
        # so escolhe a quem vale a pena perguntar.
        COBRE_MIN = 0.25
        # ⚠️ a referencia do filtro e a REGRA DA PLACA, nao a folga desta
        # corrida: numa passada apertada a folga encolhe, o filtro encolhe com
        # ela e deixa de suspeitar de quem devia. Foi o que fez o filtro
        # devolver "0 de 3" numa passada em que havia estrago.
        perto = 0.20 * 2 + COBRE_MIN
        redes_z = redes_com_plano(t)
        pads_z = [(px, py, pr) for px, py, pr, pn, cams in pads
                  if pn in redes_z and 1 in cams]
        susp = set()
        for (x1, y1), (x2, y2), _lw, rd, cm in novas:
            if cm != 1 or rd in susp:
                continue
            for px, py, pr in pads_z:
                if dist_ponto_seg(px, py, x1, y1, x2, y2) < pr + perto:
                    susp.add(rd)
                    break
        cands = sorted(set(todas) & susp)
        print("     %d de %d travessias passam perto de um pad do plano: so "
              "essas vale a pena interrogar" % (len(cands), len(todas)),
              file=sys.stderr)

        # ⚠️ E o atalho tem por onde voltar. Se tirar as suspeitas todas nao
        # chegar, volta-se a lista completa — um atalho que erra sem recuo e so
        # um erro mais rapido.
        if cands and com(set(cands)):
            print("     tirar so as suspeitas nao chegou: volto a lista toda.",
                  file=sys.stderr)
            cands = todas
        # ⚠️ E SE NEM ASSIM, A LISTA E TUDO O QUE ELE ESCREVEU. As travessias
        # por B.Cu sao as suspeitas obvias, mas nao sao as unicas: uma pista em
        # F.Cu que passe rente a um pad de terra tambem o pode isolar do seu
        # coto ate a via. Uma retraccao que nao alcanca tudo o que escreveu nao
        # e uma retraccao — e desiste de gravar um trabalho que era quase todo
        # bom por causa de uma pista que nem sequer podia acusar.
        if not cands or com(set(cands)):
            todas_novas = sorted({s[3] for s in novas})
            if len(todas_novas) > len(cands) and not com(set(todas_novas)):
                print("     nao eram as travessias: alargo a TODAS as %d redes "
                      "que escrevi." % len(todas_novas), file=sys.stderr)
                cands = todas_novas
            else:
                io.open(p, "w", encoding="utf-8").write(
                    texto_com(novas, novas_vias, t))
                print("     nem retirando tudo o que escrevi isto zera: o "
                      "estrago ja la estava. Nao retiro nada.", file=sys.stderr)
                cands = []
        if cands:
            # ⚠️ VARRIMENTO, e nao bisseccao — medido, e ao contrario do que se
            # espera. Escrevi uma versao que partia ao meio, a pensar em
            # O(culpadas · log n), e ficou PIOR nos dois eixos: mais lenta (com
            # 5 culpadas em 33, descer nas metades que falham custa mais testes
            # que percorrer a lista) e com pior resultado — 25 ligacoes por
            # fazer contra 19 — porque o recuo que ela precisa para o caso das
            # culpadas-em-conjunto retira TODAS as candidatas de uma vez.
            # A bisseccao paga-se quando ha uma culpada. Aqui ha varias.
            manter = []
            for c in cands:                 # devolve-as uma a uma; fica o que couber
                if not com(set(cands) - set(manter) - {c}):
                    manter.append(c)
            fora = [c for c in cands if c not in manter]
            com(set(fora))
            novas = [s for s in novas if s[3] not in fora]
            novas_vias = [v for v in novas_vias if v[2] not in fora]
            feitas = [f for f in feitas if f[0] not in fora]
            print("     %d rede(s) devolvidas a pessoa: %s"
                  % (len(fora), " ".join(fora) or "—"), file=sys.stderr)

    maus, perfil = estado(p)
    maus = piorou(maus0, maus)
    pior = piorou(perfil0, perfil)
    if maus or pior:
        io.open(p, "w", encoding="utf-8").write(t)
        raise SystemExit("*** ainda viola (%s) ou piora (%s). Nao gravo nada."
                         % (" ".join(redes(maus)) or "—",
                            " ".join(redes(pior)) or "—"))
    print("  gravado: %d segmentos · %d vias · nenhuma rede piorou"
          % (len(novas), len(novas_vias)), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
