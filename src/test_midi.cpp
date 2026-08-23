// test_midi v0.2.0 — validação nativa do parser MIDI e do mapa.
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

        // ---- O CC VEM DO MAPA, NUNCA DECORADO ----
        //
        // Este bloco ja carregava o comentario certo ("o teste segue o
        // MAPA, nao um numero decorado") e a linha seguinte cravava
        // `{0xB0, 20, 127}`. A intencao estava escrita e a implementacao
        // decorou — entao cada remapeamento reprovava um firmware
        // correto, ate as falhas virarem paisagem.
        //
        // Com o CC derivado do manifest, remapear NAO quebra o teste. O
        // que quebra o teste e o contrato mudar: param sumir, param
        // entrar ou sair do mapa. Que e o que um banco deve vigiar.
        auto CcDe = [&](int param_id) -> int {
            for(int k = 0; k < m.n_midi; k++)
                if(m.midi_map[k].param_id == param_id) return m.midi_map[k].cc;
            return -1;
        };

        const int id_time = t.Find("dly_time");
        Check("manifest expoe dly_time", id_time >= 0);

        // GOV_CEILING: A DECISAO MUDOU, E O TESTE MUDA COM ELA.
        //
        // Ate 18/08 `gov_ceiling` estava FORA do mapa de proposito:
        // transicoes de link BLE faziam o iOS reenviar o estado de todos
        // os widgets e o instrumento obedecia, emudecendo tres vezes numa
        // sessao. Este teste era a trava dessa decisao.
        //
        // A decisao foi revista com o hardware: com dois exciters a
        // protecao duplicou, virou `gov_ceiling_a/_b`, e a pagina de
        // protecao entrou no mapa em 110/116 — a razao esta no manifest,
        // ao lado do AddMidi.
        //
        // O QUE ISTO CUSTOU, e e o motivo de estar escrito aqui: quando a
        // decisao mudou, este teste DEVERIA ter ficado vermelho e forcado
        // a atualizacao do raciocinio. Ele ja estava vermelho — pelo nome
        // antigo, que nao existia mais — e o sinal se perdeu no ruido de
        // tres falhas cronicas. Falha cronica nao e divida: e um sensor
        // desligado que ninguem lembra de ter desligado.
        const int id_gov_a = t.Find("gov_ceiling_a");
        const int id_gov_b = t.Find("gov_ceiling_b");
        Check("manifest expoe gov_ceiling_a e _b (protecao POR RAMO)",
              id_gov_a >= 0 && id_gov_b >= 0);
        snprintf(d, sizeof(d), "cc_a=%d cc_b=%d", CcDe(id_gov_a), CcDe(id_gov_b));
        Check("gov_ceiling_a/_b ESTAO no mapa (pagina de protecao)",
              CcDe(id_gov_a) >= 0 && CcDe(id_gov_b) >= 0, d);

        // SRC_MIX: APOSENTADO em 20/08. O teste antigo vivia dentro de
        // um `if(id_src >= 0)` e passou a NAO RODAR quando o param sumiu
        // — sem uma linha de saida, sem uma falha, sem nada. Teste que
        // some em silencio e pior que teste que falha: a suite fica
        // menor e o relatorio continua dizendo "ok".
        //
        // Agora a ausencia e AFIRMADA. Se `src_mix` voltar, isto reprova
        // e alguem le a razao pela qual ele saiu.
        Check("src_mix continua aposentado (nao existe na tabela)",
              t.Find("src_mix") < 0);

        // ---- VIA COMPLETA, com o CC vindo do mapa ----
        if(id_time >= 0)
        {
            const int cc_time = CcDe(id_time);
            snprintf(d, sizeof(d), "dly_time esta no CC %d", cc_time);
            Check("dly_time tem CC no mapa", cc_time >= 0, d);
            if(cc_time >= 0)
            {
                const uint8_t msg[] = {0xB0, (uint8_t)cc_time, 64};
                Feed(p, msg, 3);
                snprintf(d, sizeof(d), "cc=%d norm=%.4f real=%.4f (curve %.1f)",
                         cc_time, t.Get(id_time), t.Real(id_time),
                         t.Curve(id_time));
                Check("CC do dly_time, val 64 -> norm 0.50",
                      fabsf(t.Get(id_time) - 0.5039f) < 0.002f, d);
            }
        }

        // `rev_size_a`, nao `rev_size`: o reverb virou por cadeia e o
        // nome ganhou sufixo. O teste antigo procurava o nome sem sufixo
        // dentro de um `if(id_rs >= 0)` e PAROU DE RODAR nesse dia — sem
        // falhar, sem imprimir, sem deixar rastro. Foi esta reescrita,
        // ao trocar o guarda silencioso por afirmacao, que o descobriu.
        const int id_rs = t.Find("rev_size_a");
        Check("manifest expoe rev_size_a", id_rs >= 0);
        if(id_rs >= 0)
        {
            const int cc_rs = CcDe(id_rs);
            if(cc_rs >= 0)
            {
                const uint8_t m4[] = {0xB0, (uint8_t)cc_rs, 127};
                Feed(p, m4, 3);
                snprintf(d, sizeof(d), "cc=%d norm=%.4f", cc_rs, t.Get(id_rs));
                Check("CC do rev_size, val 127 -> norm 1.00",
                      fabsf(t.Get(id_rs) - 1.0f) < 0.001f, d);
            }
        }

        // A INVARIANTE: MIDI e knob no mesmo parametro dao o mesmo valor.
        // Testada contra uma Macro de um alvo com peso 1.0 — que e a
        // forma de todo knob deste manifest — e nao contra um indice de
        // knob nem contra um numero de CC. A invariante nao depende de
        // qual CC carrega o param, e agora o teste tambem nao.
        const int id_lvl = t.Find("level");
        Check("manifest expoe level", id_lvl >= 0);
        if(id_lvl >= 0)
        {
            const int cc_lvl = CcDe(id_lvl);
            Check("level tem CC no mapa", cc_lvl >= 0);
            if(cc_lvl >= 0)
            {
                const uint8_t msg3[] = {0xB0, (uint8_t)cc_lvl, 127};
                Feed(p, msg3, 3);

                cotuca::ParamTable t2;
                cotuca::Manifest   m2;
                t2.Init();
                m2.Init(t2);
                cotuca::Macro remoto;
                remoto.AddTarget(t2.Find("level"), 1.0f);
                remoto.Apply(t2, 1.0f); // o mesmo gesto, pela via do knob
                snprintf(d, sizeof(d), "cc=%d midi=%.4f knob=%.4f",
                         cc_lvl, t.Get(id_lvl), t2.Get(t2.Find("level")));
                Check("CC e knob no mesmo param dao o MESMO valor",
                      fabsf(t.Get(id_lvl) - t2.Get(t2.Find("level"))) < 1e-6f, d);
            }
        }

        // CC FORA DO MAPA nao pode mexer em nada. O numero e escolhido
        // por VARREDURA, nao decorado: um numero fixo vira "dentro do
        // mapa" no dia em que alguem o usar, e o teste passa a provar o
        // contrario do que diz.
        int cc_livre = -1;
        for(int cand = 0; cand < 128 && cc_livre < 0; cand++)
        {
            bool usado = false;
            for(int k = 0; k < m.n_midi; k++)
                if(m.midi_map[k].cc == cand) { usado = true; break; }
            if(!usado) cc_livre = cand;
        }
        snprintf(d, sizeof(d), "cc livre encontrado: %d", cc_livre);
        Check("existe ao menos um CC fora do mapa para testar", cc_livre >= 0, d);
        if(cc_livre >= 0 && id_time >= 0)
        {
            const float before = t.Get(id_time);
            const uint8_t msg4[] = {0xB0, (uint8_t)cc_livre, 0};
            Feed(p, msg4, 3);
            Check("CC fora do mapa nao move parametro nenhum",
                  t.Get(id_time) == before);
        }
    }

    printf("\n%s — %d falha(s)\n", fails ? "FALHOU" : "TUDO OK", fails);
    return fails ? 1 : 0;
}
