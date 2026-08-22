# Battle Scars — Cotucatronics
### O que o firmware nos ensinou enquanto a gente o ensinava a cantar
*Registro de aprendizados — resultados e metodologia. Atualizado ao fim da caçada do shimmer ardido e dos dois controles de I/O.*

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

## 2. A família da instrumentação que destrói o dado (8 casos)

A lição mais repetida do projeto, cometida inclusive **depois de nomeada**. Instrumentação é código de produção e falha como código de produção.

1. A janela de graça **zerava o medidor** e apagava o pico de estreia — o dado que ela existia para revelar.
2. A sonda de perfil **se sobrescrevia** a cada ativação: a medição barata apagava a cara.
3. O filtro `grep` da sessão de leitura **escondia a linha de erro** que explicava a falha.
4. Os contadores `static` **zeravam no boot da sessão de leitura** — medíamos a leitura, não o voo.
5. Os marcadores do HAL gravavam **fora do portão** `bb_rec` — o comando `bb` da autópsia contaminava a cena do crime.
6. O `BlackboxArm()` ligava **no meio do processamento de uma linha serial** — M1/M2 pareavam eventos de linhas diferentes e produziram um álibi falso (M1==M2 "provando" que nenhuma linha prendia).
7. As ~20 escritas/volta com `SCB_CleanDCache` viraram **suspeitas de criar o defeito que mediam** (inocentadas depois, mas o voo enxuto precisou existir para isso).
8. **O mudo sem porta de saída (18/08).** O comando `bb arm` chamava `LogMute(true)` e **não existia nenhum caminho de volta** — a flag só zerava no boot. Quatro janelas seguidas de teste de MIDI reportaram "0 CCs" e eu li como link morto. O log estava mudo **porque eu mesmo tinha armado a caixa-preta numa janela anterior**: cada medição seguinte mediu o meu próprio silêncio.

**A escalada, nomeada: números → conclusões.** Os casos 1 a 7 estragaram *medições* — um pico perdido, um perfil sobrescrito, um contador zerado. O caso 8 estragou uma *conclusão*, e a conclusão foi **publicada**: com base nesses quatro zeros eu commitei neste documento uma causa errada para a falha de MIDI (ver o adendo no capítulo 11). Instrumentação quebrada não para na planilha — ela chega ao registro permanente, e de lá volta como premissa da sessão seguinte.

**O agravante:** o próprio comando avisava — `"caixa-preta armada; log mudo ate o proximo reset"`. A mensagem saía **uma vez**, no armamento, e eu nunca a li porque vinha antes das linhas que eu estava esperando. **Aviso que aparece uma vez não é aviso; é sorte.**

**O antídoto (v0.39.0): todo comando de status denuncia os modos que silenciam.** `board`, `midi` e `mscope` imprimem `[LOG MUDO]` no cabeçalho. E o ponto fino, que é a lição de desenho: **a declaração tem que ser imune ao modo que declara.** Um banner impresso pelo `Log()` normal morreria mudo junto com todo o resto — por isso existe `LogAlways()`, que atravessa o mudo, e só o banner o usa. Antídoto que pega a mesma doença não é antídoto.

Esse antídoto é o **mesmo** da cicatriz 7 do capítulo 3 (o rótulo que mentia, "transições: NENHUMA" impresso com todos os contadores zerados): as duas falhas são o instrumento **não declarando o próprio estado**, e as duas se curam obrigando-o a declarar.

9. **O watchdog que condenou o inocente (18/08).** O CTOS ganhou veto
   automático: efeito que estoura o prazo entra em bypass. Na primeira
   execução ele mediu **4 277 071 629 µs** — lixo, assinatura de subtração
   estourada — leu como estouro e **desligou o freeze**, que gastava 35 µs de
   500. Os 15 s de "aceite do freeze" que rodaram em seguida foram, na
   verdade, 15 s do instrumento **sem freeze**, e eu quase os apresentei como
   resultado.

**A escalada completa a família: números → conclusões → AÇÕES.** Os casos 1–7
corromperam *medições*. O caso 8 corrompeu uma *conclusão*, e ela foi
publicada. O caso 9 **agiu**: não relatou errado, ele *mudou o sistema* com
base no erro.

> **Um instrumento que só observa erra e alguém percebe. Um instrumento que
> age erra e altera o sistema antes de alguém perceber.**

**A regra:** todo mecanismo automático de veto exige **teste de
plausibilidade antes do gatilho**. Amostra implausível é **descartada e
contada**, nunca julgada — e o contador aparece no status, senão o descarte
vira outro silêncio. Juiz que aceita prova corrompida condena o inocente.

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

## 10. O dia 17/08 — quatro cicatrizes

**Estado de diagnóstico que sobrevive ao diagnóstico.** O `level` ficou em 0.4 desde o A/B com o Max e virou default do repo por três commits, junto com gate e governor desligados. Pior que o descuido: eu **documentei a violação no STATUS como temporária** e segui em frente. Registro que se sabe errado e não conserta não é aviso — é permissão. O dB só descobriu indo tocar o instrumento real. É prima da instrumentação-que-apaga-o-dado, mas pior: aquela estraga a medição, esta estraga o instrumento, e some do radar justamente por estar anotada. **Estado de bancada tem prazo, e o prazo é a mesma sessão.**

**Regra que sobrevive ao motivo.** O `fx2_on` nascia em bypass pra proteger de um risco que a medição já tinha eliminado — vocoder em 37% do prazo, watchdog funcionando, morte do loop com outra causa já consertada. A regra continuava cobrando preço (o dB encontrou "só o delay" seis vezes num dia) por um perigo que não existia mais. E o custo era invisível de um lado e óbvio do outro: **quem escreve a regra não é quem aperta o RESET.** Regra de segurança precisa de data de revisão, senão vira superstição.

**`bytes 0` tem três causas, e a ordem de suspeita muda com o contexto.** No dia da solda é cruzamento TX/RX (o guia manda desconfiar dele primeiro, e está certo). Depois que o fio está provado, é quase sempre o **link BLE** — que cai sozinho, some depois de RESET, e é derrubado por outro central que se conecte ao mesmo periférico. No palco vai ser sempre essa. Receita: *esquecer o device → parear de novo → reiniciar o app que manda*. E **"conectado" num app prova que aquele aparelho tem o link, não que o CC sai de lá** — BLE MIDI costuma aceitar um central por vez, e um segundo app toma a conexão em silêncio.

**Guardar um teste que passa é guardar um instrumento.** O que fechou o caso do MIDI foi um teste que já tinha passado: mesmo botão, mesmo CC, funcionara vinte minutos antes e parou sem ninguém tocar no firmware. Referência conhecida-boa vale mais que hipótese nova — se o que funcionava parou e o firmware não mudou, o firmware não é o suspeito.

### O corolário que atravessa as quatro: replace que falha calado

Duas destas cicatrizes quase não existiram — o script que as escrevia usou uma âncora errada, o `replace` não casou, **não deu erro**, e eu reportei como feito. O mesmo padrão já tinha comido uma célula de instrumentação inteira (o `VOLTAS=0, nan` que eu li como bug de escopo e era **código que nunca foi inserido**).

**Edição programática sem verificação é a mesma família da instrumentação que apaga o dado**, aplicada ao próprio ato de documentar. Regra: `assert` na âncora antes de substituir, e `grep` depois pra confirmar que entrou. Ferramenta que falha calada mente com a mesma confiança com que acerta.

---

## 11. MIDI, BLE e a hora que eu gastei trocando de suspeito (17/08)

**O teste que elimina variáveis vale mais que dez que trocam de suspeito.**
Passei uma hora atrás de um MIDI que não chegava, e troquei de hipótese
cinco vezes — link BLE caindo, fader desconfigurado, app sem autorização,
DMA abortado, controlador mandando dump. **Nenhuma foi confirmada nem
descartada com evidência.** O dado era sempre o mesmo (`bytes 0`) e a
explicação era nova a cada rodada.

O **loopback** (jumper D13→D14, comando `midiloop`) resolveu em dez
segundos: mensagem sai pelo TX, volta pelo RX, percorre UART → DMA → ISR →
fila → parser → mapa → parâmetro. Passou com 113 mensagens, zero órfãos,
zero ressync, zero perdidos. **A placa inteira provada, e o problema
isolado fora dela.**

Ele estava disponível desde o começo, na fila congelada esperando um fio.
A regra: **quando o mesmo sintoma se repete pela terceira vez, pare de
gerar hipóteses e procure o teste que corta o sistema ao meio.** Trocar de
suspeito parece progresso e não é — é a mesma medição vestida de nova.

**E a causa real, achada pelo dB depois de eu esgotar cinco hipóteses: o
pareamento BLE não sobrevive ao reset da placa.** Todo `bytes 0` da tarde
veio depois de um reset sem reparear; todo sucesso veio depois de parear.
Não era link instável, nem 5V no pino de 3V3, nem brown-out do rail, nem
central competindo — era procedimento de operação que ninguém tinha
escrito. **Gravar e testar MIDI não cabem no mesmo ciclo.**

> **ADENDO DE 18/08 — esta conclusão foi publicada em cima de silêncio
> auto-infligido.** Ela continua acima, sem corte, porque o registro do
> que eu concluí importa tanto quanto a correção.
>
> As quatro janelas de "0 CCs" que sustentaram o *"não sobrevive ao
> reset"* aconteceram **com o log mudo pelo meu próprio `bb arm`** (caso 8
> do capítulo 2). Elas não mediram o link: mediram o meu silêncio. **O
> pareamento pode ou não sobreviver ao reset — nós ainda não sabemos, e o
> que estava escrito aqui como causa confirmada não estava confirmado.**
>
> O que permanece verdadeiro e independente do erro: o dB observou por
> conta própria que reparear fazia funcionar, e o loopback (113 mensagens,
> zero órfãos) prova o caminho **dentro** da placa. O que cai é a
> atribuição de causa exclusiva.
>
> A revalidação está desenhada: gravar (o que já é um reset) → **não
> reparear** → `log on` e conferir eco → só então `mscope stats`. Sem eco
> confirmado, aborta e não interpreta. **Qualquer resultado é lucro:** ou
> some uma burocracia inventada, ou o procedimento vira dado.
>
> **RESOLVIDO no mesmo dia, por medição: o pareamento SOBREVIVE ao reset.**
> Firmware v0.40.0 gravado (DFU + gravação + reset, sem reparear nada),
> canal provado por eco, fader movido: **65 CCs, 220 bytes, zero órfãos,
> zero ressync, zero perdidos.** A causa que este capítulo publicou está
> **refutada**, não apenas em dúvida. Não havia fenômeno de BLE nenhum —
> havia um log que eu tinha mandado calar. **O procedimento
> "gravar → reparear → testar" era superstição nascida do caso 8**, e o
> BOARDS.md pode voltar a ser simples.
>
> A lição que sobra é sobre a forma do erro, não sobre BLE: **uma
> explicação plausível preencheu o buraco que a medição quebrada abriu, e
> depois se defendeu sozinha** — cada novo zero parecia confirmá-la, porque
> cada novo zero vinha da mesma instrumentação quebrada. Hipótese que se
> alimenta do próprio defeito não é hipótese; é eco.

O que me cegou: `bytes 0` é idêntico em todas as causas — fio cruzado,
módulo sem alimentação, app sem autorização, central competindo, e
pareamento velho. Eu tinha o sintoma certo e nenhuma forma de distinguir,
e em vez de procurar o que distinguisse, **gerei explicações**. A pergunta
que resolveria — *"o aparelho ainda está pareado?"* — é a mais básica do
checklist e foi a única que eu não fiz.

**Instrumento que só diz "não chegou" precisa de um par que diga "o que eu
mandei, chegou".** O `mscope` mede o que entra, e por isso nunca podia
distinguir "a fonte não mandou" de "mandou e se perdeu". O loopback é o
par dele: fonte conhecida, caminho real, resposta binária. Todo canal de
entrada devia nascer com o seu.

**Erro de UART engolido em silêncio.** O callback do DMA fazia
`if(res != OK) return;` sem contar nada — overrun, framing e ruído de
linha ficavam invisíveis, e `bytes 0` com a porta reportando **ESCUTANDO**
era indistinguível de "ninguém está mandando". Contador adicionado.

**E a escuta DMA pode ser abortada sem voltar sozinha.** Erro de UART pode
derrubar a recepção no stack da ST, e o `DmaListenStart` não se rearma: o
sintoma seria exatamente "funcionou e parou pra sempre", com a porta
dizendo que escuta. Agora há vigia no `PollMidi` que rearma, contador de
rearmes, e o comando manual `rearm`. **Não é remendo — é a contrapartida
obrigatória de um DMA circular que ruído de linha pode derrubar.**

**Estado de teste anterior contamina o teste seguinte.** Os búzios ficaram
ligados de uma rodada passada, com limiar 0.04, sorteando `dly_time`,
`dly_mix`, `voc_pitch` e `voc_tone` **continuamente** durante os testes de
MIDI — o instrumento se modificava sozinho enquanto eu media outra coisa.
É a prima do `tone on` esquecido durante a medição do reverb. **Zerar o
ambiente antes de medir é parte da medição.** (E de brinde: 0.04 dispara
com o chão de ruído do piezo, sem ninguém tocar — dado que só apareceu por
acidente.)

---

## 11b. O conserto que quase sobreviveu ao problema (18/08)

Com o vocoder fora do build, o dB ouviu o instrumento e disse: **"o som está
com uma presença de grave que nunca teve antes, muito melhor sonoramente."**

A causa está em uma linha de escopo. O passa-alta de 120 Hz vive **dentro**
do bloco do Fx2, e `fx2_want` exige um módulo no slot:

```cpp
const bool fx2_want = fx2 && params.Real(manifest.p_fx2_on) >= 0.5f;
```

Slot vazio ⇒ bloco pulado ⇒ **filtro não existe mais**. O grave do piezo
passa inteiro.

**Por que isso é um acerto e não sorte.** Aquele filtro nasceu pra tratar um
defeito *do vocoder* — o estouro de grave que só acontecia com ele ligado. O
estouro se **ouvia na saída**, então o lugar óbvio pra filtrar era o master.
A decisão foi outra, e está escrita no manifest: *"NO CAMINHO DO FX2, não no
master: sem vocoder não estoura."*

Se tivesse ido pro master, ele teria sobrevivido à saída do vocoder e o
instrumento ficaria fino **para sempre**, sem ninguém saber por quê — um
filtro sem dono, tratando um problema que não existe mais.

**A regra:** *um conserto pertence ao escopo do defeito, não ao lugar onde o
sintoma aparece.* Filtro que existe por causa de um efeito mora no caminho
daquele efeito. Quando o efeito sai, o conserto sai junto — de graça, sem
ninguém precisar lembrar.

O corolário incomoda mais: **todo conserto instalado fora do escopo do seu
defeito vira dívida invisível.** Ele continua rodando depois que a razão
morreu, e o custo aparece como "o instrumento sempre soou assim".

---

## 11c. O cabo (18/08) — cinco hipóteses, nenhuma certa

A placa passou horas "morta": USB enumerava, DFU gravava, e o app não dava
sinal — sem banner, sem resposta, sem som. **A causa era o cabo USB.** Com um
cabo curto, tudo voltou ao normal. Quem achou foi o dB, trocando o cabo.

**O que eu testei, e o veredito de cada uma:**

| Hipótese minha | Como testei | Veredito |
|---|---|---|
| freeze quebrando o boot | build com o freeze fora | ❌ mesma falha |
| alimentação / rail com o WIDI | só USB, WIDI fora | ❌ mesma falha |
| cabo ou USB do host | DFU enumera e grava | ❌ *(conclusão errada)* |
| QSPI corrompida | leitura de volta = ruído | ❌ era upload não suportado |
| `cdc_dead` sem retentativa | conserto gravado | ❌ sintoma idêntico |

**As bissecções foram bem feitas e a conclusão foi errada.** Cada teste
isolou uma variável e deu resposta limpa; o problema é que a variável certa
nunca entrou na lista como *suspeita ativa* — eu a marquei como **descartada**
logo no começo, porque "o DFU enumera e grava, logo o cabo está bom".

