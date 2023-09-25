#include "syscall.h"

int64_t sys_write(uint64_t fd, const char *buffer, uint64_t size) {
    return syscall(SYS_WRITE, fd, (uint64_t)buffer, size);
}

int64_t sys_exit(int32_t xstate) {
    return syscall(SYS_EXIT, (uint64_t) xstate, 0, 0);
}