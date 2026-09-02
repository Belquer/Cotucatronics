---
name: instrumentacao
description: Regras para escrever ou alterar código de instrumentação no Cotucatronics — log, caixa-preta, contadores, sondas, mscope. Use SEMPRE que for adicionar ou mexer em qualquer coisa que meça, registre ou reporte. Oito casos provam que instrumentação falha como código de produção.
---

# Instrumentação — a família de bug mais repetida do projeto

**Instrumentação é código de produção e falha como código de produção.**
Oito casos em `docs/battle_scars.md` cap. 2, cometidos inclusive *depois de
nomeada* a família. Parte do IFÁ10000.

## A escalada, para saber o que está em jogo

- Casos 1–7 estragaram **números** (um pico perdido, um perfil sobrescrito).
- Caso 8 estragou uma **conclusão**, e a conclusão foi **publicada** no
  battle_scars como causa confirmada de uma falha. De lá ela voltaria como
  premissa da sessão seguinte.

Instrumentação quebrada não para na planilha.

## Regras duras

1. **Dado de evento único grava uma vez e trava.** Não atualiza. A janela de
   graça zerava o medidor e apagava o pico de estreia — o dado que ela existia
   para revelar.
2. **Estado vence contagem.** Um token de voo (`in_flight`) é imune ao problema
   do armamento; contadores pareados não são — já produziram álibi falso.
3. **A sessão de leitura não escreve nada.** Contadores `static` zeram no boot:
   se a leitura escreve, você mediu a leitura, não o voo.
4. **Toda escrita de instrumentação passa pelo mesmo portão** (`bb_rec`). Um
   marcador fora do portão contamina a cena do crime.
5. **Armar por comando explícito no início do voo**, nunca no meio de um fluxo.
6. **A medição não pode alterar o que mede.** Se puder, prove que não alterou
   (voo enxuto de controle) antes de confiar no número.

## Regra do caso 9 — instrumentação que AGE

A escalada da família, completa: **números → conclusões → ações.**
Casos 1–7 corromperam *medições*. O caso 8 corrompeu uma *conclusão*, e ela
foi publicada. O caso 9 **agiu**: um watchdog leu 4 277 071 629 µs (subtração
estourada), tratou como estouro de prazo e **desligou um efeito saudável** que
gastava 35 µs de 500.

> **Um instrumento que só observa erra e alguém percebe. Um instrumento que
> age erra e altera o sistema antes de alguém perceber.**

**Se o mecanismo INTERVÉM (veto, bypass, mute, reset automático):**

1. **Teste de plausibilidade antes do gatilho.** Amostra fora de faixa
   física é **descartada e contada**, nunca julgada.
2. **O contador de descartes aparece no status.** Senão o descarte vira outro
   silêncio, e você troca um erro visível por um invisível.
3. **A ação é permanente até rearme explícito.** Desfazer sozinho transforma
   defeito reprodutível em intermitente — e intermitente é o mais caro de
   todos.

## Regra do desperdício sem sintoma

**Desperdício que preserva a correção não tem sintoma.** Código que faz o
trabalho duas vezes produz a saída **certa** — revisão não pega, teste não
pega, ouvido não pega. Só medição pega, e só se alguém medir.

Em 18/08 três efeitos seguidos pagaram o dobro pela mesma forma: a API
oferecia estéreo, o dado era mono, e passar o mesmo valor duas vezes parecia
inofensivo.

**O conserto não é disciplina** — falhei três vezes *sabendo* da regra. É
fazer **o caminho barato ser o padrão** e o caro ter que ser pedido.

> **E a mais cara: conserto sem perguntar "onde mais esta forma aparece?"
> reincide.** Trinta segundos de pergunta teriam pego as três de uma vez — e
> explicam também os quatro mecanismos que falharam em silêncio neste projeto
> (`log_muted`, `cdc_dead`, `Add()`, `AddMidi()`), consertados um a um.

**Ao consertar qualquer coisa, antes de fechar:** procure a mesma forma no
resto do código. `grep` pelo padrão, não pelo sintoma.

## Regra da mensagem que afirma — cite o outro estado

