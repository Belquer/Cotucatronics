// ============================================================
// Cotucatronics OS — esqueleto v0.84.0 (board-agnostic)
// Este arquivo NÃO sabe qual placa é o cérebro, nem qual motor de
// DSP está rodando: fala com `Module`, não com o RNBO.
// Compile com: make BOARD=daisy_seed
// ============================================================

#define COTUCA_OS_VERSION "0.84.0"

#include "hal.h"
#include "params.h"
#include "manifest.h"
#include "module.h"
#include "dynamics.h"
#include "reverb.h"
#include "freeze.h"
#include "pitch.h"
#include "effect_slot.h"
#include "shimmer.h"
#include "drive.h"
#include "phaser.h"
#include "warble.h"
#include "detune.h"
#include "suave.h"
#include "midi.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

static cotuca::ParamTable params;
static cotuca::Manifest   manifest;

// ============================================================
// DECISÃO DE ARQUITETURA — PRIORIDADE ENTRE FONTES DE CONTROLE
//
// Três fontes escrevem nos mesmos parâmetros: o painel falso do
// SERIAL, o MIDI, e o KNOB FÍSICO quando existir.
//
// >>> ÚLTIMA ESCRITA VENCE. Não há hierarquia. <<<
//
// As três são PARES. Nenhuma tem precedência sobre a outra, e nenhuma
// "trava" ou "solta" as demais. Quem mexeu por último definiu o valor,
// ponto — como acontece com duas mãos no mesmo knob.
//
// POR QUÊ, e por que isto não é preguiça:
//
// 1. Não existe fonte mais legítima. O serial é bancada, o MIDI vai
//    ser o pedal e o controlador do palco, o knob é a mão na
//    escultura. Eleger um vencedor fixo quebraria justamente o caso
//    de uso do outro — knob-vence-sempre tornaria o pedal decorativo,
//    MIDI-vence-sempre congelaria o painel no meio do show.
//
// 2. A alternativa séria seria "take-over": o knob físico só assume
//    quando o valor dele CRUZA o valor atual, pra não haver salto.
//    É o comportamento certo pra sintetizador com preset. Aqui não:
//    a escultura não tem cena salva, e take-over custa um parâmetro
//    que não obedece até você achar a posição — no palco isso é um
//    knob quebrado. Salto é honesto, silêncio não.
//
// 3. Toda fonte escreve NORMALIZADO na ParamTable, no mesmo ponto.
//    Não há caminho paralelo pra divergir: a curva do manifest, o
//    Shaped() do módulo e o Real() do DSP acontecem depois, iguais
//    pra qualquer origem. MIDI é um knob remoto.
//
// >>> CONDIÇÃO PRA ISSO SER VERDADE, e é uma armadilha real: <<<
// Uma fonte que reescreve o parâmetro A CADA VOLTA DO LOOP não é par
// de ninguém — ela vence sempre, porque a última escrita é sempre
// dela. O laço de knobs abaixo roda a cada ~1 ms, então ele SÓ pode
// aplicar quando o valor MUDOU. Sem essa guarda, ligar os pots
// físicos mataria o MIDI e o serial em silêncio, e o sintoma seria
// "o CC não faz nada" — um bug de arquitetura disfarçado de bug de
// parser. Ver o laço em main().
// ============================================================

// ---- MIDI ----
// A máquina de estados é board-agnostic (midi.h). O HAL entrega
// bytes; o parser entende; o mapa do manifest decide o que move.
static cotuca::MidiParser midi;

// Chamado pelo parser quando um CC completo passou pelo filtro de
// canal. Roda no LOOP PRINCIPAL, não em ISR — hal::PollMidi() é quem
// despacha, exatamente como PollSerial() faz com as linhas.
// ---- DIAGNOSTICO DE RADIO (camada 2 do sistema WIDI/BLE) ----
//
// E o mscope da camada de radio, e a filosofia e a mesma: a ferramenta e
// NOSSA e nenhum app de terceiro entra no caminho critico. O mscope
// responde "o que chegou"; isto responde "o link esta vivo, e se nao
// esta, ha quanto tempo parou".
//
// A distincao que o comando existe pra fazer: `bytes 0` e IDENTICO em
// "nunca conectou" e em "conectou e caiu", e os consertos sao
// diferentes. Idade do ultimo byte separa os dois em uma linha.
static uint32_t ble_last_rx_ms   = 0; // 0 = nunca recebeu nada
static uint32_t ble_last_bytes   = 0;
static uint32_t ble_session_base = 0; // CcCount no inicio da sessao
static uint32_t ble_drops        = 0;
static uint32_t ble_reconnects   = 0;
static bool     ble_drop_flagged = false;

// ============================================================
// QUARENTENA DE RECONEXAO — o detector de dump virou DEFESA
//
// CASO DE CAMPO (18/08): transicoes de link BLE fazem o iOS/Loopy
// reenviar a posicao de todos os widgets de uma vez. O instrumento
// obedecia: som silenciado na desconexao, e freeze com frz_size no
// TETO na reconexao. Comandos fantasma, que ninguem deu.
//
// O detector ja existia como SENSOR (>= 4 CCs distintos em < 100 ms
// apos silencio > 5 s = provavel reconexao). Aqui ele vira portao.
//
// POR QUE SEGURAR ANTES DE DECIDIR, e nao bloquear depois de detectar:
// o gatilho e o QUARTO CC distinto. Bloquear a partir dali deixaria os
// TRES PRIMEIROS entrarem — e com 14 CCs no mapa, tres bastam pro
// estrago observado (um deles ja seria o frz_size). Entao todo CC que
// chega depois de silencio > 5 s fica SEGURO por ate 100 ms, sem ser
// aplicado, ate a duvida se resolver:
//
//   confirmou dump  -> descarta tudo o que estava seguro + 500 ms de
//                      quarentena. Nada fantasma encosta em parametro.
//   nao confirmou   -> libera o segurado NA ORDEM, e a vida segue.
//
// O CUSTO, declarado: o primeiro gesto depois de 5 s parado chega com
// ate 100 ms de atraso. Uma vez, e so depois de silencio. Um fader
// varrido continua instantaneo, porque a partir do segundo CC a janela
// ja se resolveu.
//
// POR QUE O FALSO POSITIVO E DESPREZIVEL: gesto humano nao tem esse
// perfil. Um fader real e UM CC repetido em rampa; dois dedos sao dois
// CCs. Quatro CCs DISTINTOS no mesmo centesimo de segundo nao e mao —
// e maquina reenviando estado.
static constexpr uint32_t kQuarWindowMs   = 100;  // janela de decisao
static constexpr uint32_t kQuarBlockMs    = 500;  // bloqueio apos confirmar
static constexpr uint32_t kQuarSilenceMs  = 5000; // silencio que abre suspeita
static constexpr int      kQuarDistinct   = 4;    // CCs distintos = dump
static constexpr int      kQuarHoldMax    = 16;

struct HeldCc { uint8_t cc, value; };
static HeldCc   quar_held[kQuarHoldMax];
static int      quar_held_n   = 0;
static uint8_t  quar_seen[kQuarHoldMax];
static int      quar_seen_n   = 0;
static uint32_t quar_win      = 0; // 0 = sem janela aberta
static uint32_t quar_block_to = 0; // fim do bloqueio confirmado
static uint32_t quar_last_ms  = 0;
static uint32_t cc_quarentena = 0; // CCs recusados — visivel no `midi`

static void ApplyCcNow(uint8_t cc, uint8_t value)
{
    // O mapa é do manifest, e aplicá-lo também: ver Manifest::ApplyMidiCc.
    // CC fora do mapa devolve -1 e é ignorado de propósito — mas o
    // parser já o guardou no histórico. No dia da solda, ver um CC
    // chegando e não fazer nada é informação MUITO diferente de não
    // ver nada.
    if(manifest.ApplyMidiCc(params, cc, value) >= 0)
        midi.NoteApplied(); // CC que achou dono; fora do mapa nao conta
}

static void QuarFlush()
{
    for(int i = 0; i < quar_held_n; i++)
        ApplyCcNow(quar_held[i].cc, quar_held[i].value);
    quar_held_n = 0;
    quar_win    = 0;
}

// Chamado do loop principal. Existe porque a janela pode expirar SEM
// que chegue outro CC — o humano que mexeu uma vez e parou. Sem este
// tique o gesto dele ficaria seguro pra sempre.
static void QuarTick()
{
    using namespace cotuca;
    if(quar_win && (hal::NowMs() - quar_win) > kQuarWindowMs)
        QuarFlush();
}

// CC RESERVADO COMO COMANDO, nao como parametro: pede a placa pra
// EMPURRAR o estado dela pro controlador. Resolve na direcao certa um
// problema que vinha sendo contornado na errada.
//
// O PROBLEMA: os defaults do manifest nao sobrevivem a conexao. Toda
// gravacao restaura o aprovado, e o primeiro CC do Loopy sobrescreve
// com onde os faders dele estao parados — entao o instrumento acorda
// no estado do controlador, nao no seu proprio.
//
// Zerar os defaults seria contornar: faria o controlador SEMPRE vencer.
// Isto inverte: a placa fala primeiro, e o controlador se alinha a ela.
static constexpr uint8_t kCcSync = 34;

static void MidiPushEstado()
{
    using namespace cotuca;
    int n = 0;
    for(int i = 0; i < manifest.n_midi; i++)
    {
        const int id = manifest.midi_map[i].param_id;
        if(id < 0) continue;
        const float norm = params.Get(id);
        uint8_t v = (uint8_t)(norm * 127.f + 0.5f);
        if(v > 127) v = 127;
        const uint8_t msg[3] = {0xB0, manifest.midi_map[i].cc, v};
        hal::MidiSendBytes(msg, 3);
        // Respira entre mensagens: a 31250 baud tres bytes levam ~1 ms,
        // e despejar 12 CCs de uma vez e exatamente o padrao que a
        // quarentena do OUTRO lado (se houver) trataria como dump.
        hal::DelayMs(3);
        n++;
    }
    hal::Log("sync: %d CCs empurrados pro controlador — os faders do"
             " Loopy devem pular pro estado da placa", n);
}

static void OnMidiCc(uint8_t /*ch*/, uint8_t cc, uint8_t value)
{
    using namespace cotuca;
    // CC de COMANDO: nao e parametro, nao passa pela quarentena, nao
    // entra no mapa. Borda de subida so.
    if(cc == kCcSync)
    {
        static bool armado = true;
        if(value >= 64)
        {
            if(armado) { armado = false; MidiPushEstado(); }
        }
        else
            armado = true;
        return;
    }

    const uint32_t now = hal::NowMs();

    // 1) Bloqueio confirmado: o CC e REGISTRADO pelo parser (ja
    //    aconteceu antes daqui) mas nao encosta em parametro.
    if(quar_block_to)
    {
        if(now < quar_block_to)
        {
            cc_quarentena++;
            return;
        }
        quar_block_to = 0;
    }

    const uint32_t gap = quar_last_ms ? (now - quar_last_ms) : 0xFFFFFFFFu;
    quar_last_ms       = now;

    // 2) Primeiro CC depois de silencio longo: abre a duvida.
    if(gap > kQuarSilenceMs)
    {
        quar_win    = now ? now : 1u;
        quar_seen_n = 0;
        quar_held_n = 0;
    }

    // 3) Dentro da janela: SEGURA, nao aplica.
    if(quar_win && (now - quar_win) <= kQuarWindowMs)
    {
        bool ja = false;
        for(int i = 0; i < quar_seen_n; i++)
            if(quar_seen[i] == cc) ja = true;
        if(!ja && quar_seen_n < kQuarHoldMax)
            quar_seen[quar_seen_n++] = cc;
        if(quar_held_n < kQuarHoldMax)
            quar_held[quar_held_n++] = {cc, value};

        if(quar_seen_n >= kQuarDistinct)
        {
            // ---- A DEFESA VIROU ESTORVO (18/08) ----
            //
            // A quarentena nasceu porque o despejo de reconexao punha
            // `src_mix` e `gate_thresh` em valores perigosos e emudecia
            // o instrumento. Duas decisoes do dB mataram essa premissa
            // no mesmo dia:
            //
            //   1. os params de roteamento e seguranca SAIRAM do mapa —
            //      o que nao esta mapeado nao pode ser derrubado;
            //   2. os defaults do que o MIDI alcanca foram a ZERO, e o
            //      Loopy (que so transmite) virou a fonte da verdade.
            //
            // Com isso o despejo deixou de ser ataque e virou a
            // INICIALIZACAO desejada. A defesa passou a descartar
            // exatamente o que o instrumento precisa: 5 mensagens
            // chegaram no boot e ZERO foram aplicadas.
            //
            // Entao ela vira SENSOR: continua detectando e contando
            // (diagnostico util — reconexao e evento que se quer ver),
            // e para de descartar. Libera o que estava seguro.
            QuarFlush();
            ble_reconnects++;
            hal::BlackboxWrite(87, (float)ble_reconnects);
            hal::Log("ble: (re)conexao detectada — %d CCs distintos em"
                     " <%lu ms. APLICADOS: com defaults em zero, o"
                     " despejo do controlador E a inicializacao.",
                     quar_seen_n, (unsigned long)kQuarWindowMs);
            quar_win    = 0;
            quar_seen_n = 0;
            return;
        }
        if(false)
        {
            // CONFIRMADO. Descarta o que estava seguro — inclusive os
            // tres primeiros, que e o ponto inteiro deste desenho.
            cc_quarentena += (uint32_t)quar_held_n;
            quar_held_n   = 0;
            quar_win      = 0;
            quar_block_to = now + kQuarBlockMs;
            ble_reconnects++;
            hal::BlackboxWrite(87, (float)ble_reconnects);
            hal::BlackboxWrite(89, (float)cc_quarentena);
            hal::Log("ble: DUMP DE RECONEXAO barrado — %d CCs distintos"
                     " em <%lu ms apos silencio. %lu CCs recusados,"
                     " %lu ms de quarentena.",
                     quar_seen_n, (unsigned long)kQuarWindowMs,
                     (unsigned long)cc_quarentena,
                     (unsigned long)kQuarBlockMs);
        }
        return;
    }

    // 4) Janela expirou sem dump: libera o segurado, na ordem.
    if(quar_win)
        QuarFlush();

    ApplyCcNow(cc, value);
}

static void OnMidiByte(uint8_t b) { midi.Byte(b); }

// ============================================================
// MIDI SCOPE — o analisador que a gente nao tem no bolso
//
// Duas colunas: hex CRU e o que o PARSER CONCLUIU. Byte igual
// significa coisas opostas conforme o estado (0x45 e "data1 de um CC"
// ou "orfao" dependendo do que veio antes), entao mostrar so o hex
// seria mostrar metade.
//
// ORCAMENTO: escreve pelo hal::Log, que descarta em vez de esperar.
// Em rajada o scope tem BURACOS, e isso e projeto, nao defeito —
// ferramenta de diagnostico que segura o loop e o proximo DelayMs
// (battle_scars cap. 7). Os CONTADORES nunca mentem; o stream sim.
// ============================================================
static int      mscope_mode = 0; // 0 = off, 1 = stream, 2 = stats
static uint32_t msc_msgs = 0, msc_last_s = 0, msc_win_msgs = 0;

static const char* VerdictoStr(cotuca::MidiVerdict v)
{
    using V = cotuca::MidiVerdict;
    switch(v)
    {
        case V::RealTime:     return "realtime";
        case V::StatusCanal:  return "status canal";
        case V::StatusSystem: return "status system (cancela running)";
        case V::SysexInicio:  return "sysex inicio";
        case V::SysexCorpo:   return "sysex corpo";
        case V::SysexFim:     return "sysex fim";
        case V::DadoOrfao:    return "?? DADO SEM STATUS";
        case V::DadoParcial:  return "data parcial";
        case V::MsgCompleta:  return "MSG COMPLETA";
        case V::Truncada:     return "!! TRUNCADA (ressync)";
    }
    return "?";
}

static void OnMidiTrace(uint8_t b, cotuca::MidiVerdict v, uint8_t status,
                        int n_data)
{
    using namespace cotuca;
    using V = cotuca::MidiVerdict;
    if(v == V::MsgCompleta) { msc_msgs++; msc_win_msgs++; }
    if(mscope_mode != 1)
        return;
    // Decodifica o que der: CC completo vira nome de param e valor real.
    if(v == V::MsgCompleta && (status & 0xF0) == 0xB0)
    {
        const uint8_t cc  = midi.History(0).cc;
        const uint8_t val = midi.History(0).value;
        const int     pid = manifest.FindMidiParam(cc);
        if(pid >= 0)
            hal::Log("  %02X  CC ch%d cc%d val%d -> %s %.3f", b,
                     (status & 0x0F) + 1, cc, val, params.Name(pid),
                     (double)params.Real(pid));
        else
            hal::Log("  %02X  CC ch%d cc%d val%d -> (fora do mapa)", b,
                     (status & 0x0F) + 1, cc, val);
        return;
    }
    hal::Log("  %02X  %s%s", b, VerdictoStr(v),
             (v == V::DadoParcial && n_data == 1) ? " (data1)" : "");
}

// ---- módulos DSP, por SLOT ----
// Preenchidos do REGISTRO em module.h. O main não inclui header de
// motor nenhum: não sabe que existe RNBO. Slot vazio = passthrough.
static cotuca::Module* mod_slot[static_cast<int>(cotuca::ModuleSlot::kCount)]
    = {};

// índice do parâmetro dentro do módulo, resolvido por nome no boot.
// -1 = nome não existe naquele slot → não empurra nada.
static int   mod_index[8];
static float mod_last[8]; // só empurra o que mudou

// Fonte que alimenta o módulo: crossfade das entradas + tom de teste.
// Estático porque o callback de áudio não aloca. 64 > block_size (48).
// TEM que ser >= ao block_size da placa. Com 64 e bloco 128, o clamp
// abaixo processaria metade do bloco e deixaria a outra metade do
// buffer de saida com lixo do bloco anterior — sem erro, com ruido.
// >= ao maior block_size que o manifest pode pedir (48/128/256).
static constexpr size_t kMaxBlock = 256;
static float            src_buf[kMaxBlock];

// Scratch pro slot Fx2: Module::Process não é seguro in-place, o motor
// lê a entrada enquanto escreve a saída.
static float fx_buf[2][kMaxBlock];

// Portadora ausente = silêncio, não lixo. Usado quando a placa não
// tem entrada de áudio: um módulo de 2 inputs continua rodando, só
// que sem portadora.
static float zero_buf[kMaxBlock] = {};

// REVERB — bloco permanente do shell, não módulo. Ver reverb.h pras
// duas razões (pool de SRAM a 92,4% e a natureza fim-de-linha).
// Pico de saida por balde de pitch (13 semitons da cerca dos buzios).
static float note_peak[13] = {};

static cotuca::HighPass fx2_hp_l, fx2_hp_r;
static cotuca::Reverb reverb;
static bool           reverb_ok = false;

// FREEZE — bloco permanente do shell, no lugar que era do vocoder na
// cadeia. Ver freeze.h pro porquê de ser C++ e não módulo RNBO.
//
// O buffer vai pra SDRAM via AllocBig e ISSO ESTA CERTO, ao contrário
// do reverb, que teve que sair de lá. A diferença é o PADRAO DE
// ACESSO, que a etapa 4 do IFA10000 existe pra perguntar:
//
//   reverb : 48 leituras ESPALHADAS por amostra (8 combs + 4 allpass,
//            estéreo) — cada uma num buffer diferente, cada uma um
//            provável miss. SDRAM custou +12,9 pontos de CPU.
//   freeze : 4 leituras SEQUENCIAIS por amostra (2 grãos x 2 canais),
//            avançando uma posição por amostra. A linha de cache
//            trazida serve as próximas 16 leituras.
//
// Mesma região, custo diferente, porque a pergunta certa nunca foi
// "quantos bytes" e sim "de que jeito".
static cotuca::Freeze freeze;
static bool           freeze_ok = false;
static float*         freeze_mem = nullptr;
// 2 s estéreo a 48 kHz = 192 000 floats = 768 KB. Dois segundos porque
// é o que sustenta uma batida inteira da escultura com cauda; abaixo
// de ~1 s a varredura volta ao início rápido demais e o ouvido acha o
// laço, que é exatamente o defeito que este bloco existe pra não ter.
static constexpr float kFreezeSeconds = 2.0f;

// MOTOR DE PITCH COMPARTILHADO — um buffer, ate 4 vozes. Ver pitch.h e
// docs/CTOS.md §5b. Shimmer, detune e warble consomem ESTE, em vez de
// cada um alocar o seu: e onde o "compartilhado" vira economia real.
//
// 100 ms basta pra faixa util (0.5x a 2x): subindo uma oitava, a
// leitura se aproxima da gravacao em um grao por grao, e a folga de
// partida cobre isso com margem. 9600 floats = 38 KB.
static cotuca::PitchEngine pitch;
static bool                pitch_ok  = false;
static float*              pitch_mem = nullptr;
// 400 ms, nao 100. Os 100 ms cobriam o MINIMO pra leitura nao alcancar
// a gravacao, e isso travava o grao em 25 ms (teto cap/4). Duas razoes
// independentes pedem grao longo, e as duas apareceram no mesmo dia:
//
//   MEDIR: janela de 25 ms tem largura de banda de ~40 Hz e laterais a
//   80 Hz — portadora e lateral quase encostadas. Encurtar o grao NAO
//   ajuda: afasta as laterais e alarga a portadora na mesma proporcao.
//   Com 100 ms, banda ~10 Hz e laterais a 20 Hz: portadora legivel.
//
//   SOAR: grao curto e granular audivel ("chuvisco"); o shimmer quer
//   cauda lisa. O freeze ja tinha mostrado isso na etapa 9.
//
// Custo: 38 400 floats = 150 KB em SDRAM, acesso sequencial.
static constexpr float     kPitchSeconds = 0.40f;

// VALIDACAO ISOLADA (etapa 2 do IFA10000 pro motor): rota de teste que
// substitui o sinal pela leitura deslocada, 100% molhado, pra medir a
// frequencia de saida por FFT. Nao e caminho de producao — o shimmer
// so entra depois que isto passar.
static volatile bool  pitch_test  = false;
static volatile float pitch_ratio = 2.0f;
static volatile float pitch_pk_in  = 0.f;
static volatile float pitch_pk_out = 0.f;

