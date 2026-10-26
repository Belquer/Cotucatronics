---
name: ifa10000
description: Certification protocol for every effect that enters Cotucatronics — 10 steps with an explicit pass criterion, plus step 0 of clean measurement. Use when shipping any new effect (external module or permanent shell block), when revalidating an existing effect, or when asked whether something is certified.
---

# IFÁ10000 — effect certification

Full document: `docs/IFA10000.md`. The stories that bought each step:
`docs/battle_scars.md`.

**Every effect that enters goes through here. No exception, no shortcut.** An
effect that does not pass does not enter the build — it stays in the repo,
documented, with the step that failed.

## Param audit — three layers, fifteen minutes

Run it whole after any refactor that touches the callback. It found a dead feature
five commits old (scar 12j).

**1. Who reads each param?** Compare the list of `t.Add("nome"` in the manifest with
`params.Real(manifest.p_nome)` in `main.cpp`. A param with no reader is a **dead
feature until proven otherwise** — and it compiles, stays in the table, and keeps
the CC pointing at nothing.

**2. Declared range × internal clamp.** See the section below. Mind the DIRECTION: a
range LARGER than the clamp is dead travel (a defect); a range SMALLER is a musical
choice (it is not).

**3. Live sweep.** Each param at the minimum and the maximum, with a read-back from
the board. It catches what the text does not see: a wrong curve, a clamp in the
path, a param that does not arrive.

## Before anything: does the param's range match the effect's clamp?

A thirty-second question that already caught a fader with **61% dead travel** (scar
12i).

Every effect that clamps a param internally — `if(n > hi) n = hi` — creates a
ceiling the manifest does not know about. The two files are each correct in
isolation and the defect lives in the space between them: the param holds 400, the
effect uses 167, and more than half the fader does nothing.

```
1. find the effect's clamps:  grep -n "if(n > \|if(ms > \|> teto" <efeito>.h
2. compute the REAL ceiling (it may depend on another parameter: in freeze the
   grain's ceiling is a third of the CORE)
3. compare with the min/max of the t.Add() in the manifest
4. range larger than the clamp -> cut the RANGE, do not add a curve
```

**A limit REMOVES the useless from the reach; a curve only redistributes it.** The
distinction is from `dly_time` on 08/18, and it was applied once and not generalized
— which is why it is here, as a routine question instead of a note about one param.

## BEFORE ANYTHING: the single box

**Read `docs/FEATURES.md` in full before adding anything.** Every current feature is
there with its params, CCs and state; the retired ones are in the same document's
archive, with *why it went out* and *how to revive it*.

The question the box exists to force: ***what is already here that I should review
before adding more?***

It cost two ceilings blown in silence on the same day (`kMaxParams` and
`kMaxMidiMap`) — the cowrie shells vanished from the instrument and no one knew until
someone looked at a debug table. And a retired feature with no record becomes
folklore: code that stays there because no one remembers why.

**Retired something? It goes to the archive in the SAME commit.**

## The M-track — boards in enclosures

If the question is **«does this board fit in this enclosure?»** — holes, angles,
sizes, volume, fastening, assembly —, this is not the sheet: it is the **M-track of
IFÁ10000**, which lives in the sub-skill `/3ddesign`
(`.claude/skills/3ddesign/references/ifa10000-m.md`), orchestrated by `/aipcb`. Its
priority order: the board brings its own means of fastening; with that, the
enclosure yields to the board; the board only yields with a written justification
signed by dB.

## How to conduct it

1. Open `docs/IFA10000.md` and copy the **certification sheet** into the session
   report.
2. Run **step 0** before any number (skill `medicao-limpa`).
3. Go through the 10 steps **in order**. A failed step interrupts — you do not skip
   to the next "to see if it passes".
4. Fill the sheet with **numbers**, not with checkmarks. `[x] ok` is not a result.
5. Step 10 closes by syncing the repo **and** the vault in the same action.

## The steps, one line each

| # | Step | Passes if |
|---|---|---|
| 0 | **CLEAN MEASUREMENT** | channel proven, environment zeroed, unit and regime declared |
| 1 | CONTRACT | wrapper and `description.json` tell the same story |
| 2 | EMBARK | clean boot, no `-1` on any param binding |
| 3 | ENGINE *(external engine)* | a param moved live changes the sound at once |
| 4 | MEMORY | fits with declared slack, region justified in the code |
| 5 | LOAD | peak fits within the deadline with ≥ 25% slack, on both blocks |
| 6 | MATERIAL | responds to the right signal, and the record says which it was |
| 7 | RANGE | there is a table of where it saturates, measured before the limiter |
| 8 | SAFETY | the decision for each param in the cowrie shells written in the manifest |
| 9 | ACCEPTANCE | dB listened ≥ 1 min playing the sculpture and approved |

## Each new param must prove that it MOVES something

Existing in the table, receiving a CC and showing up in `list` **is not the same** as
being connected to the sound. On 08/18 `shm_mix` did all three things and scaled
nothing — the mixing function was never called. The effect worked, sounded nice, and
the control turned loose.

**Before closing step 2, for each new param:** move the extreme and prove the sound
changes. If it does not change, the param is disconnected — and the symptom the
musician will report is *"it's quiet"*, which sends you looking for gain instead of
wiring.
| 10 | RECORD | whoever clones tomorrow reproduces without asking anything |

## Sister skills — the executable arm of the protocol

| Skill | When |
|---|---|
| `medicao-limpa` | before **every** measurement (it is step 0) |
| `instrumentacao` | when writing/changing a log, black box, counters, probes |
| `bug-de-ordem` | when something works sometimes, or a test comes out incoherent |
| `edicao-segura` | on **every** file change |

## Rules that hold at any step

- **The peak decides, the average lies.** An FFT module: average 7.8%, peak 210%.
- **The debut is the price of the change; the regime is the rent.** Measure
  separately.
- **The access pattern matters more than size.** 108 KB in SDRAM = +12.9 pp of CPU;
  the same in SRAM = +5.6 pp.
- **The wrong ruler produces a false "it doesn't work".** Each effect has its
  material.
- **Proven plumbing is not a delivered feature.** The acceptance is the musician's.
- **A waiver is a measurement, not an opinion.** Every limitation or deviation that
  is **shipped** in a certification goes into a **risk register** with a MEASURED
  verdict — `SAFE-EMBARK` (with proof), `MUST-FIX`, or a decision by ⟨dB⟩ — **never
  asserted** "looks fine". GO only with zero open MUST-FIX. And you certify against
  a **clean signal**: it is step 0 (`medicao-limpa`) — clean the environment
  artifact before judging the residue, otherwise you certify noise. The `/aipcb`
  skill instantiates this for boards (the risk register as the exit gate of the
  triple review; anchor: the C16↔J14 waiver that measurement turned into MUST-FIX).
- **A living document:** a new effect that reveals a hole in the protocol gains a
  new step. IFÁ10000 improves with each certification.
