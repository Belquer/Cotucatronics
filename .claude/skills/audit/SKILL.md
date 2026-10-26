---
name: audit
description: Deep consistency audit in Cotucatronics — hunts ghost names, inverted tombstones, false statements, dead numbers, hand tables aging on top of generated ones, dead links, orphan exemptions, objectless checks, sentences broken by a rename, and desynchronized destinations. Use when deleting a name from the whole tree, when removing a feature, when closing a restructuring, and before calling documentation clean.
---

# /audit — audit without breaking anything

**An audit that breaks the build has lost the right to be right.** And one that
"cleans" a name while leaving the sentence around it lying is worse than not
having run: removing the qualifier from a dead statement makes the dead pass for
alive.

This skill is the process. The mechanical part is in `docs/operacao/auditar.sh`
(v1.2.0), which calls `coerencia.py`, `versoes_readme.py` and `vizinhos.py` — run
it first, always.

## THE ORDER IS NON-NEGOTIABLE

It exists because inverting two steps breaks the build for real.

```
1. BASELINE   run every checker BEFORE touching anything. Save the exit code
              AND the counters (ghosts, orphans, retired, ERC…).
              Without a baseline you do not know what is inherited and what is yours.
2. INVENTORY  by `git ls-files`, NEVER by hand-globbing (see below).
3. CLASSIFY   each occurrence, before editing a single one.
4. EDIT       with a unique anchor (skill `edicao-segura`), never a blind replace.
5. GREP       the report comes from the file, not from the intention.
6. RE-VERIFY  and compare against the baseline, number by number.
7. SYNC       the three destinations.
8. PENDING    sweep what the tree declares to be done, and for each one
              ask whether it is still true. A resolved pending item left
              written is a false statement with a date.
```

### Step 2 enumerates by git, never by hand

I swept the source with `*.h *.cpp Makefile` — 33 files — and called the work
done. The tree has **137 tracked files in 14 extensions**. Left out were `.py`
(22), `.command` (8), `.sh` (3), `.lds`, `.ino`, `.html`, `.txt` and a **second
`Makefile`**. The `check_repo.sh` escaped and only showed up in the final sweep, by
accident.

> **Enumerating is git's job; choosing by hand is how you forget a whole file type.**
> `git ls-files -z | xargs -0 grep -lI` — and also `--others --exclude-standard`
> for what is not tracked, plus the vault, which lives outside git.

The `auditar.sh` prints the extensions that exist in the tree alongside the result:
a new extension appears in plain view, instead of staying out in silence.

### Step 2 and step 4 have an ordering dependency

There are files that are a **license** for others: `docs/params-aposentados.txt`
and the `<!-- doc-check: ignore … -->` directives authorize the doc to cite a name
that no longer exists in the source.

> **Deleting the license before the mention breaks `check_docs`.** Mentions first,
> license last, checker to close.

## THE THREE DESTINATIONS

Repo (`docs/`) · vault (`~/Documents/Obsidian Vault/Cotucatronics/`) · skills
(`.claude/skills/*/SKILL.md`). The skill is the one that **loads by itself at the
time of use** — leaving it dirty is leaving dirty the copy that speaks first. And
the frontmatter's `description:` counts as body: it shows up in the lister.

⚠️ The vault has a copy of the **root** `README.md`, not the one in
`docs/operacao/`. Comparing by basename produces a false positive.

## THE CLASSES — and which of them grep catches

The `auditar.sh` covers the first five. The others require reading, and that is why
it **does not pretend** to cover them: a checker that always passes is its own
class 10.

| # | Class | Mechanical? |
|---|---|---|
| 1 | dead link (referenced file that does not exist) | ✅ |
| 2 | orphan exemption (`doc-check: ignore` of a name absent from the body) | ✅ |
| 3 | orphan retired item / **duplicate entry** in the list | ✅ |
| 4 | desynchronized destination (repo ≠ vault ≠ skill) | ✅ |
| 5 | ghost / wrong CC / range that does not match | ✅ `check_docs` |
| 6 | **inverted tombstone** | ❌ reading |
| 7 | **hand table aging on top of a generated one** | ❌ reading |
| 8 | **intra-file contradiction** | ❌ reading |
| 9 | **dead number passing for current** | ❌ reading |
| 10 | **objectless check** | ❌ reading |
| 11 | **defect orphaned from its subject** | ❌ reading |
| 12 | **pending item** (🔴🟡⬜⏳ TODO, "to confirm", "open") | ✅ `coerencia.py` |
| 13 | **sentence broken by the rename** | ✅ `vizinhos.py` |

