#!/usr/bin/env python3
"""gera_daisy_sym v0.3.0 — o simbolo da Daisy Seed, GERADO da folha de dados.

    ./gera_daisy_sym.py > /tmp/daisy.txt

⚠️ NAO ESCREVER ESTE SIMBOLO A MAO. Sao 40 pinos, e a numeracao fisica do
header nao se deduz do `Dnn` — `D0` e o pino 1, mas `D15` e o 22 e `D17` e o
24, porque os seis pinos analogicos (16..21) se metem pelo meio.

A fonte e o `Seed3_pinout.csv` do fabricante, guardado ao lado deste ficheiro.

⚠️ A 2026-09-05 a placa passou do **Daisy Seed rev4** para o **Seed3**. A pinagem
foi CONFERIDA pino a pino entre os dois CSV: **o nome STM32 bate nos 40 pinos,
sem uma excepcao** — e a doc do fabricante diz, textualmente, *"maintaining
pin-to-pin compatibility with the original Seed pinout and footprint"*. Por
isso o simbolo e o footprint NAO mudam. Mudou o codec (TI TAC5242, 32 bit /
192 kHz) e o conector, que agora e USB-C na propria placa.

⚠️ Os dois CSV tem COLUNAS COM SENTIDOS DIFERENTES: onde o do rev4 poe a
funcao alternativa (`SDMMC1_D3`), o do Seed3 poe `GPIO`. Comparar coluna a
coluna da 35 "diferencas" que nao existem. **Compara-se o nome STM32.**
A antiga fonte era o `Daisy_Seed_Rev4_Pinout.csv`, que vinha dentro da
libDaisy (`doc/`). Regra do dB, 2026-09-04: "a folha de dados da Seed para o
`Dnn` -> header, e nada de memoria".

⚠️ E A REGRA VALE COM MAIS FORCA PARA O QUE SE JULGA SABER. No mesmo dia
afirmou-se, com confianca e em dois documentos, que a Seed junta AGND e DGND
por dentro. Nao junta — a doc da libDaisy manda liga-los FORA, e este CSV
mostra-os em dois pinos distintos, 20 e 40. Ver battle_scars 15bw.
"""
import csv, io, os, re, sys

VERSAO = "0.3.0"


def num(v):
    """Todo numero que entra num ficheiro KiCad passa por aqui — ver
    gera_simbolos.py §num(). Sem isto sai `-30.479999999999997`, que o KiCad
    aceita e que nenhum leitor com `[\\d.]+` apanha."""
    return "%g" % (round(float(v), 4) + 0.0)
CSV = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Seed3_pinout.csv")
W, H = 25.4, 53.34
POT = {'AGND': 'power_in', 'GND': 'power_in', 'VIN': 'power_in',
       '3v3A': 'power_out', '3v3D': 'power_out'}


