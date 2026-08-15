// ============================================================
// Cotucatronics OS — esqueleto v0.4.0 (board-agnostic)
// Este arquivo NÃO sabe qual placa é o cérebro, nem qual motor de
// DSP está rodando: fala com `Module`, não com o RNBO.
// Compile com: make BOARD=daisy_seed
// ============================================================

#define COTUCA_OS_VERSION "0.4.0"

#include "hal.h"
#include "params.h"
#include "manifest.h"
#include "module.h"
#include "module_rnbo.h"
#include <cstring>
#include <cstdlib>
#include <cmath>

static cotuca::ParamTable params;
static cotuca::Manifest   manifest;

// ---- módulo DSP ----
static cotuca::Module* dsp = nullptr;

// índice do parâmetro no módulo, resolvido por nome no boot.
// -1 = nome não existe no export → não empurra nada.
static int   mod_index[4];
static float mod_last[4]; // só empurra o que mudou

// Fonte que alimenta o módulo: crossfade das entradas + tom de teste.
// Estático porque o callback de áudio não aloca. 64 > block_size (48).
static constexpr size_t kMaxBlock = 64;
static float            src_buf[kMaxBlock];

// ---- oscilador de teste mínimo (sem DaisySP: portável) ----
static float osc_phase = 0.f;
static float osc_inc   = 0.f;

static void OscSetFreq(float hz, float sr) { osc_inc = hz / sr; }
static float OscProcess()
{
    float s = sinf(osc_phase * 2.f * 3.14159265f) * 0.5f;
    osc_phase += osc_inc;
    if(osc_phase >= 1.f) osc_phase -= 1.f;
    return s;
}

// ---- callback de áudio genérico ----
static void AudioCb(const float* const* in, float** out, size_t size)
{
    const float level = params.Real(manifest.p_level);
    const float mix   = params.Real(manifest.p_tone_mix);
    // SRC MIX: 0.0 = IN_L (piezo)  ·  1.0 = IN_R (jack de linha).
    // Crossfade linear, aplicado ANTES de qualquer processamento — é a
    // fonte do instrumento, não um efeito.
    const float src = params.Real(manifest.p_src_mix);
    OscSetFreq(params.Real(manifest.p_tone_freq),
               cotuca::hal::Info().sample_rate);

    const bool has_in = cotuca::hal::Info().has_audio_in;

    if(size > kMaxBlock)
        size = kMaxBlock; // não estoura o scratch; nunca acontece com blk=48

    // 1) FONTE — crossfade das duas entradas + tom de teste por cima.
    //    O tom entra aqui, antes do módulo, pra poder ser a fonte do
    //    eco no teste de bancada.
    for(size_t i = 0; i < size; i++)
    {
        const float dry  = has_in ? (in[0][i] * (1.f - src) + in[1][i] * src)
                                  : 0.f;
        const float tone = OscProcess();
        src_buf[i]       = dry * (1.f - mix) + tone * mix;
    }

    // 2) MÓDULO — escreve direto no out (1 entrada, 2 saídas).
    if(dsp)
    {
        const float* ins[1] = {src_buf};
        dsp->Process(ins, out, size);
    }
    else
    {
        for(size_t i = 0; i < size; i++)
            out[0][i] = out[1][i] = src_buf[i]; // sem módulo: passa reto
    }

    // 3) LEVEL — por último, master de saída.
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] *= level;
        out[1][i] *= level;
    }
}

// ---- painel falso via serial ----
static void HandleLine(const char* line)
{
    using namespace cotuca;
    if(line[0] == 'k' && line[1] >= '1' && line[1] <= '5')
    {
        int   idx = line[1] - '1';
        float v   = strtof(line + 2, nullptr);
        manifest.knob[idx].Apply(params, v);
        hal::Log("knob %d -> %.2f", idx + 1, v);
        return;
    }
    if(strncmp(line, "tone on", 7) == 0)
    { params.Set(manifest.p_tone_mix, 0.5f); hal::Log("tone ON"); return; }
    if(strncmp(line, "tone off", 8) == 0)
    { params.Set(manifest.p_tone_mix, 0.0f); hal::Log("tone OFF"); return; }
    if(strncmp(line, "board", 5) == 0)
    {
        const auto& b = hal::Info();
        hal::Log("%s | knobs:%d | in:%s | %.0fHz/blk%d",
                 b.name, b.num_knobs, b.has_audio_in ? "sim" : "nao",
                 b.sample_rate, (int)b.block_size);
        return;
    }
    if(strncmp(line, "mods", 4) == 0)
    {
        if(!dsp)
        { hal::Log("nenhum modulo carregado"); return; }
        hal::Log("modulo \"%s\": %d params", dsp->Name(), dsp->ParamCount());
        for(int i = 0; i < dsp->ParamCount(); i++)
            hal::Log("  [%d] %s", i, dsp->ParamName(i));
        for(int b = 0; b < manifest.n_bind; b++)
            hal::Log("  %-10s -> %s [%d]",
                     params.Name(manifest.bind[b].param_id),
                     manifest.bind[b].mod_name,
                     mod_index[b]);
        const auto& a = cotuca::RnboAllocStatsGet();
        hal::Log("  ram: %u SRAM (de %u) + %u SDRAM, %d blocos, %d falhas",
                 (unsigned)a.internal_bytes, (unsigned)a.internal_cap,
                 (unsigned)a.big_bytes, a.blocks, a.failures);
        return;
    }
    if(strncmp(line, "list", 4) == 0)
    {
        for(int i = 0; i < params.Count(); i++)
            hal::Log("%2d %-10s norm=%.2f real=%.2f",
                     i, params.Name(i), params.Get(i), params.Real(i));
        return;
    }
    hal::Log("? k1..k5 <0-1> | tone on/off | board | list | mods");
}

