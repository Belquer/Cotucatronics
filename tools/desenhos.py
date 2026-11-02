#!/usr/bin/env python3
"""desenhos v0.4.0 — regenera os desenhos tecnicos e denuncia os velhos.

    python3 tools/desenhos.py            # confere; sai != 0 se algum esta velho
    python3 tools/desenhos.py --gerar    # regenera e confere

⚠️ EXISTE PORQUE "MANTER ATUALIZADO" NAO E UMA REGRA, E UMA LEMBRANCA.
Um desenho que descreve o que a fonte deixou de dizer e pior que nenhum: tem
ar de oficial e mente. O `preamp-piezo.svg` viveu assim ate 2026-09-05 — dizia
`C2 1 nF` e o `RV1` vivo, quando o esquema ja dizia `20n` e `DNP`.

O criterio e mecanico e nao pede opiniao: **se a FONTE e mais nova que o
DESENHO, o desenho esta velho.** Nao se pergunta se mudou o que importa.
"""
import os, subprocess, sys, pathlib

VERSAO = "0.4.0"
RAIZ = pathlib.Path(__file__).resolve().parents[1]
CLI = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"

# desenho  <-  fonte que o gera
PARES = [
    ("docs/desenhos/esquematicos/cotuca_energia.svg", "kicad/cotuca_cerebro/cotuca_energia.kicad_sch"),
    ("docs/desenhos/esquematicos/cotuca_preamp.svg",  "kicad/cotuca_cerebro/cotuca_preamp.kicad_sch"),
    ("docs/desenhos/esquematicos/cotuca_soquete.svg", "kicad/cotuca_cerebro/cotuca_soquete.kicad_sch"),
    ("docs/desenhos/esquematicos/cotuca_conector.svg", "kicad/cotuca_cerebro/cotuca_conector.kicad_sch"),
    ("docs/desenhos/esquematicos/cotuca_painel.svg",   "kicad/cotuca_painel/cotuca_painel.kicad_sch"),
    # a caixa: pagina 1 tem gerador proprio desde 2026-09-05; as p2..p6 outro
]
# ⚠️ Ate 2026-09-05 a pagina 1 nao estava aqui, por nao ter gerador no repo. Ganhou
# um (`gen_caixa.py`) e entrou. Agora TODOS os desenhos desta pasta tem fonte.
# ⚠️ 2026-09-06: as 6 paginas da caixa sairam desta tabela porque foram APAGADAS —
# desenhavam uma caixa que nunca foi cortada (paredes de 6, frente de 40, dois
# arcade, um berco que nao existe). A fonte da caixa e agora o
# `caixa/cotuca_caixa_corte.py`, que tem portoes proprios. Ver cicatriz 15cf.


def gerar(fonte, destino):
    if not os.path.exists(CLI):
        print("  kicad-cli nao esta em %s" % CLI); return False
    out = RAIZ / "build" / "svg"
    out.mkdir(parents=True, exist_ok=True)
    r = subprocess.run([CLI, "sch", "export", "svg", "--output", str(out),
                        "--no-background-color", str(RAIZ / fonte)],
                       capture_output=True, text=True)
    ger = out / (pathlib.Path(fonte).stem + ".svg")
    if r.returncode != 0 or not ger.exists():
        print("  FALHOU a gerar %s: %s" % (destino, r.stderr.strip()[:60])); return False
    (RAIZ / destino).write_bytes(ger.read_bytes())
    return True


def main():
    ger = "--gerar" in sys.argv
    print("desenhos v%s" % VERSAO)
    velhos = 0
    for destino, fonte in PARES:
        fd, ff = RAIZ / destino, RAIZ / fonte
        if not ff.exists():
            print("  ? fonte ausente: %s" % fonte); velhos += 1; continue
        if ger and fonte.endswith(".kicad_sch"):
            gerar(fonte, destino)
        elif ger and fonte.endswith(".py"):
            subprocess.run([sys.executable, str(RAIZ / fonte)], capture_output=True)
        if not fd.exists():
            print("  ✗ FALTA      %s" % destino); velhos += 1; continue
        # ⚠️ a FONTE mais nova que o DESENHO e a definicao de velho.
        if ff.stat().st_mtime > fd.stat().st_mtime + 1:
            print("  ✗ VELHO      %s  (a fonte mudou depois)" % destino); velhos += 1
        else:
            print("  ✓ em dia     %s" % destino)
    print("  %d desenho(s) por regenerar" % velhos if velhos else "  todos em dia.")
    return 1 if velhos else 0


if __name__ == "__main__":
    sys.exit(main())
