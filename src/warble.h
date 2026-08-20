#pragma once
// ============================================================
// Cotucatronics OS — warble v0.1.0
// Deriva de fita: delay curto modulado por LFO lento e irregular.
//
// NAO USA O MOTOR DE PITCH, e isso e correcao de premissa. O desenho
// original listava warble entre os consumidores do motor granular —
// mas fita nao desloca altura por grao: ela desloca porque a DISTANCIA
// entre cabeca e fita varia. Um delay curto com tempo modulado
// reproduz exatamente isso por Doppler:
//
//   atraso cresce  -> leitura desacelera -> altura CAI
//   atraso encolhe -> leitura acelera    -> altura SOBE
//
// E mais liso que granular (nao ha costura entre graos), mais barato
// (uma leitura interpolada por amostra em vez de duas com janela), e
// mais fiel — o artefato de fita e continuo, nao granular.
//
// A IRREGULARIDADE E O EFEITO. LFO senoidal puro soa como vibrato de
// sintetizador; fita real tem wow (lento, ~0,5-2 Hz) somado a flutter
// (rapido, irregular) e a deriva mecanica que nunca repete. O param
// `irreg` cruza entre periodico e passeio aleatorio suavizado.
//
// MONO DESDE A PRIMEIRA VERSAO. Antes do reverb a cadeia e um canal
// so; drive e phaser nasceram estereo e pagaram o dobro ate serem
// corrigidos — a licao foi tratada como instancia duas vezes antes de
// virar classe.
//
// C++ puro. Buffer vem de fora.
// ============================================================

#include <cstddef>
#include <cstdint>
#include <cmath>

namespace cotuca
{

class Warble
{
  public:
    // Floats para `ms` de historico MONO. ~40 ms cobre o atraso central
    // de 20 ms mais a excursao maxima com folga.
    static size_t FloatsNeeded(float sample_rate, float ms)
    {
        return (size_t)(sample_rate * ms * 0.001f) + 4u;
    }

    bool Init(float sample_rate, float* mem, size_t n_floats, float ms)
    {
        if(!mem || sample_rate <= 0.f) return false;
        if(n_floats < FloatsNeeded(sample_rate, ms)) return false;
        sr_  = sample_rate;
        cap_ = FloatsNeeded(sample_rate, ms);
        buf_ = mem;
        for(size_t i = 0; i < cap_; i++) buf_[i] = 0.f;
        write_ = 0;
        fase_  = 0.f;
        alvo_  = 0.f;
        passeio_ = 0.f;
        rng_   = 0x2545F491u;
        centro_ = 0.020f * sr_;      // 20 ms
        SetRate(0.6f);
        SetDepth(0.f);
        SetIrreg(0.5f);
        atraso_ = centro_;
        return true;
    }

    void SetRate(float hz)
    {
        if(hz < 0.05f) hz = 0.05f;
        if(hz > 3.f)   hz = 3.f;
        rate_ = hz;
    }

    // Excursao do atraso, em MILISSEGUNDOS de desvio. O desvio de
    // altura resultante e ~2*pi*rate*depth — entao 1 ms a 0,6 Hz da
    // ~4 cents (deriva quase imperceptivel, o "wow" de fita boa), e
    // 3 ms a 2 Hz da ~65 cents (fita estragada, e as vezes e o que se
    // quer).
    void SetDepth(float ms)
    {
        if(ms < 0.f) ms = 0.f;
        if(ms > 3.f) ms = 3.f;
        depth_ = ms * 0.001f * sr_;
    }

    void SetIrreg(float v)
    {
        irreg_ = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    }

    // UMA VEZ POR BLOCO. As transcendentais moram aqui — a licao do
    // drive (36 us por ter posto tanh no caminho de amostra) e do
    // phaser (LFO por bloco desde o inicio).
    void AtualizaLfo(size_t block_size)
    {
        const float dt = (float)block_size / sr_;
        fase_ += rate_ * dt;
        if(fase_ >= 1.f) fase_ -= (float)(int)fase_;

        // Componente PERIODICA: senoidal, que e como wow de fita se
        // comporta — aceleracao suave, sem canto.
        const float per = std::sin(6.283185307179586f * fase_);

        // Componente IRREGULAR: passeio aleatorio suavizado. Sorteia um
        // alvo novo a cada ~1/rate e caminha ate ele — deriva mecanica
        // nao salta, ela vaga.
        cont_ += dt * rate_;
        if(cont_ >= 0.5f)
        {
            cont_ -= 0.5f;
            alvo_ = Rand() * 2.f - 1.f;
        }
        // Filtro de 1 polo lento: o passeio nunca alcanca o alvo de
        // uma vez, e e isso que faz parecer mecanico em vez de ruidoso.
        passeio_ += 0.15f * (alvo_ - passeio_);

        lfo_ = per * (1.f - irreg_) + passeio_ * irreg_;
    }

    // MONO, in-place. Uma leitura interpolada por amostra.
    float ProcessMono(float x)
    {
        if(cap_ == 0) return x;
        buf_[write_] = x;
        if(++write_ >= cap_) write_ = 0;

        // ATRASO SUAVIZADO POR AMOSTRA. O LFO anda por bloco; se o
        // atraso saltasse junto, cada borda de bloco seria um degrau de
        // altura — clique a cada milissegundo. Interpolar entre blocos
        // e o que torna a deriva continua.
        const float alvo = centro_ + lfo_ * depth_;
        atraso_ += 0.002f * (alvo - atraso_);

        float p = (float)write_ - atraso_;
        while(p < 0.f) p += (float)cap_;
        while(p >= (float)cap_) p -= (float)cap_;

        size_t i0 = (size_t)p;
        if(i0 >= cap_) i0 = cap_ - 1u;
        const float f  = p - (float)i0;
        const size_t i1 = (i0 + 1u >= cap_) ? 0u : i0 + 1u;
        return buf_[i0] + f * (buf_[i1] - buf_[i0]);
    }

  private:
    float Rand()
    {
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        return (float)(rng_ >> 8) * (1.f / 16777216.f);
    }

    float*   buf_   = nullptr;
    size_t   cap_   = 0;
    size_t   write_ = 0;
    float    sr_    = 48000.f;
    float    centro_ = 960.f;
    float    depth_  = 0.f;
    float    rate_   = 0.6f;
    float    irreg_  = 0.5f;
    float    fase_   = 0.f;
    float    cont_   = 0.f;
    float    alvo_   = 0.f;
    float    passeio_ = 0.f;
    float    lfo_    = 0.f;
    float    atraso_ = 960.f;
    uint32_t rng_    = 0x2545F491u;
};

} // namespace cotuca
