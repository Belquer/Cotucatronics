---
name: bug-de-ordem
description: Diagnóstico das três famílias de bug de ordem do Cotucatronics — corrida de contexto, ordem de configuração, e variável com dois empregos. Use quando algo funciona às vezes, funciona no simulador mas não na placa, ou quando um teste dá resultado incoerente. Nomear a família vem ANTES de consertar.
---

# As três famílias de bug de ordem

**Bug novo: nomear a família antes de consertar.** Conserto sem nome vira
remendo, e o remendo esconde a família para a próxima vez.
Ver `docs/battle_scars.md` cap. 1.

## A — Corrida de contexto

*Armar e conferir precisam acontecer no mesmo contexto.*

Se o armamento roda numa ISR/callback e a checagem no loop (ou vice-versa),
os dois observam mundos diferentes. Sintoma clássico: contadores pareados que
"provam" o oposto do que acontece — M1==M2 dizendo que nenhuma linha prendeu,
quando o par vinha de linhas diferentes.

**Pergunta diagnóstica:** *quem arma e quem lê, e rodam no mesmo contexto?*

## B — Ordem de configuração

*Configurar antes de consumir.*

Um valor lido antes de ser configurado não dá erro — dá o default, em silêncio.
Foi assim que o vocoder ficou mudo: `r_window` preenchido depois do
`prepareToProcess`, quando o patch já tinha lido o tamanho.

**Pergunta diagnóstica:** *quem lê esse valor, e já leu antes de eu escrever?*
Na dúvida, **escrever nos dois momentos** (antes e depois) e comentar por quê.

## C — Uma variável com dois empregos

*A mais traiçoeira, porque o teste parece limpo.*

Uma coisa serve a dois propósitos, e um teste muda os dois de uma vez. O
resultado mede a soma e não separa nada. Nomeada em 17/08 depois de custar
uma escalada de ganho fantasma (`fx2_gain` 8→48→64 calibrado contra o caso
errado).

**Pergunta diagnóstica, obrigatória antes de mexer:** *quais são TODOS os
empregos desta variável?* Listar. Se forem dois, o voo precisa isolar um.

## Protocolo quando o sintoma se repete

**Na terceira repetição do mesmo sintoma, pare de gerar hipóteses e procure o
teste que corta o sistema ao meio.**

Trocar de suspeito parece progresso e não é — é a mesma medição vestida de
nova. Uma hora foi gasta com cinco hipóteses (link BLE, fader, app, DMA,
dump), nenhuma confirmada nem descartada, enquanto o **loopback** — que prova
a placa inteira em dez segundos — esperava na fila desde o começo.

Para canal de entrada, o teste que corta ao meio **é o loopback**.

## Registro

Hipótese nova: **registrar a aposta com percentual** antes de testar. Aposta
sem número é opinião que se reescreve depois do resultado.
