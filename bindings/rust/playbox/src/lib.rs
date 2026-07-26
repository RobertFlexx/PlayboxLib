//! Safe idiomatic Rust API for PlayboxLib.
//!
//! Thin wrappers over [`playbox_sys`] with owned resources, typed events, and
//! the take/restore callback pattern so hooks can call `&mut App` methods.

use playbox_sys as sys;
use std::cell::RefCell;
use std::ffi::{CStr, CString};
use std::os::raw::{c_double, c_int, c_void};
use std::ptr;

/* ---- Re-export all constants ---- */

pub use sys::{
    PB_APP_FLAG_CUSTOM_CLEAR, PB_APP_FLAG_NO_AUTO_CLEAR, PB_APP_FLAG_NO_FOCUS,
    PB_APP_FLAG_NO_MOUSE, PB_BLEND_ADD, PB_BLEND_ALPHA, PB_BLEND_MUL, PB_BLEND_REPLACE,
    PB_BOX_ASCII, PB_BOX_DASHED, PB_BOX_DOUBLE, PB_BOX_HEAVY, PB_BOX_ROUNDED, PB_BOX_SINGLE,
    PB_EVENT_FOCUS, PB_EVENT_KEY, PB_EVENT_MOUSE, PB_EVENT_NONE, PB_EVENT_QUIT, PB_EVENT_RESIZE,
    PB_EVENT_TEXT, PB_KEY_BACKSPACE, PB_KEY_COUNT, PB_KEY_DEL, PB_KEY_DOWN, PB_KEY_END,
    PB_KEY_ENTER, PB_KEY_ESC, PB_KEY_F1, PB_KEY_F10, PB_KEY_F11, PB_KEY_F12, PB_KEY_F2,
    PB_KEY_F3, PB_KEY_F4, PB_KEY_F5, PB_KEY_F6, PB_KEY_F7, PB_KEY_F8, PB_KEY_F9, PB_KEY_HOME,
    PB_KEY_INS, PB_KEY_LEFT, PB_KEY_NONE, PB_KEY_PGDN, PB_KEY_PGUP, PB_KEY_RIGHT, PB_KEY_TAB,
    PB_KEY_UP, PB_MOUSE_BUTTON_COUNT, PB_MOUSE_LEFT, PB_MOUSE_MIDDLE, PB_MOUSE_RIGHT,
    PB_STYLE_BOLD, PB_STYLE_DIM, PB_STYLE_ITALIC, PB_STYLE_NONE, PB_STYLE_REVERSE,
    PB_STYLE_STRIKETHROUGH, PB_STYLE_UNDERLINE, PB_VERSION_MAJOR, PB_VERSION_MINOR,
    PB_VERSION_PATCH, PB_VERSION_STRING,
};

thread_local! {
    static CURRENT: RefCell<*mut App> = const { RefCell::new(ptr::null_mut()) };
}

/* ---- Color ---- */

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Color(pub sys::pb_color);

impl Default for Color {
    fn default() -> Self {
        Color::rgb(0, 0, 0)
    }
}

impl Color {
    #[inline]
    pub fn rgb(r: u8, g: u8, b: u8) -> Self {
        Color(sys::pb_color { r, g, b })
    }

    #[inline]
    pub fn from_raw(c: sys::pb_color) -> Self {
        Color(c)
    }

    #[inline]
    pub fn raw(self) -> sys::pb_color {
        self.0
    }

    pub fn fade(self, alpha: f32) -> Self {
        let a = alpha.clamp(0.0, 1.0);
        Color(sys::pb_color {
            r: ((self.0.r as f32) * a + 0.5) as u8,
            g: ((self.0.g as f32) * a + 0.5) as u8,
            b: ((self.0.b as f32) * a + 0.5) as u8,
        })
    }

    pub fn lerp(a: Color, b: Color, t: f32) -> Self {
        let t = t.clamp(0.0, 1.0);
        Color(sys::pb_color {
            r: (a.0.r as f32 + (b.0.r as f32 - a.0.r as f32) * t + 0.5) as u8,
            g: (a.0.g as f32 + (b.0.g as f32 - a.0.g as f32) * t + 0.5) as u8,
            b: (a.0.b as f32 + (b.0.b as f32 - a.0.b as f32) * t + 0.5) as u8,
        })
    }
}

/* ---- Cell ---- */

#[derive(Clone, Copy, Debug)]
pub struct Cell(pub sys::pb_cell);

impl Default for Cell {
    fn default() -> Self {
        Cell::new(b' ' as u32, Color::rgb(255, 255, 255), Color::rgb(0, 0, 0), 0)
    }
}

impl Cell {
    #[inline]
    pub fn new(ch: u32, fg: Color, bg: Color, style: u16) -> Self {
        Cell(sys::pb_cell {
            ch,
            fg: fg.0,
            bg: bg.0,
            style,
        })
    }

