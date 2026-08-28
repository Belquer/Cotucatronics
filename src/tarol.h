#pragma once
// ============================================================
// Cotucatronics OS — TAROLATOR v0.1.0
//
// Nome do dB, e o nome E o diagnostico. Nasceu como oitavador por
// divisao de frequencia, e nao produz oitava: numa membrana os modos sao
// INARMONICOS — a medicao de 28/08 achou-lhe oito (278, 370, 406, 672,
// 732, 768, 809, 1061 Hz) — e um detector de cruzamento por zero nao acha
// fundamental nenhuma nisso. Ele treme, e o que sai e rajada de ruido com
// o envelope da pancada. Soa a tarol.
//
// PROVEI-O COM SENOIDES E ELE PASSOU: 768 -> 384 Hz com 0,0% de erro, e
// ate com o segundo harmonico forte. O material do teste nao se parecia
// com a fonte, e eu TINHA o espectro da fonte medido uma hora antes.
// Fica como a licao: banco nativo prova a ALGEBRA, nao o timbre.
//
// O EFEITO FICOU porque o dB gostou — e entao passou a chamar-se o que e.
// Nome que descreve a intencao em vez do resultado engana o proximo, e o
// proximo e ele daqui a um mes.
//
// TUDO FIXO MENOS A DOSE. Os tres numeros abaixo sao os que produziram o
// som aprovado; deixa-los como params daria dois faders com curso morto,
// e a regra do dB e SE NAO SOA, NAO FICA. Quem quiser outro timbre muda
// aqui, com o ouvido, e nao no palco.
//
// C++ puro, sem alocacao, sem estado global. ~17 us por cadeia, medido em
// A/B no mesmo binario.
// ============================================================
#include <cmath>

namespace cotuca
{

class Tarol
{
  public:
    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        // FILTRO DE RASTREIO. Cruzamento por zero conta HARMONICOS como se
        // fossem a fundamental: sem isto, um sinal com o segundo parcial
        // forte dispara o dobro das vezes e o "sub" sai na oitava errada.
        // 1200 Hz deixa passar a fundamental da membrana (768 Hz) e come o
        // que vem acima.
        // OS TRES NUMEROS FIXOS, e o porque de cada um:
        //   1200 Hz de rastreio — deixa passar a regiao onde a membrana
        //          tem energia e come o que vem acima, para o gatilho nao
        //          contar harmonicos.
        //   6000 Hz de tom — o dB julgou de ouvido: abaixo de ~4200 Hz o
        //          efeito desaparecia, porque o que ele produz e banda
        //          larga e um filtro baixo simplesmente o apaga.
        //   0.004 de portao — abaixo disto cala, senao o ruido de fundo
        //          cruza o zero ao acaso e gera tarol no silencio.
        SetRastreio(1200.f);
        SetTom(6000.f);
        flip_ = false; prev_ = 0.f; lp_ = 0.f; sub_lp_ = 0.f; env_ = 0.f;
        SetPortao(0.004f);
    }

  private:
    void SetRastreio(float hz) { a_rast_ = Polo(hz); }
    void SetTom(float hz)      { a_tom_  = Polo(hz); }
  public:

    // PORTAO: abaixo disto o sub CALA. Sem ele o ruido de fundo cruza o
    // zero ao acaso e produz uma oitava aleatoria no silencio — e o
    // silencio e onde ela seria mais audivel.
    void SetPortao(float lim)  { portao_ = lim < 0.f ? 0.f : lim; }

    // Devolve SO o sub. Quem mistura e o chamador: camada paralela ao
    // seco, e nao um `mix` interno que escondesse a dose.
    float Processa(float x)
    {
        // 1) envelope, para o sub seguir a dinamica da pancada
        const float ax = x < 0.f ? -x : x;
        env_ += (ax > env_ ? 0.02f : 0.0006f) * (ax - env_);

        // 2) rastreio
        lp_ += (1.f - a_rast_) * (x - lp_);

        // 3) SCHMITT, e a histerese nao e refinamento: perto do zero o
        //    sinal cruza varias vezes por ciclo, e cada cruzamento a mais
        //    inverte o flip-flop. Sem histerese o divisor nao divide —
        //    treme. O limiar acompanha o envelope para funcionar tanto no
        //    ataque quanto na cauda.
        const float h = env_ * 0.15f + 1e-5f;
        bool sobe = false;
        if(!armado_ && lp_ >  h) { armado_ = true;  sobe = true; }
        if( armado_ && lp_ < -h) { armado_ = false; }
        if(sobe) flip_ = !flip_;

        // 4) a quadrada a metade da taxa, com a amplitude do original
        const float quad = (flip_ ? 1.f : -1.f) * env_;

        // 5) tom: tira o gume da quadrada sem lhe tirar os harmonicos que
        //    fazem a fundamental ausente funcionar
        sub_lp_ += (1.f - a_tom_) * (quad - sub_lp_);

        prev_ = lp_;
        return env_ > portao_ ? sub_lp_ : 0.f;
    }

  private:
    float Polo(float hz) const
    {
        if(hz < 10.f) hz = 10.f;
        return std::exp(-2.f * 3.14159265f * hz / sr_);
    }
    float sr_ = 48000.f;
    float a_rast_ = 0.f, a_tom_ = 0.f;
    float lp_ = 0.f, sub_lp_ = 0.f, env_ = 0.f, prev_ = 0.f;
    float portao_ = 0.004f;
    bool  flip_ = false, armado_ = false;
};

} // namespace cotuca
