// test_midi v0.1.1 — validação nativa do parser MIDI e do mapa.
//
// midi.h é C++ puro e board-agnostic, e manifest.h também (só inclui
// params.h, module.h e midi.h). Então a máquina de estados E a via
// completa CC→ParamTable rodam no Mac, sem placa.
//
// O QUE ISTO PROVA: o parser, o mapa do manifest e a normalização.
// O QUE NÃO PROVA: a UART — baud rate, pino, DMA, fila. Isso só o
// loopback com jumper prova (T10 no TESTES.md). Um parser perfeito
// alimentado por uma UART a 9600 baud não recebe byte nenhum.
#include "midi.h"
#include "params.h"
#include "manifest.h"
#include <cstdio>
#include <cmath>

static int fails = 0;

static void Check(const char* what, bool ok, const char* detail = "")
{
    printf("  %s %-56s %s\n", ok ? "OK  " : "FALHA", what, detail);
    if(!ok)
        fails++;
}

// ---- captura o que o parser despachou ----
static int     n_cc = 0;
static uint8_t last_ch, last_cc, last_val;

static void Capture(uint8_t ch, uint8_t cc, uint8_t val)
{
    n_cc++;
    last_ch  = ch;
    last_cc  = cc;
    last_val = val;
}

static void Feed(cotuca::MidiParser& p, const uint8_t* b, int n)
{
    for(int i = 0; i < n; i++)
        p.Byte(b[i]);
}

