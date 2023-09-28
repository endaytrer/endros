#include "lib.h"
#include "syscall.h"
i64 read(u64 fd, char *buffer, u64 size) {
    return sys_read(fd, buffer, size);
}
i64 write(u64 fd, const char *buffer, u64 size) {
    return sys_write(fd, buffer, size);
}

i64 exit(i32 xstate) {
    return sys_exit(xstate);
}

i64 yield(void) {
    return sys_yield();
}

i64 get_time(TimeVal *ts, u64 _tz) {
    return sys_get_time(ts, _tz);
}

void *sbrk(i64 size) {
    return (void *)sys_sbrk(size);
}


pid_t fork(void) {
    return sys_fork();
}
i64 exec(const char *path) {
    return sys_exec(path);
}
i64 waitpid(pid_t pid) {
    return sys_waitpid(pid);
}
