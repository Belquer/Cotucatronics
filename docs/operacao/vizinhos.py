#!/usr/bin/env python3
"""vizinhos v1.0.0 — caca FRASE PARTIDA PELA RENOMEACAO.

NASCEU DE UM CASO REAL (04/09). Renomeei uma frase no hal_daisy_seed.cpp
com ancora unica e verificada. A ancora casou uma vez, o grep confirmou o
texto novo, o build passou. E a frase ficou assim:

    // ... "a serial morre sob callback
    // ligado": quem morria era o LOOP

porque a CONTINUACAO estava na linha seguinte, que ninguem tocou.

> A ancora unica prova que o casamento foi UNICO. NAO prova que a frase
> continua a LER-SE. Sao garantias diferentes, e so a primeira e mecanica.

Esta ferramenta le o diff e assinala toda linha alterada cuja VIZINHA
seguinte parece continuacao: comeca em minuscula, ou em aspas/parentese
de fecho, ou em pontuacao de continuacao. Nao decide — manda ler.

USO:  vizinhos.py            diff da arvore de trabalho
      vizinhos.py --staged   diff do que esta em stage
      vizinhos.py <rev>      diff contra uma revisao
"""
import re, subprocess, sys
print("vizinhos v1.0.0")

args = sys.argv[1:]
# --termo X aperta a rede: numa remocao de nome, o risco concentra-se onde
# a CAUDA REMOVIDA continha o nome — a continuacao foi escrita a concordar
# com ele. Foi exactamente assim que se partiu a frase de 04/09.
TERMO = None
for a in list(args):
    if a.startswith("--termo="): TERMO = a.split("=", 1)[1]; args.remove(a)
cmd = ["git", "diff", "-U2"]
if "--staged" in args: cmd.append("--staged"); args.remove("--staged")
if args: cmd.append(args[0])

try:
    diff = subprocess.run(cmd, capture_output=True, text=True, check=True).stdout
except subprocess.CalledProcessError as e:
    print("ERRO a correr git diff:", e); sys.exit(2)

# comeco de linha que cheira a CONTINUACAO de uma frase da linha anterior
CONT = re.compile(r'^\s*(?://+|#|\*|>)?\s*'          # marcador de comentario
                  r'(?:[a-zà-ÿ]|["\'”\)\]]|[,;:—–-])')  # minuscula, fecho, pontuacao
# excepcoes: linha que e claramente item de lista ou tabela nao e continuacao
NAO = re.compile(r'^\s*(?://+|#)?\s*(?:[-*+]\s|\||\d+\.\s)')

def cauda_comum(a, b):
    """quantos caracteres de FIM as duas linhas partilham"""
    n = 0
    while n < min(len(a), len(b)) and a[-1 - n] == b[-1 - n]: n += 1
    return n

# O DISCRIMINADOR. Uma renomeacao no MEIO da linha nao ameaca a
# continuacao: a cauda fica igual e a frase reata na linha seguinte tal
# como reatava. So ha risco quando a edicao chega ao FIM da linha.
# Sem isto a ferramenta assinalava 211 vizinhancas — ruido a fingir de
# achado, que e a classe 10 aplicada a mim proprio.
CAUDA_SEGURA = 12

ficheiro = None
achados = 0
linhas = diff.split("\n")
for i, l in enumerate(linhas):
    if l.startswith("+++ b/"): ficheiro = l[6:]; continue
    if not l.startswith("+") or l.startswith("+++"): continue
    novo = l[1:].rstrip()
    # a linha `-` correspondente, se houver, e o que estiver logo acima
    velho = None
    for k in range(i - 1, max(i - 6, 0), -1):
        if linhas[k].startswith("-") and not linhas[k].startswith("---"):
            velho = linhas[k][1:].rstrip(); break
        if linhas[k].startswith("+"): continue
        break
    if velho is None: continue                 # linha nova pura: sem cauda a partir
    nc = cauda_comum(velho, novo)
    if nc >= CAUDA_SEGURA: continue            # fim intacto
    if TERMO and TERMO.lower() not in velho[len(velho) - nc - 40:].lower():
        continue                               # a cauda removida nao tinha o termo
    for j in range(i + 1, min(i + 3, len(linhas))):
        v = linhas[j]
        if not v or v[0] not in " +": break
        corpo = v[1:]
        if not corpo.strip() or NAO.match(corpo): break
        if CONT.match(corpo):
            achados += 1
            print("\n  %s" % ficheiro)
            print("    antes     | %s" % velho[-90:])
            print("    depois    | %s" % novo[-90:])
            print("    vizinha   | %s" % corpo.rstrip()[:96])
            print("              ^ a edicao tocou o FIM da linha e a vizinha")
            print("                continua a frase. Ainda le-se?")
        break

print("\n  %d vizinhanca(s) a conferir%s."
      % (achados, " (filtrado por '%s')" % TERMO if TERMO else ""))
print("  Nao e defeito: e o sitio onde uma renomeacao parte a frase sem")
print("  que ancora, grep ou compilador digam nada.")
sys.exit(0)