// PECA 2 EM VALIDACAO: o motor de pitch e o primeiro efeito a rodar
// dentro de um EffectSlot. Escolhido em vez do freeze porque o freeze
// tem cauda de 2 s aprovada de ouvido, e o fade do slot e outra coisa
// (bypass de 8 ms) — envolver o freeze agora poria duas rampas
// disputando o mesmo gesto.
//
// O slot entrega tres coisas de uma vez: fade sem clique, custo ZERO
// desligado, e a cronometragem por efeito que faltava (o `ctos` media
// so o freeze).
static cotuca::EffectSlot pitch_slot;

// ---- SHIMMER: motor PROPRIO, no ponto de derivacao da cauda ----
//
// Motor separado do `pitch` porque "compartilhado" e afirmacao sobre a
// FONTE, nao sobre o recurso (docs/CTOS.md §5b, correcao de 18/08). O
// shimmer grava a SAIDA DO REVERB — e a cauda que sobe de oitava e
// realimenta; detune e warble vao gravar o seco, noutro ponto.
static cotuca::PitchEngine pitch_tail;
static bool                pitch_tail_ok  = false;
static float*              pitch_tail_mem = nullptr;
static cotuca::Shimmer     shimmer;
static cotuca::EffectSlot  shimmer_slot;

// DRIVE — primeiro da cadeia, zero memoria. Fecha a classe "sem
// historico" e valida o EffectSlot nela.
static cotuca::Drive      drive;
static cotuca::EffectSlot drive_slot;

// PHASER — depois do drive, antes do delay. Zero buffer, mas N
// multiplicacoes por amostra: a regua do drive vale, trabalho nao e
// memoria. `bypass_style: cut` — sem cauda, corta seco (nao ha nada
// decaindo pra deixar terminar).
static cotuca::Phaser     phaser;
static cotuca::EffectSlot phaser_slot;

// WARBLE — deriva de fita. Buffer minusculo (40 ms mono = 7,5 KB), em
// SRAM: acesso e uma leitura interpolada por amostra, perto da cabeca
// de escrita, entao cabe na cache sem esforco.
static cotuca::Warble     warble;
static cotuca::EffectSlot warble_slot;

// DETUNE — motor PROPRIO no ponto pos-modulo. Terceiro ponto de
// derivacao (seco / cauda do reverb / pos-modulo), e o unico dos cinco
// que consome DUAS vozes do mesmo buffer: o caso que o "compartilhado"
// previa desde o desenho.
static cotuca::PitchEngine pitch_det;
static bool                pitch_det_ok  = false;
static float*              pitch_det_mem = nullptr;
static cotuca::Detune      detune;
// I/O: balanco da entrada e largura da saida. Nascem no NEUTRO (1.0)
// pra nao produzirem um fade-in de estreia que ninguem pediu.
static cotuca::Suave       suave_gl, suave_gr, suave_width;

// ---- TETO DE SOMA DO CTOS ----
//
// IRMAO DO WATCHDOG, NAO SUBSTITUTO. O watchdog por-efeito pega o
// efeito BUGADO — um que sozinho estoura seu orcamento tem defeito, e o
// veto e justo porque ha culpado. Este pega N efeitos SAUDAVEIS que
// juntos nao cabem no prazo. Nao ha culpado, e procurar um fabricaria
// injustica: qualquer regra de "quem foi" acabaria punindo o inocente.
//
// A politica e PRIORIDADE DECLARADA no manifest, decidida a frio pelo
// dB. O sistema no calor nao escolhe nada: le a ordem e obedece.
//
// MEDE O BLOCO INTEIRO, nao a soma de estimativas. Estimativa some
// justamente no caso que importa (interacao, cache, estreia); o
// cronometro do bloco e o unico numero que corresponde ao que de fato
// acontece.
static constexpr float    kTetoFrac      = 0.85f;  // do prazo do bloco
static constexpr uint32_t kBlocosSeguidos = 4;     // ~4 ms antes de agir
// CARENCIA DEPOIS DE CADA QUEDA. Zerar o param NAO faz o efeito parar de
// custar: o EffectSlot desce por rampa e continua processando enquanto
// desce. Sem esperar, o mecanismo mede de novo antes da propria acao
// surtir efeito, ve o bloco ainda alto e derruba o proximo — MEDIDO: com
// teto em 620 e bloco em 626, caiu o detune E o phaser, quando o detune
// (-125 us) sozinho ja resolvia. Um efeito perdido a mais do que o
// necessario e exatamente o que a politica de prioridade existe pra
// evitar.
//
// 250 blocos ~ 250 ms: folgado pras rampas de 8 ms e curto o bastante
// pra que um estouro real nao fique sustentado. A cauda de 2 s do freeze
// nao cabe nesta janela, e nao deve caber: durante ela o custo ESTA la, e
// uma segunda queda seria correta.
static constexpr uint32_t kCarenciaBlocos = 250;
static uint32_t soma_us        = 0;   // ultimo bloco medido
static uint32_t soma_pico_us   = 0;
static uint32_t soma_seguidos  = 0;
static uint32_t soma_quedas    = 0;
static uint32_t soma_carencia  = 0;   // blocos restantes de espera
static uint32_t soma_teto_us   = 0;   // 0 = automatico (kTetoFrac do prazo)
static uint32_t soma_teto_forc = 0;   // != 0 = teto artificial, so pra teste
static bool     soma_caiu[cotuca::Manifest::kMaxSacrificaveis]   = {false};
static uint32_t soma_caiu_ms[cotuca::Manifest::kMaxSacrificaveis] = {0};
static uint32_t soma_caiu_us[cotuca::Manifest::kMaxSacrificaveis] = {0};
// Estourou e nao sobrou nada pra derrubar: a base sozinha nao cabe.
// Estado DIFERENTE de "tudo bem", e tem que se declarar — senao o
// silencio do mecanismo seria lido como sucesso dele.
static bool     soma_sem_saida = false;
// CC de aviso na saida MIDI: valor = indice+1 do efeito derrubado.
static constexpr uint8_t kCCAvisoTeto = 119;
// AVISO PENDENTE — o callback MARCA, o loop FALA.
//
// A primeira versao chamava LogAlways() e MidiSendBytes() de dentro do
// AudioCb. A serial morreu no primeiro teste: escrever no CDC de dentro
// do callback starva a ISR do USB, que e o defeito de prioridade 1 deste
// projeto, ja catalogado, e que eu cometi DEPOIS de ele estar escrito.
//
// Familia A dos bugs de ordem: quem arma e quem executa tem que estar no
// mesmo contexto — e o contexto que pode falar com o mundo e o loop.
// `volatile` porque cruza callback -> loop.
static volatile int      aviso_teto_idx = -1;   // -1 = nada pendente
static volatile uint32_t aviso_teto_us  = 0;
static volatile uint32_t aviso_teto_prazo = 0;
static volatile bool     aviso_sem_saida  = false;

// ---- MEDIDOR DE ENTRADA, POR CANAL, CRU ----
//
// Nao existia, e a falta custou duas perguntas sem resposta: "chega
// sinal no segundo piezo?" e "o in_gain de 8.0 (18 dB) satura?". As
// duas se respondem com o mesmo numero, e nenhuma se responde de
// ouvido — piezo mudo e piezo baixo soam igual atras de um reverb.
//
// CRU DE PROPOSITO: antes do balanco, antes do in_gain, antes do gate.
// Medir depois de qualquer ganho mistura "o que o transdutor entregou"
// com "o que o firmware fez com isso", e e a primeira pergunta que
// precisa de resposta limpa.
static float    in_pk_l = 0.f, in_pk_r = 0.f;
static uint32_t in_n_l  = 0,   in_n_r  = 0;   // amostras acima de -60 dBFS
static uint32_t in_clip_l = 0, in_clip_r = 0; // amostras em |x| >= 0.99

// ---- HISTOGRAMA DE ENTRADA ----
//
// O pico sozinho nao dimensiona um atenuador: um pico e UMA amostra, e
// nao diz se o gesto inteiro mora perto do teto ou se foi um transiente
// solitario. O histograma diz — e a diferenca decide se o divisor
// precisa de 6 ou de 12 dB.
//
// BALDE PELO EXPOENTE DO FLOAT. `|x|` em [2^e, 2^(e+1)) e exatamente
// uma faixa de 6.02 dB, e o expoente sai de um shift e uma mascara. Sem
// log10f no caminho de audio — a versao obvia (um log por amostra por
// canal) custaria mais que varios efeitos da suite somados.
static constexpr int kNBaldes = 11;   // 0 = clip, 1..9 = -6k dB, 10 = fundo
static uint32_t hist_l[kNBaldes] = {0};
static uint32_t hist_r[kNBaldes] = {0};
static uint32_t hist_o[kNBaldes] = {0};   // POS in_gain, ja somado em mono
static float    out_pk_g   = 0.f;         // pico pos-ganho
static uint32_t out_clip_g = 0;
// LATCH: um clip de dez segundos atras ainda tem que aparecer. Contador
// que zera sozinho esconde justamente o evento raro que se procura.
static bool     clip_latch_l = false, clip_latch_r = false, clip_latch_o = false;

static inline int Balde(float a)
{
    uint32_t u;
    __builtin_memcpy(&u, &a, sizeof(u));
    const int e = (int)((u >> 23) & 0xFFu) - 127;   // |a| em [2^e, 2^(e+1))
    int b = -e;                                     // e=-1 -> 1 (-6..0 dBFS)
    if(b < 0) b = 0;                                // >= 0 dBFS: clip
    if(b >= kNBaldes) b = kNBaldes - 1;             // fundo
    return b;
}
static cotuca::EffectSlot  detune_slot;
static bool               warble_ok = false;
alignas(32) static float  warble_mem[1924];
static volatile uint32_t   shimmer_pico = 0; // so o shimmer, sem o reverb

// BISSECCAO (18/08): uma variavel, um voo. Com 0 aqui o freeze nao
// aloca, nao inicializa e nao entra no callback — se o som e a serial
// voltarem, o culpado esta isolado; se nao voltarem, esta em outro
// lugar e o freeze fica inocentado por medicao, nao por opiniao.
#define COTUCA_FREEZE_ENABLED 1

// BUFFERS DO REVERB EM SRAM, NAO SDRAM — e a diferenca foi medida.
//
// Com os buffers em SDRAM (via AllocBig) o reverb custou +12,9 pontos
// de media: 8,0% -> 20,9% no bloco 256. A causa e o padrao de acesso:
// 48 leituras/escritas por amostra espalhadas em 24 linhas de atraso,
// 2,3 M de acessos por segundo — o padrao mais hostil a cache que
// existe, e a SDRAM da Daisy e bem mais lenta que a SRAM interna.
//
// Por que cabe aqui e nao no pool dos modulos: sao regioes DIFERENTES.
// O pool de modulos (49 KB) esta a 92,4% e nao aceitaria isto; a SRAM
// principal tem 512 KB com ~300 KB livres. Alocacao estatica, sem
// portao, sem fragmentacao.
//
// E por que isto NAO viola a regra do AllocBig: a regra existe pra que
// ninguem toque na RAM GRANDE (SDRAM) sem passar pelo portao. Isto e
// .bss comum — a mesma classe do src_buf e do fx_buf logo acima.
alignas(32) static float reverb_mem[28160];

// ============================================================
// REGRA: ORDEM IMPORTA — DUAS FAMÍLIAS.
//
// A) CORRIDA DE CONTEXTO: quem ARMA e quem CHECA têm que ser o
//    mesmo contexto. Este projeto roda dois concorrentes — o
//    callback de áudio (interrupção) e o loop principal — e toda
//    vez que uma proteção foi armada num e a condição que ela
//    protege aconteceu no outro, apareceu bug.
//
// B) ORDEM DE CONFIGURAÇÃO: quem CONFIGURA tem que vir antes de
//    quem CONSOME. Não há concorrência aqui — é sequência dentro
//    de uma função só, e por isso engana mais: o código lê certo
//    de cima pra baixo e mesmo assim está errado, porque o
//    consumidor leu um valor que ainda não tinha sido escrito.
//
// As duas dão o mesmo sintoma — comportamento errado sem erro,
// sem log, sem crash — mas o conserto difere. Em (A) move-se o
// armar pra junto do checar. Em (B) move-se o configurar pra
// antes do consumir. Confundir as duas leva a "sincronizar" o
// que só precisava ser reordenado.
//
// ---- casos (A), corrida de contexto ----
//
// Este projeto tem dois contextos que rodam concorrentes — o
// callback de áudio (interrupção) e o loop principal. Toda vez
// que uma proteção foi ARMADA num e a condição que ela protege
// aconteceu no outro, apareceu bug. Três vezes, todas com a
// lógica correta e só a ordem errada:
//
//  1. DESPACHO SERIAL. O callback de RX do USB (ISR) chamava o
//     painel falso direto, e hal::Log() transmite pelo mesmo
//     USB: o segundo Log esperava um TX que só completa quando a
//     ISR retorna. Deadlock — `list` matava a placa. Conserto: a
//     ISR só enfileira, PollSerial() despacha do loop.
//
//  2. LIGAÇÃO DE PARÂMETRO. bind[] era montado ANTES dos Add()
//     correspondentes no mesmo Init(). Os ids valiam 0, que é um
//     id VÁLIDO — o do `level` —, então o knob de volume dirigia
//     quatro parâmetros do freeze em silêncio. Conserto: todos
//     os Add() antes de todos os bind[].
//
//  3. JANELA DE GRAÇA (abaixo). Duas vezes seguidas: primeiro a
//     transição 0→1 era detectada no loop enquanto o bloco caro
//     rodava no callback; depois o medidor era zerado no loop
//     enquanto a checagem acontecia no callback. Nos dois casos
//     a proteção existia e chegava tarde.
//
// O padrão é sempre o mesmo e o conserto também: mova o ARMAR
// para junto do CHECAR, não o contrário.
// ============================================================

// ---- watchdog de módulo ----
//
// Um módulo que estoura o prazo do bloco não soa mal: ele mata o
// loop principal por inanição, e a placa fica surda no serial — sem
// log, sem cpu, sem jeito de diagnosticar a não ser regravando às
// cegas. Aconteceu com o freeze, cujo Process() não cabe em 1 ms.
//
// O watchdog lê a carga do bloco ANTERIOR de dentro do callback (o
// único contexto que ainda roda quando o loop morreu), e trava o
// Fx2 em bypass ao primeiro estouro. A CPU cai, o loop revive, e o
// evento vira log em vez de placa muda.
//
// Trava, não histerese: um módulo que estourou uma vez vai estourar
// de novo, e ficar oscilando entre ativo e bypass soaria pior que
// ficar fora.
// JANELA DE GRAÇA. Um módulo pode fazer trabalho de UMA VEZ no
// primeiro Process — o RNBO aloca buffers de FFT preguiçosamente,
// por exemplo. Condenar por isso confundiria custo de estreia com
// custo de regime, que é justamente a pergunta que interessa.
//
// Durante a graça o watchdog não trava, e ao fim dela o medidor é
// ZERADO: assim o pico reportado depois é só regime permanente.
static volatile bool     fx2_overrun  = false;
static bool              fx2_reported = false;
static volatile uint32_t fx2_grace    = 0;
static volatile bool     fx2_grace_done = false;
static bool              fx2_was_on   = false;
// A transicao 0->1 e detectada DENTRO do callback, nao no loop.
// Detectar no loop cria corrida: o callback dispara entre o parametro
// mudar e o loop chegar na linha que arma a graca, e o bloco caro da
// estreia roda com graca=0 — trava o watchdog antes da protecao
// existir. Foi exatamente o que aconteceu na primeira tentativa.
static volatile bool     fx2_prev_want = false;
// Pico da ESTREIA, capturado no fim da graca ANTES do reset. Sem
// isto o numero da estreia se perde justamente por causa da protecao
// que existe pra separa-lo do regime — a instrumentacao apagava o
// dado que ela mesma foi criada pra revelar.
static volatile float    fx2_startup_peak = 0.f;
// Custo dos primeiros blocos, isolado um a um. Responde a pergunta
// que decide o conserto da estreia: o custo esta CONCENTRADO no
// primeiro Process (alocacao preguicosa de uma vez) ou ESPALHADO
// pelos blocos iniciais (crescimento incremental)? Warm-up no Init
// resolve o primeiro caso; so mexer no alocador resolve o segundo.
static constexpr int  kProbeBlocks = 8;
static volatile float fx2_probe[kProbeBlocks] = {};
// So a PRIMEIRA ativacao apos o boot interessa: e nela que a
// alocacao preguicosa acontece. Reativar depois mede so o regime e
// apagaria o dado — foi o que aconteceu na primeira tentativa.
static volatile bool  fx2_probe_done = false;

// ---- BANCADA: cerco ao bloco que mata ----
// Dois contadores em volta do Process. A diferenca entre eles e o
// diagnostico inteiro:
//
//   in == out    → todo Process que comecou terminou. Se a placa
//                  morreu assim, morreu FORA do modulo.
//   in == out+1  → um Process entrou e nunca saiu. Trava ou fault
//                  DENTRO do modulo, no bloco de numero `in`.
//
// O watchdog nao consegue ver essa diferenca porque ele so roda no
// inicio do bloco SEGUINTE, que no caso de trava nunca existe.
static volatile uint32_t fx2_in  = 0;
static volatile uint32_t fx2_out = 0;
// Picos medidos em volta do modulo: entrada (saida do delay) e saida
// crua do vocoder, antes do ganho. A razao diz o ganho que falta.
static volatile float    fx2_peak_in  = 0.f;
static volatile float    fx2_peak_out = 0.f;
static constexpr float   kOverrunTrip = 0.90f;
// BANCADA 2026-08-16 — era 24. Reduzido pra 2 pra descobrir se o
// vocoder novo ESTOURA ou TRAVA, que sao doencas diferentes:
//
//   estoura → Process retorna tarde. O watchdog pega no bloco
//             seguinte e loga "Forcado em bypass". Placa sobrevive.
//   trava   → Process nao retorna (ou da fault). NAO EXISTE bloco
//             seguinte, entao o watchdog nunca roda. Placa morre.
//
// Com 24 blocos os dois casos pareciam iguais: a placa morria antes
// da protecao existir. Com 2, o watchdog tem chance quase imediata, e
// o desfecho responde a pergunta.
//
// Encurtar e seguro porque o warm-up do Init ja paga a alocacao
// preguicosa do RNBO — a graca longa era pra quando isso nao existia.
static constexpr uint32_t kGraceBlocks = 2;

// ============================================================
// PAINEL v2 — 2 botões momentâneos
//   B1 = randomizador   ·   B2 = assinalável (hold)
//
// O HAL entrega NÍVEL já com debounce; a detecção de BORDA é aqui,
// porque é aqui que se sabe o que uma borda significa. B1 age na
// subida (um gesto = um sorteio); B2 acompanha o nível (segura,
// vale; solta, volta).
// ============================================================
static bool  btn_prev[2] = {false, false};
static bool  b2_sim      = false; // simulação por serial
static float b2_saved    = 0.f;
static bool  b2_held     = false;

// Rampa curta pro sorteio. Trocar parâmetro em degrau CLICA — é a
// mesma descontinuidade que o soft-knee do governor teve que aprender
// a evitar. 50 ms é curto o bastante pra parecer instantâneo ao gesto
// e longo o bastante pra não estalar.
//
// O B2 NÃO rampa, de propósito: hold é chave, não varredura. Rampar
// um freeze borraria justamente o instante que ele existe pra capturar.
static constexpr uint32_t kRampMs = 50; // duracao REAL da rampa
// RAMPA POR TEMPO, nao por contagem de voltas.
//
// Antes: `step++` por volta, 50 voltas = 50 ms — verdade so enquanto o
// loop girasse a 1 kHz. O DelayMs(1) era relogio E metronomo da rampa,
// e tirar um tirava o outro: UMA VARIAVEL SERVINDO A DOIS PROPOSITOS,
// terceira familia do catalogo — irma da corrida-de-contexto e da
// ordem-de-configuracao, agora no desenho experimental.
//
// Agora e FUNCAO do relogio: t = (agora - inicio) / duracao.
//   - dura 50 ms REAIS, gire o loop como girar;
//   - loop degradado da avaliacoes esparsas, mas sempre no ponto certo
//     da curva — nao ha passos pra perder, ha so o agora;
//   - sem estado incremental corruptivel: qualquer volta reconstroi o
//     valor do zero. Menos uma coisa que degrada quando o mundo degrada.
struct ParamRamp
{
    int      id;
    float    from, to;
    uint32_t start_ms;
    uint32_t dur_ms;
    bool     done;
};
static ParamRamp ramps[8];
static int       n_ramps = 0;

// xorshift32: barato, determinístico e suficiente pra sortear timbre.
// A semente varia com o tick em que o primeiro sorteio acontece, então
// não repete a mesma sequência a cada boot.
// ---- BUZIOS: detector de transiente ----
// O DETECTOR mora no callback (envelope, limiar, histerese, cooldown)
// e o SORTEIO mora no loop principal — familia (A) da regra de ordem:
// quem decide o disparo e quem conta o cooldown sao o MESMO contexto,
// e o loop so consome uma flag pronta. Sorteio no callback seria
// mexer em params e logar de dentro da interrupcao.
static float             buzios_env   = 0.f;
static bool              buzios_above = false;
static volatile bool     buzios_fire  = false;
static volatile uint32_t buzios_cd    = 0; // cooldown, em blocos
// Diagnostico do detector, direto na caixa-preta: o pico que o
// envelope ALCANCOU (slot 8) e quantas vezes disparou (slot 9).
// Separa "limiar alto demais" de "disparou e nao soou" sem serial —
// que e o estado normal com o vocoder ligado.
static float             buzios_env_peak = 0.f;
static uint32_t          buzios_fires    = 0;

static uint32_t rng_state = 0;
static float    RandNorm()
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (float)(rng_state & 0xFFFFFF) / (float)0xFFFFFF;
}

// ---- cadeia de dinâmica do shell ----
static cotuca::Gate     gate;
static cotuca::Governor gov_l; // só no ramo OUT_L (exciter)
static cotuca::Limiter  lim_l; // um por ramo: estados independentes
static cotuca::Limiter  lim_r;
static cotuca::Shifter  shift_l; // anti-feedback, só no ramo do exciter

// ---- oscilador de teste mínimo (sem DaisySP: portável) ----
static float osc_phase = 0.f;
static float osc_inc   = 0.f;

static void OscSetFreq(float hz, float sr) { osc_inc = hz / sr; }
static float OscProcess()
{
    float s = sinf(osc_phase * 2.f * 3.14159265f) * 0.5f;
    osc_phase += osc_inc;
    if(osc_phase >= 1.f) osc_phase -= 1.f;
    return s;
}

