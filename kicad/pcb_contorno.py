#!/usr/bin/env python3
"""pcb_contorno v0.7.0 — contorno e colocacao MECANICA de um .kicad_pcb.

POR QUE EXISTE.

O contorno de uma placa nao e escolha estetica: e ditado pela caixa. E a
posicao do encoder e dos pots tambem nao: os eixos tem de sair pelos furos da
tampa. Escrever isso a mao no editor e transcrever numeros de um desenho para
outro — que e exactamente a familia de erro que este projecto ja pagou.

⚠️ A FONTE E O GERADOR DA CAIXA, NAO OS DESENHOS.
Houve tres outras fontes plausiveis para esta caixa — desenhos de uma que nunca
foi cortada, e o corte em nogueira de 6,4, que e real mas e OUTRA caixa. Todas
apagadas a 06/09 (estao no historico). A que esta a ser cortada e a do
`caixa/cotuca_caixa_corte.py`: acrilico cast de 6,0, frente 55, tras 80, TRES
arcade e nenhum berco. Este ficheiro IMPORTA esse gerador.

⚠️ O QUE ELE NAO FAZ: colocar o resto, rotear, e decidir seja o que for de
estetico. Isso e trabalho de layout com veto do dB no render.

⚠️ EIXOS. No desenho da caixa o y cresce para CIMA a partir do canto
inferior-esquerdo da face; no KiCad cresce para BAIXO. A conversao vive so na
funcao `da_tampa()`, e nao se repete em sitio nenhum — a mesma disciplina do
`nota()` dos geradores de esquema, que nasceu de 9 notas impressas ao
contrario por a coordenada estar repetida em cada chamada.

USO:  pcb_contorno.py <projecto>        # cerebro | painel
"""
import io
import os
import re
import sys
import uuid

VERSAO = "0.7.0"
AQUI = os.path.dirname(os.path.abspath(__file__))

# --- a caixa, IMPORTADA do seu gerador --------------------------------------
#
# ⚠️ A FONTE E O `caixa/cotuca_caixa_corte.py`, e nao um desenho nem um SVG.
# Ele tem `if __name__ == "__main__"`, por isso importa-se sem correr nada — e
# importar e mais forte que medir: nao ha parsing entre a cota e o uso, logo
# nao ha sitio onde as duas divirjam.
#
# Historico, porque explica os portoes que estao mais abaixo:
#   · a placa foi dimensionada contra DESENHOS de uma caixa que nao foi
#     construida (paredes de 6 mm mas frente de 40, dois arcade, um berco de
#     powerbank que nunca foi cortado);
#   · depois contra o corte em nogueira de 6,4 — uma caixa REAL mas OUTRA;
#   · agora contra o gerador do acrilico, que e o que esta a ser cortado.
# Tres fontes plausiveis para a mesma caixa. A que vale e a que virou objecto.
sys.path.insert(0, os.path.join(AQUI, os.pardir, "caixa"))
import cotuca_caixa_corte as CAIXA          # noqa: E402

NUM = r"-?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?"   # o `[\d.]+` ingenuo perde notacao cientifica
ESPESSURA = CAIXA.T
INTERIOR = (CAIXA.BODY_W, CAIXA.BODY_D)
ALTURA_UTIL = (CAIXA.HF - 2 * CAIXA.T, CAIXA.HB - 2 * CAIXA.T)   # frente, tras


def _da_tampa_para_face(x, y):
    """coords do CORPO da tampa -> coords do DESENHO da face (0..200, 0..122,6).

    ⚠️ O gerador guarda os furos ja convertidos para o corpo (`to_top`), que
    desconta a parede. Quem pendura a placa nos veios raciocina na face. Medir
    a distancia do furo a aresta cortada da tampa da um numero diferente (e
    verdadeiro) e nao serve aqui — os dedos da junta fazem a aresta nao ser a
    face."""
    return (round(x + CAIXA.T, 4),
            round(y + CAIXA.T * CAIXA.SLOPE / CAIXA.RUN, 4))


def _furos_tampa():
    o7 = sorted((h[1], h[2]) for h in CAIXA.TOP_HOLES if h[3] == 7)
    if len(o7) != 3:
        raise SystemExit("*** a tampa tem %d furos de o7 — esperava 3 (encoder "
                         "e dois pots). A caixa mudou; a placa tem de mudar "
                         "com ela." % len(o7))
    d = dict(zip(("encoder", "pot_L", "pot_R"),
                 [_da_tampa_para_face(*c) for c in o7]))
    jan = CAIXA.TOP_RECTS
    if len(jan) != 1:
        raise SystemExit("*** %d janelas na tampa — esperava 1 (o OLED)." % len(jan))
    d["janela_oled"] = _da_tampa_para_face(jan[0][1], jan[0][2])
    return d


