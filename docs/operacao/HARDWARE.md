# HARDWARE — Cotucatronics

> Decisões fechadas, com o **porquê** de cada uma. Esquemáticos em
> `esquematicos/`. Quando uma decisão for revista, o motivo antigo fica
> registrado — premissa revogada em silêncio é a família de bug mais cara
> deste projeto.

---

## 1 · ENERGIA — entrada única, sem boost

```
qualquer powerbank PD ──USB-C──> [decoy PD 15V, 5A] ──> RAIL 15V
                                       ┌──> TPA3116D2 estéreo  (+2200µF +100nF)
                                       └──> step-down 5V       (SEM banco)
                                                  └──> Daisy VIN + ponte ESP32
```

**UM amp estéreo, não dois.** O desenho dizia `amp A` e `amp B` de quando eram
dois módulos; hoje é um TPA3116D2 de dois canais, e as duas saídas partilham a
alimentação dentro da placa — **um banco à entrada serve os dois**. O `WIDI VI`
também saiu: é a ponte ESP32.

**Por que 15V e não 12V:** 5V/9V/**15V**/20V são os perfis PD **obrigatórios**;
12V é opcional e muitos powerbanks não oferecem. 15V = qualquer powerbank
serve, que era o objetivo (powerbank vira consumível, não peça do projeto).

**Por que sem boost:** o MT3608 de 2A não deu conta — entregava 6V sob carga.
Capacitor não resolveu, e o diagnóstico é o verbete: *capacitor empresta
energia pra PICO; se a tensão continua caindo, falta corrente MÉDIA, e nenhum
capacitor fabrica potência.* O decoy PD não cria tensão, **negocia** — o
trabalho pesado fica na eletrônica do powerbank, dimensionada de fábrica.

**Obrigatório:** **2200 µF / 35 V + 100 nF colados nos pinos do amp.**
Medido em 31/08: sem banco, alimentar a Daisy pelo trilho fazia o exciter
zumbir, e alimentá-la à parte limpava. **Na galeria não há fonte à parte**,
portanto isto é requisito da peça, não recomendação.

🚫 **NADA DE BANCO NA ENTRADA DO STEP-DOWN.** Tentado em 31/08 e desfeito no
mesmo dia: **guincho agudo** imediato. O amp é carga de corrente pulsada e
quer o banco; o step-down é carga de **potência constante** e não o tolera.

📖 O mecanismo (impedância de entrada negativa, critério de Middlebrook) e o
resto das regras de desacoplamento estão em
[`ATERRAMENTO.md`](ATERRAMENTO.md) §4.

### Os dois pots estão com as pontas trocadas — corrigido em firmware

Medido em 31/08: **sentido horário DIMINUÍA o volume** nos dois. Não é
software — é a ordem dos dois fios das **extremidades** do potenciómetro, com
o cursor a subir na direcção da ponta ligada ao `GND`.

O conserto ficou em `hal_daisy_seed.cpp`, na tabela `kKnobInverte[2]`, e não na
macro. A fronteira é essa: **o sentido de um potenciómetro é um facto do
cobre**, e invertendo já na leitura tudo a jusante — captura, histerese,
passos, `Inverso()` — trabalha em coordenadas corrigidas, sem um segundo sítio
onde alguém possa esquecer o sinal.

Se algum dia os fios forem re-soldados na ordem certa, **é essa tabela que
passa a `false`** — não há mais nenhum lugar a mexer.

### Terra, massa e desacoplamento

📖 **O assunto tem capítulo próprio: [`ATERRAMENTO.md`](ATERRAMENTO.md).**
Taxonomia dos três tipos de terra, estrela, `AGND`/`DGND`, condensadores por
tipo de carga, entradas de alta impedância, acoplamento pelo ar, e o método de
medição sob carga. **Quando este arquivo e aquele discordarem, aquele manda.**

Revisão de terra no desenho de placa: skill `revisao-terra`.

O desenho alvo **deste** instrumento:

```
   ILHA DE ÁUDIO — só sinal
     AGND ── piezos ── comuns dos DPDT ── sleeve dos jacks ── entrada do amp
          ↑ encontra o DGND SÓ por dentro da Daisy
   TRILHO DE FORÇA E LÓGICA
     DGND ── fio próprio à estrela ── pots (3V3 da Daisy) ── encoders ── botões
          ── ponte ESP32 (fio próprio à estrela)
```

### O pot não chega às pontas — batentes esticados em firmware

Medido em 31/08, varrendo os dois de ponta a ponta. **O valor de cobre**, sem
inversão nem correcção:

| pot | fundo | topo |
|---|---|---|
| k1 (L) | 0,0009 | **0,9756** |
| k2 (R) | 0,0005 | **0,9753** |

O fundo chega quase perfeito; **o topo pára a 0,975** — o cursor nunca entrega
os 3V3 inteiros. Depois da inversão isso vira um **piso de 0,025**, que não é
silêncio: é cerca de **−32 dB, audível**. Foi exactamente o que se ouviu com os
pots no mínimo, e o teste que o provou foi pôr `level_l`/`level_r` a zero por
comando — aí houve **silêncio total**, o que também absolveu o amp.

O conserto são `kKnobFundo` (0,035) e `kKnobTopo` (0,990) em
`hal_daisy_seed.cpp`, aplicados depois da inversão.

⚠️ **As constantes ficam DENTRO do curso medido, não nele.** Assim cada ponta
ganha ~1% de zona morta onde o valor assenta em 0 ou 1 e **lá fica**. Batente
posto no valor medido daria um extremo que só se alcança com sorte e que o
ruído do ADC faria tremer.

E há um segundo defeito que o piso causava, menos óbvio: com o parâmetro em 0
e o pot no seu mínimo de 0,025, a **janela de captura era 0,02** — menor que o
piso. O pot ficava **permanentemente incapaz de retomar o parâmetro**. Esticar
os batentes cura os dois de uma vez.

`knob` mostra agora `cobre X INV -> vale Y`: o valor bruto ao lado do tratado,
porque **uma correcção que apaga a própria evidência não se pode re-conferir**.

### Os condensadores desta peça

| consumidor | precisa | estado |
|---|---|---|
| **amp TPA3116** | 2200 µF / 35 V + 100 nF nos pinos | ✅ soldado |
| **step-down 5 V** | **nada na entrada** | ✅ removido (era o guincho) |
| **ponte ESP32** | 100 µF / 16 V + 100 nF + terra própria | ⬜ por soldar |
| **Daisy** | 100 µF / 16 V + 100 nF | ⬜ recomendado |

📖 O porquê de cada valor, e as proibições, em [`ATERRAMENTO.md`](ATERRAMENTO.md) §4.

**Terra em ESTRELA, não margarida.** O ponto é onde o `GND` do decoy PD chega
ao amp, e é dali que bifurca para o step-down. O retorno do amp é o fio mais
curto e grosso.

🚫 **Switch só no positivo. Terra nunca passa por chave.**

---

## 2 · ENTRADA DOS PIEZOS (×2) — só proteção, ganho intacto

⚠️ **O LAÇO ACÚSTICO É REAL, E CEIFA O ADC — medido em 31/08.**

Exciter e piezo vivem no mesmo corpo, portanto **são** um laço de realimentação.
Medido com a mesma força de toque, mudando só se a saída toca:

| | saída a tocar | saída muda |
|---|---|---|
| **IN_L** | **0,0 dBFS · 313 amostras ceifadas** | **−7,4 dBFS · 0 ceifadas** |

**O laço acrescenta ~7,4 dB**, e é exactamente o que falta para bater no tecto
com `level` a 0,6. (Para comparar: a corda dava −17 dBFS e a membrana −21 em
medições sem laço.)

O laço **não é defeito** — é metade do que o instrumento tem de bom. O que não
presta é o fim dele: **ceifado no ADC**, distorção digital dura e sem controlo.

