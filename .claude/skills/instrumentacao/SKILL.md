---
name: instrumentacao
description: Regras para escrever ou alterar código de instrumentação no Cotucatronics — log, caixa-preta, contadores, sondas, mscope. Use SEMPRE que for adicionar ou mexer em qualquer coisa que meça, registre ou reporte. Oito casos provam que instrumentação falha como código de produção.
---

# Instrumentação — a família de bug mais repetida do projeto

**Instrumentação é código de produção e falha como código de produção.**
Oito casos em `docs/battle_scars.md` cap. 2, cometidos inclusive *depois de
nomeada* a família. Parte do IFÁ10000.

## A escalada, para saber o que está em jogo

- Casos 1–7 estragaram **números** (um pico perdido, um perfil sobrescrito).
- Caso 8 estragou uma **conclusão**, e a conclusão foi **publicada** no
  battle_scars como causa confirmada de uma falha. De lá ela voltaria como
  premissa da sessão seguinte.

Instrumentação quebrada não para na planilha.

## Regras duras

1. **Dado de evento único grava uma vez e trava.** Não atualiza. A janela de
   graça zerava o medidor e apagava o pico de estreia — o dado que ela existia
   para revelar.
2. **Estado vence contagem.** Um token de voo (`in_flight`) é imune ao problema
   do armamento; contadores pareados não são — já produziram álibi falso.
3. **A sessão de leitura não escreve nada.** Contadores `static` zeram no boot:
   se a leitura escreve, você mediu a leitura, não o voo.
4. **Toda escrita de instrumentação passa pelo mesmo portão** (`bb_rec`). Um
   marcador fora do portão contamina a cena do crime.
5. **Armar por comando explícito no início do voo**, nunca no meio de um fluxo.
6. **A medição não pode alterar o que mede.** Se puder, prove que não alterou
   (voo enxuto de controle) antes de confiar no número.

## Regra do caso 9 — instrumentação que AGE

A escalada da família, completa: **números → conclusões → ações.**
Casos 1–7 corromperam *medições*. O caso 8 corrompeu uma *conclusão*, e ela
foi publicada. O caso 9 **agiu**: um watchdog leu 4 277 071 629 µs (subtração
estourada), tratou como estouro de prazo e **desligou um efeito saudável** que
gastava 35 µs de 500.

> **Um instrumento que só observa erra e alguém percebe. Um instrumento que
> age erra e altera o sistema antes de alguém perceber.**

**Se o mecanismo INTERVÉM (veto, bypass, mute, reset automático):**

1. **Teste de plausibilidade antes do gatilho.** Amostra fora de faixa
   física é **descartada e contada**, nunca julgada.
2. **O contador de descartes aparece no status.** Senão o descarte vira outro
   silêncio, e você troca um erro visível por um invisível.
3. **A ação é permanente até rearme explícito.** Desfazer sozinho transforma
   defeito reprodutível em intermitente — e intermitente é o mais caro de
   todos.

## Regra do filtro — "implausível" não se define por magnitude

Se você filtrar outliers por tamanho, **descarta a catástrofe junto com o
ruído**: um estouro real de 5 ms parece exatamente um erro de relógio.
Ajustar o limiar só move a cegueira — do outro lado do corte há sempre uma
catástrofe do mesmo tamanho.

**Outlier reprodutível é bug achável.** Em 18/08 os outliers do relógio eram
`System::GetUs()` dividindo antes da subtração: o valor devolvido só usa 1/200
da faixa de 32 bits e dá a volta a cada ~21,5 s, então a subtração `uint32`
não corrige. Conserto: medir em **ticks crus**, converter **depois** da
subtração — e aí o filtro deixa de ser necessário.

**Antes de filtrar, pergunte:** *por que este número existe?* Filtro é o que
se faz quando a causa é desconhecida **e** o custo de investigar é alto; nos
dois outros casos é um curativo que esconde.

**Corolário para atuadores:** veto automático exige relógio **monotônico e
confiável**. Sem isso, o caso 9 está armado para reincidir.

## Regra do observador — o diagnóstico não pode depender do paciente

Serial, MIDI e caixa-preta passam todos pelo loop principal. Quando ele
morreu, **os três calaram juntos** e nenhum pôde relatar. Uma hora de
hipóteses sem observação por baixo, todas erradas.

> **Um instrumento de diagnóstico não pode depender do que ele diagnostica.**

**Ao criar qualquer canal de diagnóstico, declare de que contexto ele
depende.** Se todos dependem do mesmo, você tem um ponto único de cegueira —
e ele falha exatamente quando é mais necessário. O LED dirigido pelo callback
de áudio existe por isso: o observador vive no contexto que sobrevive.

**Sintoma diagnóstico de segunda ordem:** quando vários canais calam ao mesmo
tempo, isso não é coincidência — é a assinatura **do que eles têm em comum**.

## Regra do caso 8 — modos que silenciam

**Todo modo que silencia ou altera o instrumento tem que se declarar em todo
comando de status.** `board`, `midi` e `mscope` imprimem `[LOG MUDO]`.

E o ponto fino de desenho:

> **A declaração tem que ser imune ao modo que declara.**

Um banner impresso pelo `Log()` normal morre mudo junto com todo o resto. Por
isso existe `hal::LogAlways()`, que atravessa o mudo, e **só o banner o usa**.
Antídoto que pega a mesma doença não é antídoto.

**Aviso que aparece uma vez não é aviso; é sorte.** O `bb arm` avisava
`"log mudo ate o proximo reset"` — uma vez, no armamento. Ninguém leu.

Mesmo antídoto da cicatriz 7 (o rótulo `"transições: NENHUMA"` impresso com
todos os contadores zerados): **o instrumento declara os próprios modos.**

## Ao criar um canal de entrada novo

**Instrumento que só diz "não chegou" precisa de um par que diga "o que eu
mandei, chegou".** O `mscope` mede o que entra e nunca distingue "a fonte não
mandou" de "mandou e se perdeu". O loopback é o par dele: fonte conhecida,
caminho real, resposta binária. **Todo canal de entrada nasce com o seu.**

## Ao terminar

Perguntar: *"se esta instrumentação estivesse quebrada, o resultado seria
distinguível de um resultado real?"* Se não for, ela ainda não está pronta.
