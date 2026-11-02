#!/usr/bin/env python3
"""encaixa_grade v0.3.1 — poe a geometria de um .kicad_sch na grade de 1,27 mm.

    ./encaixa_grade.py --medir  FICHEIRO...     # so conta, nao escreve
    ./encaixa_grade.py          FICHEIRO...     # escreve

POR QUE (skill `kicad` §3). O KiCad liga por COINCIDENCIA DE COORDENADA, e a
grade do editor e 1,27 mm. Coordenadas em milimetros inteiros ligam HOJE —
pino e fio partilham o mesmo valor — e partem no dia em que alguem arrastar um
fio: o cursor salta para a grade, a ponta vai para 97,79 e o pino fica em
98,00. Os dois continuam DESENHADOS encostados e a ligacao ja nao existe.

⚠️ SO A COLOCACAO, NUNCA O DESENHO DENTRO DO SIMBOLO. O bloco `lib_symbols` e
mascarado antes de qualquer substituicao: arredondar o interior distorce
triangulos de diodo para nao consertar ligacao nenhuma. Medido em 2026-09-01: os
pinos internos estavam 100% na grade e so a colocacao estava fora.

⚠️ E O RISCO NAO E PERDER LIGACAO, E INVENTAR UMA. Arredondar e funcao pura,
logo dois pontos que coincidiam continuam a coincidir. O perigo e o inverso:
dois pontos DISTINTOS a menos de meia grade caem no mesmo sitio e criam uma
ligacao que ninguem desenhou — e ligacao a mais nao aparece em lado nenhum
ate a fabricacao. Este script LEVANTA cada caso desses em vez de avisar.

Aceitacao: netlist identico antes e depois. Nao e opcional.
"""
import io, re, sys

VERSAO = "0.3.1"
G = 1.27


def _mascara_lib(s):
    """Devolve (texto_fora, reinserir) com o `lib_symbols` fora do alcance."""
    i = s.find('(lib_symbols')
    if i < 0:
        return s, (lambda t: t)
    d = 0
    for j in range(i, len(s)):
        if s[j] == '(': d += 1
        elif s[j] == ')':
            d -= 1
            if d == 0:
                bloco = s[i:j + 1]
                marca = '\x00LIBSYMS\x00'
                return s[:i] + marca + s[j + 1:], (lambda t: t.replace(marca, bloco, 1))
    raise SystemExit('lib_symbols sem fecho')


def _snap(v):
    return round(round(v / G) * G, 4)