**O erro de raciocínio, que é o que vale guardar:** DFU e app usam o mesmo
cabo mas **não o mesmo regime**. O bootloader faz transferências curtas, em
rajada, sem áudio rodando; o app mantém CDC contínuo com o loop e o áudio
disputando. **Um cabo marginal passa no primeiro e falha no segundo.** Provar
que o cabo serve pro DFU não prova que ele serve pro app — e eu tratei como
se provasse.

**A regra:** *descartar um suspeito exige testá-lo no regime em que o defeito
aparece.* Descarte feito em outro regime não é descarte, é mudança de
assunto. E suspeito descartado cedo demais some da lista — não é reexaminado
por mais bissecções que se faça depois, porque bissecção só divide o espaço
que sobrou.

> Custou horas de sessão, seis gravações, um conserto no alvo errado
> (legítimo, mas não era a causa) e três afirmações minhas de "isso fecha o
> diagnóstico" que não fechavam nada.

---

## 11d. O instrumento que não pode depender do paciente (18/08)

O loop principal morreu. Serial, MIDI e caixa-preta pararam juntos — e os
**três** passam pelo loop. Nenhum conseguiu relatar a própria morte.

Passei uma hora testando hipóteses (freeze, alimentação, QSPI, cabo, log)
**sem nenhuma observação por baixo**. Todas erradas. O que destravou foi
piscar o LED da placa direto do loop: em dois segundos ele disse mais que uma
hora de sondas pela serial, porque não passava por nada que estava morto.

**E a primeira versão do LED tinha o mesmo defeito.** Era o próprio loop que
piscava, então ele só sabia dizer *"morreu"*, nunca *"onde"*. A segunda versão
é dirigida pelo **callback de áudio** — o único contexto que sobrevive — e
pisca N vezes com N igual à última fase que o loop completou.

> **Um instrumento de diagnóstico não pode depender do que ele diagnostica.**
> Se o observador e o paciente compartilham o destino, não há autópsia — só
> silêncio.

Corolário prático: **todo canal de diagnóstico precisa declarar de que
contexto ele depende.** Serial depende do loop. MIDI depende do loop.
Caixa-preta depende do loop pra ser lida. O LED dirigido pelo áudio depende
só da interrupção. Quando os três primeiros calam ao mesmo tempo, isso não é
coincidência — é a assinatura do que eles têm em comum.

---

## 11e. Comandos fantasma: a quarentena funcionou e não bastou (18/08)

Transições de link BLE fazem o iOS reenviar o estado de todos os widgets. O
instrumento obedecia. A quarentena de reconexão foi construída pra isso e
**barrou 154 CCs em duas reconexões** — funcionou. E mesmo assim o resto do
despejo passou e pôs **sete parâmetros no máximo**: `src_mix` em 1.0 (jack de
linha, nada plugado) e `gate_thresh` em 1.0 (portão sempre fechado)
emudeceram o instrumento.

O gatilho da quarentena é *≥ 4 CCs distintos em < 100 ms após > 5 s de
silêncio*. As duas brechas: despejo mais **espalhado** que 100 ms, ou **sem os
5 s de silêncio** porque o músico estava tocando.

**A defesa mais forte não é apertar o gatilho — é reduzir a superfície.**
`src_mix`, `gate_thresh` e `gov_ceiling` são **roteamento e segurança**. A
regra da whitelist dos búzios já dizia que esses jamais entram em sorteio; a
mesma lógica vale pro MIDI, e ninguém tinha feito a ligação:

> **O que não está mapeado não pode ser derrubado por fantasma.**

Foram exatamente esses três que emudeceram o instrumento. Custa uma linha por
CC tirá-los do mapa, e nenhuma quantidade de heurística de timing dá a mesma
garantia.

**Assimetria que o dado expõe:** o instrumento aceita "tudo no máximo" como se
fosse música. Sete parâmetros no topo simultâneo não é gesto humano — é
assinatura de máquina, detectável **por si só**, sem depender de timing. Daí a
segunda defesa: **limite de taxa de mudança por parâmetro** vindo de MIDI. Um
fader real anda em rampa; `src_mix` de 0 a 127 num único CC é fisicamente
impossível.

---

## 11f. O filtro de sanidade que descartava a catástrofe (18/08)

O caso 9 (watchdog que vetou um efeito saudável com base em 4 277 071 629 µs)
foi consertado com um **filtro de plausibilidade**: amostra acima de 10× o
orçamento é descartada e contada, nunca julgada. Isso impediu vetos falsos —
e criou um defeito novo, apontado pelo dB:

> **Um estouro catastrófico apareceria exatamente assim.**

O filtro definia "implausível" por **magnitude**. Mas magnitude sozinha **não
distingue outlier-por-erro de outlier-por-evento**. Um bloco que realmente
levasse 5 ms — o pior caso que o veto existe pra pegar — seria descartado
como ruído de relógio. O watchdog ficou cego justamente para aquilo que ele
foi feito para ver.

**E ajustar o limiar não conserta: só move a cegueira.** Qualquer corte por
magnitude tem, do outro lado, uma catástrofe do mesmo tamanho.

**É a família do dead-man cego pro host morto** (o disjuntor que declarava o
CDC morto e nunca mais tentava) **e da janela de graça que apagava a estreia**
(o filtro que existia pra perdoar o custo inicial e apagava a única medição
dele). Nos três, um mecanismo de proteção destrói o dado que justifica sua
existência.

### A causa real, achada por leitura de fonte

```cpp
uint32_t GetTick() { return tim_hal_handle_.Instance->CNT; }
uint32_t GetUs()   { return GetTick() / (GetFreq() / 1000000); }
```

`System::GetUs()` **divide antes de quem chama subtrair**. O `CNT` é um
contador livre de 32 bits, mas o valor devolvido vive numa faixa muito menor
— a 200 MHz, `2³²/200 ≈ 21 474 836 µs`, ou seja **dá a volta a cada ~21,5 s**.

Subtração `uint32` só corrige a volta quando o valor ocupa a faixa **inteira**.
Como não ocupa, uma volta no meio da medição produz `0 - 21 474 836` =
**4 273 492 460** — que é o número observado, a menos de onde no ciclo a volta
caiu.

**A taxa confirma:** o bloco mede ~100 µs a cada 1 ms, então a volta cai
dentro da janela em ~10% das vezes. Uma volta a cada 21,5 s em 2 h dá ~335
voltas → ~33 medições corrompidas esperadas. Observadas: 21. Mesma ordem.

**Conserto:** medir em **ticks crus** e converter **depois** da subtração. O
`CNT` ocupa os 32 bits todos, então a volta se corrige sozinha. Com o relógio
certo, o filtro por magnitude foi **removido** — número grande voltou a
significar estouro de verdade, e volta a vetar.

**A regra:** *filtro que define "implausível" por magnitude descarta a
catástrofe junto com o ruído.* Se um outlier incomoda, ache **por que ele
existe** — outlier reprodutível é bug achável, quase sempre de configuração
ou de aritmética, não fenômeno.

**Pré-requisito que isso estabelece:** um atuador automático exige um relógio
**monotônico e confiável**. Sem isso, o caso 9 está armado para reincidir —
não é questão de se, é de quando.

---

## 11g. A defesa que sobreviveu à ameaça (18/08)

A quarentena de reconexão foi construída porque despejos de estado do iOS
punham `src_mix` e `gate_thresh` em valores perigosos e **emudeceram o
instrumento três vezes numa sessão**. Ela funcionava: barrou 154 e 14 CCs em
dois eventos.

Horas depois, duas decisões do dB — **cada uma correta isoladamente** —
mataram a ameaça que a justificava:

1. `src_mix` e `gov_ceiling` **saíram do mapa MIDI**. *O que não está mapeado
   não pode ser derrubado.*
2. Os defaults do que o MIDI alcança foram a **zero**, e o Loopy (que só
   transmite) virou a fonte da verdade.

Com isso o despejo deixou de ser ataque e virou a **inicialização desejada**.
E a quarentena continuou rodando, agora descartando exatamente o que o
instrumento precisava para acordar:

```
msgs completas 5 | APLICADAS 0
quarentena: 5 CCs recusados
```

Cinco mensagens no boot, zero aplicadas, instrumento mudo. O sintoma era
idêntico a "o MIDI não responde" — e a causa era uma defesa nossa fazendo
exatamente o que fora mandada fazer.

**Não é bug: é defesa órfã.** Nenhuma linha estava errada. O que mudou foi o
mundo em volta dela, e ninguém revisou a defesa quando isso aconteceu.

**É a família do passa-alta de 120 Hz** (cap. 11b) — conserto que existia por
causa do vocoder. A diferença é instrutiva: **aquele saiu sozinho porque
estava no escopo do defeito** (dentro do bloco do Fx2), e este não saiu porque
estava no escopo do sistema inteiro.

**A regra, mais dura que a do cap. 11b:**

> **Toda defesa precisa declarar a ameaça que a justifica**, para que se possa
> perguntar se a ameaça ainda existe.

Sem isso, defesas acumulam e viram folclore: cada uma custando alguma coisa,
nenhuma sendo revista, e a soma delas eventualmente impedindo o sistema de
funcionar. O conserto aqui não foi remover — foi **rebaixar a defesa a
sensor**: continua detectando e anunciando a reconexão (diagnóstico que se
quer ter), e parou de descartar.

**Corolário de processo:** quando uma decisão remove uma classe de ameaça,
o passo seguinte é *listar o que existia por causa dela*. Nós tomamos as duas
decisões e não fizemos essa pergunta.

---

## 11h. Proposta que o outro lado já executou (18/08)

Propus um mapa de CCs renumerado, apresentei a tabela, pedi confirmação — e
**não implementei**. O dB alinhou o Loopy pela tabela.

O resultado foram várias rodadas caçando um `level` que "não respondia":
o fader mandava CC 20 (o novo `level`) e a placa ainda tinha CC 20 =
`dly_time`. Os dois lados estavam certos em relação a documentos diferentes.

O diagnóstico só destravou quando o dB colou a tabela de volta e perguntou
*"mas... e isso?"*.

**A regra:** *proposta que o outro lado já executou deixa de ser proposta.*
Uma tabela apresentada como decisão vira contrato no instante em que alguém
age sobre ela — e o custo de implementar tarde é maior que o de implementar
junto, porque no intervalo os dois lados divergem **em silêncio**.

---

## 11i. Os dois tetos que estouraram calados, e a caixa única (18/08)

Quatro parâmetros novos do shimmer empurraram **três** para fora da tabela:
`kMaxParams = 32`, o manifest declarava 35, e o `Add()` devolvia `-1` **sem
dizer nada**. Os três últimos declarados eram os **búzios inteiros** — a
feature sumiu do instrumento.

No mesmo dia, o mapa MIDI chegou a **17** entradas com `kMaxMidiMap = 16` e
**sem nenhuma guarda**: a 17ª foi escrita **fora do array**. O comando `midi`
imprimia a linha (lê pelo mesmo índice estourado) e o `ApplyMidiCc` não achava
o dono — **duas leituras contraditórias da mesma tabela**, que é a assinatura
de escrita fora dos limites.

**Sintomas:** uma feature ausente, e um `?` numa tabela de debug. Nenhum erro,
nenhum log. Os dois só apareceram porque alguém olhou.

**Quinta instância da mesma família no mesmo dia:** `log_muted` sem porta de
saída, `cdc_dead` que nunca retentava, `replace` que não achava a âncora,
`Add()` devolvendo −1, e o mapa MIDI sem guarda. **Mecanismo que falha em
silêncio.**

**Conserto em dois níveis, e o segundo é o que importa:** os tetos subiram
(48 e 32, com folga proposital pros ~12 params dos quatro efeitos que faltam),
e o estouro passou a **gritar no boot** com o número exato. Subir o teto
resolve hoje; a falha ruidosa resolve todas as próximas.

> **Estouro de capacidade é defeito de PROJETO, não condição de runtime.** Quem
> adiciona o 49º parâmetro precisa saber na hora, não descobrir por uma feature
> que sumiu.

### A causa profunda, e a resposta do dB: a caixa única

O teto era o sintoma. A causa é que **não havia lugar único onde a lista de
features vive** — então ninguém revisa o que já existe antes de acrescentar, e
o custo de cada adição fica invisível até algo quebrar.

Daí [`docs/FEATURES.md`](FEATURES.md), decidido pelo dB em 18/08:

- **Toda feature vigente numa caixa só**, com params, CCs e estado.
- **Aposentadas vão pro arquivo**, no mesmo documento, com *por que saiu* e
  *como reviver*.
- **A caixa é o começo de toda sessão**, e tem checklist próprio: *o que já
  está aqui que eu deveria revisar antes de acrescentar mais?*

**Por que o arquivo importa tanto quanto a lista:** este projeto já tem
**sete** features aposentadas, e cada uma deixou rastro — o vocoder deixou
params vivos e CCs mortos, o freeze RNBO deixou um wrapper, a regra do
pareamento deixou um procedimento que ninguém precisava seguir. **Aposentar
sem registrar produz folclore**: coisas que continuam no código porque ninguém
lembra por que estão lá.

É a mesma raiz do cap. 11g (a defesa que sobreviveu à ameaça): sem declarar
por que algo existe, não há como perguntar se ainda precisa existir.

---

## 11j. O controle que não controlava (18/08)

O dB, sobre o shimmer recém-nascido: **"o som está lindo mas o ganho está
baixo, mesmo com o mix no máximo."**

A causa era literal: **eu nunca chamava `Mistura()`.** O `shm_mix` existia,
era lido, escalava um campo interno — e nada no caminho de áudio consumia esse
campo. Todo o nível audível vinha do laço de realimentação, então o controle
girava sem estar ligado a coisa alguma.

**O que torna esse defeito perigoso é que o efeito FUNCIONAVA.** Soava bonito,
respondia ao feedback, ao shift, ao tamanho da sala. Um controle morto no meio
de um efeito vivo não produz sintoma até alguém tentar usá-lo — e mesmo então
o relato natural é *"está baixo"*, que manda procurar ganho, não fiação.

**A regra:** *todo param novo precisa de um teste que prove que ele MOVE
alguma coisa.* Não que exista, não que receba CC, não que apareça no `list` —
que **mude o som**. O PAT (T18) faz isso pelo lado do MIDI; falta o
equivalente do lado do DSP.

**Sinal de alerta genérico:** quando um controle "não parece fazer efeito", a
primeira hipótese não deve ser *"a faixa está errada"* ou *"o ganho é baixo"*.
Deve ser ***"ele está ligado em alguma coisa?"*** — e essa pergunta se
responde lendo o código, em segundos, antes de qualquer ajuste.

---

## 11k. Reportar como feito o que o assert barrou (18/08)

Três vezes seguidas eu anunciei um conserto como gravado, e o `main.cpp`
tinha ficado em **v0.67.0** o tempo todo. As edições morriam num `assert` de
âncora — comportamento **correto** da disciplina de edição segura, que
protege o arquivo — e eu só conferia o resultado da **gravação**, nunca o do
**script**.

O `EXPIROU`/`GRAVOU` da laçada me dizia que o binário subiu. Não me dizia
*qual* binário.

**O que mascarou:** os consertos que importavam estavam em `params.h` e
`midi.h`, e **esses** foram escritos. Os búzios voltaram, o `shm_shift`
passou a responder — tudo funcionou. Só o log de capacidade no boot não
existia, e era justamente a peça que provaria que o resto estava certo.

> **Sempre conferir a versão do `main.cpp` — no arquivo E na placa — como
> parte da rotina.** O boot já imprime `Cotucatronics OS vX.Y.Z`: comparar
> com o fonte custa uma linha e fecha a diferença entre *"eu editei"* e *"a
> placa tem"*.

**A regra geral:** *saída de ferramenta não é confirmação de intenção.* A
laçada confirma que gravou; o `assert` confirma que a âncora casou; o
`grep` confirma que o texto entrou. **São três confirmações diferentes, e eu
tratei a primeira como se cobrisse as três.**

