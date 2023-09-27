#ifndef __SYSCALL_H
#define __SYSCALL_H
#include <stdint.h>


#define SYS_READ 63
#define SYS_WRITE 64
#define SYS_EXIT 93
#define SYS_YIELD 124
#define SYS_GET_TIME 169
#define SYS_SBRK 214
#define SYS_FORK 220
#define SYS_EXEC 221
#define SYS_WAITPID 260

#define SYSCALL static inline __attribute__((always_inline)) long

SYSCALL syscall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    register uint64_t x10 asm("x10") = arg0;
    register uint64_t x11 asm("x11") = arg1;
    register uint64_t x12 asm("x12") = arg2;
    register uint64_t x17 asm("x17") = id;
    asm volatile(
        "ecall\n"
        : "+r" (x10)
        : "r"(x10), "r" (x11), "r" (x12), "r" (x17)
    );
    return x10;
}

SYSCALL sys_read(uint64_t fd, char *buffer, uint64_t size) {
    return syscall(SYS_READ, fd, (uint64_t)buffer, size);
}
SYSCALL sys_write(uint64_t fd, const char *buffer, uint64_t size) {
    return syscall(SYS_WRITE, fd, (uint64_t)buffer, size);
}

SYSCALL sys_exit(int32_t xstate) {
    return syscall(SYS_EXIT, (uint64_t) xstate, 0, 0);
}

SYSCALL sys_yield(void) {
    return syscall(SYS_YIELD, 0, 0, 0);
}

SYSCALL sys_get_time(void *ts, uint64_t _tz) {
    return syscall(SYS_GET_TIME, (uint64_t)ts, _tz, 0);
}

SYSCALL sys_sbrk(int64_t size) {
    return syscall(SYS_SBRK, (uint64_t)size, 0, 0);
}

SYSCALL sys_fork(void) {
    return syscall(SYS_FORK, 0, 0, 0);
}

SYSCALL sys_exec(const char *path) {
    return syscall(SYS_EXEC, (uint64_t)path, 0, 0);
}

SYSCALL sys_waitpid(int pid) {
    return syscall(SYS_WAITPID, pid, 0, 0);
}

#endif