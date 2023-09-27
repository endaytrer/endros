#include <stdbool.h>
#include <string.h>
#include "process.h"
#include "elf.h"
#include "printk.h"
#include "sbi.h"
#include "timer.h"

PCB process_control_table[NUM_PROCS];
CPU cpus[NCPU];
pid_t next_pid = 1;

void load_process(PCB *process, void *app_start) {
    // set process to be ready;

    vpn_t ptbase_vpn;

    pfn_t app_ptbase = uptalloc(&ptbase_vpn);
    process->ptbase_pfn = app_ptbase;
    process->ptbase_vpn = ptbase_vpn;

    PTReference_2 *ptref_base = kalloc(2 * PAGESIZE);
    process->ptref_base = ptref_base;

    // map trampoline
    extern void strampoline();
    uptmap(ptbase_vpn, ptref_base, 0, ADDR_2_PAGE(TRAMPOLINE), ADDR_2_PAGE(strampoline), PTE_VALID | PTE_READ | PTE_EXECUTE);
    Elf64_Ehdr *elf_header = (Elf64_Ehdr *)app_start;
    vpn_t max_vpn = 0;
    for (int i = 0; i < elf_header->e_phnum; i++) {
        Elf64_Phdr *program_header = (Elf64_Phdr *)((uint64_t)app_start + elf_header->e_phoff) + i;
        if (program_header->p_type != PT_LOAD) {
            continue;
        }
        uint64_t start_va = program_header->p_vaddr;
        uint64_t end_va = program_header->p_vaddr + program_header->p_memsz;
        uint64_t flags = PTE_VALID | PTE_USER;
        flags |= (program_header->p_flags & PF_R) ? PTE_READ : 0;
        flags |= (program_header->p_flags & PF_W) ? PTE_WRITE : 0;
        flags |= (program_header->p_flags & PF_X) ? PTE_EXECUTE : 0;
        void *program_start = (void *)((uint64_t)app_start + program_header->p_offset);
        if (ADDR_2_PAGEUP(end_va) > max_vpn)
            max_vpn = ADDR_2_PAGEUP(end_va);
        for (vpn_t vpn = ADDR_2_PAGE(start_va); vpn < ADDR_2_PAGEUP(end_va); ++vpn) {
            vpn_t kernel_vpn;
            pfn_t pfn = uptalloc(&kernel_vpn);
            uint64_t offset = (uint64_t)PAGE_2_ADDR(vpn) - start_va;
            memcpy(PAGE_2_ADDR(kernel_vpn), (void *)((uint64_t)program_start + offset), PAGESIZE);
            uptmap(ptbase_vpn, ptref_base, kernel_vpn, vpn, pfn, flags);
        }
    }

    // map user stack;
    // guard page



    max_vpn += 1;
    vpn_t heap_bottom = max_vpn + USER_STACK_SIZE / PAGESIZE;
    for (vpn_t vpn = max_vpn; vpn < heap_bottom; vpn++) {
        vpn_t kernel_vpn;
        pfn_t pfn = uptalloc(&kernel_vpn);
        uptmap(ptbase_vpn, ptref_base, kernel_vpn, vpn, pfn, PTE_USER | PTE_VALID | PTE_READ | PTE_WRITE);
    }
    process->brk = PAGE_2_ADDR(heap_bottom);
    process->heap_bottom = PAGE_2_ADDR(heap_bottom);

    // trap context
    vpn_t trap_vpn;
    pfn_t pfn = uptalloc(&trap_vpn);
    uptmap(ptbase_vpn, ptref_base, trap_vpn, ADDR_2_PAGE(TRAPFRAME), pfn, PTE_VALID | PTE_READ | PTE_WRITE);
    process->trapframe = PAGE_2_ADDR(trap_vpn);

    // kernel stack with guard page
    uint8_t *kernel_sp = kalloc(KERNEL_STACK_SIZE + PAGESIZE);

    // ptunmaping guard page
    pfn_t kernel_stack_pfn = ptunmap(ADDR_2_PAGE(kernel_sp));

    process->kernel_stack_vpn = ADDR_2_PAGE(kernel_sp);
    process->kernel_stack_pfn = kernel_stack_pfn;

    kernel_sp += KERNEL_STACK_SIZE + PAGESIZE;

    app_init_context(PAGE_2_ADDR(trap_vpn), elf_header->e_entry, (uint64_t)PAGE_2_ADDR(max_vpn) + USER_STACK_SIZE, (uint64_t)kernel_sp);
}
void free_process_space(PCB *process) {
    ptref_free(process->ptbase_pfn, process->ptbase_vpn, process->ptref_base);
    
    ptmap(process->kernel_stack_vpn, process->kernel_stack_pfn, PTE_VALID | PTE_READ | PTE_WRITE);
    // remap guard page
    kfree(PAGE_2_ADDR(process->kernel_stack_vpn), KERNEL_STACK_SIZE + PAGESIZE);
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
    for (uint64_t i = 0; i < NUM_PROCS; i++) {
        if (process_control_table[i].status == UNUSED) {
            if (!process)
                process = process_control_table + i;

            break;
        }
    }

    // load init

    process->status = READY;
    process->cpuid = cpuid;
    process->pid = next_pid;
    process->flags = 0;
    load_process(process, (void *)*((const uint64_t *)_num_app + 1));

    enable_timer_interrupt();
    set_next_trigger();
    run(cpuid, process);
}

void schedule(int cpuid) {
    // round-robin scheduler
    uint64_t id = cpus[cpuid].running - process_control_table;
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