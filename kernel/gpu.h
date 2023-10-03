#ifndef _K_GPU_H
#define _K_GPU_H
#include "drivers/virtio_gpu.h"
#define RGBA(r, g, b, a) ((((a) & 255) << 24) | (((r) & 255) << 16) | (((g) & 255) << 8) | ((b) & 255))
extern VirtIOGPU virtio_gpu;
extern File gpu_device;

void init_gpu(VirtIOHeader *gpu_header);
void rainbow(void);

#endif