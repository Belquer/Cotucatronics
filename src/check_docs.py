#!/usr/bin/env python3
"""check_docs — dá testemunha à documentação (versão em VERSION).

O compilador cobra os símbolos; nada cobrava a doc. Depois do audit de
deriva de 2026-08-22 (doze achados, ver bts 13h), isto passa a cobrar.

PRINCÍPIO: o manifest é a única fonte de verdade. Tudo que fala sobre
ele é verificado contra ele.

  fantasma  = nome com cara de param citado na doc e inexistente no
              manifest  -> ERRO, quebra o build
  CC errado = número citado que não bate com o mapa  -> ERRO
  órfão     = `static` com <=2 ocorrências no fonte  -> AVISO

Exceção só DECLARADA, na própria linha ou na anterior:
  <!-- doc-check: ignore nome (motivo) -->
Fantasma declarado deixa de ser fantasma; fantasma silencioso quebra.

CUIDADO DE IMPLEMENTAÇÃO (regra 3 da 12e): isto casa padrão em TEXTO.
Imprime o que casou antes de falhar — senão vira o verificador que
acusa a frase que documenta a remoção, que foi a 12c.
"""
import re, sys, pathlib

# A versao mora AQUI e e impressa na primeira linha da saida. Ela ja
# viveu so no docstring, onde envelheceu de 0.1.0 a 0.3.1 sem ser
# corrigida — porque nunca aparecia em lugar nenhum.
VERSION = "0.14.0"

RAIZ = pathlib.Path(__file__).resolve().parent.parent
SRC  = RAIZ / "src"
# rglob, NAO glob: `docs/operacao/` chegou com sete documentos e o glob
# raso nao via nenhum. Pasta nova entra sem vigilancia por padrao, e o
# verificador que nao alcanca um documento diz "ok" sobre ele — que e a
# forma mais silenciosa desta familia (verificacao sem objeto PASSA, e
# ninguem desconfia de um ok).
DOCS = [RAIZ / "README.md"] + sorted((RAIZ / "docs").rglob("*.md"))

def faixas():
    """min/max de cada param, pra conferir FAIXAS citadas na prosa.

    A LACUNA QUE O AUDIT DE 2026-08-22 DEIXOU ABERTA. O item de backlog do
    `frz_size` dizia "faixa 20-400" quando o manifest declarava 20-165 —
    e o nome estava certo, entao o verificador de fantasmas passou por
    ele. Nome confere; NUMERO nao conferia.

    Confere so quando param e faixa aparecem na MESMA linha: prosa esta
    cheia de numeros, e conferir todos seria falso-positivo garantido.
    """
    m = (SRC / "manifest.h").read_text(errors="replace")
    out = {}
    for nome, mi, ma in re.findall(
            r't\.Add\("([a-z_0-9]+)",\s*[-\d.f]+,\s*([-\d.]+)f?,\s*([-\d.]+)f?',
            m):
        out[nome] = (float(mi), float(ma))
    return out

def verdade():
    m = (SRC / "manifest.h").read_text(errors="replace")
    params = set(re.findall(r't\.Add\("([a-z_0-9]+)"', m))
    # AddMidi(cc, param, canal) — o canal entrou em v0.8.2 (2026-08-24). Ate ai o
    # regex exigia DOIS argumentos e fechava parentese: com tres, deixou de
    # casar e o contador passou a dizer "0 CCs" ao lado de um "ok".
    # A chave e o PAR (cc, canal), nao o numero: com seis canais o mesmo CC
    # aparece em tres, e indexar so pelo numero apagaria dois tercos calado.
    ccs = {}
    for num, alvo, canal in re.findall(
            r'AddMidi\(\s*(\d+)\s*,\s*([A-Za-z_0-9\[\]]+)\s*'
            r'(?:,\s*([A-Za-z_0-9]+)\s*)?\)', m):
        ccs[(int(num), canal or 'kCanalCompart')] = alvo
    if not ccs:
        print("check_docs: AddMidi nao casou NENHUMA vez — o regex envelheceu.")
        raise SystemExit(2)
    return params, ccs


