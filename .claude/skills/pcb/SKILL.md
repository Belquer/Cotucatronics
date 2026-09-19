---
name: pcb
description: APOSENTADA em 2026-09-19 — fundida na /aipcb. Não usar. Todo o pipeline de ponta a ponta (prompt → gerbers), a orquestração das skills irmãs e a certificação mecânica placa↔caixa (IFÁ10000-M) entram agora pelo /aipcb.
---

Fundida no `/aipcb` em 2026-09-19. Não usar.

Todo o trabalho que corria aqui — o pipeline de 9 estágios (mais a etapa 0 de
pré-voo e o portão 3.5), o Portão de Certificação de Risco, o Portão de
Certificação Mecânica IFÁ10000-M, a orquestração de `kicad_layout`, `caixa`,
`revisao-terra`, `design-review`, `pm` e `para_fabrica` — vive agora no
`/aipcb`, a skill canónica única.

O conteúdo original desta orquestradora (as referências em `references/` e os
scripts em `scripts/`) foi movido para `.claude/skills/aipcb/references/` e
`.claude/skills/aipcb/scripts/`. Ver `.claude/skills/aipcb/SKILL.md`, secção
«Histórico» (v1.9.0), para o registo da fusão.
