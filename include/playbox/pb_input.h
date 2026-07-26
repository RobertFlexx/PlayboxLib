#ifndef PLAYBOX_PB_INPUT_H
#define PLAYBOX_PB_INPUT_H

#include "pb_export.h"
#include "pb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pb_input pb_input;

PB_API pb_input* pb_input_create(void);
PB_API void pb_input_destroy(pb_input* in);

PB_API int pb_input_attach(pb_input* in, void* term_handle);

PB_API int pb_input_poll(pb_input* in, pb_event* out_ev);

PB_API void pb_input_flush(pb_input* in);

#ifdef __cplusplus
}
#endif

#endif
