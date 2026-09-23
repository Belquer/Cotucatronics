# STATUS — Cotucatronics
<!-- doc-check: ignore svg_colisoes (ferramenta em tools/, fora do alcance do check_docs) -->
<!-- doc-check: ignore dentro_do_contorno (ferramenta em kicad/, fora do alcance do check_docs) -->
<!-- doc-check: ignore sincroniza_pcb (ferramenta em kicad/, fora do alcance do check_docs) -->
<!-- doc-check: ignore para_fabrica (ferramenta em kicad/, fora do alcance do check_docs) -->
<!-- doc-check: ignore funcao_dos_pads (ferramenta em kicad/, fora do alcance do check_docs) -->
<!-- doc-check: ignore confere_arcade (funcao do pcb_contorno.py, em kicad/) -->
<!-- doc-check: ignore y_max (valor da constante FRENTE_CEREBRO, no pcb_contorno.py) -->
<!-- doc-check: ignore y_min (valor da constante FRENTE_CEREBRO, no pcb_contorno.py) -->
<!-- doc-check: ignore refresca_redes (funcao do sincroniza_pcb, em kicad/) -->
<!-- doc-check: ignore silk_over_copper (codigo de violacao do DRC do KiCad) -->
<!-- doc-check: ignore silk_overlap (codigo de violacao do DRC do KiCad) -->
<!-- doc-check: ignore starved_thermal (codigo de violacao do DRC do KiCad) -->
<!-- doc-check: ignore lib_footprint_mismatch (codigo de violacao do DRC do KiCad) -->
<!-- doc-check: ignore courtyards_overlap (codigo de violacao do DRC do KiCad) -->

**Onde estamos.** O par deste documento é
[battle_scars.md](battle_scars.md), que diz *o que aprendemos chegando
aqui*. Leia os dois antes de tocar em código.

> Atualizado em **2026-09-06 (a caixa está pronta para a Ponoko)** · leva:
> **`caixa/cotuca_caixa_r2_acrilico_3p0_55cf3955.svg`, 20 peças, pronto a
> encomendar.** O nome leva revisão e **digest do desenho** — a r1 (19 peças,
> sem reforço) foi a encomenda #1263497 e chamava-se igual à r2 antes disto.
> Acrílico transparente 3,0 mm, kerf 0,2, chapa P2, ocupa 377,1 × 326,0 com
> 6,9 mm de margem, ≈ 6,84 m de corte (≈ $40 a $5,87/m, mais envio).
>
> A vigésima peça é o **REFORÇO da tampa** — faixa de 193 × 50 colada por
> baixo, só na faixa dos arcade: 6 mm onde o pé pisa, 3 mm onde os eixos
> atravessam. Furos ø30 concêntricos, verificado. **O protótipo em madeira
> tinha 6 mm e os botões apertaram lá** — foi essa medição, num objecto que
> existiu, que fechou a questão.
>
> Arcade à distância máxima: **x = 30 · 100 · 170**, 70 entre eixos, 40 entre
> bordos. Se um pé largo ainda apertar dois, a resposta é um ressalto colado,
> não mais espaço — não há mais espaço numa face de 200.
>
> ⚠️ **A PLACA NÃO ESTÁ PRONTA, e é independente da caixa.** Os três arcade
> descem 40 mm e passam por cima do cérebro, com **7,5 mm** de altura livre no
> pior caso. **Seis peças não cabem** — `RV1` (10), `C13` (11) e os headers
> `J4 J5 J10 J15` (20 com a ficha e o fio). É colocação, não redesenho.
> Orientação decidida por medição: `FRENTE_CEREBRO = "y_min"`, porque a outra
> punha a Daisy (17) e o `C1` (25) debaixo de botões. `pcb_contorno.py` **v0.7.0**
> tem o portão que mede isto — nenhum DRC o vê. Cicatriz **15ci**.
>
> Fila: mover as 6 peças · furos de fixação (símbolo → esquema → PCB) · rotear
> 139 e 47 · `revisao-terra` · gerbers.

> Atualizado em **2026-09-05 (KiCad para a JLCPCB)** · leva: **o painel nasceu**
> (`kicad/cotuca_painel/`, ERC 0) e o contrato entre as duas placas passou a ser
> verificável — `tools/contrato_idc.py` lê as 26 vias dos dois projectos por
> geometria e confere uma contra a outra; a netlist do próprio KiCad confirma
> 26/26. Duas delas (13 e 14, terra do LED) **nunca tinham sido desenhadas**:
> saíam como `1.7763568394002505e-15` e nenhum leitor com `[\d.]+` as via.
>
> **O OPA1642 estava errado de três maneiras ao mesmo tempo** — pinos 1 e 3
> trocados face ao SOIC-8 real, os dois amplificadores no mesmo encapsulamento,
> e três linhas de BOM para um chip. É agora um símbolo de três unidades, com a
> pinagem tirada do `LM2904` da biblioteca oficial do KiCad. Nada disto se via
> no protótipo, que é ligado à mão.
>
> **29 referências renomeadas** (`R0A`→`R10`…): o KiCad trata uma referência
> sem parte numérica como não anotada e renumera-a sozinho ao gravar. A dezena
> passou a dizer o canal. Provado por comparação de netlist sob o mapa.
>
> **Placa: Daisy Seed3**, conferida pino a pino contra o rev4 — o nome STM32
> bate nos 40 pinos. Codec TI TAC5242, 32 bit / 192 kHz. Símbolo, footprint e
> netlist idênticos.
>
> **Footprints: 77 de 79**, todos os nomes verificados contra as bibliotecas
> instaladas. Faltam os dois pots. **`docs/COTUCA_BOM.md`** é a BOM do
> instrumento inteiro, **gerada das folhas** — ≈ US$ 22 em componentes de placa,
> ≈ US$ 97 com os módulos.
>
> ⚠️ **O que trava a fábrica:** o `OPA1642` na biblioteca de peças da JLC (stock
> da LCSC não é o mesmo armazém), o footprint dos pots, e o valor de `R5`/`R6`
> do painel. **Não há `.kicad_pcb` nenhum** — o layout é o passo seguinte.
>
> Ferramentas novas: `colisoes_sch.py` (o portão dos esquemáticos, que o
> `svg_colisoes` não conseguia ser), `contrato_idc.py`, `bom_completa.py`.
> Cicatrizes 15cb–15ce, e a skill `kicad` de 270 para ~600 linhas.

