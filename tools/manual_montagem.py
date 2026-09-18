#!/usr/bin/env python3
"""manual_montagem — o manual de montagem do Cotucatronics, estilo IKEA, GERADO.

POR QUE E GERADO E NAO ESCRITO.

"Quando um artefacto ganha uma fonte, os desenhos anteriores dele ou passam a
ser gerados dessa fonte, ou morrem." Um manual escrito a mao diz a posicao de
uma peca uma vez e envelhece na primeira revisao da placa. Este le:

  · as placas FABRICADAS (kicad/cotuca_*/cotuca_*.kicad_pcb, pelo pcbnew) —
    contorno, corpo de cada peca, pad 1, rede de cada pad, lado, DNP;
  · o pacote que foi a fabrica (export/*_06db1c7/cpl_rico_*.csv) — valor e
    lado de cada peca montada;
  · as licoes de campo (caixa/LICOES-MONTAGEM.md #7, #8, #9) e o guia da caixa
    (caixa/CAIXA-ACRILICO-MONTAGEM.md), cujas regras estao aqui escritas como
    passos, com a fonte ao lado.

Os mapas sao desenhados do KiCad: cada passo destaca as pecas dele, marca o
pad 1 com um quadrado, e as pecas que vao POR BAIXO aparecem em vista
ESPELHADA — como se ve com a placa virada na mao.

USO:  (Python do KiCad)  tools/manual_montagem.py
      escreve docs/montagem/manual_montagem.html
"""
import csv
import datetime
import html
import io
import os
import subprocess
import sys

VERSAO = "0.1.4"
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PACOTE = "06db1c7"
SAIDA = os.path.join(RAIZ, "docs", "montagem", "manual_montagem.html")

try:
    import pcbnew
except ImportError:
    sys.exit("RECUSO: corre com o Python do KiCad:\n  /Applications/KiCad/KiCad.app/"
             "Contents/Frameworks/Python.framework/Versions/3.9/bin/python3.9")

E = html.escape
mm = lambda v: v / 1e6


# ----------------------------------------------------------------- dados
class Placa:
    def __init__(self, nome):
        self.nome = nome
        p = os.path.join(RAIZ, "kicad", "cotuca_%s" % nome, "cotuca_%s.kicad_pcb" % nome)
        self.b = pcbnew.LoadBoard(p)
        c = pcbnew.SHAPE_POLY_SET()
        self.b.GetBoardPolygonOutlines(c, False)
        o = c.COutline(0)
        self.contorno = [(mm(o.CPoint(k).x), mm(o.CPoint(k).y)) for k in range(o.PointCount())]
        xs = [x for x, _ in self.contorno]; ys = [y for _, y in self.contorno]
        self.x0, self.x1, self.y0, self.y1 = min(xs), max(xs), min(ys), max(ys)
        self.fp = {}
        for f in self.b.GetFootprints():
            ref = f.GetReference()
            cy = f.GetCourtyard(pcbnew.F_CrtYd)
            if cy.OutlineCount() == 0:
                cy = f.GetCourtyard(pcbnew.B_CrtYd)
            corpo = []
            if cy.OutlineCount():
                g = cy.COutline(0)
                corpo = [(mm(g.CPoint(k).x), mm(g.CPoint(k).y)) for k in range(g.PointCount())]
            pads = []
            for pd in f.Pads():
                q = pd.GetPosition(); s = pd.GetSize()
                pads.append(dict(n=pd.GetNumber(), x=mm(q.x), y=mm(q.y), w=mm(s.x), h=mm(s.y),
                                 quadrado=pd.GetShape() in (pcbnew.PAD_SHAPE_RECT, pcbnew.PAD_SHAPE_ROUNDRECT),
                                 rede=pd.GetNetname(),
                                 npth=pd.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH))
            # ⚠️ o pino 1 so se marca quando a FORMA o distingue dos outros. Os pads
            # SMD de uma resistencia sao todos rectangulos arredondados: marca-los
            # como "pino 1" ensinava a quem monta que uma resistencia tem lado.
            f1 = [x for x in pads if x["n"] == "1"]
            outros = [x for x in pads if x["n"] != "1" and not x["npth"]]
            p1_distinto = bool(f1 and outros and f1[0]["quadrado"]
                               and not all(o["quadrado"] for o in outros))
            self.fp[ref] = dict(ref=ref, valor=f.GetValue(), fpid=f.GetFPIDAsString(), p1=p1_distinto,
                                baixo=f.IsFlipped(), dnp=f.IsDNP(), corpo=corpo, pads=pads,
                                x=mm(f.GetPosition().x), y=mm(f.GetPosition().y))
        # o que foi montado, pelo pacote que foi a fabrica
        cpl = os.path.join(RAIZ, "export", "%s_2026-09-12_%s" % (nome, PACOTE), "cpl_rico_%s.csv" % nome)
        self.cpl = {r["Ref"]: r for r in csv.DictReader(io.open(cpl, encoding="utf-8"))}

    def pad1(self, ref):
        ps = [p for p in self.fp[ref]["pads"] if p["n"] == "1"]
        return ps[0] if ps else None

    def rede(self, ref, n):
        for p in self.fp[ref]["pads"]:
            if p["n"] == n:
                return p["rede"]
        return "?"


def commit():
    try:
        h = subprocess.run(["git", "-C", RAIZ, "rev-parse", "--short", "HEAD"],
                           capture_output=True, text=True).stdout.strip()
        sujo = subprocess.run(["git", "-C", RAIZ, "status", "--porcelain", "--untracked-files=no"],
                              capture_output=True, text=True).stdout.strip()
        return h + (" (árvore com alterações)" if sujo else "")
    except Exception:
        return "?"


