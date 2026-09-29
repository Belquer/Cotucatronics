// ============================================================================
// Cotucatronics — ponte BLE MIDI <-> UART MIDI      ponte v1.4.1
//                 + consola sem fio por NUS (ppc/006 fase 1)
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

#define PONTE_VERSAO "1.4.1"

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

// ============================================================================
// CONSOLA SEM FIO — Nordic UART Service ao lado do BLE MIDI   (ppc/006 fase 1)
// ============================================================================
//
// POR QUE EXISTE. Metade do trabalho de Setembro foi diagnostico pela consola
// da Daisy, e a consola vive no USB — o cabo que obriga a abrir a caixa. Isto
// poe a MESMA consola por radio, sem tocar no MIDI.
//
// ⚠️ E NAO HA FIO SOBRANDO entre as duas placas: so IO20/IO21/IO3. A consola
// tem de caber no par que ja carrega MIDI, e a unica moldura que nao se
// confunde com CC e o SysEx — que a Daisy ja parseia.
//
//     Mac  --BLE NUS-->  esta placa  --F0 7D <tipo> ... F7-->  Daisy
//
//   tipo 0x01  Daisy -> Mac   (linhas de log)
//   tipo 0x02  Mac   -> Daisy (comandos)
//
// (!) O CORPO DE UM SYSEX SO ADMITE BYTES < 0x80, e o log da Daisy tem
// acentos e simbolos em UTF-8. Entao empacota-se 8->7: sete bytes originais
// viajam em oito, com os bits altos recolhidos no primeiro. Nao e escolha
// estetica — sem isso um "ç" fecha o SysEx a meio.
//
// ⚠️ O SERVICO NUS NAO VAI NO ANUNCIO. O pacote de anuncio tem 31 bytes e ja
// leva o servico MIDI e o nome "Cotucatronics", que o perfil do Loopy usa
// como identidade. Servico anunciado a mais partiria o pareamento existente
// para ganhar o que a descoberta pos-ligacao ja da de graca.
#include <NimBLEDevice.h>
#include <Update.h>
#include <esp_ota_ops.h>

static const char* NUS_SVC = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char* NUS_RX  = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";  // Mac escreve
static const char* NUS_TX  = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";  // placa notifica

static NimBLECharacteristic* nus_tx = nullptr;

// Fila do que veio do Mac. ⚠️ A CALLBACK DO NIMBLE CORRE NOUTRA TAREFA, e
// escrever na Serial1 de la interleava com o que a biblioteca MIDI esta a
// enviar — tres bytes de um CC partidos ao meio por uma linha de consola.
// Entao a callback so ENFILEIRA; quem escreve e o laco, que e o unico dono
// da porta. Mesma disciplina do `MidiTxPump` da Daisy.
static const size_t NUS_FILA = 1024;
static uint8_t   nus_fila[NUS_FILA];
static volatile size_t nus_cab = 0, nus_cau = 0;
static uint32_t  nus_descartes = 0;   // bytes do Mac que nao couberam
static uint32_t  nus_para_daisy = 0, nus_do_daisy = 0;
// ⚠️ CONTA A ESCRITA, NAO SO O FIM DO CAMINHO. A 29/09 o `consola:` deu zero
// e isso nao distinguia "o BLE nao entregou" de "a fila nao escoou" — um
// contador no fim do percurso responde por todo o percurso, que e o mesmo
// defeito do `pkout` sem o `pk`. Este conta a CHEGADA.
static uint32_t  nus_escritas = 0, nus_bytes_crus = 0;

