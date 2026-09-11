#!/usr/bin/env python3
"""para_fabrica — o pacote da JLCPCB, e o portao que se recusa a fazer um mau.

POR QUE EXISTE.

Uma encomenda de placas e irreversivel e demora semanas. O erro nao se descobre
no ecra: descobre-se quando a caixa chega. Por isso este passo nao e "exportar
gerbers" — e um PORTAO que mede a placa e **recusa** produzir o pacote se ela
nao estiver boa, e so depois exporta.

⚠️ A GARANTIA NAO E EU LEMBRAR-ME DE CORRER O DRC. E o pacote nao poder nascer
sem ele. Uma verificacao que depende de disciplina humana nao e uma garantia,
e uma esperanca.

O QUE E MEDIDO, e porque cada um esta na lista:

  geometria        `clearance`, `short`, `tracks_crossing`, cobre fora do
                   contorno, furos coincidentes — a fabrica recusa, ou pior,
                   nao recusa e entrega uma placa em curto.
  ligacoes         uma placa com ligacoes por fazer nao e uma placa incompleta:
                   e uma placa que nao funciona. Conta-se POR REDE, porque o
                   total esconde regressoes (15co).
  planos           ⚠️ NAO se decide por `Zone [X] <-> Zone [X]`: essa medicao
                   e INSTAVEL. Cinco corridas sobre copias identicas do mesmo
                   ficheiro deram 3, 0, 0, 3, 0 — e o perfil por rede deu o
                   mesmo nas cinco. O rotulo varia; os factos nao.
                   Um plano partido a serio faz subir as ligacoes por fazer da
                   sua rede, e e o perfil que decide.
  ilhas orfas      `starved_thermal`: um pad agarrado a uma ilha estreita de
                   cobre. Solda mal e conduz pior.

⚠️ O QUE ESTE PORTAO NAO PODE VER, e por isso nao promete: se a placa faz o que
o esquema queria. Ele prova que o cobre corresponde a netlist, nao que a
netlist esta certa. Isso e a `design-review` e o ouvido do dB.

⚠️ E A ROTACAO DA MONTAGEM E UM RISCO CONHECIDO. O angulo que o KiCad escreve
no CPL e o angulo do footprint; a JLC espera o angulo da peca na biblioteca
DELES, e para muitos encapsulamentos os dois nao batem. Isto NAO se verifica
por script — verifica-se na pre-visualizacao da JLC, peca a peca, antes de
pagar. O ficheiro sai; a promessa nao.

USO:  para_fabrica.py <cerebro|painel|todas> [--incompleta]

`--incompleta` produz o pacote mesmo com ligacoes por fazer, e carimba
INCOMPLETA no nome do ficheiro — para que um pacote mau nunca se possa
confundir com um bom.
"""
import collections
import datetime
import json
import os
import re
import shutil
import subprocess
import sys
import zipfile

VERSAO = "0.17.0"
AQUI = os.path.dirname(os.path.abspath(__file__))
# ⚠️ O caminho macOS e o default, mas honra-se KICAD_CLI do ambiente: um caminho
# hardcoded rebenta noutro computador (skill kicad §8). Igual ao roteia.py, para
# que o portao corra no host de quem tem o KiCad noutro sitio, sem editar a fonte.
KICAD = os.environ.get("KICAD_CLI",
                       "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli")
SAIDA = os.path.join(AQUI, os.pardir, "fabrica")
# ⚠️ fabrica/ e GITIGNORED: e scratch, some com o container e nunca chega ao
# PR. O pacote ENTREGUE e copiado para export/, a pasta de topo RASTREADA — e o
# unico output do portao que sai daqui e chega ao repositorio (ver .gitignore).
EXPORT = os.path.join(AQUI, os.pardir, "export")

# ⚠️ AS CAMADAS SAO EXPLICITAS. A omissao exporta o que o projecto tiver
# guardado, e o que o projecto tem guardado ninguem sabe de cor.
CAMADAS = ("F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,"
           "F.Mask,B.Mask,Edge.Cuts")

GEOMETRIA = ("clearance", "shorting_items", "tracks_crossing", "track_dangling",
             "via_dangling", "copper_edge_clearance", "hole_clearance",
             "holes_co_located", "drill_out_of_range", "annular_width",
             "track_width", "copper_sliver", "malformed_courtyard",
             "invalid_outline", "unresolved_variable")

