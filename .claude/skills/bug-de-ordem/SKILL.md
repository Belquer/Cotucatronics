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

## Quarta família: a lacuna do observador externo

Um processo com humano no circuito, observado de fora por varredura
periódica, tem entre o evento e a leitura uma janela em que o estado errado
é gravado. Isso **não é latência** — é corrupção.

Caso (22/08): o modo `learn` pulsava um CC para o dB armar um widget no
Loopy; um script lia o histórico a cada 3 s e mandava avançar. Entre uma
chamada e outra passavam minutos, e o firmware seguia pulsando o mesmo CC.
Todo widget armado nesse intervalo aprendeu o CC errado — quatro deles.

O sinal certo já existia e estava sendo desperdiçado: um CC chegando com o
número do alvo atual **só pode** vir de um widget que acabou de aprendê-lo.
A placa via isso e apenas incrementava um contador.

> **Quem decide o avanço tem que ser quem vê a confirmação.** Detecção de
> fora só serve quando nada muda enquanto ninguém olha.

**Sintoma que a denuncia:** "está tudo caindo no mesmo valor / no mesmo
passo". Estado que deveria variar e não varia, num processo cujo relógio
está fora do sistema observado.

**Irmã da terceira família:** um modo que anda sozinho e um modo que espera
confirmação não são configuração do mesmo procedimento — são dois
procedimentos. Tratá-los como um parâmetro é dar dois empregos a uma
variável, e escolher errado desalinha tudo em silêncio.

## Quando fechar janelas não acaba, inverta a pergunta

Cinco versões de firmware num mecanismo de endereçamento por MIDI learn: a
placa pulsava um CC, o operador armava o widget, ela avançava. Cada versão
fechava uma janela onde o número errado podia ser gravado, e a cada janela
fechada aparecia outra. Nunca fechou.

O dB inverteu: *"configurei todos para mandar no canal 1 — fala com eles e
vê se batem; se não baterem, ajusta do seu lado."* Duas varreduras e 128
bytes de contador fecharam 65 de 65, e flagraram quatro widgets com número
de outro parâmetro sem ninguém procurar.

> **Um mecanismo que precisa de janela de sincronia a cada passo, com uma
> das pontas fora da sua vista, está errado — não mal ajustado.** Procure a
> formulação em que cada lado fala quando quer, e alguém apenas escuta e se
> adapta.

**Sinal de que você está no mecanismo errado:** consertos que geram
consertos, todos na mesma família (timing, ordem, sincronia), sem que o
problema original encolha.

**E declare o que você não enxerga.** Eu dirigi três horas de coreografia
sem ter acesso nenhum ao lado do operador — não via widget, nem nome, nem
página. Isso muda qual mecanismo é viável e tinha de ser dito na primeira
mensagem, não depois de ele perguntar.

## Registro

Hipótese nova: **registrar a aposta com percentual** antes de testar. Aposta
sem número é opinião que se reescreve depois do resultado.
