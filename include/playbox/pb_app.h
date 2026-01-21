#ifndef PLAYBOX_PB_APP_H
#define PLAYBOX_PB_APP_H

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

    typedef struct {
        const char* title;
        int target_fps;
        pb_on_init_fn on_init;
        pb_on_event_fn on_event;
        pb_on_update_fn on_update;
        pb_on_draw_fn on_draw;
        pb_on_shutdown_fn on_shutdown;
    } pb_app_desc;

    pb_app* pb_app_create(const pb_app_desc* desc, void* user);
    void pb_app_destroy(pb_app* app);

    int pb_app_run(pb_app* app);

    void pb_app_quit(pb_app* app);
    void pb_app_request_resize(pb_app* app);

    int pb_app_width(const pb_app* app);
    int pb_app_height(const pb_app* app);

    void pb_app_set_title(pb_app* app, const char* title);

    int pb_app_is_replay(const pb_app* app);
    int pb_app_is_recording(const pb_app* app);
    uint32_t pb_app_replay_seed(const pb_app* app);
    int pb_app_replay_initial_size(const pb_app* app, int* out_w, int* out_h);

    #ifdef __cplusplus
}
#endif

#endif
