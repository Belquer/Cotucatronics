#!/bin/bash
# fichas.command v0.1.0 — embrulho de duplo-clique para tools/fichas.py
cd "$(dirname "$0")"
echo "Fichas tecnicas do Cotucatronics"
echo
echo "  1) descarregar o que falta   (vai a rede)"
echo "  2) so conferir o que ja esta (nao vai a rede)"
echo "  3) descarregar TUDO de novo"
echo
read -p "escolha [1]: " escolha
case "${escolha:-1}" in
  2) python3 fichas.py --confere ;;
  3) python3 fichas.py --tudo ;;
  *) python3 fichas.py ;;
esac
echo
echo "indice: docs/datasheets/README.md"
read -p "Carrega Enter para fechar..."