> Atualizado em 2026-09-04 (limpeza) · leva: **o efeito aposentado do slot
> Fx2 saiu da documentação inteira** — 43 menções em oito ficheiros (`STATUS`,
> `battle_scars`, `FEATURES`, `IFA10000`, `CTOS`, `params-aposentados` e
> três skills). Onde ele era a RAZÃO de uma decisão viva, a razão ficou e
> só o nome saiu: o bloco 48, o defeito 🔴 da serial (que é starvation da
> ISR do USB por callback longo, e não de um efeito), o passa-alta do Fx2
> e as quatro etapas do IFÁ10000 que ele ensinava.
>
> Na mesma leva, **o motor de DSP externo saiu da documentação inteira** —
> mais 53 menções, incluindo 15 no README. Onde o nome era identificador de
> API (as macros da receita de embarque) a receita saiu em vez de ser
> renomeada: macro com nome inventado é pior que macro nenhuma. As três
> directivas `doc-check: ignore` do `FEATURES.md` saíram com os nomes que
> isentavam, e o caminho de volta para os `.maxpat` do dB foi **reprovado
> antes de reescrito** (`git show 8271e97:Max/` lista os quatro; o do delay
> devolve 104 335 bytes). `check_docs` a 0 fantasmas nas duas rondas.
>
> Do processo nasceu a skill **`/audit`** + `auditar.sh` v1.0.0 e
> `versoes_readme.py` v1.0.0. Na estreia contra esta árvore apanhou **5
> links mortos** no `MAPA-DE-TERRA.md` (apontava `../` estando um nível
> mais fundo), uma **entrada duplicada** na lista de aposentados, e **15 de
> 58 versões do mapa do README derivadas** — uma delas com o README à
> FRENTE do fonte (`ponte.ino` v0.5.0 anunciada, v0.4.0 no ficheiro).
> Todas consertadas; o mapa está a 60/60.
>
> Segunda passagem do `/audit` (`auditar.sh` v1.1.0 + `coerencia.py` v1.0.0):
> **duas das minhas próprias checagens estavam a acusar pelo critério errado**
> — contavam ocorrências simples quando o `check_docs` só vê nomes **entre
> crases**, e supunham escopo de linha quando `isentos()` varre o ficheiro
> inteiro. Reescritas em Python. As **6 isenções órfãs** foram a zero sem
> perder história: `oct_on` e `test_tone_on` **migraram** para a lista de
> aposentados (52 → 54) por carregarem registo único; as outras quatro eram
> cópias de um registo que vive no `battle_scars`. O **vault** ficou alinhado
> nos três ficheiros em atraso — o `HARDWARE.md` de lá ainda tinha os nomes
> velhos da matriz (`route_l_a`), renomeados a 02/09.
>
> Passada nova de **itens pendentes**: 186 linhas marcadas na árvore. Fechou-se
> o **⚪4 do `AUDIT-23-08`** (versões velhas no README), com prova; e o **⚪5**
> ficou registado como **aberto e maior**: o `"76 CCs"` está hoje em três
> comentários do `main.cpp`, não dois, e o número real são 119.
>
> **04/09, fecho: a limpeza acabou, ponta a ponta.** Com a decisão do dB:
> `src/TESTES.md` (v0.25.0) e `src/BOARDS.md` (v0.14.0) limpos sem esperar
> pela outra sessão, e **os 86 comentários de fonte** em 13 ficheiros. O build
> prova que foram só comentários: **QSPI 283 196 B, SRAM 322 488 B, ITCM 308 B
> — idênticos ao baseline**, antes e depois.
>
> Saíram três **receitas falsas** — mandavam usar ficheiros que não existem:
> a "bancada" do `BOARDS.md`, o bloco de revivência do `Makefile` (v0.4.1), e
> os binds comentados do `manifest.h` (v1.12.2), onde vivia a afirmação de que
> os params do módulo **continuavam na tabela**. Não continuavam, e foi ela
> que me mandou procurar código que não existe. Está lá agora a dizê-lo.
>
> O **T12** do `TESTES.md` (98 linhas, o ensaio de coincidência espectral)
> virou lápide de 8 linhas: as duas lições que ele comprou vivem na etapa 6 do
> IFÁ10000 e no cap. 6 do battle_scars, que é onde servem a qualquer efeito.
> O mapa de CC à mão do `BOARDS.md`, que dizia "instrumento atual" e listava
> quatro params mortos, foi **removido em vez de corrigido** — apontar para o
> `CCMAP.md`, que é gerado, em vez de manter viva a tabela que mente.
>
> A cauda do README foi consertada: os defeitos em aberto passam a espelhar
> este documento, o signal path deixa de estar "à espera de hardware", o Pot 5
> deixa de fazer um crossfade aposentado a 20/08, e os "próximos passos"
> apontam para aqui em vez de duplicarem uma lista que volta a derivar.
>
> **⚪5 do `AUDIT-23-08` fechado:** o `"76 CCs"` estava em três comentários do
> `main.cpp` (v0.301.1), não dois, e o número real são 119. Corrigido nos três.
> **🟡2 estava fechado e ninguém tinha dado por isso.** Fui escrevê-lo e
> encontrei-o feito: `Colisoes()`/`Colisao()` no `manifest.h`, chamadas em
> `main.cpp:5134`, e o `board` a imprimir `colisao de CC`. Implementado depois
> do audit de 23/08, com a entrada por fechar. Construir por cima teria sido
> trabalho a duplicar código que já existia.
>
> **05/09 — placa nova gravada, e os três params invisíveis deixaram de o ser.**
> A Daisy nova entrou (uma unidade só, por agora), com o bootloader já lá — o
> DFU que apareceu expunha `0x90000000`, a QSPI. Gravada e conferida nos dois
> lados: fonte e placa em **v0.302.0**.
>
> O `board` acusava `sem CC: 3 de 122 params — alcançáveis por `set`,
> invisíveis ao iPad`: `sim_laco`, `sim_hz`, `sim_fb`, o banco de ensaio do
> supressor. Ganharam **CC 107/108/109** em `ch1`, colados ao `bench_boost`
> (105/106) porque são a mesma família — coisas de bancada, não de palco, e a
> vizinhança é defesa: um widget mal configurado atinge o vizinho. Escolhidos
> livres nas **duas** fontes (fonte e `CCMAP` gerado) e fora de 120–127
> (Channel Mode), 0/32 (Bank Select) e 32–63 (os LSB de 0–31).
>
> Medido na placa depois de gravar: **`sem CC` 3 → 0 · CCs 119 → 122 ·
> `colisao de CC: nenhuma`** — dita pelo detector que já lá estava, que é
> exactamente o trabalho dele. ⚠️ **Não há widget no Loopy para estes**, por
> decisão do dB: o CC existe para o param deixar de ser invisível, não porque
> haja página à espera.
>
> **E o `check_docs` acusou-me 10 ficheiros "tocados sem subir a versão"** —
> os do commit `1cc896c`. Olhados, não presumidos, que é o que a docstring
> dele pede: são **comentário puro**, e a prova é o binário byte a byte igual
> (QSPI 283 196 B · SRAM 322 488 B · ITCM 308 B, antes e depois). **Não
> bumpados de propósito:** `hal`, `params` e `dynamics` reportam versão no
> `board`, e subi-la anunciaria uma mudança que não existe — além de obrigar a
> regravar para nada. Fica escrito para não se voltar a discutir.
>
> **Uma contradição minha, apanhada na varredura de pendentes.** Ao escolher
> os CCs escrevi que 120–127 são Channel Mode e não se usam. Mas o projecto
> usa **120/121/122 de propósito** como CCs de COMANDO (`sync`, preset load,
> preset save) — não são params, por isso não entram no mapa. O comentário
> lia-se como proibição geral e passou a dizer o que é: a faixa alta não está
> livre, está reservada a outra coisa.
>
> **O `/audit` foi apertado contra as duas falhas desta sessão** (`auditar.sh`
> v1.2.0). A primeira: varri o fonte com `*.h *.cpp Makefile` — 33 ficheiros —
> e a árvore tem **137 rastreados em 14 extensões**; o `check_repo.sh` escapou
> e só apareceu por acaso na varredura final. O inventário passa a ser por
> `git ls-files`, e imprime as extensões da árvore ao lado, para que tipo novo
> apareça à vista. A segunda: a **frase partida pela renomeação** — âncora
> única, `grep` limpo, `make` a passar, e a frase a ficar *"morre sob callback
> ligado"* porque a continuação estava na linha seguinte. Nasceu o
> `vizinhos.py` v1.0.0, que lê o diff e assinala só onde a edição tocou o FIM
> da linha. Corrido sobre esta sessão: **16 vizinhanças, todas lidas, nenhuma
> outra partida.** E a varredura exaustiva confirma **zero em 137 ficheiros**.
>
> ⚠️ **A cauda do README está podre por conta própria** e a limpeza só a
> destapou: dizia "Nenhum defeito conhecido" com um 🔴 prioridade 1 aberto;
> dizia que gravar "exige o botão BOOT" quinze linhas depois de anunciar o
> `flash.sh` a entrar por `dfu go`; dava o signal path como "ainda não
> implementado, hardware a caminho"; e apontava três vezes para
> `src/modules/README.md`, que não existe. **Isto não foi consertado** —
> está fora do pedido, e é decisão do dB.
>
> Antes, 2026-09-04 (deep clean) · leva: o supressor passa a
> chamar-se **SuMiA™** em todas as instâncias · **deep clean da documentação**
> — o README declarava dois estados incompatíveis a quinze linhas um do outro
> ("bloco 48, sem motor externo" e um módulo de FFT ligado com bloco 256),
> anunciava
> v0.172.0 com
> o fonte em v0.301.0, e dizia que os pots e botões não estavam soldados · a
> pasta `Max/` (1,8 MB de patches aposentados) saiu da árvore com o caminho de
> volta provado · o `CCMAP.md` tinha o cabeçalho à mão a envelhecer por cima
> de uma tabela gerada · e dez documentos vivos entraram no mapa do README
>
> ⚠️ **HARDWARE NOVO: o pré-amp está montado e testado** (04/09), com som mais
> cheio. Isso invalida toda a medição de ganho anterior — §14 da
> `medicao-limpa`. A sessão de escuta estava a começar quando o deep clean a
> interrompeu; **o pré-voo tem de ser refeito do zero.**
>
> Antes, 2026-09-04 · leva: **gravar passou a repor o estado** —
> `flash.sh` v0.4.0 + `estado.py`, guardando antes do DFU e repondo depois,
> porque a regra existia na skill e vivia só na minha cabeça · a **marca da
> abertura refeita em três peças** (casco vazado, lábio, boca) contra
> desenhos e renders do dB, com **espelho cima↔baixo a 0 px de 1215** e o
> casco provado fechado por **alagamento** · e o **exame de pinos** a apanhar
> o `PINOS.md` a contradizer-se a si próprio
>
> **05/09 — o teste do terra no documento dono estava velho.** O
> `ATERRAMENTO.md` (v0.2.0) é, pelo README, o dono do assunto, mas a sua
> secção *Conferência* ainda mandava medir a ilha contra o trilho e dizia *"se
> apitar, há uma ligação a mais"*. **Com o net-tie do revB, apitar é o
> correcto** — o teste velho faria ler uma placa certa como defeituosa. Passou
> a espelhar o `MAPA-DE-TERRA.md` §6: três medições e por ordem, porque o
> teste do meio sozinho não separa *"ponte única"* de *"nenhuma ponte"*, e uma
> delas mata o áudio. A premissa (20↔40 aberto na Daisy nua) está **medida em
> 04/09**; o que continua por medir é a placa montada.
>
> ⚠️ **Continua por confirmar com o multímetro** o `AGND`/`DGND` unidos fora
> da placa (P3 da varredura de terra) — o dB refez o aterramento em 03/09 e
> reportou som mais limpo, mas a medida não foi feita.
>
> Antes, 2026-09-02 (fecho) · leva: **compressor** (49 dB de factor
> de crista, medidos) · **medidor de saída** `pkout` · e o **chiado de banda
> larga** diagnosticado até uma regra com nome — `AGND`/`DGND` unidos fora
> da placa, P3 da varredura de terra, **por confirmar com o multímetro**
>
> Antes, 2026-09-02 (noite) · leva: **SuMiA™** — supressor de
> microfonias automatizado, por exciter, quatro bocas, provado num **banco
> de ensaio** sem a escultura · o notch mudou de eixo (cadeia → **ramo**) ·
> **rampa de arranque supervisionada**, que recua quando apanha um modo ·
> e três incidentes de microfonia que ferem, todos meus
>
> Antes, 2026-09-02 (fim do dia) · leva: **a marca da abertura
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


