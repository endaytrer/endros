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

        // since we need TWO CONTIGUOUS PHYSICAL pages, we have to MANUALLY ALLOCATE by changing pbrk and pfn_start
        vpn_t dma_vpn;
        pfn_t dma_pfn = dmalloc(&dma_vpn, 2, true);

        desc = (VirtQueueDesc *)PAGE_2_ADDR(dma_vpn);
        avail = (VirtQueueAvail *)ADDR(dma_vpn, sizeof(VirtQueueDesc) * VIRTIO_NUM_DESC);
        used = (VirtQueueUsed *)PAGE_2_ADDR(dma_vpn + 1);

        queue->layout.legacy.avail_offset = sizeof(VirtQueueDesc) * VIRTIO_NUM_DESC;
        queue->layout.legacy.used_offset = PAGESIZE;

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
        pfn_t driver_to_device_pfn = dmalloc(&driver_to_device_vpn, 1, true);

        vpn_t device_to_driver_vpn;
        pfn_t device_to_driver_pfn = dmalloc(&device_to_driver_vpn, 1, true);

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
        pfn_t indirect_pfn = dmalloc(&indirect_vpn, 1, false);
        VirtQueueDesc *indirect_list = (VirtQueueDesc *)PAGE_2_ADDR(indirect_vpn);
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
        queue->indirect_pfns[head] = indirect_pfn;
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
        u16 last;
        for (u16 i = 0; i < num_inputs; i++) {
            VirtQueueDesc *desc = queue->desc_shadow + queue->free_head;
            desc->addr = inputs[i];
            desc->len = input_lengths[i];
            desc->flags = VRING_DESC_F_NEXT;
            last = queue->free_head;
            queue->free_head = desc->next;
            queue->desc[last] = queue->desc_shadow[last];
            queue->num_used++;
        }
        for (u16 i = 0; i < num_outputs; i++) {
            VirtQueueDesc *desc = queue->desc_shadow + queue->free_head;
            desc->addr = outputs[i];
            desc->len = output_lengths[i];
            desc->flags = VRING_DESC_F_WRITE;
            if (i != num_outputs - 1)
                desc->flags |= VRING_DESC_F_NEXT;
            last = queue->free_head;
            queue->free_head = desc->next;
            queue->desc[last] = queue->desc_shadow[last];
            queue->num_used++;
        }
    }
    u16 avail_slot = queue->avail_idx & (VIRTIO_NUM_DESC - 1); // using & to warp

    // fence, adding avail slot index;
    queue->avail->ring[avail_slot] = head;
    __sync_synchronize();
    queue->avail_idx += 1;
    queue->avail->idx = queue->avail_idx;
    __sync_synchronize();
    return head;
}

i64 queue_add_notify_pop(VirtIOQueue *queue, VirtIOHeader *hdr, pfn_t inputs[], u32 input_lengths[], u8 num_inputs, pfn_t outputs[], u32 output_lengths[], u8 num_outputs) {
    i32 token = add_queue(queue, inputs, input_lengths, num_inputs, outputs, output_lengths, num_outputs);
    if (token < 0) {
        printk("[kernel.drivers.virtio] add queue failed\n");
        return -1;
    }
    // write notify register to notify device
    if (should_notify(queue))
        hdr->queue_notify = queue->queue_idx;
    while (1) {
        __sync_synchronize();
        if (queue->last_used_idx != queue->used->idx)
            break;
    }
    u16 last_used_slot = queue->last_used_idx & (VIRTIO_NUM_DESC - 1);
    u16 index = (u16)queue->used->ring[last_used_slot].id;
    u32 len = queue->used->ring[last_used_slot].len;

    if (index != token) {
        printk("[kernel.drivers.virtio] used token error\n");
        return -1;
    }
    recycle_descriptors(queue, index);
    queue->last_used_idx++;
    return len;
}

void recycle_descriptors(VirtIOQueue *queue, u16 head) {
    u16 original_free_head = queue->free_head;
    queue->free_head = head;

    VirtQueueDesc *head_desc = queue->desc_shadow + head;
    if (head_desc->flags & VRING_DESC_F_INDIRECT) {
        VirtQueueDesc *indirect_list = queue->indirect_lists[head];
        pfn_t pfn = queue->indirect_pfns[head];
        head_desc->addr = 0;
        head_desc->len = 0;
        queue->num_used -= 1;
        head_desc->next = original_free_head;

        // dealloc. since we allocate this using dmalloc(, 1); we use dmafree(, 1);
        // we do not have to free the addresses pointed from indirect list:
        // - req / resp are handelled 
        dmafree(pfn, ADDR_2_PAGE(indirect_list), 1);
        queue->indirect_lists[head] = NULL;
        // apply changes. Don't know if it is necessary.
    } else {
        u16 next = head;
        while (1) {
            VirtQueueDesc *desc = queue->desc_shadow + next;
            desc->addr = 0;
            desc->len = 0;
            queue->num_used -= 1;
            if (desc->flags & VRING_DESC_F_NEXT) {
                queue->desc[next] = *desc;
                next = desc->next;
            } else {
                desc->next = original_free_head;
                break;
            }
        }
    }
}
bool should_notify(VirtIOQueue *queue) {
    __sync_synchronize();
    if (queue->event_idx) {
        u16 avail_event = queue->used->avail_event;
        return queue->avail_idx >= ((avail_event + 1) & (VIRTIO_NUM_DESC - 1));
    } else {
        return (queue->used->flags & 0x0001) == 0;
    }
}