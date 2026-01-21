#include "playbox/pb_renderer.h"
#include "playbox/pb_term.h"
#include "pb_utf8.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct pb_renderer {
    pb_term* term;
    pb_fb back;
    pb_cell clear;
    int full;
    pb_color cur_fg;
    pb_color cur_bg;
    uint16_t cur_style;
    char* out;
    size_t olen;
    size_t ocap;
};

static int color_eq(pb_color a, pb_color b){
    return a.r==b.r && a.g==b.g && a.b==b.b;
}

static int cell_eq(pb_cell a, pb_cell b){
    return a.ch == b.ch && a.style == b.style && color_eq(a.fg,b.fg) && color_eq(a.bg,b.bg);
}

static int sb_reserve(pb_renderer* r, size_t add){
    size_t need = r->olen + add + 1;
    if(need <= r->ocap) return 1;
    size_t nc = r->ocap ? r->ocap : 8192;
    while(nc < need) nc *= 2;
    char* nb = (char*)realloc(r->out, nc);
    if(!nb) return 0;
    r->out = nb;
    r->ocap = nc;
    return 1;
}

static int sb_append(pb_renderer* r, const char* s, size_t n){
    if(n == 0) return 1;
    if(!sb_reserve(r, n)) return 0;
    memcpy(r->out + r->olen, s, n);
    r->olen += n;
    r->out[r->olen] = '\0';
    return 1;
}

static int sb_cstr(pb_renderer* r, const char* s){
    return sb_append(r, s, strlen(s));
}

static int sb_int(pb_renderer* r, int v){
    char tmp[64];
    int n = snprintf(tmp, sizeof(tmp), "%d", v);
    if(n <= 0) return 0;
    return sb_append(r, tmp, (size_t)n);
}

static int emit_reset(pb_renderer* r){
    return sb_cstr(r, "\x1b[0m");
}

static int emit_style(pb_renderer* r, uint16_t style){
    if(!emit_reset(r)) return 0;
    if(style & PB_STYLE_BOLD) if(!sb_cstr(r, "\x1b[1m")) return 0;
    if(style & PB_STYLE_DIM) if(!sb_cstr(r, "\x1b[2m")) return 0;
    if(style & PB_STYLE_UNDERLINE) if(!sb_cstr(r, "\x1b[4m")) return 0;
    if(style & PB_STYLE_REVERSE) if(!sb_cstr(r, "\x1b[7m")) return 0;
    return 1;
}

static int emit_fg(pb_renderer* r, pb_color c){
    if(!sb_cstr(r, "\x1b[38;2;")) return 0;
    if(!sb_int(r, (int)c.r)) return 0;
    if(!sb_cstr(r, ";")) return 0;
    if(!sb_int(r, (int)c.g)) return 0;
    if(!sb_cstr(r, ";")) return 0;
    if(!sb_int(r, (int)c.b)) return 0;
    if(!sb_cstr(r, "m")) return 0;
    return 1;
}

static int emit_bg(pb_renderer* r, pb_color c){
    if(!sb_cstr(r, "\x1b[48;2;")) return 0;
    if(!sb_int(r, (int)c.r)) return 0;
    if(!sb_cstr(r, ";")) return 0;
    if(!sb_int(r, (int)c.g)) return 0;
    if(!sb_cstr(r, ";")) return 0;
    if(!sb_int(r, (int)c.b)) return 0;
    if(!sb_cstr(r, "m")) return 0;
    return 1;
}

static int emit_move(pb_renderer* r, int x, int y){
    if(!sb_cstr(r, "\x1b[")) return 0;
    if(!sb_int(r, y)) return 0;
    if(!sb_cstr(r, ";")) return 0;
    if(!sb_int(r, x)) return 0;
    if(!sb_cstr(r, "H")) return 0;
    return 1;
}

pb_renderer* pb_renderer_create(void){
    pb_renderer* r = (pb_renderer*)calloc(1, sizeof(pb_renderer));
    if(!r) return NULL;
    r->term = NULL;
    r->back = pb_fb_make(0,0);
    r->clear = pb_cell_make(' ', pb_rgb(255,255,255), pb_rgb(0,0,0), 0);
    r->full = 1;
    r->cur_fg = pb_rgb(255,255,255);
    r->cur_bg = pb_rgb(0,0,0);
    r->cur_style = 0xFFFFu;
    r->out = NULL;
    r->olen = 0;
    r->ocap = 0;
    return r;
}

