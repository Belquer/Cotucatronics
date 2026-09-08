#!/usr/bin/env python3
"""mpn_nas_folhas — o MPN passa a viver NAS FOLHAS, e nao so numa tabela minha.

POR QUE EXISTE.

Os MPN viviam numa tabela dentro do `tools/bom_completa.py`. Funcionava — e
escondia que **o esquema nao sabia deles**. Quando entrou uma revisao de fora
(`kicad-happy`), ela leu as folhas e disse a verdade:

    ERROR SS-001: BOM has <50% MPN coverage (0/36 unique parts).
                  Board is not pre-fab ready.

Zero. E tem razao: quem manda os ficheiros para uma fabrica que MONTA precisa
que o MPN esteja no que se exporta, e o que se exporta sai das folhas. E a §12w
da skill outra vez — *uma regra que vive dentro de uma ferramenta so vale para
essa ferramenta*, agora aplicada ao numero de peca em vez da largura de pista.

⚠️ E DUAS FOLHAS NAO TEM GERADOR. A `energia` e o `preamp` foram feitas por
geradores que ja nao existem. A regra da casa e "nunca editar o `.kicad_sch` a
mao" (§8) e nao ha `.py` onde a cumprir — por isso o conserto nao e editar: e
esta ferramenta, que **carimba a partir da fonte, e se pode voltar a correr**
depois de qualquer regeneracao. Um script idempotente nao e uma edicao a mao.

⚠️ A FONTE CONTINUA A SER UMA SO. O `PECAS` do `bom_completa` manda; as folhas
sao carimbadas a partir dele. Com `--conferir` esta ferramenta diz se alguma
folha se afastou — porque uma copia sem verificador volta a divergir, e foi
assim que a `BOM-MPN-revB.md` morreu.

USO:  mpn_nas_folhas.py            carimba
      mpn_nas_folhas.py --conferir so verifica (sai != 0 se divergir)
"""
import io
import os
import re
import sys

VERSAO = "0.3.1"
AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, os.path.join(RAIZ, "tools"))
from bom_completa import MPN, MPN_PAINEL, MPN_POR_FP, PECAS, PECAS_PAINEL  # noqa: E402,F401

FOLHAS = [
    ("cerebro", "cotuca_cerebro/cotuca_energia.kicad_sch"),
    ("cerebro", "cotuca_cerebro/cotuca_preamp.kicad_sch"),
    ("cerebro", "cotuca_cerebro/cotuca_soquete.kicad_sch"),
    ("cerebro", "cotuca_cerebro/cotuca_conector.kicad_sch"),
    ("painel",  "cotuca_painel/cotuca_painel.kicad_sch"),
]


def peca(placa, ref, fp):
    """(MPN, fornecedor) — e o MPN e o NUMERO DE PECA, nao a descricao.

    ⚠️ A primeira versao lia a coluna de descricao do `bom_completa`, que
    mistura numero com prosa. As folhas ficaram com `MPN = "header 1x4 2,54"` e
    a revisao de fora disse "50/50 parts have MPNs", porque so verifica que o
    campo nao esta vazio. So se viu quando uma ferramenta tentou USAR o campo:
    o descarregador de fichas foi procurar "header 1x4 2,54" na LCSC.

    ⚠️ E O VAZIO E UMA RESPOSTA. Um resistor 0805 de 1% nao tem MPN. Deixar o
    campo vazio e honesto; enche-lo com a descricao e uma cobertura falsa."""
    tab = MPN_PAINEL if placa == "painel" else MPN
    forn = (PECAS_PAINEL if placa == "painel" else PECAS).get(ref, ("", ""))[1]
    if ref in tab:
        return tab[ref], forn
    if fp in MPN_POR_FP:
        return MPN_POR_FP[fp], forn
    return "", ""


def limpa(s):
    """⚠️ O `**` do markdown nao entra num campo do KiCad. A tabela usa-o para
    destacar na BOM; a folha leva o texto nu."""
    return s.replace("**", "").strip()


