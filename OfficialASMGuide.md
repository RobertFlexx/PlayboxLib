# PlayboxLib + Assembly Games (ASM Guide)
## You dont have to make the game in the examples folder, thats just an example.
This document shows how to use PlayboxLib from an assembly-written game.

PlayboxLib is a C terminal game framework. Assembly works with it because Playbox exposes a normal C ABI. If you can link against C and follow the platform calling convention, you can write your game logic in ASM and still use Playbox for terminal setup, input, framebuffer drawing, and record/replay.

This guide targets x86_64 Linux first. macOS notes are included. Windows is possible, but Playbox itself is POSIX terminal focused, so treat Windows as out-of-scope unless you are doing MSYS2 or WSL.

---

## What you are building

You will typically do one of these two patterns:

### Pattern A: C glue, game logic in ASM (recommended)

* C owns `main()` and calls Playbox.
* Your callbacks (`on_event`, `on_update`, `on_draw`) are ASM functions.

This is the cleanest and most portable option.

### Pattern B: ASM `main()`, call Playbox directly

* ASM owns `main()`.
* ASM calls Playbox `pb_app_create`, sets up function pointers, runs the loop.

This works, but it is more annoying because you must build and fill structs like `pb_app_desc` exactly.

This guide focuses on Pattern A because it gives you the most fun per minute and the fewest ABI foot-guns.

---

## Quick facts you must not ignore

* On x86_64 Linux and most UNIXes, you use the SysV AMD64 ABI.
* Stack alignment before `call` must be 16-byte aligned.
* Integer and pointer args go in registers: RDI, RSI, RDX, RCX, R8, R9.
* Floating args use XMM registers.
* Return values: RAX for integer/pointers. XMM0 for double/float.
* Do not pass Playbox structs by value from ASM unless you are extremely confident. Prefer passing pointers.

Playbox calls your callbacks like this:

* `on_event(pb_app* app, void* user, const pb_event* ev)`
* `on_update(pb_app* app, void* user, double dt)`
* `on_draw(pb_app* app, void* user, pb_fb* fb)`

So your ASM callbacks receive:

* RDI = `pb_app* app`
* RSI = `void* user`
* RDX = `pb_event* ev` (for on_event)
* XMM0 = `double dt` (for on_update)
* RDX = `pb_fb* fb` (for on_draw)

---

## Minimal file layout

A simple, sane layout:

```
examples/asm_demo/
  main.c
  game.asm
  game.h
```

* `main.c` contains Playbox app creation.
* `game.asm` contains your callbacks.
* `game.h` declares the ASM symbols so C can call them.

---

## Minimal C glue (Pattern A)

Create `examples/asm_demo/game.h`:

```c
#pragma once
#include "playbox/pb.h"

typedef struct {
    int tick;
    int x;
    int y;
} asm_state;

void asm_on_event(pb_app* app, void* user, const pb_event* ev);
void asm_on_update(pb_app* app, void* user, double dt);
void asm_on_draw(pb_app* app, void* user, pb_fb* fb);
```

Create `examples/asm_demo/main.c`:

```c
#include "playbox/pb.h"
#include "game.h"

int main(void){
    pb_app_desc d = (pb_app_desc){0};
    d.title = "asm demo";
    d.target_fps = 60;
    d.on_event = asm_on_event;
    d.on_update = asm_on_update;
    d.on_draw = asm_on_draw;

    asm_state st = {0};
    st.x = 2;
    st.y = 2;

    pb_app* app = pb_app_create(&d, &st);
    if(!app) return 1;

    pb_app_run(app);
    pb_app_destroy(app);
    return 0;
}
```

That is the entire C side.

---

## Assembly callback requirements

Your ASM file must export these three symbols:

* `asm_on_event`
* `asm_on_update`
* `asm_on_draw`

On Linux with NASM:

* Mark them `global`.
* Use `default rel` if you want nicer RIP-relative addressing.

You can keep all of your game state in the `asm_state` struct that is passed through `user`.

From ASM, `user` is just a pointer. You decide the memory layout.

---

## A small but real ASM demo

This example:

* Moves a character around with arrow keys.
* Quits on Q.
* Draws a tiny UI.

This is written for NASM, x86_64 Linux.

Create `examples/asm_demo/game.asm`:

```asm
bits 64
default rel

global asm_on_event
global asm_on_update
global asm_on_draw

extern pb_app_quit
extern pb_fb_clear
extern pb_fb_put
extern pb_fb_text
extern pb_cell_make
extern pb_rgb

section .rodata
msg_title: db "ASM demo: arrows move, q quits", 0

section .text

; asm_state layout must match C:
; struct { int tick; int x; int y; }
%define ST_TICK 0
%define ST_X    4
%define ST_Y    8

; pb_key values are from Playbox headers.
; Keep these in sync with include/playbox/pb_keys.h.
; If you do not want hardcoded numbers, do key mapping on the C side.
%define PB_KEY_UP    1001
%define PB_KEY_DOWN  1002
%define PB_KEY_LEFT  1003
%define PB_KEY_RIGHT 1004

; pb_event_type values from pb.h / pb_event.h
%define PB_EVENT_KEY 1

; pb_event key layout is a Playbox internal contract.
; The safe route is: handle key decoding in C, pass simplified inputs to ASM.
; For a small demo, we assume:
; ev->type is at offset 0 (u32)
; ev->as.key.key at some known offset.
; If this ever changes, update offsets or switch to a C shim.

; YOU SHOULD REPLACE THESE OFFSETS WITH THE REAL ONES FROM YOUR pb_event struct.
%define EV_TYPE 0
%define EV_KEY_KEY 8
%define EV_KEY_PRESSED 28
%define EV_KEY_CODEPOINT 12

asm_on_event:
    ; RDI=pb_app* app, RSI=user, RDX=ev
    push rbp
    mov rbp, rsp

    ; load event type
    mov eax, dword [rdx + EV_TYPE]
    cmp eax, PB_EVENT_KEY
    jne .done

    ; only on press
    mov eax, dword [rdx + EV_KEY_PRESSED]
    test eax, eax
    jz .done

    ; quit if codepoint == 'q' or 'Q'
    mov eax, dword [rdx + EV_KEY_CODEPOINT]
    cmp eax, 'q'
    je .quit
    cmp eax, 'Q'
    je .quit

    ; key enum
    mov eax, dword [rdx + EV_KEY_KEY]

    cmp eax, PB_KEY_UP
    je .up
    cmp eax, PB_KEY_DOWN
    je .down
    cmp eax, PB_KEY_LEFT
    je .left
    cmp eax, PB_KEY_RIGHT
    je .right
    jmp .done

.up:
    sub dword [rsi + ST_Y], 1
    jmp .done
.down:
    add dword [rsi + ST_Y], 1
    jmp .done
.left:
    sub dword [rsi + ST_X], 1
    jmp .done
.right:
    add dword [rsi + ST_X], 1
    jmp .done

.quit:
    ; pb_app_quit(app)
    mov rdi, rdi
    call pb_app_quit

.done:
    mov rsp, rbp
    pop rbp
    ret

asm_on_update:
    ; RDI=pb_app* app, RSI=user, XMM0=dt
    ; Just tick.
    push rbp
    mov rbp, rsp

    add dword [rsi + ST_TICK], 1

    mov rsp, rbp
    pop rbp
    ret

asm_on_draw:
    ; RDI=pb_app* app, RSI=user, RDX=fb
    push rbp
    mov rbp, rsp

    ; Make colors: pb_rgb(r,g,b) returns pb_color
    ; pb_color is usually a 32-bit packed int, returned in EAX.

    ; fg = pb_rgb(220,220,220)
    mov edi, 220
    mov esi, 220
    mov edx, 220
    call pb_rgb
    mov r12d, eax

    ; bg = pb_rgb(10,12,16)
    mov edi, 10
    mov esi, 12
    mov edx, 16
    call pb_rgb
    mov r13d, eax

    ; cell = pb_cell_make(' ', fg, bg, 0)
    mov edi, ' '
    mov esi, r12d
    mov edx, r13d
    xor ecx, ecx
    call pb_cell_make
    mov r14d, eax

    ; pb_fb_clear(fb, cell)
    mov rdi, rdx
    mov esi, r14d
    call pb_fb_clear

    ; pb_fb_text(fb, 2, 1, msg, fg, bg, 0)
    mov rdi, rdx
    mov esi, 2
    mov edx, 1
    lea rcx, [msg_title]
    mov r8d, r12d
    mov r9d, r13d
    sub rsp, 16
    mov qword [rsp + 0], 0
    call pb_fb_text
    add rsp, 16

    ; draw player '@' at (x,y)
    mov eax, dword [rsi + ST_X]
    mov ebx, dword [rsi + ST_Y]

    ; cell = pb_cell_make('@', fg, bg, 0)
    mov edi, '@'
    mov esi, r12d
    mov edx, r13d
    xor ecx, ecx
    call pb_cell_make

    ; pb_fb_put(fb, x, y, cell)
    mov rdi, rdx
    mov esi, eax
    mov edx, ebx
    mov ecx, eax
    call pb_fb_put

    mov rsp, rbp
    pop rbp
    ret
```

Important warnings about the ASM above:

* The `pb_event` offsets and key enum values are placeholders. You must match your real header layouts.
* The safest approach is to do event decoding in C and pass a simplified input state into ASM.

If you want this to be truly stable long-term, do this:

* C `on_event` reads `pb_event`.
* C updates an `input_state` struct.
* ASM reads `input_state` and never touches `pb_event`.

That avoids struct layout fragility.

---

## Building the ASM demo

There are two normal ways to assemble:

### Option 1: NASM (simple)

Build object:

```bash
nasm -f elf64 -g -F dwarf -o examples/asm_demo/game.o examples/asm_demo/game.asm
```

Then compile and link with Playbox:

```bash
cc -Iinclude -Icpp -Iexamples/asm_demo \
  -o build/asm_demo \
  examples/asm_demo/main.c examples/asm_demo/game.o \
  build/lib/libplaybox.a -lm
```

If you built Playbox as a shared library:

```bash
cc -Iinclude -Icpp -Iexamples/asm_demo \
  -o build/asm_demo \
  examples/asm_demo/main.c examples/asm_demo/game.o \
  -Lbuild/lib -lplaybox -Wl,-rpath,'$ORIGIN/../lib' -lm
```

### Option 2: GAS (.S) (more portable)

If you prefer GNU as:

* Write `game.S` instead of NASM.
* Assemble with `cc -c game.S`.

Example:

```bash
cc -c -o examples/asm_demo/game.o examples/asm_demo/game.S
```

---

## CMake integration

If you want people to build the ASM example with CMake:

* Add a CMake option `PLAYBOXLIB_BUILD_ASM_EXAMPLES`.
* Use `enable_language(ASM_NASM)` or compile `.S` with the C compiler.

Example approach (NASM):

```cmake
option(PLAYBOXLIB_BUILD_ASM_EXAMPLES "Build assembly examples" OFF)

if(PLAYBOXLIB_BUILD_ASM_EXAMPLES)
  enable_language(ASM_NASM)
  add_executable(pb_asm_demo examples/asm_demo/main.c examples/asm_demo/game.asm)
  target_include_directories(pb_asm_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/cpp)
  target_link_libraries(pb_asm_demo PRIVATE playboxlib)
endif()
```

If you want to avoid NASM and keep dependencies low, use `.S` and let `cc` assemble it.

---

## Meson integration

Meson can build ASM with:

* `nasm` objects
* or `.S` compiled by the C compiler

A simple Meson approach using NASM:

```meson
nasm = find_program('nasm', required: false)

if nasm.found()
  game_o = custom_target('asm_demo_game',
    input: 'examples/asm_demo/game.asm',
    output: 'game.o',
    command: [nasm, '-f', 'elf64', '-o', '@OUTPUT@', '@INPUT@']
  )

  executable('pb_asm_demo',
    ['examples/asm_demo/main.c', game_o],
    dependencies: playbox_dep,
    install: false
  )
endif()
```

If you want maximum portability, prefer `.S` and use `cc` to assemble.

---

## Record/replay with an ASM game

Record:

```bash
PLAYBOX_RECORD=run.pbr ./pb_asm_demo
```

Replay:

```bash
PLAYBOX_REPLAY=run.pbr ./pb_asm_demo
```

If your ASM game uses randomness:

* Do not call system time for RNG.
* Seed deterministically.

If you want the replay seed:

* Read it from C and store it into your `asm_state` before running.
* Or call `pb_app_replay_seed(app)` from ASM if you export it and treat it as a C function.

Recommended: use C glue to seed your ASM state.

---

## Keeping it stable over time

Assembly is extremely sensitive to ABI details. The way to keep your ASM games from breaking when Playbox evolves:

1. Avoid reading Playbox structs directly in ASM

Do not parse `pb_event` fields in ASM.

Instead:

* C decodes `pb_event`.
* C writes a simple `input_state` with fixed offsets.
* ASM only reads `input_state`.

2. Only call Playbox functions that have simple signatures

Good calls from ASM:

* `pb_fb_put(fb, x, y, cell)`
* `pb_fb_text(fb, x, y, cstr, fg, bg, style)`
* `pb_fb_clear(fb, cell)`
* `pb_app_quit(app)`

Harder calls:

* Anything that returns or takes structs by value
* Anything that requires varargs

3. Keep a small compatibility header

You can generate a tiny header of constants for ASM builds:

* key enums
* event type enums
* style flags

If you change Playbox keys, regenerate the header.

---

## macOS notes

* macOS uses the SysV AMD64 ABI too.
* The object format is Mach-O, not ELF.
* NASM can emit Mach-O with `-f macho64`.
* Symbol naming is different in some toolchains, so test.

Because Playbox is POSIX terminal based, the core behavior works, but build/link steps differ.

If you want the simplest cross-platform assembly story, use `.S` and let `clang` assemble it.

---

## FAQ

### Does Playbox need special ASM support?

No. Playbox is C. Assembly can already use it.

What helps is documentation, examples, and build targets.

### Should I put an ASM API in Playbox?

Not recommended.

Keep the C ABI as the only contract. Add an example project for ASM users.

### What is the best ASM game to write with Playbox?

Start with something tiny:

* Pong
* Snake
* Breakout

Then do something spicy:

* a tiny roguelike loop
* a particle sandbox

If you want the coolest demo, make a small physics toy and record/replay it.

---

## Recommended next additions to the repo

If you want to officially support ASM usage in a clean way, add these:

* `docs/asm.md` (this file)
* `examples/asm_demo/`
* a C shim example that converts `pb_event` into a stable `input_state`
* optional CMake + Meson toggles to build the ASM example

That is real support. No extra bloat in the library.
