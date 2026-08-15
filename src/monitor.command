#!/bin/bash
# monitor.command v0.1.0 — wrapper duplo-clicavel do cotuca_serial.py
cd "$(dirname "$0")"
echo "monitor.command v0.1.0"
echo
echo "Comandos do firmware: board | list | k1..k4 <0-1> | tone on | tone off"
echo "Sine de 440 Hz:       k2 0.1875   depois   k3 1"
echo
read -p "Comandos a enviar (vazio = so escutar): " CMDS
if [ -z "$CMDS" ]; then
  ./cotuca_serial.py --listen 20
else
  ./cotuca_serial.py --send $CMDS --listen 5
fi
echo
read -p "Press Enter to close..."
