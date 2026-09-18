#!/usr/bin/env python3
"""encaixe — a placa monta-se? O portao da trilha M do IFA10000, SO DE LEITURA.

POR QUE EXISTE.

    "De que vale uma placa correta eletricamente se nao podemos monta-la?"
                                                        — dB, 18/09/2026

Nesse dia, com as duas placas rev C ja na fabrica e todos os portoes eletricos
verdes, uma sequencia de perguntas simples encontrou:

  · o painel sem um unico ponto de fixacao (pendurado em eixos que deixaram de
    o segurar, e num encoder sem rosca);
  · os pots comprados sem bucim roscado, e os que havia em casa sem caber;
  · a fita do IDC com metade do comprimento que o percurso pede;
  · o J14 do cerebro (8,85 mm de corpo) debaixo de um arcade que deixa 7,5.

Cada resposta saiu de um script solto no /tmp. Este ficheiro e esses scripts
tornados portao: o `para_fabrica` chama-o e recusa se ele disser que nao.

⚠️ SO LE. Nunca grava. O `pcb_contorno` mistura verificar e colocar, e correr
esse para ler o relatorio REESCREVEU o cerebro fabricado (18/09, reposto do
git). Aqui nao ha caminho de escrita.

⚠️ E NAO REFAZ CONTAS. A geometria da caixa, os discos dos arcade, a tabela de
alturas e a medida do courtyard vivem no `pcb_contorno` e no
`dentro_do_contorno`; este chama-os. Duas copias da mesma conta divergem.

O QUE CONFERE, por placa:
  M2  fixacao   >= 3 pontos nao colineares: furos de montagem (footprint
                MountingHole ou furo nao metalizado >= 2,5 mm) + pecas que a
                ficha prova aparafusarem (FIXACAO, com a fonte)
  M4  contorno  courtyard dentro do bordo (dentro_do_contorno)
  M6  volume    pecas debaixo dos arcade mais altas que a folga (pcb_contorno
                confere_arcade) — so nas placas que assentam no fundo
  CABOS         cada cabo entre placas: percurso 3D pela parede de tras + a
                folga de servico da caixa, contra o comprimento no BOM

USO:  encaixe.py <cerebro|painel|todas>
      encaixe.py --sabota        prova que cada verificacao sabe dizer nao
"""
import io
import math
import os
import re
import sys

VERSAO = "0.1.1"
AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)

# Pecas que PRENDEM a placa pela propria rosca, e so com a ficha a prova-lo.
# Vazio de proposito: o EC11E15244G1 nao tem rosca (ficha Alps: colar liso) e
# nenhum pot com bucim roscado esta ainda escolhido e medido (18/09).
FIXACAO = {
    # SW1: o encoder EM MAO tem bucim roscado (EC11 generico; foto do dB,
    # 18/09) — nao o EC11E15244G1 do BOM, que nao tem. A peca em mao manda.
    "painel": {"SW1": "EC11 generico com bucim roscado, peca em mao (dB, 18/09)"},
    "cerebro": {},
}

# Folga de servico em todo o cabo que vai do fundo a casca: e assim que o fundo
# sai com o cerebro agarrado. FONTE: caixa/CAIXA-ACRILICO-MONTAGEM.md §6b.
FOLGA_CABO = 80.0

# Cabos entre placas: (nome, (placa, ref), (placa, ref), regex do BOM)
CABOS = [
    ("IDC painel<->cerebro", ("cerebro", "J14"), ("painel", "J1"),
     r'"Cabo IDC 2x13[^"]*?~(\d+) mm'),
]


def _pcbnew():
    try:
        import pcbnew
        return pcbnew
    except ImportError:
        sys.exit("RECUSO: sem `pcbnew`. Corre com o Python do KiCad:\n"
                 "  /Applications/KiCad/KiCad.app/Contents/Frameworks/"
                 "Python.framework/Versions/3.9/bin/python3.9")


def _pcb(nome):
    return os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_pcb" % nome)


def _alvo(P, nome):
    alvo = dict(P.PLACAS[nome])
    if alvo["x0"] is None or alvo["y0"] is None:
        alvo = P.centra_no_fundo(alvo)
    return alvo


