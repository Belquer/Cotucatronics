---
name: instrumentacao
description: Rules for writing or changing instrumentation code in Cotucatronics — log, black box, counters, probes, mscope. Use WHENEVER you add or touch anything that measures, records, or reports. Eight cases prove that instrumentation fails like production code.
---

# Instrumentation — the most repeated bug family in the project

**Instrumentation is production code and it fails like production code.**
Eight cases in `docs/battle_scars.md` ch. 2, committed even *after* the family
was named. Part of IFÁ10000.

## The escalation, so you know what's at stake

- Cases 1–7 ruined **numbers** (a lost peak, an overwritten profile).
- Case 8 ruined a **conclusion**, and the conclusion was **published** in
  battle_scars as a confirmed cause of a failure. From there it would come back
  as a premise of the following session.

Broken instrumentation does not stop at the spreadsheet.

## Hard rules

1. **A single-event datum writes once and locks.** It does not update. The grace
   window would zero the meter and erase the debut peak — the very datum it
   existed to reveal.
2. **State beats counting.** A flight token (`in_flight`) is immune to the
   arming problem; paired counters are not — they have already produced a false
   alibi.
3. **The read session writes nothing.** `static` counters zero on boot: if the
   read writes, you measured the read, not the flight.
4. **Every instrumentation write goes through the same gate** (`bb_rec`). A
   marker outside the gate contaminates the crime scene.
5. **Arm by explicit command at the start of the flight**, never in the middle
   of a flow.
6. **The measurement must not alter what it measures.** If it can, prove it did
   not (a lean control flight) before trusting the number.

## The case-9 rule — instrumentation that ACTS

The family's escalation, complete: **numbers → conclusions → actions.**
Cases 1–7 corrupted *measurements*. Case 8 corrupted a *conclusion*, and it was
published. Case 9 **acted**: a watchdog read 4,277,071,629 µs (an overflowed
subtraction), treated it as a deadline overrun, and **shut down a healthy
effect** that was spending 35 µs out of 500.

> **An instrument that only observes errs and someone notices. An instrument
> that acts errs and alters the system before anyone notices.**

**If the mechanism INTERVENES (veto, bypass, mute, automatic reset):**

1. **Plausibility test before the trigger.** A sample outside the physical
   range is **discarded and counted**, never judged.
2. **The discard counter appears in the status.** Otherwise the discard becomes
   another silence, and you trade a visible error for an invisible one.
3. **The action is permanent until explicit rearm.** Undoing itself turns a
   reproducible defect into an intermittent one — and intermittent is the most
   expensive of all.

## The waste-without-symptom rule

**Waste that preserves correctness has no symptom.** Code that does the work
twice produces the **right** output — review doesn't catch it, tests don't catch
it, the ear doesn't catch it. Only measurement catches it, and only if someone
measures.

On 08/18 three effects in a row paid double for the same shape: the API offered
stereo, the data was mono, and passing the same value twice seemed harmless.

**The fix is not discipline** — I failed three times *knowing* the rule. It is
making **the cheap path the default** and the expensive one something you have
to ask for.

> **And the most expensive: a fix without asking "where else does this shape
> appear?" recurs.** Thirty seconds of the question would have caught all three
> at once — and it also explains the four mechanisms that failed silently in
> this project (`log_muted`, `cdc_dead`, `Add()`, `AddMidi()`), fixed one by
> one.

**When fixing anything, before closing:** look for the same shape in the rest of
the code. `grep` by the pattern, not by the symptom.

## The asserting-message rule — cite the other state

Before writing any diagnostic sentence, ask:
***"what other state produces exactly this evidence?"***

If there is one, **the sentence has to cite it**. On 08/18 four of my
instruments treated absence of data as data — "probable DROPOUT" when nobody was
touching anything, "degradation" for what was someone playing, "it didn't come
back" with 21 CCs applied underneath, and "MIDI session absent" with a perfect
link.

