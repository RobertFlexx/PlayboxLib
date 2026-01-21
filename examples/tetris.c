#include "playbox/pb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define BW 10
#define BH 20

static const uint16_t TETRO[7][4] = {
    { 0x0F00, 0x2222, 0x00F0, 0x4444 },
    { 0x8E00, 0x6440, 0x0E20, 0x44C0 },
    { 0x2E00, 0x4460, 0x0E80, 0xC440 },
    { 0x6600, 0x6600, 0x6600, 0x6600 },
    { 0x6C00, 0x4620, 0x06C0, 0x8C40 },
    { 0x4E00, 0x4640, 0x0E40, 0x4C40 },
    { 0xC600, 0x2640, 0x0C60, 0x4C80 }
};

static const pb_color PIECE_COL[8] = {
    {0,0,0},
    {80,230,250},
    {100,120,250},
    {250,150,90},
    {240,220,90},
    {120,240,120},
    {220,120,240},
    {250,90,110}
};

typedef struct {
    int board[BH][BW];
    int cur, rot, px, py;
    int next[5];
    int bag[7];
    int bag_i;

    int hold;
    int hold_used;

    int game_over;
    int lines;
    int score;
    int level;

    double fall_timer;
    double fall_delay;

    double lock_timer;
    double lock_delay;

    int move_l;
    int move_r;
    int soft;

    double das_timer;
    double arr_timer;

    int want_rot_cw;
    int want_rot_ccw;
    int want_hard;
} ts;

static int cell_at(uint16_t mask, int x, int y){
    int bit = y*4 + x;
    return (mask >> (15 - bit)) & 1;
}

static void bag_refill(ts* s){
    for(int i=0;i<7;i++) s->bag[i] = i;
    for(int i=6;i>0;i--){
        int j = rand() % (i+1);
        int tmp = s->bag[i]; s->bag[i]=s->bag[j]; s->bag[j]=tmp;
    }
    s->bag_i = 0;
}

static int pop_piece(ts* s){
    if(s->bag_i >= 7) bag_refill(s);
    return s->bag[s->bag_i++];
}

static void queue_init(ts* s){
    bag_refill(s);
    for(int i=0;i<5;i++) s->next[i] = pop_piece(s);
}

static void queue_push(ts* s){
    for(int i=0;i<4;i++) s->next[i] = s->next[i+1];
    s->next[4] = pop_piece(s);
}

static int collides(const ts* s, int type, int rot, int px, int py){
    uint16_t m = TETRO[type][rot & 3];
    for(int y=0;y<4;y++){
        for(int x=0;x<4;x++){
            if(!cell_at(m,x,y)) continue;
            int bx = px + x;
            int by = py + y;
            if(bx < 0 || bx >= BW) return 1;
            if(by >= BH) return 1;
            if(by >= 0 && s->board[by][bx]) return 1;
        }
    }
    return 0;
}

static void merge_piece(ts* s){
    uint16_t m = TETRO[s->cur][s->rot & 3];
    for(int y=0;y<4;y++){
        for(int x=0;x<4;x++){
            if(!cell_at(m,x,y)) continue;
            int bx = s->px + x;
            int by = s->py + y;
            if(by >= 0 && by < BH && bx >= 0 && bx < BW){
                s->board[by][bx] = s->cur + 1;
            }
        }
    }
}

static int clear_lines(ts* s){
    int cleared = 0;
    for(int y=0;y<BH;y++){
        int full = 1;
        for(int x=0;x<BW;x++){
            if(!s->board[y][x]){ full=0; break; }
        }
        if(full){
            cleared++;
            for(int yy=y; yy>0; yy--){
                memcpy(s->board[yy], s->board[yy-1], sizeof(s->board[yy]));
            }
            memset(s->board[0], 0, sizeof(s->board[0]));
        }
    }
    return cleared;
}

static void recalc_speed(ts* s){
    s->level = 1 + s->lines / 10;
    double d = 0.72 - 0.045 * (s->level - 1);
    if(d < 0.06) d = 0.06;
    s->fall_delay = d;
    s->lock_delay = 0.50;
}

static void spawn(ts* s){
    s->cur = s->next[0];
    queue_push(s);
    s->rot = 0;
    s->px = 3;
    s->py = -1;
    s->hold_used = 0;
    s->lock_timer = 0.0;

    if(collides(s, s->cur, s->rot, s->px, s->py)){
        s->game_over = 1;
    }
}