// ---- callback de áudio genérico ----
// ---- ONDE O LOOP PAROU, CONTADO PELO LED ----
//
// Um pisca de 1 Hz dirigido pelo proprio loop provou que ele trava, e
// nao conseguiu dizer ONDE — porque quem piscava era o paciente. Um
// instrumento que depende do paciente nao faz autopsia.
//
// Aqui quem pisca e o CALLBACK DE AUDIO, o unico contexto que continua
// vivo (roda por interrupcao). Ele le a ultima FASE que o loop
// completou e pisca esse numero, com pausa longa entre as rajadas:
//
//   1 pisco  = travou logo no inicio da volta
//   5 piscos = chegou ate a FASE 5 e travou depois
//
// E o "ultima fase" da cacada do DelayMs, num canal que nao passa por
// nada que morreu.
static volatile uint8_t loop_fase = 0;

// ---- CTOS: WATCHDOG COM VETO (ver docs/CTOS.md §3) ----
//
// O CTOS mede o tempo do EFEITO por bloco. Estouro => bypass no MESMO
// contexto que detectou (familia A dos bugs de ordem: quem arma e quem
// checa nao podem estar em contextos diferentes).
//
// BYPASS PERMANENTE ate `ctos rearm`, decisao do dB em 18/08. Religar
// sozinho transformaria defeito reproduzivel em intermitente — e
// intermitente foi o que custou a sessao inteira. O instrumento perde o
// EFEITO, nunca a BASE.
//
// Orcamento: metade do prazo do bloco. Um efeito sozinho passando de
// 50% nao deixa margem pra base + dinamica + reverb no mesmo bloco.
static constexpr float kEfeitoFracao = 0.50f;
static volatile bool     ctos_bypass_frz   = false;
static volatile uint32_t ctos_bypass_ms    = 0;   // quando estourou
static volatile uint32_t ctos_pico_us      = 0;   // pior caso visto
static volatile uint32_t ctos_orcamento_us = 0;
static volatile uint32_t ctos_ocorrencias  = 0;
static volatile uint32_t loop_voltas       = 0;

static void LedFaseTick(size_t block_size, float sample_rate)
{
    // Contado em amostras: o callback e o nosso relogio agora.
    static uint32_t n = 0;
    static bool     aceso = false;
    n += (uint32_t)block_size;
    const uint32_t por_ms = (uint32_t)(sample_rate / 1000.f);
    const uint32_t ms     = n / (por_ms ? por_ms : 48u);

    const uint8_t  alvo = loop_fase ? loop_fase : 1u;
    const uint32_t dur  = (uint32_t)alvo * 300u + 1200u;
    const uint32_t t    = ms % dur;
    const bool quer = (t < (uint32_t)alvo * 300u) && ((t % 300u) < 150u);
    if(quer != aceso)
    {
        aceso = quer;
        cotuca::hal::SetLed(aceso);
    }
}

