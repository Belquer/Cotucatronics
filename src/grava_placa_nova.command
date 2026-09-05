#!/bin/bash
# grava_placa_nova.command v0.1.0 — grava o app na QSPI de uma Daisy nova.
#
# POR QUE EXISTE (04/09). A janela de DFU do bootloader dura 2 SEGUNDOS, e
# uma conversa com o assistente demora trinta. Nao se ganha essa corrida por
# mensagens: quem espera tem de estar na mesma sala que o botao.
#
# Corra ISTO, e so entao faca o gesto — as vezes que forem precisas.
cd "$(dirname "$0")"
echo "grava_placa_nova v0.1.0"
echo
echo "  1. deixe esta janela a correr"
echo "  2. toque RST.  O LED pulsa ~2 s."
echo "  3. DURANTE o pulsar, toque BOOT uma vez."
echo "  4. repita ate aparecer GRAVADO."
echo
echo "  (Ctrl-C para desistir)"
echo
make -j8 >/dev/null 2>&1 || { echo "BUILD FALHOU"; read -p "Enter..."; exit 1; }
echo "  build ok — a espera do DFU..."
i=0
while true; do
  i=$((i+1))
  if dfu-util -l 2>/dev/null | grep -q "Found DFU"; then
    echo "  DFU visto — a gravar..."
    if make program-dfu 2>&1 | grep -q "File downloaded successfully"; then
      echo
      echo "  >>> GRAVADO. A placa deve reiniciar e fazer a abertura no OLED."
      break
    fi
  fi
  [ $((i % 40)) -eq 0 ] && echo "  ...ainda a espera ($((i/4))s). Toque RST, depois BOOT."
  sleep 0.25
done
echo
read -p "Press Enter to close..."
