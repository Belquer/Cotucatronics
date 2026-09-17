# Montagem em casa — chapa quente + o portão de custo

Referência do **Estágio 8** da pipeline `pcb`, e do portão humano ⟨dB⟩ que decide
**quem monta a placa**: a JLC (montagem SMT), ou tu, em casa, na chapa quente.
Subordinada à mesma filosofia dos portões — um portão que só sabe uma resposta
não é portão. A montagem em casa é a **segunda saída**, e há um número que decide
qual delas se toma.

> A JLC-assembly é cómoda mas o seu preço a **baixa quantidade** é dominado por
> **custo fixo** — setup da linha, o stencil, o markup das peças, e a engenharia
> (extended-parts, feeders). Para 5 placas, esse fixo pode passar muitas vezes o
> valor do cobre nu + as peças a granel. Aí a montagem em casa não é economia de
> tostões: é a ordem de grandeza certa.

## 1. O portão de custo — quando ir para casa

**Regra de ouro:** se, para a quantidade que precisas, a **montagem** (a linha da
JLC: setup + stencil + markup de peças + engenharia) **>> (PCB nua + peças a
granel)**, encaminha para a **via de casa**. Não é o preço absoluto que decide, é
a **proporção**: quando o fixo domina, a quantidade é pequena demais para pagar a
linha.

- **A favor da JLC-assembly:** quantidade que dilui o fixo; passo fino (QFN/BGA)
  que a chapa+mão não colocam com fiabilidade; muitas placas iguais.
- **A favor de casa:** protótipo / série curta; a maioria das peças em 0805/1206
  (fáceis à chapa); poucas polarizadas; já tens chapa quente + stencil + pasta.

O portão corre no ⟨dB⟩ do Estágio 8, ao lado da vistoria de rotação: **antes de
pagar a montagem, comparar a cotação da montagem contra (nua + peças).** Se o
fixo domina → **PCB nua + stencil** e monta-se em casa.

## 2. O bónus escondido — a via de casa torna o risco de rotação MOOT

O perigo dos **180°** (a biblioteca da JLC roda o modelo dela vs. o nosso
footprint — ver `portao-humano.md`) existe **porque uma máquina coloca a peça sem
olhar para a nossa render**. Em casa, **o operador és tu**, e colocas cada peça a
olhar para a **render 3D do nosso próprio board** como mapa (o `kit/MAPA_COLOCACAO.md`
+ a `VISTORIA.md`). A fonte-da-verdade e quem coloca são a **mesma** — a rotação
da biblioteca da JLC deixa de poder trair, porque a biblioteca da JLC não entra na
montagem. Isto **não** dispensa a vistoria de polaridade (banda/cátodo/pino-1
continuam a poder ir ao contrário se colocares mal), mas remove a classe inteira
do «180° da biblioteca».

## 3. O fluxo de casa — a ordem que não queima peça

1. **Encomenda: PCB nua + stencil, com _Assembly OFF_.** No JLCONE, pede a placa
   nua e o **stencil** (framework/frameless conforme a tua chapa) e **desliga a
   montagem**. Sem montagem, o pacote de fábrica é só gerbers+stencil — o `bom`/
   `cpl` da JLC não são precisos para a fábrica, mas o **`kit/`** é o que TU usas.
2. **Peças pelo carrinho da LCSC.** O `kit/COMPRA_LCSC.csv` é o carrinho pronto —
   uma linha por peça física, `Qty` + `Qty+spares` (leva sobresselentes: peças de
   chapa perdem-se e queimam-se). As THT e as que não vão pela chapa compram-se na
   mesma lista ou ao distribuidor (pots, radiais, módulos).
3. **Stencil + pasta Sn63/Pb37.** Alinha o stencil sobre a placa, espalha a
   **pasta de solda Sn63/Pb37** (eutéctica, 183 °C — mais tolerante que a
   sem-chumbo para a chapa manual), levanta o stencil na vertical.
4. **Colocar por MAPA.** Coloca o SMD do **TOPO** seguindo o `kit/MAPA_COLOCACAO.md`
   — agrupado por valor (todos os de um valor de uma vez), TOP e BOTTOM separados,
   com bottom e THT sinalizados. A render do nosso board é o mapa de orientação
   (§2).
5. **Refluxo do SMD do TOPO na chapa quente.** Placa na chapa, sobe até a pasta
   colapsar e brilhar (curva do perfil da pasta), segura o pico, arrefece. **Só o
   TOPO** vai à chapa.
6. **Peças do lado BOTTOM à mão.** As peças do fundo **cairiam na chapa** se
   tentasses refluir o board ao contrário — **soldam-se à mão**, uma a uma, depois
   do topo estar feito. O `MAPA` lista-as em separado (ex.: `C16` no cérebro).
7. **THT por último, todas à mão.** Conectores, pots, radiais, módulos (Daisy,
   ESP32) — depois do SMD todo, para não estorvarem a chapa nem o acesso.
8. **Inspecção com lupa.** Juntas de **SOIC/SOT** ao conta-fios: pontes entre
   pinos, junta seca, tombstone de passivo, pino levantado. As polarizadas
   conferem-se contra a `VISTORIA.md` (banda/cátodo/pino-1).

## 4. O que o pipeline já te entrega para isto

O `kicad/kit_montagem.py` (v1.0.0) é o **entregável de casa**: dá-lhe a pasta do
pacote e emite o `kit/` — `MAPA_COLOCACAO.md`, `COMPRA_LCSC.csv`, `LEIA-ME-KIT.txt`
(ver `fab-automation.md §5`). É saída-padrão do Estágio 8 **ao lado** do pacote de
fábrica, precisamente para que a via de casa não dependa de trabalho manual a
cada corrida.
