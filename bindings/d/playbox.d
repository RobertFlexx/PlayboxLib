/**
 * PlayboxLib — production D bindings.
 *
 * Low-level `extern(C)` ABI + idiomatic `App` / `Framebuffer` wrappers.
 *
 * Build demo:
 *   dub run --root bindings/d
 * or:
 *   dmd -fPIC -Ibindings/d bindings/d/demo.d bindings/d/playbox.d \
 *     -L-Lbuild/lib -L-lplaybox -L-lm -L-rpath=build/lib -ofbuild/bin/pb_d_demo
 */
module playbox;

import core.stdc.string : memset, strlen;
import std.string : toStringz;
import std.conv : to;

// ---------------------------------------------------------------------------
// ABI
// ---------------------------------------------------------------------------

extern (C) {
    struct pb_color { ubyte r, g, b; }
    struct pb_cell {
        uint ch;
        pb_color fg;
        pb_color bg;
        ushort style;
    }
    struct pb_fb {
        int w, h;
        pb_cell* cells;
        int cam_x, cam_y;
        int clip_x0, clip_y0, clip_x1, clip_y1;
    }
    struct pb_app;

    struct pb_key_event {
        int key;
        uint codepoint;
        ubyte alt, ctrl, shift, pressed;
    }
    struct pb_mouse_event {
        int x, y;
        ubyte button, pressed;
        int wheel;
        ubyte shift, alt, ctrl;
    }
    struct pb_resize_event { int width, height; }
    struct pb_focus_event { ubyte focused; }

    union pb_event_data {
        pb_key_event key;
        uint text;
        pb_mouse_event mouse;
        pb_resize_event resize;
        pb_focus_event focus;
    }
    struct pb_event {
        int type;
        pb_event_data as;
    }

    alias pb_on_init_fn = void function(pb_app*, void*);
    alias pb_on_event_fn = void function(pb_app*, void*, const(pb_event)*);
    alias pb_on_update_fn = void function(pb_app*, void*, double);
    alias pb_on_draw_fn = void function(pb_app*, void*, pb_fb*);
    alias pb_on_shutdown_fn = void function(pb_app*, void*);

    struct pb_app_desc {
        const(char)* title;
        int target_fps;
        uint flags;
        pb_cell clear;
        pb_on_init_fn on_init;
        pb_on_event_fn on_event;
        pb_on_update_fn on_update;
        pb_on_draw_fn on_draw;
        pb_on_shutdown_fn on_shutdown;
    }

    struct pb_sheet {
        pb_fb atlas;
        int tile_w, tile_h, cols, rows, owns_atlas;
    }
    struct pb_anim {
        const(int)* frames;
        int frame_count;
        float fps, t;
        int loop; // matches C pb_anim.loop
    }
    struct pb_particle {
        float x, y, vx, vy, life, max_life;
        pb_color color;
        ubyte alive;
    }
    struct pb_particles {
        pb_particle* items;
        int count, capacity;
    }

    enum {
        PB_STYLE_BOLD = 1, PB_STYLE_DIM = 2, PB_STYLE_UNDERLINE = 4,
        PB_STYLE_REVERSE = 8, PB_STYLE_ITALIC = 16, PB_STYLE_STRIKETHROUGH = 32
    }
    enum {
        PB_KEY_NONE=0, PB_KEY_ESC, PB_KEY_ENTER, PB_KEY_BACKSPACE, PB_KEY_TAB,
        PB_KEY_UP, PB_KEY_DOWN, PB_KEY_LEFT, PB_KEY_RIGHT,
        PB_KEY_HOME, PB_KEY_END, PB_KEY_PGUP, PB_KEY_PGDN, PB_KEY_INS, PB_KEY_DEL,
        PB_KEY_F1, PB_KEY_F2, PB_KEY_F3, PB_KEY_F4, PB_KEY_F5, PB_KEY_F6,
        PB_KEY_F7, PB_KEY_F8, PB_KEY_F9, PB_KEY_F10, PB_KEY_F11, PB_KEY_F12, PB_KEY_COUNT
    }
    enum { PB_MOUSE_LEFT=0, PB_MOUSE_MIDDLE=1, PB_MOUSE_RIGHT=2 }
    enum {
        PB_EVENT_NONE=0, PB_EVENT_KEY, PB_EVENT_TEXT, PB_EVENT_MOUSE,
        PB_EVENT_RESIZE, PB_EVENT_QUIT, PB_EVENT_FOCUS
    }
    enum {
        PB_BOX_SINGLE=0, PB_BOX_DOUBLE, PB_BOX_ROUNDED, PB_BOX_HEAVY, PB_BOX_ASCII, PB_BOX_DASHED
    }
    enum { PB_BLEND_REPLACE=0, PB_BLEND_ALPHA, PB_BLEND_ADD, PB_BLEND_MUL }

    pb_color pb_rgb_ex(ubyte r, ubyte g, ubyte b);
    pb_cell pb_cell_ex(uint ch, pb_color fg, pb_color bg, ushort style);
    const(char)* pb_version_string();
    void pb_version(int* major, int* minor, int* patch);
    int pb_char_width(uint codepoint);

    pb_app* pb_app_create(const(pb_app_desc)* desc, void* user);
    void pb_app_destroy(pb_app* app);
    int pb_app_run(pb_app* app);
    void pb_app_quit(pb_app* app);
    void pb_app_request_resize(pb_app* app);
    int pb_app_width(const(pb_app)* app);
    int pb_app_height(const(pb_app)* app);
    void pb_app_set_title(pb_app* app, const(char)* title);
    void pb_app_set_clear(pb_app* app, pb_cell clear);
    void pb_app_set_target_fps(pb_app* app, int fps);
    int pb_app_is_replay(const(pb_app)* app);
    int pb_app_is_recording(const(pb_app)* app);
    uint pb_app_replay_seed(const(pb_app)* app);

    int pb_is_key_down(const(pb_app)* app, int key);
    int pb_is_key_pressed(const(pb_app)* app, int key);
    int pb_is_key_released(const(pb_app)* app, int key);
    int pb_is_char_down(const(pb_app)* app, uint cp);
    int pb_is_char_pressed(const(pb_app)* app, uint cp);
    int pb_get_mouse_x(const(pb_app)* app);
    int pb_get_mouse_y(const(pb_app)* app);
    int pb_is_mouse_button_down(const(pb_app)* app, int button);
    int pb_is_mouse_button_pressed(const(pb_app)* app, int button);
    int pb_is_mouse_button_released(const(pb_app)* app, int button);
    int pb_get_mouse_wheel(const(pb_app)* app);
    int pb_get_fps(const(pb_app)* app);
    double pb_get_frame_time(const(pb_app)* app);
    int pb_is_focused(const(pb_app)* app);

    void pb_fb_clear(pb_fb* fb, pb_cell fill);
    void pb_fb_put(pb_fb* fb, int x, int y, pb_cell c);
    pb_cell pb_fb_get(const(pb_fb)* fb, int x, int y);
    void pb_fb_put_blend(pb_fb* fb, int x, int y, pb_cell c, float alpha, int mode);
    void pb_fb_text(pb_fb* fb, int x, int y, const(char)* utf8, pb_color fg, pb_color bg, ushort style);
    void pb_fb_text_centered(pb_fb* fb, int y, const(char)* utf8, pb_color fg, pb_color bg, ushort style);
    int pb_fb_text_wrap(pb_fb* fb, int x, int y, int max_w, int max_h, const(char)* utf8, pb_color fg, pb_color bg, ushort style);
    void pb_fb_text_clipped(pb_fb* fb, int x, int y, int max_w, const(char)* utf8, pb_color fg, pb_color bg, ushort style);
    int pb_fb_measure_text(const(char)* utf8);
    void pb_fb_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_cell c);
    void pb_fb_fill_shade(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, int level);
    void pb_fb_hline(pb_fb* fb, int x, int y, int w, pb_cell c);
    void pb_fb_vline(pb_fb* fb, int x, int y, int h, pb_cell c);
    void pb_fb_line(pb_fb* fb, int x0, int y0, int x1, int y1, pb_cell c);
    void pb_fb_circle(pb_fb* fb, int cx, int cy, int radius, pb_cell c);
    void pb_fb_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_cell c);
    void pb_fb_fill_triangle(pb_fb* fb, int x0, int y0, int x1, int y1, int x2, int y2, pb_cell c);
    void pb_fb_box(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, ushort style);
    void pb_fb_box_double(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, ushort style);
    void pb_fb_box_ex(pb_fb* fb, int x, int y, int w, int h, int box_style, pb_color fg, pb_color bg, ushort style);
    void pb_fb_panel(pb_fb* fb, int x, int y, int w, int h, const(char)* title, pb_color border, pb_color title_fg, pb_color fill, ushort style);
    void pb_fb_panel_ex(pb_fb* fb, int x, int y, int w, int h, const(char)* title, int box_style, pb_color border, pb_color title_fg, pb_color fill, ushort style);
    void pb_fb_shadow(pb_fb* fb, int x, int y, int w, int h, pb_color shadow, float alpha);
    void pb_fb_blit(pb_fb* dst, int dx, int dy, const(pb_fb)* src);
    void pb_fb_blit_masked(pb_fb* dst, int dx, int dy, const(pb_fb)* src, uint transparent_ch);
    void pb_fb_blit_blend(pb_fb* dst, int dx, int dy, const(pb_fb)* src, float alpha, int mode);
    void pb_fb_blit_tile(pb_fb* dst, int dx, int dy, const(pb_sheet)* sheet, int tile_id);
    void pb_fb_plot(pb_fb* fb, int px, int py, pb_color color);
    void pb_fb_plot_blend(pb_fb* fb, int px, int py, pb_color color, float alpha);
    void pb_fb_plot_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color);
    void pb_fb_braille_clear(pb_fb* fb, pb_color bg);
    void pb_fb_braille_plot(pb_fb* fb, int px, int py, pb_color color);
    void pb_fb_braille_plot_blend(pb_fb* fb, int px, int py, pb_color color, float alpha);
    void pb_fb_braille_line(pb_fb* fb, int x0, int y0, int x1, int y1, pb_color color);
    void pb_fb_braille_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color);
    void pb_fb_braille_fill_triangle(pb_fb* fb, int x0, int y0, int x1, int y1, int x2, int y2, pb_color color);
    void pb_fb_quad_plot(pb_fb* fb, int px, int py, pb_color color);
    void pb_fb_quad_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color);
    void pb_fb_pixel(pb_fb* fb, int x, int y, pb_color color);
    void pb_fb_set_camera(pb_fb* fb, int cam_x, int cam_y);
    void pb_fb_get_camera(const(pb_fb)* fb, int* out_x, int* out_y);
    void pb_fb_set_clip(pb_fb* fb, int x, int y, int w, int h);
    void pb_fb_reset_clip(pb_fb* fb);
    void pb_fb_fill_gradient_v(pb_fb* fb, int x, int y, int w, int h, pb_color top, pb_color bottom);
    void pb_fb_fill_gradient_h(pb_fb* fb, int x, int y, int w, int h, pb_color left, pb_color right);
    void pb_fb_fill_dither(pb_fb* fb, int x, int y, int w, int h, pb_color a, pb_color b, int pattern);

    pb_sheet pb_sheet_create(int cols, int rows, int tile_w, int tile_h);
    void pb_sheet_free(pb_sheet* sheet);
    void pb_sheet_set_tile(pb_sheet* sheet, int tile_id, const(pb_fb)* src);

    int pb_particles_init(pb_particles* ps, int capacity);
    void pb_particles_free(pb_particles* ps);
    void pb_particles_emit(pb_particles* ps, float x, float y, float vx, float vy, float life, pb_color color);
    void pb_particles_update(pb_particles* ps, double dt);
    void pb_particles_draw_braille(pb_fb* fb, const(pb_particles)* ps);
    void pb_particles_draw_half(pb_fb* fb, const(pb_particles)* ps);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

