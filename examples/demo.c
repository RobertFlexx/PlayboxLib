#include "playbox/pb.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    double t;
    double x;
    double y;
    double vx;
    double vy;
    int show_help;
    int mx;
    int my;
    pb_sheet sheet;
    pb_anim anim;
    int anim_frames[4];
    pb_cam2d cam;
} demo_state;

static int clampi(int v, int lo, int hi){
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static double clampd(double v, double lo, double hi){
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static void on_event(pb_app* app, void* user, const pb_event* ev){
    demo_state* s = (demo_state*)user;
    if(ev->type == PB_EVENT_QUIT){
        pb_app_quit(app);
        return;
    }
    if(ev->type == PB_EVENT_KEY && ev->as.key.pressed){
        pb_key k = ev->as.key.key;
        uint32_t cp = ev->as.key.codepoint;

        if(k == PB_KEY_ESC || cp=='q' || cp=='Q'){
            pb_app_quit(app);
            return;
        }
        if(cp=='h' || cp=='H'){
            s->show_help = !s->show_help;
            return;
        }
    }
}

static void on_update(pb_app* app, void* user, double dt){
    demo_state* s = (demo_state*)user;

    if(dt < 0.0) dt = 0.0;
    if(dt > 0.25) dt = 0.25;

    s->t += dt;
    pb_anim_update(&s->anim, dt);
    s->mx = pb_get_mouse_x(app);
    s->my = pb_get_mouse_y(app);

    /* Arrow keys via polling — no manual event bookkeeping. */
    double speed = 40.0;
    if(pb_is_key_down(app, PB_KEY_LEFT) || pb_is_char_down(app, 'a')) s->vx = -speed;
    if(pb_is_key_down(app, PB_KEY_RIGHT) || pb_is_char_down(app, 'd')) s->vx = speed;
    if(pb_is_key_down(app, PB_KEY_UP) || pb_is_char_down(app, 'w')) s->vy = -speed;
    if(pb_is_key_down(app, PB_KEY_DOWN) || pb_is_char_down(app, 's')) s->vy = speed;

    int w = pb_app_width(app);
    int h = pb_app_height(app);
    if(w < 1) w = 1;
    if(h < 1) h = 1;

    double minx = 1.0;
    double miny = 1.0;
    double maxx = (double)(w - 2);
    double maxy = (double)(h - 2);

    if(maxx < minx) maxx = minx;
    if(maxy < miny) maxy = miny;

    s->x += s->vx * dt;
    s->y += s->vy * dt;

    if(s->x < minx){ s->x = minx; s->vx = fabs(s->vx); }
    if(s->x > maxx){ s->x = maxx; s->vx = -fabs(s->vx); }
    if(s->y < miny){ s->y = miny; s->vy = fabs(s->vy); }
    if(s->y > maxy){ s->y = maxy; s->vy = -fabs(s->vy); }

    s->x = clampd(s->x, minx, maxx);
    s->y = clampd(s->y, miny, maxy);
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    demo_state* s = (demo_state*)user;

    int w = fb->w;
    int h = fb->h;
    if(w < 1) w = 1;
    if(h < 1) h = 1;

    pb_color bg = pb_rgb(10, 12, 18);
    pb_color fg = pb_rgb(220, 224, 230);
    pb_color neon = pb_rgb(80, 200, 255);
    pb_color mag = pb_rgb(255, 110, 200);
    pb_color panel = pb_rgb(18, 22, 30);
    pb_color border = pb_rgb(70, 85, 105);
    pb_color accent = pb_rgb(120, 255, 180);
    pb_color dim = pb_rgb(90, 100, 120);

    /* Soft vertical gradient + shade accent strip */
    pb_fb_fill_gradient_v(fb, 0, 0, w, h, pb_rgb(8, 10, 16), pb_rgb(22, 28, 48));
    pb_fb_fill_shade(fb, 0, h - 3, w, 1, pb_rgb(40, 60, 90), pb_rgb(12, 14, 22), 2);

    /* Braille + quadrant hi-res art */
    {
        int bcx = w;
        int bcy = h * 2;
        float ang = (float)s->t * 1.4f;
        for(int i = 0; i < 48; i++){
            float a = ang + (float)i * 0.13f;
            float rad = 8.f + (float)(i % 9);
            int px = bcx + (int)(cosf(a) * rad * 2.2f);
            int py = bcy + (int)(sinf(a) * rad * 2.2f);
            pb_fb_braille_plot_blend(fb, px, py, neon, 0.25f + (float)(i % 6) * 0.1f);
        }
        pb_fb_braille_fill_circle(fb, bcx + (int)(cosf(ang) * 28), bcy + (int)(sinf(ang) * 20), 7, mag);
        pb_fb_quad_fill_circle(fb, w + (int)(cosf(ang * 0.7f) * 18), h + (int)(sinf(ang * 0.7f) * 12), 5, accent);
        pb_fb_braille_fill_triangle(fb,
            bcx - 20, bcy + 10,
            bcx - 5, bcy - 8,
            bcx - 35, bcy - 4,
            pb_color_fade(neon, 0.55f));
        pb_fb_braille_rect_rotated(fb, (float)(bcx + 30), (float)(bcy - 12),
                                   18.f, 10.f, ang * 0.8f, pb_color_fade(accent, 0.7f), 1);
        pb_fb_braille_line_rotated(fb, (float)bcx, (float)bcy, 40.f, ang, neon);
    }

    if(w >= 2 && h >= 2){
        pb_fb_box_ex(fb, 0, 0, w, h, PB_BOX_ROUNDED, border, bg, 0);
    }

    pb_fb_textf(fb, 2, 0, neon, bg, PB_STYLE_BOLD,
                " PlayboxLib  %dx%d  %d FPS  |  WASD/arrows  H help  ESC quit ",
                fb->w, fb->h, pb_get_fps(app));

    /* Decorative geometry */
    int cx = w / 2;
    int cy = h / 2;
    pb_cell ring = pb_cell_make('*', pb_color_fade(neon, 0.55f), bg, 0);
    pb_fb_circle(fb, cx, cy, clampi(h / 3, 3, 14), ring);

    float ang = (float)s->t * 1.4f;
    int lx = cx + (int)(cosf(ang) * (float)(w / 4));
    int ly = cy + (int)(sinf(ang) * (float)(h / 4));
    pb_fb_line(fb, cx, cy, lx, ly, pb_cell_make(0x00B7u, accent, bg, 0));

    pb_fb_fill_triangle(fb,
        cx - 6, cy + 4,
        cx + 6, cy + 4,
        cx, cy - 6,
        pb_cell_make(0x25B2u, pb_color_fade(mag, 0.7f), bg, 0));

    /* Half-block orbit trail */
    int pcx = w / 2;
    int pcy = h;
    int pr = clampi(h / 2, 4, 20);
    for(int i = 0; i < 24; i++){
        float a = ang - (float)i * 0.12f;
        float fade = 1.f - (float)i / 24.f;
        int px = pcx + (int)(cosf(a) * (float)pr);
        int py = pcy + (int)(sinf(a) * (float)pr);
        pb_fb_plot_blend(fb, px, py, mag, fade);
    }
    pb_fb_plot_fill_circle(fb, pcx + (int)(cosf(ang) * (float)pr),
                           pcy + (int)(sinf(ang) * (float)pr), 2, mag);

    /* Sprite sheet + anim (tiny usage demo) */
    if(s->sheet.atlas.cells){
        int tile = pb_anim_frame(&s->anim);
        pb_fb_blit_tile(fb, w - 8, 2, &s->sheet, tile);
    }

    pb_fb_set_clip(fb, 1, h - 1, w - 2, 1);
    pb_fb_textf(fb, 2, h - 1, dim, bg, 0,
                " mouse %d,%d  focus=%s  dt=%.3f  see also: pb_cube3d pb_ui_demo ",
                s->mx, s->my, pb_is_focused(app) ? "yes" : "no", pb_get_frame_time(app));
    pb_fb_reset_clip(fb);

    int bx = clampi((int)(s->x + 0.5), 0, fb->w - 1);
    int by = clampi((int)(s->y + 0.5), 0, fb->h - 1);
    pb_fb_put(fb, bx, by, pb_cell_make('@', mag, bg, PB_STYLE_BOLD));

    if(s->mx >= 0 && s->my >= 0 && s->mx < w && s->my < h){
        pb_fb_put(fb, s->mx, s->my, pb_cell_make('+', accent, bg, 0));
    }

    if(s->show_help && w >= 10 && h >= 6){
        int hw = clampi((w < 64) ? (w - 2) : 60, 8, w - 2);
        int hh = clampi((h < 14) ? (h - 2) : 12, 4, h - 2);
        int hx = (w - hw) / 2;
        int hy = (h - hh) / 2;

        pb_fb_shadow(fb, hx, hy, hw, hh, pb_rgb(0, 0, 0), 0.4f);
        pb_fb_panel_ex(fb, hx, hy, hw, hh, "PlayboxLib", PB_BOX_ROUNDED, neon, fg, panel, 0);

        int tx = hx + 2;
        int ty = hy + 2;
        if(hw > 6 && hh > 4){
            pb_fb_text_wrap(fb, tx, ty, hw - 4, hh - 4,
                "TUI raylib: poll input, braille/quad, soft 3D (pb_cube3d), UI widgets (pb_ui_demo).\n"
                "Also: cam2d, sheets/anim, particles, rounded panels.",
                fg, panel, 0);
            pb_fb_text(fb, tx, hy + hh - 2, "Press H to close.", neon, panel, 0);
        }
    }
}
int main(void){
    demo_state s;
    memset(&s, 0, sizeof(s));

    s.x = 10.0;
    s.y = 5.0;
    s.vx = 22.0;
    s.vy = 14.0;
    s.show_help = 1;
    pb_cam2d_init(&s.cam);
    s.cam.zoom = 1.0f;

    s.sheet = pb_sheet_create(4, 1, 3, 2);
    if(s.sheet.atlas.cells){
        pb_color cols[4] = {
            pb_rgb(90, 200, 255), pb_rgb(255, 110, 200),
            pb_rgb(120, 255, 180), pb_rgb(255, 200, 90)
        };
        for(int i = 0; i < 4; i++){
            pb_fb tile = pb_fb_make(3, 2);
            pb_fb_clear(&tile, pb_cell_make(' ', cols[i], cols[i], 0));
            pb_fb_put(&tile, 1, 0, pb_cell_make(0x25CFu, pb_rgb(20,20,30), cols[i], 0));
            pb_fb_put(&tile, 1, 1, pb_cell_make('|', pb_rgb(20,20,30), cols[i], 0));
            pb_sheet_set_tile(&s.sheet, i, &tile);
            pb_fb_free(&tile);
            s.anim_frames[i] = i;
        }
        s.anim.frames = s.anim_frames;
        s.anim.frame_count = 4;
        s.anim.fps = 6.0f;
        s.anim.loop = 1;
        pb_anim_reset(&s.anim);
    }

    pb_app_desc d;
    memset(&d, 0, sizeof(d));
    d.title = "PlayboxLib Demo";
    d.target_fps = 60;
    d.flags = PB_APP_FLAG_CUSTOM_CLEAR;
    d.clear = pb_cell_make(' ', pb_rgb(220,224,230), pb_rgb(10,12,18), 0);
    d.on_event = on_event;
    d.on_update = on_update;
    d.on_draw = on_draw;

    pb_app* app = pb_app_create(&d, &s);
    if(!app){ pb_sheet_free(&s.sheet); return 1; }
    int ok = pb_app_run(app);
    pb_app_destroy(app);
    pb_sheet_free(&s.sheet);
    return ok ? 0 : 1;
}
