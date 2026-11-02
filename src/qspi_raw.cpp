// Cotucatronics OS — qspi_raw v0.10.0 — ver qspi_raw.h para o porquê.
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
volatile uint32_t g_dummy_qpi = 0;   // a tabela provada nao usa dummy
// Botoes de varredura da LEITURA hibrida (0xEB). O `SSHIFT` e a variavel que o
// DESENHO-ESCRITA-QSPI.md nomeia: "com prescaler alto, a amostragem de
// meia-fase e o que separa `03 07 0F 1F` de um ID legivel. Uma variavel."
// 2 = deixar como o memory-mapped o tinha (o default: nao inventar).
volatile uint32_t g_sshift    = 2;
volatile uint32_t g_dummy_ler = 6;   // o mapeado usa 6
// Opcode e modo tambem viram argumento. O chip respondeu 0x88
// constante ao comando em 4 linhas — dois nibbles `1000`, uma linha
// alta e tres baixas: assinatura de chip que NAO esta em QPI e so
// escuta a IO0. Entao a exploracao precisa varrer modo, nao so dummy.
// 🔴 OS DEFAULTS ERAM DA HIPOTESE REFUTADA (corrigido 2026-09-29).
// Eram `0xAF` (JEDEC em QPI) e modo 3 (quatro linhas), escolhidos quando se
// acreditava que a libDaisy punha o chip em QPI. Ela nunca o faz — ver
// DESENHO-ESCRITA-QSPI.md e bts 13j. Uma sonda cujo default e a hipotese
// errada responde `00 00 00` para sempre e parece hardware mudo.
volatile uint32_t g_opcode = 0x9F;   // JEDEC em SPI simples
volatile uint32_t g_modo   = 1;      // 1 = uma linha, 3 = quatro linhas

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
void     SetSshift(uint32_t v)   { g_sshift = v; }
void     SetDummyLer(uint32_t n) { g_dummy_ler = n & 0x1Fu; }
uint32_t Sshift()                { return g_sshift; }
uint32_t DummyLer()              { return g_dummy_ler; }
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

// Declaradas aqui para que o `LeJedecId` use A MESMA janela que a escrita.
// ⚠️ Duas copias da sequencia e o defeito desta sessao em miniatura: o
// documento foi corrigido, a copia no codigo nao, e foi a copia que me
// ensinou o bug. Uma janela, um sitio.
bool AbreIndirecto(uint32_t& ccr_antes, uint32_t& cr_antes);
void FechaIndirecto(uint32_t ccr_antes, uint32_t cr_antes);
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
    uint32_t ccr_antes = 0, cr_antes = 0;

    // A janela e a mesma da escrita: ver `AbreIndirecto`. Nao se duplica.
    if(!AbreIndirecto(ccr_antes, cr_antes))
    {
        g_diag_sr     = QUADSPI->SR;
        g_diag_falhas = 99;            // 99 = a janela nao abriu
        FechaIndirecto(ccr_antes, cr_antes);
        return 0;
    }

    // 🔴 ESTE COMENTARIO ESTAVA ERRADO E REENSINOU O BUG (corrigido 2026-09-29).
    //
    // Ele dizia: "a libDaisy poe o chip em QPI no Init (`EnterQuadMode`)" e
    // "TODOS os comandos futuros nascem em QPI tambem". As duas frases foram
    // REFUTADAS no mesmo dia em que foram escritas — ver
    // docs/DESENHO-ESCRITA-QSPI.md, secao "O PROTOCOLO REAL", e bts 13j.
    // A libDaisy declara `EnterQuadMode` e NUNCA a chama; o comentario dela
    // di-lo em voz alta. O chip esta em SPI SIMPLES.
    //
    // O custo de nao ter corrigido isto aqui: a 2026-09-29 escrevi o driver de
    // escrita com os tres campos de modo em quad, porque LI ESTE COMENTARIO
    // e acreditei nele. O documento estava certo e o codigo continuou a
    // mentir — e o codigo e que se le quando se vai programar.
    //
    // O FORMATO REAL, provado (libDaisy src/per/qspi.cpp:1045, 312, 472):
    //   comando            opcode  instr  endereco  dado   dummy
    //   JEDEC ID           0x9F      1      nenhum     1      0
    //   RDSR               0x05      1      nenhum     1      0
    //   apagar bloco 64K   0xD8      1         1     nenhum   0
    //   programar pagina   0x02      1         1        1      0
    //   ler                0xEB      1         4        4      6
    //
    // Os `g_modo`/`g_dummy_qpi` abaixo sao os botoes de VARREDURA desta
    // sonda, nao o formato correcto — o formato correcto esta na tabela.
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

    FechaIndirecto(ccr_antes, cr_antes);
    return id;
}