### 6. Inverted tombstone

A comment that claims that something **still stands** — and it does not.

Real case: `manifest.h` said, of a group of params of a retired effect, *"they
STILL stand in the table on purpose"*. They did not: all the remaining occurrences
were inside comments, and there was not a single `t.Add()` for any of them. I
believed the comment and reported to dB that code had to change. It was false.

> **A comment that claims the state of the code is the least reliable statement in
> the file** — it does not compile, no one tests it, and it survives the change
> that disproves it. Prove by the symbol (`grep` the declaration), never by the
> prose.

### 7. Hand table on top of a generated one

`src/BOARDS.md` had a table titled *"CC map of the **current** instrument"*,
written by hand, that listed three nonexistent params and one removed on 08/28. The
truly generated map is `docs/operacao/CCMAP.md`, produced by `gerar_ccmap.py` from
`manifest.h`, and it carries a *do not edit by hand* warning.

> **Before correcting a table, find out whether it is generated.** Correcting the
> generated one by hand is work the next generation undoes; correcting the hand one
> when a generated one exists is keeping alive the one that lies.

⚠️ **And when pointing the hand one at the source, confirm the source does not
point back.** The `ROTEIRO-KICAD` §5 listed the IDC's 26 vias and came to say «see
the generator» — but the generator's header said «the pinout comes from §5». The
two were left pointing at each other, and neither was the source.

> **A document explains; a generator asserts.** In §5 only what **cannot** be
> deduced from the table was left — why the grounds are where they are, and the 6
> dB measured with the panel board lit. That does not live in the generator and
> would die with the table.

How to find out: `grep -rl '<doc name>' --include='*.py' --include='*.sh'`.

### 8. Intra-file contradiction

The `README.md` announced `flash.sh` entering the bootloader by `dfu go` and, three
hundred lines down, said *"flashing requires the BOOT button"*. Both in the same
file, both affirmative, neither marked as historical.

> Search for the **pair**, not the sentence: a state declared twice is a candidate
> to diverge. Versions, counts, "no known defects", "not yet implemented", CPU
> numbers.

### 9. Dead number passing for current — **the trap of this skill**

The `README` said *"With the external engine's delay loaded: 5.7% average"*. The
real load today is 59.5%. While the qualifier was there, the sentence at least
warned that it was from another configuration.

> **Removing the qualifier from a dead number is falsifying it.** When deleting a
> name from a sentence that carries a measurement: either you update the number
> against the source that measures, or you delete the whole sentence. Never just
> the name.

### 10. Objectless check

The `check_repo.sh` had a whole block checking exports that no longer existed: it
always passed, and was going to keep passing forever.

A subtle form, found in this tree: a `doc-check: ignore route_exciter` exemption
whose comment says *"the mention is the record of the rename"* — and the mention it
protected is no longer in the body. **The exemption became the only record of what
it existed to protect.** Deleting it as "orphan" would delete the last copy. Read
before pruning.

### 11. Defect orphaned from its subject

🔴 *"The serial dies with effect X on"* — the effect went out weeks ago. The defect
is real (starvation of the USB ISR by a long callback), the **repro** is what died
with the subject.

> When retiring a feature, sweep the open defects: the ones that name it either
> close with it, or lose their way to reproduce. Rename by the mechanism.

### How to prune an exemption without losing the record

A `doc-check: ignore` only works if the name appears **between backticks** in the
body — and the scope is the **whole file**, despite `check_docs`'s docstring saying
"on the line or the previous one". Before pruning, three questions:

1. Does the name appear in backticks in this file's body? If so, **it is not an
   orphan**.
2. Does the exemption's comment carry information that exists nowhere else? If so,
   **migrate it to `params-aposentados.txt`** — it is the declared home for a
   retired name — and only then delete the directive.
3. Does another file carry the same record AND use the name? Then this one is a
   leftover.

That is how six orphans became zero without losing a line of history: two migrated,
four were copies of a record that lived in another document.

### 12. The pending pass