## 19/09/2026 — cérebro #1 na bancada: o que foi montado diferente da BOM

A montagem do primeiro cérebro está em curso, pelo manual
(`docs/montagem/manual_montagem.html`). O que este exemplar tem de diferente
da BOM fica aqui, para ninguém o ler depois como erro de montagem:

- **`C14`/`C24`: eletrolítico, não filme.** A BOM pede WIMA MKS2 2,2 µF 63 V
  com passo 5 mm; não foi comprado (o dB contava com o stock do lab) e o
  único 2,2 µF do lab era um filme de 400 V, grande de mais para o passo
  5 mm e o corpo de 7,8 × 11,6 mm. Montou-se um **eletrolítico**, com o **+ no
  lado do `R14`/`R24`** (VBIAS, ~7 V; a entrada da Daisy fica sempre abaixo), e
  a polaridade foi conferida pelo dB. ⚠️ **Valor e tensão do eletrolítico
  montado: por anotar.** O que muda: nada audível se ≥ 2,2 µF; o manual
  (v0.3.0) passa a aceitar esta troca, com o + marcado no desenho, e proíbe
  cerâmico multicamada (microfónico, e perde capacitância com 7 V em cima).
- **Ordem dos botões arcade na tampa: azul A · azul B · branco**, com o branco
  do lado do capacitor alto — decidido na bancada porque o branco (búzios) não
  tem LED e é mais curto. Medido pelo dB a 19/09: **o branco desce 20 mm abaixo
  do painel, com conectores**; o capacitor é o **C1, 2200 µF**.
  A `confere_arcade` (`pcb_contorno.py` **v0.24.0**) passou a ter **uma
  profundidade por botão** e **altura de electrolítico por diâmetro** (D16 = 25,
  D10 = 16 mm, de catálogo). Com isso a altura livre é **7,5 mm** sob um azul e
  **21,5 mm** sob o branco.
