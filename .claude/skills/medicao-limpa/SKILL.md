---
name: medicao-limpa
description: Pré-voo obrigatório antes de QUALQUER medição no Cotucatronics — prova que o canal fala, zera o ambiente, declara unidade e regime. Use antes de rodar qualquer teste de bancada, ler a caixa-preta, medir CPU, ou interpretar contadores. É a etapa 0 do IFÁ10000.
---

# Medição limpa — etapa 0 do IFÁ10000

**Um zero num canal não confirmado não é dado. É ausência de dado.**
Quatro janelas de teste de MIDI foram lidas como "link morto" quando o
log estava mudo por um `bb arm` meu (caso 8 de `docs/battle_scars.md`).

Nenhuma medição começa antes destes cinco passos.

## 1. Provar que o canal fala — ANTES de qualquer leitura

Mandar um comando de eco conhecido e **conferir a resposta**:

```
log on
board
```

- Sem eco → **aborta**. Não interpreta, não formula hipótese, não relata.
- Com eco → o canal está vivo e o silêncio seguinte passa a significar algo.

Se um comando de status imprimir `[LOG MUDO]`, o mudo está ativo: `log on`
destrava. Firmware < v0.39.0 não declara — nesse caso, resetar.

## 2. Zerar o ambiente — faz parte da medição

Estado de teste anterior contamina o teste seguinte. Já aconteceu duas vezes:
`tone on` esquecido mascarando o piezo na medição do reverb; búzios ligados
modificando o instrumento sozinho durante os testes de MIDI.

```
tone off
set buzios_on 0
list
```

Conferir no `list` se algum parâmetro está fora do default aprovado —
varredura de CC deixa `fx2_gain` e `src_mix` bagunçados. Se estiver, **o
RESET devolve o manifest aprovado** (e num teste de MIDI, exige reparear).

## 2b. Conferir que ninguém mais está mexendo

Estado de teste anterior não é a única contaminação. **Comandos fantasma
existem:** transições de link BLE fazem o iOS reenviar o estado de todos os
widgets, e o instrumento obedece. Já pôs sete parâmetros no máximo no meio de
uma sessão — `src_mix` e `gate_thresh` no topo emudeceram tudo.

```
ble      → quantas (re)conexões e quantos CCs a quarentena recusou
list     → algum param no extremo (0.00 ou 1.00) que você não pôs lá?
```

**Sete parâmetros no topo simultâneo não é gesto humano.** Se o `list` mostra
extremos que você não colocou, a medição anterior está contaminada e o
instrumento precisa voltar aos defaults antes de qualquer leitura.

## 3. Declarar o que está sendo medido

Toda medição registra, por escrito, antes de rodar:

- **unidade**: µs? % de que prazo? RMS, pico ou norm? real ou normalizado?
- **regime TEMPORAL**: estreia ou permanente?
- **regime FÍSICO**: como o sinal está sendo produzido? Um transdutor solto,
  batido por contato direto, é outro instrumento que o mesmo transdutor
  montado e excitado por vibração. Medi a entrada com **dedo no disco solto**
  e registrei "a entrada satura" — afirmação sobre o instrumento montado,
  feita com uma medição que nunca o tocou (cicatriz 12a).

> **O regime é parte do dado, não contexto do dado.** Um número sem regime
> declarado não é medição incompleta: é medição cuja validade ninguém pode
> julgar, inclusive quem a fez.
- **contexto**: qual bloco (48/256), qual clock, `-O2` ou `-O3`?
- **material**: qual sinal está entrando (ver etapa 6 do IFÁ10000)?

## 4. Um voo, uma variável

Antes de mexer, **listar os empregos daquela variável**. A terceira família
de bug de ordem é "uma coisa com dois empregos, e um teste que muda os dois
de uma vez". Se a variável serve a dois propósitos, o voo mede dois efeitos
somados e não separa nenhum.

## 5. Caixa-preta: armar por comando, no início do voo

- `bb arm` **explicitamente**, no começo — nunca no meio de um fluxo.
- **A sessão de leitura não escreve nada.**
- `bb arm` silencia o log até `log on` (ou reset). Está declarado no
  cabeçalho dos comandos de status a partir da v0.39.0.

## 5b. Antes de dimensionar, confirmar que o problema existe no regime real

Dimensionar cedo é resolver um problema que talvez não exista, com um número
que com certeza não vale.

