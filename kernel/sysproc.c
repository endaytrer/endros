#include <string.h>
#include "syscall.h"
#include "printk.h"
#include "machine.h"
#include "timer.h"
#include "process.h"
#include "pagetable.h"
extern pid_t next_pid;

i64 sys_yield(void) {
    int cpuid = 0;
    schedule(cpuid);
    return 0;
}

i64 sys_sbrk(i64 size) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;
    void *old_brk = proc->brk;

    if ((u64)old_brk + size >= HEAP_TOP || (u64)old_brk + size < (u64)proc->heap_bottom) {
        printk("[kernel] trying to allocate more than heap size / less than 0");
        return -1;
    }
    if (size > 0) {
        for (int vpn = ADDR_2_PAGEUP(old_brk); vpn < ADDR_2_PAGEUP((u64)old_brk + size); ++vpn) {
            vpn_t kernel_vpn;
            pfn_t pfn = uptalloc(&kernel_vpn);
            uptmap(proc->ptbase_vpn, proc->ptref_base, kernel_vpn, vpn, pfn, PTE_USER | PTE_VALID | PTE_READ | PTE_WRITE);
        }
    }
    if (size < 0) {
        for (int vpn = ADDR_2_PAGEUP((u64)old_brk + size); vpn < ADDR_2_PAGEUP(old_brk); ++vpn) {
            uptunmap(proc->ptbase_vpn, proc->ptref_base, vpn);
        }
    }
    return (i64)old_brk;
}

i64 sys_get_time(TimeVal *ts, u64 _tz) {
    int cpuid = 0;
    vpn_t kernel_vpn = walkupt(cpus[cpuid].running->ptref_base, ADDR_2_PAGE(ts));
    TimeVal *kernel_ts = (TimeVal *)ADDR(kernel_vpn, OFFSET(ts));

    kernel_ts->usec = get_time_us();
    kernel_ts->sec = ts->usec / MICRO_PER_SEC;
    return 0;
}

i64 sys_exit(i32 xstate) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;

    char buf[16];
    printk("[kernel] Process ");
    printk(itoa(proc->pid, buf));
    printk(" exited with code ");
    printk(itoa(xstate, buf));
    printk("\n");

    // if it is waited by parent, reschedule parent and set to terminated
    if (proc->flags & FLAGS_WAITED) {
        proc->parent->status = READY;
        // set return code of waitpid() to be xstate
        proc->parent->trapframe->x[10] = xstate;
        terminate(proc);
        unload_process(proc);
        schedule(cpuid);
        return 0;
    }
    proc->status = ZOMBIE;
    proc->exit_code = xstate;
    schedule(cpuid);
    return 0;
}

i64 sys_fork(void) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;
    PCB *new_proc = NULL;
    for (u64 i = 0; i < NUM_PROCS; i++) {
        if (process_control_table[i].status != UNUSED) {
            continue;
        }
        new_proc = process_control_table + i;
        break;
    }
    if (!new_proc) {
        printk("[kernel] cannot allocate new process because process table is full.");
        return -1;
    }
    new_proc->pid = next_pid;
    new_proc->status = READY;
    new_proc->cpuid = cpuid;
    new_proc->parent = proc;
    new_proc->flags = 0;

    vpn_t ptbase_vpn;
    pfn_t ptbase_pfn = uptalloc(&ptbase_vpn);
    PTReference_2 *ptref_base = kalloc(2 * PAGESIZE);

    new_proc->ptbase_vpn = ptbase_vpn;
    new_proc->ptbase_pfn = ptbase_pfn;
    new_proc->ptref_base = ptref_base;

    extern void strampoline();

    // copy remaining spaces into new pagetable
    ptref_copy(ptbase_pfn, ptbase_vpn, ptref_base, proc->ptbase_pfn, proc->ptbase_vpn, proc->ptref_base);

    // map new trampoline
    uptmap(ptbase_vpn, ptref_base, 0, ADDR_2_PAGE(TRAMPOLINE), ADDR_2_PAGE(strampoline), PTE_VALID | PTE_READ | PTE_EXECUTE);
    new_proc->brk = proc->brk;
    new_proc->heap_bottom = proc->heap_bottom;

    new_proc->trapframe = (TrapContext *)PAGE_2_ADDR(walkupt(ptref_base, ADDR_2_PAGE(TRAPFRAME)));

    // allocate kernel stack with guard page
    u8 *kernel_sp = kalloc(KERNEL_STACK_SIZE + PAGESIZE);

    // ptunmaping guard page
    pfn_t kernel_stack_pfn = ptunmap(ADDR_2_PAGE(kernel_sp));

    new_proc->kernel_stack_vpn = ADDR_2_PAGE(kernel_sp);
    new_proc->kernel_stack_pfn = kernel_stack_pfn;

    kernel_sp += KERNEL_STACK_SIZE + PAGESIZE;

    // remap kernel stack
    new_proc->trapframe->kernel_sp = (u64)kernel_sp;

    // set return code of new process to be 0
    new_proc->trapframe->x[10] = 0;


    // copy fdt
    new_proc->cwd_inode = proc->cwd_inode;
    memcpy(new_proc->opened_files, proc->opened_files, sizeof(new_proc->opened_files));

    return next_pid++;
}

i64 sys_waitpid(pid_t pid) {
    int cpuid = 0;
    PCB *proc = cpus[cpuid].running;
    PCB *child_proc = NULL;
    for (int i = 0; i < NUM_PROCS; i++) {
        if (process_control_table[i].pid == pid && process_control_table[i].parent == proc) {
            child_proc = process_control_table + i;
        }
    }
    if (!child_proc) {
        printk("[kernel] Cannot wait pid\n");
        return -1;
    }
    // already exited
    if (child_proc->status == ZOMBIE) {
        terminate(child_proc);
        unload_process(child_proc);
        return 0;
    }
    child_proc->flags |= FLAGS_WAITED;
    proc->status = BLOCKED;
    schedule(cpuid);
    return 0;
}