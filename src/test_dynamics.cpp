// test_dynamics v0.1.0 — banco nativo da matematica de blocos.
//
// A versao entrou em 23/08, quando o `make check-docs` passou a
// cobrar o bump e revelou que este arquivo nunca teve uma. Regra da
// casa e que todo arquivo carrega versao visivel — e o que faltava
// nao era a regra, era a testemunha.
// Banco de teste nativo pra dynamics.h — roda no Mac, sem placa.
// dynamics.h é C++ puro e board-agnostic, então a matemática dos três
// blocos pode ser verificada aqui antes de gastar um ciclo de flash.
#include "dynamics.h"
#include <cstdio>
#include <cmath>

static const float SR = 48000.f;
static int fails = 0;

static void Check(const char* what, bool ok, const char* detail = "")
{
    printf("  %s %-52s %s\n", ok ? "OK  " : "FALHA", what, detail);
    if(!ok) fails++;
}

// Quantos ms leva pra o ganho cruzar `target` a partir do estado atual
template <typename F>
static float MsUntil(F&& step, float target, bool rising, float max_ms = 5000.f)
{
    const int max_n = (int)(max_ms * SR / 1000.f);
    for(int n = 0; n < max_n; n++)
    {
        const float g = step();
        if(rising ? (g >= target) : (g <= target))
            return n * 1000.f / SR;
    }
    return -1.f;
}

