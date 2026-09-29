#pragma once
// ============================================================
// Cotucatronics OS — qspi_raw v0.10.0
//
// ACESSO DIRETO AO QUADSPI, PARA A JANELA DE ESCRITA.
//
// POR QUE NAO USAR A libDaisy. `PersistentStorage::Save()` tem uma
// pre-condicao que ninguem escreveu: "o app nao executa da QSPI". Ela e
// verdadeira em BOOT_NONE e REVOGADA por APP_TYPE=BOOT_QSPI, sem
// nenhum aviso de compilacao (bts 12v).
//
// E o fecho transitivo do `EraseSector` dela atravessa a fronteira da
// libDaisy e entra no HAL da ST — 1.308 + 2.666 linhas para caber em
// 64 KB de ITCM, com fronteira que se move a cada atualizacao. Mover
// so uma parte nao e solucao incompleta: e falha GARANTIDA no pior
// instante, porque a chamada que ficou de fora e buscada da memoria
// que acabou de sair do ar.
//
// Esta unidade fala com os registradores. Fecho transitivo ZERO:
// nenhuma chamada para fora, nenhuma variavel fora da ITCM, nenhum
// Log. O criterio e SUPERFICIE DE DEPENDENCIA, nao linhas escritas —
// e por isso a rotina que mais escreve e a que menos herda.
//
// A JANELA. Medido no .map de 9f47efb: a tabela de vetores mora em
// 0x90040000 e os 51 handlers em 0x9005xxxx, o HardFault inclusive.
// Durante a janela qualquer interrupcao e fatal ANTES de chegar ao
// handler — o processador busca o vetor na memoria apagada. Nao ha
// lista de excecoes possivel, entao `__disable_irq()` na janela
// inteira e obrigatorio, e a resposta simples venceu por necessidade.
// ============================================================

#include <cstdint>

#ifdef COTUCA_TEM_ITCM
#define COTUCA_ITCM __attribute__((section(".itcm_text"), noinline))
// TODA funcao desta unidade leva COTUCA_ITCM. Em 22/08 eu marquei os
// dois ajudantes e ESQUECI a funcao principal — ela rodou da QSPI e se
// matou na instrucao seguinte a desligar o periferico. O linker vinha
// dizendo: `ITCMRAM: 68 B` era pouco demais pro que eu tinha escrito, e
// eu nao li. CONFERIR O TAMANHO DA SECAO e parte do aceite.
#else
#define COTUCA_ITCM __attribute__((noinline))
#endif