def versoes_divergentes():
    """Cabecalho `// nome vX.Y.Z` contra `#define COTUCA_*_VERSION "X.Y.Z"`.

    NASCEU DE UM CASO REAL (2026-08-24): quatro ficheiros carregavam DUAS versoes
    que tinham divergido — `hal_daisy_seed` dizia 0.9.0 na macro estando em
    0.38.0 no cabecalho, `midi` dizia 0.2.1 estando em 0.8.0. As macros nunca
    eram impressas em lado nenhum, logo nada as corrigia: a placa anunciava
    uma versao dezenas de revisoes atrasada e ninguem podia notar.

    O cabecalho e o que o humano edita; a macro e o que a placa diz. Quando
    discordam, a rotina de "o fonte e a placa batem?" mede o ficheiro errado.
    """
    fora = []
    for f in sorted(SRC.glob("*.h")) + sorted(SRC.glob("*.cpp")):
        txt = f.read_text(errors="replace")
        cab = re.search(r'^//[^\n]*?\bv(\d+\.\d+\.\d+)', txt, re.M)
        mac = re.search(r'#define\s+COTUCA_[A-Z_]*VERSION\s+"(\d+\.\d+\.\d+)"', txt)
        if not cab or not mac:
            continue
        # main.cpp declara no proprio cabecalho que a versao mora SO na macro
        if "A VERSAO MORA SO EM" in txt:
            continue
        if cab.group(1) != mac.group(1):
            fora.append((f.name, cab.group(1), mac.group(1)))

    # ---- PYTHON: a mesma familia, sem macro para cruzar ----
    #
    # CASO REAL (2026-08-28): `nivel_tempo.py` dizia v0.2.0 no docstring e
    # IMPRIMIA v0.1.0. A regra acima nao o via — ela cruza cabecalho com
    # `#define`, e um script nao tem `#define`. Foi apanhado por acaso,
    # ao ler a saida durante outra medicao.
    #
    # Aqui a forma e outra: em vez de cruzar DUAS fontes conhecidas,
    # recolhe-se TODA a auto-identificacao do ficheiro e exige-se que
    # concordem. As tres formas que este repo usa:
    #   1. primeira linha do docstring   """nome vX.Y.Z — ..."""
    #   2. VERSAO / VERSION = "X.Y.Z"
    #   3. o que o script IMPRIME        print("nome vX.Y.Z ...")
    #
    # A (3) e a que mais importa: e a unica que o operador ve. Numero que
    # ninguem imprime nao tem quem o corrija — e numero que se imprime
    # errado mente com autoridade.
    for f in sorted(SRC.glob("*.py")) + sorted(SRC.glob("*.sh")):
        txt = f.read_text(errors="replace")
        achados = {}
        doc = re.match(r'\s*(?:#![^\n]*\n)?\s*"""([^\n]*)', txt)
        if doc:
            m = re.search(r'\bv(\d+\.\d+\.\d+)', doc.group(1))
            if m: achados.setdefault(m.group(1), []).append("docstring")
        for m in re.finditer(r'^(?:VERSAO|VERSION)\s*=\s*"(\d+\.\d+\.\d+)"',
                             txt, re.M):
            achados.setdefault(m.group(1), []).append("constante")
        for m in re.finditer(r'print\s*\(\s*f?"[^"]*?\bv(\d+\.\d+\.\d+)', txt):
            achados.setdefault(m.group(1), []).append("print")
        if len(achados) > 1:
            ordenado = sorted(achados.items())
            fora.append((f.name,
                         "%s (%s)" % (ordenado[0][0], "/".join(ordenado[0][1])),
                         "%s (%s)" % (ordenado[1][0], "/".join(ordenado[1][1]))))
    return fora

def isentos(texto):
    """nomes declarados como proposta/removido, na linha ou na anterior"""
    fora = set()
    for mm in re.finditer(r'<!--\s*doc-check:\s*ignore\s+([a-z_0-9]+)', texto):
        fora.add(mm.group(1))
    return fora

