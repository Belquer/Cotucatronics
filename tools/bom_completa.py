#!/usr/bin/env python3
"""bom_completa v0.10.2 — a BOM do instrumento inteiro, gerada dos esquemas.

POR QUE E GERADA, e nao escrita a mao.

A `BOM-MPN-revB.md` foi escrita a mao e a 2026-09-05 divergia do esquema em quatro
sitios ao mesmo tempo: nomes de referencia anteriores a uma renomeacao,
contagens (`C6A`/`C6B` onde ha um `C6` so), quatro `PWR_FLAG` onde havia dois,
e seis pecas desenhadas como pecas que ja tinham sido decididas como fio. Uma
lista a mao ao lado de coisa gerada envelhece sempre.

Esta le as folhas. O que nao se pode ler das folhas — MPN, preco, fornecedor,
e o que vive FORA das placas — vive na tabela `PECAS` aqui em baixo, com a
proveniencia de cada preco declarada: `medido` (fui a fonte na data) ou
`estimado` (ordem de grandeza, nao serve para fechar pedido).

USO:  bom_completa.py            escreve docs/COTUCA_BOM.md
      bom_completa.py --conferir so verifica, nao escreve (sai != 0 se divergir)
"""
import io
import os
import re
import subprocess
import sys
from datetime import date

VERSAO = "0.10.2"
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SAIDA = os.path.join(RAIZ, "docs", "COTUCA_BOM.md")

FOLHAS = [
    ("cerebro", "kicad/cotuca_cerebro/cotuca_energia.kicad_sch"),
    ("cerebro", "kicad/cotuca_cerebro/cotuca_preamp.kicad_sch"),
    ("cerebro", "kicad/cotuca_cerebro/cotuca_soquete.kicad_sch"),
    ("cerebro", "kicad/cotuca_cerebro/cotuca_conector.kicad_sch"),
    ("painel",  "kicad/cotuca_painel/cotuca_painel.kicad_sch"),
]

