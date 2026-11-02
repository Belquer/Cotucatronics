#!/usr/bin/env python3
"""gera_conector v0.10.0 — a folha do CONECTOR DO PAINEL (IDC 2x13).

⚠️ A PINAGEM E ESTE FICHEIRO. O `VIAS` aqui em baixo E a fonte, e nao uma
copia do que esta escrito noutro sitio.

Ate 2026-09-08 este cabecalho dizia "a pinagem sai do ROTEIRO-KICAD.md §5" e o §5
listava as 26 vias a mao. Duas fontes do mesmo facto, e a que nao corre foi a
que envelheceu: o pino 19 deixou de ser terra a 2026-09-05 e a tabela de la
continuou a dize-lo, o que levou DUAS vezes a conclusao de que havia uma via
livre que nao havia. O §5 passou a apontar para ca — e apontar para ca so vale
enquanto este cabecalho nao apontar de volta, senao ficam os dois a apontar um
para o outro e nenhum e a fonte.

O contrato e entre as duas placas:
o mesmo nome de rede dos dois lados, via a via. Este ficheiro e uma das duas
metades; a outra nasce no projecto do painel.

⚠️ NAO INVENTAR NOMES. Cada rotulo aqui tem de existir, com a mesma grafia,
na folha do soquete — senao as duas metades nao se encontram e o ERC nao diz
nada, porque um rotulo global sozinho e legal.

⚠️ A GUARDA DE TERRA NAO E ESTETICA. O relogio do OLED vai entre terras dos
dois lados (vias 1 e 3), os pots ensanduichados (23..26) e os LED em par
proprio (11..14): o painel aceso mediu 6 dB de chao na entrada, e o caminho
era este cabo.
"""
import io, sys, uuid

VERSAO = "0.10.0"
G = 1.27
# ⚠️⚠️ UUID DETERMINISTICO. NAO VOLTAR A `uuid4()`. Ver §8b da skill /kicad:
# um gerador que mina um uuid por elemento a cada corrida faz de cada
# regeneracao um diff de centenas de linhas com ZERO linhas nao-uuid — e uma
# mudanca real de duas linhas desaparece la dentro. Diff ilegivel nao e
# revisto, e revisao que nao acontece e o mesmo que revisao que aprova tudo.
#
# ⚠️ Este ficheiro foi o TERCEIRO. Converti o `gera_soquete` e o `gera_painel`,
# relatei "os dois geradores", e este ficou — porque contei os que tinha
# olhado, nao os que existem. Encontrou-o a passada nova da skill `audit`
# (classe 14), a correr sobre todos os `gera_*.py` em vez de sobre a memoria.
NS = uuid.uuid5(uuid.NAMESPACE_DNS, "gera-conector.cotucatronics")
_CHAVES = set()


def u(chave):
    """uuid estavel para `chave`. Duas chamadas com a mesma chave GRITAM.

    Com `uuid4()` a colisao era impossivel; com chaves passa a ser possivel, e
    o KiCad engole-a sem dizer nada. Sem esta guarda trocava-se um ruido
    visivel por um defeito silencioso."""
    if chave in _CHAVES:
        raise SystemExit("*** dois elementos pedem o mesmo uuid: %r" % (chave,))
    _CHAVES.add(chave)
    return str(uuid.uuid5(NS, chave))


# ⚠️ FIXO NO VALOR QUE O FICHEIRO JA TINHA — confirmado que nao e referenciado
# em lado nenhum — para que passar a determinista nao o mexa uma unica vez.
RAIZ_SCH = "98a43dba-f9ec-48a9-9a61-382f286748cc"

def g(v): return round(round(v / G) * G, 4)
L = []
USADOS = set()   # preenchido por simbolo(); NUNCA escrito a mao