    #[inline]
    pub fn from_raw(c: sys::pb_cell) -> Self {
        Cell(c)
    }

    #[inline]
    pub fn raw(self) -> sys::pb_cell {
        self.0
    }

    pub fn blend(dst: Cell, src: Cell, alpha: f32, mode: i32) -> Cell {
        unsafe { Cell(sys::pb_cell_blend(dst.0, src.0, alpha, mode)) }
    }
}

/* ---- Typed events ---- */

#[derive(Clone, Copy, Debug)]
pub enum Event {
    None,
    Key {
        key: i32,
        codepoint: u32,
        alt: bool,
        ctrl: bool,
        shift: bool,
        pressed: bool,
    },
    Text(u32),
    Mouse {
        x: i32,
        y: i32,
        button: u8,
        pressed: bool,
        wheel: i32,
        shift: bool,
        alt: bool,
        ctrl: bool,
    },
    Resize {
        width: i32,
        height: i32,
    },
    Quit,
    Focus {
        focused: bool,
    },
}

impl Event {
    /// Convert a raw C event into a typed Rust event.
    ///
    /// # Safety
    /// `ev` must be a valid `pb_event` with a consistent `type_` / union payload.
    pub unsafe fn from_raw(ev: &sys::pb_event) -> Self {
        match ev.type_ {
            PB_EVENT_KEY => {
                let k = ev.as_.key;
                Event::Key {
                    key: k.key,
                    codepoint: k.codepoint,
                    alt: k.alt != 0,
                    ctrl: k.ctrl != 0,
                    shift: k.shift != 0,
                    pressed: k.pressed != 0,
                }
            }
            PB_EVENT_TEXT => Event::Text(ev.as_.text),
            PB_EVENT_MOUSE => {
                let m = ev.as_.mouse;
                Event::Mouse {
                    x: m.x,
                    y: m.y,
                    button: m.button,
                    pressed: m.pressed != 0,
                    wheel: m.wheel,
                    shift: m.shift != 0,
                    alt: m.alt != 0,
                    ctrl: m.ctrl != 0,
                }
            }
            PB_EVENT_RESIZE => {
                let r = ev.as_.resize;
                Event::Resize {
                    width: r.width,
                    height: r.height,
                }
            }
            PB_EVENT_QUIT => Event::Quit,
            PB_EVENT_FOCUS => {
                let f = ev.as_.focus;
                Event::Focus {
                    focused: f.focused != 0,
                }
            }
            _ => Event::None,
        }
    }
}

/* ---- Framebuffer ---- */

/// Cell framebuffer. May be owned (`new`) or borrowed (draw callback).
pub struct Framebuffer {
    raw: *mut sys::pb_fb,
    owned: bool,
}

