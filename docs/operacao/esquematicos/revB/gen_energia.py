#!/usr/bin/env python3
#
# ⚠️ ESTE GERADOR NAO E MAIS A FONTE. A 04/09 o projeto passou a ser
# `kicad/cotuca_cerebro/` — um projeto KiCad a serio, com `.kicad_pro` e
# folha raiz, que e o que as folhas multiplas exigem. As copias soltas que
# viviam aqui foram removidas: havia TRES do `cotuca_energia.kicad_sch`, com
# ERC de 0, 15 e 66 violacoes, e ninguem sabia qual mandava.
#
# Este ficheiro fica pelo DESENHO que documenta — as escolhas, os porques e
# as medidas — e como ponto de partida se alguem quiser voltar a gerar por
# script. Nao regerar por cima do projeto sem falar com o dB.
#
# Gera a folha ENERGIA do carrier rev B em .kicad_sch, com simbolos
# auto-contidos (nenhuma biblioteca externa) + uma previa SVG para conferencia.
#
# ⚠️ O `.kicad_sch` NO REPO ESTA UM PASSO A FRENTE DESTE GERADOR.
#
# O `kiutils` escreve o formato `20211014` (KiCad 6). A 04/09 as duas folhas
# foram migradas com `kicad-cli sch upgrade` para `20260306` (KiCad 10.0.5),
# que e o que a maquina do dB abre.
#
# Se regerar, o ficheiro VOLTA a 20211014. O passo que falta a seguir e:
#
#     /Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli sch upgrade FICHEIRO
#
# Aceitacao da migracao, medida e nao suposta: ERC com a MESMA contagem de
# violacoes antes e depois, e netlist IDENTICO (redes e nos). Foi verificado
# nas duas folhas.
#
# ⚠️ E O GERADOR NAO CORRE NESTA MAQUINA: falta o `kiutils`
# (`pip3 install kiutils`). Enquanto assim for, o `.kicad_sch` e o unico
# artefacto vivo — o que inverte a regra §8 da skill `kicad` sem que nada
# no repo o dissesse.

import uuid, math
from kiutils.schematic import Schematic
from kiutils.items.schitems import SchematicSymbol, Connection, GlobalLabel, Junction
from kiutils.items.common import Position, Property, Effects, Font, Stroke, Justify
from kiutils.items.syitems import SyRect, SyPolyLine, SyCircle
from kiutils.items.common import Fill
from kiutils.symbol import Symbol, SymbolPin
from kiutils.items.schitems import Text as SchText

U = lambda: str(uuid.uuid4())
LIB = "cotuca"

def eff(size=1.27, jl=None, hide=False):
    e = Effects(font=Font(width=size, height=size), hide=hide)
    if jl: e.justify = Justify(horizontally=jl)
    return e

def pin(num, name, x, y, ang, length=2.54, etype="passive"):
    return SymbolPin(electricalType=etype, graphicalStyle="line",
                     position=Position(X=x, Y=y, angle=ang), length=length,
                     name=name, nameEffects=eff(1.27, hide=True),
                     number=num, numberEffects=eff(1.0, hide=True))

def rect(x1, y1, x2, y2):
    return SyRect(start=Position(X=x1, Y=y1), end=Position(X=x2, Y=y2),
                  stroke=Stroke(width=0.254), fill=Fill(type="none"))

def line(pts):
    return SyPolyLine(points=[Position(X=a, Y=b) for a, b in pts],
                      stroke=Stroke(width=0.254), fill=Fill(type="none"))

def circ(cx, cy, r):
    return SyCircle(center=Position(X=cx, Y=cy), radius=r,
                    stroke=Stroke(width=0.254), fill=Fill(type="none"))

