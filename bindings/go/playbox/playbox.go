// Package playbox is a cgo binding for PlayboxLib.
//
// Call Destroy() when finished with an App (or rely on the finalizer).
// The finalizer is a safety net; prefer explicit Destroy for deterministic cleanup.
package playbox

/*
#cgo CFLAGS: -I${SRCDIR}/../../../include
#cgo LDFLAGS: -L${SRCDIR}/../../../build/lib -lplaybox -lm -Wl,-rpath,${SRCDIR}/../../../build/lib
#include "playbox/pb.h"
#include <stdlib.h>
#include <string.h>

extern void goOnInit(void* user);
extern void goOnEvent(void* user, pb_event* ev);
extern void goOnUpdate(void* user, double dt);
extern void goOnDraw(void* user, pb_fb* fb);
extern void goOnShutdown(void* user);

static void c_on_init(pb_app* a, void* user) { (void)a; goOnInit(user); }
static void c_on_event(pb_app* a, void* user, const pb_event* ev) { (void)a; goOnEvent(user, (pb_event*)ev); }
static void c_on_update(pb_app* a, void* user, double dt) { (void)a; goOnUpdate(user, dt); }
static void c_on_draw(pb_app* a, void* user, pb_fb* fb) { (void)a; goOnDraw(user, fb); }
static void c_on_shutdown(pb_app* a, void* user) { (void)a; goOnShutdown(user); }

static pb_app* pb_go_create(const char* title, int fps, void* user) {
    pb_app_desc d;
    memset(&d, 0, sizeof(d));
    d.title = title;
    d.target_fps = fps;
    d.on_init = c_on_init;
    d.on_event = c_on_event;
    d.on_update = c_on_update;
    d.on_draw = c_on_draw;
    d.on_shutdown = c_on_shutdown;
    return pb_app_create(&d, user);
}

// cgo represents anonymous unions as opaque byte arrays; extract via helpers.
static pb_key_event pb_go_ev_key(const pb_event* e) { return e->as.key; }
static uint32_t pb_go_ev_text(const pb_event* e) { return e->as.text; }
static pb_mouse_event pb_go_ev_mouse(const pb_event* e) { return e->as.mouse; }
static pb_resize_event pb_go_ev_resize(const pb_event* e) { return e->as.resize; }
static pb_focus_event pb_go_ev_focus(const pb_event* e) { return e->as.focus; }
*/
import "C"
import (
	"runtime"
	"runtime/cgo"
	"unsafe"
)

// Color is an RGB triple.
type Color struct{ R, G, B uint8 }

// Cell is one terminal cell.
type Cell struct {
	Ch     uint32
	Fg, Bg Color
	Style  uint16
}

// Framebuffer wraps a live pb_fb pointer (never a copy of the struct).
type Framebuffer struct{ raw *C.pb_fb }

// App is a Playbox application.
//
// Destroy must be called when finished. A finalizer also calls Destroy as a
// safety net, but prefer explicit Destroy so resources are released promptly.
type App struct {
	raw        *C.pb_app
	title      *C.char
	handle     cgo.Handle
	OnInit     func(*App)
	OnEvent    func(*App, Event)
	OnUpdate   func(*App, float64)
	OnDraw     func(*App, *Framebuffer)
	OnShutdown func(*App)
}

// Event is a typed input/window event. Use Key/Mouse/Resize/Focus accessors.
type Event struct {
	Type int
	raw  C.pb_event
}

// KeyEvent payload for EventKey.
type KeyEvent struct {
	Key                 int
	Codepoint           uint32
	Alt, Ctrl, Shift    bool
	Pressed             bool
}

// MouseEvent payload for EventMouse.
type MouseEvent struct {
	X, Y             int
	Button           uint8
	Pressed          bool
	Wheel            int
	Shift, Alt, Ctrl bool
}

// ResizeEvent payload for EventResize.
type ResizeEvent struct {
	Width, Height int
}

// FocusEvent payload for EventFocus.
type FocusEvent struct {
	Focused bool
}