def _chapa_fundo():
    b = CAIXA.part_bottom()
    calcos = sorted((h[1], h[2]) for h in CAIXA.BOT_HOLES if h[3] == 3.4)
    return dict(w=max(p[0] for p in b), h=max(p[1] for p in b),
                calcos=calcos,
                espacadores=sorted((h[1], h[2]) for h in CAIXA.BOT_HOLES
                                   if h[3] == 3.2),
                # ⚠️ altura do calco = quantas chapas empilham. Sao 12 pecas
                # (6 `CB` com sexagono para porca cativa + 6 `CT` que a tapam)
                # para 6 posicoes, logo duas por posicao. Derivado, nao escrito.
                altura=round(sum(1 for n, _, _, _ in CAIXA.PARTS
                                 if n[:2] in ("CB", "CT")) / len(calcos) * CAIXA.T, 3))


TAMPA = _furos_tampa()
FUNDO = _chapa_fundo()
# DECIDIDO por medicao, 06/09: a aresta y=20 do KiCad olha para a FRENTE.
#
# Nao e estetica. Os tres arcade descem da tampa e passam por cima da placa, e a
# altura livre por baixo deles e de 7,5 mm no pior caso. As duas orientacoes
# nao empatam:
#   y_min -> 15 pecas debaixo dos discos, TODAS pequenas: resistencias, diodos,
#            ceramicos, um trimmer, e tres conectores (J3 J10 J15) que sao os
#            unicos a conferir contra os 7,5;
#   y_max ->  8 pecas, mas duas delas sao a DAISY em soquete (~17 mm) e o C1
#            (2200 uF, 25 mm). Nenhuma das duas cabe, com cabeca de botao
#            nenhuma.
# Menos pecas nao e melhor: o que conta e a mais ALTA de cada lista.
FRENTE_CEREBRO = "y_min"      # "y_min" = a aresta y=20 do KiCad | "y_max" = a y=120

CALCO = 12.0     # os calcos sao quadrados de 12 x 12
CALCO_H = FUNDO["altura"]

# --- placas -----------------------------------------------------------------
# (largura, altura) e canto inferior-esquerdo no datum da face que a segura
PLACAS = {
    "painel":  dict(w=176.0, h=36.0, x0=14.0, y0=74.0, assenta="tampa",
                    ficheiro="cotuca_painel/cotuca_painel.kicad_pcb",
                    origem=(20.0, 20.0)),
    # ⚠️ O cerebro NAO assenta no chao da caixa: assenta na chapa de FUNDO, que
    # e removivel (a casca desta caixa e soldada — quem abre e o fundo).
    # x0/y0 ficam a None: sao DERIVADOS, centrando a placa na chapa. Ver
    # `centra_no_fundo()` para a razao, que nao e arrumacao.
    "cerebro": dict(w=180.0, h=100.0, x0=None, y0=None, assenta="fundo",
                    ficheiro="cotuca_cerebro/cotuca_cerebro.kicad_pcb",
                    origem=(20.0, 20.0)),
}

# --- colocacao do resto -----------------------------------------------------
#
# Coordenadas ABSOLUTAS do KiCad, em mm. Nao sao esteticas: cada uma responde a
# uma razao que se pode discutir, e estao aqui em vez de espalhadas pelo editor
# para se poderem ler todas de uma vez.
#
# PAINEL (contorno x 20..196, y 20..56). Os tres eixos ficam na linha y=38,
# postos por FIXOS a partir dos furos da tampa. O resto arruma-se a volta:
#   · J1 (IDC) no extremo ESQUERDO — a fita vai ao cerebro, que fica em baixo
#     na caixa, e o lado esquerdo e o que lhe fica mais perto;
#   · J2 (OLED) tambem a esquerda — a janela da tampa esta em x 17,5..72,5, e o
#     cabo do modulo e curto;
#   · os JST na aresta de BAIXO — os arcade estao em (50,35) e (150,35) da
#     tampa, abaixo desta placa;
#   · C1/C2 COLADOS aos pots: 8 mm do cursor. A nota do esquema e explicita —
#     longe, o condensador filtra a pista e nao o cursor.
COLOCA = {
 "painel": {
    # J1 ROTADO 90°: o IDC 2x13 e 32 mm de comprido e a placa so tem 36 de
    # altura — na vertical saia 13 mm por baixo do contorno.
    # O IDC ocupa 32 mm da aresta de cima: x 26..58. Tudo o resto vive a
    # direita dele ou nas duas filas de baixo.
    "J1":  (26.0, 24.0, 90),
    "J2":  (68.0, 24.0, 0),
    "J3":  (26.0, 34.0, 0),     "J4":  (38.0, 34.0, 0),   "J5":  (50.0, 34.0, 0),
    "J6":  (26.0, 46.0, 0),     "J7":  (38.0, 46.0, 0),
    "Q1":  (52.0, 46.0, 0),     "Q2":  (64.0, 46.0, 0),
    "R1":  (52.0, 40.0, 0),     "R2":  (64.0, 40.0, 0),
    "R3":  (76.0, 40.0, 0),     "R4":  (88.0, 40.0, 0),
    "R5":  (76.0, 46.0, 0),     "R6":  (88.0, 46.0, 0),
    # C1/C2 COLADOS ao cursor: 6 mm do pino 2 de cada pot, do lado livre.
    "C1":  (133.0, 38.0, 0),    "C2":  (168.0, 38.0, 0),
 },
}


