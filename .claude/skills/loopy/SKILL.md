---
name: loopy
description: Gerar e alterar projectos e perfis de controlo do Loopy Pro por script — o formato do `.lpproj` (SQLite com JSON), o do `.lpcontrolprofile`, e a diferença entre os dois. Use ao criar páginas/widgets, ao mapear CCs, ao regenerar bindings depois de renomear params, ou quando o dB pedir algo "para importar no Loopy".
---

# Loopy Pro por script

## PRIMEIRO: perfil ou projecto? São coisas diferentes

O erro que custa a volta é entregar um e o outro é que era preciso.

| | `.lpcontrolprofile` | `.lpproj` |
|---|---|---|
| **contém** | o MAPEAMENTO de CC → widget | os **widgets em si**, páginas, cores, títulos |
| **cria widgets?** | ⛔ **não** — endereça os que já existem | ✅ sim |
| **serve para** | ligar um controlador a uma página já construída | entregar a **página construída** |
| **precisa de** | o `.lpproj` exportado, para saber os índices | nada |

> **"Dá-me para importar no Loopy" quase sempre quer dizer PROJECTO.** Perfil só
> resolve se os widgets já existirem — e se ainda não existem, o perfil não tem
> a que se agarrar.

---

## O formato do `.lpproj`

Um zip. Dentro:

```
Project.sqlite      ← tudo o que interessa
Info.plist  Profiles/  Profile.lpcontrolprofile
Resources/  Resources.plist  Thumbnail.jpg
```

### `Project.sqlite`, duas tabelas

```sql
CREATE TABLE objects (id INTEGER PRIMARY KEY, type TEXT, contents BLOB);
CREATE TABLE meta    (name TEXT UNIQUE, value BLOB);
```

⚠️ **O `contents` é JSON em texto, NÃO é plist binário.** Vale a pena confirmar
antes de assumir — `b'{"frame":...'` e não `b'bplist00'`. Isto torna o formato
inteiramente tratável por script.

### Referências entre objectos

Toda ligação é um par com o tipo e o `id`:

```json
{"_type": "SliderWidget", "_value": 1526}
```

O mesmo padrão embrulha valores estruturados: `{"_type":"Rect","_value":[x,y,w,h]}`,
`{"_type":"Color","_value":[r,g,b,a]}`, `{"_type":"Set","_value":[...]}`.

### Os objectos que interessam para uma página

**`Scene`** — uma página:

```json
{"title":"central", "columns":5, "rows":2, "orientation":3,
 "widgets":[{"_type":"SliderWidget","_value":1526}, ...],
 "tracks":[], "playGroups":[]}
```

**`SliderWidget`** — fader:

```json
{"frame":{"_type":"Rect","_value":[2.25, 1, 0.5, 0.5]},
 "title":"Piezo Mix", "value":0.5, "type":0,
 "color":{"_type":"Color","_value":[0.90,0.36,0.17,1]}}
```

**`ButtonWidget`** — botão:

```json
{"frame":{"_type":"Rect","_value":[1,0,0.5,0.5]},
 "state":0, "title":"BUZIOS",
 "color":{"_type":"Color","_value":[1,0.008,0.004,1]}}
```

Outros vistos: `SteppedDialWidget` (switch de N posições), `RotaryEncoderWidget`,
`TextWidget`, `Group`, `Track`.

⚠️ **O `frame` é em unidades de GRELHA da cena, não em píxeis.** Uma cena `5x2`
tem coordenadas de 0 a 5 na horizontal; `0.5` de largura é meia célula.

**`Project`** amarra tudo: `scenes` (lista de referências), `currentScene`,
`midiSources`, `groups`, `activeControlProfileNames`.

### Para acrescentar uma página

1. `id` novos que não colidam — `max(id)+1` em diante.
2. Um `Scene` com `widgets` a apontar para os ids novos.
3. Acrescentar a referência do `Scene` à lista `scenes` do `Project`.
4. Rezipar preservando **todos** os outros ficheiros do bundle.

⚠️ **Ao re-zipar, escrever TAMBÉM as entradas de directório.** Um `os.walk` que
só escreve ficheiros perde `Control Profiles/` e `Resources/`, que fazem parte
do bundle. E não chega escrever só as vazias — `Control Profiles/` contém outro
directório. **O teste que apanha isto é comparar a lista de nomes do zip de
entrada com a do de saída**; contar ficheiros não apanha nada.

⚠️ **Nunca escrever por cima do projecto do dB.** Gerar um ficheiro novo, e
deixá-lo importar — projecto é onde vive o trabalho dele, e um erro aqui apaga
horas de arranjo de ecrã.

---

## O formato do `.lpcontrolprofile`

Decodificado em 24/08 e confirmado contra dois bindings feitos à mão.

🔴 **DUAS EXTENSÕES PARECIDAS, SÍTIOS DIFERENTES (dB, 23/09).** Custou uma
importação recusada e uma renomeação à mão:

| onde | como se chama |
|---|---|
| **dentro** do `.lpproj`, em `Control Profiles/` | `<nome>.lpcontrol**profile**` |
| **exportado solto**, para importar no Loopy | `<nome>.lpcontrol**ler**profile` |

