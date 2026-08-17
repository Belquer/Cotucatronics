# Battle Scars — Cotucatronics
### O que o firmware nos ensinou enquanto a gente o ensinava a cantar
*Registro de aprendizados — resultados e metodologia. Atualizado ao fim da caçada do DelayMs (Búzios entregue).*

---

## 0. Por que este documento existe

O Cotucatronics saiu do zero até um instrumento com delay, vocoder espectral com portadora própria, stack anti-feedback completo e randomização por transiente ("Búzios") em poucas semanas de bancada. No caminho, cometemos e catalogamos dezenas de erros — e o padrão mais valioso que emergiu não foi nenhum truque de DSP: foi **uma disciplina forense de bancada**. Este documento registra as cicatrizes para que os próximos módulos, esculturas e colaboradores herdem as lições sem pagar o preço de novo.

Personagens: **dB** (design de instrumento, patches Max/RNBO, ouvido e decisões musicais), **Claude Code** (compilação, gravação, medição em hardware), **Claude chat** (arquitetura, hipóteses, prompts, auditoria externa via /gitsync). O triângulo funciona porque cada vértice pode vetar os outros dois com dados.

---

## 1. As três famílias de bug de ordem

Todos os bugs graves de firmware desta fase tinham **lógica correta e ordem errada**. Eles caem em três famílias, e distinguir a família importa porque o conserto difere.

**Família A — Corrida de contexto.** Dois contextos concorrentes (callback de áudio vs loop principal vs ISR), com a proteção armada num e a condição acontecendo no outro. Casos: despacho serial rodando na ISR (travava a placa no comando `list`); a janela de graça armada no loop enquanto o bloco caro rodava no callback (duas vezes). Conserto: **mover o armar para junto do checar — mesmo contexto, sempre.** Corolário: toda flag que cruza callback↔loop é `volatile` (ou atomic).

**Família B — Ordem de configuração.** Sem concorrência nenhuma; sequência errada dentro de uma função só. Engana mais justamente porque o código lê certo de cima pra baixo. Casos: `bind[]` preenchido antes do `Add()` dos params; `RequestBlockSize` depois do `Init()` dos módulos (módulos preparados pra bloco 48 recebendo 256 — estouro de heap silencioso rodando por baixo de medições que pareciam limpas). Conserto: **quem configura vem antes de quem consome.** Risco de confundir com a Família A: leva a "sincronizar" o que só precisava ser reordenado — resposta cara pra problema barato.

**Família C — Variável com dois empregos.** Uma coisa servindo a dois propósitos, e um teste (ou conserto) mexendo nos dois de uma vez. Casos: o `DelayMs(1)` era relógio do loop **e** metrônomo da rampa — removê-lo para testar o relógio destruiu a rampa junto, contaminando a conclusão auditiva do voo; o `@title`/classname default (`rnbomatic`) servindo a todos os exports até colidir. Conserto: **antes de mexer numa variável, listar todos os empregos dela.** No desenho experimental: um voo, uma variável — e conferir que a variável é mesmo uma.

---

## 2. A família da instrumentação que destrói o dado (7 casos)

A lição mais repetida do projeto, cometida inclusive **depois de nomeada**. Instrumentação é código de produção e falha como código de produção.

1. A janela de graça **zerava o medidor** e apagava o pico de estreia — o dado que ela existia para revelar.
2. A sonda de perfil **se sobrescrevia** a cada ativação: a medição barata apagava a cara.
3. O filtro `grep` da sessão de leitura **escondia a linha de erro** que explicava a falha.
4. Os contadores `static` **zeravam no boot da sessão de leitura** — medíamos a leitura, não o voo.
5. Os marcadores do HAL gravavam **fora do portão** `bb_rec` — o comando `bb` da autópsia contaminava a cena do crime.
6. O `BlackboxArm()` ligava **no meio do processamento de uma linha serial** — M1/M2 pareavam eventos de linhas diferentes e produziram um álibi falso (M1==M2 "provando" que nenhuma linha prendia).
7. As ~20 escritas/volta com `SCB_CleanDCache` viraram **suspeitas de criar o defeito que mediam** (inocentadas depois, mas o voo enxuto precisou existir para isso).

