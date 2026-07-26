
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "playbox/pb_fb.h"
#include "pb_utf8.h"

static void pb_fb_init_view(pb_fb* fb){
    fb->cam_x = 0;
    fb->cam_y = 0;
    fb->clip_x0 = 0;
    fb->clip_y0 = 0;
    fb->clip_x1 = fb->w;
    fb->clip_y1 = fb->h;
}

static inline int pb_screen_ok(const pb_fb* fb, int sx, int sy){
    if(!fb || !fb->cells) return 0;
    if(sx < 0 || sy < 0 || sx >= fb->w || sy >= fb->h) return 0;
    if(sx < fb->clip_x0 || sy < fb->clip_y0 || sx >= fb->clip_x1 || sy >= fb->clip_y1) return 0;
    return 1;
}

static inline void pb_world_to_screen(const pb_fb* fb, int x, int y, int* sx, int* sy){
    *sx = x - fb->cam_x;
    *sy = y - fb->cam_y;
}

static inline pb_cell* pb_cell_at(pb_fb* fb, int sx, int sy){
    return &fb->cells[(size_t)sy * (size_t)fb->w + (size_t)sx];
}

pb_fb pb_fb_make(int w, int h){
    pb_fb fb;
    memset(&fb, 0, sizeof(fb));
    fb.w = (w < 0) ? 0 : w;
    fb.h = (h < 0) ? 0 : h;
    size_t n = (size_t)fb.w * (size_t)fb.h;
    fb.cells = (pb_cell*)calloc(n ? n : 1, sizeof(pb_cell));
    pb_fb_init_view(&fb);
    return fb;
}

void pb_fb_free(pb_fb* fb){
    if(!fb) return;
    free(fb->cells);
    fb->cells = NULL;
    fb->w = 0;
    fb->h = 0;
    pb_fb_init_view(fb);
}

void pb_fb_clear(pb_fb* fb, pb_cell fill){
    if(!fb || !fb->cells) return;
    size_t n = (size_t)fb->w * (size_t)fb->h;
    if(n == 0) return;
    fb->cells[0] = fill;
    if(n == 1) return;
    size_t done = 1;
    while(done < n){
        size_t chunk = done;
        if(chunk > n - done) chunk = n - done;
        memcpy(fb->cells + done, fb->cells, chunk * sizeof(pb_cell));
        done += chunk;
    }
}

void pb_fb_put_screen(pb_fb* fb, int x, int y, pb_cell c){
    if(!pb_screen_ok(fb, x, y)) return;
    *pb_cell_at(fb, x, y) = c;
}

pb_cell pb_fb_get_screen(const pb_fb* fb, int x, int y){
    pb_cell z;
    z.ch = ' ';
    z.fg = (pb_color){255,255,255};
    z.bg = (pb_color){0,0,0};
    z.style = 0;
    if(!fb || !fb->cells || x < 0 || y < 0 || x >= fb->w || y >= fb->h) return z;
    return fb->cells[(size_t)y * (size_t)fb->w + (size_t)x];
}

void pb_fb_put(pb_fb* fb, int x, int y, pb_cell c){
    if(!fb) return;
    int sx, sy;
    pb_world_to_screen(fb, x, y, &sx, &sy);
    pb_fb_put_screen(fb, sx, sy, c);
}

pb_cell pb_fb_get(const pb_fb* fb, int x, int y){
    if(!fb){
        pb_cell z = {' ', {255,255,255}, {0,0,0}, 0};
        return z;
    }
    int sx, sy;
    pb_world_to_screen(fb, x, y, &sx, &sy);
    return pb_fb_get_screen(fb, sx, sy);
}

void pb_fb_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_cell c){
    if(!fb || !fb->cells || w <= 0 || h <= 0) return;
    int sx0, sy0, sx1, sy1;
    pb_world_to_screen(fb, x, y, &sx0, &sy0);
    pb_world_to_screen(fb, x + w, y + h, &sx1, &sy1);

    if(sx0 > sx1){ int t=sx0; sx0=sx1; sx1=t; }
    if(sy0 > sy1){ int t=sy0; sy0=sy1; sy1=t; }

    if(sx0 < fb->clip_x0) sx0 = fb->clip_x0;
    if(sy0 < fb->clip_y0) sy0 = fb->clip_y0;
    if(sx1 > fb->clip_x1) sx1 = fb->clip_x1;
    if(sy1 > fb->clip_y1) sy1 = fb->clip_y1;
    if(sx0 < 0) sx0 = 0;
    if(sy0 < 0) sy0 = 0;
    if(sx1 > fb->w) sx1 = fb->w;
    if(sy1 > fb->h) sy1 = fb->h;
    if(sx0 >= sx1 || sy0 >= sy1) return;

    int row_w = sx1 - sx0;
    for(int yy = sy0; yy < sy1; yy++){
        pb_cell* row = pb_cell_at(fb, sx0, yy);
        for(int i = 0; i < row_w; i++) row[i] = c;
    }
}

