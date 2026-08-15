#!/bin/bash
# measure.command v0.1.0 — wrapper duplo-clicavel do cotuca_measure.py
cd "$(dirname "$0")"
echo "measure.command v0.1.0"
echo
echo "Mede a frequencia real da saida do firmware por FFT."
echo "Deixe o tom soando antes (./monitor.command -> 'tone on')."
echo
echo "Entradas disponiveis:"
python3 -c "
import sounddevice as sd
for i,d in enumerate(sd.query_devices()):
    if d['max_input_channels']>0:
        print(f\"  [{i}] {d['name']}\")
" 2>/dev/null || echo "  (nao consegui listar — sounddevice instalado?)"
echo
read -p "Device (Enter = varrer todas): " DEV
read -p "Frequencia esperada em Hz (Enter = so medir): " EXP

ARGS=""
if [ -z "$DEV" ]; then ARGS="--scan"; else ARGS="--device $DEV"; fi
if [ -n "$EXP" ]; then ARGS="$ARGS --expect $EXP"; fi

./cotuca_measure.py $ARGS
echo
read -p "Press Enter to close..."