def mksym(name, graphics, pins, ref="U", val=""):
    s = Symbol(libraryNickname=LIB, entryName=name, unitId=None)
    s.pinNamesHide = True; s.hidePinNumbers = True
    s.inBom = True; s.onBoard = True
    s.properties = [
        Property(key="Reference", value=ref, position=Position(X=0, Y=6.35, angle=0), effects=eff()),
        Property(key="Value",     value=val, position=Position(X=0, Y=-6.35, angle=0), effects=eff()),
        Property(key="Footprint", value="",  position=Position(X=0, Y=0, angle=0), effects=eff(hide=True)),
        Property(key="Datasheet", value="",  position=Position(X=0, Y=0, angle=0), effects=eff(hide=True)),
    ]
    # ⚠️ SEM SUFIXO NO `entryName` — o kiutils acrescenta `_unitId_styleId`
    # ele proprio. Ate 01/09 este ficheiro passava `f"{name}_0_1"` E os
    # ids, e o kiutils somava outra vez: saia `D_V_0_1_0_1`, com QUATRO
    # numeros. O KiCad separa o sufixo `_<unidade>_<estilo>` para deduzir
    # o nome do pai, encontrava `D_V_0_1` — que nao existe — e recusava a
    # FOLHA INTEIRA, sem uma linha de erro que apontasse para aqui.
    #
    # O `check_sch.py` nao apanha isto: ele confere a REDE, nao a
    # gramatica do KiCad. Verificador que passa e folha que nao abre sao
    # perguntas diferentes, e so uma estava a ser feita.
    u0 = Symbol(libraryNickname=None, entryName=name, unitId=0, styleId=1)
    u0.graphicItems = graphics
    u1 = Symbol(libraryNickname=None, entryName=name, unitId=1, styleId=1)
    u1.pins = pins
    s.units = [u0, u1]
    return s

# ---------------- biblioteca local ----------------
SYM = {}
SYM["CONN2_R"] = mksym("CONN2_R", [rect(-5.08, -3.81, 0, 3.81)],
    [pin("1", "1", 2.54, 2.54, 180), pin("2", "2", 2.54, -2.54, 180)], "J")
SYM["CONN2_L"] = mksym("CONN2_L", [rect(0, -3.81, 5.08, 3.81)],
    [pin("1", "1", -2.54, 2.54, 0), pin("2", "2", -2.54, -2.54, 0)], "J")
SYM["CONN3_L"] = mksym("CONN3_L", [rect(0, -6.35, 5.08, 6.35)],
    [pin("1", "1", -2.54, 5.08, 0), pin("2", "2", -2.54, 0, 0), pin("3", "3", -2.54, -5.08, 0)], "J")
SYM["FUSE_H"] = mksym("FUSE_H", [rect(-3.81, -1.27, 3.81, 1.27), line([(-3.81, 0), (3.81, 0)])],
    [pin("1", "1", -6.35, 0, 0), pin("2", "2", 6.35, 0, 180)], "F")
SYM["SW_H"] = mksym("SW_H",
    [circ(-3.81, 0, 0.635), circ(3.81, 0, 0.635), line([(-3.175, 0.635), (3.81, 3.175)])],
    [pin("1", "1", -6.35, 0, 0), pin("2", "2", 6.35, 0, 180)], "SW")
# diodo vertical: pino 1 = catodo em cima, pino 2 = anodo em baixo
SYM["D_V"] = mksym("D_V",
    [line([(-1.905, 1.27), (1.905, 1.27)]), line([(-1.905, -1.27), (1.905, -1.27), (0, 1.27), (-1.905, -1.27)])],
    [pin("1", "K", 0, 3.81, 270), pin("2", "A", 0, -3.81, 90)], "D")
# diodo horizontal: pino 1 = anodo esquerda, pino 2 = catodo direita
SYM["D_H"] = mksym("D_H",
    [line([(1.27, -1.905), (1.27, 1.905)]), line([(-1.27, 1.905), (-1.27, -1.905), (1.27, 0), (-1.27, 1.905)])],
    [pin("1", "A", -3.81, 0, 0), pin("2", "K", 3.81, 0, 180)], "D")
SYM["C_V"] = mksym("C_V", [line([(-2.54, 0.762), (2.54, 0.762)]), line([(-2.54, -0.762), (2.54, -0.762)])],
    [pin("1", "1", 0, 3.81, 270), pin("2", "2", 0, -3.81, 90)], "C")