# via -> (rotulo, nota).  Do ROTEIRO-KICAD §5, verbatim.
VIAS = {
     1: ("GND", ""),                  2: ("OLED_CLK", "D8 · guardado por terra"),
     3: ("GND", ""),                  4: ("OLED_DIN", "D10"),
     5: ("OLED_CS", "D7"),            6: ("OLED_DC", "D6"),
     7: ("OLED_RST", "D5"),           8: ("GND", ""),
     9: ("+3V3A", "pino 21 · ADC ref"), 10: ("GND", ""),
    11: ("+5V", "LED"),              12: ("+5V", "LED"),
    13: ("GND", "retorno LED"),      14: ("GND", "retorno LED"),
    15: ("LED_FRZ_A", "D27"),        16: ("LED_FRZ_B", "D23"),
    17: ("BOT_FRZ_A", "D24"),        18: ("BOT_FRZ_B", "D22"),
    # ⚠️ A VIA 19 DEIXOU DE SER TERRA a 2026-09-05, para o botao branco dos
    # buzios. Das nove terras, era a unica que nao guardava nenhuma das tres
    # coisas medidas — o relogio do OLED (1 e 3), os pots (23 e 26) e o par
    # dos LED (13 e 14). E o que la entra e um botao: digital lento, corrente
    # nenhuma, o vizinho menos exigente que existe.
    19: ("BOT_BUZIOS", "D26 · botao branco"),
                                     20: ("ENC_A_A", "D0 · o unico encoder"),
    21: ("ENC_A_B", "D1"),           22: ("ENC_A_PUSH", "D2"),
    23: ("GND", ""),                 24: ("POT_L", "D15"),
    25: ("POT_R", "D16"),            26: ("GND", ""),
}

def simbolo(lib, ref, val, x, y):
    USADOS.add(lib)
    x, y = g(x), g(y)
    ps = "".join(
        f'\t\t(property "{k}" "{v}"\n\t\t\t(at {g(x+dx)} {g(y+dy)} 0)\n'
        f'\t\t\t{"(hide yes) " if h else ""}(show_name no) (do_not_autoplace no)\n'
        f'\t\t\t(effects (font (size {s} {s})))\n\t\t)\n'
        for k, v, dx, dy, s, h in
        (("Reference", ref, 0, -21.59, 1.27, False), ("Value", val, 0, 21.59, 1.0, False),
         ("Footprint", _fp(ref), 0, 0, 1.27, True), ("Datasheet", "", 0, 0, 1.27, True),
         ("Description", "", 0, 0, 1.27, True)))
    L.append(f'\t(symbol\n\t\t(lib_id "cotuca:{lib}")\n\t\t(at {x} {y} 0)\n\t\t(unit 1)\n'
             f'\t\t(body_style 1)\n\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n\t\t(on_board yes)\n'
             f'\t\t(in_pos_files yes)\n\t\t(dnp no)\n\t\t(uuid "{u("sym:" + ref)}")\n{ps}{_instancia(ref)}\t)\n')


# ------------------------------------------------------------- FOOTPRINTS
#
# ⚠️ SO O QUE ESTA DETERMINADO. As pecas mecanicas (DPDT tipo 2, jacks TRS,
# EC11, pots, recept. do OLED, eletroliticos radiais, terminais de parafuso)
# dependem do EXEMPLAR FISICO e ficam VAZIAS ate o dB medir — o brief diz
# "medir antes de soldar". Footprint mecanico errado e placa inutil, e um
# footprint plausivel escrito hoje parece verdade amanha.
#
# A JLC monta so o lado SMD (decidido com o dB a 2026-09-05); o THT solda o dB.
FOOTPRINT = {
    "C16": "Capacitor_SMD:C_0805_2012Metric",
    "J14": "Connector_IDC:IDC-Header_2x13_P2.54mm_Vertical",
}


def _fp(ref):
    return FOOTPRINT.get(ref, "")