def processa(p, escrever):
    s = io.open(p, encoding='utf-8').read()
    fora, repor = _mascara_lib(s)

    antes, depois, movidos = [], [], 0

    def coord(m):
        nonlocal movidos
        x, y = float(m.group(2)), float(m.group(3))
        nx, ny = _snap(x), _snap(y)
        antes.append((x, y)); depois.append((nx, ny))
        if (nx, ny) != (x, y):
            movidos += 1
        resto = m.group(4) or ''
        return '%s%g %g%s' % (m.group(1), nx, ny, resto)

    # ⚠️ APENAS estes construtores. `(at` de um `(property` nao entra: e a
    # posicao do TEXTO do rotulo, e move-lo nao muda ligacao nenhuma.
    # ⚠️ SO O `(xy` fica por regex, porque nao tem forma alternativa.
    #
    # As juncoes e os `no_connect` estavam aqui, casados por
    # `\(junction \(at ` — que exige tudo numa linha. No formato expandido
    # do KiCad 10 o `(at` vai para a linha seguinte, e o padrao apanhou
    # ZERO das 26 juncoes da folha do pre-amp.
    #
    # E juncao e SEMANTICA (skill `kicad` §4): e ela que faz os fios
    # ligarem-se num ponto. Mover os fios e deixar as juncoes mata toda a
    # ligacao em T — as redes de dois membros sobreviveram e as de tres ou
    # mais morreram, que foi exactamente o sintoma.
    #
    # E o pior: e o MESMO erro que a v0.2.0 corrigiu para os simbolos.
    # Consertei o casamento por estrutura num sitio e deixei-o por
    # indentacao no outro. Um conserto nao adoptado por TODOS os chamadores
    # nao esta feito.
    fora = re.sub(r'(\(xy )(-?[\d.]+(?:[eE][-+]?\d+)?) (-?[\d.]+(?:[eE][-+]?\d+)?)()\)',
                  lambda m: coord(m) + ')', fora)
    # COLOCACAO de simbolos e rotulos.
    #
    # ⚠️ POR ESTRUTURA, NUNCA POR INDENTACAO. A v0.1.0 casava
    # `\n\t\t(at ` — que e como o kiutils formata — e a folha da energia
    # tinha essa forma. A do pre-amp estava no estilo compacto antigo,
    # `(symbol (lib_id "...") (at 30 70.08 0)` tudo numa linha, e nao casou:
    # os FIOS moveram-se e os SIMBOLOS ficaram. Resultado medido pelo netlist:
    # 18 redes viraram 72, todas `unconnected-*`. Todas as ligacoes da folha,
    # destruidas por um regex que descrevia um layout de texto em vez da
    # arvore.
    #
    # Aqui percorre-se cada bloco `(symbol`/`(*_label`/`(text` equilibrado e
    # encaixa-se o PRIMEIRO `(at` dele, que e sempre a colocacao — os `(at`
    # seguintes pertencem a `(property`, e sao posicao de texto.
    def coloca(txt):
        out, i = [], 0
        alvo = re.compile(r'\((symbol|global_label|hierarchical_label|label|text|junction|no_connect)\b')
        while True:
            m = alvo.search(txt, i)
            if not m:
                out.append(txt[i:]); break
            j, d = m.start(), 0
            for k in range(j, len(txt)):
                if txt[k] == '(': d += 1
                elif txt[k] == ')':
                    d -= 1
                    if d == 0:
                        fim = k + 1; break
            else:
                out.append(txt[i:]); break
            bloco = txt[j:fim]
            novo = re.sub(r'\(at (-?[\d.]+(?:[eE][-+]?\d+)?) (-?[\d.]+(?:[eE][-+]?\d+)?)( -?[\d.]+(?:[eE][-+]?\d+)?)?\)',
                          lambda mm: coord2(mm), bloco, count=1)
            out.append(txt[i:j]); out.append(novo); i = fim
        return ''.join(out)

    def coord2(m):
        nonlocal movidos
        x, y = float(m.group(1)), float(m.group(2))
        nx, ny = _snap(x), _snap(y)
        antes.append((x, y)); depois.append((nx, ny))
        if (nx, ny) != (x, y):
            movidos += 1
        return '(at %g %g%s)' % (nx, ny, m.group(3) or '')

    fora = coloca(fora)

    # ⚠️ LIGACAO INVENTADA: pontos distintos que passaram a coincidir
    colisoes = {}
    for (a, b) in zip(antes, depois):
        colisoes.setdefault(b, set()).add(a)
    inventadas = {k: v for k, v in colisoes.items() if len(v) > 1}

    print('  %-28s %3d pontos | %3d movidos | %d colisao(oes) nova(s)'
          % (p, len(antes), movidos, len(inventadas)))
    for k, v in list(inventadas.items())[:5]:
        print('     ⚠️ %s <- %s' % (k, sorted(v)))
    if escrever and not inventadas:
        io.open(p, 'w', encoding='utf-8').write(repor(fora))
        print('     escrito')
    elif escrever:
        print('     *** NAO ESCRITO: ha ligacoes inventadas, decidir a mao')
    return len(inventadas)


if __name__ == '__main__':
    print('encaixa_grade v%s' % VERSAO)
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    sys.exit(sum(processa(p, '--medir' not in sys.argv) for p in args))
