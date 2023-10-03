#include "printk.h"
#include "pagetable.h"
#include "process.h"
#include "fdt.h"
#include "machine_spec.h"
#include "filesystem.h"
#include "gpu.h"
#include "drivers/virtio_gpu.h"


extern void sbss();
extern void ebss();


void clear_bss(void) {
    for (u8 *ptr = (u8 *)sbss; ptr != (u8 *)ebss; ptr++) {
        *ptr = 0;
    }
}

VirtIOHeader *blk_header;
VirtIOHeader *gpu_header;

void init_virtio(void) {
    blk_header = (VirtIOHeader *)(~((u64)0));
    gpu_header = (VirtIOHeader *)(~((u64)0));

    for (int i = 0; i < num_virtio_mmio; i++) {
        if (virtio_mmio_headers[i]->device_id == VIRTIO_DEVICE_BLK &&
                (u64)virtio_mmio_headers[i] < (u64)blk_header) {
            blk_header = virtio_mmio_headers[i];
        }
        if (virtio_mmio_headers[i]->device_id == VIRTIO_DEVICE_GPU &&
            (u64)virtio_mmio_headers[i] < (u64)gpu_header) {
            gpu_header = virtio_mmio_headers[i];
        }
    }

    if (blk_header == (VirtIOHeader *)(~((u64)0))) {
        panic("[kernel] Cannot find loadable disk\n");
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
    init_virtio();
    
    init_filesystem(blk_header);
    init_gpu(gpu_header);

    init_scheduler();
}
