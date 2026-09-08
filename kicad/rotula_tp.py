#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""rotula_tp.py — poe o NOME DA REDE ao lado de cada ponto de teste.

    rotula_tp.py <ficheiro.kicad_pcb> [--escreve]
    rotula_tp.py <ficheiro.kicad_pcb> --legenda [--escreve]

⚠️⚠️ PORQUE E QUE ISTO E PRECISO

A placa impressa dizia `TP1`...`TP11` e mais nada. Os nomes das redes viviam no
campo `Value`, que esta na **F.Fab** — camada de documentacao, que a fabrica NAO
imprime. Quem chega a bancada com o osciloscopio numa mao ve `TP7` e nao sabe se
e `+3V3A` ou `AGND`.

⚠️ E O NOME SAI DO `(net ...)` DO PAD, nao de uma tabela.

Podia ler o campo `Value`, que ja tem o nome escrito. Nao le: o `Value` e uma
AFIRMACAO sobre a rede e o `(net ...)` do pad e a rede. Se um dia o ponto for
movido para outro sitio do cobre, o `Value` continua a dizer o que dizia e a
serigrafia passaria a mentir — impressa, em tinta, na peca fisica. Lendo o pad,
o rotulo ou esta certo ou nao existe.

⚠️ E E IDEMPOTENTE: cada rotulo tem `uuid` derivado da referencia, e a corrida
seguinte apaga o seu antes de escrever. Sem isso, correr duas vezes empilhava
dois textos no mesmo sitio — e dois textos identicos sobrepostos leem-se como
um borrao e nao como um erro.
"""

VERSAO = "0.2.0"

import io
import math
import os
import re
import subprocess
import sys
import tempfile
import uuid as _uuid

NUM = r"-?\d+(?:\.\d+)?"
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
NS = _uuid.uuid5(_uuid.NAMESPACE_DNS, "rotula-tp.cotucatronics")
TAM = 0.8               # mm; menor que a referencia, que manda mais
FOLGA = 0.25


def blocos(t):
    """⚠️ Conta parentesis SALTANDO AS CADEIAS (§2d): um `descr` de footprint
    tem `length*width=7.2*11mm^2` e amigos, e um contador cru dessincroniza."""
    i = 0
    while True:
        j = t.find('\n\t(footprint "', i)
        if j < 0:
            return
        k, d, ins = j + 1, 0, False
        while k < len(t):
            c = t[k]
            if c == '"':
                ins = not ins
            elif not ins:
                if c == "(":
                    d += 1
                elif c == ")":
                    d -= 1
                    if d == 0:
                        break
            k += 1
        yield t[j:k + 1]
        i = k + 1


# ⚠️ MARGEM SOBRE O MODELO, medida contra o portao e nao deduzida.
#
# A minha caixa e a do `svg_colisoes` sao dois modelos do mesmo texto, e o meu
# aprovava arranjos que ele reprova (0 -> 4 texto×texto). Nao vale a pena
# adivinhar qual das contas esta certa: sobe-se a margem ate o portao concordar,
# e regista-se o valor que concordou.
MARGEM = 1.35


def caixa(texto, tam, rodado):
    """(meia-largura, meia-altura) em coordenadas da PLACA."""
    w = (max(len(texto), 1) * tam * 0.35 + 0.15) * MARGEM
    h = tam * 0.7 * MARGEM
    return (h, w) if rodado else (w, h)


def geometria(t):
    """pads, textos de serigrafia e contorno — os obstaculos que contam."""
    pads, textos, silk = [], [], []
    for bl in blocos(t):
        a = re.search(r'\n\t\t\(at (' + NUM + r') (' + NUM + r')(?: (' + NUM + r'))?\)', bl)
        if not a:
            continue
        fx, fy = float(a.group(1)), float(a.group(2))
        ang = math.radians(float(a.group(3) or 0))

        def ab(px, py):
            return (fx + px * math.cos(ang) + py * math.sin(ang),
                    fy - px * math.sin(ang) + py * math.cos(ang))

        for pm in re.finditer(r'\(pad "[^"]*" \w+[\s\S]{0,200}?\(at (' + NUM + r') ('
                              + NUM + r')(?: ' + NUM + r')?\)[\s\S]{0,200}?\(size ('
                              + NUM + r') (' + NUM + r')\)', bl):
            px, py, sw, sh = map(float, pm.groups())
            pads.append(ab(px, py) + (max(sw, sh) / 2,))
        # ⚠️ AS REFERENCIAS SAO OBSTACULO, e tentei o contrario.
        #
        # A ideia era: os rotulos escolhem primeiro e o `arruma_silk`, que corre
        # a seguir, afasta as referencias. Faz sentido — o que se le na bancada
        # e a REDE, nao o `TP7` — e nao funciona com um portao POR FERRAMENTA:
        # o estado intermedio tem os rotulos em cima das referencias, e o portao
        # do `rotula_tp` recusa-o (0 -> 12) sem saber que o passo seguinte o
        # arrumava.
        #
        # ⚠️ Um portao por passo nao pode aprovar um passo que so fica bom
        # depois do proximo. Ou o portao passa a ser do FLUXO, ou cada passo se
        # basta. Escolhi bastar-se, que e o que nao exige refazer o fluxo todo.
        m = re.search(r'\(property "Reference" "([^"]*)"\s*\n\s*\(at (' + NUM + r') ('
                      + NUM + r')', bl)
        if m:
            textos.append(ab(float(m.group(2)), float(m.group(3)))
                          + caixa(m.group(1), 1.0, False))
        for sm in re.finditer(r'\(fp_(line|rect)\s*\(start (' + NUM + r') (' + NUM
                              + r')\)\s*\(end (' + NUM + r') (' + NUM
                              + r')\)[\s\S]{0,200}?\(layer "[FB]\.SilkS', bl):
            x1, y1, x2, y2 = (float(sm.group(k)) for k in (2, 3, 4, 5))
            cantos = ([(x1, y1), (x2, y1), (x2, y2), (x1, y2)] if sm.group(1) == "rect"
                      else [(x1, y1), (x2, y2)])
            for k in range(len(cantos) - 1 if sm.group(1) == "line" else 4):
                A, B = ab(*cantos[k]), ab(*cantos[(k + 1) % len(cantos)])
                silk.append(A + B)
    # ⚠️ e os textos de placa (`gr_text`) tambem sao obstaculos: o
    # `cerebro rev B` e o `COTUCATRONICS` estao na serigrafia como qualquer
    # designador, e esquece-los seria escrever por cima do titulo da placa.
    for m in re.finditer(r'\(gr_text "([^"]*)"\s*\n\s*\(at (' + NUM + r') (' + NUM
                         + r')[\s\S]{0,200}?\(layer "([^"]+)"', t):
        if "SilkS" in m.group(4):
            textos.append((float(m.group(2)), float(m.group(3)))
                          + caixa(m.group(1), 1.0, False))
    return pads, textos, silk


def contorno(t):
    xs, ys = [], []
    for m in re.finditer(r'\(gr_(?:line|rect)\s*\(start (' + NUM + r') (' + NUM
                         + r')\)\s*\(end (' + NUM + r') (' + NUM
                         + r')\)[\s\S]{0,160}?Edge\.Cuts', t):
        xs += [float(m.group(1)), float(m.group(3))]
        ys += [float(m.group(2)), float(m.group(4))]
    if not xs:
        raise SystemExit("*** sem contorno Edge.Cuts — LEITURA INCOMPLETA")
    return min(xs), min(ys), max(xs), max(ys)


def livre(x, y, cx, pads, textos, silk, lim):
    if not (lim[0] + cx[0] <= x <= lim[2] - cx[0]
            and lim[1] + cx[1] <= y <= lim[3] - cx[1]):
        return False
    for px, py, pr in pads:
        if abs(px - x) < pr + cx[0] + FOLGA and abs(py - y) < pr + cx[1] + FOLGA:
            return False
    for tx, ty, tw, th in textos:
        if abs(tx - x) < tw + cx[0] and abs(ty - y) < th + cx[1]:
            return False
    for a1, b1, a2, b2 in silk:
        dx, dy = a2 - a1, b2 - b1
        L2 = dx * dx + dy * dy
        u = 0.0 if L2 == 0 else max(0.0, min(1.0, ((x - a1) * dx + (y - b1) * dy) / L2))
        if abs(a1 + u * dx - x) < cx[0] and abs(b1 + u * dy - y) < cx[1]:
            return False
    return True


def num(v):
    """⚠️ §2b: `cos(pi/2)` da 6.1e-17, e nenhum leitor com `(-?[\\d.]+)` o apanha."""
    return "%g" % (round(float(v), 4) + 0.0)


def main():
    print("rotula_tp v%s" % VERSAO)
    alvos = [a for a in sys.argv[1:] if not a.startswith("-")]
    escreve = "--escreve" in sys.argv
    legenda = "--legenda" in sys.argv
    if not alvos:
        raise SystemExit("USO: rotula_tp.py <ficheiro.kicad_pcb> [--escreve]")

    for p in alvos:
        t = io.open(p, encoding="utf-8").read()
        # ⚠️ apaga os rotulos da corrida anterior ANTES de medir o espaco, senao
        # cada um bloqueia o seu proprio lugar e a segunda corrida nao acha sitio.
        t = re.sub(r'\n\t\(gr_text "[^"]*"[\s\S]*?\(uuid "%s[0-9a-f-]*"\)[\s\S]*?\n\t\)'
                   % NS.hex[:8], "", t)
        lim = contorno(t)
        pads, textos, silk = geometria(t)
        tps = []
        for bl in blocos(t):
            r = re.search(r'\(property "Reference" "(TP\d+)"', bl)
            if not r:
                continue
            a = re.search(r'\n\t\t\(at (' + NUM + r') (' + NUM + r')', bl)
            n = re.search(r'\(net (?:\d+ )?"([^"]*)"', bl)
            if not a or not n or not n.group(1):
                print("   ⚠️ %s sem rede no pad — nao se rotula o que nao se sabe"
                      % r.group(1), file=sys.stderr)
                continue
            tps.append((r.group(1), float(a.group(1)), float(a.group(2)), n.group(1)))
        if not tps:
            print("   %-26s sem pontos de teste" % os.path.basename(p))
            continue

        # ⚠️⚠️ UMA FILA DE PONTOS DE TESTE QUER UMA FILA DE ROTULOS.
        #
        # A primeira versao escolhia por rotulo a primeira posicao livre, e o
        # resultado alternava acima/abaixo conforme o que estava no caminho —
        # onze rotulos em ziguezague. Cada um estava correcto e o conjunto era
        # ilegivel: quem olha uma fila procura uma LINHA, e um rotulo fora dela
        # parece ser de outra coisa.
        #
        # ⚠️ E o criterio nao e por rotulo, e do CONJUNTO: escolhe-se o desvio
        # que serve mais deles, e so quem nao couber la e que sai da linha. Um
        # optimo local por elemento nao da um optimo do conjunto — e aqui o
        # conjunto e que se le.
        desvios = [(90, -5.6), (90, 5.6), (90, -7.2), (90, 7.2),
                   (0, -2.4), (0, 2.4)]
        # ⚠️ a pontuacao SIMULA a colocacao, nao mede em vacuo. A primeira
        # versao testava cada rotulo isolado contra os obstaculos existentes, e
        # "deitado" ganhava — porque cada um, sozinho, cabe. Postos os onze, os
        # deitados colidem UNS COM OS OUTROS: `V_TRILHO_A2` tem 8 mm de largura
        # e o passo da fila e 4 mm. Um criterio que ignora o efeito de cada
        # escolha nas seguintes escolhe sempre o arranjo que nao existe.
        melhor, quantos = None, -1
        for rot, dy in desvios:
            postos = list(textos)
            n = 0
            for _r, x, y, rede in sorted(tps, key=lambda z: int(z[0][2:])):
                cx = caixa(rede, TAM, rot == 90)
                if livre(x, y + dy, cx, pads, postos, silk, lim):
                    postos.append((x, y + dy) + cx)
                    n += 1
            if n > quantos:
                melhor, quantos = (rot, dy), n
        rot0, dy0 = melhor

        if legenda:
            # ⚠️⚠️ QUANDO A FILA E APERTADA DE MAIS, O SITIO E OUTRO.
            #
            # No cerebro sao onze pontos a 4 mm de passo numa faixa ja ocupada:
            # os rotulos individuais colocam-se, mas espalham-se por quatro
            # alturas em vez de formarem uma linha. **Uma fila meio rotulada e
            # pior que uma anonima** — parece que os que faltam sao de outra
            # coisa.
            #
            # Entao a informacao vai toda para um sitio so, e a referencia FICA
            # na serigrafia: aqui ela serve, porque e o que liga o ponto a
            # linha da legenda. E o inverso do painel, onde `TP7` nao servia
            # para nada por o nome estar ao lado.
            #
            # ⚠️ E o bloco vai PERTO DA FILA, nao no canto mais vazio. Uma
            # legenda a 15 cm do que descreve obriga a procura-la, e quem esta
            # com a ponta de prova numa mao nao procura — desiste.
            linhas = ["PONTOS DE TESTE"] + [
                "%-5s %s" % (ref, rede)
                for ref, _x, _y, rede in sorted(tps, key=lambda z: int(z[0][2:]))]
            lw = max(len(l) for l in linhas) * TAM * 0.35 * MARGEM + 0.3
            lh = len(linhas) * TAM * 1.55 / 2
            cxs, cys = (sum(z[1] for z in tps) / len(tps),
                        sum(z[2] for z in tps) / len(tps))
            cand = []
            y = lim[1] + lh + 1.0
            while y <= lim[3] - lh - 1.0:
                x = lim[0] + lw + 1.0
                while x <= lim[2] - lw - 1.0:
                    if livre(x, y, (lw, lh), pads, textos, silk, lim):
                        cand.append((math.hypot(x - cxs, y - cys), x, y))
                    x += 1.0
                y += 1.0
            if not cand:
                print("  %-26s sem espaco para a legenda" % os.path.basename(p))
                continue
            _d, bx, by = min(cand)
            y0 = by - lh + TAM * 0.775
            blocos_novos = ""
            for k, l in enumerate(linhas):
                u = _uuid.uuid5(NS, "legenda:%d" % k)
                blocos_novos += (
                    '\t(gr_text "%s"\n\t\t(at %s %s 0)\n\t\t(layer "F.SilkS")\n'
                    '\t\t(uuid "%s")\n\t\t(effects\n\t\t\t(font\n'
                    '\t\t\t\t(size %s %s)\n\t\t\t\t(thickness 0.12)\n\t\t\t)\n'
                    '\t\t\t(justify left)\n\t\t)\n\t)\n'
                    % (l, num(bx - lw + 0.2), num(y0 + k * TAM * 1.55),
                       NS.hex[:8] + str(u)[8:], num(TAM), num(TAM)))
            k = t.rstrip().rindex(")")
            t = t.rstrip()[:k] + blocos_novos + ")\n"
            print("  %-26s legenda de %d linhas em (%.0f, %.0f) · %.0f mm da fila"
                  % (os.path.basename(p), len(linhas), bx, by, _d))
            novos, sem = [("legenda", "", bx, by, 0)], []
        else:
            novos, sem = None, None

        if not legenda:
            # ⚠️⚠️ A REFERENCIA DO PONTO DE TESTE SAI DA SERIGRAFIA.
            #
            # Sao 22 textos numa faixa de 40 x 14 mm — onze referencias e onze
            # rotulos, com os pontos a 4 mm de passo. Nao cabem, e a margem nao
            # resolve aritmetica.
            #
            # ⚠️ E a pergunta certa nao e "como e que cabem", e "quais e que servem".
            # Na bancada, `TP7` impresso nao diz nada: nao se compra, nao se solda,
            # e para o cruzar com a documentacao ja e preciso ter a documentacao na
            # mao — altura em que se le la a rede. `+3V3A` diz tudo, sozinho.
            #
            # Entao a referencia MUDA DE CAMADA, para a F.Fab: continua a ver-se no
            # editor e deixa de gastar tinta e espaco na peca fisica. Nao se apaga —
            # apagar perderia o cruzamento com a netlist e com o CPL.
            for ref, x, y, rede in tps:
                t = re.sub(
                    r'(\(property "Reference" "%s"[\s\S]{0,200}?\(layer ")F\.SilkS(")'
                    % ref, r"\1F.Fab\2", t)
            pads, textos, silk = geometria(t)   # ⚠️ reler: a faixa mudou de dono

            novos, sem = [], []
            for ref, x, y, rede in sorted(tps, key=lambda z: int(z[0][2:])):
                posto = None
                for rot, dy in [(rot0, dy0)] + [d for d in desvios if d != (rot0, dy0)]:
                    cx = caixa(rede, TAM, rot == 90)
                    if livre(x, y + dy, cx, pads, textos, silk, lim):
                        posto = (x, y + dy, rot, cx)
                        break
                if posto is None:
                    sem.append(ref)
                    continue
                px, py, rot, cx = posto
                textos.append((px, py) + cx)     # passa a ser obstaculo dos seguintes
                novos.append((ref, rede, px, py, rot))

            # ⚠️⚠️ O `gr_text` ENTRA ANTES DO PARENTESIS FINAL.
            #
            # A primeira versao fazia `t += ...`, o que poe o texto DEPOIS do `)`
            # que fecha o `(kicad_pcb ...)`. O ficheiro deixaria de abrir. So nao
            # aconteceu porque a corrida foi em ensaio — e "so nao aconteceu porque
            # nao gravei" nao e uma defesa, e sorte.
            blocos_novos = ""
            for ref, rede, px, py, rot in novos:
                u = _uuid.uuid5(NS, "rotulo:%s" % ref)
                blocos_novos += (
                    '\t(gr_text "%s"\n\t\t(at %s %s %d)\n\t\t(layer "F.SilkS")\n'
                    '\t\t(uuid "%s")\n\t\t(effects\n\t\t\t(font\n'
                    '\t\t\t\t(size %s %s)\n\t\t\t\t(thickness 0.12)\n\t\t\t)\n'
                    '\t\t)\n\t)\n' % (rede, num(px), num(py), rot,
                                       NS.hex[:8] + str(u)[8:], num(TAM), num(TAM)))
            if blocos_novos:
                k = t.rstrip().rindex(")")
                t = t.rstrip()[:k] + blocos_novos + ")\n"
        if not legenda:
            print("  %-26s %d rotulo(s)%s" % (os.path.basename(p), len(novos),
                  ("  ⚠️ sem sitio: " + " ".join(sem)) if sem else ""))
            for ref, rede, px, py, rot in novos:
                print("     %-5s %-14s em (%.1f, %.1f) %s" % (ref, rede, px, py,
                      "de pe" if rot else "deitado"))
        if escreve:
            # ⚠️⚠️ ABRE, E NAO PIORA O QUE IMPORTA — antes de tocar no original.
            #
            # Esta ferramenta ACRESCENTA texto, logo o TOTAL de colisoes sobe
            # por construcao: um rotulo novo encosta ao contorno do proprio
            # ponto de teste. O que nao pode subir e `texto×texto`, que e o
            # unico defeito sempre: dois textos por cima um do outro nao se
            # leem, e um rotulo ilegivel e pior que nenhum — porque ocupa o
            # lugar de um que se leria.
            d = tempfile.mkdtemp()
            c = os.path.join(d, os.path.basename(p))
            io.open(c, "w", encoding="utf-8").write(t)
            r = subprocess.run([KICAD, "pcb", "drc", "-o", os.path.join(d, "x.rpt"), c],
                               capture_output=True, text=True)
            if "Failed to load" in (r.stdout + r.stderr):
                print("     *** o resultado NAO ABRE — nada gravado.\n     %s"
                      % (r.stdout + r.stderr).strip().splitlines()[-1][:150],
                      file=sys.stderr)
                return 1

            def mede(f):
                s = os.path.join(d, os.path.basename(f) + ".svg")
                subprocess.run([KICAD, "pcb", "export", "svg", "-o", s, "--layers",
                                "F.Silkscreen", "--page-size-mode", "2",
                                "--exclude-drawing-sheet", f],
                               capture_output=True, text=True)
                v = subprocess.run(
                    [sys.executable, os.path.join(os.path.dirname(
                        os.path.dirname(os.path.abspath(__file__))),
                        "tools", "svg_colisoes.py"), s],
                    capture_output=True, text=True)
                n = re.search(r"(\d+) colisoes", v.stdout)
                return (v.stdout.count("texto×texto"), int(n.group(1)) if n else 0)
            a_, b_ = mede(p), mede(c)
            if b_[0] > a_[0]:
                print("     *** texto×texto %d -> %d. Nada gravado."
                      % (a_[0], b_[0]), file=sys.stderr)
                return 1
            io.open(p, "w", encoding="utf-8").write(t)
            print("     gravado · abre ✓ · texto×texto %d->%d · total %d->%d"
                  % (a_[0], b_[0], a_[1], b_[1]), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
