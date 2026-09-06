#!/usr/bin/env python3
"""gera_footprints v0.1.0 — footprints que nao existem em biblioteca nenhuma.

Por agora um so: o soquete do ESP32-C3 SuperMini.

⚠️ NAO EXISTE FOOTPRINT OFICIAL deste modulo. A biblioteca da Espressif tem o
`ESP32-C3-MINI-1`, que e o MODULO CRU e outra peca — confundi-los custa a placa.
Os da comunidade discordam entre si porque cada pino tem DUAS filas de furos: a
interior a 15,24 (a do soquete) e a exterior castelada, a ~18.

⚠️ UM DIAGRAMA DE PINAGEM SEM A FACE DECLARADA E MEIO FACTO. Correram TRES
leituras contraditorias antes desta, e a causa nao era a peca variar: o diagrama
"PIN Definitions" do vendedor mostra o lado de BAIXO e nao o diz em lado nenhum.
Lido como vista de cima da o espelho — e o espelho poe o 5V onde estava o GND.

O que vale, confirmado pelo dB com a peca na mao e batendo com a pesquisa:

    VISTA DE CIMA (lado dos componentes), USB-C para CIMA:
      coluna ESQUERDA, de cima p/ baixo:  5  6  7  8  9 10 20 21
      coluna DIREITA,  de cima p/ baixo: 5V  G 3V3  4  3  2  1  0

E os pads levam os NOMES DA SERIGRAFIA, nao numeros: quem montar compara o silk
do modulo com o nome do pad e nao tem nada que interpretar. Se um dia chegar uma
remessa espelhada, ve-se ao pousar o modulo, nao ao ligar.

Medidas: 2x8, passo 2,54, filas a 15,24 (confirmado por tres fontes
independentes e pela ficha do vendedor). Contorno 22,5 x 18. O USB-C sai 1 mm
da borda — MEDIDO pelo dB na peca em mao, 06/09.

USO:  gera_footprints.py
"""
import io, os, sys, uuid

VERSAO = "0.2.0"
AQUI = os.path.dirname(os.path.abspath(__file__))
LIB = os.path.join(AQUI, "cotuca.pretty")

PASSO, FILAS = 2.54, 15.24
W, H = 18.0, 22.5      # x, y — o modulo esta 'de pe', USB em -y
USB_SAI = 1.0          # medido pelo dB
FURO, PAD = 1.0, 1.7

# ⚠️ VISTA DE CIMA, USB-C em -y (para cima). As colunas sao em x; a ordem de
# cada lista e de -y para +y, ou seja de cima para baixo no ecra.
ESQ  = ["IO5", "IO6", "IO7", "IO8", "IO9", "IO10", "IO20", "IO21"]
DIR  = ["5V", "GND", "3V3", "IO4", "IO3", "IO2", "IO1", "IO0"]
SILK  = {"IO21": "21", "IO20": "20", "IO10": "10", "IO9": "9", "IO8": "8",
         "IO7": "7", "IO6": "6", "IO5": "5", "IO0": "0", "IO1": "1",
         "IO2": "2", "IO3": "3", "IO4": "4", "3V3": "3.3", "GND": "G", "5V": "5V"}


def u():
    return str(uuid.uuid4())