static void AudioCb(const float* const* in, float** out, size_t size)
{
    const float level = params.Real(manifest.p_level);
    const float mix   = params.Real(manifest.p_tone_mix);
    // BALANÇO dos dois piezos, aplicado ANTES de qualquer
    // processamento — é a fonte do instrumento, não um efeito.
    //   centro (0) = os dois cheios
    //   +1 = só IN_R    -1 = só IN_L
    // Um lado abaixa, o outro NÃO SOBE nem desce: é isso que separa
    // balanço de crossfade, e é o que resolve "um encobre o outro".
    const float bal      = params.Real(manifest.p_piezo_mix);
    const float alvo_gl  = 1.f - (bal > 0.f ?  bal : 0.f);
    const float alvo_gr  = 1.f - (bal < 0.f ? -bal : 0.f);
    OscSetFreq(params.Real(manifest.p_tone_freq),
               cotuca::hal::Info().sample_rate);

    const bool has_in = cotuca::hal::Info().has_audio_in;

    if(size > kMaxBlock)
        size = kMaxBlock; // não estoura o scratch; nunca acontece com blk=48

    const uint32_t t_bloco0 = cotuca::hal::NowTicks();

    // TETO DE SOMA: efeito derrubado fica derrubado ate `ctos rearm`.
    // O latch e reaplicado TODO bloco, e nao so no instante da queda:
    // sem isso um CC do Loopy religaria o efeito e o teto teria virado
    // sugestao. Bypass permanente foi decisao do dB em 18/08 — religar
    // sozinho fabrica intermitencia e esconde o defeito.
    for(int k = 0; k < manifest.n_sacrif; k++)
        if(soma_caiu[k]) params.Set(manifest.sacrif[k].param_on, 0.f);

    // 0) MEDIDOR DE ENTRADA — antes de tudo, inclusive do balanço.
    if(has_in)
        for(size_t i = 0; i < size; i++)
        {
            const float al = in[0][i] < 0.f ? -in[0][i] : in[0][i];
            const float ar = in[1][i] < 0.f ? -in[1][i] : in[1][i];
            if(al > in_pk_l) in_pk_l = al;
            if(ar > in_pk_r) in_pk_r = ar;
            if(al > 0.001f)  in_n_l++;
            if(ar > 0.001f)  in_n_r++;
            if(al >= 0.99f) { in_clip_l++; clip_latch_l = true; }
            if(ar >= 0.99f) { in_clip_r++; clip_latch_r = true; }
            hist_l[Balde(al)]++;
            hist_r[Balde(ar)]++;
        }

    // 1) FONTE — BALANÇO das duas entradas + tom de teste por cima.
    //    O tom entra aqui, antes do módulo, pra poder ser a fonte do
    //    eco no teste de bancada.
    for(size_t i = 0; i < size; i++)
    {
        // Ganhos suavizados por amostra: o CC tem 7 bits e o degrau
        // de um passo é audível como clique num ganho de entrada.
        const float gl   = suave_gl.Passo(alvo_gl);
        const float gr   = suave_gr.Passo(alvo_gr);
        const float dry  = has_in ? (in[0][i] * gl + in[1][i] * gr) : 0.f;
        const float tone = OscProcess();
        src_buf[i]       = dry * (1.f - mix) + tone * mix;
    }

    // 1.5) GANHO DE ENTRADA — pós-fonte, PRÉ-GATE. Ver a nota no
    //      manifest: é aqui que o piezo ganha corpo sem empurrar a
    //      saída contra o governor e o limiter.
    {
        const float ig = params.Real(manifest.p_in_gain);
        if(ig != 1.f)
            for(size_t i = 0; i < size; i++)
                src_buf[i] *= ig;

        // POS-GANHO, e ja somado em mono: e o que a cadeia de efeitos
        // realmente ve. Medir so a entrada crua responderia "o
        // transdutor entrega?" e deixaria "o firmware estoura?" em
        // aberto — sao as duas metades da mesma pergunta.
        for(size_t i = 0; i < size; i++)
        {
            const float a = src_buf[i] < 0.f ? -src_buf[i] : src_buf[i];
            if(a > out_pk_g) out_pk_g = a;
            if(a >= 0.99f) { out_clip_g++; clip_latch_o = true; }
            hist_o[Balde(a)]++;
        }
    }

    // 1.6) ROTA DE TESTE DO MOTOR DE PITCH (validacao isolada).
    //
    // Substitui o sinal pela leitura deslocada, 100% molhado, pra que a
    // frequencia de saida possa ser medida por FFT: seno de 848 Hz
    // entra, 1696 Hz tem que sair com ratio 2.0.
    //
    // Fica AQUI, antes do gate e do modulo, porque validar o motor
    // sozinho exige que nada mais esteja no caminho — medir com o
    // delay ligado mediria os dois. Etapa 0 do IFA10000 aplicada ao
    // proprio motor.
    pitch_slot.SetOn(pitch_test);

    // ---- GRAVAR SEMPRE, PROCESSAR SO QUANDO LIGADO ----
    //
    // A primeira versao pulava o bloco INTEIRO quando desligado — e
    // isso incluia a gravacao. Ao ligar, o motor comecava a ler um
    // buffer parado no ponto em que tinha sido interrompido, e a
    // descontinuidade era um CLIQUE. O fade estava certo; ele cruzava
    // pra um sinal que ja entrava quebrado.
    //
    // O freeze ja tinha escrito a regra no proprio comentario —
    // "gravar SEMPRE e o que permite ao gesto pegar o som que acabou de
    // acontecer" — e eu li como detalhe DELE em vez de como regra de
    // EFEITO COM HISTORICO.
    //
    // Entao "custo zero desligado" vale pro PROCESSAMENTO, nao pra
    // gravacao. Gravar sao dois stores por amostra; os graos, a
    // interpolacao e as janelas sao o que custa 105 us.
    if(pitch_ok && !pitch_slot.Ativo())
        for(size_t i = 0; i < size; i++)
            pitch.Write(src_buf[i], src_buf[i]);

    if(pitch_ok && pitch_slot.Ativo())
    {
        pitch_slot.Comeca(cotuca::hal::NowTicks());
        pitch.SetRatio(0, pitch_ratio);
        if(!pitch.Active(0)) pitch.SetActive(0, true);
        // MEDE OS DOIS LADOS. A teoria dizia que ratio 1.0 devolveria
        // o sinal com ondulacao de +3 dB; a bancada mostrou -25 dB.
        // Quando teoria e medicao discordam, quem mede esta certo — e
        // o pico de ENTRADA vs SAIDA localiza onde some.
        float pin = 0.f, pout = 0.f;
        for(size_t i = 0; i < size; i++)
        {
            const float a = src_buf[i] < 0 ? -src_buf[i] : src_buf[i];
            if(a > pin) pin = a;
            // O motor e ESTEREO mas a fonte aqui e mono (src_buf):
            // grava o mesmo nos dois lados e le o esquerdo.
            pitch.Write(src_buf[i], src_buf[i]);
            float pl, pr;
            pitch.Read(0, pl, pr);
            (void)pr;
            // FADE DE IGUAL-POTENCIA: seco e molhado sao
            // descorrelacionados durante o cruzamento, entao rampa
            // linear abriria um buraco de -3 dB no meio. Raiz de cada
            // lado soma POTENCIA e o nivel fica constante.
            float gs, gm;
            pitch_slot.Passo(gs, gm);
            src_buf[i] = src_buf[i] * gs + pl * gm;
            const float b = pl < 0 ? -pl : pl;
            if(b > pout) pout = b;
        }
        if(pin  > pitch_pk_in)  pitch_pk_in  = pin;
        if(pout > pitch_pk_out) pitch_pk_out = pout;

        const uint32_t prazo
            = (uint32_t)(1e6f * (float)size
                         / cotuca::hal::Info().sample_rate);
        if(pitch_slot.Termina(cotuca::hal::NowTicks(),
                              cotuca::hal::TicksPerUs(),
                              (uint32_t)(0.5f * (float)prazo)))
        {
            // VETO: o instrumento perde o efeito, nunca a base.
            pitch_test = false;
            pitch_slot.SetOn(false);
        }
        // (a voz e solta FORA deste bloco — aqui dentro a condicao
        //  nunca seria verdadeira, porque o bloco so roda com o slot
        //  ativo. Bug de escopo da primeira versao.)
    }

    if(pitch_ok && !pitch_slot.Ativo() && pitch.Active(0))
        pitch.SetActive(0, false); // rampa terminou: libera a voz

    // 2) GATE — na entrada, pós-crossfade e pós-tom. Silencia o chão
    //    de ruído do piezo antes que o módulo o transforme em cauda.
    gate.SetThreshold(params.Real(manifest.p_gate_thresh));
    gate.SetRelease(params.Real(manifest.p_gate_release));
    for(size_t i = 0; i < size; i++)
        src_buf[i] = gate.Process(src_buf[i]);

    // BUZIOS: detector de transiente, POS-GATE de proposito — o
    // gate ja separou gesto de chao de ruido, entao o limiar aqui le
    // batida, nao ambiente. Ataque instantaneo, queda ~40 ms,
    // histerese em 60% do limiar, cooldown em blocos.
    if(params.Real(manifest.p_buzios_on) >= 0.5f)
    {
        const float thr = params.Real(manifest.p_buzios_thresh);
        for(size_t i = 0; i < size; i++)
        {
            const float a = src_buf[i] < 0.f ? -src_buf[i] : src_buf[i];
            if(a > buzios_env)
                buzios_env = a;
            else
                buzios_env *= 0.9995f;
            if(buzios_env > buzios_env_peak)
            {
                buzios_env_peak = buzios_env;
                cotuca::hal::BlackboxWrite(8, buzios_env_peak);
            }
            if(!buzios_above && buzios_env > thr)
            {
                buzios_above = true;
                if(buzios_cd == 0)
                {
                    buzios_fires++;
                    cotuca::hal::BlackboxWrite(9, (float)buzios_fires);
                    buzios_fire = true; // o loop sorteia
                    const float ms = params.Real(manifest.p_buzios_cooldown);
                    buzios_cd = 1
                               + (uint32_t)(ms * 0.001f
                                            * cotuca::hal::Info().sample_rate
                                            / (float)size);
                }
            }
            else if(buzios_above && buzios_env < thr * 0.6f)
                buzios_above = false;
        }
        if(buzios_cd > 0)
            buzios_cd--;
    }

    // ch2 da convenção de 2 entradas: IN_R CRU, pré-crossfade e
    // pré-gate. Ver o contrato em module.h.
    //
    // HOJE NINGUÉM USA. O vocoder v3 é 1-input e gera a própria
    // portadora (senoides + ruído) por dentro; o freeze, que era o
    // outro cliente, está fora do build. O canal continua montado
    // porque o CONTRATO do slot é 2-in — módulo futuro que precise de
    // referência externa acha ela pronta aqui. Fiação viva sem
    // consumidor, não código morto.
    const float* carrier = has_in ? in[1] : zero_buf;

    // 2.5) DRIVE — PRIMEIRO EFEITO DA CADEIA.
    //
    // Depois do gate e antes de tudo o mais: sujeira antes de
    // modulacao, tempo e espaco. Depois do gate de proposito — saturar
    // o chao de ruido e depois tentar porta-lo seria empurrar contra a
    // porta o que ela deveria segurar.
    //
    // SEM CRONOMETRO POR DENTRO. O shimmer mediu 209 us com dois
    // NowTicks por amostra dentro da regiao medida, e boa parte era a
    // propria instrumentacao. A medida limpa aqui e `cpu` ligado vs
    // desligado — o medidor do callback nao tem custo por amostra.
    drive_slot.SetOn(params.Real(manifest.p_drv_on) >= 0.5f);
    if(drive_slot.Ativo())
    {
        drive.SetDrive(params.Real(manifest.p_drv_drive));
        drive.SetTone(params.Real(manifest.p_drv_tone));
        const float mx = params.Real(manifest.p_drv_mix);
        float gs = 0.f, gm = 0.f;
        for(size_t i = 0; i < size; i++)
        {
            drive_slot.Passo(gs, gm);
            // O fade escala o MIX, nao o sinal: o seco passa inteiro o
            // tempo todo, senao ligar o drive abriria um buraco no
            // meio do instrumento.
            drive.SetMix(mx * gm);
            // MONO: a fonte aqui e um canal so. Chamar o par estereo
            // calcularia tanh duas vezes sobre o mesmo valor.
            src_buf[i] = drive.ProcessMono(src_buf[i]);
        }
    }

    // 2.6) PHASER — depois do drive, antes do delay.
    //
    // Ordem de pedaleira: sujeira, depois MOVIMENTO, depois tempo e
    // espaco. Phaser antes do delay faz o eco repetir o movimento;
    // depois, o movimento passaria por cima dos ecos e borraria os dois.
    phaser_slot.SetOn(params.Real(manifest.p_phs_on) >= 0.5f);
    if(phaser_slot.Ativo())
    {
        phaser.SetRate(params.Real(manifest.p_phs_rate));
        phaser.SetDepth(params.Real(manifest.p_phs_depth));
        phaser.SetFeedback(params.Real(manifest.p_phs_feedback));
        phaser.SetStages((int)(params.Real(manifest.p_phs_stages) + 0.5f));
        // LFO E MAPEAMENTO DE FREQUENCIA: UMA VEZ POR BLOCO. As unicas
        // transcendentais do efeito ficam aqui fora — foi o que fez o
        // drive custar 36 us antes de aprender a licao.
        phaser.AtualizaLfo(size);
        const float mx = params.Real(manifest.p_phs_mix);
        float gs = 0.f, gm = 0.f;
        for(size_t i = 0; i < size; i++)
        {
            phaser_slot.Passo(gs, gm);
            phaser.SetMix(mx * gm);
            // MONO: a fonte aqui e um canal so.
            src_buf[i] = phaser.ProcessMono(src_buf[i]);
        }
    }

    // 2.7) WARBLE — depois do phaser, antes do delay.
    //
    // Sujeira, movimento de FASE, movimento de ALTURA, depois tempo e
    // espaco. O warble antes do delay faz o eco repetir a deriva ja
    // impressa; depois, ele derivaria os ecos junto e a fita soaria
    // aplicada ao resultado em vez de a fonte.
    //
    // GRAVA SEMPRE (regra dos efeitos com historico). Aqui a regra
    // morde de verdade: o buffer tem 40 ms, entao parar de gravar em
    // bypass faria o efeito acordar lendo 40 ms de passado congelado.
    warble_slot.SetOn(params.Real(manifest.p_wrb_on) >= 0.5f);
    if(warble_ok && warble_slot.Ativo())
    {
        warble.SetRate(params.Real(manifest.p_wrb_rate));
        warble.SetDepth(params.Real(manifest.p_wrb_depth));
        warble.SetIrreg(params.Real(manifest.p_wrb_irreg));
        warble.AtualizaLfo(size);
        float gs = 0.f, gm = 0.f;
        for(size_t i = 0; i < size; i++)
        {
            warble_slot.Passo(gs, gm);
            const float w = warble.ProcessMono(src_buf[i]);
            // O fade cruza entre seco e derivado. Nao ha `mix`: o
            // cruzamento so existe pra ligar e desligar sem clique.
            src_buf[i] = src_buf[i] * gs + w * gm;
        }
    }
    else if(warble_ok)
    {
        // Bypass: grava, nao processa.
        for(size_t i = 0; i < size; i++) warble.ProcessMono(src_buf[i]);
    }

    // 3) MÓDULO DO INSTRUMENTO — escreve direto no out (2 out).
    cotuca::Module* inst = mod_slot[(int)cotuca::ModuleSlot::Instrument];
    if(inst)
    {
        const float* ins[2] = {src_buf, carrier};
        inst->Process(ins, out, size);
    }
    else
    {
        for(size_t i = 0; i < size; i++)
            out[0][i] = out[1][i] = src_buf[i]; // sem módulo: passa reto
    }

    // 4) FX2 — segundo efeito de instrumento (freeze). ANTES do
    //    level, como o contrato em module.h diz. A ordem importa:
    //    depois do level, o freeze capturaria áudio com o volume já
    //    aplicado, e mexer no level com o som congelado não mudaria
    //    nada — o master deixaria de ser master.
    //
    //    Slot vazio = passthrough silencioso, não erro.
    //    BYPASS: fx2_on nasce em 0. Modulo pesado que estoura o
    //    prazo do bloco nao soa mal — mata o loop principal por
    //    inanicao e a placa fica surda no serial. Com bypass, ligar
    //    e reversivel.
    cotuca::Module* fx2      = mod_slot[(int)cotuca::ModuleSlot::Fx2];
    const bool      fx2_want = fx2 && params.Real(manifest.p_fx2_on) >= 0.5f;
    if(fx2_want && !fx2_prev_want)
    {
        // acabou de ligar: abre a graca AQUI, antes de qualquer bloco
        fx2_grace   = kGraceBlocks;
        fx2_overrun = false;
        cotuca::hal::BlackboxArm(); // so a partir daqui os slots valem
    }
    fx2_prev_want = fx2_want;

    if(fx2_want && fx2_grace > 0)
    {
        // O medidor e zerado a cada bloco da graca, entao o que se le
        // no inicio de um bloco e a carga do bloco ANTERIOR, isolada.
        const uint32_t done = kGraceBlocks - fx2_grace;
        const float    prev = cotuca::hal::CpuLoadMax();
        if(done > 0)
        {
            if(prev > fx2_startup_peak)
                fx2_startup_peak = prev;
            if(!fx2_probe_done && done <= (uint32_t)kProbeBlocks)
                fx2_probe[done - 1] = prev;
        }
        cotuca::hal::CpuLoadReset();

        if(--fx2_grace == 0)
        {
            // ZERA AQUI, no mesmo contexto de quem checa. Deixar o
            // reset pro loop principal cria corrida: o medidor ainda
            // carrega o pico da estreia quando o bloco seguinte faz a
            // checagem, e o watchdog trava por um custo que a graca
            // existia justamente pra perdoar. Terceira vez nesta
            // funcao que contexto errado gera bug — armar protecao e
            // checar condicao TEM que ser o mesmo contexto.
            fx2_probe_done = true; // congela o perfil da estreia
            fx2_grace_done = true; // o loop so loga
        }
    }
    else if(fx2_want && !fx2_overrun
            && cotuca::hal::CpuLoadMax() > kOverrunTrip)
        fx2_overrun = true; // trava; quem loga é o loop principal
    if(fx2_want && !fx2_overrun)
    {
        // O SLOT sempre entrega DOIS canais; quantos o módulo lê é
        // decisão dele, declarada em InputCount() e respeitada no
        // wrapper. Passar menos que dois aqui faria um módulo 2-in
        // ler in[1] fora dos limites — sem crash, com lixo no áudio.
        //
        // ch1 = sinal do ponto da cadeia. ch2 = IN_R cru, HOJE SEM
        // CLIENTE: o vocoder v3 é 1-input e tem portadora interna. O
        // comentário anterior prometia cross-synthesis externa e ficou
        // stale quando o dB reexportou o patch — a promessa sobreviveu
        // ao recurso.
        //
        // ANTES daqui ia {out[0], out[1]}, o par estéreo, o que dava
        // ao módulo o próprio sinal duas vezes. Isso era bug de
        // verdade e continua consertado.
        const float* ins[2]  = {out[0], carrier};
        float*       outs[2] = {fx_buf[0], fx_buf[1]};
        // scratch limpo: export de 1 saída não escreve out[1], e sem
        // isto o canal direito ficaria com o bloco anterior.
        for(size_t i = 0; i < size; i++)
            fx_buf[0][i] = fx_buf[1][i] = 0.f;
        fx2_in++;
        fx2->Process(ins, outs, size);
        fx2_out++;
        // GANHO DE COMPENSACAO DO SLOT. O vocoder sai muito abaixo do
        // delay porque cross-synthesis so devolve energia onde as duas
        // fontes coincidem. Ver a nota no manifest pra por que isto nao
        // e `level` nem `bench_boost`.
        //
        // Aqui e ANTES do level e da dinamica: o governor e o limiter
        // continuam a jusante, entao exagerar comprime em vez de
        // estourar.
        // PASSA-ALTA do Fx2, antes do ganho e da medição de pico.
        {
            const float hz = params.Real(manifest.p_fx2_hp);
            fx2_hp_l.SetCutoff(hz);
            fx2_hp_r.SetCutoff(hz);
            for(size_t i = 0; i < size; i++)
            {
                fx_buf[0][i] = fx2_hp_l.Process(fx_buf[0][i]);
                fx_buf[1][i] = fx2_hp_r.Process(fx_buf[1][i]);
            }
        }
        const float g = params.Real(manifest.p_fx2_gain);
        // MEDE OS DOIS LADOS. `pin` = o que entrou (saida do delay, que
        // e o que o ouvido usa de referencia); `pout` = o que o vocoder
        // devolveu, ANTES do ganho. A razao entre eles e exatamente o
        // ganho de compensacao que falta — numero medido, nao chutado.
        //
        // Isto tem que sair no log dos primeiros blocos porque a serial
        // morre logo depois que o vocoder sobe. Medir num lugar que so
        // pode ser lido quando ja nao da pra ler nao mede nada.
        float pin = 0.f, pout = 0.f;
        for(size_t i = 0; i < size; i++)
        {
            const float a = out[0][i] < 0 ? -out[0][i] : out[0][i];
            const float b = fx_buf[0][i] < 0 ? -fx_buf[0][i] : fx_buf[0][i];
            if(a > pin) pin = a;
            if(b > pout) pout = b;
        }
        if(pin > fx2_peak_in) fx2_peak_in = pin;
        if(pout > fx2_peak_out) fx2_peak_out = pout;
        // CAIXA-PRETA. Escrita a cada bloco, do callback, porque e aqui
        // que o dado existe e daqui a poucos blocos nao havera mais
        // serial pra contar. Custa tres stores.
        cotuca::hal::BlackboxWrite(0, fx2_peak_in);
        cotuca::hal::BlackboxWrite(1, fx2_peak_out);
        for(size_t i = 0; i < size; i++)
        {
            out[0][i] = fx_buf[0][i] * g;
            out[1][i] = fx_buf[1][i] * g;
        }
    }

    // 4.3) DETUNE — PÓS-MÓDULO, onde o estéreo existe.
    //
    // Antes disto a cadeia e um canal so, e sem estereo nao ha spread —
    // detune mono soa como desafinacao, com largura soa como CORO.
    //
    // GRAVA SEMPRE (regra dos efeitos com historico): o buffer tem
    // 400 ms, entao parar de gravar em bypass faria o efeito acordar
    // lendo passado congelado — o clique que a Peca 2 ensinou.
    detune_slot.SetOn(params.Real(manifest.p_det_on) >= 0.5f);
    if(pitch_det_ok)
    {
        const bool det = detune_slot.Ativo();
        if(det)
        {
            detune.SetCents(params.Real(manifest.p_det_cents));
            detune.SetVoices((int)(params.Real(manifest.p_det_voices) + 0.5f));
            detune.SetSpread(params.Real(manifest.p_det_spread));
            pitch_det.SetRatio(0, detune.RatioA());
            pitch_det.SetRatio(1, detune.RatioB());
            pitch_det.SetActive(0, true);
            pitch_det.SetActive(1, detune.Duas());
        }
        const float mx = params.Real(manifest.p_det_mix);
        float gs = 0.f, gm = 0.f;
        for(size_t i = 0; i < size; i++)
        {
            // Grava o canal esquerdo — a fonte pos-modulo e a mesma
            // nos dois lados; a LARGURA nasce aqui, do pan, nao da
            // fonte.
            pitch_det.WriteMono(out[0][i]);
            if(!det) continue;
            detune_slot.Passo(gs, gm);
            detune.SetMix(mx * gm);
            // LEITURA MONO: metade do custo de duas leituras estereo.
            const float a = pitch_det.ReadMono(0);
            const float b = detune.Duas() ? pitch_det.ReadMono(1) : 0.f;
            detune.Mistura(a, b, out[0][i], out[1][i]);
        }
        if(!det)
        {
            pitch_det.SetActive(0, false);
            pitch_det.SetActive(1, false);
        }
    }

    // 4.4) FREEZE — PÓS-DELAY, PRÉ-REVERB.
    //
    // A posição é a decisão musical inteira. Antes do reverb, o
    // congelado ganha a sala JUNTO com o resto — pertence ao mesmo
    // lugar que o instrumento vivo. Depois do reverb, ele viria seco
    // por cima de uma sala que continua molhando o vivo, e soaria
    // colado, não pertencente.
    //
    // Isto roda a cada bloco mesmo com `frz_on` em 0: enquanto não
    // congela, o freeze é um gravador silencioso que não toca no
    // áudio. Ele PRECISA gravar sempre — congelar só o que já passou
    // é a única forma de o gesto pegar o som que acabou de acontecer.
#if COTUCA_FREEZE_ENABLED
    // CTOS: o freeze e EFEITO, entao roda sob veto. Ver docs/CTOS.md.
    if(freeze_ok && !ctos_bypass_frz)
    {
        const uint32_t t0 = cotuca::hal::NowTicks();
        freeze.SetMix(params.Real(manifest.p_frz_mix));
        freeze.SetSize(params.Real(manifest.p_frz_size));
        // Borda decidida DENTRO do freeze (SetFrozen ignora repetição),
        // então empurrar o estado a cada bloco é barato e não cria
        // corrida: quem arma e quem consome são o mesmo contexto.
        freeze.SetFrozen(params.Real(manifest.p_frz_on) >= 0.5f);
        for(size_t i = 0; i < size; i++)
            freeze.Process(out[0][i], out[1][i]);

        // SUBTRAI EM TICKS, converte DEPOIS. O contador ocupa os 32 bits
        // inteiros, entao a volta se corrige sozinha na subtracao
        // uint32 — que e exatamente o que `System::GetUs()` impedia ao
        // dividir antes.
        //
        // O FILTRO DE SANIDADE POR MAGNITUDE FOI REMOVIDO, e de
        // proposito. Ele descartava tudo acima de 10x o orcamento —
        // que e EXATAMENTE como um estouro catastrofico apareceria.
        // Ajustar o limiar so moveria a cegueira. Com o relogio certo,
        // um numero grande e um estouro DE VERDADE e tem que vetar.
        const uint32_t gasto
            = (cotuca::hal::NowTicks() - t0) / cotuca::hal::TicksPerUs();
        const uint32_t prazo
            = (uint32_t)(1e6f * (float)size
                         / cotuca::hal::Info().sample_rate);
        ctos_orcamento_us = (uint32_t)(kEfeitoFracao * (float)prazo);
        if(gasto > ctos_pico_us)
            ctos_pico_us = gasto;
        if(gasto > ctos_orcamento_us)
        {
            // VETO. Aqui mesmo, no contexto que mediu.
            ctos_ocorrencias++;
            if(!ctos_bypass_frz)
            {
                ctos_bypass_frz = true;
                ctos_bypass_ms  = cotuca::hal::NowMs();
            }
        }
    }
#endif

    // 4.5) REVERB — PÓS-FX2, PRÉ-LEVEL.
    //
    // Aqui molha OS DOIS RAMOS, que é o que se quer de uma sala: ela
    // envolve o instrumento inteiro, não um canal dele. E vem antes do
    // level porque o master tem que mandar no resultado final, cauda
    // incluída — reverb depois do level faria o knob de volume mudar a
    // proporção seco/molhado, que é comportamento de pedal quebrado.
    //
    // >>> PLANO B, se o laço acústico do exciter engordar: mover pra
    // >>> molhar SÓ o OUT_R (jack de linha), deixando o exciter seco.
    // >>> Cauda no exciter volta pelo piezo e realimenta o próprio
    // >>> reverb — é o tipo de laço que cresce devagar e surpreende no
    // >>> palco. O governor e o shifter seguram, mas segurar não é o
    // >>> mesmo que estar certo. Se acontecer, o conserto é trocar as
    // >>> duas linhas abaixo por uma que só passe out[1].
    if(reverb_ok)
    {
        reverb.SetMix(params.Real(manifest.p_rev_mix));
        reverb.SetSize(params.Real(manifest.p_rev_size));
        reverb.SetDamp(params.Real(manifest.p_rev_damp));

        // ---- SHIMMER: o laco fecha AQUI DENTRO ----
        //
        //   cauda -> pitch(+12/+7) -> passa-alta 200 Hz -> x feedback
        //         -> injeta na ENTRADA do reverb -> cauda ...
        //
        // Grava-se SEMPRE (regra dos efeitos com historico: bypass
        // pula o processamento, nunca a gravacao — foi o que causou o
        // clique na Peca 2). O que o slot liga e desliga e o laco.
        // A cronometragem cerca SO o shimmer. A primeira versao envolvia
        // o `reverb.Process()` junto e media 299 us — atribuindo ao
        // efeito o custo da BASE. E o mesmo erro que o `NowUs()` foi
        // criado pra evitar: sem separar, o veto pune o inocente.
        uint32_t shm_ticks = 0;
        const bool shm = pitch_tail_ok && shimmer_slot.Ativo();
        // CHAVE PROPRIA, nao "mix > 0". Fundir as duas decisoes obriga
        // a zerar o mix pra desligar, e o ajuste se perde.
        shimmer_slot.SetOn(params.Real(manifest.p_shm_on) >= 0.5f);
        if(shm)
        {
            shimmer_slot.Comeca(cotuca::hal::NowTicks());
            shimmer.SetMix(params.Real(manifest.p_shm_mix));
            shimmer.SetFeedback(params.Real(manifest.p_shm_feedback));
            shimmer.SetTone(params.Real(manifest.p_shm_tone));
            const int   modo = (int)(params.Real(manifest.p_shm_shift) + 0.5f);
            (void)modo;
            // +12 = 2.0 ; +7 = 1.498 (quinta justa temperada)
            pitch_tail.SetRatio(0, 2.0f);
            pitch_tail.SetRatio(1, 1.498307f);
            pitch_tail.SetActive(0, modo == 0 || modo == 2);
            pitch_tail.SetActive(1, modo == 1 || modo == 2);
        }

        for(size_t i = 0; i < size; i++)
        {
            float pl = 0.f, pr = 0.f, gm = 0.f;
            if(shm)
            {
                const uint32_t t0 = cotuca::hal::NowTicks();
                float al = 0.f, ar = 0.f, bl = 0.f, br = 0.f;
                pitch_tail.Read(0, al, ar);
                pitch_tail.Read(1, bl, br);
                // No modo "ambos", meio ganho cada: duas vozes somadas
                // a ganho cheio dobrariam a energia do laco e o teto de
                // 0.85 deixaria de ser teto.
                const float k = (pitch_tail.Active(0) && pitch_tail.Active(1))
                                    ? 0.5f : 1.f;
                pl = (al + bl) * k;
                pr = (ar + br) * k;
                // TOM ANTES DOS DOIS USOS. O ouvido soma a realimentacao
                // E a camada direta; filtrar so uma delas deixa metade da
                // aspereza no ar. Filtrado aqui, `pl` ja segue tratado
                // pros dois caminhos.
                shimmer.Tom(pl, pr);
                float add_l, add_r;
                shimmer.Realimenta(pl, pr, add_l, add_r);
                float gs;
                shimmer_slot.Passo(gs, gm);
                out[0][i] += add_l * gm;   // injeta no reverb
                out[1][i] += add_r * gm;
                shm_ticks += cotuca::hal::NowTicks() - t0;
            }

            reverb.Process(out[0][i], out[1][i]);

            // GRAVA A CAUDA ANTES DA CAMADA DIRETA. Se a camada
            // entrasse no buffer, ela realimentaria por fora do laco do
            // reverb e o `feedback` deixaria de ser o unico dono do
            // ganho de laco.
            if(pitch_tail_ok) pitch_tail.Write(out[0][i], out[1][i]);

            if(shm)
            {
                // CAMADA DIRETA — e o que da autoridade ao `shm_mix`.
                // A versao anterior nunca somava nada por mix: ele so
                // ligava o slot, e todo o nivel vinha do laco. Dai
                // "mix no maximo e ganho baixo": o controle nao
                // controlava.
                const float m = shimmer.Mix() * gm;
                out[0][i] += pl * m;
                out[1][i] += pr * m;
            }
        }

        if(shm)
        {
            const uint32_t prazo
                = (uint32_t)(1e6f * (float)size
                             / cotuca::hal::Info().sample_rate);
            const uint32_t us = shm_ticks / cotuca::hal::TicksPerUs();
            if(us > shimmer_pico) shimmer_pico = us;
            if(us > (uint32_t)(0.5f * (float)prazo))
            {
                params.Set(manifest.p_shm_on, 0.f);
                shimmer_slot.SetOn(false);
            }
        }
        if(pitch_tail_ok && !shimmer_slot.Ativo())
        {
            pitch_tail.SetActive(0, false);
            pitch_tail.SetActive(1, false);
        }
    }

    // ---- HISTOGRAMA DE PICO POR NOTA (bancada) ----
    //
    // "Um grave está estourando" precisa virar "QUAIS notas e QUANTO",
    // senao o filtro e chute. O voc_pitch e quantizado em semitons e a
    // cerca cobre 110–440 Hz, entao da pra medir NOTA A NOTA: cada
    // bloco vai pro balde do pitch corrente, guardando o PICO ali.
    //
    // Mede a saida do reverb, ANTES do level e do limiter: e o sinal
    // que o limiter vai ter que segurar. Medir depois dele mostraria
    // 0.95 em tudo — o limiter escondendo justamente o que se procura.
    {
        // RESOLUCAO: 12 baldes cobrindo a CERCA dos buzios [0.5, 1.0],
        // que sao 24 semitons — 2 semitons por balde, nao 4. A versao
        // anterior dividia a faixa INTEIRA (0..1, 48 semitons) em 12, e
        // cada rotulo cobria 4 semitons: grosseiro demais pra decidir
        // entre notch e filtro.
        const float pn = params.Get(manifest.p_voc_pitch);
        int   b = (int)((pn - 0.5f) * 24.f + 0.5f);
        if(b < 0) b = 0; else if(b > 12) b = 12;
        float pk = 0.f;
        for(size_t i = 0; i < size; i++)
        {
            const float a = out[0][i] < 0 ? -out[0][i] : out[0][i];
            if(a > pk) pk = a;
        }
        if(pk > note_peak[b])
        {
            note_peak[b] = pk;
            cotuca::hal::BlackboxWrite(32 + b, pk);
        }
    }

    // 5) LEVEL — master de saída, ANTES da dinâmica de saída: é
    //    justamente o level exagerado que o governor e o limiter
    //    precisam segurar.
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] *= level;
        out[1][i] *= level;
    }

    // 5b) LARGURA — mid/side, e tem que ser AQUI.
    //
    //   mid = (L+R)/2   side = (L-R)/2   L = mid+w*side   R = mid-w*side
    //
    // DEPOIS DO LEVEL E ANTES DO SPLIT, que é o ÚLTIMO ponto em que os
    // dois canais ainda são a mesma imagem estéreo. Passado o split,
    // OUT_L é o exciter (com governor e shifter SSB, porque realimenta
    // o piezo) e OUT_R é o jack de linha limpo: são dois DESTINOS, não
    // um par estéreo, e somar/subtrair os dois ali misturaria o ramo
    // protegido com o desprotegido — desfazendo a proteção
    // anti-feedback justamente no fio que fecha o laço acústico.
    //
    // A largura vem do reverb, do shimmer e do detune, que são os
    // únicos que produzem estéreo nativo. Em 1.0 a conta é identidade,
    // então o bloco inteiro é pulado: custo literalmente zero no
    // neutro.
    {
        const float alvo_w = params.Real(manifest.p_out_width);
        if(alvo_w < 0.9999f || suave_width.Valor() < 0.9999f)
        {
            for(size_t i = 0; i < size; i++)
            {
                const float w = suave_width.Passo(alvo_w);
                const float m = (out[0][i] + out[1][i]) * 0.5f;
                const float s = (out[0][i] - out[1][i]) * 0.5f * w;
                out[0][i] = m + s;
                out[1][i] = m - s;
            }
        }
    }

    // 6) SPLIT — os ramos deixam de ser iguais aqui.
    //    OUT_L vai pro exciter no corpo da escultura e é o ramo que
    //    realimenta o piezo: leva o governor. OUT_R é jack de linha,
    //    não realimenta, e não leva.
    // SHIFTER — anti-feedback, entre o split e o governor, SÓ no ramo
    // do exciter. O jack de linha sai SEM shift por design: quem
    // grava ou manda pra PA quer o sinal do instrumento, não o sinal
    // protegido contra um laço acústico que não existe do lado de lá.
    //
    // Antes do governor de propósito: o governor deve reagir ao sinal
    // que de fato vai pro exciter, já deslocado.
    shift_l.SetShift(params.Real(manifest.p_shift_hz));
    for(size_t i = 0; i < size; i++)
        out[0][i] = shift_l.Process(out[0][i]);

    gov_l.SetCeiling(params.Real(manifest.p_gov_ceiling));
    gov_l.SetRelease(params.Real(manifest.p_gov_release));
    for(size_t i = 0; i < size; i++)
        out[0][i] = gov_l.Process(out[0][i]);

    // 6b) BENCH BOOST — FERRAMENTA DE BANCADA, só no ramo do exciter,
    //     imediatamente antes do limiter. Ver o bloco de aviso em
    //     manifest.h: nunca vai a knob nem a cena, e nasce em 1.0.
    const float boost = params.Real(manifest.p_bench_boost);
    if(boost != 1.0f)
        for(size_t i = 0; i < size; i++)
            out[0][i] *= boost;

    // 7) LIMITER — último bloco de processamento em CADA ramo.
    //    Um limiter por ramo, não um compartilhado: um pico no
    //    exciter não pode abaixar o jack de linha.
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = lim_l.Process(out[0][i]);
        out[1][i] = lim_r.Process(out[1][i]);
    }

    // 8) ROTEAMENTO — só liga/desliga o ramo. Depois do limiter
    //    porque mutar só pode reduzir: nada aqui consegue burlar o
    //    teto.
    const float r_exc = params.Real(manifest.p_route_exciter) >= 0.5f ? 1.f : 0.f;
    const float r_jck = params.Real(manifest.p_route_jack) >= 0.5f ? 1.f : 0.f;
    if(r_exc < 0.5f || r_jck < 0.5f)
    {
        for(size_t i = 0; i < size; i++)
        {
            out[0][i] *= r_exc;
            out[1][i] *= r_jck;
        }
    }
    // Autopsia por LED: daqui, porque este contexto sobrevive ao loop.
    LedFaseTick(size, cotuca::hal::Info().sample_rate);

    // ---- TETO DE SOMA: mede o bloco inteiro e decide ----
    {
        const uint32_t tpu = cotuca::hal::TicksPerUs();
        if(tpu)
        {
            // Subtrai em TICKS e converte depois: dividir antes faz a
            // volta do contador deixar de se corrigir sozinha. Mesma
            // armadilha que derrubou o watchdog em 4 277 071 629 us.
            soma_us = (cotuca::hal::NowTicks() - t_bloco0) / tpu;
            if(soma_us > soma_pico_us) soma_pico_us = soma_us;

            const uint32_t prazo
                = (uint32_t)(1e6f * (float)size
                             / cotuca::hal::Info().sample_rate);
            soma_teto_us = soma_teto_forc ? soma_teto_forc
                                          : (uint32_t)(kTetoFrac * (float)prazo);

            if(soma_carencia)
            {
                // Em carencia nao se mede pra decidir: a medicao ainda
                // contem o efeito que esta descendo.
                soma_carencia--;
                soma_seguidos = 0;
            }
            else if(soma_us > soma_teto_us)
            {
                // BLOCOS SEGUIDOS, nao um. O primeiro bloco de qualquer
                // efeito e caro (estreia), e derrubar por causa dele
                // puniria o ato de LIGAR. So o regime conta.
                if(++soma_seguidos >= kBlocosSeguidos)
                {
                    soma_seguidos = 0;
                    // Procura o de MENOR prioridade que esteja ligado.
                    int alvo = -1;
                    for(int k = 0; k < manifest.n_sacrif; k++)
                        if(!soma_caiu[k]
                           && params.Real(manifest.sacrif[k].param_on) >= 0.5f)
                        { alvo = k; break; }

                    if(alvo >= 0)
                    {
                        soma_caiu[alvo]    = true;
                        soma_caiu_ms[alvo] = cotuca::hal::NowMs();
                        soma_caiu_us[alvo] = soma_us;
                        params.Set(manifest.sacrif[alvo].param_on, 0.f);
                        soma_quedas++;
                        // AVISO OBRIGATORIO — mas MARCADO, nao falado.
                        // Derrubar em silencio seria a familia "falha em
                        // silencio"; falar daqui mata a serial.
                        aviso_teto_idx   = alvo;
                        aviso_teto_us    = soma_us;
                        aviso_teto_prazo = prazo;
                        // Espera a rampa descer antes de julgar de novo.
                        soma_carencia    = kCarenciaBlocos;
                    }
                    else if(!soma_sem_saida)
                    {
                        soma_sem_saida  = true;
                        aviso_sem_saida = true;
                        aviso_teto_us   = soma_us;
                    }
                }
            }
            else if(soma_seguidos)
                soma_seguidos--;   // volta devagar: nao zera num bloco bom
        }
    }
}

// B1: sorteia novos valores pros params da whitelist do manifest.
// Só os de CARÁTER — nunca level, fonte, proteção ou roteamento. O
// porquê de cada proibição está no manifest.h.
// Sorteia UM valor dentro da cerca de um alvo; com máscara, pousa só
// em graus da escala. O valor vai pela MESMA via dos knobs — Set() no
// loop, Shaped() empurrado ao módulo com o motor vivo — via que o
// nudge de voc_pitch já provou propagar ao vivo (T12d).
static float DrawTarget(const cotuca::RandTarget& rt)
{
    if(rt.n_scale > 0 && rt.norm_per_semi > 0.f)
    {
        // semitons dentro da cerca E dentro da escala
        int allowed[49];
        int n = 0;
        const int lo
            = (int)ceilf((rt.min_norm - rt.center_norm) / rt.norm_per_semi);
        const int hi
            = (int)floorf((rt.max_norm - rt.center_norm) / rt.norm_per_semi);
        for(int st = lo; st <= hi && n < 49; st++)
        {
            const int grau = ((st % 12) + 12) % 12;
            for(int k = 0; k < rt.n_scale; k++)
                if(rt.scale[k] == grau)
                {
                    allowed[n++] = st;
                    break;
                }
        }
        if(n == 0)
            return rt.center_norm; // cerca sem grau nenhum: fica no tom
        const int pick = (int)(RandNorm() * (float)n) % n;
        return rt.center_norm + (float)allowed[pick] * rt.norm_per_semi;
    }
    if(rt.real_fence)
    {
        // uniforme na UNIDADE do param; a curva é invertida depois.
        float real;
        if(rt.real_step > 0.f)
        {
            // degraus: min + k*passo, k sorteado
            const int n = 1 + (int)((rt.max_norm - rt.min_norm)
                                    / rt.real_step + 0.001f);
            const int k = (int)(RandNorm() * (float)n) % n;
            real        = rt.min_norm + (float)k * rt.real_step;
        }
        else
            real = rt.min_norm + RandNorm() * (rt.max_norm - rt.min_norm);
        return params.NormFromReal(rt.id, real);
    }
    return rt.min_norm + RandNorm() * (rt.max_norm - rt.min_norm);
}