SYM["CP_V"] = mksym("CP_V",
    [line([(-2.54, 0.762), (2.54, 0.762)]), line([(-2.54, -0.762), (2.54, -0.762)]),
     line([(-2.0, 2.2), (-1.0, 2.2)]), line([(-1.5, 1.7), (-1.5, 2.7)])],
    [pin("1", "+", 0, 3.81, 270), pin("2", "-", 0, -3.81, 90)], "C")

# ---- PWR_FLAG: declara QUEM ALIMENTA ----
#
# O ERC exige que toda rede com pino `power_in` tenha alguem a declarar
# `power_out`. Os nossos simbolos sao caseiros e nenhum o declara — a
# alimentacao chega por rotulo global, vinda de outra folha.
#
# A alternativa era ISENTAR os dois erros. O dB recusou, e com razao:
# "declarar quem alimenta e resposta; isentar e mandar o verificador
# olhar para o lado". Mesma regra do Schottky — peca que impede o erro
# vale mais que regra que o proibe.
#
# ⚠️ UM POR REDE, E SO UM. Dois `power_out` na MESMA rede sao conflito
# para o ERC, e trocariamos um erro por outro. A distribuicao entre as
# folhas segue a fonte de cada rede.
#
# Referencia comeca por `#`: convencao do KiCad para simbolo que nao
# entra na BOM. Ele nao e uma peca, e uma AFIRMACAO.
SYM["PWRFLAG"]=mksym("PWRFLAG",
                 [line([(0,0),(0,2.54)]),line([(-1.27,2.54),(1.27,2.54)]),
                  line([(-1.27,2.54),(0,3.81)]),line([(1.27,2.54),(0,3.81)])],
                 [pin("1","pwr",0,0,90,0,"power_out")],"#FLG")

PINPOS = {n: {p.number: (p.position.X, p.position.Y) for u in s.units if u.pins for p in u.pins}
          for n, s in SYM.items()}

# ---------------- colocacoes ----------------
PLACE = [
    # As duas bandeiras desta folha, nas FONTES de cada trilho: o +15V
    # sai do decoy PD, o +5V do step-down. Nao ha bandeira de GND aqui —
    # ela vive na folha do pre-amp, que e onde ha pino `power_in` nessa
    # rede. Um `power_out` por rede, e SO um: dois sao conflito, e
    # trocariamos um erro por outro.
    # ⚠️ NUM PONTO LIVRE DO FIO, e NAO na coordenada do rotulo.
    # A primeira versao punha-as exactamente onde o rotulo global esta,
    # e o corpo da bandeira tapava-o: o `+15V` lia-se "15V" e o `+5V`
    # ficava com o triangulo dentro do hexagono. Electricamente certo,
    # visualmente ilegivel — e o render foi que o mostrou, porque
    # sobreposicao e propriedade do desenho, nao do fonte.
    ("#FLG1", "PWRFLAG", "", 128, 45.46),   # +15V, no fio SW1->rotulo
    ("#FLG2", "PWRFLAG", "", 100, 162),     # +5V,  no fio J3.3->rotulo
    ("J1",  "CONN2_R", "entrada decoy PD",  35,  48),
    ("F1",  "FUSE_H",  "PTC 3A",            62,  45.46),
    ("D1",  "D_V",     "SMBJ24A",           85,  55),
    ("SW1", "SW_H",    "latching",         110,  45.46),
    ("J5",  "CONN2_L", "LED do SW1",       170,  65),
    ("C1",  "CP_V",    "2200u/35V",         58, 106),
    ("C2",  "C_V",     "100n",              80, 106),
    ("J2",  "CONN2_L", "ao amp TPA3116",   115, 100.54),
    ("J3",  "CONN3_L", "step-down 5V fixo",115, 148),
    ("D2",  "D_H",     "1N5819",            62, 195),
    ("C3",  "CP_V",    "100u",              88, 203),
    ("C4",  "C_V",     "100n",             110, 203),
    ("J4",  "CONN2_L", "a ponte ESP32",    145, 197.54),
]

def P(ref, num):
    for r, t, v, x, y in PLACE:
        if r == ref:
            lx, ly = PINPOS[t][num]
            return (round(x + lx, 3), round(y - ly, 3))
    raise KeyError(ref)

