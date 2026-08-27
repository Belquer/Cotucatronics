// ============================================================================
// teste_minimo v0.4.0 — o firmware mais curto que responde a duas perguntas
//
// POR QUE EXISTE (27/08). Duas Daisy novas, gravadas com sucesso, enumeram o
// USB e nunca transmitem um byte. Eliminado: o hardware (duas placas, mesmo
// sintoma), a alteracao do dia (a v0.171.0 faz igual), o bootloader e o
// linker (versoes conferidas). Restava saber de que lado esta o defeito, e
// nenhuma sonda nossa podia dizer — todas falavam pela serial que estava muda.
//
// Este programa parte o espaco em dois com UMA gravacao:
//   fala  -> a cadeia de gravacao esta boa; o defeito e do nosso app
//   calado-> a cadeia esta partida; o nosso codigo nao esta em causa
//
// E de caminho responde a segunda pergunta, que e a do dB: a libDaisy
// identifica a placa por PINOS ATERRADOS NA FABRICA (PD3 = Seed 1.1,
// PD4 = Seed 2 DFM), e o ramo final e um FALL-THROUGH para rev4. Uma placa
// mais nova do que esta biblioteca conhece e detectada como a mais VELHA,
// em silencio, e leva o driver de codec errado. Entao nao se deduz a
// revisao: pergunta-se a `CheckBoardVersion()`.
// ============================================================================
#include "daisy_seed.h"

using namespace daisy;
static DaisySeed hw;

int main(void)
{
    hw.Init();
    hw.StartLog(false);   // false: nao espera pelo PC — se esperasse, um host
                          // ausente seria indistinguivel de placa muda

    const auto v = hw.CheckBoardVersion();
    const char* nome = (v == DaisySeed::BoardVersion::DAISY_SEED_1_1)   ? "Seed 1.1 (rev5) · WM8731"
                     : (v == DaisySeed::BoardVersion::DAISY_SEED_2_DFM) ? "Seed 2 DFM · PCM3060"
                                                                        : "Seed v1 (rev4) · AK4556  <<< ATENCAO: e o FALL-THROUGH";
    // OS PINOS DE IDENTIDADE, LIDOS UM A UM. A libDaisy so olha PD3 e PD4;
    // se esta placa aterrar outro (PD5 e o da rev7, segundo relato de campo),
    // a deteccao nao ve e cai no ramo mais velho. Perguntar a cada pino
    // separa "placa antiga" de "placa que esta biblioteca nao conhece".
    GPIO id[6];
    bool nivel[6];
    for(int i = 0; i < 6; i++)
    {
        id[i].Init(Pin(PORTD, i), GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
        nivel[i] = id[i].Read();
    }
    // ---- O TESTE DE VERDADE: iniciar o audio ----
    // O nosso app e todo audio. Se o codec detectado estiver errado, e AQUI
    // que ele morre — e as linhas antes e depois dizem exatamente isso, sem
    // depender de nenhuma inferencia sobre LED ou porta.
    hw.PrintLine("ANTES de SetAudioBlockSize/StartAudio");
    hw.SetAudioBlockSize(48);
    hw.PrintLine("  block size: ok");
    hw.StartAudio([](AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                     size_t sz) {
        for(size_t i = 0; i < sz; i++) { out[0][i] = 0.f; out[1][i] = 0.f; }
    });
    hw.PrintLine("DEPOIS de StartAudio — o audio iniciou sem travar");

    // ---- SDRAM: o maior suspeito que sobra ----
    // O app reserva 64 MB de 64 MB. Este teste nao so ALOCA como ESCREVE E
    // RELE, porque alocar e so aritmetica de ponteiro: se a temporizacao da
    // SDRAM estiver errada nesta revisao, alocar passa e o primeiro acesso
    // e que morre. Marca antes e depois de cada etapa, para o silencio dizer
    // ONDE parou em vez de so dizer que parou.
    static float DSY_SDRAM_BSS pool[16 * 1024 * 1024];   // 64 MB, como o app
    hw.PrintLine("ANTES de tocar na SDRAM (64 MB reservados no .bss)");
    volatile float* q = pool;
    const size_t passo = 4096;                 // uma amostra por 16 KB
    size_t escritas = 0;
    for(size_t i = 0; i < 16u*1024u*1024u; i += passo) { q[i] = (float)i; escritas++; }
    hw.PrintLine("  escreveu %u pontos ao longo dos 64 MB", (unsigned)escritas);
    size_t erros = 0;
    for(size_t i = 0; i < 16u*1024u*1024u; i += passo)
        if(q[i] != (float)i) erros++;
    hw.PrintLine("DEPOIS da SDRAM — %u erros de releitura em %u pontos",
                 (unsigned)erros, (unsigned)escritas);

    uint32_t n = 0;
    while(1)
    {
        hw.PrintLine("teste_minimo v0.4.0 — volta %u", (unsigned)n++);
        hw.PrintLine("  placa detectada: %s", nome);
        hw.PrintLine("  PD0..PD5 (0 = aterrado na fabrica): %d %d %d %d %d %d",
                     nivel[0], nivel[1], nivel[2], nivel[3], nivel[4], nivel[5]);
        hw.SetLed(n & 1);
        System::Delay(1000);
    }
}
