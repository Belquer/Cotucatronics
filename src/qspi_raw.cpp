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

// CICLOS DUMMY DO 0xAF EM QPI.
//
// O SR=0x02 (TCF, transferencia COMPLETA) com FLEVEL=0 e zero esperas
// estouradas provou que o periferico executa o comando inteiro e
// termina limpo — quem nao responde e o CHIP. As linhas de dado ficam
// em alta e lemos o 0xFF do pull-up.
//
// Em QPI, o JEDEC (0xAF) costuma exigir ciclos dummy entre a
// instrucao e o dado; em SPI simples, o 0x9F nao exige nenhum. Se o
// chip esta em QPI (o que explica o 0x9F falhar) E precisa de dummies
// (o que explica o 0xAF falhar), as duas leituras se explicam pela
// MESMA causa — que e o tipo de hipotese que vale testar antes das
// que precisam de duas causas.
//
// Ajustavel aqui, uma variavel por voo.
// Vira ARGUMENTO em vez de constante: uma gravacao, muitas tentativas.
// Gravar uma vez por valor custaria minutos e um ciclo de DFU cada —
// e o numero certo se acha varrendo, nao deduzindo.
volatile uint32_t g_dummy_qpi = 2;
// Opcode e modo tambem viram argumento. O chip respondeu 0x88
// constante ao comando em 4 linhas — dois nibbles `1000`, uma linha
// alta e tres baixas: assinatura de chip que NAO esta em QPI e so
// escuta a IO0. Entao a exploracao precisa varrer modo, nao so dummy.
volatile uint32_t g_opcode = 0xAF;
volatile uint32_t g_modo   = 3;    // 1 = uma linha, 3 = quatro linhas

// Diagnostico da ultima transacao.
//
// NAO precisa estar na ITCM, e eu quase pus por engano. So a BUSCA DE
// INSTRUCAO depende da QSPI; dado em SRAM (onde o `.bss` mora em
// BOOT_QSPI) e lido normalmente com o memory-mapped desligado. O
// compilador avisou com "section type conflict" — `.itcm_text` e
// secao de CODIGO — e o aviso estava certo por um motivo alem do que
// ele disse.
volatile uint32_t g_diag_sr     = 0;
volatile uint32_t g_diag_falhas = 0;

void     SetDummyQpi(uint32_t n) { g_dummy_qpi = n & 0x1Fu; }
void     SetOpcode(uint32_t op)  { g_opcode = op & 0xFFu; }
void     SetModo(uint32_t m)     { g_modo = (m == 1u) ? 1u : 3u; }
uint32_t UltimoSR()     { return g_diag_sr; }
uint32_t UltimasFalhas(){ return g_diag_falhas; }

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

    // TABELA DE COMANDOS — e a coluna MODO nao e decoracao.
    //
    // A libDaisy poe o chip em QPI no Init (`EnterQuadMode`, que o
    // fecho transitivo do EraseSector revelou). Chip em QPI IGNORA
    // comando mandado em 1 linha: nao responde, o barramento fica em
    // pull-up, e ler barramento em repouso da 0xFF. Foi exatamente o
    // que veio na primeira tentativa — FF FF FF, assinatura da causa.
    //
    //   comando        opcode  modo     uso
    //   JEDEC ID       0x9F    1 linha  chip em SPI
    //   JEDEC ID       0xAF    QPI      chip em QPI   <- o nosso caso
    //
    // TODOS os comandos futuros — erase, program, status-poll —
    // nascem em QPI tambem, com seus opcodes e formatos quad. Esta
    // coluna existe pra ninguem redescobrir isso no erase.
    QUADSPI->DLR = 3 - 1;                       // 3 bytes
    QUADSPI->CCR = (1u << QUADSPI_CCR_FMODE_Pos)   // leitura indireta
                   | (g_modo << QUADSPI_CCR_DMODE_Pos)
                   | (0u << QUADSPI_CCR_ADMODE_Pos)// sem endereco
                   | (g_modo << QUADSPI_CCR_IMODE_Pos)
                   | (g_dummy_qpi << QUADSPI_CCR_DCYC_Pos)
                   | (g_opcode & 0xFFu);

    // INSTRUMENTAR ANTES DE CHUTAR. "FF FF FF" nao distingue "o
    // periferico nao executou" de "executou e o chip nao respondeu" —
    // sao causas opostas com a mesma leitura. O SR responde.
    uint32_t id = 0;
    uint32_t esperas_falhas = 0;
    for(int i = 0; i < 3; i++)
    {
        if(!EsperaFlag(QUADSPI_SR_FTF | QUADSPI_SR_TCF, 200000u))
        { esperas_falhas++; break; }
        id = (id << 8) | (*(volatile uint8_t*)&QUADSPI->DR);
    }
    const uint32_t sr_fim = QUADSPI->SR;
    if(!EsperaFlag(QUADSPI_SR_TCF, 200000u)) esperas_falhas++;
    g_diag_sr      = sr_fim;
    g_diag_falhas  = esperas_falhas;
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