Corolário desagradável: quanto melhor a proteção contra edição silenciosa
(o `assert`), mais fácil é *não perceber* que ela agiu — porque o efeito de
uma edição barrada é o arquivo continuar igual, e arquivo igual não faz
barulho.

---

## 11l. O RESET que não precisava existir (18/08)

Esta sessão gastou perto de **vinte gravações** brigando com a janela de ~2 s
do bootloader. Metade falhou porque eu pedia o RESET e **só depois** começava
a escutar — a família de ordem, aplicada a duas pessoas em vez de a duas
funções.

A pergunta do dB — *"tem como eu colocar um jumper pra você fazer o rst por
aí?"* — tinha uma resposta melhor que jumper: **`System::ResetToBootloader(
DAISY_INFINITE_TIMEOUT)`**. A placa se reinicia direto no DFU e **espera pra
sempre**. Comando `dfu go` pela serial, e a gravação acontece sem ninguém
tocar em nada.

**Resultado medido, na primeira gravação seguinte:**

| | tentativas |
|---|---|
| últimas três com RESET físico | **119**, 66, 47 |
| primeira com `dfu go` | **1** |

**A lição não é sobre a API — é sobre quanto tempo se pode gastar contornando
um atrito antes de perguntar se ele precisa existir.** O atrito era aceito
como fato da bancada desde a primeira sessão, e todo o fluxo foi construído em
volta dele: laçadas de retentativa, pedidos de "toca o RESET", coordenação de
tempo entre duas pessoas. A solução estava no header da libDaisy o tempo todo.

> **Atrito repetido vira paisagem.** Quando algo incomoda pela terceira vez,
> pergunte se ele precisa existir — *antes* de automatizar o contorno.

**E a regra irmã, que a rotina de versão encontrou em minutos:** o `board` não
imprimia a versão fora do banner de boot, então a checagem que o dB acabara de
pedir era **inexecutável** para quem conecta depois. Uma regra que não pode
ser executada não é regra. Corrigido no mesmo ciclo.

**Virou skill portátil** — `bancada-firmware`, sem nada de Cotucatronics
nela: reset por software antes do botão, conferência de versão nos dois
lados, e "saída de ferramenta não é confirmação de intenção". Vale para
qualquer projeto embarcado futuro.

---

## 11m. Escrevi a regra três vezes e violei quatro (18/08)

**"Silêncio não distingue 'ninguém mexeu' de 'caiu'."** Está no `ble`, está no
BOARDS.md, está no battle_scars. E eu construí **quatro** instrumentos que
tratam ausência de dado como dado:

1. O detector de queda BLE dizia **"provável QUEDA"** quando o dB só tinha
   parado de mexer no fader. Corrigido — e o próprio conserto documentou a
   regra.
2. O monitor de tráfego me fez ler **"degradação do link"** num gráfico que
   era o dB tocando normalmente: fluxo humano é naturalmente esburacado.
3. O veredito do teste de fade imprimiu **"não voltou"** enquanto os
   contadores embaixo mostravam 21 CCs aplicados.
4. O aviso de sessão MIDI disparou **"SESSAO MIDI AUSENTE"** com o link
   perfeito, porque "zero bytes desde o boot" também é verdade quando ninguém
   encostou em nada ainda.

**O padrão não é esquecer a regra — é aplicá-la ao sistema e não a mim.** Nas
quatro vezes eu sabia que silêncio é ambíguo; nas quatro vezes escrevi código
que resolvia a ambiguidade **escolhendo um lado**, porque escolher produz uma
frase útil e admitir não produz.

> **A correção nunca é o limiar. É parar de afirmar.**
>
> Instrumento que não consegue distinguir dois estados tem que **dizer isso**,
> e vir com o teste que distingue:
>
> ```
> >>> Se voce JA mexeu num controle, a sessao caiu: reconecte.
> >>> Se ainda nao mexeu, isto aqui nao diz nada: mexa e olhe de novo.
> ```

**Sinal de alerta para escrever qualquer mensagem de diagnóstico:** se a
frase afirma uma causa, pergunte *"que outro estado produz exatamente esta
evidência?"* — e se houver um, a frase tem que citá-lo.

---

## 11n. Duas camadas com o mesmo sintoma (18/08)

Depois de toda gravação o dB tinha de esquecer o Bluetooth, reparear o WIDI e
voltar ao Loopy. Três passos, toda vez.

Medido com o pino do LB:

| Camada | Depois de gravar |
|---|---|
| **BLE** — o que o LB vê | ✅ conectado, `LB 0/100%` |
| **Sessão CoreMIDI** — o que o Loopy usa | ❌ morre |

**O pareamento nunca quebrou.** Esquecer o dispositivo era trabalho gasto num
problema que não existia — a mesma forma da regra *"reparear após todo
RESET"*, revogada por medição algumas horas antes, no mesmo dia.

Ritual de três passos vira **dois**: reconectar no painel Bluetooth MIDI (dá
pra fazer dentro do próprio Loopy) e voltar a tocar.

**E isso expõe o limite da camada 1 que construímos hoje:** o LB responde
*"o rádio está ligado?"*, **nunca** *"o MIDI está fluindo?"*. A matriz de seis
quadrantes precisa da ressalva — `FIXO + zero tráfego` não distingue *"o app
parou de mandar"* de *"a sessão MIDI caiu com o rádio intacto"*. Mesmo par de
sintomas, causas diferentes, e a segunda acontece **toda vez** que se grava.

> **Instrumento novo não resolve a pergunta — ele move a fronteira do que
> ainda é ambíguo.** Vale saber onde a nova fronteira ficou.

---

## 11o. O desperdício que preserva a correção (18/08)

Três efeitos seguidos pagaram o dobro pela mesma razão:

| Efeito | O que duplicava |
|---|---|
| **drive** | `tanh` calculado 2× sobre o mesmo valor mono |
| **phaser** | 2N allpass em vez de N |
| **detune** | gravação escrevendo o mesmo número nos dois canais |

**A causa não é "a cadeia é mono".** É que **a interface oferecia estéreo, o
dado era mono, e nada tornava o descompasso visível.** Todo efeito expunha
`Process(l, r)` como entrada natural; passar o mesmo valor duas vezes parecia
inofensivo, e o tipo `float` não sabe se carrega um canal ou dois.

### As três regras, da mais estreita à mais útil

**1. Desperdício que preserva a correção não tem sintoma.** Os três produziam
áudio **certo**. A única evidência era custo. Revisão não pega, teste não
pega, ouvido não pega — **só medição pega, e só se alguém medir.**

**2. Quando a forma da API não corresponde à forma do dado, o descompasso
cobra em silêncio e repetidamente.** O conserto não é disciplina: eu falhei
três vezes *sabendo* da regra. É fazer o caminho barato ser o **padrão** e o
caro ter que ser pedido — `ProcessMono` devia ter nascido como `Process`, e o
estéreo como `ProcessStereo`.

**3. Conserto sem perguntar "onde mais esta forma aparece?" reincide.**
Consertei no drive, repeti no phaser, repeti na gravação do detune. A pergunta
custa trinta segundos e teria pego as três de uma vez.

> **A terceira é a mais cara do projeto inteiro.** Ela também explica o
> `log_muted` → `cdc_dead` → `Add()` → `AddMidi()`: quatro mecanismos que
> falham em silêncio, consertados um a um, porque nenhum conserto perguntou
> onde mais aquela forma existia.

**O agravante da gravação do detune:** ela roda mesmo com o efeito **em
bypass**. Eram 33 µs pagos **continuamente** para escrever o mesmo número duas
vezes — custo que nem aparece quando o efeito está desligado, que é quando se
espera custo zero.

---

## 11p. O sinal errado como régua, terceira aparição (18/08)

O phaser soou "como um tremolo bem sutil". Medido: **excursão de nível de
0,1 dB** com o notch varrendo — ou seja, não havia notch.

A causa era um par de sinais trocados na equação de diferenças:

```c
y = a*x + x[n-1] - a*y[n-1]   →  H(z) = (a + z⁻¹)/(1 + a·z⁻¹)   polo em z = −a
y = -a*x + x[n-1] + a*y[n-1]  →  H(z) = (z⁻¹ − a)/(1 − a·z⁻¹)   polo em z = +a
```

Polo em **−a** fica perto de **Nyquist**. Com `a ≈ 0,97` — calculado para
200 Hz — toda a ação de fase acontecia perto de **24 kHz**, e os notches
nasciam fora do espectro audível. **A conta do coeficiente estava certa; a
equação é que não correspondia a ela.**

Depois do conserto: **0,1 dB → 14,3 dB de excursão.**

**O relato do dB foi o que tornou isso achável em minutos.** *"Parece um
tremolo bem sutil"* descreve exatamente o resíduo de um notch fora de banda —
sobra a ondulação de nível, some o caráter. *"Não funciona"* teria mandado
procurar no LFO ou no mix.

> **Sintoma descrito pelo caráter vale mais que sintoma descrito pela
> ausência.** Vale pedir: *"com o que se parece?"*, não *"funcionou?"*.

---

## 11q. O estado que não sobrevive ao flash (20/08)

Configurei o detune, gravei a placa para consertar os tetos das tabelas, e
anunciei *"o detune está ligado e pronto"*. O dB foi tocar e não ouviu nada.

Não era o efeito. Era o relato. A regravação zerou todos os params — que é
**a regra que o próprio dB pediu** (tudo nasce em 0, ele sobe pelo Loopy). Eu
repeti como presente um estado que era verdade antes do flash:

```
37 det_on     norm=0.00 real=0.00     ← desligado
39 det_voices norm=0.00 real=1.00     ← uma voz
40 det_spread norm=0.00 real=0.00     ← sem largura
```

O caminho do código estava certo, o `Mistura()` era chamado, o motor
funcionava. Nada disso importava com a chave em zero.

> **Estado de placa afirmado antes de um flash não sobrevive ao flash: ou se
> relê depois, ou não se afirma.** Vale para qualquer estado volátil —
> params, calibração, modo de log. A regravação é a fronteira, e afirmação
> não a atravessa.

**Corolário que virou código.** A denúncia de estouro de tabela existia desde
a v0.70 e **não evitou a segunda ocorrência**, porque só saía no banner de
boot — quem conecta a serial depois nunca a vê. Agora o estado das tabelas
sai também no `board`, sob demanda:

```
tabelas: 55/64 params | 37/48 CCs
```

> **Aviso que exige estar presente na hora certa é aviso que se perde.** Todo
> alarme de boot precisa de um gêmeo consultável a qualquer momento.

---

## 11r. Metade da queixa (20/08)

O dB: *"tem frequências agudas, **médio-agudas** no shimmer que estão bem
ardidas"*. Encontrei uma causa real e a consertei.

**Aliasing.** Ler o buffer a `ratio 2.0` é compressão no tempo: tudo acima de
12 kHz atravessa Nyquist e **dobra de volta inarmônico**. Medido no motor: um
seno de 20 kHz reaparecia em **8 kHz a −11,9 dB**. E o laço fabricava isso
sozinho — com 3500 Hz na entrada, o pico **mais alto de toda a cauda** era
19963 Hz, que não é oitava de nada. Depois do anti-alias: **−64 dB**.

Publiquei a vitória. O dB voltou: *"ainda ardido"*.

A resposta estava na minha própria tabela, e eu a li como efeito colateral
inofensivo:

| | antes | depois |
|---|---|---|
| acima de 12 kHz | 19,2% | 1,2% |
| **2–6 kHz** | **26,4%** | **35,7%** |

A faixa médio-aguda **cresceu como fatia**. Eu tinha tirado energia de cima e
deixado a metade médio-aguda intacta — proporcionalmente mais audível do que
antes. O passa-baixa em 6 kHz que eu instalei **não toca em 2–6 kHz**: por
construção, não podia consertar a metade que o dB nomeou primeiro.

> **A queixa tem tantas partes quantas o dB nomeou.** Consertar uma parte e
> medir melhora não é consertar a queixa. Antes de publicar, reler a própria
> medição perguntando o que ela diz sobre a parte que eu *não* ataquei.

---

## 11s. A hipótese que a medição matou (20/08)

Para a metade restante eu tinha uma hipótese boa, e o próprio código a
endossava: o cabeçalho do `detune.h` já dizia que grãos descorrelacionados
"somam em potência, nunca em fase" e que o caminho, se incomodasse, era
**grão mais longo**. Emenda de fase espalha energia justo no médio-agudo.

Medi antes de implementar. Seno puro de 1000 Hz, pitch ×2 — a saída ideal é
um seno puro de 2000 Hz, e tudo fora do pico é sujeira fabricada pelo motor:

| grão | sujeira |
|---|---|
| 50 ms | −15,8 dB |
| 100 ms (o atual) | −25,5 dB |
| 200 ms (buffer maior) | −36,2 dB |

Dez decibéis disponíveis. Ia gastar memória para pegá-los. Antes, uma última
medição: **planura espectral** da faixa 2–6 kHz na cauda real — próxima de 1
é ruído, próxima de 0 é tonal.

```
cauda do shimmer:  0,009      cauda seca de reverb:  0,004
```

**Tonal.** A sujeira granular existe, mas não é o que arde — e grão de 200 ms
até **piorou** a faixa (49,7% contra 35,7%). Hipótese morta antes de virar
código, por uma medição de trinta segundos.

O que arde é **som legítimo no lugar mais sensível do ouvido**: o 1–3 kHz da
escultura levado a 2–6 kHz pela oitava e sustentado pelo reverb. Não é
defeito de implementação — é o que um shimmer fiel faz.

> **Contra som legítimo no lugar errado não se usa filtro de ruído: usa-se
> EQ.** E a profundidade se escolhe contra uma **régua medida**, não de
> ouvido: a razão (2–6 kHz)/(300–2000 Hz) da cauda *seca* é o timbre natural
> do instrumento na sala. O shimmer dava 1,01 contra 0,28 do seco; o sino de
> −6 dB em 3 kHz o pôs em 0,19.

**Cicatriz menor, cara em tempo: cascata fraca não é filtro íngreme.** A
primeira versão do anti-alias eram três passa-baixas de um polo. Medido:
−25,6 dB, insuficiente. A conta explica — `y = (1−a)x + a·y` tem ganho
`(1−a)/(1+a)` em Nyquist, que com corte em 10 kHz é apenas **−4,8 dB por
polo**, e três já custavam −6,5 dB em 8 kHz. Fraco onde precisava cortar,
pesado onde precisava passar. Butterworth de 6ª ordem acertou as duas pontas.

> **Empilhar filtros baratos não substitui escolher a topologia certa.**
> Medir a resposta do filtro, não presumir que a cascata soma o que se espera.

---

## 11t. A convenção que só funcionava porque todos os casos eram iguais (20/08)

O `piezo_mix` nasceu grudado num piezo só — `real=-1.00`, o extremo
esquerdo — e um dos dois transdutores teria ficado mudo sem sintoma.

A assinatura sempre disse a verdade:

```c
int Add(const char* name, float def_norm, float min_real, float max_real, ...)
                          ^^^^^^^^ NORMALIZADO, está no nome
