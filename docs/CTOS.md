# CTOS — Cotucatronics Operational System

**Documento de desenho v0.4.0 · 2026-08-20 · design antes de código**

> **Status: PROPOSTA.** Nada aqui está implementado. O documento existe pra
> ser discutido e aprovado antes de qualquer refactor — e pra que a discussão
> aconteça sobre a fronteira, não sobre o código que já foi escrito.

---

## 0. Por que existe

Um efeito derrubou o loop principal. O instrumento continuou tocando — o
áudio roda por interrupção — mas a placa ficou surda a **tudo**: serial,
MIDI, comandos, parâmetros. Só um LED piscando direto do loop conseguiu
relatar a morte, porque todos os outros instrumentos de diagnóstico passavam
pelo loop que tinha morrido.

**A pergunta que isso levanta não é "qual efeito tem o bug".** É: *por que um
efeito consegue derrubar a base?*

Num sistema compartimentado isso seria estruturalmente impossível. O efeito
poderia falhar de todas as formas — travar, estourar o prazo, ler fora dos
limites — e o instrumento perderia **o efeito**, nunca **a base**.

### Ressalva de honestidade, e ela importa

O freeze é o **suspeito principal** da morte do loop, e **não está
confirmado**. O quadro, depois de o cabo ser identificado e estabilizado:

| Build | Freeze | Loop |
|---|---|---|
| v0.46.2 | **off** | ✅ vivo |
| v0.47.0 | on | ❌ morto |
| v0.48.0 | on | ❌ morto |
| v0.48.1 | on + log mudo | ❌ morto |

Três mortes com freeze, uma vida sem ele — consistente, e o log já foi
inocentado no caminho. **O que falta é uma única gravação:** um build com o
freeze fora, com o cabo confirmado. A v0.47.1 era exatamente isso e a
gravação expirou. (Houve um freeze-off que morreu, v0.46.1, mas com o cabo
ainda se movendo — não conta.)

**Isso não enfraquece o CTOS — reforça.** O argumento não é *"o freeze é
culpado, então compartimente"*. É:

> Passamos horas sem conseguir dizer **qual metade do sistema** estava
> quebrada, porque não existe fronteira que separe as metades. Um sistema em
> que a pergunta *"foi a base ou foi o efeito?"* não tem resposta observável
> é um sistema sem compartimentos — e a dificuldade do diagnóstico É o
> sintoma.

O CTOS se justifica pela **capacidade de responder essa pergunta**, não pela
culpa de um efeito específico.

---

## 1. Divisão de contexto como CONTRATO, não convenção

Duas metades, dois contextos de execução, e a fronteira é lei:

| | **CTOS** | **Efeito** |
|---|---|---|
| Onde roda | loop principal | **só** callback de áudio |
| Interface | comandos, params, estado | `buffer → buffer` |
| Pode bloquear? | sim (é o dono do tempo ocioso) | **nunca** |
| Pode alocar? | sim, no boot | **só no Init** |
| Pode logar? | sim | **nunca** |

**As duas regras duras:**

1. **Nenhum código de efeito executa no loop principal.**
2. **Nenhum código de CTOS executa no callback de áudio.**

> **Convenção é o que a gente lembra; contrato é o que o build cobra.** Todas
> as regras deste projeto que dependiam de lembrar já foram violadas pelo
> menos uma vez — inclusive por quem as escreveu, no mesmo dia.

### 1.1 Como auditar no `make check`

Cada efeito compila pro seu próprio objeto. A fronteira vira verificável por
**símbolos importados**:

```bash
# um efeito só pode importar da libc/libm. Nada de hal, params, log.
arm-none-eabi-nm -u build/effect_*.o | grep -vE '<allowlist>' && FALHA
```

**Allowlist do efeito:** `memcpy`, `memset`, funções de `<cmath>`, e nada
mais. Qualquer referência a `hal::`, `params`, `Log`, `Blackbox` reprova o
build.

**No sentido inverso:** o CTOS não pode chamar `Process()` de efeito fora do
callback. Isso é mais difícil de provar por símbolo; a proposta é estrutural
— o ponteiro do efeito só é visível de dentro do módulo que contém o
callback, então chamá-lo de fora **não compila**.

