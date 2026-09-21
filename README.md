# Cotucatronics OS
<!-- doc-check: ignore no_connect (codigo de violacao do ERC do KiCad) -->
<!-- doc-check: ignore dentro_do_contorno (ferramenta em kicad/, fora do alcance do check_docs) -->
<!-- doc-check: ignore para_fabrica (ferramenta em kicad/, fora do alcance do check_docs) -->
<!-- doc-check: ignore pcb_contorno (ferramenta em kicad/, fora do alcance do check_docs) -->

<!-- doc-check: ignore hook_stop (script em docs/operacao/, nao param) -->
<!-- doc-check: ignore svg_colisoes (ferramenta fora de src/. O check_docs varre `src/`; estas vivem em `tools/`, `docs/desenhos/caixa/` ou `kicad/cotuca_cerebro/`, que ele nao le. Nao e fantasma: e limite de alcance do verificador.) -->

Firmware modular para **instrumentos-escultura sonora**. A ideia central é que
o mesmo cérebro rode qualquer escultura: o código de instrumento é agnóstico de
placa, e trocar de hardware significa escrever um arquivo novo — não reescrever
o sistema.

Projeto colaborativo.

> ### Estado, 2026-09-04 — **v0.301.0**
>
> **480 MHz · 48 kHz · bloco 48** (prazo de 1 000 µs) · app na QSPI
>
> **Duas cadeias de caráter** (drive, phaser, warble, tarolator por piezo) sobre
> uma **cauda compartilhada** (delay, freeze, shimmer↺reverb), com **matriz de
> roteamento 2×2** — qual cadeia entra em qual saída.
>
> **Stack anti-realimentação em quatro camadas**, todas em C++ do shell: gate,
> Governor, limiter e shifter SSB — mais o **SuMiA™**, supressor de microfonias
> automatizado, quatro bocas por ramo, que caça e preenche sozinho.
>
> **MIDI em seis canais** por ponte BLE↔UART num ESP32-C3 (`ch1` compartilhados
> · `ch2` cadeia A · `ch3` cadeia B · escuta = manda + 3), **81 CCs**, presets
> em RAM, **caixa-preta** em BACKUP_SRAM.
>
> **Painel soldado e vivo:** OLED SSD1309 por SPI com abertura animada, **três
> encoders**, **dois pots** (volume por ramo) e **quatro botões** com dois LEDs.
> O mapa está em [`docs/operacao/PINOS.md`](docs/operacao/PINOS.md), que é a
> fonte — e o fonte é que manda sobre ele.

> ### O que ainda NÃO passou pelo ouvido ou pelo multímetro
>
> - **O compressor nunca foi ouvido.** Existe desde 02/09 (`dynamics.h`) e
>   nunca foi ligado com som.
> - **Quatro itens da [pauta de escuta](docs/PAUTA-ESCUTA.md)** — o topo do
>   `drv_sujeira` (só aprovado em 0), o shimmer com transposição a sério, o
>   delay reescrito, e ~20 versões sem veredito de ouvido.
> - **O chiado de banda larga** está diagnosticado até uma regra com nome —
>   `AGND`/`DGND` unidos fora da placa, P3 da varredura de terra — e o
>   aterramento foi refeito a 03/09 com som mais limpo, **mas a medição com
>   multímetro nunca foi feita**.
> - **Os comuns dos DPDT** esperam duas continuidades: o amp é BTL? e o
>   terminal que vai à ilha é saída de potência ou só o sleeve do TRS?

---

## Arquitetura

Cinco regras que não se negociam:

1. **Fronteira HAL.** Nenhum `#include` de fabricante fora de
   `hal_<board>.cpp`. `main.cpp`, params, macros e manifests não sabem em que
   placa estão rodando. A placa se escolhe com `make BOARD=daisy_seed`.
2. **Parâmetros normalizados.** Tudo é `0.0–1.0` numa tabela central; o manifest
   é quem traduz pra Hz, segundos, dB.
3. **Macros.** Um knob físico pode mover vários parâmetros, com peso e inversão.
4. **Manifest por instrumento.** É o único arquivo que muda de escultura pra
   escultura. Declara `Requirements` — RAM
   grande, audio in, nº de knobs — conferidos no boot: placa insuficiente dá
   erro claro em vez de bug misterioso no palco.
5. **`AllocBig()`.** Abstrai a SDRAM de 64 MB. Alocador bump, só no boot, nunca
   no callback de áudio. Placa sem RAM grande devolve `nullptr` — quem chama
   **precisa** checar.

## Mapa de arquivos

