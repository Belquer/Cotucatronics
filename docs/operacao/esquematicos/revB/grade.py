"""grade v0.1.0 — encaixa a folha na grade de ligacao do KiCad.

POR QUE ISTO EXISTE: o KiCad liga por COINCIDENCIA de coordenada, e a
grade do editor e 1,27 mm. Os geradores colocavam tudo em milimetros
inteiros — 35,00 · 98,00 — que nao sao multiplos de 1,27.

Hoje ligava, porque pino e fio tinham a MESMA coordenada. Mas no dia em
que alguem arrastar um fio no editor, o cursor pula para a grade e a
ponta vai parar a 97,79 enquanto o pino fica em 98,00: os dois continuam
DESENHADOS encostados e a ligacao ja nao existe. Sem aviso.

⚠️ SO A COLOCACAO, NUNCA O DESENHO DENTRO DO SIMBOLO. Medido em 01/09:
os pinos internos ja estao todos na grade (offsets de 2,54 e 3,81 sao
multiplos de 1,27), e arredondar as linhas de um triangulo de diodo
distorceria o desenho para nao consertar nada.

⚠️ E ARREDONDAR E FUNCAO PURA: dois pontos que coincidiam continuam a
coincidir depois. E o que garante que nenhuma ligacao se perde — e o
teste que o prova e o netlist ser IGUAL antes e depois.

O risco simetrico e o oposto: dois pontos DIFERENTES a menos de 0,635 mm
podem cair no mesmo sitio e criar uma ligacao que nao existia. Por isso
o `confere_colisoes` existe e corre sempre.
"""
GRADE = 1.27


def enc(v):
    return round(round(v / GRADE) * GRADE, 4)


def encaixa(pos):
    """Position do kiutils, no lugar. O `angle` nao se toca."""
    if pos is None:
        return
    pos.X = enc(pos.X)
    pos.Y = enc(pos.Y)


def confere_colisoes(antes, depois, rot=""):
    """Dois pontos distintos que passaram a ser o mesmo = ligacao
    inventada. Levanta, nao avisa: ligacao a mais e pior que aviso a
    mais, porque nao aparece em lado nenhum ate a fabricacao."""
    d = {}
    for a, b in zip(antes, depois):
        if b in d and d[b] != a:
            raise SystemExit(
                "COLISAO DE GRADE em %s: %s e %s caem os dois em %s"
                % (rot, d[b], a, b))
        d[b] = a
