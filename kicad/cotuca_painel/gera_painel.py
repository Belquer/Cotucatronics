#!/usr/bin/env python3
"""gera_painel v0.9.0 — o projecto do PAINEL (tampa da caixa).

Segunda placa. A primeira e `cotuca_cerebro`, e o CONTRATO entre as duas e o
IDC 2x13: **o mesmo nome de rede dos dois lados, via a via**.

⚠️ A BIBLIOTECA E A MESMA, por `sym-lib-table` apontando para a do cerebro.
Duas copias derivavam, e o contrato e nomes iguais.

⚠️ NENHUM NOME SE INVENTA AQUI. Cada rotulo tem de existir, com a mesma
grafia, em `cotuca_soquete.kicad_sch` — e o `tools/contrato_idc.py` confere
as duas listas uma contra a outra. Um rotulo global sozinho e legal, portanto
o ERC nao apanha uma grafia trocada: so a comparacao apanha.

⚠️ UM TERRA SO. O painel nao tem ilha analogica: tudo referencia o DGND que
chega pelo IDC. A separacao AGND/GND vive no cerebro, no net-tie, e nao
atravessa este cabo.
"""
import io, sys, uuid

VERSAO = "0.9.0"

# ⚠️ A IDENTIDADE DA RAIZ E FIXA, e nao um uuid novo a cada corrida.
#
# O `.kicad_pro` guarda o uuid da folha raiz e o KiCad usa-o como primeiro
# segmento do caminho em `(instances)`. Um gerador que mine um uuid novo a cada
# corrida garante que os dois NUNCA batem — e ficheiro sem `(instances)`
# coerente e ficheiro que o KiCad repara ao abrir, com caixa a pedir para
# gravar. Nenhuma ferramenta de linha de comando denuncia isso.
#
# ⚠️ E este uuid E DO PAINEL. O `.kicad_pro` do painel nasceu copiado do do
# cerebro e os dois projectos reclamavam a MESMA identidade de folha raiz
# (`2ad2d1f2...`) — dois projectos, uma identidade.
RAIZ = "de128d62-4042-4c6e-a783-d968ae52c1c6"
G = 1.27
def u(): return str(uuid.uuid4())
def g(v): return round(round(v / G) * G, 4)
L = []
USADOS = set()   # preenchido por simbolo(); NUNCA escrito a mao
LIB = "../cotuca_cerebro/cotuca.kicad_sym"

def pinos_de(nome):
    import re as _re
    Lb = io.open(LIB, encoding="utf-8").read()
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