// ============================================================================
// ESCRITA: apagar, programar, ler por comando indirecto (ppc/006 fase 3)
// ============================================================================
//
// ⚠️ A TABELA DE COMANDOS NAO E DEDUZIDA — E COPIADA DE QUEM FUNCIONA.
//
// O gabarito e a libDaisy, que apaga e programa este chip com este periferico:
//   GetStatusRegister()  src/per/qspi.cpp:1045
//   EraseBlock64K()      src/per/qspi.cpp:472
//   Write()/PAGE_PROG    src/per/qspi.cpp:312
//
// E O QUE ELA DIZ E: TUDO EM UMA LINHA, ZERO DUMMY, ENDERECO DE 24 BITS.
//
//   comando            opcode  instr  endereco  dado   dummy
//   WREN               0x06      1      nenhum  nenhum   0
//   RDSR               0x05      1      nenhum     1     0
//   apagar bloco 64K   0xD8      1         1    nenhum   0
//   programar pagina   0x02      1         1        1     0
//   ler                0xEB      1         4        4     6   <- o hibrido
//
// ⚠️ O CHIP ESTA EM SPI SIMPLES. A libDaisy nunca chama `EnterQuadMode` — o
// comentario dela di-lo em voz alta. Mandar a INSTRUCAO em quatro linhas
// produz o `0x88` da cicatriz 13j: dois nibbles `1000`, uma linha alta e tres
// baixas, a assinatura de um comando quad a chegar a um chip que so escuta a
// IO0. Foi o erro da v0.3.0 deste ficheiro — escrevi `g_modo` nos tres campos
// de uma vez, e nenhum comando foi ouvido.
//
// O `0xEB` e a excepcao coerente: a instrucao vai serial e so o endereco e o
// dado vao em quatro linhas. E o formato que o memory-mapped usa hoje, logo e
// o unico formato de leitura PROVADO nesta placa.

// 🔴 ZONA SEGURA. Ver o porque em qspi_raw.h — custou o app a correr.
static uint32_t g_zona_ini = 0x90400000u;
static uint32_t g_zona_fim = 0x907FFFFFu;

bool PodeEscrever(uint32_t addr, uint32_t n)
{
    if(!n) return false;
    const uint64_t fim = (uint64_t)addr + n - 1u;
    return addr >= g_zona_ini && fim <= (uint64_t)g_zona_fim;
}

void AlargaZona(uint32_t inicio, uint32_t fim)
{
    g_zona_ini = inicio;
    g_zona_fim = fim;
}