void pb_fb_hline(pb_fb* fb, int x, int y, int w, pb_cell c){
    pb_fb_fill_rect(fb, x, y, w, 1, c);
}

void pb_fb_vline(pb_fb* fb, int x, int y, int h, pb_cell c){
    pb_fb_fill_rect(fb, x, y, 1, h, c);
}

void pb_fb_line(pb_fb* fb, int x0, int y0, int x1, int y1, pb_cell c){
    if(!fb || !fb->cells) return;
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = dx >= 0 ? 1 : -1;
    int sy = dy >= 0 ? 1 : -1;
    if(dx < 0) dx = -dx;
    if(dy < 0) dy = -dy;

    int err = dx - dy;
    for(;;){
        pb_fb_put(fb, x0, y0, c);
        if(x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if(e2 > -dy){ err -= dy; x0 += sx; }
        if(e2 < dx){ err += dx; y0 += sy; }
    }
}

void pb_fb_circle(pb_fb* fb, int cx, int cy, int radius, pb_cell c){
    if(!fb || !fb->cells || radius < 0) return;
    int x = radius;
    int y = 0;
    int err = 0;
    while(x >= y){
        pb_fb_put(fb, cx + x, cy + y, c);
        pb_fb_put(fb, cx + y, cy + x, c);
        pb_fb_put(fb, cx - y, cy + x, c);
        pb_fb_put(fb, cx - x, cy + y, c);
        pb_fb_put(fb, cx - x, cy - y, c);
        pb_fb_put(fb, cx - y, cy - x, c);
        pb_fb_put(fb, cx + y, cy - x, c);
        pb_fb_put(fb, cx + x, cy - y, c);
        y++;
        err += 1 + 2 * y;
        if(2 * (err - x) + 1 > 0){
            x--;
            err += 1 - 2 * x;
        }
    }
}

void pb_fb_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_cell c){
    if(!fb || !fb->cells || radius < 0) return;
    for(int y = -radius; y <= radius; y++){
        for(int x = -radius; x <= radius; x++){
            if(x * x + y * y <= radius * radius){
                pb_fb_put(fb, cx + x, cy + y, c);
            }
        }
    }
}

static void pb_fb_box_chars(pb_fb* fb, int x, int y, int w, int h,
                            uint32_t hc, uint32_t vc,
                            uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br,
                            pb_color fg, pb_color bg, uint16_t style){
    if(!fb || !fb->cells || w < 2 || h < 2) return;

    pb_cell hcell = pb_cell_make(hc, fg, bg, style);
    pb_cell vcell = pb_cell_make(vc, fg, bg, style);

    pb_fb_put(fb, x, y, pb_cell_make(tl, fg, bg, style));
    pb_fb_put(fb, x+w-1, y, pb_cell_make(tr, fg, bg, style));
    pb_fb_put(fb, x, y+h-1, pb_cell_make(bl, fg, bg, style));
    pb_fb_put(fb, x+w-1, y+h-1, pb_cell_make(br, fg, bg, style));

    for(int xx = x + 1; xx < x + w - 1; xx++){
        pb_fb_put(fb, xx, y, hcell);
        pb_fb_put(fb, xx, y + h - 1, hcell);
    }
    for(int yy = y + 1; yy < y + h - 1; yy++){
        pb_fb_put(fb, x, yy, vcell);
        pb_fb_put(fb, x + w - 1, yy, vcell);
    }
}

void pb_fb_box(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, uint16_t style){
    pb_fb_box_chars(fb, x, y, w, h,
                    0x2500u, 0x2502u, 0x250Cu, 0x2510u, 0x2514u, 0x2518u,
                    fg, bg, style);
}

