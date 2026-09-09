#!/usr/bin/env python3
"""pcb_contorno v0.14.0 — contorno e colocacao MECANICA de um .kicad_pcb.

POR QUE EXISTE.

O contorno de uma placa nao e escolha estetica: e ditado pela caixa. E a
posicao do encoder e dos pots tambem nao: os eixos tem de sair pelos furos da
tampa. Escrever isso a mao no editor e transcrever numeros de um desenho para
outro — que e exactamente a familia de erro que este projecto ja pagou.

⚠️ A FONTE E O GERADOR DA CAIXA, NAO OS DESENHOS.
Houve tres outras fontes plausiveis para esta caixa — desenhos de uma que nunca
foi cortada, e o corte em nogueira de 6,4, que e real mas e OUTRA caixa. Todas
apagadas a 06/09 (estao no historico). A que esta a ser cortada e a do
`caixa/cotuca_caixa_corte.py`: acrilico de 3,0, frente 55, tras 80, TRES
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

VERSAO = "0.16.1"
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
    # ⚠️ os TP no vao entre o J2 (acaba a ~80) e o RV1 (comeca a ~132), na
    # maior faixa livre do painel: 163,8 mm em y=27,5.
    **{f"TP{i + 1}": (90.0 + 6.0 * i, 27.5, 0) for i in range(5)},
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
    # ⚠️ o par do +5V vai JUNTO AOS MOSFET, que sao a carga que comuta — nao ao
    # OLED, que tem os dele. O C5 colado ao IDC, onde o +3V3A entra e de onde
    # saem 150 mm de fita.
    "C3":  (52.0, 52.0, 0),     "C4":  (64.0, 52.0, 0),   "C5":  (26.0, 30.0, 0),
    # ⚠️ o par do +5V vai JUNTO AOS MOSFET, que sao a carga que comuta — nao
    # junto ao OLED, que tem os dele. E o C5 colado ao IDC, que e onde o
    # +3V3A entra e por onde saem 150 mm de fita.
    "C3":  (52.0, 52.0, 0),     "C4":  (64.0, 52.0, 0),
    "C5":  (26.0, 30.0, 0),
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
    # ⚠️ 6 colunas e passo de 12, e nao 4 e 9. Duas razoes que se somam:
    #   · o disco do arcade da esquerda tapa as colunas de x=26 a 55, e o do
    #     meio comeca em x=95 — as unicas colunas seguras sao x=69,5 e 84. Com
    #     4 colunas so havia UMA, e as tres pecas altas do canal empilhavam-se
    #     la a 9 mm, com os courtyards por cima uns dos outros;
    #   · a 12 mm de passo os WIMA de filme (13 mm de altura, ~18 de corpo)
    #     deixam de ser excepcao e entram na grelha como tudo o resto. Eram os
    #     unicos `UNICOS` que la estavam por nao caberem, e uma excepcao que
    #     desaparece e melhor que uma excepcao bem comentada.
    # ⚠️ PASSO DE 11, e nao de 9. O disco do arcade da esquerda tapa as colunas
    # x=26..55; a unica coluna segura de cada canal e x=69,5, e as tres pecas
    # THT do canal (J, electrolitico, trimmer) acabam empilhadas la. A 9 mm os
    # courtyards sobrepunham-se — tirar uma peca de debaixo de um botao e po-la
    # em cima de outra e mudar de defeito, nao resolver.
    # ⚠️ E a largura fica em 58 (4 colunas). Alarga-la para 6 punha as zonas em
    # cima da Daisy, que desceu para y=60 por causa do mesmo arcade: um conserto
    # que cria nove violacoes onde havia uma nao e conserto.
    # ⚠️ O PRE-AMP DESCEU 2 mm PARA O CONECTOR PODER RESPIRAR. O J14 e um header
    # de duas filas encostado a frente, e um conector de duas filas precisa de
    # canal de fuga dos DOIS lados: encostar um a um bordo e dar-lhe metade dos
    # pinos. Descia-lo 4 mm abre o canal da frente de 1,05 para 5,05 mm, mas a
    # essa altura o patio dele toca no D11 e no J5, que sao a primeira fila
    # deste bloco. Dois milimetros aqui pagam quatro ali.
    ("pre-amp A", 26, 36, 58, 14.5, 11.0,
     ["J5", "D11", "R11", "R12", "R13", "C11", "R10", "R14", "C12", "C13", "RV1"]),
    ("pre-amp B", 26, 64, 58, 14.5, 11.0,
     ["J6", "D21", "R21", "R22", "R23", "C21", "R20", "R24", "C22", "C23", "RV2"]),
    ("pre-amp comum", 26, 92, 58, 14.5, 11.0,
     ["U1", "C7", "C8", "C9", "R5", "R6", "R7", "C5", "C6"]),
    # ⚠️ conectores VERTICAIS: o 1x6 tem 14,4 mm e o 1x5 tem 11,9. Num passo de
    # 9 mm colidem uns nos outros — foi o que deu 43 das 52 violacoes de
    # geometria da primeira passagem.
    # ⚠️ J10 saiu: era o header de 5 vias da ponte por fio, e o ESP32 passou a
    # soquete (U3), que e peca UNICA por ser grande de mais para a grelha.
    # ⚠️ PASSO 22 E NAO 19: o J15/J16 passaram a 1x08, que tem 20,32 mm de
    # altura, e uma peca mais alta que o passo da fila encosta na fila
    # seguinte. Os pads ficaram a 1,22 mm quando precisavam de 1,70 — curto,
    # nao aperto. O passo de uma grelha e um limite da peca MAIS ALTA que ela
    # arruma, e nao da que la estava quando se escolheu o numero.
    ("soquete", 118, 34, 32, 16.0, 22.0,
     ["J11", "J15", "J16", "R50", "R51", "J12", "J13"]),
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
# ⚠️⚠️ J14 a x=45, e a x=40 NAO CHEGAVA. O `confere_furos` que aprovou o x=40
# comparava a distancia do furo aos PADS, e um IDC 2x13 tem pads a 2,54 e um
# corpo de 41,7 mm: o furo `H1` em (35,5 · 23,5) ficava DENTRO do courtyard
# (34,4..76,1), com o parafuso debaixo do conector, e a medicao dizia
# "livre, vizinho a +4,54". Duas medidas erradas ao mesmo tempo — o objecto
# (pads em vez de corpo) e a forma (ponto em vez de circulo de 3,5).
# Com x=45 o courtyard comeca em 39,4 e o furo fica a 3,9 do corpo.
# ⚠️ Historico: a nota antiga dizia que o furo o3,2 do fundo cai
# em KiCad (35,50 · 23,50), e a 30 o IDC ficava POR CIMA dele. Nao ha parafuso
# por baixo de um conector — e a `confere_furos` recusa antes de escrever.
# ⚠️ U2 desceu 20,8 mm (era y=88,26). O topo do modulo estava a y=39,2 e o disco
# do arcade do meio chega a y=59,3 — a Daisy no soquete tem 17 mm e a altura
# livre por baixo de um botao e 7,5. Nao e aperto, e o modulo esmagado. Desceu
# o suficiente para o topo ficar em y=60; sobram 10 mm ate a aresta de baixo.
# O `evita_arcade` nao lhe toca porque as pecas UNICAS sao colocadas a mao —
# e foi por isso que ela escapou a primeira passagem e so o portao a apanhou.
# ⚠️ A PONTE FICA NA FRONTEIRA DA ILHA, e nao onde calhar. O `NT1` e o UNICO
# ponto onde AGND encontra GND, e estava em (134, 72) — a 57 mm da ilha, que
# acaba em x=75. A topologia ate estava certa (continuava a ser uma ponte so),
# mas toda a corrente de retorno analogica voltava por uma pista de 0,25 mm em
# vez de por cobre largo, e uma ponte com impedancia e meia ponte.
#
# O footprint tem os dois pads a ±2 mm em x. Com o centro em x=75, o pad 1
# (AGND) cai dentro da ilha e o pad 2 (GND) cai fora, no plano de GND: a ponte
# passa a ser plano-a-plano, que e o que ela devia ter sido sempre.
#
# ⚠️ E foi a COSTURA que denunciou isto, ao recusar-se a descer uma via para um
# plano que ali nao existia. O defeito nao era a via: era o sitio da ponte. Uma
# ferramenta que recusa e diz porque, encontra o que ninguem foi procurar.
# ⚠️ O J14 DESCEU 4 mm, e a razao e roteamento e nao mecanica. Estava em y=25,
# com a fila da frente em y=22,5 e o bordo da placa em y=20: sobrava um canal de
# 1,01 mm entre os pads e o bordo. A fila de tras sai para o lado livre, mas a
# da frente so tinha aquele canal — e treze sinais nao cabem num canal onde
# cabe uma pista. Sair pelo outro lado obrigava a passar ENTRE dois pads do
# conector, e o vao de um header de 2,54 com pads de 1,70 e 0,840 mm, quando a
# pista precisa de 0,950.
#
# Era isto que segurava os cinco sinais do OLED em cada placa, e nao
# congestionamento: o diagnostico deu ZERO pistas num raio de 8 mm.
#
# A +4 mm o canal passa a 5,05 mm — cabem cinco pistas onde cabia uma. Continua junto a frente, que e onde o cabo
# tem de sair para o painel: mover para tras resolveria melhor o cobre e
# obrigaria o cabo a atravessar a caixa toda.
#
# ⚠️ +4 mm PARECIA MELHOR E COLIDE. A primeira busca mediu distancias entre
# PADS e deu 2,49 mm de folga; o DRC devolveu dois `courtyards_overlap`, com o
# D11 e o J5. O que reserva espaco num footprint e o PATIO, nao o pad — e o
# patio do J14 chega a y=18,8, que ja e fora da placa. E a mesma familia do
# varrimento que pos uma peca em cima de um electrolitico por medir os pads de
# 1 mm e ignorar o corpo de o16.
#
# ⚠️ Um conector de duas filas precisa de canal de fuga nos DOIS lados. Encostar
# um a um bordo e dar-lhe metade dos pinos.
# ⚠️ OS PONTOS DE TESTE VAO NUMA FILA, e a fila foi ACHADA e nao escolhida a
# olho: a maior faixa livre do cerebro tem 107,8 mm em y=50,5, medida com todos
# os pads e 2,6 mm de folga. Passo de 4 mm, que e o que uma ponta de prova
# precisa para nao tocar na vizinha.
#
# ⚠️ E NAO VAO PARA y<40. Ai ficam debaixo dos discos de arcade, onde nem se
# alcancam com a caixa aberta nem cabem no que sobra de altura. Um ponto de
# teste inalcancavel e um furo que ninguem sabe para que serve.
UNICOS_CEREBRO = {"U2": (113.24, 109.06, 180), "J14": (45.0, 29.0, 90),
                  **{f"TP{i + 1}": (88.0 + 4.0 * i, 50.5, 0) for i in range(11)},
                  "NT1": (75.0, 81.0, 0),
                  "C1": (178.0, 108.0, 0), "J1": (156.0, 108.0, 0),
                  # ⚠️ os WIMA de filme tem 13 mm de altura e ~18 de corpo: nao
                  # cabem na grelha. Vao para a faixa livre entre o pre-amp
                  # (acaba em x=69,5) e a Daisy (comeca em x=97).
                  "C14": (86.0, 38.0, 0), "C24": (86.0, 66.0, 0),
                  # ⚠️ U3 RODADO 180 pela mesma razao que a Daisy: o USB-C esta
                  # em -y do footprint e sem rodar apontava para a FRENTE, que e
                  # a face da marca e nao leva furos. Rodado aponta para tras,
                  # onde a parede leva a abertura que esta placa pede.
                  # ⚠️ Posicao achada por VARRIMENTO com caixas de pads e margem
                  # generosa (3,5 mm nos corpos grandes, 2,5 no resto). O primeiro
                  # varrimento usou so pads com 1 mm e deu (174, 94) — que bate no
                  # C1, um electrolitico de o16 cujo CORPO e muito maior que os
                  # pads. Duas iteracoes de DRC depois, a licao e a mesma de
                  # sempre: medir a coisa certa custa menos que medir depressa.
                  # Das 480 posicoes livres, a mais atras — o USB fica a 12 mm
                  # da parede em vez dos 30 da tentativa anterior.
                  # ⚠️ ABERTO: o U3 e um RADIO BLE e esta a 10 mm da borda da ilha de AGND.
                  # Electricamente nao ha problema — a ilha acaba em x=75 e ele
                  # comeca em 75,75, logo o GND dele nao a fura (era o achado 🔴
                  # da P3). Mas a P6 diz que rajadas de radio acoplam PELO AR, e
                  # contra isso o cobre nao defende: o lugar certo dele e a
                  # metade direita, longe do pre-amp.
                  # Tentei tres posicoes la e as tres bateram em C1, J4 ou J1 —
                  # a metade direita esta cheia. Mover o U3 para la exige mexer
                  # noutra peca, e isso e decisao de layout com o dB no editor,
                  # nao mais uma tentativa minha as cegas.
                  "U3": (85.0, 106.0, 180),
                  # ⚠️ COLADOS ao que desacoplam: o C15 ao pino 39 da Daisy, o
                  # C16 ao J14. A mais de poucos centimetros ligam-se atraves da
                  # indutancia que existiam para contornar (P4 da revisao).
                  "C15": (105.0, 115.0, 0), "C16": (78.0, 30.0, 0)}


def furos_kicad(alvo):
    """(x, y) dos furos o3,2 em coordenadas KiCad. Derivado da caixa."""
    if alvo.get("x0") is None:
        alvo["x0"] = round((FUNDO["w"] - alvo["w"]) / 2, 3)
        alvo["y0"] = round((FUNDO["h"] - alvo["h"]) / 2, 3)
    ox, oy = alvo["origem"]
    return [(ox + (ex - alvo["x0"]), oy + (alvo["h"] - (ey - alvo["y0"])))
            for ex, ey in FUNDO["espacadores"]]


def grelha(t, zonas, unicos, furos=()):
    """Arruma cada lista de refs em grelha dentro da sua zona.

    ⚠️ NINGUEM EM DOIS SITIOS. Uma ref numa grelha E em `unicos` fica com a
    posicao de `unicos` — ate o `evita_arcade` correr, que recalcula a grelha e
    a devolve em silencio ao sitio antigo. Mudei o NT1 para a fronteira da ilha,
    o ficheiro continuou a mostra-lo onde estava, e nenhum passo se queixou.
    Uma colocacao que se desfaz sem dizer nada e pior que uma que falha."""
    duplas = [r for z in zonas for r in z[6] if r in unicos]
    if duplas:
        raise SystemExit("*** %s esta numa grelha E em UNICOS. Escolha um: a "
                         "grelha volta a coloca-lo e a sua posicao explicita "
                         "perde-se sem aviso." % " ".join(duplas))
    n = 0
    for nome, x0, y0, larg, px, py in [(z[0], z[1], z[2], z[3], z[4], z[5]) for z in zonas]:
        refs = dict((z[0], z[6]) for z in zonas)[nome]
        cols = max(1, int(larg // px))
        # ⚠️⚠️ A GRELHA SALTA OS LUGARES QUE CAEM SOBRE UM FURO.
        #
        # Ate a v0.15.0 a grelha nao sabia dos furos: enchia os lugares por
        # ordem e o `confere_furos` reprovava no FIM. O `C6` caia a 2,58 mm do
        # furo `H2` e a corrida inteira morria ali, sem alternativa — porque
        # quem falha no fim nao tem por onde tentar outra vez.
        #
        # ⚠️ Uma restricao conhecida A PRIORI nao pertence ao verificador final:
        # pertence a colocacao. O verificador fica para o que ela nao pode
        # prever — e ai a recusa e informacao, nao um beco.
        #
        # O lugar e testado pela CAIXA do passo, e nao pelo centro: uma peca
        # ocupa a celula toda, e comparar centros aprova a peca cujo corpo
        # entra no furo pela borda.
        i = 0
        for ref in refs:
            while True:
                x = x0 + (i % cols) * px
                y = y0 + (i // cols) * py
                if not any(abs(hx - x) < px / 2 + RAIO_FURO
                           and abs(hy - y) < py / 2 + RAIO_FURO
                           for hx, hy in furos):
                    break
                i += 1
            t = move(t, ref, (float(x), float(y)))
            i += 1
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


# --- planos de cobre ---------------------------------------------------------
#
# ⚠️ DUAS TERRAS, DUAS ILHAS, UMA PONTE. A P3 da revisao de terra e explicita:
# `AGND` e `GND` juntos em mais de um sitio formam anel, e a fraccao da corrente
# que escolhe o caminho de dentro passa pela referencia do codec. A ponte e o
# `NT1` e e a UNICA — por isso o plano de `GND` tem de PARAR onde a ilha comeca,
# em vez de varrer a placa toda por baixo do pre-amp.
#
# Um plano de GND desenhado antes da P3 teria criado essa segunda ponte com ar
# de melhoria: mais cobre, menos impedancia, e um anel a mais.
ILHA_AGND = {"cerebro": (22.0, 30.0, 75.0, 120.0)}      # x0, y0, x1, y1 no KiCad
# ⚠️ PLANO SO EM B.Cu, e sinal em F.Cu. A primeira versao punha cobre nas DUAS
# camadas — e isso nao deixa camada livre para sinal: toda a pista atravessa um
# plano, e atravessar um plano nao e passar por cima dele, e CORTA-LO. Medido: o
# roteamento automatico partiu a ilha de AGND em pedacos e o `AGND` por ligar
# subiu de 1 para 8, com o DRC a dizer geometria zero o tempo todo (15co).
#
# Em troca, cada pad de terra em F.Cu precisa de uma VIA para chegar ao plano —
# e essa e a `costura()` mais abaixo. E o preco normal de um dois-lados, e
# paga-se uma vez.
ZONAS_COBRE = {
    "cerebro": [("GND", "B.Cu", None), ("AGND", "B.Cu", "ilha")],
    "painel":  [("GND", "B.Cu", None)],
}


def confere_sobreposicao(t):
    """Nenhum pad de uma peca pode tocar no pad de outra.

    ⚠️ ISTO DEVIA TER SIDO UM PORTAO DESDE O PRIMEIRO DIA. Mudei o J15 e o J16
    de 1x06 para 1x08 — a peca passou de 15,24 para 20,32 mm de altura, mais
    que o passo de 19 mm da grelha da zona — e os pads da ultima fila ficaram
    ENCOSTADOS aos da peca seguinte. Nada se queixou na colocacao. So apareceu
    depois de rotear, como cinco `shorting_items` no DRC, e com o roteador ja
    a ter gasto trabalho a contornar um estrago que ninguem lhe tinha dito que
    existia.
    ⚠️ E o DRC apanha-o, sim — mas apanha-o TARDE. Um portao que recusa antes
    de escrever custa um segundo; o mesmo defeito descoberto depois do
    roteamento custa a corrida toda e faz duvidar do roteador."""
    import math
    pads = []
    for m in re.finditer(r'\n\t\(footprint ', t):
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
        a = re.search(r'\(at (' + NUM + r') (' + NUM + r')(?: (' + NUM + r'))?\)', bl)
        fx, fy = float(a.group(1)), float(a.group(2))
        ang = math.radians(float(a.group(3) or 0))
        r = re.search(r'\(property "Reference" "([^"]+)"', bl)
        ref = r.group(1) if r else "?"
        for pm in re.finditer(r'\(pad "([^"]*)"[\s\S]{0,90}?\(at (' + NUM + r') ('
                              + NUM + r')(?: ' + NUM + r')?\)\s*\(size (' + NUM
                              + r') (' + NUM + r')\)', bl):
            px, py = float(pm.group(2)), float(pm.group(3))
            raio = max(float(pm.group(4)), float(pm.group(5))) / 2
            pads.append((fx + px * math.cos(ang) + py * math.sin(ang),
                         fy - px * math.sin(ang) + py * math.cos(ang),
                         raio, ref, pm.group(1)))
    maus = []
    for i in range(len(pads)):
        xi, yi, ri, refi, ni = pads[i]
        for j2 in range(i + 1, len(pads)):
            xj, yj, rj, refj, nj = pads[j2]
            if refi == refj:
                continue
            d = math.hypot(xi - xj, yi - yj)
            if d < ri + rj:
                maus.append((refi, ni, refj, nj, d, ri + rj))
    if maus:
        linhas = ["*** %d par(es) de pads de pecas DIFERENTES a tocar-se:" % len(maus)]
        for refi, ni, refj, nj, d, lim in maus[:10]:
            linhas.append("      %s.%s e %s.%s a %.2f mm (precisavam de %.2f)"
                          % (refi, ni, refj, nj, d, lim))
        linhas.append("    Isto e um curto-circuito no cobre, e nao um aperto.")
        raise SystemExit("\n".join(linhas))
    return ["     nenhum pad de pecas diferentes se toca (%d pads conferidos)"
            % len(pads)]


def costura(t, nome):
    """Uma via por cada pad de terra, para ele chegar ao plano em B.Cu.

    ⚠️ SEM ISTO O PLANO SO EM B.Cu DESLIGA TUDO. Um pad SMD vive em F.Cu e nao
    ve o cobre da outra camada; a via e o unico caminho. Sao ~47 vias nesta
    placa, e e o preco normal de um dois-lados com plano.

    ⚠️ E A VIA VAI PARA O LADO, nao em cima do pad. Via no pad suga solda por
    capilaridade na montagem e deixa a junta pobre — e a JLC monta o lado SMD
    desta placa.

    ⚠️⚠️ E ENTAO A VIA PRECISA DE UM COTO ATE AO PAD. A primeira versao pos as
    vias ao lado e nao as ligou a nada: o DRC devolveu `via_dangling` em cada
    uma. Cobre adjacente nao e cobre ligado — 0,9 mm de vazio isola tao bem
    como 9 mm. O coto e a ligacao, e sai na mesma camada do pad.

    ⚠️ E o teste de espaco olha pads E PISTAS. A primeira versao so olhou pads,
    e enfiou duas vias a 0,10 e 0,15 mm de pistas ja roteadas a mao.

    ⚠️⚠️ E SO SE COSTURA AO PLANO QUE PASSA POR BAIXO. Uma via desce a camada
    de baixo a procura do cobre da sua rede; se ali o cobre for de OUTRA rede,
    a via nao chega a nada e o DRC chama-lhe `via_dangling`. Aconteceu com o
    NT1 — a ponte entre AGND e GND — que esta em (132, 72), fora da ilha de
    AGND, que vai de x 22 a 75: uma via para um plano que ali nao existe.

    Isto **nao e um erro da costura, e um sintoma**: um pad de terra analogica
    fora da ilha da terra analogica e uma pergunta de desenho, nao de fabrico.
    A costura recusa a via e diz de quem e o pad, para a pergunta chegar a
    quem a pode responder."""
    import math
    t = re.sub(r'\n\t\(via[\s\S]*?\n\t\)', '', t)
    terras = {"GND", "AGND"}
    ocup = []
    for m in re.finditer(r'\n\t\(footprint ', t):
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
        a = re.search(r'\(at (' + NUM + r') (' + NUM + r')(?: (' + NUM + r'))?\)', bl)
        fx, fy = float(a.group(1)), float(a.group(2))
        ang = math.radians(float(a.group(3) or 0))
        for pm in re.finditer(r'\(pad "[^"]*" (\w+)[\s\S]*?\(at (' + NUM + r') ('
                              + NUM + r')(?: ' + NUM + r')?\)[\s\S]*?\(size (' + NUM
                              + r') (' + NUM + r')\)([\s\S]{0,400}?)\(uuid', bl):
            tipo, px, py, sw, sh, resto = pm.groups()
            px, py, sw, sh = float(px), float(py), float(sw), float(sh)
            n = re.search(r'\(net "([^"]+)"\)', resto)
            ocup.append((fx + px * math.cos(ang) + py * math.sin(ang),
                         fy - px * math.sin(ang) + py * math.cos(ang),
                         max(sw, sh) / 2, n.group(1) if n else None, tipo))
    pistas = []
    for sm in re.finditer(r'\(segment\s*\(start (' + NUM + r') (' + NUM + r')\)\s*'
                          r'\(end (' + NUM + r') (' + NUM + r')\)\s*\(width (' + NUM
                          + r')\)\s*\(layer "F\.Cu"\)', t):
        x1, y1, x2, y2, w = (float(v) for v in sm.groups())
        pistas.append((x1, y1, x2, y2, w / 2))

    def longe_das_pistas(vx, vy, folga=0.55):
        for x1, y1, x2, y2, r in pistas:
            dx, dy = x2 - x1, y2 - y1
            L2 = dx * dx + dy * dy
            u = 0.0 if L2 == 0 else max(0.0, min(1.0, ((vx - x1) * dx + (vy - y1) * dy) / L2))
            if math.hypot(vx - (x1 + u * dx), vy - (y1 + u * dy)) < r + 0.3 + folga:
                return False
        return True

    # onde e que o plano de cada rede realmente esta
    planos_de = {}
    for rede_z, camada, forma in ZONAS_COBRE.get(nome, []):
        if camada != "B.Cu":
            continue
        if forma == "ilha":
            planos_de[rede_z] = ILHA_AGND[nome]
        else:
            planos_de[rede_z] = None            # a placa toda

    def ha_plano(rede_p, x, y):
        if rede_p not in planos_de:
            return False
        cx = planos_de[rede_p]
        if cx is None:
            return True
        return cx[0] <= x <= cx[2] and cx[1] <= y <= cx[3]

    vias, postas, orfaos = [], [], []
    for x, y, r, rede, tipo in ocup:
        if rede not in terras or tipo == "thru_hole":
            continue          # o passante ja atravessa: nao precisa de via
        if not ha_plano(rede, x, y):
            orfaos.append((rede, x, y))
            continue
        for dx, dy in ((0, r + 0.9), (0, -(r + 0.9)), (r + 0.9, 0), (-(r + 0.9), 0),
                       (r + 1.3, r + 1.3), (-(r + 1.3), -(r + 1.3))):
            vx, vy = x + dx, y + dy
            if any(math.hypot(vx - a, vy - b) < c + 0.55 for a, b, c, _, _ in ocup):
                continue
            if any(math.hypot(vx - a, vy - b) < 1.1 for a, b in postas):
                continue
            if not longe_das_pistas(vx, vy):
                continue
            vias.append((vx, vy, rede, x, y))
            postas.append((vx, vy))
            break
    L = ""
    for vx, vy, rede, px, py in vias:
        L += ('\t(via\n\t\t(at %g %g)\n\t\t(size 0.6)\n\t\t(drill 0.3)\n'
              '\t\t(layers "F.Cu" "B.Cu")\n\t\t(net "%s")\n\t\t(uuid "%s")\n\t)\n'
              % (round(vx, 3), round(vy, 3), rede, u()))
        L += ('\t(segment\n\t\t(start %g %g)\n\t\t(end %g %g)\n\t\t(width 0.4)\n'
              '\t\t(layer "F.Cu")\n\t\t(net "%s")\n\t\t(uuid "%s")\n\t)\n'
              % (round(px, 3), round(py, 3), round(vx, 3), round(vy, 3), rede, u()))
    for rede_o, x, y in orfaos:
        print("     ⚠️ pad de %s em (%.1f, %.1f) esta FORA do plano de %s: nao "
              "costuro. Isto e uma pergunta de desenho." % (rede_o, x, y, rede_o))
    i = t.rstrip().rfind(')')
    return t[:i] + L + t[i:], len(vias)


def zona(rede, camada, pts, prio=0):
    v = "".join("\t\t\t\t(xy %g %g)\n" % p for p in pts)
    return ('\t(zone\n\t\t(net 0)\n\t\t(net_name "%s")\n\t\t(layer "%s")\n'
            '\t\t(uuid "%s")\n\t\t(hatch edge 0.5)\n\t\t(priority %d)\n'
            '\t\t(connect_pads\n\t\t\t(clearance 0.5)\n\t\t)\n'
            '\t\t(min_thickness 0.25)\n\t\t(filled_areas_thickness no)\n'
            '\t\t(fill\n\t\t\t(thermal_gap 0.5)\n\t\t\t(thermal_bridge_width 0.5)\n\t\t)\n'
            '\t\t(polygon\n\t\t\t(pts\n%s\t\t\t)\n\t\t)\n\t)\n'
            % (rede, camada, u(), prio, v))


def planos(t, nome, alvo):
    """Escreve as zonas de cobre. Substitui as que houver."""
    t = re.sub(r'\n\t\(zone[\s\S]*?\n\t\)(?=\n\t\(|\n\))', '', t)
    ox, oy = alvo["origem"]
    w, h = alvo["w"], alvo["h"]
    m = 0.3                                   # recuo ao contorno
    todo = [(ox + m, oy + m), (ox + w - m, oy + m),
            (ox + w - m, oy + h - m), (ox + m, oy + h - m)]
    L = []
    for rede, camada, forma in ZONAS_COBRE.get(nome, []):
        if forma == "ilha":
            x0, y0, x1, y1 = ILHA_AGND[nome]
            pts = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
            # ⚠️ prioridade MAIOR: a ilha ganha o espaco ao plano geral, senao
            # o GND enchia-o primeiro e a ilha ficava sem cobre.
            L.append(zona(rede, camada, pts, prio=1))
        else:
            L.append(zona(rede, camada, todo, prio=0))
    i = t.rstrip().rfind(')')
    return t[:i] + "".join(L) + t[i:], len(L)


# --- marca na serigrafia ----------------------------------------------------
# ⚠️ A COR NAO VIVE NO FICHEIRO. Serigrafia e uma camada; preto ou branco e uma
# opcao do pedido na JLCPCB, nao um atributo do `.kicad_pcb`. O que se faz aqui
# e po-la na camada certa; a cor escolhe-se no carrinho.
# ⚠️ VAI NA SERIGRAFIA DE TRAS. Tres razoes, e nenhuma e estetica:
#   · a frente esta cheia — a primeira tentativa poe-la em cima do C6 e deu
#     SESSENTA E QUATRO sobreposicoes de uma vez;
#   · a caixa e de acrilico TRANSPARENTE e a placa assenta na chapa de fundo:
#     o lado que se ve por baixo do instrumento e o de tras;
#   · e atras nao ha nada com que colidir, hoje nem depois.
# Texto em camada de tras leva `mirror`, senao le-se ao contrario no cobre.
# ⚠️ `mirror` E O CERTO para a face de tras: e assim que o texto se le quando se
# olha para as COSTAS da placa, que e o que se ve pelo fundo transparente da
# caixa. No editor, visto de frente, aparece invertido — e essa e a aparencia
# correcta, nao um defeito.
MARCA_LADO = "B.SilkS"
# ⚠️ COORDENADAS ABSOLUTAS, e no sitio que foi MEDIDO. A primeira versao punha a
# marca a meio da placa por fraccao — e "meio" atras sao os 130 pads passantes,
# que dao `silk_over_copper` e serigrafia por cima de solda. Atras nao ha
# serigrafia de pecas, mas ha PADS: o vazio de uma camada nao e o vazio da face.
# A faixa livre foi procurada por varrimento contra todos os pads passantes:
# 123 x 8 mm a partir de (73, 23).
MARCA = {
    "cerebro": [(140.0, 27.5, "COTUCATRONICS", 5.0, 0.8),
                # ⚠️ x=87 e nao 82: centrado em 82 o texto (13 caracteres a
                # 2,2 mm, ~20 mm de largura) comeca em x=72 e encosta ao pad
                # 26 do J14, em 70,48. O DRC apanha `silk_over_copper`, e o
                # que se perde e real: a JLC recorta a serigrafia sobre a
                # abertura de mascara, logo o inicio do texto desaparecia.
                (87.0, 27.0, "cerebro rev B", 2.2, 0.35)],
    "painel":  [(108.0, 26.0, "COTUCATRONICS", 3.5, 0.6),
                (60.0, 26.0, "painel rev A", 1.6, 0.25)],
}


def marca(t, nome, alvo):
    """Escreve a marca em F.SilkS. Substitui a que houver, para nao duplicar."""
    t = re.sub(r'\n\t\(gr_text "COTUCATRONICS"[\s\S]*?\n\t\)', '', t)
    t = re.sub(r'\n\t\(gr_text "(cerebro|painel) rev [AB]"[\s\S]*?\n\t\)', '', t)
    L = []
    for x, y, txt, h, esp in MARCA.get(nome, []):
        L.append('\t(gr_text "%s"\n\t\t(at %g %g)\n\t\t(layer "%s")\n'
                 '\t\t(uuid "%s")\n\t\t(effects\n\t\t\t(font\n\t\t\t\t'
                 '(size %g %g)\n\t\t\t\t(thickness %g)\n\t\t\t\t(bold yes)\n'
                 '\t\t\t)\n\t\t\t(justify mirror)\n\t\t)\n\t)\n'
                 % (txt, round(x, 3), round(y, 3), MARCA_LADO, u(), h, h, esp))
    i = t.rstrip().rfind(')')
    return t[:i] + "".join(L) + t[i:]


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


RAIO_FURO = 3.5      # M3 com anilha o6,5 / 2 + folga


def _courtyards(t):
    """[(ref, [(x,y), ...])] — o contorno de montagem de cada peca, em
    coordenadas da placa.

    ⚠️ `F.CrtYd` E NAO OS PADS. O courtyard e o corpo COM a folga de montagem
    ja incluida — e para isso que a biblioteca o desenha. Comparar com pads
    mede a coisa errada: um IDC 2x13 tem pads a 2,54 e um corpo de 42 mm, e foi
    assim que o furo `H1` deu "livre" com o parafuso debaixo do conector.
    """
    import math
    out = []
    i = 0
    while True:
        j = t.find('\n\t(footprint "', i)
        if j < 0:
            return out
        k, d, ins = j + 1, 0, False
        while k < len(t):
            c = t[k]
            if c == '"':
                ins = not ins
            elif not ins:
                if c == "(":
                    d += 1
                elif c == ")":
                    d -= 1
                    if d == 0:
                        break
            k += 1
        bl = t[j:k + 1]
        i = k + 1
        rm = re.search(r'\(property "Reference" "([^"]+)"', bl)
        am = re.search(r'\n\t\t\(at (' + NUM + r') (' + NUM + r')(?: (' + NUM + r'))?\)', bl)
        if not rm or not am:
            continue
        fx, fy = float(am.group(1)), float(am.group(2))
        ang = math.radians(float(am.group(3) or 0))
        pts = []
        for cm in re.finditer(r'\((?:fp_line|fp_rect|fp_poly|fp_circle)'
                              r'[\s\S]{0,600}?\(layer "F\.CrtYd"\)', bl):
            for c2 in re.finditer(r'\((?:xy|start|end|center) (' + NUM + r') ('
                                  + NUM + r')\)', cm.group(0)):
                px, py = float(c2.group(1)), float(c2.group(2))
                pts.append((fx + px * math.cos(ang) + py * math.sin(ang),
                            fy - px * math.sin(ang) + py * math.cos(ang)))
        if pts:
            out.append((rm.group(1), pts))


def confere_furos(t, alvo):
    """Os 4 furos o3,2 da chapa de fundo caem em zona livre da placa?

    ⚠️ Sao eles que a seguram. Um furo debaixo de um conector nao e um furo
    apertado: e um parafuso que nao entra depois da placa montada — e isso
    descobre-se com a caixa fechada e a solda feita.

    ⚠️⚠️ E O FURO NAO E UM PONTO: E UM CIRCULO DE 3,5 mm.
    Um M3 com anilha o6,5 pede cobre livre e o corpo do vizinho FORA desse
    circulo. A v0.14.0 comparava a distancia ao CENTRO e usava PADS: o `H1` deu
    "livre, vizinho J14 a +4,54" enquanto o centro do furo estava DENTRO do
    corpo do IDC, cujo courtyard vai de x 34,4 a 76,1. Duas medidas erradas ao
    mesmo tempo — o objecto (pads em vez de corpo) e a forma (ponto em vez de
    circulo) — e as duas davam o mesmo veredicto tranquilizador.
    """
    import math
    ox, oy = alvo["origem"]
    cys = _courtyards(t)
    if not cys:
        raise SystemExit("*** nenhum courtyard lido — LEITURA INCOMPLETA, e um "
                         "leitor que nao le nada aprova tudo")
    L, mau = [], []
    for ex, ey in FUNDO["espacadores"]:
        kx = ox + (ex - alvo["x0"])
        ky = oy + (alvo["h"] - (ey - alvo["y0"]))
        pior, quem = 1e9, "—"
        for ref, pts in cys:
            if ref.startswith("H"):
                continue          # os proprios furos nao se estorvam
            x0 = min(q[0] for q in pts); x1 = max(q[0] for q in pts)
            y0 = min(q[1] for q in pts); y1 = max(q[1] for q in pts)
            dx = max(x0 - kx, kx - x1, 0.0)
            dy = max(y0 - ky, ky - y1, 0.0)
            d = math.hypot(dx, dy)
            if d < pior:
                pior, quem = d, ref
        L.append("     furo o3,2 em KiCad (%6.2f,%6.2f)  %-7s  corpo de %s a %+.2f"
                 % (kx, ky, "livre" if pior >= RAIO_FURO else "OCUPADO", quem, pior))
        if pior < RAIO_FURO:
            mau.append("%s a %.2f mm do furo em (%.2f,%.2f), e o M3 pede %.1f"
                       % (quem, pior, kx, ky, RAIO_FURO))
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


def pads_de(bl):
    """Pads do footprint: (dx, dy, r) no referencial dele."""
    out = []
    for pm in re.finditer(r'\(pad [\s\S]*?\(at (' + NUM + r') (' + NUM + r')(?: ' + NUM
                          + r')?\)[\s\S]*?\(size (' + NUM + r') (' + NUM + r')\)', bl):
        px, py, sw, sh = map(float, pm.groups())
        out.append((px, py, max(sw, sh) / 2))
    return out


def colide(x, y, ang, pads, centros, raio):
    """A peca posta em (x, y) com rotacao `ang` toca algum disco?

    ⚠️ TESTE EXACTO, com os pads rodados. Um raio unico a volta da origem e
    conservador ao ponto de tornar a zona do soquete insoluvel: um header 1x6
    tem a origem no pino 1 e 13,5 mm de corpo, e o teste grosseiro assume que
    esses 13,5 apontam sempre para o disco. Apontam para onde a rotacao mandar,
    e isso sabe-se."""
    import math
    a = math.radians(ang or 0.0)
    for px, py, pr in pads:
        rx = x + px * math.cos(a) + py * math.sin(a)
        ry = y - px * math.sin(a) + py * math.cos(a)
        if any(math.hypot(rx - cx, ry - cy) <= raio + pr for cx, cy in centros):
            return True
    return False


# --- modulos que precisam do USB acessivel ----------------------------------
# ⚠️ Isto nao e uma ligacao: e um GESTO. Nenhum ERC, DRC ou netlist sabe que
# alguem tem de enfiar um cabo naquele conector com a caixa fechada. E a
# terceira coisa que a caixa impoe a placa e nenhuma ferramenta ve — depois do
# contorno (planta) e da coluna de ar (volume), o ACESSO.
ACESSO_USB = {
    # ref: (pinos que marcam a ponta onde esta o USB, altura do conector)
    "U2": ((1, 40), 1.5),
    # ⚠️ pinos por NOME, como tudo o que toca neste modulo. IO5 e 5V sao os dois
    # extremos da ponta do USB-C.
    "U3": (("IO5", "5V"), 3.2),
}
# tamanho da abertura na parede: tem de passar a FICHA e o dedo que a leva la,
# nao so o conector. Uma ficha USB-C tem ~12 x 6,5 de corpo e ~20 de pega.
ABERTURA = (18.0, 12.0)
PEDIDO = os.path.join(AQUI, os.pardir, "caixa", "aberturas.json")


def confere_acesso(t, alvo):
    """Cada USB que precisa de ser alcancado de fora bate num recorte?"""
    import math
    pedidos = []
    # ⚠️ TODOS os recortes largos, e nao so os de 12x6. O filtro estava preso ao
    # tamanho do recorte do decoy — e no dia em que a caixa ganhou uma abertura
    # de 18x12 pedida por esta mesma funcao, ela nao a viu e continuou a dizer
    # que nao havia acesso. Portao que so reconhece a forma que ja conhecia
    # nao e portao, e o modo de falha dele e dizer NAO quando devia dizer SIM.
    recortes = [(r[1] + CAIXA.T, r[2], r[3], r[4])
                for r in CAIXA.BACK_RECTS if r[3] >= 10]
    L = ["     ACESSO USB: %d recorte(s) na parede de tras" % len(recortes)]
    ox, oy = alvo["origem"]
    for ref, (pinos, alt) in ACESSO_USB.items():
        for m in re.finditer(r'\n\t\(footprint ', t):
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
            if re.search(r'\(property "Reference" "%s"' % ref, bl):
                break
        else:
            continue
        a = re.search(r'\(at (' + NUM + r') (' + NUM + r')(?: (' + NUM + r'))?\)', bl)
        fx, fy, rot = float(a.group(1)), float(a.group(2)), float(a.group(3) or 0)
        ang = math.radians(rot)
        pos = {}
        for pm in re.finditer(r'\(pad "([^"]+)" [\s\S]*?\(at (' + NUM + r') (' + NUM + r')', bl):
            n = pm.group(1)
            n = int(n) if n.isdigit() else n
            if n in pinos:
                px, py = float(pm.group(2)), float(pm.group(3))
                pos[n] = (fx + px * math.cos(ang) + py * math.sin(ang),
                          fy - px * math.sin(ang) + py * math.cos(ang))
        if len(pos) != 2:
            continue
        ux = sum(p[0] for p in pos.values()) / 2
        uy = sum(p[1] for p in pos.values()) / 2
        # KiCad -> placa -> fundo -> interior -> face
        bx, by = ux - ox, uy - oy
        if FRENTE_CEREBRO != "y_min":
            bx, by = alvo["w"] - bx, alvo["h"] - by
        ix = bx + alvo["x0"] + 0.5
        iy = by + alvo["y0"] + 0.5
        face_x = ix + CAIXA.T
        z = CAIXA.T + (CALCO_H + 4.0) + 1.6 + alt      # altura do conector
        recuo = CAIXA.BODY_D - iy
        L.append("     %s: USB na face x=%.1f · z=%.1f · a %.1f mm da parede de tras"
                 % (ref, face_x, z, recuo))
        pedidos.append(dict(ref=ref, x=round(face_x, 2), z=round(z, 2),
                            w=ABERTURA[0], h=ABERTURA[1], recuo=round(recuo, 1),
                            porque="USB-C de %s, para reprogramar montado" % ref))
        bate = [r for r in recortes
                if abs(r[0] - face_x) <= r[2] / 2 and abs(r[1] - z) <= r[3] / 2]
        if bate:
            L.append("     %s: alinha com o recorte em x=%.1f ✓" % (ref, bate[0][0]))
        else:
            perto = min(recortes, key=lambda r: abs(r[0] - face_x)) if recortes else None
            L.append("     ⚠️ %s NAO alcanca recorte nenhum." % ref)
            if perto:
                L.append("     ⚠️ o mais proximo esta a %.1f mm em x e %.1f em z, e "
                         "e do decoy (entrada de energia)."
                         % (abs(perto[0] - face_x), abs(perto[1] - z)))
            L.append("     ⚠️ com a caixa fechada, so tirando o fundo (6 parafusos).")
    # ⚠️ A DEPENDENCIA INVERTE-SE AQUI, e so aqui. A caixa impoe a placa o
    # contorno, os furos e o volume; a placa impoe a caixa as ABERTURAS, porque
    # so ela sabe onde os conectores calharam. Escrever as aberturas a mao na
    # parede era transcrever de um ficheiro para o outro — a familia de erro que
    # este projecto ja pagou tres vezes. Sai um pedido; a caixa le-o.
    import json
    io.open(PEDIDO, "w", encoding="utf-8").write(json.dumps(
        {"gerado_por": "pcb_contorno v%s" % VERSAO,
         "parede": "tras",
         "aberturas": pedidos}, indent=2, ensure_ascii=False) + "\n")
    L.append("     escrito caixa/aberturas.json com %d pedido(s)" % len(pedidos))
    return L


def discos_arcade(alvo):
    """Centros e raio dos discos dos arcade, em coordenadas do KiCad."""
    import math
    ang = math.radians(CAIXA.ANG)
    horiz = 35.0 * math.cos(ang)
    ox, oy = alvo["origem"]
    fx = sorted(h[1] + CAIXA.T for h in CAIXA.TOP_HOLES if h[3] == 30)
    by = horiz - CAIXA.T - 0.5 - alvo["y0"]
    r = max(h[3] for h in CAIXA.TOP_HOLES if h[3] == 30) / 2 + 3.0
    if FRENTE_CEREBRO == "y_min":
        return [(ox + (x - CAIXA.T - 0.5 - alvo["x0"]), oy + by) for x in fx], r
    return [(ox + alvo["w"] - (x - CAIXA.T - 0.5 - alvo["x0"]),
             oy + alvo["h"] - by) for x in fx], r


def evita_arcade(t, alvo, zonas):
    """Troca as pecas ALTAS que caem sob um arcade por pecas baixas da MESMA
    zona, ou por um lugar vago dela.

    ⚠️ TROCAR, e nao empurrar. A grelha das zonas e o que mantem o analogico
    junto e longe do digital — mexer numa peca para fora da sua zona desfaz a
    razao pela qual as zonas existem. Trocar duas pecas dentro da zona nao
    desfaz nada: a zona ja e uma grelha arbitraria por dentro.

    ⚠️ E E AUTOMATICO de proposito. Os arcade ja mudaram de sitio uma vez
    (60 -> 70 mm entre eixos) e voltarao a mudar se o pe do dB pedir. Uma lista
    de trocas escrita a mao envelhecia na primeira delas — a familia de erro da
    cicatriz 15ce."""
    import math
    centros, raio = discos_arcade(alvo)
    livre = folga_arcade(alvo)

    corpos = {}

    def corpo(ref):
        if ref not in corpos:
            for m in re.finditer(r'\n\t\(footprint ', t):
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
                    ra = re.search(r'\(at ' + NUM + r' ' + NUM + r' (' + NUM + r')\)', bl)
                    corpos[ref] = (pads_de(bl), float(ra.group(1)) if ra else 0.0)
                    break
            else:
                corpos[ref] = ([], 0.0)
        return corpos[ref]

    def sob(x, y, ref=None):
        if ref is None:
            return any(math.hypot(x - cx, y - cy) <= 15.0 for cx, cy in centros)
        pads, rot = corpo(ref)
        return colide(x, y, rot, pads, centros, 15.0 + 1.0)

    trocas = []
    for nome, x0, y0, larg, px, py, refs in zonas:
        cols = max(1, int(larg // px))
        # todos os lugares da zona, ocupados e vagos
        n = len(refs)
        # ⚠️⚠️ O MESMO FILTRO DOS FUROS QUE A `grelha` — senao esta funcao
        # devolve as pecas aos lugares que a outra tinha saltado.
        #
        # A docstring da `grelha` ja avisava disto por outras palavras: "ate o
        # `evita_arcade` correr, que recalcula a grelha e a devolve em silencio
        # ao sitio antigo". Eu li o aviso, pus o filtro num sitio so, e o `C6`
        # voltou para cima do furo sem nada se queixar.
        #
        # ⚠️ Duas funcoes que constroem a MESMA lista por conta propria e a
        # classe 14 da skill `audit` — corpo copiado em vez de chamado. A cura
        # de fundo e uma so fazer o que a outra ja faz; o filtro repetido aqui
        # e o remendo minimo, e fica dito que e remendo.
        _fur = furos_kicad(alvo)
        lugares = [(x0 + (i % cols) * px, y0 + (i // cols) * py)
                   for i in range(((n + cols - 1) // cols) * cols * 2)]
        lugares = [(lx, ly) for lx, ly in lugares
                   if not any(abs(hx - lx) < px / 2 + RAIO_FURO
                              and abs(hy - ly) < py / 2 + RAIO_FURO
                              for hx, hy in _fur)][:((n + cols - 1) // cols) * cols]
        ocupa = {i: refs[i] for i in range(n)}
        altura = {r: (altura_de(fp_de(t, r)) or 99.0) for r in refs}
        maus = [i for i in range(n)
                if sob(*lugares[i], ref=refs[i]) and altura[refs[i]] > livre]
        bons = [i for i in range(len(lugares))
                if (i >= n or altura[refs[i]] <= livre)]
        bons = [j for j in bons
                if all(not sob(*lugares[j], ref=refs[i]) for i in maus)]
        # ⚠️ o lugar escolhido e o MAIS AFASTADO das outras pecas altas, e nao o
        # primeiro livre. Duas THT no mesmo passo de 9 mm sobrepoem courtyards —
        # tirar uma peca de debaixo de um botao e po-la em cima de outra peca
        # nao e resolver, e mudar de defeito. O DRC apanha, mas so depois.
        altos = [k for k in range(n) if altura[refs[k]] > 5.0]
        for i in maus:
            if not bons:
                raise SystemExit(
                    "*** %s nao cabe sob um arcade e a zona '%s' nao tem lugar "
                    "livre fora dos discos. Isto ja nao e colocacao — e a placa "
                    "a nao caber por baixo dos botoes." % (refs[i], nome))

            def longe(j):
                lx, ly = lugares[j]
                d = [math.hypot(lx - lugares[k][0], ly - lugares[k][1])
                     for k in altos if k != i and k < len(lugares)]
                return min(d) if d else 999.0

            bons.sort(key=longe, reverse=True)
            j = bons.pop(0)
            outro = ocupa.get(j)
            ocupa[i], ocupa[j] = outro, ocupa[i]
            trocas.append((refs[i], lugares[i], lugares[j], outro or "(vago)"))
        # ⚠️ O TROCADOR VERIFICA-SE A SI PROPRIO. Com 8 refs e tres pecas altas
        # a disputarem dois lugares fora dos discos, ele poe duas no mesmo sitio
        # e nao se queixa — o DRC apanhava depois, com 46 violacoes e 11 furos
        # sobrepostos. Um arrumador que produz sobreposicao tem de o dizer ele,
        # e nao deixar o portao seguinte adivinhar de quem foi a culpa.
        postos = [r for r in ocupa.values() if r]
        if len(postos) != len(set(postos)):
            import collections
            rep = [r for r, n in collections.Counter(postos).items() if n > 1]
            raise SystemExit(
                "*** a zona '%s' ficou com %s em mais de um lugar. O trocador "
                "nao tinha lugares livres que chegassem: %d pecas altas sob os "
                "discos e %d lugares fora deles." % (nome, rep, len(maus), len(bons) + len(maus)))
        vistos = {}
        for i, ref in ocupa.items():
            if ref:
                xy = (float(lugares[i][0]), float(lugares[i][1]))
                if xy in vistos:
                    raise SystemExit("*** %s e %s no mesmo ponto %s da zona '%s'"
                                     % (vistos[xy], ref, xy, nome))
                vistos[xy] = ref
                t = move(t, ref, xy)
    return t, trocas


def folga_arcade(alvo):
    """Altura livre por baixo dos arcade, acima da face de cima da placa."""
    import math
    ang = math.radians(CAIXA.ANG)
    horiz = 35.0 * math.cos(ang)
    sob = CAIXA.HF + (CAIXA.HB - CAIXA.HF) * horiz / CAIXA.RUN - CAIXA.T
    return (sob - CAIXA.T) - CAIXA.T - (40.0 - 2 * CAIXA.T) - (CALCO_H + 4.0 + 1.6)


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
        ra = re.search(r'\(at ' + NUM + r' ' + NUM + r' (' + NUM + r')\)', bl)
        rot = float(ra.group(1)) if ra else 0.0
        if colide(bx, by, rot, pads_de(bl), [(a, ay) for a in ax], 15.0):
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
    t = marca(t, nome, alvo)
    t, n_z = planos(t, nome, alvo)
    # ⚠️ A COSTURA VEM DEPOIS DA COLOCACAO, e nao aqui. Ela le a posicao de cada
    # pad para saber onde por a via; se correr antes de as pecas serem movidas,
    # cose os pads onde eles AINDA NAO ESTAO e deixa as vias para tras.
    # Escapou por a placa ja chegar colocada — a segunda corrida e idempotente e
    # os numeros batem. So se ve numa peca que muda de sitio, e foi assim que
    # apareceu: mudei o NT1 e a costura continuou a falar do sitio antigo.
    # ⚠️ Um defeito de ordem que so aparece quando alguma coisa muda e um
    # defeito que passa em todos os testes menos no dia em que interessa.
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
        t, n = grelha(t, ZONAS_CEREBRO, UNICOS_CEREBRO, furos_kicad(alvo))
        print("     %d pecas colocadas por zona" % n, file=sys.stderr)
        t, trocas = evita_arcade(t, alvo, ZONAS_CEREBRO)
        for ref, de, para, outro in trocas:
            print("     arcade: %-4s sai de (%g,%g) para (%g,%g), troca com %s"
                  % (ref, de[0], de[1], para[0], para[1], outro), file=sys.stderr)
    relato += confere_sobreposicao(t)
    t, n_v = costura(t, nome)
    if alvo["assenta"] == "fundo":
        relato += confere_furos(t, alvo)
        # ⚠️ FRENTE_CEREBRO ainda nao esta decidido — ver a constante.
        relato += confere_arcade(t, alvo, FRENTE_CEREBRO)
        relato += confere_acesso(t, alvo)
    io.open(p, "w", encoding="utf-8").write(t)
    print("  %s: contorno %g x %g mm em (%g, %g)"
          % (nome, alvo["w"], alvo["h"], alvo["origem"][0], alvo["origem"][1]),
          file=sys.stderr)
    relato.append("     %d zona(s) de cobre em B.Cu · %d vias de costura" % (n_z, n_v))
    for l in relato:
        print(l, file=sys.stderr)
    for ref, furo, alvo_xy, eixo, xy in postos:
        print("     %-4s eixo em %-11s tampa%s -> KiCad %s · origem %s "
              "(eixo do footprint a %+g,%+g)"
              % (ref, furo, TAMPA[furo], alvo_xy, xy, eixo[0], eixo[1]),
              file=sys.stderr)


if __name__ == "__main__":
    main()
