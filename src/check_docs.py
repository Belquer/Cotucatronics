#!/usr/bin/env python3
"""check_docs — dá testemunha à documentação (versão em VERSION).

O compilador cobra os símbolos; nada cobrava a doc. Depois do audit de
deriva de 22/08 (doze achados, ver bts 13h), isto passa a cobrar.

PRINCÍPIO: o manifest é a única fonte de verdade. Tudo que fala sobre
ele é verificado contra ele.

  fantasma  = nome com cara de param citado na doc e inexistente no
              manifest  -> ERRO, quebra o build
  CC errado = número citado que não bate com o mapa  -> ERRO
  órfão     = `static` com <=2 ocorrências no fonte  -> AVISO

Exceção só DECLARADA, na própria linha ou na anterior:
  <!-- doc-check: ignore nome (motivo) -->
Fantasma declarado deixa de ser fantasma; fantasma silencioso quebra.

CUIDADO DE IMPLEMENTAÇÃO (regra 3 da 12e): isto casa padrão em TEXTO.
Imprime o que casou antes de falhar — senão vira o verificador que
acusa a frase que documenta a remoção, que foi a 12c.
"""
import re, sys, pathlib

# A versao mora AQUI e e impressa na primeira linha da saida. Ela ja
# viveu so no docstring, onde envelheceu de 0.1.0 a 0.3.1 sem ser
# corrigida — porque nunca aparecia em lugar nenhum.
VERSION = "0.6.0"

RAIZ = pathlib.Path(__file__).resolve().parent.parent
SRC  = RAIZ / "src"
# rglob, NAO glob: `docs/operacao/` chegou com sete documentos e o glob
# raso nao via nenhum. Pasta nova entra sem vigilancia por padrao, e o
# verificador que nao alcanca um documento diz "ok" sobre ele — que e a
# forma mais silenciosa desta familia (verificacao sem objeto PASSA, e
# ninguem desconfia de um ok).
DOCS = [RAIZ / "README.md"] + sorted((RAIZ / "docs").rglob("*.md"))

def faixas():
    """min/max de cada param, pra conferir FAIXAS citadas na prosa.

    A LACUNA QUE O AUDIT DE 22/08 DEIXOU ABERTA. O item de backlog do
    `frz_size` dizia "faixa 20-400" quando o manifest declarava 20-165 —
    e o nome estava certo, entao o verificador de fantasmas passou por
    ele. Nome confere; NUMERO nao conferia.

    Confere so quando param e faixa aparecem na MESMA linha: prosa esta
    cheia de numeros, e conferir todos seria falso-positivo garantido.
    """
    m = (SRC / "manifest.h").read_text(errors="replace")
    out = {}
    for nome, mi, ma in re.findall(
            r't\.Add\("([a-z_0-9]+)",\s*[-\d.f]+,\s*([-\d.]+)f?,\s*([-\d.]+)f?',
            m):
        out[nome] = (float(mi), float(ma))
    return out

def verdade():
    m = (SRC / "manifest.h").read_text(errors="replace")
    params = set(re.findall(r't\.Add\("([a-z_0-9]+)"', m))
    ccs = {}
    for num, alvo in re.findall(r'AddMidi\((\d+),\s*([A-Za-z_0-9\[\]]+)\)', m):
        ccs[int(num)] = alvo
    return params, ccs

def isentos(texto):
    """nomes declarados como proposta/removido, na linha ou na anterior"""
    fora = set()
    for mm in re.finditer(r'<!--\s*doc-check:\s*ignore\s+([a-z_0-9]+)', texto):
        fora.add(mm.group(1))
    return fora

