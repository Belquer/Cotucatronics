---
name: edicao-segura
description: Source-editing discipline in Cotucatronics — anchor checked before, grep after, semantic version bumped and reported. Use on EVERY file change in the project. Silently failed edits have already cost an entire instrumentation cell and two document entries that I reported as written and that never existed.
---

# Safe source editing

**An edit that fails silently is worse than one that breaks the build** — it lets
you report as done something that does not exist.

It has already cost: two entries of `battle_scars.md` reported as written and never
inserted (wrong anchor, no verification); and an entire instrumentation cell whose
absence showed up as `VOLTAS=0, nan` — the code had never made it into the file.

## Protocol

### 1. Before — is the anchor unique?

```python
assert s.count(ancora) == 1, "ancora <nome>"
```

Check **before writing**. If the assert blows up, nothing was written and the file
remains intact — that is the desired behavior, not a process failure.

An anchor with 0 occurrences = the text you imagined does not exist.
An anchor with 2+ = you do not know which one you will change.

### 2. After — confirm in the file

```bash
grep -n "<trecho novo>" <arquivo>
```

**Never report an edit without having done the grep.** The report comes from the
file, not from the intention.

### 3. Compile before reporting

A change in `.cpp`/`.h` is only "done" after a clean `make`. Common errors that only
the compiler catches: a file-scope function using `hal::` without the
`using namespace cotuca;`, and a helper defined after its caller.

### 4. Semantic version — always

Every file touched bumps and **exposes the version at runtime**:

- MAJOR: an incompatible change (protocol, signature, removal)
- MINOR: a compatible addition (new command, new tunable)
- PATCH: a fix that does not change behavior

`main.cpp` carries `COTUCA_OS_VERSION` **and** the header comment — **both** need to
change together.

### 4b. Check the VERSION — in the file and on the board

**Mandatory routine**, and it exists because it failed three times in a row on the
same day: I announced fixes as flashed while `main.cpp` stayed stuck on an old
version, because the edits died in an anchor `assert` and I only looked at the result
of the *flashing*.

```bash
head -3 main.cpp | grep esqueleto     # o que o fonte diz
# e depois, na placa:  `board` imprime  Cotucatronics OS vX.Y.Z
```

**The two must match.** If the source did not bump its version, the edit did not go
in — no matter that the flashing said `GRAVOU`.

> **Tool output is not confirmation of intent.** The loop confirms that it flashed.
> The `assert` confirms that the anchor matched. The `grep` confirms that the text
> went in. They are three different confirmations — do not treat the first as if it
> covered all three.

**Corollary:** the better the protection against silent editing, the easier it is not
to notice that it acted — the effect of a blocked edit is the file staying the same,
and an unchanged file makes no noise.

### 5. Report

Say **which files changed and what version they ended up at**. Always.

## The range-deletion rule

**Never delete a range whose bounds you computed without listing what is inside it.**
Twice on 20/08 this cost code and prose (scar 12e).

The compiler witnesses **symbol dependencies, and nothing more**. So:

> **The danger of a range deletion is proportional to how much of the deleted content
> is not referenced by name elsewhere.**

The two classes that vanish silently:
- **write-only code** — instrumentation, counters, log, side effect. It produces *a
  zero that looks like data*.
- **prose** — comments that explain a why. It vanishes even more quietly, because no
  one ever notices a missing paragraph.

**Antidote, two steps:**

```python
m = re.search(padrao, s, re.S)
print(repr(s[m.start():m.end()]))          # imprime o que casou
print(s[m.start():m.end()].count("\n"))    # confere a contagem
# só então apagar
```

`.*?` with `re.S` can traverse the entire file depending on the lookahead. **The
failure is running a destructive operation with a matcher of unpredictable extent** —
not the unexpected match.

**Recovery:** `git show HEAD:arquivo` and surgical re-paste.
**Never `git checkout`** when there is uncommitted work in the same file.

## The rule of the first case of a new form

A copied pattern carries a **precondition that no one wrote down**.

Case (20/08): `Add(nome, def_norm, min_real, max_real)` — the first argument is
NORMALIZED and is in the name. Every param in the project had `min_real = 0`, and with
a zero minimum the normalized and the real coincide: the distinction never needed to
exist. The first param with a **symmetric** range `[-1,+1]` was born at the end
instead of the center, with a dead transducer and no symptom.