# ref -> (MPN, fornecedor/codigo, preco_unit_USD, proveniencia)
# ⚠️ `medido` = fui a fonte a 2026-09-05 e li o numero. `estimado` = ordem de
#    grandeza; NAO serve para fechar pedido.
PECAS = {
    "U1":  ("OPA1642AIDR (TI)", "LCSC C67640", 1.0145, "medido"),
    "U2":  ("Daisy **Seed3** (Electrosmith)", "Electrosmith", 29.99, "medido"),
    # ⚠️ ZERO DE PROPOSITO, e nao por falta de decisao. O modulo esta contado na
    # linha de `modulo` em FORA; conta-lo aqui outra vez inflava o total em
    # US$4. Um zero sem explicacao e indistinguivel de um esquecimento, e por
    # isso a proveniencia di-lo em vez de ficar "—".
    "U3":  ("ESP32-C3 SuperMini — **contado em FORA**", "AliExpress", 0.0,
            "ja contado"),
    "D11": ("BAV99,215 (Nexperia)", "LCSC", 0.02, "estimado"),
    "D21": ("BAV99,215 (Nexperia)", "LCSC", 0.02, "estimado"),
    "D1":  ("SMBJ24A (Littelfuse)", "LCSC", 0.079, "medido"),
    "D2":  ("SS14 (onsemi, SMA/DO-214AC)", "Mouser 512-SS14", 0.02, "estimado"),
    # ⚠️ MF-LSMF300/24X-2 (Bourns MF-LSMF): hold 3 A / trip 5,2 A / 24 V, land
    # 2920/7450 metrico = mesma pegada 2920 do footprint Fuse_2920_7451Metric.
    # VERIFICADO em estoque no Mouser (via navegador, 2026-09-12) — 652-MF-LSMF300/24X-2.
    # O Littelfuse 2920L.../24MR (3,3 A) fica de backup em
    # export/MOUSER_ALTERNATES.md. A §7f continua a valer: o sufixo de um
    # polyfuse diz a corrente de hold, e um numero plausivel e inexistente parece
    # certo ate a encomenda voltar vazia.
    "F1":  ("MF-LSMF300/24X-2 (Bourns)", "Mouser 652-MF-LSMF300/24X-2", 0.18, "estimado"),
    "C1":  ("EEU-FR1V222 (Panasonic FR)", "LCSC C407956", 0.7162, "medido"),
    "C3":  ("EEU-FR1C101 (Panasonic FR)", "LCSC", 0.15, "estimado"),
    "C5":  ("EEU-FR1E470 (Panasonic FR)", "LCSC", 0.15, "estimado"),
    "C6":  ("EEU-FR1E471 (Panasonic FR)", "LCSC", 0.20, "estimado"),
    # ⚠️ CL31B106KAHNNWE (Samsung, 10 uF 25 V X7R 1206): VERIFICADO em estoque no
    # Mouser (via navegador, 2026-09-12) — 187-CL31B106KAHNNWE. Substitui o
    # CL31A106KBHNNNE (50 V X5R), que dava MATCH mas estava backordered/long-lead
    # — e backordered ESCAPA do filtro "no match", por isso trocou-se por um com
    # estoque "Ships Now". 25 V cobre o no. Alternativas em MOUSER_ALTERNATES.md.
    "C8":  ("CL31B106KAHNNWE (Samsung, X7R)", "Mouser 187-CL31B106KAHNNWE", 0.05, "estimado"),
    "C13": ("UES1H220MEM (Nichicon UES)", "LCSC", 0.30, "estimado"),
    "C23": ("UES1H220MEM (Nichicon UES)", "LCSC", 0.30, "estimado"),
    "C14": ("MKS2C042201M00KN00 (WIMA MKS2)", "Mouser 505-MKS2C042201M00KN", 0.70, "estimado"),
    "C24": ("MKS2C042201M00KN00 (WIMA MKS2)", "Mouser 505-MKS2C042201M00KN", 0.70, "estimado"),
    "C12": ("C1206C223J5GACTU (Kemet C0G)", "LCSC", 0.0165, "medido"),
    "C22": ("C1206C223J5GACTU (Kemet C0G)", "LCSC", 0.0165, "medido"),
    "RV1": ("3296W-1-104LF (Bourns)", "LCSC C48084", 0.4458, "medido"),
    "RV2": ("3296W-1-104LF (Bourns)", "LCSC C48084", 0.4458, "medido"),
    "J14": ("61202621621 (Wurth WR-BHD, box header 2x13 blindado)", "Mouser 710-61202621621", 0.45, "estimado"),
}
PECAS_PAINEL = {
    # ⚠️ SI2302CDS-T1-GE3 (Vishay, N-ch SOT-23 logic-level 20 V): VERIFICADO em
    # estoque no Mouser (via navegador, 2026-09-12) — 781-SI2302CDS-T1-GE3. 20 V chega:
    # Q1/Q2 sao drivers low-side de LED, Vds <= 5 V. O Mouser NAO cataloga a AOS,
    # e por isso o AO3400A dava "No Match" — fica so como VALOR de esquema e
    # ancora de equivalencia (mesmo SOT-23, 1 G 2 S 3 D). Alt em MOUSER_ALTERNATES.
    "Q1":  ("SI2302CDS-T1-GE3 (Vishay)", "Mouser 781-SI2302CDS-T1-GE3", 0.05, "estimado"),
    "Q2":  ("SI2302CDS-T1-GE3 (Vishay)", "Mouser 781-SI2302CDS-T1-GE3", 0.05, "estimado"),
    # ⚠️ ALPS, e nao o PEC11R da Bourns que a lista a mao pedia. O PEC11R tem
    # os pinos a 5,0 mm e o footprint do KiCad e do Alps, a 2,5 — a peca certa
    # no desenho errado nao entra na placa. O encapsulamento e parte da compra
    # (§7c), e nao um detalhe de layout.
    "SW1": ("EC11E15244G1 (Alps Alpine)", "LCSC C370970", 1.70, "medido"),
    # ⚠️ SUFIXO **S**, e nao o F. `S` = eixo estriado, `F` = eixo liso. A lista
    # a mao pedia o `...4020F-B103` e a nota ao lado do mesmo documento pedia
    # estriado: o MPN contradizia a sua propria nota. O que falha num MPN e
    # quase sempre o sufixo, e nao a familia.
    "RV1": ("PTV09A-1020S-B103 (Bourns)", "distribuidor", 0.60, "estimado"),
    "RV2": ("PTV09A-1020S-B103 (Bourns)", "distribuidor", 0.60, "estimado"),
    "J1":  ("61202621621 (Wurth WR-BHD, box header 2x13 blindado)", "Mouser 710-61202621621", 0.45, "estimado"),
    # J2 = header MACHO 1x07 a 2,54, cortado da barra Wurth 1x40: o OLED vem
    # com cabo MX1.25 -> jumpers femea (dB, 2026-09-17). Era PicoBlade 53047-0710 ate
    # a rev C; nenhuma placa fabricada o usa.
    "J2":  ("61304011121 (Wurth WR-PHD 1x40, cortar 1x07)", "Mouser 710-61304011121", 0.30, "estimado"),
    # ⚠️ o C3 do PAINEL e outro C3 que o do cerebro — tabelas separadas, e por
    # isso este ficava "por decidir" enquanto o do cerebro ja tinha peca. Nao
    # estava por decidir: o dB decidiu-o quando pediu o par 100u+100n junto aos
    # MOSFET ("a razao certa nao e o OLED, sao os tres LEDs de arcade").
    "C3":  ("EEU-FR1C101 (Panasonic FR)", "LCSC", 0.15, "estimado"),
}
GENERICO = {   # por familia de footprint, quando nao ha MPN proprio
    "Resistor_SMD:R_0805_2012Metric":  ("resistor 0805 1%", 0.005, "estimado"),
    "Resistor_SMD:R_1206_3216Metric":  ("resistor 1206 1/4 W", 0.010, "estimado"),
    "Resistor_SMD:R_1210_3225Metric":  ("resistor 1210 1/2 W", 0.020, "estimado"),
    "Capacitor_SMD:C_0805_2012Metric": ("condensador 0805 — ver o valor p/ dielectrico", 0.005, "estimado"),
    "Capacitor_SMD:C_1206_3216Metric": ("condensador 1206 — ver o valor p/ dielectrico", 0.020, "estimado"),
    "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical": ("header 1x2 2,54", 0.04, "estimado"),
    "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical": ("header 1x3 2,54", 0.05, "estimado"),
    "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical": ("header 1x4 2,54", 0.06, "estimado"),
    "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical": ("header 1x5 2,54", 0.07, "estimado"),
    "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical": ("header 1x6 2,54", 0.08, "estimado"),
    # ⚠️ 1x8 e nao 1x6 no J15/J16 desde 2026-09-08: os pinos 7 e 8 sao GND, para o
    # cabo ate ao DPDT ter referencia. Ver a folha do soquete.
    # ⚠️ E O CABO PASSA A OITO CONDUTORES. Isso vive na seccao FORA, porque nao
    # esta em folha nenhuma — uma alteracao de placa que muda cablagem e duas
    # linhas de BOM, nao uma.
    "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical": ("header 1x8 2,54", 0.10, "estimado"),
    "Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical": ("JST XH 1x2 (B2B-XH-A)", 0.05, "estimado"),
    "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal":
        # ⚠️ O NUMERO E 1715721, E NAO O 1935161 DA LISTA ANTIGA. Esta e a
        # familia de erro que a §7f desta casa descreve: o que falha num MPN e
        # quase sempre a variante, nao a marca. O 1935161 vinha rotulado como
        # "MKDS 1/2-5,08" e o footprint que a placa desenha e o **MKDS 1,5**/2-5,08
        # — series diferentes, corpos diferentes. Copiar o numero da lista
        # antiga podia trazer uma peca que nao entra nos furos.
        #
        # O footprint declara-se a si proprio: "Terminal Block Phoenix
        # MKDS-1,5-2-5.08, 2 pins, pitch 5.08mm, size 10.2x9.8mm, drill 1.3mm".
        # O 1715721 e essa peca: 2 vias, 5,08 mm, 17,5 A / 400 V, 1,5 mm2,
        # ligacao por parafuso, THT de onda.
        #
        # ⚠️ SERIE, e nao bancada. O instrumento replica-se as centenas, e por
        # isso o preco que conta e o do escalao de 100 e nao o de um. Medido a
        # 2026-09-08 na Rapid: GBP 0,904 a 100+ com IVA (GBP 1,13 a um). O KF301-5.0
        # generico serve para montar na bancada e NAO e a peca da serie.
        ("terminal parafuso 2p 5,08 — **1715721** (Phoenix MKDS 1,5/2-5,08); "
         "KF301-5.0 so para bancada", 0.95, "medido"),
    "Module:Electrosmith_Daisy_Seed": ("(soquete: 2 tiras femea 1x20 2,54)", 2.00, "estimado"),
}