def simbolos_do_fonte():
    """Todo identificador que existe no fonte.

    A REGRA QUE MATA A CLASSE INTEIRA DOS FALSOS POSITIVOS: se o nome
    EXISTE no codigo, cita-lo e legitimo. `sdram_pool`, `eco_ult`,
    `reverb_mem` nao sao params, e a doc fala deles com razao.

    Fantasma e nome que nao existe em lugar NENHUM — nem param, nem
    simbolo, nem aposentado declarado.
    """
    vistos = set()
    for p in list(SRC.glob("*.cpp")) + list(SRC.glob("*.h")):
        vistos |= set(re.findall(r'\b([a-z][a-z0-9]*(?:_[a-z0-9]+)+)\b',
                                 p.read_text(errors="replace")))
    return vistos

def aposentados():
    """Params que EXISTIRAM e sairam. Citalos na historia e legitimo.

    Sem esta lista, toda cicatriz que narra uma remocao vira fantasma —
    que e a 12c em forma de ferramenta: o verificador acusando a frase
    que documenta o conserto.
    """
    arq = RAIZ / "docs" / "params-aposentados.txt"
    if not arq.exists(): return set()
    fora = set()
    for linha in arq.read_text(errors="replace").splitlines():
        linha = linha.strip()
        if not linha or linha.startswith("#"): continue
        fora.add(linha.split()[0])
    return fora

def comandos_sem_ajuda():
    """Comando tratado que nao aparece na linha do `?`.

    Comando que nao consta e comando que NAO EXISTE para quem esta na
    bancada com a serial aberta — a descoberta e pela linha de ajuda, nao
    pela leitura do fonte. Tres apareceram assim no mesmo dia (`dup`,
    `espelho`, `dreno`), e o padrao e sempre o mesmo: escreve-se o
    handler, testa-se chamando pelo nome, e a linha de ajuda fica.
    """
    src = (SRC / 'main.cpp').read_text(encoding='utf-8', errors='replace')
    m = re.search(r'hal::Log\("\? ([^"]*)"\)', src)
    if not m:
        return None, []          # sem linha de ajuda: quem chama decide
    ajuda = m.group(1)
    tratados = set(re.findall(r'strncmp\(line,\s*"([a-z0-9]+)', src))
    # `k1`..`k5` e `b2`/`b3` entram por outro caminho e ja estao na linha
    # sob outra forma; o resto tem que constar literalmente.
    # ISENCAO MINIMA, e ela ja esteve larga demais: `b2` e `b3` estavam
    # aqui e sao comandos de verdade (pulso de freeze e de rearme). Uma
    # isencao generosa transforma o verificador em carimbo. Ficam so os
    # `k1..k5`, que a linha ja anuncia como faixa, e os dois `midi*`, que
    # ela lista com o nome inteiro.
    isento = {'k1', 'k2', 'k3', 'k4', 'k5', 'midiloop', 'miditest'}
    faltam = sorted(c for c in tratados
                    if c not in isento and c not in ajuda)
    return ajuda, faltam