⚠️ E a armadilha não é só o nome: a importação tinha **duas** falhas ao mesmo
tempo — a extensão errada e a identidade do dispositivo cravada no gerador.
Corrigida a primeira, o Loopy continuou a recusar (*"perfil global sem
bindings"*), e **parecia a mesma falha por consertar**. Duas causas empilhadas
lêem-se como uma só que não cedeu.

🔴 **A IDENTIDADE DO DISPOSITIVO SAI DO PROJECTO, nunca de um literal.** O
ficheiro chama-se `<id>.MIDI.<Nome do dispositivo>.controllerprofile`, e o
plist repete `Device` e `Device Name` lá dentro. Se o id não corresponder a um
controlador que o Loopy conhece, ele mostra **perfil global sem bindings** — lê
o ficheiro e não lhe acha dono. O projecto do dB tinha **dois Cotucatronics
emparelhados** (`Cotucatronics` e `Cotucatronics1`), e o literal do gerador
apontava para o que ele não estava a usar.

E o pacote traz **um ficheiro por dispositivo**, não um sozinho: gerar só o
nosso e deixar cair a pedaleira é perder os bindings dela. O
`gerar_perfil_loopy.py` v0.8.0 copia o pacote inteiro e substitui apenas os
dispositivos cujo nome diz Cotucatronics.

🔴 **CORRIGIDO EM 01/09, e o erro anterior custou um projecto inutilizado.**
Esta skill dizia que o `Subject` era posicional — `"<Tipo>:<cena>:<posição>"`.
**Não é.** Verificado num export real:

```
Subject = "#<id do objecto do widget>"      ex: "#1526"
```

E há **DOIS perfis, um por direcção**, e ambos endereçam por `id`:

| ficheiro | direcção | trigger | alvo |
|---|---|---|---|
| `Control Profile.lpcontrolprofile/Internal…` | **widget → MIDI** (envio) | `"1482:Value"` — id do widget e a acção | — |
| `Control Profiles/<nome>/<dev>.MIDI.…` | **MIDI → widget** (retorno) | `"b364"` — MIDI em hex | `"#1526"` |

```
Trigger MIDI = <status><cc>[<valor>]        em hex
               status = 0xB0 | (canal-1)    b0=ch1  b3=ch4  b4=ch5  b5=ch6
               sem valor -> contínuo · com valor -> gesto de botão
Action       = {"Identifier":"Trigger Widget",
                "Parameters":{"Widget Action":"Value"},
                "Subject":"#<id>", "Timing":"Sequential"}
```

🔴 **CONSEQUÊNCIA QUE DESTRÓI UM PROJECTO:** regerar as páginas **apaga os ids**,
e todos os bindings dos dois perfis passam a apontar para objectos que já não
existem. O projecto abre, os widgets aparecem, **e não sai nem entra um único
byte** — sem erro em lado nenhum.

Foi exactamente o que aconteceu em 01/09: um `.lpproj` gerado com 143 widgets e
158 + 202 bindings órfãos. O sintoma no instrumento era `bytes 0 | cc 0`.

**Portanto: quem regenera páginas TEM de regenerar os dois perfis na mesma
passagem**, com os ids novos — ou **reutilizar os ids antigos** para os widgets
que sobrevivem. Emitir projecto sem perfis é entregar uma página morta.

## O perfil é por DISPOSITIVO, e eles acumulam

Um bundle carrega **um perfil por nome de dispositivo**, e eles ficam lá. Num
export real de 01/09:

```
Ctos Midi.lpcontrolprofile/
    b2d29493.MIDI.Cotucatronics Bluetooth      103 bindings
    227f559e.MIDI.Cotucatronics1 Bluetooth      99 bindings
    d51a4338.MIDI.FootCtrlPlus Bluetooth        12 bindings
```

**Vale o do dispositivo que estiver LIGADO**, e mais nenhum.

⚠️ **Um nome antigo guardado de uma sessão anterior deixa a página inteira
muda.** Foi o que aconteceu em 01/09: o Loopy estava ligado como
`Cotucatronics 1` — sobra de um emparelhamento antigo — enquanto o trabalho todo
estava no perfil `Cotucatronics`. Os widgets no ecrã, o BLE ligado, e **zero
bytes no instrumento**.

O sintoma não aponta para lá: parece firmware, parece fio, parece o projecto.
**Conferir a que dispositivo o Loopy está ligado vem antes de desmontar
seja o que for** — e um bundle com três perfis é o aviso de que há por onde
errar.

⚠️ **E o teste que o apanha é do lado do INSTRUMENTO, não do Loopy:** o contador
de bytes de MIDI recebidos. Zero bytes com o BLE ligado diz que o corte está
antes da placa, e distingue "não mandou" de "mandou e não aplicou".

---

## O casamento é por TÍTULO, e por isso os renames partem-no

O gerador liga widget↔param pelo **título** do widget. Renomear um param quebra
o casamento em silêncio — o widget fica órfão e o CC deixa de chegar.

**Sintoma:** o gerador reporta `sem CC no manifest (<nome antigo>)`. Isso não é
defeito do Loopy; é o mapa de títulos por actualizar.

⚠️ **Manter os nomes antigos no mapa a apontar para os novos.** O ecrã do dB tem
o título antigo escrito nele, e obrigá-lo a renomear vinte widgets para acompanhar
um rename de código é a troca errada — **o código é que se adapta ao que está
pintado no vidro.**

E o inverso também aparece: `título não reconhecido` quer dizer que o widget
existe no ecrã e o param **já não existe no firmware** — param aposentado com
widget vivo. Esse não se conserta no mapa: **apaga-se o widget**, ou fica a
mentir para sempre.

---

## O laço de entrega

1. O dB **exporta** o projecto do Loopy (`.lpproj`).
2. Correr o gerador contra esse ficheiro.
3. **Ler a contagem de órfãos**, não só a de casados. Órfão é achado, não ruído.
4. Entregar, e dizer **o que não casou e porquê** — perfil com 80 de 87
   bindings parece completo e não é.

⚠️ **Verificar contra o `manifest.h`, nunca contra a memória.** Os CCs mudam:
numa só sessão mudaram seis. Documento ou perfil escrito à mão deriva na primeira
alteração, e quem o segue monta o instrumento de ontem.