**Regras destiladas:** dado de evento único grava uma vez e **trava** (não atualiza). Estado vence contagem (o token de voo `in_flight` é imune ao problema do armamento; contadores pareados não são). A caixa-preta arma **por comando explícito no início do voo**, nunca no meio de um fluxo. A sessão de leitura não escreve nada. Escritas em backup SRAM em **lote** (acumular em DTCM, um flush por volta ou por evento, struct `alignas(32)`).

---

## 3. Medir antes de concluir — os casos que provam a regra

**O freeze condenado pelo número errado.** Primeira medição: 233.965 µs — "impossível". A separação estreia/regime revelou 3.520 µs de regime: faltava 2,1×, não 88×. Sem separar, teríamos cortado o patch com o número errado. **Estreia é o preço da mudança; regime é o aluguel. Nunca julgar aluguel pelo preço da mudança.**

**A estreia morava no hop, não no primeiro Process.** O warm-up desenhado por intuição (uma chamada de silêncio) não teria consertado nada — o bloco 1 era barato; o custo caía no bloco 2 (primeiro hop) e decaía em dois hops. A medição bloco-a-bloco corrigiu o desenho: warm-up cobre ~4096 amostras. **A prova de que funcionou não foi o tempo — foi a memória aparecer no lugar certo** (14 KB migrando pro `MemReport` do Init).

**Pico decide, média mente.** Vocoder: média 7,8%, pico 210%. Custo FFT concentra no bloco do hop (1 a cada N). O watchdog julga o pico. Corolário do bloco: dobrar o bloco dobra o prazo mas o pico sobe junto (~19%) — **dobrar o bloco não dobra a folga.**

**`-O3` piorou.** Binário +13%, média de CPU +15% — app executando da QSPI paga I-cache miss por byte de código. **Nesta plataforma, código menor é código mais rápido.** Regra registrada: ao reverter, remedir tudo (todos os números têm -O3 como base).

**Fixar parâmetro no Max não remove o DSP.** Cirurgia de "fixar valores": 12% menos código, **0% menos CPU**. Parâmetro congelado é alavanca colada, não motor desligado. Corte de custo exige **deletar objetos de sinal**. Rota preferida para módulo pesado: construir do piso e devolver luxo por luxo com etiqueta de preço — não desbastar patch rico.

**Frequência bater com suspeito não é identificação.** Duas fontes distintas produziam 1 kHz; a conclusão inverteu duas vezes até o teste que **remove** uma fonte separar as duas. Identificação exige manipulação, não coincidência.

**Suspeitar não é medir.** O estouro de heap (Família B) "provavelmente não distorcia o tempo de FFT" — e não distorcia. Mas só a remedição completa permitiu afirmar. Números publicados com defeito ativo por baixo foram **invalidados e remedidos**, mesmo os que estavam certos por sorte.

**Teste que passa merece a mesma desconfiança que teste que falha.** O bug do zsh (word-splitting) deixou um teste "passar por acaso" no pino 18; só o silêncio inesperado no pino 19 o expôs. A expectativa violada revela o que a expectativa confirmada esconde.

**RMS ≠ pico.** O medidor reportava RMS; senóide tem pico 3 dB acima. −1,1 dBFS RMS = ADC da interface clipando — e o governor pareceu errado por causa do instrumento de medição, não do firmware. Conhecer a unidade do próprio medidor é pré-requisito de qualquer conclusão.

**O assassino confirmado por protocolo.** `hal::DelayMs(1)`: quatro voos, mesma instrumentação, **uma variável** — 1.505 voltas/morte aos 12,6 s com ele; 8,7 M de voltas/vivo sem ele; religado, morte de novo; removido, vivo de novo. Discriminante e conserto no mesmo protocolo. É o padrão-ouro: quando a hipótese sobrevive à reintrodução do suspeito, ela vira fato.

---

## 4. Cultura de hipótese (como erramos melhor)

