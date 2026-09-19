# Automação de fabrico KiCad → JLCPCB

Referência dos **estágios 4 e 8** da pipeline `/aipcb`. Caminho headless e
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
KCLI=$(.claude/skills/aipcb/scripts/bootstrap_kicad.sh 10)   # 10 = MAIOR requerido
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
⚠️⚠️ **O POS NATIVO DO KiCad NÃO É UPLOADÁVEL NA JLCONE** (medido no upload real,
2026-09-11): a JLCONE recusou o CPL do painel com **"Failed processing the CPL
file"**. Causa: o `kicad-cli` emite o cabeçalho nativo `Ref,Val,Package,PosX,PosY,
Rot,Side` com o lado em minúsculas (`top`/`bottom`), e o parser de SMT da JLC não
o reconhece. A JLC exige **exactamente** `Designator,Mid X,Mid Y,Layer,Rotation`,
com `Layer` **capitalizado** (`Top`/`Bottom`). O `para_fabrica` **v0.16.0** faz o
remapeamento na exportação (`cpl_para_jlc`: Ref→Designator, PosX→Mid X, PosY→Mid Y,
Side→Layer com Top/Bottom, Rot→Rotation; Val/Package descartados; mm sem sufixo,
zeros finais aparados; UTF-8 sem BOM) e o CPL committado é **directamente
carregável**. É a mesma lição da paridade de DFM: a **SAÍDA do portão tem de ser o
que a fábrica ACEITA**, não o que a ferramenta emite por omissão.
Rotações frequentemente precisam de correção por footprint (dor conhecida — ver
`parts-sourcing.md` §5).

⚠️ **O CPL da JLC descarta a geometria rica — por isso guarda-se um companheiro**
(lição de 2026-09-11, `para_fabrica` **v0.18.0**). O remapeamento `cpl_para_jlc`
deita fora `Val` e `Package` e capitaliza o lado — é o que a JLC ACEITA, mas é
também a **única cópia** de `Ref,Val,Package,PosX,PosY,Rot,Side` da placa, e as
ferramentas a jusante (a **VISTORIA** do `portao_humano.py`, o **kit** de
`kit_montagem.py`) precisam dela. Por isso, além do `cpl_<placa>.csv` (formato
JLC), o pipeline grava o **`cpl_rico_<placa>.csv`** (o cabeçalho nativo completo)
na mesma pasta. Regra: **o CPL da JLC nunca é a única cópia da geometria rica** —
o `cpl_rico_*` é o que a VISTORIA e o kit lêem, o `cpl_*` é o que se carrega.

### BOM (nativo, 7+/9)
```bash
kicad-cli sch export bom --output bom.csv \
  --fields 'Value,Reference,Footprint,LCSC' \
  --labels 'Comment,Designator,Footprint,LCSC Part #' \
  --group-by 'Value,Footprint,LCSC' --exclude-dnp board.kicad_sch
```
⚠️ A coluna do código LCSC tem de se chamar **`LCSC Part #`** — é essa a chave em
que a JLC lê o código no upload de BOM; `LCSC` sozinho não é reconhecido. O
`para_fabrica` v0.16.0 grava a coluna com esse cabeçalho (`mete_lcsc`).

⚠️ **A BOM de montagem agrupa por número LCSC, não por comentário** (lição de
2026-09-11, `para_fabrica` **v0.17.0**). A JLCONE casa cada linha da BOM a **uma
peça física**; duas linhas com o mesmo C-number (porque o `Value` anotado diferia
— `10k — L` vs `10k pull-down`) fazem-na avisar **"Multiple lines matched to the
same part"** e obrigam a fundir à mão. A saída do portão evita isso: a BOM de
montagem tem **UMA linha por número LCSC** — designadores unidos numa só célula,
`Qty` somada. Regra: **agrupar por `LCSC Part #`, nunca por `Comment`/`Value`
anotado** (o mesmo C-number com dois comentários é a mesma peça; comentários
iguais com C-numbers diferentes são peças diferentes). O `--group-by` nativo por
`Value,Footprint,LCSC` do `kicad-cli` não chega — a re-agregação por LCSC é do
`para_fabrica`.

---

## 2. `.kicad_dru` — regras JLC como portão de DRC

Põe `<projecto>.kicad_dru` ao lado da placa (o KiCad carrega-o; o `kicad-cli pcb
drc` honra-o). Regras orientadas à JLCPCB (2 camadas standard):

