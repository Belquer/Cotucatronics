// ============================================================
// Cotucatronics OS — esqueleto board-agnostic.
//
// A VERSAO MORA SO EM `COTUCA_OS_VERSION`, logo abaixo, e em lugar
// nenhum mais. Este cabecalho ja carregou um `v0.125.0` proprio que
// ficou quatro versoes para tras sem ninguem notar — porque ele nunca
// era impresso, e numero que ninguem le nao tem quem o corrija.
// Segunda fonte de verdade e deriva com data marcada.
// Este arquivo NÃO sabe qual placa é o cérebro, nem qual motor de
// DSP está rodando: fala com `Module`, não com o RNBO.
// Compile com: make BOARD=daisy_seed
// ============================================================

#define COTUCA_OS_VERSION "0.163.1"

// A/B DA DUPLICACAO, NO MESMO BINARIO.
//
// Existe porque delta entre builds nao e atribuivel: medindo v0.130.0
// contra v0.131.0 o pico CAIU 23 us, e "duplicar dez params economiza
// 23 us" nao tem mecanismo nenhum. Layout de codigo, alinhamento e
// inlining mudam entre binarios e engolem qualquer efeito desta escala.
//
// Com a chave, os dois bracos rodam no MESMO binario: `dup on` le
// `p_x[c]` (uma entrada por cadeia), `dup off` le `p_x[0]` (as duas
// cadeias na mesma entrada, que e o comportamento anterior a v0.131.0).
//
// O `? :` esta nos DOIS bracos, entao o custo dele cancela — a
// diferenca medida e a diferenca de leitura, e mais nada. Roda uma vez
// por bloco por efeito, nao por amostra.
//
// MODO QUE ALTERA O INSTRUMENTO: com `dup off` a cadeia B soa com os
// valores da A. Por isso se declara em `board`, `list` e `cpu`.
static bool g_dup_por_cadeia = true;
static inline int CadeiaDoParam(int c) { return g_dup_por_cadeia ? c : 0; }

#include "hal.h"
#include "params.h"
#include "manifest.h"
#include "presets.h"
#include "dynamics.h"
#include "reverb.h"
#include "freeze.h"
#include "pitch.h"
#include "effect_slot.h"
#include "shimmer.h"
#include "drive.h"
#include "phaser.h"
#include "warble.h"
// detune.h NAO e incluido: o efeito saiu do build em 20/08 (decisao de
// ouvido do dB — o warble ja cobre o territorio de modulacao de altura,
// e o detune custava 125 us, cinco vezes o phaser). O arquivo segue no
// repo, intacto e revivivel. Ver docs/FEATURES.md, secao Arquivo.
#include "suave.h"
#include "delay.h"
#include "qspi_raw.h"
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

// O canal entra no HeldCc porque a quarentena REPRODUZ o que segurou:
// guardar so (cc,valor) faria toda mensagem retida voltar mirando a
// cadeia A, e um despejo de reconexao da cadeia B reapareceria na A.
struct HeldCc { uint8_t canal, cc, value; };
static HeldCc   quar_held[kQuarHoldMax];
static int      quar_held_n   = 0;
static uint8_t  quar_seen[kQuarHoldMax];
static int      quar_seen_n   = 0;
static uint32_t quar_win      = 0; // 0 = sem janela aberta
static uint32_t quar_block_to = 0; // fim do bloqueio confirmado
static uint32_t quar_last_ms  = 0;
static uint32_t cc_quarentena = 0; // CCs recusados — visivel no `midi`

static void ApplyCcNow(uint8_t canal, uint8_t cc, uint8_t value)
{
    // O mapa é do manifest, e aplicá-lo também: ver Manifest::ApplyMidiCc.
    // CC fora do mapa devolve -1 e é ignorado de propósito — mas o
    // parser já o guardou no histórico. No dia da solda, ver um CC
    // chegando e não fazer nada é informação MUITO diferente de não
    // ver nada.
    if(manifest.ApplyMidiCc(params, canal, cc, value) >= 0)
        midi.NoteApplied(); // CC que achou dono; fora do mapa nao conta
}

static void QuarFlush()
{
    for(int i = 0; i < quar_held_n; i++)
        ApplyCcNow(quar_held[i].canal, quar_held[i].cc, quar_held[i].value);
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
// 120, NAO 34. Em 22/08 o 34 foi dado ao `drv_drive` na reorganizacao
// do mapa e ninguem cruzou as duas listas: o `OnMidiCc` intercepta o CC
// de comando ANTES do mapa e retorna, entao `drv_drive` ficou surdo a
// MIDI e cada gesto naquele fader disparava um push de estado inteiro.
// Dois donos para um numero, e o intercept vence sempre — em silencio.
// 120 fica fora de toda faixa de parametro, que e onde comando mora.
static constexpr uint8_t kCcSync = 120;

// ---- ESPELHO: a placa fala de volta, no CANAL 2 ----
//
// O LACO E O PROBLEMA CENTRAL, nao a transmissao. Se a placa manda CC
// ao mudar um param e o Loopy responde mandando CC de volta, os dois se
// empurram pra sempre. Duas defesas, e a segunda existe porque a
// primeira pode falhar num caminho que eu nao previ:
//
//   1. CANAL SEPARADO — escuta em 1 (omni), fala em 2. Estrutural: uma
//      mensagem que sai nunca se parece com uma que entra.
//   2. VALOR REPETIDO NAO SE MANDA — `eco_ult[cc]` guarda o ultimo
//      valor visto naquele CC, VENHA DE ONDE VIER. Um CC recebido
//      atualiza essa tabela, entao o espelho nao o devolve. Se as duas
//      pontas caissem no mesmo canal, o laco morreria na segunda volta
//      em vez de girar pra sempre.
// O CANAL DO ESPELHO MORA NUMA CONSTANTE SO. A versao anterior tinha
// 0xB1 escrito a mao aqui e o filtro de recepcao em lugar nenhum — e a
// separacao de canal, declarada como defesa estrutural, nunca existiu.
// Base 0: indice 1 = canal 2 no mostrador.
// O ESPELHO SAIU DO CH2 (v0.135.0). Falava no canal que virou a entrada
// da cadeia B, e a colisao obrigava a recusar aquela entrada. Agora ele
// tem canais proprios — ch3 para a cadeia A + compartilhados, ch4 para a
// cadeia B — e a placa espelha e recebe ao mesmo tempo.
//
// O status de saida se deriva do canal do VINCULO, nunca de uma
// constante solta: dois canais e uma constante e o convite para um deles
// ficar para tras no proximo remapeamento.
static inline uint8_t StatusEspelho(uint8_t canal_vinculo)
{
    return (uint8_t)(0xB0 | cotuca::CanalEspelhoDe(canal_vinculo));
}
// POR PARAM, pelo mesmo motivo do `quiet_until`: o CC deixou de
// identificar o assunto. Aqui o efeito de errar seria mais silencioso —
// o espelho acharia que ja mandou o valor da cadeia B porque a A tinha
// aquele numero, e simplesmente nao mandaria.
static uint8_t  eco_ult[cotuca::kMaxParams];  // 0xFF = nunca visto
static uint32_t eco_prox_ms = 0;
static uint32_t eco_proprio = 0;   // mensagens minhas que VOLTARAM
// CONTADOR DE TRANSMISSAO, e ele faltava. Em 22/08 eu li `eco_proprio`
// como "quantas sairam" — ele conta quantas VOLTARAM. Os dois numeros
// so coincidem se o caminho de volta for perfeito, que e exatamente a
// hipotese sob teste. Medir o retorno e chamar de envio responde a
// pergunta errada com confianca.
static uint32_t eco_tx = 0;

// ---- JANELA DE AUTORIA: depois de falar, a placa e dona do CC ----
//
// POR QUE CASAMENTO DE MENSAGEM NAO SERVE. O widget do Loopy e um
// toggle COM ESTADO: recebe a mensagem do espelho, alterna, e devolve
// o ESTADO DELE — nao a mensagem. Em fase devolve igual (eco
// idempotente, inofensivo); FORA de fase devolve o valor OPOSTO, que
// escreve no param e desfaz a mudanca autonoma. Um filtro por "isto e
// igual ao que enviei" pegaria so o inofensivo e deixaria passar
// justamente o que destroi.
//
// O criterio nao e "o que eu enviei": e "o ASSUNTO sobre o qual eu
// acabei de falar". Transmitiu o CC n, ignora qualquer CC n que chegue
// — qualquer valor, qualquer canal — por 250 ms.
//
// NAO TOCA EM GESTO HUMANO: a janela so abre quando o espelho
// transmite, e o espelho so transmite mudanca AUTONOMA (`eco_ult` ja
// impede que o que veio de MIDI volte). Gesto normal nunca encontra
// janela aberta — ela so existe depois de boot-push, queda do teto,
// buzios, ou serial.
//
// TRADE DECLARADO: gesto humano no mesmo CC dentro de 250 ms de uma
// mudanca autonoma DAQUELE CC e descartado. Raro, e e a colisao que a
// regra arbitra — a maquina acabou de falar, ela vence o instante.
// Se 250 estiver errado, e numero, nao arquitetura.
static constexpr uint32_t kJanelaMs = 250;
// INDEXADA POR PARAM, NAO POR CC (v0.133.0).
//
// Com a multiplexacao o CC deixou de identificar o parametro: o CC 30
// no ch1 e o CC 30 no ch2 sao params diferentes. Indexar por CC faria a
// janela do drive da cadeia A silenciar o drive da cadeia B.
//
// De brinde, ficou mais precisa do que era: dois params que
// compartilhassem CC (o desenho permitia, ainda que nao acontecesse) se
// silenciavam juntos. Agora o assunto e o param, que e o que a regra
// sempre quis dizer com "o assunto sobre o qual eu acabei de falar".
static uint32_t quiet_until[cotuca::kMaxParams] = {0};
static uint32_t cc_janela = 0;   // recusados pela janela
// CHAVE DO ESPELHO. Nasceu no voo A para desfazer uma colisao de canal
// que nao existe mais — e a colisao morreu, mas a chave FICA, por outra
// razao, declarada aqui para que ninguem a leia como vestigio:
//
// silenciar o espelho e o unico jeito de medir carga de MIDI sem a
// propria medicao gerar trafego, e de isolar "o widget nao recebe" de
// "o widget recebe e ignora". E ganho de teste, da mesma familia do
// `ctos prazo` e do `dup on|off`.
//
// O que saiu junto com o interino foi o ACOPLAMENTO: a entrada nao
// depende mais desta chave. Ela so cala a saida.
// ---- BIBLIOTECA DE PRESETS (em RAM) ----
//
// Mora em `.bss`, que neste APP_TYPE cai na SRAM/AXI — escolha
// EXPLICITA e nao omissao: a biblioteca e memoria FRIA (lida no `load`,
// escrita no `capturar`, nunca tocada no callback), e a DTCM, que esta
// vazia, e a unica RAM que ainda pode salvar um caminho quente. Gastar
// a rapida com dado frio seria desperdicar a que nao da pra recuperar.
static cotuca::PresetMapa  pmapa;
static cotuca::Biblioteca  biblio;
static uint16_t            mapa_hash = 0;

// Marca sujo para o espelho escoar. O `load` NAO usa a rota do `sync`:
// 3 ms por mensagem x 86 sao 258 ms de laco bloqueado, e o espelho
// normal escoa duas a cada 8 ms sem parar nada.
static void AbreJanelaEEspelha(int id)
{
    using namespace cotuca;
    if(id < 0 || id >= cotuca::kMaxParams) return;
    // A JANELA ABRE AGORA, PARA TODOS, e nao a medida que o espelho
    // manda. O gesto autonomo e o `load`, nao cada mensagem: se a janela
    // abrisse so na saida, o widget que ecoasse no milissegundo 5 —
    // antes de a mensagem dele sair — seria obedecido, e o preset seria
    // desfeito pela metade.
    quiet_until[id] = hal::NowMs() + kJanelaMs;
    eco_ult[id]     = 0xFF;   // forca o espelho a reenviar este param
}

// ---- CONFERENCIA DO SYNC ----
//
// O `sync` deixou de ser "mandei" e passou a ser "chegou". A diferenca
// importa porque a falha mais comum aqui e MUDA: o WIDI desligado, o
// Loopy fora da tela, ou os widgets escutando o canal antigo — nos tres
// a placa manda 86 CCs no vazio e, sem conferencia, diz "pronto".
//
// O INSTRUMENTO JA EXISTIA e e a propria defesa: se o Loopy recebe, os
// widgets se movem e devolvem o estado deles pelo ch1/ch2. A janela de
// autoria barra esse eco (e por isso que ela existe) — e o CONTA. Entao
// o retorno do controlador e exatamente o que a janela registra.
//
// A conferencia e ASSINCRONA de proposito: bloquear aqui impediria o
// `hal::PollMidi()` de rodar, e o eco que se quer medir chegaria numa
// fila que ninguem esvazia. O laco confere quando o prazo vence e o
// veredito sai sozinho — sem cronometro para o operador cumprir.
static uint32_t sync_confere_ate = 0;
static uint32_t sync_cc_antes    = 0;
static uint32_t sync_jan_antes   = 0;
static int      sync_mandados    = 0;
static constexpr uint32_t kSyncEsperaMs = 1500;

// TRAVA DO CAMINHO DE VOLTA — "ja ouvi o controlador alguma vez?"
//
// O AVISO do `sync` listava tres causas para o mesmo silencio e nao
// distinguia nenhuma. Mas a placa PODE distinguir uma delas: se algum CC
// ja chegou por um canal de entrada desde o boot, o fio de volta
// (WIDI OUT -> D14) esta bom, e o silencio de agora e dos WIDGETS.
//
// Mesma forma da trava do `pino`, que inocentou uma solda quando o
// contador dizia zero: acumular desde o boot em vez de medir numa
// janela. A pergunta "ja funcionou alguma vez?" nao tem relogio dentro.
static uint32_t ouvi_controlador = 0;
// E O DESCONTO DAS INJECOES, porque o `miditest` entra na MESMA fila do
// fio real e o parser nao os distingue.
//
// Sem isto a trava mente com a maior autoridade possivel: eu exercitei
// o ramo "ja funcionou" com uma injecao local e a placa afirmou que o
// fio de volta estava bom. Instrumento que o proprio exercicio engana
// nao e instrumento — e a 13x outra vez, agora do lado de quem mede.
//
// O relatorio mostra os DOIS numeros. O veredito usa so o real.
static uint32_t injetados_local = 0;
static inline uint32_t OuviDoFio()
{
    return ouvi_controlador > injetados_local
               ? ouvi_controlador - injetados_local : 0;
}

// ---- ENCODERS: acumulacao, aceleracao e o instrumento ----
//
// TRES ESTADOS COMPLETAMENTE SEPARADOS. Nenhum campo compartilhado entre
// encoders — giro simultaneo contaminado daria sintoma intermitente, que
// e o mais caro de diagnosticar.
//
// O `passos_crus` conta o que a libDaisy entregou, SEM divisao por
// detente. A divisao se mede girando dez cliques e conferindo dez — nao
// se le no datasheet, porque EC11 varia entre lotes.
struct EstadoEnc
{
    int32_t  crus       = 0;   // incrementos brutos acumulados
    int32_t  passos     = 0;   // depois da divisao por detente
    int32_t  acelerados = 0;   // depois da aceleracao por tempo
    int      resto      = 0;   // sobra da divisao, para nao perder passo
    int      ultimo_dir = 0;   // -1, 0, +1
    uint32_t ultimo_ms  = 0;   // para a aceleracao POR TEMPO
    uint32_t pushes     = 0;   // CONTAGEM, nao paridade (bts 13e)
    bool     push_agora = false;
    uint32_t rapidos    = 0;   // passos que a aceleracao multiplicou
};
static EstadoEnc enc_st[cotuca::hal::kNumEncoders];

// DIVISAO POR DETENTE — nasce em 1 e se ajusta DEPOIS DE MEDIR.
//
// Alguns EC11 dao dois ou quatro pulsos por clique. Cravar 4 "porque
// costuma ser" produziria um instrumento que mente por suposicao logo no
// voo que existe para medir. Comeca em 1, o `enc` mostra o cru, e o
// numero certo entra quando o dB girar dez e contar.
static int enc_por_detente = 1;

// ESPELHO DO ENCODER NA TELA — o mesmo instrumento, outra superficie.
//
// Nao e navegacao (isso e o passo 4): e o `enc` desenhado, para que a
// medicao dos dez detentes se faca olhando o knob e nao a serial.
//
// SO REDESENHA QUANDO O TEXTO MUDA, e com teto de taxa. A escrita e
// BLOQUEANTE (~1 KB por Update a PS_64) — redesenhar a cada volta do
// laco poria milissegundos de bloqueio no caminho do MIDI e do serial
// sem nenhum ganho, porque a tela nao muda mais rapido que o olho le.
static bool     enc_na_tela   = true;
static uint32_t enc_tela_prox = 0;
static char     enc_tela_ant[3][24] = {{0},{0},{0}};

// ACELERACAO POR TEMPO ENTRE PASSOS, nao por contagem bruta.
//
// Por contagem, um giro lento e longo acelera igual a um rapido e curto —
// e o gesto do operador e o oposto: rapido quer percorrer, lento quer
// precisar. O tempo entre passos captura a INTENCAO; a contagem captura
// so a distancia.
static constexpr uint32_t kEncRapidoMs = 40;   // abaixo disto, e varredura
static constexpr uint32_t kEncTurboMs  = 15;   // muito rapido
static constexpr int      kEncMultRap  = 4;
static constexpr int      kEncMultTurbo = 10;

// Aceleracao aplicada NO PASSO, com o intervalo desde o passo anterior.
static void AplicaAcel(EstadoEnc& st, int dir)
{
    using namespace cotuca;
    const uint32_t ag = hal::NowMs();
    const uint32_t dt = st.ultimo_ms ? (ag - st.ultimo_ms) : 0xFFFFFFFFu;
    st.ultimo_ms = ag;
    int mult = 1;
    if(dt <= kEncTurboMs)       { mult = kEncMultTurbo; st.rapidos++; }
    else if(dt <= kEncRapidoMs) { mult = kEncMultRap;   st.rapidos++; }
    st.acelerados += dir * mult;
}

static bool espelho_on = true;

// DRENAGEM DE BOOT — a travessia, que e o caso que a drenagem do voo A
// nao cobria e eu quase deixei passar.
//
// Raciocinei sobre o ESTADO FINAL (ch3/ch4 sem vinculo, logo inofensivos)
// e esqueci a MIGRACAO. Ao gravar, a placa reinicia; o WIDI e o iPad
// nao. Mensagens de espelho da versao ANTERIOR, que falava no ch2,
// chegam na versao nova pelo ch2 — que agora e entrada plena da cadeia
// B. CC 34 com o valor de `drv_drive_a` escrevendo `drv_drive_b`: o
// fantasma da 13n, na unica janela em que ele ainda cabe.
//
// A regra geral, que e por que isto fica mesmo depois da migracao
// terminar: NO BOOT, TUDO QUE ESTA NO FIO FOI ENDERECADO A UMA
// CONFIGURACAO QUE PODE NAO SER ESTA. O mapa muda entre versoes, e uma
// mensagem em voo nao sabe disso.
//
// 300 ms pelo mesmo dimensionamento do voo A (31250 baud, ~1 ms por
// mensagem, rajadas de duas a cada 8 ms) — e aqui ha folga extra de
// graca, porque ninguem move um fader de proposito nos primeiros 300 ms
// de um boot. Se mover, o contador diz.
static constexpr uint32_t kDrenoBootMs = 300;
static uint32_t boot_dreno_ate = 0;
static uint32_t cc_dreno_boot  = 0;
static uint32_t cc_canal_errado = 0;  // canal sem vinculo nenhum
static uint32_t cc_sem_dono = 0;      // (canal,cc) fora do mapa

// INVENTARIO: QUAIS CCs JA CHEGARAM, um bit por numero.
//
// O historico de mensagens nao serve pra esta pergunta e eu levei duas
// tentativas pra ver: ele guarda as N ULTIMAS MENSAGENS, e varrer um
// fader manda dezenas. Aumentei de 8 pra 96 achando que 96 > 76
// widgets — mas a unidade estava errada, e os 96 slots se encheram com
// os ultimos nove controles. Historico responde "o que acabou de
// chegar"; inventario responde "o que ja chegou alguma vez". Duas
// perguntas, duas estruturas.
// POR (CANAL, CC): o CC sozinho parou de identificar param, entao um
// inventario por CC diria "cc30 chegou" sem dizer QUAL drive chegou —
// que e a pergunta toda. Quatro canais cobrem entrada (1,2) e o espelho
// (3,4) do voo B, e custam 512 bytes contra os 128 de antes.
static constexpr int kCanaisInv = 8;
static uint8_t cc_visto[kCanaisInv][128] = {{0}};   // contagem, satura em 255

// Subtracao ANTES da comparacao: uint32 de ms volta a zero em 49 dias
// e `agora < limite` mentiria por 250 ms uma vez por volta. Esta base
// ja perdeu um watchdog em 4 277 071 629 us por isso.
static inline bool NaJanela(int param, uint32_t agora)
{
    if(param < 0 || param >= cotuca::kMaxParams) return false;
    return (int32_t)(agora - quiet_until[param]) < 0;
}

// Chamado do LOOP, nunca do callback. A regra ja custou uma placa
// pendurada: MidiSendBytes de dentro do audio starva o ISR do USB.
static void MidiEspelha()
{
    using namespace cotuca;
    if(!espelho_on) return;
    const uint32_t ag = hal::NowMs();
    if((int32_t)(ag - eco_prox_ms) < 0) return;
    int n = 0;
    for(int i = 0; i < manifest.n_midi && n < 2; i++)
    {
        const int id = manifest.midi_map[i].param_id;
        if(id < 0) continue;
        // SEM FILTRO: cada vinculo fala no canal de espelho da SUA
        // cadeia. O filtro do voo A existia porque havia um canal so —
        // era a compressao de dois estados num, e o preco era a cadeia B
        // invisivel ao iPad.
        const uint8_t cc = manifest.midi_map[i].cc;
        float norm = params.Get(id);
        if(norm < 0.f) norm = 0.f;
        if(norm > 1.f) norm = 1.f;
        const uint8_t v = (uint8_t)(norm * 127.f + 0.5f);
        if(eco_ult[id] == v) continue;
        eco_ult[id] = v;
        const uint8_t msg[3]
            = {StatusEspelho(manifest.midi_map[i].canal), cc, v};
        hal::MidiSendBytes(msg, 3);
        quiet_until[id] = ag + kJanelaMs;   // falei deste assunto
        eco_tx++;
        n++;
    }
    // A 31250 baud tres bytes levam ~1 ms. Duas por janela de 8 ms
    // segura o pior caso (varredura de fader) bem abaixo do fio.
    if(n) eco_prox_ms = ag + 8u;
}

// ---- MODO LEARN: a placa percorre o mapa pra o Loopy aprender ----
//
// Endereca as quatro paginas sem digitar CC a CC. Anda na ordem do
// mapa, que e a ordem das familias — casa com as paginas.
//
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
        const uint8_t msg[3] = {StatusEspelho(manifest.midi_map[i].canal),
                                manifest.midi_map[i].cc, v};
        eco_ult[id] = v;  // nao reenviar depois
        hal::MidiSendBytes(msg, 3);
        // (a janela por mensagem fica, mas NAO basta — ver o
        //  re-carimbo depois do laco, que e o que realmente protege)
        //
        // A JANELA DE AUTORIA ABRE AQUI TAMBEM, e faltava.
        //
        // `MidiEspelha` escrevia `quiet_until` a cada mensagem; esta
        // funcao mandava e nao escrevia nenhuma. Defesa aplicada num
        // caminho e nao no irmao (bts 13q) — e o irmao aqui e a OUTRA
        // rota de saida, que e a mais barulhenta das duas.
        //
        // A consequencia era concreta: widget de toggle devolve o
        // ESTADO DELE, que e o motivo de a janela existir. Depois de um
        // `sync`, a placa obedecia o eco de tudo que acabara de mandar.
        quiet_until[id] = hal::NowMs() + kJanelaMs;
        // Respira entre mensagens: a 31250 baud tres bytes levam ~1 ms.
        //
        // O COMENTARIO DIZIA "12 CCs" e foi escrito quando o mapa tinha
        // 12. Sao 86 hoje, e 3 ms cada dao 258 ms de laco bloqueado —
        // regressao por CRESCIMENTO, nao por edicao (bts 13p). Fica
        // registrado aqui; o `load` NAO usa esta rota justamente por
        // isso, e escoa pelo espelho normal.
        hal::DelayMs(3);
        n++;
    }
    // RE-CARIMBO DA JANELA, PARA TODOS, DEPOIS DO ULTIMO ENVIO.
    //
    // A janela por mensagem nao bastava e a conferencia mostrou: o
    // despejo leva ~258 ms (86 x 3 ms) e a janela dura 250, entao a do
    // PRIMEIRO param expira antes de o ULTIMO sair. Um controlador que
    // responde em lote — recebe tudo, depois devolve tudo — chega com os
    // primeiros ja desprotegidos.
    //
    // Medido com eco simulado: "0 barrados pela janela" com cinco ecos
    // chegando. A defesa estava de pe e nao pegava nada.
    //
    // E o mesmo raciocinio que eu escrevi para o `load` e nao apliquei
    // aqui — a janela conta do FIM do gesto, nao do meio dele, porque o
    // gesto autonomo e o `sync` inteiro e nao cada mensagem. Irmao
    // desprotegido de novo (bts 13q).
    {
        const uint32_t ag = hal::NowMs();
        for(int i = 0; i < manifest.n_midi; i++)
        {
            const int id = manifest.midi_map[i].param_id;
            if(id >= 0 && id < cotuca::kMaxParams)
                quiet_until[id] = ag + kJanelaMs;
        }
    }
    hal::Log("sync: %d CCs empurrados pro controlador — os faders do"
             " Loopy devem pular pro estado da placa", n);
    hal::Log("     janela de autoria aberta para os %d params por %lu ms:"
             " o eco dos widgets nao desfaz o que acabou de sair",
             n, (unsigned long)kJanelaMs);
    // Arma a conferencia. Os contadores sao lidos AGORA, depois do
    // despejo, para que o que chegar dali em diante seja resposta a
    // ELE — e nao trafego que ja estava no fio.
    sync_mandados    = n;
    sync_cc_antes    = midi.CcCount();
    sync_jan_antes   = cc_janela;
    sync_confere_ate = hal::NowMs() + kSyncEsperaMs;
    hal::Log("     conferindo o retorno por %lu ms — o veredito sai sozinho,"
             " nao precisa fazer nada", (unsigned long)kSyncEsperaMs);
}

