#include "playbox/pb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct { int x, y; } v2i;

typedef struct {
    v2i* body;
    int len;
    int cap;
} snake_buf;

typedef struct {
    int w, h;
    int ox, oy;

    snake_buf sn;

    int dirx, diry;
    int nextx, nexty;

    int alive;
    int paused;
    int score;
    int grow;

    v2i food;

    double step_acc;
    double step_dt;

    unsigned rng;
} st;

/* ========================= RNG ========================= */

static unsigned rng_u(st* s){
    s->rng ^= s->rng << 13;
    s->rng ^= s->rng >> 17;
    s->rng ^= s->rng << 5;
    return s->rng;
}

static int clampi(int v, int a, int b){
    return v < a ? a : (v > b ? b : v);
}

/* ========================= Snake buffer ========================= */

static void sn_reserve(snake_buf* b, int cap){
    if(cap <= b->cap) return;

    int nc = b->cap ? b->cap : 64;
    while(nc < cap) nc *= 2;

    v2i* nb = (v2i*)realloc(b->body, (size_t)nc * sizeof(v2i));
    if(!nb) return;

    b->body = nb;
    b->cap  = nc;
}

static void sn_reset(snake_buf* b){
    b->len = 0;
}

static void sn_push_front(snake_buf* b, v2i p){
    sn_reserve(b, b->len + 1);
    if(!b->body) return;

    if(b->len > 0){
        memmove(b->body + 1, b->body, (size_t)b->len * sizeof(v2i));
    }
    b->body[0] = p;
    b->len++;
}

static void sn_pop_back(snake_buf* b){
    if(b->len > 0) b->len--;
}

static int sn_contains(const snake_buf* b, int x, int y, int from_i){
    if(!b || !b->body) return 0;
    if(from_i < 0) from_i = 0;

    for(int i = from_i; i < b->len; i++){
        if(b->body[i].x == x && b->body[i].y == y) return 1;
    }
    return 0;
}

/* ========================= Field + Food ========================= */

static void place_food(st* s){
    int tries = 0;
    for(;;){
        int x = (int)(rng_u(s) % (unsigned)s->w);
        int y = (int)(rng_u(s) % (unsigned)s->h);

        if(!sn_contains(&s->sn, x, y, 0)){
            s->food.x = x;
            s->food.y = y;
            return;
        }

        if(++tries > 5000){
            s->food.x = 0;
            s->food.y = 0;
            return;
        }
    }
}

static void compute_field(st* s, int term_w, int term_h){
    const int ui_h = 2;

    int fw = term_w;
    int fh = term_h - ui_h;
    if(fh < 6)  fh = 6;
    if(fw < 12) fw = 12;

    const int maxw = 64;
    const int maxh = 28;

    s->w = clampi(fw - 2, 10, maxw);
    s->h = clampi(fh - 2,  8, maxh);

    int total_w = s->w + 2;
    int total_h = s->h + 2 + ui_h;

    s->ox = (term_w - total_w) / 2;
    s->oy = (term_h - total_h) / 2;
    if(s->ox < 0) s->ox = 0;
    if(s->oy < 0) s->oy = 0;
}

/* ========================= Game reset ========================= */

static void reset_game(st* s, int term_w, int term_h){
    compute_field(s, term_w, term_h);

    sn_reset(&s->sn);

    s->dirx =  1; s->diry = 0;
    s->nextx = 1; s->nexty = 0;

    s->alive  = 1;
    s->paused = 0;
    s->score  = 0;
    s->grow   = 0;

    s->step_acc = 0.0;
    s->step_dt  = 0.10;

    int cx = s->w / 2;
    int cy = s->h / 2;

    /* Safety: make sure cx-2 won't go negative on tiny boards */
    if(cx < 2) cx = 2;

    /*
     *      IMPORTANT FIX:
     *      Build snake so head is at (cx,cy) and body trails left.
     *      With initial direction = right, this prevents instant self-collision.
     */
    sn_push_front(&s->sn, (v2i){ cx - 2, cy });
    sn_push_front(&s->sn, (v2i){ cx - 1, cy });
    sn_push_front(&s->sn, (v2i){ cx,     cy });

    place_food(s);
}

/* ========================= Step ========================= */

