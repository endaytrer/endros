#include "printk.h"
#include "pagetable.h"
#include "process.h"
#include "fdt.h"
#include "machine_spec.h"
#include "filesystem.h"
#include "gpu.h"
#include "drivers/virtio_gpu.h"
#include <string.h>

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
    printk("\nEndrOS v0.0.0\n\nCopyright (c) 2023-2024 Zhenrong Gu\n================================================================================\nThis software is distributed under the GNU General Public License v3. Anyone is\nfree to use, modify, distribute and redistribute under certain conditions.\nPlease refer to https://www.gnu.org/licenses/gpl-3.0.html for license details.\n================================================================================\n\n");
    char buf[64];
    buf[1] = '\0';
    printk("booting with hart ");
    printk(itoa((i64) hart_id, buf, 10));
    printk(", dtb at ");
    printk(itoa((i64) dtb, buf + 2, 16));
    printk("\n");
    // since we need dtb info for gpu and blk drives, parse before initializing page table.
    if (parse_dtb(dtb) < 0) {
        panic("[kernel] Failed to parse dtb\n");
    }
    printk("[kernel] Done parsing dtb. Available memory 0x80000000-");
    printk(itoa(PAGE_2_ADDR(max_pfn), buf, 16));
    printk("\n");
    init_pagetable();
    printk("[kernel] Done initializing pagetable.\n");                                        
    init_virtio();
    
    init_filesystem(blk_header);
    init_gpu(gpu_header);

    init_scheduler();
}
