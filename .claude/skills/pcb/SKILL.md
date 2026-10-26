---
name: pcb
description: RETIRED on 2026-09-19 — merged into /aipcb. Do not use. The entire end-to-end pipeline (prompt → gerbers), the orchestration of the sister skills and the mechanical board↔enclosure certification (IFÁ10000-M) now enter through /aipcb.
---

Merged into `/aipcb` on 2026-09-19. Do not use.

All the work that ran here — the 9-stage pipeline (plus the pre-flight stage 0 and
gate 3.5), the Risk Certification Gate, the IFÁ10000-M Mechanical Certification Gate,
the orchestration of `kicad_layout`, `caixa`, `revisao-terra`, `design-review`, `pm`
and `para_fabrica` — now lives in `/aipcb`, the single canonical skill.

The original content of this orchestrator (the references in `references/` and the
scripts in `scripts/`) was moved to `.claude/skills/aipcb/references/` and
`.claude/skills/aipcb/scripts/`. See `.claude/skills/aipcb/SKILL.md`, "History"
section (v1.9.0), for the record of the merge.
