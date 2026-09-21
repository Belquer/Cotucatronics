// ============================================================================
// Cotucatronics — ponte BLE MIDI <-> UART MIDI      ponte v0.4.0
// ESP32-C3 SuperMini · substitui o WIDI Core 1.3
//
// O QUE ESTA PLACA E: um fio com rádio. Nada mais.
//
// Ela NAO conhece CC nenhum, param nenhum, canal nenhum. Nao ha mapa aqui,
// e e de proposito: um mapa com duas casas deriva, e a deriva entre dois
// firmwares e o defeito mais caro que este projeto poderia inventar agora.
// Se um dia esta ponte precisar de saber o que uma mensagem SIGNIFICA, a
// fronteira engordou e a decisao inteira volta a mesa.
//
// PRECISAO SOBRE "PASSA BYTES": o BLE MIDI e enquadrado — cada pacote leva
// um header e timestamps intercalados. Desenquadrar e OBRIGATORIO, nao ha
// como nao o fazer. Entao a ponte trabalha ao nivel da MENSAGEM: recebe,
// e reemite igual do outro lado. O que ela nao faz e interpretar o
// SENTIDO — e essa a propriedade que interessa.
//
// D28 E ATIVO EM BAIXO. Medido no firmware da Daisy em 25/08:
//     kLbAcesoEmBaixo = true          (main.cpp)
//     Pull::PULLUP no pino            (hal_daisy_seed.cpp)
// Entao: conectado = puxar a LOW. Desconectado = SOLTAR (alta impedancia)
// e deixar o pull-up interno da Daisy levar o pino a alto.
// Uma versao anterior da especificacao dizia "alto = conectado" e pedia um
// pull-down externo: as duas invertidas. O pull-down teria feito o pino
// flutuante ler "conectado" durante o boot desta placa — exatamente o modo
// de falha que ele existia para evitar.
//
// O LED E DA PESSOA, O PINO E DA MAQUINA. Servir os dois no mesmo fio e
// variavel com dois empregos: a maquina quer NIVEL para decidir, o humano
// quer CADENCIA para entender. Sao fios diferentes, e por isso o D28 nunca
// pisca.
// ============================================================================

#include <Arduino.h>
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
#include <MIDI.h>
#include <Preferences.h>

#define PONTE_VERSAO "1.0.0"

// ---- pinos -----------------------------------------------------------------
// ⚠️⚠️ MAJOR a 21/09: OS PINOS SAO OS DA PLACA, E A PLACA E QUE MANDA.
//
// Ate a v0.5.0 este ficheiro escolhia os pinos — GPIO4/5/6 — porque a ponte
// vivia em cinco fios soltos e a escolha era dele. Desde a rev C o modulo
// entra num SOQUETE (`U3` do cerebro) e os pinos estao em COBRE:
//
//     IO20 <- MIDI_TX   (D13/USART1_TX da Daisy)   -> e o RX desta placa
//     IO21 -> MIDI_RX   (D14/USART1_RX da Daisy)   -> e o TX desta placa
//     IO3  -> PONTE_STATUS (D28 da Daisy), ATIVO EM BAIXO
//
// O firmware antigo gravado numa placa rev C da EXACTAMENTE o sintoma de
// 21/09: o iPad ve e liga (o radio nao usa estes pinos), e a Daisy conta
// `pico 0 de 128 bytes do anel` — nunca chegou um byte. Nao ha como o
// perceber pelo BLE, porque o BLE funciona.
//
// ⚠️ O preco de IO20/21: sao o UART0 da ROM. No arranque o bootloader da ROM
// cospe o log dele a 115200 por IO21, e esses bytes entram no RX de MIDI da
// Daisy. Sao lixo para um parser de MIDI, que os ignora, e duram o arranque —
// mas e uma diferenca real face aos GPIO4/5, e fica dita.
// ⚠️ E por isso o `Serial` desta placa TEM de ser USB CDC (`CDCOnBoot=cdc`):
// sem isso a consola sai por IO20/21 e mistura-se com o MIDI. Ver esp32/README.
static const int PINO_TX     = 21;  // -> D14 da Daisy (RX)   ★ CRUZADO
static const int PINO_RX     = 20;  // <- D13 da Daisy (TX)   ★ CRUZADO
static const int PINO_STATUS = 3;   // -> D28 da Daisy, ATIVO EM BAIXO
static const int PINO_LED    = 8;   // LED de bordo do SuperMini, ativo em baixo

