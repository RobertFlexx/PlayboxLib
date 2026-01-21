
#include <stdlib.h>
#include <string.h>
#include "playbox/pb_fb.h"
#include "pb_utf8.h"

static inline int pb_in_bounds(const pb_fb* fb, int x, int y){
    return fb && x >= 0 && y >= 0 && x < fb->w && y < fb->h;
}

pb_fb pb_fb_make(int w, int h){
    pb_fb fb;
    fb.w = (w < 0) ? 0 : w;
    fb.h = (h < 0) ? 0 : h;
    size_t n = (size_t)fb.w * (size_t)fb.h;
    fb.cells = (pb_cell*)calloc(n ? n : 1, sizeof(pb_cell));
    return fb;
}

void pb_fb_free(pb_fb* fb){
    if(!fb) return;
    free(fb->cells);
    fb->cells = NULL;
    fb->w = 0;
    fb->h = 0;
}

void pb_fb_clear(pb_fb* fb, pb_cell fill){
    if(!fb || !fb->cells) return;
    size_t n = (size_t)fb->w * (size_t)fb->h;
    for(size_t i=0;i<n;i++) fb->cells[i] = fill;
}

void pb_fb_put(pb_fb* fb, int x, int y, pb_cell c){
    if(!pb_in_bounds(fb,x,y)) return;
    fb->cells[(size_t)y * (size_t)fb->w + (size_t)x] = c;
}

pb_cell pb_fb_get(const pb_fb* fb, int x, int y){
    pb_cell z;
    z.ch = ' ';
    z.fg = (pb_color){255,255,255};
    z.bg = (pb_color){0,0,0};
    z.style = 0;
    if(!pb_in_bounds(fb,x,y)) return z;
    return fb->cells[(size_t)y * (size_t)fb->w + (size_t)x];
}

void pb_fb_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_cell c){
    if(!fb || !fb->cells || w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w;
    int y1 = y + h;
    if(x1 > fb->w) x1 = fb->w;
    if(y1 > fb->h) y1 = fb->h;
    for(int yy=y0; yy<y1; yy++){
        for(int xx=x0; xx<x1; xx++){
            pb_fb_put(fb, xx, yy, c);
        }
    }
}

void pb_fb_box(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, uint16_t style){
    if(!fb || !fb->cells || w < 2 || h < 2) return;

    pb_cell hc = pb_cell_make(0x2500u, fg, bg, style);
    pb_cell vc = pb_cell_make(0x2502u, fg, bg, style);
    pb_cell tl = pb_cell_make(0x250Cu, fg, bg, style);
    pb_cell tr = pb_cell_make(0x2510u, fg, bg, style);
    pb_cell bl = pb_cell_make(0x2514u, fg, bg, style);
    pb_cell br = pb_cell_make(0x2518u, fg, bg, style);

    pb_fb_put(fb, x, y, tl);
    pb_fb_put(fb, x+w-1, y, tr);
    pb_fb_put(fb, x, y+h-1, bl);
    pb_fb_put(fb, x+w-1, y+h-1, br);

    for(int xx=x+1; xx<x+w-1; xx++){
        pb_fb_put(fb, xx, y, hc);
        pb_fb_put(fb, xx, y+h-1, hc);
    }
    for(int yy=y+1; yy<y+h-1; yy++){
        pb_fb_put(fb, x, yy, vc);
        pb_fb_put(fb, x+w-1, yy, vc);
    }
}

void pb_fb_text(pb_fb* fb, int x, int y, const char* utf8, pb_color fg, pb_color bg, uint16_t style){
    if(!fb || !fb->cells || !utf8) return;
    int cx = x;
    const uint8_t* s = (const uint8_t*)utf8;
    size_t n = strlen(utf8);
    size_t i = 0;
    while(i < n){
        uint32_t cp = 0;
        size_t adv = 0;
        if(!pb_utf8_decode(s+i, n-i, &cp, &adv)){
            cp = (uint32_t)'?';
            adv = 1;
        }
        if(cp == '\n'){
            cx = x;
            y++;
            i += adv;
            continue;
        }
        if(cp == '\t'){
            int spaces = 4 - (cx % 4);
            for(int k=0;k<spaces;k++){
                pb_fb_put(fb, cx, y, pb_cell_make(' ', fg, bg, style));
                cx++;
            }
            i += adv;
            continue;
        }
        pb_fb_put(fb, cx, y, pb_cell_make(cp, fg, bg, style));
        cx++;
        i += adv;
    }
}
