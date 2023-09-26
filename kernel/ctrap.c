#include "trap.h"
#include "printk.h"
#include "syscall.h"
#include "batch.h"

#define SSTATUS_SPP 0x00000100
extern AppManager app_manager;
void set_sp(TrapContext *self, uint64_t sp) {
    self->x[2] = sp;
}

void trap_return(void) {
    asm volatile (
        "csrw stvec, %0"
        :: "r" (TRAMPOLINE)
    );

    extern void strampoline();
    extern void __restore();
    uint64_t token = ((uint64_t)1 << 63) | app_manager.current_task.ptbase_pfn;
    void (*restore)(void *trap_context, uint64_t user_token) = (void (*)(void *, uint64_t))(TRAMPOLINE + (uint64_t)__restore - (uint64_t)strampoline);
    register uint64_t a0 asm("a0") = (uint64_t)TRAPFRAME;
    register uint64_t a1 asm("a1") = token;
    asm volatile (
        "fence.i\n\t"
        "jr %0"
        :: "r"(restore), "r" (a0), "r" (a1)
    );
}
void trap_handler(void) {
    TrapContext *cx = PAGE_2_ADDR(app_manager.current_task.trap_vpn);
    uint64_t scause;
    uint64_t stval;

    asm volatile (
        "csrr %0, scause\n\t"
        "csrr %1, stval\n\t"
        : "=r" (scause), "=r" (stval)
    );
    char buf[16];
    switch (scause) {
        case TRAP_UserEnvCall:
            cx->sepc += 4;
            cx->x[10] = syscall(cx->x[17], cx->x[10], cx->x[11], cx->x[12]);
            break;
        case TRAP_LoadPageFault:
            printk("Load Page fault in application.\n");
            run_next_app();
            break;
        case TRAP_StorePageFault:
            printk("Store Page fault in application.\n");
            run_next_app();
            break;

        case TRAP_InstructionPageFault:
            printk("Instruction Page fault in application.\n");
            run_next_app();
            break;
        case TRAP_IllegalInstruction:
            printk("Illegal instruction: ");
            printk(itoa(stval, buf));
            printk("\n");
            run_next_app();
            break;
        default:
            printk("Unsupported trap: ");
            printk(itoa(scause, buf));
            printk("\n");
            run_next_app();
    }
    trap_return();
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