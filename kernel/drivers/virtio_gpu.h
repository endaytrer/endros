//
// Created by endaytrer on 2023/10/2.
//

#ifndef _K_VIRTIO_GPU_H
#define _K_VIRTIO_GPU_H
#include "virtio.h"

#define VIRTIO_GPU_SUPPORTED_FEATURES VIRTIO_FEATURE_RING_EVENT_IDX
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

void init_virtio_gpu(VirtIOGPU *gpu, VirtIOHeader *header);

#endif //_K_VIRTIO_GPU_H