Hipóteses erradas com testes certos resolveram o projeto. O diagnóstico "cycle~ a 0 Hz por eventos não propagados" errou o mecanismo exato — mas o **teste de um comando** (`set voc_pitch 0.6`) que ele gerou apontou a região certa e destravou tudo. **Teste bom sobrevive a hipótese ruim.** Registrar apostas com percentuais ("60% relógio, 30% instrumentação, 10% resto") mantém o placar honesto e ensina a calibrar.

Derrubar a própria hipótese com dado é vitória, não derrota: o M1==M2 derrubou a hipótese da prisão no line_cb — e depois o furo do M1==M2 (armamento no meio da linha) a reabilitou — e depois o token a derrubou de vez. Três reviravoltas, todas por medição, nenhuma por teimosia.

Quando as hipóteses acabam, **pedir olhos novos com um relatório estruturado**: contexto, medições contraditórias, o que já foi descartado *com o dado que descartou*, e perguntas numeradas. Os melhores destravamentos da caçada vieram de relatórios assim.

---

## 5. Fronteiras de arquitetura (o que nos salvou repetidamente)

**HAL:** includes de fabricante só em `hal_<board>.cpp`. Trocar de cérebro = um arquivo + uma entrada de Makefile. Auditado em clone limpo múltiplas vezes; nunca violado.

**Módulos:** headers RNBO só em `module_rnbo_*.cpp`; o `main` fala com `Module`, nunca com motor. A fronteira fechou inclusive a porta dos fundos (`RnboAllocStatsGet` → `Module::Memory()`).

**Portão único da SDRAM:** `AllocBig()` é o único acesso; `DSY_SDRAM_BSS` proibido fora do HAL. O linker passou a **auto-aplicar** a regra (pool reivindica a região inteira; clandestino nem linka).

**Manifest = identidade do instrumento.** Params, macros, requisitos, bloco, cercas de sorteio, máscara de escala, mapa MIDI — tudo dado, nada código. Trocar escultura = trocar manifest + módulo. `Requirements` conferidos no boot: cérebro insuficiente recusa com log claro, nunca bug misterioso no palco.

**Get/Shaped/Real — três consumidores, três moedas.** Knob lê `Get` (norm cru), módulo lê `Shaped` (norm curvado), humano lê/escreve `Real` (unidades). O bug do push com `Get` (curva invisível pro DSP) e o do `set` normalizado (bench_boost "1.0" entregando 8×) nasceram de misturar moedas. A borda serial fala unidade humana; o miolo fala norm; a conversão vive na borda.

**Repo versiona o instrumento; bancada é estado transitório.** O default do repo é o que a placa É quando alguém clona e grava. Modo bancada vira *receita documentada*, nunca estado commitado.

**Clone limpo é o teste.** O `module_rnbo_alloc.h` nunca commitado ("funciona na minha máquina" por semanas) gerou o `make check` — que **na primeira execução** pegou outro defeito (export do freeze não rastreado). Hábito automatizado vence hábito lembrado; a prova é que a checagem manual recém-escrita não teria pego o segundo caso.

---

## 6. RNBO embarcado — o manual das cicatrizes

**Domínios evento vs sinal são a fonte nº 1 de bug de patch.** A régua: *olha a boca de quem come.* Freq de `cycle~` (nesta versão) = evento (param → expr → cycle~, direto). Ganho de `*~` = sinal (param → `sig~` → inlet). `delay~` (sinal) ≠ `delay` (evento). Operadores que alimentam entradas de sinal precisam ser variante `~`. Feedback fecha **só** com `feedback~` no ciclo. Na dúvida: autocomplete descreve, help patcher demonstra — copiar exemplo vivo venceu três vezes.

**Memórias:** `data @external 0` tranca como interna; `buffer~` é externo *por natureza* (o @external não pega). Receita oficial: **janelas matemáticas são do host** (firmware aloca via AllocBig, gera sqrt(hann), entrega via dataref, lendo o tamanho do próprio dataref); **memórias de estado são do patch** (`data @external 0`). Dataref se resolve **por nome**, nunca por índice (reexport reordena). `poke~`/`peek~` referenciando nome inexistente cria buffer externo fantasma no Max — funciona lá, sai mudo na placa.

