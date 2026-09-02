# STATUS — Cotucatronics
<!-- doc-check: ignore route_exciter (nome APOSENTADO em 28/08, hoje `out_l`. A mencao e o registo da renomeacao — apagar o nome velho apagaria o que explica por que ele estava errado. Ver battle_scars 15e.) -->
<!-- doc-check: ignore route_jack (idem, hoje `out_r`) -->

**Onde estamos.** O par deste documento é
[battle_scars.md](battle_scars.md), que diz *o que aprendemos chegando
aqui*. Leia os dois antes de tocar em código.

> Atualizado em 2026-09-02 (fim do dia) · leva: **a marca da abertura
> encolhida e subida** a pedido do dB — folga de 3 para 6 linhas até ao
> nome · o **PS_2 medido** e o mapa do prescaler fechado · o **espectro a
> 7 kHz** medido (o framebuffer não lhe mexe, mas o chão de banda larga
> sobe 5,4/6,9 dB) · **20 órfãos da caixa-preta a 0** · e todos os
> verificadores a zero
>
> Antes, 2026-09-02 · leva: **o travamento com a ponte, fechado na
> raiz** — faltava um `GPIO_PULLUP` no RX do MIDI · **vigia de laço** que
> reinicia a placa sozinha · **caixa-preta consertada**: 20 órfãos e uma
> colisão de endereços · e a regra que organiza o resto, **15az**
>
> Antes, 2026-09-01 · leva: **a abertura do OLED** viva (jogada de
> búzios → marca → nome → versão, 50 quadros a 20 Hz) · **framebuffer inteiro
> por SPI a PS_8**, 3 811 µs/quadro, verificado no vidro · fonte 5×7 gerada
> de glifos legíveis · o enfeite que cobrava **13% ao laço parado**
>
> Antes, 2026-08-28: **matriz de roteamento 2×2** medida ·
> **OLED vivo** e três encoders a contar · **livelock do MIDI consertado** ·
> `in_gain` removido · **tarolator**, efeito novo nascido de uma medição

---

## Rotina de operação: os dois destinos, sempre juntos

**Toda nova versão de `STATUS.md` ou `battle_scars.md` vai para os DOIS
destinos na mesma ação:**

| Destino | Caminho | Forma |
|---|---|---|
| Repo | `docs/` | cru, sem frontmatter |
| Vault | `~/Documents/Obsidian Vault/Cotucatronics/` | com frontmatter YAML + tags |

Os dois servem leitores diferentes e nenhum substitui o outro. O **repo**
é o que sobrevive ao clone limpo — documento citado que não existe no
clone é a mesma classe do `module_rnbo_alloc.h` que faltou por semanas.
O **vault** é onde se lê, linka e pensa entre sessões, com tags e
backlinks que o Markdown cru não tem.

Nunca deixar um destino pra depois: o par desalinha em silêncio, e o
desalinho só aparece quando alguém precisa da versão certa.

---

<!-- doc-check: ignore system_profiler (comando do macOS, nao simbolo do projecto) -->

## 02/09 — o dia em que um pino sem pull-up custou cinco horas

**A causa era uma linha.** A libDaisy inicia os pinos da UART com
`GPIO_NOPULL`; com a ponte ESP32 sem alimentação o RX (PB7) ficava a
flutuar, e uma entrada CMOS solta apanha ruído a MHz. A USART entrava em
tempestade **por taxa**, o CPU ficava na interrupção e o laço morria à
fome — em menos de 4 s, e sem voltar quando a linha sarava.

`g.Pull = GPIO_PULLUP`. **Verificado: 7/7 sondagens com a linha a flutuar
e todos os contadores a ZERO** — a falha não é sobrevivida, não acontece.

### O que sobreviveu como defesa, e porquê

| defesa | porque fica |
|---|---|
| tecto e recuo no rearme do `DmaListen` | a linha **presa em baixo** ainda o exercita (deu `rearmes 2`), e sem ele corria ao ritmo do laço — **10 029 rearmes com a ponte sã** |
| vigia de laço no callback de áudio | requisito de campo: peças em série, ninguém carrega num botão por elas. Reinicia se o laço não completar uma volta em 48 000 amostras |
| USART1 em prioridade 1 | estavam **todas** em `(0,0)`, e prioridade igual não permite preempção: o abort do DMA nunca completava. O agrupamento virou **invariante que a placa confere e grita** |

Saíram duas que fingiam defender: limpeza de bandeira no callback (inútil
contra tempestade por taxa, e corrida de contexto com a própria ISR) e
tecto de erros no `PollMidi` (fase 2, para uma falha que mata na fase 1 —
só corria enquanto o problema não existisse).

### A caixa-preta estava a mentir de duas maneiras

Varredura de órfãos nas duas direcções: **14 slots lidos e nunca escritos**
(zero por construção) e **6 escritos e nunca lidos**. Um deles era a
pergunta central da sessão — o slot 30 lê-se como *"laço PRESO?"* e
**ninguém o escrevia**.

E pior que órfão: **colisão de endereços.** O bloco "PICO POR NOTA" lia
`32..44`, mas 33–39 são contadores de MIDI e 40–44 o diário por minuto —
imprimia `midi.Bytes()` disfarçado de pico de áudio.

**20 órfãos → 0.** O registador de voo também estava preso a `if(voando)`
(`buzios_on`, que nasce a zero após cada gravação). Verificado:
`blocos 14 080 · CPU 29% · voltas 218 780 · laço não ficou preso`, e os
três **fecham entre si**.

### A regra que organiza as outras

**15az** — conserto adiado não fica pequeno: fica a ser o **equipamento da
próxima emergência**. Nenhum dos nove defeitos encontrados foi causado
pelo pull-up, e nenhum cresceu. O que mudou foi que passaram a ser usados,
e todos eram instrumento, ferramenta ou relatório.

Contrapeso: consertar agora não é consertar tudo o que se vê — é consertar
**o que a tarefa de agora está a usar**. Numa caçada, isso é o instrumento.

**A sessão de escuta ganhou um argumento novo** (dB, 02/09): o aviso
"gravar zera os params" é repetido desde agosto e gerido por memória. O
desenho dos presets já tem a resposta — **global slot 0 = estado de boot**
— e no dia em que a biblioteca existir, gravar deixa de zerar: carrega o 0.
A escuta não é só o estrangulamento mais antigo, é o que **aposenta esse
aviso**. Ver [PRESETS-LISTA.md](operacao/PRESETS-LISTA.md).

~~**Aberto:** os ~5% de base do laço~~ — **FECHADO em 02/09, e não
existiam.** Dois builds inertes mostraram que a primeira medição depois de
uma gravação dá ~15 000 voltas/s e as seguintes ~22 800. Descartada a
primeira, os builds diferem **0,27%** contra **1,3%** de dispersão do
próprio instrumento. Era **aquecimento** — e a regra estava escrita na
skill `medicao-limpa`, item 17. Ver battle_scars **15be**.

---

## 01/09 — o painel passou a desenhar, e o custo ficou medido

**O que existe agora.** O OLED deixou de ser duas linhas de texto e passou a
receber **o framebuffer inteiro** por `hal::OledDesenha(fb)` — uma entrada só,
com a fronteira do HAL intacta. Em cima disso, três peças de C++ **puro**, sem
hardware nenhum: `desenho.h` (primitivas), `fonte5x7.h` (43 glifos, gerados de
arte legível por `fonte5x7.py`) e `splash.h` (a abertura). Tudo isto vê-se
inteiro no computador antes de haver vidro, e o Bresenham é **inteiro**, então
o que sai no painel é bit a bit o que saiu no teste em terra.

**A abertura.** Jogada de búzios que entra pelo topo e abre em leque (1,05 s),
**fica assente** 0,45 s, uma das conchas que caiu sobe e cresce até virar
marca, o nome entra letra a letra, a versão assina. 50 quadros a 20 Hz, 2,50 s.
Corre **depois** do `StartAudio` — o instrumento já soa enquanto a jogada cai —
e apaga-se 2 s depois de acabar, porque painel aceso custa 6 dB de chão de
ruído e assinatura acesa para sempre cobrava esse preço por um enfeite.

Repete-se sem gravar: `splash` e `splash curto`.

**Os números, e os que faltam.**

| | medido |
|---|---|
| Quadro a PS_64 | 23 401 µs |
| Quadro a PS_16 | 6 945 µs |
| **Quadro a PS_8** (por omissão) | **3 811 µs** — custa 7% do laço a 20 Hz |
| Quadro a PS_4 | 2 023 µs |
| Custo fixo, deduzido dos dois pontos rápidos | ~300 µs + ~440 µs por unidade |

PS_8 e PS_4 desenham **limpo**, confirmado no vidro com padrão de 1 px. PS_8
é o escolhido, não PS_4: os dois servem, mas PS_8 tem uma oitava de folga
**provada** por baixo, e margem medida vale mais que os 3% que custa. **PS_2
não foi medido** e fica declarado como desconhecido.

**Três dívidas abertas, escritas como dívidas:**

1. ~~A semente da jogada de arranque é fraca~~ — **PAGA em 01/09.** A jogada
   passou a ser semeada do **ruído do próprio ADC**, colhido durante 150 ms
   antes de a animação começar. Quatro arranques deram `AB25A781`, `771E58D9`,
   `6E671930`, `190173E9` — e **917 leituras distintas** do conversor em cada
   colheita. Esse número tem sentido físico: 917 em 150 ms são **6 113 Hz**,
   a taxa de conversão do ADC. É a melhor prova de que a entropia é real e não
   fabricada — bate com um relógio de hardware que ninguém escolheu.

   A qualidade **é dita, não presumida**: `board` imprime a semente e a
   contagem, e grita `<- ENTROPIA FRACA` se cair abaixo de 8 leituras
   distintas (que é o que aconteceria se o ADC estivesse morto). Gémeo
   consultável, porque a linha de arranque sai 150 ms depois do `StartAudio`
   e o `reboot` derruba a USB — quem reconecta a seguir nunca a veria.
