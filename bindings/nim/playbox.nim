## PlayboxLib — production Nim bindings
##
## Low-level importc FFI + idiomatic `App` / `Framebuffer` wrappers.
##
## Build demo (from repo root, after `tools/build.sh build`):
##   nim c -d:release -o:build/bin/pb_nim_demo bindings/nim/demo.nim

import std/os

const
  playboxRoot = currentSourcePath.parentDir.parentDir.parentDir
  playboxInclude = playboxRoot / "include"
  playboxLib = playboxRoot / "build" / "lib"

{.passC: "-I" & playboxInclude.}
{.passL: "-L" & playboxLib & " -lplaybox -lm -Wl,-rpath," & playboxLib.}

# ---------------------------------------------------------------------------
# ABI
# ---------------------------------------------------------------------------

type
  PbColor* {.importc: "pb_color", header: "playbox/pb.h", bycopy.} = object
    r*, g*, b*: uint8

  PbCell* {.importc: "pb_cell", header: "playbox/pb.h", bycopy.} = object
    ch*: uint32
    fg*, bg*: PbColor
    style*: uint16

  PbFb* {.importc: "pb_fb", header: "playbox/pb.h", bycopy.} = object
    w*, h*: cint
    cells*: ptr PbCell
    cam_x*, cam_y*: cint
    clip_x0*, clip_y0*, clip_x1*, clip_y1*: cint

  PbApp* {.importc: "pb_app", header: "playbox/pb.h".} = object

  PbKeyEvent* {.importc: "pb_key_event", header: "playbox/pb.h", bycopy.} = object
    key*: cint
    codepoint*: uint32
    alt*, ctrl*, shift*, pressed*: uint8

  PbMouseEvent* {.importc: "pb_mouse_event", header: "playbox/pb.h", bycopy.} = object
    x*, y*: cint
    button*, pressed*: uint8
    wheel*: cint
    shift*, alt*, ctrl*: uint8

  PbResizeEvent* {.importc: "pb_resize_event", header: "playbox/pb.h", bycopy.} = object
    width*, height*: cint

  PbFocusEvent* {.importc: "pb_focus_event", header: "playbox/pb.h", bycopy.} = object
    focused*: uint8

  # Manual layout matching C `pb_event` (anonymous union is awkward via importc).
  PbEventData* {.union.} = object
    key*: PbKeyEvent
    text*: uint32
    mouse*: PbMouseEvent
    resize*: PbResizeEvent
    focus*: PbFocusEvent

  PbEvent* {.bycopy.} = object
    `type`*: cint
    `as`*: PbEventData

  PbOnInit* = proc (app: ptr PbApp; user: pointer) {.cdecl.}
  PbOnEvent* = proc (app: ptr PbApp; user: pointer; ev: ptr PbEvent) {.cdecl.}
  PbOnUpdate* = proc (app: ptr PbApp; user: pointer; dt: cdouble) {.cdecl.}
  PbOnDraw* = proc (app: ptr PbApp; user: pointer; fb: ptr PbFb) {.cdecl.}
  PbOnShutdown* = proc (app: ptr PbApp; user: pointer) {.cdecl.}

  # Manual layout matching C `pb_app_desc` so trampolines can use our PbEvent.
  PbAppDesc* {.bycopy.} = object
    title*: cstring
    target_fps*: cint
    flags*: uint32
    clear*: PbCell
    on_init*: PbOnInit
    on_event*: PbOnEvent
    on_update*: PbOnUpdate
    on_draw*: PbOnDraw
    on_shutdown*: PbOnShutdown

  PbSheet* {.importc: "pb_sheet", header: "playbox/pb.h", bycopy.} = object
    atlas*: PbFb
    tile_w*, tile_h*, cols*, rows*, owns_atlas*: cint

  PbParticle* {.importc: "pb_particle", header: "playbox/pb.h", bycopy.} = object
    x*, y*, vx*, vy*, life*, max_life*: cfloat
    color*: PbColor
    alive*: uint8

  PbParticles* {.importc: "pb_particles", header: "playbox/pb.h", bycopy.} = object
    items*: ptr PbParticle
    count*, capacity*: cint

