#!/bin/bash
# grava_placa_nova.command v0.2.0 — grava uma Daisy nova, esteja ela como
# estiver: de fabrica (sem bootloader) ou ja com o bootloader instalado.
#
# POR QUE EXISTE (2026-09-04). A janela de DFU do bootloader dura 2 SEGUNDOS, e
# uma conversa com o assistente demora trinta. Nao se ganha essa corrida por
# mensagens: quem espera tem de estar na mesma sala que o botao.
#
# O QUE MUDOU NA v0.2.0 (2026-09-05). A v0.1.0 corria SEMPRE `make program-dfu`,
# que so serve se o bootloader da Electrosmith ja la estiver. Numa placa de
# fabrica ficava em ciclo PARA SEMPRE sem dizer porque — o DFU que ela espera
# nunca aparece, porque quem aparece e o da ROM da ST.
#
# ⚠️ O DISCRIMINADOR NAO E O PID. Os dois DFU enumeram como `0483:df11`
# (`STM_PID` e `DAISY_PID` sao o MESMO valor no Makefile da libDaisy). Quem
# distingue e o MAPA DE MEMORIA que o dispositivo anuncia:
#
#     0x90000000  -> QSPI exposta   -> bootloader ja instalado -> program-dfu
#     0x08000000  -> flash interna  -> DFU da ROM da ST        -> program-boot
#
# A primeira versao deste sensor discriminava pelo PID e teria dito sempre
# "falta o bootloader". Foi escrita de memoria, com a saida medida a frente.
#
# A ordem completa, numa placa de fabrica (ver battle_scars):
#     BOOT+RST -> program-boot -> RST,depois BOOT -> program-dfu
# E o app NAO cabe na flash interna (283 KB contra 128 KB): nao ha atalho.
cd "$(dirname "$0")/../src"
echo "grava_placa_nova v0.2.0"

# So linhas de DFU da ST; o resto da USB (interfaces Arturia e afins) nao conta.
dfu_linha() { dfu-util -l 2>/dev/null | grep "Found DFU" | grep "0483:df11" | head -1; }

esperar_dfu() {   # $1 = texto do gesto a pedir
  echo
  echo "  $1"
  echo "  (Ctrl-C para desistir)"
  echo
  local i=0
  while true; do
    L=$(dfu_linha)
    if [ -n "$L" ]; then
      case "$L" in
        *0x90000000*) echo "  DFU visto: QSPI exposta — o bootloader ESTA instalado."; ALVO=program-dfu; return 0;;
        *0x08000000*) echo "  DFU visto: flash interna — e a ROM da ST, FALTA o bootloader."; ALVO=program-boot; return 0;;
        *) echo "  DFU visto, mas com um mapa que eu nao reconheco:"; echo "    $L"
           echo "  Nao adivinho o alvo. Pare e leia — enganar-me aqui grava no sitio errado."
           return 1;;
      esac
    fi
    i=$((i+1))
    [ $((i % 40)) -eq 0 ] && echo "  ...ainda a espera ($((i/4))s). Repita o gesto."
    sleep 0.25
  done
}

make -j8 >/dev/null 2>&1 || { echo "BUILD FALHOU"; read -p "Enter..."; exit 1; }
echo "  build ok."

esperar_dfu "Toque RST. Se o LED pulsar ~2 s, toque BOOT DURANTE o pulsar.
  Se NAO pulsar, segure BOOT e toque RST." || { read -p "Enter..."; exit 1; }

if [ "$ALVO" = "program-boot" ]; then
  echo "  a instalar o bootloader..."
  make program-boot 2>&1 | tail -3
  echo
  echo "  >>> BOOTLOADER instalado (o erro no 'leave' e normal)."
  esperar_dfu "Agora toque RST e, DURANTE o pulsar de ~2 s, toque BOOT." \
    || { read -p "Enter..."; exit 1; }
  [ "$ALVO" = "program-dfu" ] || { echo "  Esperava a QSPI e nao a vi. Pare."; read -p "Enter..."; exit 1; }
fi

echo "  a gravar o app na QSPI..."
if make program-dfu 2>&1 | grep -q "File downloaded successfully"; then
  echo
  echo "  >>> GRAVADO."
  echo "  Confirme a versao NA PLACA (saida de ferramenta nao e confirmacao):"
  echo "      python3 porta_daisy.py"
else
  echo "  >>> A GRAVACAO NAO CONFIRMOU. Nao trate como feita."
fi
read -p "Press Enter to close..."