impl Framebuffer {
    /// Allocate a new owned framebuffer via `pb_fb_create`.
    pub fn new(w: i32, h: i32) -> Result<Self, &'static str> {
        let raw = unsafe { sys::pb_fb_create(w, h) };
        if raw.is_null() {
            return Err("pb_fb_create failed");
        }
        Ok(Framebuffer { raw, owned: true })
    }

    /// Wrap a raw pointer without taking ownership (e.g. app draw callback).
    pub fn from_raw(raw: *mut sys::pb_fb) -> Self {
        Framebuffer { raw, owned: false }
    }

    #[inline]
    pub fn as_ptr(&self) -> *const sys::pb_fb {
        self.raw
    }

    #[inline]
    pub fn as_mut_ptr(&mut self) -> *mut sys::pb_fb {
        self.raw
    }

    pub fn size(&self) -> (i32, i32) {
        unsafe {
            if self.raw.is_null() {
                (0, 0)
            } else {
                ((*self.raw).w, (*self.raw).h)
            }
        }
    }

    pub fn width(&self) -> i32 {
        self.size().0
    }

    pub fn height(&self) -> i32 {
        self.size().1
    }

    pub fn clear(&mut self, fill: Cell) {
        unsafe { sys::pb_fb_clear(self.raw, fill.0) }
    }

    pub fn put(&mut self, x: i32, y: i32, c: Cell) {
        unsafe { sys::pb_fb_put(self.raw, x, y, c.0) }
    }

    pub fn get(&self, x: i32, y: i32) -> Cell {
        unsafe { Cell(sys::pb_fb_get(self.raw, x, y)) }
    }

    pub fn put_screen(&mut self, x: i32, y: i32, c: Cell) {
        unsafe { sys::pb_fb_put_screen(self.raw, x, y, c.0) }
    }

    pub fn get_screen(&self, x: i32, y: i32) -> Cell {
        unsafe { Cell(sys::pb_fb_get_screen(self.raw, x, y)) }
    }

    pub fn put_blend(&mut self, x: i32, y: i32, c: Cell, alpha: f32, mode: i32) {
        unsafe { sys::pb_fb_put_blend(self.raw, x, y, c.0, alpha, mode) }
    }

    pub fn text(&mut self, x: i32, y: i32, s: &str, fg: Color, bg: Color, style: u16) {
        let c = CString::new(s).unwrap_or_default();
        unsafe { sys::pb_fb_text(self.raw, x, y, c.as_ptr(), fg.0, bg.0, style) }
    }

    pub fn text_centered(&mut self, y: i32, s: &str, fg: Color, bg: Color, style: u16) {
        let c = CString::new(s).unwrap_or_default();
        unsafe { sys::pb_fb_text_centered(self.raw, y, c.as_ptr(), fg.0, bg.0, style) }
    }

    pub fn text_wrap(
        &mut self,
        x: i32,
        y: i32,
        max_w: i32,
        max_h: i32,
        s: &str,
        fg: Color,
        bg: Color,
        style: u16,
    ) -> i32 {
        let c = CString::new(s).unwrap_or_default();
        unsafe {
            sys::pb_fb_text_wrap(self.raw, x, y, max_w, max_h, c.as_ptr(), fg.0, bg.0, style)
        }
    }

    pub fn text_clipped(
        &mut self,
        x: i32,
        y: i32,
        max_w: i32,
        s: &str,
        fg: Color,
        bg: Color,
        style: u16,
    ) {
        let c = CString::new(s).unwrap_or_default();
        unsafe { sys::pb_fb_text_clipped(self.raw, x, y, max_w, c.as_ptr(), fg.0, bg.0, style) }
    }

    pub fn fill_rect(&mut self, x: i32, y: i32, w: i32, h: i32, c: Cell) {
        unsafe { sys::pb_fb_fill_rect(self.raw, x, y, w, h, c.0) }
    }

    pub fn box_frame(&mut self, x: i32, y: i32, w: i32, h: i32, fg: Color, bg: Color, style: u16) {
        unsafe { sys::pb_fb_box(self.raw, x, y, w, h, fg.0, bg.0, style) }
    }

    pub fn box_double(&mut self, x: i32, y: i32, w: i32, h: i32, fg: Color, bg: Color, style: u16) {
        unsafe { sys::pb_fb_box_double(self.raw, x, y, w, h, fg.0, bg.0, style) }
    }

    pub fn box_ex(
        &mut self,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        box_style: i32,
        fg: Color,
        bg: Color,
        style: u16,
    ) {
        unsafe { sys::pb_fb_box_ex(self.raw, x, y, w, h, box_style, fg.0, bg.0, style) }
    }

    pub fn panel(
        &mut self,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        title: &str,
        border: Color,
        title_fg: Color,
        fill: Color,
        style: u16,
    ) {
        let t = CString::new(title).unwrap_or_default();
        unsafe {
            sys::pb_fb_panel(
                self.raw,
                x,
                y,
                w,
                h,
                t.as_ptr(),
                border.0,
                title_fg.0,
                fill.0,
                style,
            )
        }
    }

    pub fn panel_ex(
        &mut self,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        title: &str,
        box_style: i32,
        border: Color,
        title_fg: Color,
        fill: Color,
        style: u16,
    ) {
        let t = CString::new(title).unwrap_or_default();
        unsafe {
            sys::pb_fb_panel_ex(
                self.raw,
                x,
                y,
                w,
                h,
                t.as_ptr(),
                box_style,
                border.0,
                title_fg.0,
                fill.0,
                style,
            )
        }
    }

    pub fn shadow(&mut self, x: i32, y: i32, w: i32, h: i32, shadow: Color, alpha: f32) {
        unsafe { sys::pb_fb_shadow(self.raw, x, y, w, h, shadow.0, alpha) }
    }

    pub fn hline(&mut self, x: i32, y: i32, w: i32, c: Cell) {
        unsafe { sys::pb_fb_hline(self.raw, x, y, w, c.0) }
    }

    pub fn vline(&mut self, x: i32, y: i32, h: i32, c: Cell) {
        unsafe { sys::pb_fb_vline(self.raw, x, y, h, c.0) }
    }

    pub fn line(&mut self, x0: i32, y0: i32, x1: i32, y1: i32, c: Cell) {
        unsafe { sys::pb_fb_line(self.raw, x0, y0, x1, y1, c.0) }
    }

    pub fn circle(&mut self, cx: i32, cy: i32, r: i32, c: Cell) {
        unsafe { sys::pb_fb_circle(self.raw, cx, cy, r, c.0) }
    }

    pub fn fill_circle(&mut self, cx: i32, cy: i32, r: i32, c: Cell) {
        unsafe { sys::pb_fb_fill_circle(self.raw, cx, cy, r, c.0) }
    }

    pub fn fill_triangle(
        &mut self,
        x0: i32,
        y0: i32,
        x1: i32,
        y1: i32,
        x2: i32,
        y2: i32,
        c: Cell,
    ) {
        unsafe { sys::pb_fb_fill_triangle(self.raw, x0, y0, x1, y1, x2, y2, c.0) }
    }

    pub fn blit(&mut self, dx: i32, dy: i32, src: &Framebuffer) {
        unsafe { sys::pb_fb_blit(self.raw, dx, dy, src.raw) }
    }

    pub fn blit_region(
        &mut self,
        dx: i32,
        dy: i32,
        src: &Framebuffer,
        sx: i32,
        sy: i32,
        w: i32,
        h: i32,
    ) {
        unsafe { sys::pb_fb_blit_region(self.raw, dx, dy, src.raw, sx, sy, w, h) }
    }

    pub fn blit_masked(&mut self, dx: i32, dy: i32, src: &Framebuffer, transparent_ch: u32) {
        unsafe { sys::pb_fb_blit_masked(self.raw, dx, dy, src.raw, transparent_ch) }
    }

    pub fn blit_blend(&mut self, dx: i32, dy: i32, src: &Framebuffer, alpha: f32, mode: i32) {
        unsafe { sys::pb_fb_blit_blend(self.raw, dx, dy, src.raw, alpha, mode) }
    }

    pub fn blit_tile(&mut self, dx: i32, dy: i32, sheet: &Sheet, tile_id: i32) {
        unsafe { sys::pb_fb_blit_tile(self.raw, dx, dy, &sheet.raw, tile_id) }
    }

    pub fn blit_tile_masked(
        &mut self,
        dx: i32,
        dy: i32,
        sheet: &Sheet,
        tile_id: i32,
        transparent_ch: u32,
    ) {
        unsafe {
            sys::pb_fb_blit_tile_masked(self.raw, dx, dy, &sheet.raw, tile_id, transparent_ch)
        }
    }

    pub fn nine_slice(
        &mut self,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        sheet: &Sheet,
        tile_tl: i32,
        tile_t: i32,
        tile_tr: i32,
        tile_l: i32,
        tile_c: i32,
        tile_r: i32,
        tile_bl: i32,
        tile_b: i32,
        tile_br: i32,
    ) {
        unsafe {
            sys::pb_fb_nine_slice(
                self.raw,
                x,
                y,
                w,
                h,
                &sheet.raw,
                tile_tl,
                tile_t,
                tile_tr,
                tile_l,
                tile_c,
                tile_r,
                tile_bl,
                tile_b,
                tile_br,
            )
        }
    }

    /* half-block plot */
    pub fn plot(&mut self, px: i32, py: i32, color: Color) {
        unsafe { sys::pb_fb_plot(self.raw, px, py, color.0) }
    }

    pub fn plot_blend(&mut self, px: i32, py: i32, color: Color, alpha: f32) {
        unsafe { sys::pb_fb_plot_blend(self.raw, px, py, color.0, alpha) }
    }

    pub fn plot_line(&mut self, x0: i32, y0: i32, x1: i32, y1: i32, color: Color) {
        unsafe { sys::pb_fb_plot_line(self.raw, x0, y0, x1, y1, color.0) }
    }

    pub fn plot_rect(&mut self, x: i32, y: i32, w: i32, h: i32, color: Color) {
        unsafe { sys::pb_fb_plot_rect(self.raw, x, y, w, h, color.0) }
    }

    pub fn plot_fill_rect(&mut self, x: i32, y: i32, w: i32, h: i32, color: Color) {
        unsafe { sys::pb_fb_plot_fill_rect(self.raw, x, y, w, h, color.0) }
    }

    pub fn plot_circle(&mut self, cx: i32, cy: i32, r: i32, color: Color) {
        unsafe { sys::pb_fb_plot_circle(self.raw, cx, cy, r, color.0) }
    }

    pub fn plot_fill_circle(&mut self, cx: i32, cy: i32, r: i32, color: Color) {
        unsafe { sys::pb_fb_plot_fill_circle(self.raw, cx, cy, r, color.0) }
    }

    /* braille */
    pub fn braille_clear(&mut self, bg: Color) {
        unsafe { sys::pb_fb_braille_clear(self.raw, bg.0) }
    }

    pub fn braille_plot(&mut self, px: i32, py: i32, color: Color) {
        unsafe { sys::pb_fb_braille_plot(self.raw, px, py, color.0) }
    }

    pub fn braille_plot_blend(&mut self, px: i32, py: i32, color: Color, alpha: f32) {
        unsafe { sys::pb_fb_braille_plot_blend(self.raw, px, py, color.0, alpha) }
    }

    pub fn braille_line(&mut self, x0: i32, y0: i32, x1: i32, y1: i32, color: Color) {
        unsafe { sys::pb_fb_braille_line(self.raw, x0, y0, x1, y1, color.0) }
    }

    pub fn braille_fill_rect(&mut self, x: i32, y: i32, w: i32, h: i32, color: Color) {
        unsafe { sys::pb_fb_braille_fill_rect(self.raw, x, y, w, h, color.0) }
    }

    pub fn braille_circle(&mut self, cx: i32, cy: i32, r: i32, color: Color) {
        unsafe { sys::pb_fb_braille_circle(self.raw, cx, cy, r, color.0) }
    }

    pub fn braille_fill_circle(&mut self, cx: i32, cy: i32, r: i32, color: Color) {
        unsafe { sys::pb_fb_braille_fill_circle(self.raw, cx, cy, r, color.0) }
    }

    pub fn braille_fill_triangle(
        &mut self,
        x0: i32,
        y0: i32,
        x1: i32,
        y1: i32,
        x2: i32,
        y2: i32,
        color: Color,
    ) {
        unsafe { sys::pb_fb_braille_fill_triangle(self.raw, x0, y0, x1, y1, x2, y2, color.0) }
    }

    /* quadrant */
    pub fn quad_plot(&mut self, px: i32, py: i32, color: Color) {
        unsafe { sys::pb_fb_quad_plot(self.raw, px, py, color.0) }
    }

    pub fn quad_fill_rect(&mut self, x: i32, y: i32, w: i32, h: i32, color: Color) {
        unsafe { sys::pb_fb_quad_fill_rect(self.raw, x, y, w, h, color.0) }
    }

    pub fn quad_fill_circle(&mut self, cx: i32, cy: i32, r: i32, color: Color) {
        unsafe { sys::pb_fb_quad_fill_circle(self.raw, cx, cy, r, color.0) }
    }

    /* full-block / shade */
    pub fn pixel(&mut self, x: i32, y: i32, color: Color) {
        unsafe { sys::pb_fb_pixel(self.raw, x, y, color.0) }
    }

    pub fn fill_shade(
        &mut self,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        fg: Color,
        bg: Color,
        level: i32,
    ) {
        unsafe { sys::pb_fb_fill_shade(self.raw, x, y, w, h, fg.0, bg.0, level) }
    }

    pub fn fill_gradient_v(&mut self, x: i32, y: i32, w: i32, h: i32, top: Color, bottom: Color) {
        unsafe { sys::pb_fb_fill_gradient_v(self.raw, x, y, w, h, top.0, bottom.0) }
    }

    pub fn fill_gradient_h(&mut self, x: i32, y: i32, w: i32, h: i32, left: Color, right: Color) {
        unsafe { sys::pb_fb_fill_gradient_h(self.raw, x, y, w, h, left.0, right.0) }
    }

    pub fn fill_dither(
        &mut self,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        a: Color,
        b: Color,
        pattern: i32,
    ) {
        unsafe { sys::pb_fb_fill_dither(self.raw, x, y, w, h, a.0, b.0, pattern) }
    }

    /* camera & clip */
    pub fn set_camera(&mut self, x: i32, y: i32) {
        unsafe { sys::pb_fb_set_camera(self.raw, x, y) }
    }

    pub fn get_camera(&self) -> (i32, i32) {
        let mut x = 0;
        let mut y = 0;
        unsafe { sys::pb_fb_get_camera(self.raw, &mut x, &mut y) };
        (x, y)
    }

    pub fn set_clip(&mut self, x: i32, y: i32, w: i32, h: i32) {
        unsafe { sys::pb_fb_set_clip(self.raw, x, y, w, h) }
    }

    pub fn reset_clip(&mut self) {
        unsafe { sys::pb_fb_reset_clip(self.raw) }
    }

    pub fn draw_particles_braille(&mut self, ps: &Particles) {
        unsafe { sys::pb_particles_draw_braille(self.raw, &ps.raw) }
    }

    pub fn draw_particles_half(&mut self, ps: &Particles) {
        unsafe { sys::pb_particles_draw_half(self.raw, &ps.raw) }
    }
}

