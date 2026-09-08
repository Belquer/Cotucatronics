#!/usr/bin/env python3
"""compara_netlist — a netlist de antes contra a de depois, por PERTENCA.

POR QUE EXISTE.

A skill `kicad` §9 manda comparar a netlist depois de cada alteracao ao
gerador, e §7d diz como: compara-se a **pertenca** de cada rede — o conjunto de
`(referencia, pino)` — e nunca os nomes das redes, que mudam de proposito.

⚠️ E CONTA O QUE LEU (§2c). Escrevi um leitor a olho que devolveu "0 redes"
sobre um ficheiro com 76, e um leitor que perde linhas em silencio nao reprova
nada: aprova tudo. A guarda compara os blocos `(net` que existem com os que a
leitura apanhou, e recusa se nao baterem.

USO:  compara_netlist.py <antes.net> <depois.net>
"""
import re
import sys

VERSAO = "0.1.0"


def le(caminho):
    t = open(caminho, encoding="utf-8").read()
    blocos = len(re.findall(r"\(net\s*\n\s*\(code ", t))
    redes = {}
    for m in re.finditer(r'\(net\s*\n\s*\(code "?\d+"?\)\s*\n\s*\(name "([^"]*)"\)', t):
        nome = m.group(1)
        j = t.rfind("(net", 0, m.start() + 5)
        d = 0
        for k in range(j, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    break
        corpo = t[j:k + 1]
        nos = set(re.findall(r'\(node\s*\n\s*\(ref "([^"]+)"\)\s*\n\s*\(pin "([^"]+)"\)',
                             corpo))
        redes[nome] = nos
    if blocos != len(redes):
        raise SystemExit("*** o ficheiro tem %d blocos `(net` e a leitura apanhou "
                         "%d — LEITURA INCOMPLETA, nada a jusante e confiavel"
                         % (blocos, len(redes)))
    return redes


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    a, b = le(sys.argv[1]), le(sys.argv[2])
    print("compara_netlist v%s — %d redes antes, %d depois"
          % (VERSAO, len(a), len(b)))

    # ⚠️ pela PERTENCA, e nao pelo nome: uma rede renomeada de proposito tem de
    # sair como "igual", e uma rede com o mesmo nome e outros pinos tem de sair
    # como diferente. Comparar nomes troca as duas coisas.
    ia = {frozenset(v): k for k, v in a.items() if v}
    ib = {frozenset(v): k for k, v in b.items() if v}
    so_a = [ia[s] for s in set(ia) - set(ib)]
    so_b = [ib[s] for s in set(ib) - set(ia)]

    if not so_a and not so_b:
        print("  IDENTICAS na pertenca. Nenhuma ligacao mudou.")
        return 0
    print("  %d rede(s) so ANTES, %d so DEPOIS:" % (len(so_a), len(so_b)))
    for nome in sorted(so_a):
        print("     - %-18s %s" % (nome, " ".join(sorted("%s.%s" % p for p in a[nome]))))
    for nome in sorted(so_b):
        print("     + %-18s %s" % (nome, " ".join(sorted("%s.%s" % p for p in b[nome]))))
    print("\n  ⚠️ Cada linha destas e uma ligacao que mudou. Se alguma nao era "
          "para mudar,\n     e um defeito que nenhum ERC apanha.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
