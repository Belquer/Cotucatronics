#!/usr/bin/env python3
"""gen_caixa_paginas v0.2.0 — as paginas 2 a 6 do desenho de fabricacao.

Cada pagina e um SVG. Cotas em mm. Corre-se daqui:

    python3 docs/desenhos/caixa/gen_caixa_paginas.py

⚠️ O GERADOR E A FONTE. As cotas marcadas "confirmar" mudam depois de medir
as pecas — e mudam AQUI, nunca no SVG. Um SVG editado a mao diverge do `.py`
na primeira regeneracao, e ninguem sabe qual manda.

⚠️ CAMINHOS RELATIVOS AO PROPRIO FICHEIRO. A v0.1.0 tinha
`/mnt/user-data/outputs/` e `/home/claude/svg_colisoes.py` gravados: rebentava
com FileNotFoundError na primeira tentativa noutro computador — que e a §8 da
skill `kicad`, e foi exactamente o que aconteceu ao chegar a este repo.
"""
import subprocess, sys, pathlib

VERSAO = "0.2.0"
AQUI = pathlib.Path(__file__).resolve().parent
RAIZ = AQUI.parents[2]
PORTAO = RAIZ / "tools" / "svg_colisoes.py"
S = 2.6
def mm(v): return round(v * S, 2)

STYLE = '''<style>
 .box{fill:#fafafa;stroke:#111;stroke-width:1.8}
 .h{fill:#fdeceb;stroke:#b8322c;stroke-width:1.4}
 .hc{fill:none;stroke:#b8322c;stroke-width:1}
 .z{fill:#e8f0e8;stroke:#4a7a4a;stroke-width:1;stroke-dasharray:4 3}
 .d{stroke:#3a6ea5;stroke-width:0.8;fill:none}
 .dt{font-size:9.5px;fill:#3a6ea5}
 .t{font-size:10px;fill:#111}
 .ts{font-size:8.5px;fill:#555}
 .tz{font-size:9px;fill:#2c5a2c}
 .hd{font-size:12.5px;fill:#111;font-weight:bold}
 .ti{font-size:17px;fill:#111;font-weight:bold}
 .warn{font-size:9px;fill:#8a4a00}
 .tab{fill:#fff;stroke:#111;stroke-width:1.2}
</style>'''

