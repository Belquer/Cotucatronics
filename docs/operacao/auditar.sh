#!/bin/bash
# auditar v1.3.0 — varredura mecanica de inconsistencia na documentacao.
#
# So reporta o que consegue PROVAR. As classes de julgamento (declaracao
# falsa, gap, numero morto que passa por actual) nao sao detectaveis por
# grep e estao na skill `audit`, nao aqui. Um verificador que finge cobrir
# o que nao cobre e pior que verificador nenhum — ver "verificacao sem
# objeto" no cap. 13 do battle_scars.
#
# USO:  ./auditar.sh            varredura geral
#       ./auditar.sh <termo>    + inventario de um termo nos tres destinos
set -uo pipefail
echo "auditar v1.3.0"

cd "$(dirname "$0")/../.." || exit 1
RAIZ=$(pwd)
VAULT="$HOME/Documents/Obsidian Vault/Cotucatronics"
TERMO="${1:-}"
ACHADOS=0
nota() { echo "  $*"; ACHADOS=$((ACHADOS+1)); }

DOCS=$(find README.md docs .claude/skills -name '*.md' -o -name '*.txt' 2>/dev/null | sort)

echo
echo "== 1. LINKS MORTOS (ficheiro referido que nao existe) =="
for f in $DOCS; do
  d=$(dirname "$f")
  grep -o '](\([^)#]*\.\(md\|txt\|sh\|py\|h\|cpp\|command\)\))' "$f" 2>/dev/null \
  | sed 's/^](//; s/)$//' | sort -u | while read -r alvo; do
      case "$alvo" in http*) continue;; esac
      [ -f "$RAIZ/$alvo" ] || [ -f "$d/$alvo" ] || echo "  $f -> $alvo"
    done
done

echo
echo "== 2+3. ISENCOES E LISTA DE APOSENTADOS (criterio do check_docs) =="
# Estava ERRADO em bash: contava ocorrencias simples quando o check_docs so
# ve nomes ENTRE CRASES, e supunha escopo de linha quando `isentos()` varre o
# ficheiro inteiro. Passou para o coerencia.py, que usa o criterio certo.
if [ -f docs/operacao/coerencia.py ]; then
  python3 docs/operacao/coerencia.py | tail -n +2 | sed 's/^/  /'
fi

echo
echo "== 3c. DERIVADOS: o rotulo \"gerado\" e verdade? =="
if [ -f docs/operacao/derivados.py ]; then
  python3 docs/operacao/derivados.py | tail -n +2 | sed 's/^/  /'
fi

echo
echo "== 4. DESTINOS DESSINCRONIZADOS (repo vs vault) =="
if [ -d "$VAULT" ]; then
  for f in "$VAULT"/*.md "$VAULT"/*.txt; do
    [ -e "$f" ] || continue
    b=$(basename "$f")
    [ "$b" = "README.md" ] && continue
    for c in "docs/$b" "docs/operacao/$b"; do
      if [ -f "$c" ] && ! cmp -s "$f" "$c"; then
        echo "  $b: vault difere de $c"
      fi
    done
  done
  cmp -s "$VAULT/README.md" README.md 2>/dev/null || \
    { [ -f "$VAULT/README.md" ] && echo "  README.md: vault difere da raiz"; }
else
  echo "  (vault nao encontrado em $VAULT)"
fi

echo
echo "== 5. check_docs (fonte de verdade: o manifest) =="
if [ -f src/check_docs.py ]; then
  python3 src/check_docs.py 2>&1 | grep -E 'FANTASMAS|CC errado|FAIXAS|ok —|ERRO' | sed 's/^/  /'
  python3 src/check_docs.py >/dev/null 2>&1 && echo "  exit 0" || echo "  *** exit NAO-ZERO ***"
fi

echo
echo "== 6. VERSOES DO MAPA DO README vs FICHEIROS =="
if [ -f docs/operacao/versoes_readme.py ]; then
  python3 docs/operacao/versoes_readme.py | tail -n +2 | sed 's/^/  /'
fi

echo
echo "== 7. FRASES QUE A RENOMEACAO PODE TER PARTIDO =="
if [ -f docs/operacao/vizinhos.py ]; then
  if [ -n "$TERMO" ]; then
    python3 docs/operacao/vizinhos.py --termo="$TERMO" | tail -n +2 | sed 's/^/  /'
  else
    python3 docs/operacao/vizinhos.py | tail -n +2 | tail -4 | sed 's/^/  /'
    echo "  (passe um termo para apertar a rede: ./auditar.sh <termo>)"
  fi
fi

if [ -n "$TERMO" ]; then
  echo
  echo "== 8. INVENTARIO EXAUSTIVO DE '$TERMO' =="
  # `git ls-files`, NUNCA um glob a mao. Em 2026-09-04 varri o fonte com
  # `*.h *.cpp Makefile` — 33 ficheiros — e dei o trabalho por fechado. A
  # arvore tem 137 rastreados em 14 extensoes; ficaram de fora .py, .sh,
  # .command, .lds, .ino, .html e um SEGUNDO Makefile. O check_repo.sh
  # escapou e so apareceu na varredura final. Enumerar e do git; escolher
  # a mao e como se esquece um tipo de ficheiro inteiro.
  echo "  -- rastreados pelo git --"
  git ls-files -z | xargs -0 grep -lI -i -- "$TERMO" 2>/dev/null \
    | while read -r f; do printf '  %5s  %s\n' "$(grep -ci -- "$TERMO" "$f")" "$f"; done
  echo "  -- novos, ainda nao rastreados --"
  git ls-files --others --exclude-standard -z \
    | xargs -0 grep -lI -i -- "$TERMO" 2>/dev/null | sed 's/^/       /'
  echo "  -- vault (fora do git) --"
  [ -d "$VAULT" ] && grep -rlI -i -- "$TERMO" "$VAULT"/ 2>/dev/null | sed 's|.*/|       |'
  echo "  -- cobertura: extensoes na arvore --"
  git ls-files | sed -n 's/.*\.\([a-zA-Z0-9]*\)$/\1/p' | sort -u | tr '\n' ' ' | sed 's/^/       /'
  echo
fi

echo
echo '-- fim. As classes de JULGAMENTO nao estao aqui: ver a skill audit --'
