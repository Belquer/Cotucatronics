#pragma once
// ============================================================
// Cotucatronics OS — drive v0.4.1
// Waveshaper tanh como COLA, nao como fuzz.
//
// O alvo e engordar percussao, kalimba, monocordio e molas — material
// que ja chega cru e transiente. Distorcao pesada mataria o ataque, que
// e justamente o que faz esse material soar como escultura sendo
// tocada. Entao tudo aqui e escolhido pra dar CORPO sem comer o
// transiente.
//
//   y = tanh(drive * x) / tanh(drive)
//
// A DIVISAO E O QUE IMPORTA. Sem ela, subir o `drive` sobe o volume
// junto, e o ouvido chama qualquer coisa mais alta de "melhor" — o
// controle mediria nivel disfarçado de timbre.
//
// O DENOMINADOR FOI TROCADO POR `drive` EM 22/08 E DESFEITO NO MESMO
// DIA. Fica registrado porque o erro foi de leitura, nao de conta.
//
// Dividir por `drive` normaliza o ganho de PEQUENO SINAL: a camada sai
// no mesmo nivel do seco em qualquer drive, e o controle vira timbre
// puro. Matematicamente correto, e a coisa errada pra este modulo. A
// camada fica limitada a 1/d — em 48x isso e -33 dBFS, enquanto os
// picos do instrumento chegam a -9. O dB ouviu na hora: "o timbre da
// distorcao esta bom mas bem abaixo do som dry, phaser aparece muito
// mais".
//
// As duas normalizacoes distorcem IGUAL — 12.0% de THD em 48x, medido
// nas duas. O que muda e so o peso: +30.4 dB de camada sobre o seco
// com `tanh(d)`, -3.2 dB com `d`. Um efeito paralelo precisa PESAR.
//
// O defeito real nunca esteve na divisao: estava no TETO. 12x punha o
// joelho em -19 dBFS e o material do dB vive a -30, entao o shaper
// ficava reto e sobrava so o ganho. Teto em 48x, matematica original.
// Ver bts 12o.
//
// O JOELHO E RELATIVO AO NIVEL, e e por isso que o teto subiu.
// A curva so molda perto de x ~= 1/d; abaixo disso ela e reta. O
// histograma do instrumento tocado (22/08) poe 98% das amostras abaixo
// de -18 dBFS, entao um drive de 12x — joelho em -19 dBFS — fica reto
// quase o tempo todo. NAO se conserta subindo a ENTRADA (e o `in_gain`
// que fazia isso saiu em 28/08, por nao soar): subir a entrada
// engorda o seco na mesma proporcao e a camada some por comparacao
// (medido, e foi um conselho errado meu em 22/08). Conserta-se aqui,
// no alcance do proprio controle. Ver bts 12n e 12o.
//
// ORDEM INTERNA, e cada passo tem razao:
//   entrada x drive -> tanh -> normaliza -> LPF (tone) -> mix paralelo
//
//   LPF DEPOIS da saturacao, nunca antes. Saturar gera harmonicos
//   agudos asperos; cortar o topo DEPOIS remove a aspereza e deixa o
//   corpo. Filtrar antes so mudaria o que entra no shaper.
//
//   MIX PARALELO, e e obrigatorio. E compressao paralela aplicada a
//   saturacao: o seco passa inteiro e carrega o ATAQUE, o saturado
//   entra por baixo e carrega o CORPO. Crossfade destruiria o
//   transiente — e o freeze ja reprovou uma vez por isso, com o dB
//   dizendo que nao escutava o instrumento por baixo do efeito.
//
// ZERO MEMORIA. Funcao por amostra; o unico estado e o polo do LPF.
// E o efeito mais barato da suite, e serve pra validar o EffectSlot na
// classe "sem historico" antes dos que reusam o motor granular.
//
// C++ puro: sem hal, sem motor externo, sem alocacao.
// ============================================================

#include <cmath>

namespace cotuca
{

class Drive
{
  public:
    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        SetDrive(2.f);
        SetTone(6000.f);
        SetSujeira(0.f);
        // 2 kHz, FIXO. Medido: sem ele a dobra rebate -36 dB a 3 kHz
        // (contra -65 do tanh); com ele, -65 — empate. Nao e ajustavel
        // porque nao e gosto, e o limite de Nyquist do dobrador.
        a_pre_ = 1.f - std::exp(-6.283185307179586f * 2000.f / sr_);
        mix_ = 0.f;
        y_l_ = y_r_ = 0.f;
        pre_l_ = pre_r_ = 0.f;
    }

    // `d` em VEZES, nao em dB. 1.5-3 e cola; acima de ~8 comeca a
    // virar carater proprio.
    void SetDrive(float d)
    {
        if(d < 1.f)  d = 1.f;   // abaixo de 1 a "camada" viraria atenuador
        if(d > 48.f) d = 48.f;  // 48x poe o joelho em -24 dBFS, onde o
                                // material do dB de fato vive
        if(d == drive_) return;
        drive_ = d;
        // Normalizador, calculado SO quando o drive muda. Usa a MESMA
        // aproximacao do caminho quente: dois modelos da mesma curva
        // divergiriam no pico. Acima de d=3 o TanhRapido grampeia em
        // 1.0, entao inv_ = 1.0 e a normalizacao vira inocua — que e o
        // correto, porque nesse regime a curva JA usa a escala inteira.
        inv_ = 1.f / TanhRapido(d);
    }

