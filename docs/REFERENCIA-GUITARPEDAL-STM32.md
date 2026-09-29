# Referência — GuitarML / GuitarPedalSTM32
<!-- doc-check: ignore gr_poly (token do formato .kicad_pcb) -->

<!-- doc-check: ignore GuitarPedal125B (nome de biblioteca de footprints do projecto alheio) -->
<!-- doc-check: ignore NMJ6HCD2 (MPN de jack Neutrik do projecto alheio) -->
<!-- doc-check: ignore thru_hole (token do formato .kicad_pcb) -->

> **O que é:** pedal de guitarra digital de código e hardware abertos, STM32H7B0
> + codec CS4270, KiCad + Gerbers para JLCPCB, caixa 125B furada pela Tayda.
> Autor: Keith Bloemer (GuitarML).
> **Fonte:** <https://github.com/GuitarML/GuitarPedalSTM32>
> **Lido a 2026-09-29**, no commit da ponta de `main` (clone `--depth 1`).
> Licença: ver `LICENSE.txt` do repositório deles.

**Por que está aqui:** é um instrumento montável, vendido e montado por
terceiros, com a **arquitectura mecânica-primeiro** que o ppc/007 declara para a
rev C — e que aqui não é opinião, é o processo de montagem escrito pelo autor.
Serve de prova externa de que a inversão funciona fora da nossa cabeça.

⚠️ **Isto é leitura de um projecto alheio, não medição do nosso.** Cada número
abaixo é o que o ficheiro deles diz; nenhum foi conferido contra hardware nosso,
e nenhum autoriza uma decisão sem ser reexaminado no nosso contexto.

---

## 1. O achado central: a caixa é o instrumento de medida

O processo de montagem do README deles, na ordem em que está escrito:

> 4. *Solder potentiometers and switches … and fit check with enclosure*
>    (**use enclosure to ensure proper fit while soldering**).
> 5. *Remove pcb from enclosure, solder LEDs* **using enclosure as guide for
>    height**.

**Ninguém calcula altura.** As peças entram sem soldar, a caixa decide onde
ficam, e a solda vem depois. É literalmente a regra do ppc/007 — *"nunca acertar
altura por cálculo; a mecânica decide, a solda segue"* — encontrada num produto
que se vende, não numa nota de projecto.

E há a peça de cobre que torna isso possível:

```
CustomFootprints:Pot_RightAngle_LongPin_16mm      × 6
```

> **`LongPin` não é detalhe de encomenda: é a folga que permite à mecânica
> mandar.** Um footprint de pino curto obriga a acertar a altura por cálculo,
> porque não há curso para a ajustar. A escolha do footprint é onde a
> arquitectura mecânica-primeiro passa a ser fisicamente possível ou impossível.

É o conserto directo do sintoma nº 5 dos nove do dB — *"alturas de pot e encoder
diferentes depois de soldados"*.

## 2. A caixa está no nome da biblioteca

```
GuitarPedal125B:Generic_PowerJack
```

A biblioteca de footprints tem o nome do **modelo da caixa**. Não é organização:
é a declaração de que aquele footprint só faz sentido dentro daquela caixa, e de
que trocar de caixa invalida a biblioteca. O datum está no nome do ficheiro.

## 3. A geometria, medida no `.kicad_pcb`

Contorno (`gr_poly` em `Edge.Cuts`):

```
(112,47) (118,47) (118,41) (166,41) (166,47) (172,47) (172,136) (112,136)
```

- corpo: **60 × 89 mm**
- **aba saliente de 48 × 6 mm** no topo — e o README diz para que serve:
  *"The power jack is intended to protrude from the pcb edge, fitting slightly
  into the rectangle cutout in the enclosure."*

> **A aba é a placa a entrar na parede, não a parede a abrir-se para a placa.**
> O corte da caixa é derivado da placa, e o encaixe é mecânico, não de fio.

## 4. Três faixas, e nenhum fio de sinal entre elas

Por Y, tudo o que atravessa o painel:

| Y (mm) | o que está lá |
|---|---|
| 56–59 | `J5` jack Neutrik · `J3` jack de energia |
| 71–73 | `POT1..POT6` (duas filas de três) · `SW3`, `SW4` SPDT |
| 125–127 | `J1`, `J2` jacks Neutrik · `D3` LED RGB · `LED1`, `LED2` headers |
| 133 | `FS1`, `FS2` headers dos footswitches |

**Conectores nas duas bordas, controlos numa faixa no meio.** Todos montados na
placa. O sinal de áudio nunca anda em fio dentro da caixa: entra por um jack
soldado e sai por outro jack soldado.

## 5. A conta que mais diz: 17 de 101

```
footprints no .kicad_pcb ......... 101
com pad through-hole .............. 17
```

Os 84 restantes são SMD, colocados por máquina. **Os 17 through-hole são
exactamente o conjunto que atravessa o painel** — jacks, pots, interruptores,
LEDs e os headers dos footswitches.

> **A fronteira through-hole / SMD coincide com a fronteira mecânica / eléctrica
> e com a fronteira humano / máquina.** Não é coincidência: o que tem de casar
> com um furo é o que uma pessoa monta, e é o que a caixa mede.