proc pbRgbEx*(r, g, b: uint8): PbColor {.importc: "pb_rgb_ex", header: "playbox/pb.h".}
proc pbCellEx*(ch: uint32; fg, bg: PbColor; style: uint16): PbCell {.importc: "pb_cell_ex", header: "playbox/pb.h".}
proc pbVersionString*(): cstring {.importc: "pb_version_string", header: "playbox/pb.h".}
proc pbCharWidth*(cp: uint32): cint {.importc: "pb_char_width", header: "playbox/pb.h".}

proc pbAppCreate*(desc: pointer; user: pointer): ptr PbApp {.importc: "pb_app_create", header: "playbox/pb.h".}
proc pbAppDestroy*(app: ptr PbApp) {.importc: "pb_app_destroy", header: "playbox/pb.h".}
proc pbAppRun*(app: ptr PbApp): cint {.importc: "pb_app_run", header: "playbox/pb.h".}
proc pbAppQuit*(app: ptr PbApp) {.importc: "pb_app_quit", header: "playbox/pb.h".}
proc pbAppRequestResize*(app: ptr PbApp) {.importc: "pb_app_request_resize", header: "playbox/pb.h".}
proc pbAppWidth*(app: ptr PbApp): cint {.importc: "pb_app_width", header: "playbox/pb.h".}
proc pbAppHeight*(app: ptr PbApp): cint {.importc: "pb_app_height", header: "playbox/pb.h".}
proc pbAppSetTitle*(app: ptr PbApp; title: cstring) {.importc: "pb_app_set_title", header: "playbox/pb.h".}
proc pbAppSetClear*(app: ptr PbApp; clear: PbCell) {.importc: "pb_app_set_clear", header: "playbox/pb.h".}
proc pbAppSetTargetFps*(app: ptr PbApp; fps: cint) {.importc: "pb_app_set_target_fps", header: "playbox/pb.h".}
proc pbAppIsReplay*(app: ptr PbApp): cint {.importc: "pb_app_is_replay", header: "playbox/pb.h".}
proc pbAppReplaySeed*(app: ptr PbApp): uint32 {.importc: "pb_app_replay_seed", header: "playbox/pb.h".}

proc pbIsKeyDown*(app: ptr PbApp; key: cint): cint {.importc: "pb_is_key_down", header: "playbox/pb.h".}
proc pbIsKeyPressed*(app: ptr PbApp; key: cint): cint {.importc: "pb_is_key_pressed", header: "playbox/pb.h".}
proc pbIsKeyReleased*(app: ptr PbApp; key: cint): cint {.importc: "pb_is_key_released", header: "playbox/pb.h".}
proc pbIsCharDown*(app: ptr PbApp; cp: uint32): cint {.importc: "pb_is_char_down", header: "playbox/pb.h".}
proc pbIsCharPressed*(app: ptr PbApp; cp: uint32): cint {.importc: "pb_is_char_pressed", header: "playbox/pb.h".}
proc pbGetMouseX*(app: ptr PbApp): cint {.importc: "pb_get_mouse_x", header: "playbox/pb.h".}
proc pbGetMouseY*(app: ptr PbApp): cint {.importc: "pb_get_mouse_y", header: "playbox/pb.h".}
proc pbIsMouseButtonDown*(app: ptr PbApp; button: cint): cint {.importc: "pb_is_mouse_button_down", header: "playbox/pb.h".}
proc pbIsMouseButtonPressed*(app: ptr PbApp; button: cint): cint {.importc: "pb_is_mouse_button_pressed", header: "playbox/pb.h".}
proc pbGetMouseWheel*(app: ptr PbApp): cint {.importc: "pb_get_mouse_wheel", header: "playbox/pb.h".}
proc pbGetFps*(app: ptr PbApp): cint {.importc: "pb_get_fps", header: "playbox/pb.h".}
proc pbGetFrameTime*(app: ptr PbApp): cdouble {.importc: "pb_get_frame_time", header: "playbox/pb.h".}
proc pbIsFocused*(app: ptr PbApp): cint {.importc: "pb_is_focused", header: "playbox/pb.h".}

