#include "syscall.h"
#include "printk.h"
#include "fcntl.h"
#include "mem.h"
#include "process.h"
#include "sbi.h"

i64 sys_read(u64 fd, char *buf, u64 size) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;
    if (fd != STDIN) {
        printk("[kernel] Unsupported fileno.\n");
        return -1;
    }
    if (size != 1) {
        printk("[kernel] Unsupported fileno.\n");
        return -1;
    }
    char ch = kgetc();
    if (ch == 0 || ch == 255) {
        schedule(cpuid);
    }
    vpn_t kernel_vpn = walkupt(proc->ptref_base, ADDR_2_PAGE(buf));
    char *kernel_ptr = (char *)ADDR(kernel_vpn, OFFSET(buf));
    *kernel_ptr = ch;
    return 0;
}

i64 sys_write(u64 fd, const char *buf, u64 size) {
    if (fd != STDOUT) {
        printk("[kernel] Unsupported fileno.\n");
        return -1;
    }
    // translate write buffer
    u64 start = OFFSET(buf);
    int cpuid = 0;
    for (vpn_t user_vpn = ADDR_2_PAGE(buf); user_vpn != ADDR_2_PAGEUP(buf + size); ++user_vpn) {
        vpn_t kernel_vpn = walkupt(cpus[cpuid].running->ptref_base, user_vpn);
        
        printk((const char *)ADDR(kernel_vpn, start));
        start = 0;
    }
    return 0;
}