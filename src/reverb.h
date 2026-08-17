#pragma once
// ============================================================
// Cotucatronics OS — reverb + passa-alta v0.2.0
// Freeverb clássico: 8 combs + 4 allpass por canal, estéreo.
//
// BLOCO PERMANENTE DO SHELL, não módulo RNBO. Duas razões, e as
// duas são de projeto, não de gosto:
//
//   1. O pool de SRAM dos módulos está a 92,4% com delay +
//      vocoder. Um terceiro módulo RNBO não entra sem mudar a
//      estratégia de alocação — a porta está fechada por
//      aritmética, não por preferência.
//   2. Reverb é FIM DE LINHA. Ele não é o caráter da escultura,
//      é o ar em volta dela. Bloco permanente do shell é
//      exatamente o lugar de quem molha o resultado de todo
//      mundo e não muda de escultura pra escultura.
//
// C++ puro: sem headers de fabricante, sem RNBO, sem hal. Os
// buffers vêm DE FORA (ver Init) porque `hal::AllocBig()` é o
// único portão da RAM grande e ele mora no main — a regra vale
// pra este arquivo como vale pros módulos. De quebra, isso deixa
// o reverb rodável no banco de teste nativo, no Mac, sem placa.
// ============================================================

#include <cstddef>
#include <cmath>

namespace cotuca
{

// ------------------------------------------------------------
// Afinações canônicas do Freeverb, em amostras a 44100 Hz.
// São as do Schroeder/Moorer que o Jezar publicou — números
// escolhidos por serem MUTUAMENTE PRIMOS, o que espalha os ecos
// e evita que os combs reforcem os mesmos períodos. Mexer neles
// sem entender isso produz metálico, não "outra sala".
//
// Reescalamos pra taxa real: a 48 kHz os mesmos ATRASOS EM
// AMOSTRAS dariam uma sala 8,8% menor, e a coprimalidade se
// preserva bem no arredondamento.
// ------------------------------------------------------------
namespace freeverb
{
constexpr int   kNumCombs    = 8;
constexpr int   kNumAllpass  = 4;
constexpr int   kCombTune[kNumCombs]
    = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
constexpr int   kAllpassTune[kNumAllpass] = {556, 441, 341, 225};
// Deslocamento do canal direito: descorrelaciona os dois lados
// sem precisar de um segundo jogo de afinações.
constexpr int   kStereoSpread = 23;
constexpr float kBaseRate     = 44100.f;
// Constantes do algoritmo original. O ganho fixo dos allpass em
// 0.5 é o que os torna transparentes em magnitude — eles
// difundem fase, não colorem.
constexpr float kAllpassFeedback = 0.5f;
constexpr float kFixedGain       = 0.015f;
} // namespace freeverb

class Reverb
{
  public:
    // Quantos floats o Init vai precisar nesta taxa. Chamar ANTES
    // de alocar — é o contrato que permite ao main pedir a
    // quantidade certa ao AllocBig sem este arquivo conhecer o hal.
    static size_t FloatsNeeded(float sample_rate)
    {
        const float k = Scale(sample_rate);
        size_t      n = 0;
        for(int i = 0; i < freeverb::kNumCombs; i++)
        {
            n += Tuned(freeverb::kCombTune[i], k, 0);
            n += Tuned(freeverb::kCombTune[i], k, freeverb::kStereoSpread);
        }
        for(int i = 0; i < freeverb::kNumAllpass; i++)
        {
            n += Tuned(freeverb::kAllpassTune[i], k, 0);
            n += Tuned(freeverb::kAllpassTune[i], k, freeverb::kStereoSpread);
        }
        return n;
    }

    // `mem` tem que ter pelo menos FloatsNeeded(sample_rate) floats
    // e viver enquanto o reverb viver. Devolve false se for curto —
    // e quem chama DEVE checar: reverb com buffer curto não sai
    // errado, sai lendo memória alheia.
    bool Init(float sample_rate, float* mem, size_t n_floats)
    {
        if(!mem || n_floats < FloatsNeeded(sample_rate))
            return false;

        const float k   = Scale(sample_rate);
        float*      p   = mem;
        for(int i = 0; i < freeverb::kNumCombs; i++)
        {
            comb_l_[i].Init(p, Tuned(freeverb::kCombTune[i], k, 0));
            p += comb_l_[i].size;
            comb_r_[i].Init(p, Tuned(freeverb::kCombTune[i], k,
                                     freeverb::kStereoSpread));
            p += comb_r_[i].size;
        }
        for(int i = 0; i < freeverb::kNumAllpass; i++)
        {
            ap_l_[i].Init(p, Tuned(freeverb::kAllpassTune[i], k, 0));
            p += ap_l_[i].size;
            ap_r_[i].Init(p, Tuned(freeverb::kAllpassTune[i], k,
                                   freeverb::kStereoSpread));
            p += ap_r_[i].size;
        }
        SetSize(0.5f);
        SetDamp(0.5f);
        SetMix(0.15f);
        ready_ = true;
        return true;
    }

    // Tamanho da sala: vira realimentação dos combs. A faixa
    // [0.7, 0.98] é do original e não é arbitrária — acima de ~0.98
    // a cauda deixa de decair em tempo musical e o reverb vira
    // oscilador. O teto existe pra que `rev_size 1.0` seja "sala
    // grande", não "realimentação infinita no palco".
    void SetSize(float norm)
    {
        norm     = Clamp01(norm);
        feedback_ = 0.7f + norm * 0.28f;
    }

