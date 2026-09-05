#!/usr/bin/env python3
"""gera_simbolos v0.2.0 — os simbolos que faltavam para a folha do soquete.

Emite, para juntar a `cotuca.kicad_sym`:
  NETTIE2   a ponte AGND<->GND. Duas redes no ERC, uma ligacao de cobre.
  DPDT      comutador 2 polos 2 posicoes, 6 pinos
  CONNn_L   conectores genericos de n pinos, pinos a esquerda

⚠️ O NETTIE2 nao e um resistor de 0 R nem um fio. E um componente de DOIS
pinos cujo footprint tem os dois pads UNIDOS em cobre: o esquema ve duas
redes, a placa tem uma ligacao, num sitio escolhido. Sem ele o AGND flutua;
com um fio no lugar dele, as duas redes fundem-se no ERC e perde-se a
capacidade de detectar uma segunda ponte. Ver MAPA-DE-TERRA.md §2.
"""
import sys

VERSAO = "0.2.0"


def _pino(n, nome, x, y, ang, tipo="passive", ln=5.08):
    return (f'\t\t\t\t(pin {tipo} line\n\t\t\t\t\t(at {x} {y} {ang})\n'
            f'\t\t\t\t\t(length {ln})\n'
            f'\t\t\t\t\t(name "{nome}"\n\t\t\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t\t\t)\n'
            f'\t\t\t\t\t(number "{n}"\n\t\t\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t\t\t)\n'
            f'\t\t\t\t)\n')


def _cab(nome, ref, valor, ry, vy, extra=""):
    return (f'(symbol "cotuca:{nome}"\n'
            f'\t\t\t(pin_names\n\t\t\t\t(offset 1.016)\n\t\t\t)\n{extra}'
            f'\t\t\t(exclude_from_sim no)\n\t\t\t(in_bom yes)\n\t\t\t(on_board yes)\n'
            f'\t\t\t(in_pos_files yes)\n\t\t\t(duplicate_pin_numbers_are_jumpers no)\n'
            f'\t\t\t(property "Reference" "{ref}"\n\t\t\t\t(at 0 {ry} 0)\n'
            f'\t\t\t\t(show_name no) (do_not_autoplace no)\n\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t)\n'
            f'\t\t\t(property "Value" "{valor}"\n\t\t\t\t(at 0 {vy} 0)\n'
            f'\t\t\t\t(show_name no) (do_not_autoplace no)\n\t\t\t\t(effects (font (size 1 1)))\n\t\t\t)\n'
            + "".join(f'\t\t\t(property "{p}" ""\n\t\t\t\t(at 0 0 0)\n'
                      f'\t\t\t\t(show_name no) (do_not_autoplace no) (hide yes)\n'
                      f'\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t)\n'
                      for p in ("Footprint", "Datasheet", "Description")))


def nettie2():
    corpo = ('\t\t\t(symbol "NETTIE2_0_1"\n'
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -2.54 1.27)\n\t\t\t\t\t(end 2.54 -1.27)\n'
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n\t\t\t\t\t(fill (type outline))\n\t\t\t\t)\n\t\t\t)\n')
    pins = ('\t\t\t(symbol "NETTIE2_1_1"\n'
            + _pino(1, "A", -7.62, 0, 0) + _pino(2, "B", 7.62, 0, 180) + '\t\t\t)\n')
    return _cab("NETTIE2", "NT", "NetTie-2  AGND<->GND", 3.81, -3.81) + corpo + pins + '\t\t)'


def dpdt():
    corpo = ('\t\t\t(symbol "DPDT_0_1"\n'
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -6.35 8.89)\n\t\t\t\t\t(end 6.35 -8.89)\n'
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n\t\t\t\t\t(fill (type background))\n\t\t\t\t)\n\t\t\t)\n')
    # 1/4 comuns · 2/3 e 5/6 as duas posicoes
    p = ['\t\t\t(symbol "DPDT_1_1"\n']
    p.append(_pino(1, "COM_A", -11.43, 5.08, 0))
    p.append(_pino(2, "A1", 11.43, 7.62, 180))
    p.append(_pino(3, "A2", 11.43, 2.54, 180))
    p.append(_pino(4, "COM_B", -11.43, -5.08, 0))
    p.append(_pino(5, "B1", 11.43, -2.54, 180))
    p.append(_pino(6, "B2", 11.43, -7.62, 180))
    p.append('\t\t\t)\n')
    return _cab("DPDT", "SW", "DPDT tipo 2", 11.43, -11.43) + corpo + "".join(p) + '\t\t)'


def conn(n):
    h = max(2.54 * n, 5.08)
    corpo = ('\t\t\t(symbol "CONN%d_L_0_1"\n' % n +
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -2.54 %g)\n\t\t\t\t\t(end 2.54 %g)\n'
             % (h / 2 + 1.27, -h / 2 - 1.27) +
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n\t\t\t\t\t(fill (type background))\n\t\t\t\t)\n\t\t\t)\n')
    p = ['\t\t\t(symbol "CONN%d_L_1_1"\n' % n]
    for i in range(n):
        p.append(_pino(i + 1, "%d" % (i + 1), -7.62, h / 2 - i * 2.54, 0))
    p.append('\t\t\t)\n')
    return _cab("CONN%d_L" % n, "J", "conector %dp" % n, h / 2 + 3.81, -h / 2 - 3.81) + corpo + "".join(p) + '\t\t)'


def idc26():
    """IDC 2x13 macho. Numeracao em serpentina: impares a esquerda de cima
    para baixo, pares a direita — que e como um IDC e numerado de facto."""
    h = 13 * 2.54
    corpo = ('\t\t\t(symbol "IDC26_0_1"\n'
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -7.62 %g)\n\t\t\t\t\t(end 7.62 %g)\n'
             % (h / 2 + 1.27, -h / 2 - 1.27) +
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n'
             '\t\t\t\t\t(fill (type background))\n\t\t\t\t)\n\t\t\t)\n')
    p = ['\t\t\t(symbol "IDC26_1_1"\n']
    for i in range(13):
        y = h / 2 - 1.27 - i * 2.54
        p.append(_pino(2 * i + 1, "%d" % (2 * i + 1), -12.7, y, 0))
        p.append(_pino(2 * i + 2, "%d" % (2 * i + 2), 12.7, y, 180))
    p.append('\t\t\t)\n')
    return _cab("IDC26", "J", "IDC 2x13 macho", h / 2 + 3.81, -h / 2 - 3.81) + corpo + "".join(p) + '\t\t)'


if __name__ == "__main__":
    print("gera_simbolos v%s" % VERSAO, file=sys.stderr)
    for s in (nettie2(), dpdt(), conn(4), conn(5), idc26()):
        print(s)
