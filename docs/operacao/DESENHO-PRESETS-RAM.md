# Desenho — biblioteca de presets em RAM

> **Estado: passos 1–3 IMPLEMENTADOS e provados na placa (2026-08-23, v0.148.0);
> passos 4–5 pendentes.** O ciclo `capturar → dump → reset → colar → load`
> fecha com 33 params e zero recusas. Escrito enquanto o aceite de hardware
> corre, de propósito: é trabalho de cabeça, não toca o firmware e não captura
> estado nenhum, então não corre o risco de congelar um endereçamento
> não-validado. Ver `ACEITE-MULTIPLEXACAO.md`.
>
> Complementa `PRESETS.md` (aprovado 2026-08-22) com as decisões de estrutura.

## 0. Três correções de contagem, antes de qualquer decisão

**a) São 37 famílias por cadeia, não 33.** A estimativa era 23 originais + 10
cópias de caráter. O real é **27 + 10 = 37** — contado do manifest, não de
memória.

**b) A DTCM não está apertada: está vazia.** A premissa era que `.bss` na DTCM
concorreria com o que já está lá. O relatório do linker diz outra coisa:

    DTCMRAM:        0 B / 128 KB      0.00%
    SRAM (AXI):   281 KB / 512 KB    53.73%

O `.bss` cai na **SRAM**, não na DTCM — o script tem uma seção
`.dtcmram_bss` separada, e **nada a usa**. Os 281 KB da SRAM são quase todos
`reverb_mem` (225 KB).

**c) O `sync` não abre a janela de autoria.** Achado ao investigar o ponto 3,
e vale para o firmware de hoje, não só para o desenho — ver §3.

## 1. O que entra num preset de cadeia

O critério, que é do dB e resolve os casos difíceis sozinho:

> **Preset guarda intenção musical — não estado de hardware, nem ferramenta de
> medição.**

Aplicado às 37 famílias, ele exclui **quatro**, não duas:

| fora | por quê |
|---|---|
| `exc_on` | **estado físico.** Vai virar leitura dos DPDT em D29/D30 — um preset que o escrevesse tentaria mudar uma chave de metal |
| `bench_boost` | **ferramenta de medição.** É bancada |
| `gov_ceiling` | **calibração de hardware.** O manifest já diz por quê: *"cada amp e cada exciter tem ganho e acoplamento próprios, então o teto útil difere"* — é propriedade do ramo físico, não da música |
| `gov_release` | idem |

As duas últimas são a extensão consistente do mesmo argumento que exclui o
`bench_boost`. Um preset "corda-shimmer" carregado numa escultura com outro
exciter levaria junto um teto de proteção **calibrado para outro corpo** — e a
proteção é o subsistema onde estar errado é mais caro.

**Preset de cadeia = 33 params.** Um byte cada (normalizado 0–127, a mesma
resolução que o MIDI entrega e que o `set` refina quando alguém precisa).

## 2. O que entra no preset global

Dos 12 compartilhados, o mesmo critério exclui **três**:

| fora | por quê |
|---|---|
| `tone_freq`, `tone_mix`, `tone_dest` | **o tom de teste é bancada.** Um preset que o ligasse poria uma senoide no meio da peça |

<!-- doc-check: ignore route_exciter (nome APOSENTADO em 2026-08-28, hoje `out_l`. A mencao e deliberada: e o registo da renomeacao, e apagar o nome velho apagaria justamente o que explica por que ele estava errado.) -->
<!-- doc-check: ignore route_jack (idem, hoje `out_r`) -->
**Preset global = 9 params:** `level`, `piezo_mix`, `dly_time`,
`out_l`, `out_r`, `shift_hz`, `buzios_on`, `buzios_thresh`,
`buzios_cooldown`.

⚠️ **`out_*` fica, com a tensão anotada.** É topologia de saída, o que soa
como hardware — mas *qual ramo está vivo* é decisão de performance, e o
desenho aprovado o inclui. (Chamavam-se `route_exciter`/`route_jack` até
2026-08-28; o DPDT de três posições tornou esses nomes falsos.)

🚫 **A matriz `route_*` NÃO entra, e a razão é outra.** Ela compensa como os
transdutores estão LIGADOS — facto de fiação. Dentro do preset, cada preset
carregaria a fiação, e desandar um fio obrigaria a regravar a biblioteca
inteira: a mesma família do *"corda existiria duas vezes"* que fez a
biblioteca ser UMA.

E ela **cai na regra do sufixo** se ninguém a excluir: `route_l_a` acaba em
`_a`, logo sem a excepção em `presets.h` entraria sozinha nos presets de
**cadeia** — o pior dos dois destinos possíveis.

**Global slot 0 = estado de boot**, que é o que mata o `level` nascendo em 0.

