#include "syscall.h"
#include "printk.h"
#include "fcntl.h"
#include "batch.h"

extern AppManager app_manager;
int64_t sys_write(uint64_t fd, const char *buf, uint64_t size) {
    if (fd != STDOUT) {
        printk("Unsupported fileno.\n");
        return -1;
    }
    // translate write buffer
    uint64_t start = OFFSET(buf);
    for (vpn_t user_vpn = ADDR_2_PAGE(buf); user_vpn != ADDR_2_PAGEUP(buf + size); ++user_vpn) {
        vpn_t kernel_vpn = walkupt(app_manager.current_task.ptref_base, user_vpn);
        
        printk((const char *)((uint64_t)PAGE_2_ADDR(kernel_vpn) | start));
        start = 0;
    }
    return 0;
}