2. **Faltam 7% de base do laço.** O `noinline` recuperou 8 dos 13 pontos
   perdidos; os outros não têm explicação e não são arredondamento.
3. ~~A abertura do despertar~~ — **RESOLVIDO pelo dB em 01/09**, e a decisão
   deu à animação um emprego melhor do que enfeite. Ver abaixo.

### A decisão do dB: três estados, e a jogada passa a ser instrumento

> *"a abertura corre quando ligo o aparelho · o oled funciona normalmente como
> tínhamos estabelecido para os presets, sem a abertura · quando ativarmos os
> búzios a animação volta mas sem a logo"*

| quando | o que o vidro mostra |
|---|---|
| Arranque | **abertura completa** — jogada, marca, nome, versão (50 quadros), e a marca fica **3 s** no vidro |
| Uso normal | o painel de presets, **imediato**, sem animação nenhuma |
| Búzios disparam | **só a jogada** — 30 quadros, sem concha a crescer, sem marca |

A terceira é a que muda a natureza da coisa: **a jogada no vidro é a jogada que
o instrumento acabou de fazer.** Deixa de ser decoração e passa a ser mostrador.

Segue o `buzios_on` que já governa o sorteio — **sem parâmetro novo**, porque um
interruptor próprio seria um segundo sítio onde a mesma decisão mora, e os dois
divergiriam. Disparo novo a meio da animação **reinicia**: são duas jogadas, e a
segunda não espera pela primeira. No fim devolve o vidro a quem o tinha, e o
`splash curto` de 12 quadros **foi removido** — existia para o despertar, que
acabou de ser cancelado.

### A jogada por búzios: funciona, e o que ela NÃO é

Confirmado pelo dB em 01/09: **funciona**. Mas custou uma volta a chegar lá, por
duas razões que ficam escritas.

**`buzios_on` é interruptor, não gatilho.** Ele arma o detector; quem atira a
jogada é a pancada. Mandar o CC pelo Loopy não dispara nada — e a expectativa
contrária é razoável, então está agora no [CCMAP](operacao/CCMAP.md) e não na
cabeça de ninguém. **Lacuna aberta:** não existe forma de atirar os búzios por
comando. Se se fizer, tem de passar pelo mesmo caminho do detector — sorteio
**e** animação — para não haver dois sítios onde a jogada nasce.

**O detector escuta pós-gate, não o piezo.** Com a cadeia fechada fica surdo.
Novo comando de serial **`buzios`** parte a pergunta em três: `disparos`,
`passaram o buzios_on`, `foram a animação`, e mostra o pico que o envelope
alcançou ao lado do limiar.

### O espectro a 7 kHz com o framebuffer a 20 Hz — MEDIDO em 02/09

Pedido em 01/09 e feito agora. Alternância respeitada, primeira leitura
descartada, piso do vigia em −80 dB.

| condição | pico | ocorrências |
|---|---|---|
| QUIETO 1 | 7017,6 Hz · **−24,9 dB** | 88 |
| 20 Hz 1 | 7018,3 Hz · **−25,0 dB** | 86 |
| QUIETO 2 | 7018,9 Hz · **−25,1 dB** | 86 |
| 20 Hz 2 | 7019,8 Hz · **−25,3 dB** | 81 |

**O framebuffer a 20 Hz NÃO mexe no espeto de 7 kHz** — ±0,4 dB, com as
ocorrências a sobrepor-se. Resultado negativo, e fica escrito.

**Mas duas coisas apareceram que não estavam na pergunta:**

**1. A frequência deriva, sempre no mesmo sentido** — 7016,9 → 7017,6 →
7018,3 → 7018,9 → 7019,8 Hz, ~0,7 Hz por corrida. Artefacto digital do MCU
estaria preso ao cristal e **não derivaria**. Isto é analógico, e
provavelmente térmico — o suspeito natural é o step-down da caça de 31/08.

**2. O `pk` de banda larga SOBE ao desenhar**: −69,6 → −64,2 dBFS no L e
−75,2 → −68,3 no R. **+5,4 e +6,9 dB.** O framebuffer adiciona ruído de
banda larga (comutação da SPI) e **não alimenta o tom de 7 kHz**. São dois
fenómenos no mesmo sinal, e só se separam porque foram medidos os dois.

**Nota de método:** a primeira tentativa passou `-95` como piso e o
instrumento usou `-25` em silêncio — ele espera **magnitude**. Os quatro
picos deram todos `−24,6/−24,7` contra um piso de `−25`: valor encostado à
fronteira é sinal de que se está a medir o piso, não o pico. Não foi
publicado.

### PS_2 — MEDIDO em 02/09, e o mapa do prescaler fecha

| prescaler | quadro | custo a 20 Hz | vidro |
|---|---|---|---|
| PS_64 | 23 401 µs | −45 % | limpo |
| PS_16 | 6 945 µs | −12 % | — |
| **PS_8** ← escolhido | **3 861 µs** | **−7 %** | **limpo** |
| PS_4 | 2 050 µs | −4 % | limpo |
| PS_2 | 1 419 µs | −4 % | **limpo** (foto) |

**PS_2 desenha limpo** — confirmado por fotografia do padrão de 1 px, não
por descrição: xadrez fino e uniforme, sem blocos nem costuras. O
periférico não parte até ao chão.

**A decisão continua PS_8, e agora com razão medida em vez de prudência.**
De PS_4 para PS_2 o relógio duplica e o quadro só cai 31 % (2 050 → 1 419
µs): a ~300 µs de custo fixo já são 21 % do quadro, e o laço não sente a
diferença — **−4 % nos dois**. PS_2 não compra nada que PS_4 já não dê, e
gasta a última oitava de margem eléctrica.

**Nota de leitura:** a foto mostrava uma banda escura no terço inferior, e
foi tratada como candidato a defeito até ser descartada — é **obturador
rolante contra a varredura do OLED**, artefacto da câmara. O vidro está
homogéneo. Verificar custou dois minutos; presumir teria custado uma caça.

---

## 31/08 — o dia do terra e da microfonia

### 01/09 — O PORTÃO DOS PRESETS ESTÁ FECHADO

**As duas decisões foram tomadas pelo dB e estão no código.** O mapa é agora
**`0xFB5A`** — e o global caiu de 11 para **9 params**, que são os dois
`post_level_*` a sair.

**1 · `post_level_l/r` fora do preset.** Razão dele: *«seguem os pots físicos;
se o preset os restaurar, firmware e cobre discordam sem ninguém saber qual
manda. E cada corpo da série tem ganho de exciter próprio.»* Mesma categoria do
`exc_on` e do `gov_ceiling` — **facto de exemplar, não intenção musical**.

**2 · Família do tom renomeada:** `test_tone_freq · mix · out · via`. Uma regra
de exclusão cobre as quatro (eram duas, e a terceira a nascer escaparia às duas).

<!-- doc-check: ignore test_tone_on (nome PROPOSTO e nunca criado — registo da decisao) -->

⚠️ **O nome passou por três, e o último é o que fica: `test_tone_level`.**
Primeiro `test_tone_mix`, porque o código faz um crossfade a sério
(`b*(1-m) + tom*m`, e em 0.5 ouve-se metade de cada). Depois `gain`, depois
**`level`** — escolha do dB.

E o critério que decidiu não foi a literalidade: **o casamento widget↔param no
Loopy é por TÍTULO**, portanto quem manda no nome é o que está pintado no vidro,
não o que é exacto no código. Obrigá-lo a repintar o painel para acompanhar um
nome de código é a troca errada.

⚠️ **Consequência prática:** o widget tem de passar a **«Test Tone Level»**.
Enquanto disser outra coisa, o gerador de perfil deixa-o órfão e o fader não
manda nada — **falha silenciosa, sem erro nenhum**.

Fica registado o que ele FAZ, para ninguém escrever `>= 0.5f` contra ele: é um
crossfade, não um interruptor.

⚠️ **E o `ntc_*` passou a `notch_*`.** O dB perguntou *«o que é ntc?»* — e se o
dono do instrumento não reconhece o nome, o nome falhou. Pior neste repositório:
**NTC é um componente** (termistor de coeficiente negativo), e há folhas de KiCad
ao lado do firmware. Os CCs (82–86) não mudaram.

### A ORDEM DE TRABALHO, dada pelo dB em 01/09

| | |
|---|---|
| **1** | ~~O rename~~ ✅ **feito** — era o portão duro |
| **2** | **A SESSÃO DE ESCUTA** — o gargalo mais antigo, não depende de peça nenhuma, e é ela que constrói a biblioteca de presets |
| **3** | Em paralelo, o ERC das folhas da rev B |
| **4** | A placa **espera as peças** — footprints do step-down, decoy e botão dependem de medir o que chegar. Desenhar antes é adivinhar |

🚫 **O que NÃO fazer agora, decisão dele:** o rearme do `DmaListen`. Tem gatilho
artificial desde o Schottky, mas **não bloqueia a escuta**, e mexe no caminho
que toda a telemetria atravessa.

### 01/09 — o cobre fechou, e os presets deixaram de ser inertes

**O hardware da ponte está feito e medido pelo dB:** Schottky **1N5819** com
`VIN` a **4,72 V**, teste do gatilho passado (powerbank desligado + USB na
ponte = LED do amp apagado), e **100 µF + 100 nF** nos pinos, depois do díodo.
**A terra própria à estrela não se fez, e não faz falta** — 0 mV medidos.

