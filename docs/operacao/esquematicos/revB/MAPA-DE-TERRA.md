# MAPA DE TERRA — carrier rev B

**04/09/2026 · v1.0.0 · o contrato de massa da placa, antes do layout**

> **Este documento não ensina aterramento** — isso é o
> [`../ATERRAMENTO.md`](../ATERRAMENTO.md), e a varredura é a skill
> `revisao-terra`. Aqui está **este desenho**: que condutores existem, de que
> tipo é cada um, para onde vai, e **como se prova** que ficou como está
> escrito.
>
> Se este documento e o esquema divergirem, **o esquema está certo e este
> está velho** — e a §6 é o teste que apanha a divergência.

## Por que existe

O chiado de banda larga que custou três sessões foi diagnosticado até uma
regra com nome: `AGND` e `DGND` unidos **fora** da placa, formando um anel
([`ATERRAMENTO.md`](../ATERRAMENTO.md) §5). Na permaproto isso aconteceu
porque não havia desenho — havia um trilho, e tudo o que precisava de terra
foi lá parar.

Numa placa de circuito impresso o mesmo erro é mais barato de cometer e mais
caro de consertar: um vazamento de cobre entre a ilha e o plano não se vê, não
apita em lado nenhum, e só aparece como ruído depois de cinco protótipos
estarem feitos.

**Por isso o mapa vem antes do layout, e traz o teste junto.**

---

## 1 · Classificação — todo condutor é exactamente um destes três

A pergunta que decide não é "isto é terra?", é **"isto transporta corrente,
ou define um zero?"**

| tipo | corrente | destino legal |
|---|---|---|
| **força** | alta, pulsada | fio/plano próprio até à estrela |
| **referência lógica** | ~nenhuma | fica no aparelho que lê |
| **referência de áudio** | ~nenhuma | ilha de `AGND` |

### FORÇA — vai à estrela, e só à estrela

| condutor | consumidor | nota |
|---|---|---|
| retorno do módulo **decoy PD** | — | é a origem; a estrela nasce aqui |
| retorno do **step-down** | 15 V → 5 V | ⚠️ **nada de banco grande na entrada** — Middlebrook, guincho garantido |
| retorno do módulo **TPA3116** | classe D | o maior consumidor: **o fio mais curto e mais grosso** |
| retorno dos **LED dos arcade** | ~30 mA × 2 | par próprio no IDC (vias 11–14), nunca à ilha |
| retorno do **PTC/TVS** | proteção | no nó de entrada |

🚫 **Nenhum destes toca `AGND` em ponto nenhum da placa.**

### REFERÊNCIA LÓGICA — `GND`, fica onde é lida

Daisy `DGND` · ESP32-C3 · OLED · EC11 · os dois pots · os dois arcade · gates
dos AO3400A · o divisor `V_TRILHO_A2`.

⚠️ **O 3V3 dos pots sai da MESMA Daisy que os lê.** Alimentar de uma fonte e
referenciar noutra põe a diferença entre as duas dentro da leitura.

⚠️ **O divisor 100 k/22 k é referência, não força** — apesar de estar
pendurado no trilho de 15 V. O que corre nele são 122 µA. Vai ao `GND`,
junto ao pino A2 que o lê, e **não** à estrela: levá-lo à estrela injectaria
a diferença estrela↔Daisy dentro da medição de tensão do trilho.

### REFERÊNCIA DE ÁUDIO — a ilha de `AGND`

Sleeve **e ring** dos dois jacks de piezo · malha dos cabos de piezo (**numa
ponta só**) · o terra do pré-amp inteiro · comuns dos dois DPDT · sleeve dos
dois TRS de linha · os 10 kΩ de cada entrada do amp.

---

## 2 · A ilha é uma FOLHA — entra e não sai

A ilha de `AGND` toca **um** aparelho: a Daisy, pelos seus pinos de áudio. De
lá não sai para mais lado nenhum.

