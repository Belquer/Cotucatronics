#pragma once
// ============================================================
// Cotucatronics OS — motor de pitch compartilhado v0.5.0
//
// UM BUFFER, VARIAS VOZES. E aqui que o "compartilhado" deixa de ser
// palavra e vira economia: shimmer no modo "ambos" precisa de +12 E +7
// lendo o MESMO historico gravado, e detune precisa de duas vozes
// micro-desafinadas do mesmo sinal. Um buffer por efeito seria pagar
// tres vezes pelo mesmo trabalho — e alocacao e a unica coisa que um
// Efeito nao pode pedir em voo (docs/CTOS.md §2).
//
// EXTENSAO DO FREEZE, NAO REUSO. O freeze.h tem a maquinaria granular
// e esta certificado pelo IFA10000 — mas le a taxa 1.0 e NUNCA
// deslocou altura. O que vem de la:
//
//   - janela RAIZ-DE-HANN: dois graos a 50% de sobreposicao somam
//     POTENCIA constante para conteudo descorrelacionado. Hann pura
//     soma 1.0 so para conteudo correlacionado, e o sorteio de posicao
//     abaixo garante que ele NAO e correlacionado. Custou uma versao
//     reprovada de ouvido pra descobrir que janela e estrategia de
//     posicao sao UM PAR.
//   - SORTEIO DE POSICAO: nasceu pra matar o "laco de gaiola" de 2 s do
//     freeze, e aqui paga um dividendo que ninguem projetou — mata o
//     COMB FILTERING que um pitch-shifter de janela fixa produz. Janela
//     fixa repete o mesmo recorte a cada L amostras, e isso e um pente
//     em 1/L; posicao sorteada nao tem periodo, entao nao tem pente.
//   - leitura interpolada e indice limitado ANTES do acesso.
//
// O QUE E NOVO: a taxa de avanco da leitura.
//   ratio 2.000 = oitava acima      ratio 0.500 = oitava abaixo
//   ratio 1.498 = quinta acima      ratio 1.003 = +5 cents
//
// C++ puro: sem hal, sem RNBO. Buffer vem de fora — `hal::AllocBig()` e
// o unico portao da RAM grande, e a regra vale aqui como vale pros
// modulos.
// ============================================================

#include <cstddef>
#include <cstdint>
#include <cmath>

namespace cotuca
{

class PitchEngine
{
  public:
    static constexpr int kMaxVoices = 4;

    // Floats para `seconds` de historico ESTEREO. Perguntado ao motor,
    // nunca fixado por quem chama.
    static size_t FloatsNeeded(float sample_rate, float seconds)
    {
        return 2u * (size_t)(sample_rate * seconds);
    }

    bool Init(float sample_rate, float* mem, size_t n_floats, float seconds)
    {
        if(!mem || sample_rate <= 0.f || seconds <= 0.f) return false;
        if(n_floats < FloatsNeeded(sample_rate, seconds)) return false;

        sr_  = sample_rate;
        cap_ = (size_t)(sample_rate * seconds);
        l_   = mem;
        r_   = mem + cap_;
        for(size_t i = 0; i < cap_; i++) l_[i] = r_[i] = 0.f;
        write_ = 0;
        rng_   = 0x9E3779B9u;
        for(int v = 0; v < kMaxVoices; v++)
        {
            voz_[v] = Voz{};
            SetGrainMs(v, kDefaultGrainMs);
        }
        return true;
    }

    // ESCRITA MONO — metade dos stores, e e o que os pontos mono
    // precisam.
    //
    // Terceira vez que este projeto paga o dobro por tratar uma fonte
    // mono como estereo: drive (tanh 2x), phaser (2N allpass), e agora
    // a GRAVACAO do detune. Esta doeu mais que as outras porque a
    // gravacao roda mesmo com o efeito EM BYPASS — eram 33 us pagos
    // continuamente pra escrever o mesmo numero duas vezes.
    //
    // Quem le com ReadMono deve gravar com WriteMono. O par nao e
    // opcional: gravar mono e ler estereo devolveria silencio no canal
    // direito.
    void WriteMono(float x)
    {
        if(cap_ == 0) return;
        l_[write_] = AA(x, aa_l_);
        if(++write_ >= cap_) write_ = 0;
    }

