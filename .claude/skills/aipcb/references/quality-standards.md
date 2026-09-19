# Padrões de qualidade — catálogo de falhas + números duros

Referência dos **estágios 3, 5, 6 e 7** da pipeline `/aipcb`. Os números aqui são
todos subordinados à filosofia dos portões da SKILL.md — **um portão que avisa e
deixa passar não é portão**, **medir no sítio certo**, **resultado negativo é
achado**, **a última linha de defesa não é a ferramenta que fez o trabalho**,
**prova que a guarda sabe dizer não**.

---

## 1. Colocação vs estrutura física (estágio 3)

A `caixa` é a autoridade sobre a geometria caixa↔placa e os seus 5 portões; este
estágio **verifica** que a placa honra o que a `caixa` fixou. Constraints a
travar **antes** da colocação, todas **derivadas do modelo** (nunca re-escritas —
cota à mão envelhece contra a calculada):

| constraint | fonte | regra |
|---|---|---|
| contorno | interior da caixa `W−2T × D−2T` | do modelo, não desenhado a olho |
| furos de fixação | invariante de centragem | caem em (15.5·128.5)×(3.5·96.5) do canto para **qualquer T** |
| keepout do furo | cabeça+anilha+chave de fenda | ≥ ⌀ da cabeça, não só o furo |
| conector/botão/display | linha de centro do recorte, mesmo datum | XY + Z travados; eixos knob em 105/140/175 × 92 da face |
| keepout de borda | parede + fab | componentes ≥ **2.5 mm** da borda |
| zonas de altura | altura útil, frente vs trás | altura(footprint) ≤ tecto da zona sob a tampa/arcade |

⚠️ **A origem do footprint não é o eixo.** No `EC11E-Switch` a origem está na fila
de pinos e o veio a (7,50·2,50). Alinhar a origem ao furo põe lá os pinos e deixa
o veio 7 mm ao lado. Lê-se o círculo em `F.Fab`.

**Os três que o DRC 2D não vê** (cada um o seu portão): **volume (Z)** — parte
alta por baixo do arcade passa todo o teste 2D e impede a caixa de fechar;
**acesso** — o USB-C tem de continuar alcançável com a placa montada;
**camada vazia ≠ face vazia** — camada sem objectos não prova a face livre, prova
que ninguém desenhou lá. **Aceitação:** cada feature virada ao painel tem um
*número* dentro do orçamento de tolerância, mais a montagem a seco ⟨dB⟩ de UMA
caixa antes da série.

---

## 2. Aterramento (estágio 5) — números

Invoca `revisao-terra` (P0–P7); estes são os valores duros do lado do layout.

### Regras de layout (`kicad_layout` §11a/§11h)
- **2 camadas + plano:** pour de GND só em **B.Cu**, sinal em F.Cu **sem zona**,
  e uma via em cada pad SMD-GND a descer ao plano. Pour nas duas camadas não
  deixa camada de sinal — todo trilho corta o plano. (Custo pago: **19 vias no
  cérebro, 8 no painel.**)
- **Adjacência de via ≠ ligação:** 0,9 mm de folga isola tão bem como 9 mm. Via
  ao **lado** do pad, nunca no pad (via-in-pad suga solda).
- **AGND é uma ilha** dentro do GND (prioridade maior), com ponte a GND num
  **único** ponto (net-tie). Duas pontes = malha. A ponte tem de ser
  **plano-a-plano, baixa impedância** — um net-tie a 57 mm alimentado por trilho
  fino é meia ponte (topologia certa, impedância errada).

### Desacoplamento por tipo de carga
| carga | conjunto | erro clássico |
|---|---|---|
| lógica (MCU) | **100 nF por pino** + **10 µF** bulk por trilho | nada → ruído difuso |
| pulsada audível (amp classe D) | banco grande **+ 100 nF** nos pinos | banco em falta → zumbido cruzado |
| **entrada de conversor comutado** | **nada de grande** na entrada | banco baixa-ESR → **guincho** 🔴 |
| rádio (BLE/WiFi) | **100 µF + 100 nF** locais | nada → rádio sai pelo altifalante |
| saída de step-down | **100 µF — não 2200** | C a mais mexe na estabilidade da malha |

