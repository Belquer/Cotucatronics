#!/bin/bash
# test.command v0.2.0 — compila e roda os bancos de teste nativos
#
# v0.2.0: junta o banco do midi.h ao do dynamics.h. Os dois rodam no
# Mac porque os dois arquivos sao C++ puro e board-agnostic.
cd "$(dirname "$0")"
echo "test.command v0.2.0"
echo
echo "dynamics.h e midi.h sao C++ puro e board-agnostic, entao a"
echo "matematica dos blocos e a maquina de estados MIDI rodam no Mac —"
echo "sem placa, sem flash, em 1 segundo."
echo

STATUS=0

echo "=============== dynamics ==============="
clang++ -std=c++14 -O2 -I. test_dynamics.cpp -o /tmp/cotuca_test_dynamics \
    && /tmp/cotuca_test_dynamics
[ $? -ne 0 ] && STATUS=1

echo
echo "================= midi ================="
clang++ -std=c++14 -O2 -I. test_midi.cpp -o /tmp/cotuca_test_midi \
    && /tmp/cotuca_test_midi
[ $? -ne 0 ] && STATUS=1

echo
if [ $STATUS -eq 0 ]; then
    echo "### TODOS OS BANCOS PASSARAM ###"
else
    echo "### ALGUM BANCO FALHOU ###"
fi
echo
echo "Lembrete: nenhum destes prova a UART. Baud rate, pino e DMA so"
echo "o loopback com jumper prova — T10 no TESTES.md."
echo
read -p "Press Enter to close..."
exit $STATUS
