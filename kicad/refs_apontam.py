#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""refs_apontam.py — cada referencia aponta para a PROPRIA peca?

    refs_apontam.py <ficheiro.kicad_pcb> [...]      sai != 0 se alguma nao aponta

⚠️⚠️ PORQUE E QUE O DRC NAO CHEGA

O DRC do KiCad responde "ha texto por cima de coisa?". Nao responde "este texto
diz respeito a QUE peca?" — e nao pode, porque isso nao e geometria proibida, e
significado.

Uma referencia a 11,5 mm da propria peca e a 5 mm da vizinha passa em TODOS os
verificadores: nao colide com nada, nao esta sobre cobre, nao sai da placa. E
identifica a peca errada. Quem solda a mao le `C9` ao lado do `R24` e solda o
condensador no sitio do resistor.

⚠️ Isto aconteceu, e a §11g-ter da skill `/kicad` ja o tinha PREVISTO por
escrito — *"uma referencia tecnicamente livre pode estar encostada a peca
errada, e nenhum DRC tem opiniao sobre isso"* — sem que nada o medisse. Estava
diagnosticado e por instrumentar, que e o estado em que um defeito vive mais
tempo: toda a gente sabe, ninguem conta.

⚠️ A CAUSA ESTAVA A MONTANTE. O `sincroniza_pcb` criava toda peca nova com a
referencia a `-11,5 mm` — uma constante, igual para um condensador de 11 mm e
para um ponto de teste de 1,5 mm. Eram 23 das 70 do cerebro. O `arruma_silk`,
que so mexia no que COLIDIA, deixava-as: a 11,5 mm nao se colide com nada.
"""

VERSAO = "0.1.0"

import io
import math
import os
import re
import sys

NUM = r"-?\d+(?:\.\d+)?"


def blocos(t):
    """⚠️ Conta parentesis SALTANDO AS CADEIAS (§2d). Um `descr` de footprint
    tem parentesis dentro de aspas — `length*width=7.2*11mm^2` e amigos — e um
    contador cru dessincroniza no primeiro e devolve blocos todos errados."""
    i = 0
    while True:
        j = t.find('\n\t(footprint "', i)
        if j < 0:
            return
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
        yield t[j:k + 1]
        i = k + 1


def le(caminho):
    t = io.open(caminho, encoding="utf-8").read()
    fora = []
    for bl in blocos(t):
        r = re.search(r'\(property "Reference" "([^"]+)"', bl)
        a = re.search(r'\n\t\t\(at (' + NUM + r') (' + NUM + r')(?: (' + NUM + r'))?\)', bl)
        if not r or not a:
            continue
        ref = r.group(1)
        # ⚠️ o que nao entra na BOM nem no CPL nao tem referencia a apontar:
        # net-ties e bandeiras de alimentacao sao cobre e convencao.
        if ref.startswith("#") or "exclude_from_bom" in bl:
            continue
        fx, fy = float(a.group(1)), float(a.group(2))
        ang = math.radians(float(a.group(3) or 0))
        pr = re.search(r'\(property "Reference" "[^"]+"\s*\n\s*\(at (' + NUM
                       + r') (' + NUM + r')', bl)
        dx, dy = (float(pr.group(1)), float(pr.group(2))) if pr else (0.0, 0.0)
        # ⚠️ o campo RODA com a peca — ver `arruma_silk.abs_do_campo` e a
        # §11g-quater. Somar o offset cru poe o texto do outro lado do modulo.
        X = fx + dx * math.cos(ang) + dy * math.sin(ang)
        Y = fy - dx * math.sin(ang) + dy * math.cos(ang)
        fora.append((ref, fx, fy, X, Y))
    # ⚠️ guarda de leitura (§2c): uma placa com zero pecas nao e uma placa
    # limpa, e um leitor partido — e leitor partido aprova tudo.
    if not fora:
        raise SystemExit("*** %s: zero pecas lidas — LEITURA INCOMPLETA"
                         % os.path.basename(caminho))
    return fora


def confere(caminho):
    pecas = le(caminho)
    maus = []
    for ref, fx, fy, X, Y in pecas:
        dp = math.hypot(X - fx, Y - fy)
        dm, quem = min(((math.hypot(X - ox, Y - oy), oref)
                        for oref, ox, oy, _, _ in pecas if oref != ref),
                       key=lambda z: z[0])
        if dm < dp:
            maus.append((ref, dp, quem, dm))
    maus.sort(key=lambda z: -(z[1] - z[3]))
    print("  %-26s %d pecas · %d referencia(s) a apontar para a peca errada"
          % (os.path.basename(caminho), len(pecas), len(maus)))
    for ref, dp, quem, dm in maus:
        print("     ⚠️ %-5s a %.1f mm da sua e a %.1f mm de %s" % (ref, dp, dm, quem))
    return len(maus)


def main():
    print("refs_apontam v%s" % VERSAO)
    alvos = [a for a in sys.argv[1:] if not a.startswith("-")]
    if not alvos:
        raise SystemExit("USO: refs_apontam.py <ficheiro.kicad_pcb> [...]")
    mau = sum(confere(a) for a in alvos)
    if mau:
        raise SystemExit("*** %d referencia(s) identificam a peca errada. Quem "
                         "solda a mao le o designador que esta mais perto." % mau)
    return 0


if __name__ == "__main__":
    sys.exit(main())