⚠️⚠️ **COMENTÁRIO `#` SÓ EM LINHA PRÓPRIA — NUNCA no fim de uma linha de `(rule`.**
Um `#` inline (ex.: `... )))   # 5 mil`) faz o parser de DRU do KiCad **descartar
em silêncio** essa regra e as seguintes: medido a 2026-09-10, um `# 5 mil` colado
a um rule levou o silk→pad de 4 para 2 achados **sem um único erro**. Comentário em
linha inteira (mesmo com acentos) é seguro; colado ao código, não.

```
(version 1)
# minimos absolutos JLC 2 camadas (5 mil pista/folga)
(rule "min_trace_width"   (constraint track_width    (min 0.127mm)))
(rule "min_clearance"     (constraint clearance      (min 0.127mm)))
(rule "min_annular"       (constraint annular_width  (min 0.13mm)))
(rule "min_via_hole"      (constraint hole_size      (min 0.2mm)))
(rule "min_via_diameter"  (constraint via_diameter   (min 0.45mm)))
(rule "hole_to_hole"      (constraint hole_to_hole   (min 0.5mm)))
(rule "edge_clearance"    (constraint edge_clearance (min 0.3mm)))
# silk TEXTO -> pad e -> furo (folga 0,15 mm). A condicao e BIDIRECCIONAL: o KiCad
# nao garante qual item e A e qual e B, e a versao so-`A.Type=='Text'` DEIXA PASSAR
# metade dos casos (media-se — o Q1/Q2 do painel escaparam assim).
(rule "silk_text_to_pad"
  (condition "(A.Type == 'Text' && A.Layer == '?.Silkscreen' && B.Type == 'Pad') || (B.Type == 'Text' && B.Layer == '?.Silkscreen' && A.Type == 'Pad')")
  (constraint silk_clearance (min 0.15mm)))
# silk -> furo: sai como `hole_clearance` no relatorio
(rule "silk_text_to_hole"
  (condition "(A.Type == 'Text' && A.Layer == '?.Silkscreen' && B.Type == 'Pad') || (B.Type == 'Text' && B.Layer == '?.Silkscreen' && A.Type == 'Pad')")
  (constraint physical_hole_clearance (min 0.15mm)))
(rule "silk_text_thickness" (layer "?.Silkscreen") (condition "A.Type == 'Text'")
  (constraint text_thickness (min 0.15mm)))
(rule "silk_text_height"    (layer "?.Silkscreen") (condition "A.Type == 'Text'")
  (constraint text_height (min 1.0mm)))
```
4 camadas: relaxa o cobre interno a 3,5 mil (`(layer inner)` + `0.0889mm`). O
ruleset testado da comunidade (darkxst gist) cobre 2/4 camadas, silk-over-copper,
silk-off-board, edge, hole-to-hole, castellated — copia e apara. O ruleset
completo desta casa vive em `kicad/cotuca_painel/cotuca_painel.kicad_dru`.

### 2a. Paridade com o DFM da JLC — a serigrafia (lição de 2026-09-10)

⚠️ **O nosso portão tem de MEDIR O QUE A FÁBRICA MEDE.** O DFM da JLCONE reprovou
os gerbers do painel com serigrafia a VERMELHO que a nossa pipeline sub-contou:
**silk→pad 25 vermelhos + 35 laranja, silk→furo 31 vermelhos + 9 laranja, largura
de traço 50 laranja**. O nosso DRC via só ~2-3 `silk_over_copper`.

A causa, medida, foi tripla — e cada peça é uma regra que faltava:

1. **O DRC-default do KiCad mede a serigrafia sobre cobre com `min_silk_clearance
   = 0`**: só acusa o SOBREPOSTO, nunca a **folga** de 0,15 mm que a JLC exige.
   Um `Q1`/`Q2` a 0,04 mm de um pad é vermelho na JLC e invisível para nós. →
   regra `silk_text_to_pad` (`silk_clearance` 0,15 mm), condição **bidireccional**.
2. **Não havia regra de silk→FURO.** A JLC mede a serigrafia contra o furo, não só
   contra o cobre. → regra `silk_text_to_hole` (`physical_hole_clearance` 0,15 mm,
   sai como `hole_clearance`).
