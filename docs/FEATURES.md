# FEATURES — the single box

<!-- doc-check: ignore in_gain_a (REMOVED on 28/08. The tombstone stays on purpose: it is what stops someone from reintroducing it without reading why it left, and what was lost with it.) -->
<!-- doc-check: ignore in_gain_b (idem) -->

**Living document v0.1.0 · 2026-08-18**

> **Start here.** Every current feature of the instrument is on this page.
> If it isn't here, it either does not exist or was retired — and in that case
> it's in the archive, at the end.

## Why it exists

On 18/08 two tables overflowed on the same day and **silently**: four new
parameters pushed three out (`kMaxParams`), and the 17th entry of the MIDI
map was written past the end of the array (`kMaxMidiMap`). The cowrie shells
**vanished from the instrument** and `shm_shift` arrived with no owner to be
found.

Neither of the two raised an error. The symptom was *a missing feature* and
*a `?` in a debug table*.

**The deep cause isn't the ceiling — it's the absence of a single place where
the feature list lives.** Without that, nobody reviews what already exists
before adding more, and the cost of each addition stays invisible until
something breaks.

> **The rule: before adding, review what's already here.** A retired feature
> goes to the archive; a live feature stays in the box. The box is the start
> of every session.

---

## Current

### Audio chain
| Feature | Params | CC | State |
|---|---|---|---|
| **Source** — piezo/jack crossfade + test tone | `src_mix`, `test_tone_freq`, `test_tone_level` | — | active (`src_mix` out of MIDI for safety) |
| **Per-branch volume, on the physical pots** | `post_level_l`, `post_level_r` | **116**, **117** | ✅ **28/08** · pots on A0 (D15) and A1 (D16), after the limiter · **soft takeover** against the jump, and adoption of the physical position at startup · `knob` measures |
| ~~**Input gain** per chain~~ | ~~`in_gain_a`, `in_gain_b`~~ | ~~21~~ | 🗑️ **REMOVED 28/08 — IF IT DOESN'T SOUND, IT DOESN'T STAY.** The `Governor` is an AGC: 18 dB put here were 18 dB it took back downstream, and the output didn't change. The only audible effect was the step. The balance between piezos was already handled by `piezo_mix`. **Lost:** the matching to transducers of different sensitivity — the governor only REDUCES; if it comes up in replication, it is fixed in the carrier board's divider |
| **Mix between the CHAINS** (shell) | `piezo_mix` | **58** | ⏳ acceptance pending · **moved position in the pivot**: it was balance at the input between raw signals, now it mixes the two already-processed chains. Same law: center = both full |
| **Test tone: destination** | `test_tone_out` | — | ✅ 20/08 · 0=A · 1=B · **2=BOTH (default)** · injected **after the gate**, otherwise the gate cuts the tone itself and you measure the gate |
| **Input histogram** per channel | — | — | ✅ 20/08 · `hist` / `hist reset` · 6 dB buckets by the float exponent, raw **and** post-`in_gain`, clip with latch, and the dB still to remove for the attenuator · +27 µs |
| **CTOS sum ceiling** | order in `Manifest::sacrif[]` | warning on **CC 119** | ✅ **20/08, three tests passed** · sibling of the per-effect watchdog: catches N healthy effects that together don't fit · priority declared cold, not LIFO · `ctos teto <us>` simulates · +14 µs |
| **The CTOS latch speaks** | `set` warns · `list` marks · `board` shouts | ✅ **22/08, v0.111.0** · the drop is latched until `ctos rearm`, and before that it was invisible: the `set` was undone every block and `list` showed 0, the same as an effect turned off by hand · cost a session hunting for a defect in intact drive/phaser/shimmer (bts 12k) |
| **B3 = CTOS rearm** | physical button on **D25** (pull-up, active low) | ✅ **22/08, v0.111.0** · dB plays without a computer · **gives back the enabled effects**, unlike `ctos rearm` over serial, which only releases the latch · `b3` over serial pulses the button while the panel doesn't exist (bts 12m) |
| **LED signals the latch via cadence** | beat 4× faster | ✅ **22/08, v0.111.0** · the LED remains the proof of a "live loop"; the alarm goes into the cadence, not the state · without it B3 would be useless — nobody presses a rearm without knowing they need to (bts 12m) |
| **Drive: the knee is relative to level** | `in_gain` before `drv_drive` | ✅ **22/08, measured** · a normalized shaper is clean gain below `x ≈ 1/d` · with `in_gain` at 1.0 the p2 (−19,5 dBFS) stays at ~1% THD even at the peak: inaudible by level, not by defect · fixed at `in_gain` 2,2×/6,0× (bts 12n) |
| **MIDI mirror — the board talks back** | channel 2, `eco_ult[cc]` | ✅ **22/08, v0.114.x** · sweeps the 76 params and transmits what changed · at boot it pushes the whole state (the board speaks first) · fixes the Loopy screen lying after a flash, a CTOS drop, or a change over serial |
| **Authorship window, 250 ms per CC** | `quiet_until[128]` | ✅ **22/08, v0.115.0** · the Loopy widget is a toggle WITH STATE: out of phase it returns the opposite value and undoes the autonomous change · message matching doesn't work; the criterion is "the subject I just spoke about" · measured: CC65 went from 6/8 to **8/8**, CC68 stayed 8/8 (bts 12p) |
| **`learn` mode** | ~~`learn [stop\|next\|<cc>]`~~ | ⬛ **REMOVED 22/08, v0.121.0** · five versions and it never closed the addressing · it required the board deaf, which blinded the `vistos` that would prove the result — two incompatible regimes, not two methods · preserved in commit `514e09c` (bts 12t) |
| **`drv_sujeira` (CC 36)** | tanh morph → wavefolder | ✅ **22/08, v0.4.0** · THD 17,9% → 58,9% at the peak, 8× more energy above the 7th harmonic · hard clip and asymmetry rejected (< 1% difference) · fixed 2 kHz low-pass before the fold: without it aliasing hit −36 dB at 3 kHz · +9 µs, zero at 0 |
| **`vistos` — addressing inventory** | one counter per CC, 128 bytes | ✅ **22/08, v0.120.0** · answers "what has ever arrived", which a message history doesn't answer · closed **65 of 65** in two sweeps after `learn` mode failed across five versions · caught 4 widgets with another parameter's number · declared blind spot: only sees a widget that SENDS (bts 12r, 12s) |
| **Deadline-overflow counter** | per regime, with stamp and snapshot | ✅ **22/08, v0.122.0** · the CTOS kept only the MAXIMUM, and a maximum without a denominator doesn't decide policy · measures **zero** in designed use, at the extreme with everything on, and under provocation · each overflow stamps which effects were in the air |
| **Overflow criterion (aims at the DEADLINE)** | 3 overflows in 2 s drop an effect | ✅ **22/08, v0.123.2** · the ceiling (850) is a budget, the deadline (1000) is a wall — a criterion aiming at the ceiling would drop an effect in a config that plays clean · it is born quiet, which is right · `ctos prazo <us>` demonstrates (bts 12w) |
<!-- doc-check: ignore route_exciter (name RETIRED on 28/08, now `out_l`. The mention is deliberate: it is the record of the rename, and deleting the old name would delete exactly what explains why it was wrong.) -->
<!-- doc-check: ignore route_jack (idem, now `out_r`) -->
| **Audio does not pass through a mechanical contact** | switch → GPIO → firmware cuts | ⬜ **HARDWARE RULE, 22/08** · a power switch does not conduct line level (dry circuit): it closes mechanically and stays open to the signal · cost an afternoon · `out_l`/`out_r` cut the branch (names fixed on 28/08: they were `route_exciter`/`route_jack`, and the 3-position DPDT — jack \| both \| amp — showed that the firmware names the OUTPUT, not the destination) · pole 2 of the DPDT on D29/D30 follows the same design (bts 13b) |
| **Physical freeze with LED** | switch **D24**, LED **D27** | ✅ **22/08, approved by ear** · 30mm arcade, NO microswitch · **toggle** (hit it to freeze, hit it to release), not hold · debounce **30 ms MEASURED**: at 10 ms five hits became six · the LED follows the **param**, never the gesture — if freeze changes by CC, preset, or a CTOS drop, it tells the truth · 5 V LED on 3,3 V: brightness approved, transistor plan B shelved |
| **Buttons with a target, generalized** | table in the manifest | ✅ **22/08, v0.127.1** · each button declares param + LED index; the manifest doesn't know the pin · a single `AlternaBotao()` serves them all, for the same reason the CTOS sacrifice became a single function · `-1` sentinel with explicit init (zero-init pointed the randomizer at `pre_level_ab`, bts 13f) |
| **Freeze B — wiring pending** | switch **D22**, LED **D23** | ⬜ firmware ready and declared (`B4`), still to be soldered |
| **`frz_size` — dead travel** | range 20–165 vs clamp `NucleusLen()/3` ≈ 166,7 | ✅ **fixed in `d80e45d`** · it was 20–165 against the same clamp: 61% of the fader did nothing (bts 12i) · the `165` got a comment tying it to `kNucleusMs/3` so it doesn't desync silently · **the item sat in the QUEUE for months describing the pre-fix state** — drift in a place `check-docs` doesn't look (bts 13i) |
| **Preset in flash** | save and reload at boot | ⬜ **opened 22/08** · every flash resets every param and `pre_level_ab` is born at 0 · silent loss on both sides, and the symptom mimics an effect defect (bts 12l) |
| **Gate** per chain | `gate_thresh_{a,b}`, `gate_release_{a,b}` | 30, **51** | active · duplicated by **material**, not by imbalance: the drum piezo reads a short transient, the spring one reads continuous scraping |
| **Delay** (shell, C++) — send per chain | `dly_on_{a,b}`, `dly_mix_{a,b}`, `dly_time` | 60–64 | ⏳ **re-acceptance pending** · `delay.h` v0.1.0 · +11 µs (was 45, **always**) · floor 50 ms · fixed feedback **0,4**, read from the patch |

