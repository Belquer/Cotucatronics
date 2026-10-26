# Cotucatronics OS
<!-- doc-check: ignore no_connect (KiCad ERC violation code) -->
<!-- doc-check: ignore dentro_do_contorno (tool in kicad/, outside check_docs's reach) -->
<!-- doc-check: ignore para_fabrica (tool in kicad/, outside check_docs's reach) -->
<!-- doc-check: ignore pcb_contorno (tool in kicad/, outside check_docs's reach) -->

<!-- doc-check: ignore hook_stop (script in docs/operacao/, not a param) -->
<!-- doc-check: ignore svg_colisoes (tool outside src/. check_docs sweeps `src/`; these live in `tools/`, `docs/desenhos/caixa/` or `kicad/cotuca_cerebro/`, which it does not read. It is not a ghost: it is the checker's reach limit.) -->

Modular firmware for **sound-sculpture instruments**. The central idea is that
the same brain board runs any sculpture: the instrument code is board-agnostic,
and swapping hardware means writing a new file — not rewriting
the system.

A collaborative project.

> ### State, 2026-09-04 — **v0.301.0**
>
> **480 MHz · 48 kHz · block 48** (1,000 µs deadline) · app on the QSPI
>
> **Two character chains** (drive, phaser, warble, tarolator via piezo) over
> a **shared tail** (delay, freeze, shimmer↺reverb), with a **2×2 routing
> matrix** — which chain feeds which output.
>
> **Four-layer anti-feedback stack**, all in shell C++: gate,
> Governor, limiter and SSB shifter — plus **SuMiA™**, an automated microphonics
> suppressor, four mouths per branch, that hunts and fills on its own.
>
> **MIDI on six channels** via a BLE↔UART bridge on an ESP32-C3 (`ch1` shared
> · `ch2` chain A · `ch3` chain B · listen = send + 3), **81 CCs**, presets
> in RAM, **black box** in BACKUP_SRAM.
>
> **Panel soldered and live:** SSD1309 OLED over SPI with an animated opening, **three
> encoders**, **two pots** (volume per branch) and **four buttons** with two LEDs.
> The map is in [`docs/operacao/PINOS.md`](docs/operacao/PINOS.md), which is the
> source — and the source is what rules over it.

> ### What has NOT yet passed through the ear or the multimeter
>
> - **The compressor has never been heard.** It has existed since 02/09 (`dynamics.h`) and
>   has never been turned on with sound.
> - **Four items from the [listening agenda](docs/PAUTA-ESCUTA.md)** — the top of
>   `drv_sujeira` (only approved at 0), the shimmer with real transposition, the
>   rewritten delay, and ~20 versions with no ear verdict.
> - **The broadband hiss** is diagnosed down to a named rule —
>   `AGND`/`DGND` joined off the board, P3 of the ground sweep — and the
>   grounding was redone on 03/09 with cleaner sound, **but the measurement with
>   a multimeter has never been done**.
> - **The DPDT commons** await two continuity checks: is the amp BTL? and is the
>   terminal that goes to the island a power output or just the TRS sleeve?

---

## Architecture

Five rules that are not up for negotiation:

1. **HAL boundary.** No vendor `#include` outside
   `hal_<board>.cpp`. `main.cpp`, params, macros and manifests do not know which
   board they are running on. The board is chosen with `make BOARD=daisy_seed`.
2. **Normalized parameters.** Everything is `0.0–1.0` in a central table; the manifest
   is what translates to Hz, seconds, dB.
3. **Macros.** One physical knob can move several parameters, with weight and inversion.
4. **Manifest per instrument.** It is the only file that changes from sculpture to
   sculpture. It declares `Requirements` — big
   RAM, audio in, number of knobs — checked at boot: an insufficient board gives
   a clear error instead of a mysterious bug on stage.
5. **`AllocBig()`.** Abstracts the 64 MB SDRAM. A bump allocator, only at boot, never
   in the audio callback. A board without big RAM returns `nullptr` — the caller
   **must** check.

## File map

| File | Version | What it is |
|---|---|---|
| [src/hal.h](src/hal.h) | **v0.38.0** | The contract. The vendor-independence boundary |
| [src/hal_daisy_seed.cpp](src/hal_daisy_seed.cpp) | **v0.76.0** | The only file that may include `daisy_seed.h`. **4 buttons** (D26, D24, D25 CTOS re-arm, D22 freeze B) and **2 panel LEDs** (D27, D23). 30 ms debounce, measured. **Live SSD1309 OLED** (SPI1: D5 RST · D6 DC · D7 CS · D8 CLK · D10 DIN) with a wire probe and bridge-hunter. **Internal PULL-UP on the MIDI RX (PB7)** — libDaisy starts the UART with `GPIO_NOPULL`, and the pin floating with the bridge off put the USART into a rate storm and killed the loop in 4 s. **USART1 at priority 1**, below its own DMA stream, otherwise the asynchronous abort never completes. **`DmaListen` re-arm with ceiling and backoff** — without it, it ran at the loop's pace and consumed it (10,029 re-arms with a healthy bridge). **Whole framebuffer via `OledDesenha`** (`memcpy`, not 8192 `DrawPixel`), and the SPI prescaler is born at **PS_8** — 3,811 µs per frame, verified on the glass with a 1 px pattern. PS_64 cost 23,401 µs. **ONE encoder** since 05/09 (there were three; dB closed the interface down to a single one, and that dissolved for free the `D17` conflict, which was the push of encoder G *and* the A2 of the rail divider) — and `kEncPin` is the SOLE source — `Init` reads from it and the names come out via `EncoderPinNome`. **`VarreTudo`** reads the 31 pins (kills OLED and MIDI until a reboot) and **`Reiniciar`** undoes it. `PollMidi`/`PollSerial` drain by a **snapshot** of the pointer — chasing the live pointer locked up the board on 28/08 |
| [src/reverb.h](src/reverb.h) | **v0.2.0** | Freeverb + high-pass. Pure C++, buffers come from outside |
| [src/freeze.h](src/freeze.h) | **v0.3.1** | Granular freeze: 500 ms core, 2 randomly drawn grains, Hann. Pure C++ |
| [src/pitch.h](src/pitch.h) | **v0.5.0** | Shared pitch engine: 1 buffer, up to 4 voices. **Automatic anti-alias on write** (6th-order Butterworth, zero cost if no voice rises). Pure C++ |
| [src/shimmer.h](src/shimmer.h) | **v0.3.0** | Shimmer: reverb + pitch in a loop. Feeds back into the existing Freeverb. **Tone = cutoff + presence bell at 3 kHz** |
| [src/drive.h](src/drive.h) | **v0.4.1** | Drive: normalized tanh, **1.5–48×** (the knee has to reach −30 dBFS), + `drv_sujeira`: morph to a wavefolder with its own anti-alias |
| [src/phaser.h](src/phaser.h) | **v0.3.0** | Phaser: N allpasses swept by a triangular LFO. Zero buffer |
| [src/warble.h](src/warble.h) | **v0.1.1** | Warble: tape drift via a short modulated delay. Its dry/wet is called `wrb_mix` and **is a name, not a param** |
| [src/detune.h](src/detune.h) | **v0.1.0** | Detune. **OUT of the build since 20/08** — revivable, see `docs/FEATURES.md` |
| [src/delay.h](src/delay.h) | **v0.2.0** | Delay in C++: 1 s line, fixed 0.4 feedback, 50 ms floor. **Read clamp against NaN/Inf, with a `clamps_` counter** |
| [src/effect_slot.h](src/effect_slot.h) | **v0.3.0** | Frame: asymmetric fade, zero-cost when off, timing |
| [src/pitch_spectrum.py](src/pitch_spectrum.py) | **v0.2.0** | The N largest peaks of the spectrum — a dominant peak lies when the sidebands are strong |
| [src/dynamics.h](src/dynamics.h) | **v0.6.0** | Gate, governor, limiter and **SSB shifter**. Pure C++ |
| [src/midi.h](src/midi.h) | **v0.8.1** | MIDI state machine + **the six channels** and the `escuta = manda + 3` invariant, locked in a `static_assert`. Pure C++, board-agnostic |
| [src/test_dynamics.cpp](src/test_dynamics.cpp) | **v0.1.0** | Native test bench — runs on the Mac, no board |
| [src/test_midi.cpp](src/test_midi.cpp) | **v0.6.0** | Native bench for the MIDI parser and the CC→ParamTable path |
| [src/test.command](src/test.command) | **v0.4.0** | Double-clickable wrapper for the two test benches |
| [src/main.cpp](src/main.cpp) | **v0.303.0** | Loop, audio callback, fake panel via serial. Two chains + shared tail. **Six-channel MIDI mirror** (`escuta = manda + 3`) with a 250 ms authorship window, `vistos` inventory, physical freeze buttons, `qspi` probes, **2×2 routing matrix** (which chain feeds which output, with the ramps in a buffer so the dry, the freeze and the space read the same value). **The version lives only in `COTUCA_OS_VERSION`** |
| [src/suave.h](src/suave.h) | **v0.1.0** | One-pole parameter smoother. Pure C++ — the fourth occurrence of the form becoming the single one |
| [src/teste_tarol.cpp](src/teste_tarol.cpp) | **v0.2.0** | Native bench for the tarolator. **Declares what it does NOT prove**: it passed with sine waves while the effect sounded like a box — the bench proves the algebra, not the timbre (scar 15u) |
| [src/teste_macro.cpp](src/teste_macro.cpp) | **v0.1.1** | Round trip of `Macro::Inverso`, which the pot capture depends on. If it is wrong, the pot "won't catch" and the bench blames the ADC |
| [src/notch.h](src/notch.h) | **v0.2.0** | **Anti-microphonics notch**, two mouths per chain, fixed Q 35, depth in dB. A peaking EQ with negative gain (and not a pure notch, which has no adjustable depth). It was born from a **welded** body part — exciter and piezo on the same plate, no joint to lose energy, and so no mechanical gesture worked. It comes in **before the gate**: if the microphonics passed through the gate, it would be the one holding it open |
| [src/splash.h](src/splash.h) | **v0.11.1** | **The opening**: a cowrie-shell throw that falls from above in a fan, comes to **rest**, and then one of the shells rises and grows until it becomes the mark — the name letter by letter, the version signing itself, the mark holding 3 s. Seeded from the **ADC's own noise**, and so each start is a different throw. Three states: a full opening at start, an **immediate** preset panel in normal use, and **just the throw** when the cowrie shells fire for real. 20 Hz, **pure** C++, `noinline` step. **The mark is three pieces, not one**: a hollowed hull (a *filled minus filled* ring — a double outline does not close at the tops), the lip mass with a `sin(π·t^1.5)` envelope, and the mouth open only inside the lip, with **paired** top/bottom teeth that bite half the gap. Top↔bottom mirror **0 px out of 1215, measured**; the left↔right asymmetry is on purpose and was chosen by dB on a scale of measured tails. Mark box: **45 × 27 px** at `cy=17` |
| [src/desenho.h](src/desenho.h) | **v0.3.1** | Primitives over the 1024-byte framebuffer: `Pixel/Le/Linha/Retangulo/Caixa/Elipse/SuperElipse/Texto/Largura/TextoCentrado`. **Integer** Bresenham, no float — the drawing on the board is bit for bit the one from the test on the ground. The **clipping lives only in `Pixel`**, and not an `if` per primitive, which is how one gets forgotten. In `SuperElipse`, **a row of pixels is a band and not an edge** (width measured at `|y|−0.5`) and **the cap is a stroke** — it was the apex spike that, when removed, opened the outline at the top |
| [src/fonte5x7.py](src/fonte5x7.py) → [src/fonte5x7.h](src/fonte5x7.h) | **v0.1.0** | 43 glyphs in **readable art**, not blind hex; `--ver` redraws from the generated result, which is the full round trip. A character with no glyph draws a **box**, never a space: a silently missing glyph is indistinguishable from correct text |
| [src/espectro.h](src/espectro.h) | **v0.2.0** | Hand-written radix-2 FFT, Hann, peak with **parabolic interpolation in dB**, and **dominance** — the fraction of the energy at the peak, which separates a spike from a note. It replaced an autocorrelation that failed **6 of 6** on the test (peak at multiples, and it measures periodicity instead of partial strength) |
| [src/tarol.h](src/tarol.h) | **v0.1.0** | **TAROLATOR** — percussive noise with the envelope of the hit, by frequency division. It was born an octaver and does not make an octave: on a membrane the modes are inharmonic and the divider trembles. dB's name, and the name is the diagnosis. Tone and tracking FIXED: as params they gave dead travel |
| [src/params.h](src/params.h) | **v0.12.0** | `ParamTable` + `Macro` (ceiling **128** since 28/08, raised with 91 occupied — slack in hand, not at the overflow). No dynamic allocation |
| [src/manifest.h](src/manifest.h) | **v1.13.0** | Two chains, shared character params, switch and dry/wet per channel, **95 bindings across 55 CCs**, spread over six channels + CC 120 = sync. **Physical button table** (`BotaoAlvo`: param + LED + name) |
| [docs/operacao/gerar_ccmap.py](docs/operacao/gerar_ccmap.py) | **v0.11.0** | **Owner of `CCMAP.md` AND of the A4 card** — writes both from `manifest.h`. The only part by hand is the `NOTAS` table, and the generator reports an orphan note and a note that lies about topology ("compart." on a paired param). It found ten wrong notes on its debut |
| [docs/ccmap.pdf](docs/ccmap.pdf) | **generated** | **The reference card** — 91 bindings across 52 CCs on an A4 sheet, with the pairs collapsed. `ccmap.html` is **generated** by `gerar_ccmap.py`; the PDF comes from `ccmap-pdf.sh` / `.command` v0.1.0, which warns if it goes past one sheet |
| [docs/operacao/RETRABALHO-LOOPY.md](docs/operacao/RETRABALHO-LOOPY.md) | **generated** | The whole map for remapping Loopy: 86 bindings across 49 CCs, per page |
| [esp32/ponte/ponte.ino](esp32/ponte/ponte.ino) | **v0.4.0** | **Second firmware.** BLE MIDI ↔ UART bridge on the ESP32-C3 — a wire with radio: no CC map, no params. D28 active low, open-drain. `board` brings drops with recency and TX saturations, and since v0.5.0 **drops that survive a power cut** (NVS, deferred write): one count for this session, another for all time with the number of starts |
| [esp32/README.md](esp32/README.md) | — | Wiring, chosen pins, how to flash, and the environment known to work (⚠️ NimBLE **1.4.3**, 2.x does not compile) |
| [docs/operacao/ATERRAMENTO.md](docs/operacao/ATERRAMENTO.md) | **v0.2.0** | **The ground chapter** — the three conductor types and the legal destination of each, star vs. daisy-chain, `AGND`/`DGND` and the narrow bridge, capacitors **by load type** (what the amp demands, the step-down refuses and the radio needs), high-impedance inputs, coupling through the air, and the measurement method **under load**. It owns the subject: when it disagrees with `HARDWARE.md`, it rules |
| [docs/operacao/GEAR-AUDIT.md](docs/operacao/GEAR-AUDIT.md) | **v0.1.0** | The decision to swap the WIDI Core for an ESP32-C3: US$ 45 → US$ 4 on a part replicated in a series. **No plan B — acceptance is a gate**, and what it blocks is the order |
| [docs/operacao/VOO-ESP32-PONTE.md](docs/operacao/VOO-ESP32-PONTE.md) | **v0.1.0** | The flight of the BLE↔UART bridge. **The Daisy firmware does not change a line** — the automatic `sync` is flight 2. It brings the three checks done before writing |
| [docs/operacao/gerar_projeto_loopy.py](docs/operacao/gerar_projeto_loopy.py) | **v0.2.0** | **Writes a COMPLETE `.lpproj`** from `manifest.h` — 9 pages, 143 widgets, colors per channel. It starts from a template and preserves the loop scene and the whole bundle: a project with no `Clock` or `Sequence` will not even open. **It never writes over the template** |
| [docs/operacao/gerar_perfil_loopy.py](docs/operacao/gerar_perfil_loopy.py) | **v0.5.0** | Reads the project exported from Loopy (`.lpproj`, a zip with SQLite) and **emits the bindings** already addressed, with the CCs coming from `manifest.h`. ⚠️ The address is positional — regenerate after touching the layout |
| [docs/operacao/gerar_secao_loopy.py](docs/operacao/gerar_secao_loopy.py) | **v0.3.0** | **Owner of `LOOPY-SECAO.md`** — the build sheet for the new section, with CC, channel, widget type, range and birth value, plus the presets page. Generated because in a single session **six CCs** changed: a sheet by hand drifts on the first change, and whoever follows it builds yesterday's instrument |
| [docs/operacao/LOOPY-SECAO.md](docs/operacao/LOOPY-SECAO.md) | **generated** | 105 bindings, 72 faders and 33 buttons, in learning order — it is not a layout: Loopy addresses by array index, which only exists once the widgets are created |
| [docs/operacao/CCS-POR-PAGINA.md](docs/operacao/CCS-POR-PAGINA.md) | **generated** | The CCs in the order of the Loopy pages: central / mod a / mod b / time a / time b / random / tech |
| [docs/operacao/BINDINGS-A-MAO.md](docs/operacao/BINDINGS-A-MAO.md) | **generated** | Plan B: a manual entry sheet in Control Settings, with CC and channel per line |
| [docs/operacao/ACEITE-MULTIPLEXACAO.md](docs/operacao/ACEITE-MULTIPLEXACAO.md) | **live** | MIDI acceptance script — the firmware side is green |
| [src/presets.h](src/presets.h) | **v0.11.0** | Preset structures and the **derived map version** (FNV-1a of the names; today **`0xBE1B`**). Pure C++ — proven whole on a native bench. Membership in a preset is DERIVED from the name, and `route_*` is on the short exceptions list. Ceiling per preset **56** |
| [src/test_presets.cpp](src/test_presets.cpp) | **v0.7.0** | Native bench for the presets: membership, hash (both halves) and every refusal verdict |
| [src/qspi_raw.h](src/qspi_raw.h) | **v0.2.0** | Contract for raw access to the QUADSPI. **Zero transitive closure** — nothing it calls may live in QSPI |
| [src/qspi_raw.cpp](src/qspi_raw.cpp) | **v0.2.0** | QUADSPI registers by hand, all marked `COTUCA_ITCM`. **ABORT when leaving memory-mapped** — see `docs/DESENHO-ESCRITA-QSPI.md` |
| [src/cotucatronics_qspi.lds](src/cotucatronics_qspi.lds) | **v0.1.0** | Linker script forked from libDaisy (certificate of origin at the top). Adds `.itcm_text` → ITCMRAM |
| [src/teste_minimo/](src/teste_minimo/) | **v0.4.0** | **On-board proving bench.** Thirty lines that halve the problem in a single flash: if it speaks, the chain is good and the defect is the app's. It has already ruled out the board, the flash, USB, audio init and 64 MB of SDRAM — and it prints what `CheckBoardVersion()` detected |
| [src/grava_placa_nova.command](src/grava_placa_nova.command) | **v0.2.0** | Flashes a **new** Daisy, whether factory-fresh or already with a bootloader — `flash.sh` only serves once there is an app running. It waits for the DFU and **decides the target from the memory map**, not from the PID: `0x90000000` is the QSPI (bootloader present → `program-dfu`), `0x08000000` is the ST ROM (→ `program-boot`). ⚠️ The two DFUs enumerate as `0483:df11` — `STM_PID` and `DAISY_PID` are the **same value**, and discriminating by PID would write the bootloader over a board that only wanted the app. **The `program-boot` branch has never been exercised** |
| [src/flash.sh](src/flash.sh) | **v0.4.0** | Flashes **only if the build passed**, checks the version **in the source and on the board**, and **restores the state that the flash erased**. Enters the bootloader via `dfu go` and waits for the DFU to appear, instead of asking for a RESET |
| [src/estado.py](src/estado.py) | **v0.2.0** | Saves the params before the DFU and restores them after. ⚠️ It speaks in **`real`**, which is what `set` accepts — v0.1.0 sent `norm`, was refused with `fora da faixa`, and **printed "repos" anyway** because I threw away the response. Read the response, and shout what it refuses |
| [src/porta_daisy.py](src/porta_daisy.py) | **v0.1.0** | Finds the Daisy **by asking**, never by position in the `glob`. It was born when the ESP32 came onto the USB and there started being two `usbmodem`s — choosing by index became luck |
| [docs/operacao/auditar.sh](docs/operacao/auditar.sh) · [.command](docs/operacao/auditar.command) | **v1.3.0** | **The mechanical sweep of `/audit`** — dead links, orphan exemptions, orphan and duplicate retirees, out-of-sync destinations (repo/vault), `check_docs` and the version map. **It only reports what it proves**: the judgment classes are in the skill, not here. On its debut it found 5 dead links in `MAPA-DE-TERRA.md` and a duplicate entry |
| [docs/operacao/hook_stop.sh](docs/operacao/hook_stop.sh) · [.claude/settings.json](.claude/settings.json) | **v1.0.0** | **`/audit` runs on its own at the end of every session, and WARNS.** `battle_scars` already had the rule — *"an automated habit beats a remembered habit"* — and every checker depended on someone remembering. **It never blocks:** it always exits 0, because a gate that gets in the way is a gate that gets switched off. ⚠️ The count is **per section** — the pending-items pass lists 🔴/🟡 that live inside the documents, and counting them would give a permanent alarm |
| [docs/operacao/hook_commit.sh](docs/operacao/hook_commit.sh) | **v1.0.0** | **The CHEAP checkers run before the commit** — `PreToolUse` on `git commit`, **70 ms**. `hook_stop` already caught these things, but at the close of the session, with the bad already pushed: **the gain is not avoiding the error, it is having it arrive while there is still context to fix it.** It runs only the version map (0.03 s) and the freshness of the drawings (0.02 s); `check_docs` (1.5 s) and `auditar` (3.3 s) stay at the close — an expensive check on every commit is a check that ends up getting removed. It warns, never blocks. On its debut it caught an old drawing that no one knew was old |
| [docs/operacao/derivados.py](docs/operacao/derivados.py) | **v1.1.0** | **Turns "generated" into a falsifiable claim.** `manifest.h` was already the single source and the CC map gave different numbers in different documents — the cause was the label: three sheets called themselves generated from it, no one kept them by hand *because of that*, and **no generator wrote them**. It checks who calls itself derived, from which version of the source, whether it matches today's, and **whether a generator exists that writes it**. The stamp that serves names the SOURCE (`manifest v1.13.0`), not the generator |
| [docs/operacao/vizinhos.py](docs/operacao/vizinhos.py) | **v1.0.0** | Hunts the **sentence broken by a rename** — the case where the anchor matched, `grep` confirmed, `make` passed, and the sentence lost its meaning because the continuation was on the next line. It only flags when the edit touched the **end** of the line; `--termo=<nome>` tightens to where the removed tail carried the name (211 → 89 → 16 neighborhoods) |
| [docs/operacao/coerencia.py](docs/operacao/coerencia.py) | **v1.1.0** | The checks `grep` does not do properly — exemptions that cover nothing (by `check_docs`'s criterion: **between backticks**, file scope), the health of the retirees list, and the **pending-items pass**. It was born fixing two bash checks that accused by the wrong criterion |
| [docs/operacao/versoes_readme.py](docs/operacao/versoes_readme.py) | **v1.1.1** | Checks the file map above against the files — `CLAUDE.md` demands the current version on every line and nothing enforced it. `--write` corrects it. **On its debut: 15 of 58 had drifted**, one of them with the README AHEAD of the source (`ponte.ino`). It reports what it cannot read, instead of skipping silently |
| [src/check_docs.py](src/check_docs.py) | **v0.14.0** | `make check-docs`: bears witness for the doc against the manifest: ghosts, commands with no help, pin map, ranges, and **two versions in the same file that disagree**. It aborts if `AddMidi` does not match even once. **It prints its own version on the first line**. The two-versions rule also covers `.py`, where there is no `#define`: it cross-checks the docstring, the constant and **what the script prints** |
| [docs/params-aposentados.txt](docs/params-aposentados.txt) | **live** | Retired params, removed symbols and external names — what the checker has license not to find |
| [docs/DESENHO-ESCRITA-QSPI.md](docs/DESENHO-ESCRITA-QSPI.md) | **live** | The presets route: ITCM window (**proven**), the real protocol read from the source, and **where the front stopped, with the resumption condition** |
| [docs/FUTURO.md](docs/FUTURO.md) | **live** | **Possibilities, with the cost already worked out** — it is not a to-do list: it is what was considered and why it is not now. DPDT sense, a color screen, the encoders that went out, and the **budget of ten free pins** from which everything comes. It exists because the alternative is rediscovery: three times in one week a question was reopened that already had a measured answer, kept only in a conversation |
| [docs/PAUTA-ESCUTA.md](docs/PAUTA-ESCUTA.md) | **live** | What to listen to, in order — ~20 versions with no ear verdict |
| [src/Makefile](src/Makefile) | **v0.2.0** | Build per board. `APP_TYPE=BOOT_QSPI`. No DaisySP |
| [.claude/skills/](.claude/skills/) | **live** | **The executable arm of IFÁ10000** — **fifteen** skills: **`aipcb`**, `ifa10000`, `medicao-limpa`, `instrumentacao`, `bug-de-ordem`, `edicao-segura`, `bancada-firmware`, `revisao-terra`, `kicad_layout`, `loopy`, `design-review`, `audit`, `caixa`, `pm`, plus the tombstone of `kicad`. ⚠️ **`kicad` was RETIRED on 09/09/2026** and replaced by `aipcb` — its 1033 lines are in [docs/rev_b/kicad-skill-aposentada.md](docs/rev_b/kicad-skill-aposentada.md), and in `.claude/skills/kicad/` only the tombstone remains. The underlying difference: `kicad` described generators that wrote KiCad files by hand; `aipcb` describes a flow in which **no one** writes KiCad files by hand. ⚠️ **`kicad_layout` was left with no one to call it** — it said it was "called by the `kicad` skill", and `aipcb` does not mention it. Decision to be made: it goes into `aipcb`, it is retired too, or it stays |
| [docs/desenhos/](docs/desenhos/) · [tools/desenhos.py](tools/desenhos.py) | **v0.4.0** | **The folder of technical drawings, and the tool that keeps it honest.** The schematics come from `kicad-cli sch export svg` — the same source the factory reads, so they **cannot diverge**. `desenhos.py` compares dates: **if the source is newer than the drawing, the drawing is old**, and it exits with code ≠ 0. No opinion in the criterion. ⚠️ These do not pass `svg_colisoes` (KiCad writes the letters as *paths*) — *"it may diverge but it is checked"* was traded for *"it cannot diverge but it is not checked"*, and that is declared. `preamp-piezo.svg` died on 05/09 for saying `C2 1 nF` and `RV1` alive when the schematic already said `20n` and `DNP` — **an outdated figure with an official look is worse than none** |
| [docs/operacao/PINOS.md](docs/operacao/PINOS.md) | **live** | **The pin map, and the source is the source.** Checked at every `bts`. On 04/09 it was caught saying `Livres: D0–D12` while listing the OLED as occupied three sections below — the truly free ones are six |
| [docs/operacao/SUPRESSOR-MICROFONIA.md](docs/operacao/SUPRESSOR-MICROFONIA.md) | **live** | The contract of **SuMiA™**, measured before the code, plus what it has already done in service |
| [docs/operacao/ATERRAMENTO.md](docs/operacao/ATERRAMENTO.md) · [TERRA-REFAZER.md](docs/operacao/TERRA-REFAZER.md) | **live** | The ground sweep and the bench sheet with which the grounding was redone on 03/09 |
| [docs/operacao/ARQUITETURA.md](docs/operacao/ARQUITETURA.md) | **live** | The signal drawing, end to end |
| [docs/operacao/PRESETS.md](docs/operacao/PRESETS.md) · [DESENHO-PRESETS-RAM.md](docs/operacao/DESENHO-PRESETS-RAM.md) · [PRESETS-LISTA.md](docs/operacao/PRESETS-LISTA.md) | **live** | Presets: the contract, the RAM design, and the list |
| [docs/operacao/REMAP-ESCUTA.md](docs/operacao/REMAP-ESCUTA.md) · [ROTINAS.md](docs/operacao/ROTINAS.md) | **live** | The listen remapping and the bench routines |
| [docs/operacao/gerar_ccmap.py](docs/operacao/gerar_ccmap.py) | **v0.11.0** | **The CC-map generator** — writes `CCMAP.md` *and* the `ccmap.html` card, and **stamps date, commit and dirty tree** (dB's rule, 04/09). ⚠️ Do not write another: on 04/09 I wrote a second one without looking for this one, and `CCMAP.md` itself said on the 3rd line which was the right one |
| [docs/operacao/esquematicos/](docs/operacao/esquematicos/) | **live** | The carrier board in KiCad — `brief` and `bom` of revA, `brief` and `ERC-ISENCOES` of revB. Drawing rules in the `kicad` skill |
| [caixa/cotuca_caixa_corte.py](caixa/cotuca_caixa_corte.py) | **v0.7.0** | **The source of the enclosure.** Generates the cut from `--t` and `--kerf` — changing material is regenerating, because the tooth depth IS the thickness. The filename carries the thickness. Three gates of its own: nesting, **mirror** (a rotated part that was actually mirrored cuts fine and assembles backwards) and margin to the sheet edge. A three-shelf nesting computed from the real bboxes — at T=3 the parts grow and the old column left 1.4 mm of margin |
| [caixa/](caixa/) · [tools/nesting.py](tools/nesting.py) | **v0.1.0** | The rest of the physical artifact, and the external gate: everything inside the sheet, nothing overlapping, and **the number of parts** — an absence does not collide. ⚠️ On 06/09 the 6 fabrication pages, their generators and the walnut cut died: they drew, or were, **another** enclosure, and a superseded source beside the right one measures just as well |
| [docs/COTUCA_BOM.md](docs/COTUCA_BOM.md) · [tools/bom_completa.py](tools/bom_completa.py) | **v0.10.2** | **The BOM of the whole instrument, GENERATED from the KiCad sheets** — the two boards, the modules (Daisy, ESP32-C3, amp, step-down, decoy), the panel, the cabling and the enclosure. References, values, footprints and DNP come from the schematic; MPN and price live in the generator, and **each price declares whether it was measured or estimated**. It stamps date, commit and dirty tree. `--conferir` exits ≠ 0 if it is out of date. ⚠️ **The MPN is a field of its own (`MPN`/`MPN_PAINEL`/`MPN_POR_FP`), separate from the description column** — and empty where the part is genuinely generic: a 1% 0805 resistor has no part number, it has a value and a package. And each suffix has, in `SUFIXO`, the datasheet line that justifies it **and the one that does not justify it** |
| [docs/operacao/esquematicos/revB/BOM-MPN-revB.md](docs/operacao/esquematicos/revB/BOM-MPN-revB.md) | **live** | **The BOM by MPN** — part by part, with manufacturer, package and tier. JLC assembles only the SMD; the rest dB solders. §8 records what the footprint assignment on 05/09 disproved: the `PTV09A-4` pot is bushingless and is no good for a panel-mounted shaft, the `PEC11R` encoder is not the Alps `EC11E` (pins at 5.0 mm vs 2.5), and the panel's `R5`/`R6` diverge in value between the BOM and the schematic |
| [docs/ROTEIRO-BOOT-SRAM.md](docs/ROTEIRO-BOOT-SRAM.md) · [AUDIT-23-08.md](docs/AUDIT-23-08.md) | **historical** | Closed records: the SRAM boot roadmap and the 23/08 sweep. Kept for what they explain, not as open items |
| [docs/FEATURES.md](docs/FEATURES.md) | **live** | **THE SINGLE BOX** — every feature in force, and the retired ones in the archive |
| [docs/CTOS.md](docs/CTOS.md) | **v0.4.0** | The fail-safe base, the CTOS/Effect boundary, the suite, and the **sum ceiling** (§3b) |
| [docs/PROCEDIMENTO-MIDI-LEARN.md](docs/PROCEDIMENTO-MIDI-LEARN.md) | **new** | Addressing the Loopy pages via `learn` mode. Includes the three "do nots" that cost a round |
| [docs/IFA10000.md](docs/IFA10000.md) | **live** | **Effect certification** — the 10 steps every effect passes |
| [docs/STATUS.md](docs/STATUS.md) | **live** | **Where we are** — board state, open defects, next steps |
| [docs/operacao/PRESETS-LISTA.md](docs/operacao/PRESETS-LISTA.md) | **v0.9.0** | **The preset list and how the buttons address it** — CC 121 loads / 122 writes, the channel says the destination. It lives empty until the listening session; it carries the hash of the map in force (`0xBE1B`) |
| [docs/battle_scars.md](docs/battle_scars.md) | **live** | **Lessons and methodology** — the scars, so as not to pay twice |
| [src/TESTES.md](src/TESTES.md) | **v0.25.0** | Script T1–T12, flashing flow and known defects |
| [src/BOARDS.md](src/BOARDS.md) | **v0.14.0** | Supported boards, the SDRAM rule and the bootloader note |
| [src/cotuca_serial.py](src/cotuca_serial.py) | **v0.4.0** | Serial monitor, no dependencies |
| [src/monitor.command](src/monitor.command) | **v0.1.0** | Double-clickable wrapper for the monitor |
| [src/cotuca_measure.py](src/cotuca_measure.py) | **v0.1.0** | Measures the output frequency by FFT |
| [src/teste_roteamento.py](src/teste_roteamento.py) | **v0.1.0** | **Automated T7c** — proves the two stages of routing with the test tone. It asks for the port, finds the interface by name, **measures** the channel map and aborts if the two outputs fall into the same one (a criterion that cannot fail is not a criterion) |
| [src/measure.command](src/measure.command) | **v0.1.0** | Double-clickable wrapper for the meter |
| [tools/manual_montagem.py](tools/manual_montagem.py) | **v0.16.0** | **The assembly manual, IKEA-style — generated, not written.** It reads the boards fabricated by `pcbnew` (outline, body of each part, pad 1, net of each pad, side, DNP), the package that went to the factory (`cpl_rico_*.csv`) and the field lessons, and the Mouser order that arrived (`export/mouser_40440548.csv`, bag by bag), and writes `docs/montagem/manual_montagem.html`: 49 steps, a table that translates each Mouser label code into the value to write on the bag, a map per step with the parts highlighted, and the ones that go **underneath** in mirrored view. Pin 1 is marked only when the pad shape distinguishes it (the diodes and the SOIC get an explicit mark). It runs with KiCad's Python |
| [tools/svg_colisoes.py](tools/svg_colisoes.py) | **v0.6.0** | **The exit gate of every SVG** — it measures text boxes and reports overlap. Four exit codes, and **3 = NOT VERIFIABLE** (zero texts read): a tool that cannot measure has to say so, not return "clean". **Since v0.3.0 it uses the `textLength` that KiCad writes** — a width measured by KiCad itself, not estimated. Until then it discarded every `<text>` with `opacity="0"`, which is how KiCad marks its own, and so it answered "not verifiable" to schematics with 336 measured texts inside them. ⚠️ On KiCad schematics it is **noisy** (each text comes out duplicated, and every label touches its wire): for those the gate is `colisoes_sch.py` |
| [tools/desenhos.py](tools/desenhos.py) | **v0.4.0** | Keeps the freshness of the **11** technical drawings: **a source newer than the drawing = an old drawing**, and it exits ≠ 0. ⚠️ It gauges consistency, not correctness: with the generator breaking, the sheet does not change, and the drawing becomes "up to date" without the fix inside it |
| [tools/colisoes_sch.py](tools/colisoes_sch.py) | **v0.5.0** | **The gate of the schematics**, where `svg_colisoes` does not serve. It reads the `.kicad_sch`, builds the box of each text and of each shape (wires and symbol bodies) and crosses them. The letter advance (0.85 of the body) was **measured** in KiCad's own `textLength`, not arbitrated. **It counts the texts that exist against the ones it read and refuses to approve an incomplete read** — v0.3.0 caught 0 of the 40 properties and announced "0 collisions" |
| [tools/contrato_idc.py](tools/contrato_idc.py) | **v0.2.1** | **The only contract between the two boards** — it reads the IDC's 26 ways in both projects by geometry and checks one against the other. **An incomplete read exits with code 3**: on 05/09 v0.1.0 read 24 ways and announced "all 26 match", which is exactly the defect it exists to catch |
| [kicad/cotuca_cerebro/](kicad/cotuca_cerebro/) | **rev B** | The schematic of the mother board: hierarchical root + 4 sheets (power, preamp, socket, connector). ERC **11** violations, all of the same type and all declared: reserve pins from `FUTURO.md`. `D29`/`D30` stopped counting — they carry `no_connect`, because they are the USB and are not a reserve, they are a prohibition |
| [kicad/cotuca_painel/](kicad/cotuca_painel/) | **rev A** | The panel schematic: 1 encoder, 2 buttons, 2 pots, OLED, LED. **ERC 0 violations.** It uses the *same* library as the brain board via `sym-lib-table` — one source for both boards |
| [kicad/cotuca_cerebro/cotuca.kicad_sym](kicad/cotuca_cerebro/cotuca.kicad_sym) | **34 symbols** | The library shared by both projects. The `OPA1642` pinout taken from the `LM2904` in KiCad's official library — ⚠️ **to be confirmed in the OPA1642AIDR datasheet before submitting** |
| [kicad/cotuca_cerebro/gera_simbolos.py](kicad/cotuca_cerebro/gera_simbolos.py) | **v0.12.0** | Generates the library symbols. **Every number goes through `num()`**, which rounds to 4 places: without it `h/2 − 1.27 − 6×2.54` comes out `1.7763568394002505e-15` and no reader with `[\d.]+` catches the line. It brings the **three-unit `OPA1642`** (05/09) — it replaced `OPA`+`OPAPWR`, which gave three BOM lines for one chip and had **pins 1 and 3 swapped** relative to the real SOIC-8 |
| [kicad/cotuca_cerebro/gera_daisy_sym.py](kicad/cotuca_cerebro/gera_daisy_sym.py) | **v0.3.0** | Generates the Daisy symbol from the manufacturer's CSV — since 05/09 the **Seed3** one, versioned in `kicad/cotuca_cerebro/`. It reads the columns **by header name**, and so it breaks loudly when the source changes shape instead of emitting a wrong symbol. It refuses names that are MCU port designations and says which it discarded — the Seed3 CSV puts `PG3` on pin 40, which the manufacturer's own drawing calls `DGND` |
| [kicad/cotuca_cerebro/gera_soquete.py](kicad/cotuca_cerebro/gera_soquete.py) | **v0.19.0** | Generates the socket sheet. `pinos_de()` reads the geometry from the **library**, never from memory — and **counts the pins it read against the `(pin` blocks that exist** |
| [kicad/pcb_contorno.py](kicad/pcb_contorno.py) | **v0.24.0** | **Mechanical** outline and placement of the two boards. It **imports** `caixa/cotuca_caixa_corte.py` — no parsing between the dimension and the use — and the board **centers itself on the bottom sheet**, which makes it **immune to the enclosure's thickness**: confirmed by running the generator at `T=3` and `T=6`, the mounting holes always fall at (15.5 · 128.5) × (3.5 · 96.5) of the board and the axes at 105/140/175 × 92 of the face. The thickness has changed four times already. Two gates that refuse to write: one if the board does not fit in the enclosure, another if one of the 4 bottom mounting holes ends up under a component. **The copper planes come out only on B.Cu and each SMD ground pad gets a via with a stub up to it** (`costura()`): copper on both layers leaves no layer for signal, and a two-sided board only has two — measured, 19 vias on the brain board and 8 on the panel, and `via_dangling` only drops to 1 when the stub exists, because adjacent copper is not connected copper. The `y` inversion lives only in `da_tampa()`, and the axis of each pot/encoder is **read from the footprint's `F.Fab` circle** — the origin of a footprint is not the spindle |
| [kicad/sincroniza_pcb.py](kicad/sincroniza_pcb.py) | **v0.5.0** | Does what `Update PCB from Schematic` (F8) does, and **without a GUI**: it removes extra copies, adds what is missing, removes what is left over, and puts the net on each pad from the netlist. It exists because in this project the schematics are **generated** — each regeneration changes the UUIDs, F8 stops recognizing what is there and **adds a second copy**. `--zero` empties and restores everything. It refuses to save if the board and the schematic do not end up with the same references |
| [kicad/arruma_silk.py](kicad/arruma_silk.py) | **v0.21.0** | Moves the reference fields off the top of the pads. **Silkscreen only** — it does not touch copper, traces, zones or positions, so a routed board passes through here without losing anything. It is proven by the number that does NOT change: `silk_over_copper` 39 -> 4 and connections to route 104 -> 104. If no position serves, it **leaves it as it is and says so** — pushing it to a bad spot is just moving the same problem somewhere else |
| [kicad/roteia.py](kicad/roteia.py) | **v1.18.0** | Routes **both layers**, with a via, and proves it broke nothing. A single signal layer is a planar graph and the connections are not: the one-layer version stopped at 24 of 71, and with the second it went from **122 to 23**. The via costs 30 cells, so that only those with no other way skip. **Acceptance rule: no net may get worse** — the profile of connections left to do is measured before and after, because the damage to the plane takes two forms (the zone split into islands, and the pad cut off from the copper) and a list of symptoms only protects against what has already happened. When some net gets worse, **bisection** to find the culprit crossing instead of throwing away the good ones: it isolated 2 of 33. It exempts the baseline by the (type, net) pair, never by the net name. **It tears out the copper that gets in the way, passes, and remakes what it tore out** — the only thing that attacks the last connections, which no longer have a free path — and **it backs off if the number does not drop**, because tearing out a finished net to try another may be a loss. The acceptance rule counts by **(net, kind)**: counting only by net let a pad that connected mask a plane that split, and `roteia.py --auto-teste` interrogates the rule with that case. The retraction **filters the candidates by geometry before spending DRCs on them** — only a crossing that passes within clearance+copper+clearance of a plane pad can orphan it — and that cut from 27 candidates to 5, with the same culprits and the pass dropping from ~13 minutes to 55 seconds. And **one tight pass at the end** (clearance 0.22, mesh 0.085, search pinned to a 25 mm box): with a wide clearance the router **cannot get out from between a connector's pins** — on a 2.54 header with 1.70 pads the gap is 0.840 mm and it needs 0.950 — which explained 9 failures on `J14`, 8 on `J1` and 11 on the Daisy, all in spots with zero traces around. Measured: tightening the clearance from 0.35 to 0.26 **from the start** worsened it from 23 to 34 — a greedy router improves with less appetite, not with less clearance |
| [kicad/le_drc.py](kicad/le_drc.py) | **v0.2.0** | Reads the DRC report **by net**, and separates `Zone ↔ Zone` (plane cut) from `starved_thermal` (pad clinging to a narrow island). It exists because **the total lies by omission**: 71 → 52 with the AGND island split into eight is an improvement in the total and a regression on the board. The net name **is not on the finding's line** — it comes in the next two item lines, and an extractor that matches the header returns zero with the look of a clean board |
| [kicad/faz_placa.py](kicad/faz_placa.py) | **v0.17.0** | **From the schematic to the factory package, in one command.** Each step already existed; what was missing was the **order** — stitching after placement, routing after stitching, silkscreen after routing, gate after everything — and a step out of place does not give an error, it gives a board that passes every test and is wrong. It starts by **erasing all the copper**, because when the placement can change, the copper that touched the old parts is garbage. And it **runs the three attack orders on copies and keeps the best, measuring**: there is no winning order, there is a winner on this board (brain board `curta` 19 · `drc` 23 · `longa` 29; panel `drc` 10 · `longa` 11 · `curta` 13). One less decision for someone to make |
| [kicad/roteia_fr.py](kicad/roteia_fr.py) | **v0.13.0** | **FreeRouting, which is a real router.** It exports Specctra DSN via `pcbnew`, runs FreeRouting, imports the session back. On the same board and from scratch: my router gave **12 left to connect in ~9 min**, this one gives **8 in 35 s** — and the ones it has left are all connections to the plane, not signal: **it closes every signal**. It handles the four things it does not know: it puts a *keepout* over the net-ties (otherwise it routes over the ground bridge and short-circuits it), runs with `-mt 1` because the tool itself warns that multithreading generates violations, **erases the copper first** — including the stitching, which with it goes from necessary to an obstacle — and at the end connects by via+stub the ground pads the fill did not reach, **removing the vias that end up loose**. ⚠️ It needs Java and `freerouting.jar`; without them the chain falls back to the in-house router |
| [kicad/classes_de_rede.py](kicad/classes_de_rede.py) | **v0.1.0** | Puts the track widths in the **board's net classes**, and not in a table inside a script. While the router was mine the table worked and hid that the **DRC did not know about it**; when an outside router came in, it drew the input track at signal width. It assigns by **name pattern**, so that a new net enters the right class on its own. And it lowers the minimum width to 0.15 mm — JLC does 0.127, and it was my rule that forbade the router from leaving a fine-pitch pad |
| [kicad/revisao_externa.py](kicad/revisao_externa.py) | **v0.2.0** | **The review I did not write.** It runs [kicad-happy](https://github.com/aklofas/kicad-happy) (MIT, 1100★, tested against 5800 projects) over the two boards. It exists because everything else in this chain is mine — generator, router, gate and the checkers themselves — and **a self-test has the blind spots of whoever wrote it**, which has already happened here. On the first run it caught what we never checked: **zero fiducials with 35 SMD components** (automatic assembly needs three per side), **`J15` and `J16` without a single ground pin** carrying audio off the board, and 0/69 nets with a test point. It silences false positives **by (code, net) pair and with the reason written and dated** — never a whole family, and never in silence |
| [tools/fichas.py](tools/fichas.py) · [docs/datasheets/](docs/datasheets/) | **v0.10.1** | **The datasheets of the parts with a real MPN**, downloaded and *checked inside*. It exists because `kicad-happy`'s downloader failed on all 19 and the cause was not the list: the `jlcsearch` API stopped serving the `datasheet` field — it **finds** the part and returns the field empty. The chain that works is two steps (`jlcsearch` → LCSC code → `wmsc.lcsc` → PDF), and it was not fixed in the plugin's cache because the next update would wipe it. Each PDF goes through `pdftotext` — an outside tool, which can say no — and the number has to appear in the text. Five rungs: **OK** · **FAMILY** (series datasheet, suffix with a mandatory citation) · **IN CATALOG** (no datasheet of its own, but the number is in a series catalog already on disk) · **SUSPECT** (the file does not mention the part — it blocks the gate) · **NO DATASHEET**, this one with the manufacturer's address. State: **10 OK · 2 family · 1 in catalog · 8 no datasheet** of 21. Double-click wrapper in `tools/fichas.command` |
| [kicad/mpn_nas_folhas.py](kicad/mpn_nas_folhas.py) | **v0.3.1** | Puts the **MPN on the schematic sheets**, stamped from the single source. It exists because the part numbers lived in a table inside the BOM generator, and the schematic did not know about them: an outside review read the sheets and said **`0/36 parts have MPNs — not pre-fab ready`**, with reason. ⚠️ Two sheets have no generator, and the house rule forbids editing `.kicad_sch` by hand — so this **stamps and reruns** after any regeneration, instead of editing. And the factory gate runs `--conferir`: a copy without a checker diverges again. Result: **36 of 84 (43%)** — and this number went down on purpose. v0.2.0 stamped the BOM's **description column**, and the outside review said *"50/50 parts have MPNs"* because it only checks that the field is not empty: the 99% were phrases like `header 1x4 2,54` passing for part numbers. It was only seen when a tool tried to **use** the field and went looking for it at LCSC. The 48 without an MPN are resistors by package, plain headers and modules bought pre-assembled — **empty is an answer, not a gap** |
| [kicad/compara_netlist.py](kicad/compara_netlist.py) | **v0.1.0** | The before netlist against the after one, **by membership** — the set of `(reference, pin)` of each net, and never the names, which change on purpose. It is the acceptance criterion that the `/kicad` skill §9 demands after each change to a generator. **It counts what it read**: I wrote a by-eye reader that returned "0 nets" on a file with 74, and a reader that loses lines silently fails nothing — it approves everything. The guard was proven by blinding it to a block that is still in the file |
| [kicad/lista_roteamento.py](kicad/lista_roteamento.py) | **v0.3.1** | What is left to route, **read from the board and not from memory**, ordered by blocks with references, coordinates, widths and lengths. It exists because there was a hand-written list that said 71 connections when the board had 12: the placement changed three times and the list did not know. A wrong work list is not an outdated document — it is someone routing to places where the parts no longer are. **It counts what it read and declares what it left out**: the pad↔zone connections are not traces, they are pads that lost the plane, and they come out in a section of their own instead of disappearing from the count |
| [tools/codigos_lcsc.py](tools/codigos_lcsc.py) | **v0.2.0** | **The LCSC code of each SMD line** — the MPN is for humans, and the factory does not assemble a line without a `Cxxxxx`. It reads the SMD lines from the footprint's **attribute** on the board (not from the library name) and resolves each one: a part with an MPN by exact match, a generic passive by a **verified** search. ⚠️ The distributor's search returns a **LED** first for `4.7MΩ 0805`; so the order decides nothing — fifty are requested and the description is required to match on value, package, class, tolerance (1%) and dielectric when the swap is audible. **26 of 26**, each with the description that justifies it saved in `codigos_lcsc.json` |
| [kicad/dnp_para_pcb.py](kicad/dnp_para_pcb.py) | **v0.1.0** | Carries the **DNP from the schematic to the board**. The `(dnp yes)` lives in the symbols and `pcb export pos --exclude-dnp` reads the *footprint's* attribute — the option was written and did nothing, and the CPL ordered the placement of **eight parts the BOM did not buy**. It uses the `pcbnew` API to serialize the mark as this version reads it |
| [kicad/rotula_tp.py](kicad/rotula_tp.py) | **v0.2.0** | Puts the **net name** beside each test point, on the silkscreen. The board said `TP1`…`TP11` and nothing more — the names lived in the `Value`, which is on F.Fab and **does not print**. ⚠️ The name comes from the **pad's** `(net ...)`, never from the `Value`: the `Value` is an assertion about the net and the pad **is** the net, and a wrong label in ink on the physical part cannot be corrected. The point's reference moves to Fab — on the bench `TP7` says nothing and `+3V3A` says everything. Done on the **panel** (5/5, clean gate). On the **brain board** eleven scattered labels do not fit — 4 mm pitch vs 6 mm — and `--legenda` comes in: **one block** of 12 lines 14 mm from the row, with `texto×texto 0→0` and **zero collisions added**. ⚠️ There the reference leaves the silkscreen and here it **stays**: when the name is beside it, `TP7` is noise; when it is somewhere else, it is the index |
| [kicad/refs_apontam.py](kicad/refs_apontam.py) | **v0.1.0** | **Does each reference point to its own part?** — the question no DRC asks, because it is not forbidden geometry, it is **meaning**. A reference 11.5 mm from its part and 5 mm from the neighbor passes every checker and identifies the wrong part; whoever solders by hand reads the nearest designator. There were **8**. §11g-ter of the `kicad_layout` skill had already foreseen it in writing without anything measuring it — diagnosed and awaiting instrumentation. `para_fabrica` refuses the package |
| [kicad/funcao_dos_pads.py](kicad/funcao_dos_pads.py) | **v0.3.1** | **Does the pad do what the datasheet says it does?** The ERC, the DRC and "0 left to route" check CONNECTION; none checks MEANING. A generic symbol with pins `1,2,3` matched to a footprint whose pads have a fixed function gives a correct netlist and a wrong function — that is how the OPA1642 inputs came out swapped on both channels, the AO3400A drain and source reversed, and the SS14 cathode flipped. The datasheet is indexed by **REFERENCE and not by footprint**: the same SOT-23 is the AO3400A (1G 2S 3D) and the BAV99, and a map by footprint would say the same thing of both. The contract never uses auto-generated net names (`Net-(U1A--)` changes when the defect is fixed). It is proven **by sabotaging a pad**: if it does not fail, it is worthless |
| [kicad/dentro_do_contorno.py](kicad/dentro_do_contorno.py) | **v0.4.0** | **Does the courtyard fit on the board?** Sister of `confere_furos`: outlines, not pads. KiCad **does not check courtyard against `Edge.Cuts`** and so the DRC said "geometry 0" with the panel's `J1` 2.83 mm outside the outline — a connector with the body out has all its pads inside, what sticks out is the plastic. It measures each courtyard vertex against the edge and each edge vertex against the courtyard, because a cutout can enter between two vertices without getting near either. **A footprint with no courtyard is a finding, not a pass.** Exemptions one by one and with a number in the reason (the four M3s pass at 0.06 in the drawing and the real washer spares 0.25). ⚠️ v0.1.0 failed 74 of 74 and 28 of 28 at `0.000` because it used `SHAPE_LINE_CHAIN.Distance()`, which on a closed chain measures the REGION and returns zero for every interior point — **identical zeros on top of everything are the reader, not the board** |
| [kicad/encaixe.py](kicad/encaixe.py) | **v0.7.0** | **Does the board assemble?** The gate of IFÁ10000's track M, **read only**, called by `para_fabrica`. It measures M2 (≥3 non-collinear mounting points — holes, NPTH, or parts whose datasheet proves the thread), M4 (courtyard on the edge), M6 (bodies under the arcade buttons, by the body and not by the pads) and the **cables between boards** (3D path along the back wall + the enclosure's service clearance, against the BOM). It does not redo calculations: it calls `pcb_contorno` and `dentro_do_contorno`. `--sabota` proves each check. It is born of *"what good is a board that is electrically correct if we cannot assemble it?"* (dB, 18/09) |
| [kicad/poe_furos.py](kicad/poe_furos.py) | **v0.2.0** | The four M3 holes of the brain board, with the coordinates **derived** from the enclosure's standoffs instead of written, and a radius-3.5 keepout on both layers around each one. Idempotent via `uuid5`, and it confirms that the result opens in KiCad before writing over it |
| [kicad/para_fabrica.py](kicad/para_fabrica.py) | **v0.20.0** | The JLCPCB package, and the **gate that refuses to make a bad one**. An order is irreversible and takes weeks: the error is not discovered on the screen, it is discovered when the box arrives. It measures geometry, connections left to do **by net**, planes in pieces and orphan islands; it refuses if any fails; and only then does it export gerbers, drilling, CPL and BOM, stamped with generated date, commit and dirty-tree warning. `--incompleta` produces it anyway and writes **INCOMPLETA in the name**, so that a bad package is never confused with a good one. It also says what it **cannot** see: that the netlist is correct, and the assembly rotation — the CPL angle is the footprint's and the factory expects its library's. It also runs `fichas.py --confere`, which does not go to the network and refuses the package if any datasheet on disk does not mention the part |
| [kicad/gera_footprints.py](kicad/gera_footprints.py) | **v0.3.0** | The footprint of the ESP32-C3 SuperMini, which does not exist in any library. Pads with the **silkscreen names** and the side declared in uppercase: this board's pinout appeared with three contradictory readings in two hours |
| [docs/operacao/verifica_datas.py](docs/operacao/verifica_datas.py) | **v0.2.1** | Confronts each date in the documents with the line's `git blame`. None after the clock, none after the commit that introduced it. **A date is generated data, not written** — and the `UserPromptSubmit` hook puts the clock in the context so there is no date to remember |
| [kicad/cotuca_cerebro/gera_conector.py](kicad/cotuca_cerebro/gera_conector.py) | **v0.10.0** | Generates the IDC26 connector sheet, brain-board side. Same count guard. The left-side note is **right-aligned**: with a fixed offset a long note grew into the label |
| [kicad/cotuca_painel/gera_painel.py](kicad/cotuca_painel/gera_painel.py) | **v0.17.0** | Generates the whole panel sheet. Same count guard, plus `nota()` — **the sign of `y` lives in one place only**. On the sheet `y` grows DOWN and in the symbol UP; writing the continuations with `y − 3,8` printed 8 notes back to front |
| [kicad/cotuca_cerebro/encaixa_grade.py](kicad/cotuca_cerebro/encaixa_grade.py) | **v0.3.1** | Snaps the geometry to the 1.27 mm grid. **Acceptance criterion: identical netlist** |

## Environment that works

| Item | Version |
|---|---|
| Board | Daisy Seed 2 (DFM) |
| Toolchain | ARM GNU Toolchain 15.3.rel1 (`arm-none-eabi-gcc 15.3.1`) |
| libDaisy | `~/dev/libDaisy` |
| dfu-util | 0.11 |
| Bootloader | Electrosmith `dsy_bootloader_v6_4-intdfu-2000ms.bin` (comes with libDaisy) |

> **Do not use Homebrew's `arm-none-eabi-gcc`.** It comes without newlib and libDaisy
> dies on `stdint.h: No such file or directory`. Use the cask (asks for the admin password):
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

It does three things the house rule demands and `make` does not: it flashes **only
if this time's build passed** (`make program-dfu` flashes the binary on disk,
which may be from a previous build that failed), it checks that the version **on the
board** matches the **source's**, and — since v0.4.0 —

> ⚠️ **it restores the state that the flash erased, and says which it restored.**

The params are volatile: the flash returns them all to the manifest's birth
value. That has already cost four flashes in a row zeroing the `level` with no
warning, and — worse — a microphonics notch that disappeared on the flash.
`estado.py` saves before the DFU and restores after:

```
  estado: 122 params guardados
  placa diz v0.300.0
  CONFERE
  estado: 122 params conferidos, nenhum divergiu — nada a repor.
```

> ⚠️ **The day-to-day flashing maneuver is JUST THE RESET.** `BOOT + RESET` enters
> the **ST** DFU, which only sees the internal flash — and the app lives on the QSPI.
> The symptom is `dfu-util: Last page at 0x9007760b is not writeable`
> (`Error 64`). `BOOT + RESET` serves `make program-boot`, once per
> board. A successful flash says `File downloaded successfully` and ends
> with `Error 74` on the `leave request` — that is normal.

### The app runs from the QSPI, via the bootloader

Since v0.4.0 the firmware is compiled with `APP_TYPE=BOOT_QSPI`: **the app lives in the
8 MB of QSPI and the 128 KB of internal flash belong to the Electrosmith
bootloader**. The reason is direct — with a single external DSP module the firmware already
took **94.14%** of the 128 KB internal; the second module would not fit. On the QSPI the same firmware takes
**1.52%** of 7,936 KB.

**Once per board**, flash the bootloader. Put the Seed in the STM DFU —
**hold BOOT, tap RESET, release BOOT** — and:

```bash
cd src && make program-boot
```

**Day to day, touch NOTHING.** The board enters the bootloader by command,
with an infinite timeout:

```bash
dfu go            # over the serial — the board restarts into DFU and WAITS
cd src && make program-dfu
```

That traded flashes of 119, 66 and 47 attempts (racing the ~2 s window
of the physical RESET) for **one**. See the `bancada-firmware` skill.

**If you need the manual path** (locked-up board, firmware without the command):
just tap **RESET** and flash during the bootloader's grace
window (LED pulsing, ~2 s; holding BOOT extends the window):

```bash
cd src && make program-dfu
```

`make program-dfu` **always** ends with `Error 74` on the `leave request`: the board
jumps to the app and stops responding to the last `get_status`. If
`File downloaded successfully` appeared, it flashed.

To go back to the internal flash: `make clean && make APP_TYPE=BOOT_NONE` and flash
via the STM DFU — but that erases the bootloader.

## Testing

```bash
cd src && ./cotuca_serial.py --send board list "k1 0.5" --gap 1.3
```

Or double-click `src/monitor.command`.

Firmware commands: `board` · `list` · `mods` · `cpu` · `dyn` · `probe` ·
`bb` · `midi` · `miditest` · `midiloop` · `b1` · `b2` · `k1..k5 <0-1>` ·
`set <nome> <real>` · `tone on` · `tone off`

`b1` and `b2` **simulate the panel buttons over the serial**, so the v2 panel is
testable before it exists physically: `b1` fires the randomizer, `b2`
holds the flagged parameter. `bb` reads the **black box** — memory in
BACKUP_SRAM that survives the RESET, to measure what only happens after the
serial has already died.

`set` reaches **any** parameter by name, **in real units** — Hz, ms, ×
— and refuses an out-of-range value citing the limits. With knobs 1–3 on the delay and
on the level, `test_tone_freq` and `test_tone_level` are **serial-only by design** — the test
tone is bench, not performance:

```
set tone_freq 440        # Hz, direct
set gate_release 500     # ms
set bench_boost 2        # times
```

Knob and macro keep speaking normalized (a travel position is normalized by
nature); the real unit is only at the human boundary.

### Parameter curve

`ParamTable::Add()` accepts an optional exponent:

```
real = min + pow(norm, curve) * (max - min)
```

`curve = 1.0` is linear (default, with a fast path that skips the `pow`).
`dly_time` uses **2.5**: half the knob's travel covers the first ~18% of the
range, which is where the short delays live.

**The curve is a manifest decision, not a module one.** It is ergonomics — how the knob
behaves in the hand. The DSP receives the value already curved and does not know there was a curve;
the same DSP engine serves sculptures with different curves.

`cpu` reports the audio block's load (average and peak). With the whole suite
(plus shimmer and detune): **59.5% average, 65.8% peak** of the 1,000 µs deadline,
measured and recorded in [STATUS.md](docs/STATUS.md). The delay line is
constant: it reads and writes every sample regardless of the setting.

`mods` shows the loaded DSP module, the parameters it exposed (discovered
**by name**, not by index), the manifest's bindings and the memory extract of the
SDRAM allocator.

### MIDI

MIDI port on **USART1 at 31250 baud** — TX on `D13` (pin 14), RX on `D14`
(pin 15). WIDI soldering guide, **with the warning about the TX↔RX crossover**, in
[BOARDS.md](src/BOARDS.md).

The CC→parameter map lives in the manifest and starts like this:

| CC | Parameter | Why |
|---|---|---|
| 20 | `dly_time` | also on knob 1 |
| 21 | `dly_mix` | also on knob 2 |
| 22 | `pre_level_ab` | also on knob 3 |
| 23 | `src_mix` | also on knobs 4/5 |
| 24 | `gov_ceiling` | **no knob** — the feedback edge is reachable only by MIDI or serial |

Channel configurable in the manifest, **omni by default**. CCs 20–24 are the spec's
"undefined" range, so they do not collide with mod wheel, volume or sustain.

> **MIDI is a remote knob, not a new path.** The CC delivers a
> normalized value to the `ParamTable` and stops there — exactly where the physical knob stops.
> The manifest curve, `Shaped()` and `Real()` happen afterward, the same for
> any origin. The native bench verifies this: the same gesture by CC and by
> knob gives bit for bit the same value.

**Priority between sources: last write wins.** Serial, MIDI and the future physical
knob are peers, with no hierarchy — the whole reasoning is at the top of
[main.cpp](src/main.cpp).

`midi` shows the port state, the parser counters, the map and the last
CCs received. It is the first thing to consult on soldering day: it says **at which
step the signal stopped** — wire, parser, channel or map.

`miditest <cc> <val>` injects bytes into the RX queue without going through the UART (proves the
parser and the map); `midiloop <cc> <val>` sends over the wire and needs a jumper
`D13→D14` to come back — it is the only one that proves baud rate and pin. Script in
[T10](src/TESTES.md).

### Native test benches

```bash
cd src && ./test.command
```

`dynamics.h` and `midi.h` are pure C++ and board-agnostic, so the math of the
blocks and the MIDI state machine run on the Mac — no board, no flash. **They do not
replace testing on the board**: none of them touches hardware, and none proves the UART.

`k5` is the **SRC MIX**: a linear crossfade between `IN_L` (piezo, `0.0`) and `IN_R`
(line jack, `1.0`), applied before any processing. While the
5-pot panel does not exist, `k4` also points to it — that is why
`req.min_knobs` stays at 4: a 4-knob board still reaches the SRC MIX.

Since the physical knobs are not connected yet, `ReadKnob()` returns `-1` and the
panel lives on the serial. Conversion to a target tuning, with `test_tone_freq` in
`[80, 2000]`: `norm = (Hz − 80) / 1920` — so **440 Hz = `k2 0.1875`**.

### Measure instead of trusting the ear

With `OUT_L` on a Mac audio input (here: MiniFuse 4, channel 2):

```bash
cd src && ./cotuca_measure.py --scan --expect 440
```

It reports the dominant frequency by FFT with parabolic interpolation, the level in
dBFS and the error in cents. Or double-click `src/measure.command`.

Measured across the whole range (80 / 440 / 880 / 1760 / 2000 Hz): a constant error of
**+0.4 to +0.5 cents**. Being constant in *relative* terms, it is a
clock difference — the Daisy's SAI real rate is ~0.026% above the 48000 Hz that
`AudioSampleRate()` reports and that the oscillator uses as its base. It is not a code
error, and it is inaudible (1 cent is ~1/100 of a semitone).

## Gotchas

- **No `hal::Log()` inside the audio callback.** It blocks for
  milliseconds.
- **`--specs=nano.specs` swallows `%f` silently.** The Makefile already forces
  `-u _printf_float`; if you remove it, half the panel output vanishes without warning.
- **The USB RX callback runs in an ISR.** Never call `Log()` from there — that is
  exactly what locked up the board before v0.2.1. Enqueue and dispatch in
  `PollSerial()`.
- **Do not use libDaisy's `Logger`.** Its `tx_buff_[128]` is single and
  shared: two lines in a row overwrite each other in mid-flight.
  `hal::Log` transmits on its own, with a double buffer and retry.
- **`sdram_pool` requests the whole SDRAM region, and now it shows up in the 100%.** Until
  v0.3.0 `--gc-sections` discarded the pool because nothing called `AllocBig`.
  The delay, the freeze, the reverb and the pitch engines call it, so the 64 MB
  came back all at once: the map shows `SDRAM:
  100%`. **It is the pool, not the consumption** — but there is no space left for anything else in
  `.sdram_bss`.
- **`AllocBig()` is the only access to the SDRAM.** Declaring `DSY_SDRAM_BSS` outside
  `hal_daisy_seed.cpp` is forbidden — full rule in [BOARDS.md](src/BOARDS.md).
  The boot prints the extract (`SDRAM via AllocBig: N de M bytes`); if a module
  allocates on the outside, the number does not add up.
- **`FLASH: 0 B` in the memory map is correct.** The app runs from the QSPI; the internal
  flash belongs to the bootloader. It is not an empty build.

## Open defects

**The live list is in [STATUS.md](docs/STATUS.md)** — that is where they are opened and
closed. Summary for today, 04/09:

- **🔴 The serial dies under a long callback** — priority 1. The ~3.4 ms
  bursts of the callback starve the USB ISR. Worked around with `log_mute`,
  the black box and MIDI; the proper fix (**kick-and-forget log ring**) is
  designed and not done.
- **🟡 SRAM pool at 92.4%** — a third module does not fit without changing the
  allocation strategy.
- **🟡 Block 256 costs 5.3 ms of latency** — the reason the block is 48.
- **🟢 Off-by-one in the applied counter** — a fixed difference of 1, not a loss
  rate. Confirm before calling it cosmetic.

Diagnosis of those already closed: [TESTES.md](src/TESTES.md).

## Signal path of the complete system

**Assembled and playing.** The preamp came in on 04/09 and the grounding was redone on
03/09; what remains to be confirmed with the multimeter is marked in
[STATUS.md](docs/STATUS.md). The live pin map is
[PINOS.md](docs/operacao/PINOS.md), checked at every close:

- **Inputs.** `IN_L` = piezo via a high-impedance buffer (external circuit);
  `IN_R` = line TS/TRS jack. **Pot 5** does the `piezo_mix`, which is a **balance,
  not a crossfade**: at the center both sources come in full. (`src_mix`, which
  was a crossfade and dropped both by −6 dB at the center, was retired on
  20/08 — see [FEATURES.md](docs/FEATURES.md).)
- **Outputs.** `OUT_L` → TPA3116 amp → exciter on the sculpture's body;
- **Panel (22/08).** `D24` freeze A switch · `D27` freeze A LED ·
  `D22`/`D23` freeze B (firmware ready, wiring pending) · `D25` B3 CTOS
  re-arm · `D26` B1 randomizer · `D28` WIDI LB · `D29`/`D30`
  reserved for the exciter DPDTs · `D13`/`D14` MIDI UART
  (**tied to the peripheral**: USART1 on PB6/PB7, they do not remap) ·
  `D15`–`D19` pots (A0–A4) · free: `D0`–`D12`, `D20`, `D21`.
  `OUT_R` → jack out. The routing (exciter / jack / both) is firmware, via a
  3-position toggle.
- **Switched jacks (NC)** serve as an insertion sensor via GPIO — they do not route
  signal.
- **Fixed panel.** 4 character knobs + SRC MIX + encoder with push + 2 momentary
  buttons + toggle + RGB LED.

## Next steps

**The live queue is the one in [STATUS.md](docs/STATUS.md)** — it is not duplicated here, which
is how the two versions diverge. What is at the top of it, on 04/09:

- **Redo the clean-measurement preflight.** The preamp came in on 04/09 and invalidated
  all the previous gain measurement.
- **Confirm the `AGND`/`DGND` with the multimeter**, in AC/mV and under load.
- **Kick-and-forget log ring** — kills the 🔴 priority-1 defect by
  architecture, instead of working around it.

Only what is architectural and does not age stays here:

- Second board: **Teensy 4.1**, via a new `hal_teensy41.cpp` — without touching
  anything else. It is what the `hal.h` boundary exists to allow.
