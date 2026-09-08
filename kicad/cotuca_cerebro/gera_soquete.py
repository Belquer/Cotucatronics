#!/usr/bin/env python3
"""gera_soquete v0.15.0 — a folha SOQUETE + PONTE + DPDT + SAIDAS, rev B.

Desenhada por SCRIPT e nao a mao (skill `kicad` §5): reproduzivel, diffavel,
e corrigivel na fonte. Toda a geometria cai na grade de 1,27 mm.

O QUE ESTA FOLHA RESOLVE, e cada um tem fonte:

 · A PONTE AGND<->GND. Um `NETTIE2` entre o pino 20 e o pino 40 da Daisy —
   duas redes no ERC, UMA ligacao de cobre. A Seed NAO os junta por dentro
   (doc da libDaisy: "always connect the DGND and AGND pins outside of the
   Daisy"; medido: sem continuidade 20<->40 numa placa em mao).
   ⚠️ Os dois pinos estao na DIAGONAL do modulo, nao lado a lado. Ver
   MAPA-DE-TERRA.md §2 para a consequencia de layout.

 · `Dnn` -> pino do header sai da folha de dados, nunca de memoria. O mapa
   vem do `Seed3_pinout.csv` via `gera_daisy_sym.py`. (Era o CSV do rev4
   ate 05/09; a pinagem dos dois bate nos 40 pinos, conferida.)
   ⚠️ D29/D30 SAO O USB (pinos 36/37). Nao se usam. O sense dos DPDT vai
   para D11/D12.

 · Ligacao por ROTULOS GLOBAIS e nao por fios longos: a folha da energia e a
   do pre-amp ja usam isso, e o netlist da raiz prova que atravessa.
"""
import io, re, sys, uuid

VERSAO = "0.16.0"
G = 1.27
def u(): return str(uuid.uuid4())
def g(v): return round(round(v / G) * G, 4)

L = []
USADOS = set()   # blocos do ficheiro

def simbolo(lib, ref, val, x, y, ang=0, campos=()):
    USADOS.add(lib)
    x, y = g(x), g(y)
    ps = "".join(
        f'\t\t(property "{k}" "{v}"\n\t\t\t(at {g(x+dx)} {g(y+dy)} 0)\n'
        f'\t\t\t{"(hide yes) " if h else ""}(show_name no) (do_not_autoplace no)\n'
        f'\t\t\t(effects (font (size {s} {s})){jj})\n\t\t)\n'
        for k, v, dx, dy, s, h, jj in campos)
    L.append(f'''\t(symbol
\t\t(lib_id "cotuca:{lib}")
\t\t(at {x} {y} {ang})
\t\t(unit 1)
\t\t(body_style 1)
\t\t(exclude_from_sim no)
\t\t(in_bom {_bom(ref)})
\t\t(on_board yes)
\t\t(in_pos_files {_bom(ref)})
\t\t(dnp no)
\t\t(uuid "{u()}")
{ps}{_instancia(ref)}\t)
''')


# ------------------------------------------------------------- FOOTPRINTS
#
# ⚠️ SO O QUE ESTA DETERMINADO. As pecas mecanicas (DPDT tipo 2, jacks TRS,
# EC11, pots, recept. do OLED, eletroliticos radiais, terminais de parafuso)
# dependem do EXEMPLAR FISICO e ficam VAZIAS ate o dB medir — o brief diz
# "medir antes de soldar". Footprint mecanico errado e placa inutil, e um
# footprint plausivel escrito hoje parece verdade amanha.
#
# A JLC monta so o lado SMD (decidido com o dB a 05/09); o THT solda o dB.
FOOTPRINT = {
    "U2":  "Module:Electrosmith_Daisy_Seed",      # o footprint do fabricante
    "NT1": "NetTie:NetTie-2_SMD_Pad2.0mm",        # ⚠️ FORA da BOM: nao e peca
    "C15": "Capacitor_SMD:C_0805_2012Metric",
    "R50": "Resistor_SMD:R_0805_2012Metric",
    "R51": "Resistor_SMD:R_0805_2012Metric",
    # ⚠️ footprint do projecto, nao de biblioteca instalada: este modulo nao
    # tem footprint oficial em lado nenhum. Ver kicad/gera_footprints.py.
    "U3": "cotuca:ESP32C3_SUPERMINI_SOQUETE",
    "J11": "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical",
    "J12": "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical",
    "J13": "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical",
    # ⚠️ 1x08 e nao 1x06: os pinos 7 e 8 sao TERRA. Ver o bloco do DPDT.
    "J15": "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical",
    **{f"TP{i}": "TestPoint:TestPoint_Pad_D1.5mm" for i in range(1, 12)},
    "J16": "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical",
}


def _fp(ref):
    return FOOTPRINT.get(ref, "")


# Referencias que NAO sao pecas: cobre no desenho, nada para comprar nem para
# a maquina pousar. Sem isto o net-tie sai como LINHA DE BOM e como entrada no
# ficheiro de posicionamento da JLC — a fabrica ia procurar um componente que
# nao existe.
NAO_E_PECA = {"NT1", "#FLG3", "#FLG4"}


