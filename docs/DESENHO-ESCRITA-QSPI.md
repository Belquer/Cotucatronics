# Desenho — rotina própria de escrita na QSPI

**Estado:** desenho, nada implementado. Escrito depois do inventário, antes
da primeira linha — que é a ordem que esta sessão aprendeu a respeitar.

**Por que rotina própria e não a libDaisy:** ver bts 12v. `PersistentStorage`
tem pré-condição não escrita — *"o app não executa da QSPI"* — revogada por
`APP_TYPE=BOOT_QSPI` sem aviso de compilação.

**Por que não forkar `qspi.cpp`:** o fecho transitivo decide, e decide contra
o fork.

    EraseSector chama
      ├─ CheckProgramMemory  → EnterQuadMode, GetProgramMemoryRegion, SetMode
      ├─ SetMode             → Init, e recursivamente SetMode
      ├─ WriteEnable         → HAL_QSPI_Command, HAL_QSPI_AutoPolling
      ├─ HAL_QSPI_Command    → driver da ST
      ├─ AutopollingMemReady → HAL_QSPI_AutoPolling
      └─ ERR_RECOVERY        → SetMode(MEMORY_MAPPED)

    qspi.cpp (libDaisy)        1.308 linhas
    stm32h7xx_hal_qspi.c (ST)  2.666 linhas
    ITCM disponível               64 KB

A cadeia **atravessa a fronteira da libDaisy e entra no HAL da ST**. Mover o
`EraseSector` sem o `HAL_QSPI_Command` não é solução incompleta: é falha
garantida no pior instante — a busca de instrução morre exatamente quando o
modo muda. E levar tudo é carregar ~4.000 linhas de terceiro para dentro de
64 KB, com fronteira que se move a cada atualização.

**O critério é superfície de dependência, não linhas escritas.** A rotina
própria é a que mais escreve e a que menos herda: fecho transitivo **zero**.

---

## O inventário de ISRs, e ele decide a forma da janela

Medido no `.map` e no `.elf` de `9f47efb`, BOOT_QSPI:

| o que | onde |
|---|---|
| `.isr_vector` — **a tabela de vetores** | **`0x90040000` — QSPI** |
| 51 handlers, incluindo `SysTick_Handler`, `DMA1_Stream0/1`, `OTG_FS`, `QUADSPI` | `0x9005xxxx` — QSPI |
| `HardFault_Handler` | `0x90053ef0` — QSPI |

**Isto resolve a questão por necessidade, não por escolha.** Durante a janela
a QSPI não é memory-mapped. Qualquer interrupção é fatal **antes de chegar ao
handler**: o processador busca o vetor em `0x90040000+offset`, que é
exatamente o que saiu do ar. Não existe lista de exceções possível — o
mecanismo de despacho inteiro está na memória apagada.

> **`__disable_irq()` na janela inteira é obrigatório.** A resposta mais
> simples venceu a mais fina porque a mais fina não existe.

**Nota sobre o `HardFault_Handler`:** ele também mora na QSPI. Se algo faltar
durante a janela, não há para onde ir — o processador entra em lockup. Isso
não é argumento contra o desenho; é o motivo de a rotina ter que ser
**inteiramente** ITCM, sem uma única chamada para fora.

## Parar o áudio: os dois lados, e a libDaisy mata os dois

`DaisySeed::StopAudio()` → `AudioHandle::Stop()` → `SaiHandle::StopDmaTransfer()`
→ `HAL_SAI_DMAStop()`, que faz **`SAI_Disable`** e **`HAL_DMA_Abort`** nos
streams de Tx e Rx.

Então a API cobre os dois atos distintos: para o periférico **e** aborta o
DMA. Sem isso, o DMA continuaria transferindo sozinho durante a janela — não
é fatal (DMA não busca instrução) mas deixaria a última meia-volta do buffer
em laço, e interrupções pendentes disparariam no instante em que
`__enable_irq()` voltasse.

---

## A sequência

```
save <n>                            // gesto de bancada, por serial
  1. hal::StopAudio()               // SAI parado + DMA abortado
  2. __disable_irq()                // a tabela de vetores esta na QSPI
  3. rotina ITCM:                   // fecho transitivo ZERO
       - QSPI -> modo indireto
       - le/apaga/escreve
       - QSPI -> memory-mapped
  4. __enable_irq()
  5. hal::StartAudio()
  6. sync                           // o mundo confere: re-push do espelho
```

**Tudo entre 2 e 4 roda de ITCM, sem uma única chamada para fora.** Nenhuma
função de biblioteca, nenhum `Log`, nenhuma variável em QSPI. É <100 ms, é
bancada, e é surdo por construção.

## O PROTOCOLO REAL — lido no `Init` da libDaisy, 2026-08-22

