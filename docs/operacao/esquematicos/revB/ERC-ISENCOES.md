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

> ⚠️ **05/09 — `U1C` passou a chamar-se `U1` unidade 3.** O OPA1642 era três
> símbolos de uma unidade (`U1A`/`U1B`/`U1C`) a fingir de multi-unidade, o que
> dava três linhas de BOM e três pegadas para um chip. É agora um símbolo de
> três unidades. Os pinos 4 e 8 são os mesmos; o nome é que mudou.

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

---

## 04/09 — `isolated_pin_label` em `AUDIO_IN_L` e `AUDIO_IN_R`

**Estado: ✅ MORREU EM 05/09**, no commit que criou `cotuca_soquete.kicad_sch`.
`AUDIO_IN_L` liga a `C14.2` e `U2.16`; `AUDIO_IN_R` a `C24.2` e `U2.17`.
*(eram `C4A` e `C4B` até 05/09 — ver a nota de renomeação no fim.)*
Zero menções no relatório. **O prazo cumpriu-se como estava escrito** — e é
a primeira vez que uma isenção deste repo expira por ter sido resolvida, e
não por alguém a apagar.

```
[isolated_pin_label]: Label connected to only one pin
    @(299.72 mm,  68.58 mm): Global Label 'AUDIO_IN_L'
    @(299.72 mm, 173.99 mm): Global Label 'AUDIO_IN_R'
```

**Porquê agora é falso positivo:** são as saídas do pré-amp. O destino delas
são os pinos de áudio da Daisy, que vivem na **folha do soquete — que ainda
não foi desenhada**. Hoje o rótulo toca um pino só porque só existe uma ponta.

⚠️ **E esta é exactamente a isenção que se INVERTE** (skill `kicad` §7): num
projecto de folha única é falso positivo legítimo; **assim que a folha do
soquete entrar, a mesma linha do relatório passa a significar rede sem
destino — defeito real.** A mesma frase, dois significados, e quem não marcar
a data não repara.

**Prazo:** morre no commit que acrescentar `cotuca_soquete.kicad_sch`. Se
sobreviver a esse commit, **não é isenção, é defeito**.

## 04/09 — `power_pin_not_driven` no U1 unidade 3 (pinos 4 e 8)

**Estado: ✅ RESOLVIDO em 04/09** — não por isenção, por PWR_FLAG.

Símbolos caseiros não declaram `power_out`, então todo pino `power_in` fica
órfão. A resposta é um **PWR_FLAG por rede de energia** (skill §6), e não uma
isenção: aqui a rede `V+PRE` é mesmo alimentada através de `R7` a partir do
`+15V`, e é isso que o desenho tem de dizer.

⚠️ **UM por rede, e só um** — dois `power_out` na mesma rede trocam um erro
por outro. E a bandeira não vai na coordenada do rótulo (tapa-o): vai num
ponto livre do mesmo fio, com junção.

**Como ficou:** símbolo `cotuca:PWR_FLAG` novo (pino `power_out`, referência
`#FLG`, não entra na BOM), com uma bandeira em `V+PRE` e outra em `AGND`.
Cada uma fica num ponto livre do fio, com junção — **não** na coordenada do
rótulo, que taparia o rótulo.

**Uma por rede, e só uma.** Duas `power_out` na mesma rede trocam um erro por
outro.

Efeito: ERC do projeto de **4 para 2 violações**, e o netlist **não mudou** —
23 redes, 103 nós. As bandeiras alimentam a rede para o ERC sem entrar como
componente, que é o que a convenção da referência `#` existe para fazer.

---

## 05/09 — `isolated_pin_label` × 25, sinais do painel

**Estado:** isenção **COM PRAZO**. Expira no commit que criar a folha do
conector IDC 2×13 (`ROTEIRO-KICAD.md` §5).

São os rótulos que a folha do soquete leva da Daisy para o painel — OLED
(`RST/DC/CS/CLK/DIN`), os três encoders, os dois pots, os quatro botões e os
dois LED. Hoje tocam um pino só porque só existe uma ponta.

⚠️ **É a mesma isenção que se inverte** (skill `kicad` §7): quando a folha do
conector entrar, a mesma linha passa a significar **rede sem destino**.

## 05/09 — `pin_not_connected` × 4: D20, D21, D29, D30

**Estado: NÃO é isenção — é o desenho certo, e fica declarado.**

