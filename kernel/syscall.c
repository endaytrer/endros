#include "syscall.h"
#include "printk.h"

static void(*syscall_table[])() = {
    [SYS_CHDIR]    = (void (*)())sys_chdir,
    [SYS_OPENAT]   = (void (*)())sys_openat,
    [SYS_CLOSE]    = (void (*)())sys_close,
    [SYS_LSEEK]    = (void (*)())sys_lseek,
    [SYS_READ]     = (void (*)())sys_read,
    [SYS_WRITE]    = (void (*)())sys_write,
    [SYS_EXIT]     = (void (*)())sys_exit,
    [SYS_YIELD]    = (void (*)())sys_yield,
    [SYS_GET_TIME] = (void (*)())sys_get_time,
    [SYS_SBRK]     = (void (*)())sys_sbrk,
    [SYS_FORK]     = (void (*)())sys_fork,
    [SYS_EXEC]     = (void (*)())sys_exec,
    [SYS_WAITPID]  = (void (*)())sys_waitpid,
};

i64 syscall(u64 id, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6) {
    if (!syscall_table[id]) {
        panic("[kernel] Unsupported syscall id\n");
    }
    return ((i64 (*)(u64, u64, u64, u64, u64, u64, u64))(syscall_table[id]))(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
}