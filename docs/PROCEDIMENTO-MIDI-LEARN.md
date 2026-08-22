# Procedimento recorrente — endereçar o Loopy Pro

**Método que funcionou (22/08): `vistos`, não `learn`.**

Fecharam-se 65 de 65 em duas varreduras. O modo `learn` levou cinco versões
de firmware e nunca fechou — está descrito adiante como registro, não como
recomendação.

---

## O método: a placa escuta, você varre

A pergunta certa não é *"como faço os widgets aprenderem os meus números?"*
mas *"o que os widgets estão mandando, e bate com o meu mapa?"*. Quem se
adapta é o firmware, que é o lado que se edita.

```
vistos reset      # zera o inventário
                  # ... você move TODOS os widgets, um por vez, sem pressa
vistos            # relatório
```

O relatório traz três coisas:

- **quantos dos 76 chegaram** — endereçados e corretos
- **FALTA cc<n> <param>** — nunca chegou: sem endereço, ou com número de
  outro parâmetro
- **ORFAO cc<n>** — chegou e não está no mapa: aí o conserto é no firmware,
  não no widget

`vistos` conta CCs **distintos**, não mensagens — varra à vontade, repita,
não transborda. Acumula entre varreduras até o `reset`.

## Os dois pontos cegos, e eles importam

**`vistos` só enxerga widget que ENVIA.** Um widget que só recebe é
invisível, por melhor que esteja endereçado. Em 22/08 quatro apareceram como
"faltando" e um estava perfeito — o dB o viu piscando. Se um FALTA
persistir, o teste é **apertar o widget e ver se o parâmetro muda na placa**.

**O learn tem de estar DESLIGADO.** Em learn a placa fala e não escuta:
todo CC que chega é descartado, e o inventário fica cego para tudo.

## O que os erros parecem

Quatro widgets estavam **com o número de outro parâmetro** — `wrb_on_b` no
51 (`wrb_mix_a`), e mais três. Sintoma: o parâmetro vizinho responde a dois
controles e o widget "não faz nada". O `vistos` os entrega sozinho: eles
aparecem como FALTA enquanto os vizinhos chegam.

## Ao terminar

A varredura mexe em **todos** os parâmetros — o instrumento fica tocando o
que as posições dos widgets dizem. `sync` faz o inverso: a placa empurra o
estado dela e os widgets pulam para as posições verdadeiras.

E o aviso que não muda: **sem preset em flash, a próxima gravação de
firmware zera todos os valores.** O endereçamento sobrevive (mora no iPad);
os ajustes não.

---

# Registro: o modo `learn`, REMOVIDO em v0.121.0

**Ele não existe mais no firmware.** Saiu inteiro — 136 linhas, ~10 KB de
fonte, 83 referências. Está preservado no commit `514e09c`, de onde pode ser
ressuscitado se algum dia fizer falta.

**Por que não coexistia com o `vistos`:** em `learn` a placa fala e NÃO
ESCUTA — todo CC que chega é descartado, por desenho. Mas o inventário lê
exatamente o que chega. Ligar o learn cega o único instrumento capaz de
provar que o learn funcionou. Não eram dois métodos: eram dois regimes
incompatíveis (bts 12t).

**Se precisar endereçar do zero num iPad novo**, o caminho é digitar os
números direto nos widgets, pela lista de CCs do README e do `ccmap`. É uma
passada só, determinística, e não depende de coreografia.

O que segue é o registro do mecanismo removido.

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
| `learn stop` | **pausa**, sem re-push |
| `sync` | empurra os 76 CCs de propósito, quando você quiser alinhar |

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
**8 últimas** mensagens, sem carimbo de tempo. E o caminho de volta tem
latência de BLE: mensagens antigas chegam atrasadas e se misturam às
recentes. Em 22/08 li `[26, 30, 32]` estando no 32 e quase concluí que a
placa transmitia CC errado — os 26 e 30 eram ecos atrasados de
estacionamentos anteriores. **Verificado depois: a placa transmite
exatamente o que reporta.**

**Não conte com detecção automática do arme.** Testada e reprovada por dois
motivos independentes: (a) nem todo widget devolve CC ao aprender — os
faders devolvem, outros não; (b) a latência do retorno impede distinguir
"acabou de armar" de "eco atrasado de dois passos atrás". O que sobra é o
operador dizer que armou.

---

## Pausar não é terminar

`learn stop` **pausa e não empurra nada** (9 mensagens de acerto fino, não 76).
Em 22/08 cada pausa disparava um re-push completo e despejava 76 CCs na tela
do Loopy — o dB leu isso como a placa mandando CC errado, e gastamos uma
rodada investigando um defeito que era o meu comando de pausa.

Depois de um `learn stop`, os widgets podem estar mostrando **valores dos
pulsos**, não o estado real. `sync` alinha quando você quiser.

## Ao terminar

O **fim da lista** dispara automaticamente:

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