class Page:
    def __init__(self, w, h, title, sub):
        self.o = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}" font-family="Helvetica, Arial, sans-serif">',
                  f'<rect width="{w}" height="{h}" fill="#fff"/>', STYLE]
        self.text(28, 30, title, "ti"); self.text(28, 48, sub, "ts")
    def text(self, x, y, s, cls="t", anchor="start"):
        s = s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        self.o.append(f'<text class="{cls}" x="{x}" y="{y}" text-anchor="{anchor}">{s}</text>')
    def hdim(self, x0, x1, y, label, above=True):
        self.o.append(f'<path class="d" d="M{x0} {y} L{x1} {y} M{x0} {y-4} L{x0} {y+4} M{x1} {y-4} L{x1} {y+4}"/>')
        self.text((x0+x1)/2, y-7 if above else y+14, label, "dt", "middle")
    def vdim(self, x, y0, y1, label, left=False):
        self.o.append(f'<path class="d" d="M{x} {y0} L{x} {y1} M{x-4} {y0} L{x+4} {y0} M{x-4} {y1} L{x+4} {y1}"/>')
        if left: self.text(x-6, (y0+y1)/2+3, label, "dt", "end")
        else: self.text(x+6, (y0+y1)/2+3, label, "dt")
    def face(self, ox, oy, w, h, title):
        self.o.append(f'<rect class="box" x="{ox}" y="{oy-mm(h)}" width="{mm(w)}" height="{mm(h)}"/>')
        self.text(ox, oy-mm(h)-8, title, "hd")
        self.hdim(ox, ox+mm(w), oy+16, f"{w}", above=False)
        self.vdim(ox+mm(w)+12, oy-mm(h), oy, f"{h}")
    def hole(self, ox, oy, x, y, d, label, note="", lab_dy=15):
        cx, cy = ox+mm(x), oy-mm(y); r = mm(d/2)
        self.o.append(f'<circle class="h" cx="{cx}" cy="{cy}" r="{r}"/>')
        self.o.append(f'<path class="hc" d="M{cx-r-4} {cy} L{cx+r+4} {cy} M{cx} {cy-r-4} L{cx} {cy+r+4}"/>')
        self.text(cx, cy+r+lab_dy, f"{label} ø{d}", "t", "middle")
        self.text(cx, cy+r+lab_dy+10, f"({x}, {y})", "ts", "middle")
        if note: self.text(cx, cy+r+lab_dy+19, note, "warn", "middle")
    def rect(self, ox, oy, x, y, w, h, label, note="", cls="h", lab_dy=15):
        cx, cy = ox+mm(x), oy-mm(y)
        self.o.append(f'<rect class="{cls}" x="{cx-mm(w/2)}" y="{cy-mm(h/2)}" width="{mm(w)}" height="{mm(h)}"/>')
        self.text(cx, cy+mm(h/2)+lab_dy, f"{label} {w}×{h}", "t", "middle")
        self.text(cx, cy+mm(h/2)+lab_dy+10, f"centro ({x}, {y})", "ts", "middle")
        if note: self.text(cx, cy+mm(h/2)+lab_dy+19, note, "warn", "middle")
    def zone(self, ox, oy, x0, y0, x1, y1, label, alt=""):
        self.o.append(f'<rect class="z" x="{ox+mm(x0)}" y="{oy-mm(y1)}" width="{mm(x1-x0)}" height="{mm(y1-y0)}"/>')
        self.text(ox+mm((x0+x1)/2), oy-mm(y1)+12, label, "tz", "middle")
        if alt: self.text(ox+mm((x0+x1)/2), oy-mm(y1)+22, alt, "ts", "middle")
    def save(self, fn):
        self.o.append('</svg>'); open(fn, 'w').write("\n".join(self.o))
        r = subprocess.run([sys.executable, str(PORTAO), fn], capture_output=True, text=True)
        print(r.stdout.strip())
        return r.returncode == 0

OUT = str(AQUI) + "/"

# ======================= PAGINA 2: TAMPA =======================
p = Page(1000, 640, "Página 2 — TAMPA (acrílico cast 8 mm) · 200 × 123,7 no plano",
         "Datum = canto inferior-esquerdo da tampa vista de cima (frente em baixo). y ao longo da inclinação. Tampa removível: assenta nas paredes e aparafusa a blocos internos.")
ox, oy = 60, 420
p.face(ox, oy, 200, 123.7, "vista de cima")
p.hole(ox, oy, 50, 35, 30, "freeze A"); p.hole(ox, oy, 150, 35, 30, "freeze B")
p.rect(ox, oy, 45, 92, 55, 27.5, "janela OLED", "só se a tampa for opaca; em fumado dispensa-se")
for (x, y) in [(17.25, 75.25), (72.75, 75.25), (17.25, 108.75), (72.75, 108.75)]:
    cx, cy = ox+mm(x), oy-mm(y)
    p.o.append(f'<circle class="hc" cx="{cx}" cy="{cy}" r="{mm(1.25)}"/>')
p.text(620, 300, "OLED: 4× ø2,5 (M2.5) nos cantos do módulo,", "warn")
p.text(620, 314, "55,5 × 33,5 entre centros — CONFIRMAR no módulo.", "warn")
p.hole(ox, oy, 105, 92, 7, "encoder", "ø7 ou 9 — confirmar")
p.hole(ox, oy, 140, 92, 7, "pot L", "confirmar")
p.hole(ox, oy, 175, 92, 7, "pot R", "confirmar")
for (x, y) in [(10, 8), (100, 8), (190, 8), (10, 115.7), (100, 115.7), (190, 115.7)]:
    cx, cy = ox+mm(x), oy-mm(y)
    p.o.append(f'<circle class="h" cx="{cx}" cy="{cy}" r="{mm(1.7)}"/>')