// ---- O ANUNCIO FICA DE PE ENQUANTO HOUVER VAGA -----------------------------
//
// 🔴 MEDIDO a 29/09. O `begin()` do BLEMIDI faz `advertiseOnDisconnect(true)`,
// e no NimBLE o anuncio PARA quando alguem se liga. Consequencia: com o Loopy
// agarrado, a ponte fica INVISIVEL — o Mac varre e nao a ve.
//
// ⚠️ E "nao vi" e indistinguivel de "esta desligada". O dB pediu feedback para
// esse caso; o conserto certo nao e explicar melhor o silencio, e ELIMINA-LO:
// o C3 aceita CONFIG_BT_NIMBLE_MAX_CONNECTIONS = 3 ligacoes, entao cabem o
// iPad, o Mac e uma de sobra. Reanuncia-se enquanto houver vaga, e as duas
// consolas convivem — MIDI para o Loopy, consola para quem estiver a medir.
// ============================================================================
// OTA DA PONTE — gravar por radio, sem hipotese de brick (ppc/006 fase 2)
// ============================================================================
//
// O bootloader deste nucleo traz `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`
// (verificado no sdkconfig do C3, 29/09). Isso muda tudo: a imagem nova
// arranca em PENDING_VERIFY e, se ninguem a CONFIRMAR, o bootloader volta
// sozinho a anterior no reinicio seguinte.
//
// ⚠️ E A CONFIRMACAO NAO PODE SER NA PRIMEIRA LINHA DO `setup()`. Confirmar
// cedo e o mesmo que nao ter rede: uma imagem que arranca e morre a seguir
// seria dada por boa. Aqui so se confirma depois de o RADIO estar a anunciar
// **e** o laco ter dado 15 s — o que apanha crash-loop e radio morto, que sao
// os dois modos que deixariam a caixa inalcancavel.
//
// O protocolo entra pela consola: uma linha comecada por `!` e para a PONTE,
// tudo o resto segue para a Daisy. Isso resolve de borda outra coisa — ate
// agora o estado da ponte so se via pelo USB dela.
//
//     !board            estado da ponte, por radio
//     !ota <N> <md5>    recebe N bytes de imagem, verificados por MD5
//
static bool      ota_ativo   = false;
static uint32_t  ota_total   = 0, ota_recebido = 0, ota_ultimo_ms = 0;
static uint32_t  ota_tentativas = 0;
static char      ota_erro[64] = {0};
static bool      confirmado  = false;
static uint32_t  arranque_ms = 0;

static void OtaAborta(const char* porque)
{
    Update.abort();
    ota_ativo = false;
    snprintf(ota_erro, sizeof(ota_erro), "%s", porque);
}

static uint32_t anuncio_ultimo_ms = 0;
static uint32_t reanuncios = 0;

static void MantemAnuncio()
{
    if((uint32_t)(millis() - anuncio_ultimo_ms) < 2000u) return;
    anuncio_ultimo_ms = millis();
    NimBLEServer* srv = NimBLEDevice::getServer();
    if(!srv) return;
    if(srv->getConnectedCount() >= CONFIG_BT_NIMBLE_MAX_CONNECTIONS) return;
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    if(adv->isAdvertising()) return;
    adv->start();
    reanuncios++;
}

extern volatile bool conectado;   // declarado com os contadores, mais abaixo

// Comandos da PONTE (chegam com `!`). Poucos de proposito: isto nao e uma
// segunda consola, e a porta de servico do radio.
static char cmd_pendente[96] = {0};
static volatile bool cmd_ha = false;

static void ComandoDaPonte(const char* c)
{
    char m[200];
    if(!strncmp(c, "board", 5))
    {
        NimBLEServer* sv = NimBLEDevice::getServer();
        snprintf(m, sizeof(m),
                 "ponte v%s · %s · ligacoes %u/%d · consola %lu->/%lu<- B\n"
                 "  particao %s · %s\n",
                 PONTE_VERSAO, conectado ? "com link" : "sem link",
                 sv ? (unsigned)sv->getConnectedCount() : 0,
                 CONFIG_BT_NIMBLE_MAX_CONNECTIONS,
                 (unsigned long)nus_para_daisy, (unsigned long)nus_do_daisy,
                 esp_ota_get_running_partition()->label,
                 confirmado ? "CONFIRMADA" : "por confirmar (volta atras se cair)");
        DizAoMac(m);
        return;
    }
    if(!strncmp(c, "ota ", 4))
    {
        uint32_t n = 0; const char* p = c + 4;
        while(*p >= '0' && *p <= '9') n = n * 10 + (uint32_t)(*p++ - '0');
        while(*p == ' ') p++;
        if(n < 65536u || n > 1310720u)
        { DizAoMac("ota: tamanho fora de faixa (64 KB a 1,25 MB)\n"); return; }
        if(!Update.begin(n))
        {
            snprintf(m, sizeof(m), "ota: nao comecou — %s\n",
                     Update.errorString());
            DizAoMac(m); return;
        }
        if(*p) Update.setMD5(p);      // sem MD5 ainda vai, mas diz-se
        ota_ativo = true; ota_total = n; ota_recebido = 0;
        ota_ultimo_ms = millis(); ota_tentativas++; ota_erro[0] = 0;
        snprintf(m, sizeof(m), "ota: a receber %lu B%s\n",
                 (unsigned long)n, *p ? " (MD5 declarado)" : " SEM MD5");
        DizAoMac(m);
        return;
    }
    DizAoMac("ponte: `!board` ou `!ota <bytes> <md5>`\n");
}

