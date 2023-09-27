#include "lib.h"
#include "syscall.h"
int64_t read(uint64_t fd, char *buffer, uint64_t size) {
    return sys_read(fd, buffer, size);
}
int64_t write(uint64_t fd, const char *buffer, uint64_t size) {
    return sys_write(fd, buffer, size);
}

int64_t exit(int32_t xstate) {
    return sys_exit(xstate);
}

int64_t yield(void) {
    return sys_yield();
}

int64_t get_time(TimeVal *ts, uint64_t _tz) {
    return sys_get_time(ts, _tz);
}

void *sbrk(int64_t size) {
    return (void *)sys_sbrk(size);
}


pid_t fork(void) {
    return sys_fork();
}
int64_t exec(const char *path) {
    return sys_exec(path);
}
int64_t waitpid(pid_t pid) {
    return sys_waitpid(pid);
}
