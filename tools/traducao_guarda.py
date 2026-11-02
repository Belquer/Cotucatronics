#!/usr/bin/env python3
"""traducao_guarda v0.4.0 — o que a traducao para ingles NAO pode tocar.

    tools/traducao_guarda.py [--escreve-lista]

POR QUE EXISTE. A 2026-11-02 o dB pediu o repositorio inteiro em ingles, com um
limite explicito: os COMANDOS DE CONSOLA e os NOMES DE PARAMS ficam como estao.

    `fio estado` e o que ele escreve na bancada, e esta citado em dezenas de
    procedimentos. Traduzi-lo parte a memoria muscular.

    `post_level_l` e referenciado pelos PERFIS DO LOOPY PRO, pelos dumps de
    preset guardados, pelo CCMAP e por mais oito documentos. Renomea-lo
    invalida os perfis — que custaram um dia inteiro e quatro teorias erradas
    (cicatriz 15fq).

⚠️ E ESSE LIMITE VIVIA SO EM PROSA. Uma traducao de 60 000 linhas feita a mao
perde um token sem ninguem dar por isso, e o sintoma aparece semanas depois na
bancada, com o comando a responder "?" ou o Loopy a nao mexer nada. Esta
ferramenta poe o limite DENTRO do instrumento.

COMO FUNCIONA. Le os tokens protegidos da FONTE (o despachante da consola e a
tabela de params), nao de uma lista a mao — lista a mao envelhece. Depois conta
cada token na arvore e compara com a contagem de referencia guardada em
docs/operacao/traducao-tokens.json.

    --escreve-lista    grava a contagem de referencia (correr ANTES de traduzir)
    (sem argumento)    confere contra a referencia e falha se algum token caiu
"""
import io, json, os, re, subprocess, sys

VERSAO = "0.4.0"
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF = os.path.join(RAIZ, "docs", "operacao", "traducao-tokens.json")


def params():
    """Nomes de params, lidos do manifest — a fonte, nao uma copia."""
    b = io.open(os.path.join(RAIZ, "src", "manifest.h"),
                encoding="utf-8", errors="replace").read()
    return sorted(set(re.findall(r't\.Add\("([a-z0-9_]+)"', b)))


def comandos():
    """Comandos de consola, lidos do despachante do main.cpp.

    ⚠️ So conta os que tem ESPACO — `fio estado`, `qspi janela`. Os de uma
    palavra (`board`, `list`, `cpu`) ja sao ingles ou neutros e nao correm
    risco de serem traduzidos por engano.
    """
    b = io.open(os.path.join(RAIZ, "src", "main.cpp"),
                encoding="utf-8", errors="replace").read()
    achados = re.findall(r'strncmp\(line,\s*"([a-z0-9]+ [a-z0-9 ]+)"', b)
    return sorted(set(a.strip() for a in achados))


def ficheiros():
    saida = subprocess.run(["git", "ls-files"], cwd=RAIZ,
                           capture_output=True, text=True).stdout
    bons = (".md", ".py", ".cpp", ".h", ".ino", ".sh", ".command", ".txt",
            ".json", ".csv", ".html")
    return [f for f in saida.split("\n")
            if f and f.endswith(bons) and not f.startswith("docs/rev_b/")]


def conta(tokens):
    total = {t: 0 for t in tokens}
    for f in ficheiros():
        try:
            b = io.open(os.path.join(RAIZ, f), encoding="utf-8",
                        errors="replace").read()
        except (IsADirectoryError, FileNotFoundError):
            continue
        for t in tokens:
            if t in b:
                total[t] += b.count(t)
    return total