void pb_fb_box_double(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, uint16_t style){
    pb_fb_box_chars(fb, x, y, w, h,
                    0x2550u, 0x2551u, 0x2554u, 0x2557u, 0x255Au, 0x255Du,
                    fg, bg, style);
}

void pb_fb_panel(pb_fb* fb, int x, int y, int w, int h, const char* title,
                 pb_color border, pb_color title_fg, pb_color fill, uint16_t style){
    if(!fb || !fb->cells || w < 2 || h < 2) return;
    pb_fb_fill_rect(fb, x, y, w, h, pb_cell_make(' ', title_fg, fill, 0));
    pb_fb_box(fb, x, y, w, h, border, fill, style);
    if(title && title[0] && w > 4){
        char buf[256];
        snprintf(buf, sizeof(buf), " %s ", title);
        pb_fb_text(fb, x + 1, y, buf, title_fg, fill, style | PB_STYLE_BOLD);
    }
}

int pb_fb_measure_text(const char* utf8){
    if(!utf8) return 0;
    int w = 0;
    const uint8_t* s = (const uint8_t*)utf8;
    size_t n = strlen(utf8);
    size_t i = 0;
    while(i < n){
        uint32_t cp = 0;
        size_t adv = 0;
        if(!pb_utf8_decode(s + i, n - i, &cp, &adv)){
            adv = 1;
            cp = '?';
        }
        if(cp == '\n') break;
        if(cp == '\t') w += 4 - (w % 4);
        else w += pb_char_width(cp);
        i += adv;
    }
    return w;
}

int pb_char_width(uint32_t cp){
    if(cp == 0) return 0;
    if((cp >= 0x0300u && cp <= 0x036Fu) ||
       (cp >= 0x1AB0u && cp <= 0x1AFFu) ||
       (cp >= 0x20D0u && cp <= 0x20FFu) ||
       (cp >= 0xFE00u && cp <= 0xFE0Fu) ||
       cp == 0x200Bu || cp == 0x200Cu || cp == 0x200Du || cp == 0xFEFFu){
        return 0;
    }
    if((cp >= 0x1100u && cp <= 0x115Fu) ||
       (cp >= 0x2E80u && cp <= 0xA4CFu) ||
       (cp >= 0xAC00u && cp <= 0xD7A3u) ||
       (cp >= 0xF900u && cp <= 0xFAFFu) ||
       (cp >= 0xFE10u && cp <= 0xFE19u) ||
       (cp >= 0xFE30u && cp <= 0xFE6Fu) ||
       (cp >= 0xFF00u && cp <= 0xFF60u) ||
       (cp >= 0xFFE0u && cp <= 0xFFE6u) ||
       (cp >= 0x1F300u && cp <= 0x1FAFFu) ||
       (cp >= 0x20000u && cp <= 0x3FFFFu)){
        return 2;
    }
    return 1;
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
            int spaces = 4 - ((cx - x) % 4);
            if(spaces <= 0) spaces = 4;
            for(int k=0;k<spaces;k++){
                pb_fb_put(fb, cx, y, pb_cell_make(' ', fg, bg, style));
                cx++;
            }
            i += adv;
            continue;
        }
        int cw = pb_char_width(cp);
        if(cw < 1){ i += adv; continue; }
        pb_fb_put(fb, cx, y, pb_cell_make(cp, fg, bg, style));
        if(cw == 2){
            pb_fb_put(fb, cx + 1, y, pb_cell_make(' ', fg, bg, style));
        }
        cx += cw;
        i += adv;
    }
}

void pb_fb_textf(pb_fb* fb, int x, int y, pb_color fg, pb_color bg, uint16_t style, const char* fmt, ...){
    if(!fb || !fmt) return;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    pb_fb_text(fb, x, y, buf, fg, bg, style);
}

void pb_fb_text_centered(pb_fb* fb, int y, const char* utf8, pb_color fg, pb_color bg, uint16_t style){
    if(!fb || !utf8) return;
    int tw = pb_fb_measure_text(utf8);
    int x = (fb->w - tw) / 2 + fb->cam_x;
    pb_fb_text(fb, x, y, utf8, fg, bg, style);
}

