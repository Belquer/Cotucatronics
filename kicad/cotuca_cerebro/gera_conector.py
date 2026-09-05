#!/usr/bin/env python3
"""gera_conector v0.1.0 — a folha do CONECTOR DO PAINEL (IDC 2x13).

A pinagem sai do `ROTEIRO-KICAD.md` §5, e e o CONTRATO entre as duas placas:
o mesmo nome de rede dos dois lados, via a via. Este ficheiro e uma das duas
metades; a outra nasce no projecto do painel.

⚠️ NAO INVENTAR NOMES. Cada rotulo aqui tem de existir, com a mesma grafia,
na folha do soquete — senao as duas metades nao se encontram e o ERC nao diz
nada, porque um rotulo global sozinho e legal.

⚠️ A GUARDA DE TERRA NAO E ESTETICA. O relogio do OLED vai entre terras dos
dois lados (vias 1 e 3), os pots ensanduichados (23..26) e os LED em par
proprio (11..14): o painel aceso mediu 6 dB de chao na entrada, e o caminho
era este cabo.
"""
import io, sys, uuid

VERSAO = "0.1.0"
G = 1.27
def u(): return str(uuid.uuid4())
def g(v): return round(round(v / G) * G, 4)
L = []

# via -> (rotulo, nota).  Do ROTEIRO-KICAD §5, verbatim.
VIAS = {
     1: ("GND", ""),                  2: ("OLED_CLK", "D8 · guardado por terra"),
     3: ("GND", ""),                  4: ("OLED_DIN", "D10"),
     5: ("OLED_CS", "D7"),            6: ("OLED_DC", "D6"),
     7: ("OLED_RST", "D5"),           8: ("GND", ""),
     9: ("+3V3", ""),                10: ("GND", ""),
    11: ("+5V", "LED"),              12: ("+5V", "LED"),
    13: ("GND", "retorno LED"),      14: ("GND", "retorno LED"),
    15: ("LED_FRZ_A", "D27"),        16: ("LED_FRZ_B", "D23"),
    17: ("BOT_FRZ_A", "D24"),        18: ("BOT_FRZ_B", "D22"),
    # ⚠️ A VIA 19 DEIXOU DE SER TERRA a 05/09, para o botao branco dos
    # buzios. Das nove terras, era a unica que nao guardava nenhuma das tres
    # coisas medidas — o relogio do OLED (1 e 3), os pots (23 e 26) e o par
    # dos LED (13 e 14). E o que la entra e um botao: digital lento, corrente
    # nenhuma, o vizinho menos exigente que existe.
    19: ("BOT_BUZIOS", "D26 · botao branco"),
                                     20: ("ENC_A_A", "D0 · o unico encoder"),
    21: ("ENC_A_B", "D1"),           22: ("ENC_A_PUSH", "D2"),
    23: ("GND", ""),                 24: ("POT_L", "D15"),
    25: ("POT_R", "D16"),            26: ("GND", ""),
}

def simbolo(lib, ref, val, x, y):
    x, y = g(x), g(y)
    ps = "".join(
        f'\t\t(property "{k}" "{v}"\n\t\t\t(at {g(x+dx)} {g(y+dy)} 0)\n'
        f'\t\t\t{"(hide yes) " if h else ""}(show_name no) (do_not_autoplace no)\n'
        f'\t\t\t(effects (font (size {s} {s})))\n\t\t)\n'
        for k, v, dx, dy, s, h in
        (("Reference", ref, 0, -21.59, 1.27, False), ("Value", val, 0, 21.59, 1.0, False),
         ("Footprint", "", 0, 0, 1.27, True), ("Datasheet", "", 0, 0, 1.27, True),
         ("Description", "", 0, 0, 1.27, True)))
    L.append(f'\t(symbol\n\t\t(lib_id "cotuca:{lib}")\n\t\t(at {x} {y} 0)\n\t\t(unit 1)\n'
             f'\t\t(body_style 1)\n\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n\t\t(on_board yes)\n'
             f'\t\t(in_pos_files yes)\n\t\t(dnp no)\n\t\t(uuid "{u()}")\n{ps}\t)\n')

def fio(x1, y1, x2, y2):
    L.append(f'\t(wire\n\t\t(pts\n\t\t\t(xy {g(x1)} {g(y1)}) (xy {g(x2)} {g(y2)})\n\t\t)\n'
             f'\t\t(stroke (width 0) (type default))\n\t\t(uuid "{u()}")\n\t)\n')