3. **Largura de traço da serigrafia < 0,15 mm.** Os contornos de biblioteca do
   KiCad nascem a 0,12 mm, e **NENHUMA regra de DRC do KiCad mede a largura de um
   GRÁFICO de serigrafia** (o `text_thickness` só vê texto). → a fonte:
   `kicad/normaliza_silk.py` sobe-os a 0,15 mm (Estágio 6, `--check` é portão).

E o portão passou a **recusar** por isto: `para_fabrica.py` (v0.13.0) gate no
`silk_over_copper` **filtrado a texto** (o contorno de footprint sobre um pad é o
caso (b) da `kicad_layout` §11g-ter — a JLC recorta-o, não se recusa) + no
`text_thickness`/`text_height`; o silk→furo já cai na GEOMETRIA (`hole_clearance`).

> **A lição: o DRC-default SUB-CONTA a serigrafia contra a JLC.** Contagem baixa
> de `silk_over_copper` não é placa limpa — é um verificador a medir com folga 0.
> O passo na JLCONE tem de ser um **carimbo**, não caça-surpresas.

**As laranja (avisos), por classe:** *anel 76* — abaixo do preferido, **acima** do
mín. 0,13 mm (as nossas regras não recusam; aceitável). *Largura de traço 50* —
resolvida na fonte (`normaliza_silk`). *Expansão negativa de máscara 2* e *slot
curto 2* — inerentes aos footprints THT/slot usados; imprimíveis, aceitáveis
enquanto o desenho não mudar. Só se age numa laranja se for barata **e** real.

### 2a-ter. As laranja MEDIDAS, uma a uma (lição de 2026-09-11)

⚠️ **A regra do dB: nada de descarte "cosmético" — cada laranja mede-se no
gerber/furo real e o veredicto traz o número.** Com a serigrafia já a 0 vermelho
na JLCONE, sobraram só laranja. Medidas no artefacto (não supostas), com o
`pcbnew` (features) + gerbonara/shapely (gerber), reproduzindo o mecanismo de cada
classe. Veredicto de cada uma: **manufacturável como está, ACEITE com o número** —
nenhuma tinha conserto real **e** barato que não piorasse outra coisa:

| classe JLCONE | medido no painel | mecanismo | veredicto |
|---|---|---|---|
| **anel ~75** | min **0,15 mm** nas 96 features furadas (0 < 0,13; 0 < 0,15); 33 vias 0,6/0,3 = a **via-padrão da JLC** | JLC conta top+base por feature no limite preferido 0,15 | **ACEITE**. ≥ 0,13 mín. Subir o pad testado: 17 vias caem de 0,20→~0,15 mm de folga a cobre alheio — troca **pior**. |
| **mask opening exposing trace 14** | **34/34 vias tapadas** (0 abertura sobre via); expansão de máscara dos pads = **0,0 mm**; webs de máscara todos ≥ 0,15 mm; cobre de pista exposto no nosso gerber = **0** (só 0,05 mm² de ruído de facetagem) | a JLC aplica a **expansão de casa dela** aos nossos pads de 0 expansão e expõe o pescoço pad/pista | **ACEITE**. O nosso gerber entrega 0 exposição real; as vias já estão tapadas — não há o que tapar. O pescoço fica no filete de solda. |
| **expansão negativa de máscara 2** + **slot curto 2** | ambas do **único** footprint do encoder EC11 (`SW1`), os 2 slots de fixação mecânica (furo 2,8×1,5 mm PTH) | inerente ao footprint mecânico do EC11 | **ACEITE**. Slots de montagem — a JLC fabrica montagens EC11 de rotina. |
| **silk to pad 1** | folga mín. silk→pad = **0,1924 mm** (junto de C3), o rótulo que o `fix_text` pousa no seu alvo ~0,19 | ≥ 0,15 mín (imprime bem), abaixo do preferido JLC ~0,2 | **ACEITE**. Empurrá-lo acima de 0,2 é mudar o REGIME de silk da placa toda (risco de regressão, §11g-quinquies) — não é um "nudge" trivial. |

⚠️ **O que se ganhou como PORTÃO, e o que não vale a pena gatilhar:**
- **Não** se gatilha anel a 0,15 (preferido): reprovaria a via-padrão da JLC — ruído.
  O `.kicad_dru` gatilha no mínimo absoluto 0,13, que é o piso real.