    // Escrita UNICA, feita pelo CTOS uma vez por amostra. As vozes so
    // leem — nenhuma escreve, entao nao ha ordem entre elas.
    void Write(float l, float r)
    {
        if(cap_ == 0) return;
        l_[write_] = AA(l, aa_l_);
        r_[write_] = AA(r, aa_r_);
        if(++write_ >= cap_) write_ = 0;
    }

    void SetRatio(int v, float ratio)
    {
        if(v < 0 || v >= kMaxVoices) return;
        // Limites de sanidade: fora disso o grao nao cabe no buffer e a
        // leitura atravessaria a cabeca de gravacao.
        if(ratio < 0.25f) ratio = 0.25f;
        if(ratio > 4.0f)  ratio = 4.0f;
        voz_[v].ratio = ratio;
        AtualizaAA();
    }

    void SetGrainMs(int v, float ms)
    {
        if(v < 0 || v >= kMaxVoices || cap_ == 0) return;
        float n = 0.001f * ms * sr_;
        const float lo = 0.005f * sr_;          // 5 ms
        const float hi = (float)(cap_ / 4u);
        if(n < lo) n = lo;
        if(n > hi) n = hi;
        voz_[v].grain = n;
    }

    void SetActive(int v, bool on)
    {
        if(v < 0 || v >= kMaxVoices) return;
        if(on && !voz_[v].ativa)
        {
            // (Re)nasce nos dois graos, defasados de meio periodo.
            for(int g = 0; g < kGrains; g++)
            {
                voz_[v].fase[g] = (float)g / (float)kGrains;
                voz_[v].pos[g]  = Partida(v);
            }
        }
        voz_[v].ativa = on;
        AtualizaAA();
    }

    bool Active(int v) const
    {
        return (v >= 0 && v < kMaxVoices) && voz_[v].ativa;
    }

    // LEITURA MONO — metade do custo, e e o que os pontos mono da
    // cadeia precisam.
    //
    // O `Read` estereo faz duas interpolacoes e duas janelas por grao.
    // Nos pontos onde a fonte e um canal so, a segunda e desperdicio —
    // a mesma licao que o drive e o phaser pagaram duas vezes, agora
    // aplicada a PECA COMPARTILHADA em vez de a cada efeito.
    float ReadMono(int v)
    {
        if(cap_ == 0 || v < 0 || v >= kMaxVoices || !voz_[v].ativa)
            return 0.f;
        Voz& z = voz_[v];
        float out = 0.f;
        for(int g = 0; g < kGrains; g++)
        {
            out += Le(l_, z.pos[g]) * RaizHann(z.fase[g]);
            z.pos[g] += z.ratio;
            if(z.pos[g] >= (float)cap_) z.pos[g] -= (float)cap_;
            if(z.pos[g] < 0.f)          z.pos[g] += (float)cap_;
            z.fase[g] += 1.f / z.grain;
            if(z.fase[g] >= 1.f)
            {
                z.fase[g] -= 1.f;
                z.pos[g]   = Partida(v);
            }
        }
        return out;
    }

    // Le UMA voz e avanca os graos dela. Chamada uma vez por amostra
    // por consumidor. Voz inativa devolve silencio sem custo.
    void Read(int v, float& out_l, float& out_r)
    {
        out_l = out_r = 0.f;
        if(cap_ == 0 || v < 0 || v >= kMaxVoices || !voz_[v].ativa) return;
        Voz& z = voz_[v];

        for(int g = 0; g < kGrains; g++)
        {
            const float w = RaizHann(z.fase[g]);
            out_l += Le(l_, z.pos[g]) * w;
            out_r += Le(r_, z.pos[g]) * w;

            // A LEITURA ANDA A `ratio`, A GRAVACAO ANDA A 1. E dessa
            // divergencia que nasce a altura nova.
            z.pos[g] += z.ratio;
            if(z.pos[g] >= (float)cap_) z.pos[g] -= (float)cap_;
            if(z.pos[g] < 0.f)          z.pos[g] += (float)cap_;

            z.fase[g] += 1.f / z.grain;
            if(z.fase[g] >= 1.f)
            {
                z.fase[g] -= 1.f;
                z.pos[g]   = Partida(v);
            }
        }
    }

