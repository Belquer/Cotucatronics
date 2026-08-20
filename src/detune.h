#pragma once
// ============================================================
// Cotucatronics OS — detune v0.1.0
// Vozes micro-desafinadas + largura estereo.
//
// O UNICO EFEITO QUE REALMENTE COMPARTILHA O MOTOR DE PITCH — e o caso
// que a arquitetura previu desde o inicio: duas vozes lendo o MESMO
// historico, em razoes diferentes. Um buffer, dois consumidores.
//
//   razao = 2^(cents/1200)
//   +10 cents = 1.005793 | -10 cents = 0.994260
//
// AS DUAS VOZES VAO PRA LADOS OPOSTOS. Detune mono soa como
// desafinacao (dois tons brigando pelo mesmo lugar); detune com
// largura soa como CORO — e a diferenca esta inteira no fato de os
// dois nao disputarem o mesmo ponto do estereo.
//
// POSICAO NA CADEIA: depois do modulo, onde `out[0]`/`out[1]` existem.
// Antes disso a cadeia e um canal so, e sem estereo nao ha spread.
//
// LEITURA MONO das duas vozes (PitchEngine::ReadMono), panoramizadas
// aqui. Metade do custo de duas leituras estereo, e o resultado e mais
// largo — porque o pan e nosso, nao herdado da fonte.
//
// ------------------------------------------------------------
// LIMITE CONHECIDO, DECLARADO ANTES DE ALGUEM DESCOBRIR TOCANDO:
//
// O motor e granular com posicao SORTEADA. Isso mata comb filtering
// (por isso ele existe assim) mas troca por um leve borrao: dois graos
// descorrelacionados somam em potencia, nunca em fase. Em material
// PERCUSSIVO e de banda larga isso e inaudivel ou desejavel.
//
// Em NOTA SUSTENTADA E TONAL — kalimba deixada soando, monocordio
// longo — o borrao aparece como um chorus a mais, alem do detune
// pedido. Nao e defeito: e o regime em que a janela de potencia
// constante trabalha pior, e ja apareceu quando um seno puro foi usado
// como regua de medicao do motor.
//
// Se incomodar, o caminho e grao mais longo (menos costuras por
// segundo), nao menos sorteio — sorteio menor traz o pente de volta.
// ------------------------------------------------------------
//
// C++ puro. O motor vem de fora.
// ============================================================

#include <cmath>

namespace cotuca
{

class Detune
{
  public:
    void Init(float /*sample_rate*/)
    {
        cents_ = 0.f; spread_ = 0.f; mix_ = 0.f; duas_ = true;
    }

    void SetCents(float c)
    {
        if(c < 0.f)  c = 0.f;
        if(c > 50.f) c = 50.f;
        cents_ = c;
    }
    void SetVoices(int n)  { duas_ = (n >= 2); }
    void SetSpread(float s){ spread_ = Clamp01(s); }
    void SetMix(float m)   { mix_ = Clamp01(m); }

    float RatioA() const { return std::pow(2.f,  cents_ / 1200.f); }
    float RatioB() const { return std::pow(2.f, -cents_ / 1200.f); }
    bool  Duas()   const { return duas_; }

    // `a` e `b` sao as leituras MONO das duas vozes. Soma por cima do
    // seco — CAMADA, como drive e shimmer. O detune engrossa; nao
    // substitui.
    void Mistura(float a, float b, float& l, float& r) const
    {
        // Pan por lei de potencia constante: as duas vozes se afastam
        // sem que a soma mude de nivel no caminho.
        const float g  = 0.5f + 0.5f * spread_;   // 0.5 = centro, 1 = ponta
        const float h  = 1.f - g;
        const float m  = mix_;
        l += (a * g + b * h) * m;
        r += (a * h + b * g) * m;
    }

  private:
    static float Clamp01(float v)
    {
        return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    }
    float cents_  = 0.f;
    float spread_ = 0.f;
    float mix_    = 0.f;
    bool  duas_   = true;
};

} // namespace cotuca