# ------------------------------------------------------------------ M2
def m2_fixacao(nome, b, sabota=False):
    pcbnew = _pcbnew()
    mm = lambda v: v / 1e6
    pontos = []
    for f in b.GetFootprints():
        if "MountingHole" in f.GetFPIDAsString():
            q = f.GetPosition()
            pontos.append((f.GetReference(), mm(q.x), mm(q.y), "furo de montagem"))
            continue
        for p in f.Pads():
            if (p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH
                    and mm(p.GetDrillSize().x) >= 2.5):
                q = p.GetPosition()
                pontos.append((f.GetReference(), mm(q.x), mm(q.y), "furo NPTH"))
    for ref, fonte in FIXACAO.get(nome, {}).items():
        f = [x for x in b.GetFootprints() if x.GetReference() == ref]
        if f:
            q = f[0].GetPosition()
            pontos.append((ref, mm(q.x), mm(q.y), "rosca — " + fonte))
    if sabota:
        pontos = []
    # nao colineares: a maior area de triangulo entre tres pontos
    area = 0.0
    for i in range(len(pontos)):
        for j in range(i + 1, len(pontos)):
            for k in range(j + 1, len(pontos)):
                (_, x1, y1, _), (_, x2, y2, _), (_, x3, y3, _) = (
                    pontos[i], pontos[j], pontos[k])
                area = max(area, abs((x2 - x1) * (y3 - y1)
                                     - (x3 - x1) * (y2 - y1)) / 2)
    ok = len(pontos) >= 3 and area > 100.0     # mm²: tres pontos quase em linha nao seguram
    txt = "%d ponto(s)%s" % (len(pontos), (" · triangulo %.0f mm²" % area)
                             if len(pontos) >= 3 else "")
    if not ok:
        txt += " — precisa de >= 3 nao colineares; SEM FURO NAO HA COMO PRENDER (prioridade 1: cede a placa)"
    return ok, txt, (0 if ok else 1)


# ------------------------------------------------------------------ M4
def m4_contorno(nome):
    import dentro_do_contorno as DC
    achados, isentos, n = DC.mede(_pcb(nome), DC.FOLGA, None, nome)
    ok = not achados
    txt = "%d footprints · %d achado(s) · %d isento(s)" % (n, len(achados), len(isentos))
    if achados:
        txt += " — " + ", ".join("%s %s" % (r, rz) for r, _, rz in achados)
    return ok, txt, len(achados)


# ------------------------------------------------------------------ M6
def m6_volume(P, nome, sabota=False):
    alvo = _alvo(P, nome)
    if alvo.get("assenta") != "fundo":
        return True, "nao se aplica (placa pendurada na tampa: volume por medir a mao, ver M6 da trilha)", 0
    t = io.open(_pcb(nome), encoding="utf-8").read()
    guarda = list(P.ALTURAS)
    try:
        if sabota:
            P.ALTURAS[:] = [(pad, 99.0, d) for pad, _, d in guarda]
        linhas = P.confere_arcade(t, alvo, P.FRENTE_CEREBRO)
    finally:
        P.ALTURAS[:] = guarda
    maus = [l.strip() for l in linhas if l.strip().startswith("⚠️")]
    folga = [l for l in linhas if "ALTURA LIVRE" in l]
    ok = not maus
    txt = (folga[0].strip() if folga else "folga ?") + " · " + (
        "nenhuma peca passa" if ok else "%d peca(s) nao cabem: %s"
        % (len(maus), "; ".join(m.replace("⚠️", "").strip() for m in maus)))
    # ⚠️ conta PECAS, nao verificacoes: uma placa que ja reprova no M6 tem de
    # reprovar MAIS sob sabotagem, senao a sabotagem nao prova nada.
    return ok, txt, len(maus)


# ------------------------------------------------------------------ CABOS
def _ponto_3d(P, nome, b, ref):
    """Pino 1 de `ref` em coordenadas da CAIXA: x, profundidade da frente, z."""
    C = P.CAIXA
    alvo = _alvo(P, nome)
    f = [x for x in b.GetFootprints() if x.GetReference() == ref][0]
    p1 = [p for p in f.Pads() if p.GetNumber() == "1"][0].GetPosition()
    kx, ky = p1.x / 1e6, p1.y / 1e6
    ox, oy = alvo["origem"]
    if alvo["assenta"] == "fundo":
        if P.FRENTE_CEREBRO != "y_min":
            raise SystemExit("*** FRENTE_CEREBRO != y_min: a conversao do fundo "
                             "tem de ser revista antes de medir cabos.")
        x = kx - ox + alvo["x0"] + C.T + 0.5
        d = ky - oy + alvo["y0"] + C.T + 0.5
        z = C.T + P.CALCO_H + 1.6 + (P.altura_de(f.GetFPIDAsString()) or 0.0)
    else:
        # na tampa: o desenho da face mede ao longo da inclinacao, desde a frente
        xf = kx - ox + alvo["x0"]
        yf = (alvo["y0"] + alvo["h"]) - (ky - oy)
        x = xf
        d = yf * C.RUN / C.SLOPE
        # ⚠️ z = face de baixo da tampa. A placa pendura ABAIXO dela, a uma
        # distancia que ainda nao esta desenhada — usar a tampa ALONGA o
        # percurso, que e o lado seguro para um comprimento de cabo.
        z = C.HF + (C.HB - C.HF) * d / C.D - C.T
    return x, d, z


