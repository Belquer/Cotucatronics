#!/usr/bin/env python3
"""versoes_readme v1.0.0 — confere o mapa de ficheiros do README contra os
ficheiros. O `CLAUDE.md` exige que o mapa traga a versao ACTUAL de cada um;
nada cobrava isso, e 15 de 58 tinham derivado.

USO:  versoes_readme.py           relatorio
      versoes_readme.py --write   corrige o README (so as que divergem)
"""
import re, sys, pathlib
print("versoes_readme v1.0.0")
RAIZ = pathlib.Path(__file__).resolve().parents[2]
RD = RAIZ / "README.md"
rd = RD.read_text(encoding="utf-8")

# A ordem importa: a declaracao canonica vem PRIMEIRO. Para o main.cpp e o
# `COTUCA_OS_VERSION`; procurar um `vX.Y.Z` solto num ficheiro de 8000 linhas
# apanha a primeira cicatriz citada, nao a versao.
REGRAS = (r'#define\s+COTUCA_OS_VERSION\s+"([0-9.]+)"',
          r'#define\s+VERSION\s+"([0-9.]+)"',
          r'^VERSION\s*=\s*["\']([0-9.]+)["\']',
          r'^VERSAO\s*=\s*["\']([0-9.]+)["\']',
          # o cabecalho pode trazer tokens pelo meio:
          #   `// manifest.h (instrumento "bancada") v1.12.1`
          r'^(?://|#|\*)[^\n]*?\bv([0-9]+\.[0-9]+\.[0-9]+)',
          r'^#+\s+[^\n]*?\bv([0-9]+\.[0-9]+\.[0-9]+)',
          r'\bv([0-9]+\.[0-9]+\.[0-9]+)')

def real(p):
    t = RAIZ / p
    if not t.is_file(): return None
    s = t.read_text(encoding="utf-8", errors="replace")
    cab = "\n".join(s.splitlines()[:40])          # a versao mora no cabecalho
    for rx in REGRAS:
        m = re.search(rx, cab, re.M) or re.search(rx, s, re.M)
        if m: return m.group(1)
    return None

pat = re.compile(r'(\|\s*\[[^\]]*\]\((?P<p>[^)]+)\)[^|]*\|\s*\*\*v?)(?P<v>[0-9]+\.[0-9]+\.[0-9]+)(\*\*)')
div, ok, ausente, mudo = [], 0, [], []
for m in pat.finditer(rd):
    p, decl = m.group("p"), m.group("v")
    r = real(p)
    if r is None:
        # NUNCA saltar em silencio: ficheiro que o extractor nao le fica
        # POR CONFERIR, e por conferir tem de aparecer no relatorio.
        alvo = RAIZ / p
        if alvo.is_dir(): pass            # pasta: nao carrega versao propria
        elif not alvo.is_file(): ausente.append((p, decl))
        else: mudo.append((p, decl))
        continue
    if r != decl: div.append((p, decl, r))
    else: ok += 1

for p, d, r in div:    print("  DIVERGE  %-42s README=v%-9s ficheiro=v%s" % (p, d, r))
for p, d in ausente:   print("  AUSENTE  %-42s README diz v%s" % (p, d))
for p, d in mudo:      print("  SEM VERSAO LEGIVEL  %-31s README diz v%s" % (p, d))
print("\n  %d batem · %d divergem · %d ausentes · %d por conferir"
      % (ok, len(div), len(ausente), len(mudo)))

if "--write" in sys.argv and div:
    novo = pat.sub(lambda m: (m.group(1) + (real(m.group("p")) or m.group("v")) + m.group(4)), rd)
    RD.write_text(novo, encoding="utf-8")
    print("  README.md reescrito: %d versoes acertadas" % len(div))
sys.exit(1 if div or ausente else 0)
