#include "trap.h"
#include "printk.h"
#include "syscall.h"
#include "batch.h"

#define SSTATUS_SPP 0x00000100

void set_sp(TrapContext *self, uint64_t sp) {
    self->x[2] = sp;
}

void c_init_context(TrapContext *ptr, uint64_t entry, uint64_t sp, uint64_t sstatus) {

    sstatus &= ~SSTATUS_SPP;
    memset(ptr, 0, sizeof(TrapContext));
    set_sp(ptr, sp);
    ptr->sstatus = sstatus;
    ptr->sepc = entry;
}

TrapContext *trap_handler(TrapContext *cx, uint64_t scause, uint64_t stval) {

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
            run_next_app();

    }
    return cx;
}