proc pbFbClear*(fb: ptr PbFb; fill: PbCell) {.importc: "pb_fb_clear", header: "playbox/pb.h".}
proc pbFbPut*(fb: ptr PbFb; x, y: cint; c: PbCell) {.importc: "pb_fb_put", header: "playbox/pb.h".}
proc pbFbGet*(fb: ptr PbFb; x, y: cint): PbCell {.importc: "pb_fb_get", header: "playbox/pb.h".}
proc pbFbPutBlend*(fb: ptr PbFb; x, y: cint; c: PbCell; alpha: cfloat; mode: cint) {.importc: "pb_fb_put_blend", header: "playbox/pb.h".}
proc pbFbText*(fb: ptr PbFb; x, y: cint; utf8: cstring; fg, bg: PbColor; style: uint16) {.importc: "pb_fb_text", header: "playbox/pb.h".}
proc pbFbTextCentered*(fb: ptr PbFb; y: cint; utf8: cstring; fg, bg: PbColor; style: uint16) {.importc: "pb_fb_text_centered", header: "playbox/pb.h".}
proc pbFbTextWrap*(fb: ptr PbFb; x, y, maxW, maxH: cint; utf8: cstring; fg, bg: PbColor; style: uint16): cint {.importc: "pb_fb_text_wrap", header: "playbox/pb.h".}
proc pbFbTextClipped*(fb: ptr PbFb; x, y, maxW: cint; utf8: cstring; fg, bg: PbColor; style: uint16) {.importc: "pb_fb_text_clipped", header: "playbox/pb.h".}
proc pbFbMeasureText*(utf8: cstring): cint {.importc: "pb_fb_measure_text", header: "playbox/pb.h".}
proc pbFbFillRect*(fb: ptr PbFb; x, y, w, h: cint; c: PbCell) {.importc: "pb_fb_fill_rect", header: "playbox/pb.h".}
proc pbFbFillShade*(fb: ptr PbFb; x, y, w, h: cint; fg, bg: PbColor; level: cint) {.importc: "pb_fb_fill_shade", header: "playbox/pb.h".}
proc pbFbLine*(fb: ptr PbFb; x0, y0, x1, y1: cint; c: PbCell) {.importc: "pb_fb_line", header: "playbox/pb.h".}
proc pbFbCircle*(fb: ptr PbFb; cx, cy, radius: cint; c: PbCell) {.importc: "pb_fb_circle", header: "playbox/pb.h".}
proc pbFbFillCircle*(fb: ptr PbFb; cx, cy, radius: cint; c: PbCell) {.importc: "pb_fb_fill_circle", header: "playbox/pb.h".}
proc pbFbFillTriangle*(fb: ptr PbFb; x0, y0, x1, y1, x2, y2: cint; c: PbCell) {.importc: "pb_fb_fill_triangle", header: "playbox/pb.h".}
proc pbFbBox*(fb: ptr PbFb; x, y, w, h: cint; fg, bg: PbColor; style: uint16) {.importc: "pb_fb_box", header: "playbox/pb.h".}
proc pbFbBoxEx*(fb: ptr PbFb; x, y, w, h: cint; boxStyle: cint; fg, bg: PbColor; style: uint16) {.importc: "pb_fb_box_ex", header: "playbox/pb.h".}
proc pbFbBoxDouble*(fb: ptr PbFb; x, y, w, h: cint; fg, bg: PbColor; style: uint16) {.importc: "pb_fb_box_double", header: "playbox/pb.h".}
proc pbFbPanel*(fb: ptr PbFb; x, y, w, h: cint; title: cstring; border, titleFg, fill: PbColor; style: uint16) {.importc: "pb_fb_panel", header: "playbox/pb.h".}
proc pbFbPanelEx*(fb: ptr PbFb; x, y, w, h: cint; title: cstring; boxStyle: cint; border, titleFg, fill: PbColor; style: uint16) {.importc: "pb_fb_panel_ex", header: "playbox/pb.h".}
proc pbFbShadow*(fb: ptr PbFb; x, y, w, h: cint; shadow: PbColor; alpha: cfloat) {.importc: "pb_fb_shadow", header: "playbox/pb.h".}
proc pbFbBlit*(dst: ptr PbFb; dx, dy: cint; src: ptr PbFb) {.importc: "pb_fb_blit", header: "playbox/pb.h".}
proc pbFbBlitMasked*(dst: ptr PbFb; dx, dy: cint; src: ptr PbFb; transparent: uint32) {.importc: "pb_fb_blit_masked", header: "playbox/pb.h".}
proc pbFbBlitBlend*(dst: ptr PbFb; dx, dy: cint; src: ptr PbFb; alpha: cfloat; mode: cint) {.importc: "pb_fb_blit_blend", header: "playbox/pb.h".}
proc pbFbBlitTile*(dst: ptr PbFb; dx, dy: cint; sheet: ptr PbSheet; tileId: cint) {.importc: "pb_fb_blit_tile", header: "playbox/pb.h".}
proc pbFbPlot*(fb: ptr PbFb; px, py: cint; color: PbColor) {.importc: "pb_fb_plot", header: "playbox/pb.h".}
proc pbFbPlotBlend*(fb: ptr PbFb; px, py: cint; color: PbColor; alpha: cfloat) {.importc: "pb_fb_plot_blend", header: "playbox/pb.h".}
proc pbFbPlotFillCircle*(fb: ptr PbFb; cx, cy, radius: cint; color: PbColor) {.importc: "pb_fb_plot_fill_circle", header: "playbox/pb.h".}
proc pbFbBrailleClear*(fb: ptr PbFb; bg: PbColor) {.importc: "pb_fb_braille_clear", header: "playbox/pb.h".}
proc pbFbBraillePlot*(fb: ptr PbFb; px, py: cint; color: PbColor) {.importc: "pb_fb_braille_plot", header: "playbox/pb.h".}
proc pbFbBraillePlotBlend*(fb: ptr PbFb; px, py: cint; color: PbColor; alpha: cfloat) {.importc: "pb_fb_braille_plot_blend", header: "playbox/pb.h".}
proc pbFbBrailleLine*(fb: ptr PbFb; x0, y0, x1, y1: cint; color: PbColor) {.importc: "pb_fb_braille_line", header: "playbox/pb.h".}
proc pbFbBrailleFillCircle*(fb: ptr PbFb; cx, cy, radius: cint; color: PbColor) {.importc: "pb_fb_braille_fill_circle", header: "playbox/pb.h".}
proc pbFbBrailleFillTriangle*(fb: ptr PbFb; x0, y0, x1, y1, x2, y2: cint; color: PbColor) {.importc: "pb_fb_braille_fill_triangle", header: "playbox/pb.h".}
proc pbFbQuadPlot*(fb: ptr PbFb; px, py: cint; color: PbColor) {.importc: "pb_fb_quad_plot", header: "playbox/pb.h".}
proc pbFbQuadFillCircle*(fb: ptr PbFb; cx, cy, radius: cint; color: PbColor) {.importc: "pb_fb_quad_fill_circle", header: "playbox/pb.h".}
proc pbFbPixel*(fb: ptr PbFb; x, y: cint; color: PbColor) {.importc: "pb_fb_pixel", header: "playbox/pb.h".}
proc pbFbSetCamera*(fb: ptr PbFb; camX, camY: cint) {.importc: "pb_fb_set_camera", header: "playbox/pb.h".}
proc pbFbSetClip*(fb: ptr PbFb; x, y, w, h: cint) {.importc: "pb_fb_set_clip", header: "playbox/pb.h".}
proc pbFbResetClip*(fb: ptr PbFb) {.importc: "pb_fb_reset_clip", header: "playbox/pb.h".}
proc pbFbFillGradientV*(fb: ptr PbFb; x, y, w, h: cint; top, bottom: PbColor) {.importc: "pb_fb_fill_gradient_v", header: "playbox/pb.h".}
proc pbFbFillGradientH*(fb: ptr PbFb; x, y, w, h: cint; left, right: PbColor) {.importc: "pb_fb_fill_gradient_h", header: "playbox/pb.h".}
proc pbFbFillDither*(fb: ptr PbFb; x, y, w, h: cint; a, b: PbColor; pattern: cint) {.importc: "pb_fb_fill_dither", header: "playbox/pb.h".}

