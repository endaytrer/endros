//
// Created by endaytrer on 2023/10/2.
//

#ifndef _K_VIRTIO_GPU_H
#define _K_VIRTIO_GPU_H
#include "virtio.h"

typedef struct {
    VirtIOHeader *hdr;
    u32 width;
    u32 height;
    struct dma_t frame_buffer_dma;
    struct dma_t cursor_buffer_dma;
    VirtIOQueue control_queue;
    VirtIOQueue cursor_queue;
    struct dma_t queue_buf_dma;
    struct dma_t queue_buf_send;
    /// @brief  Capacity in sectors(512 bytes)
    u64 capacity;
    u32 negotiated_features;
} VirtIOGPU;


#endif //_K_VIRTIO_GPU_H
