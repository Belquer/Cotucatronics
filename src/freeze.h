#pragma once
// ============================================================
// Cotucatronics OS — freeze granular v0.2.0
// Captura contínua + sustentação por 2 grãos sorteados num núcleo.
//
// BLOCO PERMANENTE DO SHELL, não módulo RNBO — mesma família do
// reverb.h. O freeze RNBO antigo foi REPROVADO na etapa 5 do
// IFÁ10000 (7 349 µs contra 2 667 de orçamento); em C++ o mesmo
// gesto custou 85 µs medidos. E congelar é gesto de execução, não
// caráter de escultura: quem responde a um botão segurado pertence
// ao shell.
//
// C++ puro: sem headers de fabricante, sem RNBO, sem hal. O buffer
// vem DE FORA (ver Init) porque `hal::AllocBig()` é o único portão
// da RAM grande.
//
// ------------------------------------------------------------
// A RÉGUA DE ACEITE É VIDRO, NÃO GAIOLA — e a v0.1.0 REPROVOU.
//
// O dB ouviu a v0.1.0 e disse: **"cria um loop de mais ou menos 2 s
// com o áudio tocado"** e **"não soa como freeze, soa como um delay
// lento e permanente"**. Reprovado na etapa 9.
//
// A CAUSA, e ela estava escrita como defesa neste mesmo cabeçalho:
// a v0.1.0 fazia a cabeça de leitura VARRER o trecho congelado pra
// frente a 1x, com um jitter de ±25% do grão na posição de partida.
// Eu afirmei aqui que o jitter mataria a periodicidade. **Não mata.**
// Jitter de ±25 ms sobre um ciclo de 2 s apenas balança o laço: o
// material é finito, e varredura linear passa por ele inteiro e
// volta. Varrer um trecho a 1x É reproduzir esse trecho — ou seja,
// um delay longo em laço, exatamente o que o dB descreveu. O
// argumento estava no arquivo e a aritmética nunca foi feita.
//
// O CONSERTO, e a ideia que faltava:
//
//   SUSTENTAÇÃO NÃO É REPRODUÇÃO DE TRECHO.
//
// Um freeze deve soar como um som SEGURADO, não como uma passagem
// retocada. Então não há varredura e não há ordem:
//
//   a) Congelar define um NÚCLEO curto (~500 ms) terminando no
//      instante do gesto — o timbre que acabou de existir.
//   b) Cada grão que renasce SORTEIA uma posição dentro do núcleo.
//      Sorteio não tem período: não existe "voltar ao mesmo ponto",
//      porque não existe percurso.
//   c) DOIS grãos defasados em meio período, janela Hann. Hann a
//      50% de sobreposição soma exatamente 1.0 — a amplitude não
//      pulsa. É o que separa vidro de motor.
//
// Sortear é livre de clique por construção: cada grão nasce e morre
// em zero pela janela, então qualquer posição de partida serve.
// ============================================================

#include <cstddef>
#include <cstdint>
#include <cmath>

namespace cotuca
{

class Freeze
{
  public:
    // Quantos floats o Init precisa pra `seconds` de captura ESTÉREO.
    // Perguntado ao próprio freeze, nunca fixado por quem chama.
    static size_t FloatsNeeded(float sample_rate, float seconds)
    {
        return 2u * (size_t)(sample_rate * seconds);
    }

    // Devolve false se o buffer for curto. Quem chama DEVE checar:
    // buffer curto não soa errado, soa lendo memória alheia.
    bool Init(float sample_rate, float* mem, size_t n_floats, float seconds)
    {
        if(!mem || sample_rate <= 0.f || seconds <= 0.f)
            return false;
        if(n_floats < FloatsNeeded(sample_rate, seconds))
            return false;

        sr_  = sample_rate;
        cap_ = (size_t)(sample_rate * seconds);
        l_   = mem;
        r_   = mem + cap_;
        for(size_t i = 0; i < cap_; i++)
            l_[i] = r_[i] = 0.f;

        write_  = 0;
        frozen_ = false;
        mix_    = 0.f;
        rng_    = 0x9E3779B9u;
        SetSize(kDefaultGrainMs);
        return true;
    }

    void SetMix(float m) { mix_ = Clamp01(m); }

    // Janela do grão em MILISSEGUNDOS, limitada a um terço do núcleo:
    // grão maior que isso não deixa margem pra sortear, e os dois
    // grãos passam a ler quase o mesmo trecho — que é o batimento
    // voltando pela porta dos fundos.
    void SetSize(float ms)
    {
        if(cap_ == 0)
            return;
        float n = 0.001f * ms * sr_;
        const float lo = 0.008f * sr_;
        const float hi = NucleusLen() / 3.f;
        if(n < lo) n = lo;
        if(n > hi) n = hi;
        grain_ = n;
    }