static void apply_dir(st* s){
    int nx = s->nextx, ny = s->nexty;
    if(nx == -s->dirx && ny == -s->diry) return; /* no reverse */
        s->dirx = nx;
    s->diry = ny;
}

static void step_game(st* s){
    if(!s->alive || s->paused) return;
    if(s->sn.len <= 0) return;

    apply_dir(s);

    v2i head = s->sn.body[0];
    v2i nh = (v2i){ head.x + s->dirx, head.y + s->diry };

    /* Wrap edges */
    if(nh.x < 0)      nh.x = s->w - 1;
    if(nh.x >= s->w)  nh.x = 0;
    if(nh.y < 0)      nh.y = s->h - 1;
    if(nh.y >= s->h)  nh.y = 0;

    int will_grow = 0;
    if(nh.x == s->food.x && nh.y == s->food.y){
        will_grow = 1;
        s->grow  += 2;
        s->score += 10;

        place_food(s);

        double d = 0.12 - 0.002 * (double)(s->score / 10);
        if(d < 0.045) d = 0.045;
        s->step_dt = d;
    }

    /* Collision: allow moving into the tail cell if tail will move away */
    int ignore_tail = (!will_grow && s->grow <= 0) ? 1 : 0;

    int collide = sn_contains(&s->sn, nh.x, nh.y, 0);
    if(collide && ignore_tail){
        v2i tail = s->sn.body[s->sn.len - 1];
        if(tail.x == nh.x && tail.y == nh.y) collide = 0;
    }

    if(collide){
        s->alive = 0;
        return;
    }

    sn_push_front(&s->sn, nh);

    if(s->grow > 0){
        s->grow--;
    }else{
        sn_pop_back(&s->sn);
    }
}

/* ========================= Input ========================= */

static void on_event(pb_app* app, void* user, const pb_event* ev){
    st* s = (st*)user;

    if(ev->type == PB_EVENT_RESIZE){
        int tw = ev->as.resize.width;
        int th = ev->as.resize.height;

        compute_field(s, tw, th);

        /* keep existing positions in-bounds */
        if(s->food.x >= s->w) s->food.x = s->w ? (s->w - 1) : 0;
        if(s->food.y >= s->h) s->food.y = s->h ? (s->h - 1) : 0;

        for(int i = 0; i < s->sn.len; i++){
            if(s->sn.body[i].x >= s->w) s->sn.body[i].x = s->w - 1;
            if(s->sn.body[i].y >= s->h) s->sn.body[i].y = s->h - 1;
        }
        return;
    }

    if(ev->type != PB_EVENT_KEY || !ev->as.key.pressed) return;

    pb_key k = ev->as.key.key;
    uint32_t cp = ev->as.key.codepoint;

    if(k == PB_KEY_ESC || cp=='q' || cp=='Q'){
        pb_app_quit(app);
        return;
    }

    if(cp=='r' || cp=='R'){
        reset_game(s, pb_app_width(app), pb_app_height(app));
        return;
    }

    if(cp=='p' || cp=='P'){
        s->paused = !s->paused;
        return;
    }

    if(!s->alive){
        if(cp==' ' || cp=='\n' || cp=='\r'){
            reset_game(s, pb_app_width(app), pb_app_height(app));
        }
        return;
    }

    if(k == PB_KEY_UP    || cp=='w' || cp=='W'){ s->nextx = 0;  s->nexty = -1; }
    else if(k == PB_KEY_DOWN  || cp=='s' || cp=='S'){ s->nextx = 0;  s->nexty =  1; }
    else if(k == PB_KEY_LEFT  || cp=='a' || cp=='A'){ s->nextx = -1; s->nexty =  0; }
    else if(k == PB_KEY_RIGHT || cp=='d' || cp=='D'){ s->nextx =  1; s->nexty =  0; }
}

/* ========================= Update ========================= */

static void on_update(pb_app* app, void* user, double dt){
    (void)app;
    st* s = (st*)user;

    if(dt > 0.25) dt = 0.25;
    s->step_acc += dt;

    double step = s->step_dt;
    int max_steps = 5;

    while(s->step_acc >= step && max_steps-- > 0){
        s->step_acc -= step;
        step_game(s);
        if(!s->alive) break;
    }

    if(s->step_acc > step * 2.0) s->step_acc = 0.0;
}

