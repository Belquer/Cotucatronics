---
name: aipcb
description: Levar um circuito de uma especificação em linguagem normal até um pacote de fabrico encomendado (Gerbers, BOM, CPL) — "prompt in, PCB out" — com Claude a propor e verificar, o KiCad a dar o veredicto, e três vetos humanos que não se automatizam. Use SEMPRE que aparecer /aipcb, "nova placa", "rev C", "manda para a JLC", "redesenha o cérebro", uma espec de circuito, uma mudança de esquema, ou qualquer pedido que termine em Gerber. Substitui o /kicad (aposentado em 09/09/2026): o /kicad descrevia geradores que escreviam ficheiros KiCad à mão; o /aipcb descreve um fluxo em que ninguém escreve ficheiros KiCad à mão.
---

# /aipcb — do prompt à placa

> **O veredicto vem do KiCad. O significado vem da ficha. O agente propõe e
> verifica; não serializa.**

Uma frase, três cláusulas, e cada uma é uma semana que já se pagou:
- *o veredicto vem do KiCad* — ERC, DRC, netlist, ratsnest, Gerbers: `kicad-cli`
  e a API IPC. Nunca um leitor nosso a decidir "está ligado".
- *o significado vem da ficha* — pino 2 é `−IN` porque a folha de dados do
  OPA1642 o diz, não porque o símbolo desenhou um sinal em baixo.
- *o agente propõe e verifica; não serializa* — nenhum `.py` escreve
  `.kicad_sch` ou `.kicad_pcb`. Escreve-se pela API, pela GUI, ou não se escreve.

---

## 0 · O que este fluxo é, e o que não é

**É:** uma espec entra, um pacote de fabrico sai, em horas. Entre os dois há
ferramentas que produzem e verificam, e **três portas** onde tudo para e espera
por um humano — porque foram os únicos olhares que apanharam um encoder no ar,
um conector fora da placa e o silk a sair do bordo.

**Não é:** sem vetos. Automatiza a produção e a verificação. Não automatiza o
julgamento.

**Quem faz o quê:**

| | faz | não faz |
|---|---|---|
| **chat** | espec → ppc; decisões de circuito; roteiros do Cowork; **varredura independente** nas portas 2 e 3 | não afirma facto de hardware sem fonte; não escreve ficheiros KiCad |
| **Code** | API (`kipy`/Konnect), `kicad-cli`, os três verificadores nossos, o pacote, o banco de prova | não patch de texto em `.kicad_*`; não "confirmar" o que não mediu |
| **Cowork** | o ecrã, **só por roteiro** determinístico: KiCad onde não há API, JLC, Ponoko | não improvisa, não desenha com o rato livre |
| **humano** | os três vetos, o multímetro, pagar | não solda com o circuito ligado; não refaz terra sem a Daisy fora |

---

## 1 · A entrada: uma espec tem de trazer

Sem estes, o fluxo **não começa** — devolve-se a espec com o que falta:

1. **Função** em uma frase, e o que é facto de exemplar (varia por unidade da
   série: ganho de exciter, C2 do piezo) vs facto de série.
2. **Peças fixas** com MPN **da ficha**, não da memória — e a ficha vai para
   `docs/datasheets/` no mesmo commit. MPN sem ficha aberta é hipótese e diz-se.
3. **Redes** com nome humano (`AUDIO_IN_L`, não `Net-(U1A-+)`), e quais são
   contratos entre placas (o IDC) ou com a caixa (furos, conectores de bordo).
4. **Restrições**: caixa (contorno, furos, alturas por zona), camadas, potência
   por rede, o que é ilha analógica e onde está a ponte única de terra.
5. **O que se mede na bancada** antes de fixar (valor do resistor do LED, C2 do
   piezo, tipo do DPDT) — com a medição escrita como passo, não como nota.

---

## 2 · A fonte

**KiCad, biblioteca oficial.** O símbolo oficial traz a função por pino
(`+`, `−`, `G`, `S`, `D`, `K`, `A`), o footprint oficial traz os nomes de pad
(`A B C S1 S2` no EC11). C1, C2 e C4 da rev B não podiam ter existido.

**Símbolo próprio só para o que não existe** (módulos: Daisy, SuperMini,
decoy) — e obrigatoriamente com **pinos com nome de função**, ficha ao lado,
e entrada no `funcao_dos_pads`. Um símbolo com pinos `1 2 3` sem nome é
defeito, não atalho.

**Se a Fase 1 do banco de prova provar que o atopile recusa C1/C2/C4 sozinho:**
a fonte do esquema é o `.ato`, o KiCad é só layout, e **ninguém edita o esquema
no editor** — qualquer clique é perdido na compilação. Decisão escrita aqui,
uma vez; não se revisita a meio de uma rev.

**O que nunca mais:** geradores que escrevem `.kicad_sch`/`.kicad_pcb`. Os da
rev B vivem em `docs/rev_b/geradores/` como registo de como a rev B nasceu.

---

## 3 · As três portas

Cada porta tem **o que exige para abrir**. Nada passa por "está quase".

