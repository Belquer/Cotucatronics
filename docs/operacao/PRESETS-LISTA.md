# ⚠️ MODELO CORRIGIDO EM 2026-09-01 — LER ISTO ANTES DO RESTO

O que está abaixo desta secção descreve o modelo ANTIGO, de **duas
bibliotecas** (16 de cadeia + 8 globais, com conteúdos diferentes). **Está
errado**, e o dB corrigiu-o com três frases:

> *"presets são uma lista só. O que muda é se ele é aplicado ao piezo A, B ou
> global (os dois)."*
> *"preset sempre admite dois valores porque posso selecionar um para A e outro
> para B ou vice-versa."*
> *"selecionar preset A B ou G = giro a lista até achar e confirmo. G contém os
> valores de A e B."*

## O modelo certo

**UMA lista.** Cada preset guarda um retrato completo:

| | |
|---|---|
| valores da **cadeia A** | os 39 params `_a` |
| valores da **cadeia B** | os 39 params `_b` |
| **partilhados** | `piezo_mix` · `dly_time` · `shift_hz` · `buzios_on/thresh/cooldown` |

**O destino decide quanto dele entra:**

| aplicar como | o que entra |
|---|---|
| **A** | só a parte de A, na cadeia A |
| **B** | só a parte de B, na cadeia B |
| **G** | **tudo** — parte-A na A, parte-B na B, e os partilhados |

**O estado são DOIS ponteiros**, um por cadeia: pode estar a parte-A do preset 3
na cadeia A enquanto a parte-B do preset 7 está na B. Daí `ponteiro[2]`.

## O gesto

**Gira a lista até achar, e confirma.** Não são dezasseis botões — é um
selector que percorre e um gesto de confirmação, por destino (A, B, G).

⚠️ **E isso deixa uma pergunta de protocolo em aberto**, que é a primeira a
resolver: com «gira e confirma», o CC 121 com o slot no valor já não descreve o
gesto. Ou o selector manda o índice e um segundo CC confirma, ou a confirmação
manda o índice de uma vez. **Decisão do dB antes de se escrever código** — errar
aqui é reescrever o despacho e a página do Loopy.

## O que JÁ está feito

- **Exclusões** (`presets.h` v0.9.0): saíram `out_l`, `out_r` e `pre_level_ab`,
  por decisão do dB. Os dois primeiros cortam o ramo — topologia e segurança,
  como o `route_*`; o terceiro é de teste e emergência.
- Restam **6 partilhados**, todos musicais.
- O **CC 121 carrega** e está provado na placa, mas ainda com o modelo antigo:
  ch1 lê de uma biblioteca global separada em vez de aplicar às duas cadeias.

## O que FALTA, e é a primeira tarefa

Unificar as bibliotecas em `presets.h`, e propagar. **Toca oito sítios**: as
estruturas, os helpers, o analisador do `dump`, o `capturar`, o `load`, o
despacho do CC 121, a listagem e os testes.

⚠️ **Não é trabalho de fim de sessão.** É o subsistema que vai guardar o
resultado da sessão de escuta, e foi tentado em 2026-09-01 ao fim de uma sessão longa
— a refactorização foi revertida a meio, de propósito, deixando só as exclusões.
Melhor não começada que meio feita.

---

# PRESETS — a lista, e como os botões a endereçam

> **v0.2.0** · 2026-08-28 · documento **vivo**: a lista cresce durante a sessão de
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

> Em 2026-08-22 o `cc34` foi dado ao `drv_drive` numa reorganização do mapa e
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

---

## O slot 0 do global aposenta o aviso "gravar zera os params"

Observação do dB em 2026-09-02, e muda a prioridade da biblioteca.

**"Gravar zera os params" aparece em todos os briefings desde agosto** e é
gerido por memória: repor à mão e declarar o que se repôs. Já custou pelo
menos duas vezes — um estado afirmado que o flash tinha apagado, e uma
sessão inteira a re-dialar `level` sem saber porquê.

O desenho da biblioteca já tem a resposta e ninguém a tinha nomeado:

> **Global, slot 0 = estado de boot.**

No dia em que a biblioteca existir, gravar deixa de zerar. Carrega o 0.

Isso muda o argumento da sessão de escuta: ela não é só o estrangulamento
mais antigo do projecto — é o que **aposenta uma regra de operação inteira**
e o erro humano que ela existe para conter. Regra que depende de alguém se
lembrar é dívida; regra que o arranque cumpre sozinho é desenho.

Ver a rotina de gravação na skill `bancada-firmware` §2b/§2c — as duas
deixam de fazer falta no dia em que isto existir.