Quando há medição pendente, a ordem certa é fazer primeiro **o que serve para
os dois desfechos possíveis**. Caso (20/08): antes de dimensionar atenuador
para uma saturação medida fora do regime de uso, entra a **proteção** — que não
mexe no ganho, protege o ADC clipando ou não, e não depende do resultado da
medição.

## 6. Antes de gastar código numa hipótese, a medição que a MATARIA

Hipótese boa e endossada pelo próprio código ainda é hipótese. Antes de
implementar, rodar o teste que a **derrubaria** se ela estivesse errada.

Caso (20/08): a aspereza do shimmer parecia ruído de emenda de grão — o
cabeçalho do `detune.h` até prescrevia "grão mais longo". A medição de
**planura espectral** da faixa suspeita respondeu em trinta segundos:

```
cauda do shimmer 0,009   |   cauda seca de reverb 0,004   →  TONAL, não ruído
```

Grão mais longo teria custado memória e **piorado** a faixa. Hipótese morta
antes de virar código.

- **planura ≈ 1** → ruído (emendas, aliasing, granulação)
- **planura ≈ 0** → tonal (som legítimo no lugar errado)

Os dois pedem consertos opostos: filtro de ruído contra o primeiro, **EQ**
contra o segundo.

## 7. Régua medida, não ouvido, para escolher profundidade

Quando o conserto é de grau (quanto cortar, quanto realimentar), a referência
é uma **medição do sinal que já soa certo**, não uma escolha de gosto.

Caso: para escolher a queda de presença do shimmer, a régua foi a razão
(2–6 kHz)/(300–2000 Hz) da cauda **seca** — o timbre natural do instrumento
na sala. Seco 0,28; shimmer sem sino 1,01; com −6 dB em 3 kHz, 0,19.

## 8. A queixa tem tantas partes quantas o dB nomeou

*"Agudas **e médio-agudas**"* são duas partes. Consertar uma, medir melhora e
publicar vitória deixa a outra em pé — e às vezes **pior**: tirar energia de
cima fez a faixa médio-aguda crescer de 26,4% para 35,7% como fatia do total.

Antes de publicar, reler a própria medição perguntando **o que ela diz sobre
a parte que eu não ataquei**.

## 9. Numa queixa com "mas", o pedido é o que vem depois

*"Quando ligo o drive tenho mais ganho **mas** a distorção legal
desapareceu."* Li o ganho como o defeito e reprojetei a normalização do
módulo para eliminá-lo. O ganho estava certo. A distorção ausente era o
pedido, e eu consertei a reclamação que ele não fez.

Pior: **os dois números que decidiam estavam na minha tela quando escolhi.**
As duas normalizações davam THD idêntico (12,0%); a diferença era o peso da
camada, +30,4 dB contra −3,2 dB. Escolhi a que resolvia o ganho.

> **Quando duas grandezas mudam juntas numa queixa, a que o operador NOMEIA
> como boa não é a que ele quer consertada.** Antes de mexer, repetir a
> queixa em voz alta e sublinhar o pedido.

E o corolário da etapa 6: **hipótese sobre módulo com matemática fechada se
testa replicando a matemática em dois minutos, não gravando a placa.** As
duas medições que decidiram o caso do drive — camada por tone, THD por
nível — rodaram no host, sem tocar no hardware, e mataram duas hipóteses
minhas antes de virarem código.

## 10. Número dito de passagem é medição também

Em 22/08 relatei "CPU 180 µs" numa frase lateral, ao lado de um 509 da mesma
sessão. Parece otimização de 65%. Os dois eram **regimes diferentes**: 181 é
o piso com todo efeito desligado (default pós-gravação), 526 é o mesmo
perfil do 509. Nada tinha mudado.

A regra já estava escrita nesta skill. Escapou porque a menção era *de
passagem*, não uma medição formal — e é exatamente aí que ela escapa.

> **Se um número aparece perto de outro da mesma grandeza, o leitor VAI
> compará-los.** O regime tem que vir junto, ou a comparação vira uma
> mentira que ninguém escreveu de propósito.

## 11. Verificação que exige desligar o sensor é desenho invertido

Dois mecanismos resolviam o mesmo problema e eram mutuamente exclusivos: um
deles precisava que a placa **não escutasse** para funcionar — e escutar era
a única forma de provar que ele tinha funcionado.

> **Método que exige desligar o sensor que o verificaria está invertido. O
> certo deixa o mundo agir e observa; o errado age e supõe.**