// GPIO8 e strapping, mas so no instante do boot; depois disso e o LED de
// bordo e usa-se sem drama. Nao o conduzimos antes do setup().

// ---- baud: DECLARADO, porque a fronteira fina nao se verifica sozinha ------
// A ponte so passa bytes, logo nao ha aperto de mao de versao entre as duas
// placas — e nao ha de proposito: um handshake obrigaria esta placa a falar
// por conta propria no fluxo, e ai ela deixa de ser fio e vira autora.
// O preco assumido e este: a convencao defende-se por PROCEDIMENTO. O numero
// vive aqui, no README dos dois lados, e sai no `board` de cada um. Mudar e
// MAJOR nas duas.
static const uint32_t BAUD = 31250;

// ---- buffer de TX: decisao explicita, nao default da biblioteca ------------
// O BLE entrega mais depressa do que 31250 baud escoa, logo o gargalo e o
// buffer e nao o radio. A maior rajada do sistema e CONHECIDA: o `sync` da
// Daisy manda 86 CCs = 258 bytes ~ 82 ms de UART. 1024 da folga de 4x.
static const size_t TX_BUFFER = 1024;

// O macro cria o transporte como BLE<Nome>: aqui, BLERADIO.
//
// O NOME LONGO CABE, E FOI MEDIDO. A conta do pacote primario de anuncio
// diz que nao caberia:
//   31 bytes - 3 (flags) - 18 (UUID do servico MIDI, 128 bit) = 10,
//   ou seja 2 de cabecalho + 8 caracteres, e "Cotucatronics" tem 13.
// A conta esta certa e o modelo estava incompleto: o nome vai na RESPOSTA
// AO SCAN, que e um segundo pacote de 31 bytes, e o iOS faz varredura
// ativa. Em 25/08 encurtei para "Cotuca" com base so na conta — e o dB ja
// tinha visto "Cotucatronics" aparecer no Loopy. Revertido.
//
// A licao e a da casa: nao agir sobre o modelo quando a medicao esta a
// caminho. E o nome importa alem da estetica — o perfil de controlo do
// Loopy fica preso a identidade do dispositivo.
BLEMIDI_CREATE_INSTANCE("Cotucatronics", RADIO)
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, SER)

// ---- contadores: o que a ponte sabe sobre si propria ------------------------
static volatile bool     conectado      = false;
static uint32_t          quedas         = 0;         // desconexoes desde o boot
static uint32_t          ultima_queda_ms = 0;        // 0 = nunca caiu

// ---- O QUE SOBREVIVE AO CORTE DE ENERGIA -----------------------------------
//
// O contador de quedas vivia so em RAM, e isso tornava-o inutil para a
// pergunta que se faz numa galeria: "caiu de madrugada?". Um piscar de
// energia — que e UMA DAS CAUSAS DE QUEDA — zerava a testemunha da queda.
// Instrumento cuja memoria e apagada pelo evento que ele mede so serve
// enquanto ninguem precisa dele.
//
// DUAS CONTAS, e sao perguntas diferentes:
//   `quedas`      nesta sessao  -> "esta a cair AGORA?"
//   `quedas_vida` desde sempre  -> "caiu enquanto eu nao estava?"
// `arranques` separa as duas: vida alta com arranques altos e energia
// instavel, nao radio instavel.
static Preferences       nvs;
static uint32_t          quedas_vida    = 0;
static uint32_t          arranques      = 0;
static volatile bool     nvs_sujo       = false;
static uint32_t          nvs_ultimo_ms  = 0;

// NAO SE ESCREVE A CADA QUEDA, por duas razoes independentes:
//   1. NVS tem endurance finita, e uma peca de galeria fica meses ligada.
//   2. O callback de desconexao corre no contexto do stack BLE; escrever
//      flash de la e trabalho longo no sitio errado.
// Entao a queda so MARCA, e o laco escreve — no maximo a cada 10 s.
static const uint32_t    NVS_MIN_MS     = 10000;

