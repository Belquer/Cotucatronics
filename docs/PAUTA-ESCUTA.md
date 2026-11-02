# Pauta da sessão de escuta

**Por que existe:** os itens estavam espalhados pelos alinhamentos das
últimas sessões. Sem lista, a sessão julga o que lembrar em vez do que deve —
e ~20 versões de firmware seguem sem veredito de ouvido.

**Quando:** é o passo 2 do plano dos presets, e vem **antes** de congelar o
slot 0. Salvar um estado de boot sobre parâmetros não-julgados é fotografar
uma dívida e chamá-la de referência.

---

## 1. Drive — o topo do dial de sujeira

`drv_sujeira` (CC 36) foi aprovado **só em 0**, que é o tanh tube. O
wavefolder no topo nunca passou pelo ouvido — só por medição (THD 17,9% →
58,9% no pico, 8× mais energia acima do 7º harmônico, aliasing contido a
−65 dB pelo passa-baixa fixo de 2 kHz).

**Julgar:** o topo serve? Onde no curso ele para de servir? A faixa entre 0
e 1 tem resolução útil ou o interessante está espremido?

## 2. Shimmer religado, com `shm_shift` conferido

Em 2026-08-22 o `shm_shift_a` estava em **0,00** — shimmer sem transposição soa
como reverb realimentado, não como shimmer. É possível que boa parte do
julgamento anterior do shimmer tenha sido feita nesse estado.

**Julgar:** com transposição de verdade, a cauda serve? O `shm_tone`
(corte + sino de −6 dB em 3 kHz, escolhido contra a razão medida da cauda
seca) resolveu a aspereza que gerou o "ainda ardido"?

## 3. Delay seco

Reescrito em C++ em 2026-08-20, com piso de 50 ms e feedback 0,4 lido do patch
original. **Nunca foi julgado sozinho** — sempre soou dentro de uma cadeia.

**Julgar:** o piso de 50 ms é curto o bastante? O feedback fixo em 0,4 pede
controle?

## 4. Os dois espaços com material real

A cauda paralela com espaço A e espaço B independentes foi construída para
"corda com shimmer, couro com reverb limpo". Medida (490 µs na config alvo),
nunca tocada com os dois materiais ao mesmo tempo.

**Julgar:** a separação faz o que prometia? Os `rev_mix` por cadeia dosam
como se espera?

## 5. Gate, warble, freeze duplo, búzios

Os que passaram por reescrita ou restauração e não têm veredito:
`Gate::Abre()` (retomada aberta), warble com mix, os dois freezes
independentes, e o detector de búzios restaurado dos mortos.

---

## O que a sessão NÃO deve fazer

**Não ajustar enquanto julga.** Julgar é dizer se serve; ajustar é outra
sessão. Misturar os dois produz "melhorou" sem referência.

**Não deixar o Code instrumentando durante a escuta.** Custou dois
diagnósticos falsos em 2026-08-22: scripts carregavam uma "base" inteira a cada
medição e sobrescreviam ajustes do dB no meio da tocada (bts 12l).

**Declarar o regime de CPU antes.** Com tudo ligado o pico está em ~940 µs
de um prazo de 1000 (bts 12u). Se a sessão usar configuração pesada, um
estouro de bloco é uma explicação possível para artefato ouvido — e não deve
ser confundido com defeito de efeito.
