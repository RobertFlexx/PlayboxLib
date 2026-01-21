
#ifndef PLAYBOX_PB_RENDERER_H
#define PLAYBOX_PB_RENDERER_H

#include "pb_fb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pb_renderer pb_renderer;

pb_renderer* pb_renderer_create(void);
void pb_renderer_destroy(pb_renderer* r);

int pb_renderer_bind(pb_renderer* r, void* term_handle);

void pb_renderer_set_clear(pb_renderer* r, pb_cell clear_cell);

int pb_renderer_present(pb_renderer* r, const pb_fb* fb);

void pb_renderer_force_full_redraw(pb_renderer* r);

#ifdef __cplusplus
}
#endif

#endif