def _bom(ref):
    """⚠️ UM PONTO DE TESTE NAO E UMA PECA, e a BOM sai do ESQUEMA.

    O footprint do TP ja traz `exclude_from_bom`, mas isso e do lado da PLACA:
    a BOM e exportada do esquema, onde o simbolo dizia `in_bom yes`. Resultado
    — onze linhas de compra para cobre, na lista que vai para a fabrica. So se
    viu ao comparar o CPL com a BOM, porque nenhum dos dois exportadores se
    queixa e nenhum sabe do outro.

    ⚠️ E e CRITERIO, nao lista: um `TP12` acrescentado amanha fica de fora
    sozinho. Uma lista fixa protege os nomes que ja la estao — que sao
    exactamente os que nao precisam de protecao."""
    return "no" if (ref in NAO_E_PECA or re.match(r"^TP\d+$", ref)) else "yes"

def _instancia(ref, unidade=1):
    """O bloco `(instances)` de um simbolo. NAO E OPCIONAL.

    Sem ele o KiCad repara o ficheiro ao abrir e mostra uma caixa a pedir que
    se grave: *"An error was found when loading the schematic that has been
    automatically fixed."* Nenhuma ferramenta de linha de comando denuncia
    isto — nem o ERC, nem a netlist, nem o `sch upgrade`, que devolve "not
    updated" porque o formato ja e o corrente. A 05/09 gastaram-se tres
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
        if '"cotuca_soquete.kicad_sch"' in m.group(2):
            minha = m.group(1); break
    if minha is None:
        raise SystemExit("*** nao achei a folha cotuca_soquete.kicad_sch na raiz — sem isso o "
                         "(instances) sai errado e o KiCad repara o ficheiro "
                         "ao abrir, em silencio para a linha de comando")
    return ('\t\t(instances\n\t\t\t(project "cotuca_cerebro"\n'
            '\t\t\t\t(path "/%s/%s"\n\t\t\t\t\t(reference "%s")\n'
            '\t\t\t\t\t(unit %d)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n'
            % (raiz, minha, ref, unidade))

def nao_liga(x, y):
    """`no_connect` num pino. ⚠️ Nao e "nao usado": e uma PROMESSA de que nada
    se liga ali, e o ERC passa a recusar quem tente."""
    L.append('\t(no_connect\n\t\t(at %s %s)\n\t\t(uuid "%s")\n\t)\n'
             % (g(x), g(y), u()))


def fio(x1, y1, x2, y2):
    L.append(f'\t(wire\n\t\t(pts\n\t\t\t(xy {g(x1)} {g(y1)}) (xy {g(x2)} {g(y2)})\n\t\t)\n'
             f'\t\t(stroke (width 0) (type default))\n\t\t(uuid "{u()}")\n\t)\n')

def rotulo(nome, x, y, ang=0, just="left"):
    L.append(f'''\t(global_label "{nome}"
\t\t(shape bidirectional)
\t\t(at {g(x)} {g(y)} {ang})
\t\t(fields_autoplaced yes)
\t\t(effects (font (size 1.27 1.27)) (justify {just}))
\t\t(uuid "{u()}")
\t)
''')

def texto(s, x, y, tam=1.27):
    L.append(f'\t(text "{s}"\n\t\t(exclude_from_sim no)\n\t\t(at {g(x)} {g(y)} 0)\n'
             f'\t\t(effects (font (size {tam} {tam})) (justify left))\n\t\t(uuid "{u()}")\n\t)\n')

def nota(x, y, *linhas, tam=1.6, passo=3.81):
    """Uma nota de varias linhas. O SINAL DO Y VIVE SO AQUI.

    Na FOLHA o y cresce para BAIXO; no SIMBOLO cresce para CIMA. Escrever as
    continuacoes com `y - 3.8` — que e o que a intuicao do simbolo dita —
    imprime a nota de tras para a frente, e o render mostra o ultimo periodo
    primeiro. Aconteceu em 8 notas do painel e 1 do soquete de uma vez, porque
    a coordenada estava repetida em cada sitio de chamada. Aqui esta uma vez.

    A primeira linha e o titulo (tam maior); as seguintes vao a 1.27."""
    for i, s in enumerate(linhas):
        texto(s, x, y + i * passo, tam if i == 0 else 1.27)


def juncao(x, y):
    L.append(f'\t(junction\n\t\t(at {g(x)} {g(y)})\n\t\t(diameter 0)\n\t\t(color 0 0 0 0)\n\t\t(uuid "{u()}")\n\t)\n')

def pinos_de(nome):
    """[(numero, dx, dy)] lidos da BIBLIOTECA, nunca da memoria.

    ⚠️ EXISTE PORQUE EU CODIFIQUEI OS OFFSETS A MAO. O CONN5_L calhou bater
    (h/2 = 6,35, e eu tinha escrito 6,35); o CONN4_L tem h/2 = 5,08 e eu
    escrevi 3,81, e o JACK3 nem olhei. Resultado: 12 fios pendurados, cada um
    a 1,27 ou 2,54 mm do pino que devia tocar — perto demais para se ver no
    render, longe demais para ligar.

    E a mesma familia do `Dnn -> pino do header`: a geometria mora no
    simbolo, e quem a repete de cabeca erra numa delas.
    """
    L = io.open("cotuca.kicad_sym", encoding="utf-8").read()
    i = L.find('(symbol "%s"' % nome)
    if i < 0: raise SystemExit("simbolo ausente: %s" % nome)
    d = 0
    for k in range(i, len(L)):
        if L[k] == "(": d += 1
        elif L[k] == ")":
            d -= 1
            if d == 0: b = L[i:k+1]; break
    import re as _re
    out = []
    for m in _re.finditer(r'\(pin\s+\S+\s+\S+\s*\n\s*\(at (-?[\d.]+(?:[eE][-+]?\d+)?) (-?[\d.]+(?:[eE][-+]?\d+)?)'
                          r'(?:\s+-?[\d.]+(?:[eE][-+]?\d+)?)?\)(?:.|\n)*?\(number "([^"]+)"', b):
        out.append((m.group(3), float(m.group(1)), float(m.group(2))))
    # GUARDA: contar os blocos (pin e exigir que a leitura os apanhe todos.
    # A doc acima diz que a geometria mora no simbolo — mas um leitor que perde
    # linhas em silencio nao le o simbolo, le uma parte dele. Os pinos 13 e 14
    # do IDC26 saiam como 1.7763568394002505e-15 e escapavam a `[\d.]+`.
    blocos = len(_re.findall(r'\(pin\s', b))
    if blocos != len(out):
        raise SystemExit("*** pinos_de(%s): o simbolo tem %d pinos, a leitura "
                         "apanhou %d — LEITURA INCOMPLETA, nada a jusante "
                         "e confiavel" % (nome, blocos, len(out)))
    return out


def campos(ref, val, dx=0, dy=0):
    return (("Reference", ref, dx, dy - 2.54, 1.27, False, ""),
            ("Value", val, dx, dy + 2.54, 1.0, False, ""),
            ("Footprint", _fp(ref), 0, 0, 1.27, True, ""),
            ("Datasheet", "", 0, 0, 1.27, True, ""),
            ("Description", "", 0, 0, 1.27, True, ""))


# ---------------------------------------------------------------- a Daisy
# O simbolo tem 20 pinos por lado, passo 2,54, primeiro em +25,4 do centro.
DX, DY = 190.5, 130.0          # centro do soquete
W = 25.4 + 5.08                # do centro ate a ponta do pino

def pin_xy(n):
    """Coordenada da PONTA do pino n, no simbolo colocado em (DX, DY)."""
    i = (n - 1) % 20
    y = DY - (25.4 - i * 2.54)
    return (DX - W, y) if n <= 20 else (DX + W, y)

# Dnn -> pino fisico, do CSV do fabricante (ver gera_daisy_sym.py)
PINO = {"D0":1,"D1":2,"D2":3,"D3":4,"D4":5,"D5":6,"D6":7,"D7":8,"D8":9,"D9":10,
        "D10":11,"D11":12,"D12":13,"D13":14,"D14":15,
        "AUDIO_IN_L":16,"AUDIO_IN_R":17,"AUDIO_OUT_L":18,"AUDIO_OUT_R":19,"AGND":20,
        "3v3A":21,"D15":22,"D16":23,"D17":24,"D18":25,"D19":26,"D20":27,"D21":28,
        "D22":29,"D23":30,"D24":31,"D25":32,"D26":33,"D27":34,"D28":35,
        "D29":36,"D30":37,"3v3D":38,"VIN":39,"GND":40}

# O que sai de cada pino. Nome do rotulo global, ou None para deixar solto.
LIGA = {
    "AUDIO_IN_L":"AUDIO_IN_L", "AUDIO_IN_R":"AUDIO_IN_R",
    "AUDIO_OUT_L":"OUT_L_DAISY", "AUDIO_OUT_R":"OUT_R_DAISY",
    # ⚠️ O `3v3D` (pino 38) NAO leva rotulo: desde 05/09 nada o consome. Os
    # pots passaram para o `+3V3A` (pino 21), que e o fundo de escala do ADC,
    # e o OLED foi para os 5 V. Um rotulo global com um pino so nao e uma
    # rede: e um aviso de ERC a fingir de ligacao. O pino continua a ser uma
    # SAIDA disponivel — quem a quiser, liga-a e declara o consumidor.
    "AGND":"AGND", "GND":"GND", "VIN":"+5V", "3v3A":"+3V3A",
    "D13":"MIDI_TX", "D14":"MIDI_RX", "D28":"PONTE_STATUS",
    "D17":"V_TRILHO_A2",
    "D5":"OLED_RST","D6":"OLED_DC","D7":"OLED_CS","D8":"OLED_CLK","D10":"OLED_DIN",
    "D0":"ENC_A_A","D1":"ENC_A_B","D2":"ENC_A_PUSH",   # o unico encoder
    # ⚠️ UM encoder desde 05/09. D3/D4/D9 (enc B) e D18/D19 (enc G) sairam,
    # e o D17 deixou de ser o push do G para ser o A2 do divisor — que
    # dissolveu o conflito sem se tocar em firmware que funcionava.
    "D15":"POT_L","D16":"POT_R",
    "D22":"BOT_FRZ_B","D23":"LED_FRZ_B","D24":"BOT_FRZ_A",    "D26":"BOT_BUZIOS","D27":"LED_FRZ_A",  # o buzios ganha botao branco no painel
    # ⚠️ O SENSE DOS DPDT NAO EXISTE, e a decisao e de 05/09. A DPDT tipo 2
    # usa os DOIS polos para o mesmo canal (comuns amarrados), logo nao sobra
    # contacto para ler o estado. `exc_on` continua a ser INTENCAO de
    # software, e nao leitura de hardware — o que quer dizer que o firmware
    # acredita no que lhe dizem. Ver docs/FUTURO.md.
}
# ⚠️ D29 e D30 NAO entram. Sao USB_HS_D-/D+ (pinos 36/37). Ver PINOS.md.

simbolo("DAISY_SEED", "U2", "Daisy Seed3", DX, DY,
        campos=(("Reference","U2",0,-30.48,1.27,False,""),
                ("Value","Daisy Seed3",0,30.48,1.0,False,""),
                ("Footprint",_fp("U2"),0,0,1.27,True,""),
                ("Datasheet","Seed3_pinout.csv",0,0,1.27,True,""),
                ("Description","",0,0,1.27,True,"")))

TOCO = 10.16
for nome, rot in LIGA.items():
    if nome not in PINO or not rot: continue
    n = PINO[nome]; x, y = pin_xy(n)
    if n <= 20:
        fio(x, y, x - TOCO, y); rotulo(rot, x - TOCO, y, 180, "right")
    else:
        fio(x, y, x + TOCO, y); rotulo(rot, x + TOCO, y, 0, "left")

texto("SOQUETE DA DAISY — o Dnn -> pino sai da folha de dados, nunca de memoria.",
      DX - 60, DY - 44, 1.6)
texto("D29 e D30 NAO se usam: sao USB_HS_D-/D+ (pinos 36/37).", DX - 60, DY - 40)


# ------------------------------------------------- A PONTE: o NetTie AGND<->GND
#
# ⚠️ ESTE E O UNICO SITIO ONDE AS DUAS TERRAS SE ENCONTRAM. No ERC continuam
# duas redes; no cobre e uma ligacao, num ponto escolhido. Se o netlist algum
# dia mostrar UMA rede de terra, alguem substituiu isto por um fio.
NTX, NTY = 90.0, 205.0
simbolo("NETTIE2", "NT1", "NetTie-2", NTX, NTY,
        campos=campos("NT1", "NetTie-2  AGND<->GND", 0, 0))
fio(NTX - 7.62, NTY, NTX - 17.78, NTY); rotulo("AGND", NTX - 17.78, NTY, 180, "right")
fio(NTX + 7.62, NTY, NTX + 17.78, NTY); rotulo("GND",  NTX + 17.78, NTY, 0, "left")
texto("A PONTE, e e a unica. A Seed NAO junta AGND e DGND por dentro:", NTX - 30, NTY - 15.24, 1.6)
texto("a doc da libDaisy manda liga-los fora, e 20<->40 mediu ABERTO.", NTX - 30, NTY - 11.43)
texto("Os pinos 20 e 40 estao na DIAGONAL do modulo — o net-tie fica", NTX - 30, NTY + 11.43)
texto("encostado ao 20, dentro da ilha, com UM condutor ate ao 40.", NTX - 30, NTY + 15.24)

# --------------------------------------------- ponte ESP32-C3, EM SOQUETE 2x8
# ⚠️ Era um header de 5 vias com o modulo ligado por fio. Passou a soquete a
# pedido do dB. Duas consequencias que nao sao obvias:
#   · a terra deixa de ir "junto com os fios" — vai pelo plano, que e melhor. A
#     nota antiga dizia o contrario e foi apagada (premissa revogada).
#   · os pinos que NAO se usam passam a existir como pads, e o ERC quer saber
#     deles: levam `no_connect`.
# ⚠️ IO8 e o LED azul de bordo e IO9 e o BOOT — nenhum serve para o status.
# Fica IO3, que a ficha da como livre de strapping, flash, USB e JTAG.
EX, EY = 300.0, 60.0
simbolo("ESP32C3", "U3", "ESP32-C3 SuperMini", EX, EY,
        campos=campos("U3", "ESP32-C3 SuperMini"))
# ⚠️ `y` DO SIMBOLO CRESCE PARA CIMA; o da folha, para BAIXO. Logo
# `abs_y = simbolo_y - offset`, com MENOS. Somar espelha o conector: o +5V
# do pino 1 ia parar ao pino 5, e o unico sinal era um `pin_not_connected`
# num extremo — os do meio calham bater por simetria.
# ⚠️ O MAPA E POR NOME DE PINO, e nao por ordem. O footprint e o simbolo usam os
# NOMES DA SERIGRAFIA do modulo (`5V`, `IO21`), porque esta placa apareceu com
# tres pinagens contraditorias em duas horas — cicatriz 15ck. Indexar por ordem
# reintroduzia exactamente a traducao que os nomes existem para eliminar.
# ⚠️ `+5V_PONTE` E NAO `+5V`. A ponte e alimentada ATRAS de um Schottky (D2,
# 1N5819) com o seu par local (C3 100u + C4 100n) — para que o USB do modulo,
# quando se liga para reprogramar, NAO realimente o trilho de 5 V da placa
# toda. Foi um defeito ja pago na bancada.
# Ao trocar o J10 (header por fio) pelo U3 (soquete) liguei o modulo ao trilho
# CRU e deixei o diodo para tras — e ainda por cima no dia em que abrimos um
# buraco na caixa para chegar a esse USB. O dB apanhou-o a ler o esquema.
#   Trocar uma peca por outra nao herda o que estava ANTES dela no caminho.
PONTE = {"5V": ("+5V_PONTE", ""),
         "GND": ("GND", ""),
         # ⚠️ sem observacao por pino: a 1,0 de tamanho elas cabiam no desenho
         # mas nao no espaco — batiam no rotulo e no fio, 4 colisoes. O que
         # elas diziam esta na nota do bloco, que e onde se le antes de decidir.
         "IO20": ("MIDI_TX", ""),
         "IO21": ("MIDI_RX", ""),
         "IO3": ("PONTE_STATUS", "")}
_vistos = set()
for _n, px, py in pinos_de("ESP32C3"):
    x, y = EX + px, EY - py
    esq = px < 0
    if _n in PONTE:
        rot, obs = PONTE[_n]
        dx = -10.16 if esq else 10.16
        fio(x, y, x + dx, y)
        rotulo(rot, x + dx, y, 180 if esq else 0, "right" if esq else "left")
        if obs: texto(obs, EX + (-30 if esq else 20), y, 1.0)
        _vistos.add(_n)
    else:
        nao_liga(x, y)
if _vistos != set(PONTE):
    raise SystemExit("*** o mapa da ponte cita pinos que o simbolo nao tem: %s"
                     % (set(PONTE) - _vistos))
texto("PONTE ESP32-C3 EM SOQUETE — TX/RX CRUZADOS: IO20 e o RX do modulo,", EX - 40, EY - 18.0, 1.6)
texto("IO21 o TX. A terra vai pelo PLANO: o soquete tratou disso.", EX - 40, EY - 14.2)
texto("IO8 e o LED de bordo, IO9 e o BOOT — nao usar. Antena no lado", EX - 40, EY + 14.2, 1.6)
texto("OPOSTO ao USB: manter cobre longe dela no layout.", EX - 40, EY + 18.0)
texto("+5V_PONTE e o +5V ATRAS do D2, com o C3/C4 locais: o USB do modulo,", EX - 40, EY + 21.8, 1.6)
texto("ligado para reprogramar, nao realimenta o trilho da placa toda.", EX - 40, EY + 25.6)

# --------------------------------------------------- modulo do amp (4 pinos)
AX, AY = 300.0, 110.0
simbolo("CONN4_L", "J11", "modulo TPA3116", AX, AY, campos=campos("J11", "modulo TPA3116"))
for (_n, px, py), rot in zip(pinos_de("CONN4_L"), ["+15V", "GND", "AMP_IN_L", "AMP_IN_R"]):
    x, y = AX + px, AY - py          # y do simbolo cresce para CIMA
    fio(x, y, x - 10.16, y); rotulo(rot, x - 10.16, y, 180, "right")
texto("MODULO DO AMP — 15 V e as duas linhas.", AX - 40, AY - 11.43, 1.6)


# ------------------------------------- DPDT: exciter OU linha, por canal
#
# Comum = a saida da Daisy. Uma posicao vai ao borne do exciter, a outra ao
# TRS de linha. Os DOIS comuns amarrados (o dB decide os dois canais juntos).
# ⚠️ A referencia leva NUMERO, nunca letra. `SWL`/`SWR` liam-se bem mas o
# KiCad trata uma referencia sem parte numerica como NAO ANOTADA, e renumera-a
# sozinho ao gravar — a correspondencia com os desenhos quebra em silencio.
# O canal continua a ler-se: esta no valor e na nota, que e onde se le.
# ⚠️ CONECTOR, E NAO A CHAVE. O DPDT tipo 2 e de PAINEL: prende pela rosca na
# caixa e liga por fio. A BOM-MPN ja o dizia ("J DPDT | header 1x6"), mas o
# esquema desenhava a chave — e o layout ia tentar pousa-la no PCB. Mesma
# correcao feita aos botoes arcade do painel a 05/09.
#
# Comum = a saida da Daisy, nos DOIS polos. Uma posicao vai ao borne do
# exciter, a outra ao TRS de linha. Os dois comuns amarrados.
for n, (canal, (SX, SY)) in enumerate((("L", (300.0, 175.0)),
                                       ("R", (300.0, 235.0))), start=15):
    ref = f"J{n}"
    simbolo("CONN8_L", ref, f"fio -> DPDT tipo 2, canal {canal}", SX, SY,
            campos=campos(ref, f"fio -> DPDT tipo 2, canal {canal}"))
    # a ordem das vias e a dos pinos da chave: 1 e 4 comuns, 2/5 exciter,
    # 3/6 linha. Mantida para o cabo poder ser crimpado pino a pino.
    # ⚠️ 2/5 = LINHA e 3/6 = EXCITER, e nao o contrario. No desenho antigo o
    # fio em SY+7,62 encontrava o pino cujo y no SIMBOLO era -7,62 (na folha o
    # y cresce para baixo), e a leitura do codigo dava a troca. Quem apanhou
    # foi a comparacao de netlist, nao o olho.
    # ⚠️ 7 e 8 SAO TERRA, e os seis primeiros nao se mexem.
    #
    # O `kicad-happy` — revisao de outra pessoa — acusou este conector de nao
    # ter um unico pino de terra em seis. A minha primeira leitura foi que o
    # sinal saia sem referencia de retorno; ao ler ESTE bloco vi que e mais
    # subtil: o sinal sai no pino 1 e VOLTA pelo 2 ou pelo 3 conforme a chave,
    # portanto o retorno ja esta num condutor do mesmo cabo e a malha ja e
    # pequena. O que faltava nao era o retorno — era a REFERENCIA: um cabo de
    # audio de 150 mm ate uma chave de painel, sem terra nenhuma ao lado,
    # apanha modo comum e nao tem por onde o escoar.
    #
    # ⚠️ E os pinos 1..6 ficam onde estavam DE PROPOSITO. O cabo e crimpado
    # pino a pino contra os terminais da chave; renumerar o meio para
    # intercalar terra daria um arranjo melhor no papel e um cabo que ja nao
    # corresponde ao que esta montado. A terra entra no fim, onde nao desloca
    # nada.
    mapa = {"1": f"OUT_{canal}_DAISY", "4": f"OUT_{canal}_DAISY",
            "2": f"LINHA_{canal}",     "5": f"LINHA_{canal}",
            "3": f"EXC_{canal}",       "6": f"EXC_{canal}",
            "7": "GND",                "8": "GND"}
    for num, px, py in pinos_de("CONN8_L"):
        x, y = SX + px, SY - py
        esq = px < 0
        dx = -10.16 if esq else 10.16
        fio(x, y, x + dx, y)
        rotulo(mapa[num], x + dx, y, 180 if esq else 0, "right" if esq else "left")
    texto(f"canal {canal} — exciter OU linha, comuns amarrados. 1,4=comum "
          f"2,5=linha 3,6=exciter 7,8=GND (referencia do cabo)",
          SX - 30, SY - 19.05, 1.27)

# ------------------------ pontos de teste
#
# ⚠️ NAO SE POE UM EM CADA REDE. A revisao de fora conta "0 de 69" e a resposta
# preguicosa e por 69 — numa placa de 180x100, com a caixa fechada por cima. O
# criterio nao e a cobertura: e **o que se mede quando ela nao funciona**, e
# isso e uma lista curta que se escreve a pensar na bancada e nao no relatorio.
#
# A ordem e a da depuracao real: primeiro se ha tensao, depois se ha referencia,
# depois se ha sinal. Cada um leva a razao escrita, porque um ponto de teste sem
# razao e um furo que ninguem sabe para que serve.
#
# ⚠️ E O `AGND` TEM O SEU. Sem um ponto em cada terra nao se pode medir a ponte:
# a diferenca entre elas com o instrumento a tocar e o unico numero que diz se o
# NT1 esta a fazer o que devia.
#
# ⚠️ Footprint `TestPoint_Pad_D1.5mm`, que traz `exclude_from_pos_files
# exclude_from_bom`: e cobre, nao e peca. Nao entra no CPL nem na lista de
# compras, e a fabrica nao tenta montar nada la.
TESTE = [
    ("V_TRILHO_A2", "trilho bruto, antes de tudo"),
    ("+15V",        "depois do fusivel e do TVS"),
    ("V+PRE",       "alimentacao do pre-amp, depois do R7"),
    ("VBIAS",       "meia tensao do divisor — se estiver fora, o audio ceifa"),
    ("+5V",         "saida do step-down"),
    ("+5V_PONTE",   "depois do Schottky, o que a ESP32 ve mesmo"),
    ("+3V3A",       "alimentacao dos pots"),
    ("GND",         "referencia da sonda"),
    ("AGND",        "a outra terra — a diferenca para GND mede a ponte"),
    ("LINHA_L",     "saida de audio L, antes do jack"),
    ("LINHA_R",     "saida de audio R"),
]
for i, (rede, porque) in enumerate(TESTE):
    ref = f"TP{i + 1}"
    TX, TY = 365.0, 30.0 + i * 11.0
    simbolo("CONN1_L", ref, rede, TX, TY, campos=campos(ref, rede))
    for _num, px, py in pinos_de("CONN1_L"):
        x, y = TX + px, TY - py
        fio(x, y, x - 10.16, y)
        rotulo(rede, x - 10.16, y, 180, "right")
    texto(porque, TX + 6.35, TY + 1.27, 1.0)
texto("PONTOS DE TESTE — o que se mede quando nao funciona",
      365.0 - 24.0, 30.0 - 12.7, 1.6)

# ------------------------ 10 k de cada entrada do amp ao GND do modulo
#
# Referencia de entrada do amp. Nao e forca: fica na ilha do modulo.
for i, (canal, RY) in enumerate((("L", 285.0), ("R", 300.0))):
    RX = 120.0
    # R5x e nao R2x: o pre-amp usa a dezena 2 para o canal B dos seus
    # resistores, e a referencia tem de ser unica em TODA a hierarquia.
    simbolo("R_H", f"R5{i}", f"10k — {canal}", RX, RY,
            campos=campos(f"R5{i}", f"10k — {canal}"))
    fio(RX - 6.35, RY, RX - 16.51, RY); rotulo(f"AMP_IN_{canal}", RX - 16.51, RY, 180, "right")
    fio(RX + 6.35, RY, RX + 16.51, RY); rotulo("GND", RX + 16.51, RY, 0, "left")
nota(90.0, 271.2,
     "10 k de cada entrada do amp ao GND do modulo — referencia,",
     "nao forca. Sem eles a entrada flutua quando o DPDT comuta.")

# ---------------------------------------------- TRS de linha, sleeve ao AGND
for i, (canal, JY) in enumerate((("L", 175.0), ("R", 235.0))):
    JX = 385.0
    # ⚠️ CONECTOR: o jack TRS de 1/4" e de painel (Neutrik NMJ6HCD2), prende
    # na caixa e liga por fio — a BOM-MPN diz "header 1x3".
    simbolo("CONN3_L", f"J1{i+2}", f"fio -> TRS linha {canal}", JX, JY,
            campos=campos(f"J1{i+2}", f"fio -> TRS linha {canal}"))
    # 1 = tip (sinal) · 2 = ring · 3 = sleeve. Ring E sleeve ao AGND.
    for (n, px, py), rot in zip(pinos_de("CONN3_L"),
                                [f"LINHA_{canal}", "AGND", "AGND"]):
        x, y = JX + px, JY - py
        esq = px < 0
        dx = -10.16 if esq else 10.16
        fio(x, y, x + dx, y)
        rotulo(rot, x + dx, y, 180 if esq else 0, "right" if esq else "left")
texto("TRS de linha — sleeve E ring ao AGND, como os jacks de piezo.", 350.0, 155.0, 1.6)

# --------------------------------------------------------------- escrever
# ⚠️ OS SIMBOLOS TEM DE VIR EMBUTIDOS. Uma folha com `(lib_symbols)` vazio
# abre, mas o KiCad nao sabe onde estao os pinos: 69 fios pendurados e 69
# rotulos soltos, em pares perfeitos — que e a assinatura de "nao ha pinos",
# nao de "ha ligacoes erradas". A `sym-lib-table` serve o editor; o ficheiro
# tem de se bastar.
def _confere_lib(caminho):
    """Depois de escrever: todo `lib_id` tem entrada no `lib_symbols`?

    Um simbolo referido e nao embutido nao da erro nenhum — o KiCad ve um
    simbolo SEM PINOS e cada fio fica solto. A 05/09 custou 10 fios soltos no
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

