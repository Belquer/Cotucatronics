# ppc/003 — pipeline de verificação 3D placa ↔ caixa — **rev C**
<!-- doc-check: ignore import_step (função do build123d, biblioteca externa) -->
<!-- doc-check: ignore bd_warehouse (biblioteca externa Apache-2.0, standoffs/parafusos) -->
<!-- doc-check: ignore pcb_contorno (ferramenta em kicad/, fora do alcance do check_docs) -->

> **Estado: PLANO.** O home executável é a sub-skill **`/3ddesign`**; a
> disciplina responde/NÃO-responde é a **`/aipcb §5`**. Este documento é o plano
> do pipeline 3D — o que corre hoje (Fase A) e o que casa com a caixa rev
> parafusada (Fase B). Pareia com a **caixa rev parafusada** (por isso **rev C**).

- **Data:** 2026-09-19
- **Ramo:** `claude/slack-session-sm2ldz`
- **Placa de referência:** `kicad/cotuca_cerebro/cotuca_cerebro.kicad_pcb`
- **Home executável:** `.claude/skills/3ddesign/SKILL.md` (+ `references/ifa10000-m.md`)
- **Scripts:** `mech/params.py` (v0.1.0), `mech/verifica_3d.py` (v0.1.0)
- **Saída:** `mech/3d/RELATORIO_3D.md` (+ `mech/3d/*.step`, gitignored)
- **Contexto (2 linhas):** a rev B provou que folga/altura/montagem não têm
  portão; a trilha M (IFÁ10000-M) fechou a **fixação**, e este pipeline fecha o
  **volume**. Extraído da /aipcb para a /3ddesign na fusão /pcb→/aipcb.

## As 5 refinações — dadas por assentes

1. **Pré-voo antes de construir.** O portão §0 corre **antes** de qualquer
   medição e **falha fechado**.
2. **O portão é reproduzível dos NOSSOS scripts sem freecadcmd** —
   `kicad-cli` (via AppRun) + `build123d`. Os MCPs de CAD são **aceleradores**,
   e **nenhum está presente aqui**.
3. **As asserções 4-6 são declared-envelope**, rotuladas como tal — **não
   física**. A peça em mão (M9, ⟨dB⟩) confirma-as.
4. **Fase A primeiro, depois Fase B.**
5. **Rótulo de rev = C** (pareia com a caixa rev parafusada).

---

## §0 — Pré-voo (proven-here)

Provado nesta sessão (2026-09-19), baked na `/3ddesign §0`:

- Resolver o `kicad-cli` e **rejeitar** os impostores:
  - stubs `kbcache/.../bin/kicad-cli` e `fakebin/kicad-cli` — shell **48 B** que
    só ecoa a versão (medido);
  - `/usr/bin/kicad-cli` **7.0.11** — falha em *Unknown token 'generator_version'*;
  - ✅ único válido: `APPDIR=<appdir> <appdir>/AppRun.sh kicad-cli` → **10.0.6**,
    com o alvo resolvido a ser **ELF de 237 KB** (não script).
- **Prova por passo:** `file <appdir>/bin/kicad-cli` diz `ELF`; `AppRun.sh
  kicad-cli version` diz `10.0.6`.
- `import build123d` → **0.12.0** ok; `import kiutils` ok. **FreeCAD e os 4 MCPs
  de CAD ausentes** → futuro/opcional, nunca baseline.

## §1 — Params single-source

`mech/params.py` é a **fonte única**: cotas da caixa, **espessura 3,0 mm**
(acrílico fundido), **ângulo da tampa 11,8°**, alturas máx por zona, folga de
borda, e o bloco `ENVELOPE_DECLARADO` das 4-6. **Prova:** nenhuma cota escrita à
mão noutro ficheiro da trilha 3D; `verifica_3d.py` importa, não copia.

## §2 — Pipeline reproduzível (adopt-stack, freecadcmd-free)