Antes de escrever qualquer frase de diagnóstico, pergunte:
***"que outro estado produz exatamente esta evidência?"***

Se houver um, **a frase tem que citá-lo**. Em 18/08 quatro instrumentos meus
trataram ausência de dado como dado — "provável QUEDA" quando ninguém mexia,
"degradação" no que era alguém tocando, "não voltou" com 21 CCs aplicados
embaixo, e "sessão MIDI ausente" com o link perfeito.

**O padrão não é esquecer a regra — é aplicá-la ao sistema e não a si mesmo.**
Nas quatro vezes a ambiguidade era conhecida; nas quatro o código resolveu
**escolhendo um lado**, porque escolher produz uma frase útil e admitir não
produz.

> **A correção nunca é o limiar. É parar de afirmar.**

Instrumento que não distingue dois estados tem que dizer isso **e vir com o
teste que distingue**:

```
>>> Se voce JA mexeu num controle, a sessao caiu: reconecte.
>>> Se ainda nao mexeu, isto aqui nao diz nada: mexa e olhe de novo.
```

**Corolário:** instrumento novo não resolve a pergunta — ele **move a
fronteira** do que ainda é ambíguo. Saiba onde a fronteira nova ficou.

## Regra da defesa órfã — declare a ameaça, ou ela vira folclore

Toda proteção que você adiciona (quarentena, veto, filtro, bypass, janela de
graça) **precisa declarar no código a ameaça que a justifica**. Não por
documentação: porque sem isso ninguém consegue perguntar *"essa ameaça ainda
existe?"*.

Em 18/08 a quarentena de reconexão — que tinha barrado 154 CCs de despejos
reais — passou a descartar a **inicialização legítima** do instrumento, porque
duas decisões corretas removeram a ameaça e ninguém revisou a defesa. Cinco
mensagens no boot, zero aplicadas, instrumento mudo, sintoma idêntico a
"o MIDI não responde".

**Nenhuma linha estava errada.** Mudou o mundo em volta.

**Ao remover uma classe de ameaça, liste o que existia por causa dela.**
Defesa que perdeu o inimigo raramente some sozinha — a menos que esteja no
**escopo do defeito** (ver o passa-alta que saiu junto com o vocoder, cap.
11b do battle_scars). Escopo largo = sobrevivência indevida.

**Rebaixar costuma ser melhor que remover:** a quarentena virou sensor —
continua detectando e anunciando, parou de descartar. Preserva o diagnóstico
e devolve o controle.

## Regra do filtro — "implausível" não se define por magnitude

Se você filtrar outliers por tamanho, **descarta a catástrofe junto com o
ruído**: um estouro real de 5 ms parece exatamente um erro de relógio.
Ajustar o limiar só move a cegueira — do outro lado do corte há sempre uma
catástrofe do mesmo tamanho.

**Outlier reprodutível é bug achável.** Em 18/08 os outliers do relógio eram
`System::GetUs()` dividindo antes da subtração: o valor devolvido só usa 1/200
da faixa de 32 bits e dá a volta a cada ~21,5 s, então a subtração `uint32`
não corrige. Conserto: medir em **ticks crus**, converter **depois** da
subtração — e aí o filtro deixa de ser necessário.

**Antes de filtrar, pergunte:** *por que este número existe?* Filtro é o que
se faz quando a causa é desconhecida **e** o custo de investigar é alto; nos
dois outros casos é um curativo que esconde.

**Corolário para atuadores:** veto automático exige relógio **monotônico e
confiável**. Sem isso, o caso 9 está armado para reincidir.

## Regra do observador — o diagnóstico não pode depender do paciente

Serial, MIDI e caixa-preta passam todos pelo loop principal. Quando ele
morreu, **os três calaram juntos** e nenhum pôde relatar. Uma hora de
hipóteses sem observação por baixo, todas erradas.

> **Um instrumento de diagnóstico não pode depender do que ele diagnostica.**

**Ao criar qualquer canal de diagnóstico, declare de que contexto ele
depende.** Se todos dependem do mesmo, você tem um ponto único de cegueira —
e ele falha exatamente quando é mais necessário. O LED dirigido pelo callback
de áudio existe por isso: o observador vive no contexto que sobrevive.