```

**Todo parâmetro do projeto até aqui tinha `min_real = 0`.** Com mínimo zero,
normalizado e real coincidem, e a distinção nunca precisou existir. Copiei o
padrão de chamada de dezenas de vizinhos corretos sem verificar que a
condição que os tornava corretos — mínimo zero — ainda valia. O `piezo_mix` é
o primeiro de faixa **simétrica**, e nele `def_norm = 0` é a ponta, não o meio.

> **Padrão copiado carrega uma pré-condição que ninguém escreveu.** Ao ser o
> primeiro caso de uma forma nova (faixa negativa, unidade diferente, sinal
> invertido), reler a assinatura em vez de imitar o vizinho — os vizinhos
> estavam certos por um motivo que pode não valer para o caso novo.

Pego pelo `list` antes do dB tocar, e só porque a leitura de estado depois de
gravar já era rotina (11q). A cicatriz anterior pagou por esta.

---

## 11u. Não afirmei estado errado — destruí estado em silêncio (20/08)

A 11q dizia: *estado afirmado antes de um flash não sobrevive ao flash*.
Escrevi a regra, virou skill, e no mesmo dia o dB relatou "sem som no p2".

O `level` estava em **0,00**. Eu havia gravado a placa **quatro vezes** na
mesma conversa, cada gravação zerando o master, e **nunca reposto nem
avisado**. Ele estava re-dialando pelo Loopy a cada rodada sem que eu
soubesse, e uma das perguntas dele nasceu de um silêncio que era meu.

A 11q cobria a metade errada do problema. Afirmar estado velho é um erro de
**relato**; destruir estado sem avisar é um erro de **ação** — e este é pior,
porque o outro lado não tem como saber que aconteceu.

> **Toda gravação destrói estado. Repor o estado de operação e DECLARAR o
> que foi reposto faz parte de gravar** — não é cortesia, é fechar a ação.
> Mínimo: `level`, `in_gain` e as chaves do que estava tocando.

---

## 11v. A pergunta que o instrumento não sabe responder (20/08)

*"Chega sinal no segundo piezo?"* — e não havia como saber. O firmware media
CPU, MIDI, picos por nota, ganho do limiter, custo de cada efeito, e **não
media a própria entrada, por canal**. Piezo mudo e piezo baixo soam
exatamente igual atrás de um reverb.

Medidor cru (pré-balanço, pré-`in_gain`, pré-gate) resolveu em cinco minutos
e respondeu de imediato, antes mesmo de alguém tocar:

```
IN_L (p1): pico 0.00182 = -54.8 dBFS | 4783 amostras
IN_R (p2): pico 0.00422 = -47.5 dBFS | 9515 amostras   ← chega ao conversor
```

Pino morto daria zero absoluto. O canal existe; a pergunta virou "o transdutor
entrega?", que é outra pergunta e tem outro dono.

**A mesma lacuna produziu um erro de fato que durou o dia inteiro.** Citei
`in_gain = 8,0` como "18 dB de fábrica" em três respostas e o pus como defeito
aberto no STATUS. O default é **1,0×** — está escrito no manifest, na mesma
linha do `Add()`. O 8,0 era **ajuste do dB**, dialado pelo Loopy e persistido
até a gravação seguinte.

> **Valor lido de uma placa viva não tem procedência.** Antes de chamar um
> número de "default do projeto", ler o default no fonte — a placa mostra o
> que está lá, não quem o pôs.

> **A pergunta que o instrumento não sabe responder é a que será respondida
> com hipótese.** Quando um canal de sinal novo entra (entrada, saída, sensor),
> o medidor dele entra junto — não depois do primeiro mistério.

**O desfecho inverteu o diagnóstico.** Com o dB tocando:

```
IN_L (p1): pico 1.00000 =  -0.0 dBFS |  42 365 amostras | clip 65
IN_R (p2): pico 0.46398 =  -6.7 dBFS | 244 237 amostras | clip 0
```

O p2 — o canal "sem som" — entrega −6,7 dBFS e tem **quase 6× mais amostras
ativas** que o p1. Quem tem defeito é o **p1**, que bate no teto do conversor
(65 amostras em clip) antes de qualquer ganho de firmware e ocupa todo o
espaço. "Sem som no p2" era o p1 saturado encobrindo o vizinho — exatamente o
caso que o `piezo_mix` foi construído para resolver, chegando pela porta de
trás como se fosse um canal morto.

> **O sintoma nomeia o canal onde a falta é OUVIDA, não o canal onde a causa
> está.** Numa soma, o que some pode estar perfeito: quem o apaga é o outro
> somando. Antes de investigar o lado silencioso, medir o lado alto.

E o conserto de firmware aqui é paliativo declarado: baixar `in_gain` e abaixar
o p1 pelo balanço recupera equilíbrio, mas **clipping no ADC não se desfaz a
jusante**. O conserto real é físico.

---

## 11w. Dois canais de saída não são uma imagem estéreo (20/08)

A especificação do `out_width` dizia "na saída final, após a espacialização
estéreo". Aplicado ao pé da letra, teria desfeito o stack anti-feedback.

`OUT_L` vai para o exciter **no corpo da escultura** e é o ramo que realimenta
o piezo: leva governor e shifter SSB. `OUT_R` é jack de linha, não realimenta,
e sai limpo **por desenho**. Depois do split eles não são L e R de uma imagem
— são dois **destinos** com processamentos diferentes.

`mid = (L+R)/2` e `side = (L-R)/2` ali dentro somariam o ramo protegido com o
desprotegido, reinjetando sinal sem shift no fio que fecha o laço acústico. A
proteção continuaria existindo no código e teria deixado de existir no ar.

O lugar certo é **entre o LEVEL e o split** — o último ponto em que os dois
canais ainda são a mesma imagem.

> **Antes de qualquer operação que MISTURE canais (mid/side, soma mono,
> correlação, pan), perguntar o que cada canal É naquele ponto.** "Estéreo" é
> uma propriedade do trecho, não do cabo.

---

## 11x. Canal mudo prova que aquele canal está mudo (20/08)

Implementando o **aviso obrigatório** do teto de soma — a parte do desenho que
existia justamente para não repetir a família "falha em silêncio" — chamei
`LogAlways()` e `MidiSendBytes()` de dentro do `AudioCb`.

Escrever no CDC de dentro do callback starva a ISR do USB. **É o defeito de
prioridade 1 do `STATUS.md`, escrito lá havia dias, e eu o cometi mesmo
assim.** A implementação do aviso derrubou o instrumento.

Família A dos bugs de ordem, na forma mais pura: quem arma e quem executa têm
que estar no mesmo contexto, e o contexto que pode falar com o mundo é o loop.
Conserto: o callback marca (`volatile int aviso_teto_idx`), o loop lê a marca,
loga e manda o CC.

**Mas a cicatriz não é essa. É o que eu disse depois.**

Com a serial muda, afirmei ao dB: *"a placa continua tocando — só o canal de
conversa caiu"*. Eu tinha medido **uma** coisa: que o CDC não respondia. Dali
inferi o estado do áudio, que é outro subsistema, por nenhuma razão além de ser
a hipótese confortável. O dB descobriu tocando: *"não há som"*. O travamento
era geral.

> **Canal mudo prova que aquele canal está mudo, e nada além disso.** A
> ausência de resposta é a informação mais pobre que existe: ela não distingue
> "o canal caiu" de "o sistema caiu". Antes de afirmar o que sobreviveu, medir
> o que sobreviveu — ou dizer "não sei".

Terceira aparição da mesma família num dia. A 11q foi sobre **afirmar estado
velho**; a 11u sobre **destruir estado calado**; esta é sobre **afirmar estado
que nunca foi medido**. As três têm a mesma raiz: tratar como sabido o que
apenas era conveniente.

---

## 11y. Agir de novo antes de observar a própria ação (20/08)

Teto de soma, teste 1: teto em 620 µs, bloco em 626. Derrubar o detune
(−125 µs) resolvia com folga. **Caíram dois** — detune e phaser:

```
"detune" em BYPASS — bloco 626 us de teto 620
"phaser" em BYPASS — bloco 633 us de teto 620      <- MAIOR que antes
```

Zerar o param **não faz o efeito parar de custar**: o `EffectSlot` desce por
rampa e continua processando enquanto desce. O mecanismo mediu de novo antes
que a própria ação surtisse efeito, viu o bloco ainda alto — mais alto, com o
ruído — e derrubou o próximo da fila.

O preço é exatamente o que a política de prioridade existia para evitar: um
efeito perdido a mais do que o necessário. E o modo de falha é insidioso
porque **cada queda individual parece justificada** pela medição do momento.

> **Mecanismo corretivo automático precisa de carência entre agir e medir de
> novo.** Sem ela ele reage à sua própria ação ainda em curso, e a correção
> vira sobre-correção. A carência não é conservadorismo: é o tempo que a
> realidade leva para refletir o que já foi feito.

250 ms aqui — folgado para as rampas de 8 ms, curto para que um estouro real
não fique sustentado. A cauda de 2 s do freeze não cabe nessa janela, e não
deve caber: durante ela o custo **está** lá, e uma segunda queda seria correta.

---

## 11z. Chave nova num recurso compartilhado cria dependência muda (20/08)

O dB pediu `on`/`off` para o delay e o reverb, que até então ligavam pelo
`mix`. Pedido simples, e cada uma das duas chaves trouxe um problema
diferente.

**O reverb é recurso compartilhado.** O shimmer não tem reverb próprio — ele
realimenta o da cadeia, e isso é o desenho, não um atalho. Um `rev_on` que
pula o bloco inteiro (o que se quer, para custar zero) **emudece o shimmer
junto**, e o sintoma seria "o shimmer parou de funcionar" com o `shm_on` em
127 e nada errado à vista.

> **Desligar um recurso compartilhado emudece quem depende dele, em
> silêncio.** Ao dar chave a algo que outros consomem, a dependência tem que
> se declarar no ponto onde alguém vai olhar — não no comentário do código.

Aqui o `board` passou a imprimir, e só quando a combinação acontece:

```
*** shm_on ligado com rev_on DESLIGADO: o shimmer realimenta o reverb
    e esta MUDO. CC 61 para acordar os dois. ***
```

**O delay é módulo opaco.** A regra da casa — *"bypass pula o processamento,
nunca a gravação"*, que nasceu do clique da Peça 2 — **não é aplicável a um
módulo RNBO**, que não separa as duas coisas. Pular o `Process()` zera o custo
e deixa a linha de delay rançosa: ao religar sai um pedaço de áudio velho.

Não há terceira opção, então a escolha é declarada: o `dly_on` mistura para o
seco e **o módulo segue rodando**. Paga-se custo para não pagar ranço — e o
teto de soma, fechado horas antes, é o que torna esse custo aceitável.

> **Regra herdada tem pré-condição.** Antes de aplicar uma regra da casa a um
> caso novo, verificar se a estrutura que a tornava possível existe ali. A
> regra do bypass pressupunha poder separar gravação de processamento; num
> módulo fechado, essa separação não existe, e insistir nela produziria um
> defeito em nome de uma boa prática.

---

## 12a. Validade de medição tem regime (20/08)

Medi a entrada dos piezos com **dedo direto no disco solto** e registrei o
resultado como *"a entrada satura"*. Escrevi isso no STATUS como defeito
aberto, propus um plano de oito passos em cima disso, e cheguei a dar valores
de divisor resistivo.

O dB cortou: **esse regime não é o de uso.** Piezo desacoplado, excitado por
contato direto, é o caso mais violento que a pastilha encontra na vida. A mesma
pastilha colada na escultura e excitada pela vibração dela entrega outra coisa,
e provavelmente muito menos.

Nenhum número estava errado. **O que estava errado era o alcance da
conclusão.** Eu media um regime e afirmava sobre outro.

> **Toda medição vale para o regime em que foi feita, e o regime é parte do
> dado — não contexto do dado.** Um número sem regime declarado não é uma
> medição incompleta: é uma medição cuja validade ninguém pode julgar,
> inclusive quem a fez.

O projeto já tinha metade dessa regra, na forma *"estreia ou permanente?"* —
regime **temporal**. Faltava a outra metade: regime **físico**, o de como o
sinal está sendo produzido. Piezo solto e piezo montado são dois instrumentos
diferentes para efeito de medição.

**A correção mudou o plano de forma, não só de conteúdo.** A versão anterior ia
dimensionar um atenuador a partir do número errado. A nova ordem é: montar só a
**proteção** (1 kΩ + 2× 1N4148 — não mexe no ganho, protege o ADC de qualquer
jeito), instalar na escultura, tocar, e **só então** medir. O atenuador virou
condicional: entra se clipar tocando, e talvez não entre.

> **Antes de dimensionar uma solução, verificar se o problema existe no regime
> real.** Dimensionar cedo é resolver um problema que talvez não exista, com um
> número que com certeza não vale.

E há um bônus de método: a proteção é a peça **útil em qualquer desfecho**.
Quando há uma medição pendente, a ordem certa é fazer primeiro o que serve
para os dois resultados possíveis.

---

## 12b. Defeito físico num transdutor imita assimetria de circuito (20/08)

O p2 chegava mais baixo que o p1. Medi, confirmei a diferença, e a diferença
virou premissa: entrou no STATUS como característica do par de canais e quase
virou a **justificativa** para duplicar gate e ganho por cadeia.

Era **um piezo com defeito**. Trocados de canal, p1 e p2 chegam exatamente
iguais.

**O que separou as duas hipóteses não foi medir mais fundo — foi trocar os
piezos de canal.** Nenhuma medição elétrica, por mais precisa, distingue "este
canal atenua" de "este transdutor entrega menos": as duas produzem o mesmo
número. O que distingue é **permutar o suspeito**, que é um experimento, não
uma leitura.

> **Quando um número descreve um par assimétrico, troque os elementos de lugar
> antes de atribuir a assimetria ao sistema.** Uma medição mais precisa do
> mesmo arranjo só confirma o número com mais casas decimais — e o número
> nunca esteve em dúvida.

Mesma família da medição do dedo-no-disco (12a): **o número estava certo; o
que ele descrevia é que não era o que se supunha.** Ali era o regime; aqui é
qual elemento do arranjo carrega a propriedade medida.

**A decisão sobreviveu à justificativa, e isso importa registrar.** Gate e
ganho duplicam mesmo — mas por **material**, não por desequilíbrio: o piezo do
tambor lê transientes curtos e fortes, o da mola/corda lê raspagem contínua de
baixo nível. Um limiar que deixa a raspagem passar abre demais para o tambor.
A duplicação vale com dois piezos perfeitamente casados, e valeria se fossem
idênticos de fábrica.

> **Justificativa errada envelhece mal.** Se ficasse escrito "duplicamos
> porque o p2 era mais baixo", alguém mede os canais iguais no futuro e conclui
> que a duplicação é dispensável — desfazendo uma decisão certa por causa de um
> motivo errado que ninguém revisou. A razão registrada tem que ser a razão que
> se sustenta, não a que estava à mão quando se decidiu.

---

## 12c. Verificador que casa texto não distingue o dado da narração (20/08)

Certifiquei a remoção dos LEDs LG/LR com um teste automático: rodar `ble` e
procurar as letras `LG` ou `LR` na saída. Ele acusou **fantasmas**.

A linha que ele encontrou era esta:

```
LB 0/100%   (LG/LR removidos 20/08)
```

O verificador achou exatamente **a frase que anuncia que eles foram
removidos**. A certificação tinha passado; quem falhou foi o certificador — e
eu publiquei "SIM — ERRADO" antes de olhar a saída com os próprios olhos.

> **Verificador que casa padrão em texto precisa distinguir o DADO da NARRAÇÃO
> sobre o dado.** Procurar um símbolo na saída inteira encontra também os
> comentários, cabeçalhos e avisos que falam sobre ele — e quanto melhor a
> mensagem explica o que mudou, mais provável que ela dispare o alarme.

Casar na estrutura, não no texto livre: a linha de dado tem forma conhecida
(`LB <n>/<n>%`), e é nela que se procura. Corolário desconfortável: **a
mensagem bem escrita é a que mais engana o verificador ingênuo.**

Mesmo dia, mesma família: quase publiquei uma decomposição de CPU em que dois
controles marcavam **+0 µs** — porque a edição que os criava tinha falhado no
`assert` e eu gravei o binário anterior. Um controle que não existe mede zero,
e zero parece dado. **O tell foi o +0 exato em dois itens independentes**;
número redondo demais é sintoma, não resultado.

---

## 12d. Delta entre builds não é atribuível (20/08)

O guarda do gate (`if(thresh > 0)`, pular quando o portão está aberto) mediu
**−58 µs** — comparando o total de v0.90.0 contra v0.91.0. Anunciei isso como o
ganho do guarda.

O dB desconfiou: o mesmo gate tinha medido **15 µs** por máscara. Quatro vezes
de diferença no instrumento invalida tudo o que ele mediu.

Medido de novo, **A/B no mesmo binário, na mesma janela** — com `thresh=0` o
guarda pula, com `thresh` mínimo ele processa sem portar nada:

| regime | gate pulado | gate rodando | custo |
|---|---|---|---|
| repouso | 159 µs | 185 µs | **+26** |
| carga total | 618 µs | 648 µs | **+30** |

**O gate custa 26–30 µs e não há super-linearidade.** A hipótese da I-cache
(código menor é mais rápido nesta plataforma) era plausível e já catalogada —
e não se aplica aqui.

Os −58 vinham de comparar **dois builds diferentes**, e entre eles mudaram
duas coisas: o guarda **e** a restauração do histograma pós-ganho, que
*acrescenta* trabalho. O saldo não fecha por ~40 µs em nenhuma direção.

> **Delta entre builds não é atribuível a nenhuma mudança específica.** Nesta
> plataforma o layout do código move a I-cache, e recompilar já muda o número.
> Custo só se atribui por **A/B no mesmo binário, na mesma janela** — de
> preferência alternando por parâmetro, não por recompilação.

A máscara, aliás, saiu absolvida: 16 contra 26 é ~1,6×, dentro do que variância
de layout e de corrida explicam. **O instrumento defeituoso era a comparação,
não a sonda** — e eu tinha acusado a sonda.

**Consequência que fica em aberto:** a rejeição da janela morna do warble
apoiava-se em 7 µs medidos por máscara. Corrigido pelo mesmo fator daria ~11 µs
— ainda pouco, mas é **estimativa**, não medição. Um A/B honesto exigiria um
param de toggle só para isso. Fica declarado como número fraco em vez de
promovido a fato.

---

## 12e. Deleção por faixa calculada, e a instrumentação órfã (20/08)

Duas vezes na mesma sessão apaguei código que não pretendia apagar. As duas
operações têm a mesma forma: **deletar uma faixa cujos limites eu calculei em
vez de listar.** O que separa as duas é só o que as pegou.

### Incidente A — silencioso, achado por acaso

Substituir `s[i0:i1]` no `main.cpp`, com `i0` no `// 1) FONTE` e `i1` no
`// 3) MÓDULO`, para trocar os blocos de fonte, gate e caráter pela versão de
duas cadeias.

