#!/usr/bin/env python3
# fonte5x7 v0.1.0 — gera src/fonte5x7.h a partir de glifos LEGIVEIS.
#
# Porque gerador e nao tabela: uma tabela de hex nao se confere. Um glifo
# errado num `0x7E` so aparece no vidro, e ai nao se sabe se foi a fonte,
# o desenho ou a SPI. Aqui o fonte-de-verdade e desenhavel a olho, e o
# `--ver` re-desenha o que foi gerado — a volta completa.
#
# Formato de saida: 5 colunas por glifo, bit 0 = linha de cima.
# E o formato do SSD1306, entao Texto() escreve coluna a coluna sem conta.
import sys

VERSAO = "0.1.0"

# 7 linhas de 5, concatenadas. '#' acende.
G = {
' ': ".....'.....'.....'.....'.....'.....'.....",
'0': ".###.'#...#'#..##'#.#.#'##..#'#...#'.###.",
'1': "..#..'.##..'..#..'..#..'..#..'..#..'.###.",
'2': ".###.'#...#'....#'...#.'..#..'.#...'#####",
'3': "#####'...#.'..#..'...#.'....#'#...#'.###.",
'4': "...#.'..##.'.#.#.'#..#.'#####'...#.'...#.",
'5': "#####'#....'####.'....#'....#'#...#'.###.",
'6': "..##.'.#...'#....'####.'#...#'#...#'.###.",
'7': "#####'....#'...#.'..#..'.#...'.#...'.#...",
'8': ".###.'#...#'#...#'.###.'#...#'#...#'.###.",
'9': ".###.'#...#'#...#'.####'....#'...#.'.##..",
'A': "..#..'.#.#.'#...#'#...#'#####'#...#'#...#",
'B': "####.'#...#'#...#'####.'#...#'#...#'####.",
'C': ".###.'#...#'#....'#....'#....'#...#'.###.",
'D': "###..'#..#.'#...#'#...#'#...#'#..#.'###..",
'E': "#####'#....'#....'####.'#....'#....'#####",
'F': "#####'#....'#....'####.'#....'#....'#....",
'G': ".###.'#...#'#....'#..##'#...#'#...#'.####",
'H': "#...#'#...#'#...#'#####'#...#'#...#'#...#",
'I': ".###.'..#..'..#..'..#..'..#..'..#..'.###.",
'J': "....#'....#'....#'....#'#...#'#...#'.###.",
'K': "#...#'#..#.'#.#..'##...'#.#..'#..#.'#...#",
'L': "#....'#....'#....'#....'#....'#....'#####",
'M': "#...#'##.##'#.#.#'#.#.#'#...#'#...#'#...#",
'N': "#...#'##..#'#.#.#'#..##'#...#'#...#'#...#",
'O': ".###.'#...#'#...#'#...#'#...#'#...#'.###.",
'P': "####.'#...#'#...#'####.'#....'#....'#....",
'Q': ".###.'#...#'#...#'#...#'#.#.#'#..#.'.##.#",
'R': "####.'#...#'#...#'####.'#.#..'#..#.'#...#",
'S': ".####'#....'#....'.###.'....#'....#'####.",
'T': "#####'..#..'..#..'..#..'..#..'..#..'..#..",
'U': "#...#'#...#'#...#'#...#'#...#'#...#'.###.",
'V': "#...#'#...#'#...#'#...#'#...#'.#.#.'..#..",
'W': "#...#'#...#'#...#'#.#.#'#.#.#'##.##'#...#",
'X': "#...#'#...#'.#.#.'..#..'.#.#.'#...#'#...#",
'Y': "#...#'#...#'.#.#.'..#..'..#..'..#..'..#..",
'Z': "#####'....#'...#.'..#..'.#...'#....'#####",
'.': ".....'.....'.....'.....'.....'.##..'.##..",
'-': ".....'.....'.....'#####'.....'.....'.....",
':': ".....'.##..'.##..'.....'.##..'.##..'.....",
'/': "....#'....#'...#.'..#..'.#...'#....'#....",
'v': ".....'.....'#...#'#...#'#...#'.#.#.'..#..",
'x': ".....'.....'#...#'.#.#.'..#..'.#.#.'#...#",
}

def colunas(art):
    linhas = art.split("'")
    assert len(linhas) == 7 and all(len(l) == 5 for l in linhas), art
    # bit 0 = linha de cima, entao o glifo cabe nos bits 0..6 de cada coluna
    return [sum((1 << y) for y in range(7) if linhas[y][x] == '#') for x in range(5)]

def desenha(cols):
    for y in range(7):
        print('  ' + ''.join('#' if (cols[x] >> y) & 1 else '.' for x in range(5)))

if '--ver' in sys.argv:
    # a volta completa: re-desenha a partir das COLUNAS geradas, nao da arte
    for ch in sorted(G):
        print(f"[{ch}]"); desenha(colunas(G[ch]))
    sys.exit(0)

ordem = sorted(G)
faixa = f"// cobertura: {''.join(ordem)}"
out = [f'''// fonte5x7 v{VERSAO} — GERADO por fonte5x7.py, nao editar a mao.
//
// 5 colunas por glifo, bit 0 = linha de cima (formato do SSD1306).
// {len(ordem)} glifos. Caractere fora da cobertura desenha CAIXA, nunca
// espaco: falta silenciosa de glifo e indistinguivel de texto certo.
{faixa}
#pragma once
#include <stdint.h>

namespace cotuca {{ namespace fonte {{

#define COTUCA_FONTE5X7_VERSION "{VERSAO}"

static const char kCobertura[] = "{''.join(ordem).replace(chr(92), '')}";
static const uint8_t kGlifos[][5] = {{''']
for ch in ordem:
    c = colunas(G[ch])
    nome = 'espaco' if ch == ' ' else ch
    out.append('    {' + ', '.join(f'0x{v:02X}' for v in c) + f'}},  // {nome}')
out.append('''};

// CAIXA para o desconhecido — ver a ausencia e melhor que nao a ver.
static const uint8_t kCaixa[5] = {0x7F, 0x41, 0x41, 0x41, 0x7F};

// Devolve as 5 colunas do caractere, ou a caixa. Nunca devolve nullptr.
inline const uint8_t* Glifo(char c)
{
    for(unsigned i = 0; kCobertura[i]; i++)
        if(kCobertura[i] == c) return kGlifos[i];
    return kCaixa;
}

}  // namespace fonte
}  // namespace cotuca''')
open('fonte5x7.h', 'w').write('\n'.join(out) + '\n')
print(f"fonte5x7 v{VERSAO}: {len(ordem)} glifos -> fonte5x7.h")
