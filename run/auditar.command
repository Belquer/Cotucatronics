#!/bin/bash
# auditar.command v1.3.0 — embrulho de duplo clique para auditar.sh
cd "$(dirname "$0")/../docs/operacao"
read -p "Termo a inventariar nos tres destinos (Enter para varredura geral): " TERMO
./auditar.sh $TERMO
echo
read -p "Press Enter to close..."
