#include "gpu.h"

void rainbow(void) {
    u32 *bf = (u32 *)PAGE_2_ADDR(virtio_gpu.frame_buffer_dma.vpn);
    for (u32 y = 0; y < virtio_gpu.height; y++) {
        for (u32 x = 0; x < virtio_gpu.width; x++) {
            u32 idx  = (y * virtio_gpu.width + x);
            bf[idx] = RGBA(x, y, x + y, 255);
        }
    }
    flush(&virtio_gpu);
}
void init_gpu(VirtIOHeader *gpu_header) {
    if (gpu_header == (VirtIOHeader *)(~((u64)0))) {
        return;
    }
    init_virtio_gpu(&virtio_gpu, gpu_header);
    wrap_virtio_gpu_file(&gpu_device, &virtio_gpu);
    rainbow();
}