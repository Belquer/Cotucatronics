# Roteiro de revalidação — troca para BOOT_SRAM

**Por que existe:** sem roteiro escrito **antes** de gravar, revalidação vira
"liguei e funcionou". Esta página é o contrato do passo 1.

**Por que a troca:** `PersistentStorage::Save()` tira a QSPI do modo
memory-mapped para apagar, e esse código mora na QSPI. A pré-condição dele é
*"o app não executa da QSPI"*, revogada por `APP_TYPE=BOOT_QSPI` sem nenhum
aviso de compilação (bts 12v). Presets dependem disso.

---

## Antes de gravar — o mapa de memória

Medido em `48832a3`, BOOT_QSPI:

| seção | onde | tamanho |
|---|---|---|
| `.isr_vector` + `.text` | QSPI 0x90040000 | **182 KB** |
| `.data` | SRAM 0x24000000 | 4,7 KB |
| `.bss` | SRAM 0x24001280 | **276 KB** |
| `.sdram_bss` | SDRAM | 64 MB (pool) |

Os 276 KB de `.bss` são dominados por **`reverb_mem` = 220 KB**; depois
`warble_mem` 15 KB e os buffers de áudio da libDaisy, 8 KB cada.

**A conta do BOOT_SRAM:** a região encolhe de 512 para **480 KB** e o `.text`
passa a morar nela.

    276 (.bss) + 5 (.data) + 182 (.text) = 463 de 480 KB

**Sobram ~17 KB, não 300.** A estimativa anterior ignorava que a SRAM já
carregava `.bss`. Cabe, mas a folga é parede declarada — não espaço.

**Duas verificações feitas antes de gravar:**

**A pilha está na DTCM** — `_estack = 0x20020000`, topo dos 128 KB de DTCM,
nos dois linkers. Os 17 KB são 17 KB de verdade, não folga otimista.

**O heap SAI da AXI no BOOT_SRAM.** O linker do BOOT_SRAM manda o heap para
`RAM_D2` (linha 248: `} > RAM_D2`); no BOOT_QSPI ele fica na AXI, em
0x24044900. Um consumidor a menos na região apertada, de graça.

**Se não couber — e o destino certo NÃO é a SDRAM.** O H750 tem **RAM_D2:
256 KB em 0x30008000**, declarada no linker e hoje sem uso pelo app (todo o
`.bss` mora na AXI). É SRAM de verdade, no barramento AHB — mais lenta que a
AXI para o M7, mas **ordens de grandeza mais perto de SRAM que de SDRAM**.
Os 220 KB de `reverb_mem` cabem nela com folga.

A pergunta "o reverb aguenta memória lenta?" quase desaparece com D2 como
destino. **Continua sendo voo separado, depois da base declarada** — mas a
trilha deixa de ser cara.

---

## Depois de gravar — a lista, nesta ordem

Nada aqui é "liguei e funcionou". Cada item tem critério.

### 1. O binário linkou e cabe
`arm-none-eabi-size` e o `.map`. **Diffar os dois `.map`** (antes/depois),
não ler só o novo: símbolo que mudou de endereço aparece no diff e some numa
leitura isolada.
**Critério:** nenhum símbolo saiu da região que devia; total ≤ 480 KB.
Confirmar no diff que o **heap** migrou para RAM_D2 e que a **pilha** segue
na DTCM — se a pilha tiver ido para a AXI, a folga real é menor que a conta.

### 2. Os quatro regimes, média E pico
Refazer com `cpu reset` entre leituras. **É BASE NOVA** — código em SRAM tem
outro perfil de I-cache, e esta plataforma já provou o peso disso no caso do
`-O3`. Esperado melhorar; esperado não é medido.
**Critério:** declarar "referência v0.124+, BOOT_SRAM". **Nenhum delta contra
os números do BOOT_QSPI é atribuível** (12d).

**Nuance que reforça o "esperado não é medido":** com o `.text` na AXI,
**código e dados do reverb passam a disputar o mesmo barramento** — buscas
de instrução e o tráfego dos buffers agora moram na mesma SRAM. A I-cache
mitiga e o esperado segue sendo melhorar sobre a QSPI, mas a melhora pode
não ser uniforme. Se algum regime surpreender, o candidato é R2/R3 — os de
reverb pesado.

### 3. Espelho MIDI
TX de 76 no boot (`midi` → `eco: TX 76`). Janela de autoria pelo teste que a
validou: 4 ciclos de 500 ms num CC com widget → **8/8**; CC sem widget →
**8/8**. Colisão dentro da janela → `cc_janela` incrementa.

### 4. `vistos`
`vistos reset`, varrer uma família, conferir que os CCs aparecem.

### 5. Critério de estouro
`ctos prazo 300` → derruba na ordem declarada, o relatório diz **PRAZO
ARTIFICIAL** e **CAIU por ESTOURO**. `ctos prazo 0` devolve.

### 6. Serial e caixa-preta sob carga
Com o regime pesado no ar: `list`, `ctos`, `midi`, `bb`. **Critério:** o loop
não starva — o serial responde e o LED continua batendo.

### 7. MIDI de volta ao Loopy
Mover um widget e ver o parâmetro mudar; mudar um parâmetro por serial e ver
o widget seguir.

### 8. Áudio
Tocar. Sem clique, sem dropout, sem mudança de caráter que não tenha
explicação. **Este é o único item que o Code não pode julgar sozinho.**

---

## Ao fim — o teto pede RE-EXAME, não mudança automática

Os 850 µs foram escolhidos como orçamento sobre o perfil de execução da
**QSPI**. Se a SRAM mudar os picos de verdade, 850 pode ficar frouxo ou
apertado demais em relação à nova distância até a parede.

**A pergunta:** *o orçamento administrativo ainda administra bem a distância
até o prazo?*

A resposta vai etiquetada como **escolhida sobre medição**, na forma que o
passo 0 padronizou — e o valor antigo fica registrado com o motivo da
mudança, se houver.

## O boot-report ganha a parede

480 KB é limite novo, e ninguém deve descobri-lo batendo nele. O boot passa a
declarar **"binário X de 480 KB"**.