void pb_fb_blit_region(pb_fb* dst, int dx, int dy, const pb_fb* src, int sx, int sy, int w, int h){
    if(!dst || !src || !dst->cells || !src->cells || w <= 0 || h <= 0) return;
    for(int y = 0; y < h; y++){
        int syy = sy + y;
        if(syy < 0 || syy >= src->h) continue;
        for(int x = 0; x < w; x++){
            int sxx = sx + x;
            if(sxx < 0 || sxx >= src->w) continue;
            pb_fb_put(dst, dx + x, dy + y, pb_fb_get_screen(src, sxx, syy));
        }
    }
}

void pb_fb_blit(pb_fb* dst, int dx, int dy, const pb_fb* src){
    if(!src) return;
    pb_fb_blit_region(dst, dx, dy, src, 0, 0, src->w, src->h);
}

void pb_fb_blit_masked(pb_fb* dst, int dx, int dy, const pb_fb* src, uint32_t transparent_ch){
    if(!dst || !src || !dst->cells || !src->cells) return;
    for(int y = 0; y < src->h; y++){
        for(int x = 0; x < src->w; x++){
            pb_cell c = pb_fb_get_screen(src, x, y);
            if(c.ch == transparent_ch) continue;
            pb_fb_put(dst, dx + x, dy + y, c);
        }
    }
}

void pb_fb_plot(pb_fb* fb, int px, int py, pb_color color){
    if(!fb || !fb->cells) return;
    int wx = px;
    int wy = py >> 1;
    int sx, sy;
    pb_world_to_screen(fb, wx, wy, &sx, &sy);
    if(!pb_screen_ok(fb, sx, sy)) return;

    pb_cell* cell = pb_cell_at(fb, sx, sy);
    int upper = (py & 1) == 0;

    if(cell->ch != 0x2580u){
        pb_color base = cell->bg;
        cell->ch = 0x2580u;
        cell->fg = base;
        cell->bg = base;
        cell->style = 0;
    }

    if(upper) cell->fg = color;
    else cell->bg = color;
}

void pb_fb_plot_line(pb_fb* fb, int x0, int y0, int x1, int y1, pb_color color){
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = dx >= 0 ? 1 : -1;
    int sy = dy >= 0 ? 1 : -1;
    if(dx < 0) dx = -dx;
    if(dy < 0) dy = -dy;
    int err = dx - dy;
    for(;;){
        pb_fb_plot(fb, x0, y0, color);
        if(x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if(e2 > -dy){ err -= dy; x0 += sx; }
        if(e2 < dx){ err += dx; y0 += sy; }
    }
}

void pb_fb_plot_rect(pb_fb* fb, int x, int y, int w, int h, pb_color color){
    if(w <= 0 || h <= 0) return;
    pb_fb_plot_line(fb, x, y, x + w - 1, y, color);
    pb_fb_plot_line(fb, x, y + h - 1, x + w - 1, y + h - 1, color);
    pb_fb_plot_line(fb, x, y, x, y + h - 1, color);
    pb_fb_plot_line(fb, x + w - 1, y, x + w - 1, y + h - 1, color);
}

void pb_fb_plot_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_color color){
    if(w <= 0 || h <= 0) return;
    for(int py = y; py < y + h; py++){
        for(int px = x; px < x + w; px++){
            pb_fb_plot(fb, px, py, color);
        }
    }
}

void pb_fb_plot_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color){
    if(radius < 0) return;
    int x = radius;
    int y = 0;
    int err = 0;
    while(x >= y){
        pb_fb_plot(fb, cx + x, cy + y, color);
        pb_fb_plot(fb, cx + y, cy + x, color);
        pb_fb_plot(fb, cx - y, cy + x, color);
        pb_fb_plot(fb, cx - x, cy + y, color);
        pb_fb_plot(fb, cx - x, cy - y, color);
        pb_fb_plot(fb, cx - y, cy - x, color);
        pb_fb_plot(fb, cx + y, cy - x, color);
        pb_fb_plot(fb, cx + x, cy - y, color);
        y++;
        err += 1 + 2 * y;
        if(2 * (err - x) + 1 > 0){
            x--;
            err += 1 - 2 * x;
        }
    }
}

void pb_fb_plot_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color){
    if(radius < 0) return;
    for(int y = -radius; y <= radius; y++){
        for(int x = -radius; x <= radius; x++){
            if(x * x + y * y <= radius * radius){
                pb_fb_plot(fb, cx + x, cy + y, color);
            }
        }
    }
}
