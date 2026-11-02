#!/usr/bin/env python3
"""vigia_reset v0.1.0 — apanha reinicios da Daisy pelo TESTEMUNHO DO SISTEMA.

    tools/vigia_reset.py [segundos]        (omisso: 240)

POR QUE NAO LE A PLACA. A primeira versao perguntava a semente de abertura
pela consola. Morreu a meio da primeira leitura com

    SerialException: device reports readiness to read but returned no data
    (device disconnected or multiple access on port?)

e essa morte ERA a resposta: uma Daisy que reinicia RE-ENUMERA no USB, e a
porta serie desaparece debaixo de quem a estava a ler. A sonda que pergunta ao
acusado cai junto com ele.

O no de dispositivo em /dev nao depende do firmware: o macOS cria-o quando o
aparelho enumera e apaga-o quando ele se vai. A marca de tempo dele e o
registo do sistema operativo, nao da placa.

⚠️ E HA BATIMENTO. Uma sonda que so fala quando ha novidade e indistinguivel
de uma sonda morta — foi o que aconteceu a 2026-09-29 com a versao anterior, que
imprimiu duas linhas e calou-se, e eu quase li o silencio como "sem
reinicios". Silencio nao e dado.
"""
import glob, os, sys, time

VERSAO = "0.1.0"
PADRAO = "/dev/cu.usbmodem*"


def estado():
    achados = sorted(glob.glob(PADRAO))
    if not achados:
        return None, None
    p = achados[0]
    try:
        return p, os.stat(p).st_ctime
    except OSError:
        return None, None


def main():
    segundos = float(sys.argv[1]) if len(sys.argv) > 1 else 240.0
    print("vigia_reset v%s — %s · %.0f s" % (VERSAO, PADRAO, segundos), flush=True)
    t0 = time.time()
    ant_p, ant_c = estado()
    if ant_p:
        print("  %6.1fs  base: %s (nó criado %s)"
              % (0.0, ant_p, time.strftime("%H:%M:%S", time.localtime(ant_c))), flush=True)
    else:
        print("  %6.1fs  NAO HA APARELHO — a Daisy nao esta no USB" % 0.0, flush=True)
    n = 0
    prox_bat = 15.0
    while time.time() - t0 < segundos:
        time.sleep(0.4)
        p, c = estado()
        dt = time.time() - t0
        if p and not ant_p:
            n += 1
            print("  %6.1fs  *** VOLTOU (%d) *** %s" % (dt, n, p), flush=True)
        elif ant_p and not p:
            print("  %6.1fs  *** DESAPARECEU *** (re-enumeracao = REINICIO)" % dt, flush=True)
        elif p and ant_p and c != ant_c:
            n += 1
            print("  %6.1fs  *** RE-ENUMEROU (%d) *** nó recriado às %s"
                  % (dt, n, time.strftime("%H:%M:%S", time.localtime(c))), flush=True)
        elif dt >= prox_bat:
            print("  %6.1fs  (vivo, sem reinicio)" % dt, flush=True)
            prox_bat += 15.0
        ant_p, ant_c = p, c
    print("FIM. reinicios apanhados: %d" % n, flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
