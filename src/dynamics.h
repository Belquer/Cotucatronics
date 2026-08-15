#pragma once
// dynamics.h v0.1.0
// ============================================================
// Cotucatronics OS — cadeia de dinâmica do shell
//
// C++ puro, board-agnostic, SEM RNBO. Isto não é DSP de
// instrumento: é a política de segurança e comportamento do
// shell, que vale pra toda escultura independente de qual
// módulo está carregado. Por isso vive aqui e não num patch.
//
// Ordem na cadeia:
//   fonte → GATE → [módulo] → level → split
//                                      ├ OUT_L: GOVERNOR → LIMITER
//                                      └ OUT_R:            LIMITER
//
// Os três blocos compartilham o MESMO EnvFollower — uma só
// implementação de detector, três políticas diferentes em cima.
// ============================================================

#include <cmath>

namespace cotuca
{

#define COTUCA_DYNAMICS_VERSION "0.1.0"

// ------------------------------------------------------------
// EnvFollower — detector de envelope de um polo, com constantes
// de tempo separadas pra subida e descida.
//
// Base compartilhada dos três blocos. Gate, governor e limiter
// diferem no que FAZEM com o envelope, não em como o medem.
// ------------------------------------------------------------
class EnvFollower
{
  public:
    void Init(float sample_rate)
    {
        sr_  = sample_rate > 0.f ? sample_rate : 48000.f;
        env_ = 0.f;
        SetTimes(1.f, 50.f);
    }

    void SetTimes(float attack_ms, float release_ms)
    {
        a_ = Coef(attack_ms, sr_);
        r_ = Coef(release_ms, sr_);
    }

    float Process(float x)
    {
        const float rect = x < 0.f ? -x : x;
        // sobe com a constante de attack, desce com a de release
        const float c = (rect > env_) ? a_ : r_;
        env_          = rect + (env_ - rect) * c;
        return env_;
    }

    float Value() const { return env_; }
    void  Reset() { env_ = 0.f; }

    // Coeficiente de um polo pra uma constante de tempo em ms.
    // Público porque gate/governor/limiter usam a mesma conta pra
    // suavizar o GANHO, não só o envelope.
    static float Coef(float ms, float sr)
    {
        if(ms <= 0.f || sr <= 0.f)
            return 0.f; // instantâneo
        return expf(-1000.f / (ms * sr));
    }

  private:
    float sr_ = 48000.f, env_ = 0.f, a_ = 0.f, r_ = 0.f;
};

// ------------------------------------------------------------
// Gate — silencia o chão de ruído.
//
// Política: abaixo do threshold o ganho vai a ZERO, com rampa.
// Abre rápido (attack) pra não comer o ataque de um tap no
// piezo; fecha devagar (release) pra não picotar a cauda de uma
// ressonância que decai.
//
// É o irmão de política INVERSA do Governor: o gate age no que
// é pequeno demais, o governor no que é grande demais. Mesmo
// detector, decisões opostas.
// ------------------------------------------------------------
class Gate
{
  public:
    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        det_.Init(sr_);
        // detector deliberadamente mais rápido que a rampa de
        // ganho: queremos SABER do transiente antes de decidir.
        det_.SetTimes(0.5f, 20.f);
        SetTimes(10.f, 300.f);
        thresh_ = 0.02f;
        gain_   = 0.f;
    }

    void SetTimes(float attack_ms, float release_ms)
    {
        a_ = EnvFollower::Coef(attack_ms, sr_);
        r_ = EnvFollower::Coef(release_ms, sr_);
    }

    void SetRelease(float release_ms) { r_ = EnvFollower::Coef(release_ms, sr_); }
    void SetThreshold(float t) { thresh_ = t; }

    float Process(float x)
    {
        const float env    = det_.Process(x);
        const float target = (env >= thresh_) ? 1.f : 0.f;
        // abrindo usa attack, fechando usa release
        const float c = (target > gain_) ? a_ : r_;
        gain_         = target + (gain_ - target) * c;
        return x * gain_;
    }

    float Gain() const { return gain_; }
    float Env() const { return det_.Value(); }

  private:
    EnvFollower det_;
    float       sr_ = 48000.f, a_ = 0.f, r_ = 0.f;
    float       thresh_ = 0.02f, gain_ = 0.f;
};

} // namespace cotuca
