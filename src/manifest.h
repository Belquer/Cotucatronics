#pragma once
// manifest.h (instrumento "bancada") v0.27.0
// ============================================================
// Cotucatronics OS — MANIFEST do instrumento
// Este é o ÚNICO arquivo que muda de escultura para escultura,
// junto com o export do RNBO em modules/<nome>/.
//
// Instrumento de teste: "bancada" — entrada + tom de teste
// atravessando o módulo RNBO "delay".
// ============================================================

#include "params.h"
#include "module.h"
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
    // VOCODER LIGADO (2026-08-16): volta pra 256, porque é o único
    // prazo em que módulo espectral cabe. O 128 foi MEDIDO em 112% —
    // não é aperto, é estouro.
    //
    // O preço é honesto e não é pequeno: a latência sai de 1 ms pra
    // 5,3 ms, e num instrumento realimentado isso desloca onde o
    // feedback se estabelece. Tirar o vocoder do build e voltar pra 48
    // é uma linha aqui e uma no Makefile.
    size_t block_size = 256;
};

// Grau permitido = semitom MOD 12 relativo ao centro da cerca. Lá
// menor natural: a base da portadora é 110 Hz — um Lá — então a
// máscara nasce afinada de graça, sem transposição nenhuma.
// Outra escultura declara outra máscara; é DADO de manifest.
static constexpr uint8_t kScaleAminor[] = {0, 2, 3, 5, 7, 8, 10};

