#!/bin/bash
# test.command v0.1.0 — compila e roda o banco de teste nativo do dynamics.h
cd "$(dirname "$0")"
echo "test.command v0.1.0"
echo
echo "dynamics.h e C++ puro e board-agnostic, entao a matematica dos"
echo "tres blocos roda no Mac — sem placa, sem flash, em 1 segundo."
echo
clang++ -std=c++14 -O2 -I. test_dynamics.cpp -o /tmp/cotuca_test_dynamics && /tmp/cotuca_test_dynamics
STATUS=$?
echo
read -p "Press Enter to close..."
exit $STATUS
