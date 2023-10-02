#include <string.h>
#include <fcntl.h>
#include "process.h"
#include "elf.h"
#include "printk.h"
#include "sbi.h"
#include "timer.h"
#include "filesystem.h"
#include "stdio.h"
#include "fs_file.h"

#define INIT_PROGRAM "/bin/init"


PCB process_control_table[NUM_PROCS];
CPU cpus[MAX_CPUS];
pid_t next_pid = 1;

i64 load_process(PCB *process, File *program) {
    // assert program is loadable and executable. need to be done by exterier function (i.e. `exec`), before freeing the previous process.
    // set process to be ready;

    vpn_t ptbase_vpn;

    pfn_t ptbase_pfn = dmalloc(&ptbase_vpn, 1);
    process->ptbase_pfn = ptbase_pfn;
    process->ptbase_vpn = ptbase_vpn;

    PTReference_2 *ptref_base = kalloc(2 * PAGESIZE);
    process->ptref_base = ptref_base;

    // map trampoline
    extern void strampoline();
    uptmap(ptbase_vpn, ptref_base, 0, ADDR_2_PAGE(TRAMPOLINE), ADDR_2_PAGE(strampoline), PTE_VALID | PTE_READ | PTE_EXECUTE);

    // since it is easier to deal with continuous virtual addresses, use kalloc.
    void *buf = kalloc(program->size);
    wrapped_read(program, 0, buf, program->size);
    Elf64_Ehdr *elf_header = (Elf64_Ehdr *)buf;
    vpn_t max_vpn = 0;
    for (int i = 0; i < elf_header->e_phnum; i++) {
        Elf64_Phdr *program_header = (Elf64_Phdr *)((u64)buf + elf_header->e_phoff) + i;
        if (program_header->p_type != PT_LOAD) {
            continue;
        }
        u64 start_va = program_header->p_vaddr;
        u64 end_va = program_header->p_vaddr + program_header->p_memsz;
        u64 flags = PTE_VALID | PTE_USER;
        flags |= (program_header->p_flags & PF_R) ? PTE_READ : 0;
        flags |= (program_header->p_flags & PF_W) ? PTE_WRITE : 0;
        flags |= (program_header->p_flags & PF_X) ? PTE_EXECUTE : 0;
        void *program_start = (void *)((u64)buf + program_header->p_offset);
        if (ADDR_2_PAGEUP(end_va) > max_vpn)
            max_vpn = ADDR_2_PAGEUP(end_va);
        for (vpn_t vpn = ADDR_2_PAGE(start_va); vpn < ADDR_2_PAGEUP(end_va); ++vpn) {
            vpn_t kernel_vpn;
            pfn_t pfn = dmalloc(&kernel_vpn, 1);
            u64 offset = PAGE_2_ADDR(vpn) - start_va;
            memcpy((void *)PAGE_2_ADDR(kernel_vpn), (void *)((u64)program_start + offset), PAGESIZE);
            uptmap(ptbase_vpn, ptref_base, kernel_vpn, vpn, pfn, flags);
        }
    }

    // map user stack;
    // guard page
    max_vpn += 1;
    vpn_t heap_bottom = max_vpn + USER_STACK_SIZE / PAGESIZE;
    for (vpn_t vpn = max_vpn; vpn < heap_bottom; vpn++) {
        vpn_t kernel_vpn;
        pfn_t pfn = dmalloc(&kernel_vpn, 1);
        uptmap(ptbase_vpn, ptref_base, kernel_vpn, vpn, pfn, PTE_USER | PTE_VALID | PTE_READ | PTE_WRITE);
    }
    process->brk = (void *)PAGE_2_ADDR(heap_bottom);
    process->heap_bottom = (void *)PAGE_2_ADDR(heap_bottom);

    // trap context
    vpn_t trap_vpn;
    pfn_t pfn = dmalloc(&trap_vpn, 1);
    uptmap(ptbase_vpn, ptref_base, trap_vpn, ADDR_2_PAGE(TRAPFRAME), pfn, PTE_VALID | PTE_READ | PTE_WRITE);
    process->trapframe = (TrapContext *)PAGE_2_ADDR(trap_vpn);

    // kernel stack with guard page
    u8 *kernel_sp = kalloc(KERNEL_STACK_SIZE + PAGESIZE);

    // ptunmaping guard page
    pfn_t kernel_stack_pfn = ptunmap(ADDR_2_PAGE(kernel_sp));

    process->kernel_stack_vpn = ADDR_2_PAGE(kernel_sp);
    process->kernel_stack_pfn = kernel_stack_pfn;

    kernel_sp += KERNEL_STACK_SIZE + PAGESIZE;

    app_init_context((TrapContext *)PAGE_2_ADDR(trap_vpn), elf_header->e_entry, PAGE_2_ADDR(max_vpn) + USER_STACK_SIZE, (u64)kernel_sp);

    return 0;
}
void free_process_space(PCB *process) {
    ptref_free(process->ptbase_pfn, process->ptbase_vpn, process->ptref_base);
    
    ptmap(process->kernel_stack_vpn, process->kernel_stack_pfn, PTE_VALID | PTE_READ | PTE_WRITE);
    // remap guard page
    kfree((void *)PAGE_2_ADDR(process->kernel_stack_vpn), KERNEL_STACK_SIZE + PAGESIZE);
}
void unload_process(PCB *process) {
    if (process->status != TERMINATED) {
        panic("Unloading unterminated process\n");
    }
    free_process_space(process);
    process->status = UNUSED;
}