pb_color rgb(ubyte r, ubyte g, ubyte b) { return pb_rgb_ex(r, g, b); }
pb_cell cell(uint ch, pb_color fg, pb_color bg, ushort style = 0) {
    return pb_cell_ex(ch, fg, bg, style);
}
string versionString() {
    import std.string : fromStringz;
    return fromStringz(pb_version_string()).idup;
}

// ---------------------------------------------------------------------------
// Idiomatic Framebuffer view
// ---------------------------------------------------------------------------

struct Framebuffer {
    pb_fb* raw;

    @property int width() const { return raw ? raw.w : 0; }
    @property int height() const { return raw ? raw.h : 0; }

    void put(int x, int y, pb_cell c) { pb_fb_put(raw, x, y, c); }
    void putBlend(int x, int y, pb_cell c, float alpha, int mode = PB_BLEND_ALPHA) {
        pb_fb_put_blend(raw, x, y, c, alpha, mode);
    }
    pb_cell get(int x, int y) { return pb_fb_get(raw, x, y); }
    void clear(pb_cell fill) { pb_fb_clear(raw, fill); }

    void text(int x, int y, string s, pb_color fg, pb_color bg, ushort style = 0) {
        pb_fb_text(raw, x, y, s.toStringz, fg, bg, style);
    }
    void textCentered(int y, string s, pb_color fg, pb_color bg, ushort style = 0) {
        pb_fb_text_centered(raw, y, s.toStringz, fg, bg, style);
    }
    int textWrap(int x, int y, int maxW, int maxH, string s, pb_color fg, pb_color bg, ushort style = 0) {
        return pb_fb_text_wrap(raw, x, y, maxW, maxH, s.toStringz, fg, bg, style);
    }
    void textClipped(int x, int y, int maxW, string s, pb_color fg, pb_color bg, ushort style = 0) {
        pb_fb_text_clipped(raw, x, y, maxW, s.toStringz, fg, bg, style);
    }

