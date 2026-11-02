#!/bin/bash
# consola-sem-fio v0.2.0 — a consola do Cotucatronics por BLE, INTERACTIVA.
#
# ⚠️ EXISTE POR CAUSA DA PERMISSAO DE BLUETOOTH DO macOS. Um processo sem ela
# e MORTO com SIGABRT (saida 134) — nao da erro, nao pergunta, morre. O
# pedido so aparece a aplicacao que corre o comando, e por isso isto tem de
# ser lancado por ti: da duplo clique, e o macOS pergunta UMA vez.
#
# Se disseres nao por engano: Definicoes > Privacidade e Seguranca > Bluetooth.
#
# 🔴 v0.2.0 — A FORMA DE UM TIRO SO ESTAVA ERRADA. Ate a v0.1.0 isto
# perguntava UMA linha de comandos, enviava, e fechava. A 2026-09-29, a meio de um
# troubleshoot com a caixa montada, o dB escreveu `board`, a janela fechou-se,
# e o `oled te` que escreveu a seguir foi parar ao prompt de "Enter para
# fechar". Perdeu-se o comando e perdeu-se a sessao — e cada comando novo
# pagava outra varredura de 10 s e outra ligacao, que num diagnostico de
# intermitencia e uma variavel a mais entre duas leituras.
#
# Agora liga UMA vez e fica. Escreve comandos; `sair` fecha.
cd "$(dirname "$0")/.."
VENV=".venv-ble"
if [ ! -d "$VENV" ]; then
  echo "a criar o ambiente (so da primeira vez)..."
  python3 -m venv "$VENV" && "$VENV/bin/pip" install --quiet bleak
fi
echo "consola-sem-fio v0.2.0"
echo
"$VENV/bin/python" -u tools/cotuca_ble.py --viva
echo
read -p "Enter para fechar..."
