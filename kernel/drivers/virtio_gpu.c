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
    printk(", resolution ");
    // init queue
    init_queue(&gpu->control_queue, header, 0, false, (negotiated_features & VIRTIO_FEATURE_RING_EVENT_IDX) != 0);
    init_queue(&gpu->cursor_queue, header, 1, false, (negotiated_features & VIRTIO_FEATURE_RING_EVENT_IDX) != 0);
    finish_init(header);
    vpn_t queue_buf_vpn;
    pfn_t queue_buf_pfn = dmalloc(&queue_buf_vpn, 2, false);

    gpu->queue_buf_dma = (struct dma_t) {
        .pages = 2,
        .pfn = queue_buf_pfn,
        .vpn = queue_buf_vpn
    };
    gpu->queue_buf_send = (u8 *)PAGE_2_ADDR(queue_buf_vpn);
    gpu->queue_buf_recv = (u8 *)PAGE_2_ADDR(queue_buf_vpn + 1);
    gpu->hdr = header;

    // fill display info
    RespDisplayInfo *info = get_display_info(gpu);
    gpu->x = info->x;
    gpu->y = info->y;
    gpu->width = info->width;
    gpu->height = info->height;
    gpu->size = info->width * info->height * 4;
    printk(itoa(gpu->width, buf, 10));
    printk("x");
    printk(itoa(gpu->height, buf, 10));
    printk("\n");
    setup_framebuffer(gpu);
}

RespDisplayInfo *get_display_info(VirtIOGPU *gpu) {
    *(ControlHdr *)gpu->queue_buf_send = (ControlHdr) {
        .hdr_type = GET_DISPLAY_INFO,
        .flags = 0,
        .fence_id = 0,
        .ctx_id = 0,
        ._padding = 0
    };
    u64 inputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn)};
    u32 input_lengths[] = {sizeof(ControlHdr)};
    u64 outputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn + 1)};
    u32 output_lengths[] = {sizeof(RespDisplayInfo)};
    queue_add_notify_pop(&gpu->control_queue, gpu->hdr, inputs, input_lengths, 1, outputs, output_lengths, 1);
    return (RespDisplayInfo *)gpu->queue_buf_recv;
}
i64 resource_create_2d(VirtIOGPU *gpu, u32 resource_id, u32 width, u32 height) {
    *(ReqResourceCreate2D *)gpu->queue_buf_send = (ReqResourceCreate2D) {
        .hdr = (ControlHdr) {
            .hdr_type = RESOURCE_CREATE_2D,
            .flags = 0,
            .fence_id = 0,
            .ctx_id = 0,
            ._padding = 0
        },
        .format = B8G8R8A8UNORM,
        .width = width,
        .height = height,
        .resource_id = resource_id
    };
    u64 inputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn)};
    u32 input_lengths[] = {sizeof(ReqResourceCreate2D)};
    u64 outputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn + 1)};
    u32 output_lengths[] = {sizeof(ControlHdr)};
    queue_add_notify_pop(&gpu->control_queue, gpu->hdr, inputs, input_lengths, 1, outputs, output_lengths, 1);
    return ((ControlHdr *)gpu->queue_buf_recv)->hdr_type == OK_NODATA ? 0 : -1;
}
i64 resource_attach_backing(VirtIOGPU *gpu, u32 resource_id, u64 paddr, u32 length) {
    *(ReqResourceAttachBacking *)gpu->queue_buf_send = (ReqResourceAttachBacking) {
        .hdr = (ControlHdr) {
            .hdr_type = RESOURCE_ATTACH_BACKING,
            .flags = 0,
            .fence_id = 0,
            .ctx_id = 0,
            ._padding = 0
        },
        .resource_id = resource_id,
        .nr_entries = 1,
        .addr = paddr,
        .length = length,
        ._padding = 0
    };
    u64 inputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn)};
    u32 input_lengths[] = {sizeof(ReqResourceAttachBacking)};
    u64 outputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn + 1)};
    u32 output_lengths[] = {sizeof(ControlHdr)};
    queue_add_notify_pop(&gpu->control_queue, gpu->hdr, inputs, input_lengths, 1, outputs, output_lengths, 1);
    return ((ControlHdr *)gpu->queue_buf_recv)->hdr_type == OK_NODATA ? 0 : -1;
}
i64 set_scanout(VirtIOGPU *gpu, u32 x, u32 y, u32 width, u32 height, u32 scanout_id, u32 resource_id) {
    *(ReqSetScanout *)gpu->queue_buf_send = (ReqSetScanout) {
        .hdr = (ControlHdr) {
            .hdr_type = SET_SCANOUT,
            .flags = 0,
            .fence_id = 0,
            .ctx_id = 0,
            ._padding = 0
        },
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .resource_id = resource_id,
        .scanout_id = scanout_id
    };
    u64 inputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn)};
    u32 input_lengths[] = {sizeof(ReqSetScanout)};
    u64 outputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn + 1)};
    u32 output_lengths[] = {sizeof(ControlHdr)};
    queue_add_notify_pop(&gpu->control_queue, gpu->hdr, inputs, input_lengths, 1, outputs, output_lengths, 1);
    return ((ControlHdr *)gpu->queue_buf_recv)->hdr_type == OK_NODATA ? 0 : -1;
}
i64 transfer_to_host_2d(VirtIOGPU *gpu, u32 x, u32 y, u32 width, u32 height, u64 offset, u32 resource_id) {
    *(ReqTransferToHost2D *)gpu->queue_buf_send = (ReqTransferToHost2D) {
        .hdr = (ControlHdr) {
            .hdr_type = TRANSFER_TO_HOST_2D,
            .flags = 0,
            .fence_id = 0,
            .ctx_id = 0,
            ._padding = 0
        },
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .offset = offset,
        .resource_id = resource_id,
        ._padding = 0
    };
    u64 inputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn)};
    u32 input_lengths[] = {sizeof(ReqTransferToHost2D)};
    u64 outputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn + 1)};
    u32 output_lengths[] = {sizeof(ControlHdr)};
    queue_add_notify_pop(&gpu->control_queue, gpu->hdr, inputs, input_lengths, 1, outputs, output_lengths, 1);
    return ((ControlHdr *)gpu->queue_buf_recv)->hdr_type == OK_NODATA ? 0 : -1;
}

