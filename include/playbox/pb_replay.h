#ifndef PLAYBOX_PB_REPLAY_H
#define PLAYBOX_PB_REPLAY_H

#include "pb_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
    #endif

    typedef struct pb_replay pb_replay;

    pb_replay* pb_replay_open_record(const char* path);
    pb_replay* pb_replay_open_replay(const char* path);
    void pb_replay_close(pb_replay* r);

    int pb_replay_record_set_meta(pb_replay* r, uint32_t seed, uint32_t initial_w, uint32_t initial_h);

    uint32_t pb_replay_get_seed(const pb_replay* r);
    void pb_replay_get_initial_size(const pb_replay* r, uint32_t* out_w, uint32_t* out_h);

    int pb_replay_write_frame(pb_replay* r, double dt, int event_count);
    int pb_replay_write_event(pb_replay* r, const pb_event* ev);

    int pb_replay_read_frame(pb_replay* r, double* out_dt, int* out_event_count);
    int pb_replay_read_event(pb_replay* r, pb_event* out_ev);

    #ifdef __cplusplus
}
#endif

#endif
