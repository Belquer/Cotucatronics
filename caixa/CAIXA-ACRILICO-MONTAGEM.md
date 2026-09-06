# Caixa em acrílico — materiais e montagem
### Cotucatronics · 06/09/2026 · acrílico transparente **3,0 mm**, Ponoko
> Vale para a caixa que já está a ser cortada (calços cegos, rosca no acrílico) e
> para as seguintes (calços com porca cativa — ficheiro `cotuca_caixa_corte.svg`
> regenerado hoje). A diferença está na §4.

---

## 1 · Materiais e ferramentas

| item | qtd | nota |
|---|---|---|
| **Weld-On 4** (SCIGRIP 4) | 1 frasco | solvente fino — todas as juntas a 90° |
| **Weld-On 16** | 1 tubo | gel — só a junta tampa ↔ frente e tampa ↔ trás (11,8°) |
| frasco aplicador com **agulha** | 1 | vem com o Weld-On 4 ou compra-se à parte; sem ele não há capilaridade |
| luvas de **nitrilo** | | cloreto de metileno; latex não serve |
| fita de pintor forte, ou 4 grampos de mola | | segurar a seco e durante a cura |
| esquadro pequeno | 1 | conferir os cantos antes de o solvente pegar |
| **parafuso M3 × 10, cabeça cilíndrica** | 6 | fundo → calços |
| **porca M3** | 6 | só nas caixas com calço de sexágono (série) |
| broca **ø2,5** + macho **M3** | | só na primeira caixa (calços cegos) |
| **espaçador M3 × 10 F-F** + 8× parafuso M3 × 6 | 4 + 8 | carrier no fundo. ⚠️ **10 e não 8**: a placa passa por cima de seis calços de 2 × T = **6 mm**, e o `pcb_contorno.py` pede 9 de mínimo |
| espaçadores para o módulo do amp | 4 | altura conforme o módulo |
| pé de borracha **ø15 adesivo** | 4 | fundo, a 12 mm dos cantos |
| lixa 400 e 600 (à água) | | só se quiseres as arestas polidas; o laser já deixa liso |
| pano de microfibra, álcool isopropílico | | limpar sem riscar; **nunca acetona** |

**Não usar:** super-cola (racha o acrílico em teia branca), epóxi, cola quente,
acetona para limpar (opaca a superfície).

---

## 2 · Antes de qualquer solvente: montar a seco

1. **Película de proteção fica** em todas as peças até ao fim da colagem. O
   vapor do solvente esbranquiça a superfície onde toca.
2. Encaixa a seco: **frente + esquerda + direita + trás**. Os dedos entram à
   mão com atrito leve. Se prender: lixa 400 nas laterais dos dedos dessa junta,
   uma passagem. Se folgar: aponta — é o kerf para a série (`K` no gerador).
3. Pousa a **tampa** por cima, a seco. Os dedos da frente e de trás entram
   enviesados (11,8°) — em acrílico prendem mais que em madeira. Se não
   entrarem com pressão de mão, lixa os cantos superiores dos dedos da frente e
   de trás; **nunca a tampa.**
4. Mede o exterior: 200 × 120, 55 na frente, 80 atrás. Diagonais do fundo
   aberto iguais = esquadro.
5. **Encaixa o fundo por baixo** com as peças montadas. Tem de entrar com folga
   de 0,5. Tira-o.
6. **Encaixa as peças reais nos furos** — arcade, pots, encoder, OLED, jacks,
   DPDT, USB-C — **antes de colar a tampa.** Um furo 0,5 mm fora é uma casca
   nova; agora custa cinco minutos de lixa.

---

## 3 · Soldar a casca (sem a tampa)

1. Desmonta. Monta de novo **frente + esquerda + direita + trás**, sem a tampa.
   Fita em volta. Esquadro nas diagonais.
2. Weld-On 4 no frasco de agulha. **Ponta da agulha ao longo da linha de cada
   junta, por dentro** — o líquido entra por capilaridade. Não pinceles, não
   espalhes antes. Uma passagem lenta por junta.
3. Trinta segundos sem mexer por junta. Quatro cantos, dois minutos.
4. **Nervura:** cola-se à tampa, não à casca — fica para a §4.
5. Uma hora antes de tocar. Vinte e quatro antes de carregar.

## 4 · Calços — a primeira caixa e as seguintes