- ❌ **ERRO MEU, corrigido no mesmo dia.** Escrevi que a Daisy debaixo do botão
  central provava a orientação declarada. **Não prova nada:** o `U2` cai sob o
  furo do meio nas **duas** orientações — medi as duas e só depois vi. Um facto
  que é igual nos dois lados não distingue os dois lados.
- 🔴 **O cérebro está montado GIRADO 180° face ao modelo (dB, 19/09).** A prova
  é a serigrafia: com o lado do utilizador virado para si (logo de frente) e a
  olhar de cima, a serigrafia do **painel** lê-se direita e a do **cérebro** lê-se
  **de cabeça para baixo**. Isso é a orientação `y_max` — a que foi **rejeitada**
  a 06/09 exactamente por pôr peças altas sob os botões. E nessa orientação o
  verificador acusa, medido hoje:
  · **`C1` (2200 µF, 25 mm)** sob o furo **1** — bate com o capacitor alto que
  obrigou a trocar a ordem dos botões, que era o que faltava explicar;
  · **`U2` (17 mm)**, **`J13` (1×3)** e **`J16` (1×8)** sob o furo **2** — os dois
  headers contam **20 mm** com ficha e fio.
- 🔴 **E nada impedia:** os quatro furos de fixação do cérebro (`H1`–`H4`, ø3,2)
  formam um **retângulo simétrico** — medido a 19/09 — por isso a placa aparafusa
  nos dois sentidos, e o sentido errado não tem resistência mecânica nenhuma. O
  `FRENTE_CEREBRO` é uma **constante declarada**, e o `encaixe.py` até **recusa**
  se ela não for `y_min`: a ferramenta acredita na declaração e nunca pergunta à
  placa. **Para a rev D isto é requisito:** a fixação tem de ser assimétrica (ou
  ganhar um recorte de chave), para a orientação errada **não entrar**.
- ⚠️ **Por decidir, e é o dB que decide:** girar o cérebro para a orientação do
  modelo (e ver se os conectores ainda alcançam as paredes), ou assumir a
  orientação girada e **recolocar as peças altas na rev D**.
  E o dB diz que a Daisy **cabe, apertada** — o que mede o que faltava: o
  verificador dá **7,5 mm** livres sob um azul e a Daisy tem **17 mm**, logo a
  profundidade do botão azul **não** são os 34 mm do pior caso. Para a Daisy
  caber, o azul desce **~24 mm** abaixo do painel. ⚠️ **Por medir:** a
  profundidade real do botão azul (a do branco, 20 mm, já foi medida). Enquanto
  não for medida, a `confere_arcade` reprova `U2`, `J14` e `RV1` — e reprova
  **por excesso de cautela**, que é o lado certo para errar.
- ⚠️ **FICA ABERTO: qual é a peça alta debaixo do botão branco.** Com a
  orientação confirmada, o `C1` (2200 µF) está a **64 mm** do furo mais próximo
  — não pode ter sido ele. Debaixo do furo da esquerda está a **caixa de pinos
  `J14`** (8,85 mm, mais a ficha do cabo IDC por cima); debaixo do da direita só
  há peças rasteiras. **Por medir:** o que está debaixo do botão branco.
- ✅ **O amplificador ganhou casa (dB, 20/09): prateleira sobre o cérebro, em
  4 torres M3 COLADAS a quente**, na metade de trás (pontos a 114 · 175 mm da
  esquerda e 5 · 95 mm do topo). O `onde_furar.py` v0.3.0 dizia que os quatro
  pontos **passavam com parafuso isolante** e reprovavam com metálico (caem no
  plano `GND`, em campo largo, com 4,7 a 11,1 mm de folga ao pad mais próximo).
  **O dB escolheu não furar:** a placa é insubstituível a meio da montagem, e o
  ganho de aperto não pagava o risco. Colar é reversível; furar não é.
  ⚠️ **O calor não chega por contacto** (nota corrigida a 20/09, com a razão do
  dB): o amplificador aparafusa no **topo** da torre e a cola fica na **base**.
  Sobram dois caminhos fracos — condução pela torre (os furos do módulo são a
  parte fria dele; uma anilha de nylon entre módulo e torre corta isto de vez) e
  o **ambiente da caixa fechada**, que é o que realmente mede. Cola EVA amolece
  a 60–80 °C. **Teste barato:** meia hora a tocar alto e o dedo na base da
  torre — cinco segundos sem largar ≈ abaixo de 50 °C, confortável.
  ⚠️ **E o risco maior é mecânico, não térmico:** cola quente é forte em
  cisalhamento e fraca em descascamento. Um cabo esticado descola a torre antes
  de qualquer temperatura — folga nos fios, e cordão em volta de todo o pé.
  O `encaixe.py` M7 já aceita esta casa; falta o **step-down**.
- 🔴 **A caixa não tinha lugar definido para nenhum módulo (20/09).**
  O `CAIXA-ACRILICO-MONTAGEM.md` §6 diz *"módulo do amp nos dele [espaçadores]"*,
  mas esses espaçadores nunca existiram: a chapa de fundo tem **4** furos, e são
  os do cérebro. E não há chão: a chapa é **193 × 113** e o cérebro ocupa
  **180 × 100**, o que deixa faixas de **6,5 mm** à direita e atrás — não cabe
  módulo nenhum ao lado. Medido a 20/09.
  O volume que sobra é **por cima do cérebro**: **31,4 mm** na frente e
  **56,4 mm** atrás (interior menos calço 6 + espaçador 10 + placa 1,6). Atrás
  é onde cabe, e é também para onde as regras de cabo já mandam o amp (os
  exciters saem pela direita, a linha da mesa vem do DPDT da parede de trás).
  ⚠️ **Por medir antes de decidir:** as dimensões do módulo (com dissipador e
  bornes), o padrão de furos dele, e a altura do `C1` já montado — um segundo
  andar por cima do cérebro tem de passar por cima dos 25 mm do `C1`.
  ⚠️ **E o calor:** a caixa é acrílico fechado. Um classe D a 15 V dentro de
  caixa selada não foi discutido em lado nenhum do repo.
- ⚠️ **O painel só sai com os jacks TRS fora (dB, 19/09).** Com os TRS
  montados o painel não tem por onde sair: a ponta do encoder fica presa no
  acrílico da tampa. Ordem de montagem e de manutenção: **o painel entra e sai
  antes dos jacks**. Ver `caixa/LICOES-MONTAGEM.md` #11.
