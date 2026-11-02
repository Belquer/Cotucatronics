#!/usr/bin/env python3
"""gerar_perfil_loopy v1.0.1 — escreve o .lpcontrolprofile a partir do projeto.

    gerar_perfil_loopy.py <Cotucatronics.lpproj> <saida.lpproj>

Le o projeto exportado do Loopy Pro (zip com Project.sqlite), casa cada
widget com o seu parametro pelo TITULO, e emite um perfil de controlo
com os 85 bindings ja endereçados.

FORMATO, decodificado em 2026-08-24 e confirmado contra dois bindings feitos
a mao (`b417` e `b314` no perfil exportado):

    Trigger  = [prefixo:] <status><cc>[<valor>]   em hex
               status = 0xB0 | (canal-1)          b0=ch1 b3=ch4 b4=ch5 b5=ch6
               sem valor -> continuo (Absolute)   com valor -> gesto de botao
    Subject  = "<TipoWidget>:<indice da cena>:<posicao entre os do mesmo tipo>"

O indice NAO e a posicao na tela nem na nossa lista: e a ordem do array
`widgets` da Scene, contada por tipo. Foi o que os dois bindings a mao
provaram — Sli:1:2 e Gate Thresh A, Sli:1:6 e Piezo Mix.

🔴 O endereço e o ID do widget (`"#1526"`), NAO a posicao. A versao anterior
deste cabecalho dizia o contrario, a skill `loopy` ja o tinha corrigido a
2026-09-01, e a ferramenta continuou a escrever posicional ate 2026-09-23 — tres
importacoes recusadas com "perfil sem bindings" e nenhuma a dizer porque.

O id e ESTAVEL: reordenar ou apagar widgets numa pagina NAO o muda. Mas
REGERAR as paginas apaga os ids todos, e ai os bindings ficam orfaos sem erro
nenhum — ver a skill, seccao "consequencia que destroi um projecto".
"""
import sqlite3, json, sys, re, os, zipfile, tempfile, plistlib, shutil

CANAL = {'compart': (1, 4), 'a': (2, 5), 'b': (3, 6)}

