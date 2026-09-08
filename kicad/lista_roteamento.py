#!/usr/bin/env python3
"""lista_roteamento — o que falta rotear, lido da placa e nao da memoria.

POR QUE EXISTE.

Havia uma lista escrita a mao. Dizia 71 ligacoes, e a placa tinha 12 — a
colocacao mudou tres vezes desde entao e a lista nao soube. Uma lista de
trabalho errada nao e um documento desactualizado: e alguem a rotear pistas
para sitios onde as pecas ja nao estao.

⚠️ A REGRA DA CASA: quando um artefacto ganha uma fonte, os desenhos anteriores
dele ou passam a ser gerados dessa fonte, ou morrem.

⚠️ E SAI CARIMBADA. Data gerada, commit, e aviso se a arvore estiver suja —
para se poder dizer, olhando para a folha, de que estado da placa ela saiu.

⚠️ O QUE ELA NAO DIZ: por onde passar. Diz de onde a onde, com que largura, e
em que ordem convem atacar. O caminho e o teu olho, e e por isso que estas
ficaram para ti: sao as que o roteador nao soube fazer.

USO:  lista_roteamento.py <cerebro|painel|todas>
"""
import collections
import datetime
import json
import math
import os
import re
import subprocess
import sys

VERSAO = "0.2.0"
AQUI = os.path.dirname(os.path.abspath(__file__))
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"

# ⚠️ As larguras sao as mesmas do roteador: uma lista que dissesse outra coisa
# poria a pessoa a desenhar cobre que o script recusaria.
sys.path.insert(0, AQUI)
from roteia import LARGURA                                        # noqa: E402

# ordem de ataque, e a razao de cada bloco
BLOCOS = [
    ("alta impedancia", "curtas, sobre a ilha de AGND, sem digital em paralelo (P5)",
     lambda n: n.startswith("Net-(U1") or n.startswith("Net-(C1") or
     n.startswith("Net-(C2") or n.startswith("Net-(J")),
    ("trilhos de alimentacao", "largas, e primeiro que as de sinal: ocupam mais espaco",
     lambda n: n in ("V+PRE", "V_TRILHO_A2", "+15V", "+5V", "+5V_PONTE", "+3V3A", "VBIAS")),
    ("audio", "afastadas do digital, e de preferencia sobre a ilha",
     lambda n: n.startswith(("LINHA_", "AUDIO_", "OUT_", "EXC_", "AMP_"))),
    ("terra", "o plano fez o resto; estas ficaram por fora dele",
     lambda n: n in ("GND", "AGND")),
    ("digital", "o que sobra: OLED, encoder, botoes, MIDI",
     lambda n: True),
]


def carimbo():
    def g(*a):
        return subprocess.run(a, capture_output=True, text=True, cwd=AQUI).stdout.strip()
    return (datetime.date.today().isoformat(),
            g("git", "rev-parse", "--short", "HEAD"),
            bool(g("git", "status", "--porcelain")))