def _instancia(ref, unidade=1):
    """O bloco `(instances)` de um simbolo. NAO E OPCIONAL.

    Sem ele o KiCad repara o ficheiro ao abrir e mostra uma caixa a pedir que
    se grave: *"An error was found when loading the schematic that has been
    automatically fixed."* Nenhuma ferramenta de linha de comando denuncia
    isto — nem o ERC, nem a netlist, nem o `sch upgrade`, que devolve "not
    updated" porque o formato ja e o corrente. A 2026-09-05 gastaram-se tres
    hipoteses erradas a caca disto ate se pedir ao KiCad para gravar e se
    comparar o ficheiro dele com o meu; a resposta estava no diff.

    ⚠️ O PRIMEIRO SEGMENTO DO CAMINHO SAI DO `.kicad_pro`, NAO DO `(uuid)` DA
    RAIZ. Neste projecto os dois divergem — `.kicad_pro` diz
    `2ad2d1f2...`, o `cotuca_cerebro.kicad_sch` diz `2a678ab8...` — e o KiCad
    usa o do `.kicad_pro`. Escrever o outro produz dados de instancia que
    nascem obsoletos, que e pior do que nao os escrever.

    O caminho de uma folha-filha e `/<raiz>/<folha>`; o de um simbolo na
    propria raiz e so `/<raiz>`.
    """
    import json as _json
    import re as _re
    pro = _json.load(io.open("cotuca_cerebro.kicad_pro", encoding="utf-8"))
    raiz = pro["sheets"][0][0]
    _u = _re.search(r'^\t\(uuid "([0-9a-f-]{36})"\)',
                    io.open("cotuca_cerebro.kicad_sch", encoding="utf-8").read(),
                    _re.M)
    if not _u or _u.group(1) != raiz:
        raise SystemExit("*** o (uuid) da raiz (%s) nao bate com o do .kicad_pro "
                         "(%s). Escrever (instances) assim produz dados que "
                         "nascem obsoletos e o KiCad repara o ficheiro ao abrir."
                         % (_u.group(1) if _u else "ausente", raiz))
    r = io.open("cotuca_cerebro.kicad_sch", encoding="utf-8").read()
    minha = None
    for m in _re.finditer(r'\(sheet\s*\n(?:.|\n)*?\(uuid "([0-9a-f-]{36})"\)'
                          r'((?:.|\n)*?)\n\t\)', r):
        if '"cotuca_conector.kicad_sch"' in m.group(2):
            minha = m.group(1); break
    if minha is None:
        raise SystemExit("*** nao achei a folha cotuca_conector.kicad_sch na raiz — sem isso o "
                         "(instances) sai errado e o KiCad repara o ficheiro "
                         "ao abrir, em silencio para a linha de comando")
    return ('\t\t(instances\n\t\t\t(project "cotuca_cerebro"\n'
            '\t\t\t\t(path "/%s/%s"\n\t\t\t\t\t(reference "%s")\n'
            '\t\t\t\t\t(unit %d)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n'
            % (raiz, minha, ref, unidade))

def fio(x1, y1, x2, y2):
    L.append(f'\t(wire\n\t\t(pts\n\t\t\t(xy {g(x1)} {g(y1)}) (xy {g(x2)} {g(y2)})\n\t\t)\n'
             f'\t\t(stroke (width 0) (type default))\n\t\t(uuid "{u(f"fio:{g(x1)},{g(y1)}-{g(x2)},{g(y2)}")}")\n\t)\n')

def rotulo(nome, x, y, ang, just):
    L.append(f'\t(global_label "{nome}"\n\t\t(shape bidirectional)\n\t\t(at {g(x)} {g(y)} {ang})\n'
             f'\t\t(fields_autoplaced yes)\n\t\t(effects (font (size 1.27 1.27)) (justify {just}))\n'
             f'\t\t(uuid "{u(f"rot:{nome}:{g(x)},{g(y)},{ang}")}")\n\t)\n')

def texto(s, x, y, tam=1.27):
    L.append(f'\t(text "{s}"\n\t\t(exclude_from_sim no)\n\t\t(at {g(x)} {g(y)} 0)\n'
             f'\t\t(effects (font (size {tam} {tam})) (justify left))\n\t\t(uuid "{u(f"txt:{g(x)},{g(y)}:{s}")}")\n\t)\n')