> ### 🔀 PARALLEL TAIL and TWO SPACES (20/08)
>
> The tail stopped being a queue and became **sibling destinations**, each
> with a per-chain send. And the **space doubled**: each chain has its own
> room and shimmer, because shimmer isn't an effect alongside the reverb — it
> *is* the reverb fed back with pitch, and two wettings require two rooms.
>
> Delay and freeze remain **a single one**, with a per-chain send: the gesture
> asked for was about space, and duplicating them would cost CPU without
> delivering a difference.
>
> **Measured:** A with delay+shimmer and B with a clean room = **490 µs / 530
> peak (53%)**. Worst case, both with everything = **598 / 639 (64%)** — 209 µs
> below the prediction, because a cost measured in one architecture doesn't
> carry over to another (scar 12g).
>
> **Lost:** the delay is no longer reverberated. The echoes come out dry and
> the room works on the source.
>
> **Simplified next:** `esp_on/esp_mix` and the old `rev_mix` were **the same
> control twice** — gain at the input and gain at the output of the same linear
> room. What remained is a single send per destination: `rev_on`/`rev_mix` say
> how much of the chain enters the space, the return comes back whole, and
> `shm_mix` went back to a single job (just the direct layer). **75 params, 75
> CCs.**
| **Granular freeze** (shell) — one per chain | `frz_on_{a,b}` (**the gesture**), `frz_vol_{a,b}`, `frz_size_{a,b}` | 65–70 | ✅ certified 18/08 · ✅ **physical button approved by ear 22/08** · **TWO independent instances**, each with its own buffer · the `_on` **is** the freeze gesture, not a bypass switch — there is no `frz_hold` (bts 13h) |
| **Drive** — switch and dry/wet per channel | `drv_on_{a,b}`, `drv_mix_{a,b}` · `drv_drive`, `drv_tone` shared | 38,39 / 44,45 · primary **43** | ⏳ **re-acceptance pending in the new position** (IFÁ10000 is not inherited) · +21 µs per channel |
| **Phaser** — switch and dry/wet per channel | `phs_on_{a,b}`, `phs_mix_{a,b}` · `phs_rate`, `phs_depth`, `phs_fb`, `phs_stages` shared | 40,41 / 46,47 · primary **49** | ⏳ **re-acceptance pending** · +26 µs per channel · ⚠️ `phs_mix` is a MOUNTAIN: peak at ~50% |
| **Warble** — switch per channel, shared params | `wrb_on_{a,b}` · `wrb_rate`, `wrb_depth`, `wrb_irreg` shared | 42 / 48 · primary **52** | ⏳ **re-acceptance pending** · +14 µs per channel · 16 cents measured |

