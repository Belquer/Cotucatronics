#!/usr/bin/env python3
"""estado v0.2.0 — guarda o estado dos params antes de gravar e repoe depois.

    ./estado.py guardar <ficheiro>
    ./estado.py repor   <ficheiro>

POR QUE EXISTE (04/09). Gravar apaga tudo o que e volatil: os params
renascem no valor de nascenca do manifesto, nao no que estava configurado.
A regra da bancada ja dizia que repor e DECLARAR o que foi reposto faz
parte de gravar (§2c) — mas era uma regra executada de cabeca, e por isso
falhada. Em 20/08 foram quatro gravacoes seguidas a zerar o `level` sem
reposicao nem aviso, e o dB re-dialava pelo Loopy sem saber porque.

O pedido do dB, 04/09: "faca isso sempre, ao gravar reponha os valores".
Regra que depende de eu lembrar nao e regra; e sorte. Entao mora aqui, e
o `flash.sh` chama-a nos dois lados do DFU.

O QUE ELE NAO FAZ. Nao repoe o que a gravacao mudou DE PROPOSITO: se uma
versao nova nasce com outro valor de fabrica, repor seria desfazer a
alteracao as escondidas. Por isso tudo o que ele mexe e IMPRESSO, com o
antes e o depois, e o que ele nao reconhece e dito em voz alta em vez de
ignorado em silencio.
"""
import re, sys, time, serial

VERSAO = "0.2.0"


def _porta():
    sys.path.insert(0, __file__.rsplit('/', 1)[0])
    import porta_daisy
    for _ in range(40):
        try:
            p, _v = porta_daisy.achar()
            return p
        except Exception:
            time.sleep(1.0)
    return None


def _fala(s, cmd, espera=2.0):
    s.reset_input_buffer()
    s.write(cmd.encode() + b'\n')
    time.sleep(espera)
    return s.read(20000).decode('utf-8', 'replace').splitlines()


def _le(s):
    """{nome: real} lido da placa.

    ⚠️ REAL, e nao NORM. O `set` aceita o valor REAL — `set
    gate_release_a 300` — e responde `fora da faixa: ... aceita [50 ..
    2000]` a qualquer coisa fora dele. A v0.1.0 guardava e mandava o
    NORM: para os params cuja faixa real calha em 0..1 (ganhos, chaves,
    rotas) coincidia e funcionava, e para todos os outros a placa
    recusava. Como eu deitava fora a resposta, a ferramenta imprimia
    "repos" para reposicoes que nunca aconteceram.

    Duas licoes na mesma linha, e ambas ja escritas noutro sitio: deduzi
    o contrato de um comando em vez de o LER, como fizera com o
    `porta_daisy.achar()`; e construi uma verificacao que nao podia
    reprovar, porque nao olhava para a unica coisa que sabia a verdade.
    """
    d = {}
    for ln in _fala(s, 'list', 2.5):
        m = re.match(r'\s*\d+ (\S+)\s+norm=[\d.]+\s+real=(-?[\d.]+)', ln)
        if m:
            d[m.group(1)] = float(m.group(2))
    return d


def guardar(fich):
    p = _porta()
    if not p:
        # Silencio deliberado e silencio acidental sao indistinguiveis do
        # outro lado — entao este diz qual dos dois e.
        print("  estado: NENHUMA PLACA RESPONDEU — nao ha o que guardar.")
        print("  estado: a gravacao segue, mas SEM REPOSICAO no fim.")
        return 0
    s = serial.Serial(p, 115200, timeout=1); time.sleep(1.2)
    d = _le(s); s.close()
    if not d:
        print("  estado: a placa respondeu mas o `list` veio vazio — SEM REPOSICAO.")
        return 0
    with open(fich, 'w') as f:
        for k, v in d.items():
            f.write("%s %.4f\n" % (k, v))
    print("  estado: %d params guardados" % len(d))
    return 0


def repor(fich):
    try:
        antes = dict((l.split()[0], float(l.split()[1])) for l in open(fich))
    except OSError:
        print("  estado: sem instantaneo previo — nada a repor.")
        return 0
    p = _porta()
    if not p:
        print("  estado: *** A PLACA NAO RESPONDE — %d params NAO foram repostos ***" % len(antes))
        return 1
    s = serial.Serial(p, 115200, timeout=1); time.sleep(1.2)
    agora = _le(s)
    if not agora:
        print("  estado: *** `list` vazio — %d params NAO foram repostos ***" % len(antes))
        s.close(); return 1

    sumidos = [k for k in antes if k not in agora]
    novos   = [k for k in agora if k not in antes]
    mudou, recusou = [], []
    for k, v in antes.items():
        if k not in agora:
            continue
        # tolerancia RELATIVA: 0,0005 absoluto e certo para um ganho 0..1
        # e absurdo para um tempo em ms ou uma frequencia em Hz, onde a
        # propria impressao so tem 4 casas.
        if abs(agora[k] - v) <= max(0.0005, abs(v) * 1e-4):
            continue
        # ⚠️ LER A RESPOSTA. Ela e a unica coisa que sabe se o `set`
        # pegou: ou diz `nome -> real=...`, ou diz `fora da faixa`.
        r = " ".join(_fala(s, "set %s %.4f" % (k, v), 0.35))
        if "-> real=" in r:
            mudou.append((k, agora[k], v))
        else:
            recusou.append((k, agora[k], v, r.strip() or "(sem resposta)"))

    conf = _le(s); s.close()
    for k, de, para in mudou:
        vis = conf.get(k, float('nan'))
        ok  = abs(vis - para) <= max(0.0005, abs(para) * 1e-3)
        print("  repos %-20s %.4f -> %.4f%s"
              % (k, de, para, "" if ok else "  *** leio %.4f ***" % vis))
    for k, de, para, r in recusou:
        print("  *** RECUSADO %-16s %.4f -> %.4f: %s" % (k, de, para, r))
    for k in sumidos:
        print("  estado: `%s` existia antes e sumiu nesta versao — nao reposto." % k)
    for k in novos:
        print("  estado: `%s` e novo nesta versao — fica no valor de nascenca." % k)
    if not mudou and not recusou:
        print("  estado: %d params conferidos, nenhum divergiu — nada a repor." % len(antes))
    else:
        print("  estado: %d reposto(s), %d recusado(s), %d ja iguais."
              % (len(mudou), len(recusou), len(antes) - len(mudou) - len(recusou)))
    return 1 if recusou else 0


if __name__ == '__main__':
    print("estado v%s" % VERSAO)
    if len(sys.argv) != 3 or sys.argv[1] not in ('guardar', 'repor'):
        print(__doc__); sys.exit(2)
    sys.exit((guardar if sys.argv[1] == 'guardar' else repor)(sys.argv[2]))
