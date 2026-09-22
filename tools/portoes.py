#!/usr/bin/env python3
"""portoes v0.1.0 — corre TODOS os portoes de leitura, num comando.

    python3 tools/portoes.py [--rapido]

POR QUE EXISTE, e a razao e uma cicatriz de 22/09.

Este repositorio tem 48 ferramentas. Varias sao portoes de verdade: sabem
dizer nao, e algumas provam-no com `--sabota`. **Nenhuma era corrida em
conjunto.** O resultado mediu-se nesse dia:

  · 15 das 78 figuras do manual com sobreposicao, semanas a fio — o
    `svg_colisoes.py` existia e passava-se so pelos desenhos NOVOS;
  · o `derivados.py` vermelho, a acusar dois documentos correctos;
  · o `versoes_readme.py` vermelho.

> **Portao que ninguem corre e decoracao.** Uma regra escrita numa cicatriz
> so passa a valer quando existe um comando que a verifica E alguem o corre.
> Este ficheiro e a segunda metade.

(!) SO LEITURA. Nenhuma ferramenta desta lista escreve placa, esquema ou
documento. As que escrevem — `pcb_contorno.py`, `serigrafia_dfm.py`,
`normaliza_silk.py`, `dnp_para_pcb.py`, `roteia_fr.py` — estao FORA de
proposito, e a rev C esta congelada: correr uma delas por engano reescreve
o ficheiro que foi fabricado. Ja aconteceu uma vez.

Saida: 0 = todos verdes · 1 = ha portao vermelho · 3 = faltou ferramenta.
"""
import os
import subprocess
import sys

VERSAO = "0.1.0"
AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
KP = ("/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework"
      "/Versions/3.9/bin/python3.9")
PY = sys.executable

CEREBRO = "kicad/cotuca_cerebro/cotuca_cerebro.kicad_pcb"
PAINEL = "kicad/cotuca_painel/cotuca_painel.kicad_pcb"

# DIVIDA DECLARADA, COM CATRACA.
#
# Dois portoes estao vermelhos por defeitos CONHECIDOS e ja escritos, nao por
# surpresa: os fantasmas da documentacao e os achados de encaixe da rev C, que
# esta CONGELADA e so se conserta na rev D. Apagar o portao seria esconder;
# deixa-lo vermelho para sempre treina toda a gente a ignorar o vermelho.
#
# Entao declara-se o numero. Abaixo dele, verde. Acima, VERMELHO. E se descer,
# o corredor MANDA BAIXAR A LINHA — divida so anda num sentido.
#
# (nome legivel, interpretador, argumentos, o que impede, (regex, linha_base))
PORTOES = [
    ("documentacao: nomes fantasma", PY, ["src/check_docs.py"],
     "nome que so existe na prosa",
     (r"FALHOU: (\d+) fantasma", 50)),
    ("documentacao: derivados tem gerador", PY, ["docs/operacao/derivados.py"],
     "rotulo `gerado` sem gerador"),
    ("documentacao: datas batem com o git", PY, ["docs/operacao/verifica_datas.py"],
     "data escrita a mao que envelhece"),
    ("documentacao: frase partida por renomeacao", PY, ["docs/operacao/vizinhos.py"],
     "renomeacao que parte a frase ao lado"),
    ("README: versoes dos ficheiros", PY, ["docs/operacao/versoes_readme.py"],
     "README a mentir sobre que versao esta la"),
    ("manual: sobreposicao em TODAS as figuras", PY, ["tools/colisoes_manual.py"],
     "rotulo por cima de rotulo ou a cruzar borda"),
    ("IDC: as 26 vias das duas placas", PY, ["tools/contrato_idc.py"],
     "via que muda de significado entre as placas"),
    ("ponte ESP32: firmware x cobre", KP, ["tools/pinos_ponte.py"],
     "firmware gravado com pinos que a placa nao tem"),
    # os 10: 3 pecas altas sob o botao azul (bloqueio da rev D), fixacao do
    # painel com 1 ponto, 4 TP no lado errado (bts 15fg), step-down sem casa.
    ("mecanica IFA10000-M (as duas placas)", KP, ["kicad/encaixe.py", "todas"],
     "peca que nao monta na caixa",
     (r"(\d+) achado\(s\) de encaixe", 10)),
    ("courtyard dentro do contorno (cerebro)", KP, ["kicad/dentro_do_contorno.py", "cerebro"],
     "peca a sair da placa"),
    ("courtyard dentro do contorno (painel)", KP, ["kicad/dentro_do_contorno.py", "painel"],
     "peca a sair da placa"),
    ("funcao dos pads: o pad certo, a rede certa", PY, ["kicad/funcao_dos_pads.py"],
     "rede correcta no pad errado"),
    ("referencias apontam para a propria peca (cerebro)", PY,
     ["kicad/refs_apontam.py", CEREBRO], "serigrafia a apontar para o vizinho"),
    ("referencias apontam para a propria peca (painel)", PY,
     ["kicad/refs_apontam.py", PAINEL], "serigrafia a apontar para o vizinho"),
    ("vias tapadas (cerebro)", PY, ["kicad/vias_tented.py", CEREBRO],
     "via aberta a apanhar solda"),
    ("vias tapadas (painel)", PY, ["kicad/vias_tented.py", PAINEL],
     "via aberta a apanhar solda"),
]