def simbolos_do_fonte():
    """Todo identificador que existe no fonte.

    A REGRA QUE MATA A CLASSE INTEIRA DOS FALSOS POSITIVOS: se o nome
    EXISTE no codigo, cita-lo e legitimo. `sdram_pool`, `eco_ult`,
    `reverb_mem` nao sao params, e a doc fala deles com razao.

    Fantasma e nome que nao existe em lugar NENHUM — nem param, nem
    simbolo, nem aposentado declarado.
    """
    vistos = set()
    # NOME DE FICHEIRO OU PASTA TAMBEM E COISA REAL. Em 2026-08-27 o
    # `teste_minimo` — um banco de provas em `src/teste_minimo/` — foi
    # acusado de fantasma duas vezes: existe no repo e nao existe como
    # simbolo C++. A regra desta funcao ja dizia que citar o que EXISTE e
    # legitimo; faltava-lhe ver a arvore, e nao so o codigo.
    for x in SRC.rglob("*"):
        vistos.add(x.stem)
        vistos.add(x.name)
    for p in list(SRC.glob("*.cpp")) + list(SRC.glob("*.h")):
        vistos |= set(re.findall(r'\b([a-z][a-z0-9]*(?:_[a-z0-9]+)+)\b',
                                 p.read_text(errors="replace")))
    return vistos

def aposentados():
    """Params que EXISTIRAM e sairam. Citalos na historia e legitimo.

    Sem esta lista, toda cicatriz que narra uma remocao vira fantasma —
    que e a 12c em forma de ferramenta: o verificador acusando a frase
    que documenta o conserto.
    """
    arq = RAIZ / "docs" / "params-aposentados.txt"
    if not arq.exists(): return set()
    fora = set()
    for linha in arq.read_text(errors="replace").splitlines():
        linha = linha.strip()
        if not linha or linha.startswith("#"): continue
        fora.add(linha.split()[0])
    return fora

def comandos_sem_ajuda():
    """Comando tratado que nao aparece na linha do `?`.

    Comando que nao consta e comando que NAO EXISTE para quem esta na
    bancada com a serial aberta — a descoberta e pela linha de ajuda, nao
    pela leitura do fonte. Tres apareceram assim no mesmo dia (`dup`,
    `espelho`, `dreno`), e o padrao e sempre o mesmo: escreve-se o
    handler, testa-se chamando pelo nome, e a linha de ajuda fica.
    """
    src = (SRC / 'main.cpp').read_text(encoding='utf-8', errors='replace')
    m = re.search(r'hal::Log\("\? ([^"]*)"\)', src)
    if not m:
        return None, []          # sem linha de ajuda: quem chama decide
    ajuda = m.group(1)
    tratados = set(re.findall(r'strncmp\(line,\s*"([a-z0-9]+)', src))
    # `k1`..`k5` e `b2`/`b3` entram por outro caminho e ja estao na linha
    # sob outra forma; o resto tem que constar literalmente.
    # ISENCAO MINIMA, e ela ja esteve larga demais: `b2` e `b3` estavam
    # aqui e sao comandos de verdade (pulso de freeze e de rearme). Uma
    # isencao generosa transforma o verificador em carimbo. Ficam so os
    # `k1..k5`, que a linha ja anuncia como faixa, e os dois `midi*`, que
    # ela lista com o nome inteiro.
    isento = {'k1', 'k2', 'k3', 'k4', 'k5', 'midiloop', 'miditest'}
    faltam = sorted(c for c in tratados
                    if c not in isento and c not in ajuda)
    return ajuda, faltam