- **Sim** se gatilha a **tapagem das vias** (`kicad/vias_tented.py`, no
  `para_fabrica` v0.15.0): a classe "mask opening exposing trace" só é benigna
  *enquanto* as vias estiverem tapadas. Destapá-las é uma escolha que **nenhum
  DRC recusa** e que torna a laranja em vermelho real. A guarda lê a tapagem no
  texto (`(tenting ...)` do setup + override por via), sem pcbnew, e prova-se nos
  dois sentidos (`--auto-teste`: default destapado e override `none` são apanhados).

### 2a-bis. O 2º round — o portão do DRC continuou a mentir (lição de 2026-09-10)

⚠️ **As regras `.kicad_dru` acima NÃO foram suficientes.** Duas passagens deram o
painel por "limpo" e a JLCONE continuou a mostrar **silk→pad 18 vermelhos + 32
laranja, silk→furo 11 vermelhos + 4 laranja, largura 50 laranja**. Reproduzindo os
números **no gerber exportado** (a única verdade — é o que a JLC ingere), a causa
foi outra e mais funda:

1. **O `kicad-cli pcb drc` não dispara as regras `silk_clearance` do `.kicad_dru`.**
   Sobre uma placa cheia de serigrafia rente a pad, o DRC reportou **0**
   `silk_over_copper` e **0** da regra custom. O portão media com uma régua que
   não mede. *Contagem 0 de um verificador que não dispara é o pior "limpo".*
2. **O vermelho da JLC nasce do nosso próprio `--subtract-soldermask`.** Esse
   flag apara a serigrafia **rente** à abertura de máscara (0 de folga) — e a
   borda coincidente conta como silk-sobre-pad na JLC. Só se vê **no gerber**,
   nunca na placa nem no DRC.
3. **A "largura 50 laranja" era TEXTO E GRÁFICO a EXACTAMENTE 0,15 mm.** O bump de
   contornos a 0,15 (round 1) não a mexeu: 0,15 é o *mínimo*, e a JLC marca a
   laranja quem fica **no** mínimo. O alvo tem de ficar **acima** — usámos 0,18 mm.
   E o `text_thickness` do DRC também não apanha texto que já está a 0,15.

**A correcção, medida a cada passo contra o gerber:**
- **Fonte** (`kicad/serigrafia_dfm.py`): sobe todo traço (texto **e** gráfico) a
  0,18 mm; recorta cada gráfico de serigrafia — **incluindo discos CHEIOS** (o
  corpo do C3 electrolítico era um `fp_circle` *filled* por cima dos dois pads, e
  recortar só o contorno não tira o enchimento) — a 0,15 mm de folga de pad e
  furo; reposiciona o texto de marca (`COTUCATRONICS` caíra por cima da fila de
  test-points/vias) e afasta o designador que fica junto de um pad, medindo o
  **glifo no gerber** (a forma de texto do pcbnew **não bate** com a plotada).
- **Portão** (`kicad/mede_serigrafia.py`, chamado por `para_fabrica` v0.14.0 depois
  de exportar): mede o **gerber** — silk→pad (contra a máscara), silk→furo (contra
  o `.drl`, incl. slots e vias) e **largura de cada traço** (a abertura dos D01,
  texto e gráfico). Recusa se gerbonara/shapely faltarem — um portão que não
  consegue medir **não passa**.

> **A lição-mãe: não há embarque "cosmético" para serigrafia.** O rótulo
> "auto-clip / order-safe" para silk→pad já foi ERRADO em todas as vezes. Um
> "cosmético" é uma **medição** (princípio da certificação de risco): ou o gerber
> mostra 0 vermelhos, ou não está limpo. Reds de serigrafia vão a **zero, medidos
> no gerber** — a única excepção seria apagar um marcador de pino/polaridade/cátodo
> exigido, e aí PÁRA-SE e mostra-se esse item, não se passa em bloco.

> **E a lição do traço:** endurecer os *defaults* do `.kicad_pro` não toca no texto
> que já existe. O portão tinha de **medir a largura do traço no artefacto**, não
> confiar num default. A largura vive no gerber; o custo é lê-la.

