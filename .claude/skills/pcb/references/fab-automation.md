# Automação de fabrico KiCad → JLCPCB

Referência dos **estágios 4 e 8** da pipeline `pcb`. Caminho headless e
scriptável do esquema a um pacote de fabricação que a JLCPCB aceita. Alvo:
**KiCad 8/9/10**. ⚠️ Confere sempre o que é sensível à versão contra
`kicad-cli <sub> --help` no binário instalado — o help é a autoridade para o teu
build. E lembra o portão do **estágio 0**: a versão MAIOR do KiCad tem de ser
igual ao token `(version …)` dos ficheiros.

Fontes: KiCad CLI docs (`docs.kicad.org/9.0/en/cli/cli.html`,
`.../10.0/...`) · JLCPCB capabilities (`jlcpcb.com/capabilities/pcb-capabilities`)
· gist de DRC JLC (`gist.github.com/darkxst/f713268e5469645425eed40115fb8b49`) ·
IPC-2221 (`resources.altium.com/p/ipc-2221-calculator-pcb-trace-current-and-heating`)
· Freerouting CLI (`github.com/freerouting/freerouting`).

> No repo, o **`kicad/para_fabrica.py`** já embrulha tudo o que segue e recusa-se
> a emitir o pacote se algo estiver mau. Este documento é o cheat-sheet de fundo;
> a pipeline **delega** ao `para_fabrica.py`.

---

## 0. KiCad ausente no ambiente? Bootstrap por AppImage

⚠️ **Antes de tudo isto tens de ter um `kicad-cli` cuja versão MAIOR bata com o
token `(version …)` dos ficheiros** (portão do estágio 0). Se o ambiente não
tiver KiCad — ou só tiver um velho demais — a pipeline **não está bloqueada**:
há um plano-B provado.

**Cadeia de recurso (o que falhou vs. o que passou, sessão 2026-09-10):**

| Fonte | Resultado |
|---|---|
| `apt` | só KiCad **7** — velho demais para ficheiros v10 |
| conda-forge | **não tem** pacote `kicad` |
| PPA oficial do KiCad | **bloqueado** pelo proxy de egresso (403) |
| `downloads.kicad.org` / `kicad.org` / sourceforge | **bloqueados** (403) |
| **AppImage numa release do GitHub** | ✅ **passa** o proxy (`/releases/download/…`) |

O proxy deixa passar as URLs `github.com/<owner>/<repo>/releases/download/<tag>/<asset>`
(redirecionam para `objects.githubusercontent.com`, alcançável). As páginas web
do GitHub e a `api.github.com` continuam gated — usa a URL de download direta.

**Comando (embrulhado em `scripts/bootstrap_kicad.sh`):**
```bash
KCLI=$(.claude/skills/pcb/scripts/bootstrap_kicad.sh 10)   # 10 = MAIOR requerido
"$KCLI" version                                            # -> 10.0.6
```
O script é **idempotente** (reutiliza download/árvore extraída), **verifica** o
download (HTTP-ok + tamanho mínimo, e sha256 se lhe passares `KICAD_APPIMAGE_SHA256`)
e **nunca prossegue em silêncio** sobre um download parcial. URL e versão são
parametrizáveis (`KICAD_APPIMAGE_URL`, arg/`KICAD_MAJOR`).

**Asset provado:** `pkgforge-dev/KiCad-AppImage`, `KiCad-10.0.6-1-anylinux-x86_64.AppImage`
(~487 MB), extraído com `--appimage-extract` (**sem FUSE**). Provado: `kicad-cli
version` → 10.0.6, e faz o **parse de um board `(version 20260206)`** (o DRC correu).

**Notas de execução:**
- `squashfs-root/bin/kicad-cli` é um lançador **`sharun`**: corre-o pelo **caminho
  real** que o script imprime — **SEM `LD_LIBRARY_PATH`** (ele resolve as libs
  empacotadas pelo próprio argv0).
- Aviso **benigno** em cada corrida, ignora-o:
  `Error: schema file '.../kicad/schemas/api.v1.schema.json' not found` — é o
  schema da IPC-API, nada tem a ver com o parse do board; o DRC/ERC funcionam.
- Exit code 5 num `pcb drc --exit-code-violations` = **encontrou violações**
  (sucesso do gate), não falha de parse.

> ⚠️ **CAVEAT — é um build da COMUNIDADE, não o oficial do KiCad.** É o plano-B.
> Quando o oficial (apt/PPA/`downloads.kicad.org`) estiver alcançável e der a
> versão MAIOR certa, **prefere-o sempre**. Depois do bootstrap, **volta a correr
> o portão de versão do estágio 0** (MAIOR do `kicad-cli` == token dos ficheiros).

Detalhe e parâmetros: **`scripts/bootstrap_kicad.sh`** (cabeçalho comentado).

---

## 1. `kicad-cli` — cheat-sheet (tudo headless)

### ERC (esquema) — portão de build
```bash
kicad-cli sch erc --output erc.rpt --format report \
  --severity-error --exit-code-violations board.kicad_sch
```

