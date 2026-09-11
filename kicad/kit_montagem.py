#!/usr/bin/env python3
"""kit_montagem — o kit de montagem em casa / chapa quente, a partir do pacote.

POR QUE EXISTE.

O pacote da JLC (`para_fabrica.py`) serve para a JLC montar. Mas montar EM CASA
na chapa quente e outro fluxo: pede-se a PCB nua + stencil (montagem OFF),
compram-se as pecas na LCSC, imprime-se pasta pelo stencil, colocam-se as pecas
A MAO e faz-se refluxo. Este passo NAO desenha nem mede a placa — le o pacote ja
gerado e escreve os tres papeis que faltam para montar sem pensar duas vezes:

  MAPA_COLOCACAO.md  mapa de colocacao AGRUPADO POR VALOR (colocam-se todos os
                     100n, depois todos os 10k, em lote), com TOP e BOTTOM
                     separados e cada peca de baixo/THT sinalizada.
  COMPRA_LCSC.csv    carrinho da LCSC — um codigo por linha, com margem de
                     sobras para uma peca que cai no chao nao travar a montagem.
  LEIA-ME-KIT.txt    o arranque rapido em 6 linhas.

⚠️ A FONTE DO VALOR/FOOTPRINT E O CPL RICO. O CPL da JLC (`cpl_<board>.csv`)
so tem Designator,Mid X,Mid Y,Layer,Rotation — perdeu Val/Package no upload da
JLC. Por isso `para_fabrica` v0.18.0 preserva `cpl_rico_<board>.csv` ao lado, e
e esse que se le. Se faltar (pacote antigo), reconstroi-se do `cpl_<board>.csv`
(posicao/lado/rotacao) + `bom_<board>.csv` (valor/footprint/LCSC), pelo mesmo
metodo que a VISTORIA usa: geometria do CPL, identidade da peca do BOM.

⚠️ O VALOR E O FOOTPRINT VEM SEMPRE DO BOM, mesmo com o CPL rico presente. O
BOM ja e a lista de MONTAGEM: o `para_fabrica` normalizou la o valor (fundiu
`100n`, `100n COLADO ao cursor` e `100n COLADO ao IDC` numa so linha `100n`) e
juntou os designadores. Agrupar pelo Val cru do CPL rico voltaria a partir o
lote dos 100n em tres — exactamente o que o BOM ja resolveu. Do CPL vem so a
GEOMETRIA (X, Y, lado, rotacao); a IDENTIDADE da peca vem do BOM.

USO:  kit_montagem.py <pasta_do_pacote> [<pasta_do_pacote> ...]

Escreve num subdirectorio `kit/` de cada pasta. Puro CSV — nao precisa de KiCad.
Deterministico, UTF-8 sem BOM, sem edicao a mao de nenhum CSV gerado.
"""
import csv
import glob
import io
import math
import os
import re
import sys

VERSAO = "1.0.0"


def natkey(ref):
    """Ordena designadores como um humano: C2 < C10 < C16."""
    m = re.match(r"([A-Za-z]+)(\d+)", ref or "")
    return (m.group(1), int(m.group(2))) if m else (ref or "", 1 << 30)