# ----------------------------------------------------------------- desenho
def mapa(P, refs, baixo=False, rotulos=None, marcas=(), furos=(), legenda=None):
    """SVG da placa com `refs` destacadas. `baixo` espelha em x (vista por baixo)."""
    PX = 4.2
    pad = 6.0
    W = (P.x1 - P.x0 + 2 * pad) * PX
    H = (P.y1 - P.y0 + 2 * pad) * PX

    def tx(x):
        xx = (P.x0 + P.x1 - x) if baixo else x
        return (xx - P.x0 + pad) * PX

    def ty(y):
        return (y - P.y0 + pad) * PX

    out = ['<svg viewBox="0 0 %.0f %.0f" role="img" aria-label="%s" class="mapa">'
           % (W, H, E("mapa da placa %s, %s, com %s em destaque"
                      % (P.nome, "vista por baixo" if baixo else "vista por cima", ", ".join(refs) or "nada")))]
    pts = " ".join("%.1f,%.1f" % (tx(x), ty(y)) for x, y in P.contorno)
    out.append('<polygon points="%s" class="placa"/>' % pts)
    for ref, f in P.fp.items():
        if not f["corpo"] or ref in refs:
            continue
        # pecas do outro lado desenham-se a tracejado
        lado_outro = f["baixo"] != baixo
        pts = " ".join("%.1f,%.1f" % (tx(x), ty(y)) for x, y in f["corpo"])
        out.append('<polygon points="%s" class="corpo%s"/>' % (pts, " outro" if lado_outro else ""))
    for x, y, rot in furos:
        out.append('<circle cx="%.1f" cy="%.1f" r="%.1f" class="furo"/>' % (tx(x), ty(y), 1.6 * PX))
        out.append('<circle cx="%.1f" cy="%.1f" r="%.1f" class="furo-anilha"/>' % (tx(x), ty(y), 3.5 * PX))
        out.append('<text x="%.1f" y="%.1f" class="rot furo-rot">%s</text>' % (tx(x), ty(y) - 4.3 * PX, E(rot)))
    for ref in refs:
        f = P.fp.get(ref)
        if not f:
            continue
        if f["corpo"]:
            pts = " ".join("%.1f,%.1f" % (tx(x), ty(y)) for x, y in f["corpo"])
            out.append('<polygon points="%s" class="alvo"/>' % pts)
        for p in f["pads"]:
            if p["npth"]:
                continue
            r = max(0.5, min(p["w"], p["h"]) / 2) * PX
            if p["n"] == "1" and f["p1"]:
                out.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" class="pad1"/>'
                           % (tx(p["x"]) - r, ty(p["y"]) - r, 2 * r, 2 * r))
            elif p["quadrado"]:
                out.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" class="pad"/>'
                           % (tx(p["x"]) - r, ty(p["y"]) - r, 2 * r, 2 * r))
            else:
                out.append('<circle cx="%.1f" cy="%.1f" r="%.1f" class="pad"/>' % (tx(p["x"]), ty(p["y"]), r))
        if f["corpo"]:
            cx = sum(x for x, _ in f["corpo"]) / len(f["corpo"])
            cy = min(y for _, y in f["corpo"])
            lab = (rotulos or {}).get(ref, ref)
            out.append('<text x="%.1f" y="%.1f" class="rot">%s</text>' % (tx(cx), ty(cy) - 5, E(lab)))
    for ref, n, txt in marcas:
        for p in P.fp[ref]["pads"]:
            if p["n"] == n:
                out.append('<circle cx="%.1f" cy="%.1f" r="%.1f" class="marca"/>' % (tx(p["x"]), ty(p["y"]), 2.2 * PX))
                out.append('<text x="%.1f" y="%.1f" class="rot marca-rot">%s</text>' % (tx(p["x"]) + 3.2 * PX, ty(p["y"]) + 4, E(txt)))
    out.append("</svg>")
    cap = legenda or ("Vista **%s** · em azul: o que se coloca neste passo · ■ = pad 1" %
                      ("POR BAIXO (espelhada — como você vê com a placa virada)" if baixo else "por cima"))
    return ('<figure class="fig%s"><div class="sc">%s</div><figcaption>%s</figcaption></figure>'
            % (" figbaixo" if baixo else "", "".join(out), md(cap)))


def md(s):
    """negrito **x** e `codigo` — o minimo para o texto dos passos."""
    s = E(s)
    import re
    s = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", s)
    s = re.sub(r"`(.+?)`", r"<code>\1</code>", s)
    return s


# ----------------------------------------------------------------- blocos de pagina
PASSOS = []


def passo(capitulo, titulo, corpo, fig="", aviso=None, pare=None, qtd=None, fonte=None):
    PASSOS.append(dict(cap=capitulo, titulo=titulo, corpo=corpo, fig=fig,
                       aviso=aviso, pare=pare, qtd=qtd, fonte=fonte))


def lista(itens):
    return "<ul>" + "".join("<li>%s</li>" % md(i) for i in itens) + "</ul>"


def tabela(cab, linhas, cls=""):
    h = "".join("<th>%s</th>" % md(c) for c in cab)
    b = "".join("<tr>" + "".join("<td>%s</td>" % md(str(c)) for c in l) + "</tr>" for l in linhas)
    return '<div class="sc"><table class="%s"><thead><tr>%s</tr></thead><tbody>%s</tbody></table></div>' % (cls, h, b)