# ------------------------------------- D29 e D30 sao o USB: NO-CONNECT
#
# ⚠️ Nao sao "pinos livres", sao PROIBIDOS. Sao o USB_HS_D-/D+ (pinos 36 e 37)
# e usa-los como GPIO mata a enumeracao — cicatriz 15by, em que o PINOS.md os
# reservava para os DPDT. Ate agora isso vivia so em documento; o `no_connect`
# poe a regra dentro do ficheiro, e o ERC passa a recusar quem la ligue um fio.
#
# Os outros 11 pinos livres NAO levam no-connect: sao reserva declarada no
# FUTURO.md, e prometer "nada se liga aqui" seria mentir sobre eles.
for _n in (36, 37):
    _x, _y = pin_xy(_n)
    L.append('\t(no_connect\n\t\t(at %s %s)\n\t\t(uuid "%s")\n\t)\n' % (g(_x), g(_y), u()))


# ------------------------------------- desacoplamento do +5V junto a Daisy
#
# ⚠️ O trilho +5V estava SEM UM UNICO CONDENSADOR. A revisao de terra (P4) diz
# que logica sem par local da "ruido difuso", e o +5V alimenta a Daisy, a fita
# do painel e o step-down. O 100 uF nao vem aqui: a rajada e o radio, e o radio
# esta atras do D2 com o seu proprio par (C3/C4). Aqui basta o pequeno, que e
# quem deixa passar as arestas.
CDX, CDY = 150.0, 300.0
simbolo("C_V", "C15", "100n", CDX, CDY, campos=campos("C15", "100n"))
for (_n, px, py), (rot, ang, just) in zip(pinos_de("C_V"),
                                          [("+5V", 90, "left"), ("GND", 270, "left")]):
    x, y = CDX + px, CDY - py
    dy = -10.16 if py > 0 else 10.16
    fio(x, y, x, y - dy); rotulo(rot, x, y - dy, ang, just)
