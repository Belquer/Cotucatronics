# ppc/008 — a versão nova: cérebro, silêncio, e o que a pesquisa desmentiu

<!-- doc-check: ignore funcao_dos_pads (portao do /aipcb, ferramenta em kicad/) -->

> **Estado: PESQUISA FECHADA EM 5 DE 7 FRENTES, decisão principal TOMADA.**
> Escrito a 2026-09-29, a seguir à noite em que a caixa montada se recusou a
> funcionar e o dB disse: *"consertaremos uma coisa e outra vai estragar."*

**O pedido que o originou, nas palavras dele:** deixar o sistema actual como
aprendizado, seguir para caixa e placa novas, **som mais limpo, nível
audiófilo**, aproveitando o que for bom — *"sem apego às peças, focando no
objetivo de um instrumento sólido, limpo e preciso."*

---

## §0 — A DECISÃO, e ela inverteu-se a meio da pesquisa

> **Não se faz placa própria agora.** Faz-se **caixa e carrier novas,
> mecânica-primeiro, com um Daisy Seed3** — e mede-se.

Eu tinha recomendado o contrário (STM32H7R3 próprio). O estudo de custo
desmentiu-me, e a razão é simples: **o benefício que eu invoquei já está à
venda.**

| por unidade | 5 un | 10 un | 30 un | 100 un |
|---|---|---|---|---|
| placa própria (JLCPCB) | ~$33 | ~$24 | ~$18 | ~$14,6 |
| Seed3 + carrier | ~$55 | ~$49 | ~$44 | ~$40 |
| **poupança** | ~$22 | ~$25 | ~$26 | **~$25** |

**A poupança é plana** — a curva de desconto da Daisy também é (5 % a 100 un),
e o ganho vem de trocar um módulo de $30 por ~$9 de silício, não de escala.

Contra isso: a média da indústria é **2,9 respins**, ~90 % dos primeiros
protótipos falham, e só o hardware dos respins custa **$700–1600**. Ponto de
equilíbrio: **~40 placas** (só hardware) ou **~200** com o tempo de projecto a
preço de mercado. Claramente certo a partir de **300–500 un/ano**.

**A 10 unidades, a placa própria perde feio:** semanas e dois ou três respins
para poupar $250.

### E o projecto já tinha decidido isto, a 05/09

A `COTUCA_BOM.md` diz, e a razão é a mesma de hoje:

> *"A Daisy Seed rev4. **RESOLVIDO a 05/09: passou a Seed3.** A pinagem foi
> conferida pino a pino... o nome STM32 bate nos 40 pinos, sem uma excepção...
> Símbolo, footprint e netlist ficaram **idênticos**. O que mudou a favor:
> codec **TI TAC5242**, 32 bit/192 kHz, chão de ruído de −120 dB."*

A pesquisa de hoje **não descobriu o cérebro — confirmou uma decisão de há três
semanas**, e o Seed3 é `drop-in`: *"backwards compatible with existing firmware
binaries"*. Zero alterações de firmware, zero de netlist.

⚠️ **Acção com prazo:** a Electrosmith deixou de vender por distribuidores e o
mercado de memória está tenso até 2027. A 29/09 havia **44 Seed3** e **436
Seed2 DFM** em stock directo, a $29,99.

### Quando reabrir a questão da placa própria

1. Quando houver compromisso de **300+ unidades/ano**; **ou**
2. quando estiver **medido** que a arquitectura sem SDRAM serve (ver §1) — e aí
   desenha-se uma placa deliberadamente **melhor**, não só mais barata.

---

## §1 — A conta de memória, que é o que decide qualquer placa própria

**Medido na placa, não estimado** (`SDRAM via AllocBig` no arranque):

```
SDRAM via AllocBig: 2 035 216 de 67 108 864 bytes
  motores de pitch   150 KB × 2
  delay              187 KB
  freeze             750 KB × 2
```

**1,94 MB de RAM grande + 318 KB de `.bss` = ~2,26 MB.**

🔴 **NENHUM STM32H7 TEM ISSO INTERNAMENTE.** O maior (H7A3/H7B3/H7B0) tem
1,4 MB, e **1024 KB contíguos**. As peças com RAM suficiente são muito mais
lentas (U5G9, 160 MHz, 0,27× a nossa CPU) ou só existem em BGA (N6).

