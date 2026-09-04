#!/usr/bin/env python3
# Folha PRE-AMP do carrier rev B: dois canais + polarizacao e alimentacao
# partilhadas. Simbolos auto-contidos. Gera .kicad_sch + previa SVG.
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

import uuid, importlib.util
from kiutils.schematic import Schematic
from kiutils.items.schitems import SchematicSymbol, Connection, GlobalLabel, Junction, Text as SchText
from kiutils.items.common import Position, Property, Effects, Font, Stroke, Justify, PageSettings, Fill
from kiutils.items.syitems import SyRect, SyPolyLine, SyCircle
from kiutils.symbol import Symbol, SymbolPin

# ⚠️ ESCREVE AO LADO DE SI PROPRIO, e nao num caminho absoluto.
# Ate 01/09 estava `/mnt/user-data/outputs/`, que e o disco da maquina
# onde ele foi escrito e nao existe em mais lado nenhum: o gerador
# rebentava com FileNotFoundError na primeira tentativa de o correr aqui.
# Fonte que so corre na maquina do autor nao e fonte — e um artefacto
# com codigo ao lado.
import pathlib
AQUI = pathlib.Path(__file__).resolve().parent


U = lambda: str(uuid.uuid4()); LIB = "cotuca"

def eff(size=1.27, jl=None, hide=False):
    e = Effects(font=Font(width=size, height=size), hide=hide)
    if jl: e.justify = Justify(horizontally=jl)
    return e
def pin(num, name, x, y, ang, length=2.54, et="passive"):
    return SymbolPin(electricalType=et, graphicalStyle="line",
        position=Position(X=x, Y=y, angle=ang), length=length,
        name=name, nameEffects=eff(1.0, hide=True), number=num, numberEffects=eff(1.0, hide=True))
def rect(x1,y1,x2,y2): return SyRect(start=Position(X=x1,Y=y1), end=Position(X=x2,Y=y2), stroke=Stroke(width=0.254), fill=Fill(type="none"))
def line(pts):        return SyPolyLine(points=[Position(X=a,Y=b) for a,b in pts], stroke=Stroke(width=0.254), fill=Fill(type="none"))

def mksym(name, gr, pins, ref="U"):
    s = Symbol(libraryNickname=LIB, entryName=name, unitId=None)
    s.pinNamesHide=True; s.hidePinNumbers=True; s.inBom=True; s.onBoard=True
    s.properties=[Property(key="Reference",value=ref,position=Position(X=0,Y=7.62,angle=0),effects=eff()),
                  Property(key="Value",value="",position=Position(X=0,Y=-7.62,angle=0),effects=eff()),
                  Property(key="Footprint",value="",position=Position(X=0,Y=0,angle=0),effects=eff(hide=True)),
                  Property(key="Datasheet",value="",position=Position(X=0,Y=0,angle=0),effects=eff(hide=True))]
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
    u0=Symbol(libraryNickname=None,entryName=name,unitId=0,styleId=1); u0.graphicItems=gr
    u1=Symbol(libraryNickname=None,entryName=name,unitId=1,styleId=1); u1.pins=pins
    s.units=[u0,u1]; return s

SYM={}
SYM["R_H"]=mksym("R_H",[rect(-3.81,-1.27,3.81,1.27)],[pin("1","1",-6.35,0,0),pin("2","2",6.35,0,180)],"R")
SYM["R_V"]=mksym("R_V",[rect(-1.27,-3.81,1.27,3.81)],[pin("1","1",0,6.35,270),pin("2","2",0,-6.35,90)],"R")
SYM["RV_H"]=mksym("RV_H",[rect(-3.81,-1.27,3.81,1.27),line([(-2.2,3.4),(2.2,-1.9)]),line([(2.2,-1.9),(0.6,-1.5)]),line([(2.2,-1.9),(1.6,-0.4)])],
                  [pin("1","1",-6.35,0,0),pin("2","2",6.35,0,180)],"RV")
SYM["C_H"]=mksym("C_H",[line([(-0.762,-2.54),(-0.762,2.54)]),line([(0.762,-2.54),(0.762,2.54)])],
                 [pin("1","1",-3.81,0,0),pin("2","2",3.81,0,180)],"C")