## 6. O BOM de fábrica NÃO tem os pots

```
STM32H7_v3_guitar_pedal_bom.csv  ->  39 linhas, zero pots, zero jacks
```

As peças de painel vivem na **prosa do README** (*"Six 16mm right angle
potentiometers, 2 three way toggles, 2 Footswitches, Two 3mm LEDs, Two 3mm LED
bezels"*), com as lojas ao lado e os prazos de entrega declarados.

> **São dois BOMs com donos diferentes**: o de fábrica é contrato com a máquina,
> o de montagem é lista de compras de quem monta. Fundi-los faz a fábrica
> recusar linhas que não sabe colocar; separá-los sem o dizer faz alguém
> encomendar a placa e descobrir, duas semanas depois, que não tem pots.

⚠️ Confrontar com a `/kicad` §7c, que diz *"o designador é CHAVE"* entre
esquema e BOM. Continua verdade — mas **há dois BOMs**, e a regra tem de dizer
em qual o designador vive.

## 7. A caixa é um artefacto derivado, e publicado

Os furos da caixa não estão num PDF nem numa nota: estão num **link de gabarito
da Tayda**, um por variante do painel.

```
Two Footswitch / Two LED  ->  drill.taydakits.com/box-designs/... 
Single Footswitch / RGB   ->  drill.taydakits.com/box-designs/...
```

> **O gabarito de furação é a saída de um gerador, e tem uma variante por
> variante de painel.** É a mesma disciplina que o projecto já aplica aos SVG da
> caixa — aqui aplicada a um serviço de terceiros.

## 8. A placa carimba-se

Silkscreen, lido na fotografia `images/pcb_pic.jpg`:

```
Keith Bloemer
STM32H7-v3
8/3/26
```

**Autor, versão e data no cobre.** É a regra de versionamento deste projecto
(`vX.Y.Z` visível em tempo de execução) aplicada ao hardware — e no hardware é
mais importante, porque a placa sobrevive ao repositório que a gerou e vai
parar a uma bancada onde ninguém tem o `git log`.

## 9. Os gotchas — todos em prosa, nenhum num verificador

Estão no README deles como `IMPORTANT:`, o que quer dizer que **nenhuma
ferramenta os apanha** e que foram todos descobertos a montar:

1. **A base do toggle tem de ser isolada com fita** — fica encostada às pernas
   do pot vizinho e faz curto. *"The control layout spacing … is very tight."*
2. **Os pots precisam de tampas anti-pó** ou o corpo deles curto-circuita as
   ligações no **verso** da placa.
3. **O JLCPCB roda peças que o KiCad exportou noutra orientação** — ICs, relés,
   transístores, LDOs e díodos, conferidos um a um na ferramenta de
   visualização deles antes de confirmar a encomenda.
4. **O jack de energia precisa de ajuste antes de soldar pots e toggles**,
   segurando a placa pelos três jacks de 1/4".

> **Todos os quatro são de MONTAGEM, e os portões do `/aipcb` não medem
> montagem.** Ver `/aipcb` §5: ERC, DRC, folgas e courtyards podem estar todos
> verdes com a caixa por montar. Estes quatro são a prova externa de que a
> lacuna é real e não teórica — um projecto maduro, vendido, e a lista de
> avisos continua a ser prosa.

## 10. O que NÃO transfere, e é preciso dizê-lo

| deles | nosso |
|---|---|
| 6 pots, 2 toggles, 2 footswitches | muito mais controlos, OLED, encoders |
| uma placa só | sanduíche de três (controlo · E/S · cérebro), ppc/007 |
| caixa 125B de alumínio fundido, furada por fornecedor | acrílico e madeira, cortados por nós |
| 9 V DC centro-negativo, jack simples | conversão na placa (USB-C + PD sink) |
| alimentação por jack, sem comutação de potência | compartimento de potência com parede e terra próprios |

⚠️ **E o "zero cabos" deles não é zero.** `FS1`, `FS2`, `LED1` e `LED2` são
headers de 2 pinos com fio até aos footswitches e LEDs — o próprio README manda
*"solder wires to footswitches"*. A regra honesta que se tira daqui não é "zero
cabos", é:

> **Zero fios de SINAL. Fio só para o que se move ou o que vive na parede longe
> da placa** — e mesmo esses, com header na placa em vez de solda directa, para
> que a placa saia sem dessoldar nada.

O sintoma nº 3 do dB — *"TRS de linha a bloquear a remoção do painel"* — é
exactamente o que esta regra evita.

---

## Ficheiros que vale a pena reler na fonte

| ficheiro | para quê |
|---|---|
| `README.md` | o processo de montagem, e os quatro `IMPORTANT:` |
| `hardware/.../STM32H7_GuitarPedal.kicad_pcb` | contorno, aba, as três faixas |
| `hardware/.../CustomFootprints.pretty/` | `Pot_RightAngle_LongPin_16mm`, `PJ-320A` |
| `hardware/.../fab/JLCPCB/` | o BOM de fábrica, e o que ele não tem |
| `images/pcb_pic.jpg` | frente e verso da placa montada |