> ### 📖 `wrb_mix` is a NAME, not a param — display rename (20/08)
>
> **There is no `wrb_mix` param, and it's not an oversight.** The warble's
> dry/wet is the **crossfade of the `EffectSlot` itself** (`gs`/`gm` in the
> callback), and that's how it passed IFÁ10000. It doesn't appear in `list`,
> has no CC, isn't adjustable.
>
> The name exists only to align the vocabulary with its neighbors — drive and
> phaser **have** an adjustable `mix` — and so that nobody looks for a stage
> that never existed. **If it ever becomes a real param, that's a change to the
> approved effect** and goes through dB's decision, not through table
> consistency.
>
> The `EffectSlot` was not touched: it's a shared resource, and renaming inside
> it would brush against the comments explaining the equal-power curve and the
> ramp's internal names.
| **Shimmer** (shell) | `shm_on`, `shm_mix`, `shm_shift`, `shm_feedback`, `shm_tone` | 33, 37, 36, 35, **57** | ✅ **approved** 20/08 · 2 s tail · `shm_tone` **rises to brighten**: 0 = bell −6 dB at 3 kHz + cut at 6 kHz, 1 = open up to 18 kHz |
| **Reverb** Freeverb (shell, frame) | `rev_on`, `rev_mix`, `rev_size`, `rev_damp` | **61**, 24, 25, 26 | ✅ certified T14a · ⚠️ `rev_on` at 0 **mutes the shimmer along with it** (it feeds back into this reverb) — the `board` declares it |