W = []
def w(*pts): W.append([(round(a, 3), round(b, 3)) for a, b in pts])

# --- cadeia de entrada ---
w(P("J1", "1"), P("F1", "1"))
w(P("F1", "2"), (85, 45.46))
w((85, 45.46), P("D1", "1"))
w((85, 45.46), P("SW1", "1"))
w(P("D1", "2"), (85, 72))
w(P("J1", "2"), (46, 50.54), (46, 72))
w(P("SW1", "2"), (140, 45.46))
w((140, 45.46), (140, 62.46), P("J5", "1"))
w(P("J5", "2"), (182, 67.54), (182, 80))

# --- ramo do amplificador ---
w((35, 98), (58, 98))
w((58, 98), P("C1", "1"))
w((58, 98), (80, 98))
w((80, 98), P("C2", "1"))
w((80, 98), P("J2", "1"))
w(P("C1", "2"), (58, 120))
w(P("C2", "2"), (80, 120))
w((58, 120), (80, 120))
w((80, 120), (80, 127))
w(P("J2", "2"), (130, 103.08), (130, 120), (80, 120))

# --- ramo do step-down ---
w((35, 142.92), P("J3", "1"))
w(P("J3", "2"), (85, 148), (85, 167))
w(P("J3", "3"), (100, 153.08), (100, 172))

# --- ponte ESP32 ---
w((35, 195), P("D2", "1"))
w(P("D2", "2"), (88, 195))
w((88, 195), P("C3", "1"))
w((88, 195), (110, 195))
w((110, 195), P("C4", "1"))
w((110, 195), P("J4", "1"))
w(P("C3", "2"), (88, 215))
w(P("C4", "2"), (110, 215))
w((88, 215), (110, 215))
w((110, 215), (110, 222))
w(P("J4", "2"), (160, 200.08), (160, 215), (110, 215))

JUNC = [(85, 45.46), (140, 45.46), (58, 98), (80, 98), (80, 120),
        (88, 195), (110, 195), (110, 215)]

# ⚠️ JUNCAO SOB CADA BANDEIRA — sem ela o pino NAO LIGA.
#
# Medido em 01/09: pino que cai no MEIO de um segmento de fio nao se
# liga no KiCad; ele liga em PONTA de fio ou onde ha juncao explicita.
# As bandeiras estao a meio do fio de proposito (na coordenada do
# rotulo tapavam-no), entao precisam da juncao.
#
# O ERC apanhou-o como `pin_not_connected`, e so o apanhou porque
# alguem o voltou a correr: eu tinha movido as bandeiras, re-renderizado
# e OLHADO o desenho — mas nao repeti a verificacao electrica. Editar
# invalida TODAS as inspeccoes anteriores, nao so a visual.
JUNC.extend([(128, 45.46), (100, 162)])

# As duas bandeiras desta folha, nas FONTES de cada trilho: o +15V sai
# do decoy PD, o +5V sai do step-down. Nao ha bandeira de GND aqui —
# ela vive na folha do pre-amp, que e onde ha pino `power_in` nela. Um
# `power_out` por rede, e so um.
GLBL = [
    ("+15V", 140, 45.46, 0), ("+15V", 35, 98, 180), ("+15V", 35, 142.92, 180),
    ("+5V", 100, 172, 270), ("+5V", 35, 195, 180),
    ("GND", 85, 72, 270), ("GND", 46, 72, 270), ("GND", 182, 80, 270),
    ("GND", 80, 127, 270), ("GND", 85, 167, 270), ("GND", 110, 222, 270),
]

