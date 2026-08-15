#pragma once
// ============================================================
// Cotucatronics OS — params v0.1.0
// Tabela de parâmetros + macros de knob. Board-agnostic:
// não inclui hal.h nem header de fabricante nenhum.
//
// Sem alocação dinâmica, sem exceções, sem std::string —
// tudo array fixo, seguro pra chamar do callback de áudio.
// ============================================================

#include <cstddef>

namespace cotuca
{

#define COTUCA_PARAMS_VERSION "0.1.0"

// knobs físicos do painel (manifest declara Macro knob[kNumKnobs])
constexpr int kNumKnobs = 4;

// tetos estáticos — estourar é erro de manifest, não de runtime
constexpr int kMaxParams       = 32;
constexpr int kMaxMacroTargets = 4;

inline float Clamp01(float v)
{
    return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
}

// ------------------------------------------------------------
// ParamTable
// Guarda cada parâmetro em duas formas:
//   - normalizado 0..1  → o que knob/serial/RNBO falam
//   - real (min..max)   → o que o DSP consome
// Conversão é linear. Se algum dia um parâmetro precisar de
// curva (freq em oitavas, tempo em log), o lugar de mexer é
// Real() — nada fora daqui olha a escala.
// ------------------------------------------------------------
class ParamTable
{
  public:
    void Init() { count_ = 0; }

    // Retorna o id do parâmetro (índice). -1 se a tabela encheu.
    int Add(const char* name, float def_norm, float min_real, float max_real)
    {
        if(count_ >= kMaxParams)
            return -1;
        const int id = count_++;
        name_[id]    = name;
        min_[id]     = min_real;
        max_[id]     = max_real;
        norm_[id]    = Clamp01(def_norm);
        return id;
    }

    void Set(int id, float norm)
    {
        if(!Valid(id))
            return;
        norm_[id] = Clamp01(norm);
    }

    float Get(int id) const { return Valid(id) ? norm_[id] : 0.f; }

    float Real(int id) const
    {
        if(!Valid(id))
            return 0.f;
        return min_[id] + norm_[id] * (max_[id] - min_[id]);
    }

    const char* Name(int id) const { return Valid(id) ? name_[id] : "?"; }

    int Count() const { return count_; }

  private:
    bool Valid(int id) const { return id >= 0 && id < count_; }

    const char* name_[kMaxParams] = {};
    float       min_[kMaxParams]  = {};
    float       max_[kMaxParams]  = {};
    // volatile: escrito pela ISR do serial, lido pelo callback de
    // áudio. float de 32 bits é atômico no M7, então não há rasgo —
    // volatile só impede o compilador de cachear o valor.
    volatile float norm_[kMaxParams] = {};
    int            count_            = 0;
};

// ------------------------------------------------------------
// Macro — um knob físico movendo N parâmetros de uma vez.
//
// Convenção de peso (w) para um knob em v (0..1):
//   w > 0 : alvo = v * w          (w é a PROFUNDIDADE; w=1 → 1:1)
//   w < 0 : alvo = 1 + v * w      (invertido; w=-1 → 1-v)
// Ou seja knob[1].AddTarget(p_damp, -0.5f) faz o parâmetro ir
// de 1.0 (knob em zero) até 0.5 (knob no talo).
//
// Isso é uma decisão de design, não uma lei — se quiser outra
// curva (bipolar em torno do default, por ex.), o único lugar
// a mexer é Apply().
// ------------------------------------------------------------
struct Macro
{
    void AddTarget(int param_id, float weight)
    {
        if(n_ >= kMaxMacroTargets || param_id < 0)
            return;
        target_[n_] = param_id;
        weight_[n_] = weight;
        n_++;
    }

    void Apply(ParamTable& t, float v) const
    {
        v = Clamp01(v);
        for(int i = 0; i < n_; i++)
        {
            const float w = weight_[i];
            const float x = (w >= 0.f) ? (v * w) : (1.f + v * w);
            t.Set(target_[i], x);
        }
    }

    int TargetCount() const { return n_; }

  private:
    int   target_[kMaxMacroTargets] = {};
    float weight_[kMaxMacroTargets] = {};
    int   n_                        = 0;
};

} // namespace cotuca