> ### 🔀 ARCHITECTURE PIVOT underway (20/08) — two character chains
>
> Hybrid instruments (drum + string + springs in the same body) call for
> different treatment by material. dB's decision:
>
> ```
> p1 → [DRIVE · PHASER · WARBLE] ┐
>                                 ├→ MIX → [DELAY · FREEZE · SHIMMER↺REVERB]
> p2 → [DRIVE · PHASER · WARBLE] ┘        → OUT_WIDTH → split
>      character: DUPLICATED               space: SHARED
> ```
>
> The tail stays single on purpose: it's what makes the drum and the string
> sound like **the same object**. Duplicating shimmer and reverb would separate
> what should musically unite — and they're precisely the most expensive.
>
> **`piezo_mix` changes position:** it leaves the input and becomes the balance
> between the two **already-processed** chains.
>
> Each duplicated effect goes through the **IFÁ10000 in the new position** — it
> doesn't inherit the old chain's acceptance (an inherited rule has a
> precondition, scar 11z).
>
> Order: (a) detune out ✅ · (b) infrastructure of the two chains + mix ✅
> (620 µs / 66%) · (c) ceilings resolved in the design ✅ (96 params / 48 CCs) ·
> (d) character collapsed to shared params ✅ · (e) `out_width` retired ✅ ·
> (f) **anti-feedback protection for branch B** ← next, and blocking for the
> hardware · (g) second space — **delivered another way**: instead of a
> `rev_type` choosing the character, out came **two independent spaces** with
> per-chain params (`rev_*_{a,b}`, `shm_*_{a,b}`), which is the design that
> "string with shimmer, leather with clean reverb" called for · (h) send
> per chain · (i) DPDT GPIOs · (j) IFÁ10000 and approval by ear.
>
> ### 🔴 Branch B has no anti-feedback protection
>
> Governor and SSB shifter exist **only on `OUT_L`** — correct while `OUT_R`
> was a clean line jack *by design*. With **two exciters**, `OUT_R` now drives
> a transducer on the body of the sculpture with neither of the two, closing an
> unprotected acoustic loop. **It's new work, not configuration**, and it must
> exist before exciter 2 is soldered.
>
> ### Output fork is a GLOBAL mode (decided 20/08)
>
> Amps and jacks share `OUT_L`/`OUT_R`, so there's no "exciters in stems and
> jacks in mix". **stems:** `OUT_L` = chain A, `OUT_R` = chain B — each exciter
> gets its own material, the jacks record separately. **mix:** `OUT_L` =
> `OUT_R` = the mixture, both exciters drive the whole sound.
| **Fx2 high-pass** | `fx2_hp` | — | ⏸ inactive while the Fx2 slot is empty |
| **Governor / limiter / output gate** | `gov_ceiling`, `gov_release` | — | active (out of MIDI for safety) |
| **SSB shifter** | `shift_hz` | — | ✅ certified T8, off (waiting for the exciter) |
| **Level** master | `pre_level_ab` | 20 | active |

