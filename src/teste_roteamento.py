#!/usr/bin/env python3
"""teste_roteamento v0.1.0 — prova os DOIS andares do roteamento, medindo.

    ./teste_roteamento.py

O QUE ELE PROVA, e por que sao coisas diferentes:

  route_l_a/b, route_r_a/b   ANTES dos efeitos   qual CADEIA entra em qual SAIDA
  out_l, out_r               DEPOIS do limiter   corta o RAMO inteiro

PRE-CONDICAO DE FIACAO, e ela NAO e opcional: cada saida da Daisy num canal
PROPRIO da interface. Em 2026-08-28 este teste correu com as duas saidas somadas
numa entrada so, e os dois criterios de ISOLAMENTO passaram sem testar nada —
"o L nao afecta o R" estava garantido porque o L nao chegava a interface.
Medicao que nao pode falhar nao e medicao.

O mapa medido nessa bancada saiu CRUZADO (OUT_L -> ch2, OUT_R -> ch1), e por
isso ele e MEDIDO no arranque em vez de assumido.

Cada comando confere o eco da placa e ABORTA se nao vier: uma corrida inteira
ja foi reportada como facto sobre a fiacao com a placa fora do barramento.
"""
import time, numpy as np, sounddevice as sd, serial

VERSAO = "0.1.0"
SR, NCH = 48000, 6
INTERFACE = "MiniFuse"        # casa por NOME, nunca por indice

_s = None
DEV = None

def acha_interface():
    """Acha a interface pelo NOME. Indice de dispositivo muda quando o
    utilizador liga outra coisa — e o indice errado mede o microfone do
    portatil e reporta silencio como se fosse a placa."""
    for i, d in enumerate(sd.query_devices()):
        if d['max_input_channels'] >= 2 and INTERFACE.lower() in d['name'].lower():
            return i, d['name'], d['max_input_channels']
    raise RuntimeError("interface %r nao encontrada" % INTERFACE)

def abre():
    """Acha a placa PERGUNTANDO (porta_daisy), nunca por nome fixo."""
    global _s, DEV, NCH
    import porta_daisy
    porta, versao = porta_daisy.achar()
    _s = serial.Serial(porta, 115200, timeout=1)
    time.sleep(2.5); _s.reset_input_buffer()
    DEV, nome, nch = acha_interface()
    NCH = min(nch, 6)
    print("placa: %s v%s" % (porta, versao))
    print("audio: [%d] %s (%d entradas)\n" % (DEV, nome, nch))
    return versao

def cmd(c, espera=0.35):
    _s.write((c + "\n").encode()); time.sleep(espera)
    return _s.read(9000).decode('utf-8','replace')

def rms(secs=0.7):
    x = sd.rec(int(secs*SR), samplerate=SR, channels=NCH, device=DEV, dtype='float32')
    sd.wait()
    x = x[int(0.15*SR):]           # descarta o ataque/rampa
    return [20*np.log10(max(np.sqrt(np.mean(x[:,c]**2)), 1e-9)) for c in range(NCH)]

def linha(rot, v, alvos=None):
    alvos = alvos if alvos is not None else range(NCH)
    print("  %-26s %s" % (rot, "  ".join("ch%d %7.1f" % (c+1, v[c]) for c in alvos)))


L = R = None                    # MEDIDOS no arranque, nunca assumidos

def pos(c, esperado):
    r = cmd(c, 0.45)
    if esperado not in r:
        print("\n*** ABORTA: '%s' nao confirmou. veio: %r" % (c, r.strip()[:160]))
        raise SystemExit(1)

def med(rot):
    time.sleep(0.6); v = rms(1.0)
    print("  %-38s OUT_L %7.1f   OUT_R %7.1f" % (rot, v[L], v[R]))
    return v[L], v[R]

def sw(*pares):
    for p, v in pares: pos("set %s %d" % (p, v), "real=%d.000" % v)

abre(); rms(0.4)
print("teste_roteamento v%s\n" % VERSAO)

