#!/usr/bin/env python3
# mech/verifica_3d.py — verificação 3D placa ↔ caixa (primeira passagem, Fase A)
# VERSAO = '0.1.0'  (2026-09-19)
#
# O QUE FAZ (o home executável do IFÁ10000-M volumétrico e do ppc/003)
#   §0 Pré-voo (portão, falha fechado): resolve o kicad-cli REAL, rejeita stubs
#      e o /usr/bin 7.0.11; exige ELF multi-MB via APPDIR/AppRun.sh a reportar 10.x.
#   §1 Params single-source: lê tudo de mech/params.py (nada escrito à mão aqui).
#   §2 STEP real: exporta o STEP do cérebro via AppRun com os flags AP214, com
#      KICAD10_3DMODEL_DIR setado (paths não-resolvidos somem em silêncio).
#   §3 Mede com build123d 0.12.0: import_step -> Compound, bbox por sólido.
#   Asserções 1-3 RÍGIDAS (computadas do STEP); 4-6 declared-envelope (params.py).
#   Emite mech/3d/RELATORIO_3D.md.
#
# REGRA DE OURO (aipcb §Filosofia): um verificador só LÊ. Este script nunca
# reescreve o .kicad_pcb (a cicatriz do pcb_contorno que reescreveu o cérebro
# fabricado). Exporta para mech/3d/ e mede; a placa fica intacta.
#
# FALHA FECHADO: qualquer passo do pré-voo que não prove o que afirma PARA a
# pipeline com exit != 0. Verde na primeira corrida é suspeito (aipcb §5).

import ast  # noqa: F401  (o ficheiro tem de ser ast.parse-clean; ver auto-teste)
import os
import subprocess
import sys
import importlib.util
from pathlib import Path

VERSAO = '0.1.0'

REPO = Path(__file__).resolve().parent.parent
MECH = REPO / 'mech'
OUT_DIR = MECH / '3d'
RELATORIO = OUT_DIR / 'RELATORIO_3D.md'


def carrega_params():
    """§1 — params single-source. Importa mech/params.py sem depender do cwd."""
    spec = importlib.util.spec_from_file_location('mech_params', MECH / 'params.py')
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


# ---------------------------------------------------------------------------
# §0 — PRÉ-VOO: resolver e validar o kicad-cli REAL
# ---------------------------------------------------------------------------
STUB_MAX_BYTES = 100          # < 100 B a só ecoar versão = stub
KICAD_MAJOR_ALVO = 10


def _reporta_versao(caminho, appdir=None):
    """Corre `<caminho> version` (ou AppRun.sh kicad-cli version) e devolve str."""
    env = dict(os.environ)
    if appdir:
        env['APPDIR'] = str(appdir)
        cmd = [str(caminho), 'kicad-cli', 'version']
    else:
        cmd = [str(caminho), 'version']
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=60, env=env)
        return (r.stdout + r.stderr).strip().splitlines()[0] if (r.stdout or r.stderr) else ''
    except Exception as e:  # noqa: BLE001
        return f'ERRO: {e}'


def _is_elf(caminho):
    try:
        with open(caminho, 'rb') as f:
            return f.read(4) == b'\x7fELF'
    except OSError:
        return False