void pb_renderer_destroy(pb_renderer* r){
    if(!r) return;
    pb_fb_free(&r->back);
    free(r->out);
    free(r);
}

int pb_renderer_bind(pb_renderer* r, void* term_handle){
    if(!r) return 0;
    r->term = (pb_term*)term_handle;
    r->full = 1;
    r->cur_style = 0xFFFFu;
    return r->term ? 1 : 0;
}

void pb_renderer_set_clear(pb_renderer* r, pb_cell clear_cell){
    if(!r) return;
    r->clear = clear_cell;
}

void pb_renderer_force_full_redraw(pb_renderer* r){
    if(!r) return;
    r->full = 1;
}

int pb_renderer_present(pb_renderer* r, const pb_fb* fb){
    if(!r || !r->term || !fb || !fb->cells) return 0;

    if(r->back.w != fb->w || r->back.h != fb->h){
        pb_fb_free(&r->back);
        r->back = pb_fb_make(fb->w, fb->h);
        pb_fb_clear(&r->back, r->clear);
        r->full = 1;
    }

    r->olen = 0;

    if(r->full){
        if(!emit_reset(r)) return 0;
        if(!emit_fg(r, r->clear.fg)) return 0;
        if(!emit_bg(r, r->clear.bg)) return 0;
        if(!sb_cstr(r, "\x1b[2J\x1b[H")) return 0;
        r->cur_style = 0xFFFFu;
        r->cur_fg = r->clear.fg;
        r->cur_bg = r->clear.bg;
    }

    for(int y=0; y<fb->h; y++){
        int x = 0;
        while(x < fb->w){
            size_t idx = (size_t)y * (size_t)fb->w + (size_t)x;
            pb_cell c = fb->cells[idx];
            pb_cell b = r->back.cells[idx];

            if(!r->full && cell_eq(c,b)){
                x++;
                continue;
            }

            int run_start = x;
            int run_end = x + 1;
            pb_cell first = c;

            while(run_end < fb->w){
                size_t j = (size_t)y * (size_t)fb->w + (size_t)run_end;
                pb_cell cj = fb->cells[j];
                pb_cell bj = r->back.cells[j];

                if(!r->full && cell_eq(cj, bj)) break;
                if(cj.style != first.style) break;
                if(!color_eq(cj.fg, first.fg)) break;
                if(!color_eq(cj.bg, first.bg)) break;
                run_end++;
            }

            if(!emit_move(r, run_start + 1, y + 1)) return 0;

            if(r->cur_style != first.style){
                if(!emit_style(r, first.style)) return 0;
                r->cur_style = first.style;
                r->cur_fg = (pb_color){0,0,0};
                r->cur_bg = (pb_color){0,0,0};
                if(!emit_fg(r, first.fg)) return 0;
                if(!emit_bg(r, first.bg)) return 0;
                r->cur_fg = first.fg;
                r->cur_bg = first.bg;
            } else {
                if(!color_eq(r->cur_fg, first.fg)){
                    if(!emit_fg(r, first.fg)) return 0;
                    r->cur_fg = first.fg;
                }
                if(!color_eq(r->cur_bg, first.bg)){
                    if(!emit_bg(r, first.bg)) return 0;
                    r->cur_bg = first.bg;
                }
            }

            for(int xx=run_start; xx<run_end; xx++){
                size_t k = (size_t)y * (size_t)fb->w + (size_t)xx;
                pb_cell ck = fb->cells[k];
                char utf8[8];
                size_t wn = pb_utf8_encode(ck.ch ? ck.ch : (uint32_t)' ', utf8);
                if(!sb_append(r, utf8, wn)) return 0;
                r->back.cells[k] = ck;
            }

            x = run_end;
        }
    }

    int wrote = pb_term_write(r->term, r->out ? r->out : "", (int)r->olen);
    r->full = 0;
    return wrote > 0 ? 1 : 0;
}
