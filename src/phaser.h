#pragma once
// ============================================================
// Cotucatronics OS — phaser v0.3.0
// N allpass de 1a ordem em serie, varridos por LFO, com feedback.
//
//   allpass:  y[n] = -a*x[n] + x[n-1] + a*y[n-1]
//
// OS SINAIS IMPORTAM, e a primeira versao os tinha trocados:
// `y = a*x + x1 - a*y1` da H(z) = (a + z^-1)/(1 + a*z^-1), cujo POLO
// fica em z = -a — perto de NYQUIST. Com a ~ 0.97 (calculado pensando
// em 200 Hz) a acao de fase acontecia toda perto de 24 kHz, e os
// notches nasciam fora do espectro audivel.
//
// A forma certa poe o polo em z = +a, perto de DC:
//   H(z) = (z^-1 - a)/(1 - a*z^-1)
//   DC:      H = (1-a)/(1-a) = 1   fase 0
//   Nyquist: H = (-1-a)/(1+a) = -1  fase 180 graus
//
// A conta do COEFICIENTE estava certa o tempo todo; a equacao de
// diferencas e que nao correspondia a ela. Sintoma: excursao de nivel
// de 0,1 dB com o notch varrendo — "parece um tremolo bem sutil".
//   saida  = mix(seco, cadeia_com_feedback)
//
// ZERO BUFFER. O unico estado sao dois floats por estagio — nada de
// linha de atraso. Mas ATENCAO A REGUA DO DRIVE: trabalho nao e
// memoria. Sao N multiplicacoes e somas por amostra, e com 8 estagios
// estereo isso e 32 operacoes por amostra. Barato, nao gratis.
//
// SEM TRANSCENDENTAL NO CAMINHO DE AUDIO. O LFO e o mapeamento
// frequencia->coeficiente rodam UMA VEZ POR BLOCO, nao por amostra —
// que foi exatamente o erro que fez o drive custar 36 us em vez de 21.
//
// ------------------------------------------------------------
// O MIX E INVERTIDO EM RELACAO AOS OUTROS EFEITOS. Leia antes de
// "consertar".
//
// Nos outros (drive, shimmer, freeze) o mix e CAMADA: 0 = so seco,
// 1 = efeito no maximo. Aqui nao pode ser, e a razao e o mecanismo:
//
//   O phaser nao ADICIONA nada audivel. Uma cadeia de allpass tem
//   MAGNITUDE PLANA — ela so gira fase. Sozinha, e praticamente
//   inaudivel. O que se ouve sao os NOTCHES, e notch e CANCELAMENTO
//   entre o seco e o desviado.
//
//   Cancelamento maximo exige amplitudes IGUAIS. Entao:
//     mix 0.5  -> notch profundo   <- o "ligado"
//     mix 1.0  -> so allpass, SEM notch, quase inaudivel
//     mix 0.0  -> so seco
//
// Ou seja: o controle e uma montanha, nao uma rampa. E o mesmo tipo de
// dependencia declarada do shimmer com o `rev_mix` — comportamento que
// surpreende se nao estiver escrito, e que nao e defeito.
// ------------------------------------------------------------
//
// FAIXA DE VARREDURA E O QUE DECIDE SE SOA LIQUIDO OU FINO. Notch que
// so passeia no agudo produz "telefone"; o corpo do som do dB tem
// grave, entao a varredura cobre 200 Hz a 2 kHz — os medio-graves onde
// a kalimba, o monocordio e as molas vivem.
//
// C++ puro: sem hal, sem motor externo, sem alocacao.
// ============================================================

#include <cmath>

namespace cotuca
{

class Phaser
{
  public:
    static constexpr int kMaxStages = 8;

    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        for(int i = 0; i < kMaxStages; i++)
            xs_l_[i] = ys_l_[i] = xs_r_[i] = ys_r_[i] = 0.f;
        fb_l_ = fb_r_ = 0.f;
        fase_ = 0.f;
        SetStages(6);
        SetRate(0.3f);
        SetDepth(1.f);
        SetFeedback(0.f);
        SetMix(0.f);
        AtualizaLfo(0);   // deixa `a_` valido antes do primeiro bloco
    }

    void SetStages(int n)
    {
        // Pares, porque cada PAR de allpass produz um notch. Numero
        // impar daria meio notch — audivel como assimetria, nao como
        // riqueza.
        if(n < 2) n = 2;
        if(n > kMaxStages) n = kMaxStages;
        stages_ = n & ~1;
    }

    void SetRate(float hz)
    {
        if(hz < 0.02f) hz = 0.02f;
        if(hz > 8.f)   hz = 8.f;
        rate_ = hz;
    }

    void SetDepth(float d) { depth_ = Clamp01(d); }