# ⚠️⚠️ SERIGRAFIA: PARIDADE COM O DFM DA JLC (licao de 2026-09-10, DOIS rounds).
#
# 1o round: a JLCONE marcou o painel com DEZENAS de vermelhos de serigrafia que
# este portao NAO viu: silk->pad 25, silk->furo 31. Poem-se regras `.kicad_dru`
# (`silk_clearance` texto->pad, `physical_hole_clearance` texto->furo,
# `text_thickness`, `text_height`) para o DRC as reportar.
#
# ⚠️ 2o round: NAO CHEGOU. Duas passagens deram "limpo" e a JLCONE continuou com
# 18 vermelhos silk->pad, 11 silk->furo, 50 laranja de largura. Porque:
#   (a) o `kicad-cli pcb drc` NAO disparou as regras `silk_clearance` do .kicad_dru
#       (deram 0 — o DRC continua a sub-contar);
#   (b) o vermelho da JLC nasce do proprio `--subtract-soldermask`, que apara a
#       serigrafia RENTE ao pad (0 folga = borda que a JLC conta como vermelho),
#       e isso so se ve NO GERBER, nao na placa;
#   (c) a largura "50 laranja" era TEXTO E GRAFICO a EXACTAMENTE 0,15 mm — no
#       minimo, que a JLC marca a laranja; o bump de contornos a 0,15 nao a limpou.
# A correccao real e NA FONTE (`serigrafia_dfm.py`: sobe largura a 0,18, recorta
# graficos — incl. discos cheios — a 0,15 de pad/furo, reposiciona texto) e a
# medicao real e NO GERBER (`mede_serigrafia.py`, chamado abaixo depois de exportar).
# As regras `.kicad_dru` ficam como cinto-e-suspensorios; o portao do GERBER e o
# que MANDA agora.
#
# ⚠️ So TEXTO de serigrafia. Um CONTORNO de footprint sobre um pad (caso (b) da
# skill `kicad_layout` §11g-ter) e recortado pela JLC sem estrago — recusar por
# ele travava placas boas. O `silk_over_copper` e portanto FILTRADO a itens onde
# entra um texto; o `hole_clearance` (silk->furo) ja cai na GEOMETRIA acima.
# ⚠️ A largura de um GRAFICO de serigrafia (fp_line) nao tem regra de DRC no
# KiCad; sobe-se na FONTE com `normaliza_silk.py` (Estagio 6), fora deste portao.
SILK_TEXTO = ("text_thickness", "text_height")   # sempre texto, sempre defeito


def _silk_de_texto(v):
    """True se a violacao envolve um TEXTO de serigrafia (Reference/Value/gr_text)."""
    for it in v.get("items", []):
        d = it.get("description", "")
        if d.startswith(("Reference field", "Value field", "PCB text", "Text '")):
            return True
    return False


def mede(pcb):
    """(por tipo, por rede, planos partidos, ilhas orfas, violacoes)."""
    r = subprocess.run([KICAD, "pcb", "drc", "--refill-zones", "--save-board",
                        "--severity-error", "--severity-warning",
                        "--format", "json", "-o", "/tmp/_f.json", pcb],
                       capture_output=True, text=True)
    if not os.path.exists("/tmp/_f.json"):
        raise SystemExit("*** o DRC nao produziu relatorio:\n%s" % r.stderr)
    d = json.load(open("/tmp/_f.json"))
    tipos = collections.Counter(x["type"] for x in d.get("violations", []))
    orfas = tipos.get("starved_thermal", 0)
    por_rede, partidos = collections.Counter(), 0
    for x in d.get("unconnected_items", []):
        ds = [i.get("description", "") for i in x.get("items", [])]
        redes = {m.group(1) for m in
                 (re.search(r"\[([^\]]+)\]", s) for s in ds) if m}
        if len(ds) == 2 and all("Zone [" in s for s in ds) and len(redes) == 1:
            partidos += 1
        for n in redes:
            por_rede[n] += 1
    return tipos, por_rede, partidos, orfas, d.get("violations", [])


def carimbo():
    """⚠️ Data GERADA, e commit, e o aviso se a arvore estiver suja. Um pacote
    que nao diz de que estado do repositorio saiu nao se pode reproduzir, e um
    pacote que nao se reproduz nao se pode corrigir."""
    def g(*a):
        return subprocess.run(a, capture_output=True, text=True,
                              cwd=AQUI).stdout.strip()
    sujo = bool(g("git", "status", "--porcelain"))
    return (datetime.date.today().isoformat(), g("git", "rev-parse", "--short", "HEAD"),
            sujo)


# ⚠️ MEDIDO, nao contado de cabeca. O dB disse "AO3400A x3" e a placa tem DOIS
# (Q1 e Q2) — as cinco ocorrencias no `.kicad_sch` sao valor, propriedade e
# instancia do mesmo par. Sao SETE pecas com polaridade, nao nove, e a lista que
# se leva a pre-visualizacao tem de ser a da placa e nao a da memoria.
# ⚠️ PERGUNTAS QUE O PACOTE LEVA CONSIGO, porque nao se respondem por script.
#
# Um pacote de fabrico que sai sem as suas duvidas escritas obriga quem o
# recebe a lembrar-se delas. Estas seguem no LEIA-ME, ao lado dos ficheiros.
ABERTAS = {
    "painel": [
        "(!) ORDEM DO J2 (OLED) POR CONFIRMAR COM O MODULO EM MAO.\n"
        "    A placa tem: 1 GND · 2 +5V · 3 CLK · 4 DIN · 5 CS · 6 DC · 7 RST\n"
        "    O Waveshare 2,42\" SPI tem, no conector: VCC · GND · DIN · CLK ·\n"
        "    CS · DC · RST.\n"
        "    Com um cabo 1:1, VCC e GND TROCAM — o OLED morre ao ligar — e o\n"
        "    DIN/CLK tambem. Ou se reordena o J2 (o cabo de fabrica passa a\n"
        "    servir), ou se crimpa um cabo cruzado. Confirmar no modulo antes\n"
        "    de decidir; nao ha como um script ver o conector de outra pessoa.",
    ],
    "cerebro": [
        "(!) A PLACA E DE 2 CAMADAS. Encomendar 2, e nao 4: so ha F.Cu e B.Cu,\n"
        "    com GND (prio 0) e AGND (prio 1) ambas em B.Cu e o NT1 como ponte\n"
        "    unica. Pedir 4 paga um plano interior que este desenho nao usa.",
        "(!) OS POTS SAO THT E NAO PASSAM PELA JLC: PTV09A-1020S-B103, do\n"
        "    distribuidor. O `-1` e vertical com bucim; o `-4` e rear mount e o\n"
        "    eixo nao sobe pela tampa.",
    ],
}

