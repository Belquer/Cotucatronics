#!/usr/bin/env python3
"""cotuca_ble v0.6.1 — a consola do Cotucatronics SEM FIO, por BLE.

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

✅ E O iPAD PODE ESTAR LIGADO AO MESMO TEMPO. A ponte v1.7.0 leva
CONFIG_BT_NIMBLE_MAX_CONNECTIONS = 3, e o dB confirmou em bancada a 29/09:
Loopy e consola agarrados ao mesmo tempo, os dois a funcionar.

🔴 Este paragrafo dizia o CONTRARIO ate 29/09 — "aceita uma ligacao de cada
vez ... se o Loopy estiver agarrado, isto nao liga". Era verdade antes do
conserto, ficou aqui depois dele, e nesse mesmo dia fez-me avisar o dB de uma
limitacao que ja nao existia. E a cicatriz 15fx: documentacao que afirma
premissa revogada re-ensina o bug, e o cabecalho e o que se le. Ao consertar
um limite, procurar TODO sitio que o anuncia.
"""
import argparse
import asyncio
import sys

VERSAO = "0.6.0"

NUS_SVC = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX  = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"   # o Mac ESCREVE aqui
NUS_TX  = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"   # a placa NOTIFICA aqui


async def manda_ota(c, caminho):
    """Grava a ponte por radio (ppc/006 fase 2).

    ⚠️ ESCRITA COM CONFIRMACAO, SEMPRE. Sem resposta e mais rapido e ja nos
    custou um diagnostico (bts 15fr): num comando perdido perde-se uma letra,
    numa imagem perde-se a imagem. O MD5 apanha o que a contagem nao apanhar.
    """
    import hashlib, os
    dados = open(caminho, "rb").read()
    md5 = hashlib.md5(dados).hexdigest()
    print("  imagem: %s · %d B · md5 %s" % (os.path.basename(caminho),
                                            len(dados), md5))
    await c.write_gatt_char(NUS_RX, ("!ota %d %s\n" % (len(dados), md5)).encode(),
                            response=True)
    await asyncio.sleep(1.0)

    PEDACO = 180          # cabe num MTU tipico sem fragmentar
    enviados = 0
    t0 = asyncio.get_event_loop().time()
    while enviados < len(dados):
        fim = min(enviados + PEDACO, len(dados))
        await c.write_gatt_char(NUS_RX, dados[enviados:fim], response=True)
        enviados = fim
        if enviados % (PEDACO * 100) == 0 or enviados == len(dados):
            dt = asyncio.get_event_loop().time() - t0
            print("    %6d / %d B  (%.1f kB/s)"
                  % (enviados, len(dados), enviados / 1024.0 / max(dt, 0.01)))
    print("  imagem entregue — a ponte confere o MD5 e reinicia.")
    print("  ⚠️ Se ela nao voltar a anunciar em 15 s, o bootloader volta"
          " sozinho a imagem anterior no reinicio seguinte.")
    await asyncio.sleep(6.0)


async def corre(nome, comandos, segundos, ota=None):
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
        # ⚠️ ATE A PONTE v1.3.0, "nao vi" tambem queria dizer "o Loopy esta
        # agarrado": o anuncio parava enquanto havia ligacao, e a ponte ficava
        # INVISIVEL. Desde a v1.3.0 ela reanuncia enquanto ha vaga (3 no C3),
        # entao "nao vi" voltou a significar so uma coisa.
        print("\n    (a ponte v1.3.0+ continua a anunciar mesmo com o Loopy"
              " ligado — cabem 3. Se ela nao aparece, esta sem alimentacao,"
              " fora de alcance, ou com firmware anterior a v1.3.0.)")
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
        if ota:
            await manda_ota(c, ota)
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
        # (!) o `stop_notify` rebenta no macOS a fechar ("Service Discovery
        # has not been performed yet") — sair do contexto ja desliga, e uma
        # falha na ARRUMACAO nao pode esconder um resultado que correu bem.
        try:
            await c.stop_notify(NUS_TX)
        except Exception:
            pass
    print("\n[fim]")
    return 0


def main():
    print("cotuca_ble v%s" % VERSAO)
    p = argparse.ArgumentParser()
    p.add_argument("--send", nargs="*", default=[])
    p.add_argument("--listen", type=float, default=0.0)
    p.add_argument("--nome", default="Cotucatronics")
    p.add_argument("--ota", default=None,
                   help="grava a PONTE com este .bin, por radio")
    a = p.parse_args()
    if not a.send and not a.listen and not a.ota:
        a.listen = 5.0
    try:
        return asyncio.run(corre(a.nome, a.send, a.listen, a.ota))
    except Exception as e:
        print("  FALHOU: %s" % e)
        return 3


if __name__ == "__main__":
    raise SystemExit(main())
