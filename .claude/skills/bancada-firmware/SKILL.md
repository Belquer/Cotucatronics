---
name: bancada-firmware
description: Rotina de bancada para qualquer projeto de firmware embarcado — entrar no bootloader por software em vez de por botão, conferir que a versão na placa bate com a do fonte, e não confundir saída de ferramenta com confirmação de intenção. Use ao gravar, ao montar ciclo de build/flash, ou quando um ciclo de gravação estiver custando tempo repetidamente.
---

# Bancada de firmware — as três regras

## 1. PROCURE O RESET POR SOFTWARE ANTES DE ACEITAR O BOTÃO

**Antes de construir qualquer coisa em volta de um RESET físico, procure na
API da plataforma como entrar no bootloader por comando.** Quase toda MCU
moderna tem — e quase todo projeto descobre tarde.

```
STM32 / libDaisy   System::ResetToBootloader(DAISY_INFINITE_TIMEOUT)
Genérico ARM       NVIC_SystemReset() com flag em RAM não-inicializada
RP2040             reset_usb_boot()
ESP32              esp_restart() + boot pin, ou OTA
AVR/Arduino        watchdog reset, ou toque de DTR pelo próprio serial
```

**O ganho é de ordem de grandeza, não incremental.** Num projeto real: as
gravações levavam **119, 66 e 47 tentativas** disputando uma janela de ~2 s
depois do botão. Com o comando por software, a primeira gravação seguinte
levou **1**.

**E o timeout infinito importa mais que o reset.** Entrar no bootloader com
janela curta só move o problema; entrar e *esperar* elimina a coordenação
entre duas partes.

> **O atrito repetido vira paisagem.** A janela de 2 s foi aceita como fato da
> bancada desde a primeira sessão, e todo o fluxo — laçadas de retentativa,
> pedidos de "toca o RESET", sincronização de tempo — foi construído em volta
> dela. A solução estava no header da biblioteca o tempo todo.
>
> **Quando um atrito se repete pela terceira vez, pergunte se ele precisa
> existir** — antes de automatizar o contorno.

## 2. CONFIRA A VERSÃO: NO FONTE E NA PLACA

**Toda gravação termina comparando duas strings.** O firmware deve imprimir a
versão **a qualquer momento**, não só no banner de boot — quem conecta a
serial depois do boot perde o banner.

```
placa:  <comando de status>  ->  "Projeto vX.Y.Z"
fonte:  head -3 main.cpp     ->  "vX.Y.Z"
```

Se não baterem, a edição não entrou — **por mais que a gravação tenha dito
que gravou.**

*Custou três anúncios de "consertado" com o fonte travado numa versão antiga:
as edições morriam em `assert` de âncora, e só o resultado da gravação era
conferido.*

**Se a placa não sabe dizer a própria versão fora do boot, conserte isso
primeiro** — uma regra que não pode ser executada não é regra.

## 2b. ESTADO DE PLACA NÃO SOBREVIVE AO FLASH

Params, calibração, modo de log — tudo volátil morre na regravação. Neste
projeto os params **nascem todos em zero por regra do dB**, então depois de
gravar a placa está sempre no estado neutro, nunca no que estava configurado.

Caso (20/08): configurei o detune, gravei para consertar outra coisa, e
anunciei *"está ligado e pronto"* repetindo um estado anterior ao flash. O
efeito estava com a chave em zero e nunca tocou.

> **Ou se relê o estado depois de gravar, ou não se afirma nada sobre ele.**
> A regravação é uma fronteira, e afirmação não a atravessa.

Reler custa um comando:

```
list
```

**Corolário — alarme de boot precisa de gêmeo sob demanda.** A denúncia de
estouro de tabela existia havia nove versões e não evitou a segunda
ocorrência, porque só saía no banner: quem conecta a serial depois nunca a
vê. Todo alarme de boot ganha uma via consultável a qualquer momento (aqui,
`board` passou a imprimir `tabelas: N/teto params | N/teto CCs`).

## 2c. GRAVAR DESTRÓI ESTADO — REPOR E DECLARAR FAZ PARTE DE GRAVAR

A §2b cobre o erro de **relato** (afirmar estado que o flash apagou). Esta
cobre o de **ação**, que é pior porque o outro lado não tem como saber que
aconteceu.

