#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""manifesto_fontes.py — o portão STOP do abastecimento (sem rede, puro texto)

    manifesto_fontes.py <manifesto.csv|.json>   sai != 0 se alguma linha bloqueia
    manifesto_fontes.py --sabota                 prova que sabe dizer NAO
    manifesto_fontes.py --exemplos <dir>         escreve exemplo_bom/exemplo_mau

⚠️⚠️ O QUE ESTA FERRAMENTA EXISTE PARA APANHAR

A pipeline nao pode avancar para o layout/fabrica enquanto UMA linha do BOM
ficar por resolver. O erro caro e avancar com a linha "quase resolvida" — um
MPN que a LCSC vende mas que a maquina da JLC nao monta, ou um footprint
convertido que ninguem conferiu. Esta ferramenta le o manifesto de fontes e
recusa-se a dar por feito o abastecimento se alguma linha nao estiver num dos
tres estados legais:

  (A) C-number da LCSC valido, na biblioteca de MONTAGEM da JLC (stock > 0) e
      com tier registado (Basic/Preferred/Extended);
  (B) footprint importado/a-mao **verificado** (§4 do parts-sourcing) cujo
      C-number esta confirmado na biblioteca da JLC;
  (C) linha marcada explicitamente como fornecida-pelo-cliente, com footprint
      verificado (so quando a encomenda permite peca consignada).

Qualquer linha fora de (A/B/C) = BLOQUEADA. Nao se gera o pacote de fabrica.

⚠️ Esta ferramenta NAO fala com a rede. Ela nao confirma o stock na JLC —
confia no `assembly_stock` que o passo de sourcing (codigos_lcsc.py + a UI da
JLC) escreveu no manifesto. O stock vivo confirma-se antes, na origem; aqui o
que se guarda e a decisao, e o portao le a decisao.

Formato do manifesto (uma linha por linha do BOM agrupado):
  designator, intended_MPN, resolved_LCSC, tier, assembly_stock, source,
  footprint_source, verified, datasheet_ref, notes
