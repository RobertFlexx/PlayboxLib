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
    (void)app;
    demo_state* s = (demo_state*)user;

    int w = fb->w;
    int h = fb->h;
    if(w < 1) w = 1;
    if(h < 1) h = 1;

    pb_color bg = pb_rgb(12, 14, 18);
    pb_color fg = pb_rgb(220, 220, 220);
    pb_color neon = pb_rgb(110, 200, 255);
    pb_color mag = pb_rgb(250, 120, 220);
    pb_color panel = pb_rgb(20, 24, 30);
    pb_color border = pb_rgb(80, 90, 100);

    pb_fb_fill_rect(fb, 0, 0, fb->w, fb->h, pb_cell_make(' ', fg, bg, 0));

    if(w >= 2 && h >= 2){
        pb_fb_box(fb, 0, 0, w, h, border, bg, 0);
    }

    char top[256];
    snprintf(top, sizeof(top), " PlayboxLib Demo  |  %dx%d  |  ESC/Q quit  |  H help ", fb->w, fb->h);
    if(w >= 4){
        pb_fb_text(fb, 2, 0, top, neon, bg, PB_STYLE_BOLD);
    }

    int bx = (int)(s->x + 0.5);
    int by = (int)(s->y + 0.5);
    bx = clampi(bx, 0, fb->w - 1);
    by = clampi(by, 0, fb->h - 1);

    pb_fb_put(fb, bx, by, pb_cell_make('@', mag, bg, PB_STYLE_BOLD));

    if(s->show_help && w >= 10 && h >= 6){
        int hw = (w < 60) ? (w - 2) : 58;
        int hh = (h < 12) ? (h - 2) : 10;
        hw = clampi(hw, 8, w - 2);
        hh = clampi(hh, 4, h - 2);

        int hx = (w - hw) / 2;
        int hy = (h - hh) / 2;

        pb_fb_fill_rect(fb, hx, hy, hw, hh, pb_cell_make(' ', fg, panel, 0));
        pb_fb_box(fb, hx, hy, hw, hh, neon, panel, 0);

        int tx = hx + 2;
        int ty = hy + 1;

        if(hw > 6 && hh > 3){
            pb_fb_text(fb, tx, ty++, "Standalone ANSI renderer (no ncurses).", fg, panel, 0);
            pb_fb_text(fb, tx, ty++, "Framebuffer cells + events + diff rendering.", fg, panel, 0);
            ty++;

            pb_fb_text(fb, tx, ty++, "Use this as the base for TUI games:", fg, panel, PB_STYLE_BOLD);
            pb_fb_text(fb, tx+2, ty++, "- netris-style block games", fg, panel, 0);
            pb_fb_text(fb, tx+2, ty++, "- powder / cellular sandboxes", fg, panel, 0);
            pb_fb_text(fb, tx+2, ty++, "- roguelikes, shooters, shmups", fg, panel, 0);

            if(ty < hy + hh - 1){
                pb_fb_text(fb, tx, hy + hh - 2, "Press H to close.", neon, panel, 0);
            }
        }
    }
}

int main(void){
    demo_state s;
    memset(&s, 0, sizeof(s));

    s.x = 10.0;
    s.y = 5.0;
    s.vx = 25.0;
    s.vy = 12.0;
    s.show_help = 1;

    pb_app_desc d;
    memset(&d, 0, sizeof(d));
    d.title = "PlayboxLib Demo";
    d.target_fps = 60;
    d.on_event = on_event;
    d.on_update = on_update;
    d.on_draw = on_draw;

    pb_app* app = pb_app_create(&d, &s);
    if(!app) return 1;
    int ok = pb_app_run(app);
    pb_app_destroy(app);
    return ok ? 0 : 1;
}