p.text(ox+mm(100), oy-mm(1.5)-2, "6× ø3,4 escareado — parafusos M3 da tampa, em (10 | 100 | 190, 8) e (10 | 100 | 190, 115,7)", "ts", "middle")
p.hdim(ox+mm(10), ox+mm(100), oy+40, "90"); p.hdim(ox+mm(100), ox+mm(190), oy+40, "90")
p.vdim(ox-22, oy-mm(115.7), oy-mm(8), "107,7", left=True)
p.hdim(ox+mm(50), ox+mm(150), oy-mm(35)-mm(15)-8, "100 entre eixos")
p.hdim(ox+mm(105), ox+mm(140), oy-mm(92)-mm(6), "35"); p.hdim(ox+mm(140), ox+mm(175), oy-mm(92)-mm(6), "35")
p.text(620, 120, "Notas", "hd")
p.text(620, 140, "· Furos de knobs a 92 mm da frente, no plano da tampa.", "ts")
p.text(620, 154, "· Freeze a 35 mm da frente: o pé chega sem tocar nos knobs (26 mm livres).", "ts")
p.text(620, 168, "· Escareado dos M3: 90°, ø6,5 na face — a cabeça fica rente.", "ts")
p.text(620, 182, "· Bordas da tampa SEM dentes: assenta a topo nas paredes.", "ts")
p.text(620, 196, "· Furos em acrílico: broca própria ou laser; nunca broca de metal a alta rotação.", "ts")
p.text(620, 216, "Folgas medidas", "hd")
p.text(620, 236, "encoder ø18 → pot L ø13: 19,5 mm · pot L → pot R: 22 · pot R → borda: 18,5", "ts")
p.text(620, 250, "encoder → janela OLED: 23,5 · knobs → freeze: 33", "ts")
ok2 = p.save(OUT + "caixa-fabricacao-p2-tampa.svg")

# ======================= PAGINA 3: FUNDO =======================
p = Page(1000, 700, "Página 3 — FUNDO (acrílico cast 6 mm) · 200 × 120",
         "Datum = canto esquerdo-frente visto de CIMA (de dentro da caixa). Paredes assentam por cima; dentes das paredes entram em ranhuras no fundo.")
ox, oy = 60, 460
p.face(ox, oy, 200, 120, "vista de cima, de dentro")
p.o.append(f'<rect class="hc" x="{ox+mm(6)}" y="{oy-mm(114)}" width="{mm(188)}" height="{mm(108)}" stroke-dasharray="6 3"/>')
p.text(ox+mm(100), oy-mm(117), "interior 188 × 108 (linha das paredes)", "ts", "middle")
p.zone(ox, oy, 22, 10, 142, 110, "CARRIER 120 × 100", "4 espaçadores M3 × 8 mm")
for (x, y) in [(25.5, 13.5), (138.5, 13.5), (25.5, 106.5), (138.5, 106.5)]:
    cx, cy = ox+mm(x), oy-mm(y)
    p.o.append(f'<circle class="h" cx="{cx}" cy="{cy}" r="{mm(1.6)}"/>')
p.text(ox+mm(82), oy-mm(58), "4× ø3,2 — padrão do carrier, CONFIRMAR", "warn", "middle")
p.zone(ox, oy, 148, 44, 190, 96, "AMP", "")
for i in range(5):
    y = 50 + i*8
    p.o.append(f'<rect class="h" x="{ox+mm(151)}" y="{oy-mm(y+1.5)}" width="{mm(36)}" height="{mm(3)}"/>')
p.text(620, 300, "Ventilação sob o amp: 5 rasgos 36 × 3, passo 8, de y=50 a 82.", "ts")
for (x, y) in [(12, 12), (188, 12), (12, 108), (188, 108)]:
    cx, cy = ox+mm(x), oy-mm(y)
    p.o.append(f'<circle class="hc" cx="{cx}" cy="{cy}" r="{mm(7.5)}" stroke-dasharray="3 2"/>')