**Onde:** por dentro, nos 4 cantos e a meio da frente e de trás. Face de baixo
a **T mm da borda inferior das paredes** (3, nesta caixa) — usa o próprio fundo como calibre:
encosta-o por dentro, rente à borda, cola os calços em cima dele, tira o fundo
antes de secar.

**Primeira caixa (calços cegos, já cortados):** empilha 2 quadrados com Weld-On
4, cola o par à parede e ao vizinho. Depois de seco: **furo ø2,5 por baixo,
macho M3**, devagar, sem forçar — acrílico racha por tensão. Vale umas dezenas
de aperto/desaperto.

**Série (calços com sexágono):** cada calço são **dois quadrados diferentes**:
- **CB** (baixo): tem o sexágono de 5,7 mm — cola-se com o sexágono virado
  **para baixo**
- **CT** (cima): liso — cola-se em cima do CB e tapa o sexágono

Antes de colar o CT, **mete a porca M3 no sexágono**. Ela fica cativa: o CT
tapa por cima, o fundo tapa por baixo, e o sexágono não a deixa rodar. O
parafuso enrosca em aço para sempre. Sem furo, sem macho, sem calor.

## 5 · A tampa, por último

Só depois de a §2·6 ter confirmado que todas as peças entram nos furos:

1. **Nervura:** cola-se sob a tampa com Weld-On 4, ao longo dos 200 mm, a
   **y ≈ 62 mm da frente** (entre a fila dos arcade e a dos knobs). Peso em
   cima, 30 min.
2. Weld-On **16** nas ranhuras da frente e de trás da tampa (as de 11,8°).
   Weld-On **4** nas ranhuras das laterais (as de 90°).
3. Pousa, alinha os dedos, peso distribuído (tábua + livro). Limpa o excesso do
   gel com pano seco enquanto está fresco.
4. 24 h. Só então tira as películas.

## 6 · Montar o interior

Tudo entra **pelo fundo aberto**, e tudo com conector — nenhum fio soldado de
placa a painel.

1. **Na tampa, por dentro:** arcade (porca), pots e encoder (porca), OLED (4×
   M2.5). Cabos: JST nos arcade, o de 7 vias no OLED.
2. **Nas laterais e trás:** jacks (porca), DPDT (porca), decoy PD (2 parafusos
   ou fita dupla forte). Fios ao carrier com header.
3. **No fundo:** carrier nos 4 espaçadores M3 × 10; módulo do amp nos dele;
   os dois ligam ao resto por terminal de parafuso e IDC.
4. Liga o IDC do painel, os jacks, os DPDT, a ponte. **Folga de ~80 mm** em
   todos os cabos que vão do fundo à casca — é o que permite abrir sem
   desligar.
5. Fundo para cima, 6 parafusos M3 × 10. Pés adesivos.

### 6b · Percursos de cabo

Vinham do desenho da página 4, que descrevia uma caixa que nunca foi feita — as
cotas dela morreram, estas regras não, porque falam de **relações** e não de
milímetros.

- **Piezos** entram pela esquerda e vão direitos ao pré-amp: **< 60 mm**. É o
  troço de maior impedância do instrumento inteiro.
- **Exciters** saem do amp pela direita, e **nunca cruzam o pré-amp**.
- **IDC do painel** (26 vias) corre pela parede de **trás**, longe da esquerda.
- **Linha para a mesa**: do DPDT (parede de trás) ao amp e aos jacks de trás.
- **Malha do cabo do piezo**: ligada **só do lado da placa**. Ligada nas duas
  pontas fecha uma espira e ela canta.

E a folga: **~80 mm** em todos os cabos que vão do fundo à casca — é o que
permite abrir a caixa sem desligar nada.

## 7 · Trocar ou reparar

| o quê | como |
|---|---|
| carrier, amp, step-down, ponte | 6 parafusos, o fundo desce com tudo |
| arcade, pot, encoder | desligar o JST/fio, porca por dentro, sai por cima |
| OLED | 4× M2.5 por dentro, cabo de 7 vias |
| jack, DPDT | porca por dentro, fio ao header |
| **a casca** | não se repara: corta-se uma nova e transplanta-se o interior |

Regra: **o que custa dinheiro sai; o que fica soldado é acrílico.**

## 8 · O que esta caixa decide para a série

- kerf real em acrílico (`K` no gerador: dedos justos → 0,15; folgados → 0,25)
- se a junta de 11,8° serve com Weld-On 16, ou se a série leva bisel
- se 55 mm de frente chega com os arcade dentro
- se os calços de porca cativa entram no próximo pedido (sim, salvo surpresa)
