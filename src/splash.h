// splash v0.3.0 — a abertura: jogada de buzios, uma concha cresce, vira
// marca, o nome entra letra a letra, a versao assina.
//
// C++ PURO. Nao inclui hal, nao toca em periferico, nao dorme. Quem chama
// pede UM quadro e recebe UM quadro; a cadencia e de quem chama. E por
// isso que isto se ve inteiro no computador antes de haver vidro.
//
// ⚠️ NUNCA no callback de audio. Isto e trabalho de laco.
//
// A marca E o buzio. Nao sao dois desenhos: e a mesma `Concha`, com raio
// maior e mais dentes. Se fossem dois, divergiam na primeira alteracao.
#pragma once
#include <stdint.h>
#include "desenho.h"

namespace cotuca { namespace splash {

#define COTUCA_SPLASH_VERSION "0.3.0"

static const int kBuzios = 16;

// ---- linha do tempo, em quadros a 20 Hz ----
// Completo: 50 quadros = 2,50 s. Curto: 12 quadros = 0,60 s.
struct Tempo { int jogada, cresce, nome, fim; };

// ABERTURA: so no arranque. 50 quadros = 2,50 s.
static const Tempo kAbertura = { 30, 39, 45, 50 };

// JOGADA: quando os BUZIOS DISPARAM de verdade. Termina onde a jogada
// assenta — sem concha a crescer, sem marca, sem nome. O ecra mostra a
// jogada que o instrumento acabou de fazer, e nao a sua assinatura.
//
// Com `jogada == fim`, o ramo da marca nunca corre: nao ha bandeira nova
// a testar dentro do desenho, so uma linha do tempo que acaba mais cedo.
static const Tempo kJogada = { 30, 30, 30, 30 };

class Splash
{
  public:
    // Semente vem do gerador de buzios do proprio instrumento: cada
    // arranque e uma jogada diferente, e nao um enfeite pseudo-aleatorio
    // que e sempre o mesmo.
    void Comeca(uint32_t semente, bool so_jogada = false)
    {
        t_      = so_jogada ? kJogada : kAbertura;
        rng_    = semente ? semente : 0x1F35Cu;
        quadro_ = 0;
        ativo_  = true;
        for(int i = 0; i < kBuzios; i++)
        {
            bz_[i].xf     = (int16_t)(12 + (int)(Rand() % 105));
            bz_[i].yf     = (int16_t)(14 + (int)(Rand() % 40));
            // ⚠️ O ULTIMO TEM DE POUSAR COM FOLGA ANTES DO FIM DA FASE.
            // Com escalonamento de 12 e voo de 16, o ultimo assentava no
            // quadro 27 de 30: sobravam 3 quadros (0,15 s) de "jogada
            // assente", que ao olho nao e um estado, e um piscar. Com 8 e
            // 14 assenta no 21 e a jogada FICA POUSADA 0,45 s, que e o
            // quadro 1 da referencia.
            bz_[i].t0     = (uint8_t)((i * 8) / kBuzios);
            bz_[i].aberto = (Rand() & 1) != 0;               // cara ou coroa
        }
    }

    bool Ativo() const { return ativo_; }
    int  Quadros() const { return t_.fim; }

