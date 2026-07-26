#ifndef PLAYBOX_PB_TERM_H
#define PLAYBOX_PB_TERM_H

#include "pb_export.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pb_term pb_term;

PB_API pb_term* pb_term_create(void);
PB_API void pb_term_destroy(pb_term* t);

PB_API int pb_term_enter(pb_term* t);
PB_API void pb_term_leave(pb_term* t);

PB_API int pb_term_get_size(pb_term* t, int* out_w, int* out_h);

PB_API int pb_term_read(pb_term* t, uint8_t* buf, int cap);
PB_API int pb_term_write(pb_term* t, const char* s, int n);

PB_API void pb_term_set_nonblocking(pb_term* t, int enabled);

PB_API void pb_term_request_focus(pb_term* t);
PB_API void pb_term_enable_mouse(pb_term* t, int enabled);
/* visible: 1 show, 0 hide. Terminals start hidden after pb_term_enter. */
PB_API void pb_term_set_cursor_visible(pb_term* t, int visible);
/* capture: 1 = any-motion tracking (FPS look), 0 = button/drag only. */
PB_API void pb_term_set_mouse_capture(pb_term* t, int capture);

PB_API int pb_term_poll_resize(pb_term* t);

#ifdef __cplusplus
}
#endif

#endif
