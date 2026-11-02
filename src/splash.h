// splash v0.11.1 — a abertura: jogada de buzios, uma concha cresce, vira
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
#include <math.h>
#include "desenho.h"

namespace cotuca { namespace splash {

#define COTUCA_SPLASH_VERSION "0.11.1"

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
            //    as letras. Pedido do dB em 2026-09-02.
            //
            //    Agora ra=26/rb=11 -> anel em (31,15), centro em cy=17:
            //    a marca ocupa y=2..32 e a folga passa a OITO pixeis.
            // proporcao ~1,7:1 na marca (era 2,4:1), pela foto
            // marca um pouco MENOR, a pedido do dB: 26 -> 22 de raio
            const int   ra = 7  + (int)(15.f * p + 0.5f);
            const int   rb = 5  + (int)( 8.f * p + 0.5f);
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
    // A CONCHA, unica. `aberto` = a fenda dentada virada para cima; caido
    // fechado e o dorso liso. Serve o buzio de 13 px e a marca de 61.
    //
    // ⚠️ REDESENHADA EM 2026-09-03, com buzios de verdade a frente.
    //
    // Era uma ELIPSE com a fenda ao centro, e o dB viu logo o que estava
    // mal: "me lembra um pouco uma bola de futebol americano". Estava
    // certo — elipse pura e uma lente com bicos, e um buzio nao tem bicos.
    //
    // Tres correcoes, todas da foto:
    //
    //   1. SUPERELIPSE em vez de elipse (n = 2,6). Enche os lados e
    //      arredonda as pontas — "os buzios tem a lateral um pouco mais
    //      arredondada".
    //   2. MENOS ALONGADA: a proporcao passa de ~2,4:1 para ~1,7:1, que e
    //      a da foto.
    //   3. FENDA ASSIMETRICA: descentrada para baixo e mais curta que a
    //      concha — "a abertura e assimetrica". Na elipse antiga ela
    //      atravessava a peca ao meio, que e o que fazia a costura de bola.
    static void Concha(uint8_t* fb, int cx, int cy, int ra, int rb, bool aberto)
    {
        // Anatomia do buzio, do desenho do dB (IMG_5910, 2026-09-04). Nao e uma
        // elipse com um corte: sao TRES pecas encaixadas.
        //
        //   1. o CASCO e um contorno vazado, nao um vulto macico;
        //   2. dentro dele flutua a massa do LABIO, cheia — pontuda numa
        //      ponta e bojuda na outra;
        //   3. so dentro do labio e que abre a BOCA, com dentes de cima e
        //      de baixo que se alternam e nunca se tocam.
        //
        // Eu tinha as tres coladas numa peca so, e por isso lia bola.
        // n = 3,2 achata o topo e a base e arredonda os cantos: o casco
        // le RETANGULO ARREDONDADO, nao elipse. Eu tinha-o em 2,2 por ter
        // lido "laterais mais pontiagudas" como os BICOS quando o dB
        // falava das arestas longas — inferencia minha sobre a palavra.
        // O render que ele mandou a seguir resolveu em dois segundos o que
        // eu vinha a adivinhar: imagem e medicao, leitura de palavra nao e.
        const float kN = 3.2f;

        // pequeno demais para anatomia: um vulto cheio com um risco
        if(ra < 12)
        {
            desenho::SuperElipse(fb, cx, cy, ra, rb, kN, true, true);
            desenho::Linha(fb, cx - ra + 2, cy, cx + ra - 2, cy, false);
            return;
        }

        // 1. casco vazado: CHEIO MENOS CHEIO, e nao contorno + contorno.
        //
        // ⚠️ Engrossar um contorno desenhando-o duas vezes com raios
        // diferentes parece bater certo e nao bate: a TAMPA do interno cai
        // na mesma linha que o ARCO do externo, e as duas nao se encontram.
        // Saiam 3 px de buraco de cada lado na penultima linha — "a linha
        // debaixo esta interrompida", 2026-09-04. Dois tracos vizinhos so se
        // tocam onde a curva anda devagar; nos topos ela salta.
        //
        // Um anel feito de cheio-menos-cheio nao pode abrir, porque nao ha
        // duas curvas para casar: ha uma regiao e um buraco dentro dela.
        desenho::SuperElipse(fb, cx, cy, ra,     rb,     kN, true,  true);
        desenho::SuperElipse(fb, cx, cy, ra - 2, rb - 2, kN, false, true);
        if(!aberto) return;

        // 2. o labio. A boca fica CENTRADA no casco (pedido do dB), e a
        // assimetria vive na FORMA, nao na posicao: o envelope sin(pi*t^k)
        // sobe devagar de um lado e engorda no outro, o que da a ponta fina
        // numa ponta e o bojo na outra.
        //
        // k=1,5 — ESCOLHIDO PELO OLHO DELE numa escala medida, nao
        // deduzido por mim. O caminho foi 2 -> 1,3 -> 1 (simetrica) -> 1,15
        // -> 1,5, e o criterio final foi dele e era mensuravel: "a linha
        // horizontal do canto esquerdo mais longa do que a do direito".
        //
        // Em px de rabo macico (esquerda/direita): k=1 da 4/4, k=1,15 da
        // 7/4, k=1,5 da 10/4, k=1,8 da 11/4. Acima de ~2,2 satura em 16/3 —
        // o rabo bate no fim do labio e o expoente deixa de o alongar.
        // Para ir alem disso mexe-se em `la`, nao aqui.
        //
        // Nao substituir por sin(pi*t): isso e k=1, e ja foi recusado.
        const int   la = (ra * 4) / 5;          // meio-comprimento
        const int   lb = (rb * 3) / 5;          // meia-altura no bojo
        const int   el = ra >= 20 ? 2 : 1;      // espessura do labio
        const int   passo = ra >= 20 ? 3 : 2;   // largura de um dente

        for(int x = -la; x <= la; x++)
        {
            const float t = (float)(x + la) / (float)(2 * la);
            int h = (int)(lb * sinf(3.14159265f * powf(t, 1.50f)) + 0.5f);
            if(h < 0) h = 0;
            for(int y = -h; y <= h; y++)
                desenho::Pixel(fb, cx + x, cy + y, true);
        }

        // 3. a boca, aberta por dentro do labio
        for(int x = -la + el; x <= la - el; x++)
        {
            const float t = (float)(x + la) / (float)(2 * la);
            const int   h = (int)(lb * sinf(3.14159265f * powf(t, 1.50f)) + 0.5f);
            const int   ha = h - el;            // meia-altura da abertura

            // ⚠️ O RABO E LINHA CHEIA. Ate a v0.10.0 este risco escuro
            // corria por TODO o labio, inclusive onde nao ha vao nenhum —
            // eu tinha-o posto de proposito, para "ligar a ponta magra a
            // boca larga". O efeito real era partir em dois o traco que
            // vai do canto ao centro: "sem a interrupcao da linha do canto
            // esquerdo ao centro", 2026-09-04.
            //
            // A boca abre onde HA boca. No rabo, o labio fica macico.
            if(ha < 1) continue;
            for(int y = -ha; y <= ha; y++)
                desenho::Pixel(fb, cx + x, cy + y, false);

            // dentes EMPARELHADOS: onde ha dente, ha em cima E em baixo,
            // um por cima do outro. Ate a v0.10.0 alternavam, e alternar e
            // uma assimetria — a marca nao fechava no espelho horizontal.
            //
            // Eles MORDEM SO UM PEDACO (d = metade do vao). Dente que
            // atravessa a abertura toda le listra; o que faz ler boca e o
            // canal escuro que sobra a correr no meio, entre as duas
            // fileiras. E e esse canal que garante que nao se tocam.
            // ⚠️ VAO DE 1 px NAO DA DENTE, DA COCEGA. Onde a boca so agora
            // comeca a abrir, um "dente" tem 1 px e sai separado do rabo
            // por uma coluna escura — le ponto solto, e era o que ainda
            // partia a linha que vai do canto ao centro depois de eu ja ter
            // consertado o risco. O rabo entra na boca inteiro; os dentes
            // comecam onde ha vao para eles.
            if(ha < 2) continue;
            int d = ha / 2; if(d < 1) d = 1;
            // ⚠️ A FASE CONTA-SE DO CENTRO, nao da ponta. Contada de `-la`,
            // a fileira so por acaso cairia espelhada — e era ela que ainda
            // torcia a marca depois de o labio ja estar simetrico.
            const int ax = (x < 0 ? -x : x);
            if((((ax + passo / 2) / passo) % 2) != 0) continue;
            for(int y = -ha; y <= -ha + d - 1; y++)
                desenho::Pixel(fb, cx + x, cy + y, true);
            for(int y = ha - d + 1; y <= ha; y++)
                desenho::Pixel(fb, cx + x, cy + y, true);
        }
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