def versao_derivada():
    """Arquivo tocado sem a versao dele mudar no mesmo commit.

    A regra da casa e que toda alteracao sobe a versao. O que faltava era
    a TESTEMUNHA: em 2026-08-23 tres `sed` de bump nao casaram o padrao e
    falharam em SILENCIO — o hal.h ficou em v0.18.0 enquanto tres funcoes
    novas entravam. O script disse nada, e nada foi lido como sucesso.

    O `check-docs` vigiava a doc contra o codigo e nao vigiava se o
    NUMERO acompanhou o arquivo. Este e o buraco.

    Criterio: no ULTIMO commit que tocou o arquivo, a linha de versao
    mudou? Se nao, entra na lista. E AVISO e nao erro, porque existe
    commit legitimo que nao muda comportamento — mas ele tem que ser
    olhado, nao presumido.
    """
    import subprocess
    fora = []
    arquivos = sorted(SRC.glob("*.h")) + sorted(SRC.glob("*.cpp")) \
             + sorted(SRC.glob("*.py")) + sorted(SRC.glob("*.sh"))
    for p in arquivos:
        rel = str(p.relative_to(RAIZ))
        try:
            d = subprocess.run(["git", "log", "-p", "-1", "--", rel],
                               cwd=str(RAIZ), capture_output=True,
                               text=True, timeout=20).stdout
        except Exception:
            continue
        if not d:
            continue
        # ⚠️ COMMIT QUE NAO MUDA LINHA NENHUMA NAO E UM TOQUE.
        #
        # `chmod +x` aparece no `git log` do ficheiro com "0 insertions,
        # 0 deletions". O criterio "o ultimo commit que o tocou mexeu na
        # versao?" acusava-o na mesma, e subir a versao por causa de um
        # bit de permissao seria escrever um numero que nao descreve
        # alteracao nenhuma — mentira com cara de disciplina.
        #
        # Apanhado em 2026-09-02 no `porta_daisy.py`. O verificador estava a
        # cobrar a regra a quem nao a tinha violado, e isso gasta a
        # credibilidade dele: aviso que se aprende a ignorar deixa de ser
        # aviso, e leva os verdadeiros com ele.
        tem_conteudo = any(
            (l.startswith("+") and not l.startswith("+++")) or
            (l.startswith("-") and not l.startswith("---"))
            for l in d.splitlines())
        if not tem_conteudo:
            continue
        # linha ADICIONADA que carrega um numero de versao
        # RECONHECER A VERSAO EM QUALQUER LINGUAGEM DA CASA.
        #
        # A primeira versao exigia `//` ou a palavra "version" e nao
        # reconhecia docstring de Python (`"""nome v0.2.0 ..."""`).
        # Resultado: acusava dois arquivos CORRETOS de deriva — o
        # verificador de ponto cego com ponto cego proprio, na mesma
        # sessao em que a regra foi escrita.
        #
        # Agora casa em comentario de qualquer linguagem, docstring, ou
        # linha que cite o nome do arquivo — que e como toda declaracao
        # de versao deste projeto se parece.
        stem = p.stem
        subiu = any(l.startswith("+") and not l.startswith("+++")
                    and re.search(r'v?\d+\.\d+\.\d+', l)
                    and (re.search(r'(?i)version', l)
                         or re.match(r'\+\s*(//|#|"""|\*)', l)
                         or stem in l)
                    for l in d.splitlines())
        if not subiu:
            fora.append(p.name)
    return fora