```
   ILHA DE AGND (só sinal, corrente nenhuma)
     jacks de piezo (sleeve + ring) ─┐
     pré-amp, terra completo ────────┤
     comuns dos DPDT ────────────────┼── AGND ──┐
     sleeve dos TRS de linha ────────┤          │
     10 k das entradas do amp ───────┘          │
                                                │  ⚠️ A PONTE, E É A ÚNICA
   PLANO DE GND (força e lógica)                │
     estrela ── amp · step-down · decoy         │
     Daisy DGND · ESP32 · painel ───────────────┘
                          nos pinos da Daisy, na folha do soquete
```

**A ponte mora num sítio só, e o desenho tem de dizer onde**: nos pinos da
Daisy, na folha do soquete. Não é um `net tie` espalhado nem "os planos
tocam-se ali no canto" — é um ponto que se aponta com o dedo.

---

## 3 · Desacoplamento, por tipo de carga

Ver [`ATERRAMENTO.md`](../ATERRAMENTO.md) §4 para o porquê. Nesta placa:

| consumidor | leva | ⚠️ |
|---|---|---|
| **TPA3116** | 2200 µF + 100 nF nos pinos | sem o banco: zumbido cruzado |
| **step-down** (entrada) | **só 100 nF** | 🔴 **banco grande aqui oscila** (Middlebrook) |
| **step-down** (saída) | 100 µF, **não 2200** | capacitância a mais mexe na malha |
| **ESP32-C3** | 100 µF + 100 nF locais | sem isto, o rádio sai pelo altifalante |
| **Daisy** | 100 µF + 100 nF locais | — |
| **OPA1642** | 100 nF por rail, **colado** | a centímetros já não serve |

**Todo consumidor leva o PAR.** Só o grande deixa passar as arestas; só o
pequeno não tem energia. Um sozinho é achado.

⚠️ **Inrush:** o banco de 2200 µF pode fazer a fonte PD ler curto e
re-negociar. Se o arranque ficar intermitente, **é isto** — está declarado
aqui para não se caçar defeito novo.

---

## 4 · A ponte ESP32 ↔ Daisy — o laço de terra que NÃO se negoceia

As duas podem estar em estados de alimentação diferentes (uma no USB, outra no
trilho). **Existe estado em que uma está viva e a outra não** — logo:

🔴 **Terra directa ao lado dos fios de sinal, obrigatória.** Sem ela, a placa
alimentada injecta corrente pelos díodos de protecção da desalimentada, a
linha fica indefinida e quem tenta transmitir fica preso.

⚠️ **O defeito aparece do lado ALIMENTADO.** É exactamente a família do
travamento de 02/09, que se manifestava na Daisy e morava do outro lado do
cabo.

🚫 **Não tirar esta terra para resolver ruído.** Ruído de alimentação
resolve-se com desacoplamento local (§3), não cortando terra de sinal.

---

## 5 · Alta impedância e acoplamento pelo ar

- **Piezo é uma antena.** Sinal e retorno viajam **juntos**; malha ligada
  **numa ponta só** (a da placa), senão vira anel.
- **Pré-amp encostado aos jacks de entrada**, dentro da ilha, pista de entrada
  curta. É restrição de layout, não preferência.
- 🚫 **Os cabos de entrada nunca correm paralelos à saída do amp** — classe D
  é onda quadrada a centenas de kHz com corrente alta, e isso é imune a
  esquema de terra. Cruzamentos em **ângulo recto**. Na caixa: piezos à
  esquerda, exciters à direita, e é por isso.
- **O painel aceso mediu 6 dB de chão na entrada.** Por isso o relógio do OLED
  vai guardado por terra dos dois lados no IDC, e os LED têm par próprio.

---

## 6 · O TESTE QUE PROVA A PONTE ÚNICA

