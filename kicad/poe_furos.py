#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""poe_furos.py — os 4 furos M3 que seguram o cerebro ao fundo da caixa.

    poe_furos.py [--escreve]

⚠️⚠️ AS COORDENADAS SAO DERIVADAS DA CAIXA, NUNCA ESCRITAS A MAO.

Saem do `pcb_contorno.FUNDO["espacadores"]`, que sai do modelo da caixa. O
`pcb_contorno` ja prova a algebra: a posicao do furo RELATIVA A PLACA e
constante — (15,5 · 128,5) x (3,5 · 96,5) do canto — **para qualquer espessura
de acrilico**, porque a parede que se desconta entra dos dois lados. Escrever
`(35.5, 23.5)` aqui seria verdade para T=6 e mentira para T=3, e mentira do
tipo que so aparece com a caixa em cima da mesa.

⚠️ E O FURO E DA CAIXA: QUEM SE MOVE E A PECA. O `confere_furos` recusa antes
de escrever se algum cair debaixo de um conector — porque um parafuso que nao
entra descobre-se com a caixa fechada e a solda feita.

⚠️ KEEPOUT DE COBRE. As zonas de `GND`/`AGND` inundam o `B.Cu` ate a folga
normal, e um parafuso M3 com anilha o6,5 encosta a cabeca metalica ao cobre.
Cada furo leva um circulo de exclusao de 3,5 mm de raio — a anilha mais folga.
"""

VERSAO = "0.2.0"

import io
import math
import os
import re
import subprocess
import sys
import tempfile
import uuid as _uuid

AQUI = os.path.dirname(os.path.abspath(__file__))
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
NS = _uuid.uuid5(_uuid.NAMESPACE_DNS, "poe-furos.cotucatronics")
FP = ("/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints/"
      "MountingHole.pretty/MountingHole_3.2mm_M3.kicad_mod")
RAIO_LIVRE = 3.5          # anilha o6,5 / 2 + folga


def posicoes():
    """(x, y) dos quatro furos em coordenadas KiCad, vindas da caixa."""
    sys.path.insert(0, AQUI)
    import pcb_contorno as P
    alvo = dict(P.PLACAS["cerebro"])
    if alvo["x0"] is None:
        alvo["x0"] = round((P.FUNDO["w"] - alvo["w"]) / 2, 3)
        alvo["y0"] = round((P.FUNDO["h"] - alvo["h"]) / 2, 3)
    ox, oy = alvo["origem"]
    out = [(round(ox + (ex - alvo["x0"]), 3),
            round(oy + (alvo["h"] - (ey - alvo["y0"])), 3))
           for ex, ey in P.FUNDO["espacadores"]]
    # ⚠️ guarda: quatro, e nao "os que houver". Se a caixa mudar de numero de
    # espacadores, isto grita em vez de silenciosamente por menos furos.
    if len(out) != 4:
        raise SystemExit("*** a caixa tem %d espacadores o3,2 e a placa espera "
                         "4 — decidir qual manda antes de furar." % len(out))
    return sorted(out), alvo, P


def num(v):
    """§2b: nenhum numero entra num ficheiro KiCad em notacao cientifica."""
    return "%g" % (round(float(v), 4) + 0.0)


def limpa(t):
    """Tira os furos e os keepouts desta ferramenta — idempotencia."""
    n = 0
    for marca in ('\n\t(footprint "MountingHole:', '\n\t(zone'):
        i = 0
        while True:
            j = t.find(marca, i)
            if j < 0:
                break
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
            if NS.hex[:8] in bl:
                t = t[:j] + t[k + 1:]
                n += 1
                i = j
            else:
                i = k + 1
    return t, n


def main():
    print("poe_furos v%s" % VERSAO)
    escreve = "--escreve" in sys.argv
    furos, alvo, P = posicoes()
    p = os.path.join(AQUI, alvo["ficheiro"])
    t = io.open(p, encoding="utf-8").read()
    t, tirados = limpa(t)

    # ⚠️ o furo e da caixa: se estiver ocupado, quem se move e a peca. Isto
    # recusa ANTES de escrever, porque um furo tapado descobre-se com a caixa
    # fechada e a solda feita.
    for l in P.confere_furos(t, alvo):
        print("   " + l.strip())

    mod = io.open(FP, encoding="utf-8").read()
    novo = ""
    for i, (x, y) in enumerate(furos, 1):
        u = _uuid.uuid5(NS, "furo:%d" % i)
        uu = NS.hex[:8] + str(u)[8:]
        b = mod.replace('(footprint "MountingHole_3.2mm_M3"',
                        '(footprint "MountingHole:MountingHole_3.2mm_M3"\n'
                        '\t\t(layer "F.Cu")\n\t\t(uuid "%s")\n'
                        '\t\t(at %s %s)' % (uu, num(x), num(y)), 1)
        b = b.replace('"REF**"', '"H%d"' % i, 1)
        # ⚠️ §1 do ppc/002: ESCONDER a referencia de silk dos furos. O furo e
        # mecanico e o "Hn" na serigrafia so suja a placa por cima do cobre. Sai
        # da FONTE (aqui), nao a mao no .kicad_pcb — senao a proxima regeneracao
        # apaga o conserto em silencio (skill `kicad` §8: o gerador e a fonte).
        # (Contraste com o painel, §2, onde a serigrafia NUNCA se esconde.)
        # A mesma indentacao do `(layer ...)` mantem o bloco byte-a-byte igual ao
        # que o KiCad grava.
        b = re.sub(r'(\(property "Reference" "H%d"[\s\S]*?\n)([ \t]*)'
                   r'(\(layer "F\.SilkS"\)\n)' % i,
                   r'\1\2\3\2(hide yes)\n', b, count=1)
        b = b.replace('\n\t(layer "F.Cu")\n', '\n', 1)   # ja posto acima
        novo += "\t" + b.replace("\n", "\n\t").rstrip("\t") + "\n"
        # keepout circular: 12 lados, que a conta de poligono ja existe no KiCad
        pts = "".join("\t\t\t\t(xy %s %s)\n"
                      % (num(x + RAIO_LIVRE * math.cos(k * math.pi / 6)),
                         num(y + RAIO_LIVRE * math.sin(k * math.pi / 6)))
                      for k in range(12))
        novo += ('\t(zone\n\t\t(net 0)\n\t\t(net_name "")\n'
                 '\t\t(layers "F.Cu" "B.Cu")\n\t\t(uuid "%s")\n'
                 '\t\t(name "keepout_furo_%d")\n\t\t(hatch edge 0.5)\n'
                 '\t\t(connect_pads\n\t\t\t(clearance 0)\n\t\t)\n'
                 '\t\t(min_thickness 0.25)\n\t\t(filled_areas_thickness no)\n'
                 '\t\t(keepout\n\t\t\t(tracks not_allowed)\n\t\t\t(vias not_allowed)\n'
                 '\t\t\t(pads allowed)\n\t\t\t(copperpour not_allowed)\n'
                 '\t\t\t(footprints allowed)\n\t\t)\n'
                 '\t\t(placement\n\t\t\t(enabled no)\n\t\t\t(sheetname "")\n\t\t)\n'
                 '\t\t(polygon\n\t\t\t(pts\n%s\t\t\t)\n\t\t)\n\t)\n'
                 % (NS.hex[:8] + str(_uuid.uuid5(NS, "keep:%d" % i))[8:], i, pts))

    k = t.rstrip().rindex(")")
    t = t.rstrip()[:k] + novo + ")\n"
    print("   %d furo(s) M3 + keepout o%.1f · %d resto(s) da corrida anterior"
          % (len(furos), RAIO_LIVRE * 2, tirados))

    if not escreve:
        print("   ensaio: nada gravado. Junta --escreve para aplicar.")
        return 0
    # ⚠️ tem de ABRIR antes de substituir o original (a licao do painel partido)
    d = tempfile.mkdtemp()
    c = os.path.join(d, os.path.basename(p))
    io.open(c, "w", encoding="utf-8").write(t)
    r = subprocess.run([KICAD, "pcb", "drc", "-o", os.path.join(d, "x.rpt"), c],
                       capture_output=True, text=True)
    if "Failed to load" in (r.stdout + r.stderr):
        print("*** o resultado NAO ABRE — nada gravado.\n    %s"
              % (r.stdout + r.stderr).strip().splitlines()[-1][:150])
        return 1
    io.open(p, "w", encoding="utf-8").write(t)
    print("   gravado (e confirmado que abre).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
