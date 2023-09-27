#include "sbi.h"
#include "printk.h"
#include "trap.h"
#include "pagetable.h"
#include "timer.h"
#include "process.h"
extern void sbss();
extern void ebss();


void clear_bss(void) {
    for (uint8_t *ptr = (uint8_t *)sbss; ptr != (uint8_t *)ebss; ptr++) {
        *ptr = 0;
    }
}


void init(void) {
    asm volatile(
        "csrw stvec, %0"
        :: "r" (TRAMPOLINE)
    );
    clear_bss();
    init_pagetable();
    init_scheduler();
}
