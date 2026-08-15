# modules/ — manual de embarque de módulos RNBO

**Documento v0.2.0** · receita validada em placa com o módulo `delay`
(RNBO 1.4.3 / Max 9.1.4, Daisy Seed 2, firmware v0.5.0)

Este é o **template de toda escultura futura**. Quem for embarcar o segundo
módulo não precisa redescobrir nada: seguindo os sete passos abaixo, o export
compila e roda. Fora deles, o RNBO não entra num alvo bare-metal sem exceções,
sem `malloc` e com 128 KB de flash.

---

## Layout

```
src/modules/
  README.md              <- este arquivo
  <nome>/
    rnbo_export/         <- saída crua do Max. NÃO editar à mão.
      <export_name>.cpp  <- o patcher gerado
      description.json   <- contrato: params, inlets, outlets
      presets.json
      dependencies.json
      rnbo/              <- a biblioteca do RNBO (vem do "copy code library")
```

> **`rnbo_export/` é gerado, não é nosso.** Reexportar do Max sobrescreve tudo.
> Toda a adaptação vive em `src/module_rnbo.cpp` — nunca dentro do export. Se
> você se pegar editando o `.cpp` gerado, a receita está errada em algum ponto.

---

## Passo 1 — configurações de export no Max

Na sidebar do objeto `rnbo~`, **Export → C++ Source Code Export**:

| Configuração | Valor usado | Por quê |
|---|---|---|
| **Export Name** | `cotucatronics_delay` | vira o nome do `.cpp`. Prefixe com o projeto pra não colidir. |
| **Class Name** | **um por módulo** (`rnbomatic_delay`, `rnbomatic_verb`, …) | ver o alerta abaixo — é a configuração que mais dói se esquecer. |
| **Output Directory** | `src/modules/<nome>/rnbo_export` | |
| **Copy C++ Code Library Dependencies** | ✅ **ligado** | copia a pasta `rnbo/` junto. Sem isso o export referencia uma instalação do Max que não existe na máquina de build, e o `-I` do Makefile aponta pro vazio. É o que torna o repositório autocontido. |
| **Fixed Vector Size** | *vazio* | |
| **Fixed Sample Rate** | *vazio* | |

**Vector size e sample rate ficam sem fixar de propósito.** Fixar hardcoda
48000/48 dentro do código gerado; deixando livre, o patcher se adapta no
`prepareToProcess(sample_rate, block_size, true)` com os valores que o
`hal::Info()` reporta. O mesmo export passa a servir qualquer placa e qualquer
block size — que é exatamente a promessa da fronteira HAL. Custa uns poucos
ciclos de indireção e paga em portabilidade.

> ### ⚠️ Class Name: a colisão que espera o módulo #2
>
> O export do `delay` saiu com o **classname padrão `rnbomatic`** — foi o que
> validamos, e com um módulo só funciona. Mas o `.cpp` gerado é incluído **como
> fonte** (passo 4), e ele define no `namespace RNBO` global:
>
> - `template <class ENGINE> class rnbomatic`
> - `creaternbomatic()`
> - `rnbomaticFactoryFunction()` / `rnbomaticSetLogger()`
>
> Dois exports com o mesmo classname na mesma build = **redefinição, erro de
> link**. E não dá pra contornar embrulhando o include num namespace nosso: o
> arquivo gerado abre `namespace RNBO { … }` por dentro, e embrulhar moveria
> junto o `RNBO::Platform` do alocador — o hook de memória do passo 5 deixaria
> de casar, calado.
>
> **Portanto: todo export novo sai com Class Name próprio.** E na próxima
> reexportação do `delay`, aproveite e dê um a ele também.

Confira o `description.json` depois de exportar — ele é o contrato:

```json
"parameters": [ {"paramId": "dly_time", "minimum": 0, "maximum": 1, …},
                {"paramId": "dly_mix",  "minimum": 0, "maximum": 1, …} ],
"inlets":  ["signal"],
"outlets": ["signal", "signal"]
```

Dois hábitos que economizam depuração:

1. **Nomeie os parâmetros no Max com o nome que o manifest vai usar**
   (`dly_time`, não `param_1`). A ligação é por nome (passo 6).
2. **Deixe todos os parâmetros em `0–1`.** Aí a `ParamTable` normalizada entra
   direto no `setParameterValue()`, sem tabela de conversão. Escalonamento pra
   Hz/segundos/dB é papel do patch ou do manifest, não do módulo.

## Passo 2 — as três macros obrigatórias

