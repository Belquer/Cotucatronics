// ============================================================
// Cotucatronics OS — esqueleto v0.14.1 (board-agnostic)
// Este arquivo NÃO sabe qual placa é o cérebro, nem qual motor de
// DSP está rodando: fala com `Module`, não com o RNBO.
// Compile com: make BOARD=daisy_seed
// ============================================================

#define COTUCA_OS_VERSION "0.14.1"

#include "hal.h"
#include "params.h"
#include "manifest.h"
#include "module.h"
#include "dynamics.h"
#include <cstring>
#include <cstdlib>
#include <cmath>

static cotuca::ParamTable params;
static cotuca::Manifest   manifest;

// ---- módulos DSP, por SLOT ----
// Preenchidos do REGISTRO em module.h. O main não inclui header de
// motor nenhum: não sabe que existe RNBO. Slot vazio = passthrough.
static cotuca::Module* mod_slot[static_cast<int>(cotuca::ModuleSlot::kCount)]
    = {};

// índice do parâmetro dentro do módulo, resolvido por nome no boot.
// -1 = nome não existe naquele slot → não empurra nada.
static int   mod_index[8];
static float mod_last[8]; // só empurra o que mudou

// Fonte que alimenta o módulo: crossfade das entradas + tom de teste.
// Estático porque o callback de áudio não aloca. 64 > block_size (48).
// TEM que ser >= ao block_size da placa. Com 64 e bloco 128, o clamp
// abaixo processaria metade do bloco e deixaria a outra metade do
// buffer de saida com lixo do bloco anterior — sem erro, com ruido.
// >= ao maior block_size que o manifest pode pedir (48/128/256).
static constexpr size_t kMaxBlock = 256;
static float            src_buf[kMaxBlock];

// Scratch pro slot Fx2: Module::Process não é seguro in-place, o motor
// lê a entrada enquanto escreve a saída.
static float fx_buf[2][kMaxBlock];

// Portadora ausente = silêncio, não lixo. Usado quando a placa não
// tem entrada de áudio: um módulo de 2 inputs continua rodando, só
// que sem portadora.
static float zero_buf[kMaxBlock] = {};

// ============================================================
// REGRA: QUEM ARMA E QUEM CHECA TÊM QUE SER O MESMO CONTEXTO.
//
// Este projeto tem dois contextos que rodam concorrentes — o
// callback de áudio (interrupção) e o loop principal. Toda vez
// que uma proteção foi ARMADA num e a condição que ela protege
// aconteceu no outro, apareceu bug. Três vezes, todas com a
// lógica correta e só a ordem errada:
//
//  1. DESPACHO SERIAL. O callback de RX do USB (ISR) chamava o
//     painel falso direto, e hal::Log() transmite pelo mesmo
//     USB: o segundo Log esperava um TX que só completa quando a
//     ISR retorna. Deadlock — `list` matava a placa. Conserto: a
//     ISR só enfileira, PollSerial() despacha do loop.
//
//  2. LIGAÇÃO DE PARÂMETRO. bind[] era montado ANTES dos Add()
//     correspondentes no mesmo Init(). Os ids valiam 0, que é um
//     id VÁLIDO — o do `level` —, então o knob de volume dirigia
//     quatro parâmetros do freeze em silêncio. Conserto: todos
//     os Add() antes de todos os bind[].
//
//  3. JANELA DE GRAÇA (abaixo). Duas vezes seguidas: primeiro a
//     transição 0→1 era detectada no loop enquanto o bloco caro
//     rodava no callback; depois o medidor era zerado no loop
//     enquanto a checagem acontecia no callback. Nos dois casos
//     a proteção existia e chegava tarde.
//
// O padrão é sempre o mesmo e o conserto também: mova o ARMAR
// para junto do CHECAR, não o contrário.
// ============================================================