SYM["C_V"]=mksym("C_V",[line([(-2.54,0.762),(2.54,0.762)]),line([(-2.54,-0.762),(2.54,-0.762)])],
                 [pin("1","1",0,3.81,270),pin("2","2",0,-3.81,90)],"C")
SYM["CP_V"]=mksym("CP_V",[line([(-2.54,0.762),(2.54,0.762)]),line([(-2.54,-0.762),(2.54,-0.762)]),
                          line([(-2.2,2.2),(-1.2,2.2)]),line([(-1.7,1.7),(-1.7,2.7)])],
                 [pin("1","+",0,3.81,270),pin("2","-",0,-3.81,90)],"C")
SYM["JACK3"]=mksym("JACK3",[rect(-6.35,-6.35,0,6.35),line([(-4,4),(-1.5,4)]),line([(-4,0),(-1.5,0)]),line([(-4,-4),(-1.5,-4)])],
                 [pin("1","T",2.54,5.08,180),pin("2","R",2.54,0,180),pin("3","S",2.54,-5.08,180)],"J")
SYM["BAV99"]=mksym("BAV99",[rect(-5.08,-3.81,5.08,3.81),line([(-2.54,-2.2),(-2.54,2.2)]),line([(-4.2,2.2),(-0.9,2.2),(-2.54,-1.0),(-4.2,2.2)]),
                            line([(2.54,2.2),(2.54,-2.2)]),line([(0.9,-2.2),(4.2,-2.2),(2.54,1.0),(0.9,-2.2)])],
                 [pin("3","C",0,6.35,270),pin("1","A",-2.54,-6.35,90),pin("2","K",2.54,-6.35,90)],"D")
SYM["OPA"]=mksym("OPA",[line([(-7.62,7.62),(-7.62,-7.62),(7.62,0),(-7.62,7.62)])],
                 [pin("1","-",-10.16,2.54,0,2.54,"input"),pin("2","+",-10.16,-2.54,0,2.54,"input"),pin("3","O",10.16,0,180,2.54,"output")],"U")
SYM["OPAPWR"]=mksym("OPAPWR",[rect(-5.08,-5.08,5.08,5.08)],
                 [pin("8","V+",0,7.62,270,2.54,"power_in"),pin("4","V-",0,-7.62,90,2.54,"power_in")],"U")
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

PINPOS={n:{p.number:(p.position.X,p.position.Y) for u in s.units if u.pins for p in u.pins} for n,s in SYM.items()}

PLACE=[]; W=[]; JUNC=[]; GLBL=[]; NOTES=[]
LP={}
def add(ref,typ,val,x,y,lp=None):
    PLACE.append((ref,typ,val,x,y))
    if lp: LP[ref]=lp
def P(ref,num):
    for r,t,v,x,y in PLACE:
        if r==ref:
            lx,ly=PINPOS[t][num]; return (round(x+lx,3), round(y-ly,3))
    raise KeyError(ref)
def w(*pts): W.append([(round(a,3),round(b,3)) for a,b in pts])