void run(int cpuid, PCB *process) {
    cpus[cpuid].running = process;
    process->status = RUNNING;
    process->cpuid = cpuid;

    trap_return(cpuid);
}

void init_scheduler(void) {
    extern void _num_app(void);
    PCB *process = NULL;
    int cpuid = 0;
    for (u64 i = 0; i < NUM_PROCS; i++) {
        if (process_control_table[i].status == UNUSED) {
            if (!process)
                process = process_control_table + i;

            break;
        }
    }

    // load init

    process->status = READY;
    process->cpuid = cpuid;
    process->pid = next_pid++;
    process->flags = 0;

    // set cwd to be /
    process->cwd_file.fs = &rootfs;
    process->cwd_file.inum = rootfs.super.root_inode;
    fs_file_init(&process->cwd_file);

    // set opened files to be [STDIN, STDOUT, STDERR]
    memset(process->opened_files, 0, sizeof(process->opened_files));
    process->opened_files[STDIN] = (FileDescriptor) {
            .file = &stdin,
            .open_flags = O_RDONLY,
            .seek = 0
    };
    process->opened_files[STDOUT] = (FileDescriptor) {
            .file = &stdout,
            .open_flags = O_WRONLY,
            .seek = 0
    };
    process->opened_files[STDERR] = (FileDescriptor) {
            .file = &stderr,
            .open_flags = O_WRONLY,
            .seek = 0
    };

    FSFile program_fsfile;
    File program;
    getfile(&rootfs.root, INIT_PROGRAM, &program_fsfile);
    wrap_fsfile(&program, &program_fsfile);
    if (load_process(process, &program) < 0) {
        panic("[kernel] cannot load process\n");
    }

    enable_timer_interrupt();
    set_next_trigger();
    run(cpuid, process);
}

void schedule(int cpuid) {
    // round-robin scheduler
    u64 id = cpus[cpuid].running - process_control_table;
    bool flag = false;
    PCB *process = NULL;
    for (int target_id = (id + 1) % NUM_PROCS; !flag || target_id != (id + 1) % NUM_PROCS; target_id = (target_id + 1) % NUM_PROCS) {
        if (process_control_table[target_id].status == READY) {
            process = process_control_table + target_id;
            break;
        }
        if (target_id == id) {
            flag = true;
        }
    }
    if (process == NULL) {
        printk("[kernel] All tasks finished!\n");
        shutdown();
    }
    run(cpuid, process);
}