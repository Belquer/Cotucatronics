#pragma once
// ============================================================
// Cotucatronics OS — params v0.10.0
// Tabela de parâmetros + macros de knob. Board-agnostic:
// não inclui hal.h nem header de fabricante nenhum.
//
// Sem alocação dinâmica, sem exceções, sem std::string —
// tudo array fixo, seguro pra chamar do callback de áudio.
// ============================================================

#include <cstddef>
#include <cstring>
#include <cmath>

namespace cotuca
{

#define COTUCA_PARAMS_VERSION "0.10.0"

// knobs físicos do painel (manifest declara Macro knob[kNumKnobs]).
// 4 de caráter + SRC MIX = 5.
constexpr int kNumKnobs = 5;

// tetos estáticos — estourar é erro de manifest, não de runtime
// 48, nao 32. Em 18/08 a suite de efeitos levou o manifest a 35 params
// e os TRES ultimos foram recusados EM SILENCIO — os buzios inteiros
// sumiram do instrumento, e o unico sintoma foi um "?" no mapa MIDI.
//
// A folga e de proposito: a suite prevista (drive, phaser, warble,
// detune) traz ~12 params a mais, e teto apertado transforma cada
// efeito novo numa caca ao estouro.
// 64. Estourou DE NOVO em 18/08 com a suite completa: 50 params
// declarados contra teto de 48, e os buzios sumiram pela SEGUNDA vez.
//
// A denuncia no boot funcionou — e ninguem leu, porque a serial foi
// conectada depois do banner. Por isso a versao e o estado das tabelas
// tambem saem no `board`, que pode ser pedido a qualquer momento.
// 96. Terceiro estouro previsto — e o primeiro RESOLVIDO NO DESENHO em
// vez de no acidente. O pivo de 20/08 (duas cadeias de caracter) leva a
// 71 params, e o painel fisico e os presets vem depois. Custo de subir:
// 6 arrays x 32 entradas x 4 bytes = 768 bytes de SRAM.
//
// Estouro de capacidade e defeito de PROJETO. Duas vezes ele foi
// descoberto por uma feature que sumiu calada; desta vez a conta foi
// feita ANTES de escrever o manifest.
// 128 desde 28/08. A conta acima continua valida e o custo foi o
// previsto — 6 arrays x 32 entradas x 4 bytes, ~768 bytes de SRAM, com a
// placa em 55,6% de 512 KB. Subiu-se com 91/96 ocupados, ou seja com
// folga de CINCO, e nao com o estouro na mao: o painel fisico e a
// navegacao do OLED ainda nao pediram params nenhuns, e vao pedir.
//
// Estouro de capacidade e defeito de PROJETO, e a terceira vez que se
// resolve no desenho em vez de no acidente.
constexpr int kMaxParams       = 128;
constexpr int kMaxMacroTargets = 4;

inline float Clamp01(float v)
{
    return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
}

// ------------------------------------------------------------
// ParamTable
// Guarda cada parâmetro em duas formas:
//   - normalizado 0..1  → o que knob/serial/RNBO falam
//   - real (min..max)   → o que o DSP consome
// A conversão tem EXPOENTE por parâmetro (`curve`):
//
//     real = min + pow(norm, curve) * (max - min)
//
// curve = 1.0 → linear (default, e há um caminho rápido que
// pula o pow). curve > 1 empurra a resolução pro começo do
// curso: com 2.5, metade do knob cobre os primeiros 18% da
// faixa. curve < 1 faz o contrário.
//
// A curva é ERGONOMIA, decisão de MANIFEST — é sobre como o
// knob se comporta na mão. Não é decisão de módulo: o DSP
// recebe o valor final e não sabe que curva existiu.
// ------------------------------------------------------------
class ParamTable
{
  public:
    void Init() { count_ = 0; }