Regra de ferro: todo consumidor tem **o par** (só o grande passa as arestas, só o
pequeno tem energia rápida — um sozinho é achado); tensão ≥ **1,5–2×** o trilho;
polaridade certa; a poucos mm dos pinos.

### Costura (via stitching)
Passo entre **λ/20 (conservador) e λ/10**, λ = c/(f·√ε_eff) na frequência mais
alta. Costurar em: transições de camada, bordas da placa, gargalos do pour,
retornos de conector, perímetros de blindagem. Não há passo universal — calcula
do topo de frequência. (Para uma placa áudio/MCU lenta, borda+gargalo+retorno
importam muito mais que uma grelha RF fina.)

---

## 3. Serigrafia / rótulos (estágio 6) — números

| parâmetro | recomendado | mínimo absoluto |
|---|---|---|
| stroke da linha | **0,15 mm (6 mil)** | 0,10 mm |
| altura do carácter | **1,0 mm** min; 1,5 preferido | 0,6–0,8 mm |
| silk↔pad / silk↔furo | **≥ 0,15 mm**; silk fora das aberturas de máscara | 0,10 mm |

- **Silk fora de pads/furos.** Silk sobre cobre exposto = defeito: a fábrica corta
  o silk sobre a abertura e o texto desaparece. *Meio designador é pior que
  nenhum — parece um.*
- **Legibilidade:** designadores lidos de ≤ 2 direcções (0° e 90°), nunca de
  pernas para o ar nem espelhados.
- **Polaridade/pino-1:** banda de cátodo, `+` em caps polarizados, ponto/entalhe
  de pino-1 em todo IC — no layer **impresso**.
- **TP:** o nome vem do `(net …)` do pad, **nunca do `Value`** (o `Value` vive em
  F.Fab, não se imprime, e mente se o ponto se move). Bloco de legenda **perto** do
  conector, não a 90 mm.

### A regra que nenhum DRC tem: "cada designador aponta para a SUA peça"
Dona: **`refs_apontam.py`**. O DRC responde "há texto sobre algo?" — não responde
"que peça este texto refere?", porque é significado, não geometria proibida. Uma
ref a 11,5 mm da sua peça e a 5 mm da vizinha passa tudo e **identifica a peça
errada** — o soldador lê `C9` ao lado de `R24` e enfia o cap onde vai a
resistência. Provado nos dois sentidos (defeito injectado; token renomeado para
gritar em vez de aprovar zero peças).

⚠️ **O campo roda com a peça** (`X = fx + dx·cos a + dy·sin a`,
`Y = fy − dx·sin a + dy·cos a`). Um offset cru pôs U2 (180°) 7,2 mm ao lado — e
*não havia nada no sítio errado, por isso a ferramenta disse "livre"*. **O
guarda do movedor mede com a régua do juiz** — `arruma_silk` corre o gate de fab
antes/depois e recusa se o número sobe. Graus: texto×texto = defeito sempre; ref
sobre outra peça = defeito; ref sobre a própria = aviso; ref sobre cobre = defeito.

---

## 4. Catálogo de modos de falha (estágio 7)

Cada linha: o defeito, o check que o apanha, o estágio que o possui.
🔴 destrói peça/perde sinal · 🟠 audível/visível · 🟡 latente · ⚪ higiene.

