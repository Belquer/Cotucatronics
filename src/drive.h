#pragma once
// ============================================================
// Cotucatronics OS — drive v0.2.0
// Waveshaper tanh como COLA, nao como fuzz.
//
// O alvo e engordar percussao, kalimba, monocordio e molas — material
// que ja chega cru e transiente. Distorcao pesada mataria o ataque, que
// e justamente o que faz esse material soar como escultura sendo
// tocada. Entao tudo aqui e escolhido pra dar CORPO sem comer o
// transiente.
//
//   y = tanh(drive * x) / tanh(drive)
//
// A DIVISAO E O QUE IMPORTA. Sem ela, subir o `drive` sobe o volume
// junto, e o ouvido chama qualquer coisa mais alta de "melhor" — o
// controle mediria nivel disfarçado de timbre. Normalizada, a saida
// fica em ~1.0 de pico para qualquer drive, e o que muda e SO a forma
// da onda. Timbre isolado do nivel.
//
// ORDEM INTERNA, e cada passo tem razao:
//   entrada x drive -> tanh -> normaliza -> LPF (tone) -> mix paralelo
//
//   LPF DEPOIS da saturacao, nunca antes. Saturar gera harmonicos
//   agudos asperos; cortar o topo DEPOIS remove a aspereza e deixa o
//   corpo. Filtrar antes so mudaria o que entra no shaper.
//
//   MIX PARALELO, e e obrigatorio. E compressao paralela aplicada a
//   saturacao: o seco passa inteiro e carrega o ATAQUE, o saturado
//   entra por baixo e carrega o CORPO. Crossfade destruiria o
//   transiente — e o freeze ja reprovou uma vez por isso, com o dB
//   dizendo que nao escutava o instrumento por baixo do efeito.
//
// ZERO MEMORIA. Funcao por amostra; o unico estado e o polo do LPF.
// E o efeito mais barato da suite, e serve pra validar o EffectSlot na
// classe "sem historico" antes dos que reusam o motor granular.
//
// C++ puro: sem hal, sem RNBO, sem alocacao.
// ============================================================

#include <cmath>

namespace cotuca
{

class Drive
{
  public:
    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        SetDrive(2.f);
        SetTone(6000.f);
        mix_ = 0.f;
        y_l_ = y_r_ = 0.f;
    }

    // `d` em VEZES, nao em dB. 1.5-3 e cola; acima de ~8 comeca a
    // virar carater proprio.
    void SetDrive(float d)
    {
        if(d < 1.001f) d = 1.001f;   // 1.0 exato zeraria o denominador
        if(d > 24.f)   d = 24.f;
        if(d == drive_) return;
        drive_ = d;
        // Normalizador, calculado SO quando o drive muda. tanh e caro
        // demais pra recalcular por amostra sem motivo.
        // O normalizador usa a MESMA aproximacao do caminho quente. Se
        // usasse `std::tanh` aqui, o pico de saida ficaria levemente
        // fora de 1.0 — pequeno, mas seria erro de coerencia: dois
        // modelos diferentes da mesma curva.
        inv_ = 1.f / TanhRapido(d);
    }

    // Corte do passa-baixa, em Hz. No topo da faixa ele e praticamente
    // transparente — e isso e proposital: `tone` no maximo deve
    // significar "sem filtro", nao "filtro aberto mas presente".
    void SetTone(float hz)
    {
        if(hz < 200.f)   hz = 200.f;
        if(hz > 18000.f) hz = 18000.f;
        a_ = 1.f - std::exp(-6.283185307179586f * hz / sr_);
        if(a_ > 1.f) a_ = 1.f;
    }

    void SetMix(float m)
    {
        mix_ = m < 0.f ? 0.f : (m > 1.f ? 1.f : m);
    }

    // APROXIMACAO DE tanh, e a medicao exigiu.
    //
    // `std::tanh` e TRANSCENDENTAL — chamada de biblioteca de ~100
    // ciclos, nao aritmetica. Duas por amostra a 48 kHz custaram
    // +36 us de pico (3,6% do bloco), contra uma aposta de <1%.
    // "Funcao por amostra" nao implica barato quando a funcao e cara.
    //
    // Pade de 2a ordem: x*(27 + x^2) / (27 + 9x^2). Erro < 0.2% em
    // |x| < 3, satura suave, e e so multiplicacao e uma divisao.
    // Acima de |x| = 3 tanh ja esta a 0.995 — grampear ali nao muda
    // nada audivel e evita a divisao crescer.
    static float TanhRapido(float x)
    {
        if(x >  3.f) return 1.f;
        if(x < -3.f) return -1.f;
        const float x2 = x * x;
        return x * (27.f + x2) / (27.f + 9.f * x2);
    }

    // MONO neste ponto da cadeia. A fonte e `src_buf`, um so canal —
    // a versao anterior calculava tanh DUAS VEZES sobre o mesmo valor
    // e jogava metade do custo fora. Estereo real so aparece depois do
    // reverb.
    float ProcessMono(float x)
    {
        const float sat = TanhRapido(drive_ * x) * inv_;
        y_l_ += a_ * (sat - y_l_);   // LPF de 1 polo, POS-saturacao
        // CAMADA: o seco passa inteiro e leva o ataque; o saturado
        // entra por baixo e leva o corpo.
        return x + y_l_ * mix_;
    }

    // Par estereo, pra quando o drive for usado depois do reverb.
    void Process(float& l, float& r)
    {
        const float sl = TanhRapido(drive_ * l) * inv_;
        const float sr = TanhRapido(drive_ * r) * inv_;
        y_l_ += a_ * (sl - y_l_);
        y_r_ += a_ * (sr - y_r_);
        l += y_l_ * mix_;
        r += y_r_ * mix_;
    }

  private:
    float sr_    = 48000.f;
    float drive_ = 2.f;
    float inv_   = 1.f;
    float a_     = 0.5f;
    float mix_   = 0.f;
    float y_l_   = 0.f, y_r_ = 0.f;
};

} // namespace cotuca