static void TriggerRandomize(bool autorand)
{
    using namespace cotuca;
    n_ramps = 0;
    for(int i = 0; i < manifest.n_randomizable
                   && n_ramps < (int)(sizeof(ramps) / sizeof(ramps[0]));
        i++)
    {
        const RandTarget& rt = manifest.randomizable[i];
        ramps[n_ramps].id    = rt.id;
        ramps[n_ramps].from  = params.Get(rt.id);
        ramps[n_ramps].to    = DrawTarget(rt);
        ramps[n_ramps].start_ms = hal::NowMs();
        ramps[n_ramps].dur_ms   = kRampMs;
        ramps[n_ramps].done     = false;
        // Caixa-preta, slots 4..7 rotativos: id na parte inteira, norm
        // na fracao. E o unico registro que sobrevive quando a serial
        // ja caiu — que e o estado normal com o vocoder ligado.
        static int bb_rot = 0;
        hal::BlackboxWrite(4 + bb_rot,
                           (float)rt.id + ramps[n_ramps].to * 0.999f);
        bb_rot = (bb_rot + 1) & 3;
        n_ramps++;
    }
    if(n_ramps == 0)
    {
        hal::Log("%s: nada na whitelist deste manifest",
                 autorand ? "buzios" : "B1");
        return;
    }
    // log dos valores sorteados — enquanto a serial viver, e o probe
    // de bancada; depois dela, sobra a caixa-preta.
    char linha[96];
    int  off = 0;
    for(int i = 0; i < n_ramps && off < (int)sizeof(linha) - 16; i++)
        off += snprintf(linha + off, sizeof(linha) - off, " %s=%.2f",
                        params.Name(ramps[i].id), (double)ramps[i].to);
    hal::Log("%s:%s", autorand ? "buzios" : "B1", linha);
}

// B2: momentâneo. Guarda o valor, força 1, devolve ao soltar.
static void SetButton2(bool pressed)
{
    using namespace cotuca;
    const int t = manifest.button2_target;
    if(t < 0)
        return; // sem função neste instrumento
    if(pressed && !b2_held)
    {
        b2_saved = params.Get(t);
        params.Set(t, 1.f);
        b2_held = true;
        hal::Log("B2: %s = 1 (segurando)", params.Name(t));
    }
    else if(!pressed && b2_held)
    {
        params.Set(t, b2_saved);
        b2_held = false;
        hal::Log("B2: %s de volta a %.3f", params.Name(t), b2_saved);
    }
}

// ---- painel falso via serial ----
// TODO COMANDO DE STATUS DENUNCIA OS MODOS QUE SILENCIAM.
// Um rotulo que mente (cicatriz 7) e uma medicao que mede o proprio
// silencio (cicatriz 8) tem o mesmo antidoto: o instrumento declara os
// proprios modos, e a declaracao atravessa o modo que declara.
// ---- CAMADA 1: OS TRES LEDS DO WIDI, MEDIDOS SEM SEREM NOMEADOS ----
//
// Ideia do dB: em vez de um LED pra humano olhar, os pads vao pra pinos
// de entrada e o instrumento le o proprio estado de link.
//
// A PRIMEIRA VERSAO CLASSIFICAVA, E ESTAVA ERRADA. Eu escrevi
// "fixo = conectado, piscando = anunciando" a partir da legenda da CME
// — que estava marcada no codigo como NAO VERIFICADA — e o dB desmentiu
// por medicao: desconectou o iPad e o LB continuou FIXO. Antes disso o
// classificador ja trocava de estado 15 vezes em 90 s ENQUANTO o MIDI
// fluia, o que nao e estado de link: e atividade.
//
// Entao aqui nao ha nome, so numero: transicoes e duty de CADA pad, por
// janela de 2 s, correlacionados com trafego. A legenda sai da tabela
// que o `ble raw` produzir — medir vem antes de nomear.
static constexpr bool kLbAcesoEmBaixo = true; // confirmado: V3=3V3, LB=0V

static volatile uint32_t lb_trans[3] = {0, 0, 0};
static volatile uint8_t  lb_duty[3]  = {0, 0, 0};
static bool              lb_cru      = false;
static bool              lb_espelho  = false;

static void LbTick()
{
    using namespace cotuca;
    static uint32_t prox_ms = 0, jan0 = 0;
    static bool     ant[3]  = {false, false, false};
    static bool     primeira = true;
    static uint32_t n_trans[3] = {0, 0, 0}, n_aceso[3] = {0, 0, 0};
    static uint32_t n_amostras = 0;

    const uint32_t ag = hal::NowMs();
    if(ag < prox_ms)
        return;
    prox_ms = ag + 5u; // 200 Hz

    bool aceso[3];
    for(int i = 0; i < 3; i++)
    {
        const bool nivel = hal::ReadWidiLed(i);
        aceso[i] = kLbAcesoEmBaixo ? !nivel : nivel;
        if(primeira) ant[i] = aceso[i];
        if(aceso[i] != ant[i]) { n_trans[i]++; ant[i] = aceso[i]; }
        if(aceso[i]) n_aceso[i]++;
    }
    if(primeira) { jan0 = ag; primeira = false; }
    n_amostras++;
    if(lb_espelho)
        hal::SetLed(aceso[0]);

    if(ag - jan0 < 2000u)
        return;

    for(int i = 0; i < 3; i++)
    {
        lb_trans[i] = n_trans[i];
        lb_duty[i]  = (uint8_t)(n_amostras ? (n_aceso[i] * 100u / n_amostras)
                                           : 0u);
    }
    if(lb_cru)
    {
        const bool traf
            = ble_last_rx_ms && (ag - ble_last_rx_ms) < 2000u;
        hal::Log("lb: LB %lu/%u%%  LG %lu/%u%%  LR %lu/%u%%  | trafego %s",
                 (unsigned long)lb_trans[0], (unsigned)lb_duty[0],
                 (unsigned long)lb_trans[1], (unsigned)lb_duty[1],
                 (unsigned long)lb_trans[2], (unsigned)lb_duty[2],
                 traf ? "SIM" : "nao");
    }
    jan0 = ag; n_amostras = 0;
    for(int i = 0; i < 3; i++) { n_trans[i] = 0; n_aceso[i] = 0; }
}

static void declara_modos()
{
    using namespace cotuca;
    if(hal::LogMuted())
        hal::LogAlways("[LOG MUDO] o log esta silenciado — `log on` destrava."
                       " O que voce nao esta vendo NAO e ausencia de dado.");
}