### Porta 1 — esquema congelado
- `kicad-cli sch erc` = 0 (isenções só com prazo, e o prazo escrito)
- `funcao_dos_pads` = 0 nas duas placas
- contratos: `contrato_idc` 26/26 (ou o que for), redes de caixa nomeadas
- revisão de terra (skill `revisao-terra`) feita sobre **este** esquema
- render do esquema visto pelo humano
- **O humano diz "congela".** A partir daqui, mudança de esquema é rev seguinte.

### Porta 2 — render antes de rotear
- peças fixas (furos da tampa, conectores de bordo, furos M3) colocadas pela
  API nas coordenadas **derivadas** da caixa, não escritas à mão
- `paridade_caixa` = 0; courtyards **dentro** do contorno com ≥ 0,5 mm;
  furos com raio de exclusão ≥ 3,5 mm a qualquer courtyard
- render 2D das duas placas + o 3D (`product-view`) com a caixa
- **varredura independente** (chat) sobre os ficheiros, não sobre o relatório
- **O humano veta ou não.** Só depois: terra e áudio à mão, resto Freerouting.

### Porta 3 — pré-visualização antes de pagar
- `kicad-cli pcb drc` = 0 com silk-sobre-cobre e silk-fora-da-placa ativos
- ratsnest do KiCad = 0 por rotear (não um contador nosso de proximidade)
- zonas re-preenchidas **neste** ficheiro (fill guardado; zero `filled_polygon`
  é placa sem terra)
- `funcao_dos_pads` = 0 outra vez (o roteamento não muda redes; a sincronização
  pode ter mudado)
- BOM = CPL = placa em designadores; código LCSC em **toda** linha que a JLC
  monta; MPN pode ser vazio, código não
- varredura independente completa
- pacote sobe à JLC (Cowork), pré-visualização 3D aberta na lista de peças
  polarizadas do `LEIA-ME.txt`
- **O humano confere as polaridades e paga.**

---

## 4 · Ferramentas por estação

| estação | ferramenta | regra |
|---|---|---|
| ler ficheiros sem KiCad aberto | `rjwalters/kicad-tools` ou o nosso `kaudit.py` | só para **ler**; delimitar por parêntesis, nunca por contagem de caracteres |
| esquema | editor KiCad (humano/Cowork) · `kicad-mcp-kipy` para ler · atopile se a Fase 1 disser | biblioteca oficial |
| placa | `kicad-python` (`kipy`) · Konnect | SWIG `pcbnew` está em manutenção: não se escreve código novo nele. ⚠️ **O socket é por processo**: `/tmp/kicad/api.sock` é o gestor de projecto e recusa tudo com `no handler … GetOpenDocuments`; o editor de placa é `api-<PID>.sock`. Usar `kicad/_v2/liga.py`. (15ev) |
| verificação | `kicad-cli sch erc / pcb drc / export netlist` | é o veredicto |
| roteamento | terra e áudio à mão; Freerouting via Konnect para o resto | nunca "faz_placa do zero" numa placa já colocada |
| peças | LCSC/JLC via API; ficha lida com `pdftotext` e a **tabela de encomenda** conferida por sufixo | busca difusa responde com confiança ao que não sabe: filtrar por descrição estrita, não por ordem |
| pacote | `kicad-cli pcb export gerbers/drill/pos`, BOM do KiCad | nada escrito por nós no pacote |
| ecrã | Cowork por `docs/roteiro/NNN.md` | hotkeys, valores em diálogos, coordenadas digitadas |
| revisão externa | kicad-happy, Konnect audits | segunda opinião; **não** portão |

**Os três nossos, e só estes:**
- `funcao_dos_pads.py` — por referência, da ficha: pad → rede → função
  esperada. Contrato "estes pads na mesma rede", nunca nome auto-gerado.
- `contrato_idc.py` — via a via entre placas, pela netlist do KiCad.
- `paridade_caixa.py` — contorno, furos e conectores de bordo contra o
  gerador da caixa (`caixa/cotuca_caixa_corte.py`), por courtyard.

Mais `svg_colisoes.py` sobre o silk exportado, e a **varredura** (`kaudit` +
`conn` ou equivalente) como leitura independente.

---

## 5 · Portões: a pergunta que respondem, e a que NÃO respondem

Todo verificador entra nesta tabela com as duas colunas. Foi a coluna da
direita que faltou na rev B.

| verificador | responde | NÃO responde |
|---|---|---|
| ERC | tipos de pino compatíveis, rótulos soltos | direção de díodo, função de pino, D/S trocado |
| DRC | folgas, larguras, silk, courtyard×courtyard | courtyard×bordo (ativar), zona por preencher, pad sem rede |
| ratsnest 0 | tudo o que tem rede está ligado | pad **sem** rede (símbolo↔footprint desencontrados) |
| "sincronizado" | as peças batem | as **redes** batem — só se o sincronizador refrescar redes |
| `funcao_dos_pads` | função por pad da ficha | posição, roteamento, valores |
| `contrato_idc` | vias iguais entre placas | ordem do cabo do OLED (é do módulo) |
| `confere_furos` | courtyard fora do círculo de 3,5 | nada além de furos |
| kicad-happy | contagem e forma | leitura de significado ("50/50 têm MPN" eram descrições) |
| render / olho | o que os números não contam | nada que exija medir |

