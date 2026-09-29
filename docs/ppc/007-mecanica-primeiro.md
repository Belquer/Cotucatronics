# ppc/007 — arquitectura **mecânica-primeiro**: o painel é o datum
<!-- doc-check: ignore funcao_dos_pads (portao do /aipcb, ferramenta em kicad/) -->

<!-- doc-check: ignore GuitarPedal125B (biblioteca de footprints do projecto de referencia) -->

> ⚠️ **Sobre o número.** Este documento foi escrito como *"ppc/004"*. O 004 já
> estava tomado por [`004-cerebro-revD.md`](004-cerebro-revD.md), e renumerar em
> silêncio um documento existente é pior do que ter um número fora de ordem —
> por isso entra como **007**.
>
> **A ordem de EXECUÇÃO é outra coisa, e essa mantém-se: este ppc vem antes do
> [`003`](003-verificacao-3d-placa-caixa.md).** A montagem 3D passa a ser
> desenhada sobre a arquitectura nova, não sobre a rev B. O `003` já tem a nota.

> **Estado: REGISTADO E INCORPORADO a 2026-09-29.** As cinco skills e os dois
> documentos estão commitados; o que falta é aplicar a arquitectura ao desenho
> da rev C, que é trabalho de placa e não de documento.

---

## §0 — A cicatriz-mãe, antes dos consertos

O dB tentou montar e relatou **nove** coisas:

1. caixa impossível de montar no tamanho actual
2. módulo do amp gigante
3. TRS de linha a bloquear a remoção do painel
4. cabo blindado dos piezos grosso demais
5. alturas de pot e encoder diferentes depois de soldados
6. OLED exposto a danos
7. módulo USB-C sem sítio para aparafusar
8. headers e jumper wires precários
9. e o conjunto: nada sai sem desmontar outra coisa

**Parecem nove problemas. São um:** o fundo foi tratado como datum e a mecânica
como consumidora do desenho eléctrico.

A prática de pedais e de Eurorack é o inverso:

> **O painel é o datum. O fundo é tampa.**

**Os nove sintomas ficam listados de propósito.** São a evidência que impede a
próxima geração de revogar a regra por parecer arbitrária. Regra sem a factura
ao lado é preferência, e preferências revogam-se.

## §1 — Registar ✅

- **bts 15ga** — a cicatriz, com os nove sintomas, a tabela sintoma→regra, a
  lacuna dos portões e o "rev B = bancada".
- **`docs/operacao/HARDWARE.md` §0** — a inversão declarada como **regra do
  projecto**, não opinião desta revisão, e colocada **antes** da secção de
  energia, porque manda nela.

## §2 — Incorporar na `/aipcb` ✅

**§2b · Mecânica-primeiro**, colocada **antes da §2 e antes da porta 1** — é a
secção que decide se a §2 faz sentido, porque escolher footprints antes de fixar
o datum é escolher peças para uma caixa que ainda não existe.

As sete asserções: painel-datum · ordem de montagem (enfiar → aparafusar →
soldar) · sanduíche por header sem cabos de sinal · conectores PCB-mount com
rosca de painel · energia na placa · potência em compartimento próprio · display
atrás do painel. E a transversal: **madeira é aparência, painel é tolerância.**

**E a §2b acrescenta uma regra que o texto original não tinha:**

> **Pino longo não é detalhe de encomenda — é a folga física que permite à
> mecânica mandar.** Footprint de pino curto torna a asserção da ordem de
> montagem **inexecutável**, qualquer que seja a intenção do desenho.

## §3 — Incorporar na `/caixa` ✅

**§0a · A caixa recebe o painel.** A caixa deixa de ser desenhada a partir do
interior estimado: recebe o painel, e os cortes são derivados dele.

E o `ppc/003` ganhou a **sétima asserção — "a caixa é montável"**: cada peça
entra e sai na ordem declarada, com a ferramenta a alcançar cada parafuso. Não é
folga, é **sequência** — uma caixa cujas peças cabem todas e que exige dessoldar
algo para trocar um pot falha esta asserção com todas as folgas verdes.

## §4 — Incorporar na `/audit` ✅

**Passada nova: FONTE DE DATUM.** Para cada dimensão presente em dois
artefactos, qual é a fonte e qual é a derivada.

> **Dimensão com duas fontes é defeito, mesmo que os números batam hoje.**

E o refinamento que o uso obriga: **a derivada pode estar certa e ainda assim
ser achado.** A passada não pergunta se o número está certo — pergunta quem o
manda. Auditar o valor em vez da procedência aprova um sistema que parte na
semana seguinte.

## §5 — E uma sobre nós ✅

`/aipcb` §5 passa a dizer, onde os portões são reportados:

> 🔴 **Nenhum portão desta lista mede MONTABILIDADE.** ERC, DRC,
> `funcao_dos_pads`, folgas e courtyards — todos verdes, e a caixa não se monta.

A verificação é o dB a montar; o modelo do `ppc/003` é a única coisa que a
antecipa. **Um portão verde aqui é silêncio com cara de aprovação**, que é a
família de defeito mais cara deste projecto.

## §6 — rev B = bancada ✅

Escrito na bts 15ga, no `HARDWARE.md` §0 e no topo do `STATUS.md`:

> **A rev B valida pré-amp, firmware, SuMiA, búzios e a sessão de escuta. Não
> são as placas do instrumento.**

## §7 — A prova externa, que o texto original não pedia

`docs/REFERENCIA-GUITARPEDAL-STM32.md` — o pedal aberto da GuitarML, vendido e
montado por terceiros. O processo de montagem **deles** diz:

> *"use enclosure to ensure proper fit while soldering"* ·
> *"solder LEDs using enclosure as guide for height"*

e a biblioteca de footprints chama-se **`GuitarPedal125B`** — a caixa no nome do
ficheiro. **17 de 101 footprints** são through-hole: exactamente os que
atravessam o painel.

> **Uma regra de arquitectura que também aparece num produto alheio deixou de
> ser preferência.** Procurar a confirmação externa antes de declarar uma regra
> custa uma tarde e poupa uma revisão inteira a defendê-la.

⚠️ **E a referência também corrige a regra:** o "zero cabos" deles **não é
zero** — os footswitches têm fio, por header. A versão honesta é **zero fios de
SINAL; fio só para o que se move ou vive na parede longe da placa**, e mesmo
esse com header, para que a placa saia sem dessoldar nada.

## §8 — O que falta, e não é documento

1. **Desenhar o painel da rev C como peça própria**, com as coordenadas que
   passam a ser fonte para tudo o resto.
2. **Escolher footprints com curso vertical declarado** — pino longo onde
   atravessa o painel, e o curso escrito ao lado da escolha.
3. **A sanduíche de três placas**: definir os headers, o passo, e qual placa
   carrega qual conector.
4. **Correr a passada fonte-de-datum** contra os artefactos que já existem, que
   é onde vão aparecer as dimensões com dois donos.
5. **Escrever a ordem de montagem** — é o entregável da sétima asserção, e sem
   ela a asserção não é verificável.

## §9 — O que eu não sei

- Se a sanduíche de três placas cabe na caixa actual, ou se a caixa tem de
  crescer. **Não estimei** — depende do painel, que ainda não existe como peça.
- Se o chip de PD sink na placa passa nos requisitos de dissipação junto ao
  compartimento de potência.
- Qual é o curso vertical real dos encoders que já temos: pode ser que a
  asserção da ordem de montagem exija peças diferentes, e isso é compra.
