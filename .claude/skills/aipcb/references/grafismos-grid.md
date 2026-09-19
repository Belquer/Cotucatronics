# Grafismos por GRID — colocação por coordenada, não por arrasto

Referência do **Estágio 6** da pipeline `/aipcb`, para a colocação de **grafismos**
(logo, nome, símbolo, banner decorativo) na serigrafia. **Substitui** o método de
"arrastar no visualizador de gerber", que fica **explicitamente aposentado** no
fim deste documento.

> Grafismo aqui é serigrafia **decorativa** — o que não é referência nem valor
> funcional. As refs/valores seguem o resto do Estágio 6 (`arruma_silk.py`,
> `refs_apontam.py`); este documento é só sobre o que se põe por gosto.

---

## Causa-raiz — por que este método existe

Posicionar um grafismo **por olho / por pixel** — arrastar até "parecer certo" —
custou três coisas nesta sessão:

1. **Re-render em loop.** Cada nudge pedia um novo render para conferir, e o
   ciclo *arrasta → renderiza → ainda não → arrasta* não converge: o olho
   corrige por frações que o gerber nem resolve, e a placa vira um churn de
   commits que só mexem serigrafia.
2. **Colisão com pads.** O banner assentou **sobre os pads MIDI/áudio do U2** —
   invisível no arrasto, porque no viewer o texto **desenhou-se por cima** dos
   pads como se nada fosse.
3. **A armadilha do viewer.** **Arrastar no visualizador de gerber ENGANA**: o
   viewer **NÃO recorta silk sobre pad** — mostra o texto inteiro, bonito, por
   cima do cobre. **A fábrica recorta** (silk sobre pad é removido na produção),
   e o que chega é **texto quebrado**, com letras comidas onde havia pad ou furo.
   O que se aprovou no ecrã não é o que se fabrica.

> **O viewer de gerber mostra o silk que a fábrica vai apagar.** Aprovar
> posição no viewer é aprovar uma imagem que não existe na placa.

---

## Método — em coordenadas, uma passada só

1. **Fecha o design com SÓ silk funcional.** Referências e valores arrumados
   (Estágio 6 normal); **nenhum grafismo decorativo ainda**. O grafismo entra
   por último, contra uma placa já estável.

2. **Gera um render da placa com grid de 0,5 mm.** Linhas maiores rotuladas a
   cada **5 mm**; os **eixos XY no MESMO frame de coordenadas do KiCad** (a mesma
   origem, o mesmo sentido — para o número que se lê no grid ser o número que se
   escreve na peça); as **zonas "no-go" sombreadas** (courtyards + pads + furos +
   **margem da borda ≥ 1,5 mm**); e os **retângulos de espaço livre anotados com
   as dimensões em mm**. O render é o mapa; o grid é a régua.

3. **Declara a bbox de cada grafismo ANTES.** Ex.: *"COTUCATRONICS, altura de
   texto 2,6 mm = **45 × 8,5 mm**"*. Saber a caixa que o grafismo ocupa **antes**
   de o colocar é o que troca tentativa-e-erro por "cabe / não cabe" lido no
   mapa. Um grafismo sem bbox declarada não entra.

4. **O humano aprova a posição LENDO as coordenadas do grid.** Uma passada só:
   olha o mapa, escolhe o retângulo livre, lê a coordenada de canto/centro. Sem
   arrastar, sem "chega um bocadinho pra esquerda".

5. **Transfere o grafismo para as coordenadas exatas.** **1 edição só-silk → 1
   regen de gerber.** O transfer **rejeita silk sobre pad/furo** (é a
   *gerber-truth*: o que a fábrica recortaria é recusado na origem, não descoberto
   no fim). Sem iteração — a posição já foi aprovada no passo 4 contra o mapa
   verdadeiro.

6. **Aposenta o "arrastar no visualizador de gerber".** Não é um passo opcional
   ao lado deste — é o método **substituído**. O viewer serve para conferir o que
   já foi colocado por coordenada; **nunca** para escolher a posição.

---

## Nota de versionamento

Grafismo **altera o gerber** (silk é gerber — ver `references/versionamento.md`).
Mas sob a regra **congela-no-release**, o **rev só sobe quando a placa é
efetivamente fabricada/pedida**. Enquanto a placa não foi mandada, mover ou
colocar um grafismo gera **pacote novo (stamp de commit novo), mesma rev**. A
letra do rev não se mexe por causa de um logo numa placa que ainda não foi à
fábrica.

---

## BEST-EFFORT — o helper `kicad/grid_grafismo.py`

O método acima é o contrato. A ferramenta abaixo automatiza o passo 2 (o mapa).
**Só se commita se realmente funcionar** — ver o estado no cabeçalho do próprio
`kicad/grid_grafismo.py` e no relatório da sessão. Se não for possível deixá-la
**sólida** num tempo razoável, fica um **stub documentado com esta spec**, e o
relatório di-lo **stub** — não se finge ferramenta.

Spec (v0.1.0):

- Renderiza a **face superior** (F.Cu + F.SilkS + F.Courtyard) via
  `kicad-cli`/`pcbnew`.
- Sobrepõe o **grid de 0,5 mm** + **majors de 5 mm rotulados** + **eixos em mm no
  frame do KiCad** (mesma origem/sentido do ficheiro).
- **Sombreia as zonas no-go** (courtyards / pads / furos / margem da borda).
- Imprime os **maiores retângulos livres com dimensões** (para casar com a bbox
  declarada do grafismo no passo 3).
- Usa a `kicad-cli` real e o `pcbnew` de Python.