- ⚠️ **(histórico) DIVERGÊNCIA — a orientação do cérebro na caixa.** O modelo declara
  `FRENTE_CEREBRO = "y_min"`, e **nessa** orientação o `C1` fica a **64 mm** do
  botão mais próximo: nada alto sob os furos. O `C1` só cai debaixo de um arcade
  na orientação **oposta** (`y_max`) — que foi rejeitada em 06/09 exactamente
  por pôr o `C1` e a Daisy sob botões. Ou a placa montada está virada ao
  contrário do modelo, ou as cotas dos furos não são as do desenho. **Não
  registei como facto até medir**, porque muda o que a rev D tem de resolver.
  Na orientação `y_max` o verificador também acusa **U2 (Daisy, 17 mm)** e os
  headers **J13/J16 (20 mm com ficha)** debaixo de botões azuis, onde só cabem
  7,5 mm. **Por medir:** que peça está debaixo de cada um dos três furos, e a
  altura real do `C1` (o catálogo diz 25 mm, e só 21,5 cabem sob o branco).
  Ver `caixa/LICOES-MONTAGEM.md` #10.
- **Painel, estado da primeira ligação (22/09):** alimentação **5 V e 3,3 V**
  boas · os **três botões** respondem (o branco dos búzios é **NF**, exceção
  declarada no firmware, `hal` v0.76.0) · os **dois pots** lidos pelo ADC ·
  **LEDs dos arcade a funcionar** depois de trocar o chicote comprado, que vinha
  com as cores cruzadas (bts 15fh).
  **Cabo do OLED, cores levantadas na peça (dB, 22/09)** — a Waveshare não as
  publica, a wiki dela só lista nomes de pino: `J2` pino 1 = **vermelho** (+5 V)
  · 2 **preto** (GND) · 3 **azul** (DIN) · 4 **amarelo** (CLK) · 5 **laranja**
  (CS) · 6 **verde** (DC) · 7 **branco** (RST). A ordem do cabo coincide com a
  do `J2`, logo é ligação direta — contando o pino 1 pelo **pad quadrado**, que
  com o header soldado por baixo fica do lado oposto ao que a serigrafia sugere.
  ✅ **O encoder está bom (22/09): 19 pulsos em 20 cliques, 3 pushes em 3.**
  Contava 1 em 20 porque o laço principal estava a 1 volta/s, estrangulado
  pelo painel partido — e o `Debounce` da libDaisy quer 1 kHz. Consertado o
  envio do SPI, passou a contar **sem lhe tocar** (`battle_scars` **15fi**).
  **O sentido está certo como está (dB, 22/09):** o encoder foi soldado na
  placa, e a orientação que ele dá é a que vale. O que estava errado era a
  minha leitura, não a peça — `enc inverte a` **não** se corre, e o
  `enc_sinal` fica em `+1` no fonte.
- ❌ **Módulo OLED nº 2 morto: o CONVERSOR DE ALTA TENSÃO dele (22/09).**
  Medido no canto da alimentação do módulo, contra o `GND` dele: **nada acima
  de 5 V**, onde tinham de estar 12–13. O SSD1309 não acende o vidro com os
  5 V da entrada. Isso explica o quadro inteiro — lógica viva, dados a chegar
  com a forma certa, vidro sem tensão para acender — e **nenhuma medição do
  lado do fio podia lá chegar**. O caminho até ao módulo está provado bom de
  ponta a ponta: sinais, ausência de pontes e curtos, 5 V e terra nos pinos
  dele, cada sinal no nome certo, selecção de interface em SPI, e o barramento
  a transmitir (1,5 V de média no `CLK` e no `DIN` com 85% de ocupação).
  **O módulo novo entra num caminho todo medido.** Ver `battle_scars` **15fm**.
- ✅ **A placa já não pode matar o módulo dessa maneira (OS 0.312.0).** Três
  guardas, e as duas últimas só apareceram por se ter ido exercitar a
  primeira: (1) não conduz o primeiro pino sem confirmar que o módulo responde
  com alimentação; (2) **larga o barramento quando o painel adormece** — antes
  a guarda só protegia o arranque; (3) os comandos da consola armam sono de um
  minuto. ⚠️ Rev D leva na mesma resistores em série nos cinco sinais
  (`ppc/004 §3b.4`): o firmware é remendo, o cobre é o conserto.
- ✅ **Dívida das colisões do manual: FECHADA a 22/09 — 78 de 78 figuras
  passam o portão.** E a medição antiga ("15 dos 47") estava a ser feita com
  caixas erradas: os SVGs não declaravam `xmlns`, o verificador não achava o
  `<style>`, e modelava todo rótulo centrado como se começasse no ponto. O que
  se consertou foi dos dois lados — o `manual_montagem.py` v0.23.0 passou a pôr
  **todos** os rótulos (peças, marcas, furos e vazios) no mesmo motor de
  colocação, com teste contra as arestas reais do contorno e varredura de
  recurso; o `svg_colisoes.py` v0.9.0 deixou de adivinhar CSS em silêncio. Ver
  `battle_scars.md` **15fl**.