**Este é o resultado mais valioso da rodada, e é conhecimento, não código.**
Está aqui porque a próxima tentativa pode travar a placa, e o achado tem que
sobreviver à sessão.

**A libDaisy NUNCA entra em quad:**

```cpp
// These functions are defined, but we haven't added the ability to
// switch to quad mode. So they're currently unused.
QSPIHandle::Result EnterQuadMode() __attribute__((unused));
```

O chip está em **SPI simples**. Isso **refuta** a hipótese que explicava o
`FF FF FF` (chip em QPI ignorando comando de 1 linha) — hipótese plausível,
que batia com o sintoma, e errada (bts 13j).

**O formato da leitura memory-mapped, que é o que o chip entende hoje:**

| campo | valor |
|---|---|
| `InstructionMode` | **1 LINHA** |
| `Instruction` | `0xEB` — quad I/O fast read |
| `AddressMode` | **4 linhas** |
| `DataMode` | **4 linhas** |
| `DummyCycles` | **6** |
| `AlternateByteMode` | **4 linhas** |
| `AlternateBytes` | **`0xA0`** |
| `AlternateBytesSize` | 8 bits |
| `SIOOMode` | **`INST_ONLY_FIRST_CMD`** |

🔴 **AS QUATRO ÚLTIMAS LINHAS FALTAVAM ATÉ 2026-09-29, e eram as que decidiam.**
Com `AlternateBytes = 0xA0` os bits M5-M4 valem `10`, o que põe o chip em
**leitura contínua (XIP)** — e nesse modo **o chip deixa de interpretar o
primeiro byte como opcode**. Foi por isso que, durante cinco semanas, nenhum
comando indirecto funcionou: não estavam errados, não estavam a ser ouvidos.
Ver bts 15fw e 15fy. A tabela acima é agora a estrutura COMPLETA lida em
`EnableMemoryMappedMode`.

Não é SPI nem QPI: é **híbrido**. Só a instrução vai serial; endereço e dado
vão em quatro linhas. Qualquer comando novo nasce sabendo disso.

**O chip que a libDaisy supõe** aparece num comentário comentado ao lado dos
dummies: `IS25LP080D_DUMMY_CYCLES_READ_QUAD` — um IS25LP da ISSI. **Suposição
da biblioteca, não leitura** — daí o JEDEC ID continuar valendo.

**E o `0x88` deixou de ser mistério:** são dois nibbles `1000` — uma linha
alta e três baixas. É a assinatura de comando em quatro linhas chegando num
chip que só escuta a IO0.

**Próxima tentativa, com o protocolo conhecido em vez de suposto:** `0x9F`
com instrução e dado em **1 linha**, **zero** dummy, e o **`SSHIFT` do `CR`
conferido** — com prescaler alto, a amostragem de meia-fase é o que separa
`03 07 0F 1F` de um ID legível. Uma variável.

## O chip nasce anônimo

A primeira coisa que a rotina lê é o **JEDEC ID (0x9F)** — três bytes,
fabricante e densidade.

Isso não é um pré-requisito da rota: **é o primeiro incremento dela.** A
libDaisy não expõe leitura de ID e o handle do periférico é privado, então
ler o ID exige exatamente o mesmo acesso de registrador que escrever, menos o
apagamento. É o "olá mundo" da infraestrutura: valida ITCM, mudança de modo e
retorno ao memory-mapped **com o único comando que não pode estragar nada**.

O ID entra no boot-report, e chip desconhecido é **recusado gritando** em vez
de escrito com a tabela de comandos errada — a família 3 fechada por desenho,
agora no nível do silício.

## Onde os presets moram

A decidir com o mapa na mão. O que já se sabe:

- `QSPIFLASH` do app: `0x90040000`, 7936 KB. O binário usa ~189 KB.
- Os primeiros 256 KB do chip (`0x90000000`–`0x9003FFFF`) estão **fora** da
  região do app em BOOT_QSPI — área do bootloader. **Não escrever lá:**
  arrisca brickar até o BOOT+RST.
- Erase é por **setor de 4 KB**. Os 8 slots vivem num setor só, logo todo
  `save` é **ler os 8, alterar 1, regravar o bloco**.
- +4 bytes de contador de gravações no cabeçalho do setor: a flash aguenta
  ~100k ciclos e o save-por-comando nunca esgota, mas o contador transforma
  "será que gastou?" de mistério em leitura — daqui a dez anos, numa galeria.

**A garantia é de linker**, não de convenção — mesma jogada do portão da
SDRAM: clandestino nem linka. O `.lds` forkado com certidão de origem já
existe no ramo `investigacao-boot-sram` e é reaproveitável.

## Aceite