int main()
{
    printf("test_midi — validacao nativa de midi.h v%s\n\n",
           COTUCA_MIDI_VERSION);
    char d[96];

    // ---------- CC básico ----------
    printf("Control Change basico\n");
    {
        cotuca::MidiParser p;
        p.Init(cotuca::kMidiOmni, Capture);
        n_cc = 0;
        const uint8_t msg[] = {0xB0, 20, 64};
        Feed(p, msg, 3);
        snprintf(d, sizeof(d), "n=%d cc=%d val=%d ch=%d",
                 n_cc, last_cc, last_val, last_ch);
        Check("B0 14 40 vira um CC 20 = 64 no canal 0",
              n_cc == 1 && last_cc == 20 && last_val == 64 && last_ch == 0, d);
    }

    // ---------- running status ----------
    printf("\nRunning status\n");
    {
        cotuca::MidiParser p;
        p.Init(cotuca::kMidiOmni, Capture);
        n_cc = 0;
        // um status, TRES mensagens — e o caso comum de um knob varrendo
        const uint8_t msg[] = {0xB0, 20, 0, 20, 64, 20, 127};
        Feed(p, msg, 7);
        snprintf(d, sizeof(d), "n=%d ultimo val=%d", n_cc, last_val);
        Check("status omitido: 3 CCs de um B0 so", n_cc == 3 && last_val == 127, d);
    }

    // ---------- real-time no meio ----------
    printf("\nReal-time intercalado\n");
    {
        cotuca::MidiParser p;
        p.Init(cotuca::kMidiOmni, Capture);
        n_cc = 0;
        // 0xF8 (clock) ENTRE os dois bytes de dados — legal em MIDI
        const uint8_t msg[] = {0xB0, 20, 0xF8, 64};
        Feed(p, msg, 4);
        snprintf(d, sizeof(d), "n=%d val=%d realtime=%lu",
                 n_cc, last_val, (unsigned long)p.RealTime());
        Check("clock entre os data bytes nao quebra o CC",
              n_cc == 1 && last_val == 64 && p.RealTime() == 1, d);

        // e nao apaga o running status
        n_cc = 0;
        const uint8_t m2[] = {0xF8, 20, 100};
        Feed(p, m2, 3);
        snprintf(d, sizeof(d), "n=%d val=%d", n_cc, last_val);
        Check("clock nao apaga o running status", n_cc == 1 && last_val == 100, d);
    }

    // ---------- mensagens ignoradas nao desalinham ----------
    printf("\nMensagens ignoradas (o ponto todo do parser)\n");
    {
        cotuca::MidiParser p;
        p.Init(cotuca::kMidiOmni, Capture);
        n_cc = 0;
        // note on (3 bytes), pitch bend (3), program change (2), depois um CC
        const uint8_t msg[] = {0x90, 60, 100,   // note on
                               0xE0, 0,  64,    // pitch bend
                               0xC0, 5,         // program change: 1 byte!
                               0xD0, 90,        // channel pressure: 1 byte
                               0xB0, 21, 32};   // o CC que interessa
        Feed(p, msg, 13);
        snprintf(d, sizeof(d), "n=%d cc=%d val=%d outras=%lu",
                 n_cc, last_cc, last_val, (unsigned long)p.Other());
        Check("nota/bend/pgm/pressure engolidos, CC seguinte intacto",
              n_cc == 1 && last_cc == 21 && last_val == 32 && p.Other() == 4, d);
    }

    // ---------- sysex ----------
    printf("\nSysex\n");
    {
        cotuca::MidiParser p;
        p.Init(cotuca::kMidiOmni, Capture);
        n_cc = 0;
        // corpo de sysex e todo < 0x80: sem estado, viraria data solto
        const uint8_t msg[] = {0xB0, 20, 10,               // CC legitimo
                               0xF0, 1, 2, 3, 20, 64, 0xF7, // dump
                               0xB0, 22, 99};              // CC depois
        Feed(p, msg, 13);
        snprintf(d, sizeof(d), "n=%d cc=%d val=%d", n_cc, last_cc, last_val);
        Check("corpo de sysex nao vira CC, e o CC seguinte funciona",
              n_cc == 2 && last_cc == 22 && last_val == 99, d);
    }
    {
        // sysex interrompido por status, sem 0xF7 — cabo arrancado
        cotuca::MidiParser p;
        p.Init(cotuca::kMidiOmni, Capture);
        n_cc = 0;
        const uint8_t msg[] = {0xF0, 1, 2, 3, 0xB0, 20, 55};
        Feed(p, msg, 7);
        snprintf(d, sizeof(d), "n=%d val=%d in_sysex=%d",
                 n_cc, last_val, (int)p.InSysex());
        Check("sysex sem F7, cortado por status, nao trava o parser",
              n_cc == 1 && last_val == 55 && !p.InSysex(), d);
    }

    // ---------- data orfao ----------
    printf("\nFluxo pego no meio\n");
    {
        cotuca::MidiParser p;
        p.Init(cotuca::kMidiOmni, Capture);
        n_cc = 0;
        const uint8_t msg[] = {40, 50, 60, 0xB0, 20, 70}; // 3 dados sem status
        Feed(p, msg, 6);
        snprintf(d, sizeof(d), "n=%d orfaos=%lu val=%d",
                 n_cc, (unsigned long)p.Orphan(), last_val);
        Check("dados sem status sao descartados e contados",
              n_cc == 1 && p.Orphan() == 3 && last_val == 70, d);
    }

    // ---------- filtro de canal ----------
    printf("\nFiltro de canal\n");
    {
        cotuca::MidiParser p;
        p.Init(2, Capture); // canal 2 do fio = canal 3 no controlador
        n_cc = 0;
        const uint8_t msg[] = {0xB0, 20, 11,   // canal 0: fora
                               0xB2, 20, 22,   // canal 2: passa
                               0xB5, 20, 33};  // canal 5: fora
        Feed(p, msg, 9);
        snprintf(d, sizeof(d), "n=%d val=%d alheios=%lu",
                 n_cc, last_val, (unsigned long)p.Filtered());
        Check("so o canal configurado passa", n_cc == 1 && last_val == 22
                                                  && p.Filtered() == 2, d);
    }
    {
        cotuca::MidiParser p;
        p.Init(cotuca::kMidiOmni, Capture);
        n_cc = 0;
        const uint8_t msg[] = {0xB0, 20, 11, 0xB7, 20, 22, 0xBF, 20, 33};
        Feed(p, msg, 9);
        snprintf(d, sizeof(d), "n=%d alheios=%lu", n_cc, (unsigned long)p.Filtered());
        Check("omni aceita os 16 canais", n_cc == 3 && p.Filtered() == 0, d);
    }

    // ---------- normalizacao ----------
    printf("\nNormalizacao 0-127 -> 0-1\n");
    {
        snprintf(d, sizeof(d), "0->%.4f  64->%.4f  127->%.4f",
                 cotuca::MidiNorm(0), cotuca::MidiNorm(64),
                 cotuca::MidiNorm(127));
        Check("127 chega em 1.0 exato (divide por 127, nao por 128)",
              cotuca::MidiNorm(0) == 0.f && cotuca::MidiNorm(127) == 1.f
                  && fabsf(cotuca::MidiNorm(64) - 0.5039f) < 0.001f, d);
    }

    // ---------- VIA COMPLETA: bytes -> parser -> mapa -> ParamTable ----------
    // Este e o criterio de aceite do T10, rodando sem placa: CC 20
    // valor 64 tem que levar dly_time a 0,50 normalizado.
    printf("\nVia completa: bytes -> parser -> manifest -> ParamTable\n");
    {
        static cotuca::ParamTable t;
        static cotuca::Manifest   m;
        t.Init();
        m.Init(t);

        static cotuca::Manifest*   pm = &m;
        static cotuca::ParamTable* pt = &t;
        struct Apply
        {
            static void Cc(uint8_t, uint8_t cc, uint8_t val)
            {
                pm->ApplyMidiCc(*pt, cc, val);
            }
        };

        cotuca::MidiParser p;
        p.Init(m.midi_channel, Apply::Cc);

        const int id_time = t.Find("dly_time");
        const int id_gov  = t.Find("gov_ceiling");
        Check("manifest expoe dly_time e gov_ceiling", id_time >= 0 && id_gov >= 0);

        const uint8_t msg[] = {0xB0, 20, 64};
        Feed(p, msg, 3);
        snprintf(d, sizeof(d), "norm=%.4f real=%.4f (curve %.1f)",
                 t.Get(id_time), t.Real(id_time), t.Curve(id_time));
        Check("CC 20 val 64 -> dly_time norm 0.50",
              fabsf(t.Get(id_time) - 0.5039f) < 0.002f, d);

        // gov_ceiling: o parametro SEM knob, que so o MIDI alcanca
        const uint8_t msg2[] = {0xB0, 24, 127};
        Feed(p, msg2, 3);
        snprintf(d, sizeof(d), "norm=%.4f", t.Get(id_gov));
        Check("CC 24 val 127 -> gov_ceiling em 1.00 (param sem knob)",
              fabsf(t.Get(id_gov) - 1.0f) < 0.001f, d);

        // A INVARIANTE: MIDI e knob no mesmo parâmetro dão o mesmo
        // valor. Testada contra uma Macro de um alvo com peso 1.0 —
        // que é a forma de todo knob deste manifest — e NÃO contra um
        // índice de knob específico.
        //
        // A primeira versão cravava knob[2] porque era ali que o
        // `level` estava. O painel v2 remapeou (k1 virou level) e o
        // teste passou a acusar falha num firmware correto: ele estava
        // medindo o layout do painel, não a invariante. Layout é
        // decisão de manifest e muda de escultura pra escultura; a
        // invariante não muda nunca.
        const int id_lvl = t.Find("level");
        const uint8_t msg3[] = {0xB0, 22, 127};
        Feed(p, msg3, 3);

        cotuca::ParamTable t2;
        cotuca::Manifest   m2;
        t2.Init();
        m2.Init(t2);
        cotuca::Macro remoto;
        remoto.AddTarget(t2.Find("level"), 1.0f);
        remoto.Apply(t2, 1.0f); // o mesmo gesto, pela via do knob
        snprintf(d, sizeof(d), "midi=%.4f knob=%.4f",
                 t.Get(id_lvl), t2.Get(t2.Find("level")));
        Check("CC e knob no mesmo param dao o MESMO valor",
              fabsf(t.Get(id_lvl) - t2.Get(t2.Find("level"))) < 1e-6f, d);

        // CC fora do mapa nao pode mexer em nada
        const float before = t.Get(id_time);
        const uint8_t msg4[] = {0xB0, 99, 0};
        Feed(p, msg4, 3);
        Check("CC fora do mapa nao move parametro nenhum",
              t.Get(id_time) == before);
    }

    printf("\n%s — %d falha(s)\n", fails ? "FALHOU" : "TUDO OK", fails);
    return fails ? 1 : 0;
}
