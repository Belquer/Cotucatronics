#!/usr/bin/env python3
"""cotuca_serial v0.4.0 — monitor/tester serial do Cotucatronics OS.

Sem dependencias externas (nada de pyserial): fala direto com o tty
via stty + file descriptor, que e o suficiente pra USB CDC da Daisy.

Uso:
    ./cotuca_serial.py                       # so escuta
    ./cotuca_serial.py --send board list     # manda comandos e mostra resposta
    ./cotuca_serial.py --port /dev/cu.usbmodemXXXX --listen 5
"""

VERSION = "0.4.0"

import argparse
import glob
import serial
import os
import subprocess
import sys
import threading
import time

BAUD = 115200


# ⚠️ REGIME DE VALIDADE DESTE SONDADOR, e ele recusa julgar fora dele.
#
# A placa tem um vigia no callback de audio que a REINICIA se o laco nao
# completar uma volta em 48 000 amostras (1 s). Somando o arranque, ha uma
# janela em que ela esta viva e nao responde — porque esta a reiniciar.
#
# Um sondador com janela menor que (vigia + arranque) pode SEMPRE dizer
# "muda" a uma placa viva. Aconteceu tres vezes em 01-2026-09-02, e uma delas
# quase me fez reportar como morta uma placa que estava a recuperar
# sozinha, que era exactamente o comportamento desejado.
#
# Entao a janela minima e essa soma, e o veredicto passa a ter tres
# valores em vez de dois: RESPONDE · REINICIOU E VOLTOU · MUDA. O do meio
# e informacao, nao ruido.
VIGIA_S    = 1.0     # o vigia dispara em 48 000 amostras
ARRANQUE_S = 8.0     # medido: RESET ate responder ao `board`
JANELA_MIN = VIGIA_S + ARRANQUE_S


def find_port(quieto=False):
    """A porta certa e a que RESPONDE — e quem sabe perguntar e o
    `porta_daisy`, que existe desde 2026-08-25 EXACTAMENTE para isto.

    ⚠️ ATE 2026-09-02 ESTE FICHEIRO TINHA A SUA PROPRIA COPIA. Antes disso
    tinha `sorted(glob(...))[-1]`, que mandou cinco comandos para a ponte
    ESP32 enquanto o script anunciava que tinham sido aplicados — e o
    `porta_daisy.py` ja tinha ESSE defeito escrito na docstring, como
    PREVISAO, uma semana antes:

        "o modo de falha seria mandar comandos da Daisy para o ESP32: sem
         erro, sem resposta, e um diagnostico que procuraria o defeito no
         lado errado."

    O conserto existia, estava certo, e nao tinha sido adoptado. Corrigir
    isto com uma copia nova teria criado a TERCEIRA implementacao da mesma
    coisa — e tres implementacoes divergem, sempre. Ver battle_scars 15bd.
    """
    try:
        import porta_daisy
    except ImportError:
        sys.exit("porta_daisy.py nao encontrado ao lado deste script")
    # ⚠️ `achar()` devolve o PAR (porta, versao) — apanhado a correr, nao
    #    a ler, dois minutos depois de eu escrever que adoptar funcao
    #    partilhada exige respeitar o contrato dela.
    porta, _versao = porta_daisy.achar(verboso=not quieto)
    return porta

def configure(port):
    # raw, sem echo, sem tratamento de CR/LF. A CDC ignora o baud, mas
    # setar nao custa nada e mantem o contrato de 115200 do projeto.
    subprocess.run(
        ["stty", "-f", port, str(BAUD), "raw", "-echo", "-echoe", "-echok", "clocal", "cread"],
        check=True,
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default=None)
    ap.add_argument("--send", nargs="*", default=[], help="comandos a enviar, em ordem")
    ap.add_argument("--listen", type=float, default=3.0, help="segundos de escuta no fim")
    ap.add_argument("--gap", type=float, default=0.8, help="pausa entre comandos")
    args = ap.parse_args()

    print(f"cotuca_serial v{VERSION}")

    port = args.port or find_port()
    configure(port)
    print(f"[porta] {port} @ {BAUD}\n")

    # O_NONBLOCK evita travar no open quando nao ha DCD; /dev/cu.* nao
    # levanta DTR, entao abrir NAO reseta a placa.
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    stop = threading.Event()

    def reader():
        buf = b""
        while not stop.is_set():
            try:
                chunk = os.read(fd, 4096)
            except BlockingIOError:
                time.sleep(0.02)
                continue
            except OSError:
                break
            if not chunk:
                time.sleep(0.02)
                continue
            buf += chunk
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                text = line.decode("utf-8", "replace").rstrip("\r")
                if text:
                    print(f"  < {text}", flush=True)

    t = threading.Thread(target=reader, daemon=True)
    t.start()

    time.sleep(0.4)  # deixa drenar o que ja estava no buffer

    for cmd in args.send:
        print(f"  > {cmd}", flush=True)
        os.write(fd, (cmd + "\n").encode())
        time.sleep(args.gap)

    time.sleep(args.listen)
    stop.set()
    t.join(timeout=1.0)
    os.close(fd)
    print("\n[fim]")


if __name__ == "__main__":
    main()