**A caçada do ruído fechou com veredito:** trilho **único**. Separar as fontes
**piora**, e monotonamente — +9,6 → +17,3 → +24,3 dB em 8–16 kHz. Mecanismo: o
cabo de áudio vira o único elo de terra entre amp e Daisy. A resposta é
**encadeamento de ganho**, que a arquitectura já faz.

**As duas decisões que travavam os presets foram tomadas:** `post_level_l/r`
fora do preset, e `test_tone_*` → `test_tone_*`.

**E o CC 121 (carregar preset) passou a existir** — não existia, e os botões do
Loopy eram inertes. Canal diz o destino, valor diz o slot. O **122 (gravar) não
existe por decisão**: um CC que escreve preset destrói uma biblioteca com uma
mensagem perdida.

📄 **`LOOPY-SECAO.md`** é a folha de montagem, gerada do `manifest.h`.

⚠️ **ITEM NOVO, do dB — pente de 1000 Hz exactos**, de 1 a 23 kHz, +17 a +35 dB,
**imune às três topologias de alimentação**. 48000/48 = taxa de bloco. O teste
que decide é **bloco 48 → 96**: se o espaçamento cair para 500 Hz, é manipulação
a confirmar. `src/medir_pente.py` mede o espaçamento (o `cotuca_measure`
responde outra pergunta — dá o pico, não a distância entre picos).

### Fim de 31/08 — o instrumento está de pé

Firmware **v0.227.0**. Três coisas mudaram no fim do dia:

**Os níveis passaram a dizer a topologia:** `pre_level_ab` (antes do
governor/limiter — alimenta a protecção) e `post_level_l`/`post_level_r`
(depois do limiter — só reduzem). Não são redundantes: estão em lados opostos
da protecção. E o `pre_level_ab` **nasce a 1.0**, primeira excepção deliberada
a «todo param nasce em zero» — ele é de teste e emergência, e o piezo deve
entrar quente na dinâmica de saída.

**O `board` grita quando o master está a zero**, e explica porque o tom de teste
soa e os piezos não. Esse defeito disfarçou-se cinco vezes num dia.

**O painel OLED nasce apagado**, acorda no encoder e dorme aos 5 s — e **acende só no FIM do gesto** — roda-se o que for preciso com a tela ainda apagada, e ela acende 500 ms depois de a mão parar, sem ter mexido em valor nenhum — custava
**6 dB de chão de ruído** e um espeto a 7040 Hz. Ver `HARDWARE.md` §1b.

⚠️ **Estado que ainda se perde a cada gravação** (é o argumento dos presets):
```
set microfonia1_hz_l 770 · set microfonia1_db_l 15
set microfonia2_hz_l 834 · set microfonia2_db_l 15 · set microfonia_on_l 1
```

🔴 **A PONTE TEM DE ESTAR ALIMENTADA — defeito aberto.**

A Daisy não arranca com a ponte ESP32 sem alimentação: o `D14` (RX da USART1)
fica pendurado num chip morto, e o laço principal deixa de completar voltas.
**Sintoma de saúde mais rápido: a série abre em 0,0 s quando está sã, e leva
~59 s quando está doente.**

Testado ao fim de 31/08, e o que o teste corrigiu:

| condição | resultado |
|---|---|
| ponte alimentada | ✅ arranca, série instantânea |
| ponte sem alimentação, **com** laço de terra directo | ❌ trava na mesma |
| terra directo **ou** pela Daisy | ❌ trava nos dois |

⚠️ **A terra sozinha não cura** — eu disse que curava, antes de testar, e o dB
soldou por causa disso. O conserto de firmware de hoje (tecto de 3 ms e recuo na
transmissão, v0.216.0) **também não chega**: recriada a falha, a placa travou.

**Caminho, para amanhã:** o problema é da **recepção**, não da transmissão —
parar e rearmar o `DmaListen` quando os erros de UART passarem de um limiar.
⚠️ **Mas com instrumento novo primeiro:** o LED de sub-fases só é válido com o
laço PARADO; com ele a correr dá números arbitrários, e eu diagnostiquei a
partir de um deles.

### O que ficou resolvido

| | |
|---|---|
| **Pots** | sentido invertido, batentes esticados (o cobre para a 0,0009–0,9756), e a histerese que media do centro e calava o iPad — **210 150 janelas em 20 s parado → 0** |
| **Microfonia no tambor** | **770 e 834 Hz**, achadas pela vigia (199 e 252 de 999 capturas, dominância 0,96/0,98), cortadas a 15 dB. **Resolveu** |
| **Ruído do amp** | guincho = banco na entrada do step-down · clique = terra · zumbido = rádio da ponte pelo trilho partilhado |
| **Colisão de CC** | notch entrara no 115, do `bench_boost`, nos mesmos canais → movido para 82–86, e nasceu `Manifest::Colisoes()` |

### O que ficou por fazer

**Bancada (hardware), com o plano escrito em [`ATERRAMENTO.md`](operacao/ATERRAMENTO.md):**
1. Separar a **ilha de `AGND`** — piezos, DPDT, jacks, entrada do amp. Conferir
   com continuidade, **com a Daisy fora do soquete**.
2. **100 µF + 100 nF** nos pinos da ponte ESP32, e fio de terra próprio à
   estrela. ⚠️ 100 µF, **não** 2200 — ela pendura-se na saída do step-down.
3. Afastar os cabos dos piezos dos cabos dos exciters (acoplamento **pelo ar**,
   imune a esquema de terra).

**Firmware:**
- **Detector automático de microfonia.** O argumento fechou-se hoje: não é só
  que cada peça da série terá modos próprios — os modos **mudam durante a
  sessão**. A vigia já tem as peças (dominância medida, contagem de aparições,
  95% de ciclo útil); falta a decisão automática.
- **Duas bocas podem não chegar** — já se viram quatro modos capazes de gritar
  (770, 834, 930, 1403). Subir para quatro é uma constante em `notch.h`.
- **O notch não passou pelo IFA10000.** Efeito novo, certificação devida.
- **Presets** — travados por duas decisões não tomadas (ver abaixo). E hoje
  ficou concreto o custo: afinou-se por medição dois valores que só existem
  enquanto a placa não reiniciar.

**Decisões que só o dB pode tomar, e que travam os presets:**
- `post_level_l`/`post_level_r` entram no preset global? *Recomendo que não* — seguem
  botões físicos, como o `exc_on`.
- Renomear `test_tone_*` → `test_tone_*` para alinhar com os widgets do Loopy?

Ambas são livres **só até à primeira captura** congelar o mapa (hoje `0x3505`).

## Como retomar

```bash
cd ~/CODE/GitHub/Cotucatronics/src
make BOARD=daisy_seed && make check
# RESET simples (SEM segurar BOOT — isso entra no DFU da STM, onde mora
# o bootloader). Grave na janela de graça de ~2 s:
make program-dfu
./cotuca_serial.py --send board mods list --gap 2
```

Placa roda da QSPI via bootloader da Electrosmith (`APP_TYPE=BOOT_QSPI`),
480 MHz permanente, `-O3` (ver a cicatriz do `-O3` no cap. 3 do battle
scars — ele PIOROU; reverter exige remedir tudo).

---

## O que toca hoje

**Pedaleira de cinco efeitos residentes, em ordem fixa.** Vocoder fora do
build desde 18/08 (decisão de ouvido, revivível). Bloco **48**.

```
piezo/linha → src_mix → in_gain → gate → DRIVE → PHASER → WARBLE   (por cadeia)
            → MATRIZ 2×2 ─┬─ DELAY (uma instância, não segue a matriz)
                          ├─ FREEZE  (por cadeia, segue)
                          └─ [SHIMMER ↺ REVERB] (por cadeia, segue)
            → level → governor+SSB por ramo → limiter → out_l / out_r
            → [DPDT 3 pos: jack | ambos | amp]        ← isto é COBRE
```

`piezo_mix` é BALANÇO, não crossfade: no centro os dois piezos entram cheios.

**O `out_width` (mid/side) foi aposentado em 20/08** — com dois transdutores
no mesmo corpo não há imagem estéreo a alargar: a soma é acústica, no objeto.

**A matriz decide qual CADEIA entra em qual SAÍDA** (`route_a_l` … `route_b_r`,
CC 97/98). O destino de cada saída não é decisão do firmware: é a posição do
DPDT. O firmware nomeia SAÍDA; o cobre nomeia destino.

O shimmer não tem reverb próprio: ele realimenta o Freeverb que já está na
cadeia, e por isso aparece dentro do laço.

| Recurso | Medido |
|---|---|
| CPU (suíte + shimmer + detune) | **59,5% média / 65,8% pico** do prazo de 1000 µs |
| Tabelas | **91/96 params · 91/128 CCs** (o `board` declara, 28/08) |
| Bloco | **48** (1 ms de prazo) |
| Custo por efeito | drive +21 µs · warble +14 · phaser +26 · detune **+125** |

---

## ✅ Estado do repo é INSTRUMENTO

Resolvido em 2026-08-17. A dinâmica voltou e a placa **sobe tocando**:

| | |
|---|---|
| `pre_level_ab` 1.00 · `in_gain` **8×** | `gate_thresh` 0.02 · `gov_ceiling` 0.85 |
| `rev_mix` 0.35 · `fx2_hp` 120 Hz | `fx2_on` 1 · `buzios_on` 1 · `buzios_thresh` 0.04 |

`shift_hz` segue em 0: é anti-feedback do laço piezo→corpo→piezo, e liga
quando o **exciter** entrar no caminho — antes disso só metaliza o timbre.

