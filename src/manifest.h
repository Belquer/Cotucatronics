#pragma once
// manifest.h (instrumento "bancada") v0.85.0
// ============================================================
// Cotucatronics OS — MANIFEST do instrumento
// Este é o ÚNICO arquivo que muda de escultura para escultura,
// junto com o export do RNBO em modules/<nome>/.
//
// Instrumento de teste: "bancada" — entrada + tom de teste
// atravessando o módulo RNBO "delay".
// ============================================================

#include "params.h"
#include "midi.h"

namespace cotuca
{

// O que este instrumento EXIGE do cérebro. O boot confere
// contra hal::Info() e recusa rodar em placa insuficiente —
// erro claro no boot em vez de bug misterioso no palco.
struct Requirements
{
    size_t big_ram_bytes = 0;     // ex: 8MB p/ freeze longo
    bool   needs_audio_in = true; // esculturas com piezo: sim
    int    min_knobs      = 4;

    // ---- TAMANHO DE BLOCO: decisão de INSTRUMENTO ----
    //
    // Define a latência, e num instrumento realimentado a latência é
    // parte do laço acústico — mexer nela muda ONDE o feedback se
    // estabelece. Por isso mora aqui e não no HAL.
    //
    // O prazo de cada bloco é block_size / sample_rate, e é ele que
    // um módulo pesado precisa caber. Módulo espectral concentra o
    // custo no bloco do hop: o que tem que caber é o PICO.
    //
    //    48  →  1000 us  — latência mínima, o normal do instrumento
    //   128  →  2667 us  — 2,7x de orçamento, 2,7 ms de latência
    //   256  →  5333 us  — 5,3x de orçamento, 5,3 ms de latência
    //
    // Medido com o vocoder (FFT 2048/hop 1024): pico de 5609 us em
    // regime — não coube nem no 256, por 5%. A variante FFT
    // 1024/hop 512 deve cair pra faixa de 2,5–2,8k us e caber no 256
    // com folga.
    // VOCODER FORA DO BUILD (2026-08-18, decisão de ouvido do dB):
    // volta pra 48. O 256 nunca foi escolha — era exigência da FFT do
    // vocoder, e saiu junto com ela.
    //
    // O que se ganha de volta é o que o 256 custava: a latência cai de
    // 5,3 ms pra 1 ms, e num instrumento realimentado isso REPÕE onde
    // o feedback se estabelece. O freeze que entra no lugar é granular
    // no domínio do tempo — não tem prazo mínimo de janela, roda em
    // qualquer bloco.
    //
    // Pra reviver o vocoder: 256 aqui e o módulo de volta no Makefile.
    // Os dois arquivos mudam JUNTOS — é por isso que a nota está nos
    // dois.
    size_t block_size = 48;
};

// Grau permitido = semitom MOD 12 relativo ao centro da cerca. Lá
// menor natural: a base da portadora é 110 Hz — um Lá — então a
// máscara nasce afinada de graça, sem transposição nenhuma.
// Outra escultura declara outra máscara; é DADO de manifest.
static constexpr uint8_t kScaleAminor[] = {0, 2, 3, 5, 7, 8, 10};

// Alvo de sorteio (B1 e búzios): cerca por parâmetro + máscara de
// escala opcional. Cerca em NORMALIZADO, sorteio uniforme dentro dela.
// Com máscara (n_scale > 0) o sorteio só pousa em semitons permitidos:
// passo norm_per_semi a partir de center_norm.
struct RandTarget
{
    int            id            = -1;
    float          min_norm      = 0.f;   // cerca; ver real_fence
    float          max_norm      = 1.f;
    const uint8_t* scale         = nullptr;
    int            n_scale       = 0;
    float          center_norm   = 0.5f;
    float          norm_per_semi = 0.f;
    // true = a cerca está em REAL (unidade do param) e o sorteio é
    // uniforme NELA. Necessário quando o param tem curva != 1: sorteio
    // uniforme em norm entortaria a distribuição — "100 a 500 ms"
    // viraria "quase sempre perto de 100".
    bool           real_fence    = false;
    // Passo do sorteio em REAL (0 = contínuo). Com real_fence, o
    // sorteio pousa só em min + k*passo — degraus perceptíveis em vez
    // de variação contínua que o ouvido lê como "quase igual".
    float          real_step     = 0.f;
};

// Semântica do botão 2. Momentânea por ora; Toggle fica declarado
// pra o dia em que existir, mas não implementado — declarar sem
// implementar é melhor que improvisar depois sem lugar previsto.
enum class ButtonMode
{
    Hold,   // param = 1 enquanto pressionado, volta ao anterior ao soltar
    Toggle, // FUTURO: alterna a cada pressionada
};

struct Manifest
{
    Requirements req; // instrumento de teste: defaults servem

    // ids de parâmetros preenchidos no Init()
    int p_level;      // volume master
    int p_tone_freq;  // freq do oscilador de teste
    int p_tone_mix;   // mistura tom de teste vs entrada
    // BALANCO dos dois piezos, nao crossfade. Centro = OS DOIS CHEIOS;
    // girar abaixa o dominante sem cortar o outro. O crossfade que
    // estava aqui (`src_mix`, aposentado 20/08) derrubava OS DOIS no
    // meio — -6 dB cada no centro — e por isso nao resolvia "um
    // encobre o outro".
    //
    // FAIXA SIMETRICA [-1,+1] com neutro em 0: e o que faz o default
    // zero cair no centro sozinho, sem excecao a regra. CC 64 = centro.
    //
    // AGORA PODE IR PRO MIDI, e o `src_mix` nao podia. Aquele emudecia
    // o instrumento nos extremos (3x num dia so); este, no extremo,
    // perde UM piezo e mantem o outro. Nao existe posicao muda.
    int p_piezo_mix;  // -1 = so IN_L ... 0 = ambos cheios ... +1 = so IN_R
    // ---- GANHO DE ENTRADA — pós-fonte, PRÉ-GATE ----
    // O lugar que este manifest reservou pra ele desde o primeiro dia,
    // no bloco do bench_boost: "o ganho de compensação do piezo NÃO
    // mora no level: mora pós-entrada / pré-gate".
    //
    // POR QUE PRÉ-GATE, e não depois: o gate precisa decidir sobre o
    // sinal JÁ no nível certo. Amplificar depois dele significaria
    // amplificar junto o que ele deixou passar, e o limiar do gate
    // passaria a significar coisas diferentes conforme o ganho.
    //
    // POR QUE NÃO É O MESMO QUE SUBIR O LEVEL: o level empurra a
    // SAÍDA contra o governor e o limiter — mais volume ali é mais
    // compressão, que é o artefato que se quer evitar. O ganho de
    // entrada levanta o sinal ANTES da cadeia inteira, então delay,
    // vocoder e reverb recebem material mais quente e a dinâmica
    // continua trabalhando na mesma proporção.
    //
    // O QUE ELE NÃO RESOLVE, e é honesto dizer: ele levanta o CHÃO DE
    // RUÍDO do piezo junto com o sinal. A relação sinal/ruído não
    // melhora — o que melhora é o quanto da faixa útil se usa. Se o
    // ruído começar a incomodar, o conserto é o buffer de alta
    // impedância (receita no BOARDS.md), não mais ganho aqui.
    // ---- DUAS CADEIAS DE CARATER (pivo de 20/08) ----
    //
    // POR QUE DUPLICAR, e a razao NAO e desequilibrio entre transdutores.
    //
    // Cada piezo capta um MATERIAL diferente do instrumento hibrido: o do
    // tambor le transientes curtos e fortes; o da mola/corda le raspagem
    // continua de baixo nivel. E diferenca de FONTE, nao de transdutor.
    // Um limiar de gate que deixa a raspagem passar abre demais pro
    // tambor; um ganho que serve a corda satura no golpe.
    //
    // A duplicacao vale com dois piezos perfeitamente casados, e valeria
    // se fossem identicos de fabrica. ISTO IMPORTA ESTAR ESCRITO: houve
    // um momento em que o p2 chegava mais baixo e isso quase virou a
    // justificativa — era DEFEITO FISICO de um piezo, nao propriedade do
    // circuito, e com os dois em ordem os canais chegam iguais. Se
    // ficasse registrado "duplicamos porque p2 era mais baixo", alguem
    // mede os canais iguais no futuro e conclui que a duplicacao e
    // dispensavel. Justificativa errada envelhece mal.
    static constexpr int kCadeias = 2;
    int p_in_gain[kCadeias];
    int p_dly_time;   // → módulo RNBO "dly_time"
    int p_dly_mix[kCadeias];    // → módulo RNBO "dly_mix"
    int p_dly_on[kCadeias];     // 0/1 — chave própria (o mix deixa de ser a chave)

    // ---- cadeia de dinâmica do shell (C++ puro, não RNBO) ----
    // Todos serial-only: são política de segurança e comportamento,
    // não gesto de performance. O painel tem 5 knobs e nenhum deles
    // se gasta aqui.
    int p_gate_thresh[kCadeias];
    int p_gate_release[kCadeias];
    int p_gov_ceiling[2];
    int p_gov_release[2];

    // Roteamento de saída: 0/1. DOIS ANDARES, e eles NÃO fazem a
    // mesma coisa — 28/08, e a confusão entre os dois é o que fez
    // `route_jack` existir com esse nome:
    //
    //   route_*  ANTES dos efeitos. Matriz 2×2: qual CADEIA entra em
    //            qual SAÍDA (A→L, A→R, B→L, B→R). É aqui que se
    //            trocam as cadeias quando os transdutores estão
    //            ligados invertidos, sem duplicar preset nenhum.
    //   out_*    DEPOIS do limiter. Corta o RAMO inteiro, e continua
    //            a ser o único mute de verdade: zerar uma coluna da
    //            matriz cala o seco, o freeze e o espaço daquela
    //            cadeia, mas o retorno do delay ainda entra (uma
    //            instância só, envios já somados — ver main.cpp).
    //
    // O FIRMWARE NÃO NOMEIA DESTINO, e é por isso que `route_jack`
    // era um nome falso. Cada saída passa por um DPDT de três
    // posições — jack | ambos | amp — que mora DEPOIS daqui. `out[1]`
    // não é "o jack": é uma saída que PODE estar no jack, e a chave é
    // que decide. O firmware nomeia SAÍDA; o cobre nomeia destino.
    int p_exc_on[2];    // exciter no ar? gateia a protecao do ramo
    int p_route_l[2];   // a cadeia A/B entra em OUT_L?
    int p_route_r[2];   // a cadeia A/B entra em OUT_R?
    int p_out_l;        // OUT_L no ar?  (mute pos-limiter)
    int p_out_r;        // OUT_R no ar?

    // ============================================================
    // >>>>>>>>>>>>>  FERRAMENTA DE BANCADA  <<<<<<<<<<<<<
    //
    // NUNCA mapear bench_boost a knob, encoder, botão ou cena.
    // Ele existe por um motivo só: dar headroom pra EXERCITAR o
    // limiter em teste. O caminho de sinal normal não consegue
    // passar de ~0,5 interno, e um limiter que nunca engata é um
    // limiter não testado.
    //
    // Por que não subir o `level` em vez disso: level [0,1] é
    // CONTRATO DE ERGONOMIA DO PALCO. O knob de volume da
    // escultura não pode ter quatro quintos de curso perigoso.
    //
    // E o ganho de compensação do piezo, quando vier, NÃO mora
    // aqui nem no level: mora pós-entrada / pré-gate, que é onde
    // ele pertence — antes do gate poder decidir sobre o sinal já
    // no nível certo.
    //
    // NÃO PERSISTE. Nasce sempre em 1.0. O boot tem tripwire.
    // ============================================================
    int p_bench_boost[2];

    // SHIFTER — bloco PERMANENTE do shell (dynamics.h), não módulo.
    // Anti-feedback: deslocar alguns Hz quebra a coerência do laço
    // piezo→corpo→piezo antes que ele cresça — o sinal que volta não
    // coincide mais em fase com o que saiu, e a realimentação não
    // encontra frequência pra travar.
    //
    // >>> DESTINO DO ENCODER FÍSICO, junto com gov_ceiling. Os dois
    // >>> formam o controle da "borda de feedback": gov_ceiling diz
    // >>> ONDE parar de crescer, shift_hz diz O QUANTO desmanchar o
    // >>> laço. Push do encoder alterna entre os dois.
    //
    // 0 Hz = desligado. Acima de ~10 Hz começa a soar como efeito
    // (metálico, desafinado) em vez de proteção transparente.
    int p_shift_hz;

