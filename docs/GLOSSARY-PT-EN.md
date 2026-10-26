# Glossary PT→EN — Cotucatronics translation

**v0.1.0 · 2026-10-26 · shared glossary for the EN translation**

> **Shared glossary for the EN translation — follow exactly for uniform
> terminology.** Every translator MUST use the EN term in the right-hand
> column so the whole corpus reads with one voice. When two docs could pick
> different words for the same Portuguese term, this table is the tie-breaker.
>
> **Tier 3 (code / filenames / silk) is NOT translated in this pass.** See the
> DO-NOT-TRANSLATE section at the bottom. Leave those strings byte-for-byte as
> they are in the source.
>
> **Read the CONTEXT-DEPENDENT section before you touch `fonte`, `placa`,
> `plano`, `rede`, `chave`, `trilho`, `faixa`, `gravar`, `esquema`, `desenho`,
> `folga`, `ficha`, `energia` or `regime`** — these do not map one-to-one and
> a wrong pick changes the engineering meaning.

---

## Core PT→EN table (follow exactly)

Sorted by Portuguese term. EN is the mandatory rendering unless the
CONTEXT-DEPENDENT section overrides it.

| PT | EN |
|----|----|
| aceite | acceptance / accepted (PT-PT) |
| alimentação (fonte de ~) | power supply |
| âncora | anchor |
| aterramento | grounding |
| aviso | warning |
| bancada | bench (workbench) |
| biblioteca | library |
| bloco | block |
| botão | button |
| build sheet | build sheet |
| búzios | cowrie shells |
| cabe / caber | fits / to fit |
| cabo | cable |
| cadeia | chain (signal chain) |
| caixa | enclosure |
| calço | standoff / spacer |
| camada | layer (PCB layer) |
| canal / canais | channel / channels |
| carga | load |
| carimbo | stamp (version stamp) |
| cauda | tail (reverb tail) |
| cérebro | brain board (the MCU / main board) |
| cicatriz | scar |
| cobre | copper |
| coerência | consistency |
| comando | command |
| comentário | comment |
| congestão | congestion |
| conector | connector |
| conserto | fix (repair) |
| contador | counter |
| contorno | outline (board outline) |
| corpo | body |
| corrente | current (electrical) |
| corte | cut (laser cut of the enclosure) |
| critério | criterion |
| cunha | wedge |
| datum | datum |
| defesa | safeguard |
| defeito | defect |
| derivados | derivatives |
| desenho | drawing (see CONTEXT) |
| destino | destination / target |
| diagnóstico | diagnosis |
| encaixe | fit / fitment |
| encoder | encoder |
| entrada | input |
| esquema | schematic (see CONTEXT) |
| espelho | mirror |
| estágio | stage |
| estrela | star (star-ground topology) |
| etapa | step / stage |
| exciter | exciter |
| fábrica | factory |
| fader | fader |
| faixa | range (see CONTEXT) |
| falha | failure / fault |
| fase | phase |
| ficha | datasheet (see CONTEXT) |
| filtro | filter |
| fios | wires |
| fila | queue |
| fixação | mounting / fastening |
| folga | clearance (see CONTEXT) |
| folha | sheet |
| fonte | source (see CONTEXT) |
| footprint | footprint |
| furação | drilling |
| furo / furos | hole / holes |
| ganho | gain |
| gerador | generator |
| gerar | generate |
| geometria | geometry |
| gesto | gesture |
| gravação | flashing (firmware) (see CONTEXT) |
| gravar | flash (firmware) (see CONTEXT) |
| guarda | guard |
| hipótese | hypothesis |
| isenção | exemption / exception |
| janela | window |
| laço / laçada | loop |
| lápide | tombstone |
| layout | layout |
| leitura | read / reading |
| level | level |
| lição | lesson |
| ligações | connections |
| ligado | on / connected |
| lista | list |
| mapa | map |
| máscara | mask |
| mecanismo | mechanism |
| medição | measurement |
| medido | measured |
| medir | measure |
| mensagem | message |
| microfonia | microphonics (acoustic feedback) |
| módulo | module |
| montagem | assembly |
| motor | motor |
| mudo | mute / dead (no response) |
| netlist | netlist |
| nome | name |
| número | number |
| olho | eye (visual inspection) |
| ordem | order |
| pacote | package (fab package) |
| pads | pads |
| painel | panel board |
| parafusado | screwed |
| parafuso | screw |
| parâmetro / param | parameter |
| parte | part (portion) |
| pauta | agenda (see CONTEXT) |
| peça / peças | part / parts (component) |
| pico | peak |
| pino / pinos | pin / pins |
| pistas | traces (PCB traces) |
| placa | board (PCB) (see CONTEXT) |
| plano | plane (see CONTEXT) |
| ponte | bridge |
| ponto | point |
| portão | gate |
| preset | preset |
| probe / sonda | probe |
| proteção | protection |
| rede | net (see CONTEXT) |
| referência | reference |
| regime | regime / operating mode (see CONTEXT) |
| relatório | report |
| retrabalho | rework |
| retorno | return |
| reverb | reverb |
| revisão | review (the abbreviation "rev" stays "rev") |
| roteador | router (the routing tool) |
| roteamento | routing |
| roteiro | roadmap / script |
| rotear | route (verb) |
| ruído | noise |
| saída | output |
| serigrafia | silkscreen |
| sessão | session |
| sinal | signal |
| símbolo | symbol |
| sintoma | symptom |
| slot | slot |
| soma | sum |
| sonda | probe |
| step (ficheiro STEP) | STEP (file) |
| tabela | table |
| tampa | lid |
| teto | ceiling (upper limit) |
| terra | ground |
| teste | test |
| tone | tone |
| trava / travar | latch / lock |
| trilho | rail (see CONTEXT) |
| troca | swap |
| unidade | unit |
| varredura | sweep / scan |
| verdade (fonte da ~) | truth (source of truth) |
| verificador | checker |
| vista | view |
| VISTORIA | inspection |
| widget | widget |
| zero | zero |