**The pattern is not forgetting the rule — it is applying it to the system and
not to oneself.** All four times the ambiguity was known; all four times the
code resolved it by **picking a side**, because picking produces a useful
sentence and admitting does not.

> **The fix is never the threshold. It is to stop asserting.**

An instrument that cannot distinguish two states has to say so **and come with
the test that distinguishes them**:

```
>>> Se voce JA mexeu num controle, a sessao caiu: reconecte.
>>> Se ainda nao mexeu, isto aqui nao diz nada: mexa e olhe de novo.
```

**Corollary:** a new instrument does not resolve the question — it **moves the
frontier** of what is still ambiguous. Know where the new frontier ended up.

## The orphan-safeguard rule — declare the threat, or it becomes folklore

Every protection you add (quarantine, veto, filter, bypass, grace window)
**needs to declare in the code the threat that justifies it**. Not through
documentation: because without it nobody can ask *"does this threat still
exist?"*.

On 08/18 the reconnection quarantine — which had blocked 154 CCs of real dumps —
began discarding the instrument's **legitimate initialization**, because two
correct decisions removed the threat and nobody reviewed the safeguard. Five
messages at boot, zero applied, instrument dead, symptom identical to "MIDI
isn't responding".

**No line was wrong.** The world around it changed.

**When you remove a class of threat, list what existed because of it.** A
safeguard that has lost its enemy rarely disappears on its own — unless it is in
the **scope of the defect** (see the high-pass filter that left together with
the module, ch. 11b of battle_scars). Broad scope = undue survival.

**Demoting is usually better than removing:** the quarantine became a sensor —
it keeps detecting and announcing, it stopped discarding. It preserves the
diagnosis and gives back control.

## The filter rule — "implausible" is not defined by magnitude

If you filter outliers by size, **you discard the catastrophe along with the
noise**: a real 5 ms overrun looks exactly like a clock error. Adjusting the
threshold only moves the blindness — on the other side of the cut there is
always a catastrophe of the same size.

**A reproducible outlier is a findable bug.** On 08/18 the clock outliers were
`System::GetUs()` dividing before the subtraction: the returned value only uses
1/200 of the 32-bit range and wraps around every ~21.5 s, so the `uint32`
subtraction does not correct it. Fix: measure in **raw ticks**, convert
**after** the subtraction — and then the filter stops being necessary.

**Before filtering, ask:** *why does this number exist?* A filter is what you do
when the cause is unknown **and** the cost of investigating is high; in the
other two cases it is a band-aid that hides.

**Corollary for actuators:** an automatic veto demands a **monotonic and
reliable** clock. Without one, case 9 is armed to recur.

## The observer rule — the diagnosis cannot depend on the patient

Serial, MIDI, and black box all pass through the main loop. When it died, **all
three went silent together** and none could report. An hour of hypotheses with
no observation underneath, all wrong.

> **A diagnostic instrument cannot depend on what it diagnoses.**

**When creating any diagnostic channel, declare which context it depends on.** If
they all depend on the same one, you have a single point of blindness — and it
fails exactly when it is most needed. The LED driven by the audio callback
exists for this reason: the observer lives in the context that survives.

**Second-order diagnostic symptom:** when several channels go silent at the same
time, that is not a coincidence — it is the signature **of what they have in
common**.

## The case-8 rule — modes that silence

**Every mode that silences or alters the instrument has to declare itself in
every status command.** `board`, `midi`, and `mscope` print `[LOG MUDO]`.

And the fine point of design:

> **The declaration has to be immune to the mode it declares.**

A banner printed by the normal `Log()` dies silently along with everything else.
That is why `hal::LogAlways()` exists, which crosses the silence, and **only the
banner uses it**. An antidote that catches the same disease is not an antidote.

**A warning that appears once is not a warning; it is luck.** The `bb arm`
warned `"log mudo ate o proximo reset"` — once, at arming. Nobody read it.

Same antidote as scar 7 (the label `"transições: NENHUMA"` printed with every
counter at zero): **the instrument declares its own modes.**

## When creating a new input channel