- ✅ **`tools/portoes.py` v0.1.0 — um comando corre os 16 portões de leitura.**
  Nasceu do pedido do dB de 22/09 (*"verifique que as bts estão sendo devidamente
  implementadas e não só registradas"*): 48 ferramentas, nenhuma corrida em
  conjunto, e **três vermelhas** no primeiro varrimento. Traz catraca de dívida —
  50 fantasmas de documentação e 10 achados de encaixe da rev C têm a linha
  declarada; acima dela reprova, abaixo manda baixar a linha.
- ✅ **22/09: o cérebro toca.** Piezo → pré-amp → Daisy → delay e reverb →
  saída de linha, nos **dois** canais, sem clip em lado nenhum. O que estava
  entre a montagem e este parágrafo está em `battle_scars.md` **15fd**, **15fe**
  e **15ff**: ponte com pinos errados, três soldas frias em cadeia, e o divisor
  de entrada que nunca tinha sido medido com o piezo real.
- **Estado medido deste exemplar (22/09), para não se reinventar depois:**
  · piezos: **18 nF** (corda de metal, entra no `p1`/`J5`) e **17 nF**
  (estrutura junto à pele do tambor, entra no `p2`/`J6`)
  · `C12` e `C22`: **22 nF de fábrica + 100 nF C0G soldado por cima** →
  atenuação de **−17,8 dB** por canal
  · picos tocando forte: **−1,4 dBFS** no `p1` e **−15,6 dBFS** no `p2`, `clip 0`
  · `pre_level_ab` = **0,5** (era 1,0; a soma interna estourava em +5,5 dBFS)
  · `k1`/`k2` = 0,60 · `post_level_l/r` = 0,88 · saída em **−12,7 dBFS** de pico
  ⚠️ **A assimetria de 14 dB entre os canais é do INSTRUMENTO**, não defeito:
  corda de metal e pele de tambor são fontes diferentes. Quem "corrigir" isso
  desfaz o ajuste.
  ⚠️ **Por decidir:** o `p1` está a 1,4 dB do teto. O firmware recomenda mais
  6 dB — seria trocar o 100 nF do `C12` por 220 nF. Não feito ainda.
  ⚠️ **Tudo isto vive na RAM:** qualquer regravação da Daisy repõe os params a
  zero, e só os capacitores ficam.
- **DPDT, a fiação que já funcionou (dB, 21/09, descrita da peça montada):
  3 fios e UM jumper, chave usada na DIAGONAL.** `A1` = linha (mixer) ·
  `B1` **vazia** · `A2`–`B2` jumpeados = o comum, que recebe o sinal ·
  `A3` **vazia** · `B3` = amp (exciter). Nas três posições dá **só linha**,
  **os dois**, **só amp**. Na placa: `J15` pino `1` → comum, `2` → `A1`,
  `3` → `B3`; `4`/`5`/`6` não se usam, `7`/`8` são dreno.
  ⚠️ **A verificar com o dB:** ele anotou o comum como *GND*. Nesta placa o
  comum é `OUT_L_DAISY` (o sinal) — se fosse GND, a linha e o amp nunca
  receberiam sinal, porque `LINHA_L` e `AMP_IN_L` só se encontram com a saída
  da Daisy **através da chave**.
- **(histórico) a leitura anterior: 3 fios e 3 jumpers**, com
  **nenhum terra na chave**. Medido na placa: os três sinais estão
  **duplicados** no header — `1`=`4` (`OUT_L_DAISY`), `2`=`5` (`LINHA_L`),
  `3`=`6` (`AMP_IN_L`) — porque os dois polos trabalham em paralelo, e é isso
  que dá a posição do meio com **as duas saídas**. Logo **6 fios sem jumper** e
  **3 fios com 3 jumpers** são a mesma ligação: na primeira o paralelo é feito
  no cobre da placa, na segunda é feito na chave.
  O desenho do manual mostrava só a de 6 fios e por isso pareceu divergir do
  que estava montado; passou a mostrar as duas, com os jumpers desenhados nas
  três linhas.
- **Amplificador: energia pelo `J2`, áudio pelo `J11` (dB, 20/09).** O módulo
  em mão alimenta-se por **borne de parafuso**, então o `J2` leva a força
  (+15 V/GND, fio curto e grosso — é retorno de força, regra da estrela) e o
  `J11` leva o áudio (2 = referência, 3 = L, 4 = R). ⚠️ **O `J11` pino 1 NÃO se
  liga:** é o mesmo +15 V, e ligar os dois põe a força em paralelo por dois
  caminhos. Declarado em `encaixe.py` **v0.5.0** (`COMPLEMENTARES` +
  `VESTIGIO_PINO`), com o portão a exigir a declaração pino a pino — declarar só
  o par seria calar a duplicação em vez de a resolver. A rev D encolhe o `J11`
  para 1×3.
- **Step-down em mão: `GND IN` e `GND OUT` são o MESMO nó** (bipado pelo dB,
  20/09). Logo vai **um fio só** ao `J3` pino 2, do lado `OUT−` — que é o que o
  `ATERRAMENTO.md` pede (fio de terra dedicado; o retorno do 5 V por outro
  caminho já custou um clique a ~3 Hz).
- **`U1` (OPA1642AID da Mouser) marca o pino 1 com uma faixa na ponta**, a ligar
  os pinos 1 e 8, não com ponto. Confirmado pelo dB com o teste de diodo no chip
  solto (vermelha no pino 4 = V−, 0,4–0,8 V para os outros). O manual (v0.2.1)
  descreve as duas marcas e o teste.

## 17/09/2026 — um ramo só, e o `J2` é o da placa que foi à fábrica

O ramo `claude/slack-session-sm2ldz` (rev C, pacotes e encomendas de 12/09) foi
**fundido no `main`**, e os dois ficaram iguais. A placa, as posições e o `J2`
vêm da rev C; do `main` ficaram as guardas (malha a 1,27, courtyard contra o
bordo, `sincroniza_pcb` na cadeia, guarda do `liga()`) — nenhuma contradiz a
placa encomendada.

**O `J2` do OLED:** header **macho** 1×07 a 2,54 na placa, onde entram os
jumpers **fêmea** do cabo que vem com o módulo (MX1.25 do lado dele). Ordem
`1 VCC · 2 GND · 3 DIN · 4 CLK · 5 CS · 6 DC · 7 RST`, que é a do módulo lida a
partir do VCC: **com jumpers soltos o contrato é o nome de cada fio**, e os sete
batem. Fontes: página Waveshare «Control Interface» e o dB, 17/09. O PicoBlade
`RST…VCC` que o `main` teve de 08/09 a 17/09 não chegou a nenhuma placa
fabricada (cicatriz 15ew).

**Provado sobre as placas encomendadas** (commit `06db1c7`, JLC
`W2026091304545438`): gerador fundido reproduz a netlist do painel (24 redes, 0
diferenças) · `funcao_dos_pads` 0 falhas nas duas, com o contrato do `J2` a
reprovar sob sabotagem e o contrato PicoBlade velho a reprovar 6 de 7 · IDC 26/26
· ERC do painel 0 · courtyards: cérebro 0, painel com o `J1` a **0,405** do bordo
esquerdo (regra 0,5; o corpo fica dentro) — isenção com o número, e na próxima
revisão `J1 x ≥ 26,1`.

**Encomenda JLC `W2026091304545438`: as duas placas, cérebro e painel**
(confirmado pelo dB, 17/09). **Enviada a 17/09**: 5 de cada, dos pacotes
`06db1c7` — as mesmas placas medidas acima. DHL Express, tracking `7099774990`.

⚠️ **Na montagem, `Q1`/`Q2` do painel:** o mapa do kit diz `AO3400A`; solda-se o
**`SI2302CDS-T1-GE3`** que veio da Mouser (marcação **N2**). Mesma pinagem, 1 G ·
2 S · 3 D, conferida na ficha Vishay doc. 68645 (17/09). Ver a nota no MANIFEST.

⚠️ **O painel não tem por onde ser preso (18/09).** Sem furos de montagem; o
encoder `EC11E15244G1` não tem rosca e os pots foram para a tampa. **Reprovado em
M2 da trilha M do IFÁ10000** (certificação mecânica placa↔caixa, nova, na `/aipcb`).
Pontos furáveis medidos, em mm do canto superior esquerdo, lado dos componentes:
**(77,5 · 32,0) · (109,0 · 32,0) · (146,0 · 4,0) · (172,0 · 32,0)** — M3 com anilha,
fora do cobre de outras redes, em acrílico inteiro da tampa. Os primeiros 60 mm
ficam em balanço. A tampa leva os furos a condizer (a caixa cede à placa).

**Pots do painel (18/09):** a placa tem o layout Bourns PTV09A de montagem
traseira (3 pinos a 2,5 mm, patas a 8,8 mm entre centros, 7,0 mm da linha dos
pinos à das patas — conferido na ficha Bourns PTV09). Os pots em casa são de
16 mm e não cabem; os da Mouser nunca foram comprados. Para os pots **prenderem
a placa** à tampa têm de ter **bucim roscado** (Bourns estilo `-6`, ou um 9 mm
genérico com M7): o furo de ø7 da tampa abre-se para passar a rosca, e as três
cotas acima medem-se na peça antes de soldar. **10 kΩ linear (B10K)** — os 100 kΩ
do projecto são os trimpots do cérebro.

**Cabo painel↔cérebro (18/09):** as duas pontas são a caixa de pinos macho Würth
`61202621621` (4 compradas). O cabo é **IDC 2×13 fêmea-fêmea, com chave,
directo (pino 1 ↔ pino 1), ~300 mm**: percurso medido ~170 mm pela parede de
trás (`J1` do painel a 98 mm da frente, `J14` do cérebro a 19) mais os 80 mm de
folga que a caixa pede para tirar o fundo. O BOM dizia ~150, estimado.

⚠️ **Cérebro rev C reprova em M6 (`encaixe.py`, pelo corpo): três peças dentro
dos discos dos arcade**, contra o raio do FURO (15) e 7,5 mm de folga: `J14`
(8,85 mm) a **11,6** do centro do arcade da esquerda, trimpot `RV1` (10 mm) a
**13,3**, Daisy `U2` (17 mm) a **14,8** do do meio. O corpo do botão por baixo da
tampa é mais estreito e nunca foi medido — **medir o diâmetro do corpo roscado e
do micro-interruptor** decide: pode absolver o `U2` e talvez o `RV1`; o `J14` é o
que mais provavelmente bate. Pela prioridade 2, o que cede é a caixa (mover o
arcade na tampa), não a placa fabricada.

**Portão novo (18/09): `kicad/encaixe.py`**, só leitura, chamado pelo
`para_fabrica` — hoje recusa as duas placas: o painel em M2, o cérebro em M6.

**Painel, M6 medido pelo dB (18/09): o header do OLED não cabe por cima** — nem com
jumpers direitos, nem em L. **Resolvido: header soldado por baixo, jumpers em L
para trás e para baixo** (`caixa/LICOES-MONTAGEM.md` #7). Âncora de pinos, visível
dos dois lados: **o pad quadrado é o VCC**, e daí `VCC · GND · DIN · CLK · CS · DC ·
RST`; confirmar por continuidade com o `TP1` (+5V) e o `TP3` (GND) antes de ligar.
Não repetir no `J1` nem nos JST dos LEDs.

**`J1` (IDC) também não cabe por cima** (bate no OLED e no aperto do encoder — dB,
18/09). **Virar a caixa de pinos para baixo dá 0/26 ou 2/26 pinos certos e, em
duas combinações, +3V3A em curto com GND** — simulado. Solução: **fita soldada
directamente nos pads do painel, por baixo**, ficha IDC só do lado do cérebro;
procedimento e continuidade em `caixa/LICOES-MONTAGEM.md` #8. Virar nas **duas**
pontas dá 26/26 em 8 de 16 combinações, mas por baixo do cérebro só há 9 mm.

**O encoder real tem rosca (EC11 genérico, corpo 6,5 mm) e fixa a folga
placa↔acrílico em 6,5 mm.** Tudo o que é mais alto na face de cima bate: `J3`–`J7`
e o `C3` vão por baixo (os LEDs `J6`/`J7` com **fios soldados**, não JST); **os pots
não podem ir na placa** — ficam na tampa, fiados, e os de 16 mm da gaveta servem
(não é preciso comprar os RK097). Tabela e âncoras em `LICOES-MONTAGEM.md` #9.

**`J4` do cérebro («a ponte ESP32») é vestígio (18/09):** a tomada da ponte de
quando era por fio; a ponte está no soquete `U3` desde 06/09. **Fica vazio**;
na próxima revisão, sai. Declarado no `encaixe.py` (M1) — cicatriz 15ez.

---

## ⚠️ 08/09/2026 — «sincronizado» nunca quis dizer o que eu li

O `sincroniza_pcb` escrevia a rede nos pads **só ao acrescentar um footprint
novo**. Para as peças já na placa, nada refrescava nada: a placa guardava as
redes do dia em que a peça entrou.

Seis defeitos foram consertados no esquema, o comando correu, disse **«já estão
sincronizados»**, e a placa ficou com as vinte e duas falhas — o `EXC_R` ainda
lá estava.

> **O relatório dele não mentia.** Ele compara *footprints* — quais existem,
> referência, valor — e sobre isso estava tudo certo. Um verificador responde à
> pergunta que lhe fizeram, e a pergunta era outra.

⚠️ **Isto muda como se lê o histórico:** **nenhuma alteração de esquema alguma
vez chegou a uma placa já roteada por esta via.** Não deu na vista porque, até
hoje, cada mudança vinha com peças novas — e aí o caminho que escreve redes
corria. Qualquer «sincronizado» anterior a esta data significa *as peças
batem*, e não *a placa está igual ao esquema*.

Consertado em `sincroniza_pcb` **v0.5.0** (`refresca_redes`), que põe em cada
pad a rede da netlist **e apaga a que a netlist já não tem**.


## Rotina de operação: os dois destinos, sempre juntos

**Toda nova versão de `STATUS.md` ou `battle_scars.md` vai para os DOIS
destinos na mesma ação:**

| Destino | Caminho | Forma |
|---|---|---|
| Repo | `docs/` | cru, sem frontmatter |
| Vault | `~/Documents/Obsidian Vault/Cotucatronics/` | com frontmatter YAML + tags |

Os dois servem leitores diferentes e nenhum substitui o outro. O **repo**
é o que sobrevive ao clone limpo — documento citado que não existe no
clone é a mesma classe do cabeçalho de alocação que faltou por semanas.
O **vault** é onde se lê, linka e pensa entre sessões, com tags e
backlinks que o Markdown cru não tem.

Nunca deixar um destino pra depois: o par desalinha em silêncio, e o
desalinho só aparece quando alguém precisa da versão certa.

---


## 02/09, fecho — o chiado, e a medição que derrubou dois planos meus

**O ruído branco perene**, que o dB descreveu como *"um chiado como um rádio
fora da estação"*. Quatro factos, todos medidos por ele:

| teste | resultado |
|---|---|
| mexer o pote (volume digital) | **não muda** → entra depois do conversor |
| volume de **entrada do amp a zero** | **não muda** → não entra pela entrada |
| tirar o cabo, ou desligar a Daisy | **pára** → é a Daisy, pelo cabo |
| tirar a USB do Mac | **melhora, não resolve** |

E do lado do código, provado por leitura: com `out_l = out_r = 0` o único
código depois do mute é o LED e a contabilidade de CPU. **O conversor recebe
zeros exactos** — o firmware está ilibado por construção.

**Diagnóstico, pela varredura de terra (P3):** `AGND` e `DGND` **combinados
fora da placa**, no trilho de permaproto. A Daisy já os une internamente num
ponto; uni-los outra vez faz um **anel**, a corrente divide-se, e a fracção
que escolhe o caminho de dentro atravessa a **referência do codec**. Explica
os quatro factos de uma vez, e é branco porque é a soma de tudo o que corre
no trilho.

**Por confirmar com o multímetro**, em AC/mV e sob carga. O plano de ataque
e o critério de aceitação estão escritos no relatório da varredura.

### O compressor, e a medição que o pediu

Construído o **`pkout`** — faltava o outro lado da conta do `pk`. A primeira
leitura deu **74,7 dB de margem** e eu ia construir um andar de ganho em
cima dela. Era o **ambiente**. Com o dB a bater:

```
ENTRADA  IN_L  0,0 dBFS · 24 CLIPS   (piezo directo, sem pré-amp)
SAÍDA    OUT_L pico −0,4 dBFS · média −49,2 dBFS   →  MARGEM 0,4 dB
```

**49 dB de factor de crista.** O amp precisava de tanto ganho porque estava
a ser pedido para tornar audível a **média**. Logo a resposta não era ganho
— era **compressão**. Novo bloco `Compressor` em `dynamics.h`, params
partilhados, **nasce desligado**.

**Requisito de hardware, medido:** o piezo precisa de ~6 dB de atenuação
antes do conversor, ou os transitórios continuam a ser destruídos.

---

## 02/09, noite — SuMiA™, e o preço de iterar num ser humano

**O que existe agora.** O `notch` chama-se **`microfonia`** e mudou de eixo:
era por cadeia (piezo), é por **ramo** (exciter). O laço fecha-se pelo par
exciter+corpo, e o dB tem tambor no L e botijão no R — um ajuste por cadeia
obrigava a mesma frequência a servir os dois. **Quatro bocas por ramo**,
CC 1–18 no ch1.

**O SuMiA™** (nome dele; o trocadilho é exacto) corre no laço, 250 ms
alternando ramos. Detecta pela **dominância**, age à primeira vista,
**aprofunda** de 18 até 24 dB quando a boca não chega, e **despeja a mais
grave** quando estão cheias — regra dele: *"as mais agudas porque são as
mais dolorosas"*. Nunca toca em boca posta à mão. Não persiste, não entra
em preset, e **`micro_auto` está fora do preset por segurança**: uma cena
não pode desligar a protecção.

**A rampa de arranque é supervisionada.** Sobe em 8 s, curva quadrática, e
**recua 25%** sempre que o SuMiA™ apanha um modo — a boca ganha tempo de
morder e o que já soava baixa. Um corpo que uiva muito demora mais a abrir;
um corpo limpo abre nos 8 s. **A peça negoceia com o próprio corpo.**

### O achado da escuta: o laço atravessava DOIS corpos

Com as quatro células da matriz abertas: **1536,4 Hz a +24,4 dB,
dominância 0,99**. Com `A→L` e `B→R` apenas: **1543,1 Hz a −24,3 dB,
dominância 0,27**. **Quarenta e nove decibéis.**

E mesmo com a matriz separada, os dois ramos reportaram **3275,7 e 3275,8
Hz** — o tambor está **soldado** ao botijão. A matriz separa os caminhos
eléctricos; **não separa o metal**. Por isso qualquer pote tem de rearmar
os dois caçadores.

### O banco de ensaio, e porque devia ter vindo primeiro

O SuMiA™ foi afinado **com a escultura ligada ao lado de uma criança**, e
três vezes a microfonia chegou aos ouvidos dela — uma delas fez o filho do
dB gritar. Ver **15bi**, **15bj**, **15bk**.

Existe agora um **tom de ensaio** (`sim_laco`, `sim_hz`, `sim_fb`) que
injecta um pico de amplitude conhecida antes do notch, com as saídas mudas.
Provou os cinco comportamentos em **dez minutos**: erro de detecção
< 0,3 Hz, preenchimento por ordem, aprofundamento 18→21→24, despejo da mais
grave, e silêncio quando resolvido.

**Declarado no código:** o tom **não** testa se o notch quebra um laço
real. Isso só o corpo responde.

**Aberto, e é o próximo:** o **ruído branco perene** que toca o tempo todo.

---

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
1. Separar a **ilha de `AGND`** — piezos, DPDT, jacks, entrada do amp.
   Conferir com as **três medições por ordem** do
   [`ATERRAMENTO.md`](operacao/ATERRAMENTO.md) v0.2.0 — "com a Daisy fora do
   soquete" era o método antigo, e com o net-tie do revB dá o veredito ao
   contrário.
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

**Pedaleira de cinco efeitos residentes, em ordem fixa.** Bloco **48**.

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

**T16c passou.** 2 779 CCs em 90 s com a carga a **65,1%** do prazo do
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
do Fx2. Slot Fx2 vazio ⇒ `fx2_want` falso ⇒ bloco pulado ⇒ **sem
filtro**. Ele existia só pra conter o estouro de grave que o módulo do Fx2
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

## Fechado em 20/08 — o motor de DSP externo saiu, inteiro

O delay era o último consumidor do runtime. Reescrito em C++ (`delay.h`, grafo
lido do export: feedback fixo **0,4**, crossfade, mono nos dois canais), o
runtime saiu por completo.

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

Saíram junto: `MODULE_SRC`, o diretório de export, `module.h`, `mod_slot[]`,
`ModBind` e o
push-por-nome, seis arquivos, quatro exports, os seis params órfãos do módulo,
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
da saída do módulo de caráter do build:

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
ao destruído a cada batida. O registro do antigo sorteio de nota mostra o
padrão — faixa estreita, centro escolhido, e no caso dele até quantização
em escala.

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

**🔴 A serial morre sob callback longo — prioridade 1.**
As rajadas de ~3,4 ms do callback starvam a ISR do USB; o CDC do host
apodrece. Contornado por `log_mute`, caixa-preta e MIDI, mas o conserto
certo está desenhado e não feito: **ring de log kick-and-forget**, em que
o loop nunca toca o USB e a drenagem acontece por ISR.

**🟡 Bloco 256 custa 5,3 ms de latência.** Num instrumento realimentado
isso desloca onde o feedback se estabelece — é o preço de qualquer módulo
de FFT que o exija, e a razão de o bloco ser 48. Restrição conhecida, não
resolvida.

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
3. **Defaults de palco:** `buzios_thresh`, `buzios_cooldown`.
4. **Painel v2** quando o hardware chegar (T11 já testa por serial).