# quem tem posicao MECANICA fixa, e onde (so o painel, por agora)
FIXOS = {"painel": {"SW1": "encoder", "RV1": "pot_L", "RV2": "pot_R"}}


def da_tampa(p, alvo, xy):
    """(x, y) do desenho da caixa -> (x, y) do KiCad, na placa `alvo`.

    ⚠️ A INVERSAO DO Y VIVE SO AQUI. No desenho o y cresce para cima; no KiCad
    para baixo. Repetir esta conta em cada sitio de chamada e como se perdem
    nove notas de uma vez."""
    ox, oy = alvo["origem"]
    bx = xy[0] - alvo["x0"]
    by = (alvo["y0"] + alvo["h"]) - xy[1]
    return (round(ox + bx, 3), round(oy + by, 3))



FPLIB = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints"


# --- CEREBRO: zonas, e nao 57 coordenadas a mao ---------------------------
#
# Cada peca vai para a zona da sua FOLHA, e dentro da zona arruma-se em grelha.
# Zonar por folha nao e arrumacao: e o que mantem o analogico junto e longe do
# digital, que e a razao pela qual o pre-amp tem folha propria.
#
# O que manda na planta sao duas pecas: o `U2` (Daisy, 17 x 50 mm) e o `C1`
# (2200 uF, corpo ø16 x 25). A energia fica do lado de TRAS.
# ⚠️ A ALTURA DEIXOU DE SER O MOTIVO. A caixa desenhada tinha 28 mm livres a
# frente; a que foi cortada tem ~42 (frente de 55 e nao de 40) e ~67 atras. O
# C1 de 25 mm ja cabia a frente. O que mantem a energia atras agora e a parede:
# e nela que estao os recortes do USB, das linhas e dos DPDT.
#
#   x 20..200, y 20..120 (180 x 100)
#   ┌──────────────────────────────────────────────────┐
#   │ J14 (fita ao painel, aresta de cima)             │
#   │ PRE-AMP        │ U2 Daisy │ soquete │ ENERGIA    │
#   │ analogico      │          │         │ (tras)     │
#   └──────────────────────────────────────────────────┘
ZONAS_CEREBRO = [
    # (nome, x0, y0, largura, passo_x, passo_y, [refs])
    ("pre-amp A", 26, 34, 58, 14.5, 9.0,
     ["J5", "D11", "R11", "R12", "R13", "C11", "R10", "R14", "C12", "C13", "RV1"]),
    ("pre-amp B", 26, 58, 58, 14.5, 9.0,
     ["J6", "D21", "R21", "R22", "R23", "C21", "R20", "R24", "C22", "C23", "RV2"]),
    ("pre-amp comum", 26, 82, 58, 14.5, 9.0,
     ["U1", "C7", "C8", "C9", "R5", "R6", "R7", "C5", "C6"]),
    # ⚠️ conectores VERTICAIS: o 1x6 tem 14,4 mm e o 1x5 tem 11,9. Num passo de
    # 9 mm colidem uns nos outros — foi o que deu 43 das 52 violacoes de
    # geometria da primeira passagem.
    ("soquete", 118, 34, 32, 16.0, 19.0,
     ["J10", "J11", "J15", "J16", "R50", "R51", "NT1", "J12", "J13"]),
    ("energia", 152, 34, 44, 14.0, 13.0,
     ["F1", "D1", "D2", "C2", "C4", "R8", "R9", "J3", "J4", "J2", "C3"]),
]
# ⚠️ Fora da grelha por serem grandes de mais para ela:
#   C1  2200 uF, corpo ø16 x 25 — vai ao canto de TRAS, que e onde a caixa tem
#       58 mm livres (a frente so tem 28);
#   J1  terminal de parafuso do decoy, na mesma aresta.
# ⚠️ A Daisy vai RODADA 180°. Os pinos 1 e 40 estao a y=0 do footprint, e e
# nessa ponta que a Electrosmith poe o USB-C. Sem rodar, o conector aponta para
# a face da FRENTE — que e a da marca, sem furos, e a que so tem 28 mm livres.
# Rodada, aponta para TRAS, onde a parede ja leva o recorte de 12 x 6 do USB.
# A origem desloca-se para o canto oposto (+15,24 · +48,26) para o modulo
# ocupar o MESMO rectangulo: rodar sem compensar a origem tira-o da placa.
# ⚠️ J14 a x=40 e nao a x=30: o furo o3,2 do fundo (o que segura a placa) cai
# em KiCad (35,50 · 23,50), e a 30 o IDC ficava POR CIMA dele. Nao ha parafuso
# por baixo de um conector — e a `confere_furos` recusa antes de escrever.
UNICOS_CEREBRO = {"U2": (113.24, 88.26, 180), "J14": (40.0, 25.0, 90),
                  "C1": (178.0, 108.0, 0), "J1": (156.0, 108.0, 0),
                  # ⚠️ os WIMA de filme tem 13 mm de altura de corpo e nao
                  # cabem num passo de 9 mm — batiam na fila de baixo. Vao para
                  # a faixa livre entre o pre-amp e a Daisy.
                  "C14": (88.0, 40.0, 0), "C24": (88.0, 64.0, 0)}


