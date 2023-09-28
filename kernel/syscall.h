#ifndef _K_SYSCALL_H
#define _K_SYSCALL_H
#include <type.h>


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
    u64 sec;
    u64 usec;
} TimeVal;

i64 sys_write(u64 fd, const char *buf, u64 size);
i64 sys_read(u64 fd, char *buf, u64 size);
i64 sys_exit(i32 xstate);
i64 sys_yield(void);
i64 sys_get_time(TimeVal *ts, u64 _tz);
i64 sys_sbrk(i64 size);
i64 syscall(u64 id, u64 arg0, u64 arg1, u64 arg2);
i64 sys_fork(void);
i64 sys_exec(const char *path);
i64 sys_waitpid(pid_t pid);

#endif