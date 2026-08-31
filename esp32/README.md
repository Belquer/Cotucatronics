# esp32/ — a ponte BLE MIDI

> **`ponte` v0.5.0** · ESP32-C3 SuperMini · substitui o WIDI Core 1.3.
> A decisão está em [GEAR-AUDIT.md](../docs/operacao/GEAR-AUDIT.md) e o voo em
> [VOO-ESP32-PONTE.md](../docs/operacao/VOO-ESP32-PONTE.md).

**Esta placa é um fio com rádio.** Não conhece CC nenhum, param nenhum, canal
nenhum. Se um dia precisar de saber o que uma mensagem *significa*, a fronteira
engordou e a decisão inteira volta à mesa.

---

## Fiação — cinco fios

| ESP32-C3 | Daisy | |
|---|---|---|
| `5V` | rail 5 V | aceita 4,3–6,0 V |
| `GND` | GND | |
| `GPIO4` (TX) | **D14** (RX) | ★ CRUZADO |
| `GPIO5` (RX) | **D13** (TX) | ★ CRUZADO |
| `GPIO6` (status) | **D28** | **ativo em BAIXO** |
| `GPIO8` | — | LED de bordo, só desta placa |

**DADOS CRUZADOS.** Ligar reto não queima: dá silêncio, idêntico a módulo
desligado. Se o `midi` da Daisy der 0 bytes, desconfiar do cruzamento **antes
de tudo**. A cicatriz nasceu com o WIDI e aplica-se igual aqui.

🔴 **A DAISY NÃO ARRANCA SEM ESTA PONTE LIGADA.** O `D14` dela é o RX da
USART1 e, com a ponte fora, flutua com o DMA à escuta — o que prende a
transmissão e trava o laço principal. Medido em 31/08. Ver
[`HARDWARE.md`](../docs/operacao/HARDWARE.md) §4. **Defeito aberto do lado da
Daisy; até lá, a ponte é requisito de arranque.**

🔌 **SCHOTTKY EM SÉRIE NA ALIMENTAÇÃO DO TRILHO — SS14 ou 1N5819.**
Ânodo no trilho de 5 V, cátodo no pino `5V` desta placa.

Nesta placa o `5V` liga ao **VBUS do USB directamente**, sem díodo de bordo.
Sem o Schottky, o cabo USB **realimenta o trilho inteiro** — medido em 31/08:
o LED do amp acendia com o powerbank desligado. Com o díodo, podes alimentar
pelos dois sem consequência, e a regra abaixo deixa de ser necessária.

**Bancada, enquanto o díodo não estiver soldado: alimenta por USB *ou* pelo
pino de 5 V, nunca os dois.**

⚠️ **DESACOPLAMENTO OBRIGATÓRIO — 100 µF + 100 nF nos pinos `5V`/`GND`.**

📖 O porquê está em [`docs/operacao/ATERRAMENTO.md`](../docs/operacao/ATERRAMENTO.md) §4 e §7.

Medido em 31/08: sem eles, **o rádio sai pelo exciter**. As rajadas de
transmissão são picos de corrente de microssegundos, e sem reservatório local
eles viram ondulação no trilho partilhado, que o amp amplifica.

E o sintoma tem duas caras, o que o faz parecer dois defeitos:

| estado do BLE | o que se ouve | por quê |
|---|---|---|
| **conectado** | zumbido contínuo | rajadas no intervalo de ligação, rápidas |
| **desconectado** | clique a ~3 Hz | rajadas no intervalo de anúncio, ~100–300 ms |

**E terra própria até ao ponto de estrela**, não pendurada na terra do
step-down — um conversor comutado abana a própria terra, e quem se pendura
nela herda o abanão.

**Não pôr 2200 µF aqui.** Ela vive na saída do step-down, e banco grande junto
a conversor comutado é o caminho para oscilação — ver `HARDWARE.md`.

### Por que estes GPIO

A GPIO Matrix do C3 põe a UART em quase qualquer pino — o oposto da Daisy.
Evitados: `GPIO20/21` (console UART0, preciso para depurar), `GPIO2/8/9`
(strapping), `GPIO18/19` (USB nativo). O `GPIO8` é strapping mas só no instante
do boot; depois disso é o LED de bordo e usa-se sem drama.

### D28 é ativo em BAIXO, e não leva resistor

Medido no firmware da Daisy: `kLbAcesoEmBaixo = true` e o pino entra com
`Pull::PULLUP`. Portanto **conectado = puxar a LOW; sem link = SOLTAR** e
deixar o pull-up interno levar o pino a alto.

