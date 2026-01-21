
#ifndef PLAYBOX_PB_TERM_H
#define PLAYBOX_PB_TERM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pb_term pb_term;

pb_term* pb_term_create(void);
void pb_term_destroy(pb_term* t);

int pb_term_enter(pb_term* t);
void pb_term_leave(pb_term* t);

int pb_term_get_size(pb_term* t, int* out_w, int* out_h);

int pb_term_read(pb_term* t, uint8_t* buf, int cap);
int pb_term_write(pb_term* t, const char* s, int n);

void pb_term_set_nonblocking(pb_term* t, int enabled);

void pb_term_request_focus(pb_term* t);
void pb_term_enable_mouse(pb_term* t, int enabled);

int pb_term_poll_resize(pb_term* t);

#ifdef __cplusplus
}
#endif

#endif