int main()
{
    printf("test_dynamics — validacao nativa de dynamics.h v%s\n\n",
           COTUCA_DYNAMICS_VERSION);

    // ---------- EnvFollower ----------
    printf("EnvFollower\n");
    {
        cotuca::EnvFollower e;
        e.Init(SR);
        e.SetTimes(10.f, 100.f);
        // degrau de 1.0: apos 1 constante de tempo deve estar ~63%
        const int n = (int)(0.010f * SR);
        for(int i = 0; i < n; i++) e.Process(1.f);
        const float v = e.Value();
        char d[64]; snprintf(d, sizeof(d), "env=%.4f (esperado ~0.632)", v);
        Check("degrau: 63% apos 1 tau de attack", v > 0.60f && v < 0.67f, d);

        // retificacao: entrada negativa gera envelope positivo
        cotuca::EnvFollower e2; e2.Init(SR); e2.SetTimes(1.f, 1000.f);
        for(int i = 0; i < 500; i++) e2.Process(-0.5f);
        snprintf(d, sizeof(d), "env=%.4f", e2.Value());
        Check("retifica entrada negativa", e2.Value() > 0.4f, d);
    }

    // ---------- Gate ----------
    printf("\nGate (thresh 0.02, attack 10ms, release 300ms)\n");
    {
        cotuca::Gate g;
        g.Init(SR);
        g.SetThreshold(0.02f);
        g.SetTimes(10.f, 300.f);

        // sinal bem abaixo do threshold: ganho deve ir a ~0
        for(int i = 0; i < (int)(2.f * SR); i++)
            g.Process(0.001f * sinf(i * 0.05f));
        char d[64]; snprintf(d, sizeof(d), "gain=%.5f", g.Gain());
        Check("chao de ruido (-60dBFS) fecha o gate", g.Gain() < 0.02f, d);

        // agora um sinal acima: deve abrir
        float t_open = MsUntil([&] {
            static int i = 0;
            return g.Process(0.3f * sinf(i++ * 0.05f)), g.Gain();
        }, 0.63f, true);
        snprintf(d, sizeof(d), "%.1f ms ate 63%% (alvo ~10)", t_open);
        Check("abre com attack ~10ms", t_open > 5.f && t_open < 25.f, d);

        // e o quanto do transiente passa no primeiro ms
        cotuca::Gate g2; g2.Init(SR); g2.SetThreshold(0.02f); g2.SetTimes(10.f, 300.f);
        float peak_1ms = 0.f;
        for(int i = 0; i < (int)(0.001f * SR); i++)
        {
            const float y = g2.Process(0.5f);
            if(fabsf(y) > peak_1ms) peak_1ms = fabsf(y);
        }
        snprintf(d, sizeof(d), "%.1f%% do sinal no 1o ms", 100.f * peak_1ms / 0.5f);
        Check("transiente: quanto passa no primeiro ms", true, d);
    }

    // ---------- Governor ----------
    printf("\nGovernor (ceiling 0.5, slope 1/4, knee 50%%)\n");
    {
        // abaixo do teto: nao deve tocar
        cotuca::Governor gv; gv.Init(SR); gv.SetCeiling(0.5f); gv.SetRelease(800.f);
        for(int i = 0; i < (int)(1.f * SR); i++) gv.Process(0.2f * sinf(i * 0.05f));
        char d[64]; snprintf(d, sizeof(d), "gain=%.4f", gv.Gain());
        Check("abaixo do teto: ganho fica em 1.0", gv.Gain() > 0.98f, d);

        // bem acima do teto: deve comprimir
        cotuca::Governor gv2; gv2.Init(SR); gv2.SetCeiling(0.5f); gv2.SetRelease(800.f);
        float peak = 0.f;
        for(int i = 0; i < (int)(2.f * SR); i++)
        {
            const float y = gv2.Process(1.0f * sinf(i * 0.05f));
            if(i > (int)(1.5f * SR) && fabsf(y) > peak) peak = fabsf(y);
        }
        snprintf(d, sizeof(d), "gain=%.3f, pico saida=%.3f", gv2.Gain(), peak);
        Check("acima do teto: comprime", gv2.Gain() < 0.9f && peak < 1.0f, d);

        // continuidade no joelho: varrer envelope e olhar saltos de ganho
        cotuca::Governor probe; probe.Init(SR); probe.SetCeiling(0.5f);
        float prev_out = 0.f, max_jump = 0.f;
        bool  first = true;
        for(float amp = 0.f; amp <= 1.2f; amp += 0.002f)
        {
            cotuca::Governor g3; g3.Init(SR); g3.SetCeiling(0.5f); g3.SetTimes(0.01f, 0.01f);
            for(int i = 0; i < 2000; i++) g3.Process(amp);
            const float out = amp * g3.Gain();
            if(!first) { const float j = fabsf(out - prev_out); if(j > max_jump) max_jump = j; }
            prev_out = out; first = false;
        }
        snprintf(d, sizeof(d), "maior salto=%.5f por passo de 0.002", max_jump);
        Check("curva monotona e sem degrau no joelho", max_jump < 0.005f, d);
    }

    // ---------- Limiter ----------
    printf("\nLimiter (ceiling 0.95, inegociavel)\n");
    {
        cotuca::Limiter l; l.Init(SR);
        float peak = 0.f;
        for(int i = 0; i < (int)(2.f * SR); i++)
        {
            const float y = l.Process(2.0f * sinf(i * 0.05f)); // +6dB acima do teto
            if(fabsf(y) > peak) peak = fabsf(y);
        }
        char d[64]; snprintf(d, sizeof(d), "pico=%.5f (teto=%.2f)", peak, cotuca::Limiter::Ceiling());
        Check("sinal 2x acima do teto nunca passa", peak <= 0.9501f, d);

        // transiente instantaneo: o clamp tem que pegar mesmo sem rampa
        cotuca::Limiter l2; l2.Init(SR);
        float first_sample = l2.Process(10.f);
        snprintf(d, sizeof(d), "1a amostra=%.4f", first_sample);
        Check("clamp pega transiente na 1a amostra", fabsf(first_sample) <= 0.9501f, d);

        // distorcao: quanto o limiter deforma um seno so 10% acima do teto
        cotuca::Limiter l3; l3.Init(SR);
        double num = 0, den = 0;
        for(int i = 0; i < (int)(1.f * SR); i++)
        {
            const float x = 1.05f * sinf(2.f * (float)M_PI * 440.f * i / SR);
            const float y = l3.Process(x);
            if(i > (int)(0.5f * SR))
            {
                const double ref = 0.95 * sin(2.0 * M_PI * 440.0 * i / SR);
                num += (y - ref) * (y - ref); den += ref * ref;
            }
        }
        const double thd = sqrt(num / den) * 100.0;
        snprintf(d, sizeof(d), "erro RMS vs seno ideal = %.2f%%", thd);
        Check("limiter suave a 10% acima do teto", thd < 15.0, d);
    }

    printf("\n%s (%d falha%s)\n", fails ? "REPROVOU" : "TUDO PASSOU",
           fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
