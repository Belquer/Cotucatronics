# Fontes de componentes — a árvore de 3 níveis e o portão STOP

Referência do **estágio 2** da pipeline `pcb`. Toda linha do BOM tem de acabar
**ou** com um C-number da LCSC em stock que a JLC monta mesmo, **ou** com um
footprint próprio, justificado e verificado por datasheet. Este documento fixa a
árvore de decisão de 3 níveis e o portão STOP que o `scripts/manifesto_fontes.py`
aplica.

> ⚠️ **Preços e taxas mudam.** Os números aqui são "ordem de grandeza / como
> funciona", nunca um contrato — **re-confirmar a cotação viva na página da
> encomenda da JLCPCB** antes de confiar em qualquer valor em cache.

---

## 1. O modelo de peças da JLCPCB

### Os tiers

O tier é propriedade da peça LCSC **como está em stock para montagem**, não da
entrada no catálogo da LCSC — uma peça pode vender-se em bobine e **não** estar
na biblioteca de montagem.

| Tier | O que é | Taxa de feeder |
|---|---|---|
| **Basic** | ~600–700 jellybeans sempre carregados na máquina (R/C 0402/0603/0805, transístores, LEDs, reguladores comuns) | **Nenhuma** |
| **Preferred Extended** | Extended de alta rotação, marcado "Preferred" | **Isento** em Economic PCBA |
| **Extended** (não-preferred) | Todo o resto na biblioteca; precisa de feeder montado para o teu job | **por tipo de peça único** |

Ranking custo-óptimo: **Basic > Preferred Extended > Extended > (custom / fora da
biblioteca)**. A taxa de feeder do Extended é **uma vez por C-number distinto**,
não por colocação (100× o mesmo condensador = uma taxa).

### O que "na biblioteca de montagem da JLC" significa

= o C-number aparece no catálogo de **montagem SMT da JLCPCB** com stock de
montagem > 0 e tier atribuído. É um **pool de stock diferente** do retalho da
LCSC — empresas irmãs, logística separada, mesma referência C-number.

Como conferir uma peça (`C#####`):
- **UI oficial:** `https://jlcpcb.com/parts` — mostra tier, stock de montagem,
  escada de preço. (Fraca para busca paramétrica — ver §2.)
- **Na encomenda:** o fluxo de montagem casa cada C-number e sinaliza "fora de
  stock" / "não na biblioteca" por linha. Autoritativo, mas não scriptável antes.
- **Programático:** §2.

Não há **API REST pública oficial** de stock/montagem. Todo acesso programático
é via datasets raspados ou wrappers da comunidade.

---

## 2. Busca programática (achar peça na biblioteca / equivalente)

### (a) `jlcparts` (yaqwsx) — fonte offline recomendada

- Repo: `https://github.com/yaqwsx/jlcparts` · UI: `https://yaqwsx.github.io/jlcparts/`
- Uma GitHub Action raspa a biblioteca de montagem para `cache.sqlite3`; depois
  `jlcparts buildtables ... cache.sqlite3 web/public/data` emite JSON por
  categoria. Puxa a DB construída e **consulta local por SQL** — sem limites de
  rate, determinista, offline.
- Cada registo traz: C-number, fabricante+MPN, categoria/subcategoria,
  pacote/footprint, flag Basic/Extended(+Preferred), stock, escada de preço,
  URL do datasheet, e um blob de atributos paramétricos. É isto que permite a
  busca paramétrica de "acha um equivalente".
- ⚠️ O repo canónico é **yaqwsx/jlcparts** (nomes tipo "yaganidev/jlcparts" são
  mirrors/forks).

### (b) `jlcsearch` (tscircuit)

Motor de busca de peças JLC em stock; **qualquer página devolve JSON acrescentando
`.json`** ao URL → queries paramétricas triviais com LCSC#, fabricante, pacote,
descrição, stock, preço.

### (c) Apify "JLCPCB Parts Finder"

`https://apify.com/amicable_grid/jlcpcb-parts-finder/api` — C-number → JSON com
stock vivo, escada de preço, specs, pacote, MOQ, datasheet. API paga; bom para
confirmar uma peça a pedido.

### (d) Endpoints diretos da LCSC (frágeis — precisam CSRF + cookies)

`GET https://lcsc.com/api/global/additional/search?q=<C-number>` e
`POST https://lcsc.com/api/products/search`. Throttle ~100 req/min, sem API
oficial. Usar com parcimónia.

### Checklist de equivalente SEGURO (fallback do nível 1)

Quando o MPN exacto não é peça montável, busca no dataset uma peça na biblioteca
(Basic → Preferred Extended → Extended) que bata em **todos** estes antes de substituir:

1. **Footprint/pacote** idêntico (0402, SOT-23-3, SOIC-8). Pacote diferente =
   footprint diferente = não é drop-in.
2. **Valor primário** — R/C/L, ou a família funcional para ICs.
3. **Tensão** ≥ original. **4. Corrente/potência** ≥ original.
5. **Tolerância** ≤ original (mais apertada ou igual, nunca mais folgada).
6. **Dieléctrico/temp** — em caps, **X7R/X5R/C0G têm de bater de classe** (um Y5V
   "equivalente" de um X7R não é seguro).