Dentro dessa faixa também morava o bloco `1.5`, que continha o **escritor** do
histograma pós-ganho (`out_pk_g`, `out_clip_g`, `hist_o[]`).

**Compilou limpo.** As variáveis continuaram declaradas e o comando `hist`
continuou **lendo** elas. O sintoma seria `hist` imprimindo *"pós-ganho 0"*
para sempre — indistinguível de "não chega sinal depois do ganho". Achado por
acaso, três turnos depois, procurando uma âncora para outra coisa. Nunca foi
commitado, mas rodou em firmware gravado durante várias corridas de medição.

### Incidente B — ruidoso, pego em dez segundos

`re.search` com `.*?` e lookahead, `re.S`, para remover um comentário do
`manifest.h`. Alvo: **7 linhas**. Apagou **50**. Colateral de 43: o bloco
inteiro de documentação do `in_gain` (por que mora pré-gate, por que não é o
mesmo que subir o `level`, o que ele **não** resolve) e a linha
`static constexpr int kCadeias = 2;`.

**Não reconstruí com confiança por que o padrão casou tão longe — e isso fica
assim de propósito**, porque a regra que sai daí é melhor que qualquer
explicação póstuma:

> **A falha é rodar operação destrutiva com um matcher de extensão
> imprevisível — não o casamento inesperado.** `.*?` com `re.S` pode
> atravessar o arquivo inteiro dependendo do lookahead. A extensão precisa ser
> **conhecida antes de deletar, não descoberta depois.**

Antídoto de duas etapas: **casar → imprimir o que casou → conferir a contagem
de linhas → só então apagar.**

### A regra

> **O perigo de uma deleção por faixa é proporcional a quanto do conteúdo
> apagado não é referenciado por nome em outro lugar.**

O compilador testemunha **dependências de símbolo, e nada mais.**

**Só-escrita é a subclasse mais perigosa** — produz *zero com cara de dado*.
Mas a mesma cegueira cobre as 43 linhas de prosa do `in_gain`: nenhum símbolo,
nenhuma reclamação, e prosa some ainda mais silenciosamente, porque ninguém
nunca vai notar a falta de um parágrafo que explicava um porquê.

> **O que lê, o compilador cobra. O que escreve, não.**

### A topologia explica os dois tempos

Não foi sorte nem atenção: **B apagou algo com vinte referências, A apagou algo
com zero.** O que tem muitos dependentes é quase impossível de perder em
silêncio; o que não tem nenhum é quase impossível de notar. O incidente B fez
barulho por sorte estrutural, não por mérito meu — se aquela regex tivesse
comido só comentários e instrumentação, teria passado igual ao A.

### Subtipo novo na família: INSTRUMENTAÇÃO ÓRFÃ

Os oito casos anteriores são *instrumentação presente que corrompe o que
mede*. Este é *instrumentação **ausente** que não avisa*. Sintoma oposto — lá
número errado, aqui número que nunca chega — e a mesma raiz:

> **A instrumentação não tem quem a defenda.** Ninguém depende dela, então nada
> a protege: nem compilador, nem teste, nem uso.

### Defesa acionável: dar testemunha à instrumentação

Fazer com que algo dependa dela **por nome** — um `static_assert`, um contador
que o `board` lê e cujo desaparecimento quebre o build, uma linha de teste que
referencie o escritor. Dá ao compilador uma testemunha onde hoje não há
nenhuma. É *hábito automatizado vence hábito lembrado* aplicado justamente à
classe de código que este projeto mais perde.

### Recuperação: recuperar um BLOCO não é restaurar um ARQUIVO

`git checkout` é o reflexo e é o **errado** quando há trabalho não commitado no
mesmo arquivo — no incidente B teria destruído a unificação de params. O que
funcionou:

```bash
git show HEAD:src/manifest.h     # extrai o bloco
# recola cirurgicamente, com âncora literal
```

### E uma correção do próprio relato

Eu disse ao dB *"terceira vez nesta sessão que uma regex minha come mais do que
devia"*. São **duas**, e uma nem era regex — era fatiamento por índice.

> **Contar errado para o lado que se acusa mais não é humildade: é o mesmo
> defeito com o sinal trocado. Em ambos os casos o número não veio de
> contagem.**

Vale sozinho porque a tentação de errar para o lado autocrítico é alta num
documento de cicatrizes, e corrompe o registro tanto quanto o exagero na
direção oposta.

**E a defesa que eu já tinha cobria isto.** A skill `edicao-segura` manda
âncora **literal e única**, conferida antes. Abandonei-a nas duas vezes, e nas
duas pelo mesmo motivo: a operação parecia grande demais para âncoras
individuais. É o raciocínio de quem pula o cinto na viagem curta — a regra
descartada exatamente na hora em que valia mais.

---

## 12f. Eliminação cirúrgica: o compilador como inventário (20/08)

Tirar o RNBO do projeto era remover 56 sítios espalhados por três arquivos,
seis arquivos-fonte, quatro diretórios de export e o sistema de slots inteiro.
Um mês antes eu teria feito isso apagando faixas e rezando — e acabara de
escrever a cicatriz 12e sobre por que isso não funciona.

**O método que funcionou inverte a 12e.** Se o compilador testemunha
dependências de símbolo, então **removo primeiro o símbolo-raiz e deixo que
ele liste os dependentes.**

