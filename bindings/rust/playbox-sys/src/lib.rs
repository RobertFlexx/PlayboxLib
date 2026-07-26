//! Low-level FFI bindings to PlayboxLib (C ABI).
//!
//! Covers all `PB_API` entry points from `pb_fb`, `pb_gfx`, `pb_app`, and
//! `pb_state`. Skips `pb_fb_textf` (varargs — format in Rust, call `pb_fb_text`).

#![allow(non_camel_case_types, non_snake_case, dead_code, clippy::all)]

use std::os::raw::{c_char, c_double, c_float, c_int, c_void};

/* ---- Version (pb_export.h) ---- */

pub const PB_VERSION_MAJOR: c_int = 1;
pub const PB_VERSION_MINOR: c_int = 1;
pub const PB_VERSION_PATCH: c_int = 0;
pub const PB_VERSION_STRING: &str = "1.1.0";

/* ---- Types (pb_types.h) ---- */

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct pb_color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

pub type pb_style = u16;

pub const PB_STYLE_NONE: u16 = 0;
pub const PB_STYLE_BOLD: u16 = 1 << 0;
pub const PB_STYLE_DIM: u16 = 1 << 1;
pub const PB_STYLE_UNDERLINE: u16 = 1 << 2;
pub const PB_STYLE_REVERSE: u16 = 1 << 3;
pub const PB_STYLE_ITALIC: u16 = 1 << 4;
pub const PB_STYLE_STRIKETHROUGH: u16 = 1 << 5;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct pb_cell {
    pub ch: u32,
    pub fg: pb_color,
    pub bg: pb_color,
    pub style: u16,
}

pub type pb_key = c_int;

pub const PB_KEY_NONE: c_int = 0;
pub const PB_KEY_ESC: c_int = 1;
pub const PB_KEY_ENTER: c_int = 2;
pub const PB_KEY_BACKSPACE: c_int = 3;
pub const PB_KEY_TAB: c_int = 4;
pub const PB_KEY_UP: c_int = 5;
pub const PB_KEY_DOWN: c_int = 6;
pub const PB_KEY_LEFT: c_int = 7;
pub const PB_KEY_RIGHT: c_int = 8;
pub const PB_KEY_HOME: c_int = 9;
pub const PB_KEY_END: c_int = 10;
pub const PB_KEY_PGUP: c_int = 11;
pub const PB_KEY_PGDN: c_int = 12;
pub const PB_KEY_INS: c_int = 13;
pub const PB_KEY_DEL: c_int = 14;
pub const PB_KEY_F1: c_int = 15;
pub const PB_KEY_F2: c_int = 16;
pub const PB_KEY_F3: c_int = 17;
pub const PB_KEY_F4: c_int = 18;
pub const PB_KEY_F5: c_int = 19;
pub const PB_KEY_F6: c_int = 20;
pub const PB_KEY_F7: c_int = 21;
pub const PB_KEY_F8: c_int = 22;
pub const PB_KEY_F9: c_int = 23;
pub const PB_KEY_F10: c_int = 24;
pub const PB_KEY_F11: c_int = 25;
pub const PB_KEY_F12: c_int = 26;
pub const PB_KEY_COUNT: c_int = 27;

pub type pb_mouse_button = c_int;

pub const PB_MOUSE_LEFT: c_int = 0;
pub const PB_MOUSE_MIDDLE: c_int = 1;
pub const PB_MOUSE_RIGHT: c_int = 2;
pub const PB_MOUSE_BUTTON_COUNT: c_int = 8;

pub type pb_event_type = c_int;