---

## CONTEXT-DEPENDENT — read the sentence before you pick (FLAG LOUDLY)

These terms do NOT map one-to-one. Picking the wrong one changes the
engineering meaning. Decide from context every single time.

- **fonte** —
  - `fonte da verdade` → **source of truth**
  - `fonte de datum` → **datum source**
  - `fonte de alimentação` → **power supply**
  - `fonte` (typography / silk text) → **font**
- **placa** — almost always **board (PCB)**; rarely **nameplate**. Read the sentence.
- **plano** — copper context → **plane** (e.g. "planos e cobre" = planes and
  copper); planning context → **plan**.
- **rede** — PCB context → **net** (e.g. "medir por rede" = measure per net);
  networking context → **network**.
- **chave** — electrical/mechanical → **switch**; abstract → **key**.
- **trilho** — power context → **rail** (power rail); PCB geometry → **track**.
- **faixa** — numeric/value context → **range** (e.g. "faixa negativa" =
  negative range, "deleção por faixa" = range deletion); frequency/audio
  context → **band**.
- **gravar / gravação** — firmware context → **flash / flashing** (writing the
  MCU); audio/data context → **record / write**.
- **esquema** — circuit context → **schematic**; abstract plan → **scheme**.
- **desenho** — a figure/drawing → **drawing**; "documento de desenho" →
  **design document** (design-before-code sense).
- **folga** — mechanical context → **clearance** (e.g. "folga da tampa" = lid
  clearance); scheduling/margin → **slack**.
- **ficha** — "ficha técnica" → **datasheet**; a plug/part → **plug / connector**.
- **energia** — power-supply context → **power** (e.g. `gen_energia` = power
  schematic); physics → **energy**.
- **regime** — measurement context → **operating mode / regime** (steady vs.
  transient); otherwise **regime**.
- **pauta** — "PAUTA-ESCUTA" = **listening agenda**; musical context → **staff**.
- **escuta** — monitoring context → **monitoring / listening**.

---

## DO-NOT-TRANSLATE (Tier 3 — out of scope for this prose pass)

Leave these byte-for-byte as in the source. They are code, identifiers, or
machine-read strings, NOT prose:

- **Code identifiers** — function/variable/class names, enum values,
  `seed::Dnn`, `COTUCA_OS_VERSION`, net names (`AGND`, `DGND`, etc.), `#define`s.
- **Filenames and paths** — `boxes.py`, `check_sch.py`, `main.cpp`, `delay.h`,
  `export/MANIFEST.md`, any `docs/...` path.
- **Slash-command / skill names** — `/aipcb`, `/3ddesign`, `/caixa`, `/pcb`,
  `/kicad`, `edicao-segura`, `revisao-terra`, `design-review`, `ifa10000`, etc.
- **KiCad silk strings** — any text that is printed on the silkscreen; keep the
  literal string.
- **Tool / library / format names** — `build123d`, `kicad-cli`, KiCad, Daisy,
  Loopy Pro, `.lpproj`, `.lpcontrolprofile`, QSPI, SRAM, STEP, Gerber, CPL, BOM.
- **Part / vendor codes** — LCSC codes, MPNs, rev letters (rev A/B/C/D).
- **Firmware command tokens** — literal command words the firmware parses
  (`varre`, `enc`, `encpino`, `vistos`, `reset`, `board`, etc.) stay as typed.
- **Proper product name** — **Cotucatronics** (and `IFÁ10000` / `IFÁ10000-M`
  certification names) stay as-is.

---

*Generated by the git-editor setup worker for the PT→EN translation.
Expand this table as new recurring terms surface; keep it sorted and bump the
version line (`vX.Y.Z · YYYY-MM-DD`) on every change.*