p.text(620, 314, "Pés: 4× ø15 adesivos a 12 mm dos cantos (tracejado). Sem furos.", "ts")
p.zone(ox, oy, 150, 100, 176, 114, "DECOY", "")
p.hdim(ox+mm(25.5), ox+mm(138.5), oy+40, "113"); p.vdim(ox-22, oy-mm(106.5), oy-mm(13.5), "93", left=True)
p.text(620, 120, "Ranhuras dos dentes das paredes", "hd")
p.text(620, 140, "Frente e trás: 6 ranhuras 10 × 6 cada, passo 30, a 3 mm da borda.", "ts")
p.text(620, 154, "Esquerda e direita: 4 ranhuras 10 × 6, passo 30, a 3 mm da borda.", "ts")
p.text(620, 168, "Geometria exata dos dentes: página 5 (gerada do mesmo ficheiro, kerf 0,2).", "ts")
p.text(620, 188, "Alturas por zona", "hd")
p.text(620, 208, "sob os freeze (frente): ≤ 8 mm livres depois de montado", "ts")
p.text(620, 222, "carrier + espaçador 8 + Daisy no soquete 20 = 30 → fica na metade de trás", "ts")
p.text(620, 236, "amp com dissipador ~25 → zona alta, direita", "ts")
p.text(620, 250, "step-down e ESP32: encaixam no carrier por header — sem furos no fundo", "ts")
ok3 = p.save(OUT + "caixa-fabricacao-p3-fundo.svg")

# ======================= PAGINA 4: INTERIOR E CABOS =======================
p = Page(1000, 640, "Página 4 — INTERIOR: blocos da tampa, percursos de cabo, alturas",
         "Vista de cima com a tampa retirada. Blocos de acrílico 12 × 12 × 25 colados nos cantos e a meio das paredes longas, com inserto M3 — recebem os 6 parafusos da tampa.")
ox, oy = 60, 440
p.face(ox, oy, 200, 120, "planta interior")
p.o.append(f'<rect class="hc" x="{ox+mm(6)}" y="{oy-mm(114)}" width="{mm(188)}" height="{mm(108)}" stroke-dasharray="6 3"/>')
for (bx, by) in [(6, 6), (182, 6), (6, 102), (182, 102), (94, 6), (94, 102)]:
    p.o.append(f'<rect class="tab" x="{ox+mm(bx)}" y="{oy-mm(by+12)}" width="{mm(12)}" height="{mm(12)}"/>')
p.text(660, 338, "6 blocos 12×12×25 (quadrados brancos), inserto M3,", "ts")
p.text(660, 352, "alinhados com os furos da tampa em x = 10 | 100 | 190.", "ts")
p.zone(ox, oy, 8, 20, 60, 100, "PRÉ-AMP", "ilha AGND · junto aos jacks")
p.zone(ox, oy, 70, 20, 142, 100, "DAISY + PONTE", "zona alta")
p.zone(ox, oy, 148, 44, 190, 96, "AMP", "dissipador para cima")
# percursos de cabo
p.o.append(f'<path class="d" d="M{ox-24} {oy-mm(35)} L{ox+mm(8)} {oy-mm(35)}" stroke-width="2"/>')
p.o.append(f'<path class="d" d="M{ox-24} {oy-mm(85)} L{ox+mm(8)} {oy-mm(85)}" stroke-width="2"/>')
p.text(ox-30, oy-mm(60), "piezos", "dt", "end")
p.o.append(f'<path class="d" d="M{ox+mm(190)} {oy-mm(80)} L{ox+mm(200)+6} {oy-mm(80)}" stroke-width="2"/>')
p.text(ox+mm(200)+14, oy-mm(80)-6, "exciters", "dt")
p.text(660, 372, "Chicote do painel: sai do carrier pela borda de TRÁS", "ts")
p.text(660, 386, "e sobe colado à parede até à tampa.", "ts")
p.text(660, 120, "Regras de percurso", "hd")
p.text(660, 140, "· Cabo dos piezos entra pela esquerda e vai direto ao pré-amp: < 60 mm.", "ts")
p.text(660, 154, "· Cabo dos exciters sai do amp pela direita. Nunca cruza o pré-amp.", "ts")
p.text(660, 168, "· IDC do painel (26 vias) corre pela parede de trás, longe da esquerda.", "ts")
p.text(660, 182, "· Linha para a mesa: do DPDT (parede de trás) ao amp e aos jacks de trás.", "ts")
p.text(660, 196, "· Malha do cabo do piezo: só do lado da placa.", "ts")
p.text(660, 216, "Alturas úteis (interior)", "hd")
p.text(660, 236, "frente (y=0): 40 − 6 fundo − 8 tampa = 26 mm", "ts")
p.text(660, 250, "trás (y=120): 70 − 14 = 56 mm", "ts")
p.text(660, 264, "a meio (y=60): 41 mm", "ts")
p.text(660, 284, "Blocos da tampa", "hd")
p.text(660, 304, "Colados com cimento de acrílico ANTES de furar o inserto.", "ts")
p.text(660, 318, "Inserto M3 a quente ou roscado; nunca forçar — racha.", "ts")
ok4 = p.save(OUT + "caixa-fabricacao-p4-interior.svg")

