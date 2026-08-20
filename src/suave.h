#pragma once
// ============================================================
// Cotucatronics OS — suavizador de parametro v0.1.0
//
// Um polo, para tirar o degrau de qualquer valor que venha do bloco e
// seja usado por amostra. Existe porque o `ParamTable` NAO suaviza: ele
// guarda e devolve o numero cru, e quem consome por amostra herda o
// degrau inteiro de um CC de 7 bits.
//
// O projeto ja resolvia isso TRES vezes, cada uma por conta propria: o
// `effect_slot` rampa o ganho, o warble suaviza o delay, o shimmer
// interpola o tom. Este arquivo e a quarta ocorrencia virando forma
// unica — a regra dos tres erros de mono/estereo: conserto sem
// perguntar "onde mais esta forma aparece?" reincide.
//
// NAO E CROSSFADE E NAO E RAMPA DE LIGA/DESLIGA. Para ligar e desligar
// efeito existe o `effect_slot`, que tem fade assimetrico e sabe
// declarar-se inativo para custar zero. Este aqui e so o filtro que
// impede o clique num valor continuo.
//
// C++ puro, sem estado global, sem alocacao.
// ============================================================

#include <cmath>

namespace cotuca
{

class Suave
{
  public:
    // `inicial` evita a rampa de estreia: um ganho que nasce em 0 e
    // sobe ate 1 no primeiro bloco e um fade-in que ninguem pediu.
    void Init(float sample_rate, float ms = 20.f, float inicial = 0.f)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        SetMs(ms);
        y_ = inicial;
    }

    void SetMs(float ms)
    {
        if(ms < 0.1f) ms = 0.1f;
        a_ = std::exp(-1.f / (0.001f * ms * sr_));
    }

    // Sem rampa: para quando o salto e legitimo (boot, preset, reset).
    void Salta(float v) { y_ = v; }

    float Passo(float alvo)
    {
        y_ += (1.f - a_) * (alvo - y_);
        return y_;
    }

    float Valor() const { return y_; }

  private:
    float sr_ = 48000.f;
    float a_  = 0.f;
    float y_  = 0.f;
};

} // namespace cotuca