**An instrument that only says "it didn't arrive" needs a partner that says
"what I sent, arrived".** `mscope` measures what comes in and never distinguishes
"the source didn't send" from "it sent and was lost". The loopback is its
partner: known source, real path, binary answer. **Every input channel is born
with its own.**

## The channel-without-a-meter rule — the question that becomes a hypothesis

**A new signal channel comes in with its own meter.** Not after the first
mystery.

Case (08/20): *"is signal reaching the second piezo?"* The firmware measured
CPU, MIDI, peaks per note, limiter gain, and cost per effect — and did not
measure the **input itself, per channel**. A dead piezo and a quiet piezo sound
identical behind a reverb, so the question only had an answer by hypothesis.

A raw meter (pre-gain, pre-gate) answered before anyone played:

```
IN_L (p1): pico 0.00182 = -54.8 dBFS | 4783 amostras
IN_R (p2): pico 0.00422 = -47.5 dBFS | 9515 amostras   ← reaches the converter
```

A dead pin would give an absolute zero. The question became "does the transducer
deliver?" — which has a different owner.

**Measure RAW.** After any gain, the reading mixes "what the transducer
delivered" with "what the firmware did", and it is the first question that needs
a clean answer.

## The provenance rule — a value off a live board is not a default

I cited `in_gain = 8.0` as "18 dB from the factory" in three answers and logged
it as an open defect. The default is **1.0×**, written on the same line as
`Add()`. The 8.0 was the musician's setting, persisted until the next flash.

> **Before calling a number a "project default", read the default in the
> source.** The board shows what is there, not who put it there.

## A safeguard without a counter is a hypothesis

Every mechanism that **refuses, discards, latches, or overwrites** has to produce
a number that goes up when it acts. Without one, "it worked" and "it never ran"
are indistinguishable — including to whoever wrote it.

Three cases, all on 08/22 and all in the same module:

- **The channel separation of the MIDI mirror** was declared in the header and
  never written in the code: the receiver in omni accepted its own transmission
  channel coming back. There was no "my own messages refused" counter, so the
  absent safeguard looked like a silent safeguard. It only showed up by reading
  the CC history by hand.
- **The sum-ceiling latch** zeroed the `_on` on every block without saying
  anything. The operator's `set` was undone 3000 times per second and the `list`
  showed 0, identical to an effect turned off by hand. It cost a session looking
  for a defect in three intact modules.
- **Silent discard** is the forbidden family: the authorship window discards CCs
  within 250 ms, and discarding without counting would have traded a visible
  loop for an invisible disappearance.

> **If the firmware overwrites, refuses, or undoes an operator's intent, it owes
> that to the operator out loud** — at the moment of the command, not in a
> diagnosis that only someone who already suspects will ask for.

## The counter has to measure what its name says

`eco_proprio` counted messages that **came back**. I read it as "how many went
out" and reported "the board only sent 4 of 8". The two numbers only coincide if
the return path is perfect — which was **exactly the hypothesis under test**.

Measuring the return and calling it the send answers the wrong question with
confidence. With a separate TX counter, the experiment became 6/8 against 8/8 and
isolated the cause in another system.

> **Before reading a counter, say out loud what it increments.** If the sentence
> is not identical to the question you are asking, the number is no good.

## Sizing: state the unit of BOTH numbers

The CC history held 8 messages and I needed to inventory 76 widgets. I increased
it to 96 — *"96 > 76, plenty of room"*. It covered **nine**.

96 was in MESSAGES, 76 in WIDGETS, and sweeping one fader sends dozens of
messages. The inequality was true and meant nothing.

> **Before choosing a size, say out loud the unit of the chosen number AND the
> unit of the requirement.** If they are not the same, the slack is imaginary.

And the structure was wrong along with it: **a history answers "what just
arrived"; an inventory answers "what has ever arrived"**. The second is not
solved by widening the first — however large it gets, it is still a queue that
expels the old. The right fix cost 128 bytes.

## Every instrument has a blind spot — declare yours

