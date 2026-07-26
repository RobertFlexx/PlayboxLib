
#ifndef PLAYBOX_PB_FB_H
#define PLAYBOX_PB_FB_H

#include "pb_export.h"
#include "pb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int w;
    int h;
    pb_cell* cells;
    /* Camera: world (x,y) maps to screen (x - cam_x, y - cam_y). */
    int cam_x;
    int cam_y;
    /* Scissor in screen-space cells [x0,x1) x [y0,y1). */
    int clip_x0;
    int clip_y0;
    int clip_x1;
    int clip_y1;
} pb_fb;

PB_API pb_fb pb_fb_make(int w, int h);
PB_API void pb_fb_free(pb_fb* fb);

PB_API void pb_fb_clear(pb_fb* fb, pb_cell fill);

static inline pb_cell pb_cell_make(uint32_t ch, pb_color fg, pb_color bg, uint16_t style){
    pb_cell c; c.ch=ch; c.fg=fg; c.bg=bg; c.style=style; return c;
}

PB_API void pb_fb_put(pb_fb* fb, int x, int y, pb_cell c);
PB_API pb_cell pb_fb_get(const pb_fb* fb, int x, int y);
/* Screen-space put/get (ignore camera). Still respect clip. */
PB_API void pb_fb_put_screen(pb_fb* fb, int x, int y, pb_cell c);
PB_API pb_cell pb_fb_get_screen(const pb_fb* fb, int x, int y);

PB_API void pb_fb_text(pb_fb* fb, int x, int y, const char* utf8, pb_color fg, pb_color bg, uint16_t style);
PB_API void pb_fb_textf(pb_fb* fb, int x, int y, pb_color fg, pb_color bg, uint16_t style, const char* fmt, ...);
PB_API void pb_fb_text_centered(pb_fb* fb, int y, const char* utf8, pb_color fg, pb_color bg, uint16_t style);

PB_API void pb_fb_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_cell c);
PB_API void pb_fb_box(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, uint16_t style);
PB_API void pb_fb_box_double(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, uint16_t style);
PB_API void pb_fb_panel(pb_fb* fb, int x, int y, int w, int h, const char* title,
                 pb_color border, pb_color title_fg, pb_color fill, uint16_t style);

PB_API void pb_fb_hline(pb_fb* fb, int x, int y, int w, pb_cell c);
PB_API void pb_fb_vline(pb_fb* fb, int x, int y, int h, pb_cell c);
PB_API void pb_fb_line(pb_fb* fb, int x0, int y0, int x1, int y1, pb_cell c);

PB_API void pb_fb_circle(pb_fb* fb, int cx, int cy, int radius, pb_cell c);
PB_API void pb_fb_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_cell c);

PB_API void pb_fb_blit(pb_fb* dst, int dx, int dy, const pb_fb* src);
PB_API void pb_fb_blit_region(pb_fb* dst, int dx, int dy, const pb_fb* src, int sx, int sy, int w, int h);
PB_API void pb_fb_blit_masked(pb_fb* dst, int dx, int dy, const pb_fb* src, uint32_t transparent_ch);

/* Half-block "pixel" mode: 1 cell = 2 vertical pixels (U+2580 ▀).
   Pixel space is (fb->w) x (fb->h * 2). Uses camera. */
PB_API void pb_fb_plot(pb_fb* fb, int px, int py, pb_color color);
PB_API void pb_fb_plot_line(pb_fb* fb, int x0, int y0, int x1, int y1, pb_color color);
PB_API void pb_fb_plot_rect(pb_fb* fb, int x, int y, int w, int h, pb_color color);
PB_API void pb_fb_plot_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_color color);
PB_API void pb_fb_plot_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color);
PB_API void pb_fb_plot_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color);

PB_API int pb_fb_measure_text(const char* utf8);
PB_API int pb_char_width(uint32_t codepoint);

#ifdef __cplusplus
}
#endif

#endif