**Sintoma diagnóstico de segunda ordem:** quando vários canais calam ao mesmo
tempo, isso não é coincidência — é a assinatura **do que eles têm em comum**.

## Regra do caso 8 — modos que silenciam

**Todo modo que silencia ou altera o instrumento tem que se declarar em todo
comando de status.** `board`, `midi` e `mscope` imprimem `[LOG MUDO]`.

E o ponto fino de desenho:

> **A declaração tem que ser imune ao modo que declara.**

Um banner impresso pelo `Log()` normal morre mudo junto com todo o resto. Por
isso existe `hal::LogAlways()`, que atravessa o mudo, e **só o banner o usa**.
Antídoto que pega a mesma doença não é antídoto.

**Aviso que aparece uma vez não é aviso; é sorte.** O `bb arm` avisava
`"log mudo ate o proximo reset"` — uma vez, no armamento. Ninguém leu.

Mesmo antídoto da cicatriz 7 (o rótulo `"transições: NENHUMA"` impresso com
todos os contadores zerados): **o instrumento declara os próprios modos.**

## Ao criar um canal de entrada novo

**Instrumento que só diz "não chegou" precisa de um par que diga "o que eu
mandei, chegou".** O `mscope` mede o que entra e nunca distingue "a fonte não
mandou" de "mandou e se perdeu". O loopback é o par dele: fonte conhecida,
caminho real, resposta binária. **Todo canal de entrada nasce com o seu.**

## Regra do canal sem medidor — a pergunta que vira hipótese

**Canal de sinal novo entra com o medidor dele junto.** Não depois do primeiro
mistério.

Caso (20/08): *"chega sinal no segundo piezo?"* O firmware media CPU, MIDI,
picos por nota, ganho do limiter e custo por efeito — e não media a **própria
entrada, por canal**. Piezo mudo e piezo baixo soam idênticos atrás de um
reverb, então a pergunta só tinha resposta por hipótese.

Um medidor cru (pré-ganho, pré-gate) respondeu antes de alguém tocar:

```
IN_L (p1): pico 0.00182 = -54.8 dBFS | 4783 amostras
IN_R (p2): pico 0.00422 = -47.5 dBFS | 9515 amostras   ← chega ao conversor
```

Pino morto daria zero absoluto. A pergunta virou "o transdutor entrega?" — que
tem outro dono.

**Medir CRU.** Depois de qualquer ganho, a leitura mistura "o que o transdutor
entregou" com "o que o firmware fez", e é a primeira pergunta que precisa de
resposta limpa.

## Regra da procedência — valor de placa viva não é default

Citei `in_gain = 8,0` como "18 dB de fábrica" em três respostas e o registrei
como defeito aberto. O default é **1,0×**, escrito na mesma linha do `Add()`.
O 8,0 era ajuste do músico, persistido até a gravação seguinte.

> **Antes de chamar um número de "default do projeto", ler o default no
> fonte.** A placa mostra o que está lá, não quem o pôs.

## Defesa sem contador é hipótese

Todo mecanismo que **recusa, descarta, trava ou sobrescreve** tem que
produzir um número que sobe quando ele age. Sem isso, "funcionou" e "nunca
rodou" são indistinguíveis — inclusive para quem o escreveu.

Três casos, todos em 22/08 e todos no mesmo módulo:

- **A separação de canal do espelho MIDI** foi declarada no cabeçalho e
  nunca escrita no código: o receptor em omni aceitava o próprio canal de
  transmissão de volta. Não havia contador de "mensagens minhas recusadas",
  então a defesa ausente parecia uma defesa silenciosa. Só apareceu lendo o
  histórico de CCs à mão.
- **A trava do teto de soma** zerava os `_on` a cada bloco sem dizer nada. O
  `set` do operador era desfeito 3000 vezes por segundo e o `list` mostrava
  0, idêntico a um efeito desligado à mão. Custou uma sessão procurando
  defeito em três módulos intactos.
- **Descarte mudo** é a família proibida: a janela de autoria descarta CCs
  dentro de 250 ms, e descartar sem contar teria trocado um laço visível por
  um sumiço invisível.

