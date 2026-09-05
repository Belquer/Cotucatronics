#!/usr/bin/env python3
# Desenho de fabricacao da caixa (cunha 200 x 120, frente 40 / tras 70)
# Cotas em mm a partir do datum: canto ESQUERDA-FRENTE-BAIXO.
# ⚠️ CAMINHOS RELATIVOS AO PROPRIO FICHEIRO, e o portao e o do repo.
# A versao que chegou escrevia em `/mnt/user-data/outputs/` e nao chamava
# verificador nenhum. Agora escreve ao lado de si e passa pelo
# `tools/svg_colisoes.py` — o do repo, v0.2.0, nao a copia de 7344 bytes que
# veio junto e que aprovava com zero textos.
import subprocess, sys, pathlib

AQUI = pathlib.Path(__file__).resolve().parent
RAIZ = AQUI.parents[2]
PORTAO = RAIZ / "tools" / "svg_colisoes.py"

S = 2.6  # px por mm

def mm(v): return round(v * S, 2)

out = []
W_, H_ = 1520, 1240
out.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W_}" height="{H_}" viewBox="0 0 {W_} {H_}" font-family="Helvetica, Arial, sans-serif">')
out.append(f'<rect width="{W_}" height="{H_}" fill="#fff"/>')
out.append('''<style>
 .box{fill:#fafafa;stroke:#111;stroke-width:1.8}
 .h{fill:#fdeceb;stroke:#b8322c;stroke-width:1.4}
 .hc{fill:none;stroke:#b8322c;stroke-width:1}
 .d{stroke:#3a6ea5;stroke-width:0.8;fill:none}
 .dt{font-size:9.5px;fill:#3a6ea5}
 .t{font-size:10px;fill:#111}
 .ts{font-size:8.5px;fill:#555}
 .hd{font-size:12.5px;fill:#111;font-weight:bold}
 .ti{font-size:17px;fill:#111;font-weight:bold}
 .warn{font-size:9px;fill:#8a4a00}
</style>''')

def text(x, y, s, cls="t", anchor="start"):
    out.append(f'<text class="{cls}" x="{x}" y="{y}" text-anchor="{anchor}">{s}</text>')

def hdim(x0, x1, y, label, above=True):
    """cota horizontal em px, label centrado"""
    out.append(f'<path class="d" d="M{x0} {y} L{x1} {y} M{x0} {y-4} L{x0} {y+4} M{x1} {y-4} L{x1} {y+4}"/>')
    text((x0+x1)/2, y-7 if above else y+14, label, "dt", "middle")

def vdim(x, y0, y1, label):
    out.append(f'<path class="d" d="M{x} {y0} L{x} {y1} M{x-4} {y0} L{x+4} {y0} M{x-4} {y1} L{x+4} {y1}"/>')
    out.append(f'<text class="dt" x="{x+5}" y="{(y0+y1)/2+3}">{label}</text>')

def hole(ox, oy, x, y, d, label, note=""):
    """furo circular; x,y em mm no referencial da face; ox,oy = origem da face em px (canto inferior-esquerdo)"""
    cx, cy = ox + mm(x), oy - mm(y)
    out.append(f'<circle class="h" cx="{cx}" cy="{cy}" r="{mm(d/2)}"/>')
    out.append(f'<path class="hc" d="M{cx-mm(d/2)-4} {cy} L{cx+mm(d/2)+4} {cy} M{cx} {cy-mm(d/2)-4} L{cx} {cy+mm(d/2)+4}"/>')
    text(cx, cy + mm(d/2) + 15, f"{label} ø{d}", "t", "middle")
    text(cx, cy + mm(d/2) + 25, f"({x}, {y})", "ts", "middle")
    if note: text(cx, cy + mm(d/2) + 34, note, "warn", "middle")

def rect_cut(ox, oy, x, y, w, h, label, note=""):
    """recorte retangular centrado em x,y"""
    cx, cy = ox + mm(x), oy - mm(y)
    out.append(f'<rect class="h" x="{cx-mm(w/2)}" y="{cy-mm(h/2)}" width="{mm(w)}" height="{mm(h)}"/>')
    text(cx, cy + mm(h/2) + 11, f"{label} {w}×{h}", "t", "middle")
    text(cx, cy + mm(h/2) + 21, f"centro ({x}, {y})", "ts", "middle")
    if note: text(cx, cy + mm(h/2) + 30, note, "warn", "middle")


