---
name: 3ddesign
description: Integração mecânica 3D placa ↔ caixa do Cotucatronics — o home executável da certificação IFÁ10000-M e do pipeline ppc/003. Use em "cabe na caixa?", "folga da tampa", "os furos alinham?", "IFÁ10000-M", "verificação 3D", "STEP", ou qualquer pergunta de encaixe placa-vs-caixa (volume, altura por zona, cabo, montagem). Chamada pela /aipcb no Portão de Certificação Mecânica (estágio 3, antes do cobre; e outra vez antes do 8, com a caixa real). Mede geometria REAL do STEP — não opinião. Não desenha esquema nem roteia cobre (isso é /aipcb + kicad_layout); não corta a caixa (isso é /caixa).
---

# /3ddesign — o encaixe volumétrico placa ↔ caixa

> **A folga vem do STEP, não do desenho. A montagem vem do olho, não do número.
> O que se afirma sem medir chama-se envelope-declarado — e assina-se, não se
> disfarça de física.**

Esta skill é o **home executável** da trilha mecânica do IFÁ10000 (a
**IFÁ10000-M**) e do **ppc/003**. A `/aipcb` **orquestra-a** — chama-a no Portão
de Certificação Mecânica como chama `kicad_layout`, `caixa` e `revisao-terra`.
Não reimplementa o que elas fazem: mede o **STEP real** da placa, cruza-o com as
cotas da caixa, e diz **sim/não** com número.

Nasceu do painel rev C: cinco placas fabricadas **sem um único ponto de
fixação** porque nenhum portão perguntava «como é que isto se prende, e cabe?».
A trilha M respondeu à fixação; esta skill responde ao **volume**.

---

## 1 · Responde / NÃO responde

| ferramenta | responde | NÃO responde |
|---|---|---|
| `kicad-cli` STEP export | a **geometria real** da placa em 3D (corpos, alturas, contorno) | folga contra a caixa · cabo · montagem |
| `build123d` / OCP | bbox, altura por sólido, **interferência** de sólidos (`&`/`intersect`), folga mínima (`distances`) | se o modelo 3D **existe** e está **certo** (peça sem STEP some em silêncio) |
| `kiutils` | zonas nomeadas do `.kicad_pcb` (contorno, footprints, courtyards p/ keep-out) | qualquer geometria **3D** — é leitor 2D do ficheiro |
| olho humano ⟨dB⟩ | o envelope 4-6, o encaixe a seco — **a peça em mão manda** | nada que exija medir do STEP |

**Regra (da /aipcb §5):** um portão que "passa" só se aceita depois de o ter
visto **recusar** sob sabotagem. Verde na primeira corrida é suspeito.

---

## 0 · Pré-voo — o portão que falha fechado

Antes de acreditar em qualquer número, prova-se a cadeia. Qualquer passo que não
prove o que afirma **PARA a pipeline** (exit ≠ 0).

1. **Resolver o `kicad-cli` — e REJEITAR os impostores.**
   - ❌ **stubs**: `kbcache/.../bin/kicad-cli` e `fakebin/kicad-cli` — shell < 100 B
     que **só ecoa a versão**. Ecoar `10.0.6` não é ser o KiCad 10.
   - ❌ **`/usr/bin/kicad-cli` 7.0.11** — falha a abrir o board novo com
     *Unknown token 'generator_version'*.
   - ✅ **único caminho válido:** `APPDIR=<appdir> <appdir>/AppRun.sh kicad-cli`
     a reportar **10.0.6**. **Checagem dura:** o alvo resolvido (`<appdir>/bin/
     kicad-cli`) é um **ELF multi-MB**, não um script. `file` diz `ELF`, não
     `POSIX shell script`.
2. **O STEP real sai** com:
   `AppRun.sh kicad-cli pcb export step --subst-models` (STEP **AP214** por
   defeito — rótulo + cor por sólido), **com `KICAD10_3DMODEL_DIR` setado**
   (paths não-resolvidos **somem em silêncio**). O board outline tem de ser
   **polígono fechado**; **exit 0 e ficheiro > 1 MB** (ref cérebro: **3,0 MB**;
   medido 3,19 MB). ⚠️ **Este KiCad 10 NÃO tem `--format`** — passá-lo faz o
   `kicad-cli` cuspir o help e sair (parece exit-0 sem escrever ficheiro).
   Warnings de `Fuse_2920` e `Electrosmith_Daisy_Seed` são **esperados** a menos
   que a lib esteja apontada — declaram-se como **envelope conhecido**, não erro.
3. **`import build123d`** (0.12.0) tem de correr. **FreeCAD e os 4 MCPs de CAD
   estão AUSENTES aqui** → futuro/opcional, **nunca baseline**. Nenhum MCP de CAD
   é conectável neste ambiente: a skill usa **bibliotecas + CLI, sem MCP**.

---

## 2 · A stack adotada (com licença)

- **`kicad-cli` STEP export** (via AppRun) — a geometria real da placa.
- **`build123d` 0.12.0** (Apache-2.0) — o **motor de medição**:
  `import_step` → `Compound`; `.solids()` / `get_type(Solid)` preserva `.label` /
  `.color` do AP214; `bounding_box()` → `BoundBox`; **`distances(*others)`** =
  folga mínima; **`intersect()` / `&`** não-vazio = **interferência**.
- **`kiutils`** (MIT, offline) — lê contorno + footprints + courtyards do
  `.kicad_pcb` para **nomear** as zonas de keep-out (não faz 3D).
- **`bd_warehouse`** (Apache-2.0) — standoffs / insertos / parafusos; casa com a
  diretriz «tudo parafusado» da `caixa`.