// Style / key / event / box / blend constants.
const (
	StyleNone          = C.PB_STYLE_NONE
	StyleBold          = C.PB_STYLE_BOLD
	StyleDim           = C.PB_STYLE_DIM
	StyleUnderline     = C.PB_STYLE_UNDERLINE
	StyleReverse       = C.PB_STYLE_REVERSE
	StyleItalic        = C.PB_STYLE_ITALIC
	StyleStrikethrough = C.PB_STYLE_STRIKETHROUGH

	KeyNone      = C.PB_KEY_NONE
	KeyEsc       = C.PB_KEY_ESC
	KeyEnter     = C.PB_KEY_ENTER
	KeyBackspace = C.PB_KEY_BACKSPACE
	KeyTab       = C.PB_KEY_TAB
	KeyUp        = C.PB_KEY_UP
	KeyDown      = C.PB_KEY_DOWN
	KeyLeft      = C.PB_KEY_LEFT
	KeyRight     = C.PB_KEY_RIGHT
	KeyHome      = C.PB_KEY_HOME
	KeyEnd       = C.PB_KEY_END
	KeyPgUp      = C.PB_KEY_PGUP
	KeyPgDn      = C.PB_KEY_PGDN
	KeyIns       = C.PB_KEY_INS
	KeyDel       = C.PB_KEY_DEL
	KeyF1        = C.PB_KEY_F1
	KeyF2        = C.PB_KEY_F2
	KeyF3        = C.PB_KEY_F3
	KeyF4        = C.PB_KEY_F4
	KeyF5        = C.PB_KEY_F5
	KeyF6        = C.PB_KEY_F6
	KeyF7        = C.PB_KEY_F7
	KeyF8        = C.PB_KEY_F8
	KeyF9        = C.PB_KEY_F9
	KeyF10       = C.PB_KEY_F10
	KeyF11       = C.PB_KEY_F11
	KeyF12       = C.PB_KEY_F12

	MouseLeft   = C.PB_MOUSE_LEFT
	MouseMiddle = C.PB_MOUSE_MIDDLE
	MouseRight  = C.PB_MOUSE_RIGHT

	EventNone   = C.PB_EVENT_NONE
	EventKey    = C.PB_EVENT_KEY
	EventText   = C.PB_EVENT_TEXT
	EventMouse  = C.PB_EVENT_MOUSE
	EventResize = C.PB_EVENT_RESIZE
	EventQuit   = C.PB_EVENT_QUIT
	EventFocus  = C.PB_EVENT_FOCUS

	BoxSingle  = C.PB_BOX_SINGLE
	BoxDouble  = C.PB_BOX_DOUBLE
	BoxRounded = C.PB_BOX_ROUNDED
	BoxHeavy   = C.PB_BOX_HEAVY
	BoxASCII   = C.PB_BOX_ASCII
	BoxDashed  = C.PB_BOX_DASHED

	BlendReplace = C.PB_BLEND_REPLACE
	BlendAlpha   = C.PB_BLEND_ALPHA
	BlendAdd     = C.PB_BLEND_ADD
	BlendMul     = C.PB_BLEND_MUL
)

func cColor(c Color) C.pb_color {
	return C.pb_rgb_ex(C.uchar(c.R), C.uchar(c.G), C.uchar(c.B))
}

func cCell(c Cell) C.pb_cell {
	return C.pb_cell_ex(C.uint(c.Ch), cColor(c.Fg), cColor(c.Bg), C.ushort(c.Style))
}

func goColor(c C.pb_color) Color {
	return Color{R: uint8(c.r), G: uint8(c.g), B: uint8(c.b)}
}

func goCell(c C.pb_cell) Cell {
	return Cell{Ch: uint32(c.ch), Fg: goColor(c.fg), Bg: goColor(c.bg), Style: uint16(c.style)}
}

// RGB constructs a Color.
func RGB(r, g, b uint8) Color { return Color{r, g, b} }