**Limite honesto:** a checagem por símbolos pega dependência, não
comportamento. Um efeito que faça `while(1)` sem importar nada passa na
auditoria. É o watchdog (§3) que cobre isso — as duas defesas são
complementares e nenhuma sozinha basta.

---

## 2. Contrato de recursos do efeito

Um efeito, para ser embarcável:

- [ ] **Aloca só no `Init`**, e só via `hal::AllocBig()`. Zero alocação em
      runtime.
- [ ] **Nunca bloqueia, nunca espera, nunca dorme.** Sem `Delay`, sem
      retry, sem laço de espera.
- [ ] **Nenhum laço de duração não-limitada.** Todo `while` no caminho de
      áudio precisa de teto provável por construção.
- [ ] **Todo índice de buffer é limitado ANTES do acesso**, não confiando em
      invariante de quem chama.
- [ ] **Todo estado cross-context é `volatile`** e escrito por um lado só.
- [ ] **Não loga, não mede tempo, não fala com o mundo.** Quem observa é o
      CTOS, de fora.

### 2.1 Caso de estudo: o freeze v0.1.0/v0.2.0

O que dá pra **verificar lendo o código**, sem atribuir a ele a morte do loop:

**Alocação:** ✅ conforme. Aloca uma vez, no `Init`, via `AllocBig`.

**Laços sem teto:** ❌ **viola.**
```cpp
float WrapF(float x) const {
    while(x < 0.f) x += n;      // n pequeno + x grande = muitas voltas
    while(x >= n)  x -= n;
}
```
Duas voltas em uso normal. Mas o teto depende de `x` e `n` serem sãos — e
**dentro do callback de áudio, um laço cujo teto depende de dados é um risco
estrutural**, não um detalhe. `fmod` ou aritmética direta não têm esse
problema.

**Índice não limitado:** ❌ **viola.**
```cpp
const size_t i0 = (size_t)p;   // se p sair da faixa, le fora do buffer
return b[i0] + f * (b[i1] - b[i0]);
```
`i1` é protegido contra passar do fim; `i0` **não**. Confia que `p` está na
faixa. Num buffer de 768 KB em SDRAM, um `p` inválido lê memória alheia sem
erro nenhum.

**Estado cross-context:** ✅ não se aplica. Todo o estado do freeze é escrito
e lido dentro do callback — não há travessia de contexto. *(A afirmação
inicial de que ele violava as três não se sustenta na leitura do código; ele
viola duas, e as duas são reais.)*

**A lição do caso:** as duas violações são do tipo que **não aparece em
teste** — funcionam sempre, até o dia em que os dados saem da faixa. Nenhuma
seria pega por medição de CPU, escuta, ou pelo IFÁ10000 como ele está hoje.
Só um contrato explícito as pega, e só antes de embarcar.

---

## 3. Watchdog com veto automático

O CTOS mede o tempo do efeito **por bloco**. Estouro do orçamento:

1. o efeito entra em **bypass automático**, no mesmo contexto que detectou;
2. a razão fica registrada e consultável;
3. **o instrumento continua tocando** — perde o efeito, mantém a base.

```
> ctos
CTOS v0.1.0 | loop vivo | fase 6 | uptime 412 s
  efeito fx2 "freeze": BYPASS por overrun
    estourou em t=3.2 s | pico 1180 us de 1000 us | 1 ocorrencia
  bypass e PERMANENTE ate `ctos rearm` — efeito que estourou uma vez
  estoura de novo, e religar sozinho esconderia o defeito
```

**Isto ressuscita por desenho o bypass do fx2**, que existiu e foi revogado
quando o vocoder passou a nascer ligado. A diferença é de natureza: antes era
**proteção pontual**, decidida caso a caso e revogável por conveniência;
agora é **lei estrutural do CTOS**, e a condição de retorno já estava escrita
no IFÁ10000 — *módulo não-certificado entra em bypass até passar pelo
protocolo*.