Aberto-dreno por construção: a ponte nunca conduz alto. Assim *"sem link"* lê
igual quer a placa esteja desligada, em boot, ou desconectada — e essa é a
direção segura da falha.

---

## Gravar

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc ponte
arduino-cli upload -p /dev/cu.usbmodemXXXX --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc ponte
```

`CDCOnBoot=cdc` **não é opcional**: sem ele o `Serial` sai pelo UART0 (GPIO20/21)
em vez do USB, e o `board` fica invisível.

### Ambiente que se sabe funcionar

| | versão |
|---|---|
| `arduino-cli` | homebrew |
| core `esp32:esp32` | **3.1.1** |
| `MIDI Library` | **5.0.2** |
| `BLE-MIDI` | **2.2.0** |
| `NimBLE-Arduino` | **1.4.3** |

⚠️ **NimBLE 2.x NÃO compila** com BLE-MIDI 2.2.0: o 2.x removeu
`NimBLESecurity` e mudou a assinatura do `onWrite`. Se o `lib install` trouxer
a 2.x, descer para a 1.4.3.

---

## `board` — o que a ponte sabe sobre si própria

```
ponte v0.1.0 · BAUD 31250 · sem link
  pinos: TX=4 RX=5 STATUS=6 (ativo em BAIXO) LED=8
  quedas: 0 · nenhuma desde o boot
  msgs: BLE->ser 0 · ser->BLE 0 · TX sem espaco 0
```

**As quedas trazem recência** — *"há quantos segundos"*, não só quantas. A
pergunta que se faz numa galeria é *"está a cair agora?"*, e contagem sem o
quando não é dado. De brinde, separa montagem de operação sem ninguém marcar
nada: na montagem as quedas são densas e depois cessam.

**E sobrevivem ao corte de energia, desde a v0.5.0.** Viviam só em RAM, e isso
tornava-as inúteis para a pergunta que importa numa galeria — *"caiu de
madrugada?"*. Um piscar de energia, que é **uma das causas de queda**, zerava
a testemunha da queda.

```
quedas: 0 · nenhuma desde o boot        <- esta sessão: "está a cair AGORA?"
desde sempre: 0 quedas em 4 arranques   <- NVS: "caiu enquanto eu não estava?"
```

`arranques` separa as duas leituras: vida alta **com** arranques altos é
energia instável, não rádio instável. `zera` apaga a memória de vida.

⚠️ **Não se escreve a cada queda**, por duas razões independentes: a NVS tem
endurance finita e a peça fica meses ligada; e o callback de desconexão corre
no contexto do stack BLE, onde escrever flash é trabalho longo no sítio
errado. A queda **marca**, e o laço escreve — no máximo a cada 10 s. O `board`
mostra `(por gravar)` enquanto há coisa pendente.

**`TX sem espaço` conta e não cala.** O BLE entrega mais depressa do que 31250
baud escoa, logo o gargalo é o buffer e não o rádio — e a maior rajada do
sistema é conhecida: o `sync` da Daisy manda 86 CCs = 258 bytes ≈ 82 ms de
UART. O buffer é de 1024, folga de 4×.

---

## O baud é convenção, e defende-se por procedimento

**31250 8N1.** A ponte só passa bytes, logo **não há aperto de mão de versão**
entre as duas placas — e não há de propósito: um handshake obrigaria esta placa
a falar por conta própria no fluxo, e aí deixa de ser fio e vira autora, com
janela de autoria e mensagem sem gesto humano.

O preço assumido é que a fronteira fina **não se consegue verificar a si
própria**. A defesa é procedimental e visível: o número vive neste README, no
README da Daisy, e sai no `board` de cada lado. **Mudar é MAJOR nos dois.**

> Enquanto a ponte só passar bytes, não há aperto de mão; se ela engordar por
> outro motivo, o argumento cai e o handshake volta com o custo já pago.

---

## Uma precisão sobre "passa bytes"

O BLE MIDI é **enquadrado**: cada pacote leva um header e timestamps
intercalados. Desenquadrar é obrigatório — não há como não o fazer. Então a
ponte trabalha ao nível da **mensagem**: recebe, e reemite igual do outro lado.

O que ela não faz é interpretar o **sentido**. Não há tabela, não há número de
CC escrito em lado nenhum deste código, e é essa a propriedade que interessa.
