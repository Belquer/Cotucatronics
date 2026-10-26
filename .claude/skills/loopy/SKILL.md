---
name: loopy
description: Generate and change Loopy Pro projects and control profiles by script — the format of the `.lpproj` (SQLite with JSON), that of the `.lpcontrolprofile`, and the difference between the two. Use when creating pages/widgets, when mapping CCs, when regenerating bindings after renaming params, or when dB asks for something "para importar no Loopy" ("to import into Loopy").
---

# Loopy Pro by script

## FIRST: profile or project? They are different things

The mistake that costs a round-trip is delivering one when the other was needed.

| | `.lpcontrolprofile` | `.lpproj` |
|---|---|---|
| **contains** | the MAPPING of CC → widget | the **widgets themselves**, pages, colors, titles |
| **creates widgets?** | ⛔ **no** — it addresses the ones that already exist | ✅ yes |
| **serves to** | connect a controller to an already-built page | deliver the **built page** |
| **needs** | the exported `.lpproj`, to know the indices | nothing |

> **"Give it to me to import into Loopy" almost always means PROJECT.** A profile
> only solves it if the widgets already exist — and if they don't yet, the profile
> has nothing to hold on to.

---

## The format of the `.lpproj`

A zip. Inside:

```
Project.sqlite      ← everything that matters
Info.plist  Profiles/  Profile.lpcontrolprofile
Resources/  Resources.plist  Thumbnail.jpg
```

### `Project.sqlite`, two tables

```sql
CREATE TABLE objects (id INTEGER PRIMARY KEY, type TEXT, contents BLOB);
CREATE TABLE meta    (name TEXT UNIQUE, value BLOB);
```

⚠️ **The `contents` is JSON as text, NOT a binary plist.** It is worth confirming
before assuming — `b'{"frame":...'` and not `b'bplist00'`. This makes the format
entirely scriptable.

### References between objects

Every link is a pair with the type and the `id`:

```json
{"_type": "SliderWidget", "_value": 1526}
```

The same pattern wraps structured values:
`{"_type":"Rect","_value":[x,y,w,h]}`,
`{"_type":"Color","_value":[r,g,b,a]}`, `{"_type":"Set","_value":[...]}`.

### The objects that matter for a page

**`Scene`** — a page:

```json
{"title":"central", "columns":5, "rows":2, "orientation":3,
 "widgets":[{"_type":"SliderWidget","_value":1526}, ...],
 "tracks":[], "playGroups":[]}
```

**`SliderWidget`** — fader:

```json
{"frame":{"_type":"Rect","_value":[2.25, 1, 0.5, 0.5]},
 "title":"Piezo Mix", "value":0.5, "type":0,
 "color":{"_type":"Color","_value":[0.90,0.36,0.17,1]}}
```

**`ButtonWidget`** — button:

```json
{"frame":{"_type":"Rect","_value":[1,0,0.5,0.5]},
 "state":0, "title":"BUZIOS",
 "color":{"_type":"Color","_value":[1,0.008,0.004,1]}}
```

Others seen: `SteppedDialWidget` (N-position switch), `RotaryEncoderWidget`,
`TextWidget`, `Group`, `Track`.

⚠️ **The `frame` is in GRID units of the scene, not in pixels.** A `5x2` scene has
coordinates from 0 to 5 horizontally; `0.5` of width is half a cell.

**`Project`** ties it all together: `scenes` (list of references), `currentScene`,
`midiSources`, `groups`, `activeControlProfileNames`.

### To add a page

1. New `id`s that do not collide — `max(id)+1` onward.
2. A `Scene` with `widgets` pointing to the new ids.
3. Add the reference of the `Scene` to the `scenes` list of the `Project`.
4. Re-zip preserving **all** the other files of the bundle.

⚠️ **When re-zipping, write the directory entries TOO.** An `os.walk` that only
writes files loses `Control Profiles/` and `Resources/`, which are part of the
bundle. And writing only the empty ones is not enough — `Control Profiles/`
contains another directory. **The test that catches this is comparing the list of
names in the input zip with that of the output one**; counting files catches
nothing.

⚠️ **Never write over dB's project.** Generate a new file, and let him import it —
the project is where his work lives, and an error here erases hours of screen
arrangement.

---

## The format of the `.lpcontrolprofile`

Decoded on 08/24 and confirmed against two bindings made by hand.