# titulo normalizado -> (param base, lado)  ·  lado: 'compart' | 'a' | 'b' | 'pag'
# 'pag' = herda o lado da pagina (A-mod -> a, B-time -> b)
TITULOS = {
 'level':('level','compart'), 'piezo_mix':('piezo_mix','compart'),
 'gate_thresh_a':('gate_thresh_a','a'), 'gate_thresh_b':('gate_thresh_b','b'),
 'gate_release_a':('gate_release_a','a'), 'gate_release_b':('gate_release_b','b'),
 'buzios':('buzios_on','compart'), 'buzios_thresh':('buzios_thresh','compart'),
 'buzios_cooldown':('buzios_cooldown','compart'),
 'tone_mix':('tone_mix','compart'), 'tone_dest':('tone_dest','compart'),
 # NOMES NOVOS DO PAINEL (2026-08-28). O casamento e por TITULO, entao
 # renomear um widget torna-o orfao na geracao seguinte — e o sintoma
 # seria 'esse controlo deixou de funcionar', sem nada a apontar para
 # a causa. Os titulos antigos ficam: um mapa que aceita os dois nao
 # custa nada e sobrevive a um projeto que nao foi actualizado.
 'test_tone_gain':('tone_mix','compart'),
 'test_tone_freq':('tone_freq','compart'),
 'test_tone_dest':('tone_dest','compart'),
 'test_tone_out':('tone_dest','compart'),   # nome do switch de 3 posicoes
 'test_tone_stage':('test_tone_stage','compart'), # pre (directo) / post (cadeia)
 'test_tone_via':('test_tone_stage','compart'),   # nome antigo, ainda casa
 'shift_hz':('shift_hz','compart'),
 # ---- CORRIGIDOS a 2026-09-23: apontavam para params que FORAM RENOMEADOS ----
 # `tone_freq`, `tone_mix`, `tone_dest` e `test_tone_stage` nao existem no
 # manifest ha muito. A ferramenta dizia "sem CC no manifest", que se le como
 # "o param nao tem CC" — quando o CC existe e era o MAPA daqui que apontava
 # para um nome morto. Lapide invertida, na tabela de quem procura.
 'test_tone_level':('test_tone_level','compart'),
 'test_tone_freq':('test_tone_freq','compart'),
 'test_tone_out':('test_tone_out','compart'),
 'test_tone_stage':('test_tone_via','compart'),
 'test_tone_via':('test_tone_via','compart'),
 # ---- NOTCH: no manifest chamam-se `notch_*`, no painel do dB "Notch N" ----
 # Sao BIDIMENSIONAIS (ramo x boca) e por isso precisam da chave que nomeia
 # a boca — ver a nota em `ccs_do_manifest`.
 'notch_a':('notch_on_a','compart'), 'notch_b':('notch_on_b','compart'),
 'notch_1_hz_a':('notch_hz_a1','compart'), 'notch_1_gain_a':('notch_db_a1','compart'),
 'notch_1_hz_b':('notch_hz_b1','compart'), 'notch_1_gain_b':('notch_db_b1','compart'),
 'notch_2_hz_a':('notch_hz_a2','compart'), 'notch_2_gain_a':('notch_db_a2','compart'),
 'notch_2_hz_b':('notch_hz_b2','compart'), 'notch_2_gain_b':('notch_db_b2','compart'),
 'notch_3_hz_a':('notch_hz_a3','compart'), 'notch_3_gain_a':('notch_db_a3','compart'),
 'notch_3_hz_b':('notch_hz_b3','compart'), 'notch_3_gain_b':('notch_db_b3','compart'),
 'notch_4_hz_a':('notch_hz_a4','compart'), 'notch_4_gain_a':('notch_db_a4','compart'),
 'notch_4_hz_b':('notch_hz_b4','compart'), 'notch_4_gain_b':('notch_db_b4','compart'),
 # ---- TAROLATOR: o widget chama-se assim, o param `tarol_on_*` ----
 'tarolator_a':('tarol_on_a','a'), 'tarolator_b':('tarol_on_b','b'),
 'tarolator_mix_a':('tarol_mix_a','a'), 'tarolator_mix_b':('tarol_mix_b','b'),
 # MUTE DE RAMO (pos-limiter). Os nomes velhos ainda casam: um perfil
 # exportado antes de 2026-08-28 continua a encontrar o param certo.
 'out_l':('out_l','compart'), 'out_r':('out_r','compart'),
 'route_exciter':('out_l','compart'), 'route_jack':('out_r','compart'),
 # MATRIZ 2x2: qual cadeia entra em qual saida. O canal diz a cadeia.
 'route_l':('route_l','a'), 'route_r':('route_r','a'),
 'route_l_a':('route_l_a','a'), 'route_l_b':('route_l_b','b'),
 'route_r_a':('route_r_a','a'), 'route_r_b':('route_r_b','b'),
 'a to l':('route_l_a','a'), 'a to r':('route_r_a','a'),
 'b to l':('route_l_b','b'), 'b to r':('route_r_b','b'),
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

AMBIGUOS = []   # identificadores que davam CC errado em silencio

def ccs_do_manifest(raiz):
    """param -> cc, lido do manifest.h (fonte unica da verdade).

    ⚠️ INDICE PERDIDO DAVA VINCULO ERRADO EM SILENCIO (2026-09-23).

    A versao anterior capturava `p_([A-Za-z0-9_]+)` e parava no `[`. Para um
    param BIDIMENSIONAL — `p_notch_hz[0][0]`, que tem OITO CCs (3,4,7,8,11,
    12,15,16) — ela guardava uma chave so, e a atribuicao deixava ganhar a
    ULTIMA. Seis identificadores estavam nessa situacao:

        p_notch_db (8 CCs) · p_notch_hz (8) · p_notch_on (2)
        p_gov_ceiling (2)  · p_gov_release (2) · p_bench_boost (2)

    Isto nao produz orfao — produz **vinculo que parece certo e endereca o
    controlo errado**, que e pior: o widget responde, so que ao outro ramo ou
    a outra boca do notch. E nenhuma verificacao a jusante o apanharia.

    Agora as chaves nomeiam o elemento:
        p_X            -> "X"            (so se for inequivoco)
        p_X[i]         -> "X_a" / "X_b"
        p_X[i][j]      -> "X_a1".."X_b4"
    E chave que receberia DOIS CCs diferentes e DESCARTADA e registada em
    AMBIGUOS — um orfao barulhento vale mais que um vinculo mudo e errado.
    """
    txt = open(os.path.join(raiz,'src','manifest.h')).read()
    achados = {}   # chave -> conjunto de CCs

    def poe(chave, cc):
        achados.setdefault(chave, set()).add(int(cc))

    for cc, par, idxs in re.findall(
            r'AddMidi\(\s*(\d+)\s*,\s*p_([A-Za-z0-9_]+)((?:\s*\[\s*\d+\s*\])*)', txt):
        ns = [int(v) for v in re.findall(r'\d+', idxs)]
        poe(par, cc)                                   # nome nu
        if len(ns) >= 1:
            poe("%s_%s" % (par, 'ab'[ns[0]]), cc)      # por cadeia
        if len(ns) >= 2:
            poe("%s_%s%d" % (par, 'ab'[ns[0]], ns[1] + 1), cc)   # por boca

    m = {}
    del AMBIGUOS[:]
    for chave, ccs in sorted(achados.items()):
        if len(ccs) == 1:
            m[chave] = next(iter(ccs))
        else:
            AMBIGUOS.append((chave, sorted(ccs)))
    return m

def main():
    lpproj = sys.argv[1]
    # A extensao e `.lpcontrolprofile` nos DOIS sitios — dentro do .lpproj e
    # exportado. A 2026-09-23 andamos a troca-la por `.lpcontrollerprofile` e a
    # importar tres versoes seguidas: era um DESVIO, e a causa real do "perfil
    # sem bindings" estava no Subject (ver acima). Extensao errada e sintoma
    # visivel; Subject errado e sintoma mudo — e foi o mudo que mandava.
    saida  = sys.argv[2] if len(sys.argv) > 2 else 'Cotucatronics-gerado.lpcontrolprofile'
    raiz   = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    raiz   = os.path.dirname(raiz) if os.path.basename(raiz)=='docs' else raiz
    tmp = tempfile.mkdtemp()
    zipfile.ZipFile(lpproj).extractall(tmp)
    db  = sqlite3.connect(os.path.join(tmp,'Project.sqlite'))
    obj = {i:(t,b) for i,t,b in db.execute("SELECT id,type,contents FROM objects")}
    proj = json.loads(next(b for i,(t,b) in obj.items() if t=='Project'))
    ccs  = ccs_do_manifest(raiz)

    # 🔴 O SUBJECT E SEMPRE `#<id>`, e a crenca contraria custou um dia inteiro.
    #
    # Esta linha dizia: "o perfil que vive DENTRO do .lpproj usa id; o exportado
    # sozinho, posicao" — e punha o modo por id atras de uma variavel de
    # ambiente. **A skill `loopy` corrigiu isto a 2026-09-01**, verificado num export
    # real, e a ferramenta nunca foi actualizada:
    #
    #     Subject = "#<id do objecto do widget>"      ex: "#1526"
    #
    # Provado outra vez a 2026-09-23, comparando lado a lado o perfil que o LOOPY
    # escreveu com o que esta ferramenta escrevia:
    #
    #     Loopy:      "Subject": "#1526"
    #     ferramenta: "Subject": "LPSliderWidget:1:0"
    #
    # Um Subject posicional nao corresponde a widget nenhum, e o Loopy mostra
    # "perfil sem bindings" — sem erro, sem dizer qual. O dB importou tres
    # versoes deste ficheiro a caçar a EXTENSAO, que nunca foi a causa.
    #
    # ⚠️ A licao de metodo: a skill estava certa e a ferramenta contradizia-a.
    # Documento corrigido sem a ferramenta corrigida e uma correccao que nao
    # chegou onde o trabalho acontece.
    dentro_do_projeto = True
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
            # IDENTIDADE PERSISTENTE, nao posicao. Descoberto em 2026-08-24 ao
            # abrir o perfil de DENTRO do projeto: os bindings feitos a mao
            # apontavam para "#1522", que e o `id` do widget no SQLite. O
            # `LPSliderWidget:1:6` que aparece no perfil exportado sozinho e
            # uma CONVERSAO — fora do projeto o id nao significa nada.
            # Dentro do projeto o id e estavel: reordenar a pagina nao mexe
            # nele, e foi reordenar que desenderecou dois bindings ontem.
            alvo = f"#{wid}"
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

    # ---- A IDENTIDADE DO DISPOSITIVO SAI DO PROJECTO, NAO DE UM LITERAL ----
    #
    # ⚠️ Ate 2026-09-23 isto estava cravado:
    #     'Device':'227f559e', 'Device Name':'Cotucatronics1 Bluetooth'
    # Um literal que foi verdade em Agosto. O dB importou o perfil e o Loopy
    # deu-lhe "perfil global sem bindings" — ele LE o ficheiro, nao encontra
    # dono para aquele id, e mostra-o sem controlador. O projecto dele tinha
    # DOIS Cotucatronics emparelhados (`Cotucatronics` e `Cotucatronics1`) e o
    # literal apontava para o que ele nao estava a usar.
    #
    # E o pacote tambem estava errado em FORMA: saia com UM ficheiro sozinho,
    # quando o do Loopy traz um ficheiro POR DISPOSITIVO. Agora copia-se o
    # pacote do projecto inteiro e substituem-se so os dispositivos cujo nome
    # diz Cotucatronics — os outros (pedaleira, etc.) passam intactos.
    perfis = []
    base = os.path.join(tmp, 'Control Profiles')
    if os.path.isdir(base):
        for pacote in sorted(os.listdir(base)):
            d = os.path.join(base, pacote)
            if not os.path.isdir(d): continue
            for f in sorted(os.listdir(d)):
                if f.endswith('.controllerprofile'):
                    perfis.append((pacote, os.path.join(d, f), f))
    alvos = []
    for pacote, caminho, nome in perfis:
        try:
            pl = plistlib.load(open(caminho, 'rb'))
        except Exception:
            continue
        if 'cotucatronics' in str(pl.get('Device Name', '')).lower():
            alvos.append((pacote, nome, pl.get('Device'), pl.get('Device Name')))
    if not alvos:
        # sem molde: cai no comportamento antigo, e DIZ que esta a adivinhar
        print("   (!) o projecto nao traz nenhum perfil de dispositivo com"
              " 'Cotucatronics' no nome — a identidade vai ADIVINHADA")
        alvos = [('Cotucatronics.lpcontrolprofile',
                  '227f559e.MIDI.Cotucatronics1 Bluetooth.controllerprofile',
                  '227f559e', 'Cotucatronics1 Bluetooth')]

    # ---- SAIDA COMO .lpproj: a UNICA que funciona para bindings de widget ----
    #
    # 🔴 DESCOBERTO a 2026-09-23 com dois perfis REAIS que o dB exportou do Loopy:
    #
    #     exportado solto: "Subject": "Playing Tracks" · "Identifier": "Mute"
    #     este gerador:    "Subject": "#1775"          <- id de um WIDGET
    #
    # Perfil exportado sozinho enderea ACCOES GLOBAIS — tocar, parar, mute.
    # Nunca widgets, e nao pode: solto, ele nao sabe de projecto nenhum, e um
    # `#1775` ali nao significa coisa alguma. O Loopy carrega-o e mostra
    # "perfil global sem bindings", que era exactamente o sintoma.
    #
    # ⚠️ Por isso um perfil de widgets NAO SE ENTREGA SOLTO. Tem de voltar
    # para dentro do `.lpproj`, em `Control Profiles/`. Foi por isso que o
    # `.lpproj` sempre funcionou ao dB e o perfil nunca — e eu passei o dia a
    # afinar o invólucro de uma entrega que nao podia funcionar de forma
    # nenhuma. A forma estava certa; o SITIO e que estava errado.
    if saida.endswith('.lpproj'):
        import zipfile as _zf
        if os.path.exists(saida): os.remove(saida)
        alvo_nomes = {nome: (dev, devnome) for _, nome, dev, devnome in alvos}
        # ⚠️ COPIA ENTRADA A ENTRADA DO ARQUIVO, nao anda pelo sistema de
        # ficheiros. Um `os.walk` perde as ENTRADAS DE DIRECTORIO do zip
        # (`Resources/`, `Control Profiles/`…) — o meu primeiro tentame
        # entregou 16 entradas onde o original tinha 20, e um bundle que nao
        # e estruturalmente identico ao que o Loopy escreveu e mais uma
        # variavel a somar a uma investigacao que ja tinha de mais.
        import io as _io
        orig = _zf.ZipFile(lpproj)
        z = _zf.ZipFile(saida, 'w', _zf.ZIP_DEFLATED)
        trocados = 0
        for info in orig.infolist():
            base = info.filename.rstrip('/').split('/')[-1]
            if base in alvo_nomes and 'Control Profiles' in info.filename:
                dev, devnome = alvo_nomes[base]
                plist = {'Bindings': binds, 'Device': dev,
                         'Device Name': devnome, 'Type': 'MIDI'}
                buf = _io.BytesIO(); plistlib.dump(plist, buf)
                z.writestr(info, buf.getvalue())
                trocados += 1
            else:
                z.writestr(info, orig.read(info.filename))
        z.close(); orig.close()
        print("   dispositivos substituidos DENTRO do projecto: %d" % trocados)
        print("   paginas, ids e tudo o resto: INTACTOS")
        print(f"\n   escrito: {saida}  ({len(binds)} bindings)")
        shutil.rmtree(tmp)
        return

    pacote_saida = alvos[0][0]
    raiz_saida = saida
    if not raiz_saida.endswith('.lpcontrolprofile'):
        print("   (!) a saida nao acaba em `.lpcontrolprofile`.")
    if os.path.isdir(raiz_saida): shutil.rmtree(raiz_saida)
    os.makedirs(raiz_saida, exist_ok=True)
    # copia tudo o que havia no pacote de origem (outros controladores)
    origem = os.path.join(base, pacote_saida) if os.path.isdir(base) else None
    copiados = 0
    if origem and os.path.isdir(origem):
        for f in sorted(os.listdir(origem)):
            if f.endswith('.controllerprofile'):
                shutil.copy2(os.path.join(origem, f), os.path.join(raiz_saida, f))
                copiados += 1
    escritos = 0
    print("   (!) saida SOLTA: um perfil de widgets so funciona DENTRO do"
          " .lpproj. Passe uma saida terminada em `.lpproj`.")
    for pacote, nome, dev, devnome in alvos:
        if pacote != pacote_saida: continue
        plist = {'Bindings': binds, 'Device': dev,
                 'Device Name': devnome, 'Type': 'MIDI'}
        with open(os.path.join(raiz_saida, nome), 'wb') as f:
            plistlib.dump(plist, f)
        escritos += 1
        print("   dispositivo: %-28s id %s" % (devnome, dev))
    print(f"\n   escrito: {raiz_saida}  ({len(binds)} bindings"
          f" em {escritos} dispositivo(s), {copiados - escritos} outro(s) intacto(s))")
    shutil.rmtree(tmp)

main()