1. **JEDEC ID lido e a placa sobrevive** — valida o mecanismo inteiro sem
   escrever nada.
2. **ID no boot-report**, e chip desconhecido recusado com log.
3. `save 1` / `load 1` sobrevivendo a **desligar da tomada**, não só a reset.
4. `list` idêntico antes e depois de um `save` — a escrita não mexe em estado.
5. Preset de mapa velho **recusado gritando** no boot, não carregado torto.
6. Contador de gravações subindo, e visível.

## O que ainda não sei

- **Qual chip.** Por desenho: o ID é a primeira leitura.
- **Se o erase de 4 KB cabe confortavelmente na janela** — depende do chip e
  fica sabido depois do ID.
- **Se `__disable_irq()` por ~100 ms tem efeito colateral** no USB CDC (a
  enumeração pode cair e voltar). Bancada, aceitável, mas a ser observado.

**E o registro que fecha a rota do BOOT_SRAM:** dezenas de `SetMode`
sobrevivem em todo boot — quase certamente a I-cache segurando a rotina
curta. Mas *rotina curta sobrevive* não escala para *apagamento de 4 KB
sobrevive*: o erase é longo, o polling é laço, e laço longo é onde a I-cache
eventualmente perde uma linha e busca na QSPI morta. **ITCM é a diferença
entre funciona-por-sorte e funciona-por-construção.**

---

# Onde a frente parou (2026-08-22) — e como retomar

**Parada por dimensionamento, não por desistência.** O risco arquitetural
acabou; o que resta é sequência de registrador, que é trabalho de leitura.

## O que ficou PROVADO

1. **A janela ITCM funciona.** A placa sai do memory-mapped executando de uma
   RAM independente, dialoga com o periférico, volta e sobrevive. Reproduzido
   em seis gravações. `ITCMRAM: 212 B` é a prova de que a seção foi aplicada.
2. **O protocolo real está lido no fonte**, não suposto — ver a seção do
   `Init` acima. A libDaisy **nunca** entra em quad.
3. **A sonda varre sem gravar**: `qspi id [dummy] [modo] [opcode]`.
4. **`00 00 00` distingue-se de `FF FF FF`** no relatório, com a
   interpretação etiquetada ao lado. Informação de diagnóstico que não
   existia antes — ver cicatriz 13k.

## ACHADO DE PROTOCOLO: sair do memory-mapped exige ABORT

Desligar o `QUADSPI_CR_EN` **não basta**. O periférico guarda estado da
configuração mapeada — contador de timeout, `TCEN`, transação em curso — e
com esse estado de pé a linha de dado fica presa. O sintoma foi ler
`00 00 00` até no `0x06` (WREN), que **não tem fase de dado**.

    QUADSPI->CR |= QUADSPI_CR_ABORT;
    while(QUADSPI->SR & QUADSPI_SR_BUSY) { }   // com teto de voltas
    __DSB();
    LimpaFlags();
    QUADSPI->CR |= QUADSPI_CR_EN;

A libDaisy faz isto por dentro do `SetMode`. **Rotina própria não herda nada
— tem que fazer explícito**, e este é o primeiro item dessa conta.

*(Veio de um chute, e está registrado como achado assim mesmo. Ver 13l sobre
por que um chute que acerta parcialmente é o mais perigoso.)*

## AS TRÊS HIPÓTESES VIVAS — nomeadas, não testadas

O gabarito é `QSPIHandle::Impl::GetStatusRegister()` em
`libDaisy/src/per/qspi.cpp:1045`, que faz uma leitura indireta de 1 byte
neste chip, com este periférico, e **funciona**. Ele difere da rotina própria
em três pontos, e qualquer um deles explica o resultado atual:

1. **Dois passos vs um.** O HAL chama `HAL_QSPI_Command` e **depois**
   `HAL_QSPI_Receive`; a rotina própria configura `CCR` com `FMODE=01` numa
   escrita só. A ordem de escrita do `CCR` pode ser o que dispara a leitura.
2. **`NbData` / `DLR`.** O gabarito pede **1 byte** (`NbData = 1` → `DLR = 0`);
   a sonda pede 3 (`DLR = 2`). Comprimento errado pode deixar a transação
   pendurada.
3. **Ordem de escrita do `CCR`** em relação a `DLR` e `AR`. No QUADSPI a
   escrita do `CCR` é o gatilho quando não há endereço — se `DLR` for escrito
   depois, chegou tarde.

**Não são independentes o bastante para bissecção cega.** Testar uma custa
uma gravação e, se falhar, não elimina as outras duas: sem o manual não se
distingue "a hipótese está errada" de "a execução da hipótese está errada".

## CONDIÇÃO DE RETOMADA

