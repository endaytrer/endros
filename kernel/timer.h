#ifndef _K_TIMER_H
#define _K_TIMER_H

#include <type.h>
#include "machine.h"
#include "trap.h"
#define TICKS_PER_SEC 10
#define MICRO_PER_SEC 1000000

static inline __attribute__((always_inline)) void enable_timer_interrupt(void) {
    u64 temp;
    asm volatile(
        "csrr %0, sie\n\t"
        "ori %0, %0, %1\n\t"
        "csrw sie, %0"
        :"=r" (temp)
        :"i"(SIE_STIMER)
    );
}

static inline __attribute__((always_inline)) u64 get_time(void) {
    u64 time;
    asm volatile(
        "csrr %0, time"
        : "=r" (time)
    );
    return time;
}

static inline __attribute__((always_inline)) u64 get_time_us(void) {
    return get_time() / (CLOCK_FREQ / MICRO_PER_SEC);
}

void set_next_trigger(void);

#endif