namespace
{
const uint32_t kUma    = 1u;   // uma linha  (IO0 so)
const uint32_t kQuatro = 3u;   // quatro linhas
const uint32_t kAd24   = 2u;   // ADSIZE=2 -> 3 bytes de endereco

// O registo de endereco leva o deslocamento na flash, nao o ponteiro do CPU.
//
// Com ADSIZE de 24 bits o periferico so poe tres bytes no fio, portanto
// `0x90050000` e `0x00050000` saem IGUAIS neste chip de 8 MB — e e por isso
// que a libDaisy passa o endereco mapeado e funciona. A conversao fica
// explicita para o codigo nao depender dessa coincidencia: se algum dia o
// endereco passar a 32 bits, quem nao converte escreve no sitio errado.
COTUCA_ITCM uint32_t NaFlash(uint32_t addr_mapeado)
{
    return addr_mapeado - 0x90000000u;
}

// DRENA A FIFO. O RM0433 diz do BUSY: "limpa-se automaticamente quando a
// operacao com a memoria Flash termina E A FIFO ESTA VAZIA". A segunda metade
// da frase e a que faltava: depois do ABORT ficavam 30 bytes de pre-busca do
// memory-mapped na FIFO, e o BUSY nao caia por causa deles — 200 000 voltas a
// esperar por uma condicao que so eu podia produzir.
COTUCA_ITCM uint32_t DrenaFifo()
{
    uint32_t tirados = 0;
    while(((QUADSPI->SR >> QUADSPI_SR_FLEVEL_Pos) & 0x3Fu) && tirados < 64u)
    {
        (void)*(volatile uint8_t*)&QUADSPI->DR;
        tirados++;
    }
    return tirados;
}

// 🔴 SAIR DO MEMORY-MAPPED E DESLIGAR O PERIFERICO, NAO ABORTA-LO.
//
// Medido a 2026-09-29, quatro gravacoes a insistir no ABORT:
//
//   SR antes do ABORT     0x00000420 / 0x00000020   BUSY=1
//   SR depois do ABORT    0x00001E22   BUSY=1 FLEVEL=30 TCF=1
//   com o TCF limpo       0x00001E20   BUSY=1 FLEVEL=30
//   com a FIFO drenada    0x00001E20   BUSY=1 FLEVEL=30  <- nao mexeu
//   voltas gastas         200000 de 200000, sempre
//
// O ABORT nao tira este periferico do memory-mapped. E a cicatriz 13l
// registou "sair do memory-mapped exige ABORT" como ACHADO DE PROTOCOLO —
// parcialmente certo, e por isso caro: ela propria avisa que "um chute que
// acerta parcialmente e o que financia o chute seguinte", e financiou quatro.
//
// O QUE O GABARITO FAZ (libDaisy `SetMode` -> `Init` -> `HAL_QSPI_DeInit`):
//   1. CR.EN = 0        <- reinicia a maquina de estados E a FIFO
//   2. FTHRES
//   3. esperar o BUSY cair   (o HAL desiste se nao cair)
//   4. PRESCALER/SSHIFT/FSEL/DFM, DCR
//   5. CR.EN = 1
//
// Nenhum ABORT em lado nenhum. E `SetMode` e literalmente `Init(config_)`: a
// biblioteca nao transiciona de modo, RECONSTROI o periferico.
//
// ⚠️ E o TCEN sai. O contador de timeout pertence ao modo mapeado — e ele que
// mantem o nCS agarrado entre acessos.
// ⚠️ O ABR GUARDA-SE NUM ESTATICO, nao num parametro: a janela nunca se
// encaixa dentro de outra (as interrupcoes estao mascaradas e isto e uma linha
// de execucao so), e acrescentar um terceiro parametro a cinco assinaturas era
// convite a que um dos cinco ficasse de fora.
static uint32_t g_abr_antes = 0;

// 🔴 E O SEGUNDO PROBLEMA, EMPILHADO SOBRE O PRIMEIRO: O CHIP ESTA EM LEITURA
// CONTINUA (XIP), E NESSE MODO NAO EXISTE OPCODE.
//
// O `EnableMemoryMappedMode` da libDaisy manda, alem do 0xEB:
//     AlternateByteMode = 4 LINHAS
//     AlternateBytes    = 0xA0        <- M7-M0; M5-M4 = 10 liga o continuo
//     SIOOMode          = INST_ONLY_FIRST_CMD
//
// Com M5-M4 = 10 o chip passa a esperar ENDERECO onde antes esperava
// instrucao. Foi por isso que, a 2026-09-29, TODOS os comandos falharam do mesmo
// modo — 0x9F, 0xAF, 0x05, uma linha, quatro linhas, 0 a 10 ciclos dummy,
// SSHIFT nos tres estados: vinte e quatro combinacoes, todas `FF` ou `88`.
// Nenhuma delas estava errada. O chip nao estava a ouvir opcodes.
//
// ⚠️ E A TABELA DO DESENHO-ESCRITA-QSPI.md OMITIA ESTES TRES CAMPOS. Ela
// listava InstructionMode/Instruction/AddressMode/DataMode/DummyCycles e
// dizia "qualquer comando novo nasce sabendo disso" — e o que faltava era
// precisamente o que impedia qualquer comando de nascer. Tabela incompleta
// com ar de completa custa mais que tabela ausente.
//
// A SAIDA: uma leitura 0xEB com o byte alternativo a ZERO (M5-M4 != 10)
// devolve o chip ao modo de comando. E a mudanca minima sobre o unico formato
// provado nesta placa — so o byte alternativo muda.
COTUCA_ITCM void SaiDoContinuo()
{
    QUADSPI->DLR = 0;                                  // 1 byte, descartado
    QUADSPI->ABR = 0x00u;                              // M5-M4 != 10
    QUADSPI->CCR = (1u << QUADSPI_CCR_FMODE_Pos)
                   | (kQuatro << QUADSPI_CCR_DMODE_Pos)
                   | (0u << QUADSPI_CCR_ABSIZE_Pos)    // 8 bits
                   | (kQuatro << QUADSPI_CCR_ABMODE_Pos)
                   | (kAd24 << QUADSPI_CCR_ADSIZE_Pos)
                   | (kQuatro << QUADSPI_CCR_ADMODE_Pos)
                   | (kUma << QUADSPI_CCR_IMODE_Pos)
                   | (6u << QUADSPI_CCR_DCYC_Pos)
                   | 0xEBu;
    QUADSPI->AR = 0;
    if(EsperaFlag(QUADSPI_SR_FTF | QUADSPI_SR_TCF, 200000u))
        (void)*(volatile uint8_t*)&QUADSPI->DR;
    EsperaFlag(QUADSPI_SR_TCF, 200000u);
    LimpaFlags();
    DrenaFifo();
}

COTUCA_ITCM bool AbreIndirecto(uint32_t& ccr_antes, uint32_t& cr_antes)
{
    ccr_antes   = QUADSPI->CCR;
    cr_antes    = QUADSPI->CR;
    g_abr_antes = QUADSPI->ABR;

    QUADSPI->CR &= ~QUADSPI_CR_EN;              // 1
    __DSB();
    LimpaFlags();

    bool ok = false;                             // 3
    for(uint32_t i = 0; i < 200000u; i++)
        if(!(QUADSPI->SR & QUADSPI_SR_BUSY)) { ok = true; break; }

    QUADSPI->CCR = 0;                            // sem FMODE herdado
    uint32_t cr_novo = cr_antes & ~(QUADSPI_CR_EN | QUADSPI_CR_ABORT
                                    | QUADSPI_CR_TCEN);
    if(g_sshift == 0u)      cr_novo &= ~QUADSPI_CR_SSHIFT;
    else if(g_sshift == 1u) cr_novo |= QUADSPI_CR_SSHIFT;
    QUADSPI->CR = cr_novo;                       // 2 = como estava
    __DSB();
    QUADSPI->CR |= QUADSPI_CR_EN;                // 5
    __DSB();
    if(ok) SaiDoContinuo();                      // 6 — sem isto nao ha opcode
    return ok;
}

// ⚠️ O ABR TEM DE VOLTAR. O memory-mapped le com o byte alternativo 0xA0; se
// ele ficar a zero, a primeira busca de instrucao do app le lixo e a placa
// morre a seguir ao `return` desta funcao. E o app CORRE daqui.
COTUCA_ITCM void FechaIndirecto(uint32_t ccr_antes, uint32_t cr_antes)
{
    LimpaFlags();
    DrenaFifo();
    QUADSPI->CR &= ~QUADSPI_CR_EN;
    __DSB();
    QUADSPI->ABR = g_abr_antes;
    QUADSPI->CCR = ccr_antes;
    QUADSPI->CR  = cr_antes;
    __DSB();
    __ISB();
}

// Comando sem endereco e sem dado (WREN). A escrita do CCR e o gatilho.
COTUCA_ITCM bool CmdSo(uint32_t opcode)
{
    QUADSPI->CCR = (0u << QUADSPI_CCR_FMODE_Pos)
                   | (0u << QUADSPI_CCR_DMODE_Pos)
                   | (0u << QUADSPI_CCR_ADMODE_Pos)
                   | (kUma << QUADSPI_CCR_IMODE_Pos)
                   | (opcode & 0xFFu);
    const bool ok = EsperaFlag(QUADSPI_SR_TCF, 200000u);
    LimpaFlags();
    return ok;
}

// RDSR pela tabela provada: instrucao 1 linha, dado 1 linha, zero dummy,
// 1 byte. Devolve VALIDADE; o valor sai por ponteiro.
//
// ⚠️ 0x00 E RESPOSTA LEGITIMA — chip ocioso, nao ocupado, nao armado para
// escrita (cicatriz 13k). Nenhum valor do dominio pode significar "sem dado",
// e foi por isso que esta funcao nao devolve o byte.
COTUCA_ITCM bool RdsrCru(uint8_t* fora)
{
    QUADSPI->DLR = 0;                            // 1 byte
    QUADSPI->CCR = (1u << QUADSPI_CCR_FMODE_Pos) // leitura indirecta
                   | (kUma << QUADSPI_CCR_DMODE_Pos)
                   | (0u << QUADSPI_CCR_ADMODE_Pos)
                   | (kUma << QUADSPI_CCR_IMODE_Pos)
                   | (0u << QUADSPI_CCR_DCYC_Pos)
                   | 0x05u;
    if(!EsperaFlag(QUADSPI_SR_FTF | QUADSPI_SR_TCF, 200000u))
    {
        LimpaFlags();
        return false;
    }
    *fora = *(volatile uint8_t*)&QUADSPI->DR;
    EsperaFlag(QUADSPI_SR_TCF, 200000u);
    LimpaFlags();
    return true;
}

// Espera o WIP cair. ⚠️ TEM DE CORRER DENTRO DA JANELA ja aberta — chamar o
// `LeStatus` publico aqui reabriria a janela a meio de uma operacao, que e a
// familia de bug de ordem aplicada ao periferico.
COTUCA_ITCM bool EsperaPronto(uint32_t voltas)
{
    for(uint32_t i = 0; i < voltas; i++)
    {
        uint8_t st;
        if(!RdsrCru(&st))    return false;
        if(!(st & 0x01u))    return true;        // WIP limpo
    }
    return false;
}
} // namespace