### ⚠️ E o baseline de CPU que eu usei estava errado

Eu li **29,6 % / 40,8 %** na placa — mas **com o shimmer inactivo**. O
`STATUS.md` documenta o caso cheio: **59,5 % média / 65,8 % pico**.

> **Valor lido de placa viva não tem procedência.** Eu medi o que estava a
> correr e tratei-o como o pior caso.

Com 65,8 %, o H7B0 a 280 MHz dá **113 % do prazo** — rebenta. Era a peça
elegante (1024 KB contíguos seguravam `.bss` + pitch + delay dentro do chip) e
morre por aqui.

### O achado técnico que torna a placa própria possível um dia

> **O QUADSPI do H750 só lê em modo mapeado. O OCTOSPI escreve.**

É **por isso** que a Daisy precisa de SDRAM paralela. Qualquer peça com OCTOSPI
(H723, H7A3/B3/B0, H7R3/S3) permite PSRAM série como RAM de trabalho.
Candidatos verificados: **STM32H7R3Z8T6** ($6,10/100 na ST, 600 MHz, LQFP144) +
**APS6404L-3SQR-SN** (8 MB, SOP-8, 3,0 V, $2,90/100).

### 🔴 CORRECÇÃO (estudo da memória, 29/09): a PSRAM NÃO devolve os 12,9 pontos

Aquela medição foi **externo contra interno**, não SDRAM contra PSRAM. Custo de
uma linha de cache de 32 bytes (o Cortex-M7 tem linha fixa de 32 B):

| memória | 1.ª palavra | pico | **falha de 32 B** |
|---|---|---|---|
| **SRAM interna** | ~0 | ~3 GB/s | **~11 ns** |
| XSPI x16 @152 MHz (APS256, config da ST) | ~50 ns | 608 MB/s | 103 ns |
| **SDRAM da Daisy** @120 MHz CL3 | ~80 ns | 480 MB/s | **147 ns** |
| HyperRAM x8 @166 MHz | 56 ns | 333 MB/s | 152 ns |
| HyperRAM x8 @100 MHz (3,0 V) | 56 ns | 200 MB/s | 216 ns |

**A PSRAM empata com a SDRAM. Só a memória interna é ~14× melhor**, e é aí que
vivem os 12,9 pontos.

### E daí sai a melhor ideia da pesquisa toda: ENCOLHER, não acrescentar

> Um buffer de freeze de **2 s estéreo a 48 kHz** são **750 KB em float32** — e
> **375 KB em int16**.

Dois desses são **750 KB**, e o **H7A3/H7B3 tem 1024 KB contíguos**. Com os
pitch e o delay a caber no resto, **a memória externa desaparece**: zero sinais,
zero erratas de OCTOSPI, zero corrente de refrescamento ao pé do analógico, e os
12,9 pontos de volta.

⚠️ **A tensão que isto cria, e que só se resolve medindo:** o H7A3/H7B3 corre a
**280 MHz = 0,58×** a nossa CPU, o que dava 113 % do prazo pela conta de
relógio. Mas essa conta **não contabiliza as paragens de memória**, que é
precisamente o que melhora quando tudo vem para dentro. A conta de relógio diz
que não cabe; a física da memória diz que pode caber. **Não se decide por
argumento.**

### ⚠️ E a peça que eu recomendei NÃO serve

A **APS6404L** (Quad-SPI) tem **escrita mapeada quebrada**. Errata da ST
(ES0478 §2.6.8, ES0491 §2.8.6, ES0596 §2.4.1): *"memory-mapped write error
response when DQS output is disabled"*, estado **P — contorno parcial**. E o
contorno obrigatório traz o seu próprio defeito: *"os bytes que deviam ser
mascarados são escritos na memória"* — ou seja, **escrita parcial estraga as
amostras vizinhas**. O representante da AP Memory confirma-o no fórum da ST.

Se houver memória externa, as opções sãs são **x16 XSPI** (APS256XXN, $3,90/100,
mais rápida que a nossa SDRAM actual) ou **HyperRAM** (IS66WVH8M8BLL, validada
pela ST, com driver e exemplo, e cujo RWDS torna a escrita por byte **realmente
segura** — contorna a pior errata).

### 🔴 Dois riscos novos, e o segundo é de RUÍDO

