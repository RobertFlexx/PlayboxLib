
#ifndef PLAYBOX_PB_TIME_H
#define PLAYBOX_PB_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t pb_time_ns(void);
void pb_sleep_ms(int ms);

#ifdef __cplusplus
}
#endif

#endif
