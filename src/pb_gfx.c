#include "playbox/pb_gfx.h"
#include "pb_utf8.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Braille bit positions for a 2x4 grid inside one cell:
     (0,0)=1  (1,0)=8
     (0,1)=2  (1,1)=16
     (0,2)=4  (1,2)=32
     (0,3)=64 (1,3)=128
*/
static const uint8_t PB_BRAILLE_BITS[4][2] = {
    {0x01, 0x08},
    {0x02, 0x10},
    {0x04, 0x20},
    {0x40, 0x80}
};

void pb_fb_set_camera(pb_fb* fb, int cam_x, int cam_y){
    if(!fb) return;
    fb->cam_x = cam_x;
    fb->cam_y = cam_y;
}

void pb_fb_get_camera(const pb_fb* fb, int* out_x, int* out_y){
    if(!fb) return;
    if(out_x) *out_x = fb->cam_x;
    if(out_y) *out_y = fb->cam_y;
}

void pb_cam2d_init(pb_cam2d* cam){
    if(!cam) return;
    memset(cam, 0, sizeof(*cam));
    cam->zoom = 1.0f;
}

void pb_cam2d_shake(pb_cam2d* cam, float mag_x, float mag_y){
    if(!cam) return;
    cam->shake_x = mag_x;
    cam->shake_y = mag_y;
}

void pb_cam2d_world_to_screen(const pb_cam2d* cam, float wx, float wy, int* sx, int* sy){
    if(!cam) return;
    float z = cam->zoom > 1e-4f ? cam->zoom : 1.0f;
    float sx_f = (wx - cam->target_x) * z + cam->offset_x - cam->shake_x;
    float sy_f = (wy - cam->target_y) * z + cam->offset_y - cam->shake_y;
    if(sx) *sx = (int)floorf(sx_f + 0.5f);
    if(sy) *sy = (int)floorf(sy_f + 0.5f);
}

void pb_cam2d_apply(pb_fb* fb, const pb_cam2d* cam){
    if(!fb || !cam) return;
    /* Map world origin so target lands at offset on screen.
     * screen = (world - target) * zoom + offset - shake
     * With integer camera: screen = world - cam
     * Approximate by setting cam so that at zoom≈1: cam = target - offset + shake
     * For zoom != 1, callers should transform draw coords via world_to_screen;
     * apply still snaps the integer camera for cell-space draws. */
    float z = cam->zoom > 1e-4f ? cam->zoom : 1.0f;
    float cam_x = cam->target_x - cam->offset_x / z + cam->shake_x / z;
    float cam_y = cam->target_y - cam->offset_y / z + cam->shake_y / z;
    pb_fb_set_camera(fb, (int)floorf(cam_x + 0.5f), (int)floorf(cam_y + 0.5f));
}

void pb_fb_set_clip(pb_fb* fb, int x, int y, int w, int h){
    if(!fb || w <= 0 || h <= 0) return;
    int x0 = x;
    int y0 = y;
    int x1 = x + w;
    int y1 = y + h;
    if(x0 < 0) x0 = 0;
    if(y0 < 0) y0 = 0;
    if(x1 > fb->w) x1 = fb->w;
    if(y1 > fb->h) y1 = fb->h;
    fb->clip_x0 = x0;
    fb->clip_y0 = y0;
    fb->clip_x1 = x1;
    fb->clip_y1 = y1;
}

void pb_fb_reset_clip(pb_fb* fb){
    if(!fb) return;
    fb->clip_x0 = 0;
    fb->clip_y0 = 0;
    fb->clip_x1 = fb->w;
    fb->clip_y1 = fb->h;
}

pb_color pb_rgb_ex(uint8_t r, uint8_t g, uint8_t b){
    return pb_rgb(r, g, b);
}

pb_cell pb_cell_ex(uint32_t ch, pb_color fg, pb_color bg, uint16_t style){
    return pb_cell_make(ch, fg, bg, style);
}

pb_fb* pb_fb_create(int w, int h){
    pb_fb* fb = (pb_fb*)malloc(sizeof(pb_fb));
    if(!fb) return NULL;
    *fb = pb_fb_make(w, h);
    if(!fb->cells){
        free(fb);
        return NULL;
    }
    return fb;
}

void pb_fb_destroy(pb_fb* fb){
    if(!fb) return;
    pb_fb_free(fb);
    free(fb);
}

const char* pb_version_string(void){
    return PB_VERSION_STRING;
}

void pb_version(int* major, int* minor, int* patch){
    if(major) *major = PB_VERSION_MAJOR;
    if(minor) *minor = PB_VERSION_MINOR;
    if(patch) *patch = PB_VERSION_PATCH;
}

void pb_fb_pixel(pb_fb* fb, int x, int y, pb_color color){
    pb_fb_put(fb, x, y, pb_cell_make(0x2588u, color, color, 0));
}

