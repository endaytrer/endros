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
    // page 0;
    translate_2_pages(proc->ptref_base, kernel_path, path);

    File program;
    if (getfile(&proc->cwd_file, kernel_path, &program) < 0) {
        printk("[kernel] Unable to find file\n");
        return -1;
    }
    if (program.type == DIRECTORY || (program.permission & PERMISSION_X) == 0 || (program.permission & PERMISSION_R) == 0) {
        printk("[kernel] Program not executable\n");
        return -1;
    }
    kfree(kernel_path, 2 * PAGESIZE);
    free_process_space(cpus[cpuid].running);
    return load_process(cpus[cpuid].running, &program);
}