| # | defeito | sev | check | estágio dono |
|---|---|---|---|---|
| 1 | footprint errado / passo ≠ datasheet | 🔴 | pad map vs datasheet; revisão DFM | B — DFM |
| 2 | pin-mapping errado (símbolo genérico em footprint fixo) | 🔴 | `funcao_dos_pads.py` (`--sabota`) | A — Eléctrica |
| 3 | peça polarizada invertida | 🔴 | contrato de polaridade + pino-1/cátodo silk + preview de rotação | A + C |
| 4 | rotação de conector errada no CPL | 🟠 | BOM↔CPL↔placa + **preview ⟨dB⟩ peça-a-peça** | B — DFM |
| 5 | silk sob pads / sobre cobre | 🟠 | contagem `silk_over_copper`; `arruma_silk` com 4 classes | C — Silk |
| 6 | designador aponta a peça errada | 🟠 | `refs_apontam.py` | C — Silk |
| 7 | thermal esfomeado / spoke fino demais | 🟠 | `starved_thermal`; spoke ≥ 0.09/0.16 mm por peso | B — DFM |
| 8 | ilha por rotear / plano estilhaçado | 🔴 | não-roteado por (rede, espécie); `Zone<->Zone` só indicação | A — Eléctrica |
| 9 | pad órfão do plano (trilho cortou) | 🔴 | perfil por rede (o total esconde) | A — Eléctrica |
| 10 | largura insuficiente para potência | 🔴 | IPC-2221 de I, ΔT; larguras nas classes de rede | A/B |
| 11 | desacoplamento errado / em falta | 🟠 | `revisao-terra` P4 — o *par* por consumidor | Terra |
| 12 | laço de terra / AGND-DGND juntos 2× | 🔴 | `revisao-terra` P3 (continuidade ilha↔trilho aberta) | Terra |
| 13 | retorno cruzando fosso sob sinal rápido | 🟠 | `revisao-terra` P5/P6; retorno com o sinal; costura λ/20 | Terra |
| 14 | furos ausentes/mal postos; parafuso sob peça | 🔴 | `caixa` Gate 5 `confere_furos` | Caixa |
| 15 | sobreposição de courtyard | 🟠 | DRC courtyard; IPC-7351B 0.25 mm; mede por courtyard não por pad | B — DFM |
| 16 | USB não chega ao painel; peça alta sob a tampa | 🔴 | face-Z + acesso + zona de altura (§1) | Caixa |
| 17 | contorno ≠ interior da caixa | 🔴 | contorno importado do modelo; invariante `pcb_contorno.py` | Caixa |
| 18 | TP anónimo / rótulo mente | 🟡 | rótulo do `(net)` não do `Value`; bloco perto do conector | C — Silk |
| 19 | folga de borda / peça fora da placa | 🟠 | keepout ≥2.5 mm; limites do router = **contorno** não extensão das peças | B/C |
| 20 | pacote não diz de que estado do repo veio | ⚪→🔴 | `para_fabrica` carimba data/commit/dirty; `INCOMPLETA` | B — DFM |

---

## Números-chave (referência rápida)

**Serigrafia:** stroke **0,15 mm** (0,10 chão) · altura **1,0 mm** (0,6–0,8 chão)
· silk↔pad **≥ 0,15 mm**, fora das aberturas de máscara.

**Costura:** passo **λ/20 → λ/10**, λ = c/(f·√ε_eff) no topo de frequência.

**Largura (IPC-2221):** `A(mil²) = (I/(k·ΔT^0.44))^(1/0.725)`, largura = A/espessura;
k = **0.048 externo**, **0.024 interno**; 1 oz = 1.378 mil, 2 oz = 2.756 mil.

**Desacoplamento:** lógica **100 nF/pino + 10 µF** · RF **100 µF + 100 nF** ·
saída step-down **100 µF (não 2200)** · entrada de comutado **sem banco baixa-ESR**
🔴 · tensão ≥ **1,5–2×** o trilho, a poucos mm dos pinos.

**Courtyard/espaço/térmica/borda:** excesso IPC-7351B **0,10 (L) / 0,25 (N) /
0,50 (M) mm** · borda **≥ 2,5 mm** · spoke térmico chão JLC **0,09/0,09 (1 oz),
0,16/0,16 (2 oz)** · furos de fixação **2,5–5 mm** ⌀, keepout ≥ varredura da
cabeça/anilha.

**Factos do projecto (carregados, não re-derivados):** 2 camadas: pour B.Cu só,
sinal F.Cu sem zona, via por pad SMD-GND (custo 19/8 vias); furos em
(15.5·128.5)×(3.5·96.5) para qualquer T; eixos knob 105/140/175 × 92 da face.