impl Drop for Framebuffer {
    fn drop(&mut self) {
        if self.owned && !self.raw.is_null() {
            unsafe { sys::pb_fb_destroy(self.raw) }
            self.raw = ptr::null_mut();
        }
    }
}

/* ---- Free functions ---- */

pub fn measure_text(s: &str) -> i32 {
    let c = CString::new(s).unwrap_or_default();
    unsafe { sys::pb_fb_measure_text(c.as_ptr()) }
}

pub fn measure_text_ex(s: &str) -> i32 {
    let c = CString::new(s).unwrap_or_default();
    unsafe { sys::pb_fb_measure_text_ex(c.as_ptr()) }
}

pub fn char_width(codepoint: u32) -> i32 {
    unsafe { sys::pb_char_width(codepoint) }
}

pub fn version() -> &'static str {
    unsafe {
        CStr::from_ptr(sys::pb_version_string())
            .to_str()
            .unwrap_or("")
    }
}

pub fn version_numbers() -> (i32, i32, i32) {
    let mut major = 0;
    let mut minor = 0;
    let mut patch = 0;
    unsafe { sys::pb_version(&mut major, &mut minor, &mut patch) };
    (major, minor, patch)
}

/* ---- Sheet ---- */

pub struct Sheet {
    raw: sys::pb_sheet,
}