def main():
    print("traducao_guarda v%s" % VERSAO)

    # 🔴 A LISTA VEM DA REFERENCIA, NAO DA FONTE VIVA.
    #
    # A v0.1.0 lia os tokens do `main.cpp` e do `manifest.h` a cada corrida —
    # e por isso APROVOU o proprio defeito que existia para apanhar: ao
    # renomear `fio estado` para `wire state`, o token sumiu da lista de
    # vigilancia junto com a fonte, e a guarda anunciou "OK".
    #
    # E a familia que o projecto nomeou a 2026-09-29 na QSPI: a sonda que pergunta
    # ao acusado cai junto com ele. A fonte viva so serve para CRIAR a
    # referencia; depois disso, quem manda e a referencia.
    if "--escreve-lista" in sys.argv or not os.path.exists(REF):
        p, c = params(), comandos()
    else:
        ref0 = json.loads(io.open(REF, encoding="utf-8").read())
        p, c = ref0["params"], ref0["comandos"]
    tokens = p + c
    print("  protegidos: %d params + %d comandos de consola = %d tokens"
          % (len(p), len(c), len(tokens)))
    agora = conta(tokens)

    if "--escreve-lista" in sys.argv:
        os.makedirs(os.path.dirname(REF), exist_ok=True)
        io.open(REF, "w", encoding="utf-8").write(
            json.dumps({"versao": VERSAO, "params": p, "comandos": c,
                        "contagem": agora}, indent=1, ensure_ascii=False,
                       sort_keys=True) + "\n")
        print("  referencia gravada: %s" % os.path.relpath(REF, RAIZ))
        print("  (correr sem argumento depois de traduzir, para conferir)")
        return 0

    if not os.path.exists(REF):
        print("  SEM REFERENCIA. Corre --escreve-lista ANTES de traduzir —")
        print("  sem a contagem de antes, nao ha com o que comparar.")
        return 3

    ref = json.loads(io.open(REF, encoding="utf-8").read())
    base = ref["contagem"]
    sumidos, caidos, novos = [], [], []
    for t in tokens:
        a, d = base.get(t, 0), agora[t]
        if a and not d:
            sumidos.append(t)
        elif d < a:
            caidos.append((t, a, d))
    for t in tokens:
        if t not in base:
            novos.append(t)

    # 🔴 A OCORRENCIA QUE CARREGA vs A QUE NARRA.
    #
    # `fio estado` aparece 5 vezes: UMA no despachante do main.cpp, quatro em
    # documentos que a citam. Se a traducao mudar o despachante e deixar os
    # documentos, a contagem na arvore cai de 5 para 4 — um aviso brando — e o
    # comando DEIXA DE EXISTIR. As quatro mencoes em prosa nao o mantem vivo.
    #
    # Por isso o portao duro e por FICHEIRO-FONTE: o comando tem de continuar
    # no `main.cpp`, o param tem de continuar no `manifest.h`. A contagem na
    # arvore fica como sinal brando, para apanhar prosa reescrita.
    fonte_cmd = io.open(os.path.join(RAIZ, "src", "main.cpp"),
                        encoding="utf-8", errors="replace").read()
    fonte_par = io.open(os.path.join(RAIZ, "src", "manifest.h"),
                        encoding="utf-8", errors="replace").read()
    # ⚠️ E O TESTE E DO CONSTRUTO QUE FAZ O COMANDO EXISTIR, nao de uma
    # ocorrencia qualquer do texto. A v0.3.0 procurava a string entre aspas e
    # aceitava `hal::Log("fio estado — QUEM MANDA...")` como prova de que o
    # comando existia: uma mensagem que NARRA o comando satisfez o teste de que
    # ele FUNCIONA. So o `strncmp(line, "...")` o despacha.
    orfaos = []
    despachados = set(re.findall(r'strncmp\(line,\s*"([^"]+)"', fonte_cmd))
    for t in c:
        if not any(d == t or d.startswith(t + " ") for d in despachados):
            orfaos.append(("comando", t, "src/main.cpp (strncmp)"))
    for t in p:
        if ('"%s"' % t) not in fonte_par:
            orfaos.append(("param", t, "src/manifest.h"))
    for tipo, t, onde in orfaos:
        print("  *** SUMIU DA FONTE  %-8s %-24s  (%s)" % (tipo, t, onde))

    for t in sumidos:
        print("  *** DESAPARECEU  %-24s  %d -> 0" % (t, base[t]))
    for t, a, d in caidos:
        print("      caiu         %-24s  %d -> %d" % (t, a, d))
    for t in novos:
        print("      (token novo, nao estava na referencia: %s)" % t)

    if orfaos:
        print("\nFALHOU: %d token(s) sumiram do FICHEIRO QUE OS FAZ EXISTIR."
              % len(orfaos))
        print("Um comando de bancada ou um nome de param deixou de existir —")
        print("as mencoes em prosa nao o mantem vivo.")
        return 1
    if sumidos:
        print("\nFALHOU: %d token(s) protegido(s) DESAPARECERAM da arvore."
              % len(sumidos))
        print("A traducao apagou um comando de bancada ou um nome de param.")
        return 1
    if caidos:
        print("\n%d token(s) com menos ocorrencias. Pode ser legitimo (prosa"
              " reescrita) — CONFERIR UM A UM, nao assumir." % len(caidos))
        return 0
    print("  OK: nenhum token protegido perdido.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
