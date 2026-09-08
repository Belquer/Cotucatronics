#!/usr/bin/env python3
"""revisao_externa — a revisao que NAO fui eu que escrevi.

POR QUE EXISTE.

Toda a cadeia desta casa foi escrita por mim: o gerador, o roteador, o portao,
e os proprios verificadores. Isso tem um limite que nao se conserta escrevendo
mais um verificador — **um auto-teste tem exactamente os pontos cegos de quem o
escreveu**, e ja aconteceu aqui: a regra de aceitacao do roteador dizia que a
placa estava boa com a terra em pedacos (15dc).

O `kicad-happy` e de outra pessoa, tem licenca MIT, e foi testado contra 5800
projectos abertos. Nao roteia nem gera — so olha. E olha para coisas que eu
nunca verifiquei porque nunca me ocorreram.

O que ele apanhou nesta placa a primeira corrida, em 0,2 segundos:

  · ZERO FIDUCIAIS com 35 componentes SMD. A montagem automatica precisa de
    tres por face para alinhar, e a nossa cadeia nao tem uma linha sobre isso.
  · os conectores J15 e J16, de 6 pinos, NAO TEM PINO DE TERRA.
  · `AGND` com traco minimo de 0,187 mm — passa o DRC e e fino para um retorno
    analogico.
  · os pinos nao usados da Daisy sem marca de nao-ligado.

⚠️ E ELE DECLARA O QUE NAO VERIFICOU. A auditoria de ciclo de vida sai marcada
como "not performed" com a razao, em vez de faltar em silencio. Um relatorio
que so diz o que correu bem e um relatorio que esconde metade.

⚠️ O QUE ISTO NAO E: nao substitui o `para_fabrica.py`. O portao decide sobre
geometria, planos e ligacoes — coisas que se medem. Isto e uma segunda opiniao
sobre desenho, e as duas servem para coisas diferentes.

USO:  revisao_externa.py <cerebro|painel|todas> [--tudo]

Sem `--tudo`, mostra so ERROR e WARNING. As INFO sao muitas e uteis a ler uma
vez, nao a cada corrida.
"""
import glob
import os
import re
import subprocess
import sys

VERSAO = "0.2.0"
AQUI = os.path.dirname(os.path.abspath(__file__))
CACHE = os.path.expanduser("~/.claude/plugins/cache/kicad-happy/kicad-happy")


def scripts():
    """⚠️ A versao vem do que esta instalado, e nao de um numero que eu escreva
    aqui. Um caminho com a versao a martelo envelhece na primeira actualizacao
    do plugin e ninguem repara ate ele deixar de correr."""
    vs = sorted(glob.glob(os.path.join(CACHE, "*", "skills", "kicad", "scripts")))
    if not vs:
        raise SystemExit(
            "*** o kicad-happy nao esta instalado.\n"
            "    claude plugin marketplace add aklofas/kicad-happy\n"
            "    claude plugin install kicad-happy@kicad-happy")
    return vs[-1]


def corre(script, alvo, extra=()):
    d = scripts()
    r = subprocess.run([sys.executable, os.path.join(d, script), alvo, "--text"]
                       + list(extra), capture_output=True, text=True)
    return r.stdout


# ⚠️ FALSOS POSITIVOS CONHECIDOS — com a razao escrita, e com data.
#
# Um achado silenciado sem razao escrita e um achado apagado. Cada entrada aqui
# tem de dizer PORQUE e falso, e quem verificou. Se a razao deixar de valer, a
# entrada sai — e o achado volta.
#
# ⚠️ E nao se silencia uma familia inteira: silencia-se o par (codigo, rede).
# `VM-001` continua a valer para qualquer outra rede.
SILENCIADOS = {
    ("VM-001", "MIDI_RX"): None,
    ("VM-001", "MIDI_TX"): None,
    ("VM-001", "PONTE_STATUS"): None,
}
PORQUE_VM001 = """08/09: as tres redes ligam U2.15/14/35 (Daisy) a U3.IO21/IO20/IO3
   (ESP32-C3), e os DOIS tem E/S a 3,3 V — verificado no netlist da placa.
   O analisador viu o pino `5V` do modulo ESP32 ligado a +5V_PONTE e deduziu
   dai que o modulo inteiro e de dominio 5 V. Esse pino alimenta o regulador
   de bordo dele; as saidas sao 3,3 V. A logica da ferramenta e boa — um
   modulo com pino de 5 V PARECE mesmo uma peca de 5 V."""


def achados(saida, tudo):
    """(nivel, codigo, texto) de cada achado, e o que ele DECLARA nao ter feito."""
    fora, nao_feito, calados = [], [], []
    for l in saida.splitlines():
        m = re.match(r"\s*\[\d+\]\s+(ERROR|WARNING|INFO)\s+(\S+):\s+(.*)", l)
        if not m:
            continue
        nivel, cod, txt = m.groups()
        if "not run" in txt or "not performed" in txt.lower():
            nao_feito.append(txt)
            continue
        rede = re.search(r"Net (\S+):", txt)
        if not tudo and rede and (cod, rede.group(1)) in SILENCIADOS:
            calados.append((cod, rede.group(1)))
            continue
        if nivel != "INFO" or tudo:
            fora.append((nivel, cod, txt))
    return fora, nao_feito, calados


def uma(nome, tudo):
    base = os.path.join(AQUI, "cotuca_%s" % nome, "cotuca_%s" % nome)
    print("\n══ %s ══" % nome)
    total = 0
    for rotulo, script, alvo in (("esquema", "analyze_schematic.py", base + ".kicad_sch"),
                                 ("placa", "analyze_pcb.py", base + ".kicad_pcb")):
        if not os.path.exists(alvo):
            continue
        extra = ("--stage", "pre_fab") if script == "analyze_pcb.py" else ()
        fora, nao_feito, calados = achados(corre(script, alvo, extra), tudo)
        print("  %s: %d achado(s)" % (rotulo, len(fora)))
        for nivel, cod, txt in sorted(fora, key=lambda x: (x[0] != "ERROR", x[1])):
            print("     %-7s %-8s %s" % (nivel, cod, txt))
        for n in nao_feito:
            print("     ⚠️ nao verificado: %s" % n[:110])
        if calados:
            print("     %d achado(s) silenciados como falso positivo: %s"
                  % (len(calados), " ".join(sorted(r for _c, r in calados))))
            print("       %s" % PORQUE_VM001.replace("\n", "\n    "))
        total += sum(1 for n, _, _ in fora if n == "ERROR")
    return total


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    d = scripts()
    print("revisao_externa v%s — kicad-happy %s (MIT, de Andrew Klofas)"
          % (VERSAO, os.path.basename(os.path.dirname(os.path.dirname(os.path.dirname(d))))))
    tudo = "--tudo" in sys.argv
    erros = sum(uma(q, tudo) for q in
                (["cerebro", "painel"] if sys.argv[1] == "todas" else [sys.argv[1]]))
    print("\n⚠️ Isto e uma segunda opiniao sobre DESENHO. Quem decide sobre "
          "geometria,\n   planos e ligacoes continua a ser o `para_fabrica.py`.")
    return 1 if erros else 0


if __name__ == "__main__":
    sys.exit(main())