  private:
    static constexpr int   kGrains         = 2;
    static constexpr float kDefaultGrainMs = 100.f;
    // Jitter MODESTO, ao contrario do freeze. La o sorteio precisava
    // cobrir um nucleo inteiro pra matar um laco de 2 s; aqui ele so
    // precisa descorrelacionar os graos o bastante pra nao formar
    // pente. Sorteio largo demais viraria warble audivel — e warble e
    // outro efeito da suite, com controle proprio.
    static constexpr float kJitterFrac = 0.15f;

  public:
    // AJUSTAVEL SO PRA MEDIR. Com jitter em 0 os graos ficam
    // deterministicos e um SENO valida a aritmetica da razao por FFT —
    // com jitter, os dois graos leem o mesmo trecho de um sinal
    // altamente correlacionado com fase aleatoria e INTERFEREM, o que
    // derruba o nivel e espalha o espectro. Seno e o pior caso possivel
    // pra janela de potencia constante, e foi a regua errada na
    // primeira tentativa de validacao.
    //
    // Material real (percussao, kalimba, molas) e descorrelacionado e
    // nao sofre disso — por isso o jitter volta pro valor de projeto
    // depois da medicao.
    void SetJitter(float frac)
    {
        if(frac < 0.f) frac = 0.f;
        if(frac > 0.5f) frac = 0.5f;
        jitter_ = frac;
    }

  private:

    struct Voz
    {
        float ratio = 1.f;
        float grain = 2400.f;
        float pos[kGrains]  = {0.f, 0.f};
        float fase[kGrains] = {0.f, 0.5f};
        bool  ativa = false;
    };

    // ONDE O GRAO NASCE, e este e o calculo que faz o pitch funcionar.
    //
    // A leitura anda a `ratio` e a gravacao a 1. Durante um grao de L
    // amostras, a leitura percorre L*ratio e a gravacao L — entao a
    // distancia entre elas muda de L*(ratio-1) por grao.
    //
    // Subindo (ratio>1) a leitura ALCANCA a gravacao; se alcancar, ela
    // le amostras que ainda nao existem — silencio ou lixo. Entao o
    // grao nasce ATRAS o bastante pra a folga cobrir a aproximacao de
    // um grao inteiro, com margem.
    //
    // Descendo (ratio<1) a leitura fica pra tras e o risco e o oposto:
    // dar a volta e encontrar material antigo demais. O buffer de 100 ms
    // cobre com folga na faixa util (0.5x a 2x).
    // ------------------------------------------------------------
    // ANTI-ALIAS: limite de banda NA ESCRITA, automatico.
    //
    // Ler o buffer a `ratio` e compressao no tempo: um parcial em f sai
    // em ratio*f. Tudo acima de sr/(2*ratio) atravessa Nyquist e DOBRA
    // de volta pra dentro da banda audivel — e volta INARMONICO, sem
    // relacao com a nota tocada. Medido na bancada com ratio 2.0: um
    // seno de 20 kHz reaparecia em 8 kHz a -11.9 dB, e no laco do
    // shimmer um simples 3500 Hz fabricava um pico em 19963 Hz que era
    // o MAIS ALTO de toda a cauda. Era isso o "ardido".
    //
    // POR QUE AQUI, E NAO NO SHIMMER: o defeito nao e do shimmer, e de
    // ler rapido — e quem le rapido e o motor. Posto no consumidor,
    // cada efeito futuro que puxe pitch pra cima teria que lembrar
    // sozinho. Posto aqui, herdam calados. E a regra dos tres erros de
    // mono/estereo: conserto sem perguntar "onde mais esta forma
    // aparece?" reincide.
    //
    // CUSTO ZERO QUANDO NAO PRECISA: com todas as vozes em ratio <=
    // 1.05 (detune, warble) o coeficiente e 0 e o filtro e um `return`.
    // So quem sobe oitava paga.
    // ------------------------------------------------------------
    // POR QUE BUTTERWORTH, E NAO UM POLO EM CASCATA. A primeira versao
    // usava tres passa-baixas de um polo. MEDIDO: derrubou a dobra de
    // -11.9 para -25.6 dB — melhor, e insuficiente. A conta explica:
    // `y = (1-a)x + a*y` tem ganho (1-a)/(1+a) em Nyquist, que com
    // corte em 10 kHz e apenas -4.8 dB POR POLO. Tres davam 14 dB no
    // agudo e ja custavam -6.5 dB em 8 kHz: fraco onde tinha que
    // cortar, pesado onde tinha que deixar passar.
    //
    // Butterworth de 6a ordem (tres biquads) e plano ate ~8 kHz
    // (-0.3 dB) e cai 36 dB/oitava depois. Mesma familia de custo, as
    // duas pontas certas.
    struct Biq
    {
        float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
        float z1 = 0.f, z2 = 0.f;

