// ============================================================
// Cotucatronics OS — módulo DSP: vocoder (export do RNBO)
// module_rnbo_vocoder v0.1.0
//
// Slot Fx2. Cross-synthesis espectral de duas entradas:
//   ch1 = escultura (modulador)  ·  ch2 = jack de linha (portadora)
//
// Um dos arquivos que podem incluir headers do RNBO. Ver
// module_rnbo_delay.cpp pra explicação do renome de símbolos.
// ============================================================

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
inline void* malloc(size_t n) { return cotuca::rnbo_alloc::Alloc(n); }
inline void  free(void*) {}
inline void* calloc(size_t c, size_t s) { return cotuca::rnbo_alloc::Calloc(c, s); }
inline void* realloc(void* p, size_t n) { return cotuca::rnbo_alloc::Realloc(p, n); }
} // namespace Platform
} // namespace RNBO

#include "RNBO_Common.h"
#include "RNBO_MinimalEngine.h"

#define rnbomatic                rnbomatic_vocoder
#define listquicksort            listquicksort_vocoder
#define listpartition            listpartition_vocoder
#define listswapelements         listswapelements_vocoder
#define rnbomaticFactoryFunction rnbomaticFactoryFunction_vocoder
#define rnbomaticSetLogger       rnbomaticSetLogger_vocoder
#include "modules/vocoder/rnbo_export/cotucatronics_vocoder.cpp"
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
using Patcher = RNBO::rnbomatic_vocoder<>;

// ------------------------------------------------------------
// A JANELA VEM DO HOST, e isso é regra, não gambiarra.
//
// O patch tem três datarefs. Dois são `data @external 0` —
// frzmag1/frzmag2, as memórias de molde — e o RNBO os aloca e
// gerencia sozinho, porque são ESTADO do patch.
//
// O terceiro é `r_window`, a janela sqrt(hann) das sete análises
// e sínteses. Ele é `buffer~`, e `buffer~` é EXTERNO POR
// NATUREZA no RNBO: `@external 0` é atributo de `data` e não se
// aplica. Não adianta o patch tentar — a janela tem que vir de
// fora.
//
// Se ninguém preencher, o buffer nasce vazio, as sete janelas
// multiplicam por zero e o vocoder sai MUDO. Sem erro de boot,
// sem log. É a pior classe de defeito, e foi por isso que a
// conferência do description.json existe.
//
// Preencher aqui é o certo, não o remendo: sqrt(hann) é função
// matemática exata, mais confiável gerada em C++ que desenhada
// no Max, e alimentar dataref externo é literalmente pra isso que
// o mecanismo existe. O conhecimento específico do patch — que
// há uma janela chamada "r_window" — mora neste arquivo, que já
// é específico deste patch. Nada vaza pra fronteira.
// ------------------------------------------------------------
constexpr const char* kWindowRef = "r_window";
constexpr size_t      kWindowFallback = 2048; // só se o patch não pedir

class VocoderModule final : public Module
{
  public:
    bool Init(float sample_rate, size_t block_size) override
    {
        const MemReport before = rnbo_alloc::Stats();

        engine_.initialize();

        // ANTES do prepareToProcess: as views das FFTs são ligadas à
        // janela em processDataViewUpdate, e prepareToProcess já as
        // consulta.
        window_ok_ = FillWindow();

        patcher_.prepareToProcess(sample_rate, block_size, true);

        n_params_ = static_cast<int>(patcher_.getNumParameters());
        mem_      = rnbo_alloc::Delta(before);

        if(n_params_ <= 0 || mem_.failures != 0)
            return false;
        // Sem janela o módulo sairia mudo em silêncio. Melhor recusar
        // no boot, com log, do que carregar um vocoder que não soa.
        return window_ok_;
    }

    const char* Name() const override { return "vocoder"; }
    int         InputCount() const override { return 2; } // ch2 = portadora
    int         ParamCount() const override { return n_params_; }

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
        return -1;
    }

    void SetParam(int i, float normalized) override
    {
        if(i < 0 || i >= n_params_) return;
        patcher_.setParameterValueNormalized(
            static_cast<RNBO::ParameterIndex>(i), normalized, RNBO::TimeNow);
    }

    MemReport Memory() const override { return mem_; }

    void Process(const float* const* in, float** out, size_t size) override
    {
        // description.json: 2 entradas, 2 saídas.
        const RNBO::SampleValue* ins[2]  = {in[0], in[1]};
        RNBO::SampleValue*       outs[2] = {out[0], out[1]};
        patcher_.process(ins, 2, outs, 2, static_cast<RNBO::Index>(size));
    }

    size_t WindowSamples() const { return window_samples_; }

  private:
    // Acha o dataref pelo NOME, não por índice: reexportar o patch
    // reordena datarefs pela mesma razão que reordena parâmetros.
    bool FillWindow()
    {
        const RNBO::DataRefIndex n = patcher_.getNumDataRefs();
        for(RNBO::DataRefIndex i = 0; i < n; i++)
        {
            RNBO::DataRef* ref = patcher_.getDataRef(i);
            if(!ref || !ref->getName())
                continue;
            if(strcmp(ref->getName(), kWindowRef) != 0)
                continue;

            // Tamanho lido do PRÓPRIO dataref quando o patch o pede.
            // Fixar 2048 aqui faria o firmware e o patch discordarem
            // em silêncio no dia em que a FFT mudar de tamanho.
            size_t bytes = ref->getSizeInBytes();
            if(bytes == 0)
                bytes = kWindowFallback * sizeof(float);

            const size_t count = bytes / sizeof(float);
            if(count < 2)
                return false;

            float* w = static_cast<float*>(hal::AllocBig(bytes));
            if(!w)
                return false; // sem SDRAM: recusa, não roda mudo

            // sqrt(hann): a raiz é o que faz análise e síntese
            // somarem plano no overlap-add (COLA). Hann puro nas duas
            // pontas somaria hann², que ondula.
            const float twopi = 6.283185307179586f;
            for(size_t k = 0; k < count; k++)
            {
                const float hann
                    = 0.5f * (1.f - cosf(twopi * (float)k / (float)(count - 1)));
                w[k] = sqrtf(hann);
            }

            ref->setData(reinterpret_cast<char*>(w), bytes);
            ref->setTouched(true);
            patcher_.processDataViewUpdate(i, RNBO::TimeNow);
            window_samples_ = count;
            return true;
        }
        return false; // "r_window" não existe: patch trocado
    }

    Patcher               patcher_;
    RNBO::MinimalEngine<> engine_{&patcher_};
    int                   n_params_       = 0;
    bool                  window_ok_      = false;
    size_t                window_samples_ = 0;
    MemReport             mem_;
};

VocoderModule   g_vocoder;
ModuleRegistrar g_registrar(ModuleSlot::Fx2, &g_vocoder);

} // namespace
} // namespace cotuca
