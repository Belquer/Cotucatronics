#!/bin/bash
# consola-sem-fio v0.1.0 — abre a consola do Cotucatronics por BLE.
#
# ⚠️ EXISTE POR CAUSA DA PERMISSAO DE BLUETOOTH DO macOS. Um processo sem ela
# e MORTO com SIGABRT (saida 134) — nao da erro, nao pergunta, morre. O
# pedido so aparece a aplicacao que corre o comando, e por isso isto tem de
# ser lancado por ti: da duplo clique, e o macOS pergunta UMA vez.
#
# Se disseres nao por engano: Definicoes > Privacidade e Seguranca > Bluetooth.
cd "$(dirname "$0")/.."
VENV=".venv-ble"
if [ ! -d "$VENV" ]; then
  echo "a criar o ambiente (so da primeira vez)..."
  python3 -m venv "$VENV" && "$VENV/bin/pip" install --quiet bleak
fi
echo
read -p "comandos a enviar (vazio = so escutar 10 s): " CMDS
echo
if [ -z "$CMDS" ]; then
  "$VENV/bin/python" -u tools/cotuca_ble.py --listen 10
else
  "$VENV/bin/python" -u tools/cotuca_ble.py --send $CMDS --listen 3
fi
echo
read -p "Enter para fechar..."