⚠️ **Hoje não há controlo de ganho de entrada.** O `in_gain` foi aposentado em
31/08 por estar inerte — decisão certa, mas isto é o primeiro caso que pede um
trim de entrada a sério. Enquanto não houver, o controlo é **baixar o `level`**
até o `pk` deixar de acusar `clip`.

**Diagnóstico:** `pk reset`, tocar, `pk`. Se `clip > 0`, está a ceifar. Repetir
com `set level 0` separa **laço** de **fonte quente**: se o pico cair, é laço.


```
piezo (+) ──1kΩ──●──> AUDIO IN
                 ├── D1 1N4148 (cátodo → 3V3)
                 └── D2 1N4148 (cátodo → o nó, ânodo → GND)
piezo (−) ────────────> trilho de terra (direto, sem resistor)
```

- Jacks **1/4" TRS** no painel · tip = sinal, sleeve = terra, ring aterrado
- Fio **blindado**: centro = (+), malha = (−)
- Piezo A → AUDIO IN L → cadeia A · Piezo B → AUDIO IN R → cadeia B

**Sem atenuador, e o motivo é de método:** o clipe medido veio de *dedo direto
no disco solto* — regime que não é o de uso. Na escultura o piezo lê flexão de
placa, nível muito mais baixo. **Validade de medição tem REGIME.** Pads do
divisor previstos na carrier board, não populados.

**Os diodos são invisíveis abaixo de 3,3 V** — não atenuam, não colorem, só
acordam no pico excepcional. Os dois apontam na mesma direção (cátodo pro lado
mais positivo do seu par).

**Verbete:** *defeito físico num transdutor imita perfeitamente assimetria de
circuito.* O p2 chegando baixo era o piezo, não o canal — o teste que separou
foi **trocar os piezos de canal**, não medir mais fundo.

---

## 3 · SAÍDA — dois ramos, e o destino é da CHAVE

```
OUT_L ──> [DPDT 3 pos: jack | ambos | amp] ─┬──> jack TS
                                            └──> TPA3110 PBTL ──> exciter DAEX32QMB-4 (4Ω)
OUT_R ──> idem, chave própria
```

**O firmware não nomeia destino — nomeia SAÍDA.** `OUT_L` não é "o exciter"
e `OUT_R` não é "o jack": cada um vai para onde a sua chave mandar, e as
três posições existem justamente porque o destino muda em operação.

<!-- doc-check: ignore route_exciter (nome APOSENTADO em 28/08, hoje `out_l`. A mencao e deliberada: e o registo da renomeacao, e apagar o nome velho apagaria justamente o que explica por que ele estava errado.) -->
<!-- doc-check: ignore route_jack (idem, hoje `out_r`) -->
Foi por não ver isto que os params se chamaram `route_exciter` /
`route_jack` até 28/08 — e o nome enganou *dentro da sessão que o
corrigiu*, quando `route_exciter 0` foi usado a pensar que silenciava só o
transdutor. Hoje chamam-se `out_l` / `out_r` (CC 113/114), e cortam o ramo
inteiro, seja qual for o destino em que a chave estiver.

**Qual CADEIA entra em qual SAÍDA é decisão de firmware**, e é a matriz 2×2
— `route_l_a`, `route_l_b`, `route_r_a`, `route_r_b`, nos CC 97/98 (o CC diz
a saída, o canal diz a cadeia). Nasce cheia, que reproduz exactamente o
código fixo que a antecedeu. Serve para inverter as cadeias quando os
transdutores estão ligados ao contrário, em dois gestos e sem duplicar
preset nenhum — e por isso **fica fora dos presets**: é facto de fiação, não
de timbre.

⚠️ **Esta chave contradiz a regra do parágrafo seguinte, e a regra é que
está mal formulada.** *"Áudio não passa por contato mecânico"* nasceu de uma
chave de POTÊNCIA (contato seco, não conduz nível de linha) e generalizou
demais. O DPDT de sinal conduz áudio de propósito. A regra correcta é a que
o caso original ensinou: **chave de potência não serve para comutar sinal.**