def pinos_de(nome):
    """offsets lidos da BIBLIOTECA, nunca de cabeca — ver gera_soquete.py."""
    import re as _re
    Lb = io.open("cotuca.kicad_sym", encoding="utf-8").read()
    i = Lb.find('(symbol "%s"' % nome); d = 0
    for k in range(i, len(Lb)):
        if Lb[k] == "(": d += 1
        elif Lb[k] == ")":
            d -= 1
            if d == 0: b = Lb[i:k+1]; break
    ps = [(m.group(3), float(m.group(1)), float(m.group(2))) for m in
          _re.finditer(r'\(pin\s+\S+\s+\S+\s*\n\s*\(at (-?[\d.]+(?:[eE][-+]?\d+)?) (-?[\d.]+(?:[eE][-+]?\d+)?)'
                       r'(?:\s+-?[\d.]+(?:[eE][-+]?\d+)?)?\)(?:.|\n)*?\(number "([^"]+)"', b)]
    # GUARDA: contar os blocos (pin e exigir que a leitura os apanhe todos.
    # Um leitor que perde linhas em silencio nao reprova nada: os pinos 13 e 14
    # do IDC26, escritos como 1.7763568394002505e-15 por ruido de virgula
    # flutuante, escaparam a `[\d.]+` e a folha saiu com 24 rotulos em vez de
    # 26 — sem um unico aviso, em quatro geradores ao mesmo tempo.
    blocos = len(_re.findall(r'\(pin\s', b))
    if blocos != len(ps):
        raise SystemExit("*** pinos_de(%s): o simbolo tem %d pinos, a leitura "
                         "apanhou %d — LEITURA INCOMPLETA, nada a jusante "
                         "e confiavel" % (nome, blocos, len(ps)))
    return ps

def nota(x, y, *linhas, tam=1.6, passo=3.81):
    """Nota de varias linhas. O SINAL DO Y VIVE SO AQUI — na folha cresce para
    BAIXO, no simbolo para CIMA, e repetir a coordenada em cada sitio de
    chamada foi como 9 notas sairam de tras para a frente a 2026-09-05."""
    for i, s in enumerate(linhas):
        texto(s, x, y + i * passo, tam if i == 0 else 1.27)


JX, JY = 160.0, 110.0
simbolo("IDC26", "J14", "IDC 2x13 -> painel", JX, JY)

# ------------------------ 100 nF do +3V3A, COLADO ao IDC
#
# ⚠️ O +3V3A e a alimentacao dos pots, e sai daqui por 150 mm de fita. A
# leitura da Daisy e ratiometrica e tolera o que for COMUM ao trilho e a
# referencia — mas o que entra so pelo lado do cabo entra na leitura. O par
# vai colado ao conector nas DUAS placas, que e onde o cabo comeca e acaba.
# Achado da P4 da revisao de terra, 2026-09-06: o trilho nao tinha nenhum.
CX3, CY3 = JX - 40.0, JY + 60.0
simbolo("C_V", "C16", "100n", CX3, CY3)
for (_n, px, py), (rot, ang, just) in zip(pinos_de("C_V"),
                                          [("+3V3A", 90, "left"), ("GND", 270, "left")]):
    x, y = CX3 + px, CY3 - py
    dy = -10.16 if py > 0 else 10.16
    fio(x, y, x, y - dy); rotulo(rot, x, y - dy, ang, just)
for num, px, py in pinos_de("IDC26"):
    n = int(num); rot, obs = VIAS[n]
    x, y = JX + px, JY - py          # y do simbolo cresce para CIMA
    esq = px < 0
    dx = -10.16 if esq else 10.16
    fio(x, y, x + dx, y)
    rotulo(rot, x + dx, y, 180 if esq else 0, "right" if esq else "left")
    # A nota do lado ESQUERDO acaba a 25,5 mm do pino, encostada pela direita:
    # com deslocamento fixo (-34,0) uma nota comprida crescia PARA DENTRO do
    # rotulo — o "D26 · botao branco" da via 19 entrava 4,9 mm² no BOT_BUZIOS.
    # Alinhar pela ponta que importa e o conserto; empurrar o -34 so adia.
    if obs:
        if esq:
            texto(obs, x - 25.5 - len(obs) * 1.0 * 0.85, y, 1.0)
        else:
            texto(obs, x + 26.0, y, 1.0)