    // Retorna o id do parâmetro (índice). -1 se a tabela encheu.
    // steps: quantização OPCIONAL do normalizado (0 = contínuo).
    // steps=49 num param de ±24 semitons = grade cromática exata.
    // Aplicada no Set(), que é o ponto ÚNICO de escrita — knob, serial
    // e MIDI passam todos por lá, então nenhum caminho escapa da grade.
    int Add(const char* name,
            float       def_norm,
            float       min_real,
            float       max_real,
            float       curve = 1.0f,
            int         steps = 0)
    {
        if(count_ >= kMaxParams)
        {
            // FALHA RUIDOSA. A versao anterior devolvia -1 e seguia: o
            // parametro simplesmente NAO EXISTIA, e quem o usasse
            // escrevia no id -1 sem erro. Custou os buzios inteiros, e
            // o unico sintoma foi um "?" numa tabela de debug.
            //
            // Estouro de capacidade e defeito de PROJETO, nao condicao
            // de runtime: quem adiciona o 49o parametro precisa saber
            // na hora, nao descobrir por uma feature que sumiu.
            overflow_++;
            return -1;
        }
        const int id = count_++;
        name_[id]    = name;
        min_[id]     = min_real;
        max_[id]     = max_real;
        curve_[id]   = curve > 0.f ? curve : 1.0f; // expoente <= 0 não faz sentido
        steps_[id]   = steps;
        norm_[id]    = Quant(id, Clamp01(def_norm));
        return id;
    }

    // Procura por nome. -1 se não existe — quem chama DEVE checar.
    int Find(const char* name) const
    {
        for(int i = 0; i < count_; i++)
            if(strcmp(name_[i], name) == 0)
                return i;
        return -1;
    }

    // Set() fala NORMALIZADO — é o que knob e macro produzem, e o
    // knob é normalizado por natureza (posição de curso).
    void Set(int id, float norm)
    {
        if(!Valid(id))
            return;
        norm_[id] = Quant(id, Clamp01(norm));
    }

    // SetReal() fala na UNIDADE DO PARÂMETRO: Hz, ms, ×, 0–1.
    // É o que um humano digita. Inverte a curva pra achar o
    // normalizado equivalente:
    //
    //     real = min + pow(norm, curve) · (max - min)
    //  ⇒  norm = pow((real - min)/(max - min), 1/curve)
    //
    // Devolve false se estiver fora da faixa — e RECUSA em vez de
    // saturar calado: quem digita 12 num parâmetro que vai até 8
    // errou, e saturar em 8 esconderia o erro.
    bool SetReal(int id, float real)
    {
        if(!Valid(id))
            return false;
        const float lo = min_[id], hi = max_[id];
        const float a = lo < hi ? lo : hi;
        const float b = lo < hi ? hi : lo;
        if(real < a || real > b)
            return false;

        const float span = hi - lo;
        if(span == 0.f)
        {
            norm_[id] = 0.f;
            return true;
        }
        const float t = Clamp01((real - lo) / span);
        const float c = curve_[id];
        norm_[id]     = (c == 1.0f) ? t : powf(t, 1.0f / c);
        return true;
    }

    float Get(int id) const { return Valid(id) ? norm_[id] : 0.f; }

    float Real(int id) const
    {
        if(!Valid(id))
            return 0.f;
        const float n = norm_[id];
        const float c = curve_[id];
        // caminho rápido: a esmagadora maioria é linear, e Real() é
        // chamado a cada bloco de áudio.
        const float shaped = (c == 1.0f) ? n : powf(n, c);
        return min_[id] + shaped * (max_[id] - min_[id]);
    }

    // Normalizado DEPOIS da curva, sempre em 0..1. É o que um
    // módulo DSP quer receber: a curva do manifest já aplicada, mas
    // sem a unidade real (Hz, ms) que é assunto nosso, não dele.
    //
    // Equivale a (Real - min)/(max - min). Empurrar Real() direto só
    // funcionava porque os params do delay são [0,1] por acaso — com
    // shift_hz em [0,20] o módulo receberia 20 onde espera 1.
    float Shaped(int id) const
    {
        if(!Valid(id))
            return 0.f;
        const float c = curve_[id];
        return (c == 1.0f) ? norm_[id] : powf(norm_[id], c);
    }

