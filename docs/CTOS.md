# CTOS — Cotucatronics Operational System

**Documento de desenho v0.1.0 · 2026-08-18 · design antes de código**

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