> **When it is the first case of a new form** — negative range, different unit,
> inverted sign, new type — **re-read the signature instead of imitating the
> neighbor.** The neighbors were right for a reason that may not hold for the new case.

## The premise rule

If the task begins with a premise about the system's state ("with v0.39.0
flashed..."), **verify the premise before acting on it**. An entire session was once
planned on top of a firmware that was not on the board.

## A float range guard is written by NEGATION

**NaN fails every comparison.** That makes the direct form unsafe:

```cpp
if(r >= 0.f && r < (float)cap_)  { usa(r); }        // NaN PASSA como "fora"
if(!(r >= 0.f && r < (float)cap_)) { r = 0.f; }     // NaN cai na rede
```

On 22/08, `delay.h:117` had `while(r < 0.f) r += cap_;` and hid **two** defects: with
`atraso_` at +Inf the loop never terminated; with NaN it exited immediately and
`(size_t)NaN` — **undefined** behavior — indexed out of the buffer silently. The
second was the worse one and was invisible.

> **Every float range guard that protects an index is written by negation.** Any
> `if(x >= lo && x < hi)` guarding a float-derived access is a candidate for having
> the inverted form.

When auditing, the pattern to look for is: **float → index**. Converting a float to an
integer with an out-of-range value (or NaN/Inf) is UB, not a wrap.

**And read the context before touching.** The second defect only appeared because the
line was read in full before being swapped. A partial fix that looks complete is
worse than none: it closes the symptom that was denouncing the rest.

## The section size proves the attribute was applied

When `__attribute__((section(...)))` **is the mechanism** — code that needs to run
from another memory, data that needs to live in another region — the section size in
the build output is the check, and it is free.

On 22/08 I marked the attribute on two helpers and forgot it on the main function. It
ran from where it should not have and crashed the board: two physical RESETs and a
bootloader reinstall. The number was printed before all of that:

    ITCMRAM:  68 B    <- too little for what I had written
    ITCMRAM: 212 B    <- after the fix

> **A section of the wrong size is an unapplied attribute until proven otherwise.**
> Reading "the section exists" and stopping there is halfway; what matters is "the
> section has the right size".

Same family as the `.map` that caught the `.bss` going to the DTCM in an `APP_TYPE`
swap: **the linker speaks; the cost is reading.**

Write the expectation in the file header, next to the attribute's `#define` — that is
where someone will add the next function and forget.

## A negative sentinel requires explicit init AND explicit test

`-1` as "none" coexists in the same type with `0`, which means "the first". The two
halves fail silently, for opposite reasons:

```cpp
BotaoAlvo t[N] = {};        // zero-init: param = 0, e ZERO E INDICE VALIDO
if(SacrifTravado(i)) ...    // -1 e VERDADEIRO; 0 (indice valido) e falso
```

On 22/08 both appeared on the same day, in the same project. In the first, the
"locked" mark appeared on every healthy effect and vanished at index 0. In the second,
the randomizer button started pointing to `level` and to the freeze LED.

> **`= {}` does not respect the sentinel; `if(x)` does not test it.** An initializer
> in the `struct` itself (`int param = -1;`) and an explicit comparison (`>= 0`).

## GPIO remaps in the source; a peripheral does not

Before proposing "I'll change it in firmware instead of redoing the solder", ask
whether the pin is pure GPIO or is tied to a peripheral.

On 22/08 the freeze LED and switch came swapped and cost two numbers in the source —
GPIO has no pin preference. Minutes later, the MIDI back required a new wire: `D13`/`D14`
are `PB6`/`PB7`, the legs of USART1, and the function is tied by the silicon.

> The difference does not appear in the schematic — it appears in the chip's
> alternate-function table. UART, SAI, ADC and QSPI do not remap freely.

## A restructuring has three fronts; the compiler enforces one

Code, documentation and symbols. The compiler enforces the symbols that **break** —
and stays silent about the ones that survive unused and about the doc that keeps
describing the dead design.

On 22/08 an audit found twelve cases of drift from a single restructuring: ghost
params cited in the doc for months, counters read-without-writer and
written-without-reader, and three orphans that the compiler was already flagging in a
warning without anyone reading it.

> **When finishing a restructuring, sweep the three fronts.** And if the lesson
> repeats, automate it: `make check-docs` in this project checks the doc against the
> manifest on every build. An automated habit beats a remembered habit.