🔴 **CORRECTED ON 09/01, and the previous error cost one unusable project.** This
skill said that the `Subject` was positional — `"<Type>:<scene>:<position>"`.
**It is not.** Verified in a real export:

```
Subject = "#<id of the widget object>"      ex: "#1526"
```

And there are **TWO profiles, one per direction**, and both address by `id`:

| file | direction | trigger | target |
|---|---|---|---|
| `Control Profile.lpcontrolprofile/Internal…` | **widget → MIDI** (send) | `"1482:Value"` — widget id and the action | — |
| `Control Profiles/<name>/<dev>.MIDI.…` | **MIDI → widget** (return) | `"b364"` — MIDI in hex | `"#1526"` |

```
Trigger MIDI = <status><cc>[<value>]        in hex
               status = 0xB0 | (channel-1)  b0=ch1  b3=ch4  b4=ch5  b5=ch6
               no value -> continuous · with value -> button gesture
Action       = {"Identifier":"Trigger Widget",
                "Parameters":{"Widget Action":"Value"},
                "Subject":"#<id>", "Timing":"Sequential"}
```

🔴 **CONSEQUENCE THAT DESTROYS A PROJECT:** regenerating the pages **erases the
ids**, and all the bindings of the two profiles now point to objects that no
longer exist. The project opens, the widgets appear, **and not a single byte goes
out or comes in** — with no error anywhere.

That is exactly what happened on 09/01: a `.lpproj` generated with 143 widgets and
158 + 202 orphan bindings. The symptom on the instrument was `bytes 0 | cc 0`.

**Therefore: whoever regenerates pages HAS to regenerate the two profiles in the
same pass**, with the new ids — or **reuse the old ids** for the widgets that
survive. Emitting a project without profiles is delivering a dead page.

## The profile is per DEVICE, and they accumulate

A bundle carries **one profile per device name**, and they stay there. In a real
export from 09/01:

```
Ctos Midi.lpcontrolprofile/
    b2d29493.MIDI.Cotucatronics Bluetooth      103 bindings
    227f559e.MIDI.Cotucatronics1 Bluetooth      99 bindings
    d51a4338.MIDI.FootCtrlPlus Bluetooth        12 bindings
```

**The one for the device that is CONNECTED counts**, and no other.

⚠️ **An old name saved from a previous session leaves the whole page mute.** That
is what happened on 09/01: Loopy was connected as `Cotucatronics 1` — leftover
from an old pairing — while all the work was in the `Cotucatronics` profile. The
widgets on the screen, the BLE connected, and **zero bytes on the instrument**.

The symptom does not point there: it looks like firmware, it looks like a wire, it
looks like the project. **Checking which device Loopy is connected to comes before
dismantling anything** — and a bundle with three profiles is the warning that
there is a way to get it wrong.

⚠️ **And the test that catches it is on the INSTRUMENT side, not Loopy's:** the
counter of MIDI bytes received. Zero bytes with BLE connected says the cut is
before the board, and distinguishes "it didn't send" from "it sent and didn't
apply".

---

## The marriage is by TITLE, and that is why renames break it

The generator links widget↔param by the **title** of the widget. Renaming a param
breaks the marriage silently — the widget is orphaned and the CC stops arriving.

**Symptom:** the generator reports `sem CC no manifest (<old name>)`. That is not a
Loopy defect; it is the title map that needs updating.

⚠️ **Keep the old names in the map pointing to the new ones.** dB's screen has the
old title written on it, and forcing him to rename twenty widgets to keep up with
a code rename is the wrong trade — **it is the code that adapts to what is painted
on the glass.**

And the inverse also appears: `título não reconhecido` means the widget exists on
the screen and the param **no longer exists in the firmware** — a retired param
with a live widget. That one is not fixed in the map: **the widget is deleted**,
or it lies forever.

---

## The delivery loop

1. dB **exports** the project from Loopy (`.lpproj`).
2. Run the generator against that file.
3. **Read the orphan count**, not just the married one. An orphan is a finding, not
   noise.
4. Deliver, and say **what did not marry and why** — a profile with 80 of 87
   bindings looks complete and is not.

⚠️ **Verify against `manifest.h`, never against memory.** The CCs change: in a
single session six changed. A document or profile written by hand diverges on the
first change, and whoever follows it builds yesterday's instrument.
