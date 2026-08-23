#pragma once
// Cotucatronics OS — presets v0.4.0
// ============================================================
// Biblioteca de presets: estruturas e a versao do mapa.
//
// Desenho em docs/operacao/DESENHO-PRESETS-RAM.md. Este arquivo e o
// PASSO 1 dele — estruturas e hash — e existe separado do resto porque
// e a unica parte que se prova inteira em banco nativo, sem placa.
//
// C++ puro, sem alocacao, sem dependencia de placa.
#include <stdint.h>
#include <string.h>
#include "params.h"

namespace cotuca
{

// ------------------------------------------------------------------
// QUEM ENTRA NUM PRESET
//
// O criterio, e ele resolve os casos dificeis sozinho:
//
//   Preset guarda INTENCAO MUSICAL — nao estado de hardware, nem
//   ferramenta de medicao.
//
// A pertinencia e DERIVADA da tabela, nao digitada numa segunda lista:
// duas listas divergem no primeiro param novo, e a divergencia seria
// silenciosa (o param simplesmente nao entraria no preset, e ninguem
// veria). O sufixo `_a`/`_b` ja diz "e por cadeia"; o resto e a lista
// curta de excecoes, cada uma com a razao ao lado.
inline bool ForaDoPreset(const char* nome)
{
    // ESTADO FISICO: vai virar leitura dos DPDT em D29/D30. Um preset
    // que escrevesse isto tentaria mudar uma chave de metal.
    if(!strncmp(nome, "exc_on", 6))       return true;
    // FERRAMENTA DE MEDICAO: bancada, nao musica.
    if(!strncmp(nome, "bench_boost", 11)) return true;
    // CALIBRACAO DO RAMO FISICO. O manifest diz por que: cada amp e cada
    // exciter tem ganho e acoplamento proprios, entao o teto util
    // difere. Preset carregado noutra escultura levaria um teto
    // calibrado para OUTRO CORPO — e protecao herdada de outro corpo e
    // protecao falsa.
    if(!strncmp(nome, "gov_ceiling", 11)) return true;
    if(!strncmp(nome, "gov_release", 11)) return true;
    // TOM DE TESTE: preset que ligasse isto poria uma senoide no meio da
    // peca.
    if(!strncmp(nome, "tone_", 5))        return true;
    return false;
}

inline bool EhDeCadeia(const char* nome)
{
    const size_t n = strlen(nome);
    if(n < 3 || nome[n - 2] != '_') return false;
    if(nome[n - 1] != 'a' && nome[n - 1] != 'b') return false;
    return !ForaDoPreset(nome);
}

inline bool EhGlobal(const char* nome)
{
    const size_t n = strlen(nome);
    const bool pareado = n >= 3 && nome[n - 2] == '_'
                         && (nome[n - 1] == 'a' || nome[n - 1] == 'b');
    return !pareado && !ForaDoPreset(nome);
}

// ------------------------------------------------------------------
// A VERSAO DO MAPA — DERIVADA, nunca incrementada a mao.
//
// Numero que alguem precisa lembrar de subir e numero que NAO SOBE. O
// hash e FNV-1a de 16 bits sobre os NOMES DOS PARAMS NA ORDEM.
//
// O QUE MUDA O HASH: renomear, reordenar, acrescentar, remover.
// O QUE NAO MUDA: alterar a FAIXA de um param — e isso e CORRETO,
// porque o preset guarda o normalizado, que sobrevive a uma faixa nova.
//
// A tentacao e hashear a tabela inteira "por seguranca". Seria pior:
// recusaria presets bons a cada ajuste de faixa, e recusa demais ensina
// o operador a ignorar a recusa — o carimbo aparecendo do outro lado.
//
// POR QUE ISTO EXISTE: um preset capturado hoje e carregado depois de um
// remapeamento escreve valores nos params ERRADOS. E o fantasma
// mal-enderecado da cicatriz 13n com atraso de semanas, e pior, porque
// nao ha sequer um fio para suspeitar.
inline uint16_t MapaHash(const ParamTable& t)
{
    uint32_t h = 0x811Cu;                    // FNV offset basis, 16 bits
    for(int i = 0; i < t.Count(); i++)
    {
        const char* n = t.Name(i);
        if(!n) continue;
        for(const char* p = n; *p; p++)
        {
            h ^= (uint8_t)*p;
            h = (uint16_t)(h * 0x0101u);     // FNV prime de 16 bits
        }
        h ^= 0x2Cu;                          // separador: "ab","c" != "a","bc"
        h = (uint16_t)(h * 0x0101u);
    }
    return (uint16_t)h;
}

// ------------------------------------------------------------------
// O CABECALHO
//
// `n_params` e redundante com o hash de proposito: um hash que nao bate
// NAO DIZ O QUE MUDOU, e a contagem diz de que lado procurar. Custa um
// byte e transforma "recusado" em "recusado, e olhe por aqui".
//
// `formato` cobre a TRAVESSIA numa dimensao que os outros campos nao
// alcancam: "o cabecalho ganhou um campo entre a gravacao e a leitura".
// Um preset gravado com o layout v1 e lido por um firmware v2 seria
// interpretado com DESLOCAMENTO — passaria no magic e provavelmente
// falharia no checksum, reportando CORRUPCAO para um caso de migracao
// de formato. Diagnostico errado manda o operador cacar a coisa errada.
//
// CONVENCAO, e ela vive aqui ao lado do campo porque em outro lugar
// ninguem lembraria: TODA mudanca no layout do PresetHeader ou na
// ordem/largura do payload incrementa `kFormato`. Nao e a versao do
// MAPA (essa e derivada, e outra coisa); e a versao do RECIPIENTE.
struct PresetHeader
{
    uint16_t magic;        // kPresetMagic
    uint8_t  formato;      // kFormato — versao do RECIPIENTE, nao do mapa
    uint8_t  n_params;     // contagem gravada
    uint16_t versao_mapa;  // MapaHash() de quando foi capturado
    uint8_t  checksum;     // cobre cabecalho E payload — ver abaixo
    uint8_t  _pad;
};

// A PROMESSA DO `kFormato` AMARRADA AO COMPILADOR.
//
// A convencao acima ("toda mudanca de layout incrementa kFormato") e
// disciplina, e disciplina envelhece mal em comentario. Este assert a
// transforma em contrato: quem mexer no layout bate na trava AQUI, com a
// regra na mensagem, em vez de descobrir meses depois lendo um preset
// deslocado e vendo "corrupcao".
//
// Ele nao verifica que o kFormato subiu — nao da pra verificar isso em
// compilacao. Ele garante que ninguem mude o layout SEM PASSAR POR AQUI,
// que e onde a regra esta escrita. Mesmo gesto do assert do n_sacrif:
// transformar sorte em garantia, e limite em contrato.
static_assert(sizeof(PresetHeader) == 8,
              "o layout do PresetHeader mudou: incremente kFormato e ajuste "
              "este numero. Preset gravado com o layout antigo sera lido com "
              "deslocamento, e o veredito sairia 'corrupcao' para um caso de "
              "migracao de formato (ver DESENHO-PRESETS-RAM.md §5)");

static constexpr uint16_t kPresetMagic   = 0x4354u;  // 'CT'
static constexpr uint8_t  kFormato       = 1;
static constexpr int      kMaxPresetVals = 40;       // teto por preset

// O CHECKSUM COBRE O CABECALHO, nao so o payload.
//
// Hash do mapa corrompido e tao perigoso quanto payload corrompido — na
// verdade e pior, porque um bit trocado ali produz "gravado sob outro
// mapa", que e uma frase PLAUSIVEL. O operador vai procurar um
// remapeamento que nunca houve.
//
// O proprio campo `checksum` fica de fora da conta, obviamente, e o
// `_pad` tambem: byte de alinhamento nao tem valor definido e entraria
// como ruido.
inline uint8_t Checksum(const PresetHeader& h, const uint8_t* v, int n)
{
    uint32_t s = 0;
    s += (uint32_t)(h.magic & 0xFFu) + (uint32_t)(h.magic >> 8);
    s += h.formato;
    s += h.n_params;
    s += (uint32_t)(h.versao_mapa & 0xFFu) + (uint32_t)(h.versao_mapa >> 8);
    for(int i = 0; i < n; i++) s += v[i];
    return (uint8_t)(s & 0xFFu);
}

// Por que a recusa e um ENUM e nao um bool: o dB precisa saber POR QUE
// foi recusado para saber o que fazer. Magic errado = arquivo trocado;
// formato = migracao de recipiente; mapa diferente = remapeamento;
// checksum = corrupcao. Consertos diferentes, e um bool os confundiria —
// a mesma licao do "CAIU pelo teto" contra "CAIU por ESTOURO".
enum class PresetVeredito : uint8_t
{
    Ok = 0,
    MagicErrado,
    FormatoDiferente,
    TamanhoImpossivel,
    ChecksumErrado,
    MapaDiferente,
    ContagemDiferente,
};

// A ORDEM DAS CONFERENCIAS DECIDE O DIAGNOSTICO, e ela nao e indiferente.
//
// A versao anterior conferia o hash do mapa ANTES do checksum. Como o
// checksum cobre o cabecalho, um bit trocado no campo do hash seria
// reportado como "gravado sob outro mapa" — diagnostico errado com
// aparencia de diagnostico certo.
//
// A regra geral: NAO SE CONFIA EM NENHUM CAMPO ANTES DE O CHECKSUM
// PASSAR. So o magic, o formato e a coerencia de tamanho vem antes, e os
// tres pelo mesmo motivo: sao pre-condicoes para a conta do checksum
// existir (sem tamanho coerente nao ha o que somar).
//
//   1. magic      -> arquivo trocado        (mais barato e mais especifico)
//   2. formato    -> migracao de recipiente (o layout mudou sob os pes)
//   3. tamanho    -> incoerente             (nao da pra somar)
//   4. checksum   -> corrupcao              (antes de confiar em campo algum)
//   5. mapa       -> remapeamento           (agora o campo e confiavel)
//   6. contagem   -> de que lado procurar
inline PresetVeredito Conferir(const PresetHeader& h, const uint8_t* vals,
                               int n_disponivel, uint16_t mapa_atual,
                               int n_esperado)
{
    if(h.magic != kPresetMagic)   return PresetVeredito::MagicErrado;
    if(h.formato != kFormato)     return PresetVeredito::FormatoDiferente;
    if((int)h.n_params > n_disponivel || (int)h.n_params > kMaxPresetVals)
        return PresetVeredito::TamanhoImpossivel;
    if(h.checksum != Checksum(h, vals, h.n_params))
        return PresetVeredito::ChecksumErrado;
    if(h.versao_mapa != mapa_atual)   return PresetVeredito::MapaDiferente;
    if((int)h.n_params != n_esperado) return PresetVeredito::ContagemDiferente;
    return PresetVeredito::Ok;
}

inline const char* Porque(PresetVeredito v)
{
    switch(v)
    {
        case PresetVeredito::Ok:                return "ok";
        case PresetVeredito::MagicErrado:       return "nao e um preset";
        case PresetVeredito::FormatoDiferente:  return "gravado com outro formato de cabecalho";
        case PresetVeredito::TamanhoImpossivel: return "cabecalho incoerente: contagem fora do possivel";
        case PresetVeredito::ChecksumErrado:    return "dado corrompido";
        case PresetVeredito::MapaDiferente:     return "gravado sob outro mapa de params";
        case PresetVeredito::ContagemDiferente: return "numero de params diferente";
    }
    return "?";
}

// ------------------------------------------------------------------
// O MAPA ORDENADO — o que da sentido aos bytes
//
// O preset guarda uma sequencia de bytes; sem uma ordem ESTAVEL e
// DERIVADA, o byte 7 nao significa nada. A ordem e a da propria
// ParamTable, filtrada — nunca uma lista digitada, pela mesma razao de
// `EhDeCadeia` ser derivado: duas listas divergem no primeiro param novo
// e divergem em silencio.
//
// O `MapaHash` cobre exatamente essa dependencia: se a ordem da tabela
// mudar, o hash muda e o preset velho e recusado antes de escrever nos
// params errados.
struct PresetMapa
{
    int  cadeia[2][kMaxPresetVals];   // ids na ordem, por cadeia
    int  n_cadeia = 0;                // igual nas duas, por construcao
    int  global[kMaxPresetVals];
    int  n_global = 0;
    bool estourou = false;            // mais params do que cabe

