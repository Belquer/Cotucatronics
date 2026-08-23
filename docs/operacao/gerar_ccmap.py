#!/usr/bin/env python3
"""Gera a tabela do CCMAP.md a partir de src/manifest.h.

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
            ('shift', 'proteção'), ('route', 'saída'), ('bench', 'bancada')]


def familia(n):
    return next((v for k, v in FAMILIAS if n.startswith(k)), '—')


linhas, faltando = [], []
for cc, expr in sorted(re.findall(r'AddMidi\(\s*(\d+)\s*,\s*([^)]+)\)', src),
                       key=lambda x: int(x[0])):
    e = expr.strip()
    mm = re.match(r'(p_[a-z0-9_]+)(\[\d\])?$', e)
    nome = sym2name.get((mm.group(1), mm.group(2) or '')) if mm else None
    if not nome:
        faltando.append((cc, e))
        continue
    f = faixas.get(nome)
    fx = f'{f[0].rstrip(".")} – {f[1].rstrip(".")}' if f else '—'
    linhas.append(f'| {cc} | `{nome}` | {familia(nome)} | {fx} |')

if faltando:
    print(f'ERRO: {len(faltando)} CC(s) sem nome resolvido: {faltando}',
          file=sys.stderr)
    sys.exit(1)

print(f'<!-- gerado por gerar_ccmap.py de {MANIFEST.name} — nao editar a mao -->')
print('| CC | param | família | faixa real |')
print('|---|---|---|---|')
print('\n'.join(linhas))
print(f'\n<!-- {len(linhas)} CCs -->')