**O switch mora ANTES do amp, em nível de linha.** Nunca depois: o TPA3110 em
PBTL é ponte — **os dois terminais de saída são quentes**, e encostar um deles
no terra do jack mata o amp.

**Resistor de 10 kΩ permanente** do IN+ do amp ao GND: entrada flutuante é
antena amplificada direto no exciter.

**Regra de arquitetura:** *áudio não passa por contato mecânico em lugar nenhum
deste instrumento — chave governa GPIO, firmware corta o ramo.* Descoberto por
medição: chave de potência de 3 posições (marcada 12V) tem **contato seco** e
não conduz nível de linha.

**Exciter é surdo em grave: 100 Hz serve pra medir, não pra ouvir.**

**A CORRENTE DO EXCITER NÃO SE MEDE COM SHUNT.** Pelo mesmo motivo que o
switch não pode morar depois do amp: em PBTL a saída é ponte comutada a
centenas de kHz, **não há DC para medir e os dois terminais são quentes** —
um INA ali não tem modo comum válido nem sinal que faça sentido. Para o
exciter: microfone, ou pinça de corrente AC.

Quem responde "quanto o amp está a puxar" é a medição **na entrada dele**,
no ramo dos amps depois da bifurcação — ver o painel 5 dos esquemáticos. E
ela responde a pergunta ELÉCTRICA: corrente é bom detector de anomalia e mau
juiz de qualidade. Um exciter mal acoplado à escultura pode puxar o dobro e
soar menos. Para *"está melhor?"* o instrumento é o ouvido.

---

## 4 · WIDI CORE — BLE MIDI, 6 fios

> ⚠️ **SUBSTITUIÇÃO DECIDIDA, AINDA NÃO EXECUTADA.** O WIDI sai a favor de um
> ESP32-C3 (US$ 45 → US$ 4, numa peça replicada em série na ordem de
> centenas) — decisão e preço em [GEAR-AUDIT.md](GEAR-AUDIT.md), voo em
> [VOO-ESP32-PONTE.md](VOO-ESP32-PONTE.md). **Sem plano B: o aceite é
> portão**, e o que ele trava é a encomenda.
>
> Esta secção fica porque descreve o que está **na bancada hoje**. Sai
> inteira quando o portão passar — e nunca sai do `battle_scars.md`, onde o
> cruzamento TX/RX continua vivo, porque se aplica igual ao substituto.

| WIDI | Daisy | nota |
|---|---|---|
| VI | rail **5V** | não 3V3 |
| ON | jumper do VI | sem isso, dorme |
| GND | rail de terra | |
| RX | ← **D13** | |
| TX | → **D14** | ★ essencial — é o que recebe MIDI |
| LB | → **D28** | status do link |
| V3 | (só medir) | ≈3,3 V = módulo vivo |

**DADOS CRUZADOS.** Ligar reto não queima — dá silêncio, idêntico a módulo
desligado. Se `midi` der 0 bytes, desconfiar do cruzamento **antes de tudo**.
LG/LR removidos (não marcavam estado útil) → D29/D30 livres.

---

## 5 · PAINEL v2 (projetado, não montado)

- **OLED Waveshare 2.42" SSD1309**, 128×64, SPI 4 fios, tradutor de tensão
  embarcado (3,3 V ok). Driver **já existe na libDaisy** (SSD130x).
  Área visível 55,01 × 27,49 mm · módulo 61,50 × 39,50 mm · conector MX1.25 7p.

  **As sete cores do chicote que vem com o painel**, e o pino da Daisy de
  cada uma. Está aqui porque cor de fio é a única etiqueta que sobrevive à
  bancada — e em 28/08 um cabo trocado sem marca custou um diagnóstico
  inteiro, com o defeito atribuído a um conserto que estava certo:

  | cor | sinal | Daisy |
  |---|---|---|
  | vermelho | VCC | 3V3 |
  | preto | GND | GND |
  | branco | RST | **D5** |
  | verde | DC | **D6** |
  | laranja | CS | **D7** |
  | amarelo | CLK | **D8** |
  | azul | DIN (MOSI) | **D10** |

  Diagnóstico na placa: `oled` inicializa e acende · `oled off` apaga ·
  `oled texto` escreve (**prova mais que o fill**: encher pode acontecer com
  o `DC` preso, escrever exige que ele comute) · `oledvel <2..256>` varre a
  velocidade do SPI · `fio curto` caça pontes nos dez pares · `fio <rst|dc|
  cs|clk|din> <0|1>` segura um pino para medir **no conector do painel**.

  ⚠️ **O painel NUNCA responde** — é escrita pura, sem MISO. O firmware só
  pode dizer *CONFIGURADO*, nunca *presente*. O único detector é o olho.

  ⚠️ **Conferir os jumpers `BS` no verso do módulo.** Eles escolhem o
  interface — 4-wire SPI, 3-wire SPI, I²C ou paralelo. Em I²C ou 3-wire o
  pino `DC` deixa de ser `DC`, e o firmware fica a falar a língua errada num
  painel que responde o suficiente para limpar a tela e mais nada.