namespace cotuca
{
namespace qspi
{

// A secao `.itcm_text` RODA na ITCM e MORA gravada na QSPI. Alguem tem
// que copiar no boot, como o startup ja faz com o `.data`. Chamar isto
// ANTES da primeira funcao ITCM — e como ela mesma nao pode estar na
// ITCM (ainda nao existe la), fica aqui, normal.
void CopiaParaItcm();

// Le o JEDEC ID: fabricante, tipo, densidade. Devolve os 3 bytes em
// 0x00MMTTDD, ou 0 se o periferico nao respondeu.
//
// E O "OLA MUNDO" DA INFRAESTRUTURA, e por desenho: valida ITCM,
// mudanca de modo e retorno ao memory-mapped com o UNICO comando que
// nao pode estragar nada. Se ler tres bytes funciona, o mesmo
// mecanismo escreve — o que muda entre os dois e a tabela de
// comandos, nao o mecanismo.
// Roda da ITCM e devolve 5050, sem tocar no periferico. Se isto
// travar, o defeito e a copia; se passar, o defeito e o dialogo.
COTUCA_ITCM uint32_t ProvaDeVida();

COTUCA_ITCM uint32_t LeJedecId();

// Estado da ultima transacao. "FF FF FF" nao distingue "o periferico
// nao executou" de "executou e o chip nao respondeu" — o SR distingue.
// Ciclos dummy do 0xAF em QPI. Argumento, nao constante: o numero
// certo se acha varrendo, e varrer nao pode custar uma gravacao cada.
void     SetDummyQpi(uint32_t n);
void     SetOpcode(uint32_t op);   // 0x9F em SPI, 0xAF em QPI
void     SetModo(uint32_t m);      // 1 = uma linha, 4 = quatro linhas
// ---- ESCRITA: apagar, programar, ler (ppc/006 fase 3) ---------------------
//
// O cabecalho ja dizia: "se ler tres bytes funciona, o mesmo mecanismo
// escreve — o que muda entre os dois e a TABELA DE COMANDOS, nao o
// mecanismo". Isto e essa tabela.
//
// 🔴 ZONA SEGURA IMPOSTA EM CODIGO, e ela nao e cerimonia.
//
// A 29/09 uma gravacao no endereco errado apagou o app que estava a correr e
// custou meia hora de recuperacao com BOOT+RESET (bts 15fv). A causa foi uma
// subtraccao que eu nao fiz. Um `erase` com o endereco errado faz o mesmo em
// silencio e sem hipotese de arrepender — entao a funcao RECUSA qualquer
// endereco fora da zona declarada, e quem quiser sair dela tem de o dizer por
// escrito, uma vez, num sitio que se le.
//
//   0x90000000  META            (sectores de 4 KB)
//   0x90040000  ESCOLHEDOR      <- NUNCA por software
//   0x90050000  EXECUCAO        <- so o escolhedor escreve aqui
//   0x90200000  ENCENACAO       <- o OTA escreve aqui
//   0x90400000  RASCUNHO        <- a zona segura enquanto se prova o driver
//
// ⚠️ `ZonaSegura` comeca em RASCUNHO. Alargar e uma decisao, nao um ajuste.
bool     PodeEscrever(uint32_t addr, uint32_t n);
void     AlargaZona(uint32_t inicio, uint32_t fim);   // declarar e deliberado

// Status do chip: bit 0 = WIP (ocupado), bit 1 = WEL (escrita habilitada).
// Devolve VALIDADE; o byte sai por ponteiro.
// ⚠️ 0x00 e resposta LEGITIMA (chip ocioso) — cicatriz 13k: nenhum valor do
// dominio pode significar "sem dado".
bool     LeStatus(uint8_t* fora);

// Abre e fecha a janela SEM emitir comando. Enche 6 palavras:
//   [0] SR antes  [1] bytes drenados da FIFO  [2] SR depois do ABORT
//   [3] voltas do BUSY
//   [4] SR depois do EN  [5] SR restaurado
void     SondaJanela(uint32_t* fora6);

// Botoes de varredura da leitura hibrida. `sshift`: 0/1 forca, 2 = como o
// memory-mapped o tinha. `dummy_ler`: ciclos dummy do 0xEB (o mapeado usa 6).
void     SetSshift(uint32_t v);
void     SetDummyLer(uint32_t n);
uint32_t Sshift();
uint32_t DummyLer();

// Apaga um sector de 64 KB. `addr` tem de estar alinhado e na zona segura.
// Devolve false se recusou ou se o chip nao ficou pronto.
bool     ApagaSector64(uint32_t addr);

// Programa ate 256 bytes, sem atravessar a fronteira de pagina. O chamador
// parte em paginas — a funcao RECUSA um pedido que atravesse, em vez de o
// partir por dentro: partir calado esconde um erro de conta de quem chama.
bool     ProgramaPagina(uint32_t addr, const uint8_t* dados, uint32_t n);

// Le `n` bytes por comando indirecto (NAO pelo memory-mapped). E assim que o
// escolhedor le a encenacao enquanto a QSPI esta fora do modo mapeado.
bool     LeBloco(uint32_t addr, uint8_t* dados, uint32_t n);

uint32_t UltimoSR();
uint32_t UltimasFalhas();

} // namespace qspi
} // namespace cotuca
