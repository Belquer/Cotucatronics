# PRESETS — desenho aprovado

> **Estado: passos 1 e 2 IMPLEMENTADOS (23/08, v0.135.0); passos 3–6
> desenhados.** Aprovado pelo dB em 22/08.
>
> O passo 2 saiu em **dois voos separados**: A, o endereçamento de
> entrada por canal; B, o espelho para canais próprios (que na época
> foram ch3/ch4 e viraram ch4–ch6 na simetria de seis). Separados porque falham
> de jeitos diferentes e o `vistos` — o instrumento do aceite — estava
> no meio da mudança que o afeta.
> A escrita em flash está bloqueada (ver `DESENHO-ESCRITA-QSPI.md`); o
> desenho abaixo **não depende dela** para começar.

## A decisão que reorganiza tudo

> **O preset vira o centro da experiência do usuário.**
> Os params não somem — eles deixam de ser a interface.

Quem toca escolhe entre *"corda-shimmer"* e *"tambor-seco"*. Quem constrói
mexe nos 86. O painel do instrumento mostra **presets**, não parâmetros.

## Uma biblioteca, dois ponteiros

**Não são duas listas.** Se fossem, "corda" existiria duas vezes e as cópias
divergiriam no primeiro ajuste.

```
BIBLIOTECA DE CADEIA   n presets × ~34 bytes   ← ponteiro A, ponteiro B
PRESET GLOBAL          m presets × ~22 bytes   ← ponteiro único
```

Carrega "corda" no A e "tambor" no B. Se um dia houver duas cordas, carrega
"corda" nos dois. **Global slot 0 = estado de boot** — é o que mata o `level`
nascendo em 0 e os diagnósticos falsos que ele causou.

Uma **cena** = global + ponteiro A + ponteiro B.

## Duplicação dos params de caráter

Os **10** params de caráter compartilhados (`drv_drive/tone/sujeira`,
`phs_rate/depth/fb/stages`, `wrb_rate/depth/irreg`) **passam a ser por cadeia**.

> **Correção de contagem (23/08):** este documento dizia 11, e a lista sempre
> teve 10. O 11º de caráter é o `dly_time`, que fica de fora de propósito —
> ver logo abaixo. Logo 76 → **86** params, não ~87. Folga de 10 no teto de 96.

**Custo: zero CPU — CONFIRMADO em 23/08.** Os estados internos já são
separados por cadeia (dois conjuntos de allpass no phaser, dois buffers de
warble) — foi medido quando foram recolhidos: *"encolheu a tabela, não o
trabalho"*.

Medido com A/B **no mesmo binário** (`dup on|off`, que ficou no firmware):
**+6,6 µs contra 14 µs de espalhamento — indistinguível.** A comparação entre
binários teria mentido: no mesmo regime, três builds deram 460 → 437 → 347 µs
só por layout de instrução, porque o código roda da QSPI. Ver a cicatriz 12d.

**`dly_time` fica compartilhado** — é o único cuja duplicação custaria de
verdade (o delay é uma instância só; tempos diferentes exigiriam dois delays,
com memória e CPU reais). Registrado como pergunta separada.

## Multiplexação por canal MIDI

```
ENTRADA   ch1 → compartilhados
          ch2 → cadeia A
          ch3 → cadeia B
ESPELHO   ch4 → compartilhados
          ch5 → cadeia A
          ch6 → cadeia B
RECUSA    ch4–ch6 na entrada (a placa nunca obedece a própria voz)

REGRA: escuta = manda + 3. O número do CC é o mesmo nos seis.
```

**Os mesmos números de CC nos dois canais.** As páginas do Loopy ficam
idênticas — copiar a página e trocar um campo. Total de números de CC cai de
76 para ~45, **mesmo duplicando os params**.

⚠️ *(histórico)* O espelho saiu do canal 2 (que passa a ser entrada da cadeia B) e vai
para ch4–ch6. A defesa de recusar a própria voz se mantém, só muda de número.

**Estado: FEITO (v0.135.0).** O que a implementação acrescentou ao desenho:

- a **chave `espelho on|off`** teve que nascer no voo A, não no B: sem ela a
  recusa incondicional do ch2 tornava a cadeia B inalcançável e o aceite do
  voo A não tinha como rodar. Ficou depois como ganho de teste, desacoplada
  da entrada;
