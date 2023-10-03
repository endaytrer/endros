//
// Created by endaytrer on 2023/10/2.
//

#ifndef _K_VIRTIO_GPU_H
#define _K_VIRTIO_GPU_H
#include "virtio.h"
#include "../file.h"

/// virgl 3D mode is supported.
#define VIRTIO_GPU_FEATURE_VIRGL                 (1 << 0)
/// EDID is supported.
#define VIRTIO_GPU_FEATURE_EDID                  (1 << 1)

#define RESOLUTION_WIDTH(res) ((res) >> 32)
#define RESOLUTION_HEIGHT(res) ((res) & 0xffffffff)

#define VIRTIO_GPU_SUPPORTED_FEATURES VIRTIO_FEATURE_RING_EVENT_IDX


#define SCANOUT_ID 0
#define RESOURCE_ID_FB 0xbabe
#define RESOURCE_ID_CURSOR 0xdade
typedef struct {
    /// READONLY Signals pending events to the driver.
    const u32 events_read;

    /// WRITEONLY Signals pending events to the driver.
    u32 events_clear;


    /// READWRITE
    /// Specifies the maximum number of scanouts supported by the device.
    ///
    /// Minimum value is 1, maximum value is 16.
    u32 num_scanouts;

} GPUConfig;



struct dma_t {
    pfn_t pfn;
    vpn_t vpn;
    u64 pages;
};

typedef struct {
    VirtIOHeader *hdr;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    struct dma_t frame_buffer_dma;
    struct dma_t cursor_buffer_dma;
    VirtIOQueue control_queue;
    VirtIOQueue cursor_queue;
    struct dma_t queue_buf_dma;
    u8 *queue_buf_send;
    u8 *queue_buf_recv;
} VirtIOGPU;

typedef struct {
    enum {
        GET_DISPLAY_INFO = 0x100,
        RESOURCE_CREATE_2D = 0x101,
        RESOURCE_UNREF = 0x102,
        SET_SCANOUT = 0x103,
        RESOURCE_FLUSH = 0x104,
        TRANSFER_TO_HOST_2D = 0x105,
        RESOURCE_ATTACH_BACKING = 0x106,
        RESOURCE_DETACH_BACKING = 0x107,
        GET_CAPSET_INFO = 0x108,
        GET_CAPSET = 0x109,
        GET_EDID = 0x10a,
        UPDATE_CURSOR = 0x300,
        MOVE_CURSOR = 0x301,
        OK_NODATA = 0x1100,
        OK_DISPLAY_INFO = 0x1101,
        OK_CAPSET_INFO = 0x1102,
        OK_CAPSET = 0x1103,
        OK_EDID = 0x1104,
        ERR_UNSPEC = 0x1200,
        ERR_OUT_OF_MEMORY = 0x1201,
        ERR_INVALID_SCANOUT_ID = 0x1202,
    } hdr_type;
    u32 flags;
    u64 fence_id;
    u32 ctx_id;
    u32 _padding;
} ControlHdr;
/// requests
typedef struct {
    ControlHdr hdr;
    u32 resource_id;
    enum {
        B8G8R8A8UNORM = 1
    } format;
    u32 width;
    u32 height;
} ReqResourceCreate2D;

typedef struct {
    ControlHdr hdr;
    u32 resource_id;
    /// always 1
    u32 nr_entries;
    u64 addr;
    u32 length;
    u32 _padding;
} ReqResourceAttachBacking;

typedef struct {
    ControlHdr hdr;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    u32 scanout_id;
    u32 resource_id;
} ReqSetScanout;

typedef struct {
    ControlHdr hdr;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    u64 offset;
    u32 resource_id;
    u32 _padding;
} ReqTransferToHost2D;

typedef struct {
    ControlHdr hdr;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    u32 resource_id;
    u32 _padding;
} ReqResourceFlush;
/// responses
typedef struct {
    ControlHdr hdr;
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    u32 enabled;
    u32 flags;
} RespDisplayInfo;

void init_virtio_gpu(VirtIOGPU *gpu, VirtIOHeader *header);
RespDisplayInfo *get_display_info(VirtIOGPU *gpu);
i64 setup_framebuffer(VirtIOGPU *gpu);
i64 flush(VirtIOGPU *gpu);

void wrap_virtio_gpu_file(File *out, VirtIOGPU *in);
#endif //_K_VIRTIO_GPU_H
