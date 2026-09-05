#!/usr/bin/env python3
"""contrato_idc v0.2.1 — confere o IDC das DUAS placas, via a via.

    python3 tools/contrato_idc.py

⚠️ EXISTE PORQUE NENHUM ERC APANHA ISTO. Um rotulo global sozinho e LEGAL:
se o cerebro disser `BOT_FRZ_A` e o painel disser `BOT_FRZA`, os dois
projectos passam o ERC, os dois netlists sao coerentes, e as placas nao se
falam. O defeito nasce em fabrica e descobre-se com o ferro na mao.

O contrato e o IDC 2x13. Cada projecto declara os 26 nomes; este compara-os
POSICAO A POSICAO e sai != 0 a primeira diferenca.
"""
import io, re, sys, pathlib

VERSAO = "0.2.1"
RAIZ = pathlib.Path(__file__).resolve().parents[1]
LADOS = {
    "cerebro": RAIZ / "kicad/cotuca_cerebro/cotuca_conector.kicad_sch",
    "painel":  RAIZ / "kicad/cotuca_painel/cotuca_painel.kicad_sch",
}


def vias(fn):
    """{via: rotulo} — lido do FICHEIRO, nao do gerador.

    ⚠️ Comparar os dois geradores nao valeria nada: eles podem concordar um
    com o outro e discordar do que escreveram. Le-se o artefacto.
    """
    s = io.open(fn, encoding="utf-8").read()
    # pino do IDC -> coordenada absoluta; rotulo -> coordenada. Cruza-se pela
    # geometria, que e o que o KiCad usa para ligar.
    i = s.find("(lib_symbols"); d = 0
    for k in range(i, len(s)):
        if s[k] == "(": d += 1
        elif s[k] == ")":
            d -= 1
            if d == 0: lib, fora = s[i:k+1], s[:i] + s[k+1:]; break
    j = lib.find('(symbol "cotuca:IDC26"'); d = 0
    for k in range(j, len(lib)):
        if lib[k] == "(": d += 1
        elif lib[k] == ")":
            d -= 1
            if d == 0: bl = lib[j:k+1]; break
    offs = {m.group(3): (float(m.group(1)), float(m.group(2))) for m in
            re.finditer(r'\(pin\s+\S+\s+\S+\s*\n\s*\(at (-?[\d.]+(?:[eE][-+]?\d+)?) (-?[\d.]+(?:[eE][-+]?\d+)?)'
                        r'(?:\s+-?[\d.]+(?:[eE][-+]?\d+)?)?\)(?:.|\n)*?\(number "([^"]+)"', bl)}
    m = re.search(r'\(symbol\s*\n\s*\(lib_id "cotuca:IDC26"\)\s*\n\s*\(at (-?[\d.]+(?:[eE][-+]?\d+)?) (-?[\d.]+(?:[eE][-+]?\d+)?)', fora)
    if not m: return {}
    ox, oy = float(m.group(1)), float(m.group(2))
    rots = [(float(a), float(b), n) for n, a, b in
            re.findall(r'\(global_label "([^"]+)"(?:.|\n)*?\(at (-?[\d.]+(?:[eE][-+]?\d+)?) (-?[\d.]+(?:[eE][-+]?\d+)?)', fora)]
    out = {}
    for num, (px, py) in offs.items():
        ax, ay = round(ox + px, 2), round(oy - py, 2)
        perto = [n for x, y, n in rots if abs(y - ay) < 0.01 and abs(abs(x - ax) - 10.16) < 0.02]
        if perto: out[int(num)] = perto[0]
    return out


def main():
    print("contrato_idc v%s" % VERSAO)
    lados = {}
    for nome, fn in LADOS.items():
        if not fn.exists():
            print("  ? falta %s (%s) — nao ha contrato para conferir" % (nome, fn.name)); return 3
        lados[nome] = vias(fn)
        print("  %-8s %2d vias lidas de %s" % (nome, len(lados[nome]), fn.name))
    a, b = lados["cerebro"], lados["painel"]
    # ⚠️ LER MENOS DE 26 NAO E APROVACAO, E AUSENCIA DE DADO. A v0.1.0 leu 24
    # dos dois lados, faltaram as MESMAS duas, e a comparacao passou — porque
    # faltavam igualmente. Escreveu "as 26 vias batem" tendo lido 24. E o
    # defeito que este ficheiro existe para apanhar, cometido por ele proprio.
    falta = {n: sorted(set(range(1, 27)) - set(d)) for n, d in lados.items()}
    if any(falta.values()):
        for n, f in falta.items():
            if f: print("  ✗ %s: NAO consegui ler as vias %s" % (n, f))
        print("  *** LEITURA INCOMPLETA — nem aprovacao nem reprovacao")
        return 3
    mau = 0
    for v in range(1, 27):
        x, y = a.get(v), b.get(v)
        if x != y:
            print("  ✗ via %2d  cerebro=%-14s painel=%-14s" % (v, x or "—", y or "—")); mau += 1
    print("  %d via(s) em desacordo" % mau if mau else
          "  as %d vias batem, e sao as 26. Contrato honrado." % len(a))
    return 1 if mau else 0


if __name__ == "__main__":
    sys.exit(main())