// ---- watchdog de módulo ----
//
// Um módulo que estoura o prazo do bloco não soa mal: ele mata o
// loop principal por inanição, e a placa fica surda no serial — sem
// log, sem cpu, sem jeito de diagnosticar a não ser regravando às
// cegas. Aconteceu com o freeze, cujo Process() não cabe em 1 ms.
//
// O watchdog lê a carga do bloco ANTERIOR de dentro do callback (o
// único contexto que ainda roda quando o loop morreu), e trava o
// Fx2 em bypass ao primeiro estouro. A CPU cai, o loop revive, e o
// evento vira log em vez de placa muda.
//
// Trava, não histerese: um módulo que estourou uma vez vai estourar
// de novo, e ficar oscilando entre ativo e bypass soaria pior que
// ficar fora.
// JANELA DE GRAÇA. Um módulo pode fazer trabalho de UMA VEZ no
// primeiro Process — o RNBO aloca buffers de FFT preguiçosamente,
// por exemplo. Condenar por isso confundiria custo de estreia com
// custo de regime, que é justamente a pergunta que interessa.
//
// Durante a graça o watchdog não trava, e ao fim dela o medidor é
// ZERADO: assim o pico reportado depois é só regime permanente.
static volatile bool     fx2_overrun  = false;
static bool              fx2_reported = false;
static volatile uint32_t fx2_grace    = 0;
static volatile bool     fx2_grace_done = false;
static bool              fx2_was_on   = false;
// A transicao 0->1 e detectada DENTRO do callback, nao no loop.
// Detectar no loop cria corrida: o callback dispara entre o parametro
// mudar e o loop chegar na linha que arma a graca, e o bloco caro da
// estreia roda com graca=0 — trava o watchdog antes da protecao
// existir. Foi exatamente o que aconteceu na primeira tentativa.
static volatile bool     fx2_prev_want = false;
// Pico da ESTREIA, capturado no fim da graca ANTES do reset. Sem
// isto o numero da estreia se perde justamente por causa da protecao
// que existe pra separa-lo do regime — a instrumentacao apagava o
// dado que ela mesma foi criada pra revelar.
static volatile float    fx2_startup_peak = 0.f;
static constexpr float   kOverrunTrip = 0.90f;
static constexpr uint32_t kGraceBlocks = 24; // ~64 ms em bloco 128

