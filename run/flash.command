#!/bin/bash
# flash.command v0.1.0 — wrapper de duplo-clique para o `flash.sh`.
#
# Regra da casa: todo helper corrivel por shell tem um `.command` ao lado.
# O `flash.sh` era o unico da bancada sem um — descoberto em 02/09, na
# varredura do "sobrou algum bug?".
cd "$(dirname "$0")/../src"
echo "flash.command v0.1.0"
echo
./flash.sh
echo
read -p "Press Enter to close..."