    // Corte do passa-baixa, em Hz. No topo da faixa ele e praticamente
    // transparente — e isso e proposital: `tone` no maximo deve
    // significar "sem filtro", nao "filtro aberto mas presente".
    void SetTone(float hz)
    {
        if(hz < 200.f)   hz = 200.f;
        if(hz > 18000.f) hz = 18000.f;
        a_ = 1.f - std::exp(-6.283185307179586f * hz / sr_);
        if(a_ > 1.f) a_ = 1.f;
    }

    // 0 = so tanh (o tube que o dB aprovou em 22/08)
    // 1 = so wavefolder. Medido no nivel real: THD de 17.9% pra 58.9%
    // no pico, e OITO VEZES mais energia acima do 7o harmonico. Clipe
    // duro e assimetria foram testados e reprovados: em 48x o sinal ja
    // esta saturado, e mais da MESMA forma nao suja mais.
    void SetSujeira(float d)
    {
        suja_ = d < 0.f ? 0.f : (d > 1.f ? 1.f : d);
    }

    // Dobrador triangular, periodo 4 em `u` — mesma escala em que o
    // tanh satura, entao os dois se alinham no blend. Sem DC: Dobra(0)
    // = 0. Custa um floor e um abs, nenhuma transcendental.
    static float Dobra(float u)
    {
        float t = u * 0.25f + 0.25f;
        t -= std::floor(t);
        return 1.f - 4.f * (t < 0.5f ? 0.5f - t : t - 0.5f);
    }

    void SetMix(float m)
    {
        mix_ = m < 0.f ? 0.f : (m > 1.f ? 1.f : m);
    }

    // APROXIMACAO DE tanh, e a medicao exigiu.
    //
    // `std::tanh` e TRANSCENDENTAL — chamada de biblioteca de ~100
    // ciclos, nao aritmetica. Duas por amostra a 48 kHz custaram
    // +36 us de pico (3,6% do bloco), contra uma aposta de <1%.
    // "Funcao por amostra" nao implica barato quando a funcao e cara.
    //
    // Pade de 2a ordem: x*(27 + x^2) / (27 + 9x^2). Erro < 0.2% em
    // |x| < 3, satura suave, e e so multiplicacao e uma divisao.
    // Acima de |x| = 3 tanh ja esta a 0.995 — grampear ali nao muda
    // nada audivel e evita a divisao crescer.
    static float TanhRapido(float x)
    {
        if(x >  3.f) return 1.f;
        if(x < -3.f) return -1.f;
        const float x2 = x * x;
        return x * (27.f + x2) / (27.f + 9.f * x2);
    }

    // MONO neste ponto da cadeia. A fonte e `src_buf`, um so canal —
    // a versao anterior calculava tanh DUAS VEZES sobre o mesmo valor
    // e jogava metade do custo fora. Estereo real so aparece depois do
    // reverb.
    float ProcessMono(float x)
    {
        float sat = TanhRapido(drive_ * x) * inv_;
        // CUSTO ZERO COM A SUJEIRA EM 0: o dobrador e o passa-baixa
        // dele so existem quando alguem pede.
        if(suja_ > 0.f)
        {
            pre_l_ += a_pre_ * (x - pre_l_);  // ANTI-ALIAS, antes da dobra
            sat += (Dobra(drive_ * pre_l_) - sat) * suja_;
        }
        y_l_ += a_ * (sat - y_l_);   // LPF de 1 polo, POS-saturacao
        // CAMADA: o seco passa inteiro e leva o ataque; o saturado
        // entra por baixo e leva o corpo.
        return x + y_l_ * mix_;
    }

    // Par estereo, pra quando o drive for usado depois do reverb.
    void Process(float& l, float& r)
    {
        float sl = TanhRapido(drive_ * l) * inv_;
        float sr = TanhRapido(drive_ * r) * inv_;
        if(suja_ > 0.f)
        {
            pre_l_ += a_pre_ * (l - pre_l_);
            pre_r_ += a_pre_ * (r - pre_r_);
            sl += (Dobra(drive_ * pre_l_) - sl) * suja_;
            sr += (Dobra(drive_ * pre_r_) - sr) * suja_;
        }
        y_l_ += a_ * (sl - y_l_);
        y_r_ += a_ * (sr - y_r_);
        l += y_l_ * mix_;
        r += y_r_ * mix_;
    }

  private:
    float sr_    = 48000.f;
    float drive_ = 2.f;
    float inv_   = 1.f;
    float a_     = 0.5f;
    float mix_   = 0.f;
    float suja_  = 0.f;
    float a_pre_ = 1.f;
    float y_l_   = 0.f, y_r_ = 0.f;
    float pre_l_ = 0.f, pre_r_ = 0.f;
};

} // namespace cotuca