> **Quando o exciter entrar, revisar `pre_level_ab` e `gov_ceiling` juntos.** Lá
> não há atenuador: o TPA3116 recebe o que a placa mandar, e os dois
> formam o par que decide onde o laço para de crescer.

---

## 28/08 — a matriz de roteamento, medida na placa

**O que mudou.** Havia dois andares confundidos num só. Agora estão separados:

| | onde age | o que faz |
|---|---|---|
| `route_a_l/b`, `route_a_r/b` | antes dos efeitos | qual **cadeia** entra em qual **saída** |
| `out_l`, `out_r` | depois do limiter | corta o **ramo** inteiro |

A matriz nasce cheia e reproduz exactamente o `out[0] = out[1] = A + B` que a
antecedeu. Serve para inverter as cadeias quando os transdutores estão ligados
ao contrário — e **fica fora dos presets**, porque é facto de fiação e não de
timbre.

**T7c medido em v0.178.0**, chão do caminho a −69,2 dBFS, base a −35,6:

**Refeito com uma saída por canal** (`OUT_L → ch1`, `OUT_R → ch2`, separação
59,6 dB). A primeira corrida tinha as duas saídas somadas numa entrada, e por
isso dois dos critérios passaram **sem poder falhar** — ver `TESTES.md`.

| critério | OUT_L | OUT_R | |
|---|---|---|---|
| `route_a_l=0` corta A em L, e só em L | −44,8 | +0,0 | ✅ |
| `route_a_r=0` corta A em R, e só em R | −0,0 | −43,9 | ✅ |
| `out_l=0` corta o ramo L, e só L | −44,1 | +0,0 | ✅ |
| `out_r=0` corta o ramo R, e só R | +0,0 | −41,5 | ✅ |
| **inversão** A→L vira A→R | −32,6 → −77,4 | −80,9 → −36,7 | ✅ |
| o delay sobrevive à matriz | — | +41,0 dB acima do chão | ✅ |

O último separa os andares: a matriz tira o seco, o freeze e o espaço da
cadeia A, e o retorno do delay fica — uma instância só, envios já somados.

O teste vive em `src/teste_roteamento.py`: pergunta a porta, acha a interface
pelo nome, **mede** o mapa de canais e aborta se as duas saídas caírem no
mesmo.

⚠️ **O mapa de params mudou: `0xFD72` → `0xBE1B`.** Como ainda não existe
preset nenhum, saiu de graça — foi a última janela.

### Defeitos abertos que esta sessão deixou

- ✅ **`ccmap.html` / `ccmap.pdf` — RESOLVIDO, e virou GERADO.** O
  `gerar_ccmap.py` v0.3.0 escreve os **dois** destinos (`CCMAP.md` e o cartão
  A4) a partir do `manifest.h`. As notas humanas vivem numa tabela `NOTAS` — a
  única parte escrita à mão — e o gerador **denuncia nota órfã** (param que já
  não existe) e **nota que mente sobre topologia** (diz "compart." num param
  pareado). Achou **dez notas erradas** à primeira passagem, incluindo o
  `frz_size` a dizer 20–400 ms com o manifest em 20–165 (o caso que gerou a
  regra das faixas) e o `test_tone_out` com a semântica invertida. O par colapsa
  numa linha (`in_gain_a/b`), e por isso volta a caber em **uma folha**.
- ✅ **`OUT_L` — RESOLVIDO em 28/08.** Era fiação, e a placa foi inocentada
  por testemunha interna antes de se mexer num fio: com o tom em `dest 1` o
  `gov A` sobe a 0,285 enquanto o `gov B` fica em 0,001, e invertem-se com
  `dest 2` — sinal nos dois ramos, simétrico, dentro do firmware.
- ✅ **Contador de quedas da ponte — RESOLVIDO.** `ponte` v0.5.0 guarda em
  NVS, com **duas contas**: `quedas` desta sessão responde *"está a cair
  agora?"*; `desde sempre` + `arranques` responde *"caiu enquanto eu não
  estava?"*, e a segunda distingue energia instável de rádio instável.
  Escrita diferida (máx. 1×/10 s, fora do callback do BLE, porque a NVS tem
  endurance finita e a peça fica meses ligada). Persistência **provada**: o
  contador de arranques foi 2 → 3 → 4 através de resets.
- ✅ **Os 20 minutos do link BLE — PASSOU em 28/08.** `0 quedas`, com o iPad
  acordado e o Loopy em primeiro plano. E o `arranques` subiu **exactamente
  um** desde a leitura anterior — e esse um foi a sonda a abrir a porta —,
  o que fecha também a outra hipótese: **a ponte não reiniciou.** Piscar de
  alimentação teria aparecido ali.

  ⚠️ **A sonda derruba o que mede, e isto tem de ficar escrito.** Abrir a
  porta série do ESP32-C3 **reinicia a placa** (o USB nativo lê a abertura
  como reset, mesmo com `DTR`/`RTS` em falso). A primeira tentativa deste
  teste morreu assim: eu li o `board` e derrubei a ligação do dB. Para
  observar ao vivo, **abrir UMA vez e segurar** — o `/tmp/teste20.py` faz
  isso. E a corrida anterior, que "falhou aos ~6 min", **não conta como
  evidência**: eu li o `board` a meio dela.

  **A NVS pagou-se na primeira utilização real.** Ler a ponte exige
  reiniciá-la, o que apaga os contadores da sessão; só o de vida sobreviveu,
  e foi ele que deu o resultado. Há três dias este número teria desaparecido
  no acto de o ir buscar.

  Não provado: os contadores de mensagens foram-se com o reset, logo não há
  prova numérica de tráfego — só a observação do dB, que estava a tocar.
- ✅ **Tectos subidos, com folga na mão e não com o estouro.** `kMaxParams`
  96 → **128** e `kMaxPresetVals` 40 → **56** — este era o mais apertado
  (33/40), porque um param de cadeia novo gasta um slot aqui e **dois** na
  tabela de params. Custo medido: **+1504 bytes**, SRAM 55,59 % → 56,00 %. E o
  alarme de estouro de preset ganhou **gémeo consultável** no `board`: só
  saía no boot, e quem liga a serial depois nunca o via.
- ✅ **Ponto cego do `varre` — RESOLVIDO.** `varre tudo` lê os **31** pinos,
  com o preço declarado: mata o OLED e o MIDI, porque são os mesmos pinos.
  `reboot` desfaz — comando novo, porque modalidade que só sai com o dedo de
  alguém não serve a uma escultura numa galeria.
- ✅ **`check_docs` v0.12.0 vigia `.py`.** A regra antiga cruzava cabeçalho
  com `#define`, e um script não tem `#define`. A nova recolhe **toda** a
  auto-identificação — docstring, constante `VERSAO`, e o que ele **imprime**
  — e exige que concordem. A que mais importa é a terceira: é a única que o
  operador vê. Achou um segundo caso na primeira execução (`pitch_spectrum.py`
  dizia 0.2.0 e imprimia 0.1.0).

---

## 28/08 — o painel: OLED vivo, três encoders a contar

**O OLED funciona.** Enche, apaga e escreve texto formatado, e o espelho dos
encoders desenha as três linhas com os contadores. Foi a primeira vez — o que
se via antes era **imagem retida** de uma sessão anterior, porque nada
inicializa o painel no arranque (ver cicatriz 15o).

Ferramenta de bancada que ficou, e que provou os cinco fios um a um:

| comando | o que faz |
|---|---|
| `oled` · `oled off` · `oled texto` | inicializa/enche · apaga · **escreve** (prova mais que o encher) |
| `oledvel <2..256>` | varre o prescaler do SPI — oito velocidades |
| `fio curto` | caça-ponte nos dez pares |
| `fio <rst\|dc\|cs\|clk\|din> <0\|1>` | segura o pino para medir **no conector do painel** |
| `varre` / `varre reset` | lê os **15 pinos livres** de uma vez, sem opinião |
| `varre tudo` | lê os **31** — mata OLED e MIDI até um `reboot` |
| `reboot` | reinicia o app por software; o estado volta aos defaults |
| `encpino` / `encpino reset` | trava por pino dos encoders, nas duas direcções |
| `enc inverte <a\|b\|g>` | inverte o sentido **em execução**, sem gravar |

⚠️ **O `varre` normal lê 15 dos 31 pinos** — ficam de fora `D5`–`D8`, `D10`
(OLED), `D13`/`D14` (MIDI) e `D22`–`D30`. Isso é correcto para o uso comum:
sonda que dirige um pino de periférico parte o periférico. Mas criava um modo
de falha inteiro que nenhum instrumento nosso via, e **`varre tudo` abre-o**.

**Encoder G: fases trocadas no `kEncPin`** (`D19` fase A · `D18` fase B ·
`D17` push), porque montado o CW contava para trás. A troca mora no mapa de
pinos e não num sinal invertido no laço — o sentido de um encoder *é* qual
fase vem primeiro, e inverter o resultado depois faria o `encpino` mentir.

### 🔴 Defeito aberto: o encoder G é INTERMITENTE

As duas fases do G **abriram durante a sessão**, sem ninguém tocar no
hardware. O `varre` provou-o: quinze pinos livres, e só o push a comutar
(`push 14 · crus +0`). **Voltaram durante o manuseio do chicote** — e
intermitência correlacionada com manuseio é o diagnóstico, não ruído.

O ponto fraco continua lá e vai reabrir. **Resoldar antes da galeria.** O push
tem circuito próprio no EC11, o que explica ele ter sobrevivido enquanto as
duas fases morriam juntas: elas partilham um comum que o botão não usa.

