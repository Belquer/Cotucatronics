# PRESETS — a lista, e como os botões a endereçam

> **v0.2.0** · 28/08 · documento **vivo**: a lista cresce durante a sessão de
> escuta e esta tabela acompanha, para os botões do Loopy poderem ser criados
> de uma vez em vez de um a um.
>
> O sistema de presets está em [PRESETS.md](PRESETS.md); o desenho, em
> [DESENHO-PRESETS-RAM.md](DESENHO-PRESETS-RAM.md).

---

## UMA biblioteca, DOIS ponteiros

Isto não é uma escolha de implementação — é a decisão que evita um defeito, e
está escrita no `presets.h`:

> *"UMA biblioteca, DOIS ponteiros. Não são duas listas: se fossem, 'corda'
> existiria duas vezes e as cópias divergiriam no primeiro ajuste."*

Portanto **não há lista A e lista B.** Há dezasseis presets de cadeia, e cada
um pode ser carregado no A, no B, ou nos dois. Se um dia houver duas cordas
diferentes, elas são dois presets — não duas cópias da mesma.

---

## Endereçamento: um CC, três canais

O mesmo esquema de seis canais de todo o instrumento, aplicado a comando.
**O CC diz o que fazer; o canal diz onde.**

| CC | acção | ch1 | ch2 | ch3 |
|---|---|---|---|---|
| **121** | carregar preset | global | **cadeia A** | **cadeia B** |
| **122** | gravar no slot | global | **cadeia A** | **cadeia B** |

**O valor é o número do slot:**

```
0        limpo — volta aos defaults do manifest
1 … 16   presets de cadeia      (ch2 / ch3)
1 …  8   presets globais        (ch1)
```

⚠️ **O widget tem de ser BOTÃO, não fader.** O valor é a carga útil, não uma
posição: um fader varrido de 0 a 127 dispararia dezasseis carregamentos pelo
caminho. Um botão manda uma mensagem, uma vez.

### Por que 121 e 122, e não 34 ou 127

Comando mora fora da faixa de parâmetro, e isso é cicatriz:

> Em 22/08 o `cc34` foi dado ao `drv_drive` numa reorganização do mapa e
> ninguém cruzou as duas listas. O intercept de comando corre **antes** do
> mapa e retorna — então aquele fader ficou **surdo a MIDI em silêncio**, e
> cada gesto nele disparava um push de estado inteiro. Dois donos para um
> número, e o intercept vence sempre.

`120` (sync), `121`, `122` ficam juntos e longe de tudo o que é param.
`123`–`127` ficam livres para o resto: apagar, seguinte, anterior.

---

## A LISTA

> **Vazia até à sessão de escuta.** Cada linha nasce de um julgamento do dB,
> não de um palpite — o aceite de som é do músico.

### Cadeia (16 slots · CC 121/122 no ch2 ou ch3)

| slot | nome | o que é | capturado em |
|---|---|---|---|
| 0 | *(limpo)* | volta aos defaults do manifest | — |
| 1 | | | |
| 2 | | | |
| 3 | | | |
| 4 | | | |
| 5 | | | |
| 6 | | | |
| 7 | | | |
| 8 | | | |
| 9 | | | |
| 10 | | | |
| 11 | | | |
| 12 | | | |
| 13 | | | |
| 14 | | | |
| 15 | | | |
| 16 | | | |

### Global (8 slots · CC 121/122 no ch1)

| slot | nome | o que é | capturado em |
|---|---|---|---|
| 0 | *(limpo)* | volta aos defaults | — |
| 1 | | | |
| 2 | | | |
| 3 | | | |
| 4 | | | |
| 5 | | | |
| 6 | | | |
| 7 | | | |
| 8 | | | |

---

## Como criar os botões de uma vez

Uma coluna por destino, o mesmo conjunto de valores, canais diferentes:

```
coluna A   →  CC 121, ch2, valores 0,1,2,3…
coluna B   →  CC 121, ch3, valores 0,1,2,3…
global     →  CC 121, ch1, valores 0,1,2…
gravar     →  CC 122, mesmo canal da coluna, mesmo valor do slot
```

Os botões podem existir **antes** dos presets: carregar um slot vazio é
recusado com veredito próprio, não com silêncio.

---

## O que ainda não existe

⚠️ **O firmware ainda não escuta o CC 121/122.** Hoje só há `capturar
<a|b|g> <nome>` e `preset load` pela serial. Os botões ficam inertes até o
despacho ser implementado — e isso é deliberado: está no passo 3 do plano,
depois da escuta, porque ponteiros só se testam com biblioteca de verdade.

⚠️ **Gravar por botão não tem nome.** Um botão manda um número, não uma
palavra. Duas saídas: o slot nasce com nome de posição (`s01`…`s16`) e
renomeia-se depois, ou captura-se pela serial com o nome já certo. Durante a
escuta, escrever o nome na hora é provavelmente melhor — o nome é o
julgamento, e ele é mais fácil de escrever quando ainda se está a ouvir.

⚠️ **O mapa é `0xBE1B`.** Preset capturado sob outro mapa é recusado com
`gravado sob outro mapa de params` — comportamento correcto. Se o mapa mudar,
a biblioteca inteira precisa de ser recapturada, e por isso as renomeações de
param foram feitas **antes** da primeira captura.