def cabos(P, placas, bom_txt, sabota=False):
    C = P.CAIXA
    tras = C.D - C.T
    res = []
    for nome_cabo, (pa, ra), (pb, rb), rx in CABOS:
        if pa not in placas or pb not in placas:
            continue
        a = _ponto_3d(P, pa, placas[pa], ra)
        b = _ponto_3d(P, pb, placas[pb], rb)
        percurso = (tras - a[1]) + (tras - b[1]) + abs(a[2] - b[2]) + abs(a[0] - b[0])
        precisa = percurso + FOLGA_CABO
        m = re.search(rx, bom_txt)
        tem = float(m.group(1)) if m else None
        if sabota and tem is not None:
            tem = 150.0
        ok = tem is not None and tem >= precisa
        txt = ("%s: %s.%s -> %s.%s · percurso %.0f mm pela parede de tras + %.0f de "
               "folga = %.0f · BOM %s" % (nome_cabo, pa, ra, pb, rb, percurso,
                                         FOLGA_CABO, precisa,
                                         ("%.0f mm" % tem) if tem else "SEM LINHA"))
        if not ok:
            txt += " — CURTO" if tem else " — o BOM nao tem este cabo"
        res.append((ok, txt))
    return res


# ------------------------------------------------------------------ main
def corre(nomes, sabota=None):
    pcbnew = _pcbnew()
    import pcb_contorno as P
    placas = {n: pcbnew.LoadBoard(_pcb(n)) for n in ("cerebro", "painel")}
    bom_txt = io.open(os.path.join(RAIZ, "tools", "bom_completa.py"),
                      encoding="utf-8").read()
    falhas = 0
    for n in nomes:
        print("  ══ %s" % n)
        for etiqueta, (ok, txt, peso) in (
                ("M2 fixacao ", m2_fixacao(n, placas[n], sabota == "m2")),
                ("M4 contorno", m4_contorno(n)),
                ("M6 volume  ", m6_volume(P, n, sabota == "m6"))):
            print("     %s %s %s" % ("✓" if ok else "✗", etiqueta, txt))
            falhas += peso
    for ok, txt in cabos(P, placas, bom_txt, sabota == "cabo"):
        print("  %s CABO %s" % ("✓" if ok else "✗", txt))
        falhas += not ok
    return falhas


def main():
    print("encaixe v%s — a placa monta-se? (trilha M do IFA10000, so leitura)" % VERSAO)
    if "--sabota" in sys.argv:
        import contextlib
        maus = []
        for alvo, placa in (("m2", "cerebro"), ("m6", "cerebro"), ("cabo", "cerebro")):
            with contextlib.redirect_stdout(io.StringIO()):
                base = corre([placa])
                sab = corre([placa], sabota=alvo)
            diz = sab > base
            print("  sabotagem %-4s em %-7s: %d -> %d falha(s)  %s"
                  % (alvo, placa, base, sab, "✓ sabe dizer nao" if diz else "✗ NAO REPROVOU"))
            if not diz:
                maus.append(alvo)
        return 1 if maus else 0
    if len(sys.argv) < 2 or sys.argv[1] not in ("cerebro", "painel", "todas"):
        print(__doc__)
        return 2
    nomes = ["cerebro", "painel"] if sys.argv[1] == "todas" else [sys.argv[1]]
    f = corre(nomes)
    print("  %s" % ("MONTAVEL ✓" if not f else "%d achado(s) de encaixe — NAO MONTAVEL" % f))
    return 1 if f else 0


if __name__ == "__main__":
    sys.exit(main())
