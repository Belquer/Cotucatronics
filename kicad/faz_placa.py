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

VERSAO = "0.14.0"
AQUI = os.path.dirname(os.path.abspath(__file__))
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
# ⚠️ A `longa` SAIU DA LISTA, por medicao e nao por gosto: nunca ganhou em placa
# nenhuma (cerebro 29 contra 19, painel 11 contra 10) e e de longe a mais lenta,
# porque atacar primeiro as ligacoes dificeis e gastar o tecto de nos nas
# buscas que vao falhar. Continua alcancavel com `roteia.py --ordem longa` se
# alguma placa a quiser; o que sai e o PADRAO, nao a opcao.
ORDENS = ("drc", "curta")
FREEROUTING = all(os.path.exists(f) for f in (
    "/opt/homebrew/opt/openjdk/bin/java",
    os.path.expanduser("~/.local/share/freerouting/freerouting-2.4.1.jar"),
    "/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework"
    "/Versions/3.9/bin/python3.9"))
PASSADAS = 3
PASSADAS_FR = 4   # quantas vezes o FreeRouting continua antes de desistir

# ⚠️ §3 DO ppc/002: "toda corrida de gera_* e seguida de sincroniza_pcb +
# funcao_dos_pads + ERC, ou nao aconteceu." Estes sao os geradores de FOLHA que
# ESCREVEM ficheiro. Os de simbolo (gera_daisy_sym, gera_simbolos) emitem para a
# biblioteca partilhada e sao manutencao a montante, fora desta cadeia. A lista
# e EXPLICITA e nao um glob: e o idioma do resto do ficheiro (ORDENS, PLACAS do
# para_fabrica) e um gerador acrescentado entra por decisao, nao por descoberta.
GERADORES = {
    "cerebro": ("gera_soquete.py", "gera_conector.py"),
    "painel": ("gera_painel.py",),
}


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


def pyd(cwd, *a):
    """Como `py`, mas noutro directorio — os geradores de folha escrevem em
    caminhos RELATIVOS (`cotuca_soquete.kicad_sch`), logo correm na pasta da
    placa."""
    return subprocess.run([sys.executable] + [str(x) for x in a],
                          capture_output=True, text=True, cwd=cwd)