    void fillRect(int x, int y, int w, int h, pb_cell c) { pb_fb_fill_rect(raw, x, y, w, h, c); }
    void fillShade(int x, int y, int w, int h, pb_color fg, pb_color bg, int level) {
        pb_fb_fill_shade(raw, x, y, w, h, fg, bg, level);
    }
    void line(int x0, int y0, int x1, int y1, pb_cell c) { pb_fb_line(raw, x0, y0, x1, y1, c); }
    void circle(int cx, int cy, int r, pb_cell c) { pb_fb_circle(raw, cx, cy, r, c); }
    void fillCircle(int cx, int cy, int r, pb_cell c) { pb_fb_fill_circle(raw, cx, cy, r, c); }
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, pb_cell c) {
        pb_fb_fill_triangle(raw, x0, y0, x1, y1, x2, y2, c);
    }

    void boxEx(int x, int y, int w, int h, int bs, pb_color fg, pb_color bg, ushort style = 0) {
        pb_fb_box_ex(raw, x, y, w, h, bs, fg, bg, style);
    }
    void panelEx(int x, int y, int w, int h, string title, int bs,
                 pb_color border, pb_color titleFg, pb_color fill, ushort style = 0) {
        pb_fb_panel_ex(raw, x, y, w, h, title.toStringz, bs, border, titleFg, fill, style);
    }
    void shadow(int x, int y, int w, int h, pb_color sh, float alpha = 0.4f) {
        pb_fb_shadow(raw, x, y, w, h, sh, alpha);
    }

    void blitBlend(int dx, int dy, pb_fb* src, float alpha, int mode = PB_BLEND_ALPHA) {
        pb_fb_blit_blend(raw, dx, dy, src, alpha, mode);
    }
    void plot(int px, int py, pb_color c) { pb_fb_plot(raw, px, py, c); }
    void plotBlend(int px, int py, pb_color c, float a) { pb_fb_plot_blend(raw, px, py, c, a); }
    void braillePlot(int px, int py, pb_color c) { pb_fb_braille_plot(raw, px, py, c); }
    void brailleFillCircle(int cx, int cy, int r, pb_color c) { pb_fb_braille_fill_circle(raw, cx, cy, r, c); }
    void brailleFillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, pb_color c) {
        pb_fb_braille_fill_triangle(raw, x0, y0, x1, y1, x2, y2, c);
    }
    void quadFillCircle(int cx, int cy, int r, pb_color c) { pb_fb_quad_fill_circle(raw, cx, cy, r, c); }

    void setCamera(int x, int y) { pb_fb_set_camera(raw, x, y); }
    void setClip(int x, int y, int w, int h) { pb_fb_set_clip(raw, x, y, w, h); }
    void resetClip() { pb_fb_reset_clip(raw); }
    void fillGradientV(int x, int y, int w, int h, pb_color top, pb_color bottom) {
        pb_fb_fill_gradient_v(raw, x, y, w, h, top, bottom);
    }
    void fillGradientH(int x, int y, int w, int h, pb_color left, pb_color right) {
        pb_fb_fill_gradient_h(raw, x, y, w, h, left, right);
    }
}

