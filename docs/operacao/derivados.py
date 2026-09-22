#!/usr/bin/env python3
"""derivados v1.2.0 — "gerado" deixa de ser rotulo e passa a ser afirmavel.

O PROBLEMA QUE RESOLVE (05/09). O `manifest.h` ja era a fonte unica, e mesmo
assim o mapa de CC dava numeros diferentes em documentos diferentes. A causa
nao era o gerador: era o ROTULO. Tres ficheiros diziam-se "gerados de
manifest.h", ninguem os mantinha a mao por isso, e NENHUM GERADOR OS ESCREVIA.
Ficaram parados na versao de que vieram enquanto a fonte andava:

    CCS-POR-PAGINA.md   dizia vir de v0.85.0  · fw v0.178.0
    RETRABALHO-LOOPY.md dizia vir de            fw v0.156.0
    REMAP-ESCUTA.md     dizia vir de v0.171.0
    (a fonte estava em manifest v1.13.0 · fw v0.302.0)

> Ficheiro derivado por rotulo e o pior dos dois mundos: ninguem o escreve a
> mao porque "e gerado", e nada o regenera porque nao ha gerador. Apodrece
> com autoridade.

O QUE ISTO VERIFICA
  1. quem se diz derivado, e de que VERSAO da fonte;
  2. se essa versao bate com a fonte de hoje;
  3. se existe mesmo um gerador que o escreva  -> senao, ORFAO;
  4. derivado de derivado, que multiplica a distancia ate a fonte.

USO:  derivados.py
"""
import re, pathlib, sys
print("derivados v1.2.0")
RAIZ = pathlib.Path(__file__).resolve().parents[2]

def ler(p):
    try: return p.read_text(encoding="utf-8", errors="replace")
    except Exception: return ""

# ---- A FONTE UNICA, e a sua versao de hoje -------------------------------
MANIF = RAIZ / "src/manifest.h"
m = re.search(r'manifest\.h[^\n]*?v(\d+\.\d+\.\d+)', ler(MANIF))
VER_MANIF = m.group(1) if m else "?"
m = re.search(r'COTUCA_OS_VERSION\s+"([\d.]+)"', ler(RAIZ / "src/main.cpp"))
VER_FW = m.group(1) if m else "?"
print("  fonte unica: src/manifest.h v%s (firmware v%s)\n" % (VER_MANIF, VER_FW))

# ---- quem os geradores REALMENTE escrevem --------------------------------
# ⚠️ VARRER A ARVORE INTEIRA, NAO UMA PASTA COM UM PREFIXO. Ate 06/09 isto
# olhava so para `docs/operacao/gerar_*.py` e por isso acusou o COTUCA_BOM.md
# de "nenhum gerador o escreve" — quando o `tools/bom_completa.py` o escreve,
# noutra pasta e com outro nome. Verificador cujo ALCANCE e mais estreito do
# que a sua AFIRMACAO acusa inocentes, e e a mesma classe que ele persegue.
escritos = set()
import subprocess
try:
    pys = subprocess.run(["git", "ls-files", "*.py"], cwd=str(RAIZ),
                         capture_output=True, text=True, timeout=20).stdout.split()
except Exception:
    pys = []
for g in pys:
    for mm in re.finditer(r'"([A-Za-z0-9_.-]+\.(?:md|html))"', ler(RAIZ / g)):
        escritos.add(mm.group(1))

# ---- quem SE DIZ derivado ------------------------------------------------
DIZ = re.compile(r'\*\*GERAD[OA][^*]*\*\*|Gerad[oa] de|Gerad[oa] por|gerado por|nao editar a mao|não editar à mão', re.I)
achados = 0
for p in sorted((RAIZ / "docs").rglob("*.md")):
    cab = "\n".join(ler(p).splitlines()[:12])
    if not DIZ.search(cab): continue
    nome = p.name
    rel  = p.relative_to(RAIZ)
    # ⚠️ A VERSAO DO GERADOR NAO E A VERSAO DA FONTE. O LOOPY-SECAO.md
    # carimba "gerar_secao_loopy.py v0.3.0" — util, mas nao diz de que
    # manifest saiu. O carimbo que serve nomeia a FONTE, como o CCMAP:
    #     `src/manifest.h` @ <commit> · manifest v1.13.0
    mm2 = re.search(r'manifest v(\d+\.\d+\.\d+)', cab)
    if mm2:
        vers = [mm2.group(1)]
    else:
        # sem carimbo explicito: as versoes soltas que nao sejam de gerador
        vers = [v for v in re.findall(r'(?<!\.py )v(\d+\.\d+\.\d+)', cab)
                if not re.search(r'gerar_[a-z_]+\.py[^\n]{0,4}v' + re.escape(v), cab)]
    # ⚠️ E SE O GERADOR COMPOE O NOME? A varredura acima procura o nome do
    # .md como LITERAL dentro dos .py, e o `lista_roteamento.py` escreve
    #     "ROTEAMENTO-%s.md" % q.upper()
    # — nome que nunca aparece inteiro em lado nenhum. A 22/09 isto acusou
    # dois documentos correctos de orfaos. E a MESMA familia que o comentario
    # acima descreve: alcance mais estreito que a afirmacao.
    #
    # O documento NOMEIA o gerador dele. Isso e verificavel sem adivinhar:
    # basta o ficheiro existir. Uma afirmacao do documento contra o disco.
    nomeado = None
    for mg in re.finditer(r"`([\w./-]+\.py)`", cab):
        if (RAIZ / mg.group(1)).exists():
            nomeado = mg.group(1)
            break
    tem_ger = nome in escritos or nomeado is not None
    # derivado de outro DERIVADO?
    de_derivado = re.search(r'Gerad[oa] de `([A-Z][A-Za-z-]+\.md)`', cab)

    if not tem_ger:
        print("  🔴 ORFAO   %-34s diz-se gerado e NENHUM gerador o escreve" % rel)
        if vers: print("             versao(oes) que declara: %s · fonte hoje: %s" % (", ".join(vers), VER_MANIF))
        achados += 1
    # ⚠️ NEM TODO DERIVADO VEM DO MANIFEST. O COTUCA_BOM.md sai do KiCad e
    # carimba `commit <sha>`, que e a identidade certa para ele. Comparar
    # contra a versao do manifest era eu a assumir uma fonte unica que nao
    # vale para tudo — e a acusar um carimbo correcto de estar atrasado.
    elif re.search(r'commit `?[0-9a-f]{7,}', cab):
        print("  ✅ %-38s carimbado por commit%s" % (rel,
              " (gerador: %s)" % nomeado if nomeado and nome not in escritos
              else " (nao deriva do manifest)"))
    elif VER_MANIF not in vers and vers:
        print("  🟡 ATRASADO %-33s declara %s · fonte hoje v%s" % (rel, ", ".join("v"+v for v in vers), VER_MANIF))
        achados += 1
    elif not vers:
        print("  🟡 SEM CARIMBO %-30s diz-se gerado e nao diz de que versao" % rel)
        achados += 1
    else:
        print("  ✅ %-38s v%s, a bater com a fonte" % (rel, VER_MANIF))

    if de_derivado:
        print("             ⚠️ derivado de OUTRO DERIVADO (%s) — a distancia a fonte multiplica" % de_derivado.group(1))

print("\n  %d achado(s)." % achados)
print("  Regra: ficheiro derivado ou tem gerador e carimbo da versao da fonte,")
print("  ou nao se diz derivado. Rotulo sem gerador apodrece com autoridade.")
sys.exit(1 if achados else 0)
