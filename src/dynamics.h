#pragma once
// dynamics.h v0.6.0
// ============================================================
// Cotucatronics OS — cadeia de dinâmica do shell
//
// C++ puro, board-agnostic, SEM motor externo. Isto não é DSP de
// instrumento: é a política de segurança e comportamento do
// shell, que vale pra toda escultura independente de qual
// módulo está carregado. Por isso vive aqui e não num patch.
//
// Ordem na cadeia:
//   fonte → GATE → [módulo] → level → split
//        ├ OUT_L: SHIFTER → GOVERNOR → [bench_boost] → LIMITER
//        └ OUT_R:                                      LIMITER
//
// O ramo do exciter é EXCLUSIVAMENTE política de segurança, toda
// em C++ do shell. Os slots de módulo vivem no caminho principal.
// Trocar a escultura não pode trocar o anti-feedback.
//
// bench_boost é ferramenta de bancada, não parte da cadeia de
// produção — ver o bloco de aviso em manifest.h.
//
// Os três blocos compartilham o MESMO EnvFollower — uma só
// implementação de detector, três políticas diferentes em cima.
// ============================================================

#include <cmath>

namespace cotuca
{

#define COTUCA_DYNAMICS_VERSION "0.6.0"

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

    // RETOMADA ABERTA. O `main` pula o Process() inteiro quando o
    // limiar esta em 0 (portao escancarado, custo zero) — e enquanto
    // pula, `gain_` congela onde estava. Desde o boot isso e ZERO,
    // porque e onde o Init o poe.
    //
    // Sem isto, subir o limiar com som tocando reabre o gate com ganho
    // partindo de 0 e detector descarregado: ~46 ms de fade-in que o dB
    // ouve como um duck ao HABILITAR a protecao. Quem liga um gate nao
    // pede um mergulho.
    //
    // Abrir e o default seguro: se o sinal estiver mesmo abaixo do
    // limiar, o release (300 ms) fecha suave logo em seguida. O erro na
    // direcao contraria — comecar fechado — corta audio que devia
    // passar, no instante exato em que alguem estava mexendo.
    void Abre() { gain_ = 1.f; }

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
    // Soft-knee em domínio linear, sem powf.
    //
    // A curva é definida pelo SLOPE, que vai de 1.0 (transparente)
    // até kSlope ao longo do joelho:
    //
    //     slope(t) = 1 + (kSlope - 1) · t²,  t = (env - lo)/knee
    //
    // e a saída é a INTEGRAL dele, não o produto. Essa distinção não
    // é cosmética: multiplicar a excursão inteira pelo slope
    // instantâneo faz as duas metades da curva não se encontrarem no
    // topo do joelho — dá um degrau de ~0.09 na saída, que é
    // audível como click. (Foi assim que saiu na primeira versão; o
    // teste nativo pegou.)
    //
    //     out(t) = lo + knee · (t + (kSlope-1)·t³/3)
    //
    // Contínua em valor E em derivada nos dois limites do joelho.
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
            // continua da PONTA do joelho, com o slope final
            out = KneeEnd(lo, knee) + (env - hi) * kSlope;
        }
        else
        {
            const float t = (env - lo) / (knee > 1e-9f ? knee : 1e-9f);
            out           = lo + knee * (t + (kSlope - 1.f) * t * t * t / 3.f);
        }
        return out / env;
    }

    // Saída no topo do joelho — o ponto onde os dois ramos se
    // encontram. É a integral de slope(t) de 0 a 1.
    static float KneeEnd(float lo, float knee)
    {
        return lo + knee * (1.f + (kSlope - 1.f) / 3.f);
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
// ============================================================
// COMPRESSOR — sobe a MEDIA sem tocar no pico.
//
// POR QUE EXISTE, e o numero que o pediu: em 02/09 o dB caçava um chiado
// de banda larga. Medido, com ele a bater no instrumento:
//
//     OUT_L  pico -0,4 dBFS   media -49,2 dBFS
//
// QUARENTA E NOVE DECIBEIS DE FACTOR DE CRISTA. E o retrato de um
// instrumento percussivo: transitorio enorme, sustain quase nada.
//
// E e por isso que o amp precisava de tanto ganho — ele estava a ser
// pedido para tornar audivel a MEDIA, e ao faze-lo amplificava o chao de
// ruido do codec ate ser audivel. O pico nao precisava de ajuda nenhuma:
// ja estava no fundo de escala, com 24 clips a entrada.
//
// ⚠️ POR ISSO A RESPOSTA NAO E UM ANDAR DE GANHO. Eu ia construir um, a
// partir de uma medicao de AMBIENTE que dizia 74,7 dB de margem. Com o
// instrumento a tocar a margem era 0,4 dB. Ganho subiria o pico, que ja
// estava no tecto, e cortava-o. O que e preciso e subir a media SEM tocar
// no pico — e isso chama-se compressao.
//
// NAO E O GOVERNOR, e a distincao importa. O governor tem razao fixa, nao
// tem makeup, e o trabalho dele e PROTECCAO — segurar o tecto para o laco
// nao crescer. Este tem o trabalho oposto: levantar o que esta em baixo.
// Mesma forma de DSP, intencoes contrarias; um bloco com dois empregos e a
// familia C do catalogo.
// ============================================================
class Compressor
{
  public:
    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        det_.Init(sr_);
        // Ataque rapido apanha o transitorio; queda media levanta o
        // sustain sem bombear. Fixos: sao dois params que o ouvido ainda
        // nao pediu, e cada param novo e um CC e uma linha de preset.
        det_.SetTimes(3.f, 120.f);
        a_ = EnvFollower::Coef(3.f, sr_);
        r_ = EnvFollower::Coef(120.f, sr_);
        thresh_ = 0.1f; razao_ = 4.f; makeup_ = 1.f; gain_ = 1.f;
    }
    void Set(float thresh, float razao, float makeup)
    {
        thresh_ = thresh > 1e-4f ? thresh : 1e-4f;
        razao_  = razao  > 1.f   ? razao  : 1.f;
        makeup_ = makeup > 0.f   ? makeup : 0.f;
    }
    float Process(float x)
    {
        const float env = det_.Process(x);
        float alvo = 1.f;
        if(env > thresh_)
        {
            // Em dominio linear, sem log: acima do limiar, a excursao
            // passa a valer 1/razao. Nao ha powf no caminho da amostra.
            const float excesso = env - thresh_;
            const float saida   = thresh_ + excesso / razao_;
            alvo = saida / env;
        }
        const float c = (alvo < gain_) ? a_ : r_;
        gain_ = alvo + (gain_ - alvo) * c;
        return x * gain_ * makeup_;
    }
    float Gain() const { return gain_; }
    float Env() const { return det_.Value(); }

  private:
    EnvFollower det_;
    float sr_ = 48000.f, a_ = 0.f, r_ = 0.f;
    float thresh_ = 0.1f, razao_ = 4.f, makeup_ = 1.f, gain_ = 1.f;
};

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