**DECIDIDO pelo dB (18/08): bypass PERMANENTE até `ctos rearm`.** Religar
sozinho transformaria um defeito reprodutível em intermitente — e
intermitente foi o que custou esta sessão inteira. Um efeito que estourou uma
vez estoura de novo; o valor do bypass está justamente em ele **não** se
desfazer sem alguém olhar.

---

## 3b. Teto de soma — o irmão do watchdog, não o substituto

O §3 protege contra **um efeito com defeito**. Não protege contra **N efeitos
saudáveis que juntos não cabem**, e essa é uma situação de natureza diferente
que exige um mecanismo diferente.

### A distinção que decide tudo

| | Watchdog por-efeito (§3) | Teto de soma (§3b) |
|---|---|---|
| **O que pega** | um efeito que sozinho estoura seu orçamento | N efeitos saudáveis cuja soma não cabe no prazo |
| **Há culpado?** | **sim** — o efeito tem defeito individual | **não** — é excesso de ambição, ninguém está errado |
| **Pergunta certa** | "quem estourou?" | "qual o instrumento perde com menos dano?" |
| **Base da decisão** | medição do próprio efeito | **prioridade declarada a frio pelo dB** |
| **Justiça** | o veto é justo porque há culpa | não existe noção de culpa aqui |

**É aqui que o erro de punir o inocente nasceria.** Qualquer regra que tente
achar "quem foi" quando não há quem, acaba inventando um culpado — e o
inventado será algum efeito perfeitamente correto. A saída não é uma detecção
melhor: é **abandonar a pergunta**. O teto de soma não procura culpa porque
não há culpa a procurar.

### Por que não LIFO

Derrubar o último ligado é aproximação, e uma aproximação ruim: pune por
**ordem de chegada**, que não tem relação nenhuma com valor musical. O último
efeito ligado pode ser exatamente aquele que o dB estava procurando quando a
soma estourou — o gesto mais recente é, se alguma coisa, o mais intencional.

**Prioridade declarada é a versão correta.** A escolha é musical, e decisão
musical se toma **a frio**, no manifest, fora do calor do overrun. O sistema,
no instante da falha, não escolhe nada: lê a ordem e obedece.

### A política

1. **Ordem de sacrifício no manifest** (`Manifest::sacrif[]`). A ordem do
   array **é** a prioridade — índice 0 cai primeiro. Não há número de
   prioridade escondido em lugar nenhum; reordenar o array é a única forma de
   mudar a política, e isso é deliberado: a política tem que ser legível de
   uma olhada.

2. **Mede o BLOCO INTEIRO**, não a soma de estimativas. Estimativa some
   justamente no caso que importa — interação entre efeitos, cache, estreia. O
   cronômetro do bloco é o único número que corresponde ao que de fato
   acontece. Subtrai em **ticks** e converte depois: dividir antes faz a volta
   do contador deixar de se corrigir sozinha (a armadilha que já derrubou o
   watchdog em 4 277 071 629 µs).

3. **Teto em 85% do prazo, e só depois de 4 blocos SEGUIDOS acima.** O
   primeiro bloco de qualquer efeito é caro — é a estreia, e o projeto já tem
   a distinção estreia/regime documentada. Derrubar por causa dela puniria o
   ato de **ligar**. Só o regime conta. O contador volta devagar (decrementa,
   não zera) para que uma alternância não mascare um estouro sustentado.

4. **Cai o de menor prioridade que estiver LIGADO.** Bypass pelo mesmo
   caminho que o dB usaria: o param de liga/desliga vai a zero. O latch é
   reaplicado **todo bloco**, não só no instante da queda — sem isso um CC do
   Loopy religaria o efeito e o teto teria virado sugestão.

5. **O aviso é obrigatório.** Log alto + CC 119 na saída MIDI (valor =
   índice+1 do efeito derrubado). Derrubar em silêncio seria a família "falha
   em silêncio" de novo, e desta vez com o instrumento **mudando de som
   sozinho** — o pior caso possível dessa família.

   **O aviso sai do LOOP, nunca do callback.** A primeira versão logava de
   dentro do `AudioCb` e matou a serial no primeiro teste: escrever no CDC de
   dentro do callback starva a ISR do USB, que é o defeito de prioridade 1
   deste projeto e estava catalogado antes de eu cometê-lo. O callback só
   marca (`volatile`); o loop lê a marca e fala. Família A dos bugs de ordem:
   quem arma e quem executa no mesmo contexto — e o contexto que pode falar
   com o mundo é o loop.