# ================= UM CANAL =================
def canal(sfx, y0, audio):
    R=lambda n: f"{n}{sfx}"
    row=y0+20
    add(R("J"),"JACK3", f"piezo {sfx}",     30, y0+25.08)
    add(R("R2"),"R_H",  "1k",                58, row)
    add(R("R1"),"R_V",  "4M7",               85, y0+32)
    add(R("D"),"BAV99", "BAV99",            112, y0+34)
    add(R("C2"),"C_V",  "1n C0G",           140, y0+32)
    add(f"U1{sfx}","OPA","OPA1642",         182, y0+24)
    add(R("RV"),"RV_H", "trim 100k",        205, y0+6)
    add(R("C1"),"C_H",  "22p C0G",          205, y0-8, "u")
    add(R("R3"),"R_H",  "4k7",              150, y0+10, "ha")
    add(R("C3"),"CP_V", "22u NP",           128, y0+4)
    add(R("R4"),"R_H",  "100R",             232, y0+24)
    add(R("C4"),"C_H",  "2u2 filme",        262, y0+24)

    w(P(R("J"),"1"), P(R("R2"),"1"))
    w(P(R("J"),"2"), (36, y0+25.08), (36, y0+50))
    w(P(R("J"),"3"), (50, y0+30.16), (50, y0+50))
    w(P(R("R2"),"2"), (160, row))
    w((85,row), P(R("R1"),"1")); w(P(R("R1"),"2"), (85, y0+50))
    w((112,row), P(R("D"),"3"))
    w(P(R("D"),"1"), (104, y0+40.35), (104, y0+56))
    w(P(R("D"),"2"), (120, y0+40.35), (120, y0+56))
    w((140,row), P(R("C2"),"1")); w(P(R("C2"),"2"), (140, y0+50))
    w((160,row), (160, y0+26.54), P(f"U1{sfx}","2"))
    # -in: as duas cargas saem na MESMA direccao (conserto C1 da skill)
    w(P(f"U1{sfx}","1"), (171.84, y0+6))
    w((171.84, y0+10), P(R("R3"),"2"))
    w(P(R("R3"),"1"), (128, y0+10), P(R("C3"),"2"))
    w(P(R("C3"),"1"), (128, y0-4))
    w((171.84, y0+6), P(R("RV"),"1"))
    w(P(R("RV"),"2"), (222, y0+6), (222, y0+24))
    w((190, y0+6), (190, y0-8), P(R("C1"),"1"))
    w(P(R("C1"),"2"), (222, y0-8), (222, y0+6))
    w(P(f"U1{sfx}","3"), P(R("R4"),"1"))
    w(P(R("R4"),"2"), P(R("C4"),"1"))
    w(P(R("C4"),"2"), (300, y0+24))
    JUNC.extend([(85,row),(112,row),(140,row),(160,row),
                 (171.84,y0+10),(171.84,y0+6),(190,y0+6),(222,y0+6),(222,y0+24)])
    GLBL.extend([("GND",36,y0+50,270),("GND",50,y0+50,270),("VBIAS",85,y0+50,270),
                 ("GND",104,y0+56,270),("V+PRE",120,y0+56,270),("GND",140,y0+50,270),
                 ("VBIAS",128,y0-4,90),(audio,300,y0+24,0)])
    NOTES.append((f"CANAL {sfx}", 25, y0-2, 2.0))

canal("A", 45, "AUDIO_IN_L")
canal("B", 150, "AUDIO_IN_R")

# ================= PARTILHADO =================
add("U1C","OPAPWR","OPA1642 8/4", 60, 258)
add("C8","C_V","10u",   90, 258)
add("C9","C_V","100n", 115, 258)
add("R7","R_H","100R 1/4W", 175, 243)
add("C6","CP_V","470u 25V", 205, 251)
add("C7","C_V","100n",      232, 251)
add("R5","R_V","100k",      300, 245)
add("R6","R_V","100k",      300, 275)
add("C5","CP_V","47u",      335, 260)

w((45,250.38), P("U1C","8")); w((45,250.38),(45,238))
w(P("U1C","4"), (60,278))
w((90,250.38), P("C8","1")); w(P("C8","2"), (90,278))
w((115,250.38), P("C9","1")); w(P("C9","2"), (115,278))
w((60,250.38),(115,250.38))
w((60,278),(115,278)); w((115,278),(115,286))
w((150,243), P("R7","1"))
w(P("R7","2"), (255,243))
w((205,243), P("C6","1")); w(P("C6","2"), (205,266))
w((232,243), P("C7","1")); w(P("C7","2"), (232,266))
w((205,266),(232,266)); w((232,266),(232,276))
w((300,232), P("R5","1"))
w(P("R5","2"), (300,260))
w((300,260), P("R6","1")); w(P("R6","2"), (300,288))
w((300,260), P("C5","1")); w(P("C5","2"), (335,280))
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
JUNC.extend([(45,246),(60,272)])   # sob as bandeiras
JUNC.extend([(90,250.38),(115,250.38),(205,243),(232,243),(232,266),(300,260),(90,278),(115,278)])
# As duas bandeiras desta folha, nos nos que os rotulos ja ocupam.
# ⚠️ NUM PONTO LIVRE DO FIO, e nao na coordenada do rotulo — ver a nota
# em gen_energia.py: posta sobre o rotulo, a bandeira tapa-o.
add("#FLG1","PWRFLAG","",45,246)     # V+PRE — no fio U1C.8 -> rotulo
add("#FLG2","PWRFLAG","",60,272)     # GND   — no fio U1C.4 -> rotulo
GLBL.extend([("V+PRE",45,238,90),("GND",60,278,270),("GND",115,286,270),
             ("+15V",150,243,180),("V+PRE",255,243,0),("GND",232,276,270),
             ("V+PRE",300,232,90),("VBIAS",300,260,0),("GND",300,288,270),("GND",335,280,270)])

