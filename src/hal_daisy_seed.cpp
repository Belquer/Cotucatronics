// ============================================================
// Cotucatronics OS — HAL: Daisy Seed
// Único arquivo do projeto que pode incluir daisy_seed.h.
// ============================================================

// hal_daisy_seed v0.86.0
#include "hal.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include "hid/encoder.h"
#include "hid/disp/oled_display.h"
#include "util/oled_fonts.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

#define COTUCA_HAL_VERSION "0.86.0"

using namespace daisy;

namespace cotuca::hal
{

static DaisySeed hw;

static BoardInfo info = {
    .name         = "Daisy Seed",
    // DOIS, e nao e escolha: A0–A4 sao ALIASES de D15–D19, e o encoder
    // global ocupa D17/D18/D19. Sobram D15 e D16 — que sao exatamente os
    // dois pots de volume que o HARDWARE.md ja previa. Declarar cinco
    // fazia o laco iterar sobre knobs que nao existem, e o `ReadKnob`
    // devolvia -1 para todos: inofensivo enquanto era stub, mentira
    // assim que deixasse de ser.
    .num_knobs    = 2, // A0=D15 e A1=D16
    .num_buttons  = 4, // B1 rnd · B2 freeze A · B3 rearme CTOS · B4 freeze B
    .has_audio_in = true,
    .sample_rate  = 48000.f,
    // Valor de partida; o manifest pede o dele em RequestBlockSize()
    // antes do StartAudio. Ver Requirements::block_size.
    .block_size   = 48,
    .big_ram_bytes = 64u * 1024u * 1024u, // SDRAM
    .cpu_hz        = 0, // preenchido no Init com o clock real
};

// ---- botões do painel ----
//
// PINOS: D26 e D27. A escolha NÃO é arbitrária e a armadilha é real:
// na Daisy, A0–A4 são ALIASES de D15–D19 — o mesmo pino físico. Os
// cinco pots ocupam A0–A4, ou seja D15–D19, e D13/D14 são a UART do
// MIDI. D26 e D27 são dos poucos que não têm alias de ADC (A10=D25,
// A11=D28) nem função já tomada.
//
// Pull-up INTERNO: o botão fecha pra GND, então repouso lê alto e
// pressionado lê baixo. Isso dispensa resistor externo no painel.
//
// Debounce por TEMPO, não por contagem de chamadas: o contrato do
// hal.h diz que ReadButton pode ser chamado em qualquer ritmo, e
// debounce contado quebraria em silêncio se o loop mudasse de
// cadência — que é exatamente o tipo de acoplamento invisível que
// esta base já pagou caro pra aprender a evitar.
constexpr int      kNumButtons  = 4;   // B1 rnd, B2 frzA, B3 rearme, B4 frzB
// LED DO FREEZE — D24, saida. Confirmado livre no fonte antes de
// reservar: D24 nao aparece em lugar nenhum, e os potenciometros nem
// estao implementados (ReadKnob e stub), entao nao ha conflito de ADC.
// Pino se confirma no codigo, nao na memoria.
// LEDS DE PAINEL, por INDICE — o manifest declara qual botao acende
// qual, e nao conhece pino. A fronteira board-agnostic e o motivo
// desta base existir: quem troca de placa troca este arquivo e mais
// nada.
constexpr int      kNumLeds = 2;   // 0 = freeze A (D27), 1 = freeze B (D23)
static GPIO        leds[kNumLeds];
static GPIO        widi_led;     // LB=D28 (LG/LR removidos 20/08 — nao
                                 // marcavam nada, medido)
// 30 ms, MEDIDO — nao escolhido.
//
// Eram 10 ms, e com o botao de arcade EG Starts cinco batidas viraram
// SEIS alternancias: o microswitch quica mais que 10 ms, sobretudo na
// soltura. 30 ms fica bem acima do quique tipico e continua
// imperceptivel como gesto — nenhum humano bate duas vezes em 30 ms.
//
// Se um dia um botao diferente exigir outro numero, o instrumento pra
// medir ja existe: o contador de batidas no `board`, contra a
// contagem do operador. Paridade nao serve — 5 e 7 batidas dao o
// mesmo estado final.
constexpr uint32_t kDebounceMs  = 30;

struct ButtonState
{
    GPIO     gpio;
    bool     stable   = false; // já filtrado
    bool     raw_last = false;
    uint32_t changed_ms = 0;
};
static ButtonState buttons[kNumButtons];

// ---- pool SDRAM: alocador bump, simples e determinístico ----
static char DSY_SDRAM_BSS sdram_pool[64u * 1024u * 1024u];
static size_t             sdram_used = 0;

// ---- áudio: adapta o callback genérico ao formato da libDaisy ----
static AudioCallback user_cb = nullptr;

// Mede quanto do prazo do bloco o DSP consome. GetTick() é barato
// (contador de ciclos), então instrumentar o callback não distorce
// a medida de forma relevante.
static CpuLoadMeter cpu_meter;

static void DaisyAudioCb(AudioHandle::InputBuffer  in,
                         AudioHandle::OutputBuffer out,
                         size_t                    size)
{
    cpu_meter.OnBlockStart();
    if(user_cb)
        user_cb(in, out, size);
    cpu_meter.OnBlockEnd();
}

// ---- serial ----
//
// ATENÇÃO ao contrato: UsbRxCallback roda em contexto de INTERRUPÇÃO.
// Chamar line_cb() dali significa rodar o painel falso inteiro dentro
// da ISR do USB — e como hal::Log() transmite pelo mesmo periférico,
// o segundo Log fica esperando um TX que só completa quando a ISR
// retornar. Deadlock: a CDC trava calada e a placa vira tijolo até o
// reset. Foi exatamente isso que "list" (4 Logs seguidos) provocou.
//
// Então a ISR só ENFILEIRA linhas prontas, e PollSerial() — chamado do
// loop principal — é quem despacha. É pra isso que PollSerial existe
// no contrato do hal.h.
static SerialLineCallback line_cb = nullptr;

// 160, e o numero tem razao: e o mesmo teto do `hal::Log`. Uma linha
// que a placa consegue IMPRIMIR tem que ser uma linha que ela consegue
// LER de volta — senao o `preset dump` produz texto que ela mesma
// recusa, que foi exatamente o que aconteceu em 23/08 com 64.
constexpr size_t kLineMax    = 160;
constexpr size_t kQueueLines = 8;

// montagem da linha corrente: só a ISR encosta
static char   rx_buf[kLineMax];
static size_t rx_len = 0;

// fila SPSC: a ISR move q_head, o loop principal move q_tail.
// Cada índice tem um único escritor, então não precisa de lock.
static char              queue[kQueueLines][kLineMax];
// Marcadores da autopsia do loop. Nao sao volatile de proposito: sao
// escritos e lidos so no loop principal — o unico contexto que os toca.
static uint32_t          ser_m1 = 0, ser_m2 = 0, log_m3 = 0;
// TRAVA DE GRAVACAO. Quinta vez nesta sessao que instrumentacao apaga o
// proprio dado: a sessao que LE a caixa-preta tambem passa pelo
// PollSerial e pelo Log, e sobrescrevia os marcadores antes de eu
// olhar. So grava depois do BlackboxArm(), que so o voo chama.
static bool              bb_rec = false;

// ---- DEAD-MAN DO CDC: host morto vira ESTADO, nao excecao ----
//
// O host pode sumir sem avisar o device: ele reclama "Device not
// configured" do lado dele, e o lado de ca nao recebe memo nenhum.
// Enumeracao segue "configurada" pra sempre e a transmissao pendente
// nunca completa — o endpoint IN espera um host que nao vai ler.
//
// Entao o detector tem que ser NOSSO: N falhas seguidas declaram o
// CDC morto, e a partir dai o Log NAO CHAMA MAIS o transmit. Nao
// adianta limitar a espera de uma chamada que pode nao voltar; o
// unico conserto e nao fazer a chamada.
//
// REARME por RX: byte chegando pela serial e prova de host vivo —
// e a unica prova confiavel, porque vem do outro lado.
constexpr int            kCdcDeadAfter = 3;
// Intervalo de ressurreicao do CDC. Meio segundo e imperceptivel pra
// quem esta na bancada e barato pro loop: uma tentativa unica, sem
// System::Delay, entao o custo e o de uma chamada que falha rapido.
constexpr uint32_t       kCdcRetryMs   = 500;
static uint32_t          cdc_retry_ms  = 0;
// MUDO DO LOG, EM ESCOPO DE FICHEIRO — e a correcao de 27/08.
//
// Era um `static bool` DENTRO do Log, e por isso a ISR de recepcao nao
// lhe chegava. O rearme por byte recebido limpava o `cdc_dead` e o
// `cdc_fails` e deixava este armado: o canal voltava a "vivo" e continuava
// com UMA tentativa por chamada, sem espera entre elas. Uma tentativa
// isolada raramente calha com o endpoint livre, entao o log ficava morto
// para sempre com cara de ressuscitado.
//
// Irmao desprotegido (bts 13q) em forma de sinalizador: dois estados
// descrevem a mesma coisa, o rearme tocava num e nao no outro. O sintoma
// e o pior possivel — placa viva, arranque completo, e ZERO bytes: em
// 27/08 isso passou por firmware travado durante uma noite inteira, e
// custou uma placa que se suspeitou morta sem estar.
static bool              log_mudo      = false;
// Teto da carencia de enumeracao. 3 s cobre com folga o tempo de o host
// enumerar um CDC; a saida antecipada faz com que quase nunca se pague.
constexpr uint32_t       kBootMinMs    = 3000;  // sempre esperado
constexpr uint32_t       kBootGraceMs  = 2000;  // e ate mais isto, se preciso
static bool              cdc_dead      = false;
static bool              log_muted     = false;
// UM CANAL QUE ATRAVESSA O MUDO. So o banner de status usa. Se o
// instrumento nao consegue dizer "estou mudo", o mudo vira armadilha:
// foi exatamente assim que quatro medicoes viraram zero e o zero virou
// conclusao publicada (cicatriz 8).
static bool              log_force     = false;
static int               cdc_fails     = 0;
static uint32_t          log_dropped = 0;
static bool              rx_over     = false;   // linha corrente estourou
static uint32_t          serial_truncadas = 0;  // linhas recusadas por tamanho
static volatile uint32_t q_head = 0;
static volatile uint32_t q_tail = 0;

static void UsbRxCallback(uint8_t* buf, uint32_t* len)
{
    for(uint32_t i = 0; i < *len; i++)
    {
        char c = static_cast<char>(buf[i]);
        if(c == '\n' || c == '\r')
        {
            // LINHA TRUNCADA NAO E COMANDO — e recusada inteira.
            //
            // A versao anterior descartava os caracteres excedentes em
            // silencio e DESPACHAVA o pedaco. Um `set drv_drive_a 12`
            // cortado vira `set drv_drive_a 1`, que e um comando valido
            // e obedece calado: o operador pede uma coisa e a placa faz
            // outra, sem nada no log.
            //
            // Executar meio comando e pior que nao executar nenhum. O
            // contador aparece no `board`, para que "meu comando nao fez
            // nada" nao seja indistinguivel de "digitei errado".
            if(rx_over)
            {
                serial_truncadas++;
                rx_len  = 0;
                rx_over = false;
                continue;
            }
            if(rx_len > 0)
            {
                rx_buf[rx_len]      = '\0';
                const uint32_t next = (q_head + 1) % kQueueLines;
                // fila cheia: descarta a linha. Perder um comando é
                // muito melhor que bloquear dentro da ISR.
                if(next != q_tail)
                {
                    memcpy(queue[q_head], rx_buf, rx_len + 1);
                    q_head = next;
                    // REARME: chegou byte, o host esta vivo.
                    //
                    // OS TRES, e nao dois. O `log_mudo` faltava aqui e era
                    // justamente o que mantinha o canal em uma-tentativa-
                    // sem-espera depois de "ressuscitar".
                    cdc_dead  = false;
                    cdc_fails = 0;
                    log_mudo  = false;
                }
            }
            rx_len  = 0;
            rx_over = false;
        }
        else if(rx_len < kLineMax - 1)
            rx_buf[rx_len++] = c;
        else
            rx_over = true;   // estourou: a linha inteira sera RECUSADA
    }
}

// ---- contrato ----
bool Init()
{
    // Boost de clock: 400 -> 480 MHz. PERMANENTE, nao e modo bancada.
    // Folga de graca: +20% de trabalho por microssegundo, sem custo
    // relevante pra este uso. Info().cpu_hz traz o valor real lido da
    // placa, nao o nominal.
    hw.Init(true);

    // ---- BACKUP SRAM: a sequencia INTEIRA, nesta ordem ----
    // Custou uma placa muda: so o clock nao basta. No H7 o dominio de
    // backup nasce com protecao de escrita, e QUALQUER acesso a
    // 0x38800000 sem destravar da bus fault — nao "escrita ignorada",
    // FAULT. A placa morre no primeiro `bb`, com o Fx2 desligado,
    // parecendo defeito de boot.
    //
    //   1. EnableBkUpAccess  destrava o dominio (PWR_CR1_DBP)
    //   2. BKPRAM_CLK_ENABLE liga o clock da memoria
    //   3. EnableBkUpReg     mantem o conteudo — sem ele o dado
    //                        atravessa reset mas nao standby
    //
    // Chamar aqui, no Init, ANTES de qualquer leitura: a caixa-preta e
    // lida pelo comando serial, que roda muito depois, mas quem
    // configura tem que vir antes de quem consome. Familia (B) da
    // regra documentada no main.cpp — a mesma do RequestBlockSize.
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_BKPRAM_CLK_ENABLE();
    HAL_PWREx_EnableBkUpReg();
    info.cpu_hz = System::GetSysClkFreq();
    hw.SetAudioBlockSize(info.block_size);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    info.sample_rate = hw.AudioSampleRate();
    hw.StartLog(false);

    // ---- CARENCIA DE ENUMERACAO, LIMITADA E COM SAIDA ANTECIPADA ----
    //
    // `StartLog(false)` nao espera pelo host — e todo o arranque deste
    // firmware saia antes de a enumeracao terminar, para um CDC que
    // ninguem tinha aberto. Isso nunca doeu enquanto o loop chegava ao
    // shell: dali em diante havia como perguntar. Numa placa onde o
    // arranque MORRE, ou onde o canal nao ressuscita, a unica testemunha
    // morre junto — e o sintoma (porta viva, zero bytes, nem banner) e
    // INDISTINGUIVEL de firmware travado. Foi assim que uma noite inteira
    // se gastou a caçar um travamento que nao existia.
    //
    // Nao e um `Delay` cego: tenta transmitir ate conseguir, e SAI no
    // primeiro sucesso. Com host ligado custa milissegundos; sem host,
    // custa o teto e segue. O instrumento nao pode cobrar o paciente.
    {
        // MINIMO INCONDICIONAL PRIMEIRO. A versao anterior desta carencia
        // saia no primeiro envio bem-sucedido — e isso acontece ANTES de o
        // host terminar de configurar a interface: medido em 27/08, o ping
        // saia (2 bytes no fio) e tudo o que vinha a seguir morria. Sair
        // cedo demais e nao esperar sao a mesma coisa.
        System::Delay(kBootMinMs);
        const char*    ping = "\r\n";
        const uint32_t t0   = System::GetNow();
        while(System::GetNow() - t0 < kBootGraceMs)
        {
            if(hw.usb_handle.TransmitInternal((uint8_t*)ping, 2)
               == UsbHandle::Result::OK)
                break;
            System::Delay(10);
        }
    }

    Log("hal_daisy_seed v" COTUCA_HAL_VERSION);
    // ---- ADC, DEPOIS DO LOG E NAO ANTES ----
    //
    // A primeira versao punha isto ANTES do `StartLog`, com o argumento
    // "configura antes de consumir". O argumento estava certo e a
    // colocacao errada, e a diferenca so aparece quando algo corre mal:
    // entre o `hw.Init` (que enumera o USB) e o `StartLog` (que faz o
    // canal falar) ha uma janela em que a placa JA APARECE ao host e
    // ainda nao consegue dizer nada. Qualquer coisa que trave ali produz
    // "enumera e nao fala" — indistinguivel de um travamento no laco, e
    // com o log morto nao ha como perguntar.
    //
    // Regra: **inicializacao que pode falhar nao mora antes do canal que
    // reportaria a falha.** O ganho de a fazer 20 ms mais cedo nao paga
    // um modo de falha mudo, e o laco so le knobs muito depois disto.
    Log("adc: inicializando (A0=D15 · A1=D16)...");
    AdcInit();
    Log("adc: %s", AdcConfigurado() ? "pronto" : "*** FALHOU ***");
    hw.usb_handle.SetReceiveCallback(UsbRxCallback, UsbHandle::FS_INTERNAL);

    // botões: entrada com pull-up interno, fecham pra GND
    // WIDI LB: entrada com pull-up. D28 esta livre — D13/D14 sao a UART
    // do MIDI, D15-D19 sao os pots (A0-A4 sao os MESMOS pinos), e
    // D26 e D24 sao botoes; D27 virou SAIDA pro LED do freeze.
    // So o LB. D29/D30 reservados pras chaves DPDT dos excitadores.
    // D25 entrou em 22/08 como B3 = rearme do teto de soma: pull-up
    // interno e ativo em BAIXO, entao com o painel ainda inexistente o
    // pino flutuante le "solto" — botao que nao existe nao dispara.
    widi_led.Init(seed::D28, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);

    // LEDs de painel. Saida, sem pull — os LEDs tem resistor proprio
    // de 5 V acionados com 3,3 V. Brilho JULGADO SUFICIENTE de olho em
    // 22/08, entao o plano B (transistor chaveando o trilho de 5 V,
    // base/gate via 1k) fica arquivado sem custo.
    //
    // O LED do freeze A ficou no D27 e o switch no D24 porque a fiacao
    // ja estava feita assim: trocar dois numeros no fonte custa uma
    // gravacao, refazer solda custa tempo e risco de contato ruim.
    const Pin led_pins[kNumLeds] = {seed::D27, seed::D23};
    for(int i = 0; i < kNumLeds; i++)
    {
        leds[i].Init(led_pins[i], GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
        leds[i].Write(false);
    }

    // B1 randomizador, B2 FREEZE, B3 rearme do CTOS.
    // B2 mudou de D27 pra D24 junto com o LED — ver acima.
    // B1 randomizador · B2 freeze A · B3 rearme do CTOS · B4 freeze B
    const Pin btn_pins[kNumButtons]
        = {seed::D26, seed::D24, seed::D25, seed::D22};
    for(int i = 0; i < kNumButtons; i++)
        buttons[i].gpio.Init(btn_pins[i], GPIO::Mode::INPUT, GPIO::Pull::PULLUP);

    return true;
}

const BoardInfo& Info() { return info; }

void RequestBlockSize(size_t block_size)
{
    if(block_size == 0)
        return;
    hw.SetAudioBlockSize(block_size);
    info.block_size = hw.AudioBlockSize(); // o efetivo, não o pedido
}

void StartAudio(AudioCallback cb)
{
    cpu_meter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
    user_cb = cb;
    hw.StartAudio(DaisyAudioCb);
}

// ---- POTS ----
//
// DOIS canais de ADC, em A0 (D15) e A1 (D16). Os outros tres pinos do
// bloco (D17/D18/D19, ou seja A2/A3/A4) sao o encoder global, e por isso
// nao ha um terceiro pot possivel sem mudar o painel.
//
// O `adc_pronto` NAO e cerimonia: o `GetFloat` de um ADC nao inicializado
// devolve um numero, e numero de periferico nao configurado e exatamente
// a familia que este projeto ja pagou tres vezes — instrumento nao
// inicializado le zero, e zero parece dado. Sem a flag, um erro de ordem
// no arranque daria pots "a zero" em vez de "sem pots".
static daisy::AdcChannelConfig adc_cfg[2];
static bool                    adc_pronto = false;

void AdcInit()
{
    if(adc_pronto) return;
    adc_cfg[0].InitSingle(seed::D15);   // A0
    adc_cfg[1].InitSingle(seed::D16);   // A1
    hw.adc.Init(adc_cfg, 2);
    hw.adc.Start();
    adc_pronto = true;
}

// SENTIDO DE ROTACAO — os dois pots estao com as pontas trocadas.
//
// Medido em 31/08: girar no sentido HORARIO DIMINUIA o volume. Nao e
// defeito de software, e a ordem dos dois fios das EXTREMIDADES do pot:
// o cursor sobe em direcao a ponta que esta no GND em vez da que esta
// no 3V3. Resolver com ferro de soldar era trocar dois fios numa placa
// ja lotada; resolver aqui custa uma subtracao e fica DECLARADO.
//
// Isto mora no HAL, e nao na macro, por uma razao de fronteira: o
// sentido de um potenciometro e um fato do COBRE, nao do instrumento.
// Invertendo na leitura, tudo a jusante — captura, histerese, passos,
// `Inverso()` — ja trabalha em coordenadas corrigidas, e nao ha um
// segundo lugar onde alguem possa esquecer o sinal.
static const bool kKnobInverte[2] = {true, true};   // A0 (L), A1 (R)
// ⚠️ LIGADA, E MEDIDA COM OS DOIS EXTREMOS DECLARADOS PELO dB (03/09).
//
//     pote no MAXIMO  ->  cru 0.0008
//     pote no MINIMO  ->  cru 0.9753
//
// O potenciometro le ao contrario, e por isso a inversao fica.
//
// EU DESLIGUEI-A DURANTE MEIA HORA, e o erro merece ficar escrito porque o
// dB o nomeou como regra: eu li `0,975` no ADC e ASSUMI que aquela posicao
// era o maximo. Nunca perguntei onde o botao estava. Errei a direccao duas
// vezes — em 31/08 e hoje — e as duas por inferir a posicao a partir do
// valor.
//
//   O VALOR CRU DIZ *QUANTO*. NAO DIZ *ONDE*.
//   A posicao fisica e informacao que so quem esta na bancada tem, e esta
//   sempre a uma pergunta de distancia.
//
// Se algum dia isto voltar a parecer errado: NAO mexer no bool. Ler o cru
// nos dois extremos, com alguem a dizer qual e qual, e so entao decidir.

// BATENTES — o pot nao chega as pontas, e por isso nao silenciava.
//
// Medido em 31/08, varrendo os dois de ponta a ponta (ja invertidos):
//
//     k1 (L)  min 0.0250   max 0.9993
//     k2 (R)  min 0.0247   max 0.9994
//
// O fundo em 0.025 nao e silencio: e cerca de -32 dB, audivel. E um pot
// que nao alcanca o silencio nao tem curso — a mesma regra que aposentou
// o `in_gain` e que fixou o `mix` do tarolator.
//
// A causa e do cobre, nao do codigo: resistencia em serie nas pontas e a
// referencia do ADC nao coincidirem com os extremos do cursor. Nenhum pot
// real chega a 0.000 e 1.000, e por isso a correcao mora AQUI, ao lado da
// inversao, e nao no instrumento.
//
// As constantes ficam DENTRO do curso medido, nao nele: assim cada ponta
// ganha uma pequena zona morta onde o valor assenta exatamente em 0 ou 1
// e la FICA. Batente colocado no valor medido daria um extremo que so se
// alcanca com sorte, e que o ruido do ADC faria tremer.
static const float kKnobFundo = 0.035f;   // abaixo disto e' silencio
static const float kKnobTopo  = 0.990f;   // acima disto e' unidade

// O valor de COBRE, sem inversao e sem batentes. Existe para se poder
// RE-MEDIR: depois de esticar as pontas, `ReadKnob` nunca mais mostra
// onde o pot realmente para, e uma correcao que apaga a propria evidencia
// nao se pode conferir.
float ReadKnobCru(int index)
{
    if(!adc_pronto || index < 0 || index >= 2) return -1.f;
    return hw.adc.GetFloat(index);
}

float ReadKnob(int index)
{
    // -1 SIGNIFICA "SEM POT", e continua a ser um valor fora do dominio
    // de proposito: 0.0 seria indistinguivel de um pot no zero, e a regra
    // desta casa e que ausencia se representa FORA do dominio.
    if(!adc_pronto || index < 0 || index >= 2) return -1.f;
    float v = hw.adc.GetFloat(index);
    // A ausencia NAO se inverte: 1-(-1) daria 2, um valor dentro de
    // nenhum contrato. So o dominio valido roda.
    if(kKnobInverte[index]) v = 1.f - v;
    v = (v - kKnobFundo) / (kKnobTopo - kKnobFundo);
    return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
}

bool KnobInvertido(int index)
{
    return (index >= 0 && index < 2) && kKnobInverte[index];
}

bool AdcConfigurado() { return adc_pronto; }

// ENTROPIA FISICA, e SO ela — sem relogio misturado aqui de proposito.
//
// Quem chama tem de poder perguntar "o ADC esta mesmo a variar?", e isso
// so se responde se o que sai daqui for exclusivamente ADC. Misturar o
// contador de ciclos tornaria o valor sempre diferente, e a variacao
// deixaria de dizer se o conversor esta vivo — mediria o relogio.
//
// O ruido e real e ja foi medido: 0,23 passos de 127 nos pots, que sao
// ~119 contagens de 65535. Os bits baixos da mantissa mexem sozinhos.
//
// Devolve 0 se o ADC nao estiver pronto: zero e a resposta honesta de
// "nao sei", e quem chama que decida.
uint32_t EntropiaAdc()
{
    if(!adc_pronto) return 0;
    uint32_t e = 0;
    for(int k = 0; k < 2; k++)
    {
        const float v = ReadKnobCru(k);
        uint32_t     b;
        memcpy(&b, &v, sizeof(b));
        e = (e << 16) ^ (e >> 16) ^ b;
    }
    return e;
}

// ---- SONDA DE PINO DO ADC: solto ou ligado? ----
//
// Um pino SOLTO segue o pull interno: com pull-up le alto, com pull-down
// le baixo. Um pino ligado ao cursor de um pot ve uma fonte de impedancia
// BAIXA (10 kohm tipico contra ~40 kohm do pull interno) e NAO se deixa
// arrastar — fica onde o divisor o poe, nas duas leituras.
//
// Isto decide sem multimetro o que o valor do ADC nao consegue dizer: um
// pino solto tambem devolve um numero, e numero de pino solto e
// indistinguivel de pot parado. Mesma familia do `fio` do OLED.
//
// PRECO DECLARADO: reconfigura os pinos como GPIO e MATA o ADC ate um
// reboot. Nao ha como nao matar — sao os mesmos pinos.
// ---- CONTROLO POSITIVO DO ADC ----
//
// A pergunta que nenhuma leitura de valor responde: o `GetFloat(0)` esta
// mesmo a ler o D15, ou esta a ler outra coisa? Um ADC mal ligado tambem
// devolve um numero, e numero de canal errado e indistinguivel de pot
// parado — foi assim que dois pots diferentes leram o MESMO valor e
// derivaram juntos, e eu gastei uma sessao a caçar o cobre.
//
// Aqui a placa DIRIGE o proprio pino e ve se o ADC acompanha. Se
// acompanhar, o par (canal, pino) esta certo e o defeito e a montante. Se
// nao acompanhar, o defeito e meu.
//
// A sonda tem de provar que consegue devolver ALGO DIFERENTE antes de a
// sua leitura valer — regra da casa, e esta e a forma dela para o ADC.
//
// SEGURO com o pot ligado: o pino sai por 10 kohm tipico, 0,33 mA.
void AdcControloPositivo(float* alto_l, float* baixo_l,
                         float* alto_r, float* baixo_r)
{
    const daisy::Pin pinos[2] = {seed::D15, seed::D16};
    float a[2], b[2];
    for(int i = 0; i < 2; i++)
    {
        daisy::GPIO g;
        g.Init(pinos[i], GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
        g.Write(true);  System::Delay(80); a[i] = hw.adc.GetFloat(i);
        g.Write(false); System::Delay(80); b[i] = hw.adc.GetFloat(i);
        // devolve o pino ao ADC
        g.Init(pinos[i], GPIO::Mode::ANALOG, GPIO::Pull::NOPULL);
    }
    if(alto_l)  *alto_l  = a[0];
    if(baixo_l) *baixo_l = b[0];
    if(alto_r)  *alto_r  = a[1];
    if(baixo_r) *baixo_r = b[1];
}

void AdcSondaPinos(bool* solto_l, bool* solto_r)
{
    const daisy::Pin pinos[2] = {seed::D15, seed::D16};
    bool alto[2], baixo[2];
    for(int i = 0; i < 2; i++)
    {
        daisy::GPIO g;
        // 60 ms, E NAO 5, e a diferenca decide o veredito.
        //
        // A primeira versao esperava 5 ms. Com um condensador de filtro de
        // 0,1 uF no pino — que e a montagem NORMAL — e o pull interno de
        // ~40 kohm, a constante de tempo e ~4 ms: em 5 ms o condensador
        // mal comecou a carregar, e a sonda lia o estado ANTERIOR em vez
        // do que o pull queria. Deu "LIGADO" a um pino que estava
        // efectivamente solto atras de um condensador em serie.
        //
        // 60 ms sao 15 constantes de tempo: o pino chega onde o pull o
        // quer, se puder chegar. Sonda que nao espera o transiente mede o
        // transiente.
        g.Init(pinos[i], GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
        System::Delay(60);
        alto[i] = g.Read();
        g.Init(pinos[i], GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);
        System::Delay(60);
        baixo[i] = g.Read();
    }
    // SEGUIU O PULL nas duas = solto.
    if(solto_l) *solto_l = (alto[0] && !baixo[0]);
    if(solto_r) *solto_r = (alto[1] && !baixo[1]);
    adc_pronto = false;   // o ADC morreu: quem chama que diga
}

bool ReadWidiLed()
{
    return widi_led.Read();
}

// ⚠️ 22/09 — O BOTAO BRANCO DO BUZIOS E NORMALMENTE FECHADO, e isto e uma
// EXCECAO DECLARADA, nao um conserto silencioso.
//
// O arcade branco que o dB montou traz um microinterruptor com DOIS terminais
// so, e em repouso ele esta FECHADO: a placa media `B1 baixo em 400/400
// amostras`, e apertar ABRIA o contacto (8 amostras altas em 1,3 milhoes). Os
// outros dois botoes, com LED, sao normalmente abertos como o desenho supoe.
//
// Porque em firmware e nao no fio: o botao nao tem terceiro terminal para onde
// mudar. A alternativa era trocar o microinterruptor — e fica registada como a
// preferida (bts 15fh), porque esta excecao e uma divergencia entre o que a
// placa DIZ e o que a placa FAZ.
//
// ⚠️ E por isso ela tem de se VER: o `pino` marca os botoes declarados NF, e o
// nivel CRU continua a reportar a verdade electrica — inverter ali faria o
// diagnostico mentir exactamente onde ele serve para achar defeito.
static const bool botao_nf[kNumButtons] = {
    true,    // B1 buzios — branco, microinterruptor NF (dB, 22/09)
    false,   // B2 freeze A
    false,   // B3 rearme do CTOS
    false,   // B4 freeze B
};

bool ButtonNormalmenteFechado(int index)
{
    if(index < 0 || index >= kNumButtons) return false;
    return botao_nf[index];
}

bool ButtonNivelCru(int index)
{
    if(index < 0 || index >= kNumButtons) return false;
    return !buttons[index].gpio.Read();   // true = BAIXO = fechado pra GND
}

bool ReadButton(int index)
{
    if(index < 0 || index >= kNumButtons)
        return false;
    ButtonState&   b   = buttons[index];
    // pull-up: baixo = fechado. "Pressionado" e fechado num botao NA e ABERTO
    // num NF — a inversao vive AQUI, na leitura logica, e so aqui.
    const bool     raw = botao_nf[index] ? b.gpio.Read() : !b.gpio.Read();
    const uint32_t now = System::GetNow();

    if(raw != b.raw_last)
    {
        b.raw_last   = raw;
        b.changed_ms = now;      // começou a quicar, espera assentar
    }
    else if(now - b.changed_ms >= kDebounceMs)
    {
        b.stable = raw;          // estável há tempo suficiente
    }
    return b.stable;
}

void SetLed(bool on) { hw.SetLed(on); }

// LED do freeze. SEGUE O PARAMETRO, nunca o gesto — quem chama tem que
// passar o estado real de `frz_on`, nao "acabei de apertar". Se o
// freeze mudar por CC, serial, preset ou queda do CTOS, o LED conta a
// verdade. Mesma regra do espelho: indicador reporta estado, nao
// intencao.
void SetPanelLed(int idx, bool on)
{
    if(idx < 0 || idx >= kNumLeds) return;
    leds[idx].Write(on);
}
int  PanelLedCount() { return kNumLeds; }

// ---- log: transmissão própria, sem o Logger da libDaisy ----
//
// O Logger da libDaisy formata tudo num `static char tx_buff_[128]`
// ÚNICO e transmite sem esperar. A segunda linha sobrescreve o buffer
// enquanto o USB ainda está lendo a primeira: sai picotada e grudada
// ("SDRAM via AllocBig:$$ 0 level..."), às vezes some. Não é ritmo — é
// o buffer ser um só, e por isso o Delay() que tentei antes não curou.
// O TransmitSync() que resolveria é protected na libDaisy.
//
// Aqui: dois buffers alternados + retry até o CDC ACEITAR. Log só
// retorna depois do aceite, então quando escrevemos em B o buffer A já
// foi entregue ao hardware, e o transmit de B só é aceito quando A
// terminou. Nenhum buffer é tocado enquanto o DMA o lê.
constexpr size_t kLogMax = 160;

// A SIMETRIA VIRA CONTRATO, e nao fica em convencao.
//
// Uma linha que a placa consegue IMPRIMIR tem que ser uma linha que ela
// consegue LER DE VOLTA. Sem isso o `preset dump` produz texto que a
// propria placa recusa — que foi exatamente o que aconteceu em 23/08,
// com o log em 160 e a entrada em 64: saiam 13 de 33 params, e a
// colagem de volta recusava um por linha.
//
// Hoje os dois numeros sao iguais porque alguem os igualou. Amanha
// alguem sobe um e nao o outro, e o defeito volta em silencio nos dois
// sentidos. Este assert e a promessa amarrada — mesmo gesto do
// `n_sacrif` e do `kFormato`.
static_assert(kLineMax >= kLogMax,
              "a linha de ENTRADA tem que caber o que a de SAIDA imprime: "
              "senao o `preset dump` gera texto que a placa nao consegue "
              "recarregar (ver battle_scars 13u)");

static uint8_t log_buf[2][kLogMax];
static int     log_slot = 0;

// NUNCA chamar do callback de áudio: isto bloqueia por milissegundos.
// Só do loop principal / boot.
// definido junto ao anel do MIDI, mais abaixo
static void ConsolaEspelha(const uint8_t* b, size_t n);

void Log(const char* fmt, ...)
{
    char    tmp[kLogMax];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(tmp, kLogMax - 2, fmt, args);
    va_end(args);
    if(n < 0)
        return;
    // TRUNCAGEM SE ANUNCIA. O `vsnprintf` devolve o que CABERIA, entao
    // aqui se sabe que cortou — e cortar em silencio e a familia
    // proibida: em 23/08 o `preset dump` emitia 13 de 33 params e
    // parecia completo, porque nada na linha dizia que ela tinha fim
    // artificial.
    //
    // As tres reticencias custam tres caracteres do proprio conteudo, e
    // e o preco certo: perder mais tres letras de uma linha ja perdida
    // vale saber que ela foi perdida.
    if(n > (int)kLogMax - 3) // vsnprintf devolve o que CABERIA
    {
        n = (int)kLogMax - 3;
        if(n >= 3) { tmp[n - 3] = '.'; tmp[n - 2] = '.'; tmp[n - 1] = '.'; }
    }

    uint8_t* out = log_buf[log_slot];
    log_slot ^= 1;
    memcpy(out, tmp, (size_t)n);
    out[n++] = '\r';
    out[n++] = '\n';

    // O ESPELHO VEM ANTES DO DISJUNTOR DO USB, de proposito: quando o host
    // esta fora e o `Log` fica mudo, a consola sem fio e a UNICA que resta —
    // e e exactamente ai que ela vale mais.
    ConsolaEspelha(out, (size_t)n);

    // DISJUNTOR. O teto de 50 ms por Log parecia generoso — e virou
    // arma: com o host fora, um fluxo de logs continuos custa 50 ms
    // CADA, e o loop principal vira zumbi gastando o dia inteiro em
    // retry. Foi o mecanismo por tras de "a serial morre sob callback
    // longo": quem morria era o LOOP, afogado aqui dentro.
    //
    // Com o disjuntor: a primeira falha ARMA o mudo, e dai em diante
    // cada Log tenta UMA vez, sem espera — custo ~zero. O primeiro
    // envio que completar desarma. Log e diagnostico; diagnostico
    // nunca pode custar o paciente.
    // ORCAMENTO DURO. O disjuntor antigo mutava MENSAGENS; este muta
    // ESPERAS, que e o que mata. Teto de 3 ms quando saudavel, uma
    // tentativa quando mudo, e o que nao couber vira estatistica em vez
    // de virar parada cardiaca do loop.
    //
    // Serial e conveniencia de bancada. O instrumento nao pode morrer
    // por ela — com a carga a 65% do prazo, o USB e oficialmente
    // hostil, e o canal de controle de voo passa a ser o MIDI.
    // MIGALHA DE PAO: primeiro caractere do formato identifica QUEM
    // chamou. No voo morto, o ultimo valor e o endereco da cela.
    if(bb_rec) BlackboxWrite(31, (float)(unsigned char)fmt[0]);

    // MUDO EM VOO: nem chega perto do USB. E o unico jeito de garantir
    // que uma chamada que nao retorna nao possa acontecer.
    if(log_muted && !log_force)
    {
        log_dropped++;
        if(bb_rec) BlackboxWrite(25, (float)log_dropped);
        return;
    }

    // GATE COM RESSURREICAO. Antes isto era definitivo: tres falhas
    // e a placa ficava MUDA PARA SEMPRE, sem retentar.
    //
    // O QUE ISSO CUSTOU (18/08, e travou a sessao inteira): no boot o
    // app loga reverb, freeze, SDRAM e o banner ANTES de o host
    // terminar de enumerar o USB. As tres primeiras falhas sao a
    // CORRIDA NORMAL da enumeracao, nao um CDC doente — e matavam o
    // log definitivamente. Depois disso a placa rodava perfeita e
    // muda, e o sintoma (porta viva, zero resposta, nem banner) era
    // indistinguivel de firmware travado.
    //
    // TERCEIRO modo silencioso sem porta de saida neste projeto,
    // depois do `log_muted` e do `bb arm`. A licao que os tres
    // ensinam junta: todo modo degradado precisa de caminho de volta
    // AUTOMATICO — consertar um caso nao conserta a familia.
    //
    // Agora: quando morto, uma tentativa a cada kCdcRetryMs. Custo em
    // regime ~zero (uma chamada por meio segundo, sem espera), e a
    // primeira que passar ressuscita o canal.
    if(cdc_dead)
    {
        const uint32_t agora = System::GetNow();
        if(agora - cdc_retry_ms < kCdcRetryMs)
        {
            log_dropped++;
            if(bb_rec) BlackboxWrite(25, (float)log_dropped);
            return;
        }
        cdc_retry_ms = agora; // deixa passar UMA tentativa
    }

    const int   tries = log_mudo ? 1 : 3;
    for(int t = 0; t < tries; t++)
    {
        // M3: marcador ANTES do TransmitInternal. Se o loop morrer com
        // M3 na frente de M2, a prisao e aqui dentro — chamada que nao
        // retorna, que nenhum orcamento consegue limitar.
        if(bb_rec) BlackboxWrite(24, (float)++log_m3);
        if(hw.usb_handle.TransmitInternal(out, (size_t)n)
           == UsbHandle::Result::OK)
        {
            log_mudo  = false;
            cdc_fails = 0; // sucesso zera o dead-man
            cdc_dead  = false; // ...e RESSUSCITA o canal
            return;
        }
        if(!log_mudo)
            System::Delay(1);
    }
    log_mudo = true;
    log_dropped++;
    if(bb_rec) BlackboxWrite(25, (float)log_dropped);
    if(++cdc_fails >= kCdcDeadAfter)
    {
        cdc_dead = true;
        if(bb_rec) BlackboxWrite(26, 1.f); // marca no registro de voo
    }
}

void SetSerialLineCallback(SerialLineCallback cb) { line_cb = cb; }

void PollSerial()
{
    // drena a fila em contexto normal, onde Log() pode transmitir
    // INSTANTANEO PELO MESMO MOTIVO, e este e o IRMAO: forma identica, so
    // a exposicao difere. Esta fila e alimentada por alguem a escrever,
    // que nao ganha a drenagem; a do MIDI, por uma maquina a 31250 baud,
    // que ganha. Consertar so a que mordeu deixaria a outra a espera do
    // dia em que alguem cole um ficheiro inteiro na serial.
    const uint32_t qhead = q_head;
    while(q_tail != qhead)
    {
        // M1/M2 cercam o line_cb. M1 == M2 significa que todo comando
        // que entrou tambem saiu: a prisao seria o proprio while da
        // fila. M1 == M2+1 significa que um comando entrou e nao
        // voltou — e ai o M3 diz se foi dentro do Log.
        // TOKEN DE VOO, nao contador. Contadores pareados nao provam
        // balanco quando o gravador liga ENTRE o par — foi o que
        // aconteceu: o BlackboxArm roda no callback, na transicao do
        // fx2, ou seja NO MEIO da linha `set fx2_on 1`. A primeira
        // linha doou so o M2, a segunda so o M1, e o "M1==M2" foi
        // acidente aritmetico com cara de alibi.
        //
        // Estado e imune a isso: se o voo morrer com o token em 1, uma
        // linha entrou e nao saiu. Ponto final.
        if(bb_rec) BlackboxWrite(30, 1.f);
        if(bb_rec) BlackboxWrite(22, (float)++ser_m1);
        if(line_cb)
            line_cb(queue[q_tail]);
        if(bb_rec) BlackboxWrite(23, (float)++ser_m2);
        if(bb_rec) BlackboxWrite(30, 0.f);
        q_tail = (q_tail + 1) % kQueueLines;
    }
}

// ============================================================
// MIDI — USART1 a 31250 baud
//
// PINOS: TX = D13 (PB6, pino físico 14), RX = D14 (PB7, pino 15).
// São os pinos que a própria libDaisy usa por default no
// MidiUartTransport, então um cabo montado pra qualquer exemplo de
// MIDI da Electrosmith serve aqui sem alteração.
//
// LEMBRE QUE É CRUZADO: o TX do módulo WIDI vai no RX da placa
// (D14) e o RX do módulo no TX da placa (D13). Guia de solda em
// BOARDS.md.
//
// MODO: DmaListenStart — DMA circular com interrupção de IDLE. É o
// mesmo mecanismo do MidiUartTransport da libDaisy, e é o único
// limpo pra MIDI: o fluxo é esporádico e em rajadas curtas, então
// polling ou desperdiça o loop ou perde byte, e uma interrupção por
// byte a 31250 baud é ruído de contexto à toa. Com DMA + IDLE a CPU
// só é incomodada quando uma rajada termina.
//
// O buffer do DMA TEM que morar em seção acessível por DMA
// (.sram1_bss, via DMA_BUFFER_MEM_SECTION). Num buffer comum o DMA
// escreve na RAM e o núcleo lê o cache: os bytes chegam, e mesmo
// assim o parser vê zeros.
// ============================================================
static UartHandler                midi_uart;
static uint8_t DMA_BUFFER_MEM_SECTION midi_dma_buf[256];
static bool                       midi_started = false;
static MidiByteCallback           midi_cb      = nullptr;

// Fila SPSC de bytes, mesmo padrão da fila de linhas do serial: a
// ISR do DMA move a cabeça, o loop principal move a cauda.
//
// 128 bytes é folga larga — a mensagem MIDI mais longa que nos
// interessa tem 3 bytes, e o loop principal gira a cada ~1 ms, que a
// 31250 baud dá tempo pra ~3 bytes chegarem.
constexpr size_t kMidiRing = 128;

static uint8_t           midi_ring[kMidiRing];
static volatile uint32_t m_head       = 0;
static volatile uint32_t m_tail       = 0;
static volatile uint32_t midi_dropped    = 0;
static uint32_t          midi_atrasos    = 0; // voltas que deixaram bytes
static uint32_t          midi_sobra_pico = 0; // maior sobra numa volta
static volatile uint32_t midi_err      = 0; // erros de UART na ISR
// TETO DO REARME. Ver o bloco no `PollMidi` — sem isto o rearme e o
// defeito, nao o conserto.
static uint32_t midi_rearme_em       = 0;   // proximo instante permitido
static uint32_t midi_rearme_seguidos = 0;   // falhas consecutivas
// Agrupamento de prioridades conferido no Init do MIDI. Falso = o
// `SetPriority` acima nao preempta nada e o conserto e um no-op.
static bool     nvic_grupo_ok        = false;
// (os contadores do teto de erros sairam com o teto — 01/09)
static volatile uint32_t midi_restarts = 0; // rearmes da escuta DMA
// MARCA D'AGUA DA FILA. O furo conhecido do MIDI: os bytes chegam por
// DMA (imune a carga) mas quem DESPACHA e o loop principal, que sob
// a carga gasta 65% do prazo. Se o loop atrasa, a fila enche. Este e o
// numero que fecha esse furo — nao "caiu ou nao caiu", mas QUANTO de
// folga sobrou. Max monotonico: grava o pico e trava, nunca atualiza
// pra baixo (regra 1 da instrumentacao).
static volatile uint32_t midi_hiwater = 0;

// Roda em ISR. Só enfileira — sem Log(), sem parser, sem params.
static void MidiRxCallback(uint8_t*            data,
                           size_t              size,
                           void*               /*context*/,
                           UartHandler::Result res)
{
    // ERRO DA UART: overrun, framing, ruido na linha. Antes isto
    // retornava CALADO, e o silencio escondia a unica pista de um
    // fluxo que para de chegar — "bytes 0" com a UART dizendo
    // ESCUTANDO e indistinguivel de "ninguem esta mandando".
    if(res != UartHandler::Result::OK)
    {
        midi_err++;
        return;
    }
    for(size_t i = 0; i < size; i++)
    {
        const uint32_t next = (m_head + 1) % kMidiRing;
        if(next == m_tail)
        {
            // fila cheia: descarta e CONTA. Perder byte calado num
            // protocolo com running status vira parâmetro errado sem
            // explicação — o contador é o que separa "não chegou" de
            // "chegou e eu deixei cair".
            midi_dropped++;
            return;
        }
        midi_ring[m_head] = data[i];
        m_head            = next;
    }
    // Amostra no fim do callback: dentro dele a fila so cresce, entao
    // aqui esta o pico desta rajada. Uma leitura por callback, nao por
    // byte — instrumentacao que custa caro vira o defeito que mede.
    const uint32_t fill = (m_head + kMidiRing - m_tail) % kMidiRing;
    if(fill > midi_hiwater)
        midi_hiwater = fill;
}

void SetMidiByteCallback(MidiByteCallback cb)
{
    midi_cb = cb;
    if(midi_started)
        return; // registrar de novo não reinicializa a porta

    UartHandler::Config c;
    c.periph          = UartHandler::Config::Peripheral::USART_1;
    c.mode            = UartHandler::Config::Mode::TX_RX; // TX é pro loopback
    c.baudrate        = 31250;                            // o baud do MIDI
    c.stopbits        = UartHandler::Config::StopBits::BITS_1;
    c.parity          = UartHandler::Config::Parity::NONE;
    c.wordlength      = UartHandler::Config::WordLength::BITS_8;
    c.pin_config.tx   = seed::D13; // PB6 — vai no RX do módulo
    c.pin_config.rx   = seed::D14; // PB7 — recebe o TX do módulo

    if(midi_uart.Init(c) != UartHandler::Result::OK)
    {
        Log("ERRO: UART MIDI nao inicializou (USART1 D13/D14)");
        return;
    }

    // ---- PRIORIDADE DA USART1 ABAIXO DA DO SEU PROPRIO DMA ----
    //
    // ⚠️ LIDO NA FONTE DA libDaisy, nao deduzido: USART1, DMA1_Stream5,
    //    DMA2_Stream4, SPI e USB sao TODOS postos em (0, 0).
    //      src/per/uart.cpp:897 · src/sys/dma.c:32-35 · src/per/spi.cpp:992
    //
    // Prioridade IGUAL no Cortex-M nao permite preempcao: uma ISR em 0 nao
    // pode ser interrompida por outra em 0. O `HAL_DMA_Abort_IT` que o HAL
    // dispara ao tratar um erro em modo DMA e ASSINCRONO e completa na ISR
    // do stream. Com a USART1 a reentrar sem parar, essa ISR nunca corre:
    // o abort nunca completa, a UART fica em estado de abort, o
    // `IsListening()` da falso para sempre e o rearme devolve BUSY.
    //
    // Nao e defeito que se veja hoje — o pull-up acima tirou a tempestade
    // que o dispara. E condicao LATENTE, e fica corrigida por leitura e
    // nao por sintoma: as fontes de erro que restam (ponte ligada a mandar
    // lixo, baud errado, cabo comprido) sao de BAIXA taxa, e com baixa
    // taxa o abort completa mesmo com a ordem plana — mas depender disso e
    // depender da sorte.
    //
    // Baixo SO a USART1, para 1. Os streams de DMA ficam em 0 e passam a
    // poder preemptar. O SAI do audio tambem fica em 0: continua acima de
    // tudo isto, que e como tem de ser.
    //
    // ⚠️ SEM GATILHO ARTIFICIAL: o `pino 4 2` da ponte ja nao exercita
    //    este caminho, porque o pull-up o matou. Isto e aceitavel porque o
    //    caso morreu — seria inaceitavel se estivesse so a "sobreviver".
    // ⚠️ ESTE `1` SO PREEMPTA DENTRO DO AGRUPAMENTO CERTO.
    //
    // `HAL_NVIC_SetPriority(irq, 1, 0)` poe preempcao 1 SE houver bits de
    // preempcao. Com `NVIC_PRIORITYGROUP_4` (4 de preempcao, 0 de sub) ha;
    // com um agrupamento que gaste os bits em subprioridade, o `1` vira
    // SUBprioridade e nao preempta coisa nenhuma — o conserto seria um
    // no-op silencioso, com ar de feito.
    //
    // Lido em 02/09: `HAL_Init` poe GROUP_4
    // (Drivers/.../stm32h7xx_hal.c:147) e ninguem na libDaisy nem aqui
    // volta a chamar `SetPriorityGrouping`. Mas leitura de hoje nao e
    // garantia de amanha, entao a placa CONFERE e grita — a prioridade so
    // vale dentro do agrupamento, e verificacao so absolve no seu dominio.
    nvic_grupo_ok = (HAL_NVIC_GetPriorityGrouping() == NVIC_PRIORITYGROUP_4);
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);

    // ---- PULL-UP NO RX, E E ISTO QUE FAZ O CASO DEIXAR DE EXISTIR ----
    //
    // ⚠️ A libDaisy inicia os pinos da UART com `GPIO_NOPULL`
    //    (src/per/uart.cpp). Com a ponte ESP32 sem alimentacao o pino
    //    dela fica em alta impedancia, e o nosso RX FICA A FLUTUAR.
    //
    // Entrada CMOS solta apanha ruido a MHz. A USART ve arestas ao ritmo
    // da propria amostragem, e FE/NE/ORE disparam a uma taxa que se
    // aproxima do tempo de servico da interrupcao: o CPU fica na ISR da
    // USART e o laco morre a fome. MEDIDO em 01/09 — a placa deixava de
    // responder em menos de 4 s e NAO voltava com a linha ja em repouso.
    //
    // Repouso de uma linha de UART e ALTO, por definicao. Se o
    // transmissor desaparece, alguem tem de segurar o alto — e o STM32
    // tem pull-up interno (~40 kOhm) a espera de ser pedido. Com ~100 pF
    // de cabo a subida e de ~4 us contra 32 us por bit: sobra de longe.
    //
    // Isto e a PRIMEIRA camada, e a defesa por software e a segunda:
    // quando um caso de falha depende de um estado fisico que a placa
    // pode impedir de existir, impede-se. Mesma licao do Schottky da
    // ponte, e do pull-up interno que o D28 ja usava.
    //
    // Efeito lateral conhecido e aceite: com a ESP32 desligada, o
    // pull-up injecta ~80 uA pelo diodo de proteccao do pino dela.
    // Inofensivo a esse nivel; com 10 kOhm externo seriam ~330 uA.
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIO_InitTypeDef g = {};
        g.Pin       = GPIO_PIN_7;          // PB7 = seed::D14 = USART1_RX
        g.Mode      = GPIO_MODE_AF_PP;
        g.Pull      = GPIO_PULLUP;
        g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOB, &g);
    }
    memset(midi_dma_buf, 0, sizeof(midi_dma_buf));
    if(midi_uart.DmaListenStart(
           midi_dma_buf, sizeof(midi_dma_buf), MidiRxCallback, nullptr)
       != UartHandler::Result::OK)
    {
        Log("ERRO: UART MIDI nao entrou em escuta DMA");
        return;
    }
    midi_started = true;
    Log("MIDI: USART1 31250 8N1 | TX=D13(pino14) RX=D14(pino15)");
}

// Reinicia a escuta DMA. Existe porque erro de UART (overrun,
// framing) pode ABORTAR a recepcao no stack da ST, e o listen nunca
// volta sozinho: o sintoma e exatamente "funcionou e parou pra
// sempre", com a porta reportando ESCUTANDO e zero byte chegando.
//
// Nao e remendo — e a contrapartida obrigatoria de um DMA circular
// que pode ser derrubado por ruido de linha. Sem rearme, um unico
// glitch no fio custa o canal ate o proximo reset.
bool MidiRestart()
{
    if(!midi_started)
        return false;
    midi_uart.DmaListenStop();
    memset(midi_dma_buf, 0, sizeof(midi_dma_buf));
    const bool ok = midi_uart.DmaListenStart(midi_dma_buf,
                                             sizeof(midi_dma_buf),
                                             MidiRxCallback, nullptr)
                    == UartHandler::Result::OK;
    if(ok) midi_restarts++;
    return ok;
}

uint32_t MidiErrors() { return midi_err; }
uint32_t MidiRestarts() { return midi_restarts; }
uint32_t MidiRearmeSeguidos() { return midi_rearme_seguidos; }
bool     NvicGrupoOk()        { return nvic_grupo_ok; }

// Estado CRU do periferico e do pino, para nao bissectar as cegas.
// Em 02/09 o RX parou depois de eu mexer em tres coisas no mesmo dia
// (pull-up no PB7, prioridade da USART1, escrita directa no TDR). Em vez
// de tirar uma de cada vez e gravar tres vezes, pergunta-se ao silicio.
void UartDiag(uint32_t* cr1, uint32_t* cr3, uint32_t* isr,
              uint32_t* moder, uint32_t* pupdr, uint32_t* afrl)
{
    if(cr1)   *cr1   = USART1->CR1;
    if(cr3)   *cr3   = USART1->CR3;
    if(isr)   *isr   = USART1->ISR;
    if(moder) *moder = (GPIOB->MODER >> (7 * 2)) & 0x3u;   // PB7
    if(pupdr) *pupdr = (GPIOB->PUPDR >> (7 * 2)) & 0x3u;
    if(afrl)  *afrl  = (GPIOB->AFR[0] >> (7 * 4)) & 0xFu;
}

// Reinicio duro. NAO e o DFU: e o mesmo que o botao de RESET faz.
// Chamado pelo vigia de laco, que vive no callback de audio — o unico
// contexto que sobrevive ao laco preso.
void ReiniciaAgora() { NVIC_SystemReset(); }

void PollMidi()
{
    // ⚠️ AQUI ESTEVE UM TETO DE ERROS DE LINHA, E ELE SAIU. (01/09)
    //
    // Contava erros por segundo e calava o receptor acima de 200/s. O
    // problema nao e a regra — e o SITIO: isto corre no `PollMidi`, que e
    // a fase 2 do laco, e a falha que ele devia tratar MATAVA O LACO NA
    // FASE 1. Ou seja, so corria enquanto o problema nao existisse.
    //
    // Defesa que precisa da vitima viva para agir nao e defesa. Fica a
    // regra escrita para quem a for reimplementar no sitio certo — a ISR.
    // VIGIA DA ESCUTA — COM TETO E RECUO, e o motivo esta MEDIDO.
    //
    // ⚠️ ATE 01/09 ISTO ERA `if(!IsListening()) MidiRestart();` sem mais
    // nada, e ERA O DEFEITO, nao o conserto.
    //
    // Com o RX preso em baixo — que e o que a ponte ESP32 faz quando
    // esta sem alimentacao: BREAK permanente — o `IsListening()` volta a
    // dar falso no instante seguinte ao rearme. O laco passava a nao
    // fazer mais nada senao parar e arrancar o DMA, a cada volta.
    //
    // MEDIDO nesse dia, com a falha injetada por software (o comando
    // `pino 4 0` na propria ponte poe o TX dela em baixo, que para a
    // Daisy e indistinguivel de estar desligada):
    //
    //   a placa deixa de responder a serial E DEIXA DE ENUMERAR NA USB,
    //   e NAO volta quando a linha se restaura — porque o laco nunca sai
    //   da rajada para dar por isso.
    //
    // E com a ponte SA e a linha em repouso ja se contavam 10 029
    // rearmes: a rajada existia mesmo sem falha nenhuma.
    //
    // A regra ja estava escrita na skill `instrumentacao` — "retentativa
    // automatica precisa de teto E de contador". So o contador estava
    // aqui. O contador tornou o problema VISIVEL e nao o impediu.
    //
    // Recuo: 2 ms enquanto pode ser um glitch de linha, 1 s quando e uma
    // linha morta. O canal volta sozinho quando a ponte voltar — ao
    // ritmo da sonda, e nao ao ritmo do laco.
    if(midi_started && !midi_uart.IsListening())
    {
        const uint32_t ag = NowMs();
        if((int32_t)(ag - midi_rearme_em) >= 0)
        {
            MidiRestart();
            if(midi_rearme_seguidos < 0xFFFFFFFFu) midi_rearme_seguidos++;
            midi_rearme_em = ag + (midi_rearme_seguidos < 4 ? 2u : 1000u);
        }
    }
    else
    {
        // ⚠️ O UNICO SINAL DE SUCESSO E `IsListening()` DAR VERDADE.
        //
        // Zerar o contador pelo retorno do `DmaListenStart` seria mentira:
        // sob break ele devolve OK e a escuta cai outra vez a seguir, e o
        // recuo nunca chegaria a acontecer.
        midi_rearme_seguidos = 0;
    }

    // Despacha em contexto normal, onde o parser pode mexer em params
    // e o Log() pode transmitir. Mesmo contrato do PollSerial().
    // INSTANTANEO DO `m_head`, e e o conserto de um LIVELOCK que travou
    // a placa em 28/08, depois de vinte minutos de trafego BLE.
    //
    // Era `while(m_tail != m_head)`, com o `m_head` escrito pela ISR: a
    // condicao de paragem PERSEGUIA UM ALVO QUE SE MOVE. Se os bytes
    // chegam mais depressa do que o callback os consome, o laco NUNCA
    // termina — e nao e bloqueio, e um laco que corre para sempre, por
    // isso nenhum vigia de 'chamada que nao retorna' o apanhava. O audio
    // continuou (e interrupcao), a tela congelou, a serial calou, e o
    // instrumento de fase piscou 6 para sempre.
    //
    // Basta o callback ser MODERADAMENTE lento: a 31250 baud entra um
    // byte a cada 320 us, e o `Log()` com o host desconectado gasta
    // muito mais que isso. Ai o laco fecha-se sobre si proprio.
    const uint32_t head = m_head;
    while(m_tail != head)
    {
        const uint8_t b = midi_ring[m_tail];
        m_tail          = (m_tail + 1) % kMidiRing;
        if(midi_cb)
            midi_cb(b);
    }
    // ATRASO CRONICO SE DECLARA. Sobra depois da drenagem significa que
    // ela nao acompanha a entrada — era isto que teria denunciado o
    // livelock antes de ele travar a placa.
    if(m_tail != m_head)
    {
        midi_atrasos++;
        const uint32_t sobra = (m_head + kMidiRing - m_tail) % kMidiRing;
        if(sobra > midi_sobra_pico) midi_sobra_pico = sobra;
    }
}

uint32_t MidiAtrasos()   { return midi_atrasos; }
uint32_t MidiSobraPico() { return midi_sobra_pico; }
uint32_t MidiRingTam()   { return (uint32_t)kMidiRing; }

// ---- TRANSMISSAO DE MIDI: FILA, PEDACOS CURTOS E RECUO ----
//
// O QUE ISTO CONSERTA, e o preco que cobrou: ate 31/08 esta funcao era
// uma linha —
//
//     midi_uart.BlockingTransmit(tmp, n, 10);
//
// — e com a PONTE ESP32 DESLIGADA a placa deixava de arrancar. O D14 e
// o RX da USART1 e, sem a ponte, flutua com o DMA a escuta permanente;
// a USART1 e a MESMA para TX e RX, e a linha solta prendia quem
// tentava transmitir. O `MidiEspelha` entrava aqui e nao voltava.
// Custou uma tarde: LED preso, sem serie, reproduzivel de instalacao
// limpa. A cura foi religar um cabo — zero linhas de codigo.
//
// ⚠️ E DEFEITO DE PECA DE GALERIA: um cabo solto nao pode matar um
// instrumento exposto ao publico e replicado em serie.
//
// ------------------------------------------------------------
// DUAS TENTATIVAS FALHADAS ANTES DESTA, e ficam escritas porque quem
// vier tentara as mesmas:
//
//   1. `DmaTransmit` com callback. Parecia a resposta — assincrono,
//      ISR larga o bastao. Mas o proprio comentario da libDaisy diz:
//
//          // queue a job (blocks until the queue position is free)
//
//      Ele BLOQUEIA quando o DMA esta ocupado, e o nosso RX vive em
//      `DmaListenStart`, circular e permanente. MEDIDO: 6 bytes sairam,
//      a fila encheu (255/256) e 54 bytes foram descartados — o
//      callback de fim nunca disparou e o bastao ficou preso.
//
//   2. `PollTx`. O nome promete sondagem; a implementacao e
//      `return BlockingTransmit(buff, size, 10);`. A mesma chamada.
//
// A libDaisy NAO OFERECE transmissao nao-bloqueante nesta UART com o
// RX em escuta. Entao o que se controla nao e SE espera, e sim QUANTO
// espera — e o que se garante e que ela DESISTE.
// ------------------------------------------------------------
//
// COMO FUNCIONA:
//   MidiSendBytes  enfileira e volta SEMPRE. Cheia = descarta o RESTO
//                  DA MENSAGEM e conta (meia mensagem de MIDI e pior
//                  que nenhuma: o receptor espera o byte que falta e
//                  come o proximo status).
//   MidiTxPump     do laco. Tira UMA mensagem (3 bytes no maximo) e
//                  tenta com 3 ms de tecto — folgado para os 0,96 ms
//                  que tres bytes levam a 31250 baud. So retira da
//                  fila SE CONSEGUIU: espreita antes de consumir.
//   RECUO          tres falhas seguidas => cala-se por 500 ms. Com a
//                  ponte fora isto custa 9 ms em cada 500 (1,8%), em
//                  vez de 3 ms em cada volta do laco.
//
// O PIOR CASO ESTA LIMITADO E E DECLARADO: 3 ms por passagem, e so
// enquanto o recuo nao arma. O laco continua a servir MIDI, botoes,
// encoders e painel com o cabo no chao.
static uint8_t  midi_tx_ring[256];

// ---- ESPELHO DA CONSOLA (ppc/006 fase 1) -----------------------------------
//
// Cada linha de log sai TAMBEM embrulhada em SysEx pela UART do MIDI, para a
// ponte a reemitir por BLE. A consola do USB nao muda: isto acrescenta um
// ouvinte, nao troca de cano.
//
// ⚠️ MELHOR-ESFORCO, E DE PROPOSITO. Tres regras, e as tres vem de cicatrizes:
//
//  1. NUNCA BLOQUEIA. O `Log` e chamado de todo o lado; um espelho que espere
//     mata o laco. Aqui so se copia para um anel — a 15fi mostrou o que custa
//     um periferico bloqueante no caminho do desenho (13 000 voltas/s -> 1).
//  2. O MIDI TEM PRIORIDADE. So se emite um quadro de consola quando o anel
//     de transmissao esta praticamente vazio. Diagnostico nao pode atrasar o
//     instrumento a tocar.
//  3. O QUE NAO COUBER E CONTADO. Anel cheio descarta e soma; perder calado e
//     a familia proibida.
static uint8_t   con_ring[2048];
static volatile uint32_t con_cab = 0, con_cau = 0;
static uint32_t  con_perdidos = 0, con_quadros = 0;

static void ConsolaEspelha(const uint8_t* b, size_t n)
{
    for(size_t i = 0; i < n; i++)
    {
        const uint32_t prox = (con_cab + 1) % sizeof(con_ring);
        if(prox == con_cau) { con_perdidos++; return; }
        con_ring[con_cab] = b[i];
        con_cab = prox;
    }
}
static volatile uint32_t midi_tx_head = 0;
static volatile uint32_t midi_tx_tail = 0;
static uint32_t midi_tx_descartes = 0;   // bytes que nao couberam
static uint32_t midi_tx_erros     = 0;   // tentativas falhadas
static uint32_t midi_tx_uart_err  = 0;   // CheckError() nao-zero
static uint32_t midi_tx_bytes     = 0;   // bytes entregues
static uint32_t midi_tx_pico      = 0;   // maior ocupacao da fila
static uint32_t midi_tx_recuos    = 0;   // quantas vezes se calou
static uint32_t midi_tx_ok_ms     = 0;   // ultima vez que a UART ACEITOU
static uint32_t midi_tx_seguidas  = 0;
static uint32_t midi_tx_retry_ms  = 0;

void MidiSendBytes(const uint8_t* bytes, size_t n)
{
    if(!midi_started || !bytes || n == 0)
        return;
    const uint32_t cap = (uint32_t)sizeof(midi_tx_ring);
    for(size_t i = 0; i < n; i++)
    {
        const uint32_t prox = (midi_tx_head + 1u) % cap;
        if(prox == midi_tx_tail)
        {
            midi_tx_descartes += (uint32_t)(n - i);
            return;
        }
        midi_tx_ring[midi_tx_head] = bytes[i];
        midi_tx_head               = prox;
    }
    const uint32_t usado = (midi_tx_head + cap - midi_tx_tail) % cap;
    if(usado > midi_tx_pico) midi_tx_pico = usado;
}

void MidiTxPump()
{
    if(!midi_started) return;
    // EM RECUO: nem tenta. Subtraccao em ticks antes de comparar, que
    // e como esta casa trata a volta do uint32.
    if(midi_tx_retry_ms && (int32_t)(NowMs() - midi_tx_retry_ms) < 0) return;

    // INSTANTANEO do `head`. Perseguir um alvo que se move travou a
    // placa em 28/08 no `PollMidi`; mesma cicatriz, mesma defesa.
    const uint32_t head = midi_tx_head;
    if(head == midi_tx_tail) return;

    // O erro da UART conta-se E limpa-se: um erro de enquadramento —
    // que e o que uma linha de RX solta produz — fica a envenenar as
    // chamadas seguintes se ninguem o ler.
    if(midi_uart.CheckError()) midi_tx_uart_err++;

    // ESPREITA ANTES DE CONSUMIR: so avanca a cauda se a transmissao
    // deu certo. Consumir primeiro perderia a mensagem no timeout.
    const uint32_t cap = (uint32_t)sizeof(midi_tx_ring);
    uint8_t        tmp[3];
    uint32_t       k = 0, t = midi_tx_tail;
    while(t != head && k < sizeof(tmp))
    {
        tmp[k++] = midi_tx_ring[t];
        t        = (t + 1u) % cap;
    }
    if(!k) return;

    // ⚠️ NUNCA UM BLOQUEANTE DA HAL NO LACO. Isto travou a placa, e o
    //    instrumento de fase apontou o sitio exacto.
    //
    // MEDIDO em 01/09, com a linha de RX A FLUTUAR (o comando `pino 4 2`
    // na ponte poe o TX dela em alta impedancia): o LED ficou em `2 e 6`
    // PARA SEMPRE — fase 2, inquilino 6, que e este pump. O OLED parou,
    // a serial calou, e nao voltou nem com a linha restaurada, porque o
    // laco ja estava la dentro.
    //
    // A causa e o tecto que nao e tecto. O `BlockingTransmit` da libDaisy
    // e `HAL_UART_Transmit(..., 3)`, e a HAL sai da espera por
    //     (HAL_GetTick() - Tickstart) > Timeout
    // — o que exige que o TICK ANDE. Com o periferico a levar uma
    // tempestade de interrupcoes da linha suja, essa condicao pode nunca
    // dar verdade, e o `while` da HAL fica preso. Um tecto medido pelo
    // relogio de quem pode estar a ser esfomeado nao e um tecto.
    //
    // Entao nao se pede a HAL que espere: PERGUNTA-SE se o periferico ja
    // aceita, e so entao se escreve. Sem espera nenhuma, o pior caso e
    // exactamente zero. Um byte por passagem sobra: o laco da ~20 000
    // voltas/s e a linha leva 3 125 bytes/s.
    //
    // Escrever no TDR daqui e legitimo — este e o unico ficheiro que
    // pode tocar no fornecedor — e o periferico e conhecido: o `Init`
    // acima pede `Peripheral::USART_1`.
    uint32_t enviados = 0;
    while(enviados < k && (USART1->ISR & USART_ISR_TXE_TXFNF))
    {
        USART1->TDR = tmp[enviados++];
    }
    if(enviados)
    {
        midi_tx_tail       = (midi_tx_tail + enviados) % cap;
        midi_tx_bytes     += enviados;
        midi_tx_seguidas   = 0;
        midi_tx_retry_ms   = 0;
        midi_tx_ok_ms      = NowMs();
        if(!midi_tx_ok_ms) midi_tx_ok_ms = 1u;
        return;
    }
    // ⚠️ "O PERIFERICO NAO ACEITOU" NAO E FALHA — E A LINHA CHEIA.
    //
    // MEDIDO a 22/09: `DESCARTES 1433` contra `1026 bytes entregues`, com
    // `falhas 1404` e `recuos 445`. A 31250 baud a UART engole ~3 125 bytes/s
    // e este laco passa aqui ~13 000 vezes/s: a esmagadora maioria das
    // passagens encontra a FIFO cheia, e isso e o funcionamento NORMAL de
    // uma linha saturada.
    //
    // A versao anterior contava cada uma como erro e, ao fim de TRES
    // SEGUIDAS, calava-se 500 ms. Tres seguidas acontecem em fraccoes de
    // milissegundo. 445 recuos = 222 segundos de silencio, e o anel a
    // transbordar por tras.
    //
    // ⚠️ E o defeito estava LATENTE ate hoje: com o painel partido o laco
    // andava a UMA volta por segundo, e tres falhas seguidas levavam tres
    // segundos a acontecer. Consertar o OLED levou o laco a 12 836 voltas/s
    // e destapou isto. **Acelerar um laco e mudar o regime de tudo o que
    // corre dentro dele** — e o que era raro passa a ser a regra.
    //
    // O recuo passa a medir-se em TEMPO SEM ESCOAR, nao em tentativas: so
    // ha problema se nada sair durante 100 ms, que a 3 125 bytes/s e mais
    // de 300 bytes de atraso — sinal de linha morta, nao de linha ocupada.
    {
        const uint32_t ag = NowMs();
        if(!midi_tx_ok_ms) midi_tx_ok_ms = ag;      // primeira volta
        if((uint32_t)(ag - midi_tx_ok_ms) >= 100u)
        {
            midi_tx_erros++;
            midi_tx_recuos++;
            midi_tx_ok_ms    = ag;
            midi_tx_retry_ms = ag + 50u;            // 50 ms, nao 500
            if(!midi_tx_retry_ms) midi_tx_retry_ms = 1u;
        }
    }
}

// A BOMBA DO ESPELHO — chamada do laco, nunca de outro sitio.
//
// Um quadro por passagem, e so quando o anel do MIDI esta quase vazio: a
// consola nunca pode fazer o instrumento esperar. 105 bytes crus e o tecto
// que a ponte declara (o `Message<128>` da biblioteca MIDI dela).
void ConsolaPump()
{
    if(con_cau == con_cab) return;
    if(midi_tx_head != midi_tx_tail) return;   // MIDI primeiro, sempre

    uint8_t cru[105];
    size_t  n = 0;
    while(con_cau != con_cab && n < sizeof(cru))
    {
        cru[n++] = con_ring[con_cau];
        con_cau = (con_cau + 1) % sizeof(con_ring);
    }

    // 8->7: sete bytes viajam em oito, com os bits altos no primeiro. Sem
    // isto um acento (>= 0x80) fecharia o SysEx a meio.
    uint8_t q[3 + 128];
    size_t  o = 0;
    q[o++] = 0xF0; q[o++] = 0x7D; q[o++] = 0x01;
    for(size_t i = 0; i < n; i += 7)
    {
        const size_t bloco = (n - i) < 7 ? (n - i) : 7;
        uint8_t altos = 0;
        for(size_t k = 0; k < bloco; k++)
            if(cru[i + k] & 0x80) altos |= (uint8_t)(1u << k);
        q[o++] = altos;
        for(size_t k = 0; k < bloco; k++) q[o++] = cru[i + k] & 0x7F;
    }
    q[o++] = 0xF7;
    MidiSendBytes(q, o);
    con_quadros++;
}

void ConsolaEstado(uint32_t* quadros, uint32_t* perdidos, uint32_t* na_fila)
{
    if(quadros)  *quadros  = con_quadros;
    if(perdidos) *perdidos = con_perdidos;
    if(na_fila)
        *na_fila = (con_cab + (uint32_t)sizeof(con_ring) - con_cau)
                   % (uint32_t)sizeof(con_ring);
}

void MidiTxEstado(uint32_t* descartes, uint32_t* erros, uint32_t* uart_err,
                  uint32_t* bytes, uint32_t* pico, uint32_t* na_fila,
                  uint32_t* recuos)
{
    const uint32_t cap = (uint32_t)sizeof(midi_tx_ring);
    if(descartes) *descartes = midi_tx_descartes;
    if(erros)     *erros     = midi_tx_erros;
    if(uart_err)  *uart_err  = midi_tx_uart_err;
    if(bytes)     *bytes     = midi_tx_bytes;
    if(pico)      *pico      = midi_tx_pico;
    if(na_fila)   *na_fila   = (midi_tx_head + cap - midi_tx_tail) % cap;
    if(recuos)    *recuos    = midi_tx_recuos;
}

uint32_t MidiTxRingTam() { return (uint32_t)sizeof(midi_tx_ring); }

void MidiInjectByte(uint8_t b)
{
    // Ver o aviso de corrida em hal.h: isto escreve no mesmo m_head
    // que a ISR. Gancho de bancada, com a porta silenciosa.
    const uint32_t next = (m_head + 1) % kMidiRing;
    if(next == m_tail)
    {
        midi_dropped++;
        return;
    }
    midi_ring[m_head] = b;
    m_head            = next;
}

bool     MidiRxActive() { return midi_started && midi_uart.IsListening(); }
uint32_t SerialTruncadas() { return serial_truncadas; }

// ---- ENCODERS ----
//
// PINOS, e o criterio da escolha esta escrito porque ele vai ser
// questionado quando o proximo periferico chegar:
//
//   enc A (cadeia A)   D0  D1   push D2
//   enc B (cadeia B)   D3  D4   push D9
//   enc G (global)     D18 D19  push D17
//
// O GLOBAL MUDOU DE PINO em 23/08, e a razao vale registrar: a solda
// foi para D17/D18/D19 e o firmware supunha D20/D21/D19. O sintoma era
// desconcertante — girar movia o pino do "push" (D19, que na verdade e
// uma FASE) e as duas "fases" nunca comutavam, porque nao havia nada
// ligado nelas.
//
// Perdi rodadas diagnosticando fiacao com base num mapa suposto. A
// medicao so podia confirmar ou negar o que eu ja imaginava: nenhum
// instrumento apontado para D20/D21 diria "o fio esta no D18".
//
// Dai o `varre`, que le TODOS os livres: supor o mapa e depois medir
// contra a suposicao so encontra o que ja se esperava.
//
// Os DIGITAIS PUROS sao o recurso escasso — D0..D4, D9, D11, D12 e so.
// Gastei seis neles e tres no bloco ADC, e escolhi o TOPO do bloco
// (D19/D20/D21) para deixar A0..A3 contiguos: quatro pots num chicote so
// e o tipo de folga que se agradece depois.
//
// D11/D12 FICAM LIVRES DE PROPOSITO: sao o I2C_1 (PB8/PB9), que o INA219
// vai querer. Conferido no fonte da libDaisy — o codec usa I2C_2, em
// pinos nao expostos, entao o I2C_1 esta inteiro.
//
// E NAO HA DEFAULT PARA COLIDIR: `Encoder::Init(a, b, click)` exige os
// tres pinos. O risco que o OLED teve — RST default em D30, justo um
// reservado — nao existe nesta classe.
static daisy::Encoder enc[kNumEncoders];
static bool           enc_pronto = false;

// A TABELA E A FONTE, e o `EncoderInit` LE dela — nao ao contrario.
//
// Ate 28/08 havia duas listas: o `Init` com os pinos escritos a mao e
// esta tabela para a sonda. O comentario ja dizia "UMA fonte: se o Init
// mudar e isto nao, o proximo diagnostico mede o pino errado e culpa a
// solda certa" — e a segunda lista continuou la, a espera. No dia em que
// o G precisou de ter as fases trocadas, trocar so uma delas teria feito
// exactamente o que o comentario previa.
//
// Havia ainda uma TERCEIRA copia, os nomes em texto no `encpino` do
// main.cpp. Vive agora aqui ao lado, e sai pelo `EncoderPinNome`.
//
// ⚠️ A LICAO DO ENCODER G FICA, MESMO COM ELE FORA (removido a 05/09).
//
// Ele tinha as fases trocadas de proposito, e quando isso se fez o dB
// reportou que a contagem PAROU DE VEZ. Fui procurar um mapa de pinos
// alternativo e estava errado: o `Encoder::Init` e o `Debounce` tratam `a` e
// `b` de forma SIMETRICA, logo trocar so pode INVERTER, nunca zerar. A
// simetria era prova, e eu procurei explicacao noutro lado. O que tinha
// acontecido: as duas fases abriram no chicote, e o `varre` mostrou-o.
//
// E a troca morava AQUI e nao num sinal invertido no laco, porque o sentido
// de um encoder E qual fase vem primeiro. Inverter o resultado depois
// esconderia a causa e faria a sonda mentir.
//
//                       fase A       fase B       push
static const daisy::Pin kEncPin[kNumEncoders][3] = {
    {seed::D0,  seed::D1,  seed::D2},
};

static const char* kEncPinNome[kNumEncoders][3] = {
    {"D0",  "D1",  "D2"},
};

const char* EncoderPinNome(int idx, int qual)
{
    if(idx < 0 || idx >= kNumEncoders || qual < 0 || qual > 2) return "??";
    return kEncPinNome[idx][qual];
}

bool EncoderInit()
{
    if(enc_pronto) return true;
    for(int e = 0; e < kNumEncoders; e++)
        enc[e].Init(kEncPin[e][0], kEncPin[e][1], kEncPin[e][2]);
    enc_pronto = true;
    return true;
}

bool EncoderNivelCru(int idx, int qual)
{
    if(idx < 0 || idx >= kNumEncoders || qual < 0 || qual > 2) return false;
    static daisy::GPIO g;
    g.Init(kEncPin[idx][qual], GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
    return !g.Read();   // true = BAIXO = fechado pra GND
}

uint16_t EncoderNiveisCrus()
{
    static daisy::GPIO g[kNumEncoders][3];
    static bool        feito = false;
    if(!feito)
    {
        for(int e = 0; e < kNumEncoders; e++)
            for(int q = 0; q < 3; q++)
                g[e][q].Init(kEncPin[e][q], GPIO::Mode::INPUT,
                             GPIO::Pull::PULLUP);
        feito = true;
    }
    uint16_t m = 0;
    for(int e = 0; e < kNumEncoders; e++)
        for(int q = 0; q < 3; q++)
            if(!g[e][q].Read()) m |= (uint16_t)(1u << (e * 3 + q));
    return m;
}

// Todos os pinos que NAO estao ocupados por periferico. Inclui os nove
// dos encoders — se o fio estiver noutro lugar, e aqui que aparece.
// D15 E D16 SAIRAM DAQUI EM 31/08, e a razao e um defeito que custou uma
// sessao inteira e uma solda do dB.
//
// O `VarreLivres` corre a CADA volta do laco e, na primeira, reconfigura
// toda esta lista como INPUT+PULLUP. Isso tira os pinos do modo ANALOGICO
// — e o ADC passa a amostrar uma entrada desligada. Os dois canais dos
// pots liam o MESMO ~0,4 V para sempre, e nada que se fizesse ao cobre
// mudava nada, porque o firmware roubava os pinos antes de o cobre ter
// chance.
//
// LISTA DE "LIVRES" E UMA AFIRMACAO SOBRE POSSE, e envelhece: ela estava
// certa quando foi escrita e passou a mentir no instante em que os pots
// ganharam dono. Ver a trava em `AdcInit` — a partir de agora um pino
// reclamado pelo ADC que ainda conste aqui GRITA no arranque.
static const int kLivres[] = {0,1,2,3,4,9,11,12,17,18,19,20,21};

// ---- O PONTO CEGO DO `varre`, e como se abre ----
//
// A lista acima cobre 15 dos 31 pinos. Ficam de fora os que TEM DONO:
// D5-D8 e D10 (OLED), D13/D14 (a UART do MIDI) e D22-D30 (botoes, LEDs,
// DPDT). Isso e correcto para o uso normal — sonda que dirige um pino de
// periferico parte o periferico.
//
// Mas cria um modo de falha inteiro que nenhum instrumento nosso via:
// FIO QUE VA PARAR A UM DESSES NAO APARECE. Em 28/08 cacei um encoder
// por meia tarde com quatro mapas de pinos em desacordo, e se um fio
// tivesse escorregado para o D20 do OLED eu teria lido "parado" em toda
// a gama e concluido "fio aberto" — a conclusao errada, com a sonda a
// dizer a verdade sobre os pinos que olhava.
//
// `VarreTudo` abre o ponto cego, com o preco declarado: reconfigura TUDO
// como entrada, e por isso MATA o OLED e o MIDI ate um reboot. Nao ha
// como nao matar — e o mesmo pino.
static const int kTodos[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
                             18,19,20,21,22,23,24,25,26,27,28,29,30};
static const daisy::Pin kTodosPin[] = {
    seed::D0,  seed::D1,  seed::D2,  seed::D3,  seed::D4,  seed::D5,
    seed::D6,  seed::D7,  seed::D8,  seed::D9,  seed::D10, seed::D11,
    seed::D12, seed::D13, seed::D14, seed::D15, seed::D16, seed::D17,
    seed::D18, seed::D19, seed::D20, seed::D21, seed::D22, seed::D23,
    seed::D24, seed::D25, seed::D26, seed::D27, seed::D28, seed::D29,
    seed::D30,
};
static constexpr int kNumTodos = (int)(sizeof(kTodos)/sizeof(kTodos[0]));
static bool varre_tudo = false;

void Reiniciar()
{
    // Reset do NUCLEO, nao do bootloader: volta ao app. O
    // `System::ResetToBootloader` e o outro gesto, e vive no `dfu`.
    NVIC_SystemReset();
}

int  PinosTodos(const int** lista) { *lista = kTodos; return kNumTodos; }
bool VarreTudoAtivo() { return varre_tudo; }

uint32_t VarreTudo()
{
    static daisy::GPIO g[kNumTodos];
    if(!varre_tudo)
    {
        for(int i = 0; i < kNumTodos; i++)
            g[i].Init(kTodosPin[i], GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
        varre_tudo = true;
    }
    uint32_t m = 0;
    for(int i = 0; i < kNumTodos; i++)
        if(!g[i].Read()) m |= (1u << i);
    return m;
}
static const daisy::Pin kLivresPin[] = {
    seed::D0, seed::D1, seed::D2, seed::D3, seed::D4, seed::D9,
    seed::D11, seed::D12, seed::D17,
    seed::D18, seed::D19, seed::D20, seed::D21};
static constexpr int kNumLivres = (int)(sizeof(kLivres)/sizeof(kLivres[0]));

int PinosVarridos(const int** lista) { *lista = kLivres; return kNumLivres; }

uint32_t VarreLivres()
{
    static daisy::GPIO g[kNumLivres];
    static bool feito = false;
    if(!feito)
    {
        // TRAVA CONTRA O ROUBO DE PINO, e mora AQUI porque e aqui que o
        // roubo acontece. Ate 31/08 o D15 e o D16 estavam nesta lista, e
        // esta linha arrancava-os do modo ANALOGICO na primeira volta do
        // laco: o ADC passava a amostrar uma entrada desligada e os dois
        // pots liam o mesmo valor para sempre. Nada que se fizesse ao
        // cobre mudava nada, porque o firmware chegava primeiro.
        //
        // Se alguem voltar a por um pino do ADC aqui, isto GRITA em vez de
        // o roubar em silencio.
        for(int i = 0; i < kNumLivres; i++)
            if(adc_pronto && (kLivres[i] == 15 || kLivres[i] == 16))
                Log("*** ERRO: D%d e do ADC e esta em kLivres — o varredor"
                    " acabou de o roubar; o pot vai ler lixo.", kLivres[i]);
        for(int i = 0; i < kNumLivres; i++)
            g[i].Init(kLivresPin[i], GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
        feito = true;
    }
    uint32_t m = 0;
    for(int i = 0; i < kNumLivres; i++)
        if(!g[i].Read()) m |= (1u << i);
    return m;
}

void EncoderPoll()
{
    if(!enc_pronto) return;
    // O proprio Debounce se limita a 1 kHz por dentro, entao chamar a
    // cada volta do laco e correto e nao desperdica.
    for(int i = 0; i < kNumEncoders; i++) enc[i].Debounce();
}

int  EncoderIncremento(int i)
{ return (enc_pronto && i >= 0 && i < kNumEncoders) ? enc[i].Increment() : 0; }
bool EncoderPushNivel(int i)
{ return enc_pronto && i >= 0 && i < kNumEncoders && enc[i].Pressed(); }
bool EncoderPushBorda(int i)
{ return enc_pronto && i >= 0 && i < kNumEncoders && enc[i].RisingEdge(); }

// ---- OLED ----
//
// FIACAO SOLDADA (23/08) contra os defaults da libDaisy:
//
//   CLK  D8  = PORTG11   <- e o default, nao mexe
//   DIN  D10 = PORTB5    <- e o default, nao mexe
//   CS   D7  = PORTG10   <- e o default, nao mexe
//   DC   D6  = PORTC12   <- default seria D9, SOBRESCRITO
//   RST  D5  = PORTD2    <- default seria D30, SOBRESCRITO
//
// O RST fora do default e escolha boa: o D30 esta reservado para a
// leitura dos DPDT do exciter, e usa-lo aqui criaria uma disputa que so
// apareceria no dia da solda do painel.
//
// MISO NAO E ATRIBUIDO, e isso vem do proprio default da libDaisy:
// `direction = TWO_LINES_TX_ONLY` com `miso = Pin(PORTX, 0)`, que e o
// "nenhum pino" da casa. Nao se gasta pino, e a pergunta se respondeu
// lendo o fonte em vez de escolhendo.
// O INVOLUCRO, nao o driver cru: `OledDisplay` traz `SetCursor` e
// `WriteString`, e o driver sozinho so tem `DrawPixel`. Escrever texto a
// mao a partir do FontDef seria reimplementar o que ja existe testado.
// ---- ESCRITA DO FRAMEBUFFER INTEIRO ----
//
// ⚠️ UM DRIVER SO, E E O QUE ESTA INICIALIZADO.
//
// A primeira versao criou um SEGUNDO driver ao lado (`oled_dma`), nunca
// lhe chamou `Init`, e mandou-o transmitir. Travou a placa duas vezes —
// e eu cheguei a escrever uma cicatriz a acusar o DMA da libDaisy. O
// defeito era meu: OBJECTO POR INICIALIZAR, que e a familia que esta
// casa tem escrita desde o `adc_pronto` ("periferico nao configurado
// devolve um numero, e numero parece dado").
//
// O `driver_` do `OledDisplay` e PRIVADO — nao se alcanca nem por
// subclasse. Mas ele e parametro de template, e isso da a saida: a
// subclasse REGISTA-SE a si propria no `Init`, e o HAL usa o ponteiro.
// Sem segunda instancia, sem tocar na biblioteca.
class OledFb : public daisy::SSD130x4WireSpi128x64Driver
{
  public:
    static OledFb* eu;
    void Init(Config c)
    {
        daisy::SSD130x4WireSpi128x64Driver::Init(c);
        eu = this;
    }
    // ⚠️ `memcpy`, e nao 8192 chamadas a `DrawPixel`.
    //
    // A conversao pixel a pixel custava 23 933 us por quadro — MEDIDO.
    // O SPI a PS_64 gasta ~4 ms, portanto vinte desses milissegundos
    // eram so o laco de conversao. A 5 Hz isso tirava 12% das voltas
    // (21 523 -> 18 831 por segundo); a 20 Hz tiraria metade.
    //
    // O `buffer_` do driver tem exactamente o formato do nosso
    // framebuffer — pagina de 128 bytes, bit `p` do byte e a linha
    // `pagina*8 + p`. Entao e uma copia e mais nada.
    //
    // A licao: o custo estava no CODIGO, nao no periferico. Fui desenhar
    // DMA para um problema que era um laco de 8192 iteracoes.
    void Copia(const uint8_t* fb) { memcpy(buffer_, fb, sizeof(buffer_)); }

    // O QUADRO EM PEDACOS DE 32 BYTES, e o numero foi MEDIDO (22/09).
    //
    // O `Update()` da libDaisy manda a pagina inteira — 128 bytes — num
    // `BlockingTransmit` so. Nesta placa um envio de 128 bytes devolve
    // `ERR` e gasta os 100 ms do prazo, em QUALQUER prescaler; ate 32
    // bytes passa sempre, e os bytes saem mesmo (o tempo cresce com o
    // tamanho). A sonda `oledspi` imprime a tabela.
    //
    // O sintoma sem isto: 8 paginas x 100 ms = 800 ms por quadro, o laco
    // principal a cair de 13 000 voltas/s para 1, e o vidro APAGADO —
    // com o `oled` a dizer "tela ACESA", porque o driver do SSD130x
    // DEITA FORA o `Result` de cada envio e o nosso `OledInit` nunca o viu.
    //
    // (!) NAO e o conserto da causa, e digo-o: por que e que um envio de
    // 64 bytes esgota o prazo nesta configuracao continua em aberto. Isto
    // e a volta por cima da fronteira medida, com a fronteira escrita.
    void EnviaQuadro()
    {
        for(uint8_t pag = 0; pag < 8; pag++)
        {
            transport_.SendCommand((uint8_t)(0xB0 + pag));
            transport_.SendCommand(0x00);
            transport_.SendCommand(0x10);
            for(int off = 0; off < 128; off += 32)
                transport_.SendData(&buffer_[128 * pag + off], 32);
        }
    }
};
OledFb* OledFb::eu = nullptr;

static daisy::OledDisplay<OledFb> oled;

static uint32_t oled_fb_quadros    = 0;   // quadros desenhados
static uint32_t oled_fb_descartes  = 0;   // nunca, no caminho bloqueante
static uint32_t oled_fb_us_pico    = 0;   // pior caso do QUADRO INTEIRO, em us


// CONTADORES DO PROPRIO DESENHO. Faltavam, e a falta custou uma rodada
// inteira de hipoteses: sem eles, "o bloco nao executou" e "executou e
// pintou preto" leem exatamente igual — uma tela apagada.
static uint32_t oled_updates = 0;
static uint32_t oled_chars   = 0;
// VELOCIDADE AJUSTAVEL EM TEMPO DE EXECUCAO — ganho de teste, nao
// configuracao. Cravar um prescaler custaria uma gravacao por tentativa;
// assim a varredura inteira cabe numa. Mesma familia do `ctos prazo` e
// do `dup on|off`.
static int oled_ps = 8;   // VERIFICADO no vidro: PS_4 e PS_8 desenham limpo.
static bool oled_pronto = false;

// MEMORIA DAS RETENCOES DO `fio`, e ela existe por uma medicao perdida.
//
// A 22/09 o dB mediu os cinco pinos no conector contando que as cinco
// retencoes estivessem de pe. Nao estavam: um reinicio pelo meio tinha
// devolvido CLK e DIN ao SPI, que em repouso os deixa BAIXOS. O padrao
// medido — 0 V nos dois do SPI, 3,3 V nos tres GPIO — foi lido como
// avaria e e o repouso normal do barramento.
//
// A retencao NAO sobrevive a um reinicio, e ninguem tinha como saber.
// Estes -1 dizem "ninguem retem"; o arranque zera-os por serem estaticos,
// que e exactamente a semantica certa.
static int oled_retido[5] = {-1, -1, -1, -1, -1};
static const char* const kFioNome[5] = {"rst", "dc", "cs", "clk", "din"};

// Reinicializa DE VERDADE quando o prescaler muda: o `if(oled_pronto)`
// existia para nao repetir o Init, e com velocidade ajustavel ele viraria
// exatamente o defeito que faz um ajuste "nao pegar" em silencio.
bool OledVelocidade(int ps)
{
    if(ps != 2 && ps != 4 && ps != 8 && ps != 16 && ps != 32
       && ps != 64 && ps != 128 && ps != 256)
        return false;
    oled_ps     = ps;
    oled_pronto = false;   // forca o Init a refazer o SPI
    return true;
}
int OledVelocidadeAtual() { return oled_ps; }

// O PAINEL TEM ALIMENTACAO? — e esta pergunta vale um modulo por sessao.
//
// MEDIDO a 22/09: com a bancada desligada e a Daisy viva no USB, o `TP5`
// (rede +5V) da ZERO. Os 5 V do painel vem do step-down, que corre do +15 V;
// o pad 39 do `U2` e VIN, uma ENTRADA. Ou seja: sempre que se trabalha so com
// USB, o modulo esta sem VCC — e o firmware, que inicializa o OLED no arranque
// e desenha sem parar, conduz-lhe cinco sinais a 3,3 V.
//
// Um modulo nessas condicoes e alimentado pelos DIODOS DE PROTECCAO das
// entradas dele. Mata devagar, ao longo de sessoes, e deixa a logica com ar de
// viva — pull-ups bons, niveis de repouso certos, vidro morto. Ja morreram
// DOIS: o primeiro por polaridade invertida na placa de distribuicao (23/08,
// causa conhecida e resolvida), o segundo sem causa apurada ate aqui.
//
// A sonda nao precisa de fio novo: o proprio modulo tem PULL-UP nas entradas,
// e ele so existe quando o modulo tem VCC. Entao pergunta-se com o pull-down
// INTERNO — quem vence e quem esta la.
//
//   painel alimentado -> os cinco leem ALTO (o pull-up do modulo ganha)
//   painel sem 5 V    -> os cinco leem baixo (so o pull-down interno manda)
//
// Exige 3 dos 5 e nao os 5: um pino com defeito nao deve impedir o painel bom.
bool OledPainelAlimentado()
{
    const daisy::Pin kPino[5] = {seed::D5, seed::D6, seed::D7,
                                 seed::D8, seed::D10};
    daisy::GPIO g[5];
    for(int i = 0; i < 5; i++)
        g[i].Init(kPino[i], GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);
    System::Delay(5);
    int altos = 0;
    for(int i = 0; i < 5; i++) altos += g[i].Read() ? 1 : 0;
    return altos >= 3;
}

bool OledInit(bool forcar)
{
    if(oled_pronto) return true;
    // (!) ANTES DE CONDUZIR UM UNICO PINO. Recusar e barato; o modulo nao e.
    if(!forcar && !OledPainelAlimentado())
    {
        Log("oled: RECUSO — o painel nao responde com alimentacao (os cinco"
            " pinos seguem o pull interno).");
        Log("  Conduzir sinal a 3,3 V num modulo sem VCC alimenta-o pelos"
            " diodos de proteccao e mata-o devagar. Ja custou modulos.");
        Log("  Ligue os 15 V (o +5V do painel vem do step-down) e repita."
            "  `oled forca` passa por cima, se souber o que faz.");
        return false;
    }
    daisy::OledDisplay<OledFb>::Config cfg;
    // Defaults ja dao MODO 0 (polarity LOW + phase ONE_EDGE), que e o que
    // o SSD1309 quer. Nao se toca no que ja esta certo.
    cfg.driver_config.transport_config.pin_config.dc    = seed::D6;
    cfg.driver_config.transport_config.pin_config.reset = seed::D5;
    // OS TRES DO SPI SAO OS DEFAULTS, E MESMO ASSIM SE DECLARAM AQUI.
    //
    // Nao muda comportamento nenhum — muda quem SABE. Enquanto vinham do
    // default da libDaisy, tres pinos estavam ocupados sem que o nosso
    // fonte dissesse: quem atribuisse D7 a outra coisa nao encontraria
    // conflito em lugar nenhum, e a colisao so apareceria na bancada.
    //
    // E o `make check-docs` os acusou por isso — "no PINOS.md e nao
    // aparece no fonte". Ocupacao que vive so no default de uma
    // biblioteca e o gemeo da reserva que vive so em documento (o D30):
    // nos dois casos o compilador nao sabe, e o custo aparece na solda.
    cfg.driver_config.transport_config.spi_config.pin_config.sclk = seed::D8;
    cfg.driver_config.transport_config.spi_config.pin_config.mosi = seed::D10;
    cfg.driver_config.transport_config.spi_config.pin_config.nss  = seed::D7;
    // PRESCALER ALTO NO PRIMEIRO VOO, de proposito: fio comprido e
    // solda de bancada perdoam menos em MHz alto, e este teste nao tem
    // requisito de taxa nenhum. Se acender, da para descer depois e
    // medir onde para de acender — que e informacao, nao chute.
    using BP = SpiHandle::Config::BaudPrescaler;
    BP ps = BP::PS_64;
    switch(oled_ps)
    {
        case 2:   ps = BP::PS_2;   break;
        case 4:   ps = BP::PS_4;   break;
        case 8:   ps = BP::PS_8;   break;
        case 16:  ps = BP::PS_16;  break;
        case 32:  ps = BP::PS_32;  break;
        case 64:  ps = BP::PS_64;  break;
        case 128: ps = BP::PS_128; break;
        case 256: ps = BP::PS_256; break;
        default:  ps = BP::PS_64;  break;
    }
    cfg.driver_config.transport_config.spi_config.baud_prescaler = ps;
    oled.Init(cfg);
    // O `Init` da libDaisy e VOID. Nao ha status, entao nao ha status a
    // reportar: o `true` daqui significa "configurei", e o chamador tem
    // que dizer isso com essas palavras.
    oled_pronto = true;
    return true;
}

// ---- O UNICO PONTO DE ENTRADA NOVO ----
//
// 1024 bytes, ordem de pagina do SSD1309: `fb[pagina*128 + coluna]`, e o
// bit `p` do byte e a linha `pagina*8 + p`.
//
// ⚠️ O BUFFER TEM DE SOBREVIVER A CHAMADA. O DMA le-o DEPOIS de esta
// funcao voltar — um buffer local do chamador seria lixo a caminho do
// painel. Quem chama guarda-o em estatico.
//
// Devolve `false` quando havia transferencia em curso e o quadro foi
// descartado. Descarte nao e erro: a 20 Hz nao devia acontecer, e se
// acontecer e sinal de que a taxa esta acima do que o SPI aguenta.
// ⚠️ O CAMINHO DE DMA TRAVA A PLACA. MEDIDO EM 01/09, DUAS VEZES.
//
// A ideia era boa e e do dB: por DMA o laco paga o DISPARO e a
// transferencia atravessa o periferico — os 20 KB/s existem no
// barramento SPI, nao no orcamento do laco. Mesma distincao do `Log()`
// com fila drenada por ISR.
//
// Mas na pratica, com o `SpiHandle::DmaTransmit` desta libDaisy, o laco
// morre. Reproduzido a 5 Hz — nao e questao de taxa. Assinatura: a USB
// continua a enumerar em 0,0 s e o CDC aceita ligacao, mas o laco nao
// responde. Ou seja: as ISRs vivem e o laco fica preso.
//
// ⚠️ E DISTINGUE-SE DA OUTRA TRAVAGEM DO DIA: a da ponte sem
// alimentacao dava ~59 s para abrir a porta. Esta abre instantaneamente.
// Dois modos de falha, duas assinaturas — e foi o tempo de abertura que
// os separou.
//
// NAO ESTA DIAGNOSTICADO ao nivel do token. Fica o caminho para quem
// voltar: o `SendDataDma` faz `SCB_CleanInvalidateDCache_by_Addr`, e o
// `DmaTransmit` da libDaisy tem o mesmo "queue a job (blocks until the
// queue position is free)" que ja travou a UART hoje. A bandeira
// `ocupado` devia impedir a fila — mas so impede a NOSSA, e o painel
// partilha o SPI1 com o que mais lá estiver.
//
// Ate isso estar medido, o caminho e o BLOQUEANTE, com a taxa a ceder
// se o numero mandar. O `oled.Update()` da libDaisy e conhecido e
// funciona ha semanas nesta placa.
bool OledDesenha(const uint8_t* fb)
{
    if(!oled_pronto || !fb) return false;
    const uint32_t t0 = NowTicks();
    if(!OledFb::eu) return false;
    OledFb::eu->Copia(fb);
    OledFb::eu->EnviaQuadro();   // 32 bytes de cada vez: ver EnviaQuadro
    const bool ok = true;
    const uint32_t us = (NowTicks() - t0) / TicksPerUs();
    if(us > oled_fb_us_pico) oled_fb_us_pico = us;
    if(ok) { oled_fb_quadros++; oled_updates++; }
    else   { oled_fb_descartes++; }
    return ok;
}

// ⚠️ O PICO E RETIDO, entao quem mede TEM de o zerar antes.
//
// Medido em 01/09: tres corridas seguidas a prescalers diferentes
// imprimiram todas `23288 us` — o pico da PRIMEIRA, com PS_64. As duas
// seguintes eram muito mais rapidas e o numero nao mexeu, porque um
// maximo so sobe. Numero velho com cara de novo, que e a familia que
// este dia inteiro pagou.
void OledFbZeraPico() { oled_fb_us_pico = 0; }

void OledDmaEstado(uint32_t* quadros, uint32_t* descartes, uint32_t* us_pico)
{
    if(quadros)   *quadros   = oled_fb_quadros;
    if(descartes) *descartes = oled_fb_descartes;
    if(us_pico)   *us_pico   = oled_fb_us_pico;
}

void OledPreenche(bool aceso)
{
    if(!oled_pronto) return;
    oled.Fill(aceso);
    if(OledFb::eu) OledFb::eu->EnviaQuadro();   // 32 em 32   // BLOQUEANTE, e aceitavel: roda no loop, por comando
    oled_updates++;
}

// TEXTO — e ele prova mais que o `Fill`.
//
// Tela toda acesa prova que os fios chegam e que o painel liga. TEXTO
// prova ENDERECAMENTO DE PIXEL: que CLK e DIN carregam dado coerente,
// na ordem certa, e que o DC separa comando de dado. Um painel com DC
// trocado acende inteiro e nao escreve nada legivel.
void OledTexto(const char* l1, const char* l2)
{
    if(!oled_pronto) return;
    oled.Fill(false);
    if(l1 && *l1)
    {
        // Centraliza pela largura da fonte, sem numero magico: 7 px por
        // caractere e o que o Font_7x10 declara, e a conta se refaz
        // sozinha se a fonte mudar.
        int   n = 0; while(l1[n]) n++;
        const int larg = n * 7;
        const int x    = larg < 128 ? (128 - larg) / 2 : 0;
        oled.SetCursor((uint16_t)x, 18);
        oled.WriteString(l1, Font_7x10, true);
        oled_chars += (uint32_t)n;
    }
    if(l2 && *l2)
    {
        int   n = 0; while(l2[n]) n++;
        const int larg = n * 6;
        const int x    = larg < 128 ? (128 - larg) / 2 : 0;
        oled.SetCursor((uint16_t)x, 38);
        oled.WriteString(l2, Font_6x8, true);
    }
    if(OledFb::eu) OledFb::eu->EnviaQuadro();   // 32 em 32
}

// TRES LINHAS, fonte pequena, alinhadas a esquerda.
//
// Formato de INSTRUMENTO e nao de titulo: nada de centralizar, porque
// numero que muda de largura ficaria dancando na tela e o olho perde a
// referencia. Coluna fixa le-se de relance.
void OledLinhas(const char* l1, const char* l2, const char* l3)
{
    if(!oled_pronto) return;
    oled.Fill(false);
    const char* v[3] = {l1, l2, l3};
    for(int i = 0; i < 3; i++)
    {
        if(!v[i] || !*v[i]) continue;
        oled.SetCursor(0, (uint16_t)(6 + i * 20));
        oled.WriteString(v[i], Font_6x8, true);
        for(const char* q = v[i]; *q; q++) oled_chars++;
    }
    if(OledFb::eu) OledFb::eu->EnviaQuadro();   // 32 em 32
    oled_updates++;
}

uint32_t OledUpdates() { return oled_updates; }
uint32_t OledChars()   { return oled_chars; }

// SONDA DE FIO — o gemeo, na direcao da SAIDA, do `pino` que inocentou
// a solda do freeze B.
//
// La o problema era "o botao fecha?" e a resposta veio de ler o nivel
// CRU antes de qualquer camada. Aqui o problema e "o sinal CHEGA?", e a
// resposta e a placa SEGURAR um nivel enquanto alguem mede no outro
// extremo. Continuidade com multimetro testa o fio; isto testa o
// caminho — pino, solda, fio e conector.
bool OledSondaPino(const char* nome, bool alto)
{
    static daisy::GPIO sonda;
    daisy::Pin p;
    if(!strcmp(nome, "rst"))      p = seed::D5;
    else if(!strcmp(nome, "dc"))  p = seed::D6;
    else if(!strcmp(nome, "cs"))  p = seed::D7;
    else if(!strcmp(nome, "clk")) p = seed::D8;
    else if(!strcmp(nome, "din")) p = seed::D10;
    else return false;
    sonda.Init(p, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
    sonda.Write(alto);
    for(int i = 0; i < 5; i++)
        if(!strcmp(nome, kFioNome[i])) oled_retido[i] = alto ? 1 : 0;
    // O painel deixa de estar inicializado: quem reconfigura um pino do
    // SPI como GPIO quebra o barramento, e afirmar o contrario seria
    // exatamente a string fixa que ja mentiu aqui hoje.
    oled_pronto = false;
    return true;
}

int OledCacaPonte(char* achados, int cap)
{
    static const char* kNome[5] = {"rst", "dc", "cs", "clk", "din"};
    const daisy::Pin   kPino[5] = {seed::D5, seed::D6, seed::D7,
                                   seed::D8, seed::D10};
    daisy::GPIO g[5];
    int n = 0;
    if(cap > 0) achados[0] = 0;

    // TESTE DIFERENCIAL, e a primeira versao estava errada.
    //
    // Ela dirigia um pino ALTO e acusava ponte em quem lesse alto. Deu
    // 20 de 20 — todos os pares — que e "numero redondo demais em itens
    // independentes", o sintoma que a casa ja catalogou.
    //
    // A causa: o modulo OLED tem PULL-UP nas entradas dele, e ele vence
    // o pull-down interno do MCU. Todo pino le alto sempre, e o teste
    // media os pull-ups do painel em vez de pontes.
    //
    // Ponte de verdade e o pino que ACOMPANHA o dirigido: alto quando
    // ele esta alto E baixo quando ele esta baixo. Um pino preso por
    // pull-up externo fica alto nas duas leituras e nao acusa nada —
    // que e o comportamento correto.
    //
    // E o gemeo importa: sem a segunda leitura, "acompanha" e
    // indistinguivel de "esta sempre alto".
    for(int dono = 0; dono < 5; dono++)
    {
        for(int i = 0; i < 5; i++)
            if(i == dono) g[i].Init(kPino[i], GPIO::Mode::OUTPUT,
                                    GPIO::Pull::NOPULL);
            else          g[i].Init(kPino[i], GPIO::Mode::INPUT,
                                    GPIO::Pull::NOPULL);
        bool com_alto[5], com_baixo[5];
        g[dono].Write(true);  System::Delay(3);
        for(int i = 0; i < 5; i++) com_alto[i] = g[i].Read();
        g[dono].Write(false); System::Delay(3);
        for(int i = 0; i < 5; i++) com_baixo[i] = g[i].Read();

        for(int i = 0; i < 5; i++)
        {
            if(i == dono) continue;
            // ACOMPANHOU: alto com alto e baixo com baixo.
            if(!(com_alto[i] && !com_baixo[i])) continue;
            n++;
            const int usado = (int)strlen(achados);
            if(cap - usado > 24)
                snprintf(achados + usado, (size_t)(cap - usado),
                         " %s=%s", kNome[dono], kNome[i]);
        }
    }
    for(int i = 0; i < 5; i++)
        g[i].Init(kPino[i], GPIO::Mode::INPUT, GPIO::Pull::NOPULL);
    for(int i = 0; i < 5; i++) oled_retido[i] = -1;
    oled_pronto = false;
    return n;
}

// LEITOR DE TRES ESTADOS — o gemeo, na direcao da ENTRADA, do `fio`.
//
// O `fio` SEGURA um nivel e manda medir la fora; e quando o multimetro
// diz 0 V ele nao sabe dizer QUAL dos dois 0 V e:
//
//   ha cobre a puxar o pino para a terra, ou
//   nao chega nivel nenhum ate la, e o 0 V e a ausencia.
//
// Isto le do lado de ca, com o pull INTERNO invertido duas vezes:
//
//   pull-up ALTO  e pull-down baixo  = SOLTO — so o pull interno manda
//   as duas BAIXO                    = preso a TERRA
//   as duas ALTO                     = alguem de fora puxa para CIMA
//
// (!) O terceiro caso e o SAUDAVEL quando a fita esta ligada: o modulo
// OLED tem pull-up nas entradas e vence o pull-down interno do MCU. Foi
// esse mesmo pull-up que fez a primeira versao do caca-ponte acusar 20
// pontes em 20 pares. Com a fita FORA o saudavel passa a ser SOLTO — e
// por isso a leitura so CONCLUI em par: uma com fita, outra sem. Uma
// leitura sozinha nao separa "o painel esta la" de "o painel nao esta".
void OledLeCinco(bool* com_pullup, bool* com_pulldown)
{
    const daisy::Pin kPino[5] = {seed::D5, seed::D6, seed::D7,
                                 seed::D8, seed::D10};
    daisy::GPIO g[5];
    for(int i = 0; i < 5; i++)
        g[i].Init(kPino[i], GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
    System::Delay(5);
    for(int i = 0; i < 5; i++) com_pullup[i] = g[i].Read();
    for(int i = 0; i < 5; i++)
        g[i].Init(kPino[i], GPIO::Mode::INPUT, GPIO::Pull::PULLDOWN);
    System::Delay(5);
    for(int i = 0; i < 5; i++) com_pulldown[i] = g[i].Read();
    // Deixa-os neutros: pull interno preso falsearia a proxima medicao
    // com multimetro, que e a ferramenta externa deste diagnostico.
    for(int i = 0; i < 5; i++)
        g[i].Init(kPino[i], GPIO::Mode::INPUT, GPIO::Pull::NOPULL);
    for(int i = 0; i < 5; i++) oled_retido[i] = -1;
    oled_pronto = false;
}

// QUEM MANDA NOS CINCO PINOS AGORA — a pergunta que valida (ou anula)
// qualquer medicao feita no conector com o multimetro.
//
// Devolve o nivel retido pelo `fio` (0 ou 1) ou -1 para "nao retido".
// Se o SPI estiver configurado, NENHUMA retencao esta de pe: um reinicio
// refaz o barramento e os pinos voltam ao repouso dele.
// SONDA DO SPI — le o codigo de erro que o nosso caminho deita fora.
//
// POR QUE EXISTE, e custou uma tarde (22/09). O painel nao acendia, e
// medimos a tarde inteira do lado do FIO: continuidade, pontes, curtos,
// alimentacao, ordem dos sinais no modulo. Tudo bom. O que denunciou foi
// o TEMPO do quadro: 800 ms, e IGUAL de PS_256 a PS_4 — 64x de relogio
// sem mexer um milissegundo. Transferencia a serio encolhe; timeout nao.
//
// 800 ms = 8 paginas x 100 ms, que e o prazo por omissao do
// `BlockingTransmit`. E por pagina vao 3 comandos de 1 byte e 1 bloco de
// 128: se os 32 envios esgotassem dava 3 200 ms. Logo os curtos passam e
// os longos nao.
//
// O `SSD130x` da libDaisy DEITA FORA o `Result` de cada envio, e o nosso
// `OledInit` devolve `void` disfarcado de sucesso. Esta sonda faz os
// envios ela mesma, com a MESMA configuracao, e relata os dois numeros
// que faltavam: o codigo e o tempo, por tamanho.
//
// (!) Toma o SPI para si. Pede `oled` a seguir.
int OledSondaSpi(int ps_pedido, int* tam, int* res, uint32_t* us, int cap)
{
    static const int kN[] = {1, 8, 16, 24, 31, 32, 33, 40, 64, 128};
    static uint8_t   buf[128];
    for(int i = 0; i < 128; i++) buf[i] = (uint8_t)(0x55u + (unsigned)i);

    SpiHandle         spi;
    SpiHandle::Config c;
    c.periph          = SpiHandle::Config::Peripheral::SPI_1;
    c.mode            = SpiHandle::Config::Mode::MASTER;
    c.direction       = SpiHandle::Config::Direction::TWO_LINES_TX_ONLY;
    c.datasize        = 8;
    c.clock_polarity  = SpiHandle::Config::ClockPolarity::LOW;
    c.clock_phase     = SpiHandle::Config::ClockPhase::ONE_EDGE;
    c.nss             = SpiHandle::Config::NSS::HARD_OUTPUT;
    // A FRONTEIRA E DO FIFO OU DO TEMPO? So se sabe movendo o relogio.
    // O FIFO do SPI1 no H7 tem 32 bytes: fronteira parada nos 32 em
    // qualquer velocidade = o periferico enche a fila e nao desloca nada.
    using BPS = SpiHandle::Config::BaudPrescaler;
    BPS bp = BPS::PS_8;
    switch(ps_pedido)
    {
        case 2:   bp = BPS::PS_2;   break;
        case 4:   bp = BPS::PS_4;   break;
        case 16:  bp = BPS::PS_16;  break;
        case 64:  bp = BPS::PS_64;  break;
        case 256: bp = BPS::PS_256; break;
        default:  bp = BPS::PS_8;   break;
    }
    c.baud_prescaler  = bp;
    c.pin_config.sclk = seed::D8;
    c.pin_config.miso = daisy::Pin(daisy::PORTX, 0);
    c.pin_config.mosi = seed::D10;
    c.pin_config.nss  = seed::D7;
    if(spi.Init(c) != SpiHandle::Result::OK)
    {
        oled_pronto = false;
        return -1;               // o proprio Init recusou: -1 diz isso
    }

    int n = 0;
    for(unsigned k = 0; k < sizeof(kN) / sizeof(kN[0]) && n < cap; k++)
    {
        const uint32_t t0 = System::GetUs();
        const SpiHandle::Result r
            = spi.BlockingTransmit(buf, (size_t)kN[k]);
        const uint32_t t1 = System::GetUs();
        tam[n] = kN[k];
        res[n] = (r == SpiHandle::Result::OK) ? 0 : 1;
        us[n]  = t1 - t0;
        n++;
    }
    oled_pronto = false;
    return n;
}

int OledRetido(int i)
{
    if(i < 0 || i > 4) return -1;
    return oled_pronto ? -1 : oled_retido[i];
}

// SOLTAR O BARRAMENTO — os cinco pinos como ENTRADA, sem conduzir nada.
//
// A guarda do `OledInit` so testa NO ARRANQUE do painel. Se a bancada for
// desligada DEPOIS — e e o que acontece numa sessao real — os pinos ficam como
// saidas do SPI, o firmware continua a desenhar, e a injeccao pelos diodos de
// proteccao recomeca sem ninguem dar por ela. Ver bts 15fm.
//
// O sitio natural para soltar e quando o painel ADORMECE: ele ja dorme aos 5 s
// sem encoder, e a dormir nao tem nada para mostrar. Assim a placa so conduz
// enquanto ha alguem a mexer, e ao acordar o `OledInit` volta a PERGUNTAR se
// ha alimentacao antes de conduzir o primeiro pino.
void OledSolta()
{
    const daisy::Pin kPino[5] = {seed::D5, seed::D6, seed::D7,
                                 seed::D8, seed::D10};
    daisy::GPIO g[5];
    for(int i = 0; i < 5; i++)
        g[i].Init(kPino[i], GPIO::Mode::INPUT, GPIO::Pull::NOPULL);
    oled_pronto = false;
}

bool OledConfigurado() { return oled_pronto; }

uint32_t MidiDropped() { return midi_dropped; }
uint32_t MidiHiWater() { return midi_hiwater; }
uint32_t MidiRingSize() { return (uint32_t)kMidiRing; }

uint32_t NowMs() { return System::GetNow(); }

void DelayMs(uint32_t ms) { System::Delay(ms); }

void* AllocBig(size_t bytes)
{
    bytes = (bytes + 7u) & ~size_t(7); // alinhamento 8
    if(sdram_used + bytes > sizeof(sdram_pool))
        return nullptr;
    void* p = &sdram_pool[sdram_used];
    sdram_used += bytes;
    return p;
}

size_t AllocBigUsed() { return sdram_used; }

float CpuLoadAvg() { return cpu_meter.GetAvgCpuLoad(); }
float CpuLoadMax() { return cpu_meter.GetMaxCpuLoad(); }
void RebootToDfu()
{
    // DAISY_INFINITE_TIMEOUT: entra no bootloader da Electrosmith (que
    // e quem enxerga a QSPI onde o app mora) e NAO desiste. Sem isso a
    // janela seria de ~2 s e o problema de sincronia voltaria.
    System::ResetToBootloader(System::BootloaderMode::DAISY_INFINITE_TIMEOUT);
}
const char* VersaoImpl() { return COTUCA_HAL_VERSION; }


uint32_t NowTicks() { return System::GetTick(); }
uint32_t TicksPerUs()
{
    // Calculado uma vez: HAL_RCC_GetPCLK1Freq() nao e barato pra chamar
    // de dentro do callback de audio a cada bloco.
    static uint32_t t = 0;
    if(t == 0)
    {
        const uint32_t hz = System::GetTickFreq();
        t = hz / 1000000u;
        if(t == 0) t = 1; // nunca dividir por zero no caminho de audio
    }
    return t;
}
void  CpuLoadReset() { cpu_meter.Reset(); }

// ---- CAIXA-PRETA ----
// Mora na secao .backup_sram que o linker da libDaisy ja reserva
// (STM32H750IB_flash.lds, 4 KB em 0x38800000). O que faz ela servir
// nao e o endereco: e o NOLOAD. Secao NOLOAD nao e carregada nem
// zerada pelo startup, entao o conteudo atravessa o reset.
//
// Poe-se um MAGICO junto pra distinguir "voo anterior" de "acabou de
// energizar e isso e lixo". Sem ele, a primeira leitura depois de
// plugar o cabo devolveria numeros aleatorios com cara de medicao —
// pior que nao ter dado nenhum.
namespace
{
constexpr uint32_t kBbMagic = 0xC0BEEF01u;
constexpr int      kBbSlots = 128;
struct Blackbox
{
    uint32_t magic;
    float    slot[kBbSlots];
};
// ENDERECO FIXO, no FIM da regiao — e o motivo custou DOIS voos:
//
// A primeira versao usava __attribute__((section(".backup_sram"))),
// e o linker colocou g_bb na BASE (0x38800000), empurrando o
// daisy::boot_info pra +0x24. So que o BOOTLOADER e um binario
// SEPARADO que linka o BootInfo DELE na base — e o escreve A CADA
// BOOT, pra decidir se fica em DFU ou pula pro app. Cada reset
// esmagava exatamente os bytes da nossa palavra magica. A caixa-
// preta lia "vazia" depois de um voo inteiro gravado — nao porque
// nao gravou, mas porque o bootloader passou por cima.
//
// (De quebra, o boot_info deslocado quebrava o ResetToBootloader()
// da libDaisy: app e bootloader olhando enderecos diferentes.)
//
// A regiao tem 4 KB. O bootloader usa a base; nos usamos os
// ultimos 256 bytes, por ponteiro fixo — sem linker no meio, sem
// briga possivel. 0xF00 e multiplo de 32 (linha de cache).
// MOVIDA de 0x38800F00 pra 0x38800800 quando os slots foram a 128:
// 128*4+4 = 516 bytes, e a partir de 0x38800F00 isso ESTOURARIA o fim da
// regiao (0x38801000). Estouro de backup SRAM nao daria erro — escreveria
// fora e o dado voltaria lixo, que e a mesma familia silenciosa das
// outras armadilhas deste instrumento. Em 0x38800800 sobram 1,5 KB de
// folga, e a base segue livre pro BootInfo do bootloader.
constexpr uintptr_t kBbAddr = 0x38800800u;
static Blackbox&    g_bb    = *reinterpret_cast<Blackbox*>(kBbAddr);

// A ARMADILHA QUE APAGOU O PRIMEIRO VOO: o M7 tem D-cache, e a
// libDaisy o liga. Escrever em g_bb escrevia NO CACHE — a SRAM
// fisica nunca via o dado, e o reset (que apaga o cache) levava a
// medicao junto. O `bb` respondia "vazia" depois de um voo inteiro
// gravado. Nao ha erro, nao ha aviso: cache write-back fazendo o
// trabalho dele.
//
// CleanDCache empurra as linhas sujas pra memoria fisica. Custa
// dois flushes de linha (48 bytes) por bloco — barato o bastante
// pro callback de audio.
inline void BbFlush()
{
    SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(&g_bb),
                            (int32_t)sizeof(g_bb));
}
} // namespace

void LogMute(bool on) { log_muted = on; }
bool LogMuted() { return log_muted; }

// Atravessa o mudo — e SO isso. Uma linha, em contexto de comando, onde
// a serial ja provou que recebe. Nao serve pra diagnostico em voo.
void LogAlways(const char* fmt, ...)
{
    char    tmp[kLogMax];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tmp, kLogMax - 2, fmt, args);
    va_end(args);
    log_force = true;
    Log("%s", tmp);
    log_force = false;
}

void BlackboxArm()
{
    // ZERA SEMPRE, nao so quando a magica e invalida. Antes, `disparos` e
    // `consumidos` do voo ANTERIOR sobreviviam e apareciam misturados com
    // os do voo novo — dado velho com cara de dado fresco, mesma familia
    // das outras armadilhas deste instrumento.
    bb_rec     = true;
    g_bb.magic = kBbMagic;
    for(int i = 0; i < kBbSlots; i++) g_bb.slot[i] = 0.f;
    BbFlush();
}

void BlackboxWrite(int slot, float value)
{
    if(slot < 0 || slot >= kBbSlots) return;
    g_bb.slot[slot] = value;
    BbFlush(); // sem isto a escrita morre no cache — ver nota acima
}

float BlackboxRead(int slot)
{
    if(slot < 0 || slot >= kBbSlots) return 0.f;
    return g_bb.slot[slot];
}

bool BlackboxValid() { return g_bb.magic == kBbMagic; }
int  BlackboxSlots() { return kBbSlots; }

} // namespace cotuca::hal
