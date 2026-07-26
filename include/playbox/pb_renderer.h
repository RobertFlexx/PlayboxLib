#ifndef PLAYBOX_PB_RENDERER_H
#define PLAYBOX_PB_RENDERER_H

#include "pb_export.h"
#include "pb_fb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pb_renderer pb_renderer;

PB_API pb_renderer* pb_renderer_create(void);
PB_API void pb_renderer_destroy(pb_renderer* r);

PB_API int pb_renderer_bind(pb_renderer* r, void* term_handle);

PB_API void pb_renderer_set_clear(pb_renderer* r, pb_cell clear_cell);

PB_API int pb_renderer_present(pb_renderer* r, const pb_fb* fb);

PB_API void pb_renderer_force_full_redraw(pb_renderer* r);

#ifdef __cplusplus
}
#endif

#endif