Caso (20/08): quatro gravações na mesma conversa, cada uma zerando `level`,
nenhuma reposta nem anunciada. O dB re-dialava pelo Loopy sem saber por quê, e
uma das perguntas dele ("sem som no p2") nasceu de um silêncio que era meu.

> **Gravar não termina no `File downloaded successfully`.** Termina em repor o
> estado de operação e **dizer qual estado foi reposto**.

Mínimo a repor e declarar: `level`, `in_gain`, e as chaves do que estava
tocando. Se a decisão for deixar zerado (teste seco, por exemplo), **dizer
isso também** — silêncio deliberado e silêncio acidental são indistinguíveis
do outro lado.

## 2d. IDENTIFIQUE A PLACA PELA RESPOSTA, NUNCA PELO NOME DA PORTA

Com duas placas na USB, o seletor por nome (`if '11201' not in x`) é uma aposta
em como o sistema operativo vai enumerar hoje. Em 01/09 a ESP32 apareceu como
`usbmodem101`, passou o filtro, e **ficou em primeiro na ordem alfabética**:
cinco comandos de configuração foram para a ponte BLE.

```python
# a porta certa é a que RESPONDE
for p in sorted(glob.glob('/dev/cu.usbmodem*')):
    s = serial.Serial(p, 115200, timeout=1); time.sleep(0.8)
    s.write(b'board\n'); time.sleep(1.0)
    if 'Cotucatronics OS' in s.read(300).decode('utf8','replace'):
        return s
```

E o agravante não foi o seletor, foi o relatório: o script imprimia
`notches repostos: ...` **incondicionalmente**, então dizia a mesma coisa quer
tivesse funcionado quer não. **Script auxiliar de bancada obedece à mesma regra
que o firmware: não se imprime "feito", imprime-se a leitura de volta.**

## 3. SAÍDA DE FERRAMENTA ≠ CONFIRMAÇÃO DE INTENÇÃO

São confirmações **diferentes**, e cada uma cobre só o seu pedaço:

| Confirma | Não confirma |
|---|---|
| `GRAVOU` da laçada | *qual* binário subiu |
| `assert` da âncora | que o arquivo foi escrito |
| build limpo | que o código novo está no build |
| `grep` no fonte | que a placa tem o fonte |

**Corolário incômodo:** quanto melhor a proteção contra edição silenciosa
(asserts, âncoras), mais fácil é **não perceber que ela agiu** — o efeito de
uma edição barrada é o arquivo continuar igual, e arquivo igual não faz
barulho.

## 4. Se o RESET manual for inevitável: laçada primeiro, botão depois

A janela do bootloader dura ~2 s. **Inicie a laçada de gravação ANTES de
pedir o toque** — pedir o evento antes de estar pronto para recebê-lo é a
mesma família de bug de ordem que "configurar antes de consumir", aplicada a
duas pessoas em vez de a duas funções.

```bash
# laçada em background, tentando 1×/s
for i in $(seq 1 120); do <flash> && break; sleep 1; done &
# só ENTÃO: "toca o RESET"
```

## Antes da primeira gravação exploratória: declare o bom conhecido

Em 22/08 uma troca de `APP_TYPE` produziu travamento reprodutível. Investiguei
por **seis gravações seguidas**, cada uma com um conserto novo, sem nunca
fixar o ponto de retorno. Quando o app parou de enumerar o USB, não havia
para onde voltar sem reconstruir — e a recuperação custou BOOT+RST,
reinstalação do bootloader e vários RESETs físicos do operador.

O repositório tinha o chão o tempo todo. Levei seis gravações para usá-lo.

> **Antes da primeira gravação exploratória, dizer em voz alta qual commit é
> o bom conhecido e como se volta a ele.** Não é cerimônia: é a diferença
> entre "reverto num comando" e "reconstruo às cegas com a placa muda".

E um conserto por gravação, com a placa respondendo entre cada um. Empilhar
consertos numa gravação só torna todo diagnóstico seguinte ambíguo.

## A janela do bootloader não se pega por comando, se pega por espera

Gravar **depois** de pedir o RESET falha: o comando leva mais tempo que a
janela de 2 s. O certo é o inverso — **ficar esperando o bootloader aparecer
e gravar no instante em que surgir**:

```
polling de `dfu-util -l` a cada 0,25 s; grava no primeiro acerto
```

Funcionou de primeira depois de duas tentativas perdidas pelo método antigo.

## Recuperação quando o app não enumera nada

Sintoma: sem porta serial, sem DFU, LED aceso. O app subiu e derrubou o USB.

1. **BOOT+RST** entra no DFU da **ROM da ST** (`0483:df11`, mostra a flash
   interna em `0x08000000`) — independente do app e do bootloader.
2. `make program-boot` reinstala o bootloader da Daisy. O erro no
   `leave request` no fim é normal: o dispositivo já saiu.
3. RESET simples, e gravar o app pela espera descrita acima.

## Recuperação quando a placa ENUMERA mas não fala

Sintoma diferente do anterior, e mais enganador: a porta serial existe, abre
sem erro, e devolve **zero bytes** — a tudo. Nem saída espontânea, nem
resposta a comando.

A tentação é concluir "travou". **Não conclua da mudez.** O sistema
operativo continua a ver o dispositivo, e ele é testemunha independente do
firmware:

```bash
ls -l /dev/cu.usbmodem*                       # a marca de tempo do node
ioreg -p IOUSB -w0 -l | grep -E '"idProduct"|"USB Product Name"'
```

- **A marca de tempo do node muda a cada re-enumeração.** Se ela muda quando
  você manda um comando, o **RX funciona e o comando executou** — só o TX
  está morto.
- **`idProduct` diz em que modo ela está.** `0x5740` = CDC do app;
  `0xdf11` = DFU. Uma placa que "não entra em DFU" pode estar a reiniciar e
  a voltar ao app, e os dois casos são indistinguíveis pelo `/dev`.

Caso real (28/08): a placa calou depois de um `dfu`, e ficou muda a 8 s de
escuta passiva e a três comandos. O node mudou de 11:08 para 11:12 a cada
`dfu go` — estava a reiniciar o tempo todo. **A gravação resolveu a mudez**,
porque o disjuntor do log vive no app.

> **Canal mudo prova que aquele canal está mudo, e nada além disso.**
> Testemunha que não depende do acusado vale mais que dez tentativas de o
> fazer falar.

E não atribua a mudez ao seu último comando só porque coincidiram: nesta
bancada uma coincidência perfeita já produziu um *"era o cabo"* que era o
dongle.

## Gravação que falha a meio deixa a flash PARCIALMENTE APAGADA

`dfu-util` apaga antes de escrever. Se falhar no apagamento — visto em
28/08 aos **52%**, com `Error during special command "ERASE_PAGE"
get_status` e `make: *** Error 74` — **não há app nenhum na placa**, e o
estado é pior que antes de começar.

O que fazer, por ordem:

1. **Confira se ela ainda está em DFU** (`dfu-util -l | grep 0483:df11`).
   Depois de um download falhado ela costuma ficar, porque o `:leave` nunca
   chegou a correr.
2. **Repita imediatamente**, sem pedir gesto nenhum ao operador. A segunda
   tentativa correu limpa até `File downloaded successfully`.
3. Só se ela tiver saído do DFU é que se volta ao BOOT+RST.

**Suspeito habitual: a cadeia de USB.** A placa estava em `1-1.4`, atrás de
dois hubs. Antes de culpar a flash, tire os hubs do caminho.

> **`Error 74` e `File downloaded successfully` são as duas únicas leituras
> que importam.** A regra §3 aplicada ao caso: o eco do seu script diz que
> ele correu, não que o binário subiu.

## Um comando destrutivo pede confirmação — e o eco dela pode não chegar

O `dfu` deste projeto exige `dfu go`; `dfu` sozinho só imprime o aviso. Isso
está certo — não há volta por software.

Mas repare no modo de falha combinado: se o log estiver mudo, **você manda
`dfu`, não recebe o aviso, e conclui que o comando não existe ou não pegou.**
A confirmação existe para o humano, e o humano é justamente quem fica sem
ver quando o canal de saída morre.

> **Ao mandar um comando e não receber eco, a próxima pergunta não é "mandei
> certo?" — é "este canal ainda fala?".** Prove o canal com um comando de
> eco conhecido antes de reescrever o comando.