1. STEP real: `AppRun.sh kicad-cli pcb export step --subst-models` (AP214),
   **`KICAD10_3DMODEL_DIR` setado** — paths não-resolvidos somem em silêncio.
   ⚠️ este KiCad 10 **não tem `--format`**. **Prova:** exit 0, ficheiro **3,19 MB**
   (ref cérebro ~3,0 MB); warnings `Fuse_2920`/`Electrosmith_Daisy_Seed` =
   envelope conhecido.
2. Medir: `build123d` `import_step` → `Compound`; `.solids()` preserva
   `.label`/`.color`; `bounding_box()`; **`distances`** = folga mínima;
   **`&`/`intersect`** = interferência. **Prova:** **56 sólidos**, bbox
   **180 × 100 × 22,02 mm** do cérebro (medido).
3. Nomear zonas de keep-out: `kiutils` (offline, lê o `.kicad_pcb`).
4. Fixação/parafusos: `bd_warehouse` (standoffs/insertos). Painel laser:
   `boxes.py` **como ferramenta externa, NÃO vendorizado** (GPLv3).

Licenças: build123d/bd_warehouse Apache-2.0, kiutils MIT, boxes.py GPLv3.
Referência-só: KiCad StepUp (FreeCAD), enclozure (CadQuery).

## §3 — As 6 asserções (rígidas vs declaradas)

- **Rígidas (do STEP):** (1) placa cabe na cavidade (bbox+calços, folga borda ≥
  declarada); (2) furos alinham placa↔postes/brackets (parafusado); (3) altura
  por zona — componente mais alto de cada zona (Daisy Seed, IDC J14,
  eletrolíticos) < interior da tampa na **cunha 11,8° POR ZONA**.
- **Envelope-declarado (afirmamos o número, não é simulação):** (4) recortes
  tampa (janela OLED, furos pots 16 mm Caso B, encoder); (5) cabos/serviço
  (J1↔J14, pots-para-tampa Caso B); (6) montabilidade (parafusos acessíveis,
  placa removível sem dessoldar). **⟨dB⟩ assina.**

## §4 — Fases

- **Fase A** (roda hoje): params single-source + STEP livre + asserções 1-3 +
  cunha por zona.
- **Fase B**: asserções 4-6 (cabo/serviço), casada com a caixa rev parafusada.

## §5 — Tabela do revisor (responde / NÃO responde) — ver /aipcb §5

| ferramenta | responde | NÃO responde |
|---|---|---|
| `kicad-cli` STEP | geometria real | folga · cabo · montagem |
| `build123d`/OCP | bbox, altura, interferência de sólidos | se o modelo 3D existe/está certo |
| `kiutils` | zonas nomeadas do `.kicad_pcb` | geometria 3D |
| olho humano ⟨dB⟩ | envelope 4-6 — peça em mão manda | o que exige medir |

## §6 — Integração MANIFEST

Cada `mech/3d/*.step` e o `RELATORIO_3D.md` entram no `export/MANIFEST.md` com
carimbo + flag **«bbox declarado vs STEP real»** (**divergiu = vermelho**). Nesta
corrida o `params.py` STUB (caixa 120×80) diverge do cérebro real (180×100) — é
o vermelho a funcionar, não um bug.

---

## O que NÃO mexer

- O `.kicad_pcb` do cérebro/painel: `verifica_3d.py` **só lê**. Nunca correr um
  gerador para ler o relatório (cicatriz do `pcb_contorno`).
- Os rótulos rev: **rev C** fica até um board ser efectivamente fabricado
  (congelamento-no-lançamento, /aipcb Estágio 8).

## Aceitação (medida)

- [x] Pré-voo rejeita stubs + 7.0.11, aceita AppRun 10.0.6 (ELF 237 KB).
- [x] STEP exporta > 1 MB (3,19 MB) com os 2 warnings de envelope conhecido.
- [x] `build123d` mede bbox (56 sólidos, 180×100×22 mm).
- [x] Asserções 1-3 correm do STEP; 4-6 lidas de `params.py` como envelope.
- [x] `RELATORIO_3D.md` emitido com carimbo + linha MANIFEST.
- [ ] **Fase B** (cabo/serviço) — pendente, casa com a caixa rev parafusada.