# ======================= PAGINA 5: JUNTAS, CORTE, FERRAGENS =======================
p = Page(1000, 720, "Página 5 — JUNTAS, LISTA DE CORTE E FERRAGENS",
         "Dentes de 10 mm, profundidade = espessura (6). Kerf 0,2 mm: dentes +0,1 por lado. Só a base tem dentes; a tampa assenta a topo.")
p.text(60, 90, "Lista de corte (acrílico cast)", "hd")
rows = [("fundo", "200 × 120", "6", "dentes nas 4 bordas (para as paredes)"),
        ("frente", "200 × 40", "6", "dentes em baixo e nos 2 lados"),
        ("trás", "200 × 70", "6", "dentes em baixo e nos 2 lados"),
        ("esquerda", "120 × 40→70 (trapézio)", "6", "ranhuras em baixo e nos 2 lados; topo liso inclinado"),
        ("direita", "120 × 70→40 (espelho)", "6", "idem"),
        ("tampa", "200 × 123,7", "8", "sem dentes; 6 furos M3 escareados"),
        ("blocos", "12 × 12 × 25 (×6)", "12", "de barra; inserto M3")]
y = 110
p.text(60, y, "peça", "t"); p.text(180, y, "dimensão externa", "t"); p.text(400, y, "esp.", "t"); p.text(450, y, "bordas", "t")
for r in rows:
    y += 16; p.text(60, y, r[0], "ts"); p.text(180, y, r[1], "ts"); p.text(400, y, r[2], "ts"); p.text(450, y, r[3], "ts")

p.text(60, 252, "Padrão de dentes — exemplo da borda de baixo da FRENTE (200 mm)", "hd")
ox, oy = 60, 340
p.o.append(f'<rect class="box" x="{ox}" y="{oy-mm(20)}" width="{mm(200)}" height="{mm(20)}"/>')
for i in range(6):
    x0 = 12 + i*30 + 10   # dente de 10 entre ranhuras de 20? -> dentes de 10, espacos de 20, comeca a 12
    p.o.append(f'<rect class="h" x="{ox+mm(x0)}" y="{oy}" width="{mm(10)}" height="{mm(6)}"/>')
p.text(ox+mm(100), oy+mm(6)+14, "6 dentes de 10 × 6, passo 30, o primeiro a 22 mm da borda esquerda", "ts", "middle")
p.hdim(ox, ox+mm(22), oy-mm(20)-10, "22"); p.hdim(ox+mm(22), ox+mm(32), oy-mm(20)-10, "10"); p.hdim(ox+mm(32), ox+mm(52), oy-mm(20)-10, "20")
p.text(60, 400, "As paredes esquerda/direita recebem RANHURAS onde a frente/trás têm dentes (complementares).", "ts")
p.text(60, 414, "O fundo tem dentes para cima nas 4 bordas; as paredes têm ranhuras em baixo. Um gerador (boxes.py) produz o DXF a partir destes números.", "ts")

