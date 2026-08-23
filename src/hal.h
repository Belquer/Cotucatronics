#pragma once
// hal.h v0.22.0
// ============================================================
// Cotucatronics OS — HAL (Hardware Abstraction Layer)
// A fronteira de independência de fornecedor.
//
// Regra de ouro: NADA fora de hal_<board>.cpp pode incluir
// headers de fabricante (daisy_seed.h, Arduino.h, etc).
// main.cpp, params, macros, manifests e módulos RNBO só
// conhecem este arquivo.
//
// Trocar de placa = escrever um novo hal_<board>.cpp
// e compilar com: make BOARD=<nome>
// ============================================================

#include <cstdint>
#include <cstddef>

namespace cotuca::hal
{

// ---- capacidades da placa (preenchido pela implementação) ----
struct BoardInfo
{
    const char* name;         // "Daisy Seed", "Teensy 4.1"...
    int         num_knobs;    // ADCs disponíveis no painel
    int         num_buttons;  // botões momentâneos no painel
    bool        has_audio_in; // false = só síntese/playback
    float       sample_rate;
    size_t      block_size;
    size_t      big_ram_bytes; // RAM "grande" p/ delays longos, freeze etc.
                               // Daisy: 64MB SDRAM. Placas sem: 0.
    uint32_t    cpu_hz;        // clock real do núcleo, lido da placa —
                               // não o nominal. Boost muda isto.
};

// ---- áudio ----
// Callback genérico: buffers de float não-intercalados.
// in/out[canal][amostra]. Canais fixos em 2 por simplicidade.
using AudioCallback = void (*)(const float* const* in,
                               float**             out,
                               size_t              size);

// ---- serial (painel falso / debug) ----
using SerialLineCallback = void (*)(const char* line);

// ---- MIDI ----
// O HAL entrega BYTES CRUS, um a um. Não parseia: a máquina de
// estados MIDI é board-agnostic e mora em midi.h, do lado do shell.
// A placa só sabe puxar bytes de um fio a 31250 baud.
//
// Por que byte e não mensagem: parser no HAL significaria reescrever
// o parser a cada placa nova, e é justamente o tipo de lógica que não
// muda de hardware pra hardware. O que muda é COMO os bytes chegam —
// DMA, interrupção, polling —, e isso sim é assunto da placa.
using MidiByteCallback = void (*)(uint8_t byte);

// ---- contrato que toda placa implementa ----
bool             Init();                       // clocks, codec, power
const BoardInfo& Info();
void             StartAudio(AudioCallback cb);

// Pede um tamanho de bloco. Chamar ANTES de StartAudio; depois
// dele não tem efeito. A placa pode recusar e manter o seu — o
// valor efetivo fica em Info().block_size, que é a verdade.
//
// É o manifest quem pede, não a placa quem impõe: bloco define
// LATÊNCIA, e latência é característica do instrumento.
void             RequestBlockSize(size_t block_size);
float            ReadKnob(int index);          // 0.0–1.0 (stub até ter pots)
// true = PRESSIONADO. Já com debounce; quem chama vê só transições
// limpas. O contrato é NÍVEL, não evento: detectar borda é do shell,
// que é onde se sabe o que uma borda significa.
//
// Chamar do loop principal, em qualquer ritmo — o debounce é por
// tempo, não por contagem de chamadas.
bool             ReadButton(int index);        // true = pressionado
// NIVEL CRU do pino: sem debounce, sem borda, sem estado. Existe para
// responder "o botao fecha para GND?" sem passar por camada nenhuma que
// possa mascarar a resposta — mesma razao de o `pk` medir a entrada de
// audio antes de qualquer ganho. true = pino BAIXO (o que o botao faz).
bool             ButtonNivelCru(int index);

// Nivel CRU do pino ligado ao pad LB do WIDI (seed::D28), com pull-up
// interno. Devolve o que o pino le, SEM interpretacao — quem classifica
// "conectado" ou "anunciando" e o main, porque a polaridade e a legenda
// sao do modulo, nao da placa.
//
// Pull-up interno significa que pino solto le ALTO. Entao "alto
// constante" e ambiguo: pode ser LED apagado, ou fio nao ligado. O
// comando `ble` diz isso em vez de fingir que sabe.
// Le o pad LB (D28) do WIDI. Nivel CRU, sem interpretacao.
//
// ERAM TRES. LG (D29) e LR (D30) foram lidos por dois dias e NAO
// MARCARAM NADA util — removidos do hardware em 20/08, com D29/D30 de
// volta ao pool de pinos livres (ver BOARDS.md).
//
// O QUE O LB INDICA SEGUE PERGUNTA ABERTA. A legenda da CME ("pisca
// lento = anunciando, fixo = conectado") foi assumida como sendo do LB
// e o dB DESMENTIU por medicao: desconectou e o LB ficou FIXO. Entao o
// LB nao e "conectado/desconectado" — e o que ele carrega ainda nao se
// sabe. A hipotese "e um dos outros dois" morreu junto com eles: LG e
// LR estao descartados como candidatos POR NAO MARCAREM, o que e
// resultado, nao ausencia de resultado.
bool             ReadWidiLed();

// Reinicia a placa DIRETO no bootloader, com timeout INFINITO — ela
// fica esperando a gravacao ate ser gravada.
//
// Isto elimina o maior atrito da bancada: a janela de ~2 s do
// bootloader depois de um RESET fisico. Esta sessao gastou umas quinze
// gravacoes tentando acertar esse tempo, e varias falharam porque o
// comando saiu depois do toque. Agora a ORDEM some do problema: a
// placa entra em DFU por comando e espera.
//
// NAO TEM VOLTA POR SOFTWARE. Uma vez em DFU, so gravar (ou um RESET
// fisico) traz o app de volta — por isso o comando exige confirmacao.
void             RebootToDfu();
void             SetLed(bool on);

// LEDs de painel, por INDICE — quem chama nao conhece pino. Cada um
// SEGUE O PARAMETRO que o manifest lhe atribuiu, nunca o gesto:
// indicador reporta estado real, nao intencao.
void             SetPanelLed(int idx, bool on);
int              PanelLedCount();
void             Log(const char* fmt, ...);    // printf via USB/serial
void             SetSerialLineCallback(SerialLineCallback cb);
void             PollSerial();                 // chamar no loop principal
void             DelayMs(uint32_t ms);

// Relogio monotonico em ms desde o boot. Existe pra INSTRUMENTACAO: um
// contador de voltas sem carimbo de tempo diz que o loop parou, mas nao
// QUANDO — e o quando e o que cruza com o evento que o matou.
uint32_t         NowMs();

// ---- MIDI: liga a porta e registra quem recebe os bytes ----
//
// Chamar UMA VEZ, no boot, antes do StartAudio. É esta chamada que
// liga a UART — placa que nunca a chama não paga periférico nenhum.
// Placa sem UART: implementa vazio, e PollMidi() nunca entrega nada.
void             SetMidiByteCallback(MidiByteCallback cb);

// Despacha os bytes acumulados. CHAMAR DO LOOP PRINCIPAL, junto com
// PollSerial() — e pelo mesmo motivo: o RX chega por interrupção, e
// rodar o parser (que mexe em params e pode logar) dentro da ISR é a
// receita do deadlock que matou o `list` na v0.2.0. A ISR só
// enfileira; quem despacha é o loop.
void             PollMidi();

// Transmite bytes crus pela MESMA porta. Existe pro teste de
// loopback (T10): com um jumper de TX pra RX, o firmware manda uma
// mensagem MIDI e ela volta pelo caminho real — UART, DMA, ISR, fila,
// parser. É a única validação que prova baud rate e pino.
void             MidiSendBytes(const uint8_t* bytes, size_t n);

// GANCHO DE BANCADA. Empurra um byte na fila de RX como se tivesse
// vindo do fio. Prova o parser e o mapeamento sem hardware nenhum —
// mas NÃO prova a UART, e por isso não substitui o loopback.
//
// CUIDADO: escreve no mesmo índice que a ISR do RX. Com MIDI ao vivo
// entrando, as duas escritas correm. É gancho de bancada com a porta
// silenciosa, não ferramenta de uso geral.
void             MidiInjectByte(uint8_t b);

// Diagnóstico da porta, pro comando serial `midi`.
bool             MidiRxActive();    // a escuta chegou a começar?
uint32_t         MidiDropped();     // bytes perdidos por fila cheia
// Linhas de comando RECUSADAS por passarem do tamanho maximo. Linha
// truncada nao e comando: executar metade e pior que nao executar.
uint32_t         SerialTruncadas();

// ---- OLED (SSD1309 128x64, 4-wire SPI) ----
//
// TESTE DE VIDA, nao driver de painel. Prova que os cinco fios chegam e
// para ai — sem fonte, sem layout, sem taxa de refresh.
//
// DECLARACAO QUE PRECEDE O USO: este display NAO E DETECTAVEL. O
// barramento e TWO_LINES_TX_ONLY e o painel nunca responde, entao
// nenhuma funcao aqui pode dizer "presente". `OledInit` devolve se a
// CONFIGURACAO foi feita — o `Init` da libDaisy e `void` e nao tem o que
// reportar. Chamar isso de deteccao seria a string fixa `placa viva: sim`
// outra vez (bts, cap. instrumentacao).
bool             OledInit();          // true = CONFIGURADO, nao detectado
void             OledPreenche(bool aceso);   // tela toda acesa/apagada
void             OledTexto(const char* l1, const char* l2);
bool             OledConfigurado();
uint32_t         MidiErrors();      // erros de UART (overrun/framing/ruido)
uint32_t         MidiRestarts();    // vezes que a escuta DMA foi rearmada

// Reinicia a escuta DMA. Erro de UART pode abortar a recepcao no stack
// da ST, e ela NAO volta sozinha — o sintoma e "funcionou e parou pra
// sempre" com a porta reportando escuta ativa. O PollMidi ja rearma
// automaticamente; isto existe pro comando manual de bancada.
bool             MidiRestart();

// Aloca da RAM "grande" (SDRAM na Daisy). Chame só no boot,
// nunca no callback de áudio. Retorna nullptr se a placa não
// tem essa capacidade ou se acabou — quem chama DEVE checar.
//
// Este é o ÚNICO portão de acesso à RAM grande no projeto.
// Declarar DSY_SDRAM_BSS (ou equivalente) fora de hal_<board>.cpp
// é proibido — ver BOARDS.md.
void*            AllocBig(size_t bytes);

// Carga do bloco de áudio, 0.0–1.0 (1.0 = estourou o prazo).
// Média e pico desde o boot. Placa sem medidor devolve -1.
// Relogio do CTOS. Mede o tempo do EFEITO por bloco — CpuLoadMeter
// mede o callback inteiro e nao sabe dizer QUEM gastou; sem separar, o
// veto puniria o inocente.
//
// EM TICKS, DE PROPOSITO, e isto e a correcao de um defeito real.
// `System::GetUs()` faz `CNT / (freq/1e6)` — divide ANTES de quem chama
// subtrair. O CNT e um contador livre de 32 bits, mas o valor devolvido
// vive numa faixa muito menor: a 200 MHz, 2^32/200 = ~21,5 s. Subtracao
// uint32 so corrige a volta quando o valor usa a faixa INTEIRA; como
// nao usa, uma volta no meio da medicao produz ~4,27e9 em vez de ~100.
//
// Foi exatamente o numero que desligou o freeze num veto falso (caso 9
// do battle_scars). Medir em TICKS e converter DEPOIS da subtracao faz
// a volta se corrigir sozinha, porque o CNT ocupa os 32 bits todos.
uint32_t         NowTicks();
uint32_t         TicksPerUs();

float            CpuLoadAvg();
float            CpuLoadMax();

// Zera média e pico. Necessário pra rearmar o watchdog de módulo:
// CpuLoadMax() é monotônico, então sem reset um estouro passado
// travaria o watchdog pra sempre.
void             CpuLoadReset();

// ---- CAIXA-PRETA: memória que sobrevive ao RESET ----
//
// Existe por um motivo específico e recorrente: MEDIR O QUE SÓ
// ACONTECE DEPOIS QUE A SERIAL JÁ MORREU. Um módulo pesado derruba
// o USB alguns blocos depois de subir, e a partir daí a placa não
// tem mais como contar o que viu — todo instrumento que depende de
// log vira inútil justamente no trecho que interessa.
//
// Com isto o firmware grava DURANTE o voo e a gente lê DEPOIS do
// reset, quando o serial voltou a funcionar. É gravador de voo, não
// armazenamento: sobrevive a RESET, NÃO sobrevive a queda de
// energia, e não deve guardar nada que importe pro instrumento.
//
// Escrever é barato (uma store) e pode ser feito do callback de
// áudio. Ler é do loop principal.
//
// Slot fora da faixa é ignorado na escrita e devolve 0 na leitura.
// Depois de energizar do zero o conteúdo é lixo: Valid() diz se o
// que está lá foi escrito por uma execução anterior ou não.
void  BlackboxWrite(int slot, float value);
float BlackboxRead(int slot);
bool  BlackboxValid();   // false = ligou agora, não há voo anterior
void  BlackboxArm();     // marca "há dados"; chamar antes de gravar

// SILENCIA o Log no caminho do USB: ele passa a gravar só na caixa-
// preta. Existe porque em voo NINGUÉM lê a serial — ela morre — e as
// confirmações de comando são luxo que está custando o instrumento.
void  LogMute(bool on);
uint32_t MidiHiWater();   // pico de bytes na fila — o furo do PollMidi
uint32_t MidiRingSize();  // capacidade, pra dar sentido ao pico
bool  LogMuted();                     // o instrumento sabe que esta mudo
void  LogAlways(const char* fmt, ...); // ...e consegue dizer isso
int   BlackboxSlots();

// Total já entregue por AllocBig(), em bytes. Serve de extrato:
// o boot loga isto, e módulo que aloque por fora não aparece aqui.
size_t           AllocBigUsed();

} // namespace cotuca::hal