def uma(caminho, placa, conferir):
    p = os.path.join(AQUI, caminho)
    t = io.open(p, encoding="utf-8").read()

    # ⚠️ conta os simbolos COLOCADOS, e nao os do `lib_symbols`: os de
    # biblioteca nao tem designador e carimba-los seria escrever peca numa
    # definicao em vez de numa instancia.
    corpo = t
    m = re.search(r'\n\t\(lib_symbols\b', t)
    if m:
        d, i = 0, m.start() + 1
        for k in range(i, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    break
        corpo = t[:m.start()] + t[k + 1:]
    # ⚠️ A GUARDA CONTA O MESMO UNIVERSO QUE O LEITOR (§2c): se ela contasse os
    # pontos de teste e o leitor os saltasse de proposito, dava alarme falso —
    # e guarda que da alarme falso e guarda que se desliga.
    esperados = len(re.findall(r'\(property "Reference" "(?!#)(?!TP\d)[^"]*"', corpo))

    postos, faltam, divergem, n = [], [], [], 0
    saida, i0 = "", 0
    for m in re.finditer(r'\(property "Reference" "([^"]+)"', t):
        ref = m.group(1)
        if ref.startswith("#") or re.match(r"TP\d+$", ref):
            continue                     # PWR_FLAG e pontos de teste: cobre
        # o bloco do simbolo que contem esta propriedade
        j = t.rfind("(symbol", 0, m.start())
        d = 0
        for k in range(j, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    break
        bl = t[j:k + 1]
        if '(lib_id' not in bl:
            continue
        n += 1
        fpm = re.search(r'\(property "Footprint" "([^"]*)"', bl)
        fp = fpm.group(1) if fpm else ""
        # ⚠️ UM PONTO DE TESTE NAO TEM MPN, e nao e uma falta. E cobre: o
        # footprint traz `exclude_from_bom`, nao entra no CPL nem na lista de
        # compras, e nao ha nada para encomendar. Conta-lo como "sem MPN" fazia
        # a cobertura cair de 99% para 80% sem nenhuma peca a faltar — e uma
        # metrica que piora quando o desenho melhora esta a medir a coisa
        # errada.
        if fp.startswith("TestPoint:"):
            continue
        mpn, forn = peca(placa, ref, fp)
        mpn, forn = limpa(mpn), limpa(forn)
        ja = re.search(r'\(property "MPN" "([^"]*)"', bl)
        if not mpn:
            faltam.append(ref)
            # ⚠️ e se a folha tinha um MPN que a fonte ja nao tem, TIRA-SE. Um
            # campo que sobrevive a fonte que o justificava e uma afirmacao sem
            # dono — e foi isso que a primeira versao deixou lá.
            if ja and not conferir:
                saida += t[i0:j] + re.sub(
                    r'\t\t\(property "(MPN|Fornecedor)"[\s\S]*?\n\t\t\)\n', "", bl)
                i0 = k + 1
            continue
        if ja and ja.group(1) == mpn:
            postos.append(ref)
            continue
        if ja:
            divergem.append((ref, ja.group(1), mpn))
        if conferir:
            continue
        novo = bl
        campos = ('\t\t(property "MPN" "%s"\n\t\t\t(at 0 0 0)\n\t\t\t(hide yes)\n'
                  '\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n'
                  '\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n'
                  '\t\t\t\t)\n\t\t\t)\n\t\t)\n' % mpn)
        if forn:
            campos += ('\t\t(property "Fornecedor" "%s"\n\t\t\t(at 0 0 0)\n'
                       '\t\t\t(hide yes)\n\t\t\t(show_name no)\n'
                       '\t\t\t(do_not_autoplace no)\n\t\t\t(effects\n'
                       '\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n'
                       '\t\t\t)\n\t\t)\n' % forn)
        if ja:
            novo = re.sub(r'\t\t\(property "(MPN|Fornecedor)"[\s\S]*?\n\t\t\)\n',
                          "", novo)
        dm = re.search(r'\t\t\(property "Datasheet"[\s\S]*?\n\t\t\)\n', novo)
        if not dm:
            faltam.append(ref)
            continue
        novo = novo[:dm.end()] + campos + novo[dm.end():]
        saida += t[i0:j] + novo
        i0 = k + 1
        postos.append(ref)
    t2 = saida + t[i0:]

    if n != esperados:
        raise SystemExit("*** %s: a folha tem %d simbolos com designador e a "
                         "leitura apanhou %d — LEITURA INCOMPLETA"
                         % (caminho, esperados, n))
    if not conferir and i0:
        io.open(p, "w", encoding="utf-8").write(t2)
    return n, postos, faltam, divergem


def main():
    conferir = "--conferir" in sys.argv
    print("mpn_nas_folhas v%s%s" % (VERSAO, "  (so confere)" if conferir else ""))
    tot, tot_ok, tot_falta, mau = 0, 0, [], 0
    for placa, caminho in FOLHAS:
        n, postos, faltam, divergem = uma(caminho, placa, conferir)
        tot += n
        tot_ok += len(postos)
        tot_falta += faltam
        print("  %-34s %2d simbolo(s) · %2d com MPN · %2d sem"
              % (os.path.basename(caminho), n, len(postos), len(faltam)))
        for ref, tinha, devia in divergem:
            print("     %s %s: a folha dizia %r · a fonte diz %r"
                  % ("⚠️" if conferir else "acertado", ref, tinha, devia))
            # ⚠️ divergencia so e FALTA quando nao se conserta. A v0.3.0 contava
            # como falta o que ela propria tinha acabado de carimbar, e saia com
            # erro a mandar "corre sem --conferir" — que era o que estava a
            # fazer. Um relatorio que reprova o proprio conserto ensina a
            # ignorar o relatorio.
            if conferir:
                mau += 1
    print("  TOTAL: %d de %d com MPN (%.0f%%)"
          % (tot_ok, tot, 100.0 * tot_ok / max(tot, 1)))
    if tot_falta:
        print("  sem MPN, e cada um e uma decisao por tomar:")
        print("     %s" % " ".join(sorted(set(tot_falta))))
    if mau:
        raise SystemExit("*** %d simbolo(s) divergem da fonte. Corre sem "
                         "--conferir para os acertar." % mau)
    return 0


if __name__ == "__main__":
    sys.exit(main())
