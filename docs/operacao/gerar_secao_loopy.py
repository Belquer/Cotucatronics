#!/usr/bin/env python3
"""gerar_secao_loopy v0.3.0 — escreve LOOPY-SECAO.md a partir do manifest.h.

    ./gerar_secao_loopy.py

POR QUE E GERADO E NAO ESCRITO A MAO: em 01/09 mudaram SEIS CCs numa
sessao (notch para 82-86, familia tone_ renomeada, niveis renomeados).
Folha de montagem escrita a mao deriva na primeira mudanca, e quem a
segue no iPad monta o instrumento de ontem. O `gerar_ccmap.py` ja
provou o principio; isto e o mesmo principio, outra superficie.

O QUE ELE NAO SABE, e diz que nao sabe: a posicao dos widgets no ecra.
O Loopy enderessa por indice de array dentro da cena, e esse indice so
existe depois de alguem os criar. Este documento e a ORDEM DE
APRENDIZAGEM, nao o layout.
"""
import re, sys, pathlib

VERSAO = "0.3.0"
AQUI = pathlib.Path(__file__).resolve().parent
SRC = (AQUI / ".." / ".." / "src" / "manifest.h").resolve()

CANAIS = {"kCanalCompart": ("ch1", "compartilhado"),
          "kCanalEntradaA": ("ch2", "cadeia A"),
          "kCanalEntradaB": ("ch3", "cadeia B")}

# Familias na ordem em que fazem sentido numa pagina. O prefixo decide.
FAMILIAS = [
    ("pre_level", "NIVEIS — antes e depois do limitador"),
    ("post_level", "NIVEIS — antes e depois do limitador"),
    ("notch", "NOTCH anti-microfonia"),
    ("test_tone", "TOM DE TESTE"),
    ("gate", "GATE"),
    ("drv", "DRIVE"),
    ("phs", "PHASER"),
    ("wrb", "WARBLE"),
    ("tarol", "TAROLATOR"),
    ("frz", "FREEZE"),
    ("rev", "REVERB"),
    ("shm", "SHIMMER"),
    ("dly", "DELAY"),
    ("route", "ROTEAMENTO"),
    ("out", "SAIDA"),
    ("exc", "EXCITERS"),
    ("piezo", "FONTE"),
    ("gov", "GOVERNOR"),
    ("buzios", "BUZIOS — sorteio"),
    ("shift", "SHIFT"),
    ("bench", "BANCADA — nao vai para o painel"),
]


def familia(nome):
    for k, t in FAMILIAS:
        if nome.startswith(k):
            return t
    return "OUTROS"


def tipo_widget(nome, lo, hi, passos):
    """Fader ou botao? A primeira versao errou isto, e o erro era de
    premissa: eu tratei "faixa 0-1 sem passos declarados" como
    interruptor, e e o CONTRARIO — sem passos e continuo. O
    `pre_level_ab` saiu como botao numa folha de montagem, o que faria
    o dB montar um interruptor onde precisa de um fader.

    E `passos` sozinho tambem nao chega: os `_on` desta casa declaram-se
    como `0.f, 1.f` sem passos nenhuns, e sao interruptores por
    convencao. Entao o NOME tambem decide — e e por isso que o nome
    carregar a topologia vale para as ferramentas, nao so para quem le.
    """
    if passos == 2:
        return "Botao (toggle)"
    if passos > 2:
        return "Botao x%d (passos)" % passos
    if nome.endswith("_on") or "_on_" in nome:
        return "Botao (toggle)"
    if nome.startswith("route_") or nome.startswith("out_"):
        return "Botao (corta o ramo)"
    return "Fader"