// MakeCell constructs a Cell.
func MakeCell(ch uint32, fg, bg Color, style uint16) Cell {
	return Cell{Ch: ch, Fg: fg, Bg: bg, Style: style}
}

// Version returns the library version string.
func Version() string {
	return C.GoString(C.pb_version_string())
}

// MeasureText returns display width of a UTF-8 string in cells.
func MeasureText(s string) int {
	cs := C.CString(s)
	defer C.free(unsafe.Pointer(cs))
	return int(C.pb_fb_measure_text(cs))
}

// ---------------------------------------------------------------------------
// Event accessors
// ---------------------------------------------------------------------------

// Key returns the key payload (valid when Type == EventKey).
func (e Event) Key() KeyEvent {
	k := C.pb_go_ev_key(&e.raw)
	return KeyEvent{
		Key:       int(k.key),
		Codepoint: uint32(k.codepoint),
		Alt:       k.alt != 0,
		Ctrl:      k.ctrl != 0,
		Shift:     k.shift != 0,
		Pressed:   k.pressed != 0,
	}
}

// Text returns the Unicode codepoint (valid when Type == EventText).
func (e Event) Text() uint32 {
	return uint32(C.pb_go_ev_text(&e.raw))
}

// Mouse returns the mouse payload (valid when Type == EventMouse).
func (e Event) Mouse() MouseEvent {
	m := C.pb_go_ev_mouse(&e.raw)
	return MouseEvent{
		X: int(m.x), Y: int(m.y),
		Button:  uint8(m.button),
		Pressed: m.pressed != 0,
		Wheel:   int(m.wheel),
		Shift:   m.shift != 0,
		Alt:     m.alt != 0,
		Ctrl:    m.ctrl != 0,
	}
}

// Resize returns the resize payload (valid when Type == EventResize).
func (e Event) Resize() ResizeEvent {
	r := C.pb_go_ev_resize(&e.raw)
	return ResizeEvent{Width: int(r.width), Height: int(r.height)}
}

// Focus returns the focus payload (valid when Type == EventFocus).
func (e Event) Focus() FocusEvent {
	f := C.pb_go_ev_focus(&e.raw)
	return FocusEvent{Focused: f.focused != 0}
}

// ---------------------------------------------------------------------------
// App
// ---------------------------------------------------------------------------

// NewApp creates an application. Caller should Destroy it when done.
func NewApp(title string, fps int) *App {
	a := &App{}
	a.handle = cgo.NewHandle(a)
	a.title = C.CString(title)
	a.raw = C.pb_go_create(a.title, C.int(fps), unsafe.Pointer(a.handle))
	if a.raw == nil {
		C.free(unsafe.Pointer(a.title))
		a.handle.Delete()
		panic("pb_app_create failed")
	}
	runtime.SetFinalizer(a, (*App).Destroy)
	return a
}

// Run enters the main loop.
func (a *App) Run() int {
	return int(C.pb_app_run(a.raw))
}

// Quit requests the main loop to exit.
func (a *App) Quit() { C.pb_app_quit(a.raw) }

// Destroy frees the app. Safe to call more than once.
// Prefer explicit Destroy over relying on the finalizer alone.
func (a *App) Destroy() {
	if a == nil {
		return
	}
	runtime.SetFinalizer(a, nil)
	if a.raw != nil {
		C.pb_app_destroy(a.raw)
		a.raw = nil
	}
	if a.title != nil {
		C.free(unsafe.Pointer(a.title))
		a.title = nil
	}
	if a.handle != 0 {
		a.handle.Delete()
		a.handle = 0
	}
}

func (a *App) Width() int  { return int(C.pb_app_width(a.raw)) }
func (a *App) Height() int { return int(C.pb_app_height(a.raw)) }
func (a *App) FPS() int    { return int(C.pb_get_fps(a.raw)) }
func (a *App) FrameTime() float64 {
	return float64(C.pb_get_frame_time(a.raw))
}
func (a *App) Focused() bool { return C.pb_is_focused(a.raw) != 0 }