> **Se o firmware sobrescreve, recusa ou desfaz uma intenção do operador,
> ele deve isso ao operador em voz alta** — no momento do comando, não num
> diagnóstico que só quem já suspeita vai pedir.

## O contador tem que medir o que o nome dele diz

`eco_proprio` contava mensagens que **voltaram**. Li como "quantas saíram" e
relatei "a placa só mandou 4 de 8". Os dois números só coincidem se o
caminho de volta for perfeito — que era **exatamente a hipótese sob teste**.

Medir o retorno e chamar de envio responde a pergunta errada com confiança.
Com o contador de TX separado, o experimento virou 6/8 contra 8/8 e isolou a
causa num outro sistema.

> **Antes de ler um contador, dizer em voz alta o que ele incrementa.** Se a
> frase não for idêntica à pergunta que se está fazendo, o número não serve.

## Dimensionar: diga a unidade dos DOIS números

O histórico de CCs guardava 8 mensagens e eu precisava inventariar 76
widgets. Aumentei para 96 — *"96 > 76, folga de sobra"*. Cobriu **nove**.

96 era de MENSAGENS, 76 de WIDGETS, e varrer um fader manda dezenas de
mensagens. A desigualdade era verdadeira e não significava nada.

> **Antes de escolher um tamanho, dizer em voz alta a unidade do número
> escolhido E a unidade do requisito.** Se não forem a mesma, a folga é
> imaginária.

E a estrutura estava errada junto: **histórico responde "o que acabou de
chegar"; inventário responde "o que já chegou alguma vez"**. A segunda não
se resolve alargando a primeira — por maior que fique, continua sendo uma
fila que expulsa o antigo. O conserto certo custou 128 bytes.

## Todo instrumento tem ponto cego — declare o seu

O inventário `vistos` só enxerga widget que **envia**. Um widget que só
recebe é invisível para ele, por melhor que esteja endereçado. Quatro
apareceram como "faltando" e um estava perfeito — o dB o viu piscando na
tela enquanto meu relatório dizia que não existia.

> **Ao entregar um instrumento, dizer o que ele NÃO consegue ver.** Sem
> isso, o silêncio dele é lido como ausência do fenômeno, e não como
> ausência de alcance. Ver também a regra do canal mudo.

## Parser que não casa deve FALHAR, nunca devolver default

```python
return float(m.group(1)) if m else 0      # <- o 0 e a mentira
```

Em 22/08 esse padrão, num script meu de bancada, transformou "meu regex não
casou" em "o pico é 0 µs". Resultado: registrei **um defeito de firmware que
não existe**, priorizei como passo 0 de um plano, e escondi o número real —
pico de 829 µs contra um teto de 850, a 21 µs de derrubar efeito.

O medidor estava perfeito o tempo todo.

> **A regra do canal mudo vale para o código que eu escrevo para medir.** O
> instrumento que mente com mais eficácia é o de bancada, porque ninguém o
> revisa: ele é "só um script".

Num leitor de bancada: sem `else 0`, sem `or 0`, sem `get(k, 0)`. Se o dado
não veio, dizer que não veio — e conferir a saída CRUA antes de confiar num
extrator, especialmente quando o número parece redondo demais.

## Ganho de teste verifica a AÇÃO e a NARRAÇÃO

Uma defesa que mede zero em regime real é indemonstrável sem ganho de teste
(`ctos teto <us>`, `ctos prazo <us>`). Mas exercitar a ação é metade.

Em 22/08, `ctos prazo 300` provou que o critério derrubava os efeitos certos
na ordem certa — e o relatório mentiu duas vezes no mesmo teste: imprimiu
`>1000 us` enquanto contava contra 300, e rotulou como `CAIU pelo teto`
quedas que vieram do critério de estouro. As duas só apareceram porque
alguém leu o log em vez de conferir só o comportamento.

> **O relatório é a única coisa que vai existir quando o evento acontecer de
> verdade, sozinho, num palco.** Ao exercitar uma defesa, ler a narração
> dela com a mesma desconfiança com que se lê o comportamento.