impl Sheet {
    pub fn create(cols: i32, rows: i32, tile_w: i32, tile_h: i32) -> Self {
        Sheet {
            raw: unsafe { sys::pb_sheet_create(cols, rows, tile_w, tile_h) },
        }
    }

    /// Wrap an owned framebuffer as a sprite sheet atlas.
    ///
    /// Consumes `atlas` and transfers its cell buffer into the sheet. The sheet
    /// takes ownership of the atlas cells (`owns_atlas = 1`).
    pub fn wrap(mut atlas: Framebuffer, tile_w: i32, tile_h: i32) -> Result<Self, &'static str> {
        if !atlas.owned || atlas.raw.is_null() {
            return Err("Sheet::wrap requires an owned Framebuffer");
        }
        let fb = unsafe { ptr::read(atlas.raw) };
        let raw_ptr = atlas.raw;
        atlas.raw = ptr::null_mut();
        atlas.owned = false;

        let mut sheet = unsafe { sys::pb_sheet_wrap(fb, tile_w, tile_h) };
        // pb_sheet_wrap sets owns_atlas=0; we transferred an owned buffer, so claim it.
        sheet.owns_atlas = 1;

        // Free the heap pb_fb header only (cells already moved into the sheet).
        unsafe {
            (*raw_ptr).cells = ptr::null_mut();
            (*raw_ptr).w = 0;
            (*raw_ptr).h = 0;
            sys::pb_fb_destroy(raw_ptr);
        }
        Ok(Sheet { raw: sheet })
    }

    pub fn set_tile(&mut self, tile_id: i32, src: &Framebuffer) {
        unsafe { sys::pb_sheet_set_tile(&mut self.raw, tile_id, src.raw) }
    }

    pub fn tile_size(&self) -> (i32, i32) {
        (self.raw.tile_w, self.raw.tile_h)
    }

    pub fn grid(&self) -> (i32, i32) {
        (self.raw.cols, self.raw.rows)
    }
}