func (a *App) SetTitle(title string) {
	if a.title != nil {
		C.free(unsafe.Pointer(a.title))
	}
	a.title = C.CString(title)
	C.pb_app_set_title(a.raw, a.title)
}

func (a *App) SetClear(c Cell) {
	C.pb_app_set_clear(a.raw, cCell(c))
}

func (a *App) SetTargetFPS(fps int) {
	C.pb_app_set_target_fps(a.raw, C.int(fps))
}

func (a *App) RequestResize() { C.pb_app_request_resize(a.raw) }

func (a *App) IsKeyDown(key int) bool {
	return C.pb_is_key_down(a.raw, C.pb_key(key)) != 0
}
func (a *App) IsKeyPressed(key int) bool {
	return C.pb_is_key_pressed(a.raw, C.pb_key(key)) != 0
}
func (a *App) IsKeyReleased(key int) bool {
	return C.pb_is_key_released(a.raw, C.pb_key(key)) != 0
}
func (a *App) IsCharDown(cp uint32) bool {
	return C.pb_is_char_down(a.raw, C.uint(cp)) != 0
}
func (a *App) IsCharPressed(cp uint32) bool {
	return C.pb_is_char_pressed(a.raw, C.uint(cp)) != 0
}

func (a *App) MouseX() int { return int(C.pb_get_mouse_x(a.raw)) }
func (a *App) MouseY() int { return int(C.pb_get_mouse_y(a.raw)) }
func (a *App) Mouse() (int, int) {
	return a.MouseX(), a.MouseY()
}
func (a *App) IsMouseDown(button int) bool {
	return C.pb_is_mouse_button_down(a.raw, C.int(button)) != 0
}
func (a *App) IsMousePressed(button int) bool {
	return C.pb_is_mouse_button_pressed(a.raw, C.int(button)) != 0
}
func (a *App) IsMouseReleased(button int) bool {
	return C.pb_is_mouse_button_released(a.raw, C.int(button)) != 0
}
func (a *App) MouseWheel() int { return int(C.pb_get_mouse_wheel(a.raw)) }

func (a *App) IsReplay() bool     { return C.pb_app_is_replay(a.raw) != 0 }
func (a *App) IsRecording() bool  { return C.pb_app_is_recording(a.raw) != 0 }

// ---------------------------------------------------------------------------
// Framebuffer
// ---------------------------------------------------------------------------

func (f *Framebuffer) W() int {
	if f == nil || f.raw == nil {
		return 0
	}
	return int(f.raw.w)
}
func (f *Framebuffer) H() int {
	if f == nil || f.raw == nil {
		return 0
	}
	return int(f.raw.h)
}

func (f *Framebuffer) Clear(c Cell) {
	C.pb_fb_clear(f.raw, cCell(c))
}

func (f *Framebuffer) Put(x, y int, c Cell) {
	C.pb_fb_put(f.raw, C.int(x), C.int(y), cCell(c))
}

func (f *Framebuffer) PutBlend(x, y int, c Cell, alpha float32, mode int) {
	C.pb_fb_put_blend(f.raw, C.int(x), C.int(y), cCell(c), C.float(alpha), C.pb_blend_mode(mode))
}

func (f *Framebuffer) Get(x, y int) Cell {
	return goCell(C.pb_fb_get(f.raw, C.int(x), C.int(y)))
}

func (f *Framebuffer) Text(x, y int, s string, fg, bg Color, style uint16) {
	cs := C.CString(s)
	defer C.free(unsafe.Pointer(cs))
	C.pb_fb_text(f.raw, C.int(x), C.int(y), cs, cColor(fg), cColor(bg), C.ushort(style))
}

func (f *Framebuffer) TextCentered(y int, s string, fg, bg Color, style uint16) {
	cs := C.CString(s)
	defer C.free(unsafe.Pointer(cs))
	C.pb_fb_text_centered(f.raw, C.int(y), cs, cColor(fg), cColor(bg), C.ushort(style))
}