Checklist ao exercitar defesa: o rótulo usa o valor EFETIVO (não o real,
quando há modo de teste)? O modo de teste se declara? Se há mais de um
critério, o registro diz QUAL agiu?

## Queda de CPU depois de proteção agindo é consequência, não melhora

Com prazo artificial o bloco caiu de 453 para 246 µs — porque a defesa tinha
desligado cinco efeitos. Um leitor apressado vê "246 µs, ótimo" onde metade
do instrumento saiu do ar.

> Gêmeo do "zero com cara de dado": **número bom com cara de vitória.** Toda
> leitura de carga vem com o retrato do que estava no ar.

## Código de diagnóstico tem as mesmas pré-condições do de produção

Ele roda no mesmo hardware, na mesma ordem de inicialização, com os mesmos
periféricos. *"É só um log"*, *"é só um recado"*, *"é só um script"* são as
três frases que precedem as falhas mais caras deste projeto.

Em 22/08, três vezes no mesmo dia o instrumento de medir virou a falha:

- um parser de bancada devolvia `0` quando o regex não casava, e eu
  registrei um defeito de firmware **inexistente** (12u);
- o relatório de uma defesa mentia sobre o próprio critério, imprimindo
  ">1000 us" enquanto contava contra 300 (12x);
- o vigia do loop lia a SRAM de **backup** antes do `hal::Init()` — domínio
  de energia não habilitado — e o resultado foi app rodando, LED batendo, e
  **USB nunca subindo**. Placa viva e muda, recuperável só por BOOT+RST e
  reinstalação do bootloader (12z).

> **Antes de acrescentar qualquer sonda: em que ponto da inicialização ela
> roda, e o que ela toca já está de pé nesse ponto?**

## Retentativa automática precisa de teto E de contador

Uma defesa que se auto-cura sem orçamento vira a doença. O rearme automático
da escuta MIDI rearmava a cada volta do loop enquanto a escuta não se
estabelecesse: **376.906 rearmes num voo**, ~3,7 s de loop gasto — que foi o
travamento que o vigia detectou.

> Toda retentativa automática precisa de **teto por unidade de tempo** e de
> um **contador do que o teto barrou** — senão "está tentando o tempo todo"
> é indistinguível de "nunca precisou tentar".

**E varra os irmãos:** a regra do trabalho ilimitado já existia neste
projeto, aplicada ao `Log()`, com comentário longo explicando por quê. Duas
funções abaixo dela no mesmo arquivo não a seguiam. **Lição aplicada num
lugar não é lição aplicada.**

## Toda linha de relatório é afirmação verificável, ou é proibida

Quatro vezes em 22/08 o instrumento mentiu, e as quatro têm a mesma
anatomia: o relatório tem **duas camadas** — o dado e a **moldura** do dado —
e a moldura foi escrita como decoração enquanto só o dado foi tratado como
código.

    pico 0 µs             <- o regex nao casou, e o codigo fez `else 0`
    ESTOUROS (>1000 us)   <- contava contra os 300 artificiais
    "CC 42 no fio"         <- a chamada so LEU; a placa seguia no 41
    placa viva: sim       <- STRING FIXA, nenhuma verificacao

> `placa viva: sim` só pode existir como resultado de um ping com timeout.
> **Sem o ping, a linha não existe** — não vira "provavelmente", não vira
> texto otimista.

Rótulo, unidade, condição e afirmação de estado são medição igual ao número.
Prosa de status mente com mais autoridade, porque ninguém revisa uma frase em
português como revisa um número.

**E "é só um script de bancada" tem ficha** — junto com "é só um log" e "é só
um recado", completa o trio das frases que precedem as falhas mais caras.

## Nunca peça ação humana de dentro de uma janela cronometrada

A mensagem "faça X agora" só aparece na tela **depois** que a chamada começa
— e a chamada É a janela. O operador lê o pedido quando o tempo já passou.

Em 22/08 isso aconteceu quatro vezes: teste do MIDI de volta, varredura de
frequência, aceite do espelho, e a contagem de batidas no botão. Nas quatro,
o resultado foi zero e o instrumento parecia quebrado.