static void OnMidiCc(uint8_t ch, uint8_t cc, uint8_t value)
{
    using namespace cotuca;
    // A PLACA NAO OBEDECE A PROPRIA VOZ.
    //
    // Medido em 22/08 com o `midi`: cada mensagem que saia no canal 2
    // voltava pelo RX e era ACEITA, porque o receptor esta em omni —
    // omni inclui o canal em que ela mesma fala. A separacao de canal
    // estava declarada no cabecalho e nao existia no codigo: metade de
    // uma defesa e uma defesa orfa, e esta deixou o laco de pe.
    // A RECUSA CONDICIONAL DO CH2 SAIU JUNTO COM A AMEACA (v0.135.0).
    //
    // Ela existia porque o espelho falava no ch2 e a placa obedeceria a
    // propria voz. Com o espelho no ch3/ch4 o ch2 e SO entrada, e manter
    // a recusa deixaria a cadeia B pela metade — defesa orfa e a que
    // sobrevive ao inimigo.
    //
    // A voz propria continua barrada, e por um caminho mais simples: os
    // canais 3 e 4 NAO TEM VINCULO NENHUM no mapa, entao caem na recusa
    // por canal logo abaixo, contados. Endereco que nao existe nao
    // precisa de defesa dedicada.
    //
    // E o que protege contra o laco de verdade nao e canal, e a JANELA
    // DE AUTORIA: o widget ouve o ch3, reage, e manda no ch1 — mesmo
    // canal de sempre. Nenhuma separacao de canal pega isso; so a janela
    // por param pega.
    if(cotuca::EhCanalEspelho(ch)) { eco_proprio++; return; }
    // O fio ainda carrega o que foi enderecado a versao anterior.
    if(boot_dreno_ate && (int32_t)(hal::NowMs() - boot_dreno_ate) < 0)
    { cc_dreno_boot++; return; }
    // CANAL SEM VINCULO NENHUM: recusado e CONTADO.
    //
    // No voo A so 1 e 2 endereçam. 3 e 4 ainda nao existem como saida do
    // espelho, mas a recusa ja vale para eles e para qualquer outro —
    // defesa que so nasce junto com a ameaca chega atrasada, e esta e
    // barata porque o canal virou parte do endereço: um CC que chega
    // num canal sem vinculo simplesmente nao acha dono.
    if(!cotuca::EhCanalEntrada(ch))
    {
        cc_canal_errado++;
        if(ch < kCanaisInv && cc_visto[ch][cc] < 255) cc_visto[ch][cc]++;
        return;
    }
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

    // Registrado ANTES de qualquer recusa: a pergunta que esta trava
    // responde e "o fio de volta funciona?", e ela nao depende de o
    // firmware ter obedecido a mensagem.
    ouvi_controlador++;

    const uint32_t now = hal::NowMs();

    // O PARAM SE RESOLVE ANTES DA JANELA, e a ordem inverteu de proposito.
    //
    // Antes a janela era consultada por CC e o param so aparecia la na
    // frente. Com a multiplexacao o CC nao identifica mais o assunto, e
    // consultar a janela antes de saber o param silenciaria a cadeia
    // errada — o drive da A calando o drive da B.
    const int pid = manifest.FindMidiParam(ch, cc);

    // INVENTARIO por (canal, cc): registrado ANTES de qualquer recusa,
    // porque a pergunta que ele responde e "isto chegou alguma vez", e
    // essa pergunta nao depende de o firmware ter obedecido.
    if(ch < kCanaisInv && cc_visto[ch][cc] < 255) cc_visto[ch][cc]++;

    // CC que chega num canal sem vinculo: nao move nada, e e CONTADO.
    // Sem o contador isto seria indistinguivel de "o widget nao mandou",
    // que e o erro que custou uma sessao inteira em 22/08.
    if(pid < 0) { cc_sem_dono++; return; }

    // JANELA DE AUTORIA, agora por PARAM. Descarte CONTADO — descarte
    // mudo e a familia proibida: defesa que trabalha calada e
    // indistinguivel de defesa que nao existe, e foi assim que o laco do
    // canal 2 passou.
    if(NaJanela(pid, now)) { cc_janela++; return; }
    // Passou a janela: o Loopy e o autor deste valor e ja o conhece,
    // entao o espelho nao deve devolve-lo.
    eco_ult[pid] = value;

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
            quar_held[quar_held_n++] = {ch, cc, value};

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
            // CARIMBA A BASE DA SESSAO. Ate 22/08 este contador era
            // LIDO e nunca ESCRITO: o `ble` imprimia
            // `CcCount() - ble_session_base` com a base congelada em
            // zero, ou seja, o TOTAL rotulado como "msgs da sessao".
            // Pior que zero-com-cara-de-dado — numero plausivel e
            // errado, que ninguem tem como desconfiar. Uma reconexao
            // e o inicio de sessao que o rotulo sempre prometeu.
            ble_session_base = midi.CcCount();
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

    ApplyCcNow(ch, cc, value);
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
        const int     pid = manifest.FindMidiParam((uint8_t)(status & 0x0F), cc);
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



// Fonte que alimenta o módulo: crossfade das entradas + tom de teste.
// Estático porque o callback de áudio não aloca. 64 > block_size (48).
// TEM que ser >= ao block_size da placa. Com 64 e bloco 128, o clamp
// abaixo processaria metade do bloco e deixaria a outra metade do
// buffer de saida com lixo do bloco anterior — sem erro, com ruido.
// >= ao maior block_size que o manifest pode pedir (48/128/256).
static constexpr size_t kMaxBlock = 256;
static float            src_buf[kMaxBlock];


// REVERB — bloco permanente do shell, não módulo. Ver reverb.h pras
// duas razões (pool de SRAM a 92,4% e a natureza fim-de-linha).
// Pico de saida por balde de pitch (13 semitons da cerca dos buzios).

// ---- ESPACO POR CADEIA ----
//
// DUAS SALAS DE VERDADE, e a razao e que shimmer NAO e um efeito ao
// lado do reverb: ele E o reverb realimentado com pitch. Pra cadeia A
// ter shimmer e a B ter sala limpa, cada uma precisa da propria — nao
// ha como uma sala so ser molhada de dois jeitos ao mesmo tempo.
//
// Custo medido: +99 us a sala, +182 us o shimmer. Pior caso (as duas
// com shimmer) da 807 us contra teto de 850 — cabe, e o teto de soma
// existe exatamente pra esse limite.
struct Espaco
{
    cotuca::Reverb      reverb;
    bool                reverb_ok = false;
    cotuca::Shimmer     shimmer;
    cotuca::EffectSlot  shimmer_slot;
    cotuca::PitchEngine pitch;      // motor proprio: o shimmer le a cauda
    bool                pitch_ok  = false;
    float*              pitch_mem = nullptr;
    cotuca::Suave       suave;      // rampa da chave da sala
    uint32_t            pico_us   = 0;
};
static Espaco esp[cotuca::Manifest::kCadeias];
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
// UM FREEZE POR CADEIA. Cada uma congela SEU material no SEU instante;
// um freeze so, alimentado pela soma, congelaria os dois juntos — e o
// gesto e justamente pegar um material enquanto o outro segue vivo.
static cotuca::Freeze freeze_c[cotuca::Manifest::kCadeias];
static bool           freeze_c_ok[cotuca::Manifest::kCadeias] = {};
static float*         freeze_c_mem[cotuca::Manifest::kCadeias] = {};
static cotuca::Freeze freeze;
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

// PECA 2 EM VALIDACAO: o motor de pitch e o primeiro efeito a rodar
// dentro de um EffectSlot. Escolhido em vez do freeze porque o freeze
// tem cauda de 2 s aprovada de ouvido, e o fade do slot e outra coisa
// (bypass de 8 ms) — envolver o freeze agora poria duas rampas
// disputando o mesmo gesto.
//
// O slot entrega tres coisas de uma vez: fade sem clique, custo ZERO
// desligado, e a cronometragem por efeito que faltava (o `ctos` media
// so o freeze).

// ---- SHIMMER: motor PROPRIO, no ponto de derivacao da cauda ----
//
// Motor separado do `pitch` porque "compartilhado" e afirmacao sobre a
// FONTE, nao sobre o recurso (docs/CTOS.md §5b, correcao de 18/08). O
// shimmer grava a SAIDA DO REVERB — e a cauda que sobe de oitava e
// realimenta; detune e warble vao gravar o seco, noutro ponto.
static cotuca::PitchEngine pitch_tail;
static bool                pitch_tail_ok  = false;
static cotuca::Shimmer     shimmer;
static cotuca::EffectSlot  shimmer_slot;

// DRIVE — primeiro da cadeia, zero memoria. Fecha a classe "sem
// historico" e valida o EffectSlot nela.
static cotuca::EffectSlot drive_slot;

// PHASER — depois do drive, antes do delay. Zero buffer, mas N
// multiplicacoes por amostra: a regua do drive vale, trabalho nao e
// memoria. `bypass_style: cut` — sem cauda, corta seco (nao ha nada
// decaindo pra deixar terminar).
static cotuca::EffectSlot phaser_slot;

// WARBLE — deriva de fita. Buffer minusculo (40 ms mono = 7,5 KB), em
// SRAM: acesso e uma leitura interpolada por amostra, perto da cabeca
// de escrita, entao cabe na cache sem esforco.
static cotuca::EffectSlot warble_slot;

// DETUNE — motor PROPRIO no ponto pos-modulo. Terceiro ponto de
// derivacao (seco / cauda do reverb / pos-modulo), e o unico dos cinco
// que consome DUAS vozes do mesmo buffer: o caso que o "compartilhado"
// previa desde o desenho.
// I/O: balanco da entrada e largura da saida. Nascem no NEUTRO (1.0)
// pra nao produzirem um fade-in de estreia que ninguem pediu.
static cotuca::Suave       suave_gl, suave_gr;

// DELAY EM C++ — substitui o modulo RNBO em 20/08. Era o ultimo
// consumidor do runtime RNBO no instrumento. Ver delay.h: o grafo do
// patch foi lido do export e reproduzido, feedback 0.4 incluido.
static cotuca::Delay       delay_fx;
static bool                delay_ok  = false;
static float*              delay_mem = nullptr;
static constexpr float     kDelaySeg = 1.0f;   // teto do tempo
static cotuca::Suave       suave_dly, suave_rev;

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
// QUAL CRITERIO DERRUBOU. Com dois criterios agindo, "CAIU pelo teto"
// para ambos e relatorio que perde a informacao mais util: carga
// SUSTENTADA acima do orcamento e PICOS estourando o prazo pedem
// consertos diferentes, e o registro nao pode confundi-los.
static bool     soma_caiu_est[cotuca::Manifest::kMaxSacrificaveis] = {false};
static bool     soma_sem_saida = false;

// ---- ESTOUROS DE PRAZO: contagem, nao maximo ----
//
// O CTOS guardava o PICO e mais nada. Um pico de 1035 us num prazo de
// 1000 pode ser evento unico (uma varredura de fader, o `sync`
// empurrando 76 CCs) ou pode ser rotina — diagnosticos OPOSTOS com o
// mesmo numero. Maximo sem denominador nao decide politica.
//
// POR REGIME, NAO GLOBAL: um contador que atravessa mudancas de
// configuracao mistura o perfil leve com o pesado e volta a ser numero
// sem denominador. Zera junto com o resto do CTOS, e cada estouro
// carimba QUAIS efeitos estavam no ar — se eles coincidirem com
// eventos discretos, o carimbo os entrega.
static uint32_t est_n        = 0;   // quantos passaram do prazo
static uint32_t est_ult_ms   = 0;   // carimbo do ultimo
static uint32_t est_ult_us   = 0;   // quanto foi o ultimo
static uint32_t est_pior_us  = 0;   // o pior desde o reset
static uint16_t est_perfil   = 0;   // efeitos no ar no ultimo estouro
static uint32_t est_prim_ms  = 0;   // carimbo do primeiro
// A TRAVA TEM QUE FALAR. Enquanto `soma_caiu[k]` estiver de pe, o
// callback zera aquele `_on` a cada bloco: o `set` do operador e
// desfeito 3000 vezes por segundo, o `list` mostra 0, e esse 0 e
// visualmente identico ao de um efeito honestamente desligado. A
// diferenca entre "eu desliguei" e "o CTOS derrubou" morava numa
// variavel que ninguem via — e custou uma sessao inteira procurando
// defeito em drive, phaser e shimmer que estavam todos intactos.
// Devolve o indice de sacrificio, ou -1 se o param nao esta travado.
static int SacrifTravado(int param_id)
{
    for(int k = 0; k < manifest.n_sacrif; k++)
        if(manifest.sacrif[k].param_on == param_id && soma_caiu[k])
            return k;
    return -1;
}

// REARME PELO BOTAO — E ASSIMETRICO EM RELACAO AO SERIAL, DE PROPOSITO.
// `ctos rearm` solta a trava e NAO religa: e comando administrativo, a
// frio, com o operador na frente de um terminal, e religar decidiria
// por ele qual configuracao ele queria de volta.
//
// O botao e o oposto disso: e o dB no meio da performance, sem
// computador. Se ele apertar e nada voltar a soar, o botao esta
// quebrado do ponto de vista dele. E aqui NAO ha ambiguidade sobre o
// que devolver — `soma_caiu[k]` so fica de pe pra efeito que ESTAVA
// LIGADO quando o teto o derrubou. Devolver isso nao e escolher por
// ninguem: e desfazer o que o mecanismo fez.
//
// Se nao couber de novo, cai de novo em menos de um segundo e o LED
// volta a bater rapido. Isso e informacao honesta, nao falha.
static int soma_rearmes_botao = 0;
static int RearmaCtosDevolvendo()
{
    int n = 0;
    for(int k = 0; k < manifest.n_sacrif; k++)
        if(soma_caiu[k])
        {
            params.SetReal(manifest.sacrif[k].param_on, 1.f);
            soma_caiu[k] = false;
            n++;
        }
    soma_sem_saida = false;
    soma_seguidos  = 0;
    soma_carencia  = 0;
    soma_pico_us   = 0;
    if(n) soma_rearmes_botao++;
    return n;
}

// Quantos efeitos estao travados agora. O LED do loop le isto.
static int SomaTravados()
{
    int n = 0;
    for(int k = 0; k < manifest.n_sacrif; k++)
        if(soma_caiu[k]) n++;
    return n;
}
// Estourou e nao sobrou nada pra derrubar: a base sozinha nao cabe.
// Estado DIFERENTE de "tudo bem", e tem que se declarar — senao o
// silencio do mecanismo seria lido como sucesso dele.
// (declarado acima, junto de soma_caiu — RearmaCtosDevolvendo usa)
// ---- CC DE AVISO NA SAIDA MIDI ----
//
// A placa MANDA este CC quando decide sozinha derrubar um efeito. E o
// unico CC do instrumento que anda na direcao da saida: todos os outros
// sao o dB falando com a placa; este e a placa falando com o dB.
//
// DOIS VALORES, NAO UM INDICE (decisao do dB, 20/08):
//   127 = teto de SOMA — N efeitos saudaveis nao cabem juntos
//    64 = watchdog POR-EFEITO — o shimmer sozinho estourou o orcamento
//
// A versao anterior mandava `indice+1` do efeito derrubado, o que
// carregava mais informacao e servia pior: no palco o dB nao le um
// numero, ve um indicador. Dois valores bem separados sao duas posicoes
// distintas de lampada. QUAL efeito caiu segue no log, que e onde essa
// pergunta se faz — depois, com tempo.
//
// E sao dois valores porque sao DUAS DOENCAS: no teto de soma nao ha
// culpado (excesso de ambicao), no watchdog ha (o efeito estourou
// sozinho). Um indicador so, com dois estados, cobre os dois sem
// fundi-los.
static constexpr uint8_t kCCAviso     = 119;
static constexpr uint8_t kAvisoSoma   = 127;
static constexpr uint8_t kAvisoEfeito = 64;
// AVISO DO AUTO-VETO DO SHIMMER. O tripwire por-efeito derrubava o
// shimmer e nao dizia nada: o dB veria o som sumir sem uma linha de
// log. O teto de soma sempre teve aviso completo; este ficou de uma
// era anterior, e e a mesma familia "falha em silencio" dentro do
// proprio CTOS. Callback MARCA, loop FALA (regra da 11x).
static volatile bool     aviso_shm_kill = false;
static volatile uint32_t aviso_shm_us   = 0;
static volatile uint32_t aviso_shm_prazo = 0;
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

// ---- SACRIFICIO, UM SO LUGAR ----
// Dois criterios chamam isto: o de SEQUENCIA (blocos seguidos acima do
// teto) e o de ESTOURO (blocos acima do PRAZO numa janela). Duplicar a
// logica faria os dois divergirem no primeiro conserto — e a ordem de
// prioridade e a coisa que menos pode ter duas versoes.
static bool SacrificaUm(uint32_t us, uint32_t prazo, bool por_estouro)
{
    int alvo = -1;
    for(int k = 0; k < manifest.n_sacrif; k++)
        if(!soma_caiu[k]
           && params.Real(manifest.sacrif[k].param_on) >= 0.5f)
        { alvo = k; break; }
    if(alvo < 0)
    {
        if(!soma_sem_saida)
        {
            soma_sem_saida  = true;
            aviso_sem_saida = true;
            aviso_teto_us   = us;
        }
        return false;
    }
    soma_caiu[alvo]    = true;
    soma_caiu_ms[alvo] = cotuca::hal::NowMs();
    soma_caiu_us[alvo]  = us;
    soma_caiu_est[alvo] = por_estouro;
    params.Set(manifest.sacrif[alvo].param_on, 0.f);
    soma_quedas++;
    // AVISO MARCADO, nao falado: falar do callback mata a serial.
    aviso_teto_idx   = alvo;
    aviso_teto_us    = us;
    aviso_teto_prazo = prazo;
    soma_carencia    = kCarenciaBlocos;
    return true;
}

// ---- CRITERIO DE ESTOURO: mira o PRAZO, nao o teto ----
//
// TETO E PRAZO SAO COISAS DIFERENTES, e o vocabulario e permanente:
//
//   TETO  (850 us) = ORCAMENTO ADMINISTRATIVO. Onde o CTOS decide que
//                    a carga esta alta demais pro conforto. Escolhido.
//   PRAZO (1000 us) = PAREDE FISICA. Onde o bloco perde o deadline e
//                    o audio falha de verdade. Imposto pelo hardware.
//
// Confundir os dois faz a protecao criar o defeito que deveria evitar.
//
// O teto (850) e orcamento administrativo; o prazo (1000) e a parede
// fisica. Medido em 22/08: com tudo ligado o pico fica em ~975 us — o
// teto e ultrapassado em TODO pico e nada falha. Um criterio de pico
// que mirasse o teto derrubaria efeito o tempo todo numa configuracao
// que toca limpa: seria a protecao criando o defeito que deveria
// evitar, que e a razao pela qual baixar o teto foi descartado.
//
// Estouro de PRAZO e falha real, e mede ZERO nos dois regimes e sob
// provocacao (`sync` de 76 CCs, varredura de fader, chaveamento do
// wavefolder). Entao esta defesa nasce quieta — que e o certo: ela
// existe e nao age ate o fenomeno aparecer.
//
// TRES numa janela de 2 s, nao um: um estouro isolado ja aconteceu uma
// vez em toda a bancada e nao se reproduz. Punir o evento raro seria
// punir o instrumento por um acidente.
// ESCOLHIDOS, NAO MEDIDOS — e a etiqueta importa.
//
// 22/08: o contador de estouros mediu ZERO em todo regime de bancada
// (uso desenhado, extremo com tudo ligado, e sob provocacao dirigida
// ao `sync` de 76 CCs, varredura de fader e chaveamento do
// wavefolder). Sem estouro observado, nao ha dado do qual derivar
// estes numeros: eles vem de um CRITERIO declarado, nao de medicao.
//
// O criterio: nao punir o evento isolado. Houve um pico de 1035 us em
// toda a bancada, uma vez, sem culpado nomeado e sem se reproduzir.
// Derrubar efeito por causa dele seria punir o instrumento por um
// acidente. Tres numa janela de 2 s exige um padrao, nao um acidente.
//
// REVISAR se o contador em uso real discordar. Constante escolhida e
// constante medida sao coisas diferentes, e daqui a meses a pergunta
// "por que 3?" tem que achar a resposta aqui em vez de virar
// arqueologia.
static constexpr uint32_t kEstJanelaMs = 2000;
static constexpr uint32_t kEstLimite   = 3;
static uint32_t est_rajada    = 0;
static uint32_t est_rajada_ms = 0;
static uint32_t est_prazo_forc = 0;   // != 0 = prazo artificial, so teste

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
// ---- DUAS CADEIAS DE CARATER, UMA POR PIEZO ----
//
// Cada piezo capta um MATERIAL diferente do instrumento hibrido: o do
// tambor le transientes curtos e fortes, o da mola/corda le raspagem
// continua de baixo nivel. Diferenca de FONTE, nao de transdutor — vale
// com dois piezos perfeitamente casados. Ver a nota longa no manifest.
//
// A CAUDA NAO DUPLICA (delay, freeze, shimmer, reverb): e ela que faz
// tambor e corda soarem como o MESMO OBJETO. Duplicar separaria o que o
// desenho existe pra unir — e sao justamente os mais caros.
struct Cadeia
{
    cotuca::Gate       gate;
    cotuca::Drive      drive;
    cotuca::Phaser     phaser;
    cotuca::Warble     warble;
    cotuca::EffectSlot drive_slot, phaser_slot, warble_slot;
    bool               warble_ok = false;
    bool               gate_ativo = false;  // pra detectar a borda pulado->ativo
    float              buf[kMaxBlock];
};
static Cadeia cad[cotuca::Manifest::kCadeias];
alignas(32) static float warble_mem[cotuca::Manifest::kCadeias][1924];
static float             tone_buf[kMaxBlock];
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
alignas(32) static float reverb_mem[cotuca::Manifest::kCadeias][28160];

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

// ============================================================
// PAINEL v2 — 3 botões momentâneos
//   B1 = randomizador · B2 = assinalável (hold) · B3 = rearme do CTOS
//
// O HAL entrega NÍVEL já com debounce; a detecção de BORDA é aqui,
// porque é aqui que se sabe o que uma borda significa. B1 age na
// subida (um gesto = um sorteio); B2 acompanha o nível (segura,
// vale; solta, volta).
// ============================================================
// O ARRAY E DIMENSIONADO PELA MESMA CONSTANTE QUE LIMITA O LACO, e a
// historia deste par explica por que:
//
// Em 22/08 o array ficou em [2] enquanto o laco passou a `b < 3`:
// `btn_prev[2] = now` escrevia FORA, em cima de b2_sim/b3_sim.
// Consertado alargando os dois a mao — e em 23/08 o par divergiu de
// novo, no outro sentido: o array e o laco em 3, e o botao de indice 3
// (freeze B, D22) NUNCA ERA LIDO. O nivel cru chegava, o contador ficava
// em zero, e a suspeita caiu na solda do dB, que estava perfeita.
//
// Alargar a mao conserta o caso e preserva a forma. Agora o tamanho vem
// de `kMaxBotoes` e o laco para em `num_buttons`, que sao as duas fontes
// que ja existiam — nenhum numero digitado no meio.
static bool  btn_prev[cotuca::Manifest::kMaxBotoes] = {false};
static bool  b2_sim      = false; // simulação por serial
static bool  b3_sim      = false; // B3 (rearme) simulado por serial

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
static cotuca::Governor gov_l, gov_r; // só no ramo OUT_L (exciter)
static cotuca::Limiter  lim_l; // um por ramo: estados independentes
static cotuca::Limiter  lim_r;
// ANTI-FEEDBACK POR RAMO. Ate 20/08 so o ramo L tinha: OUT_R era jack
// de linha e nao realimentava. Com DOIS exciters os dois fecham laco, e
// a protecao duplica junto com o hardware.
static cotuca::Shifter  shift_l, shift_r;

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
    // O balanco dos piezos saiu daqui no pivo de 20/08: ele agora mistura
    // as duas cadeias JA PROCESSADAS, no bloco 2.
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

    // 1) DUAS CADEIAS DE CARATER — uma por piezo, em paralelo.
    //
    //   p1 -> in_gain_A -> gate_A -> [DRIVE PHASER WARBLE] -\
    //                                                        MIX -> cauda
    //   p2 -> in_gain_B -> gate_B -> [DRIVE PHASER WARBLE] -/
    //
    // O gate e o ganho duplicam pelo mesmo motivo dos efeitos: um limiar
    // que deixa a raspagem da mola passar abre demais pro tambor.

    // TOM DE TESTE gerado UMA VEZ. Chamar OscProcess() por cadeia
    // avancaria a fase duas vezes por amostra e dobraria a frequencia —
    // o oscilador tem estado, e estado nao se consome em duplicata.
    const int dest_tom = (int)(params.Real(manifest.p_tone_dest) + 0.5f);
    if(mix > 0.f)
        for(size_t i = 0; i < size; i++) tone_buf[i] = OscProcess();

    for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
    {
        Cadeia&      k = cad[c];
        float* const b = k.buf;

        // 1.1) ENTRADA + GANHO
        const float ig = params.Real(manifest.p_in_gain[c]);
        for(size_t i = 0; i < size; i++)
            b[i] = has_in ? in[c][i] * ig : 0.f;

        // 1.2) GATE — por cadeia
        // CUSTO ZERO COM O PORTAO ABERTO. `thresh` em 0 e o default e
        // significa portao escancarado: o gate processa cada amostra
        // pra devolve-la igual. MEDIDO: 15 us por bloco pros dois, sem
        // efeito audivel nenhum. Duplicar a cadeia duplicou tambem esse
        // desperdicio, e foi so ai que ele apareceu.
        //
        // CORRECAO 20/08: a versao anterior deste comentario afirmava
        // que "o gain fica em 1.0 enquanto isso". E FALSO — `Gate::Init`
        // poe `gain_` em ZERO, e pular o Process() CONGELA esse zero.
        // Eu afirmei um estado que nao conferi, e a consequencia era
        // audivel: subir o limiar com som tocando reabria o gate a
        // partir de ganho 0, com ~46 ms de fade-in.
        //
        // Agora a transicao pulado->ativo forca retomada ABERTA. Se o
        // sinal estiver mesmo abaixo do limiar, o release fecha suave.
        const float thr = params.Real(manifest.p_gate_thresh[c]);
        if(thr > 0.f)
        {
            if(!k.gate_ativo) { k.gate.Abre(); k.gate_ativo = true; }
            k.gate.SetThreshold(thr);
            k.gate.SetRelease(params.Real(manifest.p_gate_release[c]));
            for(size_t i = 0; i < size; i++) b[i] = k.gate.Process(b[i]);
        }
        else
            k.gate_ativo = false;

        // 1.3) TOM DE TESTE — DEPOIS DO GATE, e o ponto e documentado.
        //
        // Antes do gate, o proprio gate cortaria o tom e a medicao
        // estaria medindo o GATE em vez do alvo. Se algum teste futuro
        // precisar excitar o gate, declara um SEGUNDO ponto de injecao
        // explicitamente — medicao sem regime declarado nao vale.
        if(mix > 0.f && (dest_tom == 2 || dest_tom == c))
            for(size_t i = 0; i < size; i++)
                b[i] = b[i] * (1.f - mix) + tone_buf[i] * mix;

        // 1.4) DRIVE — sujeira antes de modulacao, tempo e espaco.
        // Depois do gate de proposito: saturar o chao de ruido e depois
        // tentar porta-lo seria empurrar contra a porta o que ela
        // deveria segurar.
        //
        // SEM CRONOMETRO POR DENTRO. O shimmer mediu 209 us com dois
        // NowTicks por amostra dentro da regiao medida, e boa parte era
        // a propria instrumentacao. A medida limpa e `cpu` on/off.
        k.drive_slot.SetOn(params.Real(manifest.p_drv_on[c]) >= 0.5f);
        if(k.drive_slot.Ativo())
        {
            k.drive.SetDrive(params.Real(manifest.p_drv_drive[CadeiaDoParam(c)]));
            k.drive.SetTone(params.Real(manifest.p_drv_tone[CadeiaDoParam(c)]));
            k.drive.SetSujeira(params.Real(manifest.p_drv_sujeira[CadeiaDoParam(c)]));
            const float mx = params.Real(manifest.p_drv_mix[c]);
            float gs = 0.f, gm = 0.f;
            for(size_t i = 0; i < size; i++)
            {
                k.drive_slot.Passo(gs, gm);
                // O fade escala o MIX, nao o sinal: o seco passa inteiro
                // o tempo todo, senao ligar abriria um buraco no meio.
                k.drive.SetMix(mx * gm);
                b[i] = k.drive.ProcessMono(b[i]);
            }
        }

        // 1.5) PHASER — sujeira, depois MOVIMENTO, depois tempo/espaco.
        k.phaser_slot.SetOn(params.Real(manifest.p_phs_on[c]) >= 0.5f);
        if(k.phaser_slot.Ativo())
        {
            k.phaser.SetRate(params.Real(manifest.p_phs_rate[CadeiaDoParam(c)]));
            k.phaser.SetDepth(params.Real(manifest.p_phs_depth[CadeiaDoParam(c)]));
            k.phaser.SetFeedback(params.Real(manifest.p_phs_feedback[CadeiaDoParam(c)]));
            k.phaser.SetStages((int)(params.Real(manifest.p_phs_stages[CadeiaDoParam(c)]) + 0.5f));
            // LFO e mapeamento de frequencia UMA VEZ POR BLOCO: as unicas
            // transcendentais ficam fora do laco de amostra.
            k.phaser.AtualizaLfo(size);
            const float mx = params.Real(manifest.p_phs_mix[c]);
            float gs = 0.f, gm = 0.f;
            for(size_t i = 0; i < size; i++)
            {
                k.phaser_slot.Passo(gs, gm);
                k.phaser.SetMix(mx * gm);
                b[i] = k.phaser.ProcessMono(b[i]);
            }
        }

        // 1.6) WARBLE — movimento de ALTURA, ainda antes do delay.
        //
        // GRAVA SEMPRE (regra dos efeitos com historico). Aqui morde de
        // verdade: o buffer tem 40 ms, entao parar de gravar em bypass
        // faria o efeito acordar lendo 40 ms de passado congelado.
        k.warble_slot.SetOn(params.Real(manifest.p_wrb_on[c]) >= 0.5f);
        if(k.warble_ok && k.warble_slot.Ativo())
        {
            k.warble.SetRate(params.Real(manifest.p_wrb_rate[CadeiaDoParam(c)]));
            k.warble.SetDepth(params.Real(manifest.p_wrb_depth[CadeiaDoParam(c)]));
            k.warble.SetIrreg(params.Real(manifest.p_wrb_irreg[CadeiaDoParam(c)]));
            k.warble.AtualizaLfo(size);
            // `mix` ESCALA O MOLHADO, e o fade do slot escala o mix —
            // mesma forma do drive e do phaser. Ate 20/08 o warble
            // cruzava direto pro derivado e ligar bastava pra ouvir;
            // agora mix em 0 significa mudo, e isso e mudanca no efeito
            // aprovado, feita a pedido do dB por consistencia.
            const float mx = params.Real(manifest.p_wrb_mix[c]);
            float gs = 0.f, gm = 0.f;
            for(size_t i = 0; i < size; i++)
            {
                k.warble_slot.Passo(gs, gm);
                const float w = k.warble.ProcessMono(b[i]);
                const float m = mx * gm;
                b[i] = b[i] * (1.f - m) + w * m;
            }
        }
        else if(k.warble_ok)
        {
            for(size_t i = 0; i < size; i++) k.warble.ProcessMono(b[i]);
        }
    }

    // 2) MIX ENTRE AS CADEIAS — o `piezo_mix` mudou de posicao.
    //
    // Ele era balanco na ENTRADA, entre dois sinais crus. Agora e entre
    // duas cadeias JA PROCESSADAS, e a lei continua sendo balanco e nao
    // crossfade: centro = as duas cheias, girar abaixa a dominante sem
    // cortar a outra.
    {
        const float bal     = params.Real(manifest.p_piezo_mix);
        const float alvo_ga = 1.f - (bal > 0.f ?  bal : 0.f);
        const float alvo_gb = 1.f - (bal < 0.f ? -bal : 0.f);
        // O CROSSFADE SECO/ESPACO SAIU DAQUI em 20/08. Na cauda
        // PARALELA o seco passa inteiro por fora e cada destino tem seu
        // proprio envio — nao ha mais o que dividir aqui. Este bloco so
        // aplica o balanco entre as cadeias.
        for(size_t i = 0; i < size; i++)
        {
            const float ga = suave_gl.Passo(alvo_ga);
            const float gb = suave_gr.Passo(alvo_gb);
            cad[0].buf[i] *= ga;
            cad[1].buf[i] *= gb;
            src_buf[i] = cad[0].buf[i] + cad[1].buf[i];
        }

    // BUZIOS — RESTAURADO EM 20/08.
    //
    // Ele foi APAGADO em 6a8175d, a reestruturacao em duas cadeias:
    // vivia pos-gate, dentro da faixa que eu substitui de uma vez, e
    // sumiu junto. Passou CINCO commits morto — sem erro de
    // compilacao (as variaveis seguiram declaradas e lidas no loop) e
    // com os dois CCs apontando pro nada.
    //
    // A posicao mudou de significado: era pos-gate de UM sinal, agora
    // le a SOMA das duas cadeias ja processadas. Uma batida em
    // qualquer piezo dispara um sorteio — que e o gesto, e nao seria
    // se cada cadeia tivesse o seu.
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


        // POS-GANHO — o que a cauda de fato ve. A reestruturacao das
        // duas cadeias apagou este trecho junto com o bloco antigo, e o
        // `hist` continuou IMPRIMINDO a coluna que ninguem mais
        // alimentava: teria reportado "pos-ganho 0" pra sempre, e zero
        // num canal que ninguem escreve nao e ausencia de sinal, e
        // ausencia de medicao. Instrumento que mente, pela enesima vez.
        //
        // O ponto mudou de significado com o pivo: era "depois do
        // in_gain", agora e "depois dos DOIS ganhos e da mistura".
        for(size_t i = 0; i < size; i++)
        {
            const float a = src_buf[i] < 0.f ? -src_buf[i] : src_buf[i];
            if(a > out_pk_g) out_pk_g = a;
            if(a >= 0.99f) { out_clip_g++; clip_latch_o = true; }
            hist_o[Balde(a)]++;
        }
    }

    // 3) MÓDULO DO INSTRUMENTO — escreve direto no out (2 out).
    // 3) CAUDA PARALELA — cada destino recebe SUA mistura das cadeias.
    //
    // Ate 20/08 a cauda era uma FILA: delay -> freeze -> [shimmer ↺
    // reverb]. Toda cadeia atravessava tudo na mesma ordem, e por isso
    // era impossivel dar delay a uma e sala a outra.
    //
    // Agora sao DESTINOS IRMAOS. Cada um tem envio por cadeia, e o seco
    // passa inteiro por fora. Um efeito continua sendo UM — o custo nao
    // duplicou — mas cada um recebe sua propria dose de A e de B.
    //
    // O QUE MUDOU NO SOM, e e real: o delay nao entra mais no reverb.
    // Antes cada eco ganhava sala; agora os ecos saem secos e a sala
    // trabalha sobre a fonte. Outra estetica, decidida pelo dB.
    //
    // O ESPACO DUPLICA DE VERDADE, e so ele. Shimmer nao e um efeito ao
    // lado do reverb: ele E o reverb realimentado com pitch. Corda com
    // shimmer e couro com sala limpa exige duas salas — nao ha como uma
    // sala so ser molhada de dois jeitos ao mesmo tempo.
    {
        // SECO: passa inteiro, sempre. Envio e TAP, nao desvio — quem
        // manda 127 pro delay nao some da saida direta.
        for(size_t i = 0; i < size; i++)
            out[0][i] = out[1][i] = cad[0].buf[i] + cad[1].buf[i];

        // ---- DELAY (uma instancia, envio por cadeia) ----
        {
            float ea = params.Real(manifest.p_dly_on[0]) >= 0.5f
                           ? params.Real(manifest.p_dly_mix[0]) : 0.f;
            float eb = params.Real(manifest.p_dly_on[1]) >= 0.5f
                           ? params.Real(manifest.p_dly_mix[1]) : 0.f;
            if(delay_ok)
            {
                delay_fx.SetTime(params.Real(manifest.p_dly_time));
                delay_fx.SetMix(1.f);   // retorno 100% molhado: quem dosa e o envio
                for(size_t i = 0; i < size; i++)
                {
                    const float x = cad[0].buf[i] * ea + cad[1].buf[i] * eb;
                    const float w = delay_fx.ProcessMono(x);
                    out[0][i] += w;
                    out[1][i] += w;
                }
            }
        }

        // ---- FREEZE: UM POR CADEIA ----
        //
        // NAO HA CHAVE SEPARADA: `frz_on` E o gesto de congelar. O
        // param se chama `on` pra ler igual aos vizinhos, mas o que ele
        // faz e segurar o que acabou de passar.
        //
        // SO O MOLHADO SAI DAQUI, e isto era um DEFEITO ate 20/08: o
        // `Freeze::Process` e CAMADA — soma o congelado e deixa o sinal
        // PASSAR. Na primeira versao paralela eu somava `l` inteiro na
        // saida, entao o envio entrava de novo e a cadeia engrossava
        // mesmo sem nada congelado. Subtrair a entrada isola o que o
        // freeze de fato acrescentou.
#if COTUCA_FREEZE_ENABLED
        if(!ctos_bypass_frz)
            for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
            {
                if(!freeze_c_ok[c]) continue;
                const uint32_t t0 = cotuca::hal::NowTicks();
                freeze_c[c].SetMix(params.Real(manifest.p_frz_vol[c]));
                freeze_c[c].SetSize(params.Real(manifest.p_frz_size[c]));
                freeze_c[c].SetFrozen(params.Real(manifest.p_frz_on[c]) >= 0.5f);
                for(size_t i = 0; i < size; i++)
                {
                    const float x = cad[c].buf[i];
                    float l = x, r = x;
                    freeze_c[c].Process(l, r);
                    out[0][i] += l - x;   // SO o congelado
                    out[1][i] += r - x;
                }
                const uint32_t us = (cotuca::hal::NowTicks() - t0)
                                    / cotuca::hal::TicksPerUs();
                if(us > ctos_pico_us) ctos_pico_us = us;
            }
#endif

        // ---- ESPACOS: um por cadeia, cada um com sala e shimmer ----
        for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
        {
            Espaco& e = esp[c];
            if(!e.reverb_ok) continue;
            // O ENVIO E A CHAVE. `rev_on` diz se a cadeia entra no
            // espaco; `rev_mix`, quanto. O shimmer nao tem envio proprio
            // porque ele NAO tem sala propria: ele e esta sala
            // realimentada com pitch, e entra pelo mesmo caminho.
            const bool  rev_quer = params.Real(manifest.p_rev_on[c]) >= 0.5f;
            const float alvo = rev_quer ? 1.f : 0.f;
            if(alvo < 0.5f && e.suave.Valor() <= 0.0001f) continue;
            const float env = rev_quer ? params.Real(manifest.p_rev_mix[c]) : 0.f;
            const bool  shm_quer = params.Real(manifest.p_shm_on[c]) >= 0.5f;
            e.reverb.SetSize(params.Real(manifest.p_rev_size[c]));
            e.reverb.SetDamp(params.Real(manifest.p_rev_damp[c]));
            // Com UMA sala por cadeia a molhadura ainda tem um valor so
            // dentro dela: quem ligou define. Reverb tem precedencia; o
            // shimmer usa a sala de passagem.
            e.reverb.SetMix(1.f);

            e.shimmer_slot.SetOn(shm_quer);
            const bool shm = e.pitch_ok && e.shimmer_slot.Ativo();
            if(shm)
            {
                e.shimmer_slot.Comeca(cotuca::hal::NowTicks());
                e.shimmer.SetMix(params.Real(manifest.p_shm_mix[c]));
                e.shimmer.SetFeedback(params.Real(manifest.p_shm_feedback[c]));
                e.shimmer.SetTone(params.Real(manifest.p_shm_tone[c]));
                const int modo = (int)(params.Real(manifest.p_shm_shift[c]) + 0.5f);
                e.pitch.SetRatio(0, 2.0f);
                e.pitch.SetRatio(1, 1.498307f);
                e.pitch.SetActive(0, modo == 0 || modo == 2);
                e.pitch.SetActive(1, modo == 1 || modo == 2);
            }

            const uint32_t t0 = cotuca::hal::NowTicks();
            for(size_t i = 0; i < size; i++)
            {
                const float g = e.suave.Passo(alvo);
                float l = cad[c].buf[i] * env;
                float r = l;
                float pl = 0.f, pr = 0.f, gm = 0.f;
                if(shm)
                {
                    float al = 0.f, ar = 0.f, bl = 0.f, br = 0.f;
                    e.pitch.Read(0, al, ar);
                    e.pitch.Read(1, bl, br);
                    // Duas vozes a ganho cheio dobrariam a energia do
                    // laco e o teto de 0.85 deixaria de ser teto.
                    const float k = (e.pitch.Active(0) && e.pitch.Active(1))
                                        ? 0.5f : 1.f;
                    pl = (al + bl) * k;
                    pr = (ar + br) * k;
                    // TOM ANTES DOS DOIS USOS: o ouvido soma a
                    // realimentacao E a camada direta.
                    e.shimmer.Tom(pl, pr);
                    float add_l, add_r;
                    e.shimmer.Realimenta(pl, pr, add_l, add_r);
                    float gs;
                    e.shimmer_slot.Passo(gs, gm);
                    l += add_l * gm;
                    r += add_r * gm;
                }
                e.reverb.Process(l, r);
                // GRAVA A CAUDA ANTES DA CAMADA DIRETA: se ela entrasse
                // no buffer, realimentaria por fora do laco e o
                // `feedback` deixaria de ser o unico dono do ganho.
                if(e.pitch_ok) e.pitch.Write(l, r);
                if(shm)
                {
                    const float m = e.shimmer.Mix() * gm;
                    l += pl * m;
                    r += pr * m;
                }
                // RETORNO INTEIRO. Quem dosa e o envio; escalar aqui
                // tambem seria o mesmo ganho duas vezes.
                out[0][i] += l * g;
                out[1][i] += r * g;
            }
            const uint32_t us = (cotuca::hal::NowTicks() - t0)
                                / cotuca::hal::TicksPerUs();
            e.pico_us = us;
            if(c == 0 && us > shimmer_pico) shimmer_pico = us;
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

    // 5b) [vago] — aqui morava o OUT_WIDTH (mid/side), aposentado em
    // 20/08 quando a arquitetura de DOIS EXCITERS o esvaziou de sentido.
    //
    // M/S pressupoe que L e R sao um PAR ESTEREO. Com dois exciters no
    // MESMO CORPO nao ha imagem estereo: a soma e ACUSTICA, no objeto,
    // antes do ar. E em `stems` os dois canais sao FONTES INDEPENDENTES
    // — aplicar M/S ali misturaria o piezo 1 dentro do piezo 2, que e
    // contaminacao silenciosa.
    //
    // Nao sobrou modo em que o controle significasse algo. Ver
    // docs/FEATURES.md para reviver, se um dia existir um modo em que a
    // saida seja um par estereo de verdade.

    // 6) SPLIT — cada ramo leva a SUA protecao.
    //
    // Ate 20/08 so o ramo L a tinha, e estava certo: OUT_R era jack de
    // linha e nao realimentava. Com DOIS exciters no corpo da escultura
    // os dois fecham laco acustico, e proteger so um deixaria o outro
    // microfonando livre.
    //
    // O QUE SE PERDEU NA TROCA, e e real: nao existe mais uma tomada
    // LIMPA. Cada saida alimenta amp E jack ao mesmo tempo, entao quem
    // grava recebe o sinal ja deslocado e comprimido. O `exc_on` abaixo
    // e o que devolve essa escolha.
    //
    // `shift_hz` E COMPARTILHADO pelos dois: dois exciters no MESMO
    // corpo com deslocamentos diferentes bateriam um contra o outro.
    // Mesmo shift desfaz o acordo de fase da microfonia sem batimento.
    //
    // Antes do governor de proposito: o governor deve reagir ao sinal
    // que de fato vai pro exciter, ja deslocado.
    {
        const float hz = params.Real(manifest.p_shift_hz);
        shift_l.SetShift(hz);
        shift_r.SetShift(hz);
        cotuca::Governor* gv[2] = {&gov_l, &gov_r};
        cotuca::Shifter*  sh[2] = {&shift_l, &shift_r};
        for(int b = 0; b < 2; b++)
        {
            // PORTAO POR RAMO. Com o exciter fora do circuito nao ha
            // laco pra quebrar, e a protecao so custa timbre — o jack
            // volta a receber o instrumento como ele e.
            //
            // Hoje e um param; quando os DPDT forem soldados, D29/D30
            // leem o polo 2 e dizem o estado REAL. Ate la, quem declara
            // e o dB, e errar pra ligado e o lado seguro: proteger sem
            // precisar custa timbre, nao proteger quando precisa custa
            // o instrumento.
            if(params.Real(manifest.p_exc_on[b]) < 0.5f) continue;
            gv[b]->SetCeiling(params.Real(manifest.p_gov_ceiling[b]));
            gv[b]->SetRelease(params.Real(manifest.p_gov_release[b]));
            float* o = out[b];
            for(size_t i = 0; i < size; i++)
                o[i] = gv[b]->Process(sh[b]->Process(o[i]));
        }
    }

    // 6b) BENCH BOOST — FERRAMENTA DE BANCADA, por ramo, imediatamente
    //     antes do limiter. Ver o bloco de aviso em manifest.h: nunca
    //     vai a knob nem a cena, e nasce em 1.0.
    for(int b = 0; b < 2; b++)
    {
        const float boost = params.Real(manifest.p_bench_boost[b]);
        if(boost != 1.0f)
            for(size_t i = 0; i < size; i++) out[b][i] *= boost;
    }

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

            // ESTOURO DE PRAZO. So conta e carimba — nunca loga daqui:
            // a regra do callback que ja pendurou a placa uma vez.
            const uint32_t prazo_est = est_prazo_forc ? est_prazo_forc : prazo;
            if(soma_us > prazo_est)
            {
                const uint32_t ag = cotuca::hal::NowMs();
                if(!est_n) est_prim_ms = ag;
                est_n++;
                est_ult_ms = ag;
                est_ult_us = soma_us;
                if(soma_us > est_pior_us) est_pior_us = soma_us;
                // Retrato do que estava no ar. Um bit por familia por
                // cadeia; sem isto o numero volta a nao ter denominador.
                uint16_t m = 0;
                for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
                {
                    if(params.Real(manifest.p_drv_on[c]) >= 0.5f) m |= 1u << (c * 7 + 0);
                    if(params.Real(manifest.p_phs_on[c]) >= 0.5f) m |= 1u << (c * 7 + 1);
                    if(params.Real(manifest.p_wrb_on[c]) >= 0.5f) m |= 1u << (c * 7 + 2);
                    if(params.Real(manifest.p_dly_on[c]) >= 0.5f) m |= 1u << (c * 7 + 3);
                    if(params.Real(manifest.p_frz_on[c]) >= 0.5f) m |= 1u << (c * 7 + 4);
                    if(params.Real(manifest.p_rev_on[c]) >= 0.5f) m |= 1u << (c * 7 + 5);
                    if(params.Real(manifest.p_shm_on[c]) >= 0.5f) m |= 1u << (c * 7 + 6);
                }
                est_perfil = m;
                // RAJADA: tres numa janela de 2 s derrubam um efeito.
                // Fora da janela, a contagem recomeca — estouro
                // esparso nao acumula ate virar queda.
                const uint32_t ag2 = cotuca::hal::NowMs();
                if(est_rajada && (ag2 - est_rajada_ms) > kEstJanelaMs)
                    est_rajada = 0;
                if(!est_rajada) est_rajada_ms = ag2;
                if(++est_rajada >= kEstLimite && !soma_carencia)
                {
                    est_rajada = 0;
                    SacrificaUm(soma_us, prazo_est, true);
                }
            }
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
                    SacrificaUm(soma_us, prazo, false);
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
// B2 = BOTAO DE FREEZE FISICO (D27), e agora ALTERNA em vez de segurar.
//
// Era momentaneo: apertar forcava 1, soltar devolvia o valor guardado.
// Virou toggle por pedido do dB — bateu congela, bateu solta. Com botao
// de arcade a alternancia e o gesto certo: segurar um arcade durante
// uma cauda de freeze e desconfortavel, e o `frz_on` deste instrumento
// JA E o gesto de congelar, nao uma chave de bypass.
//
// O TOGGLE MORA NO PARAM, que e a fonte da verdade. O botao so escreve
// ali; espelho, janela de autoria, LED e CTOS leem de la. Nenhuma rota
// nova — e por isso a mudanca cabe em poucas linhas.
//
// ORIGEM LOCAL, e isso importa pro espelho: como a mudanca nao veio de
// MIDI, `eco_ult` nao a conhece e o espelho vai transmitir o `frz_on`
// novo pro Loopy sozinho — abrindo a janela de 250 ms naquele CC, que
// e o que impede o widget de devolver e desfazer. Caminho existente,
// verificado, nao reimplementado.
// CONTADOR DE BATIDAS, e ele existe por um motivo de metodo: contar
// alternancias por LOG exige que o operador bata dentro de uma janela
// que so aparece na tela DEPOIS de a janela comecar — o operador nao
// tem como saber quando. Contador nao tem esse problema: bate quando
// quiser, le quando quiser.
//
// E a paridade sozinha nao prova debounce: 5 e 7 batidas dao o mesmo
// estado final. O contador distingue.
// UM SO LUGAR PRA TODOS OS BOTOES COM ALVO. Era `AlternaFreeze` com
// um alvo unico; o segundo botao fisico obrigaria a repetir o gesto
// inteiro, e repetir gesto e como as duas versoes comecam a divergir
// no primeiro conserto — a mesma razao que fez o sacrificio do CTOS
// virar funcao unica.
static uint32_t botao_batidas[cotuca::Manifest::kMaxBotoes] = {0};

// TRAVA DE NIVEL CRU, e ela existe porque a janela de 400 ms do `pino`
// repetia a cicatriz 13e: pedir "aperta agora" de dentro de uma medicao
// cronometrada, quando a mensagem so chega depois de a janela abrir.
//
// A trava inverte: a placa acumula desde o boot e o operador aperta
// quando quiser. `ja_baixo` responde "este pino ja fechou alguma vez?" —
// que e A pergunta, e ela nao tem relogio dentro.
//
// Amostrado no LOOP e CRU: sem debounce, sem borda. Um contato de 2 ms
// que o debounce descartaria aparece aqui, e essa diferenca e o proprio
// diagnostico quando alguem suspeita de mau contato.
static bool     cru_ja_baixo[8] = {false};
static uint32_t cru_n_baixo[8]  = {0};
static uint32_t cru_amostras    = 0;

static void AlternaBotao(int idx)
{
    using namespace cotuca;
    if(idx < 0 || idx >= Manifest::kMaxBotoes) return;
    const Manifest::BotaoAlvo& b = manifest.botao[idx];
    if(b.param < 0) return;   // botao sem alvo declarado
    const bool ligado = params.Real(b.param) >= 0.5f;
    params.Set(b.param, ligado ? 0.f : 1.f);
    botao_batidas[idx]++;
    hal::Log("B%d (%s): %s -> %s (botao fisico, batida %lu)",
             idx + 1, b.nome ? b.nome : "?", params.Name(b.param),
             ligado ? "SOLTA" : "CONGELA",
             (unsigned long)botao_batidas[idx]);
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

static volatile uint32_t lb_trans = 0;
static volatile uint8_t  lb_duty  = 0;
static bool              lb_cru      = false;
static bool              lb_espelho  = false;

static void LbTick()
{
    using namespace cotuca;
    static uint32_t prox_ms = 0, jan0 = 0;
    static bool     ant      = false;
    static bool     primeira = true;
    static uint32_t n_trans = 0, n_aceso = 0;
    static uint32_t n_amostras = 0;

    const uint32_t ag = hal::NowMs();
    if(ag < prox_ms)
        return;
    prox_ms = ag + 5u; // 200 Hz

    // SO O LB. LG/LR saíram do hardware em 20/08 por nao marcarem nada.
    const bool nivel = hal::ReadWidiLed();
    const bool aceso = kLbAcesoEmBaixo ? !nivel : nivel;
    if(primeira) ant = aceso;
    if(aceso != ant) { n_trans++; ant = aceso; }
    if(aceso) n_aceso++;
    if(primeira) { jan0 = ag; primeira = false; }
    n_amostras++;
    if(lb_espelho)
        hal::SetLed(aceso);

    if(ag - jan0 < 2000u)
        return;

    lb_trans = n_trans;
    lb_duty  = (uint8_t)(n_amostras ? (n_aceso * 100u / n_amostras) : 0u);
    if(lb_cru)
    {
        const bool traf
            = ble_last_rx_ms && (ag - ble_last_rx_ms) < 2000u;
        hal::Log("lb: LB %lu/%u%%  | trafego %s",
                 (unsigned long)lb_trans, (unsigned)lb_duty,
                 traf ? "SIM" : "nao");
    }
    jan0 = ag; n_amostras = 0;
    n_trans = 0; n_aceso = 0;
}

static void declara_modos()
{
    using namespace cotuca;
    if(hal::LogMuted())
        hal::LogAlways("[LOG MUDO] o log esta silenciado — `log on` destrava."
                       " O que voce nao esta vendo NAO e ausencia de dado.");
    // `dup off` faz a cadeia B soar com os valores da A: e modo que
    // altera o instrumento, entao se declara junto com o log mudo. Sem
    // isto, "a cadeia B nao responde ao set" tem duas explicacoes e o
    // status nao distingue nenhuma.
    if(!g_dup_por_cadeia)
        hal::Log("[DUP OFF] os dez params de caracter estao COMPARTILHADOS"
                 " — a cadeia B soa com os valores da A. `dup on` devolve.");
    // O aviso do interino do voo A saiu junto com o interino: o espelho
    // nao ocupa mais canal de entrada, entao nao ha entrada recusada
    // para declarar. O que sobrou de modo declaravel e o espelho MUDO,
    // porque placa que nao espelha e indistinguivel de placa cujo fio
    // de saida caiu.
    if(boot_dreno_ate && (int32_t)(hal::NowMs() - boot_dreno_ate) < 0)
        hal::LogAlways("[DRENANDO O BOOT] a entrada esta recusada por mais"
                       " ~%ld ms — o fio ainda pode carregar mensagem"
                       " enderecada a versao anterior.",
                       (long)(boot_dreno_ate - hal::NowMs()));
    if(!espelho_on)
        hal::Log("[ESPELHO MUDO] a placa nao esta transmitindo estado —"
                 " os widgets nao vao acompanhar. `espelho on` devolve.");
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
    // `qspi id` — le o JEDEC ID pela rotina ITCM.
    //
    // E O "OLA MUNDO" DA JANELA DE ESCRITA, por desenho: valida ITCM,
    // mudanca de modo e retorno ao memory-mapped com o unico comando
    // que nao pode estragar nada. Se ler tres bytes funciona, o mesmo
    // mecanismo escreve — o que muda e a tabela de comandos.
    // `qspi vida` — a rotina ITCM roda e volta? Sem tocar no chip.
    if(strncmp(line, "qspi vida", 9) == 0)
    {
        const uint32_t r = cotuca::qspi::ProvaDeVida();
        hal::Log("qspi: prova de vida da ITCM devolveu %lu (esperado 5050)"
                 " — a rotina executa de la e retorna", (unsigned long)r);
        return;
    }
    if(strncmp(line, "qspi id", 7) == 0)
    {
        // `qspi id [n]` — n = ciclos dummy, pra varrer sem regravar.
        {
            const char* a = line + 7;
            while(*a == ' ') a++;
            // `qspi id [dummy] [modo] [opcode-hex]`
            char* fim = nullptr;
            if(*a >= '0' && *a <= '9')
            {
                cotuca::qspi::SetDummyQpi((uint32_t)strtol(a, &fim, 10));
                while(fim && *fim == ' ') fim++;
                if(fim && *fim >= '1' && *fim <= '4')
                {
                    cotuca::qspi::SetModo((uint32_t)strtol(fim, &fim, 10));
                    while(fim && *fim == ' ') fim++;
                    if(fim && *fim)
                        cotuca::qspi::SetOpcode((uint32_t)strtol(fim, nullptr, 16));
                }
            }
        }
        // A JANELA. A tabela de vetores e os 51 handlers moram na
        // QSPI; qualquer interrupcao aqui dentro busca o vetor da
        // memoria que saiu do ar e e fatal ANTES de chegar ao handler.
        // Nao ha lista de excecoes possivel — so mascarar tudo.
        __disable_irq();
        const uint32_t id = cotuca::qspi::LeJedecId();
        __enable_irq();

        hal::Log("qspi: SR=%08lX  esperas que estouraram=%lu",
                 (unsigned long)cotuca::qspi::UltimoSR(),
                 (unsigned long)cotuca::qspi::UltimasFalhas());
        // IMPRIME SEMPRE O VALOR CRU, e o motivo custou uma rodada:
        // o filtro anterior tratava `id == 0` como "nao respondeu" — e
        // 0x00 e EXATAMENTE o que um chip ocioso devolve ao RDSR
        // (0x05). Resposta perfeita reportada como ausencia, que e a
        // familia do `else 0` (bts 12u) na quinta aparicao do dia.
        //
        // Agora o cru vem sempre, e a INTERPRETACAO vem etiquetada:
        //   FF FF FF  = linha em repouso, o chip nao dirigiu nada
        //   00 00 00  = o chip dirigiu tudo baixo (RDSR ocioso = isto)
        //   misto     = dado de verdade
        hal::Log("qspi: cru = %02lX %02lX %02lX  (%s)",
                 (unsigned long)((id >> 16) & 0xFF),
                 (unsigned long)((id >> 8) & 0xFF),
                 (unsigned long)(id & 0xFF),
                 id == 0xFFFFFFu ? "linha em repouso — chip mudo"
                 : id == 0u      ? "tudo baixo — o chip DIRIGIU"
                                 : "dado misto");
        if(id == 0xFFFFFFu)
        {
            hal::Log("qspi: o periferico NAO respondeu (id=0). A rotina"
                     " voltou viva, entao a janela funciona — o que"
                     " falhou foi o dialogo com o chip.");
            return;
        }
        hal::Log("qspi: JEDEC ID = %02lX %02lX %02lX  (fabricante,"
                 " tipo, densidade)",
                 (unsigned long)((id >> 16) & 0xFF),
                 (unsigned long)((id >> 8) & 0xFF),
                 (unsigned long)(id & 0xFF));
        // Densidade: o terceiro byte e log2 do tamanho em bytes nos
        // JEDEC comuns. 0x17 = 8 MB. Declarado como INFERENCIA, nao
        // como leitura — se o chip nao seguir a convencao, o numero
        // mente e a etiqueta avisa.
        const uint32_t d = id & 0xFF;
        if(d >= 0x10 && d <= 0x20)
            hal::Log("  densidade inferida: %lu MB (2^%lu bytes)",
                     (unsigned long)(1ul << (d - 20)), (unsigned long)d);
        return;
    }
    // vistos [reset] — inventario de quais CCs ja chegaram
    if(strncmp(line, "vistos", 6) == 0)
    {
        const char* a = line + 6;
        while(*a == ' ') a++;
        if(strncmp(a, "reset", 5) == 0)
        {
            for(int q = 0; q < kCanaisInv; q++)
                for(int i = 0; i < 128; i++) cc_visto[q][i] = 0;
            hal::Log("vistos: zerado. Varra os widgets e rode `vistos`.");
            return;
        }
        int n = 0, faltam = 0;
        hal::Log("INVENTARIO — (canal,CC) que ja chegaram pelo fio:");
        // O CANAL ENTRA NO RELATORIO porque parou de ser redundante: o
        // mesmo numero de CC endereça params diferentes conforme o
        // canal, entao "cc30 chegou" nao responde mais nada sozinho.
        for(int k = 0; k < manifest.n_midi; k++)
        {
            const uint8_t cc = manifest.midi_map[k].cc;
            const uint8_t ca = manifest.midi_map[k].canal;
            const int     id = manifest.midi_map[k].param_id;
            const int vis = (ca < kCanaisInv) ? cc_visto[ca][cc] : 0;
            if(vis) n++;
            else
            {
                faltam++;
                hal::Log("  FALTA ch%d cc%-3d %s", (int)ca + 1, (int)cc,
                         id >= 0 ? params.Name(id) : "?");
            }
        }
        hal::Log("  %d de %d vinculos endereçados | %d faltam",
                 n, manifest.n_midi, faltam);
        // (canal,CC) que chegou e NAO esta no mapa: widget apontando pra
        // endereço que o firmware nao conhece. E aqui que o remapeamento
        // entra — e agora ele diz em QUE canal, que e metade da pista.
        for(int q = 0; q < kCanaisInv; q++)
            for(int c = 0; c < 128; c++)
                if(cc_visto[q][c]
                   && manifest.FindMidiParam((uint8_t)q, (uint8_t)c) < 0)
                    hal::Log("  ORFAO ch%d cc%d chegou %d vezes, fora do mapa",
                             q + 1, c, (int)cc_visto[q][c]);
        return;
    }
    // b3 — pulsa o botao de rearme (o painel ainda nao existe)
    if(strncmp(line, "b3", 2) == 0)
    {
        b3_sim = true;
        hal::Log("B3 simulado: pulso de rearme");
        return;   // o laco do painel ve a borda e devolve; b3_sim cai la
    }
    // b2 — PULSA o botao de freeze, como o `b3` pulsa o rearme.
    //
    // Era `b2 [0|1]` (segurar/soltar) e virou pulso junto com o botao:
    // simular "segurar" um botao que agora ALTERNA nao significa nada.
    // Gancho que nao acompanha a mudanca do que ele simula vira
    // instrumento que mente sobre o instrumento.
    if(strncmp(line, "b2", 2) == 0)
    {
        b2_sim = true;   // o laco consome a borda e baixa
        if(manifest.botao[1].param < 0)
            hal::Log("B2: pulso (mas o botao 2 nao tem alvo, sem efeito)");
        else
            hal::Log("B2: pulso — alterna %s",
                     params.Name(manifest.botao[1].param));
        return;
    }
    if(strncmp(line, "tone on", 7) == 0)
    { params.Set(manifest.p_tone_mix, 0.5f); hal::Log("tone ON"); return; }
    if(strncmp(line, "tone off", 8) == 0)
    { params.Set(manifest.p_tone_mix, 0.0f); hal::Log("tone OFF"); return; }
    // GANHO DE TESTE DA DRENAGEM. Sem ele a defesa e indemonstravel: a
    // janela de 300 ms fecha antes de a serial USB enumerar, entao
    // ninguem consegue observa-la agindo, e "contador em 0" fica
    // ambiguo entre "funcionou e nada chegou" e "nunca rodou".
    //
    // Mesma familia do `ctos prazo`: a defesa so vira demonstravel
    // quando o parametro que a governa e alcancavel de fora.
    // `pino` — nivel CRU dos botoes, amostrado por um instante.
    //
    // Responde "o botao fecha para GND?" sem debounce, sem borda e sem
    // contador no meio. Um contador em zero tem tres causas (aberto,
    // terminal NC, pino morto) e nao distingue nenhuma; o nivel cru
    // distingue as tres, e por isso ele existe.
    // ---- PRESETS ----
    //
    // INGESTAO POR COLAGEM: uma linha de `dump` e um COMANDO VALIDO.
    //
    // Sem isto o `dump` seria so de saida e a biblioteca morreria no
    // desligamento sem volta — dump que ninguem consegue recarregar nao
    // e persistencia, e um relatorio.
    //
    // O formato ja e por NOME e as linhas de mesmo nome se juntam,
    // entao colar o arquivo inteiro no terminal reconstroi a
    // biblioteca. Linha `#` e comentario e some sem reclamar.
    if(line[0] == '#') return;
    if(strncmp(line, "cadeia ", 7) == 0 || strncmp(line, "global ", 7) == 0)
    {
        const bool eh_global = (line[0] == 'g');
        const char* a = line + 7;
        while(*a == ' ') a++;
        char nome[cotuca::kNomeMax] = {0};
        int  k = 0;
        while(*a && *a != ' ' && k < cotuca::kNomeMax - 1) nome[k++] = *a++;
        nome[k] = 0;
        if(!k) { hal::Log("preset: linha sem nome"); return; }

        // Acha ou cria. Linhas de mesmo nome ACUMULAM no mesmo preset —
        // e por isso que o dump pode quebrar em quantas linhas quiser.
        int idx;
        if(eh_global)
        {
            idx = biblio.AchaGlobal(nome);
            if(idx < 0)
            {
                idx = biblio.VagaGlobal();
                if(idx < 0) { hal::Log("preset: biblioteca global cheia"); return; }
                cotuca::Biblioteca::PoeNome(biblio.global[idx].nome, nome);
                biblio.global[idx].usado = true;
                biblio.global[idx].n = (uint8_t)pmapa.n_global;
            }
        }
        else
        {
            idx = biblio.AchaCadeia(nome);
            if(idx < 0)
            {
                idx = biblio.VagaCadeia();
                if(idx < 0) { hal::Log("preset: biblioteca de cadeia cheia"); return; }
                cotuca::Biblioteca::PoeNome(biblio.cadeia[idx].nome, nome);
                biblio.cadeia[idx].usado = true;
                biblio.cadeia[idx].n = (uint8_t)pmapa.n_cadeia;
            }
        }

        int aceitos = 0, recusados = 0;
        char ruins[90]; ruins[0] = 0;
        while(*a)
        {
            while(*a == ' ') a++;
            if(!*a) break;
            char pn[40]; int p = 0;
            while(*a && *a != '=' && *a != ' ' && p < 38) pn[p++] = *a++;
            pn[p] = 0;
            if(*a != '=')
            {
                if(p) recusados++;
                continue;
            }
            a++;
            int v = 0; bool tem = false;
            while(*a >= '0' && *a <= '9') { v = v * 10 + (*a++ - '0'); tem = true; }
            if(!tem || v > 127)
            {
                recusados++;
                if(strlen(ruins) < 70)
                { strncat(ruins, " ", 2); strncat(ruins, pn, 30); }
                continue;
            }
            // CASA PELO NOME, contra a lista DERIVADA. Nome que nao
            // existe no mapa e RECUSADO E NOMEADO — aceitar em silencio
            // seria carregar um preset pela metade e chamar de sucesso.
            int achou = -1;
            for(int j = 0; j < (eh_global ? pmapa.n_global : pmapa.n_cadeia); j++)
            {
                const char* nm = params.Name(eh_global ? pmapa.global[j]
                                                       : pmapa.cadeia[0][j]);
                if(eh_global) { if(!strcmp(nm, pn)) { achou = j; break; } }
                else
                {
                    const size_t L = strlen(nm);
                    if(L >= 2 && !strncmp(nm, pn, L - 2) && strlen(pn) == L - 2)
                    { achou = j; break; }
                }
            }
            if(achou < 0)
            {
                recusados++;
                if(strlen(ruins) < 70)
                { strncat(ruins, " ", 2); strncat(ruins, pn, 30); }
                continue;
            }
            if(eh_global) biblio.global[idx].val[achou] = (uint8_t)v;
            else          biblio.cadeia[idx].val[achou] = (uint8_t)v;
            aceitos++;
        }
        hal::Log("preset \"%s\": +%d params%s%s", nome, aceitos,
                 recusados ? "  RECUSADOS:" : "",
                 recusados ? ruins : "");
        return;
    }
    if(strncmp(line, "capturar", 8) == 0)
    {
        const char* a = line + 8;
        while(*a == ' ') a++;
        // ALVO EXPLICITO. `capturar a`, `capturar b` ou `capturar g`.
        // Sem default: capturar na cadeia errada e silencioso e so
        // aparece quando alguem carrega o preset e o som e outro.
        char alvo = *a; if(alvo) a++;
        while(*a == ' ') a++;
        if(alvo != 'a' && alvo != 'b' && alvo != 'g')
        {
            hal::Log("uso: capturar <a|b|g> <nome> — o alvo e obrigatorio,"
                     " capturar na cadeia errada nao tem sintoma");
            return;
        }
        if(!*a) { hal::Log("capturar: falta o nome"); return; }
        if(alvo == 'g')
        {
            int i = biblio.AchaGlobal(a);
            if(i < 0) i = biblio.VagaGlobal();
            if(i < 0) { hal::Log("capturar: biblioteca global cheia (%d)",
                                 (int)cotuca::kMaxPresetsGlobal); return; }
            cotuca::PresetGlobal& p = biblio.global[i];
            cotuca::Biblioteca::PoeNome(p.nome, a);
            p.n = (uint8_t)pmapa.n_global;
            for(int k = 0; k < pmapa.n_global; k++)
                p.val[k] = (uint8_t)(params.Get(pmapa.global[k]) * 127.f + 0.5f);
            p.usado = true;
            hal::Log("capturar: global \"%s\" no slot %d (%d params, em RAM)",
                     p.nome, i, (int)p.n);
            return;
        }
        const int c = (alvo == 'a') ? 0 : 1;
        int i = biblio.AchaCadeia(a);
        if(i < 0) i = biblio.VagaCadeia();
        if(i < 0) { hal::Log("capturar: biblioteca de cadeia cheia (%d)",
                             (int)cotuca::kMaxPresetsCadeia); return; }
        cotuca::PresetCadeia& p = biblio.cadeia[i];
        cotuca::Biblioteca::PoeNome(p.nome, a);
        p.n = (uint8_t)pmapa.n_cadeia;
        for(int k = 0; k < pmapa.n_cadeia; k++)
            p.val[k] = (uint8_t)(params.Get(pmapa.cadeia[c][k]) * 127.f + 0.5f);
        p.usado = true;
        biblio.ponteiro[c] = i;
        hal::Log("capturar: \"%s\" no slot %d, da cadeia %c (%d params, em RAM)",
                 p.nome, i, alvo == 'a' ? 'A' : 'B', (int)p.n);
        hal::Log("        RAM: desliga da tomada e some. `preset dump` guarda"
                 " no computador enquanto a flash nao destrava.");
        // A CAPTURA E COM PERDA, E ISSO SE DIZ NA HORA DA CAPTURA.
        //
        // O preset guarda 7 bits (0..127), a mesma resolucao do MIDI —
        // decisao do desenho, e o que mantem o `dump` legivel e
        // editavel a mao. Mas o `set` aceita float, entao um valor
        // ajustado de ouvido pode voltar levemente diferente: 24,00
        // capturado volta 23,83.
        //
        // Dito AQUI e nao no `load` porque e aqui que a perda acontece,
        // e porque descobrir isso depois — comparando um som lembrado
        // com um som carregado — e a forma mais cara possivel de
        // aprender que a operacao nao era exata.
        hal::Log("        quantizado em 7 bits (0..127, igual ao MIDI):"
                 " ajuste de `set` mais fino que 1/127 nao sobrevive.");
        return;
    }
    if(strncmp(line, "preset", 6) == 0)
    {
        const char* a = line + 6;
        while(*a == ' ') a++;

        if(strncmp(a, "list", 4) == 0)
        {
            hal::Log("BIBLIOTECA (RAM) — mapa 0x%04X | %d params por cadeia,"
                     " %d no global",
                     (unsigned)mapa_hash, pmapa.n_cadeia, pmapa.n_global);
            int n = 0;
            for(int i = 0; i < cotuca::kMaxPresetsCadeia; i++)
                if(biblio.cadeia[i].usado)
                {
                    hal::Log("  cadeia %2d  %-16s %d params%s%s", i,
                             biblio.cadeia[i].nome, (int)biblio.cadeia[i].n,
                             biblio.ponteiro[0] == i ? "   <- A" : "",
                             biblio.ponteiro[1] == i ? "   <- B" : "");
                    n++;
                }
            for(int i = 0; i < cotuca::kMaxPresetsGlobal; i++)
                if(biblio.global[i].usado)
                {
                    hal::Log("  global %2d  %-16s %d params%s", i,
                             biblio.global[i].nome, (int)biblio.global[i].n,
                             biblio.ponteiro_global == i ? "   <- G" : "");
                    n++;
                }
            if(!n) hal::Log("  (vazia — `capturar a <nome>` cria o primeiro)");
            return;
        }

        if(strncmp(a, "dump", 4) == 0)
        {
            // TEXTO LEGIVEL E POR NOME, nunca posicional. Um formato
            // posicional seria mais curto e transformaria qualquer
            // reordenacao do manifest em corrupcao SILENCIOSA — o mesmo
            // defeito do cabecalho, movido para dentro da linha.
            // LINHAS CURTAS, NOME REPETIDO, E O LEITOR JUNTA.
            //
            // A primeira versao punha os 33 params numa linha so e o
            // `hal::Log` cortava em 160 chars: saiam 13 de 33, e a linha
            // parecia completa. Alargar o buffer seria consertar o caso
            // e preservar a forma — o proximo param a mais quebraria de
            // novo, em silencio.
            //
            // Com o nome repetido a cada linha, o formato deixa de
            // depender de comprimento nenhum: quem le junta as linhas de
            // mesmo nome. E de quebra fica mais facil de editar a mao,
            // que e o ponto todo do dump ser texto.
            hal::Log("# cotucatronics presets — mapa 0x%04X", (unsigned)mapa_hash);
            hal::Log("# linhas com o MESMO nome se juntam — nao ha limite"
                     " de params por preset");
            hal::Log("# reordenar = mover bloco · deletar = apagar linhas ·"
                     " versionar = git");
            for(int i = 0; i < cotuca::kMaxPresetsCadeia; i++)
            {
                const cotuca::PresetCadeia& p = biblio.cadeia[i];
                if(!p.usado) continue;
                char linha[120];
                int  o = 0, nesta = 0;
                for(int k = 0; k < p.n && k < pmapa.n_cadeia; k++)
                {
                    if(nesta == 0)
                        o = snprintf(linha, sizeof(linha), "cadeia %s", p.nome);
                    const char* nm = params.Name(pmapa.cadeia[0][k]);
                    char base[32]; int b = 0;
                    for(; nm[b] && b < 30; b++) base[b] = nm[b];
                    base[b] = 0;
                    if(b >= 2 && base[b-2] == '_') base[b-2] = 0;  // tira o _a
                    o += snprintf(linha + o, sizeof(linha) - o, " %s=%d",
                                  base, (int)p.val[k]);
                    nesta++;
                    if(nesta == 5) { hal::Log("%s", linha); nesta = 0; }
                }
                if(nesta) hal::Log("%s", linha);
            }
            for(int i = 0; i < cotuca::kMaxPresetsGlobal; i++)
            {
                const cotuca::PresetGlobal& p = biblio.global[i];
                if(!p.usado) continue;
                char linha[120];
                int  o = 0, nesta = 0;
                for(int k = 0; k < p.n && k < pmapa.n_global; k++)
                {
                    if(nesta == 0)
                        o = snprintf(linha, sizeof(linha), "global %s", p.nome);
                    o += snprintf(linha + o, sizeof(linha) - o, " %s=%d",
                                  params.Name(pmapa.global[k]), (int)p.val[k]);
                    nesta++;
                    if(nesta == 5) { hal::Log("%s", linha); nesta = 0; }
                }
                if(nesta) hal::Log("%s", linha);
            }
            hal::Log("# fim");
            return;
        }

        if(strncmp(a, "load", 4) == 0)
        {
            const char* b = a + 4;
            while(*b == ' ') b++;
            char alvo = *b; if(alvo) b++;
            while(*b == ' ') b++;
            if((alvo != 'a' && alvo != 'b' && alvo != 'g') || !*b)
            { hal::Log("uso: preset load <a|b|g> <nome>"); return; }

            if(alvo == 'g')
            {
                const int i = biblio.AchaGlobal(b);
                if(i < 0) { hal::Log("preset load: nao achei global \"%s\"", b); return; }
                const cotuca::PresetGlobal& p = biblio.global[i];
                for(int k = 0; k < p.n && k < pmapa.n_global; k++)
                {
                    params.Set(pmapa.global[k], (float)p.val[k] / 127.f);
                    AbreJanelaEEspelha(pmapa.global[k]);
                }
                biblio.ponteiro_global = i;
                hal::Log("preset load: global \"%s\" — %d params, janela de"
                         " autoria aberta, espelho escoa sozinho",
                         p.nome, (int)p.n);
                return;
            }
            const int c = (alvo == 'a') ? 0 : 1;
            const int i = biblio.AchaCadeia(b);
            if(i < 0) { hal::Log("preset load: nao achei \"%s\"", b); return; }
            const cotuca::PresetCadeia& p = biblio.cadeia[i];
            for(int k = 0; k < p.n && k < pmapa.n_cadeia; k++)
            {
                params.Set(pmapa.cadeia[c][k], (float)p.val[k] / 127.f);
                AbreJanelaEEspelha(pmapa.cadeia[c][k]);
            }
            biblio.ponteiro[c] = i;
            hal::Log("preset load: \"%s\" na cadeia %c — %d params, janela"
                     " aberta, espelho escoa sozinho",
                     p.nome, alvo == 'a' ? 'A' : 'B', (int)p.n);
            return;
        }

        hal::Log("preset: list | dump | load <a|b|g> <nome>   ·"
                 " capturar <a|b|g> <nome>");
        return;
    }
    // ATENCAO A ORDEM: `oledvel` TEM que ser testado antes de `oled`,
    // senao o strncmp de 4 letras engole o prefixo — foi exatamente o que
    // aconteceu quando escrevi este comando, e o arquivo JA documentava a
    // mesma armadilha para `miditest`/`midiloop` contra `midi`.
    //
    // Licao escrita a dois comandos de distancia e licao nao aplicada.
    if(strncmp(line, "oledvel", 7) == 0)
    {
        const char* a = line + 7;
        while(*a == ' ') a++;
        const int ps = atoi(a);
        if(!hal::OledVelocidade(ps))
        {
            hal::Log("uso: oledvel <2|4|8|16|32|64|128|256>   (agora: %d)",
                     hal::OledVelocidadeAtual());
            return;
        }
        hal::OledInit();
        hal::OledPreenche(true);
        hal::Log("oledvel: prescaler %d, painel reinicializado, tela ACESA.",
                 hal::OledVelocidadeAtual());
        hal::Log("  quanto MAIOR o numero, mais LENTO o SPI — fio comprido"
                 " ou solda marginal perdoam mais devagar.");
        return;
    }
    // `oled [on|off]` — TESTE DE VIDA, nao driver de painel.
    //
    // Tela toda acesa prova que os cinco fios chegam. Apagar em seguida
    // prova que nao e so o painel energizado — um SSD1309 sem CLK/DIN
    // pode acender ruido no reset e parecer resposta.
    //
    // Init e Update rodam AQUI, no laco, por comando explicito. A escrita
    // e bloqueante (~1 KB a PS_64) e isso e aceitavel num comando de
    // bancada; no callback de audio nao seria, e por isso nao esta la.
    if(strncmp(line, "oled", 4) == 0)
    {
        const char* a = line + 4;
        while(*a == ' ') a++;
        const bool primeira = !hal::OledConfigurado();
        hal::OledInit();
        if(primeira)
        {
            // O QUE A PLACA SABE E O QUE ELA NAO PODE SABER, separados.
            // O `Init` da libDaisy e void: nao ha status. E o barramento
            // e TX-only, entao o painel NUNCA responde — nao existe
            // deteccao possivel aqui, hoje ou nunca.
            hal::Log("oled: CONFIGURADO (nao detectado — o painel e escrita"
                     " pura e nunca responde)");
            hal::Log("      SPI1 modo 0, PS_64 | CLK D8 · DIN D10 · CS D7"
                     " · DC D6 · RST D5 | MISO nao atribuido");
        }
        if(strncmp(a, "off", 3) == 0)
        {
            hal::OledPreenche(false);
            hal::Log("oled: apagada — e apagar prova que nao e so o painel"
                     " energizado (aceso e apagado sao escritas diferentes)");
            return;
        }
        if(strncmp(a, "texto", 5) == 0)
        {
            const char* t = a + 5;
            while(*t == ' ') t++;
            hal::OledTexto(*t ? t : "COTUCATRONICS", "v" COTUCA_OS_VERSION);
            hal::Log("oled: texto escrito. ISTO PROVA MAIS QUE O FILL —"
                     " tela cheia so mostra que o painel liga; texto prova"
                     " enderecamento de pixel e que o DC separa comando de"
                     " dado. Painel com DC trocado acende inteiro e nao"
                     " escreve nada legivel.");
            return;
        }
        hal::OledPreenche(true);
        hal::Log("oled: tela ACESA. Se acendeu inteira, os cinco fios chegam."
                 " `oled texto` escreve · `oled off` apaga.");
        return;
    }
    // `enc [reset]` — o teste E o instrumento de diagnostico depois.
    //
    // Reporta o CRU e a interpretacao ao lado, que e a regra da casa:
    // "crus" e o que a libDaisy entregou, "passos" e depois da divisao
    // por detente, "acel" e depois da aceleracao. Ver os tres juntos e o
    // que permite dizer se um numero errado veio do hardware, da divisao
    // ou da aceleracao — com um numero so, as tres causas leem igual.
    if(strncmp(line, "enc", 3) == 0)
    {
        const char* a = line + 3;
        while(*a == ' ') a++;
        hal::EncoderInit();
        // A tela liga junto: o instrumento serve para olhar o knob, nao
        // a serial. `enc tela off` cala se atrapalhar.
        hal::OledInit();
        if(strncmp(a, "tela", 4) == 0)
        {
            const char* v = a + 4;
            while(*v == ' ') v++;
            if(*v) enc_na_tela = (strncmp(v, "on", 2) == 0);
            enc_tela_ant[0][0] = 0;   // forca redesenho
            if(!enc_na_tela) hal::OledPreenche(false);
            hal::Log("enc: espelho na tela %s",
                     enc_na_tela ? "LIGADO" : "desligado");
            return;
        }
        if(strncmp(a, "reset", 5) == 0)
        {
            for(int e = 0; e < cotuca::hal::kNumEncoders; e++)
                enc_st[e] = EstadoEnc{};
            enc_tela_ant[0][0] = 0;   // forca redesenho
            hal::Log("enc: zerado. Gire e olhe a TELA — o contador acumula,"
                     " nao ha janela nem pressa.");
            return;
        }
        if(strncmp(a, "detente", 7) == 0)
        {
            const char* v = a + 7;
            while(*v == ' ') v++;
            const int n = atoi(v);
            if(n >= 1 && n <= 8)
            {
                enc_por_detente = n;
                for(int e = 0; e < cotuca::hal::kNumEncoders; e++)
                { enc_st[e].passos = 0; enc_st[e].resto = 0;
                  enc_st[e].acelerados = 0; }
                hal::Log("enc: %d pulso(s) por detente. Passos zerados —"
                         " gire dez e confira dez.", n);
            }
            else
                hal::Log("enc detente <1..8>  (agora: %d)", enc_por_detente);
            return;
        }
        static const char* kNome[] = {"cadeia A", "cadeia B", "global"};
        hal::Log("ENCODERS — %d pulso(s) por detente"
                 " (`enc detente <n>` ajusta, `enc reset` zera)",
                 enc_por_detente);
        for(int e = 0; e < cotuca::hal::kNumEncoders; e++)
        {
            const EstadoEnc& st = enc_st[e];
            hal::Log("  %-9s crus %+ld | passos %+ld | acel %+ld |"
                     " ult %s | push %lu%s",
                     kNome[e], (long)st.crus, (long)st.passos,
                     (long)st.acelerados,
                     st.ultimo_dir > 0 ? "CW " : st.ultimo_dir < 0 ? "CCW" : "-  ",
                     (unsigned long)st.pushes,
                     st.push_agora ? "  [APERTADO AGORA]" : "");
        }
        hal::Log("  (crus = o que a libDaisy entregou · passos = depois da"
                 " divisao · acel = depois da aceleracao por tempo)");
        hal::Log("  tela: %s | %lu updates ao painel | %lu chars escritos",
                 hal::OledConfigurado() ? (enc_na_tela ? "espelho LIGADO"
                                                       : "espelho desligado")
                                        : "OLED NAO CONFIGURADO",
                 (unsigned long)hal::OledUpdates(),
                 (unsigned long)hal::OledChars());
        return;
    }
    // `fio <rst|dc|cs|clk|din> <0|1>` — segura o pino e para.
    if(strncmp(line, "fio curto", 9) == 0)
    {
        char achados[160] = {0};
        const int n = hal::OledCacaPonte(achados, (int)sizeof(achados));
        if(!n)
            hal::Log("fio curto: nenhuma ponte entre os cinco pinos."
                     " Os dez pares conferidos, todos limpos.");
        else
            hal::Log("fio curto: %d PONTE(S):%s", n, achados);
        hal::Log("  (teste DIFERENCIAL: ponte e o pino que ACOMPANHA o"
                 " dirigido — alto com alto E baixo com baixo. Pino preso"
                 " por pull-up do painel fica alto nas duas e nao acusa.)");
        hal::Log("  o SPI foi desfeito — rode `oled` para reinicializar.");
        return;
    }
    if(strncmp(line, "fio", 3) == 0)
    {
        const char* a = line + 3;
        while(*a == ' ') a++;
        char nome[8] = {0};
        int  k = 0;
        while(*a && *a != ' ' && k < 7) nome[k++] = *a++;
        while(*a == ' ') a++;
        const bool alto = (*a == '1');
        if(!*nome || (*a != '0' && *a != '1'))
        {
            hal::Log("uso: fio <rst|dc|cs|clk|din> <0|1>");
            hal::Log("  segura o pino no nivel e PARA. Mede no conector do"
                     " painel: 0 = ~0 V, 1 = ~3,3 V.");
            hal::Log("  se o nivel nao chegar la, o defeito esta entre a"
                     " Daisy e o painel — pino, solda, fio ou conector.");
            return;
        }
        if(!hal::OledSondaPino(nome, alto))
        { hal::Log("fio: nome invalido (rst|dc|cs|clk|din)"); return; }
        hal::Log("fio %s = %s  (~%s V no conector do painel). Fica assim.",
                 nome, alto ? "ALTO" : "baixo", alto ? "3,3" : "0");
        hal::Log("  o SPI foi desconfigurado por isto — rode `oled` depois"
                 " para reinicializar o painel.");
        return;
    }
    if(strncmp(line, "pino", 4) == 0)
    {
        // PRIMEIRO A TRAVA (sem relogio), depois a janela (com relogio).
        // A ordem importa: a trava responde sozinha na maioria dos casos,
        // e quem le para de ler antes de precisar coordenar nada.
        hal::Log("pino: desde o boot, %lu amostras no laco —"
                 " CRU, sem debounce", (unsigned long)cru_amostras);
        for(int i = 0; i < hal::Info().num_buttons && i < 8; i++)
            hal::Log("  B%d  ja fechou pra GND: %-3s  (%lu amostras em baixo)",
                     i + 1, cru_ja_baixo[i] ? "SIM" : "NAO",
                     (unsigned long)cru_n_baixo[i]);
        if(strstr(line, "reset"))
        {
            for(int i = 0; i < 8; i++) { cru_ja_baixo[i] = false; cru_n_baixo[i] = 0; }
            cru_amostras = 0;
            hal::Log("  trava zerada — aperte os botoes e rode `pino` de novo");
            return;
        }
        int baixo[8] = {0};
        const int n = hal::Info().num_buttons;
        const int kAmostras = 400;
        for(int a = 0; a < kAmostras; a++)
        {
            for(int i = 0; i < n && i < 8; i++)
                if(hal::ButtonNivelCru(i)) baixo[i]++;
            hal::DelayMs(1);
        }
        hal::Log("pino: %d amostras em ~%d ms, por botao —"
                 " CRU, sem debounce", kAmostras, kAmostras);
        for(int i = 0; i < n && i < 8; i++)
            hal::Log("  B%d  baixo em %3d/%d amostras  (%s)",
                     i + 1, baixo[i], kAmostras,
                     baixo[i] == 0            ? "SEMPRE ALTO — aberto"
                     : baixo[i] == kAmostras  ? "SEMPRE BAIXO — fechado o tempo todo"
                                              : "ALTERNOU — o botao fecha e abre");
        return;
    }
    if(strncmp(line, "dreno", 5) == 0)
    {
        const char* a = line + 5;
        while(*a == ' ') a++;
        const uint32_t ms = (uint32_t)atoi(a);
        boot_dreno_ate = hal::NowMs() + (ms ? ms : kDrenoBootMs);
        hal::Log("dreno: rearmado por %lu ms — a entrada esta RECUSADA e"
                 " contada ate la (%lu drenados ate agora)",
                 (unsigned long)(ms ? ms : kDrenoBootMs),
                 (unsigned long)cc_dreno_boot);
        return;
    }
    if(strncmp(line, "espelho", 7) == 0)
    {
        const char* a = line + 7;
        while(*a == ' ') a++;
        if(*a) espelho_on = (strncmp(a, "on", 2) == 0);
        hal::Log("espelho: %s — a entrada (ch1/ch2) nao depende disto",
                 espelho_on ? "LIGADO (ch3 cadeia A, ch4 cadeia B)"
                            : "MUDO");
        return;
    }
    if(strncmp(line, "dup", 3) == 0)
    {
        const char* a = line + 3;
        while(*a == ' ') a++;
        if(*a) g_dup_por_cadeia = (strncmp(a, "on", 2) == 0);
        hal::Log("dup: %s — os dez params de caracter leem %s",
                 g_dup_por_cadeia ? "POR CADEIA" : "COMPARTILHADO",
                 g_dup_por_cadeia ? "p_x[c] (uma entrada por cadeia)"
                                  : "p_x[0] (cadeia B soa com valores da A)");
        return;
    }
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
        // OLED: "configurado", jamais "presente". Ver o cabecalho do
        // OledInit — nao ha caminho de leitura, entao afirmar presenca
        // seria inventar.
        hal::Log("oled: %s",
                 hal::OledConfigurado()
                     ? "configurado (D5 D6 D7 D8 D10) — presenca NAO e"
                       " verificavel: o painel nunca responde"
                     : "nao configurado ainda — `oled` inicializa e acende");
        if(hal::SerialTruncadas())
            hal::Log("serial: %lu linha(s) RECUSADA(S) por passar de %d chars"
                     " — comando pela metade nao e comando",
                     (unsigned long)hal::SerialTruncadas(), 160);
        hal::Log("tabelas: %d/%d params | %d/%d CCs%s",
                 params.Count(), (int)cotuca::kMaxParams,
                 manifest.n_midi, (int)cotuca::kMaxMidiMap,
                 (params.Overflow() || manifest.midi_overflow)
                     ? "   *** ESTOUROU ***" : "");
        // PARAMS SEM CC — o ponto cego que a duplicacao criou.
        //
        // Param sem CC nao esta quebrado: continua alcancavel por `set`
        // e continua sendo salvo. Mas param inalcancavel por MIDI, com
        // default zero, e indistinguivel de param DESLIGADO para quem
        // so tem o iPad na mao — e essa e a forma "feature morta parece
        // feature desligada", que ja custou uma sessao neste projeto.
        //
        // A linha imprime o CRU (a contagem) e a etiqueta ao lado, e
        // nomeia os primeiros para que a conta nao vire um numero sem
        // assunto.
        {
            bool tem_cc[cotuca::kMaxParams] = {false};
            for(int k = 0; k < manifest.n_midi; k++)
            {
                const int p = manifest.midi_map[k].param_id;
                if(p >= 0 && p < params.Count()) tem_cc[p] = true;
            }
            int sem = 0;
            char nomes[160]; nomes[0] = 0;
            for(int p = 0; p < params.Count(); p++)
            {
                if(tem_cc[p]) continue;
                sem++;
                if(sem <= 6)
                {
                    strncat(nomes, sem > 1 ? " " : "",
                            sizeof(nomes) - strlen(nomes) - 1);
                    strncat(nomes, params.Name(p),
                            sizeof(nomes) - strlen(nomes) - 1);
                }
            }
            hal::Log("sem CC: %d de %d params — alcancaveis por `set`,"
                     " invisiveis ao iPad", sem, params.Count());
            if(sem) hal::Log("        %s%s", nomes, sem > 6 ? " ..." : "");
        }
        // A TRAVA DO CTOS VEM ANTES DE QUALQUER OUTRO AVISO: enquanto
        // ela esta de pe, todo aviso que le um `_on` le o zero que ela
        // impos, nao a intencao do operador — e cala por causa disso.
        // Foi exatamente assim que o aviso do shimmer logo abaixo ficou
        // mudo enquanto o shimmer estava derrubado.
        {
            int travados = 0;
            for(int k = 0; k < manifest.n_sacrif; k++)
                if(soma_caiu[k]) travados++;
            if(travados)
                hal::Log("*** CTOS: %d efeito(s) derrubado(s) pelo teto e"
                         " TRAVADO(S). Ligar por CC ou por serial nao"
                         " adianta ate `ctos rearm`. Veja `ctos`. ***",
                         travados);
        }
        // DEPENDENCIA DECLARADA. O shimmer nao tem reverb proprio; com
        // `rev_on` em 0 ele emudece sem que nada pareca errado. Aviso
        // que so aparece quando a combinacao acontece — defesa orfa
        // nunca: declare a ameaca, ou ela vira folclore.
        for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
            if(params.Real(manifest.p_shm_on[c]) >= 0.5f
               && params.Real(manifest.p_rev_on[c]) < 0.5f)
                hal::Log("*** shm_on_%c ligado com rev_on_%c em 0: nada"
                         " entra no espaco %c, e o shimmer E a sala"
                         " realimentada. Ligue o envio. ***",
                         'a' + c, 'a' + c, 'A' + c);
        for(int k = 0; k < manifest.n_botoes; k++)
        {
            const cotuca::Manifest::BotaoAlvo& b = manifest.botao[k];
            if(b.param < 0) continue;
            hal::Log("B%d (%s): alterna %s | LED %d | %lu batidas",
                     k + 1, b.nome ? b.nome : "?", params.Name(b.param),
                     b.led_hw, (unsigned long)botao_batidas[k]);
        }
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
        const int trv = SacrifTravado(id);
        if(trv >= 0)
            hal::Log("*** ...e o CTOS vai desfazer isso. \"%s\" caiu pelo"
                     " teto em t=%.1f s e a trava esta de pe: o callback"
                     " zera esse _on todo bloco. `ctos rearm` pra soltar."
                     " ***",
                     manifest.sacrif[trv].nome,
                     (double)soma_caiu_ms[trv] / 1000.0);
        return;
    }
    if(strncmp(line, "dyn", 3) == 0)
    {
        for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
            hal::Log("gate %c thr=%.4f rel=%.0fms gain=%.2f env=%.4f",
                     'A' + c,
                     params.Real(manifest.p_gate_thresh[c]),
                     params.Real(manifest.p_gate_release[c]),
                     cad[c].gate.Gain(), cad[c].gate.Env());
        {
            cotuca::Governor* gv[2] = {&gov_l, &gov_r};
            for(int b = 0; b < 2; b++)
                hal::Log("gov %c ceil=%.2f rel=%.0fms gain=%.2f env=%.3f %s",
                         'A' + b,
                         params.Real(manifest.p_gov_ceiling[b]),
                         params.Real(manifest.p_gov_release[b]),
                         gv[b]->Gain(), gv[b]->Env(),
                         params.Real(manifest.p_exc_on[b]) >= 0.5f
                             ? "" : "(exciter FORA: protecao pulada)");
        }
        hal::Log("lim   ceil=%.2f gainL=%.2f gainR=%.2f",
                 cotuca::Limiter::Ceiling(), lim_l.Gain(), lim_r.Gain());
        hal::Log("boost A %.2fx | B %.2fx  (BANCADA — nascem em 1.00)",
                 params.Real(manifest.p_bench_boost[0]),
                 params.Real(manifest.p_bench_boost[1]));
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
        hal::Log("caixa-preta do voo anterior:");
        hal::Log("  blocos processados : %.0f", (double)hal::BlackboxRead(2));
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
    if(strncmp(line, "list", 4) == 0)
    {
        for(int i = 0; i < params.Count(); i++)
            hal::Log("%2d %-10s norm=%.2f real=%.2f%s",
                     i, params.Name(i), params.Get(i), params.Real(i),
                     SacrifTravado(i) >= 0 ? "   *** TRAVADO PELO CTOS ***"
                                          : "");
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
            // ALIMENTA A TABELA DE ECO, igual ao espelho. Sem isto o
            // teste tem comportamento DIFERENTE da feature que ele
            // deveria exercitar — e foi assim que um `midiloop` ficou
            // batendo com o espelho durante a bancada de 22/08.
            {
                // O canal do teste decide de QUAL cadeia e o vinculo:
                // ch3 espelha a A, ch4 espelha a B. Alimentar sempre a A
                // faria o `midiloop` divergir da feature no exato ponto
                // que ele existe para exercitar.
                // O canal do teste diz de QUAL entrada e o vinculo:
                // espelho - deslocamento devolve o canal de origem.
                const uint8_t cesp = (uint8_t)(ch - 1);
                if(cotuca::EhCanalEspelho(cesp))
                {
                    const uint8_t cin
                        = (uint8_t)(cesp - cotuca::kDeslocEspelho);
                    const int pid
                        = manifest.FindMidiParam(cin, (uint8_t)cc);
                    if(pid >= 0) eco_ult[pid] = (uint8_t)val;
                }
            }
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
            // Conta a INJECAO para que a trava do fio de volta possa
            // descontá-la. Aqui, e nao no OnMidiCc, porque e aqui que se
            // sabe a origem — la embaixo o parser ja nao distingue.
            injetados_local++;
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
        // OS TUNAVEIS DO MOTOR SEGUEM ALCANCAVEIS — na instancia que
        // EXISTE, que e a da cauda (shimmer). A rota de teste isolada
        // foi aposentada em 20/08 por custar uma instancia inteira so
        // pra bancada; os ajustes nao foram junto.
        if(strncmp(a, "grain", 5) == 0)
        {
            const float g = strtof(a + 5, nullptr);
            if(pitch_tail_ok) pitch_tail.SetGrainMs(0, g);
            hal::Log("pitch: grao = %.1f ms na cauda (laterais em +-%.0f Hz)",
                     (double)g, (double)(2000.f / (g > 0.f ? g : 1.f)));
            return;
        }
        if(strncmp(a, "jitter", 6) == 0)
        {
            const float j = strtof(a + 6, nullptr);
            if(pitch_tail_ok) pitch_tail.SetJitter(j);
            hal::Log("pitch: jitter = %.3f na cauda  (0 = graos"
                     " deterministicos, so pra MEDIR com seno)", (double)j);
            return;
        }
        hal::Log("pitch: a ROTA DE TESTE isolada foi aposentada em 20/08.");
        hal::Log("  custava uma instancia inteira do motor (153,6 KB de"
                 " SDRAM) so pra bancada, e");
        hal::Log("  com o detune fora do build o unico consumidor real"
                 " virou o shimmer, na cauda.");
        hal::Log("  restam: `pitch grain <ms>` e `pitch jitter <0-1>`,"
                 " que agem na instancia da cauda.");
        hal::Log("  reviver: docs/FEATURES.md, secao Arquivo.");
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
        const float ig = params.Real(manifest.p_in_gain[0]);
        // O REGIME NAO ESTA NO ALCANCE DESTE COMANDO, E ELE DIZ ISSO.
        //
        // Em 20/08 a entrada foi medida com dedo direto no disco solto e
        // o resultado virou "a entrada satura" no STATUS — afirmacao
        // sobre o instrumento MONTADO feita com uma medicao que nunca o
        // tocou. Nenhum numero estava errado; o alcance da conclusao e
        // que estava.
        //
        // O firmware nao tem como saber se o piezo esta colado na
        // escultura ou solto na bancada. O que ele pode fazer e nao
        // deixar o leitor esquecer que essa pergunta existe.
        hal::Log("HISTOGRAMA DE ENTRADA  (in_gain %.2fx)", (double)ig);
        hal::Log("  REGIME: nao sei qual e — declare voce. Piezo solto"
                 " batido a dedo e piezo");
        hal::Log("  montado excitado por vibracao sao instrumentos"
                 " diferentes pra esta medicao.");
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
            const float ig = params.Real(manifest.p_in_gain[0]);
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
            // O contador de estouros zera JUNTO: ele so significa
            // alguma coisa com um regime declarado, e o rearm e o
            // marco natural desse regime. Global, atravessando
            // mudancas de configuracao, seria numero sem denominador.
            est_n = est_ult_ms = est_ult_us = 0;
            est_pior_us = est_prim_ms = 0;
            est_perfil = 0;
            est_rajada = 0;
            for(int k = 0; k < manifest.n_sacrif; k++)
                soma_caiu_est[k] = false;
            hal::Log("ctos: efeitos rearmados — o veto volta a valer do zero");
            hal::Log("  teto de soma: latches soltos. Os efeitos NAO"
                     " religam sozinhos — ligue os que quiser de volta");
            return;
        }
        // `ctos prazo <us>` — prazo ARTIFICIAL, so pra teste. Sem
        // isto o criterio de estouro seria indemonstravel: ele mede
        // zero em todo regime real, e defesa que nunca se viu agir e
        // hipotese. Espelha o `ctos teto`, que existe pelo mesmo motivo.
        if(strncmp(a, "prazo", 5) == 0)
        {
            const char* v = a + 5;
            while(*v == ' ') v++;
            const long us = strtol(v, nullptr, 10);
            est_prazo_forc = (us > 0) ? (uint32_t)us : 0;
            est_rajada = 0;
            if(est_prazo_forc)
                hal::Log("ctos: prazo ARTIFICIAL em %lu us — %lu estouros"
                         " em %lu ms derrubam um efeito. `ctos prazo 0`"
                         " devolve o prazo real.",
                         (unsigned long)est_prazo_forc,
                         (unsigned long)kEstLimite,
                         (unsigned long)kEstJanelaMs);
            else
                hal::Log("ctos: prazo de volta ao real");
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
                hal::Log("      %d. %-8s CAIU por %-8s em t=%.1f s"
                         " (bloco estava em %lu us)", k,
                         manifest.sacrif[k].nome,
                         soma_caiu_est[k] ? "ESTOURO" : "carga",
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
        {
            const uint32_t prazo_real
                = (uint32_t)(1e6f * (float)hal::Info().block_size
                             / hal::Info().sample_rate);
            // O ROTULO USA O PRAZO EFETIVO. Em 22/08 ele imprimiu
            // ">1000 us" enquanto contava contra os 300 artificiais do
            // teste — instrumento que mente sobre o proprio criterio.
            const uint32_t prazo = est_prazo_forc ? est_prazo_forc
                                                  : prazo_real;
            if(est_prazo_forc)
                hal::Log("  *** PRAZO ARTIFICIAL em %lu us (real: %lu)"
                         " — modo de teste ***",
                         (unsigned long)est_prazo_forc,
                         (unsigned long)prazo_real);
            if(!est_n)
                hal::Log("  ESTOUROS DE PRAZO (>%lu us): nenhum desde o"
                         " rearm", (unsigned long)prazo);
            else
            {
                const uint32_t janela = est_ult_ms > est_prim_ms
                                            ? est_ult_ms - est_prim_ms : 0;
                hal::Log("  ESTOUROS DE PRAZO (>%lu us): %lu em %.1f s"
                         " — pior %lu us, ultimo %lu us em t=%.1f s",
                         (unsigned long)prazo, (unsigned long)est_n,
                         (double)janela / 1000.0,
                         (unsigned long)est_pior_us,
                         (unsigned long)est_ult_us,
                         (double)est_ult_ms / 1000.0);
                char perfil[80]; int w = 0;
                static const char* fam[7]
                    = {"drv","phs","wrb","dly","frz","rev","shm"};
                for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
                    for(int i = 0; i < 7; i++)
                        if(est_perfil & (1u << (c * 7 + i)))
                            w += snprintf(perfil + w, sizeof(perfil) - w,
                                          "%s%s%c", w ? " " : "",
                                          fam[i], 'a' + c);
                hal::Log("    no ar quando estourou: %s",
                         w ? perfil : "nenhum efeito");
                hal::Log("    criterio: %lu estouros em %lu ms derrubam"
                         " um efeito (rajada atual %lu)",
                         (unsigned long)kEstLimite,
                         (unsigned long)kEstJanelaMs,
                         (unsigned long)est_rajada);
            }
        }
        hal::Log("  rearmes pelo botao B3: %d  (o serial solta a trava"
                 " sem religar; o botao DEVOLVE os efeitos)",
                 soma_rearmes_botao);
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
        hal::Log("  WIDI LED LB (D28) — transicoes por 2 s / %% aceso:");
        hal::Log("    LB %lu/%u%%   (LG/LR removidos 20/08)",
                 (unsigned long)lb_trans, (unsigned)lb_duty);
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
        if(lb_duty > 90 && lb_trans == 0 && ble_last_rx_ms == 0
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
        // A DEFESA TEM QUE APARECER NUM CONTADOR. Uma que trabalha em
        // silencio e indistinguivel de uma que nao existe — foi
        // exatamente por nao ter este numero que o laco do canal 2
        // passou despercebido ate o `midi` mostrar as mensagens
        // proprias voltando.
        // OS DOIS JUNTOS, e a diferenca entre eles E o dado: TX conta o
        // que a placa pos no fio, "voltaram" conta o que o caminho
        // devolveu. TX > voltaram significa perda entre D13 e o iPad.
        hal::Log("  volta: %lu pelo FIO + %lu injetados por `miditest`%s",
                 (unsigned long)OuviDoFio(),
                 (unsigned long)injetados_local,
                 OuviDoFio() ? "   (o fio de volta FUNCIONA)"
                             : "   (o fio de volta NUNCA foi provado)");
        hal::Log("  eco: TX %lu (ch%d compart / ch%d A / ch%d B)"
                 " | voltaram %lu | perdidas %ld",
                 (unsigned long)eco_tx,
                 cotuca::CanalEspelhoDe(cotuca::kCanalCompart) + 1,
                 cotuca::CanalEspelhoDe(cotuca::kCanalEntradaA) + 1,
                 cotuca::CanalEspelhoDe(cotuca::kCanalEntradaB) + 1,
                 (unsigned long)eco_proprio,
                 (long)eco_tx - (long)eco_proprio);
        hal::Log("  delay: %lu vezes que o clamp do ponteiro de leitura"
                 " mordeu%s", (unsigned long)delay_fx.Clamps(),
                 delay_fx.Clamps()
                     ? "  *** atraso_ saiu da faixa: NaN ou Inf ***" : "");
        hal::Log("  autoria: %lu CCs recusados pela janela de %lu ms",
                 (unsigned long)cc_janela, (unsigned long)kJanelaMs);
    // OS DOIS DESCARTES NOVOS, cada um com o seu numero. Sem separa-los,
    // "o CC nao fez nada" teria duas causas indistinguiveis: canal sem
    // vinculo (widget na pagina errada) e endereço fora do mapa (widget
    // no numero errado). Sao consertos diferentes.
    hal::Log("     canal sem vinculo: %lu | (canal,cc) fora do mapa: %lu"
             " | drenados no boot: %lu",
             (unsigned long)cc_canal_errado, (unsigned long)cc_sem_dono,
             (unsigned long)cc_dreno_boot);
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
        // O CANAL ENTRA AQUI PELO MESMO MOTIVO QUE ENTROU NO `vistos`,
        // e eu consertei um e nao o irmao: sem ele este relatorio
        // imprime `cc 34 -> drv_drive_a` e `cc 34 -> drv_drive_b` em
        // linhas indistinguiveis, e quem esta na bancada le duplicata
        // onde ha dois enderecos.
        //
        // Defesa (ou correcao) aplicada num caminho e nao no outro e a
        // familia mais antiga do catalogo — o ramo R sem governor, o
        // `sync` sem janela de autoria, e agora este.
        for(int i = 0; i < manifest.n_midi; i++)
            hal::Log("  ch%d cc %-3d -> %s",
                     manifest.midi_map[i].canal + 1,
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
    // TODO COMANDO TRATADO TEM QUE APARECER AQUI, e o `make check-docs`
    // cobra. Tres comandos escritos hoje (`dup`, `espelho`, `dreno`)
    // existiam sem constar, e comando que nao consta e comando que nao
    // existe para quem esta na bancada com a serial aberta.
    hal::Log("? k1..k5 <0-1> | set <nome> <real> | tone on/off | board | list | mods | dyn | cpu | hist [reset] | probe | bb | midi | mscope on/off/stats | log on/off | ble [raw|mirror on/off] | sync | dfu go | pitch <ratio> | ctos [rearm|teto|prazo] | rearm | vistos [reset] | qspi id | miditest | midiloop | dup on/off | espelho on/off | capturar <a|b|g> <nome> | preset list/dump/load | (colar linhas `cadeia`/`global` de um dump recarrega a biblioteca) | oled [texto <msg>|off] | enc [reset|detente <n>|tela on/off] | fio <pino> <0|1> | fio curto | oledvel <n> | pino [reset] | dreno <ms> | pk (entrada crua por canal) | b1 randomize | b2 freeze | b3 rearme");
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
    // A `.itcm_text` mora gravada na QSPI e RODA na ITCM. Copiar aqui,
    // antes de qualquer chamada pra la — e depois do hal::Init(),
    // porque a copia usa cache e o cache e configurado la dentro.
    cotuca::qspi::CopiaParaItcm();
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


    // Salvaguarda (c): bench_boost TEM que nascer em 1.0. Hoje não
    // existe persistência de cena, então isto nunca dispara — é
    // tripwire pro dia em que existir. Um boost de 8x restaurado em
    // silêncio seria a escultura gritando no palco sem explicação.
    {
        const float b = params.Real(manifest.p_bench_boost[0])
                        + params.Real(manifest.p_bench_boost[1]) - 1.f;
        if(b != 1.0f)
            hal::Log("AVISO: bench_boost nasceu em %.2f e nao 1.00 — "
                     "ferramenta de bancada NAO deve persistir", b);
    }

    for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
        cad[c].gate.Init(hal::Info().sample_rate);
    gov_l.Init(hal::Info().sample_rate);
    gov_r.Init(hal::Info().sample_rate);
    lim_l.Init(hal::Info().sample_rate);
    lim_r.Init(hal::Info().sample_rate);
    shift_l.Init(hal::Info().sample_rate);
    shift_r.Init(hal::Info().sample_rate);

    // ---- MIDI ----
    // Depois do manifest.Init(): o parser recebe o canal do manifest,
    // e o mapa CC→param só existe depois dos Add(). Mesma regra de
    // ordem de configuração que vale pros bind[].
    // 0xFF = nunca visto, entao o primeiro passe do espelho empurra o
    // estado INTEIRO pro Loopy. E a resposta na direcao certa pro
    // problema que mordeu tres vezes hoje: gravar zera tudo e a tela do
    // dB continuava mostrando o valor velho. Agora a placa fala primeiro.
    for(int i = 0; i < cotuca::kMaxParams; i++) eco_ult[i] = 0xFF;
    // Armado DEPOIS do hal::Init(), porque `NowMs()` antes dele nao tem
    // base de tempo valida — a mesma pre-condicao que derrubou o vigia
    // do loop em 22/08 (bts 12z).
    boot_dreno_ate = hal::NowMs() + kDrenoBootMs;
    // O MAPA DE PRESET E O HASH SE CONSTROEM DEPOIS DO manifest.Init(),
    // porque os dois LEEM a tabela ja povoada. Ler antes daria um mapa
    // vazio e um hash de nada — e o hash de nada bate com o hash de
    // nada, entao todo preset passaria na conferencia (P2 do audit:
    // quem configura vem antes de quem consome).
    pmapa.Construir(params);
    mapa_hash = cotuca::MapaHash(params);
    hal::Log("presets: mapa 0x%04X | %d por cadeia + %d global%s",
             (unsigned)mapa_hash, pmapa.n_cadeia, pmapa.n_global,
             pmapa.estourou ? "   *** ESTOUROU o teto por preset ***" : "");
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
        const float  sr   = hal::Info().sample_rate;
        const size_t need = cotuca::Reverb::FloatsNeeded(sr);
        const size_t have = sizeof(reverb_mem[0]) / sizeof(reverb_mem[0][0]);
        for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
        {
            esp[c].reverb_ok = esp[c].reverb.Init(sr, reverb_mem[c], have);
            esp[c].suave.Init(sr, 20.f, 0.f);
            esp[c].shimmer.Init(sr);
            esp[c].shimmer_slot.Init(sr, 8.f, 2000.f); // cauda de 2 s
        }
        reverb_ok = esp[0].reverb_ok;
        suave_gl.Init(hal::Info().sample_rate, 20.f, 1.f);
        suave_gr.Init(hal::Info().sample_rate, 20.f, 1.f);
        // Chaves nascem FECHADAS (0): default zero, como todo param
        // alcancavel por MIDI.
        suave_dly.Init(hal::Info().sample_rate, 20.f, 0.f);
        suave_rev.Init(hal::Info().sample_rate, 20.f, 0.f);
        // Loga OS DOIS numeros. Se a taxa mudar e o `need` passar o
        // `have`, o reverb recusa (passthrough) e o log diz por que —
        // em vez de rodar lendo memoria alheia, que e o que um
        // buffer curto faria em silencio.
        hal::Log("espaco: sala A %s | sala B %s (%u de %u floats cada,"
                 " %u KB em SRAM no total)",
                 esp[0].reverb_ok ? "pronta" : "BUFFER CURTO",
                 esp[1].reverb_ok ? "pronta" : "BUFFER CURTO",
                 (unsigned)need, (unsigned)have,
                 (unsigned)(sizeof(reverb_mem) / 1024));
    }

    // MOTOR DE PITCH: alocar e inicializar ANTES do StartAudio, mesma
    // regra de ordem do reverb e do freeze.
    {
        const float  sr   = hal::Info().sample_rate;

        // Motor da cauda + shimmer. Mesmo tamanho de buffer: a oitava
        // acima exige a mesma folga de partida.
        // UM MOTOR DE PITCH POR ESPACO. Cada shimmer le a cauda da SUA
        // sala; um motor compartilhado misturaria as duas e o shimmer
        // de A ouviria o reverb de B.
        const size_t need_t
            = cotuca::PitchEngine::FloatsNeeded(sr, kPitchSeconds);
        for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
        {
            esp[c].pitch_mem = (float*)hal::AllocBig(need_t * sizeof(float));
            if(esp[c].pitch_mem)
                esp[c].pitch_ok = esp[c].pitch.Init(sr, esp[c].pitch_mem,
                                                    need_t, kPitchSeconds);
        }
        hal::Log("motores de pitch: A %s | B %s (%u KB cada em SDRAM)",
                 esp[0].pitch_ok ? "pronto" : "SEM BUFFER",
                 esp[1].pitch_ok ? "pronto" : "SEM BUFFER",
                 (unsigned)(need_t * sizeof(float) / 1024));
        // DELAY: 1 s a 48 kHz = 192 KB. Vai pra SDRAM pelo portao
        // unico (`AllocBig`), como os motores de pitch — nao cabe no
        // pool de SRAM e nao deve competir com ele.
        {
            const size_t nd = cotuca::Delay::FloatsNeeded(sr, kDelaySeg);
            delay_mem = (float*)hal::AllocBig(nd * sizeof(float));
            if(delay_mem)
                delay_ok = delay_fx.Init(sr, delay_mem, nd, kDelaySeg);
            hal::Log("delay: %s (%.0f ms max, %u KB em SDRAM, feedback %.2f)",
                     delay_ok ? "pronto" : "SEM BUFFER — passthrough",
                     (double)(kDelaySeg * 1000.f),
                     (unsigned)(nd * sizeof(float) / 1024),
                     (double)cotuca::Delay::kFeedback);
        }
        for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
        {
            Cadeia& k = cad[c];
            k.drive.Init(sr);   k.drive_slot.Init(sr, 8.f);
            k.phaser.Init(sr);  k.phaser_slot.Init(sr, 8.f);
            k.warble_ok = k.warble.Init(sr, warble_mem[c],
                                        sizeof(warble_mem[c])
                                            / sizeof(warble_mem[c][0]), 40.f);
            k.warble_slot.Init(sr, 8.f);
        }
        hal::Log("warble: A %s | B %s (40 ms cada, %u KB em SRAM no total)",
                 cad[0].warble_ok ? "pronto" : "BUFFER CURTO",
                 cad[1].warble_ok ? "pronto" : "BUFFER CURTO",
                 (unsigned)(sizeof(warble_mem)/1024));
        // CAUDA DE 2 s NA SAIDA (pedido do dB). Entrar e imediato;
        // sair e decaimento — o shimmer E a sala se empilhando, e sala
        // nao para de existir quando alguem solta o botao.
        shimmer_slot.Init(sr, 8.f, 2000.f);
        hal::Log("shimmer: %s (motor da cauda, %u KB em SDRAM)",
                 pitch_tail_ok ? "pronto" : "SEM BUFFER — inativo",
                 (unsigned)(need_t * sizeof(float) / 1024));
    }

    // FREEZE: mesma regra de ordem do reverb — alocar e inicializar
    // ANTES do StartAudio. E o tamanho é PERGUNTADO ao freeze, nunca
    // fixado aqui: se a taxa mudar, o pedido acompanha sozinho.
    {
#if COTUCA_FREEZE_ENABLED
        const float  sr   = hal::Info().sample_rate;
        const size_t need = cotuca::Freeze::FloatsNeeded(sr, kFreezeSeconds);
        for(int c = 0; c < cotuca::Manifest::kCadeias; c++)
        {
            freeze_c_mem[c] = (float*)hal::AllocBig(need * sizeof(float));
            if(freeze_c_mem[c])
                freeze_c_ok[c] = freeze_c[c].Init(sr, freeze_c_mem[c], need,
                                                  kFreezeSeconds);
        }
        hal::Log("freeze: A %s | B %s (%.1f s, %u KB cada em SDRAM)",
                 freeze_c_ok[0] ? "pronto" : "SEM BUFFER",
                 freeze_c_ok[1] ? "pronto" : "SEM BUFFER",
                 (double)kFreezeSeconds,
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
        // ---- ENCODERS, no laco ----
        hal::EncoderPoll();
        for(int e = 0; e < cotuca::hal::kNumEncoders; e++)
        {
            EstadoEnc& st = enc_st[e];
            const int  inc = hal::EncoderIncremento(e);
            if(inc)
            {
                st.crus += inc;
                st.ultimo_dir = inc;
                // Divisao com RESTO guardado: sem ele, dividir por 4
                // perderia ate tres pulsos a cada troca de sentido, e a
                // perda apareceria como "o encoder as vezes nao anda".
                st.resto += inc;
                while(st.resto >= enc_por_detente)
                { st.resto -= enc_por_detente; st.passos += 1; AplicaAcel(st, +1); }
                while(st.resto <= -enc_por_detente)
                { st.resto += enc_por_detente; st.passos -= 1; AplicaAcel(st, -1); }
            }
            if(hal::EncoderPushBorda(e)) st.pushes++;
            st.push_agora = hal::EncoderPushNivel(e);
        }

        // ---- ENCODER NA TELA ----
        if(enc_na_tela && hal::OledConfigurado()
           && (int32_t)(hal::NowMs() - enc_tela_prox) >= 0)
        {
            char l[3][24];
            static const char* kIni[] = {"A", "B", "G"};
            bool mudou = false;
            for(int e = 0; e < cotuca::hal::kNumEncoders; e++)
            {
                const EstadoEnc& st = enc_st[e];
                snprintf(l[e], sizeof(l[e]), "%s c%+ld p%+ld #%lu%s",
                         kIni[e], (long)st.crus, (long)st.passos,
                         (unsigned long)st.pushes, st.push_agora ? " *" : "");
                if(strcmp(l[e], enc_tela_ant[e]) != 0)
                { mudou = true; snprintf(enc_tela_ant[e],
                                         sizeof(enc_tela_ant[e]), "%s", l[e]); }
            }
            if(mudou)
            {
                hal::OledLinhas(l[0], l[1], l[2]);
                // 60 ms entre desenhos: mais rapido que isso o olho nao
                // acompanha, e cada Update bloqueia o laco.
                enc_tela_prox = hal::NowMs() + 60u;
            }
        }

        MidiEspelha();   // LOOP, nunca callback — ver cabecalho

        // VEREDITO DO SYNC. Roda no laco porque so aqui o
        // `hal::PollMidi()` ja teve chance de processar o que voltou.
        if(sync_confere_ate
           && (int32_t)(hal::NowMs() - sync_confere_ate) >= 0)
        {
            sync_confere_ate = 0;
            const uint32_t voltaram = midi.CcCount() - sync_cc_antes;
            const uint32_t barrados = cc_janela - sync_jan_antes;
            if(!voltaram)
            {
                // O QUE A PLACA SABE E O QUE ELA NAO SABE, separados.
                // Ela sabe que mandou e nao ouviu; nao sabe por que, e
                // dizer "o Loopy nao esta conectado" seria escolher uma
                // das tres causas para produzir uma frase util.
                hal::LogAlways("sync: AVISO — mandei %d CCs e NADA voltou em"
                               " %lu ms.", sync_mandados,
                               (unsigned long)kSyncEsperaMs);
                // A TRAVA CORTA O ESPACO DE CAUSAS AO MEIO.
                if(OuviDoFio())
                    hal::LogAlways("      MAS o fio de volta JA FUNCIONOU neste"
                                   " voo (%lu CCs pelo FIO desde o boot). Entao"
                                   " nao e cabo nem pareamento: sao os widgets,"
                                   " que nao estao ouvindo ch%d-ch%d.",
                                   (unsigned long)OuviDoFio(),
                                   cotuca::kDeslocEspelho + 1,
                                   cotuca::kDeslocEspelho
                                       + cotuca::kNumCanaisEntrada);
                else
                    hal::LogAlways("      E a placa NUNCA ouviu o controlador"
                                   " neste voo. Restam duas: os widgets nao"
                                   " escutam ch%d-ch%d, ou o fio de volta"
                                   " (WIDI OUT -> D14) nao fecha.",
                                   cotuca::kDeslocEspelho + 1,
                                   cotuca::kDeslocEspelho
                                       + cotuca::kNumCanaisEntrada);
                hal::LogAlways("      Nao confirmado quanto ao RETORNO. A"
                               " TRANSMISSAO a placa fez: conferir no MIDI"
                               " Wrench separa as duas coisas.");
            }
            else
            {
                hal::LogAlways("sync: OK — %d CCs enviados, %lu voltaram do"
                               " controlador (%lu barrados pela janela,"
                               " nenhum desfez nada).",
                               sync_mandados, (unsigned long)voltaram,
                               (unsigned long)barrados);
                if(voltaram < (uint32_t)sync_mandados / 2)
                    hal::LogAlways("      Voltou MENOS DA METADE: o link"
                                   " responde, mas parte dos widgets pode"
                                   " estar noutra pagina ou noutro canal.");
            }
        }

        FASE(3);
        // ---- PAINEL: botões (físico OU simulado por serial) ----
        // Amostra CRUA dos botoes, toda volta do laco. Barato (uma
        // leitura de GPIO por botao) e e o que alimenta a trava.
        {
            cru_amostras++;
            for(int i = 0; i < hal::Info().num_buttons && i < 8; i++)
                if(hal::ButtonNivelCru(i))
                { cru_ja_baixo[i] = true; cru_n_baixo[i]++; }
        }
        rng_state += 0x9E3779B9u; // gira sempre: a semente vira do tick
        // DESPACHO POR SAIDA ANTECIPADA, e nao por cadeia `else if`.
        //
        // A cadeia anterior tinha um ramo MORTO e ninguem via: para
        // b == 2, a condicao `b >= 1 && now` casava primeiro, entao o
        // `else if(b == 2 && now)` do rearme NUNCA executava. O
        // comentario de la afirmava que o B3 "cai no return do
        // AlternaBotao e segue pro rearme logo abaixo" — mas `else if`
        // nao funciona assim: o ramo tomado encerra a cadeia. O botao de
        // rearme do CTOS nao rearmava nada desde que foi escrito.
        //
        // Com saida antecipada, cada botao tem UM bloco e o especifico
        // vem antes do generico. Ramo morto por ordenacao deixa de ser
        // possivel, porque nao ha ordenacao entre condicoes — ha
        // sequencia de decisoes que terminam.
        for(int b = 0;
            b < hal::Info().num_buttons
                && b < cotuca::Manifest::kMaxBotoes;
            b++)
        {
            const bool now = hal::ReadButton(b) || (b == 1 && b2_sim)
                             || (b == 2 && b3_sim);
            if(now == btn_prev[b]) continue;
            btn_prev[b] = now;
            if(!now) continue;          // so age ao PRESSIONAR

            if(b == 0)
            { TriggerRandomize(false); continue; }

            if(b == 2)
            {
                // Segurar nao repete: o teto que derrubou uma vez
                // derrubaria de novo, e rearme em rajada viraria
                // oscilacao audivel.
                const int n = RearmaCtosDevolvendo();
                if(n) hal::Log("B3: %d efeito(s) devolvido(s) pelo botao", n);
                else  hal::Log("B3: nada estava travado");
                b3_sim = false;   // pulso consumido: sem isso o simulado
                                  // ficaria preso em alto e nunca mais
                                  // haveria outra borda
                continue;
            }

            // Qualquer outro botao com alvo declarado alterna. Botao sem
            // alvo cai no `return` do AlternaBotao e nao faz nada, que e
            // o comportamento certo — e agora e o UNICO caminho para
            // "nao faz nada", em vez de um efeito colateral da ordem.
            AlternaBotao(b);
            if(b == 1) b2_sim = false;   // pulso simulado consumido
        }

        // LED DO FREEZE — SEGUE O PARAM, NUNCA O GESTO.
        //
        // Atualizado aqui e nao dentro do AlternaFreeze de proposito: se
        // o freeze mudar por CC, serial, preset ou queda do CTOS, o LED
        // acompanha do mesmo jeito. Indicador que segue o gesto mentiria
        // em todos esses casos — e "mentir so as vezes" e o pior modo de
        // um indicador, porque ensina a confiar nele.
        {
            static bool led_ant[cotuca::Manifest::kMaxBotoes] = {false};
            for(int k = 0; k < manifest.n_botoes; k++)
            {
                const cotuca::Manifest::BotaoAlvo& b = manifest.botao[k];
                if(b.param < 0 || b.led_hw < 0) continue;
                const bool quer = params.Real(b.param) >= 0.5f;
                if(quer != led_ant[k])
                { led_ant[k] = quer; hal::SetPanelLed(b.led_hw, quer); }
            }
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
                const uint8_t msg[3] = {0xB0, kCCAviso, kAvisoSoma};
                hal::MidiSendBytes(msg, 3);
            }
        }
        if(aviso_shm_kill)
        {
            aviso_shm_kill = false;
            hal::LogAlways("CTOS: \"shimmer\" em BYPASS pelo watchdog"
                           " POR-EFEITO — %lu us de teto %lu (metade do"
                           " prazo de %lu).",
                           (unsigned long)aviso_shm_us,
                           (unsigned long)(aviso_shm_prazo / 2u),
                           (unsigned long)aviso_shm_prazo);
            hal::LogAlways("  isto NAO e o teto de soma: aqui HA culpado,"
                           " o efeito estourou o orcamento dele sozinho."
                           " `ctos rearm` devolve.");
            // MESMO CC, valor proprio: um indicador so cobre os dois
            // casos, e o valor diz qual dos dois foi.
            const uint8_t msg[3] = {0xB0, kCCAviso, kAvisoEfeito};
            hal::MidiSendBytes(msg, 3);
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
        // (Aqui morava o log de bancada por-4-blocos do fx2. REMOVIDO:
        // com o host desconectado, cada Log gastava o teto de 50 ms de
        // retry e chegava um novo a cada 21 ms — o loop virava zumbi
        // dentro do proprio instrumento de medicao, e foi ESSA a causa
        // de "a serial morre com o vocoder ligado". O cerco ja tinha
        // cumprido o papel; as medicoes vivem na caixa-preta.)


        // ARMADILHA DO PROPRIO INSTRUMENTO, custou tres voos:
        // contadores `static` ZERAM no boot, e o boot empurra os binds.
        // Entao a sessao que LE a caixa-preta reescrevia os slots antes
        // de eu olhar, e o `bb` reportava sempre "5 pushes" — o numero
        // da leitura, nao do voo. Diagnostico que se auto-apaga e pior
        // que diagnostico ausente: parece dado.
        //
        // Conserto: so grava com o voo ATIVO. buzios_on nasce em 0,
        // entao a sessao de leitura nao toca em nada.

        // O LED JA E O BATIMENTO — nao dava pra roubar sem perder a
        // prova de "loop vivo", que custou o diagnostico inteiro do
        // buzios. Entao o alarme entra na CADENCIA, nao no estado: com
        // efeito travado ele bate 4x mais rapido. Loop morto continua
        // congelando o LED nos dois regimes, que e o que importa
        // preservar. Sem isso, o botao B3 seria inutil: ninguem aperta
        // um rearme sem saber que precisa rearmar.
        if(++tick >= (SomaTravados() ? kLedToggleTicks / 4 : kLedToggleTicks))
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
