# Refazer o terra — folha de bancada

Escrita em 02/09 para a montagem do dia seguinte. O diagnóstico está em
battle_scars **15bm** e na varredura P3.

## ANTES de desmontar — anota isto (regra R3)

Multímetro em **AC / mV**, **com a peça a tocar** (medição sem carga dá zero
em cima de um defeito — regra R1):

| medir entre | antes | depois |
|---|---|---|
| `AGND` da Daisy ↔ trilho do permaproto | | |
| `AGND` da Daisy ↔ negativo da entrada do amp | | |
| `AGND` da Daisy ↔ `GND` dos 15 V | | |

E anota **como soa** — o chiado, com uma palavra que consigas repetir amanhã.

## O desenho

```
ILHA DE AGND  (referência — não transporta corrente)
├── gnd dos jacks dos piezos
├── AGND da Daisy
└── negativo da ENTRADA do amp
     …e mais nada.

ESTRELA  (corrente — no negativo da fonte)
├── GND dos 15 V do amp      ← fio próprio, curto e grosso
├── alimentação da Daisy
└── alimentação da ESP32
```

**A ponte entre os dois mundos já existe e é única:** a Daisy une `AGND` e
`DGND` internamente, num ponto. Não a dupliques.

## A pergunta que decide cada fio

> **Este condutor transporta corrente, ou define um zero?**

Corrente → estrela, fio próprio. Zero → ilha, e não sai de lá.

## Critério de aceitação, escrito ANTES (regra R7)

**A tensão AC entre `AGND` e o trilho SOBE** — passam a ser dois zeros
diferentes, e isso é o esperado, não um defeito. **E o chiado desce
audivelmente.**

Se o chiado **não** descer: desfaz e diz. A hipótese cai, e a próxima é a
P1 (estrela contra margarida) — pode haver um consumidor a chegar à estrela
*através* de outro.

## Uma variável de cada vez (regra R2)

Faz **só** a separação do `AGND`. Não aproveites para mexer em cabos,
condensadores ou percursos ao mesmo tempo — se mexeres em duas coisas e
melhorar, não sabes qual agiu, e uma delas pode estar a piorar.

## Cuidados de percurso

- **Sinal e retorno do piezo viajam juntos** — lado a lado ou torcidos. Área
  entre eles é espira, e espira apanha campo.
- **Cabo blindado: malha numa ponta só**, a da Daisy. Nas duas pontas ela
  vira anel e conduz.
- **Cabos de entrada não correm em paralelo com a saída do amp** — classe D
  é onda quadrada a centenas de kHz com corrente alta. Cruzamentos em
  ângulo recto (P6).

## ⚠️ Não confundir com o erro de 31/08

Ali eu mandei tirar a terra de **SINAL** entre a ESP e a Daisy, e isso matou
a placa. **Aqui não se tira terra nenhuma** — separa-se uma **referência**
de um **trilho de corrente**. São coisas opostas, e a passada P3b da
varredura existe por causa desse erro.

## Por verificar depois

- **P1** (estrela contra margarida) e **P2** (ilhas são folhas) — exigem ver
  a montagem feita.
- **O amp tem `100 µF + 100 nF` nos pinos?** Amp classe D sem banco local é
  candidato ao residual que sobrou depois de tirar a USB do Mac.