nota(CDX - 30, CDY + 20.0,
     "100 nF do +5V ao GND, colado ao pino 39 da Daisy. O trilho nao tinha",
     "nenhum: achado da P4 da revisao de terra, 06/09.")

# ------------------------------------------- os dois PWR_FLAG que faltavam
#
# ⚠️ O `ERC-ISENCOES.md` e a BOM diziam QUATRO `PWR_FLAG`; o esquema tinha
# DOIS, e sobravam exactamente 2 `power_pin_not_driven` — os pinos 39 (+5V) e
# 40 (GND) da Daisy. Simbolo caseiro nao declara `power_out`, entao todo pino
# `power_in` fica orfao; a resposta e declarar quem alimenta, nao isentar.
# Estes dois NAO sao pecas: `in_bom no`, fora da BOM e do CPL.
# ⚠️ O terceiro nasceu com o `+5V_PONTE`: quem o alimenta e o catodo de um
# Schottky, e o ERC nao sabe que um diodo e fonte. Declara-se quem alimenta —
# nao se isenta o aviso, que era so tapar a pergunta.
for k, (rede, FX, FY) in enumerate((("+5V", 60.0, 300.0), ("GND", 90.0, 300.0),
                                    ("+5V_PONTE", 160.0, 300.0)), start=3):
    ref = f"#FLG{k}"
    simbolo("PWR_FLAG", ref, "PWR_FLAG", FX, FY,
            campos=(("Reference", ref, 0, -2.54, 1.27, True, ""),
                    ("Value", "PWR_FLAG", 0, 4.44, 1.0, False, ""),
                    ("Footprint", "", 0, 0, 1.27, True, ""),
                    ("Datasheet", "", 0, 0, 1.27, True, ""),
                    ("Description", "", 0, 0, 1.27, True, "")))
    # o fio sobe: com o rotulo em baixo ele batia no valor "PWR_FLAG",
    # que o campos() poe a +4,44 (na folha, +y e para BAIXO).
    fio(FX, FY, FX, FY - 5.08)
    rotulo(rede, FX, FY - 5.08, 90, "left")


