#!/usr/bin/env python3
"""modo_exciter v0.1.1 — devolve a proteccao toda antes de o exciter voltar.

    ./modo_exciter.py            # boost a 1.0 (unidade), o estado conhecido-bom
    ./modo_exciter.py 3          # boost a 3x, se o instrumento estiver fraco

POR QUE EXISTE (04/09). Em modo jack o exciter esta fora do circuito: nao ha
laco para quebrar, e a proteccao so custa timbre. Por isso, para uma
demonstracao pelo jack, desligou-se TUDO — supressor, bocas, portao do
exciter (que e quem corre o shifter e o Governor) — e subiu-se o
`bench_boost` ao topo.

Cada uma dessas coisas e segura sozinha em modo jack e PERIGOSA no instante
em que o exciter volta ao ar. E sao cinco, espalhadas por nomes que nao se
parecem uns com os outros. Repo-las a mao, a meio de uma sessao, e onde se
esquece uma — e a que se esquece e a que morde.

⚠️ NAO E O INVERSO DE UM INTERRUPTOR. Voltar ao exciter e uma LISTA, e esta
aqui inteira para nao viver na cabeca de ninguem.
"""
import sys, time, serial

VERSAO = "0.1.1"

# As oito bocas que o SuMiA™ achou sozinho em 04/09, medidas na escultura
# real e nao no banco de ensaio. Nao vivem em preset nenhum e a gravacao
# apaga-as: e daqui que se repoem. Regra do dB (02/09): "repoe as bocas
# conhecidas e deixa o supressor so preencher as livres".
BOCAS = [
    ("microfonia1_hz_l", 3025.7), ("microfonia1_db_l", 18),
    ("microfonia2_hz_l", 4523.4), ("microfonia2_db_l", 24),
    ("microfonia3_hz_l",  790.9), ("microfonia3_db_l", 18),
    ("microfonia4_hz_l", 5434.5), ("microfonia4_db_l", 18),
    ("microfonia1_hz_r", 4523.1), ("microfonia1_db_r", 24),
    ("microfonia2_hz_r",  804.6), ("microfonia2_db_r", 24),
    ("microfonia3_hz_r", 3025.0), ("microfonia3_db_r", 18),
    ("microfonia4_hz_r", 5126.9), ("microfonia4_db_r", 18),
]


def main():
    print("modo_exciter v%s" % VERSAO)
    try:
        boost = float(sys.argv[1]) if len(sys.argv) > 1 else 1.0
    except ValueError:
        print(__doc__); return 2
    if not (1.0 <= boost <= 8.0):
        # ⚠️ A faixa e [1,8] no manifesto. Um valor fora dela seria RECUSADO
        # pela placa com "fora da faixa" — e este script estaria a anunciar
        # um ganho baixado que nunca baixou, no exacto momento em que o laco
        # se fecha. Ver battle_scars 15bn.
        print("  boost fora de [1, 8] — a placa recusaria e eu diria que tinha feito.")
        return 2
    sys.path.insert(0, __file__.rsplit('/', 1)[0])
    import estado
    p = estado._porta()
    if not p:
        print("  *** A PLACA NAO RESPONDE — NADA FOI REPOSTO ***")
        return 1
    s = serial.Serial(p, 115200, timeout=1); time.sleep(1.2)

    # ORDEM IMPORTA: baixar o ganho ANTES de fechar o laco, e por as bocas
    # ANTES de ligar o caçador. Errar a ordem e abrir o laco quente.
    passos = [
        ("baixa o ganho de bancada",  [("bench_boost_l", boost), ("bench_boost_r", boost)]),
        ("repoe as 8 bocas",          BOCAS),
        ("liga os notches",           [("microfonia_on_l", 1), ("microfonia_on_r", 1)]),
        ("portao do exciter: shifter + Governor",
                                      [("exc_filter_l", 1), ("exc_filter_r", 1)]),
        ("liga o SuMiA™",             [("micro_auto", 1)]),
    ]
    mau = 0
    for nome, cmds in passos:
        print("  %s" % nome)
        for k, v in cmds:
            r = " ".join(estado._fala(s, "set %s %s" % (k, v), 0.3))
            if "-> real=" not in r:
                print("    *** %s FALHOU: %s" % (k, r.strip() or "(sem resposta)")); mau += 1

    d = estado._le(s); s.close()
    print()
    print("  micro_auto %.0f | notch L %.0f R %.0f | portao L %.0f R %.0f | boost %.1f/%.1f"
          % (d.get("micro_auto", -1), d.get("microfonia_on_l", -1), d.get("microfonia_on_r", -1),
             d.get("exc_filter_l", -1), d.get("exc_filter_r", -1),
             d.get("bench_boost_l", -1), d.get("bench_boost_r", -1)))

    # ⚠️ ESTE NAO SE MEXE SOZINHO — e escolha musical, nao seguranca.
    diag = d.get("route_b_l", 0) >= 0.5 or d.get("route_a_r", 0) >= 0.5
    if diag:
        print()
        print("  ⚠️ AS DIAGONAIS DA MATRIZ ESTAO ABERTAS (route_b_l / route_a_r).")
        print("     Medido em 31/08: matriz toda aberta = +24,4 dB, dominancia 0,99")
        print("     (uivo). So A->L e B->R = -24,3 dB. Sao 49 dB de margem de laco,")
        print("     e as diagonais fecham o laco pelos DOIS corpos — o tarol esta")
        print("     soldado ao botijao. Nao mexo nisto: e escolha sua.")
        print("       set route_b_l 0 · set route_a_r 0")
    print()
    print("  FALHAS: %d" % mau if mau else "  tudo reposto e conferido.")
    return 1 if mau else 0


if __name__ == '__main__':
    sys.exit(main())