void pb_fb_plot_blend(pb_fb* fb, int px, int py, pb_color color, float alpha){
    if(!fb) return;
    if(alpha < 0.f) alpha = 0.f;
    if(alpha > 1.f) alpha = 1.f;
    int wx = px;
    int wy = py >> 1;
    pb_cell cur = pb_fb_get(fb, wx, wy);
    int upper = (py & 1) == 0;
    pb_color base;
    if(cur.ch == 0x2580u){
        base = upper ? cur.fg : cur.bg;
    }else{
        base = cur.bg;
    }
    pb_fb_plot(fb, px, py, pb_color_lerp(base, color, alpha));
}

void pb_fb_braille_clear(pb_fb* fb, pb_color bg){
    if(!fb) return;
    pb_fb_clear(fb, pb_cell_make(0x2800u, bg, bg, 0));
}

static void pb_braille_set_dot(pb_fb* fb, int px, int py, pb_color color, float alpha){
    if(!fb || !fb->cells) return;

    /* Map braille pixel -> cell, then apply camera in put path via world cell coords. */
    int cell_x = px >> 1;
    int cell_y = py >> 2;
    int lx = px & 1;
    int ly = py & 3;

    int sx = cell_x - fb->cam_x;
    int sy = cell_y - fb->cam_y;
    if(sx < 0 || sy < 0 || sx >= fb->w || sy >= fb->h) return;
    if(sx < fb->clip_x0 || sy < fb->clip_y0 || sx >= fb->clip_x1 || sy >= fb->clip_y1) return;

    pb_cell* cell = &fb->cells[(size_t)sy * (size_t)fb->w + (size_t)sx];
    uint32_t bits = 0;
    if(cell->ch >= 0x2800u && cell->ch <= 0x28FFu){
        bits = cell->ch - 0x2800u;
    }else{
        cell->bg = cell->bg;
        cell->fg = color;
    }

    bits |= PB_BRAILLE_BITS[ly][lx];
    cell->ch = 0x2800u + bits;

    if(alpha < 0.999f){
        cell->fg = pb_color_lerp(cell->fg, color, alpha);
    }else{
        cell->fg = color;
    }
}

void pb_fb_braille_plot(pb_fb* fb, int px, int py, pb_color color){
    pb_braille_set_dot(fb, px, py, color, 1.f);
}

void pb_fb_braille_plot_blend(pb_fb* fb, int px, int py, pb_color color, float alpha){
    pb_braille_set_dot(fb, px, py, color, alpha);
}

void pb_fb_braille_line(pb_fb* fb, int x0, int y0, int x1, int y1, pb_color color){
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = dx >= 0 ? 1 : -1;
    int sy = dy >= 0 ? 1 : -1;
    if(dx < 0) dx = -dx;
    if(dy < 0) dy = -dy;
    int err = dx - dy;
    for(;;){
        pb_fb_braille_plot(fb, x0, y0, color);
        if(x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if(e2 > -dy){ err -= dy; x0 += sx; }
        if(e2 < dx){ err += dx; y0 += sy; }
    }
}

void pb_fb_braille_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_color color){
    if(w <= 0 || h <= 0) return;
    for(int py = y; py < y + h; py++){
        for(int px = x; px < x + w; px++){
            pb_fb_braille_plot(fb, px, py, color);
        }
    }
}

void pb_fb_braille_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color){
    if(radius < 0) return;
    int x = radius;
    int y = 0;
    int err = 0;
    while(x >= y){
        pb_fb_braille_plot(fb, cx + x, cy + y, color);
        pb_fb_braille_plot(fb, cx + y, cy + x, color);
        pb_fb_braille_plot(fb, cx - y, cy + x, color);
        pb_fb_braille_plot(fb, cx - x, cy + y, color);
        pb_fb_braille_plot(fb, cx - x, cy - y, color);
        pb_fb_braille_plot(fb, cx - y, cy - x, color);
        pb_fb_braille_plot(fb, cx + y, cy - x, color);
        pb_fb_braille_plot(fb, cx + x, cy - y, color);
        y++;
        err += 1 + 2 * y;
        if(2 * (err - x) + 1 > 0){
            x--;
            err += 1 - 2 * x;
        }
    }
}

void pb_fb_braille_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color){
    if(radius < 0) return;
    for(int y = -radius; y <= radius; y++){
        for(int x = -radius; x <= radius; x++){
            if(x * x + y * y <= radius * radius){
                pb_fb_braille_plot(fb, cx + x, cy + y, color);
            }
        }
    }
}

void pb_fb_braille_line_rotated(pb_fb* fb, float cx, float cy, float length,
                                float angle_rad, pb_color color){
    float hx = cosf(angle_rad) * length * 0.5f;
    float hy = sinf(angle_rad) * length * 0.5f;
    pb_fb_braille_line(fb,
        (int)floorf(cx - hx + 0.5f), (int)floorf(cy - hy + 0.5f),
        (int)floorf(cx + hx + 0.5f), (int)floorf(cy + hy + 0.5f),
        color);
}

