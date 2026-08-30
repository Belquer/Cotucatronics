// teste_tarol v0.2.0 — banco nativo do TAROLATOR.
//
// ATENCAO AO QUE ESTE BANCO PROVA, E AO QUE NAO PROVA.
//
// A v0.1.0 chamava-se teste_oitava e passou nos cinco casos com 0,0% de
// erro. Na placa, o dB: "nao parece um oitavador, parece ruido tipo
// caixa". O teste estava correcto e era IRRELEVANTE — provei o divisor com
// SENOIDES, e uma pancada de membrana nao tem fundamental: a medicao da
// fonte real, feita uma hora antes, achou-lhe oito modos INARMONICOS
// (278, 370, 406, 672, 732, 768, 809, 1061 Hz).
//
// BANCO NATIVO PROVA A ALGEBRA, NAO O TIMBRE. Cicatriz 15u.
//
// O que ele prova, e continua a valer como regressao:
//   - o flip-flop divide por dois em material PERIODICO
//   - o filtro de rastreio impede que o 2o harmonico dobre a contagem
//   - o portao cala no silencio e no ruido baixo
//   - a saida segue o envelope da entrada
//
// O que NAO prova: que soe bem, nem o que sai de material inarmonico —
// isso mediu-se na placa, com o ouvido do dB e o espectro.
#include "tarol.h"
#include <cstdio>
#include <cmath>
using namespace cotuca;
static const float SR = 48000.f;

// frequencia por contagem de cruzamentos ascendentes, na segunda metade
// (a primeira tem o transiente do envelope)
static float freq(const float* y, int n)
{
    int cruz = 0; bool cima = false; int ini = n/2, vistos = 0;
    for(int i = ini; i < n; i++)
    {
        if(!cima && y[i] > 0.f) { cima = true; cruz++; }
        if( cima && y[i] < 0.f) { cima = false; }
        vistos++;
    }
    return cruz * SR / (float)vistos;
}

static float rms(const float* y, int n)
{
    double s = 0; for(int i=n/2;i<n;i++) s += (double)y[i]*y[i];
    return (float)std::sqrt(s / (n - n/2));
}

int main()
{
    printf("teste_tarol v0.2.0 — divisor (o que ele faz em material PERIODICO)\n\n");
    const int N = (int)(SR * 0.5f);
    static float y[24000*2];
    int mau = 0;

    struct Caso { const char* nome; float f; float amp; float h2; float esp; };
    Caso casos[] = {
        {"membrana 768 Hz",        768.f, 0.30f, 0.0f, 384.f},
        {"corda 406 Hz",           406.f, 0.30f, 0.0f, 203.f},
        {"grave 200 Hz",           200.f, 0.30f, 0.0f, 100.f},
        {"768 Hz fraco (0.02)",    768.f, 0.02f, 0.0f, 384.f},
        {"768 Hz + 2o harmonico",  768.f, 0.30f, 0.5f, 384.f},
    };
    for(auto& c : casos)
    {
        Tarol o; o.Init(SR);
        for(int i = 0; i < N; i++)
        {
            const float t = (float)i / SR;
            float x = c.amp * std::sin(2*M_PI*c.f*t);
            if(c.h2 > 0.f) x += c.amp*c.h2 * std::sin(2*M_PI*2*c.f*t);
            y[i] = o.Processa(x);
        }
        const float f = freq(y, N);
        const float erro = 100.f * std::fabs(f - c.esp) / c.esp;
        const bool ok = erro < 6.f;
        if(!ok) mau++;
        printf("  %-24s -> %6.1f Hz (esperado %6.1f, erro %4.1f%%) rms %.4f  %s\n",
               c.nome, (double)f, (double)c.esp, (double)erro, (double)rms(y,N),
               ok ? "ok" : "*** FALHOU");
    }

    // PORTAO: silencio e ruido baixo NAO podem gerar sub
    printf("\n  portao (o sub tem de CALAR no silencio):\n");
    {
        Tarol o; o.Init(SR);
        for(int i = 0; i < N; i++) y[i] = o.Processa(0.f);
        const float r = rms(y, N);
        printf("    silencio absoluto  rms %.6f  %s\n", (double)r,
               r < 1e-5f ? "ok" : "*** FALHOU");
        if(r >= 1e-5f) mau++;
    }
    {
        Tarol o; o.Init(SR);
        unsigned s = 1;
        for(int i = 0; i < N; i++)
        {
            s = s*1664525u + 1013904223u;
            const float ru = ((float)(s>>8 & 0xFFFF)/32768.f - 1.f) * 0.001f;
            y[i] = o.Processa(ru);
        }
        const float r = rms(y, N);
        printf("    ruido a 0.001      rms %.6f  %s\n", (double)r,
               r < 1e-4f ? "ok" : "*** FALHOU (sub aleatorio no silencio)");
        if(r >= 1e-4f) mau++;
    }
    printf("\n%s\n", mau ? "FALHOU" : "TUDO PASSOU");
    return mau ? 1 : 0;
}
