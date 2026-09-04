#!/bin/bash
# modo_exciter.command v0.1.1 — envolucro clicavel do modo_exciter.py
cd "$(dirname "$0")"
echo "Ganho de bancada ao voltar ao exciter."
echo "  1  = unidade, o estado conhecido-bom (Enter aceita)"
read -p "boost [1]: " B
python3 modo_exciter.py "${B:-1}"
read -p "Press Enter to close..."