void pb_fb_braille_rect_rotated(pb_fb* fb, float cx, float cy, float w, float h,
                                float angle_rad, pb_color color, int filled){
    float hw = w * 0.5f, hh = h * 0.5f;
    float c = cosf(angle_rad), s = sinf(angle_rad);
    float corners[4][2] = {
        {-hw, -hh}, { hw, -hh}, { hw,  hh}, {-hw,  hh}
    };
    int px[4], py[4];
    for(int i = 0; i < 4; i++){
        float rx = corners[i][0] * c - corners[i][1] * s;
        float ry = corners[i][0] * s + corners[i][1] * c;
        px[i] = (int)floorf(cx + rx + 0.5f);
        py[i] = (int)floorf(cy + ry + 0.5f);
    }
    if(!filled){
        for(int i = 0; i < 4; i++){
            int j = (i + 1) & 3;
            pb_fb_braille_line(fb, px[i], py[i], px[j], py[j], color);
        }
        return;
    }
    /* Scan bbox and test point-in-rotated-rect via inverse rotation */
    int minx = px[0], maxx = px[0], miny = py[0], maxy = py[0];
    for(int i = 1; i < 4; i++){
        if(px[i] < minx) minx = px[i];
        if(px[i] > maxx) maxx = px[i];
        if(py[i] < miny) miny = py[i];
        if(py[i] > maxy) maxy = py[i];
    }
    for(int y = miny; y <= maxy; y++){
        for(int x = minx; x <= maxx; x++){
            float dx = (float)x - cx, dy = (float)y - cy;
            float lx = dx * c + dy * s;
            float ly = -dx * s + dy * c;
            if(lx >= -hw && lx <= hw && ly >= -hh && ly <= hh)
                pb_fb_braille_plot(fb, x, y, color);
        }
    }
}

void pb_fb_plot_line_angled(pb_fb* fb, float cx, float cy, float length,
                            float angle_rad, pb_color color){
    float hx = cosf(angle_rad) * length * 0.5f;
    float hy = sinf(angle_rad) * length * 0.5f;
    pb_fb_plot_line(fb,
        (int)floorf(cx - hx + 0.5f), (int)floorf(cy - hy + 0.5f),
        (int)floorf(cx + hx + 0.5f), (int)floorf(cy + hy + 0.5f),
        color);
}

void pb_fb_plot_rect_rotated(pb_fb* fb, float cx, float cy, float w, float h,
                             float angle_rad, pb_color color, int filled){
    float hw = w * 0.5f, hh = h * 0.5f;
    float c = cosf(angle_rad), s = sinf(angle_rad);
    float corners[4][2] = {{-hw,-hh},{hw,-hh},{hw,hh},{-hw,hh}};
    int px[4], py[4];
    for(int i = 0; i < 4; i++){
        float rx = corners[i][0] * c - corners[i][1] * s;
        float ry = corners[i][0] * s + corners[i][1] * c;
        px[i] = (int)floorf(cx + rx + 0.5f);
        py[i] = (int)floorf(cy + ry + 0.5f);
    }
    if(!filled){
        for(int i = 0; i < 4; i++){
            int j = (i + 1) & 3;
            pb_fb_plot_line(fb, px[i], py[i], px[j], py[j], color);
        }
        return;
    }
    int minx = px[0], maxx = px[0], miny = py[0], maxy = py[0];
    for(int i = 1; i < 4; i++){
        if(px[i] < minx) minx = px[i];
        if(px[i] > maxx) maxx = px[i];
        if(py[i] < miny) miny = py[i];
        if(py[i] > maxy) maxy = py[i];
    }
    for(int y = miny; y <= maxy; y++){
        for(int x = minx; x <= maxx; x++){
            float dx = (float)x - cx, dy = (float)y - cy;
            float lx = dx * c + dy * s;
            float ly = -dx * s + dy * c;
            if(lx >= -hw && lx <= hw && ly >= -hh && ly <= hh)
                pb_fb_plot(fb, x, y, color);
        }
    }
}

void pb_fb_fill_gradient_v(pb_fb* fb, int x, int y, int w, int h, pb_color top, pb_color bottom){
    if(!fb || w <= 0 || h <= 0) return;
    for(int row = 0; row < h; row++){
        float t = (h > 1) ? (float)row / (float)(h - 1) : 0.f;
        pb_color c = pb_color_lerp(top, bottom, t);
        pb_fb_hline(fb, x, y + row, w, pb_cell_make(' ', c, c, 0));
    }
}

void pb_fb_fill_gradient_h(pb_fb* fb, int x, int y, int w, int h, pb_color left, pb_color right){
    if(!fb || w <= 0 || h <= 0) return;
    for(int col = 0; col < w; col++){
        float t = (w > 1) ? (float)col / (float)(w - 1) : 0.f;
        pb_color c = pb_color_lerp(left, right, t);
        pb_fb_vline(fb, x + col, y, h, pb_cell_make(' ', c, c, 0));
    }
}

