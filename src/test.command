#!/bin/bash
# test.command v0.4.0 — compila e roda os bancos de teste nativos
#
# v0.2.0: junta o banco do midi.h ao do dynamics.h. Os dois rodam no
# Mac porque os dois arquivos sao C++ puro e board-agnostic.
cd "$(dirname "$0")"
echo "test.command v0.4.0 — os CINCO testes nativos"
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
echo "=============== presets ==============="
clang++ -std=c++14 -O2 -I. test_presets.cpp -o /tmp/cotuca_test_presets \
    && /tmp/cotuca_test_presets
[ $? -ne 0 ] && STATUS=1

# ⚠️ ESTES DOIS NASCERAM DEPOIS E NUNCA FORAM LIGADOS AQUI.
#
# Descoberto em 02/09: este corredor chamava TRES dos cinco testes
# nativos. O `teste_macro` e o `teste_tarol` existiam, compilavam e
# passavam — e ninguem os corria, porque o caminho que toda a gente usa
# nao os chamava. Apodreceram em silencio, e eu subi a versao de um deles
# nesse mesmo dia sem o executar uma vez.
#
# Familia da 15bd: a ferramenta existe e o caminho de uso nao a adopta.
# Teste que ninguem corre nao e teste — e uma opiniao antiga.
echo
echo "=============== macro (ida-e-volta do Inverso) ==============="
clang++ -std=c++14 -O2 -I. teste_macro.cpp -o /tmp/cotuca_teste_macro \
  && /tmp/cotuca_teste_macro
[ $? -ne 0 ] && STATUS=1

echo
echo "=============== tarolator ==============="
clang++ -std=c++14 -O2 -I. teste_tarol.cpp -o /tmp/cotuca_teste_tarol \
  && /tmp/cotuca_teste_tarol
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