def simbolo(lib, ref, val, x, y, ry=-7.62, vy=7.62):
    USADOS.add(lib)
    x, y = g(x), g(y)
    ps = "".join(
        f'\t\t(property "{k}" "{v}"\n\t\t\t(at {g(x)} {g(y+dy)} 0)\n'
        f'\t\t\t{"(hide yes) " if h else ""}(show_name no) (do_not_autoplace no)\n'
        f'\t\t\t(effects (font (size {s} {s})))\n\t\t)\n'
        for k, v, dy, s, h in (("Reference", ref, ry, 1.27, False), ("Value", val, vy, 1.0, False),
                               ("Footprint", _fp(ref), 0, 1.27, True), ("Datasheet", "", 0, 1.27, True),
                               ("Description", "", 0, 1.27, True)))
    L.append(f'\t(symbol\n\t\t(lib_id "cotuca:{lib}")\n\t\t(at {x} {y} 0)\n\t\t(unit 1)\n'
             f'\t\t(body_style 1)\n\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n\t\t(on_board yes)\n'
             f'\t\t(in_pos_files yes)\n\t\t(dnp no)\n\t\t(uuid "{u()}")\n{ps}{_instancia(ref)}\t)\n')


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
    "J1":  "Connector_IDC:IDC-Header_2x13_P2.54mm_Vertical",
    # JST XH e nao header 2,54: e o que a BOM-MPN escolheu (B2B-XH-A), e um
    # conector com trava nao se solta com a tampa a fechar sobre os fios.
    "J3":  "Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical",
    "J4":  "Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical",
    "J5":  "Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical",
    "J6":  "Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical",
    "J7":  "Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical",
    # OLED: o "MX1.25" do modulo Waveshare e um Molex PicoBlade 53047-0710
    "J2":  "Connector_Molex:Molex_PicoBlade_53047-0710_1x07_P1.25mm_Vertical",
    "Q1":  "Package_TO_SOT_SMD:SOT-23",
    "Q2":  "Package_TO_SOT_SMD:SOT-23",
    "C1":  "Capacitor_SMD:C_0805_2012Metric",
    "C2":  "Capacitor_SMD:C_0805_2012Metric",
    "R1":  "Resistor_SMD:R_0805_2012Metric",
    "R2":  "Resistor_SMD:R_0805_2012Metric",
    "R3":  "Resistor_SMD:R_0805_2012Metric",
    "R4":  "Resistor_SMD:R_0805_2012Metric",
    # ⚠️ R5/R6 sao os de SERIE DOS LED. A BOM-MPN diz 47 Ω **1/2 W** (1210 ou
    # 2512) e o esquema ainda diz "100R serie (~30 mA)". O VALOR esta por
    # decidir; o encapsulamento 0805 estava errado em qualquer dos casos —
    # 0805 e 1/8 W. 1210 aguenta 1/2 W nas series que o declaram; se a peca
    # escolhida for 1/4 W, sobe para 2512.
    "R5":  "Resistor_SMD:R_1210_3225Metric",
    "R6":  "Resistor_SMD:R_1210_3225Metric",
    # Alps EC11E, decidido a 05/09: pinos a 2,5 mm, footprint oficial no
    # KiCad, e ja era a peca do PINOS.md. LCSC C370970 (EC11E15244G1), com
    # push, 30 detentes/15 pulsos, eixo de 20 mm — US$1,70 @5, 1125 em stock.
    "SW1": "Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm",
    # POR CONFIRMAR, e nao por medir:
    #  SW1  o PEC11R da BOM NAO tem footprint no KiCad e NAO e o Alps EC11E:
    #       a ficha da Bourns poe os pinos do encoder a 5,0 mm e o Alps a 2,5.
    #  RV1/RV2  o PTV09A-4 da BOM e "Horizontal/Rear Mount, BUSHINGLESS" — sem
    #       rosca e com eixo deitado. Nao serve para eixo a subir pelo furo da
    #       tampa. O estilo vertical e o PTV09A-1, que TEM footprint no KiCad.
}


def _fp(ref):
    return FOOTPRINT.get(ref, "")

def _instancia(ref, unidade=1):
    """O bloco `(instances)`. NAO E OPCIONAL — ver gera_soquete.py.

    O painel e UMA folha, que e a sua propria raiz: o caminho e `/<raiz>`,
    sem segundo segmento. Numa hierarquia seria `/<raiz>/<folha>`."""
    return ('\t\t(instances\n\t\t\t(project "cotuca_painel"\n'
            '\t\t\t\t(path "/%s"\n\t\t\t\t\t(reference "%s")\n'
            '\t\t\t\t\t(unit %d)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n'
            % (RAIZ, ref, unidade))


def fio(x1, y1, x2, y2):
    L.append(f'\t(wire\n\t\t(pts\n\t\t\t(xy {g(x1)} {g(y1)}) (xy {g(x2)} {g(y2)})\n\t\t)\n'
             f'\t\t(stroke (width 0) (type default))\n\t\t(uuid "{u()}")\n\t)\n')

def rot(nome, x, y, ang, just):
    L.append(f'\t(global_label "{nome}"\n\t\t(shape bidirectional)\n\t\t(at {g(x)} {g(y)} {ang})\n'
             f'\t\t(fields_autoplaced yes)\n\t\t(effects (font (size 1.27 1.27)) (justify {just}))\n'
             f'\t\t(uuid "{u()}")\n\t)\n')

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


