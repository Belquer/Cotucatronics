#pragma once
// ============================================================
// Cotucatronics OS — delay v0.2.0
// Reescrita em C++ do patch `cotucatronics_delay`, que era o ULTIMO
// consumidor do runtime do motor externo no instrumento.
//
// O GRAFO DO PATCH, LIDO DO EXPORT E REPRODUZIDO AQUI:
//
//   seco     = x * (1 - mix)
//   entra    = x + realimentacao
//   atrasado = linha(entra, tempo)
//   saida    = atrasado * mix + seco
//   realim.  = atrasado * 0.4
//
// FEEDBACK FIXO EM 0.4, e nao e escolha minha: e o valor que estava no
// patch (`dspexpr_01_in2 = 0.4`), que nao o expunha como parametro. O
// delay foi certificado de ouvido no T5 COM esse numero — mudar aqui
// seria trocar o caracter aprovado por baixo do pano.
//
// TEMPO: 50 + t * 950 ms, LINEAR. O patch fazia 10..1000 e o manifest
// cortava a faixa por baixo em 0.05 pra chegar em ~59 ms, porque abaixo
// disso o ouvido nao separa em eco, so espessa.
//
// PISO DE 50 ms AGORA MORA AQUI (decisao do dB, 2026-08-20), e a cerca sai
// da tabela de params. Isso devolve o CURSO INTEIRO ao fader: `dly_time`
// volta a ser 0..1 de ponta a ponta, e 0 ja e um tempo util. Cerca no
// EFEITO vale pra qualquer manifest que o use; cerca no PARAM valia so
// pra este instrumento.
//
// CROSSFADE, NAO CAMADA. `saida = atrasado*mix + seco*(1-mix)` e o que
// o patch fazia, e fica assim de proposito: os efeitos da suite sao
// camada, o delay nao e, e a diferenca e herdada, nao esquecida.
//
// ------------------------------------------------------------
// DUAS DIFERENCAS DELIBERADAS, DECLARADAS:
//
// 1. REALIMENTACAO POR AMOSTRA. O `feedback~` do patch quebra o ciclo
//    com um bloco de atraso, entao la o laco fechava com delay + 1
//    bloco (~1 ms). Aqui fecha por amostra. Diferenca inaudivel contra
//    59-1000 ms de delay, e e o comportamento natural em C++ — mas
//    esta escrito porque "reproduzi fielmente" precisa dizer onde nao.
//
// 2. TEMPO SUAVIZADO. O patch trocava o atraso em degrau, por bloco.
//    Varrer o `dly_time` produzia descontinuidade. Aqui o atraso e
//    interpolado por amostra, o que da a deriva de fita ao varrer —
//    mesma fisica do warble, e desejavel num delay.
// ------------------------------------------------------------
//
// C++ puro. Buffer vem de fora (1 s a 48 kHz = 192 KB, SDRAM).
// ============================================================

#include <cstdint>   // o contador do clamp
#include <cstddef>
#include <cmath>

namespace cotuca
{

class Delay
{
  public:
    // O patch nao expunha isto. Esta aqui como CONSTANTE NOMEADA em vez
    // de literal solto pra que quem for mexer saiba que esta mexendo no
    // que foi aprovado de ouvido.
    static constexpr float kFeedback = 0.4f;

    // Piso e teto do tempo, em segundos. O piso e a borda da percepcao
    // de eco: abaixo dele o ouvido nao separa as repeticoes, so escuta
    // o som engrossar — curso de fader que nao faz nada audivel.
    static constexpr float kMinSeg = 0.050f;
    static constexpr float kMaxSeg = 1.000f;

    static size_t FloatsNeeded(float sample_rate, float max_seg)
    {
        return (size_t)(sample_rate * max_seg) + 4u;
    }

