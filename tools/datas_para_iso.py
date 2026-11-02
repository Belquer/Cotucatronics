#!/usr/bin/env python3
"""datas_para_iso v0.2.0 — passa DD/MM para ISO 8601, sem partir fraccoes.

    tools/datas_para_iso.py [--escreve] [ficheiro ...]

POR QUE EXISTE. O repositorio escreve datas como `04/09`, que um leitor de
ingles le como 9 de Abril. O dB pediu formato inequivoco (02/11/2026), e a
escolha e **ISO 8601** por tres razoes medidas, nao por gosto:

  1. o repositorio ja usa ISO 106 vezes;
  2. o `verifica_datas.py` ja o le (`ISO = \\b(20\\d\\d)-(\\d\\d)-(\\d\\d)\\b`),
     portanto mudar de formato nao cega o verificador que vigia as datas;
  3. ordena lexicograficamente.

🔴 POR QUE UM `sed` SERIA DESTRUTIVO. Sao 1 293 ocorrencias, e o repositorio
esta cheio de `n/m` que NAO sao datas: `19/71` ligacoes por rotear, `15/78`
figuras, `10/12` sacrificaveis, `36/84` MPNs. Uma substituicao cega
transformaria contagens em datas e ninguem daria por isso — o numero continua
a ser um numero.

A LOGICA E A DO `verifica_datas.py`, nao uma nova:

  · o que esta entre CRASES fica — data citada e literal, nao afirmacao;
  · linha com algum `n/m` de `m > 12` e LINHA DE CONTAS e nao se le data nela;
  · o ANO vem do `git blame` da linha, nunca do relogio — uma linha de Dezembro
    lida em Janeiro nao passa a ser do ano novo.

⚠️ E RECUSA EM VEZ DE ADIVINHAR. Toda candidata fora da janela de vida do
projecto sai na lista de SUSPEITAS e nao e tocada: o projecto comecou em
Agosto de 2026, portanto `03/04` e quase de certeza uma fraccao e nao 3 de
Abril. Converter o que nao se sabe e pior que deixar ambiguo.
"""
import io, os, re, subprocess, sys, datetime, collections

VERSAO = "0.2.0"
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BR = re.compile(r"\b(\d\d)/(\d\d)(?:/(20\d\d))?\b")
MESES_VIVOS = {7, 8, 9, 10, 11, 12}      # o projecto vive de Julho/2026 em diante


def blame_anos(caminho):
    r = subprocess.run(["git", "blame", "--line-porcelain", "--", caminho],
                       cwd=RAIZ, capture_output=True, text=True)
    if r.returncode:
        return {}
    out, n = {}, None
    for l in r.stdout.split("\n"):
        m = re.match(r"^[0-9a-f]{40} \d+ (\d+)", l)
        if m:
            n = int(m.group(1))
        elif l.startswith("author-time ") and n:
            out[n] = datetime.date.fromtimestamp(int(l.split()[1])).year
            n = None
    return out


def mascara_crases(linha):
    """Devolve a linha com o conteudo entre crases substituido por sentinelas,
    para que as posicoes nao se desloquem."""
    return re.sub(r"`[^`]*`", lambda m: "\x00" * len(m.group(0)), linha)


def processa(caminho, escreve):
    try:
        bruto = io.open(os.path.join(RAIZ, caminho), encoding="utf-8").read()
    except (UnicodeDecodeError, IsADirectoryError, FileNotFoundError):
        return [], [], 0
    anos = blame_anos(caminho)
    linhas = bruto.split("\n")
    feitas, suspeitas, n = [], [], 0
    for i, linha in enumerate(linhas, 1):
        limpa = mascara_crases(linha)
        racios = re.findall(r"\b\d+/(\d+)\b", limpa)
        if any(int(x) > 12 for x in racios):
            continue                                  # linha de contas
        nova, desloc = linha, 0
        for m in BR.finditer(limpa):
            d, mes, ano = int(m.group(1)), int(m.group(2)), m.group(3)
            # 🔴 VIRGULA DECIMAL FAZ DA CANDIDATA UMA DIVISAO, NAO UMA DATA.
            # Em `(1,27/10,16)` o `\b` cai depois da virgula e o `27/10` casa —
            # e e 1,27 a dividir por 10,16. O filtro de meses nao o apanha
            # porque Outubro esta dentro da janela de vida do projecto.
            # Apanhado a olho em kicad/roteia.py:106-107, duas ocorrencias que
            # teriam virado `2026-10-27` no meio de uma conta de passo de malha.
            antes2 = limpa[max(0, m.start() - 2):m.start()]
            depois2 = limpa[m.end():m.end() + 2]
            if re.match(r"\d[,.]$", antes2) or re.match(r"^[,.]\d", depois2):
                suspeitas.append((caminho, i, m.group(0) + " (virgula decimal)",
                                  linha.strip()[:90]))
                continue
            if mes not in MESES_VIVOS or not (1 <= d <= 31):
                suspeitas.append((caminho, i, m.group(0), linha.strip()[:90]))
                continue
            a = int(ano) if ano else anos.get(i)
            if a is None:
                suspeitas.append((caminho, i, m.group(0) + " (sem testemunha)",
                                  linha.strip()[:90]))
                continue
            try:
                iso = datetime.date(a, mes, d).isoformat()
            except ValueError:
                suspeitas.append((caminho, i, m.group(0) + " (data impossivel)",
                                  linha.strip()[:90]))
                continue
            ini, fim = m.start() + desloc, m.end() + desloc
            nova = nova[:ini] + iso + nova[fim:]
            desloc += len(iso) - (fim - ini)
            feitas.append((caminho, i, m.group(0), iso))
            n += 1
        linhas[i - 1] = nova
    if escreve and n:
        io.open(os.path.join(RAIZ, caminho), "w",
                encoding="utf-8").write("\n".join(linhas))
    return feitas, suspeitas, n


def main():
    print("datas_para_iso v%s" % VERSAO)
    escreve = "--escreve" in sys.argv
    alvos = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not alvos:
        saida = subprocess.run(["git", "ls-files"], cwd=RAIZ,
                               capture_output=True, text=True).stdout
        bons = (".md", ".py", ".cpp", ".h", ".ino", ".sh", ".command", ".txt")
        alvos = [f for f in saida.split("\n") if f and f.endswith(bons)]
    tot, todas, susp = 0, [], []
    for f in alvos:
        feitas, s, n = processa(f, escreve)
        tot += n
        todas += feitas
        susp += s
    por_f = collections.Counter(c for c, _, _, _ in todas)
    print("  %s %d data(s) em %d ficheiro(s)"
          % ("convertidas:" if escreve else "a converter:", tot, len(por_f)))
    for c, k in por_f.most_common(10):
        print("    %-52s %d" % (c, k))
    if len(por_f) > 10:
        print("    ... e mais %d ficheiro(s)" % (len(por_f) - 10))
    if susp:
        print("\n  ⚠️ %d SUSPEITA(S) — nao tocadas, conferir a mao:" % len(susp))
        for c, i, t, l in susp[:25]:
            print("    %s:%d  %-22s %s" % (c, i, t, l))
        if len(susp) > 25:
            print("    ... e mais %d" % (len(susp) - 25))
    if not escreve:
        print("\n  (ensaio. `--escreve` aplica)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