def main():
    print("gerar_secao_loopy v%s" % VERSAO)
    src = SRC.read_text(encoding="utf-8")

    sym2name, faixa, defa, passos = {}, {}, {}, {}
    for m in re.finditer(
            r'(p_[a-z0-9_]+)((?:\[\d\])*)\s*=\s*t\.Add\("([a-z0-9_]+)",'
            r'\s*([-0-9.]+)f?,\s*([-0-9.]+)f?,\s*([-0-9.]+)f?'
            r'(?:,\s*([-0-9.]+)f?)?(?:,\s*(\d+))?', src):
        sym, idx, nome = m.group(1), m.group(2) or "", m.group(3)
        sym2name[(sym, idx)] = nome
        defa[nome] = float(m.group(4))
        faixa[nome] = (float(m.group(5)), float(m.group(6)))
        passos[nome] = int(m.group(8)) if m.group(8) else 0

    linhas = []
    for m in re.finditer(
            r'AddMidi\(\s*(\d+)\s*,\s*([^,)]+)\s*(?:,\s*(\w+)\s*)?\)', src):
        cc, expr, canal = int(m.group(1)), m.group(2).strip(), m.group(3)
        mm = re.match(r'(p_[a-z0-9_]+)((?:\[\d\])*)$', expr)
        if not mm:
            continue
        nome = sym2name.get((mm.group(1), mm.group(2) or ""))
        if not nome:
            print("  ⚠️  CC %d sem nome resolvido: %s" % (cc, expr))
            continue
        linhas.append((cc, nome, canal or "kCanalEntradaA"))

    # agrupa por familia, e dentro dela por CC
    porfam = {}
    for cc, nome, canal in linhas:
        porfam.setdefault(familia(nome), []).append((cc, nome, canal))

    out = []
    A = out.append
    A("# Loopy Pro — secção nova, com MIDI learn")
    A("")
    A("**Gerado por `gerar_secao_loopy.py v%s` a partir do `manifest.h`.**"
      " Não editar à mão — correr o gerador." % VERSAO)
    A("")
    A("⚠️ **Isto é a ORDEM DE APRENDIZAGEM, não o layout.** O Loopy endereça"
      " widgets por índice de array dentro da cena, e esse índice só existe"
      " depois de alguém os criar. Cria os widgets na ordem abaixo e faz"
      " `learn` um a um — assim o mapa fica igual ao documento.")
    A("")
    A("## Regra dos canais, e ela é o endereço")
    A("")
    A("| canal | destino |")
    A("|---|---|")
    A("| **ch1** | parâmetros compartilhados — e só eles |")
    A("| **ch2** | cadeia A |")
    A("| **ch3** | cadeia B |")
    A("")
    A("**Escuta = manda + 3.** O mesmo número de CC no ch4/ch5/ch6 é o retorno"
      " do instrumento. Widget que só manda usa ch1–ch3; widget que também"
      " ouve aprende o par.")
    A("")
    A("⚠️ **O mesmo CC em canais diferentes são parâmetros DIFERENTES.** Uma"
      " tabela sem a coluna de canal descreve metade do instrumento.")
    A("")

    total = 0
    for fam, _ in [(f, 0) for f in dict.fromkeys(t for _, t in FAMILIAS)] + [("OUTROS", 0)]:
        if fam not in porfam:
            continue
        itens = sorted(set(porfam[fam]))
        A("## %s" % fam)
        A("")
        A("| CC | canal | parâmetro | widget | faixa | nasce |")
        A("|---|---|---|---|---|---|")
        for cc, nome, canal in itens:
            ch, quem = CANAIS.get(canal, ("?", canal))
            lo, hi = faixa.get(nome, (0.0, 1.0))
            d = defa.get(nome, 0.0)
            A("| **%d** | %s (%s) | `%s` | %s | %g – %g | %g |"
              % (cc, ch, quem, nome, tipo_widget(nome, lo, hi, passos.get(nome, 0)),
                 lo, hi, d))
            total += 1
        A("")

    A("## Página de PRESETS")
    A("")
    A("**CC 121 = carregar.** O canal diz o destino e o valor diz o slot —"
      " a mesma gramática do resto.")
    A("")
    A("| widget | CC | canal | valor | efeito |")
    A("|---|---|---|---|---|")
    A("| Preset global 1..8 | **121** | ch1 | 0..7 | carrega o slot global |")
    A("| Preset cadeia A 1..16 | **121** | ch2 | 0..15 | carrega na cadeia A |")
    A("| Preset cadeia B 1..16 | **121** | ch3 | 0..15 | carrega na cadeia B |")
    A("")
    A("São **botões de valor fixo**, não faders: cada botão manda o CC 121"
      " com o seu número de slot. No Loopy, o binding fica"
      " `B0 79 <slot>` (ch1), `B1 79 <slot>` (ch2), `B2 79 <slot>` (ch3) —"
      " `79` é 121 em hexadecimal.")
    A("")
    A("**Slot vazio responde.** A placa diz `preset CC: slot N ... VAZIO` no"
      " log. Um botão de palco que não faz nada é indistinguível de um botão"
      " que não chegou, e as duas coisas pedem consertos opostos.")
    A("")
    A("🚫 **CC 122 (gravar) NÃO existe, e a omissão é deliberada.** Um CC que"
      " escreve preset destrói uma biblioteca com uma mensagem perdida, e não"
      " traz nome nenhum para lhe dar. Capturar fica no serial —"
      " `capturar <a|b|g> <nome>` — onde o gesto é deliberado e o preset"
      " nasce com nome.")
    A("")
    A("⚠️ **Os presets vivem em RAM.** Desliga da tomada e somem."
      " `preset dump` guarda no computador enquanto a escrita na flash não"
      " destrava.")
    A("")
    A("⚠️ **Quantização em 7 bits.** O preset guarda 0..127, como o MIDI."
      " Medido em 01/09: um notch capturado a 834 Hz volta a 847,6 Hz — a 834,"
      " com curva 2.0 sobre 60–8000, um passo vale ~14 Hz. O notch tem 24 Hz"
      " de largura e ainda cobre o modo, mas ajuste mais fino que 1/127 não"
      " sobrevive à captura.")
    A("")
    A("---")
    A("")
    A("**%d vínculos** nesta folha." % total)

    alvo = AQUI / "LOOPY-SECAO.md"
    alvo.write_text("\n".join(out) + "\n", encoding="utf-8")
    print("  escrito: %s  (%d vinculos)" % (alvo.name, total))
    return 0


if __name__ == "__main__":
    sys.exit(main())
