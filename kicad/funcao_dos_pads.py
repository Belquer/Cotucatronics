#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""funcao_dos_pads.py — o pad certo leva a rede certa? (significado, nao ligacao)

    funcao_dos_pads.py <ficheiro.kicad_pcb> [...]     sai != 0 se algo nao bate
    funcao_dos_pads.py --sabota                       prova que sabe dizer NAO

⚠️⚠️ A CLASSE QUE ESTA FERRAMENTA EXISTE PARA APANHAR

Um simbolo generico da biblioteca — pinos `1,2,3` sem nome de funcao — casado
com um footprint real cujos pads tem funcao FIXA (SOIC-8, SOT-23, D_SMA, EC11).
A netlist do KiCad fica **correcta**: cada pino liga a um pad. E a funcao fica
**errada**: o sinal entra pela entrada inversora, a fonte do MOSFET vai ao
catodo do LED, o catodo do Schottky aponta para o lado errado.

**Nenhum ERC, DRC ou "0 por rotear" ve isto**, porque os tres verificam
LIGACAO. Esta verifica SIGNIFICADO — a unica pergunta que nenhum deles faz:
*este pad, que a ficha diz ser a fonte, esta na rede que a fonte tem de ter?*

A varredura de 08/09 encontrou CINCO defeitos desta classe numa placa que
passava em tudo o resto: U1 (as duas entradas trocadas, nos dois canais),
Q1/Q2 (dreno e fonte), D2 (catodo ao contrario), SW1 (nenhum pad ligado) e
EXC/AMP_IN (duas redes onde tinha de ser uma).

⚠️⚠️ E O CONTRATO NAO USA NOMES DE REDE AUTO-GERADOS.

`Net-(U1A--)` muda de nome quando se conserta o defeito, e um contrato escrito
sobre esses nomes passa a acusar o conserto. Por isso a forma normal aqui e
**"estes dois pads tem de estar na MESMA rede"** — que sobrevive a qualquer
renomeacao e diz exactamente o que interessa: quem esta ligado a quem.