6. **Permanente até `ctos rearm` ou boot**, pela política já fechada no §3.
   `ctos rearm` solta os latches mas **não religa os efeitos**: soltar o latch
   devolve o *direito* de ligar; ligar continua sendo gesto do dB. Religar
   automaticamente decidiria por ele qual configuração ele queria de volta.

7. **Estourar sem ter o que derrubar é um estado próprio**, e se declara. Se
   todos os sacrificáveis já caíram e a soma ainda não cabe, a base sozinha
   não cabe — e isso não se resolve por bypass. O silêncio do mecanismo nesse
   caso seria lido como sucesso dele.

### Como testar sem esperar o limite chegar em cena

```
ctos teto <us>     # teto artificial, força o estouro sem carregar a CPU
ctos teto 0        # volta ao automático
ctos               # mostra bloco, pico, teto, quedas e a ordem inteira
```

Descobrir se o mecanismo funciona no instante em que o instrumento encontra o
limite é descobrir tarde demais.

### Custo

O mecanismo custou **+14 µs** por bloco (dentro do ruído entre corridas; o
pico não mudou). São duas leituras de tick e uma varredura de seis
comparações. Já se cronometrava por efeito; somar é de graça.

---

## 4. Diagnóstico ortogonal de primeira classe

**O princípio, comprado caro:** *um instrumento de diagnóstico não pode
depender do que ele diagnostica.* Serial, MIDI e caixa-preta passam todos
pelo loop principal — quando ele morreu, **nenhum dos três conseguiu
relatar**. A primeira versão do pulso no LED tinha o mesmo defeito: era o
próprio loop que piscava, então só sabia dizer "morreu", nunca "onde".

**Vira permanente no CTOS:**

- **O loop sempre pisca.** "Loop vivo?" nunca mais depende de serial.
- **O padrão codifica a última fase completada** — N piscos = fase N, pausa
  longa entre rajadas. É a lição da caçada do `DelayMs`, agora estrutural em
  vez de instrumentação temporária.
- **Quem pisca é o callback de áudio**, lendo um `volatile` que o loop
  publica. O observador vive no contexto que sobrevive.

| Padrão | Significa |
|---|---|
| N piscos, repetindo | loop vivo, completando até a fase N |
| aceso fixo / apagado fixo | callback vivo, **loop morto** |
| nada, e sem som | placa morta ou sem alimentação |

**Extensão natural, se um segundo indicador existir:** um LED por contexto —
um pro CTOS, um pro efeito. Aí a pergunta *"qual metade quebrou?"* passa a
ter resposta **visual e imediata**, sem ferramenta nenhuma.

---

## 5. Mapa: o que mora onde

Explícito, pra que a próxima adição não precise adivinhar.

### CTOS — loop principal

| Responsabilidade | Por quê |
|---|---|
| WIDI / MIDI (`PollMidi`) | despacho precisa de contexto que possa bloquear |
| Serial (`PollSerial`, comandos) | idem |
| Tabela de parâmetros | dono único da verdade; efeito só lê |
| Watchdog e bypass | quem julga não pode ser julgado no mesmo contexto |
| Caixa-preta | registro é do sistema, não do efeito |
| Estado de boot, manifest, defaults | identidade da escultura |
| Roteamento e crossfade de fonte | decide **o que** entra no efeito |
| **Dinâmica de saída**: gate, governor, limiter | segurança do instrumento |
| **Reverb** | moldura permanente, não caráter trocável |
| Búzios (sorteio) | mexe em params — é ato de CTOS, não de DSP |
| Pulso de vida e fase | diagnóstico é infraestrutura |

### Efeito — só callback de áudio

| Responsabilidade | Por quê |
|---|---|
| Miolo DSP trocável | é o que muda de escultura pra escultura |
| Buffer próprio, alocado no `Init` | recurso declarado, nunca pedido em voo |
| Leitura de params (só leitura) | o efeito obedece, não decide |

