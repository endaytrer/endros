#include <string.h>
#include "virtio.h"
#include "../mem.h"
#include "../printk.h"
#include "../pagetable.h"

i32 header_init(VirtIOHeader *hdr, u32 supported_features){
    // write status to be driver
    hdr->status = 0;
    hdr->status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;

    // write device features
    u32 device_features = hdr->device_features;
    device_features &= supported_features;
    hdr->device_features_sel = device_features;

    // write status to be feature_ok
    hdr->status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK;

    if (hdr->version == VIRTIO_VERSION_LEGACY)
        hdr->legacy_guest_page_size = PAGESIZE;

    return device_features;
}

void finish_init(VirtIOHeader *hdr) {
    hdr->status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK;
}

bool queue_used(VirtIOHeader *hdr, u16 idx) {
    hdr->queue_sel = idx;
    u32 version = hdr->version;
    if (version == VIRTIO_VERSION_LEGACY) {
        return hdr->legacy_queue_pfn != 0;
    } else {
        return hdr->queue_ready != 0;
    }
}

/// queue part size: 
///
/// desc = sizeof(VirtQueueDesc) * queue_size
/// avail = sizeof(u16) * (3 + queue_size)
/// used = sizeof(u16) * 3 + sizeof(struct used_elem) * queue_size


