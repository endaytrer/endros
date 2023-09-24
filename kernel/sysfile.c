#include "syscall.h"
#include "printk.h"
#include "fcntl.h"
#include "batch.h"
int64_t sys_write(uint64_t fd, uint8_t *buf, uint64_t size) {
    if (fd == STDOUT) {
        printk(buf);
        return 0;
    };
    printk("Unsupported fileno.\n");
    run_next_app();
}