As duas formas que funcionam:

1. **Anunciar, encerrar o turno, medir no turno seguinte.** O operador age
   quando quiser; a leitura não depende de relógio.
2. **Contador em vez de janela.** O instrumento acumula e alguém lê quando
   pedir — `N batidas desde o boot`, legível a qualquer momento.

A segunda é melhor quando o fenômeno se repete: tira a coordenação do caminho
crítico inteiro.

> **Coordenação por cronômetro entre dois agentes que não compartilham
> relógio é a forma mais cara de medir qualquer coisa.**

**E paridade não substitui contagem:** cinco e sete batidas terminam no mesmo
estado. O estado final era compatível com debounce bom E com debounce ruim.

## A regra que fecha a família — ausência mora fora do domínio

**Nenhum valor de dado pode significar "sem dado".** Ausência se representa
**fora do domínio**: flag separada, sentinela impossível, ou par
`(valor, validade)`. Nunca por um valor que o domínio consegue produzir.

Cinco casos em 22/08, e é a mesma linha de raciocínio nas cinco:

| onde | valor legítimo | lido como |
|---|---|---|
| parser de bancada | pico `0 µs` | "não casou" → defeito **inexistente** |
| `voltaram 0` | zero retornos reais | "a placa não transmite" |
| `eco_proprio` | retorno, não envio | "mandou 4 de 8" |
| `mscope` sem sinal | canal calado | "a fonte não mandou" |
| `RDSR = 0x00` | **chip ocioso** | "não respondeu" |

O último é o mais claro: `if(!id)` tratava como ausência a resposta **mais
normal que um chip pode dar**. Trocado por impressão do valor cru, o mesmo
hardware distinguiu `00` (alguém puxa a linha) de `FF` (linha solta) — e essa
distinção apontou a causa seguinte. **O diagnóstico só existiu porque o
filtro saiu da frente.**

Cinco num dia é **forma estrutural**, não descuido em série. A tentação é
sempre a mesma: o valor "vazio" está ali, é conveniente, e `if(!x)` é mais
curto que carregar a validade junto.

**Ao escrever qualquer leitor, perguntar:** *este valor sentinela é
produzível pelo dado real?* Se for, ele não pode ser o sentinela.

**Corolário para relatórios: imprima o CRU, etiquete a interpretação ao
lado.** Nunca só a interpretação. Na mesma linha, o leitor pode discordar da
segunda sem perder a primeira.

## Contador que não se zera mede a HISTÓRIA, não a corrida

Todo máximo retido, soma acumulada ou contagem citada como resultado **desta**
corrida tem de ser **zerada por quem mede**, no início. Senão o número é de
outra corrida com cara de número desta.

*Custou três medições de prescalers diferentes imprimirem `23288 us` idêntico —
o pico da primeira. O verdadeiro era 3 811 µs, seis vezes menor. E repeti o
defeito uma hora depois, num comando novo que lia o mesmo contador sem o zerar:
a família não se cura por ter sido nomeada num sítio, cura-se em cada leitor
novo do contador.*

## Antes de publicar dois números, MULTIPLIQUE-OS

Custo por evento × taxa **tem de dar** a fração. Se não der, um dos dois está
errado — e descobre-se **de graça**, sem placa e sem serial, na própria frase
que se ia escrever.

*Entreguei `-3%` e `23288 us` na mesma linha. Vinte quadros por segundo a
23,3 ms são 47% do laço, não 3%. As duas metades da contradição estavam no meu
próprio relatório.*

Depois de consertado, os dois passaram a fechar: 20 × 3 811 µs = 4,05%, e o laço
mediu 4%. **É o fecho que prova a medição**, não a plausibilidade de cada número
sozinho.

## Antes de pedir que alguém OLHE, descreva o que a FALHA vai parecer

Se a descrição da falha e a descrição do sucesso são a mesma, **não há teste** —
há uma pergunta que só tem uma resposta possível.

*Pedi "cinza uniforme = limpo, faixas = corrompeu" para um xadrez de 1 px. Mas o
comando anterior deixava o ecrã **todo aceso**, e um xadrez de 1 px à distância
de trabalho também lê como branco. "Aterrou" e "não aterrou" davam a mesma
resposta ocular. Três respostas "limpo" viraram zero verificações.*