POLARIZADAS = {
    "cerebro": [
        ("U1", "OPA1642AIDR", "SOIC-8", "pino 1 — o ponto no canto"),
        ("D11", "BAV99,215", "SOT-23", "a JLC roda SOT-23 180 graus com frequencia"),
        ("D21", "BAV99,215", "SOT-23", "idem"),
        ("D1", "SMBJ24A", "SMB", "catodo — a barra"),
        ("D2", "SS14", "SMA", "catodo. (!) o Value diz `1N5819` e o MPN diz "
                              "`SS14`: e a mesma peca em SMA, mas o desenho e a "
                              "compra chamam-lhe nomes diferentes"),
    ],
    "painel": [
        ("Q1", "AO3400A", "SOT-23", "a JLC roda SOT-23 180 graus com frequencia"),
        ("Q2", "AO3400A", "SOT-23", "idem"),
    ],
}


def cpl_bate_bom(dest, nome):
    """O CPL e a BOM tem de concordar sobre QUEM e montado.

    ⚠️ Sao dois ficheiros escritos por dois exportadores a partir de duas
    fontes: o CPL sai da PLACA, a BOM sai do ESQUEMA. Nada no KiCad os obriga a
    concordar, e cada um obedece a marcas que vivem do seu lado.

    ⚠️ Foi assim que oito pecas DNP — C11 C13 C21 C23 R13 R23 RV1 RV2 — sairam
    no CPL sem linha na BOM: o `(dnp yes)` esta nos simbolos do esquema e o
    `--exclude-dnp` do `pcb export pos` le o atributo do FOOTPRINT, que nao o
    tinha. A maquina recebia ordem para colocar oito pecas que ninguem comprou.

    ⚠️ E ao contrario: os pontos de teste sairam na BOM sem estar no CPL. O
    footprint traz `exclude_from_bom`, mas a BOM vem do esquema, onde os
    simbolos nao tinham a marca — uma linha de compra para uma coisa que e
    cobre e nao se compra.

    Nenhum dos dois da erro em lado nenhum. So a COMPARACAO os ve."""
    import csv as _csv
    def desig(caminho):
        # ⚠️ Le a coluna `Designator` PELO NOME, nao pela posicao. A BOM saiu no
        # esquema canonico da JLC (Comment,Designator,...) e o Designator ja nao
        # e a 1a coluna; um leitor posicional apanharia o Comment e a comparacao
        # partia-se em silencio (a familia 15dw: um leitor partido aprova tudo).
        if not os.path.exists(caminho):
            return None
        with open(caminho, encoding="utf-8-sig", newline="") as f:
            rows = list(_csv.DictReader(f))
        fora = set()
        for r in rows:
            fora |= set(re.findall(r"[A-Z]+\d+", r.get("Designator", "") or ""))
        return fora
    cpl = desig(os.path.join(dest, "cpl_%s.csv" % nome))
    bom = desig(os.path.join(dest, "bom_%s.csv" % nome))
    if cpl is None or bom is None:
        return None
    # ⚠️ guarda de leitura (§2c): zero designadores nao e "concordam", e um
    # leitor partido — e um leitor partido APROVA tudo.
    if not cpl or not bom:
        return ("leitura do CPL/BOM devolveu %d e %d designadores — nada a "
                "jusante e confiavel" % (len(cpl), len(bom)))
    so_cpl, so_bom = sorted(cpl - bom), sorted(bom - cpl)
    mal = []
    if so_cpl:
        mal.append("no CPL e nao na BOM (a maquina coloca, ninguem comprou): %s"
                   % " ".join(so_cpl))
    if so_bom:
        mal.append("na BOM e nao no CPL (comprado, ninguem coloca): %s"
                   % " ".join(so_bom))
    return " · ".join(mal) if mal else None