# ⚠️⚠️ O MPN E UM CAMPO PROPRIO, E NAO A DESCRICAO.
#
# A coluna de cima mistura numero de peca com prosa — "SS14 (onsemi, SMA/DO-
# 214AC)", "61202621621 (Wurth WR-BHD, box header 2x13 blindado)", "terminal
# parafuso 2p 5,08 — 1715721 (Phoenix ...); KF301-5.0 so para bancada". Serve
# para LER numa
# tabela; nao serve para uma maquina procurar a peca.
#
# Carimbei essa coluna no campo `MPN` das folhas e a revisao de fora disse
# "50/50 parts have MPNs" — porque so verifica que o campo NAO ESTA VAZIO. Os
# 99% eram descricoes a passar por numeros de peca, e so se viu quando uma
# ferramenta tentou USAR o campo: o descarregador de fichas foi procurar
# "header 1x4 2,54" na LCSC.
#
# ⚠️ E O VAZIO E UMA RESPOSTA. Um resistor 0805 de 1% nao tem MPN: tem valor e
# encapsulamento, e escolhe-se na altura. Inventar um numero para ele encher a
# coluna e pior que a coluna vazia — a §7f desta casa: um MPN plausivel e errado
# custa uma encomenda inteira e parece certo ate a caixa chegar.
MPN = {
    "U1":  "OPA1642AIDR",      "D11": "BAV99,215",   "D21": "BAV99,215",
    "D1":  "SMBJ24A",          "D2":  "SS14",        "F1":  "MF-LSMF300/24X-2",
    "C1":  "EEU-FR1V222",      "C3":  "EEU-FR1C101", "C5":  "EEU-FR1E470",
    "C6":  "EEU-FR1E471",      "C8":  "CL31B106KAHNNWE",
    "C13": "UES1H220MEM",      "C23": "UES1H220MEM",
    "C14": "MKS2C042201M00KN00", "C24": "MKS2C042201M00KN00",
    "C12": "C1206C223J5GACTU", "C22": "C1206C223J5GACTU",
    "RV1": "3296W-1-104LF",    "RV2": "3296W-1-104LF",
    "J14": "61202621621",
    # ⚠️ vazios de proposito, e cada um com a razao:
    # U2/U3 sao MODULOS comprados montados — o numero e do vendedor, nao do
    # fabricante, e muda com o lote. Vivem na seccao FORA, com nome e loja.
}
MPN_PAINEL = {
    "Q1":  "SI2302CDS-T1-GE3", "Q2":  "SI2302CDS-T1-GE3",
    "SW1": "EC11E15244G1",
    # ⚠️ `-1` E NAO `-4`: o `-4` e REAR MOUNT, sem bucim, eixo deitado.
    # O footprint e o `PTV09A-1` (vertical, com bucim) e o eixo tem de subir
    # pela tampa. Comprado com `-4`, a peca chega, solda, e o botao nao existe
    # — porque o eixo aponta para dentro da caixa.
    # ⚠️ E o sufixo `S` (eixo estriado) ja tinha sido corrigido uma vez, de `F`
    # (liso). Duas correccoes no mesmo MPN, em sitios diferentes do sufixo, e a
    # §7f a repetir-se: familia certa, peca errada, e nada acusa.
    "RV1": "PTV09A-1020S-B103", "RV2": "PTV09A-1020S-B103",
    "J1":  "61202621621",      "J2":  "61304011121",
    "C3":  "EEU-FR1C101",
    **{f"J{i}": "B2B-XH-A" for i in (3, 4, 5, 6, 7)},
}
# ⚠️ §7f: TODO SUFIXO ENTRA COM A LINHA DA FICHA QUE O JUSTIFICA.
#
# Nao e cerimonia. A ficha do BAV99 tem um `215` no meio — e sao 215 mA de
# corrente de diodo duplo carregado, NAO o codigo de embalagem `,215`. Quem
# conferisse o sufixo por busca de texto dava-o por provado com um numero que
# fala de outra coisa. A confirmacao errada e pior que nenhuma: fecha a duvida.
#
# Formato: MPN -> (o que a ficha PROVA, o que ela NAO prova).
SUFIXO = {
    "3296W-1-104LF": (
        "W = pacote de ajuste lateral (tabela de dimensoes, 'W series'); "
        "104 = 100 k (tabela de codigos: 104 -> 100000)",
        # ⚠️ eu ia escrever 'LF = lead-free'. A legenda da propria ficha diz o
        # contrario: 'Blank: Lead-free product compliant with RoHS' e
        # 'LF: High-end product'. O vazio ja e sem chumbo; o LF e outra coisa.
        "LF NAO e sem-chumbo — a legenda diz 'LF: High-end product'"),
    "BAV99,215": (
        "BAV99 = diodo duplo SOT23 (tabela 3, Ordering information)",
        # ⚠️ e este nao se resolve descarregando outra vez: a ficha diz, com
        # todas as letras, 'Packing information removed.'
        ",215 e codigo de embalagem e NAO esta nesta ficha ('Packing "
        "information removed.') — pedir o documento de embalagem da Nexperia"),
    "OPA1642AIDR": ("A = grau; ID = SOIC-8; R = fita e carretel — os tres na "
                    "tabela de encomenda no fim da ficha", ""),
}

