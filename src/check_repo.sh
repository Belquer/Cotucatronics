#!/bin/bash
# check_repo v0.1.0 — o repo compila num CLONE LIMPO?
#
# Existe por um defeito real: module_rnbo_alloc.h foi incluido pelos
# tres modulos e NUNCA commitado. O build local sempre funcionou
# porque o arquivo estava na arvore; um clone novo quebrava. Varios
# commits foram empurrados assim.
#
# "Funciona na minha maquina" nao e descuido de quem esquece — e o
# resultado previsivel de conferir a arvore em vez do repositorio.
# Isto confere o repositorio.
#
# Uso:  ./check_repo.sh      ou      make check
# Saida: 0 se tudo rastreado, 1 se falta algo.

cd "$(dirname "$0")" || exit 2
echo "check_repo v0.1.0"
echo

fail=0

tracked() { git ls-files --error-unmatch "$1" >/dev/null 2>&1; }

# ---- 1. headers locais incluidos pelos fontes ----
echo "1. headers locais em #include \"...\""
for f in *.cpp *.h; do
    [ -f "$f" ] || continue
    # so os includes entre aspas, e so os que existem AQUI (os de
    # sistema e os do RNBO vem por -I e nao sao nossos)
    grep -ho '#include "[^"]*\.h"' "$f" 2>/dev/null \
        | sed 's/#include "//; s/"//' \
        | while read -r h; do
            [ -f "$h" ] || continue
            if ! tracked "$h"; then
                echo "   FALTA  $h  (incluido por $f)"
                echo "x" >> /tmp/.cotuca_check_fail
            fi
        done
done
[ -f /tmp/.cotuca_check_fail ] && fail=1 && rm -f /tmp/.cotuca_check_fail
[ "$fail" = 0 ] && echo "   ok — todos rastreados"

# ---- 2. o que o Makefile manda compilar ----
echo
echo "2. arquivos de CPP_SOURCES"
srcs=$(make -s print-cpp-sources 2>/dev/null | tail -1)
if [ -z "$srcs" ]; then
    echo "   AVISO: nao consegui ler CPP_SOURCES do Makefile"
else
    miss=0
    for s in $srcs; do
        base=$(basename "$s")
        # so confere os nossos; os da libDaisy vem de fora do repo
        [ -f "$base" ] || continue
        if ! tracked "$base"; then
            echo "   FALTA  $base"
            miss=1
        fi
    done
    [ "$miss" = 0 ] && echo "   ok — todos rastreados" || fail=1
fi

# ---- 3. exports do RNBO que os modulos incluem por caminho ----
echo
echo "3. exports do RNBO incluidos por caminho"
miss=0
for f in module_rnbo_*.cpp; do
    [ -f "$f" ] || continue
    grep -ho '#include "modules/[^"]*"' "$f" 2>/dev/null \
        | sed 's/#include "//; s/"//' | while read -r p; do
            if [ ! -f "$p" ]; then
                echo "   SUMIU  $p  (referenciado por $f)"
                echo "x" >> /tmp/.cotuca_check_fail
            elif ! tracked "$p"; then
                echo "   FALTA  $p  (referenciado por $f)"
                echo "x" >> /tmp/.cotuca_check_fail
            fi
        done
done
if [ -f /tmp/.cotuca_check_fail ]; then
    fail=1; rm -f /tmp/.cotuca_check_fail
else
    echo "   ok"
fi

echo
if [ "$fail" = 0 ]; then
    echo "PASSOU — um clone limpo tem tudo que o build precisa."
else
    echo "REPROVOU — falta arquivo no repositorio. git add antes de empurrar."
fi
exit $fail
