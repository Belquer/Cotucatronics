// teste_macro v0.1.0 — a ida e a volta da Macro, que a captura do pot usa.
//
// POR QUE ISTO EXISTE. O pot e ABSOLUTO e o param e partilhado com o MIDI:
// para nao saltar, ele so assume quando PASSA pelo valor actual. Saber por
// onde passar exige traduzir o param de volta para posicao de knob, e e o
// `Macro::Inverso` que o faz.
//
// Se ele estiver errado o defeito nao e visivel: o pot simplesmente "nao
// pega", ou pega no sitio errado, e a bancada diagnostica um ADC que esta
// bom. Ida-e-volta e barato de provar e caro de descobrir na placa.
#include "params.h"
#include <cstdio>
#include <cmath>
using namespace cotuca;

static int mau = 0;

static void volta(const char* rot, float peso)
{
    ParamTable t;
    const int id = t.Add("alvo", 0.0f, 0.f, 1.f);
    Macro m; m.AddTarget(id, peso);
    float pior = 0.f; float onde = 0.f;
    for(int i = 0; i <= 100; i++)
    {
        const float v = i / 100.f;
        m.Apply(t, v);
        const float volta = m.Inverso(t.Get(id));
        const float e = std::fabs(volta - v);
        if(e > pior) { pior = e; onde = v; }
    }
    const bool ok = pior < 0.002f;
    if(!ok) mau++;
    printf("  %-22s peso %+5.2f   pior erro %.5f (em v=%.2f)  %s\n",
           rot, (double)peso, (double)pior, (double)onde, ok ? "ok" : "*** FALHOU");
}

int main()
{
    printf("teste_macro v0.1.0 — Apply seguido de Inverso devolve a posicao\n\n");
    volta("direto (level_l/level_r)",  1.0f);
    volta("meio curso",            0.5f);
    volta("invertido",            -1.0f);
    volta("invertido parcial",    -0.5f);

    printf("\n  casos de fronteira:\n");
    {
        ParamTable t; Macro m;
        printf("    macro VAZIA          alvo %d, inverso %.1f  %s\n",
               m.PrimeiroAlvo(), (double)m.Inverso(0.5f),
               (m.PrimeiroAlvo() < 0 && m.Inverso(0.5f) < 0.f) ? "ok" : "*** FALHOU");
        if(!(m.PrimeiroAlvo() < 0 && m.Inverso(0.5f) < 0.f)) mau++;
    }
    {
        ParamTable t; const int id = t.Add("z", 0.f, 0.f, 1.f);
        Macro m; m.AddTarget(id, 0.f);
        // PESO ZERO NAO TEM INVERSO, e devolver 0 seria pior que devolver
        // -1: o laco trataria 0 como "posicao valida no fundo" e o pot
        // assumiria o param sempre que passasse pelo zero.
        printf("    peso ZERO            inverso %.1f  %s\n",
               (double)m.Inverso(0.5f), m.Inverso(0.5f) < 0.f ? "ok" : "*** FALHOU");
        if(!(m.Inverso(0.5f) < 0.f)) mau++;
    }
    printf("\n%s\n", mau ? "FALHOU" : "TUDO PASSOU");
    return mau ? 1 : 0;
}
