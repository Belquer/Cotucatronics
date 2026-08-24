# Aceite de hardware — multiplexação por canal (voos A e B)

> **Estado (23/08, v0.171.0): o lado do FIRMWARE está VERDE.**
> O que resta é remapear o Loopy — ver
> [RETRABALHO-LOOPY.md](RETRABALHO-LOOPY.md).
>
> | via | prova |
> |---|---|
> | placa → iPad em **ch4** (compartilhados) | *(a confirmar)* |
> | placa → iPad em **ch5** (cadeia A) | MIDI Wrench |
> | placa → iPad em **ch6** (cadeia B) | MIDI Wrench |
> | iPad → placa (fio de volta) | 93 CCs contados pela trava |
> | endereçamento por canal, 37 pares | banco nativo |
> | recusa de ch4–ch6 na entrada | varredura do mapa |
>
> **Os learns do Loopy foram apagados em 23/08**, então o remapeamento é
> do zero e não uma migração de 37 widgets. O mapa inteiro está no
> documento acima.
>
> ⚠️ **Se o Loopy não reagir mesmo com os learns refeitos, não é o WIDI:**
> o Wrench provou a entrega. É a fonte MIDI dentro do Loopy, nas
> preferências dele.

## Por que antes do passo 3

**Preset é fotografia: fotografa o que estiver na frente, inclusive o
defeito.** Capturar sobre um endereçamento não-validado congela o erro *dentro
da biblioteca* — e aí ele deixa de estar num lugar e passa a estar em todos os
presets. Se um widget da cadeia B ainda mandar no ch1, ou se o espelho não
chegar em ch4–ch6, o preset "corda-shimmer" nasce com valores que vieram do
lugar errado.

E o retrabalho do Loopy bloqueia o passo 3.5 (a escuta que constrói a
biblioteca) de qualquer jeito — fazer o aceite agora junta as duas tarefas
humanas em vez de intercalá-las.

## O que já está provado, e por qual instrumento

| afirmação | provado por | onde |
|---|---|---|
| todo par `_a`/`_b` usa o MESMO número de CC | banco nativo, **37/37 pares** | `test_midi` |
| `_a` no ch1, `_b` no ch2, sem exceção | banco nativo, **37/37** | `test_midi` |
| mesmo CC em ch1 e ch2 move params DIFERENTES | banco nativo | `test_midi` |
| nenhum vínculo de entrada nos canais do espelho | varredura do mapa | `test_midi` |
| cada cadeia espelha num canal próprio | `CanalEspelhoDe` | `test_midi` |
| espelho nunca coincide com entrada | **`static_assert`**, exercitado | `midi.h` |
| a placa transmite em ch5 e ch4 | contador `eco: TX` | `midi` |

**O que NÃO está provado e só a bancada prova:** que o TX *chega* no iPad nos
canais certos, e que a entrada funciona **pelo caminho real do WIDI** — não
apenas pela tabela. `voltaram 0` hoje é honesto: o caminho de volta não está
no fio.

## O esquema em vigor — SEIS canais

    MANDA    ch1 compartilhados · ch2 cadeia A · ch3 cadeia B
    ESCUTA   ch4 compartilhados · ch5 cadeia A · ch6 cadeia B

**Escuta = manda + 3**, mesmo número de CC nos seis.

> Este documento descreveu o esquema de QUATRO canais até 23/08 — e o
> roteiro mandava conferir `ch3`/`ch4` quando a placa já falava
> `ch4`/`ch5`/`ch6`. **Documento errado não fica parado: manda alguém ao
> lugar errado com o multímetro na mão** (bts 14h).

## Roteiro

Serial aberta (`./monitor.command`), e `log on` antes de tudo.

**1. TX nos canais certos.** ✅ **FEITO 23/08** — o MIDI Wrench viu os três
valores de `level` (CC 100 = 64, 127, 0) em **ch5**. O caminho
`placa → WIDI → iPad` está provado ponta a ponta.

*(procedimento original abaixo, para repetir quando necessário)*
 No MIDI Wrench, depois de um `sync`:

```
sync
```

Esperado: rajada em **ch4** (compartilhados), **ch5** (cadeia A) e
**ch6** (cadeia B).
Confirmar no `midi` que o TX subiu:

```
midi
```

**2. Injeção nos canais do espelho.** Mandar um CC qualquer pelo ch5 e pelo
ch4 e confirmar que foi **descartado e contado**, não obedecido:

```
midi
```

Esperado: `voltaram` sobe (a placa reconhece como voz própria) e **nenhum
param se move**. Se algum se mover, o `static_assert` foi contornado por
alguma rota nova — é achado grave, parar e reportar.

**3. Entrada pelos dois canais, pelo caminho real.** O mesmo número de CC no
ch1 e no ch2 tem que mover params diferentes. O drive é o mais fácil de ler:

```
set drv_drive_a 1.5
set drv_drive_b 1.5
list
```

Mandar **CC 34 = 30** no **ch1** e **CC 34 = 100** no **ch2**, e reler:

```
list
```

Esperado: `drv_drive_a` e `drv_drive_b` com valores **diferentes**. Este é o
teste que o Code rodou por serial; aqui ele roda pelo WIDI.

**4. Retrabalho do Loopy.**

- widgets da cadeia B passam a **enviar no ch2** (eram ch1, em outros números);
- widgets de escuta migram de ch2 para **ch5** (cadeia A) e **ch4** (cadeia B);
- os órfãos `cc24`, `cc26`, `cc33` são reendereçados — são
  `gate_thresh_b`, `gate_release_b` e `drv_mix_b`, que trocaram de número
  para o do par e mudaram de canal.

⚠️ **Números liberados pela cadeia B, não reaproveitar ainda:** 22, 24, 26,
32, 33, 42, 43, 52, 53, 62, 63, 68–70, 80–88, 106, 116–118. Enquanto houver
widget velho apontando para eles, um número reaproveitado vira widget velho
mexendo em param novo.

**4b. O caminho de VOLTA, e ele é independente do de ida.**

O `sync` agora confere sozinho e reporta. Se der `AVISO`, a linha seguinte
diz o que a placa sabe:

- *"o fio de volta JÁ FUNCIONOU neste voo (N CCs pelo FIO)"* → não é cabo nem
  pareamento; são os widgets, que ainda não escutam ch4–ch6;
- *"a placa NUNCA ouviu o controlador"* → restam duas: widgets no canal
  errado, **ou** o fio `WIDI OUT → D14` não fecha.

**O teste mais barato para separar as duas:** mexa em **qualquer fader** do
Loopy e rode `midi`. A trava acumula desde o boot e não tem relógio — se
`volta:` subir, o fio está bom e o resto é configuração.

**5. Fechamento pelo inventário.**

```
vistos reset
```

Varrer todos os widgets das páginas, e então:

```
vistos
```

Esperado: **86 de 86 vínculos endereçados**, zero `ORFAO`.

## O item aberto: `cc119`

Aparece como `ORFAO ch1 cc119` e **ninguém sabe quem manda**. O teste que
separa as duas hipóteses:

- com o **iPad desconectado**, `vistos reset`, esperar, `vistos` — se o cc119
  aparecer, é interno (algo da própria cadeia de MIDI);
- se só aparecer com o iPad ligado, é **widget esquecido** numa página.

Um comando, duas hipóteses, resposta binária.

## Comandos úteis na bancada

| comando | para quê |
|---|---|
| `pk` | entrada CRUA por canal, pré-ganho — responde "chega sinal?" sem hipótese |
| `vistos [reset]` | inventário por (canal, CC), com órfãos nomeados |
| `midi` | contadores: TX, voltaram, janela, canal sem vínculo, fora do mapa, drenados |
| `board` | versão, tabelas, params sem CC, e todo modo que altera o instrumento |
| `espelho on/off` | cala a saída sem afetar a entrada — mede MIDI sem gerar tráfego |
| `dreno <ms>` | rearma a drenagem de entrada, para exercitar a defesa |
| `dup on/off` | A/B da duplicação no mesmo binário |

## Depois do verde

O passo 3 (presets em RAM) começa sobre endereçamento **provado**, e a escuta
que constrói a biblioteca acontece com a certeza de que cada knob mexe no que
diz que mexe.