    // ---- REVERB — bloco PERMANENTE do shell (reverb.h) ----
    // Freeverb clássico, pós-fx2 e pré-level: a sala envolve o
    // instrumento inteiro, não um canal dele.
    //
    // Serial/MIDI por ora — os cinco knobs estão cheios. Escultura que
    // quiser reverb na mão declara um macro aqui; é decisão de peça,
    // não do shell.
    //
    // rev_mix nasce em 0.15: presença de sala sem virar efeito. Reverb
    // é o ar em volta da escultura, e ar demais afoga o gesto do piezo.
    // Passa-alta NO CAMINHO DO FX2, não no master. O estouro só
    // acontece com o vocoder ligado (medido), então filtrar o master
    // castigaria o delay de graça. 0 Hz = desligado.

    // ENVIO DA CADEIA PRO ESPACO — mesma forma do delay e do freeze.
    //
    // Ate 20/08 havia TRES controles pro mesmo caminho: `esp_mix` na
    // entrada, `rev_mix` na saida, e duas chaves. Numa sala LINEAR
    // ganho na entrada e ganho na saida sao o MESMO controle escrito
    // duas vezes — o dB viu antes de mim.
    //
    // Ficou um so: `rev_on`/`rev_mix` dizem quanto da cadeia entra no
    // espaco. O retorno volta inteiro, porque num send/return o retorno
    // E o molhado — o seco ja passa por fora.
    //
    // `shm_mix` VOLTOU AO EMPREGO UNICO: so a camada pitcheada direta.
    // Ele tambem escalava a saida da sala quando o reverb estava
    // desligado, que e a familia C do catalogo (variavel com dois
    // empregos) plantada por mim ontem.
    int p_rev_on[kCadeias];     // 0/1 — chave. ATENÇÃO: emudece o shimmer junto
    int p_rev_mix[kCadeias];
    int p_rev_size[kCadeias];
    int p_rev_damp[kCadeias];

    // FREEZE — bloco permanente do shell (freeze.h), no lugar do
    // vocoder na cadeia: pós-delay, pré-reverb. O congelado ganha a
    // sala junto, que é o que faz ele PERTENCER ao instrumento em vez
    // de flutuar por cima.
    // FREEZE: UM POR CADEIA, e SEM chave separada.
    //
    // NAO HA CHAVE SEPARADA porque congelar JA E o liga/desliga. Foi o
    // dB que viu: "ele nao precisa de on/off ao contrario de todos os
    // outros, o hold dele ja e on off".
    //
    // E o param se chama `frz_on` e nao `frz_hold` pra ler igual aos
    // vizinhos na lista — o gesto e o mesmo, o nome e que acompanha a
    // familia. Consistencia de vocabulario onde o comportamento ja e
    // consistente.
    //
    // DOIS FREEZES DE VERDADE, nao um com envio: cada cadeia congela
    // SEU material, no SEU instante. Um freeze so, alimentado pela
    // soma, congelaria os dois juntos — e o gesto e justamente pegar
    // um material enquanto o outro segue vivo.
    int p_frz_on[kCadeias];     // A CHAVE, que aqui E o gesto de congelar
    int p_frz_vol[kCadeias];    // nivel do congelado
  // 0..1 seco/congelado
    int p_frz_size[kCadeias];

    // DRIVE — waveshaper tanh como COLA. Primeiro da cadeia: sujeira
    // antes de modulacao, tempo e espaco (ordem de pedaleira).
    // CARATER: PARAMS COMPARTILHADOS, chave e dry/wet POR CANAL.
    //
    // Seis efeitos com params independentes ficou confuso de operar
    // (decisao de uso do dB, 20/08). Volta a existir UM drive, UM
    // phaser, UM warble — o que duplica e so o que se opera ao vivo.
    //
    // NAO ESPERE ECONOMIA DE CPU, e isto foi verificado na estrutura
    // antes de medir: cada efeito segue processando os DOIS sinais com
    // estados internos separados (dois conjuntos de allpass, dois
    // buffers de warble), senao os canais se contaminam. O que encolhe
    // e a superficie de controle e a tabela — nao o trabalho.
    int p_drv_on[kCadeias];     // 0/1 — chave, POR CANAL
    int p_drv_mix[kCadeias];    // 0..1 — dry/wet, POR CANAL
    int p_drv_drive[kCadeias];   // POR CADEIA (era compartilhado ate v0.131.0)
    int p_drv_tone[kCadeias];   // POR CADEIA (era compartilhado ate v0.131.0)
    int p_drv_sujeira[kCadeias];   // POR CADEIA (era compartilhado ate v0.131.0)

    // PHASER — N allpass varridos por LFO. Depois do drive, antes do
    // delay: sujeira, depois movimento, depois tempo e espaco.
    //
    // ATENCAO ao `phs_mix`: e INVERTIDO em relacao aos outros efeitos.
    // O notch nasce de CANCELAMENTO entre seco e desviado, e
    // cancelamento maximo exige amplitudes iguais — entao ~0.5 e o
    // "ligado" e 1.0 e quase inaudivel. Ver phaser.h.
    int p_phs_on[kCadeias];        // chave, POR CANAL
    int p_phs_mix[kCadeias];       // dry/wet POR CANAL · MONTANHA: pico em ~0.5
    int p_phs_rate[kCadeias];   // POR CADEIA (era compartilhado ate v0.131.0)
    int p_phs_depth[kCadeias];   // POR CADEIA (era compartilhado ate v0.131.0)
    int p_phs_feedback[kCadeias];   // POR CADEIA (era compartilhado ate v0.131.0)
    int p_phs_stages[kCadeias];   // POR CADEIA (era compartilhado ate v0.131.0)

    // WARBLE — deriva de fita: delay curto modulado. Depois do phaser
    // (sujeira, movimento de fase, movimento de altura), antes do
    // delay.
    //
    // NAO tem `mix`: fita nao mistura com ela mesma. O sinal inteiro
    // deriva, ou nao deriva — misturar com o seco daria flanger, que e
    // outro efeito.
    // WARBLE GANHOU `mix` EM 20/08, por decisao do dB (consistencia:
    // todo efeito de caracter tem chave e dry/wet por canal).
    //
    // ISTO MUDA UM EFEITO APROVADO. Ate aqui o dry/wet do warble era o
    // crossfade do EffectSlot, e ligar bastava pra ouvir. Agora, com
    // mix em 0 — que e o default — ele fica MUDO ligado, igual a drive
    // e phaser. Reaceite pelo IFA10000 esta pendente.
    int p_wrb_on[kCadeias];     // chave, POR CANAL
    int p_wrb_mix[kCadeias];    // dry/wet, POR CANAL
    int p_wrb_rate[kCadeias];   // POR CADEIA (era compartilhado ate v0.131.0)
    int p_wrb_depth[kCadeias];   // POR CADEIA (era compartilhado ate v0.131.0)
    int p_wrb_irreg[kCadeias];   // POR CADEIA (era compartilhado ate v0.131.0)

    // ---- PARAM PRIMARIO POR EFEITO ----
    //
    // O que o pot de "parameter" do painel controla. E destilacao
    // forcada: se nao da pra nomear o knob que mais importa, o efeito
    // nao esta maduro. Tambem e o unico param, alem do on/off, que
    // recebe CC — os secundarios seguem alcancaveis por `set`.
    struct EfeitoCaracter
    {
        int         p_on;
        int         p_primario;
        const char* nome;
        const char* nome_primario;
    };
    EfeitoCaracter caracter[kCadeias][3];

    // TOM DE TESTE: pra qual cadeia. 0=A 1=B 2=AMBAS (default).
    //
    // Tom que entra so numa cadeia deixa a outra INVISIVEL ao teste, e
    // canal mudo prova so que aquele canal esta mudo. Sem destino
    // selecionavel, nenhum teste futuro distingue "B quebrada" de "B nao
    // excitada". AMBAS valida simetria de uma vez; A ou B isolam quando
    // o sintoma for assimetrico.
    int p_tone_dest;
    int p_test_tone_stage;   // 0=pre (pos-level) · 1=post (cadeia)

    // DETUNE — vozes micro-desafinadas + largura. Depois do modulo,
    // onde o estereo existe: sem estereo nao ha spread.

    // SHIMMER — reverb + pitch-up num laco de realimentacao. Nao tem
    // reverb proprio: realimenta o Freeverb da cadeia.
    //
    // NAO HA `shm_decay`, e e deliberado. A especificacao pedia o
    // param E dizia "decay = tempo de cauda do proprio reverb" — o que
    // faria dele um segundo dono de `rev_size`. Variavel com dois
    // empregos e a terceira familia de bug de ordem deste projeto.
    // `rev_size` (CC 25) E o decay, e a dependencia esta declarada.
    // ON/OFF EXPLICITO, pedido do dB: todo efeito que entra na suite
    // ganha o seu. Antes o shimmer ligava quando `mix > 0`, o que
    // funde duas decisoes numa so — desligar obrigava a zerar o mix e
    // perder o ajuste. Chave separada preserva a posicao do fader.
    int p_shm_on[kCadeias];       // 0/1 — chave
    int p_shm_mix[kCadeias];      // 0..1 — CAMADA por cima do seco
    int p_shm_shift[kCadeias];    // 0=+12  1=+7  2=ambos
    int p_shm_feedback[kCadeias]; // 0..1 — quantas oitavas empilham
    int p_shm_tone[kCadeias];     // 0..1 — 0 = 6 kHz (quente) ... 1 = 18 kHz

    // ------------------------------------------------------------
    // ORDEM DE SACRIFICIO — a politica do TETO DE SOMA do CTOS.
    //
    // DUAS SITUACOES DIFERENTES, DOIS MECANISMOS. O watchdog por-efeito
    // pega o efeito BUGADO: um efeito que sozinho estoura seu orcamento
    // tem defeito individual, e o veto e justo porque ha culpado. O teto
    // de soma pega outra coisa: N efeitos SAUDAVEIS que juntos nao cabem
    // no prazo do bloco. Ali nao ha culpado — e excesso de ambicao, e
    // procurar culpa fabricaria um.
    //
    // Entao nao se pergunta "quem estourou?" (ninguem), e sim "qual o
    // instrumento perde com menos dano?". A resposta e PRIORIDADE
    // DECLARADA: o dB decide A FRIO, aqui, fora do calor do overrun.
    //
    // POR QUE NAO LIFO. Derrubar o ultimo ligado e aproximacao: pune por
    // ordem de chegada, que nao tem relacao com valor musical. O ultimo
    // ligado pode ser exatamente o que se estava buscando.
    //
    // A ORDEM DO ARRAY E A PRIORIDADE. Indice 0 cai primeiro. Reordenar
    // aqui e a unica forma de mudar a politica — nao ha numero de
    // prioridade escondido em lugar nenhum.
    struct Sacrificavel
    {
        int         param_on;
        const char* nome;
    };
    // ESTE LIMITE E LIDO NO CALLBACK DE AUDIO, e por isso ele nao pode
    // seguir a tabela de params.
    //
    // Uma varredura de 23/08 procurou "que custos escalam com uma tabela
    // que cresceu?" e achou o `sync` gastando 258 ms onde foi dimensionado
    // para 36 (bts 13p). O callback saiu limpo — o unico laco sobre tabela
    // ali e o do `n_sacrif` — mas saiu limpo por SORTE DE DESENHO: nada
    // dizia que ele nao podia escalar com params, e ninguem tinha
    // verificado.
    //
    // O assert transforma a sorte em garantia. Se um dia alguem quiser
    // sacrificar POR PARAM em vez de por efeito, o build reclama antes de
    // o callback engordar — que e o unico lugar onde engordar custa
    // amostra perdida, e nao apenas paciencia.
    //
    // Se este numero precisar passar de 16, a pergunta certa nao e "posso
    // aumentar?" e sim "isto ainda e por EFEITO?".
    static constexpr int kMaxSacrificaveis = 12;
    static_assert(kMaxSacrificaveis <= 16,
                  "kMaxSacrificaveis e varrido no callback de audio: tem que "
                  "ser constante pequena ligada a contagem de EFEITOS, nunca "
                  "a de params, que cresce (ver battle_scars 13p)");
    Sacrificavel sacrif[kMaxSacrificaveis];
    int          n_sacrif = 0;