impl Drop for Sheet {
    fn drop(&mut self) {
        unsafe { sys::pb_sheet_free(&mut self.raw) }
    }
}

/* ---- Anim ---- */

pub struct Anim {
    raw: sys::pb_anim,
    /// Heap-stable frame indices so `raw.frames` stays valid across moves.
    _frames: Box<[c_int]>,
}

impl Anim {
    pub fn new(frames: &[i32], fps: f32, loop_: bool) -> Self {
        let frames_owned: Box<[c_int]> = frames.to_vec().into_boxed_slice();
        let raw = sys::pb_anim {
            frames: frames_owned.as_ptr(),
            frame_count: frames_owned.len() as c_int,
            fps,
            t: 0.0,
            loop_: if loop_ { 1 } else { 0 },
        };
        Anim {
            raw,
            _frames: frames_owned,
        }
    }

    pub fn reset(&mut self) {
        self.raw.frames = self._frames.as_ptr();
        unsafe { sys::pb_anim_reset(&mut self.raw) }
    }

    pub fn update(&mut self, dt: f64) {
        self.raw.frames = self._frames.as_ptr();
        unsafe { sys::pb_anim_update(&mut self.raw, dt) }
    }

    pub fn frame(&self) -> i32 {
        unsafe { sys::pb_anim_frame(&self.raw) }
    }
}

/* ---- Particles ---- */

pub struct Particles {
    raw: sys::pb_particles,
}

impl Particles {
    pub fn new(capacity: i32) -> Result<Self, &'static str> {
        let mut raw = unsafe { std::mem::zeroed::<sys::pb_particles>() };
        let ok = unsafe { sys::pb_particles_init(&mut raw, capacity) };
        if ok == 0 {
            return Err("pb_particles_init failed");
        }
        Ok(Particles { raw })
    }

    pub fn emit(&mut self, x: f32, y: f32, vx: f32, vy: f32, life: f32, color: Color) {
        unsafe { sys::pb_particles_emit(&mut self.raw, x, y, vx, vy, life, color.0) }
    }

    pub fn update(&mut self, dt: f64) {
        unsafe { sys::pb_particles_update(&mut self.raw, dt) }
    }

    pub fn count(&self) -> i32 {
        self.raw.count
    }

    pub fn capacity(&self) -> i32 {
        self.raw.capacity
    }
}