# ⚠️ FORA DE PROPOSITO, E DIZ-SE PORQUE. Silencio e indistinguivel de esquecimento.
DE_FORA = [
    ("tools/svg_colisoes.py nas folhas do KiCad",
     "os renders do `kicad-cli` trazem centenas de colisoes de rotulo herdadas; "
     "e divida REAL, mas anterior a este portao e grande de mais para entrar "
     "vermelha — entra quando for atacada, e ate la esta no STATUS"),
    ("kicad/para_fabrica.py",
     "gera o pacote da fabrica: escreve ficheiros, logo nao e portao de leitura"),
    ("docs/operacao/coerencia.py",
     "conta tarefas abertas e sai 1 por existirem — e relatorio, nao portao"),
]


def main():
    print("portoes v%s — %d portoes de leitura" % (VERSAO, len(PORTOES)))
    rapido = "--rapido" in sys.argv
    faltou, maus, linhas = 0, 0, []
    for entrada in PORTOES:
        nome, interp, args, impede = entrada[:4]
        base = entrada[4] if len(entrada) > 4 else None
        alvo = os.path.join(RAIZ, args[0])
        if not os.path.exists(alvo):
            linhas.append("  ?? %-48s FERRAMENTA AUSENTE: %s" % (nome, args[0]))
            faltou += 1
            continue
        if interp == KP and not os.path.exists(KP):
            linhas.append("  ?? %-48s sem o Python do KiCad" % nome)
            faltou += 1
            continue
        if rapido and interp == KP:
            linhas.append("  -- %-48s saltado (--rapido)" % nome)
            continue
        p = subprocess.run([interp] + args, cwd=RAIZ,
                           capture_output=True, text=True)
        ok = p.returncode == 0
        saida = (p.stdout or "") + (p.stderr or "")
        if not ok and base:
            import re as _re
            m = _re.search(base[0], saida)
            n = int(m.group(1)) if m else None
            if n is None:
                linhas.append("  ?? %-48s nao consegui contar os achados" % nome)
                faltou += 1
                continue
            if n < base[1]:
                linhas.append("  ok %-48s divida DESCEU: %d (linha %d)"
                              " — BAIXE A LINHA neste ficheiro" % (nome, n, base[1]))
                continue
            if n == base[1]:
                linhas.append("  ok %-48s divida declarada: %d" % (nome, n))
                continue
            linhas.append("  XX %-48s %d achados, linha declarada %d — SUBIU"
                          % (nome, n, base[1]))
            maus += 1
            continue
        maus += not ok
        linhas.append("  %s %-48s %s" % ("ok" if ok else "XX", nome,
                                         "" if ok else "impede: " + impede))
        if not ok:
            for l in saida.strip().splitlines()[-4:]:
                linhas.append("       %s" % l.strip()[:100])
    print("\n".join(linhas))
    print("\n  fora de proposito, de propria vontade:")
    for nome, porque in DE_FORA:
        print("    · %s — %s" % (nome, porque))
    if faltou:
        print("\n  FALTOU ferramenta: nem aprovacao nem reprovacao.")
        return 3
    if maus:
        print("\n  %d portao(oes) VERMELHO(S). Uma regra so vale com o portao"
              " a correr." % maus)
        return 1
    print("\n  ✓ todos os portoes verdes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