def _ler_csv(caminho):
    with io.open(caminho, "r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def _lado(v):
    """Normaliza o lado para 'Top'/'Bottom' (CPL rico da 'top'/'bottom';
    CPL da JLC ja da 'Top'/'Bottom')."""
    v = (v or "top").strip().lower()
    return "Bottom" if v.startswith("b") else "Top"


def _trim(v):
    """Coordenada como o CPL a escreve: sem sufixo, sem zeros finais."""
    v = (v or "").strip()
    if "." in v:
        v = v.rstrip("0").rstrip(".")
    return v or "0"


def carrega_pos(rows):
    """Geometria por designador. Aceita tanto o CPL rico
    (Ref,Val,Package,PosX,PosY,Rot,Side) como o CPL da JLC
    (Designator,Mid X,Mid Y,Layer,Rotation) — le cada coluna pelos dois nomes."""
    pos = {}
    for r in rows:
        ref = (r.get("Ref") or r.get("Designator") or "").strip()
        if not ref:
            continue
        pos[ref] = {
            "ref": ref,
            "x": _trim(r.get("PosX") or r.get("Mid X") or ""),
            "y": _trim(r.get("PosY") or r.get("Mid Y") or ""),
            "rot": _trim(r.get("Rot") or r.get("Rotation") or "0"),
            "side": _lado(r.get("Side") or r.get("Layer") or ""),
        }
    return pos


def carrega_bom(rows):
    """Identidade da peca por designador: valor (Comment), footprint, LCSC.
    Expande a coluna Designator (lista separada por virgulas). Devolve tambem a
    Qty declarada por linha, para conferir contra a contagem de designadores."""
    por_ref, qty_linha = {}, {}
    for r in rows:
        comment = (r.get("Comment") or r.get("Value") or "").strip()
        fp = (r.get("Footprint") or "").strip()
        lcsc = (r.get("LCSC Part #") or r.get("LCSC") or "").strip()
        qty = (r.get("Qty") or "").strip()
        desig = re.findall(r"[A-Z]+\d+", r.get("Designator") or r.get("Ref") or "")
        for d in desig:
            por_ref[d] = {"val": comment, "fp": fp, "lcsc": lcsc}
        if lcsc:
            qty_linha[lcsc] = qty_linha.get(lcsc, 0) + len(desig)
    return por_ref, qty_linha


def eh_smt(fp, lcsc):
    """SMD (vai na chapa) vs THT (soldar a mao). Mesma heuristica da VISTORIA:
    footprint SMD, ou tem LCSC (a JLC so monta SMD catalogado)."""
    return ("SMD" in (fp or "").upper()) or bool(lcsc)


def pecas_do_pacote(pasta):
    """Descobre a placa e junta geometria (CPL) + identidade (BOM) por peca.

    Devolve (nome, lista_de_pecas, avisos). Cada peca: ref, val, fp, lcsc, x, y,
    side, rot, smt. A fonte da geometria e o CPL rico; se faltar, o CPL da JLC."""
    avisos = []
    boms = sorted(glob.glob(os.path.join(pasta, "bom_*.csv")))
    if not boms:
        raise SystemExit("*** %s nao tem bom_<board>.csv" % pasta)
    nome = re.match(r"bom_(.+)\.csv$", os.path.basename(boms[0])).group(1)

    rico = os.path.join(pasta, "cpl_rico_%s.csv" % nome)
    jlc = os.path.join(pasta, "cpl_%s.csv" % nome)
    if os.path.exists(rico):
        fonte_pos, pos = rico, carrega_pos(_ler_csv(rico))
    elif os.path.exists(jlc):
        fonte_pos, pos = jlc, carrega_pos(_ler_csv(jlc))
        avisos.append("sem cpl_rico_%s.csv — geometria reconstruida do CPL da "
                      "JLC + BOM (metodo VISTORIA)" % nome)
    else:
        raise SystemExit("*** %s nao tem cpl_rico_%s.csv nem cpl_%s.csv"
                         % (pasta, nome, nome))

    bom, qty_linha = carrega_bom(_ler_csv(os.path.join(pasta, "bom_%s.csv" % nome)))

    pecas = []
    for ref in sorted(pos, key=natkey):
        b = bom.get(ref)
        if not b:
            avisos.append("%s tem posicao mas nao esta no BOM — ignorado" % ref)
            continue
        p = dict(pos[ref])
        p.update(val=b["val"], fp=b["fp"], lcsc=b["lcsc"],
                 smt=eh_smt(b["fp"], b["lcsc"]))
        pecas.append(p)
    # designadores no BOM sem posicao (nao deviam existir — o cpl_bate_bom trava)
    for ref in bom:
        if ref not in pos:
            avisos.append("%s no BOM sem posicao no CPL" % ref)
    return nome, pecas, fonte_pos, qty_linha, avisos


def agrupa(pecas):
    """Agrupa por (valor, footprint, LCSC). Devolve grupos ordenados por valor."""
    g = {}
    for p in pecas:
        chave = (p["val"], p["fp"], p["lcsc"])
        g.setdefault(chave, []).append(p)
    for lista in g.values():
        lista.sort(key=lambda p: natkey(p["ref"]))
    return sorted(g.items(), key=lambda kv: (kv[0][0].lower(), kv[0][1], kv[0][2]))


def com_sobras(q):
    """Margem de sobras: +2 ate 5 pecas, +20% (arredondado a cima) acima disso.
    Uma peca que cai no chao nao pode travar a montagem."""
    return q + 2 if q <= 5 else q + int(math.ceil(q * 0.2))


def _sec_mapa(L, titulo, pecas, nota):
    L.append("## %s" % titulo)
    L.append("")
    L.append(nota)
    L.append("")
    if not pecas:
        L.append("_(nenhuma peca deste lado)_")
        L.append("")
        return
    for (val, fp, lcsc), lista in agrupa(pecas):
        tipo = "SMD" if lista[0]["smt"] else "THT — soldar a mao, por ultimo"
        cod = lcsc if lcsc else "— (sem LCSC)"
        L.append("### %s · %s" % (val or "(sem valor)", tipo))
        L.append("- **Footprint:** %s" % (fp or "?"))
        L.append("- **LCSC:** %s" % cod)
        L.append("- **Qty:** %d" % len(lista))
        L.append("- **Colocar:**")
        for p in lista:
            flag = "  ⚠️ BOTTOM — soldar a mao" if p["side"] == "Bottom" else ""
            L.append("    - `%s` @ (%s, %s) %s  rot %s°%s"
                     % (p["ref"], p["x"], p["y"], p["side"], p["rot"], flag))
        L.append("")


def escreve_mapa(caminho, nome, pecas, fonte_pos):
    top = [p for p in pecas if p["side"] == "Top"]
    bot = [p for p in pecas if p["side"] == "Bottom"]
    bottom_refs = ", ".join(p["ref"] for p in sorted(bot, key=lambda p: natkey(p["ref"]))) or "nenhuma"
    tht = ", ".join(p["ref"] for p in sorted(
        [p for p in pecas if not p["smt"]], key=lambda p: natkey(p["ref"]))) or "nenhuma"
    L = []
    L.append("# MAPA DE COLOCACAO — cotuca %s" % nome)
    L.append("")
    L.append("> Gerado por `kit_montagem.py` v%s a partir de `%s` + `bom_%s.csv`."
             % (VERSAO, os.path.basename(fonte_pos), nome))
    L.append(">")
    L.append("> **Como montar (ordem que este mapa assume):**")
    L.append("> 1. Refluxo do **SMD do TOPO** na chapa quente PRIMEIRO — coloca-se "
             "por lote, todos os de um valor antes do valor seguinte.")
    L.append("> 2. **Pecas do lado BOTTOM soldam-se A MAO** (nao vao na chapa): %s."
             % bottom_refs)
    L.append("> 3. **THT por ultimo**, todos a mao: %s." % tht)
    L.append("> 4. **Orientacao/polaridade:** a referencia e o RENDER 3D da placa "
             "e a `VISTORIA.md` do pacote, peca a peca — este mapa da posicao e "
             "rotacao, nao decide o sentido do pino 1 / catodo / faixa.")
    L.append("")
    _sec_mapa(L, "TOP — refluxo na chapa quente", top,
              "_Coloca-se por lote (todos os de um valor de uma vez), depois refluxo._")
    _sec_mapa(L, "BOTTOM — soldar a mao (NAO vai na chapa)", bot,
              "_Depois do topo estar feito. Cada peca aqui e soldada a mao._")
    with io.open(caminho, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L).rstrip("\n") + "\n")


