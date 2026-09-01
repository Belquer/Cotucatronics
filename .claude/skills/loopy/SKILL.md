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

```
Trigger  = [prefixo:] <status><cc>[<valor>]      em hex
           status = 0xB0 | (canal-1)             b0=ch1  b3=ch4  b4=ch5  b5=ch6
           sem valor  -> contínuo (Absolute)
           com valor  -> gesto de botão
Subject  = "<TipoWidget>:<índice da cena>:<posição entre os do mesmo tipo>"
```

🚫 **O `índice` NÃO é a posição no ecrã nem na nossa lista.** É a ordem do array
`widgets` da `Scene`, **contada por tipo**. Foi o que dois bindings feitos à mão
provaram: `Sli:1:2` era o *Gate Thresh A* e `Sli:1:6` o *Piezo Mix*.

⚠️ **O endereço é POSICIONAL, e isso é uma armadilha silenciosa:** reordenar
widgets numa página **reendereça todos os bindings** sem aviso nenhum. O perfil
continua a carregar, os controlos passam a mexer noutros parâmetros, e nada
denuncia. **Regerar o perfil depois de mexer no layout**, sempre.

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
