#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""codigos_lcsc.py — o codigo LCSC de cada linha SMD que a JLC vai montar.

    tools/codigos_lcsc.py            procura e grava
    tools/codigos_lcsc.py --conferir so verifica o que ja esta gravado

⚠️⚠️ PORQUE E QUE O MPN NAO CHEGA

O MPN e para humanos; **o codigo LCSC e o que a maquina le**. A JLC nao monta
uma linha sem `Cxxxxx`, tenha ela MPN ou nao — e por isso as 48 referencias sem
MPN dividem-se em DUAS coisas que nao se podem tratar igual:

  · THT e modulos     — comprados a mao, e o vazio esta certo
  · passivos SMD      — a JLC monta-os, e cada um precisa de codigo,
                        mesmo sendo um resistor generico de 0805

⚠️⚠️ A BUSCA MENTE COM CONFIANCA, E POR ISSO NAO SE USA A ORDEM

`4.7MΩ 0805` devolve um **LED** em primeiro lugar (C84256, 615 nm, mcd).
`0Ω 0805` devolve 1 kΩ e 10 kΩ. `100Ω 1206` devolve 1 kΩ e 3,3 kΩ. A ordenacao
e por semelhanca de texto e o valor nao domina — pegar no primeiro resultado
poe um LED onde vai um resistor, e o ficheiro de montagem nao tem como notar.

O que E fiavel sao as **descricoes**: o `100Ω 1210` certo estava na lista, so
nao em primeiro. Entao pede-se muitos e analisa-se cada descricao a serio —
valor, encapsulamento e classe tem de bater. A ordem nao decide nada.

⚠️⚠️ E COMPARA-SE COM TOLERANCIA, NUNCA POR IGUALDADE

A primeira versao comparava `22e-12` com `22 * 1e-12` por `==`. Diferem no
ultimo bit, e os condensadores certos foram rejeitados em silencio. O que saiu
foi **"0 de 35 passam"** — que se le exactamente como "o catalogo nao tem isto".

