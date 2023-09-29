#include "sbi.h"
#include "printk.h"
#include "trap.h"
#include "pagetable.h"
#include "timer.h"
#include "process.h"
#include "fdt.h"
#include "drivers/virtio_blk.h"
#include <string.h>

extern void sbss();
extern void ebss();


void clear_bss(void) {
    for (u8 *ptr = (u8 *)sbss; ptr != (u8 *)ebss; ptr++) {
        *ptr = 0;
    }
}

void parse_dtb(struct fdt_header *dtb) {
    // dtb discovery
    char buf[16];
    char *name_block = (char *)((u64)dtb + ntohl(dtb->off_dt_strings));
    printk(name_block + 0x0);
    printk("\n");
    fdt32_t *fdt_structs = (fdt32_t *)((u64)dtb + ntohl(dtb->off_dt_struct));
    for (u32 i = 0; i < ntohl(dtb->size_dt_struct) / sizeof(fdt32_t); i++) {
        printk(itoa(ntohl(fdt_structs[i]), buf));
        printk(" ");
    }
    printk("\n");
}

void init(u64 hart_id, struct fdt_header *dtb) {
    asm volatile(
        "csrw stvec, %0"
        :: "r" (TRAMPOLINE)
    );
    // virtio device
    clear_bss();
    init_pagetable();
    VirtIOBlk *blk = kalloc(sizeof(VirtIOBlk));
    init_virtio_blk(blk, VIRTIO0);
    vpn_t super_block_vpn;
    pfn_t super_block_pfn = uptalloc(&super_block_vpn);
    read_block(blk, 0, super_block_vpn, super_block_pfn);
    read_block(blk, 8, super_block_vpn, super_block_pfn);
    read_block(blk, 16, super_block_vpn, super_block_pfn);
    read_block(blk, 24, super_block_vpn, super_block_pfn);
    read_block(blk, 32, super_block_vpn, super_block_pfn);
    read_block(blk, 40, super_block_vpn, super_block_pfn);
    read_block(blk, 48, super_block_vpn, super_block_pfn);
    init_scheduler();
}