    void SetFeedback(float f)
    {
        // TRAVADO EM 0.9. Acima disso a cadeia auto-oscila: o laco
        // passa a ter ganho >= 1 na frequencia do notch e o phaser vira
        // oscilador. O governor a jusante seguraria o nivel, mas o som
        // deixaria de ser phaser.
        f = Clamp01(f);
        fb_ = f * 0.9f;
    }

    void SetMix(float m) { mix_ = Clamp01(m); }

    // UMA VEZ POR BLOCO. Aqui moram o LFO e a unica transcendental do
    // efeito — mante-la fora do caminho de amostra e o que torna o
    // phaser mais barato que o drive, apesar de fazer mais contas.
    void AtualizaLfo(size_t block_size)
    {
        fase_ += rate_ * (float)block_size / sr_;
        if(fase_ >= 1.f) fase_ -= (float)(int)fase_;

        // TRIANGULAR, nao senoidal. No phaser a triangular varre a
        // frequencia em velocidade constante, e o ouvido le isso como
        // movimento parelho; a senoidal desacelera nos extremos e soa
        // como se "parasse" no topo e no fundo da varredura.
        const float tri = fase_ < 0.5f ? (fase_ * 2.f) : (2.f - fase_ * 2.f);
        const float d   = 0.5f - 0.5f * depth_;   // depth encolhe pro centro
        const float u   = d + tri * (1.f - 2.f * d);

        // Mapeamento LOGARITMICO — o ouvido ouve frequencia em oitavas,
        // e varredura linear passaria correndo pelo grave e se
        // arrastando no agudo.
        const float fc = kFMin * std::exp(u * kLogSpan);

        // Coeficiente do allpass de 1a ordem para polo em `fc`.
        const float t = std::tan(3.14159265358979f * fc / sr_);
        a_ = (1.f - t) / (1.f + t);
    }

    // MONO — e este e o caminho que a cadeia usa antes do reverb.
    //
    // A fonte ali e `src_buf`, um canal so. Chamar o par estereo
    // processaria 2N allpass em vez de N sobre o MESMO valor: metade do
    // trabalho jogada fora. Custou 33 us em vez de ~17 na primeira
    // medicao — e e o mesmo erro do drive, cometido no efeito seguinte
    // ao conserto. A licao ali foi tratada como instancia; era CLASSE.
    float ProcessMono(float x)
    {
        float v = x + fb_l_ * fb_;
        for(int i = 0; i < stages_; i++)
        {
            const float y = -a_ * v + xs_l_[i] + a_ * ys_l_[i];
            xs_l_[i] = v; ys_l_[i] = y; v = y;
        }
        fb_l_ = v;
        // CROSSFADE de proposito — ver a nota do mix no cabecalho.
        return x * (1.f - mix_) + v * mix_;
    }

    // Par estereo, pra quando o efeito for usado depois do reverb.
    void Process(float& l, float& r)
    {
        float xl = l + fb_l_ * fb_;
        float xr = r + fb_r_ * fb_;
        for(int i = 0; i < stages_; i++)
        {
            const float yl = -a_ * xl + xs_l_[i] + a_ * ys_l_[i];
            xs_l_[i] = xl; ys_l_[i] = yl; xl = yl;
            const float yr = -a_ * xr + xs_r_[i] + a_ * ys_r_[i];
            xs_r_[i] = xr; ys_r_[i] = yr; xr = yr;
        }
        fb_l_ = xl;
        fb_r_ = xr;
        // CROSSFADE, e de proposito — ver a nota do mix no cabecalho.
        // O notch nasce do cancelamento entre seco e desviado, e
        // cancelamento maximo exige amplitudes iguais.
        l = l * (1.f - mix_) + xl * mix_;
        r = r * (1.f - mix_) + xr * mix_;
    }

  private:
    // 200 Hz a 2 kHz: os medio-graves onde o corpo do material vive.
    // Varredura so no agudo e o que faz phaser soar "telefone".
    static constexpr float kFMin    = 200.f;
    static constexpr float kLogSpan = 2.302585f;  // ln(2000/200)

    static float Clamp01(float v)
    {
        return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    }

    float sr_    = 48000.f;
    int   stages_ = 6;
    float rate_  = 0.3f;
    float depth_ = 1.f;
    float fb_    = 0.f;
    float mix_   = 0.f;
    float fase_  = 0.f;
    float a_     = 0.f;
    float xs_l_[kMaxStages] = {}, ys_l_[kMaxStages] = {};
    float xs_r_[kMaxStages] = {}, ys_r_[kMaxStages] = {};
    float fb_l_ = 0.f, fb_r_ = 0.f;
};

} // namespace cotuca