func (f *Framebuffer) TextWrap(x, y, maxW, maxH int, s string, fg, bg Color, style uint16) int {
	cs := C.CString(s)
	defer C.free(unsafe.Pointer(cs))
	return int(C.pb_fb_text_wrap(f.raw, C.int(x), C.int(y), C.int(maxW), C.int(maxH),
		cs, cColor(fg), cColor(bg), C.ushort(style)))
}

func (f *Framebuffer) TextClipped(x, y, maxW int, s string, fg, bg Color, style uint16) {
	cs := C.CString(s)
	defer C.free(unsafe.Pointer(cs))
	C.pb_fb_text_clipped(f.raw, C.int(x), C.int(y), C.int(maxW),
		cs, cColor(fg), cColor(bg), C.ushort(style))
}

func (f *Framebuffer) FillRect(x, y, w, h int, c Cell) {
	C.pb_fb_fill_rect(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cCell(c))
}

func (f *Framebuffer) Box(x, y, w, h int, fg, bg Color, style uint16) {
	C.pb_fb_box(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cColor(fg), cColor(bg), C.ushort(style))
}

func (f *Framebuffer) BoxDouble(x, y, w, h int, fg, bg Color, style uint16) {
	C.pb_fb_box_double(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cColor(fg), cColor(bg), C.ushort(style))
}

func (f *Framebuffer) BoxEx(x, y, w, h int, boxStyle int, fg, bg Color, style uint16) {
	C.pb_fb_box_ex(f.raw, C.int(x), C.int(y), C.int(w), C.int(h),
		C.pb_box_style(boxStyle), cColor(fg), cColor(bg), C.ushort(style))
}

func (f *Framebuffer) Panel(x, y, w, h int, title string, border, titleFg, fill Color, style uint16) {
	cs := C.CString(title)
	defer C.free(unsafe.Pointer(cs))
	C.pb_fb_panel(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cs,
		cColor(border), cColor(titleFg), cColor(fill), C.ushort(style))
}

func (f *Framebuffer) PanelEx(x, y, w, h int, title string, boxStyle int, border, titleFg, fill Color, style uint16) {
	cs := C.CString(title)
	defer C.free(unsafe.Pointer(cs))
	C.pb_fb_panel_ex(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cs,
		C.pb_box_style(boxStyle), cColor(border), cColor(titleFg), cColor(fill), C.ushort(style))
}

func (f *Framebuffer) Shadow(x, y, w, h int, shadow Color, alpha float32) {
	C.pb_fb_shadow(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cColor(shadow), C.float(alpha))
}

func (f *Framebuffer) HLine(x, y, w int, c Cell) {
	C.pb_fb_hline(f.raw, C.int(x), C.int(y), C.int(w), cCell(c))
}
func (f *Framebuffer) VLine(x, y, h int, c Cell) {
	C.pb_fb_vline(f.raw, C.int(x), C.int(y), C.int(h), cCell(c))
}
func (f *Framebuffer) Line(x0, y0, x1, y1 int, c Cell) {
	C.pb_fb_line(f.raw, C.int(x0), C.int(y0), C.int(x1), C.int(y1), cCell(c))
}
func (f *Framebuffer) Circle(cx, cy, r int, c Cell) {
	C.pb_fb_circle(f.raw, C.int(cx), C.int(cy), C.int(r), cCell(c))
}
func (f *Framebuffer) FillCircle(cx, cy, r int, c Cell) {
	C.pb_fb_fill_circle(f.raw, C.int(cx), C.int(cy), C.int(r), cCell(c))
}
func (f *Framebuffer) FillTriangle(x0, y0, x1, y1, x2, y2 int, c Cell) {
	C.pb_fb_fill_triangle(f.raw, C.int(x0), C.int(y0), C.int(x1), C.int(y1), C.int(x2), C.int(y2), cCell(c))
}

