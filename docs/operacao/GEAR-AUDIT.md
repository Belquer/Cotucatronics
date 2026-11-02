# GEAR-AUDIT — WIDI Core → ESP32-C3

> **v0.1.0** · 2026-08-25 · a decisão de substituir o módulo BLE MIDI, com o
> preço de cada lado. Quando esta decisão for revista, o motivo antigo fica
> — premissa revogada em silêncio é a família de bug mais cara deste projeto.

O voo que executa esta decisão está em
[VOO-ESP32-PONTE.md](VOO-ESP32-PONTE.md).

---

## 1 · O número, e por que ele decide

| | WIDI Core 1.3 | ESP32-C3 SuperMini |
|---|---|---|
| custo unitário | ~US$ 45 | ~US$ 4 |
| firmware | fechado | nosso |
| pino de status | semântica do fabricante | semântica nossa |
| jumper de ON | necessário | não existe |
| depuração | nenhuma | console USB |

**Não é 30% mais barato — é uma ordem de grandeza.** E o instrumento é
replicado em série, na ordem de centenas: quarenta e um dólares por unidade,
vezes centenas, é a diferença entre um projeto viável e um protótipo caro.

Num exemplar único a troca não se justificaria. É a **escala** que a torna
obrigatória, e é por isso que o requisito se escreve como escala e não como
preferência técnica.

---

## 2 · O que o WIDI entregava, e o que cobrava

Entregava uma coisa real: **funcionava sem que ninguém escrevesse uma linha.**
Emparelha, conecta, passa MIDI. Para chegar até aqui, isso valeu o preço.

Cobrava quatro:

- **Caixa fechada.** Quando falhava, falhava. Não havia o que ler, o que
  instrumentar, o que corrigir. O diagnóstico terminava em "o módulo não
  está a responder" e não havia passo seguinte.
- **O pino LB dizia o que o fabricante decidiu que dizia.** Ativo em baixo,
  sem documentação do que distingue *advertising* de *conectado* de *erro*.
  Nós medimos transições e duty para inferir — inferência sobre uma decisão
  de outra pessoa.
- **O jumper de ON.** Um gesto de montagem, um modo de falha ("está ligado
  mas dorme"), e uma linha de checklist, vezes centenas de unidades.
- **31250 baud como lei.** A serial tinha de ser MIDI porque a outra ponta
  era MIDI. Não havia decisão a tomar — e não havia como saber que era uma
  decisão, porque estava do lado de lá.

---

## 3 · O que passa a ser nosso — e é grátis

A linha que separa o que a migração dá de graça do que ela apenas
**disponibiliza** é uma só:

> **É grátis tudo o que vive só do lado do ESP32. É caro tudo o que atravessa.**

Do lado grátis, e já incorporado ao voo 1:

- **A cadência do LED de estado.** Sólido, piscando, ritmo de erro — escolha
  nossa, num LED do próprio ESP32, sem tocar na Daisy.
- **O dimensionamento do buffer de TX**, pela maior rajada conhecida do
  sistema, em vez de default herdado de biblioteca.
- **A contagem de quedas de link, com recência** — que responde *"está a
  cair agora?"* e não *"quantas vezes desde o boot"*.
- **O comportamento na reconexão**, incluindo o relatório do que foi feito.
- **O nome anunciado no Bluetooth.**

Nada disso o WIDI permitia, e nada disso custa uma linha da Daisy. **A
migração já cobra a liberdade que era para cobrar.**

---

## 4 · O que fica registrado e NÃO construído

**Os 31250 baud deixam de ser lei e viram decisão nossa.** Hoje são
imposição do WIDI; depois da migração as duas pontas são nossas, e o número
podia subir com ganho direto na rajada.

Não se muda agora, e o motivo tem preço nomeado: **mudar o baud arrasta a
Daisy**, e no momento em que arrasta cria-se uma matriz de compatibilidade
entre as versões dos dois firmwares — ESP32 antigo com Daisy nova fica mudo,
com sintoma **idêntico a fio trocado**. Numa peça replicada em centenas isso
é um custo permanente, não um inconveniente de bancada.

> Isto é a **bts 12g em hardware**: separar o que é LEI do que é DECISÃO
> nossa. O número era lei enquanto uma das pontas era de terceiro. Deixou de
> ser — e passar por lei é exatamente o que a cicatriz manda separar.

Também não entram neste voo: OSC, editor web, presets na flash do ESP32,
ESP-NOW. Todos disponíveis, nenhum bloqueia nada, todos entram quando
virarem necessidade real com requisito claro.

---

## 5 · Sem plano B — e o que isso significa de facto

**O WIDI sai de vez. Não fica em header, não fica como reserva.** Com isso o
aceite do voo 1 deixa de ser confirmação e passa a ser **portão**: falhar um
critério trava a montagem em vez de adiar.

Três consequências que a decisão arrasta:

**O plano B não desapareceu — mudou de natureza.** Sem BLE, o instrumento
não fica sem MIDI: fica **sem MIDI sem fio**. D13/D14 são UART, e um jack
TRS ou DIN de MIDI cabeado funciona sem rádio nenhum. Em palco, a diferença
entre *"não há instrumento"* e *"há instrumento com cabo"* é toda.

**Falhar deixa de significar impasse.** O WIDI era caixa fechada: quando
falhava, não havia o que depurar. A ponte é código nosso, com biblioteca
pública e console USB. Falhar passa a significar *"há um bug e ele tem
endereço"* — e **eliminar a caixa preta é o que torna tolerável não ter para
onde recuar**.

**O portão autoriza a compra.** Numa série de centenas, o que o aceite trava
não é a bancada: é a encomenda. Portão em protoboard, **antes** de soldar
seja o que for e antes de comprar em quantidade.

---

## 6 · O que sai dos documentos, e o que nunca sai

Quando o portão passar, o WIDI sai do `HARDWARE.md`, do `PINOS.md`, do
README e do esquemático — tudo o que descreve o **estado atual**.

**Não sai do `battle_scars.md`.** As cicatrizes não documentam o presente:
registram o que custou chegar aqui, e apagá-las apagaria a lição junto com a
peça.

E uma delas nem é histórica: **o cruzamento TX/RX continua vivo.** Nasceu
com o WIDI e aplica-se inteira ao substituto — o ESP32 também liga TX no D14
e RX no D13. Apagá-la por associação seria perder a lição no exato momento
em que ela volta a ser necessária.