impl Drop for Particles {
    fn drop(&mut self) {
        unsafe { sys::pb_particles_free(&mut self.raw) }
    }
}

/* ---- App ---- */

struct Hooks {
    on_init: Option<Box<dyn FnMut(&mut App)>>,
    on_event: Option<Box<dyn FnMut(&mut App, Event)>>,
    on_update: Option<Box<dyn FnMut(&mut App, f64)>>,
    on_draw: Option<Box<dyn FnMut(&mut App, &mut Framebuffer)>>,
    on_shutdown: Option<Box<dyn FnMut(&mut App)>>,
}

pub struct App {
    raw: *mut sys::pb_app,
    _title: CString,
    hooks: Box<Hooks>,
}

impl App {
    pub fn new(title: &str, target_fps: i32) -> Result<Self, &'static str> {
        Self::with_options(title, target_fps, 0, Cell::default())
    }

    pub fn with_options(
        title: &str,
        target_fps: i32,
        flags: u32,
        clear: Cell,
    ) -> Result<Self, &'static str> {
        let title_c = CString::new(title).map_err(|_| "title contains NUL")?;
        let hooks = Box::new(Hooks {
            on_init: None,
            on_event: None,
            on_update: None,
            on_draw: None,
            on_shutdown: None,
        });

        let mut desc = unsafe { std::mem::zeroed::<sys::pb_app_desc>() };
        desc.title = title_c.as_ptr();
        desc.target_fps = target_fps;
        desc.flags = flags;
        desc.clear = clear.0;
        desc.on_init = Some(tramp_init);
        desc.on_event = Some(tramp_event);
        desc.on_update = Some(tramp_update);
        desc.on_draw = Some(tramp_draw);
        desc.on_shutdown = Some(tramp_shutdown);

        let raw = unsafe { sys::pb_app_create(&desc, ptr::null_mut()) };
        if raw.is_null() {
            return Err("pb_app_create failed");
        }

        Ok(App {
            raw,
            _title: title_c,
            hooks,
        })
    }

    pub fn on_init<F: FnMut(&mut App) + 'static>(&mut self, f: F) {
        self.hooks.on_init = Some(Box::new(f));
    }

    pub fn on_event<F: FnMut(&mut App, Event) + 'static>(&mut self, f: F) {
        self.hooks.on_event = Some(Box::new(f));
    }

    pub fn on_update<F: FnMut(&mut App, f64) + 'static>(&mut self, f: F) {
        self.hooks.on_update = Some(Box::new(f));
    }

    pub fn on_draw<F: FnMut(&mut App, &mut Framebuffer) + 'static>(&mut self, f: F) {
        self.hooks.on_draw = Some(Box::new(f));
    }

    pub fn on_shutdown<F: FnMut(&mut App) + 'static>(&mut self, f: F) {
        self.hooks.on_shutdown = Some(Box::new(f));
    }

    pub fn run(&mut self) -> i32 {
        CURRENT.with(|c| *c.borrow_mut() = self as *mut App);
        let rc = unsafe { sys::pb_app_run(self.raw) };
        CURRENT.with(|c| *c.borrow_mut() = ptr::null_mut());
        rc
    }

    pub fn quit(&mut self) {
        unsafe { sys::pb_app_quit(self.raw) }
    }

    pub fn request_resize(&mut self) {
        unsafe { sys::pb_app_request_resize(self.raw) }
    }

    pub fn width(&self) -> i32 {
        unsafe { sys::pb_app_width(self.raw) }
    }

    pub fn height(&self) -> i32 {
        unsafe { sys::pb_app_height(self.raw) }
    }

    pub fn size(&self) -> (i32, i32) {
        (self.width(), self.height())
    }

    pub fn set_title(&mut self, title: &str) {
        if let Ok(c) = CString::new(title) {
            unsafe { sys::pb_app_set_title(self.raw, c.as_ptr()) }
            self._title = c;
        }
    }

    pub fn set_clear(&mut self, clear: Cell) {
        unsafe { sys::pb_app_set_clear(self.raw, clear.0) }
    }

    pub fn set_target_fps(&mut self, fps: i32) {
        unsafe { sys::pb_app_set_target_fps(self.raw, fps) }
    }

    /* input polling */
    pub fn is_key_down(&self, key: i32) -> bool {
        unsafe { sys::pb_is_key_down(self.raw, key) != 0 }
    }

    pub fn is_key_pressed(&self, key: i32) -> bool {
        unsafe { sys::pb_is_key_pressed(self.raw, key) != 0 }
    }

    pub fn is_key_released(&self, key: i32) -> bool {
        unsafe { sys::pb_is_key_released(self.raw, key) != 0 }
    }

    pub fn is_char_down(&self, codepoint: u32) -> bool {
        unsafe { sys::pb_is_char_down(self.raw, codepoint) != 0 }
    }

    pub fn is_char_pressed(&self, codepoint: u32) -> bool {
        unsafe { sys::pb_is_char_pressed(self.raw, codepoint) != 0 }
    }

    pub fn mouse_x(&self) -> i32 {
        unsafe { sys::pb_get_mouse_x(self.raw) }
    }

    pub fn mouse_y(&self) -> i32 {
        unsafe { sys::pb_get_mouse_y(self.raw) }
    }

    pub fn mouse(&self) -> (i32, i32) {
        (self.mouse_x(), self.mouse_y())
    }

    pub fn is_mouse_button_down(&self, button: i32) -> bool {
        unsafe { sys::pb_is_mouse_button_down(self.raw, button) != 0 }
    }

    pub fn is_mouse_button_pressed(&self, button: i32) -> bool {
        unsafe { sys::pb_is_mouse_button_pressed(self.raw, button) != 0 }
    }

    pub fn is_mouse_button_released(&self, button: i32) -> bool {
        unsafe { sys::pb_is_mouse_button_released(self.raw, button) != 0 }
    }

    pub fn mouse_wheel(&self) -> i32 {
        unsafe { sys::pb_get_mouse_wheel(self.raw) }
    }

    pub fn frame_time(&self) -> f64 {
        unsafe { sys::pb_get_frame_time(self.raw) }
    }

    pub fn fps(&self) -> i32 {
        unsafe { sys::pb_get_fps(self.raw) }
    }

    pub fn is_focused(&self) -> bool {
        unsafe { sys::pb_is_focused(self.raw) != 0 }
    }

    /* replay */
    pub fn is_replay(&self) -> bool {
        unsafe { sys::pb_app_is_replay(self.raw) != 0 }
    }

    pub fn is_recording(&self) -> bool {
        unsafe { sys::pb_app_is_recording(self.raw) != 0 }
    }

    pub fn replay_seed(&self) -> u32 {
        unsafe { sys::pb_app_replay_seed(self.raw) }
    }

    pub fn replay_initial_size(&self) -> Option<(i32, i32)> {
        let mut w = 0;
        let mut h = 0;
        let ok = unsafe { sys::pb_app_replay_initial_size(self.raw, &mut w, &mut h) };
        if ok != 0 {
            Some((w, h))
        } else {
            None
        }
    }

    pub fn version() -> &'static str {
        version()
    }

    pub fn as_ptr(&self) -> *const sys::pb_app {
        self.raw
    }

    pub fn as_mut_ptr(&mut self) -> *mut sys::pb_app {
        self.raw
    }
}