def uma(nome):
    pcb = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_pcb" % nome)
    subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--format", "json",
                    "-o", "/tmp/_lr.json", pcb], capture_output=True)
    d = json.load(open("/tmp/_lr.json"))
    # ⚠️ CONTA O QUE LEU, E DECLARA O QUE DEIXOU DE FORA. A primeira versao
    # descartava em silencio as ligacoes em que uma das pontas e uma ZONA, e
    # dava 11 onde o DRC dava 12. Uma lista que conta menos que a ferramenta de
    # onde saiu ensina a pessoa a confiar no numero errado — e aquelas nao sao
    # ruido: sao pads que perderam o plano, e o conserto delas nao e uma pista,
    # e uma via ou um coto ate ao cobre.
    faltam, orfaos = [], []
    for x in d.get("unconnected_items", []):
        its = x.get("items", [])
        ds = [i.get("description", "") for i in its]
        if any("Zone [" in s for s in ds):
            for i, s in zip(its, ds):
                po = i.get("pos") or {}
                if "Zone [" not in s and "x" in po:
                    rede = re.search(r"\[([^\]]+)\]", s)
                    quem = re.search(r"of (\w+)", s)
                    orfaos.append((rede.group(1) if rede else "?",
                                   quem.group(1) if quem else "?",
                                   po["x"], po["y"]))
            continue
        pontas = []
        for i, s in zip(its, ds):
            po = i.get("pos") or {}
            if "x" not in po:
                pontas = []
                break
            rede = re.search(r"\[([^\]]+)\]", s)
            quem = re.search(r"of (\w+)", s)
            pontas.append((rede.group(1) if rede else "?",
                           quem.group(1) if quem else "pista",
                           po["x"], po["y"]))
        if len(pontas) == 2:
            faltam.append(pontas)

    data, commit, sujo = carimbo()
    L = ["# Lista de roteamento — %s" % nome, ""]
    L.append("Gerada por `kicad/lista_roteamento.py` v%s em **%s**, "
             "commit `%s`.%s" % (VERSAO, data, commit,
                                 "  \n⚠️ **A árvore estava suja**: a placa pode "
                                 "não corresponder a este commit." if sujo else ""))
    L.append("")
    L.append("**%d ligações**, mais **%d pad(s) que perderam o plano** — %d no "
             "total, que é o que o DRC diz. Tudo o resto já está feito e o DRC "
             "dá geometria zero com os planos inteiros, o que quer dizer que "
             "**o que desenhares tem de continuar a dar isso**."
             % (len(faltam), len(orfaos), len(faltam) + len(orfaos)))
    L.append("")
    L.append("Depois de as fazeres, o portão diz se ficou bom:")
    L.append("")
    L.append("```bash")
    L.append("python3 kicad/para_fabrica.py %s" % nome)
    L.append("```")
    L.append("")

    usadas = set()
    for titulo, porque, teste in BLOCOS:
        deste = [f for f in faltam
                 if id(f) not in usadas and teste(f[0][0])]
        if not deste:
            continue
        for f in deste:
            usadas.add(id(f))
        L.append("## %s" % titulo)
        L.append("")
        L.append("*%s*" % porque)
        L.append("")
        por_rede = collections.defaultdict(list)
        for f in deste:
            por_rede[f[0][0]].append(f)
        for rede in sorted(por_rede):
            larg = LARGURA.get(rede, 0.25)
            L.append("**`%s`** — %d ligação(ões) · **%.2f mm**"
                     % (rede, len(por_rede[rede]), larg))
            for f in sorted(por_rede[rede], key=lambda q: (q[0][3], q[0][2])):
                (_, a, ax, ay), (_, b, bx, by) = f
                L.append("- [ ] `%s` (%.1f, %.1f) → `%s` (%.1f, %.1f) · %.1f mm"
                         % (a, ax, ay, b, bx, by, math.hypot(bx - ax, by - ay)))
            L.append("")
    if orfaos:
        L.append("## pads que perderam o plano")
        L.append("")
        L.append("*Não são pistas: são pads da rede de terra que o cobre do "
                 "plano deixou de alcançar. O conserto é um **coto curto até ao "
                 "cobre mais próximo da mesma rede**, ou uma via, e não uma "
                 "pista até outro pad.*")
        L.append("")
        for rede, quem, x, y in sorted(orfaos, key=lambda q: (q[0], q[3])):
            L.append("- [ ] `%s` pad de `%s` em (%.1f, %.1f)" % (rede, quem, x, y))
        L.append("")
    return "\n".join(L) + "\n", len(faltam) + len(orfaos)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    print("lista_roteamento v%s" % VERSAO)
    quais = ["cerebro", "painel"] if sys.argv[1] == "todas" else [sys.argv[1]]
    for q in quais:
        txt, n = uma(q)
        dest = os.path.join(AQUI, os.pardir, "docs", "operacao",
                            "ROTEAMENTO-%s.md" % q.upper())
        open(dest, "w", encoding="utf-8").write(txt)
        print("  %s: %d ligacoes -> %s" % (q, n, os.path.relpath(dest, AQUI + "/..")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
