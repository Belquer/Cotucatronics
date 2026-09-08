#!/usr/bin/env python3
"""roteia_fr — roteamento pelo FreeRouting, que e um roteador a serio.

POR QUE EXISTE, E POR QUE NAO EXISTIA ANTES.

Escrevi um roteador de raiz e passei uma sessao a afina-lo porque tinha
concluido que **nao havia export de DSN** e que nenhum autoroteador externo era
alcancavel. Era falso: o `pcbnew` expoe `ExportSpecctraDSN` e
`ImportSpecctraSES`. Ver cicatriz 15de.

A comparacao, na mesma placa e de raiz:

    o meu roteador   103 -> 12 por ligar, em ~4 minutos
    o FreeRouting    109 ->  6 por ligar, em 21 SEGUNDOS, 0 violacoes

E as 6 dele sao ligacoes ao plano de terra, nao sinal: **ele fecha todos os
sinais**. Faz rip-up e shove a serio, que o meu faz mal ou nao faz.

⚠️ CORRE COM O PYTHON DO PROPRIO KICAD. O `pcbnew` importado do Python do
sistema falha no `dlopen` a pedir as bibliotecas wx do KiCad. O interpretador
do bundle esta em Contents/Frameworks/Python.framework/.../bin/python3.9 e
importa sem um queixume.

TRES COISAS QUE O FREEROUTING NAO SABE, E QUE ESTA FERRAMENTA TRATA:

⚠️ 1. NET-TIES. A ponte entre AGND e GND aparece-lhe como cobre "sem rede", e
   ele roteia por cima: dois `shorting_items` na primeira tentativa. Poe-se um
   keepout por cima dela no DSN.

⚠️ 2. A LARGURA MINIMA. Ele estreita a pista para sair de um pad de passo fino
   — que e como a industria resolve o problema que me tinha bloqueado — e isso
   viola uma regra de largura minima demasiado apertada. A regra da placa
   ajusta-se ao que a fabrica faz, e nao ao contrario.

⚠️ 3. `-mt 1`, E NAO MAIS. O proprio FreeRouting avisa que a optimizacao
   multi-tarefa esta partida e gera violacoes de folga. Um aviso da propria
   ferramenta sobre si mesma nao se ignora por pressa.

⚠️ E ELE TELEFONA PARA CASA. Tenta enviar telemetria para api.freerouting.app a
   cada arranque. Nao ha opcao documentada para desligar; a chamada falha nesta
   maquina, mas fica dito.

USO:  roteia_fr.py <cerebro|painel|todas> [--passagens N]
"""
import os
import re
import subprocess
import sys

VERSAO = "0.10.2"
AQUI = os.path.dirname(os.path.abspath(__file__))
PY_KICAD = ("/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework"
            "/Versions/3.9/bin/python3.9")
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
JAVA = "/opt/homebrew/opt/openjdk/bin/java"
JAR = os.path.expanduser("~/.local/share/freerouting/freerouting-2.4.1.jar")
PASSAGENS = 100
FOLGA_KEEPOUT = 2000        # um em volta do net-tie, em unidades do DSN (um)


def kicad_py(codigo):
    r = subprocess.run([PY_KICAD, "-c", codigo], capture_output=True, text=True)
    if r.returncode:
        raise SystemExit("*** o Python do KiCad falhou:\n%s" % r.stderr)
    return r.stdout.strip()


