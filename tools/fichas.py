#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""fichas.py — descarrega a ficha tecnica de cada peca com MPN real.

    tools/fichas.py            descarrega o que falta
    tools/fichas.py --confere  so verifica o que ja esta em disco
    tools/fichas.py --tudo     volta a descarregar tudo

⚠️⚠️ PORQUE ESTE FICHEIRO EXISTE, E NAO O DO PLUGIN

O `sync_datasheets_lcsc.py` do kicad-happy 2.2.1 falhou nas 19 pecas, e a
causa NAO era a minha lista: a API do `jlcsearch` deixou de servir o campo
`datasheet`. Ela ENCONTRA a peca (casamento exacto no MPN, com stock e preco)
e devolve o campo vazio. O plugin trata "achei mas sem URL" como fim de linha
e nunca chega ao segundo caminho que ele proprio tem.

A cadeia que funciona sao DOIS passos:

    jlcsearch  MPN  -> codigo LCSC (Cxxxxx)
    wmsc.lcsc  Cxxxxx -> pdfUrl

⚠️ E nao consertei a cache do plugin: `~/.claude/plugins/cache/.../2.2.1/` e
copia de trabalho de outra pessoa, e a proxima actualizacao apaga a edicao sem
avisar. Conserto que vive num sitio que outro processo reescreve nao e
conserto — e uma armadilha para quem vier depois ver a versao 2.2.2 partida
outra vez.

⚠️⚠️ CASAMENTO EXACTO, E SO EXACTO

Uma busca por texto devolve primos. `SS14` traz `SS14**0**`; `SI2302CDS-T1-GE3`
traz o `SI2302` cru e o `SI2302DS`; uma ficha da familia certa e do sufixo
errado le-se igual a uma ficha certa — e a §7f desta casa ja custou tres pecas
nisso (o eixo liso em
vez de estriado, os 5,0 mm em vez de 2,5, o 3 A que nao existe a 24 V). Aqui
so entra `mfr == MPN`. Sem primeiro-resultado, sem parecido.

⚠️⚠️ E O PDF E CONFERIDO POR DENTRO