> Um falso negativo com cara de facto do catalogo e pior que um erro: ninguem
> volta a procurar o que ja foi dado por inexistente.
"""

VERSAO = "0.2.0"

import io
import json
import os
import re
import subprocess
import sys
import time
import urllib.parse
import urllib.request

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
SAIDA = os.path.join(AQUI, "codigos_lcsc.json")
UA = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)"
PLACAS = (("cerebro", "kicad/cotuca_cerebro/cotuca_cerebro.kicad_pcb"),
          ("painel", "kicad/cotuca_painel/cotuca_painel.kicad_pcb"))

MULT = {"p": 1e-12, "n": 1e-9, "u": 1e-6, "µ": 1e-6, "m": 1e-3,
        "": 1.0, "k": 1e3, "K": 1e3, "M": 1e6, "G": 1e9}

# ⚠️ A CHAVE E (valor, encapsulamento) TAL COMO ESTA NA PLACA, de proposito.
# Se alguem mudar `4M7` para `4.7M` no esquema, isto deixa de casar e GRITA, em
# vez de continuar a servir o codigo de uma peca que ja nao e a desenhada.
# Especificacao: (classe, valor, unidade, [consultas], nota)
ESPEC = {
    ("R", "0R", "0805"):     (0.0, "Ω", ["0R 0805", "0Ω 0805 Thick Film"], ""),
    ("R", "100R", "0805"):   (100.0, "Ω", ["100Ω 0805 resistor"], ""),
    ("R", "100R 1/4W", "1206"): (100.0, "Ω", ["100Ω 1206 resistor"], "1/4 W = 250 mW, e o 1206 da-os"),
    ("R", "100k", "0805"):   (100e3, "Ω", ["100kΩ 0805 resistor"], ""),
    ("R", "10k — L", "0805"): (10e3, "Ω", ["10kΩ 0805 resistor"], ""),
    ("R", "10k — R", "0805"): (10e3, "Ω", ["10kΩ 0805 resistor"], ""),
    ("R", "1k", "0805"):     (1e3, "Ω", ["1kΩ 0805 resistor"], ""),
    ("R", "22k", "0805"):    (22e3, "Ω", ["22kΩ 0805 resistor"], ""),
    ("R", "4M7", "0805"):    (4.7e6, "Ω", ["4.7MΩ 0805 resistor"], ""),
    ("R", "4k7", "0805"):    (4.7e3, "Ω", ["4.7kΩ 0805 resistor"], ""),
    ("R", "100R gate", "0805"): (100.0, "Ω", ["100Ω 0805 resistor"], ""),
    ("R", "10k pull-down", "0805"): (10e3, "Ω", ["10kΩ 0805 resistor"], ""),
    ("R", "100R serie (~30 mA)", "1210"): (100.0, "Ω", ["100Ω 1210 resistor"], ""),
    ("C", "100n", "0805"):   (100e-9, "F", ["100nF 0805 X7R 50V"], ""),
    ("C", "100n COLADO ao IDC", "0805"): (100e-9, "F", ["100nF 0805 X7R 50V"], ""),
    ("C", "100n COLADO ao cursor", "0805"): (100e-9, "F", ["100nF 0805 X7R 50V"], ""),
    ("C", "10u", "1206"):    (10e-6, "F", ["10µF 1206 X5R"], ""),
    ("C", "22p C0G", "0805"): (22e-12, "F", ["22pF 0805 C0G capacitor"], "C0G obrigatorio"),
    ("C", "22n C0G", "1206"): (22e-9, "F", ["22nF 1206 C0G capacitor", "22nF 1206 NP0"],
                               "⚠️ C0G no caminho de sinal do pre-amplificador — "
                               "um X7R aqui MUDA O SOM. Se a JLC nao tiver C0G "
                               "em 1206, e decisao do dB, nao substituicao minha."),
}
# ⚠️ A CASA ESPECIFICA RESISTORES A 1 %, e o `GENERICO` do `bom_completa` diz
# "resistor 0805 1%". A v0.1.0 nao olhava para a tolerancia e escolheu um ±5%
# para o `4M7` — porque ordenava por stock e o ±5% tinha mais. Ficava uma peca
# que contradiz a BOM sem nada a assinalar. Havia um ±1% na mesma lista.
TOLERANCIA_R = "±1%"

# ⚠️ dieletrico exigido: so entra quando a troca se OUVE.
DIELECTRICO = {("C", "22p C0G", "0805"): ("C0G", "NP0"),
               ("C", "22n C0G", "1206"): ("C0G", "NP0")}


def json_de(url, tenta=3):
    for i in range(tenta):
        try:
            req = urllib.request.Request(
                url, headers={"User-Agent": UA, "Accept": "application/json"})
            with urllib.request.urlopen(req, timeout=30) as r:
                return json.loads(r.read())
        except Exception:
            if i == tenta - 1:
                return {}
            time.sleep(1.5 * (i + 1))
    return {}


def busca(termo, n=50):
    return json_de("https://jlcsearch.tscircuit.com/api/search?q=%s&limit=%d&full=true"
                   % (urllib.parse.quote(termo), n)).get("components", []) or []


def valores(desc, unidade):
    """Todos os valores desta unidade que a descricao declara."""
    out = set()
    for m in re.finditer(r'(?<![\w.])(\d+(?:\.\d+)?)\s*([pnuµmkKMG]?)%s(?![\w])'
                         % unidade, desc):
        out.add(float(m.group(1)) * MULT[m.group(2)])
    return out


def bate(a, b):
    """⚠️ TOLERANCIA RELATIVA, nunca `==`. Ver o cabecalho: comparar
    `22e-12` com `22 * 1e-12` por igualdade rejeitou os condensadores certos e
    a saida — "0 de 35 passam" — leu-se como facto do catalogo."""
    if a == 0 or b == 0:
        return abs(a - b) < 1e-15
    return abs(a - b) / max(abs(a), abs(b)) < 1e-6


def escolhe(chave, alvo, unidade, consultas, exige_diel):
    classe = "Resistor" if chave[0] == "R" else "Capacitor"
    pac = chave[2]
    vistos, bons = set(), []
    for termo in consultas:
        for c in busca(termo):
            cod = "C%s" % c.get("lcsc")
            if cod in vistos:
                continue
            vistos.add(cod)
            d = c.get("description") or ""
            if pac not in d or classe not in d:
                continue
            if not any(bate(v, alvo) for v in valores(d, unidade)):
                continue
            if exige_diel and not any(x in d for x in exige_diel):
                continue
            if chave[0] == "R" and TOLERANCIA_R not in d:
                continue
            bons.append(c)
        time.sleep(0.5)
    # ⚠️ `is_basic` primeiro, e nao e preferencia estetica: uma peca Extended
    # leva taxa de carregamento por referencia na JLC, e num painel de 46 linhas
    # isso passa o custo de montagem a frente do custo das pecas.
    bons.sort(key=lambda c: (not c.get("is_basic"), -(c.get("stock") or 0)))
    return bons, len(vistos)


def inventario():
    """As linhas SMD, lidas do ATRIBUTO do footprint na placa.

    ⚠️ Do atributo, nao do nome da biblioteca. `attr smd` e o que o KiCad
    escreve e o que a fabrica le; deduzir SMD de um nome como `R_0805` acerta
    quase sempre e falha exactamente nos casos raros que interessam."""
    import collections
    inv = collections.defaultdict(list)
    for placa, p in PLACAS:
        t = io.open(os.path.join(RAIZ, p), encoding="utf-8").read()
        blocos = re.findall(r'\(footprint "([^"]+)"(.*?)\n\t\)\n', t, re.S)
        n_smd = 0
        for lib, corpo in blocos:
            am = re.search(r'\(attr ([^)]*)\)', corpo)
            if not am or "smd" not in am.group(1):
                continue
            n_smd += 1
            ref = re.search(r'\(property "Reference" "([^"]+)"', corpo)
            val = re.search(r'\(property "Value" "([^"]*)"', corpo)
            inv[(placa, val.group(1) if val else "",
                 lib.split(":")[-1])].append(ref.group(1) if ref else "?")
        if not n_smd:
            raise SystemExit("*** %s: zero footprints SMD — a leitura falhou, "
                             "e uma placa sem SMD nenhum nao e um facto, e um "
                             "leitor partido." % placa)
    return inv


def familia(pac):
    for f in ("0805", "1206", "1210", "2920"):
        if f in pac:
            return f
    return pac


def main():
    print("codigos_lcsc.py v%s" % VERSAO)
    conferir = "--conferir" in sys.argv
    sys.path.insert(0, AQUI)
    from bom_completa import MPN, MPN_PAINEL
    inv = inventario()
    antigo = {}
    if os.path.exists(SAIDA):
        antigo = json.load(io.open(SAIDA, encoding="utf-8")).get("linhas", {})

    linhas, faltam = {}, []
    for (placa, val, pac), refs in sorted(inv.items()):
        tab = MPN_PAINEL if placa == "painel" else MPN
        mpn = next((tab[r] for r in refs if tab.get(r)), "")
        chave = None
        for c in ESPEC:
            if c[1] == val and c[2] == familia(pac):
                chave = c
                break
        rot = "%s/%s/%s" % (placa, val, pac)

        if mpn:
            # ⚠️ Caminho EXACTO: peca com MPN resolve-se por casamento exacto,
            # que se provou fiavel nas 21 fichas. Nunca por busca difusa.
            if conferir or (antigo.get(rot, {}).get("codigo")
                            and antigo[rot].get("mpn") == mpn):
                linhas[rot] = antigo.get(rot, {})
            else:
                cs = busca(mpn, 10)
                achatar = lambda s: re.sub(r"[^A-Z0-9]", "",
                                           re.sub(r"\([^)]*\)", "", s.upper()))
                hit = next((c for c in cs
                            if achatar(c.get("mfr") or "") == achatar(mpn)), None)
                if hit:
                    linhas[rot] = {
                        "refs": sorted(refs), "valor": val, "encaps": pac,
                        "mpn": mpn, "codigo": "C%s" % hit.get("lcsc"),
                        "basic": bool(hit.get("is_basic")),
                        "prova": hit.get("description") or "",
                        "como": "casamento exacto do MPN"}
                else:
                    # ⚠️ O MPN NAO EXISTIR NA LCSC NAO DISPENSA O CODIGO. O
                    # `C1206C223J5GACTU` e um Kemet que a LCSC nao cataloga, e a
                    # v0.1.0 parava aqui e devolvia a linha sem codigo — ou seja,
                    # a peca com a especificacao MAIS exigente da placa (C0G no
                    # caminho de sinal) era a unica que nao tinha sequer uma
                    # tentativa de encontrar equivalente. Cai para a busca
                    # verificada, que exige o dielectrico.
                    mpn = ""
            if mpn:
                time.sleep(0.5)
        if not linhas.get(rot) and chave and not conferir:
            alvo, un, consultas, nota = (ESPEC[chave][0], ESPEC[chave][1],
                                         ESPEC[chave][2], ESPEC[chave][3])
            bons, vistos = escolhe((chave[0], val, familia(pac)), alvo, un,
                                   consultas, DIELECTRICO.get(chave))
            b = bons[0] if bons else None
            linhas[rot] = {
                "refs": sorted(refs), "valor": val, "encaps": pac, "mpn": "",
                "codigo": "C%s" % b.get("lcsc") if b else "",
                "basic": bool(b.get("is_basic")) if b else None,
                "prova": (b.get("description") or "") if b else
                         "nenhum de %d candidatos passou (valor+encaps+classe%s)"
                         % (vistos, "+dielectrico" if DIELECTRICO.get(chave) else ""),
                "como": "busca verificada: %d de %d candidatos passaram"
                        % (len(bons), vistos),
                "nota": nota}
        elif not linhas.get(rot):
            linhas[rot] = antigo.get(rot, {
                "refs": sorted(refs), "valor": val, "encaps": pac, "mpn": mpn,
                "codigo": "", "basic": None,
                "prova": "sem especificacao em ESPEC — nao se adivinha",
                "como": ""})
        e = linhas[rot]
        if not e.get("codigo"):
            faltam.append((rot, sorted(refs)))
        print("  %-44s %-11s %-6s %s"
              % (rot[:44], e.get("codigo") or "—",
                 "basic" if e.get("basic") else ("ext" if e.get("basic") is False else ""),
                 (e.get("prova") or "")[:52]))

    if not conferir:
        h = subprocess.run(["git", "-C", RAIZ, "log", "-1", "--format=%h"],
                           capture_output=True, text=True).stdout.strip()
        sujo = bool(subprocess.run(["git", "-C", RAIZ, "status", "--porcelain"],
                                   capture_output=True, text=True).stdout.strip())
        from datetime import date
        io.open(SAIDA, "w", encoding="utf-8").write(json.dumps(
            {"gerado_por": "tools/codigos_lcsc.py v%s" % VERSAO,
             "data": date.today().isoformat(), "commit": h, "arvore_suja": sujo,
             "linhas": linhas}, indent=2, ensure_ascii=False, sort_keys=True) + "\n")

    n = len(linhas)
    print("\n  %d de %d linhas SMD com codigo LCSC" % (n - len(faltam), n))
    if faltam:
        print("  SEM CODIGO, e a JLC nao monta a linha:")
        for rot, refs in faltam:
            print("     %-42s %s" % (rot[:42], " ".join(refs)))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
