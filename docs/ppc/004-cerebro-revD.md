# ppc/004 — cérebro **rev D**: tirar o que não se monta
<!-- doc-check: ignore confere_arcade (função do pcb_contorno.py, em kicad/) -->

> **Estado: PLANO, com execução BLOQUEADA** em três medidas (§5). Escrito a
> 2026-09-19, com a rev C em cima da bancada a ser montada.

- **Data:** 2026-09-19
- **Ramo:** `claude/slack-session-sm2ldz`
- **Pedido do dB (19/09):** *"limpe todos os componentes não utilizados da
  placa"* → opção **(a)**: nova revisão do cérebro. Os **pads de teste ficam**
  (decisão do dB, 19/09): não custam altura nem dinheiro, e são por onde se mede
  a placa quando ela não liga.
- **Pareia com:** `docs/IFA10000.md` (trilha M), `ppc/003` (volume 3D),
  `caixa/LICOES-MONTAGEM.md` #10.

## §0 — A regra que este plano existe para impor

> **A rev C está CONGELADA.** Ela está a ser montada agora, e o manual, a BOM,
> os mapas e o pacote de fábrica são **gerados dela**. Um `git diff` em
> `kicad/cotuca_cerebro/` invalida em silêncio tudo o que já foi impresso.

A rev D nasce como **cópia** (`kicad/cotuca_cerebro_revD/`), e o portão é
mecânico: se a árvore mostrar a rev C alterada, o trabalho está errado, seja
qual for o resultado.

⚠️ **E o `main()` do `pcb_contorno.py` ESCREVE a placa.** Correr
`pcb_contorno.py cerebro` na rev C reescreve o ficheiro fabricado — já
aconteceu uma vez (cicatriz). Na rev D, toda conferência corre em **leitura**,
chamando `confere_*` sobre o texto lido, como foi feito a 19/09 para os arcade.

## §1 — O que sai, e porquê

| ref | o que é | porque sai |
|---|---|---|
| `J4` | header 1×2 "a ponte ESP32" | **vestígio**: a ponte entra no soquete `U3`, que já traz 5 V e GND. O `J4` só repete essas duas redes. Três revisões viram-no e nenhuma perguntou o que liga do outro lado |
| `C11` `C21` | 22 pF C0G | perna de **ganho** do pré-amp, que hoje é seguidor (`R10`/`R20` = 0 Ω) |
| `R13` `R23` | 4k7 | idem |
| `C13` `C23` | 22 µF NP | idem |
| `RV1` `RV2` | trimpot 100 k | idem |

**O que FICA, e é decisão explícita:**

- **`TP1`..`TP9`** — pads de teste (dB, 19/09).
- **O seguidor como está.** Tirar a perna de ganho é tirar **pads vazios**, não
  mudar o circuito: nenhuma rede viva muda de função. Se alguém quiser o ganho
  ajustável de volta, volta na rev E, com medição — não é o que este plano faz.

## §2 — Onde vive cada coisa (medido no repo, 19/09)

| artefacto | quem manda | nota |
|---|---|---|
| `cotuca_preamp.kicad_sch` | **o próprio ficheiro** | ⚠️ o gerador `docs/operacao/esquematicos/revB/gen_preamp.py` está **duas gerações atrás** e **não se corre**: reintroduz `U1A/U1B/U1C` e referências não anotadas. Aqui a regra "o gerador é a fonte" está **invertida**, e o cabeçalho dele já o diz |
| `cotuca_energia.kicad_sch` (tem o `J4`) | o próprio ficheiro | idem |
| `cotuca_conector.kicad_sch` | `gera_conector.py` v0.10.0 | gerado; não é tocado por este plano |
| `cotuca_soquete.kicad_sch` | `gera_soquete.py` | idem |
| `cotuca_cerebro.kicad_pcb` | colocação/roteamento na placa | as peças saem daqui **e** do esquema, ou o netlist deixa de bater |

Consequência: **§3 é edição de `.kicad_sch` à mão**, e vale a `edicao-segura`
(varrer por contagem de parêntesis, nunca por indentação; contar o que se leu).

## §3 — Ordem de execução

1. **Copiar** a rev C para `kicad/cotuca_cerebro_revD/`, com o `.kicad_pro`
   próprio. Registar o commit da rev C de onde saiu.
2. **Medir o ponto de partida**: `kicad-cli sch erc`, netlist, DRC. Estes três
   números são o "bom conhecido" a que tudo se compara.
3. **Tirar os 9 símbolos** do esquema (8 da perna de ganho + `J4`), um de cada
   vez, com contagem antes/depois.
4. **Tirar os 9 footprints** da placa, e o cobre que só os servia.
5. **Reencher as zonas** e correr o DRC.
6. **Recolocar as peças altas** — ⚠️ **§5 bloqueia este passo**.
7. **Portões**: ERC 0 · DRC 0 · `encaixe.py` · `confere_arcade` em leitura ·
   `compara_netlist.py` contra a rev C.
8. **Documentos**: BOM, manual e MANIFEST **continuam a apontar para a rev C**
   enquanto a rev D não for fabricada. A rev D entra como linha nova, não como
   substituição.

## §4 — Aceitação (o que prova que está feito)

- `compara_netlist.py` rev C → rev D: as **únicas** diferenças são as redes que
  morreram com as peças removidas. Qualquer rede viva que mude é **defeito**.
- ERC: **0 violações**, e nenhuma isenção nova.
- DRC: **0**, com as zonas reenchidas.
- `git status` da rev C: **limpo**. Se não estiver, o trabalho volta atrás.
- A BOM da rev D não tem nenhuma linha marcada DNP.

## §5 — ⚠️ O QUE BLOQUEIA, e não é burocracia

A 19/09 o dB teve de trocar a ordem dos botões arcade porque o **C1 (2200 µF)**
ficou alto de mais debaixo de um furo. O modelo diz o contrário: na orientação
declarada (`FRENTE_CEREBRO = "y_min"`) o `C1` está a **64 mm** do botão mais
próximo. O `C1` só cai sob um arcade na orientação **oposta** — a que foi
rejeitada a 06/09 por pôr o `C1` **e a Daisy** sob botões.

Enquanto isto não for medido, **não se sabe onde a rev D tem de pôr as peças
altas** — e recolocar peças pela suposição errada é pior do que não recolocar.

Faltam três medidas, todas do dB, na caixa montada:

1. **Que peça está debaixo de cada um dos três furos de ø30** — em especial, a
   Daisy (`U2`, 17 mm) está debaixo de algum?
2. **Altura real do `C1`** montado, da placa ao topo. Catálogo: 25 mm. Sob o
   botão branco cabem **21,5 mm**; sob um azul, **7,5 mm**.
3. **Que lado da caixa** está de frente quando o dB diz que o branco ficou à
   direita (o lado dos jacks? o da USB-C?).

Com as três, resolve-se a divergência, a `confere_arcade` passa a dizer a
verdade sobre esta caixa, e o passo 6 do §3 deixa de ser adivinhação.
