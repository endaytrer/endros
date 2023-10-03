#ifndef _K_SYSCALL_H
#define _K_SYSCALL_H
#include <type.h>
#include "pagetable.h"

#define SYS_CHDIR    49
#define SYS_OPENAT   56
#define SYS_CLOSE    57
#define SYS_LSEEK    62
#define SYS_READ     63
#define SYS_WRITE    64
#define SYS_EXIT     93
#define SYS_YIELD    124
#define SYS_GET_TIME 169
#define SYS_SBRK     214
#define SYS_FORK     220
#define SYS_EXECVE   221
#define SYS_WAITPID  260

typedef int pid_t;
typedef struct {
    u64 sec;
    u64 usec;
} TimeVal;

void translate_buffer(PTReference_2 *ptref_base, void *kernel_buf, void *user_buf, u64 size, bool write);
// for translating path argument. Read only
void translate_2_pages(const PTReference_2 *ptref_base, void *kernel_buf, const void *user_buf, int unit_size);
i64 sys_chdir(const char *filename);
i64 sys_openat(i32 dfd, const char *filename, int flags, int mode);
i64 sys_close(u32 fd);
i64 sys_lseek(u32 fd, i64 off_high, i64 off_low, u64 *result, u32 whence);
i64 sys_write(u32 fd, const char *buf, u64 size);
i64 sys_read(u32 fd, char *buf, u64 size);
i64 sys_exit(i32 xstate);
i64 sys_yield(void);
i64 sys_get_time(TimeVal *ts, u64 _tz);
i64 sys_sbrk(i64 size);
i64 sys_fork(void);
i64 sys_execve(const char *path, const char *const *argv, const char *const *envp);
i64 sys_waitpid(pid_t pid);


i64 syscall(u64 id, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6);

#endif