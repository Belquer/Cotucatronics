# FUTURO — o que se pensou, o que custa, e por que não é agora

> **Este documento não é uma lista de tarefas.** O que está por fazer vive no
> [`STATUS.md`](STATUS.md); os defeitos, no [`battle_scars.md`](battle_scars.md).
> Aqui ficam **possibilidades**: coisas que se consideraram, com o custo já
> apurado e a razão de terem ficado de fora.
>
> Existe porque a alternativa é redescobrir. Três vezes nesta semana alguém
> reabriu uma questão que já tinha resposta medida, e a resposta estava numa
> conversa que ninguém guardou.

## O orçamento, e é ele que decide quase tudo

**Dez pinos livres:** `D3` `D4` `D9` `D11` `D12` `D18` `D19` `D20` `D21` `D25`

Cada ideia abaixo gasta deste bolso. E há dois que **não** estão nele e nunca
vão estar:

🔴 **`D29` e `D30` são as linhas de dados do USB** (`USB_HS_D−`/`D+`, pinos
36/37 do header). Usá-los como GPIO mata a serial — e uma placa assim dá
firmware a correr, painel bom, alimentação normal, e nenhuma forma de falar
com ela. Já passámos um dia inteiro nesse quadro por outra causa.

`D20` e `D21` são os únicos livres que também são **ADC** (`ADC_INP11`,
`ADC1_INP4`) — quem precisar de leitura analógica gasta desses dois primeiro.

---

## 1 · Sense dos DPDT — `exc_on` a ler hardware em vez de acreditar

**Estado: descartado a 2026-09-05. Nice to have.**

Hoje o `exc_on` é **intenção de software**: alguém declara "o exciter está no
circuito" e o firmware acredita. Quando a declaração e a realidade divergem,
a proteção anti-realimentação (shifter + Governor) fica ligada sem laço para
quebrar — custa timbre — ou desligada com laço no ar, que é o caso que já
custou incidentes de microfonia.

**Por que ficou de fora:** a **DPDT tipo 2** usa os dois polos para o mesmo
canal (comuns amarrados, como o desenho pede), portanto **não sobra contacto**
para ler o estado.

**O que custaria:**

| caminho | custo |
|---|---|
| **3PDT** ou DPDT com terminal auxiliar | 1 pino por canal · peça diferente na BOM · o polo extra é limpo e é a via certa |
| ler um terminal comutado com pull-up | 1 pino por canal · ⚠️ põe um pino do MCU em contacto com o caminho de áudio |
| ⭐ **detectar por sinal** | 0 pinos · o `AUDIO_OUT` a sair sem o `AUDIO_IN` reagir *é* a assinatura de exciter fora do circuito, e o SuMiA™ já mede dominância |

A terceira não estava na conversa e talvez seja a melhor: **não gasta pino
nenhum** e usa medição que já existe. Fica como ideia, não como plano.

## 2 · Tela colorida

**Estado: possibilidade. Nunca orçamentada.**

⚠️ **O aviso que qualquer estudo tem de começar por ler:** o painel
monocromático aceso **mediu 6 dB de subida no chão de ruído da entrada** —
IN_L de −75,3 para −69,3 dBFS, com um espeto a 7040 Hz e dominância 0,95. É
corrente de accionamento das linhas, **proporcional aos píxeis acesos**.

Uma tela colorida tem mais píxeis, mais bits por píxel e mais taxa de
actualização. **O custo em ruído não é linear e não é pequeno**, e num
instrumento cuja entrada é um piezo de alta impedância isso vai direto ao
sinal.

Foi por causa desses 6 dB que o painel actual **nasce apagado** e dorme aos
5 s (v0.221.0). Uma tela colorida provavelmente não pode ficar acesa enquanto
se toca — e aí a pergunta é se ainda serve.

**O que se mediria antes de decidir:** o chão de entrada com a tela nova
acesa e apagada, no regime de uso, com o instrumento a tocar — nunca em
repouso, que foi o erro de 2026-09-02 (74,7 dB de margem em repouso contra 0,4 dB
a tocar).

**Pinos:** SPI já existe (`D5..D10`). Uma tela maior pede provavelmente mais
largura de banda, não mais pinos — mas confirmar contra a folha de dados da
tela, não de memória.

## 3 · Os encoders que saíram

**Estado: reversível, e os pinos estão guardados.**

Havia três; a 2026-09-05 ficou **um**, com push+giro a dar os outros controlos.
`D3 D4 D9` (encoder B) e `D18 D19` (encoder G) voltaram ao bolso.

⚠️ **Se algum voltar, cuidado com o `D17`.** Ele era o push do encoder G e
passou a ser o **A2 do divisor** que lê a tensão do trilho. Devolver o
encoder G tal como estava recria a colisão — e ela é invisível aos
verificadores, porque os dois usos moram em documentos diferentes.

E fica a lição do encoder G, que vale independentemente: quando se trocaram
as fases dele e a contagem parou, o defeito **não era a troca** — o `Init` e
o `Debounce` tratam `a` e `b` simetricamente, logo trocar só pode inverter,
nunca zerar. Eram as duas fases abertas no chicote.

## 4 · Coisas menores, com o custo já sabido

| ideia | pinos | nota |
|---|---|---|
| **LED RGB** em vez dos dois monocromáticos | +2 (ou 1 com WS2812) | ⚠️ um LED consome miliamperes e **já é audível** quando a massa está mal feita — foi assim que se apanhou acoplamento em 2026-08-31. Um RGB comuta mais |
| **Quarto botão arcade** | +1 | ⚠️ o terceiro **já existe**: o branco dos búzios, em `J5` → IDC 19 → Daisy 33. Ele não tem LED, e é por isso que há dois MOSFET para três botões — sabe-se que está ligado pelo **display**. O IDC 2×13 está cheio: um quarto sinal exige 2×15, ou sacrificar terra — e as **oito** que restam guardam todas coisa medida (relógio do OLED 1/3, referência do ADC 8/10, retorno dos LED 13/14 que valeram 6 dB, retornos dos pots 23/26) |
| **Entrada de expressão / pedal** | +1 ADC | gasta `D20` ou `D21`, os únicos livres com ADC |
| **Segundo par de exciters** | — | não é pino, é potência e é caixa. O amp actual tem dois canais |

---

## A regra que este documento serve

> **Uma possibilidade sem custo apurado é uma conversa; com custo apurado é
> uma escolha.**

Cada entrada aqui tem o preço em pinos, em ruído medido, ou em peça de BOM.
Quem reabrir uma delas começa onde a última conversa parou, e não do princípio.

E quando uma destas for feita, **sai daqui** — para o `FEATURES.md` se vive,
ou para o `battle_scars.md` se ensinou alguma coisa a caminho.