def ordinate(ox, oy, w, h, feats, xrows=2, yrows=2):
    """cotas ordinais: origem 0 no canto inferior-esquerdo da face; cada x na
    base, cada y na esquerda, com linhas testemunha e rotulos escalonados."""
    xs = sorted(set(round(f[0], 2) for f in feats)); ys = sorted(set(round(f[1], 2) for f in feats))
    base = oy + 30
    out.append(f'<path class="d" d="M{ox} {base} L{ox+mm(w)} {base}"/>')
    text(ox, base + 12, "0", "dt", "middle")
    for i, x in enumerate(xs):
        px = ox + mm(x); row = i % xrows
        out.append(f'<path class="d" d="M{px} {oy} L{px} {base + 4 + row*11}" stroke-dasharray="2 2"/>')
        text(px, base + 12 + row*11, f"{x:g}", "dt", "middle")
    text(ox + mm(w), base + 12 + xrows*11 + 2, f"{w:g}", "dt", "middle")
    left = ox - 30
    out.append(f'<path class="d" d="M{left} {oy} L{left} {oy-mm(h)}"/>')
    text(left - 4, oy + 3, "0", "dt", "end")
    for i, y in enumerate(ys):
        py = oy - mm(y); row = i % yrows
        out.append(f'<path class="d" d="M{ox} {py} L{left - 4 - row*30} {py}" stroke-dasharray="2 2"/>')
        text(left - 6 - row*30, py + 3, f"{y:g}", "dt", "end")
    text(left - 6, oy - mm(h) + 3, f"{h:g}", "dt", "end")

def face(ox, oy, w, h, title):
    """face retangular; origem px = canto inferior esquerdo"""
    out.append(f'<rect class="box" x="{ox}" y="{oy-mm(h)}" width="{mm(w)}" height="{mm(h)}"/>')
    text(ox, oy - mm(h) - 8, title, "hd")

text(28, 30, "Cotucatronics — caixa cunha · desenho de fabricação", "ti")
text(28, 48, "Cotas em mm. Datum de cada face = canto INFERIOR-ESQUERDO da face, visto de fora. (x, y) = centro do furo. Paredes 6 mm, tampa 9 mm.", "ts")
text(28, 61, "⚠ Diâmetros marcados 'confirmar' dependem da peça em mão — furar só depois de medir.", "warn")

# ================= TOPO (200 x 123.7 no plano, inclinado) =================
ox, oy = 110, 420
face(ox, oy, 200, 123.7, "TOPO (tampa, 9 mm) — 200 × 123,7 no plano · y medido ao longo da inclinação, a partir da FRENTE")
hole(ox, oy, 50, 35, 30, "freeze A")
hole(ox, oy, 150, 35, 30, "freeze B")
rect_cut(ox, oy, 45, 92, 55, 27.5, "janela OLED", "+4 furos ø2,5 do módulo — confirmar")
hole(ox, oy, 105, 92, 7, "encoder", "ø7 ou 9 — confirmar EC11")
hole(ox, oy, 140, 92, 7, "pot L", "confirmar")
hole(ox, oy, 175, 92, 7, "pot R", "confirmar")
hdim(ox+mm(50), ox+mm(150), oy-mm(35)-mm(15)-6, "100 entre eixos")
text(ox+mm(100), oy-mm(62), "26 mm livres entre os freeze e os knobs", "ts", "middle")
ordinate(ox, oy, 200, 123.7, [(50,35),(150,35),(45,92),(105,92),(140,92),(175,92)])

# ================= PERFIL (cunha) =================
px0, py0 = 780, 420
text(px0, py0-mm(70)-40, "PERFIL (vista da direita) — frente 40, trás 70", "hd")
pts = [(px0, py0), (px0+mm(120), py0), (px0+mm(120), py0-mm(70)), (px0, py0-mm(40))]
out.append('<polygon class="box" points="' + " ".join(f"{x},{y}" for x,y in pts) + '"/>')
hdim(px0, px0+mm(120), py0+14, "120 (profundidade)", above=False)
vdim(px0-16, py0-mm(40), py0, "40")
vdim(px0+mm(120)+10, py0-mm(70), py0, "70")
text(px0+mm(60), py0-mm(28), "tampa inclinada: 123,7 no plano · ângulo 14°", "ts", "middle")
text(px0+mm(6), py0-mm(8), "FRENTE", "ts"); text(px0+mm(120)-40, py0-mm(8), "TRÁS", "ts")
text(px0, py0+40, "Zona baixa (frente): só peças até 8 mm por baixo dos freeze — pré-amp, step-down, ESP32.", "ts")
text(px0, py0+52, "Zona alta (trás): Daisy no soquete (~20), módulo do amp com dissipador (~25), eletrolíticos.", "ts")

# ================= ESQUERDA (120 x 40..70) =================
ox, oy = 110, 740
text(ox, oy-mm(70)-8, "ESQUERDA — piezos IN · datum = canto FRENTE-BAIXO", "hd")
pts = [(ox, oy), (ox+mm(120), oy), (ox+mm(120), oy-mm(70)), (ox, oy-mm(40))]
out.append('<polygon class="box" points="' + " ".join(f"{x},{y}" for x,y in pts) + '"/>')
hole(ox, oy, 35, 22, 9.5, "piezo A", "TRS ¼\" — ø9,5 ou 12, confirmar")
hole(ox, oy, 85, 22, 9.5, "piezo B", "confirmar")
ordinate(ox, oy, 120, 70, [(35,22),(85,22)], yrows=1)