// SONDA DA JANELA — abre e fecha o memory-mapped SEM mandar comando nenhum.
//
// PARA QUE SERVE: o `SR=0x1E20` (BUSY=1, FLEVEL=30) aparece igual para
// qualquer opcode, o que ja e prova de que a tabela de comandos nao e a
// variavel. Esta sonda parte o espaco de busca ao meio e as duas metades sao
// accionaveis:
//
//   BUSY nao cai depois do ABORT  -> o problema e SAIR do memory-mapped, e
//                                    nenhuma tabela de comandos vai funcionar
//   BUSY cai e a FIFO esvazia     -> a janela esta boa e o problema e o
//                                    comando, e ai a tabela volta a ser suspeita
//
// Nao toca no chip: nenhum opcode e emitido. E o controlo positivo da janela.
COTUCA_ITCM void SondaJanela(uint32_t* fora6)
{
    const uint32_t pm = __get_PRIMASK();
    __disable_irq();
    fora6[0] = QUADSPI->SR;                       // como estava
    fora6[1] = QUADSPI->CR;
    const uint32_t ccr_antes = QUADSPI->CCR;
    const uint32_t cr_antes  = QUADSPI->CR;
    QUADSPI->CR &= ~QUADSPI_CR_EN;            // DESLIGAR, nao abortar
    __DSB();
    LimpaFlags();
    fora6[1] = DrenaFifo();
    uint32_t voltas = 0;
    for(; voltas < 200000u; voltas++)
        if(!(QUADSPI->SR & QUADSPI_SR_BUSY)) break;
    __DSB();
    fora6[2] = QUADSPI->SR;                       // depois do ABORT
    fora6[3] = voltas;                            // quantas voltas custou
    LimpaFlags();
    QUADSPI->CCR = 0;
    QUADSPI->CR  = cr_antes & ~(QUADSPI_CR_EN | QUADSPI_CR_ABORT
                                | QUADSPI_CR_TCEN);
    __DSB();
    QUADSPI->CR |= QUADSPI_CR_EN;
    fora6[4] = QUADSPI->SR;                       // depois de religar o EN
    QUADSPI->CR &= ~QUADSPI_CR_EN;
    __DSB();
    QUADSPI->CCR = ccr_antes;
    QUADSPI->CR  = cr_antes;
    __DSB();
    __ISB();
    fora6[5] = QUADSPI->SR;                       // restaurado
    __set_PRIMASK(pm);
}

