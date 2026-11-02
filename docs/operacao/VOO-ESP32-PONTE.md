# VOO — ponte BLE↔UART MIDI no ESP32-C3

> **v0.1.0** · 2026-08-25 · substitui o WIDI Core. A decisão e o preço estão em
> [GEAR-AUDIT.md](GEAR-AUDIT.md); aqui está o que se constrói e como se aceita.

**A propriedade que define o voo 1: o firmware da Daisy não muda uma linha.**
Os dois módulos falam MIDI serial 31250 8N1 pelos mesmos D13/D14.

---

## 0 · São DOIS voos, e a separação é o que preserva a regra

A discussão de arquitetura anexou ao voo 1 coisas que **mudam o firmware da
Daisy** — em particular o `sync` automático na subida do D28. Isso é o voo 2.
Chamar-lhe um só quebraria a propriedade acima, e a disciplina de uma
variável por voo é o que já pagou por si três vezes neste projeto.

| | voo 1 | voo 2 |
|---|---|---|
| o quê | a ponte | `sync` automático na reconexão |
| Daisy | **intocada** | muda |
| depende de | nada | do voo 1 aceite |

---

## 1 · As três verificações, feitas antes de escrever

Custaram minutos e mudaram a forma do voo. Ficam registradas porque
**verificação não declarada será refeita, ou pior, presumida.**

### D28 é lido como nível ou como borda?

`LbTick` ([main.cpp:2460](../../src/main.cpp:2460)) amostra a 200 Hz e
calcula **transições por janela de 2 s** e **duty**. Não existe evento de
borda instantânea.

```cpp
static constexpr bool kLbAcesoEmBaixo = true; // confirmado: V3=3V3, LB=0V
const bool aceso = kLbAcesoEmBaixo ? !nivel : nivel;
```

```cpp
widi_led.Init(seed::D28, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
```

**Duas correções ao desenho original, e ambas invertem o que estava escrito
— ver a secção 3.** E confirma a separação em dois voos: detectar subida é
código novo na Daisy.

### A janela de autoria assume emissor humano?

**Não.** O `MidiPushEstado` carimba a janela e **re-carimba todos os params
depois do último envio** ([main.cpp:616](../../src/main.cpp:616)), com a
conta escrita no próprio comentário: *"o despejo leva ~258 ms (86 × 3 ms) e
a janela dura 250, então a do PRIMEIRO param expira antes de o ÚLTIMO sair."*

Autoria declarada já existe. O segundo emissor em rajada da fila — o `load`
de preset — também já está tratado: **não usa esta rota**, e escoa pelo
espelho normal.

**Fica um por verificar: o randomizador dos búzios alargado.** Entra como
requisito do desenho dele, não como dívida deste voo.

### `eco_ult` guarda emissão ou recepção?

Guarda **valor**, atualizado por qualquer origem
([main.cpp:239](../../src/main.cpp:239)):

```cpp
// eco_ult[cc] guarda o ultimo valor visto naquele CC, VENHA DE ONDE VIER
if(eco_ult[id] == v) continue;
```

Logo: uma mensagem perdida no fio deixa `eco_ult` a dizer "mandei", o param
não muda, e **nada diverge do ponto de vista da placa**. Reenvio por
divergência nunca agiria — seria defesa com nome de defesa. Por isso o voo 2
usa **repetição espalhada**, não autocorreção.

---

## 2 · Hardware — cinco fios

| ESP32-C3 SuperMini | Daisy | nota |
|---|---|---|
| **5V** | rail 5V | aceita 4,3–6,0 V |
| **GND** | GND | |
| GPIO (TX) | **D14** (RX) | ★ CRUZADO |
| GPIO (RX) | **D13** (TX) | ★ CRUZADO |
| GPIO (status) | **D28** | **ativo em BAIXO** — ver secção 3 |

**DADOS CRUZADOS.** Ligar reto não queima: dá silêncio, idêntico a módulo
desligado. Se `midi` der 0 bytes, desconfiar do cruzamento **antes de tudo**.
A cicatriz nasceu com o WIDI e aplica-se inteira aqui.

**Lógica 3,3 V dos dois lados, sem conversor.** O C3 **não é 5 V tolerante**;
a Daisy fala 3,3 V.