def main():
    print(f"check_docs v{VERSION}")
    params, ccs = verdade()
    simb = simbolos_do_fonte()
    apos = aposentados()
    # FAMILIA: `gov_ceiling` citado quando existem gov_ceiling_a/b.
    # Sem isto o verificador acusa o nome da familia, que e como a doc
    # fala dela — e falar da familia e mais legivel que listar as duas.
    familias = {re.sub(r'_(a|b)$', '', p) for p in params}
    print(f"manifest: {len(params)} params ({len(familias)} familias),"
          f" {len(ccs)} CCs | fonte: {len(simb)} simbolos |"
          f" aposentados: {len(apos)}")
    erros, avisos = [], []

    for doc in DOCS:
        if not doc.exists(): continue
        texto = doc.read_text(errors="replace")
        livres = isentos(texto)
        # nome com cara de param: em crase, minusculo, com _ , 2+ partes
        for mm in re.finditer(r'`([a-z][a-z0-9]*(?:_[a-z0-9]+)+)`', texto):
            nome = mm.group(1)
            if (nome in params or nome in familias or nome in livres
                    or nome in simb or nome in apos):
                continue
            linha = texto[:mm.start()].count("\n") + 1
            erros.append((doc.name, linha, nome,
                          texto.split("\n")[linha-1].strip()[:70]))

    ajuda, sem_ajuda = comandos_sem_ajuda()
    if ajuda is None:
        print("\nAVISO: nao achei a linha de ajuda `?` — checagem sem objeto,"
              " que e pior que checagem ausente. Conferir o regex.")
    else:
        print(f"\nCOMANDOS SEM AJUDA: {len(sem_ajuda)}")
        for c in sem_ajuda:
            print(f"  `{c}` e tratado e nao aparece no `?`")

    print(f"\nFANTASMAS: {len(erros)}")
    for d, l, n, ctx in erros[:40]:
        print(f"  {d}:{l}  `{n}`")
        print(f"      | {ctx}")

    # órfãos no fonte
    for f in sorted(SRC.glob("*.cpp")) + sorted(SRC.glob("*.h")):
        t = f.read_text(errors="replace")
        for mm in re.finditer(r'^static\s+(?:const\s+)?[A-Za-z_][\w:*&<> ]*?\b(\w+)\s*(?:\[[^\]]*\])?\s*=', t, re.M):
            nome = mm.group(1)
            n = len(re.findall(r'\b' + re.escape(nome) + r'\b', t))
            if n <= 2:
                avisos.append((f.name, nome, n))
    # ---- FAIXAS: nome certo, numero errado ----
    #
    # A LACUNA QUE O AUDIT DE 22/08 DEIXOU: o item do `frz_size` dizia
    # "faixa 20-400" com o manifest em 20-165. O NOME estava certo, entao
    # o verificador de fantasmas passou por ele.
    #
    # SO ONDE A DOC DECLARA A FAIXA COM A PALAVRA. Duas tentativas mais
    # amplas foram descartadas por falso-positivo: casar qualquer par de
    # numeros na linha acusou o `drv_sujeira` pelo "1,5-48x" do
    # `drv_drive`, e o `dly_time` por "60-64", que e faixa de CC. Os
    # numeros da doc sao majoritariamente CC e versao — conferir todos
    # seria acusar o inocente, e verificador que acusa inocente perde a
    # autoridade que o torna util (12c).
    fx = faixas()
    ruins = []
    padrao = re.compile(
        r'faixa\s+(\d+(?:[.,]\d+)?)\s*[-\u2013a]+\s*(\d+(?:[.,]\d+)?)',
        re.I)
    for doc in DOCS:
        if not doc.exists(): continue
        for ln, linha in enumerate(doc.read_text(errors="replace").split("\n"), 1):
            if "doc-check: ignore" in linha: continue
            mm = padrao.search(linha)
            if not mm: continue
            citados = {x for x in re.findall(r'`([a-z][a-z0-9_]+)`', linha)
                       if x in fx or (x + "_a") in fx}
            if len(citados) != 1: continue
            nome = citados.pop()
            base = nome if nome in fx else nome + "_a"
            mi, ma = fx[base]
            a = float(mm.group(1).replace(",", "."))
            b = float(mm.group(2).replace(",", "."))
            if (a, b) != (mi, ma):
                ruins.append((doc.name, ln, nome, a, b, mi, ma,
                              linha.strip()[:66]))
    print(f"\nFAIXAS DECLARADAS QUE NAO BATEM: {len(ruins)}")
    for d, l, n, a, b, mi, ma, ctx in ruins[:12]:
        print(f"  {d}:{l}  `{n}` diz {a:g}-{b:g}, manifest diz {mi:g}-{ma:g}")
        print(f"      | {ctx}")
    erros += [(d, l, n, ctx) for d, l, n, _, _, _, _, ctx in ruins]

    print(f"\nORFAOS (aviso, exige julgamento): {len(avisos)}")
    for f, n, c in avisos[:20]:
        print(f"  {f}: {n} ({c} ocorrencias)")

    if erros:
        print(f"\nFALHOU: {len(erros)} fantasma(s). Corrija, ou declare com")
        print("  <!-- doc-check: ignore nome (motivo) -->")
        return 1
    print("\nok — nenhum fantasma")
    return 0

sys.exit(main())