def redes_com_plano(t):
    """As redes que tem zona de cobre nesta placa.

    ⚠️⚠️ E O CAMPO CHAMA-SE `net`, NAO `net_name`. Escrevi tres filtros a
    procurar `(net_name "X")` dentro de `(zone ...)`, e o KiCad 10 escreve
    `(net "X")`. Os tres devolviam SEMPRE conjunto vazio — e um filtro vazio nao
    da erro: da a resposta mais tranquilizadora que existe.

    Li dezenas de vezes a linha "0 de 34 travessias passam perto de um pad do
    plano" e li-a como um achado. Nao era: era isto. E a guarda dos pads do
    plano em B.Cu, que medi e declarei inutil, nunca chegou a fazer nada —
    portanto o que eu medi foi duas vezes a mesma coisa.

    ⚠️ POR ISSO ESTA FUNCAO GRITA. Se a placa tem `(zone` e ela nao achar rede
    nenhuma, o silencio deixa de ser uma opcao."""
    redes = set()
    for m in re.finditer(r'\(zone\b', t):
        j = m.start()
        d = 0
        for k in range(j, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    break
        cab = t[j:min(k, j + 400)]
        n = re.search(r'\(net_name "([^"]*)"\)', cab) or \
            re.search(r'\(net "([^"]+)"\)', cab)
        if n and n.group(1) and not n.group(1).isdigit():
            redes.add(n.group(1))
    if "(zone" in t and not redes:
        raise SystemExit("*** a placa tem zonas e eu nao achei a rede de "
                         "nenhuma. O filtro esta partido, e um filtro partido "
                         "devolve vazio sem se queixar.")
    return redes


def keepouts_dos_nettie(dsn):
    """Um keepout por cada net-tie. ⚠️ Sem isto o FreeRouting roteia por cima
    do cobre da ponte, que ele ve como 'sem rede', e curto-circuita-a."""
    n = 0
    for m in re.finditer(r'\(component "?NetTie[^"\n]*"?\s*\n\s*\(place (\w+) '
                         r'(-?\d+) (-?\d+)', dsn):
        ref, x, y = m.group(1), int(m.group(2)), int(m.group(3))
        k = FOLGA_KEEPOUT + 3000
        bloco = ('    (keepout "%s" (rect F.Cu %d %d %d %d))\n'
                 '    (keepout "%s" (rect B.Cu %d %d %d %d))\n'
                 % (ref, x - k, y - k, x + k, y + k,
                    ref, x - k, y - k, x + k, y + k))
        i = dsn.index("(structure")
        j = dsn.index("\n", i) + 1
        dsn = dsn[:j] + bloco + dsn[j:]
        n += 1
    return dsn, n


def uma(nome, passagens):
    pcb = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_pcb" % nome)
    dsn, ses = "/tmp/fr_%s.dsn" % nome, "/tmp/fr_%s.ses" % nome
    print("\n══ %s ══" % nome)

    # ⚠️⚠️ COMECA POR APAGAR O COBRE, E ISSO INCLUI A COSTURA.
    #
    # O FreeRouting roteia de raiz e sabe ligar um pad ao plano sozinho. As
    # vias de costura que o `pcb_contorno` poe para os pads SMD deixam de ser
    # redundantes e passam a ATRAPALHAR: medido, com elas o cerebro sai com o
    # plano de terra PARTIDO e mais quatro ligacoes por fazer; sem elas, o
    # plano fica inteiro.
    #
    # Uma peca que era necessaria com uma ferramenta pode ser um estorvo com
    # outra. A costura nao estava errada — estava a resolver um problema que
    # este roteador ja nao tem.
    tt = open(pcb, encoding="utf-8").read()
    n_s = len(re.findall(r"\n\t\(segment", tt))
    n_v = len(re.findall(r"\n\t\(via", tt))
    if n_s or n_v:
        tt = re.sub(r"\n\t\(segment[\s\S]*?\n\t\)", "", tt)
        tt = re.sub(r"\n\t\(via[\s\S]*?\n\t\)", "", tt)
        open(pcb, "w", encoding="utf-8").write(tt)
        print("  0. cobre apagado: %d pistas, %d vias (a costura tambem)"
              % (n_s, n_v))

    kicad_py("import pcbnew;b=pcbnew.LoadBoard(%r);"
             "assert pcbnew.ExportSpecctraDSN(b,%r)" % (pcb, dsn))
    t = open(dsn, encoding="utf-8").read()
    t, n = keepouts_dos_nettie(t)

    # ⚠️⚠️ O PLANO SAI DO DSN, E FOI ISTO QUE FECHOU A PLACA.
    #
    # O DSN leva os planos como `(plane GND (polygon ...))`, e o FreeRouting
    # respeita-os: considera um pad LIGADO se ele cair dentro do poligono, e
    # nao lhe roteia nada. O KiCad so o considera ligado se o cobre ENCHIDO lhe
    # chegar — e a volta de um conector cercado por pistas nao chega. Os dois
    # tem razao sobre coisas diferentes, e o resultado eram 15 ligacoes de
    # terra por fazer que nenhum dos dois se achava responsavel por resolver.
    #
    # Sem a declaracao, a terra passa a ser rede como as outras e ele roteia-a
    # com pista e via. Medido, na mesma placa e de raiz:
    #
    #     com plano no DSN:  122 ligacoes -> 5 por fazer (FR) · 15 (KiCad)
    #     sem plano no DSN:  147 ligacoes -> 0 por fazer (FR) ·  0 (KiCad)
    #
    # ⚠️ E O PLANO NAO DESAPARECE. Ele continua no `.kicad_pcb` e enche por
    # baixo; o que muda e deixar de ser o MECANISMO de ligacao e passar a ser
    # reforco. Cobre a mais numa rede de terra nao faz mal nenhum — o que fazia
    # mal era contar com ele e ninguem verificar se chegou.
    n_pl = len(re.findall(r"\(plane ", t))
    t = re.sub(r'\s*\(plane \w+ \(polygon[\s\S]*?\n\s{4}\)', "", t)
    open(dsn, "w", encoding="utf-8").write(t)
    print("  1. DSN exportado · %d keepout(s) sobre net-ties · %d plano(s) "
          "retirados (a terra vai ser roteada, nao presumida)" % (n, n_pl))

    r = subprocess.run([JAVA, "-jar", JAR, "-de", dsn, "-do", ses,
                        "-mp", str(passagens), "-mt", "1"],
                       capture_output=True, text=True)
    saida = (r.stdout or "") + (r.stderr or "")
    m = re.search(r"started with (\d+) unrouted nets.*?final score: [\d.]+ "
                  r"\((\d+) unrouted and (\d+) violations\)", saida, re.S)
    if m:
        print("  2. FreeRouting: %s ligacoes -> %s por fazer · %s violacao(oes)"
              % m.groups())
    for l in saida.splitlines():
        if "could not be routed" in l or re.match(r"\s+- \S+\s+->", l):
            print("     %s" % l.strip())
    if not os.path.exists(ses):
        raise SystemExit("*** o FreeRouting nao produziu sessao:\n%s"
                         % saida[-2000:])

    kicad_py("import pcbnew;b=pcbnew.LoadBoard(%r);"
             "assert pcbnew.ImportSpecctraSES(b,%r);"
             "assert pcbnew.SaveBoard(%r,b)" % (pcb, ses, pcb))
    print("  3. sessao importada e gravada")

    # ⚠️ E RETIRA O QUE O DRC ACUSA. O `roteia.py` da casa ja fazia isto e o
    # `roteia_fr` nao: importava o que o FreeRouting produzisse, violacoes
    # incluidas. Ele proprio anuncia quantas deixou — 4 numa placa, 3 na outra —
    # e eu lia o numero e seguia.
    #
    # Entregar cobre que o DRC acusa e pior que nao entregar nada, e vale
    # igual para cobre que veio de fora: a origem nao muda a regra.
    subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--severity-error",
                    "--format", "json", "-o", "/tmp/_fr9.json", pcb],
                   capture_output=True)
    import json as _j
    d9 = _j.load(open("/tmp/_fr9.json"))
    # ⚠️ RETIRA-SE O SEGMENTO CULPADO, E NAO A REDE INTEIRA.
    #
    # A primeira versao apagava todo o cobre das redes acusadas. Numa rede de
    # sinal isso e uma ligacao; no GND sao centenas de segmentos, e a placa
    # passou de 2 ligacoes por fazer para 28. O DRC diz a POSICAO do item
    # culpado — usa-se essa, e nao o nome da rede.
    #
    # ⚠️ E as vias nao se tocam: uma via retirada deixa a pista do outro lado
    # pendurada, o que troca um `clearance` por um `via_dangling` e mais uma
    # ligacao por fazer.
    culpados = []
    for x in d9.get("violations", []):
        if x["type"] not in ("clearance", "shorting_items", "tracks_crossing",
                             "copper_edge_clearance", "track_width"):
            continue
        for i in x.get("items", []):
            po = i.get("pos") or {}
            if "Track [" in i.get("description", "") and "x" in po:
                culpados.append((round(po["x"], 4), round(po["y"], 4)))
    if culpados:
        tt = open(pcb, encoding="utf-8").read()
        fora = 0
        for cx, cy in set(culpados):
            # o `pos` do DRC e o MEIO do segmento
            def meio(m):
                return (round((float(m.group(1)) + float(m.group(3))) / 2, 4),
                        round((float(m.group(2)) + float(m.group(4))) / 2, 4))
            novo, n2 = "", 0
            i0 = 0
            for m in re.finditer(r'\n\t\(segment\s*\(start (-?[\d.]+) (-?[\d.]+)\)'
                                 r'\s*\(end (-?[\d.]+) (-?[\d.]+)\)[\s\S]*?\n\t\)', tt):
                mx, my = meio(m)
                if abs(mx - cx) < 0.02 and abs(my - cy) < 0.02:
                    novo += tt[i0:m.start()]
                    i0 = m.end()
                    n2 += 1
            if n2:
                tt = novo + tt[i0:]
                fora += n2
        if fora:
            open(pcb, "w", encoding="utf-8").write(tt)
            print("  3b. %d segmento(s) retirados: o DRC acusou-os, e voltam "
                  "para a lista" % fora)
    n = repara_terra(nome, pcb)
    if n:
        print("  4. %d pad(s) de terra ligados ao plano por via + coto" % n)