**Contenção de DMA.** Medição num fórum da ST: HyperRAM a 100 MHz deu
**178 MB/s** — e caiu para **54 MB/s** quando um DMA de UART disputou o mesmo
barramento AXI. **O nosso DMA de SAI corre sempre.** É o maior risco isolado
deste plano, e mede-se no primeiro protótipo.

**Corrente de refrescamento da PSRAM.** A ficha da AP Memory: *"a corrente de
pico do refrescamento periódico interno pode chegar a 25 mA, durando algumas
dezenas de microssegundos"*. A ISSI especifica **60 mA** em rajada a 166 MHz.
**Isso é um transitório periódico, a taxa fixa, ao lado do analógico** — a mesma
família do transitório do callback que estraga a Daisy (§5). Se houver PSRAM,
**leva trilho filtrado próprio**.

🔴 **O RISCO QUE TEM DE SER MEDIDO ANTES DE DESENHAR COBRE:** os motores de
pitch fazem **acesso disperso**, e acesso disperso em memória série é dominado
por latência. Já medimos **+12,9 pontos de CPU** ao pôr o reverb em SDRAM. Com
456 KB contíguos no H7R3, **1,9 MB vai para a PSRAM, incluindo os buffers
quentes**. Experiência proposta: pôr um motor de pitch atrás de um caminho de
acesso deliberadamente lento **no hardware actual** e medir o custo no callback.

---

## §2 — O firmware já está portável, e isso é desenho, não sorte

```
main.cpp        8 318 linhas   inclui SÓ headers do projecto
hal.h             531 linhas   inclui SÓ <cstdint> e <cstddef>
DSP (18 headers)               nenhum toca em libDaisy
─────────────────────────────────────────────────────────
hal_daisy_seed.cpp  2 554      ← o ÚNICO que inclui libDaisy
qspi_raw.cpp          664      ← inclui só stm32h7xx.h (CMSIS)
```

Busca por chamadas reais a libDaisy fora da camada de placa (`daisy::`,
`DaisySeed`, `dsy_*`, `AdcHandle`, `SaiHandle`): **zero**. As ocorrências em
`main.cpp` são comentários.

**A migração é um ficheiro** — 94 funções, das quais ~20 tocam no silício. E o
`qspi_raw.cpp` escrito a 29/09 é CMSIS puro: **transfere-se tal e qual**.

**Proposta de refinamento:** acrescentar `AllocFast()` (SRAM interna) ao lado do
`AllocBig()` (memória externa), para a partição quente/frio ficar **explícita e
auditável no arranque**, como já é a contabilidade da SDRAM.

---

## §3 — O silêncio: a linha de base é NOSSA, e é melhor que a literatura

| condição | IN_L | IN_R |
|---|---|---|
| **melhor já medido** (painel apagado) | **−75,3** | **−77,7** dBFS |
| com o painel OLED aceso | −69,3 | −71,3 dBFS (**−6 dB**) |
| **antes do conserto de terra** | −34,2 | −34,2 dBFS |
| saída em repouso | pico −74,7 · média −89,6 dBFS | |
| tom permanente medido 29/09 | **4999,1 Hz a 33,9 dB**, dominância 1,00 | |

**Refazer o terra valeu 35 a 39 dB.** Trocar o conversor vale ~15–33 dB.

⚠️ O agente de aterramento procurou medição publicada equivalente ao nosso
ganho e **não encontrou nenhuma**. Isto é dado próprio, e vale mais aqui do que
qualquer fonte externa.

### Por que foi o terra, e por que não podia ter sido outra coisa

Archambeault (IEEE EMC Society, 2008) mede a transição: **abaixo de ~800 kHz** a
corrente de retorno segue o caminho de **menor resistência**; acima, o de
**menor indutância**.

- **A banda de áudio está toda abaixo disso.** O retorno do áudio **não
  acompanha o traço** — espalha-se pelo plano. Não se controla por roteamento;
  controla-se por **não partilhar cobre**. É **acoplamento por impedância
  comum**.
- **A assinatura confirma:** −34,2 dBFS **igual nos dois canais**. Simétrico é
  referência partilhada; diafonia ou cabo estragado seriam assimétricos.