    // ---- FREEZE (slot Fx2) ----
    // Serial-only: os cinco knobs estao todos ocupados. As faixas
    // ESPELHAM as do export, porque o modulo recebe normalizado e o
    // RNBO remapeia — declarar [-24,24] aqui faz `set frz_transp 12`
    // significar 12 semitons de verdade.
    // BYPASS do slot Fx2. NASCE DESLIGADO de proposito.
    //
    // Um modulo pesado que estoura o prazo do bloco de audio nao
    // "soa mal": ele mata o loop principal por inanicao, e a placa
    // fica surda no serial — sem log, sem cpu, sem jeito de
    // diagnosticar a nao ser regravando. Ja aconteceu com o freeze.
    //
    // Com bypass, o boot sempre responde, e ligar o modulo vira um
    // experimento reversivel em vez de uma aposta.

    // Variante ENXUTA do patch: a cirurgia do dB fixou auto, fade e
    // pitchjit internamente e removeu transp/panjit/ampjit. Restam
    // dois params vivos. O export ainda EXPOE lowcut, highcut,
    // direct, auto, autodel, fade e pitchjit — nao ligamos porque,
    // segundo o autor, estao fixos por dentro; ligar daria a ilusao
    // de controle sobre algo que nao responde.
    // ---- VOCODER (slot Fx2) ----
    // Cross-synthesis: ch1 = escultura (modulador), ch2 = jack de
    // linha CRU (portadora).
    //
    // `voc_tone` nasceu no export de 2026-08-16 e ficou com o k5, que
    // estava livre desde o painel v2. mix e pitch seguem serial-only.

    // ---- GANHO DE COMPENSAÇÃO DO SLOT Fx2 ----
    // O vocoder sai MUITO mais baixo que o delay. Não é defeito:
    // cross-synthesis espectral só devolve energia onde as duas fontes
    // coincidem, e coincidência é raríssima comparada com um delay, que
    // devolve o sinal inteiro.
    //
    // POR QUE UM PARÂMETRO NOVO, e não subir `level` ou `bench_boost`:
    //
    //   level        é o master do palco, contrato [0,1]. Gastá-lo pra
    //                compensar um módulo faria o knob de volume da
    //                escultura significar coisas diferentes conforme o
    //                Fx2 estivesse ligado ou não.
    //   bench_boost  é ferramenta de bancada, proibida de sair da
    //                bancada. Ver o bloco dela acima.
    //
    // Isto é CASAMENTO DE NÍVEL ENTRE MÓDULOS, que é problema próprio e
    // merece nome próprio. Fica no slot, não no master: trocar o módulo
    // do Fx2 troca o ganho junto, como tem que ser.
    //
    // Aplicado ANTES do level e da dinâmica, então governor e limiter
    // continuam protegendo — é por isso que 8x é seguro de oferecer.

    // ============================================================
    // PAINEL v2 — 5 pots + 2 botões momentâneos
    //
    //   k1 = level      k2 = piezo_mix    k3–k5 = macros de caráter
    //   B1 = randomizador                 B2 = assinalável
    // ============================================================

    // ---- B1: RANDOMIZADOR, com whitelist EXPLÍCITA ----
    //
    // Só entra aqui parâmetro de CARÁTER do módulo — o que muda o som
    // sem mudar o que o instrumento é. Ficam PROIBIDOS por design:
    //
    //   level          é o volume. Sortear volume no palco é o mesmo
    //                  que dar um susto em quem está ouvindo.
    //   src_mix        escolhe a FONTE. Sortear isso troca o
    //                  instrumento, não o som dele.
    //   gate_*         política sobre o chão de ruído do piezo.
    //   gov_*, limiter proteção do exciter. Sortear proteção é
    //                  sortear se o hardware sobrevive.
    //   route_*, out_* sortear roteamento pode emudecer a peça, ou
    //                  mandar para o transdutor errado o que estava
    //                  endereçado ao outro.
    //   shift_hz       anti-feedback. Mesmo argumento do governor.
    //   bench_boost    ferramenta de bancada, nunca de performance.
    //
    // A regra por trás: o botão pode surpreender o TIMBRE, nunca a
    // SEGURANÇA nem a IDENTIDADE do instrumento. Whitelist e não
    // blacklist porque param novo nasce fora — esquecer de proibir é
    // erro silencioso, esquecer de permitir só faz o botão ignorá-lo.
    RandTarget randomizable[8];
    int        n_randomizable = 0;

    // ---- BÚZIOS: o sorteio disparado por transiente ----
    //
    // Nome do dB, e ele descreve o gesto melhor que qualquer termo
    // técnico: cada batida na escultura é uma jogada de búzios, e o
    // que cai decide o timbre. A escala de Lá menor é o que faz a
    // sorte cair sempre dentro do tom — surpresa sem nota errada.
    // Detector no shell (main.cpp), pós-gate: batida acima do limiar
    // dispara UM sorteio nas mesmas cercas do B1. buzios_on nasce em 0
    // — é gesto de performance, liga por serial/MIDI, e é candidato ao
    // B2 como toggle quando Toggle existir.
    int p_buzios_on;
    int p_buzios_thresh;   // limiar do envelope pós-gate, 0..1
    int p_buzios_cooldown; // ms entre sorteios; 2 batidas rápidas = 1

    // ---- B2: ASSINALÁVEL ----
    // -1 = sem função. Momentâneo: segura, vale 1; solta, volta.
    // ---- BOTOES COM ALVO, generalizado ----
    //
    // Era `button2_target`, um alvo unico, e o segundo botao fisico
    // (freeze B) obrigaria a repetir o gesto inteiro — que e quando as
    // duas versoes comecam a divergir no primeiro conserto. Agora e
    // tabela: cada botao declara o param que alterna e o pino do LED
    // que espelha esse param.
    //
    // `led_hw` e o INDICE do LED no HAL, nao o pino: o manifest nao
    // conhece pino, e a fronteira board-agnostic e o motivo de esta
    // base existir. -1 = sem LED.
    // SENTINELA NEGATIVA NAO SOBREVIVE A ZERO-INIT, e este projeto ja
    // pagou por isso hoje (o `SacrifTravado` devolvendo -1 testado como
    // booleano). `BotaoAlvo x[] = {}` poe param=0 e led_hw=0 — e ZERO E
    // INDICE VALIDO de parametro e de LED. Sem os inicializadores
    // abaixo, o botao randomizador alternaria o `level` e acenderia o
    // LED do freeze A.
    //
    // O relatorio do `board` pegou isso na primeira leitura: "B1 (?):
    // alterna level | LED 0". Instrumento que declara o proprio estado
    // paga o custo dele na primeira execucao.
    struct BotaoAlvo
    {
        int         param  = -1;   // o que alterna na borda, -1 = nenhum
        int         led_hw = -1;   // indice do LED que espelha, -1 = nenhum
        const char* nome   = nullptr;
    };
    static constexpr int kMaxBotoes = 6;
    BotaoAlvo  botao[kMaxBotoes] = {};
    int        n_botoes = 0;

    void AddBotao(int idx, int param, int led_hw, const char* nome)
    {
        if(idx < 0 || idx >= kMaxBotoes) return;
        botao[idx] = {param, led_hw, nome};
        if(idx + 1 > n_botoes) n_botoes = idx + 1;
    }
    ButtonMode button2_mode   = ButtonMode::Hold;

    // ---- ligação com o módulo DSP, POR NOME ----
    // O manifest não conhece índice de parâmetro do RNBO: reexportar
    // o patch reordena tudo. Declara-se o nome, e o boot resolve o
    // índice via Module::FindParam(). Nome que não existe = erro
    // claro no log, não parâmetro fantasma mexendo em nada.
    // O SISTEMA DE MODULOS SAIU EM 20/08 junto com o RNBO. O delay,
    // ultimo modulo, virou C++ no shell (delay.h). Nao ha mais slot,
    // bind, nem push por nome — todo efeito le os params direto, como
    // sempre foi pros efeitos da casa.

    // ============================================================
    // ---- MIDI: CC → parâmetro ----
    //
    // MIDI É UM KNOB REMOTO, NÃO UM CAMINHO NOVO. O CC entrega um
    // valor normalizado à ParamTable e para por aí — exatamente onde
    // o knob físico para. Tudo que vem depois (curva do manifest,
    // Shaped() pro módulo, Real() pro DSP) acontece igual, porque é
    // a mesma via. Um `dly_time` movido por CC tem a mesma ergonomia
    // curvada de quando é movido pelo knob 1.
    //
    // Isso não é coincidência de implementação: os knobs deste
    // manifest são macros de UM alvo com peso 1.0, e `Macro::Apply`
    // com um alvo de peso 1.0 é literalmente `t.Set(id, v)`. Não há
    // divergência possível entre as duas vias enquanto for assim.
    // Se algum knob virar macro composta e o mesmo gesto tiver que
    // ser alcançável por MIDI, o certo é apontar o CC pra macro —
    // não duplicar os alvos aqui.
    //
    // A tabela é CC → PARÂMETRO e não CC → knob de propósito: o
    // painel tem 5 knobs e o instrumento tem 15 parâmetros. MIDI
    // alcança os que não couberam no painel — gov_ceiling é o
    // exemplo, e é o mais importante deles.
    // ============================================================
    int        midi_channel = kMidiOmni; // omni: aceita todos os canais
    MidiCcBind midi_map[kMaxMidiMap];
    int        midi_overflow = 0; // entradas recusadas por falta de espaco

    // TODA escrita no mapa passa por aqui. A versao anterior fazia
    // `midi_map[n_midi++] = {...}` direto, SEM GUARDA — e a 17a entrada
    // escreveu FORA DO ARRAY. Estouro de capacidade e defeito de
    // PROJETO: quem adiciona o 33o CC precisa saber no boot.
    void AddMidi(uint8_t cc, int param_id, uint8_t canal = kCanalEntradaA)
    {
        if(n_midi >= kMaxMidiMap) { midi_overflow++; return; }
        midi_map[n_midi].cc       = cc;
        midi_map[n_midi].canal    = canal;
        midi_map[n_midi].param_id = param_id;
        n_midi++;
    }
    int        n_midi = 0;

    // Aplica um CC pela via do knob. Devolve o id do parâmetro movido,
    // ou -1 se o CC não está no mapa.
    //
    // Mora aqui, e não no main.cpp, porque o mapa é do manifest —
    // e porque assim o banco de teste nativo (test_midi.cpp) exercita
    // ESTA função, não uma cópia dela. Lógica de mapeamento duplicada
    // entre firmware e teste é teste que valida a si mesmo.
    // Só CONSULTA o mapa, sem aplicar. O scope precisa disso pra
    // decodificar o CC em nome de parâmetro sem provocar efeito
    // colateral — instrumento de medição que altera o medido é a
    // família inteira do catálogo forense.
    int FindMidiParam(uint8_t canal, uint8_t cc) const
    {
        for(int i = 0; i < n_midi; i++)
            if(midi_map[i].cc == cc && midi_map[i].canal == canal)
                return midi_map[i].param_id;
        return -1;
    }

    // O CANAL E PARTE DO ENDERECO, e por isso e argumento e nao default:
    // uma sobrecarga sem canal deixaria qualquer chamador antigo mirar
    // silenciosamente na cadeia A. O compilador tem que cobrar os dois
    // valores em cada chamada — o custo de errar aqui e o CC do bumbo
    // mexendo no efeito da corda, que ninguem depura de ouvido.
    int ApplyMidiCc(ParamTable& t, uint8_t canal, uint8_t cc, uint8_t value) const
    {
        for(int i = 0; i < n_midi; i++)
        {
            if(midi_map[i].cc != cc || midi_map[i].canal != canal)
                continue;
            // Set() fala NORMALIZADO — o mesmo ponto de entrada do
            // knob físico. Curva, Shaped() e Real() vêm depois, iguais
            // pra qualquer origem.
            t.Set(midi_map[i].param_id, MidiNorm(value));
            return midi_map[i].param_id; // primeiro vence; duplicata é erro
        }
        return -1;
    }

    Macro knob[kNumKnobs];

