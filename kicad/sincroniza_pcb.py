#!/usr/bin/env python3
"""sincroniza_pcb v0.1.0 — poe no PCB as pecas que o esquema tem e ele nao.

POR QUE EXISTE. O `Update PCB from Schematic` (F8) do KiCad e GUI e nao tem
comando. Neste projecto os esquemas sao GERADOS: cada regeneracao muda os UUID,
e o F8 passa a ser um passo manual, repetido, num sitio onde a maquina ja faz
tudo o resto. Falhou duas vezes seguidas — uma por a biblioteca ter nascido com
o editor aberto, outra sem causa que se visse do lado de ca — e cada falha
custou uma ida e volta e um susto de "a placa parece mais vazia".

⚠️ O QUE ELE FAZ, e so isto: acrescenta footprints que faltam, tira os que
sobram, e poe o nome da rede em cada pad, lido do NETLIST. Nao roteia, nao
move nada do que ja la esta, e nao toca em pistas.

⚠️ E VERIFICA-SE ANTES DE ESCREVER: conta o que ficou, compara com o netlist, e
recusa-se a gravar se nao bater. Um sincronizador que erra em silencio e pior
que um passo manual — o passo manual pelo menos falha a vista.

⚠️ `--zero` ESVAZIA A PLACA ANTES DE SINCRONIZAR. Tudo o que existir de
footprints e apagado e reposto a partir da netlist. Existe porque um F8 falhado
deixa a placa num estado que ninguem consegue auditar — copias a mais, pecas
que ja nao estao no esquema, posicoes largadas onde o KiCad calhou — e porque
NESTE projecto isso nao custa nada: a colocacao inteira vive no
`pcb_contorno.py`, logo repor e correr um comando.

NAO apaga pistas nem zonas. Se a placa ja estiver routeada, `--zero` deixa o
cobre orfao — usar so antes de rotear.

USO:  sincroniza_pcb.py <projecto> [--zero]
"""
import io, os, re, subprocess, sys, uuid

VERSAO = "0.3.1"
AQUI = os.path.dirname(os.path.abspath(__file__))
KICAD = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"
# ⚠️ DUAS RAIZES DE BIBLIOTECA: a do projecto (`cotuca.pretty`, ao lado deste
# ficheiro) e a instalada com o KiCad. Procurar so na primeira fazia o `--zero`
# esvaziar a placa e nao a conseguir repor — apaguei as duas placas antes de ter
# provado o caminho de volta, que e a ordem errada de fazer as coisas.
RAIZES = [AQUI,
          "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints"]
NUM = r"-?\d+(?:\.\d+)?"
PROJ = {"cerebro": "cotuca_cerebro", "painel": "cotuca_painel"}


def blocos(t, chave):
    """Cada bloco `(chave ...)` de topo, com os indices."""
    out = []
    for m in re.finditer(r"\n\t\(%s[ \"]" % chave, t):
        j = m.start() + 1
        d = 0
        for k in range(j, len(t)):
            if t[k] == "(":
                d += 1
            elif t[k] == ")":
                d -= 1
                if d == 0:
                    out.append((j, k + 1, t[j:k + 1]))
                    break
    return out


def le_netlist(sch):
    r = subprocess.run([KICAD, "sch", "export", "netlist", "--format",
                        "kicadsexpr", "-o", "/tmp/_sync.net", sch],
                       capture_output=True, text=True)
    if r.returncode:
        raise SystemExit("*** kicad-cli falhou: %s" % r.stderr.strip()[:200])
    t = io.open("/tmp/_sync.net", encoding="utf-8").read()
    comps = {}
    for m in re.finditer(r'\(comp\s+\(ref "([^"]+)"\)(.*?)\n\t\t\)', t, re.S):
        ref, b = m.group(1), m.group(2)
        fp = re.search(r'\(footprint "([^"]+)"\)', b)
        val = re.search(r'\(value "([^"]*)"\)', b)
        comps[ref] = dict(fp=fp.group(1) if fp else None,
                          val=val.group(1) if val else "")
    redes = {}
    for m in re.finditer(r'\(net\s+\(code "?\d+"?\)\s+\(name "([^"]+)"\)(.*?)'
                         r'(?=\n\t\t\(net|\n\t\)\s*\n)', t, re.S):
        nome, corpo = m.group(1), m.group(2)
        for n in re.finditer(r'\(ref "([^"]+)"\)\s*\(pin "([^"]+)"\)', corpo):
            redes[(n.group(1), n.group(2))] = nome
    return comps, redes


