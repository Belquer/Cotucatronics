# SuMiA™ — supressor de microfonias automatizado

> **CONSTRUÍDO E A CORRER** desde 2026-09-02 (`main.cpp`, `PassoDoSupressor`).
> Este documento nasceu como o *contrato*, escrito antes do código, e ficou
> a dizer "ainda não construído" durante dois dias depois de o código
> existir — apanhado no deep clean de 2026-09-04.
>
> **O que ele já fez em serviço:** a 2026-09-04, na escultura real e não no banco
> de ensaio, encheu as quatro bocas dos dois ramos sozinho — 3025,7 · 4523,4
> · 790,9 · 5434,5 Hz no L e semelhantes no R. É a primeira vez que segura
> achados próprios fora da bancada.
>
> ⚠️ **Ligado por omissão** (`micro_auto` nasce em 1). Durante uma sessão de
> escuta deve ser **desligado** — um caçador que muda um notch a meio do
> julgamento faz atribuir ao efeito uma mudança que foi dele.

O contrato original, medido antes do código, segue abaixo — vale como
desenho e como registo do que se decidiu e porquê.

## O que ele é, e o que não é

**Não é um ritual disparado por gesto.** Foi essa a primeira ideia (três
toques no encoder) e o dB substituiu-a por uma melhor: o supressor **corre
sozinho no arranque e sempre que os potenciómetros se mexem**.

Isso obriga-o a ser **silencioso**. Um ritual pode subir o ganho até o laço
nascer; uma coisa que corre a cada mexida de pote, não. Ele **escuta o que
já está a acontecer** — quando um pico domina a banda, corta-o; quando o
modo desaparece, recua devagar.

**É para o caminho do AMP, não para o jack.** Sem exciter não há laço.

## Os três gatilhos

| gatilho | porquê |
|---|---|
| **arranque** | os params nascem zerados e o corpo é o que é |
| **mexida de qualquer pote** | o ganho do laço escala com o volume: um ajuste feito a um volume **não vale a outro** |
| (contínuo, em cadência lenta) | os modos derivam com a temperatura |

## ⚠️ QUALQUER pote rearma OS DOIS caçadores

**Nunca "pote L rearma o supressor L".** Os exciters estão no mesmo corpo —
no caso medido, o tambor está **soldado** ao botijão, é uma peça de metal
só. Subir o volume de um exciter muda o ganho do laço que o **outro** piezo
vê.

Medido em 2026-09-02, e é a prova: com a matriz separada (`route_b_l=0`,
`route_a_r=0`), os dois ramos reportaram **3275,7 e 3275,8 Hz** — a mesma
frequência às décimas. A matriz separa os caminhos **eléctricos**; não
separa o **metal**.

Corolário para a calibração: **isolar a matriz não isola os corpos.** Não se
pode assumir que L e R encontram modos diferentes — encontram os mesmos, com
forças diferentes.

## O que a sessão de escuta mediu, e que o supressor tem de reproduzir

**Cada modo que morre entrega o lugar ao seguinte.** Na mesma sessão:
1536 Hz → 3276 Hz → 535 Hz, e a segunda boca acabou-se ao terceiro. Foi essa
cascata que motivou **quatro bocas por ramo** (custo medido: ~0,5% de CPU
por boca; oito no total custam ~4%, com pico em 42,7% e margem até 80% — não
é a CPU que limita).

**O laço cruzado da matriz vale 49 dB.** Com as quatro células abertas:
1536,4 Hz a **+24,4 dB**, dominância **0,99**. Com `A→L` e `B→R` apenas:
1543,1 Hz a **−24,3 dB**, dominância **0,27**. O `route` deixou de ser
detalhe de encaminhamento e passou a ser controlo de primeira ordem sobre a
microfonia.

**A dominância é o que separa uivo de nota**, e o limiar sai de medições e
não da teoria: microfonia real mediu **0,97 a 1,00** quando estava a gritar,
e **0,14 a 0,29** depois de cortada. Uma calibração anterior em senóides
sintéticas dera 0,85, que teria recusado casos reais de 0,61–0,68.

## As três decisões, tomadas pelo dB em 2026-09-02

### 1. As bocas não vivem em preset nenhum

**Efémeras.** Cada vez que a Daisy liga, o supressor entra em acção, suprime,
e fica quieto. Nada persiste.

Isso resolve por construção o problema que estava aberto: notch em preset
global **viajaria entre unidades**, e as peças são replicadas em série com
modos próprios de cada soldadura — uma frequência certa numa peça é ruído
noutra. Não persistindo, não viaja.

E limita a acumulação a **uma sessão** e a **quatro bocas por ramo**.

### 2. O rearme é o pote, e vem do gesto humano

> *"a ação do usuário sempre que há microfonia, a ação instintiva é mexer no
> volume, então isso deve resolver"*

O gatilho não é uma cadência arbitrária: é o **gesto que a pessoa já faz**.
Quem ouve microfonia mexe no volume — e é exactamente aí que o ganho do laço
muda e a decisão antiga fica inválida.

⚠️ E **qualquer pote rearma OS DOIS caçadores** — ver a secção acima.

### 3. NÃO HÁ RECUO, e a razão é que o notch esconde a própria prova

Depois de colocado, o modo está quieto **por causa da boca**. Olhar o
espectro para decidir se ela ainda faz falta é olhar para o próprio
conserto. Para saber a verdade seria preciso **baixar a boca e ouvir se o
uivo volta** — deixar a pessoa apanhar com aquilo, de propósito, em ciclo.

E as contas são assimétricas:

| | custo |
|---|---|
| uma boca a mais que já não faz falta | ~20 Hz a Q 35, 15 dB — **inaudível** |
| um recuo errado | **o uivo volta, com o ganho que o pôs lá** |

Com as bocas efémeras e limitadas a quatro, a acumulação não degenera.
**O recuo é o botão de desligar; o rearme é o pote.**

### 4. Arbitragem: ganha sempre a mais AGUDA

> *"no caso de mais de 4 picos, sempre suprimir as mais agudas porque são as
> mais dolorosas"*

Com as quatro bocas cheias e um modo novo:
- se o novo for **mais agudo** que a boca mais grave em uso → **despeja a
  mais grave** e fica com ele;
- se for **mais grave** que todas as quatro → **não entra**.

O critério é a **dor**, não a força. Fica registado o caso de fronteira que
isto deixa em aberto: um modo **grave e muito forte** pode ficar de fora
enquanto quatro agudos fracos ocupam as bocas. Se isso aparecer na escuta,
a regra ganha um desempate por nível — mas não se inventa antes de doer.

## O que falta decidir

- **O limiar de dominância** a que ele decide "isto é uivo". As medições
  desta sessão dão a faixa (0,97–1,00 a gritar; 0,14–0,29 depois de cortado),
  mas o valor final sai do ouvido do dB e não do meio da faixa.
- **A cadência.** De quanto em quanto tempo ele analisa, e quanto tempo um
  pico tem de persistir antes de merecer uma boca.
- **A profundidade inicial.** 15 dB resolveu tudo o que apareceu hoje à mão;
  se o automático deve começar mais raso e aprofundar, ou entrar já a 15.