pub const PB_EVENT_NONE: c_int = 0;
pub const PB_EVENT_KEY: c_int = 1;
pub const PB_EVENT_TEXT: c_int = 2;
pub const PB_EVENT_MOUSE: c_int = 3;
pub const PB_EVENT_RESIZE: c_int = 4;
pub const PB_EVENT_QUIT: c_int = 5;
pub const PB_EVENT_FOCUS: c_int = 6;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct pb_key_event {
    pub key: pb_key,
    pub codepoint: u32,
    pub alt: u8,
    pub ctrl: u8,
    pub shift: u8,
    pub pressed: u8,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct pb_mouse_event {
    pub x: c_int,
    pub y: c_int,
    pub button: u8,
    pub pressed: u8,
    pub wheel: c_int,
    pub shift: u8,
    pub alt: u8,
    pub ctrl: u8,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct pb_resize_event {
    pub width: c_int,
    pub height: c_int,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct pb_focus_event {
    pub focused: u8,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union pb_event_data {
    pub key: pb_key_event,
    pub text: u32,
    pub mouse: pb_mouse_event,
    pub resize: pb_resize_event,
    pub focus: pb_focus_event,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct pb_event {
    pub type_: pb_event_type,
    pub as_: pb_event_data,
}

/* ---- Framebuffer (pb_fb.h) ---- */

#[repr(C)]
pub struct pb_fb {
    pub w: c_int,
    pub h: c_int,
    pub cells: *mut pb_cell,
    pub cam_x: c_int,
    pub cam_y: c_int,
    pub clip_x0: c_int,
    pub clip_y0: c_int,
    pub clip_x1: c_int,
    pub clip_y1: c_int,
}

/* ---- Graphics extras (pb_gfx.h) ---- */

pub type pb_box_style = c_int;

pub const PB_BOX_SINGLE: c_int = 0;
pub const PB_BOX_DOUBLE: c_int = 1;
pub const PB_BOX_ROUNDED: c_int = 2;
pub const PB_BOX_HEAVY: c_int = 3;
pub const PB_BOX_ASCII: c_int = 4;
pub const PB_BOX_DASHED: c_int = 5;

pub type pb_blend_mode = c_int;

pub const PB_BLEND_REPLACE: c_int = 0;
pub const PB_BLEND_ALPHA: c_int = 1;
pub const PB_BLEND_ADD: c_int = 2;
pub const PB_BLEND_MUL: c_int = 3;

#[repr(C)]
pub struct pb_sheet {
    pub atlas: pb_fb,
    pub tile_w: c_int,
    pub tile_h: c_int,
    pub cols: c_int,
    pub rows: c_int,
    pub owns_atlas: c_int,
}

#[repr(C)]
pub struct pb_anim {
    pub frames: *const c_int,
    pub frame_count: c_int,
    pub fps: c_float,
    pub t: c_float,
    pub loop_: c_int,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct pb_particle {
    pub x: c_float,
    pub y: c_float,
    pub vx: c_float,
    pub vy: c_float,
    pub life: c_float,
    pub max_life: c_float,
    pub color: pb_color,
    pub alive: u8,
}

#[repr(C)]
pub struct pb_particles {
    pub items: *mut pb_particle,
    pub count: c_int,
    pub capacity: c_int,
}

/* ---- App (pb_app.h) ---- */

pub enum pb_app {}

pub type pb_on_init_fn = Option<unsafe extern "C" fn(*mut pb_app, *mut c_void)>;
pub type pb_on_event_fn =
    Option<unsafe extern "C" fn(*mut pb_app, *mut c_void, *const pb_event)>;
pub type pb_on_update_fn = Option<unsafe extern "C" fn(*mut pb_app, *mut c_void, c_double)>;
pub type pb_on_draw_fn = Option<unsafe extern "C" fn(*mut pb_app, *mut c_void, *mut pb_fb)>;
pub type pb_on_shutdown_fn = Option<unsafe extern "C" fn(*mut pb_app, *mut c_void)>;

pub const PB_APP_FLAG_NO_AUTO_CLEAR: u32 = 1 << 0;
pub const PB_APP_FLAG_NO_MOUSE: u32 = 1 << 1;
pub const PB_APP_FLAG_NO_FOCUS: u32 = 1 << 2;
pub const PB_APP_FLAG_CUSTOM_CLEAR: u32 = 1 << 3;

#[repr(C)]
pub struct pb_app_desc {
    pub title: *const c_char,
    pub target_fps: c_int,
    pub flags: u32,
    pub clear: pb_cell,
    pub on_init: pb_on_init_fn,
    pub on_event: pb_on_event_fn,
    pub on_update: pb_on_update_fn,
    pub on_draw: pb_on_draw_fn,
    pub on_shutdown: pb_on_shutdown_fn,
}

/* ---- Extern API ---- */

extern "C" {
    /* pb_fb.h */
    pub fn pb_fb_make(w: c_int, h: c_int) -> pb_fb;
    pub fn pb_fb_free(fb: *mut pb_fb);

    pub fn pb_fb_clear(fb: *mut pb_fb, fill: pb_cell);

    pub fn pb_fb_put(fb: *mut pb_fb, x: c_int, y: c_int, c: pb_cell);
    pub fn pb_fb_get(fb: *const pb_fb, x: c_int, y: c_int) -> pb_cell;
    pub fn pb_fb_put_screen(fb: *mut pb_fb, x: c_int, y: c_int, c: pb_cell);
    pub fn pb_fb_get_screen(fb: *const pb_fb, x: c_int, y: c_int) -> pb_cell;

    pub fn pb_fb_text(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        utf8: *const c_char,
        fg: pb_color,
        bg: pb_color,
        style: u16,
    );
    // pb_fb_textf skipped (varargs)
    pub fn pb_fb_text_centered(
        fb: *mut pb_fb,
        y: c_int,
        utf8: *const c_char,
        fg: pb_color,
        bg: pb_color,
        style: u16,
    );

    pub fn pb_fb_fill_rect(fb: *mut pb_fb, x: c_int, y: c_int, w: c_int, h: c_int, c: pb_cell);
    pub fn pb_fb_box(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        fg: pb_color,
        bg: pb_color,
        style: u16,
    );
    pub fn pb_fb_box_double(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        fg: pb_color,
        bg: pb_color,
        style: u16,
    );
    pub fn pb_fb_panel(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        title: *const c_char,
        border: pb_color,
        title_fg: pb_color,
        fill: pb_color,
        style: u16,
    );

    pub fn pb_fb_hline(fb: *mut pb_fb, x: c_int, y: c_int, w: c_int, c: pb_cell);
    pub fn pb_fb_vline(fb: *mut pb_fb, x: c_int, y: c_int, h: c_int, c: pb_cell);
    pub fn pb_fb_line(fb: *mut pb_fb, x0: c_int, y0: c_int, x1: c_int, y1: c_int, c: pb_cell);

    pub fn pb_fb_circle(fb: *mut pb_fb, cx: c_int, cy: c_int, radius: c_int, c: pb_cell);
    pub fn pb_fb_fill_circle(fb: *mut pb_fb, cx: c_int, cy: c_int, radius: c_int, c: pb_cell);

    pub fn pb_fb_blit(dst: *mut pb_fb, dx: c_int, dy: c_int, src: *const pb_fb);
    pub fn pb_fb_blit_region(
        dst: *mut pb_fb,
        dx: c_int,
        dy: c_int,
        src: *const pb_fb,
        sx: c_int,
        sy: c_int,
        w: c_int,
        h: c_int,
    );
    pub fn pb_fb_blit_masked(
        dst: *mut pb_fb,
        dx: c_int,
        dy: c_int,
        src: *const pb_fb,
        transparent_ch: u32,
    );

    pub fn pb_fb_plot(fb: *mut pb_fb, px: c_int, py: c_int, color: pb_color);
    pub fn pb_fb_plot_line(
        fb: *mut pb_fb,
        x0: c_int,
        y0: c_int,
        x1: c_int,
        y1: c_int,
        color: pb_color,
    );
    pub fn pb_fb_plot_rect(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        color: pb_color,
    );
    pub fn pb_fb_plot_fill_rect(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        color: pb_color,
    );
    pub fn pb_fb_plot_circle(
        fb: *mut pb_fb,
        cx: c_int,
        cy: c_int,
        radius: c_int,
        color: pb_color,
    );
    pub fn pb_fb_plot_fill_circle(
        fb: *mut pb_fb,
        cx: c_int,
        cy: c_int,
        radius: c_int,
        color: pb_color,
    );

    pub fn pb_fb_measure_text(utf8: *const c_char) -> c_int;
    pub fn pb_char_width(codepoint: u32) -> c_int;

    /* pb_gfx.h — text wrap / clip */
    pub fn pb_fb_text_wrap(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        max_w: c_int,
        max_h: c_int,
        utf8: *const c_char,
        fg: pb_color,
        bg: pb_color,
        style: u16,
    ) -> c_int;
    pub fn pb_fb_text_clipped(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        max_w: c_int,
        utf8: *const c_char,
        fg: pb_color,
        bg: pb_color,
        style: u16,
    );
    pub fn pb_fb_measure_text_ex(utf8: *const c_char) -> c_int;

    /* camera & scissor */
    pub fn pb_fb_set_camera(fb: *mut pb_fb, cam_x: c_int, cam_y: c_int);
    pub fn pb_fb_get_camera(fb: *const pb_fb, out_x: *mut c_int, out_y: *mut c_int);
    pub fn pb_fb_set_clip(fb: *mut pb_fb, x: c_int, y: c_int, w: c_int, h: c_int);
    pub fn pb_fb_reset_clip(fb: *mut pb_fb);

    /* box styles / panels / shadows */
    pub fn pb_fb_box_ex(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        box_style: pb_box_style,
        fg: pb_color,
        bg: pb_color,
        style: u16,
    );
    pub fn pb_fb_panel_ex(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        title: *const c_char,
        box_style: pb_box_style,
        border: pb_color,
        title_fg: pb_color,
        fill: pb_color,
        style: u16,
    );
    pub fn pb_fb_shadow(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        shadow: pb_color,
        alpha: c_float,
    );

    /* braille */
    pub fn pb_fb_braille_clear(fb: *mut pb_fb, bg: pb_color);
    pub fn pb_fb_braille_plot(fb: *mut pb_fb, px: c_int, py: c_int, color: pb_color);
    pub fn pb_fb_braille_plot_blend(
        fb: *mut pb_fb,
        px: c_int,
        py: c_int,
        color: pb_color,
        alpha: c_float,
    );
    pub fn pb_fb_braille_line(
        fb: *mut pb_fb,
        x0: c_int,
        y0: c_int,
        x1: c_int,
        y1: c_int,
        color: pb_color,
    );
    pub fn pb_fb_braille_fill_rect(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        color: pb_color,
    );
    pub fn pb_fb_braille_fill_circle(
        fb: *mut pb_fb,
        cx: c_int,
        cy: c_int,
        radius: c_int,
        color: pb_color,
    );
    pub fn pb_fb_braille_circle(
        fb: *mut pb_fb,
        cx: c_int,
        cy: c_int,
        radius: c_int,
        color: pb_color,
    );

    /* quadrant */
    pub fn pb_fb_quad_plot(fb: *mut pb_fb, px: c_int, py: c_int, color: pb_color);
    pub fn pb_fb_quad_fill_rect(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        color: pb_color,
    );
    pub fn pb_fb_quad_fill_circle(
        fb: *mut pb_fb,
        cx: c_int,
        cy: c_int,
        radius: c_int,
        color: pb_color,
    );

    /* full-block / shade */
    pub fn pb_fb_pixel(fb: *mut pb_fb, x: c_int, y: c_int, color: pb_color);
    pub fn pb_fb_fill_shade(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        fg: pb_color,
        bg: pb_color,
        level: c_int,
    );

    /* blends & fills */
    pub fn pb_cell_blend(dst: pb_cell, src: pb_cell, alpha: c_float, mode: pb_blend_mode)
        -> pb_cell;
    pub fn pb_fb_put_blend(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        c: pb_cell,
        alpha: c_float,
        mode: pb_blend_mode,
    );
    pub fn pb_fb_blit_blend(
        dst: *mut pb_fb,
        dx: c_int,
        dy: c_int,
        src: *const pb_fb,
        alpha: c_float,
        mode: pb_blend_mode,
    );
    pub fn pb_fb_plot_blend(
        fb: *mut pb_fb,
        px: c_int,
        py: c_int,
        color: pb_color,
        alpha: c_float,
    );
    pub fn pb_fb_fill_gradient_v(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        top: pb_color,
        bottom: pb_color,
    );
    pub fn pb_fb_fill_gradient_h(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        left: pb_color,
        right: pb_color,
    );
    pub fn pb_fb_fill_dither(
        fb: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        a: pb_color,
        b: pb_color,
        pattern: c_int,
    );

    /* triangles */
    pub fn pb_fb_fill_triangle(
        fb: *mut pb_fb,
        x0: c_int,
        y0: c_int,
        x1: c_int,
        y1: c_int,
        x2: c_int,
        y2: c_int,
        c: pb_cell,
    );
    pub fn pb_fb_braille_fill_triangle(
        fb: *mut pb_fb,
        x0: c_int,
        y0: c_int,
        x1: c_int,
        y1: c_int,
        x2: c_int,
        y2: c_int,
        color: pb_color,
    );

    /* sprite sheets */
    pub fn pb_sheet_wrap(atlas: pb_fb, tile_w: c_int, tile_h: c_int) -> pb_sheet;
    pub fn pb_sheet_create(cols: c_int, rows: c_int, tile_w: c_int, tile_h: c_int) -> pb_sheet;
    pub fn pb_sheet_free(sheet: *mut pb_sheet);
    pub fn pb_sheet_set_tile(sheet: *mut pb_sheet, tile_id: c_int, src: *const pb_fb);
    pub fn pb_fb_blit_tile(
        dst: *mut pb_fb,
        dx: c_int,
        dy: c_int,
        sheet: *const pb_sheet,
        tile_id: c_int,
    );
    pub fn pb_fb_blit_tile_masked(
        dst: *mut pb_fb,
        dx: c_int,
        dy: c_int,
        sheet: *const pb_sheet,
        tile_id: c_int,
        transparent_ch: u32,
    );
    pub fn pb_fb_nine_slice(
        dst: *mut pb_fb,
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        sheet: *const pb_sheet,
        tile_tl: c_int,
        tile_t: c_int,
        tile_tr: c_int,
        tile_l: c_int,
        tile_c: c_int,
        tile_r: c_int,
        tile_bl: c_int,
        tile_b: c_int,
        tile_br: c_int,
    );

    /* animation */
    pub fn pb_anim_reset(anim: *mut pb_anim);
    pub fn pb_anim_update(anim: *mut pb_anim, dt: c_double);
    pub fn pb_anim_frame(anim: *const pb_anim) -> c_int;

    /* particles */
    pub fn pb_particles_init(ps: *mut pb_particles, capacity: c_int) -> c_int;
    pub fn pb_particles_free(ps: *mut pb_particles);
    pub fn pb_particles_emit(
        ps: *mut pb_particles,
        x: c_float,
        y: c_float,
        vx: c_float,
        vy: c_float,
        life: c_float,
        color: pb_color,
    );
    pub fn pb_particles_update(ps: *mut pb_particles, dt: c_double);
    pub fn pb_particles_draw_braille(fb: *mut pb_fb, ps: *const pb_particles);
    pub fn pb_particles_draw_half(fb: *mut pb_fb, ps: *const pb_particles);

    /* FFI constructors / version */
    pub fn pb_rgb_ex(r: u8, g: u8, b: u8) -> pb_color;
    pub fn pb_cell_ex(ch: u32, fg: pb_color, bg: pb_color, style: u16) -> pb_cell;
    pub fn pb_fb_create(w: c_int, h: c_int) -> *mut pb_fb;
    pub fn pb_fb_destroy(fb: *mut pb_fb);
    pub fn pb_version_string() -> *const c_char;
    pub fn pb_version(major: *mut c_int, minor: *mut c_int, patch: *mut c_int);

    /* pb_app.h */
    pub fn pb_app_create(desc: *const pb_app_desc, user: *mut c_void) -> *mut pb_app;
    pub fn pb_app_destroy(app: *mut pb_app);
    pub fn pb_app_run(app: *mut pb_app) -> c_int;
    pub fn pb_app_quit(app: *mut pb_app);
    pub fn pb_app_request_resize(app: *mut pb_app);
    pub fn pb_app_width(app: *const pb_app) -> c_int;
    pub fn pb_app_height(app: *const pb_app) -> c_int;
    pub fn pb_app_set_title(app: *mut pb_app, title: *const c_char);
    pub fn pb_app_set_clear(app: *mut pb_app, clear: pb_cell);
    pub fn pb_app_set_target_fps(app: *mut pb_app, fps: c_int);
    pub fn pb_app_is_replay(app: *const pb_app) -> c_int;
    pub fn pb_app_is_recording(app: *const pb_app) -> c_int;
    pub fn pb_app_replay_seed(app: *const pb_app) -> u32;
    pub fn pb_app_replay_initial_size(
        app: *const pb_app,
        out_w: *mut c_int,
        out_h: *mut c_int,
    ) -> c_int;

    /* pb_state.h */
    pub fn pb_is_key_down(app: *const pb_app, key: pb_key) -> c_int;
    pub fn pb_is_key_pressed(app: *const pb_app, key: pb_key) -> c_int;
    pub fn pb_is_key_released(app: *const pb_app, key: pb_key) -> c_int;
    pub fn pb_is_char_down(app: *const pb_app, codepoint: u32) -> c_int;
    pub fn pb_is_char_pressed(app: *const pb_app, codepoint: u32) -> c_int;
    pub fn pb_get_mouse_x(app: *const pb_app) -> c_int;
    pub fn pb_get_mouse_y(app: *const pb_app) -> c_int;
    pub fn pb_is_mouse_button_down(app: *const pb_app, button: c_int) -> c_int;
    pub fn pb_is_mouse_button_pressed(app: *const pb_app, button: c_int) -> c_int;
    pub fn pb_is_mouse_button_released(app: *const pb_app, button: c_int) -> c_int;
    pub fn pb_get_mouse_wheel(app: *const pb_app) -> c_int;
    pub fn pb_get_frame_time(app: *const pb_app) -> c_double;
    pub fn pb_get_fps(app: *const pb_app) -> c_int;
    pub fn pb_is_focused(app: *const pb_app) -> c_int;
}