static void reset(ts* s){
    memset(s, 0, sizeof(*s));
    s->hold = -1;
    queue_init(s);
    recalc_speed(s);
    spawn(s);
}

static void score_lines(ts* s, int cleared){
    if(cleared <= 0) return;
    int base = 0;
    if(cleared == 1) base = 100;
    else if(cleared == 2) base = 300;
    else if(cleared == 3) base = 500;
    else base = 800;
    s->score += base * s->level;
    s->lines += cleared;
    recalc_speed(s);
}

static void try_rotate(ts* s, int dir){
    int nr = (s->rot + dir) & 3;
    int kicks[7][2] = { {0,0},{-1,0},{1,0},{-2,0},{2,0},{0,-1},{0,1} };
    for(int i=0;i<7;i++){
        int nx = s->px + kicks[i][0];
        int ny = s->py + kicks[i][1];
        if(!collides(s, s->cur, nr, nx, ny)){
            s->rot = nr;
            s->px = nx;
            s->py = ny;
            return;
        }
    }
}

static int drop_distance(const ts* s, int type, int rot, int px, int py){
    int d = 0;
    while(!collides(s, type, rot, px, py + 1)){
        py++;
        d++;
    }
    return d;
}

static void lock_and_next(ts* s){
    merge_piece(s);
    int c = clear_lines(s);
    score_lines(s, c);
    spawn(s);
}

static void do_hard_drop(ts* s){
    int d = drop_distance(s, s->cur, s->rot, s->px, s->py);
    s->py += d;
    s->score += 2 * d;
    lock_and_next(s);
}

static void do_hold(ts* s){
    if(s->hold_used || s->game_over) return;
    int cur = s->cur;
    if(s->hold < 0){
        s->hold = cur;
        spawn(s);
    }else{
        int tmp = s->hold;
        s->hold = cur;
        s->cur = tmp;
        s->rot = 0;
        s->px = 3;
        s->py = -1;
        s->lock_timer = 0.0;
        if(collides(s, s->cur, s->rot, s->px, s->py)) s->game_over = 1;
    }
    s->hold_used = 1;
}

static void set_move(ts* s, int dir, int down){
    if(dir < 0) s->move_l = down ? 1 : 0;
    if(dir > 0) s->move_r = down ? 1 : 0;
}

static int try_move(ts* s, int dx, int dy){
    if(!collides(s, s->cur, s->rot, s->px + dx, s->py + dy)){
        s->px += dx;
        s->py += dy;
        return 1;
    }
    return 0;
}

static void on_event(pb_app* app, void* user, const pb_event* ev){
    ts* s = (ts*)user;

    if(ev->type == PB_EVENT_KEY && ev->as.key.pressed){
        pb_key k = ev->as.key.key;
        uint32_t cp = ev->as.key.codepoint;

        if(k == PB_KEY_ESC || cp=='q' || cp=='Q') pb_app_quit(app);
        if(cp=='r' || cp=='R') reset(s);

        if(s->game_over) return;

        if(k == PB_KEY_LEFT || cp=='a' || cp=='A') set_move(s, -1, 1);
        if(k == PB_KEY_RIGHT || cp=='d' || cp=='D') set_move(s, +1, 1);

        if(k == PB_KEY_DOWN || cp=='s' || cp=='S') s->soft = 1;

        if(k == PB_KEY_UP || cp=='w' || cp=='W') s->want_rot_cw = 1;
        if(cp=='z' || cp=='Z') s->want_rot_ccw = 1;

        if(cp==' ') s->want_hard = 1;

        if(cp=='c' || cp=='C') do_hold(s);
        if(k == PB_KEY_TAB) do_hold(s);
    }

    if(ev->type == PB_EVENT_KEY && !ev->as.key.pressed){
        pb_key k = ev->as.key.key;
        uint32_t cp = ev->as.key.codepoint;
        if(k == PB_KEY_LEFT || cp=='a' || cp=='A') set_move(s, -1, 0);
        if(k == PB_KEY_RIGHT || cp=='d' || cp=='D') set_move(s, +1, 0);
        if(k == PB_KEY_DOWN || cp=='s' || cp=='S') s->soft = 0;
    }
}

