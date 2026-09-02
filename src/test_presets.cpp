// test_presets v0.4.0 — validação nativa das estruturas de preset.
//
// Roda no Mac, sem placa: `presets.h` é C++ puro e a pergunta que ele
// responde — "este preset foi gravado sob este mapa?" — não depende de
// hardware nenhum.
//
// O teste que importa é o do HASH, e ele tem duas metades opostas:
// muda quando deve, e NÃO muda quando não deve. Um hash que só se testa
// pela primeira metade passa sendo largo demais — e hash largo demais
// recusa preset bom, o que ensina o operador a ignorar a recusa.
#include <cstdio>
#include <cstring>
#include "params.h"
#include "manifest.h"
#include "presets.h"

static int fails = 0;
static void Check(const char* what, bool ok, const char* detail = "")
{
    printf("  %s %-58s %s\n", ok ? "OK  " : "FALHA", what, detail);
    if(!ok) fails++;
}

int main()
{
    char d[160];
    printf("test_presets — estruturas e versao do mapa\n\n");

    static cotuca::ParamTable t;
    static cotuca::Manifest   m;
    t.Init();
    m.Init(t);

    // ---- QUEM ENTRA ----
    printf("Pertinencia (derivada da tabela, nao digitada)\n");
    int cad = 0, glob = 0, fora = 0;
    for(int i = 0; i < t.Count(); i++)
    {
        const char* n = t.Name(i);
        if(cotuca::EhDeCadeia(n))    cad++;
        else if(cotuca::EhGlobal(n)) glob++;
        else                          fora++;
    }
    snprintf(d, sizeof(d), "%d de cadeia (2x%d) + %d global + %d fora = %d",
             cad, cad / 2, glob, fora, t.Count());
    Check("a soma das tres classes cobre a tabela inteira",
          cad + glob + fora == t.Count(), d);
    // ⚠️ ESTES NUMEROS ENVELHECEM, E E ESSE O TRABALHO DELES.
    //
    // Eram 33 e 9. Em 02/09 este teste falhava com 39 e 6, e as tres
    // falhas eram DO TESTE: ele guardava requisitos que o dB tinha
    // revogado e alteracoes que eu tinha feito sem o correr.
    //
    //   33 -> 39 por cadeia: os notches anti-microfonia entraram em 01/09
    //         (notch1_hz, notch1_db, notch2_hz, notch2_db, notch_on).
    //    9 ->  6 globais: o dB tirou `post_level_l/r`, `out_l/r` e
    //         `pre_level_ab` do preset em 01/09 — sao ajuste de sala e de
    //         botao fisico, nao de musica.
    //
    // Nao se apagam por incomodarem: e a contagem que denuncia um param
    // que entrou ou saiu do preset SEM ninguem decidir. Quando falharem
    // outra vez, a pergunta e "quem mudou a pertinencia, e foi de
    // proposito?" — e a resposta actualiza-se AQUI, com a data.
    Check("preset de cadeia tem 39 params por cadeia", cad / 2 == 39);
    Check("preset global tem 6 params", glob == 6);
    Check("as duas cadeias tem o MESMO numero de params", cad % 2 == 0);

    // As exclusoes, uma a uma e por NOME: a contagem sozinha passaria se
    // duas exclusoes se cancelassem com duas inclusoes indevidas.
    const char* devem_ficar_fora[] = {
        "exc_on_a", "exc_on_b", "bench_boost_a", "bench_boost_b",
        "gov_ceiling_a", "gov_ceiling_b", "gov_release_a", "gov_release_b",
        "test_tone_freq", "test_tone_level", "test_tone_out",
        // ⚠️ DECISOES DO dB DE 01/09, e agora GUARDADAS e nao apenas
        //    nao-contrariadas. Tira-los da lista de "tem de entrar" so
        //    deixava o teste de os exigir; poe-los aqui obriga o codigo a
        //    manter a decisao.
        //
        //    "pode tirar level l r do preset, usuario ajusta no botao
        //     fisico" e "tira out_l, out_r e pre_level_ab" — sao ajuste de
        //    sala e de botao, nao de musica. Se voltarem ao preset, isto
        //    grita em vez de o descobrirmos numa sessao de escuta.
        "pre_level_ab", "out_l", "out_r", "post_level_l", "post_level_r"};
    int erros_fora = 0;
    for(const char* n : devem_ficar_fora)
        if(cotuca::EhDeCadeia(n) || cotuca::EhGlobal(n))
        {
            printf("       -> %s entrou no preset e nao devia\n", n);
            erros_fora++;
        }
    Check("as 11 exclusoes ficam fora, uma a uma", erros_fora == 0);

    // E o gemeo: params que TEM que entrar. Sem isto, uma exclusao larga
    // demais (`if(strncmp(nome,"g",1))`) passaria nos testes acima.
    const char* devem_entrar[] = {"drv_drive_a", "drv_drive_b", "gate_thresh_a",
                                  "shm_tone_b", "dly_time",
        // `pre_level_ab` SAIU desta lista em 02/09. Estava aqui como
        // "param de musica que tem de entrar no preset", e o dB tirou-o do
        // preset em 01/09 junto com `out_l`, `out_r` e `post_level_l/r`:
        // sao ajuste de sala e de botao fisico, nao de musica. O teste
        // guardava o requisito revogado e acusava o codigo por o cumprir.
                                  "piezo_mix", "buzios_thresh"};
    int erros_dentro = 0;
    for(const char* n : devem_entrar)
        if(!cotuca::EhDeCadeia(n) && !cotuca::EhGlobal(n))
        {
            printf("       -> %s ficou fora do preset e devia entrar\n", n);
            erros_dentro++;
        }
    Check("os params de musica entram (exclusao nao e larga demais)",
          erros_dentro == 0);

    // ---- O HASH ----
    printf("\nVersao do mapa (FNV-1a sobre os nomes, na ordem)\n");
    const uint16_t base = cotuca::MapaHash(t);
    snprintf(d, sizeof(d), "0x%04X", base);
    Check("o hash do manifest atual e estavel entre chamadas",
          cotuca::MapaHash(t) == base, d);

    // MUDA QUANDO DEVE: renomear.
    {
        static cotuca::ParamTable t2;
        t2.Init();
        t2.Add("drv_drve_a", 0.f, 0.f, 1.f);   // um typo
        const uint16_t h1 = cotuca::MapaHash(t2);
        static cotuca::ParamTable t3;
        t3.Init();
        t3.Add("drv_drive_a", 0.f, 0.f, 1.f);
        const uint16_t h2 = cotuca::MapaHash(t3);
        snprintf(d, sizeof(d), "0x%04X vs 0x%04X", h1, h2);
        Check("renomear um param MUDA o hash", h1 != h2, d);
    }

    // MUDA QUANDO DEVE: reordenar.
    {
        static cotuca::ParamTable a, b;
        a.Init(); a.Add("um", 0.f, 0.f, 1.f); a.Add("dois", 0.f, 0.f, 1.f);
        b.Init(); b.Add("dois", 0.f, 0.f, 1.f); b.Add("um", 0.f, 0.f, 1.f);
        snprintf(d, sizeof(d), "0x%04X vs 0x%04X",
                 cotuca::MapaHash(a), cotuca::MapaHash(b));
        Check("reordenar MUDA o hash", cotuca::MapaHash(a) != cotuca::MapaHash(b), d);
    }

    // MUDA QUANDO DEVE: acrescentar.
    {
        static cotuca::ParamTable a, b;
        a.Init(); a.Add("um", 0.f, 0.f, 1.f);
        b.Init(); b.Add("um", 0.f, 0.f, 1.f); b.Add("dois", 0.f, 0.f, 1.f);
        Check("acrescentar um param MUDA o hash",
              cotuca::MapaHash(a) != cotuca::MapaHash(b));
    }

    // O SEPARADOR FAZ SEU TRABALHO: sem ele, "ab"+"c" e "a"+"bc" dariam
    // o mesmo hash, e uma renomeacao que so move a fronteira entre dois
    // nomes passaria despercebida.
    {
        static cotuca::ParamTable a, b;
        a.Init(); a.Add("ab", 0.f, 0.f, 1.f); a.Add("c", 0.f, 0.f, 1.f);
        b.Init(); b.Add("a", 0.f, 0.f, 1.f);  b.Add("bc", 0.f, 0.f, 1.f);
        snprintf(d, sizeof(d), "0x%04X vs 0x%04X",
                 cotuca::MapaHash(a), cotuca::MapaHash(b));
        Check("\"ab\"+\"c\" difere de \"a\"+\"bc\" (o separador serve)",
              cotuca::MapaHash(a) != cotuca::MapaHash(b), d);
    }

    // NAO MUDA QUANDO NAO DEVE: a faixa. Esta e a metade que quase nunca
    // se testa, e a que impede o hash de ficar largo demais.
    {
        static cotuca::ParamTable a, b;
        a.Init(); a.Add("drv_drive_a", 0.f, 1.5f, 48.f);
        b.Init(); b.Add("drv_drive_a", 0.f, 0.5f, 96.f);  // outra faixa
        snprintf(d, sizeof(d), "0x%04X vs 0x%04X",
                 cotuca::MapaHash(a), cotuca::MapaHash(b));
        Check("mudar a FAIXA nao muda o hash (o preset guarda normalizado)",
              cotuca::MapaHash(a) == cotuca::MapaHash(b), d);
    }

    // ---- A RECUSA ----
    printf("\nConferencia do cabecalho: cada defeito com o seu veredito\n");
    {
        uint8_t vals[3] = {10, 20, 30};
        cotuca::PresetHeader h;
        h.magic       = cotuca::kPresetMagic;
        h.formato     = cotuca::kFormato;
        h.n_params    = 3;
        h.versao_mapa = base;
        h._pad        = 0;
        h.checksum    = cotuca::Checksum(h, vals, 3);

        Check("cabecalho bom passa",
              cotuca::Conferir(h, vals, 3, base, 3) == cotuca::PresetVeredito::Ok);

        cotuca::PresetHeader x = h; x.magic = 0;
        Check("magic errado -> MagicErrado",
              cotuca::Conferir(x, vals, 3, base, 3)
                  == cotuca::PresetVeredito::MagicErrado);

        x = h; x.formato = (uint8_t)(cotuca::kFormato + 1);
        Check("formato diferente -> FormatoDiferente (nao 'corrupcao')",
              cotuca::Conferir(x, vals, 3, base, 3)
                  == cotuca::PresetVeredito::FormatoDiferente);

        x = h; x.n_params = 99;
        Check("contagem alem do buffer -> TamanhoImpossivel (nao le memoria)",
              cotuca::Conferir(x, vals, 3, base, 3)
                  == cotuca::PresetVeredito::TamanhoImpossivel);

        Check("dado corrompido -> ChecksumErrado",
              cotuca::Conferir(h, (const uint8_t[]){10, 20, 31}, 3, base, 3)
                  == cotuca::PresetVeredito::ChecksumErrado);

        // O TESTE QUE A ORDEM EXISTE PARA PASSAR.
        //
        // Um bit trocado NO CAMPO DO HASH tem que ler como CORRUPCAO, e
        // nao como "gravado sob outro mapa". A versao anterior conferia o
        // mapa antes do checksum e reportava remapeamento — diagnostico
        // errado com aparencia de certo, que manda o operador cacar um
        // remapeamento que nunca houve.
        x = h; x.versao_mapa ^= 0x0040u;   // um bit, sem recalcular o checksum
        {
            const cotuca::PresetVeredito v = cotuca::Conferir(x, vals, 3, base, 3);
            snprintf(d, sizeof(d), "veredito = \"%s\"", cotuca::Porque(v));
            Check("bit trocado NO CAMPO DO HASH le como corrupcao, nao remapeamento",
                  v == cotuca::PresetVeredito::ChecksumErrado, d);
        }

        // E o gemeo: um remapeamento DE VERDADE (cabecalho integro, hash
        // legitimamente outro) tem que continuar lendo como remapeamento.
        // Sem este par, a ordem nova poderia estar mascarando tudo como
        // corrupcao e o teste acima passaria igual.
        x = h; x.versao_mapa = (uint16_t)(base + 1);
        x.checksum = cotuca::Checksum(x, vals, 3);   // integro, so outro mapa
        {
            const cotuca::PresetVeredito v = cotuca::Conferir(x, vals, 3, base, 3);
            snprintf(d, sizeof(d), "veredito = \"%s\"", cotuca::Porque(v));
            Check("remapeamento REAL (cabecalho integro) le como remapeamento",
                  v == cotuca::PresetVeredito::MapaDiferente, d);
        }

        x = h; x.n_params = 2; x.checksum = cotuca::Checksum(x, vals, 2);
        Check("contagem diferente e integra -> ContagemDiferente",
              cotuca::Conferir(x, vals, 3, base, 3)
                  == cotuca::PresetVeredito::ContagemDiferente);

        // Cada veredito com frase propria — e TODAS diferentes, senao dois
        // consertos diferentes leem igual na tela.
        const cotuca::PresetVeredito todos[] = {
            cotuca::PresetVeredito::MagicErrado,
            cotuca::PresetVeredito::FormatoDiferente,
            cotuca::PresetVeredito::TamanhoImpossivel,
            cotuca::PresetVeredito::ChecksumErrado,
            cotuca::PresetVeredito::MapaDiferente,
            cotuca::PresetVeredito::ContagemDiferente};
        int iguais = 0;
        for(size_t a = 0; a < sizeof(todos)/sizeof(todos[0]); a++)
        {
            if(!strcmp(cotuca::Porque(todos[a]), "?")) iguais++;
            for(size_t b = a + 1; b < sizeof(todos)/sizeof(todos[0]); b++)
                if(!strcmp(cotuca::Porque(todos[a]), cotuca::Porque(todos[b])))
                    iguais++;
        }
        Check("os seis vereditos tem frases distintas e nao-vazias", iguais == 0);
    }

    printf("\n%s — %d falha(s)\n", fails ? "FALHOU" : "TUDO OK", fails);
    return fails ? 1 : 0;
}