**Escolha dos GPIO — o oposto da Daisy.** A GPIO Matrix põe a UART em quase
qualquer pino, então escolhem-se os que caem mais perto da fiação existente.
**Evitar:** GPIO20/21 (console USB — necessário para gravar e depurar),
GPIO2/8/9 (strapping), GPIO18/19 (USB).

**Não há jumper de ON.** O ESP32 liga ao ser alimentado — um gesto de
montagem a menos e um modo de falha a menos, vezes centenas de unidades.

**Bancada:** alimenta por USB **ou** pelo pino de 5V, nunca os dois.

---

## 3 · D28 é ATIVO EM BAIXO, e não leva resistor externo

> **O ESP32 puxa o pino a LOW quando conectado, e deixa em aberto (ou alto)
> quando não.** Se inverter, o firmware lê o oposto do real — plausivelmente,
> sem erro nenhum. É a mensagem mal-endereçada num fio só.

A especificação original dizia *"alto = conectado"* e pedia um **pull-down de
10 kΩ**. Os dois estão errados. O pull-down foi **proposta do Code**, aceita
na especificação sem que nenhum dos dois lesse o código — e o registro fica
porque o raciocínio que o produziu era bom:

O raciocínio era *"não acionado tem de ler como sem-link — a direção segura
da falha"*, e está certo. A implementação era o oposto, porque **supôs-se
lógica ativa em alto sem ler a fonte**. Com LB ativo em baixo e pull-up
interno já presente, a direção segura **já existe por construção**: não
acionado → alto → `aceso = false` → sem link.

**O pull-down proposto teria invertido exatamente isso**, fazendo o pino
flutuante ler "conectado" durante o boot do ESP32 — o modo de falha que ele
existia para evitar, produzido pela defesa contra ele.

> **Antes de propor uma defesa, ler o default na fonte.** É a mesma regra que
> evitou o problema do MISO no OLED, e não foi aplicada aqui. A decisão pode
> não existir — e quando já existe e está certa, acrescentar uma segunda
> inverte-a.

**Consequência prática: nenhum componente novo.** O pull-up interno da Daisy
já faz o trabalho.

### O pino tem dois consumidores, e eles querem moedas diferentes

A máquina quer **nível**, para decidir. O humano quer **cadência**, para
entender. Servir os dois no mesmo fio é a família C — variável com dois
empregos — e o conserto não é escolher um, é **separar**:

- **D28 = nível puro**, idêntico ao LB. Nada de piscar.
- **A cadência vai num LED próprio do ESP32**, num pino dele.

Duas moedas, dois fios, zero conflito. E o LED expressivo **não custa um voo**
— nasce inteiro do lado do ESP32, sem tocar na Daisy.

---

## 4 · A ponte, e o que ela se recusa a fazer

Biblioteca: **Arduino-BLE-MIDI (lathoub)** com **NimBLE** (gasta bem menos
RAM/flash que a pilha padrão).

O laço é literalmente: BLE MIDI in → Serial2 (31250) out, e o inverso.

> **Sem mapa de CC, sem params, sem interpretação. A ponte passa bytes.**

É o que impede o mapa de ganhar uma **segunda casa** — que seria o defeito
mais caro que este projeto podia inventar agora. E é o que torna a fronteira
fina o bastante para que a deriva entre os dois firmwares seja impossível por
construção, e não por disciplina.

**O preço dessa finura, assumido:** a fronteira fina **não se consegue
verificar a si própria**. Um aperto de mão de versão exigiria que o ESP32
falasse por conta própria no fluxo — e aí deixa de passar bytes e passa a ser
autora, com janela de autoria, mensagem sem gesto humano, e o mapa a ganhar a
casa que o desenho existe para impedir.

> **Enquanto a ponte só passar bytes, não há aperto de mão; se ela engordar
> por outro motivo, o argumento cai e o handshake volta com o custo já pago.**

A condição de reabertura mora na mesma frase de propósito: quem chegar daqui
a três meses lê o parágrafo da proibição e **para ali**. Exceção em parágrafo
vizinho é exceção que não existe para quem só encontra metade.