- **`espelho off` não abre a entrada na hora** enquanto os canais se
  sobrepõem — desligar um transmissor não esvazia o fio, e mensagem do
  espelho em voo vira escrita **mal-endereçada** na cadeia irmã. No voo B a
  sobreposição virou impossível por construção (`static_assert`);
- os **números liberados pela cadeia B** (22, 24, 26, 32, 33, 42, 43, 52, 53,
  62, 63, 68–70, 80–88, 106, 116–118) seguem com widgets antigos apontando
  para eles. O `vistos` os nomeia como `ORFAO ch1 ccNN`.

**Consequências de implementação:**
- A **janela de autoria passa a ser indexada por PARÂMETRO, não por CC** — com
  multiplexação, CC 30 significa coisas diferentes conforme o canal.
- O **`vistos` precisa reportar canal** — CC sozinho deixou de identificar param.
- **`ccmap` e docs ganham coluna de canal** — e é o momento de gerar as tabelas
  do manifest (`<!-- gerado: ccmap -->`) em vez de conferir à mão.

## Armazenamento: RAM primeiro, flash depois

A biblioteca vive em **RAM**, e `preset dump` / `preset load` por serial a
persistem **no computador**, como texto. Isso entrega o sistema inteiro hoje —
biblioteca, ponteiros, capturar, recall, navegação no OLED — sem esperar a
flash.

Ao desligar, a placa esquece; recarregar é um comando. Quando a flash
destravar, **a biblioteca só muda de casa** — nenhuma linha do sistema muda.
*Uma variável por vez, e a que está bloqueada não bloqueia as outras.*

## Formato: texto legível, versionado em git

```
# presets.txt — biblioteca de cadeia
1  corda-shimmer   in_gain=64 gate=12 drv_on=0 ... shm_on=1 shm_shift=88
2  tambor-seco     in_gain=71 gate=40 drv_on=1 ... rev_on=1 rev_size=30
```

Reordenar = mover linhas. Renomear = editar texto. Deletar = apagar linha.
**Versionar = git. Backup = grátis. Levar pro próximo instrumento = copiar.**

Mesma filosofia do manifest: *identidade do instrumento é dado, não código*.

## Salvar ≠ capturar

**Escrever na flash para o áudio** (a janela ITCM exige). Então:

- **`capturar`** = instantâneo, em RAM, sem glitch. É o gesto de bancada e
  pode ter botão no Loopy.
- **`gravar`** = escreve na flash, para o áudio, comando explícito.

Um botão de "save" durante a performance produziria corte no som.

## Navegação no OLED

```
PRESET
> A  corda-shimmer
  B  tambor-seco
  G  palco-quente
```

Encoder move o cursor entre as três linhas · **push entra** (o encoder passa a
rolar a biblioteca, o nome muda ao vivo) · **push de novo confirma e carrega**.
Um encoder, sem botão extra, os três estados visíveis de uma vez.

**Pré-escuta ao rolar** (carrega enquanto navega) com **restauração no
cancelamento** — é comportamento de instrumento. Custa uma cópia do estado
anterior ao entrar no modo de edição, e rate-limit no espelho.

---

## O WORKFLOW

```
dB  →  desenha tudo no Loopy Pro (iPad), testa de ouvido, captura o preset
    →  se precisar refazer ou reordenar a lista, faz por texto (dump/load)
usuário → usa SÓ o OLED do instrumento: escolhe preset A, preset B, toca
```

O iPad é a **bancada de construção**. O OLED é a **interface de performance**.
A complexidade existe, mas só quem constrói a encontra.

## Ordem de implementação

1. **Duplicar os params de caráter** (antes da biblioteca, senão ela nasce sem
   poder expressar drive diferente entre corda e tambor)
2. **Multiplexação por canal** + espelho para ch4–ch6
3. **Sistema de presets em RAM** com `dump`/`load`
4. **SESSÃO DE ESCUTA = sessão de construção da biblioteca** — o julgamento
   das ~20 versões *vira* a biblioteca, em vez de ser pré-requisito burocrático
5. **Flash** quando o QSPI destravar
6. **App** só se a prática pedir (navegador com Web Serial, não Python — zero
   instalação; o contra é que não roda no iPad)