    // Borda de subida CONGELA; borda de descida volta a capturar.
    // Toda a decisão de estado mora aqui, e não no Process — quem
    // arma e quem consome ficam no mesmo contexto (família A dos
    // bugs de ordem).
    void SetFrozen(bool on)
    {
        if(on == frozen_)
            return;
        frozen_ = on;
        if(!on)
            return;

        // O NÚCLEO termina no instante do gesto: é o timbre que
        // acabou de existir. Começar pelo material antigo entregaria
        // um som de 2 s atrás, que soa como atraso, não como
        // sustentação — e foi metade do defeito da v0.1.0.
        nucleus_ = NucleusLen();
        nuc0_    = WrapF((float)write_ - nucleus_);
        for(int g = 0; g < kGrains; g++)
        {
            // Meio período de defasagem: é o que faz as duas Hann
            // somarem 1.0 e a amplitude não pulsar.
            phase_[g] = (float)g / (float)kGrains;
            pos_[g]   = PickInNucleus();
        }
    }

    bool Frozen() const { return frozen_; }

    // Um par estéreo por chamada, in-place, igual ao reverb.
    void Process(float& l, float& r)
    {
        if(cap_ == 0)
            return;

        if(!frozen_)
        {
            // Gravador silencioso: enquanto não congela, não toca no
            // áudio. Gravar SEMPRE é o que permite ao gesto pegar o
            // som que acabou de acontecer.
            l_[write_] = l;
            r_[write_] = r;
            if(++write_ >= cap_)
                write_ = 0;
            return;
        }

        float wl = 0.f, wr = 0.f;
        for(int g = 0; g < kGrains; g++)
        {
            const float w = Hann(phase_[g]);
            wl += Read(l_, pos_[g]) * w;
            wr += Read(r_, pos_[g]) * w;

            pos_[g] += 1.f;
            if(pos_[g] >= (float)cap_)
                pos_[g] -= (float)cap_;

            phase_[g] += 1.f / grain_;
            if(phase_[g] >= 1.f)
            {
                phase_[g] -= 1.f;
                // SORTEIO, não avanço. Sem percurso não há período,
                // e sem período não há gaiola.
                pos_[g] = PickInNucleus();
            }
        }

        l = l * (1.f - mix_) + wl * mix_;
        r = r * (1.f - mix_) + wr * mix_;
    }

  private:
    static constexpr int   kGrains         = 2;
    static constexpr float kDefaultGrainMs = 100.f;
    // Núcleo de 500 ms: longo o bastante pra o sorteio ter de onde
    // escolher e a textura respirar, curto o bastante pra ser UM
    // timbre em vez de uma passagem musical. Acima de ~1 s o ouvido
    // volta a reconhecer material, e reconhecer material é o começo
    // de perceber repetição.
    static constexpr float kNucleusMs = 500.f;

    float NucleusLen() const
    {
        float n = 0.001f * kNucleusMs * sr_;
        const float teto = (float)(cap_ / 2u);
        return n > teto ? teto : n;
    }

    // Posição sorteada dentro do núcleo, com margem pro grão inteiro
    // caber sem atravessar a borda.
    float PickInNucleus()
    {
        const float margem = nucleus_ > grain_ ? nucleus_ - grain_ : 0.f;
        return WrapF(nuc0_ + Rand01() * margem);
    }

    static float Clamp01(float v)
    {
        return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    }

    // Hann em AMPLITUDE (não em potência): duas delas a 50% de
    // sobreposição somam 1.0 exato, que é a condição de não pulsar.
    static float Hann(float p)
    {
        return 0.5f * (1.f - std::cos(6.283185307179586f * p));
    }

    float WrapF(float x) const
    {
        const float n = (float)cap_;
        if(n <= 0.f)
            return 0.f;
        while(x < 0.f) x += n;
        while(x >= n)  x -= n;
        return x;
    }

    // xorshift32: barato, sem estado global, determinístico por
    // instância — dois freezes não compartilham sorte.
    float Rand01()
    {
        rng_ ^= rng_ << 13;
        rng_ ^= rng_ >> 17;
        rng_ ^= rng_ << 5;
        return (float)(rng_ >> 8) * (1.f / 16777216.f);
    }

    // Linear. A posição sorteada é fracionária, e truncar produziria
    // um degrau de até uma amostra a cada renascimento.
    float Read(const float* b, float p) const
    {
        const size_t i0 = (size_t)p;
        const float  f  = p - (float)i0;
        const size_t i1 = (i0 + 1u >= cap_) ? 0u : i0 + 1u;
        return b[i0] + f * (b[i1] - b[i0]);
    }

    float*   l_       = nullptr;
    float*   r_       = nullptr;
    size_t   cap_     = 0;
    size_t   write_   = 0;
    float    sr_      = 48000.f;
    float    grain_   = 4800.f;
    float    mix_     = 0.f;
    bool     frozen_  = false;
    float    nuc0_    = 0.f;  // início do núcleo
    float    nucleus_ = 24000.f;
    float    pos_[kGrains]   = {0.f, 0.f};
    float    phase_[kGrains] = {0.f, 0.5f};
    uint32_t rng_ = 0x9E3779B9u;
};

} // namespace cotuca