A defesa da convenção é **procedimental e visível**: baud declarado nos dois
READMEs e impresso pelo `board` de cada lado. MAJOR nos dois se mudar.

---

## 5 · Buffer de TX — decisão explícita, não default

**O BLE entrega mais rápido do que 31250 baud escoa.** O gargalo é o buffer,
não o rádio.

A maior rajada do sistema é conhecida: o `sync`, **86 CCs = 258 bytes ≈ 82 ms
de UART**. O buffer dimensiona-se por esse número.

**Transbordo é CONTADO, nunca dropado calado.** Default herdado de biblioteca
é a mesma classe de premissa não examinada que o audit inteiro ataca.

---

## 6 · Contador de quedas — com recência

Contagem acumulada responde *"quantas vezes desde o boot"*. A pergunta real,
numa galeria, é ***"está a cair agora?"*** — e **dado sem o quando não é dado**.

Implementa-se com janela deslizante ou instante da última queda.

E traz uma coisa de brinde: **distingue montagem de operação sem ninguém
marcar nada.** Durante a montagem as quedas são densas e depois cessam; numa
galeria com interferência, reaparecem espaçadas. A mesma leitura separa
*"estava a ser montado"* de *"está com problema"* — sem estado, sem modo, só
o quando.

---

## 7 · Antes do portão: o perfil do Loopy tem de ser regerado

**Os 86 bindings estão presos à identidade do dispositivo.** O ficheiro
exportado chama-se `227f559e.MIDI.Cotucatronics1 Bluetooth.controllerprofile`
— e esse `227f559e` é o identificador que o Loopy atribuiu ao WIDI.

Um dispositivo BLE novo é, para o Loopy, **outro dispositivo**: os 86
bindings não se aplicam e os widgets deixam de responder.

⚠️ **O sintoma disso é indistinguível de a ponte não funcionar.** Emparelha,
conecta, mexe num slider, não acontece nada — e o veredito natural é *"o
ESP32 não funciona"*. **O portão reprovaria a coisa certa pelo motivo
errado**, e sem plano B isso custa caro.

É um defeito que não vive dentro de nenhuma peça: o ESP32 está bom, o Loopy
está bom, o perfil está bom. **A relação entre o perfil e a identidade do
dispositivo é que se partiu**, e ninguém a tem em ficheiro.

**Já está resolvido, e o procedimento é curto** — o formato foi decifrado em
2026-08-24 e `Device`/`Device Name` são dois campos de texto no plist:

1. ligar o ESP32 e deixar o Loopy vê-lo;
2. exportar um perfil qualquer para descobrir o identificador novo;
3. regerar com [gerar_perfil_loopy.py](gerar_perfil_loopy.py) para esse
   identificador;
4. importar.

**Tem de acontecer ANTES do portão, senão o portão mede outra coisa.**

---

## 8 · O PORTÃO — e os dois tipos de critério não se misturam

Sem plano B, o aceite **trava a montagem e autoriza a encomenda**. Corre em
protoboard, antes de soldar e antes de comprar em quantidade.

### Julgamento — do ouvido do dB, e vinculativo

- [ ] **Latência no gesto real.** O dB gira um knob e julga. BLE MIDI fica em
      3–15 ms conforme o intervalo que o iOS negocia; para controle remoto é
      tolerável, mas **quem decide é o ouvido**, não o número.

> O julgamento é **feito uma vez e vinculativo**. Aprovado, não se reabre
> depois com números; reprovado, trava. O que mata um portão não é ter
> critério subjetivo — é ter critério subjetivo que se pode revisitar até dar
> o resultado desejado.

### Medição — não depende de ninguém estar a olhar

- [ ] **Vinte minutos de operação com zero quedas na janela.** O contador
      nasceu como proteção da serial, virou diagnóstico de galeria, e paga o
      critério de aceite que ele mesmo tornou possível.
- [ ] **Pareamento** com o Loopy tão simples quanto o do WIDI: aparece nas
      configurações de MIDI Bluetooth, conecta, permanece.
- [ ] **Rajada:** `sync` empurrando 86 CCs de uma vez, sem perda. O `midi`
      conta bytes/msgs/aplicadas e o `vistos` nomeia o que chegou por canal.