func (f *Framebuffer) Blit(dx, dy int, src *Framebuffer) {
	C.pb_fb_blit(f.raw, C.int(dx), C.int(dy), src.raw)
}
func (f *Framebuffer) BlitBlend(dx, dy int, src *Framebuffer, alpha float32, mode int) {
	C.pb_fb_blit_blend(f.raw, C.int(dx), C.int(dy), src.raw, C.float(alpha), C.pb_blend_mode(mode))
}
func (f *Framebuffer) BlitMasked(dx, dy int, src *Framebuffer, transparent uint32) {
	C.pb_fb_blit_masked(f.raw, C.int(dx), C.int(dy), src.raw, C.uint(transparent))
}

func (f *Framebuffer) Plot(px, py int, col Color) {
	C.pb_fb_plot(f.raw, C.int(px), C.int(py), cColor(col))
}
func (f *Framebuffer) PlotBlend(px, py int, col Color, alpha float32) {
	C.pb_fb_plot_blend(f.raw, C.int(px), C.int(py), cColor(col), C.float(alpha))
}
func (f *Framebuffer) PlotLine(x0, y0, x1, y1 int, col Color) {
	C.pb_fb_plot_line(f.raw, C.int(x0), C.int(y0), C.int(x1), C.int(y1), cColor(col))
}
func (f *Framebuffer) PlotFillCircle(cx, cy, r int, col Color) {
	C.pb_fb_plot_fill_circle(f.raw, C.int(cx), C.int(cy), C.int(r), cColor(col))
}

func (f *Framebuffer) BrailleClear(bg Color) {
	C.pb_fb_braille_clear(f.raw, cColor(bg))
}
func (f *Framebuffer) BraillePlot(px, py int, col Color) {
	C.pb_fb_braille_plot(f.raw, C.int(px), C.int(py), cColor(col))
}
func (f *Framebuffer) BraillePlotBlend(px, py int, col Color, alpha float32) {
	C.pb_fb_braille_plot_blend(f.raw, C.int(px), C.int(py), cColor(col), C.float(alpha))
}
func (f *Framebuffer) BrailleLine(x0, y0, x1, y1 int, col Color) {
	C.pb_fb_braille_line(f.raw, C.int(x0), C.int(y0), C.int(x1), C.int(y1), cColor(col))
}
func (f *Framebuffer) BrailleFillRect(x, y, w, h int, col Color) {
	C.pb_fb_braille_fill_rect(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cColor(col))
}
func (f *Framebuffer) BrailleFillCircle(cx, cy, r int, col Color) {
	C.pb_fb_braille_fill_circle(f.raw, C.int(cx), C.int(cy), C.int(r), cColor(col))
}
func (f *Framebuffer) BrailleCircle(cx, cy, r int, col Color) {
	C.pb_fb_braille_circle(f.raw, C.int(cx), C.int(cy), C.int(r), cColor(col))
}
func (f *Framebuffer) BrailleFillTriangle(x0, y0, x1, y1, x2, y2 int, col Color) {
	C.pb_fb_braille_fill_triangle(f.raw, C.int(x0), C.int(y0), C.int(x1), C.int(y1), C.int(x2), C.int(y2), cColor(col))
}

func (f *Framebuffer) QuadPlot(px, py int, col Color) {
	C.pb_fb_quad_plot(f.raw, C.int(px), C.int(py), cColor(col))
}
func (f *Framebuffer) QuadFillRect(x, y, w, h int, col Color) {
	C.pb_fb_quad_fill_rect(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cColor(col))
}
func (f *Framebuffer) QuadFillCircle(cx, cy, r int, col Color) {
	C.pb_fb_quad_fill_circle(f.raw, C.int(cx), C.int(cy), C.int(r), cColor(col))
}

