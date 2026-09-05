#!/usr/bin/env python3
"""coerencia v1.1.0 — as checagens que o grep nao faz direito.

Substitui as seccoes 2 e 3 do `auditar.sh`, que estavam ERRADAS: contavam
ocorrencias simples quando o `check_docs` so ve nomes ENTRE CRASES, e
supunham escopo de linha quando `isentos()` varre o FICHEIRO INTEIRO.
Verificador que acusa pelo criterio errado e a classe 10 da skill `audit`.

Acrescenta a passada de ITENS PENDENTES.

USO:  coerencia.py            relatorio
      coerencia.py --pendentes  so a passada de pendentes
"""
import re, sys, pathlib
print("coerencia v1.1.0")
RAIZ = pathlib.Path(__file__).resolve().parents[2]
SRC  = RAIZ / "src"
DOCS = [RAIZ/"README.md"] + sorted((RAIZ/"docs").rglob("*.md"))

def ler(p):
    try: return p.read_text(encoding="utf-8", errors="replace")
    except Exception: return ""

# --- o vocabulario legitimo, pelas MESMAS regras do check_docs -------------
manif  = ler(SRC/"manifest.h")
params = set(re.findall(r'Add\("([a-z0-9_]+)"', manif))
familias = {re.sub(r'_[ab]$', '', p) for p in params}
simb = set()
for p in list(SRC.glob("*.cpp")) + list(SRC.glob("*.h")):
    simb |= set(re.findall(r'\b([a-z][a-z0-9_]{2,})\b', ler(p)))
APOS_F = RAIZ/"docs/params-aposentados.txt"
apos = [l for l in ler(APOS_F).splitlines() if l.strip() and not l.startswith("#")]
apos_n = [l.split()[0] for l in apos]

CRASE = re.compile(r'`([a-z][a-z0-9]*(?:_[a-z0-9]+)+)`')
IGN   = re.compile(r'<!--\s*doc-check:\s*ignore\s+([a-z_0-9]+)')

achados = 0
def diz(s):
    global achados; print("  " + s); achados += 1

if "--pendentes" not in sys.argv:
    print("\n== ISENCOES QUE NAO COBREM NADA ==")
    print("   (criterio certo: o nome tem de aparecer ENTRE CRASES no corpo)")
    for d in DOCS:
        t = ler(d)
        corpo = "\n".join(l for l in t.splitlines() if "doc-check: ignore" not in l)
        usados = set(CRASE.findall(corpo))
        for nome in sorted(set(IGN.findall(t))):
            if nome in usados: continue
            outra = (nome in params or nome in familias or nome in simb or nome in apos_n)
            porque = "e ja coberto por param/simbolo/aposentado" if outra else "e nao esta coberto por mais nada"
            diz("%s: isenta '%s', que nao aparece em crase no corpo %s"
                % (d.relative_to(RAIZ), nome, porque))

    print("\n== LISTA DE APOSENTADOS: SAUDE ==")
    vivos = [n for n in apos_n if n in params]
    for n in vivos:
        diz("*** '%s' esta na lista de aposentados E VIVO no manifest — "
            "o verificador deixou de o vigiar" % n)
    dup = sorted({n for n in apos_n if apos_n.count(n) > 1})
    for n in dup: diz("duplicado na lista: '%s'" % n)
    # orfao NAO e defeito: a lista e um registo. So se conta.
    citados = set()
    for d in DOCS:
        if d.name == "params-aposentados.txt": continue
        citados |= set(CRASE.findall(ler(d)))
    orf = [n for n in apos_n if n not in citados]
    print("  %d entradas · %d vivas no manifest · %d duplicadas · %d nao citadas "
          "(nao e defeito: a lista e registo)" % (len(apos_n), len(vivos), len(dup), len(orf)))

# --- ITENS PENDENTES -------------------------------------------------------
print("\n== ITENS PENDENTES (o que a arvore declara por fazer) ==")
# ALTA PRECISAO. `\bTODO\b` com re.I casa o portugues "todo" — a passada
# nascia com ~200 falsos positivos e isso e ruido a fingir de achado.
# TODO/FIXME sao MAIUSCULAS-so; as frases sao insensiveis.
# ⚠️ NAO conta como tarefa: neste projecto e quase sempre CUIDADO
# PERMANENTE ("o endereco e posicional, regerar depois"), nao 'por fazer'.
# Somar as duas dava 190 quando as tarefas a serio sao muito menos —
# numero inflacionado e a mesma familia do verificador que berra.
MARCA_CS = re.compile(r'(🔴|🟡|🟢|⚪|⬜|⏳|\bTODO\b|\bFIXME\b|\bXXX\b|\bHACK\b)')
AVISO_CS = re.compile(r'⚠️')
MARCA_CI = re.compile(
    r'(por fazer|por confirmar|por medir|por verificar|por limpar|por soldar|'
    r'em aberto|ficou por|fica por|falta(?:m)? (?:ainda|fazer|medir|soldar)|'
    r'\\bpend[êe]ncia|aceite pendente|n[ãa]o foi validad|n[ãa]o validad|'
    r'por decidir|a confirmar|por rodar)', re.I)
def MARCAS_search(l):
    return MARCA_CS.search(l) or MARCA_CI.search(l)
tot = avisos_n = 0
for d in DOCS:
    hits = []
    for i, l in enumerate(ler(d).splitlines(), 1):
        if "doc-check" in l: continue
        if MARCAS_search(l):
            s = re.sub(r'\s+', ' ', l.strip())
            hits.append((i, s[:104]))
        elif AVISO_CS.search(l):
            avisos_n += 1        # cuidado permanente, nao tarefa
    if hits:
        print("  -- %s (%d)" % (d.relative_to(RAIZ), len(hits)))
        for i, s in hits[:6]: print("     %5d  %s" % (i, s))
        if len(hits) > 6: print("     ... mais %d" % (len(hits)-6))
        tot += len(hits)
print("  TOTAL: %d tarefas declaradas · %d avisos ⚠️ (cuidado permanente,\n         nao tarefa — contados a parte de proposito)" % (tot, avisos_n))
sys.exit(1 if achados else 0)
