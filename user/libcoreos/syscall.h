#ifndef __SYSCALL_H
#define __SYSCALL_H
#include <type.h>


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

SYSCALL syscall(u64 id, u64 arg0, u64 arg1, u64 arg2) {
    register u64 x10 asm("x10") = arg0;
    register u64 x11 asm("x11") = arg1;
    register u64 x12 asm("x12") = arg2;
    register u64 x17 asm("x17") = id;
    asm volatile(
        "ecall\n"
        : "+r" (x10)
        : "r"(x10), "r" (x11), "r" (x12), "r" (x17)
    );
    return x10;
}

SYSCALL sys_read(u64 fd, char *buffer, u64 size) {
    return syscall(SYS_READ, fd, (u64)buffer, size);
}
SYSCALL sys_write(u64 fd, const char *buffer, u64 size) {
    return syscall(SYS_WRITE, fd, (u64)buffer, size);
}

SYSCALL sys_exit(i32 xstate) {
    return syscall(SYS_EXIT, (u64) xstate, 0, 0);
}

SYSCALL sys_yield(void) {
    return syscall(SYS_YIELD, 0, 0, 0);
}

SYSCALL sys_get_time(void *ts, u64 _tz) {
    return syscall(SYS_GET_TIME, (u64)ts, _tz, 0);
}

SYSCALL sys_sbrk(i64 size) {
    return syscall(SYS_SBRK, (u64)size, 0, 0);
}

SYSCALL sys_fork(void) {
    return syscall(SYS_FORK, 0, 0, 0);
}

SYSCALL sys_exec(const char *path) {
    return syscall(SYS_EXEC, (u64)path, 0, 0);
}

SYSCALL sys_waitpid(int pid) {
    return syscall(SYS_WAITPID, pid, 0, 0);
}

#endif