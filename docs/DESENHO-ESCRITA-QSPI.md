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
