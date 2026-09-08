#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""dnp_para_pcb.py — leva o DNP do esquema para a placa.

    dnp_para_pcb.py            carimba
    dnp_para_pcb.py --conferir so verifica (sai != 0 se divergir)

⚠️⚠️ PORQUE E QUE ISTO TEM DE EXISTIR

O `(dnp yes)` vive nos SIMBOLOS do esquema. O `kicad-cli pcb export pos
--exclude-dnp` le o atributo do FOOTPRINT, na placa. Os dois lados nao se
falam, e nada avisa: o CPL saiu com oito pecas — C11 C13 C21 C23 R13 R23 RV1
RV2 — que a BOM nao tinha, porque a BOM (do esquema) obedeceu ao DNP e o CPL
(da placa) nao sabia dele.

A maquina recebia ordem para colocar oito pecas que ninguem comprou. Nenhum
verificador o via: cada exportador estava certo a partir da fonte DELE.

⚠️ E o carimbo NAO substitui o portao. O `para_fabrica` compara os
designadores do CPL com os da BOM e recusa se discordarem — porque este
ficheiro pode falhar, e um carimbo em que se confia sem comparar e a mesma
familia da 15dw: um campo preenchido a passar por um campo certo.

⚠️ USA O `pcbnew`, e nao escreve o token a mao. A marca DNP mudou de forma
entre versoes do formato; `SetDNP(True)` grava o que ESTA versao do KiCad le,
e o que o exportador dela vai obedecer. Escrever `(dnp yes)` a mao acerta ate
a proxima actualizacao, e depois falha em silencio — que e a pior maneira.
"""

VERSAO = "0.1.0"

import io
import os
import re
import subprocess
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
PY39 = ("/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/"
        "Versions/3.9/bin/python3.9")
PARES = (("cerebro", "cotuca_cerebro", "cotuca_cerebro.kicad_pcb"),
         ("painel", "cotuca_painel", "cotuca_painel.kicad_pcb"))


def dnp_do_esquema(pasta):
    """As referencias marcadas DNP nas folhas desta placa.

    ⚠️ Le por LINHA e nao por bloco de proposito. A primeira tentativa contou
    parentesis para delimitar o simbolo e devolveu ZERO — porque contar `(` e
    `)` cruamente conta tambem os que estao DENTRO de cadeias, e as notas deste
    esquema tem parentesis (`100R serie (~30 mA)`, `(!)`). A profundidade
    dessincroniza e os blocos saem todos errados, em silencio.
    """
    refs = set()
    for f in sorted(os.listdir(os.path.join(AQUI, pasta))):
        if not f.endswith(".kicad_sch"):
            continue
        ls = io.open(os.path.join(AQUI, pasta, f),
                     encoding="utf-8").read().splitlines()
        for i, l in enumerate(ls):
            if "(dnp yes)" not in l:
                continue
            for j in range(i, min(i + 10, len(ls))):
                m = re.search(r'\(property "Reference" "([^"]+)"', ls[j])
                if m:
                    refs.add(m.group(1))
                    break
            else:
                raise SystemExit("*** %s: `(dnp yes)` na linha %d sem "
                                 "Reference a seguir — LEITURA INCOMPLETA"
                                 % (f, i + 1))
    return refs


CODIGO = r'''
import sys, pcbnew
pcb, alvo, so_ler = sys.argv[1], set(sys.argv[2].split(",")) - {""}, sys.argv[3] == "1"
b = pcbnew.LoadBoard(pcb)
tem, mudou = set(), []
for fp in b.GetFootprints():
    r = fp.GetReference()
    if fp.IsDNP():
        tem.add(r)
    if not so_ler and (r in alvo) != fp.IsDNP():
        fp.SetDNP(r in alvo)
        mudou.append(r)
if mudou:
    pcbnew.SaveBoard(pcb, b)
    b2 = pcbnew.LoadBoard(pcb)
    tem = {f.GetReference() for f in b2.GetFootprints() if f.IsDNP()}
print("TEM:" + ",".join(sorted(tem)))
print("MUDOU:" + ",".join(sorted(mudou)))
'''


def main():
    print("dnp_para_pcb v%s" % VERSAO)
    conferir = "--conferir" in sys.argv
    mau = 0
    for nome, pasta, pcb in PARES:
        alvo = dnp_do_esquema(pasta)
        caminho = os.path.join(AQUI, pasta, pcb)
        r = subprocess.run([PY39, "-c", CODIGO, caminho, ",".join(sorted(alvo)),
                            "1" if conferir else "0"],
                           capture_output=True, text=True)
        # ⚠️ NAO se confia no codigo de saida: o `wx` desta versao devolve
        # `assert "traits" failed` de vez em quando e o processo sai mal com o
        # trabalho feito. Le-se o que ele IMPRIMIU.
        tem, mudou = set(), set()
        for l in r.stdout.splitlines():
            if l.startswith("TEM:"):
                tem = set(x for x in l[4:].split(",") if x)
            elif l.startswith("MUDOU:"):
                mudou = set(x for x in l[6:].split(",") if x)
        if not r.stdout.strip():
            raise SystemExit("*** %s: o pcbnew nao imprimiu nada.\n%s"
                             % (nome, r.stderr.strip()[-500:]))
        print("  %-9s esquema pede %2d · placa tem %2d%s"
              % (nome, len(alvo), len(tem),
                 " · carimbadas %d" % len(mudou) if mudou else ""))
        if tem != alvo:
            print("     ⚠️ so no esquema: %s" % (" ".join(sorted(alvo - tem)) or "—"))
            print("     ⚠️ so na placa:   %s" % (" ".join(sorted(tem - alvo)) or "—"))
            mau += 1
    if mau:
        raise SystemExit("*** %d placa(s) discordam do esquema quanto ao DNP.%s"
                         % (mau, "" if conferir else
                            " O carimbo correu e NAO resolveu — nao insistir, "
                            "ver porque."))
    return 0


if __name__ == "__main__":
    sys.exit(main())