def mete_lcsc(caminho, placa):
    """Poe a coluna LCSC na BOM exportada. Devolve motivo de recusa, ou None.

    ⚠️⚠️ A CHAVE E (PLACA, REFERENCIA). NUNCA A REFERENCIA SOZINHA.

    A v0.5.0 indexou so pela referencia e as duas placas ATROPELARAM-SE: o
    `C1` do cerebro e um electrolitico THT de 2200 uF e o `C1` do painel e um
    100 n 0805, e a BOM do cerebro saiu com `C49678` — o 100 n — na linha do
    2200 uF. `R5`/`R6` levaram o codigo do 100 R 1210 do painel estando o
    cerebro a pedir 100 k 0805.

    ⚠️ E o portao DEU VERDE. Nada nele comparava a peca escolhida com a peca
    desenhada; so contava se a coluna estava preenchida — a mesma familia da
    15dw, um campo cheio a passar por um campo certo. A montagem teria soldado
    um condensador ceramico de 0805 onde vai um electrolitico de 16 mm.

    Por isso ha AQUI DOIS testes, e o segundo e independente do primeiro:
    a chave inclui a placa, E o encapsulamento gravado tem de aparecer no
    footprint que a BOM declara. Se a chave voltar a partir-se, o
    encapsulamento apanha-o — `CP_Radial_D16.0mm` nao contem `C_0805`.

    ⚠️⚠️ E A LINHA DA BOM E POR PECA, NAO POR COMENTARIO (licao de 2026-09-11).

    O `kicad-cli sch export bom --group-by Value,Footprint` parte uma peca em
    varias linhas quando o Value traz anotacoes humanas: o painel tinha `100n`,
    `100n COLADO ao cursor` e `100n COLADO ao IDC` — TRES linhas, TODAS com o
    mesmo LCSC `C49678` — e o cerebro tinha `10k — L` e `10k — R`, DUAS linhas
    com o mesmo `C17414`. A JLCONE recusa isto com "Multiple lines in the BOM
    have been matched to the same part": para a montagem, uma peca e UMA linha.

    Por isso a BOM de montagem reagrupa PELO LCSC — a identidade real da peca
    montada. Um numero LCSC identifica um so componente fisico; duas linhas com
    o mesmo LCSC sao a mesma peca e fundem-se, com todos os designadores juntos
    e Qty = contagem. O comentario (Value) e anotacao humana e NAO parte uma
    linha de montagem; da linha fundida sai um Value representativo (o mais
    curto). As pecas SEM LCSC (THT/DNP/nao-catalogadas) ficam como estao, uma
    linha por (Value,Footprint) que o KiCad ja deu — a JLC nao as monta.

    Cabecalho canonico da JLC: `Comment,Designator,Footprint,Qty,LCSC Part #`."""
    import csv as _csv
    fonte = os.path.join(os.path.dirname(AQUI), "tools", "codigos_lcsc.json")
    if not os.path.exists(fonte):
        return "nao ha tools/codigos_lcsc.json — corre tools/codigos_lcsc.py"
    with open(fonte, encoding="utf-8") as f:
        d = json.load(f)
    por_ref = {}
    for rot, e in d.get("linhas", {}).items():
        if rot.split("/", 1)[0] != placa:
            continue
        for r in e.get("refs", []):
            por_ref[r] = (e.get("codigo", ""), e.get("encaps", ""))

    with open(caminho, encoding="utf-8-sig", newline="") as f:
        rows = list(_csv.DictReader(f))
    if not rows:
        return "a BOM exportada saiu sem linhas"

    def natkey(ref):
        m = re.match(r"([A-Za-z]+)(\d+)", ref)
        return (m.group(1), int(m.group(2))) if m else (ref, 1 << 30)

    # 1a passagem: cada linha do KiCad -> (comment, footprint, lcsc, designadores).
    regs, sem = [], []
    for r in rows:
        comment = (r.get("Comment") or r.get("Value") or "").strip()
        fp = (r.get("Footprint") or "").strip()
        desig = re.findall(r"[A-Z]+\d+", r.get("Designator") or "")
        # ⚠️ so as linhas SMD precisam de codigo: as THT o dB compra e solda, e
        # exigir codigo a um borne de parafuso travava o pacote por nada.
        cods = {por_ref[x] for x in desig if x in por_ref}
        if len(cods) > 1:
            return ("a linha %s junta designadores com codigos diferentes (%s) "
                    "— a BOM agrupa por valor+footprint, logo isto diz que a "
                    "fonte discorda de si propria"
                    % (desig[:3], sorted(cods)))
        cod = ""
        if cods:
            cod, encaps = cods.pop()
            # ⚠️ SEGUNDA GUARDA, independente da chave: o encapsulamento que a
            # fonte gravou tem de aparecer no footprint que a BOM declara. E o
            # que apanha uma troca entre placas mesmo que a chave se parta.
            if encaps and encaps not in fp:
                return ("%s: a fonte diz encapsulamento %r e a BOM declara %r — "
                        "a peca escolhida NAO e a peca desenhada"
                        % (",".join(desig[:3]), encaps, fp))
            if not cod:
                sem.extend(x for x in desig if x in por_ref)
        regs.append((comment, fp, cod, desig))
    if sem:
        return ("sem codigo LCSC, e a JLC nao monta a linha: %s"
                % " ".join(sorted(set(sem))))

    # 2a passagem: FUNDE por LCSC (uma peca montada = uma linha). As linhas sem
    # LCSC passam intactas (chave unica por si so, nunca se fundem entre elas).
    grupos, ordem = {}, []
    for comment, fp, cod, desig in regs:
        chave = cod if cod else ("__sem__", tuple(desig), fp)
        if chave not in grupos:
            grupos[chave] = {"comments": [], "fps": set(), "desig": set(), "cod": cod}
            ordem.append(chave)
        g = grupos[chave]
        g["comments"].append(comment)
        g["fps"].add(fp)
        g["desig"].update(desig)

    # ⚠️ Comment representativo da linha fundida: o Value mais curto (o de base,
    # sem as anotacoes que separavam as linhas), desempate alfabetico.
    linhas_saida = []
    for chave in ordem:
        g = grupos[chave]
        desig = sorted(g["desig"], key=natkey)
        comment = sorted(g["comments"], key=lambda c: (len(c), c))[0]
        fp = sorted(g["fps"])[0]
        linhas_saida.append((desig, comment, fp, str(len(desig)), g["cod"]))
    linhas_saida.sort(key=lambda t: natkey(t[0][0]))

    with open(caminho, "w", encoding="utf-8", newline="") as f:
        w = _csv.writer(f, quoting=_csv.QUOTE_ALL)
        # ⚠️ o cabecalho e o esquema canonico da JLC; a coluna do codigo e
        # `LCSC Part #` — a chave em que a JLC lê o LCSC no upload de BOM;
        # `LCSC` sozinho nao e reconhecido.
        w.writerow(["Comment", "Designator", "Footprint", "Qty", "LCSC Part #"])
        for desig, comment, fp, qty, cod in linhas_saida:
            w.writerow([comment, ",".join(desig), fp, qty, cod])
    return None