```
1. remove o `#include "module.h"` e a declaração de `mod_slot[]`
2. compila
3. o compilador enumera CADA sítio restante, com número de linha
4. remove um por um, recompilando
```

Quatro erros na primeira passada, depois um, depois zero. **Nenhuma faixa
calculada, nenhuma prosa perdida.** O que a 12e chama de cegueira do
compilador vira ferramenta quando se usa na ordem certa: ele não vê o que
some, mas vê perfeitamente o que ficou órfão.

**A regra complementar da 12e:**

> **Para remover um subsistema, apague o símbolo do qual tudo depende e deixe
> o compilador enumerar o resto.** Não caçe as referências à mão: a lista dele
> é completa, a sua não é.

**Onde o método não alcança, e isso importa.** Ele só enumera o que tem
dependência de símbolo. O que sobrou invisível:

- **params órfãos** (`voc_mix`, `fx2_gain`…) — existiam na tabela e ninguém
  os lia. Só apareceram quando removi as declarações e o compilador reclamou
  do único consumidor restante, que era um histograma que media um vocoder
  fora do build havia dois dias.
- **prosa** — comentários explicando um sistema que não existe mais. Nenhum
  compilador reclama de um parágrafo órfão.
- **verificações sem objeto** — o `check_repo.sh` tinha um bloco inteiro
  conferindo exports do RNBO. Passava sempre, e ia continuar passando para
  sempre, verificando nada.

> **Depois que o compilador parar de reclamar, ainda falta o que ele não vê:
> params sem leitor, prosa sem assunto, e verificações sem objeto.** Este
> último é o pior dos três: uma checagem que não tem mais o que checar **passa
> com sucesso** e ninguém desconfia de um `ok`.

**O que a remoção rendeu, medido:**

| | antes | depois |
|---|---|---|
| tudo ligado | 678 µs | **545 µs** |
| base (nada ligado) | 182 µs | **115 µs** |
| binário | 212 424 B | 175 980 B |
| params | 61 | 54 |
| repo | — | −8 MB de export |

Os **67 µs da base** são o dado que interessa: o módulo RNBO rodava **ligado ou
desligado**, porque um `Module::Process` não sabe se está em bypass. O efeito
que substituiu custa 11 µs quando ligado e **zero** quando não — a disciplina de
custo-zero-desligado, que todo efeito da casa segue, era estruturalmente
impossível dentro do sistema de slots.

---

## 12g. "Impossível" que era "impossível nesta arquitetura" (20/08)

O dB pediu envio por cadeia em cada efeito da cauda — `dly_mix_a`,
`frz_mix_b`, e assim por diante. Respondi que **não era fisicamente
possível**, e argumentei bem: a cauda é serial, o `piezo_mix` soma A e B
antes dela, e quando o som chega ao freeze já é um objeto único. Não há um
"A" ali dentro para dosar.

Cada frase estava correta. **A conclusão não**, porque toda ela repousava
numa premissa que eu não examinei: *a cauda é serial*.

Serial era uma **escolha de desenho**, tomada quando havia um efeito de cada
tipo e nenhuma razão para outra coisa. Em paralelo — cada efeito virando um
destino irmão com envio próprio — o pedido não só é possível como custa quase
nada: **cada efeito continua sendo um só**, apenas recebe sua própria dose de
cada cadeia.

O dB insistiu, eu reexaminei, e o custo real da mudança foi **+15 a 20 µs**
contra os "impossível" que eu tinha dito.

> **Antes de dizer "não é possível", separar o que é lei do que é decisão
> minha.** Física não muda; arquitetura é escolha, e uma escolha antiga com
> boas razões *na época* não vira lei por ter durado.

O sintoma de que a distinção não foi feita é justamente a **qualidade do
argumento**: eu sustentei "impossível" com uma cadeia correta de deduções
sobre um sistema que eu mesmo tinha construído. Argumento bom sobre premissa
não examinada é mais perigoso que argumento fraco — ele convence os dois lados.

### E a estimativa não atravessou junto

Previ **807 µs** para o pior caso somando os custos medidos de reverb (+99) e
shimmer (+182) da cauda **serial**. Medido depois da mudança: **639 µs**, 209
abaixo.

A razão é a mesma da cicatriz: na cauda serial cada efeito processava o sinal
**inteiro**; em paralelo cada um recebe só o seu envio, e o shimmer de uma
cadeia não processa a cauda da outra. **Duas salas custaram menos que o dobro
de uma.**

> **Custo medido numa arquitetura não se transporta para outra.** Somar
> números de um desenho para prever outro é a mesma família do "delta entre
> builds não é atribuível" (12d) — ali o layout muda o número, aqui a
> topologia muda o que cada peça processa.

Errei para o lado seguro desta vez. Mas errar 26% para cima numa conta que
decide se algo cabe é o tipo de erro que, na direção contrária, teria matado
uma ideia boa antes de ela ser tentada.

---

## 12h. Redundância criada por uma mudança em outro lugar (20/08)

O dB perguntou: *"qual a função de esp_mix? já não temos rev_mix?"*

Tinha razão, e eu precisei ler o código para descobrir que sim:

```
entrada da sala = cadeia × esp_mix     ← ganho na ENTRADA
sala 100% molhada
retorno         = saída  × rev_mix     ← ganho na SAÍDA
```

Dois ganhos em série no mesmo caminho. **Numa sala linear, entrada 0,5 com
saída 1,0 soa idêntico a entrada 1,0 com saída 0,5** — era o mesmo controle
escrito duas vezes.

**Ninguém adicionou um controle redundante.** Cada peça era correta quando
entrou: `rev_mix` era o dry/wet do reverb desde sempre, e `esp_mix` nasceu
como **crossfade** entre "vai pro espaço" e "sai seco", num desenho em que a
cauda era serial e o seco precisava contornar.

Quando a cauda virou **paralela**, o seco passou a passar por fora sozinho — e
o crossfade virou um ganho simples. **A redundância foi criada pelo refactor,
não pela adição.**

> **Refactor não cria só código morto: cria CONTROLE morto.** Um parâmetro que
> era necessário na arquitetura antiga vira duplicata na nova, e nada avisa —
> dois ganhos em série compilam, medem e soam. Só ficam confusos.

Depois de mudar a topologia, a pergunta não é só "o que quebrou?", mas **"o que
deixou de ser necessário?"**. A primeira o compilador responde; a segunda não
tem quem responda.

### E um segundo defeito no mesmo lugar

No dia anterior eu tinha feito o shimmer abrir a sala sozinho, com esta linha:

```c
rm = rev_quer ? rev_mix : shm_mix;   // shm_mix vira o ganho da SALA
...
m  = shimmer.Mix() * gm;             // shm_mix TAMBEM e a camada direta
```

**`shm_mix` com dois empregos** — a camada pitcheada direta *e* o ganho de saída
do espaço. É a **família C** do catálogo deste repositório, que eu mesmo
mantenho, plantada por mim menos de vinte e quatro horas antes.

O que a fez passar: ela resolvia o problema imediato ("o shimmer precisa de um
nível quando o reverb está off") com o param que estava à mão. **O segundo
emprego nunca foi decidido — foi aproveitado.**

> **Variável com dois empregos raramente nasce assim: ela ganha o segundo
> quando alguém precisa de um valor e há um por perto.** A pergunta que
> impede: *este param já responde a outra pergunta?*

Conserto dos dois: um envio só por destino (`rev_on`/`rev_mix`, mesma forma do
delay e do freeze), retorno inteiro, e `shm_mix` de volta ao emprego único.
Quatro params e quatro CCs a menos.

---

## 12i. Faixa do param maior que o alcance do efeito (20/08)

O dB: *"não estou vendo o frz_size_b funcionar"*.

O param ia de **20 a 400 ms**. Dentro do efeito, `SetSize` limita o grão a um
**terço do núcleo**, e o núcleo tem 500 ms:

```
faixa do param:  20 .. 400 ms
teto real:       500/3 = 167 ms
→ 61% do curso do fader não fazia NADA
```

Varrendo a parte de cima, os dois canais ficam travados no mesmo valor e nada
muda. **O sintoma de um fader que para de responder é indistinguível do
sintoma de um controle quebrado** — e foi assim que o dB o descreveu.

**Não era defeito do B.** O A tinha exatamente o mesmo, e ninguém tinha
percebido porque ninguém tinha varrido o topo com atenção.

### O que faz isto passar

A trava mora **no efeito** (`freeze.h`, `nucleus/3`) e a faixa mora **no
manifest**. Nenhum dos dois conhece o outro, e **os dois estão corretos em
isolamento**: o param guarda 400 e o efeito usa 167, cada um fazendo o seu
trabalho. Não há erro para o compilador ver, nem para uma medição de CPU, nem
para um teste de "o efeito funciona?".

> **Sempre que um efeito trava um param internamente, a faixa declarada tem
> que bater com a trava.** Faixa maior que o alcance é curso morto; faixa
> menor é alcance desperdiçado. E ninguém detecta nenhum dos dois — o defeito
> vive no espaço ENTRE dois arquivos que estão certos.

### E a lição já existia

Em 18/08 o `dly_time` teve exatamente isto: ia de 10 ms a 1 s e o primeiro
terço ficava abaixo da percepção de eco. O conserto de lá está escrito no
manifest, com a frase certa:

> *"O corte é no PARÂMETRO, não na curva, e a diferença importa: curva
> redistribui o mesmo alcance e deixa o inútil no fim do curso; limite REMOVE
> o inútil do alcance."*

**A lição foi aplicada e não generalizada.** Ficou como nota sobre *aquele*
param em vez de virar pergunta a fazer sobre *todos*. Dois meses depois o
mesmo defeito estava em outro lugar, escrito por mim, e precisou do dB
tropeçar nele.

> **Conserto que vira nota local reincide; conserto que vira pergunta de
> rotina, não.** A pergunta aqui é curta: *o efeito trava este param? em
> quanto? a faixa bate?*

### A varredura que a pergunta produziu

Feita agora em todos os efeitos com trava interna:

| efeito | trava | faixa do param | |
|---|---|---|---|
| freeze `size` | núcleo/3 = 167 ms | era 20–400 | ❌ **61% morto — corrigido pra 20–165** |
| warble `depth` | 3,0 ms | 0–3 | ✅ bate |
| phaser `stages` | 8 | 4–8 | ✅ bate |
| delay `time` | buffer − 2 amostras | 50–1000 ms | ✅ trava a 2 amostras do topo, inofensivo |
| pitch `grain` | buffer/4 = 100 ms | não é param | — fora do mapa |

Um em cinco estava errado. **A varredura levou dois minutos e só aconteceu
porque alguém tropeçou** — que é o argumento para ela virar item do IFÁ10000
em vez de reação.

---

## 12j. A feature que morreu e ninguém notou por cinco commits (20/08)

O dB pediu uma auditoria dos params, um por um. A primeira camada — *quem lê
cada param?* — acusou dois: `buzios_thresh` e `buzios_cooldown`, declarados,
na tabela, **mapeados em CC**, e sem nenhum leitor.

Não eram params órfãos. Era o **detector inteiro dos búzios** que não existia
mais.

Apagado em `6a8175d`, a reestruturação em duas cadeias: ele vivia **pós-gate**,
dentro da faixa que substituí de uma vez. Cicatriz 12e cobrando pela terceira
vez — e desta vez levou uma **feature**, não instrumentação. Os búzios são o
gesto que dá nome ao mecanismo: cada batida na escultura é uma jogada, e o que
cai decide o timbre.

### O que fez isso durar cinco commits

Nada denunciou, e vale listar por quê:

- **compilou limpo** — `buzios_env`, `buzios_fire`, `buzios_cd` continuaram
  declaradas, e o loop continuou LENDO `buzios_fire`. Só ninguém mais o
  escrevia.
- **os params continuaram na tabela** e atingindo min e max normalmente. Uma
  varredura de valores passaria.
- **os CCs continuaram no mapa**, apontando para o nada.
- **o `board` reportava tudo certo**, porque tudo estava certo — exceto que o
  efeito não acontecia.

> **Feature morta não se parece com defeito: parece com feature desligada.** O
> instrumento com `buzios_on` em 1 e nenhum sorteio é indistinguível do
> instrumento com `buzios_on` em 0 — e `buzios_on` nasce em 0, então o estado
> normal É o estado quebrado.

### A auditoria que a achou, em três camadas

Nenhuma delas depende de alguém tropeçar:

1. **quem lê cada param** — grep de `params.Real(manifest.p_X)` contra a lista
   de `t.Add()`. Achou os búzios em segundos.
2. **faixa declarada × trava interna** — a da cicatriz 12i.
3. **varredura viva** — cada param no mínimo e no máximo, com leitura de volta
   da placa. Passou 75/75.

Quinze minutos no total, e achou uma feature morta há cinco commits.

> **Param sem leitor é feature morta até prova em contrário.** A checagem é
> textual, barata, e é a única das três que encontra o que o compilador não vê
> — porque um param órfão compila.

### E um falso positivo que quase reportei

A camada 2 acusou `drv_drive`, `drv_tone` e `phs_rate` de "alcance
desperdiçado": faixa declarada **menor** que a trava do efeito. Fui conferir e
os três são deliberados, com o raciocínio escrito no manifest — *"curso inteiro
útil, agressivo só no fim"*.

> **A trava do efeito é rede de segurança, não alvo.** Faixa MENOR que a trava
> é escolha musical; só faixa MAIOR produz curso morto. Um verificador que não
> distingue as duas direções acusa metade dos params corretos.

Segunda vez no mesmo dia que um verificador meu produz alarme falso, e pelo
mesmo motivo: **a regra foi escrita como "compare os números" quando o que
importa era "compare, e saiba qual direção é defeito"**.

---

## 12k. A trava muda: três efeitos "quebrados" que estavam intactos (22/08)

**Sintoma.** "phaser, drive e shimmer não estão soando, warble, delay,
rvb e frz estão." Fui procurar defeito nos três módulos.

**O que os três tinham em comum não era um defeito — era uma trava.**
O teto de soma do CTOS havia disparado 28 minutos antes, durante a
varredura da auditoria de parâmetros (que liga os dez efeitos de uma vez
e estourou os 850 µs), e derrubou, na ordem declarada: phaser A e B,
drive A e B, freeze A e B, shimmer A e B. A queda é **travada** — de
propósito, senão o mecanismo oscilaria. E a trava é reimposta **a cada
bloco**:

```cpp
if(soma_caiu[k]) params.Set(manifest.sacrif[k].param_on, 0.f);
```

**A impressão digital estava à vista e eu não a li.** Tudo que o
operador reportou mudo estava na lista de sacrifício. Tudo que soava —
delay e reverb — **não está na lista**. O warble aparecia como
"desligado", nunca caiu, e por isso ligava normal. A lista de sacrifício
era a resposta inteira, disponível num `ctos` desde o primeiro minuto.

**O que fez isso custar uma sessão: os três canais de leitura mentiam
por omissão.**

| canal | o que mostrava | o que era |
|---|---|---|
| `set drv_on_a 1` | `real=0.000` | recusado pela trava, sem dizer |
| `set shm_on_a 1` | `real=1.000` | aceito e desfeito no bloco seguinte |
| `list` | `norm=0.00 real=0.00` | idêntico a um efeito honestamente desligado |
| `board` | silêncio | o aviso de shimmer-sem-sala lê `shm_on`, que a trava zerou |

Esse último é o mais perverso: **a trava emudeceu a defesa que existia
para esse caso.** O aviso "shm_on ligado com rev_on em 0" estava correto
e nunca disparou, porque a condição que ele testa é justamente o valor
que a trava esmaga. Defesa que lê estado pode ser calada por quem
escreve esse estado — a ordem de leitura importa.

**Prova de que não havia defeito:** `ctos rearm` e os três voltaram, com
custo de CPU medido — drive +12 µs, phaser +24 µs, shimmer +296 µs.
Zero linhas de correção em drive.h, phaser ou shimmer.h.

**A lição não é "não trave".** A trava está certa. A lição é que **um
mecanismo que desfaz o comando do operador tem obrigação de dizer que
desfez** — no momento do comando, não num diagnóstico que só quem
suspeita vai pedir. v0.110.0 faz a trava falar nos três canais: o `set`
avisa que será desfeito, o `list` marca `*** TRAVADO PELO CTOS ***`, e o
`board` grita antes de qualquer outro aviso (justamente porque os outros
leem estado que ela falseia).

**Regra:** *estado imposto por mecanismo é indistinguível de estado
escolhido por gente, a menos que alguém escreva a diferença. Se o
firmware sobrescreve uma intenção, ele deve isso ao operador em voz
alta.*

**Sub-lição, do teste do próprio conserto:** `ctos teto 300` não derrubou
nada e eu quase li isso como "o conserto não funciona". Base 188 µs +
phaser 24 µs = 212, abaixo de 300. **Um teto de teste tem que ficar
abaixo da base**, ou o teste não testa nada. Foi o teste que falhou, não
o código.

**Sub-lição 2:** escrevi `SacrifTravado(i) ? "" : "TRAVADO"` numa função
que devolve **−1 quando não está travado**. −1 é verdadeiro; a marca
apareceria em todo param são e sumiria justamente no índice 0. Sentinela
negativa exige `>= 0` explícito — nunca teste de verdade.

---

## 12l. Gravar apaga o instrumento, e ninguém é avisado (22/08)

**Sintoma, dez minutos depois de 12k:** "e drv shm e rvb não estão
soando". Fui procurar a trava do CTOS de novo. Não era.

Era `level = 0.00`. Eu havia gravado duas vezes (v0.110.0 e v0.111.0)
**enquanto o dB tocava**. Cada gravação reinicia a placa, todo parâmetro
volta ao default, e `level` nasce em 0. Os ajustes de uma sessão inteira
de Loopy evaporam — e o Loopy só remanda um CC quando aquele fader se
mexe, então nada se restaura sozinho.

**O que torna isso pior que o teto de soma:** a queda do teto ao menos
*acontece* por um motivo audível e agora se anuncia (12k). A perda por
gravação é **silenciosa dos dois lados**: a placa reinicia sem dizer, e o
operador só descobre pela ausência de som — que ele naturalmente lê como
defeito no efeito que estava usando. Duas vezes hoje eu mesmo caí nessa
leitura.

**A confusão foi minha e era evitável.** Eu estava rodando tetos
artificiais e zerando `phs_on_a`/`drv_on_a` no fim de cada teste com o
instrumento em uso. Todo relato de "não soa" que chegou nessa janela
tinha uma causa minha antes de qualquer causa do firmware.

**Regra 1:** *não se instrumenta o instrumento enquanto ele está sendo
tocado. Se for inevitável, o primeiro suspeito de qualquer sintoma
relatado nessa janela sou eu, não o código.*

**Regra 2:** *estado que só existe em RAM morre a cada gravação. Num
instrumento sem tela, isso é perda de trabalho invisível — e o sintoma
dela imita defeito de efeito.* Aponta pra preset em flash: salvar e
recarregar no boot. Aberto em 22/08.

---

## 12m. O botão que devolve, e o LED que avisa que ele existe (22/08)

Resolvido 12k, ficou a pergunta prática: o dB toca **sem computador**. Se
o teto derrubar no meio da performance, `ctos rearm` por serial não está
ao alcance dele. Resposta dele: *"rearme e botão físico"*. B3 em D25.

**Duas escolhas de desenho que não são óbvias.**

**O botão é assimétrico em relação ao serial, de propósito.** `ctos
rearm` solta a trava e **não religa** — decisão antiga e correta, com o
argumento escrito em código: religar decidiria pelo operador qual
configuração ele queria de volta. Para o botão esse argumento **não vale**,
e é importante entender por quê: `soma_caiu[k]` só fica de pé para efeito
que **estava ligado** quando o teto o derrubou. Não há ambiguidade
nenhuma sobre o que devolver. Devolver não é escolher por ninguém — é
desfazer o que o mecanismo fez. E um botão de rearme que rearma sem
trazer som de volta está quebrado do ponto de vista de quem aperta.

Se não couber de novo, cai de novo em menos de um segundo. Isso é
informação honesta, não falha.

**O alarme entrou na CADÊNCIA do LED, não no estado dele.** O LED já é o
batimento que separa "loop vivo" de "loop morto" — a pergunta que custou
o diagnóstico inteiro do búzios. Roubá-lo para sinalizar trava teria
pago o alarme com a prova. Então: devagar = vivo e sem trava; **4× mais
rápido = tem efeito travado**; congelado = loop morto, nos dois regimes.
Um canal, dois fatos, nenhum perdido.

**A dependência que ninguém declara:** *um controle de correção sem um
indicador da condição que ele corrige é inútil — ninguém aperta um
rearme sem saber que precisa rearmar.* O botão e o LED são uma feature
só; entregar um sem o outro seria entregar nada.

**Sub-lição de implementação:** o simulado `b3` setava o pulso e nada o
baixava. O laço vê a borda, dispara, e `btn_prev` fica em alto **para
sempre** — o botão funcionaria exatamente uma vez por boot. Pulso
simulado tem que ser consumido por quem o lê.

---

## 12n. O efeito que funciona e mesmo assim não soa (22/08)

**Relato:** "tudo funciona exceto o drive." O drive custava +12 µs
medidos — rodava. E não se ouvia.

**Duas hipóteses minhas, as duas refutadas por medição:**

*Hipótese 1: o `drv_tone` nasce em 800 Hz (fundo da faixa) e o LPF fica
DEPOIS do saturador, então mata os harmônicos que são o drive.* Plausível
e errada. Replicando a matemática exata do módulo: a camada somada entra
a **+2,4 dB** sobre o seco em tone 800, contra +3,6 dB em tone 16000 —
1,2 dB de diferença. Mais escura, nunca inaudível.

*Hipótese 2: o `gm` do EffectSlot não chega a 1, então o drive processa
somando zero.* Também errada — e era o mesmo slot que o phaser usa, que
funcionava. Ler o código teria bastado para descartar essa antes de
suspeitar dela.

**Medição na placa, com o `env` do governor como medidor de saída:** na
configuração exata do relato (4×, mix 0.5, tone 800), o drive soma
**+7,5 dB**. Não é inaudível. É alto.

**A resposta não estava no drive. Estava no nível de quem toca.**

    y = tanh(d·x) / tanh(d)

A divisão é o que isola timbre de nível — a razão de ser do módulo,
escrita no cabeçalho dele. Mas ela tem um corolário que ninguém tinha
escrito: **abaixo do joelho, `tanh(d·x) ≈ d·x`, e o waveshaper normalizado
vira ganho limpo.** O joelho fica em `x ≈ 1/d`.

THD da saturação, por nível de entrada:

| pico | 2× | 4× | 6× | 9× | 12× |
|---|---|---|---|---|---|
| −34 dBFS | 0,0 | 0,0 | 0,1 | 0,2 | 0,4 |
| −20 dBFS | 0,3 | 1,1 | 2,5 | 5,2 | 8,4 |
| −9 dBFS | 3,3 | 10,6 | 18,2 | 26,5 | 31,1 |

Com `in_gain` em 1.0, o p1 monta a −9,2 dBFS e o p2 a **−19,5 dBFS**. O
drive colore o instante do ataque do p1 e **desaparece no p2 até no
pico**. O relato "não soa" estava descrevendo o instrumento com exatidão.

**Conserto: `in_gain`, não drive.** Havia 9 e 19 dB de headroom medido e
parado (bts 12a: zero clip com os piezos montados). 2,2× e 6,0×.

**Regra:** *num waveshaper normalizado, o controle de drive não tem
efeito absoluto — ele tem efeito RELATIVO ao nível que chega. "O efeito
não soa" e "o sinal não alcança o joelho do efeito" são sintomas
idênticos e conserto em lugares opostos.* Antes de mexer no efeito,
medir onde o sinal está em relação à faixa útil dele.

**Meta-regra, das duas hipóteses queimadas:** *hipótese sobre módulo com
matemática fechada se testa replicando a matemática em dois minutos,
não flasheando a placa.* As duas medições que decidiram tudo — a camada
por tone e a THD por nível — rodaram no host, sem tocar no hardware.

---

## 12o. O teto era o defeito, e eu consertei a divisão (22/08)

**Queixa:** "quando ligo o drive tenho mais ganho mas a distorção legal que
tinha antes desapareceu."

Ganho sem distorção é a assinatura exata de um waveshaper operando **abaixo
do joelho**, onde `tanh(d·x) ≈ d·x` e a curva é reta. O joelho fica em
`x ≈ 1/d`. Com o teto de `drv_drive` em 12×, ele caía em −19 dBFS — e o
histograma do instrumento tocado põe **98% das amostras abaixo de −18 dBFS**,
com o grosso entre −45 e −25.

**O diagnóstico estava certo e o conserto foi no lugar errado.** Concluí que
a normalização `tanh(d·x)/tanh(d)` era a culpada: ela divide pelo pico para
entrada de fundo de escala, que o instrumento nunca alcança, então não
normaliza nada e sobra ganho bruto de `d`. Troquei o denominador por `d`,
que normaliza o ganho de pequeno sinal. Matematicamente correto.

O dB reprovou em uma frase: *"o timbre da distorção está bom mas bem abaixo
do som dry, phaser aparece muito mais."*

**Eu tinha os dois números na tela quando escolhi errado:**

| | ÷tanh(d) | ÷d |
|---|---|---|
| THD em 48×, nível real | 12,0% | 12,0% |
| camada vs seco | **+30,4 dB** | **−3,2 dB** |

As duas distorcem **igual**. A única diferença é o peso. Um efeito paralelo
precisa PESAR — a camada de `÷d` fica limitada a `1/d`, que em 48× é
−33 dBFS enquanto os picos do instrumento chegam a −9.

**A raiz do erro foi de leitura, não de conta.** A queixa anterior era
"mais ganho, sem distorção", e eu li o **ganho** como o defeito. O ganho
estava certo; a distorção ausente é que era o problema. Consertei a
reclamação que ele não fez.

**Regra:** *quando duas grandezas mudam juntas numa queixa, a que o operador
NOMEIA como boa não é a que ele quer que você conserte.* "Mais ganho MAS sem
distorção" tem um "mas": o que vem depois é o pedido.

**Sub-lição:** antes disso queimei duas hipóteses por medição — o `drv_tone`
nascendo em 800 Hz (mede +2,4 dB de camada contra +3,6 em 16 kHz: mais
escuro, nunca inaudível) e o `gm` do EffectSlot não chegando a 1 (mesmo slot
do phaser, que funcionava — ler o código teria bastado). **Hipótese sobre
módulo com matemática fechada se testa replicando a matemática em dois
minutos, não flasheando a placa.** As duas medições que decidiram tudo
rodaram no host.

---

## 12p. Defesa órfã: a separação de canal que existia só no cabeçalho (22/08)

Implementei o espelho MIDI — a placa transmite seu estado de volta ao iPad
— com duas defesas declaradas contra realimentação:

1. canal separado (escuta em 1, fala em 2)
2. tabela `eco_ult[cc]`, que impede reenviar valor repetido

**A primeira nunca existiu.** Escrevi `kStatusEco = 0xB1` para transmitir e
**não escrevi filtro nenhum na recepção**. O receptor está em omni — e omni
inclui o canal em que a própria placa fala. O `midi` mostrou o crime:

```
cc 65  val 127 ch 1  -> frz_on_a      <- o Loopy respondendo
cc 65  val 127 ch 2  -> frz_on_a      <- a MINHA mensagem, aceita
```

A placa estava obedecendo à própria voz. Metade de uma defesa é uma defesa
órfã, e esta deixou o laço inteiro de pé enquanto o cabeçalho afirmava que
ele estava fechado.

**O que a fez visível foi um contador que não existia.** Não havia número
para "mensagens minhas recusadas na entrada", então uma defesa que nunca
trabalhou era indistinguível de uma que trabalhava em silêncio. Só apareceu
quando o histórico de CCs foi lido à mão.

**Regra:** *defesa sem contador é hipótese. Se o mecanismo não produz um
número que sobe quando ele age, ninguém — inclusive quem o escreveu — pode
distinguir "funcionou" de "nunca rodou".*

**Sub-lição, do mesmo dia e do mesmo módulo:** `eco_proprio` conta mensagens
que **voltaram**; eu o li como "quantas saíram" e relatei "a placa só mandou
4 de 8". Os dois números só coincidem se o caminho de volta for perfeito —
que era **exatamente a hipótese sob teste**. Medir o retorno e chamar de
envio responde a pergunta errada com confiança. Contador de TX separado,
posto depois: `eco: TX 8 | voltaram 4 | perdidas 4`.

**E o número certo mudou a conclusão inteira:** com TX de verdade, o
experimento virou CC65 = 6/8 contra CC68 = 8/8, o que isolou o widget do
Loopy como causa e absolveu o firmware.

---

## 12q. A lacuna entre varreduras é onde o erro mora (22/08)

O modo `learn` percorre os 76 CCs pulsando cada um para o dB armar os
widgets do Loopy. Primeira versão: a placa pulsa, **eu** leio o histórico a
cada 3 s de fora, detecto a resposta do widget e mando `learn next`.

Funcionou por dois CCs. Depois o dB: *"todos estão ficando em cc 26."*

**A causa é a lacuna, não a detecção.** Entre uma chamada minha e outra
podem passar minutos, e o firmware continua pulsando o CC estacionado
indefinidamente. Todo widget armado nesse intervalo aprende o mesmo CC.
Quatro deles aprenderam o 26 porque foi o que ficou no fio durante uma
pausa.

**O sinal certo já existia e estava sendo desperdiçado.** Um CC que chega no
canal de entrada com o número do alvo atual só pode vir de um widget que
acabou de aprendê-lo — é prova de arme, no instante do arme. A placa via
isso e apenas incrementava um contador.

**Regra:** *quem decide o avanço tem que ser quem vê a confirmação.*
Observador externo tem sempre uma lacuna entre o evento e a leitura, e num
processo com humano no circuito essa lacuna não é latência — é a janela em
que o estado errado é gravado. Detecção de fora só serve quando nada muda
enquanto ninguém olha.

**Sub-lição sobre modos:** a caminhada automática avança depois de 4 pulsos
**tenha ou não alguém armado**. Com humano no circuito, o modo certo é o
estacionado: repete indefinidamente e só sai quando a confirmação chega.
Um modo que anda sozinho e um modo que espera não são configuração — são
dois procedimentos diferentes, e escolher errado desalinha tudo em silêncio.

**Sub-lição de implementação:** o `learn next` limpava `learn_fixo`, então o
primeiro passo manual disparava a caminhada automática inteira. O comando
que deveria dar UM passo dava todos.

---

## 12r. Unidade errada num dimensionamento (22/08)

Precisava saber quais dos 76 widgets do Loopy já estavam endereçados. O
histórico de CCs da placa guardava **8** mensagens — pouco. Aumentei para
**96**, com este raciocínio: *96 > 76 widgets, cobre a varredura inteira com
folga.*

Cobriu nove.

**A unidade estava errada.** 96 é de MENSAGENS; 76 é de WIDGETS. Varrer um
fader de ponta a ponta manda dezenas de mensagens. Os 96 slots se encheram
com os últimos nove controles tocados, e os primeiros da varredura caíram
fora antes de eu ler.

**E a estrutura estava errada junto com a unidade.** Histórico responde *"o
que acabou de chegar"*; inventário responde *"o que já chegou alguma vez"*.
São perguntas diferentes e a segunda não se resolve alargando a primeira —
por maior que fique, ela continua sendo uma fila que expulsa o antigo. O
conserto certo custou 128 bytes: um contador por número de CC, que não
transborda por mais que se varra.

**Regra:** *antes de dimensionar uma estrutura, dizer em voz alta a unidade
do número que se está escolhendo E a unidade do requisito.* Se as duas não
forem a mesma, a folga é imaginária. "96 > 76" é verdade e não significa
nada quando um é mensagem e o outro é widget.

**Sub-lição:** dobrei o tamanho, gravei, testei, falhou, e só então
questionei a estrutura. A pergunta *"esta estrutura responde à minha
pergunta?"* é mais barata que uma gravação e vem antes dela.

---

## 12s. Inverter a pergunta resolveu o que três horas de mecanismo não (22/08)

O modo `learn` foi construído para endereçar os widgets do Loopy: a placa
pulsa cada CC, o dB arma o widget, avança. Ganhou confirmação automática,
distinção entre widget novo e já armado por tempo de resposta, pausa que não
empurra, estacionamento que não avança sozinho. Cinco versões de firmware.

Nunca fechou. Em cada passo havia uma janela onde o número errado podia ser
gravado, e a cada janela fechada aparecia outra.

**O dB inverteu a pergunta:** *"eu configurei todos para mandar no canal 1.
Você fala com eles e vê se batem? Se não baterem, você ajusta do seu lado."*

Em vez de fazer os widgets aprenderem os números da placa, **ler o que os
widgets mandam e adaptar o firmware**. Duas varreduras e um comando novo
(`vistos`, 128 bytes) fecharam 65 de 65, com quatro widgets flagrados
carregando número de outro parâmetro — descobertos sozinhos, sem ninguém
procurar.

**Por que o learn era o caminho difícil:** ele exigia coreografia entre três
coisas que não se enxergam — a placa pulsando, o operador achando o widget,
e eu dirigindo **sem ver nada do lado dele**. Eu não tenho acesso ao Loopy:
não vejo widget, nem nome, nem página. Isso deveria ter sido dito na
primeira mensagem sobre o assunto, e só foi dito depois de o dB perguntar.

**Regra:** *quando um mecanismo precisa de uma janela de sincronia a cada
passo e você não enxerga uma das pontas, o mecanismo está errado, não mal
ajustado.* Procurar a formulação em que os dois lados falam quando querem —
e alguém apenas escuta e se adapta.

**Sub-lição, e foi erro puro:** anunciei "CC 42" no chat depois de uma
chamada que só LEU o estado — a placa continuou pulsando o 41. O widget
aprendeu 41. **Nunca anunciar um estado que não se acabou de escrever.**
Ler e afirmar são coisas diferentes.

**Sub-lição 2 — o instrumento cego:** `vistos` só enxerga widget que ENVIA.
Um widget que só recebe é invisível para ele, por melhor que esteja
endereçado. Quatro apareceram como "faltando" e um deles estava perfeito —
o dB viu piscando. Todo instrumento tem um ponto cego; declará-lo é parte de
entregá-lo.

---

## 12t. Verificação que exige desligar o sensor é desenho invertido (22/08)

O modo `learn` e o inventário `vistos` resolviam o mesmo problema —
endereçar os widgets do Loopy — e eram **mutuamente exclusivos**, o que
demorei a enxergar porque parecia questão de preferência.

**Em `learn` a placa fala e NÃO ESCUTA:** todo CC que chega é descartado,
por desenho — os widgets já armados alternam e devolvem durante a caminhada
inteira, e sem esse descarte a caminhada se atropela. Mas o inventário lê
exatamente o que chega. Ligar o learn cega o único instrumento capaz de
provar que o learn funcionou.

Não eram dois métodos para o mesmo fim. Eram **dois regimes incompatíveis**,
e um deles não podia coexistir com a verificação do próprio resultado.

**Regra:** *método de verificação que exige desligar o sensor que o
verificaria é desenho invertido. O método certo deixa o mundo agir e
observa; o errado age e supõe.*

O `learn` agia — pulsava, contava, avançava — e **supunha** que o widget
tinha aprendido, porque não podia perguntar. O `vistos` não faz nada: espera
o mundo se mover e relata. Fechou 65 de 65 em duas passadas o que cinco
versões de mecanismo não fecharam.

**Como reconhecer antes de gastar cinco versões:** se o mecanismo precisa
suspender a entrada, silenciar um canal, ou "não escutar agora" para
funcionar, perguntar imediatamente *o que eu deixo de poder observar
enquanto ele roda* — e se a resposta incluir a própria coisa que ele
deveria produzir, parar ali.

**Aposentado pela forma da casa:** removido em v0.121.0, 136 linhas e ~10 KB
de fonte. O caminho de volta está nomeado (commit 514e09c, onde ele está
inteiro) e a alternativa manual documentada (lista de CCs no README e no
ccmap). Remover sem nomear a volta seria apagar, não aposentar.

---

## 12u. Citei o piso ao lado de um número de carga (22/08)

Relatei de passagem "CPU 180 µs" logo depois de remover o `learn`, ao lado
de uma referência de 509 µs da mesma sessão. Parece otimização de 65%. Não é
nada.

**O 180 é o piso:** medido logo após gravar, quando todo parâmetro volta ao
default e **todo efeito nasce desligado**. O 509 era carga: drive 48× nas
duas cadeias, reverb A+B, shimmer A. Medindo os três com rótulo:

| regime | µs |
|---|---|
| todo efeito desligado | 181 |
| drive 48× A+B · reverb A+B · shimmer A | 526 |
| + sujeira, shimmer B, delay, phaser, warble | 697 |

O 526 bate com o 509 dentro do ruído. Nada mudou.

Isto é a **12d outra vez**, e agora com agravante: a regra já estava escrita
nesta base, na skill `medicao-limpa`, e mesmo assim o número saiu sem
rótulo — porque era menção *de passagem*, não uma medição formal. É aí que a
regra escapa.

**Regra:** *número dito de passagem é medição também.* Se ele aparece perto
de outro número da mesma grandeza, o leitor VAI compará-los, e o regime tem
que vir junto ou a comparação é uma mentira que ninguém escreveu de
propósito.

**E o "achado de brinde" era falso — CORRIGIDO no mesmo dia.** Registrei
que o `cpu` reportava **pico 0 µs** e que o instrumento estava quebrado.
Chegou a virar o passo 0 do plano dos presets.

O medidor da libDaisy está correto: `Reset()` põe `firstCycle_ = true` e o
próximo `OnBlockEnd` faz `max_ = min_ = avg_ = load`, recuperando do NaN. A
placa imprime `69.8% media, 82.9% pico`, como sempre imprimiu.

**O defeito era o meu leitor.** A linha sai como `"...% media, ...% pico"` —
o número vem ANTES da palavra — e eu procurei `pico ([\d.]+)%`. Não casou. E
o meu código era:

```python
return float(m.group(1)) if m else 0      # <- o 0 e a mentira
```

**Um parser que não casa devolveu 0, e eu li 0 como medição.** Esta base já
tem a regra escrita, do histograma: *zero num canal que ninguém escreve não
é ausência de sinal, é ausência de medição.* Eu a violei no meu próprio
script de bancada, e o resultado não foi só um número errado — foi um
**defeito inventado no registro permanente** e priorizado como passo 0 de um
plano.

**Regra:** *leitor que não encontra o dado deve FALHAR, nunca devolver
default.* `if else 0` num parser é a mesma família do canal mudo, e o
instrumento que mente com mais eficácia é o que a gente escreve para medir
os outros.

**E o número escondido era o que mais importava.** Medindo os quatro
regimes com o leitor consertado:

| regime | média | pico |
|---|---|---|
| todo efeito desligado | 183 | 262 |
| drive 48× A+B · reverb A+B · shimmer A | 536 | 687 |
| + sujeira, shimmer B, delay, phaser, warble | 697 | 992 |
| **tudo ligado** | **818** | **912 / 946 / 948** |

A última linha é regime PERMANENTE, três janelas seguidas — não é estreia.
**94% do prazo de bloco de 1000 µs**, sustentado. E há um pico de 1035 µs
registrado no CTOS: acima do prazo, ou seja, uma falha de áudio real.

A sessão inteira eu disse "509 µs, cabe", "697 µs, folga até 850" — **todas
médias**, porque o leitor descartava os picos. A folga que eu vinha
reportando não existia.

**E isso expõe um limite do teto de soma que o desenho dele assumiu:** o
teto exige blocos CONSECUTIVOS acima para derrubar efeito — histerese
deliberada, para não oscilar. A suposição embutida era que picos fossem
raros. Nesta configuração eles são a norma: o teto de 850 é ultrapassado em
todo pico e `quedas` fica em 0. A proteção não está quebrada; ela está
protegendo contra um fenômeno diferente do que está acontecendo.

---

## 12v. Pré-condição de API herdada, revogada sem aviso de compilação (22/08)

Para os presets, a orientação que chegou foi: *"a libDaisy já resolveu isso
— a classe de storage persistente escreve numa área reservada afastada do
código."* Plausível, vinda de quem conhece a plataforma, e **errada em dois
pontos** que só o código mostra.

**Ponto 1 — ela não escolhe região nenhuma.**

```cpp
address_offset_ = address_offset & (uint32_t)(~0xff);   // alinha a 256 B
...
qspi_.Erase(address_offset_, address_offset_ + sizeof(s));
```

Alinha a **256 bytes**, não a setor. O `Erase` apaga o **setor de 4 KB**
inteiro que contém o endereço. Quem escolhe onde é seguro é o chamador — e
o `address_offset` tem **valor default 0**, que em `BOOT_QSPI` cai dentro
dos 256 KB reservados ao bootloader.

**Ponto 2, e este é o que mata:**

```cpp
QSPIHandle::Result QSPIHandle::Impl::EraseSector(...)
    RETURN_IF_ERR(SetMode(Config::Mode::INDIRECT_POLLING));
