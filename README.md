# Cotucatronics OS
<!-- doc-check: ignore no_connect (KiCad ERC violation code) -->
<!-- doc-check: ignore dentro_do_contorno (tool lives in kicad/, out of check_docs' reach) -->
<!-- doc-check: ignore para_fabrica (tool lives in kicad/, out of check_docs' reach) -->
<!-- doc-check: ignore pcb_contorno (tool lives in kicad/, out of check_docs' reach) -->

<!-- doc-check: ignore hook_stop (script in docs/operacao/, not a param) -->
<!-- doc-check: ignore svg_colisoes (tool outside src/. check_docs sweeps `src/`; these live in `tools/`, `docs/desenhos/caixa/` or `kicad/cotuca_cerebro/`, which it does not read. Not a phantom: it is the checker's reach.) -->

Modular firmware for **sound-sculpture instruments**. The core idea is that the
same brain runs any sculpture: the instrument code is board-agnostic, and
changing hardware means writing one new file — not rewriting the system.

A collaborative project.

> ### Status, 2026-09-04 — **v0.301.0**
>
> **480 MHz · 48 kHz · block 48** (1,000 µs deadline) · app in QSPI
>
> **Two character chains** (drive, phaser, warble, tarolator per piezo) over a
> **shared tail** (delay, freeze, shimmer↺reverb), with a **2×2 routing
> matrix** — which chain feeds which output.
>
> **Four-layer anti-feedback stack**, all in shell C++: gate, Governor, limiter
> and SSB shifter — plus **SuMiA™**, the automated microphonic-feedback
> suppressor, four notches per branch, which hunts and fills on its own.
>
> **MIDI across six channels** over a BLE↔UART bridge on an ESP32-C3 (`ch1`
> shared · `ch2` chain A · `ch3` chain B · listen = send + 3), **81 CCs**,
> presets in RAM, **black box** in BACKUP_SRAM.
>
> **Panel soldered and alive:** SSD1309 OLED over SPI with an animated splash,
> **three encoders**, **two pots** (per-branch volume) and **four buttons** with
> two LEDs. The map lives in
> [`docs/operacao/PINOS.md`](docs/operacao/PINOS.md), which is the source — and
> the code is what rules over it.

> ### What has NOT yet passed the ear or the multimeter
>
> - **The compressor has never been heard.** It has existed since 02/09
>   (`dynamics.h`) and has never been switched on with sound.
> - **Four items from the [listening agenda](docs/PAUTA-ESCUTA.md)** — the top
>   of `drv_sujeira` (only ever approved at 0), shimmer with real transposition,
>   the rewritten delay, and ~20 versions with no verdict from the ear.
> - **The broadband hiss** is diagnosed down to a named rule — `AGND`/`DGND`
>   joined off-board, P3 of the ground sweep — and the grounding was redone on
>   03/09 with cleaner sound, **but the multimeter measurement was never made**.
> - **The DPDT commons** are waiting on two continuity checks: is the amp BTL?
>   and is the terminal going to the island a power output or just the TRS
>   sleeve?

---

## Architecture

Five rules that are not negotiable:

1. **HAL boundary.** No vendor `#include` outside `hal_<board>.cpp`.
   `main.cpp`, params, macros and manifests do not know which board they are
   running on. The board is chosen with `make BOARD=daisy_seed`.
2. **Normalized parameters.** Everything is `0.0–1.0` in one central table; the
   manifest is what translates to Hz, seconds, dB.
3. **Macros.** One physical knob can move several parameters, with weight and
   inversion.
4. **Manifest per instrument.** It is the only file that changes from sculpture
   to sculpture. It declares `Requirements` — big RAM, audio in, number of knobs
   — checked at boot: an inadequate board gives a clear error instead of a
   mysterious bug on stage.
5. **`AllocBig()`.** Abstracts the 64 MB of SDRAM. Bump allocator, boot only,
   never in the audio callback. A board without big RAM returns `nullptr` — the
   caller **must** check.

## File map

| File | Version | What it is |
|---|---|---|
| [src/hal.h](src/hal.h) | **v0.45.0** | The contract. The vendor-independence boundary |
| [src/hal_daisy_seed.cpp](src/hal_daisy_seed.cpp) | **v0.86.0** | The only file allowed to include `daisy_seed.h`. **4 buttons** (D26, D24, D25 CTOS rearm, D22 freeze B) and **2 panel LEDs** (D27, D23). 30 ms debounce, measured. **SSD1309 OLED alive** (SPI1: D5 RST · D6 DC · D7 CS · D8 CLK · D10 DIN) with a wire probe and bridge hunter. **Internal PULL-UP on the MIDI RX (PB7)** — libDaisy starts the UART with `GPIO_NOPULL`, and the pin floating with the bridge unplugged put the USART into a framing-error storm and killed the loop in 4 s. **USART1 at priority 1**, below its own DMA stream, or the asynchronous abort never completes. **`DmaListen` rearm with a ceiling and backoff** — without it, it ran at the loop's pace and consumed it (10,029 rearms with a healthy bridge). **Whole framebuffer via `OledDesenha`** (`memcpy`, not 8,192 `DrawPixel` calls), and the SPI prescaler is born at **PS_8** — 3,811 µs per frame, verified on the glass with a 1 px pattern. PS_64 cost 23,401 µs. **ONE encoder** since 05/09 (there were three; dB closed the interface down to one, and that dissolved the `D17` conflict for free — it was both the G encoder's push *and* the A2 of the rail divider) — and `kEncPin` is the SINGLE source: `Init` reads from it and the names come out of `EncoderPinNome`. **`VarreTudo`** reads all 31 pins (kills OLED and MIDI until a reboot) and **`Reiniciar`** undoes it. `PollMidi`/`PollSerial` drain from a **snapshot** of the pointer — chasing the live pointer locked the board up on 28/08 |
| [src/reverb.h](src/reverb.h) | **v0.2.0** | Freeverb + high-pass. Pure C++, buffers come from outside |
| [src/freeze.h](src/freeze.h) | **v0.3.1** | Granular freeze: 500 ms core, 2 randomly placed grains, Hann. Pure C++ |
| [src/pitch.h](src/pitch.h) | **v0.5.0** | Shared pitch engine: 1 buffer, up to 4 voices. **Automatic anti-aliasing on write** (6th-order Butterworth, zero cost if no voice shifts up). Pure C++ |
| [src/shimmer.h](src/shimmer.h) | **v0.3.0** | Shimmer: reverb + pitch in a loop. Feeds back into the existing Freeverb. **Tone = cut + presence bell at 3 kHz** |
| [src/drive.h](src/drive.h) | **v0.4.1** | Drive: normalized tanh, **1.5–48×** (the knee has to reach −30 dBFS), plus `drv_sujeira`: a morph into a wavefolder with its own anti-aliasing |
| [src/phaser.h](src/phaser.h) | **v0.3.0** | Phaser: N allpasses swept by a triangular LFO. Zero buffer |
| [src/warble.h](src/warble.h) | **v0.1.1** | Warble: tape drift via a short modulated delay. Its dry/wet is called `wrb_mix` and **that is a name, not a param** |
| [src/detune.h](src/detune.h) | **v0.1.0** | Detune. **OUT of the build since 20/08** — revivable, see `docs/FEATURES.md` |
| [src/delay.h](src/delay.h) | **v0.2.0** | Delay in C++: 1 s line, fixed 0.4 feedback, 50 ms floor. **Read clamped against NaN/Inf, with a `clamps_` counter** |
| [src/effect_slot.h](src/effect_slot.h) | **v0.3.0** | The frame: asymmetric fade, zero cost when off, timing |
| [src/pitch_spectrum.py](src/pitch_spectrum.py) | **v0.2.0** | The N largest spectral peaks — the dominant peak lies when the sidebands are strong |
| [src/dynamics.h](src/dynamics.h) | **v0.6.0** | Gate, governor, limiter and **SSB shifter**. Pure C++ |
| [src/midi.h](src/midi.h) | **v0.9.0** | MIDI state machine + **the six channels** and the `listen = send + 3` invariant, locked down with a `static_assert`. Pure C++, board-agnostic |
| [src/test_dynamics.cpp](src/test_dynamics.cpp) | **v0.1.0** | Native test bench — runs on the Mac, no board |
| [src/test_midi.cpp](src/test_midi.cpp) | **v0.6.0** | Native bench for the MIDI parser and the CC→ParamTable path |
| [run/test.command](run/test.command) | **v0.4.0** | Double-clickable wrapper for both test benches |
| [src/main.cpp](src/main.cpp) | **v0.329.0** | Loop, audio callback, fake panel over serial. Two chains + shared tail. **MIDI mirror across six channels** (`listen = send + 3`) with a 250 ms authorship window, a `vistos` inventory, physical freeze buttons, `qspi` probes, and a **2×2 routing matrix** (which chain feeds which output, with the ramps in a buffer so the dry, the freeze and the space all read the same value). **The version lives only in `COTUCA_OS_VERSION`** |
| [src/suave.h](src/suave.h) | **v0.1.0** | One-pole parameter smoother. Pure C++ — the fourth occurrence of the shape turning into a single one |
| [src/teste_tarol.cpp](src/teste_tarol.cpp) | **v0.2.0** | Native bench for the tarolator. **Declares what it does NOT prove**: it passed with sine waves while the effect sounded like a snare drum — a bench proves the algebra, not the timbre (scar 15u) |
| [src/teste_macro.cpp](src/teste_macro.cpp) | **v0.1.1** | Round trip of `Macro::Inverso`, which pot capture depends on. If it is wrong, the pot "doesn't catch" and the bench blames the ADC |
| [src/notch.h](src/notch.h) | **v0.2.0** | **Anti-microphonic notch**, two notches per chain, fixed Q 35, depth in dB. A peaking EQ with negative gain (and not a true notch, which has no adjustable depth). It was born out of a **solid-bodied** piece — exciter and piezo on the same plate, with no joint to lose energy in, which is why no mechanical gesture worked. It goes **before the gate**: if the feedback got through the gate, it would be the thing holding it open |
| [src/splash.h](src/splash.h) | **v0.11.1** | **The splash**: a cowrie-shell cast that falls from above in a fan, **settles**, and then one of the shells rises and grows until it becomes the mark — name letter by letter, version signing itself, mark held for 3 s. Seeded from the **ADC's own noise**, so every boot is a different cast. Three states: full splash at boot, **immediate** preset panel in normal use, and **the cast alone** when the cowries are really thrown. 20 Hz, **pure** C++, step marked `noinline`. **The mark is three pieces, not one**: a hollow shell (a ring made as *filled minus filled* — a double outline does not close at the tops), the lip mass with a `sin(π·t^1.5)` envelope, and the mouth opened only inside the lip, with **paired** upper/lower teeth biting half a gap. Top↔bottom mirror **0 px out of 1,215, measured**; the left↔right asymmetry is deliberate and was chosen by dB from a scale of measured tails. Mark bounding box: **45 × 27 px** at `cy=17` |
| [src/desenho.h](src/desenho.h) | **v0.3.1** | Primitives over the 1,024-byte framebuffer: `Pixel/Le/Linha/Retangulo/Caixa/Elipse/SuperElipse/Texto/Largura/TextoCentrado`. **Integer** Bresenham, no floats — the drawing on the board is bit-for-bit the one in the ground test. **Clipping lives only in `Pixel`**, not an `if` per primitive, which is how one gets forgotten. In `SuperElipse`, **a row of pixels is a band and not an edge** (width measured at `|y|−0.5`) and **the cap is a stroke** — it was the apex spike that, once removed, opened the outline at the top |
| [src/fonte5x7.py](src/fonte5x7.py) → [src/fonte5x7.h](src/fonte5x7.h) | **v0.1.0** | 43 glyphs as **readable art**, not blind hex; `--ver` redraws from the generated result, which closes the loop. A character with no glyph draws a **box**, never a space: a silently missing glyph is indistinguishable from correct text |
| [src/espectro.h](src/espectro.h) | **v0.2.0** | Hand-written radix-2 FFT, Hann, peak with **parabolic interpolation in dB**, and **dominance** — the fraction of energy at the peak, which separates a spike from a note. It replaced an autocorrelation that failed **6 out of 6** in the trial (peaks at multiples, and it measures periodicity instead of partial strength) |
| [src/tarol.h](src/tarol.h) | **v0.1.0** | **TAROLATOR** — percussive noise with the envelope of the strike, by frequency division. It was born as an octaver and does not do octaves: on a membrane the modes are inharmonic and the divider wobbles. The name is dB's, and the name is the diagnosis. Tone and tracking are FIXED: as params they gave dead travel |
| [src/params.h](src/params.h) | **v0.12.0** | `ParamTable` + `Macro` (ceiling **128** since 28/08, raised with 91 occupied — headroom by hand, not after the overflow). No dynamic allocation |
| [src/manifest.h](src/manifest.h) | **v3.0.0** | Two chains, shared character params, per-channel switch and dry/wet, **95 bindings across 55 CCs**, spread over six channels + CC 120 = sync. **Physical button table** (`BotaoAlvo`: param + LED + name) |
| [docs/operacao/gerar_ccmap.py](docs/operacao/gerar_ccmap.py) | **v0.11.0** | **Owner of `CCMAP.md` AND of the A4 card** — it writes both from `manifest.h`. The only hand-written part is the `NOTAS` table, and the generator flags an orphan note and a note that lies about topology ("shared" on a paired param). It found ten wrong notes on its first run |
| [docs/ccmap.pdf](docs/ccmap.pdf) | **generated** | **The reference card** — 91 bindings across 52 CCs on one A4 sheet, with pairs collapsed. `ccmap.html` is **generated** by `gerar_ccmap.py`; the PDF comes out of `ccmap-pdf.sh` / `.command` v0.1.0, which warns if it runs past one sheet |
| [docs/operacao/RETRABALHO-LOOPY.md](docs/operacao/RETRABALHO-LOOPY.md) | **generated** | The full map for remapping Loopy: 86 bindings across 49 CCs, by page |
| [esp32/ponte/ponte.ino](esp32/ponte/ponte.ino) | **v1.7.0** | **The second firmware.** BLE MIDI ↔ UART bridge on the ESP32-C3 — a wire with a radio: no CC map, no params. D28 active low, open-drain. `board` reports dropouts with recency and TX saturations, and since v0.5.0 **dropouts that survive a power cut** (NVS, deferred write): one count for this session, another for all time with the number of boots |
| [esp32/README.md](esp32/README.md) | — | Wiring, chosen pins, how to flash, and the environment known to work (⚠️ NimBLE **1.4.3**; 2.x does not compile) |
| [docs/operacao/ATERRAMENTO.md](docs/operacao/ATERRAMENTO.md) | **v0.2.0** | **The grounding chapter** — the three conductor types and the legal destination of each, star versus daisy chain, `AGND`/`DGND` and the narrow bridge, capacitors **by load type** (what the amp demands, the step-down refuses and the radio needs), high-impedance inputs, coupling through the air, and the measurement method **under load**. It owns the subject: when it disagrees with `HARDWARE.md`, it wins |
| [docs/operacao/GEAR-AUDIT.md](docs/operacao/GEAR-AUDIT.md) | **v0.1.0** | The decision to swap the WIDI Core for an ESP32-C3: US$ 45 → US$ 4 on a part replicated in series. **No plan B — acceptance is the gate**, and what it blocks is the order |
| [docs/operacao/VOO-ESP32-PONTE.md](docs/operacao/VOO-ESP32-PONTE.md) | **v0.1.0** | The BLE↔UART bridge flight. **The Daisy firmware does not change one line** — automatic `sync` is flight 2. Carries the three checks made before writing any code |
| [docs/operacao/gerar_projeto_loopy.py](docs/operacao/gerar_projeto_loopy.py) | **v0.2.0** | **Writes a COMPLETE `.lpproj`** from `manifest.h` — 9 pages, 143 widgets, colours by channel. It starts from a template and preserves the loop scene and the whole bundle: a project without `Clock` or `Sequence` will not even open. **Never writes over the template** |
| [docs/operacao/gerar_perfil_loopy.py](docs/operacao/gerar_perfil_loopy.py) | **v1.0.1** | Reads the project exported from Loopy (`.lpproj`, a zip with SQLite) and **emits the bindings** already addressed, with the CCs coming from `manifest.h`. ⚠️ The address is positional — regenerate after touching the layout |
| [docs/operacao/gerar_secao_loopy.py](docs/operacao/gerar_secao_loopy.py) | **v0.3.0** | **Owner of `LOOPY-SECAO.md`** — the build sheet for the new section, with CC, channel, widget type, range and birth value, plus the presets page. Generated because **six CCs** changed in a single session: a hand-written sheet drifts at the first change, and whoever follows it builds yesterday's instrument |
| [docs/operacao/LOOPY-SECAO.md](docs/operacao/LOOPY-SECAO.md) | **generated** | 105 bindings, 72 faders and 33 buttons, in learning order — it is not a layout: Loopy addresses by array index, which only exists once the widgets have been created |
| [docs/operacao/CCS-POR-PAGINA.md](docs/operacao/CCS-POR-PAGINA.md) | **generated** | The CCs in the order of Loopy's pages: central / mod a / mod b / time a / time b / random / tech |
| [docs/operacao/BINDINGS-A-MAO.md](docs/operacao/BINDINGS-A-MAO.md) | **generated** | Plan B: a manual entry sheet for Control Settings, with CC and channel per row |
| [docs/operacao/ACEITE-MULTIPLEXACAO.md](docs/operacao/ACEITE-MULTIPLEXACAO.md) | **living** | The MIDI acceptance script — the firmware side is green |
| [src/presets.h](src/presets.h) | **v0.11.0** | Preset structures and the **derived map version** (FNV-1a of the names; today **`0xBE1B`**). Pure C++ — provable end to end on a native bench. Preset membership is DERIVED from the name, and `route_*` is on the short exception list. Per-preset ceiling **56** |
| [src/test_presets.cpp](src/test_presets.cpp) | **v0.7.0** | Native preset bench: membership, hash (both halves) and every refusal verdict |
| [src/qspi_raw.h](src/qspi_raw.h) | **v0.10.0** | Contract for raw QUADSPI access. **Zero transitive closure** — nothing it calls may live in QSPI |
| [src/qspi_raw.cpp](src/qspi_raw.cpp) | **v0.10.0** | QUADSPI registers by hand, everything marked `COTUCA_ITCM`. **Erases, programs and reads** (`qspi prova` passes on the board). Leaving memory-mapped mode is **`CR.EN=0`, NOT `ABORT`** — and you also have to **leave continuous read** (`ABR=0x00`) or the chip does not hear opcodes. Safe zone enforced in code. See `docs/DESENHO-ESCRITA-QSPI.md` and scar 15fw |
| [src/cotucatronics_qspi.lds](src/cotucatronics_qspi.lds) | **v0.1.0** | Linker script forked from libDaisy (provenance note at the top). Adds `.itcm_text` → ITCMRAM |
| [src/teste_minimo/](src/teste_minimo/) | **v0.4.0** | **On-board proving bench.** Thirty lines that cut the problem in half in one flash: if it talks, the chain is good and the defect belongs to the app. It has already ruled out the board, the flashing, USB, audio init and 64 MB of SDRAM — and it prints what `CheckBoardVersion()` detected |
| [run/grava_placa_nova.command](run/grava_placa_nova.command) | **v0.2.0** | Flashes a **new** Daisy, whether factory-fresh or already carrying a bootloader — `flash.sh` only works once there is an app running. It waits for DFU and **decides the target from the memory map**, not from the PID: `0x90000000` is QSPI (bootloader present → `program-dfu`), `0x08000000` is ST's ROM (→ `program-boot`). ⚠️ Both DFUs enumerate as `0483:df11` — `STM_PID` and `DAISY_PID` are the **same value**, and discriminating by PID means flashing the bootloader over a board that only wanted the app. **The `program-boot` branch has never been exercised** |
| [src/flash.sh](src/flash.sh) | **v0.4.0** | Flashes **only if the build passed**, checks the version **in the source and on the board**, and **restores the state the flash wiped**. It enters the bootloader via `dfu go` and waits for DFU to appear, instead of asking for a RESET |
| [src/estado.py](src/estado.py) | **v0.2.0** | Saves the params before DFU and restores them after. ⚠️ It speaks in **`real`**, which is what `set` accepts — v0.1.0 sent `norm`, was refused with `out of range`, and **printed "restored" anyway** because I was throwing the reply away. It reads the reply, and shouts about whatever is refused |
| [src/porta_daisy.py](src/porta_daisy.py) | **v0.1.0** | Finds the Daisy **by asking**, never by position in the `glob`. It was born when the ESP32 joined the USB bus and there were suddenly two `usbmodem` devices — choosing by index became luck |
| [docs/operacao/auditar.sh](docs/operacao/auditar.sh) · [.command](run/auditar.command) | **v1.3.0** | **The mechanical sweep of `/audit`** — dead links, orphan exemptions, orphan and duplicate retirements, desynchronized destinations (repo/vault), `check_docs` and the version map. **It only reports what it can prove**: the judgement classes live in the skill, not here. On its first run it found 5 dead links in `MAPA-DE-TERRA.md` and one duplicate entry |
| [docs/operacao/hook_stop.sh](docs/operacao/hook_stop.sh) · [.claude/settings.json](.claude/settings.json) | **v1.0.0** | **`/audit` runs on its own at the end of every session, and WARNS.** `battle_scars` already had the rule — *"an automated habit beats a remembered habit"* — and every checker depended on somebody remembering. **It never blocks:** it always exits 0, because a gate that gets in the way is a gate that gets switched off. ⚠️ The count is **per section** — the pending-items pass lists 🔴/🟡 that live inside the documents, and counting them would mean a permanent alarm |
| [docs/operacao/hook_commit.sh](docs/operacao/hook_commit.sh) | **v1.0.0** | **The CHEAP checkers run before the commit** — `PreToolUse` on `git commit`, **70 ms**. `hook_stop` already caught these things, but at the end of the session, with the bad one already pushed: **the gain is not avoiding the mistake, it is having it arrive while there is still context to fix it.** It runs only the version map (0.03 s) and drawing freshness (0.02 s); `check_docs` (1.5 s) and `auditar` (3.3 s) stay at the close — an expensive check on every commit is a check that eventually gets removed. It warns, never blocks. On its first run it caught a stale drawing nobody knew was stale |
| [docs/operacao/derivados.py](docs/operacao/derivados.py) | **v1.2.0** | **Makes "generated" a refutable claim.** `manifest.h` was already the single source and the CC map gave different numbers in different documents — the cause was the label: three sheets claimed to be generated from it, nobody maintained them by hand *because of that*, and **no generator wrote them**. It checks who claims to be derived, from which version of the source, whether it matches today's, and **whether a generator exists that writes it**. The stamp that works names the SOURCE (`manifest v1.13.0`), not the generator |
| [docs/operacao/vizinhos.py](docs/operacao/vizinhos.py) | **v1.0.0** | Hunts the **sentence broken by a rename** — the case where the anchor matched, `grep` confirmed, `make` passed, and the sentence lost its meaning because the continuation was on the next line. It only flags when the edit touched the **end** of the line; `--termo=<name>` narrows it to where the removed tail carried the name (211 → 89 → 16 neighbourhoods) |
| [docs/operacao/coerencia.py](docs/operacao/coerencia.py) | **v1.1.0** | The checks `grep` does not do properly — exemptions that cover nothing (by `check_docs`' criterion: **between backticks**, file scope), health of the retirement list, and the **pending-items pass**. It was born fixing two bash checks that were accusing by the wrong criterion |
| [docs/operacao/versoes_readme.py](docs/operacao/versoes_readme.py) | **v1.2.0** | Checks the file map above against the files — `CLAUDE.md` demands the current version on every row and nothing was enforcing it. `--write` fixes them. **On its first run: 15 of 58 had drifted**, one of them with the README AHEAD of the source (`ponte.ino`). It reports what it cannot read, instead of skipping silently |
| [src/check_docs.py](src/check_docs.py) | **v0.14.0** | `make check-docs`: puts the docs on the stand against the manifest — phantoms, commands without help text, the pin map, ranges, and **two versions in the same file that disagree**. It aborts if `AddMidi` matches nothing. **It prints its own version on the first line**. The two-versions rule also covers `.py`, where there is no `#define`: it cross-checks the docstring, the constant and **what the script prints** |
| [docs/params-aposentados.txt](docs/params-aposentados.txt) | **living** | Retired params, removed symbols and external names — what the checker has licence not to find |
| [docs/DESENHO-ESCRITA-QSPI.md](docs/DESENHO-ESCRITA-QSPI.md) | **living** | The preset route: **RESOLVED on 29/09** — the window, the complete command table, the sequence that works, and why libDaisy refuses to write to QSPI when the program is running from it |
| [docs/FUTURO.md](docs/FUTURO.md) | **living** | **Possibilities, with the cost already established** — it is not a task list: it is what was considered and why it is not now. DPDT sense, a colour screen, the encoders that were removed, and the **budget of ten free pins** everything comes out of. It exists because the alternative is rediscovery: three times in one week a question was reopened that already had a measured answer, kept only in a conversation |
| [docs/PAUTA-ESCUTA.md](docs/PAUTA-ESCUTA.md) | **living** | What to listen to, in order — ~20 versions with no verdict from the ear |
| [src/Makefile](src/Makefile) | **v0.2.0** | Per-board build. `APP_TYPE=BOOT_QSPI`. No DaisySP |
| [.claude/skills/](.claude/skills/) | **living** | **The executable arm of IFÁ10000** — **fifteen** skills: **`aipcb`**, `ifa10000`, `medicao-limpa`, `instrumentacao`, `bug-de-ordem`, `edicao-segura`, `bancada-firmware`, `revisao-terra`, `kicad_layout`, `loopy`, `design-review`, `audit`, `caixa`, `pm`, plus the headstone of `kicad`. ⚠️ **`kicad` was RETIRED on 09/09/2026** and replaced by `aipcb` — its 1,033 lines are in [docs/rev_b/kicad-skill-aposentada.md](docs/rev_b/kicad-skill-aposentada.md), and only the headstone remains in `.claude/skills/kicad/`. The underlying difference: `kicad` described generators that wrote KiCad files by hand; `aipcb` describes a flow in which **nobody** writes KiCad files by hand. ⚠️ **`kicad_layout` was left with nobody to call it** — it said "called by the `kicad` skill", and `aipcb` does not mention it. Decision pending: it joins `aipcb`, it gets retired too, or it stays |
| [docs/desenhos/](docs/desenhos/) · [tools/desenhos.py](tools/desenhos.py) | **v0.4.0** | **The technical-drawing folder, and the tool that keeps it honest.** The schematics come out of `kicad-cli sch export svg` — the same source the factory reads, so they **cannot diverge**. `desenhos.py` compares dates: **if the source is newer than the drawing, the drawing is stale**, and it exits non-zero. No opinion in the criterion. ⚠️ These do not pass `svg_colisoes` (KiCad writes letters as *paths*) — we traded *"it can diverge but it gets checked"* for *"it cannot diverge but it does not get checked"*, and that is declared. `preamp-piezo.svg` died on 05/09 for saying `C2 1 nF` and `RV1` alive when the schematic already said `20n` and `DNP` — **an out-of-date figure with an official look is worse than none** |
| [docs/operacao/PINOS.md](docs/operacao/PINOS.md) | **living** | **The pin map, and the source is the code.** Checked at every `bts`. On 04/09 it was caught saying `Free: D0–D12` while listing the OLED as occupied three sections below — genuinely free is six |
| [docs/operacao/SUPRESSOR-MICROFONIA.md](docs/operacao/SUPRESSOR-MICROFONIA.md) | **living** | The **SuMiA™** contract, measured before the code, plus what it has already done in service |
| [docs/operacao/ATERRAMENTO.md](docs/operacao/ATERRAMENTO.md) · [TERRA-REFAZER.md](docs/operacao/TERRA-REFAZER.md) | **living** | The ground sweep and the bench sheet the grounding was redone with on 03/09 |
| [docs/operacao/ARQUITETURA.md](docs/operacao/ARQUITETURA.md) | **living** | The signal design, end to end |
| [docs/operacao/PRESETS.md](docs/operacao/PRESETS.md) · [DESENHO-PRESETS-RAM.md](docs/operacao/DESENHO-PRESETS-RAM.md) · [PRESETS-LISTA.md](docs/operacao/PRESETS-LISTA.md) | **living** | Presets: the contract, the RAM design, and the list |
| [docs/operacao/REMAP-ESCUTA.md](docs/operacao/REMAP-ESCUTA.md) · [ROTINAS.md](docs/operacao/ROTINAS.md) | **living** | The listening remap and the bench routines |
| [docs/operacao/gerar_ccmap.py](docs/operacao/gerar_ccmap.py) | **v0.11.0** | **The CC-map generator** — it writes `CCMAP.md` *and* the `ccmap.html` card, and **stamps the date, the commit and a dirty-tree warning** (dB's rule, 04/09). ⚠️ Do not write another one: on 04/09 I wrote a second without looking for this one, and `CCMAP.md` itself said on its 3rd line which was the right one |
| [docs/operacao/esquematicos/](docs/operacao/esquematicos/) | **living** | The carrier board in KiCad — `brief` and `bom` for revA, `brief` and `ERC-ISENCOES` for revB. Drawing rules in the `kicad` skill |
| [caixa/cotuca_caixa_corte.py](caixa/cotuca_caixa_corte.py) | **v0.7.0** | **The source of the enclosure.** It generates the cut from `--t` and `--kerf` — changing material means regenerating, because the tooth depth IS the thickness. The filename carries the thickness. Three gates of its own: nesting, **mirror** (a part that was rotated but in fact mirrored cuts fine and assembles backwards) and margin to the sheet edge. Three-shelf nesting computed from the real bounding boxes — at T=3 the parts grow and the old column left 1.4 mm of margin |
| [caixa/](caixa/) · [tools/nesting.py](tools/nesting.py) | **v0.1.0** | The rest of the physical artefact, and the external gate: everything inside the sheet, nothing overlapping, and **the part count** — an absence does not collide. ⚠️ On 06/09 the 6 fabrication pages, their generators and the walnut cut all died: they drew, or they were, **another** enclosure, and a superseded source alongside the right one measures just as well |
| [docs/COTUCA_BOM.md](docs/COTUCA_BOM.md) · [tools/bom_completa.py](tools/bom_completa.py) | **v0.10.2** | **The BOM for the whole instrument, GENERATED from the KiCad sheets** — both boards, the modules (Daisy, ESP32-C3, amp, step-down, decoy), the panel, the wiring and the enclosure. References, values, footprints and DNP come from the schematic; MPN and price live in the generator, and **every price declares whether it was measured or estimated**. It stamps the date, the commit and a dirty-tree warning. `--conferir` exits non-zero if it is out of date. ⚠️ **The MPN is a field of its own (`MPN`/`MPN_PAINEL`/`MPN_POR_FP`), separate from the description column** — and empty where the part is genuinely generic: a 1% 0805 resistor has no part number, it has a value and a package. And every suffix carries, in `SUFIXO`, the datasheet line that justifies it **and the one that does not** |
| [docs/operacao/esquematicos/revB/BOM-MPN-revB.md](docs/operacao/esquematicos/revB/BOM-MPN-revB.md) | **living** | **The BOM by MPN** — part by part, with manufacturer, package and tier. JLC assembles only the SMD; dB solders the rest. §8 records what the footprint assignment on 05/09 disproved: the `PTV09A-4` pot is bushingless and will not take a panel-mounted shaft, the `PEC11R` encoder is not the Alps `EC11E` (5.0 mm pin pitch against 2.5), and panel `R5`/`R6` disagree in value between the BOM and the schematic |
| [docs/ROTEIRO-BOOT-SRAM.md](docs/ROTEIRO-BOOT-SRAM.md) · [AUDIT-23-08.md](docs/AUDIT-23-08.md) | **historical** | Closed records: the SRAM boot script and the 23/08 sweep. Kept for what they explain, not as open items |
| [docs/FEATURES.md](docs/FEATURES.md) | **living** | **THE SINGLE BOX** — every feature in force, and the retired ones in the archive |
| [docs/CTOS.md](docs/CTOS.md) | **v0.4.0** | The fail-safe base, the CTOS/Effect boundary, the suite, and the **sum ceiling** (§3b) |
| [docs/PROCEDIMENTO-MIDI-LEARN.md](docs/PROCEDIMENTO-MIDI-LEARN.md) | **new** | Addressing the Loopy pages via `learn` mode. Includes the three "do nots" that cost a round |
| [docs/IFA10000.md](docs/IFA10000.md) | **living** | **Effect certification** — the 10 stages every effect goes through |
| [docs/STATUS.md](docs/STATUS.md) | **living** | **Where we are** — board status, open defects, next steps |
| [docs/operacao/PRESETS-LISTA.md](docs/operacao/PRESETS-LISTA.md) | **v0.9.0** | **The preset list and how the buttons address it** — CC 121 loads / 122 saves, the channel says the destination. It stays empty until the listening session; it carries the hash of the map in force (`0xBE1B`) |
| [docs/battle_scars.md](docs/battle_scars.md) | **living** | **Lessons and methodology** — the scars, so we do not pay twice |
| [docs/REFERENCIA-GUITARPEDAL-STM32.md](docs/REFERENCIA-GUITARPEDAL-STM32.md) | **living** | **External proof of the mechanics-first architecture** — GuitarML's open pedal, read from its `.kicad_pcb` and from their assembly process: the enclosure as a measuring instrument, long-pin pots, 17 of 101 footprints through-hole |
| [docs/ppc/007-mecanica-primeiro.md](docs/ppc/007-mecanica-primeiro.md) | **living** | **The panel is the datum, the bottom is a lid** — nine assembly symptoms, one cause; incorporated into `/aipcb` §2b, `/caixa` §0a, `/audit` and `HARDWARE.md` §0 |
| [src/TESTES.md](src/TESTES.md) | **v0.25.0** | Script T1–T12, the flashing flow and known defects |
| [src/BOARDS.md](src/BOARDS.md) | **v0.14.0** | Supported boards, the SDRAM rule and the bootloader note |
| [src/cotuca_serial.py](src/cotuca_serial.py) | **v0.4.0** | Serial monitor, no dependencies |
| [run/monitor.command](run/monitor.command) | **v0.1.0** | Double-clickable wrapper for the monitor |
| [src/cotuca_measure.py](src/cotuca_measure.py) | **v0.1.0** | Measures the output frequency by FFT |
| [src/teste_roteamento.py](src/teste_roteamento.py) | **v0.1.0** | **T7c automated** — proves both stages of the routing with the test tone. It asks for the port, finds the interface by name, **measures** the channel map and aborts if both outputs land on the same one (a criterion that cannot fail is not a criterion) |
| [run/measure.command](run/measure.command) | **v0.1.0** | Double-clickable wrapper for the meter |
| [tools/manual_montagem.py](tools/manual_montagem.py) | **v0.23.0** | **The assembly manual, IKEA style — generated, not written.** It reads the fabricated boards via `pcbnew` (outline, body of each part, pad 1, net of each pad, side, DNP), the package that went to the factory (`cpl_rico_*.csv`), the field lessons, and the Mouser order that arrived (`export/mouser_40440548.csv`, bag by bag), and it writes `docs/montagem/manual_montagem.html`: 49 steps, a table that translates the code on each Mouser label into the value to write on the bag, a map per step with the parts highlighted, and the ones that go **underneath** shown mirrored. Pin 1 is only marked when the pad shape distinguishes it (the diodes and the SOIC get an explicit mark). Runs with KiCad's Python |
| [tools/svg_colisoes.py](tools/svg_colisoes.py) | **v0.9.0** | **The exit gate for every SVG** — it measures text boxes and reports overlap. Four exit codes, and **3 = NOT VERIFIABLE** (zero texts read): a tool that cannot measure has to say so, not return "clean". **Since v0.3.0 it uses the `textLength` KiCad writes** — width measured by KiCad itself, not estimated. Until then it discarded every `<text>` with `opacity="0"`, which is how KiCad marks its own, and so it answered "not verifiable" about schematics with 336 measured texts inside them. ⚠️ On KiCad schematics it is **noisy** (every text comes out duplicated, and every label touches its own wire): for those the gate is `colisoes_sch.py` |
| [tools/desenhos.py](tools/desenhos.py) | **v0.4.0** | Guards the freshness of the **11** technical drawings: **source newer than the drawing = stale drawing**, and it exits non-zero. ⚠️ It verifies coherence, not correctness: with the generator crashing, the sheet does not change, and the drawing becomes "up to date" without the fix inside it |
| [tools/colisoes_sch.py](tools/colisoes_sch.py) | **v0.5.0** | **The schematic gate**, where `svg_colisoes` does not serve. It reads the `.kicad_sch`, builds the box of each text and each shape (wires and symbol bodies) and crosses them. The letter advance (0.85 of the body) was **measured** from KiCad's own `textLength`, not arbitrated. **It counts the texts that exist against the ones it read and refuses to approve an incomplete read** — v0.3.0 caught 0 of the 40 properties and announced "0 collisions" |
| [tools/contrato_idc.py](tools/contrato_idc.py) | **v0.2.1** | **The only contract between the two boards** — it reads the 26 IDC ways in both projects by geometry and checks one against the other. **An incomplete read exits with code 3**: on 05/09 v0.1.0 read 24 ways and announced "all 26 match", which is exactly the defect it exists to catch |
| [tools/cotuca_ble.py](tools/cotuca_ble.py) | **v0.7.1** | **The Daisy console WITHOUT A CABLE**, over BLE (`ppc/006` phase 1). The path is `Mac --BLE NUS--> bridge --SysEx @ UART--> Daisy`, and no leg of it is new: NUS lives beside BLE MIDI on the same server, and SysEx is the only frame that cannot be confused with a CC on the pair that already exists — **there is no spare wire** between the two boards. ⚠️ On macOS a process without Bluetooth permission is **killed with SIGABRT**, without being asked: use `consola-sem-fio.command`, which is launched by you and makes macOS ask |
| [tools/vigia_reset.py](tools/vigia_reset.py) | **v0.1.0** | **Catches Daisy resets by the SYSTEM's testimony**, not the board's: a Daisy that resets re-enumerates on USB, and the node in `/dev` is a macOS record, independent of the firmware. ⚠️ The version that asked the board **died with the board** (`device disconnected`) — the probe that questions the accused falls along with it. It has a heartbeat, because a probe that only speaks when there is news is indistinguishable from a dead probe |
| [tools/traducao_guarda.py](tools/traducao_guarda.py) | **v0.4.0** | **What the translation into English must NOT touch**: 122 param names and 14 console commands. ⚠️ v0.1.0 read the list from the live source and therefore **approved the very defect** it existed to catch; v0.3.0 accepted a log message that *narrates* the command as proof that it *works*. The hard gate is now the **construct that makes it exist** — the dispatcher's `strncmp` and the manifest's `t.Add` |
| [tools/portoes.py](tools/portoes.py) | **v0.1.0** | **Runs ALL the read-only gates in one command.** It exists because 48 tools lived in this repository and nobody ran them together: on 22/09 three were red without anyone knowing, and 15 of the manual's 78 figures had been overlapping for weeks with the checker already written. **A gate nobody runs is decoration.** It carries a **debt ratchet**: the known defects (50 documentation phantoms, 10 fit findings on the frozen rev C) have a declared line — below it passes, **above it fails**, and if it drops it tells you to lower the line. Read-only: the tools that write the board stay out, and it says why |
| [tools/colisoes_manual.py](tools/colisoes_manual.py) | **v0.1.0** | **The overlap gate for the manual's 78 figures.** It extracts each `<svg>` and **inlines the document's `<style>`** before measuring — without it the checker assumes 12 px and `text-anchor: start`, measures every centred label offset by half its width, and lies in both directions |
| [tools/pinos_ponte.py](tools/pinos_ponte.py) | **v0.1.0** | **Do the bridge firmware and the copper talk about the same pins?** It cost an afternoon at the bench: BLE connected, Loopy showed the controls, and not one byte ever arrived — the bridge was flashed on `GPIO4/5/6` and the board wires `IO20/21/3` |
| [kicad/vias_tented.py](kicad/vias_tented.py) | **v0.1.0** | Are the vias tented? An open via catches solder during assembly |
| [kicad/onde_furar.py](kicad/onde_furar.py) | **v0.3.0** | Where a hole can be opened in an **already fabricated** board — read-only |
| [kicad/grid_grafismo.py](kicad/grid_grafismo.py) | **v0.1.0** | The map for placing artwork by coordinate, not by dragging |
| [kicad/kit_montagem.py](kicad/kit_montagem.py) | **v1.0.0** | The home / hotplate assembly kit, built from the factory package |
| [kicad/cotuca_cerebro/](kicad/cotuca_cerebro/) | **rev B** | The mainboard schematic: hierarchical root + 4 sheets (power, preamp, socket, connector). ERC **11** violations, all of the same kind and all declared: reserve pins from `FUTURO.md`. `D29`/`D30` stopped counting — they carry `no_connect`, because they are the USB and they are not a reserve, they are a prohibition |
| [kicad/cotuca_painel/](kicad/cotuca_painel/) | **rev A** | The panel schematic: 1 encoder, 2 buttons, 2 pots, OLED, LED. **ERC 0 violations.** It uses the *same* library as the brain via `sym-lib-table` — one source for both boards |
| [kicad/cotuca_cerebro/cotuca.kicad_sym](kicad/cotuca_cerebro/cotuca.kicad_sym) | **34 symbols** | The library shared by both projects. `OPA1642` pinout taken from the `LM2904` in KiCad's official library — ⚠️ **to be confirmed against the OPA1642AIDR datasheet before submitting** |
| [kicad/cotuca_cerebro/gera_simbolos.py](kicad/cotuca_cerebro/gera_simbolos.py) | **v0.12.0** | Generates the library symbols. **Every number goes through `num()`**, which rounds to 4 places: without it `h/2 − 1.27 − 6×2.54` comes out as `1.7763568394002505e-15` and no reader using `[\d.]+` catches the line. It carries the **three-unit `OPA1642`** (05/09) — it replaced `OPA`+`OPAPWR`, which gave three BOM lines for one chip and had **pins 1 and 3 swapped** relative to the real SOIC-8 |
| [kicad/cotuca_cerebro/gera_daisy_sym.py](kicad/cotuca_cerebro/gera_daisy_sym.py) | **v0.3.0** | Generates the Daisy symbol from the manufacturer's CSV — since 05/09 the **Seed3** one, versioned in `kicad/cotuca_cerebro/`. It reads the columns **by header name**, and so it fails loudly when the source changes shape instead of emitting a wrong symbol. It refuses names that are MCU port designations and says which it discarded — the Seed3 CSV puts `PG3` on pin 40, which the manufacturer's own drawing calls `DGND` |
| [kicad/cotuca_cerebro/gera_soquete.py](kicad/cotuca_cerebro/gera_soquete.py) | **v0.19.0** | Generates the socket sheet. `pinos_de()` reads the geometry from the **library**, never from memory — and **counts the pins it read against the `(pin` blocks that exist** |
| [kicad/pcb_contorno.py](kicad/pcb_contorno.py) | **v0.24.0** | **Mechanical** outline and placement for both boards. It **imports** `caixa/cotuca_caixa_corte.py` — no parsing between the dimension and its use — and the board **centres itself on the bottom sheet**, which makes it **immune to the enclosure's thickness**: confirmed by running the generator at `T=3` and `T=6`, the mounting holes always land at (15.5 · 128.5) × (3.5 · 96.5) of the board and the shafts at 105/140/175 × 92 of the face. The thickness has already changed four times. Two gates that refuse to write: one if the board does not fit the enclosure, another if one of the 4 bottom mounting holes ends up under a component. **The copper pours come out on B.Cu only and every SMD ground pad gets a via with a stub reaching it** (`costura()`): copper on both layers leaves no layer for signal, and a two-layer board only has two — measured, 19 vias on the brain and 8 on the panel, and `via_dangling` only drops to 1 once the stub exists, because adjacent copper is not connected copper. The `y` inversion lives only in `da_tampa()`, and each pot/encoder shaft is **read from the footprint's `F.Fab` circle** — a footprint's origin is not the shaft |
| [kicad/sincroniza_pcb.py](kicad/sincroniza_pcb.py) | **v0.5.0** | Does what `Update PCB from Schematic` (F8) does, **without a GUI**: removes extra copies, adds what is missing, removes what is left over, and sets the net on each pad from the netlist. It exists because in this project the schematics are **generated** — every regeneration changes the UUIDs, F8 stops recognizing what is there and **adds a second copy**. `--zero` empties and restores everything. It refuses to save if the board and the schematic do not end up with the same references |
| [kicad/arruma_silk.py](kicad/arruma_silk.py) | **v0.21.0** | Moves reference fields off the pads. **Silkscreen only** — it does not touch copper, traces, zones or positions, so a routed board passes through here losing nothing. It is proved by the number that does NOT change: `silk_over_copper` 39 → 4 and unrouted connections 104 → 104. If no position will do, it **leaves it as it is and says so** — pushing it somewhere bad is moving the same problem, not solving it |
| [kicad/roteia.py](kicad/roteia.py) | **v1.18.0** | Routes **both layers**, with vias, and proves it broke nothing. One signal layer is a planar graph and the connections are not: the single-layer version stopped at 24 of 71, and with the second it went from **122 to 23**. A via costs 30 cells, so that only what has no way around takes one. **Acceptance rule: no net may get worse** — the profile of unrouted connections is measured before and after, because damage to the plane takes two forms (the zone split into islands, and the pad cut off from the copper) and a list of symptoms only protects against what has already happened. When one does get worse, it **bisects** to find the guilty crossing instead of throwing away the good ones: it isolated 2 out of 33. It exempts the baseline by the (kind, net) pair, never by the net's name. **It tears out the copper that is in the way, passes through, and puts back what it tore out** — the only thing that attacks the last connections, which no longer have a free path — and it **reverts if the number does not drop**, because tearing out a finished net to try another one can be a net loss. The acceptance rule counts by **(net, kind)**: counting by net alone let a pad that was connecting mask a plane that was splitting, and `roteia.py --auto-teste` interrogates the rule with that case. The retraction **filters candidates by geometry before spending DRCs on them** — only a crossing that passes within clearance+copper+clearance of a plane pad can orphan it — and that cut 27 candidates down to 5, with the same culprits and the pass dropping from ~13 minutes to 55 seconds. And **one tight pass at the end** (clearance 0.22, grid 0.085, search bounded to a 25 mm box): with wide clearance the router **cannot get out from between a connector's pins** — on a 2.54 header with 1.70 pads the gap is 0.840 mm and it needs 0.950 — which explained 9 failures on `J14`, 8 on `J1` and 11 on the Daisy, all in places with zero traces around them. Measured: tightening clearance from 0.35 to 0.26 **from the start** made it worse, 23 → 34 — a greedy router improves with less appetite, not with less clearance |
| [kicad/le_drc.py](kicad/le_drc.py) | **v0.2.0** | Reads the DRC report **by net**, and separates `Zone ↔ Zone` (cut plane) from `starved_thermal` (pad clinging to a narrow island). It exists because **the total lies by omission**: 71 → 52 with the AGND island split into eight is an improvement in the total and a regression on the board. The net name **is not on the finding's line** — it comes on the next two item lines, and an extractor that matches the header returns zero with the look of a clean board |
| [kicad/faz_placa.py](kicad/faz_placa.py) | **v0.17.0** | **From schematic to factory package, in one command.** Every step already existed; what was missing was the **order** — stitching after placement, routing after stitching, silkscreen after routing, gate after everything — and a step out of place does not give an error, it gives a board that passes every test and is wrong. It starts by **erasing all the copper**, because when placement can change, the copper that touched the old parts is rubbish. And it **runs the three attack orders on copies and keeps the best, by measuring**: there is no winning order, there is a winner on this board (brain `curta` 19 · `drc` 23 · `longa` 29; panel `drc` 10 · `longa` 11 · `curta` 13). One fewer decision for somebody to make |
| [kicad/roteia_fr.py](kicad/roteia_fr.py) | **v0.13.0** | **FreeRouting, which is a real router.** It exports Specctra DSN via `pcbnew`, runs FreeRouting, and imports the session back. On the same board and from scratch: my router left **12 unconnected in ~9 min**, this one leaves **8 in 35 s** — and the ones it has left are all plane connections, not signal: **it closes every signal.** It handles the four things it does not know about: it puts a *keepout* over the net-ties (otherwise it routes over the ground bridge and shorts it), runs with `-mt 1` because the tool itself warns that multithreading generates violations, **erases the copper first** — including the stitching, which with it goes from necessary to obstruction — and at the end connects by via+stub the ground pads the pour did not reach, **removing any vias left dangling**. ⚠️ It needs Java and `freerouting.jar`; without them the chain falls back to the in-house router |
| [kicad/classes_de_rede.py](kicad/classes_de_rede.py) | **v0.1.0** | Puts the trace widths in the **board's net classes**, not in a table inside a script. While the router was mine the table worked and hid the fact that the **DRC did not know about it**; when an outside router came in, it drew the input trace at signal width. It assigns by **name pattern**, so a new net joins the right class on its own. And it lowers the minimum width to 0.15 mm — JLC does 0.127, and it was my own rule that forbade the router from leaving a fine-pitch pad |
| [kicad/revisao_externa.py](kicad/revisao_externa.py) | **v0.2.0** | **The review I did not write.** It runs [kicad-happy](https://github.com/aklofas/kicad-happy) (MIT, 1,100★, tested against 5,800 projects) over both boards. It exists because everything else in this chain is mine — generator, router, gate and the checkers themselves — and **a self-test has the blind spots of whoever wrote it**, which has already happened here. On its first run it caught what we had never checked: **zero fiducials with 35 SMD components** (automated assembly needs three per side), **`J15` and `J16` without a single ground pin** carrying audio off the board, and 0/69 nets with a test point. It silences false positives **by (code, net) pair and with the reason written and dated** — never a whole family, and never silently |
| [tools/fichas.py](tools/fichas.py) · [docs/datasheets/](docs/datasheets/) | **v0.10.1** | **The datasheets for the parts with a real MPN**, downloaded and *checked from the inside*. It exists because `kicad-happy`'s downloader failed on all 19 and the cause was not the list: the `jlcsearch` API stopped serving the `datasheet` field — it **finds** the part and returns the field empty. The chain that works is two steps (`jlcsearch` → LCSC code → `wmsc.lcsc` → PDF), and it was not fixed inside the plugin's cache because the next update would erase it. Every PDF goes through `pdftotext` — an outside tool, one that can say no — and the number has to appear in the text. Five rungs: **OK** · **FAMILY** (series datasheet, suffix with a mandatory citation) · **IN THE CATALOGUE** (no datasheet of its own, but the number is in a series catalogue already on disk) · **SUSPECT** (the file does not mention the part — it blocks the gate) · **NO DATASHEET**, this one with the manufacturer's address. Status: **10 OK · 2 family · 1 in the catalogue · 8 with no datasheet** out of 21. Double-click wrapper in `run/fichas.command` |
| [kicad/mpn_nas_folhas.py](kicad/mpn_nas_folhas.py) | **v0.3.1** | Puts the **MPN on the schematic sheets**, stamped from the single source. It exists because the part numbers lived in a table inside the BOM generator, and the schematic knew nothing about them: an outside review read the sheets and said **`0/36 parts have MPNs — not pre-fab ready`**, and it was right. ⚠️ Two sheets have no generator, and the house rule forbids editing `.kicad_sch` by hand — so this **stamps and runs again** after any regeneration, instead of editing. And the factory gate runs `--conferir`: a copy without a checker drifts again. Result: **36 of 84 (43%)** — and this number went down on purpose. v0.2.0 stamped the BOM's **description column**, and the outside review said *"50/50 parts have MPNs"* because it only checks that the field is not empty: the 99% were phrases like `header 1x4 2.54` passing as part numbers. It only came to light when a tool tried to **use** the field and went looking for that on LCSC. The 48 without an MPN are resistors by package, plain headers and modules bought pre-assembled — **empty is an answer, not a gap** |
| [kicad/compara_netlist.py](kicad/compara_netlist.py) | **v0.1.0** | The netlist from before against the one from after, **by membership** — the set of `(reference, pin)` for each net, and never the names, which change on purpose. It is the acceptance criterion the `/kicad` skill §9 demands after every change to a generator. **It counts what it read**: I wrote a reader by eye that returned "0 nets" on a file with 74, and a reader that loses lines silently fails nothing — it approves everything. The guard was proved by blinding it to a block that is still in the file |
| [kicad/lista_roteamento.py](kicad/lista_roteamento.py) | **v0.3.1** | What is left to route, **read from the board and not from memory**, ordered in blocks with references, coordinates, widths and lengths. It exists because there was a hand-written list saying 71 connections when the board had 12: the placement changed three times and the list never found out. A wrong worklist is not an out-of-date document — it is somebody routing to places where the parts no longer are. **It counts what it read and declares what it left out**: pad↔zone connections are not traces, they are pads that lost the plane, and they come out in a section of their own instead of disappearing from the count |
| [tools/codigos_lcsc.py](tools/codigos_lcsc.py) | **v0.2.0** | **The LCSC code for every SMD line** — the MPN is for humans, and the factory will not assemble a line without a `Cxxxxx`. It reads the SMD lines from the footprint's **attribute** on the board (not from the library name) and resolves each one: a part with an MPN by exact match, a generic passive by a **verified** search. ⚠️ The distributor's search returns an **LED** first for `4.7MΩ 0805`; so the ordering decides nothing — fifty are requested and the description is required to match on value, package, class, tolerance (1%) and dielectric when the substitution is audible. **26 of 26**, each with the description that justifies it recorded in `codigos_lcsc.json` |
| [kicad/dnp_para_pcb.py](kicad/dnp_para_pcb.py) | **v0.1.0** | Carries the **DNP from the schematic to the board**. `(dnp yes)` lives on the symbols and `pcb export pos --exclude-dnp` reads the *footprint*'s attribute — the option was written and did nothing, and the CPL told the factory to place **eight parts the BOM was not buying**. It uses the `pcbnew` API to serialize the mark the way this version reads it |
| [kicad/rotula_tp.py](kicad/rotula_tp.py) | **v0.2.0** | Puts the **net name** beside each test point, on the silkscreen. The board said `TP1`…`TP11` and nothing else — the names lived in `Value`, which is on F.Fab and **does not get printed**. ⚠️ The name comes from the **pad's** `(net ...)`, never from `Value`: `Value` is a claim about the net and the pad **is** the net, and a wrong label in ink on the physical part cannot be corrected. The point's reference moves to Fab — at the bench `TP7` says nothing and `+3V3A` says everything. Done on the **panel** (5/5, clean gate). On the **brain** eleven scattered labels do not fit — 4 mm pitch against 6 mm — so `--legenda` comes in: **one block** of 12 lines, 14 mm from the row, with `text×text 0→0` and **zero collisions added**. ⚠️ There the reference leaves the silkscreen and here it **stays**: when the name is right beside it, `TP7` is noise; when it is somewhere else, it is the index |
| [kicad/refs_apontam.py](kicad/refs_apontam.py) | **v0.1.0** | **Does every reference point at its own part?** — the question no DRC asks, because it is not forbidden geometry, it is **meaning**. A reference 11.5 mm from its part and 5 mm from the neighbour passes every checker and identifies the wrong part; whoever solders by hand reads the nearest designator. There were **8**. §11g-ter of the `kicad_layout` skill had already predicted it in writing with nothing measuring it — diagnosed and awaiting instrumentation. `para_fabrica` refuses the package |
| [kicad/funcao_dos_pads.py](kicad/funcao_dos_pads.py) | **v0.3.1** | **Does the pad do what the datasheet says it does?** ERC, DRC and "0 unrouted" verify CONNECTION; none verifies MEANING. A generic symbol with pins `1,2,3` married to a footprint whose pads have fixed functions gives a correct netlist and a wrong function — that is how the OPA1642 inputs came out swapped on both channels, the AO3400A drain and source reversed, and the SS14 cathode flipped. The datasheet is indexed by **REFERENCE and not by footprint**: the same SOT-23 is both the AO3400A (1G 2S 3D) and the BAV99, and a per-footprint map would say the same thing about both. The contract never uses auto-generated net names (`Net-(U1A--)` changes when the defect is fixed). It is proved by **sabotaging a pad**: if it does not fail, it is worth nothing |
| [kicad/dentro_do_contorno.py](kicad/dentro_do_contorno.py) | **v0.4.0** | **Does the courtyard fit on the board?** Sister to `confere_furos`: outlines, not pads. KiCad **does not check courtyard against `Edge.Cuts`**, which is why the DRC said "geometry 0" with the panel's `J1` sitting 2.83 mm outside the outline — a connector with its body hanging out has all its pads inside; what sticks out is the plastic. It measures every courtyard vertex against the edge and every edge vertex against the courtyard, because a cut-out can enter between two vertices without coming near either. **A footprint with no courtyard is a finding, not a pass.** Exemptions one by one and with a number in the reason (the four M3s pass at 0.06 in the drawing and the real washer leaves 0.25 to spare). ⚠️ v0.1.0 failed 74 of 74 and 28 of 28 at `0.000` because it used `SHAPE_LINE_CHAIN.Distance()`, which on a closed chain measures the REGION and returns zero for every interior point — **identical zeros across everything are the reader, not the board** |
| [kicad/encaixe.py](kicad/encaixe.py) | **v0.7.0** | **Does the board assemble?** The IFÁ10000 M-track gate, **read-only**, called by `para_fabrica`. It measures M2 (≥3 non-collinear fixing points — holes, NPTH, or parts whose datasheet proves the thread), M4 (courtyard at the edge), M6 (bodies under the arcade buttons, by body and not by pads) and the **cables between boards** (3D path along the back wall + the enclosure's service slack, against the BOM). It does not redo arithmetic: it calls `pcb_contorno` and `dentro_do_contorno`. `--sabota` proves each check. It was born out of *"what is an electrically correct board worth if we cannot assemble it?"* (dB, 18/09) |
| [kicad/poe_furos.py](kicad/poe_furos.py) | **v0.2.0** | The brain's four M3 holes, with the coordinates **derived** from the enclosure's standoffs instead of written out, and a 3.5-radius keepout on both layers around each one. Idempotent via `uuid5`, and it confirms the result opens in KiCad before writing over the previous one |
| [kicad/para_fabrica.py](kicad/para_fabrica.py) | **v0.20.0** | The JLCPCB package, and the **gate that refuses to make a bad one**. An order is irreversible and takes weeks: the mistake is not discovered on screen, it is discovered when the box arrives. It measures geometry, unrouted connections **by net**, planes in pieces and orphan islands; it refuses if any of those fail; and only then does it export gerbers, drill, CPL and BOM, stamped with the generation date, the commit and a dirty-tree warning. `--incompleta` produces one anyway and writes **INCOMPLETA in the filename**, so that a bad package is never mistaken for a good one. It also says what it **cannot** see: that the netlist is right, and the assembly rotation — the CPL angle is the footprint's and the factory expects its library's. It also runs `fichas.py --confere`, which does not go to the network and refuses the package if any datasheet on disk does not mention the part |
| [kicad/gera_footprints.py](kicad/gera_footprints.py) | **v0.3.0** | The ESP32-C3 SuperMini footprint, which exists in no library. Pads with the **silkscreen names** and the side declared in capitals: this board's pinout turned up with three contradictory readings in two hours |
| [docs/operacao/verifica_datas.py](docs/operacao/verifica_datas.py) | **v0.2.1** | Checks every date in the documents against the line's `git blame`. None after the clock, none after the commit that introduced it. **A date is generated data, not written data** — and the `UserPromptSubmit` hook puts the clock in the context so there is no date to remember |
| [kicad/cotuca_cerebro/gera_conector.py](kicad/cotuca_cerebro/gera_conector.py) | **v0.10.0** | Generates the IDC26 connector sheet, brain side. Same counting guard. The note on the left is **right-aligned**: with a fixed offset, a long note grew into the label |
| [kicad/cotuca_painel/gera_painel.py](kicad/cotuca_painel/gera_painel.py) | **v0.17.0** | Generates the whole panel sheet. Same counting guard, plus `nota()` — **the sign of `y` lives in one place only**. On the sheet `y` grows DOWNWARD and in the symbol UPWARD; writing the continuation lines with `y − 3.8` printed 8 notes back to front |
| [kicad/cotuca_cerebro/encaixa_grade.py](kicad/cotuca_cerebro/encaixa_grade.py) | **v0.3.1** | Snaps the geometry to the 1.27 mm grid. **Acceptance criterion: identical netlist** |

## Environment known to work

| Item | Version |
|---|---|
| Board | Daisy Seed 2 (DFM) |
| Toolchain | ARM GNU Toolchain 15.3.rel1 (`arm-none-eabi-gcc 15.3.1`) |
| libDaisy | `~/dev/libDaisy` |
| dfu-util | 0.11 |
| Bootloader | Electrosmith `dsy_bootloader_v6_4-intdfu-2000ms.bin` (ships with libDaisy) |

> **Do not use Homebrew's `arm-none-eabi-gcc`.** It comes without newlib and
> libDaisy dies with `stdint.h: No such file or directory`. Use the cask (it
> asks for an admin password):
> ```bash
> brew install --cask gcc-arm-embedded
> brew install dfu-util
> ```

## Build and flash

```bash
git clone --recurse-submodules https://github.com/electro-smith/libDaisy ~/dev/libDaisy
make -C ~/dev/libDaisy
```

```bash
cd src && make BOARD=daisy_seed
```

To flash, **use `flash.sh` and not `make program-dfu`**:

```bash
cd src && ./flash.sh
```

It does three things the house rule demands and `make` does not: it flashes
**only if the build passed this time** (`make program-dfu` flashes the binary on
disk, which may be from an earlier failed build), it checks that the version
**on the board** matches the one **in the source**, and — since v0.4.0 —

> ⚠️ **it restores the state the flash wiped, and says which it restored.**

The params are volatile: flashing returns all of them to the manifest's birth
value. That has already cost four flashes in a row zeroing `level` without
warning, and — worse — a feedback notch that vanished on the flash.
`estado.py` saves before the DFU and restores after:

```
  estado: 122 params guardados
  placa diz v0.300.0
  CONFERE
  estado: 122 params conferidos, nenhum divergiu — nada a repor.
```

> ⚠️ **The everyday flashing manoeuvre is JUST THE RESET.** `BOOT + RESET`
> enters **ST's** DFU, which only sees the internal flash — and the app lives in
> QSPI. The symptom is `dfu-util: Last page at 0x9007760b is not writeable`
> (`Error 64`). `BOOT + RESET` is for `make program-boot`, once per board. A
> successful flash says `File downloaded successfully` and ends with `Error 74`
> on the `leave request` — that is normal.

### The app runs from QSPI, via the bootloader

Since v0.4.0 the firmware is compiled with `APP_TYPE=BOOT_QSPI`: **the app lives
in the 8 MB of QSPI and the 128 KB of internal flash belong to Electrosmith's
bootloader.** The reason is straightforward — with a single external DSP module
the firmware already occupied **94.14%** of the 128 internal KB; the second
module would not fit. In QSPI the same firmware occupies **1.52%** of 7,936 KB.

**Once per board**, flash the bootloader. Put the Seed into STM's DFU — **hold
BOOT, tap RESET, release BOOT** — and:

```bash
cd src && make program-boot
```

**Day to day, DO NOT touch anything.** The board enters the bootloader by
command, with an infinite timeout:

```bash
dfu go            # over serial — the board reboots into DFU and WAITS
cd src && make program-dfu
```

That turned flashes of 119, 66 and 47 attempts (racing the physical RESET's
~2 s window) into **one**. See the `bancada-firmware` skill.

**If you need the manual path** (locked-up board, firmware without the
command): just tap **RESET** and flash during the bootloader's grace window
(LED pulsing, ~2 s; holding BOOT extends the window):

```bash
cd src && make program-dfu
```

`make program-dfu` **always** ends with `Error 74` on the `leave request`: the
board jumps to the app and stops answering the last `get_status`. If
`File downloaded successfully` appeared, it flashed.

To go back to internal flash: `make clean && make APP_TYPE=BOOT_NONE` and flash
over STM's DFU — but that erases the bootloader.

## Testing

```bash
cd src && ./cotuca_serial.py --send board list "k1 0.5" --gap 1.3
```

Or double-click `run/monitor.command`.

Firmware commands: `board` · `list` · `mods` · `cpu` · `dyn` · `probe` ·
`bb` · `midi` · `miditest` · `midiloop` · `b1` · `b2` · `k1..k5 <0-1>` ·
`set <name> <real>` · `tone on` · `tone off`

`b1` and `b2` **simulate the panel buttons over serial**, so panel v2 is
testable before it physically exists: `b1` fires the randomizer, `b2` holds the
flagged parameter. `bb` reads the **black box** — memory in BACKUP_SRAM that
survives a RESET, to measure what only happens after the serial has already
died.

`set` reaches **any** parameter by name, **in real units** — Hz, ms, × — and
refuses an out-of-range value while quoting the limits. With knobs 1–3 on the
delay and the level, `test_tone_freq` and `test_tone_level` are **serial-only by
design** — the test tone is bench, not performance:

```
set tone_freq 440        # Hz, direct
set gate_release 500     # ms
set bench_boost 2        # times
```

Knob and macro keep speaking normalized (travel position is normalized by
nature); real units only exist at the human boundary.

### Parameter curve

`ParamTable::Add()` takes an optional exponent:

```
real = min + pow(norm, curve) * (max - min)
```

`curve = 1.0` is linear (the default, with a fast path that skips the `pow`).
`dly_time` uses **2.5**: half the knob's travel covers the first ~18% of the
range, which is where the short delays live.

**Curve is a manifest decision, not a module decision.** It is ergonomics — how
the knob behaves in the hand. The DSP receives the already-curved value and does
not know a curve happened; the same DSP engine serves sculptures with different
curves.

`cpu` reports the audio block's load (average and peak). With the whole suite
(plus shimmer and detune): **59.5% average, 65.8% peak** of the 1,000 µs
deadline, measured and recorded in [STATUS.md](docs/STATUS.md). The delay line
is constant: it reads and writes every sample regardless of the setting.

`mods` shows the loaded DSP module, the parameters it exposed (discovered **by
name**, not by index), the manifest's bindings and the SDRAM allocator's memory
statement.

### MIDI

MIDI port on **USART1 at 31250 baud** — TX on `D13` (pin 14), RX on `D14`
(pin 15). WIDI soldering guide, **including the TX↔RX crossover warning**, in
[BOARDS.md](src/BOARDS.md).

The CC→parameter map lives in the manifest and starts like this:

| CC | Parameter | Why |
|---|---|---|
| 20 | `dly_time` | also on knob 1 |
| 21 | `dly_mix` | also on knob 2 |
| 22 | `pre_level_ab` | also on knob 3 |
| 23 | `src_mix` | also on knobs 4/5 |
| 24 | `gov_ceiling` | **no knob** — the feedback edge is only reachable over MIDI or serial |

Channel configurable in the manifest, **omni by default**. CCs 20–24 are the
spec's "undefined" range, so they do not collide with mod wheel, volume or
sustain.

> **MIDI is a remote knob, not a new path.** The CC hands a normalized value to
> the `ParamTable` and stops there — exactly where the physical knob stops. The
> manifest's curve, `Shaped()` and `Real()` happen afterwards, identically for
> any origin. The native bench verifies this: the same gesture via CC and via
> knob gives bit-for-bit the same value.

**Priority between sources: last write wins.** Serial, MIDI and the future
physical knob are peers, with no hierarchy — the whole reasoning is at the top
of [main.cpp](src/main.cpp).

`midi` shows the port's state, the parser's counters, the map and the last CCs
received. It is the first thing to consult on soldering day: it says **which
stage the signal stopped at** — wire, parser, channel or map.

`miditest <cc> <val>` injects bytes into the RX queue without going through the
UART (it proves the parser and the map); `midiloop <cc> <val>` sends over the
wire and needs a `D13→D14` jumper to come back — it is the only one that proves
the baud rate and the pin. Script in [T10](src/TESTES.md).

### Native test benches

```bash
cd src && ./test.command
```

`dynamics.h` and `midi.h` are pure C++ and board-agnostic, so the block
arithmetic and the MIDI state machine run on the Mac — no board, no flash.
**They do not replace on-board testing**: neither of them touches hardware, and
neither proves the UART.

`k5` is the **SRC MIX**: a linear crossfade between `IN_L` (piezo, `0.0`) and
`IN_R` (line jack, `1.0`), applied before any processing. While the 5-pot panel
does not exist, `k4` also points at it — which is why `req.min_knobs` stays at
4: a 4-knob board can still reach the SRC MIX.

Since the physical knobs are not wired yet, `ReadKnob()` returns `-1` and the
panel lives on the serial. Conversion to a target tuning, with `test_tone_freq`
in `[80, 2000]`: `norm = (Hz − 80) / 1920` — so **440 Hz = `k2 0.1875`**.

### Measure instead of trusting the ear

With `OUT_L` on one of the Mac's audio inputs (here: MiniFuse 4, channel 2):

```bash
cd src && ./cotuca_measure.py --scan --expect 440
```

It reports the dominant frequency by FFT with parabolic interpolation, the level
in dBFS and the error in cents. Or double-click `run/measure.command`.

Measured across the whole range (80 / 440 / 880 / 1760 / 2000 Hz): a constant
error of **+0.4 to +0.5 cents**. Being constant in *relative* terms, it is a
clock difference — the Daisy's real SAI rate is ~0.026% above the 48000 Hz that
`AudioSampleRate()` reports and that the oscillator uses as its base. It is not
a code error, and it is inaudible (1 cent is ~1/100 of a semitone).

## Gotchas

- **No `hal::Log()` inside the audio callback.** It blocks for milliseconds.
- **`--specs=nano.specs` swallows `%f` silently.** The Makefile already forces
  `-u _printf_float`; remove it and half the panel's output disappears without
  warning.
- **The USB RX callback runs in an ISR.** Never call `Log()` from there — that
  is exactly what locked the board up before v0.2.1. Queue it and dispatch in
  `PollSerial()`.
- **Do not use libDaisy's `Logger`.** Its `tx_buff_[128]` is single and shared:
  two consecutive lines overwrite each other mid-flight. `hal::Log` transmits on
  its own, with a double buffer and retry.
- **`sdram_pool` asks for the entire SDRAM region, and now it shows up as 100%.**
  Until v0.3.0 `--gc-sections` discarded the pool because nothing called
  `AllocBig`. The delay, the freeze, the reverb and the pitch engines all call
  it, so the 64 MB came back all at once: the map shows `SDRAM:
  100%`. **It is the pool, not the consumption** — but there is no room left for
  anything else in `.sdram_bss`.
- **`AllocBig()` is the only access to SDRAM.** Declaring `DSY_SDRAM_BSS`
  outside `hal_daisy_seed.cpp` is forbidden — the full rule is in
  [BOARDS.md](src/BOARDS.md). Boot prints the statement (`SDRAM via AllocBig: N
  de M bytes`); if a module allocates outside it, the number does not add up.
- **`FLASH: 0 B` in the memory map is correct.** The app runs from QSPI; the
  internal flash belongs to the bootloader. It is not an empty build.

## Open defects

**The living list is in [STATUS.md](docs/STATUS.md)** — that is where they are
opened and closed. Today's summary, 04/09:

- **🔴 The serial dies under a long callback** — priority 1. The callback's
  ~3.4 ms bursts starve the USB ISR. Worked around with `log_mute`, the black
  box and MIDI; the right fix (**kick-and-forget log ring**) is designed and not
  done.
- **🟡 SRAM pool at 92.4%** — a third module will not fit without changing the
  allocation strategy.
- **🟡 Block 256 costs 5.3 ms of latency** — the reason the block is 48.
- **🟢 Off-by-one in the applied counter** — a fixed difference of 1, not a loss
  rate. Confirm before calling it cosmetic.

Diagnosis of the ones already closed: [TESTES.md](src/TESTES.md).

## Signal path of the whole system

**Assembled and playing.** The preamp went in on 04/09 and the grounding was
redone on 03/09; what is still unconfirmed with the multimeter is marked in
[STATUS.md](docs/STATUS.md). The living pin map is
[PINOS.md](docs/operacao/PINOS.md), checked at every close:

- **Inputs.** `IN_L` = piezo via a high-impedance buffer (external circuit);
  `IN_R` = line TS/TRS jack. **Pot 5** does the `piezo_mix`, which is a
  **balance, not a crossfade**: at the centre both sources come in at full. (The
  `src_mix`, which was a crossfade and dropped both by −6 dB at the centre, was
  retired on 20/08 — see [FEATURES.md](docs/FEATURES.md).)
- **Outputs.** `OUT_L` → TPA3116 amp → exciter on the sculpture's body;
- **Panel (22/08).** `D24` freeze A switch · `D27` freeze A LED ·
  `D22`/`D23` freeze B (firmware ready, wiring pending) · `D25` B3 CTOS
  rearm · `D26` B1 randomizer · `D28` WIDI LB · `D29`/`D30` reserved for the
  exciter DPDTs · `D13`/`D14` MIDI UART (**tied to the peripheral**: USART1 on
  PB6/PB7, not remappable) · `D15`–`D19` pots (A0–A4) · free: `D0`–`D12`,
  `D20`, `D21`.
  `OUT_R` → jack out. Routing (exciter / jack / both) is firmware, via a
  3-position toggle.
- **Switched (NC) jacks** act as insertion sensors over GPIO — they do not route
  signal.
- **Fixed panel.** 4 character knobs + SRC MIX + encoder with push + 2 momentary
  buttons + toggle + RGB LED.

## Next steps

**The living queue is the one in [STATUS.md](docs/STATUS.md)** — it is not
duplicated here, which is how the two versions diverge. What is at the top of
it, as of 04/09:

- **Redo the clean-measurement pre-flight.** The preamp went in on 04/09 and
  invalidated every earlier gain measurement.
- **Confirm `AGND`/`DGND` with the multimeter**, in AC/mV and under load.
- **Kick-and-forget log ring** — kills the 🔴 priority-1 defect by architecture,
  instead of working around it.

Only what is architectural and does not age stays here:

- Second board: **Teensy 4.1**, via a new `hal_teensy41.cpp` — without touching
  anything else. That is what the `hal.h` boundary exists to allow.
