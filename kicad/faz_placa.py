#!/usr/bin/env python3
"""faz_placa — do esquema ao pacote da fabrica, num comando.

POR QUE EXISTE.

Cada passo destes ja existia e funcionava. O que nao existia era a ORDEM, e a
ordem e metade do problema: a costura tem de vir depois da colocacao, o
roteamento depois da costura, a serigrafia depois do roteamento, e o portao
depois de tudo. Um passo fora de sitio nao da erro — da uma placa que passa em
todos os testes e esta errada (15cr).

⚠️ E COMECA POR APAGAR TODO O COBRE. Quando a colocacao pode mudar, o cobre que
tocava nas pecas antigas e lixo: pistas a apontar para onde a peca ja nao esta
dao curtos-circuitos que aparecem como "linha de base". Nao se remenda. Numa
cadeia automatizada, roteat de raiz custa dois minutos, e e por isso que a
cadeia se paga.

⚠️ A ORDEM DE ATAQUE DO ROTEADOR ESCOLHE-SE MEDINDO, nao opinando. Um roteador
guloso da resultados diferentes conforme quem entra primeiro, e qual e a melhor
DEPENDE DA PLACA. Medido, de raiz e ate convergir:

    cerebro:  curta 19 · drc 23 · longa 29
    painel:   drc 10   · longa 11 · curta 13

A `longa` nunca ganhou, e e a mais lenta: saiu do padrao.

Nao ha ordem vencedora, ha vencedora nesta placa. Por isso esta ferramenta
corre as tres em copias e fica com a melhor — e uma decisao a menos para
alguem tomar, que e o que a automacao serve para fazer.

⚠️ E SO UM DE CADA VEZ. Corri uma comparacao de ordens e este comando ao mesmo
tempo, sobre os mesmos ficheiros, e as duas passaram meia hora a escrever por
cima uma da outra. O resultado nao foi um erro: foi um NUMERO — 34 violacoes de
geometria que nao existiam em placa nenhuma. Uma medicao contaminada tem o
mesmo aspecto de uma boa, e por isso ha um ferrolho.

USO:  faz_placa.py <cerebro|painel|todas> [--sem-zerar]
"""
import os
import re
import shutil
import subprocess
import sys

VERSAO = "0.7.1"
AQUI = os.path.dirname(os.path.abspath(__file__))
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
# ⚠️ A `longa` SAIU DA LISTA, por medicao e nao por gosto: nunca ganhou em placa
# nenhuma (cerebro 29 contra 19, painel 11 contra 10) e e de longe a mais lenta,
# porque atacar primeiro as ligacoes dificeis e gastar o tecto de nos nas
# buscas que vao falhar. Continua alcancavel com `roteia.py --ordem longa` se
# alguma placa a quiser; o que sai e o PADRAO, nao a opcao.
ORDENS = ("drc", "curta")
PASSADAS = 3


class Ferrolho:
    """Um de cada vez sobre os mesmos ficheiros. ⚠️ Nao e paranoia: duas
    escritas concorrentes sobre um .kicad_pcb nao dao excepcao, dao numeros."""

    def __enter__(self):
        self.f = os.path.join(AQUI, ".faz_placa.lock")
        if os.path.exists(self.f):
            quem = open(self.f).read().strip()
            try:
                os.kill(int(quem), 0)
                raise SystemExit("*** ja ha um faz_placa a correr (pid %s).\n"
                                 "    Duas escritas sobre a mesma placa nao dao "
                                 "erro, dao numeros errados." % quem)
            except (ValueError, ProcessLookupError):
                pass            # ferrolho de um processo que ja morreu
        open(self.f, "w").write(str(os.getpid()))
        return self

    def __exit__(self, *_):
        if os.path.exists(self.f):
            os.remove(self.f)


def py(*a, **kw):
    return subprocess.run([sys.executable] + [str(x) for x in a],
                          capture_output=True, text=True, cwd=AQUI, **kw)


def zera(pcb):
    t = open(pcb, encoding="utf-8").read()
    n = len(re.findall(r"\n\t\(segment", t)), len(re.findall(r"\n\t\(via", t))
    t = re.sub(r"\n\t\(segment[\s\S]*?\n\t\)", "", t)
    t = re.sub(r"\n\t\(via[\s\S]*?\n\t\)", "", t)
    open(pcb, "w", encoding="utf-8").write(t)
    return n


def por_fazer(pcb):
    """Ligacoes por fazer + violacoes de geometria. ⚠️ As duas, sempre: a ordem
    que fecha mais ligacoes pode ser a que estraga mais cobre, e escolher pela
    primeira sem olhar a segunda e escolher o pior resultado com confianca."""
    r = subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--save-board",
                        "--severity-error", "--format", "json",
                        "-o", "/tmp/_fp.json", pcb], capture_output=True)
    import json
    d = json.load(open("/tmp/_fp.json"))
    geo = sum(1 for x in d.get("violations", [])
              if x["type"] not in ("silk_over_copper", "silk_overlap",
                                   "silk_edge_clearance", "lib_footprint_mismatch",
                                   "lib_footprint_issues", "starved_thermal"))
    return len(d.get("unconnected_items", [])), geo


