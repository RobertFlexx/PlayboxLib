#ifndef PLAYBOX_PB_UI_H
#define PLAYBOX_PB_UI_H

#include "pb_export.h"
#include "pb_fb.h"
#include "pb_gfx.h"
#include "pb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pb_app pb_app;

/* ---- Popup / modal UI helpers ---- */

typedef enum {
    PB_POPUP_RESULT_NONE = 0,
    PB_POPUP_RESULT_CONFIRM = 1,
    PB_POPUP_RESULT_CANCEL = 2
} pb_popup_result;

typedef struct {
    const char* title;
    const char* body;
    const char* hint;
    int width;
    int height;
    pb_box_style box_style;
    pb_color border;
    pb_color title_fg;
    pb_color fill;
    pb_color body_fg;
    pb_color hint_fg;
    float shadow_alpha;
    int dim_backdrop;
    int center;
    int x, y;
} pb_popup_desc;

PB_API void pb_popup_desc_init(pb_popup_desc* desc);
PB_API void pb_popup_draw(pb_fb* fb, const pb_popup_desc* desc);
PB_API void pb_popup_draw_ex(pb_fb* fb, const pb_popup_desc* desc,
                             int* out_x, int* out_y, int* out_w, int* out_h);
PB_API pb_popup_result pb_popup_handle_event(const pb_event* ev);

PB_API void pb_toast_draw(pb_fb* fb, int y, const char* text,
                          pb_color fg, pb_color bg, uint16_t style);

PB_API void pb_ui_draw_frame_stats(pb_fb* fb, const pb_app* app, int x, int y);

/* ---- Immediate-mode widgets ---- */

typedef unsigned int pb_ui_id;

typedef struct {
    pb_color panel;
    pb_color border;
    pb_color text;
    pb_color muted;
    pb_color accent;
    pb_color hot;
    pb_color active;
    pb_color track;
    pb_box_style box_style;
} pb_ui_style;

typedef struct {
    const pb_app* app;
    pb_fb* fb;
    pb_ui_style style;
    int mouse_x, mouse_y;
    int mouse_down;
    int mouse_pressed;
    int mouse_released;
    pb_ui_id hot;
    pb_ui_id active;
    /* Layout cursor */
    int cx, cy;
    int row_h;
    int col_w;
    int indent;
    int content_x0;
    int content_x1;
    int stack_n;
    int stack_x[8];
    int stack_y[8];
    int stack_w[8];
} pb_ui_ctx;

PB_API void pb_ui_style_default(pb_ui_style* st);
PB_API void pb_ui_begin(pb_ui_ctx* ui, pb_fb* fb, const pb_app* app);
PB_API void pb_ui_end(pb_ui_ctx* ui);

PB_API void pb_ui_at(pb_ui_ctx* ui, int x, int y);
PB_API void pb_ui_spacer(pb_ui_ctx* ui, int h);
PB_API void pb_ui_begin_column(pb_ui_ctx* ui, int x, int y, int w);
PB_API void pb_ui_end_column(pb_ui_ctx* ui);
PB_API void pb_ui_begin_row(pb_ui_ctx* ui);
PB_API void pb_ui_end_row(pb_ui_ctx* ui);

PB_API void pb_ui_label(pb_ui_ctx* ui, const char* text);
PB_API void pb_ui_separator(pb_ui_ctx* ui);

/* Returns 1 on click (press+release while hot). */
PB_API int  pb_ui_button(pb_ui_ctx* ui, pb_ui_id id, const char* label, int w);
PB_API int  pb_ui_checkbox(pb_ui_ctx* ui, pb_ui_id id, const char* label, int* checked);
/* Returns 1 if value changed. value in [min,max]. */
PB_API int  pb_ui_slider(pb_ui_ctx* ui, pb_ui_id id, float* value, float minv, float maxv, int w);
PB_API void pb_ui_progress(pb_ui_ctx* ui, float t01, int w, const char* label);

/* Panel/window: draws frame, nests layout inside. Returns 1 if drawn. */
PB_API int  pb_ui_begin_panel(pb_ui_ctx* ui, int x, int y, int w, int h, const char* title);
PB_API void pb_ui_end_panel(pb_ui_ctx* ui);

/* Modal on top of existing popup helpers: draw + consume confirm/cancel. */
PB_API pb_popup_result pb_ui_modal(pb_ui_ctx* ui, const pb_popup_desc* desc, const pb_event* ev);

#ifdef __cplusplus
}
#endif

#endif
