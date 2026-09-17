# Protocolo de revisão tripla (estágio 7)

Referência do **estágio 7** da pipeline `pcb`. Modelado no `ifa10000`: **critério
de passagem explícito por passo, números não vistos, passo reprovado interrompe,
resultado negativo registado.** Três **lentes** diferentes para não repetirem o
mesmo olhar, cada uma idealmente por um **revisor independente** — quem desenhou
auto-completa o que espera, não o que está na folha — e o conjunto corre **3×**.

> A regra que governa tudo: **a última linha de defesa não pode ser a mesma
> ferramenta que fez o trabalho** (`kicad_layout` §12s). Um auto-teste tem
> exactamente os pontos cegos de quem o escreveu — o gate de fabrica apanhou um
> plano estilhaçado que o auto-teste do router não viu.

---

## Por que três lentes, e não uma lista

Uma lista única de checks lê-se de uma penada e cansa-se a meio — e os defeitos
não vêm de uma família só. Três lentes forçam três **posturas mentais**
diferentes sobre a mesma placa: *implementa o circuito?* · *a fábrica consegue
construir?* · *cabe no mundo e na mão?* Um defeito que escapa a uma lente cai
noutra porque a pergunta é outra.

---

## Lente A — Eléctrica / conectividade

**Pergunta:** o cobre implementa o circuito pretendido?

- **ERC limpo** (dono: `kicad`); netlist ↔ layout batem; PCB sincronizado por
  **referência, não UUID** (`sincroniza_pcb.py` — o F8 casa por UUID e duplica
  cada peça num esquema gerado por script).
- **Não-roteado por rede** (não o total — um total melhora enquanto uma rede
  regride). Invariante: **nenhuma rede pode piorar**, chave (rede, espécie) — um
  plano estilhaça enquanto o não-roteado da rede fica na mesma.
- **`funcao_dos_pads.py`** — pad → rede → **função** (significado, não ligação).
  Um símbolo genérico (pinos 1,2,3) num footprint de função fixa dá netlist
  **correcta** e função **errada**: sinal na entrada inversora, fonte do MOSFET
  no cátodo do LED, cátodo do Schottky ao contrário. **Nenhum ERC/DRC/"0 por
  rotear" vê isto.** O contrato escreve-se como *"estes dois pads têm de estar na
  MESMA rede"* (sobrevive a renomeações), nomes literais só para decisões humanas
  (`GND`, `+5V`). Provado com `--sabota`.

**Passa se:** ERC=0, paridade esquema↔placa OK, não-roteado por rede = 0 com o
invariante mantido, `funcao_dos_pads.py` sai 0.

---

## Lente B — Fabricabilidade / DFM

**Pergunta:** a fábrica e o montador constroem-na bem — e a fábrica **aceita**-a?

- **DRC vs as capacidades reais** com `.kicad_dru` (ver `fab-automation.md` §2) e
  **`--refill-zones`** — senão o DRC compara com um fill velho e devolve folgas
  fantasma. Atenção: anel, trilho/espaço mín., furo mín., **spoke térmico**
  (abaixo de 0,09/0,09 mm a 1 oz não etcha fiável e mesmo assim passa o DRC).
- **`starved_thermal`** (pad num gargalo de cobre — solda mal, conduz pior);
  **courtyards** IPC-7351B (excesso nominal 0,25 mm; medir colocações novas por
  **courtyard, não por pad**).
- **BOM ↔ CPL ↔ placa** — acordo a três (designador, orientação, valor).
- ⚠️ **A rotação de montagem não é verificável por script** (`para_fabrica`): o
  ângulo do CPL é o do footprint, a fábrica espera o da biblioteca dela; para
  muitos pacotes diferem — **confirmar peça-a-peça no preview ⟨dB⟩ antes de
  pagar**.
- O pacote sai **carimbado** com data/commit/árvore-suja; um pacote incompleto é
  impossível de confundir com um bom (`INCOMPLETA` no nome).

**Passa se:** DRC=0 com regras JLC e zonas re-preenchidas, courtyards sem
sobreposição, BOM=CPL=placa, rotação ⟨dB⟩ confirmada, carimbo presente.

---

## Lente C — Físico / mecânico + fatores humanos

**Pergunta:** cabe no mundo e no humano que a usa?

- **Encaixe na caixa** (`caixa` gates 4/5; volume + acesso + face — ver
  `quality-standards.md` §1).
- **Legibilidade do silk com o OLHO** (`design-review`) — contagem-zero autoriza
  olhar *sabendo onde*, não saltar o olho; `refs_apontam.py` para peça-errada.
- **Alcance de conectores**, **térmica** (espaço peça-a-peça para calor, cobre
  para corrente), e "lê-se do lado que se solda" — que **só o olho** confirma.

**Passa se:** os gates de `caixa` passam, o `design-review` (svg_colisoes + olho)
passa, `refs_apontam.py` sai 0, e o alcance/térmica estão anotados com número.

---

## Revisores independentes / adversariais

- **Uma pessoa diferente por lente onde possível;** no mínimo, a última linha de
  defesa não é a ferramenta que fez o trabalho (§12s).
