#!/usr/bin/env python3
"""pinos_ponte v0.1.0 — o firmware da ponte e a PLACA falam dos mesmos pinos?

    (Python do KiCad)  tools/pinos_ponte.py [--sabota]

POR QUE EXISTE, e custou uma tarde de bancada (2026-09-21).

O dB montou tudo, o iPad viu o `Cotucatronics` e ligou, mexeu nos parametros
do Loopy — e nada. A placa dizia a verdade no `board`:

    midi: 0 volta(s) com sobra · pico 0 de 128 bytes do anel
    midi tx: DESCARTES 70 | falhas 631   <-- a ponte esta ligada?

**Nunca chegou um byte.** O motivo: o `esp32/ponte/ponte.ino` v0.5.0 punha a
UART em `GPIO4/5/6` — escolha legitima de quando a ponte vivia em CINCO FIOS
SOLTOS — e a placa rev C liga o soquete `U3` em `IO20/IO21/IO3`. Dois
documentos, os dois corretos no seu tempo, e nenhuma ferramenta a compara-los.

⚠️ E o sintoma nao acusa a causa: **o BLE funciona na mesma**. O radio nao usa
estes pinos, entao o iPad liga, o Loopy mostra os controlos, e tudo parece bem
ate se perguntar a Daisy quantos bytes chegaram.

> Duas fontes do mesmo facto — o mapa de pinos — e a que nao corre e a que
> envelhece. Aqui as duas "correm": uma e gravada no ESP32, a outra e fabricada
> em cobre. Por isso a comparacao tem de ser um TERCEIRO, e e este ficheiro.

SO LE. Nao grava firmware nem placa.
"""
import os
import re
import sys

VERSAO = "0.1.0"
AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
INO = os.path.join(RAIZ, "esp32", "ponte", "ponte.ino")
PCB = os.path.join(RAIZ, "kicad", "cotuca_cerebro", "cotuca_cerebro.kicad_pcb")

# o que cada constante do firmware TEM de casar, do lado da placa:
#   constante -> (pad do U3 esperado pela rede, rede)
# ⚠️ o sentido importa e e cruzado: o TX desta placa entra na rede que vai ao
# RX da Daisy. Trocar os dois nao queima nada — da silencio, que e o modo de
# falha mais caro de diagnosticar.
LIGACOES = {
    "PINO_TX":     "MIDI_RX",        # ESP transmite -> rede que chega ao RX da Daisy
    "PINO_RX":     "MIDI_TX",        # ESP recebe    <- rede que sai do TX da Daisy
    "PINO_STATUS": "PONTE_STATUS",
}


def do_firmware(texto=None):
    t = texto if texto is not None else open(INO, encoding="utf-8").read()
    achados = {}
    for nome in LIGACOES:
        m = re.search(r"static\s+const\s+int\s+%s\s*=\s*(\d+)\s*;" % nome, t)
        if not m:
            sys.exit("RECUSO: nao encontrei `%s` em %s — a leitura falhou, e "
                     "leitura incompleta nao aprova nada" % (nome, INO))
        achados[nome] = int(m.group(1))
    return achados


def da_placa():
    try:
        import pcbnew
    except ImportError:
        sys.exit("RECUSO: sem `pcbnew`. Corre com o Python do KiCad:\n"
                 "  /Applications/KiCad/KiCad.app/Contents/Frameworks/"
                 "Python.framework/Versions/3.9/bin/python3.9")
    b = pcbnew.LoadBoard(PCB)
    f = b.FindFootprintByReference("U3")
    if f is None:
        sys.exit("RECUSO: a placa nao tem `U3` — o soquete da ponte")
    por_rede = {}
    for p in f.Pads():
        rede = p.GetNetname()
        if rede:
            por_rede.setdefault(rede, []).append(p.GetNumber())
    return por_rede


def corre(sabota=False):
    fw = do_firmware()
    if sabota:
        fw["PINO_TX"] = 4           # o valor da v0.5.0, que a placa nao tem
    placa = da_placa()
    linhas, maus = [], 0
    for const, rede in sorted(LIGACOES.items()):
        pads = placa.get(rede, [])
        # os pads do SuperMini chamam-se IO20, IO21, IO3... — o numero e o que conta
        nums = []
        for pad in pads:
            m = re.search(r"(\d+)", pad)
            if m:
                nums.append(int(m.group(1)))
        ok = fw[const] in nums
        maus += not ok
        linhas.append("  %s %-12s firmware GPIO%-3d · placa %s (%s)"
                      % ("✓" if ok else "✗", const, fw[const],
                         "/".join("IO%d" % n for n in nums) or "—", rede))
    return linhas, maus


def main():
    print("pinos_ponte v%s — o firmware da ponte e a placa falam dos mesmos pinos?"
          % VERSAO)
    sabota = "--sabota" in sys.argv
    linhas, maus = corre()
    print("\n".join(linhas))
    if sabota:
        _l, m2 = corre(sabota=True)
        print("  sabotagem (PINO_TX de volta ao 4 da v0.5.0): %d -> %d divergencia(s)  %s"
              % (maus, m2, "✓ sabe dizer nao" if m2 > maus else "✗ NAO REPROVOU"))
        return 0 if m2 > maus else 1
    if maus:
        print("  ✗ %d divergencia(s): a ponte gravada NAO fala com esta placa.\n"
              "    Sintoma na bancada: o iPad liga e nada acontece; a Daisy diz\n"
              "    `pico 0 de 128 bytes do anel`. Regravar a ponte." % maus)
        return 1
    print("  ✓ o firmware da ponte casa com o cobre da placa")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