### Gesture and automation
| Feature | Params | CC | State |
|---|---|---|---|
| **Cowrie shells** — random draw by transient | `buzios_on`, `buzios_thresh`, `buzios_cooldown` | 31, 32 | active · whitelist: `dly_time`, `dly_mix` |
| **B2 hold** — freezes while held | `button2_target` = `frz_on` | — | waiting for panel v2 |

### Infrastructure (CTOS)
| Feature | Command | State |
|---|---|---|
| **Shared pitch engine** | `pitch <ratio>` | ✅ validated — 1 buffer, 4 voices, per tap point |
| **EffectSlot** — fade, zero-cost, timing | — | ✅ validated |
| **Watchdog with veto** — bypass until `ctos rearm` | `ctos` | active |
| **Life pulse on the LED** per phase | — | active |
| **Reconnection quarantine** | `ble` | ⚠️ downgraded to a **sensor** — it detects, doesn't discard |
| **Black box** | `bb` | active · ⚠️ `bb arm` silences the log until `log on` |
| **MIDI scope** | `mscope` | active |
| **BLE diagnostics** — WIDI LEDs on D28/29/30 | `ble raw` | ✅ legend measured |
| **Sync** — pushes state to the controller | `sync` / CC 34 | ⏸ ready, useless until Loopy does MIDI learn |
| **Capacity reporting** of the tables | boot | ✅ new, 18/08 |
| **`dfu go`** — restarts into the bootloader by software | `dfu go` | ✅ new — swapped 119 attempts for 1 |
| **Missing MIDI session warning** | `ble` | ✅ new — steady LB + zero bytes ⇒ "reconnect, don't forget" |

---

## Archive — retired