func (f *Framebuffer) Pixel(x, y int, col Color) {
	C.pb_fb_pixel(f.raw, C.int(x), C.int(y), cColor(col))
}
func (f *Framebuffer) FillShade(x, y, w, h int, fg, bg Color, level int) {
	C.pb_fb_fill_shade(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cColor(fg), cColor(bg), C.int(level))
}
func (f *Framebuffer) FillGradientV(x, y, w, h int, top, bottom Color) {
	C.pb_fb_fill_gradient_v(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cColor(top), cColor(bottom))
}
func (f *Framebuffer) FillGradientH(x, y, w, h int, left, right Color) {
	C.pb_fb_fill_gradient_h(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cColor(left), cColor(right))
}
func (f *Framebuffer) FillDither(x, y, w, h int, a, b Color, pattern int) {
	C.pb_fb_fill_dither(f.raw, C.int(x), C.int(y), C.int(w), C.int(h), cColor(a), cColor(b), C.int(pattern))
}

func (f *Framebuffer) SetCamera(x, y int) {
	C.pb_fb_set_camera(f.raw, C.int(x), C.int(y))
}
func (f *Framebuffer) GetCamera() (int, int) {
	var x, y C.int
	C.pb_fb_get_camera(f.raw, &x, &y)
	return int(x), int(y)
}
func (f *Framebuffer) SetClip(x, y, w, h int) {
	C.pb_fb_set_clip(f.raw, C.int(x), C.int(y), C.int(w), C.int(h))
}
func (f *Framebuffer) ResetClip() {
	C.pb_fb_reset_clip(f.raw)
}

// ---------------------------------------------------------------------------
// Particles
// ---------------------------------------------------------------------------

// Particles is a lightweight braille/half-block particle system.
type Particles struct {
	raw C.pb_particles
}

// NewParticles allocates a particle pool.
func NewParticles(capacity int) *Particles {
	p := &Particles{}
	if C.pb_particles_init(&p.raw, C.int(capacity)) == 0 {
		panic("pb_particles_init failed")
	}
	runtime.SetFinalizer(p, (*Particles).Destroy)
	return p
}

// Destroy frees the particle pool. Prefer explicit Destroy over the finalizer.
func (p *Particles) Destroy() {
	if p == nil {
		return
	}
	runtime.SetFinalizer(p, nil)
	C.pb_particles_free(&p.raw)
}

func (p *Particles) Emit(x, y, vx, vy, life float32, col Color) {
	C.pb_particles_emit(&p.raw, C.float(x), C.float(y), C.float(vx), C.float(vy), C.float(life), cColor(col))
}
func (p *Particles) Update(dt float64) {
	C.pb_particles_update(&p.raw, C.double(dt))
}
func (p *Particles) DrawBraille(fb *Framebuffer) {
	C.pb_particles_draw_braille(fb.raw, &p.raw)
}
func (p *Particles) DrawHalf(fb *Framebuffer) {
	C.pb_particles_draw_half(fb.raw, &p.raw)
}

// ---------------------------------------------------------------------------
// CGO trampolines
// ---------------------------------------------------------------------------

//export goOnInit
func goOnInit(user unsafe.Pointer) {
	a := cgo.Handle(user).Value().(*App)
	if a.OnInit != nil {
		a.OnInit(a)
	}
}

//export goOnEvent
func goOnEvent(user unsafe.Pointer, ev *C.pb_event) {
	a := cgo.Handle(user).Value().(*App)
	if a.OnEvent != nil {
		// cgo renames C field `type` to `_type`
		a.OnEvent(a, Event{Type: int(ev._type), raw: *ev})
	}
}

//export goOnUpdate
func goOnUpdate(user unsafe.Pointer, dt C.double) {
	a := cgo.Handle(user).Value().(*App)
	if a.OnUpdate != nil {
		a.OnUpdate(a, float64(dt))
	}
}

//export goOnDraw
func goOnDraw(user unsafe.Pointer, fb *C.pb_fb) {
	a := cgo.Handle(user).Value().(*App)
	if a.OnDraw != nil {
		a.OnDraw(a, &Framebuffer{raw: fb})
	}
}

//export goOnShutdown
func goOnShutdown(user unsafe.Pointer) {
	a := cgo.Handle(user).Value().(*App)
	if a.OnShutdown != nil {
		a.OnShutdown(a)
	}
}