// ------------------------------------------------------------
// Shifter — frequency shifter SSB (banda lateral única).
//
// QUARTO bloco permanente do stack, e o único que não é dinâmica:
// é anti-feedback. Desloca TODO o espectro alguns Hz pra cima. O
// laço piezo→corpo→exciter→piezo depende de coincidência de fase
// numa frequência de ressonância pra crescer; com o retorno
// deslocado, nenhuma frequência fecha o laço consigo mesma e a
// realimentação não encontra onde travar.
//
// Não confundir com pitch shift: aqui é deslocamento LINEAR em
// Hz, não em intervalo. As harmônicas deixam de ser múltiplas
// inteiras — daí soar metálico acima de ~10 Hz, e daí a faixa
// parar em 10.
//
// Por que SSB e não modulação em anel: modulação em anel produz
// as DUAS bandas laterais, e a inferior desce em frequência —
// metade do sinal iria pro lado errado e o laço acharia caminho
// de volta. SSB suprime uma delas.
//
// Estrutura: par de cadeias allpass IIR que mantêm ~90° de
// diferença de fase entre si na banda de áudio (Hilbert
// aproximado), oscilador de quadratura em forma acoplada, e
//
//     out = I·cos(ωt) + Q·sin(ωt)      → banda superior
//
// SÓ no ramo do exciter. O jack de linha sai SEM shift, por
// design: quem grava ou manda pra PA quer o sinal do instrumento,
// não o sinal protegido contra um laço acústico que não existe do
// lado de lá.
// ------------------------------------------------------------
class Shifter
{
  public:
    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        // Coeficientes clássicos do par de Hilbert IIR.
        //
        // ATENÇÃO: os números publicados são `a`, e a seção Allpass2
        // usa `a²`. Elevar ao quadrado aqui NÃO é detalhe — usá-los
        // crus derruba a supressão de banda lateral de ~47 dB pra
        // ~19 dB, com o pior caso bem no meio da banda de áudio.
        // Medido: sem o quadrado, 18,9 dB a 5 kHz; com, 46,8 dB.
        //
        // Locais e não membros de propósito: array `static constexpr`
        // de classe precisa de definição fora dela em C++14, e a
        // build da libDaisy é gnu++14.
        constexpr float ca[kSections] = {0.6923877778065f, 0.9360654322959f,
                                         0.9882295226860f, 0.9987488452737f};
        constexpr float cb[kSections] = {0.4021921162426f, 0.8561710882420f,
                                         0.9722909545651f, 0.9952884791278f};
        for(int i = 0; i < kSections; i++)
        {
            a_[i] = Allpass2{ca[i] * ca[i]};
            b_[i] = Allpass2{cb[i] * cb[i]};
        }
        delay_ = 0.f;
        osc_x_ = 1.f;
        osc_y_ = 0.f;
        SetShift(0.f);
    }

    void SetShift(float hz)
    {
        if(hz == hz_)
            return;
        hz_             = hz;
        const float w   = 2.f * 3.14159265358979f * hz / sr_;
        osc_c_          = cosf(w);
        osc_s_          = sinf(w);
    }

    float Process(float x)
    {
        // I e Q: mesma entrada nas duas cadeias, mas o ramo I recebe
        // a amostra ATRASADA de uma. É esse atraso que completa os
        // 90° — sem ele, ou com ele no ramo errado, as cadeias não
        // ficam em quadratura e a banda lateral indesejada sobrevive.
        float i_sig = delay_;
        delay_      = x;
        for(int k = 0; k < kSections; k++)
            i_sig = a_[k].Process(i_sig);

        float q_sig = x;
        for(int k = 0; k < kSections; k++)
            q_sig = b_[k].Process(q_sig);

        // oscilador de quadratura, forma acoplada: 4 mults por
        // amostra em vez de dois sinf/cosf.
        const float nx = osc_x_ * osc_c_ - osc_y_ * osc_s_;
        const float ny = osc_x_ * osc_s_ + osc_y_ * osc_c_;
        osc_x_         = nx;
        osc_y_         = ny;

        // a amplitude do oscilador acoplado deriva devagar; isto a
        // puxa de volta pra 1 sem raiz quadrada.
        if(++renorm_ >= 1024)
        {
            renorm_        = 0;
            const float g  = 1.5f - 0.5f * (osc_x_ * osc_x_ + osc_y_ * osc_y_);
            osc_x_ *= g;
            osc_y_ *= g;
        }

        // I·cos + Q·sin com ESTE par de cadeias dá a banda SUPERIOR.
        // O sinal depende de qual cadeia carrega o atraso; foi
        // determinado por medição, não por dedução.
        return i_sig * osc_x_ + q_sig * osc_y_;
    }

    float Shift() const { return hz_; }

  private:
    // Allpass de 2ª ordem na forma y[n] = a²·(x[n] + y[n−2]) − x[n−2].
    // Duas amostras de estado, um multiplicador.
    struct Allpass2
    {
        float a2 = 0.f;
        float x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;

        float Process(float x)
        {
            const float y = a2 * (x + y2) - x2;
            x2            = x1;
            x1            = x;
            y2            = y1;
            y1            = y;
            return y;
        }
    };

    static constexpr int kSections = 4; // 8ª ordem por ramo

    Allpass2 a_[kSections], b_[kSections];
    float    delay_ = 0.f;
    float    sr_ = 48000.f, hz_ = -1.f;
    float    osc_x_ = 1.f, osc_y_ = 0.f, osc_c_ = 1.f, osc_s_ = 0.f;
    int      renorm_ = 0;
};

} // namespace cotuca
