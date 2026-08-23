# OPERAÇÃO — Cotucatronics

> **Estado congelado em 22/08/2026** · firmware v0.130.0 · commit `6002a70`
>
> Esta pasta é o **manual de operação**: o que você precisa para ligar, tocar,
> endereçar e consertar o instrumento. Ela é distinta dos documentos de
> *processo* (`STATUS.md`, `battle_scars.md`, `FEATURES.md`, `CTOS.md`), que
> registram como se chegou até aqui.

## Índice

| documento | para quê |
|---|---|
| **[PINOS.md](PINOS.md)** | mapa de pinos — **autoridade final** sobre o que está ligado onde |
| **[CCMAP.md](CCMAP.md)** | os 76 CCs, com faixa real de cada param · **gerado do manifest** |
| **[HARDWARE.md](HARDWARE.md)** | energia, entrada, saída, WIDI, painel, checklist de energização |
| **[ARQUITETURA.md](ARQUITETURA.md)** | topologia de sinal, CTOS, e todos os comandos serial |
| **[PRESETS.md](PRESETS.md)** | o desenho aprovado + o workflow (Loopy constrói, OLED opera) |
| **[ROTINAS.md](ROTINAS.md)** | `ud`, `bts`, `ppc`, `cp`, `cv` — o que cada uma faz |
| `esquematicos/` | SVGs, brief e BOM da carrier board |
| `gerar_ccmap.py` | regenera o CCMAP a partir do `manifest.h` |

## Como usar em cinco minutos

**Ligar:** powerbank PD → decoy 15V → switch. Confere `board` no serial: versão,
memória e pinos saem no boot.

**Tocar pelo iPad:** Loopy Pro, canal 1. A placa responde e **espelha de volta
no canal 2** — a tela do Loopy conta a verdade mesmo depois de gravação ou de
queda do CTOS.

**Se o MIDI não chegar:** `midi` mostra bytes/msgs/aplicadas. Zero bytes →
desconfie do cruzamento TX/RX **antes de tudo** (ver HARDWARE §4).

**Se um widget estiver no CC errado:** `vistos` — a placa escuta, você varre o
Loopy, e o inventário entrega o culpado sozinho.

**Se algo cair sozinho:** `ctos` — quedas por carga e por estouro são
reportadas **separadamente**, porque pedem consertos diferentes.

## As regras da casa

Estas atravessam tudo e valem para qualquer sessão futura:

- **Um voo, uma variável** — e listar os empregos da variável antes de mexer.
- **Toda medição declara REGIME e UNIDADE.** RMS ou pico? norm ou real? qual
  bloco? estreia ou permanente?
- **Delta entre builds não é atribuível.** Custo se mede por A/B no mesmo
  binário, alternando por parâmetro.
- **Nenhum valor do domínio significa "sem dado".** Ausência mora fora do
  domínio — flag separada ou sentinela impossível, nunca um zero que o dado
  pode produzir.
- **O relatório imprime o cru e etiqueta a interpretação ao lado.**
- **A hipótese plausível é a perigosa.** O teste que decide é o que lê a fonte,
  não o que confirma a hipótese.
- **Sempre o cirúrgico** — medido em delta de *comportamento*, não de esforço.
  Atalho cobra 10× depois.
- **Feature nova: o aceite é do músico.**

## Sobre a validade deste documento

Os números aqui foram **extraídos do fonte** em `6002a70`, não da memória.
`CCMAP.md` é gerado; `PINOS.md` foi lido do `hal_daisy_seed.cpp`.

⚠️ **Os SVGs em `esquematicos/` são anteriores ao painel v2.** Os circuitos
analógicos (piezo, WIDI, saída, OLED) continuam válidos; **as anotações de
pino neles estão superadas** — `PINOS.md` é a autoridade. Regenerar quando o
painel for soldado.

**Se este documento e o código divergirem, o código está certo.** O `make
check-docs` cobre a doc do repo; esta pasta entra na rotina `ud`.