/* ========================= Draw ========================= */

static void draw_cell(pb_fb* fb, int x, int y, pb_color fg, pb_color bg, uint16_t stl, uint32_t ch){
    pb_fb_put(fb, x, y, pb_cell_make(ch, fg, bg, stl));
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    (void)app;
    st* s = (st*)user;

    pb_color bg0    = pb_rgb(10,12,16);
    pb_color panel  = pb_rgb(16,20,26);
    pb_color border = pb_rgb(90,110,140);
    pb_color fg     = pb_rgb(220,220,220);
    pb_color neon   = pb_rgb(110,200,255);
    pb_color foodc  = pb_rgb(250,90,110);
    pb_color headc  = pb_rgb(120,240,140);
    pb_color bodyc  = pb_rgb(80,190,120);

    pb_fb_fill_rect(fb, 0, 0, fb->w, fb->h, pb_cell_make(' ', fg, bg0, 0));

    const int ui_h = 2;
    int bx = s->ox;
    int by = s->oy + ui_h;

    int total_w = s->w + 2;
    int total_h = s->h + 2;

    pb_fb_fill_rect(fb, bx, s->oy, total_w, total_h + ui_h, pb_cell_make(' ', fg, panel, 0));
    pb_fb_box(fb, bx, s->oy, total_w, total_h + ui_h, border, panel, 0);

    pb_fb_fill_rect(fb, bx + 1, by + 1, s->w, s->h, pb_cell_make(' ', fg, bg0, 0));
    pb_fb_box(fb, bx, by, total_w, total_h, border, bg0, 0);

    char top[256];
    snprintf(top, sizeof(top),
             " PlayboxLib Snake | Score %d | %s%s | WASD/Arrows  P pause  R reset  Q quit ",
             s->score,
             s->alive ? "" : "DEAD",
             s->paused ? " (paused)" : ""
    );

    pb_fb_text(fb, bx + 1, s->oy + 0, top, neon, panel, PB_STYLE_BOLD);
    pb_fb_text(fb, bx + 1, s->oy + 1,
               "Eat red, don’t eat yourself. Wrap edges. Space/Enter restarts after death.",
               fg, panel, 0
    );

    /* food */
    int fx = bx + 1 + s->food.x;
    int fy = by + 1 + s->food.y;
    draw_cell(fb, fx, fy, pb_rgb(0,0,0), foodc, PB_STYLE_BOLD, ' ');

    /* snake */
    for(int i = s->sn.len - 1; i >= 0; i--){
        v2i p = s->sn.body[i];
        int x = bx + 1 + p.x;
        int y = by + 1 + p.y;
        pb_color c = (i == 0) ? headc : bodyc;
        draw_cell(fb, x, y, pb_rgb(0,0,0), c, (i == 0) ? PB_STYLE_BOLD : 0, ' ');
    }

    if(!s->alive){
        int cw = 30, ch = 5;
        int cx = bx + (total_w - cw) / 2;
        int cy = by + (total_h - ch) / 2;
        pb_fb_fill_rect(fb, cx, cy, cw, ch, pb_cell_make(' ', fg, pb_rgb(0,0,0), 0));
        pb_fb_box(fb, cx, cy, cw, ch, foodc, pb_rgb(0,0,0), 0);
        pb_fb_text(fb, cx + 10, cy + 1, "GAME OVER", foodc, pb_rgb(0,0,0), PB_STYLE_BOLD);
        pb_fb_text(fb, cx + 4,  cy + 3, "Space/Enter to restart", fg, pb_rgb(0,0,0), 0);
    }
}

/* ========================= main ========================= */

int main(void){
    st s;
    memset(&s, 0, sizeof(s));

    s.rng = 0xA341316Cu ^ (unsigned)pb_time_ns();
    reset_game(&s, 80, 24);

    pb_app_desc d;
    memset(&d, 0, sizeof(d));

    d.title      = "PlayboxLib Snake";
    d.target_fps = 120;
    d.on_event   = on_event;
    d.on_update  = on_update;
    d.on_draw    = on_draw;

    pb_app* app = pb_app_create(&d, &s);
    if(!app) return 1;

    int ok = pb_app_run(app);

    pb_app_destroy(app);
    free(s.sn.body);
    return ok ? 0 : 1;
}