The `vistos` inventory only sees a widget that **sends**. A widget that only
receives is invisible to it, however well addressed. Four showed up as "missing"
and one was perfect — dB saw it blinking on the screen while my report said it
did not exist.

> **When you deliver an instrument, say what it CANNOT see.** Without that, its
> silence is read as absence of the phenomenon, and not as absence of reach. See
> also the dead-channel rule.

## A parser that doesn't match must FAIL, never return a default

```python
return float(m.group(1)) if m else 0      # <- o 0 e a mentira
```

On 08/22 this pattern, in one of my bench scripts, turned "my regex didn't
match" into "the peak is 0 µs". Result: I logged **a firmware defect that does
not exist**, prioritized it as step 0 of a plan, and hid the real number — a peak
of 829 µs against a ceiling of 850, 21 µs from dropping an effect.

The meter was perfect the whole time.

> **The dead-channel rule applies to the code I write to measure.** The
> instrument that lies most effectively is the bench one, because nobody reviews
> it: it is "just a script".

In a bench reader: no `else 0`, no `or 0`, no `get(k, 0)`. If the datum didn't
come, say it didn't come — and check the RAW output before trusting an
extractor, especially when the number looks too round.

## A test gain verifies the ACTION and the NARRATION

A safeguard that measures zero in the real regime is undemonstrable without a
test gain (`ctos teto <us>`, `ctos prazo <us>`). But exercising the action is
half of it.

On 08/22, `ctos prazo 300` proved that the criterion dropped the right effects
in the right order — and the report lied twice in the same test: it printed
`>1000 us` while counting against 300, and it labeled as `CAIU pelo teto` drops
that came from the overrun criterion. Both only showed up because someone read
the log instead of checking only the behavior.

> **The report is the only thing that will exist when the event happens for
> real, alone, on a stage.** When exercising a safeguard, read its narration with
> the same suspicion you read the behavior.

Checklist when exercising a safeguard: does the label use the EFFECTIVE value
(not the real one, when there is a test mode)? Does the test mode declare itself?
If there is more than one criterion, does the record say WHICH one acted?

## A CPU drop after protection acts is a consequence, not an improvement

With an artificial deadline the block dropped from 453 to 246 µs — because the
safeguard had turned off five effects. A hasty reader sees "246 µs, great" where
half of the instrument went off the air.

> Twin of the "zero that looks like a datum": **a good number that looks like a
> victory.** Every load reading comes with the portrait of what was on the air.

## Diagnostic code has the same preconditions as production code

It runs on the same hardware, in the same initialization order, with the same
peripherals. *"It's just a log"*, *"it's just a note"*, *"it's just a script"*
are the three sentences that precede the most expensive failures in this project.

On 08/22, three times in the same day the measuring instrument became the
failure:

- a bench parser returned `0` when the regex didn't match, and I logged a
  **nonexistent** firmware defect (12u);
- the report of a safeguard lied about its own criterion, printing ">1000 us"
  while counting against 300 (12x);
- the loop watchdog read the **backup** SRAM before `hal::Init()` — a power
  domain not enabled — and the result was app running, LED blinking, and **USB
  never coming up**. A board alive and mute, recoverable only by BOOT+RST and
  reinstalling the bootloader (12z).

> **Before adding any probe: at what point in initialization does it run, and is
> what it touches already up at that point?**

## An automatic retry needs a ceiling AND a counter

A safeguard that self-heals without a budget becomes the disease. The automatic
rearm of the MIDI listener rearmed on every loop pass while the listener did not
settle: **376,906 rearms in one flight**, ~3.7 s of loop spent — which was the
lockup the watchdog detected.

> Every automatic retry needs a **ceiling per unit of time** and a **counter of
> what the ceiling blocked** — otherwise "it's trying all the time" is
> indistinguishable from "it never needed to try".

**And sweep the siblings:** the unbounded-work rule already existed in this
project, applied to `Log()`, with a long comment explaining why. Two functions
below it in the same file did not follow it. **A lesson applied in one place is
not a lesson applied.**

## Every report line is a verifiable assertion, or it is forbidden