Vão em `src/module_rnbo.cpp`, **antes de qualquer `#include` do RNBO** — são
lidas pelos headers, então chegar depois é o mesmo que não existir:

```cpp
#define RNBO_NOTHROW            // a libDaisy compila com -fno-exceptions; sem isto o RNBO faz throw e o compilador recusa
#define RNBO_USE_FLOAT32        // SampleValue vira float (padrão é double) e casa com os buffers do hal::AudioCallback
#define RNBO_USECUSTOMALLOCATOR // tira o RNBO do malloc da libc e entrega a alocação pra gente (passo 5)
```

Detalhando o porquê de cada uma:

- **`RNBO_NOTHROW`** — a build da libDaisy passa `-fno-exceptions`. Sem a macro,
  o RNBO tenta `throw std::runtime_error` e a compilação morre. Com ela, os
  caminhos de erro viram retorno/abort em vez de exceção. Não é opcional em
  bare-metal.
- **`RNBO_USE_FLOAT32`** — por padrão `SampleValue` é `double`. Com float32 ele
  passa a ser `float`, que é exatamente o que o `hal::AudioCallback` entrega: os
  buffers vão direto pro `process()`, **sem conversão e sem buffer
  intermediário**. E float é mais rápido que double no Cortex-M7 mesmo com FPU
  de dupla precisão. Também derruba pela metade a memória das linhas de delay.
- **`RNBO_USECUSTOMALLOCATOR`** — o RNBO deixa de chamar `malloc`/`free` da libc
  e passa a chamar quatro funções nossas em `RNBO::Platform`. Sem isso, a
  alocação cai no heap do newlib, que na Daisy é minúsculo e mal delimitado — a
  linha de delay não caberia, e a falha apareceria como áudio corrompido em vez
  de erro no boot.

Uma quarta, **opcional mas recomendada**:

```cpp
#define RNBO_NO_PATCHERFACTORY  // sem fábrica dinâmica de patchers: não há carregamento em runtime
```

Não há carregamento de patch em runtime aqui — o patcher é estático. A macro
troca os símbolos `extern "C" GetPatcherFactoryFunction`/`SetLogger` (que
colidiriam entre exports) por versões prefixadas pelo classname.

## Passo 3 — a fronteira: um arquivo, e só um

`src/module_rnbo.cpp` é o **único arquivo do projeto que pode incluir headers do
RNBO**. É a mesma regra do `hal_<board>.cpp` com `daisy_seed.h`, aplicada ao
motor de DSP: `main.cpp` não sabe que existe RNBO, e trocar de motor significa
escrever outro `module_<engine>.cpp`.

`module_rnbo.h` — o header público — **não inclui nada do RNBO**, de propósito.
Ele declara só a `Module*` e o extrato de memória.

## Passo 4 — MinimalEngine, com o export incluído como fonte

```cpp
#include "RNBO_Common.h"
#include "RNBO_MinimalEngine.h"
#include "modules/delay/rnbo_export/cotucatronics_delay.cpp"  // <- fonte, não objeto
```

Duas decisões aqui:

**O `.cpp` gerado entra como `#include`, não como unidade de compilação
separada.** Ele define a template `rnbomatic<>` inteira, e template só se
instancia onde é vista. Compilado à parte, ele também **não veria as macros do
passo 2** — cada `.cpp` tem seus próprios defines, e o resultado seria um
patcher com `double`, com exceções e com o `malloc` da libc, linkando contra um
firmware que espera o oposto. Silencioso e horrível de diagnosticar. Por isso o
Makefile lista **`module_rnbo.cpp` e não o export** em `CPP_SOURCES`.

**`MinimalEngine` em vez de `CoreObject`/`RNBOEngine`.** A engine completa traz
scheduler de eventos, MIDI, presets, data refs carregados em runtime e
`std::function` por todo lado — nada disso serve num instrumento que só recebe
`setParameterValue` do loop principal. A `MinimalEngine` é a fatia mínima que o
patcher precisa pra rodar `process()`, e é a diferença entre caber e não caber
nos 128 KB.

Instanciação — **a ordem dos membros importa**, a engine recebe `&patcher_` no
construtor:

```cpp
RNBO::rnbomatic<>     patcher_;
RNBO::MinimalEngine<> engine_{&patcher_};
```

E o objeto do módulo é **estático**, não `new`: vida do programa inteiro, zero
heap.

```cpp
RnboModule g_module;
Module* CreateModule() { return &g_module; }
```

## Passo 5 — o alocador híbrido

