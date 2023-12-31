#ifndef _K_VIRTIO_BLK_H
#define _K_VIRTIO_BLK_H
#include "virtio.h"
#include "../block_device.h"

/// features


/// Device supports request barriers. (legacy)
#define VIRTIO_BLK_FEATURE_BARRIER        (1 << 0)
/// Maximum size of any single segment is in `size_max`.
#define VIRTIO_BLK_FEATURE_SIZE_MAX       (1 << 1)
/// Maximum number of segments in a request is in `seg_max`.
#define VIRTIO_BLK_FEATURE_SEG_MAX        (1 << 2)
/// Disk-style geometry specified in geometry.
#define VIRTIO_BLK_FEATURE_GEOMETRY       (1 << 4)
/// Device is read-only.
#define VIRTIO_BLK_FEATURE_RO             (1 << 5)
/// Block size of disk is in `blk_size`.
#define VIRTIO_BLK_FEATURE_BLK_SIZE       (1 << 6)
/// Device supports scsi packet commands. (legacy)
#define VIRTIO_BLK_FEATURE_SCSI           (1 << 7)
/// Cache flush command support.
#define VIRTIO_BLK_FEATURE_FLUSH          (1 << 9)
/// Device exports information on optimal I/O alignment.
#define VIRTIO_BLK_FEATURE_TOPOLOGY       (1 << 10)
/// Device can toggle its cache between writeback and writethrough modes.
#define VIRTIO_BLK_FEATURE_CONFIG_WCE     (1 << 11)
/// Device supports multiqueue.
#define VIRTIO_BLK_FEATURE_MQ             (1 << 12)
/// Device can support discard command, maximum discard sectors size in
/// `max_discard_sectors` and maximum discard segment number in
/// `max_discard_seg`.
#define VIRTIO_BLK_FEATURE_DISCARD        (1 << 13)
/// Device can support write zeroes command, maximum write zeroes sectors
/// size in `max_write_zeroes_sectors` and maximum write zeroes segment
/// number in `max_write_zeroes_seg`.
#define VIRTIO_BLK_FEATURE_WRITE_ZEROES   (1 << 14)
/// Device supports providing storage lifetime information.
#define VIRTIO_BLK_FEATURE_LIFETIME       (1 << 15)
/// Device can support the secure erase command.
#define VIRTIO_BLK_FEATURE_SECURE_ERASE   (1 << 16)

#define VIRTIO_BLK_SUPPORTED_FEATURES ( \
    VIRTIO_BLK_FEATURE_RO | \
    VIRTIO_BLK_FEATURE_FLUSH | \
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

void wrap_virtio_blk_device(BufferedBlockDevice *out, VirtIOBlk *in);
#endif