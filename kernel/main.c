#include "sbi.h"
#include "printk.h"
#include "batch.h"
#include "trap.h"
#include "pagetable.h"

extern void sbss();
extern void ebss();


void clear_bss(void) {
    for (uint8_t *ptr = (uint8_t *)sbss; ptr != (uint8_t *)ebss; ptr++) {
        *ptr = 0;
    }
}


void init(void) {
    clear_bss();
    init_pagetable();
    init_app_manager();
}

int main(void) {
    init();
    // load apps

    run_next_app();

    printk("\nDone!\n");
    return 0;
}

