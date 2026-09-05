#!/bin/bash
# hook_stop v1.0.0 — o /audit corre sozinho no fim da sessao, e AVISA.
#
# POR QUE EXISTE. O battle_scars ja tinha a regra escrita — "habito
# automatizado vence habito lembrado" — e todos os verificadores desta arvore
# dependiam de alguem se lembrar: check_docs, auditar, derivados, vizinhos,
# versoes_readme. Quem os corria era a disciplina de quem estava ao teclado.
#
# AVISA, NUNCA BLOQUEIA. Sai sempre 0. Um portao que impede fechar a sessao e
# um portao que se desliga na primeira vez que estorva, e ai deixa de haver
# verificacao nenhuma. Este diz o que achou e sai da frente.
#
# ⚠️ A CONTAGEM E POR SECCAO, e a razao importa: a passada de ITENS PENDENTES
# lista 🔴/🟡 que vivem DENTRO dos documentos — sao o backlog do dB, nao
# defeitos. Conta-los daria um alarme permanente, e alarme permanente e ruido.
# So contam as seccoes estruturais: links mortos, isencoes, derivados,
# destinos dessincronizados, check_docs e o mapa de versoes.
cd "$(dirname "$0")/../.." || exit 0

OUT=$(./docs/operacao/auditar.sh 2>/dev/null) || true

RESUMO=$(printf '%s\n' "$OUT" | awk '
  /^== / { sec = $0; next }
  sec ~ /ITENS PENDENTES/ { next }                 # backlog, nao defeito
  /LINKS MORTOS/ { next }
  sec ~ /LINKS MORTOS/ && /->/            { n["links mortos"]++ }
  sec ~ /ISENCOES/ && /isenta/            { n["isencoes orfas"]++ }
  sec ~ /DERIVADOS/ && /ORFAO/            { n["derivados sem gerador"]++ }
  sec ~ /DERIVADOS/ && /ATRASADO|SEM CARIMBO/ { n["derivados sem carimbo"]++ }
  sec ~ /DESSINCRONIZADOS/ && /difere/    { n["repo != vault"]++ }
  sec ~ /VERSOES/ && /DIVERGE|AUSENTE/    { n["versoes do README"]++ }
  /FALHOU|fantasma\(s\)/                  { n["fantasmas"]++ }
  END { for (k in n) printf "%s: %d · ", k, n[k] }')

if [ -z "$RESUMO" ]; then
  exit 0                                   # nada a dizer, e silencio e correcto
fi

MSG="/audit no fecho — ${RESUMO%· }
correr  ./docs/operacao/auditar.sh  para o detalhe"
python3 -c 'import json,sys; print(json.dumps({"systemMessage": sys.stdin.read().strip()}))' <<< "$MSG"
exit 0
