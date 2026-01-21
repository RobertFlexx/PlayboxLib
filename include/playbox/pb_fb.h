
#ifndef PLAYBOX_PB_FB_H
#define PLAYBOX_PB_FB_H

#include "pb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int w;
    int h;
    pb_cell* cells;
} pb_fb;

pb_fb pb_fb_make(int w, int h);
void pb_fb_free(pb_fb* fb);

void pb_fb_clear(pb_fb* fb, pb_cell fill);

static inline pb_cell pb_cell_make(uint32_t ch, pb_color fg, pb_color bg, uint16_t style){
    pb_cell c; c.ch=ch; c.fg=fg; c.bg=bg; c.style=style; return c;
}

void pb_fb_put(pb_fb* fb, int x, int y, pb_cell c);
pb_cell pb_fb_get(const pb_fb* fb, int x, int y);

void pb_fb_text(pb_fb* fb, int x, int y, const char* utf8, pb_color fg, pb_color bg, uint16_t style);
void pb_fb_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_cell c);
void pb_fb_box(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, uint16_t style);

#ifdef __cplusplus
}
#endif

#endif