*(2026-09-11 — o cérebro foi levado pelo mesmo passo v2, medido no gerber. Antes:
silk→pad 23 vermelhos (F) + 2 (B), traço mín 0,12 mm. Depois de `serigrafia_dfm`:
silk→pad/→furo 0 vermelho nos dois lados, traço todo ≥ 0,18 mm — e cada marcador
de polaridade/pino-1/cátodo (D1 SMB, D2 SMA, D11/D21 SOT-23, U1 SOIC-8, «+» de
C1/C3/C5/C6) renderizado e verificado a sobreviver e a ler-se. Uma correcção na
ferramenta: `serigrafia_dfm` v0.2.0 passou a tratar texto de footprint da classe
`PCB_TEXT` como FP_TEXT (o painel só tinha Reference/Value; o cérebro traz 17
rótulos de pino do módulo ESP32-C3 em U3 a 0,8 mm) — antes o `widen` rebentava no
`GetShape()` de um texto. As laranjas do cérebro, medidas no artefacto: anel mín
0,15 mm em 217 features furadas plateadas (0 < 0,13; 0 < 0,15; H1–H4 são NPTH, sem
anel — excluídas), 72 vias 0,6/0,3 = a via-padrão da JLC; 0 slots (o cérebro não
tem o EC11 do painel — a classe slot/negative-mask não se aplica: mediu-se, não se
supôs); expansão de máscara 0,0 mm e 72 vias TODAS tapadas (0 abertas); silk→pad
no limite = 0,163 mm (F) / 0,195 mm (B), ≥ 0,15 mín. Todas ACEITES com o número.
⚠️ NÃO se pôs `cotuca_cerebro.kicad_dru`: a regra `text_height ≥ 1,0 mm` do DRU do
painel dispararia nos 17 rótulos de 0,8 mm de U3 (que `para_fabrica` trata como
recusa dura) — são uma laranja legítima (a JLC imprime 0,8 mm; subi-los a 1,0 no
passo de 2,54 mm do soquete arriscava novos silk→pad vermelhos). O portão de silk
que MANDA é a medição do gerber, e essa passa. Ver relatório 002 §6a.)*

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

Junto do zip, o pacote enxuto leva ainda: `cpl_<placa>.csv` (formato JLC) +
**`cpl_rico_<placa>.csv`** (geometria rica para as ferramentas a jusante, §1 CPL) +
`bom_<placa>.csv` (agrupada por LCSC, §1 BOM) + `LEIA-ME.txt` + `VISTORIA.md` (o
questionário do portão humano) + a pasta **`kit/`** (o kit de montagem em casa,
abaixo).

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
de gerbers + `cpl_*.csv` + `cpl_rico_*.csv` + `bom_*.csv` + `LEIA-ME.txt` +
`VISTORIA.md` + `kit/`** — para
**`export/<placa>_<data>_<commit>/`**, a pasta de topo **rastreada**, e imprime
esse caminho durável como última linha. O `.gitignore` mantém `fabrica/` fora e
só abre exceção aos `.zip` sob `export/` (`!export/**/*.zip`, contra o `*.zip`
global). **A entrega dos gerbers só está feita quando esse `export/…` está
commitado** — é parte do portão do estágio 8, não um afterthought; qualquer um
descarrega o pacote carimbado direto do PR.

### 5a. O kit de montagem em casa — `kicad/kit_montagem.py` (v1.0.0)

Saída-padrão do Estágio 8 **ao lado** do pacote de fábrica: dá-lhe a pasta de um
pacote e emite a subpasta **`kit/`**, o que precisas para montar a placa **em casa,
na chapa quente** (ver `montagem-casa.md` para o fluxo e o portão de custo). Lê o
`cpl_rico_*.csv` (geometria) + o `bom_*.csv` (agrupado por LCSC) e escreve:

- **`MAPA_COLOCACAO.md`** — o mapa de colocação, **agrupado por valor** (todos os de
  um valor de uma vez), com **TOP e BOTTOM separados**, e as peças de **bottom** e
  as **THT** sinalizadas (o bottom solda-se à mão porque cairia na chapa; a THT vai
  por último). Dá posição e rotação — **não** decide o sentido do pino-1/cátodo,
  que é a `VISTORIA.md` + a render 3D que decidem.
- **`COMPRA_LCSC.csv`** — o carrinho da LCSC, uma linha por peça física, com `Qty` e
  **`Qty+spares`** (peças de chapa perdem-se/queimam-se — leva sobresselentes).
- **`LEIA-ME-KIT.txt`** — a ordem de montagem em prosa.

O kit torna o **risco de rotação da biblioteca da JLC moot** na via de casa: quem
coloca (tu) usa a render do nosso próprio board como mapa, então a rotação da
biblioteca da JLC nem entra (ver `montagem-casa.md §2`).

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
