#!/usr/bin/env python3
"""cotuca_serial v0.3.0 — monitor/tester serial do Cotucatronics OS.

Sem dependencias externas (nada de pyserial): fala direto com o tty
via stty + file descriptor, que e o suficiente pra USB CDC da Daisy.

Uso:
    ./cotuca_serial.py                       # so escuta
    ./cotuca_serial.py --send board list     # manda comandos e mostra resposta
    ./cotuca_serial.py --port /dev/cu.usbmodemXXXX --listen 5
"""

VERSION = "0.3.0"

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
# "muda" a uma placa viva. Aconteceu tres vezes em 01-02/09, e uma delas
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
    """A porta certa e a que RESPONDE, nunca a que tem o nome certo.

    ⚠️ Ate 01/09 isto era `sorted(glob(...))[-1]` — ordem alfabetica. Nesse
    dia a ponte ESP32 enumerou como `usbmodem101`, entrou na lista, e cinco
    comandos de configuracao foram para ela enquanto o script anunciava que
    tinham sido aplicados. `[-1]` calhava dar certo umas vezes e errado
    noutras, que e o pior dos casos: nao falha o suficiente para se notar.

    Nome de dispositivo e uma etiqueta que o sistema operativo atribui.
    Identidade e o que o outro lado responde. Ver battle_scars 15ax.
    """
    ports = sorted(glob.glob("/dev/cu.usbmodem*"))
    if not ports:
        sys.exit("nenhuma /dev/cu.usbmodem* encontrada — a Daisy esta plugada e fora do DFU?")
    if _pyserial is None:
        print("[AVISO] sem pyserial: NAO consigo confirmar identidade da porta.\n"
              f"        a escolher {ports[-1]} pelo NOME, que ja mandou comandos\n"
              "        para a ponte ESP32 uma vez (battle_scars 15ax).\n"
              "        `pip3 install pyserial` faz a sonda voltar.", file=sys.stderr)
        return ports[-1]
    for cand in ports:
        try:
            with _pyserial.Serial(cand, BAUD, timeout=1) as s:
                time.sleep(0.8)
                s.reset_input_buffer()
                s.write(b"board\n")
                time.sleep(1.2)
                if b"Cotucatronics OS" in s.read(400):
                    if not quieto:
                        print(f"[porta] {cand} respondeu Cotucatronics OS",
                              file=sys.stderr)
                    return cand
        except Exception:
            pass
    sys.exit("NENHUMA porta respondeu 'Cotucatronics OS' ao `board`.\n"
             f"  candidatas: {ports}\n"
             "  a placa pode estar em DFU, presa, ou so ha a ponte ligada.")


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