def repara_terra(nome, pcb):
    """Uma via no cobre aberto, e um coto ate ao pad, para cada pad de terra que
    o plano nao alcanca.

    POR QUE E PRECISO.

    O FreeRouting considera um pad ligado se ele cai DENTRO do poligono do
    plano. O KiCad so o considera ligado se o cobre ENCHIDO lhe chegar — e a
    volta de um conector apertado o cobre entre as pistas fica mais fino que o
    minimo da zona e e apagado no enchimento. Os dois tem razao sobre coisas
    diferentes, e a que interessa e a do KiCad, que e a que vai para a fabrica.

    ⚠️ E NAO SE CONSERTA AFROUXANDO REGRAS. Tentei: aliviar o termico tirou os
    `starved_thermal` e nao ligou um unico pad; baixar o `min_thickness` de
    0,25 para 0,10 nao mudou UM numero. O cobre nao esta fino, esta ausente —
    as pistas cercam o pad e o plano nao tem por onde entrar.

    O conserto e o que uma pessoa faz: sai-se do cerco com um coto curto e
    desce-se ao plano com uma via, ja em cobre aberto.

    ⚠️ E A VIA VAI PARA O LADO, nunca em cima do pad: via no pad suga solda por
    capilaridade e deixa a junta pobre."""
    import json
    import math
    sys.path.insert(0, AQUI)
    from roteia import (pads_da_placa, pistas_da_placa, vias_da_placa,
                        dist_ponto_seg, KICAD)
    import uuid as _uuid

    def partidos(caminho):
        """Quantos planos estao em pedacos. ⚠️ `Zone [X] <-> Zone [X]` na MESMA
        rede, que e o plano cortado — e nao aparece como erro de geometria."""
        subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--format", "json",
                        "-o", "/tmp/_rp3.json", caminho], capture_output=True)
        dd = json.load(open("/tmp/_rp3.json"))
        n = 0
        for x in dd.get("unconnected_items", []):
            ds = [i.get("description", "") for i in x.get("items", [])]
            if len(ds) == 2 and all("Zone [" in s for s in ds):
                a1 = re.search(r"\[([^\]]+)\]", ds[0])
                b1 = re.search(r"\[([^\]]+)\]", ds[1])
                if a1 and b1 and a1.group(1) == b1.group(1):
                    n += 1
        return n

    part0 = partidos(pcb)
    subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--format", "json",
                    "-o", "/tmp/_rep.json", pcb], capture_output=True)
    d = json.load(open("/tmp/_rep.json"))
    t = open(pcb, encoding="utf-8").read()
    redes_z = redes_com_plano(t)
    alvos = []
    for x in d.get("unconnected_items", []):
        for i in x.get("items", []):
            s, po = i.get("description", ""), (i.get("pos") or {})
            m = re.search(r"\[([^\]]+)\]", s)
            if m and m.group(1) in redes_z and "pad" in s.lower() and "x" in po:
                alvos.append((m.group(1), po["x"], po["y"]))
    alvos = sorted(set(alvos))
    orfaos_xy = {(round(x, 3), round(y, 3)) for _r, x, y in alvos}
    if not alvos:
        return 0

    pads = pads_da_placa(t)
    pistas = pistas_da_placa(t)
    vias = vias_da_placa(t)
    FOLGA, VIA_R = 0.3, 0.3

    # ⚠️ O CONTORNO TAMBEM E UM OBSTACULO. A primeira versao procurava sitio
    # para a via so contra pads, pistas e outras vias, e enfiou duas para FORA
    # da placa: dois `copper_edge_clearance` num conserto que devia so
    # consertar. Uma reparacao que estraga tem de saber onde a placa acaba.
    NUM = r"-?\d+(?:\.\d+)?"
    bordo = [tuple(map(float, m.groups())) for m in
             re.finditer(r'\(gr_line\s*\(start (' + NUM + r') (' + NUM
                         + r')\)\s*\(end (' + NUM + r') (' + NUM
                         + r')\)[\s\S]{0,120}?Edge\.Cuts', t)]
    bx = [v for b in bordo for v in (b[0], b[2])]
    by = [v for b in bordo for v in (b[1], b[3])]
    lim = (min(bx) + 0.8, min(by) + 0.8, max(bx) - 0.8, max(by) - 0.8) if bordo else None

    def cruza(x1, y1, x2, y2, rede_c):
        """⚠️ Distancia nao chega: duas pistas podem cruzar-se a MEIO sem que
        nenhuma ponta esteja perto da outra. O coto de reparacao cruzou uma
        pista do painel e deu `tracks_crossing`."""
        def o(ax, ay, bx, by, cx, cy):
            v = (by - ay) * (cx - bx) - (bx - ax) * (cy - by)
            return 0 if abs(v) < 1e-9 else (1 if v > 0 else 2)
        for a1, b1, a2, b2, n, _L in pistas:
            if n == rede_c:
                continue
            if (o(x1, y1, x2, y2, a1, b1) != o(x1, y1, x2, y2, a2, b2)
                    and o(a1, b1, a2, b2, x1, y1) != o(a1, b1, a2, b2, x2, y2)):
                return True
        return False

    def livre(x, y, r):
        if lim and not (lim[0] <= x <= lim[2] and lim[1] <= y <= lim[3]):
            return False
        for px, py, pr, pn, _c in pads:
            if math.hypot(x - px, y - py) < pr + r + FOLGA:
                return False
        for a1, b1, a2, b2, n, _L in pistas:
            if dist_ponto_seg(x, y, a1, b1, a2, b2) < r + FOLGA + 0.15:
                return False
        for vx, vy, _n in vias:
            if math.hypot(x - vx, y - vy) < VIA_R + r + FOLGA:
                return False
        return True

    novos, feitos, postos = "", 0, []
    for rede, px, py in alvos:
        posto = None
        for raio in [v / 10.0 for v in range(12, 60, 2)]:
            for k in range(16):
                ang = k * math.pi / 8
                x, y = px + raio * math.cos(ang), py + raio * math.sin(ang)
                if (livre(x, y, VIA_R)
                        and all(livre(px + (x - px) * f, py + (y - py) * f, 0.15)
                                for f in (0.45, 0.7, 0.9))
                        and not cruza(px, py, x, y, rede)):
                    posto = (round(x, 3), round(y, 3))
                    break
            if posto:
                break
        if not posto:
            # ⚠️ SEGUNDA ESTRATEGIA: LIGAR AO PAD VIZINHO DA MESMA REDE.
            #
            # A via precisa de 0,6 mm de cobre livre, e a volta de um conector
            # apertado esse espaco nao existe — foi o que devolveu "sem sitio"
            # para metade dos pinos de terra do J14 e do J1, e com eles ficava
            # solta a PONTE DE TERRA e a alimentacao do ESP32.
            #
            # Mas um conector tem varios pinos de terra, e nem todos estao
            # cercados. Um coto ate ao vizinho que ja chega ao plano resolve
            # sem cobre aberto: a pista corre ENTRE os pads, que e onde o passo
            # de 2,54 deixa folga, e nao precisa de sitio para uma via.
            #
            # ⚠️ E o vizinho tem de estar LIGADO. Amarrar dois orfaos um ao
            # outro da uma ilha de dois em vez de um, e o DRC conta na mesma:
            # dois orfaos nao fazem um ligado.
            viz = None
            for qx, qy, qr, qn, qc in pads:
                if qn != rede or (abs(qx - px) < 0.01 and abs(qy - py) < 0.01):
                    continue
                if (round(qx, 3), round(qy, 3)) in orfaos_xy:
                    continue
                d = math.hypot(qx - px, qy - py)
                if d >= 8.0 or (viz and d >= viz[2]):
                    continue
                if cruza(px, py, qx, qy, rede):
                    continue
                if all(livre(px + (qx - px) * f, py + (qy - py) * f, 0.15)
                       for f in (0.3, 0.5, 0.7)):
                    viz = (qx, qy, d)
            if viz:
                novos += ('\t(segment\n\t\t(start %g %g)\n\t\t(end %g %g)\n'
                          '\t\t(width 0.4)\n\t\t(layer "F.Cu")\n\t\t(net "%s")\n'
                          '\t\t(uuid "%s")\n\t)\n'
                          % (round(px, 3), round(py, 3), round(viz[0], 3),
                             round(viz[1], 3), rede, _uuid.uuid4()))
                pistas.append((px, py, viz[0], viz[1], rede, 0))
                feitos += 1
                continue
            print("     ⚠️ %s em (%.1f, %.1f): sem sitio para a via nem vizinho "
                  "ligado. Fica para a pessoa." % (rede, px, py))
            continue
        vx, vy = posto
        novos += ('\t(via\n\t\t(at %g %g)\n\t\t(size 0.6)\n\t\t(drill 0.3)\n'
                  '\t\t(layers "F.Cu" "B.Cu")\n\t\t(net "%s")\n\t\t(uuid "%s")\n\t)\n'
                  % (vx, vy, rede, _uuid.uuid4()))
        novos += ('\t(segment\n\t\t(start %g %g)\n\t\t(end %g %g)\n\t\t(width 0.4)\n'
                  '\t\t(layer "F.Cu")\n\t\t(net "%s")\n\t\t(uuid "%s")\n\t)\n'
                  % (round(px, 3), round(py, 3), vx, vy, rede, _uuid.uuid4()))
        vias.append((vx, vy, rede))
        pistas.append((px, py, vx, vy, rede, 0))
        postos.append((vx, vy, rede))
        feitos += 1
    if not novos:
        return 0
    i = t.rstrip().rfind(")")
    open(pcb, "w", encoding="utf-8").write(t[:i] + novos + t[i:])

    # ⚠️ E CONFERE O QUE ACABOU DE FAZER. Uma via de reparacao pode cair onde o
    # cobre do plano tambem nao chega, e ai nao repara nada: fica solta, e o
    # DRC chama-lhe `via_dangling`. Entregar cobre que nao se consegue provar e
    # pior que nao entregar nada — e a regra que as outras ferramentas desta
    # casa ja seguiam e esta nao seguia.
    subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--severity-error",
                    "--severity-warning", "--format", "json",
                    "-o", "/tmp/_rep2.json", pcb], capture_output=True)
    d2 = json.load(open("/tmp/_rep2.json"))
    soltas = set()
    for x in d2.get("violations", []):
        if x["type"] != "via_dangling":
            continue
        for i2 in x.get("items", []):
            po = i2.get("pos") or {}
            if "x" in po:
                soltas.add((round(po["x"], 3), round(po["y"], 3)))
    # ⚠️⚠️ E CONFERE SE PARTIU UM PLANO. O coto de reparacao e cobre como
    # qualquer outro: passa entre o pad e a zona, e pode cortar exactamente o
    # plano que ele veio ligar. Conferi so `via_dangling` e entreguei uma placa
    # com a terra em TRES pedacos — o portao da fabrica apanhou-a.
    #
    # E a mesma licao da 15dc, num tanque novo: uma ferramenta que escreve
    # cobre tem de medir as DUAS especies de estrago, e nao a que me ocorreu.
    # Uma licao aprendida num sitio nao se aplica sozinha ao sitio do lado.
    if partidos(pcb) > part0:
        # ⚠️ RETIRA-SE A CULPADA, E NAO A REPARACAO TODA. A primeira versao
        # tirava as dez de uma vez e deitava fora oito ligacoes boas por causa
        # de duas mas — que e o mesmo erro que o roteador ja tinha aprendido a
        # nao fazer (§12c). Devolvem-se uma a uma e fica o que couber.
        com_tudo = open(pcb, encoding="utf-8").read()

        def escreve(sem):
            tt = com_tudo
            for vx, vy, _r in sem:
                tt = re.sub(r'\n\t\(via\n\t\t\(at %g %g\)[\s\S]*?\n\t\)'
                            % (vx, vy), "", tt)
                tt = re.sub(r'\n\t\(segment\n\t\t\(start (-?[\d.]+) (-?[\d.]+)\)\n'
                            r'\t\t\(end %g %g\)[\s\S]*?\n\t\)' % (vx, vy), "", tt)
            open(pcb, "w", encoding="utf-8").write(tt)
            return partidos(pcb)

        manter = []
        for c in postos:
            if escreve([x for x in postos if x not in manter and x is not c]) <= part0:
                manter.append(c)
        fora = [c for c in postos if c not in manter]
        escreve(fora)
        if partidos(pcb) > part0:
            escreve(postos)
            print("     ⚠️ a reparacao partia o plano e nem retirando tudo "
                  "zerava: retirada inteira.", file=sys.stderr)
            return 0
        print("     ⚠️ %d reparacao(oes) partiam o plano e foram retiradas; %d "
              "ficam." % (len(fora), len(manter)), file=sys.stderr)
        feitos = len(manter)
        # ⚠️ E NAO SE SAI AQUI. A limpeza das vias soltas vem a seguir, e sair
        # antes dela deixava passar um `via_dangling` sempre que o plano se
        # partia — dois consertos em fila em que o primeiro engolia o segundo.
        t = open(pcb, encoding="utf-8").read()

    if soltas:
        tt = open(pcb, encoding="utf-8").read()
        fora = 0
        for vx, vy in soltas:
            alvo = r'\n\t\(via\n\t\t\(at %g %g\)[\s\S]*?\n\t\)' % (vx, vy)
            novo, k = re.subn(alvo, "", tt)
            if k:
                tt, fora = novo, fora + k
            cot = (r'\n\t\(segment\n\t\t\(start (-?[\d.]+) (-?[\d.]+)\)\n'
                   r'\t\t\(end %g %g\)[\s\S]*?\n\t\)' % (vx, vy))
            tt = re.sub(cot, "", tt)
        open(pcb, "w", encoding="utf-8").write(tt)
        print("     ⚠️ %d via(s) de reparacao ficaram soltas e foram retiradas: "
              "ali o plano tambem nao chega." % fora)
        feitos -= fora
    return feitos


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    for f, o in ((PY_KICAD, "o Python do KiCad"), (JAVA, "o Java"), (JAR, "o freerouting.jar")):
        if not os.path.exists(f):
            raise SystemExit("*** falta %s em %s" % (o, f))
    print("roteia_fr v%s" % VERSAO)
    p = (int(sys.argv[sys.argv.index("--passagens") + 1])
         if "--passagens" in sys.argv else PASSAGENS)
    for q in (["cerebro", "painel"] if sys.argv[1] == "todas" else [sys.argv[1]]):
        uma(q, p)
    print("\n⚠️ O portao ainda nao correu. `para_fabrica.py` e quem diz se ficou bom.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
