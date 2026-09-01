#!/usr/bin/env python3
"""gerar_projeto_loopy v0.2.0 — escreve um .lpproj COMPLETO do manifest.h.

    ./gerar_projeto_loopy.py <molde.lpproj> [saida.lpproj]

PROJECTO, NAO PERFIL. Perfil (.lpcontrolprofile) mapeia CC para widgets
que JA EXISTEM; projecto traz os widgets. Ver a skill `loopy`.

⚠️ PARTE DE UM MOLDE, e nao do zero. Um .lpproj precisa de Clock,
Sequence, fontes MIDI, grupos e faixas para sequer abrir — objectos que
nada tem a ver com paginas de controlo. Reconstruir isso as cegas era
adivinhar um formato proprietario inteiro; copiar o molde e substituir
so as paginas mantem o risco onde ele se entende.

⚠️ E NUNCA ESCREVE POR CIMA DO MOLDE. Ficheiro novo, sempre — projecto e
onde vivem horas de arranjo de ecra do dB.

O QUE PRESERVA: a cena de loops (a que tem `tracks`), e todo o resto do
bundle. O QUE REFAZ: as paginas de controlo, todas.
"""
import json, os, re, shutil, sqlite3, sys, tempfile, zipfile, pathlib

VERSAO = "0.2.0"
AQUI = pathlib.Path(__file__).resolve().parent
SRC = (AQUI / ".." / ".." / "src" / "manifest.h").resolve()

# Grelha: cena 5x2, widgets de 0,5 => 10 colunas x 4 linhas por pagina.
COLS, ROWS, W = 10, 4, 0.5
POR_PAGINA = COLS * ROWS

CANAL_ROT = {"kCanalCompart": "ch1", "kCanalEntradaA": "ch2", "kCanalEntradaB": "ch3"}

# Cor por CANAL, porque o canal E o endereco nesta casa: o mesmo CC em
# canais diferentes sao params diferentes, e a cor e o unico aviso que
# chega antes do dedo.
COR = {"ch1": [0.42, 0.49, 0.52, 1],    # ardosia — compartilhado
       "ch2": [0.07, 0.44, 0.37, 1],    # verde-azulado — cadeia A
       "ch3": [0.44, 0.26, 0.51, 1]}    # violeta — cadeia B
COR_PRESET = [0.60, 0.42, 0.13, 1]      # latao

# As paginas, por familia. A ordem aqui e a ordem no Loopy.
PAGINAS = [
    ("central", "ch1", ["NIVEIS", "FONTE", "SAIDA", "DELAY", "SHIFT"]),
    ("tech",    "ch1", ["TOM DE TESTE", "BUZIOS"]),
    ("A-dyn",   "ch2", ["GATE", "DRIVE", "TAROLATOR", "PHASER"]),
    ("B-dyn",   "ch3", ["GATE", "DRIVE", "TAROLATOR", "PHASER"]),
    ("A-space", "ch2", ["REVERB", "SHIMMER", "DELAY", "FREEZE"]),
    ("B-space", "ch3", ["REVERB", "SHIMMER", "DELAY", "FREEZE"]),
    ("A-mod",   "ch2", ["WARBLE", "NOTCH", "ROTEAMENTO", "GOVERNOR", "EXCITERS"]),
    ("B-mod",   "ch3", ["WARBLE", "NOTCH", "ROTEAMENTO", "GOVERNOR", "EXCITERS"]),
]

# ⚠️ FORA DO PAINEL, de proposito. `bench_boost` e controlo de BANCADA:
# um widget para ele so serviria para alguem o mexer em palco.
FORA = ("bench_boost",)

FAMILIAS = [("pre_level", "NIVEIS"), ("post_level", "NIVEIS"), ("notch", "NOTCH"),
            ("test_tone", "TOM DE TESTE"), ("gate", "GATE"), ("drv", "DRIVE"),
            ("phs", "PHASER"), ("wrb", "WARBLE"), ("tarol", "TAROLATOR"),
            ("frz", "FREEZE"), ("rev", "REVERB"), ("shm", "SHIMMER"),
            ("dly", "DELAY"), ("route", "ROTEAMENTO"), ("out", "SAIDA"),
            ("exc", "EXCITERS"), ("piezo", "FONTE"), ("gov", "GOVERNOR"),
            ("buzios", "BUZIOS"), ("shift", "SHIFT"), ("bench", "BANCADA")]