def cpl_para_jlc(caminho):
    """Reescreve o CPL no ESQUEMA que a JLCPCB exige no upload de montagem SMT.

    ⚠️ O `kicad-cli pcb export pos --format csv` emite o formato NATIVO do KiCad
    — cabecalho `Ref,Val,Package,PosX,PosY,Rot,Side` e lado em minusculas
    (`top`/`bottom`). O parser de SMT da JLCONE NAO o reconhece e recusa o
    ficheiro com "Failed processing the CPL file" (medido no upload real do
    painel, 2026-09-11). A JLC exige exactamente `Designator,Mid X,Mid Y,Layer,
    Rotation`, com `Layer` capitalizado (`Top`/`Bottom`). Esta e a mesma licao da
    paridade de DFM: a SAIDA do portao tem de ser o que a fabrica ACEITA, nao o
    que a ferramenta emite por omissao.

    Mapa: Ref->Designator, PosX->Mid X, PosY->Mid Y, Side->Layer, Rot->Rotation.
    Val e Package sao descartados (a JLC le a peca do BOM, nao do CPL).
    Coordenadas em mm, sem sufixo; zeros finais aparados. UTF-8 sem BOM."""
    import csv as _csv
    with open(caminho, encoding="utf-8-sig", newline="") as f:
        rows = list(_csv.DictReader(f))
    lay = {"top": "Top", "bottom": "Bottom"}

    def trim(v):
        v = (v or "").strip()
        if "." in v:
            v = v.rstrip("0").rstrip(".")
        return v or "0"
    out = ["Designator,Mid X,Mid Y,Layer,Rotation"]
    for r in rows:
        ref = (r.get("Ref") or r.get("Designator") or "").strip()
        if not ref:
            continue
        x = trim(r.get("PosX") or r.get("Mid X") or "")
        y = trim(r.get("PosY") or r.get("Mid Y") or "")
        side = (r.get("Side") or r.get("Layer") or "").strip().lower()
        rot = trim(r.get("Rot") or r.get("Rotation") or "")
        out.append("%s,%s,%s,%s,%s" % (ref, x, y, lay.get(side, side.capitalize() or "Top"), rot))
    with open(caminho, "w", encoding="utf-8", newline="") as f:
        f.write("\n".join(out) + "\n")


