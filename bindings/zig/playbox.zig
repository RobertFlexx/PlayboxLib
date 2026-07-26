//! PlayboxLib — production Zig bindings.
//! Full C ABI via @cImport plus idiomatic App / Framebuffer wrappers.
//!
//! Build demo:
//!   zig build -C bindings/zig
//! or:
//!   zig build-exe bindings/zig/demo.zig -Mroot=bindings/zig/demo.zig \
//!     --dep playbox -Mplaybox=bindings/zig/playbox.zig \
//!     -Iinclude -Lbuild/lib -lplaybox -lm -rpath build/lib \
//!     -femit-bin=build/bin/pb_zig_demo

const std = @import("std");

pub const c = @cImport({
    @cInclude("playbox/pb.h");
});

pub const Color = c.pb_color;
pub const Cell = c.pb_cell;
pub const Fb = c.pb_fb;
pub const Event = c.pb_event;
pub const Key = c.pb_key;
pub const BoxStyle = c.pb_box_style;
pub const BlendMode = c.pb_blend_mode;
pub const Sheet = c.pb_sheet;
pub const Anim = c.pb_anim;
pub const Particles = c.pb_particles;
pub const Particle = c.pb_particle;

pub const rgb = c.pb_rgb_ex;
pub const makeCell = c.pb_cell_ex;
pub const version = c.pb_version_string;
pub const charWidth = c.pb_char_width;
pub const measureText = c.pb_fb_measure_text;

// Re-export common constants for ergonomics
pub const KEY_ESC = c.PB_KEY_ESC;
pub const KEY_ENTER = c.PB_KEY_ENTER;
pub const KEY_UP = c.PB_KEY_UP;
pub const KEY_DOWN = c.PB_KEY_DOWN;
pub const KEY_LEFT = c.PB_KEY_LEFT;
pub const KEY_RIGHT = c.PB_KEY_RIGHT;
pub const KEY_TAB = c.PB_KEY_TAB;
pub const KEY_BACKSPACE = c.PB_KEY_BACKSPACE;
pub const KEY_HOME = c.PB_KEY_HOME;
pub const KEY_END = c.PB_KEY_END;
pub const KEY_F1 = c.PB_KEY_F1;

pub const EVENT_KEY = c.PB_EVENT_KEY;
pub const EVENT_TEXT = c.PB_EVENT_TEXT;
pub const EVENT_MOUSE = c.PB_EVENT_MOUSE;
pub const EVENT_RESIZE = c.PB_EVENT_RESIZE;
pub const EVENT_QUIT = c.PB_EVENT_QUIT;
pub const EVENT_FOCUS = c.PB_EVENT_FOCUS;

pub const STYLE_BOLD = c.PB_STYLE_BOLD;
pub const STYLE_DIM = c.PB_STYLE_DIM;
pub const STYLE_UNDERLINE = c.PB_STYLE_UNDERLINE;
pub const STYLE_ITALIC = c.PB_STYLE_ITALIC;

pub const BOX_SINGLE = c.PB_BOX_SINGLE;
pub const BOX_DOUBLE = c.PB_BOX_DOUBLE;
pub const BOX_ROUNDED = c.PB_BOX_ROUNDED;
pub const BOX_HEAVY = c.PB_BOX_HEAVY;
pub const BOX_ASCII = c.PB_BOX_ASCII;
pub const BOX_DASHED = c.PB_BOX_DASHED;

pub const BLEND_ALPHA = c.PB_BLEND_ALPHA;
pub const BLEND_ADD = c.PB_BLEND_ADD;
pub const BLEND_MUL = c.PB_BLEND_MUL;
pub const MOUSE_LEFT = c.PB_MOUSE_LEFT;

pub fn cell(ch: u32, fg: Color, bg: Color, style: u16) Cell {
    return makeCell(ch, fg, bg, style);
}

/// Header-inline helpers reimplemented so we do not depend on @cImport inlines.
pub fn colorFade(col: Color, a: f32) Color {
    const t = @max(0.0, @min(1.0, a));
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(col.r)) * t),
        .g = @intFromFloat(@as(f32, @floatFromInt(col.g)) * t),
        .b = @intFromFloat(@as(f32, @floatFromInt(col.b)) * t),
    };
}

pub fn colorLerp(a: Color, b: Color, t: f32) Color {
    const u = @max(0.0, @min(1.0, t));
    const lerp = struct {
        fn f(x: u8, y: u8, v: f32) u8 {
            return @intFromFloat(@as(f32, @floatFromInt(x)) + (@as(f32, @floatFromInt(y)) - @as(f32, @floatFromInt(x))) * v);
        }
    }.f;
    return .{ .r = lerp(a.r, b.r, u), .g = lerp(a.g, b.g, u), .b = lerp(a.b, b.b, u) };
}