proc pbSheetCreate*(cols, rows, tileW, tileH: cint): PbSheet {.importc: "pb_sheet_create", header: "playbox/pb.h".}
proc pbSheetFree*(sheet: ptr PbSheet) {.importc: "pb_sheet_free", header: "playbox/pb.h".}
proc pbSheetSetTile*(sheet: ptr PbSheet; tileId: cint; src: ptr PbFb) {.importc: "pb_sheet_set_tile", header: "playbox/pb.h".}

proc pbParticlesInit*(ps: ptr PbParticles; capacity: cint): cint {.importc: "pb_particles_init", header: "playbox/pb.h".}
proc pbParticlesFree*(ps: ptr PbParticles) {.importc: "pb_particles_free", header: "playbox/pb.h".}
proc pbParticlesEmit*(ps: ptr PbParticles; x, y, vx, vy, life: cfloat; color: PbColor) {.importc: "pb_particles_emit", header: "playbox/pb.h".}
proc pbParticlesUpdate*(ps: ptr PbParticles; dt: cdouble) {.importc: "pb_particles_update", header: "playbox/pb.h".}
proc pbParticlesDrawBraille*(fb: ptr PbFb; ps: ptr PbParticles) {.importc: "pb_particles_draw_braille", header: "playbox/pb.h".}
proc pbParticlesDrawHalf*(fb: ptr PbFb; ps: ptr PbParticles) {.importc: "pb_particles_draw_half", header: "playbox/pb.h".}