### DRC (placa) — portão de build
```bash
kicad-cli pcb drc --output drc.json --format json \
  --schematic-parity --all-track-errors \
  --severity-error --exit-code-violations board.kicad_pcb
```
⚠️ Ambos saem **exit 0 mesmo com violações** salvo `--exit-code-violations`. O
DRC usa as regras da placa **mais** qualquer `.kicad_dru` (§2).

### Gerbers (set 2 camadas JLCPCB)
```bash
kicad-cli pcb export gerbers --output gerbers/ \
  --layers F.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts \
  --use-drill-file-origin --subtract-soldermask board.kicad_pcb
```
JLCPCB lê extensões Protel **e** nomes X2 `.gbr` — deixa Protel ligado, envia um
set consistente. Edge.Cuts é **obrigatório**. 4 camadas: acrescenta
`In1.Cu,In2.Cu`.

### Drill (Excellon — default JLC)
```bash
kicad-cli pcb export drill --output gerbers/ --format excellon \
  --drill-origin plot --excellon-units mm --generate-map board.kicad_pcb
```
⚠️ **Mesma origem** para gerbers e drill (`--use-drill-file-origin` +
`--drill-origin plot`) ou os furos saem deslocados.

### CPL / pick-and-place
```bash
kicad-cli pcb export pos --output cpl.csv --format csv --units mm \
  --side both --smd-only --exclude-dnp board.kicad_pcb
```
JLC quer colunas `Designator, Mid X, Mid Y, Layer, Rotation`; o KiCad emite
`Ref, PosX, PosY, Side, Rot` e `front`/`back` — remapear para `Top`/`Bottom`.
Rotações frequentemente precisam de correção por footprint (dor conhecida — ver
`parts-sourcing.md` §5).

### BOM (nativo, 7+/9)
```bash
kicad-cli sch export bom --output bom.csv \
  --fields 'Value,Reference,Footprint,LCSC' \
  --labels 'Comment,Designator,Footprint,LCSC Part #' \
  --group-by 'Value,Footprint,LCSC' --exclude-dnp board.kicad_sch
```

---

## 2. `.kicad_dru` — regras JLC como portão de DRC

Põe `<projecto>.kicad_dru` ao lado da placa (o KiCad carrega-o; o `kicad-cli pcb
drc` honra-o). Regras orientadas à JLCPCB (2 camadas standard):

```
(version 1)
(rule "min_trace_width"   (constraint track_width   (min 0.127mm)))   # 5 mil
(rule "min_clearance"     (constraint clearance     (min 0.127mm)))   # 5 mil
(rule "min_annular"       (constraint annular_width (min 0.13mm)))
(rule "min_via_hole"      (constraint hole_size     (min 0.2mm)))
(rule "min_via_diameter"  (constraint via_diameter  (min 0.45mm)))
(rule "hole_to_hole"      (constraint hole_to_hole  (min 0.5mm)))
(rule "edge_clearance"    (constraint edge_clearance(min 0.3mm)))
(rule "silk_over_pad"
  (constraint silk_clearance (min 0.15mm)) (layer outer)
  (condition "A.Type == 'Text' && B.Type == 'Pad'"))
```
4 camadas: relaxa o cobre interno a 3,5 mil (`(layer inner)` + `0.0889mm`). O
ruleset testado da comunidade (darkxst gist) cobre 2/4 camadas, silk-over-copper,
silk-off-board, edge, hole-to-hole, castellated — copia e apara.

---

## 3. Largura de pista & dimensionamento de via (IPC-2221)

```
I = k · ΔT^0.44 · A^0.725   →   A(mil²) = ( I / (k·ΔT^0.44) )^(1/0.725)
largura(mil) = A / espessura ;  k=0.048 externo, k=0.024 interno
1 oz Cu = 1.378 mil ≈ 0.035 mm
```

### Tabela — 1 oz Cu, ΔT = 10 °C
| Corrente | Externa | Interna |
|---|---|---|
| 0.5 A | 0.13 mm (5 mil) | 0.30 mm (12 mil) |
| 1 A | 0.30 mm (12 mil) | 0.75 mm (30 mil) |
| 2 A | 0.80 mm (31 mil) | 2.0 mm (80 mil) |
| 3 A | 1.4 mm (55 mil) | 3.6 mm (140 mil) |
| 5 A | 2.8 mm (108 mil) | 7.2 mm (285 mil) |

Interna precisa de ~2× a largura para a mesma corrente; 2 oz Cu = metade da
largura; ΔT=20 °C permite ~30% mais estreito.

**Defaults práticos:** sinal 0,2–0,25 mm (bem acima do mín JLC 5 mil); potência
pela tabela + margem de inrush; **GND é plano, não trilho**. ⚠️ As larguras vivem
nas **classes de rede** (`kicad/classes_de_rede.py`) — assim o DRC e qualquer
router as obrigam.

