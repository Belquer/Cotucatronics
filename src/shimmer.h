#pragma once
// ============================================================
// Cotucatronics OS — shimmer v0.3.0
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
        SetTone(0.f);
        tl_ = tr_ = 0.f;
        fb_l_ = fb_r_ = 0.f;
        z_l_  = z_r_  = 0.f;
    }

    void SetMix(float m)      { mix_ = Clamp01(m); }

    // TOM: passa-baixa de um polo no sinal ja pitcheado, aplicado ANTES
    // da realimentacao E antes da camada direta — as duas coisas que o
    // ouvido soma. Um polo de proposito: o que se quer e INCLINAR o
    // espectro, nao cortar; corte ingreme deixa buraco audivel.
    //
    // 0 = 6 kHz (quente, o default) ... 1 = 18 kHz (aberto).
    // A ESCALA CORRE PRA CIMA porque todo param nasce em 0 e 0 tem que
    // ser o estado BOM. Um "tom" em que 0 significasse o mais brilhante
    // reproduziria a aspereza a cada boot — a regra do default zero
    // existe pra desligar efeito, nao pra escolher o pior timbre.
    //
    // Medido na cauda (com anti-alias ligado no motor):
    //   sem tom  centroide 4300 Hz | 6-12k 22.2% | medio 38.6%
    //   6 kHz    centroide 2763 Hz | 6-12k  8.3% | medio 55.2%
    // ------------------------------------------------------------
    // POR QUE UM SINO, E NAO SO UM PASSA-BAIXA. A primeira versao era
    // so corte em 6 kHz. MEDIDO: derrubou o que estava ACIMA de 6 kHz
    // (de 28.2% para 12.4%) e deixou 2-6 kHz intacto — pior, a faixa
    // virou uma FATIA MAIOR do total, de 26.4% para 35.7%. O dB tinha
    // dito "agudas E MEDIO-agudas"; metade da queixa nao foi tocada.
    //
    // E nao e ruido: a planura espectral de 2-6 kHz na cauda e 0.009,
    // tao tonal quanto a cauda seca (0.004). E som legitimo — o 1-3 kHz
    // da escultura levado pra 2-6 kHz pela oitava e sustentado pelo
    // reverb, bem no pico de sensibilidade do ouvido. Contra som
    // legitimo no lugar errado nao se usa filtro de ruido: usa-se EQ.
    //
    // ALVO MEDIDO, nao escolhido de ouvido: a cauda SECA tem razao
    // (2-6 kHz)/(300-2000 Hz) = 0.28 — o timbre natural do instrumento
    // na sala. O shimmer sem sino da 0.71. Com -6 dB em 3 kHz da 0.19,
    // um pouco mais macio que o natural, que e o que "suave" pede.
    //
    // 0 = macio (sino -6 dB, corte 6 kHz) ... 1 = aberto (sem sino,
    // corte 18 kHz). Sobe pra clarear: 0 tem que ser o estado BOM, ou
    // a aspereza volta a cada boot.
    // ------------------------------------------------------------
    void SetTone(float norm)
    {
        norm = Clamp01(norm);
        const float hz = 6000.f * std::pow(3.f, norm);
        tom_a_ = std::exp(-6.283185307179586f * hz / sr_);
        sino_.Ajusta(kSinoHz, sr_, kSinoQ, -6.f * (1.f - norm));
    }

    void Tom(float& l, float& r)
    {
        tl_ = l * (1.f - tom_a_) + tl_ * tom_a_;  l = tl_;
        tr_ = r * (1.f - tom_a_) + tr_ * tom_a_;  r = tr_;
        l = sino_.Process(l, sz_l_);
        r = sino_.Process(r, sz_r_);
    }
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
    // Sino (peaking EQ). Coeficientes UNICOS, estado por canal: o
    // filtro e o mesmo nos dois lados, so a memoria e que nao pode ser.
    static constexpr float kSinoHz = 3000.f;
    static constexpr float kSinoQ  = 1.0f;
    struct Sino
    {
        float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
        bool  reto = true;   // 0 dB = passa direto, sem custo

        void Ajusta(float hz, float sr, float q, float db)
        {
            reto = (db > -0.05f);
            if(reto) return;
            const float A  = std::pow(10.f, db / 40.f);
            const float w  = 6.283185307179586f * hz / sr;
            const float cw = std::cos(w);
            const float al = std::sin(w) / (2.f * q);
            const float a0 = 1.f + al / A;
            b0 = (1.f + al * A) / a0;
            b1 = (-2.f * cw) / a0;
            b2 = (1.f - al * A) / a0;
            a1 = (-2.f * cw) / a0;
            a2 = (1.f - al / A) / a0;
        }
        float Process(float x, float* z) const
        {
            if(reto) return x;
            const float y = b0 * x + z[0];
            z[0] = b1 * x - a1 * y + z[1];
            z[1] = b2 * x - a2 * y;
            return y;
        }
    };
    Sino  sino_;
    float sz_l_[2] = {0.f, 0.f};
    float sz_r_[2] = {0.f, 0.f};
    float tom_a_ = 0.f;
    float tl_ = 0.f, tr_ = 0.f;
    float fb_l_ = 0.f, fb_r_ = 0.f;
};

} // namespace cotuca