// ⚠️ A MASCARA VIVE AQUI DENTRO, nao no chamador.
//
// O contrato estava escrito em prosa no topo deste ficheiro — "o chamador
// mascara interrupcoes" — e a 2026-09-29 escrevi tres comandos novos que nao o
// cumpriram, porque prosa nao e verificada por nada. Regra que vive so em
// documento custa a mesma disciplina todas as vezes; dentro do instrumento,
// custa uma.
COTUCA_ITCM bool LeStatus(uint8_t* fora)
{
    const uint32_t pm = __get_PRIMASK();
    __disable_irq();
    uint32_t ccr, cr;
    if(!AbreIndirecto(ccr, cr))
    {
        g_diag_sr = QUADSPI->SR;
        FechaIndirecto(ccr, cr);
        __set_PRIMASK(pm);
        return false;      // a janela nao abriu: nao se emite comando nenhum
    }
    const bool ok = RdsrCru(fora);
    g_diag_sr = QUADSPI->SR;
    FechaIndirecto(ccr, cr);
    __set_PRIMASK(pm);
    return ok;
}

COTUCA_ITCM bool ApagaSector64(uint32_t addr)
{
    if(!PodeEscrever(addr, 65536u)) return false;
    if(addr & 0xFFFFu)              return false;   // tem de estar alinhado
    const uint32_t pm = __get_PRIMASK();
    __disable_irq();
    uint32_t ccr, cr;
    if(!AbreIndirecto(ccr, cr))
    {
        g_diag_sr = QUADSPI->SR;
        FechaIndirecto(ccr, cr);
        __set_PRIMASK(pm);
        return false;      // a janela nao abriu: nao se emite comando nenhum
    }
    bool ok = CmdSo(0x06u);                         // WREN
    if(ok)
    {
        QUADSPI->CCR = (0u << QUADSPI_CCR_FMODE_Pos)
                       | (0u << QUADSPI_CCR_DMODE_Pos)
                       | (kUma << QUADSPI_CCR_ADMODE_Pos)
                       | (kAd24 << QUADSPI_CCR_ADSIZE_Pos)
                       | (kUma << QUADSPI_CCR_IMODE_Pos)
                       | (0u << QUADSPI_CCR_DCYC_Pos)
                       | 0xD8u;                      // block erase 64K
        QUADSPI->AR  = NaFlash(addr);                // o AR e o gatilho
        ok = EsperaFlag(QUADSPI_SR_TCF, 200000u);
        LimpaFlags();
        if(ok) ok = EsperaPronto(2000000u);          // o apagar e lento
    }
    g_diag_sr = QUADSPI->SR;
    FechaIndirecto(ccr, cr);
    __set_PRIMASK(pm);
    return ok;
}