    void Construir(const ParamTable& t)
    {
        n_cadeia = n_global = 0;
        estourou = false;
        for(int i = 0; i < t.Count(); i++)
        {
            const char*  nome = t.Name(i);
            if(!nome) continue;
            const size_t L = strlen(nome);
            if(EhDeCadeia(nome))
            {
                // So a cadeia A avanca o indice; a B casa pelo nome
                // irmao. Assim as duas listas ficam ALINHADAS por
                // construcao — o byte k e o mesmo param nas duas — em
                // vez de por coincidencia de ordem.
                if(nome[L - 1] != 'a') continue;
                if(n_cadeia >= kMaxPresetVals) { estourou = true; continue; }
                char irmao[32];
                if(L >= sizeof(irmao)) { estourou = true; continue; }
                memcpy(irmao, nome, L + 1);
                irmao[L - 1] = 'b';
                const int ib = t.Find(irmao);
                if(ib < 0) { estourou = true; continue; }
                cadeia[0][n_cadeia] = i;
                cadeia[1][n_cadeia] = ib;
                n_cadeia++;
            }
            else if(EhGlobal(nome))
            {
                if(n_global >= kMaxPresetVals) { estourou = true; continue; }
                global[n_global++] = i;
            }
        }
    }
};

// ------------------------------------------------------------------
// A BIBLIOTECA
//
// UMA biblioteca, DOIS ponteiros. Nao sao duas listas: se fossem,
// "corda" existiria duas vezes e as copias divergiriam no primeiro
// ajuste. Carrega-se "corda" no A e "tambor" no B; se um dia houver
// duas cordas, carrega-se "corda" nos dois.
static constexpr int kMaxPresetsCadeia = 16;
static constexpr int kMaxPresetsGlobal = 8;
static constexpr int kNomeMax          = 16;

struct PresetCadeia
{
    char    nome[kNomeMax] = {0};
    uint8_t val[kMaxPresetVals] = {0};
    uint8_t n = 0;
    bool    usado = false;
};

struct PresetGlobal
{
    char    nome[kNomeMax] = {0};
    uint8_t val[kMaxPresetVals] = {0};
    uint8_t n = 0;
    bool    usado = false;
};

struct Biblioteca
{
    PresetCadeia cadeia[kMaxPresetsCadeia];
    PresetGlobal global[kMaxPresetsGlobal];
    int          ponteiro[2] = {-1, -1};   // -1 = nenhum carregado
    int          ponteiro_global = -1;

    // Nome copiado com truncamento EXPLICITO e terminador garantido.
    // `strncpy` sem o terminador a mao ja produziu nome sem fim neste
    // tipo de estrutura em projetos demais para eu repetir.
    static void PoeNome(char* dst, const char* src)
    {
        int k = 0;
        for(; src && src[k] && k < kNomeMax - 1; k++) dst[k] = src[k];
        dst[k] = 0;
    }

    int VagaCadeia() const
    {
        for(int i = 0; i < kMaxPresetsCadeia; i++)
            if(!cadeia[i].usado) return i;
        return -1;
    }
    int VagaGlobal() const
    {
        for(int i = 0; i < kMaxPresetsGlobal; i++)
            if(!global[i].usado) return i;
        return -1;
    }
    int AchaCadeia(const char* nome) const
    {
        for(int i = 0; i < kMaxPresetsCadeia; i++)
            if(cadeia[i].usado && !strcmp(cadeia[i].nome, nome)) return i;
        return -1;
    }
    int AchaGlobal(const char* nome) const
    {
        for(int i = 0; i < kMaxPresetsGlobal; i++)
            if(global[i].usado && !strcmp(global[i].nome, nome)) return i;
        return -1;
    }
};

} // namespace cotuca