# ----------------------------------------------------------------- conteudo
def constroi():
    C = Placa("cerebro")
    Pn = Placa("painel")

    montadas = lambda P: [r for r in P.cpl]
    smd = lambda P, lado: [r for r, v in P.cpl.items()
                           if v["Side"] == lado and not any(k in v["Package"] for k in
                              ("_THT", "Radial", "PinHeader", "Terminal", "IDC", "JST", "Rotary",
                               "Potentiometer", "Daisy", "SUPERMINI", "C_Rect"))]

    # ------------------------------------------------------------ 0 antes de começar
    passo("0", "Ferramentas", lista([
        "**Ferro de solda** com ponta fina (cônica ou chanfro 1–2 mm), ~330 °C",
        "**Solda** 0,5–0,8 mm com fluxo, e **fluxo** extra (caneta ou pasta)",
        "**Pinça** de ponta fina · **lupa** ou lente de aumento · **malha dessoldadora**",
        "**Multímetro** com bipe de continuidade — vai ser usado em quase todos os passos",
        "Alicate de corte rente · alicate de bico · chave Phillips pequena",
        "Furadeira com brocas **ø3,2** e **ø7,5** (ou escareador) — para placa e tampa",
        "Para a caixa: **Weld-On 4** com frasco de agulha, **Weld-On 16**, luvas de **nitrilo**, fita crepe, esquadro",
        "Caneta permanente fina, para marcar fios e pinos",
    ]), qtd="1 bancada")

    passo("0", "Cinco regras que valem para tudo", lista([
        "**O quadrado é o pino 1.** Em toda peça com polaridade, o pad quadrado na placa marca o pino 1 (ou o **+**). Nos desenhos deste manual ele aparece como ■.",
        "**Nunca vire de cabeça para baixo um conector com chave** (IDC, JST dos LEDs). Virar troca pinos — alguns são alimentação. Onde o manual manda soldar algo por baixo, ele diz exatamente o quê.",
        "**Antes de ligar qualquer coisa na energia, bipe.** Os passos dizem quais pontos têm que bipar e quais **não** podem bipar.",
        "**A peça na sua mão manda.** Se ela não bate com o desenho, **pare e pergunte**. Não force, não lixe placa, não improvise.",
        "**Marque cada passo como feito** na caixinha ✓ quando terminar. A página lembra onde você parou.",
    ]), aviso="Solda com chumbo e solvente de acrílico são tóxicos: janela aberta, mãos lavadas, nada de comer na bancada.")

    # ------------------------------------------------------------ 1 inventário
    def inv_placa(P):
        grupos = {}
        for r, v in P.cpl.items():
            grupos.setdefault((v["Val"], v["Package"]), []).append(r)
        linhas = []
        for (val, pk), refs in sorted(grupos.items(), key=lambda k: (k[0][1], k[0][0])):
            f = P.fp[refs[0]]
            lado = "**por baixo**" if P.cpl[refs[0]]["Side"] == "bottom" else "por cima"
            linhas.append(["☐", "`%s`" % val, len(refs), " ".join(sorted(refs, key=lambda x: (x[0], int(''.join(c for c in x if c.isdigit()) or 0)))),
                           pk.replace("_", " ")[:34], lado])
        return linhas

    dnp = [r for r, f in C.fp.items() if f["dnp"] and f["corpo"]]
    passo("1", "Confira as peças do CÉREBRO (placa grande)",
          tabela(["", "valor", "qtd", "referências", "encapsulamento", "lado"], inv_placa(C)) +
          "<p>%s</p>" % md("**Não montar** (o lugar existe na placa, mas fica vazio): `%s`. Os pontos `TP` são só pads de teste, e os `H` são furos." % " ".join(sorted(dnp))),
          qtd="%d peças" % len(C.cpl),
          fonte="pacote de fábrica `cerebro_2026-09-12_06db1c7` · `cpl_rico_cerebro.csv`")

    passo("1", "Confira as peças do PAINEL (placa comprida)",
          tabela(["", "valor", "qtd", "referências", "encapsulamento", "lado no desenho"], inv_placa(Pn)),
          aviso="Os transistores `Q1`/`Q2` que vieram são **SI2302CDS** (marcados **N2**), não AO3400A como diz a lista. É a peça certa — mesma pinagem.",
          qtd="%d peças" % len(Pn.cpl),
          fonte="pacote de fábrica `painel_2026-09-12_06db1c7` · MANIFEST v5")

    passo("1", "Confira os módulos e as peças do painel frontal", tabela(
        ["", "peça", "qtd", "vai onde"], [
            ["☐", "**Daisy Seed** (placa de áudio)", 1, "soquete `U2` do cérebro"],
            ["☐", "**ESP32-C3 SuperMini** (ponte MIDI)", 1, "soquete `U3` do cérebro"],
            ["☐", "**Amplificador TPA3116** estéreo", 1, "fundo da caixa, espaçadores"],
            ["☐", "**Step-down 5 V** fixo", 1, "fios no `J3` do cérebro"],
            ["☐", "**Decoy PD 15 V** (gatilho USB-C)", 1, "parede de trás"],
            ["☐", "**OLED Waveshare 2,42\"** + o cabo dele (MX1.25 → jumpers fêmea)", 1, "tampa, por dentro"],
            ["☐", "**Encoder EC11 com rosca** e porca", 1, "`SW1` do painel + tampa"],
            ["☐", "**Potenciômetro 16 mm, 10 kΩ linear**, com porca", 2, "tampa (ligados por fios)"],
            ["☐", "**Botão arcade 30 mm AZUL** com LED 5 V", 2, "tampa"],
            ["☐", "**Botão arcade 30 mm BRANCO** (búzios)", 1, "tampa"],
            ["☐", "**Jack TRS 1/4\"** Neutrik NMJ6HCD2", 4, "2 na esquerda (piezos), 2 atrás (linha)"],
            ["☐", "**Chave DPDT ON-ON-ON**", 2, "parede de trás"],
            ["☐", "**Piezos** e **exciters** (transdutores 4 Ω)", "2 + 2", "na escultura"],
            ["☐", "**Cabo IDC 2×13 fêmea-fêmea, 300 mm**", 1, "painel ↔ cérebro"],
            ["☐", "**Barra de pinos 1×40** (Würth 61304011121)", 4, "cortar em todos os headers"],
            ["☐", "**Tira fêmea 1×20** (soquete da Daisy)", 2, "`U2`"],
            ["☐", "**Tira fêmea 1×8** (soquete do ESP32)", 2, "`U3`"],
            ["☐", "Jumpers fêmea **em L** (90°)", 7, "OLED"],
            ["☐", "Cabo USB-C curto com dados (PD)", 1, "powerbank → decoy"],
        ]), fonte="`docs/COTUCA_BOM.md` §5 e `caixa/LICOES-MONTAGEM.md` #6, #7, #9")

    passo("1", "Confira a caixa e a ferragem", tabela(
        ["", "peça", "qtd", "nota"], [
            ["☐", "Acrílico: **TAMPA, FRENTE, TRÁS, ESQUERDA, DIREITA, FUNDO**", 6, "3,0 mm transparente; não tire a película ainda"],
            ["☐", "Acrílico: **NERVURA**", 1, "vai colada embaixo da tampa"],
            ["☐", "Acrílico: **REFORÇO** 193 × 50", 1, "vai colado embaixo da tampa, na faixa dos arcade"],
            ["☐", "Acrílico: calços **CB** (com sextavado) e **CT** (lisos)", "6 + 6", "12 × 12"],
            ["☐", "Parafuso M3 × 10 cabeça cilíndrica + porca M3", "6 + 6", "fundo → calços"],
            ["☐", "Espaçador M3 × **10** F-F + parafuso M3 × 6", "4 + 8", "cérebro no fundo (**10, não 8**)"],
            ["☐", "Espaçador M3 de **6,5 mm** (ou 6 + arruela) + parafusos", "2 + 4", "painel na tampa"],
            ["☐", "Espaçador M2.5 + parafusos", "4 + 8", "OLED na tampa"],
            ["☐", "Espaçadores para o amplificador", 4, "altura conforme o módulo"],
            ["☐", "Pé de borracha ø15 adesivo", 4, "fundo"],
        ]), fonte="`caixa/CAIXA-ACRILICO-MONTAGEM.md` §1 · corte r2 (20 peças)")

    # ------------------------------------------------------------ 2 cérebro
    top_smd = smd(C, "top")
    ordem = [("resistores e capacitores 0805", lambda v: "0805" in v["Package"]),
             ("peças 1206 e 1210", lambda v: "1206" in v["Package"] or "1210" in v["Package"]),
             ("diodos duplos BAV99 (SOT-23)", lambda v: "SOT-23" in v["Package"]),
             ("diodos D1 e D2 e o fusível F1", lambda v: any(k in v["Package"] for k in ("D_SMA", "D_SMB", "Fuse"))),
             ("o amplificador operacional U1 (SOIC-8)", lambda v: "SOIC" in v["Package"])]
    for nome, sel in ordem:
        refs = [r for r in top_smd if sel(C.cpl[r])]
        if not refs:
            continue
        linhas = [["☐", r, "`%s`" % C.cpl[r]["Val"]] for r in sorted(refs, key=lambda x: (x[0], int(''.join(c for c in x if c.isdigit()) or 0)))]
        aviso = None
        pare = None
        marcas = []
        if "D1 e D2" in nome:
            marcas = [(r, "1", "K (faixa)") for r in refs if r.startswith("D")]
        if "U1" in nome:
            marcas = [("U1", "1", "pino 1")]
        if "SOT-23" in nome:
            aviso = "SOT-23 só encaixa de um jeito: **2 pernas de um lado, 1 do outro** — igual ao desenho dos pads."
        if "D1 e D2" in nome:
            pare = "Diodo tem lado. A **faixa** no corpo do diodo fica do lado do pad marcado **K** no desenho (o cátodo). Ao contrário, a placa não liga ou queima o diodo."
        if "U1" in nome:
            pare = "O **ponto/chanfro do pino 1** do chip vai no pad marcado **pino 1** no desenho. Girado, todos os pinos trocam."
        passo("2", "Cérebro · solde %s" % nome,
              tabela(["", "ref", "valor"], linhas) + lista([
                  "Estanhe **um** pad, segure a peça com a pinça, derreta esse pad — a peça fica presa.",
                  "Confira se está reta e centrada. Só então solde o(s) outro(s) pad(s).",
              ]), fig=mapa(C, refs, marcas=marcas), aviso=aviso, pare=pare, qtd="×%d" % len(refs),
              fonte="`kit/MAPA_COLOCACAO.md` do cérebro")
    passo("2", "Cérebro · vire a placa e solde o C16 (100n) POR BAIXO",
          lista(["O **C16** é a única peça SMD por baixo. Não tem polaridade."]),
          fig=mapa(C, ["C16"], baixo=True), qtd="×1",
          fonte="`MAPA_COLOCACAO.md`: C16 Bottom")

    # ⚠️ o J4 NAO: e a tomada de 01/09, quando a ponte ESP32 era um modulo a
    # parte ligado por cabo. Hoje ela encaixa no soquete U3, que ja tem o 5V e o
    # GND — o J4 so repete essas duas redes. Fica vazio.
    headers = [r for r in ("J3", "J5", "J6", "J11", "J12", "J13", "J15", "J16") if r in C.cpl]
    passo("2", "Cérebro · headers de pinos (corte da barra 1×40)",
          tabela(["", "ref", "pinos", "para"], [["☐", r, len(C.fp[r]["pads"]), "`%s`" % C.cpl[r]["Val"]] for r in headers]) +
          lista(["Corte a barra no número de pinos certo (conte duas vezes).",
                 "Encaixe o lado **curto** dos pinos na placa, por cima. Solde **um** pino, confira se ficou em pé (90°), depois os outros.",
                 "O **J4** (\"a ponte ESP32\") **fica vazio**: a ponte encaixa no soquete U3, que já tem a alimentação."]),
          fig=mapa(C, headers), qtd="×%d" % len(headers))
    passo("2", "Cérebro · bornes de parafuso J1 e J2",
          lista(["A abertura dos parafusos fica virada **para fora da placa**, onde os fios vão chegar.",
                 "`J1` = entrada do decoy PD (1 = **+**, 2 = GND) · `J2` = alimentação do amplificador (1 = **+15 V**, 2 = GND)."]),
          fig=mapa(C, ["J1", "J2"]), qtd="×2")
    passo("2", "Cérebro · capacitores de filme C14 e C24 (2u2)",
          lista(["Sem polaridade. Encoste o corpo na placa antes de soldar."]),
          fig=mapa(C, ["C14", "C24"]), qtd="×2")
    elec = [r for r in ("C5", "C3", "C6", "C1") if r in C.cpl]
    passo("2", "Cérebro · capacitores eletrolíticos",
          tabela(["", "ref", "valor"], [["☐", r, "`%s`" % C.cpl[r]["Val"]] for r in elec]) +
          lista(["A perna **longa** (+) vai no pad **■ quadrado**. A **faixa** no corpo do capacitor marca o **−**, do lado oposto ao quadrado.",
                 "Solde do menor para o maior: C5 → C3 → C6 → C1."]),
          fig=mapa(C, elec),
          pare="Eletrolítico ao contrário **estoura** quando liga. Confira os quatro antes de soldar.",
          qtd="×%d" % len(elec))
    passo("2", "Cérebro · caixa de pinos IDC J14 (para o cabo do painel)",
          lista(["A caixa de pinos tem um **entalhe** (chave) no meio de um dos lados compridos. Coloque-a com o entalhe do lado desenhado na serigrafia da placa.",
                 "O **pino 1** fica no pad marcado ■ no desenho."]),
          fig=mapa(C, ["J14"]), qtd="×1")
    passo("2", "Cérebro · soquetes da Daisy (U2) e do ESP32 (U3)",
          lista(["Use as **tiras fêmea**: 2 × 1×20 no `U2`, 2 × 1×8 no `U3`.",
                 "Truque para ficarem retos: encaixe as tiras **no módulo**, apoie o módulo sobre a placa, e solde as tiras com o módulo no lugar. Depois tire o módulo.",
                 "**Ainda não** deixe os módulos na placa — eles entram depois do teste de energia."]),
          fig=mapa(C, ["U2", "U3"]), qtd="2 + 2 tiras")

    # ------------------------------------------------------------ 3 painel
    p_smd = smd(Pn, "top")
    passo("3", "Painel · solde as peças SMD (por cima)",
          tabela(["", "ref", "valor"], [["☐", r, "`%s`" % Pn.cpl[r]["Val"]] for r in sorted(p_smd, key=lambda x: (x[0], int(''.join(c for c in x if c.isdigit()) or 0)))]) +
          lista(["Mesma técnica do cérebro: um pad, a peça, confere, os outros.",
                 "`Q1`/`Q2` são SOT-23: só encaixam de um jeito."]),
          fig=mapa(Pn, p_smd), qtd="×%d" % len(p_smd))

    passo("3", "Painel · o encoder SW1, por cima",
          lista(["Encaixe o encoder por cima: 3 pinos (A C B) de um lado, 2 pinos (botão) do outro, 2 patas grossas nas laterais.",
                 "Solde as 2 patas primeiro (seguram), depois os 5 pinos."]),
          fig=mapa(Pn, ["SW1"]),
          pare="Confira se o **seu** encoder tem os **2 pinos do botão** (do lado oposto ao A C B). Sem eles o botão de apertar não funciona — avise antes de soldar.",
          qtd="×1", fonte="`LICOES-MONTAGEM.md` #9")

    passo("3", "Painel · REGRA: daqui em diante, quase tudo vai POR BAIXO",
          "<p>%s</p>" % md("Na caixa, a placa do painel fica **só 6,5 mm abaixo do acrílico** — é a altura do corpo do encoder. Tudo o que for mais alto que isso **bate na tampa**. Por isso os conectores, o capacitor e os fios do painel vão **por baixo** da placa.") +
          lista(["A partir de agora os desenhos do painel estão em **vista por baixo** (espelhada): é assim que você vê com a placa virada na mão.",
                 "Por baixo a serigrafia **não aparece**. Use o **pad quadrado ■** como referência, sempre."]),
          fig=mapa(Pn, ["SW1"], baixo=True, legenda="Vista **por baixo**: o encoder (já soldado) aparece do outro lado · os tracejados são as peças que estão por cima"),
          fonte="`LICOES-MONTAGEM.md` #9")

    passo("3", "Painel · capacitor C3 (100 µF) por baixo",
          lista(["Enfie as pernas **por baixo**. A perna **longa (+)** vai no furo do pad **■**.",
                 "Solde **por cima** e corte as pernas."]),
          fig=mapa(Pn, ["C3"], baixo=True),
          pare="Perna longa no ■. Ao contrário estoura.", qtd="×1")

    passo("3", "Painel · conectores dos botões J3, J4, J5 (JST) por baixo",
          lista(["Encaixe cada JST **por baixo** e solde por cima.",
                 "São dos interruptores dos arcade — não têm polaridade, então virar não faz mal **nestes três**.",
                 "`J3` = azul A · `J4` = azul B · `J5` = branco (búzios)."]),
          fig=mapa(Pn, ["J3", "J4", "J5"], baixo=True), qtd="×3")

    passo("3", "Painel · LEDs dos arcade: FIOS soldados em J6 e J7 (sem JST)",
          tabela(["pad", "rede", "fio vai para"],
                 [["■ 1", "`%s`" % Pn.rede("J6", "1"), "**+** do LED do arcade (ânodo)"],
                  ["2", "`%s`" % Pn.rede("J6", "2"), "**−** do LED do arcade"]]) +
          lista(["Aqui **não** use JST: virado por baixo ele trocaria + com −.",
                 "Solde dois fios (vermelho no ■, preto no outro) direto nos furos, **por baixo**. Faça igual no `J7` (arcade B).",
                 "Marque na ponta de cada fio: **A+**, **A−**, **B+**, **B−**."]),
          fig=mapa(Pn, ["J6", "J7"], baixo=True, marcas=[("J6", "1", "+"), ("J7", "1", "+")]),
          qtd="4 fios", fonte="`LICOES-MONTAGEM.md` #9")

    j2 = [(p["n"], p["rede"]) for p in sorted(Pn.fp["J2"]["pads"], key=lambda p: int(p["n"]))]
    passo("3", "Painel · header do OLED (J2) por baixo",
          tabela(["pad", "sinal", "cor do fio no seu cabo (anote)"],
                 [["■ 1" if n == "1" else n, "`%s`" % r, "__________"] for n, r in j2]) +
          lista(["Corte 7 pinos da barra. Encaixe **por baixo** e solde por cima.",
                 "Os jumpers do OLED entram **por baixo**, com conectores **em L virados para trás e para baixo**."]),
          fig=mapa(Pn, ["J2"], baixo=True, marcas=[("J2", "1", "VCC"), ("J2", "2", "GND")]),
          pare="Por baixo a ordem aparece **ao contrário**. O pad **■ é o VCC**. Antes de ligar: bipe o ■ com o ponto de teste **TP1** (+5V) e o vizinho dele com o **TP3** (GND). Se não bipar, **pare**.",
          qtd="1 × 7 pinos", fonte="`LICOES-MONTAGEM.md` #7")

    j1 = {p["n"]: p["rede"] for p in Pn.fp["J1"]["pads"]}
    passo("3", "Painel · cabo do cérebro: fita soldada direto no J1 (sem conector)",
          lista(["Pegue o cabo IDC de 300 mm. **Corte fora a ficha de uma das pontas** (deixe a outra — ela vai no cérebro).",
                 "Na ponta cortada, separe os 26 fios uns 15 mm. O fio **vermelho é o 1**.",
                 "Solde **por baixo**, na ordem: **ímpares (1, 3 … 25)** na fila mais **longe** da borda comprida mais próxima; **pares (2, 4 … 26)** na fila mais **perto** dela.",
                 "O pad 1 do J1 é **redondo** (sem quadrado). Ele fica na ponta do J1 do **lado dos conectores J3/J6** — marcado ● no desenho."]),
          fig=mapa(Pn, ["J1"], baixo=True, marcas=[("J1", "1", "1 vermelho"), ("J1", "2", "2"), ("J1", "25", "25"), ("J1", "26", "26")]),
          pare="**NÃO** solde a caixa de pinos IDC por baixo, virada: dá curto do +3V3A com o GND. Só fios.",
          qtd="26 fios", fonte="`LICOES-MONTAGEM.md` #8")

    rv = {n: Pn.rede("RV1", n) for n in ("1", "2", "3")}
    passo("3", "Painel · fios dos potenciômetros (RV1, RV2)",
          tabela(["pad na placa", "rede", "perna do potenciômetro"],
                 [["■ 1" if Pn.fp["RV1"]["pads"] and any(p["n"] == "1" and p["quadrado"] for p in Pn.fp["RV1"]["pads"]) else "1", "`%s`" % rv["1"], "perna de uma ponta"],
                  ["2", "`%s`" % rv["2"], "**perna do meio** (cursor)"],
                  ["3", "`%s`" % rv["3"], "perna da outra ponta"]]) +
          lista(["Os potenciômetros **não vão na placa**: ficam presos na tampa, ligados por 3 fios cada.",
                 "Solde 3 fios de ~10 cm em cada (RV1 = esquerdo, RV2 = direito), **por baixo**. A perna do **meio** do potenciômetro vai sempre no pad **2**.",
                 "Se o volume girar ao contrário, inverte-se no firmware — não refaça a solda."]),
          fig=mapa(Pn, ["RV1", "RV2"], baixo=True), qtd="6 fios",
          fonte="`LICOES-MONTAGEM.md` #6 (caso B) e #9")

    passo("3", "Painel · dois furos de fixação (ø3,2)",
          lista(["O painel não veio com furos de fixação. Ele fica preso pelo encoder **e** por estes dois furos.",
                 "Marque com caneta e fure com broca **ø3,2**, devagar, pelo lado de cima (componentes). Tire a rebarba.",
                 "Cotas a partir do **canto superior esquerdo**, olhando por cima: **A = 77,5 → , 32,0 ↓** · **C = 146,0 → , 4,0 ↓** (mm)."]),
          fig=mapa(Pn, [], furos=[(Pn.x0 + 77.5, Pn.y0 + 32.0, "A"), (Pn.x0 + 146.0, Pn.y0 + 4.0, "C")],
                   legenda="Vista **por cima** · os círculos marcam os furos A e C com o espaço da arruela livre de trilhas"),
          aviso="Estes pontos foram medidos para não cortar nenhuma trilha. Não fure em outro lugar.",
          qtd="2 furos", fonte="certificação IFÁ10000-M · `docs/STATUS.md` 18/09")

    # ------------------------------------------------------------ 4 caixa
    passo("4", "Caixa · monte a seco, sem cola",
          lista(["**Não tire a película** das peças ainda.",
                 "Encaixe **frente + esquerda + direita + trás**. Os dentes entram à mão com leve atrito.",
                 "Apoie a **tampa** por cima (frente e trás entram inclinadas, 11,8°).",
                 "Meça: 200 × 120, altura **55 na frente** e **80 atrás**. As diagonais do fundo aberto têm que ser iguais.",
                 "Encaixe **todas as peças reais nos furos** (arcade, jacks, chaves, USB-C, encoder, potenciômetros). Um furo errado se corrige agora com lixa — depois, não."]),
          fonte="`CAIXA-ACRILICO-MONTAGEM.md` §2")
    passo("4", "Caixa · ajuste os furos da tampa",
          lista(["**Encoder**: o furo tem ø7 e a rosca é M7 — alargue para **~7,3 mm** até a rosca passar sem forçar.",
                 "**Potenciômetros**: ao lado de cada furo de ø7, fure **ø3,2** a **7,5 mm à esquerda** do centro (vista de cima, mesma altura) — é para o pino anti-giro do potenciômetro.",
                 "**Postes do painel**: fure **ø3,2** na tampa em A e C. Cotas na superfície da tampa, a partir da borda **esquerda** e da borda da **frente** (medindo ao longo da inclinação): **A = 91,5 → , 78 ↑** · **C = 160 → , 106 ↑**."]),
          aviso="Acrílico racha se a broca pegar: velocidade baixa, sem pressão, apoio de madeira embaixo.",
          fonte="`LICOES-MONTAGEM.md` #1 e #9 · geometria da `pcb_contorno`")
    passo("4", "Caixa · cole as quatro paredes",
          lista(["Monte de novo frente + esquerda + direita + trás, **sem a tampa**. Fita crepe em volta, esquadro nos cantos.",
                 "Com o frasco de agulha, passe **Weld-On 4** ao longo de cada junta **por dentro** — ele entra sozinho por capilaridade. Uma passada lenta por junta.",
                 "30 s sem mexer em cada junta. **1 hora** antes de tocar, 24 h antes de fazer força."]),
          pare="Luvas de nitrilo. Nunca super cola (racha o acrílico), nunca acetona.",
          fonte="`CAIXA-ACRILICO-MONTAGEM.md` §3")
    passo("4", "Caixa · calços com porca presa (×6)",
          lista(["Cada calço = **CB** (com sextavado, embaixo) + **CT** (liso, em cima).",
                 "Coloque o **fundo** encostado por dentro, rente à borda, como gabarito. Cole o **CB** com o sextavado **para baixo** sobre ele, nos 4 cantos e no meio da frente e de trás.",
                 "**Enfie a porca M3 no sextavado** e cole o **CT** por cima: a porca fica presa.",
                 "Tire o fundo antes de secar."]),
          fonte="`CAIXA-ACRILICO-MONTAGEM.md` §4")
    passo("4", "Caixa · reforço e nervura embaixo da tampa",
          lista(["**Reforço** (193 × 50): centrado, borda da frente a **10 mm** da borda dianteira. Confira **a seco** enfiando um botão arcade nos dois furos de ø30 — têm que ficar concêntricos. Depois Weld-On 4 pela aresta, peso por cima, 30 min.",
                 "**Nervura**: ao longo dos 200 mm, a **~62 mm da frente** (entre os arcade e os knobs). Weld-On 4, peso, 30 min."]),
          fonte="`CAIXA-ACRILICO-MONTAGEM.md` §4b e §5")
    passo("4", "Caixa · cole a tampa (por último)",
          lista(["Só depois de confirmar que tudo encaixa nos furos.",
                 "**Weld-On 16** (gel) nas ranhuras da **frente e de trás** (as inclinadas). **Weld-On 4** nas **laterais**.",
                 "Apoie, alinhe, peso distribuído. Limpe o excesso de gel com pano seco. **24 h**. Só então tire as películas."]),
          fonte="`CAIXA-ACRILICO-MONTAGEM.md` §5")

    # ------------------------------------------------------------ 5 interior
    passo("5", "Tampa por dentro · botões, potenciômetros, OLED",
          lista(["**Arcade** ×3 (azul A à esquerda, branco no meio, azul B à direita): porca por dentro.",
                 "**Potenciômetros** ×2 com porca, pino anti-giro no furo de ø3,2.",
                 "**OLED** por dentro, sobre **4 espaçadores M2.5** — o vidro olhando pela janela, o cabo para dentro."]),
          fonte="`LICOES-MONTAGEM.md` #2 e #6")
    passo("5", "Tampa por dentro · o painel",
          lista(["Passe o eixo do encoder pelo furo da tampa. **Arruela + porca** por fora, sem apertar ainda.",
                 "Espaçadores de **6,5 mm** em A e C, entre a placa e a tampa; parafusos M3 dos dois lados.",
                 "Aperte a porca do encoder por último. A placa tem que ficar **paralela** à tampa, sem forçar."]),
          aviso="Se algo raspar na tampa, **pare**: alguma peça ficou por cima e devia estar por baixo.",
          fonte="`LICOES-MONTAGEM.md` #9 · IFÁ10000-M M2")
    passo("5", "Paredes · jacks, chaves, USB-C",
          lista(["**Esquerda**: 2 jacks TRS dos **piezos** (furos ø9,5).",
                 "**Trás**: 2 jacks TRS de **linha** (ø9,5) e 2 chaves **DPDT** (ø6). Porca por dentro.",
                 "**Trás**: o **decoy PD** com o USB-C para fora (2 parafusos ou fita dupla forte)."]),
          fonte="`CAIXA-ACRILICO-MONTAGEM.md` §6 · furos do gerador da caixa")
    passo("5", "Fundo · cérebro e amplificador",
          lista(["Cérebro sobre **4 espaçadores M3 × 10** (não 8: a placa passa por cima de calços de 6 mm).",
                 "Amplificador sobre os espaçadores dele.",
                 "**Ainda sem a Daisy e o ESP32 nos soquetes.**"]),
          fonte="`CAIXA-ACRILICO-MONTAGEM.md` §1 e §6")

    # ------------------------------------------------------------ 6 fios
    def tab_j(P, ref, destinos):
        f = P.fp[ref]
        linhas = []
        for p in sorted(f["pads"], key=lambda p: (len(p["n"]), p["n"])):
            if p["npth"]:
                continue
            linhas.append(["■ " + p["n"] if (p["n"] == "1" and f["p1"]) else p["n"], "`%s`" % p["rede"], destinos.get(p["n"], "—")])
        return tabela(["pino", "rede", "liga em"], linhas)

    passo("6", "Fios · piezos → cérebro J5 e J6",
          tab_j(C, "J5", {"1": "**ponta (tip)** do jack · centro do cabo blindado", "2": "**manga (sleeve)** · malha do cabo", "3": "**anel (ring)** — vai à terra"}) +
          lista(["Igual para o `J6` (segundo piezo).", "Fio **< 60 mm**, blindado. A malha liga **só do lado da placa**."]),
          fig=mapa(C, ["J5", "J6"]), fonte="`HARDWARE.md` §2 · `CAIXA-ACRILICO-MONTAGEM.md` §6b")
    passo("6", "Fios · saídas de linha → cérebro J12 e J13",
          tab_j(C, "J12", {"1": "**ponta (tip)** do jack de linha", "2": "**manga (sleeve)**", "3": "**anel (ring)**"}) +
          "<p>%s</p>" % md("Igual para o `J13` (canal R)."),
          fig=mapa(C, ["J12", "J13"]), fonte="rede de cada pad, placa fabricada")
    passo("6", "Fios · chaves DPDT → cérebro J15 (L) e J16 (R)",
          tab_j(C, "J15", {"1": "**comum** do polo 1", "2": "polo 1, posição **jack**", "3": "polo 1, posição **amp**",
                           "4": "**comum** do polo 2", "5": "polo 2, posição **jack**", "6": "polo 2, posição **amp**",
                           "7": "não liga na chave (terra do cabo)", "8": "não liga na chave (terra do cabo)"}) +
          lista(["**Antes de soldar, meça a chave** com o multímetro: em cada posição, descubra qual pino do meio (comum) bipa com qual pino da ponta.",
                 "Posição 1: comuns ligados às pernas **jack**. Posição 3: às pernas **amp**. Posição do meio: um polo em cada — **as duas saídas**."]),
          fig=mapa(C, ["J15", "J16"]),
          pare="Chave ON-ON-ON tem tipos diferentes. **Meça antes de fiar** — não confie na posição das pernas.",
          fonte="`docs/COTUCA_BOM.md` §5 · `HARDWARE.md` §3")
    passo("6", "Fios · amplificador",
          tab_j(C, "J11", {"1": "**NÃO ligue numa entrada de áudio** (+15 V)", "2": "GND da entrada do amp",
                           "3": "entrada **L** do amp", "4": "entrada **R** do amp"}) +
          tab_j(C, "J2", {"1": "**+** da alimentação do amp", "2": "**−** da alimentação do amp"}),
          fig=mapa(C, ["J11", "J2"]),
          pare="As saídas do amplificador para os exciters têm os **dois terminais quentes**. Nunca ligue uma saída do amp ao GND nem a um jack — **queima o amp**.",
          fonte="`HARDWARE.md` §1 e §3")
    passo("6", "Fios · energia: decoy PD e step-down",
          tab_j(C, "J1", {"1": "**+** do decoy PD (15 V)", "2": "**−** do decoy PD"}) +
          tab_j(C, "J3", {"1": "**VIN** do step-down", "2": "**GND** do step-down", "3": "**VOUT (5 V)** do step-down"}),
          fig=mapa(C, ["J1", "J3"]),
          pare="Confira as marcas **VIN / GND / VOUT** no seu step-down. VIN no 5 V queima a Daisy.",
          fonte="`HARDWARE.md` §1")
    passo("6", "Fios · botões e LEDs dos arcade (painel)",
          lista(["Interruptor de cada arcade → o JST dele (J3 azul A, J4 azul B, J5 branco). Sem polaridade.",
                 "LED dos azuis: fio **A+/B+** no terminal **+** do LED do botão, **A−/B−** no **−**. O branco não tem LED."]),
          fonte="valor de cada conector na placa")
    passo("6", "Fios · o cabo do painel no cérebro",
          lista(["A ficha que sobrou no cabo IDC vai no **J14** do cérebro: a chave da ficha entra no entalhe da caixa de pinos — só entra de um jeito.",
                 "O cabo corre pela **parede de trás**, com **~80 mm de folga**: é o que deixa tirar o fundo sem desligar."]),
          fig=mapa(C, ["J14"]), fonte="`CAIXA-ACRILICO-MONTAGEM.md` §6b")

    # ------------------------------------------------------------ 7 energia
    passo("7", "Antes de ligar · bipes (sem energia)",
          tabela(["", "entre", "tem que"], [
              ["☐", "+15 V e GND (bornes J2)", "**NÃO bipar**"],
              ["☐", "+5 V e GND (TP1 e TP3 do painel)", "**NÃO bipar**"],
              ["☐", "+3V3A e GND (TP2 e TP3 do painel)", "**NÃO bipar**"],
              ["☐", "ficha IDC solta: pino 9 ↔ TP2 do painel", "bipar"],
              ["☐", "ficha IDC solta: pinos 11 e 12 ↔ TP1", "bipar"],
              ["☐", "ficha IDC solta: pino 1 ↔ TP3", "bipar"],
              ["☐", "ficha IDC solta: pino 2 ↔ pad 4 do J2 (OLED CLK)", "bipar"],
              ["☐", "pad ■ do J2 ↔ TP1", "bipar (é o VCC do OLED)"],
          ]), pare="Qualquer linha errada: **pare** e ache o fio antes de continuar.",
          fonte="`HARDWARE.md` checklist · `LICOES-MONTAGEM.md` #7 e #8")
    passo("7", "Primeira energia · passo a passo",
          lista(["Sem Daisy, sem ESP32, **exciters desligados**.",
                 "Ligue o powerbank no decoy PD. Meça no J1: **15 V**.",
                 "Meça a saída do step-down (J3 pino 3): **5 V**. Só com 5 V confirmados siga.",
                 "Desligue. Encaixe a **Daisy** no U2 e o **ESP32** no U3, na orientação do desenho da serigrafia (conferir o canto do pino 1 e o lado do USB).",
                 "Ligue de novo. Só **por último** conecte os exciters, com volume baixo."]),
          pare="Se algo esquentar, cheirar ou o LED do amp acender sem motivo: **desligue na hora**.",
          fonte="`HARDWARE.md` checklist antes de energizar")
    passo("7", "Antes de fechar · confira o que ainda está aberto",
          lista(["**Arcade da esquerda × J14 do cérebro**: com o fundo no lugar, aperte o botão até o fim e veja se o corpo dele encosta na caixa de pinos J14 ou no trimpot/Daisy. Se encostar, **pare** — avise para ajustar a caixa.",
                 "**Botão do encoder**: aperte e veja se o menu reage. Se não, o seu encoder pode não ter os pinos do botão.",
                 "Fundo: 6 parafusos M3 × 10 nas porcas presas. Pés de borracha a 12 mm dos cantos."]),
          fonte="`docs/STATUS.md` (18/09): M6 do cérebro por medir")
    return C, Pn