static void HandleLine(const char* line)
{
    using namespace cotuca;
    if(line[0] == 'k' && line[1] >= '1' && line[1] <= '5')
    {
        int   idx = line[1] - '1';
        float v   = strtof(line + 2, nullptr);
        manifest.knob[idx].Apply(params, v);
        hal::Log("knob %d -> %.2f", idx + 1, v);
        return;
    }
    // b1 — dispara o randomizador (gesto momentâneo, um sorteio)
    if(strncmp(line, "b1", 2) == 0)
    {
        TriggerRandomize(false);
        return;
    }
    // b2 [0|1] — simula segurar/soltar; sem argumento, alterna
    if(strncmp(line, "b2", 2) == 0)
    {
        const char* a = line + 2;
        while(*a == ' ') a++;
        bool want = !b2_sim;
        if(*a == '0') want = false;
        else if(*a == '1') want = true;
        b2_sim = want;
        if(manifest.button2_target < 0)
            hal::Log("B2 simulado: %s (mas button2_target = -1, sem efeito)",
                     want ? "PRESSIONADO" : "solto");
        else
            hal::Log("B2 simulado: %s", want ? "PRESSIONADO" : "solto");
        return;
    }
    if(strncmp(line, "tone on", 7) == 0)
    { params.Set(manifest.p_tone_mix, 0.5f); hal::Log("tone ON"); return; }
    if(strncmp(line, "tone off", 8) == 0)
    { params.Set(manifest.p_tone_mix, 0.0f); hal::Log("tone OFF"); return; }
    if(strncmp(line, "board", 5) == 0)
    {
        declara_modos();
        const auto& b = hal::Info();
        // VERSAO PRIMEIRO. A rotina de conferir "o fonte e a placa
        // batem?" so funciona se a placa disser a versao a qualquer
        // momento — e nao apenas no banner de boot, que se perde pra
        // quem conecta depois. Foi o primeiro buraco que a propria
        // rotina encontrou, minutos depois de ser escrita.
        hal::Log("Cotucatronics OS v" COTUCA_OS_VERSION);
        // ESTADO DAS TABELAS NO `board`, nao so no boot. A denuncia de
        // estouro existia desde a v0.70 e nao evitou a segunda
        // ocorrencia — porque so saia no banner, e quem conecta a
        // serial depois nunca a ve. Aviso que exige estar presente na
        // hora certa e aviso que se perde.
        hal::Log("tabelas: %d/%d params | %d/%d CCs%s",
                 params.Count(), (int)cotuca::kMaxParams,
                 manifest.n_midi, (int)cotuca::kMaxMidiMap,
                 (params.Overflow() || manifest.midi_overflow)
                     ? "   *** ESTOUROU ***" : "");
        hal::Log("%s | knobs:%d | in:%s | %.0fHz/blk%d",
                 b.name, b.num_knobs, b.has_audio_in ? "sim" : "nao",
                 b.sample_rate, (int)b.block_size);
        hal::Log("  cpu %lu MHz | prazo do bloco %.0f us",
                 (unsigned long)(b.cpu_hz / 1000000u),
                 1e6f * (float)b.block_size / b.sample_rate);
        return;
    }
    if(strncmp(line, "set ", 4) == 0)
    {
        // set <nome> <valor REAL> — alcança QUALQUER parâmetro pelo
        // nome, inclusive os sem knob (tone_freq e tone_mix são
        // serial-only por design: o tom é bancada, não performance).
        //
        // O VALOR É REAL, na unidade do parâmetro: Hz, ms, ×.
        // Antes era normalizado, e isso invertia a intuição em faixa
        // que não começa em zero — `set bench_boost 1.0` parecia "sem
        // boost" e entregava 8×, o máximo, num parâmetro que existe
        // justamente pra sobrecarregar a saída do exciter.
        // Knob e macro seguem normalizados: posição de curso é
        // normalizada por natureza. Quem digita, digita em unidade.
        const char* p = line + 4;
        while(*p == ' ') p++;
        const char* sp = strchr(p, ' ');
        if(!sp || sp == p)
        { hal::Log("uso: set <nome> <valor real> — veja list"); return; }

        char         name[24];
        const size_t n = (size_t)(sp - p) < sizeof(name) - 1
                             ? (size_t)(sp - p)
                             : sizeof(name) - 1;
        memcpy(name, p, n);
        name[n] = '\0';

        const int id = params.Find(name);
        if(id < 0)
        { hal::Log("? param \"%s\" nao existe (use list)", name); return; }

        const float v = strtof(sp + 1, nullptr);
        if(!params.SetReal(id, v))
        {
            hal::Log("fora da faixa: %s aceita [%.3f .. %.3f]",
                     params.Name(id), params.Min(id), params.Max(id));
            return;
        }
        hal::Log("%s -> real=%.3f (norm=%.3f, curve %.1f)",
                 params.Name(id), params.Real(id), params.Get(id),
                 params.Curve(id));
        return;
    }
    if(strncmp(line, "dyn", 3) == 0)
    {
        hal::Log("gate  thr=%.4f rel=%.0fms gain=%.2f env=%.4f",
                 params.Real(manifest.p_gate_thresh),
                 params.Real(manifest.p_gate_release),
                 gate.Gain(), gate.Env());
        hal::Log("gov   ceil=%.2f rel=%.0fms gain=%.2f env=%.3f",
                 params.Real(manifest.p_gov_ceiling),
                 params.Real(manifest.p_gov_release),
                 gov_l.Gain(), gov_l.Env());
        hal::Log("lim   ceil=%.2f gainL=%.2f gainR=%.2f",
                 cotuca::Limiter::Ceiling(), lim_l.Gain(), lim_r.Gain());
        {
            const float b = 1e6f * (float)hal::Info().block_size
                            / hal::Info().sample_rate;
            hal::Log("estreia %.0f us  (regime abaixo)",
                     fx2_startup_peak * b);
        }
        hal::Log("fx2   %s", fx2_overrun
                     ? "BYPASS FORCADO — estourou o prazo do bloco"
                     : (params.Real(manifest.p_fx2_on) >= 0.5f
                            ? "ATIVO" : "bypass (set fx2_on 1)"));
        hal::Log("shift %.1f Hz (SSB, so no ramo exciter)", shift_l.Shift());
        hal::Log("boost %.2fx  (BANCADA — nasce sempre em 1.00)",
                 params.Real(manifest.p_bench_boost));
        hal::Log("route exciter=%d jack=%d",
                 params.Real(manifest.p_route_exciter) >= 0.5f ? 1 : 0,
                 params.Real(manifest.p_route_jack) >= 0.5f ? 1 : 0);
        return;
    }
    // CAIXA-PRETA do voo anterior. E o unico jeito de saber o que o
    // vocoder fez depois que a serial caiu: ele grava durante o voo,
    // a gente le aqui, com o Fx2 desligado e o USB de novo vivo.
    // `bb arm` — arma o gravador e SILENCIA o log ANTES do voo, nunca
    // no meio de uma linha. Conserta a raiz da assimetria que fez os
    // contadores pareados mentirem.
    if(strncmp(line, "bb arm", 6) == 0)
    {
        hal::BlackboxArm();
        hal::LogMute(true);
        hal::Log("caixa-preta armada; log mudo ate o proximo reset");
        return;
    }
    if(strncmp(line, "bb", 2) == 0 && (line[2] == 0 || line[2] == ' '))
    {
        if(!hal::BlackboxValid())
        {
            hal::Log("caixa-preta: vazia (ligou agora, sem voo anterior)");
            return;
        }
        const float pi_ = hal::BlackboxRead(0);
        const float po_ = hal::BlackboxRead(1);
        hal::Log("caixa-preta do voo anterior:");
        hal::Log("  blocos processados : %.0f", (double)hal::BlackboxRead(2));
        hal::Log("  pico ENTRADA  do fx2: %.5f", (double)pi_);
        hal::Log("  pico SAIDA    do fx2: %.5f  (antes do ganho)", (double)po_);
        if(po_ > 1e-7f)
            hal::Log("  => falta %.0fx de ganho pra casar com a entrada",
                     (double)(pi_ / po_));
        else if(pi_ > 1e-7f)
            hal::Log("  => saida ZERO com entrada viva: modulo MUDO");
        hal::Log("  pico de CPU: %.0f%%", (double)hal::BlackboxRead(3) * 100.0);
        // slots 4..7: ultimos sorteios do búzios/B1 — id na parte
        // inteira, norm na fracao. Zero = slot nunca usado.
        for(int sl = 4; sl < 8; sl++)
        {
            const float v = hal::BlackboxRead(sl);
            if(v <= 0.f) continue;
            const int   pid  = (int)v;
            const float norm = v - (float)pid;
            hal::Log("  sorteio[%d]: %s = %.2f", sl - 4,
                     params.Name(pid), (double)norm);
        }
        hal::Log("  buzios: env pico=%.4f, disparos=%.0f, consumidos=%.0f",
                 (double)hal::BlackboxRead(8), (double)hal::BlackboxRead(9),
                 (double)hal::BlackboxRead(11));
        hal::Log("  loop: batimento=%.0f ticks, ultima fase=%.0f",
                 (double)hal::BlackboxRead(10), (double)hal::BlackboxRead(12));
        hal::Log("  fases: 1 knobs 2 serial 3 botoes 4 sorteio 5 rampas"
                 " 6 midi 7 watchdog 8 push-modulo");
        hal::Log("  --- PICO POR NOTA (pre-level/limiter) ---");
        for(int b = 0; b <= 12; b++)
        {
            const float pk = hal::BlackboxRead(32 + b);
            if(pk <= 0.f) continue;
            const float pn = 0.5f + (float)b / 24.f;
            const float hz = powf(2.f, (pn * 48.f - 24.f) / 12.f) * 110.f;
            hal::Log("   %6.1f Hz : pico %.3f %s", (double)hz, (double)pk,
                     pk > 0.95f ? "<<< ESTOURA" : (pk > 0.7f ? "<< quente" : ""));
        }
        hal::Log("  --- MIDI no voo ---");
        hal::Log("   bytes %.0f | msgs %.0f | aplicadas %.0f"
                 " | ressync %.0f | fila perdeu %.0f",
                 (double)hal::BlackboxRead(33), (double)hal::BlackboxRead(34),
                 (double)hal::BlackboxRead(35), (double)hal::BlackboxRead(36),
                 (double)hal::BlackboxRead(37));
        hal::Log("   --- diario do link ---");
        {
            char l[110]; int off=0;
            for(int m = 0; m < 12 && off < 90; m++)
                off += snprintf(l+off, sizeof(l)-off, " %.0f",
                                (double)hal::BlackboxRead(40+m));
            hal::Log("   msgs por minuto:%s", l);
        }
        {
            const int nt = (int)hal::BlackboxRead(84);
            // "Nenhuma transicao" e AMBIGUO e a primeira versao mentia:
            // silencio total e link estavel davam a MESMA resposta,
            // porque o detector so registra MUDANCA e o estado inicial
            // ja e "nao recebendo". O total de msgs desempata.
            const float tot = hal::BlackboxRead(34);
            if(nt == 0 && tot <= 0.f)
                hal::Log("   transicoes: nenhuma, e ZERO MSGS — nada chegou"
                         " no voo (ou a energia caiu e apagou o registro)");
            else if(nt == 0)
                hal::Log("   transicoes: nenhuma, com %.0f msgs —"
                         " link CONSTANTE o voo todo", (double)tot);
            for(int i = 0; i < nt && i < 16; i++)
                hal::Log("   t=%.0fs  %s", (double)hal::BlackboxRead(52+i*2),
                         hal::BlackboxRead(53+i*2) > 0.5f ? "VOLTOU" : "calou");
        }
        hal::Log("   erros de uart %.0f | rearmes %.0f",
                 (double)hal::BlackboxRead(38), (double)hal::BlackboxRead(39));
        hal::Log("  TOKEN de linha no fim do voo=%.0f  (1 = PRESA)",
                 (double)hal::BlackboxRead(30));
        hal::Log("  ultimo Log chamado comecava com '%c'",
                 (char)(int)hal::BlackboxRead(31));
        hal::Log("  VOLTAS completas=%.0f, ultimo carimbo=%.0f ms",
                 (double)hal::BlackboxRead(28), (double)hal::BlackboxRead(29));
        hal::Log("  fila de eventos RNBO (max)=%.0f",
                 (double)hal::BlackboxRead(27));
        hal::Log("  cdc: declarado morto no voo? %s",
                 hal::BlackboxRead(26) > 0.5f ? "SIM" : "nao");
        hal::Log("  autopsia: M1=%.0f (entrou cmd) M2=%.0f (saiu cmd)"
                 " M3=%.0f (entrou TX) | log descartados=%.0f",
                 (double)hal::BlackboxRead(22), (double)hal::BlackboxRead(23),
                 (double)hal::BlackboxRead(24), (double)hal::BlackboxRead(25));
        hal::Log("  push-laco: iteracoes=%.0f | bind0 shaped=%.4f last=%.4f",
                 (double)hal::BlackboxRead(19), (double)hal::BlackboxRead(20),
                 (double)hal::BlackboxRead(21));
        hal::Log("  rampa: passos=%.0f, escrito=%.4f, lido=%.4f",
                 (double)hal::BlackboxRead(16), (double)hal::BlackboxRead(17),
                 (double)hal::BlackboxRead(18));
        hal::Log("  push: instrument=%.0f fx2=%.0f | ultimo voc_pitch=%.4f",
                 (double)hal::BlackboxRead(13), (double)hal::BlackboxRead(14),
                 (double)hal::BlackboxRead(15));
        return;
    }
    if(strncmp(line, "probe", 5) == 0)
    {
        // Uma linha POR BLOCO, em resposta a comando. Quatro Logs em
        // rajada a 67% de CPU perdiam as linhas do meio: o loop
        // principal ganha pouco tempo e o retry de 50 ms do Log
        // estoura. Sob comando o ritmo e do host.
        const float b = 1e6f * (float)hal::Info().block_size
                        / hal::Info().sample_rate;
        hal::Log("estreia, bloco a bloco (prazo %.0f us):", b);
        for(int i = 0; i < kProbeBlocks; i++)
            hal::Log("  bloco %d: %.0f us", i + 1, fx2_probe[i] * b);
        return;
    }
    if(strncmp(line, "cpu", 3) == 0)
    {
        // `cpu reset` ZERA O PICO. A etapa 5 do IFA10000 exige, com
        // essas palavras, "pico, com o medidor zerado entre as
        // leituras" — e ate a v0.42.0 a ferramenta pra fazer isso nao
        // existia. O `strncmp(...,3)` casava com "cpu reset" e so
        // imprimia: duas leituras seguidas devolviam o mesmo pico
        // acumulado desde o boot, e a segunda parecia medicao sendo
        // eco da primeira.
        {
            const char* a = line + 3;
            while(*a == ' ') a++;
            if(strncmp(a, "reset", 5) == 0 || strncmp(a, "zero", 4) == 0)
            {
                hal::CpuLoadReset();
                hal::Log("cpu: pico zerado — a proxima leitura vale a"
                         " partir de agora");
                return;
            }
        }
        // O prazo do bloco em microssegundos. Reportar % sozinho
        // engana quando se compara tamanhos de bloco diferentes: 50%
        // de 1 ms e 50% de 2,67 ms sao cargas MUITO diferentes em
        // trabalho absoluto. O µs e invariante.
        const float budget_us = 1e6f * (float)hal::Info().block_size
                                / hal::Info().sample_rate;
        hal::Log("cpu: %.1f%% media, %.1f%% pico",
                 hal::CpuLoadAvg() * 100.f, hal::CpuLoadMax() * 100.f);
        hal::Log("     %.0f us de pico em %.0f us de prazo (bloco %d @ %.0f Hz)",
                 hal::CpuLoadMax() * budget_us, budget_us,
                 (int)hal::Info().block_size, hal::Info().sample_rate);
        return;
    }
    if(strncmp(line, "mods", 4) == 0)
    {
        for(int s = 0; s < (int)ModuleSlot::kCount; s++)
        {
            const ModuleSlot slot = (ModuleSlot)s;
            Module*          m    = mod_slot[s];
            if(!m)
            { hal::Log("slot %-10s vazio", SlotName(slot)); continue; }
            hal::Log("slot %-10s \"%s\": %d params, %d in%s",
                     SlotName(slot), m->Name(), m->ParamCount(),
                     m->InputCount(),
                     m->InputCount() >= 2 ? " (ch2 = portadora IN_R)" : "");
            for(int i = 0; i < m->ParamCount(); i++)
                hal::Log("    [%d] %s", i, m->ParamName(i));
            const Module::MemReport mem = m->Memory();
            if(mem.blocks)
                hal::Log("    ram: %u SRAM (de %u) + %u SDRAM, %d bl, %d falhas",
                         (unsigned)mem.internal_bytes, (unsigned)mem.internal_cap,
                         (unsigned)mem.big_bytes, mem.blocks, mem.failures);
        }
        for(int b = 0; b < manifest.n_bind; b++)
            hal::Log("  %-12s -> %s.%s [%d]",
                     params.Name(manifest.bind[b].param_id),
                     SlotName(manifest.bind[b].slot),
                     manifest.bind[b].mod_name,
                     mod_index[b]);
        return;
    }
    if(strncmp(line, "list", 4) == 0)
    {
        for(int i = 0; i < params.Count(); i++)
            hal::Log("%2d %-10s norm=%.2f real=%.2f",
                     i, params.Name(i), params.Get(i), params.Real(i));
        return;
    }
    // ---- MIDI ----
    // ATENÇÃO à ordem: "miditest" e "midiloop" TÊM que ser testados
    // antes de "midi", senão o strncmp de 4 letras engole os dois.
    if(strncmp(line, "miditest ", 9) == 0 || strncmp(line, "midiloop ", 9) == 0)
    {
        // miditest <cc> <val> [canal 1-16]  — injeta na fila de RX
        // midiloop <cc> <val> [canal 1-16]  — manda pelo fio (T10)
        const bool  loop = line[4] == 'l';
        const char* p    = line + 9;
        char*       end  = nullptr;
        const long  cc   = strtol(p, &end, 10);
        const long  val  = strtol(end, &end, 10);
        long        ch   = strtol(end, &end, 10);
        if(ch < 1 || ch > 16)
            ch = 1;
        if(cc < 0 || cc > 127 || val < 0 || val > 127)
        {
            hal::Log("uso: %s <cc 0-127> <val 0-127> [canal 1-16]",
                     loop ? "midiloop" : "miditest");
            return;
        }
        const uint8_t msg[3] = {(uint8_t)(0xB0 | (uint8_t)(ch - 1)),
                                (uint8_t)cc,
                                (uint8_t)val};
        if(loop)
        {
            // Sai pelo TX e só volta se houver jumper D13→D14. O
            // parser vê os bytes pelo caminho REAL: UART, DMA, ISR,
            // fila. É o único teste que prova baud rate e pino.
            if(!hal::MidiRxActive())
            { hal::Log("midiloop: UART MIDI nao esta em escuta"); return; }
            hal::MidiSendBytes(msg, 3);
            hal::Log("midiloop: TX B%d %02lX %02lX — precisa de jumper "
                     "D13->D14 pra voltar",
                     (int)(ch - 1), cc, val);
        }
        else
        {
            // NÃO prova a UART: entra depois do fio. Prova o parser,
            // o mapa e a via de aplicação. Ver T10 no TESTES.md.
            for(int i = 0; i < 3; i++)
                hal::MidiInjectByte(msg[i]);
            hal::Log("miditest: injetado CC %ld = %ld no canal %ld "
                     "(NAO passa pela UART)", cc, val, ch);
        }
        return;
    }
    // `rearm` — forca o reinicio da escuta DMA do MIDI. Diagnostico E
    // conserto: se o canal volta depois disto, a causa era o DMA
    // abortado por erro de UART, e o vigia automatico do PollMidi
    // resolve dai em diante.
    if(strncmp(line, "rearm", 5) == 0)
    {
        const bool ok = hal::MidiRestart();
        hal::Log("rearm: escuta DMA %s (rearmes totais: %lu)",
                 ok ? "REINICIADA" : "FALHOU",
                 (unsigned long)hal::MidiRestarts());
        return;
    }
    if(strncmp(line, "pitch", 5) == 0)
    {
        const char* a = line + 5;
        while(*a == ' ') a++;
        if(*a == 0 || strncmp(a, "off", 3) == 0)
        {
            // NAO desativa a voz aqui. O comando so declara a
            // INTENCAO; quem executa e o callback, quando a rampa
            // chegar a zero.
            //
            // A versao anterior matava a voz na hora, e o molhado caia
            // a zero num unico sample enquanto o slot ainda rampava —
            // o fade cruzava pra silencio ja morto. Sintoma exato:
            // entrando 0 cliques, saindo 100%. So o caminho de SAIDA
            // tinha acao dura imediata, e era so nele que clicava.
            //
            // A regra: comando declara intencao, contexto de audio
            // executa. Acao dura vinda de fora atravessa qualquer rampa.
            pitch_test = false;
            hal::Log("pitch: rota de teste DESLIGADA (a rampa de 8 ms"
                     " termina no callback)");
            return;
        }
        if(strncmp(a, "grain", 5) == 0)
        {
            const float g = strtof(a + 5, nullptr);
            if(pitch_ok) pitch.SetGrainMs(0, g);
            hal::Log("pitch: grao = %.1f ms  (laterais em +-%.0f Hz)",
                     (double)g, (double)(2000.f / (g > 0.f ? g : 1.f)));
            return;
        }
        if(strncmp(a, "jitter", 6) == 0)
        {
            const float j = strtof(a + 6, nullptr);
            if(pitch_ok) pitch.SetJitter(j);
            hal::Log("pitch: jitter = %.3f  (0 = graos deterministicos,"
                     " so pra MEDIR com seno)", (double)j);
            return;
        }
        const float r = strtof(a, nullptr);
        if(r <= 0.f)
        {
            hal::Log("uso: pitch <ratio>  (2=oitava acima, 1.498=quinta,"
                     " 0.5=oitava abaixo) | pitch off");
            return;
        }
        pitch_ratio = r;
        pitch_test  = true;
        hal::Log("pitch: rota de teste LIGADA, ratio %.4f"
                 " (100%% molhado, so o motor no caminho)", (double)r);
        hal::Log("  esperado: `tone on` a 848 Hz -> %.0f Hz na saida",
                 (double)(848.f * r));
        pitch_pk_in = pitch_pk_out = 0.f;
        return;
    }
    if(strncmp(line, "hist", 4) == 0)
    {
        const char* a = line + 4;
        while(*a == ' ') a++;
        if(strncmp(a, "reset", 5) == 0)
        {
            for(int b = 0; b < kNBaldes; b++)
                hist_l[b] = hist_r[b] = hist_o[b] = 0;
            in_pk_l = in_pk_r = out_pk_g = 0.f;
            in_n_l = in_n_r = 0;
            in_clip_l = in_clip_r = out_clip_g = 0;
            clip_latch_l = clip_latch_r = clip_latch_o = false;
            hal::Log("hist: zerado — a proxima leitura vale a partir de agora");
            return;
        }
        // FOTOGRAFIA ANTES DA CONTA. A primeira versao somava os
        // totais numa passada e imprimia noutra, com o callback de
        // audio incrementando os baldes no meio — e relatou 100.25%,
        // um percentual que nunca existiu. Copiar tudo de uma vez nao
        // torna a leitura atomica, mas garante que o que se imprime e
        // o que se somou: os numeros passam a ser um instante
        // possivel, ainda que ligeiramente velho.
        uint32_t hl[kNBaldes], hr[kNBaldes], ho[kNBaldes];
        for(int b = 0; b < kNBaldes; b++)
        { hl[b] = hist_l[b]; hr[b] = hist_r[b]; ho[b] = hist_o[b]; }
        const float pk_l = in_pk_l, pk_r = in_pk_r, pk_o = out_pk_g;
        const uint32_t cl_l = in_clip_l, cl_r = in_clip_r;
        uint32_t tl = 0, tr = 0, to = 0;
        for(int b = 0; b < kNBaldes; b++)
        { tl += hl[b]; tr += hr[b]; to += ho[b]; }
        if(tl == 0)
        {
            hal::Log("hist: nada medido ainda (`hist reset` e toque)");
            return;
        }
        const float ig = params.Real(manifest.p_in_gain);
        hal::Log("HISTOGRAMA DE ENTRADA  (in_gain %.2fx)", (double)ig);
        hal::Log("  faixa dBFS     IN_L(p1)    IN_R(p2)   POS-GANHO");
        for(int b = 0; b < kNBaldes; b++)
        {
            if(hl[b] == 0 && hr[b] == 0 && ho[b] == 0) continue;
            char faixa[16];
            if(b == 0)                 snprintf(faixa, sizeof faixa, ">= 0 CLIP");
            else if(b == kNBaldes - 1) snprintf(faixa, sizeof faixa, "<= -60");
            else snprintf(faixa, sizeof faixa, "%4d..%-4d", -6 * b, -6 * (b - 1));
            hal::Log("  %-12s %6.2f%%     %6.2f%%     %6.2f%%", faixa,
                     (double)(100.f * (float)hl[b] / (float)tl),
                     (double)(100.f * (float)hr[b] / (float)(tr ? tr : 1u)),
                     (double)(100.f * (float)ho[b] / (float)(to ? to : 1u)));
        }
        auto dbfs = [](float v) { return v > 1e-7f ? 20.f * log10f(v) : -140.f; };
        hal::Log("  pico: L %.1f dBFS | R %.1f dBFS | pos-ganho %.1f dBFS",
                 (double)dbfs(pk_l), (double)dbfs(pk_r), (double)dbfs(pk_o));
        hal::Log("  clip: L %lu%s | R %lu%s | pos-ganho %lu%s",
                 (unsigned long)in_clip_l, clip_latch_l ? " *** " : "",
                 (unsigned long)in_clip_r, clip_latch_r ? " *** " : "",
                 (unsigned long)out_clip_g, clip_latch_o ? " *** " : "");

        // O NUMERO QUE DIMENSIONA O DIVISOR. Alvo -6 dBFS de pico: deixa
        // 6 dB de folga pro gesto que ainda nao aconteceu.
        // DIMENSIONAMENTO DO ATENUADOR — e a recusa de dimensionar
        // quando nao da.
        //
        // Pico GRAMPEADO nao dimensiona nada: a medicao satura em
        // 0 dBFS e o pico real e "pelo menos isso", podendo estar 6 ou
        // 25 dB acima. Imprimir "precisa tirar +6.0 dB" nesse estado e
        // dar cara de resposta a um piso — e alguem solda o divisor
        // errado por causa disso.
        //
        // Regra da casa: mensagem que afirma tem que citar o outro
        // estado. Aqui o outro estado e "nao sei, e por isto".
        for(int c = 0; c < 2; c++)
        {
            const float    pk = c ? pk_r : pk_l;
            const uint32_t cl = c ? cl_r : cl_l;
            const char*    nm = c ? "p2 (IN_R)" : "p1 (IN_L)";
            if(pk < 1e-6f) continue;
            if(cl > 0)
            {
                hal::Log("  %s: PICO GRAMPEADO (%lu amostras). Nao da pra"
                         " dimensionar:", nm, (unsigned long)cl);
                hal::Log("      o pico real e >= 0 dBFS, quanto acima e"
                         " invisivel daqui.");
                hal::Log("      atenue 20 dB PROVISORIO, meca de novo, e o"
                         " numero passa a valer.");
                continue;
            }
            const float precisa = dbfs(pk) + 6.f;   // quanto TIRAR
            int degrau = 0;
            if(precisa > 9.f)      degrau = 12;
            else if(precisa > 3.f) degrau = 6;
            hal::Log("  %s: pico limpo. Precisa tirar %+.1f dB -> degrau"
                     " %d dB%s", nm, (double)precisa, degrau,
                     degrau == 0 ? "  (ja cabe, nao mexer)" : "");
        }
        return;
    }
    if(strncmp(line, "pk", 2) == 0)
    {
        // ENTRADA CRUA POR CANAL, primeiro: e a pergunta mais barata
        // de responder e a que mais vezes ficou sem resposta.
        {
            const float ig = params.Real(manifest.p_in_gain);
            auto dbfs = [](float v) {
                return v > 1e-7f ? 20.f * log10f(v) : -140.f;
            };
            hal::Log("ENTRADA CRUA (pre-balanco, pre-in_gain):");
            hal::Log("  IN_L (p1): pico %.5f = %6.1f dBFS | %lu amostras | clip %lu",
                     (double)in_pk_l, (double)dbfs(in_pk_l),
                     (unsigned long)in_n_l, (unsigned long)in_clip_l);
            hal::Log("  IN_R (p2): pico %.5f = %6.1f dBFS | %lu amostras | clip %lu",
                     (double)in_pk_r, (double)dbfs(in_pk_r),
                     (unsigned long)in_n_r, (unsigned long)in_clip_r);
            hal::Log("  com in_gain %.2fx: L -> %.1f dBFS | R -> %.1f dBFS%s",
                     (double)ig, (double)dbfs(in_pk_l * ig),
                     (double)dbfs(in_pk_r * ig),
                     (in_pk_l * ig >= 1.f || in_pk_r * ig >= 1.f)
                         ? "   *** SATURA ***" : "");
            if(in_n_l == 0 && in_n_r == 0)
                hal::Log("  os DOIS mudos — ninguem tocou, ou a entrada nao chega");
            else if(in_n_r == 0)
                hal::Log("  IN_R MUDO com IN_L vivo: o problema e do p2 (cabo/pino/piezo)");
            else if(in_n_l == 0)
                hal::Log("  IN_L MUDO com IN_R vivo: o problema e do p1 (cabo/pino/piezo)");
            in_pk_l = in_pk_r = 0.f;
            in_n_l = in_n_r = 0; in_clip_l = in_clip_r = 0;
        }
        hal::Log("pitch: pico ENTRADA %.5f | SAIDA %.5f | razao %.3f",
                 (double)pitch_pk_in, (double)pitch_pk_out,
                 (double)(pitch_pk_in > 1e-9f
                              ? pitch_pk_out / pitch_pk_in : 0.f));
        pitch_pk_in = pitch_pk_out = 0.f;
        return;
    }
    if(strncmp(line, "dfu", 3) == 0)
    {
        // Exige confirmacao porque nao tem volta por software: uma vez
        // em DFU, so gravar (ou RESET fisico) traz o app de volta.
        const char* a = line + 3;
        while(*a == ' ') a++;
        if(strncmp(a, "go", 2) != 0)
        {
            hal::Log("dfu: reinicia a placa DIRETO no bootloader, com"
                     " timeout INFINITO — ela espera ate ser gravada.");
            hal::Log("     confirme com `dfu go`. Nao ha volta por"
                     " software: so gravar ou RESET fisico.");
            return;
        }
        hal::Log("dfu: indo pro bootloader. Ate ja.");
        hal::DelayMs(50);   // deixa a linha sair antes de sumir o USB
        hal::RebootToDfu();
        return;
    }
    if(strncmp(line, "sync", 4) == 0)
    {
        MidiPushEstado();
        return;
    }
    if(strncmp(line, "ctos", 4) == 0)
    {
        declara_modos();
        const char* a = line + 4;
        while(*a == ' ') a++;
        if(strncmp(a, "rearm", 5) == 0)
        {
            ctos_bypass_frz  = false;
            ctos_pico_us     = 0;
            ctos_ocorrencias = 0;
            pitch_slot.ZeraPico();
            shimmer_slot.ZeraPico();
            shimmer_pico = 0;
            // TETO DE SOMA: solta os latches. NAO religa os efeitos —
            // soltar o latch devolve o DIREITO de ligar; ligar continua
            // sendo gesto do dB. Religar aqui decidiria por ele qual
            // configuracao ele queria de volta.
            for(int k = 0; k < manifest.n_sacrif; k++) soma_caiu[k] = false;
            soma_sem_saida = false;
            soma_seguidos  = 0;
            soma_carencia  = 0;
            soma_pico_us   = 0;
            hal::Log("ctos: efeitos rearmados — o veto volta a valer do zero");
            hal::Log("  teto de soma: latches soltos. Os efeitos NAO"
                     " religam sozinhos — ligue os que quiser de volta");
            return;
        }
        if(strncmp(a, "teto", 4) == 0)
        {
            // TETO ARTIFICIAL — so pra TESTE. Baixar o teto simula
            // "muitos efeitos" sem precisar carregar a CPU de verdade,
            // que e a unica forma de exercitar o mecanismo antes de o
            // instrumento chegar no limite em cena.
            const char* v = a + 4;
            while(*v == ' ') v++;
            if(*v == 0)
            {
                hal::Log("ctos teto: %lu us %s (uso: `ctos teto <us>`,"
                         " 0 = volta ao automatico)",
                         (unsigned long)soma_teto_us,
                         soma_teto_forc ? "FORCADO (teste)" : "automatico");
                return;
            }
            soma_teto_forc = (uint32_t)atoi(v);
            soma_seguidos  = 0;
            hal::Log("ctos teto: %s",
                     soma_teto_forc ? "FORCADO — isto e um teste, nao"
                                      " esqueca de voltar com `ctos teto 0`"
                                    : "de volta ao automatico");
            return;
        }
        hal::Log("CTOS | loop vivo | fase %u | %lu voltas | uptime %lu s",
                 (unsigned)loop_fase, (unsigned long)loop_voltas,
                 (unsigned long)(hal::NowMs() / 1000u));
        if(ctos_bypass_frz)
        {
            hal::Log("  efeito \"freeze\": BYPASS por overrun");
            hal::Log("    estourou em t=%.1f s | pico %lu us de %lu us"
                     " | %lu ocorrencias",
                     (double)ctos_bypass_ms / 1000.0,
                     (unsigned long)ctos_pico_us,
                     (unsigned long)ctos_orcamento_us,
                     (unsigned long)ctos_ocorrencias);
            hal::Log("    bypass e PERMANENTE ate `ctos rearm` — efeito que"
                     " estourou uma vez estoura de novo, e religar sozinho"
                     " esconderia o defeito");
        }
        else
            hal::Log("  efeito \"freeze\": ativo | pico %lu us de %lu us",
                     (unsigned long)ctos_pico_us,
                     (unsigned long)ctos_orcamento_us);
        hal::Log("    relogio: ticks crus, %lu por us (a volta do"
                 " contador se corrige na subtracao)",
                 (unsigned long)hal::TicksPerUs());

        // ---- TETO DE SOMA — mecanismo DIFERENTE do de cima ----
        hal::Log("  TETO DE SOMA (bloco inteiro, nao soma de estimativas):");
        hal::Log("    bloco %lu us | pico %lu us | teto %lu us%s | quedas %lu%s",
                 (unsigned long)soma_us, (unsigned long)soma_pico_us,
                 (unsigned long)soma_teto_us,
                 soma_teto_forc ? " (FORCADO, teste)" : "",
                 (unsigned long)soma_quedas,
                 soma_carencia ? " | EM CARENCIA (rampa descendo)" : "");
        hal::Log("    ordem de sacrificio (indice 0 cai primeiro):");
        for(int k = 0; k < manifest.n_sacrif; k++)
        {
            const bool lig
                = params.Real(manifest.sacrif[k].param_on) >= 0.5f;
            if(soma_caiu[k])
                hal::Log("      %d. %-8s CAIU pelo teto em t=%.1f s"
                         " (bloco estava em %lu us)", k,
                         manifest.sacrif[k].nome,
                         (double)soma_caiu_ms[k] / 1000.0,
                         (unsigned long)soma_caiu_us[k]);
            else
                hal::Log("      %d. %-8s %s", k, manifest.sacrif[k].nome,
                         lig ? "ligado" : "desligado");
        }
        if(soma_sem_saida)
            hal::Log("    *** estourou sem ter o que derrubar: a base"
                     " sozinha nao cabe ***");
        hal::Log("  shimmer: %s | pico %lu us de %lu us",
                 shimmer_slot.Ativo() ? "ATIVO" : "desligado (custo zero)",
                 (unsigned long)shimmer_pico,
                 (unsigned long)ctos_orcamento_us);
        hal::Log("  motor de pitch: %s | pico %lu us de %lu us",
                 pitch_slot.Ativo() ? "ATIVO" : "desligado (custo zero)",
                 (unsigned long)pitch_slot.PicoUs(),
                 (unsigned long)ctos_orcamento_us);
        hal::Log("  o instrumento perde o efeito, nunca a base."
                 " Desenho em docs/CTOS.md");
        return;
    }
    if(strncmp(line, "ble", 3) == 0)
    {
        declara_modos();
        {
            const char* a = line + 3;
            while(*a == ' ') a++;
            if(strncmp(a, "raw", 3) == 0)
            {
                const char* b = a + 3;
                while(*b == ' ') b++;
                lb_cru = (strncmp(b, "off", 3) != 0);
                hal::Log("ble raw: %s — publica transicoes/2s e %% aceso"
                         " SEM classificar. Correlacione com o trafego"
                         " antes de acreditar em qualquer legenda.",
                         lb_cru ? "LIGADO" : "desligado");
                return;
            }
            if(strncmp(a, "mirror", 6) == 0)
            {
                const char* b = a + 6;
                while(*b == ' ') b++;
                lb_espelho = (strncmp(b, "on", 2) == 0);
                hal::Log("ble: espelho do LB no LED da placa %s%s",
                         lb_espelho ? "LIGADO" : "desligado",
                         lb_espelho ? " (o pulso de fase do CTOS para"
                                      " enquanto durar)" : "");
                return;
            }
        }
        const uint32_t now = hal::NowMs();
        const uint32_t age = ble_last_rx_ms ? (now - ble_last_rx_ms) : 0u;
        hal::Log("ble: ultimo byte ha %s%.1f s | msgs da sessao %lu",
                 ble_last_rx_ms ? "" : "(nunca recebeu) ",
                 (double)age / 1000.0,
                 (unsigned long)(midi.CcCount() - ble_session_base));
        // ---- CAMADA 1: numero, nao nome ----
        hal::Log("  WIDI LEDs (D28/29/30) — transicoes por 2 s / %% aceso:");
        hal::Log("    LB %lu/%u%%   LG %lu/%u%%   LR %lu/%u%%",
                 (unsigned long)lb_trans[0], (unsigned)lb_duty[0],
                 (unsigned long)lb_trans[1], (unsigned)lb_duty[1],
                 (unsigned long)lb_trans[2], (unsigned)lb_duty[2]);
        // ---- O CRUZAMENTO QUE IMPORTA NA PRATICA ----
        //
        // LB conectado + ZERO trafego desde o boot = a SESSAO CoreMIDI
        // caiu com o radio intacto. Sao camadas diferentes: o BLE
        // sobrevive a gravacao, a sessao MIDI nao.
        //
        // Medido em 18/08: depois de `dfu go` + gravar, o LB fica em
        // 0/100% (conectado) e nao chega um byte. Reconectar no painel
        // de Bluetooth MIDI resolve; ESQUECER o dispositivo e
        // desnecessario — o pareamento nunca esteve quebrado.
        // SILENCIO NAO DISTINGUE, E ESTE AVISO QUASE ESQUECEU DISSO.
        //
        // A primeira versao disparava com "LB conectado + zero bytes
        // desde o boot" — que e verdade quando a sessao MIDI morreu E
        // quando ninguem encostou em nada ainda. Disparou num caso
        // saudavel na primeira vez que rodou.
        //
        // Terceira vez no mesmo dia que um instrumento meu trata
        // ausencia de dado como dado. A correcao nao e o limiar: e
        // parar de AFIRMAR. O aviso vira hipotese com o teste junto.
        if(lb_duty[0] > 90 && lb_trans[0] == 0 && ble_last_rx_ms == 0
           && hal::NowMs() > 20000u)
        {
            hal::Log("    >>> LB conectado e NENHUM byte desde o boot"
                     " (ha %lu s).", (unsigned long)(hal::NowMs() / 1000u));
            hal::Log("    >>> Se voce JA mexeu num controle, a sessao"
                     " CoreMIDI caiu: reconecte o WIDI no painel"
                     " Bluetooth MIDI. NAO precisa esquecer o aparelho.");
            hal::Log("    >>> Se ainda nao mexeu, isto aqui nao diz nada:"
                     " mexa e olhe de novo.");
        }
        hal::Log("    sem veredito: a legenda da CME foi DESMENTIDA em"
                 " bancada (desconectou, LB ficou fixo). `ble raw`"
                 " acompanha ao vivo — a tabela sai da medicao.");
        hal::Log("  CCs recusados pela quarentena: %lu",
                 (unsigned long)cc_quarentena);
        hal::Log("  silencios >10 s: %lu | (re)conexoes detectadas %lu",
                 (unsigned long)ble_drops, (unsigned long)ble_reconnects);
        // O VEREDITO, que e o que se olha no palco. Silencio nao e
        // diagnostico: silencio ANTES de qualquer trafego significa
        // "nunca conectou"; DEPOIS de trafego significa "caiu". Sao
        // consertos diferentes, e a serie de zeros e identica nos dois.
        const bool keepalive = midi.RealTime() > 0;
        const char* v;
        if(!ble_last_rx_ms)   v = "NUNCA RECEBEU — parear / conferir fio";
        else if(age > 2000u)
            v = keepalive
                    ? "SEM KEEPALIVE — o controlador mandava Active"
                      " Sensing e parou: QUEDA provavel"
                    : "SEM TRAFEGO — e so isso. Silencio NAO distingue"
                      " 'ninguem mexeu' de 'link caiu': olhe o LED";
        else                  v = "VIVO";
        hal::Log("  veredito: %s", v);
        if(!keepalive)
            hal::Log("  (sem Active Sensing neste link — o contador"
                     " acima conta SILENCIOS, nao quedas)");
        hal::Log("  arvore de decisao completa: BOARDS.md, 'Bluetooth parou'");
        return;
    }
    if(strncmp(line, "log ", 4) == 0)
    {
        const bool on = (strncmp(line + 4, "on", 2) == 0);
        hal::LogMute(!on);
        if(on) hal::Log("log: ligado");
        return;
    }
    if(strncmp(line, "mscope", 6) == 0)
    {
        const char* a = line + 6;
        while(*a == ' ') a++;
        declara_modos();
        if(strncmp(a, "on", 2) == 0)       mscope_mode = 1;
        else if(strncmp(a, "stats", 5) == 0) mscope_mode = 2;
        else                                 mscope_mode = 0;
        // Um scope que nao fala nao e scope. Se a caixa-preta calou o log,
        // ligar o mscope destrava — senao a medicao morre muda e o zero
        // na tela e o meu proprio silencio, nao o do MIDI.
        if(mscope_mode != 0) hal::LogMute(false);
        hal::Log("mscope: %s", mscope_mode == 1 ? "STREAM (hex + veredito)"
                             : mscope_mode == 2 ? "STATS (1 linha/s)"
                                                : "off");
        if(mscope_mode == 1)
            hal::Log("  em rajada o stream tem BURACOS de proposito;"
                     " os contadores do `midi` nao mentem");
        return;
    }
    if(strncmp(line, "midi", 4) == 0)
    {
        declara_modos();
        // A primeira coisa a consultar no dia da solda. A pergunta que
        // ele responde é em qual etapa o sinal parou: fio, parser,
        // canal ou mapa.
        hal::Log("  uart: erros %lu | rearmes da escuta %lu",
                 (unsigned long)hal::MidiErrors(),
                 (unsigned long)hal::MidiRestarts());
        // FURO DO PollMidi, fechado com numero: os bytes entram por DMA
        // (imune a carga), mas quem despacha e o loop. A marca d'agua diz
        // quanta folga sobrou entre uma volta e outra.
        hal::Log("  quarentena: %lu CCs recusados (dumps de reconexao)",
                 (unsigned long)cc_quarentena);
        hal::Log("  fila: pico %lu de %lu bytes (%.0f%% cheia no pior caso)",
                 (unsigned long)hal::MidiHiWater(),
                 (unsigned long)hal::MidiRingSize(),
                 100.0f * (float)hal::MidiHiWater()
                     / (float)hal::MidiRingSize());
        hal::Log("midi: uart %s | perdidos %lu | canal %s",
                 hal::MidiRxActive() ? "ESCUTANDO" : "PARADA",
                 (unsigned long)hal::MidiDropped(),
                 midi.Channel() == cotuca::kMidiOmni ? "omni" : "fixo");
        if(midi.Channel() != cotuca::kMidiOmni)
            hal::Log("      canal %d (1-16, como no controlador)",
                     midi.Channel() + 1);
        hal::Log("  bytes %lu | cc %lu | outras %lu | orfaos %lu | "
                 "realtime %lu | canal alheio %lu",
                 (unsigned long)midi.Bytes(), (unsigned long)midi.CcCount(),
                 (unsigned long)midi.Other(), (unsigned long)midi.Orphan(),
                 (unsigned long)midi.RealTime(),
                 (unsigned long)midi.Filtered());
        // ESTADO DA MAQUINA por extenso: "aguardando o que?" e a
        // pergunta que o numero cru nao responde.
        const int nd = midi.PendingData();
        hal::Log("  parser: running status %02X | aguardando %s%s",
                 midi.RunningStatus(),
                 midi.RunningStatus() == 0 ? "STATUS"
                 : nd == 0                 ? "data1"
                                           : "data2",
                 midi.InSysex() ? " | DENTRO DE SYSEX" : "");
        // Estes tres separam "chegou e nao virou mensagem" de "virou
        // mensagem e nao virou som" — que sao consertos diferentes.
        hal::Log("  msgs completas %lu | APLICADAS %lu | ressync %lu",
                 (unsigned long)midi.CcCount() + midi.Other(),
                 (unsigned long)midi.Applied(),
                 (unsigned long)midi.Resync());
        // Espelho na caixa-preta: com o vocoder ligado a serial morre,
        // e estes numeros sao a unica autopsia possivel do MIDI sob
        // carga. Slots 33-37.
        hal::BlackboxWrite(33, (float)midi.Bytes());
        hal::BlackboxWrite(34, (float)(midi.CcCount() + midi.Other()));
        hal::BlackboxWrite(35, (float)midi.Applied());
        hal::BlackboxWrite(36, (float)midi.Resync());
        hal::BlackboxWrite(37, (float)hal::MidiDropped());
        hal::BlackboxWrite(85, (float)hal::MidiHiWater()); // 41 e do diario (40+min)
        for(int i = 0; i < manifest.n_midi; i++)
            hal::Log("  cc %-3d -> %s",
                     manifest.midi_map[i].cc,
                     params.Name(manifest.midi_map[i].param_id));
        const int n = midi.HistoryCount();
        if(!n)
        { hal::Log("  ultimos CCs: nenhum ainda"); return; }
        hal::Log("  ultimos CCs (mais recente primeiro):");
        for(int i = 0; i < n; i++)
        {
            const cotuca::MidiCcEvent& e = midi.History(i);
            const int id = [&]() {
                for(int k = 0; k < manifest.n_midi; k++)
                    if(manifest.midi_map[k].cc == e.cc)
                        return manifest.midi_map[k].param_id;
                return -1;
            }();
            hal::Log("    cc %-3d val %-3d ch %-2d -> %s",
                     e.cc, e.value, e.channel + 1,
                     id >= 0 ? params.Name(id) : "(fora do mapa)");
        }
        return;
    }
    hal::Log("? k1..k5 <0-1> | set <nome> <real> | tone on/off | board | list | mods | dyn | cpu | hist [reset] | probe | bb | midi | mscope on/off/stats | log on/off | ble [raw|mirror on/off] | sync | dfu go | pitch <ratio> | ctos [rearm] | rearm | miditest | midiloop");
}

