#ifndef PLAYBOX_PB_STATE_H
#define PLAYBOX_PB_STATE_H

#include "pb_export.h"
#include "pb_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pb_app pb_app;

PB_API int pb_is_key_down(const pb_app* app, pb_key key);
PB_API int pb_is_key_pressed(const pb_app* app, pb_key key);
PB_API int pb_is_key_released(const pb_app* app, pb_key key);

PB_API int pb_is_char_down(const pb_app* app, uint32_t codepoint);
PB_API int pb_is_char_pressed(const pb_app* app, uint32_t codepoint);

PB_API int pb_get_mouse_x(const pb_app* app);
PB_API int pb_get_mouse_y(const pb_app* app);
PB_API int pb_is_mouse_button_down(const pb_app* app, int button);
PB_API int pb_is_mouse_button_pressed(const pb_app* app, int button);
PB_API int pb_is_mouse_button_released(const pb_app* app, int button);
PB_API int pb_get_mouse_wheel(const pb_app* app);

PB_API double pb_get_frame_time(const pb_app* app);
PB_API int pb_get_fps(const pb_app* app);
PB_API int pb_is_focused(const pb_app* app);

/* ---- Frame detection / timing stats ---- */

typedef struct {
    double dt;            /* seconds since previous frame (clamped) */
    double update_ms;     /* on_update */
    double draw_ms;       /* clear + on_draw */
    double present_ms;    /* renderer present */
    double wait_ms;       /* vsync wait */
    double frame_ms;      /* update+draw+present (no wait) */
    int fps;              /* smoothed */
    int target_fps;
    int refresh_hz;       /* vsync cadence (may equal target) */
    int dropped;          /* 1 if this frame missed vsync deadline */
    int hitch;            /* 1 if frame_ms > 2x budget */
    uint64_t frame_index;
    uint64_t drop_count;
    uint64_t hitch_count;
} pb_frame_stats;

PB_API void pb_get_frame_stats(const pb_app* app, pb_frame_stats* out);
PB_API int pb_get_dropped_frames(const pb_app* app);
PB_API int pb_was_frame_dropped(const pb_app* app);
PB_API int pb_was_frame_hitch(const pb_app* app);

#ifdef __cplusplus
}
#endif

#endif