def familia(n):
    return next((v for k, v in FAMILIAS if n.startswith(k)), "OUTROS")


def titulo(n):
    """`notch1_hz_a` -> `Ntc1 Hz A`. O casamento do PERFIL e por titulo,
    entao esta funcao e contrato: mudar aqui reendereca tudo."""
    return " ".join(p.capitalize() for p in n.split("_"))


def le_manifest():
    src = SRC.read_text(encoding="utf-8")
    sym, faixa, defa, passos = {}, {}, {}, {}
    for m in re.finditer(
            r'(p_[a-z0-9_]+)((?:\[\d\])*)\s*=\s*t\.Add\("([a-z0-9_]+)",'
            r'\s*([-0-9.]+)f?,\s*([-0-9.]+)f?,\s*([-0-9.]+)f?'
            r'(?:,\s*([-0-9.]+)f?)?(?:,\s*(\d+))?', src):
        n = m.group(3)
        sym[(m.group(1), m.group(2) or "")] = n
        defa[n] = float(m.group(4))
        faixa[n] = (float(m.group(5)), float(m.group(6)))
        passos[n] = int(m.group(8)) if m.group(8) else 0
    itens = []
    for m in re.finditer(r'AddMidi\(\s*(\d+)\s*,\s*([^,)]+)\s*(?:,\s*(\w+)\s*)?\)', src):
        e = m.group(2).strip()
        mm = re.match(r'(p_[a-z0-9_]+)((?:\[\d\])*)$', e)
        if not mm:
            continue
        n = sym.get((mm.group(1), mm.group(2) or ""))
        if not n or n.startswith(FORA):
            continue
        ch = CANAL_ROT.get(m.group(3) or "kCanalEntradaA", "ch2")
        lo, hi = faixa[n]
        itens.append(dict(cc=int(m.group(1)), ch=ch, nome=n, fam=familia(n),
                          lo=lo, hi=hi, defa=defa[n], passos=passos[n]))
    return itens


def tipo_widget(it):
    n, p = it["nome"], it["passos"]
    if p == 2:
        return "ButtonWidget"
    if p > 2:
        return "SteppedDialWidget"
    if n.endswith("_on") or "_on_" in n or n.startswith(("route_", "out_")):
        return "ButtonWidget"
    return "SliderWidget"


def corpo(it, x, y, cor):
    f = {"_type": "Rect", "_value": [x, y, W, W]}
    c = {"_type": "Color", "_value": cor}
    t = tipo_widget(it)
    if t == "SliderWidget":
        lo, hi = it["lo"], it["hi"]
        v = (it["defa"] - lo) / (hi - lo) if hi > lo else 0.0
        return t, {"frame": f, "title": titulo(it["nome"]),
                   "value": max(0.0, min(1.0, v)), "type": 0, "color": c}
    if t == "ButtonWidget":
        return t, {"frame": f, "state": 1 if it["defa"] >= 0.5 else 0,
                   "title": titulo(it["nome"]), "color": c}
    n = max(2, it["passos"])
    return t, {"frame": f, "title": titulo(it["nome"]), "color": c,
               "count": n, "selectedIndex": int(it["defa"]),
               "titles": {str(i): str(i) for i in range(n)}, "feedbackMode": 2}


