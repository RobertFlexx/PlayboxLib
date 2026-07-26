#ifndef PLAYBOX_PB_APP_H
#define PLAYBOX_PB_APP_H

#include "pb_export.h"
#include "pb_types.h"
#include "pb_fb.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pb_app pb_app;

typedef void (*pb_on_init_fn)(pb_app* app, void* user);
typedef void (*pb_on_event_fn)(pb_app* app, void* user, const pb_event* ev);
typedef void (*pb_on_update_fn)(pb_app* app, void* user, double dt);
typedef void (*pb_on_draw_fn)(pb_app* app, void* user, pb_fb* fb);
typedef void (*pb_on_shutdown_fn)(pb_app* app, void* user);

enum {
    PB_APP_FLAG_NO_AUTO_CLEAR = 1u << 0,
    PB_APP_FLAG_NO_MOUSE      = 1u << 1,
    PB_APP_FLAG_NO_FOCUS      = 1u << 2,
    PB_APP_FLAG_CUSTOM_CLEAR  = 1u << 3,
    /* Disable precise vsync-style frame pacing (uncapped / best-effort). */
    PB_APP_FLAG_NO_VSYNC      = 1u << 4
};

typedef struct {
    const char* title;
    int target_fps;
    uint32_t flags;
    pb_cell clear;
    pb_on_init_fn on_init;
    pb_on_event_fn on_event;
    pb_on_update_fn on_update;
    pb_on_draw_fn on_draw;
    pb_on_shutdown_fn on_shutdown;
} pb_app_desc;

PB_API pb_app* pb_app_create(const pb_app_desc* desc, void* user);
PB_API void pb_app_destroy(pb_app* app);

PB_API int pb_app_run(pb_app* app);

PB_API void pb_app_quit(pb_app* app);
PB_API void pb_app_request_resize(pb_app* app);

PB_API int pb_app_width(const pb_app* app);
PB_API int pb_app_height(const pb_app* app);

PB_API void pb_app_set_title(pb_app* app, const char* title);
PB_API void pb_app_set_clear(pb_app* app, pb_cell clear);
PB_API void pb_app_set_target_fps(pb_app* app, int fps);

/* Vsync-style pacing (default ON). Aligns presents to refresh_hz. */
PB_API void pb_app_set_vsync(pb_app* app, int enabled);
PB_API int pb_app_get_vsync(const pb_app* app);
/* Refresh cadence in Hz (defaults to target_fps, or PLAYBOX_REFRESH env). */
PB_API void pb_app_set_refresh_hz(pb_app* app, int hz);
PB_API int pb_app_get_refresh_hz(const pb_app* app);

PB_API int pb_app_is_replay(const pb_app* app);
PB_API int pb_app_is_recording(const pb_app* app);
PB_API uint32_t pb_app_replay_seed(const pb_app* app);
PB_API int pb_app_replay_initial_size(const pb_app* app, int* out_w, int* out_h);

/* Cursor / mouse capture (for FPS-style games). */
PB_API void pb_app_set_cursor_visible(pb_app* app, int visible);
PB_API void pb_app_set_mouse_capture(pb_app* app, int capture);

#ifdef __cplusplus
}
#endif

#endif
