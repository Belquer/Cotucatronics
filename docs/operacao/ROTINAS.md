# ROUTINES — session shortcuts

> Words that dB types and what each one triggers. The routines exist so that
> closing a session doesn't depend on remembering seven steps.

---

## `ud` — full update (end of session)

The closing routine. It runs, in this order:

1. **`bts`** — new scars in the three destinations (`battle_scars.md`, skills,
   vault). Each one with: what happened, the distilled rule, and how it
   generalizes.
2. **Skills** — a new rule goes to the skill where it would be *applied*
   (`instrumentacao`, `edicao-segura`, `medicao-limpa`, `bancada-firmware`).
   If the right skill doesn't exist in the repo, **don't invent where it lives** —
   anchor it to the one that loads alongside and declare it.
3. **`STATUS.md`** — current state, queue, and what was left open.
4. **`FEATURES.md`** — a new feature, a retirement with *why it left* and *how to
   revive*.
5. **📁 `docs/operacao/`** — **this folder**. See the checklist below.
6. **`make check-docs`** — must pass before the commit.
7. **`cp`** — commit and push.
8. **`ppc`** — resume prompt for the next session.

### Checklist of step 5 (the update of this folder)

Not every session touches everything. Check what changed:

- [ ] **Did a pin change?** → `PINOS.md`, read from `hal_daisy_seed.cpp`, not from memory
- [ ] **Did a CC or param change?** → `python3 gerar_ccmap.py > CCMAP.md` (it's generated)
- [ ] **Did hardware, power, or a part change?** → `HARDWARE.md`, with the *why*
- [ ] **Did topology, CTOS, or a serial command change?** → `ARQUITETURA.md`
- [ ] **Did the preset design change?** → `PRESETS.md`
- [ ] **A new rule that cuts across everything?** → the "house rules" section of `README.md`
- [ ] **Did the firmware version change?** → the header of this folder's `README.md`
- [ ] **Did the schematic go stale?** → regenerate or **declare the superseded part**
      (see the note about the SVGs in `README.md` — an undeclared stale doc is the
      most expensive family of bug in this project)

**The rule that justifies step 5:** *every restructuring has three fronts —
code, documentation, and symbols. The compiler charges for the first; `check-docs`
charges for the second within the repo; this folder is the third and only the
routine charges for it.*

---

## `bts` — battle scars

Records a scar in the three destinations. A scar has:

- **the case** — what happened, with numbers and addresses
- **the distilled rule** — the transferable form, that survives the case
- **how it generalizes** — the family it belongs to, or the new family it opens

A scar that only narrates the case isn't a scar, it's an anecdote.

---

## `ppc` — prompt to continue

A cold-resume prompt for Claude Code. It contains: the starting commit and version
(**the known-good declared**), what to read first, the queue in the order
that dB wants, and the house rules that apply to the session.

---

## `cp` — commit and push

A commit with a message that describes the **why**, not the what. Push to `main`.
A clean tree before opening a new front.

---

## `cv` — CV

Updates dB's Bio Master and regenerates the bio/CV outputs. Independent of the
others — it lives outside the instrument's repo.

---

## The triangle

| vertex | role |
|---|---|
| **dB** | vision, decisions, and the **acceptance by ear** — which neither of the other two can give |
| **Claude chat** | strategy, architecture, hypotheses, external audit |
| **Claude Code** | implementation, compilation, measurement on hardware |

**Each vertex can veto the other two with data.** That's how `out_width` was
retired (Code pointed out that M/S makes no sense with two exciters on the same
body), how `learn` was removed (it required the board deaf), and how BOOT_SRAM was
rejected (40% faster, with a latch with no explanation).