void pb_fb_fill_dither(pb_fb* fb, int x, int y, int w, int h, pb_color a, pb_color b, int pattern){
    if(!fb || w <= 0 || h <= 0) return;
    /* 2x2 Bayer-like patterns 0..4 density */
    static const int bayer[4] = {0, 2, 3, 1};
    int thr = pattern;
    if(thr < 0) thr = 0;
    if(thr > 4) thr = 4;
    for(int yy = 0; yy < h; yy++){
        for(int xx = 0; xx < w; xx++){
            int i = ((yy & 1) << 1) | (xx & 1);
            pb_color c = (bayer[i] < thr) ? a : b;
            pb_fb_put(fb, x + xx, y + yy, pb_cell_make(' ', c, c, 0));
        }
    }
}

pb_sheet pb_sheet_wrap(pb_fb atlas, int tile_w, int tile_h){
    pb_sheet s;
    memset(&s, 0, sizeof(s));
    s.atlas = atlas;
    s.tile_w = tile_w > 0 ? tile_w : 1;
    s.tile_h = tile_h > 0 ? tile_h : 1;
    s.cols = atlas.w / s.tile_w;
    s.rows = atlas.h / s.tile_h;
    if(s.cols < 1) s.cols = 1;
    if(s.rows < 1) s.rows = 1;
    s.owns_atlas = 0;
    return s;
}

pb_sheet pb_sheet_create(int cols, int rows, int tile_w, int tile_h){
    if(cols < 1) cols = 1;
    if(rows < 1) rows = 1;
    if(tile_w < 1) tile_w = 1;
    if(tile_h < 1) tile_h = 1;
    pb_fb atlas = pb_fb_make(cols * tile_w, rows * tile_h);
    pb_sheet s = pb_sheet_wrap(atlas, tile_w, tile_h);
    s.owns_atlas = 1;
    return s;
}

void pb_sheet_free(pb_sheet* sheet){
    if(!sheet) return;
    if(sheet->owns_atlas){
        pb_fb_free(&sheet->atlas);
        sheet->owns_atlas = 0;
    }
}

void pb_sheet_set_tile(pb_sheet* sheet, int tile_id, const pb_fb* src){
    if(!sheet || !src || tile_id < 0) return;
    int col = tile_id % sheet->cols;
    int row = tile_id / sheet->cols;
    if(row >= sheet->rows) return;
    int dx = col * sheet->tile_w;
    int dy = row * sheet->tile_h;
    /* Write into atlas in screen space (cam=0). */
    int old_cx = sheet->atlas.cam_x, old_cy = sheet->atlas.cam_y;
    sheet->atlas.cam_x = 0;
    sheet->atlas.cam_y = 0;
    for(int y = 0; y < sheet->tile_h; y++){
        for(int x = 0; x < sheet->tile_w; x++){
            pb_cell c = (x < src->w && y < src->h) ? pb_fb_get_screen(src, x, y)
                                                   : pb_cell_make(' ', pb_rgb(0,0,0), pb_rgb(0,0,0), 0);
            pb_fb_put_screen(&sheet->atlas, dx + x, dy + y, c);
        }
    }
    sheet->atlas.cam_x = old_cx;
    sheet->atlas.cam_y = old_cy;
}

void pb_fb_blit_tile(pb_fb* dst, int dx, int dy, const pb_sheet* sheet, int tile_id){
    if(!dst || !sheet || tile_id < 0) return;
    int col = tile_id % sheet->cols;
    int row = tile_id / sheet->cols;
    if(row >= sheet->rows) return;
    pb_fb_blit_region(dst, dx, dy, &sheet->atlas,
                      col * sheet->tile_w, row * sheet->tile_h,
                      sheet->tile_w, sheet->tile_h);
}

void pb_fb_blit_tile_masked(pb_fb* dst, int dx, int dy, const pb_sheet* sheet, int tile_id, uint32_t transparent_ch){
    if(!dst || !sheet || tile_id < 0) return;
    int col = tile_id % sheet->cols;
    int row = tile_id / sheet->cols;
    if(row >= sheet->rows) return;
    int sx0 = col * sheet->tile_w;
    int sy0 = row * sheet->tile_h;
    for(int y = 0; y < sheet->tile_h; y++){
        for(int x = 0; x < sheet->tile_w; x++){
            pb_cell c = pb_fb_get_screen(&sheet->atlas, sx0 + x, sy0 + y);
            if(c.ch == transparent_ch) continue;
            pb_fb_put(dst, dx + x, dy + y, c);
        }
    }
}

void pb_anim_reset(pb_anim* anim){
    if(!anim) return;
    anim->t = 0.f;
}

void pb_anim_update(pb_anim* anim, double dt){
    if(!anim || anim->frame_count <= 0 || anim->fps <= 0.f) return;
    anim->t += (float)dt;
    float dur = (float)anim->frame_count / anim->fps;
    if(anim->loop){
        while(anim->t >= dur) anim->t -= dur;
    }else if(anim->t > dur){
        anim->t = dur;
    }
}