# ----------------------------------------------------------------- pagina
CAPS = {"0": "Antes de começar", "1": "Confira tudo", "2": "Cérebro", "3": "Painel",
        "4": "Caixa", "5": "Montar o interior", "6": "Fios", "7": "Ligar e fechar"}

CSS = r"""
:root{--papel:#ffffff;--tinta:#15171b;--suave:#5d6470;--linha:#d8dbe0;--fundo2:#f2f3f5;
--azul:#1452cc;--azul-claro:#dbe6ff;--perigo:#c8102e;--perigo-fundo:#fdecee;--atencao:#8a5a00;--atencao-fundo:#fff4d6;
--placa:#eef1ec;--corpo:#9aa1ab;}
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){--papel:#101216;--tinta:#e8eaee;--suave:#9aa2ae;--linha:#2c3139;--fundo2:#181b21;
--azul:#7aa2ff;--azul-claro:#1d2a4a;--perigo:#ff6b7d;--perigo-fundo:#2b1418;--atencao:#f0c060;--atencao-fundo:#2a220f;--placa:#1b211c;--corpo:#5d6573;}}
:root[data-theme="dark"]{--papel:#101216;--tinta:#e8eaee;--suave:#9aa2ae;--linha:#2c3139;--fundo2:#181b21;
--azul:#7aa2ff;--azul-claro:#1d2a4a;--perigo:#ff6b7d;--perigo-fundo:#2b1418;--atencao:#f0c060;--atencao-fundo:#2a220f;--placa:#1b211c;--corpo:#5d6573;}
body{background:var(--papel);color:var(--tinta);font:16px/1.55 "Barlow",system-ui,sans-serif;margin:0}
.wrap{max-width:960px;margin:0 auto;padding-inline:20px;padding-block:28px 80px}
h1,h2,.num{font-family:"Barlow Condensed","Barlow",system-ui,sans-serif;text-wrap:balance}
code,.mono,.rot{font-family:"IBM Plex Mono",ui-monospace,monospace}
header.capa{border-bottom:3px solid var(--tinta);padding-block:8px 20px;margin-bottom:12px}
header.capa h1{font-size:clamp(40px,7vw,72px);line-height:.95;margin:0;font-weight:700;letter-spacing:-.5px}
header.capa p{color:var(--suave);max-width:62ch;margin:.6em 0 0}
.carimbo{font-size:12.5px;color:var(--suave);margin-top:10px}
nav.caps{display:flex;flex-wrap:wrap;gap:6px;padding-block:14px;position:sticky;top:env(safe-area-inset-top,0px);background:var(--papel);z-index:5;border-bottom:1px solid var(--linha)}
nav.caps a{font:600 13px/1 "Barlow Condensed",sans-serif;letter-spacing:.06em;text-transform:uppercase;text-decoration:none;color:var(--tinta);border:1.5px solid var(--tinta);padding:6px 10px;border-radius:2px}
nav.caps a:focus-visible,nav.caps a:hover{background:var(--tinta);color:var(--papel)}
.progresso{font:600 13px/1 "Barlow Condensed",sans-serif;letter-spacing:.06em;text-transform:uppercase;color:var(--suave);margin-left:auto;align-self:center}
section.cap{padding-top:36px}
section.cap>h2{font-size:clamp(30px,5vw,44px);margin:0 0 6px;font-weight:700;display:flex;gap:14px;align-items:baseline}
section.cap>h2 .n{font-size:.62em;border:2px solid var(--tinta);padding:0 8px;line-height:1.25}
article.passo{display:grid;grid-template-columns:84px minmax(0,1fr);gap:4px 18px;padding-block:26px;border-top:1px solid var(--linha)}
article.passo .num{font-size:64px;line-height:.85;font-weight:700;color:var(--tinta)}
article.passo.feito .num{color:var(--suave);text-decoration:line-through;text-decoration-thickness:3px}
article.passo h3{margin:0 0 8px;font:700 22px/1.2 "Barlow Condensed",sans-serif;letter-spacing:.01em;text-wrap:balance}
.qtd{display:inline-block;font:700 15px/1 "Barlow Condensed",sans-serif;border:2px solid var(--tinta);padding:3px 7px;margin-left:8px;vertical-align:2px}
.conteudo{min-width:0}
.conteudo ul{margin:.4em 0 .6em;padding-left:1.2em}.conteudo li{margin:.25em 0;max-width:68ch}
.conteudo p{max-width:68ch;margin:.5em 0}
.sc{overflow-x:auto;max-width:100%}
table{border-collapse:collapse;font-size:14.5px;margin:.6em 0;min-width:100%;font-variant-numeric:tabular-nums}
th{font:600 12px/1.2 "Barlow Condensed",sans-serif;letter-spacing:.08em;text-transform:uppercase;text-align:left;color:var(--suave);border-bottom:2px solid var(--tinta);padding:6px 8px}
td{border-bottom:1px solid var(--linha);padding:6px 8px;vertical-align:top}
code{font-size:.9em;background:var(--fundo2);padding:1px 4px;border-radius:2px}
figure.fig{margin:14px 0 6px}
figure.fig .sc{background:var(--fundo2);border:1px solid var(--linha);padding:8px}
figure.figbaixo .sc{background:repeating-linear-gradient(135deg,var(--fundo2) 0 10px,var(--papel) 10px 20px)}
figcaption{font-size:13px;color:var(--suave);margin-top:6px}
svg.mapa{display:block;width:100%;height:auto;min-width:540px}
svg .placa{fill:var(--placa);stroke:var(--tinta);stroke-width:2}
svg .corpo{fill:none;stroke:var(--corpo);stroke-width:1}
svg .corpo.outro{stroke-dasharray:4 3;opacity:.7}
svg .alvo{fill:var(--azul-claro);stroke:var(--azul);stroke-width:2.5}
svg .pad{fill:var(--papel);stroke:var(--azul);stroke-width:1.5}
svg .pad1{fill:var(--azul);stroke:var(--azul)}
svg .rot{fill:var(--azul);font-size:15px;font-weight:600;text-anchor:middle}
svg .marca{fill:none;stroke:var(--perigo);stroke-width:2.5}
svg .marca-rot{fill:var(--perigo);text-anchor:start;font-size:14px}
svg .furo{fill:var(--tinta)}
svg .furo-anilha{fill:none;stroke:var(--perigo);stroke-width:2;stroke-dasharray:5 3}
svg .furo-rot{fill:var(--perigo);font-size:17px}
.aviso,.pare{display:flex;gap:12px;align-items:flex-start;padding:10px 12px;margin:12px 0 4px;max-width:72ch;border-radius:2px}
.aviso{background:var(--atencao-fundo);border-left:4px solid var(--atencao)}
.pare{background:var(--perigo-fundo);border-left:4px solid var(--perigo)}
.icone{flex:none;width:30px;height:30px}
.aviso .tri{fill:var(--atencao)}.pare .oct{fill:var(--perigo)}
.icone text{fill:var(--papel);font:700 18px/1 "Barlow Condensed",sans-serif;text-anchor:middle}
.fonte{font-size:12px;color:var(--suave);margin-top:8px}
label.ok{display:inline-flex;gap:8px;align-items:center;margin-top:12px;font:600 14px/1 "Barlow Condensed",sans-serif;letter-spacing:.06em;text-transform:uppercase;cursor:pointer;border:1.5px solid var(--tinta);padding:7px 11px}
label.ok input{width:18px;height:18px;accent-color:var(--azul)}
label.ok:focus-within{outline:2px solid var(--azul);outline-offset:2px}
@media (max-width:620px){article.passo{grid-template-columns:1fr}article.passo .num{font-size:48px}}
@media (prefers-reduced-motion:no-preference){label.ok{transition:background .15s}}
@page{size:A4;margin:12mm 11mm}
@media print{
:root,:root[data-theme="dark"],:root:not([data-theme="light"]){--papel:#fff;--tinta:#111;--suave:#555;--linha:#ccc;--fundo2:#f3f4f6;
--azul:#1452cc;--azul-claro:#dbe6ff;--perigo:#c8102e;--perigo-fundo:#fdecee;--atencao:#8a5a00;--atencao-fundo:#fff4d6;--placa:#eef1ec;--corpo:#8a919b}
body{font-size:11.5px;-webkit-print-color-adjust:exact;print-color-adjust:exact}
.wrap{max-width:none;padding:0}
nav.caps,label.ok,.progresso{display:none}
section.cap{padding-top:0;break-before:page}
section.cap:first-of-type{break-before:auto}
article.passo{grid-template-columns:52px minmax(0,1fr);padding-block:12px;break-inside:auto}
figure.fig{max-width:100%}.sc{overflow:hidden}
article.passo .num{font-size:40px}
article.passo h3{font-size:17px;break-after:avoid}
figure.fig,.aviso,.pare,table{break-inside:avoid}
.sc{overflow:visible}
svg.mapa{min-width:0}
a{color:inherit;text-decoration:none}
}
"""

