#include "sbi.h"
#include "printk.h"
#include "trap.h"
#include "pagetable.h"
#include "timer.h"
#include "process.h"
#include "fdt.h"
#include "drivers/virtio_blk.h"
#include "block_device.h"
#include "file.h"
#include "filesystem.h"
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

static VirtIOBlk virtio_blk_0;
static BufferedBlockDevice root_block_buffered_dev;
static File root_block_device;
void init(u64 hart_id, struct fdt_header *dtb) {
    asm volatile(
        "csrw stvec, %0"
        :: "r" (TRAMPOLINE)
    );
    // virtio device
    clear_bss();
    init_pagetable();
    // wrap to virtio device
    init_virtio_blk(&virtio_blk_0, VIRTIO0);
    // wrap to buffered_block_device
    init_buffered_block_device(&root_block_buffered_dev, &virtio_blk_0, 
        (i64 (*)(void *, u64, vpn_t, pfn_t))virtio_blk_read_block,
        (i64 (*)(void *, u64, vpn_t, pfn_t))virtio_blk_write_block);
    // wrap to file
    root_block_device = (File) {
        .super = &root_block_buffered_dev,
        .permission = 06,
        .size = virtio_blk_0.capacity * SECTOR_SIZE,
        .read = (i64 (*)(void *, u64, void *, u64))read_bytes,
        .write = (i64 (*)(void *, u64, const void *, u64))write_bytes
    };


    init_filesystem(&rootfs, &root_block_device);
    ls(&rootfs.root);
    init_scheduler();
}