**Regra:** um portão que "passa" é aceite só depois de o ter visto **recusar**
sob sabotagem. Verde na primeira corrida é suspeito.

---

## 6 · Formato das mensagens

Numeração partilhada `NNN`, um por mudança de desenho, tudo no repo:

- `docs/ppc/NNN-titulo.md` — chat → Code. Contexto (2 linhas), passos com
  **prova por passo**, aceitação medida, o que não mexer.
- `docs/relatorio/NNN.md` — Code → chat. Fechou (com prova) · não fechou
  (porquê) · **decisões que precisam do humano** (marcadas) · commits.
- `docs/varredura/NNN.md` — chat, sobre os ficheiros, nas portas 2 e 3.
- `docs/roteiro/NNN.md` — chat → Cowork. Passos determinísticos, um por
  linha, com o estado esperado no fim de cada um.
- `STATUS.md` — a fila, uma página, o primeiro ficheiro de qualquer sessão.

Com Slack (Claude Tag em Team/Enterprise; Claude Code in Slack em Pro/Max): o
ppc é a mensagem, o relatório é a thread, o commit fica ligado. Os ficheiros
ficam na mesma — o Slack é transporte, o repo é memória.

**Nada que se decida vive só na conversa.** Se não está num destes ficheiros,
não aconteceu.

---

## 7 · Cicatrizes que sobrevivem (uma linha cada)

- **Símbolo genérico casado com footprint real: a netlist fica certa e a
  função fica errada; nenhum portão de ligação o vê.** (15C1–C5)
- **Um verificador responde à pergunta que lhe fizeram.** "Sincronizado" eram
  peças, não redes; "livre" eram pads, não corpos; "zona em F.Cu" era keepout.
- **"Perto" não é "ligado".** A via do D21 a 0,21 mm da pista: só o ratsnest
  do KiCad decide.
- **MPN sem abrir a ficha é palpite com número de peça.** `-4` vs `-1`,
  `1935161` vs `1715721`, `LF` que não é lead-free, `215` que são mA.
- **Facto de hardware sem fonte não entra em documento nenhum.** AGND/DGND
  "por dentro da Daisy" custou uma Seed3.
- **Lista à mão ao lado de coisa gerada envelhece em silêncio.** A tabela do
  IDC no roteiro, o `usados`, o cabeçalho de versão.
- **Cópia de emissor sobrevive às mudanças do original.** Chamar, não copiar.
- **Leitor que delimita por contagem de caracteres não é leitor.** Cinco
  sondas rápidas truncaram no mesmo dia; o definitivo delimita por parêntesis.
- **Data é dado gerado, não escrito.** Hook no Code; `date` no chat.
- **Mudança a meio do roteamento é a que custa.** Terceiro arcade, 4→2
  camadas, 6→3 mm: congelar antes, rev seguinte depois.
- **O render é a última verificação que vê o que os números não contam.**
  Encoder no ar, conector fora, silk no bordo — dez segundos numa imagem.
- **Não reconstruir a ferramenta que já existe para evitar aprender a usá-la.**
  Dezassete geradores para não clicar no KiCad.
- **«Fica para a pessoa» tem a mesma cara quando a placa está cheia e quando a
  malha não tem nó no vão.** O passo tem de dividir 1,27. (15eq)
- **Zeros idênticos em cima de tudo são leitor partido, não placa.**
  `SHAPE_LINE_CHAIN.Distance()` dá 0 dentro de uma cadeia fechada. (15er)
- **Canto arredondado: a caixa envolvente mente.** Courtyard contra o
  polígono do bordo, vértice a vértice. (15er)
- **«Existe» e «corre» são afirmações diferentes.** O `sincroniza_pcb` não
  tinha chamador: a cadeia nunca lia o esquema. (15es)
- **Guarda de nomes vê chaves; só a ficha vê significado.** Encoder
  apanhado por uma, MOSFET só pela outra. (15et)
- **A função de um conector está na peça do outro lado do cabo.** OLED com
  as sete posições trocadas e todos os portões verdes. (15eu)
- **…e do cabo que existe.** Cabo directo: o contrato é a posição. Jumpers
  soltos: é o nome de cada fio. Antes de chamar divergência a outro ramo,
  ler o commit que a criou. (15ew)

---

## 8 · Primeiro uso

`ppc/001 — banco de prova`: copiar as duas placas da rev B **de antes das
correções** para `kicad/_lab/`, instalar a pilha (KiCad 10 + IPC, `kicad-python`,
Konnect, `kicad-mcp-kipy`, atopile, kicad-happy), correr cada revisor contra
os seis defeitos conhecidos, e devolver a tabela *seis defeitos × revisores*.
O que nenhum apanhar é o que continua a ser nosso — e é a lista final da §4.
