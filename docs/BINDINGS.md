# Language bindings

PlayboxLib’s ABI is **C11**. Every other language binds to `libplaybox`.

```bash
tools/build.sh build
```

| Language | Path | Quality |
|----------|------|---------|
| **C** | `include/playbox/pb.h` | Canonical |
| **C++** | `cpp/playbox.hpp` | Full Framebuffer/Sheet/Particles + App polling |
| **Rust** | `bindings/rust/` | Complete `playbox-sys` + safe `playbox` |
| **Zig** | `bindings/zig/` | `App(T)` + `Framebuffer`, `@cImport`, demo |
| **D** | `bindings/d/` | `App` class + `Framebuffer`, correct event union, dub demo |
| **Go** | `bindings/go/playbox` | Typed events, full Framebuffer, finalizer |
| **Python** | `bindings/python/` | Pointer-safe Fb, full ctypes API |
| **Nim** | `bindings/nim/` | `App` / `Framebuffer` / `Particles`, full FFI, demo |
| **ASM** | `examples/asm_demo/` | Pattern A demo |

## Graphics API (all bindings)

High-level features available through C and mirrored in bindings:

* **Braille** (2×4) / **half-block** (2×1) / **quadrant** (2×2) pixels
* **Rounded / heavy / dashed / ASCII** boxes (`pb_fb_box_ex`)
* **Panels + drop shadows**
* **Text wrap / clip** with East-Asian & emoji width
* **Alpha / add / mul blit blend**
* **Triangles**, gradients, dither, shade fills
* **Sprite sheets**, nine-slice, **particles**
* **Float cam2d** + scissor; rotated braille/half rects
* **`pb_math`** / **`pb_3d`** soft raster (depth → braille/half/quad)
* **`pb_ui`** immediate-mode widgets + popups

C demos: `pb_demo`, `pb_cube3d`, `pb_arena3d`, `pb_ui_demo` (Meson).

## Quick checks

```bash
# Rust
PLAYBOX_LIB_DIR=$PWD/build/lib cargo check --manifest-path bindings/rust/playbox/Cargo.toml

# Python
PYTHONPATH=bindings/python PLAYBOX_LIB_DIR=$PWD/build/lib python3 -c "from playbox import version; print(version())"

# Go
cd bindings/go && go build ./playbox

# Zig
zig build -C bindings/zig

# D
dmd -fPIC -Ibindings/d bindings/d/demo.d bindings/d/playbox.d \
  -L-Lbuild/lib -L-lplaybox -L-lm -L-rpath=build/lib -ofbuild/bin/pb_d_demo
# or: dub build --root bindings/d --config=demo

# Nim
nim c -d:release -o:build/bin/pb_nim_demo bindings/nim/demo.nim

# ASM
tools/build.sh asm
```

## Zig / D / Nim

```bash
# Zig (0.16+)
zig build -C bindings/zig
# binary: bindings/zig/zig-out/bin/pb_zig_demo

# D
dmd -fPIC -Ibindings/d bindings/d/demo.d bindings/d/playbox.d \
  -L-Lbuild/lib -L-lplaybox -L-lm -L-rpath=build/lib -ofbuild/bin/pb_d_demo

# Nim
nim c -d:release -o:build/bin/pb_nim_demo bindings/nim/demo.nim
```

Each provides an idiomatic `App` + `Framebuffer` wrapper over the C ABI (input polling, braille/quad, panels, particles).

## FFI tips

* Prefer `pb_rgb_ex` / `pb_cell_ex` / `pb_fb_create` over header inlines.
* Skip `pb_fb_textf` (varargs) — format in the host language.
* Python/Go must keep framebuffer **pointers** in draw callbacks (never copy the struct).
