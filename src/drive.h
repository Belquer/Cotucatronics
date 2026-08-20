#pragma once
// ============================================================
// Cotucatronics OS — drive v0.1.0
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
        inv_ = 1.f / std::tanh(d);
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

    // Um par estereo, in-place. Sem ramo condicional no caminho quente:
    // com mix 0 o resultado ja e o seco, e quem decide pular o bloco
    // inteiro e o EffectSlot.
    void Process(float& l, float& r)
    {
        const float sl = std::tanh(drive_ * l) * inv_;
        const float sr = std::tanh(drive_ * r) * inv_;
        // LPF de 1 polo, POS-saturacao.
        y_l_ += a_ * (sl - y_l_);
        y_r_ += a_ * (sr - y_r_);
        // CAMADA: o seco passa inteiro e leva o ataque; o saturado
        // entra por baixo e leva o corpo.
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
