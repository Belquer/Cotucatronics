#pragma once
// ============================================================
// Cotucatronics OS — freeze granular v0.3.1
//
// v0.3.0 — TRES PEDIDOS DO dB, e os tres tinham raiz de desenho:
//
//   1. MIX E CAMADA, NAO CROSSFADE. Era `l*(1-mix) + wl*mix`, entao
//      mix 1.0 APAGAVA o instrumento vivo — daí "quando o frz esta
//      rolando nao escuto o som do instrumento direito". Nao era efeito
//      colateral: era o que a linha fazia. Congelar deve SOMAR uma
//      camada sustentada, nao substituir a fonte.
//
//   2. JANELA RAIZ-DE-HANN. Duas Hann a 50% somam 1.0 apenas para
//      conteudo CORRELACIONADO. Com as posicoes sorteadas (v0.2.0) os
//      graos ficaram DESCORRELACIONADOS e passaram a somar em potencia:
//      ~0,7x da amplitude — o "ganho muito baixo". sqrt(hann) da
//      potencia constante justamente nesse caso, porque h1^2 + h2^2 =
//      hann1 + hann2 = 1. Nao e ganho de maquiagem, e a janela certa
//      pro regime certo.
//
//   3. CAUDA NO CORTE. `frz_on` 1->0 cortava seco, e soava como tomada
//      arrancada em vez de gesto. Agora ha rampa de saida com os graos
//      rodando durante a descida.
//
// E paga as duas violacoes do contrato do CTOS que o proprio docs/CTOS.md
// aponta neste arquivo: laco sem teto no WrapF e indice sem limite no
// Read. Nenhuma das duas tinha aparecido em teste — funcionam sempre,
// ate o dia em que os dados saem da faixa.
//
// ✅ CERTIFICADO — IFÁ10000 completo, 2026-08-18.
//     Etapa 5: 98 us de pico do EFEITO em 500 de orcamento (CTOS),
//     37% do prazo do bloco no sistema inteiro.
//     Etapa 9: aceite do dB — "ficou otimo assim". Sustentado de 10 s
//     sem pulsacao, cauda de 2 s, e o instrumento vivo audivel por
//     baixo do congelado.
//     Conferido no `ctos` DEPOIS do teste: freeze ativo o tempo todo,
//     sem bypass. Essa checagem virou procedimento porque o primeiro
//     aceite foi anulado por um veto falso do watchdog — 15 s de
//     silencio que eu quase apresentei como resultado.
//
// ------------------------------------------------------------
// A RÉGUA DE ACEITE É VIDRO, NÃO GAIOLA — e a v0.1.0 REPROVOU.
//
// O dB ouviu a v0.1.0 e disse: **"cria um loop de mais ou menos 2 s
// com o áudio tocado"** e **"não soa como freeze, soa como um delay
// lento e permanente"**. Reprovado na etapa 9.
//
// A CAUSA, e ela estava escrita como defesa neste mesmo cabeçalho:
// a v0.1.0 fazia a cabeça de leitura VARRER o trecho congelado pra
// frente a 1x, com um jitter de ±25% do grão na posição de partida.
// Eu afirmei aqui que o jitter mataria a periodicidade. **Não mata.**
// Jitter de ±25 ms sobre um ciclo de 2 s apenas balança o laço: o
// material é finito, e varredura linear passa por ele inteiro e
// volta. Varrer um trecho a 1x É reproduzir esse trecho — ou seja,
// um delay longo em laço, exatamente o que o dB descreveu. O
// argumento estava no arquivo e a aritmética nunca foi feita.
//
// O CONSERTO, e a ideia que faltava:
//
//   SUSTENTAÇÃO NÃO É REPRODUÇÃO DE TRECHO.
//
// Um freeze deve soar como um som SEGURADO, não como uma passagem
// retocada. Então não há varredura e não há ordem:
//
//   a) Congelar define um NÚCLEO curto (~500 ms) terminando no
//      instante do gesto — o timbre que acabou de existir.
//   b) Cada grão que renasce SORTEIA uma posição dentro do núcleo.
//      Sorteio não tem período: não existe "voltar ao mesmo ponto",
//      porque não existe percurso.
//   c) DOIS grãos defasados em meio período, janela Hann. Hann a
//      50% de sobreposição soma exatamente 1.0 — a amplitude não
//      pulsa. É o que separa vidro de motor.
//
// Sortear é livre de clique por construção: cada grão nasce e morre
// em zero pela janela, então qualquer posição de partida serve.
// ============================================================

#include <cstddef>
#include <cstdint>
#include <cmath>

namespace cotuca
{

class Freeze
{
  public:
    // Quantos floats o Init precisa pra `seconds` de captura ESTÉREO.
    // Perguntado ao próprio freeze, nunca fixado por quem chama.
    static size_t FloatsNeeded(float sample_rate, float seconds)
    {
        return 2u * (size_t)(sample_rate * seconds);
    }