- **O retorno digital está muito acima** dos 800 kHz — esse acompanha o traço e
  fica na partição digital **desde que o plano seja contínuo**.

### 🔴 O corolário, que é o maior risco da revisão nova

> **Um LDO, uma ferrite ou um plano dividido NÃO protegem se o retorno
> analógico voltar a partilhar caminho resistivo com o retorno do class-D ou do
> conversor comutado.** Colocação e topologia de corrente dominam tudo o resto.

O que nos deu 35-39 dB pode ser desfeito por uma decisão de **posicionamento**,
e nenhum componente caro o recupera.

---

## §4 — Regras de layout, com a fonte e o número

⚠️ **A literatura discorda de si própria, e isso é dado:** TI **SLAA749**
(codecs) manda **dividir** o plano; **Ott** e TI **SLYT512** mandam **um plano
só, particionado**; TI **SLOA216** (class-D) manda **contínuo**. Os três foram
lidos em texto extraído do PDF porque **o sumarizador de busca fabricou dois
resumos** — inclusive inverteu o sentido do SLAA749.

**Escolha deste projecto: um plano só, particionado.** O SLAA749 escreve para
placas onde o codec é a única ponte; a nossa tem H7 a 480 MHz, Octo-SPI, QSPI e
USB — dezenas de redes atravessariam o corte.