_n=["PRE-AMP DE PIEZO - Cotucatronics carrier rev B",
 "1  Ganho 1..22 (0..27 dB) no trimpot. E AJUSTE DE EXEMPLAR - mesma categoria do gov_ceiling: nao entra em preset",
 "   nem se copia entre unidades. Cada corpo da serie tem montagem de piezo diferente.",
 "2  A ENCENACAO e o ponto, nao o ganho: pre-amp +20 dB com pre_level_ab a descer 20 dB deixa o ganho de LACO onde",
 "   esta e sobe o sinal 20 dB acima do ruido captado. Ganho cedo, atenuacao tarde.",
 "3  R1 4M7 com piezo de ~20 nF corta em 1,7 Hz. Entrada de 10k cortaria em 800 Hz - foi por isso que o pre de",
 "   linha (AD828) foi rejeitado: pre de linha come o grave.",
 "4  JFET obrigatorio. Entrada bipolar traz ruido de CORRENTE, que vezes impedancia alta domina tudo.",
 "   OPA1642 5,1 nV/rtHz = 0,72 uV RMS integrados; TL072 18 nV/rtHz = 2,5 uV. Onze dB, que e a margem em jogo.",
 "5  CERAMICO X7R E PIEZOELECTRICO. Num corpo que vibra vira microfone. C0G nos pequenos, FILME no acoplamento",
 "   de saida (C4). Nunca X7R onde passa sinal.",
 "6  C1 22p e C2 1n cortam RF antes do ADC - inclui a comutacao a 400 kHz do classe D, medida acoplada em 01/09.",
 "7  Alimentacao pelo MESMO trilho de 15V do amp, filtrada por R7/C6/C7. Medido em 01/09 em tres topologias:",
 "   separar as fontes PIORA (8-16 kHz: +9,6 -> +17,3 -> +24,3 dB). Trilho unico + filtro local, nao separacao.",
 "8  Layout: pre-amp encostado aos jacks de entrada, dentro da ILHA DE AGND, pista de entrada curta.",
 "   Cabo do piezo blindado, malha ao AGND SO do lado da placa - nas duas pontas fecha laco.",
 "9  Anel do TRS ao AGND junto do sleeve: blindagem de graca no cabo."]
NOTES.append((_n[0], 25, 22, 2.5))
NOTES.extend([(t, 25, 306 + 5.0*i, 1.5) for i,t in enumerate(_n[1:])])

# ================= MONTAGEM =================
sch = Schematic.create_new(); sch.uuid=U(); sch.paper=PageSettings(paperSize="A2")
sch.libSymbols=list(SYM.values())
for ref,typ,val,x,y in PLACE:
    s=SchematicSymbol(libraryNickname=LIB,entryName=typ,position=Position(X=x,Y=y,angle=0),
                      unit=1,inBom=True,onBoard=True,uuid=U())
    m = LP.get(ref) or ("v" if typ in ("R_V","C_V","CP_V","BAV99","JACK3","OPAPWR") else "h")
    rx,ry,vx,vy = {"v":(x+8,y-2,x+8,y+3), "h":(x-4,y-8,x-4,y+10), "ha":(x-4,y+8,x-4,y-8), "u":(x-4,y-4,x-4,y-9)}[m]
    # ⚠️ REFERENCIA `#...` NAO SE MOSTRA — convencao do KiCad para
    # simbolo que nao e peca (PWR_FLAG e afins). E aqui nao e so
    # convencao: medido no render de 01/09, o texto "#FLG1" caia por
    # cima do rotulo `V+PRE` e o "#FLG2" por cima do `GND`. O CORPO da
    # bandeira estava livre; era a etiqueta que colidia — e etiqueta e
    # o elemento que mais colide, como a lista fechada diz.
    _oculta = ref.startswith("#")
    s.properties=[Property(key="Reference",value=ref,position=Position(X=rx,Y=ry,angle=0),effects=eff(1.27,"left",_oculta)),
                  Property(key="Value",value=val,position=Position(X=vx,Y=vy,angle=0),effects=eff(1.0,"left")),
                  Property(key="Footprint",value="",position=Position(X=x,Y=y,angle=0),effects=eff(hide=True)),
                  Property(key="Datasheet",value="",position=Position(X=x,Y=y,angle=0),effects=eff(hide=True))]
    sch.schematicSymbols.append(s)
