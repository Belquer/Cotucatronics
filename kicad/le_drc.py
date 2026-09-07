#!/usr/bin/env python3
"""le_drc — o relatorio do DRC lido por rede, e nao so pelo total.

POR QUE EXISTE.

O total mente por omissao. `71 por ligar` nao diz que 23 delas sao terra e as
outras 48 sinal, e foi exactamente essa cegueira que deixou passar uma
regressao inteira: um roteamento baixou o total de 71 para 52 e ao mesmo tempo
partiu a ilha de AGND em oito pedacos. O total melhorou; a placa piorou.
Cicatriz 15co.

⚠️ E O NOME DA REDE NAO ESTA NA LINHA DO ACHADO. O cabecalho do
`unconnected_items` diz so "Missing connection between items"; a rede vem nas
DUAS linhas de item que se seguem. Um extractor que casa a linha do cabecalho
devolve zero achados com ar de placa limpa — e foi o que me devolveu, contra
um DRC que dizia 47. Verificador que mente e pior que verificador nenhum.

⚠️ E `Zone [X] <-> Zone [X]` E CATEGORIA A PARTE. E o plano em pedacos, e nao
se confunde com `starved_thermal`, que e um pad agarrado a uma ilha estreita —
esse e defeito de largura de cobre, nao de plano cortado.

USO:  le_drc.py <relatorio.rpt> [--todos]
"""
import re
import sys
import collections

VERSAO = "0.1.0"


def le(caminho):
    t = open(caminho, encoding="utf-8").read()
    achados = []
    bloco = None
    for linha in t.splitlines():
        m = re.match(r'^\[(\w+)\]: (.*)', linha)
        if m:
            if bloco:
                achados.append(bloco)
            bloco = {"tipo": m.group(1), "texto": m.group(2), "itens": []}
        elif bloco is not None and linha.startswith("    @("):
            bloco["itens"].append(linha.strip())
    if bloco:
        achados.append(bloco)
    return achados


def rede_de(item):
    m = re.search(r'\[([^\]]+)\]', item)
    return m.group(1) if m else "?"


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    print("le_drc v%s — %s" % (VERSAO, sys.argv[1].split("/")[-1]))
    achados = le(sys.argv[1])
    tipos = collections.Counter(a["tipo"] for a in achados)
    print("  %d achado(s): %s" % (len(achados), dict(tipos)))

    por_rede = collections.Counter()
    for a in achados:
        if a["tipo"] != "unconnected_items":
            continue
        redes = {rede_de(i) for i in a["itens"]}
        for r in redes:
            por_rede[r] += 1
    if por_rede:
        print("  por ligar, POR REDE (o total nao chega):")
        for r, n in sorted(por_rede.items(), key=lambda x: (-x[1], x[0])):
            print("     %-20s %d" % (r, n))

    partidas = [a for a in achados
                if len(a["itens"]) == 2
                and all("Zone [" in i for i in a["itens"])
                and rede_de(a["itens"][0]) == rede_de(a["itens"][1])]
    print("  PLANO EM PEDACOS: %d %s" % (len(partidas),
          "⚠️ o cobre foi cortado" if partidas else "✓"))
    for a in partidas:
        print("     %s <-> %s" % (a["itens"][0], a["itens"][1]))

    if "--todos" in sys.argv:
        for a in achados:
            if a["tipo"] in ("unconnected_items",):
                continue
            print("  [%s] %s" % (a["tipo"], a["texto"]))
            for i in a["itens"]:
                print("        %s" % i)


if __name__ == "__main__":
    main()