void init_queue(VirtIOQueue *queue, VirtIOHeader *hdr, u16 idx, bool indirect, bool event_idx) {
    if (queue_used(hdr, idx)) {
        panic("[kernel.drivers.virtio] queue is used\n");
    }
    if (VIRTIO_NUM_DESC > hdr->queue_num_max) {
        panic("[kernel.drivers.virtio] queue is less than size\n");
    }
    queue->queue_idx = idx;
    VirtQueueDesc *desc;
    VirtQueueAvail *avail;
    VirtQueueUsed *used;

    if (hdr->version == VIRTIO_VERSION_LEGACY) {
        /// allocate legacy. Assert within one page, since we only allocate one page here.
        /// In legacy mode, all three queues are assigned in a single page.

        // vpn_t dma_vpn;
        // pfn_t dma_pfn = uptalloc(&dma_vpn);
        // since we need TWO CONTIGUOUS PHYSICAL pages, we have to MANUALLY ALLOCATE by changing pbrk and pfn_start

        extern void *ptbrk;
        extern pfn_t pfn_start;
        vpn_t dma_vpn = ADDR_2_PAGE(ptbrk);
        pfn_t dma_pfn = pfn_start;
        ptbrk = (void *)((u64)ptbrk + 2 * PAGESIZE);
        pfn_start += 2;
        // map
        ptmap(dma_vpn, dma_pfn, PTE_VALID | PTE_READ | PTE_WRITE);
        ptmap(dma_vpn + 1, dma_pfn + 1, PTE_VALID | PTE_READ | PTE_WRITE);


        queue->layout.legacy.dma.vpn = dma_vpn;
        queue->layout.legacy.dma.pfn = dma_pfn;
        queue->layout.legacy.dma.pages = 2;
        desc = (VirtQueueDesc *)PAGE_2_ADDR(dma_vpn);
        avail = (VirtQueueAvail *)ADDR(dma_vpn, sizeof(VirtQueueDesc) * VIRTIO_NUM_DESC);
        used = (VirtQueueUsed *)PAGE_2_ADDR(dma_vpn + 1);

        queue->layout.legacy.avail_offset = sizeof(VirtQueueDesc) * VIRTIO_NUM_DESC;
        queue->layout.legacy.used_offset = PAGESIZE;

        // u64 paddr_desc = (u64)PAGE_2_ADDR(dma_pfn);
        // u64 paddr_avail = (u64)ADDR(dma_pfn, sizeof(VirtQueueDesc) * VIRTIO_NUM_DESC);
        // u64 paddr_used = (u64)ADDR(dma_pfn, sizeof(VirtQueueDesc) * VIRTIO_NUM_DESC + sizeof(VirtQueueAvail));

        // Select queue
        hdr->queue_sel = idx;

        // set queue size to VIRTIO_NUM_DESC
        hdr->queue_num = VIRTIO_NUM_DESC;

        // set legacy align
        hdr->legacy_queue_align = PAGESIZE;

        // set pfn
        hdr->legacy_queue_pfn = dma_pfn;

    } else {
        /// In modern mode, desc and avail are assigned (from driver to device) on the same page, but used (from device to driver) is on another page.
        /// This is for transformation efficiency.

        // allocate flexible.
        vpn_t driver_to_device_vpn;
        pfn_t driver_to_device_pfn = uptalloc(&driver_to_device_vpn);

        vpn_t device_to_driver_vpn;
        pfn_t device_to_driver_pfn = uptalloc(&device_to_driver_vpn);

        queue->layout.modern.driver_to_device_dma = (struct dma_t) {
            .pages = 1,
            .pfn = driver_to_device_pfn,
            .vpn = driver_to_device_vpn
        };

        queue->layout.modern.device_to_driver_dma = (struct dma_t) {
            .pages = 1,
            .pfn = device_to_driver_pfn,
            .vpn = device_to_driver_vpn
        };
        desc = (VirtQueueDesc *)PAGE_2_ADDR(driver_to_device_vpn);
        avail = (VirtQueueAvail *)ADDR(driver_to_device_vpn, sizeof(VirtQueueDesc) * VIRTIO_NUM_DESC);
        used = (VirtQueueUsed *)PAGE_2_ADDR(device_to_driver_vpn);

        queue->layout.modern.avail_offset = sizeof(VirtQueueDesc) * VIRTIO_NUM_DESC;

        u64 paddr_desc = (u64)PAGE_2_ADDR(driver_to_device_pfn);
        u64 paddr_avail = (u64)ADDR(driver_to_device_pfn, sizeof(VirtQueueDesc) * VIRTIO_NUM_DESC);
        u64 paddr_used = (u64)PAGE_2_ADDR(device_to_driver_pfn);

        // Select queue
        hdr->queue_sel = idx;

        // set queue size to VIRTIO_NUM_DESC
        hdr->queue_num = VIRTIO_NUM_DESC;


        hdr->queue_desc_low = paddr_desc;
        hdr->queue_desc_high = paddr_desc >> 32;
        hdr->queue_driver_low = paddr_avail;
        hdr->queue_driver_high = paddr_avail >> 32;
        hdr->queue_device_low = paddr_used;
        hdr->queue_device_high = paddr_used >> 32;
        hdr->queue_ready = 1;
    }
    // linking descriptors together
    queue->desc = desc;
    queue->avail = avail;
    queue->used = used;
    queue->num_used = 0;
    queue->free_head = 0;
    queue->avail_idx = 0;
    queue->last_used_idx = 0;
    queue->event_idx = event_idx;
    queue->indirect = indirect;
    memset(queue->indirect_lists, 0, sizeof(queue->indirect_lists));
    memset((void *)queue->desc_shadow, 0, sizeof(queue->desc_shadow));
    for (int i = 0; i < VIRTIO_NUM_DESC - 1; i++) {
        queue->desc_shadow[i].next = i + 1;
        desc[i].next = i + 1;
    }
}
i32 add_queue(VirtIOQueue *queue, pfn_t inputs[], u32 input_lengths[], u8 num_inputs, pfn_t outputs[], u32 output_lengths[], u8 num_outputs) {
    u8 needed = num_inputs + num_outputs;
    if (queue->num_used >= VIRTIO_NUM_DESC || needed > VIRTIO_NUM_DESC || (!queue->indirect && queue->num_used + needed > VIRTIO_NUM_DESC)) {
        return -1;
    }
    u16 head = queue->free_head;
    if (queue->indirect && needed > 1) {
        // add indirect
        /// assert size
        if (needed * sizeof(VirtQueueDesc) > PAGESIZE) {
            panic("[kernel.drivers.virtio] trying to insert more than pagesize blocks\n");
        }
        vpn_t indirect_vpn;
        pfn_t indirect_pfn = uptalloc(&indirect_vpn);
        VirtQueueDesc *indirect_list = PAGE_2_ADDR(indirect_vpn);
        for (u16 i = 0; i < num_inputs; i++) {
            indirect_list[i].addr = (u64)PAGE_2_ADDR(inputs[i]);
            indirect_list[i].len = input_lengths[i];
            indirect_list[i].flags = VRING_DESC_F_NEXT;
            indirect_list[i].next = i + 1;
        }
        for (u16 i = 0; i < num_outputs; i++) {
            indirect_list[num_inputs + i].addr = (u64)PAGE_2_ADDR(outputs[i]);
            indirect_list[num_inputs + i].len = output_lengths[i];
            indirect_list[num_inputs + i].flags = VRING_DESC_F_WRITE;
            if (i != num_outputs - 1) {
                indirect_list[num_inputs + i].flags |= VRING_DESC_F_NEXT;
            }
            indirect_list[num_inputs + i].next = num_inputs + i + 1;
        }
        // Need to store pointer to indirect_list too, because direct_desc.set_buf will only store
        // the physical DMA address which might be different.
        if (queue->indirect_lists[head]) {
            panic("[kernel.drivers.virtio] queue indirect list head is not null\n");
        }
        queue->indirect_lists[head] = indirect_list;
        // Write a descriptor pointting to indirect descriptor list.
        VirtQueueDesc *direct_desc = queue->desc_shadow + head;
        queue->free_head = direct_desc->next;
        direct_desc->addr = (u64)PAGE_2_ADDR(indirect_pfn);
        // need to be size of the whole list
        direct_desc->len = sizeof(VirtQueueDesc) * needed; 
        direct_desc->flags = VRING_DESC_F_INDIRECT;
        // write from desc_shadow to desc
        queue->desc[head] = queue->desc_shadow[head];
        queue->num_used++;
    } else {
        // add direct
        u16 last = queue->free_head;
        for (u16 i = 0; i < num_inputs; i++) {
            VirtQueueDesc *desc = queue->desc_shadow + queue->free_head;
            desc->addr = (u64)PAGE_2_ADDR(inputs[i]);
            desc->len = input_lengths[i];
            desc->flags = VRING_DESC_F_NEXT;
            last = queue->free_head;
            queue->free_head = desc->next;
            queue->desc[last] = queue->desc_shadow[last];
        }
        for (u16 i = 0; i < num_outputs; i++) {
            VirtQueueDesc *desc = queue->desc_shadow + queue->free_head;
            desc->addr = (u64)PAGE_2_ADDR(outputs[i]);
            desc->len = output_lengths[i];
            desc->flags = VRING_DESC_F_NEXT | VRING_DESC_F_WRITE;
            last = queue->free_head;
            queue->free_head = desc->next;
            queue->desc[last] = queue->desc_shadow[last];
        }
    }
    u16 avail_slot = queue->avail_idx & (VIRTIO_NUM_DESC - 1); // using & to warp

    queue->avail->ring[avail_slot] = head;

    // fence, adding avail slot index;
    __sync_synchronize();
    queue->avail_idx++;
    queue->avail->idx = queue->avail_idx;
    __sync_synchronize();
    return head;
}