**A regra de bolso pra classificar coisa nova:** *se responde a comando, mexe
em params, ou serve de referencial fixo do instrumento, é CTOS. Se transforma
amostra em amostra e pode ser trocado sem mudar a identidade da escultura, é
Efeito.*

**Nota sobre o freeze:** por esta régua ele é **Efeito** (transforma amostras,
é trocável) — mas o `frz_on` é **CTOS**, porque é gesto de execução ligado a
botão e CC. A fronteira passa entre o gesto e o processamento, e é por isso
que ela precisa estar desenhada antes: sem o mapa, "freeze" parece uma coisa
só.

---

## 5b. Suíte de efeitos residentes v1 — desenho (PROPOSTA)

Cinco efeitos em C++, todos **Efeito** pela régua do §5 (transformam amostra
em amostra, trocáveis sem mudar a identidade da escultura). Calibrados pro
material do dB — percussão, kalimba, monocórdio, molas — e pra estética
Chase Bliss / dreamy.

**Ordem fixa de pedaleira**, e ela é decisão musical, não de implementação:

```
drive → phaser → warble → detune → [delay] → shimmer → [reverb]
```

Drive antes de tudo porque cola e dá corpo ao que entra. Shimmer depois do
delay e antes do reverb porque ele **é** um reverb com realimentação afinada:
pô-lo antes do delay faria o delay repetir a cauda dele.

### Motor de pitch — UM POR PONTO DE DERIVAÇÃO

> ⚠️ **CORREÇÃO (18/08):** a primeira versão deste documento dizia "um motor
> só, consumido pelos três". **Está errado, e o erro só apareceu ao escrever
> o laço do shimmer.**

Um motor tem **um buffer com um ponto de escrita**. Ele só pode servir a
consumidores que deslocam a **mesma fonte** — e os cinco efeitos não tapam no
mesmo lugar:

```
seco → [detune, warble tapam AQUI] → drive/phaser → delay → reverb ⇄ [shimmer tapa AQUI]
```

O shimmer precisa gravar a **saída do reverb**, porque é a cauda que sobe de
oitava e realimenta. Detune e warble precisam do **seco**, antes de tudo.

**Decisão do dB: um motor por ponto de derivação.** A economia do
compartilhamento continua real — apenas é **por ponto**, não global:

| Motor | Tapa | Consumidores |
|---|---|---|
| `pitch_dry` | seco, pré-efeitos | detune (2 vozes), warble (1) |
| `pitch_tail` | saída do reverb | shimmer (1 ou 2 vozes: +12 e +7) |

Custo: 150 KB de SDRAM cada, acesso sequencial. Com 64 MB disponíveis, o
gargalo nunca foi memória — era o pool de SRAM dos módulos RNBO, e nenhum
destes usa aquele pool.

**A lição do erro:** *"compartilhado" é uma afirmação sobre a FONTE, não sobre
o recurso.* Dois efeitos só compartilham um motor de histórico se ouvirem o
mesmo ponto da cadeia. Eu escrevi o desenho assumindo que sim, sem verificar
de onde cada um puxa — e a verificação custava uma linha por efeito.

> ⚠️ **CORREÇÃO DE PREMISSA: o freeze NÃO tem pitch-shifter.** Ele tem a
> *maquinaria* granular — janela raiz-de-Hann, posições sorteadas, leitura
> interpolada — mas lê a **taxa 1.0** (`pos_ += 1.f`). Altura é taxa ≠ 1.
>
> Então o motor compartilhado é uma **extensão** da estrutura do freeze, não
> reuso de capacidade existente. O trabalho é pequeno porque a estrutura está
> provada e certificada, mas não é zero — e planejar como se fosse zero é
> como o `r_window` que ninguém conferiu.

**O que o motor herda do freeze, e é o que o torna barato:** grãos com
raiz-de-Hann somam potência constante para posições descorrelacionadas, e o
sorteio de posição **mata o comb filtering** que um pitch-shifter de janela
fixa produz. Os dois já foram medidos e aprovados de ouvido.

