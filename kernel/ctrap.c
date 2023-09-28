#include "trap.h"
#include "printk.h"
#include "syscall.h"
#include "process.h"
#include "mem.h"
#include "timer.h"
#include <string.h>

#define SSTATUS_SPP 0x00000100

void set_sp(TrapContext *self, u64 sp) {
    self->x[2] = sp;
}

void trap_return(int cpuid) {
    asm volatile (
        "csrw stvec, %0"
        :: "r" (TRAMPOLINE)
    );

    extern void strampoline();
    extern void __restore();
    u64 token = ((u64)1 << 63) | cpus[cpuid].running->ptbase_pfn;
    void (*restore)(void *trap_context, u64 user_token) = (void (*)(void *, u64))(TRAMPOLINE + (u64)__restore - (u64)strampoline);
    asm volatile (
        "fence.i"
    );
    restore(TRAPFRAME, token);
}
void trap_handler(void) {
    int cpuid = 0;

    cpus[cpuid].running->status = READY;
    PCB *proc = cpus[cpuid].running;
    u64 scause;
    u64 stval;

    asm volatile (
        "csrr %0, scause\n\t"
        "csrr %1, stval\n\t"
        : "=r" (scause), "=r" (stval)
    );
    char buf[16];
    u64 trap_code = TRAP_CODE(scause);
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
                u64 ret = syscall(proc->trapframe->x[17], proc->trapframe->x[10], proc->trapframe->x[11], proc->trapframe->x[12]);
                proc->trapframe->x[10] = ret;
                break;
            case TRAP_Exception_LoadPageFault:
                printk("[kernel] Load Page fault in process.\n");
                sys_exit(-1);
                break;
            case TRAP_Exception_StorePageFault:
                printk("[kernel] Store Page fault in application.\n");
                sys_exit(-2);
                break;
            case TRAP_Exception_InstructionPageFault:
                printk("[kernel] Instruction Page fault in application.\n");
                sys_exit(-3);
                break;
            case TRAP_Exception_IllegalInstruction:
                printk("[kernel] Illegal instruction: ");
                printk(itoa(stval, buf));
                printk("\n");
                
                sys_exit(-4);
                break;
            default:
                printk("[kernel] Unsupported exception: ");
                printk(itoa(scause, buf));
                printk("\n");
                
                sys_exit(-5);
        }
    }
    trap_return(cpuid);
}

void app_init_context(TrapContext *ptr, u64 entry, u64 user_sp, u64 kernel_sp) {
    u64 sstatus, satp;
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
    ptr->trap_handler = (u64)trap_handler;
}