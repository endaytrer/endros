//
// Created by endaytrer on 2023/10/2.
//
#include <string.h>
#include "virtio_gpu.h"
#include "../printk.h"
#include "../pagetable.h"

void init_virtio_gpu(VirtIOGPU *gpu, VirtIOHeader *header){
    u32 negotiated_features = header_init(header, VIRTIO_GPU_SUPPORTED_FEATURES);
    // in MMIO mod, the config space is just after the header;
    GPUConfig *config_space = (GPUConfig *)(header + 1);

   u32 events_read = config_space->events_read;
   u32 num_scanouts = config_space->num_scanouts;

    printk("Find a GPU device with events_read ");
    char buf[16];
    printk(itoa(events_read, buf, 10));
    printk(", num_scanouts ");
    printk(itoa(num_scanouts, buf, 10));
    printk("\n");
    // init queue
    init_queue(&gpu->control_queue, header, 0, false, (negotiated_features & VIRTIO_FEATURE_RING_EVENT_IDX) != 0);
    init_queue(&gpu->cursor_queue, header, 1, false, (negotiated_features & VIRTIO_FEATURE_RING_EVENT_IDX) != 0);
    gpu->queue_buf_send = kalloc(PAGESIZE);
    gpu->queue_buf_recv = kalloc(PAGESIZE);
    gpu->hdr = header;
}