# ppc/004 — cérebro **rev D**: tirar o que não se monta
<!-- doc-check: ignore confere_arcade (função do pcb_contorno.py, em kicad/) -->
<!-- doc-check: ignore y_min (valor da constante FRENTE_CEREBRO, no pcb_contorno.py) -->

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
| `J15` e `J16` passam de **1×8 para 1×4** | header 1×8 → 1×4 | **decidido a 21/09** (bts 15fc): os dois polos da chave estão em paralelo no cobre (`1`=`4`, `2`=`5`, `3`=`6`), logo metade dos pinos é conveniência, não necessidade. Ficam **comum · jack · amp · dreno**, e o paralelo faz-se com jumpers na chave — que é como a montagem que funciona já está feita. Metade do cabo (4 vias em vez de 8) |
| `J11` pino 1 (o conector passa a **1×3**) | header 1×4 → 1×3 | **decidido a 20/09:** o módulo alimenta-se pelo **borne de parafuso**, logo fica o `J2` para a força e o `J11` só para o áudio (referência + L + R). O pino 1 é `+15V` duplicado (bts 15fb) |

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

## §3b — O que a primeira montagem (22/09) acrescentou à lista

Três requisitos que não vêm de limpeza, vêm de bancada — ver `battle_scars.md`
**15fe** e **15ff**:

1. **Pontos de teste no caminho do piezo.** Os onze `TP` da placa cobrem
   alimentação, terras e as saídas de linha; **nenhum** toca a cadeia de
   entrada, que é a mais sensível e a que falhou. A rev D leva, por canal, um
   `TP` na **saída do pré-amp** — que repousa no VBIAS e diz num segundo se o
   `U1` está vivo.
2. **O divisor de entrada com pads para dois valores.** O `C12`/`C22` de 22 nF
   dá −6,9 dB com um piezo de 18 nF, e a bancada precisou de **−17,8 dB**. A
   rev D leva o valor medido como ponto de partida **e** um segundo pad em
   paralelo, porque cada fonte pede o seu — e a serigrafia diz que os dois
   canais podem (e devem) ficar diferentes.
3. **Os `TP` do lado acessível, com o nome impresso** (bts 15fg). No cérebro
   está certo por acaso — ele deita no fundo. **No painel está errado**: os 5
   `TP` estão em `TOP`, virados para o acrílico, e com a referência em `F.Fab`,
   que não se imprime. A próxima rev do painel leva-os para `B.Cu` +
   `B.Silkscreen`, e ganha `TP` nos botões e no encoder, que hoje não têm
   nenhum. O `encaixe.py` **M8** já reprova isto.
4. **A conta de margem escrita ao lado do valor.** O −6 dB original era uma
   estimativa que nenhuma revisão confrontou com o transdutor real. O que fecha
   isto não é outro valor: é a **frase que diz qual piezo, qual acoplamento e
   qual pico medido** justificam o valor.

## §4 — Aceitação (o que prova que está feito)

- `compara_netlist.py` rev C → rev D: as **únicas** diferenças são as redes que
  morreram com as peças removidas. Qualquer rede viva que mude é **defeito**.
- ERC: **0 violações**, e nenhuma isenção nova.
- DRC: **0**, com as zonas reenchidas.
- `git status` da rev C: **limpo**. Se não estiver, o trabalho volta atrás.
- A BOM da rev D não tem nenhuma linha marcada DNP.

## §5 — ⚠️ O QUE BLOQUEIA, e não é burocracia

**Resolvido a 19/09, e ao contrário do que eu escrevi primeiro:** a placa está
**girada 180°** face ao modelo. A prova é a serigrafia do cérebro de cabeça
para baixo com o lado do utilizador de frente; a Daisy sob o botão central não
provava nada, porque cai lá nas duas orientações. Na orientação real, o `C1`
(25 mm) fica sob o furo 1 e o `U2` + `J13` + `J16` sob o furo 2.

**Requisito novo para a rev D, e é o mais importante deste documento:** a
fixação do cérebro é um **retângulo simétrico** (`H1`–`H4`), logo a placa entra
nos dois sentidos e o errado não encontra resistência nenhuma. A rev D leva
**fixação assimétrica ou recorte de chave**: a orientação errada tem de deixar
de caber. Documentar não chega — já estava documentado numa constante, e o
`encaixe.py` recusa se ela mudar, o que fez a ferramenta acreditar na
declaração em vez de perguntar à placa.

**O que ficou a faltar é UMA medida, e ela decide a rev D inteira:** a
**profundidade do botão azul** abaixo do painel. O verificador usa o pior caso
(34 mm, cabeça = 0) e dá **7,5 mm** livres — mas a Daisy tem 17 mm e **coube,
apertada**, o que só é possível se o azul descer **~24 mm**. Com o número real:

- se o azul descer ~24 mm, a folga sob ele é ~17,5 mm, e a rev D **não precisa**
  de recolocar nada por causa dos arcade — a limpeza (§1) é todo o trabalho;
- se descer mais, `U2`, `J14` e `RV1` têm de sair de debaixo dos discos, e a rev
  D passa a ser **recolocação**, com o roteamento a acompanhar.

Medir o botão azul custa um paquímetro e decide entre um trabalho pequeno e um
grande. **Por isso o §3 passo 6 continua bloqueado.**

Outra coisa medida no mesmo dia, que a rev D não resolve mas a próxima caixa
tem de resolver: **o painel só sai com os jacks TRS fora** (LICOES #11).
