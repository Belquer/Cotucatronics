# Procedimento recorrente — endereçar o Loopy Pro pelo MIDI learn

**Quando usar:** ao criar as páginas do Loopy do zero, ao trocar de iPad,
depois de perder um layout, ou para re-endereçar um punhado de widgets que
aprenderam o CC errado.

**Firmware mínimo:** `main.cpp` v0.116.0. Antes disso a placa não confirma o
arme e a caminhada não anda sozinha.

---

## O que o mecanismo faz

A placa percorre o mapa de CCs **na ordem das famílias** (que é a ordem das
páginas: CENTRAL, MOD, TIME, TECH) e pulsa cada um até o widget confirmar.
O operador só arma; a placa detecta e avança.

**Transmite no canal 2**, que é a voz do espelho — então o que o widget
aprender é exatamente o que vai recebê-lo em operação.

**Pulso curto, 127 seguido de 0 em 60 ms, repetido a cada segundo.** O 127
sustentado por um segundo é lido pelo Loopy como *hold*, e o widget alterna
entre hold e CC normal. O gesto que ele reconhece é apertar-e-soltar.

**A placa confirma sozinha.** Um CC que chega no canal de entrada com o
número do alvo atual só pode vir de um widget que acabou de aprendê-lo — é
prova de arme, no instante do arme. A placa loga `cc26 ARMADO` e avança.

---

## O procedimento

```
learn <primeiro cc>     # ex.: learn 20 — entra ESTACIONADO
```

Estacionado é o modo certo para a caminhada inteira: ele repete o CC atual
indefinidamente e só sai dele quando o widget confirma. A caminhada
automática (`learn` sem argumento) avança depois de 4 pulsos **tenha ou não
alguém armado**, e não serve para trabalho com humano no circuito.

Depois disso, para cada widget:

1. Ler no serial qual CC está no ar: `learn: 7/76 · cc26 · gate_release_b`
2. Armar o widget correspondente no Loopy
3. A placa loga `cc26 ARMADO — avancando` e passa ao próximo, sozinha

Comandos durante a caminhada:

| comando | efeito |
|---|---|
| `learn next` | pula o CC atual sem armar (preserva o estacionamento) |
| `learn <cc>` | reposiciona num CC específico, para re-aprender um widget |
| `learn <nome>` | idem, por nome de parâmetro |
| `learn stop` | encerra e dispara o re-push |

---

## O que NÃO fazer, e por quê (cada um custou uma rodada)

**Não deixe a placa parada pulsando enquanto ninguém opera.** O firmware
repete o CC estacionado indefinidamente. Todo widget armado nesse intervalo
aprende o mesmo CC. Em 22/08 quatro widgets aprenderam o cc26 porque foi o
que ficou no fio durante uma pausa. **Se for interromper, `learn stop`.**

**Não use polling externo para detectar o arme.** Foi a primeira tentativa:
um script lendo o histórico a cada 3 s e mandando `learn next`. O atraso
entre a varredura e o arme É o defeito — quem decide o avanço tem que ser
quem vê a confirmação, e isso é a placa. Detecção de fora tem sempre uma
lacuna, e a lacuna é onde os widgets aprendem o CC errado.

**Não confie no histórico de CCs para conferir arme antigo.** Ele guarda as
**8 últimas** mensagens. Um CC que não aparece pode ter apenas rolado para
fora.

---

## Ao terminar

`learn stop` (ou o fim da lista) dispara automaticamente:

1. `eco_ult` volta a `0xFF` — a tabela **tem** que esquecer a caminhada,
   senão o espelho compara o estado real contra os `0` dos pulsos e engole
   as arestas em que os dois coincidem.
2. Re-push dos 76 CCs pelo mesmo mecanismo do boot. Os pulsos deixaram os
   widgets em posições arbitrárias; o re-push restaura a verdade da placa
   em todas as páginas.
3. Cada empurrão abre uma janela de autoria de 250 ms, que recusa a
   devolução do widget. A placa vence, como deve.

Conferir no `midi`: `eco: TX <n>` deve ter subido ~76, e `autoria:` deve
mostrar recusas — se mostrar zero, ou os widgets não estão devolvendo, ou
a janela não está funcionando.

---

## Aceite (o que provar antes de considerar endereçado)

1. Cada widget aprendeu o **CC certo** — conferir movendo o widget e vendo
   o parâmetro certo mudar no `list`.
2. Depois do re-push, todos mostram o **estado real da placa**.
3. `list` idêntico ao de antes do learn — os pulsos são **TX-only** e não
   escrevem em parâmetro nenhum.

**Estado em 22/08: não concluído.** Foram armados e confirmados os CC 20 a
25 (CENTRAL). A caminhada parou em 7/76, cc26. O mecanismo de confirmação
automática (v0.116.0) foi implementado mas **ainda não exercitado de ponta
a ponta** — os CC 20–25 foram armados com detecção externa, o método que
esta página desaconselha.