def liga(lib, ref, x, y, mapa, val="", ry=-7.62, vy=7.62, comp=10.16):
    """Coloca e liga cada pino a um rotulo global. `mapa` = {numero: rotulo}."""
    simbolo(lib, ref, val, x, y, ry, vy)
    for n, px, py in pinos_de(lib):
        if n not in mapa: continue
        ax, ay = g(x) + px, g(y) - py
        esq = px < 0
        dx = -comp if esq else comp
        fio(ax, ay, ax + dx, ay)
        rot(mapa[n], ax + dx, ay, 180 if esq else 0, "right" if esq else "left")


# ---------------------------------------------------- o IDC, o contrato
VIAS = {1:"GND", 2:"OLED_CLK", 3:"GND", 4:"OLED_DIN", 5:"OLED_CS", 6:"OLED_DC",
        7:"OLED_RST", 8:"GND", 9:"+3V3A", 10:"GND", 11:"+5V", 12:"+5V",
        13:"GND", 14:"GND", 15:"LED_FRZ_A", 16:"LED_FRZ_B", 17:"BOT_FRZ_A",
        18:"BOT_FRZ_B", 19:"BOT_BUZIOS", 20:"ENC_A_A", 21:"ENC_A_B",
        22:"ENC_A_PUSH", 23:"GND", 24:"POT_L", 25:"POT_R", 26:"GND"}
liga("IDC26", "J1", 90.0, 120.0, {str(k): v for k, v in VIAS.items()},
     "IDC 2x13 femea <- cerebro", -21.59, 21.59)
nota(55.0, 84.4,
     "O CONTRATO. Estes 26 nomes sao IDENTICOS aos da folha do soquete do",
     "cerebro — e o `tools/contrato_idc.py` confere as duas listas uma contra",
     "a outra. O ERC nao apanha uma grafia trocada: rotulo global sozinho e legal.")

# ------------------------------------------------------------------ OLED
liga("CONN7_L", "J2", 210.0, 60.0,
     {"1":"GND", "2":"+5V", "3":"OLED_CLK", "4":"OLED_DIN",
      "5":"OLED_CS", "6":"OLED_DC", "7":"OLED_RST"}, "OLED SSD1309 MX1.25 7p")
nota(175.0, 36.2,
     "OLED a 5 V, e nao a 3V3 — decidido a 05/09. O modulo Waveshare aceita",
     "3,3 ou 5 V (tem tradutor de nivel a bordo), e tira-lo do trilho de 3V3",
     "liberta a via 9 para o +3V3A dos pots. Ecra comutado no fundo de escala",
     "do ADC e pior do que o problema que se resolvia. O painel nao gera nada.")

# --------------------------------------------------------------- encoder
liga("EC11", "SW1", 210.0, 120.0,
     {"1":"ENC_A_A", "2":"GND", "3":"ENC_A_B", "4":"ENC_A_PUSH", "5":"GND"},
     "EC11 — o unico encoder")
nota(175.0, 99.2,
     "ENCODER — comum (C) e um pino do push ao GND. E o UNICO desde 05/09:",
     "push+giro dao os outros controlos, e a lista de presets e simples.")

# ------------------------------------------------------------------ pots
for i, (ref, sinal, y) in enumerate((("RV1", "POT_L", 175.0), ("RV2", "POT_R", 215.0))):
    # ⚠️ +3V3A, NAO +3V3. Sao DOIS REGULADORES na Daisy — a ficha do Seed3
    # diz "Both the Digital and Analog +3.3V pins will output the voltage from
    # THEIR regulators" — e o fundo de escala do ADC e o **3V3A**: "65535
    # corresponding to +3V3A". Alimentar o pot de um trilho e le-lo contra o
    # outro poe a diferenca dos dois reguladores dentro da leitura. A propria
    # ficha da a figura "4.1 - Potentiometer" alimentada do pino +3V3A.
    liga("POT3", ref, 210.0, y, {"1":"GND", "2":sinal, "3":"+3V3A"}, "10k lin")
    # ⚠️ 100 nF do cursor ao GND, COLADO ao pote. A distancia e o ponto:
    # longe, o condensador filtra a pista e nao o cursor.
    cx, cy = 240.0, y + 12.7
    liga("C_V", f"C{i+1}", cx, cy, {"1":sinal, "2":"GND"}, "100n COLADO ao cursor")
