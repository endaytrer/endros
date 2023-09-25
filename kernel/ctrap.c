#include "trap.h"
#include "printk.h"
#include "syscall.h"
#include "batch.h"

#define SSTATUS_SPP 0x00000100
extern AppManager app_manager;
void set_sp(TrapContext *self, uint64_t sp) {
    self->x[2] = sp;
}

void trap_handler() {
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
        case TRAP_USER_ENV_CALL:
            cx->sepc += 4;
            cx->x[10] = syscall(cx->x[17], cx->x[10], cx->x[11], cx->x[12]);
            break;
        case TRAP_STORE_FAULT:
        case TRAP_STORE_PAGE_FAULT:
            printk("Page fault in application.\n");
            run_next_app();
            break;
        case TRAP_ILLEGAL_INSTRUCTION:
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