def pinos():
    """Confere o mapa de pinos DECLARADO contra os `seed::Dnn` do fonte.

    Existe porque a placa esta entupida e os pontos de solda espremidos:
    conferir de cabeca e o que falha exatamente quando errar custa mais.

    E porque um mapa SUPOSTO ja custou horas — o encoder global estava em
    D17/D18/D19 e o firmware media D20/D21. Todos os instrumentos
    apontavam para o lugar errado, e nenhum podia dizer isso: supor o
    mapa e depois medir contra a suposicao so encontra o que ja se
    esperava.
    """
    doc = (RAIZ / "docs/operacao/PINOS.md")
    if not doc.exists():
        return None, None, None
    t = doc.read_text(encoding="utf-8", errors="replace")
    m = re.search(r'<!-- pinos: inicio -->(.*?)<!-- pinos: fim -->', t, re.S)
    if not m:
        return None, None, None
    declarado = {}
    for ln in m.group(1).splitlines():
        mm = re.match(r'\|\s*D(\d+)\s*\|\s*([^|]+?)\s*\|', ln)
        if mm:
            declarado[int(mm.group(1))] = mm.group(2)

    src = (SRC / "hal_daisy_seed.cpp").read_text(encoding="utf-8",
                                                 errors="replace")
    # As tabelas de SONDA apontam de proposito para pinos ja reclamados;
    # nao sao donos. Sem esta isencao todo pino instrumentado viraria
    # conflito, e o verificador so gritaria.
    # Recorta os BLOCOS inteiros, nao so a primeira linha: sao
    # inicializadores de varias linhas, e filtrar por linha deixava passar
    # as continuacoes — foi o que acusou seis pinos livres como donos.
    #
    # O `kEncPin` SAIU DESTA LISTA em 2026-08-28, e a razao e uma mudanca de
    # papel, nao um ajuste: ele era sonda enquanto o `EncoderInit` tinha
    # os pinos escritos a mao ao lado. Passou a ser a FONTE — o `Init` le
    # dela num laco, e os nomes saem por `EncoderPinNome`. Continuar a
    # recorta-la fez o verificador declarar que os nove pinos dos
    # encoders "nao aparecem no fonte", o que era verdade para ele e
    # falso para a placa.
    #
    # A licao vale alem deste caso: **isencao de verificador supoe um
    # PAPEL, e refactor muda papel.** Ao promover uma estrutura a fonte
    # unica, procure quem a estava a ignorar de proposito.
    limpo = src
    # `kTodosPin` entra aqui e o `kEncPin` saiu, no mesmo dia — e a
    # distincao e exactamente a licao: o criterio nao e "e uma tabela de
    # pinos", e QUEM MANDA. O `kEncPin` virou a fonte que o `EncoderInit`
    # le; o `kTodosPin` lista os 31 para uma sonda de diagnostico e nao
    # reclama nada, incluindo os RESERVADOS D29/D30.
    for nome in ("kLivresPin", "kTodosPin", "kPino"):
        limpo = re.sub(nome + r'\s*\[[^\]]*\][^=]*=\s*\{.*?\};',
                       '', limpo, flags=re.S)
    limpo = re.sub(r'strcmp\(nome[^\n]*\n', '', limpo)
    usados = set(int(mm.group(1))
                 for mm in re.finditer(r'seed::D(\d+)', limpo))

    reservados = {d for d, n in declarado.items() if "RESERVADO" in n}
    no_codigo_sem_doc = sorted(usados - set(declarado))
    no_doc_sem_codigo = sorted(d for d in declarado
                               if d not in usados and d not in reservados)
    reservado_usado   = sorted(reservados & usados)
    return no_codigo_sem_doc, no_doc_sem_codigo, reservado_usado


