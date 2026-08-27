// ============================================================================
// Cotucatronics — ponte BLE MIDI <-> UART MIDI      ponte v0.1.0
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

#define PONTE_VERSAO "0.1.0"

// ---- pinos -----------------------------------------------------------------
// A GPIO Matrix do C3 poe a UART em quase qualquer pino — o oposto da Daisy.
// Evitados: GPIO20/21 (console USB, preciso deles para depurar), GPIO2/8/9
// (strapping), GPIO18/19 (USB nativo).
static const int PINO_TX     = 4;   // -> D14 da Daisy (RX)   ★ CRUZADO
static const int PINO_RX     = 5;   // <- D13 da Daisy (TX)   ★ CRUZADO
static const int PINO_STATUS = 6;   // -> D28 da Daisy, ATIVO EM BAIXO
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
BLEMIDI_CREATE_INSTANCE("Cotucatronics", RADIO)
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, SER)

// ---- contadores: o que a ponte sabe sobre si propria ------------------------
static volatile bool     conectado      = false;
static uint32_t          quedas         = 0;         // desconexoes desde o boot
static uint32_t          ultima_queda_ms = 0;        // 0 = nunca caiu
static uint32_t          msgs_ble_para_ser = 0;
static uint32_t          msgs_ser_para_ble = 0;
static uint32_t          saturacoes     = 0;         // TX sem espaco: CONTA, nao cala

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
        ultima_queda_ms = millis();   // RECENCIA: contagem sem o quando nao e dado
    });

    Serial.printf("ponte v%s — BLE MIDI <-> UART %lu 8N1\n",
                  PONTE_VERSAO, (unsigned long)BAUD);
    Serial.printf("  TX=GPIO%d -> D14 · RX=GPIO%d <- D13 · STATUS=GPIO%d -> D28\n",
                  PINO_TX, PINO_RX, PINO_STATUS);
    Serial.println("  `board` para o estado.");
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
                Serial.printf("  msgs: BLE->ser %lu · ser->BLE %lu · TX sem espaco %lu\n",
                              (unsigned long)msgs_ble_para_ser,
                              (unsigned long)msgs_ser_para_ble,
                              (unsigned long)saturacoes);
            }
            else if(n)
                Serial.println("comandos: board");
            n = 0;
        }
        else if(n < (int)sizeof(buf) - 1)
            buf[n++] = c;
    }
}

void loop()
{
    RADIO.read();
    SER.read();
    PiscaLed();
    Shell();
}