// ---- cadeia de dinâmica do shell ----
static cotuca::Gate     gate;
static cotuca::Governor gov_l; // só no ramo OUT_L (exciter)
static cotuca::Limiter  lim_l; // um por ramo: estados independentes
static cotuca::Limiter  lim_r;
static cotuca::Shifter  shift_l; // anti-feedback, só no ramo do exciter

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

    // 2) GATE — na entrada, pós-crossfade e pós-tom. Silencia o chão
    //    de ruído do piezo antes que o módulo o transforme em cauda.
    gate.SetThreshold(params.Real(manifest.p_gate_thresh));
    gate.SetRelease(params.Real(manifest.p_gate_release));
    for(size_t i = 0; i < size; i++)
        src_buf[i] = gate.Process(src_buf[i]);

    // ch2 da convenção de 2 entradas: IN_R CRU, pré-crossfade e
    // pré-gate. Ver o contrato em module.h — é a portadora externa.
    const float* carrier = has_in ? in[1] : zero_buf;

    // 3) MÓDULO DO INSTRUMENTO — escreve direto no out (2 out).
    cotuca::Module* inst = mod_slot[(int)cotuca::ModuleSlot::Instrument];
    if(inst)
    {
        const float* ins[2] = {src_buf, carrier};
        inst->Process(ins, out, size);
    }
    else
    {
        for(size_t i = 0; i < size; i++)
            out[0][i] = out[1][i] = src_buf[i]; // sem módulo: passa reto
    }

    // 4) FX2 — segundo efeito de instrumento (freeze). ANTES do
    //    level, como o contrato em module.h diz. A ordem importa:
    //    depois do level, o freeze capturaria áudio com o volume já
    //    aplicado, e mexer no level com o som congelado não mudaria
    //    nada — o master deixaria de ser master.
    //
    //    Slot vazio = passthrough silencioso, não erro.
    //    BYPASS: fx2_on nasce em 0. Modulo pesado que estoura o
    //    prazo do bloco nao soa mal — mata o loop principal por
    //    inanicao e a placa fica surda no serial. Com bypass, ligar
    //    e reversivel.
    cotuca::Module* fx2      = mod_slot[(int)cotuca::ModuleSlot::Fx2];
    const bool      fx2_want = fx2 && params.Real(manifest.p_fx2_on) >= 0.5f;
    if(fx2_want && !fx2_prev_want)
    {
        // acabou de ligar: abre a graca AQUI, antes de qualquer bloco
        fx2_grace   = kGraceBlocks;
        fx2_overrun = false;
    }
    fx2_prev_want = fx2_want;

    if(fx2_want && fx2_grace > 0)
    {
        if(--fx2_grace == 0)
        {
            // ZERA AQUI, no mesmo contexto de quem checa. Deixar o
            // reset pro loop principal cria corrida: o medidor ainda
            // carrega o pico da estreia quando o bloco seguinte faz a
            // checagem, e o watchdog trava por um custo que a graca
            // existia justamente pra perdoar. Terceira vez nesta
            // funcao que contexto errado gera bug — armar protecao e
            // checar condicao TEM que ser o mesmo contexto.
            fx2_startup_peak = cotuca::hal::CpuLoadMax(); // guarda antes
            cotuca::hal::CpuLoadReset();
            fx2_grace_done = true; // o loop so loga
        }
    }
    else if(fx2_want && !fx2_overrun
            && cotuca::hal::CpuLoadMax() > kOverrunTrip)
        fx2_overrun = true; // trava; quem loga é o loop principal
    if(fx2_want && !fx2_overrun)
    {
        // DOIS canais de entrada. O freeze e 2-in/2-out, ao contrario
        // do delay que e 1-in. Passar um array de 1 aqui faria o
        // modulo ler in[1] fora dos limites — sem crash imediato, com
        // lixo no audio. O slot generico entrega o par estereo.
        // ch1 = sinal do ponto da cadeia; ch2 = portadora externa.
        // ANTES daqui ia {out[0], out[1]} — o par estéreo — o que
        // dava ao módulo o próprio sinal duas vezes em vez de uma
        // referência externa. Cross-synthesis precisa de duas fontes
        // DIFERENTES.
        const float* ins[2]  = {out[0], carrier};
        float*       outs[2] = {fx_buf[0], fx_buf[1]};
        // scratch limpo: export de 1 saída não escreve out[1], e sem
        // isto o canal direito ficaria com o bloco anterior.
        for(size_t i = 0; i < size; i++)
            fx_buf[0][i] = fx_buf[1][i] = 0.f;
        fx2->Process(ins, outs, size);
        for(size_t i = 0; i < size; i++)
        {
            out[0][i] = fx_buf[0][i];
            out[1][i] = fx_buf[1][i];
        }
    }

    // 5) LEVEL — master de saída, ANTES da dinâmica de saída: é
    //    justamente o level exagerado que o governor e o limiter
    //    precisam segurar.
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] *= level;
        out[1][i] *= level;
    }

    // 6) SPLIT — os ramos deixam de ser iguais aqui.
    //    OUT_L vai pro exciter no corpo da escultura e é o ramo que
    //    realimenta o piezo: leva o governor. OUT_R é jack de linha,
    //    não realimenta, e não leva.
    // SHIFTER — anti-feedback, entre o split e o governor, SÓ no ramo
    // do exciter. O jack de linha sai SEM shift por design: quem
    // grava ou manda pra PA quer o sinal do instrumento, não o sinal
    // protegido contra um laço acústico que não existe do lado de lá.
    //
    // Antes do governor de propósito: o governor deve reagir ao sinal
    // que de fato vai pro exciter, já deslocado.
    shift_l.SetShift(params.Real(manifest.p_shift_hz));
    for(size_t i = 0; i < size; i++)
        out[0][i] = shift_l.Process(out[0][i]);

    gov_l.SetCeiling(params.Real(manifest.p_gov_ceiling));
    gov_l.SetRelease(params.Real(manifest.p_gov_release));
    for(size_t i = 0; i < size; i++)
        out[0][i] = gov_l.Process(out[0][i]);

    // 6b) BENCH BOOST — FERRAMENTA DE BANCADA, só no ramo do exciter,
    //     imediatamente antes do limiter. Ver o bloco de aviso em
    //     manifest.h: nunca vai a knob nem a cena, e nasce em 1.0.
    const float boost = params.Real(manifest.p_bench_boost);
    if(boost != 1.0f)
        for(size_t i = 0; i < size; i++)
            out[0][i] *= boost;

    // 7) LIMITER — último bloco de processamento em CADA ramo.
    //    Um limiter por ramo, não um compartilhado: um pico no
    //    exciter não pode abaixar o jack de linha.
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = lim_l.Process(out[0][i]);
        out[1][i] = lim_r.Process(out[1][i]);
    }

    // 8) ROTEAMENTO — só liga/desliga o ramo. Depois do limiter
    //    porque mutar só pode reduzir: nada aqui consegue burlar o
    //    teto.
    const float r_exc = params.Real(manifest.p_route_exciter) >= 0.5f ? 1.f : 0.f;
    const float r_jck = params.Real(manifest.p_route_jack) >= 0.5f ? 1.f : 0.f;
    if(r_exc < 0.5f || r_jck < 0.5f)
    {
        for(size_t i = 0; i < size; i++)
        {
            out[0][i] *= r_exc;
            out[1][i] *= r_jck;
        }
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
        hal::Log("  cpu %lu MHz | prazo do bloco %.0f us",
                 (unsigned long)(b.cpu_hz / 1000000u),
                 1e6f * (float)b.block_size / b.sample_rate);
        return;
    }
    if(strncmp(line, "set ", 4) == 0)
    {
        // set <nome> <valor REAL> — alcança QUALQUER parâmetro pelo
        // nome, inclusive os sem knob (tone_freq e tone_mix são
        // serial-only por design: o tom é bancada, não performance).
        //
        // O VALOR É REAL, na unidade do parâmetro: Hz, ms, ×.
        // Antes era normalizado, e isso invertia a intuição em faixa
        // que não começa em zero — `set bench_boost 1.0` parecia "sem
        // boost" e entregava 8×, o máximo, num parâmetro que existe
        // justamente pra sobrecarregar a saída do exciter.
        // Knob e macro seguem normalizados: posição de curso é
        // normalizada por natureza. Quem digita, digita em unidade.
        const char* p = line + 4;
        while(*p == ' ') p++;
        const char* sp = strchr(p, ' ');
        if(!sp || sp == p)
        { hal::Log("uso: set <nome> <valor real> — veja list"); return; }

        char         name[24];
        const size_t n = (size_t)(sp - p) < sizeof(name) - 1
                             ? (size_t)(sp - p)
                             : sizeof(name) - 1;
        memcpy(name, p, n);
        name[n] = '\0';

        const int id = params.Find(name);
        if(id < 0)
        { hal::Log("? param \"%s\" nao existe (use list)", name); return; }

        const float v = strtof(sp + 1, nullptr);
        if(!params.SetReal(id, v))
        {
            hal::Log("fora da faixa: %s aceita [%.3f .. %.3f]",
                     params.Name(id), params.Min(id), params.Max(id));
            return;
        }
        hal::Log("%s -> real=%.3f (norm=%.3f, curve %.1f)",
                 params.Name(id), params.Real(id), params.Get(id),
                 params.Curve(id));
        return;
    }
    if(strncmp(line, "dyn", 3) == 0)
    {
        hal::Log("gate  thr=%.4f rel=%.0fms gain=%.2f env=%.4f",
                 params.Real(manifest.p_gate_thresh),
                 params.Real(manifest.p_gate_release),
                 gate.Gain(), gate.Env());
        hal::Log("gov   ceil=%.2f rel=%.0fms gain=%.2f env=%.3f",
                 params.Real(manifest.p_gov_ceiling),
                 params.Real(manifest.p_gov_release),
                 gov_l.Gain(), gov_l.Env());
        hal::Log("lim   ceil=%.2f gainL=%.2f gainR=%.2f",
                 cotuca::Limiter::Ceiling(), lim_l.Gain(), lim_r.Gain());
        {
            const float b = 1e6f * (float)hal::Info().block_size
                            / hal::Info().sample_rate;
            hal::Log("estreia %.0f us  (regime abaixo)",
                     fx2_startup_peak * b);
        }
        hal::Log("fx2   %s", fx2_overrun
                     ? "BYPASS FORCADO — estourou o prazo do bloco"
                     : (params.Real(manifest.p_fx2_on) >= 0.5f
                            ? "ATIVO" : "bypass (set fx2_on 1)"));
        hal::Log("shift %.1f Hz (SSB, so no ramo exciter)", shift_l.Shift());
        hal::Log("boost %.2fx  (BANCADA — nasce sempre em 1.00)",
                 params.Real(manifest.p_bench_boost));
        hal::Log("route exciter=%d jack=%d",
                 params.Real(manifest.p_route_exciter) >= 0.5f ? 1 : 0,
                 params.Real(manifest.p_route_jack) >= 0.5f ? 1 : 0);
        return;
    }
    if(strncmp(line, "cpu", 3) == 0)
    {
        // O prazo do bloco em microssegundos. Reportar % sozinho
        // engana quando se compara tamanhos de bloco diferentes: 50%
        // de 1 ms e 50% de 2,67 ms sao cargas MUITO diferentes em
        // trabalho absoluto. O µs e invariante.
        const float budget_us = 1e6f * (float)hal::Info().block_size
                                / hal::Info().sample_rate;
        hal::Log("cpu: %.1f%% media, %.1f%% pico",
                 hal::CpuLoadAvg() * 100.f, hal::CpuLoadMax() * 100.f);
        hal::Log("     %.0f us de pico em %.0f us de prazo (bloco %d @ %.0f Hz)",
                 hal::CpuLoadMax() * budget_us, budget_us,
                 (int)hal::Info().block_size, hal::Info().sample_rate);
        return;
    }
    if(strncmp(line, "mods", 4) == 0)
    {
        for(int s = 0; s < (int)ModuleSlot::kCount; s++)
        {
            const ModuleSlot slot = (ModuleSlot)s;
            Module*          m    = mod_slot[s];
            if(!m)
            { hal::Log("slot %-10s vazio", SlotName(slot)); continue; }
            hal::Log("slot %-10s \"%s\": %d params, %d in%s",
                     SlotName(slot), m->Name(), m->ParamCount(),
                     m->InputCount(),
                     m->InputCount() >= 2 ? " (ch2 = portadora IN_R)" : "");
            for(int i = 0; i < m->ParamCount(); i++)
                hal::Log("    [%d] %s", i, m->ParamName(i));
            const Module::MemReport mem = m->Memory();
            if(mem.blocks)
                hal::Log("    ram: %u SRAM (de %u) + %u SDRAM, %d bl, %d falhas",
                         (unsigned)mem.internal_bytes, (unsigned)mem.internal_cap,
                         (unsigned)mem.big_bytes, mem.blocks, mem.failures);
        }
        for(int b = 0; b < manifest.n_bind; b++)
            hal::Log("  %-12s -> %s.%s [%d]",
                     params.Name(manifest.bind[b].param_id),
                     SlotName(manifest.bind[b].slot),
                     manifest.bind[b].mod_name,
                     mod_index[b]);
        return;
    }
    if(strncmp(line, "list", 4) == 0)
    {
        for(int i = 0; i < params.Count(); i++)
            hal::Log("%2d %-10s norm=%.2f real=%.2f",
                     i, params.Name(i), params.Get(i), params.Real(i));
        return;
    }
    hal::Log("? k1..k5 <0-1> | set <nome> <real> | tone on/off | board | list | mods | dyn | cpu");
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

    // ---- módulos: pega do REGISTRO, slot a slot ----
    // O main não conhece motor nenhum. Quem se registrou, apareceu.
    for(int s = 0; s < (int)ModuleSlot::kCount; s++)
    {
        const ModuleSlot slot = (ModuleSlot)s;
        Module*          m    = GetModule(slot);
        if(!m)
        {
            hal::Log("slot %-10s vazio (passthrough)", SlotName(slot));
            continue;
        }
        if(!m->Init(hal::Info().sample_rate, hal::Info().block_size))
        {
            // slot fica nullptr = passthrough. Áudio limpo com um
            // módulo faltando é melhor que áudio corrompido com um
            // módulo meio inicializado.
            hal::Log("ERRO: \"%s\" no slot %s falhou no Init — slot vazio",
                     m->Name(), SlotName(slot));
            continue;
        }
        mod_slot[s] = m;
        hal::Log("slot %-10s \"%s\": %d params",
                 SlotName(slot), m->Name(), m->ParamCount());
    }

    for(int b = 0; b < manifest.n_bind; b++)
    {
        Module* m    = mod_slot[(int)manifest.bind[b].slot];
        mod_index[b] = m ? m->FindParam(manifest.bind[b].mod_name) : -1;
        mod_last[b]  = -1.f; // força o primeiro push
        if(m && mod_index[b] < 0)
            hal::Log("AVISO: param \"%s\" nao existe no slot %s",
                     manifest.bind[b].mod_name, SlotName(manifest.bind[b].slot));
    }

    // Salvaguarda (c): bench_boost TEM que nascer em 1.0. Hoje não
    // existe persistência de cena, então isto nunca dispara — é
    // tripwire pro dia em que existir. Um boost de 8x restaurado em
    // silêncio seria a escultura gritando no palco sem explicação.
    {
        const float b = params.Real(manifest.p_bench_boost);
        if(b != 1.0f)
            hal::Log("AVISO: bench_boost nasceu em %.2f e nao 1.00 — "
                     "ferramenta de bancada NAO deve persistir", b);
    }

    gate.Init(hal::Info().sample_rate);
    gov_l.Init(hal::Info().sample_rate);
    lim_l.Init(hal::Info().sample_rate);
    lim_r.Init(hal::Info().sample_rate);
    shift_l.Init(hal::Info().sample_rate);

    // Tamanho de bloco é decisão de INSTRUMENTO, não de placa: ele
    // define a latência, e num instrumento realimentado a latência é
    // parte do laço acústico. Por isso vem do manifest. Tem que ser
    // pedido ANTES do StartAudio — o medidor de CPU se calibra pelo
    // valor efetivo.
    hal::RequestBlockSize(manifest.req.block_size);

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

        // watchdog: o callback só trava a flag; o log é aqui, porque
        // hal::Log bloqueia por milissegundos e não pode rodar no
        // contexto de áudio.
        if(fx2_overrun && !fx2_reported)
        {
            fx2_reported = true;
            hal::Log("WATCHDOG: modulo do slot fx2 estourou o prazo do "
                     "bloco (%.0f%% de pico). Forcado em bypass.",
                     hal::CpuLoadMax() * 100.f);
            hal::Log("  para rearmar: set fx2_on 0, depois set fx2_on 1");
        }
        // o callback e quem arma a graca; aqui so se avisa
        {
            const bool on = params.Real(manifest.p_fx2_on) >= 0.5f;
            if(on && !fx2_was_on)
            {
                fx2_reported = false;
                hal::Log("fx2: janela de graca de %lu blocos (custo de "
                         "estreia nao conta)", (unsigned long)kGraceBlocks);
            }
            fx2_was_on = on;
        }
        if(fx2_grace_done)
        {
            fx2_grace_done = false;
            const float budget_us = 1e6f * (float)hal::Info().block_size
                                    / hal::Info().sample_rate;
            hal::Log("fx2 ESTREIA: %.0f us de pico (%.0f%% do prazo)",
                     fx2_startup_peak * budget_us, fx2_startup_peak * 100.f);
            hal::Log("fx2: graca encerrada — daqui pra frente e regime");
        }

        // rearma quando o operador desliga explicitamente
        if(params.Real(manifest.p_fx2_on) < 0.5f && fx2_overrun)
        {
            fx2_overrun  = false;
            fx2_reported = false;
            hal::CpuLoadReset(); // senao o pico antigo re-trava na hora
        }

        // empurra pro módulo o que mudou. Fora do callback de áudio de
        // propósito: setParameterValue enfileira evento no RNBO.
        for(int b = 0; b < manifest.n_bind; b++)
        {
            if(mod_index[b] < 0)
                continue;
            // Real(), não Get(): é aqui que a curva do manifest passa a
            // valer. Os params do módulo são declarados 0–1, então Real()
            // devolve o normalizado JÁ CURVADO, que é o que o DSP quer.
            // Shaped(), não Real(): o módulo quer 0..1 com a curva do
            // manifest já aplicada. Real() traz a unidade (Hz, ms),
            // que é assunto nosso e não dele.
            const float v = params.Shaped(manifest.bind[b].param_id);
            if(v != mod_last[b])
            {
                mod_last[b] = v;
                mod_slot[(int)manifest.bind[b].slot]->SetParam(mod_index[b], v);
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
