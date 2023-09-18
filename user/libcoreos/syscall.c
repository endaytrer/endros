#include "syscall.h"

int64_t sys_write(uint64_t fd, const char *buffer) {
    return syscall(SYS_WRITE, fd, (uint64_t)buffer, 0);
}

int64_t sys_exit(int32_t xstate) {
    return syscall(SYS_EXIT, (uint64_t) xstate, 0, 0);
}