Com `RNBO_USECUSTOMALLOCATOR`, o RNBO chama estas quatro funções e mais nenhuma:

```cpp
namespace RNBO { namespace Platform {
  void* malloc(size_t);
  void  free(void*);
  void* calloc(size_t, size_t);
  void* realloc(void*, size_t);
}}
```

A estratégia:

```
              ┌─ cabe nos 32 KB? ──► pool estático em SRAM   (rápido)
  malloc ────►┤
              └─ não cabe ────────► hal::AllocBig()  ──► SDRAM (64 MB, lenta)
```

- **Pool de 32 KB em SRAM primeiro.** Sinais pequenos são lidos amostra a
  amostra e a SDRAM é bem mais lenta que a SRAM interna. O que é pequeno e
  quente fica perto.
- **O que não couber vai pra SDRAM via `hal::AllocBig()`** — tipicamente a linha
  de delay, que é o bloco grande. Nunca `DSY_SDRAM_BSS` direto: `AllocBig()` é o
  **portão único** da regra do [BOARDS.md](../BOARDS.md), e é ele que faz o
  extrato de boot fechar.
- **`free()` é no-op.** É um alocador *bump*: não devolve. Como nada é liberado
  em runtime, isso não vaza de verdade — significa apenas que a memória do
  módulo é vitalícia, que é o comportamento desejado. Sem free não há
  fragmentação, e sem fragmentação não há surpresa de alocação no palco.
- **Header de 8 bytes por bloco.** Cada alocação guarda o tamanho útil nos 8
  bytes que antecedem o ponteiro devolvido, e o total é arredondado pra múltiplo
  de 8. O tamanho existe porque `realloc()` precisa saber quanto copiar — um
  bump allocator não tem tabela pra consultar. Os 8 bytes (em vez de 4) mantêm
  o alinhamento de 8 exigido por `double`/`int64` sem padding extra.

**Por que isso é seguro:** toda a alocação do RNBO acontece no boot, em
`initialize()` + `prepareToProcess()`. No `process()` não há alocação nenhuma —
é isso que torna um bump allocator aceitável num caminho de áudio.

**A falha é detectada no boot, não no palco.** As falhas são contadas, e o
`Init()` do módulo recusa carregar se houve alguma:

```cpp
if(g_stats.failures != 0) return false;  // patcher meio montado: melhor recusar
```

O `main.cpp` trata isso caindo pra passthrough, com log — nunca áudio
corrompido. E o comando serial `mods` imprime o extrato:

```
ram: <N> SRAM (de 32768) + <M> SDRAM, <B> blocos, 0 falhas
```

Se `SRAM` bateu no teto e a `SDRAM` cresceu muito, aumente `kInternalPool`.
Se aparecer `falhas`, a SDRAM acabou — ou o `Requirements` do manifest está
mentindo.

## Passo 6 — ligar no manifest, **por nome**

O manifest **não conhece índice de parâmetro do RNBO**: reexportar o patch
reordena tudo, e um índice fixo passaria a mexer no parâmetro errado sem avisar.
Declara-se o nome; o boot resolve o índice via `Module::FindParam()`.

```cpp
// manifest.h
p_dly_time = t.Add("dly_time", 0.5f, 0.f, 1.f);   // params do export já são 0–1:
p_dly_mix  = t.Add("dly_mix",  0.5f, 0.f, 1.f);   // min/max reais = normalizado

knob[0].AddTarget(p_dly_time, 1.0f);
knob[1].AddTarget(p_dly_mix,  1.0f);

bind[n_bind++] = {p_dly_time, "dly_time"};        // nome conferido contra description.json
bind[n_bind++] = {p_dly_mix,  "dly_mix"};
```

Nome que não existe no export vira `AVISO: param "x" nao existe no modulo` no
boot e índice `-1` — não empurra nada. Erro visível, não parâmetro fantasma.

**Empurrar parâmetro é trabalho do loop principal, nunca do callback de
áudio.** `setParameterValue()` enfileira um evento no RNBO; o `main.cpp` compara
com o último valor e só empurra o que mudou.

## Passo 7 — Makefile

```makefile
MODULE_SRC ?= module_rnbo.cpp
RNBO_DIR   ?= modules/delay/rnbo_export

CPP_SOURCES = main.cpp $(MODULE_SRC) $(HAL_SRC)   # o export NÃO entra aqui

C_INCLUDES += -I$(RNBO_DIR)/rnbo -I$(RNBO_DIR)/rnbo/common -I$(RNBO_DIR)
CPPFLAGS   += -Wno-unused-parameter -Wno-sign-compare
```