    float Curve(int id) const { return Valid(id) ? curve_[id] : 1.0f; }
    float Min(int id) const { return Valid(id) ? min_[id] : 0.f; }
    float Max(int id) const { return Valid(id) ? max_[id] : 0.f; }

    const char* Name(int id) const { return Valid(id) ? name_[id] : "?"; }

    // Grade do steps. Pública porque o sorteio do auto-rand também
    // passa por Set() e herda a mesma quantização de graça.
    // real → norm, invertendo a curva. É a MESMA conta do SetReal,
    // exposta porque o sorteio do auto-rand precisa dela: cerca "100 a
    // 500 ms" é uniforme na UNIDADE, e uniforme em norm com curva != 1
    // entortaria a distribuição (concentraria nos tempos curtos).
    float NormFromReal(int id, float real) const
    {
        if(!Valid(id))
            return 0.f;
        const float span = max_[id] - min_[id];
        if(span == 0.f)
            return 0.f;
        float frac = (real - min_[id]) / span;
        frac = Clamp01(frac);
        return powf(frac, 1.f / curve_[id]);
    }

    float Quant(int id, float norm) const
    {
        const int st = steps_[id];
        if(st < 2) return norm;
        const float g = (float)(st - 1);
        const float q = (float)(int)(norm * g + 0.5f) / g;
        return q > 1.f ? 1.f : q;
    }

    int Count() const { return count_; }

  private:
    bool Valid(int id) const { return id >= 0 && id < count_; }

  public:
    // Quantos Add() foram recusados por falta de espaco. O boot loga
    // isto — silencio aqui ja apagou uma feature inteira.
    int  Overflow() const { return overflow_; }

  private:
    int         overflow_ = 0;
    const char* name_[kMaxParams] = {};
    int         steps_[kMaxParams] = {};
    float       min_[kMaxParams]   = {};
    float       max_[kMaxParams]   = {};
    float       curve_[kMaxParams] = {};
    // volatile: escrito pela ISR do serial, lido pelo callback de
    // áudio. float de 32 bits é atômico no M7, então não há rasgo —
    // volatile só impede o compilador de cachear o valor.
    volatile float norm_[kMaxParams] = {};
    int            count_            = 0;
};

// ------------------------------------------------------------
// Macro — um knob físico movendo N parâmetros de uma vez.
//
// Convenção de peso (w) para um knob em v (0..1):
//   w > 0 : alvo = v * w          (w é a PROFUNDIDADE; w=1 → 1:1)
//   w < 0 : alvo = 1 + v * w      (invertido; w=-1 → 1-v)
// Ou seja knob[1].AddTarget(p_damp, -0.5f) faz o parâmetro ir
// de 1.0 (knob em zero) até 0.5 (knob no talo).
//
// Isso é uma decisão de design, não uma lei — se quiser outra
// curva (bipolar em torno do default, por ex.), o único lugar
// a mexer é Apply().
// ------------------------------------------------------------
struct Macro
{
    void AddTarget(int param_id, float weight)
    {
        if(n_ >= kMaxMacroTargets || param_id < 0)
            return;
        target_[n_] = param_id;
        weight_[n_] = weight;
        n_++;
    }

    void Apply(ParamTable& t, float v) const
    {
        v = Clamp01(v);
        for(int i = 0; i < n_; i++)
        {
            const float w = weight_[i];
            const float x = (w >= 0.f) ? (v * w) : (1.f + v * w);
            t.Set(target_[i], x);
        }
    }

    int TargetCount() const { return n_; }

  private:
    int   target_[kMaxMacroTargets] = {};
    float weight_[kMaxMacroTargets] = {};
    int   n_                        = 0;
};

} // namespace cotuca
