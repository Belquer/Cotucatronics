# Battle Scars — Cotucatronics
<!-- doc-check: ignore lib_id (campo do formato .kicad_sch, nao param) -->
<!-- doc-check: ignore lib_symbols (seccao do formato .kicad_sch, nao param) -->
<!-- doc-check: ignore nogueira_6p4 (perfil de dentes da caixa (6,4 mm), nome de configuracao) -->
<!-- doc-check: ignore acrilico_3p0 (idem, para acrilico de 3,0 mm) -->
<!-- doc-check: ignore confere_furos (portao em tools/, fora do alcance do check_docs) -->
<!-- doc-check: ignore confere_caixa (idem) -->
<!-- doc-check: ignore confere_acesso (idem, vive no pcb_contorno.py) -->
<!-- doc-check: ignore silk_over_copper (codigo de violacao do DRC do KiCad) -->
<!-- doc-check: ignore route_l_a  (nome ANTERIOR da matriz, renomeado em 02/09 para `route_a_l` — entrada primeiro, saida depois. A mencao aqui e o REGISTO do que se aprendeu com o nome velho; apaga-lo apagava o que explica a cicatriz.) -->
<!-- doc-check: ignore route_l_b (idem) -->
<!-- doc-check: ignore route_r_a (idem) -->
<!-- doc-check: ignore route_r_b (idem) -->
<!-- doc-check: ignore in_gain_a (REMOVIDO em 28/08 — cicatriz 15v. O nome aparece aqui porque a cicatriz E sobre a remocao dele; apaga-lo apagaria o assunto.) -->
### O que o firmware nos ensinou enquanto a gente o ensinava a cantar
*Registro de aprendizados — resultados e metodologia. Atualizado ao fim da caçada do shimmer ardido e dos dois controles de I/O.*

---

## 0. Por que este documento existe

O Cotucatronics saiu do zero até um instrumento com delay, cross-synthesis espectral com portadora própria, stack anti-feedback completo e randomização por transiente ("Búzios") em poucas semanas de bancada. No caminho, cometemos e catalogamos dezenas de erros — e o padrão mais valioso que emergiu não foi nenhum truque de DSP: foi **uma disciplina forense de bancada**. Este documento registra as cicatrizes para que os próximos módulos, esculturas e colaboradores herdem as lições sem pagar o preço de novo.

Personagens: **dB** (design de instrumento, patches Max, ouvido e decisões musicais), **Claude Code** (compilação, gravação, medição em hardware), **Claude chat** (arquitetura, hipóteses, prompts, auditoria externa via /gitsync). O triângulo funciona porque cada vértice pode vetar os outros dois com dados.

---

## 1. As três famílias de bug de ordem

Todos os bugs graves de firmware desta fase tinham **lógica correta e ordem errada**. Eles caem em três famílias, e distinguir a família importa porque o conserto difere.

**Família A — Corrida de contexto.** Dois contextos concorrentes (callback de áudio vs loop principal vs ISR), com a proteção armada num e a condição acontecendo no outro. Casos: despacho serial rodando na ISR (travava a placa no comando `list`); a janela de graça armada no loop enquanto o bloco caro rodava no callback (duas vezes). Conserto: **mover o armar para junto do checar — mesmo contexto, sempre.** Corolário: toda flag que cruza callback↔loop é `volatile` (ou atomic).

**Família B — Ordem de configuração.** Sem concorrência nenhuma; sequência errada dentro de uma função só. Engana mais justamente porque o código lê certo de cima pra baixo. Casos: `bind[]` preenchido antes do `Add()` dos params; `RequestBlockSize` depois do `Init()` dos módulos (módulos preparados pra bloco 48 recebendo 256 — estouro de heap silencioso rodando por baixo de medições que pareciam limpas). Conserto: **quem configura vem antes de quem consome.** Risco de confundir com a Família A: leva a "sincronizar" o que só precisava ser reordenado — resposta cara pra problema barato.

**Família C — Variável com dois empregos.** Uma coisa servindo a dois propósitos, e um teste (ou conserto) mexendo nos dois de uma vez. Casos: o `DelayMs(1)` era relógio do loop **e** metrônomo da rampa — removê-lo para testar o relógio destruiu a rampa junto, contaminando a conclusão auditiva do voo; o `@title`/classname default servindo a todos os exports até colidir. Conserto: **antes de mexer numa variável, listar todos os empregos dela.** No desenho experimental: um voo, uma variável — e conferir que a variável é mesmo uma.

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

**Pico decide, média mente.** Um módulo de FFT: média 7,8%, pico 210%. Custo FFT concentra no bloco do hop (1 a cada N). O watchdog julga o pico. Corolário do bloco: dobrar o bloco dobra o prazo mas o pico sobe junto (~19%) — **dobrar o bloco não dobra a folga.**

**`-O3` piorou.** Binário +13%, média de CPU +15% — app executando da QSPI paga I-cache miss por byte de código. **Nesta plataforma, código menor é código mais rápido.** Regra registrada: ao reverter, remedir tudo (todos os números têm -O3 como base).

**Fixar parâmetro no Max não remove o DSP.** Cirurgia de "fixar valores": 12% menos código, **0% menos CPU**. Parâmetro congelado é alavanca colada, não motor desligado. Corte de custo exige **deletar objetos de sinal**. Rota preferida para módulo pesado: construir do piso e devolver luxo por luxo com etiqueta de preço — não desbastar patch rico.

**Frequência bater com suspeito não é identificação.** Duas fontes distintas produziam 1 kHz; a conclusão inverteu duas vezes até o teste que **remove** uma fonte separar as duas. Identificação exige manipulação, não coincidência.

**Suspeitar não é medir.** O estouro de heap (Família B) "provavelmente não distorcia o tempo de FFT" — e não distorcia. Mas só a remedição completa permitiu afirmar. Números publicados com defeito ativo por baixo foram **invalidados e remedidos**, mesmo os que estavam certos por sorte.

**Teste que passa merece a mesma desconfiança que teste que falha.** O bug do zsh (word-splitting) deixou um teste "passar por acaso" no pino 18; só o silêncio inesperado no pino 19 o expôs. A expectativa violada revela o que a expectativa confirmada esconde.

**RMS ≠ pico.** O medidor reportava RMS; senóide tem pico 3 dB acima. −1,1 dBFS RMS = ADC da interface clipando — e o governor pareceu errado por causa do instrumento de medição, não do firmware. Conhecer a unidade do próprio medidor é pré-requisito de qualquer conclusão.

**O assassino confirmado por protocolo.** `hal::DelayMs(1)`: quatro voos, mesma instrumentação, **uma variável** — 1.505 voltas/morte aos 12,6 s com ele; 8,7 M de voltas/vivo sem ele; religado, morte de novo; removido, vivo de novo. Discriminante e conserto no mesmo protocolo. É o padrão-ouro: quando a hipótese sobrevive à reintrodução do suspeito, ela vira fato.

---

## 4. Cultura de hipótese (como erramos melhor)

Hipóteses erradas com testes certos resolveram o projeto. O diagnóstico "cycle~ a 0 Hz por eventos não propagados" errou o mecanismo exato — mas o **teste de um comando** (um `set` de uma nota) que ele gerou apontou a região certa e destravou tudo. **Teste bom sobrevive a hipótese ruim.** Registrar apostas com percentuais ("60% relógio, 30% instrumentação, 10% resto") mantém o placar honesto e ensina a calibrar.

Derrubar a própria hipótese com dado é vitória, não derrota: o M1==M2 derrubou a hipótese da prisão no line_cb — e depois o furo do M1==M2 (armamento no meio da linha) a reabilitou — e depois o token a derrubou de vez. Três reviravoltas, todas por medição, nenhuma por teimosia.

Quando as hipóteses acabam, **pedir olhos novos com um relatório estruturado**: contexto, medições contraditórias, o que já foi descartado *com o dado que descartou*, e perguntas numeradas. Os melhores destravamentos da caçada vieram de relatórios assim.

---

## 5. Fronteiras de arquitetura (o que nos salvou repetidamente)

**HAL:** includes de fabricante só em `hal_<board>.cpp`. Trocar de cérebro = um arquivo + uma entrada de Makefile. Auditado em clone limpo múltiplas vezes; nunca violado.

**Módulos:** headers do motor externo só nos `.cpp` de módulo; o `main` fala com `Module`, nunca com motor. A fronteira fechou inclusive a porta dos fundos (as estatísticas de alocação passaram a sair por `Module::Memory()`).

**Portão único da SDRAM:** `AllocBig()` é o único acesso; `DSY_SDRAM_BSS` proibido fora do HAL. O linker passou a **auto-aplicar** a regra (pool reivindica a região inteira; clandestino nem linka).

**Manifest = identidade do instrumento.** Params, macros, requisitos, bloco, cercas de sorteio, máscara de escala, mapa MIDI — tudo dado, nada código. Trocar escultura = trocar manifest + módulo. `Requirements` conferidos no boot: cérebro insuficiente recusa com log claro, nunca bug misterioso no palco.

**Get/Shaped/Real — três consumidores, três moedas.** Knob lê `Get` (norm cru), módulo lê `Shaped` (norm curvado), humano lê/escreve `Real` (unidades). O bug do push com `Get` (curva invisível pro DSP) e o do `set` normalizado (bench_boost "1.0" entregando 8×) nasceram de misturar moedas. A borda serial fala unidade humana; o miolo fala norm; a conversão vive na borda.

**Repo versiona o instrumento; bancada é estado transitório.** O default do repo é o que a placa É quando alguém clona e grava. Modo bancada vira *receita documentada*, nunca estado commitado.

**Clone limpo é o teste.** O cabeçalho de alocação do módulo, nunca commitado, ("funciona na minha máquina" por semanas) gerou o `make check` — que **na primeira execução** pegou outro defeito (export do freeze não rastreado). Hábito automatizado vence hábito lembrado; a prova é que a checagem manual recém-escrita não teria pego o segundo caso.

---

## 6. Patch embarcado — o manual das cicatrizes

**Domínios evento vs sinal são a fonte nº 1 de bug de patch.** A régua: *olha a boca de quem come.* Freq de `cycle~` (nesta versão) = evento (param → expr → cycle~, direto). Ganho de `*~` = sinal (param → `sig~` → inlet). `delay~` (sinal) ≠ `delay` (evento). Operadores que alimentam entradas de sinal precisam ser variante `~`. Feedback fecha **só** com `feedback~` no ciclo. Na dúvida: autocomplete descreve, help patcher demonstra — copiar exemplo vivo venceu três vezes.

**Memórias:** `data @external 0` tranca como interna; `buffer~` é externo *por natureza* (o @external não pega). Receita oficial: **janelas matemáticas são do host** (firmware aloca via AllocBig, gera sqrt(hann), entrega via dataref, lendo o tamanho do próprio dataref); **memórias de estado são do patch** (`data @external 0`). Dataref se resolve **por nome**, nunca por índice (reexport reordena). `poke~`/`peek~` referenciando nome inexistente cria buffer externo fantasma no Max — funciona lá, sai mudo na placa.

**Custo:** todo `buffer~` custa **8 B/amostra** (Float64Buffer, ignora a macro de float32) + arredondamento pra potência de 2 — estimativa ingênua erra por até 4×. Patch grande mata por **tempo**, não memória (freeze: 2% da SDRAM, 275% do prazo). Custo FFT concentra no bloco do hop.

**Embarque:** o runtime de desktop é inutilizável — o export entra como fonte, com um motor mínimo. As macros de configuração têm de vir **antes** dos includes: chegar depois é o mesmo que não existir, e a falha é silenciosa. Alocador híbrido (pool SRAM → AllocBig), free no-op, memória vitalícia. **O motor precisa ser dirigido:** `processEventsUntil()` + `advanceTime()` por bloco — sem isso, relógio preso em 0, nenhum evento executa, patch com portadora interna sai mudo. **Warm-up no Init** (política de slot, ~4096 amostras de silêncio) paga a estreia no boot. **Re-kick de params pós-warm-up** (reenviar todos os valores com o motor vivo) mata a classe "estado inicial que não propagou".

**Conferência pré-embarque do `description.json` é obrigatória e já pagou três vezes:** datarefs externos inesperados, params faltando (um param de mistura sumido), saída mono onde o slot espera par, classname default. Ler o JSON custa um minuto; um módulo mudo custou horas.

**Cross-synthesis é peneira:** só devolve energia onde molde e portadora coincidem. Senóide × drone esparso ≈ silêncio (e "sem diferença perceptível" que não era bug). **Banda larga é a régua de cross-synthesis; senóide é a régua de delay e shifter.** Cada efeito tem seu material de prova.

---

## 7. O loop principal sob carga (a caçada do DelayMs)

Com a carga a 65% do bloco, o mundo do loop principal muda de clima e três verdades ficaram gravadas:

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

Cada transiente na escultura é uma jogada; o que cai decide o timbre. `dly_time` sorteia em degraus de 50 ms **em unidade real** (a curva 2.5 entortaria sorteio uniforme em norm — mais uma vez, a moeda certa no lugar certo); a carta de nota do Fx2 sorteava nota a nota **sempre em Lá menor natural** (máscara de escala é dado de manifest; a base 110 Hz já é Lá, então a máscara nasce afinada); knob manual segue cromático — *a máquina se comporta, a mão pode tudo*. Cerca de segurança inegociável: level, gate, governor, limiter, routing e src_mix jamais sorteáveis. Aceite: **1 minuto de taps, 74 transientes, 74 sorteios — razão 1:1, julgada pelo músico.** O critério final de qualquer feature é o ouvido de quem toca; caixa-preta prova encanamento, não flor.

---

## 10. O dia 17/08 — quatro cicatrizes

**Estado de diagnóstico que sobrevive ao diagnóstico.** O `level` ficou em 0.4 desde o A/B com o Max e virou default do repo por três commits, junto com gate e governor desligados. Pior que o descuido: eu **documentei a violação no STATUS como temporária** e segui em frente. Registro que se sabe errado e não conserta não é aviso — é permissão. O dB só descobriu indo tocar o instrumento real. É prima da instrumentação-que-apaga-o-dado, mas pior: aquela estraga a medição, esta estraga o instrumento, e some do radar justamente por estar anotada. **Estado de bancada tem prazo, e o prazo é a mesma sessão.**

**Regra que sobrevive ao motivo.** O `fx2_on` nascia em bypass pra proteger de um risco que a medição já tinha eliminado — o módulo em 37% do prazo, watchdog funcionando, morte do loop com outra causa já consertada. A regra continuava cobrando preço (o dB encontrou "só o delay" seis vezes num dia) por um perigo que não existia mais. E o custo era invisível de um lado e óbvio do outro: **quem escreve a regra não é quem aperta o RESET.** Regra de segurança precisa de data de revisão, senão vira superstição.

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
`dly_mix` e as duas cartas do Fx2 **continuamente** durante os testes de
MIDI — o instrumento se modificava sozinho enquanto eu media outra coisa.
É a prima do `tone on` esquecido durante a medição do reverb. **Zerar o
ambiente antes de medir é parte da medição.** (E de brinde: 0.04 dispara
com o chão de ruído do piezo, sem ninguém tocar — dado que só apareceu por
acidente.)

---

## 11b. O conserto que quase sobreviveu ao problema (18/08)

Com o slot Fx2 vazio, o dB ouviu o instrumento e disse: **"o som está
com uma presença de grave que nunca teve antes, muito melhor sonoramente."**

A causa está em uma linha de escopo. O passa-alta de 120 Hz vive **dentro**
do bloco do Fx2, e `fx2_want` exige um módulo no slot:

```cpp
const bool fx2_want = fx2 && params.Real(manifest.p_fx2_on) >= 0.5f;
```

Slot vazio ⇒ bloco pulado ⇒ **filtro não existe mais**. O grave do piezo
passa inteiro.

**Por que isso é um acerto e não sorte.** Aquele filtro nasceu pra tratar um
defeito *do módulo do Fx2* — o estouro de grave que só acontecia com ele
ligado. O estouro se **ouvia na saída**, então o lugar óbvio pra filtrar era
o master. A decisão foi outra, e está escrita no manifest: *"NO CAMINHO DO
FX2, não no master: sem módulo não estoura."*

Se tivesse ido pro master, ele teria sobrevivido à saída do módulo e o
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
causa do módulo do Fx2. A diferença é instrutiva: **aquele saiu sozinho porque
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
**sete** features aposentadas, e cada uma deixou rastro — o módulo do Fx2 deixou
params vivos e CCs mortos, o freeze externo deixou um wrapper, a regra do
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
módulo externo opaco**, que não separa as duas coisas. Pular o `Process()` zera o custo
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

## 12d. Delta entre builds não é atribuível (20/08) — e nesta placa pode ser 25% (23/08)

### A atualização de 23/08, que sobe a regra de grau

Em 20/08 a lição foi *"delta entre builds não é atribuível"*. Em 23/08 mediu-se
**quanto** ele pode valer aqui, e o número muda o que a regra exige.

Duplicar dez params de caráter, com o **mesmo regime e o mesmo trabalho** nos
três binários:

| binário | o que mudou | pico médio |
|---|---|---|
| v0.130.0 | (linha de base) | **460,4 µs** |
| v0.131.0 | dez params duplicados | **437,0 µs** |
| v0.132.0 | **só** um `? :` por bloco | **347,0 µs** |

A v0.132.0 **acrescentou** trabalho e ficou 90 µs mais rápida. Entre a
primeira e a última: **113 µs, 25%, sem que o trabalho mudasse.**

O A/B no mesmo binário, alternando a cada leitura, deu o valor verdadeiro da
duplicação: **+6,6 µs contra 14 µs de espalhamento — indistinguível.**

### O mecanismo, que é desta placa e não genérico

**O código roda da QSPI.** Instrução buscada de flash externa através de
cache: alinhamento de linha e layout de função deixam de ser detalhe de
terceira ordem e viram **variável de primeira ordem**. Qualquer edição
desloca funções, e um laço quente que cruzava uma linha de cache pode passar
a caber nela — ou o contrário.

> **Nesta placa, o delta entre builds pode ser 25% sem nenhuma mudança de
> trabalho. Todo número de CPU citado sem A/B no mesmo binário é ruído com
> aparência de resultado.**

### O que isso explica retroativamente

Casos que ficaram sem culpado ganham um suspeito com mecanismo:

- o **638 vs 620 µs** da unificação de params;
- os **"40% mais rápido"** do BOOT_SRAM — que agora têm explicação
  concorrente: código em SRAM **elimina esta variável**, e parte do ganho
  pode ser a remoção da variância, não velocidade. Não muda a reprovação do
  BOOT_SRAM; muda o que se pode afirmar sobre o ganho dele.

### A consequência prática

**Todo ganho de teste que permite A/B no mesmo binário vale o código que
custa e fica no firmware.** O `ctos teto`/`ctos prazo` já eram assim; o
`dup on|off` entrou pelo mesmo motivo, e se declara em `board` porque
`dup off` altera o instrumento.

Medir custo aqui sem uma chave em tempo de execução não é medir com menos
precisão — **é não medir.**

---

### O caso original (20/08)

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

Tirar o motor externo do projeto era remover 56 sítios espalhados por três
arquivos,
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

- **params órfãos** (`fx2_gain`, `fx2_hp`…) — existiam na tabela e ninguém
  os lia. Só apareceram quando removi as declarações e o compilador reclamou
  do único consumidor restante, que era um histograma que media um módulo
  fora do build havia dois dias.
- **prosa** — comentários explicando um sistema que não existe mais. Nenhum
  compilador reclama de um parágrafo órfão.
- **verificações sem objeto** — o `check_repo.sh` tinha um bloco inteiro
  conferindo exports que já não existiam. Passava sempre, e ia continuar passando para
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

Os **67 µs da base** são o dado que interessa: o módulo externo rodava **ligado ou
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

## 13a. Guarda de faixa em float se escreve pela NEGAÇÃO (22/08)

`delay.h:117` tinha `while(r < 0.f) r += (float)cap_;` — normalizar um
ponteiro de leitura circular. Fui consertar **um** defeito e encontrei
**dois** dormindo na mesma linha, e o segundo era o pior.

    atraso_ = +Inf  ->  r = -Inf  ->  -Inf + cap_ = -Inf
                        o laco NUNCA TERMINA. Loop morto, audio seguindo:
                        "placa viva e muda", o sintoma mais caro daqui.

    atraso_ = NaN   ->  `NaN < 0` e FALSO, sai na hora, e `(size_t)NaN`
                        e INDEFINIDO — indexa fora do buffer em silencio.

O laço eterno era o que eu tinha visto. O índice fora do buffer estava
invisível e é o mais grave — corrupção silenciosa contra travamento
barulhento.

**O conserto tem uma forma, e a forma é o ensinamento:**

```cpp
if(!(r >= 0.f && r < (float)cap_)) r = 0.f;   // pega NaN de graca
```

**NaN falha TODA comparação.** Então:

| forma | com NaN | resultado |
|---|---|---|
| `if(r >= 0 && r < cap)` — direta | falso | **NaN passa como se estivesse dentro** |
| `if(!(r >= 0 && r < cap))` — negada | verdadeiro | o clamp morde |

A assimetria da IEEE 754 vira ferramenta: perguntar *"está dentro?"* deixa o
NaN escapar; perguntar *"não está comprovadamente dentro?"* o pega de graça,
sem `isnan()`, sem custo.

**Regra, para a P7 do audit:** *toda guarda de faixa em float que protege um
índice se escreve pela negação.* Qualquer `if(x >= lo && x < hi)` guardando
acesso derivado de float é candidato a ter a forma invertida — e o projeto
acabou de pagar para saber por quê.

**Sub-lição do método:** o segundo defeito só apareceu porque li o contexto
antes de tocar. Se eu tivesse ido direto trocar o `while` por um `if`,
teria consertado o travamento e deixado a corrupção silenciosa em pé — o
conserto parcial que parece completo é pior que nenhum, porque fecha o
sintoma que denunciava o resto.

**Aceite, e ele tambem tem forma:** varredura ponta a ponta (50 ms a 1 s,
tom nas duas cadeias) com o contador em **ZERO**. Defesa de proteção se
aceita provando que **não morde o legítimo**; que ela morde quando deve se
prova por gancho de teste, como o `ctos prazo` fez.

---

## 13b. Três causas empilhadas, e cada uma escondia a seguinte (22/08)

Amplificador novo na bancada — TPA3110 alimentado por boost, exciter de 4 Ω.
Sintoma inicial: som muito baixo. Levou uma tarde, e a lição não é elétrica,
é de método.

**Causa 1 — chave de potência no caminho do sinal.** A saída da Daisy passava
por uma chave de três posições marcada para 12 V. Contatos feitos para
potência são de prata ou estanho e criam película de óxido; tensão e corrente
altas **furam** essa película, e é assim que a chave funciona na aplicação
para que foi feita. Áudio de linha — ~1 V, microampères — não fura nada. **A
chave fechava mecanicamente e continuava aberta para o sinal.** Chama-se
comutação em circuito seco, e o sintoma foi exato: entrada flutuando, muito
ruído branco, seno ausente.

**Causa 2, que só apareceu depois de consertar a 1.** Com o sinal barrado, o
amplificador ficava em vazio — e por isso a fonte "segurava" 13 V firmes sob
o que parecia carga. Assim que o sinal passou, o boost caiu para 6 V. A
medição que descartava a fonte estava certa e era **inútil**, porque o regime
que ela mediu não era o de uso (12a outra vez, agora em elétrica).

**O capacitor foi útil por NÃO resolver.** 2200 µF / 35 V nos terminais do
amplificador não mudou nada — e isso é dado: capacitor empresta energia para
**picos**; se a tensão continua caindo, o que falta é corrente **média**, e
nenhum capacitor fabrica potência. O teste discrimina sozinho.

**Causa 3 — o boost.** Fonte de 12 V direta resolveu. Uma conversão a menos,
e potência que se entrega em vez de potência de catálogo.

**Regra de hardware, e vale para o painel inteiro:** *sinal de áudio não
passa por contato mecânico em lugar nenhum deste instrumento.* Chave governa
<!-- doc-check: ignore route_exciter (nome APOSENTADO em 28/08, hoje `out_l`. A mencao e deliberada: e o registo da renomeacao, e apagar o nome velho apagaria justamente o que explica por que ele estava errado.) -->
<!-- doc-check: ignore route_jack (idem, hoje `out_r`) -->
**GPIO**; o firmware corta o ramo. `route_exciter` e `route_jack` existem
para isso, e as DPDT planejadas em D29/D30 seguem o mesmo desenho.

> **Corrigido em 28/08, e a correção é do enunciado, não do caso.** A regra
> generalizou demais: ela nasceu de uma chave de POTÊNCIA, cujo contato seco
> não conduz nível de linha. O DPDT de sinal de três posições
> (jack \| ambos \| amp) conduz áudio DE PROPÓSITO, e é o que está montado.
> O que o caso realmente ensinou é mais estreito: **chave de potência não
> serve para comutar sinal.** Os params passaram a chamar-se `out_l`/`out_r`
> — o firmware nomeia SAÍDA, o cobre nomeia destino.

**Regra de método:** *causa consertada muda o regime e pode revelar a
próxima.* Uma medição feita antes do conserto pode ter medido um sistema que
não existe mais — e "já descartei a fonte" era verdade sobre o sistema
quebrado, não sobre o consertado. **Ao consertar, refazer as medições que o
conserto tornou obsoletas.**

**E o firmware ficou inocentado por telemetria, não por fé:** oito leituras
seguidas do `env` do governor deram 0.142 a 0.143, variação de 0,001, com
`gain` em 1.00. Enquanto o operador ouvia pulso, a placa entregava uma reta.
Foi isso que apontou o dedo para fora dela.

---

## 13c. Prosa de status é medição também — as quatro do mesmo dia (22/08)

Quatro vezes em 22/08 o instrumento de medir virou a falha. Só no fim ficou
claro que as quatro têm a **mesma anatomia**.

| onde | o que a linha dizia | o que era |
|---|---|---|
| parser de bancada | `pico 0 µs` | o regex não casou e o código fez `else 0` |
| relatório da defesa | `ESTOUROS (>1000 us)` | contava contra os 300 artificiais |
| anúncio no chat | "CC 42 no fio" | a chamada só LEU; a placa seguia no 41 |
| script de bancada | `placa viva: sim` | **string fixa**, nenhuma verificação |

Em todas, o relatório tem **duas camadas** — o dado e a **moldura** do dado —
e a moldura foi escrita como decoração enquanto só o dado foi tratado como
código. Rótulo, unidade, condição, afirmação de estado: nenhum deles foi
verificado, porque nenhum deles *parecia* um número.

**Regra:** *toda linha de relatório é uma afirmação verificável, ou é
proibida.* `placa viva: sim` só pode existir como resultado de um ping com
timeout. Se não houver o ping, a linha não existe — não vira "provavelmente",
não vira texto otimista.

É a 12u generalizada: de **número** dito de passagem para **frase** dita de
passagem. Prosa de status é medição, e mente com a mesma eficácia — pior,
com mais autoridade, porque ninguém revisa uma frase em português como
revisa um número.

**E "é só um script de bancada" já tem ficha:** é a frase que precede, agora
com quatro ocorrências documentadas no mesmo dia. Junto com "é só um log" e
"é só um recado" (12z), completa o trio.

---

## 13d. O tamanho da seção é a prova de que o atributo foi aplicado (22/08)

A rotina de escrita na QSPI precisa executar da ITCM — é o mecanismo inteiro.
Marquei `COTUCA_ITCM` nos dois ajudantes e **esqueci na função principal**.
Ela rodava da QSPI e se matava na instrução seguinte a desligar o periférico:
travamento total, dois RESETs físicos, uma reinstalação de bootloader.

**E o número estava na tela antes de tudo isso:**

    ITCMRAM:  68 B de 64 KB      <- pouco demais pro que eu tinha escrito
    ITCMRAM: 212 B de 64 KB      <- depois do conserto, tres vezes maior

O linker imprime isso **a cada compilação**, de graça. Eu li "a seção existe"
e parei ali, quando o que importava era "a seção tem o tamanho certo".

**Regra:** *quando o atributo de seção é o mecanismo, o tamanho da seção é a
prova de que ele foi aplicado.* Seção do tamanho errado é atributo
não-aplicado até prova em contrário — e a verificação é grátis, no output do
build.

Mesma família do `.map` que pegou o `.bss` indo parar na DTCM: **o linker
fala; o custo é ler.** Nas duas vezes a informação estava impressa e eu segui
adiante.

Ficou escrito no cabeçalho do `qspi_raw.h` como parte do aceite, porque é lá
que alguém vai acrescentar a próxima função e esquecer o atributo.

---

## 13e. O operador não consegue agir dentro de uma janela que só aparece depois dela começar (22/08)

Precisava contar quantas alternâncias cinco batidas no arcade produziam. Fiz
o óbvio: abri uma janela de 20 s, imprimi *">>> BATA 5 VEZES AGORA <<<"* e
contei pelo log.

Resultado: **zero**. E não porque o botão falhou — porque a mensagem só
aparece na tela **depois** que a chamada começa, e a chamada é a janela. O dB
leu o pedido quando os 20 s já tinham passado.

Isso aconteceu **repetidamente** ao longo do dia: no teste do MIDI de volta,
na varredura de frequência, no aceite do espelho, e aqui. Toda vez que
escrevi "faça X agora" dentro de uma chamada de ferramenta, o "agora" era um
instante que o operador nunca viu.

**Regra:** *nunca peça uma ação humana de dentro de uma janela cronometrada.*
As duas formas que funcionam:

1. **Anunciar, encerrar o turno, medir no turno seguinte** — o operador age
   quando quiser e a leitura não depende de timing.
2. **Contador em vez de janela** — o instrumento acumula, e a leitura
   acontece quando alguém pedir. Foi o que resolveu: `freeze fisico: N
   batidas desde o boot`, legível a qualquer momento.

A segunda é melhor quando o fenômeno se repete, porque tira a coordenação do
caminho crítico inteiro. **Coordenação por cronômetro entre dois agentes que
não compartilham relógio é a forma mais cara de medir qualquer coisa.**

**E a paridade não substitui o contador:** cinco e sete batidas terminam no
mesmo estado. `frz_on_a = 1.00` era compatível com debounce bom E com
debounce ruim — só a contagem distingue.

---

## 13f. Sentinela negativa não sobrevive a zero-init — a segunda no mesmo dia (22/08)

Generalizei o botão assinalável para uma tabela: cada botão declara o
parâmetro que alterna e o LED que o espelha, com `-1` significando "sem
alvo".

```cpp
BotaoAlvo botao[kMaxBotoes] = {};   // zero-init: param = 0, led_hw = 0
```

**Zero é índice válido.** O botão randomizador e o de rearme, que não deviam
ter alvo nenhum, passaram a apontar para o parâmetro 0 (`level`) e para o
LED 0 (o do freeze A). Apertar o randomizador alternaria o volume geral.

O conserto é inicializador explícito no `struct` — `int param = -1;`.

**Segunda vez hoje.** A primeira foi `SacrifTravado()` devolvendo `-1` para
"não travado" e sendo testada como booleano, onde `-1` é verdadeiro e `0`
(um índice válido) é falso. Mesma raiz: **um valor que significa "nenhum"
convive no mesmo tipo com valores que significam "o primeiro".**

**Regra:** *sentinela negativa exige inicialização explícita E teste
explícito.* `= {}` não a respeita, e `if(x)` não a testa. As duas metades
falham em silêncio e por motivos opostos.

**O que pegou:** o relatório do `board`, na primeira leitura — `B1 (?):
alterna level | LED 0`. Instrumento que declara o próprio estado cobra o
preço dele imediatamente, e este cobrou antes de o defeito chegar ao painel.

---

## 13g. GPIO troca no fonte; UART, não (22/08)

O LED do freeze foi soldado no D27 e o switch no D24 — invertidos em relação
ao que o firmware esperava. Custou dois números no fonte e uma gravação:
**GPIO não tem preferência de pino**, e refazer solda custa tempo e risco de
contato ruim.

Minutos depois, o MIDI de volta não chegava ao iPad e a resposta foi outra:
`D13` e `D14` são `PB6` e `PB7`, as pernas da **USART1**. A função está
amarrada ao pino pelo silício — não existe "transmita pelo D14", porque o
D14 *é* a entrada do periférico.

**Regra:** *antes de propor troca no firmware em vez de refiação, perguntar
se o pino é livre ou amarrado a periférico.* GPIO puro se remapeia; UART,
SAI, ADC e QSPI não. A diferença não aparece no esquema — aparece na tabela
de funções alternadas do chip.

**E eu li um contador errado no meio disso.** `voltaram 0` foi tratado como
prova de que a placa não transmitia — mas esse contador mede o que **retorna**
pelo fio, o que depende de o iPad ecoar. Zero ali era esperado, não sintoma.
O `midiloop` com o MIDI Wrench aberto resolveu em trinta segundos o que a
inferência errada tinha desviado por meia hora: **as mensagens chegavam**, e
o problema era o widget não escutar o canal 2.

---

## 13h. Toda reestruturação tem três frentes; o compilador cobra uma (22/08)

Audit de deriva doc↔código, e a causa raiz é comum aos doze achados: o
commit `3834cea` — cauda paralela, dois espaços, freeze por cadeia — foi
reestruturação grande. **O código migrou mais completamente que a
documentação e que os símbolos.**

O compilador cobrou os símbolos que **quebraram**. Ninguém cobrou os que
sobreviveram sem uso, nem a doc que continuou descrevendo o desenho morto.

**Fantasmas na doc** — nome citado, param inexistente. O `frz_hold` descrevia
"fonte e gesto separados" havia meses, enquanto o `manifest.h:264` já
registrava a decisão contrária. `frz_mix` virou `frz_vol`. `rev_type` foi
entregue de outra forma. `frz_release` é proposta legítima e era
**indistinguível de fantasma** — sem marca, ninguém sabe qual é qual.

**Instrumentação órfã, nos dois sentidos.** `ble_session_base` era **lido e
nunca escrito**: o `ble` imprimia o total rotulado como "msgs da sessão".
Pior que zero-com-cara-de-dado — número plausível e errado, que ninguém tem
como desconfiar. `soma_rearmes_botao` era **escrito e nunca lido**.

**E três que o compilador já acusava.** `zero_buf`, `seco_buf` e um `cc`
local apareciam como "defined but not used" a cada build. Nem tudo passou sem
testemunha: parte foi acusada e **o aviso virou paisagem**.

**Regra:** *toda reestruturação tem três frentes — código, documentação e
símbolos. O compilador cobra a primeira; nada cobra as outras duas.*

**Então o `make check-docs` cobra.** O manifest é a única fonte de verdade;
tudo que fala sobre ele é verificado contra ele. Fantasma silencioso quebra o
build; fantasma **declarado** deixa de ser fantasma.

**A armadilha que a ferramenta pisou na primeira execução**, e era previsível:
ela acusou a frase que documenta a remoção — exatamente a linha que eu tinha
escrito dizendo *"não existe `frz_hold`"*. É a 12c em forma nova. Três regras
tiraram os 76 falsos positivos:

1. **Se o nome existe no fonte, citá-lo é legítimo** — `sdram_pool`,
   `eco_ult`, `reverb_mem` não são params e a doc fala deles com razão. Essa
   regra sozinha matou a classe inteira.
2. **Família conta como nome** — `gov_ceiling` citado quando existem
   `gov_ceiling_a/b`; falar da família é mais legível que listar as duas.
3. **Params aposentados são declarados** em `docs/params-aposentados.txt`.
   Narrar uma remoção exige citar o que foi removido; sem a lista, toda
   cicatriz que conta um conserto viraria fantasma.

**O terceiro é o mais interessante**, porque a lista virou documentação útil
por si: o que existiu, quando saiu, e o que houve. Ressuscitar um nome exige
tirá-lo de lá — senão o verificador para de vigiá-lo.

**Ordem inegociável, e ela vem de experiência:** limpar antes de instrumentar.
Escrever o verificador contra base suja faria o primeiro build falhar em massa,
e a tentação viraria baixar a régua.

**Demonstrado nos dois sentidos**, que é o que separa defesa de hipótese:
fantasma injetado → `make: *** [check-docs] Error 1`; removido → passa.

---

## 13i. A fila também deriva, e ninguém a verifica (22/08)

O backlog trazia: *"`frz_size`: 61% de curso morto, faixa 20–400, clamp 167, etiquetado antes do painel"*. <!-- doc-check: ignore frz_size (cicatriz cita a faixa PRE-conserto) --> Verificado no fonte de agora:

    manifest    20 .. 165 ms
    clamp real  NucleusLen()/3 = (500 ms x sr)/3 = 166,7 ms

Batem. **O defeito foi corrigido em `d80e45d`** e o item ficou meses na fila
descrevendo o estado pré-conserto — inclusive citado por mim, como pendência,
em vários alinhamentos.

**E o agravante, que é o pior da história:** o item não estava só
desatualizado — ele **atravessou vários alinhamentos sendo recitado**, por
mim inclusive, cada vez ganhando um pouco mais de autoridade por repetição.
A fila não é só um lugar onde a deriva se esconde: é um lugar onde ela **se
reforça**, porque cada repetição parece confirmação.

É a deriva doc↔código do mesmo dia, num lugar que o `check-docs` vigiava **pela
metade**: ele confere NOMES contra o manifest e `frz_size` é nome válido — o
que estava errado eram os NÚMEROS.

**Fechado no mesmo dia:** o verificador ganhou checagem de **faixa declarada**
— onde a doc escreve "faixa X–Y" ao lado de um param, os números são
conferidos contra o `t.Add()`. Duas versões mais amplas foram descartadas por
falso-positivo: casar qualquer par de números acusou o `drv_sujeira` pelo
"1,5–48×" do `drv_drive`, e o `dly_time` por "60–64", que é faixa de **CC**.
Os números da doc são majoritariamente CC e versão — **verificador que acusa
inocente perde a autoridade que o torna útil**. O verificador confere nomes de param contra o
manifest, mas o item citava **números** — faixa e clamp — e números em prosa
são território de alto falso-positivo.

**Regra:** *item de backlog é uma afirmação sobre o estado do código, e
envelhece como qualquer outra.* Antes de citar uma pendência, verificá-la —
sobretudo as antigas, que são as que mais provavelmente já foram consertadas
por outro caminho.

**Trabalho real que sobrou:** o `165.f` era literal e podia dessincronizar em
silêncio se `kNucleusMs` mudasse. Ganhou comentário com a conta explícita.
Derivar de verdade exigiria o manifest incluir `freeze.h`, e essa dependência
custa mais que o comentário — **cirúrgico é o que muda menos**.

---

## 13j. A hipótese plausível é a perigosa (22/08)

O JEDEC ID vinha `FF FF FF`. A explicação era boa: *a libDaisy põe o chip em
QPI no `Init`; chip em QPI ignora comando em 1 linha; a linha fica em
pull-up; ler barramento em repouso dá `FF`.* Ela **explicava o sintoma byte a
byte**, e eu escrevi isso com essas palavras.

Estava errada. Lendo o código que configura o chip:

```cpp
// These functions are defined, but we haven't added the ability to
// switch to quad mode. So they're currently unused.
QSPIHandle::Result EnterQuadMode() __attribute__((unused));
```

**A libDaisy nunca entra em quad.** O comentário diz isso com todas as
letras, e o atributo confirma. O chip está em SPI simples.

E o protocolo real, da configuração memory-mapped, é **híbrido** — nem SPI
nem QPI:

    InstructionMode = 1 LINHA        Instruction = 0xEB (quad I/O fast read)
    AddressMode     = 4 LINHAS       DataMode    = 4 LINHAS
    DummyCycles     = 6              chip suposto: IS25LP (num comentario)

Isso também explica o `0x88` que a tentativa em quad devolveu: `0x88` são dois
nibbles `1000` — uma linha alta e três baixas, a assinatura de **comando quad
chegando num chip que só escuta a IO0**. Deixou de ser misterioso.

**Regra:** *quando o sintoma tem explicação plausível, a plausibilidade é o
perigo.* O teste que decide não é o que **confirma** a hipótese — é o que **lê
a fonte da configuração**. Seis gravações varrendo dummies e modos não
responderam o que um `grep` no `Init` respondeu em um minuto.

É o inverso do "teste bom sobrevive a hipótese ruim" que este projeto já tem:
aqui a hipótese era boa demais para ser questionada, e o que a derrubou foi
**leitura, não medição**.

**E a lista de aposentados não foi projetada para símbolos removidos — foi
exigida por eles.** Removi `zero_buf` e `seco_buf` no commit A; a cicatriz
13h narra a remoção; a narração virou fantasma no primeiro `make check-docs`
depois. O mecanismo se completou por pressão do próprio uso, que é o melhor
sinal de que o desenho está certo.

## 13k. Nenhum valor de dado pode significar "sem dado" (22/08)

A sonda QSPI reportava, para quatro comandos diferentes:

    0x05 RDSR  status   -> (nada)
    0x9F RDID  jedec    -> 03 FF FF
    0x35 RDCR  config   -> (nada)
    0xAB RDP   wake     -> 03 FF FF

O `(nada)` vinha daqui:

    if(!id || id == 0xFFFFFFu) { /* "o periferico NAO respondeu" */ }

**`0x00` é exatamente o que um chip ocioso devolve ao RDSR.** Status zero é
"não ocupado, não armado para escrita" — a resposta mais normal que existe.
O relatório estava chamando a resposta correta de ausência de resposta.

Trocado por impressão do valor cru sempre, com a interpretação **etiquetada
ao lado**, o mesmo hardware disse outra coisa:

    0x05 RDSR  cru = 00 00 00  (tudo baixo — o chip DIRIGIU)
    0x9F RDID  cru = 03 FF FF  (dado misto)
    0x06 WREN  cru = 00 00 00  (tudo baixo — o chip DIRIGIU)

E foi essa distinção — `00` significa *alguém puxou a linha para baixo*,
`FF` significa *linha solta* — que apontou o ABORT ausente. **O diagnóstico
seguinte só existiu porque o filtro saiu da frente.**

### A quinta do dia, e por isso a família tem forma, não coincidência

É literalmente o `else 0` da manhã (12u) com outra roupa:

| onde | valor legítimo | lido como |
|---|---|---|
| parser de bancada | pico `0 µs` improvável | "não casou" → **defeito inexistente** |
| `voltaram 0` | zero retornos reais | "a placa não transmite" |
| `eco_proprio` | retorno, não envio | "mandou 4 de 8" |
| `mscope` sem sinal | canal calado | "a fonte não mandou" |
| `RDSR = 0x00` | **chip ocioso** | "não respondeu" |

> **Nenhum valor do domínio pode significar "sem dado". Ausência se
> representa FORA do domínio** — flag separada, sentinela impossível, ou par
> `(valor, validade)`. Nunca por um valor que o domínio consegue produzir.

Cinco casos num dia é a assinatura de uma **forma estrutural**, não de uma
sequência de descuidos. E a forma é sempre a mesma tentação: o valor
"vazio" está ali, é conveniente, e escrever `if(!x)` é mais curto do que
carregar a validade junto.

**Corolário para relatórios:** imprima o **cru** e etiquete a interpretação
ao lado. Quando as duas ficam na mesma linha, o leitor pode discordar da
segunda sem perder a primeira — que foi exatamente o que aconteceu aqui.

## 13l. Parar no meio é a regra funcionando; parar depois é a cicatriz (22/08)

Escrevi a 13j nesta mesma sessão. Ela diz: *o teste que decide é o que lê a
fonte da configuração.* Depois disso **voltei a varrer duas vezes** — o
ABORT e o formato de comando, uma gravação cada.

O ABORT rendeu achado de protocolo de verdade (**sair do memory-mapped exige
`QUADSPI_CR_ABORT`; desligar o `EN` não basta, e a libDaisy faz isso por
dentro do `SetMode`**), o que torna o caso mais escorregadio, não menos: um
chute que acerta parcialmente é o que financia o chute seguinte.

Parei ao ver a terceira hipótese nascer. O argumento que decidiu não foi de
método, foi de **aritmética de bissecção**:

> Havia três hipóteses vivas — dois passos (`Command` + `Receive`), `NbData`
> /`DLR`, e ordem de escrita do `CCR`. Testar uma custa uma gravação e, se
> falhar, **não elimina as outras duas** — porque sem o manual não dá para
> saber se falhou a hipótese ou a execução dela. É a pior forma de
> bissecção: **a que não parte o espaço.**

Com o RM0433 aberto, a sequência de leitura indireta é **leitura**, e as três
hipóteses viram uma resposta.

> **Antes de gastar um voo numa hipótese, perguntar: se ela falhar, o espaço
> de busca diminui?** Se a resposta for não, o voo não é experimento — é
> tentativa, e tentativa se paga com gravação.

**E a diferença que importa entre parar e desistir:** a frente parou com o
**difícil resolvido** (a janela ITCM funciona por construção) e o **tedioso
identificado** (a sequência de registrador). A condição de retomada está
escrita no `DESENHO-ESCRITA-QSPI.md`, com as três hipóteses nomeadas, para
que ninguém recomece o espaço de busca.

## 13m. Falha crônica é sensor desligado que ninguém lembra de ter desligado (23/08)

Os bancos nativos falhavam **3 de 3** havia semanas. Ninguém os rodava, porque
"eles já falham". Ao abrir, nenhuma falha era do firmware:

- procurava `gov_ceiling`, que virou `gov_ceiling_a/_b`;
- esperava `dly_time` no CC 22, quando o mapa o levou ao 64;
- mandava CC 20 para o `level`, que foi para o 100.

**O agravante não é que falhavam — é o que passou despercebido por causa
disso.** O teste do `gov_ceiling` se descrevia como *"a trava da decisão"*:
existia para reprovar o build se alguém remapeasse aquele CC sem ler o
porquê. A decisão **foi revista** — com dois exciters a proteção duplicou e a
página entrou no mapa em 110/116, com a razão escrita no manifest. A trava
deveria ter ficado vermelha e forçado a atualização do raciocínio. **Já estava
vermelha**, pelo nome antigo, e o sinal se perdeu no ruído.

> **N falhas crônicas escondem a falha N+1.** Uma suíte que falha por
> paisagem não é uma dívida a pagar depois: é um sensor desligado, e ninguém
> lembra de ter desligado.

### O teste que some em silêncio é pior que o que falha

`if(id_rs >= 0) { ...testa rev_size... }` — o guarda existia para o caso de o
param não existir. Quando o reverb virou por cadeia e o nome ganhou sufixo, o
bloco **parou de rodar**: sem falhar, sem imprimir, sem deixar rastro. A suíte
encolheu e o relatório continuou dizendo `ok`.

Foi trocar o guarda silencioso por **afirmação de existência** que o
descobriu, no primeiro voo depois da reescrita.

É a família da cicatriz 12e (verificação sem objeto) na forma mais cara: ali
o verificador passava sobre nada; aqui o teste **se apagou** e a contagem de
testes caiu sem que ninguém a conferisse.

### O conserto que impede a reincidência

O bloco já carregava o comentário certo — *"o teste segue o MAPA, não um
número decorado"* — e a linha seguinte cravava `{0xB0, 20, 127}`. **Intenção
escrita, implementação decorada.**

Agora o CC vem do manifest (`CcDe(param_id)` varre `midi_map`), e o CC de
controle "fora do mapa" é achado por **varredura** em vez de escolhido — um
número fixo vira "dentro do mapa" no dia em que alguém o usar, e o teste
passa a provar o contrário do que diz.

**Com isso, remapear não quebra o teste. O que quebra é o contrato mudar:**
param sumir, param entrar ou sair do mapa. Que é o que um banco deve vigiar.

**E o ponto cego gêmeo, no verificador de docs:** `docs/operacao/` chegou com
sete documentos e o `check_docs` usava `glob("*.md")`, não recursivo — sete
documentos entraram sem vigilância nenhuma, e a saída continuou `ok`. Pasta
nova entra sem vigilância **por padrão**.

## 13n. Desligar um transmissor não esvazia o fio (23/08)

A multiplexação pôs a entrada da cadeia B no ch2, que é onde o espelho fala.
A recusa *"não obedeço a minha própria voz"* era incondicional naquele canal,
então a cadeia B nasceria inalcançável.

A correção não foi afrouxar a recusa — foi **amarrá-la à premissa que a
justifica**: a defesa existe porque a placa fala ali; se não está falando, não
há voz própria a recusar. Defesa órfã é a que continua de pé depois que a
ameaça sai.

**E aí o contador novo pegou o que a chave sozinha não resolvia.** Logo após
`espelho off`, quatro CCs chegaram marcados "fora do mapa". Eram mensagens do
*próprio espelho*, ainda em voo no fio quando a chave virou. As
compartilhadas não têm vínculo no ch2 e caíram como sem dono — **as pareadas
teriam achado dono**: o espelho manda o CC 34 com o valor de `drv_drive_a`, e
no ch2 o CC 34 é `drv_drive_b`. A placa escreveria o valor da cadeia A dentro
da B, sozinha, e o sintoma seria "a cadeia B muda quando eu não mexo".

> **Desligar um emissor não drena o meio.** Todo desligamento de transmissor
> precisa de **carência de drenagem** antes de voltar a confiar no que chega
> pelo mesmo canal — e a carência se dimensiona pelo tempo de voo do meio, não
> por gosto (31250 baud, ~1 ms por mensagem, rajadas de duas a cada 8 ms → 300
> ms com folga larga).

Mesma família da carência do rearme: **mecanismo que age precisa de intervalo
entre agir e voltar a medir**, ou reage à própria ação ainda em curso.

### A CLASSE NOVA: mensagem mal-endereçada (não órfã)

**A multiplexação por canal criou uma classe de falha que não existia.** Antes,
uma mensagem fora do mapa era **órfã**: não achava dono e não fazia nada. Com o
canal no endereço, a mesma mensagem no canal errado **acha o dono errado**.

As duas metades da evidência estavam juntas no mesmo contador:

| o que chegou | com um mapa por CC | com o canal no endereço |
|---|---|---|
| compartilhada (sem par) | órfã | órfã — **inofensiva** |
| pareada (`_a`/`_b`) | órfã | **escreve na cadeia irmã** |

E é a mais cara de diagnosticar que este projeto já produziu, porque **não há
gesto humano para correlacionar**: a cadeia B muda sozinha, com o operador
parado. Todo o repertório de depuração deste projeto — "o que você mexeu antes
de acontecer?" — não morde.

> **Ao pôr o canal (ou qualquer dimensão nova) no endereço, toda mensagem que
> antes era órfã passa a ser candidata a mal-endereçada.** A pergunta deixa de
> ser *"isto acha dono?"* e vira *"isto acha o dono CERTO?"* — e as duas só se
> distinguem se o endereço inteiro for conferido, nunca uma parte dele.

**A defesa final não é de tempo de execução.** A carência de drenagem resolveu
enquanto a sobreposição existia; no voo B o espelho ganhou canais próprios e a
sobreposição ficou **impossível por construção**, então a carência saiu e no
lugar dela entrou um `static_assert`:

```
static_assert(kCanalEspelhoA != kCanalEntradaA && ... ,
              "canal de espelho nao pode coincidir com canal de entrada");
```

### E a travessia, que o estado final esconde

Ao aposentar a carência eu raciocinei sobre o **estado final** — ch3/ch4 sem
vínculo, logo inofensivos — e esqueci a **migração**. Ao gravar, a placa
reinicia; o WIDI e o iPad não. Mensagem de espelho da versão anterior, que
falava no ch2, chega na versão nova pelo ch2 — que agora é entrada plena da
cadeia B. O fantasma da mal-endereçada, na única janela em que ele ainda cabe.

Foi o dB que apontou, e o argumento é geral:

> **Provar que a configuração NOVA é segura não prova que a TRAVESSIA é.** As
> duas pontas de uma migração podem ser sólidas e o caminho entre elas não —
> porque durante a troca coexistem endereçamentos de duas versões, e o que
> está no fio não sabe que a versão mudou.

Daí a **drenagem de boot**: nos primeiros 300 ms, entrada recusada e contada.
A regra que a sustenta depois de a migração terminar é mais larga que o caso:
*no boot, tudo que está no fio foi endereçado a uma configuração que pode não
ser esta.*

**E ela nasceu indemonstrável**, o que quase a deixou como hipótese: 300 ms
fecham antes de a serial USB enumerar, então ninguém a observa agindo, e
"contador em 0" fica ambíguo entre *funcionou e nada chegou* e *nunca rodou*.
O comando `dreno <ms>` rearma a janela e a torna observável — mesma família do
`ctos prazo`. Exercitada: declara-se com contagem viva e expira.

**Defesa que vira invariante de compilação é melhor que defesa que roda:**
custa zero, não pode ser esquecida ligada nem esquecida desligada, e explica-se
sozinha para quem a quebrar. E ela foi **exercitada antes de ser aceita** —
`kCanalEspelhoA = 1` reprova o build com a frase certa. Verificação que nunca
disparou é verificação sem objeto (12e), inclusive quando é de compilação.

### E o terceiro caso do padrão: o contador que encontra o que não foi medir

O `(canal,cc) fora do mapa` nasceu para separar *"widget na página errada"* de
*"widget no número errado"*. A primeira coisa que encontrou foi **um defeito
meu, escrito quinze minutos antes**, numa chave que eu tinha acabado de
declarar segura.

Terceiro caso do mesmo padrão neste projeto — o `make check` na primeira
execução, o `check-docs` no primeiro uso, e agora este:

> **Instrumentação nova encontra algo que não era o alvo dela.** Três casos é
> padrão, não sorte: o valor de um contador não está só no que ele foi medir,
> mas em **tornar visível uma dimensão que estava escura**. Isso é argumento
> para instrumentar cedo, não depois do primeiro mistério.

**O que fez isso ser visto foi um contador que ainda não tinha assunto.** O
`(canal,cc) fora do mapa` nasceu para distinguir "widget na página errada" de
"widget no número errado" — e a primeira coisa que encontrou foi um defeito
meu, escrito quinze minutos antes, numa chave que eu tinha acabado de
declarar como segura.

**E a mesma leitura entregou o retrabalho do Loopy de graça:** `ORFAO ch1
cc24`, `cc26`, `cc33` — exatamente os números que a cadeia B liberou, com os
widgets antigos ainda apontando para eles.

## 13o. Comando que não consta não existe (23/08)

Três comandos escritos no mesmo dia — `dup`, `espelho`, `dreno` — funcionavam
e **não apareciam na linha do `?`**. O padrão é sempre o mesmo: escreve-se o
handler, testa-se chamando pelo nome (que quem escreveu já sabe), e a linha de
ajuda fica para trás.

> **A descoberta de um comando é pela linha de ajuda, não pela leitura do
> fonte.** Para quem está na bancada com a serial aberta, comando que não
> consta é comando que não existe — e o custo não é o comando perdido, é o
> trabalho feito à mão porque a ferramenta parecia não haver.

Virou regra do `make check-docs`: todo `strncmp(line, "X"` tem que aparecer no
`?`. **Na primeira execução ela achou mais dois** — o quarto caso do padrão
"instrumentação nova encontra o que não era o alvo dela", agora no mesmo dia
em que o padrão foi nomeado:

- **`b1`** — dispara o randomize;
- **`pk`** — entrada **crua por canal**, pré-ganho. É o instrumento que nasceu
  da cicatriz do canal sem medidor, o que responde *"chega sinal?"* sem
  hipótese — e estava invisível para quem fosse à bancada procurar por ele.

**E a isenção que eu mesma escrevi estava larga demais:** pus `b2` e `b3` na
lista de isentos porque "entram por outro caminho", e os dois são comandos de
verdade (pulso de freeze, pulso de rearme). Isenção generosa transforma
verificador em carimbo — a lista ficou com os `k1..k5`, que a linha anuncia
como faixa, e os dois `midi*`, que ela lista por extenso.

## 13p. Custo proporcional a uma tabela que cresce é bomba-relógio silenciosa (23/08)

O `sync` (`MidiPushEstado`) gasta `hal::DelayMs(3)` por mensagem. Quando foi
escrito, o mapa tinha 12 CCs — **36 ms**, e o comentário ao lado dizia
*"despejar 12 CCs de uma vez"*. O mapa foi a 86. **258 ms de loop bloqueado**,
e nenhuma linha do `sync` mudou.

> **Variante nova do achado 5 do audit: a regressão veio por CRESCIMENTO, não
> por edição.** As ferramentas do projeto vigiam mudança — `git diff`,
> revisão, o verificador de docs. Nenhuma vigia *o mesmo código ficando mais
> caro porque a tabela sob ele engordou.*

E o comentário congelado no número antigo é o agravante: **instrumento que
mente por obsolescência.** Ninguém o escreveu errado; ele deixou de estar
certo sem que ninguém tocasse nele.

### A varredura que a pergunta genérica produziu

*Que outros custos escalam com uma tabela e foram dimensionados quando ela era
pequena?* Varridos todos os laços sobre `n_midi`, `params.Count()` e
`kMaxParams`:

- **O callback de áudio está limpo.** O único laço sobre tabela ali é o
  `n_sacrif`, travado em 12 e ligado à contagem de EFEITOS, não à de params.
  A bomba não chegou ao caminho quente — e isso é sorte de desenho, não
  vigilância: ninguém tinha verificado.
- **`FindMidiParam`/`ApplyMidiCc`** são varredura linear por CC recebido, e
  foram de 76 para 86 comparações. Barato, mas passou a rodar em toda
  mensagem.
- **Relatórios** (`midi`, `vistos`, `list`) imprimem uma linha por entrada.
  Cresceram junto; incômodo, não risco.

**A pergunta vale como item de audit permanente**, porque a resposta muda toda
vez que uma tabela cresce, e ninguém é avisado quando isso acontece.

## 13q. Defesa aplicada num caminho e não no irmão (23/08)

Três ocorrências, e a terceira é minha, no mesmo dia em que escrevi sobre as
outras duas:

| defesa | caminho protegido | irmão desprotegido |
|---|---|---|
| governor | ramo L | **ramo R** (até 20/08) |
| janela de autoria | `MidiEspelha` | **`MidiPushEstado`** (o `sync`) |
| canal no relatório | `vistos` | **`midi`** |

O caso do `sync` é o que custa: ele manda 86 mensagens e **não abre uma janela
sequer**. Como widget de toggle devolve o próprio estado — que é literalmente
o motivo de a janela existir — depois de um `sync` a placa obedece o eco. O
`eco_ult` impede o espelho de reenviar; não impede a entrada de aceitar.

E o do relatório é o mais barato e o mais revelador: acrescentei o canal ao
`vistos` porque sem ele o inventário não responde nada, **e não olhei o
`midi`**, que imprime `cc 34 -> drv_drive_a` e `cc 34 -> drv_drive_b` em linhas
indistinguíveis.

> **Ao consertar ou proteger um caminho, a pergunta não é "onde mais esta forma
> aparece?" — essa já está no catálogo e eu falhei nela de novo. É mais
> estreita e mais respondível: quem mais faz ESTA MESMA COISA?** Duas rotas de
> saída, dois relatórios do mesmo mapa, dois ramos de amplificação. O irmão é
> quase sempre visível do lugar onde se está.

### A versão EXECUTÁVEL da pergunta, que é do dB

*"Onde mais esta forma aparece?"* é busca por **semelhança**, e semelhança é
subjetiva. *"Quem mais faz esta mesma coisa?"* é melhor porque é busca por
**função**. Mas a versão que se executa é:

> **Ao acrescentar um campo, uma defesa ou um relatório, faça `grep` pelos
> DADOS que ele toca — não pela forma dele.**

Nos três casos o irmão cai nesse grep:

| defesa | dado tocado | o grep entrega |
|---|---|---|
| governor | ramo de saída | `OUT_L`, `OUT_R` |
| janela de autoria | caminho do espelho | `eco_ult`, quem transmite |
| canal no relatório | o `midi_map` | todo consumidor do mapa |

**E o traço comum que explica por que o irmão escapa: nos três, ele foi criado
em outro momento.** O governor L existia antes de R; o `MidiPushEstado` já
estava lá quando o `MidiEspelha` ganhou a janela; o `midi` já imprimia quando
o `vistos` ganhou canal. O irmão não está escondido — está **fora do campo de
atenção**, e atenção é sempre local à edição. Grep de dado é o que o traz de
volta ao campo.

### Aplicada na hora, ela achou o que a previsão não tinha previsto

Grep pelo dado *"recusar entrada por um tempo"* devolveu **três** mecanismos,
não os dois esperados:

| variável | escopo | gatilho |
|---|---|---|
| `quiet_until[param]` | por param | o espelho falou daquele assunto |
| `quar_block_to` | global | despejo de reconexão detectado |
| `boot_dreno_ate` | global | boot |

Os três fazem *recusar CC até T, contando*. **Os dois globais são o mesmo
mecanismo com razões diferentes** — um emprego, duas variáveis, que é a
inversão da variável-com-dois-empregos e diverge no primeiro conserto. Fica
registrado; unificar é trabalho de outro voo.

## 13r. Para provar que o binário gravado tem a fonte atual, leia na placa um número que só a fonte nova produz (23/08)

Um `make` que não reconstruiu levantou a hipótese feia: *se header não dispara
rebuild, gravei binário velho a sessão inteira.*

**A hipótese era cômoda de descartar por plausibilidade** — "ah, deve ter sido
mtime" — e plausível ela era, e certa também. Mas descartar por plausibilidade
é o defeito da 13j, e o custo de estar errado aqui era toda a sessão.

Medido: existem depfiles, mudança em header recompila, e o provável foi
granularidade de mtime numa edição no mesmo segundo.

**E a prova de que nada velho foi gravado não veio do build.** A placa
reportava `86/96 params | 86/128 CCs` — números que **só existem no header**.
Se o binário fosse velho, diria 76.

> **Evidência do lado de fora do build.** Comparar timestamps, ler o `.map` ou
> confiar no toolchain são todos testemunhos do próprio sistema sob suspeita.
> Ler na placa um valor que **só a fonte nova consegue produzir** é a única
> prova que não depende do acusado.

Generaliza: a versão em `COTUCA_OS_VERSION` já faz isso para o `main.cpp`, e o
`flash.sh` a confere. **O que ela não cobre é mudança que vive só em header** —
e aí serve qualquer número derivado: contagem de params, de vínculos, um hash.
*Escolha um número que a mudança move, e leia-o do outro lado.*

## 13s. Todo teste de rejeição precisa do gêmeo que confirma a aceitação (23/08)

Três aparições no mesmo dia, e as três com a mesma forma: **o teste positivo
passa sozinho e esconde que o mecanismo virou carimbo.**

| mecanismo | o teste que se escreve | o gêmeo que quase ninguém escreve |
|---|---|---|
| hash do mapa | renomear **muda** o hash | **mudar a faixa NÃO muda** |
| pertinência ao preset | as 11 exceções ficam **fora** | os params de música **entram** |
| ordem da conferência | hash corrompido lê **corrupção** | remapeamento real lê **remapeamento** |

Em cada caso, o mecanismo mais grosseiro possível **passa no primeiro teste**:
um hash sobre a tabela inteira muda ao renomear; um `ForaDoPreset` que devolve
`true` sempre exclui as 11; uma `Conferir` que sempre diz "corrupção" reporta
corrupção no campo do hash. Os três estão errados e os três passariam.

> **Provar que o portão fecha não prova que ele abre.** Todo teste que confirma
> uma **rejeição** precisa do gêmeo que confirma a **aceitação** — e é sempre o
> gêmeo que falta, porque a rejeição é o comportamento que se está construindo
> e a aceitação parece óbvia.

O custo de faltar não é um teste a menos: é um mecanismo **largo demais que
passa em tudo**. E largo demais tem sintoma próprio no campo — recusa demais
ensina o operador a **ignorar a recusa**, e aí a defesa está pior que ausente,
porque consome atenção sem entregar proteção.

### A estrutura em camadas que a ordem produziu, e ela generaliza

Conferir um cabeçalho binário tem três camadas, e cada uma só é legível porque
a anterior passou:

    1. o que torna a conta POSSIVEL   magic, formato, coerência de tamanho
    2. a conta                        checksum
    3. o que a conta PROTEGE          hash do mapa, contagem

É a **P2 do audit — *quem configura vem antes de quem consome*** — aplicada a
leitura de dado em vez de inicialização de módulo. Ler um campo antes de o
checksum passar é consumir antes de configurar, e o preço é um diagnóstico
plausível e errado.

**E a camada 1 não é só diagnóstico: é segurança de memória.** Um `n_params`
corrompido maior que o buffer não produz uma frase ruim — produz leitura fora
dos limites, e a soma do checksum seria feita sobre lixo.

### Disciplina em comentário envelhece; contrato no compilador, não

A convenção *"toda mudança de layout incrementa `kFormato`"* vivia como
comentário. Virou `static_assert(sizeof(PresetHeader) == 8, ...)` com a regra
na mensagem — quem mexer no layout **bate na trava onde a regra está escrita**,
em vez de descobrir meses depois lendo um preset deslocado.

O assert não verifica que o `kFormato` subiu (não dá, em compilação). Garante
que ninguém mude o layout **sem passar por ali**. Terceiro uso do mesmo gesto
no dia, depois do canal de espelho e do `n_sacrif`: **transformar disciplina em
contrato, e limite em contrato.** Exercitado — um campo a mais reprova o build
com a frase certa.

## 13t. O nível cru inocentou a solda e acusou o firmware (23/08)

O dB soldou o freeze B em D22/D23. O botão não contava nada e o LED não
acendia. **Duas suspeitas foram levantadas contra o trabalho dele — fiação
cruzada e GND compartilhado ausente — e as duas eram falsas.**

O que resolveu foi um instrumento que mede **antes de qualquer camada**:

    B2  ja fechou pra GND: SIM  (24144 amostras em baixo)   -> 2 batidas
    B4  ja fechou pra GND: SIM  (24993 amostras em baixo)   -> 0 batidas

Números praticamente idênticos. **O pino fechava; o caminho com debounce
ignorava.** A partir dali o problema deixou de ser elétrico por prova, não por
opinião — e a solda estava perfeita desde o começo.

> **Contador em zero tem três causas — aberto, terminal errado, e "ninguém
> leu" — e não distingue nenhuma.** O nível cru distingue as três, e é a mesma
> razão de o `pk` medir a entrada de áudio antes de qualquer ganho.

### Os dois defeitos, e o segundo não estava sendo procurado

```c
for(int b = 0; b < 3 && b < hal::Info().num_buttons; b++)
```

**O índice 3 — D22, freeze B — nunca era lido.** Segunda divergência do mesmo
par em dois dias: em 22/08 o array ficou em `[2]` com o laço em `3` e escreveu
fora; consertado alargando os dois à mão, e a forma sobreviveu para divergir
de novo no outro sentido.

E ao ler o laço apareceu o que ninguém procurava:

```c
else if(b >= 1 && now) { AlternaBotao(b); }
else if(b == 2 && now) { RearmaCtosDevolvendo(); }   // <- MORTO
```

Para `b == 2` a primeira condição casa, então **o botão de rearme do CTOS
nunca rearmou nada desde que foi escrito.** O comentário afirmava que o B3
*"cai no return do AlternaBotao e segue pro rearme logo abaixo"* — `else if`
não funciona assim. **Prosa descrevendo um fluxo que a linguagem não executa**,
e ela sobreviveu a todas as leituras porque descrevia uma intenção plausível.

**O conserto não foi alargar o número.** O tamanho passou a vir de
`kMaxBotoes` e o laço para em `num_buttons` — as duas fontes que já existiam,
nenhum número digitado no meio. E a cadeia `else if` virou **saída antecipada**:
cada botão tem um bloco, o específico vem antes do genérico, e ramo morto por
ordenação deixa de ser possível porque não há ordenação entre condições.

### E o erro meu que custou o desvio

Eu deixei o LED de B ligado por `set frz_on_b 1` e pedi ao dB que olhasse.
**Antes de ele responder, gravei outra versão** — a placa reiniciou, o param
voltou a 0, e o "não está aceso" que chegou depois era sobre um estado que eu
mesma tinha destruído. Foi essa leitura falsa que produziu a hipótese do GND
compartilhado e mandou o dB buscar multímetro.

> **Gravação destrói estado, e a regra já existia. O que faltava era o
> corolário: não gravar enquanto uma observação humana está em voo.** O
> operador não é um instrumento que se relê depois — a resposta dele chega
> quando chega, e se o estado mudou no meio, ela responde outra pergunta.

## 13u. Truncagem silenciosa nas duas pontas do mesmo canal (23/08)

O `preset dump` emitia **13 de 33 params** e a linha parecia completa. Ao
consertar, a colagem de volta recusava **um param por linha**. Duas
truncagens diferentes, nas duas pontas do mesmo canal serial, e **nenhuma das
duas se anunciava.**

| ponta | limite | o que fazia |
|---|---|---|
| saída (`hal::Log`) | 160 chars | cortava e imprimia como se estivesse inteiro |
| entrada (`kLineMax`) | **64 chars** | descartava o excedente **e despachava o pedaço** |

A da entrada é a pior das duas, e não por pouco: **um comando truncado
continua sendo um comando válido.** `set drv_drive_a 12` cortado vira
`set drv_drive_a 1`, que a placa obedece calada. O operador pede uma coisa, a
placa faz outra, e não há uma linha de log em lugar nenhum.

> **Linha truncada não é comando.** Executar metade é pior que não executar
> nada, porque não executar é visível e executar metade não. Recusar a linha
> inteira e contar é o único comportamento honesto.

### Como foi achado, e vale mais que o bug

Duas rodadas de teoria erraram — buffer meu, `\r` no fim. O que resolveu foi
**instrumentar TODOS os ramos de recusa** e ler o cru:

    [diag] valor ruim: pn="drv_drive" tem=0 v=0 prox=0
    [diag] sem '=':    pn="gate_re"   prox=0

Os dois cortam no caractere 63, e o segundo mostra um **nome partido no meio**
— que nenhuma hipótese de `\r` explicaria. A primeira instrumentação cobriu
só um dos ramos e não imprimiu nada, o que quase me fez concluir que a recusa
vinha de outro lugar. **Instrumentar um ramo de três e ler o silêncio como
resposta é a armadilha da própria família.**

### A variante nova de instrumento que mente: PONTO CEGO, não valor errado

A primeira instrumentação cobriu **um** dos três ramos de recusa. Ela não
imprimiu nada — e eu quase li isso como *"a recusa não está aqui"*, quando o
que dizia era *"eu não olho aqui"*.

> **Antes de concluir do silêncio de uma sonda, prove que ela cobre o caso.**
> A sonda não deu valor errado: teve **ponto cego**, e o silêncio dela foi lido
> como evidência sobre o FENÔMENO em vez de sobre a COBERTURA DELA.

É a irmã da 13k (nenhum valor do domínio significa "sem dado"), aplicada a
**instrumentação incompleta** em vez de valor ambíguo. E some com o mesmo
gesto: instrumentar todos os ramos antes de ler qualquer um.

### A verificação retroativa, porque "provavelmente" não basta

*Comando truncado é comando válido* significa que **toda sessão anterior**
rodou sobre um canal que podia obedecer pela metade em silêncio. A pergunta
tinha que ser feita: algum script de bancada mandou linha acima de 63 chars?

Varridos os 16 scripts, literais e f-strings:

    maior comando literal:          24 chars
    pior caso em f-string:          31 chars

**Nenhuma medição publicada veio de linha truncada.** Verificado, não suposto —
que é a diferença que a 12u custou.

### Consertar a forma, não o caso

Três consertos, e nenhum deles é "aumenta o número":

- **`hal::Log` marca o corte** com `...` — três caracteres do conteúdo pelo
  saber que houve conteúdo perdido. Pega todos os casos futuros, não só este.
- **`kLineMax` foi a 160, e a simetria virou CONTRATO:**
  `static_assert(kLineMax >= kLogMax)`. Uma linha que a placa consegue
  IMPRIMIR tem que ser uma linha que ela consegue LER de volta — senão o
  `dump` produz texto que ela mesma recusa. Hoje os números são iguais porque
  alguém os igualou; amanhã alguém sobe um e não o outro. Exercitado: com
  `kLineMax = 80` o build reprova com a frase certa.
- **O `dump` deixou de depender de comprimento**: linhas curtas com o nome
  repetido, e quem lê junta as de mesmo nome. Alargar o buffer consertaria o
  caso e preservaria a forma — o 34º param quebraria de novo, em silêncio.

O último é o padrão da casa outra vez: **o formato não pode ter um limite que
o conteúdo possa alcançar.**

## 13v. O ciclo completo, e o que ele prova (23/08)

    capturar -> dump -> RESET (a RAM some) -> colar de volta -> load

33 params, 7 linhas, zero recusas, e os valores voltam. **A biblioteca em RAM
já é persistência de verdade** — o computador é a flash, e o `dump` é o
formato. Quando a QSPI destravar, a biblioteca só muda de casa.

**A única diferença no retorno é declarada, não descoberta:** `drv_drive`
capturado em 30,00 volta 30,06. São os 7 bits do desenho (0..127, igual ao
MIDI), e o `capturar` diz isso na hora de capturar — não no `load`, porque é
na captura que a perda acontece, e porque descobrir depois comparando um som
lembrado com um som carregado é a forma mais cara de aprender que a operação
não era exata.

## 13w. O inventário que falta: o que está declarado e nunca foi exercitado

O B3 tinha um `else if` inalcançável **desde que foi escrito**: o rearme do
CTOS nunca funcionou. Botão declarado, no mapa de pinos, na documentação — e
morto. Ninguém notou porque **ninguém exercitou**.

E ele tem irmão: o **CC 119 = 64** (watchdog do shimmer) está marcado como
"nunca exercitado" há várias sessões. Dois casos é padrão.

A regra da casa já diz *proteção não-exercitada é proteção suposta*. **O que
falta é a lista.** Candidatos levantados, e nenhum verificado ainda:

- os caminhos de recusa do CTOS que só rodam sob condição rara;
- o `ctos teto` artificial — o `prazo` foi exercitado, o `teto` foi?
- os avisos que só disparam em estouro;
- o auto-kill do shimmer;
- a quarentena de reconexão (`quar_block_to`) — ela já agiu **alguma vez**?
- o `MidiPushEstado` sob um controlador que devolve.

> **Defesa que nunca agiu e não tem gatilho artificial é hipótese com nome de
> proteção.** O inventário é barato: para cada defesa, ou o contador é ≥ 1
> desde algum voo, ou existe um comando que a exercita. Sem nenhum dos dois,
> ela entra na lista.

O custo já foi pago uma vez, no B3. **Fila, com argumento próprio.**

## 13x. Ausência só é evidência quando a presença era possível (23/08)

Três casos no mesmo dia, e só ao juntá-los ficou claro que eram **um**:

| caso | o silêncio significava | eu li como |
|---|---|---|
| `RDSR` devolveu `0x00` | o chip respondeu **ocioso** | "não respondeu" |
| sonda instrumentando um ramo de três | não cobri os outros dois | "a recusa vem de outro lugar" |
| build sem erro depois do `touch` errado | **não recompilei** | "a trava não morde" |

Nos três, uma **ausência** foi lida como evidência sobre o mundo, quando ela
era evidência sobre o **canal** — o filtro, a cobertura da sonda, o
rebuild que não aconteceu.

> **Antes de concluir do silêncio, prove que havia caminho para o som.**
> Ausência só é evidência quando a presença era possível.

É a mesma raiz da 13k por outro lado: lá, um valor do domínio significava "sem
dado"; aqui, "sem dado" é lido como um valor do domínio. As duas se resolvem
com a mesma pergunta feita antes da conclusão: *se o fenômeno estivesse
acontecendo, este instrumento diria alguma coisa?*

### O corolário que faltava, e ele é recursivo

O terceiro caso aconteceu **dentro do commit que fala sobre travas não
exercitadas**: eu ia declarar exercitado um `static_assert` que nunca tinha
rodado, porque fiz `touch` no arquivo errado e li o "0 erros" como resposta.

A regra da casa dizia *toda proteção nasce com o gatilho artificial que a
exercita*. Faltava a linha seguinte:

> **O gatilho também precisa ser exercitado.** Um teste que passa sem ter
> rodado é indistinguível de um teste que passa por estar certo — e a
> assinatura é exatamente esta: **"0 erros" onde se esperava vermelho.**

Quando um exercício de trava não falha, a primeira hipótese não é "a trava
está errada": é **"o exercício não aconteceu"**.

## 13y. A conferência achou o defeito na defesa que ela conferia (23/08)

O dB pediu que o `sync` deixasse de ser *"mandei"* e virasse *"chegou"*: se o
Loopy não devolver o eco, avisa; se devolver, dá o OK.

O instrumento já existia e era **a própria defesa**. Se o controlador recebe,
os widgets se movem e devolvem o estado deles — a janela de autoria barra esse
eco (é para isso que ela existe) **e o conta**. Então o retorno é exatamente o
que a janela registra.

**Na primeira execução do ramo positivo, o relatório disse:**

    sync: OK — 86 CCs enviados, 5 voltaram (0 barrados pela janela)

**Zero barrados, com cinco ecos chegando.** A defesa estava de pé e não pegava
nada.

A causa: o despejo leva **258 ms** (86 × 3 ms) e a janela dura **250**. Ela
abria **por mensagem**, então a do primeiro param expirava antes de o último
sair — e um controlador que responde em lote chega com os primeiros já
desprotegidos.

> **A janela conta do FIM do gesto, não do meio dele.** O gesto autônomo é o
> `sync` inteiro, não cada mensagem. Re-carimbar todos os params depois do
> último envio é o que a torna verdadeira.

**E eu já tinha escrito isso** — no desenho do `load`, com estas palavras:
*"abre `quiet_until` para os 33, DE UMA VEZ"*, e a justificativa de que o
widget que ecoasse no milissegundo 5 seria obedecido. **Não apliquei ao
irmão.** Quarta ocorrência da 13q no mesmo dia, e desta vez o irmão era uma
regra que eu mesma tinha acabado de formular.

### O que isto diz sobre verificação

A conferência não foi um relatório sobre um sistema que funcionava: ela
**mudou o sistema no ato de medi-lo**, no sentido bom — expôs que a defesa era
nominal. Sem o `0 barrados` impresso, o `sync` continuaria "funcionando" e a
janela continuaria não pegando nada, indefinidamente.

> **Transformar "mandei" em "chegou" não é polimento de mensagem: é a única
> forma de descobrir que a coisa não chegava.** E vale para toda operação que
> hoje termina dizendo apenas que executou.

### E o exercício enganou o instrumento

Ao exercitar o ramo *"o fio de volta já funcionou"*, usei `miditest` — que
injeta na **mesma fila** do fio real. A placa contou a injeção como prova do
fio e afirmou, com todas as letras, que o caminho de volta estava bom.

> **Instrumento que o próprio exercício engana não é instrumento.** O gatilho
> artificial tem que entrar por uma porta que o instrumento saiba distinguir —
> senão exercitar a defesa é o mesmo que falsificar a medição dela.

É a 13x vista do lado de quem mede: ali, "0 erros" onde se esperava vermelho
significava que o exercício não rodou; aqui, o exercício rodou **e
contaminou**. O conserto foi contar as injeções na origem (onde a origem é
conhecida) e descontá-las do veredito, mostrando os dois números:

    volta: 0 pelo FIO + 3 injetados por `miditest`   (o fio NUNCA foi provado)

**Contar na origem e não no destino** é o ponto: lá embaixo o parser já não
distingue, e nenhuma heurística recuperaria a diferença.

### Os dois ramos, exercitados

Pela 13x — o gatilho também precisa ser exercitado:

- **AVISO**: com o fio mudo, `mandei 86 e NADA voltou`, e nomeia as **três**
  causas possíveis em vez de escolher uma;
- **OK**: com cinco ecos injetados por `miditest` (injeção local, sem fio),
  `5 voltaram, 5 barrados`;
- e o sub-ramo `Voltou MENOS DA METADE`, que também disparou.

## 13z. O painel acendeu de primeira, e as três decisões saíram do fonte (23/08)

O OLED SSD1309 subiu no primeiro voo: tela cheia, depois `COTUCATRONICS`
legível. **Nenhuma das três decisões do caminho foi tomada por tentativa.**

| pergunta | como se respondeu |
|---|---|
| MISO gasta um pino? | `direction = TWO_LINES_TX_ONLY` e `miso = Pin(PORTX,0)` — o "nenhum pino" da casa. **Não gasta.** |
| qual polaridade/fase? | o default já é `LOW` + `ONE_EDGE` = **modo 0**, que é o que o SSD1309 quer |
| quais pinos configurar? | **três dos cinco soldados já eram os defaults** — CLK D8, DIN D10, CS D7. Só DC e RST precisaram de override |

O dB tinha antecipado o MISO como *"decisão que gasta pino"* e pedido que eu
reportasse antes de escolher. **A decisão não existia** — trinta segundos de
`grep` no cabeçalho da libDaisy dissolveram a pergunta. É a lição da 13j na
forma barata: **antes de escolher, leia a configuração da fonte.**

**E o RST fora do default foi acerto de quem soldou, não meu:** o default é
`PORTB15` = **D30**, reservado para a leitura dos DPDT do exciter. Usar o
default criaria uma disputa que só apareceria no dia de soldar o painel.

### A afirmação que não se pode fazer, com o painel aceso na frente

O barramento é `TX_ONLY`, o painel **nunca responde**, e o `Init` da libDaisy
é `void`. Logo:

> **Este display não é detectável, hoje nem nunca.** Não existe caminho de
> leitura. O `board` diz **CONFIGURADO** e jamais "presente".

A tentação aqui é maior do que nos casos anteriores justamente porque **a tela
está acesa e o operador está vendo**. Mas o que ele vê é uma verificação que
a *placa* não consegue fazer — e amanhã, com o fio solto, ela diria
"configurado" com exatamente a mesma confiança. Escrever "OLED presente"
seria o `placa viva: sim` outra vez, agora com um periférico incapaz de
desmentir.

### Texto prova mais que `Fill`

Tela cheia mostra que o painel liga e que CLK e DIN chegam. **Texto prova
endereçamento de pixel** — dado coerente, na ordem certa, e o **DC separando
comando de dado**. Um painel com DC trocado *acende inteiro* e não escreve
nada legível.

> **Ao provar um caminho, escolha o teste que falha de forma diferente para
> cada defeito possível.** `Fill` e texto custam o mesmo e distinguem coisas
> diferentes.

### O prescaler alto foi escolha, não cautela vaga

`PS_64` no primeiro voo, declarado no código: fio comprido e solda de bancada
perdoam menos em MHz alto, e este teste **não tem requisito de taxa**. Se
acender, dá para descer e **medir onde para de acender** — que é informação
sobre a instalação, não chute.

## 14a. `open(f,'w')` trunca ANTES de o `read()` interno rodar (23/08)

```python
io.open(f, 'w').write(io.open(f).read().replace(velho, novo))
```

Parece uma linha e é duas: **o `open(f,'w')` é avaliado primeiro**, trunca o
arquivo, e só então o `read()` interno lê — um arquivo já vazio. Resultado:
`manifest.h` (83 KB) e `test_midi.cpp` (21 KB) foram a **zero bytes**.

O que denunciou não foi o script — ele terminou dizendo `seis corrigidos`.
Foi o banco nativo, na volta:

    Undefined symbols: _main
    error: no type named 'Manifest' in namespace 'cotuca'

> **Nunca ler e escrever o mesmo arquivo na mesma expressão.** Ler primeiro,
> fechar, só então abrir para escrita. É o padrão de dois passos, e os quatro
> arquivos do mesmo lote que o usaram **sobreviveram** — a diferença estava
> em duas linhas de Python, não em sorte.

### Por que foi barato, e o que teria sido caro

Recuperado com `git checkout HEAD -- <arquivos>` e os bumps refeitos. Custou
dois minutos **porque as duas condições estavam satisfeitas ao mesmo tempo**:

- os arquivos estavam **commitados** (HEAD era de minutos antes);
- a única mudança não commitada era **o bump de versão**, refazível de cabeça.

**Se eu tivesse rodado isso no meio de uma edição grande não commitada, o
trabalho teria sumido.** A cicatriz não é "cuidado com o Python": é que
**commitar cedo é o que transforma um destroço em inconveniente.**

### E o verificador de ponto cego nasceu com ponto cego

A regra de deriva exigia `//` ou a palavra `version` na linha adicionada — e
os scripts Python da casa declaram a versão em **docstring**:

    """nivel_tempo v0.2.0 — RMS em janelas de 50 ms."""

Resultado: dois arquivos **corretos** ficaram acusados de deriva, na mesma
sessão em que a regra foi escrita. **O verificador de ponto cego tinha ponto
cego de linguagem.**

> **Regra que reconhece um formato reconhece um dialeto, não um conceito.**
> Ao escrever qualquer verificação por padrão de texto, listar as formas que o
> próprio projeto usa — aqui eram três (`//`, `#`, docstring) e a primeira
> versão cobria uma.

### E o instrumento que apanhou o erro não era o de destino

O `check-docs` novo — escrito quinze minutos antes para vigiar deriva de
versão — continuou acusando os seis, e eu quase li isso como "o conserto não
pegou". Quem gritou foi o **compilador**, por um caminho que nada tinha a ver.
Quinto caso do padrão: **instrumentação encontra o que não era o alvo dela** —
aqui, o alvo nem era instrumentação, era o build.

## 14b. Reintroduzir o método aposentado sem reintroduzir o mecanismo (23/08)

Com os learns do Loopy apagados, escrevi o `RETRABALHO-LOOPY.md` como uma
tabela de 86 linhas e o entreguei dizendo *"remapeia a partir disto"*. O dB
perguntou: **"não tínhamos decidido substituir o midi learn pelo sync? por que
isso volta agora?"**

Ele estava certo, e o defeito não estava no documento — **estava no
enquadramento.** A tabela é referência legítima; apresentá-la como
*procedimento* trouxe de volta o trabalho manual que o método de 22/08 tinha
eliminado.

### A decisão original era mais forte do que eu lembrava

O `learn` não foi aposentado por ser chato. Foi porque **exigia a placa
surda** — descartava todo CC que chegasse, por desenho, senão os widgets já
armados atropelavam a caminhada. E o `vistos` lê exatamente o que chega:

> **Ligar o learn cegava o único instrumento capaz de provar que o learn
> funcionou.** Método que exige desligar o sensor que o verificaria é desenho
> invertido: o certo deixa o mundo agir e observa, o errado age e supõe.

### O que eu deveria ter feito, e que estava a um `grep` de distância

O disparo pontual que o `learn` tentava fazer **já existe sem o defeito**:
`midiloop <cc> <val> <canal>` manda um CC específico pelo fio real, sob
demanda, **e a placa continua escutando o tempo todo**. Arma o widget,
dispara aquele, verifica com `vistos` no fim.

O mecanismo foi aposentado; **o método não**. Eu tratei os dois como a mesma
coisa e reintroduzi trabalho manual em vez de procurar a peça que sobreviveu.

### E eu repeti o erro uma camada abaixo, na propria correcao

Corrigi o enquadramento e escrevi o procedimento novo assim: *"arma o widget
no Loopy, e o Code dispara aquele CC"*. O dB perguntou o que significava
"armar".

**Significa preparar o MIDI learn — o mecanismo aposentado.** Eu tinha tirado
a tabela do lugar de procedimento e mantido, dentro do procedimento corrigido,
um passo do mecanismo que a decisão removera. **Corrigir a moldura e preservar
a peça.**

O procedimento em vigor não prepara nada no iPad:

| aposentado (`learn`) | em vigor (`vistos`) |
|---|---|
| tu armas o widget | tu **varres** os widgets |
| a placa dita o número | a placa **escuta** o que chega |
| a placa fica **surda** | a placa nunca para de ouvir |
| **o widget se adapta** | **o firmware se adapta** |

> **Quem se adapta é o firmware, que é o lado que se edita.** Essa frase estava
> escrita no procedimento desde 22/08, e eu passei duas rodadas propondo o
> inverso.

**A palavra denunciou o que a estrutura escondia.** "Armar" não tinha lugar no
método novo, e foi por ela — não pela lógica — que o defeito apareceu. Vale
como sinal: **vocabulário emprestado de um mecanismo aposentado é indício de
que o mecanismo veio junto.**

> **Ao retomar um problema que já foi resolvido, releia COMO foi resolvido
> antes de propor caminho.** Uma decisão registrada tem duas partes — o que
> foi removido e o que ficou no lugar — e lembrar só da primeira metade
> produz exatamente a regressão que a decisão evitava.

**E o registro estava escrito e eu não o li:** o `PROCEDIMENTO-MIDI-LEARN.md`
abre com *"método que funcionou: `vistos`, não `learn`"*, 65 de 65 em duas
varreduras. O commit `ec756ef` nomeia o caminho de volta inteiro. **A casa
tinha feito a parte dela; faltou eu consultar.**

## 14c. Prefixo engole comando, e a licao estava dois comandos acima (23/08)

`oledvel 256` casou no handler do **`oled`** — `strncmp(line, "oled", 4)`
vem antes na cadeia e engole o prefixo.

**E o arquivo JA DOCUMENTAVA esta armadilha**, dois comandos de distancia:

> *"ATENCAO a ordem: `miditest` e `midiloop` TEM que ser testados antes de
> `midi`, senao o strncmp de 4 letras engole os dois."*

Licao escrita a dois comandos de distancia e licao nao aplicada.

**E o sintoma foi o pior tipo:** o comando RESPONDEU, com uma mensagem
plausivel (`tela ACESA`), executando outra coisa. Sem ler a saida crua eu
teria concluido que o PS_256 nao resolveu — quando ele nunca chegou a ser
aplicado. Comando que falha em silencio se percebe; comando que faz **outra
coisa** e reporta sucesso, nao.

> **Despacho por prefixo exige ordem, e ordem e invariante nao verificada.**
> Todo comando novo cujo nome COMECE com o nome de um existente tem que vir
> antes dele — e nada no codigo cobra isso. Candidato para o `check-docs`:
> comparar cada `strncmp(line, "X")` com os demais e acusar prefixo posterior.

## 14d. Supor o mapa e medir contra a suposicao so acha o que ja se esperava (23/08)

O encoder global nao respondia. Construi tres instrumentos para descobrir por
que — a trava de nivel cru nas duas direcoes, o teste diferencial, a sonda de
fio — e **todos apontavam para `D20`/`D21`**, porque era ali que eu supunha que
o fio estivesse.

Ele estava em **`D18`/`D19`**, com o push em `D17`.

Os instrumentos funcionavam perfeitamente. E **nenhum deles podia dizer "o fio
esta noutro pino"**, porque nao olhavam para la. Cheguei a tres diagnosticos
diferentes — terra solto, comum na ponta, fase no pino do push — e os tres
eram a mesma coisa vista pelo buraco errado.

> **O instrumento herda o ponto cego da hipotese que o gerou.** Supor o mapa e
> depois medir contra a suposicao produz medicoes internamente coerentes e
> globalmente erradas.

O sintoma que devia ter me alertado: **girar movia o pino que eu chamava de
"push"**. Botao nao muda de estado sozinho enquanto alguem gira um eixo. Eu li
isso tres vezes e inventei tres explicacoes em vez de suspeitar do mapa.

**A saida foi alargar o escopo em vez de refinar a mira:** o comando `varre` le
**todos** os quinze pinos livres, nao os nove escolhidos. Se existisse na
primeira rodada, teria dito `D17 D18 D19 COMUTAM` em dois minutos.

### E a rotina que o dB pediu, com a razao dele

> *"Esse e um projeto complexo. A placa esta entupida de fios e os pontos de
> solda espremidos entre espacos bem apertados. Vamos sempre revisar pinos
> contra o esperado."*

`make check-docs` passou a conferir o mapa de `PINOS.md` contra os `seed::Dnn`
do fonte, em tres colunas: **no codigo e fora do doc · no doc e nao usado ·
RESERVADO sendo usado.**

**E ele achou algo na primeira execucao**, como todo instrumento novo deste
projeto: `D7`, `D8` e `D10` estavam no doc e **nao apareciam no fonte**. Sao o
SPI do OLED, que vinha dos **defaults da libDaisy** — tres pinos ocupados sem
que o nosso codigo dissesse.

> **Ocupacao que vive so no default de uma biblioteca e o gemeo da reserva que
> vive so em documento.** Nos dois casos o compilador nao sabe, e o custo
> aparece na solda: quem atribuisse `D7` a outra coisa nao encontraria conflito
> em lugar nenhum.

Consertado declarando os tres explicitamente, embora sejam os defaults. Nao
muda comportamento — **muda quem sabe.**

## 14e. Medi presenca e chamei de polaridade (23/08)

O painel nao acendia. Eliminei sete causas por prova e concluí, por
eliminacao, que o display estava morto — apoiada em *"3,3 V confirmado
chegando ao OLED"*.

**Eu medi MAGNITUDE e chamei de ALIMENTACAO OK.** O GND e o 3V3 estavam
invertidos na placa de distribuicao, e um trilho invertido da exatamente a
mesma leitura no multimetro se as pontas seguirem a mesma convencao errada.

> **Tensao presente nao e tensao correta.** Antes de riscar "alimentacao" da
> lista de suspeitos, medir com o SINAL — ponta preta num terra conhecido do
> lado de quem alimenta, nunca do lado de quem recebe.

E a inversao explicava **os dois** problemas da bancada: o painel sem acender
e os nove pinos dos encoders em `so alto`. Uma causa, dois sintomas, e eu
tinha construido dois diagnosticos separados.

**O agravante e que eu tinha o contra-dado e o ignorei:** escrevi, com todas
as letras, que o MCU estava rodando e que isso exigia 3V3 em algum lugar.
Depois segui uma leitura de multimetro que dizia 0 V no pino. **Duas evidencias
incompativeis deviam ter suspendido a conclusao, e eu escolhi um lado.**

## 14f. Instrumento nao inicializado le zero, e zero pareceu dado (23/08)

O encoder global tinha as duas fases COMUTANDO no `encpino` e `crus +0` no
`enc`. Diagnostiquei hardware: mandei o dB conferir o comum no multimetro, e
o comum estava certo.

A causa era minha: **`EncoderInit()` so era chamado pelo comando `enc`**, e o
`EncoderPoll` do laco retornava cedo enquanto isso. Girar antes de digitar
`enc` nao contava nada. O `encpino` funcionava porque tem GPIOs proprios.

E **o defeito estava anotado ha horas**, com estas palavras: *"os encoders nao
funcionam ate alguem digitar `enc`, o que e aceitavel num voo de bancada e
inaceitavel quando o painel for tocar"*. Deixei para depois, e o "depois"
cobrou uma rodada de diagnostico de hardware.

> **Defeito conhecido e adiado nao fica parado: ele vira a explicacao errada
> do proximo sintoma.** E o custo nao recai sobre quem adiou — recai sobre
> quem esta com o multimetro na mao.

**A discordancia entre dois instrumentos foi o que salvou.** Um deles nao
dependia do que o outro dependia, e por isso a contradicao apareceu em vez de
virar conclusao. Vale como desenho: **quando um instrumento novo mede o mesmo
fenomeno por outro caminho, a divergencia entre eles e informacao — nao ruido
a reconciliar.**

## 14g. Teste o hardware primeiro, de todas as maneiras possiveis (23/08)

Passei uma sessao inteira eliminando causas do OLED preto: firmware,
alimentacao, encoders, fio rompido, ponte, velocidade, ordem de energizacao.
Sete suspeitos, todos por prova, e a conclusao foi "o painel morreu".

**O painel estava vivo.** O dB conseguiu acende-lo **torcendo o conector** —
tres vezes. Era **contato intermitente**, e nenhum dos meus testes podia
te-lo achado.

### Por que nenhum podia

Olhando a bateria que construí — continuidade, ponte entre pares, nivel cru,
sonda de fio segurando nivel, varredura de prescaler, binario antigo —
**todos sao estaticos**. Nenhum envolve MEXER em nada.

> **Contato intermitente e invisivel a teste estatico por definicao:** a
> continuidade passa com a ponta apoiada e falha quando o fio e puxado de
> lado. Medi com o sistema parado um defeito que so existe com o sistema em
> movimento.

### E o sinal estava na minha frente, escrito por mim

Eu escrevi, com estas palavras, sobre os encoders: *"funciona, tu manuseias,
para de funcionar. Aconteceu duas vezes hoje."* Usei isso para pedir
estabilidade mecanica — e **nunca o converti em hipotese**.

> **Intermitencia correlacionada com manuseio E o diagnostico, nao um estorvo
> que atrapalha o diagnostico.** Quando o alvo muda entre medicoes, a mudanca
> e o dado; insistir em medir com mais precisao so registra a mudanca com
> mais casas decimais.

### O culpado, e ele estava no lugar mais previsivel

O conector **JST** do modulo. Torcer o conector acendia o painel; solto,
nada. O dB tentou soldar direto no JST e *"tudo cai por terra"* — as ilhas
daquele conector nao aguentam retrabalho.

**Nao era o painel, nao era a fiacao, nao era o firmware.** Era o unico
elemento que ninguem inspeciona porque parece infraestrutura: o conector.

E o desfecho e barato — painel novo, e o velho fica de reserva, porque ele
esta provadamente vivo.

### A regra que o dB pediu, e ela vai antes das minhas

> **Antes de qualquer teste de software, exercite o hardware fisicamente:
> torca, puxe, aperte, mexa em cada conector — e observe SE ALGO MUDA.**

E o corolario que a bateria de hoje ensina: **acrescente uma dimensao de
movimento a cada sonda.** O `fio <pino> <0|1>` segura um nivel para medir no
conector — a versao util e medir **enquanto se mexe no chicote**. O `varre`
acumula desde o reset — a versao util e varrer **enquanto se torce a placa**.

Sonda que so responde com tudo parado tem um ponto cego do tamanho de todas as
falhas mecanicas — que num projeto com a placa entupida e os pontos de solda
espremidos sao a maioria.

## 14h. Documento gerado acompanha; documento escrito a mao, nao (23/08)

A multiplexacao mudou de quatro canais para seis. **Um documento se atualizou
sozinho e quatro ficaram para tras** — e o que decidiu nao foi importancia nem
visibilidade: foi **ter gerador**.

    RETRABALHO-LOOPY.md    gerado do manifest      -> acompanhou
    CCMAP.md               escrito a mao           -> ficou em 4 canais
    ACEITE-MULTIPLEXACAO   escrito a mao, 7 hits   -> ficou em 4 canais
    PRESETS.md             escrito a mao           -> ficou
    STATUS.md              escrito a mao           -> ficou

E o pior deles e o roteiro de aceite: ele manda conferir `ch3`/`ch4` no MIDI
Wrench, e a placa fala `ch4`/`ch5`/`ch6`. **Documento errado nao fica parado —
ele manda alguem ao lugar errado com o multimetro na mao.**

> **Para cada mudanca estrutural, separe os documentos que a descrevem em
> GERADOS e ESCRITOS. Os escritos sao a lista de trabalho.**

### E o gerador tambem tem premissa — a diferenca e que a dele grita

Ao regerar o `CCMAP.md` o gerador **falhou**: a tabela de canais dele dizia
`{kCanalEntradaA: 1, kCanalEntradaB: 2}`, escrita para DOIS canais, e a
mudanca para tres a deixou com `kCanalCompart` desconhecido.

**Ele nao emitiu tabela torta — abortou nomeando os doze CCs que nao
resolveu.** Era o desenho: *"sai com codigo 1 se algum CC nao resolver, em vez
de emitir tabela incompleta: tabela gerada com buraco e pior que tabela
ausente."*

> **Gerador nao dispensa manutencao — dispensa DISCIPLINA.** A premissa dele
> envelhece igual, mas ele grita em vez de mentir, e gritar e o que transforma
> manutencao em tarefa em vez de armadilha. Os gerados nao
> precisam de disciplina; os escritos so tem disciplina.

E o corolario que vale mais que a regra: **o que se pode gerar, gere.** O
`RETRABALHO-LOOPY.md` nao acompanhou por virtude de ninguem — acompanhou
porque um script o refaz. Todo documento que descreve uma tabela do codigo e
candidato a gerador, e cada um que vira gerador sai da lista de disciplina
para sempre.

---

## 14i. Cinco padroes de mensagem, todos dentro do conjunto errado (24/08)

O Loopy Pro nao aprendia o `cc21`. Testei cinco formas de disparo:

| disparo | veredito do Loopy |
|---|---|
| 30/110 alternados a 500, 1000, 1500 ms | `double tap` nas tres |
| rampa densa `0→127→0` a 60 ms | `hold` |
| rampa esparsa, 5 valores a 250 ms | `hold` |
| so 127 a cada 1 s | alterna `cc` e `hold` |
| so o miolo 34–100, sem tocar nos extremos | `double tap` |

Cinco resultados diferentes, **e todos gestos de botao**. Eu estava a escolher
QUAL botao, nao a sair do conjunto dos botoes. Cada falha parecia informacao
nova — era a mesma informacao com outra roupa.

O que quebrou o ciclo foi o dB olhar a UI: **o `cc20` era um dial e o `cc21`
um slider.** Mesmo protocolo, mesma placa, mesmo canal, widgets diferentes —
e a mensagem tinha sido identica nos dois. Nenhuma variavel da mensagem podia
explicar uma assimetria que a mensagem nao tinha.

> **Quando N tentativas falham de formas DIFERENTES mas todas do mesmo tipo, o
> tipo e a resposta.** A variedade dos vereditos disfarcou o invariante: eu li
> "double tap, hold, double tap" como tres pistas e era uma so, repetida.
> Antes da tentativa N+1, escrever o que TODAS as falhas tiveram em comum — e
> se o comum nao estiver no espaco que se esta a varrer, parar de varrer.

Ja tinha nomeado este erro uma hora antes, na mesma sessao, sobre o
espacamento (14k). Nomear a familia nao imuniza contra ela com outra variavel.

---

## 14j. O limiar era zero, e eu deduzi-o em 64 (24/08)

Todo o raciocinio sobre "o que o Loopy ve" assentava numa suposicao minha: que
valor de CC acima de 64 e ON e abaixo e OFF, como um botao com meio curso.
Nunca a escrevi como suposicao — usei-a como facto para explicar tres falhas.

A documentacao, quando o dB a colou, dizia outra coisa:

> "For CC messages, ON is a value **greater than 0** and OFF is a value of 0."

Com o limiar certo, as cinco falhas explicam-se ao milimetro. E a pior delas
inverte de sentido: mandar **so o miolo** foi a tentativa mais cuidadosa e a
mais destrutiva, porque evitar o zero significa **nunca produzir um OFF**, e
uma corrente so de ONs a 120 ms e a definicao literal de double tap. Eu tinha
desenhado esse disparo exatamente para fugir da ambiguidade dos extremos.

> **Constante de protocolo se le, nao se deduz do comportamento.** Eu tinha
> cinco medicoes e um numero inventado; as medicoes eram boas e a leitura
> delas era ficcao. E o numero deduzido nao aparece como suspeito porque
> EXPLICA os dados — encaixa-se em tudo, so que ao contrario.

---

## 14k. Tres medidas na mesma variavel sem mover o fenomeno (24/08)

O dB pediu espacamento de 500 ms, depois 1000, depois 1500, depois 2000 —
cada um porque o anterior "ainda deu double tap". Quatro pedidos, uma variavel,
zero movimento no resultado.

> **Quando tres medidas mexem numa variavel e o fenomeno nao se move, a
> variavel esta errada** — e o instinto de continuar a subir o numero e o que
> segura a investigacao no lugar. A escalada parece progresso porque cada
> passo e uma acao; mas uma serie que nao muda o resultado nao e uma serie de
> experimentos, e uma medicao repetida quatro vezes.

O sinal barato: **antes do quarto valor da mesma variavel, escrever o que o
terceiro descartou.** Se a resposta for "nada", trocar de variavel.

---

## 14l. A sonda que nunca podia ouvir, e o `ok` que ela devolvia (24/08)

Para saber se um widget mandava MIDI de volta, liguei `log on` e li a serial
durante 20 s enquanto o dB mexia no controlo. Zero bytes. Conclui que o widget
nao mandava.

Fiz o controlo de presenca — mexer no widget que SABIAMOS que mandava. Zero
bytes tambem. **O `log on` nao imprime MIDI de entrada.** A sonda nunca teve
como ouvir nada; eu estava a ler silencio num canal onde a presenca era
impossivel, e a chamar-lhe ausencia.

O instrumento valido estava a dois comandos de distancia e ja tinha sido usado
nesta mesma sessao: o `vistos`, que e estado da placa e nao fluxo de log.

> Isto e a `13x` — *"ausencia so e evidencia quando a presenca era possivel"* —
> a acontecer pela quarta vez em dois dias, agora com a agravante de eu ter
> construido a sonda no proprio dia. **Sonda nova nasce sem procedencia: o
> primeiro uso de qualquer sonda e o controlo de presenca, nao a medicao.**
> Custou-me um minuto fazer o controlo depois; teria custado o mesmo antes, e
> a conclusao errada nao teria existido.

---

## 14m. O artefacto exportado era a fonte que faltava (24/08)

O trabalho a frente era configurar 85 bindings a mao no iPad — um dia inteiro,
por gesto, sem verificacao possivel. A saida nao foi acelerar o gesto: foi
**parar de gestionar**.

O manual mencionava, de passagem, que perfis de controlo **exportam**. Custou
dois bindings feitos a mao e uma exportacao para descobrir que:

- o perfil e um **bundle de plists XML**, um ficheiro por dispositivo;
- o gatilho e hex: `b<canal-1><cc>[<valor>]`, sem valor = continuo;
- o projeto e um **zip com um SQLite**, e os widgets sao JSON com `title`;
- `Scene` = pagina, com um array ORDENADO de widgets.

Um dia de configuracao virou `gerar_perfil_loopy.py`, que le os CCs do
`manifest.h` e emite os 86 bindings ja endereçados.

> **Antes de aceitar trabalho manual repetitivo, procurar o formato de
> intercambio.** Toda aplicacao que deixa exportar tem um formato, e formato e
> superficie programavel. A pergunta nao e "como faco isto mais depressa" — e
> **"onde e que isto vive quando nao esta na tela"**.

E o corolario do projeto aplicou-se inteiro: o que se pode gerar, gere. O mapa
de CCs saiu da lista de disciplina do dB para sempre — muda o `manifest.h`,
reexporta o projeto, regera o perfil.

---

## 14n. Validar o gerador contra verdade conhecida ANTES de emitir (24/08)

O gerador tinha de adivinhar como um binding aponta para o seu widget. A forma
era `LPSliderWidget:1:2` — tipo, pagina, indice — e o indice **nao** e a posicao
na tela nem a ordem da nossa lista: e a posicao no array `widgets` da `Scene`,
contada por tipo.

Eu podia ter emitido 85 bindings e mandado o dB testar no iPad. Em vez disso,
o perfil exportado trazia **dois bindings feitos a mao** — `b417` e `b314` — e
esses eram verdade conhecida. O gerador tinha de os reproduzir ao caracter
antes de eu confiar nos outros 84.

Reproduziu. `b417 → LPSliderWidget:1:2` e `b314 → LPSliderWidget:1:6`, exatos.

> **Todo gerador de formato desconhecido tem que reproduzir uma amostra
> conhecida antes de emitir o resto.** Sem isso, 85 bindings plausiveis sao
> indistinguiveis de 85 bindings errados, e a deteccao passa para o ouvido do
> musico, dias depois, um por um. Duas amostras custaram dois gestos e
> compraram confianca em oitenta e quatro.

A confirmacao cruzada foi de graca por cima: os unicos dois bindings no
ficheiro eram exatamente os dois que a placa reportara como VOLTOU no teste
de bancada, e o `cc21`, que reportara nada, nao existia no XML. **Duas
medicoes independentes — uma pela placa, outra pelo XML — a dizerem o mesmo.**

---

## 14o. Endereco posicional desendereça em silencio (24/08)

O `Subject` de um binding e posicional. O dB corrigiu o layout — moveu o
`Dly Time` para a pagina CENTRAL — e regerei o perfil. A conferencia contra os
dois bindings conhecidos deu **DIVERGE**:

```
b417  gerado=LPSliderWidget:1:3   antes era :1:2
b314  gerado=LPSliderWidget:1:7   antes era :1:6
```

Nao era defeito do gerador: **acrescentar um slider empurrou todos os
seguintes uma posicao**, e os dois bindings que o dB fizera a mao passaram a
apontar para os widgets errados. Sem erro, sem aviso, sem sintoma ate alguem
mexer no controlo e ouvir outra coisa.

> **Endereco por posicao transforma edicao de layout em reendereçamento
> silencioso.** E a familia da mensagem mal-endereçada (`13*`): ela obedece, e
> por isso nao ha nada para correlacionar. A defesa e nao ter enderecos
> posicionais escritos a mao — regerar o conjunto TODO depois de qualquer
> mexida no layout, para que a posicao nunca seja informacao que so existe
> numa configuracao antiga.

O aviso ficou no cabecalho do gerador — e horas depois **deixou de ser
necessario**, o que e a segunda metade desta cicatriz.

**EMENDA, no mesmo dia.** Ao abrir o perfil de DENTRO do `.lpproj`, os
bindings feitos a mao apontavam para `#1522` — o `id` do widget no SQLite,
nao uma posicao. O `LPSliderWidget:1:6` que eu tinha decodificado e tomado
por lei e uma **CONVERSAO**: o Loopy troca id por posicao quando exporta o
perfil sozinho, porque fora do projeto o id nao significa nada.

Ou seja: **eu decodifiquei o formato de exportacao e chamei-lhe o formato.**
A forma fragil era a unica que eu tinha visto, e vi-a porque olhei para o
artefacto exportado em vez do artefacto interno. O gerador passou a v0.2.0 a
emitir `#<id>` para dentro do projeto, e a fragilidade posicional deixou de
existir nesse caminho.

> **O formato de intercambio nao e o formato interno** — e uma projecao, e
> projecoes perdem informacao (aqui, a identidade). Ao decodificar por
> engenharia inversa, perguntar SEMPRE se o artefacto que se tem em maos e o
> que a aplicacao guarda ou o que ela exporta. As duas respostas coexistem, e
> a exportada e quase sempre a mais pobre.

E o que salvou: a mesma disciplina da `14n`. Os dois bindings feitos a mao
eram verdade conhecida nas DUAS formas — `LPSliderWidget:1:2` no perfil
exportado e `#1525` dentro do projeto — e o gerador teve de reproduzir cada
uma no seu contexto. Foi a segunda reproducao que revelou que havia duas.

---

## 14p. O verificador contava zero CCs e assinava "ok" (24/08)

Na revisao de fecho, o `check_docs` imprimiu:

```
manifest: 86 params (49 familias), 0 CCs
...
ok — nenhum fantasma
```

**Zero CCs** — e o meu gerador acabara de extrair 86 `AddMidi` do mesmo
ficheiro. O regex era:

```python
r'AddMidi\((\d+),\s*([A-Za-z_0-9\[\]]+)\)'
```

Dois argumentos e parentese a fechar. O `AddMidi` passou a receber **tres**
quando a multiplexacao por canal entrou — e desde essa sessao o verificador
conta zero e diz `ok`. Quantas sessoes, nao sei dizer: nao ha registo, porque
o numero nunca chamou a atencao.

Duplo defeito, e o segundo pior: `ccs[int(num)] = alvo`, indexado so pelo
numero do CC. Com seis canais, o mesmo numero vive em tres — **mesmo com o
regex certo, dois tercos seriam apagados em silencio.** A estrutura envelheceu
junto com o padrao, e nenhum dos dois foi reexaminado quando o canal entrou no
endereco.

Consertei os dois e pus a defesa que faltava: **se `AddMidi` nao casar nenhuma
vez, aborta com codigo 2 em vez de imprimir zero.** Exercitada com uma forma de
quatro argumentos — zero casamentos, aborta.

> Isto e a `13n` — *"quando o canal vira parte do endereco, toda estrutura
> indexada por CC tem que ser reexaminada, e o compilador nao cobra nenhuma"* —
> com o corpo de um verificador em vez de firmware. **O regex tambem nao cobra.
> E pior: ele responde.** Uma estrutura que envelhece cala-se; um verificador
> que envelhece devolve um numero e um `ok`.
>
> **Todo contador de um verificador precisa de um piso: zero onde se esperava
> populacao aborta, nao informa.** Um `ok` ao lado de um zero e a assinatura
> desta familia, e e legivel a olho nu na primeira linha do relatorio — se
> alguem a estiver a ler em vez de a saltar para os avisos.

---

## 14q. O suspeito e a causa recuperaram JUNTOS (27/08)

A Daisy sumiu do barramento. Pedi troca de cabo; 22 s depois o STM32
reapareceu. Anunciei: **"era o cabo"**.

Nao era. Ao soldar o terra do DPDT houve um curto que derrubou o **dongle
USB** inteiro — SSD, placa de som e a Daisy junto —, e ele recuperou por
volta do mesmo instante em que o cabo mudou. **Correlacao perfeita e falsa.**

> Isto e a `13j` (hipotese plausivel com explicacao completa) com um
> agravante que ela nao previa: **quando o suspeito e a causa se movem ao
> mesmo tempo, a coincidencia e indistinguivel da confirmacao.** Nenhuma
> quantidade de "medi e melhorou" separa os dois.
>
> **Um hub no meio e uma variavel que nao avisa quando muda.** Enquanto se
> depura, as placas vao DIRETAS ao host — nao para explicar a variavel, para
> a ELIMINAR.

---

## 14r. Descartei a massa por deducao, e ela estava desligada (27/08)

Dois fios de tres nao funcionavam. Escrevi: *"isso descarta massa comum como
causa unica, porque as duas placas partilham terra pelo Mac"* — e o
argumento e razoavel: ambas estavam na mesma USB.

O dB respondeu: **"nao tinha ligado o gnd, liguei agora."**

O raciocinio nao era absurdo; era uma DEDUCAO apresentada como eliminacao.
E custou uma rodada inteira de instrumentos novos para provar fios que
teriam funcionado ao primeiro toque.

> **Eliminar por argumento nao e eliminar.** A diferenca entre "isso nao pode
> ser" e "medi e nao e" e a diferenca entre uma hipotese descartada e uma
> hipotese escondida — e a escondida nao volta a lista.

---

## 14s. Mudei o nome com a medicao a caminho (27/08)

Calculei que "Cotucatronics" nao cabia no pacote de anuncio BLE: 31 bytes,
menos 3 de flags, menos 18 do UUID de 128 bit, sobram 8 caracteres para um
nome de 13. **A conta esta certa.** Gravei "Cotuca".

O dB ja tinha visto "Cotucatronics" aparecer no Loopy. O nome vai na
**resposta ao scan**, um segundo pacote de 31 bytes que o meu modelo nao
tinha. E a gravacao derrubou a ligacao que ele acabara de fazer, e deixou o
iPad com o nome curto em cache — tres identidades a discordarem.

> **Modelo completo nao e modelo correto.** Uma conta que fecha convence
> exatamente como uma que esta certa. E quando ha uma medicao a caminho — o
> dB a olhar para a tela — o custo de esperar por ela e zero e o custo de
> nao esperar foi este.

Terceira vez no mesmo dia (com `14q` e `14r`): **falei antes da medicao
chegar.** As tres tem a mesma forma e nenhuma foi por falta de dados — foi
por nao esperar pelos que vinham.

---

## 14t. Tres instrumentos mentiram em duas horas (27/08)

Nenhum deles avisou. Todos deram numeros com cara de resultado.

**`dfu-util -U` ignora o endereco do `-s`.** Li de `0x90040000` e de
`0x90000000` e recebi bytes IDENTICOS — o que so e possivel se as duas
leituras vierem do mesmo sitio. Com base na primeira eu tinha concluido
**"a flash DIVERGE do binario em 99,5%"** e estava prestes a tratar isso
como o achado do dia. O que o denunciou foi um teste que eu quase nao fiz:
ler DUAS regioes e comparar entre si.

**`pyserial` trava a abrir com o `CRTSCTS` ligado.** Seis tentativas
"expiraram" e eu registei-as como *a placa nao responde*. Abrir o descritor
com `O_NONBLOCK` e limpar o controlo de fluxo: escreve sete bytes sem
bloquear. **A placa respondia; o meu `open` e que nao voltava.**

**Um filtro de leitura estreito demais.** Imprimi so linhas com
`SDRAM|escreveu|online|volta 0`. Com a placa na volta 67, nada casava — e o
resultado foi *"so 'Daisy is online', logo travou depois do banner"*. A
placa estava a correr havia minutos.

> **Sonda que devolve zero tem de provar que consegue devolver diferente de
> zero.** As tres falhas produziram silencio, e silencio le-se como ausencia
> do fenomeno. O controlo positivo — mostrar que a sonda ACERTA num caso
> conhecido — nao e cerimonia: e a unica coisa que distingue "nao ha" de
> "nao vejo".

---

## 14u. O firmware minimo partiu o espaco em UMA gravacao (27/08)

Duas horas de hipoteses sobre por que o app enumerava o USB e nunca falava.
Bootloader, linker, `.bss`, QSPI, ITCM, `n_ramps`, `NowMs`, `BbFlush` —
todas verificadas por leitura, todas eliminadas, e o sintoma intacto.

O que resolveu foi parar de perguntar *"o que esta errado no app?"* e
perguntar **"de que lado esta o defeito?"**. Trinta linhas: `Init`,
`StartLog`, imprimir, piscar.

Falou a primeira. E com isso, numa unica gravacao:
- a placa esta boa
- a cadeia de gravacao esta boa
- o USB CDC esta bom
- **o defeito e do nosso app**

> **Quando o espaco de busca nao esta a encolher, pare de o varrer e
> BISSECTE-O.** Um programa que so faz o minimo nao e um teste fraco: e o
> unico cuja falha ou sucesso divide o problema ao meio, porque nao partilha
> nada com o suspeito.

Depois disso cada gravacao seguinte eliminou mais um suspeito — versao da
placa, init do audio, 64 MB de SDRAM escritos e relidos — e o
`teste_minimo` ficou como banco de provas permanente.

---

## 14v. O instrumento de autopsia tinha dois autores (27/08)

O projeto tem um sinalizador de arranque desenhado exatamente para este
caso: o callback de audio pisca a ultima FASE que o loop completou, porque
*"um instrumento que depende do paciente nao faz autopsia"*.

Ele disse 5. Fui as trinta linhas entre a `FASE(5)` e a `FASE(6)` e nao
havia nada capaz de travar: `NowMs()` e um `return`, `BbFlush()` e uma
limpeza de cache, e o laco das rampas e limitado a 8 pelo proprio `sizeof`.

**A contradicao era o achado.** Um `grep` por `SetLed` mostrou DOIS
escritores: o `LedFaseTick`, no callback, e o batimento do laco, na fase 7.
O padrao que se ve e a soma dos dois — e "5 piscadas" nunca significou fase 5.

> **Antes de explicar um numero, verifique quem o escreve.** E a familia C
> — variavel com dois empregos — aplicada ao proprio instrumento de
> diagnostico, que e onde ela custa mais: um contador partilhado da um
> numero errado, um SINAL partilhado da uma TEORIA errada, e a teoria
> consome horas antes de alguem duvidar dela.

E o registo do proprio projeto ja avisava, noutra entrada: *"6 piscadas em
laco sem enumerar"* foi resolvido com `program-boot`. **O mesmo sinal, duas
causas, consertos opostos.** O discriminante e o nome do produto no USB:
`DFU in FS Mode` e o bootloader, `Daisy Seed Built In` e o app.

---

## 14w. Hardware so se elimina por REPRODUCAO (27/08)

Uma placa que nao responde e sempre suspeita de si propria, e o argumento
"deve estar queimada" e confortavel demais para se examinar. A segunda
placa, nova da caixa, gravada do zero, fez **exatamente o mesmo**: enumera,
pisca, cala-se.

> **Duas unidades independentes nao falham igual por acaso.** A reproducao
> em hardware novo e a unica coisa que transfere a suspeita de volta para o
> software — e ela custa uma gravacao, nao um dia de leitura de codigo.

O corolario incomodo: **a placa antiga foi gravada com a v0.172.1 e nunca
mais falou**, e eu passei uma hora a suspeitar da minha propria alteracao
desse dia. Gravar o chao declarado (`03e6777~1`, v0.171.0) na placa nova
produziu o MESMO silencio — o que ilibou a alteracao em uma gravacao.
**Ter o chao declarado no inicio da sessao foi o que tornou isso um
experimento em vez de uma discussao.**

---

## 14x. A deteccao de placa e um fall-through silencioso (27/08)

A libDaisy identifica a Seed por pinos aterrados na fabrica: `PD3` para a
1.1, `PD4` para a Seed2 DFM, e **nenhum** para a rev4. O ramo final nao e
um erro — e o default.

As placas novas sao **Seed3**, produto que a libDaisy mais recente
(`v8.1.0-15`, e estamos a zero commits do master) ainda nao conhece.
Medido no proprio metal: `PD0..PD5 = 1 1 1 1 1 1`, nenhum aterrado. Logo
`CheckBoardVersion()` devolve **rev4 · AK4556** — o codec errado, sem aviso.

> **Deteccao por fall-through transforma "nao reconheco" em "e o mais
> antigo".** Nao ha excecao, nao ha log, e o sintoma aparece longe: audio
> ruim, atribuido a analogico. Toda deteccao de hardware devia ter um ramo
> DESCONHECIDO que grita — e quando nao tem, quem a usa precisa de a
> perguntar em voz alta (`CheckBoardVersion()`) em vez de a presumir.

⚠️ **E nao ha conserto local:** nao se pode forcar a versao, porque nao se
sabe qual codec a Seed3 tem. A informacao nao existe do nosso lado. Fica a
aguardar suporte na biblioteca — e o instrumento que o detectou (o
`teste_minimo`) fica gravado para reconferir quando ela subir.

---

## 14y. O terceiro sinalizador, e o conserto de 18/08 que ficou pela metade (27/08)

O `Log` tem tres estados que descrevem a mesma coisa — o canal esta mau:

    cdc_fails    quantas falhas seguidas
    cdc_dead     gate de 500 ms entre tentativas
    muted        reduz as tentativas de 3 para 1, e tira a espera entre elas

A ISR de recepcao rearmava DOIS:

    // REARME: chegou byte, o host esta vivo.
    cdc_dead  = false;
    cdc_fails = 0;

O `muted` era um `static bool` **dentro** do `Log` — fora do alcance da ISR
por construcao. Entao o canal declarava-se ressuscitado e continuava a UMA
tentativa por chamada, sem espera nenhuma. Uma tentativa isolada raramente
calha com o endpoint livre: o log ficava morto para sempre, com cara de
recuperado.

**O conserto de 18/08 estava certo e incompleto.** Naquele dia o problema foi
nomeado com precisao — *"a placa rodava perfeita e MUDA, e o sintoma (porta
viva, zero resposta, nem banner) era indistinguivel de firmware travado"* — e
a ressurreicao foi acrescentada. So que a um dos irmaos.

> **`bts 13q` em forma de sinalizador.** Quando um estado e descrito por mais
> de uma variavel, o rearme tem de as tocar a TODAS — e a que esta em escopo
> mais estreito e a que se esquece, porque nem sequer aparece de onde o
> rearme e escrito. Se a ISR nao pode ver a variavel, a variavel esta no
> sitio errado.

Custo: uma noite inteira a caçar um travamento que nao existia, e uma placa
declarada morta que talvez nao estivesse.

---

## 14z. `TransmitInternal` devolve OK antes de o canal servir (27/08)

O arranque deste firmware sempre se perdeu: `StartLog(false)` nao espera pelo
host, e tudo o que sai nos primeiros milissegundos cai num CDC que ninguem
abriu. A correcao obvia — esperar — tem uma armadilha na definicao de
"pronto".

**Versao 1 (funciona):** `System::Delay(5000)` cego. Sai o arranque inteiro.

**Versao 2 (nao funciona):** esperar ate o primeiro `TransmitInternal`
devolver `OK`, e sair ai. Medido: saem exactamente **2 bytes** — o proprio
ping — e nada mais. Nem o `Log` seguinte, nem o resto do arranque. E o RX
para junto.

Ou seja: **o `OK` chega antes de o canal estar utilizavel.** Sair no primeiro
sucesso e, na pratica, equivalente a nao esperar.

**Versao 3 (a que ficou):** minimo incondicional de 3 s, e SO DEPOIS a
tentativa por ate mais 2 s, com saida antecipada. O minimo e o que garante;
a tentativa e o que evita pagar o teto quando o host ja esta la.

> **Confirmacao de uma camada nao e prontidao da pilha.** Um `OK` de envio diz
> que aquela chamada foi aceite, nao que o outro lado esta a ler. E quando o
> criterio de "pronto" e o sucesso de uma operacao da propria camada que se
> quer testar, o teste passa cedo demais por construcao.

E a linha que fecha as duas entradas de hoje: **o instrumento de diagnostico
deste projeto e a serial, e foi ela que falhou.** Quando o canal que reporta e
o canal avariado, todo sintoma vira "travado" — e o unico caminho de volta e
uma carencia cega no arranque, que nao depende de nada estar a funcionar.

---

## 15a. O mapeamento tem duas metades, e a troca de hardware quebra so uma (27/08)

Depois de o ESP32 substituir o WIDI, o `sync` funcionava — os 86 widgets do
Loopy saltavam para o estado da placa — e **o fader nao comandava nada**.

O sintoma engana porque parece direccional: "funciona num sentido e nao no
outro" le-se como problema de fio, de canal, ou de radio. Nao era nenhum dos
tres. Sao **duas metades guardadas em sitios diferentes**:

| metade | onde vive | o que conhece |
|---|---|---|
| a placa move os widgets | perfil do DISPOSITIVO | id do dispositivo: `b2d29493` |
| os widgets comandam a placa | perfil `Internal`, 154 accoes `Send MIDI` | id do ENDPOINT: `e41c21ae` |

Os dois identificadores sao diferentes e **nenhum deriva do outro** — o WIDI
era `227f559e` como dispositivo e `9294b942` como endpoint. Regenerar o
perfil do dispositivo resolve metade e deixa a outra a apontar para um
destino morto.

> **Identidade de dispositivo e uma RELACAO, e ninguem a tem em ficheiro.**
> Ao trocar hardware, procurar TODOS os sitios que guardam uma referencia a
> ele — e a pergunta que os encontra nao e "onde esta o nome dele?", e "quem
> precisa de saber quem ele e?".

E a mesma galinha-e-ovo do dispositivo, com a mesma saida: **um gesto humano
cria o identificador** (mudar o destino de UM widget a mao), exporta-se, e os
outros 153 reescrevem-se de uma vez.

---

## 15b. O unico ganho aplicado cru era o mestre (27/08)

O dB moveu o fader do `level` depressa e ouviu um clique. Causa de FORMA, nao
de valor: o `level` era lido uma vez por bloco e multiplicado igual nas 48
amostras, entao cada bloco era um degrau. A 48 kHz sao 1 ms por degrau —
inaudivel devagar, transiente quando nao e.

O projeto ja tinha o suavizador e ja o usava no gate, no delay, no reverb e
na chave da sala. **Faltava exactamente onde mais doi**: o ganho mestre e o
unico ponto onde um salto aparece INTEIRO na amplitude, sem nada a seguir que
o disfarce.

> **Inventario de quem usa uma defesa acha o buraco mais depressa que o
> ouvido.** Um `grep` por `Suave` teria mostrado quatro consumidores e uma
> ausencia — e a ausencia estava no unico caminho que nao tem nada a jusante.
> Irmao desprotegido, outra vez, agora entre um efeito e o master.

E a correcao tem uma armadilha de forma propria: o `Passo()` corre **por
amostra**, dentro do laco. Chama-lo uma vez por bloco daria uma rampa mais
lenta com exactamente os mesmos degraus — mais suave no papel, identica no
fio.

---

## 15c. O instrumento de medida nao pode depender do que o musico deixou no fader (28/08)

O tom de teste era injectado DENTRO das cadeias, portanto passava pelo
`level` como qualquer som. Com o fader do utilizador em zero — que e onde
ele estava — o tom simplesmente nao existia, e a bancada ficou a procurar
defeito num sinal que a propria interface tinha silenciado.

> **Medicao que depende de estado de utilizador nao e medicao.** Um tom de
> referencia existe para dizer o que o sistema faz; se ele obedece ao mesmo
> controlo que o musico usa para tocar, ele passa a dizer o que o musico
> deixou configurado.

O conserto nao foi mover o ponto de injecao — foi perceber que sao **duas
ferramentas com o mesmo oscilador**, e que a escolha entre elas e do
operador:

    pre  (directo)  entra DEPOIS do `level`, antes do limiter.
                    Referencia limpa. O destino escolhe SAIDA.
    post (cadeia)   entra dentro das cadeias, como sempre.
                    Atravessa o processamento. O destino escolhe CADEIA.

E o destino passou a significar *"entre o que existe neste ponto"* — saidas
num modo, cadeias no outro. Mesma pergunta, respostas diferentes conforme
onde se esta.

⚠️ **Antes do limiter, nunca depois.** Uma via de teste que contorna a
protecao da saida deixa de ser instrumento e passa a ser risco: o exciter
nao sabe que aquilo era um teste.

---

## 15d. Nao era o instrumento — era o pre-amplificador, e a LINEARIDADE provou (28/08)

O canal esquerdo da gravacao tinha 3a harmonica a −10,7 dB (29% da
fundamental) e o direito nao tinha nenhuma. Mesmo tom, mesma placa, duas
entradas.

A tentacao era procurar assimetria entre as cadeias. O que resolveu foi
medir em **tres niveis** e olhar para a escala, nao para a harmonica:

    0,50 -> 0,25   esperado -6 dB    ESQ  -1,5 dB    DIR  -6,0 dB
    0,25 -> 0,10   esperado -8 dB    ESQ  -7,6 dB    DIR  -8,0 dB

**O direito segue o nivel ao decibel. O esquerdo comprime 4,5 dB no nivel
alto** — e a harmonica acompanha esse colapso exactamente (−10,9 → −28,3 →
−40,7), enquanto no direito fica cravada no piso de ruido.

> **Distorcao mede-se por LINEARIDADE, nao por harmonicas.** A harmonica diz
> que ha distorcao; a escala diz ONDE. E o argumento que fecha e estrutural:
> o limiter e o governor da placa ficam ANTES do encaminhamento — se algum
> estivesse a agir, os DOIS canais mostrariam a mesma compressao. So um
> mostra, logo a compressao acontece depois da placa.

Custo do conserto: um botao de ganho. Custo de nao ter medido: uma sessao
inteira de vereditos musicais sobre um pre-amplificador saturado.

---

## 15e. A arquitectura era pergunta de COBRE, nao de codigo (28/08)

Tres fontes discordavam sobre o que sao as duas saidas. O codigo somava as
duas cadeias nas duas (`out[0] = out[1] = A + B`). Os params chamavam-se
`route_exciter` e `route_jack`. O `HARDWARE.md` dizia *"OUT_L (cadeia A)"*.
Gastei a sessao a tentar decidir QUAL das tres estava certa, e montei a
pergunta como escolha entre duas arquitecturas: dois exciters, ou exciter +
jack.

**Nenhuma das duas.** O dB tem dois DPDT de tres posicoes — `jack | ambos |
amp` — um por saida. O destino nao e propriedade da saida: e posicao de
chave, e muda em operacao.

> **O firmware nomeia SAIDA; o cobre nomeia DESTINO.** `out[1]` nunca foi "o
> jack": e uma saida que PODE estar no jack. Um nome de param que carrega o
> destino esta a afirmar uma coisa que o firmware nao pode saber.

E o nome falso enganou DENTRO da sessao que o corrigiu: usei
`route_exciter 0` a pensar que silenciava so o transdutor. Um nome errado
nao espera pelo proximo colaborador para cobrar — cobra ao proprio autor, no
mesmo dia.

O conserto tem duas metades, e elas NAO sao a mesma coisa: `out_l`/`out_r`
(mute de ramo, depois do limiter) e a matriz `route_l_*`/`route_r_*` (qual
cadeia entra em qual saida, antes dos efeitos). Colapsar as duas teria feito
o mute deixar de ser mute — zerar uma coluna da matriz cala o seco, o freeze
e o espaco, mas o retorno do delay continua a entrar.

> **Quando N fontes discordam sobre o que uma coisa E, pergunte se a coisa e
> uma DECISAO de quem esta a discutir.** Se for de outra pessoa — ou de uma
> chave de metal — todas as N estao erradas, e escolher entre elas e escolher
> entre erros.

---

## 15f. A regra do sufixo teria engolido a matriz (28/08)

A pertinencia ao preset e DERIVADA do nome, e isso e um acerto do desenho:
sufixo `_a`/`_b` diz "e por cadeia", e nao ha segunda lista para divergir.

Os nomes naturais da matriz sao `route_l_a`, `route_l_b`, `route_r_a`,
`route_r_b`. **Acabam todos em `_a`/`_b`.** Sem excepcao explicita, teriam
entrado sozinhos nos presets de CADEIA — e nao ha momento em que alguem
escreva a decisao de os incluir, porque a regra os inclui em silencio.

O destino errado e o pior dos dois possiveis. A matriz compensa como os
transdutores estao LIGADOS: facto de fiacao. Dentro de cada preset de cadeia,
cada preset carregaria a fiacao, e desandar um fio obrigaria a regravar a
biblioteca inteira — a mesma familia do *"corda existiria duas vezes"* que
fez a biblioteca ser UMA.

Apanhei-o por ler o `presets.h` ANTES de escolher os nomes, nao depois de os
escrever.

> **Regra derivada classifica o que ainda nao existe, e classifica em
> silencio.** Ao criar um simbolo novo num sistema com regra automatica de
> pertinencia, leia a regra primeiro e pergunte onde ELA o poe — nao onde
> voce queria que ficasse.

---

## 15g. A regra generalizou alem do caso que a gerou (28/08)

*"Audio nao passa por contato mecanico em lugar nenhum deste instrumento"*
esta escrita no `HARDWARE.md` e neste documento, com autoridade. E ela e
falsa: o DPDT de sinal do dB conduz audio DE PROPOSITO, e e o que esta
montado.

O caso que a gerou era mais estreito. Uma chave de POTENCIA de tres posicoes
(marcada 12 V) tem contato seco e nao conduz nivel de linha — custou uma
tarde. O que se aprendeu foi *chave de potencia nao serve para comutar
sinal*. O que se escreveu foi uma proibicao universal.

> **Ao escrever a regra, pergunte qual e a menor afirmacao que cobre o caso.**
> A regra larga parece mais valiosa e e mais barata de escrever, mas o preco
> aparece depois: ela contradiz o hardware real e quem a le acredita nela,
> porque documento tem autoridade.

O `check_docs` nao apanha isto e nao pode: a frase e gramaticalmente
impecavel e nao cita simbolo nenhum. So o hardware a desmente.

---

## 15h. A folha impressa e anterior ao mapa (28/08)

O `ccmap.html` — e o `ccmap.pdf` que sai dele, o cartao de referencia da
bancada — tem **27 numeros de CC que ja nao existem** (106, 116-118, 80-88,
22, 24, 26, 32, 33, 42, 43, 52, 53, 62, 63, 68-70). E anterior ao esquema de
seis canais. Ninguem notou porque um PDF nao compila, nao roda e nao falha.

Ia emendar as duas linhas do `route_*` e seguir. Teria sido pior que nao
mexer: **a folha ficaria com dois numeros certos e vinte e sete errados, e a
correcao recente e o que faz um documento parecer cuidado.**

> **Ao encontrar um erro num documento, meca o resto antes de corrigir esse.**
> A pergunta nao e "esta linha esta certa?" e sim "que fraccao deste
> documento ainda descreve o sistema?". Abaixo de um limiar, corrigir uma
> linha e desinformar.

Marcado com faixa vermelha e ponteiro para o `CCMAP.md`, que e gerado.
Regerar ou aposentar e decisao do dB — mas nao fica a fingir que esta certo.

---

## 15i. Placa muda: o barramento USB continua a testemunhar (28/08)

Depois de um `dfu`, a placa parou de responder: zero bytes em 8 s de escuta
passiva e em tres comandos com 3 s cada. Tinha respondido `v0.177.0` minutos
antes.

A tentacao era concluir "travou". O que a desmentiu nao foi nada que a placa
dissesse — foi a **marca de tempo do node `/dev`**, que mudou de 11:08 para
11:12 a cada envio de `dfu go`. Ela estava a reiniciar. Portanto o RX
funcionava, o comando executava, e so o TX estava morto.

E o `ioreg` disse o resto: `idProduct` 22336 = `0x5740`, o CDC do app — nao
`0xdf11` do DFU. Ela reiniciava e voltava ao app.

> **Quando o canal principal cala, o sistema operativo continua a ver o
> dispositivo.** Enumeracao, `idProduct` e a marca de tempo do node sao
> testemunhas independentes do firmware — e uma testemunha que nao depende do
> acusado vale mais que dez tentativas de o fazer falar.

Corolario que ja tinha sido escrito e agora tem um segundo caso: **canal mudo
prova que aquele canal esta mudo, e nada alem disso.**

A gravacao resolveu a mudez — o disjuntor do log vive no app, e o app novo
nao o tem armado. Nao atribuo a mudez ao meu comando: as duas coisas
coincidiram no tempo, e coincidencia perfeita ja produziu aqui um "era o
cabo" que era o dongle.

---

## 15j. A primeira gravacao depois de abrir o dispositivo e lixo (28/08)

A primeira corrida do mapeamento deu numeros que nao fechavam com caminho
nenhum do codigo: `dest 1` sem sinal, `dest 2` a acender dois canais, e
`ch3/ch4` a cair 44 dB numa transicao que nao os toca.

Nao acusei o firmware. Corri a verificacao do instrumento — mesmo estado,
tres medidas — e ele repete a **0,1 dB**. Os -28,8 dBFS em `ch3/ch4` eram
artefacto de arranque do `sounddevice`: a primeira `rec` depois de abrir o
dispositivo apanha o transiente da stream.

> **Sonda nova nasce sem procedencia, e "nova" inclui a primeira leitura de
> uma sonda velha.** Repetir a mesma medida tres vezes custa segundos e
> separa dado de artefacto — e um numero que nao fecha com nenhum caminho do
> codigo e suspeita contra o INSTRUMENTO antes de ser contra o sistema.

O medidor ficou com `rms()` a descartar os primeiros 150 ms, e as corridas
comecam com uma leitura de aquecimento deitada fora.

---

## 15k. Seno continuo e mau instrumento para medir delay (28/08)

No caso que separa os dois andares do roteamento, os numeros absolutos
mentiram: com tudo ligado e o delay a tocar, a saida media **-44,4 dBFS**,
mais BAIXA que a base de -35,6. Ligar um efeito baixou o nivel.

A causa e trivial e nao tem nada a ver com o firmware: o seco e a sua propria
copia atrasada sao o MESMO seno, e somam-se com fase fixa. Interferem.

A conclusao valida saiu de outro par, com uma variavel so e o chao como
referencia:

```
route_r_a=0, delay desligado   -69,2 dBFS   (chao)
route_r_a=0, delay LIGADO      -39,5 dBFS   (+29,7 dB acima do chao)
```

> **Sinal periodico continuo mede mal tudo o que produz copias de si proprio**
> — delay, reverb, chorus, freeze. Para "esta ca ou nao esta", compare contra
> o CHAO com uma variavel so, em vez de comparar dois estados que ambos
> contem interferencia.

---

## 15l. O contador de quedas mora na RAM que o evento apaga (28/08)

O criterio dos 20 minutos do link BLE tinha falhado com uma queda aos ~6 min.
Fui ler o contador da ponte para distinguir sono do iPad de instabilidade
real, e encontrei `quedas: 0 · nenhuma desde o boot` com `msgs 0 · 0 · 0`.

Zero mensagens desde o arranque significa que a ponte reiniciou. Os
contadores vivem em RAM, e o dado da corrida inteira nao existe mais.

> **Instrumento cuja memoria e apagada pelo evento que ele mede so serve
> enquanto ninguem precisa dele.** A pergunta de galeria e *"caiu de
> madrugada?"* — e um piscar de energia, que e uma das causas de queda, zera
> a testemunha da queda.

Ha onde o por: a Daisy tem BACKUP_SRAM (12 B usados de 4 KB) e o ESP32 tem
NVS. Nao esta feito — fica registado como defeito aberto, nao como ideia.

Nota de metodo que se repetiu: **abri a porta da ponte com `dtr=False` e
`rts=False` de proposito**, porque o USB nativo do C3 trata combinacoes de
DTR/RTS como pedido de reset. Ler um contador com uma sonda que o zera e a
familia inteira em miniatura.

---

## 15m. Criterio que nao pode falhar nao e criterio (28/08)

O T7c da matriz tinha cinco criterios e passou nos cinco. Dois deles eram
**vazios**, e so se soube depois de a fiacao mudar.

`OUT_L` nao chegava a interface. Portanto *"mexer no L nao afecta o R"* dava
`+0,0 dB` — **fosse qual fosse o firmware**. Mesmo com a matriz completamente
partida, aquele numero seria +0,0. E ele passou com aspecto de prova: o
`+0,0 dB` e ate mais bonito que um resultado verdadeiro, porque e exacto.

Os outros tres criterios eram reais — todos sobre o canal medido, e o canal
medido tinha sinal. A assimetria e o diagnostico: **num teste de isolamento,
o lado que se espera que NAO mude e o que precisa de prova de vida.**

> **Antes de aceitar um criterio, escreva o que teria de acontecer para ele
> FALHAR.** Se a resposta for "nada que o codigo possa fazer", ele nao esta a
> medir o codigo. E um criterio de ausencia so vale depois de provada a
> presenca no mesmo canal.

Refeito com uma saida por canal (59,6 dB de separacao), os quatro criterios de
isolamento passaram a valer, com o lado nao tocado a mover-se **`-0,0 dB`** em
todos eles. Agora o numero significa alguma coisa.

**E o mapa de canais saiu CRUZADO na primeira montagem** — `OUT_L` no `ch2`.
Assumi-lo teria trocado todos os vereditos de lado. O script passou a MEDI-LO
no arranque, e a abortar se as duas saidas cairem no mesmo canal: a
pre-condicao de fiacao virou parte do instrumento em vez de nota no roteiro.

---

## 15n. Escrevi a regra e falhei-a vinte minutos depois (28/08)

Na mesma sessao pus na skill `medicao-limpa`: *provar que o canal fala antes
de ler qualquer medicao*. Vinte minutos depois corri o mapeamento, li "nao
chega nada em lado nenhum", e **reportei que o conserto de fiacao do dB tinha
piorado as coisas**.

A placa tinha caido do barramento USB. O node `/dev` da Daisy nao existia, o
`ioreg` nao a via, e o ESP32 tinha re-enumerado com nome novo. Os comandos
nunca chegaram, o tom nunca ligou, e os +2,8 dB eram ruido. Estive a um passo
de o fazer desmontar um conserto que estava certo.

O script media audio e **nunca conferia se a placa respondia**. A regra estava
escrita em prosa, na skill, onde depende de eu me lembrar dela.

> **Regra que vive so em prosa custa a mesma disciplina toda vez.** O conserto
> nao e lembrar melhor: e por a regra DENTRO do instrumento. Cada `set` do
> `teste_roteamento.py` confere o eco da placa e ABORTA se nao vier — agora a
> corrida nao consegue produzir um numero sem prova de canal.

Corolario que ja tinha nome aqui e ganhou o pior caso: **antes de reportar que
uma accao humana piorou alguma coisa, prove que o sistema estava a responder
depois dela.** O custo de errar nao recai sobre mim — recai sobre quem esta
com o alicate na mao.

---

## 15o. Painel nao inicializado mostra uma FOTOGRAFIA VELHA (28/08)

O OLED acendeu com tres linhas de texto formatado no arranque, e o dB
concluiu — com toda a razao aparente — que o hardware estava bom e o defeito
era meu. Eu concordei, e fui procurar o defeito no software.

Nao havia nada a desenhar aquilo. **Nada inicializa o OLED no boot**: o
`EncoderInit` corre, mas nao toca no painel, e o espelho dos encoders esta
atras de `if(enc_na_tela && OledConfigurado())` — falso ate alguem digitar
`oled` ou `enc`. O proprio `board` dizia-o o tempo todo: *"oled: nao
configurado ainda"*.

O painel guarda os pixels enquanto tiver energia, e a Daisy reinicia sem lhe
cortar a alimentacao. **A imagem sobreviveu a uma gravacao, a um arranque e a
uma hora de diagnostico** — ate o primeiro `oled` pulsar o RST e apaga-la.

> **Ecra e memoria, nao janela.** O que esta la nao prova que alguem escreveu
> agora — so que alguem escreveu ALGUMA VEZ e a energia nao caiu desde entao.
> Antes de tratar conteudo de ecra como prova de vida, pergunte quem o
> desenhou NESTA sessao.

E a ironia esta a duas linhas de distancia, no comentario do proprio
`EncoderInit`: *"instrumento nao inicializado le zero, e zero pareceu dado —
a familia de sempre, agora no codigo que eu mesma escrevi para caca-la."*
Aqui e a versao invertida: **instrumento nao inicializado mostrou imagem
velha, e imagem velha pareceu prova de vida.** A mesma familia, com o sinal
trocado, e nem assim a reconheci.

Corolario que custou meia tarde: a imagem desaparecer TAMBEM nao provou o que
eu disse que provava. Anunciei *"a imagem sumiu, logo os comandos chegam"* —
e nao segue. O `OledInit` pulsa o **RST**, e um reset de hardware limpa a RAM
e desliga o painel **sem SPI nenhum**. So provou que um fio dos cinco chegava.

---

## 15p. A simetria era PROVA, e eu fui procurar explicacao noutro lado (28/08)

O dB pediu para inverter o sentido do encoder G: "e so trocar as duas
variaveis". Troquei `a` com `b` no mapa de pinos. **A contagem parou de vez.**

Reagi a isso indo procurar um mapa de pinos alternativo — e fiz o dB rodar o
encoder quatro vezes, ler tres relatorios e conferir a fiacao com o
multimetro.

A resposta estava no fonte da libDaisy, a duas linhas de distancia:

```cpp
hw_a_.Init(a, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
hw_b_.Init(b, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
...
if((a_ & 0x03) == 0x02 && (b_ & 0x03) == 0x00)      inc_ = +1;
else if((b_ & 0x03) == 0x02 && (a_ & 0x03) == 0x00) inc_ = -1;
```

**Perfeitamente simetrico.** Trocar `a` com `b` so pode negar o sentido —
nunca zerar a contagem. A troca era, por construcao, incapaz de produzir o
sintoma. Isso nao era uma hipotese a testar: era uma PROVA a ler.

> **Quando uma mudanca nao pode causar o sintoma, pare de procurar como ela o
> causou.** Ler a implementacao e barato e conclui; testar contra a
> suposicao e caro e so encontra o que ja se esperava. E o custo de nao ler
> nao recai sobre quem edita — recai sobre quem esta a rodar o knob.

A causa real: **as duas fases do G tinham aberto.** O `varre` mostrou-o sem
ambiguidade — quinze pinos livres lidos ao mesmo tempo, e SO o push comutava,
com o contador em `push 14 · crus +0`. E voltaram **durante o manuseio do
chicote**, que e o diagnostico e nao o acaso: *intermitencia correlacionada
com manuseio E o diagnostico*.

---

## 15q. A edicao ancorou em texto NAO-UNICO e a funcionalidade foi parar a outro comando (28/08)

Escrevi um subcomando `enc inverte <a|b|g>` para corrigir o sentido sem
gravar. Ancorei a insercao em:

```python
v = 'if(strncmp(a, "reset", 5) == 0)\n        {'
assert v in s          # <-- so isto
s = s.replace(v, n, 1) # <-- e o PRIMEIRO
```

Esse texto existe **tres vezes** — nos comandos `varre`, `encpino` e `enc`. O
bloco foi parar ao handler do **`vistos`**, quinhentas linhas acima.

E entao: **compilou, gravou, e `enc inverte g` respondeu com o relatorio
normal.** Sem erro em lado nenhum. Duas gravacoes com uma funcionalidade que
simplesmente nao existia, e um `vistos inverte g` que ninguem ia digitar.

> **`assert texto in fonte` prova existencia, nao UNICIDADE.** Toda ancora de
> edicao automatica confere `count(...) == 1` antes de substituir — e depois
> verifica PROGRAMATICAMENTE onde o bloco caiu, em vez de o reler e achar bem.

A verificacao que devia ter corrido desde o inicio cabe em cinco linhas:

```python
alvo = [i for i,l in enumerate(linhas) if 'strncmp(a, "inverte"' in l][0]
h    = [i for i,l in enumerate(linhas) if 'strncmp(line,' in l and i < alvo][-1]
print("bloco dentro de:", linhas[h].strip())
```

Eu ja tinha esse guarda escrito noutro script desta mesma sessao — um
`assert s.count(velho) == 1` com a mensagem `"AMBIGUO"`. Nao o usei aqui.
**Defesa que existe e nao se aplica por habito nao e defesa: e sorte.**

---

## 15r. Quatro mapas do mesmo encoder, nenhum medido (28/08)

Ao caçar o G, encontrei quatro afirmacoes em desacordo sobre onde ele estava:

| fonte | G |
|---|---|
| comentario do `hal` (intencao de projecto) | `D19` `D20` `D21` |
| `EncoderInit` (o que a placa corria) | `D18` `D19` · push `D17` |
| `PINOS.md` | push `D17` · faseA `D18` · faseB `D19` |
| o dB, de memoria | switch `D18` · fases `D19` `D20` |

Nenhuma delas media nada. E vinte linhas acima da tabela estava escrito o que
fazer, por alguem que ja tinha pago:

> *"Perdi rodadas diagnosticando fiacao com base num mapa suposto. Nenhum
> instrumento apontado para D20/D21 diria 'o fio esta no D18'. Dai o `varre`,
> que le TODOS os livres: supor o mapa e depois medir contra a suposicao so
> encontra o que ja se esperava."*

O `varre` resolveu em dez segundos o que quatro fontes nao conseguiam, e o
veredito foi contra tres delas. O dB tinha razao (`push 17, fases 18 19`), e
o codigo ja estava certo.

> **Quando N fontes discordam e nenhuma mede, nao escolha entre elas — meca.**
> E se a ferramenta de medir ja existe no repo, alguem ja pagou este preco:
> a pergunta certa e por que ela nao foi a PRIMEIRA coisa a correr.

Ponto cego declarado, para nao virar a proxima cicatriz: **o `varre` le 15 dos
31 pinos.** Ficam de fora `D5`–`D8` e `D10` (OLED), `D13`/`D14` (MIDI) e
`D22`–`D30`. Fio que va parar a um desses nao aparece — e nenhum instrumento
que temos hoje diria isso.

---

## 15s. Documento meio-gerado mente na metade escrita a mao (28/08)

O cartao de CC (`ccmap.html` -> `ccmap.pdf`) era a folha que se leva para a
bancada. Estava com **27 numeros de CC que ja nao existiam** — anterior ao
esquema de seis canais — e ninguem notou, porque **um PDF nao compila, nao
roda e nao falha**.

Ia emendar as duas linhas do `route_*` e seguir. Teria sido pior que nao
mexer: a folha ficaria com dois numeros certos e vinte e sete errados, e
**correcao recente e o que faz um documento parecer cuidado**.

A saida nao era escolher entre corrigir e aposentar. Era separar:

> **O que MUDA gera-se; o que DURA e dado.** Numero de CC, canal e quais
> params existem saem do `manifest.h`. A nota humana — *"64 = as duas cadeias
> cheias"*, *"em 0 o gate e PULADO"* — e uma tabela `NOTAS` no gerador.

E entao veio o que interessa. Com os numeros gerados, **as notas continuaram
a mentir**, porque a parte escrita a mao envelheceu sozinha. Dez delas:

- `frz_size` dizia **20…400 ms** com o manifest em **20–165**. E o MESMO
  caso que gerou a regra das faixas no `check_docs` — voltou, noutro
  ficheiro, porque a regra so olhava os `.md`.
- `drv_drive` dizia **1,5…12×** com o manifest em **1,5–48**.
- `tone_dest` dizia *0=A · 1=B · 2=AMBAS*. O manifest diz **0=A+B · 1=A ·
  2=B** — **invertido**, e um cartao de bancada invertido e pior que nenhum.
- Sete notas diziam **"compart."** em params que o pivo de duas cadeias
  tornou PAREADOS. A frase descreve uma topologia revogada.

Nenhuma delas seria apanhada por um verificador de nomes: **o nome estava
certo**. A que apanhou seis foi escrita no proprio gerador, e e de tres
linhas — nota que diz "compart." num param que entra em ch2/ch3 e denunciada.

> **Ao gerar metade de um documento, escreva o verificador da outra metade.**
> Gerar os numeros e o passo obvio, e da a sensacao de ter resolvido —
> enquanto a parte que continua manual passa a estar rodeada de dados frescos,
> o que a torna MAIS convincente e nao menos.

Corolario que ja tinha nome aqui e ganhou o segundo caso: **o que se pode
gerar, gere** — e a ultima milha conta. Este script IMPRIMIA a tabela e alguem
colava-a no `CCMAP.md` a mao; agora ESCREVE os dois destinos. **Gerador que
nao escreve deixa a ultima milha para a disciplina humana, e e exactamente ai
que o documento se descola do codigo.**

---

## 15t. O laco que perseguia um alvo que se move (28/08)

A placa travou depois de vinte minutos de trafego BLE continuo. Audio a
tocar, tela congelada, serial calada, e nenhum vigia disparou.

```cpp
while(m_tail != m_head)      // m_head e escrito pela ISR
{
    ...
    if(midi_cb) midi_cb(b);  // ...e isto pode ser lento
}
```

**A condicao de paragem persegue um alvo que se move.** Se os bytes chegam
mais depressa do que o callback os consome, o laco NUNCA termina. Nao e
bloqueio — e um laco que corre para sempre, e por isso nenhum vigia de
"chamada que nao retorna" o apanhava: a chamada retorna sempre.

A 31250 baud entra um byte a cada **320 us**. Basta o callback gastar isso
por byte, e o `Log()` com o host desconectado gasta muito mais. O sistema
fecha-se sobre si proprio, e quanto mais atrasado fica, mais atrasado fica.

> **Todo laco de drenagem tira um INSTANTANEO do ponteiro de escrita.** O que
> chegar durante a drenagem espera a volta seguinte — esse e o contrato de um
> laco principal. Perseguir o ponteiro vivo e correcto so enquanto o
> produtor for mais lento que o consumidor, e ninguem escreve isso na
> condicao.

**O QUE O ENCONTROU foi o LED de fase**, e so ele podia: o instrumento corre
no **callback de audio**, do outro lado da fronteira do laco morto. Uma sonda
que vive no mesmo contexto que o suspeito morre com ele. O dB contou seis
piscadas e isso enderecou a fase exacta — sem esse numero eu teria passado a
tarde a instrumentar o SPI do OLED, que era o meu palpite e estava bom.

**E o IRMAO levou o mesmo conserto.** O `PollSerial` tem forma identica; so a
exposicao difere — a fila dele e alimentada por alguem a escrever, que nao
ganha a drenagem. Consertar so a que mordeu deixaria a outra a espera do dia
em que alguem cole um ficheiro inteiro na serial.

**O sensor que faltava** era o proprio atraso: sobra depois da drenagem
significa que ela nao acompanha. Agora sai no `board`. Duas notas sobre
construi-lo, e as duas sao erros meus da mesma hora:

- **O primeiro limiar gritava no normal.** Com o instantaneo, sobrar e
  esperado; eu marquei `*** a drenagem esta a perder ***` com 9 bytes numa
  volta. **Alarme que dispara em operacao sa ensina a ignora-lo** — o carimbo
  a aparecer do outro lado. O limiar passou a ser metade do anel.
- **E eu escrevi o tamanho do anel a mao noutro ficheiro**, 256 quando eram
  128. Uma segunda fonte que nasceu errada, no mesmo dia em que removi tres
  copias do mapa de pinos pelo mesmo motivo. Virou acessor.

---

## 15u. Banco nativo prova a ALGEBRA, nao o TIMBRE (28/08)

Escrevi um oitavador por divisao de frequencia e provei-o em banco nativo
antes de o gravar — disciplina da casa, e passou:

```
membrana 768 Hz        -> 384,0 Hz   erro 0,0%
768 Hz + 2o harmonico  -> 384,0 Hz   erro 0,0%
portao: silencio e ruido -> CALADO
```

Cinco casos, todos verdes. Na placa, o dB: *"nao parece um oitavador, parece
ruido tipo caixa"*.

**O teste estava correcto e era irrelevante.** Eu provei-o com SENOIDES. Uma
pancada de membrana nao tem fundamental — a minha propria medicao, **uma hora
antes**, tinha-lhe achado oito modos inarmonicos (278, 370, 406, 672, 732,
768, 809, 1061 Hz). Um detector de cruzamento por zero nao acha fundamental
nenhuma nisso: treme, e o que sai e rajada de ruido.

Eu tinha o espectro da fonte medido, no mesmo terminal, e testei com material
que ela nunca produz.

> **Banco nativo prova que a conta esta certa. Nao prova que o efeito faz o
> que o nome diz.** Se existe uma medicao da fonte real, o teste alimenta-se
> DELA — e se nao existe, o resultado do banco e uma hipotese, nao um aceite.

O efeito ficou, porque o dB gostou do que ele faz de facto. E entao passou a
chamar-se o que e: **tarolator**. Nome que descreve a intencao em vez do
resultado engana o proximo — e o proximo e ele daqui a um mes. Mesma
correcao do `route_jack`, no mesmo dia; a diferenca e que desta vez o nome
certo chegou antes de o errado ficar no repo.

---

## 15v. SE NAO SOA, NAO FICA — e o controlo que o AGC anulava (28/08)

Regra do dB, nascida de um caso: *"o in_gain a 0 e a 100% nao muda nada no
som, entao e melhor remover este parametro"*.

E o mecanismo estava a duas linhas de distancia. O `Governor` e um **AGC**:

```cpp
ceiling_ = 0.5f;   // configurado em 0.85
gain_    = 1.f;    // ...e ele MEXE neste
```

Sobe-se 18 dB no `in_gain`, ele baixa 18 dB, a saida fica igual. **O
controlo funcionava perfeitamente e era inaudivel por construcao.** O unico
efeito que restava era o DEGRAU — ganho lido por bloco e aplicado por
amostra —, e um clique nao e uma funcao.

> **Controlo que nao muda o som gasta fader, CC e slot de preset — e ensina
> que controlos podem ser inertes, que e pior que a soma disso.** Ao juntar um
> controlo manual a um automatismo que age no mesmo eixo, pergunte qual dos
> dois o ouvido vai ouvir.

E a regra vale tambem para PEDACOS de curso: o `tarol_mix` nasceu 0–1 linear e
o dB so o ouvia no topo — dois tercos de fader mortos. Passou a 0–4x com
curva 0.5, que a 10% do curso ja da 1,26x.

**O QUE SE PERDEU, e fica escrito para nao ser redescoberto:** o `in_gain`
era o casamento com o transdutor. O governor so **REDUZ**; um piezo 20 dB
mais fraco nao tem como entrar na faixa util por software, e esta peca vai ser
replicada na ordem de centenas. Se isso aparecer, o conserto e o divisor da
carrier board — ou um ganho de BANCADA, fora da performance. A lapide ficou no
`FEATURES.md` e no `TESTES.md`, declarada ao verificador, porque e ela que
impede a reintroducao sem leitura.

**Deep clean, e o verificador fez o inventario por mim:** codigo (audio,
`hist`, `pk`, diagrama, manifest, comentario do `drive.h`), geradores, e sete
documentos. O detector de notas orfas que eu escrevera de manha apanhou o
`in_gain_a` sozinho, antes de eu chegar la.

---

## 15w. O pot absoluto e o param partilhado (28/08)

Os dois pots chegaram soldados e o codigo do laco ja os esperava havia
meses, com esta guarda e este aviso:

```cpp
if(v == knob_last[k])
    continue; // nao mexeu: nao reescreve, nao atropela
// "...a guarda de mudanca existe pro dia em que os pots chegarem, porque
//  sem ela o MIDI para de funcionar em silencio no mesmo dia."
```

**A guarda estava errada e o aviso estava certo.** Igualdade de float sobre
um ADC com ruido NUNCA dispara: o pot escreveria o param a cada volta do
laco — milhares de vezes por segundo — e ganharia sempre. O MIDI morreria
calado, exatamente como o comentario previa, e a guarda que devia evitar
isso era o proprio mecanismo.

> **Guarda escrita antes do hardware existir nao foi exercitada.** Ela
> descreve a intencao de quem a escreveu, e no dia em que o hardware chega
> ninguem a re-le — porque "ja esta tratado". Ao ligar um periferico que uma
> guarda antiga esperava, o primeiro passo e EXERCITA-LA, nao confiar nela.

**E o problema maior nao era o ruido: era o pot ser ABSOLUTO.** O param e
partilhado com o MIDI e a serial. Pot em 0,2 e o iPad poe o param em 0,9:
tocar no pot SALTA. O painel v2 ja tinha resolvido isto do outro lado —
*"encoder EC11 relativo, nao absoluto, mata o problema do salto do knob
compartilhado"* — e o pot nao tem essa saida.

Captura suave: assume quando PASSA pelo valor actual, larga quando alguem
escreve por outro caminho, com a janela de largar maior que a de capturar
(iguais, ficariam a trocar de dono na fronteira).

**Com uma excecao, e ela e a que faz sentido para o musico:** no arranque
ninguem escreveu o param ainda, entao a primeira leitura ADOTA a posicao
fisica. Volume que nasce longe de onde o botao esta e a primeira coisa que
assusta quem liga o instrumento.

> **Controlo absoluto que partilha um param precisa de dizer QUANDO manda.**
> As tres respostas — captura, adocao no arranque, e largar quando outro
> escreve — sao a mesma pergunta em tres momentos, e responder so a primeira
> deixa o instrumento estranho nos outros dois.

**O HARDWARE decidiu quantos.** `A0–A4` sao aliases de `D15–D19` e o encoder
global ocupa `D17/D18/D19`: sobram dois. O `num_knobs` dizia **cinco**, e o
laco iterava sobre tres que nao existem — inofensivo enquanto o `ReadKnob`
era stub, mentira no instante em que deixasse de ser. Dois deles estavam
mapeados para `dly_time` e `dly_mix_a`: um terceiro pot, um dia, mexeria no
delay sem ninguem ter decidido isso.

> **Capacidade declarada e uma AFIRMACAO sobre o hardware, e envelhece como
> qualquer outra.** Enquanto o driver e stub ninguem a cobra — e e por isso
> que ela sobrevive errada ate ao dia em que passa a ter consequencia.

E o `ReadKnob` devolve **-1**, nao 0, quando nao ha pot: ausencia representada
FORA do dominio, porque 0 e indistinguivel de um pot no fundo. Regra que este
projeto ja pagou cinco vezes num dia so.

---

## 15x. O varredor de pinos livres roubava o pino ao ADC (31/08)

Os dois pots liam **o mesmo valor**, ~0,12, e nao mexiam. O dB trocou fios,
removeu um condensador, mediu 3,3 V nos extremos, trocou os pinos entre si —
e a leitura nao mudou uma unica vez. Porque nao podia:

```cpp
static const int kLivres[] = {0,1,2,3,4,9,11,12,15,16,17,18,19,20,21};
                                              ^^  ^^
uint32_t VarreLivres()          // chamado a CADA volta do laco
{
    if(!feito)
        for(...) g[i].Init(kLivresPin[i], GPIO::Mode::INPUT, Pull::PULLUP);
```

**O `D15` e o `D16` estavam na lista de "pinos livres".** Na primeira volta do
laco, o varredor reconfigurava-os como digitais — arrancando-os do modo
ANALOGICO — e o ADC passava a amostrar uma entrada desligada. O firmware
chegava sempre primeiro; nada feito ao cobre podia mudar nada.

> **Lista de "livres" e uma AFIRMACAO SOBRE POSSE, e envelhece.** Estava certa
> quando foi escrita e passou a mentir no instante em que os pots ganharam
> dono. Ao dar dono a um pino, procure TODAS as estruturas que o davam por
> livre — e a pergunta que as encontra nao e "onde e que este pino aparece?",
> e "quem e que assume que ele nao tem dono?".

Mesma familia, no mesmo mes, tres vezes: o `num_knobs = 5` quando ha dois, a
isencao do `kEncPin` no verificador depois de ele virar fonte, e agora esta.
**Declaracao sobre hardware nao tem quem a cobre ate doer.**

A trava mora agora onde o roubo acontecia — dentro do `VarreLivres`, na
primeira volta —, e grita em vez de roubar calado.

---

## 15y. Construi duas sondas que nao podiam passar (31/08)

Cacei este defeito com duas ferramentas novas, e **as duas mentiram**, cada
uma a seu modo. A regra que ambas violaram esta escrita nesta casa ha
semanas: *sonda que devolve zero tem de provar que consegue devolver
diferente de zero.*

**A primeira nao esperou o transiente.** O `knob pino` segurava um pull
interno 5 ms para ver se o pino o seguia. Com um condensador de filtro de
0,1 uF no pino — a montagem NORMAL — e o pull de ~40 kohm, a constante de
tempo e ~4 ms: em 5 ms o condensador mal comecava a carregar, e a sonda lia o
estado anterior. Ela deu **"LIGADO"** exactamente ao caso que existia para
apanhar. Passou a esperar 60 ms, quinze constantes de tempo.

**A segunda desligava o que vinha medir.** O `knob prova` dirigia o pino como
saida digital para ver se o ADC acompanhava. Nos STM32, por-lo em push-pull
**desliga o interruptor analogico**: o ADC deixa de ver o pino. O teste estava
garantido a dizer *"nao segue"* fosse qual fosse a verdade — e eu usei esse
resultado para anunciar ao dB que **o defeito era meu**, o que por acaso era
verdade, mas nao por esta razao.

> **Antes de acreditar numa sonda, pergunte o que ela faria se a resposta
> fosse a OUTRA.** Uma sonda que nao consegue produzir os dois resultados nao
> mede — decora. E a forma mais cara dela e a que confirma a hipotese de quem
> a escreveu, porque essa nao levanta suspeita nenhuma.

Custo: uma solda do dB desfeita por um diagnostico meu errado, e uma sessao
inteira. O que finalmente resolveu nao foi sonda nenhuma — foi ler o codigo a
procura de quem mais tocava naqueles pinos.

---

## 15z. Acusei tres inocentes antes de o dB ver o curto (31/08)

Os pots foram ligados, e a placa comecou a cair do barramento — reiniciava,
re-enumerava, calava. Em duas horas eu acusei, com confianca, **tres coisas
que nao eram**:

| acusei | com que argumento | porque estava errado |
|---|---|---|
| **o hub USB** | tres falhas de USB num padrao | a Daisy nao estava em hub nenhum |
| **os condensadores** | ruido 150x pior "depois de os soldar" | medi com os pots DESLIGADOS |
| **o circuito** | estavel fora dele, instavel dentro | eu cortava a energia entre os testes |

A causa era um **curto intermitente entre PWR e GND da ponte**, que o dB
encostou ao mexer nos cabos dos pots. Brownout: a placa perde alimentacao e
desaparece do barramento — nao e travamento, e falta de corrente.

**Cada uma das minhas tres acusacoes tinha o mesmo defeito de forma.**

> **Quando o suspeito e a cura se movem juntos, a coincidencia e
> indistinguivel da confirmacao.** Isto ja estava escrito aqui, de 27/08, com
> estas palavras — e eu repeti-o tres vezes no mesmo dia, cada vez com um
> suspeito novo.

O caso do circuito e o mais claro: para o testar "fora", desliguei e liguei o
USB; para o testar "dentro", desliguei e liguei outra vez. **A recuperacao
vinha do corte de energia, e eu creditava-a a variavel que estava a mexer.** A
variavel que eu julgava isolar nunca esteve isolada.

E o caso dos condensadores custou trabalho ao dB: mandei-o dessolda-los com
base numa medicao feita com a placa FORA do circuito — os dois canais liam
meia escala com quantizacao grosseira, que e a assinatura de **entrada a
flutuar**, e eu li como "o condensador estragou o sinal". Medidos depois, com
tudo ligado: **sigma 29x MELHOR** no canal ruidoso.

> **Antes de acusar uma peca, diga em voz alta o que teria de ser verdade para
> ela ser inocente — e verifique ISSO.** As tres acusacoes tinham a mesma
> falha: eu tinha uma explicacao coerente e nao tinha um teste que a pudesse
> derrubar.

O que finalmente resolveu nao foi medicao minha nenhuma. Foi o dB a mexer nos
cabos e a ver dois fios a encostar.

---

## 15aa. O ruido contra o qual apertei as defesas era artefacto (31/08)

Medi o ruido do ADC com cuidado — 25 leituras, desvio padrao, taxa de escrita
— e escolhi os quatro numeros da captura do pot contra ele: filtro a 0,02,
histerese a 0,85 passo de MIDI. Numeros derivados de medicao, nao de palpite.

**A medicao era de uma entrada desligada.** O `VarreLivres` roubava o pino ao
ADC (cicatriz 15x), e o que eu media era um pino a flutuar. Com o defeito
consertado e o condensador no sitio, o ruido real e **catorze vezes menor**:

```
sigma 0,0056   <- o que eu medi, com o pino roubado
sigma 0,0004   <- o real, 19x abaixo de um passo de MIDI
```

As defesas ficaram apertadas contra um fantasma: filtro doze vezes mais lento
do que precisava e histerese quase tres vezes mais grossa. **Apertar defesa
contra ruido que nao existe custa RESPOSTA** — o pot ficava mole e grosso, e
ninguem saberia porque.

> **Numero derivado de medicao herda a validade da medicao.** Quando a causa
> de um defeito muda, TODA constante que foi calibrada contra o sintoma dele
> tem de ser recalibrada — e ninguem avisa, porque a constante continua a
> parecer bem fundamentada.

Os dois valores foram remedidos um a seguir ao outro, na mesma placa, e
ficaram os afrouxados: mesmo sigma, mesmas zero escritas em repouso, resposta
muito melhor. E foram ajustaveis por serial desde o inicio — foi so por isso
que a recalibracao custou um comando em vez de uma gravacao.

---

## 12. Checklist de retomada a frio (colar no início de qualquer sessão futura)

Ler o [STATUS.md](STATUS.md) e este documento antes de tocar em código. `/gitsync` se a sessão for de arquitetura. Um voo, uma variável — e listar os empregos da variável antes de mexer. Caixa-preta armada por comando no início do voo; sessão de leitura não escreve. Toda medição declara unidade (RMS? pico? norm? real?) e regime (estreia? permanente? que bloco? que clock? -O2 ou -O3?). Módulo novo: description.json → embarque pela receita → medir estreia+regime nos dois blocos → teste audível com o material certo pro efeito. Bug novo: nomear a família antes de consertar. Hipótese nova: registrar a aposta com percentual. Feature nova: o aceite é do músico. **Sintoma repetido pela terceira vez: pare de gerar hipóteses e ache o teste que corta o sistema ao meio** (para canal de entrada, é o loopback). **Zerar o ambiente antes de medir é parte da medição.** **Antes de ler qualquer medição, provar que o canal fala** — um comando de eco conhecido primeiro; zero num canal não confirmado não é dado, é ausência de dado. **Todo modo que silencia ou altera o instrumento tem que se declarar em todo comando de status, e a declaração atravessa o modo que declara.** **Documento vivo atualizado: sincronizar repo E vault na mesma ação** — nunca um destino de cada vez. **Estado de placa afirmado antes de um flash não sobrevive ao flash — reler depois de gravar, ou não afirmar.** **Todo alarme de boot precisa de um gêmeo consultável sob demanda.** **A queixa tem tantas partes quantas o dB nomeou: consertar uma e medir melhora não é consertar a queixa.** **Antes de gastar código numa hipótese, a medição que a mataria** — planura separa ruído de som legítimo. **Profundidade de EQ se escolhe contra régua medida (o sinal seco), nunca de ouvido.** **Toda gravação destrói estado: repor E declarar o que foi reposto faz parte de gravar.** **Ao ser o primeiro caso de uma forma nova, reler a assinatura em vez de imitar o vizinho.** **Valor lido de placa viva não tem procedência — default se lê no fonte.** **Canal de sinal novo entra com o medidor dele junto.** **Antes de misturar canais, perguntar o que cada canal É naquele ponto.** **Canal mudo prova que aquele canal está mudo, e nada além disso** — nunca inferir a saúde de um subsistema pela ausência de resposta de outro. **Nada de logar ou mandar MIDI de dentro do callback de áudio: o callback marca, o loop fala.** **Mecanismo corretivo automático precisa de carência entre agir e medir de novo, ou reage à própria ação ainda em curso.** **Chave nova num recurso compartilhado emudece quem depende dele: declare a dependência onde alguém vai olhar.** **Regra herdada tem pré-condição — verificar se a estrutura que a tornava possível existe no caso novo.** **Validade de medição tem REGIME, e o regime é parte do dado** — temporal (estreia/permanente) E físico (como o sinal está sendo produzido). **Antes de dimensionar uma solução, verificar se o problema existe no regime real** — e fazer primeiro o que serve para os dois desfechos possíveis. **Par assimétrico: troque os elementos de lugar antes de atribuir a assimetria ao sistema** — medir mais fundo só confirma o número, que nunca esteve em dúvida. **Justificativa errada envelhece mal:** registre a razão que se sustenta, não a que estava à mão. **Verificador que casa texto encontra a narração sobre o dado** — case na estrutura da linha de dado. **Controle que não existe mede zero, e zero parece dado:** número redondo demais em itens independentes é sintoma. **Delta entre builds não é atribuível:** custo se mede por A/B no MESMO binário, alternando por parâmetro. **Param sem leitor é feature morta até prova em contrário** — a checagem é textual e barata, e é a única que encontra o que o compilador não vê. **Feature morta parece feature desligada, e quando o default é "desligado" o estado normal É o estado quebrado.** **Efeito que trava um param internamente: a faixa declarada TEM que bater com a trava** — faixa maior é curso morto, e ninguém detecta, porque o defeito vive entre dois arquivos que estão certos. **Conserto que vira nota local reincide; conserto que vira pergunta de rotina, não.** **Refactor cria controle morto, não só código morto: depois de mudar a topologia, perguntar o que deixou de ser NECESSÁRIO.** **Variável ganha o segundo emprego quando alguém precisa de um valor e há um por perto — pergunte se o param já responde a outra coisa.** **Antes de dizer "não é possível", separar o que é LEI do que é DECISÃO minha** — argumento bom sobre premissa não examinada convence os dois lados. **Custo medido numa arquitetura não se transporta para outra.** **Para remover um subsistema, apague o símbolo-raiz e deixe o compilador enumerar o resto.** **Depois que ele calar, ainda falta o que não vê: params sem leitor, prosa sem assunto, e verificações sem objeto — esta última PASSA e ninguém desconfia de um `ok`.** **Deleção por faixa calculada: o perigo é proporcional ao que não é referenciado por nome** — o compilador testemunha símbolos e nada mais. **Casar → imprimir o que casou → conferir linhas → só então apagar.** **Recuperar um bloco não é restaurar um arquivo:** `git show HEAD:arquivo` + recolagem, nunca `checkout` com trabalho não commitado. **Contar errado para o lado autocrítico é o mesmo defeito com sinal trocado.** **Documento gerado acompanha a mudança; escrito à mão, não — separe os dois e trate os escritos como lista de trabalho.** **E o que se pode gerar, gere: cada doc que vira gerador sai da lista de disciplina para sempre.** **TESTE O HARDWARE PRIMEIRO, DE TODAS AS MANEIRAS POSSÍVEIS: torça, puxe, aperte cada conector e veja se algo muda** — contato intermitente é invisível a teste estático por definição. **Intermitência correlacionada com manuseio É o diagnóstico, não o que atrapalha o diagnóstico.** **Toda sonda ganha uma dimensão de movimento: medir enquanto se mexe no chicote, não com tudo parado.** **Tensão presente não é tensão correta: meça com o SINAL, ponta preta num terra do lado de quem alimenta.** **Defeito conhecido e adiado vira a explicação errada do próximo sintoma, e o custo recai sobre quem está com o multímetro na mão.** **Divergência entre dois instrumentos que medem por caminhos diferentes é informação, não ruído a reconciliar.** **O instrumento herda o ponto cego da hipótese que o gerou: supor o mapa e medir contra a suposição só acha o que já se esperava** — alargue o escopo em vez de refinar a mira. **Ocupação que vive só no default de uma biblioteca é irmã da reserva que vive só em documento: declare no fonte, mesmo quando for o default.** **Revisão de pinos contra o esperado entra em toda rotina de fechamento.** **Despacho por prefixo exige ordem: comando novo que começa com o nome de um existente tem que vir ANTES dele** — e o sintoma é o pior tipo, porque o comando responde com mensagem plausível executando outra coisa. **Ao retomar problema já resolvido, releia COMO foi resolvido: decisão registrada tem duas metades — o que saiu e o que ficou no lugar — e lembrar só da primeira reintroduz o que ela evitava.** **Mecanismo aposentado não aposenta o método: procure a peça que sobreviveu.** **Nunca ler e escrever o mesmo arquivo na mesma expressão: `open(f,'w')` trunca antes de o `read()` interno rodar.** **Commitar cedo é o que transforma um destroço em inconveniente.** **Antes de escolher uma configuração, leia o default na fonte: a decisão pode não existir.** **Ao provar um caminho, escolha o teste que falha diferente para cada defeito possível.** **Periférico sem caminho de leitura não é detectável — diga CONFIGURADO, nunca PRESENTE, mesmo com ele funcionando na frente.** **Janela de silêncio conta do FIM do gesto, não do meio: um despejo mais longo que a janela deixa os primeiros desprotegidos.** **Transformar "mandei" em "chegou" é a única forma de descobrir que a coisa não chegava** — toda operação que termina dizendo só que executou é candidata. **Ausência só é evidência quando a presença era possível: antes de concluir do silêncio, prove que havia caminho para o som.** **O gatilho artificial também precisa ser exercitado — "0 erros" onde se esperava vermelho é sinal de exercício que não aconteceu, não de trava errada.** **Antes de concluir do silêncio de uma sonda, prove que ela cobre o caso** — ponto cego não é ausência do fenômeno. **Defesa que nunca agiu e não tem gatilho artificial é hipótese com nome de proteção: faça o inventário do que nunca foi exercitado.** **Linha truncada não é comando: recuse inteira e conte** — meio comando é válido e obedece calado. **Truncagem se anuncia nas duas pontas do canal.** **O formato não pode ter limite que o conteúdo alcance** — alargar o buffer conserta o caso e preserva a forma. **Instrumentar um ramo de três e ler o silêncio como resposta é a armadilha da própria família.** **Operação com perda se declara onde a perda acontece.** **Antes de suspeitar do trabalho de alguém, meça antes da primeira camada** — contador em zero não distingue "não fechou" de "ninguém leu". **Cadeia `else if` esconde ramo morto por ordenação: use saída antecipada, específico antes do genérico.** **Não gravar enquanto uma observação humana está em voo** — a resposta chega depois e responde outra pergunta. **Par array+laço: derive os dois das fontes que já existem; alargar à mão conserta o caso e preserva a forma.** **Todo teste de rejeição precisa do gêmeo que confirma a aceitação** — provar que o portão fecha não prova que ele abre, e o mecanismo largo demais passa em todos os testes de rejeição. **Ao conferir formato binário: primeiro o que torna a conta possível, depois a conta, só então o que a conta protege** — ler campo antes do checksum é consumir antes de configurar (P2), e a coerência de tamanho é segurança de memória, não só diagnóstico. **Para provar que o binário gravado tem a fonte atual, leia NA PLACA um número que só a fonte nova produz** — timestamp, `.map` e toolchain são testemunho do próprio acusado. **Trava que só diz "não" ensina a contornar; trava que diz o que REEXAMINAR ensina a decidir.** **Custo proporcional a uma tabela que cresce regride por CRESCIMENTO, não por edição — e nenhuma ferramenta do projeto vigia isso:** pergunte no audit que custos foram dimensionados quando a tabela era pequena. **Ao proteger ou consertar um caminho, procurar o IRMÃO** — duas rotas de saída, dois relatórios da mesma tabela, dois ramos: o irmão é quase sempre visível de onde se está. **Comando que não consta na ajuda não existe para quem está na bancada** — e a isenção do verificador, escrita larga, transforma-o em carimbo. **Provar que a configuração nova é segura não prova que a TRAVESSIA é: pergunte o que já está em trânsito quando você troca** — e trânsito inclui fio, flash e a cabeça de quem opera. **Pôr uma dimensão nova no endereço converte mensagem órfã em MAL-ENDEREÇADA, e esta não tem gesto humano para correlacionar** — pergunte se acha o dono CERTO, não se acha dono. **Defesa que vira invariante de compilação vence defesa que roda — e ainda assim tem que ser exercitada uma vez.** **Instrumentação nova encontra o que não era o alvo dela: instrumente cedo.** **Desligar um emissor não drena o meio: todo desligamento de transmissor pede carência de drenagem, dimensionada pelo tempo de voo.** **Quando o canal vira parte do endereço, toda estrutura indexada por CC tem que ser reexaminada** — janela, inventário, último-valor e fila de quarentena, e o compilador não cobra nenhuma delas. **Nesta placa o delta entre builds pode ser 25% sem mudar o trabalho** (o código roda da QSPI, logo layout de instrução é variável de primeira ordem): número de CPU sem A/B no mesmo binário é ruído com aparência de resultado, e ganho de teste que permite o A/B fica no firmware. **N falhas crônicas escondem a falha N+1** — suíte vermelha por paisagem é sensor desligado. **Teste dentro de `if(existe)` some em silêncio quando o nome muda: afirme a existência.** **Teste que cita um CC decorado mede o mapa, não a invariante — derive o número da fonte.** **Pasta nova entra sem vigilância por padrão: confira se o verificador alcança.** **Nenhum valor do domínio pode significar "sem dado": ausência se representa FORA do domínio** — flag, sentinela impossível ou par valor+validade; cinco casos num dia provam que a forma é estrutural. **Relatório imprime o CRU e etiqueta a interpretação ao lado.** **Antes de gastar um voo numa hipótese, perguntar se falhar diminui o espaço de busca** — se não diminui, não é experimento, é tentativa, e tentativa se paga com gravação. **Segunda fonte de verdade para a versão deriva com data marcada: número que ninguém imprime não tem quem o corrija.** **Instrumento de medida nao pode depender de estado de utilizador — tom de referencia que obedece ao fader do musico mede o fader.** **Distorcao mede-se por LINEARIDADE em tres niveis, nao pela harmonica: a harmonica diz que ha, a escala diz onde.** **Via de teste entra depois do ganho e ANTES da protecao: o transdutor nao sabe que aquilo era um teste.** **Identidade de dispositivo e uma RELACAO: ao trocar hardware, pergunte QUEM PRECISA DE SABER quem ele e, nao onde esta o nome dele.** **Interpolacao de parametro corre por AMOSTRA: por bloco da a mesma escada, so que mais lenta.** **Quando um estado e descrito por mais de uma variavel, o rearme toca em TODAS — e a de escopo mais estreito e a esquecida, porque nem aparece de onde o rearme e escrito.** **Confirmacao de uma camada nao e prontidao da pilha: criterio de 'pronto' que usa a propria operacao a testar passa cedo demais por construcao — minimo incondicional primeiro, tentativa depois.** **Quando o suspeito e a causa se movem ao mesmo tempo, a coincidencia e indistinguivel da confirmacao — elimine a variavel (hub, adaptador), nao a explique.** **Eliminar por argumento nao e eliminar: 'isso nao pode ser' esconde a hipotese em vez de a descartar.** **Sonda que devolve zero tem de provar que consegue devolver diferente de zero — controlo positivo antes de ler ausencia.** **Quando o espaco de busca nao encolhe, pare de varrer e BISSECTE: um firmware minimo divide o problema ao meio numa gravacao, porque nao partilha nada com o suspeito.** **Antes de explicar um numero, verifique QUEM O ESCREVE — sinal partilhado da teoria errada, e teoria custa mais que numero.** **Hardware so se elimina por REPRODUCAO numa segunda unidade; duas nao falham igual por acaso.** **Deteccao por fall-through transforma 'nao reconheco' em 'e o mais antigo': pergunte a biblioteca o que ela detectou, nunca presuma.** **Quando N tentativas falham de formas diferentes mas todas do MESMO TIPO, o tipo e a resposta — escreva o que todas as falhas tiveram em comum antes da tentativa N+1.** **Constante de protocolo se le na fonte, nao se deduz do comportamento: numero deduzido nao parece suspeito porque EXPLICA os dados.** **Antes do quarto valor da mesma variavel, escreva o que o terceiro descartou — se for nada, troque de variavel.** **Sonda nova nasce sem procedencia: o primeiro uso de qualquer sonda e o controlo de presenca, nunca a medicao.** **Antes de aceitar trabalho manual repetitivo, procure o formato de intercambio — a pergunta e onde isto vive quando nao esta na tela.** **Todo gerador de formato desconhecido reproduz uma amostra conhecida antes de emitir o resto.** **Endereco por posicao transforma edicao de layout em reenderecamento silencioso: regere o conjunto todo, ou nao use posicao.** **Todo contador de verificador precisa de piso: zero onde se esperava populacao ABORTA, nao informa — `ok` ao lado de um zero e a assinatura da familia.** **Antes de aceitar um criterio, escreva o que teria de acontecer para ele FALHAR — se for 'nada que o codigo possa fazer', ele nao mede o codigo.** **Num teste de isolamento, o lado que se espera que NAO mude e o que precisa de prova de vida.** **Mapa de canais mede-se, nao se assume: indice trocado nao produz silencio, produz veredito trocado de lado.** **Regra que vive so em prosa custa a mesma disciplina toda vez — ponha-a DENTRO do instrumento.** **Antes de reportar que uma accao humana piorou alguma coisa, prove que o sistema estava a responder depois dela: o custo de errar recai sobre quem esta com o alicate na mao.** **O firmware nomeia SAIDA; o cobre nomeia DESTINO — quando N fontes discordam sobre o que uma coisa E, pergunte se a coisa e decisao de outra pessoa ou de uma chave de metal.** **Regra derivada classifica o que ainda nao existe, e em silencio: ao criar simbolo novo, leia a regra primeiro e pergunte onde ELA o poe.** **Ao escrever a regra, pergunte qual e a MENOR afirmacao que cobre o caso — a regra larga contradiz o hardware real e quem a le acredita nela.** **Ao encontrar um erro num documento, meca o resto antes de corrigir esse: abaixo de um limiar, corrigir uma linha e desinformar.** **Quando o canal principal cala, o sistema operativo continua a testemunhar — enumeracao, idProduct e marca de tempo do node nao dependem do acusado.** **A primeira leitura de qualquer sonda e aquecimento: numero que nao fecha com caminho nenhum do codigo e suspeita contra o INSTRUMENTO primeiro.** **Sinal periodico continuo mede mal tudo o que produz copias de si proprio: compare contra o CHAO com uma variavel so.** **Instrumento cuja memoria e apagada pelo evento que ele mede so serve enquanto ninguem precisa dele.** **Ler um contador com uma sonda que o zera e a familia inteira em miniatura.** **Ecra e memoria, nao janela: conteudo de ecra nao prova que alguem escreveu AGORA.** **Quando uma mudanca nao pode causar o sintoma, pare de procurar como ela o causou — ler a implementacao e barato e conclui.** **`assert texto in fonte` prova existencia, nao UNICIDADE: confira `count()==1` e verifique PROGRAMATICAMENTE onde o bloco caiu.** **Defesa que existe e nao se aplica por habito nao e defesa, e sorte.** **Quando N fontes discordam e nenhuma mede, nao escolha entre elas — meca; e se a ferramenta ja existe no repo, pergunte por que nao foi a PRIMEIRA a correr.** **Toda sonda declara o que NAO alcanca: o `varre` le 15 dos 31 pinos.** **O que MUDA gera-se; o que DURA e dado — e ao gerar metade de um documento, escreva o verificador da outra metade: a parte manual fica rodeada de dados frescos, o que a torna MAIS convincente.** **Gerador que nao ESCREVE deixa a ultima milha para a disciplina humana.** **Instrumento cuja memoria e apagada pelo evento que mede: guarde fora da RAM, e conte duas contas — a da sessao e a de sempre.** **Todo laco de drenagem tira um INSTANTANEO do ponteiro de escrita — perseguir o ponteiro vivo so e correcto enquanto o produtor for mais lento que o consumidor, e ninguem escreve isso na condicao.** **Sonda que vive no mesmo contexto que o suspeito morre com ele: o instrumento de fase salvou esta caca por correr no callback de audio.** **Alarme que dispara em operacao sa ensina a ignora-lo.** **Banco nativo prova a ALGEBRA, nao o TIMBRE: se existe medicao da fonte real, o teste alimenta-se DELA.** **SE NAO SOA, NAO FICA — e vale para pedacos de curso, nao so para params.** **Ao juntar um controlo manual a um automatismo que age no mesmo eixo, pergunte qual dos dois o ouvido vai ouvir.** **Guarda escrita antes de o hardware existir nao foi exercitada: ao ligar o periferico, EXERCITE-A em vez de confiar nela.** **Controlo absoluto que partilha um param precisa de dizer QUANDO manda — captura, adocao no arranque, e largar quando outro escreve sao a mesma pergunta em tres momentos.** **Capacidade declarada e uma afirmacao sobre o hardware e envelhece como qualquer outra: enquanto o driver e stub, ninguem a cobra.** **Lista de 'pinos livres' e afirmacao sobre POSSE: ao dar dono a um pino, pergunte quem e que assume que ele nao tem dono.** **Antes de acreditar numa sonda, pergunte o que ela faria se a resposta fosse a OUTRA — sonda que nao produz os dois resultados nao mede, decora.** **A sonda mais cara e a que confirma a hipotese de quem a escreveu.** **Sonda com pull nao le antes de o condensador carregar; sonda que dirige o pino como saida digital desliga o ADC que vinha medir.** **Antes de acusar uma peca, diga o que teria de ser verdade para ela ser INOCENTE — e verifique isso.** **Se a recuperacao vem sempre do mesmo gesto (reset, religar), a variavel que julga estar a testar nunca esteve isolada.** **Numero derivado de medicao herda a validade da medicao: quando a causa muda, toda constante calibrada contra o sintoma tem de ser recalibrada.** **Apertar defesa contra ruido que nao existe custa RESPOSTA.**

## 15ab. A histerese media a partir do centro, e o pot parado calava o iPad (31/08)

Sintoma: os pots estavam certos, quietos, e mesmo assim o `level` **não
respondia ao Loopy**. O contador dizia **210 150 janelas de silêncio em 20 s
com os pots imóveis** — o ritmo do laço.

A guarda contra tremor era esta:

```c
knob_passo[k] = (int)(em_passos + 0.5f);              // arredonda ao mais próximo
if(fabsf(em_passos - atual) < kKnobHist) continue;    // exige 0,35
```

**Arredondar ao mais próximo deixa um resto de até 0,5 — maior que os 0,35
exigidos.** Para qualquer posição cujo resto caísse nessa faixa, a guarda nunca
fechava: o laço reescrevia o mesmo valor eternamente. E como quem escreve abre
250 ms de silêncio ao MIDI, um pot parado silenciava o iPad para sempre.

O conserto é uma linha — a margem conta-se **a partir da fronteira do passo**,
não do centro dele:

```c
if(fabsf(em_passos - atual) < 0.5f + kKnobHist) continue;
```

> **Uma guarda mal posta imita ruído com perfeição.** Medi o ADC no mesmo
> instante: **0,23 passos de pico-a-pico**, sete vezes abaixo do limiar. Eu já
> tinha a hipótese pronta — impedância de saída do pot é máxima a meio curso, e
> os pots *estavam* a meio curso. A hipótese era verdadeira em física e
> irrelevante aqui. **Foi medir o ruído que me impediu de o condenar**, e é a
> terceira vez nesta semana que acuso o hardware antes de ler o meu código.

E o defeito escondia-se por trás de um sintoma que ninguém liga a um pot: *"o
Loopy não mexe no volume"*. Contador que conta o que devia foi o que o revelou
— sem `knob_janelas`, isto era um mistério de MIDI.

## 15ac. Escolhi o metodo pela familiaridade, e a prova reprovou-o 6 em 6 (31/08)

Para achar a frequencia da microfonia escrevi um cacador por **autocorrelacao**
— porque e o que se usa para achar periodo, e eu sabia faze-lo. Antes de gravar,
escrevi a prova em Python com senoides. **Falhou os seis casos:**

```
senoide pura 848 Hz  ->  169.59 Hz   (erro 80%)
senoide 220 Hz       ->  109.89 Hz   (erro 50%)
```

169,59 e **848 dividido por 5**. Duas razoes, e as duas de fundo:

1. A autocorrelacao tem pico em **todos os multiplos** do periodo, e o maximo
   global cai num multiplo alto.
2. Pior: ela mede **PERIODICIDADE**, e nao qual parcial e mais forte. Com a
   corda a 311 Hz e a microfonia a 1900, responderia sobre a corda — **a
   pergunta errada, respondida bem.**

Trocado por FFT com janela de Hann e interpolacao parabolica em dB: **8 em 8**,
incluindo uma frequencia entre bins e uma nota com harmonicos.

> **A ferramenta escolhe-se pela PERGUNTA, nao pela familiaridade.** "Achar
> periodicidade" e "achar o parcial mais forte" parecem a mesma pergunta e nao
> sao — e a diferenca so aparece quando ha mais de um som ao mesmo tempo, que
> e o caso real e nunca o caso de teste que se imagina primeiro.

E a licao de processo: **a prova em Python custou dez minutos e poupou uma
sessao inteira de bancada** a perseguir um cacador que apontaria sempre para o
sitio errado, com o instrumento a gritar em cima.

## 15ad. Um instrumento de medida com 3% de ciclo util nao mede: amostra (31/08)

Com os notches postos, o dB disse que sobravam duas frequencias. Corri o
cacador cem segundos e devolvi **"nao gritou nesta janela"**. Estava errado.

Cada captura olha **43 ms** de audio. Pedida pela serie, o ciclo inteiro levava
**~1,3 s**. Eu olhava **3% do tempo** e afirmava sobre os 100%.

Na primeira sessao tive sorte — dois acertos em vinte e cinco pedidos — e a
sorte fez o metodo parecer bom.

O conserto foi passar a caca para dentro da placa, como maquina de estados no
laco (nao um `while` bloqueante, que teria parado MIDI, botoes e encoders). Na
primeira corrida: **999 capturas em 45 s**, ~95% do tempo. E as duas
frequencias apareceram imediatamente, em 199 e 252 capturas cada.

> **Amostragem esparsa de um evento raro devolve "nao ha evento" com a mesma
> cara com que devolveria a verdade.** Antes de aceitar um negativo, calcular
> que fatia do tempo o instrumento estava mesmo a olhar.

## 15ae. Calibrei o discriminador em senoides e ele teria recusado a microfonia real (31/08)

O cacador classificava por **dominancia** — que fatia da energia esta no pico.
Nas senoides sinteticas a microfonia dava 0,93–1,00, e pus o limiar em 0,85,
com o `notch auto` a **recusar-se a cortar** abaixo de 0,60.

A microfonia real deu **0,61 e 0,68**. Passou por um centesimo.

A razao e fisica e obvia depois: num corpo de metal a gritar, **o corpo inteiro
ressoa** — o pico nunca fica sozinho como uma senoide de laboratorio.

O que de facto a separava nos dados reais era outra coisa: **+45 dB contra um
chao de -43**, e nao decair. Oitenta e oito decibeis de diferenca, e eu a
julgar por uma fraccao com tres centesimos de margem.

> **Limiar calibrado em sinal sintetico e uma hipotese, nao uma medida.** O
> sinal de teste nao tem o que torna o caso real dificil — se o tivesse, nao
> seria sinal de teste.

## 15af. Contei quem nao tinha CC, e o que estava mudo tinha CC ROUBADO (31/08)

Os dez params do notch entraram no CC **115**, que ja era do `bench_boost`
**nos mesmos canais**. O mapa diz na propria linha:

```cpp
return midi_map[i].param_id; // primeiro vence; duplicata e erro
```

*Duplicata e erro* — e **ninguem verificava**. O notch foi registado antes,
ganhou, e o `bench_boost` ficou inalcancavel pelo iPad **em silencio**.

O que o deixou passar foi eu olhar para o contador errado. O `board` dizia:

```
sem CC: 0 de 95 params — alcancaveis por `set`, invisiveis ao iPad
```

E dizia a **verdade**. Um param sombreado **tem** entrada no mapa: passa nesse
teste com nota cheia.

> **"Sem CC" e "com CC roubado" sao perguntas diferentes**, e so uma estava a
> ser feita. Um verificador responde exactamente o que lhe perguntam, e o
> silencio dele nao cobre o que ninguem perguntou.

Conserto: notch para 82–86, e `Manifest::Colisoes()` a gritar no `board`.
E o alarme ganhou via consultavel a qualquer momento, nao so no banner — a
regra que a §2b da bancada ja impunha, e que aqui se aplicou de primeira.

## 15ag. O instrumento nao distinguia os dois suspeitos que existia para separar (31/08)

A placa travava na FASE 2, e a fase 2 tem **sete** inquilinos. Construi
sub-fases para apontar o inquilino, e o LED passou a mostrar a sub-fase
**enquanto a fase fosse 2**:

```c
const uint8_t bruto = (loop_fase == 2u && loop_sub) ? loop_sub : loop_fase;
```

O dB contou **6**. E `6` tanto era a **FASE 6** (`PollMidi`, que RECEBE) como a
**SUB 6** (`MidiEspelha`, que TRANSMITE).

Eu tinha visto a ambiguidade e escrevi-a no aviso ao dB — e mesmo assim gravei
assim, porque "na pratica ela trava sempre na fase 2". A primeira leitura real
caiu exactamente no numero ambiguo.

Conserto: **duas rajadas**, fase e sub-fase, com pausas diferentes.

> **Se um instrumento pode confundir os dois candidatos que ele existe para
> separar, ele nao esta pronto** — por mais improvavel que a colisao pareca. E
> "improvavel" calculado por mim, sobre o defeito que ainda nao entendi, nao e
> um calculo: e uma preferencia.

## 15ah. Elevei um sintoma a "condicao que a hipotese exige" (31/08)

Durante a caçada, abrir o CDC da Daisy levava **47, 58,7 e 59,2 segundos** —
tres medicoes independentes, consistentes. Construi raciocinio em cima:

> *"Ha uma janela de ~1 minuto depois de cada arranque em que nada drena o CDC.
> E exactamente a condicao que a hipotese exige, e agora esta medida em vez de
> suposta."*

Quando a causa real foi corrigida, **a porta passou a abrir em 0,0 s**.

Os 59 segundos nao eram condicao de nada. Eram a placa meio-morta a nao
completar a enumeracao — **sintoma do mesmo defeito**, que eu promovi a
mecanismo porque encaixava na historia que eu ja tinha.

> **Medir tres vezes o mesmo numero prova que ele e estavel, nao que ele e
> causa.** A pergunta que faltou: *este numero existe quando a placa esta bem?*
> Nunca a fiz — nao havia placa boa para comparar, e em vez de declarar isso,
> usei o numero como se houvesse.

## 15ai. Cacei software durante horas, e o defeito era um cabo (31/08)

Sintoma: LED preso na fase 2, sem serial, reproduzivel de instalacao limpa
(bootloader reinstalado do DFU da ROM + aplicacao regravada). Todo o desenho da
caçada — tres voos, uma variavel, suspeito nomeado — apontava para o
`TransmitInternal` e o CDC entupido.

**Era a ponte ESP32 desligada.** O dB tinha-a removido nessa tarde para caçar
ruido de amplificador. Com ela fora, o **D14 (RX) ficava a flutuar** com o DMA
a escuta permanente; a USART1 e a mesma para TX e RX, e a linha solta prendia
`midi_uart.BlockingTransmit`. A `MidiEspelha` entrava e nao voltava.

Reconectar a ponte curou tudo. **Zero linhas de codigo.**

A prova ficou do outro lado do fio, no `board` da propria ponte:

```
msgs: BLE->ser 0 · ser->BLE 105 · TX sem espaco 0
```

105 mensagens vindas da UART — o varrimento completo do espelho, um por param.

> **O que mudou no MUNDO entre "funcionava" e "nao funciona" vem antes de
> qualquer hipotese sobre codigo.** A ponte saiu naquela tarde, e eu tinha essa
> informacao o tempo todo — fui eu que a vi sair. So a usei depois de sete
> inquilinos isolados e cinco gravacoes.

E fica o defeito real, que e de peca de galeria e continua aberto:
**um cabo de ponte solto mata o instrumento.** Numa peca exposta ao publico,
replicada em serie, isso nao pode ser. Conserto devido: barreira antes do
`BlockingTransmit` (perguntar se a anterior concluiu; se nao, **descartar e
contar**), mais rearme do estado de erro da USART.

## 15aj. Pedi o gesto errado: o DFU da ROM nao conhece a QSPI (31/08)

Pedi `BOOT+RST` para gravar a aplicacao. Resultado:

```
Downloading element to address = 0x90040000
dfu-util: Last page at 0x9007f663 is not writeable
```

`BOOT+RST` entra no **DFU da ROM da ST**, que so oferece `@Internal Flash` e
`@Option Bytes`. A aplicacao vive na **QSPI**, em `0x90040000`, e quem sabe
escrever la e o **bootloader da Daisy**, que aparece num **RST simples**.

| gesto | DFU que aparece | escreve |
|---|---|---|
| `BOOT` + `RST` | ROM da ST | flash interna (**bootloader**) |
| `RST` simples | bootloader da Daisy | **QSPI** (aplicacao) |

Conserto do metodo: o vigia de gravacao passou a esperar por `0x90000000` no
`dfu-util -l`, e nao por `df11` — que os dois DFU partilham.

## 15ak. O instrumento so era valido com o paciente parado (31/08)

As sub-fases apontavam o inquilino da fase 2, e eu li os numeros como
diagnostico a tarde inteira. Ate o dB reportar **"1 e 6"** — uma combinacao que
o codigo nao consegue produzir: entre o `SUB(6)` e a `FASE(1)` ha um `SUB(7)` e
um `SUB(0)`, e nao ha `continue`, `goto` nem `break` que os salte (conferido).

Logo os valores **estavam a mudar durante a leitura**. E o LED le os dois a cada
volta do callback, mas o padrao dura ~5 s: se mudarem no meio, o `dur` muda, o
`t = ms % dur` salta, e o numero deixa de significar coisa nenhuma.

> **O instrumento so e valido com o laco PARADO.** Com o laco a correr ele
> produz numeros arbitrarios — e eu li "2 e 6" como diagnostico quando ja podia
> ser ruido. Consertei o `BlockingTransmit` a partir dessa leitura, e o conserto
> nao curou nada: nao havia espera para limitar.

Um instrumento tem de declarar **em que regime e valido**, e recusar-se a
responder fora dele. Este respondia sempre, e com a mesma cara.

## 15al. Escalei a suspeita antes de ler o documento que eu proprio escrevi (31/08)

O dB reportou que o USB da ponte acendia o LED do amp. Respondi *"para tudo,
isso e defeito de ligacao e pode danificar pecas"*, e pedi quatro medicoes de
continuidade.

Era comportamento conhecido, e a regra estava escrita no `esp32/README.md`:

> *"Bancada: alimenta por USB **ou** pelo pino de 5 V, nunca os dois."*

Na ESP32-C3 SuperMini o `5V` liga ao VBUS **directamente no cobre**. Alimentada
pelos dois, o USB realimenta o trilho. Escrita por mim, violada por nos os dois,
e nao lida por mim quando o sintoma dela apareceu.

> **Antes de escalar um sintoma a defeito novo, procurar se ele ja esta
> descrito.** Custa um `grep`. Escalar custa a atencao de quem esta na bancada,
> num momento em que ela vale muito.

E o conserto certo nao foi o alarme: foi um **Schottky de 5 centimos** que torna
o erro impossivel. **Regra depende de alguem a ler; peca nao.**

## 15am. Apresentei uma regra por testar como se fosse a causa (31/08)

Descobri que o meu conselho de tirar a terra de sinal estava errado, escrevi a
correcao na §5b do `ATERRAMENTO.md`, e disse ao dB que **era essa a causa** da
placa nao arrancar. Ele soldou a terra de volta.

**Nao curou.** Serie ainda a levar 59 s, ainda muda. E o teste seguinte mostrou
o que faltava: a ponte tem de estar **ALIMENTADA**. Terra sozinha nao chega —
um chip sem alimentacao pendurado na linha de RX estraga a USART na mesma, com
ou sem referencia comum. Os dois caminhos de terra funcionam igualmente.

A regra continua boa pratica. Mas eu passei de "descobri um erro meu" para
"logo, e esta a causa" **sem um unico teste pelo meio** — e fiz o dB soldar por
causa disso.

> **Descobrir que se errou nao e o mesmo que descobrir a causa.** As duas
> chegam com a mesma sensacao de alivio, e so uma delas se mede.

## 15an. O master mudo disfarcou-se de piezo avariado cinco vezes (31/08)

Sintoma: o tom de teste sai alto e limpo, os piezos nao soam. O dB ia
**subir o ganho dos piezos** — mudanca permanente no instrumento, paga em
margem de microfonia, num sitio onde ninguem ligaria as duas coisas depois.

Era o `level` a zero. O tom entra **DEPOIS** dele; os piezos, antes. Essa
assimetria e que engana, e ela apontava para piezo, cabo, jack, ADC — tudo
menos para o param que ninguem ve.

Aconteceu **cinco vezes num dia**, sempre depois de gravar, porque todo param
nasce em zero por regra da casa e um master a zero e silencio.

Tres consertos, e sao de tipos diferentes:

1. **Os nomes passaram a carregar a topologia** — `level` -> `pre_level_ab`
   (antes do governor/limiter: alimenta a proteccao), `level_l`/`level_r` ->
   `post_level_l`/`post_level_r` (depois do limiter: so reduzem). O dB
   perguntou se nao eram redundantes; o codigo diz que nao, e agora o nome
   diz tambem.
2. **O `pre_level_ab` nasce a 1.0** — primeira excepcao deliberada a "todo
   param nasce em zero", com o motivo ao lado: ele e de TESTE e EMERGENCIA,
   o volume de performance sao os `post_*`, e o piezo deve entrar quente na
   dinamica de saida.
3. **O instrumento passou a explicar-se.** O `board` grita quando o master
   esta a zero, e diz PORQUE o tom soa e os piezos nao.

> **Um param sem controlo fisico, que nasce em silencio e mata so METADE das
> vias, e uma armadilha desenhada.** A metade que sobrevive e que faz o
> diagnostico apontar para o lado errado.

## 15ao. A rampa que nao soou nao ficou (31/08)

Gravar produzia um estalo seco. Isolei por eliminacao — abrir a porta, comando
curto, comando longo, fechar: **nenhum estalava** — e conclui que era o reset a
cortar o audio a meio de um bloco. Pus uma rampa de 30 ms no callback para
chegar ao reset ja em silencio. Desenho correcto, raciocinio correcto.

**Estalou exactamente igual.**

Logo o degrau nao esta nas amostras: esta no codec a perder relogio, ou no amp
a ver a entrada em alta impedancia. E a rampa custava uma espera de ate 200 ms
no caminho do reset.

> **SE NAO SOA, NAO FICA — e a regra vale contra o proprio conserto.** Ela
> saiu, com o registo no lugar dela para ninguem a reinventar com o mesmo
> raciocinio, que continua a parecer bom.

## 15ap. O painel custava 6 dB, e o numero desenhou a regra (31/08)

O dB reportou ruido novo ao ligar o OLED. Medido, com o instrumento parado:

| | painel apagado | painel aceso |
|---|---|---|
| IN_L | -75,3 dBFS | **-69,3 dBFS** |
| pico espectral | 119,7 Hz, dom 0,28 | **7040,8 Hz, dom 0,95** |

Dominancia 0,95 e assinatura de **relogio**, nao de ruido termico. E 7040 Hz
tem explicacao exacta: um painel de **64 linhas** a ~110 Hz de quadro varre
64 x 110 = **7040 linhas por segundo**. E a corrente de accionamento das
linhas, proporcional aos pixeis ACESOS — foi por isso que a medicao com os
pixeis apagados ficou limpa.

O dB perguntou se nao se podia filtrar. Podia — um notch de 201 Hz a 7 kHz e
musicalmente quase de graca. **Mas nao escala:** a peca e replicada em serie e
cada painel tera o seu relogio. Notch fixo acerta numa unidade e falha nas
outras; **painel apagado acerta em todas**, custa zero DSP, e deixa as bocas do
notch livres para a microfonia.

A regra que ele desenhou: painel nasce apagado, acorda no encoder, dorme aos
5 s. Durante a performance ninguem lhe toca — logo esta apagado exactamente
quando o ouvido esta a trabalhar.

> **Quando o conserto que escala e o mais simples, e porque a pergunta estava
> mal posta.** "Como filtro este ruido" tinha resposta; "porque e que ele
> existe quando ninguem olha para o painel" nao precisava de filtro nenhum.

**E o painel ganhou uma regra de operacao que o dB viu e eu nao:** com a tela
apagada, quem gira nao ve o que muda. Se o primeiro gesto ja mexesse no valor,
alterava-se as cegas e so se descobria depois de a tela acender — sem saber de
quanto. Entao **o gesto que acorda e engolido**.

⚠️ E engolir e LER E DEITAR FORA, nao "nao ler": `EncoderIncremento` e
`EncoderPushBorda` **consomem** o evento. Deixa-los por ler guardaria o gesto
para a volta seguinte, e ele seria aplicado na mesma — so que atrasado, que e
pior do que aplicar logo.

**E a primeira versao engolia um EVENTO, quando devia engolir um GESTO.** O dB
apanhou-o na bancada:

> *"um gesto de ligar o oled e quantos passos forem ate parar. Fica dificil
> numa situacao de show mexer exactamente um clique."*

Eu pensei em eventos; o dedo pensa em gestos. Ninguem roda um detente em palco
— pega-se no botao e roda-se ate ver. Engolir so o primeiro deixava todos os
outros passar as cegas, que era exactamente o que a regra existia para evitar.

Conserto: **cada evento adia o fim da janela**, e ela fecha 400 ms depois de a
mao parar. Rodar sem parar mantem tudo engolido. E os 400 ms nao sao palpite:
entre detentes de uma rotacao deliberada passam 100-200 ms, e janela mais curta
partiria o gesto ao meio.

> **Interface mede-se em gestos, nao em eventos.** A unidade do codigo e o
> detente; a unidade da mao e o movimento inteiro. Quando as duas nao coincidem,
> quem esta a tocar e que paga.

**E ainda faltava uma volta.** A versao seguinte engolia ate a mao parar, mas
**acendia no primeiro evento**. O dB: *"ainda nao deu, nao e por tempo."*

A regra que ele queria: ouve o encoder, **fica a engolir com o painel apagado**,
e so acende 500 ms depois de parar. A diferenca e pequena no codigo e grande na
mao — o painel a acender a meio da rotacao pisca no canto do olho e sinaliza
"estou a ouvir-te" enquanto ainda esta a deitar tudo fora. Acender no FIM faz do
acender a **resposta ao gesto completo**, e o que se ve ja e o estado com que se
vai trabalhar.

**E a quarta volta foi bug, este sim.** Com a fronteira certa, o painel
continuava apagado — e o dB descreveu-o exactamente: *"giro, continua apagado;
paro, continua apagado; giro de novo ele acende e ja esta mudando valores."*

O refrescamento so escreve quando o conteudo **muda**: compara com
`enc_tela_ant` e cala-se se for igual. Ao acordar nada tinha mudado — os
encoders estao onde estavam — portanto nao desenhava, e a tela, apagada ao
adormecer, ficava preta. So o SEGUNDO gesto mexia num valor, criava diferenca,
e ai desenhava. As duas metades do relato eram a mesma causa.

⚠️ **E o padrao ja estava no ficheiro, em dois sitios, com o comentario
`// forca redesenho`.** Terceira vez no mesmo dia que a resposta estava escrita
e eu nao a procurei — depois de ja ter escrito a cicatriz sobre isso.

**A quinta volta foi so o numero.** Com tudo a funcionar, o dB afinou a pausa
que fecha o gesto: 500 ms pareceu lento, 100 apertado, **200 ficou**. Aguenta
ser curto porque a janela nao precisa de cobrir a rotacao inteira — cada
detente ADIA o fim dela, entao so tem de ser maior que o intervalo entre dois
detentes de uma rotacao continua.

⚠️ **Esse numero e do PULSO de quem toca, e nao de calculo**, e esta escrito
assim no codigo. Se algum dia parecer errado, o teste e a mao, nao o
osciloscopio.

> **Tres tentativas de desenho, e nenhuma foi bug.** Foram tres modelos errados
> do mesmo gesto — evento, depois duracao, depois so a fronteira. O dB corrigiu
> as tres a operar, e eu nao teria chegado la a medir: nao ha instrumento na
> placa que meça "isto distrai quem esta a tocar". A quarta foi bug, e o
> conserto ja existia no ficheiro.

## 15aq. O comando morria no portao dos "sem dono" — e era codigo morto (01/09)

O `PRESETS-LISTA.md` descrevia o CC 121 (carregar preset) desde sempre, e
**nada o implementava**: os botoes de preset do Loopy eram inertes. Documento a
descrever intencao como se fosse comportamento e uma armadilha — quem o le
acredita.

Implementei, e nao funcionou. O log nem aparecia. A causa:

```cpp
const int pid = manifest.FindMidiParam(ch, cc);
if(pid < 0) { cc_sem_dono++; return; }   // <- o 121 morria AQUI
```

O 121 **nao e um param**, portanto nao tem `pid`, portanto o portao dos "sem
dono" descartava-o — e eu tinha posto a intercepcao **a jusante** desse portao.
Era codigo morto, e o unico sintoma era **nao acontecer nada**: indistinguivel
de o CC nao ter chegado.

> **Um comando que viaja pelo mesmo cano dos dados tem de ser desviado ANTES do
> filtro que existe para deitar dados fora.** O portao estava certo; o meu
> lugar e que estava errado.

**E ao mover, apareceu um risco maior que o conserto.** A intercepcao passou
tambem a frente da QUARENTENA — a defesa contra o dump de reconexao do BLE. Um
121 velho nesse dump carregaria um preset a meio da peca.

> **Param errado corrige-se com o dedo; PRESET errado troca o instrumento
> inteiro numa mensagem.** A quarentena passou a valer para o comando tambem, e
> com mais razao do que para um param.

E o CC 122 (gravar) ficou POR FAZER, deliberadamente: um CC que escreve preset
destroi uma biblioteca com uma mensagem perdida, e nao traz nome nenhum para lhe
dar. Capturar fica no serial, onde o gesto e deliberado e o preset nasce com
nome. **Decisao do dB, e nao omissao minha** — ele concordou por escrito.

## 15ar. A folha de montagem ia mandar montar um interruptor onde e um fader (01/09)

O gerador da seccao do Loopy classificava widgets assim:

```python
if passos == 2 or (lo == 0.0 and hi == 1.0 and passos in (0, 2)):
    return "Botao (toggle)"
```

Tratei "faixa 0-1 sem passos declarados" como interruptor, e e o **contrario**:
sem passos e CONTINUO. O `pre_level_ab` saiu como botao — e o dB montaria um
interruptor onde precisa de um fader, num documento cuja unica funcao e dizer o
que montar.

E `passos` sozinho tambem nao chegava: os `_on` desta casa declaram-se
`0.f, 1.f` sem passos nenhuns e sao interruptores por convencao. **Entao o NOME
tambem decide** — o que e mais um retorno de os nomes carregarem a topologia:
serve as ferramentas, nao so a quem le.

> **Gerador tambem tem premissa, e a dele grita menos que a de uma pessoa:**
> ele emite a tabela errada com a mesma confianca com que emitiria a certa. O
> `gerar_ccmap.py` ja tinha ensinado isto ao falhar oito CCs de um notch com dois
> indices; foi preciso repetir noutra ferramenta no mesmo dia.

## 15as. Entreguei duas folhas que nao abriam, e o verificador disse que estavam limpas (01/09)

Duas folhas de KiCad entregues com o verificador de netlist limpo e a revisao
visual feita **tres vezes**. Nenhuma das duas abria: `Failed to load schematic`,
uma linha, sem apontar para lado nenhum.

```
gerado    (symbol "D_V_0_1_0_1")   <- quatro numeros
correcto  (symbol "D_V_0_1")
```

O `kiutils` acrescenta `_unitId_styleId` sozinho e o gerador ja punha o sufixo a
mao. O KiCad separa esse sufixo para deduzir o nome do simbolo PAI, encontrava
`D_V_0_1` — que nao existe — e recusava a folha inteira.

> **"O verificador passou" e "a folha abre" sao perguntas DIFERENTES.** O
> `check_sch.py` confere CONECTIVIDADE; nada conferia a GRAMATICA do ficheiro. E
> quem le "verificacao limpa" acredita nas duas.

E o `kicad-cli` estava disponivel o tempo todo, dentro do bundle da aplicacao.
Deu-se por ausente porque se correu `which kicad-cli`. **Procurar pelo sitio,
nao pelo nome** — foi por isso que a folha seguiu sem ERC nenhum.

Nasceu daqui a skill `kicad`, porque a licao nao tinha onde carregar sozinha.

<!-- doc-check: ignore pin_not_connected (codigo de violacao do ERC do KiCad) -->

## 15at. Editei, renderizei, olhei — e nao repeti o ERC (01/09)

Ao mover dois `PWR_FLAG` para fora dos rotulos que estavam a tapar, refiz o
render e inspeccionei o desenho. **Nao voltei a correr o ERC.**

Ele teria dito logo: `pin_not_connected`. **Pino que cai no MEIO de um segmento
de fio nao liga no KiCad** — liga em ponta de fio, ou onde ha juncao explicita.
As bandeiras estavam soltas, e o desenho mostrava-as encostadas ao fio, bonitas.

O erro so apareceu na verificacao seguinte, e por acaso: eu estava a conferir
outra coisa.

**Editar invalida todas as inspeccoes anteriores** — isso e verdade e e a
licao facil. Mas o dB afiou o enunciado, e a versao dele e a que importa:

> **A inspeccao visual era ESTRUTURALMENTE INCAPAZ de apanhar este defeito.**
> Fio e pino desenhados no mesmo sitio, sem ligacao nenhuma. Nao havia nada
> para o olho ver — nem com mais atencao, nem com mais ampliacao, nem com
> mais iteracoes.
>
> **Olho e ERC nao sao dois niveis de rigor da MESMA pergunta. Sao perguntas
> DIFERENTES, e uma nao substitui a outra.**

E e por isso que refazer a visual me deu a sensacao de ter verificado: eu tinha
mesmo refeito uma inspeccao, com cuidado, e ela passou. So que era a inspeccao
que nao podia falhar ali.

A **15as** diz o mesmo do outro lado do espelho: verificador de REDE limpo numa
folha que **nem abria**. As duas juntas formam a regra:

> **Antes de citar uma verificacao, perguntar o que ela e CAPAZ de ver.** Uma
> verificacao limpa so absolve dentro do seu dominio, e o silencio dela sobre o
> resto tem exactamente a mesma cara.

Corolario pratico: **editou -> regerou -> repetir TODAS as verificacoes**, e nao
so a que se lembra. Aqui eram tres — abre, ERC, netlist, render — e refazer uma
delas nao diz nada sobre as outras.

---

*"Sem medir antes de concluir, o freeze teria sido condenado pelo número errado, o módulo do Fx2 descartado pelos 234 ms de estreia, e o warm-up desenhado como uma chamada só — que não resolveria nada."* — o parágrafo que resume tudo, escrito pelo Code no fechamento da fase 1 e provado verdadeiro em cada capítulo desde então.

## 15au. Tres medicoes, o mesmo numero, e eu publiquei as duas metades sem as multiplicar (01/09)

Media o custo do painel OLED em tres prescalers. As tres corridas imprimiram
`DISPARO pico 23288 us` — **exactamente o mesmo numero**, ate ao microssegundo.

Era o pico da PRIMEIRA corrida. `oled_fb_us_pico` e um maximo retido, e um maximo
so sobe: as duas corridas seguintes eram seis vezes mais rapidas e o numero nao
mexeu. **Numero de outra corrida com cara de numero desta.**

E eu entreguei ao dB, na mesma frase:

> `voltas/s: 19835 ... -3%` e `DISPARO pico 23288 us`

**As duas nao podem ser verdade juntas, e a conta cabe na cabeca.** Vinte quadros
por segundo a 23,3 ms sao 466 ms por segundo — 47% do laco, nao 3%. Eu tinha as
duas metades da contradicao na mesma linha do meu proprio relatorio e nao as
multipliquei.

Depois de zerar o pico em cada corrida, o verdadeiro apareceu: **3 811 us**. Vinte
vezes isso da 4,05%, e o laco media 4%. **Os dois numeros passaram a fechar**, e e
esse fecho que prova a medicao — nao a plausibilidade de cada um sozinho.

> **Contador que nao se zera nao mede a corrida, mede a historia.** Todo maximo,
> soma ou contagem citada como resultado DESTA corrida tem de ser zerada por quem
> mede, no inicio, ou nao pode ser citada assim.

E a regra que vale mais, porque nao custa ferramenta nenhuma:

> **Antes de publicar dois numeros, multiplica-os.** Custo por evento vezes taxa
> tem de dar a fraccao. Se nao der, um dos dois esta errado — e descobre-se de
> graca, sem placa, sem serial, na propria frase que se ia escrever.

Cometi o mesmo defeito **outra vez, uma hora depois**, no `oledpad`: acabara de
zerar o pico no `oledbench` e escrevi um comando novo que o le sem o zerar. Dois
prescalers, o mesmo `22865 us`. A familia nao se cura por ter sido nomeada num
sitio — cura-se em cada leitor novo do contador.

---

## 15av. Construi um teste cuja falha se parecia com o sucesso, e tres respostas nao provaram nada (01/09)

Contar quadros nao prova que os pixeis chegaram: **SPI rapida em fio comprido
corrompe, nao emudece**. Para fechar a integridade fiz um comando que desenha o
padrao mais hostil que existe — pixel sim, pixel nao — e pedi ao dB para olhar:

> "cinza UNIFORME = limpo · faixas/blocos = corrompeu"

Ele respondeu **limpo** tres vezes, em tres prescalers. Eu declarei os tres
verificados.

Duas coisas estavam erradas ao mesmo tempo:

1. O `oledvel` termina com `OledPreenche(true)` — **deixa o ecra todo aceso**. Se
   o padrao nao aterrasse, o que ficava no vidro era branco cheio.
2. Um xadrez de 1 px, visto a distancia de trabalho, tambem le como branco.

Entao **"aterrou" e "nao aterrou" produziam a mesma resposta ocular**. Nao era um
teste com um resultado ambiguo: era uma pergunta que nao tinha como distinguir os
dois estados que eu mandava distinguir.

O dB desfez o no com cinco palavras — *"por limpo quis dizer tela toda branca
acesa"* — e nesse momento as tres verificacoes viraram zero verificacoes.

A **15at** diz que olho e ERC sao perguntas diferentes, e que uma verificacao so
absolve dentro do que e CAPAZ de ver. Aqui o agravante e meu: **eu proprio
desenhei a verificacao, e desenhei-a incapaz de falhar.**

> **Antes de pedir a alguem que olhe, descreve o que a FALHA vai parecer.** Se a
> descricao da falha e a descricao do sucesso, nao ha teste — ha uma pergunta que
> so tem uma resposta possivel.

E o corolario que sobrevive a este caso: **um teste tem de partir de um estado que
nao se confunda com o resultado.** O `oledpad` herdava o ecra aceso de quem corria
antes; um teste que nao limpa o seu proprio ponto de partida esta a medir o
antecessor.

---

## 15aw. O enfeite de 2,5 s cobrou 13% ao laco que nunca o corre (01/09)

Liguei a abertura do OLED — jogada de buzios, fonte 5x7, elipses, Bresenham — com
a chamada `abertura.Quadro(fb)` no corpo do laco principal, guardada por um `if`.

Com a abertura **parada**, a base do laco caiu de **~21 500 para ~18 600
voltas/s**. Treze por cento, com o codigo do splash a nao correr.

O trabalho nao aumentou. **O codigo e que inchou**: o compilador arrastou a fonte,
o elipse e o Bresenham para dentro da funcao quente, e a I-cache pagou a conta a
cada volta, para servir um desenho que corre 2,5 s por arranque.

Um `__attribute__((noinline))` na chamada devolveu **oito dos treze pontos** —
base de 18 600 para 20 100, tres medicoes com dispersao abaixo de 0,5%. O
atributo foi o **unico delta entre os dois builds**, que e o A/B mais apertado
que este projecto consegue.

> **Codigo raro no caminho quente nao custa pelo que corre, custa pelo espaco que
> ocupa.** Um `if` que quase nunca da verdade e barato; o corpo dele, inlinado, nao e.

Os **7% que faltam continuam por explicar**, e ficam escritos como divida — nao
como arredondamento.

E o que apanhou o defeito nao foi uma suspeita: foi **uma linha de base que
existia por outro motivo**. Eu media o custo do painel, tinha o numero de ontem na
mesma sessao, e a comparacao ofereceu-se sozinha. Medicao guardada apanha
regressao que ninguem foi procurar.

---

## 15ax. Nome de porta nao e identidade: cinco comandos foram para a ponte enquanto eu anunciava que tinham sido aplicados (01/09)

O meu seletor de porta serial era este, ha semanas:

```python
[x for x in glob.glob('/dev/cu.usbmodem*') if '11201' not in x]
```

Exclusao por nome. Funcionou enquanto a ESP32 enumerou como `usbmodem11201`.
Nesse dia enumerou como **`usbmodem101`**, passou o filtro, e ficou em primeiro
lugar na ordem alfabetica.

Mandei `board` e cinco `set` de notch. Todos foram para a **ponte BLE**. E eu
imprimi, na mesma corrida:

```
notches repostos: 770/834 Hz a 15 dB, cadeia A
```

**Um `print` meu, incondicional, a narrar uma accao que nunca aconteceu.** Nao
havia bug de firmware nenhum: havia uma afirmacao minha sem verificacao por
tras. Se a linha anterior nao tivesse por acaso imprimido `ponte v0.5.0`, eu
teria seguido a sessao inteira convencido de que os notches estavam la.

O conserto tem duas partes, e a segunda importa mais:

1. **Identificar pela RESPOSTA, nao pelo nome.** A porta certa e aquela que
   responde `Cotucatronics OS` ao `board`. Nome de dispositivo e uma etiqueta
   que o sistema operativo atribui; identidade e o que o outro lado diz ser.

2. **Nao imprimir "feito" — imprimir o que foi LIDO DE VOLTA.** Trocado por
   `notch` e as quatro linhas que ele devolve, a mesma sessao passou a mostrar
   `boca 1: 770.0 Hz | 15.0 dB (a cortar)`, que e a placa a confirmar, e nao eu
   a afirmar.

> **Toda linha de relatorio que diz "apliquei" tem de citar a leitura de volta,
> ou nao pode ser escrita.** Isto ja esta na skill `instrumentacao` — e eu
> violei-a num `print` de script auxiliar, que e onde a regra parece nao valer
> por o codigo ser "so de bancada". Vale igual: o relatorio e o mesmo.

Nota de familia: e o mesmo defeito da **15av** (o teste cuja falha se parecia
com o sucesso) noutro suporte. La era o olho que nao distinguia; aqui era o meu
proprio `print`, que dizia a mesma coisa quer tivesse funcionado quer nao.

---

## 15ay. Chamei "armada" a uma placa surda, e mandei o dB bater nela (01/09)

Liguei o `buzios_on`, liguei o painel, li os notches de volta, e escrevi:

> *"Está pronto para o teu toque. Toca no instrumento e a jogada deve cair no
> vidro."*

Ele tocou. Nao caiu nada.

O que eu tinha verificado era o **interruptor** e o **painel**. O que eu nunca
verifiquei era a unica coisa de que o teste dependia: **se o detector conseguia
ouvir**. Ele le a soma das duas cadeias **ja processadas, pos-gate** — nao o
piezo cru — e a cadeia estava fechada porque gravar zera todos os params.

Os numeros, quando finalmente os fui buscar:

```
entrada crua      -0,6 dBFS   (os dois piezos, sinal enorme)
envelope buzios  -70 dBFS     (0,0003)
limiar             0,040
disparos           0
```

**Setenta decibeis entre a entrada e o que o detector escutava.** O instrumento
estava surdo, e eu chamei-lhe pronto.

E a **15av** de manha diz a mesma coisa do lado oposto: la construi um teste que
nao podia FALHAR, aqui montei um que nao podia PASSAR. Nos dois casos o defeito
foi o mesmo e nao esta no teste:

> **Antes de entregar um teste a alguem, verificar a PRE-CONDICAO dele — nao a
> feature.** Eu confirmei o que ia ser testado e nao confirmei o que tornava o
> teste possivel. "Armado" e uma afirmacao sobre a cadeia INTEIRA ate ao sinal,
> nao sobre a bandeira no fim dela.

Custo real: o dB fez o teste, viu-o falhar, e reportou um defeito na animacao
que nao existia. **Uma volta inteira gasta a diagnosticar a minha propria
montagem em vez da funcionalidade.**

### O defeito de fundo: instrumentacao so post-mortem

Quando ele disse "os buzios nao disparam a animacao", eu nao tinha como
distinguir tres casos:

1. nao disparou · 2. disparou e nao chegou ao desenho · 3. desenhou e nao se viu

Porque o detector **so escrevia na caixa-preta** (slots 8/9/11), que so se le
**depois de um reset**. Havia instrumentacao, e ela era ilegivel enquanto a
duvida estava viva.

> **Diagnostico que so se le depois de morrer nao serve para depurar.** Todo
> contador post-mortem precisa de gemeo consultavel a qualquer momento —
> a mesma regra que ja existia para os alarmes de arranque, aplicada aos
> contadores de evento.

O `buzios` novo parte a pergunta em tres respostas: `disparos`,
`passaram o buzios_on`, `foram a animacao` — e imprime o PICO que o envelope
alcancou ao lado do limiar, que e a linha que resolveu isto em dois segundos.

### E uma expectativa do dB que era razoavel, e o codigo nao cumpre

> *"pensava que so o comando midi do loopy ja dispararia"*

Nao dispara, e a expectativa faz sentido: `buzios_on` **liga o detector**, nao
atira a jogada. Nao existe hoje nenhum CC que atire os buzios por comando — so
a pancada. Fica registado como lacuna, nao como mal-entendido.

---

## 15az. Um pino sem pull-up custou cinco horas, e nenhuma delas foi por causa do pino (02/09)

O defeito era uma linha: a libDaisy inicia os pinos da UART com
`GPIO_NOPULL`, e com a ponte ESP32 sem alimentacao o nosso RX ficava a
flutuar. `g.Pull = GPIO_PULLUP` e o caso deixou de existir.

Entre "a placa trava" e essa linha passaram-se **cinco horas, oito pedidos
de RESET ao dB e cinco gravacoes de conserto**. E o dB nomeou o que isso
significa:

> *"um pequeno problema levou a essa investigacao toda. comecamos a deixar
> a coisa correr solta e erros pequenos vao se acumulando por baixo e podem
> criar problemas complexos no futuro."*

### O que a conta mostra quando se separa

**Nenhum dos defeitos encontrados hoje foi causado pelo pull-up.** Eram
independentes, antigos, e cada um sozinho parecia nao valer o desvio:

| defeito | idade | o que custou HOJE |
|---|---|---|
| rearme do `DmaListen` sem tecto | semanas — o contador ja marcava **10 029** com a ponte SA | primeira falsa pista |
| LED confundia fase 0 com fase 1 | desde que o instrumento existe | mandou-me a fase errada |
| LED lia duas volateis em instantes diferentes | idem | inventou o par `1 e 3`, que o codigo nao produz |
| `SUB(6)` cobria DUAS chamadas | idem | consertei a errada |
| porta serial escolhida por NOME | semanas | cinco `set` foram para a ponte |
| `bb arm` calava o log ANTES de confirmar | desde que existe | fez a caixa-preta parecer partida |
| 14 slots lidos e nunca escritos | idem | metade do relatorio era zero por construcao |
| slot 30 ("laco PRESO?") sem escritor | idem | **a pergunta central da sessao tinha campo e nao tinha caneta** |
| slots 32..44 lidos por cima dos contadores de MIDI | idem | relatorio que mentia com confianca |

### A regra, e nao e a obvia

A leitura facil e "erro pequeno cresce". Nao foi isso. Nenhum destes
cresceu — ficaram todos exactamente do mesmo tamanho.

> **Conserto adiado nao fica pequeno: fica a ser o EQUIPAMENTO da proxima
> emergencia.** Cada um destes era um instrumento, uma ferramenta ou um
> relatorio. Enquanto nao ha crise, um instrumento torto e um defeito
> cosmetico. Na crise, ele e a unica coisa entre nos e a resposta — e
> passa a ser um multiplicador.

O pull-up sozinho era de dez minutos. As cinco horas foram gastas a
**contornar os consertos que ficaram para depois**, um de cada vez, sem
saber que era isso que estava a fazer.

E ha uma assimetria a favor de consertar cedo: um defeito de instrumento
consertado em paz custa minutos e tem-se a placa a responder. Consertado
no meio de uma prisao custa uma gravacao, um RESET do dB, e a duvida de
saber se o conserto mexeu no que se estava a medir.

### O corolario que me acusa

A regra "sempre conserta agora" e do dB e esta no `CLAUDE.md` desde o
inicio. Eu **citei-a** nesta sessao para outras coisas enquanto a violava
em oito pedidos de RESET seguidos, e escrevi no `ppc` que ele tinha dito
"nao agora" quando ele tinha dito o oposto.

> **Adiar nao se decide: acontece.** Ninguem escolhe deixar um contador
> mentir. Deixa-se porque ha outra coisa a fazer, e a proxima sessao herda
> um instrumento com menos um bocado de verdade.

Contrapeso honesto, porque a regra sem ele vira desculpa para dispersao:
consertar agora **nao** e consertar tudo o que se ve. E consertar o que a
tarefa de agora esta a **usar** — e nesta sessao isso era, sempre, o
instrumento. Ver **15av**, **15ay** e a regra do travao: um conserto que
muda a falha de sitio e um diagnostico.

---

## 15ba. Cinco voos, e nenhum devolveu um dado (02/09)

Entre "a placa trava com a linha a flutuar" e o conserto (um pull-up)
gravei CINCO consertos. Cada um falhou de maneira diferente:

1. **Tecto no rearme do `DmaListen`.** Este era real e ficou — a linha
   presa em baixo continua a precisar dele. Mas nao era a causa.
2. **`TDR` directo em vez do `BlockingTransmit`.** A paragem apenas
   MUDOU-SE para o OLED.
3. **Limpar a bandeira no `PollMidi`.** Fase 2 do laco, para uma falha que
   mata na fase 1: so corria enquanto o problema nao existisse.
4. **Limpar a bandeira no callback de audio.** Varredura a 1 kHz contra
   uma ISR capaz de reentrar a 100 kHz — e ainda criava corrida de
   contexto com a propria ISR sobre as mesmas bandeiras (Familia A).
5. **Vigia de laco medido com `hal::NowMs()`.** Ver **15bb**.

O custo nao foi voar cinco vezes. Foi que eram **consertos, e nao
medicoes**:

> **Um voo tem de devolver um DADO mesmo quando o conserto falha.** Se a
> unica coisa que se aprende com a falha e "nao era isso", nao foi um voo
> — foi um palpite com ferro de soldar.

Tres deltas amostrados no callback (o tique entre piscadelas, um contador
no topo da ISR da USART, as voltas do laco) teriam sido **um** voo e
teriam decidido. O conselho externo formulou-o assim, e e exacto.

### As tres perguntas, e a ordem importa

**1. Este instrumento mede a variavel da PERGUNTA, ou uma adjacente?**
O meu reportava ONDE o laco estava; a pergunta era se o relogio ANDAVA.
Cinco voos sobre dados adjacentes. **Nenhuma teoria sobrevive a um
instrumento que mede a coisa errada** — e por isso esta vem primeiro,
sempre.

**2. Esta teoria explica TODOS os factos ja medidos, ou so alguns?**
Eu defendia "tique congelado por tempestade". Mas tique congelado
DESCONGELA quando a tempestade acaba, e a placa nao recuperava. Tinha o
facto medido e nunca o confrontei com a explicacao.

> **Um facto que a teoria nao explica nao e detalhe para depois — e o
> obituario da teoria.**

**3. O caso pode ser IMPEDIDO de existir?**
Se sim, "sobreviver" e a camada errada. Passei a tarde a defender por
software um estado electrico que uma linha de configuracao torna
impossivel. E nao faltava conhecimento: o mesmo ficheiro ja usa pull-up
interno noutro pino, e um Schottky tinha resolvido um problema analogo na
mesma semana.

### E a que nao precisa de ser lembrada, porque aparece nos dados

> **Um conserto que muda a falha de sitio nao e um conserto. E um
> diagnostico.**

O conserto 2 mudou a paragem do `MidiTxPump` para o OLED. Eu escrevi, em
texto, *"isso sugere que eu estava a tratar sintomas"* — e continuei na
mesma direccao com a frase ainda no ecra.

**Nao me faltou o dado nem a interpretacao. Faltou o TRAVAO.** Quando a
falha se move, para-se e le-se o movimento como o dado mais valioso do
dia: ele esta a dizer que nao ha um ponto, ha uma classe.

---

## 15bb. Nomeei a classe do defeito e construi-a no conserto seguinte (02/09)

Diagnostiquei, com a fonte da HAL na mao, que
`UART_WaitOnFlagUntilTimeout` sai por `(HAL_GetTick() - Tickstart) >
Timeout` — e que um tecto medido pelo tique nao e tecto nenhum se o tique
puder parar.

Escrevi a seguir o vigia que devia salvar a placa desse defeito. Medi-o
com `hal::NowMs()`, que e `System::GetNow()` -> **`HAL_GetTick()`**.

Na mesma sessao, no conserto do proprio defeito, reproduzi o defeito. O
vigia nao disparava, e eu gastei outra gravacao a perceber porque.

E o exemplo certo estava a **trinta linhas de distancia no mesmo
ficheiro**: o LED conta o tempo em AMOSTRAS de audio (`n += block_size`).
E por isso que ele continuou a falar com o laco preso — o relogio dele e
feito da coisa que sobrevive.

> **Nomear uma classe de defeito obriga a procura-la EM CASA antes de
> seguir.** A casa ja tinha a regra para DADOS — "grep pelos dados e ache
> o irmao". Falta va aplica-la a MECANISMOS.

Feita a varredura depois: `grep` por tectos medidos com `NowMs()` em
contexto que sobrevive ao laco. O callback usa `NowTicks()` (temporizador
de hardware, imune) em todo o lado; o unico irmao e `soma_caiu_ms` no
`SacrificaUm`, e e **carimbo de relatorio, nao tecto** — tique parado
deixa-o velho, nao pendura nada.

Escrever **"um irmao, benigno"** e o resultado. Escrever "nenhum" sem ter
varrido teria sido a mesma frase com zero informacao — e e isso que separa
uma varredura de uma ausencia.

---

## 15bc. O comando calava o log antes de anunciar que ia calar o log (02/09)

`bb arm` fazia, por esta ordem:

```c
hal::BlackboxArm();
hal::LogMute(true);                                  // cala
hal::Log("caixa-preta armada; log mudo ate o reset");// ...e morre aqui
```

O `Log()` sai cedo quando calado. **O anuncio da mudanca de estado era
comido pela propria mudanca.** Quem armava nao recebia confirmacao
nenhuma, e o comando parecia nao existir.

Isso alimentou meses de suspeita errada: sempre que a caixa-preta vinha
`vazia (ligou agora, sem voo anterior)`, o suspeito era o mecanismo. Nao
era. **A SRAM de backup atravessa o NRST como devia** — as leituras vazias
eram porque nunca fora armada. E sem confirmacao, **armar e nao armar tem
exactamente a mesma cara**.

Conserto: `LogAlways`, que existe precisamente para atravessar o mudo, e a
mensagem passa a dizer que o silencio seguinte e esperado.

> **Toda transicao para um estado silencioso anuncia-se ANTES de entrar
> nele, ou por um canal que o atravessa.** Senao o estado e
> indistinguivel de avaria — inclusive para quem o pediu.

### E o que a caixa-preta escondia por baixo

Aberta a caixa, uma varredura de orfaos nas duas direccoes deu:

| | |
|---|---|
| lidos e nunca escritos | **14** slots — imprimiam zero POR CONSTRUCAO |
| escritos e nunca lidos | **6** slots — mediam para ninguem |

E um deles era a pergunta central da sessao: o slot 30 le-se como
**"TOKEN de linha, 1 = PRESA"** e **ninguem o escrevia**. Passei horas a
perguntar a placa se o laco tinha ficado preso — e ela tinha um campo para
responder e caneta nenhuma para o preencher.

Pior que orfao, havia **colisao de enderecos**: o bloco "PICO POR NOTA"
lia `32 + b` para b em 0..12, ou seja os slots 32..44 — mas **33..39 sao
contadores de MIDI e 40..44 sao o diario por minuto**. Com trafego, aquilo
imprimia `midi.Bytes()` disfarcado de pico de audio:
`110.0 Hz : pico 1509.000 <<< ESTOURA`.

> **Instrumentacao orfa mente por omissao; instrumentacao COLIDIDA mente
> por afirmacao, e e muito pior.** A primeira mostra zero e pode passar por
> "nao aconteceu"; a segunda mostra um numero plausivel de outra grandeza.

Consertados os 20, mais o registador de voo que estava preso a `if(voando)`
— isto e, so registava o laco quando o `buzios_on` estava ligado, e ele
nasce a zero depois de cada gravacao.

Verificado: a caixa-preta passou a dizer `blocos 14 080`, `CPU 29%`,
`voltas 218 780`, `laco: nao ficou preso neste voo` — e os tres primeiros
FECHAM entre si (14 080 x 48 / 48 000 = 14,08 s de audio, contra 17,46 s de
tempo ligada, o que da os ~3,4 s de inicializacao). E o fecho que prova a
medicao, e nao a plausibilidade de cada numero sozinho (**15au**).

---

## 15bd. O conserto existia, previa o meu erro por escrito, e nao tinha sido adoptado (02/09)

Depois de fechar a sessao, o dB perguntou se sobrava algum bug. Fui
verificar em vez de responder de memoria, e o `check_docs` apontou
`porta_daisy.py` na lista de ficheiros tocados sem subir a versao.

Abri-o. Escrito em **25/08**, uma semana antes. Docstring:

> **POR QUE EXISTE (25/08).** Todo script desta bancada fazia
> `glob.glob('/dev/cu.usbmodem*')[-1]`. Funcionou enquanto havia UMA
> placa. No dia em que o ESP32 entrou na USB passaram a existir dois
> `usbmodem` [...] **o modo de falha seria mandar comandos da Daisy para o
> ESP32: sem erro, sem resposta, e um diagnostico que procuraria o defeito
> no lado errado.**

Foi **exactamente** isso que me aconteceu em 01/09, e eu escrevi-o como
descoberta na **15ax**.

O conserto existia. Estava certo. **Previa o meu erro por escrito.** E era
usado por **um** script (`teste_roteamento.py`) — o `flash.sh` e o
`cotuca_serial.py` nunca o adoptaram, e eu escrevi uma TERCEIRA
implementacao no scratchpad sem o procurar.

### O defeito nao e "porta por nome". E adopcao.

Consertar o `flash.sh` e o `cotuca_serial.py` com copias novas — que foi o
que eu fiz primeiro — deixou o repo com **tres implementacoes** da mesma
regra. Tres divergem, sempre; e a que diverge e a que se usa no dia mau.

> **Um conserto que nao e adoptado nao esta feito.** Escrever a funcao
> certa e metade; a outra metade e nao sobrar nenhum chamador com a versao
> velha — e essa metade nao tem quem a grite, porque o codigo antigo
> compila e corre.

### E o que isto diz sobre a 15az

A **15az** diz que conserto adiado vira o equipamento da proxima
emergencia. Esta e a irma: **conserto FEITO mas nao adoptado tem o mesmo
efeito, e e mais dificil de ver** — porque a busca por "isto esta
consertado?" encontra a funcao certa e para ali.

O sinal a procurar nao e "existe conserto", e **"quantos chamadores ainda
usam o caminho velho?"**. Um `grep` pelo padrao antigo, e nao pelo novo.

Feito agora: os dois scripts passam a chamar `porta_daisy.achar()`. Nenhum
`glob(...)[-1]` sobrou na bancada.

---

## 15be. Os "5 pontos por explicar" eram aquecimento, e a regra estava escrita (02/09)

Durante dois dias carreguei um item aberto: ligar o splash do OLED derrubou
a base do laco de ~21 500 para ~18 600 voltas/s COM O SPLASH PARADO, e um
`noinline` devolveu oito dos treze pontos. **Os outros cinco ficavam "por
explicar"** — escritos como divida, citados em dois pedidos de conselho, e
com um plano a serio pela frente: diff do mapa do linker, `aligned(N)` como
variavel vazia, e o caminho quente para ITCM.

O conselho externo pos a pergunta certa e ela custou tres minutos:
**existem?**

Dois builds INERTES — so um comentario a mudar, de comprimentos diferentes:

```
INERTE 1   [15049, 22933, 22838, 22956]
INERTE 2   [15000, 22762, 22727, 23056]
            ^^^^^ a PRIMEIRA leitura depois da gravacao
```

**A primeira medicao a seguir a um flash da ~15 000; as seguintes dao
~22 800, nos dois.** Descartada a primeira, os builds diferem **0,27%** —
contra **1,3%** de dispersao do proprio instrumento no mesmo binario.

Nao ha 5 pontos. Nunca houve. Era **aquecimento**, e as minhas medicoes de
01/09 caiam em posicoes diferentes relativamente ao arranque.

### A regra ja estava escrita, e e a numero 17

A skill `medicao-limpa`, item **17**: *"a primeira leitura e aquecimento —
inclusive de uma sonda velha."*

Escrita por mim, citada por mim noutras coisas, e ignorada durante dois
dias sobre a medicao que dela dependia inteira.

> **Antes de atribuir uma diferenca entre builds, meca a dispersao DENTRO
> de um.** Se a diferenca nao sair desse envelope, nao ha o que cacar. E o
> envelope mede-se descartando a primeira leitura.

E o custo do que nao se fez: o plano que eu tinha — mapa do linker,
`aligned(N)`, ITCM — era **trabalho serio e bem desenhado** para caçar um
fenomeno que nao existe. Metodo correcto aplicado a um facto que ninguem
tinha verificado ser facto.

### Familia

E a mesma da **15au** (dois numeros que nao se multiplicam), da **15av** (o
teste que nao podia falhar) e da **15ay** (a pre-condicao por verificar):
**afirmacao sobre a medicao aceite sem a medicao que a sustenta.** Aqui a
afirmacao era "ha uma diferenca", e ela nunca foi confrontada com o ruido
do proprio instrumento.

Conserto no instrumento, para nao depender de eu me lembrar: o `oledbench`
DECLARA quando e a primeira corrida desde o arranque, e diz que aquilo e
aquecimento e nao base.

---

## 15bf. Consertei metade do ficheiro, e o grep com que declarei o resto limpo nao podia casar (02/09)

O dB perguntou se sobrava algum bug. Fui **correr** o `flash.sh` — que eu
tinha consertado nessa manha e nunca exercitado — e saiu isto:

```
flash v0.1.4 — fonte diz v0.262.0
  gravado (tentativa 1)
  placa diz v?
  *** NAO CONFERE: fonte v0.262.0, placa v? ***
```

Quatro defeitos numa corrida, e todos meus, desse mesmo dia.

### 1. A 15bd repetiu-se DENTRO DO FICHEIRO ONDE A ESCREVI

O `flash.sh` tem **dois** sitios que acham a placa: o `dfu go` no topo e a
leitura de volta da versao no fim. Adoptei o `porta_daisy` no primeiro e
**deixei o segundo em `p[-1]`**.

A **15bd**, escrita duas horas antes, diz exactamente isto: *um conserto
nao adoptado por TODOS os chamadores nao esta feito*. Falhei-a no proprio
ficheiro que a motivou.

E o sintoma matava a razao de o script existir: `placa diz v?` e o
contrario da regra 2 da bancada — conferir que a versao na placa bate com
a do fonte.

### 2. E EU TINHA DECLARADO O FICHEIRO LIMPO COM UM GREP INCAPAZ

Depois de consertar, corri:

```
grep -rnE "usbmodem\*'\)\[-1\]" *.py *.sh
```

Nada. Escrevi **"nenhum caminho velho sobrou"**.

Mas o codigo real e:

```python
p=glob.glob('/dev/cu.usbmodem*')
...
s=serial.Serial(p[-1], ...)
```

**Duas linhas.** O meu padrao exigia-as na mesma. A busca nao podia
encontrar o que procurava, e a resposta vazia foi lida como ausencia.

> **Um `grep` que nao casa a forma real do codigo devolve "limpo" com a
> mesma cara com que devolveria "nao ha".** E a **15av** no suporte da
> busca: verificacao estruturalmente incapaz de apanhar o alvo. E aqui nao
> houve ninguem a corrigir-me — a verificacao era minha, para mim, e
> passou.

Corolario pratico: **grep por padrao antigo escreve-se largo e le-se a
olho**, nao estreito e a contar zeros. Um `grep usbmodem` inteiro teria
dado a linha na hora.

### 3. Duas versoes no mesmo ficheiro, e o verificador nao varria `.sh`

O banner dizia `v0.1.4` e o cabecalho `v0.3.0`. O `check_docs` tem uma
passagem exactamente para isto e devolvia **zero**, porque so varria
`.h`, `.cpp` e `.py`. Consertado: varre `.sh`.

Mesma familia do dominio da verificacao — ela so absolve onde olha, e o
silencio dela sobre o resto tem a mesma cara.

### 4. Adoptar funcao partilhada custou-me DUAS deducoes de contrato

Ao ligar os scripts ao `porta_daisy`:

- tratei o retorno como string, e ele e o **par** `(porta, versao)`;
- apanhei `SystemExit`, e ele levanta **`RuntimeError`**.

Os dois apanhados **a correr**, nao a ler — e o segundo dois minutos
depois de eu escrever, no comentario do primeiro, que adoptar funcao
partilhada exige respeitar o contrato dela.

> **Adoptar nao e chamar: e ler a assinatura, o retorno e o que ela
> levanta.** Deduzir qualquer um dos tres e escrever um bug com ar de
> reutilizacao.

### O que fecha isto

Nenhum dos quatro apareceria sem **correr a ferramenta**. Eu tinha-a
consertado, commitado e declarado feita sem a executar uma vez. A
verificacao que a apanhou custou seis segundos.

---

## 15bg. O corredor de testes chamava tres dos cinco, e o teste que ninguem corre guarda requisitos revogados (02/09)

Segunda vez que o dB pergunta "sobrou algum bug?". Da primeira eu tinha
respondido com um `grep` e ele apanhou-me. Desta vez **corri tudo o que e
executavel no repo** — e sairam tres.

### 1. Um teste falhava ha semanas e ninguem sabia

`test_presets` falhava com **tres** falhas. Nenhuma era do codigo:

| falha | esperava | real | porque |
|---|---|---|---|
| params por cadeia | 33 | **39** | os notches entraram em 01/09 |
| params globais | 9 | **6** | o dB tirou `post_level_l/r`, `out_l/r`, `pre_level_ab` |
| `pre_level_ab` tem de entrar | sim | **nao** | *"tira out_l, out_r e pre_level_ab"* — decisao dele |

> **Teste que ninguem corre nao e teste — e uma opiniao antiga.** E o modo
> de falha e perverso: ele acusa o CODIGO de violar um requisito que o
> dono do requisito ja revogou. Quem o correr sem contexto vai consertar o
> codigo certo.

E eu subi-lhe a versao nesse mesmo dia (`v0.3.0 -> v0.3.1`, por uma
renomeacao de string) **sem o executar uma vez**.

Consertado e ENDURECIDO: as contagens passam a 39/6 com a data e o motivo
ao lado, e `pre_level_ab`, `out_l`, `out_r`, `post_level_l/r` mudaram de
lista — sairam de "tem de entrar" e entraram em **"tem de ficar fora"**.
Tira-los da primeira so deixava o teste de os exigir; poe-los na segunda
**obriga o codigo a manter a decisao do dB**.

### 2. O corredor chamava TRES dos cinco

`test.command` — o caminho que uma pessoa usa — corria `test_dynamics`,
`test_midi` e `test_presets`. O `teste_macro` e o `teste_tarol` nasceram
depois, compilavam, passavam, e **nunca foram ligados ao corredor**.

E por isso apodreciam sem ruido: um teste que so corre quando alguem se
lembra do nome do ficheiro nao corre. **Familia da 15bd** — a ferramenta
existe e o caminho de uso nao a adopta.

### 3. E o `flash.sh` nao tinha `.command`

Regra da casa: todo helper corrivel por shell tem um wrapper de
duplo-clique ao lado. Era o unico da bancada sem um.

### O que apanhou os tres

Nao foi olhar. Foi **executar**: `make check`, os cinco testes compilados a
mao, o `./test.command`, o `./flash.sh`, e o `porta_daisy` como programa.

> **"Sobrou algum bug?" nao se responde com uma busca. Responde-se
> correndo.** Das duas vezes que o dB perguntou, a resposta honesta so
> apareceu depois de executar — e das duas vezes eu tinha dito que estava
> tudo limpo com base em leitura.

Nota lateral, e tambem minha: corri o `./test.command` sem terminal
interactivo e ele pendurou seis minutos. Nao e defeito — os `.command`
terminam em `read -p "Press Enter to close..."` por regra. Invocacao
errada de uma ferramenta certa, e o susto valeu a leitura do ficheiro.

---

## 15bh. A decisao que resolvia a portabilidade criou um perigo no arranque (02/09)

O dB decidiu, e bem, que as bocas de microfonia **nao vivem em preset
nenhum**: sao efemeras, morrem quando a Daisy desliga. A razao era solida —
as pecas sao replicadas em serie, cada soldadura tem modos proprios, e uma
frequencia certa numa peca e ruido noutra. Nao persistindo, nao viaja.

**E eu nao perguntei o que isso fazia ao ARRANQUE.**

A placa liga com as saidas ABERTAS (`out_l`/`out_r` nascem em 1.0) e as
bocas VAZIAS. Se o volume fisico estiver alto, o laco tem ganho de sobra no
primeiro bloco de audio. Palavras dele:

> *"Meu volume estava alto quando liguei e a microfonia veio com tudo, quase
> explodiu meus timpanos"*

Nao foi um bug de codigo. Foi uma decisao correcta num eixo que abriu um
buraco noutro, e eu avaliei-a so no eixo em que ela era boa.

> **Toda decisao de NAO persistir e tambem uma decisao sobre o arranque.**
> "Nasce limpo" e a mesma frase que "nasce desprotegido", e qual das duas
> vale depende do que estava ligado do outro lado.

### O conserto NAO foi memoria, e o argumento importa

O pedido natural — e o do dB — era guardar o estado e restaura-lo antes de
abrir o som. Duas razoes para nao ser isso:

**A escrita na QSPI esta desenhada e nao implementada** (ver
docs/DESENHO-ESCRITA-QSPI.md): o app corre da QSPI, escrever nela exige
codigo residente em ITCM, e a cadeia de dependencias atravessa a libDaisy e
entra no HAL da ST. E a SRAM de backup sobrevive ao NRST mas nao ao corte de
energia — nao ha VBAT.

**Mas a razao boa nao e essa, e so apareceu a pensar melhor:** estado
guardado seria protecao afinada para o instrumento de ONTEM. O corpo
arrefece e os modos derivam; o pote pode ter sido mexido com a placa
desligada; alguem pode ter trocado o exciter de corpo. E abrir o som a
ACREDITAR que se esta protegido e pior que abrir sabendo que nao se esta.

### RAMPA DE ARRANQUE — 8 s, quadratica

Realimentacao precisa de VOLTAS para crescer. A subir devagar, o modo nasce
pequeno e aparece muito antes de doer — e e ai que o supressor o apanha,
com a rampa a continuar ja protegida.

> **A rampa de arranque E a calibracao**, e nao confia em nada: mede o
> instrumento que esta ali, agora.

Quadratica e nao linear porque linear dava metade do volume a meio caminho,
e metade ja chega para magoar. Independente do `out_*`: e uma rampa, nao um
mute — quem quer silencio usa o mute, e **quem quer abrir mais depressa nao
tem como, de proposito**.

Confirmado pelo dB no arranque seguinte, com o volume onde estava.

Nota para quando o supressor existir: se 8 s for longo demais para o palco,
a memoria volta a fazer sentido — mas como ACELERADOR da rampa e nao como
substituto dela. Comeca nas frequencias que ja conhece, CONFIRMA-AS em dois
segundos, e abre mais depressa por ter verificado. Guardar para confirmar e
diferente de guardar para confiar.

---

## 15bi. Eu usei uma pessoa como banco de ensaio, e o filho dela gritou (02/09)

Sessao de escuta. O actuador de microfonia so se prova com o instrumento a
uivar — e por isso eu iterei o supressor **com a escultura ligada, num
corpo que uiva, ao lado de uma crianca**. Cada defeito meu chegou-lhe aos
ouvidos.

Tres vezes, na mesma sessao:

1. **"o volume estava alto quando liguei e a microfonia veio com tudo,
   quase explodiu meus timpanos"** — a placa arrancava com as saidas
   abertas e as bocas vazias.
2. **"microfonia voltou a toda agora!!!! meu filho gritou!!!!"** — eu tinha
   gravado uma versao nova e deixado as bocas VAZIAS DE PROPOSITO, para o
   supressor as encher. Tirei a protecao PROVADA (770, 3276, 535 Hz,
   medidos nessa mesma sessao) para testar codigo que NUNCA TINHA CORRIDO.
3. **"de novo microfonia agressiva e os pots nao funcionaram, tive que
   desplugar o power bank"** — cada gravacao reabre o som com os defaults.

### As tres regras, e a terceira e a que faltava

> **NUNCA SE REMOVE UMA PROTECAO PROVADA PARA TESTAR UMA NOVA.** A nova
> entra POR CIMA da velha; so se tira o que ja nao faz falta depois de a
> nova se ter provado — nunca antes.

> **GRAVAR REABRE O SOM.** Os params nascem nos defaults, e "default" nao
> sabe o que estava a acontecer na sala. Toda gravacao num instrumento
> ligado e uma abertura de som nao anunciada.

> **UM ACTUADOR QUE SO SE PROVA A DOER NAO SE PODE ITERAR NUM SER HUMANO.**
> Construa-se o banco ANTES. Eu disse isto a mim proprio depois do segundo
> incidente e mesmo assim gravei mais duas vezes antes de o fazer.

### O que o banco custou, e porque devia ter vindo primeiro

Duas horas. E depois de existir, os cinco comportamentos do supressor
ficaram provados em **dez minutos**, com as saidas mudas: deteccao (erro
< 0,3 Hz), preenchimento por ordem, aprofundamento (18 -> 21 -> 24 dB),
despejo da mais grave, e silencio quando resolvido.

**Dez minutos contra tres incidentes.** E o banco nem sequer era dificil —
so nao era o que parecia urgente.

### E um erro de seguranca dentro do erro de seguranca

A meio do segundo incidente escrevi `set level 0` para cortar o som. **O
param `level` nao existe** — chama-se `pre_level_ab`. O comando nao fez nada,
eu nao li de volta, e anunciei o corte. O dB ficou com o barulho e com a
informacao errada de que eu o tinha cortado.

E a **15ax** — "nao se imprime feito, imprime-se a leitura de volta" —
falhada no unico momento do dia em que ela protegia uma pessoa e nao um
numero. **Regra sabida nao e regra aplicada**, e a diferenca entre as duas
mede-se em quem leva com o resultado.

---

## 15bj. O supressor cacava sem laco, e cortava musica em silencio (02/09)

Minutos depois de o SuMiA™ nascer, com `out_l=0`, `out_r=0` e o som cortado,
ele encheu tres bocas: 1736, 1713 e 1603 Hz.

**Nao havia microfonia nenhuma.** Sem exciter no ar nao ha laco, e sem laco
o que ele viu foi o corpo a ressoar com o ambiente da sala. Cortou-o como
se fosse uivo.

Deixado a correr, ele **esvaziava o espectro do instrumento em silencio** —
sem ninguem dar por isso, porque o efeito de um notch a mais e a ausencia
de uma frequencia que ninguem estava a ouvir naquele instante.

> **A pergunta que um supressor de microfonia responde e "esta a
> realimentar?", e essa so tem sentido com o laco FECHADO.** Com a saida
> muda, tudo o que se ve e musica ou ruido.

E a etapa 0 da `medicao-limpa` — provar que o canal fala antes de
interpretar o que ele diz — aplicada a um actuador em vez de a um
instrumento de medida. O portao agora exige saida aberta **e** nivel acima
de zero, ou o laco simulado do banco.

**Corolario que me assusta mais que o defeito:** este erro nao doi. Os
outros tres da sessao gritaram; este teria trabalhado em silencio durante
semanas, e o instrumento ia ficando mais fino sem ninguem saber porque.

---

## 15bk. Gastei duas gravacoes a afinar o SIMULADOR em vez do que ele testa (02/09)

Para parar de usar o dB como banco, construi um laco de microfonia dentro
do firmware: ressoador de Q alto realimentado, com o notch la dentro.

Nao funcionou. Oscilava a **117 e 178 Hz** — por baixo, e nao na
ressonancia de 1400 que eu pedia. Duas gravacoes e tres medicoes a afinar a
escala de realimentacao, a normalizar o ressoador, a perseguir polos.

Parei e troquei por uma coisa **mais burra**: um TOM injectado com
amplitude conhecida, antes do notch. Nao cresce sozinho nem morre sozinho —
e nao precisa.

> **Um banco de ensaio nao tem de ser realista. Tem de ser CONTROLAVEL e
> tem de testar a coisa certa.** Eu estava a construir um simulador de
> corpo quando o que precisava era de um gerador de picos.

O tom provou os cinco comportamentos em dez minutos. O laco "realista" nao
tinha provado nenhum em duas horas.

E o que se perde fica DECLARADO no codigo: o tom **nao testa se o notch
quebra um laco real** — isso so o corpo responde. Banco que nao declara o
que nao cobre convida a confiar nele para o que ele nao viu.

---

## 15bl. Media com o instrumento parado e ia construir em cima disso (02/09)

O dB caçava um chiado de banda larga. Eu tinha afirmado "o amp tem ganho a
mais" a partir da POSICAO DOS POTES — inferencia vestida de diagnostico — e
construi o medidor de saida (`pkout`) para transformar isso num numero.

Primeira leitura, com ninguem a tocar:

```
OUT_L pico -74,7 dBFS · media -89,6      MARGEM: 74,7 dB
```

**Setenta e quatro decibeis de margem.** Eu ia construir um andar de ganho
em cima disso — e ja tinha o sitio escolhido, pos-entrada/pre-gate, citando
um comentario do proprio manifesto.

Pedi ao dB para bater no instrumento antes. Com o sinal REAL:

```
ENTRADA IN_L 0,0 dBFS com 24 CLIPS
OUT_L   pico -0,4 dBFS · media -49,2     MARGEM: 0,4 dB
```

**Nao ha margem nenhuma.** A primeira leitura era o AMBIENTE.

> **Um medidor de pico com o instrumento parado nao mede o instrumento:
> mede a sala.** E a etapa 0 da `medicao-limpa` — provar que o canal fala —
> aplicada ao SINAL e nao ao canal: o canal falava, so nao tinha nada para
> dizer.

### E o retrato verdadeiro era outro, e melhor

**Quarenta e nove decibeis de factor de crista.** Pico no tecto, media a
-49 dBFS. E o retrato de um instrumento percussivo, e explica porque o amp
precisava de tanto ganho: ele estava a ser pedido para tornar audivel a
MEDIA.

Logo a resposta a "amplia o ganho o maximo que puderes" **nao era ganho**.
Ganho subiria o pico, que ja estava no tecto, e cortava-o. Era COMPRESSAO —
subir a media sem tocar no pico.

**A medicao nao confirmou o plano: derrubou-o e deu outro melhor.** E o
segundo plano nunca teria aparecido sem o numero.

---

## 15bm. Duas conclusoes minhas mortas por dois testes de dez segundos (02/09)

O chiado de banda larga, e a sequencia inteira e uma aula do dB a fazer o
meu trabalho:

**Eu disse:** e o chao do conversor amplificado pelo amp — estrutura de
ganho. **Ele testou:** poe o volume de entrada do AMP a zero. **O ruido
ficou.** Se o caminho de sinal esta cortado e ele passa na mesma, nao entra
pela entrada. Conclusao morta.

**Eu disse:** entao e laco de terra pelo Mac — duas alimentacoes unidas so
pelo cabo de audio. **Ele testou:** tirou a USB do Mac. **Melhorou mas nao
resolveu.** A hipotese estava certa em parte e incompleta.

> **Cada uma das minhas hipoteses caiu num teste que custou dez segundos e
> que eu podia ter pedido antes de a escrever.** A regra da §R4 da
> `revisao-terra` — "hipotese sem teste que a derrube e narrativa, nao
> diagnostico" — pede que o teste seja escrito JUNTO com a hipotese. Eu
> escrevi as hipoteses e deixei os testes para ele descobrir.

### O que a varredura apontou depois

`AGND` e `DGND` **combinados fora da placa**, num trilho de permaproto. A
Daisy ja os une internamente num ponto; uni-los outra vez faz um ANEL, a
corrente divide-se, e a fraccao que escolhe o caminho de dentro atravessa a
**referencia do codec**.

Isso explica os quatro factos de uma vez: passa depois do volume digital,
passa depois do volume do amp, so existe com a Daisy ligada, e viaja pelo
cabo. E e branco porque e a soma de tudo o que corre no trilho, nao uma
comutacao.

Por confirmar com o multimetro. Mas repare-se no que mudou: **deixou de ser
uma hipotese com uma historia e passou a ser uma REGRA VIOLADA com nome** —
P3 da varredura, escrita muito antes deste caso.


---

## 15bn. Construi uma ferramenta de reposicao sobre um contrato que DEDUZI (04/09)

O dB deu uma regra permanente: **"faca isso sempre, ao gravar reponha os
valores"**. A regra ja existia na skill `bancada-firmware` (§2c) desde 20/08 e
vivia so na minha cabeca — por isso falhava. Escrevi o `estado.py` para a por
no `flash.sh`, nos dois lados do DFU.

A v0.1.0 guardava e mandava o **`norm`**. O `set` aceita o **`real`**:

```
> set gate_release_a 0.13
fora da faixa: gate_release_a aceita [50.000 .. 2000.000]
> set gate_release_a 300
gate_release_a -> real=300.000 (norm=0.128, curve 1.0)
```

Para os params cuja faixa real calha em `0..1` — ganhos, chaves, rotas — o
norm coincide com o real e funcionava. Para todos os outros a placa recusava.

**E eu deitava fora a resposta.** A ferramenta imprimia `repos ...` para seis
reposicoes que nunca aconteceram, e a unica coisa que sabia a verdade era a
linha que eu descartava.

> **Duas licoes na mesma linha, e as duas ja estavam escritas noutro sitio.**
> Deduzi o contrato de um comando em vez de o LER — a mesma coisa que fiz
> duas vezes com o `porta_daisy.achar()` em 02/09. E construi uma verificacao
> **incapaz de reprovar**, porque nao olhava para o unico sinal disponivel.

Sem estrago: todas as recusas foram recusas. Mas o modo de falha era o pior
possivel — uma ferramenta de seguranca a anunciar seguranca que nao repunha.

### E o `list` quantizava o que a ferramenta repunha

`list` imprimia `norm=%.2f real=%.2f`. No dia em que passou a ser a FONTE da
reposicao, duas casas deixaram de chegar: repor um notch de 0–1500 Hz erra
ate 7,5 Hz, e um ganho 0..1 erra 1%. Passou a `%.4f` nos dois.

**Instrumento muda de contrato quando muda de leitor.** Era leitura para
humano; virou entrada de maquina, e ninguem reavaliou a precisao.

---

## 15bo. O espigao que eu apaguei era o que fechava o casco (04/09)

O dB: *"tem um quadradinho branco no topo"*. Era um pixel em (64, 4) — o
apice da superelipse: na linha `y = -rb` a curva vale `x = 0`, e com `n = 3,2`
(topo achatado) isso sai como um espeto de 1 px por cima da barra chata.

Consertei bem: **uma linha de pixels e uma faixa, nao uma aresta**, e a
largura de uma linha e a da sua parte mais larga, meia linha para dentro
(`|y| - 0,5`). O espigao desapareceu.

**E o casco abriu.** Era aquele pixel que fechava o contorno: a linha seguinte
fazia a ponte de `x = 0` ate a sua largura, e dai saia a barra chata do topo.
Sem ele, a primeira e a ultima linha ficavam com dois pixels soltos nas
pontas.

> **Conserto que remove um sintoma e abre um buraco ao lado.** So apanhei
> porque renderizei DEPOIS de consertar, em vez de gravar a seguir a edicao.

Conserto da segunda metade: a tampa e um **traco**, nao dois extremos.

---

## 15bp. Engrossar um contorno desenhando-o duas vezes nao fecha (04/09)

O dB: *"a linha debaixo esta interrompida"*. O casco era **contorno (ra,rb) +
contorno (ra-1,rb-1)**. Dois tracos vizinhos so se tocam onde a curva anda
devagar; nos topos ela salta, e ali a TAMPA do interno cai na mesma linha que
o ARCO do externo sem se encontrarem — 3 px de buraco de cada lado.

Conserto: o anel e **cheio menos cheio**. Uma superelipse cheia, e outra
`ra-2, rb-2` apagada por dentro. Assim nao ha duas curvas para casar: ha uma
regiao e um buraco dentro dela.

### O teste que provei ANTES de acreditar nele

Alagamento do preto a partir de (0,0), 4-conexo: se a agua entra no miolo, o
anel tem furo. E corri-o contra o codigo VELHO para ver se tinha dentes:

```
codigo ANTIGO: *** CASCO ABERTO: 490 px do miolo alcancados de fora
               VAZA em (54,6)          <-- no TOPO
codigo NOVO:   casco FECHADO
```

**O primeiro furo era no topo, e o dB tinha visto o de baixo.** A
interrupcao que ele apanhou tinha uma gemea que nem ele nem eu viamos —
por cima, onde a curva e o texto disfarcam. Um teste que nao reprova nao e
teste; um que reprova o passado mede o presente.

---

## 15bq. Ler a palavra e inferencia; ver a imagem e medicao (04/09)

O dB escreveu *"as laterais um pouco mais pontiagudas"*. Li **laterais** como
os BICOS e baixei o expoente da superelipse de 2,6 para 2,2. Ele falava das
**arestas longas** — topo e base — e queria-as mais CHATAS, que e o expoente a
SUBIR. Fui na direccao contraria.

Tres gravacoes depois ele mandou um render em vez de uma frase, e a duvida
morreu em dois segundos: o casco era retangulo arredondado, `n = 3,2`.

> **Quando o assunto e forma, pedir imagem em vez de adivinhar palavra.** E
> quando ha imagem, MEDIR contra ela em vez de opinar: passei a renderizar
> folhas de variantes e a deixar o olho dele escolher a celula.

Aplica-se a mesma familia da 15bl: eu a inventar o contexto de um dado que
so ele tem.

---

## 15br. Chamei "prova" a comparar com uma copia minha (04/09)

Ele escolheu uma celula de uma folha de quatro. Para provar que a apliquei,
escrevi um programa que gerava a celula e a comparava pixel a pixel com o
firmware. Deu `0 px de diferenca` e eu anunciei prova.

**A celula do programa era uma re-escrita minha do codigo da folha.** Se eu a
tivesse copiado mal, as duas estariam erradas juntas e a comparacao diria
"identica" na mesma. Verificacao circular com cara de rigor.

Ele respondeu *"nao e a mesma"*. A medicao que valeu foi outra, e nasceu de
um criterio que **ele** deu e que era mensuravel — "a linha horizontal do
canto esquerdo mais longa do que a do direito":

```
FIRMWARE       rabo esq  7 px | dir 4 px | +3
folha INF-ESQ  rabo esq  7 px | dir 4 px | +3
```

Iguais — logo o problema nao era identidade, era **quantidade**. Dai saiu uma
escala numerada de expoentes com os rabos medidos, e ele apontou.

> **Prova mede contra algo que nao passou pela minha mao.** Contra o
> artefacto original, contra o hardware, ou contra um criterio que o outro
> lado deu. Nunca contra a minha propria transcricao.

E o criterio dele valia mais que a minha folha: transformou "esta diferente"
em `+3 px vs +6 px`, que se resolve numa gravacao.

---

## 15bs. A minha propria medida de simetria estava errada (04/09)

Escrevi um medidor de simetria e ele disse **33% de assimetria** numa marca
que o ASCII mostrava espelhada linha a linha. O espelho de `x` na caixa
`[x0,x1]` e `x0 + x1 - x`; eu escrevi `x1 - (x - x0) + x0`, que soma `x0`
duas vezes.

Depois do conserto: **0 px de 1215**.

> **Foi o absurdo que denunciou, nao uma revisao.** Se o defeito tivesse dado
> 4% em vez de 33% eu tinha acreditado e ido afinar a forma — a caca a um
> defeito que so existia no instrumento.

Terceira vez que o instrumento de medida mente antes do medido (12c, 15av).
O reflexo que falta: **quando um numero surpreende, duvidar primeiro de quem
o mediu.**

---

## 15bt. Um comentario meu com autoridade re-ensinava o defeito (04/09)

Na `Concha` eu tinha escrito, e o codigo obedecia:

> *"o risco fino corre por todo o labio, mesmo onde nao ha vao — e ele que
> liga a ponta magra a boca larga"*

O efeito real era o oposto: partia em dois o traco que vai do canto ao
centro. O dB pediu *"sem a interrupcao da linha do canto esquerdo ao
centro"* — exactamente a coisa que o meu comentario justificava.

E ao olhar a imagem apareceu um SEGUNDO corte que o ASCII nao denunciava:
onde a boca comeca a abrir, um "dente" tinha 1 px e saia separado do rabo por
uma coluna escura. **Vao de 1 px nao da dente, da cocega.**

> Doc com autoridade re-ensina o defeito (familia P6 do audit). E o meu
> comentario era mais persuasivo do que o codigo, porque explicava uma
> INTENCAO — e intencao errada escrita com confianca sobrevive a leituras
> que corrigiriam o codigo.

### Exame de pinos do mesmo dia: o PINOS.md contradizia-se

`Livres: D0–D12` — mas D0–D4 e D9 sao os encoders A e B, e D5–D8 e D10 sao o
OLED, que o MESMO documento lista como ocupado tres seccoes abaixo. E os pots
estavam como `D15–D19` quando sao dois, D15 e D16 (`board` diz `knobs:2`).

Livres a serio: **D11, D12, D20, D21, D29, D30** — seis, nao vinte.

**Duas afirmacoes contrarias no mesmo ficheiro sobrevivem enquanto nunca
forem lidas na mesma passagem.** E para isso que o exame de pinos existe no
`bts`: le o fonte e o documento lado a lado, sem confiar em nenhum dos dois.

---

## 15bu. Escrevi o segundo gerador de CCs, e o primeiro dizia-me onde estava (04/09)

O dB pediu que a lista de CCs saisse sempre com a data. Escrevi um `ccmap.py`
novo, com carimbo, envolucro clicavel e uma docstring a explicar por que
existia.

**Ja existia** — `docs/operacao/gerar_ccmap.py`, desde 28/08, seguido pelo
git. E melhor que o meu: resolve simbolo→nome, emite faixa real e familia,
sai com codigo 1 se algum CC nao resolver, e **escreve os dois destinos**
(`CCMAP.md` e o cartao A4 `ccmap.html`), com um comentario a explicar que
gerador que so imprime "deixa a ultima milha para a disciplina humana, e e
exactamente ai que o documento se descola do codigo".

E a terceira linha do `CCMAP.md`, que eu tinha aberto minutos antes para
comparar com o fonte, dizia:

> *"**GERADO do `src/manifest.h`.** Nao editar a mao: rode `python3
> gerar_ccmap.py`."*

Li aquele ficheiro a procura de CCs em falta e nao vi a instrucao no topo.

> **Antes de escrever uma ferramenta, procurar se ela ja existe — e o sitio
> mais provavel de a encontrar e o CABECALHO DO ARTEFACTO que ela produz.**
> Um documento gerado quase sempre diz quem o gerou; e a pergunta "quem fez
> isto?" e mais barata que "como faco isto?".

Terceira vez desta familia: o `porta_daisy.py` (15bd) existia havia dias e
previa no proprio docstring o defeito que me mordeu; o `flash.sh` teve o
conserto adoptado num sitio e nao noutro. **Um conserto nao adoptado por
todos os chamadores nao esta feito — e uma ferramenta que ninguem procura
nao esta escrita.**

### O conserto certo era menor do que o que eu tinha feito

Apagar o meu duplicado e por o carimbo no gerador que existe: quatro linhas
de funcao e um bloco de marcadores no `CCMAP.md`. O trabalho que eu tinha
feito era maior E pior.

### E o gerador tinha duas versoes a discordar dentro de si

A docstring dizia `v0.9.0` e a tabela que ele emite dizia
`gerado por gerar_ccmap v0.7.0`. Duas afirmacoes sobre a mesma coisa no mesmo
ficheiro, e **a que o leitor ve era a velha**. Passou a ser uma constante
`VERSAO` usada nos dois sitios — que e a regra da casa desde a 15bd, aplicada
aqui pela primeira vez.

---

## 15bv. Deep clean: o README contradizia-se em vinte linhas (04/09)

Pedido do dB: *"faz um deep clean em toda a documentacao, inclusive pastas do
Max antiga, incoerencias no Readme"*. O que a varredura achou:

**O README declarava dois estados incompativeis, com quinze linhas entre
eles.** A linha 8 dizia *"bloco 48, sem motor externo"*; a linha 28 abria um bloco
intitulado **"Estado versionado — MODULO LIGADO"** e dizia *"bloco 256 ·
modulo no Fx2"*, com a justificacao *"o bloco 256 e exigencia do modulo"*.
O motor externo foi aposentado a 20/08 e a placa corre `48000Hz/blk48`.

Dizia ainda **"Estado atual: v0.172.0"** (o fonte estava em v0.301.0, 129
versoes a frente) e **"os pots e botoes ainda nao foram soldados"** — no
mesmo dia em que se mapeou dois pots, quatro botoes, dois LEDs e tres
encoders soldados.

**`docs/operacao/SUPRESSOR-MICROFONIA.md` dizia "Ainda nao construido"** dois
dias depois de o supressor existir e correr — e nunca usava o nome dele.

**`docs/operacao/CCMAP.md` dizia "87 vinculos em 50 numeros de CC · Firmware
v0.177.0"** quando o fonte tinha 119 em 81. O cabecalho era escrito a mao
enquanto a tabela por baixo era gerada: **metade do documento actualizava-se
sozinha e a outra metade nao**, e era a metade parada que o leitor lia
primeiro.

**`Max/`** guardava 1,8 MB de `.maxpat` aposentados, intocados desde
15–23/08, referidos por nenhum documento. Removidos da arvore, com o caminho
de volta **provado antes de apagar**: `git show 8271e97:Max/` lista os quatro,
e o do delay devolve os 104 335 bytes.

**Dez documentos vivos estavam fora do mapa do README** — o `PINOS.md` entre
eles, que e a fonte de pinos e se confere a cada `bts`.

> **O padrao, e e o mesmo da 15bt:** duas afirmacoes contrarias sobrevivem no
> mesmo repositorio enquanto nunca forem lidas na mesma passagem. O README
> tinha-as a quinze linhas de distancia e ninguem as leu juntas durante tres
> semanas.
>
> **A defesa nao e reler:** e fazer o documento nascer do codigo. Onde isso
> ja acontecia — a tabela do `CCMAP.md` — o conteudo estava certo. Onde nao
> acontecia, envelheceu.

---

## 15bw. Um facto de hardware afirmado sem fonte, e escrito em dois documentos (04/09)

O `ATERRAMENTO.md` §5 dizia, desde que foi escrito:

> *"Placas com codec de áudio (a Daisy entre elas) juntam `AGND` e `DGND`
> internamente, num ponto só."*

**Nao juntam.** A documentacao da libDaisy diz que se devem **sempre ligar os
pinos `DGND` e `AGND` fora da Daisy**, e a Seed expoe-os em dois pinos
distintos do header — 20 e 40, no `Daisy_Seed_Rev4_Pinout.csv` do fabricante.
A regra geral de conversores (Analog Devices) diz onde juntar: ao `AGND` no
proprio encapsulamento, ao mesmo potencial, e em mais lado nenhum.

O erro nao ficou parado. Em 04/09 o dB reformulou-o com mais forca, para me
guiar no passo do soquete — *"a ponte nao e um fio, e a propria Daisy; se
alguem desenhar um net-tie o PCB ganha uma segunda ponte"* — e **eu escrevi-o
no `MAPA-DE-TERRA.md` §2 tal e qual**, com o simbolo de proibido, sem pedir
fonte nenhuma. Duas afirmacoes minhas, em dois documentos, a dizer o
contrario do que o fabricante diz.

### O que isto mandou para o lado errado

**Toda a analise do ruido branco.** O P3 da varredura de terra foi escrito
como *"AGND e DGND unidos FORA da placa — a ligacao a mais"*, e o conserto
proposto era **separar** o AGND do trilho. Mas o trilho da permaproto era a
**UNICA** ponte: separa-lo deixaria o codec sem referencia.

E ja tinha acontecido uma vez. Em 31/08, seguindo esta mesma premissa, deu-se
um conselho que **matou uma placa**: *"tirar a terra de sinal trocou um
zumbido por uma placa morta"*. A skill `revisao-terra` ate ganhou uma passada
nova por causa disso (P3b), sem que ninguem tenha voltado a duvidar da
premissa que a gerou.

> **O P3 certo e MOVER a ponte, nao remove-la.** Sai do trilho partilhado com
> corrente e entra num fio curto entre os pinos 20 e 40, no soquete. E o anel
> continua a ser defeito — mas porque a ponte era um TRILHO, nao porque ela
> existia.

### A regra, e ela ja estava escrita no mesmo prompt

O prompt que me deu o passo 3 dizia, sobre a pinagem: **"a folha de dados da
Seed para o `Dnn` → header, e nada de memoria."** E na mesma mensagem afirmava
de memoria, sem fonte, o facto mais consequente do desenho.

> **"Nada de memoria" nao se aplica so aos numeros que se procuram — aplica-se
> com mais forca aos factos que se tem por sabidos.** Um numero que nao se
> sabe manda procurar; um facto que se julga saber nao manda nada, e por isso
> atravessa dois documentos sem ninguem o parar.

E o meu papel nisto tem nome: **recebi um facto de hardware sem fonte e
escrevi-o como regra, com sinal de proibido.** A pergunta que faltou custava
uma linha: *"isto vem da folha de dados ou da memoria?"*

### O que apanhou

Uma busca pela pinagem, feita por outra razao. O resumo trouxe de passagem
*"DGND and AGND must be connected to each other outside of the Seed"* — e a
regra da [[prova-externa]] valeu duas vezes: nao aceitei o resumo como facto
(fonte primaria inacessivel, 403 e 404), **parei**, e disse que nao desenhava
a folha com a questao aberta. Foi o dB que trouxe a fonte primaria, contra si
proprio.

⚠️ **E o teste continua a fazer-se.** Continuidade 20↔40 numa Daisy nua tem de
dar **aberto**. A premissa nova tambem e para medir — sobretudo esta, que ja
foi afirmada com confianca duas vezes em sentidos opostos.

---

## 15bx. Uma Daisy morreu, e a pergunta que a matou tinha resposta na doc (04/09)

Fim do dia 04/09, e custou uma placa.

**O que aconteceu.** Sob orientacao minha — a premissa errada da 15bw — o dB
tinha DESLIGADO o AGND do DGND. Ao reconectar, o instrumento ficou sem som.
Durante a investigacao a primeira Daisy foi encontrada **muito quente**, sem
USB, sem serial, e sem DFU nem com `BOOT+RST`. Substituida.

**A premissa, agora verificada em PRIMEIRA MAO.** A doc da libDaisy, na pagina
dos ADCs, diz textualmente:

> *"When working with the Daisy and external hardware, always connect the
> DGND and AGND pins outside of the Daisy"*

Ligam-se. Sempre. E o que restava era **onde** — e a resposta e nos pinos,
num fio curto, nunca num trilho partilhado com corrente.

**O resultado, medido.** Com o aterramento refeito e o encoder G resoldado, o
chao de ruido na entrada, em repouso:

    antes   IN_L -34,2 dBFS   IN_R -34,2 dBFS
    depois  IN_L -69,2 dBFS   IN_R -73,4 dBFS      35 a 39 dB

E o sintoma tinha assinatura: **-34,2 dBFS IGUAL nos dois canais.** Ruido
simetrico e problema de REFERENCIA; um piezo ou um cabo estragado seria
assimetrico. O numero dizia "terra" antes de qualquer hipotese.

### Tres erros meus na investigacao, e sao de familias conhecidas

**1. Identifiquei um dispositivo pelo NOME e construi conclusao em cima.**
Vi um `/dev/cu.wchusbserial` e anunciei "o ESP32 continua a enumerar, logo o
Mac e o cabo estao bons, a falha esta isolada na Daisy". O ESP32 nem estava
ligado. Efeito pratico: **descartei o cabo e a porta**, que eram suspeitos
legitimos, durante varias rondas. E a cicatriz do `porta_daisy` (15bd) — nao
inferir identidade por posicao ou por nome, PERGUNTAR — cometida dentro de um
diagnostico.

**2. Testei curtos e nao testei aberturas.** Pedi tres continuidades das
linhas de USB CONTRA A MASSA. Passaram as tres, e eu dei a hipotese por
morta. Mas uma linha PARTIDA da "aberto contra a massa" e passa nesse teste.
A medicao que faltava era ao LONGO da linha, do conector ao pino.

> **Um conjunto de testes que so procura curtos declara sao o que esta
> aberto.** Toda familia de defeito tem dois lados, e testar um deles nao e
> testar a familia.

**3. Corri atras de uma janela de 2 s com uma conversa de 30 s.** A janela de
DFU do bootloader dura 2 segundos. Passei mais de uma hora a pedir `rst` e a
verificar depois, seis vezes, sempre com o mesmo nada — porque entre o toque
dele e a minha verificacao passavam trinta segundos.

A regra da bancada ate diz "a lacada primeiro, o toque depois". Eu li-a como
uma ordem DENTRO da minha mensagem, e nao como o que ela e:

> **Quando a janela e mais curta que a latencia da conversa, a espera tem de
> viver do lado de quem carrega no botao.** A ferramenta e a primeira coisa a
> escrever, nao a ultima. `grava_placa_nova.command` devia ter existido na
> primeira tentativa, e existiu na decima.

### E o que se aprendeu sobre gravar uma placa nova

Uma Daisy de fabrica nao tem o bootloader da Electrosmith. A ordem e:

    BOOT+RST            -> DFU da ROM da ST (0483:df11), flash interna
    make program-boot   -> instala o bootloader (o erro no `leave` e normal)
    RST, depois BOOT    -> DFU do bootloader, que expoe a QSPI
    make program-dfu    -> o app

⚠️ **E o app NAO cabe na flash interna** — 283 KB contra 128 KB. Nao ha
atalho por `APP_TYPE=BOOT_NONE`: tem de passar pelo bootloader.

---

## 15by. `D29` e `D30` sao o USB, e o PINOS.md reservava-os para os DPDT (04/09)

Achado ao gerar o simbolo da Seed a partir do
`Daisy_Seed_Rev4_Pinout.csv` do fabricante — nao por suspeita, e nao por
nenhum verificador. Apareceu porque, pela primeira vez, alguem cruzou o
`Dnn` com o NOME DE ESQUEMA do pino:

    pino 36  ->  D29  =  USB_HS_D_-   (PB14)
    pino 37  ->  D30  =  USB_HS_D_+   (PB15)
    pino  1  ->  D0   =  USB_HS_ID    (PB12)

E o `PINOS.md` dizia, ha semanas: *"D29/D30 estao reservados para o sense dos
DPDT dos exciters"*.

**Sao as linhas de dados do proprio USB** — as que o conector usa e por onde
passa a serial. Reconfigura-las como GPIO mata a porta.

### Por que este passou por todos os filtros

O `check_docs` cruza o PINOS.md com o FONTE, e o fonte nao usa D29/D30 — logo
"0 no codigo e fora do doc, 0 no doc e nao usados". **Passa limpo.** O
verificador so podia ver uma colisao entre o documento e o codigo NOSSO; a
colisao era entre o documento e o SILICIO.

> **Um verificador delimita o que consegue ver, e o silencio dele tem a mesma
> cara dentro e fora desse limite.** Aqui o limite era "o que o nosso codigo
> faz". A folha de dados estava fora, e ficou fora durante semanas.

### E o modo de falha era um que ja conheciamos

Uma placa com D29/D30 ligados aos DPDT teria dado: firmware a correr, painel
bom, alimentacao normal, **e nenhuma forma de falar com ela**. E exactamente
o quadro em que passamos o dia 04/09 por outra causa — e teriamos passado
outro, com cinco placas feitas e a causa no cobre.

**Sense dos DPDT passa para `D11`/`D12`** (I2C1, PB8/PB9) ou `D20`/`D21`
(ADC, PC1/PC4). Livres, e sem funcao amarrada ao silicio.

⚠️ Fica registado tambem que **`D0` e o `USB_HS_ID`** e esta a servir de fase
A do encoder A. Em modo dispositivo o ID nao e usado, portanto funciona — mas
ninguem sabia, e e a mesma familia.

---

## 15bz. Afirmei posicao fisica de pinos sem olhar para a placa (04/09)

O dB disse que os pinos 20 e 40 eram "vizinhos no header 2x20", e eu escrevi
isso no `MAPA-DE-TERRA.md` como facto, com a consequencia agarrada — *"e e
isso que torna o fio curto"*.

**Medido por ele no fim do dia: nao sao.** O pino 20 fica no canto INFERIOR
ESQUERDO, colado ao USB-C; o 40 no SUPERIOR DIREITO, no extremo oposto.
Estao na diagonal do modulo.

E a consequencia inverte-se: **a ponte de terra nao pode ser um fio curto
entre vizinhos.** O condutor atravessa o comprimento do modulo, e por onde
ele passa deixa de ser detalhe e passa a ser decisao de layout.

> E a SEGUNDA vez no mesmo dia que aceito uma afirmacao de hardware sem
> fonte e a escrevo como regra (a primeira e a 15bw, sobre os terras
> internos). Da primeira vez eu ate desconfiei — escrevi "nao confirmei que
> 20 e 40 sao vizinhos" no commit do simbolo — e mesmo assim deixei a
> afirmacao contraria de pe no mapa, num sitio que ninguem cruza com o
> outro.
>
> **Duvidar num ficheiro e afirmar noutro nao e duvidar: e espalhar.**

O simbolo continua com o 20 e o 40 frente a frente no DESENHO, porque isso
torna o net-tie legivel no esquema. Fica anotado no gerador que e conveniencia
de papel e nao a placa.

---

## 15ca — A ponte não estava dentro da Daisy, e a instrução matou uma Seed (05/09)

> **Escrita pelo dB.** Entrou como `15ca` e não como `15bn`, que era o número
> no original: o `15bn` já estava ocupado pela ferramenta de reposição
> construída sobre um contrato deduzido. Texto dele, preservado.

**O que aconteceu.** O `ATERRAMENTO.md` e a conversa de arquitetura partiam de
uma premissa: "AGND e DGND encontram-se só dentro da Daisy". A documentação
oficial da libDaisy diz o contrário — *ao trabalhar com hardware externo,
ligar sempre DGND e AGND fora da Daisy*. A premissa foi afirmada duas vezes
pelo chat, sem fonte, no mesmo dia em que o prompt escrito por ele dizia
"nada de memória". O Code recusou um resumo de busca, procurou a fonte
primária, não a alcançou, e **parou**. Devia ter sido o chat a parar.

Com a premissa errada veio uma instrução de refazer terra: "pôr fio 20–40,
depois tirar o AGND do trilho". Na bancada, o trilho e a ilha eram o mesmo
cobre. "Tirar do trilho" foi lido como "tirar da ilha", e a ilha inteira —
pré-amp, jacks, DPDT — ficou sem referência nenhuma. Custou horas e uma
Daisy Seed 3.

**O facto corrigido.** A Seed **não** junta AGND e DGND por dentro. A ponte
única tem de ser um fio, e vai nos pinos 20 (AGND) e 40 (GND) do soquete —
~~vizinhos, ~5 mm~~, no encapsulamento do conversor, como a Analog Devices
recomenda para qualquer ADC/DAC. No KiCad é um **NetTie-2**, uma ligação de
cobre só, num sítio escolhido; `AGND` e `GND` continuam duas redes. O P3
nunca foi "tirar uma segunda ponte": o trilho da permaproto era a **única**,
e o erro dele era ser trilho partilhado com corrente, não ser ponte. O 31/08
("tirar a terra de sinal matou a placa") era isto mesmo, e não se leu.

> ⚠️ **O "vizinhos, ~5 mm" acima está riscado porque está errado, e o próprio
> dB corrigiu-o na mesma sessão** — ver a nota do chat logo abaixo. Fica
> visível em vez de apagado: é uma cicatriz sobre afirmar posição de hardware
> sem fonte, e apagar o erro de dentro dela tirava-lhe a prova.

**As regras, e são para o chat cumprir, não só para a bancada:**

1. **Terra e alimentação só se refazem com a Daisy FORA do soquete.** Volta
   depois de a continuidade confirmar o estado final. Um erro de instrução
   custa uma hora; nunca uma placa. É a regra que não depende de ninguém
   acertar em nada.
2. **Nenhuma instrução de fiação sem antes ver a fiação real** — foto,
   descrição em palavras do construtor, ou tabela de continuidade do estado
   atual. Sem isso, perguntas; não passos.
3. **Refazer vem como tabela antes/depois em continuidade, e o primeiro
   passo mede o "antes".** Se não bater com a tabela, a instrução está errada
   para esta bancada, e descobre-se com o multímetro.
4. **Reversão escrita antes do gesto.** Uma linha: "se piorar, faz X".
5. **Facto de hardware sem fonte citada é hipótese, e diz-se como hipótese.**
   O tom confiante do chat não correlaciona com estar certo; trata-se assim
   dos dois lados. Se a folha de dados não está aberta, escreve-se "não sei".
6. **O construtor repete o plano por palavras suas antes de pegar no ferro.**
   Teria apanhado "trilho" ≠ "ilha" em dez segundos.

**A que organiza estas:** *quando uma instrução pode matar hardware, a garantia
não é a instrução estar certa — é a instrução não poder matar.* A regra 1 é a
única que sobrevive a um chat errado, e por isso vem primeiro.

**Origem do erro, nomeada:** a hipótese plausível é a perigosa (regra antiga),
agora aplicada ao próprio conselheiro. E "nomear uma classe obriga a procurá-la
em casa" — nomeei "nada de memória" e não fiz grep à minha própria afirmação.

### As três notas do chat, no fecho

**"Vizinhos, ~5 mm" estava errado.** Os pinos 20 e 40 ficam em cantos opostos
do 2×20 — eu afirmei a adjacência sem olhar a pinagem, no mesmo dia desta
cicatriz. O Code leu o CSV e acertou. A colocação dele é a certa: net-tie
**encostado ao pino 20**, dentro da ilha, e a outra ponta do tie cai no plano
de GND ali mesmo por uma via. O caminho curto é do pino 20 à ponte; a
distância até ao pino 40 é do plano, não do condutor.

**O roteiro tem os pinos livres errados.** Eu listei D25, D26, D29, D30 como
livres; D29/D30 são o USB, e a lista real é **D11, D12, D20, D21**. O
`ROTEIRO-KICAD.md` §4 corrige-se pela lista do Code — o CSV manda, o meu
resumo não.

**O número que paga a semana:** −34,2 → −69,2 / −73,4 dBFS em repouso.
**Trinta e cinco a trinta e nove decibéis** de chão, e simétricos nos dois
canais — o que confirma que era referência, como o Code escreveu. O sustain a
−49 passa de 15 dB de margem para 20–24, com a placa ainda em permaproto. É o
resultado que custou a Daisy, e vale dizer que existe.

Sobre a folha do soquete, um detalhe para o ERC: com o net-tie, `AGND` e `GND`
continuam duas redes e as duas PWR_FLAG existentes ficam certas — uma em cada.
O net-tie **não** substitui nenhuma delas. E as duas isenções de `AUDIO_IN`
morrem neste commit, como o prazo dizia.

> **Nota do Code, para não haver dívida silenciosa:** a folha do soquete ainda
> não existe, portanto as duas isenções de `AUDIO_IN` **continuam vivas**. O
> prazo delas morre no commit que a criar, e é lá que se confere.

---

## 15cb — MPN escolhido sem abrir a ficha é palpite com número de peça (05/09)

**05/09.** Três peças escolhidas pela família, sem ler o código de estilo. As
três desmentidas pela ficha, e nenhuma delas se via no esquema:

| escolhido | o que a ficha diz |
|---|---|
| `PTV09A-4020F-B103` | o `F` é eixo **liso**; a nota ao lado dizia "estriado" |
| `PEC11R-4220F-S0024` | pinos do encoder a **5,0 mm** — o footprint do KiCad é do Alps, a **2,5** |
| `2920L300/24MR` | não existe em stock; o que há a 24 V é o **3,3 A** |

> **Um número de peça parece um facto e é uma afirmação.** Enquanto ninguém
> abrir a ficha, "MPN escolhido" e "MPN adivinhado" são indistinguíveis — e
> escrevem-se exactamente da mesma maneira na BOM.

O que falha é sempre a mesma casa: **o sufixo**. A família está certa, o
fabricante está certo, o valor está certo. É o carácter que diz o estilo de
montagem, o tipo de eixo ou a tensão que desmente tudo.

**E eu também errei a ler.** Reportei que o `PTV09A-4` "não serve" por ser
*bushingless* — mas com o pot **na placa**, o furo do painel é passagem e quem
segura é o PCB. O `-4` serve; o que ele não permite é apertar porca ao painel.
Cheguei a essa conclusão de um resumo de busca, não da tabela de estilos, e o
dB decidiu com base nela. A tabela só apareceu quando um distribuidor
descreveu a mesma peça de outra maneira e a contradição obrigou a ir à fonte.

> **Resumo de busca não é ficha.** Um resumo diz o que alguém entendeu; a
> ficha diz o que o fabricante afirma. Só a segunda serve para decidir — e
> quando as duas discordam, a busca não empata: perde.

**A defesa que funcionou nas três** foi a mesma: ir à fonte primária. A ficha
da Bourns em PDF (extraída e **olhada** como imagem, porque o texto saía
baralhado), a tabela de estilos em texto puro, e a página do distribuidor com
stock. Nenhuma das três teria sido apanhada por ERC, netlist ou verificador —
**o esquema fica igualmente limpo com o MPN errado.**

**A regra:** todo MPN entra na BOM com a linha da ficha que justifica **cada
carácter do sufixo**. Sem isso, é família com número.

---

## 15cc — O leitor que perde linhas em silêncio aprova tudo (05/09)

Três ferramentas, no mesmo dia, leram menos do que existia e **não disseram
nada**. Uma delas era o verificador escrito para apanhar as outras duas:

| leitor | tinha | leu | o que anunciou |
|---|---|---|---|
| `pinos_de()`, em 4 geradores | 26 pinos | 24 | desenhou 24 rótulos |
| `contrato_idc.py` v0.1.0 | 26 vias | 24 | *"as 26 vias batem"* |
| `colisoes_sch.py` v0.3.0 | 40 propriedades | 0 | *"0 colisões"* |

A causa da primeira: `h/2 − 1.27 − 6×2.54` não dá zero, dá
**`1.7763568394002505e-15`**. O KiCad aceita; nenhuma regex escrita com
`[\d.]+` — que é como toda a gente escreve — apanha essa linha. **Os dois
pinos que sumiram eram terra**, e a folha saiu com 24 rótulos em vez de 26 sem
um único aviso.

> **Um leitor que perde linhas em silêncio não reprova nada — aprova tudo.**

**A guarda é contar.** Quantos blocos existem, contra quantos a leitura
apanhou. E a guarda tem de contar **o mesmo universo** que o leitor: contar
propriedades escondidas, que o leitor salta de propósito, dá alarme falso — e
guarda que dá alarme falso é guarda que se desliga.

**Provar a guarda pede DOIS testes.** Injectar o defeito conhecido prova a
*correcção*, não a guarda: a regex nova lê-o e a guarda cala-se, como deve.
Só um defeito **desconhecido** prova que a guarda existe.

E a variante que me apanhou à terceira: um `.kicad_sch` pode ter `)\t(symbol`
— o bloco seguinte a começar na MESMA linha em que o anterior fecha. Numa
renomeação de 27 referências, **uma** escapou; o relatório disse "26
renomeadas" e eu aceitei o número **sem o comparar com 27**. Quem a apanhou
foi a comparação de netlist.

> **Varre por contagem de parêntesis, não por indentação.** E quando um
> relatório diz "N feitos", compara N com o que era esperado.

---

## 15cd — Portão de linha de comando todo verde não prova que o ficheiro abre (05/09)

O dB abriu o KiCad e apareceu: *"An error was found when loading the schematic
that has been automatically fixed. Please save the schematic to repair the
broken file."* Os portões estavam **todos verdes**.

| ferramenta | o que dizia de um ficheiro sem `(instances)` |
|---|---|
| `sch erc` | as violações de sempre, nem mais nem menos |
| `sch export netlist` | exportava bem — o aviso que dava era de anotação, outra coisa |
| `sch upgrade` | *"Schematic file was not updated"* — o formato já era o corrente |

A causa era o bloco `(instances)` em falta nos símbolos. **Nenhum comando o
denuncia.** Só o editor repara.

**Gastei três hipóteses erradas antes de medir** — `(instances)` em falta, a
forma do caminho, dados obsoletos — e cheguei a escrever na skill que
"`(instances)` não era a causa". Era, do problema do editor; não era, do aviso
de anotação que eu estava a medir. Medir a coisa errada e concluir sobre a
certa.

**O método que respondeu:** pedir ao dB para abrir, clicar `OK` e **gravar** —
e depois `diff` entre o que o KiCad gravou e o que estava lá. **O diff é o
KiCad a dizer literalmente o que reparou.** Guardar a cópia ANTES de mandar
abrir, senão o diff mistura as nossas edições com as dele.

> **Quando a ferramenta que reclama é a única que sabe, é a ela que se
> pergunta.** Não se adivinha em três rondas o que um `⌘S` responde numa.

E o corolário que fecha o laço: **o gerador tem de reproduzir o que o editor
escreve**, senão a regeneração seguinte apaga a reparação e a caixa volta.

---

## 15ce — Lista escrita à mão ao lado de coisa gerada envelhece sempre (05/09)

O `gera_painel.py` tinha a biblioteca embutida declarada assim:

```python
lib_embutida({"IDC26","CONN7_L","EC11","POT3","SW_H","MOSFET_N","LED",...})
```

Uma **lista literal**, ao lado de uma folha gerada. Ao trocar os botões e os
LEDs por conectores, a lista continuou a dizer `SW_H` e `LED`: a folha saiu a
referir um símbolo que a biblioteca embutida não tinha, **o KiCad viu um
símbolo SEM PINOS**, e os 10 fios ficaram soltos. ERC saltou de 0 para 20 e o
gerador não disse uma palavra.

A mesma família estava em três sítios no mesmo dia: esta lista, o `USADOS` do
`gera_soquete.py`, e a `BOM-MPN-revB.md` — que divergia do esquema em quatro
pontos ao mesmo tempo (nomes anteriores a uma renomeação, contagens, número de
`PWR_FLAG`, e seis peças já decididas como fio mas ainda desenhadas como peça).

> **A cura não é actualizar a lista. É a lista não existir** — derivar do que
> foi realmente colocado, e recusar-se a escrever quando não bate.

O que ficou: `USADOS` preenchido por `simbolo()`, `lib_embutida()` a recusar um
símbolo que não encontra, e uma verificação **depois de escrever** de que todo
`lib_id` tem entrada no `lib_symbols`. E a BOM do instrumento passou a ser
**gerada das folhas** (`tools/bom_completa.py`).

---

## 15cf — Três fontes plausíveis para a mesma caixa, e eu escolhi duas erradas (06/09)

O contorno das duas placas e a posição dos três eixos que saem pela tampa vinham
de `caixa/medidas/*.svg` — **desenhos**. Fui corrigi-los contra
`caixa/cotuca_caixa_corte_nogueira_6p4.svg` — **um ficheiro que foi mesmo para
a fábrica**. Estava certo no método e errado no objecto: o dB tinha mandado
cortar a caixa em **acrílico**, e a de nogueira é *outra caixa*.

| fonte | é real? | é **esta** caixa? |
|---|---|---|
| `medidas/*.svg` | não — nunca foi cortado | não |
| `..._nogueira_6p4.svg` | **sim**, foi para a Ponoko | não — nogueira de **6,4** |
| `cotuca_caixa_corte.py` | sim, é gerador | não — está em **6,0** |
| **o que o dB encomendou** | — | **acrílico transparente de 3,00** |

Quatro fontes, quatro espessuras, e as três primeiras davam números plausíveis.
Cada vez que corrigi, corrigi para outra caixa errada.

⚠️ **As três primeiras foram apagadas a 06/09** — `caixa/medidas/`, os dois
`gen_caixa*.py` e os ficheiros da nogueira. Estão no histórico
(`git show 94066fd:caixa/medidas/caixa-fabricacao.svg`). Não se guarda um
artefacto real de uma geração anterior ao lado do actual: é indistinguível do
certo até se medir, e medir é precisamente o que ninguém faz antes de confiar.

> **«O ficheiro que foi para a fábrica» não chega.** Tem de ser o ficheiro da
> peça que está a ser feita **agora**. Um artefacto real da geração anterior
> mede-se tão bem como o certo, e dá números plausíveis e errados.

O que a segunda fonte me fez escrever, e que estava errado: interior
**187,2 × 107,2** (é **188 × 108** — o valor que lá estava desde o início e que
eu «corrigi»), chapa de fundo 186,2 × 106,2 (é **187 × 107**), calços de 12,8
(são **12,0**), e uma correcção de **0,137 mm** nos eixos dos knobs que não
existia — nasceu de eu medir a distância do furo à *aresta cortada* da tampa,
quando o datum é a **face**; os dedos da junta fazem as duas não coincidirem.

**E o gerador apareceu a meio.** Procurei-o por nome e por conteúdo, não o
encontrei, e escrevi no `caixa/README.md` que ele não existia — o que era
verdade quando procurei. Vinte minutos depois estava no disco, escrito pelo dB
na mesma sessão.

> **Ausência verificada tem prazo de validade.** Num repositório que outra
> pessoa está a mexer ao mesmo tempo, «procurei e não existe» é uma medição com
> hora, não uma propriedade. Antes de agir sobre uma ausência — sobretudo para
> justificar manter cota à mão — **voltar a procurar**.

### O conserto que sobrevive à próxima mudança de espessura

Corrigir a cota à mão a cada volta é perder — foram quatro em duas horas. O que
fecha isto é **não haver cota**: a placa **centra-se na chapa de fundo**, e daí
sai um invariante que se demonstra em duas linhas de álgebra e se confirma
correndo o gerador noutra espessura:

```
furo_x  = 25,5 − T − 0,5            (o gerador desconta a parede)
x0      = ((200 − 2T − 1) − 180)/2  (placa centrada na chapa)
furo_x − x0 = 15,5                  para QUALQUER T
```

Medido, não deduzido — corri o gerador a `T = 3,0` e a `T = 6,0`:

| | T = 6,0 | T = 3,0 |
|---|---|---|
| interior | 188 × 108 | 194 × 114 |
| chapa de fundo | 187 × 107 | 193 × 113 |
| calço | 12,0 mm | 6,0 mm |
| **furos, relativos à placa** | **(15,5 · 128,5) × (3,5 · 96,5)** | **idênticos** |
| **eixos dos knobs na face** | **105/140/175 × 92** | **idênticos** |

> **Quando um número externo não pára de mudar, o conserto não é segui-lo — é
> construir a peça de modo a que ele deixe de entrar na conta.** A placa passou
> a ser imune ao material da caixa, e o `pcb_contorno.py` **recusa-se a escrever**
> se o invariante deixar de valer.

### O resto do que ficou

`kicad/pcb_contorno.py` **v0.5.0** deixou de transcrever cotas e passou a
**importar o gerador da caixa**. O `cotuca_caixa_corte.py` tem
`if __name__ == "__main__"`, por isso importa-se sem correr nada — e importar é
mais forte que medir um SVG: **não há parsing entre a cota e o uso, logo não há
sítio onde as duas divirjam.**

Mais dois portões que se recusam a escrever:
- a placa **cabe** na caixa? (cérebro 180 × 100 na chapa de 187 × 107, folga
  3,50 em volta; painel 176 × 36, folga mínima 4,00)
- algum dos **4 furos de fixação** do fundo ficou debaixo de um componente? O
  `J14` estava por cima de um e saiu de x=30 para x=40. Um furo tapado não é um
  furo apertado — é um parafuso que não entra com a caixa fechada e a solda
  feita.

### A raiz, que é a mesma de sempre

`INTERIOR = (188.0, 108.0)` estava declarada e **nunca era lida** por linha
nenhuma. Por acaso estava certa. Isso é pior, não melhor: uma cota que ninguém
consulta não pode estar errada em voz alta, e por isso também não pode estar
certa **de forma verificável** — a que lá estava era indistinguível de sorte.

> Mesma família do `USADOS` escrito à mão de [15ce](#15ce), noutra roupa: ali
> uma lista ao lado de coisa gerada, aqui uma cota ao lado de coisa medida.
> **A cura não é actualizar. É derivar e recusar.**

Corolário de revisão: ao ler um gerador, `grep` cada constante declarada pelo
seu próprio nome. **Zero usos é achado**, mesmo com o valor certo.

### O que a importação descobriu, e nenhum desenho dizia

- Os calços são **12 peças** — 6 `CB` com sexágono para porca M3 cativa e 6
  `CT` que a tapam — logo **12,0 mm** de altura, e a placa passa por cima de
  seis. **Espaçador de 8 mm não serve; mínimo 15.**
- **Quem abre esta caixa é o fundo**, não a tampa: a casca é soldada e os seis
  parafusos sobem do fundo para os calços. A electrónica serve-se por baixo.
- Altura útil **43 mm à frente e 68 atrás**, contra os 28/58 dos desenhos. Isso
  **revogou a premissa escrita** para a Daisy estar rodada e o `C1` ficar atrás
  — as duas decisões continuam certas, por outro motivo (é na parede de trás
  que estão os recortes), e o comentário foi corrigido. Comentário com
  autoridade que afirma premissa revogada re-ensina o erro.
- **Não existe berço de powerbank.** Tinha uma página inteira de desenho e
  nunca foi cortado — e era a única pergunta que estava a bloquear o layout.

### E o contra-exemplo, que é metade da lição

Ia anunciar que o esquema tinha dois arcade e a caixa três. Fui ver primeiro: o
esquema já tinha **três** (`J3` freeze A, `J4` freeze B, `J5` búzios). Quem
estava desactualizado era o desenho, outra vez.

> **A divergência anunciada sem medir tem 50% de hipótese de acusar a peça
> certa** — e o custo de acusar a errada é mandar mexer no que estava bem.

---

## 15cg — A omissão errada não rebenta: responde com confiança sobre outro objecto (06/09)

A limpar a pasta `caixa/`, encontrei isto no gerador da caixa:

```python
T = _arg("t", 6.0)          # espessura
```

O `kicad/pcb_contorno.py` **importa** este módulo e não passa argumentos. Logo,
durante todo o trabalho de dimensionar as placas, o portão estava a conferi-las
contra uma caixa de **6 mm** enquanto a encomendada é de **3**. Dizia
«espaçador mínimo 15» quando o certo era 9, e «folga 3,50» quando é 6,50.

> **Um valor por omissão errado não dá erro — dá resposta.** E dá-a com o mesmo
> ar de certeza que a certa. Um portão que mede o objecto errado é pior que
> nenhum: os dois deixam passar o defeito, mas só um o faz com um ✓ ao lado.

O conserto foi de uma linha — a omissão passa a `3.0`, a caixa que está a ser
feita — mas a regra é maior:

> **A omissão de um parâmetro de fabrico é o objecto actual, nunca o último que
> se desenhou.** E quando um módulo é importado por outro, o valor por omissão
> deixa de ser conveniência de linha de comandos e passa a ser **a
> configuração de produção do importador**.

Corolário de revisão, irmão do «grep cada constante pelo seu nome» de
[15cf](#15cf): **para cada valor por omissão, perguntar quem o consome sem o
passar.** Se houver um importador, a omissão é a configuração dele.

### E o resto da limpeza, que é a mesma lição noutra escala

Morreram nesta arrumação as 6 páginas de fabricação, os dois geradores delas, e
o corte em nogueira de 6,4 com toda a sua documentação. As páginas desenhavam
uma caixa que **nunca foi cortada**; a nogueira era **real e outra**.

Tinha-as mantido «como registo». Estava errado:

> **Arquivar uma fonte vencida ao lado da actual é o mesmo que não a apagar.**
> Ela mede-se igualmente bem, e a única coisa que a distingue da certa é uma
> verificação que ninguém faz antes de confiar. Foi exactamente assim que as
> duas PCB foram dimensionadas contra a caixa errada, **duas vezes seguidas**.

O histórico do git é o arquivo. A pasta é o presente.

⚠️ **Mas apagar sem `diff` é a outra metade do erro,** e quase a cometi na
mesma sessão: havia um `caixa-fabricacao-p2-tampa.svg` solto na raiz, com ar de
cópia esquecida. Era a versão **nova**, a única com os três arcade. E antes de
apagar, o que tinha valor e não era cota — as regras de percurso de cabo — foi
resgatado para o `CAIXA-ACRILICO-MONTAGEM.md`.

> **Apaga-se a fonte vencida; não se apaga o que só ela dizia.** Ler antes de
> apagar, e mover o que sobrevive.

---

## 15ch — Atribuí um delta a uma de duas coisas que mudaram, e é a minha própria regra R2 (06/09)

O dB leu a cotação da Ponoko e disse: *«a otimização economizou $1.90 então não
muito»*. Aceitei o número, construí uma regra de projecto em cima dele —
*«o encaixe move a viagem do laser, não o corte; numa caixa de peças grandes
vale $1,90»* — e escrevi-a na skill e no README.

Estava errado, e o erro não era o número: era **atribuí-lo**.

Entre as duas encomendas mudaram **duas** coisas: o ficheiro passou de
`nogueira_6p4` (dentes para 6,4 mm) para `acrilico_3p0` (dentes para 3,0), *e* o
encaixe foi refeito. Quando as facturas chegaram, dava para medir:

| encomenda | área | corte | preço | $/m |
|---|---|---|---|---|
| #1263434 | 1289 cm² | 6,39 m | $37,56 | **5,878** |
| #1263497 | 1103 cm² | 6,07 m | $35,66 | **5,875** |

**O preço segue o comprimento de corte, e só.** Prevê-se de uma factura para a
outra com **$0,02** de erro; por área erra **$3,52**. `Setup $0.00`, sem linha
de material — **não há preço de chapa**.

E o corte é uma propriedade das **peças**, não da arrumação: os mesmos 6,07 m
antes e depois de encaixar.

> **O encaixe poupou $0,00.** O $1,90 veio inteiro da mudança de espessura, que
> muda a geometria dos dentes. Eu tinha atribuído o delta à única das duas
> mudanças em que estava a pensar.

Isto é literalmente a **R2** da skill `full-audit`, que eu próprio escrevi:

> *«Todo número declara origem: medido ou deduzido. **Delta entre builds NÃO é
> atribuível** (cicatriz 12d). Custo se mede por A/B no mesmo binário.»*

A regra estava escrita, era minha, e não a apliquei — porque desta vez o número
não veio de um build, veio de uma factura, e não reconheci a forma.

> **A regra do A/B não é sobre builds: é sobre deltas.** Duas medições que
> diferem em mais de uma coisa não atribuem nada a nenhuma delas, seja o que
> for que se esteja a medir — tempo de CPU, ruído, ou dólares.

E o corolário que dá o método: **a atribuição não se adivinha, decompõe-se.** O
preço decompôs-se assim que houve uma grandeza por metro — e aí cada troço de
corte ganhou preço: os 12 calços custam **$4,11**, os dentes e furos **$11,74**,
o piso de rectângulos lisos **$23,89**.

### Duas coisas que a decomposição matou de caminho

- **O «degrau da chapa» não existe.** Eu tinha posto «caber numa P1» como a
  alavanca nº 1 de custo. Não há cobrança de chapa: uma caixa mais pequena é
  mais barata por ter menos perímetro, linearmente, sem degraus.
- **O maior custo não está no desenho.** O envio são **$22,43 sobre $35,66 de
  peça — 39% do subtotal**. Várias caixas no mesmo pacote poupam mais que
  qualquer coisa que se faça ao ficheiro.

### E a encomenda que se salvou

A #1263434 foi o ficheiro da **nogueira de 6,4** mandado cortar em **acrílico de
3,00**. É exactamente o desencontro que estava escrito em [15cf](#15cf): junta a
dedo tem profundidade igual à espessura, logo 3 mm de folga em cada junta e
3 mm de lingueta saliente. Foi substituída pela #1263497 com crédito integral
de $58,09 — custou $0,00.

> **O aviso valeu o que valeu porque chegou antes da produção, não porque
> estava certo.** Facto de fabrico dito depois da chapa cortada é história.

---

## 15ci — O DRC vê a placa em duas dimensões; o botão desce na terceira (06/09)

O dB mediu o arcade que já comprou: **40 mm de profundidade**. Com isso a coluna
de ar na linha dos botões fecha-se:

```
face de baixo da tampa      56,1 mm acima da chapa de fundo
menos o reforço (3)         53,1
ponta do botão (−40)        13,1
face de cima da placa       11,6   (espaçador 10 + PCB 1,6)
FOLGA                        1,5 mm   ← sobre a placa NUA
```

**1,5 mm.** E os três discos de ø30 caem sobre a placa em `y 24,3 · x 30, 90,
150` do canto dela — com **15 peças** debaixo deles numa orientação e **8** na
outra, incluindo a Daisy e o `C1`, que é um electrolítico de **25 mm**.

> **Nenhum DRC vê isto.** Para o KiCad a coluna de ar acima do cobre é infinita:
> ele não sabe que existe uma tampa, e muito menos que ela tem um botão de
> 40 mm pendurado. A altura não é uma regra que se afrouxou — é uma dimensão
> que a ferramenta não tem.

Três verificações verdes — `GEOMETRIA 0`, `paridade 0`, `ERC 0` — e a placa não
entra na caixa. Cada uma respondia à sua pergunta com honestidade; nenhuma
respondia a esta.

> **Portão verde não é «está bem»: é «está bem NAQUILO que este portão mede».**
> A pergunta que fica por fazer é a que não tem ferramenta — e é por isso que
> precisa de ser feita à mão, e escrita.

O que ficou: `pcb_contorno.py` **v0.7.0** projecta os discos dos arcade sobre a
placa e lista o que lhes fica por baixo. É a caixa a impor volume à placa, na
mesma família do `confere_furos` (parafuso debaixo de um conector) e do
`confere_caixa` (placa maior que a chapa) — **a caixa manda em três dimensões,
não em duas.**

⚠️ E fica em aberto o que só o dB decide, agora com dentes: **que aresta da
placa olha para a parede da frente**. Escolher mal não é feio, é 15 peças
esmagadas em vez de 8.

### O portão certo no sítio errado deixa de ser portão

De caminho, cometi e consertei outra: pus a recusa do `--reforco-furo` no topo
do gerador da caixa. Como o `pcb_contorno.py` **importa** esse módulo — e não
tem opinião nenhuma sobre o furo do reforço — a recusa passou a rebentar a
importação.

> Um portão colocado num sítio por onde passa quem não é o seu destinatário
> deixa de ser portão e passa a ser avaria. Recusa-se no `__main__`, que é
> quem escreve a chapa.

---

## 15cj — Planta, volume, e a terceira que ninguém mede: o ACESSO (06/09)

O dB perguntou como reprograma a Daisy e o ESP32 com o instrumento montado.
Medido:

```
U2: USB-C na face x = 95,6 · z = 16,1 · a 17,9 mm da parede de trás
recorte que a caixa tem: x = 34,0 · z = 38,0 · 12 × 6
→ 61,6 mm de erro em x, 21,9 em z, e o módulo nem chega à parede
```

E o recorte que existe **não está livre**: é o do *decoy* PD, a entrada de
energia. **A caixa como foi encomendada tem zero aberturas de USB
disponíveis.**

> A caixa impõe três coisas à placa, e vão-se descobrindo por esta ordem, cada
> uma mais tarde que a anterior:
> 1. **planta** — o contorno e os furos. Vê-se num desenho.
> 2. **volume** — o que desce da tampa por cima da placa. Não se vê em desenho
>    nenhum, e nenhum DRC o mede ([15ci](#15ci)).
> 3. **acesso** — o gesto de enfiar um cabo, apertar uma porca, chegar a um
>    parafuso. **Não é uma ligação nem um sólido: é uma pessoa a fazer uma
>    coisa**, e por isso não tem representação em ficheiro nenhum.

O acesso foi o único dos três que descobri **depois de a caixa estar
encomendada**. Não por ser mais difícil de medir — a conta acima são seis
linhas — mas por não estar em lista nenhuma. As duas primeiras tinham portão
porque alguém as tinha pago antes.

> **A pergunta que falta fazer é a que não tem ferramenta.** E a forma de a
> fazer não é lembrar: é escrever a lista de gestos que o objecto montado tem
> de permitir, ao lado da lista de ligações que ele tem de fazer.

Para este instrumento, os gestos são: **reprogramar** (dois módulos),
**alimentar**, **ligar os jacks**, **trocar o powerbank**, **abrir para
manutenção**. Três estavam previstos; o primeiro não.

O que ficou: `confere_acesso` no `pcb_contorno.py`, que projecta o USB de cada
módulo declarado em `ACESSO_USB` na parede de trás e diz se bate num recorte —
e diz também **de quem é** o recorte, porque um recorte ocupado é tão inútil
como um recorte inexistente.

⚠️ **Custo real:** zero nesta caixa, porque o fundo sai com seis parafusos e
leva tudo com ele — era assim que ela já estava desenhada. Mas passa a ser
requisito escrito da r3, e não uma coisa que se lembra na hora.

---

## 15ck — Um diagrama de pinagem sem a face declarada é meio facto (06/09)

Três leituras contraditórias da mesma peça, em duas horas:

| fonte | 5V fica |
|---|---|
| pesquisa (fotos do fabricante + dois footprints) | **direita** |
| eu, a ler o diagrama «PIN Definitions» do vendedor | esquerda |
| o dB, com o módulo na mão | **direita** |

Passei o tempo a suspeitar da **peça** — a pesquisa avisara que estas placas
variam entre fabricantes, e eu tinha uma explicação pronta que encaixava. Não
era isso. O diagrama do vendedor **mostra o lado de baixo do módulo e não o diz
em lado nenhum**. Lido como vista de cima dá o espelho, e o espelho põe o 5V
onde estava o GND.

> **Vista de cima e vista de baixo de uma pinagem são a mesma imagem
> espelhada.** Nada dentro da imagem denuncia qual é. Um diagrama de pinagem
> que não declara a face não é um facto ambíguo — é **meio facto**, e a metade
> que falta é a que inverte o resultado.

⚠️ **E ter uma explicação plausível à mão atrasou a descoberta.** «As variantes
divergem» era verdade, estava documentada, e cobria os sintomas na perfeição.
Uma hipótese que explica tudo e que já se acreditava antes dos dados é o sítio
mais confortável para se estar errado.

### O que ficou, e não é «ter cuidado»

O footprint (`kicad/gera_footprints.py`) **declara a face no cabeçalho, em
maiúsculas**, e os pads levam os **nomes da serigrafia** — `5V`, `GND`, `IO21` —
em vez de números. Quem montar compara o silk do módulo com o nome do pad e não
tem nada que interpretar. Se um dia chegar uma remessa espelhada, vê-se **ao
pousar o módulo**, não ao ligar a alimentação.

> **Quando uma convenção pode ser lida ao contrário, não se escolhe a certa: põe-se
> o nome do que se vê em cima do que se solda.** O pad chamado `5V` só bate no
> silk `5V`, e isso não depende de ninguém ter percebido de que lado era a foto.

Nota de método: quando três fontes discordam sobre uma peça que **está em cima
da mesa**, a pergunta certa não é qual delas vale — é *«que medição as separa?»*
A que propus foi continuidade entre a carcaça do USB e os pinos, porque um
apito não se interpreta. Chegou primeiro a informação de que faltava a face, e
essa resolveu as três de uma vez.

---

## 15cl — O vazio de uma camada não é o vazio da face (06/09)

Pus a marca do instrumento na serigrafia de **trás**, a meio da placa, com o
raciocínio: *«atrás não há serigrafia nenhuma, logo está vazio»*. A camada
estava vazia. A **face** não: tem **130 pads passantes**, e a marca ficou por
cima deles — 13 `silk_over_copper`, serigrafia por cima de sítios que levam
solda.

> **Uma camada vazia não é espaço livre.** O que ocupa uma face são todas as
> camadas que aparecem nela — e num PCB os pads passantes atravessam. Perguntar
> «há alguma coisa nesta camada?» responde a outra pergunta.

O conserto não foi mover a olho: foi **varrer** a face contra todos os pads
passantes e encontrar a maior faixa livre — 123 × 8 mm a partir de (73, 23). A
marca foi para lá, e o `silk_over_copper` foi de 13 a **zero**.

### E a metade que NÃO era defeito

O dB também reportou a marca **espelhada**. Não é: `(justify mirror)` é o que o
KiCad escreve para texto de face de trás, e é assim que ele se lê **quando se
olha para as costas da placa** — que é o que se vê pelo fundo transparente da
caixa. No editor, visto de frente, aparece invertido, e essa é a aparência
correcta.

> **Duas queixas juntas não são um defeito com duas caras.** Uma era real e a
> outra era o comportamento certo a parecer errado — e tratá-las como uma só
> teria «consertado» a que estava boa.

---

## 15cm — Ferramenta aberta não vê biblioteca nova (06/09)

Criei `kicad/cotuca.pretty` e as `fp-lib-table` às 11:28. O dB tinha o KiCad
aberto desde antes. Fez F8, e o resultado foi meio: o `J10` **saiu** (já não
está no esquema) e o `U3` **não entrou**, porque `cotuca:` não existe para
aquela sessão. A placa ficou com uma peça a menos e nenhuma a mais.

> **Um editor lê as tabelas de bibliotecas uma vez, ao carregar o projecto.**
> «Existe no disco» não é «existe para o processo que está a correr».

É a mesma família de [15cf](#15cf) — *ausência verificada tem prazo de
validade* — virada do avesso: ali a coisa apareceu depois de eu procurar, aqui
apareceu depois de **ele** procurar.

⚠️ **Regra:** quando eu criar uma biblioteca, uma `fp-lib-table` ou uma
`sym-lib-table`, o pedido ao dB não é «faz F8» — é **«fecha o KiCad por
completo, reabre, e só então F8»**. Dizer só a segunda metade produz
exactamente o sintoma acima, e o sintoma parece perda de trabalho.

E o que salvou: o ficheiro no disco tinha data **anterior** à mudança, o que
provou em dois segundos que nada se tinha perdido. **Antes de diagnosticar um
ficheiro, ver a hora dele.**


---

## 15cn — Data e dado gerado, nao escrito (2026-09-06)

Os documentos deste projecto sao datados a mao: cabecalhos de `STATUS.md`,
cicatrizes, «atualizado em». O dB apontou que isso e a mesma classe da lista
`usados` escrita ao lado de coisa gerada — **escrita a mao envelhece em
silencio** — e mandou consertar em tres camadas, com a mecanica primeiro.

> **Data e dado gerado, nao escrito.** Instrucao que manda «confirmar a data»
> depende de alguem lembrar, e o que depende de lembrar nao e portao.

**A camada que torna as outras possiveis** e um hook `UserPromptSubmit` que
injecta o relogio no contexto antes de cada prompt. A partir daí nao ha data a
lembrar — ha data a **ler**. Confirmado na documentacao antes de escrever: o
evento existe, o stdout dele **entra no contexto**, corre em shell (logo
`$(...)` funciona), e `.claude/settings.json` na raiz e o ficheiro versionado
certo.

### E a medicao desmentiu a premissa

O plano dizia para correr o verificador **antes** de o fixar e listar o que ja
estava errado — «e o dado de quanto isto custou». Corri. A primeira versao
devolveu **86 divergencias em 544 datas**, e eu quase as reportei como o custo.

Partidas por tipo, eram outra coisa:

| | |
|---|---|
| **85 ANTIGA** | datas *anteriores* ao commit que as introduziu |
| **1 FUTURO** | `10/12` em `sacrificaveis 10/12` — uma **fraccao** |
| **0 FUTURA** | a deriva que o plano temia: **nenhuma vez em 544** |

As 85 «ANTIGA» eram todas legitimas: um documento escrito a 05/09 que regista
um evento de 23/08 diz correctamente 23/08. **Documento que regista historia
tem datas para tras — e isso e o caso normal, nao o defeito.**

> **Uma regra que grita no caso normal treina quem a le a ignora-la.** E o
> numero que ela produz nao e um custo: e o barulho dela propria.

Depois de tirar as duas classes de falso positivo — fraccoes (`n/m` com `m`
impossivel como mes desliga a leitura de `DD/MM` naquela linha) e a direccao
«para tras» — o repositorio da **540 datas, zero divergencias**.

### O que isso muda no valor da guarda

Ela **nao esta a corrigir nada** — nao havia o que corrigir. Esta a impedir. E
essa distincao vale ser dita: o hook e o verificador transformam «acertamos
porque calhou» em «nao pode falhar», e isso continua a valer o trabalho. Mas
anunciar 86 defeitos onde ha zero teria sido a terceira vez no mesmo dia que eu
sobre-afirmo a partir de dados a menos.

### Provada a dizer nao, nas duas direccoes

- **FUTURO**: cabecalho do `STATUS.md` sabotado para `2026-09-20` -> apanhado.
- **FUTURA**: num repositorio descartavel, linha commitada a 01/09 a dizer
  `2026-09-04` -> apanhado como «3 dias antes».
- E a linha de controlo, `2026-08-20` atras do mesmo commit, **nao** dispara.

⚠️ **A data desta cicatriz veio do `date`**, passada por argumento ao script que
a escreveu. Nao a escrevi a mao — seria a primeira coisa a contradizer.


---

## 15co — A ferramenta que nao ve o plano corta o plano (2026-09-07)

O dB perguntou se eu podia rotear em vez dele. Podia — e o caminho ate ao numero
final passou por quatro erros meus, cada um apanhado pelo DRC e nenhum por mim.

| tentativa | resultado |
|---|---|
| tres percursos triviais | 31 de 102, limpas |
| + A* | 52 de 71, e **109 violacoes** |
| obstaculos consertados | 27, e **5 violacoes** |
| margem de meia diagonal | 27, e **2** |
| ferramenta a retirar o que viola | 28, e **0** |
| **aplicado na placa** | **52 por ligar, mas `AGND` de 1 para 8** |

A ultima linha e a que interessa. Geometria a zero, DRC limpo de clearance e de
cruzamentos — e mesmo assim **pior**. O relatorio dizia-o na primeira linha:

    Zone [AGND] on F.Cu <-> Zone [AGND] on F.Cu

Duas partes da MESMA ilha, sem ligacao entre elas. As pistas atravessaram-na e
o cobre recuou de cada lado: a ilha ficou em pedacos.

> **Uma pista que atravessa um plano nao passa por cima dele — corta-o.** Numa
> placa em que a terra e plano e nao pista, um roteador cego a zonas destroi
> exactamente aquilo que a revisao de terra passou a tarde a proteger.

E o DRC nao me travou porque, do ponto de vista dele, nada estava errado: as
pistas cumpriam folgas, nao se cruzavam, nao tocavam em pad nenhum. **O defeito
nao estava no que a ferramenta fez, estava no que ela nao sabia existir.**

### O que salvou

Duas coisas, e nenhuma foi perspicacia:

1. **Copia antes do destrutivo.** `/tmp/backup2.kicad_pcb` foi feita no mesmo
   comando que aplicou. Reverter custou uma linha.
2. **Comparar o numero certo.** Se eu tivesse olhado so para «por ligar: 71 ->
   52» tinha anunciado sucesso. O que denunciou foi olhar **por rede** — e ver o
   `AGND` a subir enquanto o total descia.

> **Um total que melhora pode esconder uma parte que piora.** A decomposicao nao
> e refinamento do relatorio: e onde o defeito vive.

### O que fica

O `roteia.py` fica com o que provou funcionar — percursos triviais e A*, com a
ferramenta a **retirar o que o DRC acusa** em vez de o entregar. Mas fica com um
aviso em maiusculas no cabecalho: **nao usar em placas com plano na mesma camada
das pistas**, ate saber ler zonas.

E as 71 do cerebro voltam para o dB, que e onde estavam. Nao se perdeu nada
excepto o meu tempo, e ficou a saber-se uma coisa que nao se sabia.

## 15cp — o roteador não precisava de ficar esperto; o desenho é que estava errado

*07/09.* A cicatriz anterior (15co) terminou com o roteador a cortar os planos e
com uma pergunta: **como é que se ensina um roteador a ver zonas?** O dB
escolheu que se tentasse. A resposta certa era que a pergunta estava errada.

O cobre estava nas **duas** camadas — GND e a ilha de AGND em F.Cu *e* em B.Cu.
Um dois-lados só tem duas camadas. Se ambas são plano, **não sobra camada para
sinal**, e então toda a pista atravessa um plano. E atravessar um plano é
cortá-lo.

Nenhuma heurística resolve isso, porque não há caminho bom: o roteador estava a
ser-lhe pedido que passasse por um sítio onde não cabia nada. O conserto foi o
arranjo normal de qualquer dois-lados — **plano sólido em B.Cu, sinal em F.Cu,
e uma via em cada pad SMD de terra a descer ao plano**. A pergunta "como é que
ele evita cortar o plano" deixou de existir: não há plano no caminho dele.

> **Meia sessão de roteador esperto, ou vinte linhas de desenho certo.** Quando
> uma ferramenta parece precisar de ficar mais inteligente, vale a pena
> desconfiar do desenho antes de desconfiar da ferramenta — o esforço é uma
> ordem de grandeza menor e o resultado é fiável em vez de provável.

Três defeitos meus a caminho, e os três são da mesma família — **afirmar
ligação onde só havia proximidade**:

- **A via ao lado do pad não está ligada ao pad.** 19 `via_dangling`, e
  0,9 mm de vazio isola tão bem como 9 mm. Cada via leva um coto.
- **O teste de espaço da via olhou pads e ignorou pistas.** Duas vias a 0,10 e
  0,15 mm de pistas que o dB já tinha roteado à mão. O que já lá está é
  obstáculo, tenha vindo de onde vier.
- **O extractor do DRC casava a linha do cabeçalho**, onde o nome da rede não
  está — vem nas duas linhas seguintes. Devolveu-me **zero achados com ar de
  placa limpa** contra um DRC que dizia 47, e eu quase segui. Virou
  `le_drc.py`, que lê por rede.

E uma quarta, que é a mais insidiosa: **a mesma placa deu-me dois números
diferentes em duas medições e eu procurei a causa na placa.** O `--save-board`
reformata o ficheiro, e o meu medidor por expressão regular com janela fixa
passou a alcançar campos que antes não alcançava. Medição por regex, num
formato que a ferramenta reescreve, não é medição.

Medido no fim, com as zonas cheias:

```
cerebro   71 -> 47 por ligar · planos em pedacos 0
painel    32 -> 14 por ligar · planos em pedacos 0
```

## 15cq — o estrago tinha duas formas e eu só procurava uma

*07/09.* Ensinei o roteador a usar as duas camadas, e pus-lhe um guarda: depois
de escrever, medir se algum plano se tinha partido, e retirar as travessias
culpadas. O guarda procurava `Zone [X] <-> Zone [X]` — o plano em ilhas — que
era a forma do estrago da véspera (15co).

Correu, disse **planos em pedaços 0**, e gravou. O total de ligações por fazer
caiu de 47 para 20. E o `GND` subiu de 0 para 5.

A pista em B.Cu não tinha partido a zona: tinha passado **entre um pad e o
cobre**, deixando o pad órfão. O plano continuava inteiro. A terra é que não. E
o sintoma aparecia como uma ligação por fazer normalíssima, no meio das outras.

> **Uma lista de sintomas conhecidos protege contra o que já aconteceu; um
> invariante protege contra o que ainda não aconteceu.** Troquei a lista por
> um invariante: mede-se quantas ligações faltam em cada rede antes e depois, e
> a regra de aceitação passa a ser uma só — **nenhuma rede pode piorar**. Não há
> sintoma novo que passe por não estar na lista, porque não há lista.

E uma segunda, da mesma sessão e da mesma família: **isentar pelo nome em vez
de pelo defeito.** A placa chegou com uma via solta em `AGND`, que não era
minha, e eu isentei `AGND` inteira — deixando passar uma folga de 0,193 mm
noutra pista da mesma rede. A assinatura de uma isenção é o par (tipo, rede).

## 15cr — o defeito de ordem que passa em todos os testes

*07/09.* A costura das vias corria **antes** da colocação das peças: lia a
posição de cada pad para saber onde pôr a via, e lia-a antes de as peças serem
movidas. Nunca deu erro. A placa já chegava colocada, a segunda corrida é
idempotente, e os números batiam sempre.

Só apareceu no dia em que mudei uma peça de sítio. A ferramenta continuou a
falar da posição antiga, e nada se queixou.

E ao lado dele, um irmão: a peça que eu queria mover estava numa **grelha** e
também na tabela de **posições explícitas**. A tabela ganhava — até ao passo
seguinte recalcular a grelha e a devolver ao sítio antigo, em silêncio. Editei,
o ficheiro não mudou, e nenhum passo o disse.

> **Um defeito de ordem que só aparece quando algo muda passa em todos os testes
> menos no dia em que interessa.** E uma colocação que se desfaz sem dizer nada
> é pior que uma que falha. Nem um nem outro se resolve com cuidado: resolvem-se
> com a ordem certa, e com um portão que recusa arrancar se uma referência
> aparecer nas duas listas.

## 15cs — a ferramenta que recusa encontrou o que ninguém foi procurar

*07/09.* Pus a costura a recusar-se a descer uma via para um plano que não
passasse por baixo do pad — e a **dizer de quem era o pad**. Achei que estava a
tapar um caso de borda.

O pad era do `NT1`: o net-tie que é a **única ponte entre a terra analógica e a
digital**. Estava a 57 mm da ilha que devia unir, no meio do plano digital.

A topologia até estava certa — continuava a ser uma ponte só, que é a regra
inteira do assunto. Mas toda a corrente de retorno analógica voltava por uma
pista de 0,25 mm em vez de por cobre largo, e **uma ponte com impedância é meia
ponte**. A ponte passou para a fronteira da ilha, com um pad de cada lado.

> Ninguém tinha ido procurar isto, e nenhuma revisão o tinha visto. Apareceu
> porque uma ferramenta pequena se recusou a fazer uma coisa sem sentido **e
> explicou porquê**. Uma ferramenta que falha em silêncio, ou que faz o que lhe
> pedem sem perguntar, não teria encontrado nada.

## 15ct — apertar a folga piorou o resultado, e eu mudei duas coisas de uma vez

*07/09.* O roteador usava 0,35 mm de folga com a regra da placa em 0,20 — quase
o dobro. Pareceu-me óbvio que apertar abriria caminhos. Apertei a folga **e**
refinei a malha, na mesma corrida.

O resultado piorou de 23 ligações por fazer para 44, e a medição não sabia
dizer qual das duas mudanças tinha feito o estrago. Separadas:

```
folga 0,35 · malha 0,25  ->  23     <- fica esta
folga 0,26 · malha 0,25  ->  34
folga 0,26 · malha 0,20  ->  44
```

As duas eram más, por razões diferentes. A malha fina não acha mais caminhos:
acha os mesmos mais devagar, e bate no tecto de nós antes de chegar ao fim. E a
folga apertada alimenta a ganância — com mais espaço o roteador enfia mais
pistas cedo, e essas pistas fecham corredores de que as ligações seguintes
precisavam.

> **Um roteador guloso melhora quando se lhe aperta o apetite, não a folga.** E
> uma variável de cada vez, mesmo quando parece óbvio que andam juntas — porque
> quando andam juntas e o resultado piora, não se aprende nada.

## 15cu — duas escritas concorrentes não dão erro, dão um número

*07/09.* Deixei uma comparação de ordens de roteamento a correr em segundo
plano e, sem esperar por ela, lancei a cadeia completa. As duas escreviam os
**mesmos** `.kicad_pcb`, durante meia hora, por cima uma da outra.

Não houve excepção, nem ficheiro corrompido, nem aviso. Houve um resultado:
**34 violações de geometria** numa placa onde nenhuma das duas corridas, sozinha,
produzia mais do que zero. Se eu não tivesse reparado que era impossível, tinha
esse número na mão como facto.

> **Uma medição contaminada tem exactamente o mesmo aspecto de uma boa.** Não
> traz marca, não traz aviso, e entra num relatório com a mesma cara que as
> outras. É a família da 15co — o número que melhora enquanto a coisa piora —
> mas por uma via nova: não é o instrumento que mede mal, é o objecto que muda
> por baixo do instrumento.

A defesa não é lembrar-me de esperar. É um **ferrolho**: a ferramenta recusa
arrancar se outra já estiver a escrever as mesmas placas, e diz o pid da que
está. Custou dez linhas.

E há um corolário sobre trabalho em segundo plano: **lançar um segundo trabalho
sobre os mesmos ficheiros é uma escrita, não uma leitura.** Tratei-o como se
fosse inofensivo porque eu não estava a tocar em nada — mas o processo que eu
tinha deixado a correr estava.

## 15cv — o glob que abortou a linha, e as três medições que eram a mesma

*07/09.* Escrevi uma alteração ao roteador, encadeada assim:

```
pkill … && rm -f .lock cotuca_*/*.melhor && python3 - <<'PY' … PY
```

Não existia nenhum `*.melhor`. Em zsh, **um glob sem correspondência não é um
aviso: aborta a linha inteira**. O `rm` falhou, o `&&` cortou, e a edição nunca
correu. A mensagem `no matches found` saiu no meio de trinta linhas de saída de
outro comando, e eu li-a como ruído.

O que se seguiu foi pior que o erro. Corri três experiências para escolher uma
margem, obtive **25 · 134 s · 2 devolvidas** nas três, e por um momento
acreditei nelas. Só as reconheci como impossíveis porque batiam ao segundo:
três condições diferentes não dão o mesmo tempo de parede.

E entre uma coisa e outra tirei duas conclusões — "o conserto geométrico
funcionou, 76 s contra quinze minutos" e "mas piorou para 78" — **sobre código
que não estava no ficheiro.** Ambas foram para o chat como facto.

> **Um comando encadeado com `&&` tem de ser lido do fim para o princípio: o que
> interessa é o último elo, e é o mais fácil de perder de vista.** Quando o que
> se está a medir é o efeito de uma edição, a edição confirma-se no ficheiro —
> não na saída do comando que a devia ter feito.

E o sinal que salva: **resultados idênticos entre condições que deviam diferir
não são um resultado, são uma avaria do aparelho.**

## 15cw — a bissecção "a sério" era pior nos dois eixos

*07/09.* O comentário dizia bissecção e o código fazia varrimento linear — uma
rede de cada vez, um DRC por rede. Vi o comentário, achei que estava a corrigir
uma preguiça antiga, e escrevi a versão que parte ao meio.

Ficou pior nos dois eixos que interessam:

- **mais lenta**: com 5 culpadas em 33, descer recursivamente nas metades que
  falham custa mais testes que percorrer a lista uma vez. A bissecção paga-se
  quando há **uma** culpada;
- **pior resultado**: 25 ligações por fazer contra 19, porque o recuo de que ela
  precisa para o caso das culpadas-em-conjunto retira **todas** as candidatas de
  uma vez.

> **Um algoritmo melhor na notação pode ser pior na máquina, e "melhorar" uma
> parte que ninguém mediu é uma aposta, não um conserto.** O varrimento estava lá
> por uma razão que eu não tinha procurado antes de o substituir.

E uma terceira, da mesma corrida: a cadeia **ignorava o código de saída** do
roteador. Ele recusa gravar quando não consegue provar o que fez, e repõe a
placa — correctamente. Mas uma passada falhada era indistinguível de uma passada
que não tinha nada a fazer, e a cadeia seguia calada com outro número no fim.

## 15cx — zero pistas à volta não é espaço livre: era uma regra minha

*07/09.* O roteador tinha parado em 22 ligações no cérebro e 10 no painel, e a
minha explicação era congestionamento — a zona à volta do amp-op, a placa cheia.
Fui medir em vez de continuar a acreditar nisso, e a medição disse o contrário:

```
OLED_CLK  em (40.0, 22.5) ·  0 pistas a menos de 8 mm
OLED_DIN  em (42.5, 22.5) ·  0 pistas a menos de 8 mm
```

Zero. E as falhas não estavam espalhadas: concentravam-se em `J14` (9), na
Daisy (11), no `J1` do painel (8) — **todas peças de duas filas de pinos**.

O que as fecha é aritmética, não densidade. Num header de 2,54 mm com pads de
1,70, o vão entre dois pads vizinhos é **0,840 mm**. O meu roteador exigia
`largura/2 + folga + margem-da-grelha` de cada lado — 0,125 + 0,35 + 0,178 —
que dá **0,950**. Não cabe, e não cabia em sítio nenhum do conector: ele não
conseguia **sair de entre os pinos**.

> **Um sítio sem nada à volta que a ferramenta não alcança não está
> congestionado: está fechado por uma regra da própria ferramenta.** E a regra
> era minha e generosa — 0,35 mm de folga quando a placa manda 0,20.

O conserto é uma **passada apertada no fim**, e só no fim: apertar desde o
início mede-se pior (23 → 34), porque com mais espaço o roteador ganancioso
enfia pistas cedo e fecha corredores de que as ligações seguintes precisavam.
No fim não há esse risco — o que já foi roteado já lá está, e a folga apertada
só serve para entrar onde a larga não entrava.

E um segundo facto, do mesmo cálculo e na direcção contrária: **entre dois
pinos de conector não cabem a pista e o plano ao mesmo tempo.** São precisos
0,775 mm e há 0,42. Uma pista em B.Cu ali não divide o espaço com o cobre de
terra: expulsa-o, e os pinos de terra do conector ficam órfãos. Eram as três
redes que a retracção me devolvia a cada corrida, uma a um DRC cada — e agora
são recusadas de graça, na geometria.

## 15cy — a retracção que não alcança tudo o que escreveu

*07/09.* A retracção interrogava só as redes que tinham passado pela camada do
plano, porque a minha teoria do estrago era essa. Numa passada apertada, o
estrago veio de outro lado, a lista de suspeitos não o continha, e a ferramenta
**desistiu de gravar um trabalho quase todo bom** por causa de uma pista que
nem sequer sabia acusar.

> **Uma ferramenta que se recusa a entregar o que não consegue provar precisa
> de poder investigar tudo o que fez.** Senão a recusa deixa de ser prudência e
> passa a ser um ponto cego com boas maneiras.

E o filtro de suspeitos usava a folga da corrida em vez da regra da placa: numa
passada apertada a folga encolhe, o filtro encolhe com ela, e devolveu
"0 de 3 suspeitas" numa corrida em que havia estrago. **A referência de um
filtro é a propriedade física, não o parâmetro do momento.**

## 15cz — um conector de duas filas encostado ao bordo tem metade dos pinos

*07/09.* Os cinco sinais do OLED falhavam nas duas placas, e o diagnóstico dizia
**zero pistas num raio de 8 mm** à volta deles. Não era congestionamento.

O `J14` estava em y=25, com a fila da frente em y=22,5 e o bordo da placa em
y=20. Entre os pads e o bordo sobrava um canal de **1,05 mm** — cabe uma pista.
Treze sinais não cabem numa pista. E sair pelo outro lado obrigava a passar
**entre** dois pads do conector, onde o vão é 0,840 mm e a pista precisa de
0,950.

> **Um conector de duas filas precisa de canal de fuga dos dois lados.
> Encostá-lo a um bordo é dar-lhe metade dos pinos** — e a outra metade fica
> presa sem que nada no desenho o denuncie.

O conserto foi descer o conector 4 mm, e para isso descer 2 mm o bloco do
pré-amp que estava logo abaixo. Dois milímetros ali pagaram quatro aqui:
**18 ligações por fazer contra 21**, sem tocar em mais nada.

⚠️ E a primeira tentativa colidiu: procurei a nova posição medindo distâncias
entre **pads** e dei 2,49 mm de folga; o DRC devolveu dois `courtyards_overlap`.
**O que reserva espaço num footprint é o pátio, não o pad** — e o pátio do J14
chega a y=18,8, que já é fora da placa. É a mesma família do varrimento que pôs
uma peça em cima de um electrolítico por medir os pads de 1 mm e ignorar o corpo
de ⌀16.

## 15da — o que faltava não era o caminho, era o primeiro milímetro

*07/09.* Depois de mover o conector, sobravam 18 ligações no cérebro. Fui ver
onde acabavam, e **todas terminavam num pad de passo fino**: os pinos do `U1`,
um SOIC de 1,27 mm, e os do cabo plano. As pistas chegavam a cinco milímetros
do pad e paravam.

Não era falta de espaço na placa. É que **o primeiro milímetro a sair de um pad
de passo fino não cabe na folga generosa que serve para o resto do percurso** —
e essa folga era minha, 0,35 mm contra os 0,20 que a placa manda.

Já sabia que apertar a folga em toda a parte mede-se pior (23 → 34): o roteador
guloso enfia pistas cedo e fecha corredores. A saída era apertá-la **só nos
primeiros milímetros à volta das duas pontas** — ali não há corredor a fechar,
há um pad de onde é preciso sair.

```
cerebro  18 -> 12 por ligar · geometria 0 · planos inteiros
```

> **Um parâmetro que é mau em toda a parte pode ser exactamente certo num
> sítio.** Passei três experiências a tratar a folga como um número global,
> medindo-a alta e baixa, quando a resposta era que ela não devia ser um número
> global de todo.

## 15db — arrancar e refazer: a capacidade que faltava, e não estava pronta

*07/09.* Depois de o roteador parar em 12 ligações, testei tudo o que era
afinação — malha, folga, tecto de nós, ordem de ataque, um corredor achado numa
malha grossa. Nenhuma delas rendeu mais de duas ligações, e três renderam
menos. Todas partiam do mesmo pressuposto: **que existe caminho livre e falta
achá-lo.**

Para as últimas não existe. O cobre que já lá está ocupa o único sítio por onde
elas podiam passar, e nenhuma busca acha um caminho que não há.

O que um roteador a sério faz é o óbvio depois de dito: **arranca o que estorva,
passa, e volta a rotear o que arrancou** — que agora tem de dar a volta, e quase
sempre pode, porque tinha alternativas e a outra não. Implementado, numa passada
isolada levou as ligações por fazer de 15 para 8.

Na cadeia inteira mediu **14 contra 12**. E uma corrida entregou a placa com um
**plano de terra partido** que o auto-teste do roteador não apanhou — foi o
portão da fábrica que o viu.

> **A última linha de defesa não pode ser a mesma ferramenta que fez o
> trabalho.** O roteador verifica-se a si próprio e é bom que o faça, mas o
> portão existe porque um auto-teste tem exactamente os pontos cegos de quem o
> escreveu — e este tinha.

Ficou em `--arranca`, fora da cadeia. **Uma capacidade que fecha ligações e
parte planos não está pronta para correr sozinha**, por mais que seja a certa.

E a lição de método, que é a que se repete há três rondas: *quando cinco
afinações seguidas rendem uma ou duas cada e oscilam entre corridas, já não se
está a melhorar nada — está-se a ajustar ruído.* O sinal para parar de afinar e
mudar de ideia é esse, e eu passei por ele mais do que uma vez.

## 15dc — o auto-teste contava o total e deixava um estrago mascarar o outro

*07/09.* A regra de aceitação do roteador era boa: mede-se quantas ligações
faltam em **cada rede** antes e depois, e nenhuma pode piorar. Substituiu uma
lista de sintomas precisamente para não haver sintoma novo que passasse por não
estar na lista (15cq).

Passou um. Se um pad de `GND` se liga (−1) e ao mesmo tempo o **plano se parte**
(+1), o total da rede não muda, e a regra vê zero. A placa saiu com a terra em
pedaços e o auto-teste disse que estava boa — quem a apanhou foi o portão da
fábrica, que conta as espécies em separado.

```
antes  GND = 1   (um pad solto)
depois GND = 1   (o pad ligou-se, o plano partiu-se)
piorou -> NADA
```

> **Um invariante protege contra o que ainda não aconteceu, mas só na dimensão
> em que foi medido.** Contar por rede protege contra a rede errada; não
> protege contra a espécie errada dentro da mesma rede. A chave passou a ser
> **(rede, espécie)** — e um plano partido não é "mais uma ligação por fazer"
> que se compensa com outra qualquer: é uma espécie própria, com contador
> próprio.

⚠️ **E é literalmente a mesma família da assinatura `(tipo, rede)` das
violações**, que eu tinha corrigido três horas antes, no mesmo ficheiro, por
ter isentado uma rede inteira por causa de um defeito só (15cq). **Uma lição
aprendida num sítio não se aplica sozinha ao sítio do lado** — e eu li o código
corrigido dezenas de vezes sem ver o gémeo por corrigir ao lado.

Ficou um `roteia.py --auto-teste` que interroga a regra com seis casos, o
primeiro dos quais é exactamente este. Provei que tem dentes: com a chave
antiga, falha.

> **Um verificador que não é verificado é uma opinião com ar de medição.**

## 15dd — arrancar para dar passagem pode trocar uma ligação por outra

*07/09.* O arrancar-e-refazer tirava do caminho as pistas que estorvavam e
devolvia-as à fila. Se a rede arrancada não conseguisse refazer-se, o resultado
era uma troca — e às vezes um prejuízo, porque a arrancada **estava feita** e a
outra continuava por fazer.

Era isso, e não a ideia, que fazia a cadeia medir 14 contra 12.

> **Uma operação que desfaz trabalho para tentar mais trabalho precisa da mesma
> regra que tudo o resto: só vale se o número descer.** Guarda-se o estado antes,
> e volta-se atrás se não desceu.

Com a guarda mede 12 e não parte planos — igual a não arrancar nesta placa, e só
pode ajudar noutra. **Uma capacidade que só pode melhorar ou não fazer nada
pertence ao caminho por omissão**; antes das duas correcções não pertencia.