nota(175.0, 154.2,
     "POTS — 1 ao GND, 3 ao +3V3A: o MESMO trilho que o ADC usa como fundo",
     "de escala. Alimentar de um regulador e referenciar noutro poe a",
     "diferenca dos dois dentro da leitura. Sao dois na Daisy, e a ficha da a",
     "figura do potenciometro alimentada do pino +3V3A.")

# --------------------------------------------------- botoes: 2 azuis + 1 branco
# passo de 20 mm e nao 15: a 15 o VALOR de um botao (22 caracteres) caia em
# cima da REFERENCIA do seguinte — 3,9 mm², medido.
#
# ⚠️ CONECTOR, E NAO O BOTAO. O arcade de 30 mm tem anel roscado e prende no
# PAINEL, nao na placa: nao existe footprint de placa para ele. O que a placa
# leva e um header de 2 pinos, e o botao chega por fio. Desenhar o botao aqui
# poe o layout a tentar pousar 30 mm de plastico em cima do PCB.
# Decidido com o dB a 05/09, junto com o inverso para o encoder e os pots:
# esses vao NA PLACA, com o eixo pelo furo da tampa (a caixa ja tem os tres
# furos ø7 alinhados a 35 mm), porque o 100 nF do cursor tem de ficar colado.
for ref, sinal, cor, y in (("J3", "BOT_FRZ_A", "fio -> arcade AZUL freeze A", 250.0),
                           ("J4", "BOT_FRZ_B", "fio -> arcade AZUL freeze B", 270.0),
                           ("J5", "BOT_BUZIOS", "fio -> arcade BRANCO buzios", 290.0)):
    # x=85 e nao 100: a 100 o rotulo GND do botao encostava no LED_FRZ_A
    # do R1 (8,2 mm² de sobreposicao, medido pelo colisoes_sch a 05/09).
    liga("CONN2_L", ref, 85.0, y, {"1":sinal, "2":"GND"}, cor)
nota(65.0, 232.4,
     "BOTOES E LEDS SAO CONECTORES, nao pecas da placa: o arcade de 30 mm",
     "prende no painel e chega por fio. Um lado ao GND; o pull-up e interno",
     "ao MCU, no cerebro. Dois azuis (freeze) e UM BRANCO (buzios).")

# ------------------------------------- LED dos arcade, por MOSFET
#
# ⚠️ O PULL-DOWN DE 10 k NO GATE E OBRIGATORIO. Entre o arranque da Daisy e o
# primeiro `Init`, o pino esta em alta impedancia e o gate flutua: sem ele o
# MOSFET pode conduzir sozinho e o LED acende no escuro, ou pior, fica a meio
# caminho a dissipar. Nao e opcional nem "boa pratica": e o que impede o
# estado indefinido de existir.
for i, (canal, sinal, y) in enumerate((("A", "LED_FRZ_A", 250.0), ("B", "LED_FRZ_B", 312.0))):
    QX = 175.0
    liga("R_H", f"R{i+1}", QX - 30.48, y, {"1":sinal, "2":f"GATE_{canal}"}, "100R gate")
    # valor curto e ACIMA do resistor: "10k pull-down OBRIGATORIO" tem 27 mm e
    # batia no GND do MOSFET e no +5V do LED do canal seguinte. A palavra
    # OBRIGATORIO nao se perde — esta na nota deste bloco e no comentario
    # acima, que e onde ela e lida antes de alguem decidir omitir o resistor.
    liga("R_V", f"R{i+3}", QX - 15.24, y + 16.51, {"1":f"GATE_{canal}", "2":"GND"},
         "10k pull-down")
    liga("MOSFET_N", f"Q{i+1}", QX, y, {"1":f"GATE_{canal}", "2":f"LED_K_{canal}", "3":"GND"},
         "AO3400A")
    # tambem CONECTOR: o LED vive DENTRO do botao de painel. Pino 1 = anodo,
    # pino 2 = catodo — a polaridade tem de ir ao silk, que um header nao diz.
    liga("CONN2_L", f"J{i+6}", QX, y - 25.4, {"1":"+5V", "2":f"LED_R_{canal}"},
         f"fio -> LED do arcade {canal} (1=A, 2=K)")
    liga("R_H", f"R{i+5}", QX + 25.4, y - 25.4, {"1":f"LED_R_{canal}", "2":f"LED_K_{canal}"},
         "100R serie (~30 mA)")
