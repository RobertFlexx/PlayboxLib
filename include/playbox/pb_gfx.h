#ifndef PLAYBOX_PB_GFX_H
#define PLAYBOX_PB_GFX_H

#include "pb_export.h"
#include "pb_fb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Display width (East-Asian / emoji aware) ---- */
/* pb_char_width is declared in pb_fb.h */

/* ---- Text wrap / clip ---- */
/* Returns number of rows written. max_w/max_h in cells. */
PB_API int pb_fb_text_wrap(pb_fb* fb, int x, int y, int max_w, int max_h,
                           const char* utf8, pb_color fg, pb_color bg, uint16_t style);
PB_API void pb_fb_text_clipped(pb_fb* fb, int x, int y, int max_w,
                               const char* utf8, pb_color fg, pb_color bg, uint16_t style);
PB_API int pb_fb_measure_text_ex(const char* utf8); /* alias of pb_fb_measure_text */

/* ---- Camera & scissor ---- */

PB_API void pb_fb_set_camera(pb_fb* fb, int cam_x, int cam_y);
PB_API void pb_fb_get_camera(const pb_fb* fb, int* out_x, int* out_y);
PB_API void pb_fb_set_clip(pb_fb* fb, int x, int y, int w, int h);
PB_API void pb_fb_reset_clip(pb_fb* fb);

/* Float 2D camera: target/zoom/offset/shake → snaps into pb_fb_set_camera. */
typedef struct {
    float target_x, target_y;
    float offset_x, offset_y; /* screen-space bias (e.g. center player) */
    float zoom;               /* 1 = identity; >1 zooms in (world shrinks on screen) */
    float shake_x, shake_y;
} pb_cam2d;

PB_API void pb_cam2d_init(pb_cam2d* cam);
PB_API void pb_cam2d_shake(pb_cam2d* cam, float mag_x, float mag_y);
PB_API void pb_cam2d_apply(pb_fb* fb, const pb_cam2d* cam);
/* World → screen cell (uses zoom around target). */
PB_API void pb_cam2d_world_to_screen(const pb_cam2d* cam, float wx, float wy, int* sx, int* sy);

/* ---- Box styles / panels / shadows ---- */

typedef enum {
    PB_BOX_SINGLE = 0,
    PB_BOX_DOUBLE = 1,
    PB_BOX_ROUNDED = 2,
    PB_BOX_HEAVY = 3,
    PB_BOX_ASCII = 4,
    PB_BOX_DASHED = 5
} pb_box_style;

PB_API void pb_fb_box_ex(pb_fb* fb, int x, int y, int w, int h,
                         pb_box_style box_style, pb_color fg, pb_color bg, uint16_t style);
PB_API void pb_fb_panel_ex(pb_fb* fb, int x, int y, int w, int h, const char* title,
                           pb_box_style box_style, pb_color border, pb_color title_fg,
                           pb_color fill, uint16_t style);
PB_API void pb_fb_shadow(pb_fb* fb, int x, int y, int w, int h, pb_color shadow, float alpha);

/* ---- Braille pixels: 2x4 dots per cell. Pixel space = (w*2) x (h*4) ---- */

PB_API void pb_fb_braille_clear(pb_fb* fb, pb_color bg);
PB_API void pb_fb_braille_plot(pb_fb* fb, int px, int py, pb_color color);
PB_API void pb_fb_braille_plot_blend(pb_fb* fb, int px, int py, pb_color color, float alpha);
PB_API void pb_fb_braille_line(pb_fb* fb, int x0, int y0, int x1, int y1, pb_color color);
PB_API void pb_fb_braille_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_color color);
PB_API void pb_fb_braille_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color);
PB_API void pb_fb_braille_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color);
/* Axis-aligned size (w,h) in braille pixels, rotated by angle_rad around center. */
PB_API void pb_fb_braille_rect_rotated(pb_fb* fb, float cx, float cy, float w, float h,
                                       float angle_rad, pb_color color, int filled);
PB_API void pb_fb_braille_line_rotated(pb_fb* fb, float cx, float cy, float length,
                                       float angle_rad, pb_color color);

/* ---- Quadrant pixels: 2x2 per cell (▖▗▘▙…). Space = (w*2) x (h*2) ---- */

PB_API void pb_fb_quad_plot(pb_fb* fb, int px, int py, pb_color color);
PB_API void pb_fb_quad_fill_rect(pb_fb* fb, int x, int y, int w, int h, pb_color color);
PB_API void pb_fb_quad_fill_circle(pb_fb* fb, int cx, int cy, int radius, pb_color color);

/* Half-block rotated helpers (pixel space = w x h*2). */
PB_API void pb_fb_plot_rect_rotated(pb_fb* fb, float cx, float cy, float w, float h,
                                    float angle_rad, pb_color color, int filled);
PB_API void pb_fb_plot_line_angled(pb_fb* fb, float cx, float cy, float length,
                                   float angle_rad, pb_color color);

