#ifndef _K_SYSCALL_H
#define _K_SYSCALL_H
#include <stdint.h>


#define SYS_READ     63
#define SYS_WRITE    64
#define SYS_EXIT     93
#define SYS_YIELD    124
#define SYS_GET_TIME 169
#define SYS_SBRK     214
#define SYS_FORK     220
#define SYS_EXEC     221
#define SYS_WAITPID  260

typedef int pid_t;
typedef struct {
    uint64_t sec;
    uint64_t usec;
} TimeVal;

int64_t sys_write(uint64_t fd, const char *buf, uint64_t size);
int64_t sys_read(uint64_t fd, char *buf, uint64_t size);
int64_t sys_exit(int32_t xstate);
int64_t sys_yield(void);
int64_t sys_get_time(TimeVal *ts, uint64_t _tz);
int64_t sys_sbrk(int64_t size);
int64_t syscall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2);
int64_t sys_fork(void);
int64_t sys_exec(const char *path);
int64_t sys_waitpid(pid_t pid);

#endif