## 3. `load`, a janela de autoria, e um defeito que já existe

### O defeito de hoje: `sync` não abre a janela

`MidiEspelha()` escreve `quiet_until[id]` a cada mensagem. **`MidiPushEstado()`
— o `sync` — não escreve nenhuma.** Manda as 86 e sai.

Como widget de toggle devolve o *estado dele* (que é exatamente o motivo de a
janela existir), depois de um `sync` a placa **obedece o eco**. O `eco_ult`
impede o espelho de reenviar; não impede a entrada de aceitar.

**Não é hipótese de desenho — é o firmware atual.** Consertar é uma linha, mas
não com a bancada em curso.

### O que `load` acrescenta

Carregar escreve 33 params de uma vez. Se cada um anunciar, são **33 janelas
abrindo juntas** e 33 mensagens no fio.

**Custo do despejo, medido pelo que já existe:** o `sync` gasta
`hal::DelayMs(3)` por mensagem × 86 = **258 ms de loop bloqueado**. E o
comentário ao lado dele ainda diz *"despejar 12 CCs"* — foi escrito quando o
mapa tinha 12, e o bloqueio cresceu 7× em silêncio. É o achado 5 do audit
reincidindo por crescimento, não por edição.

### A decisão

**`load` NÃO usa o caminho do `sync`.** Marca os 33 params como sujos e deixa o
espelho normal escoá-los na cadência dele — duas mensagens a cada 8 ms, sem
bloquear o loop. 33 params levam ~132 ms de escoamento **sem parar nada**,
contra 99 ms de loop travado se fosse despejo.

    load  ->  escreve os params
          ->  abre quiet_until[p] para os 33, DE UMA VEZ
          ->  marca sujo; o espelho escoa 2 por 8 ms
          ->  drenagem de entrada de 300 ms (o mecanismo já existe)

**Por que a janela abre no `load` e não à medida que o espelho manda:** o
gesto autônomo é o `load`, não cada mensagem. Se a janela abrisse só quando a
mensagem saísse, o widget que ecoasse no milissegundo 5 — antes de a mensagem
dele ter saído — seria obedecido, e o `load` seria desfeito pela metade.

**A drenagem existe e serve aqui sem mudança:** é o mesmo problema da 13n com
outro gatilho — o instrumento acabou de mudar sozinho, e o que chegar nos
próximos 300 ms é reação, não intenção.

## 4. Onde a biblioteca mora

**`.bss` → SRAM (AXI), explicitamente, e não na DTCM.** A razão não é
capacidade — cabe em qualquer uma:

    16 presets de cadeia × 33 B  =    528 B
     8 presets globais  ×  9 B   =     72 B
    cabeçalhos + índices          ≈    128 B
    ------------------------------------------
    total                         ≈    728 B

É **por uso**: a biblioteca é fria — lida no `load`, escrita no `capturar`,
nunca tocada no callback de áudio. A DTCM é a RAM sem espera e sem cache do
núcleo, e gastá-la com armazenamento frio é desperdiçar a única memória que
ainda pode salvar um caminho quente.

> **Anotado para outra ocasião, não para agora:** 128 KB de DTCM inteiramente
> ociosos enquanto 225 KB de `reverb_mem` vivem na SRAM é uma assimetria que
> merece medição própria — com A/B no mesmo binário, porque nesta placa delta
> entre builds pode ser 25% (cicatriz 12d). Não é escopo dos presets.

## 5. O cabeçalho, e a lição da travessia

> **Provar que a configuração nova é segura não prova que a travessia é.**

Um preset capturado hoje e carregado depois de um remapeamento escreve valores
nos params errados: é **exatamente o fantasma mal-endereçado da 13n, com
atraso de semanas** — e pior, porque não há sequer um fio para suspeitar.

```
struct PresetHeader
{
    uint16_t magic;        // 'CT'
    uint8_t  formato;      // versao do RECIPIENTE (não do mapa)
    uint8_t  n_params;     // contagem, redundante de propósito
    uint16_t versao_mapa;  // HASH dos nomes, ver abaixo
    uint8_t  checksum;     // cobre CABEÇALHO E payload
    uint8_t  _pad;
};
```

### O `formato` é a travessia noutra dimensão

Os vereditos cobrem arquivo trocado, remapeamento e corrupção. **Nenhum
cobria "o cabeçalho ganhou um campo entre a gravação e a leitura."** Um preset
gravado com o layout v1 e lido por um firmware v2 é interpretado com
**deslocamento**: passa no magic, falha no checksum, e reporta *corrupção*
para um caso de migração de formato — mandando caçar a coisa errada.

**Convenção, e ela vive ao lado do campo porque em outro lugar ninguém
lembraria:** toda mudança no layout do `PresetHeader`, ou na ordem/largura do
payload, incrementa `kFormato`.