# x=30 e nao 130: a 130 a primeira linha entrava 9,8 mm² dentro do RV2 e a
# terceira 1,9 mm² dentro do LED D1. A faixa x 30..125, y 190..210 esta vazia.
nota(30.0, 196.0,
     "LED DOS ARCADE — 5 V, MOSFET em baixo, e o PULL-DOWN DE 10 k NO GATE",
     "e obrigatorio: entre o arranque e o Init o pino esta em alta impedancia",
     "e o gate flutua. Sem ele o LED acende sozinho, ou fica a meio a dissipar.")
# (!) e nao ⚠️: o KiCad nao tem glifo para o emoji e desenha um quadrado vazio.
nota(30.0, 345.0,
     "(!) R serie de 100R e VALOR A CONFIRMAR: depende do Vf do LED do arcade",
     "que o dB comprar. Com Vf 2,0 V da 30 mA; com 3,2 V (azul/branco) da 18 mA.")

# ------------------------------------------------------------- escrever
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

def lib_embutida(usados=None):
    """A biblioteca embutida sai do que foi REALMENTE colocado.

    ⚠️ Era uma lista literal escrita a mao ao lado de uma folha gerada. A 05/09
    trocaram-se `SW_H` e `LED` por `CONN2_L` e a lista ficou a dizer os nomes
    velhos: a folha saiu a referir um simbolo que a biblioteca embutida nao
    tinha, KiCad viu um simbolo SEM PINOS, e os 10 fios ficaram soltos. O
    gerador nao disse nada. Lista a mao ao lado de coisa gerada envelhece
    sempre — a unica cura e nao existir."""
    usados = USADOS if usados is None else usados
    if not usados:
        raise SystemExit("*** nenhum simbolo registado — lib_embutida() foi "
                         "chamada antes de se colocar seja o que for")
    Lb = io.open(LIB, encoding="utf-8").read(); out = []
    for nome in sorted(usados):
        i = Lb.find('(symbol "%s"' % nome); d = 0
        if i < 0:
            raise SystemExit("*** simbolo %s ausente de %s — a folha ia sair a "
                             "refere-lo com a biblioteca vazia, e o KiCad ve "
                             "um simbolo sem pinos" % (nome, LIB))
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
\t(uuid "{RAIZ}")
\t(paper "A2")
\t(title_block
\t\t(title "PAINEL — tampa da caixa — rev B")
\t\t(date "2026-09-05")
\t\t(rev "B")
\t\t(comment 3 "Contrato com o cerebro: IDC 2x13, mesmo nome de rede via a via.")
\t\t(comment 2 "UM terra so. A separacao AGND/GND vive no cerebro, no net-tie.")
\t\t(comment 1 "Pull-down de 10k no gate dos MOSFET: obrigatorio, nao opcional.")
\t)
{lib_embutida()}'''
io.open("cotuca_painel.kicad_sch", "w", encoding="utf-8").write(
    CAB + "".join(L) + '\t(sheet_instances\n\t\t(path "/" (page "1"))\n\t)\n\t(embedded_fonts no)\n)\n')
print("gera_painel v%s -> cotuca_painel.kicad_sch: %d blocos"
      % (VERSAO, len(L)), file=sys.stderr)
print("  lib_symbols coerente: " + str(_confere_lib("cotuca_painel.kicad_sch")) + " simbolos",
      file=sys.stderr)
