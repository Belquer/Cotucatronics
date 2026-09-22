#!/usr/bin/env python3
"""colisoes_manual v0.1.0 — o portao de sobreposicao para TODAS as figuras do manual.

    python3 tools/colisoes_manual.py [--lista]

POR QUE EXISTE. A `design-review` manda passar cada imagem pelo
`svg_colisoes.py` antes do olho, e isso vinha sendo feito **so nos desenhos
novos**. Os 78 SVGs do manual sao gerados em bloco pelo `manual_montagem.py`:
ninguem os ia buscar um a um, e por isso a divida ficou anos-luz mais velha
que a regra — 15 figuras com colisao, declaradas no STATUS e nunca consertadas.

> **Regra que nao tem portao e conselho.** Isto e o portao.

(!) E O ESTILO TEM DE IR JUNTO, que foi o que enganou a primeira medicao.
Os `<svg>` do manual nao trazem `<style>`: as classes (`.rot`, `.marca-rot`)
vivem na folha do documento. O verificador le `font-size` E `text-anchor` do
CSS por classe — sem o estilo embutido ele assume `start` e 12 px, mede caixas
deslocadas de meia largura, e reporta colisoes que nao existem enquanto cala
outras que existem. Extrair o SVG sozinho NAO e medir o que se desenha.

Saidas: 0 = todas limpas · 1 = ha colisoes · 2 = erro de uso · 3 = nao verificavel.
"""
import os
import re
import subprocess
import sys
import tempfile

VERSAO = "0.1.0"
AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
MANUAL = os.path.join(RAIZ, "docs", "montagem", "manual_montagem.html")
VERIF = os.path.join(AQUI, "svg_colisoes.py")


def estilo(h):
    """O <style> do documento, para ir dentro de cada figura."""
    m = re.search(r"<style>(.*?)</style>", h, re.S)
    if not m:
        sys.exit("RECUSO: o manual nao tem <style> — sem ele o verificador\n"
                 "mede caixas erradas, e medicao errada nao aprova nada.")
    return m.group(1)


def figuras(h):
    """Cada <svg> com o nome que o `aria-label` lhe da."""
    out = []
    for m in re.finditer(r"<svg\b.*?</svg>", h, re.S):
        s = m.group(0)
        a = re.search(r'aria-label="([^"]*)"', s)
        out.append((a.group(1) if a else "(sem aria-label)", s))
    return out


def main():
    print("colisoes_manual v%s" % VERSAO)
    if not os.path.exists(MANUAL):
        sys.exit("RECUSO: %s nao existe. Corra o manual_montagem.py primeiro."
                 % os.path.relpath(MANUAL, RAIZ))
    h = open(MANUAL, encoding="utf-8").read()
    css = estilo(h)
    figs = figuras(h)
    if not figs:
        print("  NAO VERIFICAVEL: zero figuras no manual — nao foi olhado.")
        return 3

    maus, ilegiveis, linhas = 0, 0, []
    with tempfile.TemporaryDirectory() as d:
        for i, (nome, svg) in enumerate(figs):
            # o estilo entra logo a seguir ao <svg ...>
            comp = re.sub(r"(<svg\b[^>]*>)", r"\1<style>%s</style>" % css,
                          svg, count=1)
            f = os.path.join(d, "fig%03d.svg" % i)
            open(f, "w", encoding="utf-8").write(comp)
            p = subprocess.run([sys.executable, VERIF, f],
                               capture_output=True, text=True)
            cab = (p.stdout or "").strip().splitlines()
            resumo = cab[0].split(": ", 1)[-1] if cab else "(sem saida)"
            if p.returncode == 0:
                continue
            if p.returncode == 3:
                ilegiveis += 1
            else:
                maus += 1
            linhas.append("  fig%03d  %s" % (i, nome))
            linhas.append("          %s" % resumo)
            for l in cab[1:6]:
                linhas.append("      %s" % l.strip())

    print("  %d figuras · %d com colisao · %d nao verificavel(is)"
          % (len(figs), maus, ilegiveis))
    if linhas:
        print("\n".join(linhas))
    if ilegiveis:
        print("\n  NAO VERIFICAVEL nao e reprovacao nem aprovacao: e a"
              " ferramenta a dizer que nao conseguiu olhar.")
        return 3
    if maus:
        print("\n  FALHOU: sobreposicao e pergunta de geometria, tem resposta"
              " exacta, e o olho aprova desenho errado. Conserte o GERADOR.")
        return 1
    print("  ✓ todas as figuras do manual passam o portao")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