Four times on 08/22 the instrument lied, and all four have the same anatomy: the
report has **two layers** — the datum and the **frame** of the datum — and the
frame was written as decoration while only the datum was treated as code.

    pico 0 µs             <- the regex didn't match, and the code did `else 0`
    ESTOUROS (>1000 us)   <- it was counting against the artificial 300
    "CC 42 no fio"         <- the call only READ; the board was still at 41
    placa viva: sim       <- FIXED STRING, no verification

> `placa viva: sim` can only exist as the result of a ping with a timeout.
> **Without the ping, the line does not exist** — it does not become "probably",
> it does not become optimistic text.

Label, unit, condition, and state assertion are measurement just as the number
is. Status prose lies with more authority, because nobody reviews a sentence in
Portuguese the way they review a number.

**And "it's just a bench script" has a datasheet** — along with "it's just a log"
and "it's just a note", it completes the trio of sentences that precede the most
expensive failures.

## Never ask for human action from inside a timed window

The message "do X now" only appears on the screen **after** the call begins —
and the call IS the window. The operator reads the request when the time has
already passed.

On 08/22 this happened four times: the MIDI-return test, the frequency sweep, the
mirror acceptance, and the counting of button taps. All four times the result was
zero and the instrument looked broken.

The two forms that work:

1. **Announce, end the turn, measure in the next turn.** The operator acts
   whenever they want; the reading does not depend on a clock.
2. **A counter instead of a window.** The instrument accumulates and someone
   reads it when they ask — `N taps since boot`, legible at any moment.

The second is better when the phenomenon repeats: it takes the coordination out
of the whole critical path.

> **Coordination by stopwatch between two agents that do not share a clock is the
> most expensive way to measure anything.**

**And parity does not replace counting:** five and seven taps end in the same
state. The final state was compatible with good debounce AND with bad debounce.

## The rule that closes the family — absence lives outside the domain

**No data value can mean "no data".** Absence is represented **outside the
domain**: a separate flag, an impossible sentinel, or a `(value, validity)` pair.
Never by a value that the domain can produce.

Five cases on 08/22, and it is the same line of reasoning in all five:

| where | legitimate value | read as |
|---|---|---|
| bench parser | peak `0 µs` | "didn't match" → **nonexistent** defect |
| `voltaram 0` | zero real returns | "the board doesn't transmit" |
| `eco_proprio` | return, not send | "sent 4 of 8" |
| `mscope` with no signal | silent channel | "the source didn't send" |
| `RDSR = 0x00` | **idle chip** | "didn't respond" |

The last is the clearest: `if(!id)` treated as absence the **most normal
response a chip can give**. Swapped for printing the raw value, the same hardware
distinguished `00` (someone pulling the line) from `FF` (line floating) — and that
distinction pointed to the next cause. **The diagnosis only existed because the
filter got out of the way.**

Five in one day is **structural form**, not carelessness in series. The
temptation is always the same: the "empty" value is right there, it is
convenient, and `if(!x)` is shorter than carrying the validity along.

**When writing any reader, ask:** *is this sentinel value producible by the real
data?* If it is, it cannot be the sentinel.

**Corollary for reports: print the RAW, label the interpretation next to it.**
Never just the interpretation. On the same line, the reader can disagree with the
second without losing the first.

## A counter that doesn't zero measures HISTORY, not the race

Every retained maximum, accumulated sum, or count cited as the result of **this**
race has to be **zeroed by whoever measures**, at the start. Otherwise the number
is from another race wearing the face of this one's number.

*It cost three measurements of different prescalers all printing an identical
`23288 us` — the peak of the first. The real one was 3,811 µs, six times smaller.
And I repeated the defect an hour later, in a new command that read the same
counter without zeroing it: the family is not cured by having been named in one
place, it is cured in each new reader of the counter.*

## Before publishing two numbers, MULTIPLY THEM

Cost per event × rate **has to give** the fraction. If it doesn't, one of the two
is wrong — and you find out **for free**, without a board and without serial, in
the very sentence you were about to write.