CSV com cabecalho, ou JSON (lista de objectos com essas chaves).
"""

VERSAO = "0.3.0"

import csv
import io
import json
import os
import re
import sys

# ─────────────────────────────────────────────────────────────────────────────
# ESTADOS LEGAIS

C_NUMBER = re.compile(r"^C\d+$")
TIERS = {"basic", "preferred", "preferred extended", "extended"}
# footprint de biblioteca padrao dispensa a verificacao §4; importado/a-mao nao.
FP_PADRAO = {"", "kicad_std", "kicad", "std", "existing", "biblioteca"}
CLIENTE = {"customer_supplied", "customer-supplied", "cliente", "consignado"}
COLUNAS = ["designator", "intended_MPN", "resolved_LCSC", "tier",
           "assembly_stock", "source", "footprint_source", "verified",
           "datasheet_ref", "notes"]


def sim(v):
    """verified conta como 'sim' se comeca por sim/yes/y e nao por no."""
    t = (v or "").strip().lower()
    if t.startswith(("no", "nao", "não")):
        return False
    return t.startswith(("sim", "yes", "y", "ok", "conferid", "verif"))


def stock_int(v):
    m = re.search(r"-?\d+", str(v or ""))
    return int(m.group()) if m else 0


def rotulo(row):
    return (row.get("designator") or row.get("intended_MPN") or "?").strip()


def classifica(row):
    """(ok, via, motivo) — via em {A,B,C} quando ok, senao motivo do bloqueio."""
    lcsc = (row.get("resolved_LCSC") or "").strip()
    tier = (row.get("tier") or "").strip().lower()
    stock = stock_int(row.get("assembly_stock"))
    source = (row.get("source") or "").strip().lower()
    fpsrc = (row.get("footprint_source") or "").strip().lower()
    ver = sim(row.get("verified"))

    # (C) — fornecida pelo cliente: dispensa C-number/stock, exige verificacao.
    if source in CLIENTE:
        if ver:
            return True, "C", "cliente fornece · footprint verificado"
        return False, None, "C exige verified=yes (footprint por conferir)"

    # (A/B) — precisa de um C-number montavel na biblioteca da JLC.
    faltas = []
    if not C_NUMBER.match(lcsc):
        faltas.append("resolved_LCSC ausente ou mal formado (espera C#####)")
    if stock <= 0:
        faltas.append("assembly_stock <= 0 (fora da biblioteca de montagem)")
    if tier not in TIERS:
        faltas.append("tier nao e Basic/Preferred/Extended")
    # (B) — footprint importado ou a-mao tem de estar verificado (§4).
    if fpsrc not in FP_PADRAO and not ver:
        faltas.append("footprint importado/a-mao sem verified=yes (§4)")
    if faltas:
        return False, None, "; ".join(faltas)

    via = "B" if fpsrc not in FP_PADRAO else "A"
    return True, via, "LCSC %s · %s · stock %d" % (lcsc, tier, stock)


# ─────────────────────────────────────────────────────────────────────────────
# LEITURA

def carrega(caminho):
    with io.open(caminho, encoding="utf-8") as f:
        dados = f.read()
    if caminho.lower().endswith(".json"):
        try:
            obj = json.loads(dados)
        except ValueError as e:
            raise SystemExit("*** JSON invalido em %s: %s" % (caminho, e))
        if not isinstance(obj, list):
            raise SystemExit("*** JSON tem de ser uma lista de objectos")
        if not all(isinstance(r, dict) for r in obj):
            raise SystemExit("*** JSON tem de ser uma lista de OBJECTOS "
                             "(uma linha do BOM por objecto)")
        return [dict(r) for r in obj]
    try:
        return list(csv.DictReader(io.StringIO(dados)))
    except csv.Error as e:
        raise SystemExit("*** CSV invalido em %s: %s" % (caminho, e))


def confere(rows, quieto=False):
    """Devolve o numero de linhas bloqueadas; imprime o relatorio."""
    if not rows:
        raise SystemExit("*** manifesto vazio — nada a fabricar nao e sucesso")
    bloqueadas, contagem = [], {"A": 0, "B": 0, "C": 0}
    for i, row in enumerate(rows, 1):
        ok, via, motivo = classifica(row)
        if ok:
            contagem[via] += 1
        else:
            bloqueadas.append((i, rotulo(row), motivo))
    if not quieto:
        print("  %d linha(s) · A=%d  B=%d  C=%d  ·  %d BLOQUEADA(S)"
              % (len(rows), contagem["A"], contagem["B"], contagem["C"],
                 len(bloqueadas)))
        for i, nome, motivo in bloqueadas:
            print("     ⚠️ linha %d [%s] — %s" % (i, nome, motivo))
    return len(bloqueadas)


# ─────────────────────────────────────────────────────────────────────────────
# EXEMPLOS (para o demo e para o auto-teste)

_BOM = [
    # (A) jellybean Basic, existente
    {"designator": "R1,R2", "intended_MPN": "RC0402FR-0710KL",
     "resolved_LCSC": "C25744", "tier": "Basic", "assembly_stock": "500000",
     "source": "existing", "footprint_source": "kicad_std",
     "verified": "", "datasheet_ref": "", "notes": "10k 0402"},
    # (A) equivalente Extended com stock
    {"designator": "U1", "intended_MPN": "OPA1642AIDR",
     "resolved_LCSC": "C38318", "tier": "Extended", "assembly_stock": "1200",
     "source": "equivalent", "footprint_source": "kicad_std",
     "verified": "", "datasheet_ref": "OPA1642 SOIC-8", "notes": ""},
    # (B) importado via easyeda2kicad, footprint verificado
    {"designator": "J1", "intended_MPN": "USB4110-GF-A",
     "resolved_LCSC": "C2988369", "tier": "Extended", "assembly_stock": "8000",
     "source": "imported", "footprint_source": "easyeda2kicad",
     "verified": "yes (1:1 print + DRC)", "datasheet_ref": "GCT USB4110",
     "notes": "USB-C"},
    # (C) fornecido pelo cliente, footprint verificado a mao
    {"designator": "SW1", "intended_MPN": "arcade-100mm",
     "resolved_LCSC": "", "tier": "NA", "assembly_stock": "0",
     "source": "customer_supplied", "footprint_source": "hand",
     "verified": "yes (datasheet p.3 + 1:1)", "datasheet_ref": "arcade",
     "notes": "solda a mao"},
]


def _mau():
    """copia do BOM com tres defeitos plantados, um por classe de falha."""
    rows = [dict(r) for r in _BOM]
    rows[1]["resolved_LCSC"] = ""            # A: perdeu o C-number
    rows[2]["verified"] = "no"               # B: importado por conferir
    rows[3]["verified"] = ""                 # C: cliente sem verificacao
    return rows


def _escreve_csv(caminho, rows):
    with io.open(caminho, "w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=COLUNAS)
        w.writeheader()
        for r in rows:
            w.writerow({k: r.get(k, "") for k in COLUNAS})


def sabota():
    """Provar que sabe dizer NAO — e por causa DAQUELA linha, nao por acaso."""
    print("manifesto_fontes v%s — auto-teste --sabota" % VERSAO)
    print("  [bom]  espera 0 bloqueios:")
    limpo = confere(_BOM)
    print("  [mau]  espera >0 bloqueios (3 defeitos plantados):")
    sujo = confere(_mau())
    # E o teste fino: sabotar UMA linha certa e exigir que ELA passe a bloquear.
    base = confere(_BOM, quieto=True)
    rows = [dict(r) for r in _BOM]
    rows[0]["assembly_stock"] = "0"          # tira a R1/R2 da biblioteca
    pontual = confere(rows, quieto=True)
    ok = (limpo == 0) and (sujo >= 3) and (base == 0) and (pontual == 1)
    print("     bom=%d  mau=%d  base=%d  pontual=%d  %s"
          % (limpo, sujo, base, pontual,
             "✓ sabe dizer nao" if ok else "*** NAO REPROVOU"))
    return 0 if ok else 1


# ─────────────────────────────────────────────────────────────────────────────

def main(argv):
    print("manifesto_fontes v%s" % VERSAO)
    if "--sabota" in argv:
        return sabota()
    if "--exemplos" in argv:
        d = argv[argv.index("--exemplos") + 1]
        _escreve_csv(os.path.join(d, "exemplo_bom.csv"), _BOM)
        _escreve_csv(os.path.join(d, "exemplo_mau.csv"), _mau())
        print("  escrevi exemplo_bom.csv e exemplo_mau.csv em %s" % d)
        return 0
    args = [a for a in argv if not a.startswith("--")]
    if not args:
        raise SystemExit(__doc__)
    caminho = args[0]
    if not os.path.exists(caminho):
        raise SystemExit("*** nao ha %s" % caminho)
    n = confere(carrega(caminho))
    if n:
        raise SystemExit(
            "*** %d linha(s) BLOQUEADA(S). O abastecimento NAO esta feito — "
            "nao se gera o pacote de fabrica enquanto uma linha do BOM ficar "
            "fora de (A) C-number montavel, (B) footprint verificado, ou "
            "(C) cliente-fornece verificado." % n)
    print("  ✓ todas as linhas resolvidas (A/B/C). Abastecimento feito.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