static void NvsGrava()
{
    nvs.putUInt("vida", quedas_vida);
    nvs_sujo      = false;
    nvs_ultimo_ms = millis();
}
static uint32_t          msgs_ble_para_ser = 0;
static uint32_t          msgs_ser_para_ble = 0;
static uint32_t          saturacoes     = 0;         // TX sem espaco: CONTA, nao cala
// BYTES CRUS, ANTES DO PARSER. Distingue "nao chegou nada" de "chegou lixo":
// fio errado da zero; baud errado ou inversao da bytes que o parser MIDI
// descarta em silencio. Sem este contador os dois casos sao o mesmo numero.
static uint32_t          bytes_crus     = 0;
static uint8_t           ultimos[16];
static uint8_t           n_ultimos      = 0;

// ---- estado do pino, num sitio so ------------------------------------------
// Aberto-dreno por construcao: conduzir baixo, ou SOLTAR. Nunca conduzir alto
// — quem leva o pino a alto e o pull-up da Daisy, e assim o estado "sem link"
// e o mesmo quer esta placa esteja desligada, em boot, ou desconectada.
static void AplicaStatus(bool ligado)
{
    if(ligado) { pinMode(PINO_STATUS, OUTPUT); digitalWrite(PINO_STATUS, LOW); }
    else       { pinMode(PINO_STATUS, INPUT);  }   // alta impedancia = solto
}

static void Encaminha(const midi::Message<128>& m, bool da_ble)
{
    // Reemissao ao nivel da mensagem. Sem olhar para o numero do CC, sem
    // olhar para o canal, sem tabela. O `if` aqui e sobre a FORMA da
    // mensagem (quantos bytes leva), nunca sobre o que ela quer dizer.
    if(da_ble)
    {
        SER.send(m.type, m.data1, m.data2, m.channel);
        msgs_ble_para_ser++;
    }
    else
    {
        RADIO.send(m.type, m.data1, m.data2, m.channel);
        msgs_ser_para_ble++;
    }
}

static void DaBle(const midi::Message<128>& m)
{
    // Espaco no buffer ANTES de escrever. Se nao houver, conta — e escreve
    // na mesma. Perder calado seria a falha que este projeto mais persegue:
    // toda recusa e CONTADA e visivel num comando de status.
    if(Serial1.availableForWrite() < 3) saturacoes++;
    Encaminha(m, true);
}

static void DaSerial(const midi::Message<128>& m) { Encaminha(m, false); }

void setup()
{
    Serial.begin(115200);            // USB CDC — so para o humano e o `board`
    AplicaStatus(false);             // ANTES de tudo: nasce dizendo "sem link"
    pinMode(PINO_LED, OUTPUT);
    digitalWrite(PINO_LED, HIGH);    // ativo em baixo: HIGH = apagado

    Serial1.setTxBufferSize(TX_BUFFER);   // tem de vir ANTES do begin
    Serial1.begin(BAUD, SERIAL_8N1, PINO_RX, PINO_TX);

    RADIO.begin(MIDI_CHANNEL_OMNI);
    SER.begin(MIDI_CHANNEL_OMNI);
    RADIO.turnThruOff();
    SER.turnThruOff();

    RADIO.setHandleMessage(DaBle);
    SER.setHandleMessage(DaSerial);

    BLERADIO.setHandleConnected([]() {
        conectado = true;
        AplicaStatus(true);
    });
    BLERADIO.setHandleDisconnected([]() {
        conectado = false;
        AplicaStatus(false);
        quedas++;
        quedas_vida++;
        nvs_sujo = true;              // o LACO escreve; aqui e contexto do BLE
        ultima_queda_ms = millis();   // RECENCIA: contagem sem o quando nao e dado
    });

    // NVS ANTES DE QUALQUER COISA QUE POSSA CAIR: se um link caisse antes
    // disto, a queda seria contada contra um `quedas_vida` ainda em zero e
    // perdia-se. Configurar antes de consumir, a familia B do audit.
    nvs.begin("ponte", false);
    quedas_vida = nvs.getUInt("vida", 0);
    arranques   = nvs.getUInt("boots", 0) + 1;
    nvs.putUInt("boots", arranques);
    nvs_ultimo_ms = millis();

    Serial.printf("ponte v%s — BLE MIDI <-> UART %lu 8N1\n",
                  PONTE_VERSAO, (unsigned long)BAUD);
    Serial.printf("  memoria: %lu quedas desde sempre, em %lu arranques\n",
                  (unsigned long)quedas_vida, (unsigned long)arranques);
    Serial.printf("  TX=GPIO%d -> D14 · RX=GPIO%d <- D13 · STATUS=GPIO%d -> D28\n",
                  PINO_TX, PINO_RX, PINO_STATUS);
    Serial.println("  `board` estado · `zera` apaga a memoria de quedas · `teste` dispara um CC.");
}