```

Apagar exige tirar a QSPI do modo memory-mapped. E `qspi.cpp` **não tem
atributo de seção nenhum** — esse código mora na QSPI junto com o resto do
app. Em `APP_TYPE=BOOT_QSPI` o `.text` inteiro vai para `QSPIFLASH`. Chamar
`Save()` é apagar o chão em que se pisa: a próxima busca de instrução vem da
região que acabou de sair do ar. Se funciona, funciona por sorte de I-cache.

**A pré-condição existe e não está escrita em lugar nenhum: `Save()` supõe
que o app NÃO executa da QSPI.** Ela é verdadeira em `BOOT_NONE` (app na
flash interna) e **revogada por `BOOT_QSPI`** — que é uma escolha de
Makefile, feita meses antes, sem nenhum aviso de compilação, nenhum
`static_assert`, nenhuma linha de documentação ligando as duas coisas.

**Regra:** *toda API herdada carrega pré-condições que a descrição não
menciona. Antes de construir em cima, ler a implementação procurando o que
ela SUPÕE do ambiente — e conferir se alguma decisão de build já revogou
isso.* Descrição diz o que a função faz; código diz o que ela exige.

**Onde procurar primeiro:** operações que mudam o modo de um periférico do
qual o código executa, que escrevem em memória de onde se busca instrução,
ou que desabilitam interrupções por tempo indefinido. Essas três têm
pré-condição de ambiente quase sempre.

**Consequência:** o primeiro passo dos presets não é escrever preset nenhum
— é trocar para `BOOT_SRAM` (`.text` vai para SRAM, 480 KB, binário de 180)
e revalidar o que já funciona. A QSPI vira dado puro e a pré-condição passa
a ser satisfeita de verdade, não por sorte.

---

## 12w. Teto é orçamento, prazo é parede — e a diferença salvou a defesa (22/08)

O teto de soma derruba efeito quando N blocos **consecutivos** passam de
850 µs. Histerese deliberada. Medindo com o leitor consertado (12u),
apareceu uma lacuna: em carga alta o pico fica em ~975 µs, **o teto é
ultrapassado em todo pico e `quedas` fica em zero**. A proteção não estava
quebrada — estava mirando carga sustentada, e o fenômeno era pico.

A proposta que chegou para fechar a lacuna: *"M picos acima do **teto** numa
janela T"*. Escrita assim, ela herdaria o defeito da opção que já havíamos
descartado (baixar o teto): **derrubaria efeito o tempo todo numa
configuração que toca limpa.**

**O erro só apareceu ao DIMENSIONAR.** No papel "picos acima do teto" soa
razoável; com o número na mão — pico 975, teto 850, zero falhas — fica
evidente que ultrapassar o teto é rotina benigna.

    TETO  (850 µs) = ORÇAMENTO ADMINISTRATIVO. Onde o CTOS decide que a
                     carga está alta demais para o conforto. ESCOLHIDO.
    PRAZO (1000 µs) = PAREDE FÍSICA. Onde o bloco perde o deadline e o
                     áudio falha de verdade. IMPOSTO pelo hardware.

**Regra, e é vocabulário permanente do projeto:** *confundir orçamento com
parede faz a proteção criar o defeito que deveria evitar.* Toda defesa
precisa declarar em qual dos dois ela mira — e a resposta muda o critério,
não só o número.

O critério final mira o **prazo**: três estouros numa janela de 2 s. Mede
zero em todo regime de bancada, o que é o comportamento certo de uma defesa:
existir e ficar quieta até o fenômeno aparecer.

**Onde o papel encontra o número:** opções discutidas em abstrato podem
carregar a referência errada sem que ninguém note. O dimensionamento não é
detalhe de implementação — é onde a opção é testada.

---

## 12x. O ganho de teste pegou o relatório mentindo, não só a defesa (22/08)

O critério de estouro mede zero em regime real — então, sem ganho, seria
indemonstrável. `ctos prazo <us>` força prazo artificial, espelhando o
`ctos teto` que existe pelo mesmo motivo.

Com `ctos prazo 300` a defesa agiu certo: cinco quedas na ordem declarada
até sobrar o não-sacrificável. **E o relatório mentiu duas vezes:**

| imprimia | era |
|---|---|
| `ESTOUROS DE PRAZO (>1000 us)` | contava contra os **300** artificiais |
| `phaser CAIU pelo teto` | caiu pelo critério de **estouro** |

O primeiro é rótulo com o número errado. O segundo é pior: **carga sustentada
e picos estourando o prazo pedem consertos opostos**, e o relatório os
apresentava como o mesmo evento. Quem lesse aquele log procuraria carga onde
havia pico.

**As duas só apareceram porque o ganho de teste existia** — e apareceram
como efeito colateral de exercitar a *ação*. Se eu tivesse conferido só
"caiu o que devia cair", ambas teriam sobrevivido.

**Regra:** *ganho de teste que verifica só a AÇÃO e não a NARRAÇÃO deixa a
metade mentirosa viva.* Ao exercitar uma defesa, ler o relatório dela com a
mesma desconfiança com que se lê o comportamento — o relatório é a única
coisa que vai existir quando o evento acontecer de verdade, sozinho, num
palco.

**Sub-lição, para o checklist de leitura:** com prazo artificial o `bloco`
caiu de 453 para 246 µs — porque a defesa derrubou cinco efeitos. **Queda de
CPU depois de proteção agindo é consequência, não melhora.** É o gêmeo do
"zero com cara de dado": número bom com cara de vitória. Toda leitura de
carga precisa vir com o retrato do que estava no ar — que o contador de
estouros já grava.

---

## 12y. Trabalho ilimitado em contexto compartilhado — a terceira e a quarta (22/08)

O `Log()` já tinha aprendido isto e ganhado disjuntor: *trabalho ilimitado
em contexto compartilhado é parada cardíaca esperando acontecer.* A lição
estava escrita e **duas outras funções no mesmo arquivo não a tinham**.

**PollMidi drenava sem teto:**

```cpp
while(m_tail != m_head) { ... midi_cb(b); }   // a ISR enche em paralelo
```

Produtor e consumidor no mesmo anel, consumidor sem orçamento. Se os bytes
chegam tão rápido quanto são processados, `m_tail` nunca alcança `m_head` e
a função **nunca retorna**. Nunca disparou na bancada — chegaram zero bytes
no teste — mas é a mesma forma.

**E o rearme da escuta rearmava a cada volta:**

```cpp
if(midi_started && !midi_uart.IsListening()) MidiRestart();
```

Uma defesa que se auto-cura, sem orçamento. Quando a escuta **não consegue
se estabelecer** — WIDI desconectado, linha RX solta — isso vira
moto-perpétuo: `DmaListenStop` + `memset` + `DmaListenStart`, milhares de
vezes por segundo.

**Medido pela caixa-preta: 376.906 rearmes num voo.** A ~10 µs cada, ~3,7 s
de loop gasto rearmando — exatamente a janela que o vigia detectou.

**Regra:** *uma defesa que se auto-cura sem orçamento vira a doença.* Toda
retentativa automática precisa de teto por unidade de tempo E de um contador
do que o teto barrou — senão "está tentando o tempo todo" é indistinguível
de "nunca precisou tentar".

**Meta-lição, e é a que dói:** a regra já existia neste arquivo, aplicada ao
`Log()`, com comentário longo explicando por quê. Duas funções abaixo dela
não a seguiam. **Lição aplicada num lugar não é lição aplicada** — quando
uma regra nasce de um bug, vale varrer os irmãos do mesmo arquivo no mesmo
dia.

---

## 12z. Investigação sem ponto de retorno declarado (22/08)

Ao trocar para `BOOT_SRAM` apareceu um travamento reprodutível. Investiguei
por seis gravações seguidas, cada uma com um conserto novo, **sem nunca
fixar o bom conhecido**. Quando o app parou de enumerar o USB, não havia
para onde voltar sem reconstruir — e a recuperação custou BOOT+RST,
reinstalação do bootloader, e vários RESETs físicos do dB.

O repositório tinha o chão o tempo todo: commit `163ee1d`, v0.123.2. Eu
levei seis gravações para usá-lo.

**Regra:** *antes da primeira gravação exploratória, declarar em voz alta
qual é o bom conhecido e como se volta a ele.* Não é cerimônia: é a
diferença entre "reverto num comando" e "reconstruo às cegas com a placa
muda".

**E o pior defeito do dia foi meu, no próprio instrumento de diagnóstico.**
O vigia do loop deixa recado na SRAM de backup. Pus a leitura desse recado
como **primeira linha do `main`, antes do `hal::Init()`** — e a SRAM de
backup vive num domínio de energia que só é habilitado durante a
inicialização do HAL, com a leitura ainda fazendo operação de cache.

Resultado: a pior falha que este instrumento pode ter. **O app rodava — o
LED batia — e o USB nunca subia.** Placa viva e muda, sem serial para dizer
o que houve, recuperável só por BOOT+RST e reinstalação do bootloader.

É a família "ordem de configuração", já catalogada aqui, reintroduzida no
código que existe **para investigar falhas**. Terceira vez no mesmo dia que
o instrumento de medir vira a falha: o parser que devolvia zero (12u), o
relatório que mentia sobre o próprio critério (12x), e agora o vigia que
derruba o USB.

**Regra:** *código de diagnóstico roda no mesmo hardware que o código de
produção e tem as mesmas pré-condições.* "É só um log", "é só um recado",
"é só um script" são as três frases que precedem as falhas mais caras deste
projeto.

**Sub-lição sobre a janela de 2 s do bootloader:** gravar depois do RESET
falha porque o comando leva mais tempo que a janela. O certo é **esperar o
bootloader aparecer e gravar no instante em que surgir** — polling do
`dfu-util -l`, gravação no primeiro acerto. Funcionou de primeira.

---

## 12. Checklist de retomada a frio (colar no início de qualquer sessão futura)

Ler o [STATUS.md](STATUS.md) e este documento antes de tocar em código. `/gitsync` se a sessão for de arquitetura. Um voo, uma variável — e listar os empregos da variável antes de mexer. Caixa-preta armada por comando no início do voo; sessão de leitura não escreve. Toda medição declara unidade (RMS? pico? norm? real?) e regime (estreia? permanente? que bloco? que clock? -O2 ou -O3?). Módulo novo: description.json → embarque pela receita → medir estreia+regime nos dois blocos → teste audível com o material certo pro efeito. Bug novo: nomear a família antes de consertar. Hipótese nova: registrar a aposta com percentual. Feature nova: o aceite é do músico. **Sintoma repetido pela terceira vez: pare de gerar hipóteses e ache o teste que corta o sistema ao meio** (para canal de entrada, é o loopback). **Zerar o ambiente antes de medir é parte da medição.** **Antes de ler qualquer medição, provar que o canal fala** — um comando de eco conhecido primeiro; zero num canal não confirmado não é dado, é ausência de dado. **Todo modo que silencia ou altera o instrumento tem que se declarar em todo comando de status, e a declaração atravessa o modo que declara.** **Documento vivo atualizado: sincronizar repo E vault na mesma ação** — nunca um destino de cada vez. **Estado de placa afirmado antes de um flash não sobrevive ao flash — reler depois de gravar, ou não afirmar.** **Todo alarme de boot precisa de um gêmeo consultável sob demanda.** **A queixa tem tantas partes quantas o dB nomeou: consertar uma e medir melhora não é consertar a queixa.** **Antes de gastar código numa hipótese, a medição que a mataria** — planura separa ruído de som legítimo. **Profundidade de EQ se escolhe contra régua medida (o sinal seco), nunca de ouvido.** **Toda gravação destrói estado: repor E declarar o que foi reposto faz parte de gravar.** **Ao ser o primeiro caso de uma forma nova, reler a assinatura em vez de imitar o vizinho.** **Valor lido de placa viva não tem procedência — default se lê no fonte.** **Canal de sinal novo entra com o medidor dele junto.** **Antes de misturar canais, perguntar o que cada canal É naquele ponto.** **Canal mudo prova que aquele canal está mudo, e nada além disso** — nunca inferir a saúde de um subsistema pela ausência de resposta de outro. **Nada de logar ou mandar MIDI de dentro do callback de áudio: o callback marca, o loop fala.** **Mecanismo corretivo automático precisa de carência entre agir e medir de novo, ou reage à própria ação ainda em curso.** **Chave nova num recurso compartilhado emudece quem depende dele: declare a dependência onde alguém vai olhar.** **Regra herdada tem pré-condição — verificar se a estrutura que a tornava possível existe no caso novo.** **Validade de medição tem REGIME, e o regime é parte do dado** — temporal (estreia/permanente) E físico (como o sinal está sendo produzido). **Antes de dimensionar uma solução, verificar se o problema existe no regime real** — e fazer primeiro o que serve para os dois desfechos possíveis. **Par assimétrico: troque os elementos de lugar antes de atribuir a assimetria ao sistema** — medir mais fundo só confirma o número, que nunca esteve em dúvida. **Justificativa errada envelhece mal:** registre a razão que se sustenta, não a que estava à mão. **Verificador que casa texto encontra a narração sobre o dado** — case na estrutura da linha de dado. **Controle que não existe mede zero, e zero parece dado:** número redondo demais em itens independentes é sintoma. **Delta entre builds não é atribuível:** custo se mede por A/B no MESMO binário, alternando por parâmetro. **Param sem leitor é feature morta até prova em contrário** — a checagem é textual e barata, e é a única que encontra o que o compilador não vê. **Feature morta parece feature desligada, e quando o default é "desligado" o estado normal É o estado quebrado.** **Efeito que trava um param internamente: a faixa declarada TEM que bater com a trava** — faixa maior é curso morto, e ninguém detecta, porque o defeito vive entre dois arquivos que estão certos. **Conserto que vira nota local reincide; conserto que vira pergunta de rotina, não.** **Refactor cria controle morto, não só código morto: depois de mudar a topologia, perguntar o que deixou de ser NECESSÁRIO.** **Variável ganha o segundo emprego quando alguém precisa de um valor e há um por perto — pergunte se o param já responde a outra coisa.** **Antes de dizer "não é possível", separar o que é LEI do que é DECISÃO minha** — argumento bom sobre premissa não examinada convence os dois lados. **Custo medido numa arquitetura não se transporta para outra.** **Para remover um subsistema, apague o símbolo-raiz e deixe o compilador enumerar o resto.** **Depois que ele calar, ainda falta o que não vê: params sem leitor, prosa sem assunto, e verificações sem objeto — esta última PASSA e ninguém desconfia de um `ok`.** **Deleção por faixa calculada: o perigo é proporcional ao que não é referenciado por nome** — o compilador testemunha símbolos e nada mais. **Casar → imprimir o que casou → conferir linhas → só então apagar.** **Recuperar um bloco não é restaurar um arquivo:** `git show HEAD:arquivo` + recolagem, nunca `checkout` com trabalho não commitado. **Contar errado para o lado autocrítico é o mesmo defeito com sinal trocado.**

---

*"Sem medir antes de concluir, o freeze teria sido condenado pelo número errado, o vocoder descartado pelos 234 ms de estreia, e o warm-up desenhado como uma chamada só — que não resolveria nada."* — o parágrafo que resume tudo, escrito pelo Code no fechamento da fase 1 e provado verdadeiro em cada capítulo desde então.