JS = r"""
(function(){
  var K='cotuca-manual-06db1c7';
  var st={};
  try{st=JSON.parse(localStorage.getItem(K)||'{}')||{};}catch(e){st={};}
  var cx=document.querySelectorAll('input.feito');
  function conta(){var n=0;cx.forEach(function(c){if(c.checked)n++;});
    var p=document.getElementById('progresso'); if(p)p.textContent=n+' / '+cx.length+' passos';}
  cx.forEach(function(c){
    if(st[c.id]){c.checked=true;c.closest('article').classList.add('feito');}
    c.addEventListener('change',function(){
      c.closest('article').classList.toggle('feito',c.checked);
      st[c.id]=c.checked; try{localStorage.setItem(K,JSON.stringify(st));}catch(e){}
      conta();});
  });
  conta();
})();
"""

ICONE_AVISO = ('<svg class="icone" viewBox="0 0 30 30" aria-hidden="true"><polygon class="tri" points="15,2 29,27 1,27"/>'
               '<text x="15" y="24">!</text></svg>')
ICONE_PARE = ('<svg class="icone" viewBox="0 0 30 30" aria-hidden="true"><polygon class="oct" '
              'points="9,1 21,1 29,9 29,21 21,29 9,29 1,21 1,9"/><text x="15" y="21">✋</text></svg>')


