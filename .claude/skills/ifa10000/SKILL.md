---
name: ifa10000
description: Protocolo de certificação de todo efeito que entra no Cotucatronics — 10 etapas com critério de passagem explícito, mais a etapa 0 de medição limpa. Use ao embarcar qualquer efeito novo (módulo RNBO ou bloco permanente do shell), ao revalidar um efeito existente, ou quando perguntarem se algo está certificado.
---

# IFÁ10000 — certificação de efeitos

Documento completo: `docs/IFA10000.md`. Histórias que compraram cada etapa:
`docs/battle_scars.md`.

**Todo efeito que entra passa por aqui. Sem exceção, sem atalho.** Efeito que
não passa não entra no build — fica no repo, documentado, com a etapa que
reprovou.

## Auditoria de params — três camadas, quinze minutos

Roda inteira depois de qualquer refactor que toque no callback. Achou uma
feature morta havia cinco commits (cicatriz 12j).

**1. Quem lê cada param?** Compare a lista de `t.Add("nome"` no manifest com
`params.Real(manifest.p_nome)` no `main.cpp`. Param sem leitor é **feature
morta até prova em contrário** — e compila, fica na tabela, e mantém o CC
apontando pro nada.

**2. Faixa declarada × trava interna.** Ver a seção abaixo. Atenção à
DIREÇÃO: faixa MAIOR que a trava é curso morto (defeito); faixa MENOR é
escolha musical (não é).

**3. Varredura viva.** Cada param no mínimo e no máximo, com leitura de volta
da placa. Pega o que o texto não vê: curva errada, clamp no caminho, param
que não chega.

## Antes de tudo: a faixa do param bate com a trava do efeito?

Pergunta de trinta segundos que já pegou um fader com **61% de curso morto**
(cicatriz 12i).

Todo efeito que trava um param internamente — `if(n > hi) n = hi` — cria um
teto que o manifest não conhece. Os dois arquivos ficam corretos em isolamento
e o defeito vive no espaço entre eles: o param guarda 400, o efeito usa 167,
e mais da metade do fader não faz nada.

```
1. procure os clamps do efeito:  grep -n "if(n > \|if(ms > \|> teto" <efeito>.h
2. calcule o teto REAL (pode depender de outro parametro: no freeze o teto
   do grao e um terco do NUCLEO)
3. compare com min/max do t.Add() no manifest
4. faixa maior que a trava -> corte a FAIXA, nao ponha curva
```

**Limite REMOVE o inútil do alcance; curva só o redistribui.** A distinção é
do `dly_time` em 18/08, e ela foi aplicada uma vez e não generalizada — por
isso está aqui, como pergunta de rotina em vez de nota sobre um param.

## ANTES DE TUDO: a caixa única

**Ler `docs/FEATURES.md` inteiro antes de acrescentar qualquer coisa.** Toda
feature vigente está lá com params, CCs e estado; as aposentadas estão no
arquivo do mesmo documento, com *por que saiu* e *como reviver*.

A pergunta que a caixa existe para forçar: ***o que já está aqui que eu
deveria revisar antes de acrescentar mais?***

Custou dois tetos estourados em silêncio no mesmo dia (`kMaxParams` e
`kMaxMidiMap`) — os búzios sumiram do instrumento e ninguém soube até alguém
olhar uma tabela de debug. E feature aposentada sem registro vira folclore:
código que continua lá porque ninguém lembra por quê.

**Aposentou algo? Vai pro arquivo no MESMO commit.**

## Como conduzir

1. Abrir `docs/IFA10000.md` e copiar a **ficha de certificação** para o
   relatório da sessão.
2. Rodar a **etapa 0** antes de qualquer número (skill `medicao-limpa`).
3. Percorrer as 10 etapas **na ordem**. Etapa reprovada interrompe — não se
   pula para a seguinte "pra ver se passa".
4. Preencher a ficha com **números**, não com vistos. `[x] ok` não é resultado.
5. Etapa 10 fecha sincronizando repo **e** vault na mesma ação.

## As etapas, em uma linha cada

| # | Etapa | Passa se |
|---|---|---|
| 0 | **MEDIÇÃO LIMPA** | canal provado, ambiente zerado, unidade e regime declarados |
| 1 | CONTRATO | wrapper e `description.json` contam a mesma história |
| 2 | EMBARQUE | boot limpo, nenhum `-1` em ligação de param |
| 3 | MOTOR *(RNBO)* | param mexido ao vivo muda o som na hora |
| 4 | MEMÓRIA | cabe com folga declarada, região justificada no código |
| 5 | CARGA | pico cabe no prazo com folga ≥ 25%, nos dois blocos |
| 6 | MATERIAL | responde ao sinal certo, e o registro diz qual foi |
| 7 | FAIXA | existe tabela de onde satura, medida antes do limiter |
| 8 | SEGURANÇA | decisão de cada param nos búzios escrita no manifest |
| 9 | ACEITE | o dB ouviu ≥ 1 min tocando a escultura e aprovou |

## Cada param novo precisa provar que MOVE algo

Existir na tabela, receber CC e aparecer no `list` **não é o mesmo** que estar
ligado ao som. Em 18/08 o `shm_mix` fazia as três coisas e não escalava nada —
a função de mistura nunca era chamada. O efeito funcionava, soava bonito, e o
controle girava solto.

**Antes de fechar a etapa 2, para cada param novo:** mexer no extremo e provar
que o som muda. Se não muda, o param está desconectado — e o sintoma que o
músico vai relatar é *"está baixo"*, que manda procurar ganho em vez de fiação.
| 10 | REGISTRO | quem clonar amanhã reproduz sem perguntar nada |

## Skills irmãs — o braço executável do protocolo

| Skill | Quando |
|---|---|
| `medicao-limpa` | antes de **toda** medição (é a etapa 0) |
| `instrumentacao` | ao escrever/alterar log, caixa-preta, contadores, sondas |
| `bug-de-ordem` | quando algo funciona às vezes, ou o teste sai incoerente |
| `edicao-segura` | em **toda** alteração de arquivo |

## Regras que valem em qualquer etapa

- **Pico decide, média mente.** Vocoder: média 7,8%, pico 210%.
- **Estreia é o preço da mudança; regime é o aluguel.** Medir separado.
- **Padrão de acesso importa mais que tamanho.** 108 KB em SDRAM = +12,9 pp
  de CPU; os mesmos em SRAM = +5,6 pp.
- **Régua errada produz "não funciona" falso.** Cada efeito tem seu material.
- **Encanamento provado não é feature entregue.** O aceite é do músico.
- **Documento vivo:** efeito novo que revele buraco no protocolo ganha etapa
  nova. O IFÁ10000 melhora a cada certificação.
