#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""vias_tented.py — as vias estao TAPADAS (tented)?

    vias_tented.py <ficheiro.kicad_pcb> [...]   sai != 0 se alguma via estiver aberta
    vias_tented.py --auto-teste                 prova que a guarda tem dentes

⚠️⚠️ PORQUE E QUE ISTO E UM PORTAO, E NAO SO UM AVISO

A JLCONE marca a classe DFM "solder mask opening exposing trace" (medido a
2026-09-11: 14 laranja no painel). Medido no gerber exportado, o painel entrega
esta classe BENIGNA por uma razao concreta e fragil: **as 34 vias estao todas
tapadas** (0 aberturas de mascara sobre via) e a expansao de mascara dos pads e
0,0 mm. Nesse estado a laranja nasce so da expansao de casa da JLC aplicada ao
pescoco pad/pista — inerente, cosmetica (o pescoco fica no filete de solda).

⚠️ Mas isso vale ENQUANTO as vias estiverem tapadas. Se alguem destapar as vias
— o `(tenting (front no)(back no))` no setup, ou um override numa via — as 34
vias passam a ter abertura de mascara sobre cobre, e a classe da JLC deixa de ser
laranja-cosmetica e passa a **vermelho real**: cobre exposto a oxidar, e a pista
que entra na via exposta com ela. Nenhum DRC do KiCad recusa por isto (destapar
via e uma escolha legitima de desenho, nao um erro de geometria). Por isso e uma
guarda propria, no portao de fabrica, a preservar o invariante que torna a classe
benigna.

⚠️ MEDIDO NO TEXTO, sem pcbnew (como o `refs_apontam`): a tapagem por omissao
vive no `(tenting (front yes)(back yes))` do `setup`, e uma via so fica aberta se
tiver um override proprio a destapar. Le-se o default e procura-se o override.
"""

VERSAO = "0.1.0"

import io
import os
import re
import sys


def _default_tenting(t):
    """(front, back) do bloco (tenting ...) do setup; default do KiCad e tapado."""
    m = re.search(r"\(tenting\s+\(front\s+(yes|no)\)\s*\(back\s+(yes|no)\)", t)
    if not m:
        # sem bloco: o KiCad 9/10 tapa por omissao
        return True, True
    return m.group(1) == "yes", m.group(2) == "yes"


def _via_blocos(t):
    """cada bloco `(via ...)` completo, contando parentesis e saltando cadeias."""
    i = 0
    while True:
        j = t.find("\t(via\n", i)
        if j < 0:
            j = t.find("\t(via ", i)
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


def _via_aberta(bl, df, db):
    """(front_aberta, back_aberta) de uma via, honrando override proprio.

    Override possiveis no KiCad: `(tenting front back)`, `(tenting none)`,
    `(tenting (front no)(back yes))`. Sem override, herda o default do setup."""
    m = re.search(r"\(tenting\b([^)]*(?:\([^)]*\)[^)]*)*)\)", bl)
    if not m:
        return (not df), (not db)
    body = m.group(1)
    fm = re.search(r"\(front\s+(yes|no)\)", body)
    bm = re.search(r"\(back\s+(yes|no)\)", body)
    if fm or bm:
        f = (fm.group(1) == "yes") if fm else df
        b = (bm.group(1) == "yes") if bm else db
        return (not f), (not b)
    if "none" in body:
        return True, True
    # `(tenting front back)` -> tapa ambos
    return False, False


def confere(caminho):
    t = io.open(caminho, encoding="utf-8").read()
    df, db = _default_tenting(t)
    vias = list(_via_blocos(t))
    # ⚠️ guarda de leitura (§2c): zero vias lidas de uma placa que tem vias e um
    # leitor partido, e leitor partido aprova tudo. So se aceita zero se o texto
    # tambem nao tiver a palavra `(via` — placa realmente sem vias.
    if not vias and "\t(via" in t:
        raise SystemExit("*** %s: zero vias lidas mas o ficheiro tem `(via` — "
                         "LEITURA INCOMPLETA" % os.path.basename(caminho))
    abertas = []
    for bl in vias:
        fa, ba = _via_aberta(bl, df, db)
        if fa or ba:
            at = re.search(r"\(at ([-\d.]+) ([-\d.]+)", bl)
            xy = ("@(%s,%s)" % (at.group(1), at.group(2))) if at else ""
            lados = ",".join(s for s, v in (("F", fa), ("B", ba)) if v)
            abertas.append((xy, lados))
    print("  %-26s %d via(s) · default tenting F/B=%s/%s · %d aberta(s)"
          % (os.path.basename(caminho), len(vias),
             "yes" if df else "no", "yes" if db else "no", len(abertas)))
    for xy, lados in abertas[:20]:
        print("     ⚠️ via aberta %s (mascara %s)" % (xy, lados))
    return len(abertas)


def _auto_teste():
    """Prova que a guarda tem dentes: injecta uma via destapada e exige recusa."""
    tapado = ('(kicad_pcb (version 20260206)\n'
              '\t(setup\n\t\t(tenting\n\t\t\t(front yes)\n\t\t\t(back yes)\n\t\t)\n\t)\n'
              '\t(via\n\t\t(at 10 10)\n\t\t(size 0.6)\n\t\t(drill 0.3)\n'
              '\t\t(layers "F.Cu" "B.Cu")\n\t)\n)\n')
    # 1) via a herdar o default tapado -> 0 abertas
    import tempfile
    ok = True
    for txt, esperado, nome in (
        (tapado, 0, "default tapado"),
        (tapado.replace("(front yes)", "(front no)"), 1, "default destapado F"),
        (tapado.replace('(drill 0.3)\n',
                        '(drill 0.3)\n\t\t(tenting none)\n'), 1, "override none")):
        with tempfile.NamedTemporaryFile("w", suffix=".kicad_pcb", delete=False) as f:
            f.write(txt); p = f.name
        got = confere(p)
        os.remove(p)
        marca = "ok" if got == esperado else "FALHOU"
        if got != esperado:
            ok = False
        print("     auto-teste [%s]: esperado %d, obtido %d -> %s"
              % (nome, esperado, got, marca))
    return 0 if ok else 1


def main():
    print("vias_tented v%s" % VERSAO)
    if "--auto-teste" in sys.argv:
        return _auto_teste()
    alvos = [a for a in sys.argv[1:] if not a.startswith("-")]
    if not alvos:
        raise SystemExit("USO: vias_tented.py <ficheiro.kicad_pcb> [...]")
    aberto = sum(confere(a) for a in alvos)
    if aberto:
        raise SystemExit("*** %d via(s) destapada(s): abertura de mascara sobre "
                         "cobre. A JLC marca isto (mask opening exposing trace) e o "
                         "cobre exposto oxida. Tapa as vias (setup: tenting front/back "
                         "yes) ou justifica cada abertura." % aberto)
    return 0


if __name__ == "__main__":
    sys.exit(main())