> **RM0433 (STM32H7 reference manual), capítulo QUADSPI, seção de leitura
> indireta.** Com ele aberto, a sequência é **leitura**, e as três hipóteses
> viram uma resposta.

**O critério, escrito para quem retomar não confundir pausa com abandono:**
*meia hora com o manual vale mais que dez gravações adivinhando.* Sem o
manual, a frente fica parada — e ficar parada aqui é a decisão certa, porque
o difícil já está resolvido.

## Depois que o ID ler

A ordem não mudou: ID → recusa gritada de chip desconhecido → erase → program
→ `save`/`load` sobrevivendo a desligar da tomada.


---

# ✅ RESOLVIDO — 2026-09-29. A QSPI escreve com o app a correr da QSPI.

Este documento esteve cinco semanas com a frente parada e a condição de
retomada a apontar para o RM0433. **O manual não foi preciso.** O que resolveu
foi ler três funções da libDaisy — `SetMode`, `HAL_QSPI_Abort` e
`EnableMemoryMappedMode`.

## As três hipóteses vivas: mortas, e nenhuma era a causa

| hipótese de 2026-08-22 | veredicto de 2026-09-29 |
|---|---|
| dois passos vs um (`Command`+`Receive`) | **irrelevante** — um passo funciona |
| `NbData`/`DLR` | **irrelevante** — 1 ou 3 bytes, igual |
| ordem de escrita do `CCR` | **real mas secundária** — é `DLR → CCR → AR` |

A causa não estava na lista, porque a lista foi feita a olhar para o COMANDO e
as duas causas estavam na JANELA e no MODO DO CHIP.

## O que ficou provado, com números

```
qspi janela  ->  CR.EN=0 derruba o BUSY em 0 voltas
                 CR.ABORT nao o derruba em 200 000, quatro gravacoes seguidas
qspi id      ->  9D 60 17    ISSI · IS25LP · 8 MB
qspi sr      ->  0x40        WIP=0 WEL=0 QE=1
qspi ler     ->  16/16 iguais ao memory-mapped
qspi prova   ->  apagar · conferir FF · programar · reler: 0 de 32 diferem
```

## A sequência que funciona, e a ordem é tudo

1. `CR.EN = 0` — **desligar**, não abortar. Reinicia máquina de estados e FIFO.
2. esperar o `BUSY` cair (cai imediatamente)
3. `CCR = 0`; `CR` reconstruído **sem `TCEN`** (o contador de timeout é do modo
   mapeado e é ele que segura o `nCS`)
4. `CR.EN = 1`
5. **sair da leitura contínua**: uma leitura `0xEB` com `ABR = 0x00`
6. só então o comando, com `DLR → CCR → AR` (o `AR` é o gatilho)
7. ao fechar: **restaurar o `ABR`**, senão o app não busca instruções e morre

## A tabela de comandos, provada nesta placa

Tudo em **uma linha**, **zero dummy**, endereço de **24 bits** — o chip está em
SPI simples, a libDaisy nunca chama `EnterQuadMode`:

| comando | opcode | instr | endereço | dado | dummy |
|---|---|---|---|---|---|
| WREN | `0x06` | 1 | nenhum | nenhum | 0 |
| RDSR | `0x05` | 1 | nenhum | 1 | 0 |
| apagar bloco 64K | `0xD8` | 1 | 1 | nenhum | 0 |
| programar página | `0x02` | 1 | 1 | 1 | 0 |
| ler | `0xEB` | 1 | 4 | 4 | 6 + `ABR=0x00` |

## ⚠️ E a razão de este driver existir, escrita na biblioteca

```cpp
QSPIHandle::Result QSPIHandle::Impl::CheckProgramMemory()
{
    if(System::GetProgramMemoryRegion() == System::MemoryRegion::QSPI)
        return Result::ERR;      // recusa apagar/programar
}
```

Todo caminho de escrita da libDaisy começa por aqui. **A biblioteca recusa-se a
escrever na QSPI quando o programa corre da QSPI** — que é exactamente a nossa
configuração. Não é limitação a contornar por descuido: é a declaração de que
quem escreve tem de correr de outro sítio, e é o que o `qspi_raw` faz (ITCM,
interrupções mascaradas).

## Limitação que o OTA tem de respeitar

O apagar corre com as interrupções mascaradas e leva centenas de milissegundos.
**Não pode ser chamado com o instrumento a tocar.** A gravação tem de ter o som
desligado — decisão de desenho, não detalhe de implementação.

## Onde continuar

O escolhedor (`ppc/006` fase 3) pode agora ser construído sobre mecanismo
provado. A zona segura do driver começa em `0x90400000` (rascunho) e é
`AlargaZona()` que a abre — deliberadamente, para que ninguém apague o app por
engano numa chamada distraída.
