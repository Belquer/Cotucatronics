# HARDWARE — Cotucatronics

> Decisões fechadas, com o **porquê** de cada uma. Esquemáticos em
> `esquematicos/`. Quando uma decisão for revista, o motivo antigo fica
> registrado — premissa revogada em silêncio é a família de bug mais cara
> deste projeto.

---

## 1 · ENERGIA — entrada única, sem boost

```
qualquer powerbank PD ──USB-C──> [decoy PD 15V, 5A] ──> RAIL 15V
                                                          ├──> amp A (+cap)
                                                          ├──> amp B (+cap)
                                                          └──> step-down 5V ──> Daisy VIN + WIDI VI
```

**Por que 15V e não 12V:** 5V/9V/**15V**/20V são os perfis PD **obrigatórios**;
12V é opcional e muitos powerbanks não oferecem. 15V = qualquer powerbank
serve, que era o objetivo (powerbank vira consumível, não peça do projeto).

**Por que sem boost:** o MT3608 de 2A não deu conta — entregava 6V sob carga.
Capacitor não resolveu, e o diagnóstico é o verbete: *capacitor empresta
energia pra PICO; se a tensão continua caindo, falta corrente MÉDIA, e nenhum
capacitor fabrica potência.* O decoy PD não cria tensão, **negocia** — o
trabalho pesado fica na eletrônica do powerbank, dimensionada de fábrica.

**Obrigatório:** capacitor **2200 µF / 35 V** (derating 2× sobre 15V) + 100 nF
**colados nos pinos de cada amp**. Dois consumidores no mesmo trilho roubam
transiente um do outro.

**Terra em ESTRELA, não margarida.** Um ponto físico onde chegam: GND da
Daisy, sleeve dos jacks, terra de sinal dos amps, negativo da alimentação. O
retorno da alimentação do amp é o fio mais curto e grosso.

**Switch só no positivo. Terra nunca passa por chave.**

---

## 2 · ENTRADA DOS PIEZOS (×2) — só proteção, ganho intacto

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

## 3 · SAÍDA — dois ramos, dois exciters

```
OUT_L (cadeia A) ──> [switch] ──> TPA3110 PBTL ──> exciter Dayton DAEX32QMB-4 (4Ω)
                 └──> jack TS (sempre ligado)
OUT_R (cadeia B) ──> idem
```

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
- **Encoder EC11** com detentes e push (push = confirmar). Relativo, não
  absoluto — mata o problema do salto do knob compartilhado.
- **2 pots de volume**, um por exciter.
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

- [ ] Continuidade +V ↔ GND: **não pode bipar**
- [ ] Decoy PD medido: **15 V confirmados sem carga**
- [ ] Step-down medido em **5 V** antes de encostar na Daisy
- [ ] Polaridade do capacitor (tarja = negativo) e dos diodos (faixa = cátodo)
- [ ] Switch no **+V**, terra contínuo em estrela, zero loops
- [ ] V3 do WIDI ≈ 3,3 V depois de energizar
- [ ] Exciter **por último**, com `level` baixo e anti-feedback conferido
