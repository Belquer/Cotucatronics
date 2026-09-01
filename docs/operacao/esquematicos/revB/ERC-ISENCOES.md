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

## Isenção 3 — `power_pin_not_driven` · 2 ERROS

> *U1C pino 8 (`V+`) e pino 4 (`V-`): pino de alimentação sem `power_out`*

**Razão:** os pinos do amp-op estão declarados `power_in` e a alimentação chega
por **rótulo global** (`V+PRE`). Nenhum símbolo desta folha declara `power_out`,
porque a fonte está na folha da energia.

⚠️ **Esta é a mais frágil das três, e tem conserto real:** um `PWR_FLAG` na
rede — símbolo com pino `power_out` — cala o erro **por declarar quem alimenta**,
em vez de mandar o verificador olhar para o lado.

**Recomendação:** consertar, não isentar. É a mesma regra do díodo Schottky —
**peça que impede o erro vale mais que regra que o proíbe.** Acrescentar o
`PWR_FLAG` mexe na folha, portanto é decisão do dB.

**Prazo, se ficar isenta:** até à revisão do projecto único. Não passa daí —
alimentação sem origem declarada é exactamente o que este erro existe para
apanhar, e dois erros isentos são dois erros que ninguém volta a ver.

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