// ---------------------------------------------------------------------------
// Idiomatic App
// ---------------------------------------------------------------------------

alias OnInit = void delegate();
alias OnEvent = void delegate(ref const(pb_event));
alias OnUpdate = void delegate(double);
alias OnDraw = void delegate(Framebuffer);
alias OnShutdown = void delegate();

final class App {
    private pb_app* raw_;
    private string title_;
    private static App current_;

    OnInit onInit;
    OnEvent onEvent;
    OnUpdate onUpdate;
    OnDraw onDraw;
    OnShutdown onShutdown;

    this(string title, int targetFps = 60) {
        title_ = title;
        pb_app_desc desc;
        memset(&desc, 0, desc.sizeof);
        desc.title = title_.toStringz;
        desc.target_fps = targetFps;
        desc.on_init = &trampInit;
        desc.on_event = &trampEvent;
        desc.on_update = &trampUpdate;
        desc.on_draw = &trampDraw;
        desc.on_shutdown = &trampShutdown;
        raw_ = pb_app_create(&desc, null);
        if (!raw_) throw new Exception("pb_app_create failed");
    }

    ~this() {
        if (raw_) {
            pb_app_destroy(raw_);
            raw_ = null;
        }
    }

    int run() {
        current_ = this;
        scope(exit) current_ = null;
        return pb_app_run(raw_);
    }