fn with_current<F: FnOnce(&mut App)>(f: F) {
    CURRENT.with(|c| {
        let p = *c.borrow();
        if !p.is_null() {
            unsafe { f(&mut *p) }
        }
    });
}

unsafe extern "C" fn tramp_init(_a: *mut sys::pb_app, _u: *mut c_void) {
    with_current(|app| {
        let mut cb = app.hooks.on_init.take();
        if let Some(ref mut f) = cb {
            f(app);
        }
        app.hooks.on_init = cb;
    });
}

unsafe extern "C" fn tramp_event(_a: *mut sys::pb_app, _u: *mut c_void, ev: *const sys::pb_event) {
    if ev.is_null() {
        return;
    }
    let typed = Event::from_raw(&*ev);
    with_current(|app| {
        let mut cb = app.hooks.on_event.take();
        if let Some(ref mut f) = cb {
            f(app, typed);
        }
        app.hooks.on_event = cb;
    });
}

unsafe extern "C" fn tramp_update(_a: *mut sys::pb_app, _u: *mut c_void, dt: c_double) {
    with_current(|app| {
        let mut cb = app.hooks.on_update.take();
        if let Some(ref mut f) = cb {
            f(app, dt);
        }
        app.hooks.on_update = cb;
    });
}

unsafe extern "C" fn tramp_draw(_a: *mut sys::pb_app, _u: *mut c_void, fb: *mut sys::pb_fb) {
    with_current(|app| {
        let mut cb = app.hooks.on_draw.take();
        if let Some(ref mut f) = cb {
            let mut frame = Framebuffer::from_raw(fb);
            f(app, &mut frame);
        }
        app.hooks.on_draw = cb;
    });
}

unsafe extern "C" fn tramp_shutdown(_a: *mut sys::pb_app, _u: *mut c_void) {
    with_current(|app| {
        let mut cb = app.hooks.on_shutdown.take();
        if let Some(ref mut f) = cb {
            f(app);
        }
        app.hooks.on_shutdown = cb;
    });
}

impl Drop for App {
    fn drop(&mut self) {
        if !self.raw.is_null() {
            unsafe { sys::pb_app_destroy(self.raw) }
            self.raw = ptr::null_mut();
        }
    }
}
