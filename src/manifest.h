#pragma once
// manifest.h (instrumento "bancada") v0.6.0
// ============================================================
// Cotucatronics OS — MANIFEST do instrumento
// Este é o ÚNICO arquivo que muda de escultura para escultura,
// junto com o export do RNBO em modules/<nome>/.
//
// Instrumento de teste: "bancada" — entrada + tom de teste
// atravessando o módulo RNBO "delay".
// ============================================================

#include "params.h"

namespace cotuca
{

// O que este instrumento EXIGE do cérebro. O boot confere
// contra hal::Info() e recusa rodar em placa insuficiente —
// erro claro no boot em vez de bug misterioso no palco.
struct Requirements
{
    size_t big_ram_bytes = 0;     // ex: 8MB p/ freeze longo
    bool   needs_audio_in = true; // esculturas com piezo: sim
    int    min_knobs      = 4;
};

struct Manifest
{
    Requirements req; // instrumento de teste: defaults servem

    // ids de parâmetros preenchidos no Init()
    int p_level;      // volume master
    int p_tone_freq;  // freq do oscilador de teste
    int p_tone_mix;   // mistura tom de teste vs entrada
    int p_spare;      // slot vazio de exemplo
    int p_src_mix;    // SRC MIX: 0 = IN_L (piezo), 1 = IN_R (linha)
    int p_dly_time;   // → módulo RNBO "dly_time"
    int p_dly_mix;    // → módulo RNBO "dly_mix"

    // ---- cadeia de dinâmica do shell (C++ puro, não RNBO) ----
    // Todos serial-only: são política de segurança e comportamento,
    // não gesto de performance. O painel tem 5 knobs e nenhum deles
    // se gasta aqui.
    int p_gate_thresh;
    int p_gate_release;
    int p_gov_ceiling;
    int p_gov_release;

    // ---- ligação com o módulo DSP, POR NOME ----
    // O manifest não conhece índice de parâmetro do RNBO: reexportar
    // o patch reordena tudo. Declara-se o nome, e o boot resolve o
    // índice via Module::FindParam(). Nome que não existe = erro
    // claro no log, não parâmetro fantasma mexendo em nada.
    struct ModBind
    {
        int         param_id; // id na nossa ParamTable
        const char* mod_name; // nome no export do RNBO
    };
    ModBind bind[4];
    int     n_bind = 0;

    Macro knob[kNumKnobs];

    void Init(ParamTable& t)
    {
        // nome, default (0-1), min real, max real
        p_level     = t.Add("level",     0.7f, 0.f,    1.f);
        p_tone_freq = t.Add("tone_freq", 0.4f, 80.f,   2000.f);
        p_tone_mix  = t.Add("tone_mix",  0.0f, 0.f,    1.f);
        p_spare     = t.Add("spare",     0.0f, 0.f,    1.f);
        p_src_mix   = t.Add("src_mix",   0.0f, 0.f,    1.f);
        // os params do módulo já são 0–1 no export, então min/max reais
        // coincidem com o normalizado e a conversão é identidade.
        // curve 2.5 no dly_time: ERGONOMIA, não DSP. O parâmetro do
        // patch é linear em 0–1, e linear na mão significa que quase
        // todo delay curto se amontoa no primeiro décimo do knob. Com
        // 2.5, metade do curso cobre os primeiros ~18% da faixa, que é
        // onde moram os slapbacks e os flams. O RNBO recebe o valor já
        // curvado e não sabe que houve curva.
        p_dly_time  = t.Add("dly_time",  0.5f, 0.f,    1.f,  2.5f);
        p_dly_mix   = t.Add("dly_mix",   0.5f, 0.f,    1.f);

        // GATE. gate_thresh usa curve 2 porque threshold útil vive
        // todo no fundo da faixa: linear, os primeiros 5% do curso
        // seriam a única parte usável. O default REAL é 0.02
        // (~-34 dBFS), e como Real() = pow(norm,2), o norm que
        // produz 0.02 é sqrt(0.02) = 0.1414.
        p_gate_thresh  = t.Add("gate_thresh",  0.1414f, 0.f,  1.f, 2.0f);
        // release em ms: 50–2000, default 300
        p_gate_release = t.Add("gate_release", 0.1282f, 50.f, 2000.f);

        // GOVERNOR. gov_ceiling é o futuro destino do ENCODER físico:
        // é a "borda de feedback", o ponto onde a escultura para de
        // crescer. Linear de propósito — a borda é procurada pelo
        // ouvido, e curva atrapalharia o gesto fino.
        p_gov_ceiling  = t.Add("gov_ceiling",  0.5f,    0.f,  1.f);
        // release em ms: 100–3000, default 800
        p_gov_release  = t.Add("gov_release",  0.2414f, 100.f, 3000.f);

        // knob físico → macro → parâmetro(s)
        knob[0].AddTarget(p_dly_time, 1.0f);
        knob[1].AddTarget(p_dly_mix, 1.0f);
        knob[2].AddTarget(p_level, 1.0f);
        // knob 4 aponta pro src_mix PROVISORIAMENTE, até o painel de 5
        // pots existir. É por isso que req.min_knobs segue em 4: com 4
        // knobs o SRC MIX continua alcançável, então a escultura roda.
        knob[3].AddTarget(p_src_mix, 1.0f);
        knob[4].AddTarget(p_src_mix, 1.0f); // SRC MIX definitivo

        // tone_freq e tone_mix ficaram SEM knob: os três primeiros
        // foram pro delay e pro level. Seguem alcançáveis pelo serial
        // (`tone on`/`tone off`) e pelo default — o tom de teste é
        // bancada, não performance.

        // nomes conferidos contra description.json do export
        bind[n_bind++] = {p_dly_time, "dly_time"};
        bind[n_bind++] = {p_dly_mix, "dly_mix"};
        // exemplo de macro composta (comentado):
        // knob[1].AddTarget(p_verb_size, 0.8f);
        // knob[1].AddTarget(p_verb_damp, -0.5f); // invertido
    }
};

} // namespace cotuca
