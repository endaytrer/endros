#ifndef _K_SYSCALL_H
#define _K_SYSCALL_H
#include <stdint.h>


#define SYS_WRITE 64
#define SYS_EXIT 93
#define SYS_YIELD 124
#define SYS_GET_TIME 169

typedef struct {
    uint64_t sec;
    uint64_t usec;
} TimeVal;

int64_t sys_write(uint64_t fd, const char *buf, uint64_t size);
int64_t sys_exit(int32_t xstate);
int64_t sys_yield(void);
int64_t sys_get_time(TimeVal *ts, uint64_t _tz);

int64_t syscall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2);

#endif