| regra | fonte | número |
|---|---|---|
| Um plano contínuo, particionado; disciplina de roteamento em **todas** as camadas | Ott · SLYT512 | — |
| Codec **a cavalo da partição**, digital para o MCU, analógico para os pré-amps | Ott | — |
| Empilhamento **assimétrico**: dieléctrico fino ao plano (~0,005"), núcleo grosso ao meio | Ott | **≥10 dB** menos radiação de laço |
| Nenhuma rede rápida atravessa descontinuidade; via de retorno a **<100 mil** de cada via de transição | NXP AN13202 | — |
| Energia entra na **partição digital**, alimenta o digital, **depois** filtra para o analógico | SLYT512 | — |
| **LDO** no trilho analógico, alimentado pelo buck | SLYT512 + ADI | mV → µV rms |
| Relógios longe dos pinos de referência do codec; condensador pequeno **mais perto** | Cirrus DS686F1 §5.8 | — |
| **MCLK a 256×fs (12,288 MHz)** para **contornar o PLL do codec** | TI SLAA749 | — |
| **1 kΩ + 4,7 nF** entre codec e class-D (polo a 33 kHz) | TI SLAA749 | — |
| Class-D: **1 nF + 0,1 µF** no PVCC; ferrite eficaz **acima de 1 MHz** nas saídas | TI SLOA216 | margens >10 dBµV |
| **Espalhamento de espectro** ligado no class-D | TI SLOA216 | conformidade medida assim |
| Traços do class-D ao transdutor **curtos**; nunca em paralelo com os piezos | TI SLOA216 | "a maior fonte de emissão" |
| Baixar a **força de saída** do Octo-SPI/QSPI ao mínimo que cumpra timing | NXP AN13202 | harmónicas medidas a cair |

⚠️ **NÃO é verdade que barramento estreito irradie menos por ter menos
condutores.** Procurou-se medição publicada e não existe. O barramento série
ajuda por ser **fisicamente pequeno e fácil de manter dentro da partição
digital** — a razão importa, porque a errada leva a decisões erradas.

---

## §5 — Conversores: o campo mudou debaixo de nós

🔴 **CS4270 e CS4272 estão em FIM DE VIDA** — última compra 2023-12-10, último
envio 2024-06-07, *"EOL'd due to fab closure"*. O conversor de referência do
GuitarML **não se pode desenhar hoje**. O **WM8731 também** está EOL, e a
dinâmica dele **a 48 kHz é 85 dB**, não os 90 que se citam (90 é a 96 kHz).

⚠️ **E os datasheets do CS4270/CS4272 não trazem aviso de fim de vida.** Quem só
lê a ficha é enganado — é a regra §7f da `/kicad` aplicada ao ciclo de vida.

**A escolha é o TI TAC5242** (119 dB ADC / 120 dB DAC, um trilho de 3,3 V, PLL
com detecção automática, ~$5,84) — **e é o que o Seed3 já traz.**

### A prova de que o conversor NÃO é o limite

Histórico medido da própria Daisy:

| placa | conversor | ficha | ruído medido |
|---|---|---|---|
| Seed Rev4 | AK4556 | 103/106 | 40 µV |
| Seed 1.1 | WM8731 | 85/100 | **18,6 µV** |
| Seed 1.2 | PCM3060 | **99/104** | **234 µV** |

**A placa com o melhor conversor mediu 15-22 dB pior.** Explicação da própria
Electrosmith: *"o STM32 é demasiado bom a regular o próprio consumo — há um
transitório de corrente quando processa áudio no callback."* As placas medem
**~95 dB em modo de arranque**: silenciosas quando o callback não corre.

> **O degrau de corrente do bloco modula a alimentação e cai DENTRO da banda, à
> taxa do bloco.** Isto entra no desenho da carrier como requisito: bulk
> generoso na entrada, e considerar o tamanho do bloco como variável de ruído,
> não só de latência.

⚠️ As medições da Daisy são de amadores (uma via entrada de microfone de
portátil). **Os deltas de ~20 dB são credíveis; os microvolts absolutos não.**

---

## §6 — A entrada dos piezos

**Um piezo nunca pode atacar o conversor directamente.** As entradas são de
capacitor comutado; o CS4270 di-lo: *"THD+N degrada-se para impedância de fonte
acima de 1 kΩ"*. O nosso piezo está em megaohms.

**Topologia recomendada: amplificador de carga** (piezo contra terra virtual,
realimentação capacitiva) em vez de seguidor de alta impedância. Resolve três
problemas de uma vez: ganho passa a depender de `C_piezo/C_realim`, logo fica
**insensível à capacitância do cabo**; mata ruído triboeléctrico; e entrega ao
conversor a impedância baixa que ele exige.

⚠️ **Condensadores cerâmicos Classe 2 são microfónicos**, e a placa vai
aparafusada a um corpo que vibra com exciters. **C0G ou filme no caminho do
piezo.** (O GuitarML usa Classe 2 no caminho de áudio.)

**Resultado contra-intuitivo, por confirmar na bancada:** para uma fonte
capacitiva, **subir a resistência de polarização BAIXA o ruído em banda** — a
capacitância do piezo filtra o ruído da resistência, e o integrado tende para
√(kT/C), independente de R. Dois agentes derivaram o mesmo número
independentemente, **mas não há fonte publicada que o diga no contexto do
piezo**. Fica como experiência, não como facto.

---

## §7 — O que falta, e o que NÃO foi verificado

**Estudos por fechar:** pré-amps de piezo publicados e medidos · plataformas
abertas (Mutable Instruments, Bela, Axoloti, OWL).

**Lacunas declaradas, para ninguém as tratar como factos:**

- **Zero preços lidos na Mouser ou DigiKey** — ambas bloqueiam acesso
  automático. Tudo veio da ST eStore, do LCSC e de agregadores.
- **Preços de PCB da JLCPCB são DERIVADOS**, não cotados: a ferramenta exige
  login e Gerbers. **Correr antes de orçamentar.**
- **Se a JLCPCB permite ENIG no serviço Económico** — vale ~$75 de uma vez.
- **Impostos de importação do Brasil** — fontes de 2026 contradizem-se. A
  conclusão robusta é que **o Brasil duplica o custo à chegada**; as
  percentagens não são de confiar.
- **O argumento verbatim de Rick Hartley contra dividir o plano** — atribuído
  universalmente, sem transcrição nem artigo encontrado.
- **Nenhuma medição de EMI comparando SDRAM com Octo-SPI existe** em nada que
  os agentes encontraram. Os argumentos de contagem de pinos e de excursão de
  tensão são raciocínio de primeiros princípios. **Se quisermos saber, medimos
  nós.**
- **A SDRAM da Daisy pode já ser de 1,8 V** (a AS4C16M32MSA é a peça *mobile* da
  Alliance). Se for, metade do argumento de ruído sobre excursão de tensão não
  se aplica ao nosso caso — a metade da contagem de pinos aplica-se na mesma.
- ⚠️ **O sumarizador de busca fabricou preços e resumos repetidamente** durante
  esta pesquisa — o mesmo preço para quatro peças diferentes, e a inversão do
  sentido de um documento da TI. **Todo número aqui foi lido de uma página, não
  de um resumo.**
