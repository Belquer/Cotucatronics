// ============================================================
// Cotucatronics OS — HAL: Daisy Seed
// Único arquivo do projeto que pode incluir daisy_seed.h.
// ============================================================

// hal_daisy_seed v0.40.1
#include "hal.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include "hid/encoder.h"
#include "hid/disp/oled_display.h"
#include "util/oled_fonts.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

#define COTUCA_HAL_VERSION "0.40.1"

using namespace daisy;

namespace cotuca::hal
{

static DaisySeed hw;

static BoardInfo info = {
    .name         = "Daisy Seed",
    .num_knobs    = 5, // k1 level, k2 src_mix, k3-k5 macros
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

float ReadKnob(int /*index*/)
{
    // stub: pots físicos ainda não ligados.
    // Quando ligar: retornar hw.adc.GetFloat(index);
    return -1.f; // -1 sinaliza "sem hardware, use serial"
}

bool ReadWidiLed()
{
    return widi_led.Read();
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
    const bool     raw = !b.gpio.Read(); // pull-up: baixo = pressionado
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

    // DISJUNTOR. O teto de 50 ms por Log parecia generoso — e virou
    // arma: com o host fora, um fluxo de logs continuos custa 50 ms
    // CADA, e o loop principal vira zumbi gastando o dia inteiro em
    // retry. Foi o mecanismo por tras de "a serial morre com o vocoder
    // ligado": quem morria era o LOOP, afogado aqui dentro.
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
    // por ela — com o vocoder a 65% do prazo, o USB e oficialmente
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
    while(q_tail != q_head)
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
static volatile uint32_t midi_dropped = 0;
static volatile uint32_t midi_err      = 0; // erros de UART na ISR
static volatile uint32_t midi_restarts = 0; // rearmes da escuta DMA
// MARCA D'AGUA DA FILA. O furo conhecido do MIDI: os bytes chegam por
// DMA (imune a carga) mas quem DESPACHA e o loop principal, que sob
// vocoder gasta 65% do prazo. Se o loop atrasa, a fila enche. Este e o
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

void PollMidi()
{
    // VIGIA DA ESCUTA. Se o DMA caiu, rearma sozinho. Roda no loop,
    // custo de uma leitura de flag por volta.
    if(midi_started && !midi_uart.IsListening())
        MidiRestart();

    // Despacha em contexto normal, onde o parser pode mexer em params
    // e o Log() pode transmitir. Mesmo contrato do PollSerial().
    while(m_tail != m_head)
    {
        const uint8_t b = midi_ring[m_tail];
        m_tail          = (m_tail + 1) % kMidiRing;
        if(midi_cb)
            midi_cb(b);
    }
}

void MidiSendBytes(const uint8_t* bytes, size_t n)
{
    if(!midi_started || !bytes || n == 0)
        return;
    // BlockingTransmit quer ponteiro não-const; copia pra um buffer
    // local em vez de fazer cast, que é mentira barata.
    uint8_t tmp[16];
    if(n > sizeof(tmp))
        n = sizeof(tmp);
    memcpy(tmp, bytes, n);
    midi_uart.BlockingTransmit(tmp, n, 10);
}

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

bool EncoderInit()
{
    if(enc_pronto) return true;
    //                     fase A       fase B       push
    enc[0].Init(seed::D0,  seed::D1,  seed::D2);
    enc[1].Init(seed::D3,  seed::D4,  seed::D9);
    enc[2].Init(seed::D18, seed::D19, seed::D17);
    enc_pronto = true;
    return true;
}

// Os mesmos pinos do EncoderInit, numa tabela para a sonda poder le-los
// sem passar pela classe. UMA fonte: se o Init mudar e isto nao, o
// proximo diagnostico mede o pino errado e culpa a solda certa.
static const daisy::Pin kEncPin[kNumEncoders][3] = {
    {seed::D0,  seed::D1,  seed::D2},
    {seed::D3,  seed::D4,  seed::D9},
    {seed::D18, seed::D19, seed::D17},
};

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
static const int kLivres[] = {0,1,2,3,4,9,11,12,15,16,17,18,19,20,21};
static const daisy::Pin kLivresPin[] = {
    seed::D0, seed::D1, seed::D2, seed::D3, seed::D4, seed::D9,
    seed::D11, seed::D12, seed::D15, seed::D16, seed::D17,
    seed::D18, seed::D19, seed::D20, seed::D21};
static constexpr int kNumLivres = (int)(sizeof(kLivres)/sizeof(kLivres[0]));

int PinosVarridos(const int** lista) { *lista = kLivres; return kNumLivres; }

uint32_t VarreLivres()
{
    static daisy::GPIO g[kNumLivres];
    static bool feito = false;
    if(!feito)
    {
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
static daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver> oled;
// CONTADORES DO PROPRIO DESENHO. Faltavam, e a falta custou uma rodada
// inteira de hipoteses: sem eles, "o bloco nao executou" e "executou e
// pintou preto" leem exatamente igual — uma tela apagada.
static uint32_t oled_updates = 0;
static uint32_t oled_chars   = 0;
// VELOCIDADE AJUSTAVEL EM TEMPO DE EXECUCAO — ganho de teste, nao
// configuracao. Cravar um prescaler custaria uma gravacao por tentativa;
// assim a varredura inteira cabe numa. Mesma familia do `ctos prazo` e
// do `dup on|off`.
static int oled_ps = 64;
static bool oled_pronto = false;

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

bool OledInit()
{
    if(oled_pronto) return true;
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>::Config cfg;
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

void OledPreenche(bool aceso)
{
    if(!oled_pronto) return;
    oled.Fill(aceso);
    oled.Update();   // BLOQUEANTE, e aceitavel: roda no loop, por comando
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
    oled.Update();
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
    oled.Update();
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
    oled_pronto = false;
    return n;
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
