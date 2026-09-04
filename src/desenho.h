// desenho v0.3.1 — primitivas sobre o framebuffer de 1024 bytes.
//
// C++ puro, SEM hardware: nada aqui toca em periferico, e por isso tudo
// isto se testa no computador antes de existir vidro. Um elipse torto no
// painel nao diz se errou a matematica ou a SPI; testado em terra, quando
// o painel mostra torto so pode ser o painel.
//
// Formato: SSD1306, buffer[x + (y/8)*128], bit (y%8). Byte = 8 pixeis
// VERTICAIS. E por isso que a fonte guarda COLUNAS: Texto() copia sem
// conta nenhuma quando o glifo cai alinhado com a pagina.
#pragma once
#include <stdint.h>
#include <cmath>
#include "fonte5x7.h"

namespace cotuca { namespace desenho {

#define COTUCA_DESENHO_VERSION "0.3.1"

static const int kL = 128;   // largura
static const int kA = 64;    // altura
static const int kBytes = (kL * kA) / 8;

inline void Limpa(uint8_t* fb) { for(int i = 0; i < kBytes; i++) fb[i] = 0; }

// TODO desenho passa por aqui, e e aqui que o clamp mora — um so ponto de
// recorte em vez de um `if` por primitiva, que e como se esquece um.
inline void Pixel(uint8_t* fb, int x, int y, bool on)
{
    if(x < 0 || x >= kL || y < 0 || y >= kA) return;
    uint8_t& b = fb[x + (y >> 3) * kL];
    const uint8_t m = (uint8_t)(1u << (y & 7));
    if(on) b |= m; else b = (uint8_t)(b & ~m);
}

inline bool Le(const uint8_t* fb, int x, int y)
{
    if(x < 0 || x >= kL || y < 0 || y >= kA) return false;
    return (fb[x + (y >> 3) * kL] >> (y & 7)) & 1;
}

// Bresenham inteiro: sem float no caminho, entao o mesmo desenho sai
// identico aqui e na placa — o teste em terra vale como prova.
inline void Linha(uint8_t* fb, int x0, int y0, int x1, int y1, bool on = true)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for(;;)
    {
        Pixel(fb, x0, y0, on);
        if(x0 == x1 && y0 == y1) break;
        const int e2 = err * 2;
        if(e2 > -dy) { err -= dy; x0 += sx; }
        if(e2 <  dx) { err += dx; y0 += sy; }
    }
}

inline void Retangulo(uint8_t* fb, int x0, int y0, int x1, int y1, bool on = true)
{
    Linha(fb, x0, y0, x1, y0, on);  Linha(fb, x0, y1, x1, y1, on);
    Linha(fb, x0, y0, x0, y1, on);  Linha(fb, x1, y0, x1, y1, on);
}

inline void Caixa(uint8_t* fb, int x0, int y0, int x1, int y1, bool on = true)
{
    for(int y = y0; y <= y1; y++) Linha(fb, x0, y, x1, y, on);
}

// Elipse por ponto medio, inteiro. `ra`/`rb` sao RAIOS, nao diametros.
inline void Elipse(uint8_t* fb, int cx, int cy, int ra, int rb,
                   bool on = true, bool cheia = false)
{
    if(ra <= 0 || rb <= 0) return;
    long a2 = (long)ra * ra, b2 = (long)rb * rb;
    long x = 0, y = rb;
    long d1 = b2 - a2 * rb + a2 / 4;
    long dx = 0, dy = 2 * a2 * y;
    while(dx < dy)
    {
        if(cheia) { Linha(fb,(int)(cx-x),(int)(cy+y),(int)(cx+x),(int)(cy+y),on);
                    Linha(fb,(int)(cx-x),(int)(cy-y),(int)(cx+x),(int)(cy-y),on); }
        else { Pixel(fb,(int)(cx+x),(int)(cy+y),on); Pixel(fb,(int)(cx-x),(int)(cy+y),on);
               Pixel(fb,(int)(cx+x),(int)(cy-y),on); Pixel(fb,(int)(cx-x),(int)(cy-y),on); }
        x++; dx += 2 * b2; d1 += b2 + dx;
        if(d1 >= 0) { y--; dy -= 2 * a2; d1 -= dy; }
    }
    long d2 = b2*(x*2+1)*(x*2+1)/4 + a2*(y-1)*(y-1) - a2*b2;
    while(y >= 0)
    {
        if(cheia) { Linha(fb,(int)(cx-x),(int)(cy+y),(int)(cx+x),(int)(cy+y),on);
                    Linha(fb,(int)(cx-x),(int)(cy-y),(int)(cx+x),(int)(cy-y),on); }
        else { Pixel(fb,(int)(cx+x),(int)(cy+y),on); Pixel(fb,(int)(cx-x),(int)(cy+y),on);
               Pixel(fb,(int)(cx+x),(int)(cy-y),on); Pixel(fb,(int)(cx-x),(int)(cy-y),on); }
        y--; dy -= 2 * a2; d2 += a2 - dy;
        if(d2 <= 0) { x++; dx += 2 * b2; d2 += dx; }
    }
}

// SUPERELIPSE — |x/a|^n + |y/b|^n = 1.
//
// Com n=2 e uma elipse. Acima disso os lados ENCHEM e as pontas ficam
// ROMBAS, que e a silhueta de um buzio: a elipse pura da uma lente com
// bicos, e o dB disse que lhe lembrava uma bola de futebol americano.
// Tinha razao — n=2 e exactamente essa forma.
//
// Varre por LINHA e desenha o intervalo: uma raiz por linha, no maximo 64
// linhas. Nao e caminho quente e nao ha `powf` por pixel.
inline void SuperElipse(uint8_t* fb, int cx, int cy, int ra, int rb,
                        float n, bool on = true, bool cheia = true)
{
    if(ra <= 0 || rb <= 0) return;
    int ant_x = -1;
    for(int y = -rb; y <= rb; y++)
    {
        // A LINHA E UMA FAIXA, NAO UMA ARESTA. Medindo `ty` na aresta
        // (|y|/rb), a linha do topo cai exactamente em ty=1, onde a
        // superelipse tem x=0 — e sai um espigao de 1 pixel por cima da
        // barra chata. Quanto mais romba a forma, mais o espigao se ve:
        // com n=3,2 o dB apanhou-o logo, "um quadradinho branco no topo".
        //
        // A largura de uma linha de pixels e a da sua parte MAIS LARGA, que
        // esta meia linha para dentro. Medir ai e o conserto, e vale para
        // todo n e para cheia e contorno ao mesmo tempo.
        float ay = (float)(y < 0 ? -y : y) - 0.5f;
        if(ay < 0.f) ay = 0.f;
        const float ty = ay / (float)rb;
        float u = 1.f - powf(ty, n);
        if(u < 0.f) u = 0.f;
        const int x = (int)(ra * powf(u, 1.f / n) + 0.5f);
        if(cheia)
            Linha(fb, cx - x, cy + y, cx + x, cy + y, on);
        else if(y == -rb || y == rb)
        {
            // ⚠️ A TAMPA E UM TRACO. Medir a faixa (acima) tirou o espigao
            // do apice — mas era esse espigao que fechava o contorno por
            // cima: a linha seguinte fazia a ponte de x=0 ate a sua
            // largura e dai saia a barra chata. Sem ele, a primeira e a
            // ultima linha ficavam com dois pixels soltos e o casco abria.
            // Conserto que remove um sintoma e abre um buraco ao lado.
            Linha(fb, cx - x, cy + y, cx + x, cy + y, on);
        }
        else
        {
            Pixel(fb, cx - x, cy + y, on);
            Pixel(fb, cx + x, cy + y, on);
            // fecha o contorno quando a largura salta entre linhas, senao
            // as pontas rombas saem como pontilhado
            if(ant_x >= 0)
                for(int q = (x < ant_x ? x : ant_x); q <= (x > ant_x ? x : ant_x); q++)
                { Pixel(fb, cx - q, cy + y, on); Pixel(fb, cx + q, cy + y, on); }
        }
        ant_x = x;
    }
}

// Texto 5x7 com 1 px de espaco: passo de 6 px. `esc` amplia por bloco.
// Devolve o x FINAL, para se poder encadear sem recontar caracteres.
inline int Texto(uint8_t* fb, int x, int y, const char* s, int esc = 1,
                 bool on = true)
{
    for(; *s; s++)
    {
        const uint8_t* g = fonte::Glifo(*s);
        for(int c = 0; c < 5; c++)
            for(int r = 0; r < 7; r++)
                if((g[c] >> r) & 1)
                    for(int i = 0; i < esc; i++)
                        for(int j = 0; j < esc; j++)
                            Pixel(fb, x + c*esc + i, y + r*esc + j, on);
        x += 6 * esc;
    }
    return x;
}

// Largura que um texto VAI ocupar — para centrar sem adivinhar.
inline int Largura(const char* s, int esc = 1)
{
    int n = 0; for(const char* p = s; *p; p++) n++;
    return n ? (n * 6 * esc - esc) : 0;   // sem o espaco do ultimo
}

inline int TextoCentrado(uint8_t* fb, int y, const char* s, int esc = 1,
                         bool on = true)
{
    return Texto(fb, (kL - Largura(s, esc)) / 2, y, s, esc, on);
}

}}  // namespace