### A ORDEM das conferências decide o diagnóstico

O checksum cobre o **cabeçalho**, e tem que cobrir — um bit trocado no campo
do hash produz *"gravado sob outro mapa"*, que é uma frase **plausível**, e o
operador vai procurar um remapeamento que nunca houve.

Daí a regra: **não se confia em nenhum campo antes de o checksum passar.**

    1. magic     -> arquivo trocado         (mais barato e mais específico)
    2. formato   -> migração de recipiente  (o layout mudou sob os pés)
    3. tamanho   -> incoerente              (sem isto não há o que somar)
    4. checksum  -> corrupção               (antes de confiar em campo algum)
    5. mapa      -> remapeamento            (agora o campo é confiável)
    6. contagem  -> de que lado procurar

Os três primeiros vêm antes do checksum pelo mesmo motivo: são
**pré-condições para a conta existir**.

O banco prova o **par**, não só a metade: campo do hash corrompido lê como
corrupção, **e** um remapeamento real com cabeçalho íntegro continua lendo
como remapeamento. Sem o gêmeo, a ordem nova poderia mascarar tudo como
corrupção e o primeiro teste passaria igual.

### A versão do mapa é DERIVADA, não incrementada

**Número que alguém precisa lembrar de subir é número que não sobe.** A versão
é um hash (FNV-1a de 16 bits) da concatenação dos **nomes dos params na
ordem**, calculado no `Init`:

- renomear um param muda o hash;
- reordenar muda o hash;
- acrescentar ou remover muda o hash;
- **mudar só a faixa de um param NÃO muda** — e isso é correto, porque o
  preset guarda o normalizado, que sobrevive a uma mudança de faixa.

O último ponto é a razão de o hash ser sobre nomes e não sobre a tabela
inteira: um hash largo demais recusaria presets bons e ensinaria o operador a
ignorar a recusa.

### `n_params` é redundante de propósito

O hash já cobriria. A contagem existe porque **um hash que não bate não diz o
que mudou**, e a contagem diz de que lado procurar. Custa um byte.

### A recusa é gritada, e o que ela diz

```
PRESET RECUSADO: gravado sob o mapa 0x4F2A, este firmware e 0x91C7.
  33 params gravados, 33 esperados — a CONTAGEM bate, entao mudou
  NOME ou ORDEM, nao a quantidade.
  Nada foi carregado. `preset dump` do arquivo velho ainda funciona.
```

**Recusar sem dizer o que fazer é recusar pela metade.** A última linha
importa: o texto continua legível, então nada se perdeu — o caminho é editar
os nomes no arquivo e recarregar.

## 6. O formato de texto, e por que ele é a curadoria inteira

```
# presets.txt — biblioteca de cadeia · mapa 0x91C7
1  corda-shimmer   in_gain=64 gate_thresh=12 drv_on=0 ... shm_on=1 shm_shift=88
2  tambor-seco     in_gain=71 gate_thresh=40 drv_on=1 ... rev_on=1 rev_size=30
```

**Por NOME, não por posição.** Um formato posicional seria mais curto e
transformaria qualquer reordenação do manifest em corrupção silenciosa — o
mesmo defeito do cabeçalho, movido para dentro da linha.

Reordenar = mover linha · renomear = editar texto · deletar = apagar linha ·
**versionar = git · backup = grátis · levar pro próximo instrumento = copiar.**

Mesma filosofia do manifest: *identidade do instrumento é dado, não código.*

## 7. Ordem de implementação, e o que cada passo pode provar sozinho

**FEITO (v0.148.0):** 1, 2 e 3. **Falta:** ponteiros com pré-escuta (4) e OLED (5).

1. **Estruturas + hash do mapa.** Prova: o hash muda ao renomear um param, e
   não muda ao mexer numa faixa. Banco nativo, sem placa.
2. **`capturar` + `preset list`.** Prova: capturar duas vezes com um `set` no
   meio dá presets diferentes.
3. **`preset dump` / `preset load` por serial.** Prova: `dump` → editar → `load`
   → `list` reproduz o editado. E um `load` com mapa adulterado é recusado
   gritando — **exercitar a recusa, senão ela é hipótese**.
4. **Ponteiros A/B + `load` com janela e drenagem.** Prova: carregar não é
   desfeito pelo eco dos widgets (o teste é o `vistos` e o `cc_janela` depois
   de um `load` com o iPad ligado).
5. **OLED**, quando o painel soldar.

**Antes do passo 4, consertar o `sync`** — ele tem o mesmo defeito que o
`load` precisa não ter, e é mais barato provar o conserto no `sync`, que já
existe, do que no `load`, que ainda não.
