# ROTINAS — atalhos de sessão

> Palavras que o dB digita e o que cada uma dispara. As rotinas existem para
> que fechar uma sessão não dependa de lembrar sete passos.

---

## `ud` — update completo (fim de sessão)

A rotina de fechamento. Executa, nesta ordem:

1. **`bts`** — cicatrizes novas nos três destinos (`battle_scars.md`, skills,
   vault). Cada uma com: o que aconteceu, a regra destilada, e como ela
   generaliza.
2. **Skills** — regra nova vai para a skill onde ela seria *aplicada*
   (`instrumentacao`, `edicao-segura`, `medicao-limpa`, `bancada-firmware`).
   Se a skill certa não existe no repo, **não inventar onde ela mora** —
   ancorar na que carrega junto e declarar.
3. **`STATUS.md`** — estado atual, fila, e o que ficou aberto.
4. **`FEATURES.md`** — feature nova, aposentadoria com *por que saiu* e *como
   reviver*.
5. **📁 `docs/operacao/`** — **esta pasta**. Ver o checklist abaixo.
6. **`make check-docs`** — tem que passar antes do commit.
7. **`cp`** — commit e push.
8. **`ppc`** — prompt de retomada para a próxima sessão.

### Checklist do passo 5 (o update desta pasta)

Nem toda sessão mexe em tudo. Confira o que mudou:

- [ ] **Mudou pino?** → `PINOS.md`, lido do `hal_daisy_seed.cpp`, não da memória
- [ ] **Mudou CC ou param?** → `python3 gerar_ccmap.py > CCMAP.md` (é gerado)
- [ ] **Mudou hardware, energia ou peça?** → `HARDWARE.md`, com o *porquê*
- [ ] **Mudou topologia, CTOS ou comando serial?** → `ARQUITETURA.md`
- [ ] **Mudou o desenho dos presets?** → `PRESETS.md`
- [ ] **Regra nova que atravessa tudo?** → seção "regras da casa" do `README.md`
- [ ] **Versão do firmware mudou?** → cabeçalho do `README.md` desta pasta
- [ ] **Esquemático ficou velho?** → regenerar ou **declarar a parte superada**
      (ver a nota sobre os SVGs no `README.md` — doc velha não-declarada é a
      família de bug mais cara deste projeto)

**A regra que justifica o passo 5:** *toda reestruturação tem três frentes —
código, documentação e símbolos. O compilador cobra a primeira; o `check-docs`
cobra a segunda dentro do repo; esta pasta é a terceira e só a rotina a cobra.*

---

## `bts` — battle scars

Registra uma cicatriz nos três destinos. Uma cicatriz tem:

- **o caso** — o que aconteceu, com números e endereços
- **a regra destilada** — a forma transferível, que sobrevive ao caso
- **como generaliza** — a família a que pertence, ou a família nova que abre

Cicatriz que só narra o caso não é cicatriz, é anedota.

---

## `ppc` — prompt para continuar

Prompt de retomada a frio para o Claude Code. Contém: commit e versão de
partida (**o bom conhecido declarado**), o que ler primeiro, a fila na ordem
que o dB quer, e as regras da casa que valem para a sessão.

---

## `cp` — commit e push

Commit com mensagem que descreve o **porquê**, não o quê. Push para `main`.
Árvore limpa antes de abrir frente nova.

---

## `cv` — currículo

Atualiza o Bio Master do dB e regenera as saídas de bio/CV. Independente das
outras — vive fora do repo do instrumento.

---

## O triângulo

| vértice | papel |
|---|---|
| **dB** | visão, decisões, e o **aceite de ouvido** — que nenhum dos outros dois pode dar |
| **Claude chat** | estratégia, arquitetura, hipóteses, auditoria externa |
| **Claude Code** | implementação, compilação, medição em hardware |

**Cada vértice pode vetar os outros dois com dados.** Foi assim que o
`out_width` foi aposentado (o Code apontou que M/S não faz sentido com dois
exciters no mesmo corpo), que o `learn` foi removido (exigia a placa surda), e
que o BOOT_SRAM foi reprovado (40 % mais rápido, com trava sem explicação).