def uma(nome, zerar):
    pcb = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_pcb" % nome)
    print("\n══ %s ══" % nome)
    if zerar:
        s, v = zera(pcb)
        print("  0. cobre apagado: %d pistas, %d vias" % (s, v))
    r = py("pcb_contorno.py", nome)
    if r.returncode:
        raise SystemExit("*** pcb_contorno falhou:\n" + (r.stderr or r.stdout))
    for l in r.stderr.splitlines():
        if "costura" in l or "FORA" in l:
            print("  1.%s" % l)

    guardado = pcb + ".limpo"
    shutil.copy(pcb, guardado)
    melhor = None
    for ordem in ORDENS:
        shutil.copy(guardado, pcb)
        # ⚠️ PARA QUANDO DEIXA DE GANHAR, em vez de correr um numero fixo de
        # passadas. Medido: a 1a fecha 88 ligacoes, a 2a fecha 9, a 3a fecha
        # ZERO — e a 3a custa o mesmo tempo que as outras. Um numero fixo de
        # passadas gasta sempre a ultima a provar que nao havia mais nada.
        # ⚠️ E OLHA O CODIGO DE SAIDA. O roteador recusa-se a gravar quando nao
        # consegue provar o que fez, e nesse caso repoe a placa como estava —
        # correctamente. Mas eu ignorava o codigo de saida, e uma passada
        # falhada era indistinguivel de uma passada que nao tinha o que fazer:
        # a cadeia seguia calada e o numero final era outro sem eu saber porque.
        antes = None
        for i in range(PASSADAS):
            r = py("roteia.py", pcb, "--escreve", "--ordem", ordem)
            if r.returncode:
                ult = [l for l in (r.stderr or "").splitlines() if l.strip()]
                print("     ⚠️ passada %d recusou gravar: %s"
                      % (i + 1, ult[-1] if ult else "sem mensagem"))
                break
            m = re.search(r"gravado: (\d+) segmentos", r.stderr or "")
            n = int(m.group(1)) if m else 0
            # ⚠️ para quando deixa de ganhar: medido, a 1a passada fecha 88
            # ligacoes, a 2a fecha 9 e a 3a fecha ZERO — e custa o mesmo tempo
            if n == 0 and antes is not None:
                break
            antes = n
        f, g = por_fazer(pcb)
        marca = ""
        if melhor is None or (g, f) < (melhor[1], melhor[0]):
            melhor = (f, g, ordem)
            shutil.copy(pcb, pcb + ".melhor")
            marca = "  <- fica"
        print("  2. ordem %-6s -> %2d por fazer · geometria %d%s" % (ordem, f, g, marca))
    shutil.move(pcb + ".melhor", pcb)
    os.remove(guardado)
    print("  2. escolhida a ordem %s por MEDICAO, nao por opiniao" % melhor[2])

    # ⚠️ O `--arranca` NAO ENTRA NA CADEIA, e a razao e medicao — nao gosto.
    #
    # Arrancar o que estorva, passar, e refazer o que se arrancou e mesmo a
    # capacidade que falta: numa passada isolada levou 15 ligacoes por fazer
    # para 8, e e a unica coisa que ataca o problema real (para as ultimas nao
    # ha caminho livre, e nenhuma busca acha um caminho que nao ha).
    #
    # Mas na cadeia inteira mediu 14 contra 12 — porque o que se arranca volta
    # para a fila e nem sempre encontra a volta que se esperava — e uma corrida
    # entregou a placa com **um plano de terra partido** que o auto-teste do
    # roteador nao apanhou. Foi o portao da fabrica que o apanhou, e por isso e
    # que ele existe: a ultima linha de defesa nao pode ser a mesma ferramenta
    # que fez o trabalho.
    #
    # Fica em `roteia.py --arranca` ate o auto-teste dele cobrir este caso.
    # Uma capacidade que fecha ligacoes e parte planos nao esta pronta para
    # correr sozinha.
    #
    # ⚠️ E SO AGORA A PASSADA APERTADA, sobre o que sobrou. Com a folga larga o
    # roteador nao consegue SAIR DE ENTRE OS PINOS de um conector: num header
    # de 2,54 com pads de 1,70 o vao e 0,840 mm e ele precisa de 0,950. Isso
    # explicava 9 falhas no J14, 8 no J1 e 11 na Daisy — todas peças de duas
    # filas, e nenhuma delas com pistas a volta. Um sitio com zero pistas num
    # raio de 8 mm nao esta congestionado: esta fechado por uma regra minha.
    #
    # ⚠️ APERTAR DESDE O INICIO MEDE-SE PIOR (23 -> 34): com mais espaco o
    # roteador ganancioso enfia pistas cedo e fecha corredores de que as
    # ligacoes seguintes precisavam. No fim nao ha esse risco.
    #
    # ⚠️ E a malha tem de apertar com a folga, porque a margem da grelha
    # (PASSO·0,71) conta no mesmo corredor. Malha fina na placa toda esgota a
    # busca, logo a busca fica presa a uma CAIXA em volta de cada ligacao — o
    # que sobra de uma passada larga e quase sempre curto.
    r = py("roteia.py", pcb, "--escreve", "--folga", "0.22",
           "--passo", "0.0847", "--caixa", "25")
    f, g = por_fazer(pcb)
    if r.returncode:
        print("  2b. apertada recusou gravar (a placa fica como estava)")
    else:
        print("  2b. apertada (folga 0,22) -> %d por fazer · geometria %d" % (f, g))

    r = py("arruma_silk.py", pcb, "--escreve")
    for l in r.stderr.splitlines()[-2:]:
        print("  3.%s" % l)
    return melhor


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    print("faz_placa v%s" % VERSAO)
    quais = ["cerebro", "painel"] if sys.argv[1] == "todas" else [sys.argv[1]]
    zerar = "--sem-zerar" not in sys.argv
    with Ferrolho():
        for q in quais:
            uma(q, zerar)
        print("\n══ o portao ══")
        r = py("para_fabrica.py", sys.argv[1])
    print(r.stdout.rstrip())
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
