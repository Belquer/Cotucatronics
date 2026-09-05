#!/bin/bash
# hook_commit v1.0.0 — os verificadores BARATOS correm antes do commit.
#
# POR QUE EXISTE, e a razao NAO e "prevenir um push mau".
#
# O `hook_stop.sh` ja apanhava isto: corre o /audit no fecho da sessao. O que
# ele nao consegue e apanha-lo A TEMPO. A 05/09 bumpei o hal_daisy_seed para
# v0.75.0 e nao actualizei a linha dele no mapa do README; o audit acusou —
# no commit SEGUINTE, com o mau ja empurrado.
#
# O ganho nao e evitar o erro. E o erro chegar ENQUANTO EU AINDA TENHO O
# CONTEXTO. No fecho da sessao, consertar significa reabrir ficheiros de que
# ja me esqueci; no momento do commit, conserta-se no mesmo folego.
#
# ⚠️ AVISA, NUNCA BLOQUEIA. Sai sempre 0, como o hook_stop — um portao que
# impede commitar e um portao que se desliga na primeira vez que estorva, e
# ai deixa de haver verificacao nenhuma.
#
# ⚠️ E SO OS BARATOS. Medido a 05/09:
#     versoes_readme  0,03 s     desenhos  0,02 s
#     check_docs      1,50 s     auditar   3,26 s
# Os dois primeiros custam 50 ms e correm aqui. Os pesados ficam onde ja
# estao, no fecho: verificacao cara em cada commit e verificacao que se
# acaba por remover.
cd "$(dirname "$0")/../.." || exit 0

CMD=$(python3 -c 'import json,sys
try: print(json.load(sys.stdin).get("tool_input",{}).get("command",""))
except Exception: print("")' 2>/dev/null)

case "$CMD" in *"git commit"*) ;; *) exit 0 ;; esac

AVISOS=""
if ! OUT=$(python3 docs/operacao/versoes_readme.py 2>/dev/null); then
  AVISOS="$AVISOS
$(echo "$OUT" | grep -E 'DIVERGE|AUSENTE' | head -5)"
fi
if ! OUT=$(python3 tools/desenhos.py 2>/dev/null); then
  AVISOS="$AVISOS
$(echo "$OUT" | grep -E '✗' | head -5)"
fi

[ -z "$(echo "$AVISOS" | tr -d '[:space:]')" ] && exit 0

MSG="⚠️  antes deste commit:
$AVISOS

  versoes:  python3 docs/operacao/versoes_readme.py
  desenhos: python3 tools/desenhos.py --gerar
(aviso, nao bloqueio — o commit segue)"
python3 -c 'import json,sys; print(json.dumps({"systemMessage": sys.stdin.read().strip()}))' <<< "$MSG"
exit 0
