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
    printk(itoa(capacity / 2, buf, 10));
    printk(" kiB\n");    
    // init queue
    init_queue(&blk->queue, header, 0, (negotiated_features & VIRTIO_BLK_FEATURE_RING_INDIRECT_DESC) != 0, (negotiated_features & VIRTIO_BLK_FEATURE_RING_INDIRECT_DESC) != 0);
    blk->hdr = header;
    blk->negotiated_features = negotiated_features;
    blk->capacity = capacity;
}


i64 virtio_blk_write_block(VirtIOBlk *blk, u64 sector, vpn_t buf_vpn, pfn_t buf_pfn) {
    vpn_t req_vpn;
    pfn_t req_pfn = uptalloc(&req_vpn);
    VirtIOBlkReq *req = (VirtIOBlkReq *)PAGE_2_ADDR(req_vpn);
    *req = (VirtIOBlkReq) {
        .req_type = Out,
        .reserved = 0,
        .sector = sector
    };

    vpn_t resp_vpn;
    pfn_t resp_pfn = uptalloc(&resp_vpn);
    VirtIOBlkResp *resp = (VirtIOBlkResp *)PAGE_2_ADDR(resp_vpn);
    resp->status = VIRTIO_BLK_RESP_STATUS_NOT_READY;
    pfn_t inputs[] = {req_pfn, buf_pfn};
    u32 input_lengths[] = {sizeof(VirtIOBlkReq), PAGESIZE};
    pfn_t outputs[] = {resp_pfn};
    u32 output_lengths[] = {sizeof(VirtIOBlkResp)};
    queue_add_notify_pop(blk, inputs, input_lengths, 2, outputs, output_lengths, 1);

    u8 status = resp->status;
    uptfree(req_pfn, req_vpn);
    uptfree(resp_pfn, resp_vpn);
    return status;
}
i64 virtio_blk_read_block(VirtIOBlk *blk, u64 sector, vpn_t buf_vpn, pfn_t buf_pfn) {
    vpn_t req_vpn;
    pfn_t req_pfn = uptalloc(&req_vpn);
    VirtIOBlkReq *req = (VirtIOBlkReq *)PAGE_2_ADDR(req_vpn);
    *req = (VirtIOBlkReq) {
        .req_type = In,
        .reserved = 0,
        .sector = sector
    };

    vpn_t resp_vpn;
    pfn_t resp_pfn = uptalloc(&resp_vpn);
    VirtIOBlkResp *resp = (VirtIOBlkResp *)PAGE_2_ADDR(resp_vpn);
    resp->status = VIRTIO_BLK_RESP_STATUS_NOT_READY;
    pfn_t inputs[] = {req_pfn};
    u32 input_lengths[] = {sizeof(VirtIOBlkReq)};
    pfn_t outputs[] = {buf_pfn, resp_pfn};
    u32 output_lengths[] = {PAGESIZE, sizeof(VirtIOBlkResp)};

    queue_add_notify_pop(blk, inputs, input_lengths, 1, outputs, output_lengths, 2);

    u8 status = resp->status;
    uptfree(req_pfn, req_vpn);
    uptfree(resp_pfn, resp_vpn);
    if (status == VIRTIO_BLK_RESP_STATUS_IO_ERR)
        printk("[kernel.drivers.virtio] io err\n");
    else if (status == VIRTIO_BLK_RESP_STATUS_NOT_READY)
        printk("[kernel.drivers.virtio] not ready\n");
    else if (status == VIRTIO_BLK_RESP_STATUS_UNSUPPORTED)
        printk("[kernel.drivers.virtio] unsupported\n");
    return status;
}

i64 queue_add_notify_pop(VirtIOBlk *blk, pfn_t inputs[], u32 input_lengths[], u8 num_inputs, pfn_t outputs[], u32 output_lengths[], u8 num_outputs) {
    i32 token = add_queue(&blk->queue, inputs, input_lengths, num_inputs, outputs, output_lengths, num_outputs);
    if (token < 0) {
        printk("[kernel.drivers.virtio] add queue failed\n");
        return -1;
    }
    // write notify register to notify device
//    if (should_notify(&blk->queue))
    blk->hdr->queue_notify = blk->queue.queue_idx;
    while (1) {
        __sync_synchronize();
        if (blk->queue.last_used_idx != blk->queue.used->idx)
            break;
    }
    u16 last_used_slot = blk->queue.last_used_idx & (VIRTIO_NUM_DESC - 1);
    u16 index = (u16)blk->queue.used->ring[last_used_slot].id;
    u32 len = blk->queue.used->ring[last_used_slot].len;

    if (index != token) {
        printk("[kernel.drivers.virtio] used token error\n");
        return -1;
    }
    recycle_descriptors(&blk->queue, index);
    blk->queue.last_used_idx++;
    return len;
}

void wrap_virtio_blk_device(BufferedBlockDevice *out, VirtIOBlk *in) {
    out->cache_size = 0;
    out->buffer_head = NULL;
    out->super = in;
    out->size = in->capacity * SECTOR_SIZE;
    out->read_block = (i64 (*)(void *, u64, vpn_t, pfn_t)) virtio_blk_read_block;
    out->write_block = (i64 (*)(void *, u64, vpn_t, pfn_t)) virtio_blk_write_block;

}