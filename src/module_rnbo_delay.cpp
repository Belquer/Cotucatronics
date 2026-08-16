#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

#include "module_rnbo_alloc.h"
#include "module.h"

#define RNBO_USE_FLOAT32
#define RNBO_NO_PATCHERFACTORY
#define RNBO_NOTHROW
#define RNBO_USECUSTOMALLOCATOR

namespace RNBO
{
namespace Platform
{
// inline: os dois modulos definem estas funcoes de forma IDENTICA,
// entao os simbolos fracos se fundem no link. A memoria e
// compartilhada de proposito — pool de SRAM e SDRAM sao recursos da
// placa, nao de um motor.
inline void* malloc(size_t n) { return cotuca::rnbo_alloc::Alloc(n); }
inline void  free(void*) {}
inline void* calloc(size_t c, size_t s) { return cotuca::rnbo_alloc::Calloc(c, s); }
inline void* realloc(void* p, size_t n) { return cotuca::rnbo_alloc::Realloc(p, n); }
} // namespace Platform
} // namespace RNBO

#include "RNBO_Common.h"
#include "RNBO_MinimalEngine.h"

// ---- renomeia o que COLIDE entre os dois exports ----
//
// Os dois geram `rnbomatic` e os tres templates `list*` com nomes
// identicos no namespace RNBO. Em TUs separadas isso nao da erro de
// compilacao: da violacao de ODR no LINK, e o linker escolhe uma
// definicao em silencio — o delay poderia acabar rodando o DSP do
// freeze sem uma unica mensagem.
//
// Envolver o runtime num namespace por patcher seria mais limpo, mas
// NAO FUNCIONA: o runtime especializa std::default_delete, e dentro
// de um namespace isso vira <ns>::std, que nao existe. Renomear os
// quatro simbolos e o que resta, e e suficiente — foi conferido que
// nao ha outras colisoes de nivel de namespace entre os exports.
#define rnbomatic        rnbomatic_delay
#define listquicksort    listquicksort_delay
#define listpartition    listpartition_delay
#define listswapelements listswapelements_delay
// Estes dois sao identificadores INTEIROS, nao composicoes com
// `rnbomatic` — o preprocessador substitui token completo, entao
// precisam de #define proprio. Sao guardados por
// RNBO_NO_PATCHERFACTORY em parte do codigo, mas nao em tudo.
#define rnbomaticFactoryFunction rnbomaticFactoryFunction_delay
#define rnbomaticSetLogger       rnbomaticSetLogger_delay
#include "modules/delay/rnbo_export/cotucatronics_delay.cpp"
#undef rnbomatic
#undef listquicksort
#undef listpartition
#undef listswapelements
#undef rnbomaticFactoryFunction
#undef rnbomaticSetLogger

namespace cotuca
{
namespace
{
using Patcher = RNBO::rnbomatic_delay<>;

class DelayModule final : public Module
{
  public:
    bool Init(float sample_rate, size_t block_size) override
    {
        const MemReport before = rnbo_alloc::Stats();
        engine_.initialize();
        patcher_.prepareToProcess(sample_rate, block_size, true);
        n_params_ = static_cast<int>(patcher_.getNumParameters());
        mem_      = rnbo_alloc::Delta(before);
        if(n_params_ <= 0)
            return false;
        return mem_.failures == 0;
    }

    const char* Name() const override { return "delay"; }
    int         InputCount() const override { return 1; } // description.json
    int ParamCount() const override { return n_params_; }

    const char* ParamName(int i) const override
    {
        if(i < 0 || i >= n_params_) return "?";
        return patcher_.getParameterName(static_cast<RNBO::ParameterIndex>(i));
    }

    int FindParam(const char* name) const override
    {
        for(int i = 0; i < n_params_; i++)
        {
            const char* n = patcher_.getParameterName(
                static_cast<RNBO::ParameterIndex>(i));
            if(n && strcmp(n, name) == 0) return i;
        }
        return -1; // quem chama DEVE checar
    }

    void SetParam(int i, float normalized) override
    {
        if(i < 0 || i >= n_params_) return;
        // Normalized, nao cru. Os params deste patch sao 0-1, entao
        // dava no mesmo — mas o contrato de Module::SetParam e
        // normalizado, e depender da coincidencia quebra no dia em que
        // o patch ganhar um param com faixa real. O freeze ja tem.
        patcher_.setParameterValueNormalized(
            static_cast<RNBO::ParameterIndex>(i), normalized, RNBO::TimeNow);
    }

    MemReport Memory() const override { return mem_; }

    void Process(const float* const* in, float** out, size_t size) override
    {
        // description.json: 1 entrada, 2 saidas.
        const RNBO::SampleValue* ins[1]  = {in[0]};
        RNBO::SampleValue*       outs[2] = {out[0], out[1]};
        patcher_.process(ins, 1, outs, 2, static_cast<RNBO::Index>(size));
    }

  private:
    Patcher                   patcher_;
    RNBO::MinimalEngine<>     engine_{&patcher_};
    int                       n_params_ = 0;
    MemReport                 mem_;
};

DelayModule     g_delay;
ModuleRegistrar g_registrar(ModuleSlot::Instrument, &g_delay);

} // namespace
} // namespace cotuca