---

## 28/08, fim do dia — a sessão de escuta e o que ela cobrou

**🔴 A placa travou, e o LED de fase foi quem a encontrou.** O `PollMidi`
drenava com `while(m_tail != m_head)` — condição que persegue um ponteiro
escrito pela ISR. Sob tráfego BLE contínuo o laço nunca terminava: áudio a
tocar, tela congelada, serial calada, e nenhum vigia disparou porque a
chamada sempre retorna. Consertado com instantâneo do ponteiro, **e o irmão
`PollSerial` levou o mesmo**. O `board` passou a declarar o atraso da
drenagem — sensor que teria denunciado isto antes de travar.

**🗑️ `in_gain` REMOVIDO — "se não soa, não fica"** (regra do dB). O
`Governor` é um AGC: 18 dB postos no `in_gain` eram 18 dB que ele tirava
adiante. O controlo funcionava e era inaudível **por construção**; só restava
o degrau. Deep clean em código, geradores e sete documentos. **CC 21 livre.**

⚠️ **O que se perdeu:** o casamento com transdutores de sensibilidade
diferente. O governor só REDUZ. Se aparecer na replicação, conserta-se no
divisor da carrier board — a lápide está no `FEATURES.md` e no `TESTES.md`.

**🥁 TAROLATOR — efeito novo** (`tarol_on` cc80, `tarol_mix` cc81, por
cadeia). Nasceu oitavador por divisão de frequência, e não faz oitava: numa
membrana os modos são inarmónicos e o divisor treme, gerando ruído
percussivo com o envelope da pancada. **O dB gostou e deu-lhe o nome certo.**
Tom e rastreio fixos (6000 e 1200 Hz); `mix` 0–4× com curva 0.5, porque
0–1 linear deixava dois terços de fader mortos. Custo **+2,9 %** nas duas
cadeias, medido em A/B no mesmo binário — não entra na ordem de sacrifício.

**Também:** `wrb_rate` passou a **0,5–12 Hz** (era 0,05–3, e o topo já era
lento); **`shm_on` abre o espaço sozinho** — antes só corria com `rev_on`, e
a chave existia surda.

### A medição que mudou o desenho

A membrana foi medida em vez de descrita: **76,5 % da energia entre 640 e
1280 Hz, ZERO abaixo de 160 Hz**, modo dominante em **768 Hz**. "Magro" é
literal. Isso matou a sugestão de drive (acrescentaria onde já sobra) e
ressuscitou o oitavador que eu tinha objectado — a fundamental não era grave.

### Entrada

Piezos a **−17 dBFS** (corda, toque normal), **−7,5** (corda, forte) e
**−21,3** (membrana). A membrana chega **14 dB abaixo** da corda. Separação
entre os dois piezos através do corpo: **34 dB**.

---

## 28/08 — os potenciómetros

**Dois pots, e só cabem dois:** `A0` = D15 (ramo L), `A1` = D16 (ramo R). Os
outros três do bloco ADC são o encoder global — `A0–A4` são aliases de
`D15–D19`. `num_knobs` passou de **5 para 2**; declarar cinco fazia o laço
iterar sobre knobs inexistentes.

**Params novos:** `post_level_l` / `post_level_r` (CC 116/117), volume contínuo por ramo,
**depois do limiter** — mesmo ponto do mute `out_*`, e pela mesma razão: ali
só se pode reduzir. São funções diferentes no mesmo sítio — `out_*` mata o
ramo (segurança, instantâneo), `vol_*` dosa (performance, contínuo).

### Os três problemas, e nenhum era ler o ADC

| | |
|---|---|
| **a guarda antiga matava o MIDI** | era `if(v == knob_last[k])` — igualdade de float sobre ruído **nunca dispara**, e o pot escreveria a cada volta do laço. O comentário que lá estava previa-o: *"sem ela o MIDI para de funcionar em silêncio no mesmo dia"* |
| **pot absoluto, param partilhado** | **captura suave**: só assume quando passa pelo valor actual, larga quando o iPad escreve. Janela de largar maior que a de capturar, senão troca de dono na fronteira |
| **mas no arranque a verdade é o botão** | primeira leitura **adopta** a posição física — volume que nasce longe do botão assusta quem liga |

**Medidor:** `knob` mostra cru, filtrado, param e dono; `knob <s> <c> <l> <z>`
ajusta os quatro números sem gravar, porque só a bancada sabe o ruído deste
ADC.

**Bancos nativos novos:** `src/teste_tarol.cpp` (o divisor em material
periódico) e `src/teste_macro.cpp` (a ida-e-volta do `Macro::Inverso`, de que
a captura depende — se ele errar, o pot "não pega" e a bancada culpa o ADC).

⚠️ **Nada disto foi gravado nem medido na placa** — o dB desmontou a bancada.
O `knob` diz na hora se o ADC lê.

---

## 31/08 — os pots funcionam, ponta a ponta

**`post_level_l` / `post_level_r` seguem os dois potenciómetros.** Curso completo
verificado: `0,9752 → 0,0001` com o param a acompanhar. Ruído em repouso
**σ 0,0003**, que são 26× abaixo de um passo de MIDI, e **zero escritas**
com os pots parados.

Condensadores de **0,1 µF do cursor à massa, no próprio pot** — ficam, e
valem: **29× melhor** no canal ruidoso, e a assimetria entre os dois
desapareceu. Com o sinal limpo, a filtragem foi afrouxada (filtro 12× mais
rápido, histerese 2,4× mais fina) sem custo nenhum em estabilidade.

### 🔴 A instabilidade era um curto, e eu acusei três inocentes

A placa caía do barramento durante a montagem. Em duas horas acusei **o hub
USB** (a Daisy não estava em hub nenhum), **os condensadores** (medi com os
pots desligados) e **o circuito** (cortava a energia entre os testes, e
creditava a recuperação à variável errada).

Era um **curto intermitente entre PWR e GND da ponte**, encostado ao mexer nos
cabos. Brownout — a placa perde alimentação e some do barramento; não é
travamento. **Encontrou-o o dB, a olhar para os fios.**

⚠️ **Isolar esses dois fios.** Curto de alimentação intermitente desaparece
sempre que se vai procurar.

## 31/08 — e o defeito do ADC era firmware

**`post_level_l` / `post_level_r`** (CC 116/117) seguem os dois pots físicos. Custou uma
sessão inteira, e o culpado não era o cobre:

### 🔴 O varredor de pinos livres roubava o pino ao ADC

`D15` e `D16` estavam em `kLivres`, e o `VarreLivres()` — que corre **a cada
volta do laço** — reconfigurava-os como digitais na primeira passagem,
arrancando-os do modo **analógico**. O ADC amostrava uma entrada desligada, os
dois canais liam o mesmo ~0,12, e **nada feito ao cobre podia mudar nada**: o
firmware chegava sempre primeiro.

O dB trocou fios, removeu um condensador, mediu 3,3 V nos extremos e trocou os
pinos entre si — tudo em vão, por minha causa.

> **Lista de "pinos livres" é afirmação sobre POSSE, e envelhece.** Terceira
> vez no mês: `num_knobs = 5` com dois pots, a isenção do `kEncPin` depois de
> ele virar fonte, e agora esta.

A trava mora agora dentro do `VarreLivres` e grita em vez de roubar calado.

### ⚠️ E as duas sondas que construí para o caçar mentiram

**`knob pino`** segurava o pull 5 ms; com 0,1 µF e ~40 kΩ a constante é ~4 ms —
lia antes de o condensador carregar, e deu *"LIGADO"* ao caso que existia para
apanhar. Agora espera 60 ms.

**`knob prova`** dirigia o pino como saída digital, o que nos STM32 **desliga o
interruptor analógico** — o ADC deixa de ver o pino. Estava garantida a dizer
*"não segue"*, e eu usei isso para anunciar uma conclusão.

O que resolveu não foi sonda nenhuma: foi ler o código à procura de **quem
mais tocava naqueles pinos**.

### Estado medido

| | |
|---|---|
| escritas em repouso | **0,07/s** (eram 623/s antes da quantização) |
| ruído `k2` (D16) | σ **0,00085** — limpo |
| ruído `k1` (D15) | σ **0,0055** — seis vezes pior, sem condensador de filtro |

⚠️ **Repor o 0,1 µF, agora em PARALELO** (pino → GND), pelo menos no D15. Com
o sinal limpo dá para afrouxar a filtragem e o pot fica mais responsivo.

### Também nesta sessão

- **`min_knobs` 4 → 2.** Baixar o `num_knobs` fez o `CheckRequirements()`
  recusar arrancar — correctamente. A **recusa passou a explicar-se**: o LED
  conta o código da falha (1 áudio, 2 RAM, 3 knobs) com a gramática do
  instrumento de fase, e o texto **repete-se**, para quem ligar a serial depois.
- **`AdcInit` saiu de antes do `StartLog`.** Inicialização que pode falhar não
  mora antes do canal que reportaria a falha.
- **Quantização no passo do fio** (1/127 com histerese de 0,85 passo) em vez de
  zona morta mágica — o pot tem a resolução do protocolo, nem mais nem menos.
- **O pot fecha a recepção MIDI do seu param** ao escrever, em vez de esperar
  que o espelho o anuncie até 8 ms depois.

---

## Versões

**Moram no mapa de arquivos do [README.md](../README.md), em um lugar só.**

Esta seção já teve uma tabela própria. Ela chegou a `main.cpp v0.85.1`
enquanto a placa rodava a v0.130.0 — **45 versões de deriva**, porque era uma
segunda cópia que ninguém imprimia e ninguém lia. Mesma forma que o
cabeçalho do `main.cpp` carregava (cicatriz 13k e a regra do checklist):
*número que ninguém imprime não tem quem o corrija.*

