#!/usr/bin/env python3
"""classes_de_rede — as larguras de trilho no sitio onde toda a gente as ve.

POR QUE EXISTE.

As larguras dos trilhos de potencia viviam numa tabela dentro do `roteia.py`.
Enquanto o roteador era meu isso funcionava — e escondia o problema: o DRC nao
sabia delas, e quando entrou um roteador de fora (FreeRouting) ele nao tinha
como saber, e desenhou +15V com a largura de sinal.

⚠️ UMA REGRA DE DESENHO QUE VIVE DENTRO DE UMA FERRAMENTA SO VALE PARA ESSA
FERRAMENTA. As classes de rede sao o sitio onde a placa guarda isto: qualquer
roteador as le, e o DRC passa a EXIGI-LAS em vez de as ignorar.

⚠️ E A LARGURA MINIMA BAIXA PARA 0,15 mm, de proposito. O FreeRouting estreita
a pista para sair de um pad de passo fino — que e como a industria resolve o
problema que bloqueou o meu roteador durante uma sessao inteira — e 0,20 mm de
minimo proibia-o. A JLCPCB faz 0,127 mm em cobre de 1 oz, portanto 0,15 tem 18%
de margem sobre o que a fabrica garante. A regra ajusta-se ao que a fabrica faz;
nao se aperta uma regra para la do necessario e depois se culpa o roteador.

⚠️ As redes que levam corrente continuam largas — isso e que nao se negoceia.

USO:  classes_de_rede.py <cerebro|painel|todas>
"""
import json
import os
import sys

VERSAO = "0.1.0"
AQUI = os.path.dirname(os.path.abspath(__file__))
MIN_LARGURA = 0.15          # a fabrica faz 0,127; isto tem margem
MIN_FOLGA = 0.2

CLASSES = [
    ("Potencia", 1.0, 0.25,
     ["+15V", "Net-(F1-Pad1)", "V_TRILHO_A2"],
     "entrada, fusivel e trilho bruto: a corrente toda da placa passa aqui"),
    ("Alimentacao", 0.5, 0.2,
     ["V+PRE", "VBIAS", "+5V", "+5V_PONTE", "+3V3A"],
     "trilhos regulados: dezenas de mA, e queda de tensao conta"),
]


def uma(nome):
    p = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_pro" % nome)
    d = json.load(open(p, encoding="utf-8"))
    ns = d["net_settings"]
    base = dict(ns["classes"][0])

    ns["classes"] = [c for c in ns["classes"]
                     if c["name"] not in {x[0] for x in CLASSES}]
    padrao = [c for c in ns["classes"] if c["name"] == "Default"][0]
    padrao["track_width"] = 0.25
    padrao["clearance"] = MIN_FOLGA

    for cnome, larg, folga, _redes, _porque in CLASSES:
        c = dict(base)
        c.update(name=cnome, track_width=larg, clearance=folga)
        ns["classes"].append(c)

    # ⚠️ por PADRAO e nao por nome: uma rede acrescentada ao esquema com o mesmo
    # prefixo entra na classe certa sozinha, e nao fica esquecida a 0,25 mm.
    pats = []
    for cnome, _l, _f, redes, _porque in CLASSES:
        for r in redes:
            pats.append({"pattern": r, "netclass": cnome})
    ns["netclass_patterns"] = pats

    d.setdefault("board", {}).setdefault("design_settings", {}).setdefault("rules", {})
    d["board"]["design_settings"]["rules"]["min_track_width"] = MIN_LARGURA
    json.dump(d, open(p, "w", encoding="utf-8"), indent=2)
    print("  %s: %d classes · largura minima %.2f mm · %d padrao(oes) de rede"
          % (nome, len(ns["classes"]), MIN_LARGURA, len(pats)))
    for cnome, larg, folga, redes, porque in CLASSES:
        print("     %-12s %.2f mm  %s" % (cnome, larg, porque))
        print("       %s" % " ".join(redes))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    print("classes_de_rede v%s" % VERSAO)
    for q in (["cerebro", "painel"] if sys.argv[1] == "todas" else [sys.argv[1]]):
        uma(q)
    return 0


if __name__ == "__main__":
    sys.exit(main())