def do_modulo(fp, ref, val, redes):
    """Le o .kicad_mod e devolve o bloco pronto a entrar no PCB."""
    lib, _, nome = fp.partition(":")
    for raiz in RAIZES:
        caminho = os.path.join(raiz, lib + ".pretty", nome + ".kicad_mod")
        if os.path.exists(caminho):
            break
    else:
        raise SystemExit("*** %s: nao achei o footprint '%s' em raiz nenhuma:\n"
                         "    %s" % (ref, fp, "\n    ".join(RAIZES)))
    t = io.open(caminho, encoding="utf-8").read().strip()
    t = re.sub(r'^\(footprint "[^"]+"', '(footprint "%s"' % fp, t, count=1)
    for chave in ("version", "generator", "generator_version"):
        t = re.sub(r'\n\t\(%s [^\n]*\)' % chave, '', t)
    # nome da rede em cada pad
    def rede(m):
        pad = m.group(1)
        r = redes.get((ref, pad))
        return m.group(0) if r is None else m.group(0) + '\n\t\t\t(net "%s")' % r
    t = re.sub(r'\(pad "([^"]+)"[\s\S]*?\(layers [^\)]*\)', rede, t)
    # ⚠️ O .kicad_mod TRAZ AS SUAS PROPRIAS `Reference` ("REF**") e `Value`. Se
    # eu so acrescentar as minhas, o bloco fica com DUAS de cada e o KiCad fica
    # com a ultima — a peca aparece como REF** e nada a encontra pelo nome. Tem
    # de se tirar as do modulo, nao empilhar por cima.
    for prop in ("Reference", "Value"):
        m = re.search(r'\n\t\(property "%s" "[^"]*"' % prop, t)
        if m:
            j = m.start() + 1
            d = 0
            for k in range(j, len(t)):
                if t[k] == "(":
                    d += 1
                elif t[k] == ")":
                    d -= 1
                    if d == 0:
                        t = t[:j - 1] + t[k + 1:]
                        break
    props = ""
    for p, v, lay, dy in (("Reference", ref, "F.SilkS", -11.5),
                          ("Value", val, "F.Fab", 11.5)):
        props += ('\t\t(property "%s" "%s"\n\t\t\t(at 0 %g 0)\n\t\t\t(layer "%s")\n'
                  '\t\t\t(uuid "%s")\n\t\t\t(effects (font (size 1 1) (thickness 0.15)))\n'
                  '\t\t)\n' % (p, v, dy, lay, uuid.uuid4()))
    t = t.replace('(footprint "%s"\n' % fp,
                  '(footprint "%s"\n\t\t(layer "F.Cu")\n\t\t(uuid "%s")\n'
                  '\t\t(at 0 0)\n%s' % (fp, uuid.uuid4(), props), 1)
    return "\t" + t.replace("\n", "\n\t") + "\n"


