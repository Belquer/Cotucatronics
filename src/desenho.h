// desenho v0.1.0 — primitivas sobre o framebuffer de 1024 bytes.
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
#include "fonte5x7.h"

namespace cotuca { namespace desenho {

#define COTUCA_DESENHO_VERSION "0.1.0"

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