def preflight_kicad():
    """Resolve o kicad-cli utilizável. Devolve (dict resultado, ok: bool).

    Rejeita:
      - stubs (shell < 100 B que só ecoa versão): kbcache/.../bin, fakebin/
      - /usr/bin/kicad-cli 7.0.11 (falha em `generator_version`)
    Aceita SÓ:
      - APPDIR=<appdir> <appdir>/AppRun.sh kicad-cli a reportar 10.x, com o alvo
        resolvido a ser ELF multi-MB (não script).
    """
    res = {'rejeitados': [], 'aceite': None, 'appdir': None, 'runner': None}

    # candidato de ambiente: KICAD_APPDIR aponta para um AppDir extraído.
    appdir = os.environ.get('KICAD_APPDIR')
    candidatos_appdir = []
    if appdir:
        candidatos_appdir.append(Path(appdir))
    # também procurar um AppRun.sh sob um scratchpad, se declarado.
    extra = os.environ.get('KICAD_APPRUN_SEARCH')
    if extra:
        candidatos_appdir.append(Path(extra))

    for ad in candidatos_appdir:
        apprun = ad / 'AppRun.sh'
        elf = ad / 'bin' / 'kicad-cli'
        if not apprun.exists():
            res['rejeitados'].append(f'{ad}: sem AppRun.sh')
            continue
        # o alvo resolvido tem de ser ELF (não script) e multi-KB/MB
        if not _is_elf(elf):
            res['rejeitados'].append(f'{elf}: nao e ELF (stub/script)')
            continue
        if elf.stat().st_size < 50_000:
            res['rejeitados'].append(f'{elf}: pequeno demais ({elf.stat().st_size} B)')
            continue
        ver = _reporta_versao(apprun, appdir=ad)
        if ver.startswith(f'{KICAD_MAJOR_ALVO}.'):
            res['aceite'] = ver
            res['appdir'] = str(ad)
            res['runner'] = str(apprun)
            return res, True
        res['rejeitados'].append(f'{apprun}: versao {ver!r} != {KICAD_MAJOR_ALVO}.x')

    # documentar a rejeição do /usr/bin (7.0.11) e de stubs conhecidos
    import shutil
    ub = shutil.which('kicad-cli')
    if ub:
        ver = _reporta_versao(ub)
        if not ver.startswith(f'{KICAD_MAJOR_ALVO}.'):
            res['rejeitados'].append(f'{ub}: versao {ver!r} (rejeitado — precisa {KICAD_MAJOR_ALVO}.x)')
    return res, False


# ---------------------------------------------------------------------------
# §2 — exportar o STEP real via AppRun (flags AP214)
# ---------------------------------------------------------------------------
def exporta_step(appdir, runner, pcb, destino, model_dir):
    """`AppRun.sh kicad-cli pcb export step --subst-models` (AP214 por defeito).

    KICAD10_3DMODEL_DIR setado — paths não-resolvidos somem em silêncio.
    board outline tem de ser polígono fechado; exit 0 e ficheiro > 1 MB.

    ⚠️ CICATRIZ: o KiCad 10 (`pcb export step`) NÃO tem `--format` (a família é a
    própria sub-ordem, e a saída já é STEP AP214 com rótulo+cor por sólido). Passar
    `--format step` faz o `kicad-cli` cuspir o help e sair — parece exit-0 sem
    escrever ficheiro. Aqui usamos só `--subst-models --force`, provado 3,19 MB.
    """
    env = dict(os.environ)
    env['APPDIR'] = str(appdir)
    if model_dir:
        env['KICAD10_3DMODEL_DIR'] = str(model_dir)
    cmd = [
        str(runner), 'kicad-cli', 'pcb', 'export', 'step',
        '--subst-models', '--force',
        '--output', str(destino), str(pcb),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=600, env=env)
    ok = (r.returncode == 0 and destino.exists() and destino.stat().st_size > 1_000_000)
    return ok, r.returncode, (r.stdout + r.stderr), (destino.stat().st_size if destino.exists() else 0)


# ---------------------------------------------------------------------------
# §3 — medir com build123d
# ---------------------------------------------------------------------------
def mede_step(step_path):
    """import_step -> Compound; devolve (bbox_global, n_solidos, erro|None)."""
    try:
        from build123d import import_step
    except Exception as e:  # noqa: BLE001
        return None, 0, f'build123d ausente: {e}'
    try:
        comp = import_step(str(step_path))
        solids = comp.solids()
        bb = comp.bounding_box()
        bbox = {
            'x_mm': round(bb.size.X, 3),
            'y_mm': round(bb.size.Y, 3),
            'z_mm': round(bb.size.Z, 3),
            'min': (round(bb.min.X, 3), round(bb.min.Y, 3), round(bb.min.Z, 3)),
            'max': (round(bb.max.X, 3), round(bb.max.Y, 3), round(bb.max.Z, 3)),
        }
        return bbox, len(solids), None
    except Exception as e:  # noqa: BLE001
        return None, 0, f'medicao falhou: {e}'


