#include "trap.h"
#include "printk.h"
#include "syscall.h"
#include "process.h"
#include "mem.h"
#include "timer.h"
#include <string.h>

#define SSTATUS_SPP 0x00000100

void set_sp(TrapContext *self, uint64_t sp) {
    self->x[2] = sp;
}

void trap_return(int cpuid) {
    asm volatile (
        "csrw stvec, %0"
        :: "r" (TRAMPOLINE)
    );

    extern void strampoline();
    extern void __restore();
    uint64_t token = ((uint64_t)1 << 63) | cpus[cpuid].running->ptbase_pfn;
    void (*restore)(void *trap_context, uint64_t user_token) = (void (*)(void *, uint64_t))(TRAMPOLINE + (uint64_t)__restore - (uint64_t)strampoline);
    asm volatile (
        "fence.i"
    );
    restore(TRAPFRAME, token);
}
void trap_handler(void) {
    int cpuid = 0;

    cpus[cpuid].running->status = READY;
    PCB *proc = cpus[cpuid].running;
    uint64_t scause;
    uint64_t stval;

    asm volatile (
        "csrr %0, scause\n\t"
        "csrr %1, stval\n\t"
        : "=r" (scause), "=r" (stval)
    );
    char buf[16];
    uint64_t trap_code = TRAP_CODE(scause);
    if (scause & TRAP_Interrupt) {
        switch (trap_code) {
            case TRAP_Interrupt_SupervisorTimer:
                set_next_trigger();
                schedule(0);
                break;
            default:
                printk("[kernel] Unsupported interrupt: ");
                printk(itoa(scause, buf));
                panic("\n");
        }
    } else {
        switch (trap_code) {
            case TRAP_Exception_UserEnvCall:
                proc->trapframe->sepc += 4;
                uint64_t ret = syscall(proc->trapframe->x[17], proc->trapframe->x[10], proc->trapframe->x[11], proc->trapframe->x[12]);
                proc->trapframe->x[10] = ret;
                break;
            case TRAP_Exception_LoadPageFault:
                printk("[kernel] Load Page fault in process.\n");

                terminate(cpus[cpuid].running);
                unload_process(cpus[cpuid].running);
                schedule(cpuid);
                break;
            case TRAP_Exception_StorePageFault:
                printk("[kernel] Store Page fault in application.\n");

                terminate(cpus[cpuid].running);
                unload_process(cpus[cpuid].running);
                schedule(cpuid);
                break;
            case TRAP_Exception_InstructionPageFault:
                printk("[kernel] Instruction Page fault in application.\n");
                
                terminate(cpus[cpuid].running);
                unload_process(cpus[cpuid].running);
                schedule(cpuid);
                break;
            case TRAP_Exception_IllegalInstruction:
                printk("[kernel] Illegal instruction: ");
                printk(itoa(stval, buf));
                printk("\n");
                
                terminate(cpus[cpuid].running);
                unload_process(cpus[cpuid].running);
                schedule(cpuid);
                break;
            default:
                printk("[kernel] Unsupported exception: ");
                printk(itoa(scause, buf));
                printk("\n");
                
                terminate(cpus[cpuid].running);
                unload_process(cpus[cpuid].running);
                schedule(cpuid);
        }
    }
    trap_return(cpuid);
}

void app_init_context(TrapContext *ptr, uint64_t entry, uint64_t user_sp, uint64_t kernel_sp) {
    uint64_t sstatus, satp;
    asm volatile(
        "csrr %0, sstatus\n\t"
        "csrr %1, satp"
        : "=r"(sstatus), "=r"(satp)
    );
    sstatus &= ~SSTATUS_SPP;
    memset(ptr, 0, sizeof(TrapContext));
    set_sp(ptr, user_sp);
    ptr->sstatus = sstatus;
    ptr->sepc = entry;
    ptr->kernel_satp = satp;
    ptr->kernel_sp = kernel_sp;
    ptr->trap_handler = (uint64_t)trap_handler;
}