int pb_anim_frame(const pb_anim* anim){
    if(!anim || !anim->frames || anim->frame_count <= 0 || anim->fps <= 0.f) return 0;
    int idx = (int)(anim->t * anim->fps);
    if(idx < 0) idx = 0;
    if(idx >= anim->frame_count){
        idx = anim->loop ? (idx % anim->frame_count) : (anim->frame_count - 1);
    }
    return anim->frames[idx];
}

/* ---- Unicode display width (in pb_fb.c) ---- */

int pb_fb_measure_text_ex(const char* utf8){
    return pb_fb_measure_text(utf8);
}

int pb_fb_text_wrap(pb_fb* fb, int x, int y, int max_w, int max_h,
                    const char* utf8, pb_color fg, pb_color bg, uint16_t style){
    if(!fb || !utf8 || max_w <= 0 || max_h <= 0) return 0;
    int row = 0;
    int col = 0;
    const uint8_t* s = (const uint8_t*)utf8;
    size_t n = strlen(utf8);
    size_t i = 0;
    while(i < n && row < max_h){
        uint32_t cp = 0;
        size_t adv = 0;
        if(!pb_utf8_decode(s + i, n - i, &cp, &adv)){
            cp = '?';
            adv = 1;
        }
        if(cp == '\n'){
            row++;
            col = 0;
            i += adv;
            continue;
        }
        int cw = (cp == '\t') ? (4 - (col % 4)) : pb_char_width(cp);
        if(cw < 1) cw = 1;
        if(col + cw > max_w && col > 0){
            row++;
            col = 0;
            if(row >= max_h) break;
        }
        if(cp == '\t'){
            for(int k = 0; k < cw; k++){
                pb_fb_put(fb, x + col + k, y + row, pb_cell_make(' ', fg, bg, style));
            }
        }else{
            pb_fb_put(fb, x + col, y + row, pb_cell_make(cp, fg, bg, style));
            if(cw == 2){
                pb_fb_put(fb, x + col + 1, y + row, pb_cell_make(' ', fg, bg, style));
            }
        }
        col += cw;
        i += adv;
    }
    return row + (col > 0 || row == 0 ? 1 : 0);
}

void pb_fb_text_clipped(pb_fb* fb, int x, int y, int max_w,
                        const char* utf8, pb_color fg, pb_color bg, uint16_t style){
    if(!fb || !utf8 || max_w <= 0) return;
    int col = 0;
    const uint8_t* s = (const uint8_t*)utf8;
    size_t n = strlen(utf8);
    size_t i = 0;
    while(i < n){
        uint32_t cp = 0;
        size_t adv = 0;
        if(!pb_utf8_decode(s + i, n - i, &cp, &adv)){
            cp = '?';
            adv = 1;
        }
        if(cp == '\n') break;
        int cw = (cp == '\t') ? (4 - (col % 4)) : pb_char_width(cp);
        if(cw < 1) cw = 1;
        if(col + cw > max_w){
            if(max_w >= 1 && col < max_w){
                pb_fb_put(fb, x + max_w - 1, y, pb_cell_make(0x2026u, fg, bg, style)); /* … */
            }
            break;
        }
        if(cp == '\t'){
            for(int k = 0; k < cw; k++)
                pb_fb_put(fb, x + col + k, y, pb_cell_make(' ', fg, bg, style));
        }else{
            pb_fb_put(fb, x + col, y, pb_cell_make(cp, fg, bg, style));
            if(cw == 2) pb_fb_put(fb, x + col + 1, y, pb_cell_make(' ', fg, bg, style));
        }
        col += cw;
        i += adv;
    }
}

static void pb_box_ex_chars(pb_box_style st,
                            uint32_t* hc, uint32_t* vc,
                            uint32_t* tl, uint32_t* tr, uint32_t* bl, uint32_t* br){
    switch(st){
        case PB_BOX_DOUBLE:
            *hc=0x2550u; *vc=0x2551u; *tl=0x2554u; *tr=0x2557u; *bl=0x255Au; *br=0x255Du; break;
        case PB_BOX_ROUNDED:
            *hc=0x2500u; *vc=0x2502u; *tl=0x256Du; *tr=0x256Eu; *bl=0x2570u; *br=0x256Fu; break;
        case PB_BOX_HEAVY:
            *hc=0x2501u; *vc=0x2503u; *tl=0x250Fu; *tr=0x2513u; *bl=0x2517u; *br=0x251Bu; break;
        case PB_BOX_ASCII:
            *hc='-'; *vc='|'; *tl='+'; *tr='+'; *bl='+'; *br='+'; break;
        case PB_BOX_DASHED:
            *hc=0x2504u; *vc=0x2506u; *tl=0x250Cu; *tr=0x2510u; *bl=0x2514u; *br=0x2518u; break;
        case PB_BOX_SINGLE:
        default:
            *hc=0x2500u; *vc=0x2502u; *tl=0x250Cu; *tr=0x2510u; *bl=0x2514u; *br=0x2518u; break;
    }
}

