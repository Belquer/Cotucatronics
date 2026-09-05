#!/usr/bin/env python3
"""gera_daisy_sym v0.1.0 — o simbolo da Daisy Seed, GERADO da folha de dados.

    ./gera_daisy_sym.py > /tmp/daisy.txt

⚠️ NAO ESCREVER ESTE SIMBOLO A MAO. Sao 40 pinos, e a numeracao fisica do
header nao se deduz do `Dnn` — `D0` e o pino 1, mas `D15` e o 22 e `D17` e o
24, porque os seis pinos analogicos (16..21) se metem pelo meio.

A fonte e o `Daisy_Seed_Rev4_Pinout.csv` do fabricante, que vem dentro da
libDaisy (`doc/`). Regra do dB, 04/09: "a folha de dados da Seed para o
`Dnn` -> header, e nada de memoria".

⚠️ E A REGRA VALE COM MAIS FORCA PARA O QUE SE JULGA SABER. No mesmo dia
afirmou-se, com confianca e em dois documentos, que a Seed junta AGND e DGND
por dentro. Nao junta — a doc da libDaisy manda liga-los FORA, e este CSV
mostra-os em dois pinos distintos, 20 e 40. Ver battle_scars 15bw.
"""
import csv, io, os, sys

VERSAO = "0.1.0"
CSV = os.path.expanduser("~/dev/libDaisy/doc/Daisy_Seed_Rev4_Pinout.csv")
W, H = 25.4, 53.34
POT = {'AGND': 'power_in', 'GND': 'power_in', 'VIN': 'power_in',
       '3v3A': 'power_out', '3v3D': 'power_out'}


def ler():
    rows = list(csv.reader(io.open(CSV, encoding='utf-8')))
    h = rows[0]; isch, idd = h.index('SCH name'), h.index('Arduino D#')
    pin = {}
    for r in rows[1:]:
        if len(r) > 3 and r[0].strip().isdigit():
            n = int(r[0]); sch = r[isch].strip(); d = r[idd].strip()
            pin[n] = d if d and d != 'NC' else sch.replace(' ', '_')
    if len(pin) != 40:
        raise SystemExit("esperava 40 pinos, li %d — o CSV mudou de forma" % len(pin))
    return pin


def gerar():
    pin = ler()
    def p(n, nome, x, y, ang):
        t = POT.get(nome, 'bidirectional')
        return (f'\t\t\t\t(pin {t} line\n\t\t\t\t\t(at {x} {y} {ang})\n'
                f'\t\t\t\t\t(length 5.08)\n'
                f'\t\t\t\t\t(name "{nome}"\n\t\t\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t\t\t)\n'
                f'\t\t\t\t\t(number "{n}"\n\t\t\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t\t\t)\n'
                f'\t\t\t\t)\n')
    ps = []
    # ⚠️ esquerda 1..20 e direita 21..40, os DOIS de cima para baixo. Assim o
    # pino 20 (AGND) e o 40 (GND) caem frente a frente em baixo, e o net-tie
    # entre eles fica curto tambem no papel.
    for i in range(20):
        ps.append(p(i + 1, pin[i + 1], -W - 5.08, 25.4 - i * 2.54, 0))
    for i in range(20):
        ps.append(p(i + 21, pin[i + 21], W + 5.08, 25.4 - i * 2.54, 180))
    return (f'(symbol "cotuca:DAISY_SEED"\n'
            f'\t\t\t(pin_names\n\t\t\t\t(offset 1.016)\n\t\t\t)\n'
            f'\t\t\t(exclude_from_sim no)\n\t\t\t(in_bom yes)\n\t\t\t(on_board yes)\n'
            f'\t\t\t(in_pos_files yes)\n\t\t\t(duplicate_pin_numbers_are_jumpers no)\n'
            f'\t\t\t(property "Reference" "U"\n\t\t\t\t(at 0 30.48 0)\n'
            f'\t\t\t\t(show_name no) (do_not_autoplace no)\n\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t)\n'
            f'\t\t\t(property "Value" "Daisy Seed rev4"\n\t\t\t\t(at 0 -30.48 0)\n'
            f'\t\t\t\t(show_name no) (do_not_autoplace no)\n\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t)\n'
            f'\t\t\t(property "Footprint" ""\n\t\t\t\t(at 0 0 0)\n'
            f'\t\t\t\t(show_name no) (do_not_autoplace no) (hide yes)\n\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t)\n'
            f'\t\t\t(property "Datasheet" "Daisy_Seed_Rev4_Pinout.csv"\n\t\t\t\t(at 0 0 0)\n'
            f'\t\t\t\t(show_name no) (do_not_autoplace no) (hide yes)\n\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t)\n'
            f'\t\t\t(property "Description" "SOM em soquete 2x20. Simbolo GERADO do CSV do fabricante."\n\t\t\t\t(at 0 0 0)\n'
            f'\t\t\t\t(show_name no) (do_not_autoplace no) (hide yes)\n\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t)\n'
            f'\t\t\t(symbol "DAISY_SEED_0_1"\n\t\t\t\t(rectangle\n'
            f'\t\t\t\t\t(start {-W} {H/2})\n\t\t\t\t\t(end {W} {-H/2})\n'
            f'\t\t\t\t\t(stroke (width 0.254) (type default))\n\t\t\t\t\t(fill (type background))\n\t\t\t\t)\n\t\t\t)\n'
            f'\t\t\t(symbol "DAISY_SEED_1_1"\n{"".join(ps)}\t\t\t)\n\t\t)')


if __name__ == '__main__':
    print("gera_daisy_sym v%s" % VERSAO, file=sys.stderr)
    print(gerar())