    void Init(ParamTable& t)
    {
        // ============================================================
        // DEFAULTS: ZERO PARA O QUE O MIDI ALCANCA (dB, 18/08)
        //
        // O Loopy so TRANSMITE — nao recebe sem MIDI learn, e o dB
        // decidiu nao fazer isso por ora. Consequencia: o controlador e
        // a fonte da verdade, e os faders dele estao onde estao.
        //
        // Se a placa acordasse nos valores "aprovados", eles durariam
        // ate o primeiro CC e depois pulariam pro que o fader dissesse.
        // Isso ja aconteceu hoje varias vezes e foi lido como defeito:
        // `level` em 0.49 logo apos o boot, freeze ligado sozinho, o
        // grave "sumindo". Nao era defeito — era o controlador fazendo
        // o trabalho dele por cima do manifest.
        //
        // Entao o manifest para de disputar: params MAPEADOS nascem em
        // ZERO, e o dB constroi o instrumento subindo os faders. Nada
        // pula, porque nao ha divergencia pra corrigir.
        //
        // O QUE NAO E ZERADO, E POR QUE: params que o MIDI NAO alcanca.
        // Zera-los nao alinha nada (o Loopy nunca os manda) e alguns
        // quebram o instrumento — `gov_ceiling` em 0 faz o governor
        // comprimir tudo contra o chao, que foi a causa medida do
        // "sumiu o baixo" desta sessao. Roteamento em 0 e saida muda.
        //
        // A regra, entao: ZERA O QUE O CONTROLADOR ALCANCA, PRESERVA O
        // QUE ELE NAO ALCANCA. E a mesma fronteira caráter/segurança
        // que ja governa a whitelist dos buzios e o mapa MIDI.
        // ============================================================
        // nome, default (0-1), min real, max real
        // ESTADO INSTRUMENTO restaurado 2026-08-17. Estava em 0.4 desde
        // o A/B com o Max e ficou — o dB ouviu o instrumento real e
        // disse "muuuuito baixo". Era resíduo de bancada versionado por
        // descuido, exatamente o que a regra da casa proíbe.
        //
        // FUNDO DE ESCALA (1.0), por decisão do dB: "pode ir pra 1 se
        // precisar diminuo na placa de som". Faz sentido pra ESTA
        // escultura — a saída é jack de linha, e quem tem o atenuador
        // na mão é quem mixa.
        //
        // >>> MAS ISSO NÃO É REGRA GERAL, e o dia em que o EXCITER
        // >>> entrar no caminho a conta muda: lá o atenuador não existe,
        // >>> o TPA3116 recebe o que a placa mandar, e level 1.0 com
        // >>> governor em 0.85 deixa o limiter como única rede. Quando
        // >>> a escultura tocar pelo exciter, revisar este número junto
        // >>> com o gov_ceiling — os dois formam o par que decide onde
        // >>> o laço para de crescer.
        //
        // O contrato [0,1] segue intacto: 1.0 é o topo do curso do knob,
        // não uma exceção. Quem precisar de mais que isso está pedindo
        // ganho de ENTRADA, que mora pós-entrada/pré-gate — nunca aqui.
        p_level     = t.Add("level",     0.0f, 0.f,    1.f);   // MIDI: nasce 0
        p_tone_freq = t.Add("tone_freq", 0.4f, 80.f,   2000.f);
        p_tone_mix  = t.Add("tone_mix",  0.0f, 0.f,    1.f);
        // 0.5 NORMALIZADO = 0.0 real = centro. O primeiro argumento do
        // Add() e `def_norm`, e todo param ate hoje tinha min = 0, o
        // que fazia norm e real coincidirem — a distincao nunca doeu e
        // por isso o habito passou batido. Este e o primeiro param de
        // faixa SIMETRICA: aqui norm 0.0 e o extremo esquerdo, e
        // nascia com um piezo mudo.
        p_piezo_mix = t.Add("piezo_mix", 0.5f, -1.f,   1.f);

        // Faixa [1, 8], curva 2 — resolução embaixo, onde o ajuste fino
        // acontece. Em 8.0, o TOPO da faixa, por pedido do dB depois de
        // 4x ter melhorado muito.
        //
        // ESTAR NO TETO É INFORMAÇÃO, não conforto: significa que o
        // instrumento pede mais do que este ganho oferece, e a próxima
        // pergunta deixa de ser "quanto" e passa a ser ONDE. Se 8x
        // ainda ficar curto, subir o teto daqui é o caminho errado —
        // o certo é o BUFFER DE ALTA IMPEDÂNCIA (receita no BOARDS.md),
        // que levanta o sinal ANTES do ADC e melhora a relação
        // sinal/ruído em vez de só multiplicar os dois.
        //
        // Lembrete do que este ganho não faz: ele levanta o chão de
        // ruído do piezo junto. Em 8x o chão está 18 dB acima de onde
        // estava em 1x. Se o ruído aparecer, é ele falando — não é
        // defeito novo.
        //
        // norm para 8.0 = topo da faixa = 1.0
        p_in_gain[0] = t.Add("in_gain_a", 0.0f, 1.f, 8.f, 2.0f); // nasce 1x
        p_in_gain[1] = t.Add("in_gain_b", 0.0f, 1.f, 8.f, 2.0f);
        // os params do módulo já são 0–1 no export, então min/max reais
        // coincidem com o normalizado e a conversão é identidade.
        // curve 2.5 no dly_time: ERGONOMIA, não DSP. O parâmetro do
        // patch é linear em 0–1, e linear na mão significa que quase
        // todo delay curto se amontoa no primeiro décimo do knob. Com
        // 2.5, metade do curso cobre os primeiros ~18% da faixa, que é
        // onde moram os slapbacks e os flams. O RNBO recebe o valor já
        // curvado e não sabe que houve curva.
        // norm 0.69 com curve 2.5 da real ~0.40: eco audivel e
        // claramente separado, nem slapback nem cauda longa.
        // CURVA LINEAR (dB, 18/08). Era 2.5, que dava controle fino nos
        // tempos CURTOS — util pra delay-como-timbre e pessimo pra
        // delay-como-eco. O patch mapeia real 0..1 em 10..1000 ms
        // linearmente, entao com curva 2.5 o fader em 30% dava 59 ms:
        // abaixo de ~60 ms o ouvido nao separa em eco, vira
        // espessamento. O primeiro TERCO do curso era inaudivel como
        // delay, e foi exatamente o que o dB relatou.
        //
        // Linear: 10% = 109 ms, 25% = 258 ms, 50% = 505 ms, 100% = 1 s.
        // Curso inteiro util, e o fader passa a ler em MILISSEGUNDOS —
        // previsivel pra quem toca.
        // FAIXA CORTADA POR BAIXO (dB, 18/08): o fader inteiro passa a
        // viver na regiao PERCEPTIVEL. Antes ia de 10 ms a 1 s com
        // curva 2.5, e o primeiro terco do curso ficava abaixo de
        // ~60 ms — que o ouvido nao separa em eco, so espessa. Um
        // terco de fader que nao faz nada audivel e um terco perdido.
        //
        // Agora min_real = 0.05, que o patch mapeia em ~59 ms (a borda
        // da percepcao), e o topo segue em 1 s. Linear:
        //   0% = 59 ms | 25% = 294 ms | 50% = 530 ms | 100% = 1000 ms
        //
        // O corte e no PARAMETRO, nao na curva, e a diferenca importa:
        // curva redistribui o mesmo alcance e deixa o inutil no fim do
        // curso; limite REMOVE o inutil do alcance. As cercas dos
        // buzios (100-500 ms) continuam validas porque falam em real e
        // cabem dentro da faixa nova.
        // CURSO INTEIRO desde 20/08: a cerca de 50 ms mudou-se pra
        // dentro do `delay.h`, onde vale pra qualquer manifest que o
        // use. Aqui o param volta a ser 0..1 de ponta a ponta.
        p_dly_time  = t.Add("dly_time",  0.0f, 0.f, 1.f, 1.0f); // 50..1000 ms
        p_dly_mix[0]   = t.Add("dly_mix_a",   0.0f, 0.f,    1.f);  // MIDI: nasce 0
        p_dly_mix[1]   = t.Add("dly_mix_b",   0.0f, 0.f,    1.f);
        // CHAVE PROPRIA. Delay e reverb ligavam pelo `mix` — sao
        // anteriores a convencao de chave que o dB pediu pros efeitos
        // da suite. Fundir chave e mix obriga a zerar o ajuste pra
        // desligar, e o ajuste se perde.
        p_dly_on[0]    = t.Add("dly_on_a",    0.0f, 0.f,    1.f);
        p_dly_on[1]    = t.Add("dly_on_b",    0.0f, 0.f,    1.f);

        // GATE. gate_thresh usa curve 2 porque threshold útil vive
        // todo no fundo da faixa: linear, os primeiros 5% do curso
        // seriam a única parte usável. O default REAL é 0.02
        // (~-34 dBFS), e como Real() = pow(norm,2), o norm que
        // produz 0.02 é sqrt(0.02) = 0.1414.
        // Gate de volta: o chão de ruído do piezo vira cauda no delay se
        // ninguém o cortar. 0.02 é o valor que estava antes do A/B.
        p_gate_thresh[0]  = t.Add("gate_thresh_a",  0.0f, 0.f, 1.f, 2.0f);
        p_gate_thresh[1]  = t.Add("gate_thresh_b",  0.0f, 0.f, 1.f, 2.0f);
        // release em ms: 50–2000, default 300
        p_gate_release[0] = t.Add("gate_release_a", 0.1282f, 50.f, 2000.f);
        p_gate_release[1] = t.Add("gate_release_b", 0.1282f, 50.f, 2000.f);

        // GOVERNOR. gov_ceiling é o futuro destino do ENCODER físico:
        // é a "borda de feedback", o ponto onde a escultura para de
        // crescer. Linear de propósito — a borda é procurada pelo
        // ouvido, e curva atrapalharia o gesto fino.
        // Governor em 0.85, não nos 0.5 de antes: com o level em 0.85 um
        // teto de 0.5 comprimiria o instrumento o tempo todo. Ele volta
        // como PROTEÇÃO (segura o crescimento do laço), não como cor.
        p_gov_ceiling[0] = t.Add("gov_ceiling_a", 0.85f,    0.f,  1.f);
        p_gov_ceiling[1] = t.Add("gov_ceiling_b", 0.85f,    0.f,  1.f);
        // release em ms: 100–3000, default 800
        p_gov_release[0]  = t.Add("gov_release_a",  0.2414f, 100.f, 3000.f);
        p_gov_release[1]  = t.Add("gov_release_b",  0.2414f, 100.f, 3000.f);

        // ROTEAMENTO. Os dois ligados no boot: mudo por engano é pior
        // que alto por engano — pra alto o limiter já tem resposta.
        p_exc_on[0]     = t.Add("exc_on_a",      1.0f, 0.f, 1.f);
        p_exc_on[1]     = t.Add("exc_on_b",      1.0f, 0.f, 1.f);
        // A matriz nasce CHEIA, e isso e deliberado: cheia ela
        // reproduz exatamente o codigo fixo que a antecedeu,
        // `out[0] = out[1] = A + B`. Feature nova que nasce mudando o
        // som e feature que se descobre pelo susto.
        p_route_l[0]    = t.Add("route_l_a",     1.0f, 0.f, 1.f);
        p_route_l[1]    = t.Add("route_l_b",     1.0f, 0.f, 1.f);
        p_route_r[0]    = t.Add("route_r_a",     1.0f, 0.f, 1.f);
        p_route_r[1]    = t.Add("route_r_b",     1.0f, 0.f, 1.f);
        p_out_l         = t.Add("out_l",         1.0f, 0.f, 1.f);
        p_out_r         = t.Add("out_r",         1.0f, 0.f, 1.f);

        // FERRAMENTA DE BANCADA — ver bloco acima. def_norm 0.0 com
        // faixa [1,8] e curve 1 dá real = 1.0 no boot, sempre.
        p_bench_boost[0]   = t.Add("bench_boost_a",   0.0f, 1.f, 8.f);
        p_bench_boost[1]   = t.Add("bench_boost_b",   0.0f, 1.f, 8.f);

        // SHIFTER. Faixa 0–10 Hz, default 4 Hz (norm 0.4). Linear
        // pelo mesmo motivo do gov_ceiling: a borda se procura de
        // ouvido. Acima de ~10 Hz o deslocamento deixa de ser
        // proteção transparente e vira efeito — as harmônicas param
        // de ser múltiplas inteiras e o timbre metaliza.
        p_shift_hz      = t.Add("shift_hz",      0.0f, 0.f, 10.f);

        // REVERB. Os três 0..1: o reverb.h faz o mapeamento pras
        // grandezas internas (size → realimentação [0.7, 0.98], damp →
        // corte do passa-baixa da cauda).
        // 120 Hz, APROVADO DE OUVIDO em 2026-08-17 ("não estoura mais").
        //
        // O caminho até este número, porque ele parece arbitrário e não
        // é: o histograma de pico por nota mostrou a saída do Fx2
        // picando 1,4–1,5 nas portadoras graves contra 0,55 nas agudas.
        // Isso NÃO provava energia grave — o histograma agrupa por
        // pitch da portadora, não por conteúdo espectral, e o excesso
        // podia estar em qualquer lugar. O passa-alta foi ao mesmo
        // tempo hipótese e conserto: se achatasse, era grave mesmo.
        // Achatou.
        //
        // 120 Hz é escolha musical dentro do que a medição permitiu:
        // corta o sub que a portadora empilha, e fica ABAIXO da região
        // de 174–220 Hz que o histograma mostrou saudável (0,70–0,75).
        // Cortar mais alto levaria junto o corpo do som.
        //
        // NO CAMINHO DO FX2, não no master: sem vocoder não estoura
        // (medido), então filtrar o master castigaria o delay de graça.
        // Faixa 0–400, curva 2 — resolução embaixo, que é onde se
        // procura. 0 Hz desliga.

        // 0.35 — o valor que o dB tocou e aprovou ("reverb está ótimo").
        // 0.15 era o chute inicial e nunca foi ouvido.
        p_rev_mix[0]       = t.Add("rev_mix_a",       0.0f, 0.f, 1.f);  // MIDI: nasce 0
        p_rev_mix[1]       = t.Add("rev_mix_b",       0.0f, 0.f, 1.f);
        // DESLIGAR O REVERB EMUDECE O SHIMMER, que nao tem reverb
        // proprio — ele realimenta ESTE. Dependencia real, declarada em
        // vez de deixar o dB cacando um efeito que "parou de funcionar".
        p_rev_on[0]        = t.Add("rev_on_a",        0.0f, 0.f, 1.f);
        p_rev_on[1]        = t.Add("rev_on_b",        0.0f, 0.f, 1.f);
        p_rev_size[0]      = t.Add("rev_size_a",      0.50f, 0.f, 1.f);
        p_rev_size[1]      = t.Add("rev_size_b",      0.50f, 0.f, 1.f);
        p_rev_damp[0]      = t.Add("rev_damp_a",      0.50f, 0.f, 1.f);
        p_rev_damp[1]      = t.Add("rev_damp_b",      0.50f, 0.f, 1.f);

        // ---- FREEZE ----
        // Nasce DESLIGADO. Congelar é gesto, e gesto não tem estado
        // default ligado: a placa subindo já congelada seria a
        // escultura tocando sozinha um som que ninguém pediu.
        // O GESTO, separado do envio: `frz_on_a/b` dizem QUAL cadeia
        // alimenta o freeze; este diz QUANDO congelar.
        p_frz_on[0]    = t.Add("frz_on_a",  0.0f, 0.f, 1.f);
        p_frz_on[1]    = t.Add("frz_on_b",  0.0f, 0.f, 1.f);
        p_frz_vol[0]   = t.Add("frz_vol_a", 0.0f, 0.f, 1.f);
        p_frz_vol[1]   = t.Add("frz_vol_b", 0.0f, 0.f, 1.f);
        // Mix em 1.0: congelou, é o congelado que se ouve. Meio a meio
        // deixaria o seco competindo com a sustentação e o gesto
        // perderia clareza — quem quiser camada abaixa no CC.
        // 100 ms de janela. Abaixo de ~40 ms o grão vira timbre
        // próprio (soa granular, "chuvisco"); acima de ~250 ms os dois
        // grãos leem quase o mesmo trecho e o batimento volta.
        // CUIDADO: o primeiro argumento e NORMALIZADO, nao o valor
        // real. Escrever 100.0f aqui satura em 1.0 e o grao nasce em
        // 400 ms — exatamente o tamanho em que os dois graos leem quase
        // o mesmo trecho e o batimento volta. 100 ms = (100-20)/380.
        // FAIXA CORTADA EM 165 ms (20/08). Ia ate 400, e o `SetSize` limita o
        // grao a um TERCO DO NUCLEO — que tem 500 ms, entao o teto real
        // sempre foi ~167. Sessenta e um por cento do curso do fader nao
        // fazia nada, e o dB percebeu tentando mexer no `frz_size_b`.
        //
        // Mesma licao do `dly_time` em 18/08: LIMITE remove o inutil do
        // alcance, CURVA so o redistribui. E vale pros dois canais — o
        // defeito nunca foi do B.
        // O 165 NAO E ARBITRARIO E NAO PODE DESSINCRONIZAR EM SILENCIO.
        //
        // O `Freeze` grampeia o tamanho do grao em `NucleusLen()/3`, e
        // `NucleusLen()` vale `kNucleusMs` (500 ms) convertido pra
        // amostras. Logo o teto real e 500/3 = 166,7 ms, e declarar
        // 165 poe a faixa inteira dentro do curso util com 1,7 ms de
        // margem.
        //
        // Ate 20/08 a faixa era 20..400 ms contra o mesmo clamp: 61%
        // do curso do fader nao fazia nada (bts 12i). Corrigido em
        // d80e45d — e o item ficou meses na FILA descrevendo o estado
        // pre-conserto, que e a deriva de 22/08 num lugar que o
        // `check-docs` nao olha.
        //
        // SE `kNucleusMs` MUDAR, este numero muda junto: novo teto =
        // kNucleusMs/3, menos uma margem. O literal esta aqui porque
        // derivar exigiria o manifest incluir `freeze.h`, e essa
        // dependencia custa mais que este comentario.
        p_frz_size[0]      = t.Add("frz_size_a",      0.0f, 20.f, 165.f); // MIDI: nasce 0 (= 20 ms)
        p_frz_size[1]      = t.Add("frz_size_b",      0.0f, 20.f, 165.f); // MIDI: nasce 0 (= 20 ms)

        // ---- DRIVE ----
        p_drv_on[0]    = t.Add("drv_on_a",    0.0f, 0.f, 1.f);
        p_drv_on[1]    = t.Add("drv_on_b",    0.0f, 0.f, 1.f);
        // FAIXA CORTADA POR BAIXO, mesma regua do dly_time: 1.5x ja e
        // cola audivel, e abaixo disso o waveshaper nao faz nada que o
        // ouvido perceba. Curso inteiro util, agressivo so no fim.
        //   0% = 1.5x | 25% = 4.1x | 50% = 6.8x | 100% = 12x
        // Passa-baixa pos-saturacao. No topo e transparente de
        // proposito: "tone no maximo" tem que significar SEM filtro.
        p_drv_mix[0]   = t.Add("drv_mix_a",   0.0f, 0.f, 1.f);
        p_drv_mix[1]   = t.Add("drv_mix_b",   0.0f, 0.f, 1.f);
        p_drv_drive[0] = t.Add("drv_drive_a", 0.0f, 1.5f, 48.f);
        p_drv_drive[1] = t.Add("drv_drive_b", 0.0f, 1.5f, 48.f);
        p_drv_tone[0] = t.Add("drv_tone_a", 0.0f, 800.f, 16000.f, 2.0f);
        p_drv_tone[1] = t.Add("drv_tone_b", 0.0f, 800.f, 16000.f, 2.0f);
        // NASCE EM 0 = o tube aprovado de ouvido em 22/08. Subir
        // daqui e ir pro dobrador, que e o "bem mais suja" pedido.
        p_drv_sujeira[0] = t.Add("drv_sujeira_a", 0.0f, 0.f, 1.f);
        p_drv_sujeira[1] = t.Add("drv_sujeira_b", 0.0f, 0.f, 1.f);

        // ---- PHASER ----
        p_phs_on[0]       = t.Add("phs_on_a",       0.0f, 0.f, 1.f);
        p_phs_on[1]       = t.Add("phs_on_b",       0.0f, 0.f, 1.f);
        // 0.02 a 4 Hz. Abaixo de 0.02 o movimento some; acima de ~4 ja
        // e vibrato de fase, nao phaser.
        // 3 posicoes: 4, 6 ou 8 estagios (2, 3 ou 4 notches).
        p_phs_mix[0]      = t.Add("phs_mix_a",      0.0f, 0.f, 1.f);
        p_phs_mix[1]      = t.Add("phs_mix_b",      0.0f, 0.f, 1.f);
        p_phs_rate[0] = t.Add("phs_rate_a", 0.0f, 0.02f, 4.f, 2.0f);
        p_phs_rate[1] = t.Add("phs_rate_b", 0.0f, 0.02f, 4.f, 2.0f);
        p_phs_depth[0] = t.Add("phs_depth_a", 0.0f, 0.f, 1.f);
        p_phs_depth[1] = t.Add("phs_depth_b", 0.0f, 0.f, 1.f);
        p_phs_feedback[0] = t.Add("phs_fb_a", 0.0f, 0.f, 1.f);
        p_phs_feedback[1] = t.Add("phs_fb_b", 0.0f, 0.f, 1.f);
        p_phs_stages[0] = t.Add("phs_stages_a", 0.0f, 4.f, 8.f, 1.0f, 3);
        p_phs_stages[1] = t.Add("phs_stages_b", 0.0f, 4.f, 8.f, 1.0f, 3);

        // ---- WARBLE ----
        p_wrb_on[0]    = t.Add("wrb_on_a",    0.0f, 0.f, 1.f);
        p_wrb_on[1]    = t.Add("wrb_on_b",    0.0f, 0.f, 1.f);
        p_wrb_mix[0]   = t.Add("wrb_mix_a",   0.0f, 0.f, 1.f);
        p_wrb_mix[1]   = t.Add("wrb_mix_b",   0.0f, 0.f, 1.f);
        p_wrb_rate[0] = t.Add("wrb_rate_a", 0.0f, 0.05f, 3.f, 2.0f);
        p_wrb_rate[1] = t.Add("wrb_rate_b", 0.0f, 0.05f, 3.f, 2.0f);
        p_wrb_depth[0] = t.Add("wrb_depth_a", 0.0f, 0.f, 3.f);
        p_wrb_depth[1] = t.Add("wrb_depth_b", 0.0f, 0.f, 3.f);
        p_wrb_irreg[0] = t.Add("wrb_irreg_a", 0.0f, 0.f, 1.f);
        p_wrb_irreg[1] = t.Add("wrb_irreg_b", 0.0f, 0.f, 1.f);
        // Ate 3 ms. O desvio de altura e ~2*pi*rate*depth: 1 ms a
        // 0,6 Hz da ~4 cents (fita boa); 3 ms a 2 Hz da ~65 cents
        // (fita estragada — as vezes e o que se quer).

        // TOM DE TESTE: destino. 0=A 1=B 2=AMBAS (default).
        // ORDEM DO PAINEL (28/08): 0=A+B, 1=so A, 2=so B.
        // Nasce em ZERO como todos os outros — e zero e o estado
        // normal (as duas cadeias), nao um modo de diagnostico. A
        // ordem anterior (0=A, 1=B, 2=ambas) obrigava o default a
        // ser 2 e fazia do estado comum uma excecao a regra da casa.
        p_tone_dest = t.Add("tone_dest", 0.0f, 0.f, 2.f, 1.0f, 3);
        // VIA DO TOM DE TESTE (28/08). Duas ferramentas diferentes com o
        // mesmo oscilador, e o switch diz qual:
        //   0 = DIRETO — entra no estagio de saida, DEPOIS do `level`.
        //       Referencia limpa: independente do nivel do utilizador e
        //       do processamento. E o que serve para calibrar e para o
        //       discriminante do codec.
        //   1 = CADEIA — entra dentro das cadeias, como sempre entrou.
        //       Atravessa drive, phaser, reverb e o `level`. E o que
        //       serve para excitar o processamento com sinal conhecido.
        // Nasce em ZERO (direto): o uso comum e medir, e medicao que
        // depende do que o musico deixou no fader nao e medicao.
        p_test_tone_stage = t.Add("test_tone_stage", 0.0f, 0.f, 1.f, 1.0f, 2);

        // ---- DESCRITOR POR EFEITO DE CARATER ----
        for(int c = 0; c < kCadeias; c++)
        {
            // O PRIMARIO agora e COMPARTILHADO; so a chave e por canal.
            caracter[c][0] = {p_drv_on[c], p_drv_drive[c], "drive", "drive"};
            caracter[c][1] = {p_phs_on[c], p_phs_rate[c],  "phaser", "rate"};
            caracter[c][2] = {p_wrb_on[c], p_wrb_depth[c], "warble", "depth"};
        }

        // ---- SHIMMER ----
        // Tudo em zero: o dB constroi subindo os faders (ver a nota dos
        // defaults no topo desta funcao).
        p_shm_on[0]       = t.Add("shm_on_a",       0.0f, 0.f, 1.f);
        p_shm_on[1]       = t.Add("shm_on_b",       0.0f, 0.f, 1.f);
        p_shm_mix[0]      = t.Add("shm_mix_a",      0.0f, 0.f, 1.f);
        p_shm_mix[1]      = t.Add("shm_mix_b",      0.0f, 0.f, 1.f);
        // Chave de 3 posicoes. steps=3 faz o CC cair em terços limpos.
        p_shm_shift[0]    = t.Add("shm_shift_a",    0.0f, 0.f, 2.f, 1.0f, 3);
        p_shm_shift[1]    = t.Add("shm_shift_b",    0.0f, 0.f, 2.f, 1.0f, 3);
        // O teto de 0.85 mora no shimmer.h, nao aqui: e limite de
        // ESTABILIDADE do laco, nao preferencia musical. Param vai a
        // 1.0 e o efeito escala pra dentro do seguro.
        p_shm_feedback[0] = t.Add("shm_feedback_a", 0.0f, 0.f, 1.f);
        p_shm_feedback[1] = t.Add("shm_feedback_b", 0.0f, 0.f, 1.f);
        // TOM: sobe pra clarear. 0 e o estado BOM, nao o mais brilhante
        // — a regra do default zero desliga efeito, nao escolhe o pior
        // timbre. Medido: 0 poe a cauda em centroide 3189 Hz, 1 em 5507.
        p_shm_tone[0]     = t.Add("shm_tone_a",     0.0f, 0.f, 1.f);
        p_shm_tone[1]     = t.Add("shm_tone_b",     0.0f, 0.f, 1.f);

        // tone_freq e tone_mix ficaram SEM knob: os três primeiros
        // foram pro delay e pro level. Seguem alcançáveis pelo serial
        // (`tone on`/`tone off`) e pelo default — o tom de teste é
        // bancada, não performance.

        // >>> HISTORICO (a excecao temporaria de 16/08 virou regra) <<<
        //
        // O Fx2 nasce LIGADO aqui, contra a regra do bloco de
        // declaracao. Nao e descuido: a serial esta caindo a cada 2-3
        // comandos, entao configurar por serial deixou de ser viavel e
        // o unico jeito de ouvir o vocoder e o boot ja subir no estado
        // certo.
        //
        // A regra do bypass existe pra proteger de modulo que estoura o
        // prazo e mata o loop. Esse risco esta MEDIDO e afastado: 63% de
        // 5333 us, watchdog intacto, quatro voos sem overrun. Abrir
        // excecao com a medicao na mao e diferente de abrir por
        // conveniencia.
        //
        // REVERTIDO no mesmo dia. Subir ligado resolvia ouvir sem
        // serial, mas criava um beco: com o Fx2 ativo desde o boot a
        // serial morre em segundos, e a caixa-preta so pode ser LIDA
        // com ele desligado. A ferramenta de medicao ficava inacessivel
        // justamente pela mudanca feita pra poder medir.
        //
        // Os outros defaults de escuta FICAM (dly_mix 0, voc_tone 0,
        // voc_mix 1, fx2_gain 16x), entao voar custa UM comando:
        //   set fx2_on 1
        // Um comando cabe no orcamento da serial; seis nao cabiam.
        // >>> NASCE LIGADO a partir de 2026-08-17. <<<
        //
        // A regra do bypass existia porque modulo pesado que estoura o
        // prazo mata o loop e a placa fica surda. Esse risco foi MEDIDO
        // e afastado: vocoder em 37% do prazo, watchdog intacto, e a
        // morte do loop que se culpava tinha outra causa (DelayMs, ver
        // battle_scars cap. 7) — ja consertada.
        //
        // O que fica no lugar da regra e melhor que ela: o WATCHDOG,
        // que forca bypass se o modulo estourar de verdade, e a janela
        // de graca, que perdoa o custo de estreia. Protecao por
        // medicao, nao por abstinencia.
        //
        // E o atrito era real: com fx2_on em 0, cada RESET devolvia a
        // escultura ao delay seco, e o dB encontrou "so o delay" seis
        // vezes num dia so. Instrumento que precisa de comando pra
        // tocar nao esta pronto.
        //
        // >>> PRA VOLTAR AO BYPASS: 0.0f aqui. Se um modulo novo e nao
        // >>> medido entrar no slot Fx2, VOLTE — a regra continua certa
        // >>> pra quem ainda nao passou pelo IFA10000.

        // FREEZE. Defaults iguais aos do export, pra o boot nascer
        // com o patch no estado que o autor definiu.
        // Defaults iguais aos do export: mix 1 (molhado), pitch 0.5
        // (centro, deslocamento zero), tone 0.7.
        // steps=49: o caminho MANUAL (serial/MIDI/knob) anda em grade
        // cromática — 48 semitons + centro. A ESCALA vale só pro
        // sorteio; a mão tem o cromático inteiro.

        // Faixa [1,256], curva 3. O teto subiu de 16 porque 6x ficou
        // "muuuuuito baixo" e 16 nao teria dado conta: cross-synthesis
        // devolve energia so onde as duas fontes coincidem, e a queda
        // pode passar de 40 dB. Curva 3 mantem o ajuste fino embaixo —
        // metade do curso ainda cobre 1x-32x.
        //
        // Default 1x — UNIDADE — e desta vez o numero e MEDIDO.
        //
        // A caixa-preta (voo de 16440 blocos, 2026-08-16): entrada
        // pico 0.215, saida crua do vocoder pico 2.359. A saida e 11x
        // MAIS QUENTE que a entrada. Com o ganho em 64x o pico virava
        // ~151 em fundo de escala 1.0 e o limiter esmagava tudo com
        // ~44 dB de reducao — o vocoder soava "baixo" porque estava
        // sendo pisoteado, nao porque era fraco.
        //
        // A escalada 8->48->64 foi toda na direcao errada, calibrada
        // por ouvido em cima de um sinal ja esmagado. Licao: o dB
        // calibrou o patch NO MAX e la soa certo sem ganho externo —
        // o trabalho do host e entregar UNIDADE e deixar o trim fino
        // pro CC 25. Nao compensar fantasma.
        //
        // norm = ((1-1)/255)^(1/3) = 0.0
        //
        // (historico) Default 64x, e desta vez o numero tem conta atras dele.
        //
        // Em 16x o dB ouviu ~5% do nivel do delay. O dB entao subiu o
        // ganho DENTRO do patch de 0.02 pra 0.1 — 5x — que e o lugar
        // certo pra isso: ganho no host amplifica junto o piso de ruido
        // do caminho, ganho no patch nao. Com 5x a mais, 16x passa a
        // dar ~25%, e faltam 4x pra paridade. 16 x 4 = 64.
        //
        // A base ainda e uma estimativa AUDITIVA (os "5%"), e ouvido
        // comprime. Serve pra chegar perto; quem fecha e a caixa-preta.
        //
        // norm = ((64-1)/255)^(1/3) = 0.616
        //
        // (historico) Default 8x, PROVISORIO, e baixou de 48x de proposito.
        //
        // Os 48x foram calibrados contra o pior caso possivel — tom
        // senoidal puro contra portadora de tres parciais, em que os
        // bins quase nunca coincidem e a saida beira zero. Calibrar
        // ganho no caso mais esparso que existe garante estouro no
        // caso real: com piezo (banda larga) as coincidencias sao
        // muitas, a saida sobe, e 48x estoura no primeiro tap.
        // Governor e limiter seguram, mas segurar nao e o mesmo que
        // estar certo — o instrumento passaria a viver comprimido.
        //
        // A calibracao de verdade e com a caixa-preta no caso BANDA
        // LARGA: voc_tone 0 (portadora de ruido) ou o piezo tocando.
        // Ver T12 no TESTES.md. Ate la, 8x e um chute conservador —
        // erra pra baixo, que e o lado seguro de errar.
        //
        // norm = ((8-1)/255)^(1/3) = 0.301

        // BUZIOS. thresh em norm==real (faixa 0..1). Cooldown em
        // ms, default 100: norm = (100-10)/990 = 0.091.
        // NASCE LIGADO. Búzios não podem travar nada: sorteiam dentro de
        // cercas, pela mesma via dos knobs, e a rampa é função do
        // relógio. O risco é zero e o atrito de não estar ligado é
        // real — o dB encontrou "só o delay" seis vezes num dia.
        p_buzios_on       = t.Add("buzios_on",       0.0f,   0.f,  1.f); // MIDI: nasce 0
        // 0.04, bem mais sensível que os 0.15 iniciais — pedido do dB
        // depois de tocar o instrumento real.
        //
        // O PISO NÃO É ZERO, e é o gate que o define: o detector lê o
        // sinal PÓS-GATE, e com `gate_thresh` em 0.02 tudo abaixo disso
        // já foi silenciado. Um limiar em 0.02 dispararia no primeiro
        // resíduo que o gate deixasse passar — búzios jogando sozinhos,
        // sem ninguém bater. 0.04 dá o dobro do piso de margem.
        //
        // Se precisar de MAIS sensibilidade que isso, o caminho não é
        // baixar mais este número: é baixar o `gate_thresh` junto, e aí
        // a conversa passa a ser sobre o chão de ruído do piezo.
        p_buzios_thresh   = t.Add("buzios_thresh",   0.04f,  0.f,  1.f);
        p_buzios_cooldown = t.Add("buzios_cooldown", 0.091f, 10.f, 1000.f);

        // ---- LIGACOES ----
        // TODAS depois de TODOS os Add(). Nao e estilo: um bind feito
        // antes do Add correspondente le o id AINDA ZERO, e id 0 e o
        // `level` — foi exatamente o que aconteceu, e o knob de volume
        // passou a dirigir os quatro parametros do freeze em silencio,
        // porque -1 nao aparece e 0 e um id perfeitamente valido.
        //
        // O `mods` mostra o nome do parametro de origem justamente pra
        // isso saltar aos olhos: quatro linhas dizendo "level ->" e o
        // sintoma.
        // O DELAY DEIXOU DE SER MODULO em 20/08 — virou C++ no shell
        // (delay.h). Nao ha mais bind: o callback le os params direto,
        // como todos os outros efeitos da casa.

        // ---- PAINEL v2: knob físico → macro → parâmetro(s) ----
        // AQUI EMBAIXO, junto dos bind[], e pelo MESMO motivo: knob
        // ligado antes do Add() correspondente lê id AINDA ZERO, e id 0
        // é o `level`. Este bloco morava lá em cima e só não quebrava
        // por acaso — k1–k4 apontam pra params declarados cedo. O k5
        // apontando pro vocoder foi o primeiro a cruzar a linha.
        //
        // REGRA DO PROJETO, não sugestão: k1 e k2 são FIXOS em TODA
        // escultura — k1 volume, k2 fonte. Só k3–k5 são livres, e é só
        // neles que um manifest novo mexe.
        //
        // O motivo é a mão, não o código: painel com gramática constante
        // se aprende uma vez e vale pra qualquer peça. Quem chega perto
        // de uma escultura desconhecida já sabe onde abaixar o volume e
        // onde trocar a fonte — sem manual, sem tentativa e erro em
        // público. Mover k1/k2 num manifest quebra isso pra todas as
        // outras esculturas, não só pra esta.
        knob[0].AddTarget(p_level, 1.0f);    // k1 — volume master
        knob[1].AddTarget(p_piezo_mix, 1.0f); // k2 — BALANCO dos piezos
        knob[2].AddTarget(p_dly_time, 1.0f); // k3 — macro do delay
        knob[3].AddTarget(p_dly_mix[0], 1.0f); // k4 — envio da cadeia A

        // ---- B1: o que o randomizador pode sortear ----
        // Params de CARÁTER dos módulos, e só. Com o vocoder ligado
        // entram os dois dele que mexem em timbre.
        //
        // `voc_mix` fica DE FORA: com o Fx2 no caminho, mix não é
        // caráter — é quanto do módulo existe. Sortear isso é da mesma
        // família de `src_mix`, que decide qual instrumento se está
        // tocando, e não como ele soa.
        // Cercas desta escultura.
        //
        // dly_time: 100 a 500 ms POR HIT, uniforme em MILISSEGUNDOS.
        // O patch mapeia o param 0..1 linearmente em 10..1000 ms
        // (`scale 0 1 10 1000` no gerado), então 100 ms = real 0.0909
        // e 500 ms = real 0.4949. real_fence porque dly_time tem curva
        // 2.5 — sorteio uniforme em norm concentraria nos tempos
        // curtos e "100 a 500" viraria mentira estatística.
        // Degraus de 50 ms: 100, 150, ..., 500 — nove valores. O
        // contínuo sorteava vizinhos (ex.: 210 e 240 ms) que o ouvido
        // lê como "o mesmo eco". 50 ms = 0.0505 na escala do patch.
        randomizable[n_randomizable++] = {p_dly_time, 0.0909f, 0.4949f,
                                          nullptr, 0, 0.5f, 0.f,
                                          true, 0.0505f};
        randomizable[n_randomizable++] = {p_dly_mix[0], 0.30f, 0.80f};
        // voc_pitch: UMA NOTA NOVA POR HIT, repetição aceita (sorteio
        // uniforme, sem memória). ±12 st ao redor do centro (110 Hz =
        // Lá), quantizado em Lá menor — nunca cromático fora da
        // escala. 1 st = 1/48 do curso (a faixa inteira é ±24 st).
        // Centro em 220 Hz (norm 0.75 — também um Lá), cerca ±12 st =
        // 110..440 Hz. O registro anterior (centro 110 Hz) sorteava
        // certo mas soava tudo GRAVE e parecido — o registro aprovado
        // de ouvido foi o de cima.
        // >>> voc_pitch e voc_tone SAIRAM da whitelist em 18/08 porque o
        // >>> vocoder saiu do build. Nao e decisao de musica, e
        // >>> aritmetica: param de modulo ausente nao tem o que
        // >>> sortear. Voltam junto com o modulo, e a cerca de La menor
        // >>> abaixo continua valendo pra quando isso acontecer:
        //   {p_voc_pitch, 0.50f, 1.00f, kScaleAminor, 7, 0.75f, 1.f/48.f}
        //   {p_voc_tone,  0.40f, 1.00f}
        //
        // CONSEQUENCIA A OLHO NU: sobram dly_time e dly_mix. Os buzios
        // ficaram com DUAS cartas em vez de quatro, e o sorteio soa
        // proporcionalmente mais pobre. Nao ha conserto silencioso pra
        // isso — ou volta um modulo de carater, ou a decisao e aceitar
        // o sorteio menor.
        // >>> O REVERB FICA FORA DOS BÚZIOS. Decisão do dB, e é de
        // >>> música, não de custo — chegou a ser sorteado e funcionava.
        //
        // O papel dele é UNIFORMIZAR: dar liga entre tanta variação.
        // Com delay, pitch e timbre saltando a cada batida, a sala
        // constante é o que faz as jogadas pertencerem ao MESMO LUGAR.
        // Sortear a sala junto tiraria o único referencial fixo, e o
        // resultado deixa de soar como um instrumento sendo tocado pra
        // soar como uma sequência de instrumentos diferentes.
        //
        // Dito de outro jeito: se tudo salta, nada é surpresa. O
        // contraste precisa de um fundo parado pra existir.
        //
        // Corolário de projeto, e vale pra próxima escultura: um bloco
        // permanente que serve de MOLDURA não é candidato a sorteio,
        // por mais tentador que seja. A whitelist dos búzios é pra
        // caráter do módulo, e moldura não é caráter — é contexto.

        // >>> O FREEZE TAMBÉM FICA FORA DOS BÚZIOS. Decisão do dB, e é
        // >>> de música — reversível se ele mudar de ideia.
        //
        // A razão é outra que a do reverb, e vale distinguir: o reverb
        // fica fora por ser MOLDURA. O freeze fica fora por ser GESTO.
        // Congelar é uma decisão de execução, com hora certa — é o
        // intérprete dizendo "este som, agora, para". Sorteado, deixa
        // de ser decisão e vira acidente, e o que se perde é
        // justamente o que faz o gesto significar alguma coisa.
        //
        // Terceira categoria de exclusão, então, e o manifest agora
        // carrega as três: MOLDURA (reverb), SEGURANÇA (level,
        // src_mix, gate, governor, limiter, shift) e GESTO (freeze).
        // Búzios sorteiam CARÁTER — e só.

        // ---- B2: CONGELA ENQUANTO SEGURADO ----
        // O exemplo virou o caso real. Segurar congela, soltar libera:
        // é o gesto natural do freeze, e o motivo de Hold ser o modo
        // default desde que o campo existe.
        //
        // O painel v2 ainda não está soldado — até lá o alvo vale pelo
        // `b2` do painel falso via serial e pelo CC 26.
        // B2 = freeze A (switch D24, LED D27) — indices no HAL.
        // B3 = rearme do CTOS, sem alvo de param (age no CTOS direto).
        // B4 = freeze B (switch D22, LED D23).
        AddBotao(1, p_frz_on[0], 0, "freeze A");
        AddBotao(3, p_frz_on[1], 1, "freeze B");
        button2_mode   = ButtonMode::Hold;

        // ---- SLOT Fx2 VAZIO (2026-08-18) ----
        // O vocoder saiu do build por decisão de ouvido do dB. Slot
        // vazio é passthrough silencioso, não erro — e os binds abaixo
        // ficam prontos pro dia da volta, conferidos contra o
        // description.json do export de 16/08:
        //   bind[n_bind++] = {ModuleSlot::Fx2, p_voc_mix,   "voc_mix"};
        //   bind[n_bind++] = {ModuleSlot::Fx2, p_voc_pitch, "voc_pitch"};
        //   bind[n_bind++] = {ModuleSlot::Fx2, p_voc_tone,  "voc_tone"};
        //
        // Os params p_voc_* CONTINUAM na tabela de propósito: mexer
        // neles com o módulo fora não faz nada, e é exatamente esse
        // silêncio inofensivo que mantém o caminho de volta barato.

        // ---- MAPA MIDI ----
        // Aqui embaixo pelo MESMO motivo dos bind[]: um id lido antes
        // do Add() correspondente vale 0, e 0 é o `level`. Um mapa
        // montado cedo faria todos os CCs mexerem no volume.
        //
        // CCs 20–24: faixa "undefined" da spec MIDI, então não colide
        // com nada padrão (1 é mod wheel, 7 é volume, 64 é sustain).
        // Controlador genérico sai de fábrica sem usá-las.
        // ============================================================
        // MAPA v2 (dB, 18/08) — contiguo de 20, sem CCs mortos
        //
        // Renumerado porque o mapa antigo tinha buracos (23 e 24 sairam
        // por seguranca) e dois CCs MORTOS apontando pro slot RNBO
        // vazio: mexer neles nao fazia nada, em silencio. Tres params
        // que so a serial alcancava ganharam controle.
        //
        // AGRUPADO POR FAMILIA, e isso e defesa, nao estetica: um
        // widget mal configurado atinge o VIZINHO. Hoje um botao dos
        // buzios mandava 31 junto com 32 e jogava o `gate_thresh` no
        // topo — portao sempre fechado, instrumento mudo, sintoma
        // identico ao do `src_mix`. Vizinhanca importa.
        // ============================================================
        // UMA DECADA POR EFEITO. Todo param recebe CC (decisao do dB,
        // 20/08) — nada mais so-serial. O agrupamento por decada e
        // memorizavel e, mais importante, e DEFESA: um widget mal
        // configurado atinge o VIZINHO, e vizinho da mesma familia faz
        // menos estrago que vizinho de outra. Ja aconteceu: um botao
        // dos buzios mandava 31 junto com 32 e jogava o `gate_thresh`
        // no topo — portao fechado, instrumento mudo.

        // ---- ENTRADA: 20-26 ----
        // ============================================================
        // MAPA MULTIPLEXADO POR CANAL (v0.79.0)
        //
        //   ch1 (kCanalCompart)  -> COMPARTILHADOS, e so eles
        //   ch2 (kCanalEntradaA) -> cadeia A
        //   ch3 (kCanalEntradaB) -> cadeia B
        //
        // OS COMPARTILHADOS GANHARAM CANAL PROPRIO em v0.156.0. Ate ali
        // viajavam junto com a cadeia A, e isso quebrava a unica coisa
        // que a multiplexacao compra: a pagina da B ser COPIA da pagina
        // da A. Com `level` e `dly_time` dentro da pagina da A, duplicar
        // os trazia junto e eles tinham de ser apagados da copia — todas
        // as vezes. Separados, cada pagina tem UM assunto.
        //
        // O MESMO NUMERO DE CC nos dois canais. As paginas do Loopy
        // ficam identicas: copia a pagina da cadeia A, troca o campo de
        // canal, e a B esta endereçada. Foi por isso que os numeros da
        // cadeia A ficaram TODOS onde estavam — a pagina que ja existe
        // no iPad continua valendo sem retrabalho.
        //
        // Os numeros que a cadeia B ocupava (22, 24, 26, 32, 33, 42,
        // 43, 52, 53, 62, 63, 68, 69, 70, 80-88, 106, 116, 117, 118)
        // ficaram LIVRES. Nao reaproveitar sem pensar: eles ainda vivem
        // em paginas do Loopy, e um numero reaproveitado cedo demais
        // vira widget velho mexendo em param novo.
        //
        // O canal e ENDEREÇO, nao filtro: `ApplyMidiCc` casa cc E canal,
        // e um CC que chega num canal sem vinculo NAO move nada — que e
        // o que faz a recusa de 3 e 4 ser barata no voo B.
        // ============================================================
        AddMidi(20, p_piezo_mix, kCanalCompart);        // 64 = centro, as duas cadeias   // COMPARTILHADO: so ch1
        AddMidi(21, p_in_gain[0], kCanalEntradaA);
        AddMidi(21, p_in_gain[1], kCanalEntradaB);
        AddMidi(23, p_gate_thresh[0], kCanalEntradaA);   // em 0 o gate e PULADO
        AddMidi(23, p_gate_thresh[1], kCanalEntradaB);
        AddMidi(25, p_gate_release[0], kCanalEntradaA);
        AddMidi(25, p_gate_release[1], kCanalEntradaB);

        // ESTADO INTERINO DOS CCs DE CARATER — ler antes de estranhar.
        //
        // Os dez params de caracter deixaram de ser compartilhados
        // (v0.131.0). Ate a multiplexacao por canal entrar, cada um
        // desses CCs enderecа SO A CADEIA A, e a copia da cadeia B fica
        // alcancavel apenas por `set <nome>_b`.
        //
        // O QUE MUDA NO OUVIDO: antes, mexer no CC 34 movia o drive das
        // DUAS cadeias, porque havia um valor so. Agora move so a A. Nao
        // e defeito, e o preco da duplicacao — e some quando o canal
        // MIDI passar a escolher a cadeia.
        //
        // Nao ha CC novo aqui de proposito: gastar dez numeros agora
        // seria construir o que o passo seguinte apaga. O `board`
        // reporta quantos params estao sem CC, para que "cadeia B
        // inalcancavel" nao passe por "cadeia B desligada" — feature
        // morta parece feature desligada, e o default e zero.
        //
        // ---- DRIVE: 30-35 ----
        AddMidi(30, p_drv_on[0], kCanalEntradaA);
        AddMidi(31, p_drv_mix[0], kCanalEntradaA);
        AddMidi(30, p_drv_on[1], kCanalEntradaB);
        AddMidi(31, p_drv_mix[1], kCanalEntradaB);
        AddMidi(34, p_drv_drive[0], kCanalEntradaA);
        AddMidi(34, p_drv_drive[1], kCanalEntradaB);
        AddMidi(35, p_drv_tone[0], kCanalEntradaA);
        AddMidi(35, p_drv_tone[1], kCanalEntradaB);
        AddMidi(36, p_drv_sujeira[0], kCanalEntradaA);
        AddMidi(36, p_drv_sujeira[1], kCanalEntradaB);

        // ---- PHASER: 40-47 ----
        AddMidi(40, p_phs_on[0], kCanalEntradaA);
        AddMidi(41, p_phs_mix[0], kCanalEntradaA);       // MONTANHA: pico em ~64
        AddMidi(40, p_phs_on[1], kCanalEntradaB);
        AddMidi(41, p_phs_mix[1], kCanalEntradaB);
        AddMidi(44, p_phs_rate[0], kCanalEntradaA);
        AddMidi(44, p_phs_rate[1], kCanalEntradaB);
        AddMidi(45, p_phs_depth[0], kCanalEntradaA);
        AddMidi(45, p_phs_depth[1], kCanalEntradaB);
        AddMidi(46, p_phs_feedback[0], kCanalEntradaA);
        AddMidi(46, p_phs_feedback[1], kCanalEntradaB);
        AddMidi(47, p_phs_stages[0], kCanalEntradaA);
        AddMidi(47, p_phs_stages[1], kCanalEntradaB);

        // ---- WARBLE: 50-56 ----
        AddMidi(50, p_wrb_on[0], kCanalEntradaA);
        AddMidi(51, p_wrb_mix[0], kCanalEntradaA);
        AddMidi(50, p_wrb_on[1], kCanalEntradaB);
        AddMidi(51, p_wrb_mix[1], kCanalEntradaB);
        AddMidi(54, p_wrb_rate[0], kCanalEntradaA);
        AddMidi(54, p_wrb_rate[1], kCanalEntradaB);
        AddMidi(55, p_wrb_depth[0], kCanalEntradaA);
        AddMidi(55, p_wrb_depth[1], kCanalEntradaB);
        AddMidi(56, p_wrb_irreg[0], kCanalEntradaA);
        AddMidi(56, p_wrb_irreg[1], kCanalEntradaB);

        // ---- DELAY: 60-64 — envio por cadeia, UMA instancia ----
        AddMidi(60, p_dly_on[0], kCanalEntradaA);
        AddMidi(61, p_dly_mix[0], kCanalEntradaA);
        AddMidi(60, p_dly_on[1], kCanalEntradaB);
        AddMidi(61, p_dly_mix[1], kCanalEntradaB);
        AddMidi(64, p_dly_time, kCanalCompart);          // COMPARTILHADO, 50 ms .. 1 s

        // ---- FREEZE: 65-70 — envio por cadeia, UMA instancia ----
        // `frz_on_a/b` dizem QUAL cadeia alimenta; `frz_hold` diz QUANDO
        // congelar. Fundir os dois faria escolher a fonte e disparar o
        // gesto no mesmo botao.
        AddMidi(65, p_frz_on[0], kCanalEntradaA);         // a chave E o gesto de congelar
        AddMidi(66, p_frz_vol[0], kCanalEntradaA);
        AddMidi(67, p_frz_size[0], kCanalEntradaA);
        AddMidi(65, p_frz_on[1], kCanalEntradaB);
        AddMidi(66, p_frz_vol[1], kCanalEntradaB);
        AddMidi(67, p_frz_size[1], kCanalEntradaB);

        // ---- ESPACO A: 71-79 · sala e shimmer PROPRIOS da cadeia A ----
        // `rev_on/rev_mix` sao o ENVIO, mesma forma do delay e do
        // freeze. O shimmer nao tem envio proprio: ele E esta sala
        // realimentada com pitch, e entra pelo mesmo caminho.
        AddMidi(71, p_rev_on[0], kCanalEntradaA);
        AddMidi(72, p_rev_mix[0], kCanalEntradaA);
        AddMidi(73, p_rev_size[0], kCanalEntradaA);
        AddMidi(74, p_rev_damp[0], kCanalEntradaA);
        AddMidi(75, p_shm_on[0], kCanalEntradaA);
        AddMidi(76, p_shm_mix[0], kCanalEntradaA);       // SO a camada direta
        AddMidi(77, p_shm_feedback[0], kCanalEntradaA);
        AddMidi(78, p_shm_shift[0], kCanalEntradaA);
        AddMidi(79, p_shm_tone[0], kCanalEntradaA);

        // ---- ESPACO B: 80-88 · sala independente ----
        AddMidi(71, p_rev_on[1], kCanalEntradaB);
        AddMidi(72, p_rev_mix[1], kCanalEntradaB);
        AddMidi(73, p_rev_size[1], kCanalEntradaB);
        AddMidi(74, p_rev_damp[1], kCanalEntradaB);
        AddMidi(75, p_shm_on[1], kCanalEntradaB);
        AddMidi(76, p_shm_mix[1], kCanalEntradaB);
        AddMidi(77, p_shm_feedback[1], kCanalEntradaB);
        AddMidi(78, p_shm_shift[1], kCanalEntradaB);
        AddMidi(79, p_shm_tone[1], kCanalEntradaB);

        // ---- BUZIOS: 90-92 ----
        AddMidi(90, p_buzios_on, kCanalCompart);   // COMPARTILHADO: so ch1
        AddMidi(91, p_buzios_thresh, kCanalCompart);   // COMPARTILHADO: so ch1
        AddMidi(92, p_buzios_cooldown, kCanalCompart);   // COMPARTILHADO: so ch1

        // ---- TOM DE TESTE: 93-95 ----
        AddMidi(93, p_tone_freq, kCanalCompart);   // COMPARTILHADO: so ch1
        AddMidi(94, p_tone_mix, kCanalCompart);   // COMPARTILHADO: so ch1
        AddMidi(95, p_tone_dest, kCanalCompart);         // 0=A+B 1=A 2=B
        AddMidi(96, p_test_tone_stage, kCanalCompart);          // 0=direto 1=cadeia · COMPARTILHADO: so ch1

        // ---- MASTER: 100 ----
        AddMidi(100, p_level, kCanalCompart);   // COMPARTILHADO: so ch1

        // ---- PROTECAO E ROTEAMENTO: 110-115 ----
        //
        // LONGE DE PROPOSITO, e a distancia e a defesa. Estes seis
        // saiam do MIDI em 18/08 porque despejos do iOS os punham no
        // extremo e emudeciam o instrumento TRES VEZES numa sessao.
        //
        // Voltam agora por decisao do dB (mapear tudo), e o que mudou
        // desde entao: a QUARENTENA de reconexao existe e barra dumps.
        // Mas o risco residual e real — um controlador mal mapeado que
        // varra 110-115 desliga o roteamento e o instrumento emudece
        // sem defeito nenhum. Por isso ficam a 10 CCs do vizinho mais
        // proximo: varredura acidental nao chega ate aqui.
        // A PROTECAO DUPLICA porque o hardware duplicou: com DOIS
        // exciters, o ramo B tambem fecha laco acustico. Ate 20/08
        // OUT_R era jack de linha e saia limpo por desenho.
        //
        // `shift_hz` E COMPARTILHADO, e isso e decisao acustica, nao
        // economia: dois exciters no MESMO CORPO com deslocamentos
        // diferentes bateriam um contra o outro. Mesmo shift nos dois
        // desfaz o acordo de fase da microfonia sem criar batimento.
        //
        // Governor e boost sao POR RAMO: cada amp e cada exciter tem
        // ganho e acoplamento proprios, entao o teto util difere.
        AddMidi(110, p_gov_ceiling[0], kCanalEntradaA);
        AddMidi(111, p_gov_release[0], kCanalEntradaA);
        AddMidi(112, p_shift_hz, kCanalCompart);         // COMPARTILHADO pelos dois ramos
        AddMidi(113, p_out_l, kCanalCompart);            // 0 = corta o ramo L   // COMPARTILHADO: so ch1
        AddMidi(114, p_out_r, kCanalCompart);            // 0 = corta o ramo R   // COMPARTILHADO: so ch1
        // MATRIZ 2x2 — o CC diz a SAIDA, o canal diz a CADEIA. Mesmo
        // esquema de todo o instrumento, sem numero novo por celula:
        // quatro interruptores em dois CCs.
        AddMidi(97, p_route_l[0], kCanalEntradaA);
        AddMidi(97, p_route_l[1], kCanalEntradaB);
        AddMidi(98, p_route_r[0], kCanalEntradaA);
        AddMidi(98, p_route_r[1], kCanalEntradaB);
        AddMidi(115, p_bench_boost[0], kCanalEntradaA);
        AddMidi(110, p_gov_ceiling[1], kCanalEntradaB);
        AddMidi(111, p_gov_release[1], kCanalEntradaB);
        AddMidi(115, p_bench_boost[1], kCanalEntradaB);
        // O PORTAO DA PROTECAO, um por ramo. Nasce LIGADO: proteger sem
        // precisar custa timbre, nao proteger quando precisa custa o
        // instrumento. Quando os DPDT forem soldados, D29/D30 dizem o
        // estado REAL e estes params viram o espelho do que o polo 2 le.
        AddMidi(105, p_exc_on[0], kCanalEntradaA);
        AddMidi(105, p_exc_on[1], kCanalEntradaB);

        // ---- ORDEM DE SACRIFICIO (indice 0 cai primeiro) ----
        //
        // Proposta inicial, do mais sacrificavel ao menos. E do dB
        // mudar: a ordem e decisao musical, nao tecnica.
        //
        //   (o detune era o indice 0 e saiu do build em 20/08)
        //   phaser   +26 us — movimento, nao identidade.
        //   warble   +14 us — idem, e o mais sutil dos tres.
        //   drive    +21 us — cola; a falta se nota mais que a do phaser.
        //   freeze     alto — GESTO: quando esta rolando, e intencional.
        //                     Derrubar no meio interrompe uma decisao.
        //   shimmer +250 us — CARATER. Caro, e o que mais define o som.
        //                     Cai por ultimo apesar do preco.
        auto S = [&](int id, const char* n) {
            if(n_sacrif < kMaxSacrificaveis)
            { sacrif[n_sacrif].param_on = id;
              sacrif[n_sacrif].nome = n; n_sacrif++; }
        };
        // ORDEM REVISTA no pivo de 20/08: seis slots de caracter mais a
        // cauda. Caracter cai antes da cauda porque a cauda e o que faz
        // tambor e corda soarem como o MESMO OBJETO — perde-la separa o
        // que o desenho existe pra unir.
        //
        // Dentro do caracter, B antes de A e escolha ARBITRARIA e o dB
        // deve reordenar de ouvido. Fica declarado que e arbitraria:
        // derrubar um efeito de UMA cadeia cria assimetria, e se isso
        // incomodar mais que perder o efeito nas duas, a ordem muda.
        for(int c = kCadeias - 1; c >= 0; c--) S(p_phs_on[c], "phaser");
        for(int c = kCadeias - 1; c >= 0; c--) S(p_wrb_on[c], "warble");
        for(int c = kCadeias - 1; c >= 0; c--) S(p_drv_on[c], "drive");
        for(int c = kCadeias - 1; c >= 0; c--) S(p_frz_on[c], "freeze");
        for(int c = kCadeias - 1; c >= 0; c--) S(p_shm_on[c], "shimmer");
        // 34 e o comando `sync` — fica no meio da familia do shimmer
        // por acidente historico, e nao vale remapear: mexer em numero
        // ja alinhado no Loopy custa mais que a assimetria incomoda.
        // 34 e o comando `sync` — interceptado antes do mapa, ver
        //    kCcSync no main.cpp. Nao e parametro e nao entra aqui.
        //
        // FORA DO MAPA POR SEGURANCA, e seguem no `set` da serial:
        //   src_mix     roteamento — emudeceu o instrumento 3x hoje
        //   gov_ceiling borda de feedback
        // O QUE NAO ESTA MAPEADO NAO PODE SER DERRUBADO POR FANTASMA.

        // >>> voc_pitch tem steps=49: o Set() quantiza em semitons, e o
        // >>> CC de 128 passos cai na grade cromatica sozinho. Nada a
        // >>> fazer aqui — a quantizacao mora no ponto UNICO de escrita,
        // >>> entao MIDI, serial e knob herdam a mesma grade de graca.

        // shift_hz NÃO tem ModBind: o shifter é bloco do shell, lido
        // direto do callback, como gate/governor/limiter. Slot Fx2
        // segue vago até o freeze.
        // exemplo de macro composta (comentado):
        // knob[1].AddTarget(p_verb_size, 0.8f);
        // knob[1].AddTarget(p_verb_damp, -0.5f); // invertido
    }
};

} // namespace cotuca
