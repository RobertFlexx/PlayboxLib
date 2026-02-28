# PlayboxLib

PlayboxLib is a small terminal game framework for C.

It gives you a clean, game-like loop (event -> update -> draw), a framebuffer you can draw into, input parsing (keys, text, mouse, resize), and a fast ANSI renderer that only prints what changed.

It is designed to be:

* easy to embed
* portable across POSIX terminals
* fast enough for real gameplay
* simple enough that you can read the source in one sitting

This repo also includes an optional C++ wrapper and a small tooling folder.

## (THIS IS ALPHA PHASE, EXPECT BUGS.)
---

## What PlayboxLib does

At a high level, PlayboxLib handles the annoying terminal stuff so your code can focus on the game.

PlayboxLib:

* puts the terminal into a usable "game mode" (raw input, alt screen, hidden cursor)
* reads key presses and mouse data from stdin
* converts raw bytes into structured `pb_event` values
* gives you a 2D framebuffer made of cells (`pb_cell`)
* renders the framebuffer using ANSI escape sequences
* runs a main loop with delta time (`dt`) and a target FPS
* optionally records and replays runs for reproducible debugging

You write:

* your game state struct
* an event handler
* an update function
* a draw function

---

## Quick start

### Build the library

```bash
tools/build.sh build
```

Outputs:

* `build/lib/libplaybox.a`
* `build/lib/libplaybox.so`

If the C++ wrapper exists (`cpp/playbox.cpp`):

* `build/lib/libplayboxpp.a`

### Build everything (including Rust CLI)

```bash
tools/build.sh all
```

Outputs:

* library files in `build/lib`
* Rust CLI in `build/bin/playbox` (if `tools/rust/` exists)

### Debug build

```bash
tools/build.sh all --debug
```

### Clean

```bash
tools/build.sh clean
```

### Install to a prefix

```bash
tools/build.sh all
tools/build.sh install --prefix "$HOME/.local"
```

Installs:

* headers into `$prefix/include/playbox`
* libraries into `$prefix/lib`
* tools into `$prefix/bin` (if built)

---

## Installing PlayboxLib to your path



Once installed, PlayboxLib can be used from any directory.



### Per-user install (recommended)



```bash

export C_INCLUDE_PATH="$HOME/.local/include:$C_INCLUDE_PATH"

export LIBRARY_PATH="$HOME/.local/lib:$LIBRARY_PATH"

export LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"

```



Most modern Linux distributions already include `$HOME/.local` in the default search paths.



### System-wide install



If you installed to `/usr/local`, no environment changes are required.



### Verify installation



```bash

printf '%s\n' '#include "playbox/pb.h"' | cc -x c - -lplaybox

```



If this command succeeds, PlayboxLib is correctly installed and usable from any directory.



## Using the library

Include the umbrella header:

```c
#include "playbox/pb.h"
```

Create an app by providing callbacks:

```c
static void on_event(pb_app* app, void* user, const pb_event* ev);
static void on_update(pb_app* app, void* user, double dt);
static void on_draw(pb_app* app, void* user, pb_fb* fb);

int main(void){
    pb_app_desc d = {0};
    d.title = "My Game";
    d.target_fps = 120;
    d.on_event = on_event;
    d.on_update = on_update;
    d.on_draw = on_draw;

    my_state state = {0};
    pb_app* app = pb_app_create(&d, &state);
    if(!app) return 1;

    pb_app_run(app);
    pb_app_destroy(app);
    return 0;
}
```

Callback responsibilities:

* `on_event` handles input (`pb_event`)
* `on_update` advances your simulation using `dt` seconds
* `on_draw` draws your current game state into the framebuffer

A detailed guide is in `HowToImplement.md`.

---

## Drawing model

PlayboxLib uses a "cell framebuffer".

Each cell contains:

* a Unicode codepoint (`ch`)
* foreground RGB (`fg`)
* background RGB (`bg`)
* style flags (`bold`, `dim`, `underline`, `reverse`)

Useful functions:

* `pb_fb_clear`
* `pb_fb_put`
* `pb_fb_text`
* `pb_fb_fill_rect`
* `pb_fb_box`

Example:

```c
pb_color fg = pb_rgb(220,220,220);
pb_color bg = pb_rgb(10,12,16);

pb_fb_clear(fb, pb_cell_make(' ', fg, bg, 0));
pb_fb_text(fb, 2, 2, "hello", fg, bg, PB_STYLE_BOLD);
```

You do not print directly while Playbox is running. Draw into the framebuffer and let the renderer present it.

---

## Input model

Input comes in as `pb_event`:

Event types:

* `PB_EVENT_KEY`
* `PB_EVENT_TEXT`
* `PB_EVENT_MOUSE`
* `PB_EVENT_RESIZE`
* `PB_EVENT_QUIT`

### Keys

Arrow keys and function keys are normalized to `pb_key` values like:

* `PB_KEY_UP`, `PB_KEY_DOWN`, `PB_KEY_LEFT`, `PB_KEY_RIGHT`
* `PB_KEY_F1` ... `PB_KEY_F12`

Printable keys usually show up as:

* `PB_EVENT_KEY` with `ev->as.key.codepoint`

Ctrl and Alt modifiers are available through:

* `ev->as.key.ctrl`
* `ev->as.key.alt`
* `ev->as.key.shift`

### Text

For actual text input (Unicode codepoints), you can also handle:

* `PB_EVENT_TEXT`

### Mouse

Mouse input includes:

* x, y
* button
* pressed
* wheel
* modifier keys

### Resize

`PB_EVENT_RESIZE` reports the new terminal dimensions.

Playbox automatically resizes the framebuffer and then sends you the event.

---

## Record and replay

PlayboxLib can record a run and replay it later.

This is useful for:

* reproducing bugs
* generating deterministic demos
* testing input-heavy games

### Record

```bash
PLAYBOX_RECORD=run.pbr ./your_demo
```

### Replay

```bash
PLAYBOX_REPLAY=run.pbr ./your_demo
```

### Replay metadata

Replay files store:

* delta time per frame
* events per frame
* header metadata (seed + initial terminal size)

If your game uses randomness, seed your RNG from the replay seed:

```c
if(pb_app_is_replay(app)){
    uint32_t seed = pb_app_replay_seed(app);
    rng_seed(seed);
}
```

You can override the recording seed with:

```bash
PLAYBOX_SEED=12345 PLAYBOX_RECORD=run.pbr ./your_demo
```

The replay format is documented in `docs/replay_format.md`.

---

## Tooling

This repo includes a small `tools/` folder.

Common scripts:

* `tools/build.sh` build/install helper
* `tools/replayctl.sh` run/record/replay wrapper
* `tools/dev.sh` quick dev runner (expects a demo binary name)

Example:

```bash
tools/build.sh all --debug
tools/replayctl.sh record mydemo out.pbr
tools/replayctl.sh replay mydemo out.pbr
```

---

## Repo structure

* `include/playbox/`

  * public headers
* `src/`

  * C implementation
* `cpp/`

  * optional C++ wrapper
* `tools/`

  * helper scripts and optional utilities
* `docs/`

  * file format docs and extra notes

---

## Platform notes

PlayboxLib targets POSIX terminals.

It uses:

* raw mode (`termios`)
* non-blocking stdin reads
* ANSI escape sequences for rendering

Terminals that work well:

* kitty
* wezterm
* alacritty
* foot
* konsole

If colors look wrong, check `TERM` and `COLORTERM`.

---

## Common troubleshooting

### Nothing shows on screen

* Run inside a real terminal, not a restricted embedded console
* Try `tools/build.sh doctor`

### Arrow keys do not work

* Some terminals send different escape sequences
* This is input parsing, not rendering

### Replay diverges

Replay is only deterministic if your game is deterministic.

Typical causes:

* using system time directly in gameplay logic
* using `rand()` without a stable seed
* non-deterministic physics steps

Fix:

* seed RNG from `pb_app_replay_seed(app)`
* drive simulation from dt, preferably fixed-step logic

### Resize feels unstable

* make sure your layout updates on `PB_EVENT_RESIZE`
* avoid assuming a minimum terminal size without clamping

---

## What this is good for

PlayboxLib is a good fit for:

* Snake, Tetris, Breakout, Pong
* roguelikes
* terminal dashboards
* interactive dev tools
* debug visualizers

It is not trying to be:

* a full UI framework
* a replacement for ncurses in every scenario

It is intentionally small and readable.

---

## Python asset packer (tools/py/playbox_pack.py)

This repo includes a small helper that packs files into a C header.

Why you would use it:

* ship demos with zero runtime file loading
* embed maps, shaders, small sprites, UI text, configs
* keep your project as a single binary if you want

It outputs a header that contains:

* a static const array holding the bytes
* a static const size_t symbol_len length

### Pack a text file

```bash
python3 tools/py/playbox_pack.py pack-text assets/dialog.txt -o include/pb_assets.h
```

### Pack a binary file

```bash
python3 tools/py/playbox_pack.py pack-bin assets/logo.bin -o include/pb_assets.h
```

### Control the symbol name

```bash
python3 tools/py/playbox_pack.py pack-text assets/dialog.txt --symbol dialog_txt -o include/pb_assets.h
```

### Gzip the embedded bytes (smaller header)

```bash
python3 tools/py/playbox_pack.py pack-bin assets/big_blob.bin --gz -o include/pb_assets.h
```

If you use --gz, the bytes are compressed and must be decompressed at runtime.

## License

View LICENSE [here](https://github.com/RobertFlexx/PlayboxLib/blob/main/LICENSE)

---
Heres how to use this in ASM (could be buggy) [here!](https://github.com/RobertFlexx/PlayboxLib/blob/main/OfficialASMGuide.md)


---
## Credits

Built as a minimal terminal framework focused on gameplay and tooling.
