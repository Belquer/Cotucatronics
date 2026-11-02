#!/usr/bin/env python3
"""gerar_ccmap v0.11.0 — gera a tabela do CCMAP.md a partir de src/manifest.h.

Sem versao ate 2026-08-28; nasce em 0.2.0 e nao em 0.1.0 de proposito, para nao
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
import subprocess
from datetime import datetime

# ⚠️ UMA VERSAO SO, e por isso e uma CONSTANTE e nao texto repetido.
# Ate 2026-09-04 a docstring dizia 0.9.0 e a tabela emitida dizia 0.7.0:
# duas afirmacoes sobre a mesma coisa no mesmo ficheiro, e a que o
# leitor ve era a velha.
VERSAO = "0.11.0"
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
# `(\[\d\])*` e nao `(\[\d\])?`: em 2026-08-31 o notch declarou-se com DOIS
# indices — `p_notch_hz[cadeia][boca]` — e o gerador, que so sabia ler um,
# nao resolveu oito CCs. Ele FALHOU ALTO em vez de emitir tabela torta,
# que e o desenho certo; mas o episodio mostra que o padrao de declaracao
# tambem evolui, e o leitor tem de acompanhar.
for m in re.finditer(r'(p_[a-z0-9_]+)((?:\[\d\])*)\s*=\s*t\.Add\("([a-z0-9_]+)"', src):
    sym2name[(m.group(1), m.group(2) or '')] = m.group(3)

# ---- nome -> (min, max) em unidade real ----
faixas = {}
for m in re.finditer(
        r't\.Add\("([a-z0-9_]+)",\s*([-0-9.]+)f?,\s*([-0-9.]+)f?,\s*([-0-9.]+)f?',
        src):
    faixas[m.group(1)] = (m.group(3), m.group(4))

FAMILIAS = [('piezo', 'mistura'), ('gate', 'entrada'),
            ('drv', 'drive'), ('phs', 'phaser'), ('wrb', 'warble'),
            ('dly', 'delay'), ('tarol', 'tarolator'), ('frz', 'freeze'), ('rev', 'reverb'),
            ('shm', 'shimmer'), ('buzios', 'búzios'), ('tone', 'tom'),
            ('pre_level_ab', 'saída'), ('exc', 'saída'), ('gov', 'proteção'),
            ('shift', 'proteção'), ('route', 'saída'), ('bench', 'bancada'),
            ('out_', 'saída'), ('vol_', 'saída'), ('test_tone', 'tom')]


# ==================================================================
# NOTAS — a parte DURAVEL, e a unica escrita a mao.
#
# O cartao A4 (`docs/ccmap.html`) era mantido a mao inteiro, e por isso
# ficou com 27 numeros de CC que ja nao existiam: era anterior ao esquema
# de seis canais e ninguem notou, porque um PDF nao compila, nao roda e
# nao falha. Aposenta-lo perdia estas frases; rege-lo do zero, tambem.
#
# A divisao que resolve: o que MUDA (numero de CC, canal, quais params
# existem) sai do manifest; o que DURA (a nota humana) e dado, aqui.
# Nota orfa — de param que ja nao existe — e denunciada ao gerar.
NOTAS = {
    "piezo_mix": "<b>64 = as duas cadeias cheias</b> · 0 = só A · 127 = só B",
    "gate_thresh_a": "<b>em 0 o gate é PULADO</b>",
    "gate_release_a": "50…2000 ms",
    "drv_drive": "1,5…48×",
    "drv_tone": "800…16k Hz",
    "phs_mix_a": "<b>MONTANHA:</b> pico em ~64",
    "phs_rate": "0,02…4 Hz",
    "phs_depth": "",
    "phs_fb": "",
    "phs_stages": "4/6/8",
    "wrb_rate": "0,05…3 Hz",
    "wrb_depth": "",
    "wrb_irreg": "",
    "dly_on_a": "envio da cadeia A",
    "dly_time": "50 ms…1 s · feedback fixo 0,4",
    "frz_on_a": "qual cadeia alimenta",
    "frz_size": "20…165 ms",
    "rev_on_a": "<b>o envio</b> da cadeia A",
    "rev_mix_a": "quanto entra no espaço",
    "shm_mix_a": "<b>só a camada direta</b>",
    "shm_feedback_a": "teto 0,85",
    "shm_shift_a": "0=+12 · 64=+7 · 127=ambos",
    "shm_tone_a": "<b>sobe pra clarear</b>",
    "buzios_cooldown": "10…1000 ms · 2 batidas rápidas = 1",
    "test_tone_freq": "80…2000 Hz · nasce em 848",
    "post_level_l": "<b>volume do ramo L</b> — POT físico em A0 (D15)",
    "post_level_r": "<b>volume do ramo R</b> — POT físico em A1 (D16)",
    # (out_l / out_r sairam do mapa a 2026-09-22 — uma fonte de verdade por
    #  controlo, e a chave fisica e a fonte. A nota foi com eles: nota de um
    #  vinculo que ja nao existe e lapide invertida, e este gerador acusa-a.)
    "tarol_on": "<b>TAROLATOR</b> — ruído percussivo com o envelope da pancada; nasceu oitavador e não faz oitava",
    "tarol_mix": "0…4× — 100% é o máximo que o efeito gera; acima disso o limiter grampeia",
    "test_tone_out": "<b>0=A+B</b> · 1=A · 2=B",
    "exc_filter_l": "<b>filtra o exciter: shift + governor</b>",
    "shift_hz": "<b>compartilhado</b> pelos dois ramos",
    "bench_boost_l": "bancada — NUNCA a knob nem a cena"
}

MENTIRAS = []

CSS = """@page { size: A4; margin: 8mm 7mm; }
* { box-sizing: border-box; }
body { font: 9.4pt/1.3 -apple-system,"Helvetica Neue",Arial,sans-serif;
       color:#111; margin:0; }
h1 { font-size:16pt; margin:0 0 1mm; letter-spacing:-.3px; }
.sub { font-size:8pt; color:#666; margin:0 0 3mm; }
.canais { font-size:8pt; background:#111; color:#fff; padding:1.6mm 2mm;
          border-radius:2px; margin:0 0 3.5mm; line-height:1.5; }
.canais b { color:#ffd76d; }
.cols { column-count:3; column-gap:4.5mm; }
section { break-inside:avoid; margin:0 0 2.8mm; }
h2 { font-size:9.2pt; margin:0 0 .9mm; padding:.9mm 1.6mm; background:#111;
     color:#fff; border-radius:2px; letter-spacing:.3px; }
table { width:100%; border-collapse:collapse; }
td { padding:.7mm 1.6mm; vertical-align:top; border-bottom:.3pt solid #e6e6e6; }
td.cc { width:8mm; font-weight:700; text-align:right;
        font-variant-numeric:tabular-nums; }
td.nm { width:26mm; font-family:"SF Mono",Menlo,monospace; font-size:8pt; }
td.ds { font-size:8pt; color:#333; }
tr.par td.cc { color:#8a1f11; }
footer { display:block; break-inside:avoid; margin-top:2mm; padding-top:1mm;
         border-top:.4pt solid #ccc; font-size:7pt; line-height:1.4;
         color:#666; }
footer b { color:#111; }"""


def html(linhas_dados, versoes, ncc):
    """Emite o cartao A4. `linhas_dados` = (cc, ch_manda, ch_escuta, nome,
    familia, faixa), na ordem do manifest."""
    por_fam = {}
    for cc, cm, ce, nome, fam, faixa in linhas_dados:
        por_fam.setdefault(fam, []).append((cc, cm, ce, nome, faixa))
    vivos = set(l[3] for l in linhas_dados)
    vivos |= {re.sub(r'_[ab]$', '', x) for x in vivos}
    orfas = sorted(set(NOTAS) - vivos)

    # NOTA QUE DIZ "COMPART." NUM PARAM PAREADO e a mesma familia do
    # `frz_size` que dizia 20-400 com o manifest em 20-165: o NOME esta
    # certo, entao o verificador de fantasmas passa por ela, e a frase
    # descreve uma topologia revogada. O pivo de duas cadeias tornou
    # pareados varios params que eram compartilhados, e as notas ficaram.
    global MENTIRAS
    MENTIRAS = []
    for cc, cm, ce, nome, fam, faixa in linhas_dados:
        base = re.sub(r'_[ab]$', '', nome)
        nota = NOTAS.get(nome) or NOTAS.get(base, '')
        if 'compart' in nota.lower() and cm != 'ch1':
            MENTIRAS.append('%s (cc%d): nota diz "compart." mas e PAREADO'
                            % (base, cc))

    out = ['<!DOCTYPE html><html lang="pt-BR"><head><meta charset="utf-8">',
           '<title>Cotucatronics OS — Mapa de CC</title>',
           '<style>%s</style></head><body>' % CSS,
           '<h1>Cotucatronics OS — Mapa de CC</h1>',
           '<p class="sub">GERADO por <code>gerar_ccmap.py</code> de '
           '<code>manifest.h</code> — nao editar a mao. %s · '
           '<b>%d vinculos em %d numeros de CC</b></p>' % (versoes, len(linhas_dados), ncc),
           '<div class="canais"><b>MANDA</b> ch1 compartilhados · ch2 cadeia A · '
           'ch3 cadeia B &nbsp;&nbsp; <b>ESCUTA</b> ch4 · ch5 · ch6'
           '<br>A regra e uma so: <b>escuta = manda + 3</b>. O mesmo numero de CC '
           'em todos os seis; o CANAL diz o destino.</div>',
           '<div class="cols">']
    for fam in sorted(por_fam, key=lambda f: por_fam[f][0][0]):
        out.append('<section><h2>%s</h2><table>' % fam.upper())
        # COLAPSA O PAR NUMA LINHA. `drv_drive_a` e `drv_drive_b` partilham o
        # numero de CC, a nota e a faixa — so o CANAL os distingue, e isso
        # ja esta na legenda do topo. Duas linhas quase iguais gastavam
        # metade do cartao e empurravam-no para uma segunda folha, que e
        # justamente o que um cartao de referencia nao pode ser.
        colapsado, vistos = [], set()
        for cc, cm, ce, nome, faixa in por_fam[fam]:
            base = re.sub(r'_[ab]$', '', nome)
            if base != nome:
                if (cc, base) in vistos:
                    continue
                vistos.add((cc, base))
                nome = base + '_a/b'
            colapsado.append((cc, cm, ce, nome, faixa))
        for cc, cm, ce, nome, faixa in colapsado:
            # A NOTA PERTENCE A FAMILIA, nao ao ramo. Depois do pivo de
            # duas cadeias, `drv_drive` virou `drv_drive_a`/`_b`; duplicar
            # a nota junto criaria duas copias para divergirem no primeiro
            # ajuste — a mesma razao de a biblioteca de presets ser UMA.
            ds = (NOTAS.get(nome)
                  or NOTAS.get(re.sub(r'(_[ab])?(/b)?$', '', nome), ''))
            # FAIXA SO QUANDO A NOTA NAO FALA. Onde a nota ja diz "1…8×",
            # acrescentar "1 – 8" e a mesma informacao escrita duas vezes,
            # e duas escritas da mesma coisa divergem no primeiro ajuste.
            if not ds and faixa and faixa != '0 – 1':
                ds = faixa
            par = ' class="par"' if cm != 'ch1' else ''
            out.append('<tr%s><td class="cc">%d</td><td class="nm">%s</td>'
                       '<td class="ds">%s</td></tr>' % (par, cc, nome, ds))
        out.append('</table></section>')
    out.append('</div><footer>')
    out.append('<b>CC em vermelho = PAREADO</b>: existe em ch2 (A) e ch3 (B) com o '
               'mesmo numero. CC preto = compartilhado, so ch1.<br>')
    out.append('<b>Comando fora da faixa de param:</b> 120 sync · 121 carregar '
               'preset · 122 gravar (valor = numero do slot; o canal diz o destino).<br>')
    if orfas:
        out.append('<b>NOTAS ORFAS</b> (param ja nao existe): %s' % ', '.join(orfas))
    out.append('</footer></body></html>')
    return '\n'.join(out)


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

linhas, faltando, dados = [], [], []
achados = re.findall(r'AddMidi\(\s*(\d+)\s*,\s*([^,)]+)\s*(?:,\s*(\w+)\s*)?\)',
                     src)
for cc, expr, canal in sorted(achados,
                              key=lambda x: (int(x[0]), CANAIS.get(x[2], 1))):
    e = expr.strip()
    mm = re.match(r'(p_[a-z0-9_]+)((?:\[\d\])*)$', e)
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
    dados.append((int(cc), f'ch{ch}', f'ch{ch + DESLOC_ESCUTA}',
                  nome, familia(nome), fx))

if faltando:
    print(f'ERRO: {len(faltando)} CC(s) sem nome resolvido: {faltando}',
          file=sys.stderr)
    sys.exit(1)

ncc = len({d[0] for d in dados})
tabela = ("<!-- gerado por gerar_ccmap v%s de %s — nao editar a mao -->\n"
          "| CC | manda | escuta | param | família | faixa real |\n"
          "|---|---|---|---|---|---|\n%s\n\n"
          "<!-- %d vinculos em %d numeros de CC -->"
          % (VERSAO, MANIFEST.name, "\n".join(linhas), len(dados), ncc))

if "--stdout" in sys.argv:
    print(tabela)
    sys.exit(0)

# ---- ESCREVE OS DOIS DESTINOS ----
#
# Ate 2026-08-28 este script so IMPRIMIA, e quem o chamava colava o resultado
# no CCMAP.md a mao. O cartao A4 nem isso: era mantido inteiro a mao, e
# ficou com 27 numeros de CC que ja nao existiam — anterior ao esquema de
# seis canais, e ninguem notou porque um PDF nao compila e nao falha.
#
# Gerador que nao ESCREVE deixa a ultima milha para a disciplina humana,
# e e exatamente ai que o documento se descola do codigo.
def carimbo(nvinc, ncc, ver_manifest):
    """Data, contagens, firmware e limpeza da arvore.

    ⚠️ O CARIMBO E DO GERADOR, NAO DA LEMBRANCA DE QUEM O CORRE. Pedido do
    dB, 2026-09-04: "sempre adiciona a data a lista de CCs pra eu saber quando
    ela foi gerada". Ate aqui o cabecalho do CCMAP.md era escrito a mao, e
    ficou a dizer "87 vinculos em 50 numeros de CC / Firmware v0.177.0"
    enquanto o fonte tinha 119 em 81 — sem data, ninguem podia sequer
    suspeitar de quao velho era.

    E diz mais do que o dia: de que COMMIT saiu e se a arvore esta suja. Um
    mapa gerado sobre `manifest.h` por commitar descreve codigo que mais
    ninguem tem.
    """
    quando = datetime.now().astimezone().strftime("%Y-%m-%d %H:%M %Z")
    raiz = pathlib.Path(__file__).resolve().parents[2]
    rel = "src/manifest.h"
    try:
        sha = subprocess.run(["git", "-C", str(raiz), "log", "-1", "--format=%h",
                              "--", rel], capture_output=True, text=True).stdout.strip()
        sujo = subprocess.run(["git", "-C", str(raiz), "status", "--porcelain",
                               "--", rel], capture_output=True, text=True).stdout.strip()
    except OSError:
        sha, sujo = "?", ""
    L = ["<!-- CARIMBO gerar_ccmap -->",
         "> **GERADO do `src/manifest.h`** por `gerar_ccmap.py v%s` em **%s**."
         % (VERSAO, quando),
         "> Nao editar a mao: rode `python3 docs/operacao/gerar_ccmap.py`.",
         ">",
         "> **%d vinculos em %d numeros de CC** · `%s` @ `%s` · manifest %s"
         % (nvinc, ncc, rel, sha or "?", ver_manifest),
         ">",
         "> Seis canais: manda `ch1`/`ch2`/`ch3`, escuta `ch4`/`ch5`/`ch6`"
         " (escuta = manda + 3)."]
    if sujo:
        L += [">", "> ⚠️ **ARVORE SUJA** — `%s` tem alteracoes por commitar." % rel,
              "> Este mapa descreve codigo que mais ninguem tem."]
    L.append("<!-- FIM CARIMBO -->")
    return L


AQUI = pathlib.Path(__file__).resolve().parent
md   = AQUI / "CCMAP.md"
t    = md.read_text(encoding="utf-8").split("\n")
i = next(k for k, x in enumerate(t) if "gerado por gerar_ccmap" in x)
# ⚠️ A SENTINELA DE FIM PROCURA-SE DEPOIS DO INICIO, e a razao custou o
# ficheiro. O carimbo — que nasceu DEPOIS desta funcao — escreve "N vinculos
# em M numeros de CC" no cabecalho, LINHA 6. O `next()` sem limite achava
# essa, que esta ANTES do `i`, e `t[:i] + tabela + t[j+1:]` re-anexava o
# corpo inteiro. Cada corrida duplicava a tabela, em silencio, num ficheiro
# gerado que ninguem le ate ao fim. Estavam la TRES copias, duas delas com
# numeros velhos.
#
# Texto novo que colide com uma sentinela existente e a familia de sempre:
# a sentinela supunha ser unica, e deixou de ser sem ninguem lhe tocar.
j = next(k for k, x in enumerate(t) if k > i and "vinculos em" in x)
t = t[:i] + tabela.split("\n") + t[j + 1:]

mv = re.search(r'manifest\.h[^\n]*?v(\d+\.\d+\.\d+)', src)
fw = "v%s" % (mv.group(1) if mv else "?")
ci = next((k for k, x in enumerate(t) if "<!-- CARIMBO gerar_ccmap -->" in x), None)
cj = next((k for k, x in enumerate(t) if "<!-- FIM CARIMBO -->" in x), None)
if ci is None:                      # primeira vez: entra logo abaixo do titulo
    ci = cj = next(k for k, x in enumerate(t) if x.startswith("# ")) + 1
    t.insert(ci, "")
    cj = ci
t = t[:ci] + carimbo(len(dados), ncc, fw) + t[cj + 1:]
md.write_text("\n".join(t), encoding="utf-8")
print("escrito: %s" % md.name)
card = (AQUI / ".." / "ccmap.html").resolve()
card.write_text(html(dados, "manifest %s · %s" % (fw, datetime.now().strftime("%Y-%m-%d")),
                     ncc), encoding="utf-8")
print("escrito: %s" % card.name)
vivos = set(d[3] for d in dados)
vivos |= {re.sub(r'_[ab]$', '', x) for x in vivos}
orfas = sorted(set(NOTAS) - vivos)
for m in sorted(set(MENTIRAS)):
    print("AVISO: %s" % m)
if orfas:
    print("AVISO: %d nota(s) de param que ja nao existe: %s"
          % (len(orfas), ", ".join(orfas)))
print("%d vinculos em %d numeros de CC" % (len(dados), ncc))