_n = ["ENERGIA - Cotucatronics carrier rev B",
 "1  Decoy PD sempre energizado: a negociacao acontece com SW1 desligado. Ligar depois nao renegoceia.",
 "2  SW1 latching, LED em WIRING A (a jusante do contacto) = indicador de TRILHO VIVO, nao de gesto.",
 "   Powerbank que nao negoceia fica em 5V, e o LED denuncia em vez de os amps ficarem mudos sem razao.",
 "3  C1 2200u/35V low-ESR + C2 100n colados no TERMINAL do amp. Medido 31/08: sem banco, o exciter zumbe.",
 "4  Modulos FORA da placa: decoy PD, step-down, TPA3116 estereo. Todos com terminais de parafuso.",
 "5  SEM banco na entrada do step-down: tentado em 31/08 e desfeito.",
 "6  Step-down de saida FIXA, por decisao. Modulo com regime selecionavel poe 24V a um jumper do VIN da Daisy,",
 "   e numa serie de dezenas uma seleccao que pode estar errada vai estar errada nalguma.",
 "7  D2 1N5819: sem ele, o USB da ponte realimenta o trilho de 5V inteiro (medido 31/08). Faixa = catodo,",
 "   virada para a ponte. Cerca de 0,3V de queda; a ponte aceita 4,3-6,0V. Medido: 4,72V no VIN dela.",
 "8  Orcamento do trilho de 5V: Daisy 300 + ponte 100 (pico 300) + OLED 30 + LEDs 2x30 = ~490 mA de ~1,5A.",
 "   O resistor de serie dos LEDs, dimensionado para ~30 mA, e o que mantem a folga em 3x.",
]
NOTES = [(_n[0], 25, 18, 2.5)] + [(t, 25, 236 + 4.6 * i, 1.5) for i, t in enumerate(_n[1:])]

# ---------------- montagem ----------------
sch = Schematic.create_new()
sch.uuid = U()
from kiutils.items.common import PageSettings

# ⚠️ ESCREVE AO LADO DE SI PROPRIO, e nao num caminho absoluto.
# Ate 01/09 estava `/mnt/user-data/outputs/`, que e o disco da maquina
# onde ele foi escrito e nao existe em mais lado nenhum: o gerador
# rebentava com FileNotFoundError na primeira tentativa de o correr aqui.
# Fonte que so corre na maquina do autor nao e fonte — e um artefacto
# com codigo ao lado.
import pathlib
AQUI = pathlib.Path(__file__).resolve().parent

sch.paper = PageSettings(paperSize="A3")
sch.libSymbols = list(SYM.values())

for ref, typ, val, x, y in PLACE:
    s = SchematicSymbol(libraryNickname=LIB, entryName=typ,
                        position=Position(X=x, Y=y, angle=0), unit=1,
                        inBom=True, onBoard=True, uuid=U())
    s.properties = [
        # ⚠️ REFERENCIA `#...` NAO SE MOSTRA — convencao do KiCad para
        # simbolo que nao e peca. Ver a nota gemea em gen_preamp.py: no
        # render de 01/09 a etiqueta da bandeira caia por cima do
        # rotulo global. O corpo estava livre; era o texto que colidia.
        Property(key="Reference", value=ref, position=Position(X=x, Y=y - 11, angle=0),
                 effects=eff(1.27, "left", ref.startswith("#"))),
        Property(key="Value",     value=val, position=Position(X=x, Y=y + 13, angle=0), effects=eff(1.0, "left")),
        Property(key="Footprint", value="", position=Position(X=x, Y=y, angle=0), effects=eff(hide=True)),
        Property(key="Datasheet", value="", position=Position(X=x, Y=y, angle=0), effects=eff(hide=True)),
    ]
    sch.schematicSymbols.append(s)

for pts in W:
    for a, b in zip(pts, pts[1:]):
        if a == b: continue
        sch.graphicalItems.append(Connection(type="wire",
            points=[Position(X=a[0], Y=a[1]), Position(X=b[0], Y=b[1])],
            stroke=Stroke(width=0), uuid=U()))

for x, y in JUNC:
    sch.junctions.append(Junction(position=Position(X=x, Y=y), uuid=U()))

for t, x, y, a in GLBL:
    sch.globalLabels.append(GlobalLabel(text=t, shape="bidirectional",
        position=Position(X=x, Y=y, angle=a), effects=eff(1.27), uuid=U()))

for t, x, y, sz in NOTES:
    sch.texts.append(SchText(text=t, position=Position(X=x, Y=y, angle=0), effects=eff(sz, "left"), uuid=U()))