- **Encoder EC11** com detentes e push (push = confirmar). Relativo, não
  absoluto — mata o problema do salto do knob compartilhado.
- **2 pots de volume**, um por ramo — ✅ **soldados e implementados em 28/08**:
  `A0` = **D15** (ramo L) · `A1` = **D16** (ramo R). Params `level_l`/`level_r`
  (CC 116/117), aplicados **depois do limiter**, onde só se pode reduzir.

  ⚠️ **Não há um terceiro pot possível.** `A0–A4` são aliases de `D15–D19`, e
  o encoder global ocupa `D17/D18/D19`. O bloco do ADC acabou.

  **Pot é absoluto e o param é partilhado com o MIDI**, então há **captura
  suave**: o pot só assume quando passa pelo valor actual, e larga quando o
  iPad escreve. Excepção no arranque — a primeira leitura **adopta** a
  posição física, porque aí ninguém escreveu ainda e a verdade é o botão.
  Diagnóstico: `knob` na serial.
- **Botões de arcade 30 mm** com LED (freeze A e B, búzios).
  LED de 5 V acionado com 3,3 V: **brilho aprovado no ambiente real**; plano B
  do transistor arquivado sem custo.

**Burn-in:** contraste baixo + deslocar 1–2 px a cada minutos. A peça pode
ficar meses ligada numa galeria.

---

## 6 · CARRIER BOARD rev A (projetada, não fabricada)

KiCad → JLCPCB, SMD. Bloco de energia com esquemático **verificado pino a
pino** (netlist conferida contra a conectividade pretendida, automaticamente).

Peças críticas com número LCSC verificado: **CH224K** `C970725` (PD sink),
**TPA3116D2DADR** `C50144` (amp). O medidor de corrente é o **INA219**, a
mesma peça da bancada — LCSC ⚠️ **por verificar**, ao contrário dos outros
dois. O INA226 saiu do desenho em 25/08: bancada e produção passam a ser a
mesma peça, e número medido numa peça não se transporta para outra.

⚠️ **Conector de 2 fios NÃO serve para a entrada** — PD negocia pelos pinos
CC1/CC2. Pigtail sem CC deixa o chip surdo e o powerbank nunca sai dos 5V:
falha em silêncio, em cobre.

---

## Checklist antes de energizar

⚠️ **Antes de energizar montagem nova, correr a revisão de terra** — skill
`revisao-terra`. Ruído de massa imita defeito de firmware, e diagnosticá-lo
depois custou uma tarde e três acusações a inocentes.

- [ ] Continuidade +V ↔ GND: **não pode bipar**
- [ ] Decoy PD medido: **15 V confirmados sem carga**
- [ ] Step-down medido em **5 V** antes de encostar na Daisy
- [ ] Polaridade do capacitor (tarja = negativo) e dos diodos (faixa = cátodo)
- [ ] Switch no **+V**, terra contínuo em estrela, zero loops
- [ ] V3 do WIDI ≈ 3,3 V depois de energizar
- [ ] Exciter **por último**, com `level` baixo e anti-feedback conferido