def escreve_compra(caminho, nome, pecas, qty_linha):
    """Um codigo LCSC por linha. Qty = contagem de designadores com esse codigo;
    conferida contra a Qty declarada no BOM."""
    porc = {}
    for p in pecas:
        if not p["lcsc"]:
            continue
        e = porc.setdefault(p["lcsc"], {"vals": [], "fps": set(), "qty": 0})
        e["vals"].append(p["val"])
        e["fps"].add(p["fp"])
        e["qty"] += 1
    linhas = []
    for lcsc in sorted(porc):
        e = porc[lcsc]
        val = sorted(e["vals"], key=lambda c: (len(c), c))[0]
        fp = sorted(e["fps"])[0]
        linhas.append((lcsc, val, fp, e["qty"], com_sobras(e["qty"])))
    with io.open(caminho, "w", encoding="utf-8", newline="") as f:
        f.write("# carrinho LCSC — cotuca %s — codigos exactos da BOM de montagem "
                "(bom_%s.csv)\n" % (nome, nome))
        w = csv.writer(f, quoting=csv.QUOTE_ALL)
        w.writerow(["LCSC Part #", "Value", "Footprint", "Qty", "Qty+spares"])
        for lcsc, val, fp, qty, qtys in linhas:
            w.writerow([lcsc, val, fp, str(qty), str(qtys)])
    # conferencia: a contagem por LCSC bate na Qty do BOM
    mal = [lcsc for lcsc, e in porc.items()
           if lcsc in qty_linha and qty_linha[lcsc] != e["qty"]]
    return linhas, mal


