#include "syscall.h"
#include "printk.h"

static int64_t(*syscall_table[])(uint64_t, uint64_t, uint64_t) = {
    [SYS_WRITE] = (int64_t (*)(uint64_t, uint64_t, uint64_t))sys_write,
    [SYS_EXIT]  = (int64_t (*)(uint64_t, uint64_t, uint64_t))sys_exit,
};

int64_t syscall(uint64_t id, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    if (!syscall_table[id]) {
        panic("Unsupported syscall id\n");
    }
    return syscall_table[id](arg0, arg1, arg2);
}