    bool Init(float sample_rate, float* mem, size_t n_floats, float max_seg)
    {
        if(!mem || sample_rate <= 0.f || max_seg <= 0.f) return false;
        if(n_floats < FloatsNeeded(sample_rate, max_seg)) return false;
        sr_   = sample_rate;
        cap_  = (size_t)(sample_rate * max_seg);
        buf_  = mem;
        for(size_t i = 0; i < cap_; i++) buf_[i] = 0.f;
        write_ = 0;
        // Nasce no tempo minimo, JA ASSENTADO: a suavizacao existe pra
        // varredura, nao pra estreia. Um delay que rampa do zero no
        // primeiro bloco varreria a faixa inteira audivelmente.
        atraso_ = alvo_ = kMinSeg * sr_;
        return true;
    }

    // `t` em 0..1, como o param. O mapeamento e do patch.
    void SetTime(float t)
    {
        if(t < 0.f) t = 0.f;
        if(t > 1.f) t = 1.f;
        float n = (kMinSeg + t * (kMaxSeg - kMinSeg)) * sr_;
        const float teto = (float)(cap_ - 2u);
        if(n > teto) n = teto;
        if(n < 1.f)  n = 1.f;
        alvo_ = n;
    }

    void SetMix(float m)
    {
        mix_ = m < 0.f ? 0.f : (m > 1.f ? 1.f : m);
    }

    float ProcessMono(float x)
    {
        if(cap_ == 0) return x;
        // Suavizacao do ATRASO, nao do sinal. Um polo a ~20 ms.
        atraso_ += (alvo_ - atraso_) * kSuave;

        float r = (float)write_ - atraso_;
        // UMA soma basta: `atraso_` e grampeado em [1, teto<=cap_] e
        // `write_ < cap_`, entao r vive em (-cap_, cap_).
        if(r < 0.f) r += (float)cap_;

        // CLAMP DURO, e ele fecha DUAS falhas que o `while` antigo
        // deixava abertas. `while(r < 0.f) r += cap_` era refem do
        // valor de `atraso_`:
        //
        //   atraso_ = +Inf  ->  r = -Inf  ->  -Inf + cap_ = -Inf
        //                       o laco NUNCA TERMINA. Loop morto, e o
        //                       audio segue rodando: placa "viva e
        //                       muda", que e o sintoma mais caro deste
        //                       instrumento.
        //   atraso_ = NaN   ->  `NaN < 0` e falso, sai na hora, e
        //                       `(size_t)NaN` e INDEFINIDO — indexa
        //                       fora do buffer em silencio.
        //
        // A forma NEGADA pega NaN de graca: qualquer comparacao com
        // NaN e falsa, entao a negacao e verdadeira e o clamp morde.
        // `r >= 0 && r < cap_` escrito direto deixaria o NaN passar.
        if(!(r >= 0.f && r < (float)cap_))
        {
            r = 0.f;
            if(clamps_ < 0xFFFFFFFFu) clamps_++;
        }
        const size_t i0 = (size_t)r;
        const size_t i1 = (i0 + 1u >= cap_) ? 0u : i0 + 1u;
        const float  f  = r - (float)i0;
        const float atrasado = buf_[i0] + f * (buf_[i1] - buf_[i0]);

        buf_[write_] = x + atrasado * kFeedback;
        if(++write_ >= cap_) write_ = 0;

        return atrasado * mix_ + x * (1.f - mix_);
    }

    // DEFESA SEM CONTADOR E HIPOTESE. Se o clamp morder, alguem tem
    // que poder ver — senao "nunca aconteceu" e indistinguivel de
    // "acontece toda amostra e ninguem soube".
    uint32_t Clamps() const { return clamps_; }

  private:
    uint32_t clamps_ = 0;
    static constexpr float kSuave = 0.001f;   // ~20 ms a 48 kHz

    float* buf_    = nullptr;
    size_t cap_    = 0;
    size_t write_  = 0;
    float  sr_     = 48000.f;
    float  atraso_ = 480.f;   // amostras, suavizado
    float  alvo_   = 480.f;
    float  mix_    = 0.f;
};

} // namespace cotuca