static void apply_inputs(ts* s, double dt){
    const double DAS = 0.135;
    const double ARR = 0.028;

    if(s->want_rot_cw){ try_rotate(s, +1); s->want_rot_cw = 0; }
    if(s->want_rot_ccw){ try_rotate(s, -1); s->want_rot_ccw = 0; }

    if(s->want_hard){ s->want_hard = 0; do_hard_drop(s); return; }

    int dir = 0;
    if(s->move_l && !s->move_r) dir = -1;
    if(s->move_r && !s->move_l) dir = +1;

    if(dir == 0){
        s->das_timer = 0.0;
        s->arr_timer = 0.0;
    }else{
        if(s->das_timer == 0.0 && s->arr_timer == 0.0){
            try_move(s, dir, 0);
        }
        s->das_timer += dt;
        if(s->das_timer >= DAS){
            s->arr_timer += dt;
            while(s->arr_timer >= ARR){
                s->arr_timer -= ARR;
                if(!try_move(s, dir, 0)) break;
            }
        }
    }
}

static void step_fall(ts* s, double dt){
    double fall = s->soft ? (s->fall_delay * 0.08) : s->fall_delay;
    if(fall < 0.01) fall = 0.01;

    s->fall_timer += dt;
    while(s->fall_timer >= fall){
        s->fall_timer -= fall;

        if(!collides(s, s->cur, s->rot, s->px, s->py + 1)){
            s->py++;
            if(s->soft) s->score += 1;
            s->lock_timer = 0.0;
        }else{
            s->lock_timer += fall;
            if(s->lock_timer >= s->lock_delay){
                lock_and_next(s);
                return;
            }
            break;
        }
    }

    if(collides(s, s->cur, s->rot, s->px, s->py + 1)){
        s->lock_timer += dt;
        if(s->lock_timer >= s->lock_delay){
            lock_and_next(s);
            return;
        }
    }else{
        s->lock_timer = 0.0;
    }
}

static void on_update(pb_app* app, void* user, double dt){
    (void)app;
    ts* s = (ts*)user;
    if(s->game_over) return;
    if(dt > 0.05) dt = 0.05;
    apply_inputs(s, dt);
    if(s->game_over) return;
    step_fall(s, dt);
}

static void draw_block(pb_fb* fb, int x, int y, pb_color col){
    pb_cell c1 = pb_cell_make(' ', pb_rgb(0,0,0), col, 0);
    pb_cell c2 = pb_cell_make(' ', pb_rgb(0,0,0), col, 0);
    pb_fb_put(fb, x, y, c1);
    pb_fb_put(fb, x+1, y, c2);
}

static void draw_piece_preview(pb_fb* fb, int ox, int oy, int type, pb_color bg){
    if(type < 0) {
        pb_fb_text(fb, ox, oy+1, "-", pb_rgb(140,150,170), bg, 0);
        return;
    }
    uint16_t m = TETRO[type][0];
    pb_color col = PIECE_COL[type+1];
    for(int y=0;y<4;y++){
        for(int x=0;x<4;x++){
            if(cell_at(m,x,y)){
                draw_block(fb, ox + x*2, oy + y, col);
            }else{
                pb_fb_put(fb, ox + x*2, oy + y, pb_cell_make(' ', pb_rgb(200,200,200), bg, 0));
                pb_fb_put(fb, ox + x*2 + 1, oy + y, pb_cell_make(' ', pb_rgb(200,200,200), bg, 0));
            }
        }
    }
}