const
  PbKeyEsc* = 1.cint
  PbKeyEnter* = 2.cint
  PbKeyBackspace* = 3.cint
  PbKeyTab* = 4.cint
  PbKeyUp* = 5.cint
  PbKeyDown* = 6.cint
  PbKeyLeft* = 7.cint
  PbKeyRight* = 8.cint
  PbKeyHome* = 9.cint
  PbKeyEnd* = 10.cint
  PbKeyF1* = 15.cint
  PbEventNone* = 0.cint
  PbEventKey* = 1.cint
  PbEventText* = 2.cint
  PbEventMouse* = 3.cint
  PbEventResize* = 4.cint
  PbEventQuit* = 5.cint
  PbEventFocus* = 6.cint
  PbStyleBold* = 1.uint16
  PbStyleDim* = 2.uint16
  PbStyleUnderline* = 4.uint16
  PbStyleItalic* = 16.uint16
  PbBoxSingle* = 0.cint
  PbBoxDouble* = 1.cint
  PbBoxRounded* = 2.cint
  PbBoxHeavy* = 3.cint
  PbBoxAscii* = 4.cint
  PbBoxDashed* = 5.cint
  PbBlendReplace* = 0.cint
  PbBlendAlpha* = 1.cint
  PbBlendAdd* = 2.cint
  PbBlendMul* = 3.cint
  PbMouseLeft* = 0.cint

template rgb*(r, g, b: uint8): PbColor = pbRgbEx(r, g, b)
template cell*(ch: uint32; fg, bg: PbColor; style: uint16 = 0): PbCell = pbCellEx(ch, fg, bg, style)