- O revisor é **adversarial**: tenta provar que a placa está errada, não que está
  certa. "Parece bem" não é um resultado.
- Quando a pipeline corre com múltiplos agentes, cada lente é um agente separado
  com a sua ferramenta e a sua ficha — e um quarto agente concilia o livro-razão.

---

## O livro-razão de achados (para nada cair em silêncio)

Formato fixo (une `revisao-terra` + `ifa10000`), preenchido **por passagem**:

1. **O que foi revisto** — ficheiro, revisão, data, lentes/passagens cobertas.
2. **O QUE PASSOU, explícito, por lente** (R6) — resultado negativo é achado.
3. **Achados por severidade** 🔴/🟠/🟡/⚪, cada um com: **onde · regra violada pelo
   nome · consequência medível/audível · conserto mínimo · fio/componente/
   redesenho**.
4. **Ordem de ataque** — uma variável de cada vez, com o critério de aceitação
   escrito **antes** do conserto.
5. **O que NÃO foi verificado** e porquê (R8).

Preenche-se com **números**. `[x] ok` não é resultado.

> *Diagnosticado-e-não-instrumentado é o estado em que um defeito vive mais
> tempo — toda a gente sabe, ninguém conta* (§11g-sexies). **Um achado que
> reaparece vira lente/passada/portão novo no MESMO commit, nunca uma nota.** É a
> ponte para o estágio 9.

---

## Correr 3×

O conjunto A+B+C corre **três vezes**, porque a primeira passagem muda a placa
(conserta achados) e a placa mudada é um artefacto novo por rever — **qualquer
edição invalida a inspecção anterior** (`design-review`). O conjunto corre
**no mínimo três vezes** e **não pára antes de uma passagem inteira sair sem
achados novos** — se a terceira ainda achar, continua. Os três são o piso; a
estabilidade (uma passagem limpa) é o critério de paragem.

Delega a: `kicad`, `kicad_layout`, `caixa`, `revisao-terra`, `design-review`, e
`kicad/revisao_externa.py` (kicad-happy, MIT, testado em ~5800 projectos) como
**segunda opinião externa** — a ferramenta de terceiros que pode dizer não sobre
o que a nossa aprovou.

---

## Certificação de risco — a saída da revisão tripla

O livro-razão lista o que se **achou**; a certificação decide o que **EMBARCA**.
É o portão de saída do estágio 7 para o 8: `para_fabrica` só corre com o registo
em **GO**. Modelado no `ifa10000` — o protocolo de certificação canónico do
projeto — de onde vem a frase que governa tudo aqui:

> **Um waiver é uma MEDIÇÃO, não uma opinião.**

Esta secção é a instância desse protocolo para placas.

### Certifica contra o sinal, não contra o ruído

Antes de dar veredito a qualquer item, **limpa os artefactos de ambiente**. Um
DRC a devolver dezenas de *"library not configured"* vem de um `fp-lib-table` em
falta — é ambiente, não placa; resolve-o, volta a correr, e só então lê o
**resíduo real**. É a etapa 0 (`medicao-limpa`) aplicada à revisão: certifica-se
contra **sinal limpo**.

> **Corolário: um aviso que ainda não explicaste NÃO é automaticamente ruído.**
> "Ruído" é um veredito medido — não a suposição preguiçosa de quem quer fechar.

### O REGISTO DE RISCO — uma linha por item embarcado

Todo aviso embarcado e todo waiver entra no registo, **um por linha**:

| campo | conteúdo |
|---|---|
| **id** | identificador estável do item |
| **item** | o aviso/waiver, em uma frase |
| **lente/etapa** | onde apareceu (A/B/C, ou o estágio) |
| **severidade** | 🔴/🟠/🟡/⚪ |
| **evidência MEDIDA** | o número/medição que sustenta o veredito — nunca "parece bem" |
| **veredito** | `SAFE-EMBARK` (com prova) · `MUST-FIX` · `decisão-dB` |
| **ação** | o conserto, ou o encaminhamento ao ⟨dB⟩ |

**GO/NO-GO:**

- **GO** exige **zero MUST-FIX em aberto**.
- **SAFE-EMBARK** só vale com **evidência medida** — waiver sem número é
  **MUST-FIX por omissão**.
- **decisão-dB** sobe ao portão humano ⟨dB⟩; não se auto-aprova.

### Âncora — o caso C16↔J14 (porque este registo existe)

Um courtyard-overlap entre o **C16** (0805) e o **J14** (box-header DIN-41651)
parecia um **waiver razoável** — courtyards são folga nominal, sobrepõem-se sem
drama. A opinião dizia *waive*. Ao medir **corpo-contra-corpo** em vez de
courtyard, o 0805 estava **fisicamente dentro** do shroud do header de assento
rente: colisão real, não folga. A medição inverteu a opinião — **MUST-FIX**, não
SAFE-EMBARK.

> O fosso entre o que a opinião concede e o que a medição prova **é a razão
> inteira deste registo.**

E é por isso que courtyards e colocações novas se medem por **corpo**, não por
courtyard nominal (lente B).
