#!/usr/bin/env python3
"""gerar_perfil_loopy v0.3.0 — escreve o .lpcontrolprofile a partir do projeto.

    gerar_perfil_loopy.py <Cotucatronics.lpproj> [saida.lpcontrolprofile]

Le o projeto exportado do Loopy Pro (zip com Project.sqlite), casa cada
widget com o seu parametro pelo TITULO, e emite um perfil de controlo
com os 85 bindings ja endereçados.

FORMATO, decodificado em 24/08 e confirmado contra dois bindings feitos
a mao (`b417` e `b314` no perfil exportado):

    Trigger  = [prefixo:] <status><cc>[<valor>]   em hex
               status = 0xB0 | (canal-1)          b0=ch1 b3=ch4 b4=ch5 b5=ch6
               sem valor -> continuo (Absolute)   com valor -> gesto de botao
    Subject  = "<TipoWidget>:<indice da cena>:<posicao entre os do mesmo tipo>"

O indice NAO e a posicao na tela nem na nossa lista: e a ordem do array
`widgets` da Scene, contada por tipo. Foi o que os dois bindings a mao
provaram — Sli:1:2 e Gate Thresh A, Sli:1:6 e Piezo Mix.

⚠️ O endereço e POSICIONAL. Reordenar widgets numa pagina reendereça os
bindings em silencio. Regerar o perfil depois de mexer no layout.
"""
import sqlite3, json, sys, re, os, zipfile, tempfile, plistlib, shutil

CANAL = {'compart': (1, 4), 'a': (2, 5), 'b': (3, 6)}

# titulo normalizado -> (param base, lado)  ·  lado: 'compart' | 'a' | 'b' | 'pag'
# 'pag' = herda o lado da pagina (A-mod -> a, B-time -> b)
TITULOS = {
 'level':('level','compart'), 'piezo_mix':('piezo_mix','compart'),
 'in_gain_a':('in_gain_a','a'), 'in_gain_b':('in_gain_b','b'),
 'gate_thresh_a':('gate_thresh_a','a'), 'gate_thresh_b':('gate_thresh_b','b'),
 'gate_release_a':('gate_release_a','a'), 'gate_release_b':('gate_release_b','b'),
 'buzios':('buzios_on','compart'), 'buzios_thresh':('buzios_thresh','compart'),
 'buzios_cooldown':('buzios_cooldown','compart'),
 'tone_mix':('tone_mix','compart'), 'tone_dest':('tone_dest','compart'),
 # NOMES NOVOS DO PAINEL (28/08). O casamento e por TITULO, entao
 # renomear um widget torna-o orfao na geracao seguinte — e o sintoma
 # seria 'esse controlo deixou de funcionar', sem nada a apontar para
 # a causa. Os titulos antigos ficam: um mapa que aceita os dois nao
 # custa nada e sobrevive a um projeto que nao foi actualizado.
 'test_tone_gain':('tone_mix','compart'),
 'test_tone_freq':('tone_freq','compart'),
 'test_tone_dest':('tone_dest','compart'),
 'test_tone_out':('tone_dest','compart'),   # nome do switch de 3 posicoes
 'test_tone_stage':('tone_via','compart'), # pre (directo) / post (cadeia)
 'test_tone_via':('tone_via','compart'),
 'tone_freq':('tone_freq','compart'), 'shift_hz':('shift_hz','compart'),
 'route_jack':('route_jack','compart'), 'route_exciter':('route_exciter','compart'),
 'exciter_a':('exc_on_a','a'), 'exciter_b':('exc_on_b','b'),
 'gov_ceiling_a':('gov_ceiling_a','a'), 'gov_ceiling_b':('gov_ceiling_b','b'),
 'gov_release_a':('gov_release_a','a'), 'gov_release_b':('gov_release_b','b'),
 'bench_boost_a':('bench_boost_a','a'), 'bench_boost_b':('bench_boost_b','b'),
 'drive_a':('drv_on','pag'), 'drive_b':('drv_on','pag'),
 'drv_drive':('drv_drive','pag'), 'drv_tone':('drv_tone','pag'),
 'drv_dirt':('drv_sujeira','pag'),
 'phaser_a':('phs_on','pag'), 'phaser_b':('phs_on','pag'),
 'phs_rate':('phs_rate','pag'), 'phs_depth':('phs_depth','pag'),
 'phs_fdbck':('phs_feedback','pag'), 'phs_stages':('phs_stages','pag'),
 'warble_a':('wrb_on','pag'), 'warble_b':('wrb_on','pag'),
 'wrb_rate':('wrb_rate','pag'), 'wrb_depth':('wrb_depth','pag'),
 'wrb_irreg':('wrb_irreg','pag'),
 'reverb_a':('rev_on','pag'), 'reverb_b':('rev_on','pag'),
 'rev_size_a':('rev_size','pag'), 'rev_size_b':('rev_size','pag'),
 'rev_damp_a':('rev_damp','pag'), 'rev_damp_b':('rev_damp','pag'),
 'freeze_a':('frz_on','pag'), 'freeze_b':('frz_on','pag'),
 'frz_size_a':('frz_size','pag'), 'frz_size_b':('frz_size','pag'),
 'delay_a':('dly_on','pag'), 'delay_b':('dly_on','pag'),
 'dly_time':('dly_time','compart'),
 'shimmer_a':('shm_on','pag'), 'shimmer_b':('shm_on','pag'),
 'shm_feedback_a':('shm_feedback','pag'), 'shm_feedback_b':('shm_feedback','pag'),
 'shm_tone_a':('shm_tone','pag'), 'shm_tone_b':('shm_tone','pag'),
 'shm_shift_a':('shm_shift','pag'), 'shm_shift_b':('shm_shift','pag'),
}
# "Mix" generico resolve-se pelo ultimo botao de modulo visto na pagina
MODULO_DE = {'drive':'drv_mix', 'phaser':'phs_mix', 'warble':'wrb_mix',
             'reverb':'rev_mix', 'delay':'dly_mix', 'shimmer':'shm_mix',
             'freeze':'frz_vol'}   # o "Mix" do freeze e, no manifest, frz_vol