7. **Faixa de temperatura** ≥ original.
8. Para ICs/reguladores: **pinout idêntico** + spec eléctrica (dropout, Iq,
   família lógica/limiar, speed grade).

Só depois de todas as linhas aplicáveis baterem é que o substituto é "seguro".
Regista o MPN original, o LCSC# escolhido, o tier e os parâmetros que bateram —
é o rasto de auditoria.

---

## 3. Importar / converter uma peça (fallback do nível 2)

Quando não há equivalente aceitável na biblioteca mas a peça **existe** na
LCSC/EasyEDA, puxa símbolo+footprint+3D em vez de desenhar à mão.

### `easyeda2kicad.py` (uPesy) — conversor primário

```bash
pip install easyeda2kicad
easyeda2kicad --full --lcsc_id=C2040
easyeda2kicad --full --lcsc_id=C2040 --output <proj>/lib/jlc --overwrite
```
- KiCad v6–v9; emite `.kicad_sym` + `.pretty/` + `.3dshapes/`.
- ⚠️ Aponta `--output` para uma lib **do projecto** e adiciona-a às
  `sym-lib-table`/`fp-lib-table` **do projecto** — a peça viaja com o repo.
- ⚠️ README: *"a correcção dos símbolos/footprints não é garantida — confere
  sempre."* → corre a verificação §4 antes de confiar num footprint convertido.

### `JLC2KiCadLib` (TousstNicolas) — alternativa

```bash
pip install JLC2KiCadLib
JLC2KiCadLib C1337258 -dir My_lib -symbol_lib My_symbol_lib \
  -footprint_lib My_footprint_lib -models STEP WRL
```
Escreve uma árvore de lib de projecto auto-contida, fácil de commitar.

### Outras fontes (quando o EasyEDA não tem bom modelo)

SnapEDA · Component Search Engine (SamacSys) · Ultra Librarian — export nativo
KiCad, muitas vezes **mais IPC-exacto** que o EasyEDA convertido; mas **sem
C-number**, por isso ainda deves a §2 para atar um C-number montável à linha.

---

## 4. Construir do datasheet (fallback do nível 3)

Só quando a peça é real e não tem símbolo/footprint utilizável em lado nenhum.

- **Footprint:** land patterns por **IPC-7351(B)**. Sufixo de densidade no nome:
  **M** (Most, hand-solder), **N** (Nominal — o default são), **L** (Least).
  Usa **Nominal (N)** salvo razão de densidade. Courtyard em `F.CrtYd`/`B.CrtYd`
  (o DRC do KiCad acusa sobreposição; excesso nominal ≈ 0,25 mm). **Nunca apagar
  o courtyard.**
- Ferramentas: Footprint Wizard do KiCad (famílias paramétricas) ·
  `pointhi/kicad-footprint-generator` · calculadoras IPC-7351.
- **Símbolo:** números/nomes/tipos de pino direto da tabela de pinout; pinos de
  potência com tipo correcto; pino do símbolo mapeia 1:1 ao pad do footprint.

### Verificar um footprint à-mão ANTES de confiar

1. **Auditoria de cotas vs datasheet:** passo, tamanho de pad, courtyard, span —
   passo trocado ou X/Y invertido é o assassino nº 1.
2. **Contagem/numeração de pads vs pinout;** marca de pino-1 no silk/fab bate com
   o canto do datasheet.
3. **Checker do Footprint Editor + DRC** depois de colocado.
4. **3D + teste em papel:** imprime 1:1 e põe a peça real por cima — apanha erro
   métrico/imperial e de passo na hora.
5. **Polaridade/orientação:** para peças polarizadas a marca de pino-1/cátodo/mais
   no **fab** tem de ser inequívoca — ela conduz a rotação do CPL (§5).
6. **Paste/mask:** aberturas de paste presentes; thermal pad segmentado
   (QFN/DFN).

Regista: revisão+página do datasheet do land pattern, as cotas usadas, e
quem/o quê verificou. É o artefacto que justifica um footprint próprio.

---

## 5. BOM / CPL e a armadilha da rotação

**BOM** (colunas mínimas): `Comment` (valor), `Designator`, `Footprint`,
`LCSC Part #`. Peças iguais numa linha só, designadores juntos por vírgula.

**CPL/centroide:** `Designator`, `Mid X`, `Mid Y`, `Layer` (Top/Bottom),
`Rotation` (graus, convenção JLC, em **mm**).

⚠️ **A rotação 0° do KiCad quase nunca bate com a da biblioteca da JLC** — para
muitos pacotes (SOT-23-x, SOIC/SOP, QFN, caps de tântalo/electro, LEDs, díodos
SOD-123) o desfasamento é fixo (90/180/270°); alguns precisam de offset XY. E o
CPL da JLC usa convenção **Y-flipada** vs KiCad. Enviar a rotação crua → peça
montada torta → **placa morta**. Ferramentas que corrigem: `Bouni/kicad-jlcpcb-
tools` (GUI), `matthewlai/JLCKicadTools` (CLI), KiBot. **Depois de gerar,
confirmar peça-a-peça no pré-visualizador da JLC** — é o portão ⟨dB⟩ do estágio 8,
não é scriptável.