class NusRxCB : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic* c) override
    {
        const std::string v = c->getValue();
        nus_escritas++;
        nus_bytes_crus += v.size();

        // ⚠️ EM OTA NAO SE OLHA PARA O CONTEUDO. Uma imagem tem bytes iguais
        // a '!' e a '\n' por todo o lado; interpretar durante a transferencia
        // seria o mesmo defeito do parser que confunde dado com comando.
        if(!ota_ativo)
        {
            // LINHA PARA A PONTE? So se comecar por '!' — tudo o resto e da
            // Daisy, e a ponte continua a nao saber o que significa.
            if(v.size() && v[0] == '!')
            {
                static char cmd[96];
                size_t k = 0;
                for(size_t i = 1; i < v.size() && k < sizeof(cmd) - 1; i++)
                {
                    if(v[i] == '\n' || v[i] == '\r') break;
                    cmd[k++] = v[i];
                }
                cmd[k] = 0;
                // 🔴 SO ENFILEIRA. Chamar o `ComandoDaPonte` daqui foi a
                // violacao da regra que este ficheiro ja tinha escrita tres
                // blocos acima — e o `Update.begin()` APAGA FLASH, que e uma
                // operacao longa. Feita na tarefa do BLE, a pilha perde o
                // tempo dela e DERRUBA A LIGACAO: medido a 29/09, o `!ota`
                // matava a sessao antes de enviar um byte de imagem.
                //
                // A regra vale para tudo o que demora, nao so para a Serial1.
                strncpy(cmd_pendente, cmd, sizeof(cmd_pendente) - 1);
                cmd_pendente[sizeof(cmd_pendente) - 1] = 0;
                cmd_ha = true;
                return;
            }
        }

        for(size_t i = 0; i < v.size(); i++)
        {
            const size_t prox = (nus_cab + 1) % NUS_FILA;
            if(prox == nus_cau)
            {
                nus_descartes++;
                // Em OTA, um byte perdido corrompe a imagem: aborta em vez
                // de somar e seguir. Ver a nota do escoamento.
                if(ota_ativo) OtaAborta("fila cheia a meio da imagem");
                return;
            }
            nus_fila[nus_cab] = (uint8_t)v[i];
            nus_cab = prox;
        }
    }
};

// 8->7: `n` bytes crus viram ceil(n/7)*8 bytes, todos < 0x80.
static size_t Empacota7(const uint8_t* cru, size_t n, uint8_t* fora, size_t cap)
{
    size_t o = 0;
    for(size_t i = 0; i < n; i += 7)
    {
        const size_t bloco = (n - i) < 7 ? (n - i) : 7;
        if(o + 1 + bloco > cap) break;
        uint8_t altos = 0;
        for(size_t k = 0; k < bloco; k++)
            if(cru[i + k] & 0x80) altos |= (uint8_t)(1u << k);
        fora[o++] = altos;
        for(size_t k = 0; k < bloco; k++) fora[o++] = cru[i + k] & 0x7F;
    }
    return o;
}