- [ ] **Os seis canais atravessam intactos** — ch1/2/3 entrada, ch4/5/6
      espelho. A ponte é transparente, mas confirma-se.
- [ ] **D28 acompanha o estado real**, com a polaridade da secção 3:
      desconecta o iPad e o nível muda no sentido certo.
- [ ] **Recuperação:** tirar a alimentação do ESP32 durante um gesto. Volta
      sozinho? Em quanto tempo? A Daisy percebe?

Sobre a recuperação: **o estado da Daisy sobrevive** — a ponte não guarda
nada. O dano é a visão do Loopy ficar velha, e o `sync` é o conserto. Falta
quem o dispare, e isso é o voo 2.

---

## 9 · O que NÃO fazer neste voo

Nada de OSC, editor web, presets na flash do ESP32, ESP-NOW, nem mexer no
baud. Estão todos disponíveis e nenhum bloqueia nada — entram quando virarem
necessidade real, com requisito claro.

**Este voo prova a ponte e para.**

---

## 10 · Onde mora

`esp32/` no mesmo repo, versionado junto. **Segundo firmware = segunda
versão**, e o `board` de cada lado declara a sua.

Enquanto for só ponte, a doc é fina: um README com fiação, pinos escolhidos e
como gravar. Se um dia crescer para OSC ou editor web, a fronteira engorda e
exige o mesmo rigor do principal.

---

## 11 · VOO 2 — esboço, para não se perder

Depende do voo 1 aceite. **Muda a Daisy.**

**`sync` automático na subida do D28**, com as varreduras **espalhadas**
(`t=0`, `~300 ms`, `~1 s`) e não encostadas: o modo de falha é **sistemático**
(porta MIDI ainda fechada), não aleatório — três varreduras em 250 ms
encostadas caem inteiras dentro de uma janela morta de 500 ms.

> Espalhamento é **cobertura** (erra graduado); assentamento fixo é **limiar**
> (erra binário e perde tudo).

**Não chamar `MidiPushEstado`.** Ele faz `hal::DelayMs(3)` por mensagem e
**bloqueia o laço por 258 ms**. Com um humano a disparar, ele sabe que
disparou; automático e espalhado seriam **três paragens de 258 ms sem
explicação** após cada reconexão — encoders parados, botões parados, e o
próprio `LbTick` parado, ou seja, **a detecção do link cega justamente
enquanto responde ao link**.

O conserto já existe no código: **a rota do `load`** invalida
`eco_ult[id] = 0xFF` ([main.cpp:346](../../src/main.cpp:346)) e deixa o
espelho escoar sozinho — 2 mensagens por 8 ms, sem bloquear, com a janela
carimbada no instante de cada envio, que é o carimbo correto para escoamento
escalonado. O espalhamento vira **três invalidações instantâneas**.

**Espalhamento em curso absorve borda nova REINICIANDO**, não ignorando: se
houve queda, as varreduras anteriores foram para um link morto e não valem
nada. Mesma lógica do nível seguro — assumir que não chegou.

**A conferir quando o voo 2 chegar:** com escoamento escalonado (~344 ms para
os 86) e invalidações espaçadas em ~1 s, há sobreposição. Invalidar é
idempotente — o param que ainda não escoou escoa uma vez — mas **confirmar
que o espelho lida com invalidação durante escoamento** sem contar duas vezes
nem perder um.

**O `sync` reporta o que fez** — quantas varreduras, quando, disparado por
borda ou por comando, e se foi reiniciado. Sem isso a contagem não fecha com
nenhum espalhamento possível, e quem ler daqui a um mês encontra um número
órfão.

> O critério é **reparável E anunciada**. Tela velha é recuperável mas **não
> se anuncia**: widget errado é indistinguível de widget certo, e só quem
> souber o valor esperado naquele instante percebe. Reparação que depende de
> alguém reparar num defeito que não se mostra é reparação teórica.

**A declarar no `HARDWARE.md`:** se o link cair e alguém mexer num widget, o
`sync` empurra o estado da placa de volta e o widget salta. **Não é o
instrumento a desfazer o gesto — é revelar que o gesto nunca chegou.** O
widget mudou de posição; o param não. Não declarado, vira "bug intermitente
do Bluetooth" numa sessão futura.
