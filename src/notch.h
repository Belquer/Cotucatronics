#pragma once
// ============================================================
// Cotucatronics OS — notch v0.2.0
// Filtros de rejeicao estreitos, contra microfonia.
//
// POR QUE ISTO EXISTE, e por que nao ha alternativa mecanica:
// medido em 31/08 numa peca em que o tambor e SOLDADO ao botijao —
// um cone de metal, corpo unico. Exciter e piezo vivem na mesma peca
// rigida, sem junta nenhuma entre eles, e por isso todos os gestos
// mecanicos falharam: cruzar o roteamento, procurar o no, inverter a
// polaridade, amortecer a lingua. Nao havia caminho para encurtar
// porque nao havia caminho — havia UMA peca.
//
// O QUE TORNA ESTE CORPO PIOR QUE UMA SALA: metal cortado em linguas
// tem Q altissimo. O laco nao precisa de ganho, precisa de UM PICO —
// e o pico esta la, estreito e enorme. Tocar de leve ja bastava.
//
// E E POR ISSO QUE UM NOTCH ESTREITO GANHA TANTO. Numa sala, a
// microfonia salta de modo em modo e cada notch compra pouco. Num
// corpo de Q alto ela mora numa frequencia so, e derrubar essa
// frequencia compra 6 a 10 dB antes de o modo seguinte assumir.
//
// NAO E FILTRO DE LIMPEZA, E CIRURGIA. A largura e deliberadamente
// minuscula (Q 35 => ~1/35 da frequencia, uns 20 Hz a 700 Hz): larga
// o suficiente para cobrir a deriva termica do modo, estreita o
// suficiente para o ouvido nao dar por ela. Um notch largo resolve a
// microfonia e mata o instrumento junto.
//
// PROFUNDIDADE E PARAMETRO, e nao e capricho. Rejeicao total apaga a
// frequencia, e se ela for tambem uma nota do instrumento, apaga a
// nota. Cortar 12 dB costuma bastar para o laco abrir, e deixa o
// modo audivel.
//
// ------------------------------------------------------------
// O QUE ESTE ARQUIVO NAO FAZ: nao decide ONDE por o notch.
//
// A colocacao automatica e o passo seguinte, e e ELA que faz isto
// escalar — a peca e replicada em serie, na ordem de centenas, e
// cada soldadura tera modos proprios. Notch afinado a mao nao escala.
//
// Mas o actuador vem antes do detector, por duas razoes: detector
// sem actuador nao tem o que accionar, e um detector afinado sobre
// um actuador NAO PROVADO herda a duvida dos dois. O caminho e
// provar este, e so entao confiar no que o proximo mandar.
// ------------------------------------------------------------
//
// C++ puro: sem hal, sem alocacao, sem estado global.
// ============================================================

#include <cstddef>
#include <cmath>

namespace cotuca
{

class Notch
{
  public:
        // QUATRO, e nao duas. Subido em 02/09, no meio de uma sessao de
    // escuta em que cada modo morto entregava o lugar ao seguinte: 1536,
    // 3276, 535 Hz — e a segunda boca acabou-se ao terceiro. Para por o
    // 3276 tive de APAGAR os 834 que o dB tinha afinado, e a escolha foi
    // minha e nao dele.
    //
    // Custo MEDIDO antes de subir: ~0,5% de CPU por boca (27,1% com zero,
    // 28,3% com duas, 29,0% com quatro). Oito bocas no total custam ~4%,
    // com o pico em 42,7% e margem ate 80%. Nao e a CPU que limita.
    static const int kBocas = 4;   // notches por RAMO (exciter)

    // Q FIXO, e a escolha e declarada. Ele podia ser param, mas cada
    // param novo e um CC novo no iPad e uma linha no preset — e o
    // ouvido ainda nao pediu. 35 sai de contas: um modo de metal
    // deriva com a temperatura uns poucos Hz, e 1/35 da frequencia
    // cobre isso com folga sem alargar a ferida.
    static constexpr float kQ = 35.f;

