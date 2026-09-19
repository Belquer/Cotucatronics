#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""portao_humano.py — gerador do questionário do Portão Humano ⟨dB⟩ (/aipcb)

    portao_humano.py --cpl <cpl.csv> --bom <bom.csv> [--placa nome]
                     [--pacote <placa>_<data>_<commit>] [--branch <branch-do-PR>]
                     [--out VISTORIA.md]
    portao_humano.py --sabota        prova que assinala as peças de risco conhecidas

⚠️⚠️ O QUE ESTA FERRAMENTA EXISTE PARA APANHAR

O portão ⟨dB⟩ (pré-pagamento na JLC, e também o sign-off da spec no Estágio 1 e
o veto de render/dry-fit no 3/7) dizia "confirmar rotações" — uma instrução vaga
que deixa espaço para o humano e o script imaginarem coisas diferentes e para o
erro passar. Esta ferramenta transforma esse portão num QUESTIONÁRIO passo-a-passo:
um item de cada vez, quatro campos por item (ONDE OLHAR · O QUE DEVES VER · COMO
CONFIRMAR · RESPOSTA), e o portão só avança com ✅. Qualquer ❌ ou ambiguidade PÁRA
e encaminha para um conserto — nunca para o pagamento. Ver
`references/portao-humano.md` para o método.

READ-ONLY: lê o CPL e o BOM; NUNCA os altera. O único ficheiro que escreve é o
próprio questionário (stdout, ou --out VISTORIA.md).

O questionário ABRE SEMPRE com o **Item 0 — Carregar o pacote**: qual `.zip`
carimbado carregar na JLCONE, como o buscar ao PR (o pacote vive no GitHub, não
na máquina do humano), e a prova de proveniência pelo `LEIA-ME.txt` (commit +
árvore limpa). Só depois entram os itens de inspeção.

O motor deriva os itens da placa — não são escritos à mão:
  · peças polarizadas (díodos, electrolíticos, tântalo, LEDs, ICs pino-1),
  · pacotes com risco de rotação 180° (SOT-23, SOIC/SOP, SON/QFN, SMA/SMB/SOD,
    electrolítico, módulos),
  · conectores / contratos de pinagem (OLED, IDC placa-a-placa, alimentação),
  · linha-a-linha do BOM (SMT com LCSC, THT à parte, DNP),
  · mecânica (o que atravessa o painel),
  · e os passos de nível-placa + a confirmação final antes de pagar.

