#ifndef _K_MACHINE_SPEC
#define _K_MACHINE_SPEC
#include "drivers/virtio.h"
#define MAX_VIRTIO_MMIO 64

extern u64 max_pfn;
extern u32 clock_freq;
extern u64 nCPU;
extern u64 num_virtio_mmio;
extern VirtIOHeader *virtio_mmio_headers[MAX_VIRTIO_MMIO];
#endif