### Corrente por via
Barril ≈ trilho de área `π·d_furo·t_plating` (t≈0,020–0,025 mm) na mesma fórmula:
**~1–2 A por via de 0,3 mm** a ΔT 10 °C. Regra de bolso: **~1 A por via de
0,3 mm**; **costurar N vias em paralelo** para potência. Furo mínimo barato ≥
0,3 mm (JLC vai a 0,2). Via não deve ser um estrangulamento: furo ≈ 0,75× a
largura do trilho (1 oz).

---

## 4. Freerouting (KiCad 9/10)

**Realidade: rotear à mão ou por script a potência/terra e tudo o que é sensível.**
O autorouter é só para preenchimento de sinal denso, e sempre revisto. No repo o
`kicad/roteia_fr.py` faz o round-trip por `pcbnew` (ExportSpecctraDSN/
ImportSpecctraSES) + `freerouting.jar`.

```bash
java -jar freerouting-2.x.jar -de board.dsn -do board.ses \
  -mp 100 -mt 1 -oit 0.1 -da
```
⚠️ Cuidados: **telefona para casa** salvo `-da`; **`-mt > 1` pode violar
constraints** (usa `-mt 1`); **as classes de rede carregam as larguras** (o router
só sabe o que o DSN codifica — define as classes ANTES do export); keepouts/
net-ties imperfeitos (verifica pours e re-corre o DRC depois de importar).
⚠️ **Não confies no comprimento TOTAL** — mede por rede (uma rede crítica pode
desviar-se enquanto o total melhora).

---

## 5. Pacote de fabrica reprodutível

Zip do bare board: `*_F_Cu.gbr` (top) · `*_B_Cu` (bottom) · `*_F/B_Mask` ·
`*_F/B_Silkscreen` · `*_F/B_Paste` (só com stencil) · `Edge_Cuts` (**obrigatório**)
· `*.DRL (+ -NPTH.DRL)`. Montagem: `BOM.csv` + `CPL.csv`.

### Checklist DFM antes do upload
- [ ] ERC limpo (`--exit-code-violations`).
- [ ] DRC limpo incl. `--schematic-parity`.
- [ ] `.kicad_dru` JLC passa.
- [ ] **Zonas re-preenchidas depois da última edição** (fill velho embarca calado).
- [ ] Edge.Cuts fechado, contorno único, tamanho certo.
- [ ] Silk: stroke ≥0,15 mm, altura ≥1 mm; nenhum sobre pad ou fora da placa.
- [ ] Gerber & drill partilham a **mesma origem**.
- [ ] Abrir os gerbers num visualizador — **olhos no output**, não só no DRC.
- [ ] Camadas no zip = camadas encomendadas (2 vs 4).
- [ ] PCBA: LCSC válidos e em stock; **rotações do CPL conferidas ⟨dB⟩ peça-a-peça**;
      DNPs fora de BOM/CPL.

O `para_fabrica.py` carimba **data/commit/árvore-suja** e escreve `INCOMPLETA` no
nome se forçado — um pacote incompleto nunca se confunde com um bom.

### Onde o pacote fica — `export/` é a entrega, `fabrica/` é scratch
O `para_fabrica.py` escreve a cada corrida em **`fabrica/`**, que é
**gitignored** de propósito (um pacote versionado envelhece em silêncio enquanto
a placa muda). Mas um pacote que só existe em `fabrica/` **some com o container e
nunca chega ao repo/PR** — era preciso um humano decidir como o tirar de lá. Por
isso, na passagem, o portão copia **automaticamente** o pacote enxuto — o **zip
de gerbers + `cpl_*.csv` + `bom_*.csv` + `LEIA-ME.txt`** — para
**`export/<placa>_<data>_<commit>/`**, a pasta de topo **rastreada**, e imprime
esse caminho durável como última linha. O `.gitignore` mantém `fabrica/` fora e
só abre exceção aos `.zip` sob `export/` (`!export/**/*.zip`, contra o `*.zip`
global). **A entrega dos gerbers só está feita quando esse `export/…` está
commitado** — é parte do portão do estágio 8, não um afterthought; qualquer um
descarrega o pacote carimbado direto do PR.

---

## Capacidades de fabrico JLCPCB (PCB standard)

| Parâmetro | 1–2 camadas | 4+ camadas |
|---|---|---|
| Largura/espaço mín. de pista | 5 mil (0.127 mm) | 3.5 mil (0.0889 mm) interno |
| ⌀ via (pad) mín. | 0.45 mm | 0.45 mm (0.25 avançado) |
| Furo de via mín. | 0.2 mm (0.3 barato) | 0.15 avançado |
| Anel mín. | 0.13 mm | desenha a ≥0.15 (Classe 2), 0.2 (Classe 3) |
| Furo-a-furo mín. | 0.5 mm | 0.5 mm |
| Silk: stroke / altura mín. | 0.15 mm / 1 mm | 0.15 mm / 1 mm |
| Borda↔trilho | 0.3 mm | 0.3 mm |
| Espessura da placa | 0.4–2.0 mm (1.6 default) | até 2.0+ |
| Peso do cobre | 1 oz default | 1 oz externo / 0.5 interno |

⚠️ **Não desenhes para os limites avançados** (3 mil, furo 0,15 mm) numa encomenda
barata standard.