*I delivered `-3%` and `23288 us` on the same line. Twenty frames per second at
23.3 ms is 47% of the loop, not 3%. Both halves of the contradiction were in my
own report.*

Once fixed, the two closed: 20 × 3,811 µs = 4.05%, and the loop measured 4%. **It
is the closure that proves the measurement**, not the plausibility of each number
alone.

## Before asking someone to LOOK, describe what the FAILURE will look like

If the description of the failure and the description of success are the same,
**there is no test** — there is a question that has only one possible answer.

*I asked for "uniform gray = clean, bands = corrupted" for a 1 px checkerboard.
But the previous command left the screen **fully lit**, and a 1 px checkerboard
at working distance also reads as white. "Grounded" and "not grounded" gave the
same visual answer. Three "clean" answers became zero verifications.*

Corollary: **a test has to start from a state that cannot be confused with the
result.** A test that inherits the screen, the buffer, or the counter from
whoever ran before is measuring the predecessor.

This is **15at** turned against its own author: there, the eye was incapable of
seeing the defect; here, **I was the one who designed the verification incapable
of failing**.

## A diagnosis that is only post-mortem is no good for debugging

Every counter that lives only in the black box needs a **twin that can be queried
at any moment**. It is the startup-alarm rule, applied to event counters.

*The cowrie-shell detector wrote to slots 8/9/11 and nowhere else. When the
question "did they fire?" came alive, the answer only existed after a reset — and
I could not separate "didn't fire" from "fired and didn't reach the drawing" from
"drew and wasn't seen". Three counters resolved it; their absence cost a whole
round.*

## Before delivering a test, verify the PRECONDITION — not the feature

"Armed" is an assertion about the **whole chain up to the signal**, not about the
flag at the end of it.

*I turned on `buzios_on`, turned on the panel, read the notches back, and said
"it's ready, play". I never verified that the detector could HEAR: it reads the
sum of the post-gate chains, and flashing zeroes all params. Raw input at −0.6
dBFS, detector envelope at −70 dBFS. I told them to play on a deaf instrument and
called it ready — and dB reported an animation defect that did not exist.*

Pair with the rule above (failure≠success): **a test that cannot fail and a test
that cannot pass are the same defect** — I did not ask what the test was capable
of showing before delivering it.

## A deferred fix becomes the EQUIPMENT of the next emergency

A crooked instrument is a cosmetic defect **while there is no crisis**. In the
crisis it is the only thing between you and the answer — and it becomes a
multiplier.

*Measured on 09/02: the real defect was one line (`GPIO_PULLUP` on an RX pin).
Between "the board hangs" and that line, five hours passed, eight RESET requests,
and five fix flashes. **None of the defects found along the way were caused by the
real defect** — they were nine instruments and tools with old problems, each one
alone "not worth the detour":*

- *the rearm counter had read 10,029 for weeks and nobody asked*
- *the LED confused phase 0 with phase 1, and read two volatiles at different
  instants — it invented a pair the code does not produce*
- *the serial port was chosen by NAME: five commands went to the other board*
- *`bb arm` silenced the log before printing the confirmation — arming and not
  arming had the same face*
- ***the field "did the loop get stuck?" was read and nobody wrote it** — the
  central question of the session had a field and no pen*

None of them grew. They all stayed the same size. What changed was that they
started being used.

**The asymmetry that decides:** an instrument fixed in peace costs minutes, with
the board responding. Fixed inside a prison it costs a flash, a human RESET, and
the doubt of whether the fix touched what you were measuring.

**Deferring is not decided, it happens.** Nobody chooses to leave a counter
lying — it gets left because there is something else to do, and the next session
inherits an instrument with a little less truth.

**Counterweight, or this becomes an excuse for sprawl:** fixing now is not fixing
everything you see. It is fixing **what the task at hand is using**. In a hunt,
that is always the instrument.

## When finishing

Ask: *"if this instrumentation were broken, would the result be distinguishable
from a real result?"* If not, it is not ready yet.