void pb_fb_box_ex(pb_fb* fb, int x, int y, int w, int h,
                  pb_box_style box_style, pb_color fg, pb_color bg, uint16_t style){
    if(!fb || w < 2 || h < 2) return;
    uint32_t hc, vc, tl, tr, bl, br;
    pb_box_ex_chars(box_style, &hc, &vc, &tl, &tr, &bl, &br);
    pb_fb_put(fb, x, y, pb_cell_make(tl, fg, bg, style));
    pb_fb_put(fb, x+w-1, y, pb_cell_make(tr, fg, bg, style));
    pb_fb_put(fb, x, y+h-1, pb_cell_make(bl, fg, bg, style));
    pb_fb_put(fb, x+w-1, y+h-1, pb_cell_make(br, fg, bg, style));
    pb_cell hcell = pb_cell_make(hc, fg, bg, style);
    pb_cell vcell = pb_cell_make(vc, fg, bg, style);
    for(int xx = x + 1; xx < x + w - 1; xx++){
        pb_fb_put(fb, xx, y, hcell);
        pb_fb_put(fb, xx, y + h - 1, hcell);
    }
    for(int yy = y + 1; yy < y + h - 1; yy++){
        pb_fb_put(fb, x, yy, vcell);
        pb_fb_put(fb, x + w - 1, yy, vcell);
    }
}

void pb_fb_panel_ex(pb_fb* fb, int x, int y, int w, int h, const char* title,
                    pb_box_style box_style, pb_color border, pb_color title_fg,
                    pb_color fill, uint16_t style){
    if(!fb || w < 2 || h < 2) return;
    pb_fb_fill_rect(fb, x, y, w, h, pb_cell_make(' ', title_fg, fill, 0));
    pb_fb_box_ex(fb, x, y, w, h, box_style, border, fill, style);
    if(title && title[0] && w > 4){
        char buf[256];
        snprintf(buf, sizeof(buf), " %s ", title);
        pb_fb_text_clipped(fb, x + 1, y, w - 2, buf, title_fg, fill, style | PB_STYLE_BOLD);
    }
}

void pb_fb_shadow(pb_fb* fb, int x, int y, int w, int h, pb_color shadow, float alpha){
    if(!fb || w <= 0 || h <= 0) return;
    if(alpha < 0.f) alpha = 0.f;
    if(alpha > 1.f) alpha = 1.f;
    /* Drop shadow: right + bottom edges offset by 1 */
    for(int yy = 1; yy <= h; yy++){
        for(int xx = 1; xx <= w; xx++){
            if(yy < h && xx < w) continue; /* only rim outside content */
            int sx = x + xx;
            int sy = y + yy;
            pb_cell cur = pb_fb_get(fb, sx, sy);
            pb_color bg = pb_color_lerp(cur.bg, shadow, alpha);
            pb_color fg = pb_color_lerp(cur.fg, shadow, alpha * 0.5f);
            pb_fb_put(fb, sx, sy, pb_cell_make(cur.ch ? cur.ch : ' ', fg, bg, cur.style));
        }
    }
}

pb_cell pb_cell_blend(pb_cell dst, pb_cell src, float alpha, pb_blend_mode mode){
    if(alpha < 0.f) alpha = 0.f;
    if(alpha > 1.f) alpha = 1.f;
    pb_cell out = dst;
    if(mode == PB_BLEND_REPLACE || alpha >= 0.999f){
        out = src;
        return out;
    }
    if(mode == PB_BLEND_ALPHA){
        out.fg = pb_color_lerp(dst.fg, src.fg, alpha);
        out.bg = pb_color_lerp(dst.bg, src.bg, alpha);
        if(src.ch && src.ch != ' ') out.ch = src.ch;
        out.style = src.style | dst.style;
    }else if(mode == PB_BLEND_ADD){
        out.fg.r = (uint8_t)(dst.fg.r + (int)((float)src.fg.r * alpha) > 255 ? 255 : dst.fg.r + (int)((float)src.fg.r * alpha));
        out.fg.g = (uint8_t)(dst.fg.g + (int)((float)src.fg.g * alpha) > 255 ? 255 : dst.fg.g + (int)((float)src.fg.g * alpha));
        out.fg.b = (uint8_t)(dst.fg.b + (int)((float)src.fg.b * alpha) > 255 ? 255 : dst.fg.b + (int)((float)src.fg.b * alpha));
        out.bg = pb_color_lerp(dst.bg, src.bg, alpha * 0.5f);
        if(src.ch && src.ch != ' ') out.ch = src.ch;
    }else if(mode == PB_BLEND_MUL){
        out.fg.r = (uint8_t)((float)dst.fg.r * (pb_color_lerp(pb_rgb(255,255,255), src.fg, alpha).r) / 255.f);
        out.fg.g = (uint8_t)((float)dst.fg.g * (pb_color_lerp(pb_rgb(255,255,255), src.fg, alpha).g) / 255.f);
        out.fg.b = (uint8_t)((float)dst.fg.b * (pb_color_lerp(pb_rgb(255,255,255), src.fg, alpha).b) / 255.f);
        out.bg = pb_color_lerp(dst.bg, src.bg, alpha);
    }
    return out;
}