Reconhecer cedo: se o mecanismo pede para suspender a entrada, silenciar um
canal, ou "não escutar agora", perguntar **o que deixo de poder observar
enquanto ele roda**. Se a resposta incluir a própria coisa que ele deveria
produzir, parar ali — não são cinco versões de ajuste, é o mecanismo errado.

## 12. Orçamento e parede não são o mesmo limite

    TETO  = orçamento administrativo. ESCOLHIDO. Onde se decide que a
            carga está alta demais para o conforto.
    PRAZO = parede física. IMPOSTO pelo hardware. Onde falha de verdade.

Em 22/08 uma defesa foi proposta como "M picos acima do **teto** numa janela
T". Com o número na mão — pico 975, teto 850, zero falhas — ficou evidente
que ultrapassar o teto é rotina benigna nessa carga: o critério derrubaria
efeito o tempo todo numa configuração que toca limpa.

> **Confundir orçamento com parede faz a proteção criar o defeito que
> deveria evitar.** Toda defesa declara em qual dos dois mira — e a resposta
> muda o critério, não só o número.

**E o erro só apareceu ao DIMENSIONAR.** No papel a formulação soava
razoável. Dimensionamento não é detalhe de implementação: é onde a opção é
testada contra a realidade.

## 13. Constante escolhida ≠ constante medida — etiquete

`kEstLimite = 3` / `kEstJanelaMs = 2000` vieram de um CRITÉRIO declarado
("não punir o evento isolado"), não de medição — o fenômeno media zero. O
comentário diz isso, e diz para revisar se o uso real discordar.

> Daqui a meses, "por que 3?" tem que achar a resposta no código, em vez de
> virar arqueologia.

## 14. Conserto muda o regime — refaça o que ele tornou obsoleto

Em 22/08, uma chave de potência barrava o sinal de áudio antes do
amplificador. Com o sinal barrado o amplificador ficava em vazio, e a fonte
media **13 V firmes sob carga** — medição correta, e que descartava a fonte.

Consertada a chave, o sinal passou, a carga apareceu, e a fonte caiu para
6 V. **A medição anterior era verdade sobre o sistema quebrado, não sobre o
consertado.**

> **Ao consertar uma causa, listar quais medições aquele conserto tornou
> obsoletas — e refazê-las.** "Já descartei X" tem validade limitada ao
> regime em que X foi descartado.

É a mesma família da cicatriz 12a (medir o instrumento num regime que não é o
de uso), com uma torção: aqui o regime mudou **por causa do nosso próprio
conserto**, não por descuido de montagem.

## 15. A hipótese plausível é a perigosa — leia a fonte da configuração

Em 22/08 o JEDEC ID vinha `FF FF FF` e a explicação era boa: *a libDaisy põe
o chip em QPI; chip em QPI ignora comando de 1 linha; a linha fica em
pull-up; ler barramento em repouso dá `FF`.* Explicava o sintoma **byte a
byte**.

Estava errada. O código dizia:

```cpp
// we haven't added the ability to switch to quad mode. So they're
// currently unused.
QSPIHandle::Result EnterQuadMode() __attribute__((unused));
```

Seis gravações varrendo dummies e modos não responderam o que um `grep` no
`Init` respondeu em um minuto.

> **Quando o sintoma tem explicação plausível, a plausibilidade é o perigo.**
> O teste que decide não é o que CONFIRMA a hipótese — é o que **lê a fonte
> da configuração**.

É o inverso da etapa 6 ("a medição que a mataria"): ali a hipótese era fraca e
a medição a matou barato. Aqui era boa demais para ser questionada, e o que a
derrubou foi **leitura, não medição**. Antes de varrer parâmetros de um
periférico, ler quem o configurou.

## 16. Item de backlog é afirmação sobre o código, e envelhece

*"`frz_size`: 61% de curso morto, faixa 20–400"* — verificado no fonte: a
faixa é 20–165 e o clamp é 166,7. Batem. Corrigido meses antes, e o item
seguiu na fila, citado por mim em vários alinhamentos.

> **Antes de citar uma pendência, verificá-la** — sobretudo as antigas, que
> são as que mais provavelmente já foram consertadas por outro caminho.

## 17. A primeira leitura é aquecimento — inclusive de uma sonda velha

A regra §1 diz para provar que o canal fala. Este é o irmão dela do lado do
instrumento: **a primeira aquisição depois de abrir o dispositivo é lixo.**