> **É esta secção que o `ROTEIRO-KICAD.md` §4 cita.** Uma ponte a mais não
> aparece no esquema, não aparece no render, não dá erro no ERC nem no DRC, e
> não se ouve até haver corrente a correr. Só um teste a apanha.

**E a placa foi desenhada para o tornar possível:** a Daisy está em **soquete**.
Com ela fora, a ponte física desaparece — e é isso que se mede.

### O teste, em três passos

```
1. Daisy FORA do soquete. Placa sem alimentação.
2. Multímetro em continuidade (apito).
3. Ponta A na ilha de AGND    (sleeve de um jack de piezo)
   Ponta B no plano de GND    (terra do módulo do amp)
```

| resultado | veredito |
|---|---|
| **não apita** (aberto) | ✅ **a ponte é única** — só existe através da Daisy |
| **apita** | 🔴 **há uma segunda ponte.** Não montar. Achar e cortar |

### E a confirmação positiva, para não confundir "único" com "nenhum"

```
4. Daisy DENTRO do soquete, ainda sem alimentação.
5. Mesmas duas pontas.
```

| resultado | veredito |
|---|---|
| **apita** | ✅ a ponte existe e passa pela Daisy, como desenhado |
| **não apita** | 🔴 **a ilha está flutuante** — o áudio não tem referência |

⚠️ **Os dois passos são obrigatórios, e por ordem.** O passo 3 sozinho não
distingue "ponte única" de "nenhuma ponte": as duas dão aberto, e uma delas é
um defeito grave. **Um teste que não separa os dois desfechos não é teste.**

### Critério de aceitação, escrito antes do conserto

> **Passa se, e só se: aberto sem a Daisy E fechado com ela.** Escrever os
> dois resultados no relatório, mesmo quando passam — resultado negativo é
> achado (R6 da `revisao-terra`).

### Depois, sob carga

O teste acima é de cobre, com a placa morta. O de **massa** exige corrente —
uma medição de terra em repouso dá zero em cima de um defeito. O método está
no [`ATERRAMENTO.md`](../ATERRAMENTO.md) §8: multímetro em **AC/mV**, ponta
preta na estrela, som a tocar de verdade, **todos os números anotados antes de
mexer em nada**.

Referência medida na permaproto, com tom a tocar:

| percurso | antes | depois |
|---|---|---|
| estrela → amp | 0 mV | 0 mV |
| estrela → Daisy | **42 mV** | 0 mV (fio dedicado) |
| estrela → ponte ESP32 | — | 0 mV |

**Na placa nova, a aceitação é 0 mV em todos.** Dezenas de mV querem dizer que
aquele consumidor chega à estrela através de outra coisa.

---

## 7 · O medidor de massa gratuito

Um clique periódico audível que segue um LED, um relé ou o refrescamento do
ecrã **é acoplamento a denunciar-se sozinho**. Um LED consome miliamperes: se
isso é audível, o acoplamento é grande, e tudo o resto que a placa faz passa
pelo mesmo caminho **sem padrão que dê por si**.

🚫 **Silenciá-lo por firmware esconde o defeito.** Com a massa bem feita ele
desaparece sozinho — e é esse o teste de que ficou bem feita.

---

## 8 · O que este mapa NÃO garante

Declarado, não omitido:

- **Não substitui o DRC nem a revisão de terra do layout.** É o contrato do
  esquema; o cobre pode desobedecer-lhe e nada aqui o apanha.
- **Os módulos comprados** (decoy, step-down, TPA3116) têm massa interna que
  não desenhámos e não podemos auditar. O que controlamos é **por onde entram
  e saem**.
- **A ligação AGND↔DGND dentro da Daisy** é afirmação da folha de dados, não
  medição nossa.
- **Os footprints de step-down, decoy, jacks, DPDT e EC11 não existem ainda** —
  o dB mede quando as peças chegarem. Sem footprint não há ilha desenhada, e
  parte deste mapa só vira cobre depois disso.