def main():
    if len(sys.argv) < 2:
        print(__doc__); return 2
    molde = pathlib.Path(sys.argv[1]).expanduser()
    saida = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else \
        molde.with_name("Cotucatronics-gerado-v2.lpproj")
    if saida.resolve() == molde.resolve():
        print("RECUSADO: a saida nao pode ser o molde."); return 1
    print("gerar_projeto_loopy v%s" % VERSAO)

    itens = le_manifest()
    print("  manifest: %d vinculos (fora do painel: %s)" % (len(itens), ", ".join(FORA)))

    d = tempfile.mkdtemp()
    with zipfile.ZipFile(molde) as z:
        z.extractall(d)
    sq = os.path.join(d, "Project.sqlite")
    c = sqlite3.connect(sq)

    # A cena de LOOPS preserva-se: e a unica com `tracks`, e nada nela e
    # pagina de controlo.
    manter, apagar = [], []
    for i, ct in c.execute("select id,contents from objects where type='Scene'"):
        (manter if json.loads(ct).get("tracks") else apagar).append(i)
    print("  cenas: %d preservadas, %d refeitas" % (len(manter), len(apagar)))

    for i in apagar:
        for w in json.loads(c.execute("select contents from objects where id=?",
                                      (i,)).fetchone()[0]).get("widgets", []):
            c.execute("delete from objects where id=?", (w["_value"],))
        c.execute("delete from objects where id=?", (i,))

    prox = c.execute("select max(id) from objects").fetchone()[0] + 1
    def novo(tipo, dados):
        nonlocal prox
        c.execute("insert into objects (id,type,contents) values (?,?,?)",
                  (prox, tipo, json.dumps(dados, ensure_ascii=False).encode()))
        prox += 1
        return prox - 1

    cenas, avisos = [], []
    for nome, ch, fams in PAGINAS:
        sel = [x for x in itens if x["ch"] == ch and x["fam"] in fams]
        sel.sort(key=lambda x: (fams.index(x["fam"]), x["cc"]))
        if len(sel) > POR_PAGINA:
            avisos.append("%s: %d widgets em %d lugares" % (nome, len(sel), POR_PAGINA))
        refs = []
        for k, it in enumerate(sel):
            t, corpo_ = corpo(it, (k % COLS) * W, (k // COLS) * W, COR[it["ch"]])
            refs.append({"_type": t, "_value": novo(t, corpo_)})
        cenas.append(novo("Scene", {"title": nome, "columns": 5, "rows": 2,
                                    "orientation": 3, "widgets": refs,
                                    "tracks": [], "playGroups": []}))
        print("    %-9s %s  %2d widgets" % (nome, ch, len(sel)))

    # PRESETS: botoes de valor fixo. 8 globais + 16 por cadeia = 40, que
    # e exactamente uma pagina cheia. Nao ha param por tras — o CC 121 e
    # COMANDO, e o valor e o slot.
    refs = []
    for k, (rot, cor) in enumerate(
            [("Preset G%d" % (i + 1), COR_PRESET) for i in range(8)] +
            [("Preset A%d" % (i + 1), COR["ch2"]) for i in range(16)] +
            [("Preset B%d" % (i + 1), COR["ch3"]) for i in range(16)]):
        refs.append({"_type": "ButtonWidget",
                     "_value": novo("ButtonWidget",
                                    {"frame": {"_type": "Rect",
                                               "_value": [(k % COLS) * W, (k // COLS) * W, W, W]},
                                     "state": 0, "title": rot,
                                     "color": {"_type": "Color", "_value": cor}})})
    cenas.append(novo("Scene", {"title": "presets", "columns": 5, "rows": 2,
                                "orientation": 3, "widgets": refs,
                                "tracks": [], "playGroups": []}))
    print("    %-9s      %2d botoes (8 global + 16 A + 16 B)" % ("presets", len(refs)))

    pid, pct = c.execute("select id,contents from objects where type='Project'").fetchone()
    proj = json.loads(pct)
    proj["scenes"] = ([{"_type": "Scene", "_value": i} for i in manter] +
                      [{"_type": "Scene", "_value": i} for i in cenas])
    proj["currentScene"] = {"_type": "Scene", "_value": cenas[0]}
    c.execute("update objects set contents=? where id=?",
              (json.dumps(proj, ensure_ascii=False).encode(), pid))
    c.commit(); c.close()

    if saida.exists():
        saida.unlink()
    # ⚠️ DIRECTORIOS VAZIOS TAMBEM. Um `os.walk` que so escreve ficheiros
    # perde `Control Profiles/` e `Resources/` — que estao vazios no
    # bundle e mesmo assim fazem parte dele. Comparar a lista de nomes
    # do zip de entrada com a do de saida foi o que os apanhou; contar
    # ficheiros nao teria apanhado nada.
    with zipfile.ZipFile(saida, "w", zipfile.ZIP_DEFLATED) as z:
        for r, ds, fs in os.walk(d):
            for dd in ds:
                # TODAS, e nao so as vazias: `Control Profiles/` contem
                # outro directorio, e a condicao "se estiver vazio"
                # saltava-a. O zip de origem tem entrada explicita para
                # cada uma, e a comparacao de nomes foi o que o mostrou.
                p = os.path.join(r, dd)
                z.writestr(os.path.relpath(p, d) + "/", "")
            for f in fs:
                p = os.path.join(r, f)
                z.write(p, os.path.relpath(p, d))
    shutil.rmtree(d)
    for a in avisos:
        print("  ⚠️  %s" % a)
    print("  escrito: %s" % saida)
    return 0


if __name__ == "__main__":
    sys.exit(main())