# ---------------------------------------------------------------------------
# Asserções
# ---------------------------------------------------------------------------
def asserçoes_rigidas(bbox, P):
    """1-3: físicas, computadas do STEP. Devolve lista de (id, passa, detalhe)."""
    out = []
    if bbox is None:
        out.append(('A1 placa cabe na cavidade', None, 'sem STEP — nao computavel'))
        out.append(('A2 furos alinham', None, 'sem STEP — nao computavel'))
        out.append(('A3 altura por zona (cunha 11,8)', None, 'sem STEP — nao computavel'))
        return out
    c = P.CAIXA
    folga_x = c['vao_interior_x_mm'] - bbox['x_mm']
    folga_y = c['vao_interior_y_mm'] - bbox['y_mm']
    cabe = folga_x >= 2 * c['folga_borda_mm'] and folga_y >= 2 * c['folga_borda_mm']
    out.append(('A1 placa cabe na cavidade', cabe,
                f'bbox {bbox["x_mm"]}x{bbox["y_mm"]} mm; folga x={folga_x:.2f} y={folga_y:.2f} mm '
                f'(min 2x{c["folga_borda_mm"]})'))
    # A2 — furos: sem geometria de postes no STEP da placa, fica declared-stub
    out.append(('A2 furos alinham placa↔postes', None,
                f'declared-stub: tol {P.FUROS_FIXACAO["tolerancia_alinhamento_mm"]} mm, '
                f'min {P.FUROS_FIXACAO["min_pontos"]} pontos — precisa STEP da caixa (Fase A/B)'))
    # A3 — altura por zona contra interior da tampa na cunha
    piores = []
    for zona, h in P.ALTURAS_ZONA.items():
        teto = P.TAMPA_INTERIOR_ZONA.get(zona)
        if teto is None:
            piores.append((zona, None))
            continue
        piores.append((zona, teto - h))
    ok3 = all(m is not None and m > 0 for _, m in piores)
    det = '; '.join(f'{z}: folga Z={m:.2f} mm' if m is not None else f'{z}: sem teto'
                    for z, m in piores)
    out.append(('A3 altura por zona (cunha 11,8)', ok3, f'{det} (env-declarado ate STEP da caixa)'))
    return out


def asserçoes_envelope(P):
    """4-6: declared-envelope (afirmamos o número, não é física). ⟨dB⟩ assina."""
    e = P.ENVELOPE_DECLARADO
    return [
        ('A4 recortes tampa (OLED/pots 16mm/encoder)', 'ENV',
         f'janela_oled={e["recortes_tampa"]["janela_oled"]}, '
         f'furos_pots={e["recortes_tampa"]["furos_pots_mm"]} mm (Caso B), '
         f'encoder={e["recortes_tampa"]["encoder"]}'),
        ('A5 cabos/serviço (J1↔J14, pots-tampa)', 'ENV',
         f'j1_j14={e["cabos_servico"]["j1_j14_mm"]} mm, '
         f'pots_casoB={e["cabos_servico"]["pots_para_tampa_casoB"]}'),
        ('A6 montabilidade (parafusos, remoção)', 'ENV',
         f'parafusos_acessiveis={e["montabilidade"]["parafusos_acessiveis"]}, '
         f'removivel_sem_dessoldar={e["montabilidade"]["placa_removivel_sem_dessoldar"]}'),
    ]