Caso (28/08): a primeira corrida de um mapeamento deu `ch3/ch4` a −28,8 dBFS
onde depois se mediu −77,2 — 48 dB de diferença, e nenhum caminho do código
que a produzisse. Era o transiente da stream do `sounddevice` a ser apanhado
pela primeira `rec`.

Duas defesas, as duas baratas:

```python
rms(0.4)                      # leitura de aquecimento, deitada fora
x = x[int(0.15*SR):]          # descarta o ataque dentro de cada aquisição
```

E a verificação que separa dado de artefacto: **medir o MESMO estado três
vezes.** Com o instrumento quente repetiu a 0,1 dB.

> **Número que não fecha com nenhum caminho do código é suspeita contra o
> INSTRUMENTO antes de ser contra o sistema.** Verificar a sonda custa
> segundos; caçar um defeito que não existe custa a sessão.

## 18. Sinal periódico contínuo mede mal tudo o que faz cópia de si próprio

Delay, reverb, chorus, freeze — todos devolvem uma cópia atrasada da entrada.
Com um seno contínuo, o seco e a cópia somam-se **com fase fixa** e
interferem. O número resultante não é o nível do efeito: é o resultado da
interferência.

Caso (28/08): com o delay ligado a saída mediu −44,4 dBFS contra uma base de
−35,6. **Ligar um efeito baixou o nível 8,8 dB** — e nada no firmware fazia
isso.

A comparação válida saiu de outro par, com uma variável só e o **chão** como
referência:

```
route_r_a=0, delay desligado   −69,2 dBFS   (chão)
route_r_a=0, delay LIGADO      −39,5 dBFS   (+29,7 dB acima do chão)
```

> **Para "está cá ou não está", compare contra o CHÃO com uma variável só** —
> nunca dois estados que ambos contêm interferência. Se precisar do nível
> absoluto do efeito, use rajada ou ruído, não seno.

## 17b. O aquecimento aplica-se à PLACA inteira, não só à sonda

O item 17 diz que a primeira leitura é aquecimento. **Vale também para a
primeira medição depois de uma GRAVAÇÃO** — e nesse caso o erro é enorme, não
marginal.

*Medido em 02/09 com dois builds inertes (só um comentário a mudar): a primeira
leitura após o flash deu ~15 000 voltas/s e as três seguintes ~22 800, nos dois.
Descartada a primeira, os builds diferem **0,27%** — contra 1,3% de dispersão do
próprio instrumento no mesmo binário.*

Esse artefacto gerou um item aberto — "5 pontos de base por explicar" — que
sobreviveu dois dias, atravessou dois pedidos de conselho, e quase levou a um
diff de mapa do linker e a mover o laço para ITCM. **A regra estava escrita e o
número que a aplicava custava três minutos.**

> **Antes de atribuir uma diferença entre builds, meça a dispersão DENTRO de
> um.** Se a diferença não sair desse envelope, não há o que caçar — e o
> envelope mede-se descartando a primeira leitura.

## 19. Código raro no caminho quente custa pelo TAMANHO, não pelo que corre

Um `if` que quase nunca dá verdade é barato. **O corpo dele, inlinado, não é.**
Se o compilador arrasta para dentro da função quente um desenho, uma fonte ou um
parser que corre uma vez por arranque, a I-cache paga a conta **a cada volta**.

*Medido: ligar a abertura do OLED no corpo do laço derrubou a base de ~21 500
para ~18 600 voltas/s **com o splash parado**. Um `__attribute__((noinline))`
devolveu oito dos treze pontos — e foi o **único delta** entre os dois builds,
que é o A/B mais apertado que este projecto consegue.*

Sintoma que denuncia: **a base mudou e o trabalho não.** Se o número de voltas
por segundo cai sem que nada novo corra, procure inchaço de código antes de
procurar trabalho novo.

## 20. Linha de base guardada apanha regressão que ninguém foi procurar

O defeito acima não foi apanhado por suspeita. Foi apanhado porque **havia um
número de base da mesma sessão** e a comparação ofereceu-se sozinha.

> Toda medição vale duas vezes: uma para a pergunta que a motivou, outra como
> linha de base para a pergunta seguinte. **Registe a base mesmo quando ela não
> é a resposta.**

## Ao terminar

Relatar o número **com a unidade e o regime junto**. Número sem unidade já
condenou um módulo bom (freeze, 233 965 µs de estreia contra 3 520 de regime).