### Custo — o que a aposta do dB encontra pela frente

Aposta: **< 15% do bloco 48 com os cinco ligados.** Os números que já temos:

| Peça | Medido |
|---|---|
| Freeverb (110 KB em SRAM) | **+5,6 pp** de CPU |
| Freeze: 2 grãos, estéreo, interpolado | **98 µs** de 1000 (≈10%) |

**A aposta é alcançável, mas depende de uma escolha de desenho do shimmer:**

- **Shimmer com Freeverb próprio:** +110 KB de SRAM e +5,6 pp, mais o pitch
  (~10%) ⇒ **~15% só ele**. A aposta não fecha.
- **Shimmer realimentando pelo reverb EXISTENTE:** custo = só o pitch e a
  soma ⇒ **~10%**, e zero memória nova. Mais fiel ao original de Eno/Lanois,
  onde o shimmer *é* a sala realimentada.

**Recomendação: o segundo.** E ele traz um risco declarado: o laço de
realimentação passa a atravessar o reverb do instrumento, então `rev_mix` e
`rev_size` afetam a cauda do shimmer. Isso é característica, não defeito —
mas precisa estar no `frz`-style comentário e ser dito ao músico, porque um
controle que mexe em dois lugares surpreende.

Os outros três são baratos por construção: **drive** é função por amostra
(zero memória), **phaser** é só estado de allpass (zero buffer), **warble** é
um delay de ~20 ms (≈1 KB). Três dos cinco não pesam.

### Cada efeito passa pelo IFÁ10000 completo

O freeze é o gabarito — e o que ele custou vale como aviso: **três versões**
até passar na etapa 9, e as três falhas foram de desenho, não de código
(varredura linear virando laço; janela assumindo correlação depois de o
sorteio tirá-la; mix como crossfade apagando o instrumento vivo).

**Medir individual E com todos ligados**, no bloco 48. O watchdog por slot já
cobre o estouro isolado; o que ele **não** cobre é a soma — cinco efeitos
dentro do orçamento sozinhos podem estourar juntos, e nenhum seria vetado
porque nenhum passou do próprio teto.

> **Item de projeto que isso levanta:** o CTOS precisa de um **teto de soma**,
> não só de tetos por efeito. Fica registrado aqui como pendência do §3.

### Ordem de trabalho

1. **Motor de pitch compartilhado** (extensão do freeze).
2. **Shimmer** — primeiro efeito, valida o modelo inteiro: motor
   compartilhado, slot, watchdog, IFÁ10000.
3. Os outros quatro, na ordem de custo crescente: drive, phaser, warble,
   detune.

---

## 6. Ordem do refactor, quando aprovado

1. **`ctos` como comando** — expõe loop vivo, fase, uptime, efeitos em
   bypass. É o primeiro porque é o que torna o resto observável.
2. **Pulso no LED permanente** com codificação de fase *(já existe em
   v0.49.0 como diagnóstico; vira estrutura)*.
3. **Watchdog com veto** e bypass permanente.
4. **Migrar o que está do lado errado da linha**, começando pelo que o freeze
   pôs no caminho de áudio: `WrapF` sem teto e `Read` sem limite.
5. **Checagem de fronteira no `make check`**, por símbolos importados.
6. **Cláusula de fronteira no IFÁ10000** — etapa nova, ou item da etapa 1
   (CONTRATO): *"este efeito respeita a divisão de contexto do CTOS?"*

**O loop morto atual se resolve no caminho.** Ele não é um obstáculo ao
refactor — é a prova viva de que a fronteira precisa existir, e o primeiro
caso de teste dela.

---

## 7. O que este documento NÃO resolve

- **Não sabemos ainda qual metade quebrou** — a v0.49.0 (LED por fase) é
  quem vai dizer, e ela ainda não foi lida.
- **Não substitui o ring de log kick-and-forget.** Compartimentar impede o
  efeito de matar o loop; não impede o loop de se afogar no USB sozinho.
- **A checagem por símbolos não pega comportamento.** Watchdog cobre; nenhum
  dos dois sozinho basta.

---

*"O instrumento perde o efeito, nunca a base."*