| Arquivo | Versão | O que é |
|---|---|---|
| [src/hal.h](src/hal.h) | **v0.38.0** | O contrato. A fronteira de independência de fornecedor |
| [src/hal_daisy_seed.cpp](src/hal_daisy_seed.cpp) | **v0.75.0** | Único arquivo que pode incluir `daisy_seed.h`. **4 botões** (D26, D24, D25 rearme CTOS, D22 freeze B) e **2 LEDs de painel** (D27, D23). Debounce de 30 ms, medido. **OLED SSD1309 vivo** (SPI1: D5 RST · D6 DC · D7 CS · D8 CLK · D10 DIN) com sonda de fio e caça-ponte. **PULL-UP interno no RX do MIDI (PB7)** — a libDaisy inicia a UART com `GPIO_NOPULL`, e o pino a flutuar com a ponte desligada punha a USART em tempestade por taxa e matava o laço em 4 s. **USART1 em prioridade 1**, abaixo do seu próprio stream de DMA, senão o abort assíncrono nunca completa. **Rearme do `DmaListen` com tecto e recuo** — sem ele corria ao ritmo do laço e consumia-o (10 029 rearmes com a ponte sã). **Framebuffer inteiro por `OledDesenha`** (`memcpy`, não 8192 `DrawPixel`), e o prescaler do SPI nasce em **PS_8** — 3 811 µs por quadro, verificado no vidro com padrão de 1 px. PS_64 custava 23 401 µs. **UM encoder** desde 05/09 (eram três; o dB fechou a interface num só, e isso dissolveu de graça o conflito do `D17`, que era o push do encoder G *e* o A2 do divisor do trilho) — e o `kEncPin` é a ÚNICA fonte — o `Init` lê dela e os nomes saem por `EncoderPinNome`. **`VarreTudo`** lê os 31 pinos (mata OLED e MIDI até um reboot) e **`Reiniciar`** desfaz. O `PollMidi`/`PollSerial` drenam por **instantâneo** do ponteiro — perseguir o ponteiro vivo travou a placa em 28/08 |
| [src/reverb.h](src/reverb.h) | **v0.2.0** | Freeverb + passa-alta. C++ puro, buffers vêm de fora |
| [src/freeze.h](src/freeze.h) | **v0.3.1** | Freeze granular: núcleo de 500 ms, 2 grãos sorteados, Hann. C++ puro |
| [src/pitch.h](src/pitch.h) | **v0.5.0** | Motor de pitch compartilhado: 1 buffer, até 4 vozes. **Anti-alias automático na escrita** (Butterworth 6ª ordem, custo zero se nenhuma voz sobe). C++ puro |
| [src/shimmer.h](src/shimmer.h) | **v0.3.0** | Shimmer: reverb + pitch num laço. Realimenta o Freeverb existente. **Tom = corte + sino de presença em 3 kHz** |
| [src/drive.h](src/drive.h) | **v0.4.1** | Drive: tanh normalizado, **1,5–48×** (o joelho tem que alcançar −30 dBFS), + `drv_sujeira`: morph pra wavefolder com anti-alias próprio |
| [src/phaser.h](src/phaser.h) | **v0.3.0** | Phaser: N allpass varridos por LFO triangular. Zero buffer |
| [src/warble.h](src/warble.h) | **v0.1.1** | Warble: deriva de fita por delay curto modulado. O dry/wet dele chama-se `wrb_mix` e **é nome, não param** |
| [src/detune.h](src/detune.h) | **v0.1.0** | Detune. **FORA do build desde 20/08** — revivível, ver `docs/FEATURES.md` |
| [src/delay.h](src/delay.h) | **v0.2.0** | Delay em C++: linha de 1 s, feedback fixo 0,4, piso 50 ms. **Clamp da leitura contra NaN/Inf, com contador `clamps_`** |
| [src/effect_slot.h](src/effect_slot.h) | **v0.3.0** | Moldura: fade assimétrico, custo-zero desligado, cronometragem |
| [src/pitch_spectrum.py](src/pitch_spectrum.py) | **v0.2.0** | Os N maiores picos do espectro — pico dominante mente com laterais fortes |
| [src/dynamics.h](src/dynamics.h) | **v0.6.0** | Gate, governor, limiter e **shifter SSB**. C++ puro |
| [src/midi.h](src/midi.h) | **v0.8.1** | Máquina de estados MIDI + **os seis canais** e a invariante `escuta = manda + 3`, travada em `static_assert`. C++ puro, board-agnostic |
| [src/test_dynamics.cpp](src/test_dynamics.cpp) | **v0.1.0** | Banco de teste nativo — roda no Mac, sem placa |
| [src/test_midi.cpp](src/test_midi.cpp) | **v0.6.0** | Banco nativo do parser MIDI e da via CC→ParamTable |
| [src/test.command](src/test.command) | **v0.4.0** | Wrapper duplo-clicável dos dois bancos de teste |
| [src/main.cpp](src/main.cpp) | **v0.302.0** | Loop, callback de áudio, painel falso via serial. Duas cadeias + cauda compartilhada. **Espelho MIDI em seis canais** (`escuta = manda + 3`) com janela de autoria de 250 ms, inventário `vistos`, botões físicos de freeze, sondas `qspi`, **matriz de roteamento 2×2** (qual cadeia entra em qual saída, com as rampas num buffer para o seco, o freeze e o espaço lerem o mesmo valor). **A versão mora só em `COTUCA_OS_VERSION`** |
| [src/suave.h](src/suave.h) | **v0.1.0** | Suavizador de parâmetro de um polo. C++ puro — a quarta ocorrência da forma virando única |
| [src/teste_tarol.cpp](src/teste_tarol.cpp) | **v0.2.0** | Banco nativo do tarolator. **Declara o que NÃO prova**: passou com senóides enquanto o efeito soava a caixa — banco prova a álgebra, não o timbre (cicatriz 15u) |
| [src/teste_macro.cpp](src/teste_macro.cpp) | **v0.1.1** | Ida-e-volta do `Macro::Inverso`, de que a captura do pot depende. Se ele errar, o pot "não pega" e a bancada culpa o ADC |
| [src/notch.h](src/notch.h) | **v0.2.0** | **Notch anti-microfonia**, duas bocas por cadeia, Q 35 fixo, profundidade em dB. EQ de pico com ganho negativo (e não notch puro, que não tem profundidade ajustável). Nasceu de uma peça de corpo **soldado** — exciter e piezo na mesma chapa, sem junta para perder energia, e por isso nenhum gesto mecânico funcionou. Entra **antes do gate**: se a microfonia passasse pelo portão, seria ela a segurá-lo aberto |
| [src/splash.h](src/splash.h) | **v0.11.1** | **A abertura**: jogada de búzios que cai de cima em leque, fica **assente**, e então uma das conchas sobe e cresce até virar marca — nome letra a letra, versão a assinar, marca segura 3 s. Semeada do **ruído do próprio ADC**, e por isso cada arranque é uma jogada diferente. Três estados: abertura completa no arranque, painel de presets **imediato** no uso normal, e **só a jogada** quando os búzios disparam de verdade. 20 Hz, C++ **puro**, passo `noinline`. **A marca são três peças, não uma**: casco vazado (anel *cheio menos cheio* — contorno duplo não fecha nos topos), massa do lábio com envelope `sin(π·t^1,5)`, e a boca aberta só por dentro do lábio, com dentes **emparelhados** cima/baixo que mordem meio vão. Espelho cima↔baixo **0 px de 1215, medido**; a assimetria esquerda↔direita é de propósito e foi escolhida pelo dB numa escala de rabos medidos. Caixa da marca: **45 × 27 px** em `cy=17` |
| [src/desenho.h](src/desenho.h) | **v0.3.1** | Primitivas sobre o framebuffer de 1024 bytes: `Pixel/Le/Linha/Retangulo/Caixa/Elipse/SuperElipse/Texto/Largura/TextoCentrado`. Bresenham **inteiro**, sem float — o desenho na placa é bit a bit o do teste em terra. O **recorte mora só no `Pixel`**, e não um `if` por primitiva, que é como se esquece um. Na `SuperElipse`, **uma linha de pixels é uma faixa e não uma aresta** (largura medida em `|y|−0,5`) e **a tampa é um traço** — foi o espigão do ápice que, ao ser removido, abriu o contorno por cima |
| [src/fonte5x7.py](src/fonte5x7.py) → [src/fonte5x7.h](src/fonte5x7.h) | **v0.1.0** | 43 glifos em **arte legível**, não hex cego; `--ver` re-desenha a partir do resultado gerado, que é a volta completa. Caractere sem glifo desenha **caixa**, nunca espaço: falta silenciosa de glifo é indistinguível de texto certo |
| [src/espectro.h](src/espectro.h) | **v0.2.0** | FFT radix-2 escrita à mão, Hann, pico com **interpolação parabólica em dB**, e **dominância** — a fracção da energia no pico, que separa espeto de nota. Substituiu uma autocorrelação que falhou **6 de 6** na prova (pico em múltiplos, e mede periodicidade em vez de força do parcial) |
| [src/tarol.h](src/tarol.h) | **v0.1.0** | **TAROLATOR** — ruído percussivo com o envelope da pancada, por divisão de frequência. Nasceu oitavador e não faz oitava: numa membrana os modos são inarmónicos e o divisor treme. Nome do dB, e o nome é o diagnóstico. Tom e rastreio FIXOS: como params davam curso morto |
| [src/params.h](src/params.h) | **v0.12.0** | `ParamTable` + `Macro` (teto **128** desde 28/08, subido com 91 ocupados — folga na mão, não com o estouro). Sem alocação dinâmica |
| [src/manifest.h](src/manifest.h) | **v1.13.0** | Duas cadeias, params de caráter compartilhados, chave e dry/wet por canal, **95 vínculos em 55 CCs**, distribuídos por seis canais + CC 120 = sync. **Tabela de botões físicos** (`BotaoAlvo`: param + LED + nome) |
| [docs/operacao/gerar_ccmap.py](docs/operacao/gerar_ccmap.py) | **v0.11.0** | **Dono do `CCMAP.md` E do cartão A4** — escreve os dois do `manifest.h`. A única parte à mão é a tabela `NOTAS`, e o gerador denuncia nota órfã e nota que mente sobre topologia ("compart." num param pareado). Achou dez notas erradas na estreia |
| [docs/ccmap.pdf](docs/ccmap.pdf) | **gerado** | **O cartão de referência** — 91 vínculos em 52 CCs numa folha A4, com os pares colapsados. `ccmap.html` é **gerado** pelo `gerar_ccmap.py`; o PDF sai do `ccmap-pdf.sh` / `.command` v0.1.0, que avisa se passar de uma folha |
| [docs/operacao/RETRABALHO-LOOPY.md](docs/operacao/RETRABALHO-LOOPY.md) | **gerado** | O mapa inteiro para remapear o Loopy: 86 vínculos em 49 CCs, por página |
| [esp32/ponte/ponte.ino](esp32/ponte/ponte.ino) | **v0.4.0** | **Segundo firmware.** Ponte BLE MIDI ↔ UART no ESP32-C3 — um fio com rádio: sem mapa de CC, sem params. D28 ativo em baixo, aberto-dreno. `board` traz quedas com recência e saturações de TX, e desde a v0.5.0 **quedas que sobrevivem ao corte de energia** (NVS, escrita diferida): uma conta desta sessão, outra desde sempre com o número de arranques |
| [esp32/README.md](esp32/README.md) | — | Fiação, pinos escolhidos, como gravar, e o ambiente que se sabe funcionar (⚠️ NimBLE **1.4.3**, o 2.x não compila) |
| [docs/operacao/ATERRAMENTO.md](docs/operacao/ATERRAMENTO.md) | **v0.2.0** | **O capítulo do terra** — os três tipos de condutor e o destino legal de cada um, estrela contra margarida, `AGND`/`DGND` e a ponte estreita, condensadores **por tipo de carga** (o que o amp exige, o step-down recusa e o rádio precisa), entradas de alta impedância, acoplamento pelo ar, e o método de medição **sob carga**. É o dono do assunto: quando discordar do `HARDWARE.md`, manda |
| [docs/operacao/GEAR-AUDIT.md](docs/operacao/GEAR-AUDIT.md) | **v0.1.0** | A decisão de trocar o WIDI Core por um ESP32-C3: US$ 45 → US$ 4 numa peça replicada em série. **Sem plano B — o aceite é portão**, e o que ele trava é a encomenda |
| [docs/operacao/VOO-ESP32-PONTE.md](docs/operacao/VOO-ESP32-PONTE.md) | **v0.1.0** | O voo da ponte BLE↔UART. **O firmware da Daisy não muda uma linha** — o `sync` automático é o voo 2. Traz as três verificações feitas antes de escrever |
| [docs/operacao/gerar_projeto_loopy.py](docs/operacao/gerar_projeto_loopy.py) | **v0.2.0** | **Escreve um `.lpproj` COMPLETO** do `manifest.h` — 9 páginas, 143 widgets, cores por canal. Parte de um molde e preserva a cena de loops e todo o bundle: um projecto sem `Clock` nem `Sequence` nem abre. **Nunca escreve por cima do molde** |
| [docs/operacao/gerar_perfil_loopy.py](docs/operacao/gerar_perfil_loopy.py) | **v0.5.0** | Lê o projeto exportado do Loopy (`.lpproj`, zip com SQLite) e **emite os bindings** já endereçados, com os CCs vindos do `manifest.h`. ⚠️ O endereço é posicional — regerar depois de mexer no layout |
| [docs/operacao/gerar_secao_loopy.py](docs/operacao/gerar_secao_loopy.py) | **v0.3.0** | **Dono do `LOOPY-SECAO.md`** — folha de montagem da secção nova, com CC, canal, tipo de widget, faixa e valor de nascimento, mais a página de presets. Gerado porque numa só sessão mudaram **seis CCs**: folha à mão deriva na primeira mudança, e quem a segue monta o instrumento de ontem |
| [docs/operacao/LOOPY-SECAO.md](docs/operacao/LOOPY-SECAO.md) | **gerado** | 105 vínculos, 72 faders e 33 botões, na ordem de aprendizagem — não é layout: o Loopy endereça por índice de array, que só existe depois de os widgets serem criados |
| [docs/operacao/CCS-POR-PAGINA.md](docs/operacao/CCS-POR-PAGINA.md) | **gerado** | Os CCs na ordem das páginas do Loopy: central / mod a / mod b / time a / time b / random / tech |
| [docs/operacao/BINDINGS-A-MAO.md](docs/operacao/BINDINGS-A-MAO.md) | **gerado** | Plano B: folha de entrada manual em Control Settings, com CC e canal por linha |
| [docs/operacao/ACEITE-MULTIPLEXACAO.md](docs/operacao/ACEITE-MULTIPLEXACAO.md) | **vivo** | Roteiro do aceite de MIDI — o lado do firmware está verde |
| [src/presets.h](src/presets.h) | **v0.11.0** | Estruturas de preset e a **versão do mapa derivada** (FNV-1a dos nomes; hoje **`0xBE1B`**). C++ puro — prova-se inteiro em banco nativo. A pertinência ao preset é DERIVADA do nome, e `route_*` está na lista curta de excepções. Teto por preset **56** |
| [src/test_presets.cpp](src/test_presets.cpp) | **v0.7.0** | Banco nativo dos presets: pertinência, hash (as duas metades) e cada veredito de recusa |
| [src/qspi_raw.h](src/qspi_raw.h) | **v0.2.0** | Contrato do acesso cru à QUADSPI. **Fecho transitivo zero** — nada que ele chame pode morar na QSPI |
| [src/qspi_raw.cpp](src/qspi_raw.cpp) | **v0.2.0** | Registradores da QUADSPI na mão, tudo marcado `COTUCA_ITCM`. **ABORT ao sair do memory-mapped** — ver `docs/DESENHO-ESCRITA-QSPI.md` |
| [src/cotucatronics_qspi.lds](src/cotucatronics_qspi.lds) | **v0.1.0** | Linker script bifurcado da libDaisy (certidão de origem no topo). Acrescenta `.itcm_text` → ITCMRAM |
| [src/teste_minimo/](src/teste_minimo/) | **v0.4.0** | **Banco de provas na placa.** Trinta linhas que dividem o problema ao meio numa gravação: se fala, a cadeia está boa e o defeito é do app. Já eliminou placa, gravação, USB, init do áudio e 64 MB de SDRAM — e imprime o que o `CheckBoardVersion()` detectou |
| [src/grava_placa_nova.command](src/grava_placa_nova.command) | **v0.2.0** | Grava uma Daisy **nova**, esteja de fábrica ou já com bootloader — o `flash.sh` só serve depois de haver app a correr. Espera o DFU e **decide o alvo pelo mapa de memória**, não pelo PID: `0x90000000` é a QSPI (bootloader presente → `program-dfu`), `0x08000000` é a ROM da ST (→ `program-boot`). ⚠️ Os dois DFU enumeram como `0483:df11` — `STM_PID` e `DAISY_PID` são o **mesmo valor**, e discriminar pelo PID manda gravar o bootloader por cima de uma placa que só queria o app. **O ramo `program-boot` nunca foi exercitado** |
| [src/flash.sh](src/flash.sh) | **v0.4.0** | Grava **só se o build passou**, confere a versão **no fonte e na placa**, e **repõe o estado que a gravação apagou**. Entra no bootloader por `dfu go` e espera o DFU aparecer, em vez de pedir o RESET |
| [src/estado.py](src/estado.py) | **v0.2.0** | Guarda os params antes do DFU e repõe-os depois. ⚠️ Fala em **`real`**, que é o que o `set` aceita — a v0.1.0 mandava `norm`, era recusada com `fora da faixa`, e **imprimia "repos" na mesma** porque eu deitava fora a resposta. Lê a resposta, e grita o que recusa |
| [src/porta_daisy.py](src/porta_daisy.py) | **v0.1.0** | Acha a Daisy **perguntando**, nunca por posição no `glob`. Nasceu quando o ESP32 entrou na USB e passou a haver dois `usbmodem` — a escolha por índice virou sorte |
| [docs/operacao/auditar.sh](docs/operacao/auditar.sh) · [.command](docs/operacao/auditar.command) | **v1.3.0** | **A varredura mecânica do `/audit`** — links mortos, isenções órfãs, aposentados órfãos e duplicados, destinos dessincronizados (repo/vault), `check_docs` e o mapa de versões. **Só reporta o que prova**: as classes de julgamento estão na skill, não aqui. Na estreia achou 5 links mortos no `MAPA-DE-TERRA.md` e uma entrada duplicada |
| [docs/operacao/hook_stop.sh](docs/operacao/hook_stop.sh) · [.claude/settings.json](.claude/settings.json) | **v1.0.0** | **O `/audit` corre sozinho no fim de cada sessão, e AVISA.** O `battle_scars` já tinha a regra — *"hábito automatizado vence hábito lembrado"* — e todos os verificadores dependiam de alguém se lembrar. **Nunca bloqueia:** sai sempre 0, porque portão que estorva é portão que se desliga. ⚠️ A contagem é **por secção** — a passada de itens pendentes lista 🔴/🟡 que vivem dentro dos documentos, e contá-los daria alarme permanente |
| [docs/operacao/hook_commit.sh](docs/operacao/hook_commit.sh) | **v1.0.0** | **Os verificadores BARATOS correm antes do commit** — `PreToolUse` em `git commit`, **70 ms**. O `hook_stop` já apanhava estas coisas, mas no fecho da sessão, com o mau já empurrado: **o ganho não é evitar o erro, é ele chegar enquanto ainda há contexto para o consertar.** Corre só o mapa de versões (0,03 s) e a frescura dos desenhos (0,02 s); o `check_docs` (1,5 s) e o `auditar` (3,3 s) ficam no fecho — verificação cara em cada commit é verificação que se acaba por remover. Avisa, nunca bloqueia. Na estreia apanhou um desenho velho que ninguém sabia estar velho |
| [docs/operacao/derivados.py](docs/operacao/derivados.py) | **v1.1.0** | **Faz de "gerado" uma afirmação reprovável.** O `manifest.h` já era a fonte única e o mapa de CC dava números diferentes em documentos diferentes — a causa era o rótulo: três folhas diziam-se geradas dele, ninguém as mantinha à mão *por isso*, e **nenhum gerador as escrevia**. Confere quem se diz derivado, de que versão da fonte, se bate com a de hoje, e **se existe gerador que o escreva**. O carimbo que serve nomeia a FONTE (`manifest v1.13.0`), não o gerador |
| [docs/operacao/vizinhos.py](docs/operacao/vizinhos.py) | **v1.0.0** | Caça a **frase partida pela renomeação** — o caso em que a âncora casou, o `grep` confirmou, o `make` passou, e a frase ficou sem sentido porque a continuação estava na linha seguinte. Só assinala quando a edição tocou o **fim** da linha; `--termo=<nome>` aperta para onde a cauda removida trazia o nome (211 → 89 → 16 vizinhanças) |
| [docs/operacao/coerencia.py](docs/operacao/coerencia.py) | **v1.1.0** | As checagens que o `grep` não faz direito — isenções que não cobrem nada (pelo critério do `check_docs`: **entre crases**, escopo de ficheiro), saúde da lista de aposentados, e a **passada de itens pendentes**. Nasceu a corrigir duas checagens em bash que acusavam pelo critério errado |
| [docs/operacao/versoes_readme.py](docs/operacao/versoes_readme.py) | **v1.1.1** | Confere o mapa de ficheiros acima contra os ficheiros — o `CLAUDE.md` exige a versão actual em cada linha e nada cobrava isso. `--write` acerta. **Na estreia: 15 de 58 tinham derivado**, uma delas com o README à FRENTE do fonte (`ponte.ino`). Reporta o que não consegue ler, em vez de saltar calado |
| [src/check_docs.py](src/check_docs.py) | **v0.14.0** | `make check-docs`: dá testemunha à doc contra o manifest: fantasmas, comandos sem ajuda, mapa de pinos, faixas, e **duas versões no mesmo arquivo que discordam**. Aborta se `AddMidi` não casar nenhuma vez. **Imprime a própria versão na primeira linha**. A regra das duas versões cobre também `.py`, onde não há `#define`: cruza docstring, constante e **o que o script imprime** |
| [docs/params-aposentados.txt](docs/params-aposentados.txt) | **vivo** | Params aposentados, símbolos removidos e nomes externos — o que o verificador tem licença para não achar |
| [docs/DESENHO-ESCRITA-QSPI.md](docs/DESENHO-ESCRITA-QSPI.md) | **vivo** | A rota dos presets: janela ITCM (**provada**), protocolo real lido do fonte, e **onde a frente parou com a condição de retomada** |
| [docs/FUTURO.md](docs/FUTURO.md) | **vivo** | **Possibilidades, com o custo já apurado** — não é lista de tarefas: é o que se considerou e por que não é agora. Sense dos DPDT, tela colorida, os encoders que saíram, e o **orçamento de dez pinos livres** de que tudo sai. Existe porque a alternativa é redescobrir: três vezes numa semana reabriu-se uma questão que já tinha resposta medida, guardada só numa conversa |
| [docs/PAUTA-ESCUTA.md](docs/PAUTA-ESCUTA.md) | **vivo** | O que ouvir, na ordem — ~20 versões sem veredito de ouvido |
| [src/Makefile](src/Makefile) | **v0.2.0** | Build por placa. `APP_TYPE=BOOT_QSPI`. Sem DaisySP |
| [.claude/skills/](.claude/skills/) | **vivo** | **O braço executável do IFÁ10000** — **quinze** skills: **`aipcb`**, `ifa10000`, `medicao-limpa`, `instrumentacao`, `bug-de-ordem`, `edicao-segura`, `bancada-firmware`, `revisao-terra`, `kicad_layout`, `loopy`, `design-review`, `audit`, `caixa`, `pm`, mais a lápide da `kicad`. ⚠️ **A `kicad` foi APOSENTADA a 09/09/2026** e substituída pela `aipcb` — as 1033 linhas dela estão em [docs/rev_b/kicad-skill-aposentada.md](docs/rev_b/kicad-skill-aposentada.md), e em `.claude/skills/kicad/` fica só a lápide. A diferença de fundo: a `kicad` descrevia geradores que escreviam ficheiros KiCad à mão; a `aipcb` descreve um fluxo em que **ninguém** escreve ficheiros KiCad à mão. ⚠️ **A `kicad_layout` ficou sem quem a chame** — dizia-se "chamada pela skill `kicad`", e a `aipcb` não a menciona. Decisão por tomar: entra na `aipcb`, é aposentada também, ou fica |
| [docs/desenhos/](docs/desenhos/) · [tools/desenhos.py](tools/desenhos.py) | **v0.4.0** | **A pasta dos desenhos técnicos, e a ferramenta que a mantém honesta.** Os esquemáticos saem do `kicad-cli sch export svg` — mesma fonte que a fábrica lê, portanto **não podem divergir**. O `desenhos.py` compara datas: **se a fonte é mais nova que o desenho, o desenho está velho**, e sai com código ≠ 0. Sem opinião no critério. ⚠️ Estes não passam o `svg_colisoes` (o KiCad escreve as letras como *paths*) — trocou-se *"pode divergir mas verifica-se"* por *"não pode divergir mas não se verifica"*, e está declarado. O `preamp-piezo.svg` morreu em 05/09 por dizer `C2 1 nF` e `RV1` vivo quando o esquema já dizia `20n` e `DNP` — **figura desatualizada com ar de oficial é pior que nenhuma** |
| [docs/operacao/PINOS.md](docs/operacao/PINOS.md) | **vivo** | **O mapa de pinos, e a fonte é o fonte.** Conferido a cada `bts`. Em 04/09 apanhou-se a dizer `Livres: D0–D12` enquanto listava o OLED como ocupado três secções abaixo — livres a sério são seis |
| [docs/operacao/SUPRESSOR-MICROFONIA.md](docs/operacao/SUPRESSOR-MICROFONIA.md) | **vivo** | O contrato do **SuMiA™**, medido antes do código, mais o que ele já fez em serviço |
| [docs/operacao/ATERRAMENTO.md](docs/operacao/ATERRAMENTO.md) · [TERRA-REFAZER.md](docs/operacao/TERRA-REFAZER.md) | **vivo** | A varredura de terra e a folha de bancada com que o aterramento foi refeito a 03/09 |
| [docs/operacao/ARQUITETURA.md](docs/operacao/ARQUITETURA.md) | **vivo** | O desenho do sinal, ponta a ponta |
| [docs/operacao/PRESETS.md](docs/operacao/PRESETS.md) · [DESENHO-PRESETS-RAM.md](docs/operacao/DESENHO-PRESETS-RAM.md) · [PRESETS-LISTA.md](docs/operacao/PRESETS-LISTA.md) | **vivo** | Presets: o contrato, o desenho em RAM, e a lista |
| [docs/operacao/REMAP-ESCUTA.md](docs/operacao/REMAP-ESCUTA.md) · [ROTINAS.md](docs/operacao/ROTINAS.md) | **vivo** | O remapeamento da escuta e as rotinas de bancada |
| [docs/operacao/gerar_ccmap.py](docs/operacao/gerar_ccmap.py) | **v0.11.0** | **O gerador do mapa de CCs** — escreve o `CCMAP.md` *e* o cartão `ccmap.html`, e **carimba data, commit e árvore suja** (regra do dB, 04/09). ⚠️ Não escrever outro: em 04/09 escrevi um segundo sem procurar este, e o próprio `CCMAP.md` dizia na 3ª linha qual era o certo |
| [docs/operacao/esquematicos/](docs/operacao/esquematicos/) | **vivo** | A placa de suporte em KiCad — `brief` e `bom` da revA, `brief` e `ERC-ISENCOES` da revB. Regras de desenho na skill `kicad` |
| [caixa/cotuca_caixa_corte.py](caixa/cotuca_caixa_corte.py) | **v0.7.0** | **A fonte da caixa.** Gera o corte a partir de `--t` e `--kerf` — trocar de material é regerar, porque a profundidade do dente É a espessura. O nome do ficheiro leva a espessura. Três portões próprios: nesting, **espelho** (peça rodada que na verdade foi espelhada corta bem e monta ao contrário) e margem à borda da chapa. Encaixe em três prateleiras calculado dos bbox reais — a T=3 as peças crescem e a coluna antiga deixava 1,4 mm de margem |
| [caixa/](caixa/) · [tools/nesting.py](tools/nesting.py) | **v0.1.0** | O resto do artefacto físico, e o portão externo: tudo dentro da chapa, nada sobreposto, e **o número de peças** — ausência não colide. ⚠️ A 06/09 morreram as 6 páginas de fabricação, os geradores delas e o corte em nogueira: desenhavam ou eram **outra** caixa, e fonte vencida ao lado da certa mede-se igualmente bem |
| [docs/COTUCA_BOM.md](docs/COTUCA_BOM.md) · [tools/bom_completa.py](tools/bom_completa.py) | **v0.10.2** | **A BOM do instrumento inteiro, GERADA das folhas KiCad** — as duas placas, os módulos (Daisy, ESP32-C3, amp, step-down, decoy), o painel, a cablagem e a caixa. Referências, valores, footprints e DNP saem do esquema; MPN e preço vivem no gerador, e **cada preço declara se foi medido ou estimado**. Carimba data, commit e árvore suja. `--conferir` sai ≠ 0 se estiver desactualizada. ⚠️ **O MPN é um campo próprio (`MPN`/`MPN_PAINEL`/`MPN_POR_FP`), separado da coluna de descrição** — e vazio onde a peça é genuinamente genérica: um resistor 0805 de 1% não tem número de peça, tem valor e encapsulamento. E cada sufixo tem, em `SUFIXO`, a linha da ficha que o justifica **e a que o não justifica** |
| [docs/operacao/esquematicos/revB/BOM-MPN-revB.md](docs/operacao/esquematicos/revB/BOM-MPN-revB.md) | **vivo** | **A BOM por MPN** — peça a peça, com fabricante, encapsulamento e tier. A JLC monta só o SMD; o resto solda o dB. A §8 regista o que a atribuição de footprints a 05/09 desmentiu: o pot `PTV09A-4` é bushingless e não serve para eixo pelo painel, o encoder `PEC11R` não é o Alps `EC11E` (pinos a 5,0 mm contra 2,5), e `R5`/`R6` do painel divergem em valor entre a BOM e o esquema |
| [docs/ROTEIRO-BOOT-SRAM.md](docs/ROTEIRO-BOOT-SRAM.md) · [AUDIT-23-08.md](docs/AUDIT-23-08.md) | **histórico** | Registos fechados: o roteiro do boot em SRAM e a varredura de 23/08. Guardados pelo que explicam, não como pendências |
| [docs/FEATURES.md](docs/FEATURES.md) | **vivo** | **A CAIXA ÚNICA** — toda feature vigente, e as aposentadas no arquivo |
| [docs/CTOS.md](docs/CTOS.md) | **v0.4.0** | A base à prova de falha, a fronteira CTOS/Efeito, a suíte, e o **teto de soma** (§3b) |
| [docs/PROCEDIMENTO-MIDI-LEARN.md](docs/PROCEDIMENTO-MIDI-LEARN.md) | **novo** | Endereçar as páginas do Loopy pelo modo `learn`. Inclui os três "não faça" que custaram rodada |
| [docs/IFA10000.md](docs/IFA10000.md) | **vivo** | **Certificação de efeitos** — as 10 etapas que todo efeito passa |
| [docs/STATUS.md](docs/STATUS.md) | **vivo** | **Onde estamos** — estado da placa, defeitos abertos, próximos passos |
| [docs/operacao/PRESETS-LISTA.md](docs/operacao/PRESETS-LISTA.md) | **v0.9.0** | **A lista de presets e como os botões a endereçam** — CC 121 carrega / 122 grava, canal diz o destino. Vive vazia até à sessão de escuta; traz o hash do mapa em vigor (`0xBE1B`) |
| [docs/battle_scars.md](docs/battle_scars.md) | **vivo** | **Aprendizados e metodologia** — as cicatrizes, pra não pagar duas vezes |
| [src/TESTES.md](src/TESTES.md) | **v0.25.0** | Roteiro T1–T12, fluxo de gravação e defeitos conhecidos |
| [src/BOARDS.md](src/BOARDS.md) | **v0.14.0** | Placas suportadas, a regra da SDRAM e a nota do bootloader |
| [src/cotuca_serial.py](src/cotuca_serial.py) | **v0.4.0** | Monitor serial, sem dependências |
| [src/monitor.command](src/monitor.command) | **v0.1.0** | Wrapper duplo-clicável do monitor |
| [src/cotuca_measure.py](src/cotuca_measure.py) | **v0.1.0** | Mede a frequência de saída por FFT |
| [src/teste_roteamento.py](src/teste_roteamento.py) | **v0.1.0** | **T7c automatizado** — prova os dois andares do roteamento com o tom de teste. Pergunta a porta, acha a interface pelo nome, **mede** o mapa de canais e aborta se as duas saídas caírem no mesmo (critério que não pode falhar não é critério) |
| [src/measure.command](src/measure.command) | **v0.1.0** | Wrapper duplo-clicável do medidor |
| [tools/manual_montagem.py](tools/manual_montagem.py) | **v0.10.0** | **O manual de montagem, estilo IKEA — gerado, não escrito.** Lê as placas fabricadas pelo `pcbnew` (contorno, corpo de cada peça, pad 1, rede de cada pad, lado, DNP), o pacote que foi à fábrica (`cpl_rico_*.csv`) e as lições de campo, e o pedido da Mouser que chegou (`export/mouser_40440548.csv`, saco a saco), e escreve `docs/montagem/manual_montagem.html`: 49 passos, uma tabela que traduz o código de cada etiqueta da Mouser no valor a escrever no saco, um mapa por passo com as peças destacadas, e as que vão **por baixo** em vista espelhada. O pino 1 só se marca quando a forma do pad o distingue (os díodos e o SOIC levam marca explícita). Corre com o Python do KiCad |
| [tools/svg_colisoes.py](tools/svg_colisoes.py) | **v0.6.0** | **O portão de saída de todo SVG** — mede caixas de texto e denuncia sobreposição. Quatro códigos de saída, e o **3 = NÃO VERIFICÁVEL** (zero textos lidos): ferramenta que não consegue medir tem de dizer isso, não devolver "limpo". **Desde v0.3.0 usa o `textLength` que o KiCad escreve** — largura medida pelo próprio KiCad, não estimada. Até aí descartava todo `<text>` com `opacity="0"`, que é como o KiCad marca os seus, e por isso respondia "não verificável" a esquemáticos com 336 textos medidos lá dentro. ⚠️ Em esquemáticos do KiCad é **ruidoso** (cada texto sai duplicado, e todo rótulo toca no seu fio): para esses o portão é o `colisoes_sch.py` |
| [tools/desenhos.py](tools/desenhos.py) | **v0.4.0** | Guarda a frescura dos **11** desenhos técnicos: **fonte mais nova que o desenho = desenho velho**, e sai ≠ 0. ⚠️ Ele afere coerência, não correcção: com o gerador a rebentar a folha não muda, e o desenho passa a "em dia" sem o conserto lá dentro |
| [tools/colisoes_sch.py](tools/colisoes_sch.py) | **v0.5.0** | **O portão dos esquemáticos**, onde o `svg_colisoes` não serve. Lê o `.kicad_sch`, monta a caixa de cada texto e de cada forma (fios e corpos de símbolo) e cruza. O avanço da letra (0,85 do corpo) foi **medido** no `textLength` do próprio KiCad, não arbitrado. **Conta os textos que existem contra os que leu e recusa-se a aprovar leitura incompleta** — a v0.3.0 apanhava 0 das 40 propriedades e anunciava "0 colisões" |
| [tools/contrato_idc.py](tools/contrato_idc.py) | **v0.2.1** | **O único contrato entre as duas placas** — lê as 26 vias do IDC nos dois projectos por geometria e confere uma contra a outra. **Leitura incompleta sai com código 3**: em 05/09 a v0.1.0 leu 24 vias e anunciou "as 26 batem", que é exactamente o defeito que ela existe para apanhar |
| [kicad/cotuca_cerebro/](kicad/cotuca_cerebro/) | **rev B** | O esquema da placa-mãe: raiz hierárquica + 4 folhas (energia, pré-amp, soquete, conector). ERC **11** violações, todas do mesmo tipo e todas declaradas: pinos de reserva do `FUTURO.md`. O `D29`/`D30` deixaram de contar — levam `no_connect`, porque são o USB e não são reserva, são proibição |
| [kicad/cotuca_painel/](kicad/cotuca_painel/) | **rev A** | O esquema do painel: 1 encoder, 2 botões, 2 pots, OLED, LED. **ERC 0 violações.** Usa a *mesma* biblioteca do cérebro por `sym-lib-table` — uma fonte para as duas placas |
| [kicad/cotuca_cerebro/cotuca.kicad_sym](kicad/cotuca_cerebro/cotuca.kicad_sym) | **34 símbolos** | A biblioteca partilhada pelos dois projectos. Pinagem do `OPA1642` tirada do `LM2904` da biblioteca oficial do KiCad — ⚠️ **por confirmar no datasheet do OPA1642AIDR antes de submeter** |
| [kicad/cotuca_cerebro/gera_simbolos.py](kicad/cotuca_cerebro/gera_simbolos.py) | **v0.12.0** | Gera os símbolos da biblioteca. **Todo número passa por `num()`**, que arredonda a 4 casas: sem isso `h/2 − 1.27 − 6×2.54` sai `1.7763568394002505e-15` e nenhum leitor com `[\d.]+` apanha a linha. Traz o **`OPA1642` de três unidades** (05/09) — substituiu `OPA`+`OPAPWR`, que davam três linhas de BOM para um chip e tinham **os pinos 1 e 3 trocados** face ao SOIC-8 real |
| [kicad/cotuca_cerebro/gera_daisy_sym.py](kicad/cotuca_cerebro/gera_daisy_sym.py) | **v0.3.0** | Gera o símbolo da Daisy a partir do CSV do fabricante — desde 05/09 o do **Seed3**, versionado em `kicad/cotuca_cerebro/`. Lê as colunas **pelo nome do cabeçalho**, e por isso rebenta alto quando a fonte muda de forma em vez de emitir um símbolo errado. Recusa nomes que sejam designação de porta do MCU e diz qual descartou — o CSV do Seed3 põe `PG3` no pino 40, que o desenho do próprio fabricante chama `DGND` |
| [kicad/cotuca_cerebro/gera_soquete.py](kicad/cotuca_cerebro/gera_soquete.py) | **v0.19.0** | Gera a folha do soquete. `pinos_de()` lê a geometria da **biblioteca**, nunca de cabeça — e **conta os pinos que leu contra os blocos `(pin` que existem** |
| [kicad/pcb_contorno.py](kicad/pcb_contorno.py) | **v0.24.0** | Contorno e colocação **mecânica** das duas placas. **Importa** o `caixa/cotuca_caixa_corte.py` — sem parsing entre a cota e o uso — e a placa **centra-se na chapa de fundo**, o que a torna **imune à espessura da caixa**: confirmado a correr o gerador a `T=3` e `T=6`, os furos de fixação caem sempre em (15,5 · 128,5) × (3,5 · 96,5) da placa e os eixos em 105/140/175 × 92 da face. A espessura já mudou quatro vezes. Dois portões que recusam escrever: um se a placa não couber na caixa, outro se um dos 4 furos de fixação do fundo ficar debaixo de um componente. **Os planos de cobre saem só em B.Cu e cada pad SMD de terra ganha uma via com coto até ele** (`costura()`): cobre nas duas camadas não deixa camada nenhuma para sinal, e um dois-lados só tem duas — medido, 19 vias no cérebro e 8 no painel, e o `via_dangling` só cai a 1 quando o coto existe, porque cobre adjacente não é cobre ligado. A inversão do `y` vive só no `da_tampa()`, e o eixo de cada pot/encoder é **lido do círculo `F.Fab`** do footprint — a origem de um footprint não é o veio |
| [kicad/sincroniza_pcb.py](kicad/sincroniza_pcb.py) | **v0.5.0** | Faz o que o `Update PCB from Schematic` (F8) faz, e **sem GUI**: tira copias a mais, acrescenta o que falta, remove o que sobra, e poe a rede em cada pad a partir do netlist. Existe porque neste projecto os esquemas sao **gerados** — cada regeneracao muda os UUID, o F8 deixa de reconhecer o que la esta e **acrescenta uma segunda copia**. `--zero` esvazia e repoe tudo. Recusa-se a gravar se a placa e o esquema nao ficarem com as mesmas referencias |
| [kicad/arruma_silk.py](kicad/arruma_silk.py) | **v0.21.0** | Tira os campos de referencia de cima dos pads. **Serigrafia so** — nao toca em cobre, pistas, zonas nem posicoes, logo uma placa routeada passa por aqui sem perder nada. Prova-se pelo numero que NAO muda: `silk_over_copper` 39 -> 4 e ligacoes por rotear 104 -> 104. Se nenhuma posicao servir, **deixa como esta e diz** — empurrar para um sitio mau e trocar de sitio o mesmo problema |
| [kicad/roteia.py](kicad/roteia.py) | **v1.18.0** | Roteia **as duas camadas**, com via, e prova que não estragou nada. Uma camada de sinal é um grafo planar e as ligações não são: a versão de uma camada parou em 24 de 71, e com a segunda foi de **122 para 23**. A via custa 30 células, para só saltar quem não tem volta a dar. **Regra de aceitação: nenhuma rede pode piorar** — mede-se o perfil de ligações por fazer antes e depois, porque o estrago no plano tem duas formas (a zona partida em ilhas, e o pad cortado do cobre) e uma lista de sintomas só protege contra o que já aconteceu. Quando alguma piora, **bissecção** para achar a travessia culpada em vez de deitar fora as boas: isolou 2 em 33. Isenta a linha de base pelo par (tipo, rede), nunca pelo nome da rede. **Arranca o cobre que estorva, passa, e refaz o que arrancou** — a única coisa que ataca as últimas ligações, que já não têm caminho livre — e **volta atrás se o número não descer**, porque arrancar uma rede feita para tentar outra pode ser um prejuízo. A regra de aceitação conta por **(rede, espécie)**: contar só por rede deixava um pad que se ligava mascarar um plano que se partia, e `roteia.py --auto-teste` interroga a regra com esse caso. A retracção **filtra as candidatas por geometria antes de gastar DRCs nelas** — só uma travessia que passe a menos de folga+cobre+folga de um pad do plano pode orfaná-lo — e isso cortou de 27 candidatas para 5, com as mesmas culpadas e a passada a cair de ~13 minutos para 55 segundos. E **uma passada apertada no fim** (folga 0,22, malha 0,085, busca presa a uma caixa de 25 mm): com a folga larga o roteador **não consegue sair de entre os pinos de um conector** — num header de 2,54 com pads de 1,70 o vão é 0,840 mm e ele precisa de 0,950 — o que explicava 9 falhas no `J14`, 8 no `J1` e 11 na Daisy, todas em sítios com zero pistas à volta. Medido: apertar a folga de 0,35 para 0,26 **desde o início** piorou de 23 para 34 — roteador guloso melhora com menos apetite, não com menos folga |
| [kicad/le_drc.py](kicad/le_drc.py) | **v0.2.0** | Lê o relatório do DRC **por rede**, e separa `Zone ↔ Zone` (plano cortado) de `starved_thermal` (pad agarrado a ilha estreita). Existe porque **o total mente por omissão**: 71 → 52 com a ilha de AGND partida em oito é uma melhoria no total e uma regressão na placa. O nome da rede **não está na linha do achado** — vem nas duas linhas de item seguintes, e um extractor que casa o cabeçalho devolve zero com ar de placa limpa |
| [kicad/faz_placa.py](kicad/faz_placa.py) | **v0.17.0** | **Do esquema ao pacote da fábrica, num comando.** Cada passo já existia; o que faltava era a **ordem** — costura depois da colocação, roteamento depois da costura, serigrafia depois do roteamento, portão depois de tudo — e um passo fora de sítio não dá erro, dá uma placa que passa em todos os testes e está errada. Começa por **apagar todo o cobre**, porque quando a colocação pode mudar o cobre que tocava nas peças antigas é lixo. E **corre as três ordens de ataque em cópias e fica com a melhor, medindo**: não há ordem vencedora, há vencedora nesta placa (cérebro `curta` 19 · `drc` 23 · `longa` 29; painel `drc` 10 · `longa` 11 · `curta` 13). Uma decisão a menos para alguém tomar |
| [kicad/roteia_fr.py](kicad/roteia_fr.py) | **v0.13.0** | **O FreeRouting, que é um roteador a sério.** Exporta Specctra DSN pelo `pcbnew`, corre o FreeRouting, importa a sessão de volta. Na mesma placa e de raiz: o meu roteador deu **12 por ligar em ~9 min**, este dá **8 em 35 s** — e as que lhe sobram são todas ligações ao plano, não sinal: **ele fecha todos os sinais**. Trata as quatro coisas que ele não sabe: põe *keepout* sobre os net-ties (senão roteia por cima da ponte de terra e curto-circuita-a), corre com `-mt 1` porque a própria ferramenta avisa que o multi-tarefa gera violações, **apaga o cobre antes** — incluindo a costura, que com ele passa de necessária a estorvo — e no fim liga por via+coto os pads de terra que o enchimento não alcançou, **retirando as vias que ficarem soltas**. ⚠️ Precisa de Java e do `freerouting.jar`; sem eles a cadeia cai no roteador da casa |
| [kicad/classes_de_rede.py](kicad/classes_de_rede.py) | **v0.1.0** | Põe as larguras de trilho nas **classes de rede da placa**, e não numa tabela dentro de um script. Enquanto o roteador era meu a tabela funcionava e escondia que o **DRC não sabia dela**; quando entrou um roteador de fora, ele desenhou o trilho de entrada com largura de sinal. Atribui por **padrão de nome**, para que uma rede nova entre na classe certa sozinha. E baixa a largura mínima para 0,15 mm — a JLC faz 0,127, e era a minha regra que proibia o roteador de sair de um pad de passo fino |
| [kicad/revisao_externa.py](kicad/revisao_externa.py) | **v0.2.0** | **A revisão que não fui eu que escrevi.** Corre o [kicad-happy](https://github.com/aklofas/kicad-happy) (MIT, 1100★, testado contra 5800 projectos) sobre as duas placas. Existe porque tudo o resto desta cadeia é meu — gerador, roteador, portão e os próprios verificadores — e **um auto-teste tem os pontos cegos de quem o escreveu**, o que já aconteceu aqui. À primeira corrida apanhou o que nós nunca verificámos: **zero fiduciais com 35 componentes SMD** (a montagem automática precisa de três por face), **`J15` e `J16` sem um único pino de terra** a levar áudio para fora da placa, e 0/69 redes com ponto de teste. Silencia falsos positivos **por par (código, rede) e com a razão escrita e datada** — nunca uma família inteira, e nunca em silêncio |
| [tools/fichas.py](tools/fichas.py) · [docs/datasheets/](docs/datasheets/) | **v0.10.1** | **As fichas técnicas das peças com MPN real**, descarregadas e *conferidas por dentro*. Existe porque o descarregador do `kicad-happy` falhou nas 19 e a causa não era a lista: a API do `jlcsearch` deixou de servir o campo `datasheet` — ela **encontra** a peça e devolve o campo vazio. A cadeia que funciona são dois passos (`jlcsearch` → código LCSC → `wmsc.lcsc` → PDF), e não foi consertada na cache do plugin porque a próxima actualização a apagava. Cada PDF passa pelo `pdftotext` — ferramenta de fora, que pode dizer que não — e o número tem de aparecer no texto. Cinco degraus: **OK** · **FAMÍLIA** (ficha da série, sufixo com citação obrigatória) · **NO CATÁLOGO** (sem ficha própria, mas o número está num catálogo de série já em disco) · **SUSPEITA** (o ficheiro não fala da peça — trava o portão) · **SEM FICHA**, esta com a morada do fabricante. Estado: **10 OK · 2 família · 1 no catálogo · 8 sem ficha** de 21. Embrulho de duplo-clique em `tools/fichas.command` |
| [kicad/mpn_nas_folhas.py](kicad/mpn_nas_folhas.py) | **v0.3.1** | Põe o **MPN nas folhas do esquema**, carimbado a partir da fonte única. Existe porque os números de peça viviam numa tabela dentro do gerador da BOM, e o esquema não sabia deles: uma revisão de fora leu as folhas e disse **`0/36 parts have MPNs — not pre-fab ready`**, com razão. ⚠️ Duas folhas não têm gerador, e a regra da casa proíbe editar `.kicad_sch` à mão — por isso isto **carimba e volta a correr** depois de qualquer regeneração, em vez de editar. E o portão da fábrica corre `--conferir`: uma cópia sem verificador volta a divergir. Resultado: **36 de 84 (43 %)** — e este número desceu de propósito. A v0.2.0 carimbava a **coluna de descrição** da BOM, e a revisão de fora dizia *«50/50 parts have MPNs»* porque só verifica que o campo não está vazio: os 99 % eram frases como `header 1x4 2,54` a passar por números de peça. Só se viu quando uma ferramenta tentou **usar** o campo e foi procurar isso à LCSC. Os 48 sem MPN são resistores por encapsulamento, headers lisos e módulos comprados montados — **vazio é resposta, não lacuna** |
| [kicad/compara_netlist.py](kicad/compara_netlist.py) | **v0.1.0** | A netlist de antes contra a de depois, **por pertença** — o conjunto de `(referência, pino)` de cada rede, e nunca os nomes, que mudam de propósito. É o critério de aceitação que a skill `/kicad` §9 exige depois de cada alteração a um gerador. **Conta o que leu**: escrevi um leitor a olho que devolveu «0 redes» sobre um ficheiro com 74, e um leitor que perde linhas em silêncio não reprova nada — aprova tudo. A guarda foi provada a cegá-la a um bloco que continua no ficheiro |
| [kicad/lista_roteamento.py](kicad/lista_roteamento.py) | **v0.3.1** | O que falta rotear, **lido da placa e não da memória**, ordenado por blocos com referências, coordenadas, larguras e comprimentos. Existe porque havia uma lista escrita à mão que dizia 71 ligações quando a placa tinha 12: a colocação mudou três vezes e a lista não soube. Uma lista de trabalho errada não é um documento desactualizado — é alguém a rotear para sítios onde as peças já não estão. **Conta o que leu e declara o que deixou de fora**: as ligações pad↔zona não são pistas, são pads que perderam o plano, e saem numa secção própria em vez de desaparecerem da contagem |
| [tools/codigos_lcsc.py](tools/codigos_lcsc.py) | **v0.2.0** | **O código LCSC de cada linha SMD** — o MPN é para humanos, e a fábrica não monta uma linha sem `Cxxxxx`. Lê as linhas SMD do **atributo** do footprint na placa (não do nome da biblioteca) e resolve cada uma: peça com MPN por casamento exacto, passivo genérico por busca **verificada**. ⚠️ A busca do distribuidor devolve um **LED** em primeiro lugar para `4.7MΩ 0805`; por isso a ordem não decide nada — pedem-se cinquenta e exige-se que a descrição bata em valor, encapsulamento, classe, tolerância (1 %) e dieléctrico quando a troca se ouve. **26 de 26**, cada uma com a descrição que a justifica gravada em `codigos_lcsc.json` |
| [kicad/dnp_para_pcb.py](kicad/dnp_para_pcb.py) | **v0.1.0** | Leva o **DNP do esquema para a placa**. O `(dnp yes)` vive nos símbolos e o `pcb export pos --exclude-dnp` lê o atributo do *footprint* — a opção estava escrita e não fazia nada, e o CPL mandava colocar **oito peças que a BOM não comprava**. Usa a API do `pcbnew` para serializar a marca como esta versão a lê |
| [kicad/rotula_tp.py](kicad/rotula_tp.py) | **v0.2.0** | Põe o **nome da rede** ao lado de cada ponto de teste, na serigrafia. A placa dizia `TP1`…`TP11` e mais nada — os nomes viviam no `Value`, que está na F.Fab e **não se imprime**. ⚠️ O nome sai do `(net ...)` do **pad**, nunca do `Value`: o `Value` é uma afirmação sobre a rede e o pad **é** a rede, e um rótulo errado em tinta na peça física não se corrige. A referência do ponto passa para a Fab — na bancada `TP7` não diz nada e `+3V3A` diz tudo. Feito no **painel** (5/5, portão limpo). No **cérebro** não cabem onze rótulos espalhados — 4 mm de passo contra 6 mm — e entra `--legenda`: **um bloco** de 12 linhas a 14 mm da fila, com `texto×texto 0→0` e **zero colisões acrescentadas**. ⚠️ Lá a referência sai da serigrafia e aqui **fica**: quando o nome está ao lado, `TP7` é ruído; quando está noutro sítio, é o índice |
| [kicad/refs_apontam.py](kicad/refs_apontam.py) | **v0.1.0** | **Cada referência aponta para a própria peça?** — a pergunta que nenhum DRC faz, porque não é geometria proibida, é **significado**. Uma referência a 11,5 mm da sua peça e a 5 mm da vizinha passa em todos os verificadores e identifica a peça errada; quem solda à mão lê o designador mais próximo. Eram **8**. A §11g-ter da skill `kicad_layout` já o tinha previsto por escrito sem que nada o medisse — diagnosticado e por instrumentar. O `para_fabrica` recusa o pacote |
| [kicad/funcao_dos_pads.py](kicad/funcao_dos_pads.py) | **v0.3.1** | **O pad faz o que a ficha diz que faz?** O ERC, o DRC e «0 por rotear» verificam LIGAÇÃO; nenhum verifica SIGNIFICADO. Um símbolo genérico com pinos `1,2,3` casado com um footprint cujos pads têm função fixa dá netlist correcta e função errada — foi assim que as entradas do OPA1642 saíram trocadas nos dois canais, o dreno e a fonte do AO3400A ao contrário, e o cátodo do SS14 virado. A ficha é indexada por **REFERÊNCIA e não por footprint**: o mesmo SOT-23 é o AO3400A (1G 2S 3D) e o BAV99, e um mapa por footprint diria a mesma coisa das duas. O contrato nunca usa nomes de rede auto-gerados (`Net-(U1A--)` muda quando o defeito se conserta). Prova-se **sabotando um pad**: se não reprovar, não vale |
| [kicad/dentro_do_contorno.py](kicad/dentro_do_contorno.py) | **v0.4.0** | **O courtyard cabe na placa?** Irmã do `confere_furos`: contornos, não pads. O KiCad **não confere courtyard contra `Edge.Cuts`** e por isso o DRC dizia «geometria 0» com o `J1` do painel 2,83 mm fora do contorno — um conector com o corpo de fora tem os pads todos lá dentro, o que sai é o plástico. Mede cada vértice do courtyard contra o bordo e cada vértice do bordo contra o courtyard, porque um recorte pode entrar entre dois vértices sem chegar perto de nenhum. **Footprint sem courtyard é achado, não é passe.** Isenções uma a uma e com número na razão (os quatro M3 passam 0,06 no desenho e a anilha real sobra 0,25). ⚠️ A v0.1.0 reprovou 74 de 74 e 28 de 28 a `0.000` porque usou `SHAPE_LINE_CHAIN.Distance()`, que numa cadeia fechada mede a REGIÃO e devolve zero para todo ponto interior — **zeros idênticos em cima de tudo são leitor, não são placa** |
| [kicad/encaixe.py](kicad/encaixe.py) | **v0.5.0** | **A placa monta-se?** O portão da trilha M do IFÁ10000, **só leitura**, chamado pelo `para_fabrica`. Mede M2 (≥3 pontos de fixação não colineares — furos, NPTH, ou peças cuja ficha prove a rosca), M4 (courtyard no bordo), M6 (corpos debaixo dos arcade, pelo corpo e não pelos pads) e os **cabos entre placas** (percurso 3D pela parede de trás + a folga de serviço da caixa, contra o BOM). Não refaz contas: chama o `pcb_contorno` e o `dentro_do_contorno`. `--sabota` prova cada verificação. Nasce de *«de que vale uma placa correta eletricamente se não podemos montá-la?»* (dB, 18/09) |
| [kicad/poe_furos.py](kicad/poe_furos.py) | **v0.2.0** | Os quatro furos M3 do cérebro, com as coordenadas **derivadas** dos espaçadores da caixa em vez de escritas, e um keepout de raio 3,5 nas duas camadas à volta de cada um. Idempotente por `uuid5`, e confirma que o resultado abre no KiCad antes de gravar por cima |
| [kicad/para_fabrica.py](kicad/para_fabrica.py) | **v0.20.0** | O pacote da JLCPCB, e o **portão que se recusa a fazer um mau**. Uma encomenda é irreversível e demora semanas: o erro não se descobre no ecrã, descobre-se quando a caixa chega. Mede geometria, ligações por fazer **por rede**, planos em pedaços e ilhas órfãs; recusa se algum falhar; e só então exporta gerbers, furação, CPL e BOM, carimbados com data gerada, commit e aviso de árvore suja. `--incompleta` produz na mesma e escreve **INCOMPLETA no nome**, para que um pacote mau nunca se confunda com um bom. Diz também o que **não** pode ver: que a netlist esteja certa, e a rotação da montagem — o ângulo do CPL é o do footprint e a fábrica espera o da biblioteca dela. Corre também `fichas.py --confere`, que não vai à rede e recusa o pacote se alguma ficha em disco não falar da peça |
| [kicad/gera_footprints.py](kicad/gera_footprints.py) | **v0.3.0** | O footprint do ESP32-C3 SuperMini, que nao existe em biblioteca nenhuma. Pads com os **nomes da serigrafia** e a face declarada em maiusculas: a pinagem desta placa apareceu com tres leituras contraditorias em duas horas |
| [docs/operacao/verifica_datas.py](docs/operacao/verifica_datas.py) | **v0.2.1** | Confronta cada data dos documentos com o `git blame` da linha. Nenhuma depois do relogio, nenhuma depois do commit que a introduziu. **Data e dado gerado, nao escrito** — e o hook `UserPromptSubmit` poe o relogio no contexto para nao haver data a lembrar |
| [kicad/cotuca_cerebro/gera_conector.py](kicad/cotuca_cerebro/gera_conector.py) | **v0.10.0** | Gera a folha do conector IDC26, lado cérebro. Mesma guarda de contagem. A nota do lado esquerdo é **alinhada pela direita**: com deslocamento fixo uma nota comprida crescia para dentro do rótulo |
| [kicad/cotuca_painel/gera_painel.py](kicad/cotuca_painel/gera_painel.py) | **v0.17.0** | Gera a folha do painel inteira. Mesma guarda de contagem, mais o `nota()` — **o sinal do `y` vive num sítio só**. Na folha o `y` cresce para BAIXO e no símbolo para CIMA; escrever as continuações com `y − 3,8` imprimia 8 notas de trás para a frente |
| [kicad/cotuca_cerebro/encaixa_grade.py](kicad/cotuca_cerebro/encaixa_grade.py) | **v0.3.1** | Encaixa a geometria na grelha de 1,27 mm. **Critério de aceitação: netlist idêntica** |

## Ambiente que funciona

| Item | Versão |
|---|---|
| Placa | Daisy Seed 2 (DFM) |
| Toolchain | ARM GNU Toolchain 15.3.rel1 (`arm-none-eabi-gcc 15.3.1`) |
| libDaisy | `~/dev/libDaisy` |
| dfu-util | 0.11 |
| Bootloader | Electrosmith `dsy_bootloader_v6_4-intdfu-2000ms.bin` (vem com a libDaisy) |

> **Não use o `arm-none-eabi-gcc` do Homebrew.** Ele vem sem newlib e a libDaisy
> morre em `stdint.h: No such file or directory`. Use o cask (pede senha de admin):
> ```bash
> brew install --cask gcc-arm-embedded
> brew install dfu-util
> ```

## Build e flash

```bash
git clone --recurse-submodules https://github.com/electro-smith/libDaisy ~/dev/libDaisy
make -C ~/dev/libDaisy
```

```bash
cd src && make BOARD=daisy_seed
```

Para gravar, **use o `flash.sh` e não o `make program-dfu`**:

```bash
cd src && ./flash.sh
```

Ele faz três coisas que a regra da casa exige e o `make` não faz: grava **só
se o build desta vez passou** (o `make program-dfu` grava o binário em disco,
que pode ser de um build anterior que falhou), confere que a versão **na
placa** bate com a **do fonte**, e — desde a v0.4.0 —

> ⚠️ **repõe o estado que a gravação apagou, e diz qual repôs.**

Os params são voláteis: a gravação devolve-os todos ao valor de nascença do
manifesto. Isso já custou quatro gravações seguidas a zerar o `level` sem
aviso, e — pior — um notch de microfonia que desaparecia no flash. O
`estado.py` guarda antes do DFU e repõe depois:

```
  estado: 122 params guardados
  placa diz v0.300.0
  CONFERE
  estado: 122 params conferidos, nenhum divergiu — nada a repor.
```

> ⚠️ **A manobra de gravar o dia a dia é SÓ O RESET.** `BOOT + RESET` entra
> no DFU da **ST**, que só enxerga a flash interna — e o app mora na QSPI.
> O sintoma é `dfu-util: Last page at 0x9007760b is not writeable`
> (`Error 64`). `BOOT + RESET` serve pro `make program-boot`, uma vez por
> placa. Gravação bem-sucedida diz `File downloaded successfully` e termina
> com `Error 74` no `leave request` — isso é normal.

### O app roda da QSPI, via bootloader

Desde a v0.4.0 o firmware é compilado com `APP_TYPE=BOOT_QSPI`: **o app vive nos
8 MB da QSPI e os 128 KB de flash interna são do bootloader** da Electrosmith. O
motivo é direto — com um único módulo de DSP externo o firmware já ocupava
**94,14%** dos 128 KB internos; o segundo módulo não caberia. Na QSPI o mesmo firmware ocupa
**1,52%** de 7 936 KB.

**Uma vez por placa**, grave o bootloader. Ponha a Seed no DFU da STM —
**segura BOOT, toca RESET, solta BOOT** — e:

```bash
cd src && make program-boot
```

**No dia a dia, NÃO toque em nada.** A placa entra no bootloader por comando,
com timeout infinito:

```bash
dfu go            # pela serial — a placa reinicia no DFU e ESPERA
cd src && make program-dfu
```

Isso trocou gravações de 119, 66 e 47 tentativas (disputando a janela de ~2 s
do RESET físico) por **uma**. Ver a skill `bancada-firmware`.

**Se precisar do caminho manual** (placa travada, firmware sem o comando):
só toque **RESET** e grave durante a janela de graça do
bootloader (LED pulsando, ~2 s; segurar BOOT estende a janela):

```bash
cd src && make program-dfu
```

`make program-dfu` **sempre** termina com `Error 74` no `leave request`: a placa
pula pro app e para de responder ao último `get_status`. Se apareceu
`File downloaded successfully`, gravou.

Pra voltar pra flash interna: `make clean && make APP_TYPE=BOOT_NONE` e gravar
pelo DFU da STM — mas isso apaga o bootloader.

## Testar

```bash
cd src && ./cotuca_serial.py --send board list "k1 0.5" --gap 1.3
```

Ou duplo-clique em `src/monitor.command`.

Comandos do firmware: `board` · `list` · `mods` · `cpu` · `dyn` · `probe` ·
`bb` · `midi` · `miditest` · `midiloop` · `b1` · `b2` · `k1..k5 <0-1>` ·
`set <nome> <real>` · `tone on` · `tone off`

`b1` e `b2` **simulam os botões do painel pelo serial**, então o painel v2 é
testável antes de existir fisicamente: `b1` dispara o randomizador, `b2`
segura o parâmetro assinalado. `bb` lê a **caixa-preta** — memória em
BACKUP_SRAM que sobrevive ao RESET, pra medir o que só acontece depois que a
serial já morreu.

`set` alcança **qualquer** parâmetro pelo nome, **em unidade real** — Hz, ms, ×
— e recusa valor fora da faixa citando os limites. Com os knobs 1–3 no delay e
no level, `test_tone_freq` e `test_tone_level` são **serial-only por design** — o tom de
teste é bancada, não performance:

```
set tone_freq 440        # Hz, direto
set gate_release 500     # ms
set bench_boost 2        # vezes
```

Knob e macro seguem falando normalizado (posição de curso é normalizada por
natureza); a unidade real é só na fronteira humana.

### Curva de parâmetro

`ParamTable::Add()` aceita um expoente opcional:

```
real = min + pow(norm, curve) * (max - min)
```

`curve = 1.0` é linear (default, com caminho rápido que pula o `pow`). O
`dly_time` usa **2.5**: metade do curso do knob cobre os primeiros ~18% da
faixa, que é onde moram os delays curtos.

**Curva é decisão de manifest, não de módulo.** É ergonomia — como o knob se
comporta na mão. O DSP recebe o valor já curvado e não sabe que houve curva; o
mesmo motor de DSP serve a esculturas com curvas diferentes.

`cpu` reporta a carga do bloco de áudio (média e pico). Com a suíte inteira
(mais shimmer e detune): **59,5% média, 65,8% pico** do prazo de 1 000 µs,
medido e registado no [STATUS.md](docs/STATUS.md). A linha de delay é
constante: lê e escreve toda amostra independente do ajuste.

`mods` mostra o módulo DSP carregado, os parâmetros que ele expôs (descobertos
**por nome**, não por índice), as ligações do manifest e o extrato de memória do
alocador de SDRAM.

### MIDI

Porta MIDI na **USART1 a 31250 baud** — TX em `D13` (pino 14), RX em `D14`
(pino 15). Guia de solda do WIDI, **com o aviso do cruzamento TX↔RX**, em
[BOARDS.md](src/BOARDS.md).

O mapa CC→parâmetro mora no manifest e começa assim:

| CC | Parâmetro | Por quê |
|---|---|---|
| 20 | `dly_time` | também no knob 1 |
| 21 | `dly_mix` | também no knob 2 |
| 22 | `pre_level_ab` | também no knob 3 |
| 23 | `src_mix` | também nos knobs 4/5 |
| 24 | `gov_ceiling` | **sem knob** — a borda de feedback só é alcançável por MIDI ou serial |

Canal configurável no manifest, **omni por default**. CCs 20–24 são a faixa
"undefined" da spec, então não colidem com mod wheel, volume ou sustain.

> **MIDI é um knob remoto, não um caminho novo.** O CC entrega um valor
> normalizado à `ParamTable` e para aí — exatamente onde o knob físico para.
> Curva do manifest, `Shaped()` e `Real()` acontecem depois, iguais pra
> qualquer origem. O banco nativo verifica isso: o mesmo gesto por CC e por
> knob dá bit a bit o mesmo valor.

**Prioridade entre fontes: última escrita vence.** Serial, MIDI e o futuro knob
físico são pares, sem hierarquia — o raciocínio inteiro está no topo do
[main.cpp](src/main.cpp).

`midi` mostra o estado da porta, os contadores do parser, o mapa e os últimos
CCs recebidos. É a primeira coisa a consultar no dia da solda: ele diz **em qual
etapa o sinal parou** — fio, parser, canal ou mapa.

`miditest <cc> <val>` injeta bytes na fila de RX sem passar pela UART (prova o
parser e o mapa); `midiloop <cc> <val>` manda pelo fio e precisa de um jumper
`D13→D14` pra voltar — é o único que prova baud rate e pino. Roteiro em
[T10](src/TESTES.md).

### Bancos de teste nativos

```bash
cd src && ./test.command
```

`dynamics.h` e `midi.h` são C++ puro e board-agnostic, então a matemática dos
blocos e a máquina de estados MIDI rodam no Mac — sem placa, sem flash. **Não
substituem teste em placa**: nenhum deles toca hardware, e nenhum prova a UART.

`k5` é o **SRC MIX**: crossfade linear entre `IN_L` (piezo, `0.0`) e `IN_R`
(jack de linha, `1.0`), aplicado antes de qualquer processamento. Enquanto o
painel de 5 pots não existe, o `k4` também aponta pra ele — por isso
`req.min_knobs` segue em 4: uma placa de 4 knobs ainda alcança o SRC MIX.

Como os knobs físicos ainda não estão ligados, `ReadKnob()` devolve `-1` e o
painel vive no serial. Conversão pra uma afinação alvo, com `test_tone_freq` em
`[80, 2000]`: `norm = (Hz − 80) / 1920` — logo **440 Hz = `k2 0.1875`**.

### Medir em vez de confiar no ouvido

Com o `OUT_L` numa entrada de áudio do Mac (aqui: MiniFuse 4, canal 2):

```bash
cd src && ./cotuca_measure.py --scan --expect 440
```

Reporta a frequência dominante por FFT com interpolação parabólica, o nível em
dBFS e o erro em cents. Ou duplo-clique em `src/measure.command`.

Medido na faixa inteira (80 / 440 / 880 / 1760 / 2000 Hz): erro constante de
**+0,4 a +0,5 cents**. Sendo constante em termos *relativos*, é diferença de
clock — a taxa real do SAI da Daisy é ~0,026% acima dos 48000 Hz que o
`AudioSampleRate()` reporta e que o oscilador usa como base. Não é erro de
código, e é inaudível (1 cent é ~1/100 de semitom).

## Gotchas

- **Nada de `hal::Log()` dentro do callback de áudio.** Ele bloqueia por
  milissegundos.
- **`--specs=nano.specs` engole `%f` calado.** O Makefile já força
  `-u _printf_float`; se remover, metade da saída do painel some sem aviso.
- **O callback de RX do USB roda em ISR.** Nunca chame `Log()` dali — foi
  exatamente isso que travava a placa antes da v0.2.1. Enfileire e despache no
  `PollSerial()`.
- **Não use o `Logger` da libDaisy.** O `tx_buff_[128]` dele é único e
  compartilhado: duas linhas seguidas se sobrescrevem em pleno voo. O
  `hal::Log` transmite por conta própria, com buffer duplo e retry.
- **`sdram_pool` pede a região SDRAM inteira, e agora aparece nos 100%.** Até a
  v0.3.0 o `--gc-sections` descartava o pool porque nada chamava `AllocBig`. O
  delay, o freeze, o reverb e os motores de pitch chamam, então os 64 MB
  voltaram de uma vez: o mapa mostra `SDRAM:
  100%`. **É o pool, não o consumo** — mas não sobra espaço pra mais nada em
  `.sdram_bss`.
- **`AllocBig()` é o único acesso à SDRAM.** Declarar `DSY_SDRAM_BSS` fora de
  `hal_daisy_seed.cpp` é proibido — regra completa em [BOARDS.md](src/BOARDS.md).
  O boot imprime o extrato (`SDRAM via AllocBig: N de M bytes`); se um módulo
  alocar por fora, o número não fecha.
- **`FLASH: 0 B` no mapa de memória está certo.** O app roda da QSPI; a flash
  interna é do bootloader. Não é build vazia.

## Defeitos em aberto

**A lista viva está no [STATUS.md](docs/STATUS.md)** — é lá que se abre e se
fecha. Resumo de hoje, 04/09:

- **🔴 A serial morre sob callback longo** — prioridade 1. As rajadas de
  ~3,4 ms do callback starvam a ISR do USB. Contornado por `log_mute`,
  caixa-preta e MIDI; o conserto certo (**ring de log kick-and-forget**) está
  desenhado e não feito.
- **🟡 Pool SRAM a 92,4%** — terceiro módulo não entra sem mudar a estratégia
  de alocação.
- **🟡 Bloco 256 custa 5,3 ms de latência** — a razão de o bloco ser 48.
- **🟢 Off-by-one no contador de aplicadas** — diferença fixa de 1, não taxa
  de perda. Confirmar antes de chamar de cosmético.

Diagnóstico dos que já fecharam: [TESTES.md](src/TESTES.md).

## Signal path do sistema completo

**Montado e a tocar.** O pré-amp entrou a 04/09 e o aterramento foi refeito a
03/09; o que continua por confirmar com o multímetro está marcado no
[STATUS.md](docs/STATUS.md). O mapa de pinos vivo é o
[PINOS.md](docs/operacao/PINOS.md), que se confere a cada fecho:

- **Entradas.** `IN_L` = piezo via buffer de alta impedância (circuito externo);
  `IN_R` = jack TS/TRS de linha. **Pot 5** faz o `piezo_mix`, que é **balanço,
  não crossfade**: no centro as duas fontes entram cheias. (O `src_mix`, que
  era crossfade e derrubava as duas em −6 dB no centro, foi aposentado a
  20/08 — ver [FEATURES.md](docs/FEATURES.md).)
- **Saídas.** `OUT_L` → amp TPA3116 → exciter no corpo da escultura;
- **Painel (22/08).** `D24` switch do freeze A · `D27` LED do freeze A ·
  `D22`/`D23` freeze B (firmware pronto, fiação pendente) · `D25` B3 rearme
  do CTOS · `D26` B1 randomizador · `D28` LB do WIDI · `D29`/`D30`
  reservados pras DPDT dos excitadores · `D13`/`D14` UART do MIDI
  (**amarrados ao periférico**: USART1 em PB6/PB7, não se remapeiam) ·
  `D15`–`D19` pots (A0–A4) · livres: `D0`–`D12`, `D20`, `D21`.
  `OUT_R` → jack out. O roteamento (exciter / jack / ambos) é firmware, por
  toggle de 3 posições.
- **Jacks switched (NC)** servem de sensor de inserção via GPIO — não roteiam
  sinal.
- **Painel fixo.** 4 knobs de caráter + SRC MIX + encoder com push + 2 botões
  momentâneos + toggle + LED RGB.

## Próximos passos

**A fila viva é a do [STATUS.md](docs/STATUS.md)** — não se duplica aqui, que
é como as duas versões divergem. O que está no topo dela, a 04/09:

- **Refazer o pré-voo de medição limpa.** O pré-amp entrou a 04/09 e invalidou
  toda a medição de ganho anterior.
- **Confirmar o `AGND`/`DGND` com o multímetro**, em AC/mV e sob carga.
- **Ring de log kick-and-forget** — mata o defeito 🔴 prioridade 1 por
  arquitetura, em vez de o contornar.

Fica aqui só o que é de arquitetura e não envelhece:

- Segunda placa: **Teensy 4.1**, via um `hal_teensy41.cpp` novo — sem tocar em
  mais nada. É o que a fronteira do `hal.h` existe para permitir.
