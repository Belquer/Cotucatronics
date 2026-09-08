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
import subprocess
import sys
import zipfile

VERSAO = "0.4.0"
AQUI = os.path.dirname(os.path.abspath(__file__))
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
SAIDA = os.path.join(AQUI, os.pardir, "fabrica")

# ⚠️ AS CAMADAS SAO EXPLICITAS. A omissao exporta o que o projecto tiver
# guardado, e o que o projecto tem guardado ninguem sabe de cor.
CAMADAS = ("F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,"
           "F.Mask,B.Mask,Edge.Cuts")

GEOMETRIA = ("clearance", "shorting_items", "tracks_crossing", "track_dangling",
             "via_dangling", "copper_edge_clearance", "hole_clearance",
             "holes_co_located", "drill_out_of_range", "annular_width",
             "track_width", "copper_sliver", "malformed_courtyard",
             "invalid_outline", "unresolved_variable")


def mede(pcb):
    """(por tipo, por rede, planos partidos, ilhas orfas)."""
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
    return tipos, por_rede, partidos, orfas


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


def uma(nome, incompleta):
    pcb = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_pcb" % nome)
    sch = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s.kicad_sch" % nome)
    if not os.path.exists(pcb):
        raise SystemExit("*** nao existe %s" % pcb)
    print("\n══ %s ══" % nome)
    tipos, por_rede, partidos, orfas = mede(pcb)

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

    geo = {k: v for k, v in tipos.items() if k in GEOMETRIA}
    if geo:
        recusas.append("geometria: %s" % dict(geo))
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
    corre("pcb", "export", "pos", "-o", os.path.join(dest, "cpl_%s.csv" % nome),
          "--format", "csv", "--units", "mm", "--side", "both", "--exclude-dnp", pcb)
    if os.path.exists(sch):
        corre("sch", "export", "bom", "-o", os.path.join(dest, "bom_%s.csv" % nome),
              "--fields", "Reference,Value,Footprint,QUANTITY",
              "--labels", "Designator,Comment,Footprint,Qty",
              "--group-by", "Value,Footprint", "--exclude-dnp", sch)

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
        f.write("⚠️ O BOM nao tem codigos LCSC: o esquema nao os guarda.\n"
                "   Para montagem, preenche-los a partir de docs/COTUCA_BOM.md.\n")

    print("\n   pacote: %s" % os.path.relpath(zipe, os.path.join(AQUI, os.pardir)))
    print("     %d ficheiros no zip · CPL e BOM ao lado, fora dele" % n)
    print("     commit %s%s" % (commit, "  ⚠️ ARVORE SUJA" if sujo else ""))
    if faltam:
        print("     ⚠️ INCOMPLETA: %d ligacoes por fazer" % faltam)
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
