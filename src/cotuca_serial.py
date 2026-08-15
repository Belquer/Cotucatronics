#!/usr/bin/env python3
"""cotuca_serial v0.1.0 — monitor/tester serial do Cotucatronics OS.

Sem dependencias externas (nada de pyserial): fala direto com o tty
via stty + file descriptor, que e o suficiente pra USB CDC da Daisy.

Uso:
    ./cotuca_serial.py                       # so escuta
    ./cotuca_serial.py --send board list     # manda comandos e mostra resposta
    ./cotuca_serial.py --port /dev/cu.usbmodemXXXX --listen 5
"""

VERSION = "0.1.0"

import argparse
import glob
import os
import subprocess
import sys
import threading
import time

BAUD = 115200


def find_port():
    """Daisy em modo app aparece como /dev/cu.usbmodem*."""
    ports = sorted(glob.glob("/dev/cu.usbmodem*"))
    if not ports:
        sys.exit("nenhuma /dev/cu.usbmodem* encontrada — a Daisy esta plugada e fora do DFU?")
    if len(ports) > 1:
        print(f"[aviso] varias portas: {ports} — usando {ports[-1]}", file=sys.stderr)
    return ports[-1]


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