# JY-34 e nao JY-26: a segunda linha caia em cima da referencia J14, que fica
# a JY-20,32 (h/2 + 3,81 do simbolo). 0,8 mm², apanhado pelo colisoes_sch.
nota(JX - 45, JY - 34,
     "CONECTOR DO PAINEL — IDC 2x13. Pinagem do ROTEIRO-KICAD §5.",
     "E o CONTRATO entre as duas placas: mesmo nome de rede, via a via.")
nota(JX - 45, JY + 24,
     "Guarda de terra: relogio do OLED entre terras (1 e 3), pots",
     "ensanduichados (23..26), LED em par proprio (11..14). O painel",
     "aceso mediu 6 dB de chao na entrada, e o caminho era este cabo.",
     tam=1.27)

def _confere_lib(caminho):
    """Depois de escrever: todo `lib_id` tem entrada no `lib_symbols`?

    Um simbolo referido e nao embutido nao da erro nenhum — o KiCad ve um
    simbolo SEM PINOS e cada fio fica solto. A 2026-09-05 custou 10 fios soltos no
    painel, sem uma palavra do gerador."""
    import re as _r
    t = io.open(caminho, encoding="utf-8").read()
    i = t.find("(lib_symbols"); d = 0
    for k in range(i, len(t)):
        if t[k] == "(": d += 1
        elif t[k] == ")":
            d -= 1
            if d == 0:
                lib, fora = t[i:k+1], t[:i] + t[k+1:]; break
    usa = set(_r.findall(r'\(lib_id "([^"]+)"\)', fora))
    tem = set(_r.findall(r'\(symbol "(cotuca:[^"]+)"', lib))
    if usa - tem:
        raise SystemExit("*** " + caminho + " refere simbolos que a biblioteca "
                         "embutida nao tem: " + str(sorted(usa - tem)) +
                         " — o KiCad ve-os SEM PINOS e os fios ficam soltos")
    return len(usa)

def lib_embutida(usados=None):
    """Sai do que foi REALMENTE colocado — ver gera_painel.py v0.7.1: uma lista
    literal a mao ao lado de uma folha gerada deixou a folha a referir um
    simbolo ausente, KiCad viu um simbolo sem pinos e 10 fios ficaram soltos."""
    usados = USADOS if usados is None else usados
    if not usados:
        raise SystemExit("*** nenhum simbolo registado")
    Lb = io.open("cotuca.kicad_sym", encoding="utf-8").read(); out = []
    for nome in sorted(usados):
        i = Lb.find('(symbol "%s"' % nome); d = 0
        for k in range(i, len(Lb)):
            if Lb[k] == "(": d += 1
            elif Lb[k] == ")":
                d -= 1
                if d == 0: b = Lb[i:k+1]; break
        out.append("\t\t" + b.replace('(symbol "%s"' % nome, '(symbol "cotuca:%s"' % nome, 1)
                   .replace("\n  ", "\n\t\t"))
    return "\t(lib_symbols\n" + "\n".join(out) + "\n\t)\n"

CAB = f'''(kicad_sch
\t(version 20260306)
\t(generator "cotucatronics")
\t(generator_version "10.0")
\t(uuid "{RAIZ_SCH}")
\t(paper "A3")
\t(title_block
\t\t(title "CONECTOR DO PAINEL — IDC 2x13 — carrier rev B")
\t\t(date "2026-09-05")
\t\t(rev "B")
\t\t(comment 2 "Pinagem do ROTEIRO-KICAD §5. Contrato entre as duas placas.")
\t\t(comment 1 "Guarda de terra por desenho, nao por estetica: 6 dB medidos.")
\t)
{lib_embutida()}'''
io.open("cotuca_conector.kicad_sch", "w", encoding="utf-8").write(
    CAB + "".join(L) + '\t(sheet_instances\n\t\t(path "/" (page "5"))\n\t)\n\t(embedded_fonts no)\n)\n')
print("gera_conector v%s -> cotuca_conector.kicad_sch: %d blocos"
      % (VERSAO, len(L)), file=sys.stderr)
print("  lib_symbols coerente: " + str(_confere_lib("cotuca_conector.kicad_sch")) + " simbolos",
      file=sys.stderr)