void pb_fb_put_blend(pb_fb* fb, int x, int y, pb_cell c, float alpha, pb_blend_mode mode){
    if(!fb) return;
    pb_cell dst = pb_fb_get(fb, x, y);
    pb_fb_put(fb, x, y, pb_cell_blend(dst, c, alpha, mode));
}

void pb_fb_blit_blend(pb_fb* dst, int dx, int dy, const pb_fb* src, float alpha, pb_blend_mode mode){
    if(!dst || !src) return;
    for(int y = 0; y < src->h; y++){
        for(int x = 0; x < src->w; x++){
            pb_cell s = pb_fb_get_screen(src, x, y);
            pb_fb_put_blend(dst, dx + x, dy + y, s, alpha, mode);
        }
    }
}

void pb_fb_fill_shade(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, int level){
    static const uint32_t shades[5] = {' ', 0x2591u, 0x2592u, 0x2593u, 0x2588u};
    if(level < 0) level = 0;
    if(level > 4) level = 4;
    pb_fb_fill_rect(fb, x, y, w, h, pb_cell_make(shades[level], fg, bg, 0));
}

/* Quadrant block: bits TL=1 TR=2 BL=4 BR=8 → Unicode block elements */
static const uint32_t PB_QUAD_GLYPH[16] = {
    ' ',      /* 0000 */
    0x2598u,  /* 0001 ▘ */
    0x259Du,  /* 0010 ▝ */
    0x2580u,  /* 0011 ▀ */
    0x2596u,  /* 0100 ▖ */
    0x258Cu,  /* 0101 ▌ */
    0x259Eu,  /* 0110 ▞ */
    0x259Bu,  /* 0111 ▛ */
    0x2597u,  /* 1000 ▗ */
    0x259Au,  /* 1001 ▚ */
    0x2590u,  /* 1010 ▐ */
    0x259Cu,  /* 1011 ▜ */
    0x2584u,  /* 1100 ▄ */
    0x2599u,  /* 1101 ▙ */
    0x259Fu,  /* 1110 ▟ */
    0x2588u   /* 1111 █ */
};

void pb_fb_quad_plot(pb_fb* fb, int px, int py, pb_color color){
    if(!fb) return;
    int cx = px >> 1;
    int cy = py >> 1;
    int lx = px & 1;
    int ly = py & 1;
    int bit = (ly == 0) ? (lx == 0 ? 1 : 2) : (lx == 0 ? 4 : 8);

    int sx = cx - fb->cam_x;
    int sy = cy - fb->cam_y;
    if(sx < 0 || sy < 0 || sx >= fb->w || sy >= fb->h) return;
    if(sx < fb->clip_x0 || sy < fb->clip_y0 || sx >= fb->clip_x1 || sy >= fb->clip_y1) return;

    pb_cell* cell = &fb->cells[(size_t)sy * (size_t)fb->w + (size_t)sx];
    int mask = 0;
    for(int i = 0; i < 16; i++){
        if(cell->ch == PB_QUAD_GLYPH[i]){ mask = i; break; }
    }
    if(cell->ch != PB_QUAD_GLYPH[mask] && cell->ch != ' ' && cell->ch != 0){
        mask = 0;
        cell->bg = cell->bg;
    }
    mask |= bit;
    cell->ch = PB_QUAD_GLYPH[mask];
    cell->fg = color;
    cell->style = 0;
}

void pb_fb_quad_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_color color){
    if(w <= 0 || h <= 0) return;
    for(int py = y; py < y + h; py++)
        for(int px = x; px < x + w; px++)
            pb_fb_quad_plot(fb, px, py, color);
}

void pb_fb_quad_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color){
    if(radius < 0) return;
    for(int y = -radius; y <= radius; y++)
        for(int x = -radius; x <= radius; x++)
            if(x*x + y*y <= radius*radius)
                pb_fb_quad_plot(fb, cx + x, cy + y, color);
}