/// Thin view over a live `pb_fb*` (never owns memory).
pub const Framebuffer = struct {
    raw: *Fb,

    pub fn from(raw: *Fb) Framebuffer {
        return .{ .raw = raw };
    }

    pub fn width(self: Framebuffer) c_int {
        return self.raw.w;
    }
    pub fn height(self: Framebuffer) c_int {
        return self.raw.h;
    }

    pub fn put(self: Framebuffer, x: c_int, y: c_int, cel: Cell) void {
        c.pb_fb_put(self.raw, x, y, cel);
    }
    pub fn putBlend(self: Framebuffer, x: c_int, y: c_int, cel: Cell, alpha: f32, mode: BlendMode) void {
        c.pb_fb_put_blend(self.raw, x, y, cel, alpha, mode);
    }
    pub fn get(self: Framebuffer, x: c_int, y: c_int) Cell {
        return c.pb_fb_get(self.raw, x, y);
    }
    pub fn clear(self: Framebuffer, fill: Cell) void {
        c.pb_fb_clear(self.raw, fill);
    }

    pub fn text(self: Framebuffer, x: c_int, y: c_int, s: [*:0]const u8, fg: Color, bg: Color, style: u16) void {
        c.pb_fb_text(self.raw, x, y, s, fg, bg, style);
    }
    pub fn textCentered(self: Framebuffer, y: c_int, s: [*:0]const u8, fg: Color, bg: Color, style: u16) void {
        c.pb_fb_text_centered(self.raw, y, s, fg, bg, style);
    }
    pub fn textWrap(self: Framebuffer, x: c_int, y: c_int, max_w: c_int, max_h: c_int, s: [*:0]const u8, fg: Color, bg: Color, style: u16) c_int {
        return c.pb_fb_text_wrap(self.raw, x, y, max_w, max_h, s, fg, bg, style);
    }
    pub fn textClipped(self: Framebuffer, x: c_int, y: c_int, max_w: c_int, s: [*:0]const u8, fg: Color, bg: Color, style: u16) void {
        c.pb_fb_text_clipped(self.raw, x, y, max_w, s, fg, bg, style);
    }

    pub fn fillRect(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, cel: Cell) void {
        c.pb_fb_fill_rect(self.raw, x, y, w, h, cel);
    }
    pub fn fillShade(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, fg: Color, bg: Color, level: c_int) void {
        c.pb_fb_fill_shade(self.raw, x, y, w, h, fg, bg, level);
    }
    pub fn hline(self: Framebuffer, x: c_int, y: c_int, w: c_int, cel: Cell) void {
        c.pb_fb_hline(self.raw, x, y, w, cel);
    }
    pub fn vline(self: Framebuffer, x: c_int, y: c_int, h: c_int, cel: Cell) void {
        c.pb_fb_vline(self.raw, x, y, h, cel);
    }
    pub fn line(self: Framebuffer, x0: c_int, y0: c_int, x1: c_int, y1: c_int, cel: Cell) void {
        c.pb_fb_line(self.raw, x0, y0, x1, y1, cel);
    }
    pub fn circle(self: Framebuffer, cx: c_int, cy: c_int, r: c_int, cel: Cell) void {
        c.pb_fb_circle(self.raw, cx, cy, r, cel);
    }
    pub fn fillCircle(self: Framebuffer, cx: c_int, cy: c_int, r: c_int, cel: Cell) void {
        c.pb_fb_fill_circle(self.raw, cx, cy, r, cel);
    }
    pub fn fillTriangle(self: Framebuffer, x0: c_int, y0: c_int, x1: c_int, y1: c_int, x2: c_int, y2: c_int, cel: Cell) void {
        c.pb_fb_fill_triangle(self.raw, x0, y0, x1, y1, x2, y2, cel);
    }

    pub fn box(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, fg: Color, bg: Color, style: u16) void {
        c.pb_fb_box(self.raw, x, y, w, h, fg, bg, style);
    }
    pub fn boxEx(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, bs: BoxStyle, fg: Color, bg: Color, style: u16) void {
        c.pb_fb_box_ex(self.raw, x, y, w, h, bs, fg, bg, style);
    }
    pub fn boxDouble(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, fg: Color, bg: Color, style: u16) void {
        c.pb_fb_box_double(self.raw, x, y, w, h, fg, bg, style);
    }
    pub fn panel(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, title: [*:0]const u8, border: Color, title_fg: Color, fill: Color, style: u16) void {
        c.pb_fb_panel(self.raw, x, y, w, h, title, border, title_fg, fill, style);
    }
    pub fn panelEx(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, title: [*:0]const u8, bs: BoxStyle, border: Color, title_fg: Color, fill: Color, style: u16) void {
        c.pb_fb_panel_ex(self.raw, x, y, w, h, title, bs, border, title_fg, fill, style);
    }
    pub fn shadow(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, col: Color, alpha: f32) void {
        c.pb_fb_shadow(self.raw, x, y, w, h, col, alpha);
    }

    pub fn blit(self: Framebuffer, dx: c_int, dy: c_int, src: *const Fb) void {
        c.pb_fb_blit(self.raw, dx, dy, src);
    }
    pub fn blitMasked(self: Framebuffer, dx: c_int, dy: c_int, src: *const Fb, transparent: u32) void {
        c.pb_fb_blit_masked(self.raw, dx, dy, src, transparent);
    }
    pub fn blitBlend(self: Framebuffer, dx: c_int, dy: c_int, src: *const Fb, alpha: f32, mode: BlendMode) void {
        c.pb_fb_blit_blend(self.raw, dx, dy, src, alpha, mode);
    }
    pub fn blitTile(self: Framebuffer, dx: c_int, dy: c_int, sheet: *const Sheet, id: c_int) void {
        c.pb_fb_blit_tile(self.raw, dx, dy, sheet, id);
    }

    pub fn plot(self: Framebuffer, px: c_int, py: c_int, col: Color) void {
        c.pb_fb_plot(self.raw, px, py, col);
    }
    pub fn plotBlend(self: Framebuffer, px: c_int, py: c_int, col: Color, alpha: f32) void {
        c.pb_fb_plot_blend(self.raw, px, py, col, alpha);
    }
    pub fn plotFillCircle(self: Framebuffer, cx: c_int, cy: c_int, r: c_int, col: Color) void {
        c.pb_fb_plot_fill_circle(self.raw, cx, cy, r, col);
    }

    pub fn braillePlot(self: Framebuffer, px: c_int, py: c_int, col: Color) void {
        c.pb_fb_braille_plot(self.raw, px, py, col);
    }
    pub fn braillePlotBlend(self: Framebuffer, px: c_int, py: c_int, col: Color, alpha: f32) void {
        c.pb_fb_braille_plot_blend(self.raw, px, py, col, alpha);
    }
    pub fn brailleLine(self: Framebuffer, x0: c_int, y0: c_int, x1: c_int, y1: c_int, col: Color) void {
        c.pb_fb_braille_line(self.raw, x0, y0, x1, y1, col);
    }
    pub fn brailleFillCircle(self: Framebuffer, cx: c_int, cy: c_int, r: c_int, col: Color) void {
        c.pb_fb_braille_fill_circle(self.raw, cx, cy, r, col);
    }
    pub fn brailleFillTriangle(self: Framebuffer, x0: c_int, y0: c_int, x1: c_int, y1: c_int, x2: c_int, y2: c_int, col: Color) void {
        c.pb_fb_braille_fill_triangle(self.raw, x0, y0, x1, y1, x2, y2, col);
    }
    pub fn brailleClear(self: Framebuffer, bg: Color) void {
        c.pb_fb_braille_clear(self.raw, bg);
    }

    pub fn quadPlot(self: Framebuffer, px: c_int, py: c_int, col: Color) void {
        c.pb_fb_quad_plot(self.raw, px, py, col);
    }
    pub fn quadFillCircle(self: Framebuffer, cx: c_int, cy: c_int, r: c_int, col: Color) void {
        c.pb_fb_quad_fill_circle(self.raw, cx, cy, r, col);
    }
    pub fn pixel(self: Framebuffer, x: c_int, y: c_int, col: Color) void {
        c.pb_fb_pixel(self.raw, x, y, col);
    }

    pub fn setCamera(self: Framebuffer, x: c_int, y: c_int) void {
        c.pb_fb_set_camera(self.raw, x, y);
    }
    pub fn setClip(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int) void {
        c.pb_fb_set_clip(self.raw, x, y, w, h);
    }
    pub fn resetClip(self: Framebuffer) void {
        c.pb_fb_reset_clip(self.raw);
    }

    pub fn fillGradientV(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, top: Color, bottom: Color) void {
        c.pb_fb_fill_gradient_v(self.raw, x, y, w, h, top, bottom);
    }
    pub fn fillGradientH(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, left: Color, right: Color) void {
        c.pb_fb_fill_gradient_h(self.raw, x, y, w, h, left, right);
    }
    pub fn fillDither(self: Framebuffer, x: c_int, y: c_int, w: c_int, h: c_int, a: Color, b: Color, pattern: c_int) void {
        c.pb_fb_fill_dither(self.raw, x, y, w, h, a, b, pattern);
    }
};

