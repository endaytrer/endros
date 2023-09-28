#include <string.h>
#include "virtio_blk.h"
#include "../printk.h"
#include "../pagetable.h"


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
    blk->hdr = header;
    blk->negotiated_features = negotiated_features;
    blk->capacity = capacity;
}


i64 write_block(VirtIOBlk *blk, u64 block_id, vpn_t buf_vpn, pfn_t buf_pfn) {
    vpn_t req_vpn;
    pfn_t req_pfn = uptalloc(&req_vpn);
    VirtIOBlkReq *req = PAGE_2_ADDR(req_vpn);
    *req = (VirtIOBlkReq) {
        .req_type = Out,
        .reserved = 0,
        .sector = block_id
    };

    vpn_t resp_vpn;
    pfn_t resp_pfn = uptalloc(&resp_vpn);
    VirtIOBlkResp *resp = PAGE_2_ADDR(resp_vpn);
    resp->status = VIRTIO_BLK_RESP_STATUS_NOT_READY;
    pfn_t inputs[] = {req_pfn, buf_pfn};
    u32 input_lengths[] = {sizeof(VirtIOBlkReq), PAGESIZE};
    pfn_t outputs[] = {resp_pfn};
    u32 output_lengths[] = {sizeof(VirtIOBlkResp)};
    add_queue(&blk->queue, inputs, input_lengths, 2, outputs, output_lengths, 1);

    // write notify register to notify device
    // TODO: should_notify

    blk->hdr->queue_notify = blk->queue.queue_idx;
    while (1) {
        __sync_synchronize();
        if (blk->queue.last_used_idx != blk->queue.used->idx)
            break;
    }


    uptfree(req_pfn, req_vpn);
    uptfree(resp_pfn, resp_vpn);
    return 0;
}
i64 read_block(VirtIOBlk *blk, u64 block_id, vpn_t buf_vpn, pfn_t buf_pfn) {
    vpn_t req_vpn;
    pfn_t req_pfn = uptalloc(&req_vpn);
    VirtIOBlkReq *req = PAGE_2_ADDR(req_vpn);
    *req = (VirtIOBlkReq) {
        .req_type = In,
        .reserved = 0,
        .sector = block_id
    };

    vpn_t resp_vpn;
    pfn_t resp_pfn = uptalloc(&resp_vpn);
    VirtIOBlkResp *resp = PAGE_2_ADDR(resp_vpn);
    resp->status = VIRTIO_BLK_RESP_STATUS_NOT_READY;
    pfn_t inputs[] = {req_pfn};
    u32 input_lengths[] = {sizeof(VirtIOBlkReq)};
    pfn_t outputs[] = {buf_pfn, resp_pfn};
    u32 output_lengths[] = {PAGESIZE, sizeof(VirtIOBlkResp)};
    add_queue(&blk->queue, inputs, input_lengths, 1, outputs, output_lengths, 2);

    // write notify register to notify device
    // TODO: should_notify

    blk->hdr->queue_notify = blk->queue.queue_idx;
    while (1) {
        __sync_synchronize();
        if (blk->queue.last_used_idx != blk->queue.used->idx)
            break;
        // interrupt method
        u32 interrupt = blk->hdr->interrupt_status;
        if (interrupt) {
            blk->hdr->interrupt_ack = interrupt;
            break;
        }
    }
    

    uptfree(req_pfn, req_vpn);
    uptfree(resp_pfn, resp_vpn);
    return 0;
}