#!/bin/bash
# vigia-reset v0.1.0 — apanha reinicios da Daisy enquanto mexes nos conectores.
#
# Le o NO DE DISPOSITIVO em /dev, nao a placa: uma Daisy que reinicia
# re-enumera no USB, e o macOS testemunha isso independentemente do firmware.
# Tem batimento, para "estavel" e "morto" nao terem a mesma cara.
cd "$(dirname "$0")/.."
echo "vigia-reset v0.1.0"
echo
read -p "quantos segundos vigiar (vazio = 240): " SEG
echo
python3 tools/vigia_reset.py ${SEG:-240}
echo
read -p "Enter para fechar..."
