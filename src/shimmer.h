#pragma once
// ============================================================
// Cotucatronics OS — shimmer v0.1.0
// Reverb + pitch-up num laco de realimentacao. Eno/Lanois.
//
// NAO TEM REVERB PROPRIO, E ISSO E O DESENHO. Ele realimenta o
// Freeverb que ja existe na cadeia — decisao do dB, e a mais fiel ao
// original, onde o shimmer E a sala se empilhando. Custo: zero memoria
// nova e ~zero CPU alem do pitch.
//
//   entrada_reverb  = seco + realimentacao
//   cauda           = saida do reverb
//   realimentacao   = feedback * PassaAlta200( Pitch(cauda) )
//   saida           = seco + reverb * mix        <- CAMADA, nao crossfade
//
// O PASSA-ALTA E INEGOCIAVEL. Cada volta empilha uma oitava ACIMA, mas
// tambem reinjeta tudo o que veio antes — e o grave se acumula porque
// nada o remove. Em ~3 voltas vira lama. 200 Hz e o corte que deixa a
// oitava subir e nao deixa o fundamental voltar.
//
// MIX E CAMADA. A especificacao trazia a formula de crossfade
// (`seco*(1-mix) + rev*mix`) e a anotacao "mix = CAMADA, vivo sempre
// presente" — que sao coisas opostas. Vale a anotacao: o freeze ja
// reprovou na etapa 9 por crossfade, com o dB dizendo "quando o frz
// esta rolando nao escuto o instrumento direito". Congelar e empilhar
// oitavas sao gestos que SOMAM a uma escultura que continua soando.
//
// FEEDBACK E DECAY SAO INDEPENDENTES, e a distincao e do dB:
//   feedback = QUANTO da cauda sobe de novo (quantas oitavas empilham)
//   decay    = QUANTO DURA cada degrau (e o tempo do proprio reverb)
// Nao acoplar: a interacao entre os dois e o playground.
//
// C++ puro. O reverb e o motor de pitch vem de fora — este arquivo nao
// aloca nada e nao conhece o hal.
// ============================================================

#include <cstddef>
#include <cmath>

namespace cotuca
{

class Shimmer
{
  public:
    // Teto de realimentacao. Acima disso o laco vira avalanche em
    // poucos segundos: cada volta soma uma oitava E o que ja estava la,
    // entao o ganho de laco efetivo e maior que `feedback` sozinho.
    // 0.85 e a beira; a beira-do-descontrole fica pra refinamento
    // futuro, com o governor ja a jusante como rede.
    static constexpr float kFeedbackMax = 0.85f;

    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        SetHpf(200.f);
        fb_l_ = fb_r_ = 0.f;
        z_l_  = z_r_  = 0.f;
    }

    void SetMix(float m)      { mix_ = Clamp01(m); }
    void SetFeedback(float f)
    {
        f = Clamp01(f);
        fb_ = f * kFeedbackMax;
    }

    // Passa-alta de 1a ordem no caminho de realimentacao. Uma ordem
    // basta: o que se quer e impedir ACUMULO, nao esculpir timbre — e
    // cada volta aplica o filtro de novo, entao a atenuacao no grave
    // se compoe sozinha volta a volta.
    void SetHpf(float hz)
    {
        if(hz < 20.f)   hz = 20.f;
        if(hz > 2000.f) hz = 2000.f;
        const float x = std::exp(-6.283185307179586f * hz / sr_);
        a_ = x;
    }

    // O que INJETAR na entrada do reverb neste sample. Chamado ANTES
    // de reverb.Process().
    void Realimenta(float pitch_l, float pitch_r, float& add_l, float& add_r)
    {
        // Passa-alta: y = x - media_movel(x). A media movel e o grave.
        z_l_ = a_ * z_l_ + (1.f - a_) * pitch_l;
        z_r_ = a_ * z_r_ + (1.f - a_) * pitch_r;
        add_l = fb_ * (pitch_l - z_l_);
        add_r = fb_ * (pitch_r - z_r_);
    }

    // Mistura a saida do reverb por cima do seco. CAMADA: o seco passa
    // inteiro, sempre.
    void Mistura(float rev_l, float rev_r, float& out_l, float& out_r) const
    {
        out_l += rev_l * mix_;
        out_r += rev_r * mix_;
    }

    float Mix() const { return mix_; }

  private:
    static float Clamp01(float v)
    {
        return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    }

    float sr_  = 48000.f;
    float mix_ = 0.f;
    float fb_  = 0.f;
    float a_   = 0.97f;   // coeficiente do passa-alta
    float z_l_ = 0.f, z_r_ = 0.f;
    float fb_l_ = 0.f, fb_r_ = 0.f;
};

} // namespace cotuca
