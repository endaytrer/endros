#include "syscall.h"
#include "printk.h"

static void(*syscall_table[])() = {
    [SYS_WRITE]    = (void (*)())sys_write,
    [SYS_EXIT]     = (void (*)())sys_exit,
    [SYS_YIELD]    = (void (*)())sys_yield,
    [SYS_GET_TIME] = (void (*)())sys_get_time,
    [SYS_SBRK]     = (void (*)())sys_sbrk,
    [SYS_FORK]     = (void (*)())sys_fork,
    [SYS_EXEC]     = (void (*)())sys_exec,
    [SYS_WAITPID]     = (void (*)())sys_waitpid,
};

int64_t syscall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    if (!syscall_table[id]) {
        panic("[kernel] Unsupported syscall id\n");
    }
    return ((int64_t (*)(uint64_t, uint64_t, uint64_t))(syscall_table[id]))(arg0, arg1, arg2);
}