def lib_embutida(usados):
    L = io.open("cotuca.kicad_sym", encoding="utf-8").read()
    out = []
    for nome in sorted(usados):
        i = L.find('(symbol "%s"' % nome)
        if i < 0:
            raise SystemExit("simbolo ausente da biblioteca: %s" % nome)
        d = 0
        for k in range(i, len(L)):
            if L[k] == "(": d += 1
            elif L[k] == ")":
                d -= 1
                if d == 0:
                    b = L[i:k+1]; break
        b = b.replace('(symbol "%s"' % nome, '(symbol "cotuca:%s"' % nome, 1)
        out.append("\t\t" + b.replace("\n  ", "\n\t\t"))
    return "\t(lib_symbols\n" + "\n".join(out) + "\n\t)\n"

# USADOS sai de simbolo(), nunca de uma lista a mao — ver gera_painel v0.7.1.

CAB = f'''(kicad_sch
\t(version 20260306)
\t(generator "cotucatronics")
\t(generator_version "10.0")
\t(uuid "{u()}")
\t(paper "A2")
\t(title_block
\t\t(title "SOQUETE + PONTE + DPDT + SAIDAS — carrier rev B")
\t\t(date "2026-09-05")
\t\t(rev "B")
\t\t(comment 3 "A ponte AGND<->GND e o NT1, e e a UNICA. Duas redes no ERC.")
\t\t(comment 2 "Dnn -> pino do header: Seed3_pinout.csv, nunca memoria.")
\t\t(comment 1 "D29/D30 sao o USB e nao se usam. Os DPDT nao tem sense (FUTURO.md).")
\t)
{lib_embutida(USADOS)}
'''
FIM = '\t(sheet_instances\n\t\t(path "/" (page "4"))\n\t)\n\t(embedded_fonts no)\n)\n'
io.open("cotuca_soquete.kicad_sch", "w", encoding="utf-8").write(CAB + "".join(L) + FIM)
print("gera_soquete v%s -> cotuca_soquete.kicad_sch: %d blocos"
      % (VERSAO, len(L)), file=sys.stderr)
print("  lib_symbols coerente: " + str(_confere_lib("cotuca_soquete.kicad_sch")) + " simbolos",
      file=sys.stderr)