i64 resource_flush(VirtIOGPU *gpu, u32 x, u32 y, u32 width, u32 height, u32 resource_id) {
    *(ReqResourceFlush *)gpu->queue_buf_send = (ReqResourceFlush) {
        .hdr = (ControlHdr) {
            .hdr_type = TRANSFER_TO_HOST_2D,
            .flags = 0,
            .fence_id = 0,
            .ctx_id = 0,
            ._padding = 0
        },
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .resource_id = resource_id,
        ._padding = 0
    };

    u64 inputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn)};
    u32 input_lengths[] = {sizeof(ReqResourceFlush)};
    u64 outputs[] = {PAGE_2_ADDR(gpu->queue_buf_dma.pfn + 1)};
    u32 output_lengths[] = {sizeof(ControlHdr)};
    queue_add_notify_pop(&gpu->control_queue, gpu->hdr, inputs, input_lengths, 1, outputs, output_lengths, 1);
    return ((ControlHdr *)gpu->queue_buf_recv)->hdr_type == OK_NODATA ? 0 : -1;
}

i64 setup_framebuffer(VirtIOGPU *gpu) {
    if (resource_create_2d(gpu, RESOURCE_ID_FB, gpu->width, gpu->height) < 0)
        return -1;
    u32 size = gpu->width * gpu->height * 4;
    u32 size_pages = ADDR_2_PAGEUP(size);
    vpn_t fb_vpn;
    pfn_t fb_pfn = dmalloc(&fb_vpn, size_pages, false);
    gpu->frame_buffer_dma.pages = size_pages;
    gpu->frame_buffer_dma.vpn = fb_vpn;
    gpu->frame_buffer_dma.pfn = fb_pfn;

    if (resource_attach_backing(gpu, RESOURCE_ID_FB, PAGE_2_ADDR(gpu->frame_buffer_dma.pfn), size) < 0)
        return -1;
    if (set_scanout(gpu, gpu->x, gpu->y, gpu->width, gpu->height, SCANOUT_ID, RESOURCE_ID_FB) < 0)
        return -1;
    return 0;
}

i64 flush(VirtIOGPU *gpu) {
    if (transfer_to_host_2d(gpu, gpu->x, gpu->y, gpu->width, gpu->height, 0, RESOURCE_ID_FB) < 0)
        return -1;
    if (resource_flush(gpu, gpu->x, gpu->y, gpu->width, gpu->height, RESOURCE_ID_FB) < 0)
        return -1;
    return 0;
}
i64 fb_read(VirtIOGPU *gpu, u64 offset, void *data, u64 len) {
    /// Framebuffer is write only. Reading it is equivalent to flushing it.
    flush(gpu);
    return 0;
}
i64 fb_write(VirtIOGPU *gpu, u64 offset, const void *data, u64 len) {
    u64 fb = PAGE_2_ADDR(gpu->frame_buffer_dma.vpn);
    memcpy((void *)(fb + offset), data, len);
    return 0;

}

void wrap_virtio_gpu_file(File *out, VirtIOGPU *in) {
    out->super = in;
    out->type = DEVICE;
    out->permission = PERMISSION_R | PERMISSION_W;
    out->size = &in->size;
    out->read = (i64(*)(void *, u64, void *, u64))fb_read;
    out->write = (i64(*)(void *, u64, const void *, u64))fb_write;
}