# ---- ENCAIXE NA GRADE, imediatamente antes de escrever ----
# Ver grade.py para o porque. So a COLOCACAO e os fios; o desenho dentro
# dos simbolos fica intacto, porque os pinos internos ja estao na grade.
from grade import encaixa, confere_colisoes

_antes, _depois = [], []
def _enc_lista(itens, attr="position"):
    for it in itens or []:
        p = getattr(it, attr, None)
        if p is None:
            continue
        _antes.append((p.X, p.Y)); encaixa(p); _depois.append((p.X, p.Y))

_enc_lista(sch.schematicSymbols)
_enc_lista(sch.junctions)
_enc_lista(sch.labels)
_enc_lista(sch.globalLabels)
for _w in (sch.graphicalItems or []):
    for _p in (getattr(_w, "points", None) or []):
        _antes.append((_p.X, _p.Y)); encaixa(_p); _depois.append((_p.X, _p.Y))
confere_colisoes(_antes, _depois, __file__)
print("grade: %d pontos encaixados em 1,27 mm" % len(_antes))

sch.to_file(str(AQUI / "cotuca_energia.kicad_sch"))
print("escrito:", AQUI / "cotuca_energia.kicad_sch")
print("kicad_sch escrito")

# ---------------- previa SVG (conferencia visual) ----------------
S = 3.2
def sx(v): return round((v - 10) * S, 2)
out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{sx(430)}" height="{sx(300)}" font-family="monospace"><rect width="100%" height="100%" fill="#fff"/>']
for pts in W:
    for a, b in zip(pts, pts[1:]):
        out.append(f'<line x1="{sx(a[0])}" y1="{sx(a[1])}" x2="{sx(b[0])}" y2="{sx(b[1])}" stroke="#0a0" stroke-width="1.6"/>')
for ref, typ, val, x, y in PLACE:
    for u in SYM[typ].units:
        for g in (u.graphicItems or []):
            if isinstance(g, SyRect):
                x1, y1 = sx(x + g.start.X), sx(y - g.start.Y); x2, y2 = sx(x + g.end.X), sx(y - g.end.Y)
                out.append(f'<rect x="{min(x1,x2)}" y="{min(y1,y2)}" width="{abs(x2-x1)}" height="{abs(y2-y1)}" fill="none" stroke="#800" stroke-width="1.4"/>')
            elif isinstance(g, SyPolyLine):
                d = " ".join(f'{sx(x+p.X)},{sx(y-p.Y)}' for p in g.points)
                out.append(f'<polyline points="{d}" fill="none" stroke="#800" stroke-width="1.4"/>')
            elif isinstance(g, SyCircle):
                out.append(f'<circle cx="{sx(x+g.center.X)}" cy="{sx(y-g.center.Y)}" r="{g.radius*S}" fill="none" stroke="#800" stroke-width="1.4"/>')
        for p in (u.pins or []):
            px, py = sx(x + p.position.X), sx(y - p.position.Y)
            out.append(f'<circle cx="{px}" cy="{py}" r="2.2" fill="#c00"/>')
    out.append(f'<text x="{sx(x-4)}" y="{sx(y-11)}" font-size="11" font-weight="bold">{ref}</text>')
    out.append(f'<text x="{sx(x-4)}" y="{sx(y+14)}" font-size="9" fill="#444">{val}</text>')
for x, y in JUNC:
    out.append(f'<circle cx="{sx(x)}" cy="{sx(y)}" r="3" fill="#0a0"/>')
for t, x, y, a in GLBL:
    anc = "start" if a == 0 else ("end" if a == 180 else "middle")
    dy = 12 if a == 270 else 0
    out.append(f'<text x="{sx(x)}" y="{sx(y)+dy}" font-size="11" fill="#00c" text-anchor="{anc}" font-weight="bold">{t}</text>')
for t, x, y, sz in NOTES:
    out.append(f'<text x="{sx(x)}" y="{sx(y)}" font-size="{max(9,sz*7)}" fill="#111">{t}</text>')
out.append("</svg>")
open(str(AQUI / "energia_preview.svg"), "w").write("\n".join(out))
print("svg escrito")