Corolário: **um teste tem de partir de um estado que não se confunda com o
resultado.** Teste que herda o ecrã, o buffer ou o contador de quem correu antes
está a medir o antecessor.

Isto é a **15at** virada contra o próprio autor: lá, o olho era incapaz de ver o
defeito; aqui, **fui eu que desenhei a verificação incapaz de falhar**.

## Diagnóstico só post-mortem não serve para depurar

Todo contador que vive apenas na caixa-preta precisa de **gémeo consultável a
qualquer momento**. É a regra dos alarmes de arranque, aplicada aos contadores
de evento.

*O detector de búzios escrevia nos slots 8/9/11 e em mais lado nenhum. Quando a
pergunta "dispararam?" ficou viva, a resposta só existia depois de um reset — e
eu não conseguia separar "não disparou" de "disparou e não chegou ao desenho"
de "desenhou e não se viu". Três contadores resolveram-no; a falta deles custou
uma volta inteira.*

## Antes de entregar um teste, verifique a PRÉ-CONDIÇÃO — não a feature

"Armado" é uma afirmação sobre a **cadeia inteira até ao sinal**, não sobre a
bandeira no fim dela.

*Liguei `buzios_on`, liguei o painel, li os notches de volta e disse "está
pronto, toca". Nunca verifiquei se o detector conseguia OUVIR: ele lê a soma das
cadeias pós-gate, e gravar zera todos os params. Entrada crua a −0,6 dBFS,
envelope do detector a −70 dBFS. Mandei tocar num instrumento surdo e chamei-lhe
pronto — e o dB reportou um defeito na animação que não existia.*

Par com a regra acima (falha≠sucesso): **um teste que não pode falhar e um teste
que não pode passar são o mesmo defeito** — não perguntei o que o teste era
capaz de mostrar antes de o entregar.

## Conserto adiado vira o EQUIPAMENTO da próxima emergência

Um instrumento torto é um defeito cosmético **enquanto não há crise**. Na crise
ele é a única coisa entre você e a resposta — e passa a ser um multiplicador.

*Medido em 02/09: o defeito real era uma linha (`GPIO_PULLUP` num pino de RX).
Entre "a placa trava" e essa linha passaram-se cinco horas, oito pedidos de
RESET e cinco gravações de conserto. **Nenhum dos defeitos encontrados pelo
caminho foi causado pelo defeito real** — eram nove instrumentos e ferramentas
com problemas antigos, cada um sozinho "não valia o desvio":*

- *contador de rearmes marcava 10 029 há semanas e ninguém perguntou*
- *o LED confundia fase 0 com fase 1, e lia duas voláteis em instantes
  diferentes — inventou um par que o código não produz*
- *a porta serial era escolhida por NOME: cinco comandos foram para a outra placa*
- *`bb arm` calava o log antes de imprimir a confirmação — armar e não armar
  tinham a mesma cara*
- ***o campo "o laço ficou preso?" era lido e ninguém o escrevia** — a pergunta
  central da sessão tinha campo e não tinha caneta*

Nenhum deles cresceu. Ficaram todos do mesmo tamanho. O que mudou foi que
passaram a ser usados.

**Assimetria que decide:** instrumento consertado em paz custa minutos, com a
placa a responder. Consertado dentro de uma prisão custa uma gravação, um RESET
humano, e a dúvida de saber se o conserto mexeu no que se estava a medir.

**Adiar não se decide, acontece.** Ninguém escolhe deixar um contador mentir —
deixa-se porque há outra coisa a fazer, e a sessão seguinte herda um instrumento
com menos um bocado de verdade.

**Contrapeso, senão isto vira desculpa para dispersão:** consertar agora não é
consertar tudo o que se vê. É consertar **o que a tarefa de agora está a usar**.
Numa caçada, isso é sempre o instrumento.

## Ao terminar

Perguntar: *"se esta instrumentação estivesse quebrada, o resultado seria
distinguível de um resultado real?"* Se não for, ela ainda não está pronta.