Os três `-I` são todos necessários: `rnbo/` tem `RNBO_Common.h`, `rnbo/common/`
tem `RNBO_MinimalEngine.h` e os headers internos, e a raiz do export resolve os
includes relativos do arquivo gerado. Os dois `-Wno-` calam warnings de código
gerado — que não vamos corrigir, porque não editamos o export.

---

## Regra de orçamento de memória

> ### Todo `buffer~` custa **8 bytes por amostra**, e `RNBO_USE_FLOAT32` não muda isso.

A regra mais cara de descobrir tarde. Vale pra dimensionar qualquer módulo
antes de exportar.

`RNBO_USE_FLOAT32` faz o `SampleValue` — os **sinais** que trafegam entre os
objetos do patcher — virar `float` de 4 B. Mas o **armazenamento de `buffer~`**
é outra coisa: o código gerado o instancia explicitamente como `Float64Buffer`,
independente da macro.

```cpp
// no .cpp gerado — repare no 64, e ele não responde a nenhuma macro nossa
this->delaytilde_01_del_buffer = new Float64Buffer(this->delaytilde_01_del_bufferobj);
```

Isso alcança tudo que guarda áudio ao longo do tempo: `delay~`, `buffer~`,
`record~`, linhas de freeze, tabelas de wavetable.

### A conta

```
bytes = próxima_potência_de_dois(segundos × sample_rate) × 8
```

O RNBO **arredonda pra potência de dois**, o que sozinho já pode dobrar a
conta. Somando os dois efeitos, o erro de uma estimativa ingênua chega a 4×.

| Módulo | Ingênuo (s × 48k × 4) | Real | Fator |
|---|---|---|---|
| `delay` 2 s (medido) | 384 KB | **1 048 576 B** (131 072 × 8) | 2,7× |
| freeze 10 s | 1,9 MB | **8 MB** (1 048 576 × 8) | 4,3× |
| wavetable 1 s | 192 KB | **512 KB** (65 536 × 8) | 2,7× |

> A previsão do delay no TESTES.md errou por exatamente 2× justamente por
> supor 4 B/amostra. O medido — `1 048 584` — bate ao byte com
> `131 072 × 8 + 8` de header do nosso alocador.

### Como conferir sem adivinhar

O comando `mods` dá o extrato real do alocador:

```
ram: 2000 SRAM (de 32768) + 1048584 SDRAM, 15 blocos, 0 falhas
```

**Compare com sua previsão antes de aceitar o módulo.** Divergência grande
significa que ou a conta está errada, ou alguma coisa está alocando fora do
portão — e o extrato existe exatamente pra apanhar os dois casos.

Com 64 MB de SDRAM ainda cabe muita coisa, mas o teto é real: **oito** freezes
de 10 s e acabou.

---

## Checklist do módulo novo

- [ ] Exportou com **Class Name próprio** e *Copy C++ Code Library Dependencies*
      ligado, vector size e sample rate **em branco**.
- [ ] Parâmetros nomeados como o manifest vai chamá-los, todos em `0–1`.
- [ ] Export em `src/modules/<nome>/rnbo_export/`, sem edição manual.
- [ ] `RNBO_DIR` no Makefile apontando pro export novo.
- [ ] As três macros antes dos includes em `module_rnbo.cpp`.
- [ ] `Process()` conferido contra `inlets`/`outlets` do `description.json`.
- [ ] `bind[]` no manifest com os nomes exatos do `description.json`.
- [ ] Boot limpo: `0 falhas` no `mods`, sem `AVISO: param … nao existe`.
- [ ] Confirmação sonora antes de dizer que funciona ([TESTES.md](../TESTES.md)).

## Armadilhas, em ordem de dor

| Sintoma | Causa |
|---|---|
| `throw` não compila | faltou `RNBO_NOTHROW` |
| Áudio distorcido / silêncio com o patcher "ok" | macro depois do include: o export compilou com outra configuração |
| Redefinição de `rnbomatic` no link | dois exports com o mesmo Class Name |
| Erro de link em `RNBO::Platform::malloc` | faltou `RNBO_USECUSTOMALLOCATOR`, ou as funções não estão no `namespace RNBO` global |
| Parâmetro não faz nada | nome errado no `bind[]` — procure o `AVISO` no boot |
| Roda na bancada e trava com outro block size | vector size foi fixado no export |
| `falhas` > 0 no `mods` | SDRAM esgotada, ou `Requirements` do manifest mentindo |
