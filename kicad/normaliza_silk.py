#!/usr/bin/env python3
"""normaliza_silk v0.1.0 — a largura da serigrafia, levada ao minimo da JLC.

POR QUE EXISTE.

A JLC pede serigrafia com traco >= 0,15 mm e altura de texto >= 1 mm (tabela de
capacidades em `.claude/skills/aipcb/references/fab-automation.md`). O DFM da JLC
marcou-a a LARANJA quando o traco fica abaixo — imprimivel, mas fora do preferido,
e a JLCONE conta-a. O nosso portao nao a via porque:

  - os CONTORNOS de footprint da biblioteca do KiCad nascem a 0,12 mm, e
  - NENHUMA regra de DRC do KiCad mede a largura de um GRAFICO de serigrafia
    (o `text_thickness` so ve TEXTO; a linha de um `fp_line` fica de fora).

Logo a unica garantia de largura para os graficos e a FONTE: uma passada que os
sobe a >= 0,15 mm. E como a biblioteca volta a trazer 0,12 a cada re-sync de
footprints, isto e um passo de acabamento (Estagio 6), nao uma edicao unica.

  --check   read-only: lista o que esta abaixo e sai != 0 (o portao). NAO grava.
  --write   sobe cada serigrafia < 0,15 mm para 0,15 mm e grava.

⚠️ So mexe em LARGURA de serigrafia (F/B.Silkscreen): traco de `fp_line`/`fp_rect`/
`fp_circle`, espessura de texto e dos campos Reference/Value. Nao toca em cobre,
pads, pistas, zonas nem posicoes — o numero de tudo o resto fica IGUAL.

⚠️ Corre com o Python do BUNDLE do KiCad (tem `pcbnew`), como o roteia_fr.py.
"""
import sys
import pcbnew
pcbnew.SwigPyIterator.next = pcbnew.SwigPyIterator.__next__

MIN = 150000            # 0,15 mm em nm — minimo de traco de serigrafia da JLC
SILK = {pcbnew.F_SilkS, pcbnew.B_SilkS}


def itens(b):
    """(rotulo, get_largura, set_largura) de cada serigrafia com largura."""
    out = []
    for fp in b.GetFootprints():
        ref = fp.GetReference()
        for fld, nome in ((fp.Reference(), "Reference"), (fp.Value(), "Value")):
            if fld.GetLayer() in SILK:
                out.append(("%s de %s" % (nome, ref),
                            fld.GetTextThickness, fld.SetTextThickness))
        for it in fp.GraphicalItems():
            if it.GetLayer() not in SILK:
                continue
            if it.GetClass() == "PCB_TEXT":
                out.append(("Texto de %s" % ref, it.GetTextThickness, it.SetTextThickness))
            else:
                out.append(("%s de %s" % (it.GetShapeStr(), ref), it.GetWidth, it.SetWidth))
    for it in b.GetDrawings():
        if it.GetLayer() not in SILK:
            continue
        if it.GetClass() == "PCB_TEXT":
            out.append(("Texto de placa '%s'" % it.GetText(),
                        it.GetTextThickness, it.SetTextThickness))
        else:
            out.append(("%s de placa" % it.GetShapeStr(), it.GetWidth, it.SetWidth))
    return out


def main():
    if len(sys.argv) < 2 or ("--check" not in sys.argv and "--write" not in sys.argv):
        print(__doc__)
        return 2
    path = sys.argv[1]
    b = pcbnew.LoadBoard(path)
    finos = [(rot, get) for rot, get, _set in itens(b) if 0 < get() < MIN]
    if "--check" in sys.argv:
        if finos:
            print("normaliza_silk: %d serigrafia(s) < 0,15 mm (JLC laranja):" % len(finos))
            vistos = {}
            for rot, get in finos:
                vistos[round(get() / 1e6, 4)] = vistos.get(round(get() / 1e6, 4), 0) + 1
            for w, n in sorted(vistos.items()):
                print("   %.4f mm : %d" % (w, n))
            print("   corre `normaliza_silk.py %s --write` para as subir." % path)
            return 1
        print("normaliza_silk: serigrafia toda >= 0,15 mm ✓")
        return 0
    # --write
    n = 0
    for rot, get, setw in itens(b):
        if 0 < get() < MIN:
            setw(MIN)
            n += 1
    pcbnew.SaveBoard(path, b)
    print("normaliza_silk: %d serigrafia(s) subida(s) a 0,15 mm; gravado %s" % (n, path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
