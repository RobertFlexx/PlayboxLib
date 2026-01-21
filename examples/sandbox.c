#include "playbox/pb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
    int gw, gh;
    uint8_t* g;
    int brush;
    int paused;
    int dirty_resize;
} sb;

static int clampi(int v, int lo, int hi){
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static void grid_free(sb* s){
    free(s->g);
    s->g = NULL;
    s->gw = 0;
    s->gh = 0;
}

static int ensure_grid(sb* s, int w, int h){
    w = clampi(w, 8, 10000);
    h = clampi(h, 8, 10000);
    if(w == s->gw && h == s->gh && s->g) return 1;

    size_t n = (size_t)w * (size_t)h;
    uint8_t* ng = (uint8_t*)calloc(n ? n : 1, 1);
    if(!ng){
        grid_free(s);
        return 0;
    }

    free(s->g);
    s->g = ng;
    s->gw = w;
    s->gh = h;
    return 1;
}

static inline uint8_t getc_cell(const sb* s, int x, int y){
    if(!s || !s->g) return 2;
    if(x < 0 || y < 0 || x >= s->gw || y >= s->gh) return 2;
    return s->g[(size_t)y*(size_t)s->gw+(size_t)x];
}

static inline void setc_cell(sb* s, int x, int y, uint8_t v){
    if(!s || !s->g) return;
    if(x < 0 || y < 0 || x >= s->gw || y >= s->gh) return;
    s->g[(size_t)y*(size_t)s->gw+(size_t)x] = v;
}

static void sprinkle(sb* s, int x, int y, int r, uint8_t v){
    if(!s || !s->g) return;
    r = clampi(r, 1, 24);
    int rr = r*r;
    for(int yy=-r; yy<=r; yy++){
        for(int xx=-r; xx<=r; xx++){
            if(xx*xx + yy*yy > rr) continue;
            if((rand() % 100) < 75) setc_cell(s, x+xx, y+yy, v);
        }
    }
}

static void clear_grid(sb* s){
    if(!s || !s->g) return;
    memset(s->g, 0, (size_t)s->gw*(size_t)s->gh);
}

static void on_event(pb_app* app, void* user, const pb_event* ev){
    sb* s = (sb*)user;

    if(ev->type == PB_EVENT_RESIZE){
        s->dirty_resize = 1;
        return;
    }

    if(ev->type == PB_EVENT_KEY && ev->as.key.pressed){
        uint32_t cp = ev->as.key.codepoint;
        pb_key k = ev->as.key.key;

        if(k == PB_KEY_ESC || cp=='q' || cp=='Q'){
            pb_app_quit(app);
            return;
        }

        if(cp=='p' || cp=='P') s->paused = !s->paused;

        if(cp=='c' || cp=='C') clear_grid(s);

        if(cp=='[') s->brush = clampi(s->brush - 1, 1, 24);
        if(cp==']') s->brush = clampi(s->brush + 1, 1, 24);
    }

    if(ev->type == PB_EVENT_MOUSE){
        int mx = ev->as.mouse.x;
        int my = ev->as.mouse.y;

        int gx = mx;
        int gy = my - 1;

        if(ev->as.mouse.wheel){
            s->brush = clampi(s->brush + ev->as.mouse.wheel, 1, 24);
        }

        if(ev->as.mouse.pressed){
            sprinkle(s, gx, gy, s->brush, 1);
        }
    }
}

static void tick(sb* s){
    if(!s || !s->g) return;

    for(int y=s->gh-2; y>=0; y--){
        int dir = (rand() & 1) ? 1 : -1;
        int x0 = (dir == 1) ? 0 : s->gw-1;
        int x1 = (dir == 1) ? s->gw : -1;

        for(int x=x0; x!=x1; x+=dir){
            uint8_t v = getc_cell(s, x, y);
            if(v != 1) continue;

            if(getc_cell(s, x, y+1) == 0){
                setc_cell(s, x, y, 0);
                setc_cell(s, x, y+1, 1);
                continue;
            }

            int dl = (getc_cell(s, x-1, y+1) == 0);
            int dr = (getc_cell(s, x+1, y+1) == 0);

            if(dl && dr){
                if(rand() & 1){
                    setc_cell(s, x, y, 0);
                    setc_cell(s, x-1, y+1, 1);
                }else{
                    setc_cell(s, x, y, 0);
                    setc_cell(s, x+1, y+1, 1);
                }
            }else if(dl){
                setc_cell(s, x, y, 0);
                setc_cell(s, x-1, y+1, 1);
            }else if(dr){
                setc_cell(s, x, y, 0);
                setc_cell(s, x+1, y+1, 1);
            }
        }
    }
}

static void on_update(pb_app* app, void* user, double dt){
    (void)dt;
    sb* s = (sb*)user;

    int w = pb_app_width(app);
    int h = pb_app_height(app);

    int grid_w = w;
    int grid_h = h - 2;
    if(grid_h < 1) grid_h = 1;

    if(s->dirty_resize){
        ensure_grid(s, grid_w, grid_h);
        s->dirty_resize = 0;
    } else {
        if((grid_w != s->gw || grid_h != s->gh) && (grid_w > 0 && grid_h > 0)){
            ensure_grid(s, grid_w, grid_h);
        }
    }

    if(!s->paused) tick(s);
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    sb* s = (sb*)user;
    (void)app;

    pb_color bg = pb_rgb(10,12,16);
    pb_color fg = pb_rgb(220,220,220);
    pb_color cyan = pb_rgb(110,200,255);

    pb_color sand1 = pb_rgb(230, 210, 120);
    pb_color sand2 = pb_rgb(210, 185, 105);

    pb_fb_fill_rect(fb, 0, 0, fb->w, fb->h, pb_cell_make(' ', fg, bg, 0));

    char top[256];
    snprintf(top, sizeof(top),
             " PlayboxLib Sandbox  |  paint: mouse  |  [ ] brush=%d  |  P %s  |  C clear  Q quit ",
             s->brush, s->paused ? "resume" : "pause"
    );
    pb_fb_text(fb, 1, 0, top, cyan, bg, PB_STYLE_BOLD);

    int oy = 1;

    if(s->g){
        int maxy = s->gh;
        if(maxy > fb->h - 2) maxy = fb->h - 2;
        int maxx = s->gw;
        if(maxx > fb->w) maxx = fb->w;

        for(int y=0; y<maxy; y++){
            for(int x=0; x<maxx; x++){
                uint8_t v = getc_cell(s, x, y);
                if(v == 0) continue;
                pb_color col = ((x ^ y) & 1) ? sand1 : sand2;
                pb_fb_put(fb, x, y+oy, pb_cell_make(' ', fg, col, 0));
            }
        }
    } else {
        pb_fb_text(fb, 2, 2, "grid alloc failed (out of memory?)", pb_rgb(255,120,120), bg, PB_STYLE_BOLD);
    }

    pb_fb_text(fb, 1, fb->h-1, "Tip: scroll wheel changes brush. Ctrl+C quits too.", pb_rgb(160,170,190), bg, 0);
}

int main(void){
    sb s;
    memset(&s, 0, sizeof(s));
    s.brush = 3;
    s.paused = 0;
    s.dirty_resize = 1;

    pb_app_desc d;
    memset(&d, 0, sizeof(d));
    d.title = "PlayboxLib Sandbox";
    d.target_fps = 60;
    d.on_event = on_event;
    d.on_update = on_update;
    d.on_draw = on_draw;

    pb_app* app = pb_app_create(&d, &s);
    if(!app) return 1;
    int ok = pb_app_run(app);
    pb_app_destroy(app);
    grid_free(&s);
    return ok ? 0 : 1;
}