for pts in W:
    for a,b in zip(pts,pts[1:]):
        if a==b: continue
        sch.graphicalItems.append(Connection(type="wire",points=[Position(X=a[0],Y=a[1]),Position(X=b[0],Y=b[1])],stroke=Stroke(width=0),uuid=U()))
for x,y in JUNC: sch.junctions.append(Junction(position=Position(X=x,Y=y),uuid=U()))
for t,x,y,a in GLBL: sch.globalLabels.append(GlobalLabel(text=t,shape="bidirectional",position=Position(X=x,Y=y,angle=a),effects=eff(1.27),uuid=U()))
for t,x,y,sz in NOTES: sch.texts.append(SchText(text=t,position=Position(X=x,Y=y,angle=0),effects=eff(sz,"left"),uuid=U()))

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

sch.to_file(str(AQUI / "cotuca_preamp.kicad_sch"))
print("escrito:", AQUI / "cotuca_preamp.kicad_sch")
print("kicad_sch escrito")

# ================= PREVIA SVG =================
S=3.0
sx=lambda v: round((v-10)*S,2)
o=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{sx(480)}" height="{sx(400)}" font-family="monospace"><rect width="100%" height="100%" fill="#fff"/>']
for pts in W:
    for a,b in zip(pts,pts[1:]):
        o.append(f'<line x1="{sx(a[0])}" y1="{sx(a[1])}" x2="{sx(b[0])}" y2="{sx(b[1])}" stroke="#0a0" stroke-width="1.5"/>')
for ref,typ,val,x,y in PLACE:
    for u in SYM[typ].units:
        for g in (u.graphicItems or []):
            if isinstance(g,SyRect):
                x1,y1,x2,y2=sx(x+g.start.X),sx(y-g.start.Y),sx(x+g.end.X),sx(y-g.end.Y)
                o.append(f'<rect x="{min(x1,x2)}" y="{min(y1,y2)}" width="{abs(x2-x1)}" height="{abs(y2-y1)}" fill="#fff" stroke="#800" stroke-width="1.3"/>')
            elif isinstance(g,SyPolyLine):
                d=" ".join(f'{sx(x+p.X)},{sx(y-p.Y)}' for p in g.points)
                o.append(f'<polyline points="{d}" fill="none" stroke="#800" stroke-width="1.3"/>')
        for p in (u.pins or []):
            o.append(f'<circle cx="{sx(x+p.position.X)}" cy="{sx(y-p.position.Y)}" r="2" fill="#c00"/>')
    m = LP.get(ref) or ("v" if typ in ("R_V","C_V","CP_V","BAV99","JACK3","OPAPWR") else "h")
    rx,ry,vx,vy = {"v":(x+8,y-1.5,x+8,y+3.5), "h":(x-4,y-7,x-4,y+11), "ha":(x-4,y+9,x-4,y-6), "u":(x-4,y-3,x-4,y-8)}[m]
    o.append(f'<text x="{sx(rx)}" y="{sx(ry)}" font-size="10" font-weight="bold">{ref}</text>')
    o.append(f'<text x="{sx(vx)}" y="{sx(vy)}" font-size="8.5" fill="#444">{val}</text>')
for x,y in JUNC: o.append(f'<circle cx="{sx(x)}" cy="{sx(y)}" r="2.8" fill="#0a0"/>')
for t,x,y,a in GLBL:
    anc={0:"start",180:"end"}.get(a,"middle"); dy=11 if a==270 else (-5 if a==90 else 0)
    o.append(f'<text x="{sx(x)}" y="{sx(y)+dy}" font-size="10" fill="#00c" text-anchor="{anc}" font-weight="bold">{t}</text>')
for t,x,y,sz in NOTES: o.append(f'<text x="{sx(x)}" y="{sx(y)}" font-size="{max(9,sz*6.6)}" fill="#111">{t}</text>')
o.append("</svg>")
open(str(AQUI / "preamp_preview.svg"),"w").write("\n".join(o))
print("svg escrito")