def main():
    print("sincroniza_pcb v%s" % VERSAO, file=sys.stderr)
    if not (2 <= len(sys.argv) <= 3) or sys.argv[1] not in PROJ:
        print(__doc__)
        raise SystemExit(2)
    d = PROJ[sys.argv[1]]
    zero = "--zero" in sys.argv
    base = os.path.join(AQUI, d, d)
    comps, redes = le_netlist(base + ".kicad_sch")
    p = base + ".kicad_pcb"
    t = io.open(p, encoding="utf-8").read()
    # ⚠️ DUPLICADOS PRIMEIRO. O F8 casa por UUID, e neste projecto os esquemas
    # sao GERADOS: cada regeneracao muda todos os UUID, o KiCad deixa de
    # reconhecer o que ja la esta e ACRESCENTA uma segunda copia ao lado. Ficam
    # duas de cada peca, uma na posicao boa e outra onde ele a largou, e o DRC
    # explode em curtos que nao existem no esquema.
    #
    # Isto nao e um acidente do F8: e o que ele tem de fazer quando o UUID nao
    # bate. Quem gera esquemas paga esta conta sempre — ou deixa de usar o F8.
    if zero:
        n_fora = 0
        while True:
            fps = blocos(t, "footprint")
            if not fps:
                break
            i, j, _ = fps[0]
            t = t[:i] + t[j:]
            n_fora += 1
        print("  --zero: %d footprints apagados; a repor da netlist" % n_fora,
              file=sys.stderr)
        if re.search(r'\n\t\((?:segment|via|zone)\b', t):
            print("  ⚠️ ha pistas ou zonas no ficheiro e as pecas foram apagadas: "
                  "esse cobre ficou orfao.", file=sys.stderr)
    n_dup = 0
    while True:
        fps = blocos(t, "footprint")
        vistos, dup = {}, None
        for i, j, bl in fps:
            r = re.search(r'\(property "Reference" "([^"]+)"', bl)
            if not r:
                continue
            ref = r.group(1)
            if ref in vistos:
                dup = (ref, i, j)
                break
            vistos[ref] = (i, j, bl)
        if not dup:
            break
        ref, i, j = dup
        t = t[:i] + t[j:]
        n_dup += 1
        print("     ~ %s: copia a mais removida" % ref, file=sys.stderr)
    fps = blocos(t, "footprint")
    na_placa = {}
    for i, j, bl in fps:
        r = re.search(r'\(property "Reference" "([^"]+)"', bl)
        if r:
            na_placa[r.group(1)] = (i, j, bl)
    faltam = sorted(set(comps) - set(na_placa))
    sobram = sorted(set(na_placa) - set(comps))
    print("  netlist %d · placa %d · faltam %d · sobram %d"
          % (len(comps), len(na_placa), len(faltam), len(sobram)), file=sys.stderr)
    # ⚠️ "faltam 0 e sobram 0" NAO E "nada a fazer": as copias a mais ja foram
    # tiradas acima, e sair aqui deitava fora esse trabalho sem dizer nada. Foi
    # o que aconteceu — o de-duplicador correu, anunciou 9 remocoes, e o
    # ficheiro ficou igual. Uma saida antecipada tem de olhar para TUDO o que a
    # funcao ja fez, e nao so para a ultima coisa que mediu.
    if not faltam and not sobram and not n_dup and not zero:
        print("  ja estao sincronizados", file=sys.stderr)
        return
    for r in sobram:
        i, j, bl = na_placa[r]
        t = t[:i] + t[j:]
        print("     - %s (sai: nao esta no esquema)" % r, file=sys.stderr)
        fps = blocos(t, "footprint")
        na_placa = {}
        for a, b, c in fps:
            m = re.search(r'\(property "Reference" "([^"]+)"', c)
            if m:
                na_placa[m.group(1)] = (a, b, c)
    novos = ""
    for r in faltam:
        novos += do_modulo(comps[r]["fp"], r, comps[r]["val"], redes)
        print("     + %s  %s" % (r, comps[r]["fp"]), file=sys.stderr)
    k = t.rstrip().rfind(")")
    t = t[:k] + novos + t[k:]
    # ⚠️ conferir ANTES de gravar
    fin = {}
    for i, j, bl in blocos(t, "footprint"):
        m = re.search(r'\(property "Reference" "([^"]+)"', bl)
        if m:
            fin[m.group(1)] = bl
    if set(fin) != set(comps):
        raise SystemExit("*** depois de sincronizar a placa tem %d refs e o "
                         "esquema %d. Nao gravo.\n    so na placa: %s\n    so no "
                         "esquema: %s" % (len(fin), len(comps),
                                          sorted(set(fin) - set(comps)),
                                          sorted(set(comps) - set(fin))))
    io.open(p, "w", encoding="utf-8").write(t)
    print("  gravado: %d footprints, iguais aos %d do esquema"
          % (len(fin), len(comps)), file=sys.stderr)


if __name__ == "__main__":
    main()