    // Devolve false se o buffer for curto. Quem chama DEVE checar:
    // buffer curto não soa errado, soa lendo memória alheia.
    bool Init(float sample_rate, float* mem, size_t n_floats, float seconds)
    {
        if(!mem || sample_rate <= 0.f || seconds <= 0.f)
            return false;
        if(n_floats < FloatsNeeded(sample_rate, seconds))
            return false;

        sr_  = sample_rate;
        cap_ = (size_t)(sample_rate * seconds);
        l_   = mem;
        r_   = mem + cap_;
        for(size_t i = 0; i < cap_; i++)
            l_[i] = r_[i] = 0.f;

        write_  = 0;
        mix_    = 0.f;
        rng_    = 0x9E3779B9u;
        estado_    = Estado::Capturando;
        rel_       = 0.f;
        // Cauda de kReleaseMs: o gesto termina, nao e cortado.
        rel_passo_ = 1.f / (0.001f * kReleaseMs * sr_);
        SetSize(kDefaultGrainMs);
        return true;
    }

    void SetMix(float m) { mix_ = Clamp01(m); }

    // Janela do grão em MILISSEGUNDOS, limitada a um terço do núcleo:
    // grão maior que isso não deixa margem pra sortear, e os dois
    // grãos passam a ler quase o mesmo trecho — que é o batimento
    // voltando pela porta dos fundos.
    void SetSize(float ms)
    {
        if(cap_ == 0)
            return;
        float n = 0.001f * ms * sr_;
        const float lo = 0.008f * sr_;
        const float hi = NucleusLen() / 3.f;
        if(n < lo) n = lo;
        if(n > hi) n = hi;
        grain_ = n;
    }

    // Borda de subida CONGELA; borda de descida volta a capturar.
    // Toda a decisão de estado mora aqui, e não no Process — quem
    // arma e quem consome ficam no mesmo contexto (família A dos
    // bugs de ordem).
    void SetFrozen(bool on)
    {
        if(on == (estado_ != Estado::Capturando))
            return;
        if(!on)
        {
            // NAO para seco. Entra em SOLTANDO: os graos continuam
            // rodando e o nivel desce numa rampa. Um freeze que morre em
            // degrau soa como tomada arrancada, nao como gesto.
            if(estado_ == Estado::Congelado)
            {
                estado_ = Estado::Soltando;
                rel_    = 1.f;
            }
            return;
        }
        estado_ = Estado::Congelado;
        rel_    = 1.f;

        // O NÚCLEO termina no instante do gesto: é o timbre que
        // acabou de existir. Começar pelo material antigo entregaria
        // um som de 2 s atrás, que soa como atraso, não como
        // sustentação — e foi metade do defeito da v0.1.0.
        nucleus_ = NucleusLen();
        nuc0_    = WrapF((float)write_ - nucleus_);
        for(int g = 0; g < kGrains; g++)
        {
            // Meio período de defasagem: é o que faz as duas Hann
            // somarem 1.0 e a amplitude não pulsar.
            phase_[g] = (float)g / (float)kGrains;
            pos_[g]   = PickInNucleus();
        }
    }

    bool Frozen() const { return estado_ != Estado::Capturando; }

    // Um par estéreo por chamada, in-place, igual ao reverb.
    void Process(float& l, float& r)
    {
        if(cap_ == 0)
            return;

        if(estado_ == Estado::Capturando)
        {
            // Gravador silencioso: enquanto não congela, não toca no
            // áudio. Gravar SEMPRE é o que permite ao gesto pegar o
            // som que acabou de acontecer.
            l_[write_] = l;
            r_[write_] = r;
            if(++write_ >= cap_)
                write_ = 0;
            return;
        }

        float wl = 0.f, wr = 0.f;
        for(int g = 0; g < kGrains; g++)
        {
            const float w = Hann(phase_[g]);
            wl += Read(l_, pos_[g]) * w;
            wr += Read(r_, pos_[g]) * w;

            pos_[g] += 1.f;
            if(pos_[g] >= (float)cap_)
                pos_[g] -= (float)cap_;

            phase_[g] += 1.f / grain_;
            if(phase_[g] >= 1.f)
            {
                phase_[g] -= 1.f;
                // SORTEIO, não avanço. Sem percurso não há período,
                // e sem período não há gaiola.
                pos_[g] = PickInNucleus();
            }
        }

        // CAMADA, nao crossfade: o instrumento vivo CONTINUA passando e
        // o congelado entra por cima. O limiter esta a jusante e cuida
        // da soma; apagar a fonte pra caber seria trocar um problema de
        // nivel por um de musica.
        const float g = mix_ * rel_;
        l += wl * g;
        r += wr * g;

        if(estado_ == Estado::Soltando)
        {
            rel_ -= rel_passo_;
            if(rel_ <= 0.f)
            {
                rel_    = 0.f;
                estado_ = Estado::Capturando; // volta a gravar
            }
        }
    }