COTUCA_ITCM bool ProgramaPagina(uint32_t addr, const uint8_t* dados, uint32_t n)
{
    if(!n || n > 256u)             return false;
    if(!PodeEscrever(addr, n))     return false;
    // ⚠️ RECUSA em vez de partir. Um pedido que atravessa a fronteira de
    // pagina e erro de conta de quem chama; parti-lo por dentro esconderia
    // esse erro e faria o chamador acreditar num modelo errado.
    if(((addr & 0xFFu) + n) > 256u) return false;
    const uint32_t pm = __get_PRIMASK();
    __disable_irq();
    uint32_t ccr, cr;
    if(!AbreIndirecto(ccr, cr))
    {
        g_diag_sr = QUADSPI->SR;
        FechaIndirecto(ccr, cr);
        __set_PRIMASK(pm);
        return false;      // a janela nao abriu: nao se emite comando nenhum
    }
    bool ok = CmdSo(0x06u);                          // WREN
    if(ok)
    {
        QUADSPI->DLR = n - 1u;
        QUADSPI->CCR = (0u << QUADSPI_CCR_FMODE_Pos)  // escrita indirecta
                       | (kUma << QUADSPI_CCR_DMODE_Pos)
                       | (kUma << QUADSPI_CCR_ADMODE_Pos)
                       | (kAd24 << QUADSPI_CCR_ADSIZE_Pos)
                       | (kUma << QUADSPI_CCR_IMODE_Pos)
                       | (0u << QUADSPI_CCR_DCYC_Pos)
                       | 0x02u;                       // page program
        QUADSPI->AR  = NaFlash(addr);                 // o AR e o gatilho
        for(uint32_t i = 0; i < n && ok; i++)
        {
            if(!EsperaFlag(QUADSPI_SR_FTF, 200000u)) { ok = false; break; }
            *(volatile uint8_t*)&QUADSPI->DR = dados[i];
        }
        if(ok) ok = EsperaFlag(QUADSPI_SR_TCF, 200000u);
        LimpaFlags();
        if(ok) ok = EsperaPronto(500000u);
    }
    g_diag_sr = QUADSPI->SR;
    FechaIndirecto(ccr, cr);
    __set_PRIMASK(pm);
    return ok;
}