// confere manifest.req contra a placa. Falhou = LED rápido + log.
static bool CheckRequirements()
{
    using namespace cotuca;
    const auto& b = hal::Info();
    const auto& r = manifest.req;
    bool ok = true;
    if(r.needs_audio_in && !b.has_audio_in)
    { hal::Log("ERRO: instrumento exige audio in; %s nao tem", b.name); ok = false; }
    if(r.big_ram_bytes > b.big_ram_bytes)
    { hal::Log("ERRO: exige %u bytes de RAM grande; %s tem %u",
               (unsigned)r.big_ram_bytes, b.name, (unsigned)b.big_ram_bytes); ok = false; }
    if(r.min_knobs > b.num_knobs)
    { hal::Log("ERRO: exige %d knobs; %s tem %d", r.min_knobs, b.name, b.num_knobs); ok = false; }
    return ok;
}

int main(void)
{
    using namespace cotuca;
    hal::Init();
    params.Init();
    manifest.Init(params);

    if(!CheckRequirements())
    {
        // pisca rápido pra sempre: "cérebro errado pra este instrumento"
        bool led = false;
        while(1) { hal::SetLed(led); led = !led; hal::DelayMs(100); }
    }

    // ---- módulo DSP: carrega, inicializa, resolve params por nome ----
    dsp = CreateModule();
    if(dsp && !dsp->Init(hal::Info().sample_rate, hal::Info().block_size))
    {
        hal::Log("ERRO: modulo \"%s\" falhou no Init — seguindo sem DSP",
                 dsp->Name());
        dsp = nullptr; // passthrough em vez de áudio corrompido
    }

    for(int b = 0; b < manifest.n_bind; b++)
    {
        mod_index[b] = dsp ? dsp->FindParam(manifest.bind[b].mod_name) : -1;
        mod_last[b]  = -1.f; // força o primeiro push
        if(dsp && mod_index[b] < 0)
            hal::Log("AVISO: param \"%s\" nao existe no modulo",
                     manifest.bind[b].mod_name);
    }
    if(dsp)
        hal::Log("modulo \"%s\": %d params, %d ligados",
                 dsp->Name(), dsp->ParamCount(), manifest.n_bind);

    hal::SetSerialLineCallback(HandleLine);
    hal::StartAudio(AudioCb);
    hal::Log("Cotucatronics OS v" COTUCA_OS_VERSION " [%s] — pronto.",
             hal::Info().name);
    // Toda a SDRAM do projeto passa por AllocBig(). Este log é o extrato:
    // se um módulo alocar escondido, o número aqui não fecha.
    hal::Log("SDRAM via AllocBig: %u de %u bytes",
             (unsigned)hal::AllocBigUsed(),
             (unsigned)hal::Info().big_ram_bytes);

    // Tick de ~1 ms. Nada de bloqueio longo aqui: o serial responde na
    // hora e o LED vira heartbeat contado, não um sleep de meio segundo.
    constexpr uint32_t kLedToggleTicks = 500; // 500 ms aceso / 500 apagado

    bool     led  = false;
    uint32_t tick = 0;

    while(1)
    {
        // knobs físicos, quando existirem, vencem o serial
        for(int k = 0; k < hal::Info().num_knobs; k++)
        {
            float v = hal::ReadKnob(k);
            if(v >= 0.f) manifest.knob[k].Apply(params, v);
        }
        hal::PollSerial();

        // empurra pro módulo o que mudou. Fora do callback de áudio de
        // propósito: setParameterValue enfileira evento no RNBO.
        for(int b = 0; b < manifest.n_bind; b++)
        {
            if(mod_index[b] < 0)
                continue;
            const float v = params.Get(manifest.bind[b].param_id);
            if(v != mod_last[b])
            {
                mod_last[b] = v;
                dsp->SetParam(mod_index[b], v);
            }
        }

        if(++tick >= kLedToggleTicks)
        {
            tick = 0;
            led  = !led;
            hal::SetLed(led);
        }
        hal::DelayMs(1);
    }
}