static size_t Desempacota7(const uint8_t* emp, size_t n, uint8_t* fora, size_t cap)
{
    size_t o = 0, i = 0;
    while(i < n)
    {
        const uint8_t altos = emp[i++];
        for(size_t k = 0; k < 7 && i < n && o < cap; k++, i++)
            fora[o++] = (uint8_t)(emp[i] | ((altos >> k) & 1u ? 0x80 : 0));
    }
    return o;
}

// Escreve um quadro de consola na UART. SO DO LACO — ver a nota da fila.
// Responde ao Mac pela consola. Usa-se para o que e da PONTE — o que vem da
// Daisy ja passa pelo `DaSerial`.
static void DizAoMac(const char* txt)
{
    if(!nus_tx) return;
    nus_tx->setValue((uint8_t*)txt, strlen(txt));
    nus_tx->notify();
}

static void MandaSysexAoDaisy(const uint8_t* cru, size_t n)
{
    uint8_t emp[256];
    const size_t m = Empacota7(cru, n, emp, sizeof(emp));
    Serial1.write(0xF0); Serial1.write(0x7D); Serial1.write(0x02);
    Serial1.write(emp, m);
    Serial1.write(0xF7);
    nus_para_daisy += n;
}

// ---- contadores: o que a ponte sabe sobre si propria ------------------------
volatile bool            conectado      = false;
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

static void DaSerial(const midi::Message<128>& m)
{
    // ⚠️ A CONSOLA NAO E MIDI, E NAO PODE SAIR COMO MIDI. Um quadro nosso
    // reemitido pelo `Encaminha` viraria uma mensagem de tres bytes sem
    // sentido no Loopy — ruido num canal que o dB usa a tocar.
    if(m.type == midi::SystemExclusive)
    {
        const unsigned n = m.getSysExSize();
        if(n >= 5 && m.sysexArray[1] == 0x7D && m.sysexArray[2] == 0x01)
        {
            uint8_t cru[192];
            const size_t k = Desempacota7(&m.sysexArray[3], n - 4,
                                          cru, sizeof(cru));
            // ⚠️ SEM O `conectado`, e a razao e da v1.3.0. Aquele booleano
            // e do BLEMIDI e diz "ha ALGUEM ligado" — com tres ligacoes
            // possiveis, deixou de significar o que significava: uma queda
            // do Loopy punha-o a falso e CALAVA a consola de quem continuava
            // ligado. Variavel com dois empregos, a familia C do audit.
            //
            // O `notify()` do NimBLE ja faz a coisa certa sozinho: entrega a
            // quem subscreveu, e nao faz nada se ninguem subscreveu. A
            // guarda era a mentir sobre o estado de outra pessoa.
            if(nus_tx)
            {
                nus_tx->setValue(cru, k);
                nus_tx->notify();
            }
            nus_do_daisy += k;
            return;                       // NOSSO: nao vai para o MIDI
        }
    }
    Encaminha(m, false);
}