A versão que vale é a que a **placa diz no boot**, e o `flash.sh` a confere
contra o binário depois de gravar.

---

## Fechado em 18/08 — MIDI certificado sob carga

**T16c passou.** 2 779 CCs em 90 s com o vocoder a **65,1%** do prazo do
bloco: zero órfão, zero ressync, zero perdido, zero erro de UART. A marca
d'água da fila ficou em **32 de 128 bytes — idêntica com e sem carga**, o
que fecha com número o furo do `PollMidi` (bytes entram por DMA, mas quem
despacha é o loop). Os 32 bytes são o tamanho da rajada do BLE, não atraso
do loop: se fosse latência, teria subido com a carga.

**Regra revogada:** o pareamento BLE **sobrevive** ao reset. O
`gravar → reparear → testar` era superstição nascida de um log que eu
mesmo silenciei — caso 8 do battle_scars, com adendo no capítulo 11 e a
seção do BOARDS.md marcada como REVOGADA.

**Cicatrizes viraram skills.** `.claude/skills/`: `ifa10000`,
`medicao-limpa`, `instrumentacao`, `bug-de-ordem`, `edicao-segura`. O
IFÁ10000 ganhou a **etapa 0 — MEDIÇÃO LIMPA**.

---

## Ganho sonoro não planejado (18/08): o grave voltou

Aceite do dB, sem eu ter mexido em nada de timbre: **"presença de grave que
nunca teve antes, muito melhor sonoramente."**

**Causa, verificada no código:** o passa-alta de 120 Hz mora dentro do bloco
do Fx2. Vocoder fora ⇒ slot vazio ⇒ `fx2_want` falso ⇒ bloco pulado ⇒ **sem
filtro**. Ele existia só pra conter o estouro de grave que o vocoder
causava, e saiu junto com a causa porque foi instalado no escopo certo.

**Contribuição secundária, não medida:** `block_size` de 256 → 48 derruba a
latência de 5,3 ms pra 1 ms. Num instrumento realimentado isso desloca onde
o laço acústico se estabelece e aperta o transiente. Plausível e não
isolado — se quisermos o número, é um voo com uma variável.

---

## ✅ Freeze granular — CERTIFICADO (2026-08-18)

IFÁ10000 completo. **Etapa 9 passou:** *"ficou ótimo assim"* — sustentado de
10 s sem pulsação, cauda de 2 s, instrumento vivo audível por baixo.

| Etapa | Resultado |
|---|---|
| 4 MEMÓRIA | 768 KB SDRAM, 4 leituras **sequenciais**/amostra |
| 5 CARGA | **98 µs** do efeito em 500 de orçamento; 370 de 1000 no sistema |
| 6 MATERIAL | transiente real (tap na escultura) |
| 8 SEGURANÇA | fora dos búzios — categoria **GESTO** |
| 9 ACEITE | ✅ dB aprovou |

**Três versões até chegar aqui, e cada uma consertou um erro de desenho meu:**
- **v0.1.0** reprovou de ouvido: varredura linear = laço de 2 s. Eu tinha
  escrito no cabeçalho que o jitter mataria a periodicidade. Não mata.
- **v0.2.0** trocou varredura por **sorteio num núcleo de 500 ms** — matou a
  gaiola e, no mesmo gesto, quebrou a premissa de correlação da janela,
  criando o ganho baixo. *Janela e estratégia de posição são um par.*
- **v0.3.0/0.3.1** fechou: **raiz-de-Hann** (potência constante para grãos
  descorrelacionados), **mix como camada** e não crossfade (o crossfade
  apagava o instrumento vivo), e **cauda de 2 s**.

---

## Para amanhã — pedidos do dB (18/08, fim de sessão)

O instrumento está **funcionando e aprovado** ("tudo ok! excelente"). Três
ajustes do freeze, e os três têm raiz comum de desenho:

**1. `frz_mix` é crossfade e deveria ser CAMADA.**
```cpp
l = l * (1.f - mix_) + wl * mix_;   // mix 1.0 APAGA o instrumento
```
Congelar deve **somar** uma camada sustentada, não substituir a fonte. É a
causa de "quando o frz está rolando não escuto o instrumento direito" — não é
efeito colateral, é o que a linha faz.

**2. Ganho do freeze muito baixo.** Causa técnica: duas janelas Hann somam
1.0 só para conteúdo **correlacionado**. Com as posições agora **sorteadas**,
os grãos são descorrelacionados e somam em potência — ~0,7× da amplitude
esperada. O conserto certo não é ganho de maquiagem: é **janela raiz-de-Hann**
(`sqrt(hann)`), que dá potência constante justamente para grãos
descorrelacionados.

**3. Cauda no corte.** `frz_on` 1→0 corta seco; soa como tomada arrancada, não
como gesto. Rampa de saída de ~300 ms, com os grãos rodando durante a descida.
*Decisão pendente do dB:* tempo fixo ou parâmetro `frz_release`?
<!-- doc-check: ignore frz_release (proposta nao implementada) -->
> **Nota:** `frz_release` é **proposta**, não parâmetro. A marca acima o
> declara ao verificador de deriva — fantasma declarado deixa de ser
> fantasma; fantasma silencioso quebra o build.

### Também na fila, do mesmo desenho (CTOS protegendo a base)
- **Tirar `src_mix`, `gate_thresh` e `gov_ceiling` do mapa MIDI.** São
  roteamento e segurança — foram exatamente eles que emudeceram o instrumento
  no despejo fantasma. *O que não está mapeado não pode ser derrubado.*
- **Limite de taxa de mudança por parâmetro** vindo de MIDI: fader real anda
  em rampa; salto de 0 a 127 num CC é assinatura de máquina.
- **Relógio confiável pro watchdog:** `hal::NowUs()` devolve lixo
  esporadicamente (3 amostras descartadas em 12 min). O veto repousa nele.

---

## Fechado em 20/08 — o shimmer que ardia

Duas causas distintas, medidas na bancada offline (os headers são C++ puro,
então o host reproduz o laço exato):

**1. Aliasing no motor de pitch.** Ler o buffer a `ratio 2.0` dobra a
frequência; tudo acima de 12 kHz atravessa Nyquist e volta **inarmônico**. Com
3500 Hz na entrada, o pico mais alto de toda a cauda era 19963 Hz. Anti-alias
Butterworth de 6ª ordem **na escrita do motor** (não no shimmer — o defeito é
de ler rápido, e quem lê rápido é o motor): dobra de −11,9 para **−64 dB**.

**2. Conteúdo legítimo na faixa mais sensível do ouvido.** A planura espectral
de 2–6 kHz é 0,009 — tonal, não ruído. O 1–3 kHz da escultura vai a 2–6 kHz
pela oitava e o reverb sustenta lá. Conserto: **sino de −6 dB em 3 kHz**, com
a profundidade escolhida contra a razão medida da cauda seca (0,28), não de
ouvido.

| | v0.1.0 | v0.3.0 |
|---|---|---|
| razão (2–6 k)/(300–2 k) | 1,01 | **0,19** |
| centroide da cauda | 7134 Hz | **1907 Hz** |
| 6–12 kHz | 28,2% | **4%** |

`shm_tone` (**CC 57**) move as duas coisas e **sobe pra clarear**: 0 é o sino
cheio com corte em 6 kHz, 1 abre até 18 kHz. Zero tem que ser o estado bom, ou
a aspereza volta a cada boot.

---

## Fechado em 20/08 — teto de soma do CTOS

A última fronteira estrutural. O watchdog do §3 protegia contra **um efeito
com defeito**; faltava a proteção contra **N efeitos saudáveis que juntos não
cabem**. São situações de natureza diferente e exigem mecanismos diferentes —
a distinção está inteira em [CTOS.md §3b](CTOS.md).

**Não há culpado no teto de soma**, e é por isso que ele não procura um:
qualquer regra que tente achar "quem foi" quando não há quem acaba inventando
um culpado, e o inventado seria um efeito correto. A política é **prioridade
declarada a frio** pelo dB em `Manifest::sacrif[]` — a ordem do array *é* a
prioridade. LIFO foi descartado: pune por ordem de chegada, que não tem relação
com valor musical.

Três testes, todos passados na v0.84.0:

| Teste | Esperado | Resultado |
|---|---|---|
| teto 620 µs (bloco 639) | cai **só** o detune | ✅ bloco 639 → 526 µs, os outros cinco tocando |
| teto 200 µs | cascata na ordem + estado "sem saída" | ✅ detune→phaser→warble→drive→freeze→shimmer, depois *"a base sozinha não cabe"* |
| `ctos rearm` | solta latches **sem** religar | ✅ |

Com os seis efeitos ligados o bloco fica em **577–639 µs de pico ~700**, contra
teto de 850 (85% de 1000). A suíte inteira ainda cabe com folga. Mecanismo
custou **+14 µs**.

**Duas correções que os testes forçaram:**

1. **A serial morreu no primeiro teste** — eu chamava `LogAlways` e
   `MidiSendBytes` de dentro do `AudioCb`, que é o defeito de prioridade 1
   deste documento, catalogado antes de eu cometê-lo. Travou o instrumento
   inteiro, não só o CDC. Consertado: o callback marca (`volatile`), o loop
   fala.
2. **Derrubava dois quando um bastava.** Zerar o param não faz o efeito parar
   de custar — o `EffectSlot` desce por rampa e continua processando. O
   mecanismo media de novo antes da própria ação surtir efeito. Consertado com
   **carência de 250 ms** depois de cada queda.

---