Um PDF chamado `OPA1642AIDR.pdf` pode ter qualquer coisa. A LCSC aponta fichas
de familia, redirecciona para paginas de erro, e serve HTML com extensao .pdf.
Por isso cada descarga passa pelo `pdftotext` — ferramenta de fora, que nao
escrevi e que pode dizer que nao — e o MPN tem de aparecer no texto. O que nao
passa fica marcado `POR CONFERIR`, nunca apagado em silencio nem dado por bom.
"""

VERSAO = "0.10.0"

import io
import json
import os
import re
import subprocess
import sys
import time
import urllib.parse
import urllib.request

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
DESTINO = os.path.join(RAIZ, "docs", "datasheets")
UA = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)"
PAUSA = 0.6


def json_de(url, tenta=3):
    for i in range(tenta):
        try:
            req = urllib.request.Request(
                url, headers={"User-Agent": UA, "Accept": "application/json"})
            with urllib.request.urlopen(req, timeout=25) as r:
                return json.loads(r.read())
        except Exception as e:
            if i == tenta - 1:
                return {"__erro__": str(e)}
            time.sleep(1.5 * (i + 1))
    return None


def achata(s):
    """Tira SO pontuacao e o grupo de embalagem entre parentesis.

    ⚠️ A v0.1.0 exigia igualdade literal e recusou treze pecas que a LCSC TEM —
    ela escreve `EEUFR1C101` sem hifens, `530470710` sem o traco, e
    `B2B-XH-A(LF)(SN)` com o codigo de embalagem colado. Treze "nao existe" que
    eram treze diferencas de pontuacao.

    ⚠️ E o que esta funcao NAO faz e o que a torna segura: nao corta prefixos,
    nao aceita parecidos, nao tira letras. `EEUFR1E471B` continua a ser
    diferente de `EEUFR1E471`, que e uma peca outra. Provado sobre a lista
    inteira: 21 MPNs dao 21 formas achatadas distintas, zero choques."""
    return re.sub(r"[^A-Z0-9]", "", re.sub(r"\([^)]*\)", "", s.upper()))


def codigo_lcsc(mpn):
    """MPN -> (codigo, fabricante) com casamento EXACTO, ou (None, motivo)."""
    d = json_de("https://jlcsearch.tscircuit.com/api/search?q=%s&limit=10&full=true"
                % urllib.parse.quote(mpn))
    if "__erro__" in d:
        return None, "busca falhou: %s" % d["__erro__"]
    cs = d.get("components", [])
    if not cs:
        return None, "a LCSC nao tem esta peca"
    alvo = achata(mpn)
    for c in cs:
        if achata(c.get("mfr") or "") == alvo:
            return "C%s" % c.get("lcsc"), c.get("description", "")
    # ⚠️ havia resultados e nenhum era a peca. Isto NAO e "nao encontrado" — e
    # "encontrei primos", e o primo e que e o perigo. Diz-se quais, para quem
    # ler saber que a busca correu e o que ela achou.
    return None, ("so primos na LCSC: %s"
                  % ", ".join(sorted({(c.get("mfr") or "?") for c in cs})[:4]))


def url_da_ficha(cod):
    d = json_de("https://wmsc.lcsc.com/ftps/wm/product/detail?productCode=%s" % cod)
    if "__erro__" in d:
        return None, "detalhe falhou: %s" % d["__erro__"]
    r = d.get("result") or d
    if not isinstance(r, dict):
        return None, "detalhe sem resultado"
    u = r.get("pdfUrl") or r.get("dataSheetUrl") or ""
    if isinstance(u, list):
        u = u[0] if u else ""
    if not u or not isinstance(u, str):
        return None, "a LCSC tem a peca mas nao publica ficha"
    return u, r.get("brandNameEn") or ""


def baixa(url, caminho, tenta=3):
    # ⚠️ COM RETENTATIVA. A primeira corrida perdeu a Molex 53047-0710 por um
    # `read operation timed out` — a peca existia, a URL estava certa, e a
    # unica coisa errada foi a rede naquele segundo. Falha transitoria contada
    # como "sem ficha" mente sobre o catalogo.
    dados = None
    for i in range(tenta):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": UA})
            with urllib.request.urlopen(req, timeout=90) as r:
                dados = r.read()
            break
        except Exception as e:
            if i == tenta - 1:
                return "descarga falhou apos %d tentativas: %s" % (tenta, e)
            time.sleep(2.0 * (i + 1))
    # ⚠️ extensao .pdf nao faz um PDF. Uma pagina de erro servida com o nome
    # certo passa por qualquer verificacao que so olhe para o nome.
    if not dados.startswith(b"%PDF"):
        return "o que veio nao e PDF (comeca por %r)" % dados[:12]
    if len(dados) < 4096:
        return "PDF de %d bytes — pequeno de mais para ser ficha" % len(dados)
    io.open(caminho, "wb").write(dados)
    return None


def confere(caminho, mpn):
    """O MPN aparece DENTRO do PDF? Devolve (degrau, detalhe)."""
    try:
        # ⚠️ SEM `-l`. A v0.1.0 lia 6 paginas e deu `OPA1642AIDR` por SUSPEITA
        # numa ficha onde o MPN aparece tres vezes — a tabela de encomenda da
        # TI vive no FIM. Um verificador que le meio documento reprova o
        # documento certo, e reprovacao falsa e o caminho mais curto para se
        # desligar o verificador.
        p = subprocess.run(["pdftotext", caminho, "-"],
                           capture_output=True, timeout=120)
        txt = p.stdout.decode("utf-8", "replace")
    except FileNotFoundError:
        return "POR CONFERIR", "sem pdftotext — nao pude conferir"
    except Exception as e:
        return "POR CONFERIR", "pdftotext falhou: %s" % e
    if not txt.strip():
        return "POR CONFERIR", "PDF sem texto (digitalizado?) — nao pude conferir"
    # ⚠️ ACHATA OS DOIS LADOS, e nao so os espacos. A v0.2.0 tirava espacos e
    # guardava o hifen, e deu SUSPEITA as tres Panasonic — porque a ficha da
    # Panasonic escreve `EEUFR1C101` SEM hifen, e o numero estava la, uma vez,
    # na tabela. Tres documentos certos reprovados por um traco.
    #
    # ⚠️ O que isto custa, dito: juntar tudo pode colar dois tokens vizinhos e
    # inventar um acerto. E risco real e aceite por tamanho — estes MPNs tem 10
    # e 17 caracteres, e uma colagem acidental dessas nao acontece. Para um MPN
    # curto seria imprudente, e por isso o degrau FAMILIA exige >= 5 caracteres.
    achatado = achata(txt)
    alvo = achata(mpn)
    if alvo in achatado:
        return "OK", "MPN completo no texto"
    # ⚠️ TRES degraus, nao dois. Uma ficha de FAMILIA e o documento CERTO — a
    # Bourns publica uma ficha para toda a serie 3296 e o sufixo monta-se na
    # tabela de encomenda. Chamar-lhe SUSPEITA seria acusar o documento bom.
    # Mas passa-la a OK seria dar o sufixo por provado sem o ter lido, e e ai
    # que mora a §7f. Fica um degrau proprio, que obriga a citacao a existir.
    corpo = re.split(r"[-,/]", mpn)[0].upper()
    if len(corpo) >= 5 and corpo in achatado:
        return "FAMILIA", "ficha da serie %s" % corpo
    return "SUSPEITA", "o MPN nao aparece em parte nenhuma do texto"


# ⚠️ ONDE ESTA O QUE A LCSC NAO TEM — com nome e morada, nao "procura ai".
#
# Sete destas oito sao pecas que a LCSC nao cataloga, o que por si so ja diz
# uma coisa util: NAO sao pecas de montagem JLCPCB. Compram-se noutro sitio, e
# a coluna de proveniencia da BOM ja o dizia.
#
# A oitava e outra coisa e nao se confunde com estas: a LCSC TEM a Molex
# (C17454015) e aponta para um PDF alojado em molex.com, que a esta maquina nao
# responde — 0 bytes ao fim de 120 s, em HTTP/2 e em HTTP/1.1. Isso e falha de
# rede alheia e volta a tentar-se; nao e ausencia de catalogo.
FONTE_EXTERNA = {
    "1715721": "Phoenix Contact — MKDS 1,5/2-5,08, ficha do artigo 1715721",
    "53047-0710": ("Molex — desenho de venda 530470710_sd.pdf. A LCSC tem a "
                   "peca (C17454015) e aponta para molex.com, que nao respondeu "
                   "(0 bytes em 120 s, HTTP/2 e 1.1). RETENTAVEL."),
    "C1206C223J5GACTU": "Kemet — serie C0G/NP0 1206, 22 nF 50 V",
    "MKS2C042201M00KN00": "WIMA MKS 2 — 2,2 uF 63 V PET, passo 5 mm (PCM5); Mouser 505-MKS2C042201M00KN",
    "61202621621": "Wurth Elektronik WR-BHD — box header 2x13 26p 2,54 mm THT vertical macho; Mouser 710-61202621621",
    "61304011121": "Wurth Elektronik WR-PHD — barra 1x40 2,54 mm THT vertical macho (corta p/ todos os headers 1 fila); Mouser 710-61304011121",
    "PTV09A-1020S-B103": ("Bourns PTV09A — ⚠️ e nesta que o sufixo importa: "
                          "`S` e eixo estriado e `F` e liso, e ja se escolheu o "
                          "errado uma vez. Conferir a legenda de sufixos."),
    "UES1H220MEM": "Nichicon — serie UES (bipolar audio), 22 uF 50 V",
}

def sem_ficha(mpn, motivo):
    """⚠️ TODO caminho de "sem ficha" passa por aqui.

    A v0.6.0 poe a morada do fabricante so no caminho da busca vazia, e a Molex
    — que falha na DESCARGA, nao na busca — saiu no indice como uma linha nua:
    "descarga falhou apos 3 tentativas". Quem lesse nao sabia que a peca existe,
    que tem codigo LCSC, e que o PDF esta numa URL conhecida. Tres caminhos para
    o mesmo estado davam tres qualidades de aviso, e o pior deles era o da peca
    de que mais se sabia."""
    onde = FONTE_EXTERNA.get(mpn, "")
    return (mpn, "", "SEM FICHA",
            "%s%s" % (motivo, " · **%s**" % onde if onde else ""))


_CACHE = {}


def catalogo_cobre(mpn):
    """A peca esta dentro de uma ficha que ja tenho? Devolve o ficheiro ou None.

    ⚠️ Isto nao e consolo por nao ter achado a ficha propria — e a resposta
    CERTA para uma familia inteira de pecas. A Panasonic nao publica um PDF por
    condensador: publica o catalogo da serie FR, com todos os valores em tabela.
    O `EEU-FR1V222` nao tem ficha na LCSC e esta, com todas as letras, no
    catalogo que ja veio com o `EEU-FR1C101`. Dizer 'sem ficha' com o documento
    em disco e mandar procurar o que ja se tem.

    ⚠️ E vale a pena dize-lo ao contrario: isto SO conta porque o acerto e no
    numero completo achatado, o mesmo criterio do resto. Um catalogo que
    mencione a serie mas nao o valor nao cobre a peca, e nao entra aqui."""
    import glob
    alvo = achata(mpn)
    if not _CACHE:
        for f in sorted(glob.glob(os.path.join(DESTINO, "*.pdf"))):
            try:
                r = subprocess.run(["pdftotext", f, "-"], capture_output=True,
                                   timeout=120)
                _CACHE[f] = achata(r.stdout.decode("utf-8", "replace"))
            except Exception:
                _CACHE[f] = ""
    for f, s in _CACHE.items():
        if alvo in s:
            return os.path.basename(f)
    return None


def main():
    print("fichas.py v%s" % VERSAO)
    so_confere = "--confere" in sys.argv
    tudo = "--tudo" in sys.argv
    sys.path.insert(0, AQUI)
    from bom_completa import MPN, MPN_PAINEL, MPN_POR_FP, SUFIXO
    # ⚠️ guarda de correspondencia: um apontador para uma peca que ja nao esta
    # na BOM e um endereco que ninguem vai visitar, e — pior — nao ha aviso
    # nenhum quando a peca muda de numero. Grita.
    orfaos = set(FONTE_EXTERNA) - set(MPN.values()) - set(MPN_PAINEL.values()) \
        - set(MPN_POR_FP.values())
    if orfaos:
        raise SystemExit("*** FONTE_EXTERNA aponta para peca(s) que a BOM ja "
                         "nao tem: %s" % " ".join(sorted(orfaos)))
    alvos = sorted(set(v for v in list(MPN.values()) + list(MPN_PAINEL.values())
                       + list(MPN_POR_FP.values()) if v))
    os.makedirs(DESTINO, exist_ok=True)
    linhas = []
    for i, mpn in enumerate(alvos, 1):
        nome = re.sub(r"[^A-Za-z0-9._-]", "_", mpn) + ".pdf"
        cam = os.path.join(DESTINO, nome)
        tem = os.path.exists(cam)
        if tem and not tudo:
            marca, det = confere(cam, mpn)
            print("[%2d/%d] %-22s ja em disco · %s (%s)" % (i, len(alvos), mpn, marca, det))
            linhas.append((mpn, nome, marca, det))
            continue
        if so_confere:
            onde = FONTE_EXTERNA.get(mpn, "")
            print("[%2d/%d] %-22s FALTA%s"
                  % (i, len(alvos), mpn, " · %s" % onde if onde else ""))
            linhas.append((mpn, "", "FALTA", onde or "nao esta em disco"))
            continue
        time.sleep(PAUSA)
        cod, det = codigo_lcsc(mpn)
        if not cod:
            outra = catalogo_cobre(mpn)
            if outra:
                print("[%2d/%d] %-22s NO CATALOGO · %s" % (i, len(alvos), mpn, outra))
                linhas.append((mpn, outra, "NO CATALOGO",
                               "a LCSC nao tem ficha propria; o numero completo "
                               "esta neste catalogo de serie"))
                continue
            print("[%2d/%d] %-22s sem ficha · %s" % (i, len(alvos), mpn, det))
            linhas.append(sem_ficha(mpn, det))
            continue
        time.sleep(PAUSA)
        url, marca_fab = url_da_ficha(cod)
        if not url:
            print("[%2d/%d] %-22s %s · %s" % (i, len(alvos), mpn, cod, marca_fab))
            linhas.append(sem_ficha(mpn, "%s · %s" % (cod, marca_fab)))
            continue
        erro = baixa(url, cam)
        if erro:
            print("[%2d/%d] %-22s %s · %s" % (i, len(alvos), mpn, cod, erro))
            linhas.append(sem_ficha(mpn, "%s · %s" % (cod, erro)))
            continue
        marca, det = confere(cam, mpn)
        kb = os.path.getsize(cam) // 1024
        print("[%2d/%d] %-22s %s · %d kB · %s (%s)"
              % (i, len(alvos), mpn, cod, kb, marca, det))
        linhas.append((mpn, nome, marca, det))
    # ⚠️ O `--confere` NAO ESCREVE O INDICE. Ele nao vai a rede, logo nao sabe
    # se a LCSC tem a peca — so sabe que o ficheiro nao esta ca. A v0.7.0
    # deixava-o gravar, e como este modo corre dentro do `para_fabrica`, cada
    # passagem pelo portao substituia as linhas apuradas ("a LCSC nao tem esta
    # peca · Phoenix Contact — MKDS 1,5/2-5,08") por "nao esta em disco".
    #
    # ⚠️ E o mau nao e perder texto: e um VERIFICADOR a degradar o documento que
    # verifica, de cada vez que corre, em silencio. Quem le o indice depois de
    # um portao ve menos do que quem o leu antes, e nada aponta para a causa.
    if not so_confere:
        escreve_indice(linhas, SUFIXO)
    conta = {}
    for _, _, m, _ in linhas:
        conta[m] = conta.get(m, 0) + 1
    print("\n  %s" % " · ".join("%s %d" % (k, v) for k, v in sorted(conta.items())))
    # ⚠️ Uma ficha de FAMILIA sem citacao do sufixo E a falta. O documento esta
    # certo e o sufixo esta por provar — que e exactamente o estado em que a
    # §7f diz que se compra a peca errada. Sem citacao, isto grita.
    mudos = [m for m, _, marca, _ in linhas
             if marca == "FAMILIA" and not SUFIXO.get(m, ("", ""))[0]]
    if mudos:
        print("  sufixo POR CITAR: %s" % " ".join(mudos))
    # ⚠️ SUSPEITA sai com erro; SEM FICHA nao. Uma peca que a LCSC nao tem e um
    # facto do catalogo — nao ha nada a consertar aqui. Uma ficha em disco que
    # nao fala da peca e um documento a mentir, e esse tem de doer.
    if conta.get("SUSPEITA") or mudos:
        raise SystemExit(
            "*** %d ficha(s) nao falam da peca · %d sufixo(s) por citar — ve o "
            "indice antes de encomendar." % (conta.get("SUSPEITA", 0), len(mudos)))
    return 0


def escreve_indice(linhas, sufixo):
    h = subprocess.run(["git", "-C", RAIZ, "log", "-1", "--format=%h"],
                       capture_output=True, text=True).stdout.strip()
    sujo = subprocess.run(["git", "-C", RAIZ, "status", "--porcelain"],
                          capture_output=True, text=True).stdout.strip()
    from datetime import date
    t = ["# Fichas tecnicas\n",
         "Gerado por `tools/fichas.py` v%s em **%s** · commit `%s`%s\n"
         % (VERSAO, date.today().isoformat(), h,
            " · **ARVORE SUJA**" if sujo else ""),
         "\nSo entram pecas com MPN real. Resistores por encapsulamento, headers\n"
         "lisos e modulos comprados montados nao tem MPN, e por isso nao tem\n"
         "linha aqui — ver `tools/bom_completa.py`.\n",
         "\n> **Porque e que os PDF estao no repositorio, e nao um link.**\n"
         "> A regra da casa e *nenhum facto de hardware sem fonte citada* — e a\n"
         "> ficha **e** a fonte. Um repositorio que exige citacao e nao guarda o\n"
         "> documento citado esta a apostar que o link do distribuidor existe\n"
         "> daqui a tres anos. Nao existe. E o portao que recusa o pacote\n"
         "> quando uma ficha nao fala da peca so funciona com as fichas ca\n"
         "> dentro. Sao 13 MB, que em git nao e nada.\n",
         "\n| MPN | ficha | estado | nota |\n|---|---|---|---|\n"]
    for mpn, nome, marca, det in linhas:
        lig = "[%s](%s)" % (nome, urllib.parse.quote(nome)) if nome else "—"
        t.append("| `%s` | %s | %s | %s |\n" % (mpn, lig, marca, det))
    t.append("\n**OK** — o MPN completo aparece no texto do PDF (`pdftotext`).  \n"
             "**FAMILIA** — a ficha e da serie certa e o sufixo monta-se na "
             "tabela de encomenda; ver a citacao abaixo.  \n"
             "**SUSPEITA** — o ficheiro existe e nao fala da peca; nao confiar.  \n"
             "**POR CONFERIR** — PDF sem texto extraivel; olhar com os olhos.  \n"
             "**NO CATALOGO** — sem ficha propria, mas o numero completo esta "
             "num catalogo de serie que ja esta aqui.  \n"
             "**SEM FICHA** — a LCSC nao publica; buscar no site do fabricante.\n")
    cit = [(m, s) for m, s in sorted(sufixo.items())]
    if cit:
        t.append("\n## O que a ficha prova de cada sufixo\n\n"
                 "> A ficha do BAV99 tem um `215` no meio dela, e sao **215 mA** de\n"
                 "> corrente, nao o codigo de embalagem `,215`. Conferir sufixo por\n"
                 "> busca de texto da-o por provado com um numero que fala de outra\n"
                 "> coisa — e confirmacao errada e pior que nenhuma, porque fecha a\n"
                 "> duvida.\n\n")
        for m, (prova, nao) in cit:
            t.append("**`%s`**  \n" % m)
            t.append("- prova: %s\n" % (prova or "— *por citar*"))
            if nao:
                t.append("- **NAO prova: %s**\n" % nao)
            t.append("\n")
    io.open(os.path.join(DESTINO, "README.md"), "w", encoding="utf-8").write("".join(t))


if __name__ == "__main__":
    sys.exit(main())
