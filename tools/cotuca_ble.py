#!/usr/bin/env python3
"""cotuca_ble v0.4.0 — a consola do Cotucatronics SEM FIO, por BLE.

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

VERSAO = "0.4.0"

NUS_SVC = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX  = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"   # o Mac ESCREVE aqui
NUS_TX  = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"   # a placa NOTIFICA aqui


async def corre(nome, comandos, segundos):
    from bleak import BleakClient, BleakScanner
    print("  a varrer 10 s...")
    # ⚠️ "NAO ENCONTRADO" NAO E RESPOSTA. A primeira versao dizia so isso, e
    # nao distinguia "a ponte esta morta" de "anuncia com outro nome" de "o
    # Mac nao esta a ver radio nenhum". Varrer e MOSTRAR o que se viu
    # transforma um nao em dados — e foi o que faltou a tarde inteira do
    # OLED, medir o que ha em vez de perguntar pelo que se espera.
    achados = await BleakScanner.discover(timeout=10.0, return_adv=True)
    d = None
    for dev, adv in achados.values():
        n = (adv.local_name or dev.name or "")
        if n == nome:
            d = dev; break
    if d is None:   # 2a tentativa: pelo SERVICO, que nao depende do nome
        for dev, adv in achados.values():
            if NUS_SVC in [u.lower() for u in (adv.service_uuids or [])]:
                d = dev
                print("  (achei pelo SERVICO, nao pelo nome: %r)"
                      % (adv.local_name or dev.name))
                break
    if d is None:
        print("  NAO ACHEI `%s`. O que o radio viu nestes 10 s (%d aparelhos):"
              % (nome, len(achados)))
        for dev, adv in sorted(achados.values(),
                               key=lambda x: -(x[1].rssi or -999))[:12]:
            print("    %-28s rssi %-5s %s"
                  % ((adv.local_name or dev.name or "(sem nome)")[:28],
                     adv.rssi, dev.address))
        if not achados:
            print("    NENHUM — o Bluetooth do Mac esta ligado? Sem aparelho"
                  " nenhum a varredura nao provou nada.")
        return 3
    print("  achei: %s" % d.address)
    async with BleakClient(d) as c:
        # ⚠️ DESPEJA A TABELA ANTES DE SUBSCREVER. A 29/09 a ligacao caiu no
        # `start_notify` com um "disconnected" seco, e nao havia como saber se
        # a tabela estava errada ou se a subscricao pedia emparelhamento.
        # Imprimir o que se VE antes de tentar usar transforma as duas em
        # perguntas separadas — e o que se ve fica mesmo que o resto caia.
        print("  --- tabela GATT ---")
        for sv in c.services:
            print("    servico %s" % sv.uuid)
            for ch in sv.characteristics:
                print("      %s  %s" % (ch.uuid, ",".join(ch.properties)))
        print("  --- fim da tabela ---\n")
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
            dados = (cmd + "\n").encode()
            # ⚠️ COM CONFIRMACAO, e a 29/09 isto foi o conserto do diagnostico.
            # A primeira versao escrevia SEM resposta: a ponte contou zero
            # escritas e o cliente nao se queixou de nada — silencio
            # indistinguivel de sucesso, que e a familia que este projecto
            # persegue. Com confirmacao, ou passa ou diz o nome do erro.
            try:
                await c.write_gatt_char(NUS_RX, dados, response=True)
                print("  > %s   (%d B entregues)" % (cmd, len(dados)))
            except Exception as e:
                print("  > %s   *** A ESCRITA FALHOU: %s" % (cmd, e))
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
