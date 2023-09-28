#include <string.h>
#include "virtio_blk.h"
#include "../printk.h"


void init_virtio_blk(VirtIOBlk *blk, VirtIOHeader *header) {
    u32 negotiated_features = header_init(header, VIRTIO_BLK_SUPPORTED_FEATURES);
    // in MMIO mod, the config space is just after the header;
    BlkConfig *config_space = (BlkConfig *)(header + 1);

    u64 capacity = config_space->capacity_low | ((u64)config_space->capacity_high << 32);

    printk("Find a block device of size ");
    char buf[16];
    printk(itoa(capacity / 2, buf));
    printk(" kiB\n");    
    // init queue
    init_queue(&blk->queue, header, 0, (negotiated_features & VIRTIO_BLK_FEATURE_RING_INDIRECT_DESC) != 0, (negotiated_features & VIRTIO_BLK_FEATURE_RING_INDIRECT_DESC) != 0);
    blk->negotiated_features = negotiated_features;
    blk->capacity = capacity;
}