// ============================================================
// Cotucatronics OS — módulo DSP: export do RNBO
// module_rnbo v0.2.0
//
// ÚNICO arquivo do projeto que pode incluir headers do RNBO.
// Regra espelhada da fronteira HAL: assim como só
// hal_<board>.cpp vê daisy_seed.h, só este arquivo vê RNBO.h.
// Trocar de motor de DSP = escrever outro module_<engine>.cpp.
// ============================================================

// --- configuração do RNBO. Precisa vir ANTES dos includes. ---
//
// FLOAT32: por padrão SampleValue é double. Com float32 ele passa
// a ser float, que é exatamente o que o hal::AudioCallback entrega
// — os buffers vão direto pro process(), sem conversão nem buffer
// intermediário. E float é mais rápido que double no M7 mesmo com
// FPU de dupla precisão.
#define RNBO_USE_FLOAT32

// Sem fábrica dinâmica de patchers: não há carregamento em runtime.
#define RNBO_NO_PATCHERFACTORY

// A build da libDaisy usa -fno-exceptions. Sem isto o RNBO tenta
// throw std::runtime_error e o compilador recusa.
#define RNBO_NOTHROW

// Assume o controle de toda alocação do RNBO (ver Platform abaixo).
#define RNBO_USECUSTOMALLOCATOR

#include "module_rnbo.h"
#include "hal.h"

#include <cstring>
#include <cstdint>

// ------------------------------------------------------------
// Alocador do RNBO
//
// Com RNBO_USECUSTOMALLOCATOR o RNBO deixa de usar malloc da libc
// e passa a chamar estas quatro funções. Toda a alocação dele
// acontece no boot (initialize + prepareToProcess); no process()
// não há alocação nenhuma, que é o que torna um alocador bump
// aceitável aqui.
//
// Estratégia: pool estático em SRAM primeiro, porque os sinais
// pequenos são lidos amostra a amostra e SDRAM é mais lenta. O que
// não couber — tipicamente a linha de delay — vai pra SDRAM pelo
// hal::AllocBig(), respeitando o portão único do BOARDS.md.
//
// free() é no-op: bump não devolve. Como nada é liberado em
// runtime, isso não vaza de verdade — só significa que a memória
// do módulo é vitalícia, que é o comportamento desejado.
// ------------------------------------------------------------
namespace
{
constexpr size_t kInternalPool = 32u * 1024u;
constexpr size_t kHeader       = 8; // guarda o tamanho e mantém alinhamento 8

alignas(8) uint8_t g_pool[kInternalPool];
size_t             g_pool_used = 0;

cotuca::Module::MemReport g_stats = {0, kInternalPool, 0, 0, 0};

inline size_t Align8(size_t n) { return (n + 7u) & ~size_t(7); }

void* AllocRaw(size_t size)
{
    const size_t total = Align8(size + kHeader);

    uint8_t* base = nullptr;
    if(g_pool_used + total <= kInternalPool)
    {
        base = &g_pool[g_pool_used];
        g_pool_used += total;
        g_stats.internal_bytes += total;
    }
    else
    {
        base = static_cast<uint8_t*>(cotuca::hal::AllocBig(total));
        if(!base)
        {
            g_stats.failures++;
            return nullptr;
        }
        g_stats.big_bytes += total;
    }

    g_stats.blocks++;
    // guarda o tamanho útil: realloc precisa saber quanto copiar
    *reinterpret_cast<size_t*>(base) = size;
    return base + kHeader;
}

inline size_t BlockSize(void* p)
{
    return *reinterpret_cast<size_t*>(static_cast<uint8_t*>(p) - kHeader);
}
} // namespace

namespace RNBO
{
namespace Platform
{
void* malloc(size_t size) { return AllocRaw(size); }

void free(void*) { /* bump allocator: nada a devolver */ }

void* calloc(size_t count, size_t size)
{
    const size_t total = count * size;
    void*        p     = AllocRaw(total);
    if(p)
        memset(p, 0, total);
    return p;
}

void* realloc(void* ptr, size_t size)
{
    if(!ptr)
        return AllocRaw(size);
    const size_t old = BlockSize(ptr);
    if(size <= old)
        return ptr; // cabe no bloco atual, não mexe
    void* p = AllocRaw(size);
    if(p)
        memcpy(p, ptr, old);
    return p;
}
} // namespace Platform
} // namespace RNBO

// --- headers do RNBO e o patcher gerado ---
// O .cpp gerado é incluído como fonte de propósito: ele define a
// template rnbomatic<> inteira e precisa das nossas macros acima.
#include "RNBO_Common.h"
#include "RNBO_MinimalEngine.h"
#include "modules/delay/rnbo_export/cotucatronics_delay.cpp"

namespace cotuca
{

namespace
{

class RnboModule final : public Module
{
  public:
    bool Init(float sample_rate, size_t block_size) override
    {
        engine_.initialize();
        patcher_.prepareToProcess(sample_rate, block_size, true);

        n_params_ = static_cast<int>(patcher_.getNumParameters());
        if(n_params_ <= 0)
            return false;
        // se o alocador falhou em algum ponto, o patcher está
        // meio montado — melhor recusar no boot que estourar no
        // primeiro bloco de áudio.
        return g_stats.failures == 0;
    }

    const char* Name() const override { return "delay"; }

    int ParamCount() const override { return n_params_; }

    const char* ParamName(int index) const override
    {
        if(index < 0 || index >= n_params_)
            return "?";
        return patcher_.getParameterName(
            static_cast<RNBO::ParameterIndex>(index));
    }

    int FindParam(const char* name) const override
    {
        for(int i = 0; i < n_params_; i++)
        {
            const char* n = patcher_.getParameterName(
                static_cast<RNBO::ParameterIndex>(i));
            if(n && strcmp(n, name) == 0)
                return i;
        }
        return -1; // quem chama DEVE checar: patch trocado vira erro no boot
    }

    void SetParam(int index, float normalized) override
    {
        if(index < 0 || index >= n_params_)
            return;
        // Os params deste patch já são 0–1, então o valor
        // normalizado da ParamTable entra direto.
        patcher_.setParameterValue(static_cast<RNBO::ParameterIndex>(index),
                                   normalized,
                                   RNBO::TimeNow);
    }

    MemReport Memory() const override { return g_stats; }

    void Process(const float* const* in, float** out, size_t size) override
    {
        // description.json: 1 entrada, 2 saídas.
        const RNBO::SampleValue* ins[1]  = {in[0]};
        RNBO::SampleValue*       outs[2] = {out[0], out[1]};
        patcher_.process(ins, 1, outs, 2, static_cast<RNBO::Index>(size));
    }

  private:
    // ordem importa: a engine recebe &patcher_ no construtor
    RNBO::rnbomatic<>       patcher_;
    RNBO::MinimalEngine<>   engine_{&patcher_};
    int                     n_params_ = 0;
};

RnboModule g_module; // estático: sem heap, vida do programa inteiro

// Auto-registro no slot INSTRUMENT. É isto que tira o
// `#include "module_rnbo.h"` do main.cpp: o main pergunta ao
// registro por slot e nunca nomeia um motor concreto.
ModuleRegistrar g_registrar(ModuleSlot::Instrument, &g_module);

} // namespace

} // namespace cotuca