def grelha(t, zonas, unicos):
    """Arruma cada lista de refs em grelha dentro da sua zona."""
    n = 0
    for nome, x0, y0, larg, px, py in [(z[0], z[1], z[2], z[3], z[4], z[5]) for z in zonas]:
        refs = dict((z[0], z[6]) for z in zonas)[nome]
        cols = max(1, int(larg // px))
        for i, ref in enumerate(refs):
            x = x0 + (i % cols) * px
            y = y0 + (i // cols) * py
            t = move(t, ref, (float(x), float(y)))
            n += 1
    for ref, (x, y, ang) in unicos.items():
        t = move(t, ref, (x, y), ang)
        n += 1
    return t, n


def eixo_de(fp):
    """Deslocamento (dx, dy) da ORIGEM do footprint ate ao EIXO, lido do
    proprio ficheiro `.kicad_mod`.

    ⚠️ A ORIGEM DE UM FOOTPRINT NAO E O EIXO. No `EC11E-Switch` a origem esta na
    fila de pinos e o eixo a (7,50 · 2,50); no `PTV09A-1` a (7,00 · -2,50). Pôr
    a origem no furo da tampa poe la os PINOS e deixa o eixo 7 mm ao lado — a
    tampa fecha e o botao nao entra. Foi o que aconteceu na primeira passagem.

    Le-se o circulo do corpo em `F.Fab`, que e onde o KiCad desenha o veio.
    Ler em vez de escrever: se o footprint mudar, a colocacao segue-o."""
    lib, _, nome = fp.partition(":")
    caminho = os.path.join(FPLIB, lib + ".pretty", nome + ".kicad_mod")
    t = io.open(caminho, encoding="utf-8").read()
    m = re.search(r'\(fp_circle\s*\(center ([\d.-]+) ([\d.-]+)\)'
                  r'[\s\S]{0,220}?\(layer "F\.Fab"', t)
    if not m:
        raise SystemExit("*** %s: nao achei o circulo do eixo em F.Fab. Sem ele "
                         "nao sei onde e o veio, e adivinhar poe o furo da "
                         "tampa no sitio errado." % fp)
    return (float(m.group(1)), float(m.group(2)))


def fp_de(t, ref):
    """O footprint que a placa usa para `ref` — para se ler o eixo dele."""
    for m in re.finditer(r'\n\t\(footprint "([^"]+)"', t):
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
        r = re.search(r'\(property "Reference" "([^"]+)"', bl)
        if r and r.group(1) == ref:
            return m.group(1)
    raise SystemExit("*** footprint de %s ausente da placa" % ref)


def u():
    return str(uuid.uuid4())


def contorno(t, alvo, r=3.0):
    """Rectangulo com cantos arredondados em Edge.Cuts. Substitui o que houver."""
    t = re.sub(r'\n\t\(gr_\w+[\s\S]*?\(layer "Edge\.Cuts"\)[\s\S]*?\n\t\)', '', t)
    ox, oy = alvo["origem"]
    w, h = alvo["w"], alvo["h"]
    L = []
    seg = ('\t(gr_line\n\t\t(start %g %g)\n\t\t(end %g %g)\n'
           '\t\t(stroke (width 0.1) (type default))\n\t\t(layer "Edge.Cuts")\n'
           '\t\t(uuid "%s")\n\t)\n')
    arco = ('\t(gr_arc\n\t\t(start %g %g)\n\t\t(mid %g %g)\n\t\t(end %g %g)\n'
            '\t\t(stroke (width 0.1) (type default))\n\t\t(layer "Edge.Cuts")\n'
            '\t\t(uuid "%s")\n\t)\n')
    x0, y0, x1, y1 = ox, oy, ox + w, oy + h
    L.append(seg % (x0 + r, y0, x1 - r, y0, u()))
    L.append(seg % (x1, y0 + r, x1, y1 - r, u()))
    L.append(seg % (x1 - r, y1, x0 + r, y1, u()))
    L.append(seg % (x0, y1 - r, x0, y0 + r, u()))
    k = r - r * 0.7071
    L.append(arco % (x1 - r, y0, x1 - k, y0 + k, x1, y0 + r, u()))
    L.append(arco % (x1, y1 - r, x1 - k, y1 - k, x1 - r, y1, u()))
    L.append(arco % (x0 + r, y1, x0 + k, y1 - k, x0, y1 - r, u()))
    L.append(arco % (x0, y0 + r, x0 + k, y0 + k, x0 + r, y0, u()))
    i = t.rstrip().rfind(')')
    return t[:i] + "".join(L) + t[i:]


def centra_no_fundo(alvo):
    """Centra a placa na chapa de fundo — e assim ela deixa de depender da
    ESPESSURA da caixa.

    ⚠️ ISTO E UM INVARIANTE, NAO UMA ESCOLHA ESTETICA. A espessura ja mudou
    tres vezes nesta caixa (6, 6,4, e agora acrilico de 3) e cada mudanca movia
    tudo: interior, chapa de fundo e os furos de fixacao. Com a placa CENTRADA,
    a posicao dos furos RELATIVA A PLACA e constante. A algebra, para nao ficar
    por fe — com W=200, D=120 e a chapa a ser (W-2T-1) x (D-2T-1):

        furo_x  = 25,5 - T - 0,5                 (o gerador desconta a parede)
        x0      = ((W - 2T - 1) - 180) / 2       (centrada)
        furo_x - x0 = 25 - T - (19 - 2T)/2 + ... = 15,5    para QUALQUER T

    O mesmo em y da 3,5. Por isso os furos da placa ficam sempre em
    (15,5 · 128,5) x (3,5 · 96,5) do canto dela, e a placa sobrevive a caixa
    mudar de material. Escrever `x0=3,5` a mao era verdade para T=6 e mentira
    para T=3 — e mentira do tipo que so aparece com a caixa em cima da mesa."""
    if alvo["x0"] is None:
        alvo["x0"] = round((FUNDO["w"] - alvo["w"]) / 2, 3)
    if alvo["y0"] is None:
        alvo["y0"] = round((FUNDO["h"] - alvo["h"]) / 2, 3)
    rel = [(round(e[0] - alvo["x0"], 3), round(e[1] - alvo["y0"], 3))
           for e in FUNDO["espacadores"]]
    esperado = [(15.5, 3.5), (15.5, 96.5), (128.5, 3.5), (128.5, 96.5)]
    if sorted(rel) != sorted(esperado):
        raise SystemExit("*** os furos do fundo caem em %s da placa, e o "
                         "invariante diz %s. Ou a placa deixou de ser 180x100, "
                         "ou o gerador da caixa mudou de regra — em qualquer "
                         "dos casos nao e para adivinhar." % (sorted(rel), esperado))
    return alvo


def confere_caixa(nome, alvo):
    """A placa cabe na caixa QUE FOI CORTADA? Devolve as linhas do relatorio.

    ⚠️ Existe porque a versao anterior declarava `INTERIOR = (188, 108)` e nunca
    o lia. Constante que ninguem le nao e cota: e decoracao que envelhece em
    silencio — e envelheceu, a caixa real tem 187,2 x 107,2."""
    L, mau = [], []
    if alvo["assenta"] == "fundo":
        alvo = centra_no_fundo(alvo)
    x0, y0, w, h = alvo["x0"], alvo["y0"], alvo["w"], alvo["h"]
    if alvo["assenta"] == "fundo":
        sw, sh = FUNDO["w"], FUNDO["h"]
        L.append("     assenta na chapa de FUNDO %g x %g" % (sw, sh))
        if x0 < 0 or y0 < 0 or x0 + w > sw or y0 + h > sh:
            mau.append("a placa sai da chapa de fundo")
        L.append("     folga ao rebordo do fundo: %.2f esq · %.2f dir · "
                 "%.2f frente · %.2f tras" % (x0, sw - x0 - w, y0, sh - y0 - h))
        # os calcos ocupam CALCO x CALCO centrados em cada furo o3,4
        batem = [c for c in FUNDO["calcos"]
                 if not (c[0] + CALCO / 2 <= x0 or c[0] - CALCO / 2 >= x0 + w or
                         c[1] + CALCO / 2 <= y0 or c[1] - CALCO / 2 >= y0 + h)]
        if batem:
            L.append("     ⚠️ a placa passa por cima de %d calco(s) de %gx%g: %s"
                     % (len(batem), CALCO, CALCO,
                        " ".join("(%g,%g)" % c for c in batem)))
            L.append("     ⚠️ calco tem %g mm de altura -> espacador de 8 mm NAO "
                     "serve; minimo %g mm" % (CALCO_H, CALCO_H + 3))
        else:
            L.append("     nenhum calco por baixo da placa")
        fura = [e for e in FUNDO["espacadores"]
                if x0 <= e[0] <= x0 + w and y0 <= e[1] <= y0 + h]
        L.append("     ESPESSURA da caixa %g mm -> util %g a frente, %g atras; "
                 "espacador minimo %g" % (ESPESSURA, ALTURA_UTIL[0],
                                          ALTURA_UTIL[1], CALCO_H + 3))
        L.append("     furos o3,2 do fundo sob a placa: %d de 4  %s"
                 % (len(fura), " ".join("(%g,%g)" % e for e in fura)))
    else:
        # a tampa segura a placa pelos eixos; o que conta e o vao interior
        li, lf = ESPESSURA, ESPESSURA + INTERIOR[0]
        bi, bf = ESPESSURA, ESPESSURA + INTERIOR[1]
        L.append("     pendurada na TAMPA · vao interior %g..%g x %g..%g"
                 % (li, lf, bi, bf))
        if x0 < li or x0 + w > lf or y0 < bi or y0 + h > bf:
            mau.append("a placa do painel bate nas paredes")
        L.append("     folga as paredes: %.2f esq · %.2f dir · %.2f frente · "
                 "%.2f tras" % (x0 - li, lf - x0 - w, y0 - bi, bf - y0 - h))
    if mau:
        raise SystemExit("*** " + "; ".join(mau) + " — a caixa ja existe, "
                         "quem cede e a placa.")
    return L


def confere_furos(t, alvo):
    """Os 4 furos o3,2 da chapa de fundo caem em zona livre da placa?

    ⚠️ Sao eles que a seguram. Um furo debaixo de um conector nao e um furo
    apertado: e um parafuso que nao entra depois da placa montada — e isso
    descobre-se com a caixa fechada e a solda feita."""
    import math
    RAIO = 6.5 / 2 + 0.25                      # anilha M3 o6,5 + folga
    pecas = []
    for m in re.finditer(r'\n\t\(footprint "', t):
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
        ref = re.search(r'\(property "Reference" "([^"]+)"', bl).group(1)
        at = re.search(r'\(at (' + NUM + r') (' + NUM + r')(?: (' + NUM + r'))?\)', bl)
        fx, fy, a = float(at.group(1)), float(at.group(2)), math.radians(float(at.group(3) or 0))
        for pm in re.finditer(r'\(pad [\s\S]*?\(at (' + NUM + r') (' + NUM + r')(?: ' + NUM
                              + r')?\)[\s\S]*?\(size (' + NUM + r') (' + NUM + r')\)', bl):
            px, py, sw, sh = map(float, pm.groups())
            pecas.append((fx + px * math.cos(a) + py * math.sin(a),
                          fy - px * math.sin(a) + py * math.cos(a),
                          max(sw, sh) / 2, ref))
    ox, oy = alvo["origem"]
    L, mau = [], []
    for ex, ey in FUNDO["espacadores"]:
        kx = ox + (ex - alvo["x0"])
        ky = oy + (alvo["h"] - (ey - alvo["y0"]))
        d, quem = min((math.hypot(px - kx, py - ky) - r, rf) for px, py, r, rf in pecas)
        L.append("     furo o3,2 em KiCad (%6.2f,%6.2f)  %-7s  vizinho %s a %+.2f"
                 % (kx, ky, "livre" if d > RAIO else "OCUPADO", quem, d))
        if d <= RAIO:
            mau.append("%s tapa o furo em (%.2f,%.2f)" % (quem, kx, ky))
    if mau:
        raise SystemExit("*** " + "; ".join(mau) + " — os furos sao da caixa, "
                         "quem se move e a peca.")
    return L


# --- altura dos componentes, por familia de footprint ------------------------
# ⚠️ Listar 15 refs debaixo de um botao nao e informacao: 12 delas sao 0805 de
# 0,7 mm e nao incomodam ninguem. O que interessa e a MAIS ALTA. Alturas em mm
# acima da placa, do lado dos componentes.
#
# ⚠️ CONECTOR NAO SE MEDE VAZIO. Um 1x6 vertical tem 8,5 mm de plastico e pino,
# mas o que ocupa o espaco e ele COM a ficha enfiada e o fio a sair — 20 mm e
# conservador. Medir a peca nua e medir o estado em que ela nunca esta.
ALTURAS = [
    (r"PinHeader.*Vertical",        20.0, "header + ficha + fio"),
    (r"JST_XH",                     20.0, "JST + ficha + fio"),
    (r"CP_Radial_D(\d+)",           11.0, "electrolitico radial"),
    (r"Potentiometer_.*3296",       10.0, "trimmer Bourns 3296"),
    (r"Electrosmith_Daisy",         17.0, "Daisy em soquete"),
    (r"TerminalBlock",              12.0, "terminal de parafuso"),
    (r"_THT|Radial|Disc",            8.0, "THT generico"),
    (r"SOT-23|SOD|D_SM|_0805_|_1206_|_1210_|SOIC", 2.5, "SMD"),
]


def altura_de(fp):
    for padrao, h, _ in ALTURAS:
        if re.search(padrao, fp):
            return h
    return None          # desconhecido nao e zero


def confere_arcade(t, alvo, frente_em):
    """Os botoes arcade descem da tampa. A placa passa por baixo deles?

    ⚠️ ISTO NAO E CLEARANCE DE PCB, E VOLUME. Nenhum DRC do KiCad ve um botao
    que vem de fora da placa — para o DRC a coluna de ar acima do cobre e
    infinita. A caixa e que a limita, e a conta faz-se aqui ou nao se faz.

    Medido a 06/09 com o botao na mao (40 mm de profundidade): sobra 1,5 mm
    entre a ponta do botao e a placa NUA. Qualquer componente debaixo de um
    disco de o30 e colisao."""
    import math
    ang = math.radians(CAIXA.ANG)
    # ⚠️ 40 mm sao o botao APERTADO, de ponta a ponta — inclui a cabeca que fica
    # ACIMA do painel. O que rouba altura ao interior e so a parte de BAIXO:
    #   abaixo = 40 - painel - cabeca
    # A cabeca de um arcade de 30 anda pelos 8-10 mm, mas NAO foi medida. Fica o
    # pior caso, cabeca = 0, que e o unico valor que nao mente para o lado
    # perigoso. Medir a saliencia abaixo do painel encolhe isto e so melhora.
    PAINEL = 2 * CAIXA.T             # tampa + reforco, onde o arcade monta
    BOTAO = 40.0 - PAINEL            # pior caso: toda a folga vai para baixo
    ESPACADOR = CALCO_H + 4.0        # o que o `confere_caixa` obriga, arredondado
    PCB = 1.6
    y_face = 35.0                    # os tres arcade, na face da tampa
    horiz = y_face * math.cos(ang)
    sob = CAIXA.HF + (CAIXA.HB - CAIXA.HF) * horiz / CAIXA.RUN - CAIXA.T
    livre = sob - CAIXA.T                        # acima da face de cima do fundo
    ponta = livre - CAIXA.T - BOTAO              # o reforco da tampa rouba T
    topo = ESPACADOR + PCB
    folga = ponta - topo

    fx = [h[1] + CAIXA.T for h in CAIXA.TOP_HOLES if h[3] == 30]   # corpo -> face
    if not fx:
        return ["     (a tampa nao tem arcade de o30 — nada a conferir)"]
    ax = [x - CAIXA.T - 0.5 - alvo["x0"] for x in sorted(fx)]
    ay = horiz - CAIXA.T - 0.5 - alvo["y0"]

    L = ["     ARCADE: painel %g mm · botao 40 apertado -> %.0f abaixo (pior caso)"
         % (PAINEL, BOTAO),
         "     ponta a %.1f do fundo, placa a %.1f -> ALTURA LIVRE %.1f mm"
         % (ponta, topo, folga),
         "     discos o30 sobre a placa: y %.1f · x %s"
         % (ay, " ".join("%.0f" % v for v in ax))]
    ox, oy = alvo["origem"]
    bate = []
    for m in re.finditer(r'\n\t\(footprint "', t):
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
        ref = re.search(r'\(property "Reference" "([^"]+)"', bl).group(1)
        fp = re.search(r'\(footprint "([^"]+)"', bl)
        fp = fp.group(1) if fp else ""
        at = re.search(r'\(at (' + NUM + r') (' + NUM + r')', bl)
        kx, ky = float(at.group(1)), float(at.group(2))
        # do KiCad para a placa, sabendo que aresta olha para a FRENTE da caixa
        if frente_em == "y_min":
            bx, by = kx - ox, ky - oy
        else:
            bx, by = (ox + alvo["w"]) - kx, (oy + alvo["h"]) - ky
        if any(math.hypot(bx - a, by - ay) <= 15 + 3 for a in ax):
            bate.append((ref, fp, altura_de(fp)))
    if bate:
        maus = sorted((h, r, f) for r, f, h in bate if h is None or h > folga)
        ok = len(bate) - len(maus)
        L.append("     %d peca(s) sob os discos · %d cabem nos %.1f mm"
                 % (len(bate), ok, folga))
        for h, r, f in maus:
            L.append("     ⚠️ %-5s %-5s %s" % (r, ("%.0f" % h) if h else "?",
                                               f.split(":")[-1][:44]))
        if not maus:
            L.append("     nenhuma passa dos %.1f mm — cabe" % folga)
    else:
        L.append("     nenhuma peca debaixo dos arcade")
    return L


def move(t, ref, xy, ang=None):
    """Poe o footprint `ref` em xy (e roda, se `ang`). Falha alto se faltar."""
    for m in re.finditer(r'\n\t\(footprint ', t):
        j = m.start() + 1
        d = 0
        for k in range(j, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    fim = k + 1
                    break
        bl = t[j:fim]
        r = re.search(r'\(property "Reference" "([^"]+)"', bl)
        if r and r.group(1) == ref:
            alvo_at = '%g %g' % xy + ('' if ang in (None, 0) else ' %g' % ang)
            novo, n = re.subn(r'(\(at )(-?[\d.]+) (-?[\d.]+)( -?[\d.]+)?',
                              lambda m: m.group(1) + alvo_at, bl, count=1)
            if not n:
                raise SystemExit("*** %s: nao achei o (at do footprint" % ref)
            return t[:j] + novo + t[fim:]
    raise SystemExit("*** footprint %s ausente da placa — colocacao a meio nao "
                     "e colocacao" % ref)


def main():
    print("pcb_contorno v%s" % VERSAO, file=sys.stderr)
    if len(sys.argv) != 2 or sys.argv[1] not in PLACAS:
        print(__doc__)
        raise SystemExit(2)
    nome = sys.argv[1]
    alvo = PLACAS[nome]
    p = os.path.join(AQUI, alvo["ficheiro"])
    t = io.open(p, encoding="utf-8").read()
    relato = confere_caixa(nome, alvo)
    t = contorno(t, alvo)
    postos = []
    for ref, furo in FIXOS.get(nome, {}).items():
        alvo_xy = da_tampa(nome, alvo, TAMPA[furo])
        dx, dy = eixo_de(fp_de(t, ref))
        xy = (round(alvo_xy[0] - dx, 3), round(alvo_xy[1] - dy, 3))
        t = move(t, ref, xy)
        postos.append((ref, furo, alvo_xy, (dx, dy), xy))
    for ref, (x, y, ang) in sorted(COLOCA.get(nome, {}).items()):
        t = move(t, ref, (x, y), ang)
    if nome == "cerebro":
        t, n = grelha(t, ZONAS_CEREBRO, UNICOS_CEREBRO)
        print("     %d pecas colocadas por zona" % n, file=sys.stderr)
    if alvo["assenta"] == "fundo":
        relato += confere_furos(t, alvo)
        # ⚠️ FRENTE_CEREBRO ainda nao esta decidido — ver a constante.
        relato += confere_arcade(t, alvo, FRENTE_CEREBRO)
    io.open(p, "w", encoding="utf-8").write(t)
    print("  %s: contorno %g x %g mm em (%g, %g)"
          % (nome, alvo["w"], alvo["h"], alvo["origem"][0], alvo["origem"][1]),
          file=sys.stderr)
    for l in relato:
        print(l, file=sys.stderr)
    for ref, furo, alvo_xy, eixo, xy in postos:
        print("     %-4s eixo em %-11s tampa%s -> KiCad %s · origem %s "
              "(eixo do footprint a %+g,%+g)"
              % (ref, furo, TAMPA[furo], alvo_xy, xy, eixo[0], eixo[1]),
              file=sys.stderr)


if __name__ == "__main__":
    main()