static void draw_ghost(pb_fb* fb, const ts* s, int ox, int oy, pb_color bg){
    int d = drop_distance(s, s->cur, s->rot, s->px, s->py);
    int gy = s->py + d;
    uint16_t m = TETRO[s->cur][s->rot & 3];
    pb_color col = PIECE_COL[s->cur+1];
    pb_color gcol = pb_rgb((uint8_t)((col.r + bg.r)/2), (uint8_t)((col.g + bg.g)/2), (uint8_t)((col.b + bg.b)/2));
    for(int y=0;y<4;y++){
        for(int x=0;x<4;x++){
            if(!cell_at(m,x,y)) continue;
            int bx = s->px + x;
            int by = gy + y;
            if(by < 0 || by >= BH) continue;
            int px = ox + 2 + bx*2;
            int py = oy + 2 + by;
            draw_block(fb, px, py, gcol);
        }
    }
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    ts* s = (ts*)user;
    (void)app;

    pb_color bg = pb_rgb(10, 12, 16);
    pb_color panel = pb_rgb(16, 20, 26);
    pb_color fg = pb_rgb(220, 220, 220);
    pb_color border = pb_rgb(90, 110, 140);
    pb_color dim = pb_rgb(140, 150, 170);

    pb_fb_fill_rect(fb, 0, 0, fb->w, fb->h, pb_cell_make(' ', fg, bg, 0));

    int cellw = 2;
    int board_px = BW * cellw;
    int board_py = BH;

    int total_w = board_px + 26;
    int total_h = board_py + 2;

    int ox = (fb->w - total_w) / 2;
    int oy = (fb->h - total_h) / 2;
    if(ox < 0) ox = 0;
    if(oy < 0) oy = 0;

    pb_fb_fill_rect(fb, ox, oy, total_w, total_h, pb_cell_make(' ', fg, panel, 0));
    pb_fb_box(fb, ox, oy, total_w, total_h, border, panel, 0);

    pb_fb_box(fb, ox+1, oy+1, board_px+2, board_py+2, border, bg, 0);

    for(int y=0;y<BH;y++){
        for(int x=0;x<BW;x++){
            int v = s->board[y][x];
            pb_color col = v ? PIECE_COL[v] : pb_rgb(18, 22, 28);
            int px = ox + 2 + x*cellw;
            int py = oy + 2 + y;
            draw_block(fb, px, py, col);
        }
    }

    if(!s->game_over){
        draw_ghost(fb, s, ox, oy, bg);

        uint16_t m = TETRO[s->cur][s->rot & 3];
        pb_color col = PIECE_COL[s->cur+1];
        for(int y=0;y<4;y++){
            for(int x=0;x<4;x++){
                if(!cell_at(m,x,y)) continue;
                int bx = s->px + x;
                int by = s->py + y;
                if(by < 0) continue;
                int px = ox + 2 + bx*cellw;
                int py = oy + 2 + by;
                draw_block(fb, px, py, col);
            }
        }
    }

    int px = ox + board_px + 5;
    int py = oy + 2;

    char line[128];
    pb_fb_text(fb, px, py, "PlayboxLib Tetris+", pb_rgb(110,200,255), panel, PB_STYLE_BOLD);
    py += 2;

    snprintf(line, sizeof(line), "Score: %d", s->score);
    pb_fb_text(fb, px, py++, line, fg, panel, 0);
    snprintf(line, sizeof(line), "Lines: %d", s->lines);
    pb_fb_text(fb, px, py++, line, fg, panel, 0);
    snprintf(line, sizeof(line), "Level: %d", s->level);
    pb_fb_text(fb, px, py++, line, fg, panel, 0);

    py += 1;
    pb_fb_text(fb, px, py++, "Next:", pb_rgb(250,220,110), panel, PB_STYLE_BOLD);
    draw_piece_preview(fb, px, py, s->next[0], panel);

    py += 6;
    pb_fb_text(fb, px, py++, "Hold:", pb_rgb(250,220,110), panel, PB_STYLE_BOLD);
    draw_piece_preview(fb, px, py, s->hold, panel);

    py += 6;
    pb_fb_text(fb, px, py++, "Controls:", pb_rgb(250,220,110), panel, PB_STYLE_BOLD);
    pb_fb_text(fb, px, py++, "A/D or \xE2\x86\x90\xE2\x86\x92 move", dim, panel, 0);
    pb_fb_text(fb, px, py++, "W/\xE2\x86\x91 CW  Z CCW", dim, panel, 0);
    pb_fb_text(fb, px, py++, "S/\xE2\x86\x93 soft  Space hard", dim, panel, 0);
    pb_fb_text(fb, px, py++, "C or Tab hold", dim, panel, 0);
    pb_fb_text(fb, px, py++, "R reset   Q quit", dim, panel, 0);

    if(s->game_over){
        int cw = 30;
        int ch = 6;
        int cx = ox + (board_px + 2 - cw) / 2 + 1;
        int cy = oy + (board_py + 2 - ch) / 2 + 1;
        pb_fb_fill_rect(fb, cx, cy, cw, ch, pb_cell_make(' ', fg, pb_rgb(0,0,0), 0));
        pb_fb_box(fb, cx, cy, cw, ch, pb_rgb(255,90,90), pb_rgb(0,0,0), 0);
        pb_fb_text(fb, cx+9, cy+1, "GAME OVER", pb_rgb(255,90,90), pb_rgb(0,0,0), PB_STYLE_BOLD);
        pb_fb_text(fb, cx+4, cy+3, "Press R to restart", fg, pb_rgb(0,0,0), 0);
        pb_fb_text(fb, cx+4, cy+4, "Q / ESC to quit", fg, pb_rgb(0,0,0), 0);
    }
}

int main(void){
    ts s;
    reset(&s);

    pb_app_desc d;
    memset(&d, 0, sizeof(d));
    d.title = "PlayboxLib Tetris+";
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
