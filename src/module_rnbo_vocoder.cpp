// ============================================================
// Cotucatronics OS — módulo DSP: vocoder (export do RNBO)
// module_rnbo_vocoder v0.5.0
//
// Slot Fx2. Cross-synthesis espectral de UMA entrada.
//
// PORTADORA INTERNA (export de 2026-08-16): o patch gera a própria
// portadora — mistura de senoides e ruído, que é o que os três
// `cycle~` novos e o RNBODefaultSinus de 16384 amostras fazem ali.
// O description.json confirma: 1 entrada, 2 saídas. O export
// ANTERIOR era 2/2 e esperava portadora externa no ch2.
//
// Consequência prática: o `carrier` que o shell manda (IN_R cru) é
// ignorado. Não é bug — é fiação que deixou de ser necessária —,
// mas declarar 2 entradas faria o `mods` mentir sobre o contrato.
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

// ------------------------------------------------------------
// O FALLBACK NÃO É FALLBACK: é o único caminho que roda.
//
// Descoberto em 2026-08-16. `r_window` é `buffer~`, e o export
// NUNCA chama requestSize() nele — `allocateDataRefs()` pede
// tamanho pros `data` internos e pro RNBODefaultSinus, e passa
// direto pelo r_window, que só aparece num
// `if(hasRequestedSize())` que nunca é verdadeiro.
//
// Logo `getSizeInBytes()` devolve 0 SEMPRE, e o codigo que "le o
// tamanho do proprio dataref" nunca executou uma vez. O comentario
// original temia que fixar 2048 fizesse firmware e patch
// discordarem em silencio no dia em que a FFT mudasse. O medo
// estava certo e o mecanismo de defesa nao funcionava.
//
// E a discordancia ESTAVA la: os dois exports do vocoder sao FFT
// 1024, e a janela vinha com 2048. O patch lia so a primeira
// metade do sqrt(hann) — uma rampa subindo — e o overlap-add
// nunca somou plano. Passou despercebido porque o teste auditivo
// do vocoder segue na fila congelada: o modulo nunca foi ouvido.
//
// 1024 e constante DESTE patch. Este arquivo ja e especifico deste
// patch, entao a constante pertence aqui — mas se o dB reexportar
// com outra FFT, ESTA LINHA MUDA JUNTO. Nao ha rede de seguranca
// automatica; o `mods` loga o tamanho pra conferencia manual.
// ------------------------------------------------------------
constexpr size_t      kWindowFallback = 1024;

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

        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        patcher_.prepareToProcess(sample_rate, block_size, true);

        // ------------------------------------------------------
        // PREENCHE DE NOVO, DEPOIS. Não é redundância defensiva:
        // prepareToProcess chama allocateDataRefs(), e lá dentro
        // `data_01_buffer` — que é uma VIEW do r_window — chama
        // requestSize(1024, 1).
        //
        // Isso dá ao r_window um tamanho pedido que ele NÃO tinha
        // quando preenchemos. O allocateIfNeeded() seguinte pode
        // então trocar o ponteiro por um buffer novo e ZERADO,
        // jogando fora a janela recém-escrita. E o fillr_window()
        // do próprio export não cobre o buraco: ele só roda se
        // wantsFill() for verdadeiro, e setTouched(true) — que a
        // gente chama pra avisar que os dados vieram de fora — é
        // justamente o que faz wantsFill() virar falso.
        //
        // Resultado: janela toda zero, sete multiplicações por
        // zero, VOCODER MUDO. Roda, custa CPU, não sai som, e não
        // loga nada. Exatamente o modo de falha que o comentário
        // de cima descreve — só que a causa não era ninguém
        // esquecer de preencher, era o preenchimento ser
        // descartado depois.
        //
        // A ordem certa é preencher NOS DOIS momentos: antes,
        // porque processDataViewUpdate liga as views e
        // prepareToProcess já as consulta; e depois, porque é só
        // depois que o buffer definitivo existe.
        // ------------------------------------------------------
        const bool refill_ok = FillWindow();
        window_ok_ = window_ok_ && refill_ok;

        hal::Log("  vocoder: janela %u amostras, soma=%.1f%s",
                 (unsigned)window_samples_, (double)window_sum_,
                 window_sum_ > 0.f ? "" : "  <<< ZERADA: SAIRIA MUDO");

        n_params_ = static_cast<int>(patcher_.getNumParameters());
        mem_      = rnbo_alloc::Delta(before);

        if(n_params_ <= 0 || mem_.failures != 0)
            return false;
        // Sem janela o módulo sairia mudo em silêncio. Melhor recusar
        // no boot, com log, do que carregar um vocoder que não soa.
        return window_ok_;
    }

    const char* Name() const override { return "vocoder"; }
    // 1, não 2: o export novo tem portadora interna. Ver o cabeçalho.
    int         InputCount() const override { return 1; }
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
        // ------------------------------------------------------
        // DIRIGIR O MOTOR, não só o patcher.
        //
        // MinimalEngine NÃO avança o tempo sozinho: quem chama tem
        // que rodar processEventsUntil() e advanceTime() a cada
        // bloco. Antes daqui o wrapper chamava patcher_.process()
        // direto e pulava o motor — então `_currentTime` ficava em
        // ZERO pra sempre e NENHUM evento agendado jamais executava.
        //
        // Isso não silencia o sinal puro: FFT, cycle~ e delay são
        // domínio de sinal e rodam de qualquer jeito. Silencia o que
        // depende de EVENTO — clock, metro, rampa de parâmetro,
        // inicialização adiada. Num patch cuja portadora interna é
        // montada por evento, o resultado é oscilador parado em 0 Hz
        // e cross-synthesis contra DC: saída nula, sem erro nenhum.
        //
        // É a explicação de "roda perfeito no Max e mudo na placa":
        // no Max o motor é do Max e sempre andou.
        //
        // Ordem: eventos vencidos primeiro, áudio depois, tempo por
        // último. Evento agendado DURANTE este bloco dispara no
        // começo do próximo — um bloco de latência, que é o preço
        // honesto de não fatiar o bloco por amostra.
        // ------------------------------------------------------
        engine_.processEventsUntil(engine_.getCurrentTime());

        const RNBO::SampleValue* ins[1]  = {in[0]};
        RNBO::SampleValue*       outs[2] = {out[0], out[1]};
        patcher_.process(ins, 1, outs, 2, static_cast<RNBO::Index>(size));

        engine_.advanceTime(1000.0 * (double)size / (double)sr_);
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

            // REUSA o buffer entre as duas chamadas. AllocBig nunca
            // devolve memória, então alocar de novo na segunda passada
            // vazaria a primeira — pouco, mas sem motivo.
            if(!window_buf_ || window_bytes_ != bytes)
            {
                window_buf_   = static_cast<float*>(hal::AllocBig(bytes));
                window_bytes_ = bytes;
            }
            float* w = window_buf_;
            if(!w)
                return false; // sem SDRAM: recusa, não roda mudo

            // sqrt(hann): a raiz é o que faz análise e síntese
            // somarem plano no overlap-add (COLA). Hann puro nas duas
            // pontas somaria hann², que ondula.
            const float twopi = 6.283185307179586f;
            window_sum_       = 0.f;
            for(size_t k = 0; k < count; k++)
            {
                const float hann
                    = 0.5f * (1.f - cosf(twopi * (float)k / (float)(count - 1)));
                w[k] = sqrtf(hann);
                window_sum_ += w[k];
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
    float                 sr_             = 48000.f;
    float*                window_buf_     = nullptr;
    size_t                window_bytes_   = 0;
    // Soma das amostras da janela. Serve de prova de vida: sqrt(hann)
    // de 1024 pontos soma ~652. Zero significa janela vazia, e janela
    // vazia significa vocoder mudo — que é o defeito que nao loga.
    float                 window_sum_     = 0.f;
    MemReport             mem_;
};

VocoderModule   g_vocoder;
ModuleRegistrar g_registrar(ModuleSlot::Fx2, &g_vocoder);

} // namespace
} // namespace cotuca
