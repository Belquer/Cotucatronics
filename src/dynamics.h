#pragma once
// dynamics.h v0.3.0
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

#define COTUCA_DYNAMICS_VERSION "0.3.0"

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

// ------------------------------------------------------------
// Governor — segura o ganho quando o sinal passa do teto.
//
// Política INVERSA à do Gate: o gate age no que é pequeno demais,
// o governor no que é grande demais. Mesmo detector, decisões
// opostas — por isso os dois compartilham EnvFollower.
//
// Não é limiter. O limiter é absoluto e inegociável; o governor é
// negociável e musical: soft-knee, joelho largo, release lento,
// pra que a redução soe como o instrumento "cedendo" em vez de
// bater numa parede.
//
// >>> ESTE BLOCO VAI GANHAR O ENCODER FÍSICO como controle da
// >>> "borda de feedback": girar o encoder move gov_ceiling, ou
// >>> seja move o ponto em que a escultura para de crescer.
// >>> Tocar a realimentação piezo→exciter é achar essa borda e
// >>> passear nela; o governor é o que torna isso seguro em vez
// >>> de um grito. gov_release é a memória dessa borda — release
// >>> longo segura o ganho baixo depois de um pico, evitando o
// >>> bombeamento que faz o feedback voltar a subir.
// ------------------------------------------------------------
class Governor
{
  public:
    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        det_.Init(sr_);
        det_.SetTimes(1.f, 50.f);
        SetTimes(5.f, 800.f);
        ceiling_ = 0.5f;
        gain_    = 1.f;
    }

    void SetTimes(float attack_ms, float release_ms)
    {
        a_ = EnvFollower::Coef(attack_ms, sr_);
        r_ = EnvFollower::Coef(release_ms, sr_);
    }

    void SetRelease(float release_ms) { r_ = EnvFollower::Coef(release_ms, sr_); }

    void SetCeiling(float c) { ceiling_ = c > 1e-4f ? c : 1e-4f; }

    float Process(float x)
    {
        const float env    = det_.Process(x);
        const float target = GainFor(env);
        // reduzir ganho usa attack (rápido, protege); devolver ganho
        // usa release (lento, não bombeia)
        const float c = (target < gain_) ? a_ : r_;
        gain_         = target + (gain_ - target) * c;
        return x * gain_;
    }

    float Gain() const { return gain_; }
    float Env() const { return det_.Value(); }

  private:
    // Soft-knee em domínio linear, sem powf: acima do teto só uma
    // fração `kSlope` do excesso passa, e a transição entra por uma
    // interpolação quadrática ao longo do joelho — sem degrau na
    // derivada, que é o que se ouve como "click" de compressor.
    float GainFor(float env) const
    {
        const float knee = ceiling_ * kKneeWidth;
        const float lo   = ceiling_ - knee * 0.5f;
        const float hi   = ceiling_ + knee * 0.5f;

        if(env <= lo || env <= 1e-6f)
            return 1.f;

        float out;
        if(env >= hi)
        {
            out = ceiling_ + (env - ceiling_) * kSlope;
        }
        else
        {
            // dentro do joelho: slope varia de 1.0 até kSlope
            const float t     = (env - lo) / (knee > 1e-9f ? knee : 1e-9f);
            const float slope = 1.f + (kSlope - 1.f) * t * t;
            out               = lo + (env - lo) * slope;
        }
        return out / env;
    }

    static constexpr float kSlope     = 0.25f; // 1/4 do excesso passa
    static constexpr float kKneeWidth = 0.5f;  // joelho = 50% do teto

    EnvFollower det_;
    float       sr_ = 48000.f, a_ = 0.f, r_ = 0.f;
    float       ceiling_ = 0.5f, gain_ = 1.f;
};

// ------------------------------------------------------------
// Limiter — teto absoluto. O último bloco antes de cada saída.
//
// INEGOCIÁVEL e SEM PARÂMETRO EXPOSTO, de propósito. Gate e
// governor são política musical e se ajustam; o limiter é a
// última linha entre o firmware e um exciter queimado ou um
// alto-falante estourado. Coisa que protege hardware não vira
// knob — não existe cenário em que valha desligá-lo, e existir
// um parâmetro só cria o cenário de desligá-lo por engano.
//
// Duas camadas:
//   1. ganho suavizado (attack 1 ms) quando o envelope passa do
//      teto — é o que faz a redução soar limpa em vez de áspera;
//   2. clamp duro no fim, backstop absoluto. A camada 1 tem
//      tempo de ataque, então um transiente rápido o bastante
//      passa por ela; o clamp garante que nada saia acima do
//      teto, nem por uma amostra.
//
// Sem lookahead: exigiria atrasar o sinal, e num instrumento
// realimentado a latência é parte do laço acústico. Preferimos
// 1 ms de rampa e o clamp de segurança a atraso no laço.
// ------------------------------------------------------------
class Limiter
{
  public:
    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        det_.Init(sr_);
        det_.SetTimes(0.05f, 30.f); // detector quase instantâneo
        a_    = EnvFollower::Coef(1.f, sr_);
        r_    = EnvFollower::Coef(100.f, sr_);
        gain_ = 1.f;
    }

    float Process(float x)
    {
        const float env    = det_.Process(x);
        const float target = (env > kCeiling) ? (kCeiling / env) : 1.f;
        const float c      = (target < gain_) ? a_ : r_;
        gain_              = target + (gain_ - target) * c;

        float y = x * gain_;

        // backstop: nada passa acima do teto, nem por uma amostra
        if(y > kCeiling)
            y = kCeiling;
        else if(y < -kCeiling)
            y = -kCeiling;
        return y;
    }

    float Gain() const { return gain_; }
    static constexpr float Ceiling() { return kCeiling; }

  private:
    static constexpr float kCeiling = 0.95f;

    EnvFollower det_;
    float       sr_ = 48000.f, a_ = 0.f, r_ = 0.f, gain_ = 1.f;
};

} // namespace cotuca
