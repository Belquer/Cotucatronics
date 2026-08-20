#!/bin/bash
# ccmap-pdf.command v0.1.0 — wrapper duplo-clicavel de ccmap-pdf.sh
cd "$(dirname "$0")"
./ccmap-pdf.sh
echo
read -p "Press Enter to close..."
