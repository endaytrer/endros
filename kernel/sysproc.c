#include "syscall.h"
#include "batch.h"
#include "printk.h"
int64_t sys_exit(int32_t xstate) {
    printk("Application exited with code ");
    char buf[16];
    printk(itoa(xstate, buf));
    printk("\n");
    run_next_app();
    return 0;
}