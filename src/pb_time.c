#define _POSIX_C_SOURCE 200809L

#include "playbox/pb_time.h"
#include <time.h>

uint64_t pb_time_ns(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

void pb_sleep_ms(int ms){
    if(ms <= 0) return;
    pb_sleep_ns((uint64_t)ms * 1000000ull);
}

void pb_sleep_ns(uint64_t ns){
    if(ns == 0) return;
    /* Sleep most of the interval, then spin for precision (vsync-quality pacing). */
    const uint64_t spin_guard = 200000ull; /* 0.2ms */
    if(ns > spin_guard){
        uint64_t sleep_ns = ns - spin_guard;
        struct timespec ts;
        ts.tv_sec = (time_t)(sleep_ns / 1000000000ull);
        ts.tv_nsec = (long)(sleep_ns % 1000000000ull);
        while(nanosleep(&ts, &ts) != 0){
            /* EINTR — continue with remaining */
        }
    }
    uint64_t deadline = pb_time_ns() + (ns > spin_guard ? spin_guard : ns);
    while(pb_time_ns() < deadline){
        /* busy-wait tail */
    }
}

void pb_sleep_until_ns(uint64_t deadline_ns){
    uint64_t now = pb_time_ns();
    if(deadline_ns <= now) return;
    pb_sleep_ns(deadline_ns - now);
}