Nomes literais so para redes que sao decisao humana e nao mudam: `GND`, `+5V`.
"""

VERSAO = "0.3.1"

import io
import os
import re
import sys

NUM = r"-?\d+(?:\.\d+)?"

# ─────────────────────────────────────────────────────────────────────────────
# A FICHA: que funcao tem cada pad. Por REFERENCIA e nao por footprint — o
# mesmo SOT-23 e o AO3400A (1G 2S 3D) e o BAV99 (1 anodo comum...), e um mapa
# por footprint diria a mesma coisa das duas, que e como se erra.
FICHA = {
    # ⚠️ A PECA COMPRADA E O SI2302CDS-T1-GE3 (Mouser #40440548, 12/09), nao o
    # AO3400A com que o painel foi desenhado. Conferido a 17/09 na ficha Vishay
    # Siliconix doc. 68645 rev. D (docs/datasheets/SI2302CDS-T1-GE3.pdf),
    # "SOT-23 (TO-236) Top View": 1 G · 2 S · 3 D — a mesma pinagem, logo a
    # placa encomendada (06db1c7) serve as duas. Porta a 3,3 V da Daisy contra
    # VGS(th) 0,40..0,85 V; RDS(on) <= 0,075 ohm ja a 2,5 V; VGS max +-8 V.
    ("painel", "Q1"): ("SI2302CDS-T1-GE3", {"1": "G", "2": "S", "3": "D"},
                       "ficha Vishay Si2302CDS doc. 68645, SOT-23 top view"),
    ("painel", "Q2"): ("SI2302CDS-T1-GE3", {"1": "G", "2": "S", "3": "D"},
                       "ficha Vishay Si2302CDS doc. 68645, SOT-23 top view"),
    # ⚠️⚠️ UM CONECTOR TAMBEM TEM FICHA, e a ficha e da peca do OUTRO lado.
    #
    # O J2 e um header de 7 vias: sete pads iguais, sem polaridade, sem
    # nada no cobre que diga o que cada um faz. A funcao dele nao esta na
    # nossa placa — esta no MODULO que la se liga, e um cabo directo (1-1, o
    # que qualquer pessoa compra) faz de cada posicao um contrato.
    #
    # FONTE: pagina do produto Waveshare "2.42inch OLED Display Module,
    # 128x64, SPI/I2C, White", seccao "Control Interface" (tabela + serigrafia
    # do modulo). Ate 08/09 a nossa ordem era `GND +5V CLK DIN CS DC RST`,
    # escrita sem ficha: punha os nossos +5 V no pino DC do modulo com cabo
    # directo, e no GND dele com cabo invertido. ERC, DRC, netlist e as 26
    # vias do IDC ficavam TODOS limpos.
    # ⚠️ o J2 e um header MACHO de 2,54 onde entram os JUMPERS FEMEA soltos do
    # cabo que vem com o modulo (dB, 17/09). O contrato e o NOME de cada fio:
    # o pad da placa tem de levar o sinal que o modulo chama assim. A
    # posicao do pad nao e a posicao no modulo — e a ordem dele lida a
    # partir do VCC (rev C, deddd3d; cicatriz 15ew).
    ("painel", "J2"): ("Waveshare 2.42in OLED SSD1309, cabo MX1.25->jumpers",
                       {"1": "VCC", "2": "GND", "3": "DIN", "4": "CLK",
                        "5": "CS", "6": "DC", "7": "RST"},
                       "ficha Waveshare 2.42in OLED, Control Interface"),
    ("cerebro", "D1"): ("SMBJ24A", {"1": "K", "2": "A"},
                        "D_SMB: pad 1 e o catodo (faixa do silk)"),
    ("cerebro", "D2"): ("SS14", {"1": "K", "2": "A"},
                        "D_SMA: pad 1 e o catodo (faixa do silk)"),
    ("cerebro", "U1"): ("OPA1642AIDR",
                        {"1": "OUT_A", "2": "-IN_A", "3": "+IN_A", "4": "V-",
                         "5": "+IN_B", "6": "-IN_B", "7": "OUT_B", "8": "V+"},
                        "ficha OPA1642, SOIC-8"),
}

# ─────────────────────────────────────────────────────────────────────────────
# O CONTRATO: o que cada funcao tem de ver.
#   ("=", pad, "REDE")        -> aquele pad esta nesta rede, pelo nome
#   ("~", padA, padB)         -> os dois pads na MESMA rede, seja qual for
#   ("!", pad)                -> aquele pad TEM de ter rede (nao pode ficar solto)
CONTRATO = {
 "cerebro": [
    # ⚠️ C1 — a entrada vai a NAO-inversora e a realimentacao a INVERSORA.
    # Escrito por vizinhanca e nao por nome de rede: `Net-(U1A--)` muda quando
    # isto for consertado, e um contrato sobre esse nome acusaria o conserto.
    ("~", "U1.3", "R12.2", "canal A: entrada (R12) na +IN"),
    ("~", "U1.2", "R10.1", "canal A: realimentacao (R10) na -IN"),
    ("~", "U1.5", "R22.2", "canal B: entrada (R22) na +IN"),
    ("~", "U1.6", "R20.1", "canal B: realimentacao (R20) na -IN"),
    # o clamp do BAV99 protege a ENTRADA, logo acompanha a +IN
    ("~", "D11.3", "U1.3", "clamp do canal A na entrada"),
    ("~", "D21.3", "U1.5", "clamp do canal B na entrada"),
    # alimentacao do amp-op
    ("=", "U1.8", "V+PRE", "V+ do OPA1642"),
    ("=", "U1.4", "AGND", "V- do OPA1642 (alimentacao simples)"),
    # ⚠️ C3 — o Schottky bloqueia o retorno do USB da ponte: a corrente entra
    # pelo anodo (+5V) e sai pelo catodo (+5V_PONTE).
    ("=", "D2.1", "+5V_PONTE", "catodo do SS14 vira para a ponte"),
    ("=", "D2.2", "+5V", "anodo do SS14 no trilho"),
    ("=", "D1.1", "+15V", "catodo do TVS no trilho protegido"),
    ("=", "D1.2", "GND", "anodo do TVS na terra"),
    # ⚠️ C5 — uma rede so entre a chave e a entrada do amp
    ("~", "J15.3", "J11.3", "canal L: chave -> entrada do amp"),
    ("~", "J15.6", "J11.3", "canal L: o outro terminal da chave"),
    ("~", "R50.1", "J11.3", "canal L: o 10k de terminacao na mesma rede"),
    ("~", "J16.3", "J11.4", "canal R: chave -> entrada do amp"),
    ("~", "J16.6", "J11.4", "canal R: o outro terminal da chave"),
    ("~", "R51.1", "J11.4", "canal R: o 10k de terminacao na mesma rede"),
 ],
 "painel": [
    # ⚠️ C2 — o MOSFET e interruptor de lado BAIXO: fonte na terra, dreno no
    # catodo do LED. Trocados, o diodo de corpo acende o LED sempre.
    # o contrato do OLED: cada pad contra o NOME do fio do modulo (ver FICHA)
    ("=", "J2.1", "+5V",      "fio VCC do modulo (3,3 ou 5 V)"),
    ("=", "J2.2", "GND",      "fio GND do modulo"),
    ("=", "J2.3", "OLED_DIN", "fio DIN do modulo"),
    ("=", "J2.4", "OLED_CLK", "fio CLK do modulo"),
    ("=", "J2.5", "OLED_CS",  "fio CS do modulo"),
    ("=", "J2.6", "OLED_DC",  "fio DC do modulo"),
    ("=", "J2.7", "OLED_RST", "fio RST do modulo"),
    ("=", "Q1.2", "GND", "fonte do AO3400A na terra"),
    ("=", "Q1.3", "LED_K_A", "dreno do AO3400A no catodo do LED"),
    ("=", "Q1.1", "GATE_A", "porta"),
    ("=", "Q2.2", "GND", "fonte do AO3400A na terra"),
    ("=", "Q2.3", "LED_K_B", "dreno do AO3400A no catodo do LED"),
    ("=", "Q2.1", "GATE_B", "porta"),
    # ⚠️ C4 — o encoder tem cinco pads de sinal e nenhum pode ficar solto
    ("!", "SW1.A", "encoder: canal A"),
    ("!", "SW1.B", "encoder: canal B"),
    ("!", "SW1.C", "encoder: comum"),
    ("!", "SW1.S1", "encoder: botao"),
    ("!", "SW1.S2", "encoder: botao"),
    ("=", "SW1.C", "GND", "comum do encoder na terra"),
 ],
}


def blocos(t):
    """⚠️ Conta parentesis SALTANDO AS CADEIAS (§2d): um `descr` de footprint
    traz `length*width=7.2*11mm^2` e um contador cru dessincroniza no primeiro."""
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


def pads_de(bl):
    """⚠️ Cada pad delimitado pelo PARENTESIS, nunca por um limite de
    caracteres. Uma sonda com `[\\s\\S]{0,400}` deu `J15` com 7 pads (tem 8) e
    o `J2` sem o `RST` — e eu ia acusar o desenho de um defeito que era meu."""
    out, i = {}, 0
    while True:
        j = bl.find('\t\t(pad "', i)
        if j < 0:
            return out
        k, d, ins = j + 2, 0, False
        while k < len(bl):
            c = bl[k]
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
        seg = bl[j:k + 1]
        num = re.search(r'\(pad "([^"]*)"', seg).group(1)
        n = re.search(r'\(net (?:\d+ )?"([^"]*)"', seg)
        # ⚠️ `MP` aparece duas vezes num EC11 (as duas patilhas mecanicas): o
        # dicionario guarda a ultima, e isso e o que se quer — sao a mesma coisa.
        out[num] = n.group(1) if n else None
        i = k + 1


def le(caminho):
    t = io.open(caminho, encoding="utf-8").read()
    placa = {}
    for bl in blocos(t):
        r = re.search(r'\(property "Reference" "([^"]+)"', bl)
        if not r:
            continue
        placa[r.group(1)] = pads_de(bl)
    if not placa:
        raise SystemExit("*** %s: zero footprints lidos — LEITURA INCOMPLETA"
                         % os.path.basename(caminho))
    return placa


def rede(placa, alvo):
    """`R12.2` -> a rede desse pad, ou (None, motivo)."""
    ref, _, pad = alvo.partition(".")
    if ref not in placa:
        return None, "nao ha `%s` na placa" % ref
    if pad not in placa[ref]:
        return None, "`%s` nao tem pad %s (tem: %s)" % (
            ref, pad, " ".join(sorted(placa[ref])[:8]))
    return placa[ref][pad], None


def confere(caminho, nome, contrato, sabota=None):
    placa = le(caminho)
    maus, n = [], 0
    if sabota:
        ref, _, pad = sabota.partition(".")
        if ref in placa and pad in placa[ref]:
            placa[ref][pad] = "REDE_SABOTADA"
    for regra in contrato:
        n += 1
        tipo = regra[0]
        if tipo == "=":
            _, alvo, esperada, porque = regra
            r, err = rede(placa, alvo)
            if err:
                maus.append("%s: %s" % (alvo, err))
            elif r != esperada:
                maus.append("%-9s tem %r e devia ter %r — %s"
                            % (alvo, r, esperada, porque))
        elif tipo == "~":
            _, a, b, porque = regra
            ra, ea = rede(placa, a)
            rb, eb = rede(placa, b)
            if ea or eb:
                maus.append("%s~%s: %s" % (a, b, ea or eb))
            elif ra is None or rb is None:
                maus.append("%-9s ou %-9s sem rede — %s" % (a, b, porque))
            elif ra != rb:
                maus.append("%-9s (%s) e %-9s (%s) em redes DIFERENTES — %s"
                            % (a, ra, b, rb, porque))
        elif tipo == "!":
            _, alvo, porque = regra
            r, err = rede(placa, alvo)
            if err:
                maus.append("%s: %s" % (alvo, err))
            elif r is None:
                maus.append("%-9s SEM REDE — %s" % (alvo, porque))
    print("  %-26s %d regra(s) · %d falha(s)" % (os.path.basename(caminho), n, len(maus)))
    for m in maus:
        print("     ⚠️ %s" % m)
    return len(maus)


def main():
    print("funcao_dos_pads v%s" % VERSAO)
    PLACAS = {"cerebro": "cotuca_cerebro/cotuca_cerebro.kicad_pcb",
              "painel": "cotuca_painel/cotuca_painel.kicad_pcb"}
    aqui = os.path.dirname(os.path.abspath(__file__))
    sabota = "--sabota" in sys.argv
    mau = 0
    for nome, rel in PLACAS.items():
        p = os.path.join(aqui, rel)
        if not os.path.exists(p):
            raise SystemExit("*** nao ha %s" % p)
        if sabota:
            # ⚠️ PROVAR QUE SABE DIZER NAO. Uma guarda que nunca reprovou nada
            # e uma promessa. Sabota-se um pad que ESTA certo e exige-se que a
            # contagem suba — e nao basta que suba: tem de subir por causa
            # DAQUELE pad.
            alvo = "D1.1" if nome == "cerebro" else "Q1.1"
            limpo = confere(p, nome, CONTRATO[nome])
            sujo = confere(p, nome, CONTRATO[nome], sabota=alvo)
            ok = sujo > limpo
            print("     sabotagem de %-6s %d -> %d falhas  %s"
                  % (alvo, limpo, sujo, "✓ sabe dizer nao" if ok else "*** NAO REPROVOU"))
            mau += 0 if ok else 1
        else:
            mau += confere(p, nome, CONTRATO[nome])
    if mau:
        raise SystemExit("*** %d verificacao(oes) de FUNCAO falharam. O ERC e o "
                         "DRC nao veem isto: eles verificam ligacao, isto "
                         "verifica significado." % mau)
    return 0


if __name__ == "__main__":
    sys.exit(main())
