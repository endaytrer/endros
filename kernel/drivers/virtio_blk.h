#ifndef _K_VIRTIO_BLK_H
#define _K_VIRTIO_BLK_H
#include "virtio.h"
#include "../block_device.h"


#define VIRTIO_BLK_SUPPORTED_FEATURES ( \
    VIRTIO_FEATURE_RO | \
    VIRTIO_FEATURE_FLUSH | \
    VIRTIO_FEATURE_RING_INDIRECT_DESC | \
    VIRTIO_FEATURE_RING_EVENT_IDX )

/// Block queue
#define VIRTIO_BLK_QUEUE 0
typedef volatile struct {
    /// Number of 512 Bytes sectors
    u32 capacity_low;
    u32 capacity_high;
    u32 size_max;
    u32 seg_max;
    u16 cylinders;
    u8 heads;
    u8 sectors;
    u32 blk_size;
    u8 physical_block_exp;
    u8 alignment_offset;
    u16 min_io_size;
    u32 opt_io_size;
    // ... ignored
} BlkConfig;

typedef struct {
    VirtIOHeader *hdr;
    VirtIOQueue queue;
    /// @brief  Capacity in sectors(512 bytes)
    u64 capacity;
    u32 negotiated_features;
} VirtIOBlk;

typedef struct {
    enum {
        In,
        Out
    } req_type;
    u32 reserved;
    u64 sector;
} VirtIOBlkReq;

#define VIRTIO_BLK_RESP_STATUS_OK 0
#define VIRTIO_BLK_RESP_STATUS_IO_ERR 1
#define VIRTIO_BLK_RESP_STATUS_UNSUPPORTED 2
#define VIRTIO_BLK_RESP_STATUS_NOT_READY 3

typedef struct {
    u8 status;
} VirtIOBlkResp;

void init_virtio_blk(VirtIOBlk *blk, VirtIOHeader *header);

i64 virtio_blk_write_block(VirtIOBlk *blk, u64 block_id, vpn_t buf_vpn, pfn_t buf_pfn);
i64 virtio_blk_read_block(VirtIOBlk *blk, u64 block_id, vpn_t buf_vpn, pfn_t buf_pfn);
i64 queue_add_notify_pop(VirtIOBlk *blk, pfn_t inputs[], u32 input_lengths[], u8 num_inputs, pfn_t outputs[], u32 output_lengths[], u8 num_outputs);

void wrap_virtio_blk_device(BufferedBlockDevice *out, VirtIOBlk *in);
#endif