# ================= DIREITA (120 x 70..40, vista de fora) =================
ox, oy = 520, 740
text(ox, oy-mm(70)-8, "DIREITA — exciters OUT · datum TRÁS-BAIXO", "hd")
pts = [(ox, oy), (ox+mm(120), oy), (ox+mm(120), oy-mm(40)), (ox, oy-mm(70))]
out.append('<polygon class="box" points="' + " ".join(f"{x},{y}" for x,y in pts) + '"/>')
rect_cut(ox, oy, 35, 24, 24, 12, "exciter A", "borne 2 polos — confirmar")
rect_cut(ox, oy, 85, 24, 24, 12, "exciter B", "confirmar")
ordinate(ox, oy, 120, 70, [(35,24),(85,24)], yrows=1)

# ================= TRÁS (200 x 70) =================
ox, oy = 110, 1060
face(ox, oy, 200, 70, "TRÁS — mundo · datum = canto ESQUERDO-BAIXO, visto de fora")
rect_cut(ox, oy, 34, 38, 12, 6, "USB-C", "recorte do decoy — confirmar")
hole(ox, oy, 75, 38, 9.5, "linha L", "confirmar")
hole(ox, oy, 105, 38, 9.5, "linha R", "confirmar")
hole(ox, oy, 155, 38, 6, "DPDT L", "ø6 — confirmar")
hole(ox, oy, 180, 38, 6, "DPDT R", "confirmar")
ordinate(ox, oy, 200, 70, [(34,38),(75,38),(105,38),(155,38),(180,38)], yrows=1)
text(ox+mm(55), oy-mm(62), "ventilação: 6 rasgos 3×8 em cima, alinhados com o dissipador", "ts", "middle")
for i in range(6):
    x = ox+mm(112)+i*mm(14)
    out.append(f'<rect class="h" x="{x}" y="{oy-mm(66)}" width="{mm(3)}" height="{mm(8)}"/>')

# ================= FRENTE (200 x 40) =================
ox, oy = 950, 620
face(ox, oy, 200, 40, "FRENTE — marca · 200 × 40 · SEM furos")
text(ox+mm(100), oy-mm(20)+4, "búzio + COTUCATRONICS (gravado ou serigrafado)", "ts", "middle")

# ================= FUNDO (200 x 120) =================
ox, oy = 950, 1060
face(ox, oy, 200, 120, "FUNDO · 4 pés ø15 a 12 mm dos cantos · ventilação sob o amp")
for (x, y) in [(12, 12), (188, 12), (12, 108), (188, 108)]:
    cx, cy = ox+mm(x), oy-mm(y)
    out.append(f'<circle class="hc" cx="{cx}" cy="{cy}" r="{mm(7.5)}" stroke-dasharray="3 2"/>')
for i in range(6):
    y = oy-mm(30)-i*mm(12)
    out.append(f'<rect class="h" x="{ox+mm(148)}" y="{y}" width="{mm(40)}" height="{mm(3)}"/>')
text(ox+mm(168), oy-mm(24), "6 rasgos 40×3", "ts", "middle")
ordinate(ox, oy, 200, 120, [(12,12),(188,12),(12,108),(188,108),(22,10),(142,110)], xrows=2, yrows=2)
text(ox+mm(82), oy-mm(60), "carrier 120×100 · (22..142, 10..110) · 4 espaçadores M3", "ts", "middle")
out.append(f'<rect class="hc" x="{ox+mm(22)}" y="{oy-mm(110)}" width="{mm(120)}" height="{mm(100)}" stroke-dasharray="4 3"/>')

# ================= rodapé =================
text(60, 1150, "INTERIOR ÚTIL (paredes 6): 188 × 108 no chão · 28 mm livres sob a frente · 58 sob o trás", "hd")
text(60, 1168, "Tampa de 9 mm: o pé carrega nela. 6 mm flete sob os freeze.", "ts")
text(60, 1180, "Juntas a dedo (finger joint) para corte a laser; ou caixa CNC em madeira maciça.", "ts")
text(60, 1192, "Tampa aparafusada por dentro em 6 pontos, com inserto roscado — abre para manutenção.", "ts")
text(60, 1210, "Ordem: 1) medir todas as peças marcadas 'confirmar'  2) atualizar este desenho", "warn")
text(60, 1222, "3) cortar UMA caixa e montar  4) só depois a série.", "warn")
out.append('</svg>')
FN = str(AQUI / "caixa-fabricacao.svg")
open(FN, "w").write("\n".join(out))

# ⚠️ O PORTAO CORRE AQUI, E PODE DIZER NAO. Gerador que so escreve deixa a
# ultima milha para a disciplina humana, e e ai que o desenho se descola.
_r = subprocess.run([sys.executable, str(PORTAO), FN], capture_output=True, text=True)
print(_r.stdout.strip())
if _r.returncode != 0:
    print("  *** pagina 1 NAO passou o portao (codigo %d) ***" % _r.returncode)
    sys.exit(_r.returncode)
print("ok")
