<!-- Vocabulario do KiCad e dos geradores da revB, nao simbolos do
     firmware. O check_docs varre `src/`; estes vivem no KiCad ou em
     docs/operacao/esquematicos/revB/, que ele nao le. -->
<!-- doc-check: ignore endpoint_off_grid (codigo de violacao do ERC do KiCad) -->
<!-- doc-check: ignore lib_symbol_issues (codigo de violacao do ERC do KiCad) -->
<!-- doc-check: ignore isolated_pin_label (codigo de violacao do ERC do KiCad) -->
<!-- doc-check: ignore power_pin_not_driven (codigo de violacao do ERC do KiCad) -->
<!-- doc-check: ignore pin_not_connected (codigo de violacao do ERC do KiCad) -->
<!-- doc-check: ignore power_out (tipo electrico de pino do KiCad) -->
<!-- doc-check: ignore power_in (tipo electrico de pino do KiCad) -->
<!-- doc-check: ignore confere_colisoes (funcao de revB/grade.py, fora do varrimento) -->

# ERC rev B — registo de isenções

**Corrido em 01/09 com `kicad-cli` 10.0.5.** Relatórios em `erc_energia.rpt` e
`erc_preamp.rpt`, ao lado deste ficheiro.

⚠️ **As isenções ainda NÃO estão aplicadas em lado nenhum.** O KiCad guarda-as
no `.kicad_pro`, e não existe projecto — as duas folhas são ficheiros soltos.
Este documento é o registo; a aplicação acontece quando o projecto nascer.
**Registo sem aplicação é promessa, e está marcado como tal.**

## Antes e depois do encaixe na grade

| folha | antes | depois |
|---|---|---|
| energia | 68 violações · 0 erros | **13 · 0 erros** |
| preamp | 159 violações · 2 erros | **38 · 2 erros** |

Os **176 `endpoint_off_grid` desapareceram** por conserto, não por isenção — as
coordenadas passaram a múltiplos de 1,27 mm. Ver `grade.py`.

**Prova de que nada se partiu:** o netlist é idêntico antes e depois. Arredondar
é função pura, portanto pontos que coincidiam continuam a coincidir; e o
`confere_colisoes` garante o inverso — que dois pontos distintos não caíram no
mesmo sítio a inventar uma ligação.

---

## Isenção 1 — `lib_symbol_issues` · 46 avisos

> *A configuração não inclui a biblioteca de símbolos 'cotuca'*

**Razão:** os símbolos são **auto-contidos** no próprio `.kicad_sch`, por
desenho — a folha abre em qualquer máquina sem símbolo em falta. O KiCad avisa
porque não encontra uma biblioteca registada com esse nome.

**Prazo:** até as folhas entrarem num projecto com a biblioteca declarada.
**Some sozinho nessa altura** — se não somar, a isenção caduca e vira defeito.

## Isenção 2 — `isolated_pin_label` · 3 avisos

> *`+15V` · `AUDIO_IN_L` · `AUDIO_IN_R` — rótulo ligado a um só pino*

**Razão:** são redes que **atravessam folhas** por rótulo global. Numa folha
isolada têm mesmo um pino só.

**Prazo:** até as folhas estarem todas no mesmo projecto. ⚠️ **E aí a isenção
inverte-se:** se continuarem com um pino só *dentro do projecto*, deixa de ser
falso positivo e passa a ser **rede sem destino** — defeito real.

## ~~Isenção 3~~ — CONSERTADA, não isentada

> *U1C pino 8 (`V+`) e pino 4 (`V-`): pino de alimentação sem `power_out`*

**Decisão do dB:** *«Declarar quem alimenta é resposta; isentar é mandar o
verificador olhar para o lado.»* Entraram **quatro `PWR_FLAG`**, um por rede de
energia — e quatro, não dois, porque o ERC não atravessa o R7: `V+PRE` é rede
distinta de `+15V` e precisa da sua.

| bandeira | rede | folha |
|---|---|---|
| `#FLG1` | `+15V` | energia — fonte: decoy PD |
| `#FLG2` | `+5V` | energia — fonte: step-down |
| `#FLG1` | `V+PRE` | preamp |
| `#FLG2` | `GND` | preamp |

⚠️ **Um por rede, e só um.** Dois `power_out` na mesma rede são conflito para o
ERC — trocaria um erro por outro.

**Resultado: zero erros nas duas folhas.**

Três coisas que custaram uma volta cada, e ficam para a próxima:

1. **A bandeira não vai na coordenada do rótulo** — o corpo tapa-o. O `+15V`
   lia-se «15V» e o `+5V` ficava com o triângulo dentro do hexágono.
2. **A referência `#...` não se mostra** — além da convenção do KiCad, o texto
   `#FLG1` caía por cima do rótulo `V+PRE`.
3. **Pino a meio de um fio NÃO liga** — só em ponta ou com junção explícita. Sem
   ela, `pin_not_connected`. E **isto é invisível no render**: fio e pino
   desenhados no mesmo sítio, sem ligação nenhuma.

---

## O que este ERC NÃO cobre

- **Footprints:** todos vazios, de propósito. Os SMD atribuem-se já; os que
  dependem de peça em mão (step-down, decoy PD, jack TRS, trimpot, botão) só
  depois de medir. **Footprint suposto é respin.**
- **As folhas que faltam:** soquete da Daisy + ponte + DPDT, e conector do
  painel. Nenhuma delas existe ainda.
- **A gramática do ficheiro**, que o `check_sch.py` não vê — ele confere a
  rede. Foi por isso que a duplicação de sufixo (`D_V_0_1_0_1`) passou por ele
  e fez o KiCad recusar a folha inteira.
