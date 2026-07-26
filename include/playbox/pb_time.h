#ifndef PLAYBOX_PB_TIME_H
#define PLAYBOX_PB_TIME_H

#include "pb_export.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

PB_API uint64_t pb_time_ns(void);
PB_API void pb_sleep_ms(int ms);
/* High-resolution sleep. Uses nanosleep + short spin for the last ~0.2ms. */
PB_API void pb_sleep_ns(uint64_t ns);
/* Sleep until an absolute CLOCK_MONOTONIC deadline (ns). No-op if already past. */
PB_API void pb_sleep_until_ns(uint64_t deadline_ns);

#ifdef __cplusplus
}
#endif

#endif