    // Desenha o quadro corrente e avanca. Devolve false quando acabou —
    // e o quadro final FICA desenhado, nao se apaga ao sair.
    bool Quadro(uint8_t* fb)
    {
        if(!ativo_) return false;
        desenho::Limpa(fb);
        const int f = quadro_;

        if(f < t_.jogada) Jogada(fb, f);
        else
        {
            // cresce e vira marca: um so movimento continuo, porque no
            // vidro tambem e um so movimento
            const int   d = t_.cresce - t_.jogada;
            const float p = d > 0 ? Clamp01((float)(f - t_.jogada) / (float)d) : 1.f;
            // ⚠️ MEDIDO EM PIXEIS, e nao a olho. Com ra=30/rb=13 e cy=20
            //    o anel exterior (ra+5, rb+4) acabava em y=37 e o nome
            //    comeca em y=40: TRES pixeis de folga, e a marca colava-se
            //    as letras. Pedido do dB em 02/09.
            //
            //    Agora ra=26/rb=11 -> anel em (31,15), centro em cy=17:
            //    a marca ocupa y=2..32 e a folga passa a OITO pixeis.
            const int   ra = 6  + (int)(20.f * p + 0.5f);
            const int   rb = 4  + (int)( 7.f * p + 0.5f);
            // A que cresce e UMA DAS QUE CAIRAM — parte de onde o
            // buzio 0 pousou e sobe ao centro. Nascer no meio do nada
            // seria outra concha, e a jogada deixava de ter consequencia.
            const int   x0 = bz_[0].xf, y0 = bz_[0].yf;
            const int   cx = x0 + (int)((64 - x0) * p);
            const int   cy = y0 + (int)((17 - y0) * p);
            Concha(fb, cx, cy, ra, rb, true);

            if(f >= t_.cresce)
            {
                // nome letra a letra
                const int dn = t_.nome - t_.cresce;
                const int n  = dn > 0
                    ? (int)((long)(f - t_.cresce + 1) * kNomeLen / dn)
                    : kNomeLen;
                char buf[kNomeLen + 1];
                int  k = 0;
                for(; k < n && k < kNomeLen; k++) buf[k] = kNome[k];
                buf[k] = '\0';
                // ⚠️ CENTRADO PELO NOME INTEIRO, nao pelo pedaco: senao o
                // texto escorrega para a esquerda a cada letra, e o que se
                // ve e um deslize, nao uma escrita.
                desenho::Texto(fb, (128 - desenho::Largura(kNome)) / 2, 40, buf);
            }
            if(f >= t_.nome) desenho::TextoCentrado(fb, 52, versao_);
        }

        if(++quadro_ >= t_.fim) ativo_ = false;
        return true;
    }

    void Versao(const char* v)
    {
        int i = 0;
        for(; v[i] && i < (int)sizeof(versao_) - 1; i++) versao_[i] = v[i];
        versao_[i] = '\0';
    }

  private:
    struct Buzio { int16_t xf, yf; uint8_t t0; bool aberto; };

    static float Clamp01(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }

    uint32_t Rand()
    {
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        return rng_;
    }

    // A jogada: caem de CIMA, do meio, e abrem em leque. O horizontal e
    // linear e o vertical e quadratico — que e gravidade, e e o que faz o
    // leque parecer queda e nao deslize.
    void Jogada(uint8_t* fb, int f) const
    {
        for(int i = 0; i < kBuzios; i++)
        {
            const float p = Clamp01((float)(f - bz_[i].t0) / (float)kVoo);
            if(f < bz_[i].t0) continue;
            const int x = 64 + (int)((bz_[i].xf - 64) * p);
            const int y = -6 + (int)((bz_[i].yf + 6) * p * p);
            Concha(fb, x, y, 6, 4, bz_[i].aberto);
        }
    }

    // A CONCHA, unica. `aberto` = a fenda dentada virada para cima; caido
    // fechado e o dorso liso. Os dentes escalam com o raio, entao a mesma
    // funcao serve o buzio de 13 px e a marca de 61 px.
    static void Concha(uint8_t* fb, int cx, int cy, int ra, int rb, bool aberto)
    {
        if(ra >= 16) desenho::Elipse(fb, cx, cy, ra + 5, rb + 4, true, false);
        desenho::Elipse(fb, cx, cy, ra, rb, true, true);
        if(!aberto) { desenho::Linha(fb, cx-ra+2, cy, cx+ra-2, cy, false); return; }

        const int fa = ra * 3 / 4;                 // meia-largura da fenda
        const int fe = rb <= 4 ? 1 : rb / 3;       // meia-altura
        for(int y = -fe; y <= fe; y++)
            desenho::Linha(fb, cx - fa, cy + y, cx + fa, cy + y, false);
        // Dente GROSSO na marca, fino no buzio: a 61 px de largura uma
        // risca de 1 px com folga de 3 le-se como poeira, nao como dente.
        const int esp   = ra >= 16 ? 2 : 1;
        const int passo = esp * 2;
        for(int x = -fa + 1; x <= fa - 1; x += passo)
            for(int e = 0; e < esp && x + e <= fa - 1; e++)
                desenho::Linha(fb, cx + x + e, cy - fe, cx + x + e, cy + fe, true);
    }

    static const int  kVoo     = 14;     // quadros de queda por buzio
    static const int  kNomeLen = 13;
    static constexpr const char* kNome = "COTUCATRONICS";

    Tempo    t_       = kAbertura;
    Buzio    bz_[kBuzios] = {};
    uint32_t rng_     = 0x1F35Cu;
    int      quadro_  = 0;
    bool     ativo_   = false;
    char     versao_[16] = "v0.0.0";
};

}}  // namespace
