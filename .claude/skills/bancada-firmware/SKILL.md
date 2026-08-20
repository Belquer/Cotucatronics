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