    // Amortecimento: filtro passa-baixa DENTRO da realimentação de
    // cada comb, então agudos morrem antes dos graves — que é como
    // sala real se comporta (ar e superfícies absorvem agudo).
    void SetDamp(float norm)
    {
        norm   = Clamp01(norm);
        damp1_ = norm * 0.4f;
        damp2_ = 1.f - damp1_;
    }

    // Seco/molhado. Igual-potência seria mais "correto", mas linear
    // é o que o Freeverb faz e o que o ouvido espera de um knob de
    // reverb: em 0.15 você quer 15% de sala, não 39%.
    void SetMix(float norm) { mix_ = Clamp01(norm); }

    float Mix() const { return mix_; }

    // Uma amostra estéreo, in-place. Chamado do callback de áudio:
    // sem alocação, sem branch dependente de dado, custo constante.
    void Process(float& l, float& r)
    {
        if(!ready_ || mix_ <= 0.f)
            return;

        // A entrada do tanque é a SOMA dos dois canais. É o original,
        // e faz sentido: a sala é uma só — o que difere entre os
        // lados é a posição dos microfones, não a fonte.
        const float in = (l + r) * freeverb::kFixedGain;

        float wl = 0.f, wr = 0.f;
        // Combs em PARALELO: cada um é um modo da sala, e a soma
        // deles é a densidade modal.
        for(int i = 0; i < freeverb::kNumCombs; i++)
        {
            wl += comb_l_[i].Process(in, feedback_, damp1_, damp2_);
            wr += comb_r_[i].Process(in, feedback_, damp1_, damp2_);
        }
        // Allpass em SÉRIE: difundem a fase sem colorir a magnitude,
        // transformando ecos discretos em cauda contínua.
        for(int i = 0; i < freeverb::kNumAllpass; i++)
        {
            wl = ap_l_[i].Process(wl);
            wr = ap_r_[i].Process(wr);
        }

        l += (wl - l) * mix_;
        r += (wr - r) * mix_;
    }

  private:
    static float Clamp01(float v)
    {
        return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    }
    static float Scale(float sr)
    {
        return (sr > 0.f) ? sr / freeverb::kBaseRate : 1.f;
    }
    static size_t Tuned(int base, float k, int spread)
    {
        const int n = (int)((float)(base + spread) * k + 0.5f);
        return (size_t)(n < 2 ? 2 : n);
    }

    struct Comb
    {
        float* buf  = nullptr;
        size_t size = 0;
        size_t idx  = 0;
        float  store = 0.f; // estado do passa-baixa do amortecimento
        void   Init(float* p, size_t n)
        {
            buf = p; size = n; idx = 0; store = 0.f;
            for(size_t i = 0; i < n; i++) buf[i] = 0.f;
        }
        float Process(float in, float fb, float d1, float d2)
        {
            const float out = buf[idx];
            store           = out * d2 + store * d1; // passa-baixa
            buf[idx]        = in + store * fb;
            if(++idx >= size) idx = 0;
            return out;
        }
    };

    struct Allpass
    {
        float* buf  = nullptr;
        size_t size = 0;
        size_t idx  = 0;
        void   Init(float* p, size_t n)
        {
            buf = p; size = n; idx = 0;
            for(size_t i = 0; i < n; i++) buf[i] = 0.f;
        }
        float Process(float in)
        {
            const float bufout = buf[idx];
            const float out    = -in + bufout;
            buf[idx]           = in + bufout * freeverb::kAllpassFeedback;
            if(++idx >= size) idx = 0;
            return out;
        }
    };

    Comb    comb_l_[freeverb::kNumCombs];
    Comb    comb_r_[freeverb::kNumCombs];
    Allpass ap_l_[freeverb::kNumAllpass];
    Allpass ap_r_[freeverb::kNumAllpass];
    float   feedback_ = 0.84f;
    float   damp1_    = 0.2f;
    float   damp2_    = 0.8f;
    float   mix_      = 0.15f;
    bool    ready_    = false;
};

// ------------------------------------------------------------
// PASSA-ALTA de 2ª ordem (dois polos de 1ª em série).
//
// Existe pro caminho do Fx2: certas portadoras do vocoder fazem a
// saída picar 1,4–1,5 enquanto outras ficam em 0,55. O que ainda
// NÃO se sabe é se o excesso é energia grave (e aí isto resolve)
// ou desequilíbrio de nível por nota (e aí isto não resolve, e o
// próprio resultado da medição diz qual dos dois é).
//
// Dois polos porque um só (6 dB/oitava) é raso demais pra tirar
// energia de sub sem levar junto o corpo do som. 12 dB/oitava
// corta o que sobra e preserva o que interessa.
// ------------------------------------------------------------
class HighPass
{
  public:
    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        SetCutoff(0.f);
        z1_ = z2_ = 0.f;
    }
    // 0 Hz = desligado (passa tudo), sem branch no caminho de sinal.
    void SetCutoff(float hz)
    {
        if(hz <= 0.f) { a_ = 0.f; return; }
        if(hz > sr_ * 0.45f) hz = sr_ * 0.45f;
        const float x = expf(-6.283185307f * hz / sr_);
        a_ = x;
    }
    float Process(float in)
    {
        if(a_ <= 0.f) return in;
        z1_ = in  * (1.f - a_) + z1_ * a_;   // passa-baixa 1
        const float h1 = in - z1_;           // = passa-alta 1
        z2_ = h1  * (1.f - a_) + z2_ * a_;
        return h1 - z2_;                     // passa-alta 2
    }

  private:
    float sr_ = 48000.f, a_ = 0.f, z1_ = 0.f, z2_ = 0.f;
};

} // namespace cotuca