p.text(60, 450, "Ferragens e consumíveis", "hd")
hw = [("parafuso M3 × 12 escareado", "6", "tampa → blocos"),
      ("inserto M3 para acrílico", "6", "nos blocos"),
      ("espaçador M3 × 8 fêmea-fêmea + parafuso M3 × 6", "4 + 8", "carrier"),
      ("espaçador M3 × 6 + parafuso", "4 + 8", "módulo do amp — confirmar padrão"),
      ("pé de borracha ø15 adesivo", "4", "fundo"),
      ("cimento de acrílico (Weld-On 4 ou equiv.)", "1", "juntas e blocos"),
      ("porcas dos jacks / pots / encoder / DPDT", "vêm com as peças", "—"),
      ("cabo USB-C curto 10–15 cm", "1", "berço → decoy")]
y = 470
for r in hw:
    p.text(60, y, r[0], "ts"); p.text(380, y, r[1], "ts"); p.text(480, y, r[2], "ts"); y += 15
p.text(60, 620, "Ordem de fabrico: cortar UMA caixa → montar a base (sem tampa) → conferir furos com as peças → só então a tampa → só então a série.", "warn")
ok5 = p.save(OUT + "caixa-fabricacao-p5-juntas.svg")

# ======================= PAGINA 6: BERCO DO POWERBANK =======================
p = Page(1000, 560, "Página 6 — BERÇO DO POWERBANK (opcional) · na face de TRÁS",
         "Rebaixo em acrílico 6 mm, à medida de UM modelo. Powerbank de referência: a definir (típico 100 × 60 × 25). O berço ocupa x 0..110 da face de trás; os conectores movem-se para a direita.")
ox, oy = 60, 380
p.face(ox, oy, 200, 70, "TRÁS com berço — datum canto esquerdo-baixo")
p.zone(ox, oy, 4, 4, 110, 66, "BERÇO 106 × 62", "powerbank deitado, cabo curto ao decoy")
p.rect(ox, oy, 30, 30, 12, 6, "USB-C", "")
for (x, y) in [(8, 8), (106, 8), (8, 62), (106, 62)]:
    cx, cy = ox+mm(x), oy-mm(y)
    p.o.append(f'<circle class="h" cx="{cx}" cy="{cy}" r="{mm(1.6)}"/>')
p.text(620, 340, "Berço: 4× ø3,2 em (8 | 106, 8 | 62). USB-C dentro do berço em (30, 30).", "ts")
p.hole(ox, oy, 124, 38, 9.5, "linha L", "", lab_dy=15); p.hole(ox, oy, 148, 38, 9.5, "linha R", "", lab_dy=34)
p.hole(ox, oy, 172, 38, 6, "DPDT L", "", lab_dy=15); p.hole(ox, oy, 190, 38, 6, "DPDT R", "", lab_dy=34)
p.text(620, 354, "Furos de linha e DPDT: confirmar diâmetros nas peças.", "warn")
p.text(620, 120, "Como funciona", "hd")
p.text(620, 140, "· Rebaixo de 6 mm com abas laterais de 8 mm: o powerbank entra por cima e", "ts")
p.text(620, 154, "  fica preso por uma tira elástica ou mola de chapa.", "ts")
p.text(620, 168, "· O display do powerbank fica visível. O calor fica fora da caixa.", "ts")
p.text(620, 182, "· Troca em 5 s, sem abrir nada.", "ts")
p.text(620, 202, "O preço", "hd")
p.text(620, 222, "· O berço é de UM modelo. Escrever no brief: 'powerbank de referência X,", "ts")
p.text(620, 236, "  A × B × C mm'. Se sair de linha, redesenha-se o berço, não o instrumento.", "ts")
p.text(620, 250, "· Os conectores da face de trás mudam para a direita (cotas acima).", "ts")
p.text(620, 264, "· DPDT a 16 mm entre centros: porcas de ø12 ainda cabem.", "ts")
p.text(620, 284, "Alternativa", "hd")
p.text(620, 304, "Baía interna ao longo da parede de trás: a caixa cresce 30 em profundidade,", "ts")
p.text(620, 318, "ganha uma porta, o powerbank some — e o display e o calor ficam dentro.", "ts")
ok6 = p.save(OUT + "caixa-fabricacao-p6-berco.svg")
print("TODAS LIMPAS" if all([ok2, ok3, ok4, ok5, ok6]) else "HÁ COLISÕES")