def main():
    print(f"check_docs v{VERSION}")
    params, ccs = verdade()
    simb = simbolos_do_fonte()
    apos = aposentados()
    # FAMILIA: `gov_ceiling` citado quando existem gov_ceiling_a/b.
    # Sem isto o verificador acusa o nome da familia, que e como a doc
    # fala dela — e falar da familia e mais legivel que listar as duas.
    familias = {re.sub(r'_(a|b)$', '', p) for p in params}
    print(f"manifest: {len(params)} params ({len(familias)} familias),"
          f" {len(ccs)} CCs | fonte: {len(simb)} simbolos |"
          f" aposentados: {len(apos)}")
    erros, avisos = [], []

    for doc in DOCS:
        if not doc.exists(): continue
        texto = doc.read_text(errors="replace")
        livres = isentos(texto)
        # nome com cara de param: em crase, minusculo, com _ , 2+ partes
        for mm in re.finditer(r'`([a-z][a-z0-9]*(?:_[a-z0-9]+)+)`', texto):
            nome = mm.group(1)
            if (nome in params or nome in familias or nome in livres
                    or nome in simb or nome in apos):
                continue
            linha = texto[:mm.start()].count("\n") + 1
            erros.append((doc.name, linha, nome,
                          texto.split("\n")[linha-1].strip()[:70]))

    a, b, c = pinos()
    if a is None:
        print("\nPINOS: sem tabela declarada em PINOS.md — checagem sem"
              " objeto, que e pior que checagem ausente.")
    else:
        print(f"\nPINOS: {len(a)} no codigo e fora do doc |"
              f" {len(b)} no doc e nao usados | {len(c)} RESERVADOS usados")
        for d in a: print(f"  D{d} reclamado no fonte e AUSENTE do PINOS.md")
        for d in b: print(f"  D{d} no PINOS.md e nao aparece no fonte")
        for d in c: print(f"  *** D{d} e RESERVADO e esta sendo usado ***")

    sem_bump = versao_derivada()
    print(f"\nTOCADOS SEM SUBIR A VERSAO (aviso): {len(sem_bump)}")
    for n in sem_bump:
        print(f"  {n} — o ultimo commit que o tocou nao mexeu na versao")

    ajuda, sem_ajuda = comandos_sem_ajuda()
    if ajuda is None:
        print("\nAVISO: nao achei a linha de ajuda `?` — checagem sem objeto,"
              " que e pior que checagem ausente. Conferir o regex.")
    else:
        print(f"\nCOMANDOS SEM AJUDA: {len(sem_ajuda)}")
        for c in sem_ajuda:
            print(f"  `{c}` e tratado e nao aparece no `?`")

    div = versoes_divergentes()
    print(f"\nDUAS VERSOES NO MESMO ARQUIVO QUE DISCORDAM: {len(div)}")
    for nome, cab, mac in div:
        print(f"  {nome}: {cab} · {mac}")

    print(f"\nFANTASMAS: {len(erros)}")
    for d, l, n, ctx in erros[:40]:
        print(f"  {d}:{l}  `{n}`")
        print(f"      | {ctx}")

    # órfãos no fonte
    for f in sorted(SRC.glob("*.cpp")) + sorted(SRC.glob("*.h")):
        t = f.read_text(errors="replace")
        for mm in re.finditer(r'^static\s+(?:const\s+)?[A-Za-z_][\w:*&<> ]*?\b(\w+)\s*(?:\[[^\]]*\])?\s*=', t, re.M):
            nome = mm.group(1)
            n = len(re.findall(r'\b' + re.escape(nome) + r'\b', t))
            if n <= 2:
                avisos.append((f.name, nome, n))
    # ---- FAIXAS: nome certo, numero errado ----
    #
    # A LACUNA QUE O AUDIT DE 2026-08-22 DEIXOU: o item do `frz_size` dizia
    # "faixa 20-400" com o manifest em 20-165. O NOME estava certo, entao
    # o verificador de fantasmas passou por ele.
    #
    # SO ONDE A DOC DECLARA A FAIXA COM A PALAVRA. Duas tentativas mais
    # amplas foram descartadas por falso-positivo: casar qualquer par de
    # numeros na linha acusou o `drv_sujeira` pelo "1,5-48x" do
    # `drv_drive`, e o `dly_time` por "60-64", que e faixa de CC. Os
    # numeros da doc sao majoritariamente CC e versao — conferir todos
    # seria acusar o inocente, e verificador que acusa inocente perde a
    # autoridade que o torna util (12c).
    fx = faixas()
    ruins = []
    padrao = re.compile(
        r'faixa\s+(\d+(?:[.,]\d+)?)\s*[-\u2013a]+\s*(\d+(?:[.,]\d+)?)',
        re.I)
    for doc in DOCS:
        if not doc.exists(): continue
        for ln, linha in enumerate(doc.read_text(errors="replace").split("\n"), 1):
            if "doc-check: ignore" in linha: continue
            mm = padrao.search(linha)
            if not mm: continue
            citados = {x for x in re.findall(r'`([a-z][a-z0-9_]+)`', linha)
                       if x in fx or (x + "_a") in fx}
            if len(citados) != 1: continue
            nome = citados.pop()
            base = nome if nome in fx else nome + "_a"
            mi, ma = fx[base]
            a = float(mm.group(1).replace(",", "."))
            b = float(mm.group(2).replace(",", "."))
            if (a, b) != (mi, ma):
                ruins.append((doc.name, ln, nome, a, b, mi, ma,
                              linha.strip()[:66]))
    print(f"\nFAIXAS DECLARADAS QUE NAO BATEM: {len(ruins)}")
    for d, l, n, a, b, mi, ma, ctx in ruins[:12]:
        print(f"  {d}:{l}  `{n}` diz {a:g}-{b:g}, manifest diz {mi:g}-{ma:g}")
        print(f"      | {ctx}")
    erros += [(d, l, n, ctx) for d, l, n, _, _, _, _, ctx in ruins]

    print(f"\nORFAOS (aviso, exige julgamento): {len(avisos)}")
    for f, n, c in avisos[:20]:
        print(f"  {f}: {n} ({c} ocorrencias)")

    if erros:
        print(f"\nFALHOU: {len(erros)} fantasma(s). Corrija, ou declare com")
        print("  <!-- doc-check: ignore nome (motivo) -->")
        return 1
    print("\nok — nenhum fantasma")
    return 0

sys.exit(main())