    void Init(float sample_rate)
    {
        sr_ = sample_rate > 0.f ? sample_rate : 48000.f;
        for(int b = 0; b < kBocas; b++)
        {
            hz_[b] = 0.f; db_[b] = 0.f;
            z1_[b] = z2_[b] = 0.f;
            // Coeficientes de PASSA-TUDO ate alguem os definir. Zero
            // aqui daria silencio, e silencio por falta de
            // inicializacao e indistinguivel de silencio por defeito —
            // a familia que este projecto ja pagou varias vezes.
            b0_[b] = 1.f; b1_[b] = 0.f; b2_[b] = 0.f;
            a1_[b] = 0.f; a2_[b] = 0.f;
        }
    }

    // `hz` em Hz, `db` em dB de REJEICAO (positivo = corta).
    // Recalcula so quando muda: os coeficientes envolvem tres
    // transcendentais, e ninguem paga por bloco o que nao mudou.
    void Set(int boca, float hz, float db)
    {
        if(boca < 0 || boca >= kBocas) return;
        if(hz == hz_[boca] && db == db_[boca]) return;
        hz_[boca] = hz; db_[boca] = db;
        Calcula(boca);
    }

    float Hz(int boca) const { return (boca >= 0 && boca < kBocas) ? hz_[boca] : 0.f; }
    float Db(int boca) const { return (boca >= 0 && boca < kBocas) ? db_[boca] : 0.f; }

    // Uma amostra pelas duas bocas, em serie.
    inline float Processa(float x)
    {
        for(int b = 0; b < kBocas; b++)
        {
            // Forma transposta direta II: menos estado, e o erro de
            // arredondamento nao se acumula no caminho do sinal como
            // na forma direta I.
            const float y = b0_[b] * x + z1_[b];
            z1_[b]        = b1_[b] * x - a1_[b] * y + z2_[b];
            z2_[b]        = b2_[b] * x - a2_[b] * y;
            x             = y;
        }
        return x;
    }

    // As caudas dos biquads guardam o sinal anterior. Sem isto, ligar
    // depois de um periodo desligado cospe o que la ficou.
    void Limpa()
    {
        for(int b = 0; b < kBocas; b++) z1_[b] = z2_[b] = 0.f;
    }

  private:
    void Calcula(int b)
    {
        // Fora de faixa util = passa-tudo. Nyquist com folga em cima,
        // e em baixo o corpo nao tem nada que valha cirurgia.
        if(hz_[b] < 40.f || hz_[b] > sr_ * 0.45f || db_[b] <= 0.f)
        {
            b0_[b] = 1.f; b1_[b] = 0.f; b2_[b] = 0.f;
            a1_[b] = 0.f; a2_[b] = 0.f;
            return;
        }
        // EQ de pico com ganho NEGATIVO (RBJ), e nao notch puro: o
        // notch puro tem rejeicao infinita e nao tem profundidade
        // ajustavel. Aqui a profundidade e o A.
        const float A     = powf(10.f, -db_[b] / 40.f);   // <1 = corta
        const float w0    = 2.f * 3.14159265358979f * hz_[b] / sr_;
        const float cw    = cosf(w0);
        const float alpha = sinf(w0) / (2.f * kQ);
        const float a0    = 1.f + alpha / A;
        const float inv   = 1.f / a0;
        b0_[b] = (1.f + alpha * A) * inv;
        b1_[b] = (-2.f * cw) * inv;
        b2_[b] = (1.f - alpha * A) * inv;
        a1_[b] = (-2.f * cw) * inv;
        a2_[b] = (1.f - alpha / A) * inv;
    }

    float sr_ = 48000.f;
    float hz_[kBocas] = {0.f, 0.f};
    float db_[kBocas] = {0.f, 0.f};
    float b0_[kBocas], b1_[kBocas], b2_[kBocas], a1_[kBocas], a2_[kBocas];
    float z1_[kBocas], z2_[kBocas];
};

} // namespace cotuca
