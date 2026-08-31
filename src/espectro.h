#pragma once
// ============================================================
// Cotucatronics OS — espectro v0.2.0
// FFT radix-2 e cacador de pico. Ferramenta de DIAGNOSTICO, nao de
// audio: mora no laco principal, nunca no callback.
//
// ------------------------------------------------------------
// POR QUE NAO E AUTOCORRELACAO — e a prova que o mostrou.
//
// A primeira versao do cacador usava autocorrelacao, e falhou os SEIS
// casos da prova antes de chegar a placa. Duas razoes, e as duas sao
// de fundo:
//
//   1. A autocorrelacao tem pico em TODOS os multiplos do periodo, e
//      o maximo global cai num multiplo alto. Uma senoide de 848 Hz
//      saia como 169,6 — que e 848/5, o quinto periodo.
//
//   2. Mais grave: ela mede PERIODICIDADE, e nao qual parcial e mais
//      forte. Com a corda a 311 Hz e a microfonia a 1900, ela
//      responderia sobre a corda — a pergunta errada, respondida bem.
//
// O que se quer e o PICO ESPECTRAL MAIS FORTE, e para isso a
// ferramenta e a transformada. A licao fica: escolher o metodo pela
// pergunta, e nao pela familiaridade.
// ------------------------------------------------------------
//
// JANELA DE HANN, e nao retangular. Um tom que nao cabe num numero
// inteiro de ciclos na janela espalha-se pelos vizinhos todos
// ("vazamento"), e um vazamento largo enterra um pico vizinho mais
// fraco. Hann troca resolucao por limpeza, e aqui a limpeza vale mais:
// procura-se UM pico dominante, nao dois quase iguais.
//
// INTERPOLACAO PARABOLICA EM dB. O passo do bin a 48 kHz com N=1024 e
// 46,9 Hz — quase a largura inteira do notch. Sem interpolar, o notch
// erraria o alvo por meia largura com frequencia. Em dB (e nao em
// amplitude) porque o pico de um tom janelado e aproximadamente uma
// parabola na escala logaritmica, e nao na linear.
//
// C++ puro: sem alocacao, sem hal. Os buffers vem de fora.
// ============================================================

#include <cstddef>
#include <cmath>

namespace cotuca
{

class Espectro
{
  public:
    // `n` tem de ser potencia de 2. `re` e `im` sao do chamador, com
    // n floats cada — este arquivo nao aloca nada.
    // Devolve a frequencia do pico mais forte entre `hz_lo` e `hz_hi`,
    // ou -1 se nao houver energia. `nivel_db` sai preenchido.
    static float Pico(float* re, float* im, int n, float sr,
                      float hz_lo, float hz_hi, float* nivel_db,
                      float* domin)
    {
        // 1) JANELA. Aplicada ao sinal ja em `re`; `im` zera.
        for(int i = 0; i < n; i++)
        {
            const float w = 0.5f * (1.f - cosf(2.f * kPi * (float)i / (float)(n - 1)));
            re[i] *= w;
            im[i] = 0.f;
        }

        Fft(re, im, n);

        // 2) MAGNITUDE ao quadrado, so metade (o resto e espelho).
        const int metade = n / 2;
        for(int i = 0; i < metade; i++)
            re[i] = re[i] * re[i] + im[i] * im[i];

        // ARREDONDA PARA CIMA, e nao trunca: `(int)` de 2,13 dava o bin
        // 2, e a busca comecava ABAIXO do `hz_lo` prometido. Instrumento
        // de medida que nao respeita o proprio limite mente na borda.
        int lo = (int)(hz_lo * (float)n / sr + 0.999f);
        int hi = (int)(hz_hi * (float)n / sr);
        // PISO DE QUATRO BINS. Os primeiros bins carregam o que sobra do
        // DC e o ronco da estrutura, espalhados pela janela de Hann. Um
        // pico ali e quase sempre resto, e nao modo — e foi o que a
        // primeira caca real apanhou.
        if(lo < 4) lo = 4;
        if(hi > metade - 3) hi = metade - 3;
        if(hi <= lo) { if(nivel_db) *nivel_db = -200.f; return -1.f; }

        int   pico = -1; float mx = 0.f; float soma = 0.f;
        // A SOMA COBRE UM BIN A MAIS DE CADA LADO, para que os vizinhos
        // do pico estejam SEMPRE dentro do denominador. Sem isto, um
        // pico na borda dava dominancia > 1 — impossivel, e foi o que
        // denunciou o defeito.
        for(int i = lo - 1; i <= hi + 1; i++) soma += re[i];
        for(int i = lo; i <= hi; i++)
            if(re[i] > mx) { mx = re[i]; pico = i; }
        if(pico < 0 || mx <= 1e-20f) { if(nivel_db) *nivel_db = -200.f; return -1.f; }

        // 3) INTERPOLACAO PARABOLICA EM dB.
        const float lm = Db(re[pico - 1]);
        const float lc = Db(mx);
        const float lp = Db(re[pico + 1]);
        float       d  = 0.f;
        const float den = lm - 2.f * lc + lp;
        if(den != 0.f) d = 0.5f * (lm - lp) / den;
        if(d > 1.f || d < -1.f) d = 0.f;

        if(nivel_db) *nivel_db = lc;
        // DOMINANCIA: que fatia da energia da banda esta neste pico e
        // nos dois vizinhos. Perto de 1 = um espeto sozinho, que e a
        // assinatura da microfonia. Espalhado = som de instrumento.
        if(domin)
        {
            float d = soma > 0.f ? (re[pico - 1] + mx + re[pico + 1]) / soma : 0.f;
            *domin = d > 1.f ? 1.f : d;   // trava: fraccao nao passa de 1
        }

        return ((float)pico + d) * sr / (float)n;
    }

  private:
    static constexpr float kPi = 3.14159265358979f;

    static inline float Db(float pot)
    {
        return pot > 1e-20f ? 10.f * log10f(pot) : -200.f;
    }

    // FFT radix-2, decimacao no tempo, no lugar.
    // Escrita a mao de proposito: sao trinta linhas e evita arrastar
    // uma biblioteca inteira para dentro de uma ferramenta de bancada.
    static void Fft(float* re, float* im, int n)
    {
        // Permutacao por inversao de bits.
        for(int i = 1, j = 0; i < n; i++)
        {
            int bit = n >> 1;
            for(; j & bit; bit >>= 1) j ^= bit;
            j |= bit;
            if(i < j)
            {
                float t = re[i]; re[i] = re[j]; re[j] = t;
                t = im[i]; im[i] = im[j]; im[j] = t;
            }
        }
        for(int len = 2; len <= n; len <<= 1)
        {
            const float ang = -2.f * kPi / (float)len;
            const float wr  = cosf(ang), wi = sinf(ang);
            for(int i = 0; i < n; i += len)
            {
                // Recorrencia em vez de tabela: poupa memoria, e o erro
                // acumulado sobre `len/2` passos e irrelevante para
                // achar um pico (nao se esta a reconstruir o sinal).
                float cr = 1.f, ci = 0.f;
                for(int j = 0; j < len / 2; j++)
                {
                    const int a = i + j, b = i + j + len / 2;
                    const float ur = re[a], ui = im[a];
                    const float vr = re[b] * cr - im[b] * ci;
                    const float vi = re[b] * ci + im[b] * cr;
                    re[a] = ur + vr; im[a] = ui + vi;
                    re[b] = ur - vr; im[b] = ui - vi;
                    const float nr = cr * wr - ci * wi;
                    ci = cr * wi + ci * wr;
                    cr = nr;
                }
            }
        }
    }
};

} // namespace cotuca