## Pivô de 20/08 — duas cadeias de caráter, medido

```
p1 → in_gain_A → gate_A → [DRIVE·PHASER·WARBLE] ┐
                                                 ├→ MIX → [DELAY·FREEZE·SHIMMER↺REVERB]
p2 → in_gain_B → gate_B → [DRIVE·PHASER·WARBLE] ┘   → level → OUT_WIDTH → split
```

Firmware **v0.91.0** · **71/96 params · 36/48 CCs** · SDRAM liberada: 153,6 KB.

| | µs |
|---|---|
| tudo ligado | **620 média / 663 pico (66%)** |
| base (nada ligado) | 160 |
| os seis de caráter | ~130 (90 o primeiro a ligar, 38 o segundo — I-cache) |
| cauda (reverb 99 + freeze 69 + shimmer 202) | 370 |

**A assimetria entre cadeias era I-cache, não código.** Quem liga primeiro
paga ~90, quem liga depois ~38, seja qual for a cadeia — três corridas
concordantes com a ordem invertida. As duas cadeias são simétricas.

**A base decomposta matou o caso de otimização:** dos 182 µs originais, o
desperdício real eram os dois gates com o portão aberto. Módulo do delay (45),
osso da dinâmica (94), medição (15) e escritas do warble (7) estão todos
fazendo algo. Guarda `thresh > 0` implementado — **26–30 µs medidos A/B no
mesmo binário**.

**Aberto:** aceite de ouvido no instrumento híbrido, e IFÁ10000 dos seis
efeitos nas novas posições (não herdam o aceite da cadeia antiga).

---

## Fechado em 20/08 — RNBO fora, inteiro

O delay era o último consumidor do runtime. Reescrito em C++ (`delay.h`, grafo
lido do export: feedback fixo **0,4**, crossfade, mono nos dois canais), o
RNBO saiu por completo.

| | antes | depois |
|---|---|---|
| tudo ligado | 678 µs | **545 µs / 656 pico** |
| base (nada ligado) | 182 µs | **115 µs** |
| o delay | 45 µs, **sempre** | +11 µs, só ligado |
| binário | 212 424 B | 175 980 B |
| params | 61 | **54** |
| repo | — | **−8 MB** de export |

**Os 67 µs da base são o dado que interessa:** um `Module::Process` roda ligado
ou desligado, porque o módulo não sabe que está em bypass. Custo-zero-desligado,
que todo efeito da casa segue, era **estruturalmente impossível** dentro do
sistema de slots.

Saíram junto: `MODULE_SRC`, `RNBO_DIR`, `module.h`, `mod_slot[]`, `ModBind` e o
push-por-nome, seis arquivos, quatro exports, os seis params órfãos do vocoder,
o watchdog do fx2 que julgava pelo CPU global (**achado 8**), o comando `probe`
e o histograma nota-a-nota (**achado 7**).

**Método na cicatriz 12f:** remover o símbolo-raiz e deixar o compilador
enumerar os dependentes. Quatro erros na primeira passada, depois um, depois
zero — sem faixa calculada e sem prosa perdida.

**Mapa de CC refeito:** todo param recebe CC (54/64), uma década por efeito.
Nada mais só-serial.

---

## Fechado em 20/08 — cauda PARALELA e dois espaços

O dB pediu p1 com delay e p2 com reverb, e depois corda com shimmer e couro
com sala limpa. A cauda serial não conseguia nenhum dos dois.

```
A ─┬→ ×dly_a ──┐
B ─┼→ ×dly_b ──┴→ DELAY  (uma instância) ──┐
   ├→ ×frz_a ──┐                           │
   ├→ ×frz_b ──┴→ FREEZE (uma instância) ──┤→ soma → level → split
   ├→ ×esp_a ───→ [SHIMMER_A ↺ SALA_A] ────┤
   └→ ×esp_b ───→ [SHIMMER_B ↺ SALA_B] ────┤
   └→ seco (as duas, inteiro) ─────────────┘
```

**Delay e freeze continuam sendo UM**, com envio por cadeia — o gesto pedido
era sobre espaço, e duplicá-los custaria CPU sem entregar diferença. **O
espaço duplica de verdade**, porque shimmer não é um efeito ao lado do reverb:
ele *é* o reverb realimentado com pitch, e duas molhaduras exigem duas salas.

| | previsto | medido |
|---|---|---|
| A: delay+shimmer · B: sala limpa | 625 µs | **490 / 530 pico — 53%** |
| pior caso: as duas com shimmer e sala | 807 µs | **598 / 639 pico — 64%** |

O pior caso ficou **209 µs abaixo** da previsão: somei custos medidos na cauda
serial, onde cada efeito processava o sinal inteiro. Em paralelo cada um
recebe só o seu envio — **duas salas custam menos que o dobro de uma**
(cicatriz 12g).

**Consequências declaradas:** o delay deixou de ser reverberado (os ecos saem
secos, a sala trabalha sobre a fonte); os mixes internos viraram envios por
cadeia; e o freeze virou **dois independentes**, um por cadeia, com buffer
próprio: `frz_on_{a,b}` **são** o gesto de congelar, com `frz_vol` e
`frz_size` por cadeia.

> **Correção de deriva (22/08).** Este parágrafo descrevia por meses um
> desenho de "fonte e gesto separados", com um `frz_hold` que **nunca
> existiu como parâmetro** — o `manifest.h:264` já registrava a decisão
> contrária e a doc ficou no modelo antigo. Achado pelo audit de deriva
> doc↔código; ver bts 13h.

**Proteção anti-feedback duplicada** para o ramo B (bloqueante do hardware de
dois exciters): governor e boost por ramo, `shift_hz` compartilhado — dois
exciters no mesmo corpo com deslocamentos diferentes bateriam um contra o
outro. **Painel — pinos (22/08).** D26 = B1 randomizador, D27 = B2 assinalável,
**D25 = B3 rearme do CTOS** (novo em v0.111.0). D28 = LB do WIDI.
D29/D30 reservados pras chaves DPDT dos excitadores. Os três botões usam
pull-up interno e são ativos em baixo, então pino sem botão lê "solto".

**O LED do Seed carrega dois fatos numa cadência só:** devagar = loop
vivo e sem trava; **4× mais rápido = tem efeito derrubado pelo teto,
aperte B3**; congelado = loop morto. O alarme entrou na cadência
justamente pra não gastar o estado, que é a prova de aliveness.

**Aberto, e é o mais caro agora: não há preset em flash.** Toda gravação
zera todo parâmetro e `pre_level_ab` nasce em 0. A perda é silenciosa dos dois
lados e o sintoma imita defeito de efeito — custou dois diagnósticos
falsos em 22/08 (bts 12l).

`exc_filter_l/b` gateiam a proteção; quando os DPDT forem soldados, D29/D30
substituem o param pelo estado real.

**Simplificação depois (v0.106.0):** `esp_on/esp_mix` e o `rev_mix` antigo
eram **o mesmo controle duas vezes** — ganho na entrada e ganho na saída da
mesma sala linear. O dB viu antes de mim. Ficou um envio só por destino
(`rev_on`/`rev_mix`, mesma forma do delay e do freeze), retorno inteiro, e
`shm_mix` de volta ao emprego único. Cicatriz 12h.

**Mapa de CC: 75 params, 75 CCs, nada só-serial.** Uma faixa por família, e
todo destino com a mesma forma: `on_a, mix_a, on_b, mix_b`, depois os
compartilhados.

---

## Auditoria de params (20/08) — uma feature estava morta

O dB pediu auditoria de todos os params, um por um. Três camadas, quinze
minutos:

| camada | resultado |
|---|---|
| quem lê cada param | 🔴 **detector dos búzios apagado havia 5 commits** |
| faixa × trava interna | 🟠 `frz_size` com 61% de curso morto (corrigido) |
| varredura viva min/max | ✅ 75/75 atingem os extremos declarados |

**Os búzios estavam mortos desde `6a8175d`** — o detector vivia pós-gate,
dentro da faixa que substituí na reestruturação em duas cadeias. Nada
denunciou: as variáveis seguiram declaradas e lidas no loop, os params na
tabela, os CCs no mapa apontando pro nada, e o `board` reportando tudo certo.

**Feature morta parece feature desligada** — e `buzios_on` nasce em 0, então o
estado normal era o estado quebrado. Restaurado do `3303028` e testado: oito
pulsos, oito sorteios. A posição mudou de significado — agora lê a soma das
duas cadeias, e uma batida em qualquer piezo dispara.

**Falso positivo registrado:** a camada 2 acusou `drv_drive`, `drv_tone` e
`phs_rate` de alcance desperdiçado. São deliberados. A trava do efeito é rede
de segurança, não alvo — só faixa MAIOR que a trava é defeito.

A auditoria virou seção do IFÁ10000, para rodar depois de qualquer refactor
que toque no callback.

---

## Onde estamos (23/08, v0.155.0)

**A cadeia inteira do controlador está provada, e o painel acendeu.**

| frente | estado |
|---|---|
| params de caráter por cadeia | ✅ custo indistinguível, medido com A/B no mesmo binário |
| endereçamento por canal (ch1/ch2) | ✅ 37 pares, banco nativo |
| espelho em ch4/ch5/ch6 | ✅ Wrench viu ch5; ch4 e ch6 a confirmar |
| fio de volta (iPad → placa) | ✅ 93 CCs contados |
| presets em RAM | ✅ ciclo `capturar → dump → reset → colar → load` |
| botão de freeze B (D22/D23) | ✅ 4 batidas para 4 apertos |
| rearme do CTOS pelo B3 | ✅ ressuscitado — tinha ramo inalcançável |
| OLED SSD1309 | ✅ acende e escreve. **Configurado, nunca "presente"** |