def main():
    print("gera_footprints v%s" % VERSAO, file=sys.stderr)
    if len(ESQ) != 8 or len(DIR) != 8:
        raise SystemExit("*** 2x8 e 16 pinos; tens %d+%d" % (len(ESQ), len(DIR)))
    y0 = -PASSO * (len(ESQ) - 1) / 2
    L = ['(footprint "ESP32C3_SUPERMINI_SOQUETE"',
         '\t(version 20240108)', '\t(generator "gera_footprints")',
         '\t(layer "F.Cu")',
         '\t(descr "ESP32-C3 SuperMini em soquete 2x8/15,24. Pads com os NOMES '
         'DA SERIGRAFIA do modulo — a pinagem desta placa varia entre '
         'fabricantes, e o nome nao se interpreta. USB-C em +x, sai 1 mm.")',
         '\t(tags "ESP32 C3 SuperMini soquete modulo")',
         '\t(attr through_hole)']
    for prop, val, y in (("Reference", "U*", -H / 2 - 1.5), ("Value", "ESP32C3_SUPERMINI", H / 2 + 1.5)):
        L.append('\t(property "%s" "%s"\n\t\t(at 0 %g 0)\n\t\t(layer "%s")\n'
                 '\t\t(uuid "%s")\n\t\t(effects (font (size 1 1) (thickness 0.15)))\n\t)'
                 % (prop, val, y, "F.SilkS" if prop == "Reference" else "F.Fab", u()))
    for coluna, x in ((ESQ, -FILAS / 2), (DIR, FILAS / 2)):
        for i, nome in enumerate(coluna):
            y = y0 + i * PASSO
            forma = "rect" if nome == "5V" else "circle"
            L.append('\t(pad "%s" thru_hole %s\n\t\t(at %g %g)\n\t\t(size %g %g)\n'
                     '\t\t(drill %g)\n\t\t(layers "*.Cu" "*.Mask")\n\t\t(uuid "%s")\n\t)'
                     % (nome, forma, x, y, PAD, PAD, FURO, u()))
            L.append('\t(fp_text user "%s"\n\t\t(at %g %g 0)\n\t\t(layer "F.SilkS")\n'
                     '\t\t(uuid "%s")\n\t\t(effects (font (size 0.8 0.8) (thickness 0.12)))\n\t)'
                     % (SILK[nome], x + (2.2 if x < 0 else -2.2), y, u()))
    # contorno do modulo (F.Fab), serigrafia e courtyard
    for camada, larg, esp in (("F.Fab", 0.1, 0.1), ("F.SilkS", 0.12, 0.12)):
        L.append('\t(fp_rect\n\t\t(start %g %g)\n\t\t(end %g %g)\n\t\t(stroke (width %g) '
                 '(type default))\n\t\t(fill no)\n\t\t(layer "%s")\n\t\t(uuid "%s")\n\t)'
                 % (-W / 2, -H / 2, W / 2, H / 2, larg, camada, u()))
    L.append('\t(fp_rect\n\t\t(start %g %g)\n\t\t(end %g %g)\n\t\t(stroke (width 0.05) '
             '(type default))\n\t\t(fill no)\n\t\t(layer "F.CrtYd")\n\t\t(uuid "%s")\n\t)'
             % (-W / 2 - 0.25, -H / 2 - USB_SAI - 0.25, W / 2 + 0.25, H / 2 + 0.25, u()))
    # o USB-C, para se ver de que lado esta
    L.append('\t(fp_rect\n\t\t(start %g %g)\n\t\t(end %g %g)\n\t\t(stroke (width 0.12) '
             '(type default))\n\t\t(fill no)\n\t\t(layer "F.SilkS")\n\t\t(uuid "%s")\n\t)'
             % (-4.5, -H / 2 - USB_SAI, 4.5, -H / 2, u()))
    L.append('\t(fp_text user "USB-C"\n\t\t(at 0 %g 0)\n\t\t(layer "F.SilkS")\n'
             '\t\t(uuid "%s")\n\t\t(effects (font (size 0.8 0.8) (thickness 0.12)))\n\t)'
             % (-H / 2 + 2.0, u()))
    # ⚠️ a antena ceramica fica no extremo OPOSTO ao USB e quer cobre longe
    L.append('\t(fp_text user "ANTENA - sem cobre"\n\t\t(at 0 %g 0)\n\t\t(layer "F.Fab")\n'
             '\t\t(uuid "%s")\n\t\t(effects (font (size 0.8 0.8) (thickness 0.12)))\n\t)'
             % (H / 2 - 2.0, u()))
    L.append(')')
    os.makedirs(LIB, exist_ok=True)
    p = os.path.join(LIB, "ESP32C3_SUPERMINI_SOQUETE.kicad_mod")
    io.open(p, "w", encoding="utf-8").write("\n".join(L) + "\n")
    print("  escrito %s" % p, file=sys.stderr)
    print("  16 pads: %s" % " ".join(ESQ + DIR), file=sys.stderr)
    print("  vista de CIMA, USB-C em -y. 5V em (%g, %g): coluna DIREITA, "
          "primeiro a contar do USB." % (FILAS / 2, y0), file=sys.stderr)


if __name__ == "__main__":
    main()