// Alvo de sorteio (B1 e auto-rand): cerca por parâmetro + máscara de
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
    int p_spare;      // slot vazio de exemplo
    int p_src_mix;    // SRC MIX: 0 = IN_L (piezo), 1 = IN_R (linha)
    int p_dly_time;   // → módulo RNBO "dly_time"
    int p_dly_mix;    // → módulo RNBO "dly_mix"

    // ---- cadeia de dinâmica do shell (C++ puro, não RNBO) ----
    // Todos serial-only: são política de segurança e comportamento,
    // não gesto de performance. O painel tem 5 knobs e nenhum deles
    // se gasta aqui.
    int p_gate_thresh;
    int p_gate_release;
    int p_gov_ceiling;
    int p_gov_release;

    // Roteamento de saída: 0/1. Serial-only por ora — o toggle
    // físico de 3 posições assume depois, e vira o único lugar
    // que escreve nestes dois.
    int p_route_exciter; // OUT_L → amp TPA3116 → exciter
    int p_route_jack;    // OUT_R → jack de linha

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
    int p_bench_boost;

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
    int p_fx2_on;

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
    int p_voc_mix;   // → "voc_mix"    0..1, seco/molhado
    int p_voc_pitch; // → "voc_pitch"  0..1, centro 0.5 = sem deslocamento
    int p_voc_tone;  // → "voc_tone"   0..1, novo no export de 2026-08-16

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
    int p_fx2_gain;

    // ============================================================
    // PAINEL v2 — 5 pots + 2 botões momentâneos
    //
    //   k1 = level      k2 = src_mix      k3–k5 = macros de caráter
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
    //   route_*        sortear roteamento pode emudecer a peça, ou
    //                  mandar pro exciter o que devia ir pro jack.
    //   shift_hz       anti-feedback. Mesmo argumento do governor.
    //   bench_boost    ferramenta de bancada, nunca de performance.
    //
    // A regra por trás: o botão pode surpreender o TIMBRE, nunca a
    // SEGURANÇA nem a IDENTIDADE do instrumento. Whitelist e não
    // blacklist porque param novo nasce fora — esquecer de proibir é
    // erro silencioso, esquecer de permitir só faz o botão ignorá-lo.
    RandTarget randomizable[6];
    int        n_randomizable = 0;

    // ---- AUTO-RAND: sorteio disparado por transiente ----
    // Detector no shell (main.cpp), pós-gate: batida acima do limiar
    // dispara UM sorteio nas mesmas cercas do B1. arand_on nasce em 0
    // — é gesto de performance, liga por serial/MIDI, e é candidato ao
    // B2 como toggle quando Toggle existir.
    int p_arand_on;
    int p_arand_thresh;   // limiar do envelope pós-gate, 0..1
    int p_arand_cooldown; // ms entre sorteios; 2 batidas rápidas = 1

    // ---- B2: ASSINALÁVEL ----
    // -1 = sem função. Momentâneo: segura, vale 1; solta, volta.
    int        button2_target = -1;
    ButtonMode button2_mode   = ButtonMode::Hold;

    // ---- ligação com o módulo DSP, POR NOME ----
    // O manifest não conhece índice de parâmetro do RNBO: reexportar
    // o patch reordena tudo. Declara-se o nome, e o boot resolve o
    // índice via Module::FindParam(). Nome que não existe = erro
    // claro no log, não parâmetro fantasma mexendo em nada.
    struct ModBind
    {
        ModuleSlot  slot;     // em qual slot da cadeia o módulo está
        int         param_id; // id na nossa ParamTable
        const char* mod_name; // nome do parâmetro dentro do módulo
    };
    ModBind bind[8];
    int     n_bind = 0;

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
    int        n_midi = 0;

    // Aplica um CC pela via do knob. Devolve o id do parâmetro movido,
    // ou -1 se o CC não está no mapa.
    //
    // Mora aqui, e não no main.cpp, porque o mapa é do manifest —
    // e porque assim o banco de teste nativo (test_midi.cpp) exercita
    // ESTA função, não uma cópia dela. Lógica de mapeamento duplicada
    // entre firmware e teste é teste que valida a si mesmo.
    int ApplyMidiCc(ParamTable& t, uint8_t cc, uint8_t value) const
    {
        for(int i = 0; i < n_midi; i++)
        {
            if(midi_map[i].cc != cc)
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
        // nome, default (0-1), min real, max real
        p_level     = t.Add("level",     0.4f, 0.f,    1.f);
        p_tone_freq = t.Add("tone_freq", 0.4f, 80.f,   2000.f);
        p_tone_mix  = t.Add("tone_mix",  0.0f, 0.f,    1.f);
        p_spare     = t.Add("spare",     0.0f, 0.f,    1.f);
        p_src_mix   = t.Add("src_mix",   0.0f, 0.f,    1.f);
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
        p_dly_time  = t.Add("dly_time",  0.69f, 0.f,   1.f,  2.5f);
        p_dly_mix   = t.Add("dly_mix",   0.5f, 0.f,    1.f);

        // GATE. gate_thresh usa curve 2 porque threshold útil vive
        // todo no fundo da faixa: linear, os primeiros 5% do curso
        // seriam a única parte usável. O default REAL é 0.02
        // (~-34 dBFS), e como Real() = pow(norm,2), o norm que
        // produz 0.02 é sqrt(0.02) = 0.1414.
        p_gate_thresh  = t.Add("gate_thresh",  0.0f, 0.f,  1.f, 2.0f);
        // release em ms: 50–2000, default 300
        p_gate_release = t.Add("gate_release", 0.1282f, 50.f, 2000.f);

        // GOVERNOR. gov_ceiling é o futuro destino do ENCODER físico:
        // é a "borda de feedback", o ponto onde a escultura para de
        // crescer. Linear de propósito — a borda é procurada pelo
        // ouvido, e curva atrapalharia o gesto fino.
        p_gov_ceiling = t.Add("gov_ceiling", 1.0f,    0.f,  1.f);
        // release em ms: 100–3000, default 800
        p_gov_release  = t.Add("gov_release",  0.2414f, 100.f, 3000.f);

        // ROTEAMENTO. Os dois ligados no boot: mudo por engano é pior
        // que alto por engano — pra alto o limiter já tem resposta.
        p_route_exciter = t.Add("route_exciter", 1.0f, 0.f, 1.f);
        p_route_jack    = t.Add("route_jack",    1.0f, 0.f, 1.f);

        // FERRAMENTA DE BANCADA — ver bloco acima. def_norm 0.0 com
        // faixa [1,8] e curve 1 dá real = 1.0 no boot, sempre.
        p_bench_boost   = t.Add("bench_boost",   0.0f, 1.f, 8.f);

        // SHIFTER. Faixa 0–10 Hz, default 4 Hz (norm 0.4). Linear
        // pelo mesmo motivo do gov_ceiling: a borda se procura de
        // ouvido. Acima de ~10 Hz o deslocamento deixa de ser
        // proteção transparente e vira efeito — as harmônicas param
        // de ser múltiplas inteiras e o timbre metaliza.
        p_shift_hz      = t.Add("shift_hz",      0.0f, 0.f, 10.f);

        // tone_freq e tone_mix ficaram SEM knob: os três primeiros
        // foram pro delay e pro level. Seguem alcançáveis pelo serial
        // (`tone on`/`tone off`) e pelo default — o tom de teste é
        // bancada, não performance.

        // >>> EXCECAO TEMPORARIA — BANCADA DE ESCUTA 2026-08-16 <<<
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
        p_fx2_on     = t.Add("fx2_on",     0.0f, 0.f,   1.f);

        // FREEZE. Defaults iguais aos do export, pra o boot nascer
        // com o patch no estado que o autor definiu.
        // Defaults iguais aos do export: mix 1 (molhado), pitch 0.5
        // (centro, deslocamento zero), tone 0.7.
        p_voc_mix   = t.Add("voc_mix",   0.5f, 0.f, 1.f);
        // steps=49: o caminho MANUAL (serial/MIDI/knob) anda em grade
        // cromática — 48 semitons + centro. A ESCALA vale só pro
        // sorteio; a mão tem o cromático inteiro.
        p_voc_pitch = t.Add("voc_pitch", 1.0f, 0.f, 1.f, 1.0f, 49);
        p_voc_tone  = t.Add("voc_tone",  0.7f, 0.f, 1.f);

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
        p_fx2_gain  = t.Add("fx2_gain",  0.0f, 1.f, 256.f, 3.0f);

        // AUTO-RAND. thresh em norm==real (faixa 0..1). Cooldown em
        // ms, default 100: norm = (100-10)/990 = 0.091.
        p_arand_on       = t.Add("arand_on",       0.0f,   0.f,  1.f);
        p_arand_thresh   = t.Add("arand_thresh",   0.15f,  0.f,  1.f);
        p_arand_cooldown = t.Add("arand_cooldown", 0.091f, 10.f, 1000.f);

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
        bind[n_bind++] = {ModuleSlot::Instrument, p_dly_time, "dly_time"};
        bind[n_bind++] = {ModuleSlot::Instrument, p_dly_mix, "dly_mix"};

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
        knob[1].AddTarget(p_src_mix, 1.0f);  // k2 — SRC MIX piezo/linha
        knob[2].AddTarget(p_dly_time, 1.0f); // k3 — macro do delay
        knob[3].AddTarget(p_dly_mix, 1.0f);  // k4 — macro do delay
        knob[4].AddTarget(p_voc_tone, 1.0f); // k5 — macro do vocoder

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
        randomizable[n_randomizable++] = {p_dly_mix, 0.30f, 0.80f};
        // voc_pitch: UMA NOTA NOVA POR HIT, repetição aceita (sorteio
        // uniforme, sem memória). ±12 st ao redor do centro (110 Hz =
        // Lá), quantizado em Lá menor — nunca cromático fora da
        // escala. 1 st = 1/48 do curso (a faixa inteira é ±24 st).
        // Centro em 220 Hz (norm 0.75 — também um Lá), cerca ±12 st =
        // 110..440 Hz. O registro anterior (centro 110 Hz) sorteava
        // certo mas soava tudo GRAVE e parecido — o registro aprovado
        // de ouvido foi o de cima.
        randomizable[n_randomizable++] = {p_voc_pitch, 0.50f, 1.00f,
                                          kScaleAminor, 7, 0.75f, 1.f / 48.f};
        randomizable[n_randomizable++] = {p_voc_tone, 0.40f, 1.00f};

        // ---- B2: sem função neste instrumento ----
        // Exemplo pra quando o freeze voltar ao slot Fx2:
        //   button2_target = p_frz_on;
        //   button2_mode   = ButtonMode::Hold;
        // Aí segurar o botão congela e soltar libera — que é o gesto
        // natural pra freeze, e o motivo de Hold ser o modo default.
        button2_target = -1;

        // nomes conferidos contra o description.json do vocoder
        bind[n_bind++] = {ModuleSlot::Fx2, p_voc_mix, "voc_mix"};
        bind[n_bind++] = {ModuleSlot::Fx2, p_voc_pitch, "voc_pitch"};
        bind[n_bind++] = {ModuleSlot::Fx2, p_voc_tone, "voc_tone"};

        // ---- MAPA MIDI ----
        // Aqui embaixo pelo MESMO motivo dos bind[]: um id lido antes
        // do Add() correspondente vale 0, e 0 é o `level`. Um mapa
        // montado cedo faria todos os CCs mexerem no volume.
        //
        // CCs 20–24: faixa "undefined" da spec MIDI, então não colide
        // com nada padrão (1 é mod wheel, 7 é volume, 64 é sustain).
        // Controlador genérico sai de fábrica sem usá-las.
        midi_map[n_midi++] = {20, p_dly_time};
        midi_map[n_midi++] = {21, p_dly_mix};
        midi_map[n_midi++] = {22, p_level};
        midi_map[n_midi++] = {23, p_src_mix};
        // gov_ceiling é a "borda de feedback" e não tem knob — os cinco
        // estão ocupados. Até o encoder físico existir, o CC 24 é o
        // único jeito de procurar a borda sem largar a escultura pra
        // digitar no serial. É o mapeamento que mais justifica o MIDI.
        midi_map[n_midi++] = {24, p_gov_ceiling};
        // CC 25: o ganho do Fx2. MIDI e nao serial de proposito — com o
        // vocoder ligado a serial fica surda (defeito conhecido, ver
        // TESTES.md), e o MIDI e o unico caminho de controle que
        // sobrevive. "MIDI e um knob remoto" deixou de ser slogan e
        // virou a unica mao no ganho.
        midi_map[n_midi++] = {25, p_fx2_gain};

        // shift_hz NÃO tem ModBind: o shifter é bloco do shell, lido
        // direto do callback, como gate/governor/limiter. Slot Fx2
        // segue vago até o freeze.
        // exemplo de macro composta (comentado):
        // knob[1].AddTarget(p_verb_size, 0.8f);
        // knob[1].AddTarget(p_verb_damp, -0.5f); // invertido
    }
};

} // namespace cotuca
