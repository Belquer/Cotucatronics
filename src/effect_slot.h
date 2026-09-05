#pragma once
// ============================================================
// Cotucatronics OS — EffectSlot v0.3.0
// A moldura que envolve TODO efeito da cadeia CTOS.
//
// Tres trabalhos, e os tres nasceram de cicatriz:
//
//   1. FADE DE IGUAL-POTENCIA ao ligar e desligar. Um efeito que entra
//      em degrau clica; um que sai em degrau arranca a cauda. O freeze
//      ensinou isso na etapa 9 — "corta seco, soa como tomada
//      arrancada" — e a resposta la foi uma rampa de saida propria. Em
//      vez de cada efeito reinventar a sua, a moldura resolve pra
//      todos.
//
//   2. CUSTO ZERO DESLIGADO. Com o ganho estavel em 0, o slot devolve
//      `false` e o chamador PULA o processamento inteiro. Efeito
//      desligado tem que custar nada — senao "desligar" vira mentira e
//      a soma de cinco efeitos parados ja pesa.
//
//   3. CRONOMETRAGEM POR EFEITO. O CTOS mede o EFEITO, nao o callback
//      inteiro — CpuLoadMeter nao sabe dizer QUEM gastou, e sem separar
//      o veto puniria o inocente (docs/CTOS.md §3).
//
// IGUAL-POTENCIA, e nao linear: durante o cruzamento, seco e molhado
// sao sinais DESCORRELACIONADOS. Rampa linear neles soma amplitude e
// produz um buraco de -3 dB no meio do fade; raiz de cada lado soma
// POTENCIA e o nivel fica constante. E a mesma aritmetica que fez a
// raiz-de-Hann ser a janela certa pro granular — mesma pergunta, mesmo
// regime, mesma resposta.
//
// C++ puro: sem hal, sem motor externo. O relogio vem de fora porque
// `hal::NowTicks()` mora no HAL e este arquivo nao pode inclui-lo.
// ============================================================

#include <cstddef>
#include <cstdint>
#include <cmath>

namespace cotuca
{

class EffectSlot
{
  public:
    // `fade_ms` e o tempo de cruzamento. 5-10 ms: longo o bastante pra
    // nao clicar, curto o bastante pra o gesto parecer imediato.
    // FADE ASSIMETRICO. Entrar e sair sao gestos diferentes:
    //
    //   ENTRAR e decisao — quer resposta imediata (~8 ms, so o
    //   suficiente pra nao clicar).
    //   SAIR pode ser DECAIMENTO — um efeito com cauda que corta seco
    //   soa como tomada arrancada. O freeze ensinou isso na etapa 9, e
    //   o shimmer precisa de 2 s: ele E uma sala se empilhando, e sala
    //   nao para de existir quando alguem solta o botao.
    //
    // `desce_ms = 0` usa o mesmo tempo da subida (simetrico).
    void Init(float sample_rate, float sobe_ms = 8.f, float desce_ms = 0.f)
    {
        sr_   = sample_rate > 0.f ? sample_rate : 48000.f;
        if(desce_ms <= 0.f) desce_ms = sobe_ms;
        passo_sobe_  = 1.f / (0.001f * sobe_ms  * sr_);
        passo_desce_ = 1.f / (0.001f * desce_ms * sr_);
        alvo_ = 0.f;
        g_    = 0.f;
    }

    void SetOn(bool on) { alvo_ = on ? 1.f : 0.f; }
    bool On()     const { return alvo_ >= 0.5f; }
    // Em transicao OU ligado. Quem chama usa isto pra decidir se
    // processa: desligado E estavel devolve false, e o custo e zero.
    bool Ativo()  const { return g_ > 0.f || alvo_ > 0.f; }
    float Ganho() const { return g_; }

    // Avanca a rampa UMA amostra e devolve o ganho a aplicar ao
    // molhado. O seco recebe o complemento de igual-potencia.
    //
    //   saida = seco*sqrt(1-g) + molhado*sqrt(g)
    //
    // Devolvidos juntos porque calcular um sem o outro convida a
    // divergencia — o buraco de -3 dB aparece quando alguem usa (1-g)
    // de um lado e sqrt(g) do outro.
    void Passo(float& g_seco, float& g_molhado)
    {
        if(g_ != alvo_)
        {
            if(g_ < alvo_)
            {
                g_ += passo_sobe_;
                if(g_ > alvo_) g_ = alvo_;
            }
            else
            {
                g_ -= passo_desce_;
                if(g_ < alvo_) g_ = alvo_;
            }
            // So recalcula as raizes QUANDO A RAMPA ANDA.
            //
            // A versao anterior chamava sqrt duas vezes por amostra,
            // sempre — e a rampa esta parada em 99,9% do tempo. Duas
            // VSQRT sao ~28 ciclos; a 48 kHz isso e ~2,7 us por bloco
            // POR EFEITO, pago pra recalcular o mesmo numero.
            //
            // Achado medindo o drive: +36 us caiu pra +24 com tanh
            // rapido e caminho mono, e o que sobrou nao era o efeito —
            // era esta moldura. Custo compartilhado conserta uma vez e
            // paga nos cinco.
            cache_molhado_ = std::sqrt(g_);
            cache_seco_    = std::sqrt(1.f - g_);
        }
        g_molhado = cache_molhado_;
        g_seco    = cache_seco_;
    }

    // ---- CRONOMETRAGEM E VETO (docs/CTOS.md §3) ----
    //
    // Em TICKS, nao em microssegundos, e isto e conserto de defeito
    // real: `System::GetUs()` divide antes de quem chama subtrair, o
    // valor devolvido nao ocupa a faixa de 32 bits e a volta do
    // contador produzia ~4,27e9. Subtrair em ticks e converter DEPOIS
    // faz a volta se corrigir sozinha.
    void Comeca(uint32_t ticks_agora) { t0_ = ticks_agora; }

    // Devolve true se ESTOUROU. Bypass e decisao de quem chama, no
    // mesmo contexto — quem arma e quem checa nao podem divergir.
    bool Termina(uint32_t ticks_agora, uint32_t ticks_por_us,
                 uint32_t orcamento_us)
    {
        if(ticks_por_us == 0) return false;
        const uint32_t us = (ticks_agora - t0_) / ticks_por_us;
        if(us > pico_us_) pico_us_ = us;
        // SEM FILTRO DE PLAUSIBILIDADE POR MAGNITUDE. Ele descartaria a
        // catastrofe junto com o ruido — um estouro real apareceria
        // exatamente como um numero grande. Com o relogio certo, numero
        // grande significa estouro e tem que vetar.
        return us > orcamento_us;
    }

    uint32_t PicoUs() const { return pico_us_; }
    void     ZeraPico()     { pico_us_ = 0; }

  private:
    float    sr_    = 48000.f;
    float    passo_sobe_  = 1.f / 384.f;
    float    passo_desce_ = 1.f / 384.f;
    float    alvo_  = 0.f;
    float    g_     = 0.f;
    float    cache_molhado_ = 0.f;
    float    cache_seco_    = 1.f;
    uint32_t t0_       = 0;
    uint32_t pico_us_  = 0;
};

} // namespace cotuca
