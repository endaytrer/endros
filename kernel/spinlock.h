#ifndef _K_SPINLOCK_H
#define _K_SPINLOCK_H
#include <type.h>
#define SPINLOCK_INITIALIZER 0
typedef u32 spinlock_t;

static inline __attribute__((always_inline)) void acquire(spinlock_t *spinlock) {
    while(__sync_lock_test_and_set(spinlock, 1) != 0)
    ;
}

static inline __attribute__((always_inline)) void release(spinlock_t *spinlock) {
    
    __sync_lock_release(spinlock);
}

#endif