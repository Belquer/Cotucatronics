# Fichas tecnicas
Gerado por `tools/fichas.py` v0.9.0 em **2026-09-08** · commit `18729f1` · **ARVORE SUJA**

So entram pecas com MPN real. Resistores por encapsulamento, headers
lisos e modulos comprados montados nao tem MPN, e por isso nao tem
linha aqui — ver `tools/bom_completa.py`.

> **Porque e que os PDF estao no repositorio, e nao um link.**
> A regra da casa e *nenhum facto de hardware sem fonte citada* — e a
> ficha **e** a fonte. Um repositorio que exige citacao e nao guarda o
> documento citado esta a apostar que o link do distribuidor existe
> daqui a tres anos. Nao existe. E o portao que recusa o pacote
> quando uma ficha nao fala da peca so funciona com as fichas ca
> dentro. Sao 13 MB, que em git nao e nada.

| MPN | ficha | estado | nota |
|---|---|---|---|
| `1715721` | — | SEM FICHA | a LCSC nao tem esta peca · **Phoenix Contact — MKDS 1,5/2-5,08, ficha do artigo 1715721** |
| `2920L330/24MR` | [2920L330_24MR.pdf](2920L330_24MR.pdf) | OK | MPN completo no texto |
| `3296W-1-104LF` | [3296W-1-104LF.pdf](3296W-1-104LF.pdf) | FAMILIA | ficha da serie 3296W |
| `53047-0710` | — | SEM FICHA | C17454015 · descarga falhou apos 3 tentativas: The read operation timed out · **Molex — desenho de venda 530470710_sd.pdf. A LCSC tem a peca (C17454015) e aponta para molex.com, que nao respondeu (0 bytes em 120 s, HTTP/2 e 1.1). RETENTAVEL.** |
| `AO3400A` | [AO3400A.pdf](AO3400A.pdf) | OK | MPN completo no texto |
| `B2B-XH-A` | [B2B-XH-A.pdf](B2B-XH-A.pdf) | OK | MPN completo no texto |
| `BAV99,215` | [BAV99_215.pdf](BAV99_215.pdf) | FAMILIA | ficha da serie BAV99 |
| `C1206C223J5GACTU` | — | SEM FICHA | a LCSC nao tem esta peca · **Kemet — serie C0G/NP0 1206, 22 nF 50 V** |
| `EC11E15244G1` | [EC11E15244G1.pdf](EC11E15244G1.pdf) | OK | MPN completo no texto |
| `EEU-FR1C101` | [EEU-FR1C101.pdf](EEU-FR1C101.pdf) | OK | MPN completo no texto |
| `EEU-FR1E470` | [EEU-FR1E470.pdf](EEU-FR1E470.pdf) | OK | MPN completo no texto |
| `EEU-FR1E471` | [EEU-FR1E471.pdf](EEU-FR1E471.pdf) | OK | MPN completo no texto |
| `EEU-FR1V222` | [EEU-FR1C101.pdf](EEU-FR1C101.pdf) | NO CATALOGO | a LCSC nao tem ficha propria; o numero completo esta neste catalogo de serie |
| `MKS2C042201K00KSSD` | — | SEM FICHA | a LCSC nao tem esta peca · **WIMA — serie MKS 2, 220 nF 63 V, passo 5 mm** |
| `N2526-6002RB` | — | SEM FICHA | a LCSC nao tem esta peca · **3M — serie 2526, IDC 2x13 macho blindado** |
| `N3793-6002RB` | — | SEM FICHA | a LCSC nao tem esta peca · **3M — serie 3793, IDC 2x13 femea** |
| `OPA1642AIDR` | [OPA1642AIDR.pdf](OPA1642AIDR.pdf) | OK | MPN completo no texto |
| `PTV09A-4020S-B103` | — | SEM FICHA | a LCSC nao tem esta peca · **Bourns PTV09A — ⚠️ e nesta que o sufixo importa: `S` e eixo estriado e `F` e liso, e ja se escolheu o errado uma vez. Conferir a legenda de sufixos.** |
| `SMBJ24A` | [SMBJ24A.pdf](SMBJ24A.pdf) | OK | MPN completo no texto |
| `SS14` | [SS14.pdf](SS14.pdf) | OK | MPN completo no texto |
| `UES1H220MEM` | — | SEM FICHA | a LCSC nao tem esta peca · **Nichicon — serie UES (bipolar audio), 22 uF 50 V** |

**OK** — o MPN completo aparece no texto do PDF (`pdftotext`).  
**FAMILIA** — a ficha e da serie certa e o sufixo monta-se na tabela de encomenda; ver a citacao abaixo.  
**SUSPEITA** — o ficheiro existe e nao fala da peca; nao confiar.  
**POR CONFERIR** — PDF sem texto extraivel; olhar com os olhos.  
**NO CATALOGO** — sem ficha propria, mas o numero completo esta num catalogo de serie que ja esta aqui.  
**SEM FICHA** — a LCSC nao publica; buscar no site do fabricante.

## O que a ficha prova de cada sufixo

> A ficha do BAV99 tem um `215` no meio dela, e sao **215 mA** de
> corrente, nao o codigo de embalagem `,215`. Conferir sufixo por
> busca de texto da-o por provado com um numero que fala de outra
> coisa — e confirmacao errada e pior que nenhuma, porque fecha a
> duvida.

**`3296W-1-104LF`**  
- prova: W = pacote de ajuste lateral (tabela de dimensoes, 'W series'); 104 = 100 k (tabela de codigos: 104 -> 100000)
- **NAO prova: LF NAO e sem-chumbo — a legenda diz 'LF: High-end product'**

**`BAV99,215`**  
- prova: BAV99 = diodo duplo SOT23 (tabela 3, Ordering information)
- **NAO prova: ,215 e codigo de embalagem e NAO esta nesta ficha ('Packing information removed.') — pedir o documento de embalagem da Nexperia**

**`OPA1642AIDR`**  
- prova: A = grau; ID = SOIC-8; R = fita e carretel — os tres na tabela de encomenda no fim da ficha