- **`boxes.py`** (GPLv3) — **RODAR como ferramenta externa, NÃO vendorizar**
  (GPL): painéis laser + finger joints + kerf embutido, saída SVG/DXF.

**Referência-só (não instalar):** KiCad StepUp (FreeCAD), enclozure (CadQuery).

---

## 3 · As 6 asserções

**Rígidas (físicas, computadas do STEP):**

1. **A placa cabe na cavidade** — bbox do STEP + calços, folga de borda ≥ a
   declarada em `params.py`.
2. **Furos alinham placa ↔ postes/brackets** (tudo parafusado) — dentro da
   tolerância declarada, ≥ 3 pontos.
3. **Altura por zona** — o componente mais alto de **cada zona** (Daisy Seed,
   IDC J14, eletrolíticos) < interior da tampa na **cunha de 11,8° POR ZONA** (o
   teto varia com x; a folga mede-se zona a zona, não com um número único).

**Envelope-declarado (afirmamos o número, NÃO é simulação; ⟨dB⟩ assina):**

4. **Recortes da tampa** — janela OLED, furos de pots 16 mm (Caso B), encoder.
5. **Cabos / serviço** — J1 ↔ J14, pots-para-tampa (Caso B), folga de serviço.
6. **Montabilidade** — parafusos acessíveis, placa removível **sem dessoldar**.

> **A fronteira 3-vs-4 é a lição inteira:** 1-3 saem do STEP e podem dizer
> **não** sozinhas; 4-6 são afirmações que a **peça em mão** (M9, ⟨dB⟩) confirma.
> Rotular 4-6 como física seria a mentira da rev B outra vez.

---

## 4 · Faseamento

- **Fase A** (o que roda hoje): params single-source (`mech/params.py`) + STEP
  livre + asserções **1-3** + cunha por zona.
- **Fase B**: asserções **4-6** (cabo / serviço), casada com a **caixa rev
  parafusada** — a geometria dos postes/recortes entra do gerador da caixa.

---

## 5 · Cicatrizes (uma linha cada)

- **Stub de `kicad-cli` que só ecoa a versão.** 48 B de shell não abrem um board.
- **`/usr/bin` 7.0.11 rejeita o board novo.** *Unknown token 'generator_version'*.
- **`KICAD10_3DMODEL_DIR` não-setado dropa peças em silêncio.** STEP sai "ok",
  faltam corpos, a altura mente para menos.
- **`--format step` neste KiCad 10 cospe o help e sai exit-0 sem ficheiro.** A
  família é a sub-ordem; a saída já é AP214.
- **FreeCAD não instalável aqui.** Nenhum MCP de CAD conectável. Baseline =
  bibliotecas + CLI.
- **bbox-declarado ≠ STEP-real é vermelho, não nota.** O `params.py` STUB
  (120×80) contra o cérebro real (180×100) diverge — o MANIFEST tem de o marcar.

---

## 6 · O executável

- **`mech/params.py`** (v0.1.0) — a **fonte única**: cotas da caixa, espessura
  3,0 mm (acrílico fundido), ângulo da tampa 11,8°, alturas máx por zona, folga
  de borda, e o bloco `ENVELOPE_DECLARADO` das asserções 4-6. Nada de cota
  escrita à mão noutro sítio: importa-se, não se copia.
- **`mech/verifica_3d.py`** (v0.1.0) — corre §0 (pré-voo, falha fechado), exporta
  o STEP via AppRun, mede com `build123d`, corre as asserções 1-3 (4-6 como
  envelope lido de `params.py`), e escreve **`mech/3d/RELATORIO_3D.md`**. Só lê:
  **nunca reescreve o `.kicad_pcb`** (a cicatriz do `pcb_contorno` que reescreveu
  o cérebro fabricado).

---

## 7 · MANIFEST

Cada **`mech/3d/*.step`** e o **`RELATORIO_3D.md`** entram no **`export/
MANIFEST.md`** com carimbo (`carimbo: <arquivo> · <data> · placa: …`) e a flag
**«bbox declarado vs STEP real»** — **divergiu = vermelho**. Um artefato 3D que
não se versiona não deixa detetar que envelheceu (edicao-segura §Versionar TUDO).
Os `*.step` (multi-MB) regeneram-se e ficam gitignored; o `RELATORIO_3D.md` é
rastreado.

---

## Skills irmãs

Chamada pela **`/aipcb`** (Portão de Certificação Mecânica). Delega a **`caixa`**
(geometria da caixa, os 5 portões), **`kicad_layout`** (medida por rede, cobre),
**`medicao-limpa`** (a etapa 0 do método), **`edicao-segura`** (bump semver +
reportar), e é a instância volumétrica do modelo **`ifa10000`**. A ficha M0–M10 e
a ordem de prioridades vivem em **`references/ifa10000-m.md`**; o registo de
certificados em `docs/IFA10000.md`; o plano do pipeline em `docs/ppc/003`.

---

## Histórico

- **v0.1.0 (2026-09-19)** — nasce da fusão /pcb→/aipcb: a integração 3D
  volumétrica caixa↔PCB é extraída para esta sub-skill, o home executável do
  IFÁ10000-M (a `references/ifa10000-m.md` migrou da /aipcb para cá) e do
  ppc/003. Encoda a pipeline sandbox-provada: pré-voo que rejeita stubs e o
  7.0.11, STEP AP214 via AppRun (3,19 MB, provado), medição com build123d 0.12.0
  (56 sólidos, bbox 180×100×22 mm do cérebro), asserções 1-3 rígidas + 4-6
  envelope-declarado. Traz `mech/params.py` e `mech/verifica_3d.py` (ambos
  v0.1.0). Fase A a rodar; Fase B (cabo/serviço) casada com a caixa rev
  parafusada.