# por FOOTPRINT, quando a peca e a mesma em qualquer referencia
MPN_POR_FP = {
    "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal":
        "1715721",
    "Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical": "B2B-XH-A",
    # ⚠️ TODOS os headers 1 fila 2,54 saem cortados da MESMA barra Wurth WR-PHD
    # 1x40 (61304011121), VERIFICADA em estoque no Mouser (via navegador, 2026-09-12)
    # — 710-61304011121. Aqui o vazio DEIXOU de ser resposta: a barra e uma peca
    # sourced, e o numero e o mesmo de 1x2 a 1x8 — o J2 do painel (1x07)
    # incluido, desde a rev C.
    "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical": "61304011121",
    "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical": "61304011121",
    "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical": "61304011121",
    "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical": "61304011121",
    "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical": "61304011121",
    "Connector_PinHeader_2.54mm:PinHeader_1x07_P2.54mm_Vertical": "61304011121",
    "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical": "61304011121",
}

# O que NAO esta em folha nenhuma: modulos, peca de painel, cablagem, caixa.
FORA = [
    ("modulo",   "ESP32-C3 SuperMini (ponte BLE)", "AliExpress", 1, 4.00, "estimado",
     "encaixa no soquete U3 do cerebro (5V pelo SS14, GND, TX, RX, status); o modulo cru ESP32-C3-MINI-1-H4 na LCSC e US$2,17"),
    ("modulo",   "Amp classe D TPA3116D2 estereo", "AliExpress/Amazon", 1, 7.00, "estimado",
     "ja comprado (Clyxgs); terminais de parafuso, fora da placa"),
    ("modulo",   "Step-down 5 V fixo 1,5 A", "AliExpress", 1, 1.50, "estimado",
     "ja comprado (Euogeudel, 5 un.); SAIDA FIXA, por decisao"),
    ("modulo",   "Decoy PD 15 V / 5 A", "AliExpress", 1, 3.00, "estimado",
     "ja comprado (AITRIP); receptaculo USB-C na face de tras"),
    ("painel",   "OLED Waveshare 2,42\" SSD1309 SPI", "Waveshare", 1, 18.00, "estimado",
     "ja comprado; vem com cabo MX1.25 -> jumpers femea, que entram no header J2"),
    ("painel",   "Botao arcade 30 mm iluminado AZUL 5 V", "EG Starts", 2, 2.50, "estimado",
     "ja comprados. ⚠️ A COR E SPEC: a 3,3 V o amarelo nao acende"),
    ("painel",   "Botao arcade 30 mm BRANCO (buzios)", "EG Starts", 1, 2.50, "estimado",
     "sem LED"),
    ("painel",   "Jack TRS 1/4\" NMJ6HCD2 (Neutrik)", "Stompboxparts", 4, 1.75, "medido",
     "isolado do chassis; 2 entradas de piezo + 2 saidas de linha"),
    ("painel",   "DPDT ON-ON-ON tipo 2", "ja comprados", 2, 2.00, "estimado",
     "⚠️ MEDIR O TIPO COM MULTIMETRO ANTES DE FIAR"),
    ("cablagem", "Cabo IDC 2x13, 26 vias, ~300 mm, femea-femea com chave, directo (pino 1 a pino 1)", "—", 1, 3.00, "estimado",
     "percurso medido a 2026-09-18: ~170 mm pela parede de tras (J1 do painel a 98 mm da frente, J14 do cerebro a 19) + 80 mm de folga para tirar o fundo"),
    # ⚠️ ESTE CABO NAO ESTAVA NA BOM. Uma peca que nao esta em folha nenhuma e
    # que ninguem escreveu aqui simplesmente NAO EXISTE na lista de compras, e o
    # total nem a conta como zero — nao a conta de todo. Apareceu por eu mexer
    # no conector; sem isso continuava por descobrir.
    # ⚠️ OS SOQUETES NAO ESTAVAM ORCADOS EM LADO NENHUM. O U2 custa US$29,99 e
    # o U3 tem linha propria, mas as tiras femea onde eles encaixam nao
    # apareciam: a entrada generica que as tinha nunca era alcancada, porque o
    # U2 esta na tabela especifica e essa ganha. Uma peca que so existe numa
    # ramificacao que nunca corre nao esta na lista de compras.
    ("soquete",  "Tira femea 1x20 2,54 dourada (soquete da Daisy)", "Harwin/LCSC",
     2, 1.00, "estimado",
     "M20-7822046 (Harwin) ou equivalente — a substituibilidade vale os US$2"),
    ("soquete",  "Tira femea 1x8 2,54 (soquete do ESP32-C3)", "generico",
     2, 0.15, "estimado",
     "o modulo entra em soquete para poder ser regravado e trocado"),
    ("cablagem", "Cabo 8 vias ~150 mm ao DPDT, crimpado (J15 e J16)", "—", 2, 1.00,
     "estimado",
     "8 e nao 6 desde 2026-09-08: os pinos 7 e 8 sao GND, referencia do cabo. Os dois "
     "condutores de terra nao vao a lug nenhum da chave — ou ao corpo dela, se "
     "for metalica, ou ficam como dreno junto ao par"),
    ("cablagem", "Cabo USB-C curto 15 cm C-C com dados (PD)", "—", 1, 3.00, "estimado",
     "do powerbank ao decoy"),
    ("caixa",    "Caixa (ver docs/desenhos/caixa/)", "—", 1, 0.0, "por orcar",
     # ⚠️ dizia "paredes 6 mm, tampa 9 mm" — premissa revogada (§10b). A caixa
     # que esta a ser cortada e a do `caixa/cotuca_caixa_corte.py`: acrilico de
     # 3,0 mm, rev 5 de 2026-09-06. Comentario com autoridade que afirma premissa
     # morta re-ensina o erro a quem o le.
     "acrilico 3,0 mm, 200x120, frente 55 / tras 80, rev 5; furos na tampa "
     "para encoder e 2 pots"),
]