void setup()
{
    Serial.begin(115200);            // USB CDC — so para o humano e o `board`
    AplicaStatus(false);             // ANTES de tudo: nasce dizendo "sem link"
    pinMode(PINO_LED, OUTPUT);
    digitalWrite(PINO_LED, HIGH);    // ativo em baixo: HIGH = apagado

    Serial1.setTxBufferSize(TX_BUFFER);   // tem de vir ANTES do begin
    Serial1.begin(BAUD, SERIAL_8N1, PINO_RX, PINO_TX);

    // ---- NUS ANTES do RADIO.begin(), e a ORDEM E O CONSERTO ----
    //
    // 🔴 MEDIDO a 29/09, depois de duas tentativas erradas. O `begin()` do
    // BLEMIDI faz, por esta ordem: init do dispositivo, cria o servidor, cria
    // o servico MIDI, `service->start()`, e **arranca o anuncio**. E no
    // NimBLE e o arranque do ANUNCIO que constroi a tabela GATT.
    //
    // Criar o nosso servico DEPOIS disso tem dois sintomas, nesta ordem:
    //   1a tentativa: o anuncio PARA — o Mac varre 10 s, ve 21 aparelhos, e
    //      nao ve este. Radio que deixou de anunciar tem a mesma cara de
    //      radio fora de alcance.
    //   2a tentativa (com o anuncio reiniciado a mao): anuncia e liga, mas a
    //      caracteristica de notificacao NAO EXISTE — o servico entrou na
    //      lista e as caracteristicas ficaram fora da tabela.
    //
    // Nenhum dos dois deu erro. A biblioteca aceitou as duas ordens erradas
    // em silencio, e so o cliente do outro lado soube dizer que faltava algo.
    //
    // Entao inicializa-se o dispositivo aqui, cria-se o NUS, e deixa-se o
    // `RADIO.begin()` fazer o resto — o `init` e o `createServer` dele sao
    // idempotentes, e o anuncio dele constroi a tabela com os DOIS servicos.
    NimBLEDevice::init("Cotucatronics");
    {
        NimBLEServer* srv = NimBLEDevice::createServer();
        NimBLEService* sv = srv->createService(NUS_SVC);
        NimBLECharacteristic* rx = sv->createCharacteristic(
            NUS_RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
        rx->setCallbacks(new NusRxCB());
        nus_tx = sv->createCharacteristic(NUS_TX, NIMBLE_PROPERTY::NOTIFY);
        sv->start();
        // (!) de proposito FORA do anuncio — ver a nota do bloco NUS.
    }

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
    arranque_ms = millis();   // o relogio da confirmacao comeca aqui
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
            if(n && strncmp(buf, "eco ", 4) == 0)
            {
                // INJECTA na fila do NUS como se tivesse vindo do radio.
                // Parte o caminho em dois: com isto prova-se ponte->Daisy
                // sem depender do BLE, e o que sobra e so o troco do radio.
                const char* t = buf + 4;
                for(const char* q = t; *q; q++)
                {
                    const size_t prox = (nus_cab + 1) % NUS_FILA;
                    if(prox == nus_cau) { nus_descartes++; break; }
                    nus_fila[nus_cab] = (uint8_t)*q;
                    nus_cab = prox;
                }
                const size_t prox = (nus_cab + 1) % NUS_FILA;
                if(prox != nus_cau) { nus_fila[nus_cab] = '\n'; nus_cab = prox; }
                Serial.printf("eco: %u byte(s) na fila — o laco escoa ja\n",
                              (unsigned)strlen(t) + 1);
            }
            else if(n && strncmp(buf, "board", 5) == 0)
            {
                Serial.printf("ponte v%s · BAUD %lu · %s\n", PONTE_VERSAO,
                              (unsigned long)BAUD,
                              conectado ? "CONECTADO" : "sem link");
                {
                    NimBLEServer* sv = NimBLEDevice::getServer();
                    Serial.printf("  ligacoes: %u de %d ocupada(s) ·"
                                  " %lu reanuncio(s)\n",
                                  sv ? (unsigned)sv->getConnectedCount() : 0,
                                  CONFIG_BT_NIMBLE_MAX_CONNECTIONS,
                                  (unsigned long)reanuncios);
                }
                Serial.printf("  radio: %s\n",
                              NimBLEDevice::getAdvertising()->isAdvertising()
                                  ? "a ANUNCIAR (descobrivel)"
                                  : "NAO anuncia — ou ha link, ou parou");
                Serial.printf("  NUS: %lu escrita(s) do Mac, %lu B crus"
                              " · fila %s\n",
                              (unsigned long)nus_escritas,
                              (unsigned long)nus_bytes_crus,
                              nus_cau == nus_cab ? "vazia" : "COM COISA PARADA");
                Serial.printf("  consola: %lu B do Mac -> Daisy · %lu B da Daisy"
                              " -> Mac · %lu descartados (fila cheia)\n",
                              (unsigned long)nus_para_daisy,
                              (unsigned long)nus_do_daisy,
                              (unsigned long)nus_descartes);
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
    if(cmd_ha) { cmd_ha = false; ComandoDaPonte(cmd_pendente); }
    MantemAnuncio();   // ha vaga? entao anuncia — ver a nota do bloco

    // ---- CONFIRMA A IMAGEM, E SO DEPOIS DE ELA SE PROVAR ----
    //
    // ⚠️ Confirmar cedo e o mesmo que nao ter rede. Uma imagem que arranca e
    // morre a seguir seria dada por boa, e o bootloader nunca mais voltaria
    // atras. Entao exigem-se as DUAS coisas que faltariam numa imagem ma:
    //   · o radio a ANUNCIAR (se o BLE nao subir, a caixa fica inalcancavel);
    //   · 15 s de laco (apanha crash-loop, que e o outro modo de morte).
    // Nao confirmada, a proxima reinicializacao volta a imagem anterior.
    if(!confirmado && arranque_ms
       && (uint32_t)(millis() - arranque_ms) > 15000u
       && NimBLEDevice::getAdvertising()->isAdvertising())
    {
        esp_ota_img_states_t st;
        const esp_partition_t* par = esp_ota_get_running_partition();
        if(esp_ota_get_state_partition(par, &st) == ESP_OK
           && st == ESP_OTA_IMG_PENDING_VERIFY)
        {
            esp_ota_mark_app_valid_cancel_rollback();
            Serial.printf("ota: imagem CONFIRMADA (%s) — 15 s de pe com o"
                          " radio a anunciar\n", par->label);
        }
        confirmado = true;
    }

    // ESCOA A CONSOLA. So daqui: o laco e o unico dono da Serial1, e escrever
    // da tarefa do BLE partiria uma mensagem MIDI ao meio.
    //
    // Junta o que houver numa passagem, ate 105 bytes — o tecto vem do
    // `Message<128>` da biblioteca MIDI do outro lado: 128 menos F0/7D/tipo/F7
    // sao 124 empacotados, que valem 108 crus. 105 deixa folga e e redondo.
    if(nus_cau != nus_cab)
    {
        uint8_t lote[105];
        size_t n = 0;
        while(nus_cau != nus_cab && n < sizeof(lote))
        {
            lote[n++] = nus_fila[nus_cau];
            nus_cau = (nus_cau + 1) % NUS_FILA;
        }
        if(ota_ativo)
        {
            // ⚠️ EM OTA, PERDER UM BYTE E FATAL. No modo consola um byte
            // perdido e uma letra feia; aqui e uma imagem corrompida. Por
            // isso o `nus_descartes` do callback ABORTA a gravacao em vez de
            // somar e seguir — e por isso o MD5 confere no fim, que apanha o
            // que a contagem nao apanhar.
            if(Update.write(lote, n) != n)
                OtaAborta("escrita na flash falhou");
            else
            {
                ota_recebido += n;
                ota_ultimo_ms = millis();
                if(ota_recebido >= ota_total)
                {
                    if(Update.end(true))
                    {
                        DizAoMac("ota: imagem aceite e MD5 confere."
                                 " A reiniciar — o bootloader volta atras"
                                 " sozinho se ela nao confirmar.\n");
                        delay(200);
                        ESP.restart();
                    }
                    else
                    {
                        char m[80];
                        snprintf(m, sizeof(m), "ota: RECUSADA — %s\n",
                                 Update.errorString());
                        DizAoMac(m);
                        OtaAborta("fim recusado");
                    }
                }
            }
        }
        else
        {
            MandaSysexAoDaisy(lote, n);
        }
    }
    // Transferencia que emudece a meio deixa a flash meio escrita; um prazo
    // fecha isso em vez de a deixar pendurada ate ao proximo reinicio.
    if(ota_ativo && (uint32_t)(millis() - ota_ultimo_ms) > 10000u)
    {
        OtaAborta("10 s sem bytes");
        DizAoMac("ota: ABORTADA — 10 s sem bytes. A imagem anterior fica.\n");
    }
    // ESCRITA DIFERIDA. A queda so marcou; e aqui que a flash e tocada, e
    // no maximo a cada 10 s. Isto tambem escoa naturalmente para o momento
    // certo: depois de uma queda nao ha trafego BLE a atrasar.
    if(nvs_sujo && (uint32_t)(millis() - nvs_ultimo_ms) >= NVS_MIN_MS)
        NvsGrava();
}