pub const OwnedSheet = struct {
    inner: Sheet,

    pub fn create(cols: c_int, rows: c_int, tw: c_int, th: c_int) OwnedSheet {
        return .{ .inner = c.pb_sheet_create(cols, rows, tw, th) };
    }
    pub fn deinit(self: *OwnedSheet) void {
        c.pb_sheet_free(&self.inner);
    }
    pub fn setTile(self: *OwnedSheet, id: c_int, src: *const Fb) void {
        c.pb_sheet_set_tile(&self.inner, id, src);
    }
    pub fn ptr(self: *OwnedSheet) *Sheet {
        return &self.inner;
    }
};

pub const OwnedParticles = struct {
    inner: Particles,

    pub fn init(capacity: c_int) !OwnedParticles {
        var ps: Particles = undefined;
        if (c.pb_particles_init(&ps, capacity) == 0) return error.OutOfMemory;
        return .{ .inner = ps };
    }
    pub fn deinit(self: *OwnedParticles) void {
        c.pb_particles_free(&self.inner);
    }
    pub fn emit(self: *OwnedParticles, x: f32, y: f32, vx: f32, vy: f32, life: f32, col: Color) void {
        c.pb_particles_emit(&self.inner, x, y, vx, vy, life, col);
    }
    pub fn update(self: *OwnedParticles, dt: f64) void {
        c.pb_particles_update(&self.inner, dt);
    }
    pub fn drawBraille(self: *OwnedParticles, fb: Framebuffer) void {
        c.pb_particles_draw_braille(fb.raw, &self.inner);
    }
    pub fn drawHalf(self: *OwnedParticles, fb: Framebuffer) void {
        c.pb_particles_draw_half(fb.raw, &self.inner);
    }
};