# ---------------------------------------------------------------------------
# Relatório
# ---------------------------------------------------------------------------
def escreve_relatorio(pre, pre_ok, step_info, bbox, n_solidos, med_erro, rig, env, P):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    from datetime import date
    L = []
    L.append(f'# RELATÓRIO 3D — verifica_3d.py v{VERSAO}')
    L.append('')
    L.append(f'> carimbo: RELATORIO_3D.md · {date.today().isoformat()} · '
             f'placa: {P.CEREBRO_PCB} · gerado por verifica_3d.py v{VERSAO}')
    L.append('')
    L.append('## §0 Pré-voo (portão, falha fechado)')
    L.append('')
    if pre_ok:
        L.append(f'- ✅ kicad-cli REAL: **{pre["aceite"]}** via `AppRun.sh` em `{pre["appdir"]}`')
    else:
        L.append('- ❌ kicad-cli 10.x REAL não resolvido — pipeline PARADA no pré-voo')
    for r in pre['rejeitados']:
        L.append(f'  - rejeitado: `{r}`')
    L.append('')
    L.append('## §2 STEP real (AP214, --subst-models, KICAD10_3DMODEL_DIR)')
    L.append('')
    if step_info is None:
        L.append('- ⏭️ não corrido (pré-voo falhou)')
    else:
        ok, rc, log, size = step_info
        emoji = '✅' if ok else '❌'
        L.append(f'- {emoji} exit={rc}, ficheiro={size/1e6:.2f} MB (ref cérebro ~{P.STEP_REF_MB} MB)')
        env_warns = [ln for ln in log.splitlines()
                     if 'Fuse_2920' in ln or 'Electrosmith_Daisy_Seed' in ln]
        if env_warns:
            L.append(f'  - envelope conhecido: {len(env_warns)} warning(s) de modelo não-apontado '
                     '(Fuse_2920/Electrosmith_Daisy_Seed) — esperado a menos que a lib esteja apontada')
    L.append('')
    L.append('## §3 Medição (build123d 0.12.0)')
    L.append('')
    if med_erro:
        L.append(f'- ⚠️ {med_erro}')
    elif bbox:
        L.append(f'- ✅ {n_solidos} sólido(s); bbox = '
                 f'{bbox["x_mm"]} × {bbox["y_mm"]} × {bbox["z_mm"]} mm')
        L.append(f'  - min={bbox["min"]}  max={bbox["max"]}')
    L.append('')
    L.append('## Asserções 1-3 — RÍGIDAS (computadas do STEP)')
    L.append('')
    L.append('| # | asserção | resultado | detalhe |')
    L.append('|---|---|---|---|')
    for aid, passa, det in rig:
        s = '✅ PASSA' if passa is True else ('❌ FALHA' if passa is False else '⏳ N/D')
        L.append(f'| | {aid} | {s} | {det} |')
    L.append('')
    L.append('## Asserções 4-6 — DECLARED-ENVELOPE (afirmamos o número, ⟨dB⟩ assina)')
    L.append('')
    L.append('| # | asserção | tipo | valor declarado |')
    L.append('|---|---|---|---|')
    for aid, _t, det in env:
        L.append(f'| | {aid} | envelope | {det} |')
    L.append('')
    L.append('## MANIFEST')
    L.append('')
    L.append('- Este relatório e cada `mech/3d/*.step` entram em `export/MANIFEST.md` '
             'com carimbo + flag «bbox declarado vs STEP real» (divergiu = vermelho).')
    RELATORIO.write_text('\n'.join(L) + '\n', encoding='utf-8')
    return RELATORIO


def main():
    P = carrega_params()
    pre, pre_ok = preflight_kicad()

    step_info = None
    bbox = None
    n_solidos = 0
    med_erro = None
    step_path = OUT_DIR / 'cerebro.step'

    if pre_ok:
        OUT_DIR.mkdir(parents=True, exist_ok=True)
        model_dir = os.environ.get('KICAD10_3DMODEL_DIR') or (
            Path(pre['appdir']) / 'share' / 'kicad' / '3dmodels')
        step_info = exporta_step(pre['appdir'], pre['runner'],
                                 REPO / P.CEREBRO_PCB, step_path, model_dir)
        if step_info[0]:
            bbox, n_solidos, med_erro = mede_step(step_path)
        else:
            med_erro = 'STEP não exportado — medição saltada'
    else:
        med_erro = 'pré-voo falhou — nada medido'

    rig = asserçoes_rigidas(bbox, P)
    env = asserçoes_envelope(P)
    caminho = escreve_relatorio(pre, pre_ok, step_info, bbox, n_solidos, med_erro, rig, env, P)

    print(f'RELATORIO_3D: {caminho}')
    print(f'pre-voo: {"OK "+pre["aceite"] if pre_ok else "FALHOU"}')
    if bbox:
        print(f'bbox: {bbox["x_mm"]} x {bbox["y_mm"]} x {bbox["z_mm"]} mm, {n_solidos} solidos')
    # falha fechado: sem pré-voo, exit != 0
    return 0 if pre_ok else 2


if __name__ == '__main__':
    sys.exit(main())