def norm(s): return re.sub(r'[^a-z0-9]+','_', s.lower()).strip('_')

def ccs_do_manifest(raiz):
    """param -> cc, lido do manifest.h (fonte unica da verdade)."""
    txt = open(os.path.join(raiz,'src','manifest.h')).read()
    m = {}
    for cc, par in re.findall(r'AddMidi\(\s*(\d+)\s*,\s*p_([A-Za-z0-9_]+)', txt):
        par = re.sub(r'\[\s*0\s*\]$','', par)
        m.setdefault(par, int(cc))
    # p_in_gain[0]/[1] aparecem como in_gain — repor o sufixo
    for cc, par, idx in re.findall(r'AddMidi\(\s*(\d+)\s*,\s*p_([A-Za-z0-9_]+)\[\s*(\d)\s*\]', txt):
        m[f"{par}_{'ab'[int(idx)]}"] = int(cc)
    return m

def main():
    lpproj = sys.argv[1]
    saida  = sys.argv[2] if len(sys.argv) > 2 else 'Cotucatronics-gerado.lpcontrolprofile'
    raiz   = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    raiz   = os.path.dirname(raiz) if os.path.basename(raiz)=='docs' else raiz
    tmp = tempfile.mkdtemp()
    zipfile.ZipFile(lpproj).extractall(tmp)
    db  = sqlite3.connect(os.path.join(tmp,'Project.sqlite'))
    obj = {i:(t,b) for i,t,b in db.execute("SELECT id,type,contents FROM objects")}
    proj = json.loads(next(b for i,(t,b) in obj.items() if t=='Project'))
    ccs  = ccs_do_manifest(raiz)

    # o perfil que vive DENTRO do .lpproj usa id; o exportado sozinho, posicao
    dentro_do_projeto = os.environ.get('PERFIL_NO_PROJETO') == '1'
    binds, casados, orfaos = [], [], []
    for n, s in enumerate(proj['scenes']):
        d = json.loads(obj[s['_value']][1])
        lado_pag = 'a' if d['title'].lower().startswith('a-') else \
                   'b' if d['title'].lower().startswith('b-') else None
        cont, ultimo_mod = {}, None
        for w in d['widgets']:
            t, wid = w['_type'], w['_value']
            k = cont.get(t,0); cont[t] = k+1
            if t == 'TextWidget': continue
            tit = json.loads(obj[wid][1]).get('title','')
            nt  = norm(tit)
            for mod in MODULO_DE:
                if nt.startswith(mod): ultimo_mod = mod
            if nt in ('mix_a','mix_b','mix'):
                if not ultimo_mod: orfaos.append((d['title'],tit,'Mix sem módulo antes')); continue
                base, lado = MODULO_DE[ultimo_mod], 'pag'
            elif nt in TITULOS:
                base, lado = TITULOS[nt]
            else:
                orfaos.append((d['title'], tit, 'título não reconhecido')); continue
            if lado == 'pag':
                if not lado_pag: orfaos.append((d['title'],tit,'lado indefinido')); continue
                lado = lado_pag
                base = f"{base}_{lado}"
            par = base
            if par not in ccs:
                orfaos.append((d['title'], tit, f'sem CC no manifest ({par})')); continue
            manda, escuta = CANAL[lado if lado in CANAL else 'compart']
            cc = ccs[par]
            trig = f"b{escuta-1:x}{cc:02x}"
            # IDENTIDADE PERSISTENTE, nao posicao. Descoberto em 24/08 ao
            # abrir o perfil de DENTRO do projeto: os bindings feitos a mao
            # apontavam para "#1522", que e o `id` do widget no SQLite. O
            # `LPSliderWidget:1:6` que aparece no perfil exportado sozinho e
            # uma CONVERSAO — fora do projeto o id nao significa nada.
            # Dentro do projeto o id e estavel: reordenar a pagina nao mexe
            # nele, e foi reordenar que desenderecou dois bindings ontem.
            alvo = f"#{wid}" if dentro_do_projeto else f"LP{t}:{n}:{k}"
            binds.append({'Actions':[{'Identifier':'Trigger Widget',
                          'Parameters':{'Widget Action':'Value'},
                          'Subject': alvo, 'Timing':'Sequential'}],
                          'Trigger': trig})
            casados.append((d['title'], alvo, tit, par, cc, escuta, trig))

    print(f"   casados: {len(casados)}   órfãos: {len(orfaos)}\n")
    if orfaos:
        print("   ---- NÃO CASARAM ----")
        for pag, tit, porque in orfaos: print(f"     {pag:10s} {tit:20s} {porque}")
    print(f"\n   ---- amostra ----")
    for r in casados[:6]:
        print(f"     {r[0]:8s} {r[1]:26s} {r[2]:18s} -> {r[3]:16s} cc{r[4]} ch{r[5]}  {r[6]}")

    plist = {'Bindings': binds, 'Device':'227f559e',
             'Device Name':'Cotucatronics1 Bluetooth', 'Type':'MIDI'}
    os.makedirs(saida, exist_ok=True)
    alvo = os.path.join(saida, '227f559e.MIDI.Cotucatronics1 Bluetooth.controllerprofile')
    with open(alvo,'wb') as f: plistlib.dump(plist, f)
    print(f"\n   escrito: {alvo}  ({len(binds)} bindings)")
    shutil.rmtree(tmp)

main()