def escreve_leiame(caminho, nome):
    linhas = [
        "KIT DE MONTAGEM — cotuca %s (chapa quente, em casa)" % nome,
        "1. Encomenda a PCB nua + STENCIL na JLC com a MONTAGEM DESLIGADA (assembly OFF).",
        "2. Compra as pecas na LCSC pelo COMPRA_LCSC.csv (codigos exactos; leva as sobras).",
        "3. Fixa a placa, poe o stencil e passa pasta Sn63/Pb37 com uma espatula.",
        "4. Coloca as pecas seguindo o MAPA_COLOCACAO.md — por lote, valor a valor.",
        "5. Refluxo do TOPO na chapa quente; depois solda a mao o lado BOTTOM e os THT.",
        "6. Inspeciona com lupa as juntas de SOIC/SOT (pontes, pino levantado, falta de solda).",
    ]
    with io.open(caminho, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(linhas) + "\n")


def uma(pasta):
    nome, pecas, fonte_pos, qty_linha, avisos = pecas_do_pacote(pasta)
    kit = os.path.join(pasta, "kit")
    os.makedirs(kit, exist_ok=True)
    escreve_mapa(os.path.join(kit, "MAPA_COLOCACAO.md"), nome, pecas, fonte_pos)
    linhas, mal = escreve_compra(os.path.join(kit, "COMPRA_LCSC.csv"),
                                 nome, pecas, qty_linha)
    escreve_leiame(os.path.join(kit, "LEIA-ME-KIT.txt"), nome)
    print("kit %s — %d pecas · %d TOP · %d BOTTOM · %d codigos LCSC"
          % (nome, len(pecas),
             sum(1 for p in pecas if p["side"] == "Top"),
             sum(1 for p in pecas if p["side"] == "Bottom"),
             len(linhas)))
    for a in avisos:
        print("   ⚠️ %s" % a)
    if mal:
        print("   ⚠️ contagem por LCSC diverge da Qty do BOM: %s" % ", ".join(mal))
        return False
    print("   escrito: %s" % os.path.relpath(kit, os.getcwd()))
    return True


def main(argv):
    if not argv:
        print(__doc__)
        return 2
    print("kit_montagem v%s" % VERSAO)
    ok = [uma(p) for p in argv]
    return 0 if all(ok) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
