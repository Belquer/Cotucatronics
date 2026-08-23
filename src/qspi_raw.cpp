// Cotucatronics OS — qspi_raw v0.1.0 — ver qspi_raw.h para o porquê.
#include "qspi_raw.h"
#include "stm32h7xx.h"

extern "C" {
extern char _siitcm, _sitcm, _eitcm;
}

namespace cotuca
{
namespace qspi
{

void CopiaParaItcm()
{
#ifdef COTUCA_TEM_ITCM
    const uint32_t* src = (const uint32_t*)&_siitcm;
    uint32_t*       dst = (uint32_t*)&_sitcm;
    uint32_t* const fim = (uint32_t*)&_eitcm;
    while(dst < fim) *dst++ = *src++;
    // A ITCM tem barramento proprio e nao passa pela D-cache, mas a
    // ESCRITA acima foi feita pela D-cache. Sem despejar, o codigo
    // copiado pode ainda estar no cache e a busca de instrucao ler
    // lixo. Mesma armadilha que apagou o primeiro voo da caixa-preta,
    // com a agravante de que aqui o dado corrompido e CODIGO.
    // A ESCRITA foi pela D-cache; a BUSCA sera pela I-cache. Sem
    // despejar uma e invalidar a outra, o processador pode executar o
    // que estava ali ANTES da copia. Codigo e o pior dado possivel pra
    // se corromper em silencio — eu tinha esquecido o segundo passo.
    SCB_CleanDCache();
    SCB_InvalidateICache();
    __DSB();
    __ISB();
#endif
}

// ---- daqui pra baixo, tudo roda da ITCM ----
//
// REGRA DA JANELA: nada aqui chama nada de fora. Sem HAL, sem libDaisy,
// sem Log, sem variavel global que nao esteja na ITCM ou em registrador.
// A unica coisa tocada e o periferico.

namespace
{
// Espera um bit do SR com teto de voltas — NUNCA `while` sem teto.
// Laco ilimitado aqui e o mesmo defeito que matou o loop principal
// duas vezes hoje (PollMidi e o rearme da escuta), e aqui seria pior:
// com as interrupcoes mascaradas e a QSPI fora do ar, ninguem socorre.
COTUCA_ITCM bool EsperaFlag(uint32_t mascara, uint32_t voltas)
{
    while(voltas--)
        if(QUADSPI->SR & mascara) return true;
    return false;
}

COTUCA_ITCM void LimpaFlags()
{
    QUADSPI->FCR = QUADSPI_FCR_CTCF | QUADSPI_FCR_CTEF
                   | QUADSPI_FCR_CSMF | QUADSPI_FCR_CTOF;
}
} // namespace

// PROVA DE VIDA DA ITCM, sem tocar no periferico.
//
// Antes de acusar a mudanca de modo, e preciso saber se a rotina
// sequer EXECUTA da ITCM e retorna. Se esta funcao travar, o problema
// e a copia — nao o dialogo com o chip. Um degrau por vez.
COTUCA_ITCM uint32_t ProvaDeVida()
{
    volatile uint32_t soma = 0;
    for(uint32_t i = 1; i <= 100; i++) soma += i;
    return soma;   // 5050 se a ITCM esta viva
}

COTUCA_ITCM uint32_t LeJedecId()
{
    // Guarda o modo atual pra devolver EXATAMENTE como estava. Sair
    // daqui com o periferico diferente do que se achou seria deixar o
    // instrumento num estado que ninguem declarou.
    const uint32_t ccr_antes = QUADSPI->CCR;
    const uint32_t cr_antes  = QUADSPI->CR;

    // Sai do memory-mapped. Deste ponto ate a restauracao, NADA pode
    // ser buscado da QSPI — por isso este arquivo inteiro esta na ITCM
    // e o chamador mascara interrupcoes.
    QUADSPI->CR &= ~QUADSPI_CR_EN;
    __DSB();
    LimpaFlags();
    QUADSPI->CR |= QUADSPI_CR_EN;

    QUADSPI->DLR = 3 - 1;                       // 3 bytes
    QUADSPI->CCR = (1u << QUADSPI_CCR_FMODE_Pos)   // leitura indireta
                   | (1u << QUADSPI_CCR_DMODE_Pos) // dado em 1 linha
                   | (0u << QUADSPI_CCR_ADMODE_Pos)// sem endereco
                   | (1u << QUADSPI_CCR_IMODE_Pos) // instrucao em 1 linha
                   | 0x9Fu;                        // JEDEC ID

    uint32_t id = 0;
    for(int i = 0; i < 3; i++)
    {
        if(!EsperaFlag(QUADSPI_SR_FTF | QUADSPI_SR_TCF, 200000u))
        { id = 0; break; }
        id = (id << 8) | (*(volatile uint8_t*)&QUADSPI->DR);
    }

    EsperaFlag(QUADSPI_SR_TCF, 200000u);
    LimpaFlags();

    // Devolve o periferico ao estado encontrado.
    QUADSPI->CR &= ~QUADSPI_CR_EN;
    __DSB();
    QUADSPI->CCR = ccr_antes;
    QUADSPI->CR  = cr_antes;
    __DSB();
    __ISB();
    return id;
}

} // namespace qspi
} // namespace cotuca
