#!/usr/bin/env python3
"""gerar_ccmap v0.2.0 — gera a tabela do CCMAP.md a partir de src/manifest.h.

Sem versao ate 28/08; nasce em 0.2.0 e nao em 0.1.0 de proposito, para nao
se dar por novo um ficheiro que ja tinha historia.

USO:  python3 gerar_ccmap.py [caminho/para/manifest.h]

Resolve SIMBOLO -> NOME DO PARAM pelas atribuicoes (`p_x = t.Add("nome"...)`),
porque os dois divergem: `p_phs_feedback` e o simbolo C++, `phs_fb` e o nome
que o `set` aceita. Ler o simbolo produzia um nome que NAO EXISTE — foi
exatamente o que aconteceu na primeira versao deste gerador, e a faixa saia
vazia junto, porque a busca por faixa falhava em silencio na sequencia.

Sai com codigo 1 se algum CC nao resolver, em vez de emitir tabela incompleta:
tabela gerada com buraco e pior que tabela ausente.
"""
import os
import pathlib
import re
import sys


def achar_manifest():
    if len(sys.argv) > 1:
        return pathlib.Path(sys.argv[1])
    if os.environ.get('COTUCA_MANIFEST'):
        return pathlib.Path(os.environ['COTUCA_MANIFEST'])
    aqui = pathlib.Path(__file__).resolve().parent
    for c in (aqui / '../../src/manifest.h',
              aqui / '../src/manifest.h',
              aqui / 'src/manifest.h'):
        if c.exists():
            return c
    print('ERRO: manifest.h nao encontrado. Passe o caminho como argumento.',
          file=sys.stderr)
    sys.exit(2)


MANIFEST = achar_manifest()
src = MANIFEST.read_text()

# ---- simbolo -> nome real do param ----
sym2name = {}
for m in re.finditer(r'(p_[a-z0-9_]+)(\[\d\])?\s*=\s*t\.Add\("([a-z0-9_]+)"', src):
    sym2name[(m.group(1), m.group(2) or '')] = m.group(3)

# ---- nome -> (min, max) em unidade real ----
faixas = {}
for m in re.finditer(
        r't\.Add\("([a-z0-9_]+)",\s*([-0-9.]+)f?,\s*([-0-9.]+)f?,\s*([-0-9.]+)f?',
        src):
    faixas[m.group(1)] = (m.group(3), m.group(4))

FAMILIAS = [('piezo', 'mistura'), ('in_gain', 'entrada'), ('gate', 'entrada'),
            ('drv', 'drive'), ('phs', 'phaser'), ('wrb', 'warble'),
            ('dly', 'delay'), ('frz', 'freeze'), ('rev', 'reverb'),
            ('shm', 'shimmer'), ('buzios', 'búzios'), ('tone', 'tom'),
            ('level', 'saída'), ('exc', 'saída'), ('gov', 'proteção'),
            ('shift', 'proteção'), ('route', 'saída'), ('bench', 'bancada'),
            ('out_', 'saída'), ('test_tone', 'tom')]


def familia(n):
    return next((v for k, v in FAMILIAS if n.startswith(k)), '—')


# O CANAL VIROU PARTE DO ENDERECO (v0.79.0 do manifest): o mesmo numero
# de CC no ch1 e no ch2 sao params DIFERENTES. Uma tabela sem a coluna de
# canal deixou de descrever o instrumento — descreve metade dele, e a
# metade que falta e justamente a que o dB precisa copiar no Loopy.
# Os SEIS canais (v0.156.0). Escuta = manda + 3, entao o gerador so
# precisa dos tres de entrada — a coluna de escuta se deduz.
#
# ESTE DICIONARIO JA FICOU VELHO UMA VEZ: foi escrito para dois canais e
# a mudanca para tres o deixou com `kCanalCompart` desconhecido. O
# gerador FALHOU ALTO em vez de emitir tabela torta, que era o desenho —
# mas o episodio mostra que gerador tambem tem premissa, e premissa
# tambem envelhece. A diferenca e que a dele grita.
CANAIS = {'kCanalCompart': 1, 'kCanalEntradaA': 2, 'kCanalEntradaB': 3}
DESLOC_ESCUTA = 3

linhas, faltando = [], []
achados = re.findall(r'AddMidi\(\s*(\d+)\s*,\s*([^,)]+)\s*(?:,\s*(\w+)\s*)?\)',
                     src)
for cc, expr, canal in sorted(achados,
                              key=lambda x: (int(x[0]), CANAIS.get(x[2], 1))):
    e = expr.strip()
    mm = re.match(r'(p_[a-z0-9_]+)(\[\d\])?$', e)
    nome = sym2name.get((mm.group(1), mm.group(2) or '')) if mm else None
    if not nome:
        faltando.append((cc, e))
        continue
    # Canal desconhecido NAO vira 1 por conveniencia: um default aqui
    # mentiria com a mesma eficacia do `else 0` que ja custou uma rodada
    # neste projeto. Sem canal reconhecido, entra na lista de faltando.
    if canal and canal not in CANAIS:
        faltando.append((cc, f'{e} (canal desconhecido: {canal})'))
        continue
    ch = CANAIS.get(canal, 1)
    f = faixas.get(nome)
    fx = f'{f[0].rstrip(".")} – {f[1].rstrip(".")}' if f else '—'
    linhas.append(f'| {cc} | ch{ch} | ch{ch + DESLOC_ESCUTA} |'
                  f' `{nome}` | {familia(nome)} | {fx} |')

if faltando:
    print(f'ERRO: {len(faltando)} CC(s) sem nome resolvido: {faltando}',
          file=sys.stderr)
    sys.exit(1)

print(f'<!-- gerado por gerar_ccmap v0.2.0 de {MANIFEST.name} — nao editar a mao -->')
print('| CC | manda | escuta | param | família | faixa real |')
print('|---|---|---|---|---|---|')
print('\n'.join(linhas))
ncc = len({l.split('|')[1].strip() for l in linhas})
print(f'\n<!-- {len(linhas)} vinculos em {ncc} numeros de CC -->')
