#include "syscall.h"
#include "printk.h"
#include "fcntl.h"
#include "mem.h"
#include "process.h"
#include "sbi.h"

int64_t sys_read(uint64_t fd, char *buf, uint64_t size) {
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
    char *kernel_ptr = (char *)((uint64_t)PAGE_2_ADDR(kernel_vpn) | OFFSET(buf));
    *kernel_ptr = ch;
    return 0;
}

int64_t sys_write(uint64_t fd, const char *buf, uint64_t size) {
    if (fd != STDOUT) {
        printk("[kernel] Unsupported fileno.\n");
        return -1;
    }
    // translate write buffer
    uint64_t start = OFFSET(buf);
    int cpuid = 0;
    for (vpn_t user_vpn = ADDR_2_PAGE(buf); user_vpn != ADDR_2_PAGEUP(buf + size); ++user_vpn) {
        vpn_t kernel_vpn = walkupt(cpus[cpuid].running->ptref_base, user_vpn);
        
        printk((const char *)((uint64_t)PAGE_2_ADDR(kernel_vpn) | start));
        start = 0;
    }
    return 0;
}