static int pb_orient(int ax, int ay, int bx, int by, int cx, int cy){
    return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

void pb_fb_fill_triangle(pb_fb* fb, int x0, int y0, int x1, int y1, int x2, int y2, pb_cell c){
    if(!fb) return;
    int minx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int maxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int miny = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int maxy = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    int a = pb_orient(x0, y0, x1, y1, x2, y2);
    if(a == 0) return;
    for(int y = miny; y <= maxy; y++){
        for(int x = minx; x <= maxx; x++){
            int w0 = pb_orient(x1, y1, x2, y2, x, y);
            int w1 = pb_orient(x2, y2, x0, y0, x, y);
            int w2 = pb_orient(x0, y0, x1, y1, x, y);
            if(a > 0){
                if(w0 >= 0 && w1 >= 0 && w2 >= 0) pb_fb_put(fb, x, y, c);
            }else{
                if(w0 <= 0 && w1 <= 0 && w2 <= 0) pb_fb_put(fb, x, y, c);
            }
        }
    }
}

void pb_fb_braille_fill_triangle(pb_fb* fb, int x0, int y0, int x1, int y1, int x2, int y2, pb_color color){
    if(!fb) return;
    int minx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int maxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int miny = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int maxy = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    int a = pb_orient(x0, y0, x1, y1, x2, y2);
    if(a == 0) return;
    for(int y = miny; y <= maxy; y++){
        for(int x = minx; x <= maxx; x++){
            int w0 = pb_orient(x1, y1, x2, y2, x, y);
            int w1 = pb_orient(x2, y2, x0, y0, x, y);
            int w2 = pb_orient(x0, y0, x1, y1, x, y);
            int inside = (a > 0) ? (w0 >= 0 && w1 >= 0 && w2 >= 0) : (w0 <= 0 && w1 <= 0 && w2 <= 0);
            if(inside) pb_fb_braille_plot(fb, x, y, color);
        }
    }
}

void pb_fb_nine_slice(pb_fb* dst, int x, int y, int w, int h,
                      const pb_sheet* sheet, int tile_tl, int tile_t, int tile_tr,
                      int tile_l, int tile_c, int tile_r,
                      int tile_bl, int tile_b, int tile_br){
    if(!dst || !sheet || w < sheet->tile_w * 2 || h < sheet->tile_h * 2) return;
    int tw = sheet->tile_w;
    int th = sheet->tile_h;
    pb_fb_blit_tile(dst, x, y, sheet, tile_tl);
    pb_fb_blit_tile(dst, x + w - tw, y, sheet, tile_tr);
    pb_fb_blit_tile(dst, x, y + h - th, sheet, tile_bl);
    pb_fb_blit_tile(dst, x + w - tw, y + h - th, sheet, tile_br);
    for(int xx = x + tw; xx < x + w - tw; xx += tw)
        pb_fb_blit_tile(dst, xx, y, sheet, tile_t);
    for(int xx = x + tw; xx < x + w - tw; xx += tw)
        pb_fb_blit_tile(dst, xx, y + h - th, sheet, tile_b);
    for(int yy = y + th; yy < y + h - th; yy += th)
        pb_fb_blit_tile(dst, x, yy, sheet, tile_l);
    for(int yy = y + th; yy < y + h - th; yy += th)
        pb_fb_blit_tile(dst, x + w - tw, yy, sheet, tile_r);
    for(int yy = y + th; yy < y + h - th; yy += th)
        for(int xx = x + tw; xx < x + w - tw; xx += tw)
            pb_fb_blit_tile(dst, xx, yy, sheet, tile_c);
}

int pb_particles_init(pb_particles* ps, int capacity){
    if(!ps || capacity < 1) return 0;
    memset(ps, 0, sizeof(*ps));
    ps->items = (pb_particle*)calloc((size_t)capacity, sizeof(pb_particle));
    if(!ps->items) return 0;
    ps->capacity = capacity;
    return 1;
}

void pb_particles_free(pb_particles* ps){
    if(!ps) return;
    free(ps->items);
    ps->items = NULL;
    ps->count = ps->capacity = 0;
}

void pb_particles_emit(pb_particles* ps, float x, float y, float vx, float vy,
                       float life, pb_color color){
    if(!ps || !ps->items) return;
    for(int i = 0; i < ps->capacity; i++){
        if(!ps->items[i].alive){
            pb_particle* p = &ps->items[i];
            p->x = x; p->y = y; p->vx = vx; p->vy = vy;
            p->life = life; p->max_life = life;
            p->color = color; p->alive = 1;
            if(i >= ps->count) ps->count = i + 1;
            return;
        }
    }
}

void pb_particles_update(pb_particles* ps, double dt){
    if(!ps) return;
    float fdt = (float)dt;
    for(int i = 0; i < ps->count; i++){
        pb_particle* p = &ps->items[i];
        if(!p->alive) continue;
        p->x += p->vx * fdt;
        p->y += p->vy * fdt;
        p->life -= fdt;
        if(p->life <= 0.f) p->alive = 0;
    }
}

void pb_particles_draw_braille(pb_fb* fb, const pb_particles* ps){
    if(!fb || !ps) return;
    for(int i = 0; i < ps->count; i++){
        const pb_particle* p = &ps->items[i];
        if(!p->alive) continue;
        float a = (p->max_life > 0.f) ? (p->life / p->max_life) : 1.f;
        pb_fb_braille_plot_blend(fb, (int)p->x, (int)p->y, p->color, a);
    }
}

void pb_particles_draw_half(pb_fb* fb, const pb_particles* ps){
    if(!fb || !ps) return;
    for(int i = 0; i < ps->count; i++){
        const pb_particle* p = &ps->items[i];
        if(!p->alive) continue;
        float a = (p->max_life > 0.f) ? (p->life / p->max_life) : 1.f;
        pb_fb_plot_blend(fb, (int)p->x, (int)p->y, p->color, a);
    }
}