def mapeia():
    """Descobre QUAL canal da interface ve QUAL saida, medindo. Na bancada
    de 2026-08-28 o mapa saiu CRUZADO (OUT_L -> ch2), e assumi-lo teria trocado
    todos os vereditos de lado sem nenhum deles falhar."""
    global L, R
    for c, e in [("set level 0.15","real=0.150"), ("set tone_freq 1000","real=1000.000"),
                 ("set test_tone_stage 0","real=0.000"), ("set out_l 1","real=1.000"),
                 ("set out_r 1","real=1.000"), ("set tone_mix 0","real=0.000")]:
        pos(c, e)
    time.sleep(0.5); chao = rms(1.0)
    pos("set tone_mix 0.30", "real=0.300")
    pos("set tone_dest 1", "real=1.000"); time.sleep(0.6)
    dl = [x - y for x, y in zip(rms(1.0), chao)]
    pos("set tone_dest 2", "real=2.000"); time.sleep(0.6)
    dr = [x - y for x, y in zip(rms(1.0), chao)]
    pos("set tone_mix 0", "real=0.000")
    L, R = dl.index(max(dl)), dr.index(max(dr))
    print("mapa medido:  OUT_L -> ch%d (%+.1f dB)   OUT_R -> ch%d (%+.1f dB)"
          % (L + 1, dl[L], R + 1, dr[R]))
    if L == R:
        print("*** ABORTA: as duas saidas caem no MESMO canal (ch%d)." % (L + 1))
        print("    Sem uma saida por canal, os criterios de ISOLAMENTO passam")
        print("    sem testar nada. Patchear cada saida numa entrada propria.")
        raise SystemExit(1)
    # DIAFONIA = quanto o canal ERRADO subiu. Perto de 0 dB acima do chao
    # e o que se quer; a versao anterior imprimia isto como "isolamento",
    # que le ao contrario e faz -0,2 dB parecer mau.
    dia = max(dl[R], dr[L])
    sep = min(dl[L], dr[R]) - dia
    print("diafonia: %+.1f dB acima do chao   separacao util: %.1f dB\n"
          % (dia, sep))

mapeia()
print("T7c COMPLETO — tom na CADEIA A, estagio POST\n")
for c, e in [("set level 0.15","real=0.150"), ("set tone_freq 1000","real=1000.000"),
             ("set test_tone_stage 1","real=1.000"), ("set tone_dest 1","real=1.000"),
             ("set tone_mix 0","real=0.000"), ("set dly_on_a 0","real=0.000"),
             ("set dly_mix_a 0","real=0.000")]:
    pos(c, e)
sw(("route_l_a",1),("route_l_b",1),("route_r_a",1),("route_r_b",1),("out_l",1),("out_r",1))
chao = med("CHAO (tom desligado)")
pos("set tone_mix 0.30", "real=0.300")
base = med("BASE (matriz cheia)")

print("\n--- ISOLAMENTO DA MATRIZ (o que antes nao se testava) ---")
sw(("route_l_a",0)); a = med("route_l_a=0   A nao entra em L")
sw(("route_l_a",1)); sw(("route_r_a",0)); b = med("route_r_a=0   A nao entra em R")
sw(("route_r_a",1))

print("\n--- ISOLAMENTO DO MUTE ---")
sw(("out_l",0)); c = med("out_l=0       corta o ramo L")
sw(("out_l",1)); sw(("out_r",0)); d = med("out_r=0       corta o ramo R")
sw(("out_r",1))

print("\n--- A INVERSAO: e para isto que a matriz existe ---")
sw(("route_l_a",1),("route_r_a",0)); e = med("A so no L")
sw(("route_l_a",0),("route_r_a",1)); f = med("A so no R  (INVERTIDO)")
sw(("route_l_a",1),("route_r_a",1))

print("\n--- O DELAY NAO SEGUE A MATRIZ ---")
for c2, e2 in [("set dly_time 0.35","real=0.350"), ("set dly_on_a 1","real=1.000"),
               ("set dly_mix_a 0.90","real=0.900")]:
    pos(c2, e2)
g = med("delay ligado, matriz cheia")
sw(("route_r_a",0)); h = med("delay ligado, route_r_a=0")
sw(("route_r_a",1))

def v(rot, val, cond): print("  %-42s %s" % (rot, "PASSA" if cond else "*** FALHA ***"), val)
print("\n=== VEREDITO ===")
v("matriz corta A em L, e SO em L",
  "L %+.1f / R %+.1f" % (a[0]-base[0], a[1]-base[1]),
  a[0]-base[0] < -25 and abs(a[1]-base[1]) < 2)
v("matriz corta A em R, e SO em R",
  "L %+.1f / R %+.1f" % (b[0]-base[0], b[1]-base[1]),
  b[1]-base[1] < -25 and abs(b[0]-base[0]) < 2)
v("mute corta L, e SO L",
  "L %+.1f / R %+.1f" % (c[0]-base[0], c[1]-base[1]),
  c[0]-base[0] < -25 and abs(c[1]-base[1]) < 2)
v("mute corta R, e SO R",
  "L %+.1f / R %+.1f" % (d[0]-base[0], d[1]-base[1]),
  d[1]-base[1] < -25 and abs(d[0]-base[0]) < 2)
v("INVERSAO: A sai do L e aparece no R",
  "so-L: L %.1f R %.1f | so-R: L %.1f R %.1f" % (e[0],e[1],f[0],f[1]),
  e[0]-e[1] > 25 and f[1]-f[0] > 25)
v("delay sobrevive a route_r_a=0 (acima do chao)",
  "R %.1f vs chao %.1f" % (h[1], chao[1]), h[1]-chao[1] > 12)
for c2, e2 in [("set tone_mix 0","real=0.000"), ("set dly_on_a 0","real=0.000"),
               ("set dly_mix_a 0","real=0.000")]:
    pos(c2, e2)
print("\ntom e delay desligados, confirmados.")