  private:
    static constexpr int   kGrains         = 2;
    static constexpr float kDefaultGrainMs = 100.f;
    // Núcleo de 500 ms: longo o bastante pra o sorteio ter de onde
    // escolher e a textura respirar, curto o bastante pra ser UM
    // timbre em vez de uma passagem musical. Acima de ~1 s o ouvido
    // volta a reconhecer material, e reconhecer material é o começo
    // de perceber repetição.
    static constexpr float kNucleusMs = 500.f;
    // 2 s de cauda, pedido do dB de ouvido (2026-08-18). Comecou em 300 ms
    // — suficiente pra nao soar CORTADO, e curto demais pra soar como
    // DECAIMENTO. A diferenca nao e de grau: 300 ms remove o degrau,
    // 2 s faz o congelado se dissolver na sala em vez de terminar.
    //
    // Cabe no buffer com folga: a cauda so LE o nucleo ja congelado e
    // nao adia a volta da captura alem dela mesma. O custo e que o
    // instrumento fica 2 s sem gravar material novo depois de cada
    // solta — irrelevante num buffer de 2 s que so precisa dos ~500 ms
    // anteriores ao gesto seguinte.
    static constexpr float kReleaseMs = 2000.f;

    float NucleusLen() const
    {
        float n = 0.001f * kNucleusMs * sr_;
        const float teto = (float)(cap_ / 2u);
        return n > teto ? teto : n;
    }

    // Posição sorteada dentro do núcleo, com margem pro grão inteiro
    // caber sem atravessar a borda.
    float PickInNucleus()
    {
        const float margem = nucleus_ > grain_ ? nucleus_ - grain_ : 0.f;
        return WrapF(nuc0_ + Rand01() * margem);
    }

    static float Clamp01(float v)
    {
        return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    }

    // Hann em AMPLITUDE (não em potência): duas delas a 50% de
    // sobreposição somam 1.0 exato, que é a condição de não pulsar.
    // RAIZ de Hann. Duas Hann a 50% de sobreposicao somam 1.0 em
    // AMPLITUDE, o que vale pra conteudo correlacionado. Nossos graos
    // leem posicoes SORTEADAS, entao sao descorrelacionados e somam em
    // POTENCIA — com Hann puro isso da ~0,7x e soa baixo. Com
    // h = sqrt(hann): h1^2 + h2^2 = hann1 + hann2 = 1, potencia
    // constante. A janela certa depende do regime, nao do gosto.
    static float Hann(float p)
    {
        const float h = 0.5f * (1.f - std::cos(6.283185307179586f * p));
        return std::sqrt(h);
    }

    // CONTRATO DO CTOS: sem laco cujo teto dependa dos dados. O `while`
    // anterior terminava em duas voltas em uso normal — e "em uso
    // normal" nao e garantia dentro do callback de audio. `fmod` nao
    // tem essa dependencia.
    float WrapF(float x) const
    {
        const float n = (float)cap_;
        if(n <= 0.f)
            return 0.f;
        float y = std::fmod(x, n);
        if(y < 0.f) y += n;
        return y;
    }

    // xorshift32: barato, sem estado global, determinístico por
    // instância — dois freezes não compartilham sorte.
    float Rand01()
    {
        rng_ ^= rng_ << 13;
        rng_ ^= rng_ >> 17;
        rng_ ^= rng_ << 5;
        return (float)(rng_ >> 8) * (1.f / 16777216.f);
    }

    // Linear. A posição sorteada é fracionária, e truncar produziria
    // um degrau de até uma amostra a cada renascimento.
    float Read(const float* b, float p) const
    {
        // CONTRATO DO CTOS: limita ANTES de acessar, sem confiar em
        // invariante de quem chama. O codigo anterior protegia `i1` e
        // esquecia `i0` — num buffer de 768 KB em SDRAM, um `p` fora da
        // faixa le memoria alheia em silencio.
        if(!(p >= 0.f))          // pega NaN tambem
            return 0.f;
        size_t i0 = (size_t)p;
        if(i0 >= cap_) i0 = cap_ - 1u;
        const float  f  = p - (float)i0;
        const size_t i1 = (i0 + 1u >= cap_) ? 0u : i0 + 1u;
        return b[i0] + f * (b[i1] - b[i0]);
    }

    float*   l_       = nullptr;
    float*   r_       = nullptr;
    size_t   cap_     = 0;
    size_t   write_   = 0;
    float    sr_      = 48000.f;
    float    grain_   = 4800.f;
    float    mix_     = 0.f;
    enum class Estado { Capturando, Congelado, Soltando };
    Estado   estado_  = Estado::Capturando;
    float    rel_     = 0.f;
    float    nuc0_    = 0.f;  // início do núcleo
    float    nucleus_ = 24000.f;
    float    pos_[kGrains]   = {0.f, 0.f};
    float    phase_[kGrains] = {0.f, 0.5f};
    float    rel_passo_ = 1.f / 14400.f;
    uint32_t rng_ = 0x9E3779B9u;
};

} // namespace cotuca