// ---- LED: a cadencia e nossa, e mora SO deste lado -------------------------
static void PiscaLed()
{
    // Conectado: aceso fixo. Anunciando: pulso curto a cada 2 s — o suficiente
    // para se ver de longe sem chamar atencao numa galeria.
    static uint32_t prox = 0;
    static bool     est  = false;
    const uint32_t ag = millis();
    if(conectado) { digitalWrite(PINO_LED, LOW); return; }
    if(ag < prox) return;
    est = !est;
    digitalWrite(PINO_LED, est ? LOW : HIGH);
    prox = ag + (est ? 80 : 1920);
}

static void Shell()
{
    static char buf[32];
    static int  n = 0;
    while(Serial.available())
    {
        const char c = (char)Serial.read();
        if(c == '\n' || c == '\r')
        {
            buf[n] = 0;
            if(n && strncmp(buf, "board", 5) == 0)
            {
                Serial.printf("ponte v%s · BAUD %lu · %s\n", PONTE_VERSAO,
                              (unsigned long)BAUD,
                              conectado ? "CONECTADO" : "sem link");
                Serial.printf("  pinos: TX=%d RX=%d STATUS=%d (ativo em BAIXO) LED=%d\n",
                              PINO_TX, PINO_RX, PINO_STATUS, PINO_LED);
                // QUEDAS COM RECENCIA. "Quantas desde o boot" nao responde a
                // pergunta que se faz numa galeria, que e "esta a cair AGORA?".
                if(ultima_queda_ms)
                    Serial.printf("  quedas: %lu · ultima ha %lu s\n",
                                  (unsigned long)quedas,
                                  (unsigned long)((millis() - ultima_queda_ms) / 1000));
                else
                    Serial.printf("  quedas: %lu · nenhuma desde o boot\n",
                                  (unsigned long)quedas);
                Serial.printf("  desde sempre: %lu quedas em %lu arranques%s\n",
                              (unsigned long)quedas_vida,
                              (unsigned long)arranques,
                              nvs_sujo ? "  (por gravar)" : "");
                Serial.printf("  msgs: BLE->ser %lu · ser->BLE %lu · TX sem espaco %lu\n",
                              (unsigned long)msgs_ble_para_ser,
                              (unsigned long)msgs_ser_para_ble,
                              (unsigned long)saturacoes);
            }
            else if(n && strncmp(buf, "zera", 4) == 0)
            {
                quedas = quedas_vida = 0;
                arranques = 1;
                ultima_queda_ms = 0;
                nvs.putUInt("vida", 0);
                nvs.putUInt("boots", 1);
                nvs_sujo = false;
                nvs_ultimo_ms = millis();
                Serial.println("zera: contadores de vida apagados da NVS."
                               " A sessao tambem foi a zero.");
            }
            else if(n && strncmp(buf, "cru", 3) == 0)
            {
                Serial.printf("cru: %lu bytes vistos na Serial1 · %d na fila agora\n",
                              (unsigned long)bytes_crus, Serial1.available());
                Serial.printf("     se for 0 o fio D13->GPIO%d nao entrega;\n", PINO_RX);
                Serial.printf("     se for >0 e ser->BLE for 0, chega LIXO"
                              " (baud, inversao, ou massa).\n");
            }
            else if(n && strncmp(buf, "pino", 4) == 0)
            {
                // FERRAMENTA DE FIO: conduz um GPIO para se poder conferir do
                // outro lado. Cada fio testa-se sozinho, em vez de se adivinhar
                // qual dos tres esta errado.
                int g = -1, v = -1;
                sscanf(buf + 4, "%d %d", &g, &v);
                if(g >= 0 && (v == 0 || v == 1))
                {
                    pinMode(g, OUTPUT); digitalWrite(g, v ? HIGH : LOW);
                    Serial.printf("pino: GPIO%d -> %s\n", g, v ? "ALTO" : "BAIXO");
                }
                else if(g >= 0 && v == 2)
                {
                    pinMode(g, INPUT);
                    Serial.printf("pino: GPIO%d -> SOLTO (alta impedancia)\n", g);
                }
                else Serial.println("uso: pino <gpio> <0|1|2>   (2 = solta)");
            }
            else if(n && strncmp(buf, "pisca", 5) == 0)
            {
                // Pulsa um GPIO 20x a 4 Hz. Do outro lado, o `ble` da Daisy
                // conta transicoes: se contar ~20, o fio entrega.
                int g = PINO_STATUS;
                sscanf(buf + 5, "%d", &g);
                Serial.printf("pisca: GPIO%d, 20 pulsos a 4 Hz — le `ble` na Daisy\n", g);
                pinMode(g, OUTPUT);
                for(int i = 0; i < 20; i++)
                { digitalWrite(g, LOW); delay(125); digitalWrite(g, HIGH); delay(125); }
                AplicaStatus(conectado);
                Serial.println("pisca: fim");
            }
            else if(n && strncmp(buf, "serteste", 8) == 0)
            {
                // O IRMAO DO `teste`, e faltava. O `teste` prova o caminho
                // para o RADIO; este prova o caminho para o FIO. Sem os dois,
                // "a Daisy nao ouviu" nao distingue fio partido de ponte muda.
                int cc = 20, val = 100, ch = 1, rep = 10;
                sscanf(buf + 8, "%d %d %d %d", &cc, &val, &ch, &rep);
                for(int i = 0; i < rep; i++)
                {
                    SER.sendControlChange((uint8_t)cc, (uint8_t)val, (uint8_t)ch);
                    delay(30);
                }
                Serial.printf("serteste: %dx cc%d val%d ch%d -> Serial1 (GPIO%d)\n",
                              rep, cc, val, ch, PINO_TX);
            }
            else if(n && strncmp(buf, "teste", 5) == 0)
            {
                // BANCADA APENAS, E A TENSAO FICA DITA: isto poe a ponte a
                // EMITIR, que e o que o desenho proibe em operacao. Tres
                // coisas o tornam aceitavel, e nenhuma delas e conveniencia:
                //   1. so existe atras do console USB, que ninguem tem em palco;
                //   2. e disparado por uma pessoa, nunca por logica interna;
                //   3. paga um teste que o portao precisa — SEPARAR O RADIO DO
                //      FIO. Sem ele, "o widget nao mexeu" nao distingue radio
                //      mudo de fio trocado, e o portao reprovaria a coisa certa
                //      pelo motivo errado.
                // Nao ha mapa aqui: os numeros vem do comando, nao de tabela.
                int cc = 20, val = 100, ch = 1;
                sscanf(buf + 5, "%d %d %d", &cc, &val, &ch);
                RADIO.sendControlChange((uint8_t)cc, (uint8_t)val, (uint8_t)ch);
                Serial.printf("teste: cc%d val%d ch%d -> BLE\n", cc, val, ch);
            }
            else if(n)
                Serial.println("comandos: board · cru · pino <g> <0|1|2> · pisca [g] · teste [..] · serteste [..]");
            n = 0;
        }
        else if(n < (int)sizeof(buf) - 1)
            buf[n++] = c;
    }
}

static void EspiaCru()
{
    // Le do driver e devolve? Nao da. Entao contamos o que o driver DIZ ter,
    // sem consumir — available() nao remove nada da fila.
    static int ant = 0;
    const int ag = Serial1.available();
    if(ag > ant) bytes_crus += (uint32_t)(ag - ant);
    ant = ag;
}

void loop()
{
    EspiaCru();
    RADIO.read();
    SER.read();
    PiscaLed();
    Shell();
    // ESCRITA DIFERIDA. A queda so marcou; e aqui que a flash e tocada, e
    // no maximo a cada 10 s. Isto tambem escoa naturalmente para o momento
    // certo: depois de uma queda nao ha trafego BLE a atrasar.
    if(nvs_sujo && (uint32_t)(millis() - nvs_ultimo_ms) >= NVS_MIN_MS)
        NvsGrava();
}