// confere manifest.req contra a placa. Falhou = LED rápido + log.
static bool CheckRequirements()
{
    using namespace cotuca;
    const auto& b = hal::Info();
    const auto& r = manifest.req;
    bool ok = true;
    if(r.needs_audio_in && !b.has_audio_in)
    { hal::Log("ERRO: instrumento exige audio in; %s nao tem", b.name); ok = false; }
    if(r.big_ram_bytes > b.big_ram_bytes)
    { hal::Log("ERRO: exige %u bytes de RAM grande; %s tem %u",
               (unsigned)r.big_ram_bytes, b.name, (unsigned)b.big_ram_bytes); ok = false; }
    if(r.min_knobs > b.num_knobs)
    { hal::Log("ERRO: exige %d knobs; %s tem %d", r.min_knobs, b.name, b.num_knobs); ok = false; }
    return ok;
}

int main(void)
{
    using namespace cotuca;
    hal::Init();
    params.Init();
    manifest.Init(params);

    if(!CheckRequirements())
    {
        // pisca rápido pra sempre: "cérebro errado pra este instrumento"
        bool led = false;
        while(1) { hal::SetLed(led); led = !led; hal::DelayMs(100); }
    }

    // Tamanho de bloco é decisão de INSTRUMENTO, não de placa: define
    // a latência, e num instrumento realimentado a latência é parte do
    // laço acústico.
    //
    // TEM QUE VIR ANTES DO Init DOS MÓDULOS. O Module::Init recebe
    // hal::Info().block_size e o repassa ao prepareToProcess, que
    // dimensiona os buffers internos por ele. Pedir o bloco DEPOIS
    // deixava os módulos preparados pra 48 enquanto o callback
    // entregava 256 — estouro de heap silencioso nos blocos vizinhos
    // do alocador. Mesma familia dos outros bugs de ordem desta
    // funcao: quem configura tem que vir antes de quem consome.
    hal::RequestBlockSize(manifest.req.block_size);

    // ---- módulos: pega do REGISTRO, slot a slot ----
    // O main não conhece motor nenhum. Quem se registrou, apareceu.
    for(int s = 0; s < (int)ModuleSlot::kCount; s++)
    {
        const ModuleSlot slot = (ModuleSlot)s;
        Module*          m    = GetModule(slot);
        if(!m)
        {
            hal::Log("slot %-10s vazio (passthrough)", SlotName(slot));
            continue;
        }
        if(!m->Init(hal::Info().sample_rate, hal::Info().block_size))
        {
            // slot fica nullptr = passthrough. Áudio limpo com um
            // módulo faltando é melhor que áudio corrompido com um
            // módulo meio inicializado.
            hal::Log("ERRO: \"%s\" no slot %s falhou no Init — slot vazio",
                     m->Name(), SlotName(slot));
            continue;
        }
        // ---- WARM-UP: política do slot, não remendo de módulo ----
        //
        // Módulo espectral aloca preguiçosamente no primeiro HOP, não
        // no primeiro Process. Medido no vocoder (FFT 1024/hop 512,
        // bloco 256): bloco 1 custou 932 us, bloco 2 custou 81 563,
        // bloco 4 custou 42 512, e só no bloco 6 chegou aos 3 526 do
        // regime. Uma chamada de silêncio não resolveria — o bloco 1
        // é justamente o barato.
        //
        // Então rodamos silêncio até cobrir ~4096 amostras, que dá
        // dois hops de 2048, quatro de 1024 ou oito de 512. O custo
        // vai pro boot, onde ninguém ouve, e toda ativação posterior
        // já nasce em regime.
        //
        // Genérico de propósito: vale pra qualquer módulo com estreia
        // preguiçosa, não só pro fftstream~. E preserva o alocador
        // bump simples — determinístico, sem fragmentação — que
        // crescimento esperto começaria a erodir.
        {
            const size_t bs = hal::Info().block_size;
            const int    n  = (int)((4096 + bs - 1) / bs);
            const float* wi[2] = {zero_buf, zero_buf};
            float*       wo[2] = {fx_buf[0], fx_buf[1]};
            for(int k = 0; k < n; k++)
                m->Process(wi, wo, bs);
            hal::Log("slot %-10s warm-up: %d blocos de silencio",
                     SlotName(slot), n);
        }

        mod_slot[s] = m;
        hal::Log("slot %-10s \"%s\": %d params",
                 SlotName(slot), m->Name(), m->ParamCount());
    }

    for(int b = 0; b < manifest.n_bind; b++)
    {
        Module* m    = mod_slot[(int)manifest.bind[b].slot];
        mod_index[b] = m ? m->FindParam(manifest.bind[b].mod_name) : -1;
        mod_last[b]  = -1.f; // força o primeiro push
        if(m && mod_index[b] < 0)
            hal::Log("AVISO: param \"%s\" nao existe no slot %s",
                     manifest.bind[b].mod_name, SlotName(manifest.bind[b].slot));
    }

    // Salvaguarda (c): bench_boost TEM que nascer em 1.0. Hoje não
    // existe persistência de cena, então isto nunca dispara — é
    // tripwire pro dia em que existir. Um boost de 8x restaurado em
    // silêncio seria a escultura gritando no palco sem explicação.
    {
        const float b = params.Real(manifest.p_bench_boost);
        if(b != 1.0f)
            hal::Log("AVISO: bench_boost nasceu em %.2f e nao 1.00 — "
                     "ferramenta de bancada NAO deve persistir", b);
    }

    gate.Init(hal::Info().sample_rate);
    gov_l.Init(hal::Info().sample_rate);
    lim_l.Init(hal::Info().sample_rate);
    lim_r.Init(hal::Info().sample_rate);
    shift_l.Init(hal::Info().sample_rate);

    // ---- MIDI ----
    // Depois do manifest.Init(): o parser recebe o canal do manifest,
    // e o mapa CC→param só existe depois dos Add(). Mesma regra de
    // ordem de configuração que vale pros bind[].
    midi.Init(manifest.midi_channel, OnMidiCc);
    midi.SetTrace(OnMidiTrace);
    hal::SetMidiByteCallback(OnMidiByte); // é isto que liga a UART
    hal::Log("MIDI: canal %s, %d CCs mapeados",
             manifest.midi_channel == kMidiOmni ? "omni"
                                                : "fixo (ver `midi`)",
             manifest.n_midi);

    // REVERB: aloca ANTES do StartAudio — familia (B) da regra de
    // ordem, quem configura vem antes de quem consome. O callback
    // rodando com reverb_ok false so passa direto; rodando com buffer
    // meio-alocado leria memoria alheia.
    //
    // O `FloatsNeeded` e perguntado ao proprio reverb: este arquivo nao
    // conhece as afinacoes do Freeverb, e o dia em que a taxa mudar o
    // tamanho acompanha sozinho. Fixar um numero aqui seria a mesma
    // armadilha do r_window com 2048 numa FFT de 1024.
    {
        const size_t need = cotuca::Reverb::FloatsNeeded(hal::Info().sample_rate);
        const size_t have = sizeof(reverb_mem) / sizeof(reverb_mem[0]);
        fx2_hp_l.Init(hal::Info().sample_rate);
        fx2_hp_r.Init(hal::Info().sample_rate);
        reverb_ok = reverb.Init(hal::Info().sample_rate, reverb_mem, have);
        suave_gl.Init(hal::Info().sample_rate, 20.f, 1.f);
        suave_gr.Init(hal::Info().sample_rate, 20.f, 1.f);
        suave_width.Init(hal::Info().sample_rate, 20.f, 1.f);
        // Loga OS DOIS numeros. Se a taxa mudar e o `need` passar o
        // `have`, o reverb recusa (passthrough) e o log diz por que —
        // em vez de rodar lendo memoria alheia, que e o que um
        // buffer curto faria em silencio.
        hal::Log("reverb: %s (%u de %u floats, %u KB em SRAM)",
                 reverb_ok ? "pronto" : "BUFFER CURTO — passthrough",
                 (unsigned)need, (unsigned)have,
                 (unsigned)(sizeof(reverb_mem) / 1024));
    }

    // MOTOR DE PITCH: alocar e inicializar ANTES do StartAudio, mesma
    // regra de ordem do reverb e do freeze.
    {
        const float  sr   = hal::Info().sample_rate;
        const size_t need = cotuca::PitchEngine::FloatsNeeded(sr, kPitchSeconds);
        pitch_mem = (float*)hal::AllocBig(need * sizeof(float));
        if(pitch_mem)
            pitch_ok = pitch.Init(sr, pitch_mem, need, kPitchSeconds);
        pitch_slot.Init(sr, 8.f);

        // Motor da cauda + shimmer. Mesmo tamanho de buffer: a oitava
        // acima exige a mesma folga de partida.
        const size_t need_t
            = cotuca::PitchEngine::FloatsNeeded(sr, kPitchSeconds);
        pitch_tail_mem = (float*)hal::AllocBig(need_t * sizeof(float));
        if(pitch_tail_mem)
            pitch_tail_ok = pitch_tail.Init(sr, pitch_tail_mem, need_t,
                                            kPitchSeconds);
        shimmer.Init(sr);
        drive.Init(sr);
        drive_slot.Init(sr, 8.f);
        phaser.Init(sr);
        phaser_slot.Init(sr, 8.f);
        warble_ok = warble.Init(sr, warble_mem,
                                sizeof(warble_mem)/sizeof(warble_mem[0]), 40.f);
        warble_slot.Init(sr, 8.f);
        {
            const size_t nd = cotuca::PitchEngine::FloatsNeeded(sr, kPitchSeconds);
            pitch_det_mem = (float*)hal::AllocBig(nd * sizeof(float));
            if(pitch_det_mem)
                pitch_det_ok = pitch_det.Init(sr, pitch_det_mem, nd, kPitchSeconds);
            detune.Init(sr);
            detune_slot.Init(sr, 8.f);
            hal::Log("detune: %s (%u KB em SDRAM)",
                     pitch_det_ok ? "pronto" : "SEM BUFFER — inativo",
                     (unsigned)(nd * sizeof(float) / 1024));
        }
        hal::Log("warble: %s (40 ms, %u KB em SRAM)",
                 warble_ok ? "pronto" : "BUFFER CURTO — inativo",
                 (unsigned)(sizeof(warble_mem)/1024));
        // CAUDA DE 2 s NA SAIDA (pedido do dB). Entrar e imediato;
        // sair e decaimento — o shimmer E a sala se empilhando, e sala
        // nao para de existir quando alguem solta o botao.
        shimmer_slot.Init(sr, 8.f, 2000.f);
        hal::Log("shimmer: %s (motor da cauda, %u KB em SDRAM)",
                 pitch_tail_ok ? "pronto" : "SEM BUFFER — inativo",
                 (unsigned)(need_t * sizeof(float) / 1024));
        hal::Log("pitch: %s (%.0f ms, %u floats, %u KB em SDRAM)",
                 pitch_ok ? "pronto" : "SEM BUFFER — inativo",
                 (double)(kPitchSeconds * 1000.f), (unsigned)need,
                 (unsigned)(need * sizeof(float) / 1024));
    }

    // FREEZE: mesma regra de ordem do reverb — alocar e inicializar
    // ANTES do StartAudio. E o tamanho é PERGUNTADO ao freeze, nunca
    // fixado aqui: se a taxa mudar, o pedido acompanha sozinho.
    {
#if COTUCA_FREEZE_ENABLED
        const float  sr   = hal::Info().sample_rate;
        const size_t need = cotuca::Freeze::FloatsNeeded(sr, kFreezeSeconds);
        freeze_mem = (float*)hal::AllocBig(need * sizeof(float));
        if(freeze_mem)
            freeze_ok = freeze.Init(sr, freeze_mem, need, kFreezeSeconds);
        hal::Log("freeze: %s (%.1f s, %u floats, %u KB em SDRAM)",
                 freeze_ok ? "pronto" : "SEM BUFFER — passthrough",
                 (double)kFreezeSeconds, (unsigned)need,
                 (unsigned)(need * sizeof(float) / 1024));
#else
        hal::Log("freeze: DESLIGADO no build (bisseccao)");
#endif
    }

    // ---- CAPACIDADE DAS TABELAS: DENUNCIA NO BOOT ----
    // Em 18/08 os dois tetos estouraram no mesmo dia, os dois EM
    // SILENCIO: `kMaxParams` recusou tres params (os buzios inteiros
    // sumiram) e `kMaxMidiMap` deixou a 17a entrada ser escrita FORA
    // DO ARRAY (o `shm_shift` chegava e nao achava dono).
    if(manifest.midi_overflow > 0)
        hal::Log("ERRO: %d entradas do mapa MIDI RECUSADAS (kMaxMidiMap)."
                 " Os ULTIMOS CCs declarados NAO respondem.",
                 manifest.midi_overflow);
    if(params.Overflow() > 0)
        hal::Log("ERRO: %d parametros RECUSADOS (kMaxParams). Os ULTIMOS"
                 " declarados NAO existem.", params.Overflow());
    hal::Log("tabelas: %d params (teto %d) | %d CCs (teto %d)",
             params.Count(), (int)cotuca::kMaxParams,
             manifest.n_midi, (int)cotuca::kMaxMidiMap);

    hal::SetSerialLineCallback(HandleLine);

    hal::StartAudio(AudioCb);
    hal::Log("Cotucatronics OS v" COTUCA_OS_VERSION " [%s] — pronto.",
             hal::Info().name);
    // Toda a SDRAM do projeto passa por AllocBig(). Este log é o extrato:
    // se um módulo alocar escondido, o número aqui não fecha.
    hal::Log("SDRAM via AllocBig: %u de %u bytes",
             (unsigned)hal::AllocBigUsed(),
             (unsigned)hal::Info().big_ram_bytes);

    // Tick de ~1 ms. Nada de bloqueio longo aqui: o serial responde na
    // hora e o LED vira heartbeat contado, não um sleep de meio segundo.
    constexpr uint32_t kLedToggleTicks = 500; // 500 ms aceso / 500 apagado

    bool     led  = false;
    uint32_t tick = 0;

    // Último valor lido de cada knob físico. Ver a DECISÃO DE
    // PRIORIDADE no topo: o knob só escreve quando MUDOU, senão ele
    // reescreveria o parâmetro a cada ~1 ms e venceria MIDI e serial
    // sempre — "última escrita vence" viraria "o knob vence", sem que
    // ninguém tivesse decidido isso.
    //
    // -2 e um valor impossivel (ReadKnob devolve -1 ou 0..1), entao a
    // primeira leitura real sempre conta como mudanca.
    float knob_last[kNumKnobs];
    for(int k = 0; k < kNumKnobs; k++)
        knob_last[k] = -2.f;

    // RASTREADOR DE FASE. O batimento provou que o loop MORRE (2816
    // ticks contra 1,07 milhao de blocos de audio), mas nao onde. Cada
    // etapa carimba o slot 12 ANTES de rodar; depois do reset, o
    // numero que sobrou e a ultima etapa que ele entrou — e a proxima
    // linha depois dela e a que trava. Uma store por etapa.
    // VOO ENXUTO (bancada 2026-08-16). A instrumentacao cresceu ate
    // ~20 escritas em backup SRAM por volta, cada uma com
    // SCB_CleanDCache_by_Addr — e o padrao observado (degrada 8x, DEPOIS
    // para) e compativel com o observador virando o assassino.
    //
    // kDiagFull=false deixa so DUAS escritas por volta: voltas e
    // carimbo. E o DelayMs sai junto, porque o outro suspeito do mesmo
    // tamanho e o relogio: DelayMs(1) que engole tick durante as
    // rajadas de 3,4 ms do callback explicaria os ~8 ms/volta sem
    // culpar o cache — e relogio quebra de vez, cache degrada linear.
    static constexpr bool kDiagFull = false;
    // ============================================================
    // DelayMs FORA DO LOOP — o assassino, nomeado por medicao.
    //
    // Dois voos, uma variavel trocada, instrumentacao identica:
    //   sem DelayMs : 8 748 683 voltas, vivo aos 58,7 s
    //   com DelayMs :        1 516 voltas, morto aos 12,7 s
    //
    // E o carimbo diz o mecanismo: ele e escrito IMEDIATAMENTE ANTES do
    // delay. O loop chegou la aos 12 674 ms e nunca mais voltou — entrou
    // e nao saiu. Com o callback do vocoder segurando a CPU em rajadas
    // de ~3,4 ms, a espera baseada em tick engole tick e em algum
    // momento nao termina.
    //
    // Por que simplesmente TIRAR e seguro, e nao remendo: o delay era
    // cortesia de CPU, e o audio roda por INTERRUPCAO — o loop girando
    // livre nao lhe rouba nada. A unica coisa que dependia da cadencia
    // do loop era a rampa do sorteio, e ela agora e funcao do relogio,
    // nao contagem de voltas. Tirar o metronomo deixou de ter efeito
    // colateral no exato patch em que o metronomo deixou de existir.
    // ============================================================
    static constexpr bool kUseDelay = false;
    // FASE publica em `loop_fase` ALEM da caixa-preta. A caixa-preta so
    // se le pela serial, e a serial morre junto com o loop: o dado
    // existia, o caminho ate ele e que estava morto.
    #define FASE(n) do { loop_fase = (uint8_t)(n); \
                        if(kDiagFull) cotuca::hal::BlackboxWrite(12, (float)(n)); } while(0)

    while(1)
    {
        // Declarado NO TOPO: quem configura vem antes de quem consome.
        const bool voando = params.Real(manifest.p_buzios_on) >= 0.5f;
        FASE(1);
        // Knobs físicos. Pares do MIDI e do serial: quem mexeu por
        // último vence. Hoje ReadKnob() devolve -1 (sem pot ligado) e
        // este laço não faz nada — a guarda de mudança existe pro dia
        // em que os pots chegarem, porque sem ela o MIDI para de
        // funcionar em silêncio no mesmo dia.
        for(int k = 0; k < hal::Info().num_knobs && k < kNumKnobs; k++)
        {
            const float v = hal::ReadKnob(k);
            if(v < 0.f)
                continue; // sem hardware
            if(v == knob_last[k])
                continue; // não mexeu: não reescreve, não atropela
            knob_last[k] = v;
            manifest.knob[k].Apply(params, v);
        }
        FASE(2);
        hal::PollSerial();

        FASE(3);
        // ---- PAINEL: botões (físico OU simulado por serial) ----
        rng_state += 0x9E3779B9u; // gira sempre: a semente vira do tick
        for(int b = 0; b < 2 && b < hal::Info().num_buttons; b++)
        {
            const bool now = hal::ReadButton(b) || (b == 1 && b2_sim);
            if(now == btn_prev[b])
                continue;
            btn_prev[b] = now;
            if(b == 0 && now)
                TriggerRandomize(false); // BORDA de subida
            else if(b == 1)
                SetButton2(now);    // NÍVEL
        }

        // BATIMENTO DO LOOP na caixa-preta (slot 10), a cada ~256
        // ticks. E o registro que separa de vez "loop vivo" de "loop
        // morto" num voo sem serial — a pergunta que custou o
        // diagnostico inteiro do búzios. Slot 11: quantos sorteios
        // o loop CONSUMIU (contra os disparos do slot 9).
        {
            static uint32_t hb = 0;
            if(kDiagFull && ((++hb) & 0x0F) == 0)
                hal::BlackboxWrite(10, (float)hb);
        }

        // BUZIOS: consome a flag armada pelo callback. A checagem
        // de buzios_on aqui é redundância barata — o detector já checa
        // — mas cobre a janela em que o operador desliga entre o
        // disparo e o consumo.
        if(buzios_fire)
        {
            buzios_fire = false;
            if(params.Real(manifest.p_buzios_on) >= 0.5f)
            {
                FASE(4);
                static uint32_t consumidos = 0;
                hal::BlackboxWrite(11, (float)++consumidos);
                TriggerRandomize(true);
            }
        }

        FASE(5);
        // avança as rampas do sorteio, um passo por tick
        const uint32_t agora = hal::NowMs();
        for(int i = 0; i < n_ramps; i++)
        {
            ParamRamp& r = ramps[i];
            if(r.done)
                continue;
            const uint32_t dt = agora - r.start_ms;
            float t = (r.dur_ms == 0) ? 1.f : (float)dt / (float)r.dur_ms;
            if(t >= 1.f) { t = 1.f; r.done = true; } // pousa EXATO no `to`
            params.Set(r.id, r.from + (r.to - r.from) * t);
            // Slot 16: quantos PASSOS de rampa realmente executaram.
            // Slot 17: o valor que o ultimo passo escreveu, e slot 18 o
            // que o params devolveu em seguida. Se 17 e 18 divergirem,
            // o Set nao esta guardando o que recebeu.
            static uint32_t passos = 0;
            if(kDiagFull)
            {
                hal::BlackboxWrite(16, (float)++passos);
                hal::BlackboxWrite(17, r.from + (r.to - r.from) * t);
                hal::BlackboxWrite(18, params.Get(r.id));
            }
            else
                hal::BlackboxWrite(16, (float)++passos);
        }
        // MIDI no mesmo lugar do serial, e pelo mesmo motivo: os bytes
        // chegam por interrupção e são despachados aqui, em contexto
        // normal, onde mexer em params e logar é seguro.
        FASE(6);
        hal::PollMidi();
        LbTick();
        loop_voltas++;
        // A janela de quarentena pode expirar sem que chegue outro
        // CC — o humano que mexeu uma vez e parou. Sem este tique o
        // gesto dele ficaria seguro pra sempre.
        QuarTick();

        // ---- AVISO DO TETO DE SOMA — dito AQUI, no loop.
        // O callback so marcou. Este e o contexto onde falar com o USB
        // e com o fio MIDI e seguro; o callback nao e, e a serial morreu
        // provando isso.
        if(aviso_teto_idx >= 0)
        {
            const int      k     = aviso_teto_idx;
            const uint32_t us    = aviso_teto_us;
            const uint32_t prazo = aviso_teto_prazo;
            aviso_teto_idx = -1;              // consome antes de falar
            if(k < manifest.n_sacrif)
            {
                hal::LogAlways("CTOS TETO DE SOMA: \"%s\" em BYPASS —"
                               " bloco %lu us de teto %lu (prazo %lu)",
                               manifest.sacrif[k].nome, (unsigned long)us,
                               (unsigned long)soma_teto_us,
                               (unsigned long)prazo);
                hal::LogAlways("  nao ha culpado: e a SOMA. A prioridade"
                               " declarada no manifest escolheu."
                               " `ctos rearm` devolve o direito de ligar.");
                const uint8_t msg[3]
                    = {0xB0, kCCAvisoTeto, (uint8_t)(k + 1)};
                hal::MidiSendBytes(msg, 3);
            }
        }
        if(aviso_sem_saida)
        {
            aviso_sem_saida = false;
            hal::LogAlways("CTOS TETO DE SOMA: estourou (%lu us de %lu) e"
                           " NAO HA o que derrubar.",
                           (unsigned long)aviso_teto_us,
                           (unsigned long)soma_teto_us);
            hal::LogAlways("  a base sozinha nao cabe no prazo — isto nao"
                           " se resolve por bypass.");
        }

        // ---- SENSOR DE RADIO. Roda logo depois do despacho, porque e
        // aqui que os contadores acabaram de ser atualizados.
        {
            const uint32_t now = hal::NowMs();
            const uint32_t b   = midi.Bytes();
            const uint32_t cc  = midi.CcCount();

            // QUEDA: silencio > 10 s DEPOIS de ter havido trafego. A
            // condicao "depois de trafego" e o que separa queda de
            // "nunca conectou" — sem ela, uma placa que nunca pareou
            // acumularia uma "queda" a cada 10 s.
            // SILENCIO NAO E QUEDA — e so silencio.
            //
            // Este contador ja mentiu uma vez (18/08): disse "provavel
            // QUEDA" quando o dB simplesmente parou de mexer no fader,
            // com o link intacto. Cicatriz 7, o rotulo que mente, e
            // escrita por mim horas depois de eu documentar a regra.
            //
            // MIDI so fala quando alguem toca. Sem keepalive chegando,
            // "sem trafego" e IDENTICO em "ninguem encostou" e "o radio
            // morreu", e nenhuma logica sobre o fluxo de RX separa os
            // dois. Quem separa e o LED da camada 1 — e por isso que a
            // camada 1 existe.
            //
            // A excecao mensuravel: se o controlador manda Active
            // Sensing (realtime), ai sim existe batimento cardiaco, e a
            // ausencia dele PASSA a significar queda. Enquanto
            // RealTime() for 0, isto conta SILENCIOS, nao quedas.
            if(ble_last_rx_ms && !ble_drop_flagged
               && (now - ble_last_rx_ms) > 10000u)
            {
                ble_drop_flagged = true;
                ble_drops++;
                hal::BlackboxWrite(86, (float)ble_drops);
                hal::BlackboxWrite(88, (float)((now - ble_last_rx_ms) / 1000u));
            }

            if(b != ble_last_bytes)
            {
                ble_last_bytes = b;
                ble_last_rx_ms = now;
                ble_drop_flagged = false; // voltou: rearma pra proxima
            }

            // O DETECTOR DE DUMP MUDOU DE LUGAR (18/08): virou o gate
            // em OnMidiCc, porque sensor que avisa depois de o comando
            // ja ter sido aplicado nao defende nada. Ver a QUARENTENA
            // DE RECONEXAO no topo deste arquivo.
        }

        // ---- DIARIO DE BORDO DO LINK MIDI ----
        //
        // Duas escritas por segundo respondem a pergunta do voo de 10
        // min: o link e ESTAVEL ou tem buracos? E se tem, QUANDO — pra
        // cruzar com o que aconteceu no iPad.
        //
        //   slots 40-51  msgs por MINUTO (12 min de historico)
        //   slots 52-83  transicoes: par (segundo, 1=voltou / 0=calou)
        //   slot  84     quantas transicoes houve
        //
        // Transicao e o dado que importa: contagem por minuto diz QUANTO,
        // mas so a transicao diz QUANDO comecou e quando acabou. Um
        // buraco de 40 s no meio de um minuto cheio some na media.
        {
            static uint32_t ult_s = 0, ult_msgs = 0, base_min = 0;
            static int      n_trans = 0;
            static bool     recebendo = false;
            const uint32_t  ag_s = hal::NowMs() / 1000u;
            if(ag_s != ult_s)
            {
                ult_s = ag_s;
                const uint32_t m  = msc_msgs;
                const bool     rx = (m != ult_msgs);
                ult_msgs = m;
                if(rx != recebendo)
                {
                    recebendo = rx;
                    if(n_trans < 16)
                    {
                        hal::BlackboxWrite(52 + n_trans * 2, (float)ag_s);
                        hal::BlackboxWrite(53 + n_trans * 2, rx ? 1.f : 0.f);
                        n_trans++;
                        hal::BlackboxWrite(84, (float)n_trans);
                    }
                }
                const uint32_t min = ag_s / 60u;
                if(min < 12)
                    hal::BlackboxWrite(40 + min, (float)(m - base_min));
                if(ag_s % 60u == 59u)
                    base_min = m;
            }
        }

        // ESPELHO CONTINUO do MIDI na caixa-preta. Nao adianta so
        // espelhar quando alguem digita `midi`: com o vocoder ligado a
        // serial ja morreu e ninguem digita nada. Uma vez por segundo,
        // custo desprezivel.
        {
            static uint32_t mid_s = 0;
            const uint32_t  ag    = hal::NowMs() / 1000u;
            if(ag != mid_s)
            {
                mid_s = ag;
                hal::BlackboxWrite(33, (float)midi.Bytes());
                hal::BlackboxWrite(34, (float)(midi.CcCount() + midi.Other()));
                hal::BlackboxWrite(35, (float)midi.Applied());
                hal::BlackboxWrite(36, (float)midi.Resync());
                hal::BlackboxWrite(37, (float)hal::MidiDropped());
                hal::BlackboxWrite(85, (float)hal::MidiHiWater()); // 41 e do diario (40+min)
                hal::BlackboxWrite(38, (float)hal::MidiErrors());
                hal::BlackboxWrite(39, (float)hal::MidiRestarts());
            }
        }

        // MSCOPE STATS: uma linha por segundo. Existe pro caso
        // fader-deslizando, em que o stream seria diluvio — e diluvio
        // no Log e exatamente o que o orcamento duro joga fora, entao
        // o stream mentiria por omissao justamente quando mais importa.
        if(mscope_mode == 2)
        {
            const uint32_t agora_s = hal::NowMs() / 1000u;
            if(agora_s != msc_last_s)
            {
                msc_last_s = agora_s;
                hal::Log("mscope: %lu msg/s | total %lu | orfaos %lu"
                         " ressync %lu | fila perdeu %lu | pico fila %lu",
                         (unsigned long)msc_win_msgs,
                         (unsigned long)msc_msgs,
                         (unsigned long)midi.Orphan(),
                         (unsigned long)midi.Resync(),
                         (unsigned long)hal::MidiDropped(),
                         (unsigned long)hal::MidiHiWater());
                for(int i = 0; i < 4 && i < midi.HistoryCount(); i++)
                    hal::Log("   cc%d=%d", midi.History(i).cc,
                             midi.History(i).value);
                msc_win_msgs = 0;
            }
        }

        // watchdog: o callback só trava a flag; o log é aqui, porque
        // hal::Log bloqueia por milissegundos e não pode rodar no
        // contexto de áudio.
        FASE(7);
        if(fx2_overrun && !fx2_reported)
        {
            fx2_reported = true;
            hal::Log("WATCHDOG: modulo do slot fx2 estourou o prazo do "
                     "bloco (%.0f%% de pico). Forcado em bypass.",
                     hal::CpuLoadMax() * 100.f);
            hal::Log("  para rearmar: set fx2_on 0, depois set fx2_on 1");
        }
        // o callback e quem arma a graca; aqui so se avisa
        {
            const bool on = params.Real(manifest.p_fx2_on) >= 0.5f;
            if(on && !fx2_was_on)
            {
                fx2_reported = false;
                hal::Log("fx2: janela de graca de %lu blocos (custo de "
                         "estreia nao conta)", (unsigned long)kGraceBlocks);
            }
            fx2_was_on = on;
        }
        // (Aqui morava o log de bancada por-4-blocos do fx2. REMOVIDO:
        // com o host desconectado, cada Log gastava o teto de 50 ms de
        // retry e chegava um novo a cada 21 ms — o loop virava zumbi
        // dentro do proprio instrumento de medicao, e foi ESSA a causa
        // de "a serial morre com o vocoder ligado". O cerco ja tinha
        // cumprido o papel; as medicoes vivem na caixa-preta.)
        if(fx2_grace_done)
        {
            fx2_grace_done = false;
            const float budget_us = 1e6f * (float)hal::Info().block_size
                                    / hal::Info().sample_rate;
            hal::Log("fx2 ESTREIA: %.0f us de pico (%.0f%% do prazo)",
                     fx2_startup_peak * budget_us, fx2_startup_peak * 100.f);
            hal::Log("  perfil bloco a bloco: comando `probe`");
            hal::Log("fx2: graca encerrada — daqui pra frente e regime");
        }

        // rearma quando o operador desliga explicitamente
        if(params.Real(manifest.p_fx2_on) < 0.5f && fx2_overrun)
        {
            fx2_overrun  = false;
            fx2_reported = false;
            hal::CpuLoadReset(); // senao o pico antigo re-trava na hora
        }

        // ARMADILHA DO PROPRIO INSTRUMENTO, custou tres voos:
        // contadores `static` ZERAM no boot, e o boot empurra os binds.
        // Entao a sessao que LE a caixa-preta reescrevia os slots antes
        // de eu olhar, e o `bb` reportava sempre "5 pushes" — o numero
        // da leitura, nao do voo. Diagnostico que se auto-apaga e pior
        // que diagnostico ausente: parece dado.
        //
        // Conserto: so grava com o voo ATIVO. buzios_on nasce em 0,
        // entao a sessao de leitura nao toca em nada.
        // empurra pro módulo o que mudou. Fora do callback de áudio de
        // propósito: setParameterValue enfileira evento no RNBO.
        for(int b = 0; b < manifest.n_bind; b++)
        {
            if(mod_index[b] < 0)
                continue;
            // Real(), não Get(): é aqui que a curva do manifest passa a
            // valer. Os params do módulo são declarados 0–1, então Real()
            // devolve o normalizado JÁ CURVADO, que é o que o DSP quer.
            // Shaped(), não Real(): o módulo quer 0..1 com a curva do
            // manifest já aplicada. Real() traz a unidade (Hz, ms),
            // que é assunto nosso e não dele.
            FASE(8);
            // Slot 19: ITERACOES do laco (nao disparos). Se iteracoes
            // >> pushes, o laco roda e a guarda `v != mod_last` e que
            // nunca abre — o que significaria Shaped() parado enquanto
            // Get() muda. Slot 20/21: o Shaped e o mod_last do bind 0,
            // pra ver os dois lados da comparacao que decide tudo.
            if(voando && kDiagFull)
            {
                static uint32_t iter = 0;
                hal::BlackboxWrite(19, (float)++iter);
            }
            const float v = params.Shaped(manifest.bind[b].param_id);
            if(voando && kDiagFull && b == 0)
            {
                hal::BlackboxWrite(20, v);
                hal::BlackboxWrite(21, mod_last[b]);
            }
            if(v != mod_last[b])
            {
                mod_last[b] = v;
                mod_slot[(int)manifest.bind[b].slot]->SetParam(mod_index[b], v);
                // Conta os pushes POR SLOT (13 = instrument, 14 = fx2) e
                // guarda o ultimo valor empurrado pro fx2 (15). Se o
                // delay randomiza e o vocoder nao, so ha dois desfechos:
                // ou o push do fx2 nao acontece (13 sobe, 14 nao), ou
                // acontece com valor certo e o patcher ignora (os dois
                // sobem e o slot 15 varia). Cada um manda pra um lado
                // completamente diferente.
                static uint32_t push_i = 0, push_f = 0;
                if(voando && kDiagFull)
                {
                    if(manifest.bind[b].slot == cotuca::ModuleSlot::Fx2)
                    {
                        hal::BlackboxWrite(14, (float)++push_f);
                        if(manifest.bind[b].param_id == manifest.p_voc_pitch)
                            hal::BlackboxWrite(15, v);
                    }
                    else
                        hal::BlackboxWrite(13, (float)++push_i);
                }
            }
        }

        if(++tick >= kLedToggleTicks)
        {
            tick = 0;
            led  = !led;
            hal::SetLed(led);
        }
        // ---- CELULA 1: VOLTAS COMPLETAS + CARIMBO DE TEMPO ----
        // FIM do while(1). O batimento (slot 10) diz que o loop CHEGOU
        // ao meio; este diz que ele FECHOU a volta. A diferenca separa
        // as duas explicacoes que o batimento sozinho confunde:
        //   voltas paradas → PRISAO (nenhuma volta fecha)
        //   voltas lentas  → STARVATION (dezenas em 30 s, nao zero)
        // O carimbo diz QUANDO parou, cruzavel com o fx2_on.
        if(voando)
        {
            static uint32_t voltas = 0;
            hal::BlackboxWrite(28, (float)++voltas);
            hal::BlackboxWrite(29, (float)hal::NowMs());
        }
        if(kUseDelay)
            hal::DelayMs(1);
    }
}