`coerencia.py` sweeps the marks the tree uses to say "to be done": the state emojis,
`TODO`/`FIXME`/`XXX`/`HACK`, and the Portuguese phrases ("por confirmar", "por
medir", "em aberto", "ficou por", "não foi validado").

**Two precision traps, both paid for here:**

- `\bTODO\b` with *ignorecase* matches the Portuguese **"todo"**. The first version
  of this pass brought ~200 false positives. TODO/FIXME are **uppercase-only**; only
  the phrases are case-insensitive.
- `pend[êe]ncia` without `\b` matches **de**pendência and **inde**pendência.

> A checker that screams is a checker that gets switched off. Precision before
> coverage: the pass is only worth it if every line it prints deserves reading.

What to do with the result: for each pending item, **is it still true?** The ones
already resolved and left written are class 9 in disguise — a false statement with a
date on top.

### 13. The sentence broken by the rename

I renamed a sentence in `hal_daisy_seed.cpp` with a unique, verified anchor. The
anchor matched once. The `grep` confirmed the new text. The `make` passed. And the
sentence was left like this:

```
    // ... "a serial morre sob callback
    // ligado": quem morria era o LOOP
```

because the **continuation was on the next line**, which no one touched.

> **A unique anchor proves the match was UNIQUE. It does not prove the sentence
> still READS.** They are different guarantees, and only the first is mechanical.
> Compiler, `grep` and `assert` all three stay silent.

`vizinhos.py` reads the diff and flags the neighborhoods at risk. The discriminator
that makes it usable: **there is only a risk if the edit touched the END of the
line.** A rename in the middle leaves the tail equal and the sentence resumes as it
resumed. Without that filter the tool flagged 211 neighborhoods — noise posing as a
finding, which is class 10 applied to itself. With it, 89; and with `--termo=<name>`,
which tightens to the cases where the **removed tail contained the name**, 16.

```bash
python3 docs/operacao/vizinhos.py --termo=<name-going-out>
```

It does not decide — it orders reading. In a name removal, reading those ten or
twenty lines is the price of not leaving a senseless sentence in the middle of the
tree.

### 14. Emitter body copied instead of called

The `gera_soquete.py` had a function `nao_liga(x, y)` that emitted a
`(no_connect ...)` block. And it had, five hundred lines down, **its body again**,
written by hand:

```python
L.append('\t(no_connect\n\t\t(at %s %s)\n\t\t(uuid "%s")\n\t)\n'
         % (g(_x), g(_y), u()))
```

As long as the emitter did not change, the two versions produced identical output
and nothing distinguished them. When the `uuid` came to require a key, **only the
original received it** — the copy was left with the old signature and the generator
blew up. If the change had been silent instead of breaking the signature, the copy
would have kept emitting the old version, with no error at all.

> **It is class 7 in code form.** Two sources of the same behavior, and the one not
> called is the one that ages. The hand table at least looks like a table; a
> duplicated emitter body looks like normal code, and that is why you only find it
> by searching for it.

**How to search** — not by function names, which the copy precisely does not use,
but by the **output**: the whole family of emitters writes strings with a
recognizable shape.

⚠️ **And anchor on the ACCUMULATION, not on the shape.** Searching only the shape
returned eight hits per file, and half were **readers** — `Lb.find('(symbol "%s"')`,
`_confere_lib`, `lib_embutida` — which look for the shape instead of producing it.
That is **class 10** happening inside this pass: noise posing as a finding. What
emits is what does `append`:

```bash
grep -n "L\.append(" gera_*.py | grep -o "L\.append(f\?'\{1,3\}.\{0,26\}" | sort | uniq -c
```

Each shape must appear **once**. Two or more, and only one inside a `def`: the
others are copies. Run over this project's three generators it gives one line per
shape — the only copy that existed was the one that gave rise to this class.

The same search serves any generator, swapping the accumulator: SQL built by hand
next to a builder, HTTP headers, log messages with a fixed format.

⚠️ **And the discriminator that avoids the noise:** duplicating the *call* is normal
and expected — a `nao_liga()` invoked in twenty places is the design working. The
finding is duplicating the **body**. The question is «does this place reconstruct
what a `def` already constructs?», not «does this shape appear many times?».

⚠️ **A finding even when the output is identical today.** The copy is not wrong: it
is **about to age**. Reporting only when it has already diverged is reporting the
fire, not the electrical wiring — and the divergence, when it comes, throws no
error.

## THE TEST THAT READS THE REFUSAL AS APPROVAL

In a single day, four checks of mine gave the verdict **backwards** — and none
because the audited code was wrong:

| the test | why it falsely passed |
|---|---|
| `grep -q "gravado"` | the refusal says **`"Nada gravado"`**, which contains `"gravado"` |
| `case "$out" in *gravado*)` | the same, other syntax |
| `set -- $cfg` in a `for` | **zsh does not split words** without quotes — two values entered as one |
| search for prose in a document | `"preenchimento"` where the file says `"enchimento"` |

> **The failure message almost always contains the success word**, because it is
> written for people to read: *«nada gravado»*, *«no changes committed»*, *«0 tests
> failed»*, *«failed to build»*. A test that matches the **substring** reads the
> refusal as approval.

⚠️ **It is the worst failure mode there is**, and it is worth saying why: the tool
was right, the message was right, and only whoever consulted it got the opposite.
There is nothing broken to find afterwards — only a wrong conclusion already spread
among those who read it.

**What you anchor on, in order of preference:**

1. **the exit code** — and that of the right command: `cmd | tail` returns `tail`'s,
   not `cmd`'s;
2. **the start of the line** (`^ *gravado ·`), not the loose word;
3. **the structure** (does the section exist? does the block exist?), never the
   prose — which changes when someone rewrites a sentence and touches nothing else.

⚠️ **And the same trap from the other side:** searching for prose to prove a lesson
was absorbed gave **two false negatives** in a review of ten, over one word and one
dash. **A documentation checker anchors on titles, not on sentences.**

## A TOOL NOT RUN DOES NOT EXIST

Class 14 was written, reviewed and called good. Then **I ran it**, and it returned
eight hits per file, half of them **readers** of the shape instead of emitters of it
— class 10 happening inside the pass I had just written to catch defects. The
discriminator that makes it usable (anchor on the accumulation, not on the shape)
only showed up on the first real run.

> **A tool fixed and not run is not fixed; a tool new and not run does not exist.** A
> pass yet to debut is not a weak pass — it is a promise, and promises catch no
> defects.

And you run it **over the whole tree**, not over the case that motivated it: it was
by running over all the `gera_*.py` — and not over the file where the defect was
born — that class 14 found the third generator.

## THE WHY RULE — when the name goes and when the record goes

When deleting a name from the whole tree, each occurrence is one of two things:

- **The name is the REASON for a decision that still holds.** The block is 48
  because an FFT module required 256. → **the reason stays, only the name goes.**
- **The record IS about it.** The line in the certification book, the tombstone in
  the retired archive. → **it goes whole, and that is a loss of record.**

> **Declare the losses of the second type, by name, in the report.** The user
> authorized deleting a name; they did not authorize silently deleting the proof that
> a test was run.

The name goes into the commit message — git is where history belongs.

## AN API IDENTIFIER IS NOT RENAMED

A `XPTO_NOTHROW` macro of an SDK does not become `MOTOR_NOTHROW`. A recipe with an
invented macro is not a generic recipe: it is a **false** recipe, and it fails
silently the day someone follows it.

> When the name to delete is a symbol and not prose, the choice is **delete the
> recipe** or **keep the symbol**. Generalizing is not on the table.

Same rule for real paths in git: `git show <hash>:Max/x.maxpat` is a recovery
instruction. If the name has to go, **prove the new path before writing** — and the
number it returns goes into the doc.

## CLOSE

```bash
./docs/operacao/auditar.sh <term>          # or: auditar.command
python3 docs/operacao/vizinhos.py --termo=<term>   # read each one
python3 src/check_docs.py; echo $?          # must match the baseline
git ls-files -z | xargs -0 grep -lI -i '<term>'    # must come back EMPTY
```

The last one is not `grep -rni` over a hand-written list of folders — it is the
whole tree by git. It was the hand list that left `check_repo.sh` out.

> **When the claim is about a SET, the verification counts the set — never the
> sample.** I converted two generators, reported «the two generators», and there was
> a third mining `uuid4` in the file next door: I counted the ones I had looked at,
> not the ones that exist. `grep -c uuid4 gera_*.py` costs a second and depends on no
> memory.
>
> The shape of the trap is always the same — a hand-written list posing as the
> universe. The folder list, the two generators, the files I remember having opened.
> **The universe is asked of `git`, of `glob`, of the generator; not of the memory of
> whoever audits.**

Report: files touched, counters **before → after**, findings that were not fixed
(and why), and the record losses, one by one.

See also: [[edicao-segura]] (anchor and verification),
[[medicao-limpa]] (never conclude without measuring),
[[instrumentacao]] (instrumentation fails like production code).