proc version*(): string = $pbVersionString()

# ---------------------------------------------------------------------------
# Idiomatic wrappers
# ---------------------------------------------------------------------------

type
  Framebuffer* = object
    raw*: ptr PbFb

proc width*(fb: Framebuffer): int =
  if fb.raw.isNil: 0 else: fb.raw.w.int

proc height*(fb: Framebuffer): int =
  if fb.raw.isNil: 0 else: fb.raw.h.int

proc put*(fb: Framebuffer; x, y: int; c: PbCell) =
  pbFbPut(fb.raw, x.cint, y.cint, c)

proc putBlend*(fb: Framebuffer; x, y: int; c: PbCell; alpha: float32; mode = PbBlendAlpha) =
  pbFbPutBlend(fb.raw, x.cint, y.cint, c, alpha.cfloat, mode)

proc get*(fb: Framebuffer; x, y: int): PbCell =
  pbFbGet(fb.raw, x.cint, y.cint)

proc clear*(fb: Framebuffer; fill: PbCell) =
  pbFbClear(fb.raw, fill)

proc text*(fb: Framebuffer; x, y: int; s: string; fg, bg: PbColor; style: uint16 = 0) =
  pbFbText(fb.raw, x.cint, y.cint, s.cstring, fg, bg, style)

proc textCentered*(fb: Framebuffer; y: int; s: string; fg, bg: PbColor; style: uint16 = 0) =
  pbFbTextCentered(fb.raw, y.cint, s.cstring, fg, bg, style)

proc textWrap*(fb: Framebuffer; x, y, maxW, maxH: int; s: string; fg, bg: PbColor; style: uint16 = 0): int =
  pbFbTextWrap(fb.raw, x.cint, y.cint, maxW.cint, maxH.cint, s.cstring, fg, bg, style).int

proc textClipped*(fb: Framebuffer; x, y, maxW: int; s: string; fg, bg: PbColor; style: uint16 = 0) =
  pbFbTextClipped(fb.raw, x.cint, y.cint, maxW.cint, s.cstring, fg, bg, style)

proc fillRect*(fb: Framebuffer; x, y, w, h: int; c: PbCell) =
  pbFbFillRect(fb.raw, x.cint, y.cint, w.cint, h.cint, c)

proc fillShade*(fb: Framebuffer; x, y, w, h: int; fg, bg: PbColor; level: int) =
  pbFbFillShade(fb.raw, x.cint, y.cint, w.cint, h.cint, fg, bg, level.cint)

proc line*(fb: Framebuffer; x0, y0, x1, y1: int; c: PbCell) =
  pbFbLine(fb.raw, x0.cint, y0.cint, x1.cint, y1.cint, c)

proc circle*(fb: Framebuffer; cx, cy, r: int; c: PbCell) =
  pbFbCircle(fb.raw, cx.cint, cy.cint, r.cint, c)

proc fillCircle*(fb: Framebuffer; cx, cy, r: int; c: PbCell) =
  pbFbFillCircle(fb.raw, cx.cint, cy.cint, r.cint, c)

proc fillTriangle*(fb: Framebuffer; x0, y0, x1, y1, x2, y2: int; c: PbCell) =
  pbFbFillTriangle(fb.raw, x0.cint, y0.cint, x1.cint, y1.cint, x2.cint, y2.cint, c)

proc boxEx*(fb: Framebuffer; x, y, w, h: int; boxStyle: cint; fg, bg: PbColor; style: uint16 = 0) =
  pbFbBoxEx(fb.raw, x.cint, y.cint, w.cint, h.cint, boxStyle, fg, bg, style)

proc panelEx*(fb: Framebuffer; x, y, w, h: int; title: string; boxStyle: cint;
              border, titleFg, fill: PbColor; style: uint16 = 0) =
  pbFbPanelEx(fb.raw, x.cint, y.cint, w.cint, h.cint, title.cstring, boxStyle, border, titleFg, fill, style)