**When writing the checker, the order is non-negotiable: clean first.** Against a
dirty base the first build fails en masse and the temptation becomes lowering the bar.

**And beware the checker that flags the sentence that documents the fix** — matching a
pattern in text catches the scar that narrates the removal. Three rules solved it: a
name that exists in the source is legitimate; a family counts as a name; retired ones
go to a declared list, which becomes useful documentation in itself.

## An edit anchor proves EXISTENCE, not uniqueness

The pattern that looks careful and is not:

```python
assert velho in s          # <-- só prova que existe
s = s.replace(velho, novo, 1)   # <-- e troca o PRIMEIRO
```

Real case (28/08). I anchored the insertion of a subcommand at
`if(strncmp(a, "reset", 5) == 0)`. That text exists **three times** in `main.cpp` — in
the `varre`, `encpino` and `enc` commands. The block ended up at the `vistos` handler,
five hundred lines above the destination.

**And then it compiled, flashed, and gave no error anywhere.** The new command
responded with the generic report, because `enc` did not recognize the subcommand and
fell into the final case. Two flashings with a feature that simply did not exist — and
an accessible `vistos inverte g` that no one would ever type.

The guard costs one line:

```python
assert s.count(velho) == 1, "AMBIGUO: " + tag
```

**And that is not enough.** After inserting, verify **programmatically** where the
block landed, instead of re-reading it and assuming it is fine:

```python
alvo = [i for i, l in enumerate(linhas) if 'strncmp(a, "inverte"' in l][0]
h    = [i for i, l in enumerate(linhas) if 'strncmp(line,' in l and i < alvo][-1]
print("bloco dentro de:", linhas[h].strip())
```

> **Choose anchors with the CONTEXT that makes them unique** — the previous line, the
> handler's name, the comment above. Repeated-structure text (`{`, `return;`, a
> generic `if`) is never an anchor.

The aggravating part of this case: **I had the guard written in another script of the
same session**, with the message `"AMBIGUO"`. I did not apply it here.

> **A safeguard that exists and is not applied by habit is not a safeguard: it is
> luck.** If a guard is worth writing once, it becomes the helper that EVERY edit uses
> — not a best practice remembered when convenient.

## A checker exemption assumes a ROLE, and a refactor changes the role

`check_docs` deliberately cuts out the probe tables (`kLivresPin`, `kEncPin`, `kPino`)
before looking for `seed::Dnn`: a probe points to pins that already have an owner, and
without the exemption every instrumented pin would become a conflict.

On 28/08 I promoted `kEncPin` to **single source** — `EncoderInit` now reads from it in
a loop instead of having the pins written by hand alongside. The table stopped being a
probe and became the declaration.

The checker kept cutting it out, and flagged: *"D0, D1, D2, D3, D4, D9, D17, D18, D19
in PINOS.md and they do not appear in the source"* — **nine pins, all the encoders'**.
True for it, false for the board.

> **When promoting a structure to single source, look for who was ignoring it on
> purpose.** Exemptions are written against a role; when the role changes, the
> exemption starts to lie — and it lies toward the silent side, because a checker that
> ignores flags nothing.

In this case the checker flagged it, and that was luck in the form of the error: it
compared two directions. An exemption that only suppressed the false positive would
have stayed silent forever.

## Version EVERYTHING — every generated artifact carries a stamp

It is not only the source that is versioned (§4). **Every generated artifact that
leaves the repo to be consumed outside** — BOM, export, template, package — carries a
**stamp** at the beginning and in a place valid for its format (`carimbo: <arquivo>
v<N> · <data> · placas: …`; CSV as a `#` comment, Markdown in an HTML comment,
xlsx/xls in a dedicated cell, package in `LEIA-ME.txt`), and the **`export/MANIFEST.md`
is the single source of "the newest"**: before uploading/consuming an artifact, its
stamp is checked against the MANIFEST and, if it does not match, `git pull`. It is
born from a real mistake (2026-09-12): a Mouser cart with an MPN from a **superseded
export** while the repo already had the new one — an artifact that is not versioned
does not let you detect that it has aged. The method's detail lives in the `/aipcb`
skill (`.claude/skills/aipcb/references/versionamento.md`, section "Version EVERYTHING").
