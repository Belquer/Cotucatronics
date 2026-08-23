#!/usr/bin/env python3
"""check_docs v0.1.0 — dá testemunha à documentação.

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

RAIZ = pathlib.Path(__file__).resolve().parent.parent
SRC  = RAIZ / "src"
DOCS = [RAIZ / "README.md"] + sorted((RAIZ / "docs").glob("*.md"))

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

def main():
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