def carimbo():
    def git(*a):
        try:
            return subprocess.run(["git"] + list(a), cwd=RAIZ,
                                  capture_output=True, text=True).stdout.strip()
        except Exception:
            return "?"
    sujo = bool(git("status", "--porcelain"))
    return (date.today().isoformat(), git("rev-parse", "--short", "HEAD") or "?", sujo)


def componentes():
    """[(placa, ref, valor, footprint)] lidos das folhas. So o que existe."""
    out = []
    for placa, rel in FOLHAS:
        p = os.path.join(RAIZ, rel)
        t = io.open(p, encoding="utf-8").read()
        i = t.find("(lib_symbols")
        d = 0
        for k in range(i, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    t = t[:i] + t[k + 1:]
                    break
        vistos = set()
        n = 0
        for m in re.finditer(r"\(symbol\n\s*\(lib_id ", t):
            j = m.start()
            dd = 0
            for k in range(j, len(t)):
                if t[k] == "(":
                    dd += 1
                elif t[k] == ")":
                    dd -= 1
                    if dd == 0:
                        bl = t[j:k + 1]
                        break
            r = re.search(r'\(property "Reference" "([^"]+)"', bl)
            v = re.search(r'\(property "Value" "([^"]*)"', bl)
            f = re.search(r'\(property "Footprint" "([^"]*)"', bl)
            dnp = re.search(r"\(dnp (\w+)\)", bl)
            bom = re.search(r"\(in_bom (\w+)\)", bl)
            if not r or r.group(1).startswith("#") or bom and bom.group(1) == "no":
                continue
            if r.group(1) in vistos:      # multi-unidade: uma linha por peca
                continue
            vistos.add(r.group(1))
            n += 1
            out.append((placa, r.group(1), v.group(1) if v else "",
                        f.group(1) if f else "",
                        dnp and dnp.group(1) == "yes"))
        if n == 0:
            raise SystemExit("*** %s: zero componentes lidos — leitura vazia nao "
                             "e BOM, e silencio" % rel)
    return out


def linha_peca(placa, ref, val, fp, dnp):
    tab = PECAS_PAINEL if placa == "painel" else PECAS
    if ref in tab:
        mpn, forn, preco, prov = tab[ref]
    elif fp in GENERICO:
        mpn, preco, prov = GENERICO[fp]
        forn = "—"
    else:
        mpn, forn, preco, prov = "**por decidir**", "—", 0.0, "—"
    return mpn, forn, preco, prov


def main():
    hoje, commit, sujo = carimbo()
    cs = componentes()
    smd = lambda fp: any(k in fp for k in ("_SMD", "Package_SO", "Package_TO_SOT",
                                           "Diode_SMD", "Fuse:"))
    blocos = {}
    total = 0.0
    sem_preco = []
    for placa, ref, val, fp, dnp in cs:
        mpn, forn, preco, prov = linha_peca(placa, ref, val, fp, dnp)
        lado = "SMD (JLC monta)" if smd(fp) else "THT (o dB solda)"
        blocos.setdefault((placa, lado), []).append(
            (ref, val, mpn, forn, fp.split(":")[-1] or "—", preco, prov, dnp))
        if not dnp:
            total += preco
            if prov == "—":
                sem_preco.append(ref)

    L = []
    aviso_sem_preco = (
        "⚠️ **%d peca(s) entram no total a ZERO** por nao terem preco decidido: "
        "%s. A soma ACIMA e um PISO, e nao um total — uma peca sem preco "
        "custa zero na aritmetica e nao custa zero na encomenda."
        % (len(sem_preco), ", ".join("`%s`" % r for r in sorted(sem_preco)))
    ) if sem_preco else "Todas as pecas do total tem preco declarado."
    L.append("# COTUCA_BOM — o instrumento inteiro\n")
    L.append("### gerado por `tools/bom_completa.py` v%s · **%s** · commit `%s`%s\n"
             % (VERSAO, hoje, commit,
                "  ⚠️ **ARVORE SUJA** — ha alteracoes por commitar" if sujo else ""))
    L.append("""
> **Esta lista e GERADA das folhas KiCad.** Referencias, valores, footprints e
> marcas de DNP saem do esquema; nao se editam aqui. O que o esquema nao sabe —
> MPN, preco, fornecedor, e o que vive FORA das placas — esta na tabela do
> gerador, e **cada preco declara a proveniencia**:
>
> - **medido** — fui a fonte na data do carimbo e li o numero.
> - **estimado** — ordem de grandeza. **Nao serve para fechar pedido.**
>
> ⚠️ **Stock da LCSC nao e stock de montagem da JLC.** Armazens diferentes. O
> tier Basic/Extended confirma-se na biblioteca de pecas da JLC.
""")

    ordem = [("cerebro", "SMD (JLC monta)"), ("cerebro", "THT (o dB solda)"),
             ("painel", "SMD (JLC monta)"), ("painel", "THT (o dB solda)")]
    n = 1
    for chave in ordem:
        if chave not in blocos:
            continue
        placa, lado = chave
        L.append("\n## %d · %s — %s\n" % (n, placa.upper(), lado))
        n += 1
        L.append("| ref | valor | peca / MPN | fornecedor | footprint | US$ | fonte |")
        L.append("|---|---|---|---|---|---|---|")
        for ref, val, mpn, forn, fp, preco, prov, dnp in sorted(
                blocos[chave], key=lambda r: (r[0][0], len(r[0]), r[0])):
            marca = " **(DNP)**" if dnp else ""
            L.append("| `%s`%s | %s | %s | %s | `%s` | %s | %s |"
                     % (ref, marca, val, mpn, forn, fp,
                        "—" if dnp else "%.4f" % preco, prov))

    L.append("\n## %d · Fora das placas — modulos, painel, cablagem, caixa\n" % n)
    n += 1
    L.append("| tipo | peca | fornecedor | qtd | US$/un | fonte | nota |")
    L.append("|---|---|---|---|---|---|---|")
    for tipo, nome, forn, qtd, preco, prov, nota in FORA:
        total += preco * qtd
        L.append("| %s | %s | %s | %d | %s | %s | %s |"
                 % (tipo, nome, forn, qtd,
                    "%.2f" % preco if preco else "—", prov, nota))

    L.append("\n## %d · Ordem de grandeza\n" % n)
    n += 1
    L.append("**≈ US$ %.2f por instrumento**, so componentes e modulos.\n" % total)
    L.append("""⚠️ **NAO inclui**: PCB, montagem da JLC, taxa de *setup* das pecas
Extended, caixa, parafusos, knobs, nem transporte. E a soma mistura precos
**medidos** com **estimados** — vale como ordem de grandeza, nao como orcamento.

Os DNP nao contam para o total: o footprint fica na placa, a peca nao se compra.

""" + aviso_sem_preco + "\n")
    L.append("""
## %d · Valores que o esquema e o MPN nao dizem igual

| ref | esquema | MPN escolhido | |
|---|---|---|---|
| ~~`C12`, `C22`~~ | ~~`20n C0G`~~ | | **RESOLVIDO 2026-09-05**: o esquema passou a `22n C0G`. 20 nF C0G nao e valor de serie; o esquema mentia ao BOM. |
| `R5`, `R6` (painel) | `100R serie (~30 mA)` | 47 Ω 1/2 W na `BOM-MPN-revB` | por medir na bancada com o azul a 5 V |

Enquanto nao baterem, a BOM esta a comprar uma peca e o desenho a descrever
outra — e nenhum verificador apanha isso.

## %d · O que ainda trava o pedido

0. ~~`+3V3A` sem dono.~~ **RESOLVIDO 2026-09-05** — e era defeito, nao ruido: os
   pots estavam no trilho DIGITAL e lidos contra o ANALOGICO, que e o fundo de
   escala do ADC. Via 9 passou a `+3V3A`, OLED foi para 5 V. Ver ERC-ISENCOES.
1. ~~`OPA1642AIDR` na biblioteca da JLC.~~ **CONFIRMADO 2026-09-05**: JLCPCB
   `C67640`, tier **EXTENDED**, disponivel para montagem SMT. Custa o *setup*
   unico de Extended. O `OPA1656` foi avaliado como alternativa e **recusado**
   — melhor no papel, mas o pre-amp esta medido com o 1642 e 53 MHz num no de
   4M7 e variavel nova. Razoes na `BOM-MPN-revB.md` §11.
2. **`RV1`/`RV2` do painel sem footprint.** O KiCad traz o `PTV09A-1` e o
   `-2`; a peca compravel e o **`-4`**, e nao esta confirmado que partilham o
   desenho de PCB. Le-se o desenho do `-4` na ficha, ou mede-se a peca.
3. **`R5`/`R6` do painel: valor por medir.** Fica `100R` no 1210 como partida;
   fecha-se na bancada com o azul a 5 V, medindo a corrente.
4. ~~A Daisy Seed rev4.~~ **RESOLVIDO a 2026-09-05: passou a Seed3.** A pinagem foi
   conferida pino a pino entre os dois CSV do fabricante — **o nome STM32 bate
   nos 40 pinos, sem uma excepcao** — e a doc diz textualmente *"maintaining
   pin-to-pin compatibility with the original Seed pinout and footprint"*.
   Simbolo, footprint e netlist ficaram **identicos**. O que mudou a favor:
   codec **TI TAC5242**, 32 bit / 192 kHz, chao de ruido de -120 dB (contra o
   AK4556 do rev4) — e este projecto passou a sessao de 2026-09-05 a cacar chao de
   ruido. Nada muda contra: o dB ja grava por **USB-C** desde
   que a primeira placa queimou — a placa de bancada ja e de USB-C. A caixa
   nunca teve recorte para o USB da Daisy e continua a nao ter: grava-se com a
   caixa aberta.
""" % (n, n + 1))
    txt = "\n".join(L) + "\n"
    if "--conferir" in sys.argv:
        antigo = io.open(SAIDA, encoding="utf-8").read() if os.path.exists(SAIDA) else ""
        def sem_carimbo(s):
            return "\n".join(l for l in s.split("\n") if not l.startswith("### gerado por"))
        igual = sem_carimbo(antigo) == sem_carimbo(txt)
        print("COTUCA_BOM %s" % ("em dia" if igual else "DESACTUALIZADA — corre sem --conferir"))
        raise SystemExit(0 if igual else 1)
    io.open(SAIDA, "w", encoding="utf-8").write(txt)
    print("bom_completa v%s -> docs/COTUCA_BOM.md (%d componentes em folha, "
          "%d fora das placas)" % (VERSAO, len(cs), len(FORA)), file=sys.stderr)


if __name__ == "__main__":
    main()