`D20` e `D21` estão livres de propósito. **`D29` e `D30` são as linhas de
dados do USB** (pinos 36/37, `USB_HS_D−`/`D+`): não podem ser usados, e a
ausência de ligação é o comportamento correto — ver `PINOS.md`.

Se algum dia estes quatro deixarem de aparecer aqui, alguém ligou alguma
coisa a eles, e nos dois últimos isso mata a serial.

---

## 05/09 — as referências foram renomeadas, e porquê

O KiCad trata uma referência que **não termina em dígito** como não anotada, e
renumera-a sozinho ao gravar — a correspondência com os desenhos e a BOM
quebrava em silêncio. As 29 que estavam nesse estado foram renomeadas antes de
ir para a fábrica. **A dezena passou a dizer o canal.**

| era | é | | era | é |
|---|---|---|---|---|
| `R0A`…`R4A` | `R10`…`R14` | | `R0B`…`R4B` | `R20`…`R24` |
| `C1A`…`C4A` | `C11`…`C14` | | `C1B`…`C4B` | `C21`…`C24` |
| `RVA` / `RVB` | `RV1` / `RV2` | | `DA` / `DB` | `D11` / `D21` |
| `JA` / `JB` | `J5` / `J6` | | `SWL` / `SWR` | `SW1` / `SW2` |
| `U1A`/`U1B`/`U1C` | `U1` unidades 1/2/3 | | `R20`/`R21` (soquete) | `R50`/`R51` |

Os `R20`/`R21` do soquete tiveram de sair da dezena 2 porque a referência é
única em **toda a hierarquia**, não por folha, e o canal B do pré-amp passou a
ocupá-la.

**Critério de aceitação da renomeação:** a netlist ficou idêntica depois de
aplicar o mapa — comparada pela *pertença* de cada rede, o conjunto de
`(referência, pino)`, e não pelos nomes das redes, que mudam de propósito. Foi
essa comparação que apanhou um símbolo (`R0A`) que escapara à renomeação porque
o seu bloco começava colado ao parêntesis do anterior.

---

## 05/09 — `isolated_pin_label` no `+3V3A`: **RESOLVIDO, e era um defeito**

O aviso apontava o pino 21 da Daisy (`+3V3A`) com rótulo global e nenhum
consumidor. Não era ruído de ERC: era o sintoma de os **pots estarem
alimentados pelo trilho errado**.

A ficha do Seed3 diz, textualmente, *"Both the Digital and Analog +3.3V pins
will output the voltage from **their regulators**"* — são dois reguladores — e
*"65535 corresponding to **+3V3A**"*: o fundo de escala do ADC é o analógico.
A figura de aplicação do próprio fabricante chama-se *"4.1 - Potentiometer"* e
alimenta-a do pino `+3V3A`.

Os pots estavam no `+3V3` digital e lidos contra o analógico. **A diferença
entre dois reguladores independentes ia inteira para a leitura** — que é
exactamente o que a nota do painel já avisava, enquanto o esquema fazia o
contrário dela.

**O que mudou (05/09):**
- a **via 9** do IDC passou de `+3V3` a **`+3V3A`**, e alimenta só os dois pots
  (0,66 mA);
- o **OLED foi para os 5 V** — o módulo Waveshare aceita 3,3 ou 5 V, e um ecrã
  comutado no fundo de escala do ADC seria pior do que o problema resolvido;
- o rótulo `+3V3` do pino 38 **saiu**: nada o consome, e rótulo global com um
  pino só não é uma rede, é um aviso a fingir de ligação. O pino continua
  disponível como saída para quem declare o consumidor.

**Não foi isenção. Foi conserto**, e o guarda de terra não foi tocado.

## 05/09 — `D29`/`D30` deixaram de ser pinos livres

São o `USB_HS_D−`/`D+`. Levavam a regra só em documento (cicatriz 15by, em que
o `PINOS.md` chegou a reservá-los para os DPDT). Passaram a ter **`no_connect`
no esquema**: o ERC recusa agora quem lá ligue um fio.

Os outros 11 pinos livres **não** levam `no_connect` — são reserva declarada no
`FUTURO.md`, e prometer "nada se liga aqui" seria mentir sobre eles. Por isso o
ERC do cérebro fica em **11 `pin_not_connected`**, e é o número certo.
