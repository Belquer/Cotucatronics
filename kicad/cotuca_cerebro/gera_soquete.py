#!/usr/bin/env python3
"""gera_soquete v0.1.0 — a folha SOQUETE + PONTE + DPDT + SAIDAS, rev B.

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
   vem do `Daisy_Seed_Rev4_Pinout.csv` via `gera_daisy_sym.py`.
   ⚠️ D29/D30 SAO O USB (pinos 36/37). Nao se usam. O sense dos DPDT vai
   para D11/D12.

 · Ligacao por ROTULOS GLOBAIS e nao por fios longos: a folha da energia e a
   do pre-amp ja usam isso, e o netlist da raiz prova que atravessa.
"""
import io, sys, uuid

VERSAO = "0.1.0"
G = 1.27
def u(): return str(uuid.uuid4())
def g(v): return round(round(v / G) * G, 4)

L = []   # blocos do ficheiro

def simbolo(lib, ref, val, x, y, ang=0, campos=()):
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
\t\t(in_bom yes)
\t\t(on_board yes)
\t\t(in_pos_files yes)
\t\t(dnp no)
\t\t(uuid "{u()}")
{ps}\t)
''')

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
    for m in _re.finditer(r'\(pin\s+\S+\s+\S+\s*\n\s*\(at (-?[\d.]+) (-?[\d.]+)'
                          r'(?:\s+-?[\d.]+)?\)(?:.|\n)*?\(number "([^"]+)"', b):
        out.append((m.group(3), float(m.group(1)), float(m.group(2))))
    return out


def campos(ref, val, dx=0, dy=0):
    return (("Reference", ref, dx, dy - 2.54, 1.27, False, ""),
            ("Value", val, dx, dy + 2.54, 1.0, False, ""),
            ("Footprint", "", 0, 0, 1.27, True, ""),
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
    "AGND":"AGND", "GND":"GND", "VIN":"+5V", "3v3D":"+3V3", "3v3A":"+3V3A",
    "D13":"MIDI_TX", "D14":"MIDI_RX", "D28":"PONTE_STATUS",
    "D17":"V_TRILHO_A2",
    "D5":"OLED_RST","D6":"OLED_DC","D7":"OLED_CS","D8":"OLED_CLK","D10":"OLED_DIN",
    "D0":"ENC_A_A","D1":"ENC_A_B","D2":"ENC_A_PUSH",
    "D3":"ENC_B_A","D4":"ENC_B_B","D9":"ENC_B_PUSH",
    "D19":"ENC_G_A","D18":"ENC_G_B","D17_":"",
    "D15":"POT_L","D16":"POT_R",
    "D22":"BOT_FRZ_B","D23":"LED_FRZ_B","D24":"BOT_FRZ_A","D25":"BOT_REARME",
    "D26":"BOT_BUZIOS","D27":"LED_FRZ_A",
    "D11":"DPDT_SENSE_L","D12":"DPDT_SENSE_R",
}
# ⚠️ D29 e D30 NAO entram. Sao USB_HS_D-/D+ (pinos 36/37). Ver PINOS.md.

simbolo("DAISY_SEED", "U2", "Daisy Seed rev4", DX, DY,
        campos=(("Reference","U2",0,-30.48,1.27,False,""),
                ("Value","Daisy Seed rev4",0,30.48,1.0,False,""),
                ("Footprint","",0,0,1.27,True,""),
                ("Datasheet","Daisy_Seed_Rev4_Pinout.csv",0,0,1.27,True,""),
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

# ------------------------------------------------------- ponte ESP32 (5 pinos)
EX, EY = 300.0, 60.0
simbolo("CONN5_L", "J10", "ponte ESP32-C3", EX, EY, campos=campos("J10", "ponte ESP32-C3"))
# ⚠️ `y` DO SIMBOLO CRESCE PARA CIMA; o da folha, para BAIXO. Logo
# `abs_y = simbolo_y - offset`, com MENOS. Somar espelha o conector: o +5V
# do pino 1 ia parar ao pino 5, e o unico sinal era um `pin_not_connected`
# num extremo — os do meio calham bater por simetria.
for (_n, px, py), (rot, nota) in zip(
        pinos_de("CONN5_L"),
        [("+5V",""), ("GND",""), ("MIDI_TX","-> RX da ponte"),
         ("MIDI_RX","<- TX da ponte"), ("PONTE_STATUS","")]):
    x, y = EX + px, EY - py
    fio(x, y, x - 10.16, y); rotulo(rot, x - 10.16, y, 180, "right")
    if nota: texto(nota, EX + 3.81, y, 1.0)
texto("PONTE ESP32 — TX/RX CRUZADOS. E a terra de sinal vai JUNTO", EX - 40, EY - 15.24, 1.6)
texto("com os fios: sem ela, a placa viva injecta pelos diodos da morta.", EX - 40, EY - 11.43)

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
for canal, (SX, SY) in (("L", (300.0, 175.0)), ("R", (300.0, 235.0))):
    simbolo("DPDT", f"SW{canal}", "DPDT tipo 2", SX, SY,
            campos=campos(f"SW{canal}", "DPDT tipo 2"))
    # comuns: os dois polos recebem a MESMA saida da Daisy
    for dy in (5.08, -5.08):
        fio(SX - 11.43, SY + dy, SX - 21.59, SY + dy)
        rotulo(f"OUT_{canal}_DAISY", SX - 21.59, SY + dy, 180, "right")
    # posicao 1 -> exciter · posicao 2 -> linha
    for py, rot in ((7.62, f"EXC_{canal}"), (2.54, f"LINHA_{canal}"),
                    (-2.54, f"EXC_{canal}"), (-7.62, f"LINHA_{canal}")):
        fio(SX + 11.43, SY + py, SX + 21.59, SY + py)
        rotulo(rot, SX + 21.59, SY + py, 0, "left")
    texto(f"DPDT canal {canal} — exciter OU linha. Comuns amarrados.",
          SX - 30, SY - 13.97, 1.6)

# ------------------------ 10 k de cada entrada do amp ao GND do modulo
#
# Referencia de entrada do amp. Nao e forca: fica na ilha do modulo.
for i, (canal, RY) in enumerate((("L", 285.0), ("R", 300.0))):
    RX = 120.0
    simbolo("R_H", f"R2{i}", "10k", RX, RY, campos=campos(f"R2{i}", "10k"))
    fio(RX - 6.35, RY, RX - 16.51, RY); rotulo(f"AMP_IN_{canal}", RX - 16.51, RY, 180, "right")
    fio(RX + 6.35, RY, RX + 16.51, RY); rotulo("GND", RX + 16.51, RY, 0, "left")
texto("10 k de cada entrada do amp ao GND do modulo — referencia,", 90.0, 275.0, 1.6)
texto("nao forca. Sem eles a entrada flutua quando o DPDT comuta.", 90.0, 271.2)

# ---------------------------------------------- TRS de linha, sleeve ao AGND
for i, (canal, JY) in enumerate((("L", 175.0), ("R", 235.0))):
    JX = 385.0
    simbolo("JACK3", f"J1{i+2}", f"TRS linha {canal}", JX, JY,
            campos=campos(f"J1{i+2}", f"TRS linha {canal}"))
    # ⚠️ o JACK3 tem os pinos a DIREITA (x = +2,54), nao a esquerda.
    # 1 = tip (sinal) · 2 = ring · 3 = sleeve. Ring E sleeve ao AGND.
    for (n, px, py), rot in zip(pinos_de("JACK3"),
                                [f"LINHA_{canal}", "AGND", "AGND"]):
        x, y = JX + px, JY - py
        fio(x, y, x + 10.16, y); rotulo(rot, x + 10.16, y, 0, "left")
texto("TRS de linha — sleeve E ring ao AGND, como os jacks de piezo.", 350.0, 155.0, 1.6)

# --------------------------------------------------------------- escrever
# ⚠️ OS SIMBOLOS TEM DE VIR EMBUTIDOS. Uma folha com `(lib_symbols)` vazio
# abre, mas o KiCad nao sabe onde estao os pinos: 69 fios pendurados e 69
# rotulos soltos, em pares perfeitos — que e a assinatura de "nao ha pinos",
# nao de "ha ligacoes erradas". A `sym-lib-table` serve o editor; o ficheiro
# tem de se bastar.
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

USADOS = {"DAISY_SEED", "NETTIE2", "CONN5_L", "CONN4_L", "DPDT", "R_H", "JACK3"}

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
\t\t(comment 1 "A ponte AGND<->GND e o NT1, e e a UNICA. Duas redes no ERC.")
\t\t(comment 2 "Dnn -> pino do header: Daisy_Seed_Rev4_Pinout.csv, nunca memoria.")
\t\t(comment 3 "D29/D30 sao o USB e nao se usam. Sense dos DPDT em D11/D12.")
\t)
{lib_embutida(USADOS)}
'''
FIM = '\t(sheet_instances\n\t\t(path "/" (page "4"))\n\t)\n\t(embedded_fonts no)\n)\n'
io.open("cotuca_soquete.kicad_sch", "w", encoding="utf-8").write(CAB + "".join(L) + FIM)
print("cotuca_soquete.kicad_sch: %d blocos" % len(L), file=sys.stderr)