def pagina():
    constroi()
    hoje = datetime.date.today().strftime("%d/%m/%Y")
    out = ['<title>Montagem do Cotucatronics</title>',
           '<link rel="preconnect" href="https://fonts.googleapis.com">'
           '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>'
           '<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Barlow:wght@400;600;700&family=Barlow+Condensed:wght@600;700&family=IBM+Plex+Mono:wght@400;600&display=swap">',
           "<style>%s</style>" % CSS, '<div class="wrap">',
           '<header class="capa"><h1>Cotucatronics<br>Manual de montagem</h1>'
           '<p>Das placas nuas até o instrumento ligado: cérebro, painel, caixa de acrílico e fios. '
           'Siga os passos na ordem. Onde houver ✋, pare e confira antes de seguir.</p>'
           '<div class="carimbo mono">gerado por tools/manual_montagem.py v%s · %s · commit %s · placas do pacote %s (rev C)</div></header>'
           % (VERSAO, hoje, E(commit()), PACOTE)]
    out.append('<nav class="caps" aria-label="capítulos">' +
               "".join('<a href="#cap%s">%s %s</a>' % (k, k, E(v)) for k, v in CAPS.items()) +
               '<span class="progresso" id="progresso"></span></nav>')
    atual = None
    n = 0
    for p in PASSOS:
        if p["cap"] != atual:
            if atual is not None:
                out.append("</section>")
            atual = p["cap"]
            out.append('<section class="cap" id="cap%s"><h2><span class="n">%s</span>%s</h2>' % (atual, atual, E(CAPS[atual])))
        n += 1
        pid = "p%02d" % n
        corpo = p["corpo"]
        extra = ""
        if p["aviso"]:
            extra += '<div class="aviso">%s<div>%s</div></div>' % (ICONE_AVISO, md(p["aviso"]))
        if p["pare"]:
            extra += '<div class="pare">%s<div><strong>PARE.</strong> %s</div></div>' % (ICONE_PARE, md(p["pare"]))
        out.append('<article class="passo" id="%s"><div class="num">%d</div><div class="conteudo">'
                   '<h3>%s%s</h3>%s%s%s%s<label class="ok"><input type="checkbox" class="feito" id="ok-%s"> feito</label></div></article>'
                   % (pid, n, E(p["titulo"]), ('<span class="qtd">%s</span>' % E(p["qtd"])) if p["qtd"] else "",
                      corpo, p["fig"], extra,
                      ('<div class="fonte">fonte: %s</div>' % md(p["fonte"])) if p["fonte"] else "", pid))
    out.append("</section></div><script>%s</script>" % JS)
    return "\n".join(out)


def main():
    print("manual_montagem v%s" % VERSAO)
    h = pagina()
    os.makedirs(os.path.dirname(SAIDA), exist_ok=True)
    io.open(SAIDA, "w", encoding="utf-8").write(h)
    print("  %d passos · %d KB -> %s" % (len(PASSOS), len(h) // 1024, os.path.relpath(SAIDA, RAIZ)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
