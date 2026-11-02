#!/bin/bash
# ccmap.command v0.1.0 — envolucro clicavel do gerar_ccmap.py.
#
# ⚠️ NAO escrever outro gerador. Este chama `gerar_ccmap.py`, que existe
# desde 2026-08-28 e escreve os DOIS destinos (CCMAP.md e o cartao ccmap.html).
# Em 2026-09-04 escrevi um segundo gerador sem procurar o primeiro — e o proprio
# CCMAP.md dizia, na terceira linha, qual era o certo.
cd "$(dirname "$0")/../docs/operacao"
python3 gerar_ccmap.py && echo && head -12 CCMAP.md
read -p "Press Enter to close..."
