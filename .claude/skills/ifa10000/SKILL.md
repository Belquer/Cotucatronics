---
name: ifa10000
description: Protocolo de certificação de todo efeito que entra no Cotucatronics — 10 etapas com critério de passagem explícito, mais a etapa 0 de medição limpa. Use ao embarcar qualquer efeito novo (módulo RNBO ou bloco permanente do shell), ao revalidar um efeito existente, ou quando perguntarem se algo está certificado.
---

# IFÁ10000 — certificação de efeitos

Documento completo: `docs/IFA10000.md`. Histórias que compraram cada etapa:
`docs/battle_scars.md`.

**Todo efeito que entra passa por aqui. Sem exceção, sem atalho.** Efeito que
não passa não entra no build — fica no repo, documentado, com a etapa que
reprovou.

## Como conduzir

1. Abrir `docs/IFA10000.md` e copiar a **ficha de certificação** para o
   relatório da sessão.
2. Rodar a **etapa 0** antes de qualquer número (skill `medicao-limpa`).
3. Percorrer as 10 etapas **na ordem**. Etapa reprovada interrompe — não se
   pula para a seguinte "pra ver se passa".
4. Preencher a ficha com **números**, não com vistos. `[x] ok` não é resultado.
5. Etapa 10 fecha sincronizando repo **e** vault na mesma ação.

## As etapas, em uma linha cada

| # | Etapa | Passa se |
|---|---|---|
| 0 | **MEDIÇÃO LIMPA** | canal provado, ambiente zerado, unidade e regime declarados |
| 1 | CONTRATO | wrapper e `description.json` contam a mesma história |
| 2 | EMBARQUE | boot limpo, nenhum `-1` em ligação de param |
| 3 | MOTOR *(RNBO)* | param mexido ao vivo muda o som na hora |
| 4 | MEMÓRIA | cabe com folga declarada, região justificada no código |
| 5 | CARGA | pico cabe no prazo com folga ≥ 25%, nos dois blocos |
| 6 | MATERIAL | responde ao sinal certo, e o registro diz qual foi |
| 7 | FAIXA | existe tabela de onde satura, medida antes do limiter |
| 8 | SEGURANÇA | decisão de cada param nos búzios escrita no manifest |
| 9 | ACEITE | o dB ouviu ≥ 1 min tocando a escultura e aprovou |
| 10 | REGISTRO | quem clonar amanhã reproduz sem perguntar nada |

## Skills irmãs — o braço executável do protocolo

| Skill | Quando |
|---|---|
| `medicao-limpa` | antes de **toda** medição (é a etapa 0) |
| `instrumentacao` | ao escrever/alterar log, caixa-preta, contadores, sondas |
| `bug-de-ordem` | quando algo funciona às vezes, ou o teste sai incoerente |
| `edicao-segura` | em **toda** alteração de arquivo |

## Regras que valem em qualquer etapa

- **Pico decide, média mente.** Vocoder: média 7,8%, pico 210%.
- **Estreia é o preço da mudança; regime é o aluguel.** Medir separado.
- **Padrão de acesso importa mais que tamanho.** 108 KB em SDRAM = +12,9 pp
  de CPU; os mesmos em SRAM = +5,6 pp.
- **Régua errada produz "não funciona" falso.** Cada efeito tem seu material.
- **Encanamento provado não é feature entregue.** O aceite é do músico.
- **Documento vivo:** efeito novo que revele buraco no protocolo ganha etapa
  nova. O IFÁ10000 melhora a cada certificação.