**Custo:** todo `buffer~` custa **8 B/amostra** (Float64Buffer, ignora RNBO_USE_FLOAT32) + arredondamento pra potência de 2 — estimativa ingênua erra por até 4×. Patch grande mata por **tempo**, não memória (freeze: 2% da SDRAM, 275% do prazo). Custo FFT concentra no bloco do hop.

**Embarque (a receita canônica):** `RNBO.cpp` de desktop é inutilizável — incluir o export como fonte + `MinimalEngine`. Três macros antes dos includes: `RNBO_NOTHROW`, `RNBO_USE_FLOAT32`, `RNBO_USECUSTOMALLOCATOR`. Alocador híbrido (pool SRAM → AllocBig), free no-op, memória vitalícia. **O motor precisa ser dirigido:** `processEventsUntil()` + `advanceTime()` por bloco — sem isso, relógio preso em 0, nenhum evento executa, patch com portadora interna sai mudo. **Warm-up no Init** (política de slot, ~4096 amostras de silêncio) paga a estreia no boot. **Re-kick de params pós-warm-up** (reenviar todos os valores com o motor vivo) mata a classe "estado inicial que não propagou".

**Conferência pré-embarque do `description.json` é obrigatória e já pagou três vezes:** datarefs externos inesperados, params faltando (`voc_mix` sumido), saída mono onde o slot espera par, classname default. Ler o JSON custa um minuto; um vocoder mudo custou horas.

**Vocoder é peneira:** só devolve energia onde molde e portadora coincidem. Senóide × drone esparso ≈ silêncio (e "sem diferença perceptível" que não era bug). **Banda larga é a régua de vocoder; senóide é a régua de delay e shifter.** Cada efeito tem seu material de prova.

---

## 7. O loop principal sob carga (a caçada do DelayMs)

Com o vocoder a 65% do bloco, o mundo do loop principal muda de clima e três verdades ficaram gravadas:

**Serial USB é hostil sob carga.** As rajadas de ~3,4 ms do callback starvam a ISR de USB; o CDC do host apodrece; transmit pode não retornar. Hierarquia de canais promovida a arquitetura: **caixa-preta = registro de voo** (sempre); **serial = telemetria oportunista** (quando o universo colabora); **MIDI = controle** (imune). `Log()` com orçamento duro (descarta e conta, nunca espera); destino final: ring buffer kick-and-forget drenado por ISR — o loop nunca toca o USB.

**Detecção de host morto é responsabilidade nossa.** `dev_state`/TxState não detectam host que some sem avisar. Dead-man por timeout de conclusão + rearme por RX. E o gate protege as chamadas *seguintes* — não a que já entrou: por isso "3 falhas que retornam" não pega "1 chamada que não retorna".

**Rampa por tempo, não por volta.** Rampa que avança um passo por iteração amarra duração musical à saúde do loop (Família C: o DelayMs era o metrônomo dela). Desenho certo: `t = (NowMs − start)/dur`, lerp puro, sem estado incremental — qualquer volta reconstrói o valor do zero, loop degradado só espaça as avaliações, a rampa termina no prazo aconteça o que acontecer.

**Starvation ≠ prisão, e a célula que distingue:** contador de voltas *completas* no fim do while(1) + timestamp. Parado = prisão (e o token diz dentro de quê); lento = starvation (e a resposta é tirar trabalho do loop, não priorizar).

**Desfecho (2026-08-16, fechado).** Assassino nomeado e removido; feature entregue. A **ordem dos consertos importou tanto quanto os consertos**: a rampa teve que virar função do relógio **antes** de o `DelayMs` sair, senão remover o metrônomo teria quebrado a música ao consertar o loop. Foi o único momento da caçada em que consertar na ordem errada teria produzido um segundo bug enquanto apagava o primeiro.

