// test_presets v0.1.0 — validação nativa das estruturas de preset.
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
    Check("preset de cadeia tem 33 params por cadeia", cad / 2 == 33);
    Check("preset global tem 9 params", glob == 9);
    Check("as duas cadeias tem o MESMO numero de params", cad % 2 == 0);

    // As exclusoes, uma a uma e por NOME: a contagem sozinha passaria se
    // duas exclusoes se cancelassem com duas inclusoes indevidas.
    const char* devem_ficar_fora[] = {
        "exc_on_a", "exc_on_b", "bench_boost_a", "bench_boost_b",
        "gov_ceiling_a", "gov_ceiling_b", "gov_release_a", "gov_release_b",
        "tone_freq", "tone_mix", "tone_dest"};
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
                                  "shm_tone_b", "level", "dly_time",
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
        h.versao_mapa = base;
        h.n_params    = 3;
        h.checksum    = cotuca::Checksum(vals, 3);

        Check("cabecalho bom passa",
              cotuca::Conferir(h, vals, base, 3) == cotuca::PresetVeredito::Ok);

        cotuca::PresetHeader x = h; x.magic = 0;
        Check("magic errado -> MagicErrado",
              cotuca::Conferir(x, vals, base, 3)
                  == cotuca::PresetVeredito::MagicErrado);

        x = h; x.versao_mapa = (uint16_t)(base + 1);
        Check("mapa diferente -> MapaDiferente",
              cotuca::Conferir(x, vals, base, 3)
                  == cotuca::PresetVeredito::MapaDiferente);

        x = h; x.n_params = 4;
        Check("contagem diferente -> ContagemDiferente",
              cotuca::Conferir(x, vals, base, 3)
                  == cotuca::PresetVeredito::ContagemDiferente);

        uint8_t maus[3] = {10, 20, 31};   // um bit trocado
        Check("dado corrompido -> ChecksumErrado",
              cotuca::Conferir(h, maus, base, 3)
                  == cotuca::PresetVeredito::ChecksumErrado);

        // O veredito tem que ter TEXTO, senao o dB le um numero. Foi a
        // licao do "CAIU pelo teto" contra "CAIU por ESTOURO".
        Check("todo veredito tem frase legivel",
              strcmp(cotuca::Porque(cotuca::PresetVeredito::MapaDiferente), "?") != 0
              && strcmp(cotuca::Porque(cotuca::PresetVeredito::ChecksumErrado), "?") != 0);
    }

    printf("\n%s — %d falha(s)\n", fails ? "FALHOU" : "TUDO OK", fails);
    return fails ? 1 : 0;
}