**O que falta é humano:** remapear o Loopy (mapa inteiro em
[RETRABALHO-LOOPY.md](operacao/RETRABALHO-LOOPY.md)) e a **escuta que
constrói a biblioteca** — cada julgamento vira um preset em vez de uma
anotação.

## Painel físico (23/08) — encoders vivos, OLED morto

**Os três encoders EC11 funcionam**, com o mapa conferido por medição:

| encoder | fase A | fase B | push |
|---|---|---|---|
| cadeia A | D0 | D1 | D2 |
| cadeia B | D3 | D4 | D9 |
| global | **D18** | **D19** | **D17** |

**Um pulso por detente, medido** — dez cliques deram dez. A divisão fica em 1.

**O OLED não sobreviveu.** GND e 3V3 estavam invertidos na placa de
distribuição, e o módulo recebeu polaridade reversa. Sete causas foram
eliminadas por prova — firmware (o binário que funcionou também dá preto),
alimentação, encoders, fio rompido, ponte, velocidade e ordem de energização —
e tudo que divide a mesma placa de alimentação funciona.

**Ao trocar o painel, o teste é de um minuto:** `oled` acende tudo,
`oled texto ola` prova endereçamento de pixel e o DC.

### Sondas que ficaram, e servem a cada solda nova

| comando | o que responde |
|---|---|
| `varre [reset]` | **quais dos 15 pinos livres têm algo que se move** — não supõe mapa |
| `encpino [reset]` | nível cru dos nove pinos, trava nas duas direções |
| `fio <pino> <0\|1>` | segura um pino do OLED para medir no conector |
| `fio curto` | ponte entre os cinco, teste diferencial |
| `oledvel <n>` | prescaler do SPI em tempo de execução |

Todas acumulam desde o reset — **nenhuma abre janela cronometrada.**

### Revisão de pinos entra na rotina de fechamento

`make check-docs` confere `PINOS.md` contra os `seed::Dnn` do fonte, em três
colunas: **no código e fora do doc · no doc e não usado · RESERVADO usado.**
Pedido do dB, e a razão é dele: *a placa está entupida e os pontos de solda
espremidos — conferir de cabeça falha exatamente quando errar custa mais.*

---

## Frente na fila — ampliar os búzios (pedido do dB, 23/08, "mas não ainda")

**Os búzios estão com DUAS cartas** — `dly_time` e `dly_mix_a`, num array que
cabe 8. O manifest já registrava isso como empobrecimento conhecido, herdado
da saída do vocoder do build:

> *"Os búzios ficaram com DUAS cartas em vez de quatro, e o sorteio soa
> proporcionalmente mais pobre. Não há conserto silencioso pra isso — ou volta
> um módulo de caráter, ou a decisão é aceitar o sorteio menor."*

**Há um terceiro caminho que aquele texto não previu:** a duplicação de
caráter do passo 1 criou **dez params novos da categoria certa**. Não é
"randomizar quase tudo" — é **preencher a categoria que já estava definida e
estava vazia.**

### Onde parar já está decidido, e as razões são de música

| categoria | fora | por quê |
|---|---|---|
| **MOLDURA** | reverb | o fundo parado que faz o contraste existir — *se tudo salta, nada é surpresa* |
| **GESTO** | freeze | decisão de execução com hora certa; sorteada vira acidente |
| **SEGURANÇA** | level, gate, governor, limiter, shift | — |

**Candidatos legítimos (20, contra os 2 de hoje):** `drv_drive`, `drv_tone`,
`drv_sujeira`, `phs_rate`, `phs_depth`, `phs_fb`, `phs_stages`, `wrb_rate`,
`wrb_depth`, `wrb_irreg` — nas duas cadeias.

### O que o trabalho realmente é

Subir o teto do array é trivial. **O trabalho é a FAIXA de sorteio de cada
um**, e ela é de ouvido: sortear `drv_drive` no curso inteiro produz do limpo
ao destruído a cada batida. O registro do `voc_pitch` mostra o padrão — faixa
estreita, centro escolhido, e no caso dele até quantização em escala.

E o aviso que o próprio manifest deixou: *"o registro anterior sorteava certo
mas soava tudo grave e parecido — o registro aprovado de ouvido foi o de
cima."* **Faixa de sorteio é decisão de música, e o aceite é do músico.**

---

**Duas frentes pequenas com argumento próprio, ambas de "dar testemunha ao
que só a disciplina protege":**

- **Tabela de pinos RESERVADOS em código**, com `static_assert` sobre os pinos
  efetivamente configurados. Hoje `D29/D30` estão reservados aos DPDT do
  exciter **só em documento — e o compilador não lê documento.** O default do
  RST do OLED é justamente `D30`: se a solda tivesse seguido o default, a
  colisão só apareceria no dia de montar o painel, com o fio na caixa, e nada
  a teria pego antes.
- **`defesas`**: inventário do que nunca foi exercitado (bts 13w), três
  colunas — já agiu · tem gatilho · nenhum dos dois.

**Na fila depois disso:** inventário do que nunca foi exercitado (bts 13w) ·
unificar as duas recusas temporais globais · ponteiros de preset com
pré-escuta · painel no OLED · QSPI (janela ITCM provada, falta a leitura
indireta — não gastar gravação sem o RM0433).

---

## Rota dos presets (22/08) — o difícil resolvido, o tedioso identificado

**A janela ITCM funciona.** A placa sai do memory-mapped executando de RAM
independente, dialoga com o periférico, volta e sobrevive — seis gravações,
`ITCMRAM: 212 B` provando que a seção foi aplicada. **Era o risco
arquitetural da rota dos presets, e ele acabou.**

**O que falta é protocolo**, que é trabalho de leitura: a sequência exata de
leitura indireta do QUADSPI. Três hipóteses vivas e nomeadas, condição de
retomada explícita — tudo em
[DESENHO-ESCRITA-QSPI.md](DESENHO-ESCRITA-QSPI.md).

> **Condição de retomada: RM0433, capítulo QUADSPI, leitura indireta.** Sem
> ele a frente fica parada — por dimensionamento, não por desistência. Meia
> hora com o manual vale mais que dez gravações adivinhando.

O BOOT_SRAM segue estacionado no ramo `investigacao-boot-sram`, com a
hipótese registrada para quem retomar.

---

## Defeitos abertos

**✅ FECHADO 22/08 — a entrada NÃO satura, e o divisor saiu da fila.**

Medido com os piezos **montados na escultura**, que é o regime de uso:

```
p1 (IN_L): pico  -9,2 dBFS | 18,0% das amostras ativas | clip 0
p2 (IN_R): pico -19,5 dBFS |  1,3% das amostras ativas | clip 0
pos-ganho (in_gain 2,8x):   -0,3 dBFS                  | clip 0
```

**Nenhum clip em lugar nenhum.** O plano de oito passos encolheu para dois: a
proteção (1 kΩ + 2× 1N4148), que serve de qualquer jeito porque piezo entrega
dezenas de volts num transiente forte, e recalibrar o `in_gain`.

**A cicatriz 12a acertou o desfecho.** A medição de 20/08 que dizia "a entrada
satura" foi feita com **dedo direto no disco solto** — o caso mais violento que
a pastilha encontra. Montada e excitada por vibração, ela entrega vinte dB a
menos. *"Dimensionar antes de medir no regime certo seria resolver um problema
que talvez não exista"* — e não existia.

**Dois pontos de atenção que a medição deixou:**

**O pós-ganho está a três décimos do teto** (−0,3 dBFS com `in_gain` 2,8×). Uma
batida mais forte clipa. Há folga na entrada, não na saída do ganho.

**Dez dB entre os dois piezos, e p1 com 14× mais amostras ativas.** Pode ser
execução ou acoplamento, e a medição não distingue — a separação é **trocar os
piezos de canal** (cicatriz 12b). Enquanto não se faz, `in_gain_a/b` existem
justamente para casar os dois.

**🔴 A serial morre com o vocoder ligado — prioridade 1.**
As rajadas de ~3,4 ms do callback starvam a ISR do USB; o CDC do host
apodrece. Contornado por `log_mute`, caixa-preta e MIDI, mas o conserto
certo está desenhado e não feito: **ring de log kick-and-forget**, em que
o loop nunca toca o USB e a drenagem acontece por ISR.

**🟡 Vocoder exige bloco 256.** 5,3 ms de latência num instrumento
realimentado desloca onde o feedback se estabelece. Preço aceito, não
resolvido.

**🟢 Off-by-one no contador de aplicadas.** `msgs completas N` vem sempre
**exatamente 1** acima de `APLICADAS`, com e sem carga (65/64 e
2779/2778). Diferença fixa, não taxa de perda — provavelmente a mensagem
em trânsito no instante da leitura. Confirmar antes de chamar de cosmético.

**🟡 Pool SRAM a 92,4%.** Terceiro módulo não entra sem mudar a
estratégia de alocação (rotular alocações quente/fria antes de crescer).

---

## Fila congelada (espera hardware)

T4 (buffer + piezo) · T7e (gate com transiente real) · teste percussivo do
limiter · **solda do painel v2** (5 pots + 2 botões; B2 é candidato natural pro toggle dos
Búzios).

---

## Próximos passos, em ordem

1. **Decisão do dB:** quanto de proteção volta → grava o estado
   instrumento como default do repo.
2. **Ring de log** — mata o defeito prioridade 1 por arquitetura.
3. **Defaults de palco:** `voc_mix`, `voc_tone`, `buzios_thresh`,
   `buzios_cooldown`.
4. **Painel v2** quando o hardware chegar (T11 já testa por serial).