/* ---- Full-block / shade fills ---- */

PB_API void pb_fb_pixel(pb_fb* fb, int x, int y, pb_color color);
PB_API void pb_fb_fill_shade(pb_fb* fb, int x, int y, int w, int h, pb_color fg, pb_color bg, int level);
/* level 0..4 → ' ' ░ ▒ ▓ █ */

/* ---- Soft blends & fills ---- */

typedef enum {
    PB_BLEND_REPLACE = 0,
    PB_BLEND_ALPHA = 1,
    PB_BLEND_ADD = 2,
    PB_BLEND_MUL = 3
} pb_blend_mode;

PB_API pb_cell pb_cell_blend(pb_cell dst, pb_cell src, float alpha, pb_blend_mode mode);
PB_API void pb_fb_put_blend(pb_fb* fb, int x, int y, pb_cell c, float alpha, pb_blend_mode mode);
PB_API void pb_fb_blit_blend(pb_fb* dst, int dx, int dy, const pb_fb* src, float alpha, pb_blend_mode mode);
PB_API void pb_fb_plot_blend(pb_fb* fb, int px, int py, pb_color color, float alpha);
PB_API void pb_fb_fill_gradient_v(pb_fb* fb, int x, int y, int w, int h, pb_color top, pb_color bottom);
PB_API void pb_fb_fill_gradient_h(pb_fb* fb, int x, int y, int w, int h, pb_color left, pb_color right);
PB_API void pb_fb_fill_dither(pb_fb* fb, int x, int y, int w, int h, pb_color a, pb_color b, int pattern);

/* ---- Triangles ---- */

PB_API void pb_fb_fill_triangle(pb_fb* fb, int x0, int y0, int x1, int y1, int x2, int y2, pb_cell c);
PB_API void pb_fb_braille_fill_triangle(pb_fb* fb, int x0, int y0, int x1, int y1, int x2, int y2, pb_color color);

/* ---- Sprite sheets ---- */

typedef struct {
    pb_fb atlas;
    int tile_w;
    int tile_h;
    int cols;
    int rows;
    int owns_atlas;
} pb_sheet;

PB_API pb_sheet pb_sheet_wrap(pb_fb atlas, int tile_w, int tile_h);
PB_API pb_sheet pb_sheet_create(int cols, int rows, int tile_w, int tile_h);
PB_API void pb_sheet_free(pb_sheet* sheet);
PB_API void pb_sheet_set_tile(pb_sheet* sheet, int tile_id, const pb_fb* src);
PB_API void pb_fb_blit_tile(pb_fb* dst, int dx, int dy, const pb_sheet* sheet, int tile_id);
PB_API void pb_fb_blit_tile_masked(pb_fb* dst, int dx, int dy, const pb_sheet* sheet, int tile_id, uint32_t transparent_ch);
PB_API void pb_fb_nine_slice(pb_fb* dst, int x, int y, int w, int h,
                             const pb_sheet* sheet, int tile_tl, int tile_t, int tile_tr,
                             int tile_l, int tile_c, int tile_r,
                             int tile_bl, int tile_b, int tile_br);

/* ---- Animation helper ---- */

typedef struct {
    const int* frames;
    int frame_count;
    float fps;
    float t;
    int loop;
} pb_anim;

PB_API void pb_anim_reset(pb_anim* anim);
PB_API void pb_anim_update(pb_anim* anim, double dt);
PB_API int pb_anim_frame(const pb_anim* anim);

/* ---- Lightweight particles (braille) ---- */

typedef struct {
    float x, y;
    float vx, vy;
    float life;
    float max_life;
    pb_color color;
    uint8_t alive;
} pb_particle;

typedef struct {
    pb_particle* items;
    int count;
    int capacity;
} pb_particles;

PB_API int pb_particles_init(pb_particles* ps, int capacity);
PB_API void pb_particles_free(pb_particles* ps);
PB_API void pb_particles_emit(pb_particles* ps, float x, float y, float vx, float vy,
                              float life, pb_color color);
PB_API void pb_particles_update(pb_particles* ps, double dt);
PB_API void pb_particles_draw_braille(pb_fb* fb, const pb_particles* ps);
PB_API void pb_particles_draw_half(pb_fb* fb, const pb_particles* ps);

/* ---- FFI-friendly constructors ---- */

PB_API pb_color pb_rgb_ex(uint8_t r, uint8_t g, uint8_t b);
PB_API pb_cell pb_cell_ex(uint32_t ch, pb_color fg, pb_color bg, uint16_t style);
PB_API pb_fb* pb_fb_create(int w, int h);
PB_API void pb_fb_destroy(pb_fb* fb);
PB_API const char* pb_version_string(void);
PB_API void pb_version(int* major, int* minor, int* patch);

#ifdef __cplusplus
}
#endif

#endif