def rotulo(nome, x, y, ang, just):
    L.append(f'\t(global_label "{nome}"\n\t\t(shape bidirectional)\n\t\t(at {g(x)} {g(y)} {ang})\n'
             f'\t\t(fields_autoplaced yes)\n\t\t(effects (font (size 1.27 1.27)) (justify {just}))\n'
             f'\t\t(uuid "{u()}")\n\t)\n')

def texto(s, x, y, tam=1.27):
    L.append(f'\t(text "{s}"\n\t\t(exclude_from_sim no)\n\t\t(at {g(x)} {g(y)} 0)\n'
             f'\t\t(effects (font (size {tam} {tam})) (justify left))\n\t\t(uuid "{u()}")\n\t)\n')

def pinos_de(nome):
    """offsets lidos da BIBLIOTECA, nunca de cabeca — ver gera_soquete.py."""
    import re as _re
    Lb = io.open("cotuca.kicad_sym", encoding="utf-8").read()
    i = Lb.find('(symbol "%s"' % nome); d = 0
    for k in range(i, len(Lb)):
        if Lb[k] == "(": d += 1
        elif Lb[k] == ")":
            d -= 1
            if d == 0: b = Lb[i:k+1]; break
    return [(m.group(3), float(m.group(1)), float(m.group(2))) for m in
            _re.finditer(r'\(pin\s+\S+\s+\S+\s*\n\s*\(at (-?[\d.]+) (-?[\d.]+)'
                         r'(?:\s+-?[\d.]+)?\)(?:.|\n)*?\(number "([^"]+)"', b)]

JX, JY = 160.0, 110.0
simbolo("IDC26", "J14", "IDC 2x13 -> painel", JX, JY)
for num, px, py in pinos_de("IDC26"):
    n = int(num); rot, nota = VIAS[n]
    x, y = JX + px, JY - py          # y do simbolo cresce para CIMA
    esq = px < 0
    dx = -10.16 if esq else 10.16
    fio(x, y, x + dx, y)
    rotulo(rot, x + dx, y, 180 if esq else 0, "right" if esq else "left")
    if nota: texto(nota, x + (-34.0 if esq else 26.0), y, 1.0)

texto("CONECTOR DO PAINEL — IDC 2x13. Pinagem do ROTEIRO-KICAD §5.", JX - 45, JY - 26, 1.6)
texto("E o CONTRATO entre as duas placas: mesmo nome de rede, via a via.", JX - 45, JY - 22.2)
texto("Guarda de terra: relogio do OLED entre terras (1 e 3), pots", JX - 45, JY + 24)
texto("ensanduichados (23..26), LED em par proprio (11..14). O painel", JX - 45, JY + 27.8)
texto("aceso mediu 6 dB de chao na entrada, e o caminho era este cabo.", JX - 45, JY + 31.6)

def lib_embutida(usados):
    Lb = io.open("cotuca.kicad_sym", encoding="utf-8").read(); out = []
    for nome in sorted(usados):
        i = Lb.find('(symbol "%s"' % nome); d = 0
        for k in range(i, len(Lb)):
            if Lb[k] == "(": d += 1
            elif Lb[k] == ")":
                d -= 1
                if d == 0: b = Lb[i:k+1]; break
        out.append("\t\t" + b.replace('(symbol "%s"' % nome, '(symbol "cotuca:%s"' % nome, 1)
                   .replace("\n  ", "\n\t\t"))
    return "\t(lib_symbols\n" + "\n".join(out) + "\n\t)\n"

CAB = f'''(kicad_sch
\t(version 20260306)
\t(generator "cotucatronics")
\t(generator_version "10.0")
\t(uuid "{u()}")
\t(paper "A3")
\t(title_block
\t\t(title "CONECTOR DO PAINEL — IDC 2x13 — carrier rev B")
\t\t(date "2026-09-05")
\t\t(rev "B")
\t\t(comment 1 "Pinagem do ROTEIRO-KICAD §5. Contrato entre as duas placas.")
\t\t(comment 2 "Guarda de terra por desenho, nao por estetica: 6 dB medidos.")
\t)
{lib_embutida({"IDC26"})}'''
io.open("cotuca_conector.kicad_sch", "w", encoding="utf-8").write(
    CAB + "".join(L) + '\t(sheet_instances\n\t\t(path "/" (page "5"))\n\t)\n\t(embedded_fonts no)\n)\n')
print("cotuca_conector.kicad_sch: %d blocos" % len(L), file=sys.stderr)