proc shadow*(fb: Framebuffer; x, y, w, h: int; sh: PbColor; alpha = 0.4'f32) =
  pbFbShadow(fb.raw, x.cint, y.cint, w.cint, h.cint, sh, alpha.cfloat)

proc blitBlend*(fb: Framebuffer; dx, dy: int; src: ptr PbFb; alpha: float32; mode = PbBlendAlpha) =
  pbFbBlitBlend(fb.raw, dx.cint, dy.cint, src, alpha.cfloat, mode)

proc plot*(fb: Framebuffer; px, py: int; color: PbColor) =
  pbFbPlot(fb.raw, px.cint, py.cint, color)

proc plotBlend*(fb: Framebuffer; px, py: int; color: PbColor; alpha: float32) =
  pbFbPlotBlend(fb.raw, px.cint, py.cint, color, alpha.cfloat)

proc braillePlot*(fb: Framebuffer; px, py: int; color: PbColor) =
  pbFbBraillePlot(fb.raw, px.cint, py.cint, color)

proc brailleFillCircle*(fb: Framebuffer; cx, cy, r: int; color: PbColor) =
  pbFbBrailleFillCircle(fb.raw, cx.cint, cy.cint, r.cint, color)

proc brailleFillTriangle*(fb: Framebuffer; x0, y0, x1, y1, x2, y2: int; color: PbColor) =
  pbFbBrailleFillTriangle(fb.raw, x0.cint, y0.cint, x1.cint, y1.cint, x2.cint, y2.cint, color)

proc quadFillCircle*(fb: Framebuffer; cx, cy, r: int; color: PbColor) =
  pbFbQuadFillCircle(fb.raw, cx.cint, cy.cint, r.cint, color)

proc setCamera*(fb: Framebuffer; x, y: int) =
  pbFbSetCamera(fb.raw, x.cint, y.cint)

proc setClip*(fb: Framebuffer; x, y, w, h: int) =
  pbFbSetClip(fb.raw, x.cint, y.cint, w.cint, h.cint)

proc resetClip*(fb: Framebuffer) =
  pbFbResetClip(fb.raw)

proc fillGradientV*(fb: Framebuffer; x, y, w, h: int; top, bottom: PbColor) =
  pbFbFillGradientV(fb.raw, x.cint, y.cint, w.cint, h.cint, top, bottom)

proc fillGradientH*(fb: Framebuffer; x, y, w, h: int; left, right: PbColor) =
  pbFbFillGradientH(fb.raw, x.cint, y.cint, w.cint, h.cint, left, right)

type
  App* = ref object
    raw: ptr PbApp
    title: string
    onInit*: proc (app: App)
    onEvent*: proc (app: App; ev: ptr PbEvent)
    onUpdate*: proc (app: App; dt: float)
    onDraw*: proc (app: App; fb: Framebuffer)
    onShutdown*: proc (app: App)

var currentApp: App

proc trampInit(a: ptr PbApp; u: pointer) {.cdecl.} =
  discard a
  discard u
  if currentApp != nil and currentApp.onInit != nil:
    currentApp.onInit(currentApp)

proc trampEvent(a: ptr PbApp; u: pointer; ev: ptr PbEvent) {.cdecl.} =
  discard a
  discard u
  if currentApp != nil and currentApp.onEvent != nil and ev != nil:
    currentApp.onEvent(currentApp, ev)

proc trampUpdate(a: ptr PbApp; u: pointer; dt: cdouble) {.cdecl.} =
  discard a
  discard u
  if currentApp != nil and currentApp.onUpdate != nil:
    currentApp.onUpdate(currentApp, dt.float)

proc trampDraw(a: ptr PbApp; u: pointer; fb: ptr PbFb) {.cdecl.} =
  discard a
  discard u
  if currentApp != nil and currentApp.onDraw != nil and fb != nil:
    currentApp.onDraw(currentApp, Framebuffer(raw: fb))

proc trampShutdown(a: ptr PbApp; u: pointer) {.cdecl.} =
  discard a
  discard u
  if currentApp != nil and currentApp.onShutdown != nil:
    currentApp.onShutdown(currentApp)

proc newApp*(title: string; targetFps = 60): App =
  result = App(title: title)
  var desc: PbAppDesc
  zeroMem(addr desc, sizeof(desc))
  desc.title = result.title.cstring
  desc.target_fps = targetFps.cint
  desc.on_init = trampInit
  desc.on_event = trampEvent
  desc.on_update = trampUpdate
  desc.on_draw = trampDraw
  desc.on_shutdown = trampShutdown
  result.raw = pbAppCreate(addr desc, nil)
  if result.raw.isNil:
    raise newException(IOError, "pb_app_create failed")

proc destroy*(app: App) =
  if app.raw != nil:
    pbAppDestroy(app.raw)
    app.raw = nil

proc run*(app: App): int =
  currentApp = app
  defer: currentApp = nil
  pbAppRun(app.raw).int

proc quit*(app: App) = pbAppQuit(app.raw)
proc width*(app: App): int = pbAppWidth(app.raw).int
proc height*(app: App): int = pbAppHeight(app.raw).int
proc fps*(app: App): int = pbGetFps(app.raw).int
proc frameTime*(app: App): float = pbGetFrameTime(app.raw).float

proc setTitle*(app: App; t: string) =
  app.title = t
  pbAppSetTitle(app.raw, app.title.cstring)

proc setClear*(app: App; c: PbCell) = pbAppSetClear(app.raw, c)
proc setTargetFps*(app: App; fps: int) = pbAppSetTargetFps(app.raw, fps.cint)

proc isKeyDown*(app: App; key: cint): bool = pbIsKeyDown(app.raw, key) != 0
proc isKeyPressed*(app: App; key: cint): bool = pbIsKeyPressed(app.raw, key) != 0
proc isKeyReleased*(app: App; key: cint): bool = pbIsKeyReleased(app.raw, key) != 0
proc isCharDown*(app: App; cp: uint32): bool = pbIsCharDown(app.raw, cp) != 0
proc isCharPressed*(app: App; cp: uint32): bool = pbIsCharPressed(app.raw, cp) != 0
proc mouseX*(app: App): int = pbGetMouseX(app.raw).int
proc mouseY*(app: App): int = pbGetMouseY(app.raw).int
proc isMouseDown*(app: App; button: cint): bool = pbIsMouseButtonDown(app.raw, button) != 0
proc mouseWheel*(app: App): int = pbGetMouseWheel(app.raw).int
proc focused*(app: App): bool = pbIsFocused(app.raw) != 0
proc isReplay*(app: App): bool = pbAppIsReplay(app.raw) != 0
proc replaySeed*(app: App): uint32 = pbAppReplaySeed(app.raw)

type
  Particles* = ref object
    ps: PbParticles

proc newParticles*(capacity = 256): Particles =
  result = Particles()
  if pbParticlesInit(addr result.ps, capacity.cint) == 0:
    raise newException(IOError, "particles init failed")

proc destroy*(p: Particles) =
  pbParticlesFree(addr p.ps)

proc emit*(p: Particles; x, y, vx, vy, life: float32; color: PbColor) =
  pbParticlesEmit(addr p.ps, x.cfloat, y.cfloat, vx.cfloat, vy.cfloat, life.cfloat, color)

proc update*(p: Particles; dt: float) =
  pbParticlesUpdate(addr p.ps, dt.cdouble)

proc drawBraille*(p: Particles; fb: Framebuffer) =
  pbParticlesDrawBraille(fb.raw, addr p.ps)

proc drawHalf*(p: Particles; fb: Framebuffer) =
  pbParticlesDrawHalf(fb.raw, addr p.ps)
