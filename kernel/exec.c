#include <string.h>
#include "printk.h"
#include "syscall.h"
#include "process.h"
#include "pagetable.h"
#include "filesystem.h"
#include "fs_file.h"
extern const struct {
    const char *name;
    void (*elf)();
} name_app_map[];

i64 sys_exec(const char *path) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;

    // copy path from user space to kernel space.
    // supporting path for at most 1 page, spanning in two pages.
    char *kernel_path = kalloc(2 * PAGESIZE);
    char *dist_ptr = kernel_path;

    // page 0;
    u64 offset = OFFSET((u64)path);
    vpn_t user_vpn = ADDR_2_PAGE(path);
    vpn_t kernel_vpn = walkupt(proc->ptref_base, user_vpn);
    bool within_page = false;
    while (!within_page) {
        for (char *ptr = (char *)ADDR(kernel_vpn, offset); ptr != (char *)PAGE_2_ADDR(kernel_vpn + 1);) {
            *dist_ptr = *ptr;
            if (*ptr == '\0') {
                within_page = true;
                break;
            }
            dist_ptr++;
            ptr++;
        }
        user_vpn += 1;
        kernel_vpn = walkupt(proc->ptref_base, user_vpn);
        offset = 0;
    }
    FSFile cwd;
    cwd.inum = proc->cwd_inode;
    cwd.fs = &rootfs;
    fs_file_init(&cwd);


    FSFile program;
    if (getfile(&cwd, kernel_path, &program) < 0) {
        printk("[kernel] Unable to find file\n");
        return -1;
    }

    File wrapper;
    wrap_fsfile(&wrapper, &program);

    kfree(kernel_path, 2 * PAGESIZE);
    free_process_space(cpus[cpuid].running);
    return load_process(cpus[cpuid].running, &wrapper);
}