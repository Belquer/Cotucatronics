#!/usr/bin/env python3
"""gera_simbolos v0.7.0 — os simbolos que faltavam para a folha do soquete.

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

VERSAO = "0.7.0"


def num(v):
    """Todo numero que entra num ficheiro KiCad passa por aqui.

    `h/2 - 1.27 - 6*2.54` da 1.7763568394002505e-15 em vez de 0, e o KiCad
    aceita — mas nenhuma ferramenta que le com `[\\d.]+` apanha essa linha, e
    um leitor que perde linhas em silencio aprova contratos que nao conferiu.
    Foi assim que `contrato_idc.py` leu 24 de 26 vias e disse que batiam.
    Arredondar a 4 casas mata a notacao cientifica na origem: 0,1 um decimo
    de micron, tres ordens de grandeza abaixo de qualquer tolerancia de
    fabrico, e a grelha do projecto e 1,27 mm."""
    v = round(float(v), 4)
    return "%g" % (v + 0.0)  # o +0.0 troca -0.0 por 0


def _pino(n, nome, x, y, ang, tipo="passive", ln=5.08):
    return (f'\t\t\t\t(pin {tipo} line\n\t\t\t\t\t(at {num(x)} {num(y)} {num(ang)})\n'
            f'\t\t\t\t\t(length {num(ln)})\n'
            f'\t\t\t\t\t(name "{nome}"\n\t\t\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t\t\t)\n'
            f'\t\t\t\t\t(number "{n}"\n\t\t\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t\t\t)\n'
            f'\t\t\t\t)\n')


def _cab(nome, ref, valor, ry, vy, extra=""):
    return (f'(symbol "cotuca:{nome}"\n'
            f'\t\t\t(pin_names\n\t\t\t\t(offset 1.016)\n\t\t\t)\n{extra}'
            f'\t\t\t(exclude_from_sim no)\n\t\t\t(in_bom yes)\n\t\t\t(on_board yes)\n'
            f'\t\t\t(in_pos_files yes)\n\t\t\t(duplicate_pin_numbers_are_jumpers no)\n'
            f'\t\t\t(property "Reference" "{ref}"\n\t\t\t\t(at 0 {num(ry)} 0)\n'
            f'\t\t\t\t(show_name no) (do_not_autoplace no)\n\t\t\t\t(effects (font (size 1.27 1.27)))\n\t\t\t)\n'
            f'\t\t\t(property "Value" "{valor}"\n\t\t\t\t(at 0 {num(vy)} 0)\n'
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
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -2.54 %s)\n\t\t\t\t\t(end 2.54 %s)\n'
             % (num(h / 2 + 1.27), num(-h / 2 - 1.27)) +
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
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -7.62 %s)\n\t\t\t\t\t(end 7.62 %s)\n'
             % (num(h / 2 + 1.27), num(-h / 2 - 1.27)) +
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n'
             '\t\t\t\t\t(fill (type background))\n\t\t\t\t)\n\t\t\t)\n')
    p = ['\t\t\t(symbol "IDC26_1_1"\n']
    for i in range(13):
        y = h / 2 - 1.27 - i * 2.54
        p.append(_pino(2 * i + 1, "%d" % (2 * i + 1), -12.7, y, 0))
        p.append(_pino(2 * i + 2, "%d" % (2 * i + 2), 12.7, y, 180))
    p.append('\t\t\t)\n')
    return _cab("IDC26", "J", "IDC 2x13 macho", h / 2 + 3.81, -h / 2 - 3.81) + corpo + "".join(p) + '\t\t)'


def pot3():
    """Potenciometro: 1 e 3 as pontas, 2 o cursor. O RV_H tem so DOIS pinos —
    e trimmer usado como resistencia variavel no pre-amp, e nao serve aqui."""
    corpo = ('\t\t\t(symbol "POT3_0_1"\n'
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -5.08 2.54)\n\t\t\t\t\t(end 5.08 -2.54)\n'
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n'
             '\t\t\t\t\t(fill (type none))\n\t\t\t\t)\n\t\t\t)\n')
    p = ['\t\t\t(symbol "POT3_1_1"\n',
         _pino(1, "1", -10.16, 0, 0), _pino(2, "CURSOR", 0, 7.62, 270),
         _pino(3, "3", 10.16, 0, 180), '\t\t\t)\n']
    return _cab("POT3", "RV", "10k lin", 10.16, -5.08) + corpo + "".join(p) + '\t\t)'


def ec11():
    """EC11: A, C (comum das fases), B, e os dois do push."""
    corpo = ('\t\t\t(symbol "EC11_0_1"\n'
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -6.35 7.62)\n\t\t\t\t\t(end 6.35 -7.62)\n'
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n'
             '\t\t\t\t\t(fill (type background))\n\t\t\t\t)\n\t\t\t)\n')
    p = ['\t\t\t(symbol "EC11_1_1"\n']
    for n, nome, y in ((1, "A", 5.08), (2, "C", 2.54), (3, "B", 0.0)):
        p.append(_pino(n, nome, -11.43, y, 0))
    for n, nome, y in ((4, "PUSH1", -2.54), (5, "PUSH2", -5.08)):
        p.append(_pino(n, nome, -11.43, y, 0))
    p.append('\t\t\t)\n')
    return _cab("EC11", "SW", "EC11 encoder", 10.16, -10.16) + corpo + "".join(p) + '\t\t)'


def mosfet_n():
    """AO3400A. G, D, S — e o pull-down do gate NAO esta aqui: e componente
    a parte, obrigatorio, e o desenho tem de o mostrar."""
    corpo = ('\t\t\t(symbol "MOSFET_N_0_1"\n'
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -2.54 3.81)\n\t\t\t\t\t(end 2.54 -3.81)\n'
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n'
             '\t\t\t\t\t(fill (type none))\n\t\t\t\t)\n\t\t\t)\n')
    p = ['\t\t\t(symbol "MOSFET_N_1_1"\n',
         _pino(1, "G", -7.62, 0, 0), _pino(2, "D", 0, 8.89, 270),
         _pino(3, "S", 0, -8.89, 90), '\t\t\t)\n']
    return _cab("MOSFET_N", "Q", "AO3400A", 11.43, -11.43) + corpo + "".join(p) + '\t\t)'


def led():
    corpo = ('\t\t\t(symbol "LED_0_1"\n'
             '\t\t\t\t(polyline\n\t\t\t\t\t(pts\n\t\t\t\t\t\t(xy -1.27 2.54) (xy -1.27 -2.54)\n\t\t\t\t\t)\n'
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n\t\t\t\t\t(fill (type none))\n\t\t\t\t)\n'
             '\t\t\t\t(polyline\n\t\t\t\t\t(pts\n\t\t\t\t\t\t(xy 1.27 2.54) (xy 1.27 -2.54) (xy -1.27 0) (xy 1.27 2.54)\n\t\t\t\t\t)\n'
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n\t\t\t\t\t(fill (type none))\n\t\t\t\t)\n\t\t\t)\n')
    p = ['\t\t\t(symbol "LED_1_1"\n',
         _pino(1, "A", -6.35, 0, 0), _pino(2, "K", 6.35, 0, 180), '\t\t\t)\n']
    return _cab("LED", "D", "LED", 5.08, -5.08) + corpo + "".join(p) + '\t\t)'


def esp32c3():
    """ESP32-C3 SuperMini em soquete, 2x8.

    ⚠️ OS NUMEROS DE PINO SAO OS NOMES DA SERIGRAFIA — "5V", "GND", "IO21" — e
    nao 1..16. O KiCad exige que o numero do pino bata no nome do pad, e o
    footprint (`kicad/gera_footprints.py`) usa os mesmos nomes de proposito:
    esta placa ja apareceu com tres pinagens contraditorias em duas horas
    (cicatriz 15ck) e a defesa e nao haver traducao entre o silk e o cobre.

    ⚠️ VISTA DE CIMA, USB-C para cima. A coluna da ESQUERDA no simbolo e a
    esquerda do modulo visto de cima. O diagrama do vendedor mostra o lado de
    BAIXO e nao o diz — foi o que gerou as tres leituras."""
    ESQ = ["IO5", "IO6", "IO7", "IO8", "IO9", "IO10", "IO20", "IO21"]
    DIR = ["5V", "GND", "3V3", "IO4", "IO3", "IO2", "IO1", "IO0"]
    ROTULO = {"IO20": "IO20/RX", "IO21": "IO21/TX",
              "IO8": "IO8 (LED)", "IO9": "IO9 (BOOT)"}
    h = 8 * 2.54
    corpo = ('\t\t\t(symbol "ESP32C3_0_1"\n'
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -10.16 %s)\n\t\t\t\t\t(end 10.16 %s)\n'
             % (num(h / 2 + 1.27), num(-h / 2 - 1.27)) +
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n'
             '\t\t\t\t\t(fill (type background))\n\t\t\t\t)\n\t\t\t)\n')
    p = ['\t\t\t(symbol "ESP32C3_1_1"\n']
    for i, (e, d) in enumerate(zip(ESQ, DIR)):
        y = h / 2 - 1.27 - i * 2.54
        p.append(_pino(e, ROTULO.get(e, e), -15.24, y, 0))
        tipo = "power_in" if d in ("5V", "GND", "3V3") else "passive"
        p.append(_pino(d, ROTULO.get(d, d), 15.24, y, 180, tipo=tipo))
    p.append('\t\t\t)\n')
    return (_cab("ESP32C3", "U", "ESP32-C3 SuperMini", h / 2 + 3.81, -h / 2 - 3.81)
            + corpo + "".join(p) + '\t\t)')


def opa1642():
    """OPA1642 — dual SOIC-8, TRES UNIDADES numa so referencia.

    ⚠️ SUBSTITUI `OPA` + `OPAPWR`, QUE ESTAVAM ERRADOS DE DUAS MANEIRAS.

    1. NUMERACAO TROCADA. No `OPA` o pino 1 estava na posicao do +IN e o pino
       3 na da saida. Num dual SOIC-8 e ao contrario: 1 e a SAIDA de A, 3 e o
       +IN de A. Numa placa fabricada a rede de entrada iria soldada ao pino
       de saida do chip. Nao aparecia no prototipo porque esse e ligado a mao
       e nunca passou pelo mapeamento de pinos do encapsulamento.

    2. UMA UNIDADE SO. O `OPA` era colocado DUAS vezes, como U1A e U1B, e as
       duas usavam os pinos 1/2/3 — o amplificador B ficava no encapsulamento
       do A. E, sendo referencias diferentes, davam TRES linhas de BOM e tres
       pegadas na placa para um unico chip.

    FONTE DA PINAGEM: `LM2904` da biblioteca oficial do KiCad
    (`Amplifier_Operational.kicad_sym`), o simbolo base de que `OPA1602`,
    `OPA1612AxD`, `OPA2134`, `LM4562` e `NE5532` todos herdam por `extends` —
    e portanto a pinagem padrao do dual SOIC-8.
        u1 amp A: 1 saida · 2 -IN · 3 +IN
        u2 amp B: 7 saida · 6 -IN · 5 +IN
        u3 alim.: 4 V- · 8 V+
    ⚠️ POR CONFIRMAR no datasheet do OPA1642AIDR antes de submeter a fabrica.
    Cinco duais independentes concordarem e evidencia forte, nao e a folha da
    peca que se vai comprar.

    A GEOMETRIA E A DO PROJECTO (pinos a ±10,16, nao a ±7,62 da oficial): os
    fios do `cotuca_preamp.kicad_sch` ja encostam nesses pontos, e mover o
    pino partiria ligacoes que estao certas. So os NUMEROS mudam."""
    tri = ('\t\t\t\t(polyline\n\t\t\t\t\t(pts\n'
           '\t\t\t\t\t\t(xy -7.62 7.62) (xy -7.62 -7.62) (xy 7.62 0) (xy -7.62 7.62)\n'
           '\t\t\t\t\t)\n\t\t\t\t\t(stroke (width 0.254) (type default))\n'
           '\t\t\t\t\t(fill (type none))\n\t\t\t\t)\n')
    ret = ('\t\t\t\t(rectangle\n\t\t\t\t\t(start -5.08 -5.08)\n\t\t\t\t\t(end 5.08 5.08)\n'
           '\t\t\t\t\t(stroke (width 0.254) (type default))\n'
           '\t\t\t\t\t(fill (type none))\n\t\t\t\t)\n')
    u1 = ('\t\t\t(symbol "OPA1642_1_1"\n' + tri
          + _pino(3, "+", -10.16, 2.54, 0, "input", 2.54)
          + _pino(2, "-", -10.16, -2.54, 0, "input", 2.54)
          + _pino(1, "", 10.16, 0, 180, "output", 2.54) + '\t\t\t)\n')
    u2 = ('\t\t\t(symbol "OPA1642_2_1"\n' + tri
          + _pino(5, "+", -10.16, 2.54, 0, "input", 2.54)
          + _pino(6, "-", -10.16, -2.54, 0, "input", 2.54)
          + _pino(7, "", 10.16, 0, 180, "output", 2.54) + '\t\t\t)\n')
    u3 = ('\t\t\t(symbol "OPA1642_3_1"\n' + ret
          + _pino(4, "V-", 0, -7.62, 90, "power_in", 2.54)
          + _pino(8, "V+", 0, 7.62, 270, "power_in", 2.54) + '\t\t\t)\n')
    return _cab("OPA1642", "U", "OPA1642", 10.16, -10.16) + u1 + u2 + u3 + '\t\t)'


def conn3_r():
    """Conector de 3 vias com a geometria EXACTA do JACK3.

    ⚠️ Existe para ser SUBSTITUICAO DIRECTA do JACK3. Os jacks TRS de 1/4"
    (Neutrik NMJ6HCD2) sao de PAINEL: prendem na caixa e ligam por fio, e a
    BOM-MPN ja dizia "header 1x3". Desenhar o jack no esquema poe o layout a
    tentar pousar um jack de painel no PCB.

    Os pinos ficam nas MESMAS coordenadas do JACK3 — (2.54, +5.08/0/-5.08),
    180° — para que a troca nao mova um unico fio e a netlist fique provada
    identica. Trocar geometria e simbolo ao mesmo tempo e ficar sem saber qual
    dos dois partiu a ligacao."""
    corpo = ('\t\t\t(symbol "CONN3_R_0_1"\n'
             '\t\t\t\t(rectangle\n\t\t\t\t\t(start -2.54 6.35)\n\t\t\t\t\t(end 2.54 -6.35)\n'
             '\t\t\t\t\t(stroke (width 0.254) (type default))\n'
             '\t\t\t\t\t(fill (type background))\n\t\t\t\t)\n\t\t\t)\n')
    p = ('\t\t\t(symbol "CONN3_R_1_1"\n'
         + _pino(1, "1", 2.54, 5.08, 180, "passive", 2.54)
         + _pino(2, "2", 2.54, 0.0, 180, "passive", 2.54)
         + _pino(3, "3", 2.54, -5.08, 180, "passive", 2.54)
         + '\t\t\t)\n')
    return _cab("CONN3_R", "J", "conector 3p", 7.62, -7.62) + corpo + p + '\t\t)'


if __name__ == "__main__":
    print("gera_simbolos v%s" % VERSAO, file=sys.stderr)
    for s in (nettie2(), dpdt(), conn3_r(), conn(3), conn(4), conn(5), conn(6), conn(7), idc26(),
              pot3(), ec11(), mosfet_n(), led(), opa1642(), esp32c3()):
        print(s)
