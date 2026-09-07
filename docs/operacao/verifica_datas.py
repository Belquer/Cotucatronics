#!/usr/bin/env python3
"""verifica_datas v0.1.0 — data e dado gerado, nao escrito.

Toda data que aparece num documento e uma AFIRMACAO sobre quando algo
aconteceu, e a unica testemunha independente disso e o `git`. Este verificador
confronta as duas.

Tres perguntas, e cada uma pode dizer nao:

  FUTURO   — nenhuma data pode ser posterior ao relogio. Data no futuro e
             sempre erro: ou de digitacao, ou de copiar um cabecalho.
  FUTURA   — a data escrita numa linha nao pode ser posterior ao commit que a
             introduziu. E a forma que a deriva toma: escrever "atualizado em X"
             e commitar a X-1, porque a data veio da cabeca e nao do relogio.

⚠️ O QUE **NAO** E ACHADO: uma data ANTERIOR ao commit. Documento que regista
historia diz correctamente "23/08" numa linha escrita a 05/09 — e isso e a
maioria esmagadora. A primeira versao desta guarda tratava-o como divergencia e
devolveu 85 falsos positivos contra 1 achado. Regra que grita no caso normal
treina quem a le a ignora-la.

⚠️ A TOLERANCIA E DE UM DIA, e nao zero: escrever a noite e commitar depois da
meia-noite e legitimo, e um fuso horario chega para o mesmo.

⚠️ E SO OLHA PARA LINHAS QUE O GIT CONHECE. Linha por commitar nao tem
testemunha — e declarada, nao aprovada em silencio.

USO:  verifica_datas.py [caminho ...]        # omissao: docs/
SAI:  0 limpo · 1 divergencias · 2 uso
"""
import io, os, re, subprocess, sys, datetime

VERSAO = "0.2.0"
TOLERANCIA = 1          # dias
ISO = re.compile(r"\b(20\d\d)-(\d\d)-(\d\d)\b")
BR = re.compile(r"\b(\d\d)/(\d\d)(?:/(20\d\d))?\b")


def datas_da_linha(linha, ano_ref):
    """Todas as datas de uma linha, como (texto, date).

    ⚠️ LINHA DE RACIOS NAO TEM DATAS. "params 86/96 · sacrificaveis 10/12" tem
    um `10/12` que passa em qualquer teste de dia e mes — e e uma fraccao. Se a
    linha tiver algum `n/m` com m impossivel como mes, e uma linha de contas e
    nao se le nenhum `DD/MM` nela."""
    out = []
    racios = re.findall(r"\b\d+/(\d+)\b", linha)
    linha_de_contas = any(int(m) > 12 for m in racios)
    for m in ISO.finditer(linha):
        try:
            out.append((m.group(0), datetime.date(int(m.group(1)), int(m.group(2)),
                                                  int(m.group(3)))))
        except ValueError:
            pass
    for m in (BR.finditer(linha) if not linha_de_contas else []):
        d, mes, ano = int(m.group(1)), int(m.group(2)), m.group(3)
        # ⚠️ DD/MM sem ano herda o ano do COMMIT, nao o de hoje: uma linha de
        # Dezembro lida em Janeiro nao passa a ser do ano novo.
        try:
            out.append((m.group(0), datetime.date(int(ano) if ano else ano_ref, mes, d)))
        except ValueError:
            pass
    return out


def blame(caminho):
    """{n_linha: date} do commit que introduziu cada linha."""
    r = subprocess.run(["git", "blame", "--line-porcelain", "--", caminho],
                       capture_output=True, text=True)
    if r.returncode:
        return {}
    out, n, ts = {}, None, None
    for l in r.stdout.split("\n"):
        m = re.match(r"^[0-9a-f]{40} \d+ (\d+)", l)
        if m:
            n = int(m.group(1))
        elif l.startswith("author-time ") and n:
            ts = int(l.split()[1])
        elif l.startswith("\t") and n and ts:
            out[n] = datetime.date.fromtimestamp(ts)
            n = ts = None
    return out


def main():
    hoje = datetime.date.today()
    print("verifica_datas v%s — relogio: %s" % (VERSAO, hoje), file=sys.stderr)
    alvos = sys.argv[1:] or ["docs"]
    fich = []
    for a in alvos:
        if os.path.isdir(a):
            for r, _, fs in os.walk(a):
                if "/.git" in r:
                    continue
                fich += [os.path.join(r, f) for f in fs if f.endswith((".md", ".txt"))]
        elif os.path.isfile(a):
            fich.append(a)
    achados, n_datas, sem_git = [], 0, 0
    for f in sorted(fich):
        b = blame(f)
        try:
            linhas = io.open(f, encoding="utf-8").read().split("\n")
        except UnicodeDecodeError:
            continue
        for i, linha in enumerate(linhas, start=1):
            commit = b.get(i)
            for txt, d in datas_da_linha(linha, (commit or hoje).year):
                n_datas += 1
                if d > hoje:
                    achados.append((f, i, txt, "FUTURO", "depois do relogio (%s)" % hoje, linha))
                    continue
                if not commit:
                    sem_git += 1
                    continue
                delta = (d - commit).days
                if delta > TOLERANCIA:
                    achados.append((f, i, txt, "FUTURA",
                                    "a linha entrou a %s, %d dias antes" % (commit, delta), linha))
    print("  %d datas em %d ficheiros · %d sem testemunha no git"
          % (n_datas, len(fich), sem_git), file=sys.stderr)
    if not achados:
        print("  todas coerentes com o git.", file=sys.stderr)
        return 0
    print("  %d divergencia(s):\n" % len(achados), file=sys.stderr)
    for f, i, txt, tipo, porque, linha in achados:
        print("  %-8s %s:%d  '%s'  %s" % (tipo, f, i, txt, porque))
        print("           %s" % linha.strip()[:96])
    return 1


if __name__ == "__main__":
    sys.exit(main())
