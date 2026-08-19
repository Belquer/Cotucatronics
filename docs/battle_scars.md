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

## 12. Checklist de retomada a frio (colar no início de qualquer sessão futura)

Ler o [STATUS.md](STATUS.md) e este documento antes de tocar em código. `/gitsync` se a sessão for de arquitetura. Um voo, uma variável — e listar os empregos da variável antes de mexer. Caixa-preta armada por comando no início do voo; sessão de leitura não escreve. Toda medição declara unidade (RMS? pico? norm? real?) e regime (estreia? permanente? que bloco? que clock? -O2 ou -O3?). Módulo novo: description.json → embarque pela receita → medir estreia+regime nos dois blocos → teste audível com o material certo pro efeito. Bug novo: nomear a família antes de consertar. Hipótese nova: registrar a aposta com percentual. Feature nova: o aceite é do músico. **Sintoma repetido pela terceira vez: pare de gerar hipóteses e ache o teste que corta o sistema ao meio** (para canal de entrada, é o loopback). **Zerar o ambiente antes de medir é parte da medição.** **Antes de ler qualquer medição, provar que o canal fala** — um comando de eco conhecido primeiro; zero num canal não confirmado não é dado, é ausência de dado. **Todo modo que silencia ou altera o instrumento tem que se declarar em todo comando de status, e a declaração atravessa o modo que declara.** **Documento vivo atualizado: sincronizar repo E vault na mesma ação** — nunca um destino de cada vez.

---

*"Sem medir antes de concluir, o freeze teria sido condenado pelo número errado, o vocoder descartado pelos 234 ms de estreia, e o warm-up desenhado como uma chamada só — que não resolveria nada."* — o parágrafo que resume tudo, escrito pelo Code no fechamento da fase 1 e provado verdadeiro em cada capítulo desde então.