---

## 6. A árvore de 3 níveis — decisão determinista (por linha do BOM)

Corre **por linha**. Não avança a placa para layout/fabrica enquanto **toda**
linha não passar o STOP gate.

```
Q0. Já há um C-number válido e em stock nesta linha (stock>0 E na biblioteca)?
    SIM → regista tier → PASSA.
    NÃO → NÍVEL 1.

NÍVEL 1: equivalente na biblioteca (mais barato, mais fiável).
    busca por categoria+pacote+valor → filtra stock>0 E na biblioteca →
    ranking Basic>Preferred Extended>Extended → confirma o checklist de equivalência §2.
    match seguro → atribui C-number + tier + params + MPN original → PASSA.
    sem equivalente seguro → NÍVEL 2.

NÍVEL 2: importar a peça exacta (existe na LCSC/EasyEDA).
    easyeda2kicad --full --lcsc_id=C##### --output <proj>/lib/jlc   (ou JLC2KiCadLib)
    → lib do PROJECTO → verificação §4.
    C-number na biblioteca de montagem → PASSA.
    comprável mas NÃO na biblioteca → marca "cliente-fornece/consignado" → PASSA*
        (*só se a encomenda permitir peça auto-fornecida).
    sem símbolo/footprint em lado nenhum → NÍVEL 3.

NÍVEL 3: construir do datasheet.
    símbolo do pinout · footprint IPC-7351 Nominal(N) · courtyard em F.CrtYd
    → VERIFICAÇÃO §4 (obrigatória) → atar fonte (C-number ou cliente-fornece)
    → PASSA.

═══ STOP GATE ═══  avança só quando, para CADA linha, uma é verdade:
  (A) C-number válido, em stock, na biblioteca de montagem, tier registado; OU
  (B) footprint importado/à-mão §4-VERIFICADO com C-number confirmado na
      biblioteca; OU
  (C) linha explicitamente cliente-fornece com footprint verificado (só se a
      encomenda permitir).
  Qualquer linha sem (A/B/C) = BLOQUEADA. Não se gera o pacote de fabrica.
```

### O manifesto de fontes (o artefacto do portão)

CSV/JSON commitado ao lado do projecto, uma linha por linha do BOM:

```
designator, intended_MPN, resolved_LCSC, tier(Basic/Preferred Extended/Extended/NA),
assembly_stock, source(existing|equivalent|imported|built|customer_supplied),
footprint_source(kicad_std|easyeda2kicad|JLC2KiCadLib|snapeda|hand),
verified(yes/no + método), datasheet_ref, notes
```

O abastecimento está "feito" quando **toda** linha tem `resolved_LCSC` não-vazio
**e** (`assembly_stock` > 0) **ou** flag cliente-fornece justificada, **e** todo
footprint importado/construído tem `verified=yes`. É este o ficheiro que o
`scripts/manifesto_fontes.py` lê e recusa (sai ≠0 e lista as bloqueadas). Provado
com `--sabota`.

```bash
python3 .claude/skills/pcb/scripts/manifesto_fontes.py manifesto_fontes.csv
python3 .claude/skills/pcb/scripts/manifesto_fontes.py --sabota   # prova que sabe dizer não
```

---

## Ferramentas — referência rápida

| Etapa | Ferramenta | Comando / URL |
|---|---|---|
| Busca paramétrica (offline) | jlcparts (yaqwsx) | `github.com/yaqwsx/jlcparts` |
| Busca (JSON) | jlcsearch (tscircuit) | acrescenta `.json` ao URL da listagem |
| Busca (hosted) | Apify JLCPCB Parts Finder | `apify.com/amicable_grid/jlcpcb-parts-finder` |
| UI oficial | JLCPCB Parts Library | `jlcpcb.com/parts` |
| Converter LCSC→KiCad | easyeda2kicad.py | `pip install easyeda2kicad` |
| Converter LCSC→KiCad | JLC2KiCadLib | `pip install JLC2KiCadLib` |
| Libs de fornecedor | SnapEDA · CSE · Ultra Librarian | web + plugin KiCad (sem C-number) |
| Construir footprint | Footprint Wizard · pointhi/kicad-footprint-generator | IPC-7351 Nominal(N) |
| BOM+CPL + rotação (GUI) | Bouni/kicad-jlcpcb-tools | PCM; corrige rotação+offset |
| BOM+CPL + rotação (CLI) | matthewlai/JLCKicadTools | DB de correcção de rotação |

Ferramentas do próprio repo para este estágio: `tools/codigos_lcsc.py`,
`tools/fichas.py`, `tools/bom_completa.py`, `kicad/mpn_nas_folhas.py`,
`kicad/dnp_para_pcb.py`, e o `scripts/manifesto_fontes.py` desta skill.