def uma(nome, incompleta):
    pcb = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_pcb" % nome)
    sch = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_sch" % nome)
    if not os.path.exists(pcb):
        raise SystemExit("*** nao existe %s" % pcb)
    print("\n══ %s ══" % nome)
    tipos, por_rede, partidos, orfas, viols = mede(pcb)

    recusas, avisos = [], []

    # ⚠️ AS FOLHAS TEM DE CONCORDAR COM A FONTE DAS PECAS. O MPN vive no
    # `PECAS` do bom_completa e e carimbado nas folhas pelo `mpn_nas_folhas`.
    # Uma copia sem verificador volta a divergir — foi assim que a
    # `BOM-MPN-revB.md` acabou tres dias a frente do esquema e com tres numeros
    # de peca errados. Aqui a divergencia trava o pacote.
    rmpn = subprocess.run([sys.executable,
                           os.path.join(AQUI, "mpn_nas_folhas.py"), "--conferir"],
                          capture_output=True, text=True)
    if rmpn.returncode:
        recusas.append("as folhas divergem da fonte dos MPN: %s"
                       % (rmpn.stdout or rmpn.stderr).strip().splitlines()[-1])
    else:
        for l in (rmpn.stdout or "").splitlines():
            if "TOTAL:" in l:
                print("   MPN            %s" % l.split("TOTAL:")[1].strip())
    # ⚠️ E AS FICHAS TEM DE FALAR DA PECA. Uma ficha em disco com o nome certo
    # e o conteudo de outra peca e pior que ficha nenhuma: ela FECHA a duvida.
    # O `--confere` nao vai a rede — le o que ja esta em `docs/datasheets/` e
    # so grita no que la esta a mentir. Peca sem ficha nao trava o pacote (a
    # LCSC nao cataloga oito destas, e isso e facto do catalogo, nao defeito).
    rfi = subprocess.run(
        [sys.executable, os.path.join(os.path.dirname(AQUI), "tools", "fichas.py"),
         "--confere"], capture_output=True, text=True)
    for l in (rfi.stdout or "").splitlines():
        if l.strip().startswith(("FAMILIA", "OK", "NO CATALOGO", "SEM FICHA")) \
                or " · " in l and l.startswith("  "):
            print("   fichas         %s" % l.strip())
    if rfi.returncode and "nao falam da peca" in (rfi.stdout + rfi.stderr):
        recusas.append("ha ficha(s) tecnicas que nao falam da peca — ver "
                       "docs/datasheets/README.md")

    # ⚠️⚠️ E CADA REFERENCIA TEM DE APONTAR PARA A PROPRIA PECA.
    #
    # Nenhum DRC responde a isto: uma referencia a 11,5 mm da sua peca e a 5 mm
    # da vizinha nao colide com nada, nao esta sobre cobre e nao sai da placa —
    # e identifica a peca errada. Quem solda a mao le o designador mais proximo.
    #
    # ⚠️ Isto so entra aqui porque um portao e a UNICA forma de nao voltar. A
    # §11g-ter da skill `/kicad_layout` ja tinha previsto o defeito por escrito, e ele esteve la
    # na mesma, em 8 referencias, porque nada o media. Diagnosticado e por
    # instrumentar e o estado em que um defeito vive mais tempo.
    rap = subprocess.run([sys.executable, os.path.join(AQUI, "refs_apontam.py"),
                          pcb], capture_output=True, text=True)
    for l in (rap.stdout or "").splitlines():
        if "pecas ·" in l:
            print("   designadores  %s" % l.split("·", 1)[1].strip())
    if rap.returncode:
        recusas.append("referencia(s) a identificar a peca errada: %s"
                       % " ".join(re.findall(r"⚠️ (\S+)", rap.stdout)))

    # ⚠️⚠️ E O PAD CERTO TEM DE LEVAR A REDE CERTA (funcao_dos_pads).
    #
    # A skill `kicad` §7 recomenda-o como passada do portao, e por um motivo
    # medido: e a UNICA verificacao que apanha os criticos de FUNCAO — simbolo
    # generico (pinos 1,2,3) casado com footprint polarizado (SOIC-8, SOT-23,
    # D_SMA, EC11). A netlist fica LIGADA e a funcao fica ERRADA, e nem ERC, nem
    # DRC, nem "0 por rotear" veem, porque os tres verificam LIGACAO e este
    # verifica SIGNIFICADO. Apanhou 5 dos 6 criticos da varredura de 08/09.
    # (Confere as duas placas de uma vez, pela ficha por referencia.)
    rfp = subprocess.run([sys.executable, os.path.join(AQUI, "funcao_dos_pads.py")],
                         capture_output=True, text=True)
    for l in (rfp.stdout or "").splitlines():
        if "regra(s)" in l:
            print("   funcao pads   %s" % l.strip())
    if rfp.returncode:
        recusas.append("funcao dos pads: %s"
                       % (rfp.stdout or rfp.stderr).strip().splitlines()[-1])

    # ⚠️⚠️ E AS VIAS TEM DE ESTAR TAPADAS (vias_tented).
    #
    # A JLCONE marca a classe "solder mask opening exposing trace" (14 laranja no
    # painel, medido a 2026-09-11). Medida no gerber, a classe e BENIGNA so porque
    # as 34 vias estao tapadas: 0 abertura de mascara sobre via, e a laranja nasce
    # da expansao de casa da JLC no pescoco pad/pista (cosmetica). Destapar uma via
    # e uma escolha legitima que NENHUM DRC recusa — e passa a classe de laranja
    # inerente a VERMELHO real (cobre exposto a oxidar). Esta guarda preserva o
    # invariante que a torna benigna. Ver `vias_tented.py` e fab-automation.md §2a.
    rvt = subprocess.run([sys.executable, os.path.join(AQUI, "vias_tented.py"), pcb],
                         capture_output=True, text=True)
    for l in (rvt.stdout or "").splitlines():
        if "aberta(s)" in l:
            print("   vias tapadas  %s" % l.split("·", 1)[-1].strip())
    if rvt.returncode:
        recusas.append("via(s) destapada(s) — mascara aberta sobre cobre: %s"
                       % (rvt.stdout or rvt.stderr).strip().splitlines()[-1])

    geo = {k: v for k, v in tipos.items() if k in GEOMETRIA}
    if geo:
        recusas.append("geometria: %s" % dict(geo))

    # ⚠️⚠️ SERIGRAFIA (paridade JLC): texto sobre pad, e texto fino/baixo.
    # O silk->furo cai na GEOMETRIA (sai como `hole_clearance`). Aqui trata-se do
    # silk->pad (`silk_over_copper`, FILTRADO a texto) e do texto abaixo do minimo.
    silk_pad = [v for v in viols
                if v.get("type") == "silk_over_copper" and _silk_de_texto(v)]
    silk_txt = {k: v for k, v in tipos.items() if k in SILK_TEXTO}
    if silk_pad:
        recusas.append("serigrafia: %d texto(s) sobre pad (<0,15 mm, JLC recorta "
                       "e o texto some)" % len(silk_pad))
    if silk_txt:
        recusas.append("serigrafia: texto abaixo do minimo JLC %s "
                       "(traco >=0,15 mm / altura >=1 mm)" % dict(silk_txt))
    # ⚠️ O PORTAO NAO RECUSA POR ISTO. Ver o cabecalho: a medicao nao e
    # reproduzivel, e um portao que recusa por ruido ensina a ignora-lo.
    if partidos:
        avisos.append("plano de terra: %d indicacao(oes) de pedacos — "
                      "INSTAVEL, confirma no editor antes de agir" % partidos)
    faltam = sum(por_rede.values()) - partidos
    if faltam and not incompleta:
        recusas.append("%d ligacao(oes) por fazer" % faltam)

    print("   geometria      %s" % ("%d achado(s): %s" % (sum(geo.values()), dict(geo))
                                    if geo else "0 ✓"))
    _silk_n = len(silk_pad) + sum(silk_txt.values())
    print("   serigrafia     %s (JLC: silk->pad/furo/traco/altura via .kicad_dru)"
          % ("%d texto(s) em falta" % _silk_n if _silk_n else "0 ✓"))
    print("   planos         %s" % ("%d indicacao(oes) — medicao instavel, "
                                       "ver o perfil por rede" % partidos
                                       if partidos else "sem indicacao"))
    print("   ilhas orfas    %s" % ("%d (starved_thermal)" % orfas if orfas else "0 ✓"))
    print("   ligacoes       %s" % ("%d por fazer:" % faltam if faltam else "todas feitas ✓"))
    for r, n in sorted(por_rede.items(), key=lambda x: (-x[1], x[0])):
        print("                    %-20s %d" % (r, n))

    for a2 in avisos:
        print("   ⚠️ %s" % a2)
    if recusas:
        print("\n   RECUSO produzir o pacote:")
        for r in recusas:
            print("     · %s" % r)
        if faltam and not incompleta:
            print("   (--incompleta produz na mesma, com INCOMPLETA no nome)")
        return False

    data, commit, sujo = carimbo()
    tag = "INCOMPLETA_" if faltam else ""
    dest = os.path.join(SAIDA, "%s%s_%s_%s" % (tag, nome, data, commit))
    os.makedirs(dest, exist_ok=True)
    for a in os.listdir(dest):
        os.remove(os.path.join(dest, a))

    def corre(*args):
        r = subprocess.run([KICAD] + list(args), capture_output=True, text=True)
        if r.returncode:
            raise SystemExit("*** %s falhou:\n%s" % (args[2], r.stderr or r.stdout))

    corre("pcb", "export", "gerbers", "-o", dest + os.sep, "--layers", CAMADAS,
          "--no-x2", "--subtract-soldermask", "--check-zones", pcb)
    corre("pcb", "export", "drill", "-o", dest + os.sep, "--format", "excellon",
          "--drill-origin", "absolute", "--excellon-units", "mm",
          "--excellon-zeros-format", "decimal", "--generate-map",
          "--map-format", "gerberx2", pcb)

    # ⚠️⚠️ SERIGRAFIA MEDIDA NO GERBER — o portao que faltava (licao de 2026-09-10,
    # 2o round). O DRC do KiCad SUB-CONTA a serigrafia: mediu silk->pad com folga 0
    # e as regras `silk_clearance` do `.kicad_dru` nem dispararam pelo `kicad-cli
    # pcb drc`. Duas passagens deram "limpo" e a JLCONE mostrou dezenas de vermelhos.
    # A verdade esta no GERBER exportado (ja com `--subtract-soldermask`, que apara a
    # serigrafia RENTE ao pad = a borda que a JLC conta como vermelho). `mede_
    # serigrafia.py` mede-o: silk->pad, silk->furo E largura de traco (texto E
    # grafico). ⚠️ Precisa de gerbonara+shapely; se faltarem, RECUSA (um portao que
    # nao consegue medir nao passa em silencio — a licao inteira e essa).
    # ⚠️ `mede_serigrafia` precisa de gerbonara+shapely, que vivem no python de
    # SISTEMA, nao no bundle do KiCad (que corre este portao pelo pcbnew dos outros
    # verificadores). SILK_SYS_PY aponta para esse python; default = o proprio.
    silk_py = os.environ.get("SILK_SYS_PY", sys.executable)
    rsk = subprocess.run([silk_py,
                          os.path.join(AQUI, "mede_serigrafia.py"), dest],
                         capture_output=True, text=True)
    for l in (rsk.stdout or "").splitlines():
        if "silk->pad" in l or "GERBER limpa" in l:
            print("   serigrafia(gbr)%s" % l.rstrip()[3:] if l.startswith("   ") else "   serigrafia(gbr) %s" % l.strip())
    if rsk.returncode:
        print("\n   RECUSO produzir o pacote:")
        print("     · serigrafia no GERBER: %s"
              % ((rsk.stdout or rsk.stderr).strip().splitlines() or ["ver mede_serigrafia"])[-1])
        if rsk.returncode == 3:
            print("     · (instala: pip install gerbonara shapely)")
        return False

    corre("pcb", "export", "pos", "-o", os.path.join(dest, "cpl_%s.csv" % nome),
          "--format", "csv", "--units", "mm", "--side", "both", "--exclude-dnp", pcb)
    # ⚠️ o pos nativo do KiCad NAO e uploadavel na JLCONE — reescreve para o
    # esquema Designator,Mid X,Mid Y,Layer,Rotation + Top/Bottom (ver cpl_para_jlc).
    cpl_para_jlc(os.path.join(dest, "cpl_%s.csv" % nome))
    if os.path.exists(sch):
        corre("sch", "export", "bom", "-o", os.path.join(dest, "bom_%s.csv" % nome),
              "--fields", "Reference,Value,Footprint,QUANTITY",
              "--labels", "Designator,Comment,Footprint,Qty",
              "--group-by", "Value,Footprint", "--exclude-dnp", sch)

    # ⚠️ O CODIGO LCSC ENTRA NA BOM, e sem ele a linha nao se monta.
    # O MPN e para humanos; o `Cxxxxx` e o que a maquina le, e a JLC nao monta
    # uma linha sem ele — tenha MPN ou nao. Ate a v0.4.0 este ficheiro escrevia
    # no LEIA-ME "o BOM nao tem codigos LCSC, preenche-os a mao": um passo
    # manual dentro de um processo que existe para nao ter passos manuais, e o
    # sitio exacto onde se erra a peca por cansaco.
    bomf = os.path.join(dest, "bom_%s.csv" % nome)
    if os.path.exists(bomf):
        erro = mete_lcsc(bomf, nome)
        if erro:
            print("   ⚠️ BOM sem codigos: %s" % erro)
            return False
    desacordo = cpl_bate_bom(dest, nome)
    if desacordo:
        print("   ⚠️ CPL e BOM discordam: %s" % desacordo)
        return False

    zipe = dest + "_gerbers.zip"
    with zipfile.ZipFile(zipe, "w", zipfile.ZIP_DEFLATED) as z:
        for a in sorted(os.listdir(dest)):
            if a.startswith(("cpl_", "bom_")):
                continue
            z.write(os.path.join(dest, a), a)
    n = len(zipfile.ZipFile(zipe).namelist())

    with open(os.path.join(dest, "LEIA-ME.txt"), "w") as f:
        f.write("pacote de fabrico — cotuca %s\n" % nome)
        f.write("gerado por para_fabrica v%s em %s\n" % (VERSAO, data))
        f.write("commit %s%s\n" % (commit, "  ⚠️ ARVORE SUJA" if sujo else ""))
        f.write("\ngeometria 0 · planos inteiros · %s\n"
                % ("%d ligacoes POR FAZER ⚠️" % faltam if faltam
                   else "todas as ligacoes feitas"))
        f.write("\n⚠️ A rotacao no CPL e a do KiCad, nao a da biblioteca da JLC.\n"
                "   Confirmar peca a peca na pre-visualizacao antes de pagar.\n")
        f.write("   As pecas com polaridade, que sao as unicas que importam:\n")
        for r_, mpn_, enc_, oq_ in POLARIZADAS.get(nome, []):
            f.write("     %-4s %-12s %-8s %s\n" % (r_, mpn_, enc_, oq_))
        f.write("   Os 0805/1206/1210 nao tem polaridade — nao gastar tempo neles.\n")
        f.write("\nO BOM leva a coluna LCSC preenchida (tools/codigos_lcsc.py).\n")
        for l in ABERTAS.get(nome, []):
            f.write("\n" + l + "\n")

    print("\n   pacote: %s" % os.path.relpath(zipe, os.path.join(AQUI, os.pardir)))
    print("     %d ficheiros no zip · CPL e BOM ao lado, fora dele" % n)
    print("     commit %s%s" % (commit, "  ⚠️ ARVORE SUJA" if sujo else ""))
    if faltam:
        print("     ⚠️ INCOMPLETA: %d ligacoes por fazer" % faltam)

    # ⚠️ ATE AQUI o pacote so existe em fabrica/, que e GITIGNORED — some com o
    # container e nunca chega ao repositorio nem ao PR, e era preciso um humano
    # decidir como o tirar de la. O portao nao pode terminar num artefacto que
    # ninguem consegue descarregar. Por isso o pacote enxuto (zip de gerbers +
    # CPL + BOM + LEIA-ME) e copiado AUTOMATICAMENTE para export/, a pasta de
    # topo RASTREADA (ver .gitignore), e e ESSE o caminho duravel que se anuncia
    # por ultimo — o estagio 8 so fecha quando ele esta commitado.
    rel = os.path.join(EXPORT, "%s%s_%s_%s" % (tag, nome, data, commit))
    os.makedirs(rel, exist_ok=True)
    for a in os.listdir(rel):
        os.remove(os.path.join(rel, a))
    shutil.copy2(zipe, os.path.join(rel, os.path.basename(zipe)))
    for a in ("cpl_%s.csv" % nome, "bom_%s.csv" % nome, "LEIA-ME.txt"):
        p = os.path.join(dest, a)
        if os.path.exists(p):
            shutil.copy2(p, os.path.join(rel, a))
    print("   entregue (rastreado — commita este caminho): %s"
          % os.path.relpath(rel, os.path.join(AQUI, os.pardir)))
    return True


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    print("para_fabrica v%s" % VERSAO)
    quais = (["cerebro", "painel"] if sys.argv[1] == "todas" else [sys.argv[1]])
    inc = "--incompleta" in sys.argv
    ok = [uma(q, inc) for q in quais]
    print()
    if all(ok):
        print("PRONTO para a JLCPCB." if not inc else
              "Pacote INCOMPLETO gerado — nao encomendar assim.")
        return 0
    print("NAO PRONTO. Nada foi encomendado, e e para isso que o portao serve.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
