#include "printk.h"
#include "pagetable.h"
#include "process.h"
#include "fdt.h"

#include "filesystem.h"
#include <string.h>

extern void sbss();
extern void ebss();


void clear_bss(void) {
    for (u8 *ptr = (u8 *)sbss; ptr != (u8 *)ebss; ptr++) {
        *ptr = 0;
    }
}


void init(u64 hart_id, struct fdt_header *dtb) {
    asm volatile(
        "csrw stvec, %0"
        :: "r" (TRAMPOLINE)
    );
    // virtio device
    clear_bss();
    // since we need dtb info for gpu and blk drives, parse before initializing page table.
    if (parse_dtb(dtb) < 0) {
        panic("[kernel] Failed to parse dtb\n");
    }
    init_pagetable();
    init_filesystem();
    init_scheduler();
}