| Feature | When | Why | How to revive |
|---|---|---|---|
| **External-engine freeze** | — | rejected at step 5 of IFÁ10000: **7 349 µs** against a 2 667 budget, and 89% of the SRAM pool | only if the patch slims down. Replaced by `freeze.h`, which does the same gesture in **98 µs** |
| **`fx2_gain`, `fx2_on` in the MIDI map** | 18/08 | they pointed at the empty Fx2 slot — touching them did nothing, silently | they come back with the Fx2 module, on the reserved CC 33 |
| **`src_mix`, `gov_ceiling` in the MIDI map** | 18/08 | routing and safety; iOS dumps put them at the extreme and muted the instrument **3× in one session** | `gov_ceiling` should not come back. `src_mix` was retired entirely (below) |
| **`src_mix`** (the whole param) | 20/08 | it was a **crossfade** of the two inputs: at center it dropped both by −6 dB, which is why it didn't solve "one masks the other". Replaced by `piezo_mix`, which is a **balance**. Keeping both would be two controls fighting over the same gains — catalog family C | `t.Add("src_mix", 0.f, 0.f, 1.f)` and the linear crossfade in step 1 of `AudioCb`. Knob **k2** went along to `piezo_mix` |
| **Rule "re-pair after every RESET"** | 18/08 | **revoked by measurement** — the pairing survives the reset. It was born from a log I myself silenced | — |
| **Quarantine that DISCARDS** | 18/08 | the two decisions above killed the threat; it started discarding the legitimate initialization | downgraded to a sensor, not removed |
| **Magnitude-based plausibility filter** in the watchdog | 18/08 | it discarded the catastrophe along with the noise | removed; the cause (clock) was fixed |
| **THE ENTIRE EXTERNAL DSP ENGINE** — runtime, slot system, modules, and exports | 20/08 | the delay was the **last consumer**, and it was rewritten in C++ (`delay.h`). What went with it: `MODULE_SRC`, the export directory, `module.h`, `mod_slot[]`, `ModBind`/`bind[]`/push-by-name, six `module*.h/.cpp` files, four export directories (**8 MB**), the module's six orphan params, the fx2 watchdog that judged by global CPU, the `probe` command, and the note-by-note histogram that measured a module outside the build. **It yielded −133 µs average, −67 µs on the base alone** (a `Module::Process` runs whether on or off — zero-cost-when-off was structurally impossible in the slot system), −36 KB of binary, and the 49 KB SRAM pool | all in git. Reviving requires restoring the export directory, `MODULE_SRC`, `module.h`, and the binds — and accepting the fixed cost back. **The `Max/` folder was removed from the tree on 04/09** (deep clean): it was the four `.maxpat` files that generated those exports — freeze (1,0 MB), delay (102 KB), shimmer (368 bytes, an empty sketch), and the Fx2 slot one (284 KB), 1,8 MB in all, untouched since 15–23/08. **They remain in git:** `git show 8271e97:Max/` lists the four, and `git show 8271e97:Max/<nome> > /tmp/x.maxpat` brings any of them back |
| **`out_width`** — mid/side width | 20/08 | **the two-exciter architecture drained the control of meaning.** M/S presupposes that L and R are a **stereo pair**. Here they aren't: in `stems` the two channels are **independent sources** (applying M/S would mix piezo 1 into piezo 2 — silent contamination), and in `mix` L = R, so the width is zero by construction. **No mode was left in which it meant anything.** Decisive: with two exciters on the **same body** there's no stereo image — the sum is **acoustic, in the object, before the air**, and the M/S mental model doesn't apply | revive only if a mode exists in which the output is a real stereo pair. The block sat between the LEVEL and the split (position marked as vacant in the source) and cost +4 µs, zero at neutral. It freed 1 param and **CC 59** |
| **Per-channel character params** (14 × 2) | 20/08 | **dB's usage decision**: six effects with independent parameters got confusing to operate. There's once again **one** drive, **one** phaser, **one** warble, with shared params; only `_on` and `_mix` are duplicated. **There was no CPU saving and that was expected** — each effect still processes both signals with separate internal states (two sets of allpass, two warble buffers), otherwise the channels contaminate each other. What shrank was the control surface and the table (71 → 62 params) | re-instantiate per chain: **the structure supports it** — the `struct Cadeia` already has its own object per channel, and the manifest goes back to declaring the params as `[kCadeias]` arrays |
| **Isolated test route for the pitch engine** | 20/08 | it cost an **entire `PitchEngine` instance — 153,6 KB of SDRAM** — just for the bench. With detune out of the build, the only real consumer became the shimmer, which lives in the tail and has its own instance. **Instrumentation is production code and pays production cost:** a probe that costs an instance has to justify itself by current use, not by historical convenience | re-instantiate `PitchEngine` + `EffectSlot` and block 1.6 of `AudioCb` (marked as vacant in the source) if/when a second pitch consumer enters a chain. The `pitch grain` and `pitch jitter` tunables did **not** go along: they keep acting on the tail instance |
| **Detune** (shell) — 2 micro-detuned voices + width | 20/08 | **dB's decision by ear: _"the warble alone is already good"_.** The warble covers the same territory — pitch modulation — and detune cost **+125 µs, five times the phaser**, the most expensive effect in the suite. A cost disproportionate to what it added over something already there. It was **approved by ear on 20/08 and retired the same day**, and that's legitimate: approval answers *"does this work?"*, architecture answers *"is this worth the place it takes?"* | `detune.h` v0.1.0 remains in the repo, **intact**. To revive: (1) `#include "detune.h"` in `main.cpp` — the comment left in place of the include explains it; (2) declare `Detune detune`, `EffectSlot detune_slot`, and a **third** `PitchEngine` instance, 153,6 KB of SDRAM; (3) the processing block went in **4.3, post-module** — the position is marked as vacant in the source and is mandatory: before it the chain is mono, and without stereo there's no `spread`; (4) manifest: 5 params + `AddMidi` at **52–56, which were freed**; (5) put it back at index 0 of the sacrifice order |

---

## Checklist before adding a new feature

- [ ] **Have I read this whole page?** Does some current feature already cover this?
- [ ] How many **params** and **CCs** does it add? Do they fit within the ceilings? (the boot says so, and `board` repeats it on demand — an alarm only in the banner if it overflows)
- [ ] Does it **retire** anything? If so, the retired one goes to the archive **in the
      same commit**
- [ ] Is it **CTOS** or an **Effect**? (map in `docs/CTOS.md` §5)
- [ ] If it's an Effect: it goes through the full **IFÁ10000**, step 0 included
- [ ] It gets its **own on/off**, separate from the mix — dB's decision, 18/08
- [ ] **It is born with the MONO path as default.** Before the reverb the chain is a
      single channel; `Process(l, r)` on a mono source pays double **with no symptom
      at all** — the output comes out right and only the cost reveals it. Three
      effects in a row fell into this

> **The question this document exists to force:** *what's already here that I
> should review before adding more?*