    void quit() { pb_app_quit(raw_); }
    @property int width() { return pb_app_width(raw_); }
    @property int height() { return pb_app_height(raw_); }
    @property int fps() { return pb_get_fps(raw_); }
    @property double frameTime() { return pb_get_frame_time(raw_); }

    void setTitle(string t) {
        title_ = t;
        pb_app_set_title(raw_, title_.toStringz);
    }
    void setClear(pb_cell c) { pb_app_set_clear(raw_, c); }
    void setTargetFps(int fps) { pb_app_set_target_fps(raw_, fps); }

    bool isKeyDown(int key) { return pb_is_key_down(raw_, key) != 0; }
    bool isKeyPressed(int key) { return pb_is_key_pressed(raw_, key) != 0; }
    bool isKeyReleased(int key) { return pb_is_key_released(raw_, key) != 0; }
    bool isCharDown(uint cp) { return pb_is_char_down(raw_, cp) != 0; }
    bool isCharPressed(uint cp) { return pb_is_char_pressed(raw_, cp) != 0; }
    int mouseX() { return pb_get_mouse_x(raw_); }
    int mouseY() { return pb_get_mouse_y(raw_); }
    bool isMouseDown(int button) { return pb_is_mouse_button_down(raw_, button) != 0; }
    int mouseWheel() { return pb_get_mouse_wheel(raw_); }
    bool focused() { return pb_is_focused(raw_) != 0; }
    bool isReplay() { return pb_app_is_replay(raw_) != 0; }
    uint replaySeed() { return pb_app_replay_seed(raw_); }

    private extern (C) static void trampInit(pb_app*, void*) {
        if (current_ && current_.onInit) current_.onInit();
    }
    private extern (C) static void trampEvent(pb_app*, void*, const(pb_event)* ev) {
        if (current_ && current_.onEvent && ev) current_.onEvent(*ev);
    }
    private extern (C) static void trampUpdate(pb_app*, void*, double dt) {
        if (current_ && current_.onUpdate) current_.onUpdate(dt);
    }
    private extern (C) static void trampDraw(pb_app*, void*, pb_fb* fb) {
        if (current_ && current_.onDraw && fb) current_.onDraw(Framebuffer(fb));
    }
    private extern (C) static void trampShutdown(pb_app*, void*) {
        if (current_ && current_.onShutdown) current_.onShutdown();
    }
}

/// Simple particle system wrapper.
final class Particles {
    private pb_particles ps_;
    this(int capacity = 256) {
        if (!pb_particles_init(&ps_, capacity)) throw new Exception("particles init failed");
    }
    ~this() { pb_particles_free(&ps_); }
    void emit(float x, float y, float vx, float vy, float life, pb_color color) {
        pb_particles_emit(&ps_, x, y, vx, vy, life, color);
    }
    void update(double dt) { pb_particles_update(&ps_, dt); }
    void drawBraille(Framebuffer fb) { pb_particles_draw_braille(fb.raw, &ps_); }
    void drawHalf(Framebuffer fb) { pb_particles_draw_half(fb.raw, &ps_); }
}