fn Hooks(comptime User: type) type {
    return struct {
        on_init: ?*const fn (*App(User)) void = null,
        on_event: ?*const fn (*App(User), *const Event) void = null,
        on_update: ?*const fn (*App(User), f64) void = null,
        on_draw: ?*const fn (*App(User), Framebuffer) void = null,
        on_shutdown: ?*const fn (*App(User)) void = null,
    };
}

/// Idiomatic app wrapper. `User` is your game-state type stored by value.
pub fn App(comptime User: type) type {
    return struct {
        const Self = @This();

        raw: *c.pb_app,
        user: User,
        hooks: Hooks(User),
        title_buf: [256]u8 = undefined,
        title_z: [:0]u8 = undefined,

        pub fn init(title: []const u8, target_fps: c_int, user: User) !Self {
            var self: Self = undefined;
            self.user = user;
            self.hooks = .{};
            const n = @min(title.len, self.title_buf.len - 1);
            @memcpy(self.title_buf[0..n], title[0..n]);
            self.title_buf[n] = 0;
            self.title_z = self.title_buf[0..n :0];

            var desc = std.mem.zeroes(c.pb_app_desc);
            desc.title = self.title_z.ptr;
            desc.target_fps = target_fps;
            desc.on_init = trampInit;
            desc.on_event = trampEvent;
            desc.on_update = trampUpdate;
            desc.on_draw = trampDraw;
            desc.on_shutdown = trampShutdown;

            // Temporarily create without user; we set CURRENT before run.
            const raw = c.pb_app_create(&desc, null) orelse return error.AppCreateFailed;
            self.raw = raw;
            return self;
        }

        pub fn deinit(self: *Self) void {
            c.pb_app_destroy(self.raw);
        }

        pub fn onInit(self: *Self, f: *const fn (*Self) void) void {
            self.hooks.on_init = f;
        }
        pub fn onEvent(self: *Self, f: *const fn (*Self, *const Event) void) void {
            self.hooks.on_event = f;
        }
        pub fn onUpdate(self: *Self, f: *const fn (*Self, f64) void) void {
            self.hooks.on_update = f;
        }
        pub fn onDraw(self: *Self, f: *const fn (*Self, Framebuffer) void) void {
            self.hooks.on_draw = f;
        }
        pub fn onShutdown(self: *Self, f: *const fn (*Self) void) void {
            self.hooks.on_shutdown = f;
        }

        pub fn run(self: *Self) c_int {
            current = self;
            defer current = null;
            return c.pb_app_run(self.raw);
        }

        pub fn quit(self: *Self) void {
            c.pb_app_quit(self.raw);
        }
        pub fn width(self: *const Self) c_int {
            return c.pb_app_width(self.raw);
        }
        pub fn height(self: *const Self) c_int {
            return c.pb_app_height(self.raw);
        }
        pub fn fps(self: *const Self) c_int {
            return c.pb_get_fps(self.raw);
        }
        pub fn frameTime(self: *const Self) f64 {
            return c.pb_get_frame_time(self.raw);
        }
        pub fn setTitle(self: *Self, title: []const u8) void {
            const n = @min(title.len, self.title_buf.len - 1);
            @memcpy(self.title_buf[0..n], title[0..n]);
            self.title_buf[n] = 0;
            self.title_z = self.title_buf[0..n :0];
            c.pb_app_set_title(self.raw, self.title_z.ptr);
        }
        pub fn setClear(self: *Self, fill: Cell) void {
            c.pb_app_set_clear(self.raw, fill);
        }
        pub fn setTargetFps(self: *Self, fps_v: c_int) void {
            c.pb_app_set_target_fps(self.raw, fps_v);
        }

        pub fn isKeyDown(self: *const Self, key: Key) bool {
            return c.pb_is_key_down(self.raw, key) != 0;
        }
        pub fn isKeyPressed(self: *const Self, key: Key) bool {
            return c.pb_is_key_pressed(self.raw, key) != 0;
        }
        pub fn isKeyReleased(self: *const Self, key: Key) bool {
            return c.pb_is_key_released(self.raw, key) != 0;
        }
        pub fn isCharDown(self: *const Self, cp: u32) bool {
            return c.pb_is_char_down(self.raw, cp) != 0;
        }
        pub fn isCharPressed(self: *const Self, cp: u32) bool {
            return c.pb_is_char_pressed(self.raw, cp) != 0;
        }
        pub fn mouseX(self: *const Self) c_int {
            return c.pb_get_mouse_x(self.raw);
        }
        pub fn mouseY(self: *const Self) c_int {
            return c.pb_get_mouse_y(self.raw);
        }
        pub fn isMouseDown(self: *const Self, button: c_int) bool {
            return c.pb_is_mouse_button_down(self.raw, button) != 0;
        }
        pub fn mouseWheel(self: *const Self) c_int {
            return c.pb_get_mouse_wheel(self.raw);
        }
        pub fn focused(self: *const Self) bool {
            return c.pb_is_focused(self.raw) != 0;
        }
        pub fn isReplay(self: *const Self) bool {
            return c.pb_app_is_replay(self.raw) != 0;
        }
        pub fn replaySeed(self: *const Self) u32 {
            return c.pb_app_replay_seed(self.raw);
        }

        // --- C trampolines (one App type at a time via threadlocal) ---
        threadlocal var current: ?*Self = null;

        fn trampInit(_: ?*c.pb_app, _: ?*anyopaque) callconv(.c) void {
            const self = current orelse return;
            if (self.hooks.on_init) |f| f(self);
        }
        fn trampEvent(_: ?*c.pb_app, _: ?*anyopaque, ev: ?*const Event) callconv(.c) void {
            const self = current orelse return;
            if (ev) |e| {
                if (self.hooks.on_event) |f| f(self, e);
            }
        }
        fn trampUpdate(_: ?*c.pb_app, _: ?*anyopaque, dt: f64) callconv(.c) void {
            const self = current orelse return;
            if (self.hooks.on_update) |f| f(self, dt);
        }
        fn trampDraw(_: ?*c.pb_app, _: ?*anyopaque, fb: ?*Fb) callconv(.c) void {
            const self = current orelse return;
            if (fb) |f| {
                if (self.hooks.on_draw) |cb| cb(self, Framebuffer.from(f));
            }
        }
        fn trampShutdown(_: ?*c.pb_app, _: ?*anyopaque) callconv(.c) void {
            const self = current orelse return;
            if (self.hooks.on_shutdown) |f| f(self);
        }
    };
}

/// Low-level helpers kept for demos that use raw C callbacks.
pub fn create(desc: *const c.pb_app_desc, user: ?*anyopaque) ?*c.pb_app {
    return c.pb_app_create(desc, user);
}
pub fn destroy(app: *c.pb_app) void {
    c.pb_app_destroy(app);
}
pub fn run(app: *c.pb_app) c_int {
    return c.pb_app_run(app);
}
pub fn quit(app: *c.pb_app) void {
    c.pb_app_quit(app);
}
