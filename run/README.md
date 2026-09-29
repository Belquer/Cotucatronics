# `run/` — os atalhos de duplo clique

Tudo o que se corre com dois cliques vive aqui, e só aqui. Uma linha por
ficheiro, para não ser preciso abrir nenhum para saber qual é qual.

## Bancada — falar com as placas

| ficheiro | o que faz |
|---|---|
| **`consola-sem-fio.command`** | A consola da Daisy **por BLE, sem cabo**. Pergunta que comandos enviar (vazio = só escutar). ⚠️ O macOS mata um processo sem permissão de Bluetooth **sem perguntar** — este é lançado por si, e por isso o sistema pergunta. |
| `monitor.command` | A mesma consola, mas pelo **USB**. Mais rápida e sem lacunas; precisa do cabo. |
| `measure.command` | Mede níveis e picos da placa e imprime o relatório. |
| `medir_pente.command` | Varre o pente de frequências e mede a resposta — o que se usa a afinar o notch. |
| `modo_exciter.command` | Põe a placa no modo de exciter (protecção anti-realimentação ligada) para ensaio. |

## Gravar firmware

| ficheiro | o que faz |
|---|---|
| `flash.command` | Grava a Daisy: compila, guarda o estado, grava, confere a versão na placa e **repõe o estado**. É o caminho normal. |
| `grava_placa_nova.command` | Grava uma Daisy **nova ou perdida**, esteja ela em que estado estiver — instala o bootloader antes. Usa-se uma vez por placa. |
| `test.command` | Compila e corre os bancos de teste nativos (no computador, sem placa). |

## Documentos e fábrica

| ficheiro | o que faz |
|---|---|
| `ccmap.command` | Regenera o `CCMAP.md` a partir do `manifest.h` — a tabela de CCs que o Loopy segue. |
| `ccmap-pdf.command` | O mesmo mapa, em PDF para imprimir. |
| `fichas.command` | Descarrega a ficha técnica de cada peça com MPN real. |
| `auditar.command` | Corre a auditoria de coerência da documentação. |

---

⚠️ **Todos fazem `cd` para a pasta onde o script de verdade vive** (`src/`,
`tools/`, `docs/operacao/`). Se mover um daqui, esse `cd` deixa de apontar
para o sítio certo — e o sintoma é "não encontra o ficheiro", sem dizer porquê.

⚠️ **E o portão do README da raiz confere os caminhos**: um `.command` movido
sem actualizar o que o nomeia é apanhado por `python3 tools/portoes.py`.
