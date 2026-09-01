#!/bin/bash
# medir_pente.command v0.2.0 — invólucro de dois cliques
cd "$(dirname "$0")"
echo "medir_pente.command v0.2.0"
read -p "Numero do dispositivo de entrada (MiniFuse 4 costuma ser 3): " DEV
read -p "Segundos [4]: " SEG
./medir_pente.py --device "${DEV:-3}" --seconds "${SEG:-4}"
read -p "Press Enter to close..."