O --sabota corre sobre uma placa sintética com defeitos plantados e exige que
cada classe conhecida (SOT-23, SMB/SMA cátodo, SOIC pino-1, OLED, IDC) apareça no
questionário — uma guarda que nunca assinalou nada é promessa, não verificação.
"""

VERSAO = "0.3.0"

import argparse
import csv
import io
import os
import re
import sys

# ─────────────────────────────────────────────────────────────────────────────
# TABELAS DE HEURÍSTICA (carregadas, não re-derivadas por item)

# pacotes que a biblioteca da JLC roda 180° com frequência vs o KiCad
ROT_TOKENS = ["SOT-23", "SOT23", "SOIC", "SOP", "SSOP", "TSSOP", "MSOP",
              "QFN", "SON", "DFN", "QFP", "SMA", "SMB", "SMC", "SOD"]
# módulos com pino-1/chanfro que importa orientar
MODULE_TOKENS = ["Daisy", "ESP32", "SUPERMINI", "Module", "Seed"]
# footprints de IC de passo fino onde o pino-1 é o que importa
IC_PIN1_TOKENS = ["SOIC", "SOP", "SSOP", "TSSOP", "MSOP", "QFN", "SON",
                  "DFN", "QFP", "LGA", "BGA"]
# electrolítico / tântalo (polaridade +/−)
ELECTRO_TOKENS = ["CP_", "Elec", "Radial", "Tantal", "CASE-", "_Tantalum"]
TANTAL_TOKENS = ["Tantal", "_Tantalum", "CASE-A", "CASE-B", "CASE-C"]
# conectores
CONN_TOKENS = ["PinHeader", "IDC", "Molex", "JST", "TerminalBlock",
               "PicoBlade", "Header", "Socket", "Screw", "Terminal"]
# díodos/díodos SMD por footprint
DIODE_PKG_SMA_SMB = ["D_SMA", "D_SMB", "D_SMC", "D_SOD", "SMA", "SMB", "SMC", "SOD"]

# peças que atravessam o painel (mecânica / dry-fit)
MECH_TOKENS = ["RotaryEncoder", "Encoder", "Potentiometer", "OLED", "Display",
               "LCD", "arcade", "botão", "botao", "button", "IDC", "jack",
               "TerminalBlock"]

# nomes que são a MESMA peça vendidos com designações diferentes — a armadilha
# de o desenho (Value) e a compra (MPN/LCSC) chamarem coisas distintas
ALIASES = {
    "SS14": "1N5819", "1N5819": "SS14",
    "SS34": "1N5822", "1N5822": "SS34",
    "SS12": "1N5817", "1N5817": "SS12",
}

RESPOSTA = ("**RESPOSTA:**  ☐ ✅ conforme   ☐ ❌ + o que vês: "
            "________________________________")


def _match(s, tokens):
    s = (s or "")
    up = s.upper()
    return any(t.upper() in up for t in tokens)


def _match_word(s, tokens):
    """como _match, mas o token tem de ser palavra inteira — evita 'PD' casar
    dentro de 'DPDT' e afins."""
    up = (s or "").upper()
    return any(re.search(r"(?<![A-Z0-9])" + re.escape(t.upper()) + r"(?![A-Z0-9])", up)
               for t in tokens)


def _num(s):
    """formata a coordenada/rotação do CPL sem os zeros à direita."""
    try:
        f = float(s)
        return str(int(f)) if f == int(f) else ("%.3f" % f).rstrip("0").rstrip(".")
    except (TypeError, ValueError):
        return s or "?"


# ─────────────────────────────────────────────────────────────────────────────
# LEITURA DO CPL E DO BOM (única parte que fala com ficheiros de entrada)

def carrega_cpl(rows):
    """rows: iterável de dicts do CPL. Devolve lista de peças na ordem do CPL."""
    pecas = []
    for r in rows:
        ref = (r.get("Ref") or r.get("Designator") or "").strip()
        if not ref:
            continue
        pecas.append({
            "ref": ref,
            "val": (r.get("Val") or r.get("Comment") or "").strip(),
            "pkg": (r.get("Package") or r.get("Footprint") or "").strip(),
            "x": (r.get("PosX") or "").strip(),
            "y": (r.get("PosY") or "").strip(),
            "rot": (r.get("Rot") or "0").strip(),
            "side": (r.get("Side") or "top").strip(),
        })
    return pecas


def carrega_bom(rows):
    """rows: iterável de dicts do BOM. Devolve dict ref -> {footprint,lcsc,qty}."""
    by_ref = {}
    for r in rows:
        desig = (r.get("Designator") or r.get("Ref") or "").strip()
        fp = (r.get("Footprint") or "").strip()
        lcsc = (r.get("LCSC Part #") or r.get("LCSC") or "").strip()
        qty = (r.get("Qty") or "").strip()
        for d in [d.strip() for d in desig.split(",") if d.strip()]:
            by_ref[d] = {"footprint": fp, "lcsc": lcsc, "qty": qty}
    return by_ref


def _ler_csv(caminho):
    with io.open(caminho, "r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


# ─────────────────────────────────────────────────────────────────────────────
# CLASSIFICAÇÃO

def classe_polaridade(ref, val, pkg):
    """devolve o tipo de polaridade ('diodo','electro','tantalo','led','ic') ou None."""
    r0 = ref[:1].upper()
    if r0 == "D":
        if _match(val, ["LED"]):
            return "led"
        return "diodo"
    if r0 == "C" and _match(pkg, ELECTRO_TOKENS):
        return "tantalo" if _match(pkg, TANTAL_TOKENS) else "electro"
    if r0 == "U" and _match(pkg, IC_PIN1_TOKENS):
        return "ic"
    return None


def eh_rotacao_risco(pkg):
    return _match(pkg, ROT_TOKENS) or _match(pkg, MODULE_TOKENS)


def eh_conector(ref, pkg):
    return ref[:1].upper() == "J" or _match(pkg, CONN_TOKENS)


# ─────────────────────────────────────────────────────────────────────────────
# CONSTRUÇÃO DOS ITENS (cada item: dict com item/onde/ver/confirmar)

def _onde(p, extra=""):
    base = ("JLC 3D preview — rodar para vista de TOPO; ampliar sobre %s "
            "em X=%s, Y=%s (rotação CPL = %s°, lado %s)"
            % (p["ref"], _num(p["x"]), _num(p["y"]), _num(p["rot"]), p["side"]))
    return base + (" — " + extra if extra else "")


def _parse_pacote(nome):
    """<board>_<date>_<commit> -> (board, date, commit) ou None.

    board pode ter '_' (rsplit apanha data+commit pelo fim); a data é AAAA-MM-DD
    e o commit é hex curto."""
    m = re.match(r"^(.+)_(\d{4}-\d{2}-\d{2})_([0-9a-fA-F]+)$", (nome or "").strip())
    if m:
        return m.group(1), m.group(2), m.group(3)
    return None


def item_pacote(pacote, repo, branch):
    """Item 0 — o passo obrigatório ANTES da inspeção: buscar o pacote carimbado
    (que vive no PR, não necessariamente na máquina) e carregá-lo na JLCONE, com
    a verificação de proveniência pelo LEIA-ME. Auto-preenchido do nome do pacote.
    """
    parsed = _parse_pacote(pacote)
    if parsed:
        board, _date, commit = parsed
    else:  # fallback: sem pacote reconhecível, ainda nomeia o padrão do .zip
        pacote = pacote or "<placa>_<data>_<commit>"
        board, commit = "<placa>", "<commit>"
    zipname = "%s_gerbers.zip" % pacote
    zippath = "export/%s/%s" % (pacote, zipname)
    branch_txt = ("`%s`" % branch) if branch else "a branch do PR"
    branch_cmd = branch or "<branch>"
    reponame = repo.split("/")[-1] if repo else "Cotucatronics"

    onde = ("O pacote está **committado no repo** em `export/%s/` na branch do PR "
            "(%s, repo `%s` no GitHub) — está no **GitHub, não necessariamente na "
            "tua máquina**. ⚠️ O terminal que vês na pipeline corre no **sandbox na "
            "nuvem** e **não consegue pôr ficheiros na tua máquina** — a recuperação é "
            "um comando que **TU corres localmente**. Depois carregas no site da "
            "**JLCPCB/JLCONE** (*Add Gerber*). Antes de QUALQUER item de inspeção, a "
            "placa tem de estar carregada." % (pacote, branch_txt, repo))

    ver = ("**Gerber a carregar:** `%s` — é ESTE `.zip` que entra na JLCONE, não o "
           "`.kicad_pcb` nem gerbers soltos.\n"
           "  · **Como buscar (terminal no teu Mac — o preferido):** "
           "`cd %s && git fetch origin && git checkout %s && git pull origin %s && "
           "open export/%s/` — se ainda não o clonaste: "
           "`git clone -b %s git@github.com:%s.git`.\n"
           "  · **Fallback (GitHub no browser):** navega %s até `export/%s/`, abre a "
           "página do `%s` e clica **Download raw file** (repo privado → tens de estar "
           "autenticado como o dono); leva também o `cpl_%s.csv` + `bom_%s.csv`. Ou "
           "**Code → Download ZIP** da branch e tira a pasta `export/`.\n"
           "  · **Montagem SMT:** quando a JLC pedir, junta `cpl_%s.csv` (posições) + "
           "`bom_%s.csv` (BOM), da MESMA pasta.\n"
           "  · **Camadas:** 2.\n"
           "  · Depois, arrasta o `%s` para a JLCONE (*Add Gerber*)."
           % (zippath, reponame, branch_cmd, branch_cmd, pacote, branch_cmd, repo,
              branch_txt, pacote, zipname, board, board, board, board, zipname))

    confirmar = ("**Proveniência** — PASS: carregaste o `%s` (buscado a %s no GitHub) "
                 "e o `LEIA-ME.txt` da MESMA pasta carimba o commit `%s` e árvore "
                 "limpa — é ESSA a revisão que estás a encomendar. FAIL: não encontras "
                 "o pacote na tua máquina (está só no PR — vai buscá-lo ao GitHub), ou "
                 "carregaste outro zip, ou o `LEIA-ME.txt` carimba outro commit / "
                 "árvore suja → PÁRA e vai buscar o pacote certo do PR, NÃO encomendas "
                 "(guarda contra encomendar um pacote velho ou trocado)."
                 % (zipname, branch_txt, commit))

    return {
        "num": 0,
        "item": "Carregar o pacote — buscar ao PR e carregar na JLCONE",
        "onde": onde,
        "ver": ver,
        "confirmar": confirmar,
    }


def item_placa(placa, camadas, dims, qty):
    itens = []
    itens.append({
        "item": "Nível-placa — número de camadas",
        "onde": "O gerber (ou o LEIA-ME do pacote) e o formulário da JLC.",
        "ver": ("A placa é de **%s**. O formulário da JLC tem de dizer o MESMO "
                "número de camadas." % (camadas or "N camadas (confirmar)")),
        "confirmar": ("PASS: nº de camadas no formulário = nº real do desenho. "
                      "FAIL: pedir 4 quando o desenho só usa F.Cu+B.Cu paga um "
                      "plano interior a mais → corrigir o formulário, não pagar."),
    })
    itens.append({
        "item": "Nível-placa — dimensões e contorno vs caixa",
        "onde": "A pré-visualização 2D do contorno (Edge.Cuts) e o modelo da caixa.",
        "ver": ("O contorno bate na abertura da caixa; %s. Os furos de fixação "
                "coincidem." % (dims or "dimensões conforme o desenho")),
        "confirmar": ("PASS: contorno e furos batem na caixa (dry-fit / render). "
                      "FAIL: contorno fora do encaixe ou furo sob componente → "
                      "voltar ao Estágio 3, não pagar."),
    })
    itens.append({
        "item": "Nível-placa — quantidade a encomendar",
        "onde": "O carrinho da JLC.",
        "ver": ("A quantidade pedida = a que a ordem precisa (%s)."
                % (qty or "confirmar contra a ordem")),
        "confirmar": ("PASS: quantidade certa no carrinho. FAIL: quantidade "
                      "errada → corrigir antes de pagar."),
    })
    return itens


def item_polarizada(p, tipo, bom):
    ref, val, pkg = p["ref"], p["val"], p["pkg"]
    rot_risco = eh_rotacao_risco(pkg)
    nota_alias = ""
    chave = val.upper().split()[0] if val else ""
    for k, alias in ALIASES.items():
        if k in val.upper():
            nota_alias = (" ⚠️ NOME: `%s` e `%s` são a MESMA peça (Schottky %s) "
                          "com designações diferentes — confirmar que a serigrafia "
                          "(Value) e a compra (MPN/LCSC %s) falam da mesma peça."
                          % (k, alias, pkg.replace("D_", "").replace("_", ""),
                             (bom.get(ref, {}) or {}).get("lcsc", "?")))
            break

    if tipo == "diodo" and _match(pkg, ["SOT-23", "SOT23"]):
        ver = ("A disposição de pinos e a marca de pino-1 do modelo 3D coincidem "
               "com a serigrafia (o díodo duplo BAV99-e-afins tem 1 pino de um "
               "lado, 2 do outro). ⚠️ A biblioteca da JLC roda SOT-23 180° com "
               "frequência — este é o perigo.")
        confirmar = ("PASS: pino-1 do 3D = pino-1 do silk. FAIL: rodado 180° "
                     "(1 pino trocou de lado) → corrigir a rotação no CPL/footprint, "
                     "NÃO pagar.")
    elif tipo == "diodo" and _match(pkg, DIODE_PKG_SMA_SMB):
        ver = ("A barra/anel do CÁTODO no modelo 3D coincide com a faixa impressa "
               "na serigrafia (o lado do K). ⚠️ Pacote %s tem risco de 180°." % pkg)
        confirmar = ("PASS: cátodo do 3D = faixa do silk. FAIL: invertido → o "
                     "díodo conduz ao contrário → corrigir a rotação, NÃO pagar.")
    elif tipo == "led":
        ver = ("A marca de cátodo/chanfro do LED no 3D bate no lado que a "
               "serigrafia marca (K / chanfro).")
        confirmar = ("PASS: cátodo do 3D = marca do silk. FAIL: invertido → "
                     "LED não acende / polaridade trocada → corrigir, NÃO pagar.")
    elif tipo in ("electro", "tantalo"):
        marca = "faixa − (electrolítico)" if tipo == "electro" else "faixa + (tântalo)"
        ver = ("A %s do modelo 3D fica do lado que a serigrafia marca. "
               "Confirmar a perna longa/curta vs o pad quadrado." % marca)
        confirmar = ("PASS: polaridade do 3D = marca do silk. FAIL: + e − "
                     "trocados → o condensador rebenta ao ligar → NÃO pagar.")
    elif tipo == "ic":
        ver = ("O ponto/chanfro do PINO 1 no modelo 3D fica no canto que a "
               "serigrafia marca (o ponto no canto).")
        confirmar = ("PASS: pino-1 do 3D = pino-1 do silk. FAIL: rodado 90°/180° "
                     "→ pinout todo trocado → corrigir a rotação, NÃO pagar.")
    else:
        ver = "A marca de polaridade do 3D bate na serigrafia."
        confirmar = "PASS: coincide. FAIL: invertido → corrigir, NÃO pagar."

    return {
        "item": "%s — %s (%s)" % (ref, val, pkg),
        "onde": _onde(p, "comparar 3D vs serigrafia (silk)"),
        "ver": ver + nota_alias,
        "confirmar": confirmar,
    }


def item_rotacao(p):
    ref, val, pkg = p["ref"], p["val"], p["pkg"]
    if _match(pkg, MODULE_TOKENS):
        ver = ("A orientação do módulo (chanfro / pino-1 / borda do USB) no 3D "
               "bate na serigrafia. Rotação CPL = %s°." % _num(p["rot"]))
        confirmar = ("PASS: pino-1/borda do 3D = silk. FAIL: rodado 180° → "
                     "alimentação e sinais trocam de fila → corrigir, NÃO pagar.")
    else:
        ver = ("O pino-1 (chanfro/ponto) do 3D bate na serigrafia. ⚠️ %s é dos "
               "pacotes que a JLC roda 180° — este é o perigo dos 180° da "
               "biblioteca JLC vs KiCad." % pkg)
        confirmar = ("PASS: pino-1 do 3D = pino-1 do silk. FAIL: rodado 180° → "
                     "pinout trocado (dreno/fonte, entrada/saída) → corrigir a "
                     "rotação no CPL/footprint, NÃO pagar.")
    return {
        "item": "%s — %s (%s)" % (ref, val, pkg),
        "onde": _onde(p, "confirmar orientação vs pino-1 pretendido"),
        "ver": ver,
        "confirmar": confirmar,
    }


def item_conector(p):
    ref, val, pkg = p["ref"], p["val"], p["pkg"]
    v = val.upper()
    if "OLED" in v or "DISPLAY" in v or "LCD" in v:
        ver = ("O contrato de pinagem placa↔módulo, pino a pino. ⚠️ Perigo "
               "clássico do OLED SPI: com um cabo 1:1 o **VCC e o GND trocam** "
               "(o módulo morre ao ligar) e o **DIN/CLK também trocam**. "
               "Confirmar a ordem de %s contra o conector do módulo REAL em mão "
               "(ex.: Waveshare 2,42\": VCC·GND·DIN·CLK·CS·DC·RST)." % ref)
        confirmar = ("PASS: cada pino do J bate no pino homólogo do módulo (ou o "
                     "cabo cruzado corrige). FAIL: VCC/GND ou DIN/CLK trocados → "
                     "reordenar o J ou crimpar cabo cruzado ANTES de pagar.")
    elif "IDC" in pkg.upper() or "IDC" in v:
        ver = ("O contrato placa-a-placa: os dois lados do IDC (ex.: 2x13 / "
               "26 vias) batem pino-a-pino entre as duas placas, com o pino-1 "
               "alinhado (chanfro do shroud do mesmo lado nas duas).")
        confirmar = ("PASS: 26/26 pino-a-pino, pino-1 alinhado. FAIL: fila "
                     "invertida ou pino-1 no lado errado → corrigir o footprint/"
                     "orientação, NÃO pagar.")
    elif _match_word(val, ["alim", "PD", "5V", "12V", "power", "V+", "entrada", "amp"]) \
            or _match(pkg, ["TerminalBlock"]):
        ver = ("A polaridade da alimentação: qual pino é + e qual é GND/−, "
               "batendo na serigrafia e no que o cabo entrega.")
        confirmar = ("PASS: + e GND na ordem certa. FAIL: polaridade trocada → "
                     "corrigir a serigrafia/cablagem, NÃO pagar.")
    else:
        ver = ("A ordem de pinos de %s bate no que o cabo/módulo do outro lado "
               "espera (pino-1 na serigrafia)." % ref)
        confirmar = ("PASS: pinagem coincide. FAIL: fila/ordem trocada → "
                     "corrigir antes de pagar.")
    # ⚠️⚠️ PRIMEIRO SE E PRECISO, SO DEPOIS SE ESTA CERTO (18/09).
    # O item do J4 do cerebro ("a ponte ESP32") perguntava se a ordem de pinos
    # batia "no que o cabo/modulo do outro lado espera" — e NAO HAVIA outro
    # lado: era a tomada da ponte por fio, que tinha passado a soquete (U3) a
    # 06/09. Tres revisoes olharam para ele e todas perguntaram se estava
    # CERTO. A pergunta que o apanha e anterior: o que liga aqui?
    ver = ("**Primeiro: o que liga aqui?** Escreve o nome da peca ou do cabo que "
           "encaixa em %s. Se nao souberes dizer, o conector esta A MAIS — e "
           "isso e um FAIL, nao um detalhe. So depois: " % ref) + ver
    confirmar = ("FAIL se ninguem souber dizer o que liga em %s. " % ref) + confirmar
    return {
        "item": "%s — %s (%s)" % (ref, val, pkg),
        "onde": _onde(p, "dizer o que liga aqui, e so depois confirmar a pinagem"),
        "ver": ver,
        "confirmar": confirmar,
    }


def constroi(pecas, bom, placa, camadas, dims, qty,
             pacote=None, repo="Belquer/Cotucatronics", branch=""):
    """Devolve lista de (titulo_categoria, [itens])."""
    cats = []

    # 0. Carregar o pacote — SEMPRE o primeiro item do questionário: qual ficheiro
    #    carregar (o .zip carimbado), como o buscar ao PR, e a prova de proveniência.
    cats.append(("0. Carregar o pacote (antes de tudo)",
                 [item_pacote(pacote, repo, branch)]))

    # 1. Nível-placa
    cats.append(("1. Nível-placa", item_placa(placa, camadas, dims, qty)))

    # classificar as peças
    polar, rot, conetores = [], [], []
    for p in pecas:
        if eh_conector(p["ref"], p["pkg"]):
            conetores.append(p)
            continue
        tipo = classe_polaridade(p["ref"], p["val"], p["pkg"])
        if tipo:
            polar.append((p, tipo))
        elif eh_rotacao_risco(p["pkg"]):
            rot.append(p)

    # 2. Peças polarizadas
    cats.append(("2. Peças polarizadas (banda/cátodo/pino-1 vs serigrafia)",
                 [item_polarizada(p, t, bom) for p, t in polar]))

    # 3. Pacotes com risco de rotação (não-polarizados: transístores, módulos, …)
    cats.append(("3. Pacotes com risco de rotação 180° (orientação vs pino-1)",
                 [item_rotacao(p) for p in rot]))

    # 4. Conectores / contratos de pinagem
    cats.append(("4. Conectores / contratos de pinagem",
                 [item_conector(p) for p in conetores]))

    # 5. BOM / montagem
    itens_bom = []
    smt_sem_lcsc, smt_com_lcsc, tht = [], [], []
    for p in pecas:
        b = bom.get(p["ref"], {})
        fp = b.get("footprint", "")
        lcsc = b.get("lcsc", "")
        eh_smt = ("SMD" in fp.upper()) or bool(lcsc)
        if eh_smt:
            (smt_com_lcsc if lcsc else smt_sem_lcsc).append(p["ref"])
        else:
            tht.append("%s (%s)" % (p["ref"], p["val"]))
    itens_bom.append({
        "item": "BOM — toda linha SMT com LCSC em stock de MONTAGEM",
        "onde": "O BOM (coluna LCSC) e a página da peça na JLC (Assembly / stock).",
        "ver": ("Todas as %d linhas SMT levam C-number e stock de MONTAGEM>0. "
                "Sem LCSC: %s." % (len(smt_com_lcsc),
                                   ", ".join(smt_sem_lcsc) or "nenhuma")),
        "confirmar": ("PASS: cada SMT com LCSC e stock de montagem. FAIL: linha "
                      "SMT sem LCSC ou sem stock de montagem → resolver a fonte "
                      "(⚠️ stock na LCSC ≠ stock de montagem), NÃO pagar."),
    })
    itens_bom.append({
        "item": "BOM — THT enviados à parte (não vão pela JLC)",
        "onde": "A lista de peças THT do desenho.",
        "ver": ("As peças THT são montadas por ti, à parte: %s."
                % (", ".join(tht) or "nenhuma")),
        "confirmar": ("PASS: THT marcadas como 'cliente fornece / à parte'. "
                      "FAIL: THT metida na montagem SMT → corrigir o BOM/CPL."),
    })
    itens_bom.append({
        "item": "BOM — DNP correcto",
        "onde": "As linhas marcadas DNP (do not place) no BOM/CPL.",
        "ver": "As peças DNP estão fora da montagem; as de montar estão dentro.",
        "confirmar": ("PASS: DNP coincide com a intenção. FAIL: peça a montar "
                      "marcada DNP (ou vice-versa) → corrigir antes de pagar."),
    })
    cats.append(("5. BOM / montagem", itens_bom))

    # 6. Mecânica
    mech = [p for p in pecas
            if not re.match(r"^[CR]\d", p["ref"].upper())   # passivos C*/R* nunca alcançam o painel (RV* fica)
            and (_match(p["val"], MECH_TOKENS) or _match(p["pkg"], MECH_TOKENS))]
    lista = "; ".join("%s (%s)" % (p["ref"], p["val"]) for p in mech) or "—"
    cats.append(("6. Mecânica (alcance dos recortes do painel)", [{
        "item": "Mecânica — conectores/botões/display alcançam os recortes",
        "onde": "O render da caixa e/ou o dry-fit de UMA caixa antes da série.",
        "ver": ("Cada peça que atravessa o painel alcança o recorte certo: %s. "
                "Folga do cabo e alcance com a placa montada." % lista),
        "confirmar": ("PASS: dry-fit/render mostra cada uma no seu recorte. "
                      "FAIL: encoder/pot/display/botão não alcança ou colide → "
                      "voltar ao Estágio 3, NÃO pagar."),
    }]))

    # 7. Confirmação final
    cats.append(("7. Confirmação final antes de pagar", [{
        "item": "Passe limpo — precondição do pagamento",
        "onde": "Este questionário, de cima a baixo.",
        "ver": ("TODOS os itens acima com ✅. Zero ❌, zero campo em branco, "
                "zero ambiguidade."),
        "confirmar": ("PASS (todos ✅): o humano fica AUTORIZADO a pagar — o "
                      "pagamento é acção do humano. FAIL (qualquer ❌/dúvida): "
                      "PÁRA, encaminha para o conserto, e NÃO se paga."),
    }]))

    return cats


# ─────────────────────────────────────────────────────────────────────────────
# RENDER MARKDOWN

def render(cats, placa, fonte_cpl, fonte_bom):
    L = []
    L.append("# Questionário do Portão Humano ⟨dB⟩ — %s" % (placa or "placa"))
    L.append("")
    L.append("> Gerado por `portao_humano.py` v%s a partir de `%s` + `%s`. "
             "Método: `references/portao-humano.md`."
             % (VERSAO, os.path.basename(fonte_cpl or "cpl"),
                os.path.basename(fonte_bom or "bom")))
    L.append(">")
    L.append("> **REGRA DO PORTÃO:** um item de cada vez. O portão só avança "
             "com **✅ conforme**. Qualquer **❌** ou ambiguidade **PÁRA** e "
             "encaminha para um conserto — **nunca** para o pagamento. Um passe "
             "limpo de ponta a ponta é a precondição para o humano autorizar o "
             "pagamento (o pagamento é sempre acção do humano).")
    L.append("")
    n = 0        # numeração dos itens de inspeção (1..N); o Item 0 é o pré-passo
    total = 0    # total de itens renderizados (inclui o Item 0)
    for titulo, itens in cats:
        L.append("## %s" % titulo)
        L.append("")
        if not itens:
            L.append("_(nenhum item derivado desta placa nesta categoria.)_")
            L.append("")
            continue
        for it in itens:
            total += 1
            if it.get("num") is not None:   # item com número fixo (ex.: Item 0)
                disp = it["num"]
            else:
                n += 1
                disp = n
            L.append("### Item %d — %s" % (disp, it["item"]))
            L.append("- **ONDE OLHAR:** %s" % it["onde"])
            L.append("- **O QUE DEVES VER:** %s" % it["ver"])
            L.append("- **COMO CONFIRMAR:** %s" % it["confirmar"])
            L.append("- %s" % RESPOSTA)
            L.append("")
    L.append("---")
    L.append("_%d itens (o Item 0 é o carregamento do pacote, antes da inspeção). "
             "Enquanto houver um ❌ ou um campo em branco, o portão está FECHADO e o "
             "pagamento não é autorizado._" % total)
    L.append("")
    return "\n".join(L)


# ─────────────────────────────────────────────────────────────────────────────
# AUTO-TESTE --sabota (placa sintética; sem ficheiros)

def _placa_sintetica():
    cpl = [
        {"Ref": "U1", "Val": "OPA1642", "Package": "SOIC-8_3.9x4.9mm_P1.27mm",
         "PosX": "26", "PosY": "-92", "Rot": "0", "Side": "top"},
        {"Ref": "D11", "Val": "BAV99", "Package": "SOT-23",
         "PosX": "40", "PosY": "-36", "Rot": "0", "Side": "top"},
        {"Ref": "D1", "Val": "SMBJ24A", "Package": "D_SMB",
         "PosX": "166", "PosY": "-34", "Rot": "0", "Side": "top"},
        {"Ref": "D2", "Val": "SS14", "Package": "D_SMA",
         "PosX": "180", "PosY": "-34", "Rot": "0", "Side": "top"},
        {"Ref": "C1", "Val": "470u", "Package": "CP_Radial_D10.0mm_P5.00mm",
         "PosX": "55", "PosY": "-114", "Rot": "0", "Side": "top"},
        {"Ref": "Q1", "Val": "AO3400A", "Package": "SOT-23",
         "PosX": "52", "PosY": "-46", "Rot": "0", "Side": "top"},
        {"Ref": "U2", "Val": "Daisy Seed3", "Package": "Electrosmith_Daisy_Seed",
         "PosX": "113", "PosY": "-109", "Rot": "180", "Side": "top"},
        {"Ref": "J2", "Val": "OLED SSD1309 2,54mm 7p",
         "Package": "PinHeader_1x07_P2.54mm_Vertical",
         "PosX": "68", "PosY": "-24", "Rot": "90", "Side": "top"},
        {"Ref": "J1", "Val": "IDC 2x13 femea <- cerebro",
         "Package": "IDC-Header_2x13_P2.54mm_Vertical",
         "PosX": "26", "PosY": "-29", "Rot": "90", "Side": "top"},
    ]
    bom = [
        {"Designator": "U1", "Footprint": "Package_SO:SOIC-8", "LCSC": "C67640", "Qty": "1"},
        {"Designator": "D11", "Footprint": "Package_TO_SOT_SMD:SOT-23", "LCSC": "C2500", "Qty": "1"},
        {"Designator": "D1", "Footprint": "Diode_SMD:D_SMB", "LCSC": "C19077578", "Qty": "1"},
        {"Designator": "D2", "Footprint": "Diode_SMD:D_SMA", "LCSC": "C2480", "Qty": "1"},
        {"Designator": "C1", "Footprint": "Capacitor_THT:CP_Radial", "LCSC": "", "Qty": "1"},
        {"Designator": "Q1", "Footprint": "Package_TO_SOT_SMD:SOT-23", "LCSC": "C20917", "Qty": "1"},
        {"Designator": "U2", "Footprint": "Module:Electrosmith_Daisy_Seed", "LCSC": "", "Qty": "1"},
        {"Designator": "J2", "Footprint": "Connector_PinHeader_2.54mm:PinHeader_1x07", "LCSC": "", "Qty": "1"},
        {"Designator": "J1", "Footprint": "Connector_IDC:IDC-Header_2x13", "LCSC": "", "Qty": "1"},
    ]
    return cpl, bom


def sabota():
    print("portao_humano v%s — auto-teste --sabota" % VERSAO)
    cpl_rows, bom_rows = _placa_sintetica()
    pecas = carrega_cpl(cpl_rows)
    bom = carrega_bom(bom_rows)
    cats = constroi(pecas, bom, "SINTETICA", "2 camadas", "conforme", "5",
                    pacote="sintetica_2026-09-10_deadbee",
                    branch="claude/exemplo")
    texto = render(cats, "SINTETICA", "cpl", "bom")

    # cada classe conhecida TEM de aparecer no questionário
    checks = {
        "Item 0 abre com o carregamento do pacote": (
            "Item 0" in texto and "Carregar o pacote" in texto),
        "Item 0 nomeia o .zip do pacote": (
            "sintetica_2026-09-10_deadbee_gerbers.zip" in texto and ".zip" in texto),
        "Item 0 diz como buscar ao PR (terminal + fallback)": (
            "git pull" in texto and "Download raw file" in texto),
        "SOT-23 180° (D11 díodo)": ("D11" in texto and "180" in texto),
        "SOT-23 180° (Q1 transístor)": ("Q1" in texto and "180" in texto),
        "SMB cátodo (D1)": ("D1" in texto and "CÁTODO" in texto.upper()),
        "SMA cátodo (D2)": ("D2" in texto and "CÁTODO" in texto.upper()),
        "alias SS14↔1N5819 (D2)": ("1N5819" in texto and "SS14" in texto),
        "SOIC pino-1 (U1)": ("U1" in texto and "PINO 1" in texto.upper()),
        "electrolítico polaridade (C1)": ("C1" in texto and "electrolítico" in texto),
        "módulo orientação (U2)": ("U2" in texto and "chanfro" in texto),
        "OLED VCC/GND+DIN/CLK (J2)": ("J2" in texto and "VCC" in texto and "DIN/CLK" in texto),
        "todo conector pergunta o que liga do outro lado": (
            texto.count("o que liga aqui?") >= 2),
        "IDC 26/26 (J1)": ("J1" in texto and "26" in texto),
        "regra do portão (só ✅ avança)": ("nunca" in texto and "PÁRA" in texto),
        "confirmação final antes de pagar": ("precondição" in texto and "pagamento" in texto),
    }
    ok = True
    for nome, passou in checks.items():
        print("   %s %s" % ("✓" if passou else "*** FALHOU:", nome))
        ok = ok and passou
    print("   %s" % ("✓ o gerador assinala todas as classes de risco conhecidas"
                     if ok else "*** NÃO assinalou alguma classe — o gerador não passa"))
    return 0 if ok else 1


# ─────────────────────────────────────────────────────────────────────────────

def main(argv):
    ap = argparse.ArgumentParser(add_help=True, description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--cpl", help="ficheiro CPL csv (Ref,Val,Package,PosX,PosY,Rot,Side)")
    ap.add_argument("--bom", help="ficheiro BOM csv (Designator,Comment,Footprint,Qty,LCSC)")
    ap.add_argument("--placa", default="", help="nome da placa (para o título)")
    ap.add_argument("--camadas", default="", help="ex.: '2 camadas'")
    ap.add_argument("--dims", default="", help="dimensões/contorno (texto)")
    ap.add_argument("--qty", default="", help="quantidade esperada")
    ap.add_argument("--pacote", default="",
                    help="nome da pasta do pacote (<placa>_<data>_<commit>) para o "
                         "Item 0; se omitido, deriva-se da pasta do --out")
    ap.add_argument("--repo", default="Belquer/Cotucatronics",
                    help="owner/repo no GitHub (para o Item 0)")
    ap.add_argument("--branch", default="",
                    help="branch do PR onde o pacote está committado (para o Item 0)")
    ap.add_argument("--out", default="", help="escrever para ficheiro (default: stdout)")
    ap.add_argument("--sabota", action="store_true", help="auto-teste: prova que sabe assinalar")
    args = ap.parse_args(argv)

    print("portao_humano v%s" % VERSAO, file=sys.stderr)
    if args.sabota:
        return sabota()
    if not args.cpl or not args.bom:
        ap.error("faltam --cpl e/ou --bom (ou usa --sabota)")
    for c in (args.cpl, args.bom):
        if not os.path.exists(c):
            raise SystemExit("*** não há %s" % c)

    # o Item 0 nomeia o pacote: usa --pacote, ou deriva-o do nome da pasta do --out
    pacote = args.pacote
    if not pacote and args.out:
        pacote = os.path.basename(os.path.dirname(os.path.abspath(args.out)))

    pecas = carrega_cpl(_ler_csv(args.cpl))
    bom = carrega_bom(_ler_csv(args.bom))
    cats = constroi(pecas, bom, args.placa, args.camadas, args.dims, args.qty,
                    pacote=pacote, repo=args.repo, branch=args.branch)
    texto = render(cats, args.placa, args.cpl, args.bom)

    if args.out:
        with io.open(args.out, "w", encoding="utf-8") as f:
            f.write(texto)
        print("  escrito: %s (%d peças no CPL)" % (args.out, len(pecas)),
              file=sys.stderr)
    else:
        sys.stdout.write(texto)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