A assinatura que apontou o culpado antes de qualquer medição confirmar foi a **forma da falha, não a magnitude**: o loop primeiro *degradava* (8 ms por volta em vez de 1) e só depois *parava de vez*. Cache degrada linear e não mata; relógio quebra de uma vez. E o mecanismo ficou provado pelo carimbo de tempo, escrito **imediatamente antes** do delay — o loop chegou lá aos 12 674 ms e nunca mais voltou: entrou e não saiu.

Commits: [`b9e5365`](https://github.com/Belquer/Cotucatronics/commit/b9e5365) (rampa por tempo) → [`bbf7f84`](https://github.com/Belquer/Cotucatronics/commit/bbf7f84) (DelayMs fora, com a tabela dos quatro voos) → [`7960f1d`](https://github.com/Belquer/Cotucatronics/commit/7960f1d) (Búzios, T13 aprovado, catálogo forense).

**Pendências herdadas da caçada**, que o documento deixa em aberto de propósito: o ring de log *kick-and-forget* (o loop nunca toca o USB) segue como refactor certo e não feito; a serial ainda morre sob carga, contornada por `log_mute` + caixa-preta + MIDI; e o estado do repo é de **bancada** (dinâmica neutralizada do A/B com o Max), esperando a decisão do dB sobre quanto de proteção volta e a que custo tímbrico.

---

## 8. Hardware — o que a medição decidiu

O piezo grande direto na entrada da Daisy preserva nível e grave — **opamp dispensado por medição** para este disco (receita do buffer arquivada para piezos menores/cabos longos). Proteção passiva (1k + 2×1N4148) recomendada antes do exciter: barata, invisível ao som, blinda o codec contra picos de impacto. O apito e o crackling eram **anel de terra do USB do computador** — somem na bateria, que já era o projeto; resíduo de 1 kHz a −90 dBFS é nosso (taxa de bloco) e inaudível. Pots são 3 fios + 100 nF no trilho analógico; botões, 2 fios + pull-up interno. **A física venceu a teoria duas vezes** (buffer, e a compensação de pitch que um simples 0.33→0.02 resolveu melhor que a fórmula) — o ouvido tem poder de veto sobre a elegância.

---

## 9. Búzios — o resultado que as cicatrizes compraram

Cada transiente na escultura é uma jogada; o que cai decide o timbre. `dly_time` sorteia em degraus de 50 ms **em unidade real** (a curva 2.5 entortaria sorteio uniforme em norm — mais uma vez, a moeda certa no lugar certo); `voc_pitch` sorteia nota a nota **sempre em Lá menor natural** (máscara de escala é dado de manifest; a base 110 Hz já é Lá, então a máscara nasce afinada); knob manual segue cromático — *a máquina se comporta, a mão pode tudo*. Cerca de segurança inegociável: level, gate, governor, limiter, routing e src_mix jamais sorteáveis. Aceite: **1 minuto de taps, 74 transientes, 74 sorteios — razão 1:1, julgada pelo músico.** O critério final de qualquer feature é o ouvido de quem toca; caixa-preta prova encanamento, não flor.

---

## 10. Checklist de retomada a frio (colar no início de qualquer sessão futura)

Ler o STATUS/este documento antes de tocar em código. `/gitsync` se a sessão for de arquitetura. Um voo, uma variável — e listar os empregos da variável antes de mexer. Caixa-preta armada por comando no início do voo; sessão de leitura não escreve. Toda medição declara unidade (RMS? pico? norm? real?) e regime (estreia? permanente? que bloco? que clock? -O2 ou -O3?). Módulo novo: description.json → embarque pela receita → medir estreia+regime nos dois blocos → teste audível com o material certo pro efeito. Bug novo: nomear a família antes de consertar. Hipótese nova: registrar a aposta com percentual. Feature nova: o aceite é do músico.

---

*"Sem medir antes de concluir, o freeze teria sido condenado pelo número errado, o vocoder descartado pelos 234 ms de estreia, e o warm-up desenhado como uma chamada só — que não resolveria nada."* — o parágrafo que resume tudo, escrito pelo Code no fechamento da fase 1 e provado verdadeiro em cada capítulo desde então.