def ler():
    """{numero: nome do pino}, dos DOIS formatos de CSV do fabricante.

    ⚠️ Os CSV do rev4 e do Seed3 tem colunas com sentidos diferentes:

        rev4  : `SCH name` (AGND, AUDIO IN L...) e `Arduino D#` (D0, D15...)
        Seed3 : `DAISY PIN NAME*` (D0, "A0, D15"...) e `PRIMARY FUNCTION`
                (GPIO, AGND, AUDIO IN L...)

    Ler pelo INDICE parte. Ler pelo NOME do cabecalho e o que se faz — e foi
    o que fez o gerador rebentar alto quando a fonte mudou, em vez de emitir
    um simbolo errado em silencio. Esse falhanco e a funcionalidade."""
    rows = list(csv.reader(io.open(CSV, encoding='utf-8')))
    h = [c.strip() for c in rows[0]]
    if 'SCH name' in h:                       # formato rev4
        ifun, inome = h.index('SCH name'), h.index('Arduino D#')
    elif 'PRIMARY FUNCTION' in h:             # formato Seed3
        ifun = h.index('PRIMARY FUNCTION')
        inome = h.index([c for c in h if c.startswith('DAISY PIN NAME')][0])
    else:
        raise SystemExit("*** CSV com cabecalho desconhecido: %s — nao invento "
                         "colunas" % h[:4])
    pin = {}
    for r in rows[1:]:
        if len(r) > max(ifun, inome) and r[0].strip().isdigit():
            n = int(r[0])
            fun = r[ifun].strip()
            nome = r[inome].strip()
            # "A0, D15" -> D15 (o nome digital e o que o resto do projecto usa)
            if ',' in nome:
                nome = [x.strip() for x in nome.split(',') if x.strip().startswith('D')][0]
            # no Seed3 a funcao de um GPIO e a palavra "GPIO"; o nome util e o Dnn
            if fun.upper() == 'GPIO':
                fun = ''
            # ⚠️ O CSV do Seed3 poe `PG3` na coluna do NOME do pino 40, e a
            # coluna da FUNCAO diz `GND` — e o desenho de pinagem do proprio
            # fabricante diz `DGND`. A folha contradiz o desenho. Um nome que
            # e designacao de porta do STM32 (`PG3`, `PA0`...) nao e nome de
            # pino da Daisy: nesses casos manda a funcao, e DIZ-SE que se
            # descartou. Escolher em silencio entre duas fontes que discordam
            # e o mesmo que inventar.
            if re.match(r'^P[A-K]\d{1,2}$', nome):
                print("   ⚠️ pino %d: o CSV diz nome=%r e funcao=%r — nome "
                      "descartado (e designacao de porta do MCU, nao nome de "
                      "pino). Fica %r." % (n, nome, fun, fun), file=sys.stderr)
                nome = ''
            usar = nome if nome and nome != 'NC' else fun
            pin[n] = usar.replace(' ', '_').replace('+3V3', '3v3')
    if len(pin) != 40:
        raise SystemExit("esperava 40 pinos, li %d — o CSV mudou de forma" % len(pin))
    return pin


def gerar():
    pin = ler()
    def p(n, nome, x, y, ang):
        t = POT.get(nome, 'bidirectional')
        return (f'\t\t\t\t(pin {t} line\n\t\t\t\t\t(at {num(x)} {num(y)} {num(ang)})\n'
                f'\t\t\t\t\t(length 5.08)\n'
                f'\t\t\t\t\t(name "{nome}"\n\t\t\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t\t\t)\n'
                f'\t\t\t\t\t(number "{n}"\n\t\t\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t\t\t)\n'
                f'\t\t\t\t)\n')
    ps = []
    # ⚠️ esquerda 1..20 e direita 21..40, os DOIS de cima para baixo. Assim o
    # pino 20 (AGND) e o 40 (GND) caem frente a frente em baixo, o que torna
    # o net-tie da ponte legivel no ESQUEMA.
    #
    # ⚠️ ISTO E CONVENIENCIA DE DESENHO, NAO A PLACA. No modulo real eles
    # estao na DIAGONAL — medido pelo dB em 2026-09-04: o 20 no canto inferior
    # esquerdo, colado ao USB-C, e o 40 no superior direito, no extremo
    # oposto. O condutor da ponte atravessa o modulo, e isso e restricao de
    # LAYOUT que o esquema nao mostra. Ver MAPA-DE-TERRA.md §2.
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
            f'\t\t\t(property "Value" "Daisy Seed3"\n\t\t\t\t(at 0 -30.48 0)\n'
            f'\t\t\t\t(show_name no) (do_not_autoplace no)\n\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t)\n'
            f'\t\t\t(property "Footprint" ""\n\t\t\t\t(at 0 0 0)\n'
            f'\t\t\t\t(show_name no) (do_not_autoplace no) (hide yes)\n\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t)\n'
            f'\t\t\t(property "Datasheet" "Seed3_pinout.csv"\n\t\t\t\t(at 0 0 0)\n'
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
