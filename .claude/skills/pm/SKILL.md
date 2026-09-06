---
name: pm
description: Alinhar prazos e equivaler processos entre os artefactos do Cotucatronics — caixa, placa e firmware. Qual é o caminho crítico, o que se pode fazer em paralelo, e as etapas que os dois processos têm em comum com nomes diferentes. Use ao decidir a ordem de encomendas, ao estimar quando algo estará montável, e antes de mandar seja o que for para fábrica.
---

# `/pm` — prazos e equivalências

Este projecto tem **três artefactos com relógios diferentes**: a caixa, a placa
e o firmware. Cada um tem o seu portão, a sua fábrica e o seu prazo — e o
instrumento só existe quando os três chegam ao mesmo sítio.

## 1. A pergunta é o CAMINHO CRÍTICO, não a lista de tarefas

> **O que decide a data não é o que falta fazer: é o que falta fazer que
> ninguém pode acelerar depois.** Uma semana de fábrica não se comprime com
> esforço; uma tarde de roteamento comprime-se.

| artefacto | fábrica | prazo típico | comprime-se? |
|---|---|---|---|
| caixa | Ponoko | ~3 dias de produção + envio | **não** |
| placa | JLCPCB | ~2–5 dias + envio | **não** |
| montagem | bancada | horas | sim |
| firmware | bancada | horas | sim, e faz-se depois |

**Corolário operacional:** o que vai para fábrica encomenda-se **primeiro**, e
tudo o que se possa fazer enquanto lá está faz-se **depois** da encomenda, não
antes. Adiar um pedido para «acabar uma coisa que não afecta o ficheiro» é
gastar dias de calendário em trabalho que cabia na espera.

⚠️ **Mas o inverso mata:** encomendar antes de o portão passar troca dias por
uma peça errada, e a peça errada custa o prazo INTEIRO outra vez. A ordem é
**portão → encomenda → resto**, nunca **encomenda → portão**.

## 2. O que é preciso estar decidido antes de cada encomenda

Não é «estar pronto» — é **estar decidido naquilo que o ficheiro carrega**.

**Caixa (Ponoko):** material e espessura · furos e diâmetros de tudo o que a
atravessa · o que é removível · encaixe dentro da chapa. **Não precisa** de
saber o que vai lá dentro estar routeado.

**Placa (JLCPCB):** esquema fechado · footprints · contorno e furos mecânicos ·
**roteamento** · cor da máscara e da serigrafia. **Não precisa** do firmware.

**Firmware:** precisa da placa em mão. É sempre o último, e é o único que se
corrige depois sem custo de fábrica.

## 3. As equivalências — os mesmos passos com nomes diferentes

Reconhecer isto poupa reinventar o processo em cada artefacto, e faz com que
uma cicatriz de um sirva o outro.

| etapa | caixa | placa |
|---|---|---|
| a fonte | `cotuca_caixa_corte.py` | `gera_*.py` + `pcb_contorno.py` |
| «liga?» | — | **ERC** |
| «não se sobrepõe?» | **nesting** | **DRC geometria** |
| «está tudo lá?» | contagem de peças | **paridade** esquema↔PCB |
| «cabe no outro?» | `confere_caixa` | `confere_arcade`, `confere_furos` |
| prova externa | `tools/nesting.py` | DRC do KiCad, visualizador da JLC |
| identidade | rev + digest no nome | rev + digest no nome |
| o veto | render, olho do dB | render, olho do dB |
| «uma primeiro» | cortar UMA e montar | **uma placa, não a série** |

> **A regra do «uma primeiro» é a mesma nos dois, e pela mesma razão:** nenhum
> portão mede tolerância real, e a peça em mão manda em tudo.

## 4. Quando os dois se cruzam

⚠️ **A caixa impõe volume à placa, e a placa não impõe nada à caixa.** A
dependência é de sentido único, e isso decide a ordem: **fecha-se a caixa
primeiro**, e a placa desenha-se contra ela.

O inverso — mudar a caixa depois de a placa estar routeada — custa refazer
trajectos, porque as posições mecânicas mudam e as pistas seguem-nas.

**O que a caixa fixa e a placa tem de obedecer:** contorno · furos de fixação ·
eixos que saem por ela · **e a coluna de ar** (o que desce da tampa por cima da
placa). O último não tem ferramenta que o veja — é o que precisa de portão
escrito à mão.

## 5. Estimar sem inventar

> **Prazo dado por uma fábrica é dado; prazo de trabalho meu é estimativa.**
> Dizer os dois com a mesma confiança é como dizer um número medido e um
> deduzido sem os distinguir (`medicao-limpa` §3).

Ao dar uma data, separar sempre:

```
Ponoko: entrega estimada 15/09        ← deles, com nome
roteamento: 1 sessão                  ← minha estimativa, pode dobrar
JLCPCB: 2-5 dias + envio              ← deles, faixa publicada
```

E quando um prazo escorrega, dizer **qual dos três** escorregou. Um atraso de
fábrica e um atraso meu pedem respostas diferentes: o primeiro absorve-se com
paralelismo, o segundo com corte de âmbito.

## Procedência

Nasceu a 06/09/2026, quando a caixa foi encomendada com entrega a 15/09 e ficou
claro que a placa tinha de estar na JLCPCB **nesse dia ou antes** para as duas
chegarem juntas — e que tudo o que restava da placa (roteamento, revisão de
terra) cabia dentro da espera da caixa, desde que a encomenda saísse primeiro.
