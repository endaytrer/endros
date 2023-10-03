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

i64 sys_execve(const char *path, const char *const *argv, const char *const *envp){
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;

    // copy path from user space to kernel space.
    // supporting path for at most 1 page, spanning in two pages.
    char *kernel_path = kalloc(2 * PAGESIZE);
    // page 0;
    translate_2_pages(proc->ptref_base, kernel_path, path, 1);


    // translating argv and argp
    char **kernel_argv = kalloc(2 * PAGESIZE);
    translate_2_pages(proc->ptref_base, kernel_argv, argv, sizeof(char *));

    char **kernel_envp = kalloc(2 * PAGESIZE);
    translate_2_pages(proc->ptref_base, kernel_envp, envp, sizeof(char *));

    int argc = 0;
    /// we want to push argv, envp and the stuff they pointed to to the user stack, so we need to track the size.
    /// size of argv and
    // stack layout
    // sp(16 bytes aligned) ->
    // argv[0]
    // argv[1]
    // argv[2]
    // ...
    // argv[n]
    // NULL
    // envp[0]
    // envp[1]
    // ...
    // envp[n]
    // NULL
    // strtab
    // (padding)
    // end
    u64 argsize = 0;
    for (char **argp = kernel_argv; *argp != NULL; ++argp) {
        char *argi = kalloc(2 * PAGESIZE);
        translate_2_pages(proc->ptref_base, argi, *argp, 1);
        *argp = argi;
        argc++;
        argsize += strlen(argi) + 1; // also include the 0
    }
    int envc = 0;
    for (char **envpp = kernel_envp; *envpp != NULL; ++envpp) {
        char *envpi = kalloc(2 * PAGESIZE);
        translate_2_pages(proc->ptref_base, envpi, *envpp, 1);
        *envpp = envpi;
        envc++;
        argsize += strlen(envpi) + 1; // also include the 0
    }


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
    free_process_space(proc);
    if (load_process(proc, &program) < 0) {
        printk("[kernel] load process failed");
        return -1;
    }
    u64 total_size = (argc + envc + 2) * sizeof(char *) + argsize;
    // make it 16-byte aligned
    if (total_size & 0xf)
        total_size = (total_size & ~((u64)0xf)) + 0x10;
    // sp -= total_size
    proc->trapframe->x[2] -= total_size;
    // push argv
    // mark temporary argc and argv
    u64 tsp = proc->trapframe->x[2];
    vpn_t user_vpn = ADDR_2_PAGE(tsp);
    vpn_t kernel_vpn = walkupt(proc->ptref_base, user_vpn);
    u64 tstrtab = tsp + (argc + envc + 2) * sizeof(char *);

    // put argc, argv and envp argument.
    // proc->trapframe->x[10] = argc; // not doing it here because when trap returns argc, it is the first argument.
    proc->trapframe->x[11] = tsp;
    proc->trapframe->x[12] = tsp + (argc + 1) * sizeof(char *);

    for (int i = 0; i <= argc; i++) {
        // now it is guaranteed there are no crossing pages.
        if (ADDR_2_PAGE(tsp) != user_vpn) {
            user_vpn = ADDR_2_PAGE(tsp);
            kernel_vpn = walkupt(proc->ptref_base, user_vpn);
        }
        if (i != argc) {
            *(u64 *)ADDR(kernel_vpn, OFFSET(tsp)) = tstrtab;
            u64 argi_len = strlen(kernel_argv[i]) + 1;
            translate_buffer(proc->ptref_base, kernel_argv[i], (void *)tstrtab, argi_len, false);
            kfree(kernel_argv[i], 2 * PAGESIZE);
            tstrtab += argi_len;
        } else {
            *(u64 *)ADDR(kernel_vpn, OFFSET(tsp)) = 0;
        }
        tsp += sizeof(char *);
    }
    kfree(kernel_argv, 2 * PAGESIZE);
    for (int i = 0; i <= envc; i++) {
        // now it is guaranteed there are no crossing pages.
        if (ADDR_2_PAGE(tsp) != user_vpn) {
            user_vpn = ADDR_2_PAGE(tsp);
            kernel_vpn = walkupt(proc->ptref_base, user_vpn);
        }
        if (i != envc) {
            *(u64 *)ADDR(kernel_vpn, OFFSET(tsp)) = tstrtab;
            u64 envpi_len = strlen(kernel_envp[i]) + 1;
            translate_buffer(proc->ptref_base, kernel_envp[i], (void *)tstrtab, envpi_len, false);
            kfree(kernel_envp[i], 2 * PAGESIZE);
            tstrtab += envpi_len;
        } else {
            *(u64 *)ADDR(kernel_vpn, OFFSET(tsp)) = 0;
        }
        tsp += sizeof(char *);
    }
    kfree(kernel_envp, 2 * PAGESIZE);
    return argc;
}