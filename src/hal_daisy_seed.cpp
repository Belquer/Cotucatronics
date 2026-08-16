// ============================================================
// Cotucatronics OS — HAL: Daisy Seed
// Único arquivo do projeto que pode incluir daisy_seed.h.
// ============================================================

// hal_daisy_seed v0.7.0
#include "hal.h"
#include "daisy_seed.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

#define COTUCA_HAL_VERSION "0.7.0"

using namespace daisy;

namespace cotuca::hal
{

static DaisySeed hw;

static BoardInfo info = {
    .name         = "Daisy Seed",
    .num_knobs    = 5, // 4 de caráter + SRC MIX
    .has_audio_in = true,
    .sample_rate  = 48000.f,
    // Valor de partida; o manifest pede o dele em RequestBlockSize()
    // antes do StartAudio. Ver Requirements::block_size.
    .block_size   = 48,
    .big_ram_bytes = 64u * 1024u * 1024u, // SDRAM
    .cpu_hz        = 0, // preenchido no Init com o clock real
};

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

constexpr size_t kLineMax    = 64;
constexpr size_t kQueueLines = 8;

// montagem da linha corrente: só a ISR encosta
static char   rx_buf[kLineMax];
static size_t rx_len = 0;

// fila SPSC: a ISR move q_head, o loop principal move q_tail.
// Cada índice tem um único escritor, então não precisa de lock.
static char              queue[kQueueLines][kLineMax];
static volatile uint32_t q_head = 0;
static volatile uint32_t q_tail = 0;

static void UsbRxCallback(uint8_t* buf, uint32_t* len)
{
    for(uint32_t i = 0; i < *len; i++)
    {
        char c = static_cast<char>(buf[i]);
        if(c == '\n' || c == '\r')
        {
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
                }
            }
            rx_len = 0;
        }
        else if(rx_len < kLineMax - 1)
            rx_buf[rx_len++] = c;
    }
}

// ---- contrato ----
bool Init()
{
    // >>> BENCH <<< boost de clock: 400 MHz -> 480 MHz (+20%).
    // Reverter pra hw.Init() junto com o block_size.
    hw.Init(true);
    info.cpu_hz = System::GetSysClkFreq();
    hw.SetAudioBlockSize(info.block_size);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    info.sample_rate = hw.AudioSampleRate();
    hw.StartLog(false);
    Log("hal_daisy_seed v" COTUCA_HAL_VERSION);
    hw.usb_handle.SetReceiveCallback(UsbRxCallback, UsbHandle::FS_INTERNAL);
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

bool ReadButton(int /*index*/) { return false; } // stub

void SetLed(bool on) { hw.SetLed(on); }

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
    if(n > (int)kLogMax - 3) // vsnprintf devolve o que CABERIA
        n = (int)kLogMax - 3;

    uint8_t* out = log_buf[log_slot];
    log_slot ^= 1;
    memcpy(out, tmp, (size_t)n);
    out[n++] = '\r';
    out[n++] = '\n';

    // Teto de ~50 ms: sem host lendo, desiste e devolve o controle em
    // vez de travar a placa esperando alguém que não vai chegar.
    for(int t = 0; t < 50; t++)
    {
        if(hw.usb_handle.TransmitInternal(out, (size_t)n)
           == UsbHandle::Result::OK)
            return;
        System::Delay(1);
    }
}

void SetSerialLineCallback(SerialLineCallback cb) { line_cb = cb; }

void PollSerial()
{
    // drena a fila em contexto normal, onde Log() pode transmitir
    while(q_tail != q_head)
    {
        if(line_cb)
            line_cb(queue[q_tail]);
        q_tail = (q_tail + 1) % kQueueLines;
    }
}

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
void  CpuLoadReset() { cpu_meter.Reset(); }

} // namespace cotuca::hal
