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
    init_queue(&blk->queue, header, 0, (negotiated_features & VIRTIO_FEATURE_RING_INDIRECT_DESC) != 0, (negotiated_features & VIRTIO_FEATURE_RING_INDIRECT_DESC) != 0);
    finish_init(header);
    blk->hdr = header;
    blk->negotiated_features = negotiated_features;
    blk->capacity = capacity;
}


i64 virtio_blk_write_block(VirtIOBlk *blk, u64 sector, vpn_t buf_vpn, pfn_t buf_pfn) {
    vpn_t req_vpn;
    pfn_t req_pfn = dmalloc(&req_vpn, 1, false);
    VirtIOBlkReq *req = (VirtIOBlkReq *)PAGE_2_ADDR(req_vpn);
    *req = (VirtIOBlkReq) {
        .req_type = Out,
        .reserved = 0,
        .sector = sector
    };

    vpn_t resp_vpn;
    pfn_t resp_pfn = dmalloc(&resp_vpn, 1, false);
    VirtIOBlkResp *resp = (VirtIOBlkResp *)PAGE_2_ADDR(resp_vpn);
    resp->status = VIRTIO_BLK_RESP_STATUS_NOT_READY;
    pfn_t inputs[] = {req_pfn, buf_pfn};
    u32 input_lengths[] = {sizeof(VirtIOBlkReq), PAGESIZE};
    pfn_t outputs[] = {resp_pfn};
    u32 output_lengths[] = {sizeof(VirtIOBlkResp)};
    queue_add_notify_pop(&blk->queue, blk->hdr, inputs, input_lengths, 2, outputs, output_lengths, 1);

    u8 status = resp->status;
    dmafree(req_pfn, req_vpn, 1);
    dmafree(resp_pfn, resp_vpn, 1);
    return status;
}
i64 virtio_blk_read_block(VirtIOBlk *blk, u64 sector, vpn_t buf_vpn, pfn_t buf_pfn) {
    vpn_t req_vpn;
    pfn_t req_pfn = dmalloc(&req_vpn, 1, false);
    VirtIOBlkReq *req = (VirtIOBlkReq *)PAGE_2_ADDR(req_vpn);
    *req = (VirtIOBlkReq) {
        .req_type = In,
        .reserved = 0,
        .sector = sector
    };

    vpn_t resp_vpn;
    pfn_t resp_pfn = dmalloc(&resp_vpn, 1, false);
    VirtIOBlkResp *resp = (VirtIOBlkResp *)PAGE_2_ADDR(resp_vpn);
    resp->status = VIRTIO_BLK_RESP_STATUS_NOT_READY;
    pfn_t inputs[] = {req_pfn};
    u32 input_lengths[] = {sizeof(VirtIOBlkReq)};
    pfn_t outputs[] = {buf_pfn, resp_pfn};
    u32 output_lengths[] = {PAGESIZE, sizeof(VirtIOBlkResp)};

    queue_add_notify_pop(&blk->queue, blk->hdr, inputs, input_lengths, 1, outputs, output_lengths, 2);

    u8 status = resp->status;
    dmafree(req_pfn, req_vpn, 1);
    dmafree(resp_pfn, resp_vpn, 1);
    if (status == VIRTIO_BLK_RESP_STATUS_IO_ERR)
        printk("[kernel.drivers.virtio] io err\n");
    else if (status == VIRTIO_BLK_RESP_STATUS_NOT_READY)
        printk("[kernel.drivers.virtio] not ready\n");
    else if (status == VIRTIO_BLK_RESP_STATUS_UNSUPPORTED)
        printk("[kernel.drivers.virtio] unsupported\n");
    return status;
}

void wrap_virtio_blk_device(BufferedBlockDevice *out, VirtIOBlk *in) {
    out->cache_size = 0;
    out->buffer_head = NULL;
    out->super = in;
    out->size = in->capacity * SECTOR_SIZE;
    out->read_block = (i64 (*)(void *, u64, vpn_t, pfn_t)) virtio_blk_read_block;
    out->write_block = (i64 (*)(void *, u64, vpn_t, pfn_t)) virtio_blk_write_block;

}