def regenera(nome):
    """§3 do ppc/002: regenera as folhas e SO ENTAO as da por feitas —
    sincroniza_pcb -> funcao_dos_pads -> ERC, "ou nao aconteceu".

    ⚠️ A TRINCA CORRE UMA VEZ POR PLACA, depois de TODOS os geradores dela, e
    nao a seguir a cada gerador: o netlist e HIERARQUICO e o `sincroniza_pcb`
    exporta a folha inteira pelo `kicad-cli`. Sincronizar a meio de uma
    regeneracao le um estado incoerente (falta uma folha). "Cada corrida de
    gera_*" e, na pratica, cada regeneracao da placa.

    ⚠️ ABORTA A CADEIA se qualquer passo falhar — nao ha placa que se rotee a
    partir de uma folha por sincronizar ou por verificar. E os passos que
    precisam do `kicad-cli` (o netlist do sincronizador e o ERC) FALHAM ALTO
    numa maquina sem KiCad, em vez de serem saltados em silencio: um binario em
    falta tem de parar a cadeia, nao passar por ela.
    """
    subdir = os.path.join(AQUI, "cotuca_%s" % nome)
    print("  ⟳ regenera (§3): geradores → sincroniza_pcb → funcao_dos_pads → ERC")
    for g in GERADORES[nome]:
        r = pyd(subdir, os.path.join(subdir, g))
        if r.returncode:
            raise SystemExit("*** %s falhou:\n%s" % (g, r.stderr or r.stdout))
    r = py("sincroniza_pcb.py", nome)
    if r.returncode:
        raise SystemExit("*** sincroniza_pcb falhou:\n" + (r.stderr or r.stdout))
    r = py("funcao_dos_pads.py")
    if r.returncode:
        # ⚠️ A verificacao que apanha os criticos de FUNCAO (simbolo generico ×
        # footprint polarizado) — nem ERC nem DRC a veem. Se ela falha, a folha
        # foi regenerada com um defeito de significado e o cobre nao se toca.
        raise SystemExit("*** funcao_dos_pads falhou — o pad certo nao leva a "
                         "rede certa:\n" + (r.stdout or r.stderr))
    sch = os.path.join(subdir, "cotuca_%s.kicad_sch" % nome)
    r = subprocess.run([KICAD, "sch", "erc", "--exit-code-violations",
                        "-o", "/tmp/_erc_%s.rpt" % nome, sch],
                       capture_output=True, text=True)
    if r.returncode:
        raise SystemExit("*** ERC acusou violacoes (ou falta o kicad-cli): %s\n"
                         "    relatorio em /tmp/_erc_%s.rpt"
                         % ((r.stderr or r.stdout).strip(), nome))


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
    # ⚠️ §3: a regeneracao das folhas + sincroniza + funcao_dos_pads + ERC vem
    # ANTES de tocar no cobre. Sincronizar substitui o F8 (que casa por UUID e
    # duplica pecas num esquema gerado); funcao_dos_pads + ERC provam a folha
    # antes de se rotear a partir dela.
    regenera(nome)
    if zerar:
        s, v = zera(pcb)
        print("  0. cobre apagado: %d pistas, %d vias" % (s, v))
    r = py("pcb_contorno.py", nome)
    if r.returncode:
        raise SystemExit("*** pcb_contorno falhou:\n" + (r.stderr or r.stdout))

    # ⚠️ OS FUROS ENTRAM DEPOIS DA COLOCACAO E ANTES DO ROTEAMENTO.
    #
    # Depois, porque as posicoes so se conferem contra as pecas ja colocadas —
    # e o `confere_furos` recusa se alguma tapar um furo, que e o caso em que
    # o parafuso nao entra com a caixa fechada e a solda feita.
    #
    # ⚠️ E ANTES de rotear, porque cada furo leva um keepout de o7 e o roteador
    # tem de o ver como obstaculo. Pos-los depois obrigava a arrancar o cobre
    # que passasse por la — e arrancar cobre e sempre mais caro que nao o por.
    # ⚠️ `rf` e nao `r`: a variavel `r` ainda pertence ao `pcb_contorno`, e a
    # linha seguinte le o `r.stderr` dele. Reutilizar o nome dava `None` ali e
    # matava a corrida do painel — variavel com dois empregos, familia C.
    rfur = py("poe_furos.py", "--escreve") if nome == "cerebro" else None
    if rfur is not None and rfur.returncode:
        raise SystemExit("*** poe_furos falhou:\n" + (rfur.stderr or rfur.stdout))
    for l in r.stderr.splitlines():
        if "costura" in l or "FORA" in l:
            print("  1.%s" % l)

    # ⚠️⚠️ O FREEROUTING PRIMEIRO, E O MEU ROTEADOR COMO RECUO.
    #
    # Escrevi um roteador de raiz e afinei-o uma sessao inteira porque tinha
    # concluido que nenhum autoroteador externo era alcancavel — o `pcbnew` nao
    # exporta DSN, dizia eu. Exporta (15de). Na mesma placa, de raiz:
    #
    #     o meu       103 -> 12 por ligar · ~9 min as duas placas
    #     FreeRouting 122 ->  8 por ligar · 35 SEGUNDOS as duas
    #
    # E as que sobram ao FreeRouting sao todas ligacoes ao PLANO, nao sinal:
    # ele fecha todos os sinais. Faz rip-up e shove a serio.
    #
    # ⚠️ O MEU FICA, e nao por orgulho: ele corre sem Java e sem o jar de 61 MB,
    # e a cadeia tem de continuar a andar numa maquina onde isso nao esteja.
    if FREEROUTING:
        # ⚠️ A SEQUENCIA INTEIRA, e a ordem e o que a faz funcionar.
        #
        # Nenhum destes passos sozinho fecha a placa; juntos e nesta ordem,
        # fecham. Medido, de raiz:
        #
        #   1 FreeRouting de raiz, com o plano FORA do DSN   147 -> 3 por fazer
        #   2 retrair o que o DRC acusa, e limpar pendurados
        #   3 FreeRouting a CONTINUAR, ate parar de render    -> 2, geometria 0
        #   4 o roteador da casa a acabar as ultimas          -> 0, geometria 0
        #
        # ⚠️ O passo 4 nao e redundancia. As que sobram ao FreeRouting terminam
        # em pads de passo fino, e o roteador da casa tem a fuga apertada so
        # junto as pontas (§12r) — que e exactamente o que elas precisam. Cada
        # um fecha o que o outro nao fecha.
        r = py("roteia_fr.py", nome)
        print("\n".join("  " + l for l in r.stdout.strip().splitlines()
                        if l.strip() and "portao" not in l))
        if r.returncode:
            # ⚠️ E DIZ PORQUE FALHOU. A primeira versao escrevia so "falhou" e
            # deitava fora o stderr — que era onde estava o traceback. Passei
            # duas corridas a nao saber que erro era, quando ele estava escrito
            # e a ser deitado ao lixo por mim.
            print("  ⚠️ FreeRouting falhou (codigo %d); sigo com o roteador da "
                  "casa. O que ele disse:" % r.returncode)
            for l in (r.stderr or "").strip().splitlines()[-8:]:
                print("       %s" % l)
        else:
            f, g = por_fazer(pcb)
            antes = None
            for _ in range(PASSADAS_FR):
                if f == 0 and g == 0:
                    break
                rc = py("roteia_fr.py", nome, "--continuar")
                for l in rc.stdout.splitlines():
                    if "3b." in l or "3c." in l or "FreeRouting:" in l:
                        print("    %s" % l.strip())
                f2, g2 = por_fazer(pcb)
                # ⚠️ so vale se descer: uma passada que nao rende e uma passada
                # que so gastou tempo, e a seguinte nao rende mais.
                if (g2, f2) >= (g, f) and antes is not None:
                    break
                antes, f, g = f, f2, g2
            print("  2. FreeRouting -> %d por fazer · geometria %d" % (f, g))
            # ⚠️ E O ACABAMENTO TAMBEM REPETE ENQUANTO RENDER. Uma passada
            # fecha as que consegue e muda o cobre; a seguinte tem outro tabuleiro
            # e fecha mais. Parar na primeira era deixar ligacoes em cima da
            # mesa por eu nao ter voltado a perguntar.
            for _ in range(3):
                if not f and not g:
                    break
                rf = py("roteia.py", pcb, "--escreve", "--arranca")
                for l in rf.stderr.splitlines():
                    if "roteadas" in l or "gravado" in l or "retirados" in l:
                        print("    %s" % l.strip())
                py("roteia_fr.py", nome, "--continuar")
                f2, g2 = por_fazer(pcb)
                if (g2, f2) >= (g, f):
                    break
                f, g = f2, g2
            print("  2c. o acabamento fechou -> %d por fazer · geometria %d"
                  % (f, g))
            r2 = py("arruma_silk.py", pcb, "--escreve")
            for l in r2.stderr.splitlines()[-1:]:
                print("  3.%s" % l)
            return (f, g, "freerouting+casa")

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
            r = py("roteia.py", pcb, "--escreve", "--ordem", ordem, "--arranca")
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

    # ⚠️ O `--arranca` ESTA NA CADEIA, e nao estava. A historia dele vale a pena:
    #
    # Arrancar o que estorva, passar, e refazer o que se arrancou e a unica
    # coisa que ataca o problema real das ultimas ligacoes — para elas nao ha
    # caminho livre, e nenhuma busca acha um caminho que nao ha. Numa passada
    # isolada levou 15 por fazer para 8.
    #
    # Mas na cadeia media 14 contra 12, e uma corrida entregou a placa com um
    # PLANO DE TERRA PARTIDO que o auto-teste do roteador nao apanhou. Duas
    # coisas estavam mal, e nenhuma era a ideia:
    #
    #  1. o auto-teste contava ligacoes por fazer POR REDE, e um pad que se
    #     ligava mascarava um plano que se partia — o total nao mudava. Passou
    #     a contar por (rede, especie), e ha um `roteia.py --auto-teste` que
    #     interroga a regra com o caso que ela deixou passar.
    #  2. arrancar a rede X para a Y passar troca uma ligacao por outra se a X
    #     nao conseguir refazer-se, e pode ficar pior que antes. Agora guarda o
    #     estado e VOLTA ATRAS se o numero nao descer.
    #
    # Com as duas, mede 12 e nao parte planos: igual a nao arrancar nesta
    # placa, e so pode ajudar noutra. Uma capacidade que so pode melhorar ou
    # nao fazer nada pertence ao caminho por omissao.
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
           "--passo", "0.0847", "--caixa", "25", "--arranca")
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
        # ⚠️ E UMA SEGUNDA OPINIAO QUE NAO FUI EU QUE ESCREVI. Tudo nesta cadeia
        # e meu — o gerador, o roteador, o portao, e os proprios verificadores —
        # e isso tem um limite que nao se conserta escrevendo mais um
        # verificador meu: um auto-teste tem os pontos cegos de quem o escreveu,
        # e ja aconteceu aqui (15dc). O `kicad-happy` e de outra pessoa e olha
        # para coisas que nunca me ocorreram.
        rv = py("revisao_externa.py", sys.argv[1])
    print(r.stdout.rstrip())
    if rv.stdout.strip():
        print("\n══ segunda opiniao (kicad-happy) ══")
        print(rv.stdout.rstrip())
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