        float Process(float x)
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
        void Ajusta(float fc, float sr, float q)
        {
            const float w  = 6.283185307179586f * fc / sr;
            const float cw = std::cos(w), sw = std::sin(w);
            const float al = sw / (2.f * q);
            const float a0 = 1.f + al;
            b0 = (1.f - cw) * 0.5f / a0;
            b1 = (1.f - cw) / a0;
            b2 = b0;
            a1 = (-2.f * cw) / a0;
            a2 = (1.f - al) / a0;
        }
    };

    void AtualizaAA()
    {
        float rmax = 1.f;
        for(int v = 0; v < kMaxVoices; v++)
            if(voz_[v].ativa && voz_[v].ratio > rmax) rmax = voz_[v].ratio;
        if(rmax == aa_ratio_) return;   // so recalcula quando muda
        aa_ratio_ = rmax;
        aa_on_    = (rmax > 1.05f);
        if(!aa_on_) return;
        // 0.42 e nao 0.5: a margem paga a transicao do filtro.
        const float fc = 0.42f * sr_ / rmax;
        // Q das tres secoes de um Butterworth de ordem 6.
        static const float kQ[3] = {0.5176f, 0.7071f, 1.9319f};
        for(int k = 0; k < 3; k++)
        {
            aa_l_[k].Ajusta(fc, sr_, kQ[k]);
            aa_r_[k].Ajusta(fc, sr_, kQ[k]);
        }
    }

    float AA(float x, Biq* s)
    {
        if(!aa_on_) return x;
        for(int k = 0; k < 3; k++) x = s[k].Process(x);
        return x;
    }

    float Partida(int v)
    {
        const Voz&  z    = voz_[v];
        const float aprox = z.grain * (z.ratio > 1.f ? (z.ratio - 1.f) : 0.f);
        const float folga = z.grain + aprox * 1.5f;
        return Wrap((float)write_ - folga + Jitter(z.grain));
    }

    // RAIZ de Hann: h1^2 + h2^2 = hann1 + hann2 = 1 para graos
    // descorrelacionados. Ver o cabecalho.
    static float RaizHann(float p)
    {
        const float h = 0.5f * (1.f - std::cos(6.283185307179586f * p));
        return std::sqrt(h);
    }

    float Wrap(float x) const
    {
        const float n = (float)cap_;
        if(n <= 0.f) return 0.f;
        float y = std::fmod(x, n);
        if(y < 0.f) y += n;
        return y;
    }

    float Jitter(float grain)
    {
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        const float u = (float)(rng_ >> 8) * (1.f / 16777216.f);
        return (u * 2.f - 1.f) * jitter_ * grain;
    }

    // Indice limitado ANTES do acesso, e NaN pego junto — contrato do
    // CTOS §2, e as duas violacoes que o freeze cometeu.
    float Le(const float* b, float p) const
    {
        if(!(p >= 0.f)) return 0.f;
        size_t i0 = (size_t)p;
        if(i0 >= cap_) i0 = cap_ - 1u;
        const float  f  = p - (float)i0;
        const size_t i1 = (i0 + 1u >= cap_) ? 0u : i0 + 1u;
        return b[i0] + f * (b[i1] - b[i0]);
    }

    float*   l_    = nullptr;
    float*   r_    = nullptr;
    size_t   cap_  = 0;
    size_t   write_ = 0;
    float    sr_   = 48000.f;
    uint32_t rng_  = 0x9E3779B9u;
    float    jitter_ = kJitterFrac;
    bool     aa_on_    = false;  // false = nenhuma voz sobe, filtro fora
    float    aa_ratio_ = 1.f;
    Biq      aa_l_[3];
    Biq      aa_r_[3];
    Voz      voz_[kMaxVoices];
};

} // namespace cotuca
