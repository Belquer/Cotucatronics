#!/usr/bin/env python3
"""cotuca_ble v0.1.0 — a consola do Cotucatronics SEM FIO, por BLE.

    tools/cotuca_ble.py [--send "board" "midi" ...] [--listen SEG] [--nome NOME]

POR QUE EXISTE (ppc/006, fase 1). Metade do trabalho de Setembro foi
diagnostico pela consola da Daisy — `board`, `midi`, `fio`, `oledspi`,
`knob` — e a consola vive no USB, que e o cabo que obriga a abrir a caixa.
Isto poe a MESMA consola por radio.

O CAMINHO, e nenhum troco dele e novo:

    Mac --BLE NUS--> ponte ESP32 --F0 7D <tipo> ... F7--> Daisy

⚠️ O NUS NAO E ANUNCIADO. O pacote de anuncio de 31 bytes ja leva o servico
MIDI e o nome `Cotucatronics`, que o perfil do Loopy usa como identidade —
anunciar mais partiria o pareamento para ganhar o que a descoberta
pos-ligacao ja da. Portanto: liga-se PRIMEIRO, procura-se DEPOIS.

⚠️ E o iPad pode estar ligado ao mesmo tempo. O BLE do ESP32-C3 aceita uma
ligacao de cada vez por omissao: se o Loopy estiver agarrado, isto nao liga.
Nao e defeito — e o radio. O comando diz isso em vez de esperar calado.
"""
import argparse
import asyncio
import sys

VERSAO = "0.1.0"

NUS_SVC = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX  = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"   # o Mac ESCREVE aqui
NUS_TX  = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"   # a placa NOTIFICA aqui


async def corre(nome, comandos, segundos):
    from bleak import BleakClient, BleakScanner
    print("  a procurar `%s`..." % nome)
    d = await BleakScanner.find_device_by_name(nome, timeout=10.0)
    if d is None:
        print("  NAO ENCONTRADO. A ponte esta alimentada? (o +5V dela vem do"
              " step-down, logo precisa dos 15 V)")
        return 3
    print("  achei: %s" % d.address)
    async with BleakClient(d) as c:
        svcs = {s.uuid.lower() for s in c.services}
        if NUS_SVC not in svcs:
            print("  LIGOU, mas nao tem o servico de consola. A ponte esta em"
                  " v1.1.0 ou mais nova? (`board` na USB dela diz)")
            print("  servicos vistos: %s" % sorted(svcs))
            return 3

        def chegou(_, dados):
            sys.stdout.write(dados.decode("utf-8", "replace"))
            sys.stdout.flush()

        await c.start_notify(NUS_TX, chegou)
        print("  ligado · consola aberta\n")
        for cmd in comandos:
            print("  > %s" % cmd)
            await c.write_gatt_char(NUS_RX, (cmd + "\n").encode(), response=False)
            await asyncio.sleep(1.2)
        if segundos:
            await asyncio.sleep(segundos)
        await c.stop_notify(NUS_TX)
    print("\n[fim]")
    return 0


def main():
    print("cotuca_ble v%s" % VERSAO)
    p = argparse.ArgumentParser()
    p.add_argument("--send", nargs="*", default=[])
    p.add_argument("--listen", type=float, default=0.0)
    p.add_argument("--nome", default="Cotucatronics")
    a = p.parse_args()
    if not a.send and not a.listen:
        a.listen = 5.0
    try:
        return asyncio.run(corre(a.nome, a.send, a.listen))
    except Exception as e:
        print("  FALHOU: %s" % e)
        return 3


if __name__ == "__main__":
    raise SystemExit(main())