// Leitura pelo formato HIBRIDO — o mesmo que o memory-mapped usa, que e o
// unico provado nesta placa: instrucao serial, endereco e dado em quatro
// linhas, 6 ciclos dummy.
COTUCA_ITCM bool LeBloco(uint32_t addr, uint8_t* dados, uint32_t n)
{
    if(!n) return false;
    const uint32_t pm = __get_PRIMASK();
    __disable_irq();
    uint32_t ccr, cr;
    if(!AbreIndirecto(ccr, cr))
    {
        g_diag_sr = QUADSPI->SR;
        FechaIndirecto(ccr, cr);
        __set_PRIMASK(pm);
        return false;      // a janela nao abriu: nao se emite comando nenhum
    }
    QUADSPI->DLR = n - 1u;
    QUADSPI->ABR = 0x00u;                            // M5-M4 != 10: sem XIP
    QUADSPI->CCR = (1u << QUADSPI_CCR_FMODE_Pos)
                   | (kQuatro << QUADSPI_CCR_DMODE_Pos)
                   | (0u << QUADSPI_CCR_ABSIZE_Pos)
                   | (kQuatro << QUADSPI_CCR_ABMODE_Pos)
                   | (kQuatro << QUADSPI_CCR_ADMODE_Pos)
                   | (kAd24 << QUADSPI_CCR_ADSIZE_Pos)
                   | (kUma << QUADSPI_CCR_IMODE_Pos)
                   | (g_dummy_ler << QUADSPI_CCR_DCYC_Pos)
                   | 0xEBu;                          // quad I/O fast read
    QUADSPI->AR  = NaFlash(addr);                    // o AR e o gatilho
    bool ok = true;
    for(uint32_t i = 0; i < n; i++)
    {
        if(!EsperaFlag(QUADSPI_SR_FTF | QUADSPI_SR_TCF, 200000u))
        { ok = false; break; }
        dados[i] = *(volatile uint8_t*)&QUADSPI->DR;
    }
    if(ok) EsperaFlag(QUADSPI_SR_TCF, 200000u);
    g_diag_sr = QUADSPI->SR;
    LimpaFlags();
    FechaIndirecto(ccr, cr);
    __set_PRIMASK(pm);
    return ok;
}

} // namespace qspi
} // namespace cotuca
